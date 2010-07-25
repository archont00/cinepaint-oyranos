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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "color_panel.h"
#include "color_select.h"
#include "colormaps.h"
#include "paint_funcs_area.h"
#include "pixelrow.h"


#define EVENT_MASK  GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK

typedef struct ColorPanelPrivate ColorPanelPrivate;

struct ColorPanelPrivate
{
  GtkWidget *drawing_area;
  GdkGC *gc;

  ColorSelectP color_select;
  int color_select_active;
};

static void color_panel_draw (ColorPanel *);
static gint color_panel_events (GtkWidget *area, GdkEvent *event);
static void color_panel_select_callback (PixelRow *, ColorSelectState, void *);


ColorPanel *
color_panel_new (PixelRow * initial,
		 int        width,
		 int        height)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = g_new (ColorPanel, 1);
  private = g_new (ColorPanelPrivate, 1);
  private->color_select = NULL;
  private->color_select_active = 0;
  private->gc = NULL;
  color_panel->private_part = private;

  /*  set the initial color  */
  pixelrow_init (&color_panel->color, pixelrow_tag (initial), color_panel->color_data, 1);
  copy_row (initial, &color_panel->color);

  color_panel->color_panel_widget = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (color_panel->color_panel_widget), GTK_SHADOW_IN);

  /*  drawing area  */
  private->drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (private->drawing_area), width, height);
  gtk_widget_set_events (private->drawing_area, EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (private->drawing_area), "event",
		      (GtkSignalFunc) color_panel_events,
		      color_panel);
  gtk_object_set_user_data (GTK_OBJECT (private->drawing_area), color_panel);
  gtk_container_add (GTK_CONTAINER (color_panel->color_panel_widget), private->drawing_area);
  gtk_widget_show (private->drawing_area);

  return color_panel;
}

void
color_panel_free (ColorPanel *color_panel)
{
  ColorPanelPrivate *private;

  private = (ColorPanelPrivate *) color_panel->private_part;
  
  /* make sure we hide and free color_select */
  if (private->color_select)
    {
      color_select_hide (private->color_select);
      color_select_free (private->color_select);
    }
  
  if (private->gc)
    gdk_gc_destroy (private->gc);
  g_free (color_panel->private_part);
  g_free (color_panel);
}

static void
color_panel_draw (ColorPanel *color_panel)
{
  GtkWidget *widget;
  ColorPanelPrivate *private;
  GdkColor fg;

  private = (ColorPanelPrivate *) color_panel->private_part;
  widget = private->drawing_area;

  fg.pixel = old_color_pixel;
  store_color (&fg.pixel,
	       &color_panel->color);

  gdk_gc_set_foreground (private->gc, &fg);
  gdk_draw_rectangle (widget->window, private->gc, 1, 0, 0,
		      widget->allocation.width, widget->allocation.height);
}

static gint
color_panel_events (GtkWidget *widget,
		    GdkEvent  *event)
{
  GdkEventButton *bevent;
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = (ColorPanel *) gtk_object_get_user_data (GTK_OBJECT (widget));
  private = (ColorPanelPrivate *) color_panel->private_part;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!private->gc)
	private->gc = gdk_gc_new (widget->window);

      color_panel_draw (color_panel);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  if (! private->color_select)
	    {
	      private->color_select = color_select_new (&color_panel->color,
							color_panel_select_callback,
							color_panel,
							FALSE);
	      private->color_select_active = 1;
	    }
	  else
	    {
	      if (! private->color_select_active)
		color_select_show (private->color_select);
	      color_select_set_color (private->color_select,
				      &color_panel->color, 1);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_panel_select_callback (PixelRow * col,
			     ColorSelectState state,
			     void *client_data)
{
  ColorPanel *color_panel;
  ColorPanelPrivate *private;

  color_panel = (ColorPanel *) client_data;
  private = (ColorPanelPrivate *) color_panel->private_part;

  if (private->color_select)
    {
      switch (state) {
      case COLOR_SELECT_UPDATE:
	break;
      case COLOR_SELECT_OK:
        copy_row (col, &color_panel->color);
	color_panel_draw (color_panel);
	/* Fallthrough */
      case COLOR_SELECT_CANCEL:
	color_select_hide (private->color_select);
	private->color_select_active = 0;
      }
    }
}
