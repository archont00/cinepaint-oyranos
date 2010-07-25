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
#include "appenv.h"
#include "airbrush.h"
#include "paint_options.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h" /* for airbrush_time_out() */
#include "gimage.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "devices.h"
#include "lib/intl.h"
#include "lib/widgets.h"


/*  The maximum amount of pressure that can be exerted  */
#define MAX_PRESSURE  0.075

/* Default pressure setting */
#define AIRBRUSH_PRESSURE_DEFAULT    10.0
#define AIRBRUSH_INCREMENTAL_DEFAULT FALSE

#define OFF           0
#define ON            1

/*  the airbrush structures  */

typedef struct AirbrushTimeout AirbrushTimeout;

struct AirbrushTimeout
{
  PaintCore    *paint_core;
  CanvasDrawable *drawable;
};

typedef struct AirbrushOptions AirbrushOptions;

struct AirbrushOptions
{
  PaintOptions paint_options;

  gdouble      rate;
  gdouble      rate_d;
  GtkObject   *rate_w;

  gdouble      pressure;
  gdouble      pressure_d;
  GtkObject   *pressure_w;
};


/*  the airbrush tool options  */
static AirbrushOptions *airbrush_options = NULL; 

/*  local variables  */
static gint             timer;  /*  timer for successive paint applications  */
static gint             timer_state = OFF;       /*  state of airbrush tool  */
static AirbrushTimeout  airbrush_timeout;

static gdouble          non_gui_pressure;
static gboolean         non_gui_incremental;

/*  forward function declarations  */
static void         airbrush_motion   (PaintCore16 *, CanvasDrawable *,
				       PaintPressureOptions *,
				       gdouble, GimpPaintApplicationMode);
static gint         airbrush_time_out (gpointer);
static Argument *   airbrush_invoker  (Argument *);
static void         airbrush_painthit_setup (PaintCore *,Canvas *);
static void *       airbrush_paint_func (PaintCore16 *, CanvasDrawable *, int);


/*  functions  */

static void
airbrush_options_reset (void)
{
  AirbrushOptions *options = airbrush_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w),
			    options->rate_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->pressure_w),
			    options->pressure_d);
}

static AirbrushOptions *
create_airbrush_options (void)
{
  AirbrushOptions *options;

  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *scale;

  /*  the new airbrush tool options structure  */
  options = g_new (AirbrushOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      AIRBRUSH,
		      airbrush_options_reset);
  options->rate     = options->rate_d     = 80.0;
  options->pressure = options->pressure_d = AIRBRUSH_PRESSURE_DEFAULT;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 150.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->rate_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->rate);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Rate:"), 1.0, 1.0,
			     scale, 1, FALSE);

  /*  the pressure scale  */
  options->pressure_w =
    gtk_adjustment_new (options->pressure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->pressure_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->pressure_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->pressure);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Pressure:"), 1.0, 1.0,
			     scale, 1, FALSE);

  gtk_widget_show (table);
  tools_register_options (AIRBRUSH, vbox);

  return options;
}

Tool *
tools_new_airbrush ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! airbrush_options)
    airbrush_options = create_airbrush_options ();

  tool = paint_core_new (AIRBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) airbrush_paint_func;
  private->painthit_setup = airbrush_painthit_setup;

  return tool;
}


static void *
airbrush_paint_func (PaintCore16 *paint_core,
		     CanvasDrawable *drawable,
		     int           state)
{
  GimpBrushP brush;

  if (!drawable) 
    return NULL;

  brush = get_active_brush ();
  switch (state)
    {
    case INIT_PAINT :
      /* timer_state = OFF; */
      if (timer_state == ON)
	{
	  g_warning ("killing stray timer, please report to lewing@gimp.org");
	  gtk_timeout_remove (timer);
	}
      timer_state = OFF;
      break;

    case MOTION_PAINT :
      if (timer_state == ON)
	gtk_timeout_remove (timer);
      timer_state = OFF;

      airbrush_motion (paint_core, drawable,
		       airbrush_options->paint_options.pressure_options,
		       airbrush_options->pressure,
		       airbrush_options->paint_options.incremental ?
		       INCREMENTAL : CONSTANT);

      if (airbrush_options->rate != 0.0)
	{
          gdouble rate;		
	  airbrush_timeout.paint_core = paint_core;
	  airbrush_timeout.drawable = drawable;
	  rate = airbrush_options->paint_options.pressure_options->rate ? 
	    (10000 / (airbrush_options->rate * 2.0 * paint_core->curpressure)) : 
	    (10000 / airbrush_options->rate);
	  timer = gtk_timeout_add (CAST(int) rate, airbrush_time_out, NULL);
	  timer_state = ON;
	}
      break;

    case FINISH_PAINT :
      if (timer_state == ON)
	gtk_timeout_remove (timer);
      timer_state = OFF;
      break;

    default :
      break;
    }

  return NULL;
}


void
tools_free_airbrush (Tool *tool)
{
  if (timer_state == ON)
    gtk_timeout_remove (timer);
  timer_state = OFF;

  paint_core_free (tool);
}


static gint
airbrush_time_out (gpointer client_data)
{
  /*  service the timer  */
  airbrush_motion (airbrush_timeout.paint_core,
		   airbrush_timeout.drawable,
		   airbrush_options->paint_options.pressure_options,
		   airbrush_options->pressure,
		   airbrush_options->paint_options.incremental ?
		   INCREMENTAL : CONSTANT);
  gdisplays_flush ();

  /*  restart the timer  */
  if (airbrush_options->rate != 0.0)
    {
      if (airbrush_options->paint_options.pressure_options->rate)
	{
	  /* set a new timer */
	  timer = gtk_timeout_add (CAST(int) (10000 / (airbrush_options->rate * 2.0 * airbrush_timeout.paint_core->curpressure)), 
				   airbrush_time_out, NULL);
	  return FALSE;
	}
      else 
	return TRUE;
    }
  else
    return FALSE;
}

static void 
airbrush_motion (PaintCore16          *paint_core,
		 CanvasDrawable         *drawable,
		 PaintPressureOptions *pressure_options,
		 double               pressure,
		 GimpPaintApplicationMode  mode)
{
  gdouble scale;
  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /* Get the working canvas */

  /* Set up the painthit canvas */
  paint_core_16_area_setup (paint_core, drawable);

  if (pressure_options->pressure)
    pressure = pressure * 2.0 * paint_core->curpressure;

  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            current_device ? (gfloat) pressure / 100.0 * paint_core->curpressure :
			    (gfloat) pressure / 100.0,
                            (gfloat) gimp_brush_get_opacity (), 
                            SOFT,
			    scale,
                            INCREMENTAL,
                            gimp_brush_get_paint_mode ());
}

static void
airbrush_painthit_setup (PaintCore * paint_core,Canvas * painthit)
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
airbrush_non_gui_paint_func (PaintCore    *paint_core,
			     CanvasDrawable *drawable,
			     int           state)
{
  airbrush_motion (paint_core, drawable, &non_gui_pressure_options, 
		   non_gui_pressure, non_gui_incremental);

  return NULL;
}


/*  The airbrush procedure definition  */
ProcArg airbrush_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The Drawable"
  },
  { PDB_FLOAT,
    "pressure",
    "The pressure of the airbrush strokes: 0 <= pressure <= 100"
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


ProcRecord airbrush_proc =
{
  "gimp_airbrush",
  "Paint in the current brush with varying pressure.  Paint application is time-dependent",
  "This tool simulates the use of an airbrush.  Paint pressure represents the relative intensity of the paint application.  High pressure results in a thicker layer of paint while low pressure results in a thinner layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  airbrush_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { airbrush_invoker } },
};

static Argument *
airbrush_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  CanvasDrawable *drawable;
  int num_strokes;
  double *stroke_array;
  int int_value;
  double fp_value;
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
  /*  pressure  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	non_gui_pressure = fp_value;
      else
	success = FALSE;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

      /*  point array  */
  if (success)
    stroke_array = (double *) args[4].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) airbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	airbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&airbrush_proc, success);
}

gboolean
airbrush_non_gui (CanvasDrawable *drawable,
    		  double        pressure,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = (PaintFunc16) airbrush_non_gui_paint_func;

      non_gui_pressure = pressure;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      airbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup (&non_gui_paint_core);
      return TRUE;
    }
  else
    return FALSE;
}
