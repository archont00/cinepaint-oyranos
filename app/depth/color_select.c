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
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../actionarea.h"
#include "../color_select.h"
#include "../colormaps.h"
#include "displaylut.h"
#include "../errors.h"
#include "float16.h"
#include "../rc.h"
#include "../paint_funcs_area.h"
#include "../pixelrow.h"
#include "../layout.h"
#include "../minimize.h"

#define XY_DEF_WIDTH       192
#define XY_DEF_HEIGHT      192
#define Z_DEF_WIDTH        15
#define Z_DEF_HEIGHT       192
#define COLOR_AREA_WIDTH   74
#define COLOR_AREA_HEIGHT  20

#define COLOR_AREA_MASK GDK_EXPOSURE_MASK | \
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
			GDK_BUTTON1_MOTION_MASK | GDK_ENTER_NOTIFY_MASK

typedef enum {
  HUE = 0,
  SATURATION,
  VALUE,
  RED,
  GREEN,
  BLUE,
  HUE_SATURATION,
  HUE_VALUE,
  SATURATION_VALUE,
  RED_GREEN,
  RED_BLUE,
  GREEN_BLUE
} ColorSelectFillType;

typedef enum {
  UPDATE_VALUES = 1 << 0,
  UPDATE_POS = 1 << 1,
  UPDATE_XY_COLOR = 1 << 2,
  UPDATE_Z_COLOR = 1 << 3,
  UPDATE_NEW_COLOR = 1 << 4,
  UPDATE_ORIG_COLOR = 1 << 5,
  UPDATE_CALLER = 1 << 6
} ColorSelectUpdateType;

typedef struct ColorSelectFill ColorSelectFill;
typedef void (*ColorSelectFillUpdateProc) (ColorSelectFill *);

struct ColorSelectFill {
  unsigned char *buffer;
  int y;
  int width;
  int height;
  gfloat *values;
  ColorSelectFillUpdateProc update;
};

static void color_select_update (ColorSelectP, ColorSelectUpdateType);
static void color_select_update_caller (ColorSelectP);
static void color_select_update_values (ColorSelectP);
static void color_select_update_rgb_values (ColorSelectP);
static void color_select_update_hsv_values (ColorSelectP);
static void color_select_update_pos (ColorSelectP);
static void color_select_update_sliders (ColorSelectP, int);
static void color_select_format_entry (ColorSelectP, int, char *);
static void color_select_update_entries (ColorSelectP, int);
static void color_select_update_colors (ColorSelectP, int);

static void color_select_ok_callback (GtkWidget *, gpointer);
static void color_select_cancel_callback (GtkWidget *, gpointer);
static gint color_select_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static gint color_select_xy_expose (GtkWidget *, GdkEventExpose *, ColorSelectP);
static gint color_select_xy_events (GtkWidget *, GdkEvent *, ColorSelectP);
static gint color_select_z_expose (GtkWidget *, GdkEventExpose *, ColorSelectP);
static gint color_select_z_events (GtkWidget *, GdkEvent *, ColorSelectP);
static gint color_select_color_events (GtkWidget *, GdkEvent *);
static void color_select_slider_update (GtkAdjustment *, gpointer);
static void color_select_entry_update (GtkWidget *, gpointer);
static void color_select_toggle_update (GtkWidget *, gpointer);

static void color_select_image_fill (GtkWidget *, ColorSelectFillType, gfloat *);

static void color_select_draw_z_marker (ColorSelectP, GdkRectangle *);
static void color_select_draw_xy_marker (ColorSelectP, GdkRectangle *);

static void color_select_update_red (ColorSelectFill *);
static void color_select_update_green (ColorSelectFill *);
static void color_select_update_blue (ColorSelectFill *);
static void color_select_update_hue (ColorSelectFill *);
static void color_select_update_saturation (ColorSelectFill *);
static void color_select_update_value (ColorSelectFill *);
static void color_select_update_red_green (ColorSelectFill *);
static void color_select_update_red_blue (ColorSelectFill *);
static void color_select_update_green_blue (ColorSelectFill *);
static void color_select_update_hue_saturation (ColorSelectFill *);
static void color_select_update_hue_value (ColorSelectFill *);
static void color_select_update_saturation_value (ColorSelectFill *);

static ColorSelectFillUpdateProc update_procs[] =
{
  color_select_update_hue,
  color_select_update_saturation,
  color_select_update_value,
  color_select_update_red,
  color_select_update_green,
  color_select_update_blue,
  color_select_update_hue_saturation,
  color_select_update_hue_value,
  color_select_update_saturation_value,
  color_select_update_red_green,
  color_select_update_red_blue,
  color_select_update_green_blue,
};

static ActionAreaItem action_items[2] =
{
  { "OK", color_select_ok_callback, NULL, NULL },
  { "Cancel", color_select_cancel_callback, NULL, NULL },
};

ColorSelectP
color_select_new (PixelRow           * col,
		  ColorSelectCallback  callback,
		  void                *client_data,
		  int                  wants_updates)
{
  /*  static char *toggle_titles[6] = { "Hue", "Saturation", "Value", "Red", "Green", "Blue" }; */
  static char *toggle_titles[6] = { "H", "S", "V", "R", "G", "B" };
  static gfloat slider_max_vals[6] = { 360, 100, 100,
                                       1, 1, 1 };
  static gfloat slider_pincs[6]    = { 3.6, 1, 1,
                                       0.01, 0.01, 0.01};
  static gfloat slider_incs[6]     = { 0.36, 0.1, 0.1,
                                       0.001, 0.001, 0.001 };

  ColorSelectP csp;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *xy_frame;
  GtkWidget *z_frame;
  GtkWidget *colors_frame;
  GtkWidget *colors_hbox;
  GtkWidget *right_vbox;
  GtkWidget *table;
  GtkWidget *slider;
  GSList *group;
  char buffer[16];
  int i;

  csp = g_malloc_zero (sizeof (ColorSelect));

  csp->callback = callback;
  csp->client_data = client_data;
  csp->z_color_fill = HUE;
  csp->xy_color_fill = SATURATION_VALUE;
  csp->gc = NULL;
  csp->wants_updates = wants_updates;

  {
    PixelRow r;
    pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                   (guchar *)&csp->values[3], 1);
    copy_row (col, &r);
    pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                   (guchar *) csp->orig_values, 1);
    copy_row (col, &r);
  }
  
  color_select_update_hsv_values (csp);
  color_select_update_pos (csp);

  csp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (csp->shell), "color_selection", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (csp->shell), _("Color Selection"));
  gtk_window_set_policy (GTK_WINDOW (csp->shell), FALSE, FALSE, FALSE);
  gtk_widget_set_uposition (csp->shell, color_select_x, color_select_y);
  layout_connect_window_position(csp->shell, &color_select_x, &color_select_y, TRUE);
  layout_connect_window_visible(csp->shell, &color_visible);
  minimize_register(csp->shell);
  
  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (csp->shell), "delete_event",
		      (GtkSignalFunc) color_select_delete_callback, csp);
  
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (csp->shell)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  main_hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (main_hbox), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 2);
  gtk_widget_show (main_hbox);

  xy_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (xy_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (main_hbox), xy_frame, FALSE, FALSE, 2);
  gtk_widget_show (xy_frame);

  csp->xy_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (csp->xy_color), XY_DEF_WIDTH, XY_DEF_HEIGHT);
  gtk_widget_set_events (csp->xy_color, COLOR_AREA_MASK);
  gtk_signal_connect_after (GTK_OBJECT (csp->xy_color), "expose_event",
			    (GtkSignalFunc) color_select_xy_expose,
			    csp);
  gtk_signal_connect (GTK_OBJECT (csp->xy_color), "event",
		      (GtkSignalFunc) color_select_xy_events,
		      csp);
  gtk_container_add (GTK_CONTAINER (xy_frame), csp->xy_color);
  gtk_widget_show (csp->xy_color);

  z_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (z_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (main_hbox), z_frame, FALSE, FALSE, 2);
  gtk_widget_show (z_frame);

  csp->z_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (csp->z_color), Z_DEF_WIDTH, Z_DEF_HEIGHT);
  gtk_widget_set_events (csp->z_color, COLOR_AREA_MASK);
  gtk_signal_connect_after (GTK_OBJECT (csp->z_color), "expose_event",
			    (GtkSignalFunc) color_select_z_expose,
			    csp);
  gtk_signal_connect (GTK_OBJECT (csp->z_color), "event",
		      (GtkSignalFunc) color_select_z_events,
		      csp);
  gtk_container_add (GTK_CONTAINER (z_frame), csp->z_color);
  gtk_widget_show (csp->z_color);

  /*  The right vertical box with old/new color area and color space sliders  */
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (right_vbox), 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);

  /*  The old/new color area  */
  colors_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (colors_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (right_vbox), colors_frame, FALSE, FALSE, 0);
  gtk_widget_show (colors_frame);

  colors_hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (colors_frame), colors_hbox);
  gtk_widget_show (colors_hbox);

  csp->new_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (csp->new_color), COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (csp->new_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (csp->new_color), "event",
		      (GtkSignalFunc) color_select_color_events,
		      csp);
  gtk_object_set_user_data (GTK_OBJECT (csp->new_color), csp);
  gtk_box_pack_start (GTK_BOX (colors_hbox), csp->new_color, TRUE, TRUE, 0);
  gtk_widget_show (csp->new_color);

  csp->orig_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (csp->orig_color), COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (csp->orig_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (csp->orig_color), "event",
		      (GtkSignalFunc) color_select_color_events,
		      csp);
  gtk_object_set_user_data (GTK_OBJECT (csp->orig_color), csp);
  gtk_box_pack_start (GTK_BOX (colors_hbox), csp->orig_color, TRUE, TRUE, 0);
  gtk_widget_show (csp->orig_color);

  /*  The color space sliders, toggle buttons and entries  */
  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  group = NULL;
  for (i = 0; i < 6; i++)
    {
      toggle_titles[i] = _(toggle_titles[i]);
      csp->toggles[i] = gtk_radio_button_new_with_label (group, toggle_titles[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (csp->toggles[i]));
      gtk_table_attach (GTK_TABLE (table), csp->toggles[i],
			0, 1, i, i+1, GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_signal_connect (GTK_OBJECT (csp->toggles[i]), "toggled",
			  (GtkSignalFunc) color_select_toggle_update,
			  csp);
      gtk_widget_show (csp->toggles[i]);

      csp->slider_data[i] =
        GTK_ADJUSTMENT (gtk_adjustment_new (csp->values[i],
                                            0.0,
                                            slider_max_vals[i],
                                            slider_incs[i],
                                            slider_pincs[i],
                                            0.0));

      slider = gtk_hscale_new (csp->slider_data[i]);
#if GTK_MAJOR_VERSION > 1
      gtk_widget_set_usize (GTK_WIDGET (slider), 70, 0);
#endif
      gtk_table_attach (GTK_TABLE (table), slider, 1, 2, i, i+1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
      gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
      gtk_signal_connect (GTK_OBJECT (csp->slider_data[i]), "value_changed",
			  (GtkSignalFunc) color_select_slider_update,
			  csp);
      gtk_widget_show (slider);

      csp->entries[i] = gtk_entry_new ();
      color_select_format_entry (csp, i, buffer);
      gtk_entry_set_text (GTK_ENTRY (csp->entries[i]), buffer);
      gtk_widget_set_usize (GTK_WIDGET (csp->entries[i]), 70, 0);
      gtk_table_attach (GTK_TABLE (table), csp->entries[i],
			2, 3, i, i+1, GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_signal_connect (GTK_OBJECT (csp->entries[i]), "changed",
			  (GtkSignalFunc) color_select_entry_update,
			  csp);
      gtk_widget_show (csp->entries[i]);
    }

  /*  The action area  */
  action_items[0].user_data = csp;
  action_items[1].user_data = csp;
  if (csp->wants_updates)
    {
      action_items[0].label = _("Close");
      action_items[1].label = _("Revert to Old Color");
    }
  else
    {
      action_items[0].label = _("OK");
      action_items[1].label = _("Cancel");
    }
  build_action_area (GTK_DIALOG (csp->shell), action_items, 2, 0);

  color_select_image_fill (csp->z_color, csp->z_color_fill, csp->values);
  color_select_image_fill (csp->xy_color, csp->xy_color_fill, csp->values);

  gtk_widget_show (csp->shell);

  return csp;
}

void
color_select_show (ColorSelectP csp)
{
  if (csp)
    gtk_widget_show (csp->shell);
}

void
color_select_hide (ColorSelectP csp)
{
  if (csp)
    gtk_widget_hide (csp->shell);
}

void
color_select_free (ColorSelectP csp)
{
  if (csp)
    {
      gtk_widget_destroy (csp->shell);
      gdk_gc_destroy (csp->gc);
      g_free (csp);
    }
}

void
color_select_set_color (ColorSelectP csp,
			PixelRow *   col,
			int          set_current)
{
  if (csp)
    {
      PixelRow r;

      pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     (guchar *) csp->orig_values, 1);
      copy_row (col, &r);

      color_select_update_colors (csp, 1);

      if (set_current)
	{
          pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                         (guchar *) &csp->values[3], 1);
          copy_row (col, &r);

	  color_select_update_hsv_values (csp);
	  color_select_update_pos (csp);
	  color_select_update_sliders (csp, -1);
	  color_select_update_entries (csp, -1);
	  color_select_update_colors (csp, 0);

	  color_select_update (csp, UPDATE_Z_COLOR);
	  color_select_update (csp, UPDATE_XY_COLOR);
	}
    }
}

static void
color_select_update (ColorSelectP          csp,
		     ColorSelectUpdateType update)
{
  if (csp)
    {
      if (update & UPDATE_POS)
	color_select_update_pos (csp);

      if (update & UPDATE_VALUES)
	{
	  color_select_update_values (csp);
	  color_select_update_sliders (csp, -1);
	  color_select_update_entries (csp, -1);

	  if (!(update & UPDATE_NEW_COLOR))
	    color_select_update_colors (csp, 0);
	}

      if (update & UPDATE_XY_COLOR)
	{
	  color_select_image_fill (csp->xy_color, csp->xy_color_fill, csp->values);
	  gtk_widget_draw (csp->xy_color, NULL);
	}

      if (update & UPDATE_Z_COLOR)
	{
	  color_select_image_fill (csp->z_color, csp->z_color_fill, csp->values);
	  gtk_widget_draw (csp->z_color, NULL);
	}

      if (update & UPDATE_NEW_COLOR)
	color_select_update_colors (csp, 0);

      if (update & UPDATE_ORIG_COLOR)
	color_select_update_colors (csp, 1);

      /*if (update & UPDATE_CALLER)*/
      color_select_update_caller (csp);
    }
}

static void
color_select_update_caller (ColorSelectP csp)
{
  if (csp && csp->wants_updates && csp->callback)
    {
      PixelRow r;

      pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     (guchar *) &csp->values[3], 1);
      
      (* csp->callback) (&r,
			 COLOR_SELECT_UPDATE,
			 csp->client_data);
    }
}

static void
color_select_update_values (ColorSelectP csp)
{
  if (csp)
    {
      switch (csp->z_color_fill)
	{
	case RED:
	  csp->values[BLUE] = csp->pos[0];
	  csp->values[GREEN] = csp->pos[1];
	  csp->values[RED] = csp->pos[2];
	  break;
	case GREEN:
	  csp->values[BLUE] = csp->pos[0];
	  csp->values[RED] = csp->pos[1];
	  csp->values[GREEN] = csp->pos[2];
	  break;
	case BLUE:
	  csp->values[GREEN] = csp->pos[0];
	  csp->values[RED] = csp->pos[1];
	  csp->values[BLUE] = csp->pos[2];
	  break;
	case HUE:
	  csp->values[VALUE] = csp->pos[0] * 100;
	  csp->values[SATURATION] = csp->pos[1] * 100;
	  csp->values[HUE] = csp->pos[2] * 360;
	  break;
	case SATURATION:
	  csp->values[VALUE] = csp->pos[0] * 100;
	  csp->values[HUE] = csp->pos[1] * 360;
	  csp->values[SATURATION] = csp->pos[2] * 100;
	  break;
	case VALUE:
	  csp->values[SATURATION] = csp->pos[0] * 100;
	  csp->values[HUE] = csp->pos[1] * 360;
	  csp->values[VALUE] = csp->pos[2] * 100;
	  break;
	}

      switch (csp->z_color_fill)
	{
	case RED:
	case GREEN:
	case BLUE:
	  color_select_update_hsv_values (csp);
	  break;
	case HUE:
	case SATURATION:
	case VALUE:
	  color_select_update_rgb_values (csp);
	  break;
	}
    }
}

static void
color_select_update_rgb_values (ColorSelectP csp)
{
  float h, s, v;
  float f, p, q, t;

  if (csp)
    {
      h = csp->values[HUE];
      s = csp->values[SATURATION] / 100.0;
      v = csp->values[VALUE] / 100.0;

      if (s == 0)
	{
	  csp->values[RED] = v;
	  csp->values[GREEN] = v;
	  csp->values[BLUE] = v;
	}
      else
	{
	  if (h == 360)
	    h = 0;

	  h /= 60;
	  f = h - (int) h;
	  p = v * (1 - s);
	  q = v * (1 - (s * f));
	  t = v * (1 - (s * (1 - f)));

	  switch ((int) h)
	    {
	    case 0:
	      csp->values[RED] = v;
	      csp->values[GREEN] = t;
	      csp->values[BLUE] = p;
	      break;
	    case 1:
	      csp->values[RED] = q;
	      csp->values[GREEN] = v;
	      csp->values[BLUE] = p;
	      break;
	    case 2:
	      csp->values[RED] = p;
	      csp->values[GREEN] = v;
	      csp->values[BLUE] = t;
	      break;
	    case 3:
	      csp->values[RED] = p;
	      csp->values[GREEN] = q;
	      csp->values[BLUE] = v;
	      break;
	    case 4:
	      csp->values[RED] = t;
	      csp->values[GREEN] = p;
	      csp->values[BLUE] = v;
	      break;
	    case 5:
	      csp->values[RED] = v;
	      csp->values[GREEN] = p;
	      csp->values[BLUE] = q;
	      break;
	    }
	}
    }
}

static void
color_select_update_hsv_values (ColorSelectP csp)
{
  gfloat r, g, b;
  gfloat h, s, v;
  gfloat min, max;
  gfloat delta;

  if (csp)
    {
      r = csp->values[RED];
      g = csp->values[GREEN];
      b = csp->values[BLUE];

      if (r > g)
	{
	  if (r > b)
	    max = r;
	  else
	    max = b;

	  if (g < b)
	    min = g;
	  else
	    min = b;
	}
      else
	{
	  if (g > b)
	    max = g;
	  else
	    max = b;

	  if (r < b)
	    min = r;
	  else
	    min = b;
	}

      v = max;

      if (max != 0)
	s = (max - min) / max;
      else
	s = 0;

      if (s == 0)
	h = 0;
      else
	{
	  h = 0;
	  delta = max - min;
	  if (r == max)
	    h = (g - b) / (float) delta;
	  else if (g == max)
	    h = 2 + (b - r) / (float) delta;
	  else if (b == max)
	    h = 4 + (r - g) / (float) delta;
	  h *= 60;

	  if (h < 0)
	    h += 360;
	}

      csp->values[HUE] = h;
      csp->values[SATURATION] = s * 100;
      csp->values[VALUE] = v * 100;
    }
}

static void
color_select_update_pos (ColorSelectP csp)
{
  if (csp)
    {
      switch (csp->z_color_fill)
	{
	case RED:
	  csp->pos[0] = csp->values[BLUE];
	  csp->pos[1] = csp->values[GREEN];
	  csp->pos[2] = csp->values[RED];
	  break;
	case GREEN:
	  csp->pos[0] = csp->values[BLUE];
	  csp->pos[1] = csp->values[RED];
	  csp->pos[2] = csp->values[GREEN];
	  break;
	case BLUE:
	  csp->pos[0] = csp->values[GREEN];
	  csp->pos[1] = csp->values[RED];
	  csp->pos[2] = csp->values[BLUE];
	  break;
	case HUE:
	  csp->pos[0] = csp->values[VALUE] / 100;
	  csp->pos[1] = csp->values[SATURATION] / 100;
	  csp->pos[2] = csp->values[HUE] / 360;
	  break;
	case SATURATION:
	  csp->pos[0] = csp->values[VALUE] / 100;
	  csp->pos[1] = csp->values[HUE] / 360;
	  csp->pos[2] = csp->values[SATURATION] / 100;
	  break;
	case VALUE:
	  csp->pos[0] = csp->values[SATURATION] / 100;
	  csp->pos[1] = csp->values[HUE] / 360;
	  csp->pos[2] = csp->values[VALUE] / 100;
	  break;
	}
    }
}

static void
color_select_update_sliders (ColorSelectP csp,
			     int          skip)
{
  int i;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (i != skip)
	  {
	    csp->slider_data[i]->value = (gfloat) csp->values[i];

	    gtk_signal_handler_block_by_data (GTK_OBJECT (csp->slider_data[i]), csp);
	    gtk_signal_emit_by_name (GTK_OBJECT (csp->slider_data[i]), "value_changed");
	    gtk_signal_handler_unblock_by_data (GTK_OBJECT (csp->slider_data[i]), csp);
	  }
    }
}

static void 
color_select_format_entry  (
                            ColorSelectP csp,
                            int i,
                            char * buffer
                            )
{
  switch (i)
    {
    case 0:
    case 1:
    case 2:
      /* HSV get printed as floats always */
      sprintf (buffer, "%7.5f", csp->values[i]);
      break;
      
    case 3:
    case 4:
    case 5:
      /* RGB get formatted according to current default precision */
      switch (default_precision)
        {
        case PRECISION_U8:
          sprintf (buffer, "%-7d", (guint8) (csp->values[i] * 255));
          break;

        case PRECISION_BFP:
        case PRECISION_U16:
          sprintf (buffer, "%-7d", (guint16) (csp->values[i] * 65535));
          break;

        case PRECISION_FLOAT:
          sprintf (buffer, "%7.6f", csp->values[i]);
          break;
        
        case PRECISION_FLOAT16:
	  {
	    ShortsFloat u;
	    guint16 val16 = FLT16 (csp->values[i], u);
            sprintf (buffer, "%7.6f", FLT (val16, u));
	  }
          break;

        case PRECISION_NONE:
        default:
          sprintf (buffer, "---");
          break;          
        }
      break;

    default:
      sprintf (buffer, "---");
      break;          
    }
}


static void
color_select_update_entries (ColorSelectP csp,
			     int          skip)
{
  char buffer[16];
  int i;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (i != skip)
	  {
            color_select_format_entry (csp, i, buffer);
            gtk_signal_handler_block_by_data (GTK_OBJECT (csp->entries[i]), csp);
            gtk_entry_set_text (GTK_ENTRY (csp->entries[i]), buffer);
            gtk_signal_handler_unblock_by_data (GTK_OBJECT (csp->entries[i]), csp);
	  }
    }
}

static void
color_select_update_colors (ColorSelectP csp,
			    int          which)
{
  GdkWindow *window;
  GdkColor color;
  int width, height;
  PixelRow col;
  
  if (csp)
    {
      if (which)
	{
	  window = csp->orig_color->window;
          cp_pixel_to_gdkcolor(&color, &old_color_pixel);
          pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                         (guchar *) csp->orig_values, 1);
	}
      else
	{
	  window = csp->new_color->window;
          cp_pixel_to_gdkcolor(&color, &new_color_pixel);
          pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                         (guchar *) &csp->values[3], 1);
	}

      gdk_window_get_size (window, &width, &height);

      store_display_color (&color.pixel, &col);
      cp_pixel_to_gdkcolor(&color, &color.pixel);

      if (csp->gc)
	{
	  gdk_gc_set_foreground (csp->gc, &color);
	  gdk_draw_rectangle (window, csp->gc, 1,
			      0, 0, width, height);
	}
    }
}

static void
color_select_ok_callback (GtkWidget *w,
			  gpointer   client_data)
{
  ColorSelectP csp;

  csp = (ColorSelectP) client_data;
  if (csp)
    {
      if (csp->callback)
        {
          PixelRow r;

          pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                         (guchar *) &csp->values[3], 1);

          (* csp->callback) (&r,
                             COLOR_SELECT_OK,
                             csp->client_data);
        }
    }
}

static gint
color_select_delete_callback (GtkWidget *w,
			      GdkEvent  *e,
			      gpointer   client_data)
{
  color_select_cancel_callback (w, client_data);

  return TRUE;
}
  

static void
color_select_cancel_callback (GtkWidget *w,
			      gpointer   client_data)
{
  ColorSelectP csp;

  csp = (ColorSelectP) client_data;
  if (csp)
    {
      if (csp->callback)
        {
          PixelRow r;
          
          pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                         (guchar *) csp->orig_values, 1);
          
          (* csp->callback) (&r,
                             COLOR_SELECT_CANCEL,
                             csp->client_data);
        }
    }
}

static gint
color_select_xy_expose (GtkWidget      *widget,
			GdkEventExpose *event,
			ColorSelectP    csp)
{
  if (!csp->gc)
    csp->gc = gdk_gc_new (widget->window);

  color_select_draw_xy_marker (csp, &event->area);

  return FALSE;
}

static gint
color_select_xy_events (GtkWidget    *widget,
			GdkEvent     *event,
			ColorSelectP  csp)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (bevent->x) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 1 - (bevent->y) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 1)
	csp->pos[0] = 1;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 1)
	csp->pos[1] = 1;

      gdk_pointer_grab (csp->xy_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      color_select_draw_xy_marker (csp, NULL);

      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (bevent->x) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 1 - (bevent->y) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 1)
	csp->pos[0] = 1;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 1)
	csp->pos[1] = 1;

      gdk_pointer_ungrab (bevent->time);
      color_select_draw_xy_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (mevent->x) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 1 - (mevent->y) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 1)
	csp->pos[0] = 1;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 1)
	csp->pos[1] = 1;

      color_select_draw_xy_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
color_select_z_expose (GtkWidget      *widget,
		       GdkEventExpose *event,
		       ColorSelectP    csp)
{
  if (!csp->gc)
    csp->gc = gdk_gc_new (widget->window);

  color_select_draw_z_marker (csp, &event->area);

  return FALSE;
}

static gint
color_select_z_events (GtkWidget    *widget,
		       GdkEvent     *event,
		       ColorSelectP  csp)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 1 - (bevent->y) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 1)
	csp->pos[2] = 1;

      gdk_pointer_grab (csp->z_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 1 - (bevent->y) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 1)
	csp->pos[2] = 1;

      gdk_pointer_ungrab (bevent->time);
      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES | UPDATE_XY_COLOR);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 1 - (mevent->y) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 1)
	csp->pos[2] = 1;

      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES | UPDATE_XY_COLOR);
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
color_select_color_events (GtkWidget *widget,
			   GdkEvent  *event)
{
  ColorSelectP csp;

  csp = (ColorSelectP) gtk_object_get_user_data (GTK_OBJECT (widget));
  if (!csp)
    return FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!csp->gc)
	csp->gc = gdk_gc_new (widget->window);

      if (widget == csp->new_color)
	color_select_update (csp, UPDATE_NEW_COLOR);
      else if (widget == csp->orig_color)
	color_select_update (csp, UPDATE_ORIG_COLOR);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_select_slider_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  ColorSelectP csp;
  gfloat old_values[6];
  int update_z_marker;
  int update_xy_marker;
  int i, j;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (csp->slider_data[i] == adjustment)
	  break;

      for (j = 0; j < 6; j++)
	old_values[j] = csp->values[j];

      csp->values[i] = adjustment->value;

      if ((i >= HUE) && (i <= VALUE))
	color_select_update_rgb_values (csp);
      else if ((i >= RED) && (i <= BLUE))
	color_select_update_hsv_values (csp);
      color_select_update_sliders (csp, i);
      color_select_update_entries (csp, -1);

      update_z_marker = 0;
      update_xy_marker = 0;
      for (j = 0; j < 6; j++)
	{
	  if (j == csp->z_color_fill)
	    {
	      if (old_values[j] != csp->values[j])
		update_z_marker = 1;
	    }
	  else
	    {
	      if (old_values[j] != csp->values[j])
		update_xy_marker = 1;
	    }
	}

      if (update_z_marker)
	{
	  color_select_draw_z_marker (csp, NULL);
	  color_select_update (csp, UPDATE_POS | UPDATE_XY_COLOR);
	  color_select_draw_z_marker (csp, NULL);
	}
      else
	{
	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);

	  color_select_update (csp, UPDATE_POS);

	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);
	}

      color_select_update (csp, UPDATE_NEW_COLOR);
    }
}

static void
color_select_entry_update (GtkWidget *w,
			   gpointer   data)
{
  ColorSelectP csp;
  gfloat old_values[6];
  int update_z_marker;
  int update_xy_marker;
  int i, j;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (csp->entries[i] == w)
	  break;

      for (j = 0; j < 6; j++)
	old_values[j] = csp->values[j];

      {
        gfloat val;

        val = atof (gtk_entry_get_text (GTK_ENTRY (csp->entries[i])));

        switch (i)
          {
          case 0:
          case 1:
          case 2:
            /* HSV get read as floats always */
            break;
      
          case 3:
          case 4:
          case 5:
            /* RGB get read according to current default precision */
            switch (default_precision)
              {
              case PRECISION_U8:
                val = val / 255.0;
                break;

              case PRECISION_BFP:
              case PRECISION_U16:
                val = val / 65535.0;
                break;

              case PRECISION_FLOAT:
                break;
              
	      case PRECISION_FLOAT16:
		{
		  ShortsFloat u;
		  guint16 val16 = FLT16( val, u);
		  val = FLT (val16, u);
		}
                break;
                
              case PRECISION_NONE:
              default:
                val = 0;
                break;          
              }
            break;
          }

        csp->values[i] = val;
      }
      
      if (csp->values[i] == old_values[i])
	return;

      if ((i >= HUE) && (i <= VALUE))
	color_select_update_rgb_values (csp);
      else if ((i >= RED) && (i <= BLUE))
	color_select_update_hsv_values (csp);
      color_select_update_entries (csp, i);
      color_select_update_sliders (csp, -1);

      update_z_marker = 0;
      update_xy_marker = 0;
      for (j = 0; j < 6; j++)
	{
	  if (j == csp->z_color_fill)
	    {
	      if (old_values[j] != csp->values[j])
		update_z_marker = 1;
	    }
	  else
	    {
	      if (old_values[j] != csp->values[j])
		update_xy_marker = 1;
	    }
	}

      if (update_z_marker)
	{
	  color_select_draw_z_marker (csp, NULL);
	  color_select_update (csp, UPDATE_POS | UPDATE_XY_COLOR);
	  color_select_draw_z_marker (csp, NULL);
	}
      else
	{
	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);

	  color_select_update (csp, UPDATE_POS);

	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);
	}

      color_select_update (csp, UPDATE_NEW_COLOR);
    }
}

static void
color_select_toggle_update (GtkWidget *w,
			    gpointer   data)
{
  ColorSelectP csp;
  ColorSelectFillType type = HUE;
  int i;

  if (!GTK_TOGGLE_BUTTON (w)->active)
    return;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (w == csp->toggles[i])
	  type = (ColorSelectFillType) i;

      switch (type)
	{
	case HUE:
	  csp->z_color_fill = HUE;
	  csp->xy_color_fill = SATURATION_VALUE;
	  break;
	case SATURATION:
	  csp->z_color_fill = SATURATION;
	  csp->xy_color_fill = HUE_VALUE;
	  break;
	case VALUE:
	  csp->z_color_fill = VALUE;
	  csp->xy_color_fill = HUE_SATURATION;
	  break;
	case RED:
	  csp->z_color_fill = RED;
	  csp->xy_color_fill = GREEN_BLUE;
	  break;
	case GREEN:
	  csp->z_color_fill = GREEN;
	  csp->xy_color_fill = RED_BLUE;
	  break;
	case BLUE:
	  csp->z_color_fill = BLUE;
	  csp->xy_color_fill = RED_GREEN;
	  break;
	default:
	  break;
	}

      color_select_update (csp, UPDATE_POS);
      color_select_update (csp, UPDATE_Z_COLOR | UPDATE_XY_COLOR);
    }
}

static void
color_select_image_fill (GtkWidget           *preview,
			 ColorSelectFillType  type,
			 gfloat              *values)
{
  ColorSelectFill csf;
  int height;

  csf.buffer = g_malloc (preview->requisition.width * 3);

  csf.update = update_procs[type];

  csf.y = -1;
  csf.width = preview->requisition.width;
  csf.height = preview->requisition.height;
  csf.values = values;

  height = csf.height;
  if (height > 0)
    while (height--)
      {
	(* csf.update) (&csf);
	gtk_preview_draw_row (GTK_PREVIEW (preview), csf.buffer, 0, csf.y, csf.width);
      }

  g_free (csf.buffer);
}

static void
color_select_draw_z_marker (ColorSelectP  csp,
			    GdkRectangle *clip)
{
  int width;
  int height;
  int y;
  int minx;
  int miny;

  if (csp->gc)
    {
      y = (Z_DEF_HEIGHT - 1) - ((Z_DEF_HEIGHT - 1) * csp->pos[2]);
      width = csp->z_color->requisition.width;
      height = csp->z_color->requisition.height;
      minx = 0;
      miny = 0;
      if (width <= 0)
	return;
      if (clip)
	{
	  width  = MIN(width,  clip->x + clip->width);
	  height = MIN(height, clip->y + clip->height);
	  minx   = MAX(0, clip->x);
	  miny   = MAX(0, clip->y);
	}

      if (y >= miny && y < height)
	{
	  gdk_gc_set_function (csp->gc, GDK_INVERT);
	  gdk_draw_line (csp->z_color->window, csp->gc, minx, y, width - 1, y);
	  gdk_gc_set_function (csp->gc, GDK_COPY);
	}
    }
}

static void
color_select_draw_xy_marker (ColorSelectP  csp,
			     GdkRectangle *clip)
{
  int width;
  int height;
  int x, y;
  int minx, miny;

  if (csp->gc)
    {
      x = ((XY_DEF_WIDTH - 1) * csp->pos[0]);
      y = (XY_DEF_HEIGHT - 1) - ((XY_DEF_HEIGHT - 1) * csp->pos[1]);
      width = csp->xy_color->requisition.width;
      height = csp->xy_color->requisition.height;
      minx = 0;
      miny = 0;
      if ((width <= 0) || (height <= 0))
	return;

      gdk_gc_set_function (csp->gc, GDK_INVERT);

      if (clip)
        {
	  width  = MIN(width,  clip->x + clip->width);
	  height = MIN(height, clip->y + clip->height);
	  minx   = MAX(0, clip->x);
	  miny   = MAX(0, clip->y);
	}

      if (y >= miny && y < height)
	gdk_draw_line (csp->xy_color->window, csp->gc, minx, y, width - 1, y);

      if (x >= minx && x < width)
	gdk_draw_line (csp->xy_color->window, csp->gc, x, miny, x, height - 1);

      gdk_gc_set_function (csp->gc, GDK_COPY);
    }
}

static void
color_select_update_red (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat r;
  int i;

  p = csf->buffer;

  csf->y += 1;
  r = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (r < 0)
    r = 0;
  if (r > 1)
    r = 1;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = display_u8_from_float(r);
      *p++ = 0;
      *p++ = 0;
#if 0
      *p++ = r * 255;
      *p++ = 0;
      *p++ = 0;
#endif
    }
}

static void
color_select_update_green (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat g;
  int i;

  p = csf->buffer;

  csf->y += 1;
  g = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (g < 0)
    g = 0;
  if (g > 1)
    g = 1;

  for (i = 0; i < csf->width; i++)
    {
#if 0
      *p++ = 0;
      *p++ = g * 255;
      *p++ = 0;
#endif
      *p++ = 0;
      *p++ = display_u8_from_float (g);
      *p++ = 0;
    }
}

static void
color_select_update_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat b;
  int i;

  p = csf->buffer;

  csf->y += 1;
  b = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (b < 0)
    b = 0;
  if (b > 1)
    b = 1;

  for (i = 0; i < csf->width; i++)
    {
#if 0
      *p++ = 0;
      *p++ = 0;
      *p++ = b * 255;
#endif
      *p++ = 0;
      *p++ = 0;
      *p++ = display_u8_from_float (b);
    }
}

static void
color_select_update_hue (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat h, f;
  gfloat r, g, b;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = csf->y * 360 / (gfloat) csf->height;

  h = 360 - h;

  if (h < 0)
    h = 0;
  if (h >= 360)
    h = 0;

  h /= 60;
  f = (h - (int) h);

  r = g = b = 0;

  switch ((int) h)
    {
    case 0:
      r = 1;
      g = f;
      b = 0;
      break;
    case 1:
      r = 1 - f;
      g = 1;
      b = 0;
      break;
    case 2:
      r = 0;
      g = 1;
      b = f;
      break;
    case 3:
      r = 0;
      g = 1 - f;
      b = 1;
      break;
    case 4:
      r = f;
      g = 0;
      b = 1;
      break;
    case 5:
      r = 1;
      g = 0;
      b = 1 - f;
      break;
    }

  for (i = 0; i < csf->width; i++)
    {
#if 0
      *p++ = r * 255;
      *p++ = g * 255;
      *p++ = b * 255;
#endif
      *p++ = display_u8_from_float (r);
      *p++ = display_u8_from_float (g);
      *p++ = display_u8_from_float (b);
    }
}

static void
color_select_update_saturation (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat s;
  int i;

  p = csf->buffer;

  csf->y += 1;
  s = csf->y / (gfloat) csf->height;

  if (s < 0)
    s = 0;
  if (s > 1)
    s = 1;

  s = 1 - s;

  for (i = 0; i < csf->width; i++)
    {
#if 0
      *p++ = s * 255;
      *p++ = s * 255;
      *p++ = s * 255;
#endif
      *p++ = display_u8_from_float (s);
      *p++ = display_u8_from_float (s);
      *p++ = display_u8_from_float (s);
    }
}

static void
color_select_update_value (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat v;
  int i;

  p = csf->buffer;

  csf->y += 1;
  v = csf->y / (gfloat) csf->height;

  if (v < 0)
    v = 0;
  if (v > 1)
    v = 1;

  v = 1 - v;

  for (i = 0; i < csf->width; i++)
    {
#if 0
      *p++ = v * 255;
      *p++ = v * 255;
      *p++ = v * 255;
#endif
      *p++ = display_u8_from_float (v);
      *p++ = display_u8_from_float (v);
      *p++ = display_u8_from_float (v);
    }
}

static void
color_select_update_red_green (ColorSelectFill *csf)
{
  unsigned char *p;
  int i;
  gfloat r, b;
  gfloat g, dg;

  p = csf->buffer;

  csf->y += 1;
  b = csf->values[BLUE];
  r = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (r < 0)
    r = 0;
  if (r > 1)
    r = 1;

  g = 0;
  dg = 1.0 / csf->width;

  r *= 255;
  b *= 255;
  dg *= 255;
  
  for (i = 0; i < csf->width; i++)
    {

      *p++ = display_u8_from_float (r/255.0);
      *p++ = display_u8_from_float (g/255.0);
      *p++ = display_u8_from_float (b/255.0);
#if 0
      *p++ = r;
      *p++ = g;
      *p++ = b;
#endif
      g += dg;
    }
}

static void
color_select_update_red_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  int i;
  gfloat r, g;
  gfloat b, db;

  p = csf->buffer;

  csf->y += 1;
  g = csf->values[GREEN];
  r = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (r < 0)
    r = 0;
  if (r > 1)
    r = 1;

  b = 0;
  db = 1.0 / csf->width;

  r *= 255;
  g *= 255;
  db *= 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = display_u8_from_float (r/255.0);
      *p++ = display_u8_from_float (g/255.0);
      *p++ = display_u8_from_float (b/255.0);
#if 0
      *p++ = r;
      *p++ = g;
      *p++ = b;
#endif
      b += db;
    }
}

static void
color_select_update_green_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  int i;
  gfloat g, r;
  gfloat b, db;

  p = csf->buffer;

  csf->y += 1;
  r = csf->values[RED];
  g = (csf->height - csf->y + 1) / (gfloat) csf->height;

  if (g < 0)
    g = 0;
  if (g > 1)
    g = 1;

  b = 0;
  db = 1.0 / csf->width;

  r *= 255;
  g *= 255;
  db *= 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = display_u8_from_float (r/255.0);
      *p++ = display_u8_from_float (g/255.0);
      *p++ = display_u8_from_float (b/255.0);
#if 0
      *p++ = r;
      *p++ = g;
      *p++ = b;
#endif
      b += db;
    }
}

static void
color_select_update_hue_saturation (ColorSelectFill *csf)
{
  unsigned char *p;
  float h, v, s, ds;
  gfloat f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / (gfloat) csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h);

  s = 0;
  ds = 1.0 / csf->width;

/*  v = csf->values[VALUE] / 100.0 * 255; */
  v = csf->values[VALUE] / 100.0;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * (1 - s);
#endif
          *p++ = display_u8_from_float ( v );
          *p++ = display_u8_from_float ( v * (1 - (s * (1 - f))) );
          *p++ = display_u8_from_float ( v * (1 - s) );

	  s += ds;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s * f);
	  *p++ = v;
	  *p++ = v * (1 - s);
#endif
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));

	  s += ds;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));

	  s += ds;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);

	  s += ds;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * ((1 - s));
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  *p++ = display_u8_from_float (v * ((1 - s)));
	  *p++ = display_u8_from_float (v);

	  s += ds;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
#endif
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));

	  s += ds;
	}
      break;
    }
}

static void
color_select_update_hue_value (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat h, v, dv, s;
  gfloat f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / (gfloat) csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h);

  v = 0;
  dv = (1.0 / csf->width);

  /*s = csf->values[SATURATION] / 100.0 * 255*/;
  s = csf->values[SATURATION] / 100.0;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * (1 - s);
#endif
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  *p++ = display_u8_from_float (v * (1 - s));

	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s * f);
	  *p++ = v;
	  *p++ = v * (1 - s);
#endif
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);
	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * (1 - s);
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v);
	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
#endif
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));

	  v += dv;
	}
      break;
    }
}

static void
color_select_update_saturation_value (ColorSelectFill *csf)
{
  unsigned char *p;
  gfloat h, v, dv, s;
  gfloat f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  s = (float) csf->y / (gfloat) csf->height;

  if (s < 0)
    s = 0;
  if (s > 1)
    s = 1;

  s = 1 - s;

  h = (float) csf->values[HUE];
  if (h >= 360)
    h -= 360;
  h /= 60;
  f = (h - (int) h);

  v = 0;
  dv = 1.0 / csf->width;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * (1 - s);
#endif
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  *p++ = display_u8_from_float (v * (1 - s));
	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s * f);
	  *p++ = v;
	  *p++ = v * (1 - s);
#endif
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v;
	  *p++ = v * (1 - (s * (1 - f)));
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));

	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));
	  *p++ = display_u8_from_float (v);

	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v * (1 - (s * (1 - f)));
	  *p++ = v * ((1 - s));
	  *p++ = v;
#endif
	  *p++ = display_u8_from_float (v * (1 - (s * (1 - f))));
	  *p++ = display_u8_from_float (v * ((1 - s)));
	  *p++ = display_u8_from_float (v);

	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
#if 0
	  *p++ = v;
	  *p++ = v * (1 - s);
	  *p++ = v * (1 - s * f);
#endif
	  *p++ = display_u8_from_float (v);
	  *p++ = display_u8_from_float (v * (1 - s));
	  *p++ = display_u8_from_float (v * (1 - s * f));

	  v += dv;
	}
      break;
    }
}

