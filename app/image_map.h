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
#ifndef __IMAGE_MAP_H__
#define __IMAGE_MAP_H__

/*  Image map apply function  */
typedef void * ImageMap;
typedef void (* ImageMapApplyFunc)  (PixelArea *, PixelArea *, void *);

/*  Image Map functions  */

/*  Successive image map apply functions can be called, but eventually
 *   MUST be followed with an image_map_commit or an image_map_abort call
 *   The image map is no longer valid after a call to commit or abort.
 */
ImageMap  image_map_create  (void *, CanvasDrawable *);
void      image_map_apply   (ImageMap, ImageMapApplyFunc, void *);
void      image_map_commit  (ImageMap);
void      image_map_abort   (ImageMap);
void      image_map_remove   (ImageMap);
Canvas*   image_map_get_src_canvas (ImageMap);

#endif /* __IMAGE_MAP_H__ */
