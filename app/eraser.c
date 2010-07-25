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
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "eraser.h"
#include "errors.h"
#include "gimage.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "devices.h"

/*  forward function declarations  */
static void        eraser_motion   (PaintCore *, CanvasDrawable *, gboolean, gboolean);
static Argument *  eraser_invoker  (Argument *);
static Argument *  eraser_extended_invoker  (Argument *);
static void        eraser_painthit_setup (PaintCore *, Canvas *);

static gboolean non_gui_hard, non_gui_incremental;


typedef struct EraserOptions EraserOptions;
struct EraserOptions
{
  gboolean hard;
  gboolean incremental;
};

static EraserOptions *eraser_options = NULL;

static void
eraser_toggle_update (GtkWidget *w,
		      gpointer   data)
{
  gboolean *toggle_val;

  toggle_val = (gboolean *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static EraserOptions *
create_eraser_options (void)
{
  EraserOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *hard_toggle;
  GtkWidget *incremental_toggle;

  options = (EraserOptions *) g_malloc_zero (sizeof (EraserOptions));
  options->hard = FALSE;
  options->incremental = FALSE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Eraser Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* the hard toggle */
  hard_toggle = gtk_check_button_new_with_label (_("Hard edge"));
  gtk_box_pack_start (GTK_BOX (vbox), hard_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (hard_toggle), "toggled",
		      (GtkSignalFunc) eraser_toggle_update,
		      &options->hard);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (hard_toggle), options->hard);
  gtk_widget_show (hard_toggle);
  
  /* the incremental toggle */
  incremental_toggle = gtk_check_button_new_with_label (_("Incremental"));
  gtk_box_pack_start (GTK_BOX (vbox), incremental_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (incremental_toggle), "toggled",
		      (GtkSignalFunc) eraser_toggle_update,
		      &options->incremental);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (incremental_toggle), options->incremental);
  gtk_widget_show (incremental_toggle);
  
  /*  Register this eraser options widget with the main tools options dialog  */
  tools_register_options (ERASER, vbox);

  return options;
}
  
static void *
eraser_paint_func (paint_core, drawable, state)
     PaintCore *paint_core;
     CanvasDrawable *drawable;
     int state;
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      eraser_motion (paint_core, drawable, eraser_options->hard, eraser_options->incremental);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_eraser ()
{
  Tool * tool;
  PaintCore * private;

  if (! eraser_options)
    eraser_options = create_eraser_options ();

  tool = paint_core_new (ERASER);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) eraser_paint_func;
  private->painthit_setup = eraser_painthit_setup;

  return tool;
}


void
tools_free_eraser (tool)
     Tool * tool;
{
  paint_core_free (tool);
}


void 
eraser_motion  (
                PaintCore * paint_core,
                CanvasDrawable * drawable,
                gboolean hard,
                gboolean incremental
                )
{
  /* Set up the working painthit */
  paint_core_16_area_setup (paint_core, drawable);
  
  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            current_device ? paint_core->curpressure : (gfloat) 1.0,
                            (gfloat) gimp_brush_get_opacity (),
                            hard ? HARD : SOFT,
			    1.0,
                            incremental ? INCREMENTAL : CONSTANT,
                            ERASE_MODE);
}


static void
eraser_painthit_setup (PaintCore * paint_core,Canvas * painthit)
{
    PixelArea a;
    pixelarea_init (&a, painthit, 0, 0, 0, 0, TRUE);

    /* Construct the paint hit */

    if (paint_core->setup_mode == NORMAL_SETUP)
    {
      COLOR16_NEW (paint, canvas_tag (painthit));
      COLOR16_INIT (paint);
      palette_get_background (&paint);
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
eraser_non_gui_paint_func (PaintCore *paint_core, CanvasDrawable *drawable, int state)
{
  eraser_motion (paint_core, drawable, non_gui_hard, non_gui_incremental);

  return NULL;
}


/*  The eraser procedure definition  */
ProcArg eraser_extended_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The Drawable"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  },
  { PDB_INT32,
    "hardness",
    "SOFT(0) or HARD(1)"
  },
  { PDB_INT32,
    "method",
    "CONTINUOUS(0) or INCREMENTAL(1)"
  }
};

ProcArg eraser_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The Drawable"
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


ProcRecord eraser_proc =
{
  "gimp_eraser",
  "Erase using the current brush",
  "This tool erases using the current brush mask.  If the specified drawable contains an alpha channel, then the erased pixels will become transparent.  Otherwise, the eraser tool replaces the contents of the drawable with the background color.  Like paintbrush, this tool linearly interpolates between the specified stroke coordinates.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  eraser_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { eraser_invoker } },
};

ProcRecord eraser_extended_proc =
{
  "gimp_eraser_extended",
  "Erase using the current brush",
  "This tool erases using the current brush mask.  If the specified drawable contains an alpha channel, then the erased pixels will become transparent.  Otherwise, the eraser tool replaces the contents of the drawable with the background color.  Like paintbrush, this tool linearly interpolates between the specified stroke coordinates.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  eraser_extended_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { eraser_extended_invoker } },
};

static Argument *
eraser_invoker (args)
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
      non_gui_hard=0; non_gui_incremental = 0;
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) eraser_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	eraser_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&eraser_proc, success);
}

static Argument *
eraser_extended_invoker (args)
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
  	non_gui_hard = args[4].value.pdb_int;
	non_gui_incremental = args[5].value.pdb_int;
  }

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) eraser_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	eraser_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&eraser_proc, success);
}
