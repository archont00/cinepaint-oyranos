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
#include "compat.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "actionarea.h"
#include "app_procs.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
#include "commands.h"
#include "devices.h"
#include "disp_callbacks.h"
#include "dnd.h"
#include "errors.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimage.h"
#include "general.h"
#include "interface.h"
#include "menus.h"
#include "tools.h"
#include "devices.h"
#include "layout.h"
#include "rc.h"
#include "pixmaps.h"
#include "minimize.h"
#include "tango_tools_pixmaps.h"

/*  local functions  */
static gint  tools_button_press    (GtkWidget *widget,
				    GdkEventButton *bevent,
				    gpointer   data);
static void  gdisplay_destroy      (GtkWidget *widget,
				    GDisplay  *display);

static gint  gdisplay_delete       (GtkWidget *widget,
				    GdkEvent *,
				    GDisplay  *display);

static void  toolbox_destroy       (void);
static gint  toolbox_delete        (GtkWidget *,
				    GdkEvent *,
				    gpointer);
static gint  toolbox_check_device  (GtkWidget *,
				    GdkEvent *,
				    gpointer);

static GdkPixmap *create_pixmap    (GdkWindow  *parent,
				    GdkBitmap **mask,
				    char      **data,
				    int         width,
				    int         height);

static void  tools_select_update   (GtkWidget *widget,
				    gpointer   data);

/* DnD */
static void     toolbox_set_drag_dest      (GtkWidget        *widget);
static gboolean toolbox_drag_drop          (GtkWidget        *widget,
                        GdkDragContext   *context,
                        gint              x,
                        gint              y,
                        guint             time);
static ToolType toolbox_drag_tool          (GtkWidget        *widget,
                        gpointer          data);
static void     toolbox_drop_tool          (GtkWidget        *widget,
                        ToolType          tool,
                        gpointer          data);


typedef struct ToolButton ToolButton;

struct ToolButton
{
  char **icon_data;
  char **tango_icon_data;
  char  *tool_desc;
  gpointer callback_data;
};

/*
  0   RECT_SELECT
  1   ELLIPSE_SELECT
  2   FREE_SELECT
  3   FUZZY_SELECT
  4   BEZIER_SELECT
  5   ISCISSORS,
  6   MOVE
  7   MAGNIFY
  8   CROP
  9   TRANSFORM
  9   FLIP_HORZ
  10  COLOR_PICKER
  11  BUCKET_FILL
  12  BLEND
  13  PENCIL
  14   PAINTBRUSH
  15   ERASER
  16   AIRBRUSH
  17   CLONE
  18   CONVOLVE
  19   DODGEBURN
  20   SMUDGE
  21   COLOR_BALANCE
  22   BRIGHTNESS_CONTRAST
  23   HUE_SATURATION
  24   POSTERIZE
  25   THRESHOLD
  26   CURVES
  27   LEVELS
  28   HISTOGRAM
  29   GAMMA_EXPOSE
*/

static ToolButton tool_data[] =
{
  { (char **) rect_bits,
    (char **) stock_tool_rect_select_22,
    "Select rectangular regions",
    (gpointer) RECT_SELECT },
  { (char **) circ_bits,
    (char **) stock_tool_ellipse_select_22,
    "Select elliptical regions",
    (gpointer) ELLIPSE_SELECT },
  { (char **) free_bits,
    (char **) stock_tool_free_select_22,
    "Select hand-drawn regions",
    (gpointer) FREE_SELECT },

  { (char **) fuzzy_bits,
    (char **) stock_tool_fuzzy_select_22,
    "Select contiguous regions",
    (gpointer) FUZZY_SELECT },
  { (char **) bezier_bits,
    (char **) stock_tool_path_22,
    "Select regions using Bezier curves",
    (gpointer) BEZIER_SELECT },
  { (char **) iscissors_bits,
    (char **) stock_tool_iscissors_22,
    "Select shapes from image",
    (gpointer) ISCISSORS },

  { (char **) move_bits,
    (char **) stock_tool_move_22,
    "Move layers & selections",
    (gpointer) MOVE },
  { (char **) magnify_bits,
    (char **) stock_tool_zoom_22,
    "Zoom in & out",
    (gpointer) MAGNIFY },
  { (char **) crop_bits,
    (char **) stock_tool_crop_22,
    "Crop the image",
    (gpointer) CROP },

  { (char **) scale_bits,
    (char **) stock_tool_scale_22,
    "Transform the layer or selection (rotate, scale, share, perspective)",
    (gpointer) ROTATE },
  { (char **) horizflip_bits,
    (char **) stock_tool_flip_22,
    "Flip the layer or selection",
    (gpointer) FLIP_HORZ },
  { (char **) text_bits,
    (char **) stock_tool_text_22,
    "Add text to the image",
    (gpointer) TEXT },
  { (char **) spline_bits,
    (char **) stock_tool_spline_22,
    "Spline",
    (gpointer) SPLINE },

  { (char **) colorpicker_bits,
    (char **) stock_tool_color_picker_22,
    "Pick colors from the image",
    (gpointer) COLOR_PICKER },
  { (char **) fill_bits,
    (char **) stock_tool_bucket_fill_22,
    "Fill with a color or pattern",
    (gpointer) BUCKET_FILL },
  { (char **) gradient_bits,
    (char **) stock_tool_blend_22,
    "Fill with a color gradient",
    (gpointer) BLEND },

  { (char **) pencil_bits,
    (char **) stock_tool_pencil_22,
    "Draw sharp pencil strokes",
    (gpointer) PENCIL },
  { (char **) paint_bits,
    (char **) stock_tool_paintbrush_22,
    "Paint fuzzy brush strokes",
    (gpointer) PAINTBRUSH },
  { (char **) erase_bits,
    (char **) stock_tool_eraser_22,
    "Erase to background or transparency",
    (gpointer) ERASER },

  { (char **) airbrush_bits,
    (char **) stock_tool_airbrush_22,
    "Airbrush with variable pressure",
    (gpointer) AIRBRUSH },
  { (char **) clone_bits,
    (char **) stock_tool_clone_22,
    "Clone image regions",
    (gpointer) CLONE },
  { (char **) blur_bits,
    (char **) stock_tool_blur_22,
    "Blur or sharpen",
    (gpointer) CONVOLVE },

  { (char **) dodge_bits,
    (char **) stock_tool_dodge_22,
    "Dodge or burn",
    (gpointer) DODGEBURN },
  { (char **) smudge_bits,
    (char **) stock_tool_smudge_22,
    "Smudge",
    (gpointer) SMUDGE },
  { (char **) measure_bits,
    (char **) stock_tool_measure_22,
    "Measure",
    (gpointer) MEASURE },
  { (char **) spline_edit_bits,
    (char **) stock_tool_edit_spline_22,
    "Spline Edit",
    (gpointer) SPLINE_EDIT },
};

/* non-visible tool buttons */
static ToolButton tool_nonvis_data[] =
{
  { NULL,
    NULL,
    (gpointer) BY_COLOR_SELECT },
  { NULL,
    NULL,
    (gpointer) COLOR_BALANCE },
  { NULL,
    NULL,
    (gpointer) BRIGHTNESS_CONTRAST },
  { NULL,
    NULL,
    (gpointer) HUE_SATURATION },
  { NULL,
    NULL,
    (gpointer) POSTERIZE },
  { NULL,
    NULL,
    (gpointer) THRESHOLD },
  { NULL,
    NULL,
    (gpointer) CURVES },
  { NULL,
    NULL,
    (gpointer) LEVELS },
  { NULL,
    NULL,
    (gpointer) HISTOGRAM },
  { NULL,
    NULL,
    (gpointer) GAMMA_EXPOSE }
};

  GSList *toolbox_table_group=NULL;

static int pixmap_colors[8][3] =
{
  { 0x00, 0x00, 0x00 }, /* a - 0   */
  { 0x24, 0x24, 0x24 }, /* b - 36  */
  { 0x49, 0x49, 0x49 }, /* c - 73  */
  { 0x6D, 0x6D, 0x6D }, /* d - 109 */
  { 0x92, 0x92, 0x92 }, /* e - 146 */
  { 0xB6, 0xB6, 0xB6 }, /* f - 182 */
  { 0xDB, 0xDB, 0xDB }, /* g - 219 */
  { 0xFF, 0xFF, 0xFF }, /* h - 255 */
#define NUM_TOOLS_NONVIS (sizeof (tool_nonvis_data) / sizeof (tool_nonvis_data[0]))
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (tool_data[0]))
#ifdef VERTICAL_TOOLBAR
#define COLUMNS   3
#define ROWS      7 
#else
#define COLUMNS   13
#define ROWS      2 
#endif
#define MARGIN    2

gint num_tools = NUM_TOOLS;//sizeof (tool_info) / sizeof (tool_info[0]);

/*  Widgets for each tool button--these are used from command.c to activate on
 *  tool selection via both menus and keyboard accelerators.
 */
GtkWidget *tool_widgets[NUM_TOOLS+NUM_TOOLS_NONVIS];
GtkWidget *tool_label;
GtkTooltips *tool_tips;

/*  The popup shell is a pointer to the gdisplay shell that posted the latest
 *  popup menu.  When this is null, and a command is invoked, then the
 *  assumption is that the command was a result of a keyboard accelerator
 */
GtkWidget *popup_shell = NULL;


static GtkWidget *tool_label_area = NULL;
static GtkWidget *progress_area = NULL;
static GdkColor colors[12];
GtkWidget *toolbox_shell = NULL;

//static GtkWidget * toolbox_shell = NULL;

static GtkTargetEntry toolbox_target_table[] =
{
  GIMP_TARGET_URI_LIST,
  GIMP_TARGET_TEXT_PLAIN,
  GIMP_TARGET_NETSCAPE_URL,
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_TOOL
};
static guint toolbox_n_targets = (sizeof (toolbox_target_table) /
                  sizeof (toolbox_target_table[0]));

static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint tool_n_targets = (sizeof (tool_target_table) /
                   sizeof (tool_target_table[0]));


static void
tools_select_update (GtkWidget *w,
		     gpointer   data)
{
  ToolType tool_type;

  tool_type = (ToolType) data;

  if ((tool_type != -1) && GTK_TOGGLE_BUTTON (w)->active)
    tools_select (tool_type);
}

static gint 
tools_button_press (GtkWidget *w,GdkEventButton *event,gpointer data)
{	
  if(((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
     || ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) )
    {
      tools_options_dialog_show();
    }
  return FALSE;
}

static gint
toolbox_delete (GtkWidget *w, GdkEvent *e, gpointer data)
{
  app_exit (0);

  return TRUE;
}

static void
toolbox_destroy ()
{
  app_exit_finish ();
}

static gint
toolbox_check_device   (GtkWidget *w, GdkEvent *e, gpointer data)
{
  devices_check_change (e);
  
  return FALSE;
}

static void
gdisplay_destroy (GtkWidget *w,
		  GDisplay  *gdisp)
{
  gdisplay_remove_and_delete (gdisp);
}

static gint
gdisplay_delete (GtkWidget *w,
		 GdkEvent *e,
		 GDisplay *gdisp)
{
  gdisplay_close_window (gdisp, FALSE);

  return TRUE;
}

static void
allocate_colors (GtkWidget *parent)
{
  GdkColormap *colormap;
  int i;

  gtk_widget_realize (parent);
  colormap = gdk_window_get_colormap (parent->window);

  for (i = 0; i < 8; i++)
    {
      colors[i].red = pixmap_colors[i][0] << 8;
      colors[i].green = pixmap_colors[i][1] << 8;
      colors[i].blue = pixmap_colors[i][2] << 8;

      gdk_color_alloc (colormap, &colors[i]);
    }

  colors[8] = parent->style->bg[GTK_STATE_NORMAL];
  gdk_color_alloc (colormap, &colors[8]);

  colors[9] = parent->style->bg[GTK_STATE_ACTIVE];
  gdk_color_alloc (colormap, &colors[9]);

  colors[10] = parent->style->bg[GTK_STATE_PRELIGHT];
  gdk_color_alloc (colormap, &colors[10]);

  /* postit yellow (khaki) as background for tooltips */
  colors[11].red = 61669;
  colors[11].green = 59113;
  colors[11].blue = 35979;
  gdk_color_alloc (colormap, &colors[11]);
}


static void
create_color_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GdkPixmap *default_pixmap;
  GdkPixmap *swap_pixmap;

  gtk_widget_realize (parent);

  default_pixmap = create_pixmap (parent->window, NULL, default_bits,
				  default_width, default_height);
  swap_pixmap    = create_pixmap (parent->window, NULL, swap_bits,
				  swap_width, swap_height);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_realize (frame);
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  col_area = color_area_create (54, 42, default_pixmap, swap_pixmap);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gtk_tooltips_set_tip (tool_tips, col_area, _("Foreground & background colors.  The small black "
			"and white squares reset colors.  The small arrows swap colors.  Double "
			"click to change colors."),
			NULL);
  gtk_widget_show (col_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
}


GdkPixmap *  
create_tool_pixmap (GtkWidget *parent, ToolType type)
{
  int i;

  if (type == SCALE || type == SHEAR || type == PERSPECTIVE)
    type = ROTATE;
  else if (type == FLIP_VERT)
    type = FLIP_HORZ;

  for (i=0; i<21; i++)
    {
      if ((ToolType)tool_data[i].callback_data == type)
	  return create_pixmap (parent->window, NULL,
				tool_data[i].icon_data, 22, 22);
    }
  
  g_return_val_if_fail (FALSE, NULL);
  
  return NULL;	/* not reached */
}


static void
create_tools (GtkWidget *parent)
{
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *alignment;
  GtkWidget *pixmap;
#if GTK_MAJOR_VERSION > 1
  GdkPixbuf *pixbuf;
#endif
  GSList *group;
  gint i;

  /*create_logo (parent);*/
  table = gtk_table_new (ROWS, COLUMNS, TRUE);
  gtk_box_pack_start (GTK_BOX (parent), table, TRUE, TRUE, 0);
  gtk_widget_realize (table);

  for (i = 0; i < NUM_TOOLS; i++)
    {
      tool_widgets[i] = button = gtk_radio_button_new (toolbox_table_group);
      gtk_container_border_width (GTK_CONTAINER (button), 0);
      toolbox_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      gtk_table_attach (GTK_TABLE (table), button,
			(i % COLUMNS), (i % COLUMNS) + 1,
			(i / COLUMNS), (i / COLUMNS) + 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			0, 0);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_border_width (GTK_CONTAINER (alignment), 0);
      gtk_container_add (GTK_CONTAINER (button), alignment);

#if GTK_MAJOR_VERSION > 1
      pixbuf = gdk_pixbuf_new_from_inline (-1, tool_data[i].tango_icon_data, FALSE, NULL);
      pixmap = gtk_image_new_from_pixbuf(pixbuf);
#else
      pixmap = create_pixmap_widget (table->window, tool_data[i].icon_data, 22, 22);
#endif
 
      gtk_container_add (GTK_CONTAINER (alignment), pixmap);

      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) tools_select_update,
			  tool_data[i].callback_data);

      gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			  (GtkSignalFunc) tools_button_press,
			  tool_data[i].callback_data);

      gtk_tooltips_set_tip (tool_tips, button, tool_data[i].tool_desc, NULL);

      gtk_widget_show (pixmap);
      gtk_widget_show (alignment);
      gtk_widget_show (button);
    }
    /*  The non-visible tool buttons  */
    for (; i < num_tools; i++)
    {
      tool_widgets[i] = button = gtk_radio_button_new (toolbox_table_group);
      toolbox_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

      /*  dnd stuff  */
      gtk_drag_source_set (tool_widgets[i],
                   GDK_BUTTON2_MASK,
                   tool_target_table, tool_n_targets,
                   GDK_ACTION_COPY);
      gimp_dnd_tool_source_set (tool_widgets[i],
                    toolbox_drag_tool,
                    GINT_TO_POINTER (i));

      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) tools_select_update,
			  tool_nonvis_data[i].callback_data);
    }

  gtk_widget_show (table);
}


static GdkPixmap *
create_pixmap (GdkWindow *parent, GdkBitmap **mask,
	       char **data, int width, int height)
{
  GdkPixmap *pixmap;
  GdkImage *image;
  GdkGC *gc;
  GdkVisual *visual;
  GdkColormap *cmap;
  gint r, s, t, cnt;
  guchar *mem;
  guchar value;
  guint32 pixel;

  visual = gdk_window_get_visual (parent);
  cmap = gdk_window_get_colormap (parent);
  image = gdk_image_new (GDK_IMAGE_NORMAL, visual, width, height);
  pixmap = gdk_pixmap_new (parent, width, height, -1);
  gc = NULL;

  if (mask)
    {
      GdkColor tmp_color;

      *mask = gdk_pixmap_new (parent, width, height, 1);
      gc = gdk_gc_new (*mask);
      gdk_draw_rectangle (*mask, gc, TRUE, 0, 0, -1, -1);

      tmp_color.pixel = 1;
      gdk_gc_set_foreground (gc, &tmp_color);
    }

  for (r = 0; r < height; r++)
    {
      mem = image->mem;
      mem += image->bpl * r;

      for (s = 0, cnt = 0; s < width; s++)
	{
	  value = data[r][s];

	  if (value == '.')
	    {
	      pixel = colors[8].pixel;

	      if (mask)
		{
		  if (cnt < s)
		    gdk_draw_line (*mask, gc, cnt, r, s - 1, r);
		  cnt = s + 1;
		}
	    }
	  else
	    {
	      pixel = colors[value - 'a'].pixel;
	    }

	  if (image->byte_order == GDK_LSB_FIRST)
	    {
	      for (t = 0; t < image->bpp; t++)
		*mem++ = (unsigned char) ((pixel >> (t * 8)) & 0xFF);
	    }
	  else
	    {
	      for (t = 0; t < image->bpp; t++)
		*mem++ = (unsigned char) ((pixel >> ((image->bpp - t - 1) * 8)) & 0xFF);
	    }
	}

      if (mask && (cnt < s))
	gdk_draw_line (*mask, gc, cnt, r, s - 1, r);
    }

  if (mask)
    gdk_gc_destroy (gc);

  gc = gdk_gc_new (parent);
  gdk_draw_image (pixmap, gc, image, 0, 0, 0, 0, width, height);
  gdk_gc_destroy (gc);
  gdk_image_destroy (image);

  return pixmap;
}

GtkWidget*
create_pixmap_widget (GdkWindow *parent,
		      char **data, int width, int height)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  pixmap = create_pixmap (parent, &mask, data, width, height);

  return gtk_pixmap_new (pixmap, mask);
}

void
create_toolbox ()
{
  GtkWidget *window;
  GtkWidget *main_box;
  GtkWidget *box;
  GtkWidget *menubar;
  GList     *tmp_list;
  GtkAccelGroup *table;
  int i = 0;

  tool_data[i].tool_desc = _("Select rectangular regions"); ++i;
  tool_data[i].tool_desc = _("Select elliptical regions"); ++i;
  tool_data[i].tool_desc = _("Select hand-drawn regions"); ++i;

  tool_data[i].tool_desc = _("Select contiguous regions"); ++i;
  tool_data[i].tool_desc = _("Select regions using Bezier curves"); ++i;
  tool_data[i].tool_desc = _("Select shapes from image"); ++i;

  tool_data[i].tool_desc = _("Move layers & selections"); ++i;
  tool_data[i].tool_desc = _("Zoom in & out"); ++i;
  tool_data[i].tool_desc = _("Crop the image"); ++i;

  tool_data[i].tool_desc = _("Transform the layer or selection (rotate, scale, share, perspective)"); ++i;
  tool_data[i].tool_desc = _("Flip the layer or selection"); ++i;
  tool_data[i].tool_desc = _("Add text to the image"); ++i;
  tool_data[i].tool_desc = _("Spline"); ++i;

  tool_data[i].tool_desc = _("Pick colors from the image"); ++i;
  tool_data[i].tool_desc = _("Fill with a color or pattern"); ++i;
  tool_data[i].tool_desc = _("Fill with a color gradient"); ++i;

  tool_data[i].tool_desc = _("Draw sharp pencil strokes"); ++i;
  tool_data[i].tool_desc = _("Paint fuzzy brush strokes"); ++i;
  tool_data[i].tool_desc = _("Erase to background or transparency"); ++i;

  tool_data[i].tool_desc = _("Airbrush with variable pressure"); ++i;
  tool_data[i].tool_desc = _("Clone image regions"); ++i;
  tool_data[i].tool_desc = _("Blur or sharpen"); ++i;

  tool_data[i].tool_desc = _("Dodge or burn"); ++i;
  tool_data[i].tool_desc = _("Smudge"); ++i;
  tool_data[i].tool_desc = _("Measure"); ++i;
  tool_data[i].tool_desc = _("Spline Edit"); ++i;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (window),/*"CP"*/ PROGRAM_NAME " " PROGRAM_VERSION);
  gtk_widget_set_uposition (window, toolbox_x, toolbox_y);
  layout_connect_window_position(window, &toolbox_x, &toolbox_y, TRUE);
  minimize_track_toolbox(window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc) toolbox_destroy,
		      NULL);

  /* We need to know when the current device changes, so we can update
   * the correct tool - to do this we connect to motion events.
   * We can't just use EXTENSION_EVENTS_CURSOR though, since that
   * would get us extension events for the mouse pointer, and our
   * device would change to that and not change back. So we check
   * manually that all devices have a cursor, before establishing the check.
   */
#if GTK_MAJOR_VERSION < 2
#else
  tmp_list = gdk_devices_list ();
#endif
#if GTK_MAJOR_VERSION < 2
  tmp_list = gdk_input_list_devices ();
  while (tmp_list)
    {
      if (!((GdkDeviceInfo *)(tmp_list->data))->has_cursor)
	break;
      tmp_list = tmp_list->next;
    }

  if (!tmp_list)		/* all devices have cursor */
    {
      gtk_signal_connect (GTK_OBJECT (window), "motion_notify_event",
			  GTK_SIGNAL_FUNC (toolbox_check_device), NULL);
      
      gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (window, GDK_EXTENSION_EVENTS_CURSOR);
    }
#else
      //if (!((GdkDevice *)(tmp_list->data))->has_cursor)
	//break;
#endif
#ifndef VERTICAL_TOOLBAR
  main_box = gtk_vbox_new (FALSE, 1);
#else
  main_box = gtk_hbox_new (FALSE, 1);
#endif
  gtk_container_border_width (GTK_CONTAINER (main_box), 1);
  gtk_container_add (GTK_CONTAINER (window), main_box);
  gtk_widget_show (main_box);

  /*  allocate the colors for creating pixmaps  */
  allocate_colors (main_box);

  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();
#ifdef WIN32
	/* BUG: rsr suspects this is deprecated for Linux, too. */
#else
#if GTK_MAJOR_VERSION < 2
  gtk_tooltips_set_colors (tool_tips,
			   &colors[11],
			   &main_box->style->fg[GTK_STATE_NORMAL]);
#endif
#endif
  if (!show_tool_tips)
    gtk_tooltips_disable (tool_tips);

  /*  Build the menu bar with menus  */
  menus_get_toolbox_menubar (&menubar, &table);
  gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);

  /*  Install the accelerator table in the main window  */
  gtk_window_add_accel_group (GTK_WINDOW (window), table);
#ifdef VERTICAL_TOOLBAR
  box = gtk_vbox_new (FALSE, 1);
#else
  box = gtk_hbox_new (FALSE, 1);
#endif
  gtk_box_pack_start (GTK_BOX (main_box), box, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (box), 0);
  gtk_widget_show (box);

  create_tools (box);
  /*create_tool_label (box);*/
  /*create_progress_area (box);*/
  create_color_area (box);

  gtk_widget_show (window);
  toolbox_set_drag_dest (window);

  toolbox_shell = window;
}

void
toolbox_free ()
{
  int i;

  gtk_widget_destroy (toolbox_shell);
#if 0
  for (i = 21; i < NUM_TOOLS; i++)
    {
      gtk_object_sink    (GTK_OBJECT (tool_widgets[i]));
    }
#endif
  gtk_object_destroy (GTK_OBJECT (tool_tips));
  gtk_object_unref   (GTK_OBJECT (tool_tips));
}

void
toolbox_raise_callback (GtkWidget *widget,
			gpointer  client_data)
{
  gdk_window_raise(toolbox_shell->window);
}


void
create_display_shell (int   gdisp_unique_id,
		      int   width,
		      int   height,
		      char *title,
		      int   type)
{
  static GtkWidget *image_popup_menu = NULL;
  static GtkAccelGroup *image_accel_group = NULL;
  GDisplay *gdisp;
  GtkWidget *table;
  GtkWidget *vbox;
  int n_width, n_height;
  int s_width, s_height;
  int scalesrc, scaledest;

  /*  Get the gdisplay  */
  if (! (gdisp = gdisplay_get_unique_id (gdisp_unique_id)))
    return;

  /*  adjust the initial scale -- so that window fits on screen */
  s_width = gdk_screen_width ();
  s_height = gdk_screen_height ();

  scalesrc = gdisp->scale & 0x00ff;
  scaledest = gdisp->scale >> 8;

  n_width = (width * scaledest) / scalesrc;
  n_height = (height * scaledest) / scalesrc;

  /*  Limit to the size of the screen...  */
  while (n_width > s_width || n_height > s_height)
    {
      if (scaledest > 1)
	scaledest--;
      else
	if (scalesrc < 0xff)
	  scalesrc++;

      n_width = (width * scaledest) / scalesrc;
      n_height = (height * scaledest) / scalesrc;
    }

  gdisp->scale = (scaledest << 8) + scalesrc;

  /*  The adjustment datums  */
  gdisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, 1, width));
  gdisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, 1, height));

  /*  The toplevel shell */
  gdisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref  (gdisp->shell);
  gtk_window_set_title (GTK_WINDOW (gdisp->shell), title);
  gtk_window_set_wmclass (GTK_WINDOW (gdisp->shell), "image_window", PROGRAM_NAME);
  gtk_window_set_policy (GTK_WINDOW (gdisp->shell), TRUE, TRUE, TRUE);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->shell), (gpointer) gdisp);
  gtk_widget_set_events (gdisp->shell, 
	GDK_POINTER_MOTION_MASK | 
      	GDK_POINTER_MOTION_HINT_MASK |
      	GDK_BUTTON_PRESS_MASK |
      	GDK_KEY_PRESS_MASK |
      	GDK_KEY_RELEASE_MASK); 

  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (gdisplay_delete),
		      gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "destroy",
		      (GtkSignalFunc) gdisplay_destroy,
		      gdisp);

  /*  active display callback  */
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "button_press_event",
                      GTK_SIGNAL_FUNC (gdisplay_shell_events),
                      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "key_press_event",
                      GTK_SIGNAL_FUNC (gdisplay_shell_events),
                      gdisp);

  /*  the table containing all widgets  */

  /* Ideally, we'd put everything in a 3x4 table, with the menubar occupying
   * the entire top row, and the bottom 3x3 section containing the sliders,
   * rulers, and canvas.  However, this was causing layout errors, so i put
   * the menubar and the 3x3 table in a vbox.  This seems to work correctly. 
   * -jcohen 12/4/01 */
  
  vbox = gtk_vbox_new(FALSE, 0);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_container_border_width (GTK_CONTAINER (table), 2);

  gtk_container_add (GTK_CONTAINER (gdisp->shell), vbox);
  //gtk_container_add (GTK_CONTAINER (gdisp->shell), table);
  
  /*  scrollbars, rulers, canvas, menu popup button  */
  gdisp->origin = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (gdisp->origin), GTK_SHADOW_OUT);

  gdisp->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_ruler_set_metric (GTK_RULER (gdisp->hrule), ruler_units);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS 
				(GTK_OBJECT_GET_CLASS (gdisp->hrule))->motion_notify_event,
			     GTK_OBJECT (gdisp->hrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->hrule), "button_press_event",
		      (GtkSignalFunc) gdisplay_hruler_button_press,
		      gdisp);

  gdisp->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_ruler_set_metric (GTK_RULER (gdisp->vrule), ruler_units);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS 
				(GTK_OBJECT_GET_CLASS (gdisp->vrule))->motion_notify_event,
			     GTK_OBJECT (gdisp->vrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->vrule), "button_press_event",
		      (GtkSignalFunc) gdisplay_vruler_button_press,
		      gdisp);

  gdisp->hsb = gtk_hscrollbar_new (gdisp->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->hsb, GTK_CAN_FOCUS);
  gdisp->vsb = gtk_vscrollbar_new (gdisp->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->vsb, GTK_CAN_FOCUS);
  
  /* canvas */ 
  gdisp->canvas = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (gdisp->canvas), n_width, n_height);
#if 1
  gtk_widget_set_events (gdisp->canvas, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | \
                           /*GDK_POINTER_MOTION_HINT_MASK |*/ GDK_BUTTON_PRESS_MASK | \
                           GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK | \
                           GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
                           GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | \
                           GDK_PROXIMITY_OUT_MASK);
#else
  gtk_widget_set_events (gdisp->canvas, CANVAS_EVENT_MASK);
#endif
  gtk_widget_set_extension_events (gdisp->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (gdisp->canvas, GTK_CAN_FOCUS);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->canvas), (gpointer) gdisp);

  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
                      GTK_SIGNAL_FUNC (gdisplay_shell_events),
                      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      (GtkSignalFunc) gdisplay_canvas_events,
		      gdisp);


  /*  pack all the widgets  */

  menus_get_image_menubar (gdisp);
  
  gtk_signal_connect(GTK_OBJECT(gdisp->menubar), "button-press-event", 
                  GTK_SIGNAL_FUNC(gdisplay_menubar_down), gdisp);

  //gtk_table_attach (GTK_TABLE (table), gdisp->menubar, 0, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gdisp->origin, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gdisp->hrule, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gdisp->vrule, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gdisp->canvas, 1, 2, 1, 2,   
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);  
  gtk_table_attach (GTK_TABLE (table), gdisp->hsb, 0, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gdisp->vsb, 2, 3, 0, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_box_pack_start (GTK_BOX (vbox), gdisp->menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);


  if (! image_popup_menu)
    menus_get_image_menu (&image_popup_menu, &image_accel_group);

  /*  the popup menu  */
  gdisp->popup = image_popup_menu;

  /*  the accelerator table for images  */
  gtk_window_add_accel_group (GTK_WINDOW (gdisp->shell), image_accel_group);

  gtk_widget_show (gdisp->menubar);
  gtk_widget_show (gdisp->hsb);
  gtk_widget_show (gdisp->vsb);

  //  if (show_rulers)
  if(enable_rulers_on_start)
    {
      gtk_widget_show (gdisp->origin);
      gtk_widget_show (gdisp->hrule);
      gtk_widget_show (gdisp->vrule);
    }

  gtk_widget_show (gdisp->canvas);
  gtk_widget_show (table);
  gtk_widget_show (vbox);

  gtk_widget_set_uposition(gdisp->shell, image_x + 10, image_y + 10);
  layout_connect_window_position(gdisp->shell, &image_x, &image_y, FALSE);
  minimize_register(gdisp->shell);

  gtk_widget_show (gdisp->shell);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (gdisp->canvas);
}

/* DnD functions */
static void
toolbox_set_drag_dest (GtkWidget *object)
{
  gtk_drag_dest_set (object,
             GTK_DEST_DEFAULT_ALL,
             toolbox_target_table, toolbox_n_targets,
             GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (object);

  gtk_signal_connect (GTK_OBJECT (object), "drag_drop",
              GTK_SIGNAL_FUNC (toolbox_drag_drop),
              NULL);

  gimp_dnd_tool_dest_set (object, toolbox_drop_tool, NULL);
}

static gboolean
toolbox_drag_drop (GtkWidget      *widget,
           GdkDragContext *context,
           gint            x,
           gint            y,
           guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;
/*
  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      GDrawable *drawable       = NULL;
      Layer        *layer          = NULL;
      Channel      *channel        = NULL;
      LayerMask    *layer_mask     = NULL;
      GImage       *component      = NULL;
      ChannelType   component_type = -1;

      layer = (Layer *) gtk_object_get_data (GTK_OBJECT (src_widget),
                         "gimp_layer");
      channel = (Channel *) gtk_object_get_data (GTK_OBJECT (src_widget),
                         "gimp_channel");
      layer_mask = (LayerMask *) gtk_object_get_data (GTK_OBJECT (src_widget),
                              "gimp_layer_mask");
      component = (GImage *) gtk_object_get_data (GTK_OBJECT (src_widget),
                          "gimp_component");

      if (layer)
    {
      drawable = GIMP_DRAWABLE (layer);
    }
      else if (channel)
    {
      drawable = GIMP_DRAWABLE (channel);
    }
      else if (layer_mask)
    {
      drawable = GIMP_DRAWABLE (layer_mask);
    }
      else if (component)
    {
      component_type =
        (ChannelType) gtk_object_get_data (GTK_OBJECT (src_widget),
                           "gimp_component_type");
    }

      if (drawable)
    {
          GImage *gimage;
      GImage *new_gimage;
      Layer  *new_layer;
          gint    width, height;
      gint    off_x, off_y;
      gint    bytes;

      GimpImageBaseType type;

      gimage = gimp_drawable_gimage (drawable);
          width  = gimp_drawable_width  (drawable);
          height = gimp_drawable_height (drawable);
      bytes  = gimp_drawable_bytes  (drawable);

      switch (gimp_drawable_type (drawable))
        {
        case RGB_GIMAGE: case RGBA_GIMAGE:
          type = RGB; break;
        case GRAY_GIMAGE: case GRAYA_GIMAGE:
          type = GRAY; break;
        case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
          type = INDEXED; break;
        default:
          type = RGB; break;
        }

      new_gimage = gimage_new (width, height, type);
      gimage_disable_undo (new_gimage);

      if (type == INDEXED)*/ /* copy the colormap */
/*        {
          new_gimage->num_cols = gimage->num_cols;
          memcpy (new_gimage->cmap, gimage->cmap, COLORMAP_SIZE);
        }

      gimage_set_resolution (new_gimage, gimage->xresolution, gimage->yresolution);
      gimage_set_unit (new_gimage, gimage->unit);

      if (layer)
        {
          new_layer = layer_copy (layer, FALSE);
        }
      else
        {
*/          /*  a non-layer drawable can't have an alpha channel,
           *  so add one
           */
/*          PixelRegion  srcPR, destPR;
          TileManager *tiles;

          tiles = tile_manager_new (width, height, bytes + 1);

          pixel_region_init (&srcPR, gimp_drawable_data (drawable),
                 0, 0, width, height, FALSE);
          pixel_region_init (&destPR, tiles,
                 0, 0, width, height, TRUE);

          add_alpha_region (&srcPR, &destPR);

          new_layer = layer_new_from_tiles (new_gimage,
                                                gimp_image_base_type_with_alpha(new_gimage),
                                                tiles,
                        "", OPAQUE_OPACITY, NORMAL_MODE);

          tile_manager_destroy (tiles);
        }

      gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), new_gimage);

      layer_set_name (GIMP_LAYER (new_layer),
              gimp_drawable_get_name (drawable));

      if (layer)
        {
          LayerMask *mask;
          LayerMask *new_mask;

          mask     = layer_get_mask (layer);
          new_mask = layer_get_mask (new_layer);

          if (new_mask)
        {
          gimp_drawable_set_name (GIMP_DRAWABLE (new_mask),
                      gimp_drawable_get_name (GIMP_DRAWABLE (mask)));
        }
        }

      gimp_drawable_offsets (GIMP_DRAWABLE (new_layer), &off_x, &off_y);
      layer_translate (new_layer, -off_x, -off_y);

      gimage_add_layer (new_gimage, new_layer, 0);

      gimp_context_set_display (gimp_context_get_user (),
                    gdisplay_new (new_gimage, 0x0101));

      gimage_enable_undo (new_gimage);

      return_val = TRUE;
    }
    }
*/
  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static ToolType
toolbox_drag_tool (GtkWidget *widget,
           gpointer   data)
{
  return (ToolType) data;
}

static void
toolbox_drop_tool (GtkWidget *widget,
           ToolType   tool,
           gpointer   data)
{
  //gimp_context_set_tool (gimp_context_get_user (), tool);
}


/*
 *  A text string query box
 */

typedef struct QueryBox QueryBox;

struct QueryBox
{
  GtkWidget *qbox;
  GtkWidget *entry;
  QueryFunc callback;
  gpointer data;
};

static void query_box_cancel_callback (GtkWidget *, gpointer);
static void query_box_ok_callback (GtkWidget *, gpointer);
static gint query_box_delete_callback (GtkWidget *, GdkEvent *, gpointer);

GtkWidget *
query_string_box (char        *title,
		  char        *message,
		  char        *initial,
		  QueryFunc    callback,
		  gpointer     data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;

  query_box = (QueryBox *) g_malloc_zero (sizeof (QueryBox));

  qbox = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (qbox), title);
  gtk_window_set_wmclass (GTK_WINDOW (qbox), "query_box", PROGRAM_NAME);

  gtk_widget_set_uposition(qbox, generic_window_x, generic_window_y);
  layout_connect_window_position(qbox, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(qbox);

  gtk_signal_connect (GTK_OBJECT (qbox), "delete_event",
		      (GtkSignalFunc) query_box_delete_callback,
		      query_box);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (qbox)->action_area), 2);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) query_box_ok_callback,
                      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) query_box_cancel_callback,
                      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);
  gtk_widget_show (entry);

  query_box->qbox = qbox;
  query_box->entry = entry;
  query_box->callback = callback;
  query_box->data = data;

  gtk_widget_show (qbox);

  return qbox;
}

static gint
query_box_delete_callback (GtkWidget *w,
			   GdkEvent  *e,
			   gpointer   client_data)
{
  query_box_cancel_callback (w, client_data);

  return TRUE;
}

static void
query_box_cancel_callback (GtkWidget *w,
			   gpointer   client_data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) client_data;

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
query_box_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  QueryBox *query_box;
  char *string;

  query_box = (QueryBox *) client_data;

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) string);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}


/*
 *  Message Boxes...
 */

#ifdef WIN32
#undef MessageBox
#endif

typedef struct MessageBox MessageBox;

struct MessageBox
{
  GtkWidget  *mbox;
  GtkCallback callback;
  int         i;  
  gpointer    data;
};

static void message_box_close_callback (GtkWidget *, gpointer);
static gint message_box_delete_callback (GtkWidget *, GdkEvent *, gpointer);

# define MAX_TEXTS 25
static GtkWidget *label_vbox[MAX_TEXTS+1];

GtkWidget *
message_box (const char        *orig,
	     GtkCallback  callback,
	     gpointer     data)
{
  MessageBox *msg_box;
  GtkWidget *mbox;
  GtkWidget *vbox;
  static GtkWidget *status_label[MAX_TEXTS+2];
  GtkWidget *label;
  GtkWidget *button;
  char *message = NULL;
  char *str;
  char *memory;
  static int  texts[MAX_TEXTS+2], i;
  static char *messages_texts[MAX_TEXTS+2];

  if(!messages_texts[0])
    for (i=0; i < MAX_TEXTS+2; ++i)
    {
      messages_texts[i] = 0;
      label_vbox[i] = status_label[i] = 0;
      texts[i] = 0;
    }

  if (orig)
    message = g_strdup (orig);
  else
    return NULL;

  i = 0;
  while( texts[i] && label_vbox[i] )
  {
    if(strcmp(orig, messages_texts[i]) == 0 && label_vbox[i])
    {
      char *t = _("Message repeated %d times.");
      char *m = g_strdup_printf (t, texts[i]);
      if(!status_label[i])
      {
	  status_label[i] = gtk_label_new (m);
	  gtk_box_pack_start (GTK_BOX (label_vbox[i]), status_label[i], TRUE, FALSE, 0);
	  gtk_widget_show (status_label[i]);
      }
      gtk_label_set_text( GTK_LABEL(status_label[i]), m);
      ++texts[i];
      return NULL;
    } else if(i == MAX_TEXTS) {
      if(!messages_texts[i+1]) {
        message = g_strdup (_("Too many massages. Giving up."));
      } else {
        ++texts[i+1];
        return NULL;
      }
    }
    ++i;
  }


  memory = message;
  messages_texts[i] = g_strdup (message);
  status_label[i] = 0;
  ++texts[i];

  msg_box = (MessageBox *) g_malloc_zero (sizeof (MessageBox));

  msg_box->i = i;

  mbox = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (mbox), "gimp_message", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (mbox), PROGRAM_NAME " Message");
  gtk_widget_set_uposition(mbox, generic_window_x, generic_window_y);
  layout_connect_window_position(mbox, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(mbox);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (mbox)->action_area), 2);
  gtk_signal_connect (GTK_OBJECT (mbox), "delete_event",
		      GTK_SIGNAL_FUNC (message_box_delete_callback),
		      msg_box);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) message_box_close_callback,
                      msg_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label_vbox[i] = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), label_vbox[i], TRUE, FALSE, 0);
  gtk_widget_show (label_vbox[i]);

  str = message;
  while (*str != '\0')
    {
      if (*str == '\n')
	{
	  *str = '\0';
	  label = gtk_label_new (message);
	  gtk_box_pack_start (GTK_BOX (label_vbox[i]), label, TRUE, FALSE, 0);
	  gtk_widget_show (label);
	  message = str + 1;
	}
      str++;
    }

  if (*message != '\0')
    {
      label = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (label_vbox[i]), label, TRUE, FALSE, 0);
      gtk_widget_show (label);
    }

  g_free (memory);

  msg_box->mbox = mbox;
  msg_box->callback = callback;
  msg_box->data = data;

  gtk_widget_show (mbox);

  return mbox;
}

static gint
message_box_delete_callback (GtkWidget *w, GdkEvent *e, gpointer client_data)
{
  message_box_close_callback (w, client_data);

  return TRUE;
}


static void
message_box_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  MessageBox *msg_box;

  msg_box = (MessageBox *) client_data;

  /*  If there is a valid callback, invoke it  */
  if (msg_box->callback)
    (* msg_box->callback) (w, msg_box->data);

  /*  Destroy the box  */
  label_vbox[msg_box->i] = 0;
  gtk_widget_destroy (msg_box->mbox);

  g_free (msg_box);
}


void
progress_start ()
{ if(!progress_area)
  {	return;
  }
  if (!GTK_WIDGET_VISIBLE (progress_area))
    {
      gtk_widget_set_usize (progress_area,
			    tool_label_area->allocation.width,
			    tool_label_area->allocation.height);

      /*
      gtk_container_disable_resize (GTK_CONTAINER (progress_area->parent));
      */

      gtk_widget_hide (tool_label_area);
      gtk_widget_show (progress_area);

      /*
      gtk_container_enable_resize (GTK_CONTAINER (progress_area->parent));
      */
    }
}

void
progress_update (float percentage)
{
  if (!(percentage >= 0.0 && percentage <= 1.0))
    return;

  if(!progress_area)
  {	return;
  }

  gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_area), percentage);
  if (GTK_WIDGET_VISIBLE (progress_area))
    gdk_flush ();
}

void
progress_step ()
{
  float val;
  if(!progress_area)
  {	return;
  }
  if (GTK_WIDGET_VISIBLE (progress_area))
    {
      val = gtk_progress_get_current_percentage(&(GTK_PROGRESS_BAR(progress_area)->progress))+
						  0.01;
      if (val > 1.0)
	val = 0.0;

      progress_update (val);
    }
}

void
progress_end ()
{ if(!progress_area)
  {	return;
  }
  if (GTK_WIDGET_VISIBLE (progress_area))
    {
      /*
      tk_container_disable_resize (GTK_CONTAINER (progress_area->parent));
      */

      gtk_widget_hide (progress_area);
      gtk_widget_show (tool_label_area);

      /*
      gtk_container_enable_resize (GTK_CONTAINER (progress_area->parent));
      */

      gdk_flush ();

      gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_area), 0.0);
    }
}


