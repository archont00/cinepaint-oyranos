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
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "boundary.h"
#include "by_color_select.h"
#include "canvas.h"
#include "colormaps.h"
#include "drawable.h"
#include "draw_core.h"
#include "general.h"
#include "gimage_mask.h"
#include "rc.h"
#include "gdisplay.h"
#include "rect_select.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "minimize.h"


#define DEFAULT_FUZZINESS 0.10 /* was 15/255 originally */
#define PREVIEW_WIDTH   256
#define PREVIEW_HEIGHT  256
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK


typedef struct ByColorSelect ByColorSelect;

struct ByColorSelect
{
  int  x, y;         /*  Point from which to execute seed fill  */
  int  operation;    /*  add, subtract, normal color selection  */
};

typedef struct ByColorDialog ByColorDialog;

struct ByColorDialog
{
  GtkWidget   *shell;

  gfloat       threshold; /*  threshold value for color select  */
  int          operation; /*  Add, Subtract, Replace  */
  GImage      *gimage;    /*  gimage which is currently under examination  */
};

/*  by_color select action functions  */

static void   by_color_select_button_press   (Tool *, GdkEventButton *, gpointer);
static void   by_color_select_button_release (Tool *, GdkEventButton *, gpointer);
static void   by_color_select_motion         (Tool *, GdkEventMotion *, gpointer);
static void   by_color_select_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   by_color_select_control        (Tool *, int, gpointer);

static ByColorDialog *  by_color_select_new_dialog     (void);
static void             by_color_select_type_callback  (GtkWidget *, gpointer);
static void             by_color_select_reset_callback (GtkWidget *, gpointer);
static void             by_color_select_close_callback (GtkWidget *, gpointer);
static gint             by_color_select_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void             by_color_select_fuzzy_update   (GtkAdjustment *, gpointer);

static SelectionOptions *by_color_options = NULL;
static ByColorDialog *by_color_dialog = NULL;

static Channel *  by_color_select_color (GImage *, CanvasDrawable *, PixelRow *, int, gfloat, int);
static void       by_color_select (GImage *, CanvasDrawable *, PixelRow *, gfloat, int, int, int, double, int);
static Argument * by_color_select_invoker (Argument *);

/*  by_color selection machinery  */

static Channel *
by_color_select_color (GImage        *gimage,
		       CanvasDrawable  *drawable,
		       PixelRow      *color,
		       int            antialias,
		       gfloat         threshold,
		       int            sample_merged)
{
  /*  Scan over the gimage's active layer, finding pixels within the specified
   *  threshold from the given R, G, & B values.  If antialiasing is on,
   *  use the same antialiasing scheme as in fuzzy_select.  Modify the gimage's
   *  mask to reflect the additional selection
   */
  PixelArea imagePR, maskPR;
  Channel *mask;
  Canvas * image;
  int width, height;

  if (sample_merged)
    {
      width = gimage->width;
      height = gimage->height;
      image = gimage_projection (gimage);
    }
  else
    {
      width = drawable_width (drawable);
      height = drawable_height (drawable);
      image = drawable_data (drawable);
    }

  mask = channel_new_mask (gimage->ID,
                           width, height,
                           tag_precision (drawable_tag (drawable)));

  pixelarea_init (&imagePR, image,
                  0, 0,
                  width, height,
                  FALSE);

  pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 
                  0, 0,
                  width, height,
                  TRUE);

  absdiff_area (&imagePR, &maskPR, color, threshold, antialias);
  
  return mask;
}


static void
by_color_select (GImage        *gimage,
		 CanvasDrawable  *drawable,
		 PixelRow      *color,
		 gfloat         threshold,
		 int            op,
		 int            antialias,
		 int            feather,
		 double         feather_radius,
		 int            sample_merged)
{
  Channel * new_mask;
  int off_x, off_y;

  if (!drawable) 
    return;

  new_mask = by_color_select_color (gimage, drawable, color, antialias, threshold, sample_merged);

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  if (sample_merged)
    {
      off_x = 0; off_y = 0;
    }
  else
    drawable_offsets (drawable, &off_x, &off_y);

  if (feather)
    channel_feather (new_mask, gimage_get_mask (gimage),
		     feather_radius, op, off_x, off_y);
  else
    channel_combine_mask (gimage_get_mask (gimage),
			  new_mask, op, off_x, off_y);
  channel_delete (new_mask);
}


/*  by_color select action functions  */

static void
by_color_select_button_press (Tool           *tool,
			      GdkEventButton *bevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  ByColorSelect *by_color_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  by_color_sel = (ByColorSelect *) tool->private;

  tool->drawable = gimage_active_drawable (gdisp->gimage);

  if (!by_color_dialog)
    return;

  by_color_sel->x = bevent->x;
  by_color_sel->y = bevent->y;

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  /*  Defaults  */
  by_color_sel->operation = REPLACE;

  /*  Based on modifiers, and the "by color" dialog's selection mode  */
  if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    by_color_sel->operation = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    by_color_sel->operation = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    by_color_sel->operation = INTERSECT;
  else
    by_color_sel->operation = by_color_dialog->operation;

  /*  Make sure the "by color" select dialog is visible  */
  if (! GTK_WIDGET_VISIBLE (by_color_dialog->shell))
    gtk_widget_show (by_color_dialog->shell);

  /*  Update the by_color_dialog's active gdisp pointer  */
  if (by_color_dialog->gimage)
    by_color_dialog->gimage->by_color_select = FALSE;
  by_color_dialog->gimage = gdisp->gimage;
  gdisp->gimage->by_color_select = TRUE;
}

static void
by_color_select_button_release (Tool           *tool,
				GdkEventButton *bevent,
				gpointer        gdisp_ptr)
{
  ByColorSelect * by_color_sel;
  GDisplay * gdisp;
  int x, y;
  CanvasDrawable *drawable;
  int use_offsets;

  gdisp = (GDisplay *) gdisp_ptr;
  by_color_sel = (ByColorSelect *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      Canvas * canvas;
  
      use_offsets = (by_color_options->sample_merged) ? FALSE : TRUE;
      gdisplay_untransform_coords (gdisp, by_color_sel->x, by_color_sel->y, &x, &y, FALSE, use_offsets);

      /*  Get the start color  */
      if (by_color_options->sample_merged)
	{
	  if (x < 0 || y < 0 || x >= gdisp->gimage->width || y >= gdisp->gimage->height)
	    return;
          canvas = gimage_projection (gdisp->gimage);
	}
      else
	{
	  if (x < 0 || y < 0 || x >= drawable_width (drawable) || y >= drawable_height (drawable))
	    return;
          canvas = drawable_data (drawable);
	}

      if (canvas_portion_refro (canvas, x, y) == REFRC_OK)
        {
          PixelRow col;

          pixelrow_init (&col, canvas_tag (canvas), canvas_portion_data (canvas, x, y), 1);
          
          /*  select the area  */
          by_color_select (gdisp->gimage, drawable, &col,
                           by_color_dialog->threshold,
                           by_color_sel->operation,
                           by_color_options->antialias,
                           by_color_options->feather,
                           by_color_options->feather_radius,
                           by_color_options->sample_merged);

          canvas_portion_unref (canvas, x, y);
          
          /*  show selection on all views  */
          gdisplays_flush ();

        }
    }
}

static void
by_color_select_motion (Tool           *tool,
			GdkEventMotion *mevent,
			gpointer        gdisp_ptr)
{
}

static void
by_color_select_cursor_update (Tool           *tool,
			       GdkEventMotion *mevent,
			       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
    if (layer == gdisp->gimage->active_layer)
      {
	gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
	return;
      }
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
by_color_select_control (Tool     *tool,
			 int       action,
			 gpointer  gdisp_ptr)
{
  ByColorSelect * by_color_sel;

  by_color_sel = (ByColorSelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (by_color_dialog)
	{
	  if (by_color_dialog->gimage)
	    by_color_dialog->gimage->by_color_select = FALSE;
	  by_color_dialog->gimage = NULL;
	  by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
	}
      break;
    }
}

Tool *
tools_new_by_color_select ()
{
  Tool * tool;
  ByColorSelect * private;

  /*  The tool options  */
  if (!by_color_options)
    by_color_options = create_selection_options (BY_COLOR_SELECT);

  /*  The "by color" dialog  */
  if (!by_color_dialog)
    by_color_dialog = by_color_select_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (by_color_dialog->shell))
      gtk_widget_show (by_color_dialog->shell);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (ByColorSelect *) g_malloc (sizeof (ByColorSelect));

  tool->type = BY_COLOR_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = by_color_select_button_press;
  tool->button_release_func = by_color_select_button_release;
  tool->motion_func = by_color_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = by_color_select_cursor_update;
  tool->control_func = by_color_select_control;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_by_color_select (Tool *tool)
{
  ByColorSelect * by_color_sel;

  by_color_sel = (ByColorSelect *) tool->private;

  /*  Close the color select dialog  */
  if (by_color_dialog)
    {
      if (by_color_dialog->gimage)
	by_color_dialog->gimage->by_color_select = FALSE;
      by_color_select_close_callback (NULL, (gpointer) by_color_dialog);
    }

  g_free (by_color_sel);
}

void
by_color_select_initialize (void *gimage_ptr)
{
  GImage *gimage;

  gimage = (GImage *) gimage_ptr;

}


/****************************/
/*  Select by Color dialog  */
/****************************/

static ByColorDialog *
by_color_select_new_dialog ()
{
  ByColorDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *options_box;
  GtkWidget *label;
  GtkWidget *util_box;
  GtkWidget *push_button;
  GtkWidget *slider;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkObject *data;
  GSList *group = NULL;
  guint i;
  char *button_names[4] =
  {
    _("Replace"),
    _("Add"),
    _("Subtract"),
    _("Intersect")
  };
  int button_values[4] =
  {
    REPLACE,
    ADD,
    SUB,
    INTERSECT
  };

  bcd = g_malloc_zero (sizeof (ByColorDialog));
  bcd->gimage = NULL;
  bcd->operation = REPLACE;
  bcd->threshold = DEFAULT_FUZZINESS;

  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "by_color_selection", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (bcd->shell), _("By Color Selection"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (bcd->shell)->action_area), 2);
  minimize_register(bcd->shell);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      (GtkSignalFunc) by_color_select_delete_callback,
		      bcd);

  /*  The vbox */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The horizontal box containing options box */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  options box  */
  options_box = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (options_box), 5);
  gtk_box_pack_start (GTK_BOX (hbox), options_box, TRUE, TRUE, 0);

  /*  Create the selection mode radio box  */
  frame = gtk_frame_new (_("Selection Mode"));
  gtk_box_pack_start (GTK_BOX (options_box), frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < (sizeof(button_names) / sizeof(button_names[0])); i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) by_color_select_type_callback,
			  (gpointer) ((long) button_values[i]));
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (frame);

  /*  Create the opacity scale widget  */
  util_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options_box), util_box, FALSE, FALSE, 0);
  label = gtk_label_new (_("Fuzziness Threshold"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  data = gtk_adjustment_new (bcd->threshold, 0.0, 1.0, 0.01, 0.1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) by_color_select_fuzzy_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (util_box);

  /*  The reset push button  */
  push_button = gtk_button_new_with_label (_("Reset"));
  GTK_WIDGET_SET_FLAGS (push_button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->action_area), push_button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (push_button), "clicked",
		      (GtkSignalFunc) by_color_select_reset_callback,
		      bcd);
  gtk_widget_grab_default (push_button);
  gtk_widget_show (push_button);

  /*  The close push button  */
  push_button = gtk_button_new_with_label (_("Close"));
  GTK_WIDGET_SET_FLAGS (push_button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->action_area), push_button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (push_button), "clicked",
		  (GtkSignalFunc) by_color_select_close_callback,
		  bcd);
  gtk_widget_show (push_button);

  gtk_widget_show (options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}


static void
by_color_select_type_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  if (by_color_dialog)
    by_color_dialog->operation = (long) client_data;
}

static void
by_color_select_reset_callback (GtkWidget *widget,
				gpointer   client_data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) client_data;

  if (!bcd->gimage)
    return;

  /*  check if the image associated to the mask still exists  */
  if (!drawable_gimage (GIMP_DRAWABLE(gimage_get_mask (bcd->gimage))))
    return;

  /*  reset the mask  */
  gimage_mask_clear (bcd->gimage);

  /*  show selection on all views  */
  gdisplays_flush ();

}

static gint
by_color_select_delete_callback (GtkWidget *w,
				 GdkEvent  *e,
				 gpointer   client_data)
{
  by_color_select_close_callback (w, client_data);

  return TRUE;
}

static void
by_color_select_close_callback (GtkWidget *widget,
				gpointer   client_data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);
}

static void
by_color_select_fuzzy_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  ByColorDialog *bcd;

  bcd = (ByColorDialog *) data;
  bcd->threshold = adjustment->value;
}


/*  The by_color_select procedure definition  */
ProcArg by_color_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_COLOR,
    "color",
    "the color to select"
  },
  { PDB_INT32,
    "threshold",
    "threshold in intensity levels: 0 <= threshold <= 255"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing On/Off"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  }
};

ProcRecord by_color_select_proc =
{
  "gimp_by_color_select",
  "Create a selection by selecting all pixels (in the specified drawable) with the same (or similar) color to that specified.",
  "This tool creates a selection over the specified image.  A by-color selection is determined by the supplied color under the constraints of the specified threshold.  Essentially, all pixels (in the drawable) that have color sufficiently close to the specified color (as determined by the threshold value) are included in the selection.  The antialiasing parameter allows the final selection mask to contain intermediate values based on close misses to the threshold bar.  Feathering can be enabled optionally and is controlled with the \"feather_radius\" paramter.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  by_color_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { by_color_select_invoker } },
};


static Argument *
by_color_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  CanvasDrawable *drawable;
  int op;
  gfloat threshold;
  int antialias=0;
  int feather=0;
  int sample_merged=0;
  guchar _color[TAG_MAX_BYTES];
  PixelRow color;
  double feather_radius=0;
  int int_value;

  drawable    = NULL;
  op          = REPLACE;
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
  /*  color  */
  if (success)
    {
      unsigned char *color_array;
      PixelRow temp;
      
      color_array = (unsigned char *) args[2].value.pdb_pointer;

      pixelrow_init (&temp, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     color_array, 1);
      pixelrow_init (&color, drawable_tag (drawable),
                     _color, 1);
      copy_row (&temp, &color);
    }
  /*  threshold  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value <= 255)
	threshold = (gfloat) int_value / 255.0;
      else
	success = FALSE;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing?  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[7].value.pdb_float;
    }
  /*  sample merged  */
  if (success)
    {
      int_value = args[8].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }

  /*  call the ellipse_select procedure  */
  if (success)
    by_color_select (gimage, drawable, &color, threshold, op,
		     antialias, feather, feather_radius, sample_merged);

  return procedural_db_return_args (&by_color_select_proc, success);
}
