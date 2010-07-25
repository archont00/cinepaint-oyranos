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
#include <string.h>
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "canvas.h"
#include "colormaps.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "rc.h"
#include "general.h"
#include "image_render.h"
#include "interface.h"
#include "layers_dialog.h"
#include "layers_dialogP.h"
#include "ops_buttons.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "resize.h"
#include "undo.h"
#include "fileops.h"
#include "layout.h"
#include "minimize.h"

#include "buttons/eye.xbm"
#include "buttons/linked.xbm"
#include "buttons/layer.xbm"
#include "buttons/mask.xbm"

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
#include "buttons/anchor.xpm"
#include "buttons/anchor_is.xpm"

#include "layer_pvt.h"

#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK

#define LAYER_LIST_WIDTH 200
#define LAYER_LIST_HEIGHT 150

#define LAYER_PREVIEW 0
#define MASK_PREVIEW  1
#define FS_PREVIEW 2

#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

#ifdef DEBUG
#define DBG    printf("%s:%d %s()...\n",__FILE__,__LINE__,__func__);
#else
#define DBG
#endif

typedef struct LayersDialog LayersDialog;

struct LayersDialog {
  GtkWidget *vbox;
  GtkWidget *mode_option_menu;
  GtkWidget *layer_list;
  GtkWidget *preserve_trans;
  GtkWidget *mode_box;
  GtkWidget *opacity_box;
  GtkWidget *ops_menu;
  GtkAccelGroup *accel_group;
  GtkAdjustment *opacity_data;
  GdkGC *red_gc;     /*  for non-applied layer masks  */
  GdkGC *green_gc;   /*  for visible layer masks      */
  GtkWidget *layer_preview;
  double ratio;
  int image_width, image_height;
  int gimage_width, gimage_height;

  /*  state information  */
  int gimage_id;        /*  Why not GImage*? [hsbo] */
  Layer * active_layer;
  Channel * active_channel;
  Layer * floating_sel;
  GSList * layer_widgets;
};

typedef struct LayerWidget LayerWidget;

struct LayerWidget {
  GtkWidget *eye_widget;
  GtkWidget *linked_widget;
  GtkWidget *clip_widget;
  GtkWidget *layer_preview;
  GtkWidget *mask_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  GImage *gimage;
  Layer *layer;
  GdkPixmap *layer_pixmap;
  GdkPixmap *mask_pixmap;
  int active_preview;
  int width, height;

  /*  state information  */
  int layer_mask;
  int apply_mask;
  int edit_mask;
  int show_mask;
  int visited;
};

/*  layers dialog widget routines  */
static void layers_dialog_preview_extents (void);
static void layers_dialog_set_menu_sensitivity (void);
static void layers_dialog_set_active_layer (Layer *);
static void layers_dialog_unset_layer (Layer *);
static void layers_dialog_position_layer (Layer *, int);
static void layers_dialog_add_layer (Layer *);
static void layers_dialog_remove_layer (Layer *);
static void layers_dialog_add_layer_mask (Layer *);
static void layers_dialog_remove_layer_mask (Layer *);
static void paint_mode_menu_callback (GtkWidget *, gpointer);
static void image_menu_callback (GtkWidget *, gpointer);
static void opacity_scale_update (GtkAdjustment *, gpointer);
static void preserve_trans_update (GtkWidget *, gpointer);
static gint layer_list_events (GtkWidget *, GdkEvent *);

/*  layers dialog menu callbacks  */
static void layers_dialog_map_callback (GtkWidget *, gpointer);
static void layers_dialog_unmap_callback (GtkWidget *, gpointer);
static void layers_dialog_new_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_raise_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_lower_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_duplicate_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_delete_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_scale_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_resize_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_add_layer_mask_callback (GtkWidget *, gpointer);
static void layers_dialog_apply_layer_mask_callback (GtkWidget *, gpointer);
static void layers_dialog_anchor_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_merge_layers_callback (GtkWidget *, gpointer);
static void layers_dialog_flatten_image_callback (GtkWidget *, gpointer);
#if 0
static void layers_dialog_alpha_select_callback (GtkWidget *, gpointer);
#endif
static void layers_dialog_mask_select_callback (GtkWidget *, gpointer);
static void layers_dialog_add_alpha_channel_callback (GtkWidget *, gpointer);
static void layers_dialog_save_layer_callback (GtkWidget *, gpointer);
static void layers_dialog_load_layer_callback (GtkWidget *, gpointer);
static gint lc_dialog_close_callback (GtkWidget *, gpointer);

/*  layer widget function prototypes  */
static LayerWidget *layer_widget_get_ID (Layer *);
static LayerWidget *create_layer_widget (GImage *, Layer *);
static void layer_widget_delete (LayerWidget *);
static void layer_widget_select_update (GtkWidget *, gpointer);
static gint layer_widget_button_events (GtkWidget *, GdkEvent *);
static gint layer_widget_preview_events (GtkWidget *, GdkEvent *);
static void layer_widget_boundary_redraw (LayerWidget *, int);
static void layer_widget_preview_redraw (LayerWidget *, int);
static void layer_widget_no_preview_redraw (LayerWidget *, int);
static void layer_widget_eye_redraw (LayerWidget *);
static void layer_widget_linked_redraw (LayerWidget *);
static void layer_widget_clip_redraw (LayerWidget *);
static void layer_widget_exclusive_visible (LayerWidget *);
static void layer_widget_layer_flush (GtkWidget *, gpointer);

/*  assorted query dialogs  */
static void layers_dialog_new_layer_query (int);
static void layers_dialog_edit_layer_query (LayerWidget *);
static void layers_dialog_add_mask_query (Layer *);
static void layers_dialog_apply_mask_query (Layer *);
static void layers_dialog_scale_layer_query (Layer *);
static void layers_dialog_resize_layer_query (Layer *);
void        layers_dialog_layer_merge_query (GImage *, int);


/*
 *  Shared data 
 */

GtkWidget *lc_shell = NULL;     /* declared in layers_dialogP.h */
GtkWidget *lc_subshell = NULL;  /* seems local -hsbo */

/*
 *  Local data
 */
static LayersDialog *layersD = NULL;
static GtkWidget *image_menu;
static GtkWidget *image_option_menu;

static GdkPixmap *eye_pixmap[3] = {NULL, NULL, NULL};
static GdkPixmap *linked_pixmap[3] = {NULL, NULL, NULL};
static GdkPixmap *layer_pixmap[3] = {NULL, NULL, NULL};
static GdkPixmap *mask_pixmap[3] = {NULL, NULL, NULL};

static int suspend_gimage_notify = 0;

static MenuItem layers_ops[] =
{
  { "New Layer", 'N', GDK_CONTROL_MASK,
    layers_dialog_new_layer_callback, NULL, NULL, NULL },
  { "Raise Layer", 'F', GDK_CONTROL_MASK,
    layers_dialog_raise_layer_callback, NULL, NULL, NULL },
  { "Lower Layer", 'B', GDK_CONTROL_MASK,
    layers_dialog_lower_layer_callback, NULL, NULL, NULL },
  { "Duplicate Layer", 'C', GDK_CONTROL_MASK,
    layers_dialog_duplicate_layer_callback, NULL, NULL, NULL },
  { "Delete Layer", 'X', GDK_CONTROL_MASK,
    layers_dialog_delete_layer_callback, NULL, NULL, NULL },
  { "Scale Layer", 'S', GDK_CONTROL_MASK,
    layers_dialog_scale_layer_callback, NULL, NULL, NULL },
  { "Resize Layer", 'R', GDK_CONTROL_MASK,
    layers_dialog_resize_layer_callback, NULL, NULL, NULL },
  { "Add Layer Mask", 0, 0,
    layers_dialog_add_layer_mask_callback, NULL, NULL, NULL },
  { "Apply Layer Mask", 0, 0,
    layers_dialog_apply_layer_mask_callback, NULL, NULL, NULL },
  { "Anchor Layer", 'H', GDK_CONTROL_MASK,
    layers_dialog_anchor_layer_callback, NULL, NULL, NULL },
  { "Merge Visible Layers", 'M', GDK_CONTROL_MASK,
    layers_dialog_merge_layers_callback, NULL, NULL, NULL },
  { "Flatten Image", 0, 0,
    layers_dialog_flatten_image_callback, NULL, NULL, NULL },
#if 0
  { "Alpha To Selection", 0, 0,
    layers_dialog_alpha_select_callback, NULL, NULL, NULL },
#endif
  { "Mask To Selection", 0, 0,
    layers_dialog_mask_select_callback, NULL, NULL, NULL },
  { "Make Layer", 0, 0,
    layers_dialog_add_alpha_channel_callback, NULL, NULL, NULL },
  { "Save Layer", 0, 0,
    layers_dialog_save_layer_callback, NULL, NULL, NULL },
  { "Load Layer", 0, 0,
    layers_dialog_load_layer_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL },

};

/*  the option menu items -- the paint modes  */
static MenuItem option_items[] =
{
  { "Normal", 0, 0, paint_mode_menu_callback, (gpointer) NORMAL_MODE, NULL, NULL },
  //{ "Dissolve", 0, 0, paint_mode_menu_callback, (gpointer) DISSOLVE_MODE, NULL, NULL },
  { "Multiply", 0, 0, paint_mode_menu_callback, (gpointer) MULTIPLY_MODE, NULL, NULL },
  { "Screen", 0, 0, paint_mode_menu_callback, (gpointer) SCREEN_MODE, NULL, NULL },
  { "Overlay", 0, 0, paint_mode_menu_callback, (gpointer) OVERLAY_MODE, NULL, NULL },
  { "Difference", 0, 0, paint_mode_menu_callback, (gpointer) DIFFERENCE_MODE, NULL, NULL },
  { "Addition", 0, 0, paint_mode_menu_callback, (gpointer) ADDITION_MODE, NULL, NULL },
  { "Subtract", 0, 0, paint_mode_menu_callback, (gpointer) SUBTRACT_MODE, NULL, NULL },
  { "Darken Only", 0, 0, paint_mode_menu_callback, (gpointer) DARKEN_ONLY_MODE, NULL, NULL },
  { "Lighten Only", 0, 0, paint_mode_menu_callback, (gpointer) LIGHTEN_ONLY_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/* the ops buttons */
static OpsButton layers_ops_buttons[] =
{
  { new_xpm, new_is_xpm, layers_dialog_new_layer_callback, "New Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { raise_xpm, raise_is_xpm, layers_dialog_raise_layer_callback, "Raise Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { lower_xpm, lower_is_xpm, layers_dialog_lower_layer_callback, "Lower Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { duplicate_xpm, duplicate_is_xpm, layers_dialog_duplicate_layer_callback, "Duplicate Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { delete_xpm, delete_is_xpm, layers_dialog_delete_layer_callback, "Delete Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { anchor_xpm, anchor_is_xpm, layers_dialog_anchor_layer_callback, "Anchor Layer", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


/************************************/
/*  Public layers dialog functions  */
/************************************/

void
lc_dialog_create (int gimage_id)
{
  GtkWidget *util_box;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *notebook;
  GtkWidget *separator;
  int default_index;

# ifdef DEBUG
  printf("%s:%d %s (ImageID=%d)... [lc_shell? %d]\n",__FILE__,__LINE__,__func__, gimage_id, lc_shell ? 1 : 0);
# endif
  
  if (lc_shell == NULL)
    {
      lc_shell = gtk_dialog_new ();

      gtk_window_set_title (GTK_WINDOW (lc_shell), _("Layers & Channels"));
      gtk_window_set_wmclass (GTK_WINDOW (lc_shell), "layers_and_channels", PROGRAM_NAME);
      gtk_widget_set_uposition(lc_shell, layer_channel_x, layer_channel_y);
      layout_connect_window_position(lc_shell, &layer_channel_x, &layer_channel_y, TRUE);
      layout_connect_window_visible(lc_shell, &layer_channel_visible);
      minimize_register(lc_shell);

      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (lc_shell)->vbox), 2);
      gtk_signal_connect (GTK_OBJECT (lc_shell),
			  "delete_event",
			  GTK_SIGNAL_FUNC (lc_dialog_close_callback),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (lc_shell),
			  "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &lc_shell);
      gtk_quit_add_destroy (1, GTK_OBJECT (lc_shell));

      lc_subshell = gtk_vbox_new(FALSE, 2);
      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(lc_shell)->vbox), lc_subshell, TRUE, TRUE, 2);

      /*  The hbox to hold the image option menu box  */
      util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX(lc_subshell), util_box, FALSE, FALSE, 0);

      /*  The GIMP image option menu  */
      label = gtk_label_new (_("Image:"));
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
      image_option_menu = gtk_option_menu_new ();
      image_menu = create_image_menu (&gimage_id, &default_index, image_menu_callback);
      gtk_box_pack_start (GTK_BOX (util_box), image_option_menu, TRUE, TRUE, 2);

      gtk_widget_show (image_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (image_option_menu), image_menu);
      if (default_index != -1)
	gtk_option_menu_set_history (GTK_OPTION_MENU (image_option_menu), default_index);
      gtk_widget_show (label);

      gtk_widget_show (util_box);

      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX(lc_subshell), separator, FALSE, TRUE, 2);
      gtk_widget_show (separator);

      /*  The notebook widget  */
      notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX(lc_subshell), notebook, TRUE, TRUE, 0);

      label = gtk_label_new (_("Layers"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				layers_dialog_create (),
				label);
      gtk_widget_show (label);

      label = gtk_label_new (_("Channels"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				channels_dialog_create (),
				label);
      gtk_widget_show (label);

      gtk_widget_show (notebook);

      gtk_widget_show (lc_subshell);

      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG(lc_shell)->action_area), 1);
      /*  The close button  */
      button = gtk_button_new_with_label (_("Close"));
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG(lc_shell)->action_area), button, TRUE, TRUE, 0);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) lc_dialog_close_callback,
			  GTK_OBJECT (lc_shell));
      gtk_widget_show (button);

      gtk_widget_show (GTK_DIALOG(lc_shell)->action_area);

      layers_dialog_update (gimage_id);
      channels_dialog_update (gimage_id);

      gtk_widget_show (lc_shell);

      gdisplays_flush ();

      /*  Make sure the channels page is realized  */
      gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
      gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (lc_shell))
	gtk_widget_show (lc_shell);
      else
	gdk_window_raise (lc_shell->window);

      layers_dialog_update (gimage_id);
      channels_dialog_update (gimage_id);
      lc_dialog_update_image_list ();
      gdisplays_flush ();
    }
}

void lc_dialog_update (GImage *gimage)  /* hsbo */
{
  if (!lc_shell)
    return;
    
  layersD->gimage_id = -1;  /* enforces rebuild in ...update() */
  layers_dialog_update (gimage->ID);   /* future shift to GImage* args */
  channels_dialog_update (gimage->ID);
  gdisplays_flush ();
}

void
lc_dialog_update_image_list ()
{
  int default_index;
  int default_id;
DBG
  if (lc_shell == NULL)
    return;

  default_id = layersD->gimage_id;
  layersD->gimage_id = -1;   /* enforces full rebuild in .._dialog_update() */
  image_menu = create_image_menu (&default_id, &default_index, image_menu_callback);
/* BUG: hangs here in gifload --rsr */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (image_option_menu), image_menu);

  if (default_index != -1)  /*  list has images  */
    {
      if (! GTK_WIDGET_IS_SENSITIVE (lc_subshell) )
	gtk_widget_set_sensitive (lc_subshell, TRUE);
      gtk_option_menu_set_history (GTK_OPTION_MENU (image_option_menu), default_index);

      if (default_id != layersD->gimage_id)   /* i.e. ``if (default_id != -1)'' */
	{
	  layers_dialog_update (default_id);  /* -> layersD->gimage_id = default_id */
	  channels_dialog_update (default_id);
	  gdisplays_flush ();
	}
    }
  else
    {
      if (GTK_WIDGET_IS_SENSITIVE (lc_subshell))
	gtk_widget_set_sensitive (lc_subshell, FALSE);

      layers_dialog_clear ();
      channels_dialog_clear ();
    }
}


void
lc_dialog_free ()
{ 
  DBG
  if (lc_shell == NULL)
    return;

  layers_dialog_free ();
  channels_dialog_free ();

  gtk_widget_destroy (lc_shell);
}

void
lc_dialog_rebuild (int new_preview_size)
{
  int gimage_id;
  int flag;
DBG
  gimage_id = -1;

  flag = 0;
  if (lc_shell)
    {
      flag = 1;
      gimage_id = layersD->gimage_id;
      lc_dialog_free ();
    }
  preview_size = new_preview_size;
  render_setup (transparency_type, transparency_size);
  if (flag)
    lc_dialog_create (gimage_id);
}


void
layers_dialog_flush ()
{
  GImage *gimage;
  Layer *layer;
  LayerWidget *lw;
  GSList *list;
  int gimage_pos;
  int pos;

# ifdef DEBUG_
  printf("%s:%d %s()... [layersD=%p]\n",__FILE__,__LINE__,__func__, (void*)layersD);
# endif
  
  if (!layersD)
    return;

  if (! (gimage = gimage_get_ID (layersD->gimage_id))) {
    printf("%s(): layersD has unresolvable imageID '%d'\n", __func__, layersD->gimage_id);
    return;
  }

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != layersD->gimage_width) ||
      (gimage->height != layersD->gimage_height))
    {
      layersD->gimage_id = -1;  /* enforces rebuild in ...update() */
      layers_dialog_update (gimage->ID);
    }

  /*  Set all current layer widgets to visited = FALSE  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      lw->visited = FALSE;
      list = g_slist_next (list);
    }

  /*  Add any missing layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      lw = layer_widget_get_ID (layer);

      /*  If the layer isn't in the layer widget list, add it  */
      if (lw == NULL)
	layers_dialog_add_layer (layer);
      else
	lw->visited = TRUE;

      list = g_slist_next (list);
    }

  /*  Remove any extraneous layers  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);
      if (lw->visited == FALSE)
	layers_dialog_remove_layer ((lw->layer));
    }

  /*  Switch positions of items if necessary  */
  list = layersD->layer_widgets;
  pos = 0;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next (list);

      if ((gimage_pos = gimage_get_layer_index (gimage, lw->layer)) != pos)
	layers_dialog_position_layer ((lw->layer), gimage_pos);

      pos++;
    }

  /*  Set the active layer  */
  if (layersD->active_layer != gimage->active_layer)
    layersD->active_layer = gimage->active_layer;

  /*  Set the active channel  */
  if (layersD->active_channel != gimage->active_channel)
    {
      layersD->active_channel = gimage->active_channel;

      /*  If there is an active channel, this list is single select  */
      if (layersD->active_channel != NULL)
	gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list), GTK_SELECTION_SINGLE);
      else
	gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list), GTK_SELECTION_BROWSE);
    }

  /*  set the menus if floating sel status has changed  */
  if (layersD->floating_sel != gimage->floating_sel)
    layersD->floating_sel = gimage->floating_sel;

  layers_dialog_set_menu_sensitivity ();

  gtk_container_foreach (GTK_CONTAINER (layersD->layer_list),
			 layer_widget_layer_flush, NULL);
}


void
layers_dialog_free ()
{
  GSList *list;
  LayerWidget *lw;

  if (layersD == NULL)
    return;

  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);

  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next(list);
      layer_widget_delete (lw);
    }
  layersD->layer_widgets = NULL;
  layersD->active_layer = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel = NULL;

  if (layersD->layer_preview)
    gtk_object_sink (GTK_OBJECT (layersD->layer_preview));
  if (layersD->green_gc)
    gdk_gc_destroy (layersD->green_gc);
  if (layersD->red_gc)
    gdk_gc_destroy (layersD->red_gc);

  if (layersD->ops_menu)
    gtk_object_sink (GTK_OBJECT (layersD->ops_menu));

  g_free (layersD);
  layersD = NULL;
}


/*************************************/
/*  layers dialog widget routines    */
/*************************************/

GtkWidget *
layers_dialog_create ()
{
  GtkWidget *vbox;
  GtkWidget *util_box;
  GtkWidget *button_box;
  GtkWidget *label;
  GtkWidget *menu;
  GtkWidget *slider;
  GtkWidget *listbox;
  int i = 0;

  DBG 

  layers_ops[i].label = _("New Layer"); ++i;
  layers_ops[i].label = _("Raise Layer"); ++i;
  layers_ops[i].label = _("Lower Layer"); ++i;
  layers_ops[i].label = _("Duplicate Layer"); ++i;
  layers_ops[i].label = _("Delete Layer"); ++i;
  layers_ops[i].label = _("Scale Layer"); ++i;
  layers_ops[i].label = _("Resize Layer"); ++i;
  layers_ops[i].label = _("Add Layer Mask"); ++i;
  layers_ops[i].label = _("Apply Layer Mask"); ++i;
  layers_ops[i].label = _("Anchor Layer"); ++i;
  layers_ops[i].label = _("Merge Visible Layers"); ++i;
  layers_ops[i].label = _("Flatten Image"); ++i;
#if 0
  layers_ops[i].label = _("Alpha To Selection"); ++i;
#endif
  layers_ops[i].label = _("Mask To Selection"); ++i;
  layers_ops[i].label = _("Make Layer"); ++i;
  layers_ops[i].label = _("Save Layer"); ++i;
  layers_ops[i].label = _("Load Layer"); ++i;

  i = 0;
  option_items[i].label = _("Normal"); ++i;
  //option_items[i].label = _("Dissolve"); ++i;
  option_items[i].label = _("Multiply"); ++i;
  option_items[i].label = _("Screen"); ++i;
  option_items[i].label = _("Overlay"); ++i;
  option_items[i].label = _("Difference"); ++i;
  option_items[i].label = _("Addition"); ++i;
  option_items[i].label = _("Subtract"); ++i;
  option_items[i].label = _("Darken Only"); ++i;
  option_items[i].label = _("Lighten Only"); ++i;

  i = 0;
  layers_ops_buttons[i].tooltip = _("New Layer");
  layers_ops_buttons[i].tooltip = _("Raise Layer");
  layers_ops_buttons[i].tooltip = _("Lower Layer");
  layers_ops_buttons[i].tooltip = _("Duplicate Layer");
  layers_ops_buttons[i].tooltip = _("Delete Layer");
  layers_ops_buttons[i].tooltip = _("Anchor Layer");


  if (!layersD)
    {
      layersD = g_malloc_zero (sizeof (LayersDialog));
      layersD->layer_preview = NULL;
      layersD->gimage_id = -1;
      layersD->active_layer = NULL;
      layersD->active_channel = NULL;
      layersD->floating_sel = NULL;
      layersD->layer_widgets = NULL;
      layersD->accel_group = gtk_accel_group_new ();
      layersD->green_gc = NULL;
      layersD->red_gc = NULL;

      if (preview_size)
	{
	  layersD->layer_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (layersD->layer_preview), preview_size, preview_size);
	}

      /*  The main vbox  */
      layersD->vbox = vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 2);

      /*  The layers commands pulldown menu  */
      layersD->ops_menu = build_menu (layers_ops, layersD->accel_group);

      /*  The Mode option menu, and the preserve transparency  */
      layersD->mode_box = util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

      label = gtk_label_new (_("Mode:"));
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);

      menu = build_menu (option_items, NULL);
      layersD->mode_option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (util_box), layersD->mode_option_menu, FALSE, FALSE, 2);

      gtk_widget_show (label);
      gtk_widget_show (layersD->mode_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (layersD->mode_option_menu), menu);

      layersD->preserve_trans = gtk_check_button_new_with_label (_("Keep Trans."));
      gtk_box_pack_start (GTK_BOX (util_box), layersD->preserve_trans, FALSE, FALSE, 2);
      gtk_signal_connect (GTK_OBJECT (layersD->preserve_trans), "toggled",
			  (GtkSignalFunc) preserve_trans_update,
			  layersD);
      gtk_widget_show (layersD->preserve_trans);
      gtk_widget_show (util_box);


      /*  Opacity scale  */
      layersD->opacity_box = util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);
      label = gtk_label_new (_("Opacity:"));
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
      layersD->opacity_data = GTK_ADJUSTMENT (gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 1.0, 0.0));
      slider = gtk_hscale_new (layersD->opacity_data);
      gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
      gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
      gtk_box_pack_start (GTK_BOX (util_box), slider, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (layersD->opacity_data), "value_changed",
			  (GtkSignalFunc) opacity_scale_update,
			  layersD);

      gtk_widget_show (label);
      gtk_widget_show (slider);
      gtk_widget_show (util_box);


      /*  The layers listbox  */
      listbox = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_usize (listbox, LAYER_LIST_WIDTH, LAYER_LIST_HEIGHT);
      gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 2);

      layersD->layer_list = gtk_list_new ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox), layersD->layer_list);
      gtk_list_set_selection_mode (GTK_LIST (layersD->layer_list), GTK_SELECTION_BROWSE);
      gtk_signal_connect (GTK_OBJECT (layersD->layer_list), "event",
			  (GtkSignalFunc) layer_list_events,
			  layersD);
      gtk_container_set_focus_vadjustment (GTK_CONTAINER (layersD->layer_list),
					   gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (listbox)));
      GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (listbox)->vscrollbar, GTK_CAN_FOCUS);

      gtk_widget_show (layersD->layer_list);
      gtk_widget_show (listbox);

      /* The ops buttons */

      button_box = ops_button_box_new (lc_shell, tool_tips, layers_ops_buttons);

      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /*  Set up signals for map/unmap for the accelerators  */
      gtk_signal_connect (GTK_OBJECT (layersD->vbox), "map",
			  (GtkSignalFunc) layers_dialog_map_callback,
			  NULL);
      gtk_signal_connect (GTK_OBJECT (layersD->vbox), "unmap",
			  (GtkSignalFunc) layers_dialog_unmap_callback,
			  NULL);

      gtk_widget_show (vbox);
    }

  return layersD->vbox;
}


GtkWidget *
create_image_menu (int              *default_id,
		   int              *default_index,
		   MenuItemCallback  callback)
{
  extern GSList *image_list;

  GImage *gimage;
  GtkWidget *menu_item;
  GtkWidget *menu;
  char *menu_item_label;
  char *image_name;
  GSList *tmp;
  int id;
  int num_items;

  gimage=0;
  menu_item=0;
  menu=0;
  menu_item_label=0;
  image_name=0;
  tmp=0;
  id = -1;
  num_items = 0;

  *default_index = -1;
  menu = gtk_menu_new ();

  tmp = image_list;
  while (tmp)
    {
      gimage = tmp->data;
      tmp = g_slist_next (tmp);

      /*  make sure the default index gets set to _something_, if possible  */
      if (*default_index == -1)
	{
	  id = gimage->ID;
	  *default_index = num_items;
	}

      if (gimage->ID == *default_id)
	{
	  id = *default_id;
	  *default_index = num_items;
	}

      image_name = prune_filename (gimage_filename (gimage));
      menu_item_label = (char *) g_malloc (strlen (image_name) + 15);
      sprintf (menu_item_label, "%s-%d", image_name, gimage->ID);
      menu_item = gtk_menu_item_new_with_label (menu_item_label);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			  (GtkSignalFunc) callback,
			  (gpointer) ((long) gimage->ID));
      gtk_container_add (GTK_CONTAINER (menu), menu_item);
      gtk_widget_show (menu_item);

      g_free (menu_item_label);
      menu_item_label=0;
      num_items ++;
    }

  if (!num_items)
    {
      menu_item = gtk_menu_item_new_with_label (_("[none]"));
      gtk_container_add (GTK_CONTAINER (menu), menu_item);
      gtk_widget_show (menu_item);
    }

  *default_id = id;

  return menu;
}


void
layers_dialog_update (int gimage_id)
{
  GImage *gimage;
  Layer *layer;
  LayerWidget *lw;
  GSList *list;
  GList *item_list;

# ifdef DEBUG
  printf("%s:%d %s(imageID=%d)...\n", __FILE__,__LINE__,__func__, gimage_id);
# endif
  
  if (!layersD || layersD->gimage_id == gimage_id)
    return;

  layersD->gimage_id = gimage_id;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Free all elements in the layers listbox  */
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);

  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      list = g_slist_next(list);
      layer_widget_delete (lw);
    }
  if (layersD->layer_widgets)
    g_message (_("layersD->layer_widgets not empty!"));
  layersD->layer_widgets = NULL;

  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  /*  Find the preview extents  */
  layers_dialog_preview_extents ();

  layersD->active_layer = NULL;
  layersD->active_channel = NULL;
  layersD->floating_sel = NULL;

  list = gimage->layers;
  item_list = NULL;

  while (list)
    {
      /*  create a layer list item  */
      layer = (Layer *) list->data;
      lw = create_layer_widget (gimage, layer);
      layersD->layer_widgets = g_slist_append (layersD->layer_widgets, lw);
      item_list = g_list_append (item_list, lw->list_item);

      list = g_slist_next (list);
    }

  /*  get the index of the active layer  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, 0);

  suspend_gimage_notify--;
}


void
layers_dialog_clear ()
{
  ops_button_box_set_insensitive (layers_ops_buttons);

  layersD->gimage_id = -1;
  gtk_list_clear_items (GTK_LIST (layersD->layer_list), 0, -1);
}

void
render_preview (Canvas    *preview_buf,
		GtkWidget *preview_widget,
                GImage    *gimage,
		int        width,
		int        height,
		int        channel)
{
  PixelArea src, back, dest;
  Canvas *gray_buf = NULL;
  Canvas *rgb_buf = NULL;
  Canvas *eightbit_buf = NULL;
  Canvas *stripped_alpha = NULL;
  Canvas *composite = NULL;
  Canvas *background_buf = NULL;
  Canvas *display_buf = NULL;
  Canvas *preview_buf8 = NULL;
  Canvas *preview_rgb_buf8 = NULL;
  Canvas *preview_tmp = NULL;
  gint affect[4] = {0,0,0,0};
  Tag preview_tag = canvas_tag (preview_buf);
  Alpha a = tag_alpha (preview_tag);
  Format f = tag_format (preview_tag);
  Precision p = tag_precision (preview_tag);

  int color_buf = (GTK_PREVIEW (preview_widget)->type == GTK_PREVIEW_COLOR);
  Format fp = color_buf ? FORMAT_RGB : FORMAT_GRAY;
  Precision pp = PRECISION_U8;
  Tag display_tag = tag_new (pp, fp, a); //ALPHA_NO);

	
  g_return_if_fail (preview_buf != NULL);


  if(channel < 0)
  {
    CMSTransform *transform = NULL;
    GDisplay *disp = gdisplay_get_from_gimage(gimage);
    if(gimage && disp)
      transform =
        /*gimage_get_cms_transform( disp->gimage,
                                  disp->look_profile_pipe,
                                  gdisplay_is_colormanaged(disp),
                                  gdisplay_is_proofed(disp),
                                  gdisplay_get_cms_intent(disp),
                                  gdisplay_get_cms_flags(disp),
                                  gdisplay_get_cms_proof_intent(disp),
                                  disp->cms_expensive_transf );*/
        gdisplay_cms_transform_get( disp, preview_tag );//display_tag );
    if(transform)
    {
      Alpha image_a = canvas_alpha( gimage_projection(gimage) );
      /* copy the original */
      Canvas *image_buf = canvas_new (tag_new (p, f, image_a),
                                      width, height, STORAGE_FLAT);
 
      pixelarea_init (&src, preview_buf,
		    0, 0,
		    0, 0,
		    FALSE);
      pixelarea_init (&dest, image_buf,
		    0, 0,
		    0, 0,
		    FALSE);
      copy_area (&src, &dest);

      pixelarea_init (&dest, image_buf,
		    0, 0,
		    0, 0,
		    FALSE);

      /* Dont transform the alpha mask */
      if(image_a == a)
        cms_transform_area( transform, &dest, &dest );

      preview_buf = preview_tmp = image_buf;

      /* equalise alpha */
      if(image_a != a)
      {
        Canvas *tmp = canvas_new (preview_tag, width, height, STORAGE_FLAT);
 
        pixelarea_init (&src, image_buf,
		    0, 0,
		    0, 0,
		    FALSE);
        pixelarea_init (&dest, tmp,
		    0, 0,
		    0, 0,
		    TRUE);
        canvas_portion_refro (tmp, 0 , 0);
        copy_area (&src, &dest);
        canvas_delete(image_buf);
        preview_buf = preview_tmp = tmp;
      }
    }
  }

  if (p != PRECISION_U8)
  {
    eightbit_buf = canvas_new (tag_new (PRECISION_U8, f, a),
		       width, height,
		       STORAGE_FLAT);

    pixelarea_init (&src, preview_buf,
		    0, 0,
		    0, 0,
		    FALSE);

    pixelarea_init (&dest, eightbit_buf,
		    0, 0,
		    0, 0,
		    TRUE);
    canvas_portion_refro (eightbit_buf, 0 , 0);
    copy_area (&src, &dest);
    pixelarea_init (&src, preview_buf,
		    0, 0,
		    0, 0,
		    FALSE);

    pixelarea_init (&dest, eightbit_buf,
		    0, 0,
		    0, 0,
		    FALSE);
    canvas_portion_unref(eightbit_buf, 0 , 0);
    preview_buf8 = eightbit_buf;
  }
  else 
    preview_buf8 = preview_buf;
  
  if (color_buf)  /* rgb display expected */
  {  
    affect[0] = affect[1] = affect[2] = affect[3] = 0;

    switch (channel)
      {
      case 0:    affect[0] = 1; break;
      case 1:    affect[1] = 1; break;
      case 2:    affect[2] = 1; break;
      case 3:    affect[3] = 1; break;
      default:   affect[0] = 1; break;
      }
    
    /* create a rgb preview buf if its not one already */
    if (f == FORMAT_GRAY)
    {
	rgb_buf = canvas_new (tag_new (PRECISION_U8, FORMAT_RGB, a),
		       width, height,
		       STORAGE_FLAT);
        pixelarea_init (&src, preview_buf8,
		    0, 0,
		    0, 0,
		    FALSE);
        pixelarea_init (&dest, rgb_buf,
		    0, 0,
		    0, 0,
		    TRUE);
        canvas_portion_refro (rgb_buf, 0 , 0);
	copy_area (&src, &dest);
        canvas_portion_unref(rgb_buf, 0 , 0);
	preview_rgb_buf8 = rgb_buf;
    }
    else 
	preview_rgb_buf8 = preview_buf8;	


    /* src is preview_rgb_buf */
    pixelarea_init (&src, preview_rgb_buf8,
		    0, 0,
		    0, 0,
		    FALSE);
    
#define FIXME /* create a checked or black buffer */
    /* back is background_buf for black or checkered */
    background_buf = canvas_new (tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_YES),
		       width, height,
		       STORAGE_FLAT);

    pixelarea_init (&back, background_buf,
		    0, 0,
		    0, 0,
		    FALSE);

    /* dest is composite = preview_buf_rgb composited with background_buf */
    composite = canvas_new (tag_new (PRECISION_U8, FORMAT_RGB, a),
		       width, height,
		       STORAGE_FLAT);
    
    pixelarea_init (&dest, composite,
		    0, 0,
		    0, 0,
		    TRUE);
    
    /* composite back and preview_rgb_buf */ 
    canvas_portion_refro (background_buf, 0 , 0);
    combine_areas (&src, &back, &dest, NULL,
		   NULL, 1.0, NORMAL_MODE, affect,
		   combine_areas_type (pixelarea_tag (&src),
				       pixelarea_tag (&back)), 0);

    canvas_portion_unref(background_buf, 0 , 0);

    if (a == ALPHA_YES)  /* display w/o alpha expected */
    {
	stripped_alpha =  canvas_new (
		       tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO),
		       width, height,
		       STORAGE_FLAT);
	pixelarea_init (&src, composite,
		    0, 0,
		    0, 0,
		    FALSE);
	pixelarea_init (&dest, stripped_alpha,
		    0, 0,
		    0, 0,
		    TRUE);
	canvas_portion_refro (stripped_alpha, 0 , 0);
	copy_area (&src, &dest);
        canvas_portion_unref(stripped_alpha, 0 , 0);
	display_buf = stripped_alpha;
    }
    else
     display_buf = composite;

  }
  else  /* gray display expected */
  {
    pixelarea_init (&src, preview_buf8,
		    0, 0,
		    0, 0,
		    FALSE);
    
    gray_buf = canvas_new (tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO),
		       width, height,
		       STORAGE_FLAT);
    
    pixelarea_init (&dest, gray_buf,
		    0, 0,
		    0, 0,
		    TRUE);
    canvas_portion_refro (gray_buf, 0, 0);
    extract_channel_area (&src, &dest, channel);
    canvas_portion_unref(gray_buf, 0 , 0);
    display_buf = gray_buf; 
  }
  
  /* dump it to screen */
  canvas_portion_refro (display_buf, 0, 0);
  {
    int i;
    for (i = 0; i < height; i++)
      {
        guchar * t = canvas_portion_data (display_buf, 0, i);
        gtk_preview_draw_row (GTK_PREVIEW (preview_widget), t, 0, i, width);
      }
  }
  canvas_portion_unref (display_buf, 0, 0);

  /* clean up */

  if (gray_buf)
    canvas_delete (gray_buf);
  if (stripped_alpha)
    canvas_delete (stripped_alpha);
  if (composite)
    canvas_delete (composite);
  if (background_buf)
    canvas_delete (background_buf);
  if (rgb_buf)
    canvas_delete (rgb_buf);
  if (eightbit_buf)
    canvas_delete (eightbit_buf);
  /*if (preview_tmp)
    canvas_delete (preview_tmp);*/
}

void
render_fs_preview (GtkWidget *widget,
		   GdkPixmap *pixmap)
{
  int w, h;
  int x1, y1, x2, y2;
  GdkPoint poly[6];
  int foldh, foldw;
  int i;

  gdk_window_get_size (pixmap, &w, &h);

  x1 = 2;
  y1 = h / 8 + 2;
  x2 = w - w / 8 - 2;
  y2 = h - 2;
  gdk_draw_rectangle (pixmap, widget->style->bg_gc[GTK_STATE_NORMAL], 1,
		      0, 0, w, h);
  gdk_draw_rectangle (pixmap, widget->style->black_gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));

  foldw = w / 4;
  foldh = h / 4;
  x1 = w / 8 + 2;
  y1 = 2;
  x2 = w - 2;
  y2 = h - h / 8 - 2;

  poly[0].x = x1 + foldw; poly[0].y = y1;
  poly[1].x = x1 + foldw; poly[1].y = y1 + foldh;
  poly[2].x = x1; poly[2].y = y1 + foldh;
  poly[3].x = x1; poly[3].y = y2;
  poly[4].x = x2; poly[4].y = y2;
  poly[5].x = x2; poly[5].y = y1;
  gdk_draw_polygon (pixmap, widget->style->white_gc, 1, poly, 6);

  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y1 + foldh, x1, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1, y2, x2, y2);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x2, y2, x2, y1);
  gdk_draw_line (pixmap, widget->style->black_gc,
		 x1 + foldw, y1, x2, y1);
  for (i = 0; i < foldw; i++)
    gdk_draw_line (pixmap, widget->style->black_gc,
		   x1 + i, y1 + foldh, x1 + i, (foldw == 1) ? y1 :
		   (y1 + (foldh - (foldh * i) / (foldw - 1))));
}


static void
layers_dialog_preview_extents ()
{
  GImage *gimage;

  if (! layersD)
    return;

  gimage = gimage_get_ID (layersD->gimage_id);

  layersD->gimage_width = gimage->width;
  layersD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    layersD->ratio = (double) preview_size / (double) gimage->width;
  else
    layersD->ratio = (double) preview_size / (double) gimage->height;

  if (preview_size)
    {
      layersD->image_width = (int) (layersD->ratio * gimage->width);
      layersD->image_height = (int) (layersD->ratio * gimage->height);
      if (layersD->image_width < 1) layersD->image_width = 1;
      if (layersD->image_height < 1) layersD->image_height = 1;
    }
  else
    {
      layersD->image_width = layer_width;
      layersD->image_height = layer_height;
    }
}

static void
layers_dialog_set_menu_sensitivity ()
{
  gint fs;      /*  floating sel  */
  gint ac;      /*  active channel  */
  gint lm;      /*  layer mask  */
  gint gimage;  /*  is there a gimage  */
  gint lp;      /*  layers present  */
  gint alpha;   /*  alpha channel present  */
  Layer *layer;

  lp = FALSE;

  if (! layersD)
    return;

  if ((layer =  (layersD->active_layer)) != NULL)
    lm = (layer->mask) ? TRUE : FALSE;
  else
    lm = FALSE;

  fs = (layersD->floating_sel == NULL);
  ac = (layersD->active_channel == NULL);
  gimage = (gimage_get_ID (layersD->gimage_id) != NULL);
  alpha = layer && layer_has_alpha (layer);

  if (gimage)
    lp = (gimage_get_ID (layersD->gimage_id)->layers != NULL);

  /* new layer */
  gtk_widget_set_sensitive (layers_ops[0].widget, gimage);
  ops_button_set_sensitive (&layers_ops_buttons[0], gimage);
  /* raise layer */
  gtk_widget_set_sensitive (layers_ops[1].widget, fs && ac && gimage && lp && alpha);
  ops_button_set_sensitive (&layers_ops_buttons[1], fs && ac && gimage && lp && alpha);
  /* lower layer */
  gtk_widget_set_sensitive (layers_ops[2].widget, fs && ac && gimage && lp && alpha);
  ops_button_set_sensitive (&layers_ops_buttons[2], fs && ac && gimage && lp && alpha);
  /* duplicate layer */
  gtk_widget_set_sensitive (layers_ops[3].widget, fs && ac && gimage && lp);
  ops_button_set_sensitive (&layers_ops_buttons[3], fs && ac && gimage && lp);
  /* delete layer */
  gtk_widget_set_sensitive (layers_ops[4].widget, ac && gimage && lp);
  ops_button_set_sensitive (&layers_ops_buttons[4], ac && gimage && lp);
  /* scale layer */
  gtk_widget_set_sensitive (layers_ops[5].widget, ac && gimage && lp);
  /* resize layer */
  gtk_widget_set_sensitive (layers_ops[6].widget, ac && gimage && lp);
  /* add layer mask */
  gtk_widget_set_sensitive (layers_ops[7].widget, fs && ac && gimage && !lm && lp && alpha);
  /* apply layer mask */
  gtk_widget_set_sensitive (layers_ops[8].widget, fs && ac && gimage && lm && lp);
  /* anchor layer */
  gtk_widget_set_sensitive (layers_ops[9].widget, !fs && ac && gimage && lp);
  ops_button_set_sensitive (&layers_ops_buttons[5], !fs && ac && gimage && lp);
  /* merge visible layers */
  gtk_widget_set_sensitive (layers_ops[10].widget, fs && ac && gimage && lp);
  /* flatten image */
  gtk_widget_set_sensitive (layers_ops[11].widget, fs && ac && gimage && lp);
  /* alpha select */
#if 0
  gtk_widget_set_sensitive (layers_ops[12].widget, fs && ac && gimage && lp && alpha);
#endif
  /* mask select */
  gtk_widget_set_sensitive (layers_ops[12].widget, fs && ac && gimage && lm && lp);
  /* add alpha */
  gtk_widget_set_sensitive (layers_ops[13].widget, !alpha);

  /* set mode, preserve transparency and opacity to insensitive if there are no layers  */
  gtk_widget_set_sensitive (layersD->preserve_trans, lp);
  gtk_widget_set_sensitive (layersD->opacity_box, lp);
  gtk_widget_set_sensitive (layersD->mode_box, lp);
}


static void
layers_dialog_set_active_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GtkStateType state;
  int index;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimage_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state != GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_select_item (GTK_LIST (layersD->layer_list), index);
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), layer_widget);
    }

  suspend_gimage_notify--;
}


static void
layers_dialog_unset_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GtkStateType state;
  int index;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  state = layer_widget->list_item->state;
  index = gimage_get_layer_index (layer_widget->gimage, layer);
  if ((index >= 0) && (state == GTK_STATE_SELECTED))
    {
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), NULL);
      gtk_list_unselect_item (GTK_LIST (layersD->layer_list), index);
      gtk_object_set_user_data (GTK_OBJECT (layer_widget->list_item), layer_widget);
    }

  suspend_gimage_notify--;
}


static void
layers_dialog_position_layer (Layer * layer,
			      int new_index)
{
  LayerWidget *layer_widget;
  GList *list = NULL;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the layer from the dialog  */
  list = g_list_append (list, layer_widget->list_item);
  gtk_list_remove_items (GTK_LIST (layersD->layer_list), list);
  layersD->layer_widgets = g_slist_remove (layersD->layer_widgets, layer_widget);

  suspend_gimage_notify--;

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), list, new_index);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets, layer_widget, new_index);
}


static void
layers_dialog_add_layer (Layer *layer)
{
  GImage *gimage;
  GList *item_list;
  LayerWidget *layer_widget;
  int position;

  if (!layersD || !layer)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  item_list = NULL;

  layer_widget = create_layer_widget (gimage, layer);
  item_list = g_list_append (item_list, layer_widget->list_item);

  position = gimage_get_layer_index (gimage, layer);
  layersD->layer_widgets = g_slist_insert (layersD->layer_widgets, layer_widget, position);
  gtk_list_insert_items (GTK_LIST (layersD->layer_list), item_list, position);
}


static void
layers_dialog_remove_layer (Layer * layer)
{
  LayerWidget *layer_widget;
  GList *list = NULL;

  layer_widget = layer_widget_get_ID (layer);

  if (!layersD || !layer_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the requested layer from the dialog  */
  list = g_list_append (list, layer_widget->list_item);
  gtk_list_remove_items (GTK_LIST (layersD->layer_list), list);

  /*  Delete layer widget  */
  layer_widget_delete (layer_widget);

  suspend_gimage_notify--;
}


static void
layers_dialog_add_layer_mask (Layer * layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (! GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    gtk_widget_show (layer_widget->mask_preview);

  layer_widget->active_preview = MASK_PREVIEW;

  gtk_widget_draw (layer_widget->layer_preview, NULL);
}


static void
layers_dialog_remove_layer_mask (Layer * layer)
{
  LayerWidget *layer_widget;

  layer_widget = layer_widget_get_ID (layer);
  if (!layersD || !layer_widget)
    return;

  if (GTK_WIDGET_VISIBLE (layer_widget->mask_preview))
    gtk_widget_hide (layer_widget->mask_preview);

  layer_widget->active_preview = LAYER_PREVIEW;

  gtk_widget_draw (layer_widget->layer_preview, NULL);
}


static void
paint_mode_menu_callback (GtkWidget *w,
			  gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;
  int mode;

  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;
  if (! (layer =  (gimage->active_layer)))
    return;

  /*  If the layer has an alpha channel, set the transparency and redraw  */
  /* twh */
  /*if (layer_has_alpha (layer))
  {*/
    mode = (long) client_data;
    if (layer->mode != mode)
    {
	  layer->mode = mode;

	  drawable_update (GIMP_DRAWABLE(layer), 0, 0, 0, 0);
	  gdisplays_flush ();
    }
  /*}*/
}


static void
image_menu_callback (GtkWidget *w,
		     gpointer   client_data)
{
  if (!lc_shell)
    return;
  if (gimage_get_ID ((long) client_data) != NULL)
    {
      layers_dialog_update ((long) client_data);
      channels_dialog_update ((long) client_data);
      gdisplays_flush ();
    }
}


static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  GImage *gimage;
  Layer *layer;
  gfloat opacity;

  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  if (! (layer =  (gimage->active_layer)))
    return;

  /* up rounding to avoid irritation due to float imprecision */
  opacity = ((int)(adjustment->value*10+0.1)) / 1000.0;
  adjustment->value = opacity * 100.0;
  if (layer->opacity != opacity)
    {
      layer->opacity = opacity;

      drawable_update (GIMP_DRAWABLE(layer),
                       0, 0,
                       0, 0);
      gdisplays_flush ();

    }
}


static void
preserve_trans_update (GtkWidget *w,
		       gpointer   data)
{
  GImage *gimage;
  Layer *layer;

  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  if (! (layer =  (gimage->active_layer)))
    return;

  if (GTK_TOGGLE_BUTTON (w)->active)
    layer->preserve_trans = 1;
  else
    layer->preserve_trans = 0;
}


static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  GtkWidget *event_widget;
  LayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 3)
	    gtk_menu_popup (GTK_MENU (layersD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	  break;

	case GDK_2BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  layers_dialog_edit_layer_query (layer_widget);
	  return TRUE;

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
	    }
	  return TRUE;

	default:
	  break;
	}
    }

  return FALSE;
}


/*****************************/
/*  layers dialog callbacks  */
/*****************************/

static void
layers_dialog_map_callback (GtkWidget *w,
			    gpointer   client_data)
{
  if (!layersD)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_shell),
				    layersD->accel_group);
}

static void
layers_dialog_unmap_callback (GtkWidget *w,
			      gpointer   client_data)
{
  if (!layersD)
    return;

  gtk_window_remove_accel_group (GTK_WINDOW (lc_shell),
				       layersD->accel_group);
}

static void
layers_dialog_new_layer_callback (GtkWidget *w,
				  gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((layer = gimage_floating_sel (gimage)))
    {
      floating_sel_to_layer (layer);

      gdisplays_flush ();
    }
  else
    layers_dialog_new_layer_query (layersD->gimage_id);
}


static void
layers_dialog_raise_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  gimage_raise_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}


static void
layers_dialog_lower_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  gimage_lower_layer (gimage, gimage->active_layer);
  gdisplays_flush ();
}


static void
layers_dialog_duplicate_layer_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;
  Layer *active_layer;
  Layer *new_layer;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

  active_layer = gimage_get_active_layer (gimage);
  new_layer = layer_copy (active_layer, layer_has_alpha (active_layer) /*TRUE*/);
  gimage_add_layer (gimage, new_layer, -1);
  
  /*  end the group undo  */
  undo_push_group_end (gimage);
  gdisplays_update_title (gimage->ID);	  
  gdisplays_flush ();
}


static void
layers_dialog_delete_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  if (! (layer = gimage_get_active_layer (gimage)))
    return;

  /*  if the layer is a floating selection, take special care  */
  if (layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimage_remove_layer (gimage, gimage->active_layer);

  gdisplays_update_title (gimage->ID);	  
  gdisplays_flush ();
}


static void
layers_dialog_scale_layer_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layers_dialog_scale_layer_query (gimage->active_layer);
}


static void
layers_dialog_resize_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layers_dialog_resize_layer_query (gimage->active_layer);
}


static void
layers_dialog_add_layer_mask_callback (GtkWidget *w,
				       gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layers_dialog_add_mask_query (gimage->active_layer);
}


static void
layers_dialog_apply_layer_mask_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  /*  Make sure there is a layer mask to apply  */
  if ((layer =  (gimage->active_layer)) != NULL)
    {
      if (layer->mask)
	layers_dialog_apply_mask_query (gimage->active_layer);
    }
}


static void
layers_dialog_anchor_layer_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  floating_sel_anchor (gimage_get_active_layer (gimage));
  gdisplays_flush ();
}


static void
layers_dialog_merge_layers_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layers_dialog_layer_merge_query (gimage, TRUE);
}


static void
layers_dialog_flatten_image_callback (GtkWidget *w,
				      gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  gimage_flatten (gimage);

  gdisplays_update_title (gimage->ID);	  
  gdisplays_flush ();
}

#if 0
static void
layers_dialog_alpha_select_callback (GtkWidget *w,
				     gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  gimage_mask_layer_alpha (gimage, gimage->active_layer);
  gdisplays_flush ();
}
#endif


static void
layers_dialog_mask_select_callback (GtkWidget *w,
				    gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  gimage_mask_layer_mask (gimage, gimage->active_layer);
  gdisplays_flush ();
}


static void
layers_dialog_add_alpha_channel_callback (GtkWidget *w,
					  gpointer   client_data)
{
  GImage *gimage;
  Layer *layer;

  /*  if there is a currently selected gimage
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  if (! (layer = gimage_get_active_layer (gimage)))
    return;

   /*  Add an alpha channel  */
  layer_add_alpha (layer);
  gdisplays_update_title (gimage->ID);	  
  gdisplays_flush ();
}

static void
layers_dialog_save_layer_callback (GtkWidget *w,
    				   gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layer_save (layersD->gimage_id);

/*  layer_remove_alpha (gimage->active_layer);
 */
  file_save_as_callback (w, client_data); 
}

static void
layers_dialog_load_layer_callback (GtkWidget *w,
    				   gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage, request a new layer
   */
  if (!layersD)
    return;
  if (! (gimage = gimage_get_ID (layersD->gimage_id)))
    return;

  layer_load (gimage);
 
  file_open_callback (w, client_data); 

}

static gint
lc_dialog_close_callback (GtkWidget *w,
			  gpointer   client_data)
{
  if (layersD)
    layersD->gimage_id = -1;

  gtk_widget_hide (lc_shell);

  return TRUE;
}


/****************************/
/*  layer widget functions  */
/****************************/

static LayerWidget *
layer_widget_get_ID (Layer * ID)
{
  LayerWidget *lw;
  GSList *list;

  if (!layersD)
    return NULL;

  list = layersD->layer_widgets;

  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw->layer == ID)
	return lw;

      list = g_slist_next(list);
    }

  return NULL;
}


static LayerWidget *
create_layer_widget (GImage *gimage,
		     Layer  *layer)
{
  LayerWidget *layer_widget;
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;

  list_item = gtk_list_item_new ();

  /*  create the layer widget and add it to the list  */
  layer_widget = (LayerWidget *) g_malloc_zero (sizeof (LayerWidget));
  layer_widget->gimage = gimage;
  layer_widget->layer = layer;
  layer_widget->layer_preview = NULL;
  layer_widget->mask_preview = NULL;
  layer_widget->layer_pixmap = NULL;
  layer_widget->mask_pixmap = NULL;
  layer_widget->list_item = list_item;
  layer_widget->width = -1;
  layer_widget->height = -1;
  layer_widget->layer_mask = (layer->mask != NULL);
  layer_widget->apply_mask = layer->apply_mask;
  layer_widget->edit_mask = layer->edit_mask;
  layer_widget->show_mask = layer->show_mask;
  layer_widget->visited = TRUE;

  if (layer->mask)
    layer_widget->active_preview = (layer->edit_mask) ? MASK_PREVIEW : LAYER_PREVIEW;
  else
    layer_widget->active_preview = LAYER_PREVIEW;

  /*  Need to let the list item know about the layer_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), layer_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) layer_widget_select_update,
		      layer_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      (GtkSignalFunc) layer_widget_select_update,
		      layer_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /* Create the visibility toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  layer_widget->eye_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->eye_widget), eye_width, eye_height);
  gtk_widget_set_events (layer_widget->eye_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->eye_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->eye_widget), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->eye_widget);
  gtk_widget_show (layer_widget->eye_widget);
  gtk_widget_show (alignment);

  /* Create the link toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  layer_widget->linked_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->linked_widget), eye_width, eye_height);
  gtk_widget_set_events (layer_widget->linked_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->linked_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->linked_widget), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->linked_widget);
  gtk_widget_show (layer_widget->linked_widget);
  gtk_widget_show (alignment);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->layer_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->layer_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->layer_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->layer_preview), "event",
		      (GtkSignalFunc) layer_widget_preview_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->layer_preview), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->layer_preview);
  gtk_widget_show (layer_widget->layer_preview);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  layer_widget->mask_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->mask_preview),
			 layersD->image_width + 4, layersD->image_height + 4);
  gtk_widget_set_events (layer_widget->mask_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->mask_preview), "event",
		      (GtkSignalFunc) layer_widget_preview_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->mask_preview), layer_widget);
  gtk_container_add (GTK_CONTAINER (alignment), layer_widget->mask_preview);
  if (layer->mask != NULL)
    gtk_widget_show (layer_widget->mask_preview);

  /*  the layer name label */
  if (layer_is_floating_sel (layer))
    layer_widget->label = gtk_label_new (_("Floating Selection"));
  else
    layer_widget->label = gtk_label_new (GIMP_DRAWABLE(layer)->name);
  gtk_box_pack_start (GTK_BOX (hbox), layer_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (layer_widget->label);

  layer_widget->clip_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (layer_widget->clip_widget), 1, 2);
  gtk_widget_set_events (layer_widget->clip_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (layer_widget->clip_widget), "event",
		      (GtkSignalFunc) layer_widget_button_events,
		      layer_widget);
  gtk_object_set_user_data (GTK_OBJECT (layer_widget->clip_widget), layer_widget);
  gtk_box_pack_start (GTK_BOX (vbox), layer_widget->clip_widget, FALSE, FALSE, 0);
  /*  gtk_widget_show (layer_widget->clip_widget); */

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (layer_widget->list_item);

  return layer_widget;
}


static void
layer_widget_delete (LayerWidget *layer_widget)
{
  if (layer_widget->layer_pixmap)
    gdk_pixmap_unref (layer_widget->layer_pixmap);
  if (layer_widget->mask_pixmap)
    gdk_pixmap_unref (layer_widget->mask_pixmap);

  /*  Remove the layer widget from the list  */
  layersD->layer_widgets = g_slist_remove (layersD->layer_widgets, layer_widget);

  /*  Release the widget  */
  gtk_widget_unref (layer_widget->list_item);
  g_free (layer_widget);
}


static void
layer_widget_select_update (GtkWidget *w,
			    gpointer   data)
{
  LayerWidget *layer_widget;

  if ((layer_widget = (LayerWidget *) data) == NULL)
    return;

  /*  Is the list item being selected?  */
  if (w->state != GTK_STATE_SELECTED)
    return;

  /*  Only notify the gimage of an active layer change if necessary  */
  if (suspend_gimage_notify == 0)
    {
      /*  set the gimage's active layer to be this layer  */
      gimage_set_active_layer (layer_widget->gimage, layer_widget->layer);

      gdisplays_flush ();
    }
}


static gint
layer_widget_button_events (GtkWidget *widget,
			    GdkEvent  *event)
{
  static int button_down = 0;
  static GtkWidget *click_widget = NULL;
  static int old_state;
  static int exclusive;
  LayerWidget *layer_widget;
  GSList *list;
  gint list_length;
  GtkWidget *event_widget;
  GdkEventButton *bevent;
  gint return_val;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  return_val = FALSE;
  
  if (layersD)
    {
      list = layersD->layer_widgets;
      list_length = g_slist_length (list); 
    }
  else
    return return_val;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == layer_widget->eye_widget)
	layer_widget_eye_redraw (layer_widget);
      else if (widget == layer_widget->linked_widget)
	layer_widget_linked_redraw (layer_widget);
      else if (widget == layer_widget->clip_widget)
	layer_widget_clip_redraw (layer_widget);
      break;

    case GDK_BUTTON_PRESS:
      return_val = TRUE;

      bevent = (GdkEventButton *) event;

      if (bevent->button == 3) {
	gtk_menu_popup (GTK_MENU (layersD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	return TRUE;
      }

      button_down = 1;
      click_widget = widget;
      gtk_grab_add (click_widget);

      if (widget == layer_widget->eye_widget)
	{
#if 0
	  if (list_length > 1 || !GIMP_DRAWABLE(layer_widget->layer)->visible)
#endif
          {
	    old_state = GIMP_DRAWABLE(layer_widget->layer)->visible;

	    /*  If this was a shift-click, make all/none visible  */
	    if (event->button.state & GDK_SHIFT_MASK)
	      {
		exclusive = TRUE;
		layer_widget_exclusive_visible (layer_widget);
	      }
	    else
	      {
		exclusive = FALSE;
		GIMP_DRAWABLE(layer_widget->layer)->visible = !GIMP_DRAWABLE(layer_widget->layer)->visible;
		layer_widget_eye_redraw (layer_widget);
	      }
	  }
	}
      else if (widget == layer_widget->linked_widget)
	{
	  old_state = layer_widget->layer->linked;
	  layer_widget->layer->linked = !layer_widget->layer->linked;
	  layer_widget_linked_redraw (layer_widget);
	}
      break;

    case GDK_BUTTON_RELEASE:
      return_val = TRUE;

      button_down = 0;
      gtk_grab_remove (click_widget);

      if (widget == layer_widget->eye_widget)
	{
	  if (exclusive)
	    {
	      gimage_invalidate_preview (layer_widget->gimage);
	      gdisplays_update_area (layer_widget->gimage->ID, 0, 0,
				     layer_widget->gimage->width,
				     layer_widget->gimage->height);
	      gdisplays_flush ();
	    }
	  else if (old_state != GIMP_DRAWABLE(layer_widget->layer)->visible)
	    {
	      /*  Invalidate the gimage preview  */
	      drawable_update (GIMP_DRAWABLE(layer_widget->layer),
                               0, 0,
                               0, 0);
	      gdisplays_flush ();
	    }
	}
      else if ((widget == layer_widget->linked_widget) &&
	       (old_state != layer_widget->layer->linked))
	{
	}
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (button_down && (event_widget == click_widget))
	{
	  if (widget == layer_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  layer_widget_exclusive_visible (layer_widget);
		}
	      else
		{
		  GIMP_DRAWABLE(layer_widget->layer)->visible = !GIMP_DRAWABLE(layer_widget->layer)->visible;
		  layer_widget_eye_redraw (layer_widget);
		}
	    }
	  else if (widget == layer_widget->linked_widget)
	    {
	      layer_widget->layer->linked = !layer_widget->layer->linked;
	      layer_widget_linked_redraw (layer_widget);
	    }
	}
      break;

    default:
      break;
    }

  return return_val;
}


static gint
layer_widget_preview_events (GtkWidget *widget,
			     GdkEvent  *event)
{
  GdkEventExpose *eevent;
  GdkPixmap **pixmap;
  GdkEventButton *bevent;
  LayerWidget *layer_widget;
  int valid;
  int preview_type;
  int sx, sy, dx, dy, w, h;

  pixmap = NULL;
  valid  = FALSE;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if(!layer_widget->gimage || !layer_widget->layer)
  {
# ifdef DEBUG
    printf("%s:%d %s() wrong data or widget\n", __FILE__,__LINE__,__func__);
# endif
    return FALSE;
  }

  if (widget == layer_widget->layer_preview)
    preview_type = LAYER_PREVIEW;
  else if (widget == layer_widget->mask_preview && GTK_WIDGET_VISIBLE (widget))
    preview_type = MASK_PREVIEW;
  else
    return FALSE;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
      pixmap = &layer_widget->layer_pixmap;
      valid = GIMP_DRAWABLE(layer_widget->layer)->preview_valid;
      break;
    case MASK_PREVIEW:
      pixmap = &layer_widget->mask_pixmap;
      valid = GIMP_DRAWABLE(layer_widget->layer->mask)->preview_valid;
      break;
    }

  if (layer_is_floating_sel (layer_widget->layer))
    preview_type = FS_PREVIEW;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      /*  Control-button press disables the application of the mask  */
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3) {
	gtk_menu_popup (GTK_MENU (layersD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	return TRUE;
      }

      if (event->button.state & GDK_CONTROL_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimage_set_layer_mask_apply (layer_widget->gimage, GIMP_DRAWABLE(layer_widget->layer)->ID);
	      gdisplays_flush ();
	    }
	}
      /*  Alt-button press makes the mask visible instead of the layer  */
      else if (event->button.state & GDK_MOD1_MASK)
	{
	  if (preview_type == MASK_PREVIEW)
	    {
	      gimage_set_layer_mask_show (layer_widget->gimage, GIMP_DRAWABLE(layer_widget->layer)->ID);
	      gdisplays_flush ();
	    }
	}
      else if (layer_widget->active_preview != preview_type)
	{
	  gimage_set_layer_mask_edit (layer_widget->gimage, layer_widget->layer,
				      (preview_type == MASK_PREVIEW) ? 1 : 0);
	  gdisplays_flush ();
	}
      break;

    case GDK_EXPOSE:
      if (!preview_size && preview_type != FS_PREVIEW)
	layer_widget_no_preview_redraw (layer_widget, preview_type);
      else
	{
	  if (!valid || !*pixmap)
	    {
	      layer_widget_preview_redraw (layer_widget, preview_type);

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       *pixmap,
			       0, 0, 2, 2,
			       layersD->image_width,
			       layersD->image_height);
	    }
	  else
	    {
	      eevent = (GdkEventExpose *) event;

	      w = eevent->area.width;
	      h = eevent->area.height;

	      if (eevent->area.x < 2)
		{
		  sx = eevent->area.x;
		  dx = 2;
		  w -= (2 - eevent->area.x);
		}
	      else
		{
		  sx = eevent->area.x - 2;
		  dx = eevent->area.x;
		}

	      if (eevent->area.y < 2)
		{
		  sy = eevent->area.y;
		  dy = 2;
		  h -= (2 - eevent->area.y);
		}
	      else
		{
		  sy = eevent->area.y - 2;
		  dy = eevent->area.y;
		}

	      if ((sx + w) >= layersD->image_width)
		w = layersD->image_width - sx;

	      if ((sy + h) >= layersD->image_height)
		h = layersD->image_height - sy;

	      if ((w > 0) && (h > 0))
		gdk_draw_pixmap (widget->window,
				 widget->style->black_gc,
				 *pixmap,
				 sx, sy, dx, dy, w, h);
	    }
	}

      /*  The boundary indicating whether layer or mask is active  */
      layer_widget_boundary_redraw (layer_widget, preview_type);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
layer_widget_boundary_redraw (LayerWidget *layer_widget,
			      int          preview_type)
{
  GtkWidget *widget;
  GdkGC *gc1, *gc2;
  GtkStateType state;

  if (preview_type == LAYER_PREVIEW)
    widget = layer_widget->layer_preview;
  else if (preview_type == MASK_PREVIEW)
    widget = layer_widget->mask_preview;
  else
    return;

  state = layer_widget->list_item->state;
  if (state == GTK_STATE_SELECTED)
    {
      if (layer_widget->active_preview == preview_type)
	gc1 = layer_widget->layer_preview->style->white_gc;
      else
	gc1 = layer_widget->layer_preview->style->bg_gc[GTK_STATE_SELECTED];
    }
  else
    {
      if (layer_widget->active_preview == preview_type)
	gc1 = layer_widget->layer_preview->style->black_gc;
      else
	gc1 = layer_widget->layer_preview->style->white_gc;
    }

  gc2 = gc1;
  if (preview_type == MASK_PREVIEW)
    {
      if (layersD->green_gc == NULL)
	{
	  GdkColor green;
          COLOR16_NEWDATA (green2, gfloat,
                           PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO) = {0.0, 1.0, 0.0};
          COLOR16_INIT (green2);
	  green.pixel = get_color (&green2);
	  layersD->green_gc = gdk_gc_new (widget->window);
	  gdk_gc_set_foreground (layersD->green_gc, &green);
	}
      if (layersD->red_gc == NULL)
	{
	  GdkColor red;
          COLOR16_NEWDATA (red2, gfloat,
                           PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO) = {1.0, 0.0, 0.0};
          COLOR16_INIT (red2);
	  red.pixel = get_color (&red2);
	  layersD->red_gc = gdk_gc_new (widget->window);
	  gdk_gc_set_foreground (layersD->red_gc, &red);
	}

      if (layer_widget->layer->show_mask)
	gc2 = layersD->green_gc;
      else if (! layer_widget->layer->apply_mask)
	gc2 = layersD->red_gc;
    }

  gdk_draw_rectangle (widget->window,
		      gc1, FALSE, 0, 0,
		      layersD->image_width + 3,
		      layersD->image_height + 3);

  gdk_draw_rectangle (widget->window,
		      gc2, FALSE, 1, 1,
		      layersD->image_width + 1,
		      layersD->image_height + 1);
}

static void
layer_widget_preview_redraw (LayerWidget *layer_widget,
			     int          preview_type)
{
  Canvas *preview_buf;
  GdkPixmap **pixmap;
  GtkWidget *widget;
  int offx, offy;

  preview_buf = NULL;
  pixmap = NULL;
  widget = NULL;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
    case FS_PREVIEW:
      widget = layer_widget->layer_preview;
      pixmap = &layer_widget->layer_pixmap;
      break;
    case MASK_PREVIEW:
      widget = layer_widget->mask_preview;
      pixmap = &layer_widget->mask_pixmap;
      break;
    }

  /*  allocate the layer widget pixmap  */
  if (! *pixmap)
    *pixmap = gdk_pixmap_new (widget->window,
			      layersD->image_width,
			      layersD->image_height,
			      -1);

  /*  If this is a floating selection preview, draw the preview  */
  if (preview_type == FS_PREVIEW)
    render_fs_preview (widget, *pixmap);
  /*  otherwise, ask the layer or mask for the preview  */
  else
    {
      /*  determine width and height  */
      layer_widget->width = (int) (layersD->ratio * drawable_width (GIMP_DRAWABLE(layer_widget->layer)));
      layer_widget->height = (int) (layersD->ratio * drawable_height (GIMP_DRAWABLE(layer_widget->layer)));
      if (layer_widget->width < 1) layer_widget->width = 1;
      if (layer_widget->height < 1) layer_widget->height = 1;
      offx = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->offset_x);
      offy = (int) (layersD->ratio * GIMP_DRAWABLE(layer_widget->layer)->offset_y);

      switch (preview_type)
	{
	case LAYER_PREVIEW:
	  preview_buf = layer_preview (layer_widget->layer,
				       layer_widget->width,
				       layer_widget->height);
#if 0
{
  PixelArea area;
  d_printf ("the preview_buf\n");
  pixelarea_init (&area, preview_buf,
                  0, 0,
                  0, 0,
                  FALSE);
  pixelarea_print (&area, 0,0);
}
#endif
	  break;
	case MASK_PREVIEW:
	  preview_buf = layer_mask_preview (layer_widget->layer,
					    layer_widget->width,
					    layer_widget->height);
	  break;
	}

      canvas_fixme_setx (preview_buf, offx);
      canvas_fixme_sety (preview_buf, offy);

      render_preview (preview_buf,
		      layersD->layer_preview,
                      layer_widget->gimage,
		      layersD->image_width,
		      layersD->image_height,
		      -1);


  if(!layer_widget->gimage || !layer_widget->layer || !widget->style ||
     !widget->style->black_gc)
  {
# ifdef DEBUG
    printf("%s:%d %s() wrong data or widget\n", __FILE__,__LINE__,__func__);
# endif
    return FALSE;
  }


      gtk_preview_put (GTK_PREVIEW (layersD->layer_preview),
		       *pixmap, widget->style->black_gc,
		       0, 0, 0, 0, layersD->image_width, layersD->image_height);

      /*  make sure the image has been transfered completely to the pixmap before
       *  we use it again...
       */
      gdk_flush ();
    }
}


static void
layer_widget_no_preview_redraw (LayerWidget *layer_widget,
				int          preview_type)
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

  pixmap_normal      = NULL;
  pixmap_selected    = NULL;
  pixmap_insensitive = NULL;
  widget             = NULL;
  bits               = NULL;
  width              = 0;
  height             = 0;

  state = layer_widget->list_item->state;

  switch (preview_type)
    {
    case LAYER_PREVIEW:
      widget = layer_widget->layer_preview;
      pixmap_normal = &layer_pixmap[NORMAL];
      pixmap_selected = &layer_pixmap[SELECTED];
      pixmap_insensitive = &layer_pixmap[INSENSITIVE];
      bits = (gchar *) layer_bits;
      width = layer_width;
      height = layer_height;
      break;
    case MASK_PREVIEW:
      widget = layer_widget->mask_preview;
      pixmap_normal = &mask_pixmap[NORMAL];
      pixmap_selected = &mask_pixmap[SELECTED];
      pixmap_insensitive = &mask_pixmap[INSENSITIVE];
      bits = (gchar *) mask_bits;
      width = mask_width;
      height = mask_height;
      break;
    }

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
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

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
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
		   pixmap, 0, 0, 2, 2, width, height);
}


static void
layer_widget_eye_redraw (LayerWidget *layer_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &layer_widget->eye_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &layer_widget->eye_widget->style->white;
    }
  else
    color = &layer_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (layer_widget->eye_widget->window, color);

  if (GIMP_DRAWABLE(layer_widget->layer)->visible)
    {
      if (!eye_pixmap[NORMAL])
	{
	  eye_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_NORMAL],
					 &layer_widget->eye_widget->style->white);
	  eye_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_SELECTED],
					 &layer_widget->eye_widget->style->bg[GTK_STATE_SELECTED]);
	  eye_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (layer_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &layer_widget->eye_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &layer_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = eye_pixmap[SELECTED];
	  else
	    pixmap = eye_pixmap[NORMAL];
	}
      else
	pixmap = eye_pixmap[INSENSITIVE];

      gdk_draw_pixmap (layer_widget->eye_widget->window,
		       layer_widget->eye_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, eye_width, eye_height);
    }
  else
    {
      gdk_window_clear (layer_widget->eye_widget->window);
    }
}

static void
layer_widget_linked_redraw (LayerWidget *layer_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &layer_widget->linked_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &layer_widget->linked_widget->style->white;
    }
  else
    color = &layer_widget->linked_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (layer_widget->linked_widget->window, color);

  if (layer_widget->layer->linked)
    {
      if (!linked_pixmap[NORMAL])
	{
	  linked_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_NORMAL],
					 &layer_widget->linked_widget->style->white);
	  linked_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_SELECTED],
					 &layer_widget->linked_widget->style->bg[GTK_STATE_SELECTED]);
	  linked_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (layer_widget->linked_widget->window,
					 (gchar*) linked_bits, linked_width, linked_height, -1,
					 &layer_widget->linked_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &layer_widget->linked_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = linked_pixmap[SELECTED];
	  else
	    pixmap = linked_pixmap[NORMAL];
	}
      else
	pixmap = linked_pixmap[INSENSITIVE];

      gdk_draw_pixmap (layer_widget->linked_widget->window,
		       layer_widget->linked_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, linked_width, linked_height);
    }
  else
    {
      gdk_window_clear (layer_widget->linked_widget->window);
    }
}

static void
layer_widget_clip_redraw (LayerWidget *layer_widget)
{
  GdkColor *color;
  GtkStateType state;

  state = layer_widget->list_item->state;
  color = &layer_widget->clip_widget->style->fg[state];

  gdk_window_set_background (layer_widget->clip_widget->window, color);
  gdk_window_clear (layer_widget->clip_widget->window);
}


static void
layer_widget_exclusive_visible (LayerWidget *layer_widget)
{
  GSList *list;
  LayerWidget *lw;
  int visible = FALSE;

  if (!layersD)
    return;

  /*  First determine if _any_ other layer widgets are set to visible  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw != layer_widget)
	visible |= GIMP_DRAWABLE(lw->layer)->visible;

      list = g_slist_next (list);
    }

  /*  Now, toggle the visibility for all layers except the specified one  */
  list = layersD->layer_widgets;
  while (list)
    {
      lw = (LayerWidget *) list->data;
      if (lw != layer_widget)
	GIMP_DRAWABLE(lw->layer)->visible = !visible;
      else
	GIMP_DRAWABLE(lw->layer)->visible = TRUE;

      layer_widget_eye_redraw (lw);

      list = g_slist_next (list);
    }
}


static void
layer_widget_layer_flush (GtkWidget *widget,
			  gpointer   client_data)
{
  LayerWidget *layer_widget;
  Layer *layer;
  char *name;
  char *label_name;
  int update_layer_preview = FALSE;
  int update_mask_preview = FALSE;

  layer_widget = (LayerWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  layer = layer_widget->layer;

  /*  Set sensitivity  */

  /*  to false if there is a floating selection, and this aint it  */
  if (! layer_is_floating_sel (layer_widget->layer) && layersD->floating_sel != NULL)
    {
      if (GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, FALSE);
    }
  /*  to true if there is a floating selection, and this is it  */
  if (layer_is_floating_sel (layer_widget->layer) && layersD->floating_sel != NULL)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, TRUE);
    }
  /*  to true if there is not floating selection  */
  else if (layersD->floating_sel == NULL)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (layer_widget->list_item))
	gtk_widget_set_sensitive (layer_widget->list_item, TRUE);
    }

  /*  if there is an active channel, unselect layer  */
  if (layersD->active_channel != NULL)
    layers_dialog_unset_layer (layer_widget->layer);
  /*  otherwise, if this is the active layer, set  */
  else if (layersD->active_layer == layer_widget->layer)
    {
      layers_dialog_set_active_layer (layersD->active_layer);
      /*  set the data widgets to reflect this layer's values
       *  1)  The opacity slider
       *  2)  The paint mode menu
       *  3)  The preserve trans button
       */
      layersD->opacity_data->value = layer_widget->layer->opacity * 100.0;
      gtk_signal_emit_by_name (GTK_OBJECT (layersD->opacity_data), "value_changed");
      gtk_option_menu_set_history (GTK_OPTION_MENU (layersD->mode_option_menu),
				   /*  Kludge to deal with the absence of behind and dissolve */
				   ((layer_widget->layer->mode > BEHIND_MODE) ?
				    layer_widget->layer->mode - 2 : layer_widget->layer->mode));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (layersD->preserve_trans),
				   (layer_widget->layer->preserve_trans) ?
				   GTK_STATE_ACTIVE : GTK_STATE_NORMAL);
    }

  if (layer_is_floating_sel (layer_widget->layer))
    name = _("Floating Selection");
  else
    name = GIMP_DRAWABLE(layer_widget->layer)->name;

  /*  we need to set the name label if necessary  */
  gtk_label_get (GTK_LABEL (layer_widget->label), &label_name);
  if (name && strcmp (name, label_name))
    gtk_label_set (GTK_LABEL (layer_widget->label), name);

  /*  show the layer mask preview if necessary  */
  if (layer_widget->layer->mask == NULL && layer_widget->layer_mask)
    {
      layer_widget->layer_mask = FALSE;
      layers_dialog_remove_layer_mask (layer_widget->layer);
    }
  else if (layer_widget->layer->mask != NULL && !layer_widget->layer_mask)
    {
      layer_widget->layer_mask = TRUE;
      layers_dialog_add_layer_mask (layer_widget->layer);
    }

  /*  Update the previews  */
  update_layer_preview = (! GIMP_DRAWABLE(layer)->preview_valid);

  if (layer->mask)
    {
      update_mask_preview = (! GIMP_DRAWABLE(layer->mask)->preview_valid);

      if (layer->apply_mask != layer_widget->apply_mask)
	{
	  layer_widget->apply_mask = layer->apply_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->show_mask != layer_widget->show_mask)
	{
	  layer_widget->show_mask = layer->show_mask;
	  update_mask_preview = TRUE;
	}
      if (layer->edit_mask != layer_widget->edit_mask)
	{
	  layer_widget->edit_mask = layer->edit_mask;

	  if (layer->edit_mask == TRUE)
	    layer_widget->active_preview = MASK_PREVIEW;
	  else
	    layer_widget->active_preview = LAYER_PREVIEW;

	  /*  The boundary indicating whether layer or mask is active  */
	  layer_widget_boundary_redraw (layer_widget, LAYER_PREVIEW);
	  layer_widget_boundary_redraw (layer_widget, MASK_PREVIEW);
	}
    }

  if (update_layer_preview)
    gtk_widget_draw (layer_widget->layer_preview, NULL);
  if (update_mask_preview)
    gtk_widget_draw (layer_widget->mask_preview, NULL);
}


/*
 *  The new layer query dialog
 */

typedef struct NewLayerOptions NewLayerOptions;

struct NewLayerOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *xsize_entry;
  GtkWidget *ysize_entry;
  int fill_type;
  int xsize;
  int ysize;

  int gimage_id;
};

static int fill_type = TRANSPARENT_FILL;
static char *layer_name = NULL;

static void
new_layer_query_ok_callback (GtkWidget *w,
			     gpointer   client_data)
{
  NewLayerOptions *options;
  Layer *layer;
  GImage *gimage;

  options = (NewLayerOptions *) client_data;
  if (layer_name)
    g_free (layer_name);
  layer_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
  fill_type = options->fill_type;
  options->xsize = atoi (gtk_entry_get_text (GTK_ENTRY (options->xsize_entry)));
  options->ysize = atoi (gtk_entry_get_text (GTK_ENTRY (options->ysize_entry)));

  if ((gimage = gimage_get_ID (options->gimage_id)))
    {

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      if (fill_type == 4)
	layer = layer_new (gimage->ID, options->xsize, options->ysize,
	    tag_set_alpha (gimage_tag (gimage),
	      ALPHA_NO),
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
	    layer_name, 1.0, NORMAL_MODE);
      else
	layer = layer_new (gimage->ID, options->xsize, options->ysize,
	    tag_set_alpha (gimage_tag (gimage),
	      ALPHA_YES),
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
	    layer_name, 1.0, NORMAL_MODE);
      if (layer)
	{
	  drawable_fill (GIMP_DRAWABLE(layer), fill_type);
	  gimage_add_layer (gimage, layer, -1);

	  /*  Endx the group undo  */
	  undo_push_group_end (gimage);

	  gdisplays_update_title (gimage->ID);	  
	  gdisplays_flush ();
	}
      else
	{
	  g_message (_("new_layer_query_ok_callback: could not allocate new layer"));
	}
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
new_layer_query_cancel_callback (GtkWidget *w,
				 gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
new_layer_query_delete_callback (GtkWidget *w,
				 GdkEvent *e,
				 gpointer client_data)
{
  new_layer_query_cancel_callback (w, client_data);

  return TRUE;
}


static void
new_layer_background_callback (GtkWidget *w,
			       gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  options->fill_type = BACKGROUND_FILL;
}

static void
new_layer_foreground_callback (GtkWidget *w,
			       gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  options->fill_type = FOREGROUND_FILL;
}

static void
new_layer_fur_callback (GtkWidget *w,
			       gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  options->fill_type = FOREGROUND_FILL;
}

static void
new_layer_white_callback (GtkWidget *w,
			  gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  options->fill_type = WHITE_FILL;
}

static void
new_layer_transparent_callback (GtkWidget *w,
				gpointer   client_data)
{
  NewLayerOptions *options;

  options = (NewLayerOptions *) client_data;
  options->fill_type = TRANSPARENT_FILL;
}

static void
layers_dialog_new_layer_query (int gimage_id)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", new_layer_query_ok_callback, NULL, NULL },
    { "Cancel", new_layer_query_cancel_callback, NULL, NULL }
  };
  GImage *gimage;
  NewLayerOptions *options;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char size[12];
  char *button_names[5];
  ActionCallback button_callbacks[5] =
  {
    new_layer_background_callback,
    new_layer_white_callback,
    new_layer_transparent_callback,
    new_layer_foreground_callback,
    new_layer_fur_callback
  };

  button_names[0] = _("Background");
  button_names[1] = _("White");
  button_names[2] = _("Transparent");
  button_names[3] = _("Foreground");
  button_names[4] = _("Fur/Voo Chan");
  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  gimage = gimage_get_ID (gimage_id);

  /*  the new options structure  */
  options = (NewLayerOptions *) g_malloc_zero (sizeof (NewLayerOptions));
  options->fill_type = fill_type;
  options->gimage_id = gimage_id;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "new_layer_options", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("New Layer Options"));
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (new_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Layer name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 75, 0);
  gtk_table_attach (GTK_TABLE (table), options->name_entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), (layer_name ? layer_name : _("New Layer")));
  gtk_widget_show (options->name_entry);

  /*  the xsize entry hbox, label and entry  */
  sprintf (size, "%d", gimage->width);
  label = gtk_label_new (_("Layer width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);
  options->xsize_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->xsize_entry, 75, 0);
  gtk_table_attach (GTK_TABLE (table), options->xsize_entry, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_entry_set_text (GTK_ENTRY (options->xsize_entry), size);
  gtk_widget_show (options->xsize_entry);

  /*  the ysize entry hbox, label and entry  */
  sprintf (size, "%d", gimage->height);
  label = gtk_label_new (_("Layer height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);
  options->ysize_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->ysize_entry, 75, 0);
  gtk_table_attach (GTK_TABLE (table), options->ysize_entry, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_entry_set_text (GTK_ENTRY (options->ysize_entry), size);
  gtk_widget_show (options->ysize_entry);

  gtk_widget_show (table);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (_("Layer Fill Type"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 5; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) button_callbacks[i],
			  options);

      /*  set the correct radio button  */
      if (i == options->fill_type)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radio_button), TRUE);

      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The edit layer attributes dialog
 */

typedef struct EditLayerOptions EditLayerOptions;

struct EditLayerOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  int        layer_ID;
};

static void
edit_layer_query_ok_callback (GtkWidget *w,
			      gpointer   client_data)
{
  EditLayerOptions *options;
  Layer *layer;

  options = (EditLayerOptions *) client_data;

  if ((layer = layer_get_ID (options->layer_ID)))
    {
      /*  Set the new layer name  */
      if (GIMP_DRAWABLE(layer)->name)
	{
	  /*  If the layer is a floating selection, make it a channel  */
	  if (layer_is_floating_sel (layer))
	    {
	      floating_sel_to_layer (layer);
	    }
	  g_free (GIMP_DRAWABLE(layer)->name);
	}
      GIMP_DRAWABLE(layer)->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
    }

  gdisplays_flush ();

  gtk_widget_destroy (options->query_box);

  g_free (options);
}

static void
edit_layer_query_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  EditLayerOptions *options;

  options = (EditLayerOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
edit_layer_query_delete_callback (GtkWidget *w,
				  GdkEvent *e,
				  gpointer client_data)
{
  edit_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_edit_layer_query (LayerWidget *layer_widget)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", edit_layer_query_ok_callback, NULL, NULL },
    { "Cancel", edit_layer_query_cancel_callback, NULL, NULL }
  };
  EditLayerOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  /*  the new options structure  */
  options = (EditLayerOptions *) g_malloc_zero (sizeof (EditLayerOptions));
  options->layer_ID = drawable_ID (GIMP_DRAWABLE (layer_widget->layer));
  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "edit_layer_attrributes", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Edit Layer Attributes"));
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (edit_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      ((layer_is_floating_sel (layer_widget->layer) ?
			_("Floating Selection") : GIMP_DRAWABLE(layer_widget->layer)->name)));
  gtk_widget_show (options->name_entry);
  gtk_widget_show (hbox);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The add mask query dialog
 */

typedef struct AddMaskOptions AddMaskOptions;

struct AddMaskOptions {
  GtkWidget *query_box;
  Layer * layer;
  AddMaskType add_mask_type;
};

static void
add_mask_query_ok_callback (GtkWidget *w,
			    gpointer   client_data)
{
  AddMaskOptions *options;
  GImage *gimage;
  LayerMask *mask;
  Layer *layer;

  options = (AddMaskOptions *) client_data;
  if ((layer =  (options->layer)) &&
      (gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)))
    {
      mask = layer_create_mask (layer, options->add_mask_type);
      gimage_add_layer_mask (gimage, layer, mask);
      gdisplays_flush ();
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
add_mask_query_cancel_callback (GtkWidget *w,
				gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
add_mask_query_delete_callback (GtkWidget *w,
				GdkEvent  *e,
				gpointer   client_data)
{
  add_mask_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
fill_white_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = WhiteMask;
}

static void
fill_black_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = BlackMask;
}

static void
fill_alpha_callback (GtkWidget *w,
		     gpointer   client_data)
{
  AddMaskOptions *options;

  options = (AddMaskOptions *) client_data;
  options->add_mask_type = AlphaMask;
}

static void
fill_aux_callback (GtkWidget *w,
                   gpointer   client_data)
{
  AddMaskOptions *options;
  options = (AddMaskOptions *) client_data;
  options->add_mask_type = AuxMask; 
}

static void
layers_dialog_add_mask_query (Layer *layer)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", add_mask_query_ok_callback, NULL, NULL },
    { "Cancel", add_mask_query_cancel_callback, NULL, NULL }
  };
  AddMaskOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char *button_names[4];
  ActionCallback button_callbacks[4] =
  {
    fill_white_callback,
    fill_black_callback,
    fill_alpha_callback,
    fill_aux_callback
  };

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  button_names[0] = _("White (Full Opacity)");
  button_names[1] = _("Black (Full Transparency)");
  button_names[2] = _("Layer's Alpha Channel");
  button_names[3] = _("Use Aux Channel");

  /*  the new options structure  */
  options = (AddMaskOptions *) g_malloc_zero (sizeof (AddMaskOptions));
  options->layer = layer;
  options->add_mask_type = WhiteMask;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "add_mask_options", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Add Mask Options"));
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (add_mask_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Initialize Layer Mask To:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 4; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) button_callbacks[i],
			  options);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The apply layer mask dialog
 */

typedef struct ApplyMaskOptions ApplyMaskOptions;

struct ApplyMaskOptions {
  GtkWidget *query_box;
  Layer * layer;
};

static void
apply_mask_query_apply_callback (GtkWidget *w,
				 gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;

  gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE(options->layer)),
			    options->layer, APPLY);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
apply_mask_query_discard_callback (GtkWidget *w,
				   gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;

  gimage_remove_layer_mask (drawable_gimage (GIMP_DRAWABLE(options->layer)),
			    options->layer, DISCARD);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
apply_mask_query_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  ApplyMaskOptions *options;

  options = (ApplyMaskOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
apply_mask_query_delete_callback (GtkWidget *w,
				  GdkEvent  *e,
				  gpointer   client_data)
{
  apply_mask_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_apply_mask_query (Layer *layer)
{
  static ActionAreaItem action_items[3] =
  {
    { "Apply", apply_mask_query_apply_callback, NULL, NULL },
    { "Discard", apply_mask_query_discard_callback, NULL, NULL },
    { "Cancel", apply_mask_query_cancel_callback, NULL, NULL }
  };
  ApplyMaskOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;

  { int i;
    for(i = 0; i < 3; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  /*  the new options structure  */
  options = (ApplyMaskOptions *) g_malloc_zero (sizeof (ApplyMaskOptions));
  options->layer = layer;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "layer_mask_options", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Layer Mask Options"));
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (apply_mask_query_delete_callback),
		      options);


  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new (_("Apply layer mask?"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  action_items[2].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 3, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The scale layer dialog
 */

typedef struct ScaleLayerOptions ScaleLayerOptions;

struct ScaleLayerOptions {
  GtkWidget *query_box;
  Layer * layer;

  Resize *resize;
};


static void
scale_layer_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  ScaleLayerOptions *options;
  GImage *gimage;
  Layer *layer;

  options = (ScaleLayerOptions *) client_data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer =  (options->layer)))
    {
      if ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_SCALE_UNDO);

	  if (layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  layer_scale (layer, options->resize->width, options->resize->height, TRUE);

	  if (layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      gtk_widget_destroy (options->query_box);
      resize_widget_free (options->resize);
      g_free (options);
    }
  else
    g_message (_("Invalid width or height.  Both must be positive."));
}

static void
scale_layer_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  ScaleLayerOptions *options;

  options = (ScaleLayerOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  resize_widget_free (options->resize);
  g_free (options);
}

static gint
scale_layer_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer   client_data)
{
  scale_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_scale_layer_query (Layer *layer)
{
  static ActionAreaItem action_items[3] =
  {
    { "OK", scale_layer_query_ok_callback, NULL, NULL },
    { "Cancel", scale_layer_query_cancel_callback, NULL, NULL }
  };
  ScaleLayerOptions *options;
  GtkWidget *vbox;

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  /*  the new options structure  */
  options = (ScaleLayerOptions *) g_malloc_zero (sizeof (ScaleLayerOptions));
  options->layer = layer;
  options->resize = resize_widget_new (ScaleWidget,
				       drawable_width (GIMP_DRAWABLE(layer)),
				       drawable_height (GIMP_DRAWABLE(layer)));

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "scale_layer", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Scale Layer"));
  gtk_window_set_policy (GTK_WINDOW (options->query_box), FALSE, FALSE, TRUE);
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /*  handle the wm close singal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (scale_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), options->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (options->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The resize layer dialog
 */

typedef struct ResizeLayerOptions ResizeLayerOptions;

struct ResizeLayerOptions {
  GtkWidget *query_box;
  Layer *layer;

  Resize *resize;
};

static void
resize_layer_query_ok_callback (GtkWidget *w,
				gpointer   client_data)
{
  ResizeLayerOptions *options;
  GImage *gimage;
  Layer *layer;

  options = (ResizeLayerOptions *) client_data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer = (options->layer)))
    {
      if ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_RESIZE_UNDO);

	  if (layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  layer_resize (layer,
			options->resize->width, options->resize->height,
			options->resize->off_x, options->resize->off_y);

	  if (layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      gtk_widget_destroy (options->query_box);
      resize_widget_free (options->resize);
      g_free (options);
    }
  else
    g_message (_("Invalid width or height.  Both must be positive."));
}

static void
resize_layer_query_cancel_callback (GtkWidget *w,
				    gpointer   client_data)
{
  ResizeLayerOptions *options;

  options = (ResizeLayerOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  resize_widget_free (options->resize);
  g_free (options);
}

static gint
resize_layer_query_delete_callback (GtkWidget *w,
				    GdkEvent  *e,
				    gpointer   client_data)
{
  resize_layer_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
layers_dialog_resize_layer_query (Layer *layer)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", resize_layer_query_ok_callback, NULL, NULL },
    { "Cancel", resize_layer_query_cancel_callback, NULL, NULL }
  };
  ResizeLayerOptions *options;
  GtkWidget *vbox;

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  /*  the new options structure  */
  options = (ResizeLayerOptions *) g_malloc_zero (sizeof (ResizeLayerOptions));
  options->layer = layer;
  options->resize = resize_widget_new (ResizeWidget,
				       drawable_width (GIMP_DRAWABLE(layer)),
				       drawable_height (GIMP_DRAWABLE(layer)));

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "resize_layer", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Resize Layer"));
  gtk_window_set_policy (GTK_WINDOW (options->query_box), FALSE, TRUE, TRUE);
  gtk_window_set_policy (GTK_WINDOW (options->query_box), FALSE, FALSE, TRUE);
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (resize_layer_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), options->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (options->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The layer merge dialog
 */

typedef struct LayerMergeOptions LayerMergeOptions;

struct LayerMergeOptions {
  GtkWidget *query_box;
  int gimage_id;
  int merge_visible;
  MergeType merge_type;
};

static void
layer_merge_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  LayerMergeOptions *options;
  GImage *gimage;

  options = (LayerMergeOptions *) client_data;
  if (! (gimage = gimage_get_ID (options->gimage_id)))
    return;

  if (options->merge_visible)
    gimage_merge_visible_layers (gimage, options->merge_type, 0);

  gdisplays_update_title (gimage->ID);	  
  gdisplays_flush ();
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
layer_merge_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
layer_merge_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer   client_data)
{
  layer_merge_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
expand_as_necessary_callback (GtkWidget *w,
			      gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = ExpandAsNecessary;
}

static void
clip_to_image_callback (GtkWidget *w,
			gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = ClipToImage;
}

static void
clip_to_bottom_layer_callback (GtkWidget *w,
			       gpointer   client_data)
{
  LayerMergeOptions *options;

  options = (LayerMergeOptions *) client_data;
  options->merge_type = ClipToBottomLayer;
}

void
layers_dialog_layer_merge_query (GImage *gimage,
				 int     merge_visible)  /*  if 0, anchor active layer  */
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", layer_merge_query_ok_callback, NULL, NULL },
    { "Cancel", layer_merge_query_cancel_callback, NULL, NULL }
  };
  LayerMergeOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;
  int i;
  char *button_names[3];
  ActionCallback button_callbacks[3] =
  {
    expand_as_necessary_callback,
    clip_to_image_callback,
    clip_to_bottom_layer_callback
  };

  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  button_names[0] = _("Expanded as necessary");
  button_names[1] = _("Clipped to image");
  button_names[2] = _("Clipped to bottom layer");

  /*  the new options structure  */
  options = (LayerMergeOptions *) g_malloc_zero (sizeof (LayerMergeOptions));
  options->gimage_id = gimage->ID;
  options->merge_visible = merge_visible;
  options->merge_type = ExpandAsNecessary;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "layer_merge_options", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Layer Merge Options"));
  gtk_widget_set_uposition(options->query_box, generic_window_x, generic_window_y);
  layout_connect_window_position(options->query_box, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(options->query_box);

  /* hadle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (layer_merge_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (options->query_box), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  if (merge_visible)
    label = gtk_label_new (_("Final, merged layer should be:"));
  else
    label = gtk_label_new (_("Final, anchored layer should be:"));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) button_callbacks[i],
			  options);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

