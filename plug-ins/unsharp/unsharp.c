/* $Id: unsharp.c,v 1.9 2006/12/31 09:01:31 beku Exp $
 *
 * unsharp.c 0.10 -- This is a plug-in for the GIMP 1.0
 *  http://www.stdout.org/~winston/gimp/unsharp.html
 *  (now out of date) 
 *
 * Copyright (C) 1999 Winston Chang
 *                    <winstonc@cs.wisc.edu>
 *                    <winston@stdout.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

#ifndef GIMP_VERSION
#include "lib/rint.h"
#include "lib/widgets.h"
#endif

#define PLUG_IN_VERSION "0.10"

#define SCALE_WIDTH  150

/* to show both pretty unoptimized code and ugly optimized code blocks
   There's really no reason to define this, unless you want to see how
   much pointer aritmetic can speed things up.  I find that it is about
   45% faster with the optimized code. */
/* #define READABLE_CODE */

/* uncomment this line to get a rough feel of how long the
   plug-in takes to run */
/* #define TIMER */
#ifdef TIMER
#include <sys/time.h>
#include <unistd.h>

static void timerstart (void);
static void timerstop  (void);
static struct timeval time_start, time_stop;
#endif

typedef struct
{
  gdouble radius;
  gdouble amount;
  gint    threshold;
} UnsharpMaskParams;

typedef struct
{
  gint run;
} UnsharpMaskInterface;

/* local function prototypes */
static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GimpParam  *param,
		   gint    *nreturn_vals,
		   GimpParam **return_vals);

static inline void blur_line     (gdouble    *ctable,
				  gdouble    *cmatrix,
				  gint        cmatrix_length,
				  guchar     *cur_col,
				  guchar     *dest_col,
				  gint        y,
				  glong       bytes);
static int gen_convolve_matrix   (gdouble     std_dev,
				  gdouble   **cmatrix);
static gdouble* gen_lookup_table (gdouble*    cmatrix,
				  gint        cmatrix_length);
static void unsharp_region       (GPixelRgn   srcPTR,
				  GPixelRgn   dstPTR,
				  gint        width,
				  gint        height,
				  gint        bytes,
				  gdouble     radius,
				  gdouble     amount,
				  gint        x1,
				  gint        x2,
				  gint        y1,
				  gint        y2);

static void unsharp_mask         (TileDrawable *drawable,
				  gint       radius,
				  gdouble    amount);

static void unsharp_ok_callback  (GtkWidget *widget,
				  gpointer   data);
static gint unsharp_mask_dialog  (void);

/* preview shit -- not finished yet */
#undef PREVIEW
#ifdef PREVIEW
static void preview_scroll_callback (void);
static void preview_init            (void);
static void preview_exit            (void);
static void preview_update          (void);

static GtkWidget* preview;
static gint       preview_width;    /* Width of preview widget */
static gint       preview_height;   /* Height of preview widget */
static gint       preview_x1;       /* Upper-left X of preview */
static gint       preview_y1;       /* Upper-left Y of preview */
static gint       preview_x2;       /* Lower-right X of preview */
static gint       preview_y2;       /* Lower-right Y of preview */

static gint       sel_width;    /* Selection width */
static gint       sel_height;   /* Selection height */
		
static GtkObject *hscroll_data;    /* Horizontal scrollbar data */
static GtkObject *vscroll_data;    /* Vertical scrollbar data */
#endif

static gint       run_filter = FALSE;
		

/* create a few globals, set default values */
static UnsharpMaskParams unsharp_params =
{
  30.0, /* default radius = 5 */
  0.5, /* default amount = .5 */
  0    /* default threshold = 0 */
};

/* static UnsharpMaskInterface umint = { FALSE }; */

/* Setting PLUG_IN_INFO */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "(unused)" },
    { PARAM_DRAWABLE, "drawable", "Drawable to draw on" },
    { PARAM_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
    { PARAM_FLOAT, "amount", "Strength of effect" },
    { PARAM_FLOAT, "threshold", "Threshold" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);
	
  /* Install a procedure in the procedure database. */
  gimp_install_procedure ("plug_in_unsharp_mask",
			  "An unsharp mask filter",
			  "The unsharp mask is a sharpening filter that works "
			  "by comparing using the difference of the image and "
			  "a blurred version of the image.  It is commonly "
			  "used on photographic images, and is provides a much "
			  "more pleasing result than the standard sharpen "
			  "filter.",
			  "Winston Chang <winstonc@cs.wisc.edu>",
			  "Winston Chang",
			  "1999",
			  "<Image>/Filters/Enhance/Unsharp Mask...",
			  //"GRAY*, RGB*",
			  "RGB*, GRAY*, U16_RGB*, U16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
  _("Unsharp Mask...");
  _("Enhance");
}

/* this is the actual function */
static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  TileDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

#ifdef TIMER
  timerstart();
#endif

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  *return_vals = values;
  *nreturn_vals = 1;


  INIT_I18N_UI(); 

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      gimp_get_data ("plug_in_unsharp_mask", &unsharp_params);
      if (! unsharp_mask_dialog ()) return;
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 6)
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
	  unsharp_params.radius = param[3].data.d_float;
	  unsharp_params.amount = param[4].data.d_float; 
	  unsharp_params.threshold = param[5].data.d_int32;

	  /* make sure there are legal values */
	  if ((unsharp_params.radius < 0.0) ||
	      (unsharp_params.amount<0.0))
	    status = STATUS_CALLING_ERROR;
	}
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_unsharp_mask", &unsharp_params);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

      /* here we go */
      unsharp_mask (drawable, unsharp_params.radius, unsharp_params.amount);

      /* values[0].data.d_status = status; */
      gimp_displays_flush ();

      /* set data for next use of filter */
      gimp_set_data ("plug_in_unsharp_mask", &unsharp_params,
		     sizeof (UnsharpMaskParams));

      /*fprintf(stderr, "%f %f\n", unsharp_params.radius, unsharp_params.amount);*/

      gimp_drawable_detach(drawable);
      values[0].data.d_status = status;
    }
#ifdef TIMER
  timerstop ();
#endif
}

/* -------------------------- Unsharp Mask ------------------------- */
static void
unsharp_mask (TileDrawable *drawable,
	      gint       radius,
	      gdouble    amount)
{
  GPixelRgn srcPR, destPR;
  glong width, height;
  glong bytes;
  gint  x1, y1, x2, y2;

  /* Get the input */
  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  gimp_progress_init(_("Blurring..."));

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  /* initialize pixel regions */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  unsharp_region (srcPR, destPR, width, height, bytes, radius, amount,
		  x1, x2, y1, y2);

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, x1, y1, (x2-x1), (y2-y1));
}

/* perform an unsharp mask on the region, given a source region, dest.
   region, width and height of the regions, and corner coordinates of
   a subregion to act upon.  Everything outside the subregion is unaffected.
  */
static void
unsharp_region (GPixelRgn srcPR,
		GPixelRgn destPR,
		gint      width,
		gint      height,
		gint      bytes,
		gdouble   radius,
		gdouble   amount,
		gint      x1,
		gint      x2,
		gint      y1,
		gint      y2)
{
  guchar  *cur_col;
  guchar  *dest_col;
  guchar  *cur_row;
  guchar  *dest_row;
  gint     x;
  gint     y;
  gdouble *cmatrix = NULL;
  gint     cmatrix_length;
  gdouble *ctable;

  gint row, col;  /* these are counters for loops */

  /* these are used for the merging step */
  gint threshold;
  gint diff;
  gint value;
  gint u,v;

  /* find height and width of subregion to act on */
  x = x2-x1;
  y = y2-y1;

  /* generate convolution matrix and make sure it's smaller than each dimension */
  cmatrix_length = gen_convolve_matrix(radius, &cmatrix);
  /* generate lookup table */
  ctable = gen_lookup_table(cmatrix, cmatrix_length);

  /*  allocate row buffers  */
  cur_row  = g_new (guchar, x * bytes);
  dest_row = g_new (guchar, x * bytes);

  /* find height and width of subregion to act on */
  x = x2-x1;
  y = y2-y1;

  /* blank out a region of the destination memory area, I think */
  for (row = 0; row < y; row++)
    {
      gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, (x2-x1));
      memset(dest_row, 0, x*bytes);
      gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, (x2-x1));
    }

  /* blur the rows */
  for (row = 0; row < y; row++)
    {
      gimp_pixel_rgn_get_row(&srcPR, cur_row, x1, y1+row, x);
      gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, x);
      blur_line(ctable, cmatrix, cmatrix_length, cur_row, dest_row, x, bytes);
      gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, x);

      if (row%5 == 0)
	gimp_progress_update((gdouble)row/(3*y));
    }

  /* allocate column buffers */
  cur_col  = g_new (guchar, y * bytes);
  dest_col = g_new (guchar, y * bytes);

  /* blur the cols */
  for (col = 0; col < x; col++)
    {
      gimp_pixel_rgn_get_col(&destPR, cur_col, x1+col, y1, y);
      gimp_pixel_rgn_get_col(&destPR, dest_col, x1+col, y1, y);
      blur_line(ctable, cmatrix, cmatrix_length, cur_col, dest_col, y, bytes);
      gimp_pixel_rgn_set_col(&destPR, dest_col, x1+col, y1, y);

      if (col%5 == 0)
	gimp_progress_update((gdouble)col/(3*x) + 0.33);
    }

  gimp_progress_init(_("Merging..."));

  /* find integer value of threshold */
  threshold = unsharp_params.threshold;
	
  /* merge the source and destination (which currently contains
     the blurred version) images */
  for (row = 0; row < y; row++)
    {
      value = 0;
      /* get source row */
      gimp_pixel_rgn_get_row(&srcPR, cur_row, x1, y1+row, x);
      /* get dest row */
      gimp_pixel_rgn_get_row(&destPR, dest_row, x1, y1+row, x);
      /* combine the two */
      for (u = 0; u < x; u++)
	{
	  for (v = 0; v < bytes; v++)
	    {
	      diff = (cur_row[u*bytes+v] - dest_row[u*bytes+v]);
	      /* do tresholding */
	      if (abs (2 * diff) < threshold)
		diff = 0;

	      value = cur_row[u*bytes+v] + amount * diff;
				
	      if (value < 0) dest_row[u*bytes+v] =0;
	      else if (value > 255) dest_row[u*bytes+v] = 255;
	      else  dest_row[u*bytes+v] = value;
	    }
	}
      /* update progress bar every five rows */
      if (row%5 == 0) gimp_progress_update((gdouble)row/(3*y) + 0.67);
      gimp_pixel_rgn_set_row(&destPR, dest_row, x1, y1+row, x);
    }

  /* free the memory we took */
  g_free(cur_row);
  g_free(dest_row);
  g_free(cur_col);
  g_free(dest_col);
  g_free(cmatrix);
  g_free(ctable);

}

#define ROUND(x) ((int) ((x) + 0.5))

/* this function is written as if it is blurring a column at a time,
   even though it can operate on rows, too.  There is no difference
   in the processing of the lines, at least to the blur_line function. */
static inline void
blur_line (gdouble *ctable,
	   gdouble *cmatrix,
	   gint     cmatrix_length,
	   guchar  *cur_col,
	   guchar  *dest_col,
	   gint     y,
	   glong    bytes)
{
#ifdef READABLE_CODE
/* ------------- semi-readable code ------------------- */
  gdouble scale;
  gdouble sum;
  gint i,j;
  gint row;

  /* this is to take care cases in which the matrix can go over
   * both edges at once.  It's not efficient, but this can only
   * happen in small pictures anyway.
   */
  if (cmatrix_length > y)
    {
      for (row = 0; row < y ; row++)
	{
	  scale = 0;
	  /* find the scale factor */
	  for (j = 0; j < y ; j++)
	    {
	      /* if the index is in bounds, add it to the scale counter */
	      if ((j + cmatrix_length/2 - row >= 0) &&
		  (j + cmatrix_length/2 - row < cmatrix_length))
		scale += cmatrix[j + cmatrix_length/2 - row];
	    }
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = 0; j < y; j++)
		{
		  if ((j >= row - cmatrix_length/2) &&
		      (j <= row + cmatrix_length/2))
		    sum += cur_col[j*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
    }
  else
    {  /* when the cmatrix is smaller than row length */
      /* for the edge condition, we only use available info, and scale to one */
      for (row = 0; row < cmatrix_length/2; row++)
	{
	  /* find scale factor */
	  scale=0;
	  for (j = cmatrix_length/2 - row; j<cmatrix_length; j++)
	    scale += cmatrix[j];

	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = cmatrix_length/2 - row; j<cmatrix_length; j++)
		{
		  sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] *
		    cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
      /* go through each pixel in each col */
      for (; row < y-cmatrix_length/2; row++)
	{
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = 0; j<cmatrix_length; j++)
		{
		  sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum);
	    }
	}
      /* for the edge condition , we only use available info, and scale to one */
      for (; row < y; row++)
	{
	  /* find scale factor */
	  scale=0;
	  for (j = 0; j< y-row + cmatrix_length/2; j++)
	    scale += cmatrix[j];

	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = 0; j<y-row + cmatrix_length/2; j++)
		{
		  sum += cur_col[(row + j-cmatrix_length/2)*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
    }
#endif
#ifndef READABLE_CODE
  /* --------------- optimized, unreadable code -------------------*/
  gdouble scale;
  gdouble sum;
  gint i=0, j=0;
  gint row;
  gint cmatrix_middle = cmatrix_length/2;

  gdouble *cmatrix_p;
  guchar  *cur_col_p;
  guchar  *cur_col_p1;
  guchar  *dest_col_p;
  gdouble *ctable_p;

  /* this first block is the same as the non-optimized version --
   * it is only used for very small pictures, so speed isn't a
   * big concern.
   */
  if (cmatrix_length > y)
    {
      for (row = 0; row < y ; row++)
	{
	  scale=0;
	  /* find the scale factor */
	  for (j = 0; j < y ; j++)
	    {
	      /* if the index is in bounds, add it to the scale counter */
	      if ((j + cmatrix_length/2 - row >= 0) &&
		  (j + cmatrix_length/2 - row < cmatrix_length))
		scale += cmatrix[j + cmatrix_length/2 - row];
	    }
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = 0; j < y; j++)
		{
		  if ((j >= row - cmatrix_length/2) &&
		      (j <= row + cmatrix_length/2))
		    sum += cur_col[j*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
    }
  else
    {
      /* for the edge condition, we only use available info and scale to one */
      for (row = 0; row < cmatrix_middle; row++)
	{
	  /* find scale factor */
	  scale=0;
	  for (j = cmatrix_middle - row; j<cmatrix_length; j++)
	    scale += cmatrix[j];
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = cmatrix_middle - row; j<cmatrix_length; j++)
		{
		  sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
      /* go through each pixel in each col */
      dest_col_p = dest_col + row*bytes;
      for (; row < y-cmatrix_middle; row++)
	{
	  cur_col_p = (row - cmatrix_middle) * bytes + cur_col;
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      cmatrix_p = cmatrix;
	      cur_col_p1 = cur_col_p;
	      ctable_p = ctable;
	      for (j = cmatrix_length; j>0; j--)
		{
		  sum += *(ctable_p + *cur_col_p1);
		  cur_col_p1 += bytes;
		  ctable_p += 256;
		}
	      cur_col_p++;
	      *(dest_col_p++) = ROUND (sum);
	    }
	}
	
      /* for the edge condition , we only use available info, and scale to one */
      for (; row < y; row++)
	{
	  /* find scale factor */
	  scale=0;
	  for (j = 0; j< y-row + cmatrix_middle; j++)
	    scale += cmatrix[j];
	  for (i = 0; i<bytes; i++)
	    {
	      sum = 0;
	      for (j = 0; j<y-row + cmatrix_middle; j++)
		{
		  sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
		}
	      dest_col[row*bytes + i] = (guchar) ROUND (sum / scale);
	    }
	}
    }
#endif
}

/* generates a 1-D convolution matrix to be used for each pass of 
 * a two-pass gaussian blur.  Returns the length of the matrix.
 */
static gint
gen_convolve_matrix (gdouble   radius,
		     gdouble **cmatrix_p)
{
  gint matrix_length;
  gint matrix_midpoint;
  gdouble* cmatrix;
  gint i,j;
  gdouble std_dev;
  gdouble sum;
	
  /* we want to generate a matrix that goes out a certain radius
   * from the center, so we have to go out ceil(rad-0.5) pixels,
   * inlcuding the center pixel.  Of course, that's only in one direction,
   * so we have to go the same amount in the other direction, but not count
   * the center pixel again.  So we double the previous result and subtract
   * one.
   * The radius parameter that is passed to this function is used as
   * the standard deviation, and the radius of effect is the
   * standard deviation * 2.  It's a little confusing.
   */
  radius = fabs(radius) + 1.0;
	
  std_dev = radius;
  radius = std_dev * 2;

  /* go out 'radius' in each direction */
  matrix_length = 2 * ceil(radius-0.5) + 1;
  if (matrix_length <= 0) matrix_length = 1;
  matrix_midpoint = matrix_length/2 + 1;
  *cmatrix_p = g_new (gdouble, matrix_length);
  cmatrix = *cmatrix_p;

  /*  Now we fill the matrix by doing a numeric integration approximation
   * from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
   * We do the bottom half, mirror it to the top half, then compute the
   * center point.  Otherwise asymmetric quantization errors will occur.
   *  The formula to integrate is e^-(x^2/2s^2).
   */

  /* first we do the top (right) half of matrix */
  for (i = matrix_length/2 + 1; i < matrix_length; i++)
    {
      double base_x = i - floor(matrix_length/2) - 0.5;
      sum = 0;
      for (j = 1; j <= 50; j++)
	{
	  if ( base_x+0.02*j <= radius ) 
	    sum += exp (-(base_x+0.02*j)*(base_x+0.02*j) / 
			(2*std_dev*std_dev));
	}
      cmatrix[i] = sum/50;
    }

  /* mirror the thing to the bottom half */
  for (i=0; i<=matrix_length/2; i++) {
    cmatrix[i] = cmatrix[matrix_length-1-i];
  }
	
  /* find center val -- calculate an odd number of quanta to make it symmetric,
   * even if the center point is weighted slightly higher than others. */
  sum = 0;
  for (j=0; j<=50; j++)
    {
      sum += exp (-(0.5+0.02*j)*(0.5+0.02*j) /
		  (2*std_dev*std_dev));
    }
  cmatrix[matrix_length/2] = sum/51;
	
  /* normalize the distribution by scaling the total sum to one */
  sum=0;
  for (i=0; i<matrix_length; i++) sum += cmatrix[i];
  for (i=0; i<matrix_length; i++) cmatrix[i] = cmatrix[i] / sum;

  return matrix_length;
}

/* ----------------------- gen_lookup_table ----------------------- */
/* generates a lookup table for every possible product of 0-255 and
   each value in the convolution matrix.  The returned array is
   indexed first by matrix position, then by input multiplicand (?)
   value.
*/
static gdouble *
gen_lookup_table (gdouble *cmatrix,
		  gint     cmatrix_length)
{
  int i, j;
  gdouble* lookup_table = g_new (gdouble, cmatrix_length * 256);

#ifdef READABLE_CODE
  for (i=0; i<cmatrix_length; i++)
    {
      for (j=0; j<256; j++)
	{
	  lookup_table[i*256 + j] = cmatrix[i] * (gdouble)j;
	}
    }
#endif

#ifndef READABLE_CODE
  gdouble* lookup_table_p = lookup_table;
  gdouble* cmatrix_p      = cmatrix;

  for (i=0; i<cmatrix_length; i++)
    {
      for (j=0; j<256; j++)
	{
	  *(lookup_table_p++) = *cmatrix_p * (gdouble)j;
	}
      cmatrix_p++;
    }
#endif

  return lookup_table;
}

#if 0

/**
 * gimp_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             gtk_init(). It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name or something.
 * @preview: #TRUE if the plug-in has some kind of preview in it's UI.
 *           Note that passing #TRUE is recommended also if one of the
 *           used GIMP Library widgets contains a preview (like the image
 *           menu returned by gimp_image_menu_new()).
 *
 * This function initializes GTK+ with gtk_init(), instructs GDK not to
 * use X shared memory if The GIMP was invoked with the --no-xshm command
 * line option and initializes GDK's image rendering subsystem (GdkRGB) to
 * follow the GIMP main program's colormap allocation/installation policy.
 *
 * The GIMP's colormap policy can be determinded by the user with the
 * gimprc variables @min_colors and @install_cmap.
 **/
void
gimp_ui_init (const gchar *prog_name,
	      gboolean     preview)
{
  gint    argc;
  gchar **argv;

  static gboolean initialized = FALSE;

  g_return_if_fail (prog_name != NULL);

  if (initialized)
    return;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (prog_name);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /*  It's only safe to switch Gdk SHM usage off  */
  if (! gimp_use_xshm ())
    gdk_set_use_xshm (FALSE);

  /*
  gdk_rgb_set_min_colors (gimp_min_colors ());
  gdk_rgb_set_install (gimp_install_cmap ());
  gtk_widget_set_default_visual (gdk_rgb_get_visual ());
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());*/ //FIXME Not sure what this is for

  /*  Set the gamma after installing the colormap because
   *  gtk_preview_set_gamma() initializes GdkRGB if not already done
   */
  if (preview)
    gtk_preview_set_gamma (gimp_gamma ());

  initialized = TRUE;
}
#endif

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
	gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, "Gimp");
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
			//help_func, //FIXME
			//help_data, //FIXME
			position,
			allow_shrink,
			allow_grow,
			auto_shrink,
			args);

	va_end (args);

	return dialog;
}
#endif

static void
gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
						    GtkAdjustment *other_adj)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (other_adj), adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (other_adj), adjustment);
}

/**
 * gimp_spin_button_new:
 * @adjustment:     Returns the spinbutton's #GtkAdjustment.
 * @value:          The initial value of the spinbutton.
 * @lower:          The lower boundary.
 * @upper:          The uppper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size:      The spinbutton's page size.
 * @climb_rate:     The spinbutton's climb rate.
 * @digits:         The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a subsequent
 * gtk_spin_button_new() and does some more initialisation stuff like
 * setting a standard minimun horizontal size.
 *
 * Returns: A #GtkSpinbutton and it's #GtkAdjustment.
 **/
/*GtkWidget *
gimp_spin_button_new (GtkObject **adjustment,
		      gfloat      value,
		      gfloat      lower,
		      gfloat      upper,
		      gfloat      step_increment,
		      gfloat      page_increment,
		      gfloat      page_size,
		      gfloat      climb_rate,
		      guint       digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
				    step_increment, page_increment, page_size);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
				    climb_rate, digits);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, -1);

  return spinbutton;
}*/


/**
 * gimp_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_usize:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_usize:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @constrain:           #TRUE if the range of possible values of the
 *                       #GtkSpinButton should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == #FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == #FALSE.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_data:           The widgets' help_data (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Note that if you pass a @tooltip or @help_data to this function you'll
 * have to initialize GIMP's help system with gimp_help_init() before using it.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
/*GtkObject *
gimp_scale_entry_new (GtkTable    *table,
		      gint         column,
		      gint         row,
		      const gchar *text,
		      gint         scale_usize,
		      gint         spinbutton_usize,
		      gfloat       value,
		      gfloat       lower,
		      gfloat       upper,
		      gfloat       step_increment,
		      gfloat       page_increment,
		      guint        digits,
		      gboolean     constrain,
		      gfloat       unconstrained_lower,
		      gfloat       unconstrained_upper,
		      const gchar *tooltip,
		      const gchar *help_data)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  if (! constrain &&
      unconstrained_lower <= lower &&
      unconstrained_upper >= upper)
    {
      GtkObject *constrained_adj;

      constrained_adj = gtk_adjustment_new (value, lower, upper,
					    step_increment, page_increment,
					    0.0);

      spinbutton = gimp_spin_button_new (&adjustment, value,
					 unconstrained_lower,
					 unconstrained_upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      gtk_signal_connect
	(GTK_OBJECT (constrained_adj), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 adjustment);

      gtk_signal_connect
	(GTK_OBJECT (adjustment), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 constrained_adj);

      return_adj = adjustment;

      adjustment = constrained_adj;
    }
  else
    {
      spinbutton = gimp_spin_button_new (&adjustment, value, lower, upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      return_adj = adjustment;
    }
    
  if (spinbutton_usize > 0)
    gtk_widget_set_usize (spinbutton, spinbutton_usize, -1);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  if (scale_usize > 0)
    gtk_widget_set_usize (scale, scale_usize, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
		    column + 1, column + 2, row, row + 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
		    column + 2, column + 3, row, row + 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip || help_data)
    {// FIXME
      //gimp_help_set_help_data (scale, tooltip, help_data);
      //gimp_help_set_help_data (spinbutton, tooltip, help_data);
    }

  gtk_object_set_data (GTK_OBJECT (return_adj), "label", label);
  gtk_object_set_data (GTK_OBJECT (return_adj), "scale", scale);
  gtk_object_set_data (GTK_OBJECT (return_adj), "spinbutton", spinbutton);

  return return_adj;
}*/

/**
 * gimp_double_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gdouble variable which will store the
 *              @adjustment's value.
 **/
/*void
gimp_double_adjustment_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  gdouble *val;

  val = (gdouble *) data;
  *val = adjustment->value;
}*/

#ifdef GIMP_VERSION
#define RINT(x) rint(x)
#endif

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
  *val = RINT (adjustment->value);
}*/


/* ------------------------ unsharp_mask_dialog ----------------------- */
static gint
unsharp_mask_dialog (void) 
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *adj;

  gimp_ui_init ("unsharp", TRUE);

  window = gimp_dialog_new (_("Unsharp Mask"), "unsharp",
			    gimp_standard_help_func, "filters/unsharp.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), unsharp_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), frame,
		      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Radius:"), SCALE_WIDTH, 0,
			      unsharp_params.radius, 1.0, 150.0, 0.1, 1.0, 1,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &unsharp_params.radius);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Amount:"), SCALE_WIDTH, 0,
			      unsharp_params.amount, 0.0, 5.0, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &unsharp_params.amount);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Threshold:"), SCALE_WIDTH, 0,
			      unsharp_params.threshold, 0.0, 255.0, 1.0, 10.0, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &unsharp_params.threshold);

  gtk_widget_show (window);

  gtk_main ();
  gdk_flush ();

  return run_filter;
}


/* This doesn't work yet. */
#ifdef PREVIEW
/* 'preview_init()' - Initialize the preview window... */

static void
preview_init (void)
{
  int	width;		/* Byte width of the image */

  /*
   * Setup for preview filter...
   */

  width = preview_width * img_bpp;

  preview_src   = g_new (guchar, width * preview_height);
  preview_neg   = g_new (guchar, width * preview_height);
  preview_dst   = g_new (guchar, width * preview_height);
  preview_image = g_new (guchar, preview_width * preview_height * 3);

  preview_x1 = sel_x1;
  preview_y1 = sel_y1;
  preview_x2 = preview_x1 + preview_width;
  preview_y2 = preview_y1 + preview_height;
}

/*
 * 'preview_scroll_callback()' - Update the preview when a scrollbar is moved.
 */

static void
preview_scroll_callback (void)
{
  preview_x1 = sel_x1 + GTK_ADJUSTMENT (hscroll_data)->value;
  preview_y1 = sel_y1 + GTK_ADJUSTMENT (vscroll_data)->value;
  preview_x2 = preview_x1 + MIN(preview_width, sel_width);
  preview_y2 = preview_y1 + MIN(preview_height, sel_height);

  preview_update ();
}

/*
 * 'preview_update()' - Update the preview window.
 */

static void
preview_update (void)
{

}
#endif

static void
unsharp_ok_callback (GtkWidget *widget,
		     gpointer   data)
{
  run_filter = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

#ifdef TIMER
static void
timerstart (void)
{
  gettimeofday (&time_start, NULL);
}

static void
timerstop (void)
{
  long sec, usec;
	
  gettimeofday(&time_stop,NULL);
  sec = time_stop.tv_sec  - time_start.tv_sec;
  usec = time_stop.tv_usec - time_start.tv_usec;
	
  if (usec < 0)
    {
      sec--;
      usec += 1000000;
    }
	
  fprintf (stderr, "%ld.%ld seconds\n", sec, usec);
}
#endif
