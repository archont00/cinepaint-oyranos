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
#ifndef __LAYER_H__
#define __LAYER_H__

#include "drawable.h"

#include "boundary.h"
#include "channel.h"

typedef enum
{
  APPLY,
  DISCARD
} MaskApplyMode;
/*#define APPLY   0
#define DISCARD 1 */

typedef enum
{
  WhiteMask,
  BlackMask,
  AlphaMask,
  AuxMask
} AddMaskType;


/* structure declarations */

#define GIMP_LAYER(obj)        GTK_CHECK_CAST (obj, gimp_layer_get_type (), GimpLayer)
#define GIMP_LAYER_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_layer_get_type(), GimpLayerClass)
#define GIMP_IS_LAYER(obj)     GTK_CHECK_TYPE (obj, gimp_layer_get_type())

#define GIMP_LAYER_MASK(obj)         GTK_CHECK_CAST (obj, gimp_layer_mask_get_type (), GimpLayerMask)
#define GIMP_LAYER_MASK_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_layer_mask_get_type(), GimpLayerMaskClass)
#define GIMP_IS_LAYER_MASK(obj)      GTK_CHECK_TYPE (obj, gimp_layer_mask_get_type())


guint gimp_layer_get_type (void);
guint gimp_layer_mask_get_type (void);

/*  Special undo types  */



struct LayerUndo
{
  Layer * layer;     /*  the actual layer         */
  int prev_position; /*  former position in list  */
  Layer * prev_layer;    /*  previous active layer    */
  int undo_type;     /*  is this a new layer undo */
                     /*  or a remove layer undo?  */
};

struct LayerMaskUndo
{
  Layer * layer;     /*  the layer                */
  int apply_mask;    /*  apply mask?              */
  int edit_mask;     /*  edit mask or layer?      */
  int show_mask;     /*  show the mask?           */
  LayerMask *mask;   /*  the layer mask           */
  int mode;          /*  the application mode     */
  int undo_type;     /*  is this a new layer mask */
                     /*  or a remove layer mask   */
};

struct FStoLayerUndo
{
  Layer * layer;     /*  the layer                */
  CanvasDrawable * drawable;      /*  drawable of floating sel */
};

/* function declarations */

Layer *         layer_new (int, int, int, Tag, StorageType, char *, gfloat, int);
Layer *         layer_copy (Layer *, int);
Layer *		layer_ref (Layer *);
void   		layer_unref (Layer *);

Layer *         layer_from_tiles (void *, CanvasDrawable *, Canvas *, char *, gfloat, int);
LayerMask *     layer_add_mask (Layer *, LayerMask *);
LayerMask *     layer_create_mask (Layer *, AddMaskType);
Layer *         layer_get_ID (int);
void            layer_delete (Layer *);
void            layer_apply_mask (Layer *, int);
void            layer_translate (Layer *, int, int);
void            layer_translate2 (Layer *, int, int, int, int, int, int);
void            layer_add_alpha (Layer *);
void            layer_remove_alpha (Layer *);
void            layer_scale (Layer *, int, int, int);
void            layer_resize (Layer *, int, int, int, int);
BoundSeg *      layer_boundary (Layer *, int *);
void            layer_invalidate_boundary (Layer *);
int             layer_pick_correlate (Layer *, int, int);

LayerMask *     layer_mask_new  (int, int, int, Precision, 
				 char *, gfloat, PixelRow *);
LayerMask *	layer_mask_copy	(LayerMask *);
void		layer_mask_delete	(LayerMask *);
LayerMask *	layer_mask_get_ID    (int);
LayerMask *	layer_mask_ref (LayerMask *);
void   		layer_mask_unref (LayerMask *);

/* access functions */

LayerMask *     layer_mask (Layer *);
int             layer_has_alpha (Layer *);
int             layer_is_floating_sel (Layer *);
int		layer_linked (Layer *);
Canvas *layer_preview (Layer *, int, int);
Canvas *layer_mask_preview (Layer *, int, int);

void            layer_invalidate_previews  (int);


/* from drawable.c */
Layer *          drawable_layer              (CanvasDrawable *);
LayerMask *      drawable_layer_mask         (CanvasDrawable *);

/* from channel.c */
void            channel_layer_alpha     (Channel *, Layer *);
void            channel_layer_mask      (Channel *, Layer *);

#endif /* __LAYER_H__ */
