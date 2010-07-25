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
#ifndef __CONVERT_H__
#define __CONVERT_H__

#include "cms.h"

#include "procedural_db.h"
#include "tag.h"

/*  convert functions  */
void  convert_to_rgb        (void *);
void  convert_to_grayscale  (void *);
void  convert_to_indexed    (void *);
void  convert_image_precision (void *gimage, Precision  new_precision);

/* converts the image data to the colorspace given by new_profile
   only works if gimage has a profile
   lcms_intent is littlecms intent parameter */
void convert_image_colorspace (void *gimage_ptr, CMSProfile *new_profile,
			       int lcms_intent, int cms_flags);

/*  Procedure definition and marshalling function  */
extern ProcRecord convert_rgb_proc;
extern ProcRecord convert_grayscale_proc;
extern ProcRecord convert_indexed_proc;
extern ProcRecord convert_indexed_palette_proc;

#endif  /*  __CONVERT_H__  */
