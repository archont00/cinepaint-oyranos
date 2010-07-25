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

#ifndef GIMPGRADIENTMENU_H
#define GIMPGRADIENTMENU_H

#include "config.h"

#include <glib.h>		/* Include early for obscure Win32
				   build reasons */

#ifndef GTK_ENABLE_BROKEN
#define GTK_ENABLE_BROKEN
#endif
#include <gtk/gtk.h>
#include "gimpmenu.h"


GtkWidget * gimp_gradient_select_widget           (gchar      *gname,
						   gchar      *igradient, 
						   GimpRunGradientCallback  cback,
						   gpointer    data);
void      gimp_gradient_select_widget_close_popup (GtkWidget  *widget);
void      gimp_gradient_select_widget_set_popup   (GtkWidget  *widget,
						   gchar      *gname);

gchar   * gimp_interactive_selection_gradient     (gchar      *dialogtitle,
						   gchar      *gradient_name,
						   gint        sample_sz,
						   GimpRunGradientCallback  callback,
						   gpointer    data);
  
GtkWidget * gimp_gradient_select_widget           (gchar      *gname,
						   gchar      *igradient, 
						   GimpRunGradientCallback  cback,
						   gpointer    data);
gchar    * gimp_interactive_selection_brush    (gchar     *dialogname,
						gchar     *brush_name,
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode,
						GimpRunBrushCallback  callback,
						gpointer   data);
  
GtkWidget * gimp_brush_select_widget           (gchar     *dname,
						gchar     *ibrush, 
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode,
						GimpRunBrushCallback  cback,
						gpointer   data);
  
void      gimp_brush_select_widget_set_popup   (GtkWidget *widget,
						gchar     *pname,
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode);
void      gimp_brush_select_widget_close_popup (GtkWidget *widget);

#endif /* GIMPGRADIENTMENU_H */

