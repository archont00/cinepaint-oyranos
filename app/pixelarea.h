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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PIXEL_AREA_H__
#define __PIXEL_AREA_H__

#include "tag.h"
#include "canvas.h"
#include "../lib/wire/c_typedefs.h"

typedef struct BoundBox BoundBox;


/* how to handle hitting the edge when iterating */
typedef enum 
{
  EDGETYPE_NONE  = 0,
  EDGETYPE_NEXT  = 1,  /* go to next row, stop at bottom */
  EDGETYPE_WRAP  = 2   /* wrap right->left bottom->top */
} EdgeType;


struct BoundBox
{
  /* coords of upper left corner */
  gint x1, y1;

  /* coords of lower right corner */
  gint x2, y2;
};


struct PixelArea
{
  /* the image we're iterating over */
  Canvas * canvas;

  /* the total area */
  BoundBox area;

  /* the current chunk */
  BoundBox chunk;

  /* how to ref this area when iterating */
  RefType reftype;

  /* how to handle hitting the edge */
  EdgeType edgetype;

  /* cached values for current chunk */
  Tag tag;
  guchar * data;
  guint rowstride;
  guint is_reffed;
};



/*  PixelArea functions  */
void              pixelarea_init          (PixelArea *, Canvas *,
                                           int x, int y, int w, int h,
                                           int will_dirty);

void              pixelarea_init2         (PixelArea *, Canvas *,
                                           gint, gint, gint, gint,
                                           RefType, EdgeType);

void              pixelarea_getdata       (PixelArea *, PixelRow *, int);
Tag               pixelarea_tag           (PixelArea *);

int               pixelarea_areawidth     (PixelArea *);
int               pixelarea_areaheight    (PixelArea *);

int               pixelarea_x             (PixelArea *);
int               pixelarea_y             (PixelArea *);
int               pixelarea_width         (PixelArea *);
int               pixelarea_height        (PixelArea *);
int               pixelarea_bytes         (PixelArea *);

guchar *          pixelarea_data          (PixelArea *);
int               pixelarea_rowstride     (PixelArea *);
guint             pixelarea_ref           (PixelArea *);
guint             pixelarea_unref         (PixelArea *);


/* pixel area iterators */
void *            pixelarea_register       (int, ...);
void *            pixelarea_register_noref (int, ...);
void *            pixelarea_process        (void *);
void              pixelarea_process_stop   (void *);


/* these belong elsewhere */
void              pixelarea_copy_row      (PixelArea *, PixelRow *,
                                           int, int, int, int);
void              pixelarea_copy_col      (PixelArea *, PixelRow *,
                                           int, int, int, int);
void              pixelarea_write_row     (PixelArea *, PixelRow *,
                                           int, int, int);
void              pixelarea_write_col     (PixelArea *, PixelRow *,
                                           int, int, int);

#endif /* __PIXELAREA_H__ */
