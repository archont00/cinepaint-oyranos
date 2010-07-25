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
#ifndef __GIMAGE_MASK_H__
#define __GIMAGE_MASK_H__

#include "boundary.h"
#include "gimage.h"

/*  mask functions  */
int             gimage_mask_boundary      (GImage *, BoundSeg **, BoundSeg **, int *, int *);
int             gimage_mask_bounds        (GImage *, int *, int *, int *, int *);
void            gimage_mask_invalidate    (GImage *);
gfloat          gimage_mask_value         (GImage *, int, int);
int             gimage_mask_is_empty      (GImage *);
void            gimage_mask_translate     (GImage *, int, int);
Canvas * gimage_mask_extract      (GImage *, CanvasDrawable *, int, int);
Layer *         gimage_mask_float         (GImage *, CanvasDrawable *, int, int);
void            gimage_mask_clear         (GImage *);
void            gimage_mask_undo          (GImage *);
void            gimage_mask_invert        (GImage *);
void            gimage_mask_sharpen       (GImage *);
void            gimage_mask_all           (GImage *);
void            gimage_mask_none          (GImage *);
void            gimage_mask_feather       (GImage *, double);
void            gimage_mask_border        (GImage *, int);
void            gimage_mask_grow          (GImage *, int);
void            gimage_mask_shrink        (GImage *, int);
void            gimage_mask_layer_alpha   (GImage *, Layer *);
void            gimage_mask_layer_mask    (GImage *, Layer *);
void            gimage_mask_load          (GImage *, Channel *);
Channel *       gimage_mask_save          (GImage *);
int             gimage_mask_stroke        (GImage *, CanvasDrawable *);

#endif  /*  __GIMAGE_MASK_H__  */
