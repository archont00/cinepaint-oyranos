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
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "color_panel.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "rc.h"
#include "general.h"
#include "interface.h"
#include "layers_dialogP.h"
#include "ops_buttons.h"
#include "palette.h"
#include "paint_funcs_area.h"
#include "resize.h"
#include "minimize.h"

#include "buttons/eye.xbm"
#include "buttons/channel.xbm"
#include "buttons/new.xpm"
#include "buttons/new_is.xpm"
#include "buttons/raise.xpm"
#include "buttons/raise_is.xpm"
#include "buttons/lower.xpm"
#include "buttons/lower_is.xpm"
#include "buttons/duplicate.xpm"
#include "buttons/duplicate_is.xpm"
#include "buttons/delete.xpm"
#include "buttons/delete_is.xpm"

#include "channel_pvt.h"
#include "pixelarea.h"

#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK

#define CHANNEL_LIST_WIDTH 200
#define CHANNEL_LIST_HEIGHT 150

#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

#define COMPONENT_BASE_ID 0x10000000

typedef struct ChannelWidget ChannelWidget;

struct ChannelWidget {
  GtkWidget *eye_widget;
  GtkWidget *clip_widget;
  GtkWidget *channel_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  GImage *gimage;
  Channel *channel;
  GdkPixmap *channel_pixmap;
  ChannelType type;
  int ID;
  int width, height;
  int visited;
};

typedef struct ChannelsDialog ChannelsDialog;

struct ChannelsDialog {
  GtkWidget *vbox;
  GtkWidget *channel_list;
  GtkWidget *preview;
  GtkWidget *ops_menu;
  GtkAccelGroup *accel_group;

  int num_components;
  Format format;
  ChannelType components[4];
  double ratio;
  int image_width, image_height;
  int gimage_width, gimage_height;

  /*  state information  */
  int gimage_id;
  Channel * active_channel;
  Layer *floating_sel;
  GSList *channel_widgets;
};

/*  channels dialog widget routines  */
static void channels_dialog_preview_extents (void);
static void channels_dialog_set_menu_sensitivity (void);
static void channels_dialog_set_channel (ChannelWidget *);
static void channels_dialog_unset_channel (ChannelWidget *);
static void channels_dialog_position_channel (ChannelWidget *, int);
static void channels_dialog_add_channel (Channel *);
static void channels_dialog_remove_channel (ChannelWidget *);
static gint channel_list_events (GtkWidget *, GdkEvent *);

/*  channels dialog menu callbacks  */
static void channels_dialog_map_callback (GtkWidget *, gpointer);
static void channels_dialog_unmap_callback (GtkWidget *, gpointer);
static void channels_dialog_new_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_raise_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_lower_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_duplicate_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_delete_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_channel_to_sel_callback (GtkWidget *, gpointer);

/*  channel widget function prototypes  */
static ChannelWidget *channel_widget_get_ID (Channel *);
static ChannelWidget *create_channel_widget (GImage *, Channel *, ChannelType);
static gint channel_widget_list_pos (ChannelType);
static void channel_widget_delete (ChannelWidget *);
static void channel_widget_select_update (GtkWidget *, gpointer);
static gint channel_widget_button_events (GtkWidget *, GdkEvent *);
static gint channel_widget_preview_events (GtkWidget *, GdkEvent *);
static void channel_widget_preview_redraw (ChannelWidget *);
static void channel_widget_no_preview_redraw (ChannelWidget *);
static void channel_widget_eye_redraw (ChannelWidget *);
static void channel_widget_exclusive_visible (ChannelWidget *);
static void channel_widget_channel_flush (GtkWidget *, gpointer);

/*  assorted query dialogs  */
static void channels_dialog_new_channel_query (int);
static void channels_dialog_edit_channel_query (ChannelWidget *);
static void edit_channel_use_opacity_clicked (ChannelWidget *);

/*  Only one channels dialog  */
static ChannelsDialog *channelsD = NULL;

static GdkPixmap *eye_pixmap[4] = { NULL, NULL, NULL, NULL };
static GdkPixmap *channel_pixmap[4] = { NULL, NULL, NULL, NULL };

static int suspend_gimage_notify = 0;

static guint32 button_click_time = 0;
static int button_last_id = 0;

static MenuItem channels_ops[] =
{
  { "New Channel", 'N', GDK_CONTROL_MASK,
    channels_dialog_new_channel_callback, NULL, NULL, NULL },
  { "Raise Channel", 'F', GDK_CONTROL_MASK,
    channels_dialog_raise_channel_callback, NULL, NULL, NULL },
  { "Lower Channel", 'B', GDK_CONTROL_MASK,
    channels_dialog_lower_channel_callback, NULL, NULL, NULL },
  { "Duplicate Channel", 'C', GDK_CONTROL_MASK,
    channels_dialog_duplicate_channel_callback, NULL, NULL, NULL },
  { "Delete Channel", 'X', GDK_CONTROL_MASK,
    channels_dialog_delete_channel_callback, NULL, NULL, NULL },
  { "Channel To Selection", 'S', GDK_CONTROL_MASK,
    channels_dialog_channel_to_sel_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL },
};

/* the ops buttons */
static OpsButton channels_ops_buttons[] =
{
  { new_xpm, new_is_xpm, channels_dialog_new_channel_callback, "New Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { raise_xpm, raise_is_xpm, channels_dialog_raise_channel_callback, "Raise Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { lower_xpm, lower_is_xpm, channels_dialog_lower_channel_callback, "Lower Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { duplicate_xpm, duplicate_is_xpm, channels_dialog_duplicate_channel_callback, "Duplicate Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { delete_xpm, delete_is_xpm, channels_dialog_delete_channel_callback, "Delete Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/*
 *  The edit channel attributes dialog
 */

typedef struct EditChannelOptions EditChannelOptions;

struct EditChannelOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;

  ChannelWidget *channel_widget;
  int gimage_id;
  ColorPanel *color_panel;
  double opacity;
  gint link_paint;
  double link_paint_opacity;
  gint channel_as_opacity; 
};

static EditChannelOptions *options = NULL;

/**************************************/
/*  Public channels dialog functions  */
/**************************************/

GtkWidget *
channels_dialog_create ()
{
  GtkWidget *vbox;
  GtkWidget *listbox;
  GtkWidget *button_box;

  /* translate menu items */
  channels_ops[0].label = _("New Channel");
  channels_ops[1].label = _("Raise Channel");
  channels_ops[2].label = _("Lower Channel");
  channels_ops[3].label = _("Duplicate Channel");
  channels_ops[4].label = _("Delete Channel");
  channels_ops[5].label = _("Channel To Selection");

  channels_ops_buttons[0].tooltip = _("New Channel");
  channels_ops_buttons[1].tooltip = _("Raise Channel");
  channels_ops_buttons[2].tooltip = _("Lower Channel");
  channels_ops_buttons[3].tooltip = _("Duplicate Channel");
  channels_ops_buttons[4].tooltip = _("Delete Channel");

  

  if (!channelsD)
    {
      channelsD = g_malloc_zero (sizeof (ChannelsDialog));
      channelsD->preview = NULL;
      channelsD->gimage_id = -1;
      channelsD->active_channel = NULL;
      channelsD->floating_sel = NULL;
      channelsD->channel_widgets = NULL;
      channelsD->accel_group = gtk_accel_group_new ();

      if (preview_size)
	{
	  channelsD->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
	  gtk_preview_size (GTK_PREVIEW (channelsD->preview), preview_size, preview_size);
	}

      /*  The main vbox  */
      channelsD->vbox = vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 2);

      /*  The layers commands pulldown menu  */
      channelsD->ops_menu = build_menu (channels_ops, channelsD->accel_group);

      /*  The channels listbox  */
      listbox = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_usize (listbox, CHANNEL_LIST_WIDTH, CHANNEL_LIST_HEIGHT);
      gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 2);

      channelsD->channel_list = gtk_list_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox), channelsD->channel_list);
      gtk_list_set_selection_mode (GTK_LIST (channelsD->channel_list), GTK_SELECTION_MULTIPLE);
      gtk_signal_connect (GTK_OBJECT (channelsD->channel_list), "event",
			  (GtkSignalFunc) channel_list_events,
			  channelsD);
      gtk_container_set_focus_vadjustment (GTK_CONTAINER (channelsD->channel_list),
					   gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (listbox)));
      GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (listbox)->vscrollbar, GTK_CAN_FOCUS);

      gtk_widget_show (channelsD->channel_list);
      gtk_widget_show (listbox);


      /* The ops buttons */

      button_box = ops_button_box_new (lc_shell, tool_tips, channels_ops_buttons);

      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /*  Set up signals for map/unmap for the accelerators  */
      gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "map",
			  (GtkSignalFunc) channels_dialog_map_callback,
			  NULL);
      gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "unmap",
			  (GtkSignalFunc) channels_dialog_unmap_callback,
			  NULL);

      gtk_widget_show (vbox);
    }

  return channelsD->vbox;
}


void
channels_dialog_flush ()
{
  GImage *gimage;
  Channel *channel;
  ChannelWidget *cw;
  GSList *list;
  int gimage_pos;
  int pos;
  CMSProfile *profile = 0;
  static char colour_sig[5] = {"Rgb"};
  const char* colour_sig_neu = {"Rgb"};

  if (!lc_shell)
    return;
    
  if (!channelsD)
    return;

  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  profile = gimage_get_cms_profile(gimage);
  if(profile)
      colour_sig_neu = cms_get_profile_info(profile)->color_space_name;

  if(strstr(colour_sig,colour_sig_neu) == 0)
    {
      strcpy(colour_sig, colour_sig_neu);
      channelsD->gimage_id = -1;
      channels_dialog_update (gimage->ID);
    }

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != channelsD->gimage_width) ||
      (gimage->height != channelsD->gimage_height) ||
      (tag_format (gimage_tag (gimage)) != channelsD->format))
    {
      channelsD->gimage_id = -1;
      channels_dialog_update (gimage->ID);
    }

  /*  Set all current channel widgets to visited = FALSE  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      cw->visited = FALSE;
      list = g_slist_next (list);
    }

  /*  Add any missing channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      cw = channel_widget_get_ID (channel);

      /*  If the channel isn't in the channel widget list, add it  */
      if (cw == NULL)
	channels_dialog_add_channel (channel);
      else
	cw->visited = TRUE;

      list = g_slist_next (list);
    }

  /*  Remove any extraneous auxillary channels  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);

      if (cw->visited == FALSE && cw->type == Auxillary)
	/*  will only be true for auxillary channels  */
	channels_dialog_remove_channel (cw);
    }

  /*  Switch positions of items if necessary  */
  list = channelsD->channel_widgets;
  pos = -channelsD->num_components;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);

      if (cw->type == Auxillary)
	if ((gimage_pos = gimage_get_channel_index (gimage, cw->channel)) != pos)
	  channels_dialog_position_channel (cw, gimage_pos);

      pos++;
    }

  /*  Set the active channel  */
  if (channelsD->active_channel != gimage->active_channel)
    channelsD->active_channel = gimage->active_channel;

  /*  set the menus if floating sel status has changed  */
  if (channelsD->floating_sel != gimage->floating_sel)
    channelsD->floating_sel = gimage->floating_sel;

  channels_dialog_set_menu_sensitivity ();

  gtk_container_foreach (GTK_CONTAINER (channelsD->channel_list),
			 channel_widget_channel_flush, NULL);
}


/*************************************/
/*  channels dialog widget routines  */
/*************************************/

void
channels_dialog_update (int gimage_id)
{
  ChannelWidget *cw;
  GImage *gimage;
  Channel *channel;
  GSList *list;
  GList *item_list;

  if (options)
  {
    color_panel_free (options->color_panel);
    gtk_widget_destroy (options->query_box);
    g_free (options);
    options = NULL;
  }

  if (!channelsD)
    return;

  {
    GImage *g;
    gint cchannels = 0;

    if (! (g = gimage_get_ID (gimage_id)))
      return;

    cchannels = gimage_get_num_color_channels(g);
    if (channelsD->gimage_id == gimage_id &&
        channelsD->num_components == (
           cchannels + g_slist_length(gimage_channels(g)) ) )
      return;
  }

  channelsD->gimage_id = gimage_id;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);
      channel_widget_delete (cw);
    }
  channelsD->channel_widgets = NULL;

  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  /*  Find the preview extents  */
  channels_dialog_preview_extents ();

  channelsD->active_channel = NULL;
  channelsD->floating_sel = NULL;

  /*  The image components  */
  item_list = NULL;

  switch ((channelsD->format = gimage_format (gimage)))
    {
    case FORMAT_RGB:
      cw = create_channel_widget (gimage, NULL, Red);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Red;

      cw = create_channel_widget (gimage, NULL, Green);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[1] = Green;

      cw = create_channel_widget (gimage, NULL, Blue);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[2] = Blue;
      channelsD->num_components = 3;

      if (tag_alpha(drawable_tag (GIMP_DRAWABLE(gimage->active_layer))) == ALPHA_YES)
      {
        cw = create_channel_widget (gimage, NULL, Matte);
        channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
        item_list = g_list_append (item_list, cw->list_item);
        channelsD->components[3] = Matte;
        ++channelsD->num_components;
      }
      break;

    case FORMAT_GRAY:
      cw = create_channel_widget (gimage, NULL, Gray);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Gray;

      channelsD->num_components = 1;
      break;

    case FORMAT_INDEXED:
      cw = create_channel_widget (gimage, NULL, Indexed);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Indexed;

      channelsD->num_components = 1;
      break;

    case FORMAT_NONE:
      g_warning (_("channels_dialog_update: gimage has FORMAT_NONE"));
      return;
    }

  /*  The auxillary image channels  */
  list = gimage->channels;
  while (list)
    {
      /*  create a channel list item  */
      channel = (Channel *) list->data;
      cw = create_channel_widget (gimage, channel, Auxillary);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);

      list = g_slist_next (list);
    }

  /*  get the index of the active channel  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (channelsD->channel_list), item_list, 0);
}


void
channels_dialog_clear ()
{
  ops_button_box_set_insensitive (channels_ops_buttons);

  suspend_gimage_notify++;
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;
}

void
channels_dialog_free ()
{
  GSList *list;
  ChannelWidget *cw;

  if (channelsD == NULL)
    return;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);
      channel_widget_delete (cw);
    }
  channelsD->channel_widgets = NULL;
  channelsD->active_channel = NULL;
  channelsD->floating_sel = NULL;

  if (channelsD->preview)
    gtk_object_sink (GTK_OBJECT (channelsD->preview));

  if (channelsD->ops_menu)
    gtk_object_sink (GTK_OBJECT (channelsD->ops_menu));

  g_free (channelsD);
  channelsD = NULL;
}

static void
channels_dialog_preview_extents ()
{
  GImage *gimage;

  if (! channelsD)
    return;

  gimage = gimage_get_ID (channelsD->gimage_id);
  channelsD->gimage_width = gimage->width;
  channelsD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    channelsD->ratio = (double) preview_size / (double) gimage->width;
  else
    channelsD->ratio = (double) preview_size / (double) gimage->height;

  if (preview_size)
    {
      channelsD->image_width = (int) (channelsD->ratio * gimage->width);
      channelsD->image_height = (int) (channelsD->ratio * gimage->height);
      if (channelsD->image_width < 1) channelsD->image_width = 1;
      if (channelsD->image_height < 1) channelsD->image_height = 1;
    }
  else
    {
      channelsD->image_width = channel_width;
      channelsD->image_height = channel_height;
    }
}


static void
channels_dialog_set_menu_sensitivity ()
{
  ChannelWidget *cw;
  gint fs_sensitive;
  gint aux_sensitive;

  cw = channel_widget_get_ID (channelsD->active_channel);
  fs_sensitive = (channelsD->floating_sel != NULL);

  if (cw)
    aux_sensitive = (cw->type == Auxillary);
  else
    aux_sensitive = FALSE;

  /* new channel */
  gtk_widget_set_sensitive (channels_ops[0].widget, !fs_sensitive);
  ops_button_set_sensitive (&channels_ops_buttons[0], !fs_sensitive);
  /* raise channel */
  gtk_widget_set_sensitive (channels_ops[1].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (&channels_ops_buttons[1], !fs_sensitive && aux_sensitive);
  /* lower channel */
  gtk_widget_set_sensitive (channels_ops[2].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (&channels_ops_buttons[2], !fs_sensitive && aux_sensitive);
  /* duplicate channel */
  gtk_widget_set_sensitive (channels_ops[3].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (&channels_ops_buttons[3], !fs_sensitive && aux_sensitive);
  /* delete channel */
  gtk_widget_set_sensitive (channels_ops[4].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (&channels_ops_buttons[4], !fs_sensitive && aux_sensitive);
  /* channel to selection */
  gtk_widget_set_sensitive (channels_ops[5].widget, aux_sensitive);
}


static void
channels_dialog_set_channel (ChannelWidget *channel_widget)
{
  GtkStateType state;
  int index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == Auxillary)
    {
      /*  turn on the specified auxillary channel  */
      index = gimage_get_channel_index (channel_widget->gimage, channel_widget->channel);
      if ((index >= 0) && (state != GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_select_item (GTK_LIST (channelsD->channel_list), index + channelsD->num_components);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  else
    {
      if (state != GTK_STATE_SELECTED)
	{
          gint pos = channel_widget_list_pos( channel_widget->type );
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_select_item (GTK_LIST (channelsD->channel_list), pos);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  suspend_gimage_notify--;
}


static void
channels_dialog_unset_channel (ChannelWidget * channel_widget)
{
  GtkStateType state;
  int index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == Auxillary)
    {
      /*  turn off the specified auxillary channel  */
      index = gimage_get_channel_index (channel_widget->gimage, channel_widget->channel);
      if ((index >= 0) && (state == GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), index + channelsD->num_components);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  else
    {
      if (state == GTK_STATE_SELECTED)
	{
          gint pos = channel_widget_list_pos( channel_widget->type );
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), pos);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }

  suspend_gimage_notify--;
}

static gint
channel_widget_list_pos (ChannelType type)
{
  gint pos = 0;
  switch (type)
    {
    case Red: case Gray: case Indexed:
      pos = 0;
      break;
    case Green:
      pos = 1;
      break;
    case Blue:
      pos = 2;
      break;
    case Matte:
      pos = 3;
      break;
    case Auxillary:
      g_error ("error Matte[2] in %s at %d: this shouldn't happen.",
	       __FILE__, __LINE__);
      break;
    }
  return pos;
}

static void
channels_dialog_position_channel (ChannelWidget *channel_widget,
				  int new_index)
{
  GList *list = NULL;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the channel from the dialog  */
  list = g_list_append (list, channel_widget->list_item);
  gtk_list_remove_items (GTK_LIST (channelsD->channel_list), list);
  channelsD->channel_widgets = g_slist_remove (channelsD->channel_widgets, channel_widget);

  suspend_gimage_notify--;

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (channelsD->channel_list), list, new_index + channelsD->num_components);
  channelsD->channel_widgets = g_slist_insert (channelsD->channel_widgets, channel_widget, new_index + channelsD->num_components);
}


static void
channels_dialog_add_channel (Channel *channel)
{
  GImage *gimage;
  GList *item_list;
  ChannelWidget *channel_widget;
  int position;

  if (!channelsD || !channel)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  item_list = NULL;

  channel_widget = create_channel_widget (gimage, channel, Auxillary);
  item_list = g_list_append (item_list, channel_widget->list_item);

  position = gimage_get_channel_index (gimage, channel);
  channelsD->channel_widgets = g_slist_insert (channelsD->channel_widgets, channel_widget,
					       position + channelsD->num_components);
  gtk_list_insert_items (GTK_LIST (channelsD->channel_list), item_list,
			 position + channelsD->num_components);
}


static void
channels_dialog_remove_channel (ChannelWidget *channel_widget)
{
  GList *list = NULL;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the requested channel from the dialog  */
  list = g_list_append (list, channel_widget->list_item);
  gtk_list_remove_items (GTK_LIST (channelsD->channel_list), list);

  /*  Delete the channel_widget  */
  channel_widget_delete (channel_widget);

  suspend_gimage_notify--;
}


static gint
channel_list_events (GtkWidget *widget,
		     GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  GtkWidget *event_widget;
  ChannelWidget *channel_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 3)
	    {
	      gtk_menu_popup (GTK_MENU (channelsD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	      return TRUE;
	    }
	  break;
	case GDK_2BUTTON_PRESS:
	  if (channel_widget->type == Auxillary)
	    channels_dialog_edit_channel_query (channel_widget);
	  return TRUE;
	  break;

	case GDK_KEY_PRESS:
	  kevent = (GdkEventKey *) event;
	  switch (kevent->keyval)
	    {
	    case GDK_Up:
	      /* d_printf ("up arrow\n"); */
	      break;
	    case GDK_Down:
	      /* d_printf ("down arrow\n"); */
	      break;
	    default:
	      return FALSE;
	      break;
	    }
	  return TRUE;
	  break;

	default:
	  break;
	}
    }

  return FALSE;
}


/*******************************/
/*  channels dialog callbacks  */
/*******************************/

static void
channels_dialog_map_callback (GtkWidget *w,
			      gpointer   client_data)
{
  if (!channelsD)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_shell),
			      channelsD->accel_group);
}

static void
channels_dialog_unmap_callback (GtkWidget *w,
				gpointer   client_data)
{
  if (!channelsD)
    return;

  gtk_window_remove_accel_group (GTK_WINDOW (lc_shell),
				 channelsD->accel_group);
}

static void
channels_dialog_new_channel_callback (GtkWidget *w,
				      gpointer   client_data)
{
  /*  if there is a currently selected gimage, request a new channel
   */
  if (!channelsD)
    return;
  if (channelsD->gimage_id == -1)
    return;

  channels_dialog_new_channel_query (channelsD->gimage_id);
}


static void
channels_dialog_raise_channel_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;

  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_raise_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_lower_channel_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;

  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_lower_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_duplicate_channel_callback (GtkWidget *w,
					    gpointer   client_data)
{
  GImage *gimage;
  Channel *active_channel;
  Channel *new_channel;

  /*  if there is a currently selected gimage, request a new channel
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if ((active_channel = gimage_get_active_channel (gimage)))
    {
      new_channel = channel_copy (active_channel);
      gimage_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }
}


static void
channels_dialog_delete_channel_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_remove_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_channel_to_sel_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_mask_load (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


/****************************/
/*  channel widget functions  */
/****************************/

static ChannelWidget *
channel_widget_get_ID (Channel *channel)
{
  ChannelWidget *lw;
  GSList *list;

  if (!channelsD)
    return NULL;

  list = channelsD->channel_widgets;

  while (list)
    {
      lw = (ChannelWidget *) list->data;
      if (lw->channel == channel)
	return lw;

      list = g_slist_next (list);
    }

  return NULL;
}


static ChannelWidget *
create_channel_widget (GImage      *gimage,
		       Channel     *channel,
		       ChannelType  type)
{
  ChannelWidget *channel_widget;
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;
  CMSProfile *profile = gimage_get_cms_profile(gimage);
  static const char* color_space_channel_names_[8];
  const char** color_space_channel_names = color_space_channel_names_;
  color_space_channel_names_[0] = _("Red");
  color_space_channel_names_[1] = _("Green");
  color_space_channel_names_[2] = _("Blue");
  color_space_channel_names_[3] = _("Alpha");
  color_space_channel_names_[4] = "";
  if(profile)
    {
      color_space_channel_names = cms_get_profile_info(profile)->color_space_channel_names;
    }
# ifdef DEBUG
  printf("%s:%d %s %s %s %s %s %s\n", __FILE__,__LINE__, profile ? "ja":"nein", color_space_channel_names_[0], color_space_channel_names_[1], color_space_channel_names_[2], color_space_channel_names_[3], color_space_channel_names_[4]);
# endif

  list_item = gtk_list_item_new ();

  /*  create the channel widget and add it to the list  */
  channel_widget = (ChannelWidget *) g_malloc_zero (sizeof (ChannelWidget));
  channel_widget->gimage = gimage;
  channel_widget->channel = channel;
  channel_widget->channel_preview = NULL;
  channel_widget->channel_pixmap = NULL;
  channel_widget->type = type;
  channel_widget->ID = (type == Auxillary) ? GIMP_DRAWABLE(channel)->ID : (COMPONENT_BASE_ID + type);
  channel_widget->list_item = list_item;
  channel_widget->width = -1;
  channel_widget->height = -1;
  channel_widget->visited = TRUE;

  /*  Need to let the list item know about the channel_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), channel_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) channel_widget_select_update,
		      channel_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      (GtkSignalFunc) channel_widget_select_update,
		      channel_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /* Create the visibility toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  channel_widget->eye_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->eye_widget), eye_width, eye_height);
  gtk_widget_set_events (channel_widget->eye_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->eye_widget), "event",
		      (GtkSignalFunc) channel_widget_button_events,
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->eye_widget), channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->eye_widget);
  gtk_widget_show (channel_widget->eye_widget);
  gtk_widget_show (alignment);

  /*  The preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  channel_widget->channel_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->channel_preview),
			 channelsD->image_width, channelsD->image_height);
  gtk_widget_set_events (channel_widget->channel_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->channel_preview), "event",
		      (GtkSignalFunc) channel_widget_preview_events,
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->channel_preview), channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->channel_preview);
  gtk_widget_show (channel_widget->channel_preview);

  /*  the channel name label */
  switch (channel_widget->type)
    {
    case Red:       channel_widget->label = gtk_label_new (color_space_channel_names[0]); break;
    case Green:     channel_widget->label = gtk_label_new (color_space_channel_names[1]); break;
    case Blue:      channel_widget->label = gtk_label_new (color_space_channel_names[2]); break;
    case Matte:     channel_widget->label = gtk_label_new (color_space_channel_names[3]); break;
    case Gray:      channel_widget->label = gtk_label_new (_("Gray")); break;
    case Indexed:   channel_widget->label = gtk_label_new (_("Indexed")); break;
    /*case Matte:     channel_widget->label = gtk_label_new (_("Matte")); break;*/
    case Auxillary: channel_widget->label = gtk_label_new (GIMP_DRAWABLE(channel)->name); break;
    }

  gtk_box_pack_start (GTK_BOX (hbox), channel_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (channel_widget->label);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (channel_widget->list_item);

  return channel_widget;
}


static void
channel_widget_delete (ChannelWidget *channel_widget)
{
  if (channel_widget->channel_pixmap)
    gdk_pixmap_unref (channel_widget->channel_pixmap);

  /*  Remove the channel widget from the list  */
  channelsD->channel_widgets = g_slist_remove (channelsD->channel_widgets, channel_widget);

  /*  Release the widget  */
  gtk_widget_unref (channel_widget->list_item);
  g_free (channel_widget);
}


static void
channel_widget_select_update (GtkWidget *w,
			      gpointer   data)
{
  ChannelWidget *channel_widget;

  if ((channel_widget = (ChannelWidget *) data) == NULL)
    return;

  if (suspend_gimage_notify == 0)
    {
      if (channel_widget->type == Auxillary)
	{
	  if (!channel_widget->gimage->active_channel)
	    /*  set the gimage's active channel to be this channel  */
	    gimage_set_active_channel (channel_widget->gimage, channel_widget->channel);
	  else
	    /*  unset the gimage's active channel  */
	    gimage_unset_active_channel (channel_widget->gimage, channel_widget->channel);

	  gdisplays_flush ();
	}
      else if (channel_widget->type != Auxillary)
	{
          gint pos = channel_widget_list_pos( channel_widget->type );
#if GTK_MAJOR_VERSION > 1
	  if (!channel_widget->gimage->active[ channel_widget->type ])
#else
	  if (w->state == GTK_STATE_SELECTED)
#endif
	    gimage_set_component_active (channel_widget->gimage, channel_widget->type, TRUE);
	  else
	    gimage_set_component_active (channel_widget->gimage, channel_widget->type, FALSE);
#if GTK_MAJOR_VERSION > 1
          /* update the list view */
          ++suspend_gimage_notify;
          if(channel_widget->gimage->active[ channel_widget->type ])
            gtk_list_select_item (GTK_LIST (channelsD->channel_list), pos);
          else
            gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), pos);
          --suspend_gimage_notify;
#endif
	}
    }
}


static gint
channel_widget_button_events (GtkWidget *widget,
			      GdkEvent  *event)
{
  static int button_down = 0;
  static GtkWidget *click_widget = NULL;
  static int old_state;
  static int exclusive;
  ChannelWidget *channel_widget;
  GtkWidget *event_widget;
  GdkEventButton *bevent;
  gint return_val;
  int visible;
  int width, height;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  switch (channel_widget->type)
    {
    case Auxillary:
      visible = GIMP_DRAWABLE(channel_widget->channel)->visible;
      width = drawable_width (GIMP_DRAWABLE(channel_widget->channel));
      height = drawable_height (GIMP_DRAWABLE(channel_widget->channel));
      break;
    default:
      visible = gimage_get_component_visible (channel_widget->gimage, channel_widget->type);
      width = channel_widget->gimage->width;
      height = channel_widget->gimage->height;
      break;
    }

  return_val = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == channel_widget->eye_widget)
	channel_widget_eye_redraw (channel_widget);
      break;

    case GDK_BUTTON_PRESS:
      return_val = TRUE;

      bevent = (GdkEventButton *) event;

      if (bevent->button == 3) {
	gtk_menu_popup (GTK_MENU (channelsD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	return TRUE;
      }

      button_down = 1;
      click_widget = widget;
      gtk_grab_add (click_widget);

      if (widget == channel_widget->eye_widget)
	{
	  old_state = visible;

	  /*  If this was a shift-click, make all/none visible  */
	  if (event->button.state & GDK_SHIFT_MASK)
	    {
	      exclusive = TRUE;
	      channel_widget_exclusive_visible (channel_widget);
	    }
	  else
	    {
	      exclusive = FALSE;
	      if (channel_widget->type == Auxillary)
		GIMP_DRAWABLE(channel_widget->channel)->visible = !visible;
	      else
		{
		  gimage_adjust_background_layer_visibility (channel_widget->gimage);
		  gimage_set_component_visible (channel_widget->gimage, channel_widget->type, !visible);
		}
	      channel_widget_eye_redraw (channel_widget);
	    }
	
          /*gimage_adjust_background_layer_visibility (channel_widget->gimage);*/
	}
      break;

    case GDK_BUTTON_RELEASE:
      return_val = TRUE;

      button_down = 0;
      gtk_grab_remove (click_widget);

      if (widget == channel_widget->eye_widget)
	{
	  if (exclusive || old_state != visible)
	    {
	      gimage_invalidate_preview (channel_widget->gimage);
	      gdisplays_update_area (channel_widget->gimage->ID, 0, 0,
				     channel_widget->gimage->width,
				     channel_widget->gimage->height);
	      gdisplays_flush ();
	    }
	}
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (button_down && (event_widget == click_widget))
	{
	  if (widget == channel_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  channel_widget_exclusive_visible (channel_widget);
		}
	      else
		{
		  if (channel_widget->type == Auxillary)
		    GIMP_DRAWABLE(channel_widget->channel)->visible = !visible;
		  else
		    gimage_set_component_visible (channel_widget->gimage, channel_widget->type, !visible);
		  channel_widget_eye_redraw (channel_widget);
		}
	    }
	}
      break;

    default:
      break;
    }

  return return_val;
}


static gint
channel_widget_preview_events (GtkWidget *widget,
			       GdkEvent  *event)
{
  GdkEventExpose *eevent;
  GdkEventButton *bevent;
  ChannelWidget *channel_widget;
  int valid;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:

      bevent = (GdkEventButton *) event;

      if (bevent->button == 3) {
	gtk_menu_popup (GTK_MENU (channelsD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	return TRUE;
      }
      break;

    case GDK_EXPOSE:
      if (!preview_size)
	channel_widget_no_preview_redraw (channel_widget);
      else
	{
	  switch (channel_widget->type)
	    {
	    case Auxillary:
	      valid = GIMP_DRAWABLE(channel_widget->channel)->preview_valid;
	      break;
	    default:
	      valid = gimage_preview_valid (channel_widget->gimage, channel_widget->type);
	      break;
	    }

	  if (!valid || !channel_widget->channel_pixmap)
	    {
	      channel_widget_preview_redraw (channel_widget);

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       channel_widget->channel_pixmap,
			       0, 0, 0, 0,
			       channelsD->image_width,
			       channelsD->image_height);
	    }
	  else
	    {
	      eevent = (GdkEventExpose *) event;

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       channel_widget->channel_pixmap,
			       eevent->area.x, eevent->area.y,
			       eevent->area.x, eevent->area.y,
			       eevent->area.width, eevent->area.height);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}


static void
channel_widget_preview_redraw (ChannelWidget *channel_widget)
{
  Canvas * preview_buf;
  int width, height;
  int channel;

  /*  allocate the channel widget pixmap  */
  if (! channel_widget->channel_pixmap)
    channel_widget->channel_pixmap = gdk_pixmap_new (channel_widget->channel_preview->window,
						     channelsD->image_width,
						     channelsD->image_height,
						     -1);

  /*  determine width and height  */
  switch (channel_widget->type)
    {
    case Auxillary:
      width = drawable_width (GIMP_DRAWABLE(channel_widget->channel));
      height = drawable_height (GIMP_DRAWABLE(channel_widget->channel));
      channel_widget->width = (int) (channelsD->ratio * width);
      channel_widget->height = (int) (channelsD->ratio * height);
      preview_buf = channel_preview (channel_widget->channel,
				     channel_widget->width,
				     channel_widget->height);
      break;
    default:
      width = channel_widget->gimage->width;
      height = channel_widget->gimage->height;
      channel_widget->width = (int) (channelsD->ratio * width);
      channel_widget->height = (int) (channelsD->ratio * height);
      preview_buf = gimage_composite_preview (channel_widget->gimage,
					      channel_widget->type,
					      channel_widget->width,
					      channel_widget->height);
      break;
    }

  switch (channel_widget->type)
    {
    case Red:       channel = RED_PIX; break;
    case Green:     channel = GREEN_PIX; break;
    case Blue:      channel = BLUE_PIX; break;
    case Matte:     channel = ALPHA_PIX; break;
    case Gray:      channel = GRAY_PIX; break;
    case Indexed:   channel = INDEXED_PIX; break;
    case Auxillary: channel = -1; break;
    default:        channel = -1; break;
    }
  
  render_preview (preview_buf,
		  channelsD->preview,
                  NULL,
		  channelsD->image_width,
		  channelsD->image_height,
		  channel);
 
  gtk_preview_put (GTK_PREVIEW (channelsD->preview),
		   channel_widget->channel_pixmap,
		   channel_widget->channel_preview->style->black_gc,
		   0, 0, 0, 0, channelsD->image_width, channelsD->image_height);

  /*  make sure the image has been transfered completely to the pixmap before
   *  we use it again...
   */
  gdk_flush ();
}


static void
channel_widget_no_preview_redraw (ChannelWidget *channel_widget)
{
  GdkPixmap *pixmap;
  GdkPixmap **pixmap_normal;
  GdkPixmap **pixmap_selected;
  GdkPixmap **pixmap_insensitive;
  GdkColor *color;
  GtkWidget *widget;
  GtkStateType state;
  gchar *bits;
  int width, height;

  state = channel_widget->list_item->state;

  widget = channel_widget->channel_preview;
  pixmap_normal = &channel_pixmap[NORMAL];
  pixmap_selected = &channel_pixmap[SELECTED];
  pixmap_insensitive = &channel_pixmap[INSENSITIVE];
  bits = (gchar *) channel_bits;
  width = channel_width;
  height = channel_height;

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &widget->style->white;
    }
  else
    color = &widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (widget->window, color);

  if (!*pixmap_normal)
    {
      *pixmap_normal =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_SELECTED],
				     &widget->style->bg[GTK_STATE_SELECTED]);
      *pixmap_selected =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_NORMAL],
				     &widget->style->white);
      *pixmap_insensitive =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_INSENSITIVE],
				     &widget->style->bg[GTK_STATE_INSENSITIVE]);
    }

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	pixmap = *pixmap_selected;
      else
	pixmap = *pixmap_normal;
    }
  else
    pixmap = *pixmap_insensitive;

  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   pixmap, 0, 0, 0, 0, width, height);
}


static void
channel_widget_eye_redraw (ChannelWidget *channel_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = channel_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &channel_widget->eye_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &channel_widget->eye_widget->style->white;
    }
  else
    color = &channel_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (channel_widget->eye_widget->window, color);

  switch (channel_widget->type)
    {
    case Auxillary:
      visible = GIMP_DRAWABLE(channel_widget->channel)->visible;
      break;
    default:
      visible = gimage_get_component_visible (channel_widget->gimage, channel_widget->type);
      break;
    }

  if (visible)
    {
      if (!eye_pixmap[NORMAL])
	{
	  eye_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_NORMAL],
					 &channel_widget->eye_widget->style->white);
	  eye_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_SELECTED],
					 &channel_widget->eye_widget->style->bg[GTK_STATE_SELECTED]);
	  eye_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &channel_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = eye_pixmap[SELECTED];
	  else
	    pixmap = eye_pixmap[NORMAL];
	}
      else
	pixmap = eye_pixmap[INSENSITIVE];

      gdk_draw_pixmap (channel_widget->eye_widget->window,
		       channel_widget->eye_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, eye_width, eye_height);
    }
  else
    {
      gdk_window_clear (channel_widget->eye_widget->window);
    }
}


static void
channel_widget_exclusive_visible (ChannelWidget *channel_widget)
{
  GSList *list;
  ChannelWidget *cw;
  int visible = FALSE;

  if (!channelsD)
    return;

  /*  First determine if _any_ other channel widgets are set to visible  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      if (cw != channel_widget)
	{
	  switch (cw->type)
	    {
	    case Auxillary:
	      visible |= GIMP_DRAWABLE(cw->channel)->visible;
	      break;
	    default:
	      visible |= gimage_get_component_visible (cw->gimage, cw->type);
	      break;
	    }
	}

      list = g_slist_next (list);
    }

  /*  Now, toggle the visibility for all channels except the specified one  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      if (cw != channel_widget)
	switch (cw->type)
	  {
	  case Auxillary:
	    GIMP_DRAWABLE(cw->channel)->visible = !visible;
	    break;
	  default:
	    gimage_set_component_visible (cw->gimage, cw->type, !visible);
	    break;
	  }
      else
	switch (cw->type)
	  {
	  case Auxillary:
	    GIMP_DRAWABLE(cw->channel)->visible = TRUE;
	    break;
	  default:
	    gimage_set_component_visible (cw->gimage, cw->type, TRUE);
	    break;
	  }

      channel_widget_eye_redraw (cw);

      list = g_slist_next (list);
    }
}


static void
channel_widget_channel_flush (GtkWidget *widget,
			      gpointer   client_data)
{
  ChannelWidget *channel_widget;
  int update_preview;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  /***  Sensitivity  ***/

  /*  If there is a floating selection...  */
  if (channelsD->floating_sel != NULL)
    {
      /*  to insensitive if this is an auxillary channel  */
      if (channel_widget->type == Auxillary)
	{
	  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, FALSE);
	}
      /*  to sensitive otherwise  */
      else
	{
	  if (! GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, TRUE);
	}
    }
  else
    {
      /*  to insensitive if there is an active channel, and this is a component channel  */
      if (channel_widget->type != Auxillary && channelsD->active_channel != NULL)
	{
	  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, FALSE);
	}
      /*  to sensitive otherwise  */
      else
	{
	  if (! GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, TRUE);
	}
    }

  /***  Selection  ***/

  /*  If this is an auxillary channel  */
  if (channel_widget->type == Auxillary)
    {
      /*  select if this is the active channel  */
      if (channelsD->active_channel == (channel_widget->channel))
	channels_dialog_set_channel (channel_widget);
      /*  unselect if this is not the active channel  */
      else
	channels_dialog_unset_channel (channel_widget);
    }
  else
    {
      /*  If the component is active, select. otherwise, deselect  */
      if (gimage_get_component_active (channel_widget->gimage, channel_widget->type))
	channels_dialog_set_channel (channel_widget);
      else
	channels_dialog_unset_channel (channel_widget);
    }

  switch (channel_widget->type)
    {
    case Auxillary:
      update_preview = !GIMP_DRAWABLE(channel_widget->channel)->preview_valid;
      break;
    default:
      update_preview = !gimage_preview_valid (channel_widget->gimage, channel_widget->type);
      break;
    }

  if (update_preview)
    gtk_widget_draw (channel_widget->channel_preview, NULL);
}


/*
 *  The new channel query dialog
 */

typedef struct NewChannelOptions NewChannelOptions;

struct NewChannelOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  ColorPanel *color_panel;

  int gimage_id;
  double opacity;
  gint link_paint;
  double link_paint_opacity;
};

static char *channel_name = NULL;
static PixelRow channel_col;
static unsigned char channel_color_data[TAG_MAX_BYTES];

static void
new_channel_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  NewChannelOptions *options;
  Channel *new_channel;
  GImage *gimage;

  options = (NewChannelOptions *) client_data;
  if (channel_name)
    g_free (channel_name);
  channel_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  if ((gimage = gimage_get_ID (options->gimage_id)))
    {
      new_channel = channel_new (gimage->ID, gimage->width, gimage->height,
				 tag_precision(gimage->tag),
				 channel_name, options->opacity / 100.0,
				 &options->color_panel->color);
      drawable_fill (GIMP_DRAWABLE(new_channel), TRANSPARENT_FILL);

      copy_row (&options->color_panel->color, &channel_col);

      gimage_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
new_channel_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  NewChannelOptions *options;

  options = (NewChannelOptions *) client_data;
  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
new_channel_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer client_data)
{
  new_channel_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
new_channel_query_scale_update (GtkAdjustment *adjustment,
				double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
channels_dialog_new_channel_query (int gimage_id)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", new_channel_query_ok_callback, NULL, NULL },
    { "Cancel", new_channel_query_cancel_callback, NULL, NULL }
  };
  GImage *gimage;
  NewChannelOptions *options;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *opacity_scale;
  GtkObject *opacity_scale_data;

  gimage = gimage_get_ID (gimage_id);

  /*  the new options structure  */
  options = (NewChannelOptions *) g_malloc_zero (sizeof (NewChannelOptions));
  options->gimage_id = gimage_id;
  options->opacity = 50.0;

  pixelrow_init (&channel_col,
                 tag_new (tag_precision (gimage_tag (gimage)),
                          FORMAT_RGB,
                          ALPHA_NO),
                 channel_color_data,
                 1);

  /* make the initial color blue */
  {
    COLOR16_NEWDATA (init, gfloat,
		     PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO) = {0.0, 0.0, 1.0};
    COLOR16_INIT (init);
    copy_row (&init, &channel_col);
  }
  options->color_panel = color_panel_new (&channel_col, 48, 64);

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "new_channel_options", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("New Channel Options"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);
  minimize_register(options->query_box);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (new_channel_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Channel name: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 75, 0);
  gtk_table_attach (GTK_TABLE (table), options->name_entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), (channel_name ? channel_name : _("New Channel")));
  gtk_widget_show (options->name_entry);

  /*  the opacity scale  */
  label = gtk_label_new (_("Display Opacity: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (options->opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_table_attach (GTK_TABLE (table), opacity_scale, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (opacity_scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) new_channel_query_scale_update,
		      &options->opacity);

  gtk_widget_show (opacity_scale);


  /*  the color panel  */
  gtk_table_attach (GTK_TABLE (table), options->color_panel->color_panel_widget,
		    2, 3, 0, 2, GTK_EXPAND, GTK_EXPAND, 4, 2);
  gtk_widget_show (options->color_panel->color_panel_widget);

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}



static void
edit_channel_query_button_clicked (GtkWidget *button,
				gpointer client_data)
{
#if 0
  EditChannelOptions *options;
#endif
  Channel *channel;

#if 0
  options = (EditChannelOptions *) client_data;
#endif
  channel = options->channel_widget->channel;
  if (GTK_TOGGLE_BUTTON (button)->active)
    options->link_paint = TRUE;
  else
    options->link_paint = FALSE;
  channel->link_paint = options->link_paint;
}

static void
edit_channel_as_opacity_clicked (GtkWidget *button,
    gpointer client_data)
{
  Channel *channel;
  channel = options->channel_widget->channel;
  options->channel_as_opacity = GTK_TOGGLE_BUTTON (button)->active ? TRUE : FALSE; 
  channel->channel_as_opacity = options->channel_as_opacity;
  if (channel->channel_as_opacity) 
    channel_as_opacity (channel, options->gimage_id); 
  else
    channel_as_opacity (NULL, options->gimage_id); 

}

static void
edit_channel_query_link_scale_update (GtkAdjustment *adjustment,
				gpointer client_data)
{
#if 0
  EditChannelOptions *options;
#endif
  Channel *channel;
  gfloat opacity;

#if 0
  options = (EditChannelOptions *) client_data;
#endif
  options->link_paint_opacity  = adjustment->value;
  channel = options->channel_widget->channel;
  opacity = options->link_paint_opacity / 100.0;
  channel->link_paint_opacity = opacity;
}

static void
edit_channel_query_scale_update (GtkAdjustment *adjustment,
				gpointer client_data)
{
#if 0
  EditChannelOptions *options;
#endif
  Channel *channel;
  gfloat opacity;
  int update = FALSE;
#if 0
  options = (EditChannelOptions *) client_data;
#endif
  options->opacity  = adjustment->value;
  channel = options->channel_widget->channel;
  opacity = options->opacity / 100.0;

  if (gimage_get_ID (options->gimage_id)) 
    {
    
    if (channel->opacity != opacity)
      {
	channel->opacity = opacity;
	update = TRUE;
      }

    copy_row (&options->color_panel->color, &channel->col);
    update = TRUE;
    
    if (update)
      {
	drawable_update (GIMP_DRAWABLE(channel),
                         0, 0,
                         0, 0);
	gdisplays_flush ();
      }
  }
}

static void
edit_channel_query_ok_callback (GtkWidget *w,
				gpointer   client_data)
{
#if 0
  EditChannelOptions *options;
#endif
  Channel *channel;
  gfloat opacity;
  int update = FALSE;
#if 0
  options = (EditChannelOptions *) client_data;
#endif
  channel = options->channel_widget->channel;
  opacity = options->opacity / 100.0;

  if (gimage_get_ID (options->gimage_id)) {
    /*  Set the new channel name  */
    if (GIMP_DRAWABLE(channel)->name)
      g_free (GIMP_DRAWABLE(channel)->name);
    GIMP_DRAWABLE(channel)->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
    gtk_label_set (GTK_LABEL (options->channel_widget->label), GIMP_DRAWABLE(channel)->name);
    
    channel->opacity = opacity;
    channel->link_paint = options->link_paint;
    channel->link_paint_opacity = options->link_paint_opacity /100.0;

    copy_row (&options->color_panel->color, &channel->col);
    update = TRUE;
    
    if (update)
      {
	drawable_update (GIMP_DRAWABLE(channel),
                         0, 0,
                         0, 0);
	gdisplays_flush ();
      }
  }
  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
  options = NULL;
}

static void
edit_channel_query_cancel_callback (GtkWidget *w,
				    gpointer   client_data)
{
#if 0
  EditChannelOptions *options;
#endif
  Channel *channel;

#if 0
  options = (EditChannelOptions *) client_data;
#endif
  channel = options->channel_widget->channel;

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
  options = NULL;
}


static gint
edit_channel_query_delete_callback (GtkWidget *w,
				    GdkEvent  *e,
				    gpointer client_data)
{
  edit_channel_query_cancel_callback (w, client_data);
  return TRUE;
}

static void
channels_dialog_edit_channel_query (ChannelWidget *channel_widget)
{
  static ActionAreaItem action_items[1] =
  {
    { "OK", edit_channel_query_ok_callback, NULL, NULL },
#if 0 
    { "Cancel", edit_channel_query_cancel_callback, NULL, NULL }
#endif
  };
#if 0
  EditChannelOptions *options;
#endif
  GtkWidget *vbox;
  GtkWidget *vbox1;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *opacity_scale;
  GtkObject *opacity_scale_data;
  GtkWidget *link_paint_opacity_scale;
  GtkObject *link_paint_opacity_scale_data;
  char * channel_name;
  gint index;
  Tag channel_tag = drawable_tag (GIMP_DRAWABLE (channel_widget->channel));
  Tag color_tag = tag_new (tag_precision (channel_tag), FORMAT_RGB, ALPHA_NO);

  if (options)
  {
    color_panel_free (options->color_panel);
    gtk_widget_destroy (options->query_box);
    g_free (options);
    options = NULL;
  }


  /*  the new options structure  */
  options = (EditChannelOptions *) g_malloc_zero (sizeof (EditChannelOptions));
  options->channel_widget = channel_widget;
  options->gimage_id = channel_widget->gimage->ID;
  options->opacity = (double) channel_widget->channel->opacity * 100.0;
  options->link_paint = channel_widget->channel->link_paint;
  options->link_paint_opacity = channel_widget->channel->link_paint_opacity * 100.0;
  options->channel_as_opacity = channel_widget->channel->channel_as_opacity;


  pixelrow_init (&channel_col, color_tag, channel_color_data, 1);
  copy_row (&channel_widget->channel->col, &channel_col);

  options->color_panel = color_panel_new (&channel_col, 48, 64);

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "edit_channel_atributes", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Channel Attributes"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);
  minimize_register(options->query_box);

  /* deal with the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (edit_channel_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 2, 2);
  label = gtk_label_new (_("Channel name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  channel_name = GIMP_DRAWABLE(channel_widget->channel)->name;
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), channel_name);
  gtk_widget_show (options->name_entry);
  gtk_widget_show (hbox);

  /*  the opacity scale  */
  vbox1 = gtk_vbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), vbox1, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 2, 2);

  
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Display Opacity"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (options->opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), opacity_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (opacity_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) edit_channel_query_scale_update,
		      options);

  gtk_widget_show (opacity_scale);
  gtk_widget_show (hbox);
  
  /* If its the first channel show RGBM paint toggle*/ 
  index = gimage_get_channel_index(channel_widget->gimage, channel_widget->channel);

  if(index == 0) 
  {
    button = gtk_check_button_new_with_label(_("Enable RGBM painting"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) edit_channel_query_button_clicked,
			options);
    gtk_box_pack_start (GTK_BOX (vbox1), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(button), options->link_paint); 
    gtk_widget_show (button);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);
    label = gtk_label_new (_("Matte color for RGBM paint"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    link_paint_opacity_scale_data = gtk_adjustment_new (
		options->link_paint_opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
    link_paint_opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (link_paint_opacity_scale_data));
    gtk_box_pack_start (GTK_BOX (hbox), link_paint_opacity_scale, TRUE, TRUE, 0);
    gtk_scale_set_value_pos (GTK_SCALE (link_paint_opacity_scale), GTK_POS_TOP);
    gtk_range_set_update_policy (GTK_RANGE (link_paint_opacity_scale), GTK_UPDATE_DELAYED);
    gtk_signal_connect (GTK_OBJECT (link_paint_opacity_scale_data), "value_changed",
			(GtkSignalFunc) edit_channel_query_link_scale_update,
			options);

    gtk_widget_show (link_paint_opacity_scale);
    gtk_widget_show (hbox);
  }


  /* use channel as opacity */
  button = gtk_check_button_new_with_label(_("Use this channel as opacity"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) edit_channel_as_opacity_clicked,
      options);
  gtk_box_pack_start (GTK_BOX (vbox1), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(button), options->channel_as_opacity);
  gtk_widget_show (button);
  
  gtk_widget_show (vbox1);

  /*  the color panel  */
  gtk_table_attach (GTK_TABLE (table), options->color_panel->color_panel_widget,
		    1, 2, 0, 2, GTK_EXPAND, GTK_EXPAND, 4, 2);

  gtk_widget_show (options->color_panel->color_panel_widget);

  { int i;
    for(i = 0; i < 1; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = options;
#if 0
  action_items[1].user_data = options;
#endif
  build_action_area (GTK_DIALOG (options->query_box), action_items, 1, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}
