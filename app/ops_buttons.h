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

#ifndef __OPS_BUTTONS_H__
#define __OPS_BUTTONS_H__

#include "gdisplay.h"

/* Structures */

typedef struct OpsButton OpsButton;

typedef void (*OpsButtonCallback) (GtkWidget *widget,
				   gpointer   user_data);

struct OpsButton 
{
  gchar **xpm_data;          /* xpm data for the button in sensitive state */
  gchar **xpm_is_data;       /* xpm data for the button in insensitive state */
  OpsButtonCallback callback;   
  char *tooltip;               
#if GTK_MAJOR_VERSION > 1
  GdkPixbuf *pixmap;
  GdkPixbuf *is_pixmap;
#else
  GdkPixmap *pixmap;
  GdkPixmap *is_pixmap;
#endif
  GdkBitmap *mask;
  GdkBitmap *is_mask;
  GtkWidget *pixmapwid;      /* the pixmap widget */
  GtkWidget *widget;         /* the button widget */
};


/* Function declarations */

GtkWidget * ops_button_box_new        (GtkWidget *,      /* parent widget */
				       GtkTooltips *,    
				       OpsButton *);
GtkWidget * ops_button_box_new2        (GtkWidget *,      /* parent widget */
				       GtkTooltips *,    
				       OpsButton *,
                                        GDisplay *);
void ops_button_box_set_insensitive   (OpsButton *);
void ops_button_set_sensitive         (OpsButton *, gint);

#endif /* __OPS_BUTTONS_H__ */


