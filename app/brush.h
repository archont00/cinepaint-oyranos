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

#ifndef __GIMPBRUSH_H__
#define __GIMPBRUSH_H__

#include "canvas.h"
#include "objectP.h"
#include "vector2d.h"

typedef struct GimpBrush  GimpBrush, * GimpBrushP;

struct GimpBrush
{
  GimpObject gobject;
  char *     filename;    /*  actual filename--brush's location on disk  */
  char *     name;        /*  brush's name--for brush selection dialog   */
  int        spacing;     /*  brush's spacing                            */
  Vector2d   x_axis;      /*  for calculating brush spacing              */
  Vector2d   y_axis;      /*  for calculating brush spacing              */
  Canvas *  mask;         /*  the actual mask...                         */
};

typedef struct GimpBrushClass GimpBrushClass;

struct GimpBrushClass
{
  GimpObjectClass parent_class;
};


#define BRUSH_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gimp_brush_get_type(), GimpBrushClass)

#define GIMP_TYPE_BRUSH  (gimp_brush_get_type ())
#define GIMP_BRUSH(obj)  (GIMP_CHECK_CAST ((obj), GIMP_TYPE_BRUSH, GimpBrush))
#define GIMP_IS_BRUSH(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH))

GimpBrush * gimp_brush_load     (char *filename);
GtkType     gimp_brush_get_type (void);
Canvas   * gimp_brush_get_mask (GimpBrush *brush);
char      * gimp_brush_get_name (GimpBrush *brush);
void        gimp_brush_set_name (GimpBrush *brush, char *name);

#endif /* __GIMPBRUSH_H__ */
