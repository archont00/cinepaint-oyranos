/*
 *   Print plug-in for the CinePaint/GIMP.
 *
 *   Copyright 2003-2004 Kai-Uwe Behrmann
 *
 *   separate core/gui
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  December 2003
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
//#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "lcms.h"
//#include "../../lib/libprintut.h"


#include "print_gimp.h"

#include "print-intl.h"

#include "print-lcms-funcs.h"
#include "print.h"

#ifndef DEBUG
 #define DEBUG
#endif

#ifdef DEBUG
 #define DBG printf("%s:%d %s()\n",__FILE__,__LINE__,__func__);
#else
 #define DBG 
#endif




// definitions for compatibillity
#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION > 0 && GIMP_MINOR_VERSION < 17
                            // filmgimp
    #define GimpPDBStatusType GStatusType
    #define GIMP_PDB_SUCCESS STATUS_SUCCESS
    #define GIMP_PDB_STATUS PARAM_STATUS
    #define GIMP_PDB_CALLING_ERROR STATUS_CALLING_ERROR
    #define GIMP_PDB_EXECUTION_ERROR STATUS_EXECUTION_ERROR
    #define GIMP_RUN_INTERACTIVE RUN_INTERACTIVE
    #define GIMP_RUN_NONINTERACTIVE RUN_NONINTERACTIVE
    #define GIMP_RUN_WITH_LAST_VALS RUN_WITH_LAST_VALS
    #define GimpImageType GimpDrawableType
    #define _(x) x
#else
  #ifdef GIMP_VERSION          // gimp
    //#define GParamDef GimpParamDef
    //#define GParam GimpParam
    //#define GRunModeType GimpRunModeType
    //#define GStatusType GimpStatusType
    #define GRAY GIMP_GRAY 
    #define GRAY_IMAGE GIMP_GRAY_IMAGE 
    #define GRAYA_IMAGE GIMP_GRAYA_IMAGE 
    #define INDEXED GIMP_INDEXED 
    #define INDEXED_IMAGE GIMP_INDEXED_IMAGE 
    #define INDEXEDA_IMAGE GIMP_INDEXEDA_IMAGE 
    #define RGB GIMP_RGB
    #define RGB_IMAGE GIMP_RGB_IMAGE 
    #define RGBA_IMAGE GIMP_RGBA_IMAGE 
    #define NORMAL_MODE GIMP_NORMAL_MODE 
    #define DARKEN_ONLY_MODE GIMP_DARKEN_ONLY_MODE 
    #define LIGHTEN_ONLY_MODE GIMP_LIGHTEN_ONLY_MODE 
    #define HUE_MODE GIMP_HUE_MODE 
    #define SATURATION_MODE GIMP_SATURATION_MODE 
    #define COLOR_MODE GIMP_COLOR_MODE 
    #define MULTIPLY_MODE GIMP_MULTIPLY_MODE 
    #define SCREEN_MODE GIMP_SCREEN_MODE 
    #define DISSOLVE_MODE GIMP_DISSOLVE_MODE 
    #define DIFFERENCE_MODE GIMP_DIFFERENCE_MODE 
    #define VALUE_MODE GIMP_VALUE_MODE 
    #define ADDITION_MODE GIMP_ADDITION_MODE 
    #define GProcedureType GimpPDBProcType
  #else                     // cinepaint
    #define gimp_procedural_db_proc_info gimp_query_procedure
    //#define _(x) x
  #endif
#endif

// definitions

#define M_PI    3.14159265358979323846

// variables

Linearisation linear =
{
  0.0, 0.0, 0.0, 0.0,
  1.0, 1.0, 1.0, 1.0,
  1.0, 1.0, 1.0, 1.0,
  FALSE
};

LcmsVals vals =
{
  "sRGB.icc",          // i_profile[MAX_PATH], input profile
  "",                  // o_profile[MAX_PATH], output profile
  "",                  // l_profile[MAX_PATH], linearisation profile
  "",                  // image_filename [MAX_PATH]
  "CMYK.tif",          // tiff_file[MAX_PATH], tiff filename
  INTENT_PERCEPTUAL,   // intent,              rendering intent
  INTENT_PERCEPTUAL,   // disp_intent,         displays rendering intent
  0x00,                // matrix,              conversion matrix
  0x00,                // precalc,             precalculation
  0x00,                // flags,               flags
  0x00,                // disp_flags,          flags of the active display
  0x00,                // preserveblack,       preserve black for cmyk -> cmyk
  0,                   // bit per pixel
  "",                  // color type
  -1,                  // display_ID
  -1,                  // o_image_ID,          original image
  -1,                  // e_image_ID,          exported image
  -1,                  // s_image_ID,          separated image
  -1,                  // o_drawable_ID
  -1,                  // e_drawable_ID
  -1,                  // s_drawable_ID
  FALSE,               // save_tiff,           print in an tiff file
  TRUE,                // icc,                 use an ICC profile for conversion
#ifdef SHOW_CMYK
  TRUE,
#else
  FALSE,               // show_image,          display the converted image
#endif
  PT_RGB,              // colour_space         expect this source colour space
  FALSE,               // quit_app
};

extern int lcms_run;

// functions

void
start_vals                  (   void )
{
  vals.o_image_ID = -1;                  // o_image_ID,          original image
  vals.e_image_ID = -1;                  // e_image_ID,          exported image
  vals.s_image_ID = -1;                  // s_image_ID,          separated image
  vals.o_drawable_ID = -1;                  // o_drawable_ID
  vals.e_drawable_ID = -1;                  // e_drawable_ID
  vals.s_drawable_ID = -1;                  // s_drawable_ID
  vals.save_tiff = FALSE;          // save_tiff,           print in an tiff file
  vals.quit_app = FALSE;                // quit_app
}


cmsHPROFILE
getLinearisationProfile   ( void )
{
    // linearisation
    int nEntries = 256,
        i;
    double levels = pow(2.0,16);
    LPGAMMATABLE gamma[4];
    cmsHPROFILE hLinear=NULL;
    char gamma_text[256],
         text[256];

    sprintf (gamma_text,
             "gamma %.2f|%.2f|%.2f|%.2f "
             "rescale %.2f-%.2f %.2f-%.2f %.2f-%.2f %.2f-%.2f "
             "pow(2.0,bit)=%.0f \n",
              linear.C_Gamma,linear.M_Gamma,linear.Y_Gamma,linear.K_Gamma,
              linear.C_Min,linear.C_Max,linear.M_Min,linear.M_Max,
              linear.Y_Min,linear.Y_Max,linear.K_Min,linear.K_Max,
              levels);

    g_print (gamma_text);

    gamma[0] = cmsBuildGamma (nEntries, linear.C_Gamma);
    gamma[1] = cmsBuildGamma (nEntries, linear.M_Gamma);
    gamma[2] = cmsBuildGamma (nEntries, linear.Y_Gamma);
    gamma[3] = cmsBuildGamma (nEntries, linear.K_Gamma);

    #define cat(x,y) x ## y             // join textually
    #define RANGE_CURVE(Color,Nr) \
    if (linear.cat(Color,_Min) != 0.0 || linear.cat(Color,_Max) != 1.0) { \
      for (i=0; i<nEntries; i++) { \
        int level; \
        level = (int)( (double)(gamma[Nr]->GammaTable[i]) * (linear.cat(Color,_Max) -linear.cat(Color,_Min) ) \
                       + levels * linear.cat(Color,_Min) + 0.5 ); \
        level = MAX (0 , MIN (level,levels-1)); \
        gamma[Nr]->GammaTable[i] = level; \
      } \
      cmsSmoothGamma (gamma[Nr], 0.05); \
    } 

    RANGE_CURVE (C,0)
    RANGE_CURVE (M,1)
    RANGE_CURVE (Y,2)
    RANGE_CURVE (K,3)


    hLinear = cmsCreateLinearizationDeviceLink (_cmsICCcolorSpace(PT_CMYK), gamma); 

    {
        int nargs = 1;
        size_t size = sizeof(int) + nargs * sizeof(cmsPSEQDESC);
        LPcmsSEQ pseq = (LPcmsSEQ) malloc(size);
        
        ZeroMemory(pseq, size);
        pseq ->n = 1;

        for (i=0; i < nargs; i++) {

            strcpy(pseq ->seq[i].Manufacturer, "CinePaint");
            strcpy(pseq ->seq[i].Model, "");
        }

        sprintf (text, "%s - postlinearisation tables", gamma_text);
        cmsAddTag(hLinear, icSigProfileDescriptionTag, text);
        cmsAddTag(hLinear, icSigCopyrightTag,
             "Generated by CinePaints print plug-in. No copyright, use freely");
        cmsAddTag(hLinear, icSigProfileSequenceDescTag, pseq);
    }

    cmsFreeGamma (gamma[0]);
    cmsFreeGamma (gamma[1]);
    cmsFreeGamma (gamma[2]);
    cmsFreeGamma (gamma[3]);


    // save an devicelink wich contains our linearisation data
    _cmsSaveProfile (hLinear, "/tmp/cinepaint_link_temp.icm");

  return hLinear;
}

static cmsHPROFILE
openInputProfile (void)
{
  cmsHPROFILE hIn;

  DBG
  {
    if ((strcmp (vals.i_profile,"") == 0)
     || (strcmp (vals.i_profile,"sRGB.icc") == 0)) {
      // assuming we have only RGB input data
      hIn  = cmsCreate_sRGBProfile();
    } else if ((strcmp (vals.i_profile,"lab") == 0)
            || (strcmp (vals.i_profile,"Lab") == 0)) {
      hIn  = cmsCreateLabProfile(cmsD50_xyY());
    } else {
      if (gimp_image_has_icc_profile (vals.o_image_ID, ICC_IMAGE_PROFILE)) {
        gint size = 0;
        char* buf = NULL;
        buf = gimp_image_get_icc_profile_by_mem (vals.o_image_ID, &size,
                                                 ICC_IMAGE_PROFILE);
        hIn = cmsOpenProfileFromMem (buf, size);
        if( buf && size) free (buf);
      } else {
        hIn  = cmsOpenProfileFromFile(vals.i_profile, "r");
      }
    }
  }

  return hIn;
}

void
mergeOutputProfileWithLinearisation (cmsHPROFILE profile, cmsHPROFILE linear, cmsHPROFILE merge)
{
  {
    if (strlen (vals.l_profile) > 0) {
      LPLUT        Lut_in, Lut_linear, Lut_out;
      LPGAMMATABLE gamma[16];
      int          channel, i;

      g_print ("%s:%d %s() ", __FILE__,__LINE__,__func__);
      printProfileTags (merge);

      copyProfile (profile, merge);

      g_print ("%s:%d %s() ", __FILE__,__LINE__,__func__);
      printProfileTags (merge);

      #if 0
      gamma[0] = cmsReadICCLut (hLinear, icSigAToB0Tag);
      cmsCloseProfile (hLinear);
      hLinear = cmsCreateLinearizationDeviceLink (_cmsICCcolorSpace(PT_CMYK),
                                                  gamma); 
      #else

      Lut_in =  cmsReadICCLut( profile, icSigAToB0Tag);
      Lut_linear =  cmsReadICCLut( linear, icSigAToB0Tag);

      #if 1
      for (channel=0 ; channel < Lut_linear ->OutputChan ; channel++) {
        gamma[channel] = cmsBuildGamma(Lut_linear ->InputEntries, 1.0);
        for (i=0 ; i < Lut_linear ->InputEntries ; i++)
          gamma[channel]->GammaTable[i] = Lut_linear -> L1[channel][i];
      }
      #endif

      //Lut_out = cmsDupLUT(Lut_in);

        // 2 - Postlinearization
      Lut_out = cmsAllocLinearTable (Lut_in, gamma, 2);

      #if 0
      for (i=0; i < Lut ->InputEntries; i++)  {
         {
           g_print ("C=%d ", Lut ->L2[0][i]);
           g_print ("M=%d ", Lut ->L2[1][i]);
           g_print ("Y=%d ", Lut ->L2[2][i]);
           g_print ("K=%d ", Lut ->L2[3][i]);
        }
      }
      #endif
      _cmsAddLUTTag ( merge, icSigAToB0Tag, Lut_out );

      //cmsAddTag(merge, icSigDeviceMfgDescTag,       "LittleCMS");       
      cmsAddTag(merge, icSigProfileDescriptionTag,  "merged Test profile");
      //cmsAddTag(merge, icSigDeviceModelDescTag,     "printer");      

      //cmsFreeLUT (Lut_in);
      //cmsFreeLUT (Lut_linear);
      //cmsFreeLUT (Lut_out);
      #endif
      #if 0
      for (i=0; i<gamma[3]->nEntries ; i++)
        g_print ("%d ",gamma[3]->GammaTable[i]);
      #endif

      g_print ("%s:%d %s() ", __FILE__,__LINE__,__func__);
      printProfileTags (merge);

      if (_cmsSaveProfile (merge, "/tmp/cinepaint_merge_temp.icm"))
        g_print ("%s:%d %s() smelted linearisation.\n",
                  __FILE__,__LINE__,__func__);
      else
        g_print ("%s:%d %s() Writing of profile failed\n",
                  __FILE__,__LINE__,__func__);

      g_print ("%s:%d %s() ", __FILE__,__LINE__,__func__);
      printProfileTags (merge);

    }
  }

}

cmsHTRANSFORM*
createTransform (void)
{
  return NULL;
}




int
convert_image ( void )
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpDrawable	*cmyk_drawable;	/* Drawable for image */
  GPixelRgn      src_rgn, dest_rgn;
  gint32  cmyk_image_ID,               // ID of the converted image
          cmyk_drawable_ID;
  int     pixelCount,
          alpha,
          nr_of_profiles = 2;

    cmsHPROFILE   hIn=NULL, hOut=NULL, hLinear=NULL,
                  multi[3], merge=NULL;
    cmsHTRANSFORM xform, linear_form=NULL;
    DWORD wInput, wOutput;
    int   bit, lcms_bytes,
          in_color_space = PT_ANY,
          in_channels = 3, out_channels = 4;
    uint  i;
    unsigned char *BufferIn, *BufferOut; // buffers
    int     w, h,
            tile_height, y_start, y_end,
            precision = gimp_drawable_precision (vals.e_drawable_ID);
    FILE   *fp;

  DBG
  cmyk_image_ID = vals.s_image_ID;
  cmyk_drawable_ID = vals.s_drawable_ID;

  // should be without alpha
  alpha = gimp_drawable_has_alpha(vals.e_drawable_ID);

  drawable = gimp_drawable_get (vals.e_drawable_ID);

    switch (precision) {
      case 1:         // uint8
        bit =  8;
        lcms_bytes = 1;
        break;
      case 2:         // uint16
        bit = 16; 
        lcms_bytes = 2;
        break;
      case 3:         // f32
        bit = 32; 
        lcms_bytes = 0;
        break;
      case 4:         // f16 (R&H) TODO
        bit = 16; 
        lcms_bytes = 0;
        break;
      default:
        g_print ("!!! Precision = %d not allowed!\n", precision);
        return GIMP_PDB_CALLING_ERROR;
    }

  // Make a new image and convert it to cmyk, delete the old one.
  w = gimp_drawable_width  (cmyk_drawable_ID);
  h = gimp_drawable_height (cmyk_drawable_ID);

  cmyk_drawable = gimp_drawable_get (cmyk_drawable_ID);
  g_print ("%s:%d %s() drawable_IDs %d|%d lcms_bytes:%d alpha:%d\n",
            __FILE__,__LINE__,__func__,
            drawable->id, cmyk_drawable->id, lcms_bytes, alpha); 
  if (cmyk_drawable->id != cmyk_drawable_ID) {
    cmyk_drawable->id = cmyk_drawable_ID;
    g_warning ("%s:%d %s() cmyk_drawable_ID %d\n",__FILE__,__LINE__,__func__,
                cmyk_drawable_ID); 
  }

  if (linear.use_lin == TRUE)
    hIn = getLinearisationProfile ();
  else
    hIn = openInputProfile ();

  if ( linear.use_lin == TRUE
    || icSigCmykData == cmsGetColorSpace(hIn) ) {
    g_print ("%s:%d %s() cmyk output\n",__FILE__,__LINE__,__func__); 
    in_color_space = PT_CMYK;
    vals.color_space = PT_CMYK; // TODO here is the input meant
    in_channels = 4;
    alpha = 0;
  }

  cmsCloseProfile (hIn);

  // set the types of input and output buffers
  wInput =  (COLORSPACE_SH(in_color_space) | // source image color space
             CHANNELS_SH(in_channels) |  // how many color channels
             BYTES_SH(lcms_bytes) |      // 0 - double / 1 - uint8 / 2 - uint16
             EXTRA_SH(alpha));           // alpha?

  wOutput = (COLORSPACE_SH(PT_CMYK) |    // destination image color space
             CHANNELS_SH(out_channels) | // how many color channels
             BYTES_SH(2) |               // uint16
             EXTRA_SH(alpha));           // alpha?

  // test for gui selected input profile
  if (linear.use_lin == TRUE) {
    // post linearisation
    hIn = getLinearisationProfile (); 
    //hOut = cmsCreateLabProfile(cmsD50_xyY());
#   ifdef DEBUG
    DBG printf("linear.use_lin %d\n",linear.use_lin);
#   endif

    xform = cmsCreateTransform     (hIn, wInput,
                                    NULL, wOutput,
                                    INTENT_ABSOLUTE_COLORIMETRIC, 
                                    0);

  } else {
    hIn = openInputProfile ();

    // test for gui selected output profile
    if ((strcmp (vals.o_profile,"")) == 0) {
      fp = fopen ("cmyk.icc", "r");
      if ( fp == NULL ) {
          g_message ("No separation profile found.\nYou need at least one CMYK profile (called separation profile) selected through the file selector.\nAlternatively You can link or copy Your favorite profile to the current working directory\nand rename it to cmyk.icc .");
          return GIMP_PDB_CALLING_ERROR;
      } else {
          fclose (fp);
          hOut  = cmsOpenProfileFromFile("cmyk.icc", "r");
      }
    } else {
      char* proof_profile_name = get_proof_name();
      if (proof_profile_name && (strcmp(vals.o_profile, proof_profile_name)) == 0)
      {
        gint size = 0;
        char* data = gimp_image_get_icc_profile_by_mem ( vals.o_image_ID, &size,
                     ICC_PROOF_PROFILE);
        hOut = cmsOpenProfileFromMem(data, size);
        if(data && size) free(data);
      } else {
        fp = fopen ( vals.o_profile, "r");
        if ( fp == NULL ) {
          g_message (_("Profile %s can not be found or is not readable."),
                      vals.o_profile);
          return GIMP_PDB_CALLING_ERROR;
        } else {
          fclose (fp);
          hOut  = cmsOpenProfileFromFile(vals.o_profile, "r");
        }
      }
      if( proof_profile_name ) free( proof_profile_name );
    }
    if ( hOut == NULL )
      g_message (_("Profile %s is incorrect."), vals.o_profile); 

    multi[0] = hIn;
    multi[1] = hOut;

    {
      char *text;

      text = getProfileInfo (multi[1]);
#     ifdef DEBUG
      g_print ("%s:%d %s() \n%s\n",__FILE__,__LINE__,__func__, text);
#     endif
      free (text);
    }

    if (strlen (vals.l_profile) > 0) {
      LPLUT  Lut;
      LPGAMMATABLE gamma[4];

      hLinear  = 
      #if 1
                 cmsOpenProfileFromFile (vals.l_profile, "r");
      #else
                 getLinearisationProfile (); 
      #endif
      #if 0
      gamma[0] = cmsReadICCLut (hLinear, icSigAToB0Tag);
      cmsCloseProfile (hLinear);
      hLinear = cmsCreateLinearizationDeviceLink (_cmsICCcolorSpace(PT_CMYK),
                                                  gamma); 
      //#else
      //Lut =  cmsReadICCLut( hLinear, icSigAToB0Tag);

      Lut =  cmsReadICCLut( hLinear, icSigAToB0Tag);

      for (i=0; i < Lut ->InputEntries; i++)  {
        if (0) {
           g_print ("C=%d ", Lut ->L1[0][i]);
           g_print ("M=%d ", Lut ->L1[1][i]);
           g_print ("Y=%d ", Lut ->L1[2][i]);
           g_print ("K=%d ", Lut ->L1[3][i]);
        }
      }

      cmsFreeLUT (Lut);
      #endif
      #if 0
      for (i=0; i<gamma[3]->nEntries ; i++)
        g_print ("%d ",gamma[3]->GammaTable[i]);
      #endif

      nr_of_profiles++;

      multi [nr_of_profiles-1] = hLinear;

      merge = cmsOpenProfileFromFile(vals.o_profile, "r");
      mergeOutputProfileWithLinearisation (multi [1],
                                           multi [nr_of_profiles-1],
                                           merge);
      cmsCloseProfile (merge);

      g_print ("%s:%d %s() Linearisation active.\n",__FILE__,__LINE__,__func__);
    }

    // TODO merge the linearisation profile in the printer profile
    //#define MERGE_GAMMALUT
    #ifdef MERGE_GAMMALUT
#   ifdef DEBUG
    DBG printf("MERGE_GAMMALUT\n");
#   endif
    xform = cmsCreateMultiprofileTransform (multi, nr_of_profiles,
                                            wInput, wOutput,
                                            vals.intent, 
                                            vals.matrix|vals.precalc|vals.bpc|
                                            vals.preserveblack);
    if (0) { //TODO Knopf zum verschmelzen
      cmsHPROFILE merge;

      g_print("%s:%d %s()\n",__FILE__,__LINE__,__func__);
      merge = cmsTransform2DeviceLink (xform,0);
      _cmsSaveProfile (merge, "/tmp/cinepaint_merge_sep-lin_temp.icc");
      cmsCloseProfile (merge);
      g_print("%s:%d %s() Merge Profile written.\n",__FILE__,__LINE__,__func__);
    }
    #else
    xform = cmsCreateTransform     (hIn, wInput,
                                    hOut, wOutput,
                                    vals.intent, 
                                    vals.matrix|vals.precalc|
                                    vals.flags&cmsFLAGS_BLACKPOINTCOMPENSATION|
                                    vals.preserveblack);
      DBG_S( ("matrix %d precalc %d bpc %d",
           vals.matrix,vals.precalc,vals.flags&cmsFLAGS_BLACKPOINTCOMPENSATION))

    if (strlen (vals.l_profile) > 0) {
      linear_form = cmsCreateTransform   (hLinear,
                                          COLORSPACE_SH(PT_CMYK) | 
                                          CHANNELS_SH(4) |
                                          BYTES_SH(2) | 
                                          EXTRA_SH(0),
                                          NULL,
                                          COLORSPACE_SH(PT_CMYK) | 
                                          CHANNELS_SH(4) |
                                          BYTES_SH(2) | 
                                          EXTRA_SH(0),
                                          INTENT_ABSOLUTE_COLORIMETRIC,
                                          cmsFLAGS_PRESERVEBLACK);
      DBG_S( ("vals.l_profile %s",vals.l_profile) )
    }
    #endif
  }

  if (w > 100000)     // huge image support - for lcms
    tile_height = gimp_tile_width()/8;
  else
    tile_height = gimp_tile_width();

  if (lcms_bytes == 0) {  // Floats need to be Doubles
    BufferIn  = malloc( tile_height * w * (3 + alpha) * bit * 2);
  } else {
    BufferIn  = malloc( tile_height * w * (3 + alpha) * bit );
  }
  BufferOut = malloc( tile_height * w *  4 * 2 );


  gimp_progress_init (_("separating ..."));

  // prepare for access
  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
                        drawable->width, drawable->height,
                        FALSE, FALSE);

  gimp_pixel_rgn_init (&dest_rgn, cmyk_drawable, 0, 0,
                        cmyk_drawable->width, cmyk_drawable->height,
                        FALSE, FALSE);


  // in tile_hight chunks
  for (y_start=0; y_start < h; y_start += tile_height) {

    if (y_start > (h - tile_height -1)) {
      pixelCount = w * (h - y_start);
      tile_height = h - y_start;
      y_end = h -1;
    } else {
      pixelCount = w * tile_height;
      y_end = y_start + tile_height -1;
    }

    g_print ("pixel at: %d-%d with %d*%d=%d\n",
              y_start, y_end, w, tile_height, pixelCount);

    // fill the buffer
    gimp_pixel_rgn_get_rect (&src_rgn, BufferIn, 0, y_start, 
                             w, tile_height);

    gimp_progress_update ((double)((double)y_start / (double)drawable->height));

    // convert buffer from float to double for liblcms
    if ( lcms_bytes == 0 && bit == 32 ) {
      double *buf = (double*)BufferIn;
      float *fbuf = (float*) BufferIn;
      int i;
 
      g_print ("%f ",fbuf[pixelCount/2]);

      for (i = pixelCount * (3 + alpha) - 1 ; i >= 0 ; i--) { //for all channels
        buf[i] = (double)fbuf[i] * 100;         // expected by lcms
      }
      g_print ("= %f\n",buf[pixelCount/2]);
    }

    // let lcms do the work
    cmsDoTransform(xform, BufferIn, BufferOut, pixelCount);
    g_print (".");
    if (nr_of_profiles > 2) {
    #ifndef MERGE_GAMMALUT
      cmsDoTransform(linear_form, BufferOut, BufferOut, pixelCount);
      g_print ("#");
    #endif
    }

    // copy the full buffer to the image
    gimp_pixel_rgn_set_rect (&dest_rgn, BufferOut, 0, y_start,
                             cmyk_drawable->width, tile_height);

    gimp_progress_update ((double)((double)y_end / (double)drawable->height));
  }

  g_free (BufferIn);
  g_free (BufferOut);
  cmsDeleteTransform(xform);
  cmsCloseProfile(hIn);
  cmsCloseProfile(hOut);
  if (nr_of_profiles > 2) {
    #ifdef MERGE_GAMMALUT
    cmsCloseProfile(hLinear);
    #endif
  }

  g_print ("Conversion done.\n");
  //gimp_drawable_detach (cmyk_drawable);

  vals.s_image_ID = cmyk_image_ID;
  vals.s_drawable_ID = cmyk_drawable_ID;
  return GIMP_PDB_SUCCESS;
}

int
create_separated_image      (   void )

{
  int   w = gimp_drawable_width  (vals.s_drawable_ID),
        h = gimp_drawable_height (vals.s_drawable_ID);
  gint32 CMYK_display;
  char* proof_profile_name = NULL;

  vals.s_image_ID = gimp_image_new  ( w, h, U16_RGB);

  set_tiff_filename (PT_CMYK);
  gimp_image_set_filename (vals.s_image_ID, vals.tiff_file/*(char*) "CMYK"*/);

  vals.s_drawable_ID = gimp_layer_new (vals.s_image_ID, (char*) _("Background"),
                                       w, h,
                                       U16_RGBA_IMAGE, 100.0, NORMAL_MODE);
  gimp_image_add_layer (vals.s_image_ID, vals.s_drawable_ID, 1);


  DBG
  convert_image ( );

  proof_profile_name = get_proof_name();

  if (proof_profile_name && (strcmp(vals.o_profile, proof_profile_name)) == 0)
  {
    gint size = 0;
    char* data = gimp_image_get_icc_profile_by_mem ( vals.o_image_ID, &size,
                                                     ICC_PROOF_PROFILE);
    gimp_image_set_icc_profile_by_mem (vals.s_image_ID, size, data,
                                                     ICC_IMAGE_PROFILE);
    if(data && size) free(data);
  } else
    gimp_image_set_icc_profile_by_name (vals.s_image_ID, &vals.o_profile[0],
                                        ICC_IMAGE_PROFILE);
  if( proof_profile_name ) free( proof_profile_name );

  // show the print preview
  if (vals.show_image)
  {
    int proof_intent = INTENT_RELATIVE_COLORIMETRIC;
    int is_proofed = gimp_display_is_colormanaged( vals.display_ID,
                                                   ICC_PROOF_PROFILE );

    if( is_proofed )
      proof_intent = gimp_display_get_cms_intent( vals.display_ID,
                                                   ICC_PROOF_PROFILE );

    DBG_S( ("ID:%d/%d proof_intent:%d",vals.display_ID, is_proofed, proof_intent) )

    CMYK_display = gimp_display_new (vals.s_image_ID);
    gimp_display_set_cms_intent( CMYK_display,
                                 proof_intent, ICC_IMAGE_PROFILE );
    gimp_display_set_cms_flags (CMYK_display, 0);
    gimp_display_set_colormanaged (CMYK_display, TRUE, ICC_IMAGE_PROFILE );
  }

  return GIMP_PDB_SUCCESS;
}


// taken from curve_bend.c
/* ============================================================================
 * p_pdb_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

gint 
p_pdb_procedure_available (char *proc_name)
{    
  gint             l_nparams;
  gint             l_nreturn_vals;
  GProcedureType   l_proc_type;
  gchar           *l_proc_blurb;
  gchar           *l_proc_help;
  gchar           *l_proc_author;
  gchar           *l_proc_copyright;
  gchar           *l_proc_date;
  GimpParamDef    *l_params;
  GimpParamDef    *l_return_vals;
  gint             l_rc;

  l_rc = 0;
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */

  if (gimp_procedural_db_proc_info (proc_name,
				    &l_proc_blurb,
				    &l_proc_help,
				    &l_proc_author,
				    &l_proc_copyright,
				    &l_proc_date,
                             (gint*)&l_proc_type,
				    &l_nparams, &l_nreturn_vals,
				    &l_params, &l_return_vals))
  {
     /* procedure found in PDB */
     return (l_nparams);
  }

# ifdef DEBUG
  DBG printf("Warning: Procedure %s not found.\n", proc_name);
#endif
  return -1;
}	/* end p_pdb_procedure_available */


gint32  
p_gimp_save_tiff (gint32 image_ID,
		  gint32 drawable_ID,
                  gint   colour_space,
                  gchar* embedProfile)
{
    static char     *l_procname = "file_tiff_save";
    GimpParam       *return_vals;
    int              nreturn_vals;
    guchar           filename[MAX_PATH];


   if (p_pdb_procedure_available(l_procname) >= 0)
   {
      sprintf (filename,"%s%s%s", g_dirname(vals.tiff_file),G_DIR_SEPARATOR_S,
                                  g_basename(vals.tiff_file));

      g_print ("%s:%d %s() image_ID %d drawable_ID %d -> %s with %s\n",__FILE__,
                __LINE__,__func__, image_ID, drawable_ID,filename,embedProfile);

      return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                          GIMP_PDB_INT32,     0,
                          GIMP_PDB_IMAGE,     image_ID,
                          GIMP_PDB_DRAWABLE,  gimp_drawable_get(drawable_ID),
                          GIMP_PDB_STRING,    filename,
                          GIMP_PDB_STRING,    g_basename(vals.tiff_file),
                          GIMP_PDB_INT32,     3,            // compression
                          GIMP_PDB_INT32,     0,            // fillorder
                          GIMP_PDB_INT32,     2,            // premultiply
                          GIMP_PDB_STRING,    embedProfile,
                          GIMP_PDB_END);
                                    
      if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
      {
         gimp_destroy_params (return_vals, nreturn_vals);
         return 0;
      }
#     ifdef DEBUG
      DBG printf("Error: PDB call of %s failed status:%d\n", l_procname, (int)return_vals[0].data.d_status);
#     endif
   }

   return(-1);
}

void
set_tiff_filename                      (int colour_space)
{
  char *test1, *test2;
  char *txt,
       *point = NULL;
  int   change_tiff_fn = TRUE;

  test1 = malloc (8);
  test2 = malloc (8);
  txt = g_new (char,MAX_PATH);

  if (colour_space == PT_CMYK) {
    sprintf (test1, "CMYK");
    sprintf (test2, "cmyk");
  } else if (colour_space == PT_Lab) {
    sprintf (test1, "Lab");
    sprintf (test2, "lab");
  } else {
    sprintf (vals.tiff_file, vals.image_filename);
    change_tiff_fn = FALSE;
  }

  if ( strstr(vals.image_filename, test1) == NULL
    && strstr(vals.image_filename, test2) == NULL
    && change_tiff_fn ) {

        sprintf (txt, vals.image_filename);

        g_print ("%s:%d vals.tiff_file %s\n",__FILE__,__LINE__,vals.image_filename);
        if (strchr(txt, '.') && strlen (txt) < MAX_PATH - 9) {
          point = strrchr(txt, '.');
          sprintf (point,"_%s.tif", test1);
          sprintf (vals.tiff_file,"%s",txt);
        }

        g_print ("%s:%d vals.tiff_file %s\n",__FILE__,__LINE__,vals.tiff_file);

  }
  g_free (test1);
  g_free (test2);
  g_free (txt);
  g_print ("%s:%d %s() vals.tiff_file %s\n",__FILE__,__LINE__,__func__,
                       vals.tiff_file);
}

cmsHPROFILE
get_print_profile()
{
  char *description = NULL;
  cmsHPROFILE hOut = NULL;

  if ( gimp_image_has_icc_profile (vals.o_image_ID, ICC_PROOF_PROFILE))
  {
    description = gimp_image_get_icc_profile_description( vals.o_image_ID,
                                                             ICC_PROOF_PROFILE);
  }

  if(description &&
     (strcmp(vals.o_profile, description)) == 0)
  {
    gint size = 0;
    char* data = gimp_image_get_icc_profile_by_mem ( vals.o_image_ID, &size,
                   ICC_PROOF_PROFILE);
    hOut = cmsOpenProfileFromMem(data, size);
    free( data );
    free( description );
  } else
    hOut = cmsOpenProfileFromFile(vals.o_profile, "r");

  return hOut;
}

char*
get_proof_name()
{
  char* text = NULL;

  if( gimp_image_has_icc_profile (vals.o_image_ID, ICC_PROOF_PROFILE) )
    text = gimp_image_get_icc_profile_description( vals.o_image_ID,
                                                             ICC_PROOF_PROFILE);

  return text;
}

int
test_separation_profiles               (void)
{
  cmsHPROFILE hIn;
  cmsHPROFILE hOut;
  cmsHPROFILE hLinear;
  int hIn_ok,
      profiles_ok = FALSE,
      out_icColorSpaceSignature = 0;

  if (strcmp(vals.o_profile, "" ) == 0 ) {
    vals.icc = FALSE;
    profiles_ok = TRUE;
  } else {

    // test for valid profile combination and give some informations if fails
    hOut = get_print_profile();
    if (hOut != NULL) {
      if (strcmp(vals.i_profile, ""        ) == 0 ||
          strcmp(vals.i_profile, "sRGB.icc") == 0 ||
          strcmp(vals.i_profile, "Lab") == 0 ||
          strcmp(vals.i_profile, "lab") == 0) {
        // assuming sRGB as Input, should work allways
        if (icSigLabData != cmsGetPCS(hOut)) {
        //  g_message ("no uniform PCS");
        }
        hIn_ok = TRUE;
      } else {
        if (gimp_image_has_icc_profile (vals.o_image_ID, ICC_IMAGE_PROFILE)) {
          hIn_ok = TRUE;
        } else {
          hIn = cmsOpenProfileFromFile(vals.i_profile, "r");
          if (hIn != NULL) {
            // test for device class
            hIn_ok = checkProfileDevice (_("Work Space"), hIn);
            cmsCloseProfile (hIn);
          } else {
            g_message
              (_("Your \"Work Space Profile\" is eighter not valid or does not exist. If in doubt clear the text in the \"Work Space Profile\" entry. This will switch to standard sRGB mode."));
            hIn_ok = FALSE;
          }
        }
      }
      if (hIn_ok) {
        out_icColorSpaceSignature = cmsGetColorSpace(hOut);
        if (icSigCmykData   != out_icColorSpaceSignature/* &&
          icSig4colorData != cmsGetColorSpace(hOut)*/    ) {
          g_message (_("Your \"Separation Profile\" is not in CMYK color space."));
        } else {
          profiles_ok = checkProfileDevice (_("Separation"), hOut);
          vals.color_space = PT_CMYK;
        }
      }
      cmsCloseProfile (hOut);

    } else {
      g_message 
        (_("Your \"Separation Profile\" is eighter not valid or does not exist."));
    }
    if (profiles_ok && strlen (vals.l_profile)) {
      FILE   *fp;
      fp = fopen (vals.l_profile, "r");
      if ( fp == NULL ) {
        profiles_ok = FALSE;
        g_message 
          (_("Your \"Linearisation Profile\" does not exist."));
      } else {
        fclose (fp);
        hLinear = cmsOpenProfileFromFile(vals.l_profile, "r");
        if (hOut != NULL) {
          profiles_ok = checkProfileDevice (_("Linearisation"), hLinear);
          cmsCloseProfile (hLinear);
        } else {
          profiles_ok = FALSE;
          g_message 
            (_("Your \"Linearisation Profile\" is not valid."));
        }
      }
    }
  }

  return profiles_ok;
}

int
test_tiff_settings                     (void)
{
  int profiles_ok = FALSE;


  if (vals.icc == TRUE) {
    profiles_ok = test_separation_profiles ();
  } else {
    profiles_ok = TRUE;
  }

  return profiles_ok;
}

int
test_print_settings                     (void)
{
  int profiles_ok = FALSE;
  GDrawableType drawable_type;

  if (vals.icc == TRUE) {
    profiles_ok = test_separation_profiles ();
  }

  if (vals.icc == FALSE) { // send data direct to libgimpprint

    drawable_type = gimp_drawable_type(vals.s_drawable_ID);

    switch (drawable_type)
    {
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
      g_message(_("drawable type %d is not supported for direct printing."), drawable_type);
      return FALSE;
      break;
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
    case RGB_IMAGE:
    case RGBA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:
      profiles_ok = TRUE;
      break;
    }
  }

  return profiles_ok;
}
