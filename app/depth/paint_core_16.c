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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../brush.h"
#include "../brushlist.h"
#include "../canvas.h"
#include "../draw_core.h"
#include "../drawable.h"
#include "../errors.h"
#include "float16.h"
#include "bfp.h"
#include "../gdisplay.h"
#include "../gimage.h"
#include "../gimage_mask.h"
#include "../layer.h"
#include "../noise.h"
#include "../paint_core_16.h"
#include "../paint_funcs_area.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../tools.h"
#include "../trace.h"
#include "../undo.h"
#include "../devices.h"

#define    SQR(x) ((x) * (x))

/* was formerly sqrt(x), wacom works better without modify , beku
 * The pencil should use sqrt while without it feels like a long hair brush.
 */
#define    PRESSURE_MODIFY(x)  ( x )

Canvas *  canvas_16_to_16                (Canvas *); /*IMAGEWORKS aland*/
Canvas *  canvas_16_to_8                 (Canvas *); /*IMAGEWORKS aland*/
Canvas *  canvas_16_to_float             (Canvas *); /*IMAGEWORKS thedoug*/
Canvas *  canvas_16_to_bfp               (Canvas *); 


/*  global variables--for use in the various paint tools  */
PaintCore16  non_gui_paint_core_16;
PaintCore16 spline_paint_core;

/*  local function prototypes  */
static void      paint_core_16_button_press    (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_button_release  (Tool *, GdkEventButton *, gpointer);
static void      paint_core_16_motion          (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void      paint_core_16_control         (Tool *, int, gpointer);
static void      paint_core_16_no_draw         (Tool *);


static void      painthit_init                 (PaintCore16 *, CanvasDrawable *, Canvas *);

static void
brush_to_canvas_tiles(
                          PaintCore16 * paint_core,
                          Canvas * brush_mask,
                          gfloat brush_opacity 
			);

static void
canvas_tiles_to_brush(
                          PaintCore16 * paint_core,
                          Canvas * brush_mask,
                          gfloat brush_opacity 
			);

static void
canvas_tiles_to_canvas_buf(
                          PaintCore16 * paint_core);
static void
brush_to_canvas_buf(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity
                             );

static void      painthit_finish               (CanvasDrawable *, PaintCore16 *,
                                                Canvas *);

static Canvas *  brush_mask_get                (PaintCore16 *, Canvas *, int, gdouble);
static Canvas *  brush_mask_subsample          (Canvas *, double, double, double);
static Canvas *  brush_mask_solidify           (PaintCore16 *, Canvas *);
static Canvas *  brush_mask_noise  	       (PaintCore16 *, Canvas *);

static void brush_solidify_mask_u8 ( Canvas *, Canvas *);
static void brush_solidify_mask_u16 ( Canvas *, Canvas *);
static void brush_solidify_mask_float ( Canvas *, Canvas *);
static void brush_solidify_mask_float16 ( Canvas *, Canvas *);

static void brush_mask_noise_u8 (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_u16 (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_float (PaintCore16 *, Canvas *, Canvas *);
static void brush_mask_noise_float16 (PaintCore16 *, Canvas *, Canvas *);
/*  local function prototypes  */
static void      paint_core_calculate_brush_size (Canvas *mask,
						  gdouble  scale,
						  gint    *width,
						  gint    *height,
                                                  gdouble *ratio);
static Canvas * paint_core_scale_mask           (Canvas  *brush_mask,
                                                 gdouble  pos_x,
                                                 gdouble  pos_y,      
						 gdouble  scale);
static Canvas  *scale_brush     = NULL;

/* ------------------------------------------------------------------------

   PaintCore16 Frontend

*/
Tool * 
paint_core_16_new  (
                    int type
                    )
{
  Tool * tool;
  PaintCore16 * private;

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (PaintCore16 *) g_malloc_zero (sizeof (PaintCore16));

  private->core = draw_core_new (paint_core_16_no_draw);

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->gdisp_ptr = NULL;
  tool->private = (void *) private;
  tool->preserve = TRUE;
  
  tool->button_press_func = paint_core_16_button_press;
  tool->button_release_func = paint_core_16_button_release;
  tool->motion_func = paint_core_16_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = paint_core_16_cursor_update;
  tool->control_func = paint_core_16_control;

  private->noise_mode = 1.0;
  private->undo_tiles = NULL;
  private->canvas_tiles = NULL;
  private->canvas_buf = NULL;
  private->orig_buf = NULL;
  private->canvas_buf_width = 0;
  private->canvas_buf_height = 0;
  private->noise_mask = NULL;
  private->solid_mask = NULL;

  private->linked_undo_tiles = NULL;
  private->linked_canvas_buf = NULL;
  private->painthit_setup = NULL;
  private->linked_drawable = NULL;
  private->drawable = NULL;

  return tool;
}


void 
paint_core_16_free  (Tool * tool)
{
  PaintCore16 * paint_core;

  paint_core = (PaintCore16 *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE && paint_core->core)
    draw_core_stop (paint_core->core, tool);

  /*  Free the selection core  */
  if (paint_core->core)
    draw_core_free (paint_core->core);

  /*  Cleanup memory  */
  paint_core_16_cleanup (paint_core);

  /*  Free the paint core  */
  g_free (paint_core);
}

int 
paint_core_16_init_linked  (
                     PaintCore16 * paint_core,
                     CanvasDrawable *drawable,
                     CanvasDrawable *linked_drawable,
                     double x,
                     double y
                     )
{
  GimpBrushP brush;

  
  paint_core->drawable = drawable; 
  paint_core->linked_drawable = linked_drawable; 
  paint_core->curx = x;
  paint_core->cury = y;

  if (paint_core == &non_gui_paint_core || paint_core==&spline_paint_core)
    {
      paint_core->startpressure = paint_core->lastpressure = paint_core->curpressure = 0.4;
      paint_core->startxtilt = paint_core->lastxtilt = paint_core->curxtilt = 0.1;
      paint_core->startytilt = paint_core->lastytilt = paint_core->curytilt = 0.1;
    }

  /* get the brush mask */
  if (!(brush = get_active_brush ()))
    {
      g_message (_("No brushes available for use with this tool."));
      return FALSE;
    }
 
  paint_core->spacing =
    (double) MAX (canvas_height (brush->mask), canvas_width (brush->mask)) *
    ((double) gimp_brush_get_spacing () / 100.0);
  paint_core->spacing_scale = 1.0;

  if (paint_core->spacing < 1.0)
    paint_core->spacing = 1.0;
  
  paint_core->brush_mask = brush->mask;
  paint_core->noise_mask = NULL;
  paint_core->solid_mask = NULL;
  paint_core->subsampled_mask = NULL;

  /*  Allocate the undo structure  */
#ifdef NO_TILES
  if(!paint_core->undo_tiles)
  {  paint_core->undo_tiles = canvas_new (drawable_tag (drawable),
                           drawable_width (drawable),
                           drawable_height (drawable),
						  STORAGE_FLAT);
  }
#else
  if (paint_core->undo_tiles)
    canvas_delete (paint_core->undo_tiles);
  paint_core->undo_tiles = canvas_new (drawable_tag (drawable),
                           drawable_width (drawable),
                           drawable_height (drawable),
						  STORAGE_TILED);
#endif
  canvas_set_autoalloc (paint_core->undo_tiles, AUTOALLOC_OFF);

/* For linked painting */ 

  if (linked_drawable)
    {
#ifdef NO_TILES
      if(!paint_core->linked_undo_tiles)
      {  paint_core->linked_undo_tiles = canvas_new (drawable_tag (linked_drawable),
			       drawable_width (linked_drawable),
			       drawable_height (linked_drawable),
						  STORAGE_FLAT);
	 }
#else
      if (paint_core->linked_undo_tiles)
	canvas_delete (paint_core->linked_undo_tiles);
      paint_core->linked_undo_tiles = canvas_new (drawable_tag (linked_drawable),
			       drawable_width (linked_drawable),
			       drawable_height (linked_drawable),
						  STORAGE_TILED);
#endif
      canvas_set_autoalloc (paint_core->linked_undo_tiles, AUTOALLOC_OFF);
    }

  /*  Allocate the cumulative brush stroke mask --only need one of these  */

#ifdef NO_TILES						  
  if(!paint_core->canvas_tiles)
  {  paint_core->canvas_tiles = canvas_new (tag_new (tag_precision (drawable_tag (drawable)),
                                      FORMAT_GRAY,
                                      ALPHA_NO),
                             drawable_width (drawable),
                             drawable_height (drawable),
						  STORAGE_FLAT);
  }
#else
  if (paint_core->canvas_tiles)
    canvas_delete (paint_core->canvas_tiles);
  paint_core->canvas_tiles = canvas_new (tag_new (tag_precision (drawable_tag (drawable)),
                                      FORMAT_GRAY,
                                      ALPHA_NO),
                             drawable_width (drawable),
                             drawable_height (drawable),
						  STORAGE_TILED);
#endif

  /*  Get the initial undo extents  */
  paint_core->x1 = paint_core->x2 = paint_core->curx;
  paint_core->y1 = paint_core->y2 = paint_core->cury;

  paint_core->distance = 0.0;

  return TRUE;
}


int 
paint_core_16_init  (
                     PaintCore16 * paint_core,
                     CanvasDrawable *drawable,
                     double x,
                     double y
                     )
{
  return paint_core_16_init_linked (paint_core, drawable, NULL, x, y);
}

void 
paint_core_16_interpolate  (PaintCore16 * paint_core,
                            CanvasDrawable *drawable)
{
  double dx, dy;
  double t;
  double initial;
  double dist;
  double total;
  double pressure;
  double spacing;

  /* wacom */
  gdouble dpressure, dxtilt, dytilt;


  /* see how far we've moved */
  dx = paint_core->curx - paint_core->lastx;
  dy = paint_core->cury - paint_core->lasty;
  dpressure = paint_core->curpressure - paint_core->lastpressure;
  dxtilt    = paint_core->curxtilt    - paint_core->lastxtilt;
  dytilt    = paint_core->curytilt    - paint_core->lastytilt;

  
  /* bail if we haven't moved */
  if (!dx && !dy && !dpressure && !dxtilt && !dytilt)
    {
#if 0
      printf("no movement occured!\n");
#endif
      return;
    }

  dist = sqrt (SQR (dx) + SQR (dy));
  total = dist + paint_core->distance;
  initial = paint_core->distance;
  pressure = paint_core->lastpressure;

  paint_core->curx = paint_core->lastx;
  paint_core->cury = paint_core->lasty;
  paint_core->curpressure = paint_core->lastpressure;
  paint_core->curxtilt    = paint_core->lastxtilt;
  paint_core->curytilt    = paint_core->lastytilt;

  while (paint_core->distance < total)
    {
      spacing = MAX(paint_core->spacing * PRESSURE_MODIFY (paint_core->spacing_scale), 0.2);
      paint_core->distance += spacing;
      t = (paint_core->distance - initial) / dist;
      pressure = ((paint_core->distance - initial) * dpressure) / dist +
                 paint_core->lastpressure;

      if (paint_core->distance <= total)
	{

	  paint_core->curx = paint_core->lastx + dx * t;
	  paint_core->cury = paint_core->lasty + dy * t;
	  paint_core->curpressure = paint_core->lastpressure + dpressure * t;
	  paint_core->curxtilt    = paint_core->lastxtilt + dxtilt * t;
	  paint_core->curytilt    = paint_core->lastytilt + dytilt * t;
	  (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);
	}
    }

  paint_core->distance -= spacing;

}


void 
paint_core_16_finish  (
                       PaintCore16 * paint_core,
                       CanvasDrawable *drawable,
                       int tool_id
                       )
{
  GImage *gimage;
  PaintUndo *pu;


  if (! (gimage = drawable_gimage (drawable)))
{
   return;
}

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((paint_core->x2 == paint_core->x1) || (paint_core->y2 == paint_core->y1))
    return;

  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
  pu->tool_ID = tool_id;
  pu->lastx = paint_core->startx;
  pu->lasty = paint_core->starty;
  pu->lastpressure = paint_core->startpressure;
  pu->lastxtilt    = paint_core->startxtilt;
  pu->lastytilt    = paint_core->startytilt;

  /*  Push a paint undo  */
  undo_push_paint (gimage, pu);

  /*  push an undo  */
  {
    /* assign ownership of undo_tiles to undo system */
    drawable_apply_image (drawable,
                          paint_core->x1, paint_core->y1,
                          paint_core->x2, paint_core->y2,
                          paint_core->x1, paint_core->y1,
                          paint_core->undo_tiles);
     
    paint_core->undo_tiles = NULL;
  }

  /* push a linked undo if there is one */
  if (paint_core->linked_drawable)
  {
    pu = (PaintUndo *) g_malloc (sizeof (PaintUndo));
    pu->tool_ID = tool_id;
    pu->lastx = paint_core->startx;
    pu->lasty = paint_core->starty;
    undo_push_paint (gimage, pu);
    drawable_apply_image (paint_core->linked_drawable,
			  paint_core->x1, paint_core->y1,
			  paint_core->x2, paint_core->y2,
			  paint_core->x1, paint_core->y1,
			  paint_core->linked_undo_tiles);
     
    paint_core->linked_undo_tiles = NULL;
  }

  /*  push the group end  */
  undo_push_group_end (gimage);

  /*  invalidate the drawable--have to do it here, because
   *  it is not done during the actual painting.
   */
  drawable_invalidate_preview (drawable);
  if (paint_core->linked_drawable)
    drawable_invalidate_preview (paint_core->linked_drawable);


}


void 
paint_core_16_cleanup  (
                        PaintCore16 * paint_core 
                        )
{
  if (paint_core->undo_tiles)
    {
      canvas_delete (paint_core->undo_tiles);
      paint_core->undo_tiles = NULL;
    }

  if (paint_core->linked_undo_tiles)
    {
      canvas_delete (paint_core->linked_undo_tiles);
      paint_core->linked_undo_tiles = NULL;
    }

  if (paint_core->canvas_tiles)
    {
      canvas_delete (paint_core->canvas_tiles);
      paint_core->canvas_tiles = NULL;
    }

  if (paint_core->canvas_buf)
    {
      canvas_delete (paint_core->canvas_buf);
      paint_core->canvas_buf = NULL;
      paint_core->canvas_buf_height = 0;
      paint_core->canvas_buf_width = 0;
    }

  if (paint_core->linked_canvas_buf)
    {
      canvas_delete (paint_core->linked_canvas_buf);
      paint_core->linked_canvas_buf = NULL;
    }

  if (paint_core->orig_buf)
    {
      canvas_delete (paint_core->orig_buf);
      paint_core->orig_buf = NULL;
    }
}

static void 
paint_core_16_button_press  (Tool * tool,
                             GdkEventButton * bevent,
                             gpointer gdisp_ptr)
{
  PaintCore16 * paint_core;
  GDisplay * gdisp;
  CanvasDrawable * drawable;
  CanvasDrawable * linked_drawable;
  int draw_line = 0;
  double x, y;


  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) bevent->x, (double) bevent->y,
                                 &x, &y,
                                 TRUE);
  drawable = gimage_active_drawable (gdisp->gimage);

  if (GIMP_IS_LAYER (drawable))
    linked_drawable = gimage_linked_drawable (gdisp->gimage);
  else 
    linked_drawable = NULL; 
  
  if (! paint_core_16_init_linked (paint_core, drawable, linked_drawable, x, y))
    {
      return;
    }

  paint_core->state = bevent->state;

  /* wacom stuff */
#if GTK_MAJOR_VERSION > 1
      if (gdk_event_get_axis (bevent, GDK_AXIS_PRESSURE, &paint_core->curpressure))
        paint_core->curpressure = CLAMP (paint_core->curpressure, .0,
                                  1.);
      else
        paint_core->curpressure = 1.0;

      if (gdk_event_get_axis (bevent, GDK_AXIS_XTILT, &paint_core->curxtilt))
        paint_core->curxtilt = CLAMP (paint_core->curxtilt, -1.0,
                               1.0);
      else
        paint_core->curxtilt = 0.0;

      if (gdk_event_get_axis (bevent, GDK_AXIS_YTILT, &paint_core->curytilt))
        paint_core->curytilt = CLAMP (paint_core->curytilt, -1.0,
                               1.0);
      else
        paint_core->curytilt = 0.0;
#else
  paint_core->curpressure = bevent->pressure;
  paint_core->curxtilt = bevent->xtilt;
  paint_core->curytilt = bevent->ytilt;
#endif
  
  /*  if this is a new image, reinit the core vals  */
  if (gdisp_ptr != tool->gdisp_ptr ||
      ! (bevent->state & GDK_SHIFT_MASK))
    {
      /*  initialize some values  */
      paint_core->startx = paint_core->lastx = paint_core->curx;
      paint_core->starty = paint_core->lasty = paint_core->cury;
      paint_core->startpressure = paint_core->lastpressure = paint_core->curpressure;
      paint_core->startytilt    = paint_core->lastytilt    = paint_core->curytilt;
      paint_core->startxtilt    = paint_core->lastxtilt    = paint_core->curxtilt;
    }
  /*  If shift is down and this is not the first paint
   *  stroke, then draw a line from the last coords to the pointer
   */
  else if (bevent->state & GDK_SHIFT_MASK)
    {
      draw_line = 1;
      paint_core->startx = paint_core->lastx;
      paint_core->starty = paint_core->lasty;
      paint_core->startpressure = paint_core->lastpressure;
      paint_core->startxtilt    = paint_core->lastxtilt;
      paint_core->startytilt    = paint_core->lastytilt;
    }

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;
  tool->paused_count = 0;

  /*  pause the current selection and grab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionPause);

#if 0
  /* add motion memory if you press mod1 first */
  if (bevent->state & GDK_MOD1_MASK)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
  else
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);
#endif 

  /*  Let the specific painting function initialize itself  */
  (* paint_core->paint_func) (paint_core, drawable, INIT_PAINT);

  /*  Paint to the image  */
  if (draw_line)
    {
      paint_core_16_interpolate (paint_core, drawable);
      paint_core->lastx = paint_core->curx;
      paint_core->lasty = paint_core->cury;
      paint_core->lastpressure = paint_core->curpressure;
      paint_core->lastxtilt    = paint_core->curxtilt;
      paint_core->lastytilt    = paint_core->curytilt;
    }
  else
    (* paint_core->paint_func) (paint_core, drawable, MOTION_PAINT);

  gdisplay_flush (gdisp);
}


static void 
paint_core_16_button_release  (
                               Tool * tool,
                               GdkEventButton * bevent,
                               gpointer gdisp_ptr
                               )
{
  GDisplay * gdisp;
  GImage * gimage;
  PaintCore16 * paint_core;


  gdisp = (GDisplay *) gdisp_ptr;
  gimage = gdisp->gimage;
  paint_core = (PaintCore16 *) tool->private;

  /*  resume the current selection and ungrab the pointer  */
  gdisplays_selection_visibility (gdisp->gimage->ID, SelectionResume);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Let the specific painting function finish up  */
  (* paint_core->paint_func) (paint_core, gimage_active_drawable (gdisp->gimage), FINISH_PAINT);

  /*  Set tool state to inactive -- no longer painting */
  tool->state = INACTIVE;

  paint_core_16_finish (paint_core, gimage_active_drawable (gdisp->gimage), tool->ID);
  gdisplays_flush ();
}


static void 
paint_core_16_motion  (
                       Tool * tool,
                       GdkEventMotion * mevent,
                       gpointer gdisp_ptr
                       )
{
  GDisplay * gdisp;
  PaintCore16 * paint_core;

  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;

  gdisplay_untransform_coords_f (gdisp,
                                 (double) mevent->x, (double) mevent->y,
				 &paint_core->curx, &paint_core->cury,
                                 TRUE);

  paint_core->state = mevent->state;
  /*wacom */
#if GTK_MAJOR_VERSION > 1
      if (gdk_event_get_axis (mevent, GDK_AXIS_PRESSURE, &paint_core->curpressure))
        paint_core->curpressure = CLAMP (paint_core->curpressure, .0,
                                  1.);
      else
        paint_core->curpressure = 1.0;

      if (gdk_event_get_axis (mevent, GDK_AXIS_XTILT, &paint_core->curxtilt))
      {
# ifdef DEBUG_
        printf("curxtilt = %f",paint_core->curxtilt);
# endif
        paint_core->curxtilt = CLAMP (paint_core->curxtilt, -1.0,
                               1.0);
      } else
        paint_core->curxtilt = 0.0;

      if (gdk_event_get_axis (mevent, GDK_AXIS_YTILT, &paint_core->curytilt))
      {
# ifdef DEBUG_
        printf("curytilt = %f\n",paint_core->curytilt);
# endif
        paint_core->curytilt = CLAMP (paint_core->curytilt, -1.0,
                               1.0);
      } else
        paint_core->curytilt = 0.0;
#else
  paint_core->curpressure = mevent->pressure;
  paint_core->curxtilt = mevent->xtilt;
  paint_core->curytilt = mevent->ytilt;
#endif

  
  paint_core_16_interpolate (paint_core, gimage_active_drawable (gdisp->gimage));
 

  gdisplay_flush (gdisp);
  
  paint_core->lastx = paint_core->curx;
  paint_core->lasty = paint_core->cury;
  paint_core->lastpressure = paint_core->curpressure;
  paint_core->lastxtilt    = paint_core->curxtilt;
  paint_core->lastytilt    = paint_core->curytilt;
}


static void 
paint_core_16_cursor_update  (
                              Tool * tool,
                              GdkEventMotion * mevent,
                              gpointer gdisp_ptr
                              )
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  double x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords_f (gdisp,
                               mevent->x, mevent->y,
                               &x, &y,
                               FALSE);
  
  if ((layer = gimage_get_active_layer (gdisp->gimage)))
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
          x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
          y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
        {
          /*  One more test--is there a selected region?
           *  if so, is cursor inside?
           */
          if (gimage_mask_is_empty (gdisp->gimage))
            ctype = GDK_PENCIL;
          else if (gimage_mask_value (gdisp->gimage, x, y) != 0)
            ctype = GDK_PENCIL;
        }
    }
  
  gdisplay_install_tool_cursor (gdisp, ctype);

  if (tool->type == CLONE)
    {
      ((PaintCore *)tool->private)->curx = x;
      ((PaintCore *)tool->private)->cury = y;  
      (*((PaintCore *)tool->private)->cursor_func) ((PaintCore *)tool->private, 
						    GIMP_DRAWABLE(layer),
						    mevent->state); 
    }
    
}


static void 
paint_core_16_control  (
                        Tool * tool,
                        int action,
                        gpointer gdisp_ptr
                        )
{
  PaintCore16 * paint_core;
  GDisplay *gdisp;
  CanvasDrawable * drawable;

 
  gdisp = (GDisplay *) gdisp_ptr;
  paint_core = (PaintCore16 *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  switch (action)
    {
    case PAUSE :
      draw_core_pause (paint_core->core, tool);
      break;
    case RESUME :
      draw_core_resume (paint_core->core, tool);
      break;
    case HALT :
      (* paint_core->paint_func) (paint_core, drawable, FINISH_PAINT);
      paint_core_16_cleanup (paint_core);
      break;
    }
}


static void 
paint_core_16_no_draw  (
                        Tool * tool
                        )
{
  return;
}


/* ------------------------------------------------------------------------

   PaintCore16 Backend

*/
void 
paint_core_16_area_setup  (
                     PaintCore16 * paint_core,
                     CanvasDrawable * drawable
                     )
{
  Tag   tag;
  int   x, y;
  int   dw, dh;
  int   bw, bh;
  int   x1, y1, x2, y2;
  

  bw = canvas_width (paint_core->brush_mask);
  bh = canvas_height (paint_core->brush_mask);
  
  /* adjust the x and y coordinates to the upper left corner of the brush */  
  x = (int) paint_core->curx - bw / 2;
  y = (int) paint_core->cury - bh / 2;
  
  dw = drawable_width (drawable);
  dh = drawable_height (drawable);

#define PAINT_CORE_16_C_4_cw  
  x1 = CLAMP (x , 0, dw);
  y1 = CLAMP (y , 0, dh);
  x2 = CLAMP (x + bw + 1, 0, dw);
  y2 = CLAMP (y + bh + 1, 0, dh);
  /* save the boundaries of this paint hit */
  paint_core->x = x1;
  paint_core->y = y1;
  paint_core->w = x2 - x1;
  paint_core->h = y2 - y1;
  
  /* configure the canvas buffer */
  tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);
#ifdef NO_TILES
  if(!paint_core->canvas_buf)
  {  paint_core->canvas_buf = canvas_new (tag,
                           (x2 - x1), (y2 - y1),
                           STORAGE_FLAT);
  }
#else
  if (paint_core->canvas_buf)
    canvas_delete (paint_core->canvas_buf);

  paint_core->canvas_buf = canvas_new (tag,
                           (x2 - x1), (y2 - y1),
                           STORAGE_FLAT);
#endif
  paint_core->setup_mode = NORMAL_SETUP;
  paint_core->pre_mult = drawable_has_alpha(drawable) ? UNPRE_MULT_ALPHA : PRE_MULT_ALPHA;

  (*paint_core->painthit_setup) (paint_core, paint_core->canvas_buf); 

  /* configure the linked_canvas_buffer if there is one */
  if (paint_core->linked_drawable)
  {
    tag = tag_set_alpha (drawable_tag (paint_core->linked_drawable), drawable_has_alpha(drawable) ? 
		    ALPHA_NO:ALPHA_YES);

    if (paint_core->linked_canvas_buf)
      canvas_delete (paint_core->linked_canvas_buf);

    paint_core->linked_canvas_buf = canvas_new (tag,
			     (x2 - x1), (y2 - y1),
			     STORAGE_FLAT);
    paint_core->setup_mode = LINKED_SETUP;
    (*paint_core->painthit_setup) (paint_core, paint_core->linked_canvas_buf); 
  }

  paint_core->canvas_buf_width = (x2 - x1);
  paint_core->canvas_buf_height = (y2 - y1);
 
 
  
}



Canvas * 
paint_core_16_area_original  (
                              PaintCore16 * paint_core,
                              CanvasDrawable *drawable,
                              int x1,
                              int y1,
                              int x2,
                              int y2
                              )
{
  PixelArea srcPR, destPR, undoPR;
  Tag tag;

  /*tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);
  */tag = drawable_tag (drawable);
  if (paint_core->orig_buf)
    canvas_delete (paint_core->orig_buf);
  paint_core->orig_buf = canvas_new (tag, (x2 - x1), (y2 - y1), 
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

  
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  
  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&undoPR, paint_core->undo_tiles,
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&destPR, paint_core->orig_buf,
                  0, 0, (x2 - x1), (y2 - y1), TRUE);


  {
    void * pag;
    int h;
    
    for (pag = pixelarea_register_noref (3, &srcPR, &undoPR, &destPR);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        PixelRow srow, drow;
        PixelArea *s, *d;

        d = &destPR;
        {
          int x = pixelarea_x (&undoPR);
          int y = pixelarea_y (&undoPR);
          if (canvas_portion_alloced (paint_core->undo_tiles, x, y) == TRUE)
            s = &undoPR;
          else
            s = &srcPR;
        }
        
        if (pixelarea_ref (s) == TRUE)
          {
            if (pixelarea_ref (d) == TRUE)
              {
                h = pixelarea_height (s);
                while (h--)
                  {
                    pixelarea_getdata (s, &srow, h);
                    pixelarea_getdata (d, &drow, h);
                    copy_row (&srow, &drow);
                  }
                pixelarea_unref (d);
              }
            pixelarea_unref (s);
          }
      }
  }
  
  return paint_core->orig_buf;
}

Canvas * 
paint_core_16_area_original2  (
                              PaintCore16 * paint_core,
                              CanvasDrawable *drawable,
                              int x1,
                              int y1,
                              int x2,
                              int y2
                              )
{
  PixelArea srcPR, destPR, undoPR;
  Tag tag;

  /*tag = tag_set_alpha (drawable_tag (drawable), ALPHA_YES);*/
  tag = drawable_tag (drawable);
  if (paint_core->orig_buf)
    canvas_delete (paint_core->orig_buf);
  paint_core->orig_buf = canvas_new (tag, (x2 - x1), (y2 - y1), 
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

  
  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  
  pixelarea_init (&srcPR, drawable_data (drawable),
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&undoPR, paint_core->undo_tiles,
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&destPR, paint_core->orig_buf,
                  0, 0, (x2 - x1), (y2 - y1), TRUE);


  {
    void * pag;
    int h;
    
    for (pag = pixelarea_register_noref (3, &srcPR, &undoPR, &destPR);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        PixelRow srow, drow;
        PixelArea *s, *d;

        d = &destPR;
        {
          int x = pixelarea_x (&undoPR);
          int y = pixelarea_y (&undoPR);
          /*if (canvas_portion_alloced (paint_core->undo_tiles, x, y) == TRUE)
            s = &undoPR;
          else
            s = &srcPR;*/
          if (canvas_portion_alloced (drawable_data (drawable), x, y) == TRUE)
            s = &srcPR;
          else
            s = &undoPR;
        }
        
        if (pixelarea_ref (s) == TRUE)
          {
            if (pixelarea_ref (d) == TRUE)
              {
                h = pixelarea_height (s);
                while (h--)
                  {
                    pixelarea_getdata (s, &srow, h);
                    pixelarea_getdata (d, &drow, h);
                    copy_row (&srow, &drow);
                  }
                pixelarea_unref (d);
              }
            pixelarea_unref (s);
          }
      }
  }
  
  return paint_core->orig_buf;
}


void 
paint_core_16_area_paste  (
                           PaintCore16 * paint_core,
                           CanvasDrawable * drawable,
                           gfloat brush_opacity,
                           gfloat image_opacity,
                           BrushHardness brush_hardness,
			   gdouble brush_scale,
                           ApplyMode apply_mode,
                           int paint_mode
                           )
{
  Canvas *brush_mask;
  Canvas *brush_mask2;
  Canvas *undo_canvas;
  Canvas *undo_linked_canvas;

  if (! drawable_gimage (drawable))
    {
    return;
    }
  painthit_init (paint_core, drawable, paint_core->undo_tiles);

  if (paint_core->linked_drawable)
    painthit_init (paint_core, paint_core->linked_drawable, paint_core->linked_undo_tiles);

  if (gimp_brush_get_noise_mode()) 
    brush_mask = brush_mask_noise (paint_core, paint_core->brush_mask);
  else 
    brush_mask = paint_core->brush_mask;

  //Added by IMAGEWORKS aland to be able to paint on a 8 bit image
  paint_core->spacing_scale = brush_scale; 
  brush_mask = brush_mask_get (paint_core, brush_mask, brush_hardness, brush_scale);
  if(tag_precision(drawable_tag(drawable)) == PRECISION_U8){
      brush_mask2 = canvas_16_to_8 (brush_mask);
  }else if(tag_precision(drawable_tag(drawable)) == PRECISION_U16){
      brush_mask2 = canvas_16_to_16 (brush_mask);
  }else if(tag_precision(drawable_tag(drawable)) == PRECISION_FLOAT){
      brush_mask2 = canvas_16_to_float (brush_mask);
  }else if(tag_precision(drawable_tag(drawable)) == PRECISION_BFP){
      brush_mask2 = canvas_16_to_bfp (brush_mask);
  }else{
      brush_mask2 = brush_mask;
  }

  switch (apply_mode)
    {
    case CONSTANT:
      brush_to_canvas_tiles(paint_core,brush_mask2,brush_opacity);
      canvas_tiles_to_canvas_buf(paint_core); 
      undo_canvas = paint_core->undo_tiles; 
      undo_linked_canvas = paint_core->linked_undo_tiles; 
      break;
    case INCREMENTAL:
      brush_to_canvas_buf(paint_core,brush_mask2,brush_opacity);
      undo_canvas = NULL; 
      undo_linked_canvas = NULL; 
      break;
    }
  
  if (brush_mask2 != brush_mask)
    canvas_delete (brush_mask2);

  gimage_apply_painthit (drawable_gimage (drawable), drawable,
			 undo_canvas, paint_core->canvas_buf,
			 0, 0,
			 0, 0,
			 FALSE, image_opacity, paint_mode,
			 paint_core->x, paint_core->y);

  if (paint_core->linked_drawable)
    gimage_apply_painthit (drawable_gimage (drawable), paint_core->linked_drawable,
			   undo_linked_canvas, paint_core->linked_canvas_buf,
			   0, 0,
			   0, 0,
			   FALSE, image_opacity, paint_mode,
			   paint_core->x, paint_core->y);

  painthit_finish (drawable, paint_core, paint_core->canvas_buf);
}

void
paint_core_16_area_replace  (
                             PaintCore16 * paint_core,
                             CanvasDrawable *drawable,
                             gfloat brush_opacity,
                             gfloat image_opacity,
                             BrushHardness brush_hardness,
                             ApplyMode apply_mode,
                             int paint_mode  /*this is not used here*/
                             )
{
  Canvas *brush_mask;
  Canvas *brush_mask2;

  if (! drawable_gimage (drawable))
      	  return;
  
#ifdef RHCODE  
  //I really don;t know why R&H commented out any code to deal with Alpha Channels
  // IMAGEWORKS aland
  if (1 || ! drawable_has_alpha (drawable))
    {
      paint_core_16_area_paste (paint_core, drawable,
                                brush_opacity, image_opacity,
                                brush_hardness, apply_mode, NORMAL_MODE);
  /*d_printf ("1\n");
      if (paint_core->linked_drawable)
	paint_core_16_area_paste (paint_core, paint_core->linked_drawable,
				  brush_opacity, image_opacity,
				  brush_hardness, apply_mode, NORMAL_MODE);
   */   return;
    }
#endif  

  if (!drawable_has_alpha (drawable))
    {
      paint_core_16_area_paste (paint_core, drawable,
                                brush_opacity, image_opacity,
                                brush_hardness, 1.0, apply_mode, NORMAL_MODE);
   /*d_printf ("1\n");
      if (paint_core->linked_drawable)
        paint_core_16_area_paste (paint_core, paint_core->linked_drawable,
                      brush_opacity, image_opacity,
                      brush_hardness, 1.0,apply_mode, NORMAL_MODE);
   */   return;
    }else{
        paint_core_16_area_paste (paint_core, drawable,
                      brush_opacity, image_opacity,
                      brush_hardness, 1.0, apply_mode, NORMAL_MODE);
			    	  return;
    }
  


  
  d_printf("Whoa %d\n", drawable_has_alpha (drawable));
  
  painthit_init (paint_core, drawable, paint_core->undo_tiles);

  if (paint_core->linked_drawable)
    painthit_init (paint_core, paint_core->linked_drawable, paint_core->linked_undo_tiles);

  brush_mask = brush_mask_get (paint_core, paint_core->brush_mask, brush_hardness, 1.0);

//Added by IMAGEWORKS aland
  if(tag_precision(drawable_tag(drawable)) == PRECISION_U8)
      brush_mask2 = canvas_16_to_8 (brush_mask);
  else if(tag_precision(drawable_tag(drawable)) == PRECISION_U16)
      brush_mask2 = canvas_16_to_16 (brush_mask);
  else if(tag_precision(drawable_tag(drawable)) == PRECISION_FLOAT)
      brush_mask2 = canvas_16_to_float (brush_mask);
  else if(tag_precision(drawable_tag(drawable)) == PRECISION_BFP)
      brush_mask2 = canvas_16_to_bfp (brush_mask);
  else
      brush_mask2 = brush_mask;

/*  brush_to_canvas_buf(paint_core,brush_mask,brush_opacity);
*/
 
#if 1 
  gimage_replace_painthit (drawable_gimage (drawable), drawable,
			 paint_core->canvas_buf,
			 NULL, 
			 FALSE, image_opacity, brush_mask2,
			 paint_core->x, paint_core->y);

  if (paint_core->linked_drawable)
    gimage_replace_painthit (drawable_gimage (drawable), paint_core->linked_drawable,
			   paint_core->linked_canvas_buf,
			   NULL, 
			   FALSE, image_opacity, brush_mask2,
			   paint_core->x, paint_core->y);
#else
  gimage_apply_painthit (drawable_gimage (drawable), drawable,
		  NULL, paint_core->canvas_buf,
			  0, 0, 0, 0, 
		  FALSE, image_opacity, NORMAL_MODE,
		  paint_core->x, paint_core->y);

  if (paint_core->linked_drawable)
	  gimage_apply_painthit (drawable_gimage (drawable), paint_core->linked_drawable,
			  NULL, paint_core->linked_canvas_buf,
			  0, 0, 0, 0, 
			  FALSE, image_opacity, NORMAL_MODE,
			  paint_core->x, paint_core->y);
#endif
  if (brush_mask2 != brush_mask)
    canvas_delete (brush_mask2);

  painthit_finish (drawable, paint_core, paint_core->canvas_buf);
}


static void 
painthit_init  (
                PaintCore16 * paint_core,
                CanvasDrawable *drawable,
		Canvas * undo_tiles
                )
{

  PixelArea undo;
  PixelArea canvas;
  PixelArea src;
  PixelArea dst;
  void * pag;

  gint x = paint_core->x;  
  gint y = paint_core->y; 
  gint w = paint_core->canvas_buf_width; 
  gint h = paint_core->canvas_buf_height; 

  pixelarea_init (&undo, undo_tiles,
                  x, y, w, h, TRUE);
  pixelarea_init (&canvas, drawable_data (drawable),
                  x, y, w, h, FALSE);

  for (pag = pixelarea_register_noref (2, &undo, &canvas);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      int xx = pixelarea_x (&undo);
      int yy = pixelarea_y (&undo);

      if (canvas_portion_alloced (undo_tiles, xx, yy) == FALSE)
        {
          int ll = canvas_portion_x (undo_tiles, xx, yy);
          int tt = canvas_portion_y (undo_tiles, xx, yy);
          int ww = canvas_portion_width (undo_tiles, ll, tt);
          int hh = canvas_portion_height (undo_tiles, ll, tt);

          /* alloc the portion of the undo tiles */
          canvas_portion_alloc (undo_tiles, xx, yy);

          /* init the undo section from the original image */
          pixelarea_init (&src, drawable_data (drawable),
                          ll, tt, ww, hh, FALSE);
          pixelarea_init (&dst, undo_tiles,
                          ll, tt, ww, hh, TRUE);
         copy_area (&src, &dst);
        }
    }
}

static void
canvas_tiles_to_brush(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;
    int xoff, yoff;
    int x, y;
      
    x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
    y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
    xoff = (x < 0) ? -x : 0;
    yoff = (y < 0) ? -y : 0;
    
    pixelarea_init (&srcPR, paint_core->canvas_tiles,
                    paint_core->x, paint_core->y,
                    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                    FALSE);      
    pixelarea_init (&maskPR, brush_mask,
                    xoff, yoff,
                    canvas_width (brush_mask), canvas_height (brush_mask),
                    TRUE);

    copy_area (&srcPR, &maskPR);
}

static void
brush_to_canvas_tiles(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;
    int xoff, yoff;
    int x, y;
    int w = canvas_width (brush_mask),
        h = canvas_height (brush_mask);
      
    x = (int) paint_core->curx - (w >> 1);
    y = (int) paint_core->cury - (h >> 1);
    xoff = (x < 0) ? -x : 0;
    yoff = (y < 0) ? -y : 0;
    
    pixelarea_init (&srcPR, paint_core->canvas_tiles,
                    paint_core->x, paint_core->y,
                    w,h,
                    TRUE);      

    pixelarea_init (&maskPR, brush_mask,
                    xoff, yoff,
                    w,h,
                    TRUE);

    combine_mask_and_area (&srcPR, &maskPR, brush_opacity);
}

static void
canvas_tiles_to_canvas_buf(
                          PaintCore16 * paint_core
                          )
{
  PixelArea srcPR, maskPR;

  /*  apply the canvas tiles to the canvas buf  */
  pixelarea_init (&srcPR, paint_core->canvas_buf,
                  0, 0,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  TRUE);
  pixelarea_init (&maskPR, paint_core->canvas_tiles,
                  paint_core->x, paint_core->y,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  FALSE);      
  apply_mask_to_area (&srcPR, &maskPR, 1.0);

  if (paint_core->linked_drawable)
  { 
    /*  apply the canvas tiles to the linked canvas buf  */
    pixelarea_init (&srcPR, paint_core->linked_canvas_buf,
		    0, 0,
		    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
		    TRUE);
    pixelarea_init (&maskPR, paint_core->canvas_tiles,
		    paint_core->x, paint_core->y,
		    paint_core->canvas_buf_width, paint_core->canvas_buf_height,
		    FALSE);      
    apply_mask_to_area (&srcPR, &maskPR, 1.0);
  }
}


static void
brush_to_canvas_buf(
                             PaintCore16 * paint_core,
                             Canvas * brush_mask,
                             gfloat brush_opacity 
                             )
{
  PixelArea srcPR, maskPR;
#if 1
  int xoff, yoff;
      int x, y;
      
          x = (int) paint_core->curx - (canvas_width (brush_mask) >> 1);
	      y = (int) paint_core->cury - (canvas_height (brush_mask) >> 1);
	          xoff = (x < 0) ? -x : 0;
		      yoff = (y < 0) ? -y : 0;

  /*  combine the canvas buf and the brush mask to the canvas buf  */
  pixelarea_init (&srcPR, paint_core->canvas_buf,
                  0, 0,  
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  TRUE);
  pixelarea_init (&maskPR, brush_mask,
                  xoff, yoff, 
                  canvas_width (brush_mask), canvas_height (brush_mask),
		  TRUE);
  apply_mask_to_area (&srcPR, &maskPR, brush_opacity);

  if (paint_core->linked_drawable)
    {
	/*  combine the linked canvas buf and the brush mask to the canvas buf  */
	pixelarea_init (&srcPR, paint_core->linked_canvas_buf,
			0, 0,
			paint_core->canvas_buf_width, paint_core->canvas_buf_height,
			TRUE);
	pixelarea_init (&maskPR, brush_mask,
			xoff, yoff,
			canvas_width (brush_mask), canvas_height (brush_mask),
			TRUE);
	apply_mask_to_area (&srcPR, &maskPR, brush_opacity);
    }
#else
  /*  combine the canvas buf and the brush mask to the canvas buf  */
  pixelarea_init (&srcPR, paint_core->canvas_buf,
                  0, 0,
                  paint_core->canvas_buf_width, paint_core->canvas_buf_height,
                  TRUE);
  pixelarea_init (&maskPR, brush_mask,
                  0, 0,
                  canvas_width (brush_mask), canvas_height (brush_mask),
                  TRUE);
  apply_mask_to_area (&srcPR, &maskPR, brush_opacity);

  if (paint_core->linked_drawable)
    {
	/*  combine the linked canvas buf and the brush mask to the canvas buf  */
	pixelarea_init (&srcPR, paint_core->linked_canvas_buf,
			0, 0,
			paint_core->canvas_buf_width, paint_core->canvas_buf_height,
			TRUE);
	pixelarea_init (&maskPR, brush_mask,
			0, 0,
			canvas_width (brush_mask), canvas_height (brush_mask),
			TRUE);
	apply_mask_to_area (&srcPR, &maskPR, brush_opacity);
    }
#endif
}
  


static void
painthit_finish (
                 CanvasDrawable * drawable,
                 PaintCore16 * paint_core,
                 Canvas * painthit
                 )
{
  GImage *gimage;
  int offx, offy;

  /*  Update the undo extents  */
  paint_core->x1 = MIN (paint_core->x1, paint_core->x);
  paint_core->y1 = MIN (paint_core->y1, paint_core->y);
  paint_core->x2 = MAX (paint_core->x2, (paint_core->x + canvas_width (painthit)));
  paint_core->y2 = MAX (paint_core->y2, (paint_core->y + canvas_height (painthit)));
  
  /*  Update the gimage--it is important to call gdisplays_update_area
   *  instead of drawable_update because we don't want the drawable
   *  preview to be constantly invalidated
   */
  gimage = drawable_gimage (drawable);
  drawable_offsets (drawable, &offx, &offy);
  gdisplays_update_area (gimage->ID,
                         paint_core->x + offx, paint_core->y + offy,
                         canvas_width (painthit), canvas_height (painthit));

  if (paint_core->noise_mask)
   {
     canvas_delete (paint_core->noise_mask);
     paint_core->noise_mask = NULL;
   }

  if (paint_core->solid_mask)
   {
     canvas_delete (paint_core->solid_mask);
     paint_core->solid_mask = NULL;
   }

}


static Canvas * 
brush_mask_get  (
		 PaintCore16 *paint_core,
                 Canvas* brush_mask,
                 int brush_hardness,
		 gdouble scale
                 )
{
  Canvas * bm;

#ifndef GDK_CORE_POINTER
#define GDK_CORE_POINTER 0xfedc
#endif

  if (current_device == GDK_CORE_POINTER || fabs(1.0 - scale)  < 0.001)
    bm = brush_mask;
  else
    bm = paint_core_scale_mask (brush_mask, 
                                paint_core->curx, paint_core->cury,
                                scale);

  switch (brush_hardness)
    {
    case SOFT:
      if (current_device == GDK_CORE_POINTER || fabs(1.0 - scale)  < 0.001)
        bm = brush_mask_subsample (bm,
                                   paint_core->curx, paint_core->cury, scale);
      break;
      
    case HARD:
      bm = brush_mask_solidify (paint_core, bm);
      break;

    case EXACT:
      break;
      
    default:
      bm = NULL;
      break;
    }


  return bm;
}


static Canvas * 
brush_mask_subsample  (
                       Canvas * mask,
                       double orig_x,
                       double orig_y,
                       double scale
                       )
{
  Canvas * dest;
  guint16 * m, * d;
  float new_val;
  ShortsFloat u;
  int x, y;
  int orig_w = canvas_width (mask);
  int orig_h = canvas_height (mask);
  int startx = 0,
      starty = 0,
      scal_width = orig_w,
      scal_height = orig_h,
      endx = orig_w-1,
      endy = orig_h-1;
  double ip;
  double jump_x = modf( orig_x, &ip ),
         jump_y = modf( orig_y, &ip );


  if (current_device != GDK_CORE_POINTER || fabs( scale) > 0.001)
  {
    gdouble ratio = 0;
    paint_core_calculate_brush_size (mask, scale, 
                                     &scal_width, &scal_height,
                                     &ratio);
    startx = (orig_w-scal_width)/2;
    starty = (orig_h-scal_height)/2;
    endx = startx + scal_width - 1;
    endy = starty + scal_height - 1;
  }

  dest = canvas_new (canvas_tag (mask), orig_w + 1, orig_h + 1,
                     canvas_storage (mask));

  /* temp hack */
  canvas_portion_refro (mask, 0, 0);
  canvas_portion_refrw (dest, 0, 0);

  for (y = starty; y <= endy; y++)
    {
      for (x = startx; x <= endx; x++)
	{
          float old_val;
          m = (guint16*) canvas_portion_data (mask, x, y);
	  new_val = FLT( *m, u);

          // work on a square of 2x2; upper left to lower down
          // gamma?
          d = (guint16*) canvas_portion_data (dest, x, y);
	  old_val = FLT( *d, u);
	  *d = FLT16( old_val + new_val * (1-jump_x) * (1-jump_y), u );

          d = (guint16*) canvas_portion_data (dest, x + 1, y);
	  old_val = FLT( *d, u);
	  *d = FLT16( old_val + new_val * (jump_x) * (1-jump_y), u );

	  d = (guint16*) canvas_portion_data (dest, x, y + 1);
       	  old_val = FLT( *d, u);
	  *d = FLT16( old_val + new_val * (1-jump_x) * (jump_y), u );

          d = (guint16*) canvas_portion_data (dest, x + 1, y + 1);
	  old_val = FLT( *d, u);
	  *d = FLT16( old_val + new_val * jump_x * jump_y, u );
	}
    }

  /* temp hack */
  canvas_portion_unref (mask, 0, 0);
  canvas_portion_unref (dest, 0, 0);

  return dest;
}

typedef void  (*BrushMaskNoiseFunc) (PaintCore16 *,Canvas*,Canvas*);
static BrushMaskNoiseFunc brush_mask_noise_funcs[] =
{
  brush_mask_noise_u8,
  brush_mask_noise_u16,
  brush_mask_noise_float,
  brush_mask_noise_float16
};

static Canvas * 
brush_mask_noise  (
                      PaintCore16 * paint_core,
                      Canvas * brush_mask
                      )
{
  Canvas * noise_brush = NULL;
  Precision prec = tag_precision (canvas_tag (brush_mask));  

  noise_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask),
                            canvas_height (brush_mask),
                            canvas_storage (brush_mask));
  canvas_portion_refro (brush_mask, 0, 0);
  canvas_portion_refrw (noise_brush, 0, 0);
  
  (*brush_mask_noise_funcs [prec-1]) (paint_core, brush_mask, noise_brush); 

  canvas_portion_unref (noise_brush, 0, 0);
  canvas_portion_unref (brush_mask, 0, 0);

  paint_core->noise_mask = noise_brush;
  
  return noise_brush;
}

static void brush_mask_noise_u8 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
  guint8* data = (guint8*)canvas_portion_data (noise_brush, 0, 0);
  guint8* src = (guint8*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 255 : 0;
	}
    }
}


static void brush_mask_noise_u16 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
  guint16* data = (guint16*)canvas_portion_data (noise_brush, 0, 0);
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 65535 : 0;
	}
    }
}

static void brush_mask_noise_float (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  /* get the data and advance one line into it  */
  gfloat* data =(gfloat*)canvas_portion_data (noise_brush, 0, 0);
  gfloat* src = (gfloat*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 1.0 : 0.0;
	}
    }
}

static void brush_mask_noise_float16 (
                        PaintCore16 * paint_core,
			Canvas *brush_mask,
			Canvas *noise_brush
		      )
{  
  ShortsFloat u;
  float x,y;
  gint i, j;
  gint draw_w = drawable_width (paint_core->drawable);
  gint draw_h = drawable_height (paint_core->drawable);
  BrushNoiseInfo  info;
  float f;
  float start;
  float width;
  float end;
  gint w = canvas_width (brush_mask);
  gint h = canvas_height (brush_mask);
  gfloat val;
  gfloat src_val;
  gfloat xoff; 
  gfloat yoff; 
  gfloat out;
  gfloat c,s,theta,xnew,ynew;
 
  guint16* data =(guint16*)canvas_portion_data (noise_brush, 0, 0);
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);

  gimp_brush_get_noise_info (&info);
 
  f = info.freq * 256.0;

  if (f) 
  {
    xoff = (drand48()*255.0); 
    yoff = (drand48()*255.0);
    d_printf ("%f %f\n", xoff, yoff);  
  }
  else 
  {
    xoff = 0;
    yoff = 0;
  }

  xoff = (int)(xoff + .5);
  yoff = (int)(yoff + .5);

  start = info.step_start * .7;
  width = info.step_width;

  theta = drand48();
  c = cos (2.0 * 3.14 * theta);
  s = sin (2.0 * 3.14 * theta);
  
  for (i = 0; i < h; i++)
    {
      y = (i/256.0)  * f + yoff;

      for (j = 0; j < w; j++)
	{
	  x = (j/256.0) * f + xoff;
		
#if 1 
  	  xnew = c*x + s*y;
	  ynew = -s*x + c*y;
#endif

	  val = noise (xnew, ynew);   
#if 1 
	  out =  (.5 * (val + 1.0)); 
	  end = start + width;
          if (end>1) end = 1.0;
	  out = noise_smoothstep (start, end, out);
#endif
	  
	  src_val = FLT( *src++, u);

	  *data++ = FLT16(out *src_val , u);
	}
    }
}


Canvas *
canvas_16_to_8 (
            Canvas *canvas
            )
{
  Canvas *c;
  ShortsFloat u;
  gfloat src_val;
  gint i, j;

  c = canvas_new (
              tag_new(PRECISION_U8, canvas_format(canvas), canvas_alpha(canvas)),
              canvas_width(canvas),
              canvas_height(canvas),
              canvas_storage (canvas)
              );
      // Convert 16 bit float to 8 bit


      canvas_portion_refro (canvas, 0, 0);
      canvas_portion_refrw (c, 0, 0);
 
    {  
      /* get the data and advance one line into it  */
      guint8* data = (guint8*)canvas_portion_data (c, 0, 0);
      guint16* src = (guint16*)canvas_portion_data (canvas, 0, 0);

      for (i = 0; i < canvas_height (c); i++)
        {
            for (j = 0; j < canvas_width (c); j++)
            {
	      src_val = FLT(*src, u);
	          *data = src_val*255;

              src++;
              data++;
            }
        }
    }

  canvas_portion_unref (canvas, 0, 0);
  canvas_portion_unref (c, 0, 0);

  return c;
}

Canvas *
canvas_16_to_16 (
            Canvas *canvas
            )
{
  Canvas *c;
  ShortsFloat u;
  gfloat src_val;
  gint i, j;

  c = canvas_new (
              tag_new(PRECISION_U16, canvas_format(canvas), canvas_alpha(canvas)),
              canvas_width(canvas),
              canvas_height(canvas),
              canvas_storage (canvas)
              );
      // Convert 16 bit float to 16 bit


      canvas_portion_refro (canvas, 0, 0);
      canvas_portion_refrw (c, 0, 0);
 
    {  
      /* get the data and advance one line into it  */
      guint16* data = (guint16*)canvas_portion_data (c, 0, 0);
      guint16* src = (guint16*)canvas_portion_data (canvas, 0, 0);

      for (i = 0; i < canvas_height (c); i++)
        {
            for (j = 0; j < canvas_width (c); j++)
            {
	      src_val = FLT(*src, u);
	          *data = src_val*65535;

              src++;
              data++;
            }
        }
    }

  canvas_portion_unref (canvas, 0, 0);
  canvas_portion_unref (c, 0, 0);

  return c;
}

Canvas *
canvas_16_to_float (
            Canvas *canvas
            )
{
  Canvas *c;
  ShortsFloat u;
  gfloat src_val;
  gint i, j;

  c = canvas_new (
              tag_new(PRECISION_FLOAT, canvas_format(canvas), canvas_alpha(canvas)),
              canvas_width(canvas),
              canvas_height(canvas),
              canvas_storage (canvas)
              );
      // Convert 16 bit float to float


      canvas_portion_refro (canvas, 0, 0);
      canvas_portion_refrw (c, 0, 0);
 
    {  
      /* get the data and advance one line into it  */
      gfloat* data = (gfloat*)canvas_portion_data (c, 0, 0);
      guint16* src = (guint16*)canvas_portion_data (canvas, 0, 0);

      for (i = 0; i < canvas_height (c); i++)
        {
            for (j = 0; j < canvas_width (c); j++)
            {
	      src_val = FLT(*src, u);
	      *data = src_val;

              src++;
              data++;
            }
        }
    }

  canvas_portion_unref (canvas, 0, 0);
  canvas_portion_unref (c, 0, 0);

  return c;
}

typedef void  (*BrushSolidifyMaskFunc) (Canvas*,Canvas*);
static BrushSolidifyMaskFunc brush_solidify_mask_funcs[] =
{
  brush_solidify_mask_u8,
  brush_solidify_mask_u16,
  brush_solidify_mask_float,
  brush_solidify_mask_float16
};

Canvas *
canvas_16_to_bfp (
            Canvas *canvas
            )
{
  Canvas *c;
  ShortsFloat u;
  gfloat src_val;
  gint i, j;

  c = canvas_new (
              tag_new(PRECISION_BFP, canvas_format(canvas), canvas_alpha(canvas)),
              canvas_width(canvas),
              canvas_height(canvas),
              canvas_storage (canvas)
              );
      // Convert 16 bit float to 16 bit


      canvas_portion_refro (canvas, 0, 0);
      canvas_portion_refrw (c, 0, 0);
 
    {  
      /* get the data and advance one line into it  */
      guint16* data = (guint16*)canvas_portion_data (c, 0, 0);
      guint16* src = (guint16*)canvas_portion_data (canvas, 0, 0);

      for (i = 0; i < canvas_height (c); i++)
        {
            for (j = 0; j < canvas_width (c); j++)
            {
	      src_val = FLT(*src, u);
              *data = src_val*ONE_BFP;

              src++;
              data++;
            }
        }
    }

  canvas_portion_unref (canvas, 0, 0);
  canvas_portion_unref (c, 0, 0);

  return c;
}

static Canvas * 
brush_mask_solidify  (
		      PaintCore16 * paint_core,	
                      Canvas * brush_mask
                      )
{
  Canvas * solid_brush = NULL;
  Precision prec = tag_precision (canvas_tag (brush_mask));  

  solid_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width (brush_mask) ,
                            canvas_height (brush_mask) ,
                            canvas_storage (brush_mask));
  canvas_portion_refro (brush_mask, 0, 0);
  canvas_portion_refrw (solid_brush, 0, 0);
  
  (*brush_solidify_mask_funcs [prec-1]) (brush_mask, solid_brush); 

  canvas_portion_unref (solid_brush, 0, 0);
  canvas_portion_unref (brush_mask, 0, 0);
  
  paint_core->solid_mask = solid_brush;
  
  return solid_brush;
}


static void brush_solidify_mask_u8 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  guint8* data = (guint8*)canvas_portion_data (solid_brush, 0, 0);
  guint8* src = (guint8*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 255 : 0;
	}
    }
}


static void brush_solidify_mask_u16 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  guint16* data = (guint16*)canvas_portion_data (solid_brush, 0, 0);
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 65535 : 0;
	}
    }
}

static void brush_solidify_mask_float (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  gfloat* data =(gfloat*)canvas_portion_data (solid_brush, 0, 0);
  gfloat* src = (gfloat*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? 1.0 : 0.0;
	}
    }
}

static void brush_solidify_mask_float16 (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  ShortsFloat u;
  guint16* data =(guint16*)canvas_portion_data (solid_brush, 0, 0);
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? FLT16 (1.0, u) : FLT16 (0.0, u);
	}
    }
}

static void brush_solidify_mask_bfp (
			Canvas *brush_mask,
			Canvas *solid_brush
		      )
{  
  /* get the data and advance one line into it  */
  guint16* data = (guint16*)canvas_portion_data (solid_brush, 0, 0);
  guint16* src = (guint16*)canvas_portion_data (brush_mask, 0, 0);
  gint i, j;

  for (i = 0; i < canvas_height (brush_mask); i++)
    {
      for (j = 0; j < canvas_width (brush_mask); j++)
	{
	  *data++ = (*src++) ? ONE_BFP : 0;
	}
    }
}

static void
paint_core_calculate_brush_size (Canvas *mask,
				 gdouble  scale,
				 gint    *width,
				 gint    *height,
                                 gdouble *ratio)
{
  scale = CLAMP (scale, 0.0, 1.0);

  *ratio = 1.0;

  if (current_device == GDK_CORE_POINTER)
    {
      *width  = canvas_width(mask);
      *height = canvas_height(mask);
    }
  else
    {
      if (scale < 1 / 256)
	*ratio = 1 / 16;
      else
	*ratio = PRESSURE_MODIFY (scale);
      // TODO 1.0??
      *width  = MAX ((gint)(canvas_width(mask) * *ratio + .5), 1);
      *height = MAX ((gint)(canvas_height(mask) * *ratio + .5), 1);
    }
}  

static Canvas *
paint_core_scale_mask (Canvas  *brush_mask, 
                       gdouble  orig_x,
                       gdouble  orig_y,
		       gdouble  scale)
{
  static Canvas *last_brush  = NULL;
  static gdouble last_jump_x = 0.0,
                 last_jump_y = 0.0;
  static gdouble last_scale  = 0.0;
  gint dest_width;
  gint dest_height;
  PixelArea srcPR, destPR;
  PixelRow  srow, drow;
  gdouble   ratio;
  double    ip;
  // How much is our cursor position off to the drawing pixel?
  double    jump_x = modf( orig_x, &ip ),
            jump_y = modf( orig_y, &ip );
  guint16 * s = NULL,
          * d = NULL;
  float   * buf = NULL;

  scale = CLAMP (scale, 0.0, 1.0);

  if (scale == 0.0)
    return NULL;

  if (scale == 1.0)
    return brush_mask;

  if(jump_x < 0)
    jump_x = 1.0 + jump_x;
  if(jump_y < 0)
    jump_y = 1.0 + jump_y;

  paint_core_calculate_brush_size (brush_mask, scale, 
				   &dest_width, &dest_height,
                                   &ratio);

  // we need some place to scale and position
  dest_width  ++;
  dest_height ++;

  if (brush_mask == last_brush &&
      jump_x == last_jump_x &&
      jump_y == last_jump_y &&
      scale == last_scale)
    return scale_brush;

  if (scale_brush)
    canvas_delete (scale_brush);

  last_brush  = brush_mask;
  last_jump_x = jump_x;
  last_jump_y = jump_y;
  last_scale  = scale;

  pixelarea_init (&srcPR, brush_mask,
                  0, 0,
                  0, 0,
                  FALSE);

  /* For varirous strange reasons, the scaled brush must stay the same size */
  /* I believe it has to do with gimage_apply_painthit making assumptions on the size of the painthit */
  scale_brush = canvas_new (canvas_tag (brush_mask),
                            canvas_width(brush_mask) + 1,
                            canvas_height(brush_mask) + 1,
                            canvas_storage (brush_mask));
  pixelarea_init (&destPR, scale_brush,
                  (canvas_width(brush_mask)-dest_width/*-1*/)/2,
                  (canvas_height(brush_mask)-dest_height/*-1*/)/2,
                  dest_width, dest_height,
                  TRUE);

  canvas_portion_refro (brush_mask, 0, 0);
  canvas_portion_refrw (scale_brush, 0, 0);
#if 0
  scale_area_exact (&srcPR, &destPR, jump_x, jump_y, 1./ratio, 1./ratio);
#else

  // Get the data
  // a brush should allways be one channel
  buf = (float*) g_malloc( sizeof(float) * canvas_width(scale_brush) );
  memset ( buf, 0, sizeof(float) * canvas_width(scale_brush) );

  /* Die folgenden Berechungen versuchen sowohl Positionierung als auch 
   * Skalierung auf die Zielkoordinaten zu projizieren.
   * Damit sind alle Angaben, ausser sx und sy, auf die scale_brush Koordinaten
   * dx und dy bezogen. 
   */
  {
    // the source brush pixel
    int sx,sy,
    // the scaled brush pixel in reference coordinates
        dx = 0,
        dy = 0;
    int swidth   = canvas_width( brush_mask),
        sheight  = canvas_height(brush_mask);
    int r, n_x;
    // position the old brush_mask in the middle of our scale_brush target
    float edge_x = (swidth - ratio * swidth) / 2.,
          edge_y = (sheight - ratio * sheight) / 2.;
    double sx_start, sy_start;
    double sx_ende,  sy_ende;
    double x_anteil, y_anteil;
    int rounds = 0;

    // take the cursor position into account
    jump_x += edge_x;
    jump_y += edge_y;

    if( canvas_storage(brush_mask) != STORAGE_FLAT)
      g_warning("%s:%d %s() Can not handle non flat brush_masks.",__FILE__,__LINE__,__func__);
    for( sy = 0; sy < sheight; ++sy)
    {
      // Wieviel Runden muss sy abgelesen werden?
      rounds = 1;

      // Der Quellpunkt startet hier ...
      sy_start    =   jump_y +  sy      * ratio;
      // ... Ende
      sy_ende     =   sy_start + ratio;
      y_anteil   =   ratio;

      dy = (int)sy_start;

      // Reicht der Quellpunkt ueber das Zielquadrat?
      if(sy_ende > dy + 1)
        rounds = 2;

      if(sy_start < 0.0 || sy_ende > canvas_height(scale_brush))
        printf("i%s:%d %s() %fx%f == %d?\n",__FILE__,__LINE__,__func__, sy_start, sy_ende, canvas_height(scale_brush));

      for( r = 0; r < rounds; ++r)
      {
        if(r == 0)
        {
          // the upper part

          // wohin
          dy = (int)sy_start;

          y_anteil = ratio;
          // take the source pixel part up to the projection pixel border
          if(sy_ende > dy + 1)
            y_anteil = (dy + 1) - sy_start;

        } else {
          // the lower part

          // wohin
          dy = (int)sy_ende;

          // the remaining part of the splittet source pixel
          y_anteil = sy_ende - dy;
        }

        for (sx = 0; sx < swidth; ++sx)
        {
          ShortsFloat u;
          guint16 * s = NULL;

          // Der Quellpunkt startet hier ...
          sx_start   =   jump_x +  sx * ratio;
          // ... Ende
          sx_ende    =   sx_start + ratio;
          x_anteil   =   ratio;

          // wohin
          dx = (int)sx_start;

          if(sx_start < 0.0 || sx_ende > canvas_width(scale_brush) || dx < 0)
            printf("i%s:%d %s() %fx%f == %d?\n",__FILE__,__LINE__,__func__, sx_start, sx_ende, canvas_width(scale_brush));

          // unser Quellpunkt
          s = (guint16*) canvas_portion_data (brush_mask, sx, sy);

          // Reicht der Quellpunkt ueber das Zielquadrat?
          if(sx_ende > dx + 1)
          {
            // 2. Anteil ..
            x_anteil = sx_ende - (dx + 1);
            //    .. auf Ziel aufschlagen
            buf[ dx + 1 ] += FLT(*s,u) * x_anteil * y_anteil;

            // 1. Anteil berechnen
            x_anteil = (dx + 1) - sx_start;
          }

          // 1. Anteil auf Ziel aufschlagen
          buf[ dx ] += FLT(*s,u) * x_anteil * y_anteil;
        }

        // naechste Zeile? dann schon eintragen
        if ( sy_ende >= dy + 1 )
        {
          n_x = dx;
          if ( dx < (int)sx_ende )
            ++n_x;
          for (dx = 0; dx <= n_x; ++dx)
          {
            guint16 *d = (guint16*) canvas_portion_data (scale_brush, dx, dy);
#ifdef DEBUG
            if(dx >= canvas_width(scale_brush) || dy >= canvas_height(scale_brush)) {
            printf("i%s:%d %s() %dx%d ? %dx%d\n",__FILE__,__LINE__,__func__, canvas_width(scale_brush), canvas_height(scale_brush), dx,dy);
            printf("%fx%f %fx%f %fx%f\n",sx_start, sy_start,
            sx_ende,  sy_ende,
            x_anteil, y_anteil);
            }
#endif
            *d = FLT16( buf[dx],u);
            buf[dx] = 0.0;
          }
        }
      }
    }

    // .. und den Rest einkehren
    n_x = dx;
    if ( dx < (int)sx_ende )
      ++n_x;
    for (dx = 0; dx <= n_x; ++dx)
    {
          guint16 *d = (guint16*) canvas_portion_data (scale_brush, dx, dy);
#ifdef DEBUG
          if(dx >= canvas_width(scale_brush) || dy >= canvas_height(scale_brush)) {
            printf("i%s:%d %s() %dx%d ? %dx%d\n",__FILE__,__LINE__,__func__, canvas_width(scale_brush), canvas_height(scale_brush), dx,dy);
            printf("%fx%f %fx%f %fx%f\n",sx_start, sy_start,
            sx_ende,  sy_ende,
            x_anteil, y_anteil);
          }
#endif
          *d = FLT16( buf[dx],u);
          buf[dx] = 0.0;
    }
  }

  canvas_portion_unref (scale_brush, 0, 0);
  canvas_portion_unref (brush_mask, 0, 0);
  g_free( buf );
  buf = NULL;
#endif

  return scale_brush;
}

