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
#include "actionarea.h"
#include "expose_image.h"
#include "canvas.h"
#include "drawable.h"
#include "general.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "image_render.h"
#include "interface.h"
#include "pixelarea.h"
#include "pixelrow.h"

#define USE_OFFSET 0

#define TEXT_WIDTH 45
#define TEXT_HEIGHT 25
#define SLIDER_WIDTH 200
#define SLIDER_HEIGHT 35

#define GAMMA_SLIDER  1
#define EXPOSE_SLIDER 2
#define OFFSET_SLIDER 4
#define GAMMA_TEXT    8
#define EXPOSE_TEXT  16
#define OFFSET_TEXT  32
#define ALL         0xF


typedef struct ExposeImage ExposeImage;

struct ExposeImage
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct ExposeImageDialog ExposeImageDialog;

struct ExposeImageDialog
{
  GtkWidget   *shell;
  GtkWidget   *gimage_name;
  GtkWidget   *gamma_text;
  GtkWidget   *expose_text;
  GtkWidget   *offset_text;
  GtkAdjustment  *gamma_data;
  GtkAdjustment  *expose_data;
  GtkAdjustment  *offset_data;

  GDisplay    *gdisp;
  CanvasDrawable *drawable;

  gint         preview;
  gint         forceexpose;
};

/*  gamma expose action functions  */

static void   expose_view_button_press   (Tool *, GdkEventButton *, gpointer);
static void   expose_view_button_release (Tool *, GdkEventButton *, gpointer);
static void   expose_view_motion         (Tool *, GdkEventMotion *, gpointer);
static void   expose_view_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   expose_view_control        (Tool *, int, gpointer);

static ExposeImageDialog *  expose_view_new_dialog  (GDisplay *gdisp);
static void   expose_view_update                  (ExposeImageDialog *, int);
static void   expose_view_preview                 (ExposeImageDialog *);
static void   expose_view_ok_callback             (GtkWidget *, gpointer);
static void   expose_view_reset_callback          (GtkWidget *, gpointer);
static gint   expose_view_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void   expose_view_gamma_scale_update (GtkAdjustment *, gpointer);
static void   expose_view_expose_scale_update   (GtkAdjustment *, gpointer);
static void   expose_view_offset_scale_update   (GtkAdjustment *, gpointer);
static void   expose_view_gamma_text_update  (GtkWidget *, gpointer);
static void   expose_view_expose_text_update    (GtkWidget *, gpointer);
static void   expose_view_offset_text_update    (GtkWidget *, gpointer);
static gint   expose_view_gamma_text_check (const char *, ExposeImageDialog *);
static gint   expose_view_expose_text_check (const char *, ExposeImageDialog *);
static gint   expose_view_offset_text_check (const char *, ExposeImageDialog *);

static void *expose_view_options = NULL;
static ExposeImageDialog *expose_view_dialog = NULL;

static Argument * expose_view_invoker  (Argument *);


static void
expose_view_button_press (Tool           *tool,
				  GdkEventButton *bevent,
				  gpointer        gdisp_ptr)
{
}

static void
expose_view_button_release (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr)
{
}

static void
expose_view_motion (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
}

static void
expose_view_cursor_update (Tool           *tool,
				   GdkEventMotion *mevent,
				   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
expose_view_control (Tool     *tool,
			     int       action,
			     gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      break;
    }
}

Tool *
tools_new_expose_view ()
{
  Tool * tool;
  ExposeImage * private;

  /*  The tool options  */
  if (!expose_view_options)
    expose_view_options = tools_register_no_options (EXPOSE_IMAGE,
							     "expose-view Options");

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (ExposeImage *) g_malloc_zero (sizeof (ExposeImage));

  tool->type = EXPOSE_IMAGE;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = expose_view_button_press;
  tool->button_release_func = expose_view_button_release;
  tool->motion_func = expose_view_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = expose_view_cursor_update;
  tool->control_func = expose_view_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_expose_view (Tool *tool)
{
  ExposeImage * bc;

  bc = (ExposeImage *) tool->private;

  /*  Close the color select dialog  */
  if (expose_view_dialog)
    expose_view_ok_callback (NULL, (gpointer) expose_view_dialog);

  g_free (bc);
}

void
expose_view_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  CanvasDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("expose-view does not operate on indexed drawables.");
      return;
    }

  /*  The expose-view dialog  */
  if (!expose_view_dialog)
    expose_view_dialog = expose_view_new_dialog (gdisp);
  else
    if (!GTK_WIDGET_VISIBLE (expose_view_dialog->shell))
      gtk_widget_show (expose_view_dialog->shell);

  drawable = gimage_active_drawable (gdisp->gimage);
  expose_view_dialog->gdisp = gdisp;
  
  expose_view_dialog->drawable = drawable;
  expose_view_update (expose_view_dialog, ALL);
}


/********************************/
/*  gamma expose dialog  */
/********************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", expose_view_ok_callback, NULL, NULL },
  { "Reset (LDR)", expose_view_reset_callback, NULL, NULL }
};

static ExposeImageDialog *
expose_view_new_dialog (GDisplay* gdisp)
{
  ExposeImageDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkObject *data;

  action_items[0].label = _("OK");
  action_items[1].label = _("Reset (LDR)");
  bcd = g_malloc_zero (sizeof (ExposeImageDialog));
  bcd->preview = TRUE;
  bcd->forceexpose = TRUE;
  bcd->gdisp = gdisp;

  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "expose_view", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (bcd->shell), _("Expose View"));
  
  /* handle wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (expose_view_delete_callback),
		      bcd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table containing sliders  */
  table = gtk_table_new (2,
# if USE_OFFSET
                            4,
#else
                            3,
#endif
                               FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the expose scale widget  */
  label = gtk_label_new (_("Expose"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (bcd->gdisp->expose, EXPOSE_MIN, EXPOSE_MAX, .001, .1, .001);
  bcd->expose_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) expose_view_expose_scale_update,
		      bcd);
  gtk_adjustment_set_value (GTK_ADJUSTMENT(data), gdisp->expose);

  bcd->expose_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->expose_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->expose_text, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->expose_text), "changed",
		      (GtkSignalFunc) expose_view_expose_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->expose_text);
  gtk_widget_show (slider);


  /*  Create the offset scale widget  */
# if USE_OFFSET
  label = gtk_label_new (_("offset"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (bcd->gdisp->offset, OFFSET_MIN, OFFSET_MAX, .01, .1, .01);
  bcd->offset_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) expose_view_offset_scale_update,
		      bcd);
  gtk_adjustment_set_value (GTK_ADJUSTMENT(data), gdisp->offset);

  bcd->offset_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->offset_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->offset_text, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->offset_text), "changed",
		      (GtkSignalFunc) expose_view_offset_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->offset_text);
  gtk_widget_show (slider);
#endif

  /*  Create the gamma scale widget  */
  label = gtk_label_new (_("Gamma"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
# if USE_OFFSET
                                                    2, 3,
#else
                                                    1, 2,
#endif
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (bcd->gdisp->gamma, GAMMA_MIN, GAMMA_MAX, .001, .01, .01);
  bcd->gamma_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2,
# if USE_OFFSET
                                                     2, 3,
#else
                                                     1, 2,
#endif
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) expose_view_gamma_scale_update,
		      bcd);

  bcd->gamma_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->gamma_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->gamma_text, 2, 3,
# if USE_OFFSET
                                                     2, 3,
#else
                                                     1, 2,
#endif
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->gamma_text), "changed",
		      (GtkSignalFunc) expose_view_gamma_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->gamma_text);
  gtk_widget_show (slider);


  /*  Horizontal box for preview and preserve luminosity toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The action area  */
  action_items[0].user_data = bcd;
  action_items[1].user_data = bcd;
  build_action_area (GTK_DIALOG (bcd->shell), action_items, 2, 1);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
expose_view_update (ExposeImageDialog *bcd,
			    int                       update)
{
  char text[12];

  if (update & GAMMA_SLIDER)
    {
      bcd->gamma_data->value = bcd->gdisp->gamma;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->gamma_data), "value_changed");
    }
  if (update & EXPOSE_SLIDER)
    {
      bcd->expose_data->value = bcd->gdisp->expose;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->expose_data), "value_changed");
    }
# if USE_OFFSET
  if (update & OFFSET_SLIDER)
    {
      bcd->offset_data->value = bcd->gdisp->offset;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->offset_data), "value_changed");
    }
  if (update & OFFSET_TEXT)
    {
      sprintf (text, "%0.3f", bcd->gdisp->offset);
      gtk_entry_set_text (GTK_ENTRY (bcd->offset_text), text);
    }
# endif
  if (update & GAMMA_TEXT)
    {
      sprintf (text, "%0.3f", bcd->gdisp->gamma);
      gtk_entry_set_text (GTK_ENTRY (bcd->gamma_text), text);
    }
  if (update & EXPOSE_TEXT)
    {
      sprintf (text, "%0.3f", bcd->gdisp->expose);
      gtk_entry_set_text (GTK_ENTRY (bcd->expose_text), text);
    }
  d_printf("expose_view_update %f %f \n", bcd->gdisp->gamma, bcd->gdisp->expose);
  drawable_update (expose_view_dialog->drawable, 0, 0, drawable_width (expose_view_dialog->drawable), drawable_height (expose_view_dialog->drawable));
}

static void
expose_view_preview (ExposeImageDialog *bcd)
{
  drawable_update (expose_view_dialog->drawable, 0, 0, drawable_width (expose_view_dialog->drawable), drawable_height (expose_view_dialog->drawable));
}

static void
expose_view_ok_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  ExposeImageDialog *bcd;

  bcd = (ExposeImageDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);
}

static gint
expose_view_delete_callback (GtkWidget *w,
				     GdkEvent *e,
				     gpointer d)
{
  expose_view_ok_callback (w, d);

  return TRUE;
}

static void
expose_view_reset_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  ExposeImageDialog *bcd;

  bcd = (ExposeImageDialog *) client_data;
  bcd->gamma_data->value = bcd->gdisp->gamma = GAMMA_DEFAULT;
  gtk_signal_emit_by_name (GTK_OBJECT (bcd->gamma_data), "value_changed");
  bcd->expose_data->value = bcd->gdisp->expose = EXPOSE_DEFAULT;
  gtk_signal_emit_by_name (GTK_OBJECT (bcd->expose_data), "value_changed");
# if USE_OFFSET
  bcd->offset_data->value = bcd->gdisp->offset = OFFSET_DEFAULT;
  gtk_signal_emit_by_name (GTK_OBJECT (bcd->offset_data), "value_changed");
# endif

  bcd->preview = TRUE;
  expose_view_preview (bcd);
  gdisplays_dirty ();
  gdisplays_flush ();
}


static void
expose_view_gamma_scale_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  ExposeImageDialog *bcd;

  bcd = (ExposeImageDialog *) data;

  if (bcd->gdisp->gamma != adjustment->value)
    {
      bcd->gdisp->gamma = adjustment->value;
      expose_view_update (bcd, GAMMA_TEXT);


	expose_view_preview (bcd);
    } 
  gdisplays_dirty ();
  gdisplays_flush ();
}

static void
expose_view_expose_scale_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  ExposeImageDialog *bcd;

  bcd = (ExposeImageDialog *) data;

  if (bcd->gdisp->expose != adjustment->value)
    {
      bcd->gdisp->expose = adjustment->value;
      expose_view_update (bcd, EXPOSE_TEXT);

	expose_view_preview (bcd);
    }
  gdisplays_dirty ();
  gdisplays_flush ();
}

static void
expose_view_offset_scale_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
# if USE_OFFSET
  ExposeImageDialog *bcd;

  bcd = (ExposeImageDialog *) data;

  if (bcd->gdisp->offset != adjustment->value)
    {
      bcd->gdisp->offset = adjustment->value;
      expose_view_update (bcd, OFFSET_TEXT);

	expose_view_preview (bcd);
    }
  gdisplays_dirty ();
  gdisplays_flush ();
# endif
}

static void
expose_view_gamma_text_update (GtkWidget *w,
					    gpointer   data)
{
  ExposeImageDialog *bcd = (ExposeImageDialog *) data;
  const char *str = gtk_entry_get_text (GTK_ENTRY (w));
  
  if (expose_view_gamma_text_check (str, bcd))
  {
      expose_view_update (bcd, GAMMA_SLIDER);

	expose_view_preview (bcd);
  }
}

static gint 
expose_view_gamma_text_check (
				const char *str, 
				ExposeImageDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), GAMMA_MIN, GAMMA_MAX);
  if (bcd->gdisp->gamma != value)
  {
      bcd->gdisp->gamma = value;
      return TRUE;
  }
  return FALSE;
}

static void
expose_view_expose_text_update (GtkWidget *w,
					  gpointer   data)
{
  ExposeImageDialog *bcd = (ExposeImageDialog *) data;
  const char *str = gtk_entry_get_text (GTK_ENTRY (w));

  if (expose_view_expose_text_check (str, bcd))
    {
      expose_view_update (bcd, EXPOSE_SLIDER);

      if (bcd->preview)
	expose_view_preview (bcd);
    }
}

static gint 
expose_view_expose_text_check  (
				const char *str, 
				ExposeImageDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), EXPOSE_MIN, EXPOSE_MAX);
  if (bcd->gdisp->expose != value)
  {
      bcd->gdisp->expose = value;
      return TRUE;
  }
  return FALSE;
}

static void
expose_view_offset_text_update (GtkWidget *w,
					  gpointer   data)
{
# if USE_OFFSET
  ExposeImageDialog *bcd = (ExposeImageDialog *) data;
  char *str = gtk_entry_get_text (GTK_ENTRY (w));

  if (expose_view_offset_text_check (str, bcd))
    {
      expose_view_update (bcd, OFFSET_SLIDER);

      if (bcd->preview)
	expose_view_preview (bcd);
    }
# endif
}

static gint 
expose_view_offset_text_check  (
				const char *str, 
				ExposeImageDialog *bcd
				)
{
# if USE_OFFSET
  gfloat value;

  value = BOUNDS (atof (str), OFFSET_MIN, OFFSET_MAX);
  if (bcd->gdisp->offset != value)
  {
      bcd->gdisp->offset = value;
      return TRUE;
  }
  return FALSE;
#else
  return TRUE;
# endif
}


/*  The expose_view procedure definition  */
ProcArg expose_view_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_FLOAT,
    "exposure",
    "exposure adjustment: (-20 <= exposure <= 20)"
  },
  { PDB_FLOAT,
    "offset",
    "offset adjustment: (0 <= gamma <= 12)"
  },
  { PDB_FLOAT,
    "gamma",
    "gamma adjustment: (0 <= gamma <= 8)"
  }
};

ProcRecord expose_view_proc =
{
  "gimp_expose_view",
  "Modify gamma/exposure in the specified display",
  "This procedures allows the gamma and exposure of the specified display to be modified.",
  "Alan Davidson",
  "Alan Davidson",
  "2001 + 2005",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  expose_view_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { expose_view_invoker } },
};


static Argument *
expose_view_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  GDisplay *gdisp;
  float expose;
  float offset;
  float gamma;
  float float_value;

  expose    = EXPOSE_DEFAULT;
  offset    = OFFSET_DEFAULT;
  gamma     = GAMMA_DEFAULT;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }
  if (success)
    {
      if(!(gdisp = gdisplay_get_from_gimage (gimage)))
        success = FALSE;
    }

  /*  expose  */
  if (success)
    {
      float_value = args[1].value.pdb_float;
      if (float_value < EXPOSE_MIN || float_value > EXPOSE_MAX)
        success = FALSE;
      else
        expose = float_value;
    }
  /*  offset  */
  if (success)
    {
      float_value = args[2].value.pdb_float;
      if (float_value < OFFSET_MIN || float_value > OFFSET_MAX)
        success = FALSE;
      else
        offset = float_value;
    }
  /*  gamma  */
  if (success)
    {
      float_value = args[3].value.pdb_float;
      if (float_value < GAMMA_MIN || float_value > GAMMA_MAX)
        success = FALSE;
      else
        gamma = float_value;
    }

  if (success)
    {
      gdisp->expose = expose;
      gdisp->offset = offset;
      gdisp->gamma = gamma;

      gdisplays_dirty ();
      gdisplays_flush ();
    }

  return procedural_db_return_args (&expose_view_proc, success);
}
