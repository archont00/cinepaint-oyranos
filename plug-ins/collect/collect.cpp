/*
 * Collect images of the same size and colour type into one layerd image;
 *  plug-in for cinepaint.
 *
 * Copyright (C) 2004-2006 Kai-Uwe Behrmann <ku.b@gmx.de>
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

/*
 * Open an file browser
 * open all selected images in CinePaint, which are of the same geometrical and
 * bit size
 *
 * Date: 03 December 2004
 */


#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_PNM_Image.H>
#include <FL/Fl_JPEG_Image.H>

#define PLUG_IN_NAME          "plug_in_collect"
#define PLUG_IN_BRIEF         "Collects selected images into one single image."
#define PLUG_IN_DESCRIPTION   "Collects selected images into one single image."
#define PLUG_IN_VERSION       "0.2 -  23 March 2006"
#define PLUG_IN_AUTHOR        "Kai-Uwe Behrmann <ku.b@gmx.de>"
#define PLUG_IN_COPYRIGHT     "Copyright 2004-2006 Kai-Uwe Behrmann"

extern "C" {
#include "lib/plugin_main.h"
#include "lib/wire/libtile.h"
#include "plugin_pdb.h"
#include "libgimp/stdplugins-intl.h"
}

// Declare some local functions:

void   query		(void);
void   run		(char    	*name,
					 int      nparams,
					 GParam  *param,
					 int     *nreturn_vals,
					 GParam **return_vals);

int	do_collect	();
int	file_load	(const char*	 filename,
                         int             interactiv);

#ifdef DEBUG
#include <iostream>
#define cout std::cout
#define endl std::endl
#define DBG cout << __FILE__ <<":"<< __LINE__ <<" "<< __func__ << "()\n";
#else
#define DBG
#endif


GPlugInInfo	PLUG_IN_INFO =
{
    NULL,    /* init_proc */
    NULL,    /* quit_proc */
    query,   /* query_proc */
    run,     /* run_proc */
};

typedef struct {
  char name[1024];
} Vals;
Vals vals;

/***   file-global variables   ***/

static int n_return_vals_;
static int n_args_;

/*** Functions ***/

MAIN()				// defined in lib/plugin_main.h


void
query ()
{
   static GParamDef args [] =
   {
     { PARAM_INT32,		"run_mode",		"Interactive, non-interactive" },
     { PARAM_STRING,		"directory",		"directory" },
   };
   static GParamDef return_vals [] =
   {
     { PARAM_IMAGE, 	"image",		"Output Image" },
   };
   n_args_        = sizeof (args) / sizeof (args[0]);
   n_return_vals_ = sizeof (return_vals) / sizeof (return_vals[0]);

   gimp_install_procedure (PLUG_IN_NAME,
                           PLUG_IN_BRIEF,
                           PLUG_IN_DESCRIPTION,
                           PLUG_IN_AUTHOR,
                           PLUG_IN_COPYRIGHT,
                           PLUG_IN_VERSION,
                           "<Toolbox>/File/New From/Collect..." ,
                           "*",
                           PROC_EXTENSION,
                           n_args_, n_return_vals_,
                           args, return_vals);

   // translation workaround
   _("File"); _("New From"); _("Collect...");
}

void
run (char    *name,		// I - Name of called function resp. plug-in
     int      nparams,		// I - Number of parameters passed in
     GParam  *param,		// I - Parameter values
     int     *nreturn_vals,	// O - Number of return values
     GParam **return_vals)	// O - Return values
{
  GStatusType  status;		// return status,
  GRunModeType   run_mode;
  static GParam  values [1];	// return parameter
  int result = -1;
 
  status   = STATUS_SUCCESS;
  run_mode = (GRunModeType) param[0].data.d_int32;
  values[0].type 	        = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;


  *nreturn_vals = n_return_vals_;
  *return_vals  = values;

  if (strcmp (name, PLUG_IN_NAME) == 0)
  {
    INIT_I18N_UI();
    INIT_FLTK1_CODESET();

    sprintf (vals.name, getenv("PWD"));
    gimp_get_data ("collect", &vals);

    switch (run_mode)
    {
      case RUN_INTERACTIVE:
           result = do_collect ();
           if (result < 0)
           { status = STATUS_EXECUTION_ERROR;
           }
           break;

      case RUN_NONINTERACTIVE:
           // TODO: noninteractive
           if (nparams != n_args_)
           { status = STATUS_CALLING_ERROR;
             break;
           }
           // How to handle an regular expression here?
           result = do_collect ();
           if (result < 0)
           { status = STATUS_EXECUTION_ERROR;
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
  gimp_set_data ("collect", &vals, sizeof(Vals));
}


int
file_load(const char* filename, int interactive)
{
  int image_ID;
  GimpParam* return_vals;
  int n_retvals;
  return_vals = gimp_run_procedure ("gimp_file_load",
                                    &n_retvals,
                                    GIMP_PDB_INT32,
                                    interactive ? RUN_INTERACTIVE : RUN_NONINTERACTIVE,
                                    GIMP_PDB_STRING, filename,
                                    GIMP_PDB_STRING, filename,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status != GIMP_PDB_SUCCESS)
    image_ID = -1;
  else
    image_ID = return_vals[1].data.d_image;
  gimp_destroy_params (return_vals, n_retvals);
  return image_ID;
}


Fl_Image*
raw_check( const char* fname, uchar *header, int len )
{
  const char    *home;          // Home directory
  char          preview[1024],  // Preview filename
                command[1024];  // Command
  int stat;

  const char *endung = strrchr(fname,'.');
  if( !endung ) return NULL;
  if(strstr(endung+1,"png") != 0) return NULL;
  if(strstr(endung+1,"xbm") != 0) return NULL;
  if(strstr(endung+1,"jpg") != 0) return NULL;

  sprintf (command, "dcraw -i '%s'\n", fname);
  //fputs (command, stderr);

  stat = system (command);

  if (stat) {
    if (stat > 0x200)
      g_message (_("The \"rawphoto\" plugin won't work because "
	"there is no \"dcraw\" executable in your path."));

    return NULL;
  }


  home = getenv("HOME");
  sprintf(preview, "%s/.preview.ppm", home ? home : "");

  sprintf(command,
          "dcraw -e -v -c"
          " \'%s\' > \'%s\' ", fname, preview);

  if (system(command)) return 0;

  return new Fl_JPEG_Image( preview );
}

int
do_collect ()
{
	gint32		*layers;
	gint32		 image_ID;
	gint		 nlayers;

	nlayers = -1;


        Fl_File_Chooser         *fc = 0;

        Fl_Shared_Image::add_handler(raw_check);
        Fl_File_Icon::load_system_icons();
        fc = new Fl_File_Chooser(vals.name, "*.*", Fl_File_Chooser::MULTI, _("Select images"));
        fc->show();

        while (fc->visible())
          Fl::wait(0.01);

        int count = fc->count();
        if (count > 0)
        {
          // take the first image as reference
          unsigned int max_w = 0, max_h = 0;

          #ifdef DEBUG
          cout << "adding image " << fc->value(1) << endl;
          #endif
          int load_image_ID = file_load(fc->value(1), 1);
          image_ID = load_image_ID;

          // if solo, renaming to the first selected image
          if(count > 1)
            gimp_image_set_filename (image_ID, _("Collect"));
          else
            gimp_image_set_filename (image_ID, (char*)fc->value(1));
          layers = gimp_image_get_layers (load_image_ID, &nlayers);

          if(!layers)
            return -1;

          // renaming the layer to the original filename
          gimp_layer_set_name (layers[0], strrchr(fc->value(1),'/')+1);
          GPrecisionType image_base_prec = gimp_drawable_precision (layers[0]);
          int base_gray = gimp_drawable_gray (layers[0]);

          // searching max dimensions
          max_w = gimp_image_width(load_image_ID);
          max_h = gimp_image_height(load_image_ID);

          for (int i = 2; i <= count; i ++)
          {
            if (!fc->value(i))
              break;

            load_image_ID = file_load(fc->value(i), 0 );

            // add to layer stack
            if (load_image_ID > 0)
            {
              layers = gimp_image_get_layers (load_image_ID, &nlayers);
              if (nlayers > 1) {
                g_message ("More than one layer in image\n %s - ignoring" ,fc->value(i));
              } else
              if (nlayers < 1) {
                g_message ("no layer in image\n %s - ignoring" ,fc->value(i));
              } else
              if (layers[0] <= 0) {
                g_message ("no data found in image\n %s - ignoring", fc->value(i));
              } else 
              if (image_base_prec != gimp_drawable_precision (layers[0])) {
                g_message ("different bit-depth in image\n %s - ignoring" ,fc->value(i));
              } else
              if (base_gray != gimp_drawable_gray(layers[0])) {
                g_message ("different RGB/Gray layout in image\n %s - ignoring" ,fc->value(i));
              } else {
                    #ifdef DBEUG
                    cout << "adding image " << fc->value(i) << " with type " <<
                            image_base_prec << endl;
                    #endif
                    gimp_image_add_layer (image_ID, layers[0], 0); DBG
                    // set layer name to filename
                    gimp_layer_set_name(layers[0], strrchr(fc->value(i),'/')+1);

                    // searching max dimensions
                    if (gimp_image_width(load_image_ID) > max_w)
                      max_w = gimp_image_width(load_image_ID);
                    if (gimp_image_height(load_image_ID) > max_h)
                      max_h = gimp_image_height(load_image_ID);
              }
            }
            gimp_image_delete (load_image_ID);
            gimp_image_clean_all (load_image_ID);

          } DBG
          // resize to include all images
          if (image_ID &&
              (gimp_image_width(image_ID) < max_w ||
               gimp_image_height(image_ID) < max_h) )
          {
            gimp_image_resize (image_ID, max_w, max_h, 0,0); DBG
          }

          sprintf (vals.name, fc->value(1));
          char* ptr = strrchr(vals.name, '/');
          *ptr = 0;

        } DBG

        #ifdef DBEUG
        cout << fc->count() << " files choosen in " << vals.name << endl;
        #endif

        /* number of layers and layer_ID: */
        layers = gimp_image_get_layers (image_ID, &nlayers);

        if (image_ID >= 0)
          gimp_display_new (image_ID);

        free (layers);

        if(fc) delete fc;

        return image_ID;
}


