/*
 *   Cineon image file plug-in for the GIMP.
 *   Also reads and writes DPX files.
 *   Note: Performs standard conversion to/from 8 bits.
 *
 *   Copyright 1999-2002 David Hodson <hodsond@acm.org>
 *   Much boilerplate taken from the Gimp SGI file reader/writer.
 *   (Which was itself taken from the PNG reader/writer, I reckon.)
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
 *
 *   Rev 0.2 Nov 2000 - changed to new Gimp names.
 *     Now 1.1.29+ compatible!
 *   Rev 0.3 Jan 2001
 *     Fixes to reader, added file write support.
 *   Rev 0.4 Feb 2001
 *     Added load_setargs function.
 *   Rev 0.5 Apr 2001
 *     Rearranged to start adding support for DPX files.
 *   Rev 0.5.1 Apr 2001
 *     Gamma parameter now matches applied gamma.
 *     Better Cineon file creation.
 *   Rev 0.5.2 Apr 2001
 *     Reset button on conversion dialog
 *     DPX might work now...
 *   Rev 0.5.3 Jun2 2001
 *     Mucho code rearrangment, everything seems to work.
 *   Rev 0.5.4 Jan16 2002
 *     Changes to RGB DPX. Maybe working..
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "../../lib/version.h"
#include "logImageLib.h"

#include <gtk/gtk.h>
#include "lib/plugin_main.h"
#include "lib/ui.h"
#include "../../lib/wire/iodebug.h"
#include <libgimp/stdplugins-intl.h>

#define VERBOSE 0

#define PLUG_IN_VERSION		"0.5.4 - Jan 16 2002"

#define CINEON_LOADER           "file_cineon_load"
#define CINEON_LOADER_SETARGS   "file_cineon_load_setargs"
#define CINEON_LOADER_WITHARGS  "file_cineon_load_withargs"
#define CINEON_SAVER            "file_cineon_save"

#define DPX_LOADER           "file_dpx_load"
#define DPX_LOADER_SETARGS   "file_dpx_load_setargs"
#define DPX_LOADER_WITHARGS  "file_dpx_load_withargs"
#define DPX_SAVER            "file_dpx_save"

#define CINEON_PARASITE_NAME "cineon_file_header_block"
#define DPX_PARASITE_NAME    "dpx_file_header_block"

#define MIN_GAMMA 0.1
#define MAX_GAMMA 4.0
#define GAMMA_STEP 0.001

static void query(void);
static void run(char*, int, GParam*, int*, GParam**);
static int conversion_ok(LogImageByteConversionParameters*);
static gint32 load_image(char*, LogImageByteConversionParameters*, int, int);
static gint save_image(char*, gint32, gint32, LogImageByteConversionParameters*, int, int);
static void init_gtk(void);
static int conversion_dialog(LogImageByteConversionParameters*);


GPlugInInfo	PLUG_IN_INFO = { NULL, NULL, query, run };


MAIN()


static void
query(void) {

  static GParamDef load_args[] = {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "filename",     "The name of the file to load" },
    { PARAM_STRING,     "raw_filename", "The name of the file to load" }
  };
  static GParamDef load_return_vals[] = {
    { PARAM_IMAGE,      "image",        "Output image" }
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef load_setargs_args[] = {
    { PARAM_FLOAT,      "gamma",        "Gamma value for Cineon file load" },
    { PARAM_INT32,      "black_point",  "Black point for Cineon file load" },
    { PARAM_INT32,      "white_point",  "White point for Cineon file load" }
  };
  static GParamDef* load_setargs_return_vals = 0;
  static int nload_setargs_args = sizeof (load_setargs_args) / sizeof (load_setargs_args[0]);
  static int nload_setargs_return_vals = 0;

  static GParamDef load_withargs_args[] = {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "filename",     "The name of the file to load" },
    { PARAM_STRING,     "raw_filename", "The name of the file to load" },
    { PARAM_FLOAT,      "gamma",        "Gamma value for the conversion" },
    { PARAM_INT32,      "black_point",  "Black point for the conversion" },
    { PARAM_INT32,      "white_point",  "White point for the conversion" }
  };
  static GParamDef load_withargs_return_vals[] = {
    { PARAM_IMAGE,      "image",        "Output image" }
  };
  static int nload_withargs_args =
    sizeof (load_withargs_args) / sizeof (load_withargs_args[0]);
  static int nload_withargs_return_vals =
    sizeof (load_withargs_return_vals) / sizeof (load_withargs_return_vals[0]);

  static GParamDef save_args[] = {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Drawable to save" },
    { PARAM_STRING,	"filename",	"The name of the file to save the image in" },
    { PARAM_STRING,	"raw_filename",	"The name of the file to save the image in" },
    { PARAM_FLOAT,      "gamma",        "(Optional) Gamma value for the conversion" },
    { PARAM_INT32,      "black_point",  "(Optional) Black point for the conversion" },
    { PARAM_INT32,      "white_point",  "(Optional) White point for the conversion" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure(CINEON_LOADER,
      "Loads files in Cineon image file format",
      "Loads Cineon image files. Cineon is a file format"
          " used in professional film compositing.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 1999-2002 David Hodson",
      PLUG_IN_VERSION,
      "<Load>/Cineon", NULL, PROC_PLUG_IN, nload_args, nload_return_vals,
      load_args, load_return_vals);

  gimp_install_procedure(CINEON_LOADER_SETARGS,
      "Sets parameters for Cineon or DPX file loader",
      "Sets gamma, black point, and white point for Cineon or DPX file loader.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 1999-2002 David Hodson",
      PLUG_IN_VERSION,
      NULL, NULL, PROC_PLUG_IN, nload_setargs_args, nload_setargs_return_vals,
      load_setargs_args, load_setargs_return_vals);

  gimp_install_procedure(CINEON_LOADER_WITHARGS,
      "Loads files in Cineon image file format",
      "Loads Cineon image files with specified conversion. Cineon is a file format"
          " used in professional film compositing.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 1999-2002 David Hodson",
      PLUG_IN_VERSION,
      NULL, NULL, PROC_PLUG_IN, nload_withargs_args, nload_withargs_return_vals,
      load_withargs_args, load_withargs_return_vals);

  gimp_install_procedure(CINEON_SAVER,
      "Saves files in Cineon image file format",
      "Saves Cineon image files. Cineon is a file format"
          " used in professional film compositing.",
       "David Hodson <hodsond@acm.org>",
       "Copyright 1999-2002 David Hodson",
       PLUG_IN_VERSION, "<Save>/Cineon", "U16_RGB,U16_GRAY",
       PROC_PLUG_IN, nsave_args, 0, save_args, NULL);

  gimp_install_procedure(DPX_LOADER,
      "Loads files in DPX image file format",
      "Loads DPX image files. DPX is a file format"
          " used in professional film compositing.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 2001-2002 David Hodson",
      PLUG_IN_VERSION,
      "<Load>/DPX", NULL, PROC_PLUG_IN, nload_args, nload_return_vals,
      load_args, load_return_vals);

  gimp_install_procedure(DPX_LOADER_SETARGS,
      "Sets parameters for DPX file loader",
      "Sets gamma, black point, and white point for DPX file loader.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 1999-2002 David Hodson",
      PLUG_IN_VERSION,
      NULL, NULL, PROC_PLUG_IN, nload_setargs_args, nload_setargs_return_vals,
      load_setargs_args, load_setargs_return_vals);

  gimp_install_procedure(DPX_LOADER_WITHARGS,
      "Loads files in DPX image file format",
      "Loads DPX image files with specified conversion. DPX is a file format"
          " used in professional film compositing.",
      "David Hodson <hodsond@acm.org>",
      "Copyright 1999-2002 David Hodson",
      PLUG_IN_VERSION,
      NULL, NULL, PROC_PLUG_IN, nload_withargs_args, nload_withargs_return_vals,
      load_withargs_args, load_withargs_return_vals);

  gimp_install_procedure(DPX_SAVER,
      "Saves files in DPX image file format",
      "Saves DPX image files. DPX is a file format"
          " used in professional film compositing.",
       "David Hodson <hodsond@acm.org>",
       "Copyright 1999-2002 David Hodson",
       PLUG_IN_VERSION, "<Save>/DPX", "U16_RGB,U16_GRAY",
       PROC_PLUG_IN, nsave_args, 0, save_args, NULL);

/* Old Cineon files have no extension - we'd like to register a magic
 * load handler with null file extension, but it doesn't seem to work.
 */

  gimp_register_load_handler(CINEON_LOADER, "cin", "");
  gimp_register_magic_load_handler(CINEON_LOADER, "cin", "",
      "0,long,CINEON_FILE_BLACK_MAGIC");
#if 0
  gimp_register_magic_load_handler(CINEON_LOADER, "", "",
      "0,long,CINEON_FILE_MAGIC,0,long,CINEON_FILE_BLACK_MAGIC");
#endif
  gimp_register_save_handler(CINEON_SAVER, "cin", "");

  gimp_register_load_handler(DPX_LOADER, "dpx", "");
  gimp_register_magic_load_handler(DPX_LOADER, "dpx", "",
      "0,long,DPX_FILE_BLACK_MAGIC");
  gimp_register_save_handler(DPX_SAVER, "dpx", "");
}


static void
run (char    *name,		/* I - Name of filter program. */
     int      nparams,		/* I - Number of parameters passed in */
     GParam  *param,		/* I - Parameter values */
     int     *nreturn_vals,	/* O - Number of return values */
     GParam **return_vals)	/* O - Return values */
{
  GParam* values = 0;
  GRunModeType run_mode;       
  gint32 image_ID;
  gint32 drawable_ID;

  int loader = 0;
  int withargs = 0;
  int setargs = 0;
  int saver = 0;
  int use_cineon = 0;
  int interactive = 0;
  int use_header = 0;

  LogImageByteConversionParameters conversion;
  logImageGetByteConversionDefaults(&conversion);

  run_mode = param[0].data.d_int32;

  INIT_I18N_UI();

  /* what are we doing? */

  if (run_mode == RUN_INTERACTIVE) { interactive = 1; }
  if (strcmp(name, CINEON_LOADER) == 0) { loader = 1; use_cineon = 1; }
  if (strcmp(name, DPX_LOADER) == 0) { loader = 1; }
  if (strcmp(name, CINEON_LOADER_WITHARGS) == 0) { loader = 1; withargs = 1; use_cineon = 1; }
  if (strcmp(name, DPX_LOADER_WITHARGS) == 0) { loader = 1; withargs = 1; }
  if (strcmp(name, CINEON_LOADER_SETARGS) == 0) { setargs = 1; use_cineon = 1; }
  if (strcmp(name, DPX_LOADER_SETARGS) == 0) { setargs = 1; }
  if (strcmp(name, CINEON_SAVER) == 0) { saver = 1; use_cineon = 1; }
  if (strcmp(name, DPX_SAVER) == 0) { saver = 1; }


  if (loader) {

    /* Loading image */

    if (use_cineon) {
      gimp_get_data(CINEON_LOADER, &conversion);
    } else {
      gimp_get_data(DPX_LOADER, &conversion);
    }

    values = g_new(GParam, 2);
    values[0].type          = PARAM_STATUS;
    values[0].data.d_status = STATUS_SUCCESS;
    values[1].type          = PARAM_IMAGE;
    values[1].data.d_image  = -1;

    *return_vals = values;
    *nreturn_vals = 2;
#if 0
    if (interactive) {

      init_gtk();
      if (!conversion_dialog(&conversion)) {
         values[0].data.d_status = STATUS_EXECUTION_ERROR;
         return;
      }

    } else {

      if (withargs) {

        if (nparams != 6) {
          values[0].data.d_status = STATUS_CALLING_ERROR;
          return;
        }
        conversion.gamma = param[3].data.d_float;
        conversion.blackPoint = param[4].data.d_int32;
        conversion.whitePoint = param[5].data.d_int32;
        if (!conversion_ok(&conversion)) {
          values[0].data.d_status = STATUS_CALLING_ERROR;
          return;
        }

      } else {
        if (nparams != 3) {
          values[0].data.d_status = STATUS_CALLING_ERROR;
          return;
        }
      }
    }
#endif

    image_ID = load_image(param[1].data.d_string, &conversion, use_cineon, use_header);

    if (image_ID != -1) {
      values[1].data.d_image = image_ID;
      if (use_cineon) {
        gimp_set_data(CINEON_LOADER, &conversion, sizeof(conversion));
      } else {
        gimp_set_data(DPX_LOADER, &conversion, sizeof(conversion));
      }


    } else {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }


  } else if (setargs) {

    /* set load params */

    values = g_new(GParam, 1);
    values[0].type          = PARAM_STATUS;
    values[0].data.d_status = STATUS_SUCCESS;

    *return_vals = values;
    *nreturn_vals = 1;

    if (interactive) {

        values[0].data.d_status = STATUS_CALLING_ERROR;
        return;

    } else {

      if (nparams != 3) {
        values[0].data.d_status = STATUS_CALLING_ERROR;
        return;
      }
      conversion.gamma = param[0].data.d_float;
      conversion.blackPoint = param[1].data.d_int32;
      conversion.whitePoint = param[2].data.d_int32;
      if (!conversion_ok(&conversion)) {
        values[0].data.d_status = STATUS_CALLING_ERROR;
        return;
      }
      if (use_cineon) {
        gimp_set_data(CINEON_LOADER, &conversion, sizeof(conversion));
      } else {
        gimp_set_data(DPX_LOADER, &conversion, sizeof(conversion));
      }
    }


  } else if (saver) {

    /* saving image */

    gchar* filename;

    if (use_cineon) {
      gimp_get_data(CINEON_SAVER, &conversion);
    } else {
      gimp_get_data(DPX_SAVER, &conversion);
    }

    values = g_new(GParam, 1);
    values[0].type          = PARAM_STATUS;
    values[0].data.d_status = STATUS_SUCCESS;

    *return_vals = values;
    *nreturn_vals = 1;

    image_ID    = param[1].data.d_int32;
    drawable_ID = param[2].data.d_int32;
    filename = param[3].data.d_string;

#if 0
    if (interactive) {

      init_gtk();
      if (!conversion_dialog(&conversion)) {
         values[0].data.d_status = STATUS_EXECUTION_ERROR;
         return;
      }

    } else {

      if ((nparams < 5) || (nparams > 8)) {
        values[0].data.d_status = STATUS_CALLING_ERROR;
        return;
      }
      if (nparams > 5) conversion.gamma = param[5].data.d_float;
      if (nparams > 6) conversion.blackPoint = param[6].data.d_int32;
      if (nparams > 7) conversion.whitePoint = param[7].data.d_int32;

      if (!conversion_ok(&conversion)) {
        values[0].data.d_status = STATUS_CALLING_ERROR;
        return;
      }
    }
#endif

    if (save_image(filename, image_ID, drawable_ID, &conversion, use_cineon, use_header)) {
      if (use_cineon) {
        gimp_set_data(CINEON_SAVER, &conversion, sizeof(conversion));
      } else {
        gimp_set_data(DPX_SAVER, &conversion, sizeof(conversion));
      }

    } else {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }


  /* oops!? */

  } else {
    values = g_new(GParam, 1);
    values[0].type          = PARAM_STATUS;
    values[0].data.d_status = STATUS_CALLING_ERROR;

    *return_vals = values;
    *nreturn_vals = 1;
  }
}

static int
conversion_ok(LogImageByteConversionParameters* params)
{
return
  (params->gamma >= 0.01) &&
  (params->gamma <= 99.9) &&
  (params->blackPoint >= 0) &&
  (params->blackPoint < params->whitePoint) &&
  (params->whitePoint < 1024);
}

static gint32
load_image(char *filename, LogImageByteConversionParameters *conversion,
    int use_cineon, int use_header) {

  int width, height, depth;
  int		i,		/* Looping var */
		y,		/* Current Y coordinate */
		image_type,	/* Type of image */
		layer_type,	/* Type of drawable/layer */
		tile_height,	/* Height of tile in GIMP */
		count;		/* Count of rows to put in image */
  LogImageFile* logImage;	/* File pointer */
  gint32	image,		/* Image */
		layer;		/* Layer */
  TileDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  gushort	**pixels;	/* Pixel rows */
  char		progress[255];	/* Title for progress display... */

  int lastLineRead = 0;

  logImageSetVerbose(VERBOSE);
  logImage = logImageOpen(filename, use_cineon);

  if (logImage == NULL) {
    g_print("can't open image file: %s\n", filename);
    gimp_quit();
  }

  logImageSetByteConversion(logImage, conversion);

  if (strrchr(filename, '/') != NULL) {
    sprintf(progress, "%s %s:", _("Loading"), strrchr(filename, '/') + 1);
  } else {
    sprintf(progress, "%s %s:", _("Loading"), filename);
  }

  gimp_progress_init(progress);

 /*
  * Get the image dimensions and create the image...
  */

  logImageGetSize(logImage, &width, &height, &depth);

  switch (depth) {

  case 1: 
    image_type = U16_GRAY;
    layer_type = U16_GRAY_IMAGE;
    break;

  case 3:
    image_type = U16_RGB;
    layer_type = U16_RGB_IMAGE;
    break;

  default:
    g_print("Bad image depth %d\n", depth);
    gimp_quit();
    break;
  }

  image = gimp_image_new(width, height, image_type);
  if (image == -1) {
    g_print("can't allocate new image\n");
    gimp_quit();
  }

  gimp_image_set_filename(image, filename);

#if 0
  if (use_header) {
    /* remember header if we SaveAs later */
    void* data = 0;
    int size = 0;
    if (logImageGetHeader(logImage, &size, &data) == 0) {
      GParasite* parasite = 0;
      if (use_cineon) {
        parasite = gimp_parasite_new(CINEON_PARASITE_NAME, 0, size, data);
      } else {
        parasite = gimp_parasite_new(DPX_PARASITE_NAME, 0, size, data);
      }
      if (parasite) {
        gimp_image_parasite_attach(image, parasite);
        gimp_parasite_free(parasite);
      }
    }
  }
#endif

 /*
  * Create the "background" layer to hold the image...
  */

  layer = gimp_layer_new(image, _("Background"), width, height,
                         layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

 /*
  * Get the drawable and set the pixel region for our load...
  */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

 /*
  * Temporary buffers...
  */

  /* Get tile_height rows at once. pixel[i] points to the start of the i-th row */

  tile_height = gimp_tile_height();
  pixels      = g_new(gushort*, tile_height);
  pixels[0]   = g_new(gushort, tile_height * width * depth);

  for (i = 1; i < tile_height; ++i) {
    pixels[i] = pixels[i-1] + width * depth;
  }

 /*
  * Load the image...
  */

  for (y = 0, count = 0;  y < height; ++y, ++count) {

    /* filled the chunk yet? */
    if (count >= tile_height) {
      gimp_pixel_rgn_set_rect(&pixel_rgn, (guchar *) pixels[0], 0, y - count, drawable->width, count);
      count = 0;
      gimp_progress_update((double)y / (double)height);
    }

    if (logImageGetRowBytes(logImage, pixels[count], y) == 0) {
      lastLineRead = y;
    }

  }

 /*
  * Do the last n rows (count always > 0)
  */

  gimp_pixel_rgn_set_rect(&pixel_rgn, (guchar *) pixels[0], 0, y - count, drawable->width, count);

 /*
  * Done with the file...
  */

  logImageClose(logImage);

  if (lastLineRead != (height - 1)) {
    g_print(")Bad image file, last line read was %d\n", lastLineRead);
  }

  g_free(pixels[0]);
  g_free(pixels);

 /*
  * Update the display...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);

  return (image);
}

static gint
save_image(char* filename, gint32 image_ID, gint32 drawable_ID,
  LogImageByteConversionParameters* conversion, int use_cineon, int use_header) {

  TileDrawable *drawable;
  int width, height, depth;
  LogImageFile* logImage;
  gchar* progress;
  GPixelRgn pixel_rgn;
  int tile_height;
  gushort* buffer;
  gushort** line;
  int j;
  int index;

  /*
   * Get the drawable for the current image...
   */

  drawable = gimp_drawable_get(drawable_ID);
  width = drawable->width;
  height = drawable->height;
  switch (gimp_drawable_type(drawable_ID))
    {
    case U16_GRAY_IMAGE :
      depth = 1;
      break;
    case U16_RGB_IMAGE :
      depth = 3;
      break;
    default:
      g_message("Cineon: Image must be of type 16Bit RGB or 16bit-GRAY\n");
      return FALSE;
      break;
    }

  /*
   * Open the file for writing...
   */

  logImageSetVerbose(VERBOSE);
  logImage = logImageCreate(filename, use_cineon, width, height, depth);

  if (logImage == NULL) {
    if (use_cineon) {
      g_message("Cineon: Can't create image file\n");
    } else {
      g_message("DPX: Can't create image file\n");
    }
    return FALSE;
  }

#if 0
  if (use_header) {
    GParasite* parasite = 0;
    if (use_cineon) {
      parasite = gimp_image_parasite_find(image_ID, CINEON_PARASITE_NAME);
    } else {
      parasite = gimp_image_parasite_find(image_ID, DPX_PARASITE_NAME);
    }
    if (parasite) {
      logImageSetHeader(logImage, parasite->size, parasite->data);
      gimp_parasite_free(parasite);
    }
  }
#endif

  logImageSetByteConversion(logImage, conversion);

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf("%s %s:", _("Saving"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf("%s %s:", _("Saving"), filename);
  gimp_progress_init(progress);
  g_free(progress);

  /*
   * Allocate memory for "tile_height" rows...
   */

  tile_height = gimp_tile_height();
  buffer = g_new(gushort, tile_height * width * depth);
  line = g_new(gushort*, tile_height);

  for (j = 0; j < tile_height; ++j) {
    line[j] = buffer + width * depth * j;
  }

  /*
   * Save the image...
   */

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  index = 0;
  for (j = 0; j < height; ++j) {
    if (index == 0) {
      int slab = height - j;
      if (slab > tile_height) slab = tile_height;
      gimp_pixel_rgn_get_rect(&pixel_rgn, (guchar *) buffer, 0, j, width, slab);      
    }

    logImageSetRowBytes(logImage, line[index], j);

    ++index;
    if (index == tile_height) index = 0;

    gimp_progress_update((double)j / (double)height);

  }
  logImageClose(logImage);

  g_free(buffer);
  g_free(line);

  return TRUE;
}



/* The GUI stuff */

typedef struct {

  int doit;

  LogImageByteConversionParameters *conversion;

  GtkWidget *dialog;
  GtkObject *gamma_adj;
  GtkObject *black_adj;
  GtkObject *white_adj;
  GtkWidget* message;
#if 0
  GtkWidget* useHeader;
#endif

} ConversionGui;


static void
setMessage(GtkAdjustment* adj, GtkWidget* message) {
  char buffer[40];
  sprintf(buffer, "Correct for display gamma %6.3f", 1.0 / adj->value);
  gtk_label_set_text(GTK_LABEL(message), buffer);
}

static void
resetConversion(GtkWidget* btn, ConversionGui* gui) {
  logImageGetByteConversionDefaults(gui->conversion);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(gui->gamma_adj), gui->conversion->gamma);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(gui->black_adj), gui->conversion->blackPoint);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(gui->white_adj), gui->conversion->whitePoint);
}

/*
 * 'conversion_close_callback()' - Close the conversion dialog window.
 */

static void
conversion_close_callback(GtkWidget *w, gpointer data) {
  gtk_main_quit();
}


/*
 * 'conversion_ok_callback()' - Destroy the conversion dialog and process the image.
 */

static void
conversion_ok_callback(GtkWidget *w, gpointer data) {
  ConversionGui *gui = (ConversionGui *)data;

  gui->conversion->gamma = GTK_ADJUSTMENT(gui->gamma_adj)->value;
  gui->conversion->blackPoint = GTK_ADJUSTMENT(gui->black_adj)->value;
  gui->conversion->whitePoint = GTK_ADJUSTMENT(gui->white_adj)->value;
  gui->doit = 1;

  gtk_widget_destroy(gui->dialog);
}

static void
init_gtk() {
  gchar **argv;
  gint    argc;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("cineon");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
}

/*
 * 'conversion_dialog()' - Pop up the conversion dialog.
 */

static int
conversion_dialog(LogImageByteConversionParameters *conversion) {

  GtkWidget* button_box;
  GtkWidget* button;
  GtkWidget* frame;
  GtkWidget* table;
  GtkWidget* label;
  GtkWidget* spin;
  GtkWidget* scale;
  GtkWidget* message;
  GtkWidget* box;

  int spinWidth;
  int tableHeight = 5;
  int tableWidth = 5;
  int row;

  ConversionGui gui;
  int flags = GTK_FILL | GTK_EXPAND;

  gui.doit = 0;
  gui.conversion = conversion;
  gui.dialog = gtk_dialog_new();

  /* hbox in dialog, buttons at bottom, table at top with labels and three sliders */

  gtk_window_set_title(GTK_WINDOW(gui.dialog), "Cineon - " PLUG_IN_VERSION);
  gtk_window_set_wmclass(GTK_WINDOW(gui.dialog), "cineon", PROGRAM_NAME);
  /*  gtk_window_position(GTK_WINDOW(gui.dialog), GTK_WIN_POS_MOUSE); */
  gtk_signal_connect(GTK_OBJECT(gui.dialog), "destroy",
                     (GtkSignalFunc)conversion_close_callback, NULL);

  gtk_container_set_border_width(GTK_CONTAINER (GTK_DIALOG (gui.dialog)->action_area), 2);
  gtk_box_set_homogeneous(GTK_BOX (GTK_DIALOG (gui.dialog)->action_area), FALSE);
  button_box = gtk_hbutton_box_new();
  gtk_button_box_set_spacing(GTK_BUTTON_BOX (button_box), 4);
  gtk_box_pack_end(GTK_BOX (GTK_DIALOG (gui.dialog)->action_area), button_box, FALSE, FALSE, 0);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) conversion_ok_callback,
		      &gui);
  gtk_box_pack_start(GTK_BOX (button_box), button, FALSE, FALSE, 0);
  gtk_widget_grab_default(button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (gui.dialog));
  gtk_box_pack_start(GTK_BOX (button_box), button, FALSE, FALSE, 0);

  frame = gtk_frame_new(_("Conversion"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG(gui.dialog)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new(tableHeight, tableWidth, TRUE);

  gtk_container_add(GTK_CONTAINER(frame), table);
  row = 0;

  gui.gamma_adj =
    gtk_adjustment_new(conversion->gamma, MIN_GAMMA, MAX_GAMMA, GAMMA_STEP, GAMMA_STEP * 10, 0.0);
  gui.black_adj =
    gtk_adjustment_new(conversion->blackPoint, 0.0, 1023.0, 1.0, 1.0, 0.0);
  gui.white_adj =
    gtk_adjustment_new(conversion->whitePoint, 0.0, 1023.0, 1.0, 1.0, 0.0);

  label = gtk_label_new(_("Gamma"));
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, flags, flags, 3, 2);
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(gui.gamma_adj), 0.0, 3);
  /* get a good size for the spinners */
  spinWidth = gdk_string_width( gtk_style_get_font(gtk_widget_get_style(spin)), "10.000") + 25;
  gtk_widget_set_usize(spin, spinWidth, 0);
  gtk_table_attach(GTK_TABLE(table), spin, 1, 2, row, row+1, flags, flags, 3, 2);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(gui.gamma_adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 2);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_table_attach(GTK_TABLE(table), scale, 2, 5, row, row+1, flags, flags, 3, 2);
  ++row;

  message = gtk_label_new(_("Hi there"));
  setMessage(GTK_ADJUSTMENT(gui.gamma_adj), message);
  gtk_table_attach(GTK_TABLE(table), message, 0, 5, row, row+1, flags, flags, 3, 2);
  gtk_signal_connect(gui.gamma_adj, "value_changed", GTK_SIGNAL_FUNC(setMessage), message);
  ++row;

  label = gtk_label_new(_("Black"));
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, flags, flags, 3, 2);
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(gui.black_adj), 0.0, 0);
  gtk_widget_set_usize(spin, spinWidth, 0);
  gtk_table_attach(GTK_TABLE(table), spin, 1, 2, row, row+1, flags, flags, 3, 2);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(gui.black_adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_table_attach(GTK_TABLE(table), scale, 2, 5, row, row+1, flags, flags, 3, 2);
  ++row;

  label = gtk_label_new(_("White"));
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, flags, flags, 3, 2);
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(gui.white_adj), 0.0, 0);
  gtk_widget_set_usize(spin, spinWidth, 0);
  gtk_table_attach(GTK_TABLE(table), spin, 1, 2, row, row+1, flags, flags, 3, 2);
  scale = gtk_hscale_new(GTK_ADJUSTMENT(gui.white_adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_table_attach(GTK_TABLE(table), scale, 2, 5, row, row+1, flags, flags, 3, 2);
  ++row;

  box = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  /*  box = gtk_hbox_new(FALSE, 0); */
  gtk_table_attach(GTK_TABLE(table), box, 0, 5, row, row+1, flags, flags, 3, 2);
  button = gtk_button_new_with_label(_("  Reset  "));
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(resetConversion), (gpointer)&gui);
  /*  gtk_box_pack_end(GTK_BOX(box), button, FALSE, FALSE, 0); */
  gtk_container_add(GTK_CONTAINER(box), button);

  ++row;

  gtk_widget_show_all(gui.dialog);

  gtk_main();
  gdk_flush();

  return (gui.doit);
}
