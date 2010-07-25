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
#ifndef __HISTOGRAM_H__
#define __HISTOGRAM_H__

#include "pixelarea.h"
#include "buildmenu.h"

#define HISTOGRAM_VALUE  0
#define HISTOGRAM_RED    1
#define HISTOGRAM_GREEN  2
#define HISTOGRAM_BLUE   3
#define HISTOGRAM_ALPHA  4

/*  Histogram information function  */
typedef struct Histogram Histogram;

struct Histogram
{
  /*  The calling procedure is responsible for showing this widget  */
  GtkWidget *histogram_widget;
  GtkWidget   *shell;
  GtkWidget   *info_labels[7];
  GtkWidget   *channel_menu;
  MenuItem *color_option_items;

  /*  Don't touch this :)  */
  void *     private_part;
};

typedef double  *Values;
typedef Values   HistogramValues[5];

typedef void (* HistogramInfoFunc)  (PixelArea *, PixelArea *, HistogramValues, void *);
extern HistogramInfoFunc histogram_histogram_info;
typedef void (* HistogramRangeCallback)  (int, int, int, HistogramValues, void *);
extern HistogramRangeCallback histogram_histogram_range;

/*  Histogram functions  */

Histogram *      histogram_create  (int, int, int, HistogramRangeCallback, void *);
void             histogram_free    (Histogram *);
void             histogram_update  (Histogram *, CanvasDrawable *, HistogramInfoFunc, void *);
void             histogram_range   (Histogram *, int, int);
void             histogram_channel (Histogram *, int);
HistogramValues *histogram_values  (Histogram *);
gint             histogram_bins (Histogram *histogram);
void histogram_histogram_funcs (Tag tag);

#endif /* __HISTOGRAM_H__ */




