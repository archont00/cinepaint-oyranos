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

#include <lcms.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../about_dialog.h"
#include "../bugs_dialog.h"
#include "../actionarea.h"
#include "../app_procs.h"
#include "../brightness_contrast.h"
#include "../base_frame_manager.h"
#include "../brush.h"
#include "../by_color_select.h"
#include "../channels_dialog.h"
#include "../channel.h"
#include "../cms.h"
#include "../colormaps.h"
#include "../color_balance.h"
#include "../color_correction_gui.h"
#include "../color_select.h"
#include "../commands.h"
#include "../convert.h"
#include "../curves.h"
#include "../desaturate.h"
#include "../devices.h" 
#include "../channel_ops.h"
#include "../channel.h"
#include "../drawable.h"
#include "displaylut.h"
#include "../equalize.h"
#include "../fileops.h"
#include "../floating_sel.h"
#include "../gamma_expose.h"
#include "../expose_image.h"
#include "../gdisplay_ops.h"
#include "../general.h"
#include "../gimage_cmds.h"
#include "../gimage_mask.h"
#include "../rc.h"
#include "../global_edit.h"
#include "../gradient.h"
#include "../histogram_tool.h"
#include "../hue_saturation.h"
#include "../image_render.h"
#include "../indexed_palette.h"
#include "../info_window.h"
#include "../interface.h"
#include "../invert.h"
#include "../layers_dialog.h"
#include "../layer_select.h"
#include "../levels.h"
#include "../look_profile.h"
#include "../paint_funcs_area.h"
#include "../palette.h"
#include "../patterns.h"
#include "../pixelrow.h"
#include "../plug_in.h"
#include "../posterize.h"
#include "../resize.h"
#include "../scale.h"
#include "../store_frame_manager.h"
#include "../tag.h"
#include "../threshold.h"
#include "../tips_dialog.h"
#include "../tools.h"
#include "../undo.h"
#include "../brushgenerated.h"
#include "../zoom.h"
#include "../zoombookmark.h"
#include "../minimize.h"
#include "../layout.h"
#include "../brushlist.h"
#include "../cursorutil.h"
#include "../lib/wire/precision.h"

/*  external functions  */
extern void layers_dialog_layer_merge_query (GImage *, int);

typedef struct {
  GtkWidget *dlg;
  GtkWidget *height_entry;
  GtkWidget *width_entry;
  int width;
  int height;
  Format format;
  StorageType storage;
  int fill_type;
  Precision precision;
} NewImageValues;

typedef struct
{
  GtkWidget * shell;
  Resize *    resize;
  int         gimage_id;
} ImageResize;

/*  new image local functions  */
static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static gint file_new_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);

/*  static variables  */
static   int          last_width = 256;
static   int          last_height = 256;
static   Format       last_format = FORMAT_RGB;
static   Precision    last_precision = PRECISION_NONE;
static   StorageType      last_storage = 
#ifdef NO_TILES						  
						  STORAGE_FLAT;
#else
						  STORAGE_TILED;
#endif
static   int          last_fill_type = BACKGROUND_FILL;
static   int          initialising = TRUE;

/*  preferences local functions  */
static void file_prefs_ok_callback (GtkWidget *, GtkWidget *);
static void file_prefs_save_callback (GtkWidget *, GtkWidget *);
static void file_prefs_cancel_callback (GtkWidget *, GtkWidget *);
static gint file_prefs_delete_callback (GtkWidget *, GdkEvent *, GtkWidget *);
static void file_prefs_toggle_callback (GtkWidget *, gpointer);
static void file_prefs_text_callback (GtkWidget *, gpointer);
static void file_prefs_string_menu_callback (GtkWidget *, gpointer);
static void file_prefs_int_menu_callback (GtkWidget *, gpointer);
static void file_prefs_flags_menu_callback (GtkWidget *, gpointer);
static void file_prefs_preview_size_callback (GtkWidget *, gpointer);
static gint file_prefs_gamut_alarm_callback(GtkWidget *, GdkEvent *);
static void file_prefs_gamut_alarm_select_callback (PixelRow *, ColorSelectState, void *);




/*  static variables  */
static   GtkWidget   *prefs_dlg = NULL;
static   ColorSelectP color_select = NULL;/*needed to set gamut alarm in prefs*/
static   GtkWidget   *cms_oyranos_wid[5] = {NULL,NULL,NULL,NULL,NULL};

static   int          old_transparency_type;
static   int          old_transparency_size;
static   int          old_levels_of_undo;
static   int          old_marching_speed;
static   int          old_allow_resize_windows;
static   int          old_auto_save;
static   int          old_preview_size;
static   int          old_no_cursor_updating;
static   int          old_show_tool_tips;
static   int          old_enable_rgbm_painting;
static   int          old_enable_rulers_on_start;
static   int          old_enable_brush_dialog;
static   int          old_enable_layer_dialog;
static   int          old_enable_tmp_saving;
static   int          old_enable_channel_revert; 
static   int          old_enable_paste_c_disp; 
static   int          old_cubic_interpolation;
static   int          old_confirm_on_close;
static   int          old_default_width;
static   int          old_default_height;
static   Format       old_default_format;
static   Precision    old_default_precision;
static   int          new_dialog_run;
static   int          old_stingy_memory_use;
static   int          old_tile_cache_size;
static   int          old_install_cmap;
static   int          old_cycled_marching_ants;
static   int          old_gamut_alarm_red;
static   int          old_gamut_alarm_green;
static   int          old_gamut_alarm_blue;
static   char *       old_cms_default_image_profile_name;
static   char *       old_cms_default_proof_profile_name;
static   char *       old_cms_workspace_profile_name;
static   char *       old_cms_display_profile_name;
static   int          old_cms_default_intent;
static   int          old_cms_default_flags;
static   char *       old_cms_profile_path;
static   int          old_cms_open_action;
static   int          old_cms_mismatch_action;
static   int          old_cms_bpc_by_default;
static   int          old_cms_manage_by_default;
static   int          old_cms_oyranos;
static   char *       old_temp_path;
static   char*        old_default_brush;
static   char *       old_swap_path;
static   char *       old_brush_path;
static   char *       old_pattern_path;
static   char *       old_palette_path;
static   char *       old_frame_manager_path;
static   char *       old_plug_in_path;
static   char *       old_gradient_path;

static   char *       edit_temp_path = NULL;
static   char*        edit_default_brush = NULL;
static   char *       edit_swap_path = NULL;
static   char *       edit_brush_path = NULL;
static   char *       edit_pattern_path = NULL;
static   char *       edit_palette_path = NULL;
static   char *       edit_frame_manager_path = NULL;
static   char *       edit_plug_in_path = NULL;
static   char *       edit_gradient_path = NULL;
static   int          edit_stingy_memory_use;
static   int          edit_tile_cache_size;
static   int          edit_install_cmap;
static   int          edit_cycled_marching_ants;

/*  local functions  */
static void   image_resize_callback (GtkWidget *, gpointer);
static void   image_scale_callback (GtkWidget *, gpointer);
static void   image_cancel_callback (GtkWidget *, gpointer);
static gint   image_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void   gimage_mask_feather_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_border_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_grow_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_shrink_callback (GtkWidget *, gpointer, gpointer);

/*  variables declared in gimage_mask.c--we need them to set up
 *  initial values for the various dialog boxes which query for values
 */
extern double gimage_mask_feather_radius;
extern int    gimage_mask_border_radius;
extern int    gimage_mask_grow_pixels;
extern int    gimage_mask_shrink_pixels;


static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageValues *vals;
  GImage *gimage;
  GDisplay *gdisplay;
  Layer *layer;
  Format format = FORMAT_NONE;
  Alpha alpha = ALPHA_NONE;

  vals = data;

  vals->width = atoi (gtk_entry_get_text (GTK_ENTRY (vals->width_entry)));
  vals->height = atoi (gtk_entry_get_text (GTK_ENTRY (vals->height_entry)));

  gtk_widget_destroy (vals->dlg);

  last_width = vals->width;
  last_height = vals->height;
  last_format = vals->format;
  last_precision = vals->precision;
  last_storage = vals->storage;
  last_fill_type = vals->fill_type;
/* rsr: Set breakpoint here to debug {File}{New} */
  switch (vals->fill_type)
    {
    case BACKGROUND_FILL:
    case FOREGROUND_FILL:
    case WHITE_FILL:
    case NO_FILL:
      format = (vals->format == FORMAT_RGB) ? FORMAT_RGB: FORMAT_GRAY;
      alpha = ALPHA_NO;
      break;
    case TRANSPARENT_FILL:
      format = (vals->format == FORMAT_RGB) ? FORMAT_RGB: FORMAT_GRAY;
      alpha =  ALPHA_YES;
      break;
    default:
      format = FORMAT_NONE;
      alpha = ALPHA_NONE;
      break;
    }


  {  
    Tag tag = tag_new ( vals->precision, format, alpha);
    gimage = gimage_new (vals->width, vals->height, tag);

    /*  Make the background (or first) layer  */
    layer = layer_new (gimage->ID, gimage->width, gimage->height,
                       tag, vals->storage, _("Background"), OPAQUE_OPACITY, NORMAL);
  }
  if (layer) {
    /*  add the new layer to the gimage  */
    gimage_disable_undo (gimage);
    gimage_add_layer (gimage, layer, 0);
    gimage_enable_undo (gimage);

    if (vals->fill_type != NO_FILL)
      drawable_fill (GIMP_DRAWABLE(layer), vals->fill_type);

    gimage_clean_all (gimage);
    
    gdisplay = gdisplay_new (gimage, 0x0101);
  }

  g_free (vals);
}

static gint
file_new_delete_callback (GtkWidget *widget,
			  GdkEvent *event,
			  gpointer data)
{
  file_new_cancel_callback (widget, data);

  return TRUE;
}
  

static void
file_new_cancel_callback (GtkWidget *widget,
			  gpointer   data)
{
  NewImageValues *vals;

  vals = data;

  gtk_widget_destroy (vals->dlg);
  g_free (vals);
}

static void
file_new_toggle_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    }
}

void
file_new_cmd_callback (GtkWidget *widget,
		       gpointer   callback_data,
		       guint      callback_action)
{
  GDisplay *gdisp;
  NewImageValues *vals;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GSList *group;
  char buffer[32];

  if(!new_dialog_run)
    {
      last_width = default_width;
      last_height = default_height;
      last_format = default_format;
      last_precision = default_precision;
      new_dialog_run = 1;
    }

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if ((long) callback_action)
    gdisp = gdisplay_active ();
  else
    gdisp = NULL;

  vals = g_malloc_zero (sizeof (NewImageValues));
  vals->storage = last_storage;
  vals->fill_type = last_fill_type;

  if (gdisp)
    {
      vals->width = gdisp->gimage->width;
      vals->height = gdisp->gimage->height;
      vals->format = tag_format (gimage_tag (gdisp->gimage));
      vals->precision = tag_precision( gimage_tag (gdisp->gimage) );
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->format = last_format;
      vals->precision = last_precision;
    }

  if (vals->format == FORMAT_INDEXED)
    vals->format = FORMAT_RGB;    /* no indexed images */

  vals->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->dlg), "new_image", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (vals->dlg), _("New Image"));
  gtk_widget_set_uposition(vals->dlg, generic_window_x, generic_window_y);
  layout_connect_window_position(vals->dlg, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(vals->dlg);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (vals->dlg), "delete_event", 
		      GTK_SIGNAL_FUNC (file_new_delete_callback),
		      vals);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (vals->dlg)->action_area), 2);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_cancel_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->vbox),
		      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  vals->width_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->width_entry, 75, 0);
  sprintf (buffer, "%d", vals->width);
  gtk_entry_set_text (GTK_ENTRY (vals->width_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->width_entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->width_entry);

  vals->height_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->height_entry, 75, 0);
  sprintf (buffer, "%d", vals->height);
  gtk_entry_set_text (GTK_ENTRY (vals->height_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->height_entry, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->height_entry);


  frame = gtk_frame_new (_("Image Type"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, _("RGB"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FORMAT_RGB);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->format);
  if (vals->format == FORMAT_RGB)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Grayscale"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FORMAT_GRAY);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->format);
  if (vals->format == FORMAT_GRAY)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

/* added by IMAGEWORKS (doug creel 02/20/02) */
  frame = gtk_frame_new (_("Precision"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, PRECISION_U8_STRING);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_U8);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->precision);
  if (vals->precision == PRECISION_U8)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, PRECISION_U16_STRING);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_U16);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->precision);
  if (vals->precision == PRECISION_U16)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, PRECISION_FLOAT16_STRING);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_FLOAT16);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->precision);
  if (vals->precision == PRECISION_FLOAT16)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, PRECISION_FLOAT_STRING);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_FLOAT);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->precision);
  if (vals->precision == PRECISION_FLOAT)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, PRECISION_BFP_STRING);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_BFP);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->precision);
  if (vals->precision == PRECISION_BFP)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  frame = gtk_frame_new (_("Fill Type"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, _("Background"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) BACKGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == BACKGROUND_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("White"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) WHITE_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == WHITE_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Transparent"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) TRANSPARENT_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == TRANSPARENT_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Foreground"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FOREGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == FOREGROUND_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);
#if 0
  button = gtk_radio_button_new_with_label (group, _("No Fill"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) NO_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == NO_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);
#endif
  gtk_widget_show (vals->dlg);
}

void
file_reload_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
 
  file_reload_callback (gdisplay_active ());
}

void
file_open_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_open_callback (widget, client_data);
}

void
file_save_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_save_callback (widget, client_data);
}

void
file_save_as_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  file_save_as_callback (widget, client_data);
}

void
file_save_copy_as_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  save_a_copy (); 
  file_save_as_callback (widget, client_data);
}


/* Some information regarding preferences, compiled by Raph Levien 11/3/97.

   The following preference items cannot be set on the fly (at least
   according to the existing pref code - it may be that changing them
   so they're set on the fly is not hard).

   temp-path
   swap-path
   brush-path
   pattern-path
   plug-in-path
   palette-path
   gradient-path
   stingy-memory-use
   tile-cache-size
   install-cmap
   cycled-marching-ants

   All of these now have variables of the form edit_temp_path, which
   are copied from the actual variables (e.g. temp_path) the first time
   the dialog box is started.

   Variables of the form old_temp_path represent the values at the
   time the dialog is opened - a cancel copies them back from old to
   the real variables or the edit variables, depending on whether they
   can be set on the fly.

   Here are the remaining issues as I see them:

   Still no settings for default-brush, default-gradient,
   default-palette, default-pattern, gamma-correction, color-cube,
   show-rulers, ruler-units. No widget for confirm-on-close although
   a lot of stuff is there.

   No UI feedback for the fact that some settings won't take effect
   until the next restart.

   The semantics of "save" are a little funny - it only saves the
   settings that are different from the way they were when the dialog
   was opened. So you can set something, close the window, open it
   up again, click "save" and have nothing happen. To change this
   to more intuitive semantics, we should have a whole set of init_
   variables that are set the first time the dialog is opened (along
   with the edit_ variables that are currently set). Then, the save
   callback checks against the init_ variable rather than the old_.

   */

/* Copy the string from source to destination, freeing the string stored
   in the destination if there is one there already. */
static void
file_prefs_strset (char **dst, char *src)
{
  if (*dst != NULL)
    g_free (*dst);
  *dst = g_strdup (src);
}


/* Duplicate the string, but treat NULL as the empty string. */
static char *
file_prefs_strdup (char *src)
{
  return g_strdup (src == NULL ? "" : src);
}

/* Compare two strings, but treat NULL as the empty string. */
static int
file_prefs_strcmp (char *src1, char *src2)
{
  return strcmp (src1 == NULL ? "" : src1,
		   src2 == NULL ? "" : src2);
}

static void
file_prefs_ok_callback (GtkWidget *widget,
			GtkWidget *dlg)
{

  if (levels_of_undo < 0)
    {
      g_message (_("Error: Levels of undo must be zero or greater."));
      levels_of_undo = old_levels_of_undo;
      return;
    }
  if (marching_speed < 50)
    {
      g_message (_("Error: Marching speed must be 50 or greater."));
      marching_speed = old_marching_speed;
      return;
    }
  if (default_width < 1)
    {
      g_message (_("Error: Default width must be one or greater."));
      default_width = old_default_width;
      return;
    }
  if (default_height < 1)
    {
      g_message (_("Error: Default height must be one or greater."));
      default_height = old_default_height;
      return;
    }

  gtk_widget_destroy (dlg);
  color_select_free(color_select);  
  color_select = NULL;

  prefs_dlg = NULL;

  if (show_tool_tips)
    gtk_tooltips_enable (tool_tips);
  else
    gtk_tooltips_disable (tool_tips);

  file_tmp_save (enable_tmp_saving);
  file_channel_revert (enable_channel_revert);   

  if (file_prefs_strcmp (old_cms_display_profile_name, cms_display_profile_name)) 
  {   if (cms_display_profile_name == NULL)
      {   g_message(_("The display profile is set to [none]. Color management will be switched off for all displays."));
          gdisplay_all_set_colormanaged(FALSE);
      }
      else 
      {   CMSProfile *profile = cms_get_profile_from_file(cms_display_profile_name);
	  cms_set_display_profile(profile);   
      }
  }

  if(cms_oyranos)
      cms_init();
}

static void
file_prefs_save_callback (GtkWidget *widget,
			  GtkWidget *dlg)
{
  GList *update = NULL; /* options that should be updated in .gimprc */
  GList *remove = NULL; /* options that should be commented out */
  int save_stingy_memory_use;
  int save_tile_cache_size;
  int save_install_cmap;
  int save_cycled_marching_ants;
  gchar *save_temp_path;
  gchar *save_default_brush; 
  gchar *save_swap_path;
  gchar *save_brush_path;
  gchar *save_pattern_path;
  gchar *save_palette_path;
  gchar *save_frame_manager_path;
  gchar *save_plug_in_path;
  gchar *save_gradient_path;
  gchar *save_cms_profile_path;
  int restart_notification = FALSE;

  file_prefs_ok_callback (widget, dlg);

  /* Save variables so that we can restore them later */
  save_stingy_memory_use = stingy_memory_use;
  save_tile_cache_size = tile_cache_size;
  save_install_cmap = install_cmap;
  save_cycled_marching_ants = cycled_marching_ants;
  save_temp_path = temp_path;
  save_default_brush = default_brush; 
  save_swap_path = swap_path;
  save_brush_path = brush_path;
  save_pattern_path = pattern_path;
  save_palette_path = palette_path;
  save_frame_manager_path = frame_manager_path;
  save_plug_in_path = plug_in_path;
  save_gradient_path = gradient_path;
  save_cms_profile_path = cms_profile_path;

  if (levels_of_undo != old_levels_of_undo)
    update = g_list_append (update, "undo-levels");
  if (marching_speed != old_marching_speed)
    update = g_list_append (update, "marching-ants-speed");
  if (allow_resize_windows != old_allow_resize_windows)
    update = g_list_append (update, "allow-resize-windows");
  if (enable_rulers_on_start != old_enable_rulers_on_start)
    update = g_list_append (update, "enable-rulers-on-start");
  if (auto_save != old_auto_save)
    {
      update = g_list_append (update, "auto-save");
      remove = g_list_append (remove, "dont-auto-save");
    }
  if (no_cursor_updating != old_no_cursor_updating)
    {
      update = g_list_append (update, "cursor-updating");
      remove = g_list_append (remove, "no-cursor-updating");
    }
  if (show_tool_tips != old_show_tool_tips)
    {
      update = g_list_append (update, "show-tool-tips");
      remove = g_list_append (remove, "dont-show-tool-tips");
    }
  if (enable_rgbm_painting != old_enable_rgbm_painting)
    {
      update = g_list_append (update, "enable-rgbm-painting");
      remove = g_list_append (remove, "dont-enable-rgbm-painting");
    }
/*  if (enable_brush_dialog != old_enable_brush_dialog)
    {
      update = g_list_append (update, "enable-brush-dialog");
      remove = g_list_append (remove, "dont-enable-brush-dialog");
    }
  if (enable_layer_dialog != old_enable_layer_dialog)
    {
      update = g_list_append (update, "enable-layer-dialog");
      remove = g_list_append (remove, "dont-enable-layer-dialog");
    }
*/
  if (enable_paste_c_disp!= old_enable_paste_c_disp)
    {
      update = g_list_append (update, "enable-paste-c-disp");
      remove = g_list_append (remove, "dont-enable-paste-c-disp");
    }
  if (enable_tmp_saving != old_enable_tmp_saving)
    {
      update = g_list_append (update, "enable-tmp-saving");
      remove = g_list_append (remove, "dont-enable-tmp-saving");
    }
  if (enable_channel_revert != old_enable_channel_revert)
    {
      update = g_list_append (update, "enable-channel-revert");
      remove = g_list_append (remove, "dont-enable-channel-revert");
    }
  if (cubic_interpolation != old_cubic_interpolation)
    update = g_list_append (update, "cubic-interpolation");
  if (confirm_on_close != old_confirm_on_close)
    update = g_list_append (update, "confirm-on-close");
  if (default_width != old_default_width ||
      default_height != old_default_height)
    update = g_list_append (update, "default-image-size");
  if (default_format != old_default_format)
    update = g_list_append (update, "default-image-type");
  if (default_precision != old_default_precision)
    update = g_list_append (update, "default-image-precision");
  if (preview_size != old_preview_size)
    update = g_list_append (update, "preview-size");
  if (transparency_type != old_transparency_type)
    update = g_list_append (update, "transparency-type");
  if (transparency_size != old_transparency_size)
    update = g_list_append (update, "transparency-size");
  if (gamut_alarm_red != old_gamut_alarm_red) 
    update = g_list_append (update, "gamut-alarm-red");
  if (gamut_alarm_green != old_gamut_alarm_green) 
     update = g_list_append (update, "gamut-alarm-green");
  if (gamut_alarm_blue != old_gamut_alarm_blue)  
     update = g_list_append (update, "gamut-alarm-blue");
    
  if (file_prefs_strcmp (old_cms_default_image_profile_name, cms_default_image_profile_name))    
  {   if (cms_default_image_profile_name == NULL)
      {   remove = g_list_append (update, "cms-default-image-profile-name");
      }
      else
      {   update = g_list_append (update, "cms-default-image-profile-name");    
      }
  }
  if (file_prefs_strcmp (old_cms_default_proof_profile_name, cms_default_proof_profile_name))    
  {   if (cms_default_proof_profile_name == NULL)
      {   remove = g_list_append (update, "cms-default-proof-profile-name");
      }
      else
      {   update = g_list_append (update, "cms-default-proof-profile-name");    
      }
  }
  if (file_prefs_strcmp (old_cms_workspace_profile_name, cms_workspace_profile_name))    
  {   if (cms_workspace_profile_name == NULL)
      {   remove = g_list_append (update, "cms-workspace-profile-name");
      }
      else
      {   update = g_list_append (update, "cms-workspace-profile-name");    
      }
  }
  if (file_prefs_strcmp (old_cms_display_profile_name, cms_display_profile_name)) 
  {   if (cms_display_profile_name == NULL)
      {   remove = g_list_append (update, "cms-display-profile-name");
      }
      else
      {   update = g_list_append (update, "cms-display-profile-name");    
      }
  }
  if (file_prefs_strcmp (old_cms_profile_path, cms_profile_path))
      update = g_list_append (update, "cms-profile-path");
  if (old_cms_default_intent != cms_default_intent)
      update = g_list_append (update, "cms-default-intent");
  if (old_cms_default_flags != cms_default_flags)
      update = g_list_append (update, "cms-default-flags");
  if (old_cms_open_action != cms_open_action)
      update = g_list_append (update, "cms-open-action");
  if (old_cms_mismatch_action != cms_mismatch_action)
      update = g_list_append (update, "cms-mismatch-action");
  if (old_cms_bpc_by_default != cms_bpc_by_default)
      update = g_list_append (update, "cms-bpc-by-default");
  if (old_cms_manage_by_default != cms_manage_by_default)
      update = g_list_append (update, "cms-manage-by-default");
  if (old_cms_oyranos != cms_oyranos)
      update = g_list_append (update, "cms-oyranos");
  if (edit_stingy_memory_use != stingy_memory_use)
    {
      update = g_list_append (update, "stingy-memory-use");
      stingy_memory_use = edit_stingy_memory_use;
      restart_notification = TRUE;
    }
  if (edit_tile_cache_size != tile_cache_size)
    {
      update = g_list_append (update, "tile-cache-size");
      tile_cache_size = edit_tile_cache_size;
      restart_notification = TRUE;
    }
  if (edit_install_cmap != old_install_cmap)
    {
      update = g_list_append (update, "install-colormap");
      install_cmap = edit_install_cmap;
      restart_notification = TRUE;
    }
  if (edit_cycled_marching_ants != cycled_marching_ants)
    {
      update = g_list_append (update, "colormap-cycling");
      cycled_marching_ants = edit_cycled_marching_ants;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (temp_path, edit_temp_path))
    {
      update = g_list_append (update, "temp-path");
      temp_path = edit_temp_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (default_brush, edit_default_brush))
    {
      update = g_list_append (update, "default-brush");
      default_brush = edit_default_brush;
      restart_notification = TRUE;
    }

  if (file_prefs_strcmp (swap_path, edit_swap_path))
    {
      update = g_list_append (update, "swap-path");
      swap_path = edit_swap_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (brush_path, edit_brush_path))
    {
      update = g_list_append (update, "brush-path");
      brush_path = edit_brush_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (pattern_path, edit_pattern_path))
    {
      update = g_list_append (update, "pattern-path");
      pattern_path = edit_pattern_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (frame_manager_path, edit_frame_manager_path))
    {
      update = g_list_append (update, "frame_manager-path");
      frame_manager_path = edit_frame_manager_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (palette_path, edit_palette_path))
    {
      update = g_list_append (update, "palette-path");
      palette_path = edit_palette_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (plug_in_path, edit_plug_in_path))
    {
      update = g_list_append (update, "plug-in-path");
      plug_in_path = edit_plug_in_path;
      restart_notification = TRUE;
    }
  if (file_prefs_strcmp (gradient_path, edit_gradient_path))
    {
      update = g_list_append (update, "gradient-path");
      gradient_path = edit_gradient_path;
      restart_notification = TRUE;
    }
  save_gimprc (&update, &remove);

  /* Restore variables which must not change */
  stingy_memory_use = save_stingy_memory_use;
  tile_cache_size = save_tile_cache_size;
  install_cmap = save_install_cmap;
  cycled_marching_ants = save_cycled_marching_ants;
  temp_path = save_temp_path;
  default_brush = save_default_brush;
  swap_path = save_swap_path;
  brush_path = save_brush_path;
  pattern_path = save_pattern_path;
  palette_path = save_palette_path;
  frame_manager_path = save_frame_manager_path;
  plug_in_path = save_plug_in_path;
  gradient_path = save_gradient_path;
  cms_profile_path = save_cms_profile_path;

  if (restart_notification)
    g_message (_("You will need to restart for these changes to take effect."));

  g_list_free (update);
  g_list_free (remove);

  if(cms_oyranos)
      cms_init();
}

static int
file_prefs_delete_callback (GtkWidget *widget,
			    GdkEvent *event,
			    GtkWidget *dlg)
{
  file_prefs_cancel_callback (widget, dlg);

  /* the widget is already destroyed here no need to try again */
  return TRUE;
}

static void
file_prefs_cancel_callback (GtkWidget *widget,
			    GtkWidget *dlg)
{
  gtk_widget_destroy (dlg);
  color_select_free(color_select);  
  color_select = NULL;

  prefs_dlg = NULL;

  levels_of_undo = old_levels_of_undo;
  marching_speed = old_marching_speed;
  allow_resize_windows = old_allow_resize_windows;
  auto_save = old_auto_save;
  no_cursor_updating = old_no_cursor_updating;
  show_tool_tips = old_show_tool_tips;
  enable_rulers_on_start=old_enable_rulers_on_start;
  enable_rgbm_painting = old_enable_rgbm_painting;
  enable_brush_dialog = old_enable_brush_dialog;
  enable_layer_dialog = old_enable_layer_dialog;
  enable_paste_c_disp = old_enable_paste_c_disp;
  enable_tmp_saving = old_enable_tmp_saving;
  enable_channel_revert = old_enable_channel_revert;
  cubic_interpolation = old_cubic_interpolation;
  confirm_on_close = old_confirm_on_close;
  default_width = old_default_width;
  default_height = old_default_height;
  default_format = old_default_format;
  default_precision = old_default_precision;
  gamut_alarm_red = old_gamut_alarm_red;
  gamut_alarm_blue = old_gamut_alarm_blue;
  gamut_alarm_green = old_gamut_alarm_green;  
  file_prefs_strset(&cms_default_image_profile_name, old_cms_default_image_profile_name);
  file_prefs_strset(&cms_default_proof_profile_name, old_cms_default_proof_profile_name);
  file_prefs_strset(&cms_workspace_profile_name, old_cms_workspace_profile_name);
  file_prefs_strset(&cms_display_profile_name, old_cms_display_profile_name);
  file_prefs_strset (&cms_profile_path, old_cms_profile_path);
  cms_default_intent = old_cms_default_intent;
  cms_default_flags = old_cms_default_flags;
  cms_open_action = old_cms_open_action;
  cms_mismatch_action = old_cms_mismatch_action;
  cms_bpc_by_default = old_cms_bpc_by_default;
  cms_manage_by_default = old_cms_manage_by_default;
  cms_oyranos = old_cms_oyranos;

  if (preview_size != old_preview_size)
    {
      lc_dialog_rebuild (old_preview_size);
      layer_select_update_preview_size ();
    }

  if ((transparency_type != old_transparency_type) ||
      (transparency_size != old_transparency_size))
    {
      transparency_type = old_transparency_type;
      transparency_size = old_transparency_size;

      render_setup (transparency_type, transparency_size);
      layer_invalidate_previews (-1);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }

  edit_stingy_memory_use = old_stingy_memory_use;
  edit_tile_cache_size = old_tile_cache_size;
  edit_install_cmap = old_install_cmap;
  edit_cycled_marching_ants = old_cycled_marching_ants;
  file_prefs_strset (&edit_temp_path, old_temp_path);
  file_prefs_strset (&edit_default_brush, old_default_brush);
  file_prefs_strset (&edit_swap_path, old_swap_path);
  file_prefs_strset (&edit_brush_path, old_brush_path);
  file_prefs_strset (&edit_pattern_path, old_pattern_path);
  file_prefs_strset (&edit_palette_path, old_palette_path);
  file_prefs_strset (&edit_frame_manager_path, old_frame_manager_path);
  file_prefs_strset (&edit_plug_in_path, old_plug_in_path);
  file_prefs_strset (&edit_gradient_path, old_gradient_path);
}

static void
file_prefs_toggle_callback (GtkWidget *widget,
			    gpointer   data)
{
  int *val;

  if(initialising)
    return;

  if (data==&allow_resize_windows)
    allow_resize_windows = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&auto_save)
    auto_save = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&no_cursor_updating)
    no_cursor_updating = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&show_tool_tips)
    show_tool_tips = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&enable_rulers_on_start)
    enable_rulers_on_start=GTK_TOGGLE_BUTTON(widget)->active;
  else if (data==&enable_rgbm_painting)
  {
    enable_rgbm_painting = GTK_TOGGLE_BUTTON (widget)->active;
    channel_set_rgbm (enable_rgbm_painting); 
  } 
  else if (data==&enable_brush_dialog)
    enable_brush_dialog = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&enable_layer_dialog)
    enable_layer_dialog = GTK_TOGGLE_BUTTON (widget)->active;
 else if (data==&enable_paste_c_disp)
    enable_paste_c_disp = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&enable_tmp_saving)
    enable_tmp_saving = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&enable_channel_revert)
    enable_channel_revert = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&cubic_interpolation)
    cubic_interpolation = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&confirm_on_close)
    confirm_on_close = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&edit_stingy_memory_use)
    edit_stingy_memory_use = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&edit_install_cmap)
    edit_install_cmap = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&edit_cycled_marching_ants)
    edit_cycled_marching_ants = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data==&default_format)
    {
      default_format = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    } 
  else if (data==&default_precision)
    {
      default_precision = (Precision) gtk_object_get_user_data (GTK_OBJECT (widget));
    } 
  else if (data == &cms_bpc_by_default)
    cms_bpc_by_default = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &cms_manage_by_default)
    cms_manage_by_default = GTK_TOGGLE_BUTTON (widget)->active;
  else if (data == &cms_oyranos)
  {
    if(widget == cms_oyranos_wid[0])
    {
      char command[128];
      int err = 0;

      snprintf( command, 128, "export PATH=$PATH:%s%cbin; oyranos-config-fltk", GetDirPrefix(), G_DIR_SEPARATOR );
      err = system( command );

      if(err > 0x200)
        g_warning("\"oyranos-config-fltk\" seems not to be available. Error: %d", err);
      else if (err)
        printf("\"oyranos-config-fltk\" ?. error: %d\n",err);
    }
    else
    {
      cms_oyranos = GTK_TOGGLE_BUTTON (widget)->active;
      gtk_widget_set_sensitive (cms_oyranos_wid[0], cms_oyranos);
      gtk_widget_set_sensitive (cms_oyranos_wid[1], !cms_oyranos);
      gtk_widget_set_sensitive (cms_oyranos_wid[2], !cms_oyranos);
      gtk_widget_set_sensitive (cms_oyranos_wid[3], !cms_oyranos);
      gtk_widget_set_sensitive (cms_oyranos_wid[4], !cms_oyranos);
    }
  }
  else if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
      render_setup (transparency_type, transparency_size);
      layer_invalidate_previews (-1);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();
    }
}

static void
file_prefs_preview_size_callback (GtkWidget *widget,
                                  gpointer   data)
{
  lc_dialog_rebuild ((long)data);
  layer_select_update_preview_size ();
}

static void
file_prefs_text_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;

  val = data;
  *val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
file_prefs_string_callback (GtkWidget *widget,
			    gpointer   data)
{
  gchar **val;

  val = data;
  file_prefs_strset (val, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
file_prefs_string_menu_callback (GtkWidget *widget,
				 gpointer   data) 
{  char *value = gtk_object_get_data(GTK_OBJECT(widget), "value"); 
   gchar **setting = data;
   file_prefs_strset (setting, value);
}

static void
file_prefs_int_menu_callback(GtkWidget *widget,
			     gpointer   data) 
{  guint8 *value = gtk_object_get_data(GTK_OBJECT(widget), "value"); 
   guint8 *setting = data;
   *setting = *value;
}

static void
file_prefs_flags_menu_callback(GtkWidget *widget,
			     gpointer   data) 
{  guint16 *value = gtk_object_get_data(GTK_OBJECT(widget), "value"); 
   guint16 *setting = data;
   if (*value == 0
    || *value & cmsFLAGS_NOTPRECALC
    || *value & cmsFLAGS_NULLTRANSFORM
    || *value & cmsFLAGS_HIGHRESPRECALC
    || *value & cmsFLAGS_LOWRESPRECALC)
   {
     *setting &= ~cmsFLAGS_NOTPRECALC;
     *setting &= ~cmsFLAGS_NULLTRANSFORM;
     *setting &= ~cmsFLAGS_HIGHRESPRECALC;
     *setting &= ~cmsFLAGS_LOWRESPRECALC;
   }
   *setting |= *value;
}

static gint
file_prefs_gamut_alarm_callback (GtkWidget *widget, GdkEvent  *event) 
{	
  GdkColor color; 
  GdkGC *gc; 
  PixelRow r;    
  guint8 col[3];
  col[0] = gamut_alarm_red; col[1]= gamut_alarm_green; col[2]= gamut_alarm_blue;

  switch (event->type) 
  {
    case GDK_EXPOSE:
      gc = gdk_gc_new(widget->window);
      color.pixel = gdk_rgb_xpixel_from_rgb ((gamut_alarm_red << 16) | 
                                             (gamut_alarm_green << 8) | 
                                              gamut_alarm_blue);      
      gdk_gc_set_foreground (gc, &color);
      gdk_draw_rectangle (widget->window, gc, 1, 0, 0, 20, 20);
      break;
    case GDK_BUTTON_PRESS:      
      pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO),
                   (guchar *) col, 1);        
      if (!color_select)
      {
        color_select = color_select_new (&r, file_prefs_gamut_alarm_select_callback, widget, TRUE);        
      }
      else {
	color_select_show (color_select);
        color_select_set_color (color_select, &r, 1);
      }
        	
      break;
    default:

      break;
  }
  
  return FALSE;
}

static void
file_prefs_gamut_alarm_select_callback (PixelRow * color,
			    ColorSelectState state,
			    void *client_data)
{
  gfloat *data;
  GdkEvent event;
    
  switch (state)
  {
    case COLOR_SELECT_OK:
      data=(gfloat *) pixelrow_data(color);
      gamut_alarm_red=display_u8_from_float(data[0]); 
      gamut_alarm_green=display_u8_from_float(data[1]); 
      gamut_alarm_blue=display_u8_from_float(data[2]); 
            
      event.type=GDK_EXPOSE;
      gtk_widget_event(GTK_WIDGET(client_data), &event);
      gimage_invalidate_previews ();
      gdisplays_expose_full ();
      gdisplays_flush ();

    /* fall-through */
    case COLOR_SELECT_CANCEL:
      color_select_hide(color_select);
    default:
      break;
  }      
}

void
file_pref_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *out_frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *abox;
  GtkWidget *label;
  GtkWidget *radio_box;
  GtkWidget *entry;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *optionmenu;
  GtkWidget *notebook;
  GtkWidget *table;
  GtkWidget *alignment;
  GtkWidget *color_frame;  
  GtkWidget *color_box;      
  GSList *group;

  char buffer[32];
  int transparency_vals[] =
  {
    LIGHT_CHECKS,
    GRAY_CHECKS,
    DARK_CHECKS,
    WHITE_ONLY,
    GRAY_ONLY,
    BLACK_ONLY,
  };
  int check_vals[] =
  {
    SMALL_CHECKS,
    MEDIUM_CHECKS,
    LARGE_CHECKS,
  };
  char *transparencies[6];
  char *checks[3];
  struct {
    char *label;
    char **mpath;
  } dirs[9];
  struct {
    char *label;
    int size;
  } preview_sizes[4];
  int ntransparencies = sizeof (transparencies) / sizeof (transparencies[0]);
  int nchecks = sizeof (checks) / sizeof (checks[0]);
  int ndirs = sizeof(dirs) / sizeof (dirs[0]);
  int npreview_sizes = sizeof(preview_sizes) / sizeof (preview_sizes[0]);
  int i;
  guint8 *value;
  guint16 *value16;

  GSList *profile_file_names = 0;            
  GSList *iterator = 0;
  gchar *current_filename;
  CMSProfile *current_profile;
  CMSProfileInfo *current_profile_info;

  transparencies[0] = _("Light Checks");
  transparencies[1] = _("Mid-Tone Checks");
  transparencies[2] = _("Dark Checks");
  transparencies[3] = _("White Only");
  transparencies[4] = _("Gray Only");
  transparencies[5] = _("Black Only");
  checks[0] = _("Small Checks");
  checks[1] = _("Medium Checks");
  checks[2] = _("Large Checks");
  dirs[0].label = _("Temp folder:");          dirs[0].mpath = &edit_temp_path;
  dirs[1].label = _("Swap folder:");          dirs[1].mpath = &edit_swap_path;
  dirs[2].label = _("Brushes folder:");       dirs[2].mpath = &edit_brush_path;
  dirs[3].label = _("Gradients folder:");     dirs[3].mpath = &edit_gradient_path;
  dirs[4].label = _("Patterns folder:");      dirs[4].mpath = &edit_pattern_path;
  dirs[5].label = _("Palettes folder:");       dirs[5].mpath = &edit_palette_path;
  dirs[6].label = _("Frame Manager folder:"); dirs[6].mpath = &edit_frame_manager_path;
  dirs[7].label = _("Plug-ins folder:");       dirs[7].mpath = &edit_plug_in_path;
  dirs[8].label = _("ICC Profiles folder:");   dirs[8].mpath = &cms_profile_path;
  preview_sizes[0].label = _("None");   preview_sizes[0].size = 0;
  preview_sizes[1].label = _("Small");  preview_sizes[1].size = 32;
  preview_sizes[2].label = _("Medium"); preview_sizes[2].size = 64;
  preview_sizes[3].label = _("Large");  preview_sizes[3].size = 128;

  initialising = 1;
  if (!prefs_dlg)
    {
      if (edit_temp_path == NULL)
	{
	  /* first time dialog is opened - copy config vals to edit
             variables. */
	  edit_temp_path = file_prefs_strdup (temp_path);
          edit_default_brush = file_prefs_strdup (default_brush);
	  edit_swap_path = file_prefs_strdup (swap_path);
	  edit_brush_path = file_prefs_strdup (brush_path);
	  edit_pattern_path = file_prefs_strdup (pattern_path);
	  edit_palette_path = file_prefs_strdup (palette_path);
	  edit_frame_manager_path = file_prefs_strdup (frame_manager_path);
	  edit_plug_in_path = file_prefs_strdup (plug_in_path);
	  edit_gradient_path = file_prefs_strdup (gradient_path);
	  edit_stingy_memory_use = stingy_memory_use;
	  edit_tile_cache_size = tile_cache_size;
	  edit_install_cmap = install_cmap;
	  edit_cycled_marching_ants = cycled_marching_ants;
	}
      old_transparency_type = transparency_type;
      old_transparency_size = transparency_size;
      old_levels_of_undo = levels_of_undo;
      old_marching_speed = marching_speed;
      old_allow_resize_windows = allow_resize_windows;
      old_auto_save = auto_save;
      old_preview_size = preview_size;
      old_no_cursor_updating = no_cursor_updating;
      old_show_tool_tips = show_tool_tips;
      old_enable_rulers_on_start=enable_rulers_on_start;
      old_enable_rgbm_painting = enable_rgbm_painting; 
      old_enable_brush_dialog = enable_brush_dialog; 
      old_enable_layer_dialog = enable_layer_dialog; 
      old_enable_paste_c_disp = enable_paste_c_disp; 
      old_enable_tmp_saving = enable_tmp_saving; 
      old_enable_channel_revert = enable_channel_revert; 
      old_cubic_interpolation = cubic_interpolation;
      old_confirm_on_close = confirm_on_close;
      old_default_width = default_width;
      old_default_height = default_height;
      old_default_format = default_format;
      old_default_precision = default_precision;
      file_prefs_strset(&old_cms_default_image_profile_name, cms_default_image_profile_name);
      file_prefs_strset(&old_cms_default_proof_profile_name, cms_default_proof_profile_name);
      file_prefs_strset(&old_cms_workspace_profile_name, cms_workspace_profile_name);
      file_prefs_strset(&old_cms_display_profile_name, cms_display_profile_name);
      file_prefs_strset(&old_cms_profile_path,cms_profile_path);
      old_cms_default_intent = cms_default_intent;
      old_cms_default_flags = cms_default_flags;
      old_cms_open_action = cms_open_action;
      old_cms_mismatch_action = cms_mismatch_action;
      old_cms_bpc_by_default = cms_bpc_by_default;
      old_cms_manage_by_default = cms_manage_by_default;
      old_cms_oyranos = cms_oyranos;
      old_stingy_memory_use = edit_stingy_memory_use;
      old_tile_cache_size = edit_tile_cache_size;
      old_install_cmap = edit_install_cmap;
      old_cycled_marching_ants = edit_cycled_marching_ants;
      old_gamut_alarm_red = gamut_alarm_red;
      old_gamut_alarm_blue = gamut_alarm_blue;
      old_gamut_alarm_green = gamut_alarm_green;
      file_prefs_strset (&old_temp_path, edit_temp_path);
      file_prefs_strset (&old_default_brush, edit_default_brush);
      file_prefs_strset (&old_swap_path, edit_swap_path);
      file_prefs_strset (&old_brush_path, edit_brush_path);
      file_prefs_strset (&old_pattern_path, edit_pattern_path);
      file_prefs_strset (&old_palette_path, edit_palette_path);
      file_prefs_strset (&old_frame_manager_path, edit_frame_manager_path);
      file_prefs_strset (&old_plug_in_path, edit_plug_in_path);
      file_prefs_strset (&old_gradient_path, edit_gradient_path);

      prefs_dlg = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (prefs_dlg), "preferences", PROGRAM_NAME);
      gtk_window_set_title (GTK_WINDOW (prefs_dlg), _("Preferences"));
/*      gtk_widget_set_uposition(prefs_dlg, generic_window_x, generic_window_y); */
/*      layout_connect_window_position(prefs_dlg, &generic_window_x, &generic_window_y, FALSE); */
      minimize_register(prefs_dlg);

      /* handle the wm close signal */
      gtk_signal_connect (GTK_OBJECT (prefs_dlg), "delete_event",
			  GTK_SIGNAL_FUNC (file_prefs_delete_callback),
			  prefs_dlg);

      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (prefs_dlg)->action_area), 2);

      /* Action area */
      button = gtk_button_new_with_label (_("OK"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_ok_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Save"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_save_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Cancel"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_prefs_cancel_callback,
			  prefs_dlg);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dlg)->vbox),
			  notebook, TRUE, TRUE, 0);

      /* Display page */
      out_frame = gtk_frame_new (_("Display settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new (_("Image size"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      abox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (abox), 1);
      gtk_container_add (GTK_CONTAINER (frame), abox);
      gtk_widget_show (abox);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (abox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (_("Width:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 25, 0);
      sprintf (buffer, "%d", default_width);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                       GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &default_width);
      gtk_widget_show (entry);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 25, 0);
      sprintf (buffer, "%d", default_height);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
			  (GtkSignalFunc) file_prefs_text_callback,
                          &default_height);
      gtk_widget_show (entry);

      frame = gtk_frame_new (_("Image type"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      button = gtk_radio_button_new_with_label (NULL, _("RGB"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FORMAT_RGB);
      if (last_format == FORMAT_RGB)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_format);
      gtk_widget_show (button);
      button = gtk_radio_button_new_with_label (group, _("Grayscale"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FORMAT_GRAY);
      if (last_format == FORMAT_GRAY) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_format);
      gtk_widget_show (button);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
      
#if 1 
      frame = gtk_frame_new (_("Image precision"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      button = gtk_radio_button_new_with_label (NULL, _("U8"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_U8);
      if (default_precision == PRECISION_U8)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_precision);
      gtk_widget_show (button);

      button = gtk_radio_button_new_with_label (group, _("U16"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_U16);
      if (default_precision == PRECISION_U16) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_precision);
      gtk_widget_show (button);

      button = gtk_radio_button_new_with_label (group, _("FLOAT"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_FLOAT);
      if (default_precision == PRECISION_FLOAT) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_precision);
      gtk_widget_show (button);
      button = gtk_radio_button_new_with_label (group, _("FLOAT16"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_FLOAT16);
      if (default_precision == PRECISION_FLOAT16) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_precision);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
      button = gtk_radio_button_new_with_label (group, _("BFP"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) PRECISION_BFP);
      if (default_precision == PRECISION_BFP) 
	  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                         (GtkSignalFunc) file_prefs_toggle_callback,
			  &default_precision);
      gtk_widget_show (button);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
#endif

      label = gtk_label_new (_("Gamut alarm:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      color_frame=gtk_frame_new(NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (color_frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (hbox), color_frame, FALSE, FALSE, 0);
      gtk_widget_show (color_frame);

      color_box=gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (color_box), 20, 20);

      gtk_widget_set_events (color_box, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
      gtk_signal_connect (GTK_OBJECT (color_box), "event",
		      (GtkSignalFunc) file_prefs_gamut_alarm_callback,
		      NULL);
      gtk_container_add (GTK_CONTAINER (color_frame), color_box);
      gtk_widget_show (color_box);
              
      label = gtk_label_new (_("Preview size:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      for (i = 0; i < npreview_sizes; i++)
        {
          menuitem = gtk_menu_item_new_with_label (preview_sizes[i].label);
	  gtk_menu_append (GTK_MENU (menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",

			      (GtkSignalFunc) file_prefs_preview_size_callback,
			      (gpointer)((long)preview_sizes[i].size));
	  gtk_widget_show (menuitem);
	}
      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_box_pack_start (GTK_BOX (hbox), optionmenu, TRUE, TRUE, 0);
      gtk_widget_show (optionmenu);
      for (i = 0; i < npreview_sizes; i++)
	if (preview_size==preview_sizes[i].size)
	  gtk_option_menu_set_history(GTK_OPTION_MENU (optionmenu),i);

      button = gtk_check_button_new_with_label(_("Cubic interpolation"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cubic_interpolation);
      gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cubic_interpolation);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      frame = gtk_frame_new (_("Transparency Type"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < ntransparencies; i++)
        {
	  button = gtk_radio_button_new_with_label (group, transparencies[i]);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) transparency_vals[i]));
          if (transparency_vals[i] == transparency_type)
            gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_type);
          gtk_widget_show (button);
        }
      frame = gtk_frame_new (_("Check Size"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      radio_box = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
      gtk_container_add (GTK_CONTAINER (frame), radio_box);
      gtk_widget_show (radio_box);

      group = NULL;
      for (i = 0; i < nchecks; i++)
        {
          button = gtk_radio_button_new_with_label (group, checks[i]);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
          gtk_object_set_user_data (GTK_OBJECT (button),
				    (gpointer) ((long) check_vals[i]));
          if (check_vals[i] == transparency_size)
            gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              (GtkSignalFunc) file_prefs_toggle_callback,
                              &transparency_size);
          gtk_widget_show (button);
        }


      label = gtk_label_new (_("Display"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Interface */
      out_frame = gtk_frame_new (_("Interface settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Levels of undo:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", levels_of_undo);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &levels_of_undo);
      gtk_widget_show (entry);

      button = gtk_check_button_new_with_label(_("Resize window on zoom"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   allow_resize_windows);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &allow_resize_windows);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Show Rulers By Default"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_rulers_on_start);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &enable_rulers_on_start);
      gtk_widget_show (button);

      /* Don't show the Auto-save button until we really
	 have auto-saving in the gimp.

	 button = gtk_check_button_new_with_label(_("Auto save"));
	 gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   auto_save);
	 gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	 gtk_signal_connect (GTK_OBJECT (button), "toggled",
	                           (GtkSignalFunc) file_prefs_toggle_callback,
                                   &auto_save);
         gtk_widget_show (button);
      */

      button = gtk_check_button_new_with_label(_("Disable cursor updating"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   no_cursor_updating);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &no_cursor_updating);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Show tool tips"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   show_tool_tips);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &show_tool_tips);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Marching ants speed:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", marching_speed);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &marching_speed);
      gtk_widget_show (entry);

      label = gtk_label_new (_("Interface"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Environment */
      out_frame = gtk_frame_new (_("Environment settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      button = gtk_check_button_new_with_label(_("Conservative memory usage"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   stingy_memory_use);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &edit_stingy_memory_use);
      gtk_widget_show (button);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Tile cache size (bytes):"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 75, 0);
      sprintf (buffer, "%d", old_tile_cache_size);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                          (GtkSignalFunc) file_prefs_text_callback,
                          &edit_tile_cache_size);
      gtk_widget_show (entry);

      button = gtk_check_button_new_with_label(_("Install colormap (8-bit only)"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
				   install_cmap);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_install_cmap);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_check_button_new_with_label(_("Colormap cycling (8-bit only)"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cycled_marching_ants);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &edit_cycled_marching_ants);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      if (g_visual->depth != 8)
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
      gtk_widget_show (button);

      label = gtk_label_new (_("Environment"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Directories */
      out_frame = gtk_frame_new (_("Folder settings"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      table = gtk_table_new (ndirs+1, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);

      for (i = 0; i < ndirs; i++)
	{
	  label = gtk_label_new (dirs[i].label);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1,
                            GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

          entry = gtk_entry_new ();
          gtk_widget_set_usize (entry, 25, 0);
          gtk_entry_set_text (GTK_ENTRY (entry), *(dirs[i].mpath));
	  gtk_signal_connect (GTK_OBJECT (entry), "changed",
			      (GtkSignalFunc) file_prefs_string_callback,
			      dirs[i].mpath);
          gtk_table_attach (GTK_TABLE (table), entry, 1, 2, i, i+1,
                            GTK_EXPAND | GTK_FILL, 0, 0, 0);
          gtk_widget_show (entry);
          if(strcmp(dirs[i].label,_("ICC Profiles folder:")) == 0)
          {
            cms_oyranos_wid[3] = entry;
            cms_oyranos_wid[4] = label;
          }
 	}

      label = gtk_label_new (_("Folders"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      /* Global Settings */
      out_frame = gtk_frame_new (_("Global Setting"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);
      
      button = gtk_check_button_new_with_label(_("Enable RGBM painting"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_rgbm_painting);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_rgbm_painting);
      gtk_widget_show (button);
      
      /*
      button = gtk_check_button_new_with_label(_("Show brush dialog"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_brush_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_brush_dialog);
      gtk_widget_show (button);
      button = gtk_check_button_new_with_label(_("Show layer dialog"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_layer_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_layer_dialog);
      gtk_widget_show (button);
      */
      
      button = gtk_check_button_new_with_label(_("Enable paste in center of display"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_paste_c_disp);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_paste_c_disp);
      gtk_widget_show (button);
     
      button = gtk_check_button_new_with_label(_("Enable tmp saving"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_tmp_saving);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_tmp_saving);
      gtk_widget_show (button);
      
      button = gtk_check_button_new_with_label(_("Enable Channel Revert"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   enable_channel_revert);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  (GtkSignalFunc) file_prefs_toggle_callback,
			  &enable_channel_revert);
      gtk_widget_show (button);

      label = gtk_label_new (_("Default Brush"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      /*gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1,
	GTK_FILL, GTK_FILL, 0, 0);
       */
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_widget_set_usize (entry, 20, 0);
      gtk_entry_set_text (GTK_ENTRY (entry), *(&edit_default_brush));
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
	  (GtkSignalFunc) file_prefs_string_callback,
	  (&edit_default_brush));
      /*gtk_table_attach (GTK_TABLE (table), entry, 1, 2, i, i+1,
	GTK_EXPAND | GTK_FILL, 0, 0, 0);
       */
      gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
      gtk_widget_show (entry);


      label = gtk_label_new (_("Global Settings"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);

      
      /* Color management */
      out_frame = gtk_frame_new (_("Color Management"));
      gtk_container_border_width (GTK_CONTAINER (out_frame), 10);
      gtk_widget_set_usize (out_frame, 320, 200);
      gtk_widget_show (out_frame);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 5);
      gtk_container_add (GTK_CONTAINER (out_frame), vbox);
      gtk_widget_show (vbox);

      /* profile setting box */
      frame = gtk_frame_new (_("General Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      abox = gtk_vbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (abox), 5);
      gtk_container_add (GTK_CONTAINER (frame), abox);
      gtk_widget_show (abox);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (abox), table, TRUE, FALSE, 0);
      gtk_widget_show (table);

#ifdef HAVE_OY
      /* whether to use Oyranos */
      button = gtk_check_button_new_with_label(_("Copy Oyranos Settings"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cms_oyranos);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cms_oyranos);
      gtk_table_attach (GTK_TABLE (table), button,0, 1, 0, 1,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_widget_show(button);

      /* call Oyranos dialog */
      cms_oyranos_wid[0] = button =
                            gtk_button_new_with_label(_("Oyranos Config"));
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cms_oyranos);
      gtk_table_attach (GTK_TABLE (table), button,1, 2, 0, 1,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_widget_show(button);
      if(!cms_oyranos)
        gtk_widget_set_sensitive (button, FALSE);
#endif

      /* whether to manage by default */
      button = gtk_check_button_new_with_label(_("Colormanage new displays By Default"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cms_manage_by_default);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cms_manage_by_default);
      gtk_table_attach (GTK_TABLE (table), button,0, 2, 1, 2,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 5);
      gtk_widget_show(button);

      /* profile setting box */
      cms_oyranos_wid[1] = frame = gtk_frame_new (_("Profile Settings"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      abox = gtk_vbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (abox), 5);
      gtk_container_add (GTK_CONTAINER (frame), abox);
      gtk_widget_show (abox);

      table = gtk_table_new (7, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (abox), table, TRUE, FALSE, 0);
      gtk_widget_show (table);

      /* image profiles */      
      label = gtk_label_new (_("Assumed Image Profile:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      menuitem = gtk_menu_item_new_with_label (_("[none]"));
      gtk_object_set_data(GTK_OBJECT(menuitem), "value", NULL); 
      gtk_menu_append (GTK_MENU (menu), menuitem);	
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_string_menu_callback,
			 (gpointer)&cms_default_image_profile_name );
      gtk_menu_set_active(GTK_MENU(menu), 0);

      profile_file_names = cms_read_standard_profile_dirs(CMS_ANY_PROFILECLASS);
      iterator = profile_file_names;
      for (i=1; iterator != NULL; i++)	
      {   current_filename = iterator->data;
	  current_profile = cms_get_profile_from_file(current_filename);
          current_profile_info = cms_get_profile_info(current_profile);
	  menuitem = gtk_menu_item_new_with_label (current_profile_info->description);
	  gtk_menu_append (GTK_MENU (menu), menuitem);	      
	  gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)current_filename, g_free);      
	  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			     (GtkSignalFunc) file_prefs_string_menu_callback,
			     (gpointer)&cms_default_image_profile_name );
	  /* select the current profile*/
	  if (file_prefs_strcmp(current_filename, cms_default_image_profile_name) == 0)
	  {   gtk_menu_set_active(GTK_MENU(menu), i);
	  }
	  
	  cms_return_profile(current_profile);

	  iterator = g_slist_next(iterator);          
      }
      if (profile_file_names != NULL) 
      {   g_slist_free(profile_file_names);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      gtk_table_attach (GTK_TABLE (table), optionmenu, 1, 2, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show_all (optionmenu);    


      /* editing/workspace profiles */
      label = gtk_label_new (_("Editing Profile:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      menuitem = gtk_menu_item_new_with_label (_("[none]"));
      gtk_object_set_data(GTK_OBJECT(menuitem), "value", NULL); 
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_string_menu_callback,
			 (gpointer)&cms_workspace_profile_name );     
      gtk_menu_append (GTK_MENU (menu), menuitem);	
      gtk_menu_set_active(GTK_MENU(menu), 0);

      profile_file_names = cms_read_standard_profile_dirs(CMS_ANY_PROFILECLASS);      
      iterator = profile_file_names;      
      for (i=1; iterator != NULL; i++)
      {   current_filename = iterator->data;
	  current_profile = cms_get_profile_from_file(current_filename);
          current_profile_info = cms_get_profile_info(current_profile);
	  menuitem = gtk_menu_item_new_with_label (current_profile_info->description);
	  gtk_menu_append (GTK_MENU (menu), menuitem);	      
	  gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", current_filename, g_free);      
	  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			     (GtkSignalFunc) file_prefs_string_menu_callback,
			     (gpointer)&cms_workspace_profile_name );
	  /* select the current profile*/
	  if (file_prefs_strcmp(current_filename, cms_workspace_profile_name) == 0)
	  {   gtk_menu_set_active(GTK_MENU(menu), i);
	  }
	  
	  cms_return_profile(current_profile);

	  iterator = g_slist_next(iterator);          
      }
      if (profile_file_names != NULL) 
      {   g_slist_free(profile_file_names);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      gtk_table_attach (GTK_TABLE (table), optionmenu, 1, 2, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show_all (optionmenu);    


      /* display profiles */
      label = gtk_label_new (_("Display Profile:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      menuitem = gtk_menu_item_new_with_label (_("[none]"));
      gtk_object_set_data(GTK_OBJECT(menuitem), "value", NULL); 
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_string_menu_callback,
			 (gpointer)&cms_display_profile_name );
      gtk_menu_append (GTK_MENU (menu), menuitem);	
      gtk_menu_set_active(GTK_MENU(menu), 0);

      profile_file_names = cms_read_standard_profile_dirs(icSigDisplayClass);      
      iterator = profile_file_names;
      for (i=1; iterator != NULL; i++)
      {   current_filename = iterator->data;
	  current_profile = cms_get_profile_from_file(current_filename);
          current_profile_info = cms_get_profile_info(current_profile);
	  menuitem = gtk_menu_item_new_with_label (current_profile_info->description);
	  gtk_menu_append (GTK_MENU (menu), menuitem);	      
	  gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)current_filename, g_free);      
	  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			     (GtkSignalFunc) file_prefs_string_menu_callback,
			     (gpointer)&cms_display_profile_name );
	  /* select the current profile*/
	  if (file_prefs_strcmp(current_filename, cms_display_profile_name) == 0)
	  {   gtk_menu_set_active(GTK_MENU(menu), i);
	  }
	  
	  cms_return_profile(current_profile);

	  iterator = g_slist_next(iterator);          
      }
      if (profile_file_names != NULL) 
      {   g_slist_free(profile_file_names);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      gtk_table_attach (GTK_TABLE (table), optionmenu, 1, 2, 2, 3,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show_all (optionmenu);

      /* Proof profile */
      label = gtk_label_new (_("Proof Profile:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      
      menu = gtk_menu_new ();
      menuitem = gtk_menu_item_new_with_label (_("[none]"));
      gtk_object_set_data(GTK_OBJECT(menuitem), "value", NULL);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_string_menu_callback,
			 (gpointer)&cms_default_proof_profile_name );
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_menu_set_active(GTK_MENU(menu), 0);

      profile_file_names = cms_read_standard_profile_dirs(CMS_ANY_PROFILECLASS);
      iterator = profile_file_names;
      for (i=1; iterator != NULL; i++)
      {   current_filename = iterator->data;
          current_profile = cms_get_profile_from_file(current_filename);
          current_profile_info = cms_get_profile_info(current_profile);
          menuitem = gtk_menu_item_new_with_label (current_profile_info->description);
          gtk_menu_append (GTK_MENU (menu), menuitem);
          gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", current_filename, g_free);
          gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
                             (GtkSignalFunc) file_prefs_string_menu_callback,
                             (gpointer)&cms_default_proof_profile_name );
	  /* select the current profile*/
	  if (file_prefs_strcmp(current_filename, cms_default_proof_profile_name) == 0)
	  {   gtk_menu_set_active(GTK_MENU(menu), i);
	  }
	  
	  cms_return_profile(current_profile);

	  iterator = g_slist_next(iterator);
      }
      if (profile_file_names != NULL)
      {   g_slist_free(profile_file_names);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      gtk_table_attach (GTK_TABLE (table), optionmenu, 1, 2, 3, 4,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show_all (optionmenu);


      /* default intent */
      label = gtk_label_new (_("Default Rendering Intent:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show(label);
      
      menu = gtk_menu_new (); 
      menuitem = gtk_menu_item_new_with_label (_("Perceptual"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = INTENT_PERCEPTUAL;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_default_intent );
      if (cms_default_intent == INTENT_PERCEPTUAL)
      {   gtk_menu_set_active(GTK_MENU(menu), 0);
      }

      menuitem = gtk_menu_item_new_with_label (_("Relative Colorimetric"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = INTENT_RELATIVE_COLORIMETRIC;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_default_intent );
      if (cms_default_intent == INTENT_RELATIVE_COLORIMETRIC)
      {   gtk_menu_set_active(GTK_MENU(menu), 1);
      }

      menuitem = gtk_menu_item_new_with_label (_("Saturation"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = INTENT_SATURATION;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_default_intent );
      if (cms_default_intent == INTENT_SATURATION)
      {   gtk_menu_set_active(GTK_MENU(menu), 2);
      }

      menuitem = gtk_menu_item_new_with_label (_("Absolute Colorimetric"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = INTENT_ABSOLUTE_COLORIMETRIC;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_default_intent );
      if (cms_default_intent == INTENT_ABSOLUTE_COLORIMETRIC)
      {   gtk_menu_set_active(GTK_MENU(menu), 3);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_widget_set_usize(optionmenu, 170,25);
      alignment = gtk_alignment_new(0,0,0,0);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 4, 5,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_container_add(GTK_CONTAINER(alignment), optionmenu);
      gtk_widget_show_all(alignment);

      /* default cms flags */
      label = gtk_label_new (_("Default Transform Calculation:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show(label);
      
      menu = gtk_menu_new (); 
      menuitem = gtk_menu_item_new_with_label (_("CMM default"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value16 = g_new(guint16, 1);
      *value16 = 0;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value16, g_free);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_flags_menu_callback,
			 &cms_default_flags );
      if (cms_default_flags & 0)
      {   gtk_menu_set_active(GTK_MENU(menu), 0);
      }

      menuitem = gtk_menu_item_new_with_label (_("dont Precalculate"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value16 = g_new(guint16, 1);
      *value16 = cmsFLAGS_NOTPRECALC;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value16, g_free);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_flags_menu_callback,
			 &cms_default_flags );
      if (cms_default_flags & cmsFLAGS_NOTPRECALC)
      {   gtk_menu_set_active(GTK_MENU(menu), 1);
      }

      menuitem = gtk_menu_item_new_with_label (_("High Resolution"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value16 = g_new(guint16, 1);
      *value16 = cmsFLAGS_HIGHRESPRECALC;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value16, g_free);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_flags_menu_callback,
			 &cms_default_flags );
      if (cms_default_flags & cmsFLAGS_HIGHRESPRECALC)
      {   gtk_menu_set_active(GTK_MENU(menu), 2);
      }

      menuitem = gtk_menu_item_new_with_label (_("Low Resolution"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value16 = g_new(guint16, 1);
      *value16 = cmsFLAGS_LOWRESPRECALC;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value16, g_free);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_flags_menu_callback,
			 &cms_default_flags );
      if (cms_default_flags & cmsFLAGS_LOWRESPRECALC)
      {   gtk_menu_set_active(GTK_MENU(menu), 3);
      }
      /* crashes CinePaint */
      #if 0
      menuitem = gtk_menu_item_new_with_label (_("No Conversion"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value16 = g_new(guint16, 1);
      *value16 = cmsFLAGS_NULLTRANSFORM;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value16, g_free);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_flags_menu_callback,
			 &cms_default_flags );
      if (cms_default_flags & cmsFLAGS_NULLTRANSFORM)
      {   gtk_menu_set_active(GTK_MENU(menu), 4);
      }
      #endif

      optionmenu = gtk_option_menu_new ();
      gtk_widget_set_usize(optionmenu, 250,25);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_widget_set_usize(optionmenu, 170,25);
      alignment = gtk_alignment_new(0,0,0,0);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 5, 6,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_container_add(GTK_CONTAINER(alignment), optionmenu);
      gtk_widget_show_all(alignment);

      /* whether to blackpoint compensate by default */
      button = gtk_check_button_new_with_label(_("Black Point Compensation by default"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button),
                                   cms_bpc_by_default);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
                          (GtkSignalFunc) file_prefs_toggle_callback,
                          &cms_bpc_by_default);
      gtk_table_attach (GTK_TABLE (table), button,0, 2, 6, 7,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_widget_show(button);

      /* default behaviour box */
      cms_oyranos_wid[2] = frame = gtk_frame_new (_("Default Behaviour"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      abox = gtk_vbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (abox), 5);
      gtk_container_add (GTK_CONTAINER (frame), abox);
      gtk_widget_show (abox);

      table = gtk_table_new (4, 2, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (abox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      /* upon opening image */
      label = gtk_label_new (_("When Opening Image:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show(label);
      
      menu = gtk_menu_new (); 
      menuitem = gtk_menu_item_new_with_label (_("Assign No Profile"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_ASSIGN_NONE;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_open_action );
      if (cms_open_action == CMS_ASSIGN_NONE)
      {   gtk_menu_set_active(GTK_MENU(menu), 0);
      }

      menuitem = gtk_menu_item_new_with_label (_("Assign Assumed Image Profile"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_ASSIGN_DEFAULT;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_open_action );
      if (cms_open_action == CMS_ASSIGN_DEFAULT)
      {   gtk_menu_set_active(GTK_MENU(menu), 1);
      }

      menuitem = gtk_menu_item_new_with_label (_("Prompt for Profile"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_ASSIGN_PROMPT;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_open_action);
      if (cms_open_action == CMS_ASSIGN_PROMPT)
      {   gtk_menu_set_active(GTK_MENU(menu), 2);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_widget_set_usize(optionmenu, 200,25);
      alignment = gtk_alignment_new(0,0,0,0);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 0, 1,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 5);
      gtk_container_add(GTK_CONTAINER(alignment), optionmenu);
      gtk_widget_show_all(alignment);

      /* colorspace mismatch */
      label = gtk_label_new (_("Editing profile missmatch:"));
      gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
      /* gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gtk_widget_set_usize(label, 170, 30); */
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show(label);
      
      menu = gtk_menu_new (); 
      menuitem = gtk_menu_item_new_with_label (_("Prompt"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_MISMATCH_PROMPT;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_mismatch_action );
      if (cms_mismatch_action == CMS_MISMATCH_PROMPT)
      {   gtk_menu_set_active(GTK_MENU(menu), 0);
      }

      menuitem = gtk_menu_item_new_with_label (_("Convert Automatically"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_MISMATCH_AUTO_CONVERT;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_mismatch_action );
      if (cms_mismatch_action == CMS_MISMATCH_AUTO_CONVERT)
      {   gtk_menu_set_active(GTK_MENU(menu), 1);
      }

      menuitem = gtk_menu_item_new_with_label (_("Keep Image Profile"));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      value = g_new(guint8, 1);
      *value = CMS_MISMATCH_KEEP;
      gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 (GtkSignalFunc) file_prefs_int_menu_callback,
			 &cms_mismatch_action );
      if (cms_mismatch_action == CMS_MISMATCH_KEEP)
      {   gtk_menu_set_active(GTK_MENU(menu), 2);
      }

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
      gtk_widget_set_usize(optionmenu, 200,25);
      alignment = gtk_alignment_new(0,0,0,0);
      gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 1, 2,
			GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 5);
      gtk_container_add(GTK_CONTAINER(alignment), optionmenu);
      gtk_widget_show_all(alignment);


      label = gtk_label_new (_("Color Management"));
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook), out_frame, label);
      gtk_widget_show (notebook);	  

      if(cms_oyranos)
      {
        gtk_widget_set_sensitive (cms_oyranos_wid[1], FALSE);
        gtk_widget_set_sensitive (cms_oyranos_wid[2], FALSE);
        gtk_widget_set_sensitive (cms_oyranos_wid[3], FALSE);
        gtk_widget_set_sensitive (cms_oyranos_wid[4], FALSE);
      }
    }

  gtk_widget_show (prefs_dlg);      
  initialising = 0;
}

void
file_close_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_close_window (gdisp, FALSE);
}

void
file_quit_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  app_exit (0);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_cut (gdisp);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_copy (gdisp);
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 0);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 1);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_clear (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_fill (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_stroke (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_undo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_redo (gdisp->gimage);
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_cut (gdisp);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_copy (gdisp);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_paste (gdisp);
}

void
select_toggle_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  selection_hide (gdisp->select, (void *) gdisp);
  gdisplays_flush ();
}

void
select_invert_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
select_all_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_all (gdisp->gimage);
  gdisplays_flush ();
}

void
select_none_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_none (gdisp->gimage);
  gdisplays_flush ();
}

void
select_float_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_float (gdisp->gimage, gimage_active_drawable (gdisp->gimage), 0, 0);
  gdisplays_flush ();
}

void
select_sharpen_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_sharpen (gdisp->gimage);
  gdisplays_flush ();
}

void
select_border_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_border_radius);
  query_string_box (_("Border Selection"), _("Border selection by:"), initial,
		    gimage_mask_border_callback, (gpointer)(intptr_t) gdisp->gimage->ID);
}

void
select_feather_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%f", gimage_mask_feather_radius);
  query_string_box (_("Feather Selection"), _("Feather selection by:"), initial,
		    gimage_mask_feather_callback, (gpointer)(intptr_t) gdisp->gimage->ID);
}

void
select_grow_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_grow_pixels);
  query_string_box (_("Grow Selection"), _("Grow selection by:"), initial,
		    gimage_mask_grow_callback, (gpointer)(intptr_t) gdisp->gimage->ID);
}

void
select_shrink_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_shrink_pixels);
  query_string_box (_("Shrink Selection"), _("Shrink selection by:"), initial,
		    gimage_mask_shrink_callback, (gpointer)(intptr_t) gdisp->gimage->ID);
}

void
select_by_color_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
#if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) BY_COLOR_SELECT].toolbar_position]);
#endif  
  by_color_select_initialize ((void *) gdisp->gimage);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
select_save_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gimage_mask_save (gdisp->gimage);
  gdisplays_flush ();
}

void 
view_colormanage_display_cmd_callback(GtkWidget *widget, gpointer ptr)
{   GDisplay * display = gdisplay_active ();

    gboolean new_val = GTK_CHECK_MENU_ITEM (widget)->active ? TRUE : FALSE;    
    if ((display != NULL) && (new_val != gdisplay_is_colormanaged(display)))
    {
#if 0
        /* GRAY images are not supported - unset */
        if (tag_format (gimage_tag (display->gimage)) == FORMAT_GRAY &&
            new_val != 0)
            new_val = 0;
#endif
        if (gdisplay_set_colormanaged(display, new_val) == FALSE)	 
        {   g_message(_("Cannot colormanage this display.\nEither image or display profile are undefined.\nYou can assign an image profile via Image/Assign ICC Profile...\nYou can set a display profile via File/Preferences/Color Management"));
	}
    }
}

void 
view_bpc_display_cmd_callback(GtkWidget *widget, gpointer ptr)
{
    GDisplay * display;
    gboolean new_val;

    display = gdisplay_active ();

    new_val = GTK_CHECK_MENU_ITEM (widget)->active ? TRUE : FALSE;    
    if ((display != NULL) && (new_val != (gdisplay_has_bpc(display)?1:0)))
    {   if (gdisplay_set_bpc(display, new_val) == FALSE)	 
        {   g_message(_("Cannot blackpoint compensate this display.\nEither image or display profile are undefined.\nYou can assign an image profile via Image/Assign ICC Profile...\nYou can set a display profile via File/Preferences/Color Management"));
        }
    }
}

void 
view_paper_simulation_display_cmd_callback(GtkWidget *widget, gpointer ptr)
{
    GDisplay * display;
    gboolean new_val;

    display = gdisplay_active ();

    new_val = GTK_CHECK_MENU_ITEM (widget)->active ? TRUE : FALSE;    
    if ((display != NULL) && (new_val != (gdisplay_has_paper_simulation(display)?1:0)))
    {   if (gdisplay_set_paper_simulation(display, new_val) == FALSE)	 
        {   g_message(_("Cannot simulate paper on this display.\nEither image or display profile are undefined.\nYou can assign an image profile via Image/Assign ICC Profile...\nYou can set a display profile via File/Preferences/Color Management"));
        }
    }
}

void 
view_proof_display_cmd_callback(GtkWidget *widget, gpointer ptr)
{
    GDisplay * display;
    gboolean new_val;

    display = gdisplay_active ();

    new_val = GTK_CHECK_MENU_ITEM (widget)->active ? TRUE : FALSE;    
    if ((display != NULL) && (new_val != (gdisplay_is_proofed(display)?1:0)))
    {   if (gdisplay_set_proofed(display, new_val) == FALSE)	 
        {   g_message(_("Cannot proof this display.\nEither image, proof or display profile are undefined.\nYou can assign an image profile via Image/Assign ICC Profile...\nYou can set a display profile via File/Preferences/Color Management"));
        }
    }
}

void 
view_gamutcheck_display_cmd_callback(GtkWidget *widget, gpointer ptr)
{   GDisplay * display = gdisplay_active ();

    gboolean new_val = GTK_CHECK_MENU_ITEM (widget)->active ? TRUE : FALSE;    
    if ((display != NULL) && (new_val != (gdisplay_is_gamutchecked(display))))
    {
        if(!(gdisplay_is_proofed(display)?1:0))
            gdisplay_set_proofed(display, 1);	 
        if (gdisplay_set_gamutchecked(display, new_val) == FALSE)	 
        {   g_message(_("Cannot proof this display.\nEither image, proof or display profile are undefined.\nYou can assign an image profile via Image/Assign ICC Profile...\nYou can set a display profile via File/Preferences/Color Management"));
        }
    }
}

void 
view_rendering_intent_cmd_callback(GtkWidget *widget, gpointer ptr, 
				   guint callback_action)
{   GDisplay *display = gdisplay_active();
    if(GTK_CHECK_MENU_ITEM (widget)->active)
      gdisplay_set_cms_intent(display, callback_action); 
}

void 
view_look_profiles_cmd_callback(GtkWidget *widget, gpointer ptr)
{   GDisplay *display = gdisplay_active();
    look_profile_gui(display);
}

void view_zoom_bookmark_save_cb(GtkWidget *widget, gpointer ptr)
{
   zoom_bookmark_save_requestor();
}

void view_zoom_bookmark_load_cb(GtkWidget *widget, gpointer ptr)
{
   zoom_bookmark_load_requestor();
}

void view_zoom_bookmark0_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  zoom_bookmark_jump_to(&zoom_bookmarks[0], gdisp);
}

void view_zoom_bookmark4_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  zoom_bookmark_jump_to(&zoom_bookmarks[4], gdisp);
}

void view_zoom_bookmark1_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  zoom_bookmark_jump_to(&zoom_bookmarks[1], gdisp);
}

void view_zoom_bookmark2_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  zoom_bookmark_jump_to(&zoom_bookmarks[2], gdisp);
}

void view_zoom_bookmark3_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  zoom_bookmark_jump_to(&zoom_bookmarks[3], gdisp);
}

void 
view_pan_zoom_window_cb(
   GtkWidget *widget, 
   gpointer ptr)
{
   zoom_control_open();
}

void
view_zoomin_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMIN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMOUT);
}

static void
view_zoom_val (GtkWidget *widget,
	       gpointer   client_data,
	       int        val)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, val);
}

void
view_zoom_16_1_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 1601);
}

void
view_zoom_8_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 801);
}

void
view_zoom_4_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 401);
}

void
view_zoom_2_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 201);
}

void
view_zoom_1_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 101);
}

void
view_zoom_1_2_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 102);
}

void
view_zoom_1_4_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 104);
}

void
view_zoom_1_8_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 108);
}

void
view_zoom_1_16_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 116);
}

void
view_window_info_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  if (! gdisp->window_info_dialog)
    gdisp->window_info_dialog = info_window_create ((void *) gdisp);

  info_dialog_popup (gdisp->window_info_dialog);
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  /* This routine use promiscuous knowledge of gtk internals
   *  in order to hide and show the rulers "smoothly". This
   *  is kludgy and a hack and may break if gtk is changed
   *  internally.
   */

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_hide (gdisp->origin);
	  gtk_widget_hide (gdisp->hrule);
	  gtk_widget_hide (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_show (gdisp->origin);
	  gtk_widget_show (gdisp->hrule);
	  gtk_widget_show (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  int old_val;

  gdisp = gdisplay_active ();

  old_val = gdisp->draw_guides;
  gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

  if ((old_val != gdisp->draw_guides) && gdisp->gimage->guides)
    {
      gdisplay_expose_full (gdisp);
      gdisplays_flush ();
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_new_view (gdisp);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  if (gdisp)
    shrink_wrap_display (gdisp);
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_equalize ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_invert ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_posterize_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) POSTERIZE].toolbar_position]);
  #endif
  posterize_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
image_threshold_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) THRESHOLD].toolbar_position]);
  #endif
  threshold_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
image_color_balance_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) COLOR_BALANCE].toolbar_position]);
  #endif
  color_balance_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);

}

void
image_color_correction_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  color_correction_gui(gdisp);
}

void
image_brightness_contrast_cmd_callback (GtkWidget *widget,
					gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) BRIGHTNESS_CONTRAST].toolbar_position]);
  #endif
  brightness_contrast_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
image_gamma_expose_cmd_callback (GtkWidget *widget,
					gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) GAMMA_EXPOSE].toolbar_position]);
  #endif
  gamma_expose_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
view_expose_hdr_cmd_callback (GtkWidget *widget,
					gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
# if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) EXPOSE_IMAGE].toolbar_position]);
#endif
  expose_view_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}



void
image_hue_saturation_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) HUE_SATURATION].toolbar_position]);
  #endif
  hue_saturation_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void image_curves_cmd_callback (GtkWidget *widget,gpointer client_data)
{
#if 1
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
/* BUG: GtkWidget* = 0 with 32-bit image, segv 
	all the gtk_widget_activate calls need to be armored --rsr */
#if 0	
  gtk_widget_activate (tool_widgets[tool_info[(int) CURVES].toolbar_position]);
#endif  
  curves_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
#endif
}

void image_levels_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) LEVELS].toolbar_position]);
  #endif
  levels_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_desaturate ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void channel_ops_duplicate_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_duplicate ((void *) gdisp->gimage);
}

void channel_ops_offset_cmd_callback (GtkWidget *widget,
				 gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_offset ((void *) gdisp->gimage);
}

void image_convert_rgb_cmd_callback (GtkWidget *widget,
				gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_rgb ((void *) gdisp->gimage);
}

void image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_grayscale ((void *) gdisp->gimage);
}

void image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_indexed ((void *) gdisp->gimage);
}

void image_convert_depth_cmd_callback (GtkWidget *widget,
				  gpointer client_data, 
				  guint callback_action)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_image_precision((void *) gdisp->gimage, (Precision)callback_action);
  /* convert_to_grayscale ((void *) gdisp->gimage); */
}

void image_convert_colorspace_cmd_callback (GtkWidget *widget,
					    gpointer client_data, 
					    guint callback_action)
{   GDisplay * gdisp;

    gdisp = gdisplay_active ();
    if (gimage_get_cms_profile(gdisp->gimage) == NULL)
    {   message_box(_("Cannot convert this image, because it has not profile assigned. Assign one first via Image>Assign ICC Profile..."), NULL, NULL);
        return;
    }
    cms_convert_dialog(gdisp->gimage);
}

void image_assign_cms_profile_cmd_callback (GtkWidget *widget,
					    gpointer client_data, 
					    guint callback_action)
{   GDisplay * gdisp;

    gdisp = gdisplay_active ();
    if(callback_action == ICC_IMAGE_PROFILE)
      cms_assign_dialog(gdisp->gimage, ICC_IMAGE_PROFILE);
    else if(callback_action == ICC_PROOF_PROFILE)
      cms_assign_dialog(gdisp->gimage, ICC_PROOF_PROFILE);
}

void image_resize_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_resize_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_resize;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_resize = (ImageResize *) g_malloc_zero (sizeof (ImageResize));
  image_resize->gimage_id = gdisp->gimage->ID;
  image_resize->resize = resize_widget_new (ResizeWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_resize->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_resize->shell), "image_resize", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (image_resize->shell), _("Image Resize"));
  gtk_window_set_policy (GTK_WINDOW (image_resize->shell), FALSE, FALSE, TRUE);
  gtk_widget_set_uposition(image_resize->shell, generic_window_x, generic_window_y);
  layout_connect_window_position(image_resize->shell, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(image_resize->shell);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_resize->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_resize);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_resize->resize->resize_widget, FALSE, FALSE, 0);

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = image_resize;
  action_items[1].user_data = image_resize;
  build_action_area (GTK_DIALOG (image_resize->shell), action_items, 2, 0);

  gtk_widget_show (image_resize->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_resize->shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer client_data)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_scale_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_scale;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_scale = (ImageResize *) g_malloc_zero (sizeof (ImageResize));
  image_scale->gimage_id = gdisp->gimage->ID;
  image_scale->resize = resize_widget_new (ScaleWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_scale->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_scale->shell), "image_scale", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (image_scale->shell), _("Image Scale"));
  gtk_window_set_policy (GTK_WINDOW (image_scale->shell), FALSE, FALSE, TRUE);
  gtk_widget_set_uposition(image_scale->shell, generic_window_x, generic_window_y);
  layout_connect_window_position(image_scale->shell, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(image_scale->shell);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_scale->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_scale);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_scale->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_scale->resize->resize_widget, FALSE, FALSE, 0);

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = image_scale;
  action_items[1].user_data = image_scale;
  build_action_area (GTK_DIALOG (image_scale->shell), action_items, 2, 0);

  gtk_widget_show (image_scale->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_scale->shell);
}

void image_histogram_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;
  gdisp = gdisplay_active ();
  /*Out of bounds, see app/interface.c*/
  #if 0
  gtk_widget_activate (tool_widgets[tool_info[(int) HISTOGRAM].toolbar_position]);
  #endif
  histogram_tool_initialize ((void *) gdisp);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void layers_raise_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_raise_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void layers_lower_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_lower_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

/*  int value;*/

void layers_anchor_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  floating_sel_anchor (gimage_get_active_layer (gdisp->gimage));
  gdisplays_flush ();
}

void layers_merge_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  layers_dialog_layer_merge_query (gdisp->gimage, TRUE);
}

void layers_flatten_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_flatten (gdisp->gimage);
  gdisplays_flush ();
}

void layers_alpha_select_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_alpha (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void layers_mask_select_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_mask (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void layers_add_alpha_channel_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  layer_add_alpha ( gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void tools_default_colors_cmd_callback (GtkWidget *widget,gpointer client_data)
{
  palette_set_default_colors ();
}

void tools_swap_colors_cmd_callback (GtkWidget *widget,gpointer   client_data)
{
  palette_swap_colors ();
}

void tools_select_cmd_callback (GtkWidget           *widget,
			   gpointer             callback_data,
			   guint                callback_action)
{
  GDisplay * gdisp;

  /*  Activate the approriate widget  */
  gtk_widget_activate (tool_widgets[tool_info[(long) callback_action].toolbar_position]);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
/*  active_tool->prev_tool = -1;
*/
    }

void
select_color_picker (GtkWidget           *widget, 
    			   gpointer             callback_data, 
			   guint                callback_action)
{
#if 0
  GDisplay * gdisp;
  Tool prev_active_tool = active_tool->ID;
  gtk_widget_activate (tool_widgets[tool_info[(long) callback_action].toolbar_position]);

  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
  active_tool->prev_tool = prev_active_tool;
#endif
}

void
filters_repeat_cmd_callback (GtkWidget           *widget,
			     gpointer             callback_data,
			     guint                callback_action)
{
  plug_in_repeat (callback_action);
}

void
dialogs_brushes_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  create_brush_dialog ();
}

void
dialogs_patterns_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  create_pattern_dialog ();
}

void
dialogs_palette_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  palette_create ();
}

void
dialogs_store_frame_manager_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  bfm_create_sfm (gdisplay);
}

void
dialogs_store_frame_manager_adv_forward_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_adv_forward (gdisplay);
}

void
dialogs_store_frame_manager_adv_backwards_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_adv_backwards (gdisplay);
}

void
dialogs_store_frame_manager_flip_forward_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_flip_forward (gdisplay);
}

void
dialogs_store_frame_manager_flip_backwards_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_flip_backwards (gdisplay);
}

void
dialogs_store_frame_manager_play_forward_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_play_forward (gdisplay);
}

void
dialogs_store_frame_manager_play_backwards_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_play_backwards (gdisplay);
}

void
dialogs_store_frame_manager_stop_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();
  sfm_stop (gdisplay);
}

void
dialogs_gradient_editor_cmd_callback(GtkWidget *widget,
				     gpointer   client_data)
{
  grad_create_gradient_editor ();
} /* dialogs_gradient_editor_cmd_callback */

void
dialogs_lc_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  lc_dialog_create (gdisp->gimage->ID);
}

void
dialogs_indexed_palette_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  indexed_palette_create (gdisp->gimage->ID);
}

void
dialogs_tools_options_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  tools_options_dialog_show ();
}

void
dialogs_input_devices_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  create_input_dialog ();
}

void
dialogs_device_status_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  create_device_status ();
}

void
about_dialog_cmd_callback (GtkWidget *widget,
                           gpointer   client_data)
{
  about_dialog_create (FALSE);
}

void
bugs_dialog_cmd_callback (GtkWidget *widget,
                           gpointer   client_data)
{
  bugs_dialog_create (FALSE);
}

void
tips_dialog_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  tips_dialog_create ();
}


/****************************************************/
/**           LOCAL FUNCTIONS                      **/
/****************************************************/


/*********************/
/*  Local functions  */
/*********************/

static void
image_resize_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  GImage *gimage;

  image_resize = (ImageResize *) client_data;
  if ((gimage = gimage_get_ID (image_resize->gimage_id)) != NULL)
    {
      if (image_resize->resize->width > 0 &&
	  image_resize->resize->height > 0)
	{
	  gimage_resize (gimage,
			 image_resize->resize->width,
			 image_resize->resize->height,
			 image_resize->resize->off_x,
			 image_resize->resize->off_y);
	  gdisplays_flush ();
	}
      else
	{
	  g_message (_("Resize Error: Both width and height must be greater than zero."));
	  return;
	}
    }

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
image_scale_callback (GtkWidget *w,
		      gpointer   client_data)
{
  ImageResize *image_scale;
  GImage *gimage;

  image_scale = (ImageResize *) client_data;
  if ((gimage = gimage_get_ID (image_scale->gimage_id)) != NULL)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0)
	{
	  gimage_scale (gimage,
			image_scale->resize->width,
			image_scale->resize->height);
	  gdisplays_flush ();
	}
      else
	{
	  g_message (_("Scale Error: Both width and height must be greater than zero."));
	  return;
	}
    }

  gtk_widget_destroy (image_scale->shell);
  resize_widget_free (image_scale->resize);
  g_free (image_scale);
}

static gint
image_delete_callback (GtkWidget *w,
		       GdkEvent *e,
		       gpointer client_data)
{
  image_cancel_callback (w, client_data);

  return TRUE;
}


static void
image_cancel_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;

  image_resize = (ImageResize *) client_data;

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
gimage_mask_feather_callback (GtkWidget *w,
			      gpointer   client_data,
			      gpointer   call_data)
{
  GImage *gimage;
  double feather_radius;

  if (!(gimage = gimage_get_ID ((intptr_t)client_data)))
    return;

  feather_radius = atof (call_data);

  gimage_mask_feather (gimage, feather_radius);
  gdisplays_flush ();
}


static void
gimage_mask_border_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage;
  int border_radius;

  if (!(gimage = gimage_get_ID ((intptr_t)client_data)))
    return;

  border_radius = atoi (call_data);

  gimage_mask_border (gimage, border_radius);
  gdisplays_flush ();
}


static void
gimage_mask_grow_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  GImage *gimage;
  int grow_pixels;

  if (!(gimage = gimage_get_ID ((intptr_t)client_data)))
    return;

  grow_pixels = atoi (call_data);

  gimage_mask_grow (gimage, grow_pixels);
  gdisplays_flush ();
}


static void
gimage_mask_shrink_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage;
  int shrink_pixels;

  if (!(gimage = gimage_get_ID ((intptr_t)client_data)))
    return;
 

  shrink_pixels = atoi (call_data);

  gimage_mask_shrink (gimage, shrink_pixels);
  gdisplays_flush ();
}

void
brush_increase_radius ()
{
  GDisplay *gdisplay;
  int radius; 
  gdisplay = gdisplay_active ();

  if (GIMP_IS_BRUSH_GENERATED(get_active_brush()))
    {
      radius = gimp_brush_generated_get_radius ((GimpBrushGenerated *)get_active_brush ());
      radius = radius < 100 ? radius + 1 : radius;

      gimp_brush_generated_set_radius ((GimpBrushGenerated *)get_active_brush (), radius);
      create_win_cursor (gdisplay->canvas->window, 
	  (double)radius*2.0*((double)SCALEDEST (gdisplay) / SCALESRC (gdisplay))); 
    }
  else
    e_printf ("ERROR : you can not edit this brush\n");
}

void
brush_decrease_radius ()
{
  GDisplay *gdisplay;
  gdisplay = gdisplay_active ();

  if (GIMP_IS_BRUSH_GENERATED(get_active_brush()))
    {
      int radius = gimp_brush_generated_get_radius ((GimpBrushGenerated *)get_active_brush ());
      radius = radius > 1 ? radius - 1 : radius;

      gimp_brush_generated_set_radius ((GimpBrushGenerated *)get_active_brush (), radius);
      create_win_cursor (gdisplay->canvas->window,           
	  (double)radius*2.0*((double)SCALEDEST (gdisplay) / SCALESRC (gdisplay)));                     
    }
  else
    e_printf ("ERROR : you can not edit this brush\n");
}

