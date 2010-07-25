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
#include <math.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "channel.h"
#include "compat.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "layer.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "undo.h"
#include "object.h"

#include "canvas.h"
#include "tilebuf.h"


enum {
  INVALIDATE_PREVIEW,
  LAST_SIGNAL
};


static void gimp_drawable_class_init (CanvasDrawableClass *klass);
static void gimp_drawable_init	     (CanvasDrawable      *drawable);
static void gimp_drawable_destroy    (GtkObject		*object);

static gint drawable_signals[LAST_SIGNAL] = { 0 };

static CanvasDrawableClass *parent_class = NULL;

guint
gimp_drawable_get_type ()
{
  static guint drawable_type = 0;

  if (!drawable_type)
    {
#if GTK_MAJOR_VERSION > 1
      static const GTypeInfo drawable_info =
      {
        sizeof (CanvasDrawableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_drawable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (CanvasDrawable),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_drawable_init,
      };
#if 0
      static const GInterfaceInfo pickable_iface_info =
      {
        (GInterfaceInitFunc) gimp_drawable_pickable_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };
#endif

      drawable_type = g_type_register_static (GTK_TYPE_OBJECT,
                                              "CanvasDrawable",
                                              &drawable_info, 0);
#if 0
      g_type_add_interface_static (drawable_type, GIMP_TYPE_PICKABLE,
                                   &pickable_iface_info);
#endif
#else
      GtkTypeInfo drawable_info =
      {
	"CanvasDrawable",
	sizeof (CanvasDrawable),
	sizeof (CanvasDrawableClass),
	(GtkClassInitFunc) gimp_drawable_class_init,
	(GtkObjectInitFunc) gimp_drawable_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      drawable_type = gtk_type_unique (GTK_TYPE_DATA, &drawable_info);
#endif
    }
  return drawable_type;
}

static void
gimp_drawable_class_init (CanvasDrawableClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

#if GTK_MAJOR_VERSION > 1
  parent_class = g_type_class_peek_parent (class);
#else
  parent_class = gtk_type_class (GTK_TYPE_DATA);
#endif

  drawable_signals[INVALIDATE_PREVIEW] =
    gtk_signal_new ("invalidate_preview",
		    GTK_RUN_LAST,
		    G_TYPE_FROM_CLASS (object_class),
		    GTK_SIGNAL_OFFSET (CanvasDrawableClass, invalidate_preview),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

#if GTK_MAJOR_VERSION < 2
  gtk_object_class_add_signals (object_class, (guint *)drawable_signals, LAST_SIGNAL);
#endif

  object_class->destroy = gimp_drawable_destroy;
}


/*
 *  Static variables
 */
int global_drawable_ID = 1;
static GHashTable *drawable_table = NULL;

/**************************/
/*  Function definitions  */

static guint
drawable_hash (gconstpointer v)
{
  return (guint) v;
}

static gint
drawable_hash_compare (gconstpointer v1, gconstpointer v2)
{
  return ((guint) v1) == ((guint) v2);
}

CanvasDrawable*
drawable_get_ID (int drawable_id)
{
  g_return_val_if_fail ((drawable_table != NULL), NULL);

  return (CanvasDrawable*) g_hash_table_lookup (drawable_table, 
					      (gpointer) drawable_id);
}

Tag
drawable_tag (
              CanvasDrawable *drawable
              )
{
  return (drawable ? canvas_tag (drawable->tiles) : tag_null ());
}

int 
drawable_ID  (
              CanvasDrawable * drawable
              )
{
  return (drawable ? drawable->ID : -1);
}

void 
drawable_apply_image  (
                       CanvasDrawable * drawable,
                       int x1,
                       int y1,
                       int x2,
                       int y2,
		       int x3,
		       int y3,
                       Canvas * tiles
                       )
{
  g_return_if_fail (drawable != NULL);

  if (! tiles)
    undo_push_image (gimage_get_ID (drawable->gimage_ID), drawable, x1, y1, x2, y2, x3, y3);
  else
    undo_push_image_mod (gimage_get_ID (drawable->gimage_ID), drawable, x1, y1, x2, y2, x3, y3, tiles);
  
}


void 
drawable_merge_shadow  (
                        CanvasDrawable * drawable,
                        int undo
                        )
{
  GImage * gimage;
  int x1, y1, x2, y2;

  g_return_if_fail (drawable != NULL);

  gimage = drawable_gimage (drawable);
  g_return_if_fail (gimage != NULL);

  
  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  gimage_apply_painthit (gimage, drawable,
                         NULL, gimage->shadow,
                         x1, y1,
                         (x2 - x1), (y2 - y1),
                         undo, OPAQUE_OPACITY,
                         REPLACE_MODE, x1, y1);
}


void 
drawable_fill  (
                CanvasDrawable * drawable,
                int fill_type
                )
{
  g_return_if_fail (drawable != NULL);

  {
    PixelArea dest_area;
    COLOR16_NEW (paint, drawable_tag (drawable));

    /* init the fill color */
    COLOR16_INIT (paint);
    switch (fill_type)
      {
      case BACKGROUND_FILL:
        palette_get_background (&paint);
        break;
      case FOREGROUND_FILL:
        palette_get_foreground (&paint);
        break;
      case WHITE_FILL:
        palette_get_white (&paint);
        break;
      case TRANSPARENT_FILL:
        palette_get_transparent (&paint);
        break;
      case NO_FILL:
      default:
        return;
      }

    pixelarea_init (&dest_area, drawable_data (drawable),
                    0, 0,
                    0, 0,
                    TRUE);

    color_area (&dest_area, &paint);
  
    drawable_update (drawable,
                     0, 0,
                     drawable_width (drawable), drawable_height (drawable));
  }
}


void 
drawable_update  (
                  CanvasDrawable * drawable,
                  int x,
                  int y,
                  int w,
                  int h
                  )
{
  GImage *gimage;
  int offset_x, offset_y;

  g_return_if_fail (drawable != NULL);

  gimage = drawable_gimage (drawable);
  g_return_if_fail (gimage != NULL);

  drawable_offsets (drawable, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;

  if (w == 0)
    w = drawable_width (drawable);
  
  if (h == 0)
    h = drawable_height (drawable);

  gdisplays_update_area (gimage->ID, x, y, w, h);

  /*  invalidate the preview  */
  drawable_invalidate_preview (drawable);
}


int 
drawable_mask_bounds  (
                       CanvasDrawable * drawable,
                       int * x1,
                       int * y1,
                       int * x2,
                       int * y2
                       )
{
  GImage *gimage;
  int off_x, off_y;

  g_return_val_if_fail ((drawable != NULL), FALSE);

  gimage = drawable_gimage (drawable);
  g_return_val_if_fail ((gimage != NULL), FALSE);

  if (gimage_mask_bounds (gimage, x1, y1, x2, y2))
    {
      drawable_offsets (drawable, &off_x, &off_y);
      *x1 = BOUNDS (*x1 - off_x, 0, drawable_width (drawable));
      *y1 = BOUNDS (*y1 - off_y, 0, drawable_height (drawable));
      *x2 = BOUNDS (*x2 - off_x, 0, drawable_width (drawable));
      *y2 = BOUNDS (*y2 - off_y, 0, drawable_height (drawable));
      return TRUE;
    }
  else
    {
      *x2 = drawable_width (drawable);
      *y2 = drawable_height (drawable);
      return FALSE;
    }
}


void 
drawable_invalidate_preview  (
                              CanvasDrawable * drawable
                              )
{
  GImage *gimage;

  g_return_if_fail (drawable != NULL);

  drawable->preview_valid = FALSE;

  gtk_signal_emit (GTK_OBJECT(drawable), drawable_signals[INVALIDATE_PREVIEW]);

  gimage = drawable_gimage (drawable);
  if (gimage)
    {
      gimage->comp_preview_valid[0] = FALSE;
      gimage->comp_preview_valid[1] = FALSE;
      gimage->comp_preview_valid[2] = FALSE;
      gimage->comp_preview_valid[3] = !(tag_alpha (drawable_tag (drawable)) == ALPHA_YES);
    }
}


int
drawable_dirty (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), 0);

  /*return drawable->dirty = (drawable->dirty < 0) ? 2 : drawable->dirty + 1;*/
  drawable->dirty++;
  return drawable->dirty;
}


int
drawable_clean (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), 0);

  /*return drawable->dirty = (drawable->dirty <= 0) ? 0 : drawable->dirty - 1;*/
  drawable->dirty--;
  return drawable->dirty;
}

/* Given a drawable, Look up an image based on the image ID */
GImage *
drawable_gimage (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), NULL);

  return gimage_get_ID (drawable->gimage_ID);
}


int
drawable_has_alpha (CanvasDrawable *drawable)
{
  if (tag_alpha (drawable_tag (drawable)) == ALPHA_YES)
    return TRUE;
  return FALSE;
}


int 
drawable_visible (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), FALSE);

  return drawable->visible;
}

char *
drawable_name (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), "");

  return drawable->name;
}

int
drawable_color (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), FALSE);

  switch (tag_format (drawable_tag (drawable)))
    {
    case FORMAT_RGB:
      return TRUE;
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      return FALSE;
    }

  return FALSE;
}


int
drawable_gray (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), FALSE);

  switch (tag_format (drawable_tag (drawable)))
    {
    case FORMAT_GRAY:
      return TRUE;
    case FORMAT_RGB:
    case FORMAT_INDEXED:
    case FORMAT_NONE:
      return FALSE;
    }
  return FALSE;
}


int
drawable_indexed (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), FALSE);

  switch (tag_format (drawable_tag (drawable)))
    {
    case FORMAT_INDEXED:
      return TRUE;
    case FORMAT_GRAY:
    case FORMAT_RGB:
    case FORMAT_NONE:
      return FALSE;
    }
  return FALSE;
}


Canvas *
drawable_data (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), NULL);

  return drawable->tiles;
}

Canvas *
drawable_shadow (CanvasDrawable *drawable)
{
  GImage *gimage;

  g_return_val_if_fail ((drawable != NULL), NULL);

  gimage = drawable_gimage (drawable);
  g_return_val_if_fail ((gimage != NULL), NULL);

  return gimage_shadow (gimage, drawable_width (drawable), drawable_height (drawable), 
                        drawable_tag (drawable));
}

int
drawable_bytes (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), 0);

  return tag_bytes (drawable_tag (drawable));
}


int
drawable_width (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), -1);

  return canvas_width (drawable->tiles);
}


int
drawable_height (CanvasDrawable *drawable)
{
  g_return_val_if_fail ((drawable != NULL), -1);

  return canvas_height (drawable->tiles);
}


void
drawable_offsets (CanvasDrawable *drawable, int *off_x, int *off_y)
{
  if (drawable) 
    {
      *off_x = drawable->offset_x;
      *off_y = drawable->offset_y;
    }
  else
    *off_x = *off_y = 0;
}

#define FIXME /* indexed color handling in general... */
unsigned char *
drawable_cmap (CanvasDrawable *drawable)
{
  GImage *gimage;

  if ((gimage = drawable_gimage (drawable)))
    return gimage->cmap;
  else
    return NULL;
}


Layer *
drawable_layer (CanvasDrawable *drawable)
{
  if (drawable && GIMP_IS_LAYER(drawable))
    return GIMP_LAYER (drawable);
  else
    return NULL;
}


LayerMask *
drawable_layer_mask (CanvasDrawable *drawable)
{
  if (drawable && GIMP_IS_LAYER_MASK(drawable))
    return GIMP_LAYER_MASK(drawable);
  else
    return NULL;
}


Channel *
drawable_channel    (CanvasDrawable *drawable)
{
  if (drawable && GIMP_IS_CHANNEL(drawable))
    return GIMP_CHANNEL(drawable);
  else
    return NULL;
}

void
drawable_deallocate (CanvasDrawable *drawable)
{
  g_return_if_fail (drawable != NULL);

  if (drawable->tiles)
    canvas_delete (drawable->tiles);
}

static void
gimp_drawable_init (CanvasDrawable *drawable)
{
  drawable->name = NULL;
  drawable->tiles = NULL;
  drawable->visible = FALSE;
  drawable->offset_x = drawable->offset_y = 0;
  drawable->dirty = FALSE;
  drawable->gimage_ID = -1;
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;

  drawable->ID = global_drawable_ID++;
  if (drawable_table == NULL)
    drawable_table = g_hash_table_new (drawable_hash,
				       drawable_hash_compare);
  g_hash_table_insert (drawable_table, (gpointer) drawable->ID,
		       (gpointer) drawable);
}

static void
gimp_drawable_destroy (GtkObject *object)
{
  CanvasDrawable *drawable;
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  g_hash_table_remove (drawable_table, (gpointer) drawable->ID);
  
  if (drawable->name)
    g_free (drawable->name);
  drawable->name = NULL;

  if (drawable->tiles)
    canvas_delete (drawable->tiles);
  drawable->tiles = NULL;

  if (drawable->preview)
    canvas_delete (drawable->preview);
  drawable->preview = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

  
void 
gimp_drawable_configure  (
                          CanvasDrawable * drawable,
                          int gimage_ID,
                          int width,
                          int height,
                          Tag tag,
                          StorageType storage,
                          char * name
                          )
{
  g_return_if_fail (drawable != NULL);

  if (!name)
    name = _("unnamed");
  
  if (drawable->name) 
    g_free (drawable->name);
  drawable->name = g_strdup(name);

  drawable->offset_x = 0;
  drawable->offset_y = 0;

  if (drawable->tiles)
    canvas_delete (drawable->tiles);
  drawable->tiles = canvas_new (tag, width, height, storage);

  drawable->dirty = FALSE;
  drawable->visible = TRUE;

  drawable->gimage_ID = gimage_ID;

  /*  preview variables  */
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;
}
  
