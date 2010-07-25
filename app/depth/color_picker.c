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
#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../actionarea.h"
#include "../canvas.h"
#include "../color_picker.h"
#include "../drawable.h"
#include "float16.h"
#include "bfp.h"
#include "../gdisplay.h"
#include "../info_dialog.h"
#include "../palette.h"
#include "../pixelrow.h"
#include "../tools.h"
#include "../paint_funcs_area.h"


/* maximum information buffer size */

#define MAX_INFO_BUF    32


/*  local function prototypes  */

static void  color_picker_button_press     (Tool *, GdkEventButton *, gpointer);
static void  color_picker_button_release   (Tool *, GdkEventButton *, gpointer);
static void  color_picker_motion           (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_cursor_update    (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_control          (Tool *, int, void *);
static void  color_picker_info_window_close_callback  (GtkWidget *, gpointer);

static int  get_color  (GImage *, CanvasDrawable *, int, int, int, int);
static void color_picker_info_update  (Tool *, int);

static Argument *color_picker_invoker (Argument *);

/*  local variables  */

static guchar         _color [TAG_MAX_BYTES];
static PixelRow       color;

static Format prev_format;  	/* For changing picker display when
				 * we change the format of the image
				 * Added 8Jul03 Tom Huffmn*/

static CanvasDrawable * active_drawable;
static int            update_type;
static InfoDialog *   color_picker_info = NULL;
static char           red_buf   [MAX_INFO_BUF];
static char           green_buf [MAX_INFO_BUF];
static char           blue_buf  [MAX_INFO_BUF];
static char           alpha_buf [MAX_INFO_BUF];
static char           index_buf [MAX_INFO_BUF];
static char           gray_buf  [MAX_INFO_BUF];
static char           hex_buf   [MAX_INFO_BUF];

typedef struct ColorPickerOptions ColorPickerOptions;
struct ColorPickerOptions
{
  int sample_merged;
};

static ColorPickerOptions * color_picker_options = NULL;


static void
color_picker_toggle_update (GtkWidget *w,
			    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static ColorPickerOptions *
create_color_picker_options (void)
{
  ColorPickerOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *sample_merged_toggle;

  /*  the new options structure  */
  options = (ColorPickerOptions *) g_malloc_zero (sizeof (ColorPickerOptions));
  options->sample_merged = 0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Color Picker Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the sample merged toggle button  */
  sample_merged_toggle = gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (sample_merged_toggle), options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), sample_merged_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (sample_merged_toggle), "toggled",
		      (GtkSignalFunc) color_picker_toggle_update,
		      &options->sample_merged);
  gtk_widget_show (sample_merged_toggle);


  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (COLOR_PICKER, vbox);

  return options;
}

static ActionAreaItem action_items[] =
{
  { "Close", color_picker_info_window_close_callback, NULL, NULL },
};

static void
color_picker_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  int x, y;
  const char** color_space_channel_names = 0;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  If this is the first invocation of the tool, or a different gdisplay,
   *  create (or recreate) the info dialog...
   */
  if (tool->state == INACTIVE || gdisp_ptr != tool->gdisp_ptr   ||
      active_drawable != gimage_active_drawable (gdisp->gimage) ||
      prev_format != gimage_format(gdisp->gimage))
    {
      /*  if the dialog exists, free it  */
      if (color_picker_info)
	info_dialog_free (color_picker_info);

      color_picker_info = info_dialog_new (_("Color Picker"),
					    gimp_standard_help_func,
					    _("UNKNOWN"));
      active_drawable = gimage_active_drawable (gdisp->gimage);

      color_space_channel_names = gimage_get_channel_names(gdisp->gimage);
      /*  if the gdisplay is for a color image, the dialog must have RGB  */
      switch (prev_format=tag_format (drawable_tag (active_drawable)))
	{
	case FORMAT_RGB:
	  info_dialog_add_field (color_picker_info, color_space_channel_names[RED_PIX], red_buf);
	  info_dialog_add_field (color_picker_info, color_space_channel_names[GREEN_PIX], green_buf);
	  info_dialog_add_field (color_picker_info, color_space_channel_names[BLUE_PIX], blue_buf);
	  info_dialog_add_field (color_picker_info, color_space_channel_names[ALPHA_PIX], alpha_buf);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf);
	  break;

	case FORMAT_INDEXED:
	  info_dialog_add_field (color_picker_info, _("Index"), index_buf);
	  info_dialog_add_field (color_picker_info, _("Alpha"), alpha_buf);
	  info_dialog_add_field (color_picker_info, _("Red"), red_buf);
	  info_dialog_add_field (color_picker_info, _("Green"), green_buf);
	  info_dialog_add_field (color_picker_info, _("Blue"), blue_buf);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf);
	  break;

	case FORMAT_GRAY:
	  info_dialog_add_field (color_picker_info, _("Intensity"), gray_buf);
	  info_dialog_add_field (color_picker_info, _("Alpha"), alpha_buf);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf);
	  break;

	default :
	  break;
	}

      /* Create the action area  */
      action_items[0].label = _(action_items[0].label);
      action_items[0].user_data = color_picker_info;
      build_action_area (GTK_DIALOG (color_picker_info->shell), action_items, 1, 0);
    }

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    (GDK_POINTER_MOTION_HINT_MASK |
		     GDK_BUTTON1_MOTION_MASK |
		     GDK_BUTTON_RELEASE_MASK),
		    NULL, NULL, bevent->time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp,CAST(int) bevent->x,CAST(int) bevent->y, &x, &y, FALSE, FALSE);

  /*  if the shift key is down, create a new color.
   *  otherwise, modify the current color.
   */
  if (bevent->state & GDK_SHIFT_MASK)
    {
      color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
						 color_picker_options->sample_merged,
						 COLOR_NEW));
      update_type = COLOR_UPDATE_NEW;
    }
  else
    {
      color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
						 color_picker_options->sample_merged,
						 COLOR_UPDATE));
      update_type = COLOR_UPDATE;
    }


}

static void
color_picker_button_release (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
  gdisp = (GDisplay *) gdisp_ptr;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp,CAST(int) bevent->x,CAST(int) bevent->y, &x, &y, FALSE, FALSE);

  color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
					     color_picker_options->sample_merged,
					     update_type));
}

static void
color_picker_motion (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);

  color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
					     color_picker_options->sample_merged,
					     update_type));
}

static void
color_picker_cursor_update (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);
  if (gimage_pick_correlate_layer (gdisp->gimage, x, y))
    gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
color_picker_control (Tool     *tool,
		      int       action,
		      gpointer  gdisp_ptr)
{
}

static int 
get_color  (
            GImage * gimage,
            CanvasDrawable * drawable,
            int x,
            int y,
            int sample_merged,
            int final
            )
{
  PixelRow pixel;
  Canvas *tiles;
  int width, height;

  if (!drawable && !sample_merged) 
    return FALSE;

  if (! sample_merged)
    {
      int offx, offy;
      drawable_offsets (drawable, &offx, &offy);
      x -= offx;
      y -= offy;
      width = drawable_width (drawable);
      height = drawable_height (drawable);
      tiles = drawable_data (drawable);
      pixelrow_init (&color, drawable_tag (drawable), _color, 1);
    }
  else
    {
      width = gimage->width;
      height = gimage->height;
      tiles = gimage_projection (gimage);
      pixelrow_init (&color, gimage_tag (gimage), _color, 1);
    }

  if (x < 0 || y < 0 || x >= width || y >= height)
    return FALSE;

  canvas_portion_refro (tiles, x, y);

  pixelrow_init (&pixel, canvas_tag (tiles), canvas_portion_data (tiles, x, y), 1);

  copy_row (&pixel, &color);
  
  canvas_portion_unref (tiles, x, y);

  palette_set_active_color (&color, final);

  return TRUE;
}


static void 
color_picker_info_update_u8  (
                              void
                              )
{
  guint8 * src = pixelrow_data (&color);
  Tag sample_tag = pixelrow_tag (&color);
  
  switch (tag_format (sample_tag))
    {
    case FORMAT_RGB:
      sprintf (red_buf, "%d", src [0]);
      sprintf (green_buf, "%d", src [1]);
      sprintf (blue_buf, "%d", src [2]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [3]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, "#%.2x%.2x%.2x",
               src [0], src [1], src [2]);
      break;

    case FORMAT_INDEXED:
      sprintf (index_buf, "%d", src [0]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [1]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_GRAY:
      sprintf (gray_buf, "%d", src [0]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [1]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, "#%.2x%.2x%.2x", src [0],
               src [0], src [0]);
      break;
          
    case FORMAT_NONE:
    default:
      g_warning (_("bad format"));
      break;
    }
}

static void 
color_picker_info_update_u16  (
                               void
                               )
{
  guint16 * src = (guint16*) pixelrow_data (&color);
  Tag sample_tag = pixelrow_tag (&color);
  
  switch (tag_format (sample_tag))
    {
    case FORMAT_RGB:
      sprintf (red_buf, "%d", src [0]);
      sprintf (green_buf, "%d", src [1]);
      sprintf (blue_buf, "%d", src [2]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [3]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, "#%.4x %.4x %.4x",
               src [0], src [1], src [2]);
      break;

    case FORMAT_INDEXED:
      sprintf (index_buf, "%d", src [0]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [1]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_GRAY:
      sprintf (gray_buf, "%d", src [0]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%d", src [1]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, "#%.4x %.4x %.4x", src [0],
               src [0], src [0]);
      break;
          
    case FORMAT_NONE:
    default:
      g_warning (_("bad format"));
      break;
    }
}

static void 
color_picker_info_update_float  (
                                 void
                                 )
{
  gfloat * src = (gfloat*) pixelrow_data (&color);
  Tag sample_tag = pixelrow_tag (&color);
  
  switch (tag_format (sample_tag))
    {
    case FORMAT_RGB:
      sprintf (red_buf, "%7.6f", src [0]);
      sprintf (green_buf, "%7.6f", src [1]);
      sprintf (blue_buf, "%7.6f", src [2]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", src [3]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_INDEXED:
      sprintf (index_buf, _("N/A"));
      sprintf (alpha_buf, _("N/A"));
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_GRAY:
      sprintf (gray_buf, "%7.6f", src [0]);
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", src [1]);
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;
          
    case FORMAT_NONE:
    default:
      g_warning (_("bad format"));
      break;
    }
}

static void 
color_picker_info_update_float16  (
                                 void
                                 )
{
  guint16 * src = (guint16*) pixelrow_data (&color);
  Tag sample_tag = pixelrow_tag (&color);
  ShortsFloat u;
  
  switch (tag_format (sample_tag))
    {
    case FORMAT_RGB:
      sprintf (red_buf, "%7.6f", FLT (src [0], u));
      sprintf (green_buf, "%7.6f", FLT (src [1], u));
      sprintf (blue_buf, "%7.6f", FLT (src [2], u));
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", FLT (src [3], u));
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_INDEXED:
      sprintf (index_buf, _("N/A"));
      sprintf (alpha_buf, _("N/A"));
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_GRAY:
      sprintf (gray_buf, "%7.6f", FLT (src [0], u));
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", FLT (src [1], u));
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;
          
    case FORMAT_NONE:
    default:
      g_warning (_("bad format"));
      break;
    }
}


static void 
color_picker_info_update_bfp  (
                               void
                               )
{
  guint16 * src = (guint16*) pixelrow_data (&color);
  Tag sample_tag = pixelrow_tag (&color);
  
  switch (tag_format (sample_tag))
    {
    case FORMAT_RGB:
      sprintf (red_buf, "%7.6f", BFP_TO_FLOAT (src [0]));
      sprintf (green_buf, "%7.6f", BFP_TO_FLOAT (src [1]));
      sprintf (blue_buf, "%7.6f", BFP_TO_FLOAT (src [2]));
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", BFP_TO_FLOAT (src [3]));
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_INDEXED:
      sprintf (index_buf, _("N/A"));
      sprintf (alpha_buf, _("N/A"));
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;

    case FORMAT_GRAY:
      sprintf (gray_buf, "%7.6f", BFP_TO_FLOAT (src [0]));
      if (tag_alpha (sample_tag) == ALPHA_YES)
        sprintf (alpha_buf, "%7.6f", BFP_TO_FLOAT (src [1]));
      else
        sprintf (alpha_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
      break;
          
    case FORMAT_NONE:
    default:
      g_warning (_("bad format"));
      break;
    }
}


static void 
color_picker_info_update  (
                           Tool * tool,
                           int valid
                           )
{
  if (!valid)
    {
      sprintf (red_buf, _("N/A"));
      sprintf (green_buf, _("N/A"));
      sprintf (blue_buf, _("N/A"));
      sprintf (alpha_buf, _("N/A"));
      sprintf (index_buf, _("N/A"));
      sprintf (gray_buf, _("N/A"));
      sprintf (hex_buf, _("N/A"));
    }
  else
    {
      switch (tag_precision (pixelrow_tag (&color)))
	{
	case PRECISION_U8:
	  color_picker_info_update_u8 ();
          break;

	case PRECISION_U16:
	  color_picker_info_update_u16 ();
          break;

	case PRECISION_FLOAT:
	  color_picker_info_update_float ();
          break;

	case PRECISION_FLOAT16:
	  color_picker_info_update_float16 ();
          break;

	case PRECISION_BFP:
	  color_picker_info_update_bfp ();
          break;
	
	case PRECISION_NONE:
        default:
          g_warning (_("bad precision"));
          break;
	}
    }

  info_dialog_update (color_picker_info);
  info_dialog_popup (color_picker_info);
}


Tool *
tools_new_color_picker ()
{
  Tool * tool;

  if (! color_picker_options)
    color_picker_options = create_color_picker_options ();

  tool = (Tool *) g_malloc_zero (sizeof (Tool));

  tool->type = COLOR_PICKER;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = NULL;
  tool->button_press_func = color_picker_button_press;
  tool->button_release_func = color_picker_button_release;
  tool->motion_func = color_picker_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = color_picker_cursor_update;
  tool->control_func = color_picker_control;
  tool->preserve = TRUE;
  return tool;
}

void
tools_free_color_picker (Tool *tool)
{
  if (color_picker_info)
    {
      info_dialog_free (color_picker_info);
      color_picker_info = NULL;
    }
}

/*  The color_picker procedure definition  */
ProcArg color_picker_args[] =
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
    "x",
    "x coordinate of upper-left corner of rectangle"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of upper-left corner of rectangle"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  },
  { PDB_INT32,
    "save_color",
    "save the color to the active palette"
  }
};

ProcArg color_picker_out_args[] =
{
  { PDB_COLOR,
    "color",
    "the return color"
  }
};

ProcRecord color_picker_proc =
{
  "gimp_color_picker",
  "Determine the color at the given drawable coordinates",
  "This tool determines the color at the specified coordinates.  The returned color is an RGB triplet even for grayscale and indexed drawables.  If the coordinates lie outside of the extents of the specified drawable, then an error is returned.  If the drawable has an alpha channel, the algorithm examines the alpha value of the drawable at the coordinates.  If the alpha value is completely transparent (0), then an error is returned.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  color_picker_args,

  /*  Output arguments  */
  1,
  color_picker_out_args,

  /*  Exec method  */
  { { color_picker_invoker } },
};


static Argument *
color_picker_invoker (Argument *args)
{
  GImage *gimage;
  int success = TRUE;
  CanvasDrawable *drawable;
  double x, y;
  int sample_merged;
  int save_color;
  int int_value;
  Argument *return_args;

  drawable = NULL;
  x             = 0;
  y             = 0;
  sample_merged = FALSE;
  save_color    = COLOR_UPDATE;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
    }
  /*  x, y  */
  if (success)
    {
      x = args[2].value.pdb_float;
      y = args[3].value.pdb_float;
    }
  /*  sample_merged  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }
  /*  save_color  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      save_color = (int_value) ? COLOR_NEW : COLOR_UPDATE;
    }

  /*  Make sure that if we're not using the composite, the specified drawable is valid  */
  if (success && !sample_merged)
    if (!drawable || (drawable_gimage (drawable)) != gimage)
      success = FALSE;

  /*  call the color_picker procedure  */
  if (success)
    success = get_color (gimage, drawable, (int) x, (int) y, sample_merged, save_color);

  return_args = procedural_db_return_args (&color_picker_proc, success);

  if (success)
    {
      PixelRow c;
      guchar * _c = (unsigned char *) g_malloc (3);
      pixelrow_init (&c, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), _c, 1);
      copy_row (&color, &c);
      return_args[1].value.pdb_pointer = _c;
    }

  return return_args;
}

static void
color_picker_info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}
