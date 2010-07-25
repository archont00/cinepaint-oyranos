/* TODO: make sure has_alpha gets set */

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "../../lib/wire/dl_list.h"
#include "../appenv.h"
#include "../canvas.h"
#include "../drawable.h"
#include "../errors.h"
#include "float16.h"
#include "../floating_sel.h"
#include "../gdisplay.h"
#include "../gimage.h"
#include "../gimage_mask.h"
#include "../interface.h"
#include "../layer.h"
#include "../layers_dialog.h"
#include "../paint_funcs_area.h"
#include "../palette.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../undo.h"
#include "../layer_pvt.h"
#include "../spline.h"

enum {
  LAST_SIGNAL
};

static void gimp_layer_class_init    (GimpLayerClass *klass);
static void gimp_layer_init          (GimpLayer      *layer);
static void gimp_layer_destroy       (GtkObject      *object);
static void layer_invalidate_preview (GtkObject *);

static void gimp_layer_mask_class_init (GimpLayerMaskClass *klass);
static void gimp_layer_mask_init       (GimpLayerMask      *layermask);
static void gimp_layer_mask_destroy    (GtkObject          *object);

/*
static gint layer_signals[LAST_SIGNAL] = { 0 };
static gint layer_mask_signals[LAST_SIGNAL] = { 0 };
*/

static CanvasDrawableClass *layer_parent_class = NULL;
static GimpChannelClass *layer_mask_parent_class = NULL;

guint
gimp_layer_get_type ()
{
  static guint layer_type = 0;

  if (!layer_type)
    {
      GtkTypeInfo layer_info =
      {
	"GimpLayer",
	sizeof (GimpLayer),
	sizeof (GimpLayerClass),
	(GtkClassInitFunc) gimp_layer_class_init,
	(GtkObjectInitFunc) gimp_layer_init,
#if GTK_MAJOR_VERSION > 1
	NULL,
	NULL,
#else
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
#endif
      };

      layer_type = gtk_type_unique (gimp_drawable_get_type (), &layer_info);
    }

  return layer_type;
}

static void
gimp_layer_class_init (GimpLayerClass *class)
{
  GtkObjectClass *object_class;
  CanvasDrawableClass *drawable_class;

  object_class = (GtkObjectClass*) class;
  drawable_class = (CanvasDrawableClass*) class;

  layer_parent_class = gtk_type_class (gimp_drawable_get_type ());

  /*
  gtk_object_class_add_signals (object_class, layer_signals, LAST_SIGNAL);
  */

  object_class->destroy = gimp_layer_destroy;
  drawable_class->invalidate_preview = layer_invalidate_preview;
}

static void
gimp_layer_init (GimpLayer *layer)
{
}

guint
gimp_layer_mask_get_type ()
{
  static guint layer_mask_type = 0;

  if (!layer_mask_type)
    {
      GtkTypeInfo layer_mask_info =
      {
	"GimpLayerMask",
	sizeof (GimpLayerMask),
	sizeof (GimpLayerMaskClass),
	(GtkClassInitFunc) gimp_layer_mask_class_init,
	(GtkObjectInitFunc) gimp_layer_mask_init,
#if GTK_MAJOR_VERSION > 1
	NULL,
	NULL,
#else
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
#endif
      };

      layer_mask_type = gtk_type_unique (gimp_channel_get_type (), &layer_mask_info);
    }

  return layer_mask_type;
}

static void
gimp_layer_mask_class_init (GimpLayerMaskClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  layer_mask_parent_class = gtk_type_class (gimp_channel_get_type ());

  /*
  gtk_object_class_add_signals (object_class, layer_mask_signals, LAST_SIGNAL);
  */

  object_class->destroy = gimp_layer_mask_destroy;
}

static void
gimp_layer_mask_init (GimpLayerMask *layermask)
{
}

/*
 *  Static variables
 */
extern int global_drawable_ID;

int layer_get_count = 0;


/********************************/
/*  Local function definitions  */

static void
layer_invalidate_preview (GtkObject *object)
{
  GimpLayer *layer;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_LAYER (object));
  
  layer = GIMP_LAYER (object);

  if (layer_is_floating_sel (layer)) 
    floating_sel_invalidate (layer);
}


/**************************/
/*  Function definitions  */

Layer * 
layer_new  (
            int gimage_ID,
            int width,
            int height,
            Tag tag,
            StorageType storage,
            char * name,
            gfloat opacity,
            int mode
            )
{
  Layer * layer;
  SplineTool *st;

  if (width == 0 || height == 0) {
    g_message (_("Zero width or height layers not allowed."));
    return NULL;
  }

  layer = gtk_type_new (gimp_layer_get_type ());

  gimp_drawable_configure (GIMP_DRAWABLE(layer), 
			   gimage_ID, width, height, tag, storage, name);

  /*  allocate the memory for this layer  */
  layer->linked = 0;
  layer->preserve_trans = 0;

  /*  no layer mask is present at start  */
  layer->mask = NULL;
  layer->apply_mask = 0;
  layer->edit_mask = 0;
  layer->show_mask = 0;

  layer->gimage_id = gimage_ID;

  /*  mode and opacity  */
  layer->mode = mode;
  layer->opacity = opacity;

  /*  floating selection variables  */
  layer->fs.backing_store = NULL;
  layer->fs.drawable = NULL; 
  layer->fs.initial = TRUE;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs = NULL;
  layer->fs.num_segs = 0;

  /* spline layer variables */
  layer->sl.splines=(DL_list*)malloc(sizeof(DL_list));
  DL_init((DL_node*)layer->sl.splines);
  st=(SplineTool*)malloc(sizeof(SplineTool));
  st->motion_active=0;
  st->click_on_point=0;
  st->active_spline=(Spline*)layer->sl.splines->head;
  st->active_point=NULL;
  layer->sl.spline_tool=st;
  return layer;
}


Layer *
layer_ref (Layer *layer)
{
  gtk_object_ref  (GTK_OBJECT (layer));
  gtk_object_sink (GTK_OBJECT (layer));
  return layer;
}


void
layer_unref (Layer *layer)
{
  gtk_object_unref (GTK_OBJECT (layer));
}


Layer *
layer_copy (layer, add_alpha)
     Layer * layer;
     int add_alpha;
{
  char * layer_name;
  Layer * new_layer;
  PixelArea srcPR, destPR;
  Tag layer_tag, new_layer_tag;

  /*  formulate the new layer name  */
  layer_name = (char *) g_malloc (strlen (GIMP_DRAWABLE(layer)->name) + 6);
  sprintf (layer_name, "%s copy", GIMP_DRAWABLE(layer)->name);

  layer_tag = new_layer_tag = drawable_tag (GIMP_DRAWABLE(layer));
 
  
  /*  when copying a layer, the copy ALWAYS has an alpha channel  */
  if (add_alpha)
    new_layer_tag = tag_set_alpha (new_layer_tag, ALPHA_YES);

  /*  allocate a new layer object  */
  new_layer = layer_new (GIMP_DRAWABLE(layer)->gimage_ID, 
			 drawable_width (GIMP_DRAWABLE(layer)),
                         drawable_height (GIMP_DRAWABLE(layer)), 
			 new_layer_tag, 
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
			 layer_name, 
			 layer->opacity, layer->mode);
  
  new_layer->gimage_id = layer->gimage_id;
  if (!new_layer) {
    g_message (_("layer_copy: could not allocate new layer"));
    goto cleanup;
  }

  GIMP_DRAWABLE(new_layer)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
  GIMP_DRAWABLE(new_layer)->offset_y = GIMP_DRAWABLE(layer)->offset_y;
  GIMP_DRAWABLE(new_layer)->visible = GIMP_DRAWABLE(layer)->visible;
  new_layer->linked = layer->linked;
  new_layer->preserve_trans = layer->preserve_trans;

  /*  copy the contents across layers  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
                  0, 0,
                  0, 0,
                  FALSE);
  
  pixelarea_init (&destPR, GIMP_DRAWABLE(new_layer)->tiles,
                  0, 0,
                  0, 0,
                  TRUE);
  
  copy_area (&srcPR, &destPR);

  /*  duplicate the layer mask if necessary  */
  if (layer->mask)
    {
      new_layer->mask = layer_mask_ref (layer_mask_copy (layer->mask));
      new_layer->apply_mask = layer->apply_mask;
      new_layer->edit_mask = layer->edit_mask;
      new_layer->show_mask = layer->show_mask;
    }

 cleanup:
  /*  free up the layer_name memory  */
  g_free (layer_name);

  return new_layer;
}


Layer *
layer_from_tiles (void *gimage_ptr, CanvasDrawable *drawable, Canvas *tiles, char *name, gfloat opacity,int mode)
{
  GImage * gimage;
  Layer * new_layer;
  Tag layer_tag, new_layer_tag;
  PixelArea layerPR, bufPR;

  /*  Function copies buffer to a layer
   *  taking into consideration the possibility of transforming
   *  the contents to meet the requirements of the target image type
   */

  if (!tiles)
    return NULL;

  gimage = (GImage *) gimage_ptr;

  layer_tag = drawable_tag (drawable);
  new_layer_tag = tag_set_alpha (layer_tag, ALPHA_YES);

  /*  Create the new layer  */
  new_layer = layer_new (0, canvas_width (tiles), canvas_height (tiles),
                         new_layer_tag, 
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
                         name, opacity, mode);

  if (!new_layer) {
    g_message(_("layer_from_tiles: could not allocate new layer"));
    return NULL;
  }

  /* copy the image data */
  pixelarea_init (&bufPR, tiles, 
                  0, 0,
                  0, 0,
                  FALSE);

  pixelarea_init (&layerPR, GIMP_DRAWABLE(new_layer)->tiles, 
                  0, 0,
                  0, 0,
                  TRUE);

  copy_area (&bufPR, &layerPR);

  return new_layer;
}

LayerMask *
layer_add_mask (layer, mask)
     Layer * layer;
     LayerMask * mask;
{
  if (layer->mask)
    return NULL;

  layer->mask = layer_mask_ref (mask);
  mask->layer = layer;

  /*  Set the application mode in the layer to "apply"  */
  layer->apply_mask = 1;
  layer->edit_mask = 1;
  layer->show_mask = 0;

  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  return layer->mask;
}


LayerMask *
layer_create_mask (layer, add_mask_type)
     Layer * layer;
     AddMaskType add_mask_type;
{
  PixelArea maskPR, layerPR;
  LayerMask *mask;
  char * mask_name;
  Precision prec;

  COLOR16_NEW (white_color, drawable_tag(GIMP_DRAWABLE(layer)) );
  COLOR16_NEW (black_color, drawable_tag(GIMP_DRAWABLE(layer)) );
  COLOR16_INIT (black_color);
  COLOR16_INIT (white_color);
  palette_get_white (&white_color);
  palette_get_black (&black_color);
  
  prec = tag_precision (drawable_tag (GIMP_DRAWABLE(layer)));
  
  mask_name = (char *) g_malloc (strlen (GIMP_DRAWABLE(layer)->name) + strlen ("mask") + 2);
  sprintf (mask_name, "%s mask", GIMP_DRAWABLE(layer)->name);

  /*  Create the layer mask  */
  mask = layer_mask_new (GIMP_DRAWABLE(layer)->gimage_ID, 
                         drawable_width (GIMP_DRAWABLE(layer)),
                         drawable_height (GIMP_DRAWABLE(layer)),
                         prec, mask_name, 1.0, &black_color);

  GIMP_DRAWABLE(mask)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
  GIMP_DRAWABLE(mask)->offset_y = GIMP_DRAWABLE(layer)->offset_y;

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->tiles,
                  0, 0,
                  0, 0,
                  TRUE);

  switch (add_mask_type)
    {
    case WhiteMask:
      color_area (&maskPR, &white_color);
      break;
    case BlackMask:
      color_area (&maskPR, &black_color);
      break;
    case AlphaMask:
      /*  Extract the layer's alpha channel  */
      if (layer_has_alpha (layer))
	{
	  pixelarea_init (&layerPR, GIMP_DRAWABLE(layer)->tiles, 
                          0, 0,
                          0, 0,
                          FALSE);

	  extract_alpha_area (&layerPR, NULL, &maskPR);
	}
      break;
    case AuxMask:
	{
	  extern GSList *image_list;
	  GSList *list = image_list;
	  GImage *gimage;

	  while (list)
	    {
	      gimage = (GImage*) list->data;
	      if (gimage->ID == layer->gimage_id)
		{
		  if (gimage->channel_as_opacity)
		    {
		      pixelarea_init (&layerPR, GIMP_DRAWABLE (gimage->channel_as_opacity)->tiles,
			  0, 0, 
			  0, 0,
			  FALSE);
		      extract_alpha_area (&layerPR, NULL, &maskPR); 
		    }
		  break;
		}
	      list = g_slist_next (list);
	    }
	}
      break;
      
    }

  g_free (mask_name);
  return mask;
}


Layer *
layer_get_ID (int ID)
{
  CanvasDrawable *drawable;
  drawable = drawable_get_ID (ID);
  if (drawable && GIMP_IS_LAYER (drawable)) 
    return GIMP_LAYER (drawable);
  else
    return NULL;
}


void
layer_delete (Layer * layer)
{
  gtk_object_unref (GTK_OBJECT (layer));
}

static void
gimp_layer_destroy (GtkObject *object)
{
  GimpLayer *layer;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_LAYER (object));

  layer = GIMP_LAYER (object);

  /*  if a layer mask exists, free it  */
  if (layer->mask)
    layer_mask_delete (layer->mask);
  layer->mask = NULL;

  /*  free the layer boundary if it exists  */
  if (layer->fs.segs)
    g_free (layer->fs.segs);
  layer->fs.segs = NULL;

  /*  free the floating selection if it exists  */
  if (layer_is_floating_sel (layer))
      canvas_delete (layer->fs.backing_store);

  if (GTK_OBJECT_CLASS (layer_parent_class)->destroy)
    (*GTK_OBJECT_CLASS (layer_parent_class)->destroy) (object);
}

void
layer_apply_mask (layer, mode)
     Layer * layer;
     int mode;
{
  PixelArea srcPR, maskPR;

  if (!layer->mask)
    return;

  /*  this operation can only be done to layers with an alpha channel  */
  if (! layer_has_alpha (layer))
    return;

  /*  Need to save the mask here for undo  */

  if (mode == APPLY)
    {
      /*  Put this apply mask operation on the undo stack
       */
      drawable_apply_image (GIMP_DRAWABLE(layer),
                            0, 0,
                            0, 0,
			    0, 0, 
                            NULL);

      /*  Combine the current layer's alpha channel and the mask  */
      pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles,
                      0, 0,
                      0, 0,
                      TRUE);
      
      pixelarea_init (&maskPR, GIMP_DRAWABLE(layer->mask)->tiles, 
                      0, 0,
                      drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)),
                      FALSE);

      apply_mask_to_area (&srcPR, &maskPR, 1.0);

      drawable_invalidate_preview (GIMP_DRAWABLE(layer));

      layer->mask = NULL;
      layer->apply_mask = 0;
      layer->edit_mask = 0;
      layer->show_mask = 0;
    }
  else if (mode == DISCARD)
    {
      layer->mask = NULL;
      layer->apply_mask = 0;
      layer->edit_mask = 0;
      layer->show_mask = 0;
    }
}


void
layer_translate (layer, off_x, off_y)
     Layer * layer;
     int off_x, off_y;
{
  /*  the undo call goes here  */
  undo_push_layer_displace (gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID), GIMP_DRAWABLE(layer)->ID);

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  update the layer offsets  */
  GIMP_DRAWABLE(layer)->offset_x += off_x;
  GIMP_DRAWABLE(layer)->offset_y += off_y;

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  if (layer->mask) 
    {
      GIMP_DRAWABLE(layer->mask)->offset_x += off_x;
      GIMP_DRAWABLE(layer->mask)->offset_y += off_y;
  /*  invalidate the mask preview  */
      drawable_invalidate_preview (GIMP_DRAWABLE(layer->mask));
    }
}

void
layer_translate2 (layer, off_x, off_y, x, y, w, h)
     Layer * layer;
     int off_x, off_y;
     int x, y, h, w; 
{
  /*  the undo call goes here  */
  undo_push_layer_displace (gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID), GIMP_DRAWABLE(layer)->ID);

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE(layer),
                   x, y,
                   w, h);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  update the layer offsets  */
  GIMP_DRAWABLE(layer)->offset_x = off_x;
  GIMP_DRAWABLE(layer)->offset_y = off_y;

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE(layer),
                   x, y,
                   w, h);

  if (layer->mask) 
    {
      GIMP_DRAWABLE(layer->mask)->offset_x = off_x;
      GIMP_DRAWABLE(layer->mask)->offset_y = off_y;
  /*  invalidate the mask preview  */
      drawable_invalidate_preview (GIMP_DRAWABLE(layer->mask));
    }
  gdisplays_flush ();
}


void
layer_remove_alpha (layer)
     Layer *layer;
{
  PixelArea srcPR, destPR;
  Canvas * c;
  Tag t;
  GImage* gimage = NULL;
  if(!layer)
  {	return;
  }
  t = drawable_tag (GIMP_DRAWABLE(layer));

  if (tag_alpha (t) == ALPHA_NO)
    return;

  t = tag_set_alpha (t, ALPHA_NO);

  /*  Allocate the new layer */
  c = canvas_new (t, 
                  drawable_width (GIMP_DRAWABLE(layer)),
                  drawable_height (GIMP_DRAWABLE(layer)),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

  /*  Configure the pixel areas */

  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
                  0, 0,
                  0, 0,
                  FALSE);

  pixelarea_init (&destPR, c, 
                  0, 0,
                  0, 0,
                  TRUE);
  
  /*  remove the alpha channel  */
  copy_area (&srcPR, &destPR);
  /*add_alpha_area (&srcPR, &destPR);*/

  canvas_delete (GIMP_DRAWABLE(layer)->tiles);

  /*  Configure the new layer  */
  GIMP_DRAWABLE(layer)->tiles = c;

  /*  update gdisplay titles to reflect the possibility of
   *  this layer being the only layer in the gimage
   */
  gdisplays_update_title (GIMP_DRAWABLE(layer)->gimage_ID);

  if((gimage = gimage_get_ID(GIMP_DRAWABLE(layer)->gimage_ID)))
    lc_dialog_update (gimage);
}


void
layer_add_alpha (layer)
     Layer *layer;
{
  PixelArea srcPR, destPR;
  Canvas * c;
  Tag t;
  GImage* gimage = NULL;

  t = drawable_tag (GIMP_DRAWABLE(layer));

  if (tag_alpha (t) == ALPHA_YES)
    return;

  t = tag_set_alpha (t, ALPHA_YES);

  /*  Allocate the new layer */
  c = canvas_new (t, 
                  drawable_width (GIMP_DRAWABLE(layer)),
                  drawable_height (GIMP_DRAWABLE(layer)),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

  /*  Configure the pixel areas */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
                  0, 0,
                  0, 0,
                  FALSE);

  pixelarea_init (&destPR, c, 
                  0, 0,
                  0, 0,
                  TRUE);
  
  /*  Add an alpha channel  */
  copy_area (&srcPR, &destPR);
  /*add_alpha_area (&srcPR, &destPR);*/

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID), layer);

  /*  Configure the new layer  */
  GIMP_DRAWABLE(layer)->tiles = c;

  /*  update gdisplay titles to reflect the possibility of
   *  this layer being the only layer in the gimage
   */
  gdisplays_update_title (GIMP_DRAWABLE(layer)->gimage_ID);

  if((gimage = gimage_get_ID(GIMP_DRAWABLE(layer)->gimage_ID)))
    lc_dialog_update (gimage);
}


void
layer_scale (layer, new_width, new_height, local_origin)
     Layer *layer;
     int new_width, new_height;
     int local_origin;
{
  PixelArea srcPR, destPR;
  Canvas *new_canvas;
  Tag layer_tag = drawable_tag (GIMP_DRAWABLE(layer));

  if (new_width == 0 || new_height == 0)
    return;


  /*  Update the old layer position  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  /*  Configure the pixel regions  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
                  0, 0,
                  0, 0,
                  FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_canvas = canvas_new (layer_tag, new_width, new_height,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
  pixelarea_init (&destPR, new_canvas, 0, 0, new_width, new_height, TRUE);

  /*  Scale the layer -
   *   If the layer is of type INDEXED, then we don't use pixel-value
   *   resampling because that doesn't necessarily make sense for INDEXED
   *   images.
   */
  if (tag_format (layer_tag) == FORMAT_INDEXED)
    scale_area_no_resample (&srcPR, &destPR);
  else
    scale_area (&srcPR, &destPR);

  /*  Configure the new layer  */
  if (local_origin)
    {
      int cx, cy;

      cx = GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer)) / 2;
      cy = GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer)) / 2;

      GIMP_DRAWABLE(layer)->offset_x = cx - (new_width / 2); 
      GIMP_DRAWABLE(layer)->offset_y = cy - (new_height / 2); 
    }
  else
    {
      double xrat, yrat;

      xrat = (double) new_width / (double) drawable_width (GIMP_DRAWABLE(layer));
      yrat = (double) new_height / (double) drawable_height (GIMP_DRAWABLE(layer));

      GIMP_DRAWABLE(layer)->offset_x = (int) (xrat * GIMP_DRAWABLE(layer)->offset_x);
      GIMP_DRAWABLE(layer)->offset_y = (int) (yrat * GIMP_DRAWABLE(layer)->offset_y);
    }

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID), layer);

  GIMP_DRAWABLE(layer)->tiles = new_canvas;

  /*  If there is a layer mask, make sure it gets scaled also  */
  if (layer->mask) 
    {
      GIMP_DRAWABLE(layer->mask)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
      GIMP_DRAWABLE(layer->mask)->offset_y = GIMP_DRAWABLE(layer)->offset_y;
      channel_scale (GIMP_CHANNEL(layer->mask), new_width, new_height);
    }
  
  /*  Update the new layer position  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);
}


void
layer_resize (layer, new_width, new_height, offx, offy)
     Layer *layer;
     int new_width, new_height;
     int offx, offy;
{
  PixelArea srcPR, destPR;
  Canvas *new_canvas;
  int w, h;
  int x1, y1, x2, y2;
  Tag layer_tag = drawable_tag(GIMP_DRAWABLE(layer));

  if (!new_width || !new_height)
    return;

  x1 = BOUNDS (offx, 0, new_width);
  y1 = BOUNDS (offy, 0, new_height);
  x2 = BOUNDS ((offx + drawable_width (GIMP_DRAWABLE(layer))), 0, new_width);
  y2 = BOUNDS ((offy + drawable_height (GIMP_DRAWABLE(layer))), 0, new_height);
  w = x2 - x1;
  h = y2 - y1;

  if (offx > 0)
    {
      x1 = 0;
      x2 = offx;
    }
  else
    {
      x1 = -offx;
      x2 = 0;
    }

  if (offy > 0)
    {
      y1 = 0;
      y2 = offy;
    }
  else
    {
      y1 = -offy;
      y2 = 0;
    }

  /*  Update the old layer position  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  /*  Configure the pixel regions  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
	x1, y1, w, h, FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_canvas = canvas_new (layer_tag, new_width, new_height,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
  pixelarea_init (&destPR, new_canvas, 0, 0, new_width, new_height, TRUE);

  /*  fill with the fill color  */
  if (layer_has_alpha (layer))
    {
      /*  Set to transparent and black  */
      COLOR16_NEW (black_color, layer_tag);
      COLOR16_INIT (black_color);
      palette_get_black (&black_color);
      color_area (&destPR,  &black_color);
    }
  else
    {
      COLOR16_NEW (bg_color, layer_tag);
      COLOR16_INIT (bg_color);
      palette_get_background (&bg_color);
      color_area(&destPR, &bg_color);
    }
  pixelarea_init (&destPR, new_canvas, x2, y2, w, h, TRUE);

  /*  copy from the old to the new  */
  if (w && h)
    copy_area (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID), layer);

  /*  Configure the new layer  */
  GIMP_DRAWABLE(layer)->tiles = new_canvas;
  GIMP_DRAWABLE(layer)->offset_x = x1 + GIMP_DRAWABLE(layer)->offset_x - x2;
  GIMP_DRAWABLE(layer)->offset_y = y1 + GIMP_DRAWABLE(layer)->offset_y - y2;

  /*  If there is a layer mask, make sure it gets resized also  */
  if (layer->mask) {
    GIMP_DRAWABLE(layer->mask)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
    GIMP_DRAWABLE(layer->mask)->offset_y = GIMP_DRAWABLE(layer)->offset_y;
    channel_resize (GIMP_CHANNEL(layer->mask), new_width, new_height, offx, offy);
  }

  /*  update the new layer area  */
  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);
}


BoundSeg *
layer_boundary (layer, num_segs)
     Layer *layer;
     int *num_segs;
{
  BoundSeg *new_segs;

  /*  Create the four boundary segments that encompass this
   *  layer's boundary.
   */
  new_segs = (BoundSeg *) g_malloc (sizeof (BoundSeg) * 4);
  *num_segs = 4;

  /*  if the layer is a floating selection  */
  if (layer_is_floating_sel (layer))
    {
      /*  if the owner drawable is a channel, just return nothing  */
      if (drawable_channel (layer->fs.drawable))
	{
	  *num_segs = 0;
	  return NULL;
	}
      /*  otherwise, set the layer to the owner drawable  */
      else
	layer = GIMP_LAYER(layer->fs.drawable);
    }

  new_segs[0].x1 = GIMP_DRAWABLE(layer)->offset_x;
  new_segs[0].y1 = GIMP_DRAWABLE(layer)->offset_y;
  new_segs[0].x2 = GIMP_DRAWABLE(layer)->offset_x;
  new_segs[0].y2 = GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer));
  new_segs[0].open = 1;

  new_segs[1].x1 = GIMP_DRAWABLE(layer)->offset_x;
  new_segs[1].y1 = GIMP_DRAWABLE(layer)->offset_y;
  new_segs[1].x2 = GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer));
  new_segs[1].y2 = GIMP_DRAWABLE(layer)->offset_y;
  new_segs[1].open = 1;

  new_segs[2].x1 = GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer));
  new_segs[2].y1 = GIMP_DRAWABLE(layer)->offset_y;
  new_segs[2].x2 = GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer));
  new_segs[2].y2 = GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer));
  new_segs[2].open = 0;

  new_segs[3].x1 = GIMP_DRAWABLE(layer)->offset_x;
  new_segs[3].y1 = GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer));
  new_segs[3].x2 = GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer));
  new_segs[3].y2 = GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer));
  new_segs[3].open = 0;

  return new_segs;
}


void
layer_invalidate_boundary (layer)
     Layer *layer;
{
  GImage *gimage;
  Channel *mask;

  /*  first get the selection mask channel  */
  if (! (gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)))
    return;

  /*  Turn the current selection off  */
  gdisplays_selection_visibility (gimage->ID, SelectionOff);

  mask = gimage_get_mask (gimage);

  /*  Only bother with the bounds if there is a selection  */
  if (! channel_is_empty (mask))
    {
      mask->bounds_known = FALSE;
      mask->boundary_known = FALSE;
    }

  /*  clear the affected region surrounding the layer  */
  gdisplays_selection_visibility (GIMP_DRAWABLE(layer)->gimage_ID, SelectionLayerOff);
}


int
layer_pick_correlate (layer, x, y)
     Layer *layer;
     int x, y;
{
  Canvas * canvas, * mask=NULL;
  guchar * c_data, * m_data=NULL;
  gint alpha;
  guint rc = FALSE;

  /*  Is the point inside the layer?
   *  First transform the point to layer coordinates...
   */
  x -= GIMP_DRAWABLE(layer)->offset_x;
  y -= GIMP_DRAWABLE(layer)->offset_y;
  if (x >= 0 && x < drawable_width (GIMP_DRAWABLE(layer)) &&
      y >= 0 && y < drawable_height (GIMP_DRAWABLE(layer)) &&
      GIMP_DRAWABLE(layer)->visible)
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! layer_has_alpha (layer))
	return TRUE;

      /*  Otherwise, determine if the alpha value at
       *  the given point is non-zero
       */
      alpha = tag_num_channels (drawable_tag (GIMP_DRAWABLE(layer))) - 1; 

      canvas = drawable_data (GIMP_DRAWABLE(layer));
      canvas_portion_refro (canvas, x, y);
      c_data = canvas_portion_data (canvas, x, y);

      if (layer->mask)
	{
	  mask = GIMP_DRAWABLE(layer->mask)->tiles;
          canvas_portion_refro (mask, x, y);
          m_data = canvas_portion_data (mask, x, y);
        }
      
      switch (tag_precision (canvas_tag (canvas)))
        {
        case PRECISION_U8:
          {
            guint8 * d = (guint8*) c_data;
            guint8 val = d[alpha];
            if (layer->mask)
              {
                guint8 * m = (guint8*) m_data;
                val = (val * *m) / 255;
              }
            if (val > (256/4))
              rc = TRUE;
          }
          break;
          
        case PRECISION_BFP:
        case PRECISION_U16:
          {
            guint16 * d = (guint16*) c_data;
            guint16 val = d[alpha];
            if (layer->mask)
              {
                guint16 * m = (guint16*) m_data;
                val = (val * *m) / 65535;
              }
            if (val > (65536/4))
              rc = TRUE;
          }
          break;
          
        case PRECISION_FLOAT:
          {
            gfloat * d = (gfloat*) c_data;
            gfloat val = d[alpha];
            if (layer->mask)
              {
                gfloat * m = (gfloat*) m_data;
                val = (val * *m);
              }
            if (val > 0.25)
              rc = TRUE;
          }
          break;
          
        case PRECISION_FLOAT16:
          {
            guint16 * d = (guint16*) c_data;
            gfloat val = d[alpha];
	    ShortsFloat u;
            if (layer->mask)
              {
                guint16 * m = (guint16*) m_data;
                val = val * FLT (*m, u);
              }
            if (val > .25)
              rc = TRUE;
          }
          break;
          
        }
      
      canvas_portion_unref (canvas, x, y);
      if (layer->mask)
        canvas_portion_unref (mask, x, y);
    }

  return rc;
}


/********************/
/* access functions */

LayerMask *
layer_mask (layer)
     Layer * layer;
{
  return layer->mask;
}


int
layer_has_alpha (layer)
     Layer * layer;
{
  return (drawable_has_alpha (GIMP_DRAWABLE(layer)));
}


int
layer_is_floating_sel (layer)
     Layer *layer;
{
  if (layer->fs.drawable != NULL)
    return 1;
  else
    return 0;
}

int 
layer_linked (Layer *layer)
{
  return layer->linked;
}

Canvas *
layer_preview (layer, w, h)
     Layer *layer;
     int w, h;
{
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  if (! (GIMP_DRAWABLE(layer)->preview_valid &&
         canvas_width (GIMP_DRAWABLE(layer)->preview) == w &&
         canvas_height (GIMP_DRAWABLE(layer)->preview) == h))
    {
      PixelArea srcPR, destPR;
      Canvas * preview_buf;
      Tag tag = drawable_tag (GIMP_DRAWABLE(layer));
      Precision p = tag_precision (tag);
      Format f = tag_format (tag);
      Alpha a = tag_alpha (tag);
      
      preview_buf = canvas_new (tag_new (p, f, a),
                                w, h,
                                STORAGE_FLAT);

      pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles,
                      0, 0,
                      0, 0,
                      FALSE);

      pixelarea_init (&destPR, preview_buf,
                      0, 0,
                      0, 0,
                      TRUE);

      scale_area_no_resample (&srcPR, &destPR);

      if (GIMP_DRAWABLE(layer)->preview)
	canvas_delete (GIMP_DRAWABLE(layer)->preview);

      GIMP_DRAWABLE(layer)->preview = preview_buf;
      GIMP_DRAWABLE(layer)->preview_valid = TRUE;
    }

  return GIMP_DRAWABLE(layer)->preview;
}


Canvas *
layer_mask_preview (layer, w, h)
     Layer *layer;
     int w, h;
{
  LayerMask * mask = layer->mask;

  if (!mask)
    return NULL;

  if (w < 1) w = 1; 
  if (h < 1) h = 1;

  if (! (GIMP_DRAWABLE(mask)->preview_valid &&
         canvas_width (GIMP_DRAWABLE(mask)->preview) == w &&
         canvas_height (GIMP_DRAWABLE(mask)->preview) == h))
    {
      PixelArea srcPR, destPR;
      Canvas * preview_buf;
      Precision p;

      p = tag_precision (drawable_tag (GIMP_DRAWABLE(mask)));

      preview_buf = canvas_new (tag_new (p, FORMAT_GRAY, ALPHA_NO),
                                w, h,
                                STORAGE_FLAT);
      
     pixelarea_init (&srcPR, GIMP_DRAWABLE(mask)->tiles,
                      0, 0,
                      0, 0,
                      FALSE);

      pixelarea_init (&destPR, preview_buf,
                      0, 0,
                      0, 0,
                      TRUE);

      scale_area_no_resample (&srcPR, &destPR);

      if (GIMP_DRAWABLE(mask)->preview)
	canvas_delete (GIMP_DRAWABLE(mask)->preview);

      GIMP_DRAWABLE(mask)->preview = preview_buf;
      GIMP_DRAWABLE(mask)->preview_valid = TRUE;
    }

  return GIMP_DRAWABLE(mask)->preview;
}


void
layer_invalidate_previews (gimage_id)
     int gimage_id;
{
  GSList * tmp;
  Layer * layer;
  GImage * gimage;

  if (! (gimage = gimage_get_ID (gimage_id)))
    return;

  tmp = gimage->layers;

  while (tmp)
    {
      layer = (Layer *) tmp->data;
      drawable_invalidate_preview (GIMP_DRAWABLE(layer));
      tmp = g_slist_next (tmp);
    }
}


static void
gimp_layer_mask_destroy (GtkObject *object)
{
  GimpLayerMask *layermask;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_LAYER_MASK (object));

  layermask = GIMP_LAYER_MASK (object);

  if (GTK_OBJECT_CLASS (layer_mask_parent_class)->destroy)
    (*GTK_OBJECT_CLASS (layer_mask_parent_class)->destroy) (object);
}

LayerMask *
layer_mask_new (int gimage_ID, int width, int height, 
                Precision p, char *name, gfloat opacity, PixelRow *col)
{
  LayerMask * layer_mask;
  Tag tag;

  layer_mask = gtk_type_new (gimp_layer_mask_get_type ());

  tag = tag_new (p, FORMAT_GRAY, ALPHA_NO);
  pixelrow_init (&GIMP_CHANNEL(layer_mask)->col, tag, GIMP_CHANNEL(layer_mask)->_col, 1);

  gimp_drawable_configure (GIMP_DRAWABLE(layer_mask), 
			   gimage_ID, width, height, tag, 
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
			   name);

  /*  set the layer_mask color and opacity  */
  copy_row (col, &GIMP_CHANNEL(layer_mask)->col);
  GIMP_CHANNEL(layer_mask)->opacity = opacity;
  GIMP_CHANNEL(layer_mask)->show_masked = 1;

  /*  selection mask variables  */
  GIMP_CHANNEL(layer_mask)->empty = TRUE;
  GIMP_CHANNEL(layer_mask)->segs_in = NULL;
  GIMP_CHANNEL(layer_mask)->segs_out = NULL;
  GIMP_CHANNEL(layer_mask)->num_segs_in = 0;
  GIMP_CHANNEL(layer_mask)->num_segs_out = 0;
  GIMP_CHANNEL(layer_mask)->bounds_known = TRUE;
  GIMP_CHANNEL(layer_mask)->boundary_known = TRUE;
  GIMP_CHANNEL(layer_mask)->x1 = GIMP_CHANNEL(layer_mask)->y1 = 0;
  GIMP_CHANNEL(layer_mask)->x2 = width;
  GIMP_CHANNEL(layer_mask)->y2 = height;

  return layer_mask;
}


LayerMask *
layer_mask_copy (LayerMask *layer_mask)
{
  char * layer_mask_name;
  LayerMask * new_layer_mask;
  PixelArea srcPR, destPR;
  Tag tag = drawable_tag (GIMP_DRAWABLE(layer_mask));
  
  /*  formulate the new layer_mask name  */
  layer_mask_name = (char *) g_malloc (strlen (GIMP_DRAWABLE(layer_mask)->name) + 6);
  sprintf (layer_mask_name, "%s copy", GIMP_DRAWABLE(layer_mask)->name);

  /*  allocate a new layer_mask object  */
  new_layer_mask = layer_mask_new (GIMP_DRAWABLE(layer_mask)->gimage_ID, 
                                   drawable_width (GIMP_DRAWABLE(layer_mask)), 
                                   drawable_height (GIMP_DRAWABLE(layer_mask)), 
                                   tag_precision (tag),
                                   layer_mask_name, 
                                   GIMP_CHANNEL(layer_mask)->opacity, 
                                   &GIMP_CHANNEL(layer_mask)->col);
  GIMP_DRAWABLE(new_layer_mask)->visible = GIMP_DRAWABLE(layer_mask)->visible;
  GIMP_DRAWABLE(new_layer_mask)->offset_x = GIMP_DRAWABLE(layer_mask)->offset_x;
  GIMP_DRAWABLE(new_layer_mask)->offset_y = GIMP_DRAWABLE(layer_mask)->offset_y;
  GIMP_CHANNEL(new_layer_mask)->show_masked = GIMP_CHANNEL(layer_mask)->show_masked;

  /*  copy the contents across layer masks  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer_mask)->tiles,
                  0, 0,
                  0, 0,
                  FALSE);

  pixelarea_init (&destPR, GIMP_DRAWABLE(new_layer_mask)->tiles, 
                  0, 0,
                  0, 0,
                  TRUE);

  copy_area (&srcPR, &destPR);

  /*  free up the layer_mask_name memory  */
  g_free (layer_mask_name);

  return new_layer_mask;
}


LayerMask *
layer_mask_get_ID (int ID)
{
  CanvasDrawable *drawable;
  drawable = drawable_get_ID (ID);
  if (drawable && GIMP_IS_LAYER_MASK (drawable)) 
    return GIMP_LAYER_MASK (drawable);
  else
    return NULL;
}

void
layer_mask_delete (LayerMask * layermask)
{
  gtk_object_unref (GTK_OBJECT (layermask));
}

LayerMask *
layer_mask_ref (LayerMask *mask)
{
  gtk_object_ref  (GTK_OBJECT (mask));
  gtk_object_sink (GTK_OBJECT (mask));
  return mask;
}


void
layer_mask_unref (LayerMask *mask)
{
  gtk_object_unref (GTK_OBJECT (mask));
}

void
channel_layer_mask (Channel *mask, Layer * layer)
{
  PixelArea srcPR, destPR;
  int x1, y1, x2, y2;
  COLOR16_NEW (black_color, drawable_tag(GIMP_DRAWABLE(mask)));
 
  COLOR16_INIT (black_color);
  palette_get_black (&black_color);

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->tiles, 
                  0, 0,
                  0, 0,
                  TRUE);
  
  color_area (&destPR, &black_color);

  x1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x,
               0, drawable_width (GIMP_DRAWABLE(mask)));
  y1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y,
               0, drawable_height (GIMP_DRAWABLE(mask)));
  x2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer)),
               0, drawable_width (GIMP_DRAWABLE(mask)));
  y2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer)),
               0, drawable_height (GIMP_DRAWABLE(mask)));

  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer->mask)->tiles,
                  (x1 - GIMP_DRAWABLE(layer)->offset_x), (y1 - GIMP_DRAWABLE(layer)->offset_y),
                  (x2 - x1), (y2 - y1),
                  FALSE);

  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->tiles, 
                  x1, y1,
                  (x2 - x1), (y2 - y1),
                  TRUE);

  copy_area (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}
