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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../drawable.h"
#include "../equalize.h"
#include "float16.h"
#include "../interface.h"
#include "../gimage.h"
#include "../paint_funcs_area.h"
#include "../pixelarea.h"

static void       equalize (GImage *, CanvasDrawable *, int);
static void       eq_histogram (double [3][256], unsigned char [3][256], int, double);
static Argument * equalize_invoker (Argument *);


void
image_equalize (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  CanvasDrawable *drawable;
  int mask_only = TRUE;
  

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);

  if (drawable_indexed (drawable))
    {
      g_message (_("Equalize does not operate on indexed drawables."));
      return;
    }
  equalize (gimage, drawable, mask_only);
}

static void
equalize(gimage, drawable, mask_only)
     GImage *gimage;
     CanvasDrawable *drawable;
     int mask_only;
{
  Channel *sel_mask;
  PixelArea srcPR, destPR, maskPR, *sel_maskPR;
  double hist[3][256];
  unsigned char lut[3][256];
  guchar *src, *dest, *mask;
  int no_mask;
  int w, h, j, b;
  int has_alpha;
  int alpha;
  int off_x, off_y;
  int x1, y1, x2, y2;
  double count;
  void *pr;
  Tag tag = drawable_tag (drawable);
  gint src_num_channels = tag_num_channels (tag);
  gint dest_num_channels = tag_num_channels (tag);
  gint src_rowstride = 0;
  gint mask_rowstride = 0;
  gint dest_rowstride = 0;
  mask = NULL;

  sel_mask = gimage_get_mask (gimage);
  drawable_offsets (drawable, &off_x, &off_y);
  has_alpha = drawable_has_alpha (drawable);
  alpha = has_alpha ? (src_num_channels - 1) : src_num_channels;
  count = 0.0;

  /*  Determine the histogram from the drawable data and the attendant mask  */
  no_mask = (drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2) == FALSE);

  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1,
                  (x2 - x1), (y2 - y1),
                  FALSE);

  sel_maskPR = (no_mask) ? NULL : &maskPR;

  if (sel_maskPR)
    pixelarea_init (sel_maskPR, drawable_data (GIMP_DRAWABLE (sel_mask)),
                    x1 + off_x, y1 + off_y,
                    (x2 - x1), (y2 - y1),
                    FALSE);

  /* Initialize histogram */
  for (b = 0; b < alpha; b++)
    for (j = 0; j < 256; j++)
      hist[b][j] = 0.0;

  for (pr = pixelarea_register (2, &srcPR, sel_maskPR);
       pr != NULL;
       pr = pixelarea_process (pr))
    {
      src = pixelarea_data (&srcPR);
      src_rowstride = pixelarea_rowstride (&srcPR);
      if (sel_maskPR)
	{
	  mask = pixelarea_data (sel_maskPR);
	  mask_rowstride = pixelarea_rowstride (&maskPR);
	}
      h = pixelarea_height (&srcPR);

      while (h--)
	{
	  w = pixelarea_width (&srcPR);

	  switch (tag_precision (tag))
	    { 
	    case PRECISION_U8:
	      {
		guint8 *s = (guint8*)src;
		guint8* m = (guint8*)mask;
		for (j = 0; j < w; j++)
		  {
		    if (sel_maskPR)
		      {
			for (b = 0; b < alpha; b++)
			  hist[b][s[b]] += (double) *m / 255.0;
			count += (double) *m / 255.0;
		      }
		    else
		      {
			for (b = 0; b < alpha; b++)
			  hist[b][s[b]] += 1.0;
			count += 1.0;
		      }
		    s += src_num_channels; 
		    if (sel_maskPR)
		      m ++;
		  }
	      }
	      break;
	    case PRECISION_BFP:
	    case PRECISION_U16:
	      {
		gint value_bin;
		guint16 *s = (guint16*)src;
		guint16* m = (guint16*)mask;
		for (j = 0; j < w; j++)
		  {
		    if (sel_maskPR)
		      {
			for (b = 0; b < alpha; b++)
			  {
		            value_bin = s[b]/256;
			    hist[b][value_bin] += (double) *m / 65535.0;
			  }
			count += (double) *m / 65535.0;
		      }
		    else
		      {
			for (b = 0; b < alpha; b++)
			  {
			    value_bin = s[b]/256; 
			    hist[b][value_bin] += 1.0;
			  }
			count += 1.0;
		      }
		    s += src_num_channels; 
		    if (sel_maskPR)
		      m ++;
		  }
	      }
	      break;
	    case PRECISION_FLOAT:
	      {
		gint value_bin;
		gfloat *s = (gfloat*)src;
		gfloat *m = (gfloat*)mask;
		for (j = 0; j < w; j++)
		  {
		    if (sel_maskPR)
		      {
			for (b = 0; b < alpha; b++)
			  {
		            value_bin = (int) (s[b] * 255); 
			    hist[b][value_bin] += (double) *m;
			  }
			count += (double) *m;
		      }
		    else
		      {
			for (b = 0; b < alpha; b++)
			  {
		            value_bin = (int) (s[b] * 255);
			    hist[b][value_bin] += 1.0;
			  }
			count += 1.0;
		      }
		    s += src_num_channels; 
		    if (sel_maskPR)
		      m ++;
		  }
	      }
	      break;
	    case PRECISION_FLOAT16:
	      {
		gint value_bin;
		ShortsFloat u;
		guint16 *s = (guint16*)src;
		guint16 *m = (guint16*)mask;
		for (j = 0; j < w; j++)
		  {
		    if (sel_maskPR)
		      {
			for (b = 0; b < alpha; b++)
			  {
		            value_bin = (int) (FLT(s[b], u) * 255); 
			    value_bin = BOUNDS (value_bin, 0, 255); 
			    hist[b][value_bin] += (double) *m;
			  }
			count += (double) *m;
		      }
		    else
		      {
			for (b = 0; b < alpha; b++)
			  {
			       value_bin = (int) (FLT (s[b], u) * 255);
			       value_bin = BOUNDS (value_bin, 0, 255); 
			       hist[b][value_bin] += 1.0;
			  }
			count += 1.0;
		      }
		    s += src_num_channels; 
		    if (sel_maskPR)
		      m ++;
		  }
	      }
	      break;
	     default:
	      return;
	    }	
	  src += src_rowstride;

	  if (sel_maskPR)
	    mask += mask_rowstride;
	}
    }

  /* Build equalization LUT */
  eq_histogram (hist, lut, alpha, count);

  /*  Apply the histogram  */
  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1,
                  (x2 - x1), (y2 - y1),
                  FALSE);

  pixelarea_init (&destPR, drawable_shadow (drawable),
                  x1, y1,
                  (x2 - x1), (y2 - y1),
                  TRUE);

  for (pr = pixelarea_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixelarea_process (pr))
    {
      src = pixelarea_data (&srcPR);
      dest = pixelarea_data (&destPR);
      src_rowstride = pixelarea_rowstride (&srcPR);
      dest_rowstride = pixelarea_rowstride (&destPR);
      h = pixelarea_height (&srcPR);

      while (h--)
	{

	  w = pixelarea_width (&srcPR);
	  switch (tag_precision (tag))
	    { 
	    case PRECISION_U8:
	      {
		guint8 *s = (guint8*)src;
		guint8* d = (guint8*)dest;
		for (j = 0; j < w; j++)
		  {
		    for (b = 0; b < alpha; b++)
		      d[b] = lut[b][s[b]];

		    if (has_alpha)
		      d[alpha] = s[alpha];

		    s += src_num_channels;
		    d += dest_num_channels;
		  }
	      }
	      break;
            case PRECISION_BFP:
	    case PRECISION_U16:
	      {
		gint value_bin;
		guint16 *s = (guint16*)src;
		guint16* d = (guint16*)dest;
		for (j = 0; j < w; j++)
		  {
		    for (b = 0; b < alpha; b++)
		      {
			value_bin = s[b]/256;
		        d[b] = 257 * lut[b][value_bin];
		      }

		    if (has_alpha)
		      d[alpha] = s[alpha];

		    s += src_num_channels;
		    d += dest_num_channels;
		  }
	      }
	      break;
	    case PRECISION_FLOAT:
	      {
		gint value_bin;
		gfloat *s = (gfloat*)src;
		gfloat *d = (gfloat*)dest;
		for (j = 0; j < w; j++)
		  {
		    for (b = 0; b < alpha; b++)
		      {
			value_bin = (gint)(s[b] * 255);
		        d[b] = lut[b][value_bin] / 255.0;
		      }

		    if (has_alpha)
		      d[alpha] = s[alpha];

		    s += src_num_channels;
		    d += dest_num_channels;
		  }
	      }
	      break;
	    case PRECISION_FLOAT16:
	      {
		ShortsFloat u;
		gint value_bin;
		guint16 *s = (guint16*)src;
		guint16 *d = (guint16*)dest;
		for (j = 0; j < w; j++)
		  {
		    for (b = 0; b < alpha; b++)
		      {
			value_bin = (gint)(FLT(s[b], u) * 255);
		        d[b] = FLT16 (lut[b][value_bin] / 255.0, u);
		      }

		    if (has_alpha)
		      d[alpha] = s[alpha];

		    s += src_num_channels;
		    d += dest_num_channels;
		  }
	      }
	      break;
	     default:
	      return;
	    }	

	  src += src_rowstride;
	  dest += dest_rowstride;
	}
    }

  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}

/*****/

static void
eq_histogram (hist, lut, bytes, count)
     double hist[3][256];
     unsigned char lut[3][256];
     int bytes;
     double count;
{
  int    i, k, j;
  int    part[3][257]; /* Partition */
  double pixels_per_value;
  double desired;

  /* Calculate partial sums */
  for (k = 0; k < bytes; k++)
    for (i = 1; i < 256; i++)
      hist[k][i] += hist[k][i - 1];

  /* Find partition points */
  pixels_per_value = count / 256.0;

  for (k = 0; k < bytes; k++)
    {
      /* First and last points in partition */
      part[k][0]   = 0;
      part[k][256] = 256;

      /* Find intermediate points */
      j = 0;
      for (i = 1; i < 256; i++)
	{
	  desired = i * pixels_per_value;
	  while (hist[k][j + 1] <= desired)
	    j++;

	  /* Nearest sum */
	  if ((desired - hist[k][j]) < (hist[k][j + 1] - desired))
	    part[k][i] = j;
	  else
	    part[k][i] = j + 1;
	}
    }

  /* Create equalization LUT */
  for (k = 0; k < bytes; k++)
    for (j = 0; j < 256; j++)
      {
	i = 0;
	while (part[k][i + 1] <= j)
	  i++;

	lut[k][j] = i;
      }
}


/*  The equalize procedure definition  */
ProcArg equalize_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The Drawable"
  },
  { PDB_INT32,
    "mask_only",
    "equalization option"
  }
};

ProcRecord equalize_proc =
{
  "gimp_equalize",
  "Equalize the contents of the specified drawable",
  "This procedure equalizes the contents of the specified drawable.  Each intensity channel is equalizeed independently.  The equalizeed intensity is given as inten' = (255 - inten).  Indexed color drawables are not valid for this operation.  The 'mask_only' option specifies whether to adjust only the area of the image within the selection bounds, or the entire image based on the histogram of the selected area.  If there is no selection, the entire image is adjusted based on the histogram for the entire image.",
  "Federico Mena Quintero & Spencer Kimball & Peter Mattis",
  "Federico Mena Quintero & Spencer Kimball & Peter Mattis",
  "1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  equalize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { equalize_invoker } },
};


static Argument *
equalize_invoker (args)
     Argument *args;
{
  int success = TRUE;
  int int_value;
  int mask_only;
  GImage *gimage;
  CanvasDrawable *drawable;

  drawable = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  the mask only option  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      mask_only = (int_value) ? TRUE : FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  if (success)
    equalize (gimage, drawable, mask_only);

  return procedural_db_return_args (&equalize_proc, success);
}
