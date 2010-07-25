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
#ifndef __COLORMAPS_H__
#define __COLORMAPS_H__

#include "gimage.h"               /* For the image types  */

extern GdkVisual *g_visual;
extern GdkColormap *g_cmap;

/*  Pixel values of black and white  */
extern gulong g_black_pixel;
extern gulong g_gray_pixel;
extern gulong g_white_pixel;
extern gulong g_color_pixel;
extern gulong g_normal_guide_pixel;
extern gulong g_active_guide_pixel;

/*  Foreground and Background colors  */
extern gulong foreground_pixel;
extern gulong background_pixel;

/*  Old and New colors  */
extern gulong old_color_pixel;
extern gulong new_color_pixel;

/*  Colormap entries reserved for color cycled marching ants--optional  */
extern gulong  marching_ants_pixels[8];

extern GtkDitherInfo *red_ordered_dither;
extern GtkDitherInfo *green_ordered_dither;
extern GtkDitherInfo *blue_ordered_dither;
extern GtkDitherInfo *gray_ordered_dither;

extern guchar ***ordered_dither_matrix;

extern gulong *g_lookup_red;
extern gulong *g_lookup_green;
extern gulong *g_lookup_blue;

extern gulong *color_pixel_vals;
extern gulong *gray_pixel_vals;

gulong get_color (PixelRow *);
void   store_color (gulong *pixel, PixelRow *);
void   store_display_color (gulong *pixel, PixelRow *);
void   get_standard_colormaps (void);

void   cp_pixel_to_gdkcolor(GdkColor *c, const gulong *pixel);


#endif  /*  __COLORMAPS_H__  */
