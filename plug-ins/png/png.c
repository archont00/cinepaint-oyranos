/*
 * "$Id: png.c,v 1.9 2006/11/24 20:52:55 beku Exp $"
 *
 *   Portable Network Graphics (PNG) plug-in for The GIMP -- an image
 *   manipulation program
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com) and
 *   Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz).
 *   and 1999-2000 Nick Lamb (njl195@zepler.org.uk)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   respin_cmap()               - Re-order a Gimp colormap for PNG tRNS
 *   save_image()                - Save the specified image to a PNG file.
 *   save_ok_callback()          - Destroy the save dialog and save the image.
 *   save_compression_callback() - Update the image compression level.
 *   save_interlace_update()     - Update the interlacing option.
 *   save_dialog()               - Pop up the save dialog.
 *
 * Revision History:
 *
 *   see ChangeLog
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
/* Makes rint consistent in Windows and Linux: */
#define rint(x) floor(x+0.5)

#include <gtk/gtk.h>
#include "../../lib/version.h"
#include "../../lib/plugin_main.h"
#include "../../lib/ui.h"
#include "../../lib/wire/iodebug.h"
#include "../../lib/dialog.h"
#include "../../lib/widgets.h"
#include "libgimp/stdplugins-intl.h"

#include <png.h>		/* PNG library definitions */

/*
 * Constants...
 */

#define PLUG_IN_VERSION  "1.3.3 - 30 June 2000"
#define SCALE_WIDTH      125

#define DEFAULT_GAMMA    2.20

/*
 * Structures...
 */

typedef struct
{
  gint	interlaced;
  gint	compression_level;
  gint  bkgd;
  gint  gama;
  gint  offs;
  gint  phys;
  gint  time;
} PngSaveVals;


/*
 * Local functions...
 */

static void	query                     (void);
static void	run                       (gchar   *name,
					   gint     nparams,
					   GParam  *param,
					   gint    *nreturn_vals,
					   GParam **return_vals);

static gint32	load_image                (gchar   *filename);
static gint	save_image                (gchar   *filename,
					   gint32   image_ID,
					   gint32   drawable_ID,
					   gint32   orig_image_ID);

static void	respin_cmap		  (png_structp pp,
					   png_infop info,
					   gint32   image_ID);

static gint	save_dialog               (void);
static void	save_ok_callback          (GtkWidget     *widget,
					   gpointer       data);


/*
 * Globals...
 */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

PngSaveVals pngvals = 
{
  FALSE,
  6,
  TRUE, FALSE, FALSE, TRUE, TRUE
};

static gboolean runme = FALSE;

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static GimpParamDef load_args[] =
  {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "filename",     "The name of the file to load" },
    { PARAM_STRING,     "raw_filename", "The name of the file to load" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { PARAM_IMAGE,      "image",        "Output image" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GimpParamDef	save_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Drawable to save" },
    { PARAM_STRING,	"filename",	"The name of the file to save the image in" },
    { PARAM_STRING,	"raw_filename",	"The name of the file to save the image in" },
    { PARAM_INT32,	"interlace",	"Use Adam7 interlacing?" },
    { PARAM_INT32,	"compression",	"Deflate Compression factor (0--9)" },
    { PARAM_INT32,	"bkgd",		"Write bKGD chunk?" },
    { PARAM_INT32,	"gama",		"Write gAMA chunk?" },
    { PARAM_INT32,	"offs",		"Write oFFs chunk?" },
    { PARAM_INT32,	"phys",		"Write tIME chunk?" },
    { PARAM_INT32,	"time",		"Write pHYs chunk?" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_png_load",
			  "Loads files in PNG file format",
			  "This plug-in loads Portable Network Graphics (PNG) files.",
			  "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
			  "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, Nick Lamb <njl195@zepler.org.uk>",
			  PLUG_IN_VERSION,
			  "<Load>/PNG",
			  NULL,
			  PROC_PLUG_IN,
			  nload_args, nload_return_vals,
			  load_args, load_return_vals);

  gimp_install_procedure  ("file_png_save",
			   "Saves files in PNG file format",
			   "This plug-in saves Portable Network Graphics (PNG) files.",
			   "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
			   "Michael Sweet <mike@easysw.com>, Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, Nick Lamb <njl195@zepler.org.uk>",
			   PLUG_IN_VERSION,
			   "<Save>/PNG",
			   "RGB*,GRAY*,INDEXED*,U16_RGB*,U16_GRAY*",
			   PROC_PLUG_IN,
			   nsave_args, 0,
			   save_args, NULL);

  gimp_register_magic_load_handler ("file_png_load",
				    "png",
				    "",
				    "0,string,\211PNG\r\n\032\n");
  gimp_register_save_handler       ("file_png_save",
				    "png",
				    "");
}





/*
 * 'run()' - Run the plug-in...
 */

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;
  gint32        orig_image_ID;
  //GimpExportReturnType export = GIMP_EXPORT_CANCEL; //FIXME XXX

  INIT_I18N_UI();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_png_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_png_save") == 0)
    {

      run_mode = param[0].data.d_int32;
      image_ID = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
    
      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  gimp_ui_init ("png", FALSE);
	  /*export = gimp_export_image (&image_ID, &drawable_ID, "PNG", 
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED |
				       GIMP_EXPORT_CAN_HANDLE_ALPHA ));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      *nreturn_vals = 1;
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }*/ //FIXME XXX
	  break;
	default:
	  break;
	}

      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_png_save", &pngvals);

	  /*
	   * Then acquire information with a dialog...
	   */
          if (!save_dialog())
		  return; //FIXME
            //status = GIMP_PDB_CANCEL; //FIXME
          break;

	case RUN_NONINTERACTIVE:
	  /*
	   * Make sure all the arguments are there!
	   */
          if (nparams != 12)
	    {
	      status = STATUS_CALLING_ERROR;
	    }
          else
	    {
	      pngvals.interlaced        = param[5].data.d_int32;
	      pngvals.compression_level = param[6].data.d_int32;
	      pngvals.bkgd              = param[7].data.d_int32;
	      pngvals.gama              = param[8].data.d_int32;
	      pngvals.phys              = param[9].data.d_int32;
	      pngvals.offs              = param[10].data.d_int32;
	      pngvals.time              = param[11].data.d_int32;

	      if (pngvals.compression_level < 0 ||
		  pngvals.compression_level > 9)
		status = STATUS_CALLING_ERROR;
	    };
          break;

	case RUN_WITH_LAST_VALS:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_png_save", &pngvals);
          break;

	default:
          break;
	};

      if (status == STATUS_SUCCESS)
	{
	  if (save_image (param[3].data.d_string,
			  image_ID, drawable_ID, orig_image_ID))
	    {
	      gimp_set_data ("file_png_save", &pngvals, sizeof (pngvals));
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      /*if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);*/ //FIXME
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
}


/*
 * 'load_image()' - Load a PNG image into a new image window.
 */

static gint32
load_image (gchar *filename)	/* I - File to load */
{
  int		i,		/* Looping var */
                trns,		/* Transparency present */
		bpp,		/* Bytes per pixel */
		image_type,	/* Type of image */
		layer_type,	/* Type of drawable/layer */
		empty,		/* Number of fully transparent indices */
		num_passes,	/* Number of interlace passes in file */
		pass,		/* Current pass in file */
		tile_height,	/* Height of tile in GIMP */
		begin,		/* Beginning tile row */
		end,		/* Ending tile row */
		num;		/* Number of rows to load */
  FILE		*fp;		/* File pointer */
  volatile gint32 image;	/* Image -- preserved against setjmp() */
  gint32	layer;		/* Layer */
  TileDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  png_structp	pp;		/* PNG read pointer */
  png_infop	info;		/* PNG info pointers */
  guchar	**pixels,	/* Pixel rows */
		*pixel;		/* Pixel data */
  gchar		*progress;	/* Title for progress display... */
  guchar	alpha[256],	/* Index -> Alpha */
  		*alpha_ptr;	/* Temporary pointer */

 /*
  * PNG 0.89 and newer have a sane, forwards compatible constructor.
  * Some SGI IRIX users will not have a new enough version though
  */
#if PNG_LIBPNG_VER > 88
  pp   = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct(pp);
#else
  pp = (png_structp)calloc(sizeof(png_struct), 1);
  png_read_init(pp);

  info = (png_infop)calloc(sizeof(png_info), 1);
#endif /* PNG_LIBPNG_VER > 88 */

  if (setjmp (pp->jmpbuf))
  {
    g_message ("%s\nPNG error. File corrupted?", filename);
    return image;
  }

  /* initialise image here, thus avoiding compiler warnings */

  image= -1;

 /*
  * Open the file and initialize the PNG read "engine"...
  */

  fp = fopen(filename, "rb");

  if (fp == NULL) 
    {
      g_message ("%s\nis not present or is unreadable", filename);
      gimp_quit ();
  }

  png_init_io(pp, fp);

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf ("%s %s:", _("Loading"),  strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf ("%s %s:", _("Loading"), filename);

  gimp_progress_init(progress);
  g_free (progress);

 /*
  * Get the image dimensions and create the image...
  */

  png_read_info(pp, info);

 /*
  * Latest attempt, this should be my best yet :)
  */

#ifndef WORDS_BIGENDIAN
  if(info->bit_depth == 16)
	  png_set_swap(pp);
#endif

  if (info->color_type == PNG_COLOR_TYPE_GRAY && info->bit_depth < 8) {
    png_set_expand(pp);
  }

  if (info->color_type == PNG_COLOR_TYPE_PALETTE && info->bit_depth < 8) {
    png_set_packing(pp);
  }

 /*
  * Expand G+tRNS to GA, RGB+tRNS to RGBA
  */

  if (info->color_type != PNG_COLOR_TYPE_PALETTE &&
                       (info->valid & PNG_INFO_tRNS)) {
    png_set_expand(pp);
  }

 /*
  * Turn on interlace handling... libpng returns just 1 (ie single pass)
  * if the image is not interlaced
  */

  num_passes = png_set_interlace_handling(pp);

 /*
  * Special handling for INDEXED + tRNS (transparency palette)
  */

#if PNG_LIBPNG_VER > 99
  if (png_get_valid(pp, info, PNG_INFO_tRNS) &&
      info->color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_get_tRNS(pp, info, &alpha_ptr, &num, NULL);
    /* Copy the existing alpha values from the tRNS chunk */
    for (i= 0; i < num; ++i)
      alpha[i]= alpha_ptr[i];
    /* And set any others to fully opaque (255)  */
    for (i= num; i < 256; ++i)
      alpha[i]= 255;
    trns= 1;
  } else {
    trns= 0;
  }
#else
    trns= 0;
#endif /* PNG_LIBPNG_VER > 99 */

 /*
  * Update the info structures after the transformations take effect
  */

  png_read_update_info(pp, info);
  
  if(info->bit_depth==16)
  {
	  switch (info->color_type)
	  {
		  case PNG_COLOR_TYPE_RGB :		/* RGB */
			  bpp        = 6;
			  image_type = U16_RGB;
			  layer_type = U16_RGB_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_RGB_ALPHA :	/* RGBA */
			  bpp        = 8;
			  image_type = U16_RGB;
			  layer_type = U16_RGBA_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_GRAY :		/* Grayscale */
			  bpp        = 2;
			  image_type = U16_GRAY;
			  layer_type = U16_GRAY_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_GRAY_ALPHA :	/* Grayscale + alpha */
			  bpp        = 4;
			  image_type = U16_GRAY;
			  layer_type = U16_GRAYA_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_PALETTE :	/* Indexed */
			  bpp        = 2;
			  image_type = U16_INDEXED;
			  layer_type = U16_INDEXED_IMAGE;
			  break;
		  default:				/* Aie! Unknown type */
			  g_message ("%s\nPNG unknown color model", filename);
			  return -1;
	  };
  }
  else
  {
	  switch (info->color_type)
	  {
		  case PNG_COLOR_TYPE_RGB :		/* RGB */
			  bpp        = 3;
			  image_type = RGB;
			  layer_type = RGB_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_RGB_ALPHA :	/* RGBA */
			  bpp        = 4;
			  image_type = RGB;
			  layer_type = RGBA_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_GRAY :		/* Grayscale */
			  bpp        = 1;
			  image_type = GRAY;
			  layer_type = GRAY_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_GRAY_ALPHA :	/* Grayscale + alpha */
			  bpp        = 2;
			  image_type = GRAY;
			  layer_type = GRAYA_IMAGE;
			  break;

		  case PNG_COLOR_TYPE_PALETTE :	/* Indexed */
			  bpp        = 1;
			  image_type = INDEXED;
			  layer_type = INDEXED_IMAGE;
			  break;
		  default:				/* Aie! Unknown type */
			  g_message ("%s\nPNG unknown color model", filename);
			  return -1;
	  };
  }

  image = gimp_image_new(info->width, info->height, image_type);
  if (image == -1)
  {
    g_message("Can't allocate new image\n%s", filename);
    gimp_quit();
  };

  gimp_image_set_filename(image, filename);

 /*
  * Create the "background" layer to hold the image...
  */

  layer = gimp_layer_new(image, _("Background"), info->width, info->height,
                         layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

  /*
   * Find out everything we can about the image resolution
   * This is only practical with the new 1.0 APIs, I'm afraid
   * due to a bug in libpng-1.0.6, see png-implement for details
   */

#if PNG_LIBPNG_VER > 99
  if (png_get_valid(pp, info, PNG_INFO_gAMA)) {
    /* I sure would like to handle this, but there's no mechanism to
       do so in Gimp :( */
  }
  if (png_get_valid(pp, info, PNG_INFO_oFFs)) {
    gimp_layer_set_offsets (layer, png_get_x_offset_pixels(pp, info),
                                   png_get_y_offset_pixels(pp, info));
  }
  if (png_get_valid(pp, info, PNG_INFO_pHYs)) {
    /*gimp_image_set_resolution(image,
         ((double) png_get_x_pixels_per_meter(pp, info)) * 0.0254,
         ((double) png_get_y_pixels_per_meter(pp, info)) * 0.0254);*/ //FIXME For the moment we forget about the resolution XXX
  }
#endif /* PNG_LIBPNG_VER > 99 */

 /*
  * Load the colormap as necessary...
  */

  empty= 0; /* by default assume no full transparent palette entries */

  if (info->color_type & PNG_COLOR_MASK_PALETTE) {

#if PNG_LIBPNG_VER > 99
    if (png_get_valid(pp, info, PNG_INFO_tRNS)) {
      for (empty= 0; empty < 256 && alpha[empty] == 0; ++empty);
        /* Calculates number of fully transparent "empty" entries */

      gimp_image_set_cmap(image, (guchar *) (info->palette + empty),
                          info->num_palette - empty);
    } else {
      gimp_image_set_cmap(image, (guchar *)info->palette, info->num_palette);
    }
#else
    gimp_image_set_cmap(image, (guchar *)info->palette, info->num_palette);
#endif /* PNG_LIBPNG_VER > 99 */

  }

 /*
  * Get the drawable and set the pixel region for our load...
  */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

 /*
  * Temporary buffer...
  */

  tile_height = gimp_tile_height ();
  pixel       = g_new(guchar, tile_height * info->width * bpp);
  pixels      = g_new(guchar *, tile_height);

  if(info->bit_depth==16)
  {
	  for (i = 0; i < tile_height; i ++)
		  pixels[i] = pixel + info->width * info->channels * i * 2;
  }
  else
  {
	  for (i = 0; i < tile_height; i ++)
		  pixels[i] = pixel + info->width * info->channels * i;
  }

  for (pass = 0; pass < num_passes; pass ++)
  {
	  /*
	   * This works if you are only reading one row at a time...
	   */

	  for (begin = 0, end = tile_height;
			  begin < info->height;
			  begin += tile_height, end += tile_height)
	  {
		  if (end > info->height)
			  end = info->height;

		  num = end - begin;

		  if (pass != 0) /* to handle interlaced PiNGs */
			  gimp_pixel_rgn_get_rect(&pixel_rgn, pixel, 0, begin,
					  drawable->width, num);

		  png_read_rows(pp, pixels, NULL, num);

		  gimp_pixel_rgn_set_rect(&pixel_rgn, pixel, 0, begin,
				  drawable->width, num);

		  gimp_progress_update(((double)pass + (double)end / (double)info->height) /
				  (double)num_passes);
	  };
  };

#if defined(PNG_iCCP_SUPPORTED)
  /* set icc profile */
  if (info->iccp_proflen > 0) {
    gimp_image_set_icc_profile_by_mem (image, info->iccp_proflen,
                                              info->iccp_profile,
                                              ICC_IMAGE_PROFILE);
    printf ("%s:%d %s() set embedded profile \"%s\"\n",
             __FILE__,__LINE__,__func__,
                                              info->iccp_name);
  }
#endif

 /*
  * Done with the file...
  */

  png_read_end(pp, info);
#if PNG_LIBPNG_VER < 10200	/* ?? Anyway, this function isn't in 1.2.0*/
  png_read_destroy(pp, info, NULL);
#endif

  g_free(pixel);
  g_free(pixels);
  free(pp);
  free(info);

  fclose(fp);

  if (trns) {
    gimp_layer_add_alpha(layer);
    drawable = gimp_drawable_get(layer);
    gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                        drawable->height, TRUE, FALSE);

    pixel  = g_new(guchar, tile_height * drawable->width * 2); /* bpp == 1 */

    for (begin = 0, end = tile_height;
         begin < drawable->height;
         begin += tile_height, end += tile_height)
    {
      if (end > drawable->height) end = drawable->height;
      num= end - begin;

      gimp_pixel_rgn_get_rect(&pixel_rgn, pixel, 0, begin,
                                drawable->width, num);

      for (i= 0; i < tile_height * drawable->width; ++i) {
        pixel[i*2+1]= alpha [ pixel[i*2] ];
        pixel[i*2]-= empty;
      }

      gimp_pixel_rgn_set_rect(&pixel_rgn, pixel, 0, begin,
                              drawable->width, num);
    }
    g_free(pixel);
  }

 /*
  * Update the display...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);

  return (image);
}


/*
 * 'save_image ()' - Save the specified image to a PNG file.
 */

static gint
save_image (gchar  *filename,	        /* I - File to save to */
	    gint32  image_ID,	        /* I - Image to save */
	    gint32  drawable_ID,	/* I - Current drawable */
	    gint32  orig_image_ID)      /* I - Original image before export */
{
  int		i, k,		/* Looping vars */
		bpp = 0,	/* Bytes per pixel */
		type,		/* Type of drawable/layer */
		num_passes,	/* Number of interlace passes in file */
		pass,		/* Current pass in file */
		tile_height,	/* Height of tile in GIMP */
		begin,		/* Beginning tile row */
		end,		/* Ending tile row */
		num;		/* Number of rows to load */
  FILE		*fp;		/* File pointer */
  TileDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  png_structp	pp;		/* PNG read pointer */
  png_infop	info;		/* PNG info pointer */
  gint		num_colors;	/* Number of colors in colormap */
  gint		offx, offy;	/* Drawable offsets from origin */
  guchar	**pixels,	/* Pixel rows */
		*fixed,		/* Fixed-up pixel data */
		*pixel;		/* Pixel data */
  gchar		*progress;	/* Title for progress display... */
  //gdouble       xres, yres;	/* GIMP resolution (dpi) */ //FIXME Forgetting about resolution at least for the time being XXX
  gdouble	gamma;          /* GIMP gamma e.g. 2.20 */
  png_color_16  background;     /* Background color */
  png_time      mod_time;       /* Modification time (ie NOW) */
  guchar	red, green,
                blue;           /* Used for palette background */
  time_t	cutime;         /* Time since epoch */
  struct tm	*gmt;		/* GMT broken down */

 /*
  * PNG 0.89 and newer have a sane, forwards compatible constructor.
  * Some SGI IRIX users will not have a new enough version though
  */

#if PNG_LIBPNG_VER > 88
  pp   = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info = png_create_info_struct(pp);
#else
  pp = (png_structp)calloc(sizeof(png_struct), 1);
  png_write_init(pp);

  info = (png_infop)calloc(sizeof(png_info), 1);
#endif /* PNG_LIBPNG_VER > 88 */

  if (setjmp (pp->jmpbuf))
  {
    g_message ("%s\nPNG error. Couldn't save image", filename);
    return 0;
  }

 /*
  * Open the file and initialize the PNG write "engine"...
  */

  fp = fopen(filename, "wb");
  if (fp == NULL) {
    g_message ("%s\nCouldn't create file", filename);
    return 0;
  }

  png_init_io(pp, fp);

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf ("%s %s:", _("Saving"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf ("%s %s:", _("Saving"), filename);

  gimp_progress_init(progress);
  g_free (progress);

 /*
  * Get the drawable for the current image...
  */

  drawable = gimp_drawable_get (drawable_ID);
  type     = gimp_drawable_type (drawable_ID);

 /*
  * Set the image dimensions, bit depth, interlacing and compression
  */

  png_set_compression_level (pp, pngvals.compression_level);

  info->width          = drawable->width;
  info->height         = drawable->height;
  info->interlace_type = pngvals.interlaced;

 /*
  * Set color type and remember bytes per pixel count 
  */

  switch (type)
  {
    case RGB_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB;
	info->bit_depth      = 8;
        bpp              = 3;
        break;
    case RGBA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	info->bit_depth      = 8;
        bpp              = 4;
        break;
    case GRAY_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY;
	info->bit_depth      = 8;
        bpp              = 1;
        break;
    case GRAYA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
	info->bit_depth      = 8;
        bpp              = 2;
        break;
    case INDEXED_IMAGE :
	bpp		 = 1;
	info->bit_depth      = 8;
        info->color_type = PNG_COLOR_TYPE_PALETTE;
	info->valid      |= PNG_INFO_PLTE;
        info->palette= (png_colorp) gimp_image_get_cmap(image_ID, &num_colors);
        info->num_palette= num_colors;
        break;
    case INDEXEDA_IMAGE :
	bpp		 = 2;
	info->bit_depth      = 8;
	info->color_type = PNG_COLOR_TYPE_PALETTE;
	respin_cmap (pp, info, image_ID); /* fix up transparency */
	break;
    case U16_RGB_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB;
	info->bit_depth      = 16;
        bpp              = 6;
        break;
    case U16_RGBA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	info->bit_depth      = 16;
        bpp              = 8;
        break;
    case U16_GRAY_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY;
	info->bit_depth      = 16;
        bpp              = 2;
        break;
    case U16_GRAYA_IMAGE :
        info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
	info->bit_depth      = 16;
        bpp              = 4;
        break;
    case U16_INDEXED_IMAGE :
	bpp		 = 2;
	info->bit_depth      = 16;
        info->color_type = PNG_COLOR_TYPE_PALETTE;
	info->valid      |= PNG_INFO_PLTE;
        info->palette= (png_colorp) gimp_image_get_cmap(image_ID, &num_colors);
        info->num_palette= num_colors;
        break;
    case U16_INDEXEDA_IMAGE :
	bpp		 = 4;
	info->bit_depth      = 16;
	info->color_type = PNG_COLOR_TYPE_PALETTE;
	respin_cmap (pp, info, image_ID); /* fix up transparency */
	break;
    default:
        g_message ("%s\nImage type can't be saved as PNG %d", filename, type);
        return 0;
  };

 /*
  * Fix bit depths for (possibly) smaller colormap images
  */
  
  if (info->valid & PNG_INFO_PLTE) {
    if (info->num_palette <= 2)
      info->bit_depth= 1;
    else if (info->num_palette <= 4)
      info->bit_depth= 2;
    else if (info->num_palette <= 16)
      info->bit_depth= 4;
    /* otherwise the default is fine */
  }

  // write icc profile
#if defined(PNG_iCCP_SUPPORTED)
  if (gimp_image_has_icc_profile (image_ID, ICC_IMAGE_PROFILE)) {
    int size;
    char *buffer;
    
    buffer = gimp_image_get_icc_profile_by_mem (image_ID, &size,
                                                ICC_IMAGE_PROFILE);
    png_set_iCCP (pp, info,
           gimp_image_get_icc_profile_description (image_ID, ICC_IMAGE_PROFILE),
                  0, buffer, size);
    printf ("%s:%d %s() embedd icc profile \"%s\"\n",
             __FILE__,__LINE__,__func__,
                                              info->iccp_name);
  }
#endif

  /* All this stuff is optional extras, if the user is aiming for smallest
     possible file size she can turn them all off */

#if PNG_LIBPNG_VER > 99
  if (pngvals.bkgd) {
    gimp_palette_get_background(&red, &green, &blue);
      
    background.index = 0;
    background.red = red;
    background.green = green;
    background.blue = blue;
    background.gray = (red + green + blue) / 3;
    png_set_bKGD(pp, info, &background);
  }

  if (pngvals.gama) {
    gamma = gimp_gamma();
    png_set_gAMA(pp, info, 1.0 / (gamma != 1.00 ? gamma : DEFAULT_GAMMA));
  }

  if (pngvals.offs) {
    gimp_drawable_offsets(drawable_ID, &offx, &offy);
    if (offx != 0 || offy != 0) {
      png_set_oFFs(pp, info, offx, offy, PNG_OFFSET_PIXEL);
    }
  }

  if (pngvals.phys) {
    //gimp_image_get_resolution (orig_image_ID, &xres, &yres); //FIXME
    //png_set_pHYs(pp, info, xres * 39.37, yres * 39.37, PNG_RESOLUTION_METER); //FIXME We forget about resolution XXX
  }

  if (pngvals.time) {
    cutime= time(NULL); /* time right NOW */
    gmt = gmtime(&cutime);

    mod_time.year = gmt->tm_year + 1900;
    mod_time.month = gmt->tm_mon + 1;
    mod_time.day = gmt->tm_mday;
    mod_time.hour = gmt->tm_hour;
    mod_time.minute = gmt->tm_min;
    mod_time.second = gmt->tm_sec;
    png_set_tIME(pp, info, &mod_time);
  }

#endif /* PNG_LIBPNG_VER > 99 */

  png_write_info (pp, info);

 /*
  * Turn on interlace handling...
  */

  if (pngvals.interlaced)
    num_passes = png_set_interlace_handling(pp);
  else
    num_passes = 1;

 /*
  * Convert unpacked pixels to packed if necessary
  */

  if (info->color_type == PNG_COLOR_TYPE_PALETTE && info->bit_depth < 8)
    png_set_packing(pp);

  /* Set swapping for 16 bit per sample images */
  
#ifndef WORDS_BIGENDIAN
  if (info->bit_depth == 16)
	  png_set_swap(pp);
#endif
  

 /*
  * Allocate memory for "tile_height" rows and save the image...
  */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * drawable->width * bpp);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i]= pixel + drawable->width * bpp * i;

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, FALSE, FALSE);

  for (pass = 0; pass < num_passes; pass ++)
  {
      /* This works if you are only writing one row at a time... */
    for (begin = 0, end = tile_height;
         begin < drawable->height;
         begin += tile_height, end += tile_height)
      {
	if (end > drawable->height)
	  end = drawable->height;

	num = end - begin;
	
	gimp_pixel_rgn_get_rect (&pixel_rgn, pixel, 0, begin, drawable->width, num);
        if (info->valid & PNG_INFO_tRNS) {
          for (i = 0; i < num; ++i) {
	    fixed= pixels[i];
            for (k = 0; k < drawable->width; ++k) {
              fixed[k] = (fixed[k*2+1] > 127) ? fixed[k*2] + 1 : 0;
            }
          }
       /* Forgot this case before, what if there are too many colors? */
        } else if (info->valid & PNG_INFO_PLTE && bpp == 2) {
          for (i = 0; i < num; ++i) {
	    fixed= pixels[i];
            for (k = 0; k < drawable->width; ++k) {
              fixed[k] = fixed[k*2];
            }
          }
        }
	
	png_write_rows (pp, pixels, num);
	
	gimp_progress_update (((double)pass + (double)end /
                    (double)info->height) / (double)num_passes);
      };
  };

  png_write_end (pp, info);
#if PNG_LIBPNG_VER < 10200	/* ?? Anyway, this function isn't in 1.2.0*/
  png_write_destroy (pp);
#endif

  g_free (pixel);
  g_free (pixels);

 /*
  * Done with the file...
  */

  free (pp);
  free (info);

  fclose (fp);

  return (1);
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer  data)
{
  runme = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

/* respin_cmap actually reverse fills the colormap, so that empty entries
   are left at the FRONT, this is only needed to reduce the size of the
   tRNS chunk when saving GIF-like transparent images with colormaps */

static void respin_cmap (png_structp pp, png_infop info, gint32 image_ID) {
  static const guchar trans[] = { 0 };
  static guchar after[3 * 256];
  gint colors;
  guchar *before;

  before= gimp_image_get_cmap(image_ID, &colors);

#if PNG_LIBPNG_VER > 99
  if (colors < 256) { /* spare space in palette :) */
    memcpy(after + 3, before, colors * 3);

    /* Apps with no natural background will use this instead, see
       elsewhere for the bKGD chunk being written to use index 0 */
    gimp_palette_get_background(after+0, after+1, after+2);

    /* One transparent palette entry, alpha == 0 */
    png_set_tRNS(pp, info, (png_bytep) trans, 1, NULL);
    png_set_PLTE(pp, info, (png_colorp) after, colors + 1);
  } else {
    png_set_PLTE(pp, info, (png_colorp) before, colors);
  }
#else
  info->valid	  |= PNG_INFO_PLTE;
  info->palette=     (png_colorp) before;
  info->num_palette= colors;
#endif /* PNG_LIBPNG_VER > 99 */

}

/*  local callbacks of gimp_dialog_new ()  */
static gint
gimp_dialog_delete_callback (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer   data) 
{
  GtkSignalFunc  cancel_callback;
  GtkWidget     *cancel_widget;

  cancel_callback =
    (GtkSignalFunc) gtk_object_get_data (GTK_OBJECT (widget),
					 "gimp_dialog_cancel_callback");
  cancel_widget =
    (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
				      "gimp_dialog_cancel_widget");

#if GTK_MAJOR_VERSION < 2
  /*  the cancel callback has to destroy the dialog  */
  if (cancel_callback)
    (* cancel_callback) (cancel_widget, data);
#endif

  return TRUE;
}


#if 0

void
gimp_dialog_create_action_areav (GtkDialog *dialog,
				 va_list    args)
{
  GtkWidget *hbbox = NULL;
  GtkWidget *button;

  /*  action area variables  */
  const gchar    *label;
  GtkSignalFunc   callback;
  gpointer        data;
  GtkObject      *slot_object;
  GtkWidget     **widget_ptr;
  gboolean        default_action;
  gboolean        connect_delete;

  gboolean delete_connected = FALSE;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  /*  prepare the action_area  */
  label = va_arg (args, const gchar *);

  if (label)
    {
      gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 2);
      gtk_box_set_homogeneous (GTK_BOX (dialog->action_area), FALSE);

      hbbox = gtk_hbutton_box_new ();
      gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
      gtk_box_pack_end (GTK_BOX (dialog->action_area), hbbox, FALSE, FALSE, 0);
      gtk_widget_show (hbbox);
    }

  /*  the action_area buttons  */
  while (label)
    {
      callback       = va_arg (args, GtkSignalFunc);
      data           = va_arg (args, gpointer);
      slot_object    = va_arg (args, GtkObject *);
      widget_ptr     = va_arg (args, GtkWidget **);
      default_action = va_arg (args, gboolean);
      connect_delete = va_arg (args, gboolean);

      button = gtk_button_new_with_label (label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

      if (slot_object == (GtkObject *) 1)
	slot_object = GTK_OBJECT (dialog);

      if (data == NULL)
	data = dialog;

      if (callback)
	{
	  if (slot_object)
	    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				       GTK_SIGNAL_FUNC (callback),
				       slot_object);
	  else
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (callback),
				data);
	}

      if (widget_ptr)
	*widget_ptr = button;

      if (connect_delete && callback && !delete_connected)
	{
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_callback",
			       callback);
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_widget",
			       slot_object ? slot_object : GTK_OBJECT (button));

	  /*  catch the WM delete event  */
	  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			      GTK_SIGNAL_FUNC (gimp_dialog_delete_callback),
			      data);

	  delete_connected = TRUE;
	}

      if (default_action)
	gtk_widget_grab_default (button);
      gtk_widget_show (button);

      label = va_arg (args, gchar *);
    }
}

GtkWidget *
gimp_dialog_newv (const gchar       *title,
		const gchar       *wmclass_name,
		//GimpHelpFunc       help_func, //FIXME
		//const gchar       *help_data, //FIXME
		GtkWindowPosition  position,
		gint               allow_shrink,
		gint               allow_grow,
		gint               auto_shrink,
		va_list            args)
{
	GtkWidget *dialog;

	g_return_val_if_fail (title != NULL, NULL);
	g_return_val_if_fail (wmclass_name != NULL, NULL);

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), title);
	gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, PROGRAM_NAME);
	gtk_window_set_position (GTK_WINDOW (dialog), position);
	gtk_window_set_policy (GTK_WINDOW (dialog),
			allow_shrink, allow_grow, auto_shrink);

	/*  prepare the action_area  */
	gimp_dialog_create_action_areav (GTK_DIALOG (dialog), args);

	/*  connect the "F1" help key  */
	//if (help_func) //FIXME
	//	gimp_help_connect_help_accel (dialog, help_func, help_data); //FIXME

	return dialog;
}


GtkWidget *
gimp_dialog_new (const gchar       *title,
		const gchar       *wmclass_name,
		//GimpHelpFunc       help_func, //FIXME
		//const gchar       *help_data, //FIXME
		GtkWindowPosition  position,
		gint               allow_shrink,
		gint               allow_grow,
		gint               auto_shrink,
		...)
{
	GtkWidget *dialog;
	va_list    args;

	va_start (args, auto_shrink);

	dialog = gimp_dialog_newv (title,
			wmclass_name,
			help_func, 
			help_data, 
			position,
			allow_shrink,
			allow_grow,
			auto_shrink,
			args);

	va_end (args);

	return dialog;
}

#endif
/**
 * gimp_toggle_button_sensitive_update:
 * @toggle_button: The #GtkToggleButton the "set_sensitive" and
 *                 "inverse_sensitive" lists are attached to.
 *
 * If you attached a pointer to a #GtkWidget with gtk_object_set_data() and
 * the "set_sensitive" key to the #GtkToggleButton, the sensitive state of
 * the attached widget will be set according to the toggle button's
 * "active" state.
 *
 * You can attach an arbitrary list of widgets by attaching another
 * "set_sensitive" data pointer to the first widget (and so on...).
 *
 * This function can also set the sensitive state according to the toggle
 * button's inverse "active" state by attaching widgets with the
 * "inverse_sensitive" key.
 **/
/*void
gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button)
{
  GtkWidget *set_sensitive;
  gboolean   active;

  active = gtk_toggle_button_get_active (toggle_button);

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "set_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "set_sensitive");
    }

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "inverse_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, ! active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "inverse_sensitive");
    }
}*/


/**
 * gimp_toggle_button_update:
 * @widget: A #GtkToggleButton.
 * @data:   A pointer to a #gint variable which will store the value of
 *          gtk_toggle_button_get_active().
 *
 * Note that this function calls gimp_toggle_button_sensitive_update().
 **/
/*void
gimp_toggle_button_update (GtkWidget *widget,
			   gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));
}*/


/**
 * gimp_table_attach_aligned:
 * @table:      The #GtkTable the widgets will be attached to.
 * @column:     The column to start with.
 * @row:        The row to attach the eidgets.
 * @label_text: The text for the #GtkLabel which will be attached left of the
 *              widget.
 * @xalign:     The horizontal alignment of the #GtkLabel.
 * @yalign:     The vertival alignment of the #GtkLabel.
 * @widget:     The #GtkWidget to attach right of the label.
 * @colspan:    The number of columns the widget will use.
 * @left_align: #TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be #NULL and that the widget will be attached
 * starting at (@column + 1) in this case, too.
 **/
/*void
gimp_table_attach_aligned (GtkTable    *table,
			   gint         column,
			   gint         row,
			   const gchar *label_text,
			   gfloat       xalign,
			   gfloat       yalign,
			   GtkWidget   *widget,
			   gint         colspan,
			   gboolean     left_align)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, label,
			column, column + 1,
			row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  if (left_align)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
      gtk_widget_show (widget);

      widget = alignment;
    }

  gtk_table_attach (table, widget,
		    column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (widget);
}*/

/**
 * gimp_int_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gint variable which will store the
 *              @adjustment's value.
 *
 * Note that the #GtkAdjustment's value (which is a #gfloat) will be rounded
 * with RINT().
 **/
/*void
gimp_int_adjustment_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  gint *val;

  val = (gint *) data;
  *val = rint(adjustment->value);
}*/


static gint
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *scale;
  GtkObject *scale_data;

  dlg = gimp_dialog_new (_("Save as PNG"), "png",
			 gimp_standard_help_func, "filters/png.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 7, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  toggle = gtk_check_button_new_with_label (_("Interlacing (Adam7)"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.interlaced);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.interlaced);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save background color"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 1, 2,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.bkgd);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.bkgd);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save gamma"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 2, 3,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.gama);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.gama);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save layer offset"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 3, 4,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.offs);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.offs);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save resolution"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 4, 5,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.phys);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.phys);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save creation time"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 5, 6,
		    GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &pngvals.time);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), pngvals.time);
  gtk_widget_show (toggle);

  scale_data = gtk_adjustment_new (pngvals.compression_level,
				   1.0, 9.0, 1.0, 1.0, 0.0);
  scale      = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
			     _("Compression Level:"), 1.0, 1.0,
			     scale, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pngvals.compression_level);
  gtk_widget_show (scale);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return runme;
}


