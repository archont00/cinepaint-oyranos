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
#ifndef __IMAGE_RENDER_H__
#define __IMAGE_RENDER_H__

#include "gdisplay.h"

/*  Transparency representation  */

#define LIGHT_CHECKS    0
#define GRAY_CHECKS     1
#define DARK_CHECKS     2
#define WHITE_ONLY      3
#define GRAY_ONLY       4
#define BLACK_ONLY      5

#define SMALL_CHECKS    0
#define MEDIUM_CHECKS   1
#define LARGE_CHECKS    2

/* out-of-gammut warning colour for 8bit RGB */
#define GAMMUT_WARN_R 255
#define GAMMUT_WARN_G 0
#define GAMMUT_WARN_B 255

/*  Functions  */
void render_setup (int check_type,
		   int check_size);
void render_free  (void);
void render_image (GDisplay *gdisp,
		   int       x,
		   int       y,
		   int       w,
		   int       h);

/* image exposure functions */
float image_render_get_gamma();
float image_render_get_expose();
float image_render_get_force_expose();
float image_render_get_clamp();

void image_render_set_gamma(gfloat val);
void image_render_set_expose(gfloat val);
void image_render_set_force_expose(guint val);
void image_render_set_force_clamp(guint val);
void image_render_set_visible_channels(GImage *gimage,
				       int       x,
				       int       y,
				       int       w,
				       int       h);

/*
 *  Extern variables
 */
extern guchar *temp_buf;
extern guchar *blend_dark_check;
extern guchar *blend_light_check;


#endif  /*  __IMAGE_RENDER_H__  */
