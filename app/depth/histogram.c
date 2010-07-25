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
#include <stdio.h>
#include <stdlib.h>
#include "float16.h"
#include <string.h>
#include <math.h>
#include "../appenv.h"
#include "../drawable.h"
#include "../errors.h"
#include "../gdisplay.h"
#include "../gimage.h"
#include "../gimage_mask.h"
#include "../histogram.h"
#include "../tag.h"
#include "../procedural_db.h"

#define WAITING 0
#define WORKING 1

#define WORK_DELAY 1

#define HISTOGRAM_MASK GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON1_MOTION_MASK

#define HISTOGRAM 0x1
#define RANGE     0x2
#define ALL       0xF

/*  Local structures  */
typedef struct HistogramPrivate
{
  HistogramRangeCallback  range_callback;
  void *                  user_data;
  int                     channel;
  HistogramValues         values;
  int                     start;    /* bin to start with */
  int                     end;      /* bin to end with */
  int                     bins;     /* number of bins  */
  int          color;
  
  double       mean;
  double       std_dev;
  double       median;
  double       pixels;
  double       count;
  double       percentile;
  
} HistogramPrivate;

HistogramRangeCallback histogram_histogram_range;

/**************************/
/*  Function definitions  */

static void
histogram_draw (Histogram *histogram,
		        int        update)
{
  HistogramPrivate *histogram_p;
  double max;
  double log_val;
  int i, x, y;
  int x1, x2;
  int width, height;

  histogram_p = (HistogramPrivate *) histogram->private_part;
  width = histogram->histogram_widget->allocation.width - 2;
  height = histogram->histogram_widget->allocation.height - 2;

  if (update & HISTOGRAM)
    {
      max = FLT_MIN; /* min for float */
      for (i = 0; i < histogram_p->bins; i++)
	{
	    log_val = histogram_p->values[histogram_p->channel][i];

	  if (log_val > max)
	    max = log_val;
	}

#if 0
      /*  find the maximum value  */
      max = FLT_MAX; /* max for float */
      for (i = 0; i < histogram_p->bins; i++)
	{
	  if (histogram_p->values[histogram_p->channel][i])
	    log_val = log (histogram_p->values[histogram_p->channel][i]);
	  else
	    log_val = 0;

	  if (log_val > max)
	    max = log_val;
	}
#endif
      /*  clear the histogram  */
      gdk_window_clear (histogram->histogram_widget->window);

      /*  Draw the axis  */
      gdk_draw_line (histogram->histogram_widget->window,
		     histogram->histogram_widget->style->black_gc,
		     1, height + 1, width, height + 1);

      /*  Draw the spikes  */
      for (i = 0; i < histogram_p->bins; i++)
	{
	  x = (width * i) / histogram_p->bins + 1;
	  if (histogram_p->values[histogram_p->channel][i])
#if 0
	    y = (int) ((height * log (histogram_p->values[histogram_p->channel][i])) / max);
#endif
       	  y = (int) ((height * (histogram_p->values[histogram_p->channel][i])) / max);
	  else
	    y = 0;
	  gdk_draw_line (histogram->histogram_widget->window,
			 histogram->histogram_widget->style->black_gc,
			 x, height + 1,
			 x, height + 1 - y);
	}
    }

  if ((update & RANGE) && histogram_p->start >= 0)
    {
      x1 = (width * MIN (histogram_p->start, histogram_p->end)) / histogram_p->bins + 1;
      x2 = (width * MAX (histogram_p->start, histogram_p->end)) / histogram_p->bins + 1;
      gdk_gc_set_function (histogram->histogram_widget->style->black_gc, GDK_INVERT);
      gdk_draw_rectangle (histogram->histogram_widget->window,
			  histogram->histogram_widget->style->black_gc, TRUE,
			  x1, 1, (x2 - x1) + 1, height);
      gdk_gc_set_function (histogram->histogram_widget->style->black_gc, GDK_COPY);
    }
}

static gint
histogram_events (GtkWidget *widget,
		  GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  Histogram *histogram;
  HistogramPrivate *histogram_p;
  int width;

  histogram = (Histogram *) gtk_object_get_user_data (GTK_OBJECT (widget));
  if (!histogram)
    return FALSE;
  histogram_p = (HistogramPrivate *) histogram->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      histogram_draw (histogram, ALL);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      width = histogram->histogram_widget->allocation.width - 2;

      histogram_draw (histogram, RANGE);

      histogram_p->start = BOUNDS ((((bevent->x - 1) * histogram_p->bins) / width), 0, histogram_p->bins-1);
      histogram_p->end = histogram_p->start;


      histogram_draw (histogram, RANGE);
      break;

    case GDK_BUTTON_RELEASE:
      (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				       MAX (histogram_p->start, histogram_p->end),
					   histogram_p->bins,
				       histogram_p->values,
				       histogram);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      width = histogram->histogram_widget->allocation.width - 2;

      histogram_draw (histogram, RANGE);

      histogram_p->start = BOUNDS ((((mevent->x - 1) * histogram_p->bins) / width), 0, histogram_p->bins-1);

      histogram_draw (histogram, RANGE);
      break;

    default:
      break;
    }

  return FALSE;
}

Histogram * 
histogram_create (int                     width,
		  int                     height,
		  int                     bins,
		  HistogramRangeCallback  range_callback,
		  void                   *user_data)
{
  Histogram        *histogram;
  HistogramPrivate *histogram_p;
  int               i, j;

  histogram = (Histogram *) g_malloc_zero (sizeof (Histogram));
  histogram->histogram_widget = gtk_drawing_area_new ();
  histogram->shell = NULL;
  histogram->info_labels[0] = NULL; // If the first one is null the rest are assumed to be too.
  histogram->channel_menu = NULL; 

  gtk_drawing_area_size (GTK_DRAWING_AREA (histogram->histogram_widget), width + 2, height + 2);
  gtk_widget_set_events (histogram->histogram_widget, HISTOGRAM_MASK);
  gtk_signal_connect (GTK_OBJECT (histogram->histogram_widget), "event",
		      (GtkSignalFunc) histogram_events, histogram);
  gtk_object_set_user_data (GTK_OBJECT (histogram->histogram_widget), (gpointer) histogram);

  /*  The private details of the histogram  */
  histogram_p = (HistogramPrivate *) g_malloc_zero (sizeof (HistogramPrivate));
  histogram->private_part = (void *) histogram_p;
  histogram_p->range_callback = range_callback;
  histogram_p->user_data = user_data;
  histogram_p->channel = HISTOGRAM_VALUE;
  histogram_p->start = 0;
  histogram_p->end = bins-1;
  histogram_p->bins = bins;

  /*  Initialize the values array  */
  for (j = 0; j <= HISTOGRAM_ALPHA; j++)
  {
    histogram_p->values[j] = (double*)g_malloc( sizeof(double) * bins );

    for (i = 0; i < bins; i++)
      histogram_p->values[j][i] = 0.0;
  }

  return histogram;
}

void
histogram_free (Histogram  *histogram)
{
  int j;
  HistogramPrivate *histogram_p = (HistogramPrivate*)histogram->private_part;

  gtk_widget_destroy (histogram->histogram_widget);

  for (j = 0; j <= HISTOGRAM_ALPHA; j++)
      g_free( histogram_p->values[j] );

  g_free (histogram_p);
  g_free (histogram);
}

void
histogram_update (Histogram         *histogram,
		  CanvasDrawable      *drawable,
		  HistogramInfoFunc  info_func,
		  void              *user_data)
{
  HistogramPrivate *histogram_p;
  GImage           *gimage;
  Channel          *mask;
  PixelArea         src_area, mask_area;
  int               no_mask, i, j;
  int               x1, y1, x2, y2;
  int               off_x, off_y;
  void             *pr;

  /*  Make sure the drawable is still valid  */
  if( (gimage = drawable_gimage(drawable)) == NULL )
    return;

  histogram_p = (HistogramPrivate *) histogram->private_part;
  histogram_p->color = drawable_color(drawable);

  /*  The information collection should occur only within selection bounds  */
  no_mask = (drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2) == FALSE);
  drawable_offsets ( (drawable), &off_x, &off_y);

  pixelarea_init (&src_area, drawable_data ( (drawable)),
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);

  mask = gimage_get_mask (gimage);
  pixelarea_init (&mask_area, drawable_data (GIMP_DRAWABLE(mask)),
		     x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);

  /*  Initialize the values array  */
  for (j = 0; j <= HISTOGRAM_ALPHA; j++)
    for (i = 0; i < histogram_p->bins; i++)
      histogram_p->values[j][i] = 0.0;

  /*  Apply the image transformation to the pixels  */
  if (no_mask)
  {
    for (pr = pixelarea_register (1, &src_area); pr != NULL; pr = pixelarea_process (pr))
        (* info_func) (&src_area, NULL, histogram_p->values, histogram);
  }
  else
  {
    for (pr = pixelarea_register (2, &src_area, &mask_area); pr != NULL; pr = pixelarea_process (pr))
      (* info_func) (&src_area, &mask_area, histogram_p->values, histogram);
  }

  /*  Make sure the histogram is updated  */
  gtk_widget_draw (histogram->histogram_widget, NULL);

  /*  Give a range callback  */
  (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				   MAX (histogram_p->start, histogram_p->end),
				   histogram_p->bins,
				   histogram_p->values,
				   histogram);

  /* Now we update the channel menu (if defined) */
  if (histogram->channel_menu != NULL && 0){
    /* check for alpha channel */
    if (drawable_has_alpha (drawable))
      gtk_widget_set_sensitive( histogram->color_option_items[4].widget, TRUE);
    else 
      gtk_widget_set_sensitive( histogram->color_option_items[4].widget, FALSE);
  
    /*  hide or show the channel menu based on image type  */
    if (histogram_p->color)
      for (i = 0; i < 5; ++i) 
	gtk_widget_set_sensitive( histogram->color_option_items[i].widget, TRUE);
    else 
      for (i = 1; i < 5; ++i) 
	gtk_widget_set_sensitive( histogram->color_option_items[i].widget, FALSE);

    /* set the current selection */
    gtk_option_menu_set_history ( GTK_OPTION_MENU (histogram->channel_menu), 0);
  }
}

void
histogram_range (Histogram *histogram,
		 int        start,
		 int        end)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;

  histogram_draw (histogram, RANGE);
  histogram_p->start = start;
  histogram_p->end = end;
  histogram_draw (histogram, RANGE);
}

void
histogram_channel (Histogram *histogram,
		   int        channel)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;
  histogram_p->channel = channel;
  histogram_draw (histogram, ALL);

  /*  Give a range callback  */
  (* histogram_p->range_callback) (MIN (histogram_p->start, histogram_p->end),
				   MAX (histogram_p->start, histogram_p->end),
				   histogram_p->bins,
				   histogram_p->values,
				   histogram);
}

gint
histogram_bins (Histogram *histogram)
{
  HistogramPrivate *histogram_p;
  histogram_p = (HistogramPrivate *) histogram->private_part;
  return histogram_p->bins;
}

HistogramValues *
histogram_values (Histogram *histogram)
{
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) histogram->private_part;

  return &histogram_p->values;
}


HistogramInfoFunc histogram_histogram_info;
static void  histogram_histogram_info_u8 (PixelArea*, PixelArea*, HistogramValues, void *);
static void  histogram_histogram_info_u16 (PixelArea*, PixelArea*, HistogramValues, void *);
static void  histogram_histogram_info_float (PixelArea*, PixelArea*, HistogramValues, void *);
static void  histogram_histogram_info_float16 (PixelArea*, PixelArea*, HistogramValues, void *);
static void  histogram_histogram_info_bfp (PixelArea*, PixelArea*, HistogramValues, void *);

#if 0
typedef void (*HistogramToolHistogramRangeFunc)(int, int, int, HistogramValues, void *);
#endif
static void       histogram_histogram_range_u8 (int, int, int, HistogramValues, void *);
static void       histogram_histogram_range_u16 (int, int, int, HistogramValues, void *);
static void       histogram_histogram_range_float (int, int, int, HistogramValues, void *);
static void       histogram_histogram_range_float16 (int, int, int, HistogramValues, void *);
static void       histogram_histogram_range_bfp (int, int, int, HistogramValues, void *);

typedef void (*HistogramUpdataFunc)(Histogram *, int, int);
static HistogramUpdataFunc histogram_dialog_update;
static void       histogram_dialog_update_u8 (Histogram *, int, int);
static void       histogram_dialog_update_u16 (Histogram *, int, int);
static void       histogram_dialog_update_float (Histogram *, int, int);
static void       histogram_dialog_update_float16 (Histogram *, int, int);
static void       histogram_dialog_update_bfp (Histogram *, int, int);

/* static Argument * histogram_invoker (Argument *args); */

static char * histogram_info_names[7] =
{
  "Mean: ",
  "Std Dev: ",
  "Median: ",
  "Pixels: ",
  "Intensity: ",
  "Count: ",
  "Percentile: "
};

/*  Histogram Tool Histogram Info  */
void
histogram_histogram_funcs (Tag tag)
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
     histogram_histogram_info = histogram_histogram_info_u8; 
     histogram_dialog_update = histogram_dialog_update_u8; 
     histogram_histogram_range = histogram_histogram_range_u8;
     break;
  case PRECISION_U16:
     histogram_histogram_info = histogram_histogram_info_u16; 
     histogram_dialog_update = histogram_dialog_update_u16; 
     histogram_histogram_range = histogram_histogram_range_u16;
     break;
  case PRECISION_FLOAT:
     histogram_histogram_info = histogram_histogram_info_float; 
     histogram_dialog_update = histogram_dialog_update_float; 
     histogram_histogram_range = histogram_histogram_range_float;
     break;
  case PRECISION_FLOAT16:
     histogram_histogram_info = histogram_histogram_info_float16; 
     histogram_dialog_update = histogram_dialog_update_float16; 
     histogram_histogram_range = histogram_histogram_range_float16;
     break;
  case PRECISION_BFP:
     histogram_histogram_info = histogram_histogram_info_bfp; 
     histogram_dialog_update = histogram_dialog_update_bfp; 
     histogram_histogram_range = histogram_histogram_range_bfp;
     break;
  default:
     histogram_histogram_info = NULL; 
     histogram_dialog_update = NULL; 
     histogram_histogram_range = NULL;
    break;
  } 
}

/*  histogram machinery  */

static void
histogram_histogram_info_u8 (  PixelArea       *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  Histogram *htd;
  int                  w, h;
  guchar *src, *mask = NULL;
  guint8 *s, *m = NULL;
  gint    value, red, green, blue, alpha=128;
  gfloat  scale = 1./255.;
  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint    has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;
  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guchar*)pixelarea_data (mask_area);

   while (h--)
     {
       w = pixelarea_width (src_area);
       s = (guint8*)src;

       if (mask_area)
	 m = (guint8*)mask;

       while (w--)
	 {
	   if (histogram_p->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
               if(has_alpha)
                   alpha = s[ALPHA_PIX];

	       if (mask_area)
		 {
		   values[HISTOGRAM_VALUE][value] += (double) *m * scale;
		   values[HISTOGRAM_RED][red]     += (double) *m * scale;
		   values[HISTOGRAM_GREEN][green] += (double) *m * scale;
		   values[HISTOGRAM_BLUE][blue]   += (double) *m * scale;
	           if(has_alpha)
        	       values[HISTOGRAM_ALPHA][alpha]   += (double) *m * scale;
		 }
	       else
		 {
		   values[HISTOGRAM_VALUE][value] += 1.0;
		   values[HISTOGRAM_RED][red]     += 1.0;
		   values[HISTOGRAM_GREEN][green] += 1.0;
		   values[HISTOGRAM_BLUE][blue]   += 1.0;
	           if(has_alpha)
        	       values[HISTOGRAM_ALPHA][alpha]   += 1.0;
		 }
	     }
	   else
	     {
	       value = s[GRAY_PIX];
	       if (mask_area)
		 values[HISTOGRAM_VALUE][value] += (double) *m * scale;
	       else
		 values[HISTOGRAM_VALUE][value] += 1.0;
	     }

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
histogram_histogram_info_u16 ( PixelArea       *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  Histogram *htd = (Histogram *) user_data;
  gint bins;
  int                  w, h;
  guchar *src, *mask = NULL;
  guint16 *s, *m = NULL;
  gint    value, red, green, blue, alpha=0;
  gfloat  scale = 1./65535.;
  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint    has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);
  gint 		       value_bin=0, red_bin, green_bin, blue_bin, alpha_bin=0;
  HistogramPrivate *histogram_p = (HistogramPrivate *) htd->private_part;
  bins = histogram_p->bins;
  h = pixelarea_height (src_area);
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
	   if (histogram_p->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
               if(has_alpha)
                   alpha = s[ALPHA_PIX];

	       /* figure out which bin to place it in */ 
  
	       value_bin = bins*(double)value*scale;
	       red_bin = bins*(double)red*scale;   	
	       green_bin = bins*(double)green*scale;
	       blue_bin = bins*(double)blue*scale;
               if(has_alpha)
                   alpha_bin = bins*(double)alpha*scale;

	       if (mask_area)
		 {
		   values[HISTOGRAM_VALUE][value_bin] += (double) *m * scale;
		   values[HISTOGRAM_RED][red_bin]     += (double) *m * scale;
		   values[HISTOGRAM_GREEN][green_bin] += (double) *m * scale;
		   values[HISTOGRAM_BLUE][blue_bin]   += (double) *m * scale;
	           if(has_alpha)
        	       values[HISTOGRAM_ALPHA][alpha_bin]   += (double) *m * scale;
		 }
	       else
		 {
		   values[HISTOGRAM_VALUE][value_bin] += 1.0;
		   values[HISTOGRAM_RED][red_bin]     += 1.0;
		   values[HISTOGRAM_GREEN][green_bin] += 1.0;
		   values[HISTOGRAM_BLUE][blue_bin]   += 1.0;
                   if(has_alpha)
                       values[HISTOGRAM_ALPHA][alpha_bin]   += 1.0;
		 }
	     }
	   else
	     {
	       value = s[GRAY_PIX];
	       value_bin = bins*(double)value*scale;
	       if (mask_area)
		 values[HISTOGRAM_VALUE][value_bin] += (double) *m * scale;
	       else
		 values[HISTOGRAM_VALUE][value_bin] += 1.0;
	     }

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
histogram_histogram_info_float (PixelArea *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  Histogram *htd = (Histogram *) user_data;
  gint bins;
  int                  w, h;
  guchar *src, *mask = NULL;
  gfloat *s, *m = NULL;
  gfloat    value, red, green, blue, alpha=0;
  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint    has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);
  gint value_bin=0, red_bin, green_bin, blue_bin, alpha_bin=0;
  HistogramPrivate *histogram_p = (HistogramPrivate *) htd->private_part;

  bins = histogram_p->bins;
  h = pixelarea_height (src_area);
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
	   if (histogram_p->color)
	     {
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
               if(has_alpha)
                   alpha  = s[ALPHA_PIX];
#if 1
	       red = CLAMP (red, 0.0, 1.0);
	       green = CLAMP (green, 0.0, 1.0);
	       blue = CLAMP (blue, 0.0, 1.0);
               if(has_alpha)
                   alpha = CLAMP (alpha, 0.0, 1.0);
#endif
	       value = MAX (red, green);
	       value = MAX (value, blue);

	       value_bin = (int)(value * (bins-1));
	       red_bin = (int)(red * (bins-1));   	
	       green_bin = (int)(green * (bins-1));
	       blue_bin = (int)(blue * (bins-1));
               if(has_alpha)
                   alpha_bin = (int)(alpha * (bins-1));

	       if (mask_area)
		 {
		   values[HISTOGRAM_VALUE][value_bin] += (double) *m;
		   values[HISTOGRAM_RED][red_bin] += (double) *m;
		   values[HISTOGRAM_GREEN][green_bin] += (double) *m;
		   values[HISTOGRAM_BLUE][blue_bin]   += (double) *m;
	           if(has_alpha)
	               values[HISTOGRAM_ALPHA][alpha_bin]   += (double) *m;
		 }
	       else
		 {
		   values[HISTOGRAM_VALUE][value_bin] += 1.0;
		   values[HISTOGRAM_RED][red_bin]     += 1.0;
		   values[HISTOGRAM_GREEN][green_bin] += 1.0;
		   values[HISTOGRAM_BLUE][blue_bin]   += 1.0;
                   if(has_alpha)
	               values[HISTOGRAM_ALPHA][alpha_bin]   += 1.0;
		 }
	     }
	   else
	     {
	       value = s[GRAY_PIX];
	       value_bin = (int)(value * (bins-1));
	       if (mask_area)
		 values[HISTOGRAM_VALUE][value_bin] += (double) *m;
	       else
		 values[HISTOGRAM_VALUE][value_bin] += 1.0;
	     }

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
histogram_histogram_info_float16 (PixelArea *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  Histogram *htd = (Histogram *) user_data;
  gint bins;
  int                  w, h;
  guchar *src, *mask = NULL;
  guint16 *s, *m = NULL;
  gfloat    value, red, green, blue, alpha=0;
  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint    has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);
  gint value_bin = 0, red_bin, green_bin, blue_bin, alpha_bin=0;
  ShortsFloat u;
  gfloat m_val;
  HistogramPrivate *histogram_p;

  histogram_p = (HistogramPrivate *) htd->private_part;
  bins = histogram_p->bins;
  h = pixelarea_height (src_area);
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
	   if (histogram_p->color)
	     {
	       red   = FLT (s[RED_PIX], u);
	       green = FLT (s[GREEN_PIX], u);
	       blue  = FLT (s[BLUE_PIX], u);
               if(has_alpha)
	           alpha  = FLT (s[ALPHA_PIX], u);
#if 1
	       red = CLAMP (red, -0.01, 1.01);
	       green = CLAMP (green, -0.01, 1.01);
	       blue = CLAMP (blue, -0.01, 1.01);
               if(has_alpha)
	           alpha = CLAMP (alpha, -0.01, 1.01);
#endif
	       value = MAX (red, green);
	       value = MAX (value, blue);

	       value_bin = (int)((value) * 100 + 1);
	       red_bin = (int)((red) * 100 + 1);   	
	       green_bin = (int)((green) * 100 + 1);
	       blue_bin = (int)((blue) * 100 + 1);
               if(has_alpha)
                   alpha_bin = (int)((alpha) * 100 + 1);

	       if (mask_area)
		 {
		   m_val = FLT (*m, u);
		   values[HISTOGRAM_VALUE][value_bin] += (double)m_val;
		   values[HISTOGRAM_RED][red_bin] += (double)m_val;
		   values[HISTOGRAM_GREEN][green_bin] += (double)m_val;
		   values[HISTOGRAM_BLUE][blue_bin]   += (double)m_val;
                   if(has_alpha)
                       values[HISTOGRAM_ALPHA][alpha_bin]   += (double)m_val;
		 }
	       else
		 {
		   
		   values[HISTOGRAM_VALUE][value_bin] += 1.0;
		   values[HISTOGRAM_RED][red_bin]     += 1.0;
		   values[HISTOGRAM_GREEN][green_bin] += 1.0;
		   values[HISTOGRAM_BLUE][blue_bin]   += 1.0;
                   if(has_alpha)
                       values[HISTOGRAM_ALPHA][alpha_bin]   += 1.0;
		 }
	     }
	   else
	     {
	       value = s[GRAY_PIX];
	       value_bin = (int)(value * (bins-1));
	       if (mask_area)
		 values[HISTOGRAM_VALUE][value_bin] += (double) FLT(*m, u);
	       else
		 values[HISTOGRAM_VALUE][value_bin] += 1.0;
	     }

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
histogram_histogram_info_bfp (PixelArea       *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  Histogram *htd = (Histogram *) user_data;
  gint bins;
  int                  w, h;
  guchar *src, *mask = NULL;
  guint16 *s, *m = NULL;
  gint    value, red, green, blue, alpha=0;
  gfloat  scale = 1./65535.;
  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint    has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);
  gint 		       value_bin=0, red_bin, green_bin, blue_bin, alpha_bin=0;
  HistogramPrivate *histogram_p = (HistogramPrivate *) htd->private_part;
  bins = histogram_p->bins;
  h = pixelarea_height (src_area);
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
	   if (histogram_p->color)
	     {
	       value = MAX (s[RED_PIX], s[GREEN_PIX]);
	       value = MAX (value, s[BLUE_PIX]);
	       red   = s[RED_PIX];
	       green = s[GREEN_PIX];
	       blue  = s[BLUE_PIX];
	       alpha  = s[ALPHA_PIX];

	       /* figure out which bin to place it in */ 
  
	       value_bin = bins*(double)value*scale;
	       red_bin = bins*(double)red*scale;   	
	       green_bin = bins*(double)green*scale;
	       blue_bin = bins*(double)blue*scale;
               if(has_alpha)
	           alpha_bin = bins*(double)alpha*scale;

	       if (mask_area)
		 {
		   values[HISTOGRAM_VALUE][value_bin] += (double) *m * scale;
		   values[HISTOGRAM_RED][red_bin]     += (double) *m * scale;
		   values[HISTOGRAM_GREEN][green_bin] += (double) *m * scale;
		   values[HISTOGRAM_BLUE][blue_bin]   += (double) *m * scale;
                   if(has_alpha)
		       values[HISTOGRAM_ALPHA][alpha_bin]   += (double) *m * scale;
		 }
	       else
		 {
		   values[HISTOGRAM_VALUE][value_bin] += 1.0;
		   values[HISTOGRAM_RED][red_bin]     += 1.0;
		   values[HISTOGRAM_GREEN][green_bin] += 1.0;
		   values[HISTOGRAM_BLUE][blue_bin]   += 1.0;
                   if(has_alpha)
		       values[HISTOGRAM_ALPHA][alpha_bin]   += 1.0;
		 }
	     }
	   else
	     {
	       value = s[GRAY_PIX];
	       value_bin = bins*(double)value*scale;
	       if (mask_area)
		 values[HISTOGRAM_VALUE][value_bin] += (double) *m * scale;
	       else
		 values[HISTOGRAM_VALUE][value_bin] += 1.0;
	     }

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
histogram_histogram_range_float16 (int          start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  Histogram	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  double		 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;
  double 		 bin_width = 1.0/(double)bins;
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[histogram_p->channel][i];

      if (i >= start && i <= end)
	count += values[histogram_p->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += (i*bin_width) * values[histogram_p->channel][i];
      tmp += values[histogram_p->channel][i];
      if (median == -1 && tmp > count / 2)
	median = (i*bin_width);
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[histogram_p->channel][i] * (i*bin_width - mean) * (i*bin_width - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  histogram_p->mean = mean;
  histogram_p->std_dev = std_dev;
  histogram_p->median = median;
  histogram_p->pixels = pixels;
  histogram_p->count = count;
  histogram_p->percentile = percentile;

  if (htd->shell)
    histogram_dialog_update (htd, start, end);
}

static void
histogram_histogram_range_float (int        start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  Histogram	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  double		 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;
  double 		 bin_width = 1.0/(double)(bins);
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[histogram_p->channel][i];

      if (i >= start && i <= end)
	count += values[histogram_p->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += (i*bin_width) * values[histogram_p->channel][i];
      tmp += values[histogram_p->channel][i];
      if (median == -1 && tmp > count / 2)
	median = (i*bin_width);
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[histogram_p->channel][i] * (i*bin_width - mean) * (i*bin_width - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  histogram_p->mean = mean;
  histogram_p->std_dev = std_dev;
  histogram_p->median = median;
  histogram_p->pixels = pixels;
  histogram_p->count = count;
  histogram_p->percentile = percentile;

  if (htd->shell)
    histogram_dialog_update (htd, start, end);
}

static void
histogram_histogram_range_u16 (int          start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  Histogram	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  int 			 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[histogram_p->channel][i];

      if (i >= start && i <= end)
	count += values[histogram_p->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += i * 256 * values[histogram_p->channel][i];
      tmp += values[histogram_p->channel][i];
      if (median == -1 && tmp > count / 2)
	median = i * 256;
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[histogram_p->channel][i] * (256 * i - mean) * (256 * i - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  histogram_p->mean = mean;
  histogram_p->std_dev = std_dev;
  histogram_p->median = median;
  histogram_p->pixels = pixels;
  histogram_p->count = count;
  histogram_p->percentile = percentile;

  if (htd->shell)
    histogram_dialog_update (htd, start * 256, end * 256 + 255);
}

static void
histogram_histogram_range_u8 (int              start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  Histogram	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  int 			 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[histogram_p->channel][i];

      if (i >= start && i <= end)
	count += values[histogram_p->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += i * values[histogram_p->channel][i];
      tmp += values[histogram_p->channel][i];
      if (median == -1 && tmp > count / 2)
	median = i;
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[histogram_p->channel][i] * (i - mean) * (i - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  histogram_p->mean = mean;
  histogram_p->std_dev = std_dev;
  histogram_p->median = median;
  histogram_p->pixels = pixels;
  histogram_p->count = count;
  histogram_p->percentile = percentile;

  if (htd->shell)
    histogram_dialog_update (htd, start, end);
}

static void
histogram_histogram_range_bfp (int          start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  Histogram	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  int 			 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;
  HistogramPrivate *histogram_p;

  htd = (Histogram *) user_data;
  histogram_p = (HistogramPrivate *) htd->private_part;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[histogram_p->channel][i];

      if (i >= start && i <= end)
	count += values[histogram_p->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += i * 256 * values[histogram_p->channel][i];
      tmp += values[histogram_p->channel][i];
      if (median == -1 && tmp > count / 2)
	median = i * 256;
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[histogram_p->channel][i] * (256 * i - mean) * (256 * i - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  histogram_p->mean = mean;
  histogram_p->std_dev = std_dev;
  histogram_p->median = median;
  histogram_p->pixels = pixels;
  histogram_p->count = count;
  histogram_p->percentile = percentile;

  if (htd->shell)
    histogram_dialog_update (htd, start * 256, end * 256 + 255);
}


static void
histogram_dialog_update_float16 (Histogram *htd,
			      int                  start,
			      int                  end)
{
  histogram_dialog_update_float (htd, start, end);
}

static void
histogram_dialog_update_float (Histogram *htd,
			      int                  start,
			      int                  end)
{
  char text[32];
  HistogramPrivate *histogram_p;
  histogram_p = (HistogramPrivate *) htd->private_part;

  /*  mean  */
  sprintf (text, "%.4f", histogram_p->mean);
  gtk_label_set (GTK_LABEL (htd->info_labels[0]), text);

  /*  std dev  */
  sprintf (text, "%.4f", histogram_p->std_dev);
  gtk_label_set (GTK_LABEL (htd->info_labels[1]), text);

  /*  median  */
  sprintf (text, "%.4f", histogram_p->median);
  gtk_label_set (GTK_LABEL (htd->info_labels[2]), text);

  /*  pixels  */
  sprintf (text, "%8.1f", histogram_p->pixels);
  gtk_label_set (GTK_LABEL (htd->info_labels[3]), text);

  /*  intensity  */
  sprintf (text, "%f..%f", (start-1)/100.0, (end-1)/100.0);
  gtk_label_set (GTK_LABEL (htd->info_labels[4]), text);

  /*  count  */
  sprintf (text, "%8.1f", histogram_p->count);
  gtk_label_set (GTK_LABEL (htd->info_labels[5]), text);

  /*  percentile  */
  sprintf (text, "%2.2f", histogram_p->percentile * 100);
  gtk_label_set (GTK_LABEL (htd->info_labels[6]), text);
}

static void
histogram_dialog_update_u8 (Histogram *htd,
			      int                  start,
			      int                  end)
{
  char text[32];
  HistogramPrivate *histogram_p;
  histogram_p = (HistogramPrivate *) htd->private_part;

  /*  mean  */
  sprintf (text, "%3.1f", histogram_p->mean);
  gtk_label_set (GTK_LABEL (htd->info_labels[0]), text);

  /*  std dev  */
  sprintf (text, "%3.1f", histogram_p->std_dev);
  gtk_label_set (GTK_LABEL (htd->info_labels[1]), text);

  /*  median  */
  sprintf (text, "%3.1f", histogram_p->median);
  gtk_label_set (GTK_LABEL (htd->info_labels[2]), text);

  /*  pixels  */
  sprintf (text, "%8.1f", histogram_p->pixels);
  gtk_label_set (GTK_LABEL (htd->info_labels[3]), text);

  /*  intensity  */
  if (start == end)
    sprintf (text, "%d", start);
  else
    sprintf (text, "%d..%d", start, end);
  gtk_label_set (GTK_LABEL (htd->info_labels[4]), text);

  /*  count  */
  sprintf (text, "%8.1f", histogram_p->count);
  gtk_label_set (GTK_LABEL (htd->info_labels[5]), text);

  /*  percentile  */
  sprintf (text, "%2.2f", histogram_p->percentile * 100);
  gtk_label_set (GTK_LABEL (htd->info_labels[6]), text);
}

static void
histogram_dialog_update_u16 (Histogram *htd,
			      int                  start,
			      int                  end)
{
  histogram_dialog_update_u8 (htd, start, end);
}

static void
histogram_dialog_update_bfp (Histogram *htd,
			      int                  start,
			      int                  end)
{
  histogram_dialog_update_u8 (htd, start, end);
}
