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
#ifndef __LAYER_PVT_H__
#define __LAYER_PVT_H__

#include "layer.h"

#include "boundary.h"
#include "drawable.h"
#include "canvas.h" 
#include "channel.h"

#include "channel_pvt.h"
#include "../lib/wire/c_typedefs.h"
#include "../lib/wire/dl_list.h"

struct GimpLayer
{
  CanvasDrawable drawable;

  int linked;				/* control linkage */
  int preserve_trans;			/* preserve transparency */

  LayerMask * mask;             /*  possible layer mask          */
  int apply_mask;               /*  controls mask application    */
  int edit_mask;                /*  edit mask or layer?          */
  int show_mask;                /*  show mask or layer?          */

  gfloat opacity;               /*  layer opacity                */
  int mode;                     /*  layer combination mode       */

  int gimage_id; 		/*  gimage id */
  /*  Floating selections  */
  struct
  {
    Canvas *backing_store;      /*  for obscured regions         */
    CanvasDrawable *drawable;     /*  floating sel is attached to  */
    int initial;                /*  is fs composited yet?        */

    int boundary_known;         /*  is the current boundary valid*/
    BoundSeg *segs;             /*  boundary of floating sel     */
    int num_segs;               /*  number of segs in boundary   */
  } fs;
  /* spline layer info */
  struct
  {
    void *spline_tool;	        /* each layer has own spline_tool structure */
    DL_list *splines;		/* list of splines attached to layer. */
  } sl;
};

struct GimpLayerClass
{
  CanvasDrawableClass parent_class;
};

struct GimpLayerMask
{
  GimpChannel drawable;

  Layer * layer;                 /*  ID of layer */
};

struct GimpLayerMaskClass
{
  GimpChannelClass parent_class;
};

#endif /* __LAYER_PVT_H__ */
