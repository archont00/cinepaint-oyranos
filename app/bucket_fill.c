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
#include "depth/brush_select.h"
#include "bucket_fill.h"
#include "canvas.h"
#include "drawable.h"
#include "fuzzy_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "interface.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "patterns.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "selection.h"
#include "tools.h"
#include "undo.h"

/*  the Bucket Fill structures  */
typedef enum
{
  FgColorFill,
  BgColorFill
} FillMode;

typedef struct BucketTool BucketTool;

struct BucketTool
{
  int     target_x;   /*  starting x coord          */
  int     target_y;   /*  starting y coord          */
};

/*  local function prototypes  */
static void  bucket_fill_button_press    (Tool *, GdkEventButton *, gpointer);
static void  bucket_fill_button_release  (Tool *, GdkEventButton *, gpointer);
static void  bucket_fill_motion          (Tool *, GdkEventMotion *, gpointer);
static void  bucket_fill_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void  bucket_fill_control         (Tool *, int, gpointer);

static void  bucket_fill                 (GImage *, CanvasDrawable *, FillMode, int,
					  double, double, int, double, double);

static Argument *bucket_fill_invoker     (Argument *);


typedef struct BucketOptions BucketOptions;

struct BucketOptions
{
  double    opacity;
  double    threshold;
  FillMode  fill_mode;
  int       paint_mode;
  int       sample_merged;
};

/*  local variables  */
static BucketOptions *bucket_options = NULL;


static void
bucket_fill_toggle_update (GtkWidget *widget,
			   gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
bucket_fill_scale_update (GtkAdjustment *adjustment,
			  gpointer       data)
{
  double *scale_val;

  scale_val = (double *) data;
  *scale_val = adjustment->value;
}


static void
bucket_fill_paint_mode_callback (GtkWidget *w,
				 gpointer   client_data)
{
  bucket_options->paint_mode = (long) client_data;
}

static BucketOptions *
create_bucket_options (void)
{
  BucketOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *opacity_scale;
  GtkWidget *sample_merged_toggle;
  GtkWidget *threshold_scale;
  GtkObject *opacity_scale_data;
  GtkObject *threshold_scale_data;

  /*  the new options structure  */
  options = (BucketOptions *) g_malloc_zero (sizeof (BucketOptions));
  options->opacity = 100.0;
  options->threshold = 7;
  options->fill_mode = FgColorFill;
  options->paint_mode = NORMAL;
  options->sample_merged = FALSE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Bucket Fill Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the opacity scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fill Opacity"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), opacity_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (opacity_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) bucket_fill_scale_update,
		      &options->opacity);
  gtk_widget_show (opacity_scale);
  gtk_widget_show (hbox);

  /*  the threshold scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fill Threshold"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  threshold_scale_data = gtk_adjustment_new (7.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT (threshold_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), threshold_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (threshold_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (threshold_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (threshold_scale_data), "value_changed",
		      (GtkSignalFunc) bucket_fill_scale_update,
		      &options->threshold);
  gtk_widget_show (threshold_scale);
  gtk_widget_show (hbox);

  /*  the paint mode menu  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  menu = create_paint_mode_menu (bucket_fill_paint_mode_callback, NULL);
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (hbox);

  /*  the sample merged toggle  */
  sample_merged_toggle = gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_signal_connect (GTK_OBJECT (sample_merged_toggle), "toggled",
		      (GtkSignalFunc) bucket_fill_toggle_update,
		      &options->sample_merged);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (sample_merged_toggle), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), sample_merged_toggle, FALSE, FALSE, 0);
  gtk_widget_show (sample_merged_toggle);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (BUCKET_FILL, vbox);

  /*  Post initialization  */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  return options;
}


static void
bucket_fill_button_press (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  BucketTool * bucket_tool;
  int use_offsets;

  gdisp = (GDisplay *) gdisp_ptr;
  bucket_tool = (BucketTool *) tool->private;

  use_offsets = (bucket_options->sample_merged) ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp,CAST(int) bevent->x,CAST(int) bevent->y,
			       &bucket_tool->target_x,
			       &bucket_tool->target_y, FALSE, use_offsets);

  /*  Make the tool active and set the gdisplay which owns it  */
  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;
}


static void
bucket_fill_button_release (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  GDisplay * gdisp;
  BucketTool * bucket_tool;
  FillMode fill_mode;
  Argument *return_vals;
  int nreturn_vals;

  gdisp = (GDisplay *) gdisp_ptr;
  bucket_tool = (BucketTool *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      fill_mode = bucket_options->fill_mode;

      /*  If the mode is color filling, and shift mask is down, fill with background  */
      if (bevent->state & GDK_SHIFT_MASK && fill_mode == FgColorFill)
	fill_mode = BgColorFill;

      return_vals = procedural_db_run_proc ("gimp_bucket_fill",
					    &nreturn_vals,
					    PDB_IMAGE, gdisp->gimage->ID,
					    PDB_DRAWABLE, 
					    drawable_ID (gimage_active_drawable (gdisp->gimage)),
					    PDB_INT32, (gint32) fill_mode,
					    PDB_INT32, (gint32) bucket_options->paint_mode,
					    PDB_FLOAT, (gdouble) bucket_options->opacity,
					    PDB_FLOAT, (gdouble) bucket_options->threshold * 2.55,
					    PDB_INT32, (gint32) bucket_options->sample_merged,
					    PDB_FLOAT, (gdouble) bucket_tool->target_x,
					    PDB_FLOAT, (gdouble) bucket_tool->target_y,
					    PDB_END);

      if (return_vals[0].value.pdb_int == PDB_SUCCESS)
	gdisplays_flush ();
      else
	g_message (_("Bucket Fill operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  tool->state = INACTIVE;
}


static void
bucket_fill_motion (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
}


static void
bucket_fill_cursor_update (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;
  int off_x, off_y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
	  x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	  y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimage_mask_is_empty (gdisp->gimage))
	    ctype = GDK_TCROSS;
	  else if (gimage_mask_value (gdisp->gimage, x, y) != 0)
	    ctype = GDK_TCROSS;
	}
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}


static void
bucket_fill_control (tool, action, gdisp_ptr)
     Tool * tool;
     int action;
     gpointer gdisp_ptr;
{
}


static void
bucket_fill (gimage, drawable, fill_mode, paint_mode,
	     opacity, threshold, sample_merged, x, y)
     GImage *gimage;
     CanvasDrawable *drawable;
     FillMode fill_mode;
     int paint_mode;
     double opacity;
     double threshold;
     int sample_merged;
     double x, y;
{
  PixelArea maskPR;
  Channel * mask = NULL;
  int x1, y1, x2, y2;

  
  /* error check args */
  switch (fill_mode)
    {
    case FgColorFill:
    case BgColorFill:
      break;
    default:
      g_warning (_("bad fill type"));
      return;
    }


  /* we need a selection mask.  either use the existing one, or
     calculate a new one using a seed fill */
  if (! drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      mask = find_contiguous_region (gimage, drawable, TRUE, threshold,
				     (int) x, (int) y, sample_merged);

      channel_bounds (mask, &x1, &y1, &x2, &y2);

      /*  make sure we handle the mask correctly if it was sample-merged  */
      if (sample_merged)
	{
	  int off_x, off_y;

	  /*  Limit the channel bounds to the drawable's extents  */
	  drawable_offsets (drawable, &off_x, &off_y);

	  x1 = BOUNDS (x1, off_x, (off_x + drawable_width (drawable)));
	  y1 = BOUNDS (y1, off_y, (off_y + drawable_height (drawable)));
	  x2 = BOUNDS (x2, off_x, (off_x + drawable_width (drawable)));
	  y2 = BOUNDS (y2, off_y, (off_y + drawable_height (drawable)));

	  pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 
                          x1, y1,
                          (x2 - x1), (y2 - y1),
                          FALSE);

	  /*  translate mask bounds to drawable coords  */
	  x1 -= off_x;
	  y1 -= off_y;
	  x2 -= off_x;
	  y2 -= off_y;
	}
      else
        {
          pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 
                          x1, y1,
                          (x2 - x1), (y2 - y1),
                          FALSE);
        }
    }

  {
    PixelArea bufPR, mbufPR;
    Canvas * buf_tiles = NULL, *buf_m_tiles = NULL;
   
    buf_tiles = canvas_new (tag_set_alpha (drawable_tag (drawable), ALPHA_YES),
                            (x2 - x1), (y2 - y1),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
   
    if (gimage->channels && channel_get_link_paint ((Channel*)(gimage->channels->data)))
      {
	buf_m_tiles = canvas_new (tag_set_alpha (
	      drawable_tag (GIMP_DRAWABLE((Channel*)(gimage->channels->data))), ALPHA_NO),
	    (x2 - x1), (y2 - y1),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
   
	pixelarea_init (&mbufPR, buf_m_tiles,
	    0, 0,
	    0, 0,
	    TRUE);
      }       
    pixelarea_init (&bufPR, buf_tiles,
                    0, 0,
                    0, 0,
                    TRUE);

    switch (fill_mode)
      {
      case FgColorFill:
      case BgColorFill:
        {
          COLOR16_NEW (col, canvas_tag (buf_tiles));
          COLOR16_INIT (col);
          
          if (fill_mode == FgColorFill)
            palette_get_foreground (&col);
          else
            palette_get_background (&col);
        
          color_area (&bufPR, &col);
	  if (gimage->channels && channel_get_link_paint ((Channel*)(gimage->channels->data)))
	    {
	      COLOR16_NEW (col, canvas_tag (buf_m_tiles));
	      COLOR16_INIT (col);

	      if (fill_mode == FgColorFill)
		palette_get_foreground (&col);
	      else
		palette_get_background (&col);

	      color_area (&mbufPR, &col);

	    }
        }
        break;
      
      default:
        g_warning (_("bad fill type"));
        return;
      }

    /* if we did a seed fill, copy it to the alpha channel */
    if (mask)
      {
        pixelarea_init (&bufPR, buf_tiles,
                        0, 0,
                        0, 0,
                        TRUE);
        
        apply_mask_to_area  (&bufPR, &maskPR, 1.0);
      }
  
    /* apply painthit to the image */
    gimage_apply_painthit (gimage, drawable,
                           NULL, buf_tiles,
                           0, 0,
                           0, 0,
                           TRUE,
                           (opacity / 100.0), paint_mode, x1, y1);
    if (gimage->channels && channel_get_link_paint ((Channel*)(gimage->channels->data))){
     
	  
	  gimage_apply_painthit (gimage, GIMP_DRAWABLE(((Channel*)(gimage->channels->data))),
	  NULL, buf_m_tiles,
	  0, 0,
	  0, 0,
	  TRUE,
	  (opacity / 100.0), paint_mode, x1, y1);
    canvas_delete (buf_m_tiles);
    }
    canvas_delete (buf_tiles);
  }
  
  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
  if (gimage->channels && channel_get_link_paint ((Channel*)(gimage->channels->data)))
    drawable_update (GIMP_DRAWABLE(((Channel*)(gimage->channels->data))), 
      x1, y1, (x2 - x1), (y2 - y1));

  /* clean up */
  if (mask)
    channel_delete (mask);
}


/*********************************/
/*  Global bucket fill functions */
/*********************************/


Tool *
tools_new_bucket_fill ()
{
  Tool * tool;
  BucketTool * private;

  if (! bucket_options)
    bucket_options = create_bucket_options ();

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (BucketTool *) g_malloc_zero (sizeof (BucketTool));

  tool->type = BUCKET_FILL;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = private;
  tool->button_press_func = bucket_fill_button_press;
  tool->button_release_func = bucket_fill_button_release;
  tool->motion_func = bucket_fill_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = bucket_fill_cursor_update;
  tool->control_func = bucket_fill_control;
  tool->preserve = TRUE;

  return tool;
}


void
tools_free_bucket_fill (tool)
     Tool * tool;
{
  BucketTool * bucket_tool;

  bucket_tool = (BucketTool *) tool->private;

  g_free (bucket_tool);
}



/*  The bucket fill procedure definition  */
ProcArg bucket_fill_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The affected drawable"
  },
  { PDB_INT32,
    "fill_mode",
    "The type of fill: { FG-BUCKET-FILL (0), BG-BUCKET-FILL (1), PATTERN-BUCKET-FILL (2) }"
  },
  { PDB_INT32,
    "paint_mode",
    "the paint application mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  },
  { PDB_FLOAT,
    "opacity",
    "The opacity of the final bucket fill (0 <= opacity <= 100)"
  },
  { PDB_FLOAT,
    "threshold",
    "The threshold determines how extensive the seed fill will be.  It's value is specified in terms of intensity levels (0 <= threshold <= 255).  This parameter is only valid when there is no selection in the specified image."
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  },
  { PDB_FLOAT,
    "x",
    "The x coordinate of this bucket fill's application.  This parameter is only valid when there is no selection in the specified image."
  },
  { PDB_FLOAT,
    "y",
    "The y coordinate of this bucket fill's application.  This parameter is only valid when there is no selection in the specified image."
  },
};

ProcRecord bucket_fill_proc =
{
  "gimp_bucket_fill",
  "Fill the area specified either by the current selection if there is one, or by a seed fill starting at the specified coordinates.",
  "This tool requires information on the paint application mode, and the fill mode, which can either be in the foreground color, or in the currently active pattern.  If there is no selection, a seed fill is executed at the specified coordinates and extends outward in keeping with the threshold parameter.  If there is a selection in the target image, the threshold, sample merged, x, and y arguments are unused.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of merged sampling, the x,y coordinates are relative to the image's origin; otherwise, they are relative to the drawable's origin.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  bucket_fill_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { bucket_fill_invoker } },
};


static Argument *
bucket_fill_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  CanvasDrawable *drawable;
  FillMode fill_mode;
  int paint_mode;
  double opacity;
  double threshold;
  int sample_merged;
  double x, y;
  int int_value;
  double fp_value;

  drawable    = NULL;
  fill_mode   = BgColorFill;
  paint_mode  = NORMAL_MODE;
  opacity     = 100.0;
  threshold   = 0.0;

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
  /*  fill mode  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: fill_mode = FgColorFill; break;
	case 1: fill_mode = BgColorFill; break;
	default: success = FALSE;
	}
    }
  /*  paint mode  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= NORMAL_MODE && int_value <= VALUE_MODE)
	paint_mode = int_value;
      else
	success = FALSE;
    }
  /*  opacity  */
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	opacity = fp_value;
      else
	success = FALSE;
    }
  /*  threshold  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 255.0)
	threshold = fp_value / 255.0;
      else
	success = FALSE;
    }
  /*  sample_merged  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }
  /*  x, y  */
  if (success)
    {
      x = args[7].value.pdb_float;
      y = args[8].value.pdb_float;
    }

  /*  call the blend procedure  */
  if (success)
    {
      bucket_fill (gimage, drawable, fill_mode, paint_mode,
		   opacity, threshold, sample_merged, x, y);
    }

  return procedural_db_return_args (&bucket_fill_proc, success);
}
