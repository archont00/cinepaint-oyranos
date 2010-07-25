/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_edit module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <string.h>
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "brushgenerated.h"
#include "brush_edit.h"
#include "depth/brush_select.h"
#include "actionarea.h"
#include "math.h"
#include "layout.h"
#include "rc.h"
#include "minimize.h"

static void brush_edit_close_callback (GtkWidget *w, void *data);
static gint brush_edit_preview_resize (GtkWidget *widget, GdkEvent *event, 
				       BrushEditGeneratedWindow *begw);

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", brush_edit_close_callback, NULL, NULL }
};

static void
update_brush_callback (GtkAdjustment *adjustment,
		       BrushEditGeneratedWindow *begw)
{
  if (begw->brush &&
      ((begw->radius_data->value  
	!= gimp_brush_generated_get_radius(begw->brush))
       || (begw->hardness_data->value
	   != gimp_brush_generated_get_hardness(begw->brush))
       || (begw->aspect_ratio_data->value
	   != gimp_brush_generated_get_aspect_ratio(begw->brush))
       || (begw->angle_data->value
	   !=gimp_brush_generated_get_angle(begw->brush))))
  {
    gimp_brush_generated_freeze (begw->brush);
    gimp_brush_generated_set_radius       (begw->brush,
					   begw->radius_data->value);
    gimp_brush_generated_set_hardness     (begw->brush,
					   begw->hardness_data->value);
    gimp_brush_generated_set_aspect_ratio (begw->brush,
					   begw->aspect_ratio_data->value);
    gimp_brush_generated_set_angle        (begw->brush,
					   begw->angle_data->value);
    gimp_brush_generated_thaw (begw->brush);
  }
}

static gint
brush_edit_delete_callback (GtkWidget *w,
			    BrushEditGeneratedWindow *begw)
{
  if (GTK_WIDGET_VISIBLE (w))
    gtk_widget_hide (w);
  return TRUE;
}

static void
brush_edit_clear_preview (BrushEditGeneratedWindow *begw)
{
  unsigned char * buf;
  int i;

  buf = (unsigned char *) g_malloc (sizeof (char) * begw->preview->requisition.width);

  /*  Set the buffer to white  */
  memset (buf, 255, begw->preview->requisition.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < begw->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (begw->preview), buf, 0, i,
			  begw->preview->requisition.width);

  g_free (buf);
}

static gint 
brush_edit_brush_dirty_callback(GimpBrush *brush,
				BrushEditGeneratedWindow *begw)
{
  int y, width, yend, ystart, xo;
  int scale;
  guchar *buf;
  gint w, h;
  Tag tag;

  brush_edit_clear_preview (begw);

  if (brush == NULL)
    return TRUE;

  tag = canvas_tag(brush->mask);
  w = canvas_width (brush->mask);
  h = canvas_height (brush->mask);
  scale = MAX(ceil(w/(float)begw->preview->requisition.width),
	      ceil(h/(float)begw->preview->requisition.height));

  ystart = 0;
  xo = begw->preview->requisition.width/2 - w/(2*scale);
  ystart = begw->preview->requisition.height/2 - h/(2*scale);
  yend = ystart + h/(scale);
  width = BOUNDS (w/scale, 0, begw->preview->requisition.width);

  buf = g_new (guchar, width);

  for (y = ystart; y < yend; y++)
  {
    (*display_brush_get_row_funcs (tag))(buf, brush->mask, scale *(y - ystart), width, scale);	
    gtk_preview_draw_row (GTK_PREVIEW (begw->preview), (guchar *)buf, xo, y,
			  width);
  }
  g_free(buf);
  if (begw->scale != scale)
  {
    char str[255];
    begw->scale = scale;
    g_snprintf(str, 200, "%d:1", scale);
    gtk_label_set(GTK_LABEL(begw->scale_label), str);
    gtk_widget_draw(begw->scale_label, NULL);
  }
  gtk_widget_draw(begw->preview, NULL);
  return TRUE;
}

void
brush_edit_generated_set_brush(BrushEditGeneratedWindow *begw,
			       GimpBrush *gbrush)
{
  GimpBrushGenerated *brush = 0;
  if (begw->brush == (GimpBrushGenerated*)gbrush)
    return;
  if (begw->brush)
  {
    gtk_signal_disconnect_by_data(GTK_OBJECT(begw->brush), begw);
    gtk_object_unref(GTK_OBJECT(begw->brush));
    begw->brush = NULL;
  }
  if (!gbrush || !GIMP_IS_BRUSH_GENERATED(gbrush))
  {
    begw->brush = NULL;
    if (GTK_WIDGET_VISIBLE (begw->shell))
      gtk_widget_hide (begw->shell);
    return;
  }
  brush = GIMP_BRUSH_GENERATED(gbrush);
  if (begw)
  {
    gtk_signal_connect(GTK_OBJECT (brush), "dirty",
		       GTK_SIGNAL_FUNC(brush_edit_brush_dirty_callback),
		       begw);
    begw->brush = NULL;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->radius_data),
			     gimp_brush_generated_get_radius (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->hardness_data),
			     gimp_brush_generated_get_hardness (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->angle_data),
			     gimp_brush_generated_get_angle (brush));
    gtk_adjustment_set_value(GTK_ADJUSTMENT(begw->aspect_ratio_data),
			     gimp_brush_generated_get_aspect_ratio(brush));
    begw->brush = brush;
    gtk_object_ref(GTK_OBJECT(begw->brush));
    brush_edit_brush_dirty_callback(GIMP_BRUSH(brush), begw);
  }
}

BrushEditGeneratedWindow *
brush_edit_generated_new ()
{
  BrushEditGeneratedWindow *begw ;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *table;



  begw = g_malloc (sizeof (BrushEditGeneratedWindow));
  begw->brush = NULL;

  begw->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (begw->shell), "generatedbrusheditor",PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (begw->shell), _("Brush Editor"));
  /* gtk_window_set_policy(GTK_WINDOW(begw->shell), FALSE, TRUE, FALSE); */
  gtk_widget_set_uposition(begw->shell, brush_edit_x, brush_edit_y);
  minimize_register(begw->shell);
  layout_connect_window_position(begw->shell, &brush_edit_x, &brush_edit_y, TRUE);
  layout_connect_window_visible(begw->shell, &brush_edit_visible);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (begw->shell)->vbox), vbox,
		      TRUE, TRUE, 0);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (begw->shell), "delete_event",
		      GTK_SIGNAL_FUNC (brush_edit_delete_callback),
		      begw);

/*  Populate the window with some widgets */

  /* brush's preview widget w/frame  */
  begw->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (begw->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), begw->frame, TRUE, TRUE, 0);

  begw->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (begw->preview), 125, 100);
  gtk_signal_connect_after (GTK_OBJECT(begw->frame), "size_allocate",
		       (GtkSignalFunc) brush_edit_preview_resize,
		       begw);
  gtk_container_add (GTK_CONTAINER (begw->frame), begw->preview);

  gtk_widget_show(begw->preview);
  gtk_widget_show(begw->frame);

  /* table for sliders/labels */
  begw->scale_label = gtk_label_new (_("-1:1"));
  gtk_box_pack_start (GTK_BOX (vbox), begw->scale_label, FALSE, FALSE, 0);
  begw->scale = -1;
  gtk_widget_show(begw->scale_label);
  /* table for sliders/labels */
  table = gtk_table_new(2, 4, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /* brush radius scale */
  label = gtk_label_new (_("Radius:"));
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
/*  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 0, 0); */
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  begw->radius_data = GTK_ADJUSTMENT (gtk_adjustment_new (10.0, 0.0, 300.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->radius_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 0, 1);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->radius_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush hardness scale */
  
  label = gtk_label_new (_("Hardness:"));
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  begw->hardness_data = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.01, 0.0));
  slider = gtk_hscale_new (begw->hardness_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 1, 2);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->hardness_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush aspect ratio scale */
  
  label = gtk_label_new (_("Aspect Ratio:"));
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  begw->aspect_ratio_data = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 1.0, 20.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->aspect_ratio_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 2, 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->aspect_ratio_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  /* brush angle scale */

  label = gtk_label_new (_("Angle:"));
  gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);
  begw->angle_data = GTK_ADJUSTMENT (gtk_adjustment_new (00.0, 0.0, 180.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->angle_data);
  gtk_table_attach_defaults(GTK_TABLE (table), slider, 1, 2, 3, 4);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->angle_data), "value_changed",
		      (GtkSignalFunc) update_brush_callback, begw);

  gtk_widget_show (label);
  gtk_widget_show (slider);

  gtk_table_set_row_spacings(GTK_TABLE (table), 3);
  gtk_table_set_col_spacing(GTK_TABLE (table), 0, 3);
  gtk_widget_show (table);

  /*  The action area  */
  action_items[0].label = _(action_items[0].label);
  action_items[0].user_data = begw;
  build_action_area (GTK_DIALOG (begw->shell), action_items, 1, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (begw->shell);


  return begw;
}

static gint 
brush_edit_preview_resize (GtkWidget *widget, 
			   GdkEvent *event, 
			   BrushEditGeneratedWindow *begw)
{
   gtk_preview_size (GTK_PREVIEW (begw->preview),
		     widget->allocation.width - 4,
		     widget->allocation.height - 4);
   
   /*  update the display  */   
   if (begw->brush)
     brush_edit_brush_dirty_callback(GIMP_BRUSH(begw->brush), begw);
   return FALSE;
}
 
static void
brush_edit_close_callback (GtkWidget *w, void *data)
{
  BrushEditGeneratedWindow *begw = (BrushEditGeneratedWindow *)data;
  if (GTK_WIDGET_VISIBLE (begw->shell))
    gtk_widget_hide (begw->shell);
}
