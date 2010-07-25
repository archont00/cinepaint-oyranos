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
#include "config.h"
#include "../lib/version.h"
#include "../libgimp/gimpintl.h"
#include "appenv.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gimage.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pencil.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "devices.h"

#define    PRESSURE_MODIFY(x)  sqrt( x )

/*  forward function declarations  */
static void         pencil_motion       (PaintCore *, CanvasDrawable *);
static Argument *   pencil_invoker  (Argument *);
static void         pencil_painthit_setup (PaintCore *,Canvas *);

static void *  pencil_options = NULL;

static void *
pencil_paint_func (paint_core, drawable, state)
     PaintCore *paint_core;
     CanvasDrawable *drawable;
     int state;
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      pencil_motion (paint_core, drawable);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_pencil ()
{
  Tool * tool;
  PaintCore * private;

  if (!pencil_options)
    pencil_options = tools_register_no_options (PENCIL, _("Pencil Options"));

  tool = paint_core_new (PENCIL);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) pencil_paint_func;
  private->painthit_setup = pencil_painthit_setup;

  return tool;
}


void
tools_free_pencil (tool)
     Tool * tool;
{
  paint_core_free (tool);
}


static void
pencil_motion (paint_core, drawable)
     PaintCore *paint_core;
     CanvasDrawable *drawable;
{
  /* Get the working canvas */
  paint_core_16_area_setup (paint_core, drawable);

  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            current_device ? 
                              PRESSURE_MODIFY(PRESSURE_MODIFY(paint_core->curpressure)) : 1.0,
                            (gfloat) gimp_brush_get_opacity(),
                            SOFT,
                            current_device ? 
                              PRESSURE_MODIFY(PRESSURE_MODIFY(paint_core->curpressure)) : 1.0,
/*			    1.0, *//* constant size */
                            CONSTANT,
                            gimp_brush_get_paint_mode ());
}

static void
pencil_painthit_setup (PaintCore * paint_core,Canvas * painthit)
{
    PixelArea a;
    pixelarea_init (&a, painthit, 0, 0, 0, 0, TRUE);

    /* Construct the paint hit */

    if (paint_core->setup_mode == NORMAL_SETUP)
    {
      COLOR16_NEW (paint, canvas_tag (painthit));
      COLOR16_INIT (paint);
      palette_get_foreground (&paint);
      color_area (&a, &paint);
    }
    else if (paint_core->setup_mode == LINKED_SETUP)
    {
	Channel * channel = GIMP_CHANNEL(paint_core->linked_drawable);
        gfloat g = channel_get_link_paint_opacity (channel); 
	{
	  PixelRow paint;
	  Tag t = tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO);
	  gfloat data[3];
	  data[0] = g;
	  data[1] = g;
	  data[2] = g;
	  pixelrow_init (&paint, t, (guchar*)data, 1);
          color_area (&a, &paint);
	}
    }
}

static void *
pencil_non_gui_paint_func (PaintCore *paint_core, CanvasDrawable *drawable, int state)
{
  pencil_motion (paint_core, drawable);

  return NULL;
}


/*  The pencil procedure definition  */
ProcArg pencil_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord pencil_proc =
{
  "gimp_pencil",
  "Paint in the current brush without sub-pixel sampling",
  "This tool is the standard pencil.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The brush mask is treated as though it contains only black and white values.  Any value below half is treated as black; any above half, as white.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  pencil_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { pencil_invoker } },
};

static Argument *
pencil_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  CanvasDrawable *drawable;
  int num_strokes;
  double *stroke_array;
  int int_value;
  int i;

  drawable = NULL;
  num_strokes = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[3].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) pencil_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	pencil_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup (&non_gui_paint_core);
    }

  return procedural_db_return_args (&pencil_proc, success);
}
