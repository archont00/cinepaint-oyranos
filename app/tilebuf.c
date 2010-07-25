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

#include <sys/types.h>
#include <string.h>

#include "tilebuf.h"
#include "trace.h"
#include "../lib/wire/iodebug.h"


#define TILE16_WIDTH   128 
#define TILE16_HEIGHT  128 

typedef struct Tile16 Tile16;



struct TileBuf
{
  Tag        tag;
  int        width;
  int        height;
  Canvas *   canvas;

  Tile16   * tiles16;

  int        bytes;
};

struct Tile16
{
  short     is_alloced;
  short     ref_count;
  guchar  * data;
};



static int  tile16_index       (TileBuf *, int, int);

#define tile16_xoffset(t,x) (x)%TILE16_WIDTH
#define tile16_yoffset(t,y) (y)%TILE16_HEIGHT



TileBuf * 
tilebuf_new  (
              Tag tag,
              int w,
              int h,
              Canvas * c
              )
{
  TileBuf *t;
  int n;
  
  t = (TileBuf *) g_malloc (sizeof (TileBuf));

  t->tag = tag;
  t->width = w;
  t->height = h;
  t->canvas = c;
  
  n = tile16_index (t, w - 1, h - 1) + 1;
  t->tiles16 = g_new (Tile16, n);
  while (n--)
    {
      t->tiles16[n].is_alloced = FALSE;
      t->tiles16[n].ref_count = 0;
      t->tiles16[n].data = NULL;
    }

  t->bytes = tag_bytes (tag);
  
  return t;
}


void 
tilebuf_delete  (
                 TileBuf * t
                 )
{
  if (t)
    {
      if (t->tiles16)
        {
          int n = tile16_index (t, t->width - 1, t->height - 1) + 1;
          while (n--)
            if (t->tiles16[n].data)
              g_free (t->tiles16[n].data);
#ifdef _DEBUG
		memset(t->tiles16,0,sizeof(Tile16)); /*rsr*/
#endif
          g_free (t->tiles16);
		t->tiles16=0;
        }
#ifdef _DEBUG
	 memset(t,0,sizeof(TileBuf)); /*rsr*/
#endif
      g_free (t);
	 d_heap();
    }
}


void
tilebuf_info (
              TileBuf * t
              )
{
  if (t)
    {
      trace_begin ("TileBuf 0x%x", t);
      trace_printf ("%d by %d tiled buffer", t->width, t->height);
      trace_printf ("%s %s %s",
                    tag_string_precision (tag_precision (tilebuf_tag (t))),
                    tag_string_format (tag_format (tilebuf_tag (t))),
                    tag_string_alpha (tag_alpha (tilebuf_tag (t))));
      if (t->tiles16)
        {
          int i = tile16_index (t, t->width-1, t->height-1) + 1;
          while (i--)
            {
              if (t->tiles16[i].is_alloced)
                {
                  trace_printf ("  Tile %d, ref %d, data %x",
                                i, t->tiles16[i].ref_count, t->tiles16[i].data);
                }
            }
        }
      else
        {
          trace_printf ("  No tiles");
        }
      trace_end ();
    }
}


Tag 
tilebuf_tag  (
              TileBuf * t
              )
{
  if (t)
    return t->tag;
  return tag_null ();
}


Precision 
tilebuf_precision  (
                    TileBuf * t
                    )
{
  return tag_precision (tilebuf_tag (t));
}


Format 
tilebuf_format  (
                 TileBuf * t
                 )
{
  return tag_format (tilebuf_tag (t));
}


Alpha 
tilebuf_alpha  (
                TileBuf * t
                )
{
  return tag_alpha (tilebuf_tag (t));
}


guint 
tilebuf_width  (
                TileBuf * t
                )
{
  if (t)
    return t->width;
  return 0;
}


guint 
tilebuf_height  (
                 TileBuf * t
                 )
{
  if (t)
    return t->height;
  return 0;
}









RefRC 
tilebuf_portion_refro  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;

  int i = tile16_index (t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      
      if (tile16->is_alloced == FALSE)
        if (canvas_autoalloc (t->canvas) == AUTOALLOC_ON)
          (void) tilebuf_portion_alloc (t, x, y);
      
      if (tile16->is_alloced == TRUE)
        {
          tile16->ref_count++;
          rc = REFRC_OK;
        }
    }
  
  return rc;
}


RefRC 
tilebuf_portion_refrw  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;

  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      
      if (tile16->is_alloced == FALSE)
        if (canvas_autoalloc (t->canvas) == AUTOALLOC_ON)
          (void) tilebuf_portion_alloc (t, x, y);

      if (tile16->is_alloced == TRUE)
        {
          tile16->ref_count++;
          rc = REFRC_OK;
        }
    }
  
  return rc;
}


RefRC 
tilebuf_portion_unref  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;

  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      if (tile16->ref_count > 0)
        tile16->ref_count--;
      else
        g_warning ("tilebuf unreffing a tile with ref_count==0");
      
      rc = REFRC_OK;
    }

  return rc;
}


guint 
tilebuf_portion_width  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      int offset = tile16_xoffset (t, x);
      int base = x - offset;
      if (base+TILE16_WIDTH > t->width)
        return tile16_xoffset (t, t->width) - offset;
      else
        return TILE16_WIDTH - offset;
    }
  return 0;
}


guint 
tilebuf_portion_height  (
                         TileBuf * t,
                         int x,
                         int y
                         )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      int offset = tile16_yoffset (t, y);
      int base = y - offset;
      if (base+TILE16_HEIGHT > t->height)
        return tile16_yoffset (t, t->height) - offset;
      else
        return TILE16_HEIGHT - offset;
    }
  return 0;
}


guint 
tilebuf_portion_y  (
                    TileBuf * t,
                    int x,
                    int y
                    )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      return y - tile16_yoffset (t, y);
    }
  return 0;
}


guint 
tilebuf_portion_x  (
                    TileBuf * t,
                    int x,
                    int y
                    )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      return x - tile16_xoffset (t, x);
    }
  return 0;
}


guchar * 
tilebuf_portion_data  (
                       TileBuf * t,
                       int x,
                       int y
                       )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      if (tile16->data)
        {
          return (tile16->data + (t->bytes *
                                ((tile16_yoffset (t, y) * TILE16_WIDTH) +
                                 tile16_xoffset (t, x))));
        }
    }
  return NULL;
}


guint 
tilebuf_portion_rowstride  (
                            TileBuf * t,
                            int x,
                            int y
                            )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    return TILE16_WIDTH * t->bytes;
  return 0;
}


guint
tilebuf_portion_alloced (
                         TileBuf * t,
                         int x,
                         int y
                         )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      return tile16->is_alloced;
    }
  return FALSE;
}


guint 
tilebuf_portion_alloc  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      if (tile16->is_alloced == TRUE)
        {
          return TRUE;
        }
      else
        {
          int n = TILE16_WIDTH * TILE16_HEIGHT * t->bytes;
          tile16->data = g_malloc (n);
          if (tile16->data)
            {
              memset (tile16->data, 0, n);
              tile16->is_alloced = TRUE;
              if (canvas_portion_init (t->canvas,
                                       x - tile16_xoffset (t, x),
                                       y - tile16_yoffset (t, y),
                                       TILE16_WIDTH, TILE16_HEIGHT) != TRUE)
                {
                  g_warning ("tilebuf failed to init portion...");
                }
              return TRUE;
            }
        }
    }
  return FALSE;
}


guint 
tilebuf_portion_unalloc  (
                          TileBuf * t,
                          int x,
                          int y
                          )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile16 = &t->tiles16[i];
      if (tile16->is_alloced == TRUE)
        {
          if (tile16->ref_count != 0)
            {
              g_warning ("Unallocing a reffed tile.  expect a core...\n");
            }
          g_free (tile16->data);
          tile16->data = NULL;
          tile16->is_alloced = FALSE;
          return TRUE;
        }
    }
  return FALSE;
}



static int
tile16_index (
              TileBuf * t,
              int x,
              int y
              )
{
  if (t && (x < t->width) && (y < t->height))    
    {
      x = x / TILE16_WIDTH;
      y = y / TILE16_HEIGHT;
      return (y * ((t->width + TILE16_WIDTH - 1) / TILE16_WIDTH) + x);
    }
  return -1;
}



