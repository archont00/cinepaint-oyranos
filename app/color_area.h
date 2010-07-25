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

#ifndef __COLOR_AREA_H__
#define __COLOR_AREA_H__

/*
 *  Functions
 */
GtkWidget * color_area_create (int        width,
			       int        height,
			       GdkPixmap *default_pixmap,
			       GdkPixmap *swap_pixmap);
void        color_area_update (void);
void        color_area_edit ();
/*
 *  Global variables
 */
extern int active_color;  /*  foreground (= 0) or background (= 1) */

#endif  /*  __COLOR_AREA_H__  */
