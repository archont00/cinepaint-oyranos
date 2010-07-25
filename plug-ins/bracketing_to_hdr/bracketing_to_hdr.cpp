/*
 * bracketing_to_hdr.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
 *
 * Copyright (c) 2005-2006  Hartmut Sbosny  <hartmut.sbosny@gmx.de>
 *
 * LICENSE:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/**
  @file bracketing_to_hdr.cpp
  
  Main source file of the CinePaint plugin "Bracketing to HDR".
  
  This file contains all CinePaint and Gtk references of the "Bracketing"
   project. The only two services we need from CPaint are loading an image and
   showing an float32 (HDR) image: wrapped and provided to the rest of the 
   program by the two functions declared in "bracketing_to_hdr.hpp".
  
  @internal 
   \c GPrecisionType is defined in "cinepaint???/lib/wire/precision.h" as an
   enum. There are also precision strings \c PRECISION_U8_STRING etc,
   unfortunately not in an array to have a simple reference to the enums.
    
  @bug If "Br2HdrManager.hpp" && "AllWindows.hpp" && "bracketings_to_hdr.hpp" 
   are included *after* the CPaint headers it gives the compiler error:<pre>
     In file included from /usr/include/g++/i586-suse-linux/bits/c++locale.h:44,
                 from /usr/include/g++/iosfwd:46,
                 from /usr/include/g++/bits/stl_algobase.h:70,
                 from /usr/include/g++/vector:67,
                 from br_core/br_Image.hpp:31,
                 from br_core/Br2HdrManager.hpp:19,
                 from bracketing_to_hdr.hpp:11,
                 from bracketing_to_hdr.cpp:49:
     /usr/include/libintl.h:40: error: syntax error before `char'
    </pre>
   No idea why, anyhow, thus CPaint headers included behind those.
*/

#include <FL/fl_ask.H>              // fl_alert(), fl_ask()
#include <FL/filename.H>            // fl_filename_name()
#include "br_core/br_version.hpp"   // BR_VERSION_LSTRING    
#include "br_core/Rgb.hpp"          // Rgb<>
#include "br_core/br_macros.hpp"    // IF_FAIL_DO() etc.
#include "br_core/Br2HdrManager.hpp"
#include "br_core/br_messages.hpp"  // br::msg_image_ignored(), e_printf()
#include "br_core/readEXIF.hpp"     // readEXIF()
#include "gui/AllWindows.hpp"       // object `allWins'
#include "bracketing_to_hdr.hpp"    // cpaint_load_image(), cpaint_show_image()
extern "C" {
#include "lib/plugin_main.h"
#include "lib/wire/libtile.h"
#include "plugin_pdb.h"
#include "libgimp/stdplugins-intl.h" // macros INIT_I18N_UI(), _()
}


//==================================================
//  Info strings (macros) for the CinePaint plugin
//==================================================
#define PLUGIN_NAME          "plug_in_bracketing_to_hdr"
#define PLUGIN_BRIEF         "Merges bracketed exposures into one HDR image."
#define PLUGIN_DESCRIPTION   "Loads bracketed exposures of a (still) scene and merges them into one High Dynamic Range image (radiance map)."
#define PLUGIN_VERSION       BR_VERSION_LSTRING
#define PLUGIN_AUTHOR        "Hartmut Sbosny <hartmut.sbosny@gmx.de>"
#define PLUGIN_COPYRIGHT     "Copyright 2005-2006 Hartmut Sbosny"


//==================================
//  Declaration of local functions
//==================================
void            query                           (void);
void            run                             (char*     name,
                                                 int       nparams,
                                                 GParam*   param,
                                                 int*      nreturn_vals,
                                                 GParam**  return_vals);

int             cpaint_load_image_via_PDB       (const char* fname, bool interactive);
br::BrImage     u8_from_u16                     (const br::BrImage & img16);
br::BrImage     cpaint_get_BrImage_U8           (int W, int H, gint32 layer_ID);
br::BrImage     cpaint_get_BrImage_U16          (int W, int H, gint32 layer_ID);
void            cpaint_show_rgb_imgbuf          (int W, int H, const uchar* imgbuf,
                                                 GPrecisionType );
const char*     cpaint_precision_name           (GPrecisionType);

template<typename T> 
void            copy_without_alpha              (br::Image &, GimpPixelRgn &);
                                                 

//==========================
//  file-global variables      
//==========================
GPlugInInfo  PLUG_IN_INFO =     // required by macro `MAIN()'
{
    NULL,    /* init_proc  */
    NULL,    /* quit_proc  */
    query,   /* query_proc */
    run,     /* run_proc   */
};

typedef struct { char name[1024]; }  Vals;
Vals  vals;      // for communication between CPaint and the plugin

static int n_return_vals_;
static int n_args_;


//==========================================
//
//              IMPLEMENTATION
//
//==========================================
MAIN()      // Macro defined in "lib/plugin_main.h"; requires a global GPlugInInfo
            //  object of name "PLUG_IN_INFO" (defined just above).   

            
/**+*************************************************************************\n
  querry()
******************************************************************************/
void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "directory",    "directory" },
  };
  static GParamDef return_vals [] =
  {
    { PARAM_IMAGE,      "image",        "Output Image" },
  };
  n_args_        = sizeof (args) / sizeof (args[0]);
  n_return_vals_ = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure (PLUGIN_NAME,
                          PLUGIN_BRIEF,
                          PLUGIN_DESCRIPTION,
                          PLUGIN_AUTHOR,
                          PLUGIN_COPYRIGHT,
                          PLUGIN_VERSION,
                          "<Toolbox>/File/New From/Bracketing to HDR",
                          "*",
                          PROC_EXTENSION,
                          n_args_, n_return_vals_,
                          args, return_vals);
  
  /*  To get this menupath atom in the ~.po file */
  _("Bracketing to HDR");
}

/**+*************************************************************************\n
  run()
******************************************************************************/
void
run (char*    name,             // I - Name of called function resp. plug-in
     int      nparams,          // I - Number of parameters passed in
     GParam*  param,            // I - Parameter values
     int*     nreturn_vals,     // O - Number of return values
     GParam** return_vals)      // O - Return values
{
  GStatusType    status;        // return status
  GRunModeType   run_mode;
  static GParam  values [2];    // return parameter
  int            result = -1;
 
  status                  = STATUS_SUCCESS;
  run_mode                = (GRunModeType) param[0].data.d_int32;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  *nreturn_vals           = n_return_vals_;
  *return_vals            = values;

  /*  Init the internationalization stuff */
  INIT_I18N_UI();
  INIT_FLTK1_CODESET();
  
  if (strcmp (name, PLUGIN_NAME) == 0)
    {
      sprintf (vals.name, getenv("PWD"));     // default for `vals'
    
      gimp_get_data (PLUGIN_NAME, &vals);     // get data from last call
      //printf ("nach gimp_get_data(): vals = \"%s\"\n", vals.name);
     
      switch (run_mode)
        {
        case RUN_INTERACTIVE:
            result = allWins.run();
            if (result < 0) 
              {
                status = STATUS_EXECUTION_ERROR;
              }
            break;

        case RUN_NONINTERACTIVE:    // Makes this sense? Not realy.
            if (nparams != n_args_) 
              {
                status = STATUS_CALLING_ERROR;
                break;
              }
            result = allWins.run();  
            if (result < 0) 
              {
                status = STATUS_EXECUTION_ERROR;
                break;
              }
            *nreturn_vals = n_return_vals_ + 1;
            break;
    
        case RUN_WITH_LAST_VALS:
            break;

        default:
            break;
        }
      
      values[1].type = PARAM_IMAGE;
      values[1].data.d_image = result;
    }
  
  values[0].data.d_status = status;
  gimp_set_data (PLUGIN_NAME, &vals, sizeof(Vals));  // data for next call
}


/**+*************************************************************************\n
  u8_from_u16()  --  helper for cpaint_load_image().

  Function downsamples the RGB_U16 br::BrImage object \a img16 into an RGB_U8 
   br::BrImage object allocated here. Meta data of \a img16 are copied.
   
  @param img16: br::BrImage of type RGB_U16.
  @return  downsampled br::BrImage of type RGB_U8, created here.
******************************************************************************/
br::BrImage 
u8_from_u16 (const br::BrImage & img16)
{
  //  If input image isn't of U16 type return Null-Image
  IF_FAIL_RETURN (img16.data_type() == br::DATA_U16, br::BrImage());  

  //  Allocate dest image (default settings for meta data)
  br::BrImage  img8 (img16.width(), img16.height(), br::IMAGE_RGB_U8);
  
  Rgb<guint8>*  u8  = (Rgb<guint8>*)  img8.buffer();
  Rgb<guint16>* u16 = (Rgb<guint16>*) img16.buffer();
  
  for (int i=0; i < img8.n_pixel(); i++) 
    {
      u8[i] = u16[i] >> 8;  // shift operator of the Rgb<> template
    }
  
  img8.copy_meta_data (img16);
  return img8;
}

/**+*************************************************************************\n
  cpaint_load_image()

  Function loads via CinePaint the image with filename \a fname and adds it to
   the image container of the br::Br2HdrManager object \a mBr2Hdr.

  @param fname: Name of the image to load.
  @param mBr2Hdr: Br2HdrManager instance the image shall be added to. 
  @param interactive: Has an effect only for loading of raw-data via the rawphoto
   plug-in. To avoid that the rawphoto dialog occurs at every image of a series,
   it should be set true only for the first image and false for the following one.
   Default: true.
  @return  Successful or not?

  @note Do not left behind an unfreed CPaint image!
  @note Erwaegenswert auch Variante, die ein br::BrImage returniert und kein 
    Manager-Argument besitzt. Verringerte Abhaengigkeiten merklich. Es muessten
    dann aber Informationen bereitgestellt ob es sich um das erste Bild des
    Containers handelt, und wenn nein, welches die Daten des ersten Bildes
    sind. Also das Manager-Argument doch nicht so verkehrt. Es sei denn, wir
    lueden erst komplettes Bild und prueften dann.
******************************************************************************/
bool 
cpaint_load_image (const char* fname, br::Br2HdrManager & mBr2Hdr, bool interactive)
{
  static int            first_W;       // |to store the values of the first
  static int            first_H;       // | image in mBr2Hdr's image container,
  static gint           first_gray;    // | which determines the values for all
  static GPrecisionType first_precis;  // | others
  static bool           u16_u8_warning = true; // Downsampling warning?
  char                  tmp[256];
  
  /*  To avoid unsubtantial multi-line entries in the .po-file due to '\n' format
       chars we define coherent strings without format specifiers separately. */
  const char* str_multi_layer = _("More than one layer in image.");
  const char* str_no_layer    = _("No layer in image.");
  const char* str_no_data     = _("Image contains no data.");
  const char* str_no_rgb      = _("Not an RGB image.");
  const char* str_other_dim   = _("Image has other dimension than the previous loaded images:");
  const char* str_other_type  = _("Image has other data type than the previous loaded images:");
  const char* str_rgbgray     = _("Image has different RGB/Gray layout.");
  const char* str_16bitdata   = _("16-bit input data");
  const char* str_downsample  = _("Downsampling to 8-bit will happen");

  printf("%s(\"%s\")...\n", __func__, fname);
  
  int image_ID = cpaint_load_image_via_PDB (fname, interactive);
  if (image_ID < 0)  return false;
    
  gint    nlayers  = -1;             // What for?
  gint32* layers   = gimp_image_get_layers (image_ID, &nlayers);
  gint32  layer_ID = layers[0];
  
  GPrecisionType precis = gimp_drawable_precision (layers[0]);
  gint           gray   = gimp_drawable_gray (layers[0]);

  //  Debug output
  printf ("Image has %d layers; layer_ID = %d, precis = %s, gray = %d\n",
      nlayers, layer_ID, cpaint_precision_name(precis), gray);
  
  int W = gimp_image_width (image_ID);
  int H = gimp_image_height (image_ID);
  
  bool ok = false;
  
  //  Tests for each image, be it the first in the image container or not...
  if (nlayers > 1) {
    br::msg_image_ignored (fl_filename_name(fname), "%s", str_multi_layer);
  } 
  else if (nlayers < 1) {
    br::msg_image_ignored (fl_filename_name(fname), "%s", str_no_layer);        
  } 
  else if (layers[0] <= 0) {
    br::msg_image_ignored (fl_filename_name(fname), "%s", str_no_data);        
  } 
  else if (gray) {
    br::msg_image_ignored (fl_filename_name(fname), "%s", str_no_rgb);        
  }
  else if (! (precis == PRECISION_U8  ||  
              precis == PRECISION_U16) ) {
    br::msg_image_ignored (fl_filename_name(fname), 
        _("Sorry, input images of data type '%s' are not supported."),
        cpaint_precision_name(precis));
  } 
  //  Now differentiate: first image in image container or a following one?
  else if (mBr2Hdr.size() == 0)    // first image  
    {  
      first_W = W;
      first_H = H; 
      first_precis = precis;
      first_gray = gray;
      ok = true;
    }   
  else   // image behind the first: check consistency with the first image...
    {
      if (W != first_W || H != first_H) 
        {
          br::msg_image_ignored (fl_filename_name(fname), 
              "%s\n\t\"%d x %d\"  vs.  \"%d x %d\".",  
              str_other_dim, W, H, first_W, first_H);
        }
      else if ((first_precis != precis) && ! ((first_precis == PRECISION_U8) &&
               (precis == PRECISION_U16) && allWins.main->downsample_U16()) )
        {
          br::msg_image_ignored (fl_filename_name(fname),
              "%s\n\t\"%s\"  vs.  \"%s\".",
              str_other_type, 
              cpaint_precision_name(precis),
              cpaint_precision_name(first_precis));
        } 
      else if (first_gray != gimp_drawable_gray(layers[0]))  // done above in "if (gray)"
        {  
          br::msg_image_ignored (fl_filename_name(fname), "%s", str_rgbgray);
        } 
      else 
        {
          ok = true;
        }
    }
  
  //  Take over the data..
  if (ok)
    { // If allWinw.main->downsample_U16()==true, we offer downsampling to U8
      //  data. To note, that we *must not* scale down each image individually
      //  onto the full 8-bit range, because this would change the relation 
      //  between the various input images. We can only cut "stupidly" to the
      //  8 leading bits. Therefore also converting of single float images to
      //  integers makes no sense, because no reasonable min-max-borders exist.
      //  Would make sense indeed for a *complete* set of float images.
      
      br::BrImage img;   // Null-Image as start
    
      switch (precis)
        {
        case PRECISION_U8:  
          //printf ("uint8\n"); 
          img = cpaint_get_BrImage_U8 (W,H, layer_ID);
          break;
    
        case PRECISION_U16:  
          //printf ("uint16\n"); 
          if (allWins.main->downsample_U16()) {
            if (u16_u8_warning) 
            {
              snprintf (tmp, 256, "\"%s\":  %s\n\n%s",
                  fl_filename_name(fname), str_16bitdata, str_downsample);
              int choice = fl_choice (tmp, _("Cancel"), _("Downsample"), _("No more warn"));
              if (choice == 0)        // Cancel
                {      
                  ok = false; 
                  break;              // skips this image
                }  
              else if (choice == 2)   // "Do no more warn"      
                { 
                  u16_u8_warning = false;
                }  
            }
            img = u8_from_u16 (cpaint_get_BrImage_U16 (W,H, layer_ID));
            //  If it is the first image, also first_precis is now U8
            if (mBr2Hdr.size()==0) first_precis = PRECISION_U8;
          }
          else 
            img = cpaint_get_BrImage_U16 (W,H, layer_ID);            
          break;
    
        case PRECISION_FLOAT:  // currently unreachable
          printf ("f32\n");  ok = false;
          break;
    
        default:  
          //  Might never be reached if tests above are complete
          e_printf ("\"%s\":  Not supported data type #%d: '%s'\n", 
              fl_filename_name(fname), precis, 
              cpaint_precision_name(precis));
          ok = false;
          break;
        }

      //  cpaint_get_BrImage() or u8_from_u16() could have returned Null-images:
      ok = !img.is_null();  
              
      //  Set some meta data and add the image to the Br2HdrManager
      if (ok) 
        { 
          img.name (fname);
          readEXIF (fname, &img.exposure.shutter, &img.exposure.aperture, &img.exposure.ISO);
          img.time_by_EXIF();  // default behaviour
          mBr2Hdr.add_Image (img);
          //mBr2Hdr.report_Images();
        }
    } 
  
  //  Free the image in CinePaint
  gimp_image_delete (image_ID);
  gimp_image_clean_all (image_ID);
  free (layers); 
  
  return ok;
}


/**+*************************************************************************\n
  cpaint_load_image_via_PDB()  --  Helper for cpaint_load_image().

  Function asks CPaint for loading the image with filename \a fname and returns
   its image_ID. Function wraps the PDB and "return_vals" stuff.

  @param fname: Name of file the CPaint image_ID is asked for.
  @param interactive: Has an effect only for loading of raw-data via the rawphoto
   plug-in. To avoid that the rawphoto dialog occurs at every image of a bundle, 
   it should be set true only for the first image and false for the following one.
  @return image_ID  (-1 for failure).
******************************************************************************/
int 
cpaint_load_image_via_PDB (const char* fname, bool interactive)
{
  int         image_ID;
  int         n_retvals;
  GimpParam * return_vals;
  
  return_vals = gimp_run_procedure ("gimp_file_load",
                                    & n_retvals,
                                    GIMP_PDB_INT32, 
                 interactive ? GIMP_RUN_INTERACTIVE : GIMP_RUN_NONINTERACTIVE,
                                    GIMP_PDB_STRING, fname,
                                    GIMP_PDB_STRING, fname,
                                    GIMP_PDB_END);
  
  if (return_vals[0].data.d_status != GIMP_PDB_SUCCESS)
    image_ID = -1;
  else 
    image_ID = return_vals[1].data.d_image;
  
  gimp_destroy_params (return_vals, n_retvals);
  return image_ID;
}


/**+*************************************************************************\n
  cpaint_get_BrImage_U8()  --  Helper for cpaint_load_image().

  Allocates a br::BrImage and fills-in the CPaint image buffer related to 
   \a layer_ID. A possible alpha channel in CPaint's image buffer gets removed. 
   Default settings for name and exposure time and all other meta data. Type
   dependence (U8, U16...) are handled by the template copy_without_alpha<>().
   Could be done also by a br::ImageType parameter and a switch statement.

  @param W,H: width, height of the CPaint layer.
  @param layer_ID: as got by cpaint_load_image(). Data type must be U8!
  @return  An allocated and filled br::BrImage of type RGB_U8
******************************************************************************/
br::BrImage 
cpaint_get_BrImage_U8 (int W, int H, gint32 layer_ID)
{
  //  Allocate the BrImage dest structure.   bad_alloc() possibly!
  br::BrImage  img (W, H, br::IMAGE_RGB_U8);
  
  //  Init the CPaint layer for copying
  GDrawable* drawable = gimp_drawable_get (layer_ID);
  GPixelRgn  pixel_rgn;
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0,0, W,H, false,false);

  if (gimp_drawable_has_alpha (layer_ID))     
    { 
      //  Has alpha: re-copy
      copy_without_alpha <guint8> (img, pixel_rgn);
    }
  else
    {
      //  No alpha -- fill-in directly into img.buffer()
      gimp_pixel_rgn_get_rect (&pixel_rgn, img.buffer(), 0,0, W,H); DBG
    }    
  
  return img;
}


/**+*************************************************************************\n
  cpaint_get_BrImage_U16()  --  Helper for cpaint_load_image().
  
  Description see: cpaint_get_BrImage_U8().
******************************************************************************/
br::BrImage 
cpaint_get_BrImage_U16 (int W, int H, gint32 layer_ID)
{
  //  Allocate BrImage dest structure.   bad_alloc() possibly!
  br::BrImage  img (W, H, br::IMAGE_RGB_U16);
  
  //  Init the CPaint pixel region for copying
  GDrawable* drawable = gimp_drawable_get (layer_ID);
  GPixelRgn  pixel_rgn;
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0,0, W,H, false,false);

  if (gimp_drawable_has_alpha (layer_ID))     
    { 
      //  Has alpha: re-copy  
      copy_without_alpha <guint16> (img, pixel_rgn);
    }
  else  
    { //  No alpha: fill-in directly into img.buffer()
      gimp_pixel_rgn_get_rect (&pixel_rgn, img.buffer(), 0,0, W,H); DBG
    }    
    
  return img;
}

/**+*************************************************************************\n
  copy_without_alpha<>()  --  Helper of cpaint_get_BrImage().

  Function copies the data of the CinePaint pixel region \a rgn, which has an
   alpha channel, into the (extern allocated) br::Image \a img under removing 
   the alpha channel.
  
  @pre \a img.width() and \a img.height() must be the dimensions also of the
    pixel region \a rgn!
  
  @param <T>: Type of the image data (guint8, guint16, ...)
  @param img: br::Image of RGB<T> data. Extern allocated. The destination.
  @param rgn: CPaint pixel region of RGBA<T> data of same dimension as \a img.
               The source.
******************************************************************************/
template <typename T>
void 
copy_without_alpha (br::Image & img, GPixelRgn & rgn)
{
    printf("Remove alpha channel...\n");
    int W = img.width();
    int H = img.height();
    Rgba<T>* buf = (Rgba<T>*) malloc (W * H * sizeof(Rgba<T>));

    gimp_pixel_rgn_get_rect (&rgn, (guchar*)buf, 0,0, W,H);
    
    Rgb<T>* imgbuf = (Rgb<T>*) img.buffer();
    
    for (int i=0; i < W*H; i++)
      {
        imgbuf[i].r = buf[i].r;
        imgbuf[i].g = buf[i].g;
        imgbuf[i].b = buf[i].b;    
      }  
    free (buf);
}


/**+*************************************************************************\n
  cpaint_show_rgb_imgbuf() 
   
  Function plots via CinePaint a contiguous RGB buffer \a imgbuf (without 
   alpha channel!) of data type \a prec. Intended for CPaint plot of an 
   br::BrImage with RGB data or an Array2D< Rgb<T> > array. For messages we use
   the "abstract" function pointer br::v_alert(), so they can appear on the GUI
   screen.

  @param W,H: image dimensions width and height
  @param imgbuf: contiguous (W x H)-buffer of RGB data (no alpha)
  @param prec: data type (CinePaint enum)

  @note For br::Image and TNT::Array2D<> is width==dim2(), height==dim1()! 
  
  Example of usage: Be \a A a TNT::Array2D<Rgb<float> > array. The following
   creates a CinePaint image display of the array data: 
  <pre>
    cpaint_show_rgb_imgbuf (A.dim2(), A.dim1(), (uchar*)A[0], PRECISION_FLOAT);
  </pre>  
******************************************************************************/
void 
cpaint_show_rgb_imgbuf (int W, int H, const uchar* imgbuf, GPrecisionType prec)
{
  gint32        image_ID, layer_ID;
  GImageType    image_type;
  GDrawableType drawable_type;

  switch (prec) 
    {
    case PRECISION_U8:                        // 8-bit unsigned int
        image_type    = RGB; 
        drawable_type = RGB_IMAGE; break;
  
    case PRECISION_U16:     
        image_type    = U16_RGB; 
        drawable_type = U16_RGB_IMAGE; break;
  
    case PRECISION_FLOAT:                     // 32-bit float
        image_type    = FLOAT_RGB; 
        drawable_type = FLOAT_RGB_IMAGE; break;
  
    case PRECISION_FLOAT16:
        image_type    = FLOAT16_RGB; 
        drawable_type = FLOAT16_RGB_IMAGE; break;
    
    default:
        br::v_alert (_("Precision #%d ('%s') not supported."),
            prec, cpaint_precision_name(prec));
        return;
    }       
  
  image_ID = gimp_image_new ((guint)W, (guint)H, image_type);

  if (image_ID == -1) 
    {
      const char* str_could_not = _("Could not create a new image");
      br::v_alert ("%s\n%d x %d, image_type #%d", str_could_not, W, H, image_type);
      return;
    }

  layer_ID = gimp_layer_new (image_ID,
                             "Br2HDR",          // where does this apear?
                             (guint)W, (guint)H,
                             drawable_type,
                             100.0,             // opacity
                             NORMAL_MODE );
  
  gimp_image_add_layer (image_ID, layer_ID, 0);  // Wozu?

  GDrawable* drawable = gimp_drawable_get (layer_ID);

  GPixelRgn  pixel_rgn;
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0,0, W,H, false,false);

  gimp_pixel_rgn_set_rect (&pixel_rgn, (guchar*)imgbuf, 0,0, W,H);

  gimp_drawable_flush  (drawable);
  gimp_drawable_detach (drawable);
  gimp_display_new (image_ID);
}


/**+*************************************************************************\n
  cpaint_show_image()
  
  Function plots via CinePaint the image data stored in the br::Image object 
   \a img. The type of the image data should be handled here appropriately.
   
  Typpruefung koennte neuerdings auch an br::ImageType abgehandelt werden, zumal
   durch CinePaint ohnehin nur RGB-Puffer (STORING_INTERLEAVE) zu verarbeiten
   sind. Momentaner Mangel der Metavariable br::ImageType noch, dass zu wenig
   Varianten erfasst und Fehlermeldungen dadurch zu duerftig sind (IMAGE_NONE
   oder IMAGE_UNKNOWN).
******************************************************************************/
void 
cpaint_show_image (const br::Image & img)
{
  IF_FAIL_DO (!img.is_empty(), return);
  IF_FAIL_DO (img.channels() == 3, return);  // no RGB
  IF_FAIL_DO (img.storing() == br::STORING_INTERLEAVE, return);
  
  GPrecisionType precis;
  
  switch (img.data_type())
    {
    case br::DATA_U8:  precis = PRECISION_U8; break;
    case br::DATA_U16: precis = PRECISION_U16; break;
    case br::DATA_F32: precis = PRECISION_FLOAT; break;
    default:           
        NOT_IMPL_DATA_TYPE (img.data_type()); 
        return;
    }
  
  cpaint_show_rgb_imgbuf (img.width(), img.height(), img.buffer(), precis);
}


/**+*************************************************************************\n
  Returns a human readable name of the GPrecisionType enum value \a precis; 
   useful for messages and warnings.
******************************************************************************/
const char* 
cpaint_precision_name (GPrecisionType precis)
{
  switch (precis)
    {
    case PRECISION_NONE:    return "None";
    case PRECISION_U8:      return "8-bit Unsigned Integer"; 
    case PRECISION_U16:     return "16-bit Unsigned Integer";
    case PRECISION_FLOAT:   return "32-bit IEEE Float";
    case PRECISION_FLOAT16: return "16-bit RnH Short Float";
    case PRECISION_BFP:     return "16-bit Fixed Point 0-2.0";
    default:                return "(unknown)";
    };
}



// END OF FILE
