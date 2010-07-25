/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* JPEG loading and saving file filter for the GIMP
 *  -Peter Mattis
 *
 * This filter is heavily based upon the "example.c" file in libjpeg.
 * In fact most of the loading and saving code was simply cut-and-pasted
 *  from that file. The filter, therefore, also uses libjpeg.
 */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include "gtk/gtk.h"
#include "lib/plugin_main.h"
#include "libgimp/stdplugins-intl.h"

#include <stdio.h>
#include "lcms.h"
#include "iccjpeg.h"


#define SCALE_WIDTH 125

typedef struct
{
  gdouble quality;
  gdouble smoothing;
  gint optimize;
} JpegSaveVals;

typedef struct
{
  gint  run;
} JpegSaveInterface;

/* Declare local functions.
 */
static void   query      (void);
static void   run        (char    *name,
			  int      nparams,
			  GParam  *param,
			  int     *nreturn_vals,
			  GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
			  gint32  image_ID,
			  gint32  drawable_ID);

static gint   save_dialog ();

static void   save_close_callback  (GtkWidget *widget,
				    gpointer   data);
static void   save_ok_callback     (GtkWidget *widget,
				    gpointer   data);
static void   save_scale_update    (GtkAdjustment *adjustment,
				    double        *scale_val);
static void   save_optimize_update (GtkWidget *widget,
				    gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static JpegSaveVals jsvals =
{
  0.75,  /*  quality  */
  0.0,   /*  smoothing  */
  1      /*  optimize  */
};

static JpegSaveInterface jsint =
{
  FALSE   /*  run  */
};


MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_FLOAT, "quality", "Quality of saved image (0 <= quality <= 1)" },
    { PARAM_FLOAT, "smoothing", "Smoothing factor for saved image (0 <= smoothing <= 1)" },
    { PARAM_INT32, "optimize", "Optimization of entropy encoding parameters" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_jpeg_load",
                          "loads files of the jpeg file format",
			  "FIXME: write help for jpeg_load",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          "<Load>/Jpeg",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_jpeg_save",
                          "saves files in the jpeg file format",
                          "FIXME: write help for jpeg_save",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          "<Save>/Jpeg",
                          "*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_jpeg_load", "jpg,jpeg,jp4", "", "6,string,JFIF");
  gimp_register_save_handler ("file_jpeg_save", "jpg,jpeg", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_jpeg_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_jpeg_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_jpeg_save", &jsvals);

          image_ID = param[1].data.d_int32;

          {
            char *buffer = NULL;
            int size = 0;

            if (gimp_image_has_icc_profile (image_ID, ICC_IMAGE_PROFILE))
            {
              buffer = gimp_image_get_icc_profile_by_mem (image_ID, &size, ICC_IMAGE_PROFILE);
              if (buffer)
              {
                cmsHPROFILE lp = cmsOpenProfileFromMem( buffer, size );
                if(lp)
                {
                  if(cmsGetColorSpace(lp) == icSigCmykData)
                  {
                    g_message ("%s:%d  Cmyk data in JPEG is not recommended!\n",
                               __FILE__,__LINE__);
                    status = STATUS_EXECUTION_ERROR;
                  }
                  cmsCloseProfile( lp );
                }
                free(buffer);
              }
            }
          }

          if(status == STATUS_EXECUTION_ERROR)
          {
            values[0].data.d_status = STATUS_EXECUTION_ERROR;
            return;
          }

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 7)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      jsvals.quality = param[5].data.d_float;
	      jsvals.smoothing = param[6].data.d_float;
	      jsvals.optimize = param[7].data.d_int32;
	    }
	  if (status == STATUS_SUCCESS &&
	      (jsvals.quality < 0.0 || jsvals.quality > 1.0))
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS &&
	      (jsvals.smoothing < 0.0 || jsvals.smoothing > 1.0))
	    status = STATUS_CALLING_ERROR;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_jpeg_save", &jsvals);
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  /*  Store mvals data  */
	  gimp_set_data ("file_jpeg_save", &jsvals, sizeof (JpegSaveVals));

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}


typedef struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;


/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}

static gint32
load_image (char *filename)
{
  GPixelRgn pixel_rgn;
  TileDrawable *drawable;
  gint32 image_ID;
  gint32 layer_ID;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *infile;
  guchar *buf;
  guchar **rowbuf;
  char *name;
  int image_type;
  int layer_type;
  int tile_height;
  int scanlines;
  int i, start, end;
  int m;
  int depth = 8;

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if ((infile = fopen (filename, "rb")) == NULL)
    {
      g_warning ("can't open \"%s\"\n", filename);
      gimp_quit ();
    }

  if( strrchr(filename,'.') &&
      strcmp( strrchr(filename, '.'), ".jp4") == 0 )
    depth = 16;

  name = malloc (strlen (filename) + 12);
  sprintf (name, "%s %s:", _("Loading"), filename);
  gimp_progress_init (name);
  free (name);

  image_ID = -1;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
	fclose (infile);
      if (image_ID != -1)
	gimp_image_delete (image_ID);
      gimp_quit ();
    }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  setup_read_icc_profile(&cinfo);  
  for (m = 0; m < 16; m++)
    jpeg_save_markers(&cinfo, JPEG_APP0 + m, 0xFFFF);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */
  prepareColour( &cinfo );

  /* Step 5: Start decompressor */

  jpeg_start_decompress (&cinfo);

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer */
  tile_height = gimp_tile_height ();
  buf = g_new (guchar, tile_height * cinfo.output_width * cinfo.output_components);
  rowbuf = g_new (guchar*, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  /* Create a new image of the proper size and associate the filename with it.
   */
  if(depth == 8)
  {
    switch (cinfo.output_components)
    {
    case 1:
      image_type = GRAY;
      layer_type = GRAY_IMAGE;
      break;
    case 3:
      image_type = RGB;
      layer_type = RGB_IMAGE;
      break;
    case 4:
      image_type = RGB;
      layer_type = RGBA_IMAGE;
      break;
    default:
      gimp_quit ();
    }
  } else {
    switch (cinfo.output_components)
    {
    case 1:
      image_type = U16_GRAY;
      layer_type = U16_GRAY_IMAGE;
      break;
    case 3:
      image_type = U16_RGB;
      layer_type = U16_RGB_IMAGE;
      break;
    case 4:
      image_type = U16_RGB;
      layer_type = U16_RGBA_IMAGE;
      break;
    default:
      gimp_quit ();
    }
  }

  image_ID = gimp_image_new (cinfo.output_width / (depth/8), cinfo.output_height,
                             image_type);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, _("Background"),
			     cinfo.output_width / (depth/8),
			     cinfo.output_height,
			     layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
    {
      start = cinfo.output_scanline;
      end = cinfo.output_scanline + tile_height;
      end = MIN (end,CAST(int) cinfo.output_height);
      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
	jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

      /*
      for (i = start; i < end; i++)
	gimp_pixel_rgn_set_row (&pixel_rgn, tilerow[i - start], 0, i, drawable->width);
	*/

      if(cinfo.out_color_space == JCS_CMYK)
        for(i = 0; i < scanlines*drawable->width*cinfo.output_components; ++i)
          buf[i] = 255 - buf[i];

      if(depth == 16 && 0)
        for(i = 0; i < scanlines*drawable->width*cinfo.output_components; ++i)
        {
          unsigned char c = buf[2*i];
          buf[2*i] = buf[2*i+1];
          buf[2*i+1] = c;
        }

      gimp_pixel_rgn_set_rect (&pixel_rgn, buf, 0, start, drawable->width, scanlines);

      gimp_progress_update ((double) cinfo.output_scanline / (double) cinfo.output_height);
    }

  // Step 6a: read icc profile
  {
    LPBYTE Buffer = NULL;
    size_t Len = 0;
    cmsHPROFILE hProfile=NULL;

    if (read_icc_profile(&cinfo, &Buffer, &Len))
    {
      printf ("%s:%d %s() embedded profile found\n",__FILE__,__LINE__,__func__);
    } else if (read_icc_profile2(&cinfo, &Buffer, &Len)) {
      printf ("%s:%d %s() default profile selected\n",__FILE__,__LINE__,__func__);
    }

    if(Buffer && Len)
    {
      hProfile = cmsOpenProfileFromMem(Buffer, Len);
      if (hProfile) {
        gimp_image_set_icc_profile_by_mem (image_ID, Len, Buffer, ICC_IMAGE_PROFILE);
        cmsCloseProfile (hProfile);
        free(Buffer);
        printf ("%s:%d %s() set profile\n",__FILE__,__LINE__,__func__);
      }
    }
  }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress (&cinfo);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  /* Tell the GIMP to display the image.
   */
  gimp_drawable_flush (drawable);

  return image_ID;
}

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GPixelRgn pixel_rgn;
  TileDrawable *drawable;
  GDrawableType drawable_type;
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *outfile;
  guchar *temp, *t;
  guchar *data;
  guchar *src, *s;
  char *name;
  int has_alpha;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;
  gint32 e_image_ID = -1;
  gint32 e_drawable_ID = -1;
  GPrecisionType precision;

  int rowstride, yend;
  int i, j;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  name = malloc (strlen (filename) + 11);

  sprintf (name, "%s %s:",_("Saving"), filename);
  gimp_progress_init (name);
  free (name);

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  outfile = NULL;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_compress (&cinfo);
      if (outfile)
	fclose (outfile);
      if (drawable)
	gimp_drawable_detach (drawable);

      return FALSE;
    }

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress (&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = fopen (filename, "wb")) == NULL)
    {
      fprintf (stderr, "can't open %s\n", filename);
      return FALSE;
    }
  jpeg_stdio_dest (&cinfo, outfile);

  /* Get the input image and a pointer to its data.
   */
  cinfo.input_components = gimp_drawable_num_channels(drawable_ID);
  has_alpha = gimp_drawable_has_alpha(drawable_ID);
  if(has_alpha)
    --cinfo.input_components;

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width = drawable->width;
  cinfo.image_height = drawable->height;
  /* colorspace of input image */
  cinfo.in_color_space = gimp_drawable_color(drawable_ID) 
    ? JCS_RGB : JCS_GRAYSCALE;

  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality (&cinfo, (int) (jsvals.quality * 100), TRUE /* limit to baseline-JPEG values */);
  cinfo.smoothing_factor = (int) (jsvals.smoothing * 100);
  cinfo.optimize_coding = jsvals.optimize;

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress (&cinfo, TRUE);

  // Step 4a: write icc profile
  {
    char *buffer = NULL;
    int size = 0;

    if (gimp_image_has_icc_profile (image_ID, ICC_IMAGE_PROFILE)) { 
      buffer = gimp_image_get_icc_profile_by_mem (image_ID, &size, ICC_IMAGE_PROFILE);
      if (buffer) {
        write_icc_profile (&cinfo, buffer, size);
        printf ("%s:%d %s() embedded icc profile\n",__FILE__,__LINE__,__func__);
      }
    }
  }

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  /* JSAMPLEs per row in image_buffer */
  rowstride = drawable->bpp * drawable->width;
  temp = (guchar *) malloc (cinfo.image_width * cinfo.input_components * 4);
  data = (guchar *) malloc (rowstride * gimp_tile_height ());

        export = gimp_export_image (&image_ID, &drawable_ID, "Jpeg",
                                    (GIMP_EXPORT_CAN_HANDLE_RGB |
                                     GIMP_EXPORT_CAN_HANDLE_GRAY |
                                     GIMP_EXPORT_CAN_HANDLE_INDEXED));
        if (export == GIMP_EXPORT_CANCEL)
        {
          gimp_message("An error during export occured.");
          return FALSE;
        } else if (export == GIMP_EXPORT_EXPORT) {
          e_image_ID = image_ID;
          e_drawable_ID = drawable_ID;
          drawable_type = gimp_drawable_type(e_drawable_ID);
        }

  precision = gimp_drawable_precision(drawable_ID);

/* taken from app/depth/bfp.h */
#define ONE_BFP 32768
#define BFP_TO_U8(x) (x < ONE_BFP ? x >> 7 : 255)

  while (cinfo.next_scanline < cinfo.image_height)
    {
      guint16 *s16;
      gfloat *f32;
      gfloat f;
      ShortsFloat u;
      guint16 bfp;

      if ((cinfo.next_scanline % gimp_tile_height ()) == 0)
	{
	  yend = cinfo.next_scanline + gimp_tile_height ();
	  yend = MIN (yend,CAST(int) cinfo.image_height);
	  gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, cinfo.next_scanline, cinfo.image_width,
				   (yend - cinfo.next_scanline));
	  src = data;
	}

      t = temp;
      s = src;
      i = cinfo.image_width;
      s16 = (guint16*) s;
      f32 = (gfloat*) s;

      while (i--)
	{
          switch(precision)
          {
          case PRECISION_U8:
  	    for (j = 0; j < cinfo.input_components; j++)
	      *t++ = *s++;
	    if (has_alpha)  /* ignore alpha channel */
	      s++;
          break;
          case PRECISION_U16:
            {
  	    for (j = 0; j < cinfo.input_components; j++)
	      *t++ = (guint8)((*s16++)/257);
	    if (has_alpha)  /* ignore alpha channel */
	      s16++;
            }
          break;
          case PRECISION_FLOAT16:
            {
  	    for (j = 0; j < cinfo.input_components; j++)
            {
              f = (FLT(*s16++,u)) * 255.;
	      *t++ = (guint8)(CLAMP(f, 0, 255));
            }
	    if (has_alpha)  /* ignore alpha channel */
	      s16++;
            }
          break;
          case PRECISION_FLOAT:
            {
  	    for (j = 0; j < cinfo.input_components; j++)
            {
              f = (*f32++) * 255.;
	      *t++ = (guint8)(CLAMP(f, 0, 255));
            }
	    if (has_alpha)  /* ignore alpha channel */
	      f32++;
            }
          break;
          case PRECISION_BFP:
            {
  	    for (j = 0; j < cinfo.input_components; j++)
            {
              bfp = *s16++;
	      *t++ = (guint8)(BFP_TO_U8(bfp));
            }
	    if (has_alpha)  /* ignore alpha channel */
	      s16++;
            }
          break;
          }
	}

      src += rowstride;
      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &temp, 1);

      if ((cinfo.next_scanline % 5) == 0)
	gimp_progress_update ((double) cinfo.next_scanline / (double) cinfo.image_height);
    }

  /* Step 6: Finish compression */
  jpeg_finish_compress (&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose (outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress (&cinfo);
  /* free the temporary buffer */
  free (temp);
  free (data);

  /* And we're done! */
  /*gimp_do_progress (1, 1);*/

  if (export == GIMP_EXPORT_EXPORT) {
    gimp_image_delete (e_image_ID);
    gimp_image_clean_all (e_image_ID);
  }

  gimp_drawable_detach (drawable);

  return TRUE;
}

static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *scale_data;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Save as Jpeg"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) save_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new (_("Quality"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  scale_data = gtk_adjustment_new (jsvals.quality, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) save_scale_update,
		      &jsvals.quality);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new (_("Smoothing"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  scale_data = gtk_adjustment_new (jsvals.smoothing, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) save_scale_update,
		      &jsvals.smoothing);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  toggle = gtk_check_button_new_with_label(_("Optimize"));
  gtk_table_attach(GTK_TABLE(table), toggle, 0, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc)save_optimize_update, NULL);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), jsvals.optimize);
  gtk_widget_show(toggle);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return jsint.run;
}


/*  Save interface functions  */

static void
save_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  jsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_scale_update (GtkAdjustment *adjustment,
		   double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
save_optimize_update(GtkWidget *widget,
		     gpointer  data)
{
  jsvals.optimize = GTK_TOGGLE_BUTTON(widget)->active;
}
