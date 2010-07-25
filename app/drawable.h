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
#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__

#include <gtk/gtk.h>
#if GTK_MAJOR_VERSION < 2
#include <gtk/gtkdata.h>
#endif

#include "tag.h"
#include "canvas.h"
#include "../lib/wire/c_typedefs.h"

#define GIMP_TYPE_DRAWABLE         gimp_drawable_get_type ()
#define GIMP_DRAWABLE(obj)         GTK_CHECK_CAST (obj, GIMP_TYPE_DRAWABLE, CanvasDrawable)
#define GIMP_DRAWABLE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_DRAWABLE, GimpDrawableClass)
#define GIMP_IS_DRAWABLE(obj)      GTK_CHECK_TYPE (obj, GIMP_TYPE_DRAWABLE)

guint gimp_drawable_get_type (void);


/*  drawable access functions  */
Tag              drawable_tag                (CanvasDrawable *);
int		 drawable_ID		     (CanvasDrawable *);
void             drawable_apply_image        (CanvasDrawable *, 
					      int, int, int, int, int, int,  
					      Canvas *);
void             drawable_merge_shadow       (CanvasDrawable *, int);
void             drawable_fill               (CanvasDrawable *, int);
void             drawable_update             (CanvasDrawable *, 
					      int, int, int, int);
int              drawable_mask_bounds        (CanvasDrawable *,
					      int *, int *, int *, int *);
void             drawable_invalidate_preview (CanvasDrawable *);
int              drawable_dirty              (CanvasDrawable *);
int              drawable_clean              (CanvasDrawable *);
int              drawable_has_alpha          (CanvasDrawable *);
int              drawable_color              (CanvasDrawable *);
int              drawable_gray               (CanvasDrawable *);
int              drawable_indexed            (CanvasDrawable *);
Canvas * drawable_data               (CanvasDrawable *);
Canvas * drawable_shadow             (CanvasDrawable *);
int              drawable_bytes              (CanvasDrawable *);
int              drawable_width              (CanvasDrawable *);
int              drawable_height             (CanvasDrawable *);
int		 drawable_visible	     (CanvasDrawable *);
void             drawable_offsets            (CanvasDrawable *, int *, int *);
unsigned char *  drawable_cmap               (CanvasDrawable *);
char *		 drawable_name		     (CanvasDrawable *);

CanvasDrawable *   drawable_get_ID             (int);
void		 drawable_deallocate	     (CanvasDrawable *);
void             gimp_drawable_configure     (CanvasDrawable *, 
					      int, int, int, Tag, StorageType, char *);

/* rsr changed to CanvasDrawable from GimpDrawable */
typedef struct CanvasDrawableClass CanvasDrawableClass;

struct CanvasDrawable
{
  GtkObject data;

  int ID;				/* provides a unique ID */
  int gimage_ID;			/* ID of gimage owner */

  char *name;				/* name of drawable */
  int offset_x, offset_y;		/* offset of layer in image */

  Canvas *tiles;		/* tiles for drawable data */
  int visible;				/* controls visibility */
  int dirty;				/* dirty bit */

  Canvas *preview;		/* preview of the channel */
  int preview_valid;			/* is the preview valid? */
};

struct CanvasDrawableClass
{
  GtkObjectClass parent_class;

  void (*invalidate_preview) (GtkObject *);
};

#endif /* __DRAWABLE_H__ */
