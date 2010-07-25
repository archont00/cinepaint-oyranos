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
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs_area.h"
#include "paint_core_16.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "selection.h"
#include "smudge.h"
#include "tools.h"
#include "devices.h"

/*  the smudge structures  */

typedef struct SmudgeOptions SmudgeOptions;
struct SmudgeOptions
{
  double        pressure;
  double        pressure_d;
  GtkObject    *pressure_w;
};

/*  the smudge tool options  */
static SmudgeOptions * smudge_options = NULL;

static void smudge_painthit_setup (PaintCore *,Canvas *);
static void smudge_scale_update (GtkAdjustment *, double *);
static SmudgeOptions * create_smudge_options(void);
static void * smudge_paint_func (PaintCore *,  CanvasDrawable *, int);
static void smudge_nonclipped_painthit_coords (PaintCore *,
		gint*,gint*,gint*,gint*);
static void smudge_clipped_painthit_coords (PaintCore *, 	
		gint*, gint*, gint*, gint*);

Canvas *accum_canvas = NULL;
Canvas *linked_accum_canvas = NULL;

static void         smudge_motion 	(PaintCore *, CanvasDrawable *);
static void 	    smudge_init   	(PaintCore *, CanvasDrawable *);
static void         smudge_finish    (PaintCore *, CanvasDrawable *);

static void
smudge_scale_update (GtkAdjustment *adjustment,
			 double        *scale_val)
{
  *scale_val = adjustment->value;
}

static SmudgeOptions *
create_smudge_options(void)
{
  SmudgeOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;

  /*  the new smudge tool options structure  */
  options = (SmudgeOptions *) g_malloc_zero (sizeof (SmudgeOptions));
  options->pressure = options->pressure_d = 50.0;

  /*  the main vbox  */
  vbox = gtk_vbox_new(FALSE, 1);

  /*  the pressure scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Pressure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->pressure_w =
    gtk_adjustment_new (options->pressure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->pressure_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (options->pressure_w), "value_changed",
		      (GtkSignalFunc) smudge_scale_update,
		      &options->pressure);

  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  tools_register_options(SMUDGE, vbox);

  return options;
}

void *
smudge_paint_func (PaintCore    *paint_core,
		     CanvasDrawable *drawable,
		     int           state)
{
  switch (state)
    {
    case INIT_PAINT:
      smudge_init (paint_core, drawable);
      break;
    case MOTION_PAINT:
      smudge_motion (paint_core, drawable);
      break;
    case FINISH_PAINT:
      smudge_finish (paint_core, drawable);
      break;
    }
  return NULL;
}

static void
smudge_finish ( PaintCore *paint_core,
		 CanvasDrawable * drawable)
{
  /*d_printf("In smudge_finish\n");*/
  if (accum_canvas)
  {
    canvas_delete (accum_canvas);
    accum_canvas = NULL;
  }
  if (linked_accum_canvas)
  {
    canvas_delete (linked_accum_canvas);
    linked_accum_canvas = NULL;
  }
}

static void 
smudge_nonclipped_painthit_coords (PaintCore *paint_core,
	gint * x, gint* y, gint* w, gint *h)
{
  *w = canvas_width (paint_core->brush_mask);
  *h = canvas_height (paint_core->brush_mask);
  
  *x = (int) paint_core->curx - *w / 2;
  *y = (int) paint_core->cury - *h / 2;
}

static void 
smudge_clipped_painthit_coords (PaintCore *paint_core,
	gint * x, gint* y, gint* w, gint *h)
{
  gint bw,bh,dw,dh, x_upper_left,y_upper_left;
  gint x1, y1, x2, y2;

  smudge_nonclipped_painthit_coords (paint_core,
	&x_upper_left, &y_upper_left, &bw, &bh);
  
  dw = drawable_width (paint_core->drawable);
  dh = drawable_height (paint_core->drawable);

  /*this is the evil plus 1 border on the brush*/
  x1 = CLAMP (x_upper_left , 0, dw);
  y1 = CLAMP (y_upper_left , 0, dh);
  x2 = CLAMP (x_upper_left + bw , 0, dw);
  y2 = CLAMP (y_upper_left + bh , 0, dh);

  /* save the boundaries of this paint hit */
  *x = x1;
  *y = y1;
  *w = x2 - x1;
  *h = y2 - y1;
}

static void
smudge_init ( PaintCore *paint_core,
		CanvasDrawable * drawable)
{
	PixelArea srcPR, accumPR;
	gint x, y, w, h;
	gint xc, yc, wc, hc;

	gint was_clipped;
	Tag tag, linked_tag = tag_null();

	tag = drawable_tag (drawable);
	if (paint_core->linked_drawable) 
	{
		linked_tag = drawable_tag (paint_core->linked_drawable);
	}

	smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);
	smudge_clipped_painthit_coords (paint_core, &xc, &yc, &wc, &hc);

	if ( x != xc || y != yc || w != wc || h != hc )
		was_clipped = TRUE;
	else 
		was_clipped = FALSE;


	/*  Allocate the accumulation buffer */
	accum_canvas = canvas_new (tag_set_alpha (tag, tag_alpha (tag)), w, h, STORAGE_FLAT);
	if (paint_core->linked_drawable) 
		linked_accum_canvas = canvas_new (tag_set_alpha (linked_tag, tag_alpha (linked_tag)/*ALPHA_YES*/), w, h, STORAGE_FLAT);

	/* If there is a part clipped fill all of it with black first */ 
	if (was_clipped)
	{
		/* fill accum_canvas with black */
	}

	pixelarea_init (&srcPR, drawable_data (drawable), 
			xc, yc, wc, hc, FALSE);  
	pixelarea_init (&accumPR, accum_canvas, 
			0, 0, 0, 0, TRUE); /*xc - x, yc - y, wc , hc, TRUE);*/   




			/* copy the region under the original painthit into accum_canvas */
			copy_area (&srcPR, &accumPR);

			if (paint_core->linked_drawable)
			{ 
				pixelarea_init (&srcPR, drawable_data (paint_core->linked_drawable),
						xc, yc, wc, hc, FALSE);
				pixelarea_init (&accumPR, linked_accum_canvas,
						0, 0, 0, 0, TRUE); /*xc - x, yc - y, wc , hc, TRUE);*/
      /* copy the region under the original painthit into accum_canvas */
						copy_area (&srcPR, &accumPR);
			}
}

Tool *
tools_new_smudge ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! smudge_options)
      smudge_options = create_smudge_options();

  tool = paint_core_new (SMUDGE);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) smudge_paint_func;
  private->painthit_setup = smudge_painthit_setup;

  return tool;
}

void
tools_free_smudge (Tool *tool)
{
  paint_core_free (tool);
}

static void
smudge_painthit_setup (PaintCore * paint_core, Canvas * painthit)
{
  PixelArea srcPR, destPR, tempPR;
  gfloat pressure;
  /*Tag tag = drawable_tag(paint_core->drawable);*/
  gint x, y, w, h;
  gint xc, yc, wc, hc; 
  gint was_clipped;

  if (!painthit)
    return;

  if (paint_core->setup_mode == NORMAL_SETUP)
    {
      smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);
      smudge_clipped_painthit_coords(paint_core, &xc, &yc, &wc, &hc);

      if ( x != xc || 
	  y != yc || 
	  w != wc || 
	  h != hc )
	was_clipped = TRUE;
      else 
	was_clipped = FALSE;


      /* Get the data under the current painthit and blend with accum_canvas*/
      pixelarea_init (&srcPR, drawable_data (paint_core->drawable), 
	  paint_core->x, paint_core->y, 
	  paint_core->w, paint_core->h,
	  FALSE);

      /* Set tempPR as the accum buffer*/
      pixelarea_init (&tempPR, accum_canvas, xc-x, yc-y, wc, hc, TRUE);

      pressure= (smudge_options->pressure)/100.0;


      /* blend accum with current and place in accum buffer */ 
      blend_area (&srcPR, &tempPR, &tempPR, pressure, 
		      (gint)paint_core->pre_mult);

      /* Next copy from temp (accum_canvas) to painthit buffer*/
      pixelarea_init (&tempPR, accum_canvas, xc-x, yc-y, wc, hc, FALSE);
      pixelarea_init (&destPR, painthit, 
	  0, 0, 
	  paint_core->w, paint_core->h, 
	  TRUE);  

      /*  if (!drawable_has_alpha (paint_core->drawable))
	  add_alpha_area (&tempPR, &destPR);
	  else*/
      copy_area(&tempPR, &destPR);


    }
  else
    if (paint_core->setup_mode == LINKED_SETUP)
      {
	smudge_nonclipped_painthit_coords (paint_core, &x, &y, &w, &h);
	smudge_clipped_painthit_coords(paint_core, &xc, &yc, &wc, &hc);

	if ( x != xc || y != yc || w != wc || h != hc )
	  was_clipped = TRUE;
	else 
	  was_clipped = FALSE;


	/* Get the data under the current painthit and blend with accum_canvas*/
	pixelarea_init (&srcPR, drawable_data (paint_core->linked_drawable), 
	    paint_core->x, paint_core->y, 
	    paint_core->w, paint_core->h,
	    FALSE);

	/* Set tempPR as the accum buffer*/
	pixelarea_init (&tempPR, linked_accum_canvas, 0, 0, 0, 0, TRUE); /*xc-x, yc-y, wc, hc, TRUE);*/

	pressure= (smudge_options->pressure)/100.0;


	/* blend accum with current and place in accum buffer */ 
	blend_area (&srcPR, &tempPR, &tempPR, pressure,
		      paint_core->pre_mult);
 
	/* Next copy from temp (linked_accum_canvas) to painthit buffer*/
	pixelarea_init (&tempPR, linked_accum_canvas, 0, 0, 0, 0, FALSE); /*xc-x, yc-y, wc, hc, FALSE);*/
	pixelarea_init (&destPR, painthit, 
	    0, 0, 
	    paint_core->w, paint_core->h, 
	    TRUE);  

	/*  if (!drawable_has_alpha (paint_core->drawable))
	    add_alpha_area (&tempPR, &destPR);
	    else 
	 */  copy_area(&tempPR, &destPR);
      }
}

static void
smudge_motion (PaintCore *paint_core,
    CanvasDrawable *drawable)
{
  GImage *gimage;
  Tag tag = drawable_tag (drawable);

  if (! (gimage = drawable_gimage (drawable)))
    return;
  if (tag_format (tag) == FORMAT_INDEXED )
    return; 

  /* Set up the painthit with the right thing*/ 
  paint_core_16_area_setup (paint_core, drawable);                                            
  /* Apply the painthit to the drawable */                                                    
  paint_core_16_area_replace (paint_core, drawable, 
      current_device ? paint_core->curpressure : 1.0, 
	gimp_brush_get_opacity(), SOFT, INCREMENTAL, NORMAL_MODE);
}

static void *
smudge_non_gui_paint_func (PaintCore *paint_core,
			     CanvasDrawable *drawable,
			     int        state)
{
  smudge_motion (paint_core, drawable);

  return NULL;
}

gboolean
smudge_non_gui (CanvasDrawable *drawable,
    		  double        pressure,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = (PaintFunc16) smudge_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	smudge_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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
