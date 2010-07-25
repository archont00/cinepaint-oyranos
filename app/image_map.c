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
#include "appenv.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "image_map.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"

#define TileManager Canvas
#define tile_manager_destroy canvas_delete

#define WAITING 0
#define WORKING 1

#define WORK_DELAY 1

/*  Local structures  */
typedef struct ImageMap
{
  GDisplay *        gdisp;
  CanvasDrawable *    drawable;
  TileManager *     undo_tiles;
  ImageMapApplyFunc apply_func;
  void *            user_data;
  PixelArea         src_area, dest_area;
  void *            pr;
  int               state;
  gint              idle;
} _ImageMap;


static void image_map_allocate_undo (_ImageMap *);
static gint image_map_do (gpointer);

/**************************/
/*  Function definitions  */

Canvas*
image_map_get_src_canvas (ImageMap image_map)
{
  _ImageMap *_image_map;
  _image_map = (_ImageMap *) image_map;
  
  if (_image_map->undo_tiles)
    return _image_map->undo_tiles;
  else
    return NULL;
}

static gint
image_map_do (gpointer data)
{
  _ImageMap *_image_map;
  GImage *gimage;
  int x, y, w, h;

  _image_map = (_ImageMap *) data;

  if (! (gimage = drawable_gimage ( (_image_map->drawable))))
    {
      _image_map->state = WAITING;
      return FALSE;
    }

  x = pixelarea_x (&_image_map->dest_area);
  y = pixelarea_y (&_image_map->dest_area);
  w = pixelarea_width (&_image_map->dest_area);
  h = pixelarea_height (&_image_map->dest_area);
 
  /*  Process the pixel regions and apply the image mapping  */
  (* _image_map->apply_func) (&_image_map->src_area, &_image_map->dest_area, _image_map->user_data);

  /*  apply the results  */
  gimage_apply_painthit (gimage, _image_map->drawable,
                         NULL, gimage->shadow,
                         x, y,
                         w, h,
                         FALSE, 1.0, REPLACE_MODE, x, y);

  /*  display the results  */
  if (_image_map->gdisp)
    {
      drawable_update ( (_image_map->drawable), x, y, w, h);
      gdisplay_flush (_image_map->gdisp);
    }

  _image_map->pr = pixelarea_process (_image_map->pr);

  if (_image_map->pr == NULL)
    {
      _image_map->state = WAITING;
      gdisplays_flush ();
      return FALSE;
    }
  else
    return TRUE;
}

ImageMap
image_map_create (void *gdisp_ptr,
		  CanvasDrawable *drawable)
{
  _ImageMap *_image_map;

  _image_map = (_ImageMap *) g_malloc (sizeof (_ImageMap));
  _image_map->gdisp = (GDisplay *) gdisp_ptr;
  _image_map->drawable = drawable;
  _image_map->undo_tiles = NULL;
  _image_map->state = WAITING;

  return (ImageMap) _image_map;
}

void
image_map_apply (ImageMap           image_map,
		 ImageMapApplyFunc  apply_func,
		 void              *user_data)
{
  _ImageMap *_image_map;
  int x1, y1, x2, y2;

  _image_map = (_ImageMap *) image_map;
  _image_map->apply_func = apply_func;
  _image_map->user_data = user_data;

  /*  If we're still working, remove the timer  */
  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);
      pixelarea_process_stop (_image_map->pr);
      _image_map->pr = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  The application should occur only within selection bounds  */
  drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);

  /*  If undo canvas doesnt exist allocate it  */
  if (!_image_map->undo_tiles) 
    image_map_allocate_undo(_image_map); 
    

  /*  Configure the src from the drawable data  */
  pixelarea_init (&_image_map->src_area, 
			_image_map->undo_tiles, 
			0, 0, x2 - x1, y2 - y1, 
			FALSE);

  /*  Configure the dest as the shadow buffer  */
  pixelarea_init (&_image_map->dest_area, 
                  drawable_shadow (_image_map->drawable), 
                  x1, y1,
                  x2 - x1, y2 - y1, 
                  TRUE);

  /*  Apply the image transformation to the pixels  */
  _image_map->pr = pixelarea_register (2, &_image_map->src_area, &_image_map->dest_area);

  /*  Start the intermittant work procedure  */
  _image_map->state = WORKING;
  _image_map->idle = gtk_idle_add (image_map_do, image_map);
}

void
image_map_allocate_undo(_ImageMap * _image_map )
{
  PixelArea undo;
  PixelArea canvas;
  gint x1, y1, x2, y2;
 
  drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);
     
  /*  Allocate new undo canvas  */
  _image_map->undo_tiles = canvas_new (drawable_tag (_image_map->drawable),
                                        x2 - x1,
                                        y2 - y1,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

  /*  Copy from the image to the new undo canvas  */
  pixelarea_init (&canvas, 
                  drawable_data (_image_map->drawable), 
                  x1, y1, x2 - x1, y2 - y1, 
                  FALSE);
  pixelarea_init (&undo, 
                  _image_map->undo_tiles, 
                  0, 0, x2 - x1, y2 - y1, 
                  TRUE);
  
  copy_area (&canvas, &undo);
}

void
image_map_commit (ImageMap image_map)
{
  _ImageMap *_image_map;
  int x1, y1, x2, y2;

  _image_map = (_ImageMap *) image_map;

  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);

      /*  Finish the changes  */
      while (image_map_do (image_map)) ;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  Register an undo step  */
  if (_image_map->undo_tiles)
    {
      drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);
      drawable_apply_image (_image_map->drawable,
                            0, 0, x2-x1, y2-y1, x1, y1,  
                            _image_map->undo_tiles);
    }

  g_free (_image_map);
}

void
image_map_abort (ImageMap image_map)
{
  _ImageMap *_image_map;
  PixelArea src_area, dest_area;

  _image_map = (_ImageMap *) image_map;

  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);
      pixelarea_process_stop (_image_map->pr);
      _image_map->pr = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  restore the original image  */
  if (_image_map->undo_tiles)
    {
      gint x1, y1, x2, y2;
      /*  Copy from the undo to the drawable canvas  */
      drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);
      pixelarea_init (&src_area, 
			_image_map->undo_tiles, 
			0, 0, 
			canvas_width (_image_map->undo_tiles),
			canvas_height (_image_map->undo_tiles), 
			FALSE);
      pixelarea_init (&dest_area, 
			drawable_data (_image_map->drawable), 
			x1, y1, x2 - x1, y2 - y1, 
			TRUE);
      copy_area (&src_area, &dest_area);

      /*  Update the area  */
      drawable_update ( (_image_map->drawable), x1, y1, x2 - x1, y2 - y1); 

      /*  Free the undo_tiles */
      tile_manager_destroy (_image_map->undo_tiles);
    }

  g_free (_image_map);
}

void
image_map_remove (ImageMap image_map)
{
  _ImageMap *_image_map;
  PixelArea src_area, dest_area;

  _image_map = (_ImageMap *) image_map;

  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);
      pixelarea_process_stop (_image_map->pr);
      _image_map->pr = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  restore the original image  */
  if (_image_map->undo_tiles)
    {
      gint x1, y1, x2, y2;
      /*  Copy from the undo to the drawable canvas  */
      drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);
      pixelarea_init (&src_area, 
			_image_map->undo_tiles, 
			0, 0, 
			canvas_width (_image_map->undo_tiles),
			canvas_height (_image_map->undo_tiles), 
			FALSE);
      pixelarea_init (&dest_area, 
			drawable_data (_image_map->drawable), 
			x1, y1, x2 - x1, y2 - y1, 
			TRUE);
      copy_area (&src_area, &dest_area);

      /*  Update the area  */
      drawable_update ( (_image_map->drawable), x1, y1, x2 - x1, y2 - y1); 

    }

}

