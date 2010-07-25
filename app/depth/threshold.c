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
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../actionarea.h"
#include "../drawable.h"
#include "../general.h"
#include "../gdisplay.h"
#include "float16.h"
#include "bfp.h"
#include "../histogram.h"
#include "../image_map.h"
#include "../interface.h"
#include "../threshold.h"
#include "../pixelarea.h"
#include "../minimize.h"

#define TEXT_WIDTH 75 
#define HISTOGRAM_WIDTH 256
#define HISTOGRAM_HEIGHT 150

typedef struct Threshold Threshold;

struct Threshold
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct ThresholdDialog ThresholdDialog;

struct ThresholdDialog
{
  GtkWidget   *shell;
  GtkWidget   *low_threshold_text;
  GtkWidget   *high_threshold_text;
  Histogram   *histogram;

  CanvasDrawable *drawable;
  ImageMap     image_map;
  int          color;
  gfloat       low_threshold;
  gfloat       high_threshold;
  int	       bins;
  gint         preview;
};

/*  threshold action functions  */

static void   threshold_button_press   (Tool *, GdkEventButton *, gpointer);
static void   threshold_button_release (Tool *, GdkEventButton *, gpointer);
static void   threshold_motion         (Tool *, GdkEventMotion *, gpointer);
static void   threshold_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   threshold_control        (Tool *, int, gpointer);

static ThresholdDialog *  threshold_new_dialog                 (Tag);
static void               threshold_preview                    (ThresholdDialog *);
static void               threshold_ok_callback                (GtkWidget *, gpointer);
static void               threshold_cancel_callback            (GtkWidget *, gpointer);
static gint               threshold_delete_callback            (GtkWidget *, GdkEvent *, gpointer);
static void               threshold_preview_update             (GtkWidget *, gpointer);

typedef void (*ThresholdLowThresholdTextUpdateFunc)(GtkWidget *, gpointer);
static ThresholdLowThresholdTextUpdateFunc threshold_low_threshold_text_update;
static void               threshold_low_threshold_text_update_u8  (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update_u16  (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update_float  (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update_float16  (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update_bfp  (GtkWidget *, gpointer);

typedef void (*ThresholdHighThresholdTextUpdateFunc)(GtkWidget *, gpointer);
static ThresholdHighThresholdTextUpdateFunc threshold_high_threshold_text_update;
static void               threshold_high_threshold_text_update_u8 (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update_u16 (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update_float (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update_float16 (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update_bfp (GtkWidget *, gpointer);

static void *threshold_options = NULL;
static ThresholdDialog *threshold_dialog = NULL;
static void threshold_funcs (Tag tag);

typedef void (*ThresholdFunc)(PixelArea *, PixelArea *, void *);
static ThresholdFunc threshold;
static void       threshold_u8 (PixelArea *, PixelArea *, void *);
static void       threshold_u16 (PixelArea *, PixelArea *, void *);
static void       threshold_float (PixelArea *, PixelArea *, void *);
static void       threshold_float16 (PixelArea *, PixelArea *, void *);
static void       threshold_bfp (PixelArea *, PixelArea *, void *);

typedef void (*ThresholdHistogramInfoFunc)(PixelArea *, PixelArea *, HistogramValues, void *);
static ThresholdHistogramInfoFunc threshold_histogram_info;
static void       threshold_histogram_info_u8 (PixelArea *, PixelArea *, HistogramValues, void *);
static void       threshold_histogram_info_u16 (PixelArea *, PixelArea *, HistogramValues, void *);
static void       threshold_histogram_info_float (PixelArea *, PixelArea *, HistogramValues, void *);
static void       threshold_histogram_info_float16 (PixelArea *, PixelArea *, HistogramValues, void *);
static void       threshold_histogram_info_bfp (PixelArea *, PixelArea *, HistogramValues, void *);

typedef void (*ThresholdHistogramRangeFunc)(int, int, int, HistogramValues, void *);
static ThresholdHistogramRangeFunc threshold_histogram_range;
static void       threshold_histogram_range_u8 (int, int, int, HistogramValues, void *);
static void       threshold_histogram_range_u16 (int, int, int, HistogramValues, void *);
static void       threshold_histogram_range_float (int, int, int, HistogramValues, void *);
static void       threshold_histogram_range_float16 (int, int, int, HistogramValues, void *);
static void       threshold_histogram_range_bfp (int, int, int, HistogramValues, void *);
static Argument * threshold_invoker (Argument *);

/*  threshold machinery  */

static void
threshold_funcs (Tag tag)
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
     threshold = threshold_u8;
     threshold_histogram_info = threshold_histogram_info_u8;
     threshold_histogram_range = threshold_histogram_range_u8;
     threshold_high_threshold_text_update = threshold_high_threshold_text_update_u8;
     threshold_low_threshold_text_update = threshold_low_threshold_text_update_u8;
     break;
  case PRECISION_U16:
     threshold = threshold_u16;
     threshold_histogram_info = threshold_histogram_info_u16;
     threshold_histogram_range = threshold_histogram_range_u16;
     threshold_high_threshold_text_update = threshold_high_threshold_text_update_u16;
     threshold_low_threshold_text_update = threshold_low_threshold_text_update_u16;
     break;
  case PRECISION_FLOAT:
     threshold = threshold_float;
     threshold_histogram_info = threshold_histogram_info_float;
     threshold_histogram_range = threshold_histogram_range_float;
     threshold_high_threshold_text_update = threshold_high_threshold_text_update_float;
     threshold_low_threshold_text_update = threshold_low_threshold_text_update_float;
     break;
  case PRECISION_FLOAT16:
     threshold = threshold_float16;
     threshold_histogram_info = threshold_histogram_info_float16;
     threshold_histogram_range = threshold_histogram_range_float16;
     threshold_high_threshold_text_update = threshold_high_threshold_text_update_float16;
     threshold_low_threshold_text_update = threshold_low_threshold_text_update_float16;
     break;
  case PRECISION_BFP:
     threshold = threshold_bfp;
     threshold_histogram_info = threshold_histogram_info_bfp;
     threshold_histogram_range = threshold_histogram_range_bfp;
     threshold_high_threshold_text_update = threshold_high_threshold_text_update_bfp;
     threshold_low_threshold_text_update = threshold_low_threshold_text_update_bfp;
     break;
  default:
     threshold = NULL;
     threshold_histogram_info = NULL;
     threshold_histogram_range = NULL;
     threshold_high_threshold_text_update = NULL;
     threshold_low_threshold_text_update = NULL;
    break;
  } 
}

static void
threshold_u8 ( PixelArea *src_area,
	       PixelArea *dest_area,
	       void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guchar	   *src, *dest;
  guint8           *s, *d;
  gint             value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint8 *)src;
       d = (guint8 *)dest;
       while (w--)
	 {
	   if (td->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       value = (value >= td->low_threshold && value <= td->high_threshold ) ? 255 : 0;
	     }
	   else
	     value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 255 : 0;

	   for (b = 0; b < alpha; b++)
	     d[b] = value;

	   if (has_alpha)
	     d[alpha] = s[alpha];

	   s += s_num_channels;
	   d += d_num_channels;
	 }

	 src += pixelarea_rowstride (src_area);
	 dest += pixelarea_rowstride (dest_area);
    }
}

static void
threshold_u16 ( PixelArea *src_area,
	       PixelArea *dest_area,
	       void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guchar	   *src, *dest;
  guint16          *s, *d;
  gint             value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16 *)src;
       d = (guint16 *)dest;
       while (w--)
	 {
	   if (td->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       value = (value >= td->low_threshold && value <= td->high_threshold ) ? 65535: 0;
	     }
	   else
	     value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 65535 : 0;

	   for (b = 0; b < alpha; b++)
	     d[b] = value;

	   if (has_alpha)
	     d[alpha] = s[alpha];

	   s += s_num_channels;
	   d += d_num_channels;
	 }

	 src += pixelarea_rowstride (src_area);
	 dest += pixelarea_rowstride (dest_area);
    }
}

static void
threshold_float ( PixelArea *src_area,
	       PixelArea *dest_area,
	       void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guchar	   *src, *dest;
  gfloat           *s, *d;
  gfloat           value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (gfloat *)src;
       d = (gfloat *)dest;
       while (w--)
	 {
	   if (td->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       value = (value >= td->low_threshold && value <= td->high_threshold ) ? 1.0: 0.0;
	     }
	   else
	     value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 1.0 : 0.0;

	   for (b = 0; b < alpha; b++)
	     d[b] = value;

	   if (has_alpha)
	     d[alpha] = s[alpha];

	   s += s_num_channels;
	   d += d_num_channels;
	 }

	 src += pixelarea_rowstride (src_area);
	 dest += pixelarea_rowstride (dest_area);
    }
}

static void
threshold_float16 ( PixelArea *src_area,
	       PixelArea *dest_area,
	       void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guchar	   *src, *dest;
  guint16           *s, *d;
  gfloat           red, green, blue, value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);
  ShortsFloat      u;

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16 *)src;
       d = (guint16 *)dest;
       while (w--)
	 {
	   if (td->color)
	     {
	       red = FLT (s[RED_PIX], u);
	       green = FLT (s[GREEN_PIX], u);
	       blue = FLT (s[BLUE_PIX], u);
	       value = MAX (red, green);
	       value = MAX (value, blue);
	       value = (value >= td->low_threshold && value <= td->high_threshold ) ? 1.0: 0.0;
	     }
	   else
	     value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 1.0 : 0.0;

	   for (b = 0; b < alpha; b++)
	     d[b] = FLT16 (value, u);

	   if (has_alpha)
	     d[alpha] = s[alpha];

	   s += s_num_channels;
	   d += d_num_channels;
	 }

	 src += pixelarea_rowstride (src_area);
	 dest += pixelarea_rowstride (dest_area);
    }
}

static void
threshold_bfp ( PixelArea *src_area,
	       PixelArea *dest_area,
	       void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guchar	   *src, *dest;
  guint16          *s, *d;
  gint             value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16 *)src;
       d = (guint16 *)dest;
       while (w--)
	 {
	   if (td->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       value = (value >= td->low_threshold && value <= td->high_threshold ) ? ONE_BFP: 0;
	     }
	   else
	     value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? ONE_BFP : 0;

	   for (b = 0; b < alpha; b++)
	     d[b] = value;

	   if (has_alpha)
	     d[alpha] = s[alpha];

	   s += s_num_channels;
	   d += d_num_channels;
	 }

	 src += pixelarea_rowstride (src_area);
	 dest += pixelarea_rowstride (dest_area);
    }
}


static void
threshold_histogram_info_u8 (PixelArea       *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  gint              value;
  guint8	  *src, *mask = NULL;
  guint8 *s, *m = NULL;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guint8*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guint8*)pixelarea_data (mask_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = src;

       if (mask_area)
	 m = mask;

       while (w--)
	 {
	   if (td->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	     }
	   else
	     value = s[GRAY_PIX];

	   if (mask_area)
	     values[HISTOGRAM_VALUE][value] += (double) *m /255.0;
	   else
	     values[HISTOGRAM_VALUE][value] += 1.0;

	   s += s_num_channels;

	   if (mask_area)
	     m += m_num_channels;
	 }

       src += pixelarea_rowstride (src_area);
 
       if (mask_area)
	 mask += pixelarea_rowstride (mask_area);
    }
}

static void
threshold_histogram_info_u16 (PixelArea   *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  guchar	  *src, *mask = NULL;
  guint16         *s, *m = NULL;
  guint16          red, green, blue;
  gint             value_bin;
  gint             value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);
  gint 		   bins = 256; 

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guchar*)pixelarea_data (mask_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16*)src;

       if (mask_area)
	 m = (guint16*)mask;

       while (w--)
	 {
	   if (td->color)
	     {
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
	       value = MAX (red, green);
	       value = MAX (value, blue);
	       value_bin = value/bins;
	     }
	   else
	     value_bin = s[GRAY_PIX]/bins;

	   if (mask_area)
	     values[HISTOGRAM_VALUE][value_bin] += (double) *m / 65535.0;
	   else
	     values[HISTOGRAM_VALUE][value_bin] += 1.0;

	   s += s_num_channels;

	   if (mask_area)
	     m += m_num_channels;
	 }

       src += pixelarea_rowstride (src_area);
 
       if (mask_area)
	 mask += pixelarea_rowstride (mask_area);
    }
}

static void
threshold_histogram_info_float (PixelArea   *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  guchar	  *src, *mask = NULL;
  gfloat          *s, *m = NULL;
  gfloat           red, green, blue, gray;
  gint             value_bin;
  gfloat           value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);
  gint 		   bins = 256; 

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guchar*)pixelarea_data (mask_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (gfloat*)src;

       if (mask_area)
	 m = (gfloat*)mask;

       while (w--)
	 {
	   if (td->color)
	     {
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
		
	       red = CLAMP (red, 0.0, 1.0);
	       green = CLAMP (green, 0.0, 1.0);
	       blue = CLAMP (blue, 0.0, 1.0);

	       value = MAX (red, green);
	       value = MAX (value, blue);
	       value_bin = (int)(value * (bins-1));
	     }
	   else
	     {
	       gray = s[GRAY_PIX];
	       gray = CLAMP (gray, 0.0, 1.0);
	       value_bin = (int)(gray * (bins-1));
	     }

	   if (mask_area)
	     values[HISTOGRAM_VALUE][value_bin] += (double) *m;
	   else
	     values[HISTOGRAM_VALUE][value_bin] += 1.0;

	   s += s_num_channels;

	   if (mask_area)
	     m += m_num_channels;
	 }

       src += pixelarea_rowstride (src_area);
 
       if (mask_area)
	 mask += pixelarea_rowstride (mask_area);
    }
}

static void
threshold_histogram_info_float16 (PixelArea   *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  guchar	  *src, *mask = NULL;
  guint16          *s, *m = NULL;
  gfloat           red, green, blue, gray;
  gint             value_bin;
  gfloat           value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);
  gint 		   bins = 256; 
  ShortsFloat      u;

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guchar*)pixelarea_data (mask_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16*)src;

       if (mask_area)
	 m = (guint16*)mask;

       while (w--)
	 {
	   if (td->color)
	     {
	       red   = FLT (s[RED_PIX], u);
	       green = FLT (s[GREEN_PIX], u);
	       blue  = FLT (s[BLUE_PIX], u);
		
	       red = CLAMP (red, 0.0, 1.0);
	       green = CLAMP (green, 0.0, 1.0);
	       blue = CLAMP (blue, 0.0, 1.0);

	       value = MAX (red, green);
	       value = MAX (value, blue);
	       value_bin = (int)(value * (bins-1));
	     }
	   else
	     {
	       gray = FLT (s[GRAY_PIX], u);
	       gray = CLAMP (gray, 0.0, 1.0);
	       value_bin = (int)(gray * (bins-1));
	     }

	   if (mask_area)
	     values[HISTOGRAM_VALUE][value_bin] += (double) *m;
	   else
	     values[HISTOGRAM_VALUE][value_bin] += 1.0;

	   s += s_num_channels;

	   if (mask_area)
	     m += m_num_channels;
	 }

       src += pixelarea_rowstride (src_area);
 
       if (mask_area)
	 mask += pixelarea_rowstride (mask_area);
    }
}

static void
threshold_histogram_info_bfp (PixelArea   *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  guchar	  *src, *mask = NULL;
  guint16         *s, *m = NULL;
  guint16          red, green, blue;
  gint             value_bin;
  gint             value;
  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);
  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);
  gint 		   bins = 256; 

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guchar*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guchar*)pixelarea_data (mask_area);

  while (h--)
    {
       w = pixelarea_width (src_area);
       s = (guint16*)src;

       if (mask_area)
	 m = (guint16*)mask;

       while (w--)
	 {
	   if (td->color)
	     {
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
	       value = MAX (red, green);
	       value = MAX (value, blue);
	       value_bin = value/bins;
	     }
	   else
	     value_bin = s[GRAY_PIX]/bins;

	   if (mask_area)
	     values[HISTOGRAM_VALUE][value_bin] += (double) *m / 65535.0;
	   else
	     values[HISTOGRAM_VALUE][value_bin] += 1.0;

	   s += s_num_channels;

	   if (mask_area)
	     m += m_num_channels;
	 }

       src += pixelarea_rowstride (src_area);
 
       if (mask_area)
	 mask += pixelarea_rowstride (mask_area);
    }
}



static void
threshold_histogram_range_u8 (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[32];

  td = (ThresholdDialog *) user_data;

  td->low_threshold = start;
  td->high_threshold = end;
  td->bins = bins;
  sprintf (text, "%d", (int)td->low_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%d", (int)td->high_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

static void
threshold_histogram_range_u16 (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[32];
  gint low, high;

  td = (ThresholdDialog *) user_data;

  low = start * 256;
  high = end * 256 + 255;
  td->low_threshold = low;
  td->high_threshold = high;
  td->bins = bins;
  sprintf (text, "%d", (int)td->low_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%d", (int)td->high_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

static void
threshold_histogram_range_float (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[32];
  gfloat low, high;

  td = (ThresholdDialog *) user_data;

  low = start/256.0;
  high = (end+1)/256.0;
  td->low_threshold = low;
  td->high_threshold = high;
  td->bins = bins;
  sprintf (text, "%f", td->low_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%f", td->high_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

static void
threshold_histogram_range_float16 (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[32];
  gfloat low, high;

  td = (ThresholdDialog *) user_data;

  low = start/256.0;
  high = (end+1)/256.0;
  td->low_threshold = low;
  td->high_threshold = high;
  td->bins = bins;
  sprintf (text, "%f", td->low_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%f", td->high_threshold);
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

/*  threshold action functions  */

static void
threshold_histogram_range_bfp (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[32];

  td = (ThresholdDialog *) user_data;

  td->low_threshold = start;
  td->high_threshold = end;
  td->bins = bins;
  sprintf (text, "%f", BFP_TO_FLOAT(td->low_threshold));
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%f", BFP_TO_FLOAT(td->high_threshold));
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

static void
threshold_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
threshold_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
}

static void
threshold_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
}

static void
threshold_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
threshold_control (Tool     *tool,
		   int       action,
		   gpointer  gdisp_ptr)
{
  Threshold * thresh;

  thresh = (Threshold *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (threshold_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (threshold_dialog->image_map);
	  active_tool->preserve = FALSE;
	  threshold_dialog->image_map = NULL;
	  threshold_cancel_callback (NULL, (gpointer) threshold_dialog);
	}
      break;
    }
}

Tool *
tools_new_threshold ()
{
  Tool		* tool;
  Threshold	* private;

  /*  The tool options  */
  if (!threshold_options)
    threshold_options = tools_register_no_options (THRESHOLD, "Threshold Options");

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (Threshold *) g_malloc_zero (sizeof (Threshold));

  tool->type = THRESHOLD;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = threshold_button_press;
  tool->button_release_func = threshold_button_release;
  tool->motion_func = threshold_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = threshold_cursor_update;
  tool->control_func = threshold_control;
  tool->preserve = FALSE;

  return tool;
}

void
tools_free_threshold (Tool *tool)
{
  Threshold * thresh;

  thresh = (Threshold *) tool->private;

  /*  Close the color select dialog  */
  if (threshold_dialog)
    threshold_cancel_callback (NULL, (gpointer) threshold_dialog);

  g_free (thresh);
}

void
threshold_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  Tag 	    tag;
  gint      start, end;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("Threshold does not operate on indexed drawables.");
      return;
    }

  tag = gimage_tag (gdisp->gimage);
  threshold_funcs (tag);

  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_new_dialog (tag);
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);

  threshold_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  threshold_dialog->color = drawable_color (threshold_dialog->drawable);
  threshold_dialog->image_map = image_map_create (gdisp_ptr, threshold_dialog->drawable);
  histogram_update (threshold_dialog->histogram,
		    threshold_dialog->drawable,
		    threshold_histogram_info,
		    (void *) threshold_dialog);

  switch (tag_precision (tag))
  {
  case PRECISION_U8:
     start = threshold_dialog->low_threshold;
     end = threshold_dialog->high_threshold;
     break;
  case PRECISION_BFP:
  case PRECISION_U16:
     start = threshold_dialog->low_threshold/256;
     end = threshold_dialog->high_threshold/256;
     break;
  case PRECISION_FLOAT:
  case PRECISION_FLOAT16:
     start = (gint)(threshold_dialog->low_threshold * 255);
     end =  (gint)(threshold_dialog->high_threshold * 255);
     break;
  default:
     start = 0;
     end = 255;
    break;
  }

  histogram_range (threshold_dialog->histogram, start, end);
  if (threshold_dialog->preview)
    threshold_preview (threshold_dialog);
}


/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", threshold_ok_callback, NULL, NULL },
  { "Cancel", threshold_cancel_callback, NULL, NULL }
};

static ThresholdDialog *
threshold_new_dialog (Tag tag)
{
  ThresholdDialog *td;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  gint start, end;

  td = g_malloc (sizeof (ThresholdDialog));
  td->preview = TRUE;
  
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
      td->high_threshold = 255;
      td->low_threshold = 255/2;
      td->bins = 256;
     break;
  case PRECISION_BFP:
  case PRECISION_U16:
      td->high_threshold = 65535;
      td->low_threshold = 65535/2;
      td->bins = 256;
     break;
  case PRECISION_FLOAT:
  case PRECISION_FLOAT16:
      td->high_threshold = 1.0;
      td->low_threshold = .5;
      td->bins = 256;
     break;
     break;
  default:
    break;
  }
  

  /*  The shell and main vbox  */
  td->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (td->shell), "threshold", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (td->shell), _("Threshold"));
  minimize_register(td->shell);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (td->shell), "delete_event",
		      GTK_SIGNAL_FUNC (threshold_delete_callback),
		      td);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (td->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for threshold text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold Range: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low threshold text  */
  td->low_threshold_text = gtk_entry_new ();

  switch (tag_precision (tag))
  {
  case PRECISION_U8:
     gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "127");
     break;
  case PRECISION_U16:
     gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "32767");
     break;
  case PRECISION_FLOAT:
  case PRECISION_FLOAT16:
     gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "0.5");
     break;
  case PRECISION_BFP:
     gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "16383");
     break;
  default:
    break;
  }

  gtk_widget_set_usize (td->low_threshold_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), td->low_threshold_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (td->low_threshold_text), "changed",
		      (GtkSignalFunc) threshold_low_threshold_text_update,
		      td);
  gtk_widget_show (td->low_threshold_text);

  /* high threshold text  */
  td->high_threshold_text = gtk_entry_new ();

  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "255");
     break;
  case PRECISION_BFP:
  case PRECISION_U16:
    gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "65535");
     break;
  case PRECISION_FLOAT:
  case PRECISION_FLOAT16:
    gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "1.0");
     break;
  default:
    break;
  }
  
  gtk_widget_set_usize (td->high_threshold_text, TEXT_WIDTH, 25);

  gtk_box_pack_start (GTK_BOX (hbox), td->high_threshold_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (td->high_threshold_text), "changed",
		      (GtkSignalFunc) threshold_high_threshold_text_update,
		      td);
  gtk_widget_show (td->high_threshold_text);
  gtk_widget_show (hbox);

  /*  The threshold histogram  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  td->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT, td->bins,
				    threshold_histogram_range, (void *) td);
  gtk_container_add (GTK_CONTAINER (frame), td->histogram->histogram_widget);
  gtk_widget_show (td->histogram->histogram_widget);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), td->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) threshold_preview_update,
		      td);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = td;
  action_items[1].user_data = td;
  build_action_area (GTK_DIALOG (td->shell), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (td->shell);

  /* This code is so far removed from the histogram creation because the
     function histogram_range requires a non-NULL drawable, and that
     doesn't happen until after the top-level dialog is shown. */

  switch (tag_precision (tag))
  {
  case PRECISION_U8:
     start = td->low_threshold;
     end = td->high_threshold;
     break;
  case PRECISION_BFP:
  case PRECISION_U16:
     start = td->low_threshold/256;
     end = td->high_threshold/256;
     break;
  case PRECISION_FLOAT:
  case PRECISION_FLOAT16:
     start = (gint) (td->low_threshold  * 255);
     end = (gint) (td->high_threshold * 255);
     break;
  default:
     start = -1;
     end = -1; 
    break;
  }

  histogram_range (td->histogram, start, end);
  return td;
}

static void
threshold_preview (ThresholdDialog *td)
{
  if (!td->image_map)
    g_message (_("threshold_preview(): No image map"));
  image_map_apply (td->image_map, threshold, (void *) td);
}

static void
threshold_ok_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (td->shell))
    gtk_widget_hide (td->shell);

  active_tool->preserve = TRUE;

  if (!td->preview)
    image_map_apply (td->image_map, threshold, (void *) td);
  if (td->image_map)
    image_map_commit (td->image_map);

  active_tool->preserve = FALSE;

  td->image_map = NULL;
}

static gint
threshold_delete_callback (GtkWidget *w,
			   GdkEvent *e,
			   gpointer client_data) 
{
  threshold_cancel_callback (w, client_data);

  return TRUE;
}

static void
threshold_cancel_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (td->shell))
    gtk_widget_hide (td->shell);

  if (td->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (td->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  td->image_map = NULL;
}

static void
threshold_preview_update (GtkWidget *w,
			  gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      td->preview = TRUE;
      threshold_preview (td);
    }
  else
{
    td->preview = FALSE;
active_tool->preserve = TRUE;
    image_map_remove(td->image_map);
    active_tool->preserve = FALSE;
    gdisplays_flush ();
}
}

static void
threshold_low_threshold_text_update_u8 (GtkWidget *w,
				     gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), 0, td->high_threshold);

  if (value != td->low_threshold)
    {
      td->low_threshold = value;
      start = td->low_threshold;
      end = td->high_threshold;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_low_threshold_text_update_u16 (GtkWidget *w,
				     gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), 0, td->high_threshold);

  if (value != td->low_threshold)
    {
      td->low_threshold = value;
      start = td->low_threshold/256;
      end = td->high_threshold/256;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_low_threshold_text_update_float (GtkWidget *w,
				     gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  gfloat value;
  gint start, end;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS ( atof (str), 0.0, td->high_threshold);

  if (value != td->low_threshold)
    {
      td->low_threshold = value;
      start = (gint)(td->low_threshold * 255);
      end = (gint)(td->high_threshold * 255);
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_low_threshold_text_update_float16 (GtkWidget *w,
				     gpointer   data)
{
  threshold_low_threshold_text_update_float (w,data);
}

static void
threshold_low_threshold_text_update_bfp (GtkWidget *w,
				     gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (FLOAT_TO_BFP( atof (str)), 0, td->high_threshold);

  if (value != td->low_threshold)
    {
      td->low_threshold = value;
      start = td->low_threshold/256;
      end = td->high_threshold/256;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_text_update_u8 (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;
  Tag tag;

  td = (ThresholdDialog *) data;
  tag = drawable_tag (td->drawable);
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), td->low_threshold, td->bins-1);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      start = td->low_threshold;
      end = td->high_threshold;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_text_update_u16 (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;
  Tag tag;

  td = (ThresholdDialog *) data;
  tag = drawable_tag (td->drawable);
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), td->low_threshold, 65535);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      start = td->low_threshold/256;
      end = td->high_threshold/256;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_text_update_float (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  gfloat value;
  gint start, end;
  Tag tag;

  td = (ThresholdDialog *) data;
  tag = drawable_tag (td->drawable);
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS ( atof (str), td->low_threshold, 1.0);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      start =(gint) (td->low_threshold * 255);
      end = (gint) (td->high_threshold *255);
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_text_update_float16 (GtkWidget *w,
				      gpointer   data)
{
  threshold_high_threshold_text_update_float (w,data);
}

static void
threshold_high_threshold_text_update_bfp (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;
  gint start, end;
  Tag tag;

  td = (ThresholdDialog *) data;
  tag = drawable_tag (td->drawable);
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (FLOAT_TO_BFP(atof (str)), td->low_threshold, 65535);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      start = td->low_threshold/256;
      end = td->high_threshold/256;
      histogram_range (td->histogram, start, end);
      if (td->preview)
	threshold_preview (td);
    }
}

/*  The threshold procedure definition  */
ProcArg threshold_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "low_threshold",
    "the low threshold value: (0 <= low_threshold <= maxbright)"
  },
  { PDB_INT32,
    "high_threshold",
    "the high threshold value: (0 <= high_threshold <= maxbright)"
  }
};

ProcRecord threshold_proc =
{
  "gimp_threshold",
  "Threshold the specified drawable",
  "This procedures generates a threshold map of the specified drawable.  All pixels between the values of 'low_threshold' and 'high_threshold' are replaced with white, and all other pixels with black.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  threshold_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { threshold_invoker } },
};

static Argument *
threshold_invoker (args)
     Argument *args;
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  ThresholdDialog td;
  GImage *gimage;
  CanvasDrawable *drawable;
  int low_threshold;
  int high_threshold;
  int int_value;
  int max=0, bins=0;
  int x1, y1, x2, y2;
  void *pr;

  drawable    = NULL;
  low_threshold  = 0;
  high_threshold = 0;

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
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  if( success )
  {
      switch( tag_precision( gimage_tag( gimage ) ) )
      {
      case PRECISION_U8:
	  bins = 256;
	  max = bins-1;
	  break;

      case PRECISION_BFP:
      case PRECISION_U16:
	  bins = 65536;
	  max = bins-1;
	  break;

      case PRECISION_FLOAT:
	  g_warning( "histogram_float not implemented yet." );
	  success = FALSE;
      }
  } 

  /*  low threhsold  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 0 && int_value <= max)
	low_threshold = int_value;
      else
	success = FALSE;
    }
  /*  high threhsold  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value <= max)
	high_threshold = int_value;
      else
	success = FALSE;
    }
  if (success)
    success =  (low_threshold < high_threshold);

  /*  arrange to modify the levels  */
  if (success)
    {
      td.color = drawable_color (drawable);
      td.low_threshold = low_threshold;
      td.high_threshold = high_threshold;

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); pr != NULL; pr = pixelarea_process (pr))
	threshold (&src_area, &dest_area, (void *) &td);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }
  return procedural_db_return_args (&threshold_proc, success);
}
