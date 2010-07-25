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
#include <math.h>
#include "appenv.h"
#include "paint_options.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gimage.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "paintbrush.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "devices.h"
#include "../lib/unitmenu.h"
#include "../lib/intl.h"
#include "../lib/widgets.h"


/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

/* defaults for the tool options */
#define PAINTBRUSH_DEFAULT_INCREMENTAL       FALSE
#define PAINTBRUSH_DEFAULT_USE_FADE          FALSE
#define PAINTBRUSH_DEFAULT_FADE_OUT          100.0
#define PAINTBRUSH_DEFAULT_FADE_UNIT         GIMP_UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_USE_GRADIENT      FALSE
#define PAINTBRUSH_DEFAULT_GRADIENT_LENGTH   100.0
#define PAINTBRUSH_DEFAULT_GRADIENT_UNIT     GIMP_UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_GRADIENT_TYPE     LOOP_TRIANGLE

/*  the paintbrush structures  */

typedef struct PaintbrushOptions PaintbrushOptions;
struct PaintbrushOptions
{
  PaintOptions  paint_options;

  gboolean      use_fade;
  gboolean      use_fade_d;
  GtkWidget    *use_fade_w;

  gdouble       fade_out;
  gdouble       fade_out_d;
  GtkObject    *fade_out_w;

  GimpUnit      fade_unit;
  GimpUnit      fade_unit_d;
  GtkWidget    *fade_unit_w;

  gboolean      use_gradient;
  gboolean      use_gradient_d;
  GtkWidget    *use_gradient_w;

  gdouble       gradient_length;
  gdouble       gradient_length_d;
  GtkObject    *gradient_length_w;

  GimpUnit      gradient_unit;
  GimpUnit      gradient_unit_d;
  GtkWidget    *gradient_unit_w;

  gint          gradient_type;
  gint          gradient_type_d;
  GtkWidget    *gradient_type_w;
};

/*  the paint brush tool options  */
static PaintbrushOptions * paintbrush_options = NULL;

/*  forward function declarations  */
static Argument *   paintbrush_invoker     (Argument *);
static Argument *   paintbrush_extended_invoker     (Argument *);
static void paintbrush_motion (PaintCore *, CanvasDrawable *, 
			       PaintPressureOptions *,
			       double,
			       gboolean);
static void* paintbrush_paint_func(PaintCore *paint_core, CanvasDrawable *drawable, int state);

static double non_gui_fade_out, non_gui_incremental;


static void
paintbrush_options_reset (void)
{
  PaintbrushOptions *options = paintbrush_options;
  GtkWidget *spinbutton;
  gint       digits;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_fade_w),
				options->use_fade_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fade_out_w),
			    options->fade_out_d);
#if 0
  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->fade_unit_w),
			   options->fade_unit_d);
  digits = ((options->fade_unit_d == GIMP_UNIT_PIXEL) ? 0 :
	    ((options->fade_unit_d == GIMP_UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (options->fade_unit_d))))));
  spinbutton = gtk_object_get_data (GTK_OBJECT (options->fade_unit_w), "set_digits");
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
#endif
#if 0
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_gradient_w),
				options->use_gradient_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->gradient_length_w),
			    options->gradient_length_d);
  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->gradient_unit_w),
			   options->gradient_unit_d);
  digits = ((options->gradient_unit_d == GIMP_UNIT_PIXEL) ? 0 :
	    ((options->gradient_unit_d == GIMP_UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (options->gradient_unit_d))))));
  spinbutton = gtk_object_get_data (GTK_OBJECT (options->gradient_unit_w), "set_digits");
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);

  options->gradient_type = options->gradient_type_d;

  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w), 
			       options->gradient_type_d);
#endif
}

static PaintbrushOptions *
paintbrush_options_new (void)
{
  PaintbrushOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *type_label;
  GtkWidget *spinbutton;

  /*  the new paint tool options structure  */
  options = g_new (PaintbrushOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      PAINTBRUSH,
		      paintbrush_options_reset);

  options->use_fade        = 
                  options->use_fade_d        = PAINTBRUSH_DEFAULT_USE_FADE;
  options->fade_out        = 
                  options->fade_out_d        = PAINTBRUSH_DEFAULT_FADE_OUT;
  options->fade_unit        = 
                  options->fade_unit_d       = PAINTBRUSH_DEFAULT_FADE_UNIT;
#if 0
  options->use_gradient    = 
                  options->use_gradient_d    = PAINTBRUSH_DEFAULT_USE_GRADIENT;
  options->gradient_length = 
                  options->gradient_length_d = PAINTBRUSH_DEFAULT_GRADIENT_LENGTH;
  options->gradient_unit        = 
                  options->gradient_unit_d   = PAINTBRUSH_DEFAULT_GRADIENT_UNIT;
  options->gradient_type   = 
                  options->gradient_type_d   = PAINTBRUSH_DEFAULT_GRADIENT_TYPE;
#endif  
  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 3);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  the use fade toggle  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->use_fade_w =
    gtk_check_button_new_with_label (_("Fade Out"));
  gtk_container_add (GTK_CONTAINER (abox), options->use_fade_w);
  gtk_signal_connect (GTK_OBJECT (options->use_fade_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->use_fade);
  gtk_widget_show (options->use_fade_w);

  /*  the fade-out sizeentry  */
  options->fade_out_w =  
    gtk_adjustment_new (options->fade_out_d,  1e-5, 32767.0, 1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (options->fade_out_w), 1.0, 0.0);
#if GTK_MAJOR_VERSION < 2
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton), GTK_SHADOW_NONE);
#endif
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (options->fade_out_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->fade_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

#if 0
  /*  the fade-out unitmenu  */
  options->fade_unit_w = 
    gimp_unit_menu_new ("%a", options->fade_unit_d, TRUE, TRUE, TRUE);
  gtk_signal_connect (GTK_OBJECT (options->fade_unit_w), "unit_changed",
		      GTK_SIGNAL_FUNC (gimp_unit_menu_update),
		      &options->fade_unit);
  gtk_object_set_data (GTK_OBJECT (options->fade_unit_w), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->fade_unit_w, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->fade_unit_w);
  /*  automatically set the sensitive state of the fadeout stuff  */
  gtk_widget_set_sensitive (spinbutton, options->use_fade_d);
  gtk_widget_set_sensitive (options->fade_unit_w, options->use_fade_d);
  gtk_object_set_data (GTK_OBJECT (options->use_fade_w),
		       "set_sensitive", spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton),
		       "set_sensitive", options->fade_unit_w);
#endif

#if 0
  /*  the use gradient toggle  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->use_gradient_w =
    gtk_check_button_new_with_label (_("Gradient"));
  gtk_container_add (GTK_CONTAINER (abox), options->use_gradient_w);
  gtk_signal_connect (GTK_OBJECT (options->use_gradient_w), "toggled",
		      GTK_SIGNAL_FUNC (paintbrush_gradient_toggle_callback),
		      &options->use_gradient);
  gtk_widget_show (options->use_gradient_w);

  /*  the gradient length scale  */
  options->gradient_length_w =  
    gtk_adjustment_new (options->gradient_length_d,  1e-5, 32767.0, 1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (options->gradient_length_w), 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton), GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (options->gradient_length_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->gradient_length);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 1, 2);
  gtk_widget_show (spinbutton);

  /*  the gradient unitmenu  */
  options->gradient_unit_w = 
    gimp_unit_menu_new ("%a", options->gradient_unit_d, TRUE, TRUE, TRUE);
  gtk_signal_connect (GTK_OBJECT (options->gradient_unit_w), "unit_changed",
		      GTK_SIGNAL_FUNC (gimp_unit_menu_update),
		      &options->gradient_unit);
  gtk_object_set_data (GTK_OBJECT (options->gradient_unit_w), "set_digits",
		       spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->gradient_unit_w, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->gradient_unit_w);

  /*  the gradient type  */
  type_label = gtk_label_new (_("Type:"));
  gtk_misc_set_alignment (GTK_MISC (type_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (type_label);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 3, 2, 3);
  gtk_widget_show (abox);

  options->gradient_type_w = gimp_option_menu_new2
    (FALSE, gimp_menu_item_update,
     &options->gradient_type, (gpointer) options->gradient_type_d,

     _("Once Forward"),  (gpointer) ONCE_FORWARD, NULL,
     _("Once Backward"), (gpointer) ONCE_BACKWARDS, NULL,
     _("Loop Sawtooth"), (gpointer) LOOP_SAWTOOTH, NULL,
     _("Loop Triangle"), (gpointer) LOOP_TRIANGLE, NULL,

     NULL);
  gtk_container_add (GTK_CONTAINER (abox), options->gradient_type_w);
  gtk_widget_show (options->gradient_type_w);

  gtk_widget_show (table);

  /*  automatically set the sensitive state of the gradient stuff  */
  gtk_widget_set_sensitive (spinbutton, options->use_gradient_d);
  gtk_widget_set_sensitive (spinbutton, options->use_gradient_d);
  gtk_widget_set_sensitive (options->gradient_unit_w, options->use_gradient_d);
  gtk_widget_set_sensitive (options->gradient_type_w, options->use_gradient_d);
  gtk_widget_set_sensitive (type_label, options->use_gradient_d);
  gtk_widget_set_sensitive (options->paint_options.incremental_w,
			    ! options->use_gradient_d);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "set_sensitive",
		       spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton), "set_sensitive",
		       options->gradient_unit_w);
  gtk_object_set_data (GTK_OBJECT (options->gradient_unit_w), "set_sensitive",
		       options->gradient_type_w);
  gtk_object_set_data (GTK_OBJECT (options->gradient_type_w), "set_sensitive",
		       type_label);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "inverse_sensitive",
		       options->paint_options.incremental_w);
#endif

  tools_register_options (PAINTBRUSH, vbox);

  return options;
}

static void*
paintbrush_paint_func (PaintCore *paint_core,
		       CanvasDrawable *drawable,
		       int        state)
{
  GDisplay *gdisp = gdisplay_active ();
  double fade_out;
  double gradient_length;	
  double unit_factor;
  
  g_return_val_if_fail (gdisp != NULL, NULL);
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      switch (paintbrush_options->fade_unit)
	{
	case GIMP_UNIT_PIXEL:
	  fade_out = paintbrush_options->fade_out;
	  break;
	case GIMP_UNIT_PERCENT:
	  fade_out = MAX (gdisp->gimage->width, gdisp->gimage->height) * 
	    paintbrush_options->fade_out / 100;
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (paintbrush_options->fade_unit);
	  fade_out = paintbrush_options->fade_out * 
	    MAX (gdisp->gimage->xresolution, gdisp->gimage->yresolution) / unit_factor;
	  break;
	}
      paintbrush_motion (paint_core, drawable,
			 paintbrush_options->paint_options.pressure_options,
			 paintbrush_options->use_fade ? fade_out : 0, 
			 paintbrush_options->paint_options.incremental);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_paintbrush ()
{
  Tool * tool;
  PaintCore * private;

  if (! paintbrush_options)
    {
      paintbrush_options = paintbrush_options_new ();

      /*  press all default buttons  */
      paintbrush_options_reset ();
    }

  tool = paint_core_new (PAINTBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) paintbrush_paint_func;
  private->painthit_setup = paintbrush_painthit_setup;

  return tool;
}


void
tools_free_paintbrush (Tool *tool)
{
  paint_core_free (tool);
}


static void
paintbrush_motion (PaintCore *paint_core,
		   CanvasDrawable *drawable,
		   PaintPressureOptions *pressure_options,
		   double     fade_out,
		   gboolean   incremental)
{
  double paint_left;
  gdouble scale;
  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /* model the paint left on the brush as a gaussian curve */
  paint_left = 1;
  if (fade_out)
    {
      paint_left = ((double) paint_core->distance / fade_out);
      paint_left = exp (- paint_left * paint_left * 0.5);
    }

  /* apply the next bit of remaining paint */
  if (paint_left > 0.001)
    {
      gdouble pressure = 1.0;

      if(pressure_options->opacity)
        if(pressure_options->size)
          pressure = paint_core->curpressure;
        else
          pressure = pow(paint_core->curpressure, 2);

      /* Set up the painthit canvas */
      paint_core_16_area_setup (paint_core, drawable);
      
      /* apply it to the image */
      paint_core_16_area_paste (paint_core, drawable,
                                current_device ? (gfloat) paint_left * pressure : (gfloat) paint_left,
                                (gfloat) gimp_brush_get_opacity (),
                                pressure_options->pressure ? HARD : SOFT,
				scale,
                                incremental ? INCREMENTAL : CONSTANT,
                                gimp_brush_get_paint_mode ());
    }
}

void
paintbrush_painthit_setup (PaintCore * paint_core,Canvas * painthit)
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

void*
paintbrush_non_gui_paint_func(PaintCore *paint_core, CanvasDrawable *drawable, int state)
{
  paintbrush_motion(paint_core, drawable,
		     paintbrush_options->paint_options.pressure_options,
		     non_gui_fade_out, non_gui_incremental);
  return NULL;
}

void*
paintbrush_spline_non_gui_paint_func(PaintCore *paint_core, CanvasDrawable *drawable, int state)
{	
  double paint_left, scale=1.0;

  /* model the paint left on the brush as a gaussian curve */
  paint_left = 1;

  /* apply the next bit of remaining paint */
  if (paint_left > 0.001)
    {
      /* Set up the painthit canvas */
      paint_core_16_area_setup (paint_core, drawable);
      
      /* apply it to the image */
      paint_core_16_area_paste (paint_core, drawable, (float)paint_left,
				(float)gimp_brush_get_opacity(), SOFT,
				scale, CONSTANT, gimp_brush_get_paint_mode ());
    }
  return NULL;
}

/*  The paintbrush procedure definition  */
ProcArg paintbrush_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "fade_out",
    "fade out parameter: fade_out > 0"
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

ProcArg paintbrush_extended_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "fade_out",
    "fade out parameter: fade_out > 0"
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
    "method",
    "CONTINUOUS(0) or INCREMENTAL(1)"
  }
};


ProcRecord paintbrush_proc =
{
  "gimp_paintbrush",
  "Paint in the current brush with optional fade out parameter",
  "This tool is the standard paintbrush.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The \"fade_out\" parameter is measured in pixels and allows the brush stroke to linearly fall off.  The pressure is set to the maximum at the beginning of the stroke.  As the distance of the stroke nears the fade_out value, the pressure will approach zero.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  paintbrush_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { paintbrush_invoker } },
};

ProcRecord paintbrush_extended_proc =
{
  "gimp_paintbrush_extended",
  "Paint in the current brush with optional fade out parameter",
  "This tool is the standard paintbrush.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The \"fade_out\" parameter is measured in pixels and allows the brush stroke to linearly fall off.  The pressure is set to the maximum at the beginning of the stroke.  As the distance of the stroke nears the fade_out value, the pressure will approach zero.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  paintbrush_extended_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { paintbrush_extended_invoker } },
};

static Argument *
paintbrush_invoker (Argument *args)
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
  /*  fade out  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0)
	non_gui_fade_out = fp_value;
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
      non_gui_incremental = 0;
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	paintbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&paintbrush_proc, success);
}
static Argument *
paintbrush_extended_invoker (Argument *args)
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
  /*  fade out  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0)
	non_gui_fade_out = fp_value;
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
      non_gui_incremental = args[5].value.pdb_int;
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = (PaintFunc16) paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	paintbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&paintbrush_proc, success);
}
