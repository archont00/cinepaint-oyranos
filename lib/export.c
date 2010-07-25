/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpexport.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                             

#include "config.h"

#include <gtk/gtk.h>

#include "plugin_main.h"
#include "ui.h"
#include "selection_pdb.h"
#include "edit_pdb.h"
#include "image_convert.h"
#include "convert_pdb.h"
#include "dialog.h"
#include "channelops_pdb.h"
#include "export.h"

#define gimp_drawable_is_layer_mask gimp_drawable_layer_mask	
#define gimp_drawable_is_channel gimp_drawable_channel		

typedef void (* ExportFunc) (gint32  imageID,
			     gint32 *drawable_ID);


/* the export action structure */
typedef struct 
{
  ExportFunc  default_action;
  ExportFunc  alt_action;
  gchar      *reason;
  gchar      *possibilities[2];
  gint        choice;
} ExportAction;


/* the functions that do the actual export */

static void
export_merge (gint32  image_ID,
	      gint32 *drawable_ID)
{
  gint32  nlayers;
  gint32  nvisible = 0;
  gint32  i;
  gint32 *layers;
  gint32  merged;
  gint32  transp;

  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (gimp_drawable_visible (layers[i]))
	nvisible++;
    }

  if (nvisible <= 1)
    {
      /* if there is only one (or zero) visible layer, add a new transparent
	 layer that has the same size as the canvas.  The merge that follows
	 will ensure that the offset, opacity and size are correct */
      transp = gimp_layer_new (image_ID, "-",
			       gimp_image_width (image_ID),
			       gimp_image_height (image_ID),
			       gimp_drawable_type (*drawable_ID) | 1,
			       100.0, GIMP_NORMAL_MODE);
      gimp_image_add_layer (image_ID, transp, 1);
      gimp_selection_none (image_ID);
      gimp_edit_clear (transp);
      nvisible++;
    }

  if (nvisible > 1)
    {
      g_free (layers);
      merged = gimp_image_merge_visible_layers (image_ID, GIMP_CLIP_TO_IMAGE);

      if (merged != -1)
	*drawable_ID = merged;
      else
	return;  /* shouldn't happen */
      
      layers = gimp_image_get_layers (image_ID, &nlayers);
    }    
  
  /* remove any remaining (invisible) layers */ 
  for (i = 0; i < nlayers; i++)
    {
      if (layers[i] != *drawable_ID)
	gimp_image_remove_layer (image_ID, layers[i]);
    }
  g_free (layers);  
}

static void
export_flatten (gint32  image_ID,
		gint32 *drawable_ID)
{  
  gint32 flattened;
  
  flattened = gimp_image_flatten (image_ID);

  if (flattened != -1)
    *drawable_ID = flattened;
}

static void
export_convert_rgb (gint32  image_ID,
		    gint32 *drawable_ID)
{  
  gimp_image_convert_rgb (image_ID);
}

static void
export_convert_grayscale (gint32  image_ID,
			  gint32 *drawable_ID)
{  
  gimp_image_convert_grayscale (image_ID);
}

static void
export_convert_indexed (gint32  image_ID,
			gint32 *drawable_ID)
{  
  gint32 nlayers;
  
  /* check alpha */
#ifdef WIN32
  g2_free (gimp_image_get_layers (image_ID, &nlayers));
#else
  g_free (gimp_image_get_layers (image_ID, &nlayers));
#endif
  if (nlayers > 1 || gimp_drawable_has_alpha (*drawable_ID))
    gimp_image_convert_indexed (image_ID, GIMP_FS_DITHER, GIMP_MAKE_PALETTE, 255, FALSE, FALSE, "");
  else
    gimp_image_convert_indexed (image_ID, GIMP_FS_DITHER, GIMP_MAKE_PALETTE, 256, FALSE, FALSE, ""); 
}

static void
export_add_alpha (gint32  image_ID,
		  gint32 *drawable_ID)
{  
  gint32  nlayers;
  gint32  i;
  gint32 *layers;

  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (!gimp_drawable_has_alpha (layers[i]))
	gimp_layer_add_alpha (layers[i]);
    }
  g_free (layers);  
}


/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("can't handle layers"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_merge_single =
{
  export_merge,
  NULL,
  N_("can't handle layer offsets, size or opacity"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  export_merge,
  NULL,
  N_("can only handle layers as animation frames"),
  { N_("Merge Visible Layers"), N_("Save as Animation")},
  0
};

static ExportAction export_action_animate_or_flatten =
{
  export_flatten,
  NULL,
  N_("can only handle layers as animation frames"),
  { N_("Flatten Image"), N_("Save as Animation") },
  0
};

static ExportAction export_action_merge_flat =
{
  export_flatten,
  NULL,
  N_("can't handle layers"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_flatten =
{
  export_flatten,
  NULL,
  N_("can't handle transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  export_convert_rgb,
  NULL,
  N_("can only handle RGB images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  export_convert_grayscale,
  NULL,
  N_("can only handle grayscale images"),
  { N_("Convert to Grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  export_convert_indexed,
  NULL,
  N_("can only handle indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  export_convert_rgb,
  export_convert_grayscale,
  N_("can only handle RGB or grayscale images"),
  { N_("Convert to RGB"), N_("Convert to Grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  export_convert_rgb,
  export_convert_indexed,
  N_("can only handle RGB or indexed images"),
  { N_("Convert to RGB"), N_("Convert to Indexed using default settings\n"
			     "(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  export_convert_indexed,
  export_convert_grayscale,
  N_("can only handle grayscale or indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"), 
    N_("Convert to Grayscale") },
  0
};

static ExportAction export_action_add_alpha =
{
  export_add_alpha,
  NULL,
  N_("needs an alpha channel"),
  { N_("Add Alpha Channel"), NULL},
  0
};


/* dialog functions */

static GtkWidget            *dialog = NULL;
static GimpExportReturnType  dialog_return = GIMP_EXPORT_CANCEL;

static void
export_export_callback (GtkWidget *widget,
			gpointer   data)
{
  gtk_widget_destroy (dialog);
  dialog_return = GIMP_EXPORT_EXPORT;
}

static void
export_confirm_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_widget_destroy (dialog);
  dialog_return = GIMP_EXPORT_EXPORT;
}

static void
export_skip_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_widget_destroy (dialog);
  dialog_return = GIMP_EXPORT_IGNORE;
}

static void
export_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  dialog_return = GIMP_EXPORT_CANCEL;
  dialog = NULL;
  gtk_main_quit ();
}

static void
export_toggle_callback (GtkWidget *widget,
			gpointer   data)
{
  gint *choice = (gint*)data;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *choice = 0;
  else
    *choice = 1;
}

static gint
confirm_save_dialog (const gchar *saving_what,
		     const gchar *format_name)
{
  GtkWidget    *vbox;
  GtkWidget    *label;
  gchar        *text;

  dialog_return = GIMP_EXPORT_CANCEL;
  g_return_val_if_fail (saving_what != NULL && format_name != NULL, dialog_return);

  /*
   *  Plug-ins must have called gtk_init () before calling gimp_export ().
   *  Otherwise bad things will happen now!!
   */

  /* the dialog */

  dialog = gimp_dialog_new (_("Confirm Save"), "confirm_save",
			    gimp_standard_help_func, "dialogs/confirm_save.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, FALSE, FALSE,
			    _("Confirm"), export_confirm_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (export_cancel_callback),
		      NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  text = g_strdup_printf (_("You are about to save %s as %s.\n"
			    "This will not save the visible layers."),
			  saving_what, format_name);
  label = gtk_label_new (text);
  g_free (text);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);
  gtk_main ();

  return dialog_return;
}

static gint
export_dialog (GSList      *actions,
	       const gchar *format_name)
{
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *button;
  GtkWidget    *label;
  GSList       *list;
  gchar        *text;
  ExportAction *action;

  dialog_return = GIMP_EXPORT_CANCEL;
  g_return_val_if_fail (actions != NULL && format_name != NULL, dialog_return);

  /*
   *  Plug-ins must have called gtk_init () before calling gimp_export ().
   *  Otherwise bad things will happen now!!
   */

  /* the dialog */

  dialog = gimp_dialog_new (_("Export File"), "export_file",
			    gimp_standard_help_func, "dialogs/export_file.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, FALSE, FALSE,

			    _("Export"), export_export_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Ignore"), export_skip_callback,
			    NULL, NULL, NULL, FALSE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (export_cancel_callback),
		      NULL);

  /* the headline */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Your image should be exported before it "
			   "can be saved for the following reasons:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  for (list = actions; list; list = list->next)
    {
      action = (ExportAction *) (list->data);

      text = g_strdup_printf ("%s %s", format_name, gettext (action->reason));
      frame = gtk_frame_new (text);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

      if (action->possibilities[0] && action->possibilities[1])
	{
	  GSList *radio_group = NULL;
 
	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      GTK_SIGNAL_FUNC (export_toggle_callback),
			      &action->choice);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	  gtk_widget_show (button);

	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	  gtk_widget_show (button);
	} 
      else if (action->possibilities[0])
	{
	  label = gtk_label_new (gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  gtk_widget_show (label);
	  action->choice = 0;
	}
      else if (action->possibilities[1])
	{
	  label = gtk_label_new (gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  gtk_widget_show (label);
	  action->choice = 1;
	}      
      gtk_widget_show (hbox);
      gtk_widget_show (frame);
    }

  /* the footline */
  label = gtk_label_new (_("The export conversion won't modify your original image."));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);
  gtk_main ();

  return dialog_return;
}

/**
 * gimp_export_image:
 * @image_ID: Pointer to the image_ID.
 * @drawable_ID: Pointer to the drawable_ID.
 * @format_name: The (short) name of the image_format (e.g. JPEG or GIF).
 * @capabilities: What can the image_format do?
 *
 * Takes an image and a drawable to be saved together with a
 * description of the capabilities of the image_format. If the
 * type of image doesn't match the capabilities of the format
 * a dialog is opened that informs the user that the image has
 * to be exported and offers to do the necessary conversions.
 *
 * If the user chooses to export the image, a copy is created.
 * This copy is then converted, the image_ID and drawable_ID
 * are changed to point to the new image and the procedure returns
 * GIMP_EXPORT_EXPORT. The save_plugin has to take care of deleting the
 * created image using gimp_image_delete() when it has saved it.
 *
 * If the user chooses to Ignore the export problem, the image_ID
 * and drawable_ID is not altered, GIMP_EXPORT_IGNORE is returned and 
 * the save_plugin should try to save the original image. If the 
 * user chooses Cancel, GIMP_EXPORT_CANCEL is returned and the 
 * save_plugin should quit itself with status #STATUS_CANCEL.
 *
 * Returns: An enum of #GimpExportReturnType describing the user_action.
 **/
GimpExportReturnType
gimp_export_image (gint32                 *image_ID,
		   gint32                 *drawable_ID,
		   const gchar            *format_name,
		   GimpExportCapabilities  capabilities)
{
  GSList *actions = NULL;
  GSList *list;
  GimpImageBaseType type;
  gint32  i;
  gint32  nlayers;
  gint32* layers;
  gint    offset_x;
  gint    offset_y;
  gboolean added_flatten = FALSE;
  gboolean background_has_alpha = TRUE;
  ExportAction *action;

  g_return_val_if_fail (*image_ID > -1 && *drawable_ID > -1, FALSE);

  /* do some sanity checks */
  if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_ALPHA ;
  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  /* ask for confirmation if the user is not saving a layer (see bug #51114) */
  if (!gimp_drawable_is_layer (*drawable_ID)
      && !(capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
    {
      if (gimp_drawable_is_layer_mask (*drawable_ID))
	dialog_return = confirm_save_dialog (_("a layer mask"),	format_name);
      else if (gimp_drawable_is_channel (*drawable_ID))
	dialog_return = confirm_save_dialog (_("a channel (saved selection)"),
					     format_name);
      else
	; /* this should not happen */

      /* cancel - the user can then select an appropriate layer to save */
      if (dialog_return == GIMP_EXPORT_CANCEL)
	return GIMP_EXPORT_CANCEL;
    }

  /* check alpha */
  layers = gimp_image_get_layers (*image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (gimp_drawable_has_alpha (layers[i]))
	{

	  if ( !(capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA ) )
	    {
	      actions = g_slist_prepend (actions, &export_action_flatten);
	      added_flatten = TRUE;
	      break;
	    }
	}
      else 
	{
          /* If this is the last layer, it's visible and has no alpha
             channel, then the image has a "flat" background */
      	  if (i == nlayers - 1 && gimp_layer_get_visible (layers[i])) 
	    background_has_alpha = FALSE;

	  if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
	    {
	      actions = g_slist_prepend (actions, &export_action_add_alpha);
	      break;
	    }
	}
    }
  g_free (layers);

  /* check if layer size != canvas size, opacity != 100%, or offsets != 0 */
  if (!added_flatten && nlayers == 1 && gimp_drawable_is_layer (*drawable_ID)
      && !(capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
    {
      gint iw = gimp_image_width (*image_ID),
           ih = gimp_image_height (*image_ID),
           dw = gimp_drawable_width(*drawable_ID),
           dh = gimp_drawable_height(*drawable_ID);
      float opacity = gimp_layer_get_opacity (*drawable_ID);
      gimp_drawable_offsets (*drawable_ID, &offset_x, &offset_y);
      if ((opacity < 100.0)
	  || (iw != dw)
	  || (ih != dh)
	  || offset_x || offset_y)
	{
	  if (capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA)
	    actions = g_slist_prepend (actions, &export_action_merge_single);
	  else
	    {
	      actions = g_slist_prepend (actions, &export_action_flatten);
	      added_flatten = TRUE;
	    }
	}
    }
  /* check multiple layers */
  else if (!added_flatten && nlayers > 1)
    {
      if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
	{
	  if (background_has_alpha || capabilities & GIMP_EXPORT_NEEDS_ALPHA)
	    actions = g_slist_prepend (actions, &export_action_animate_or_merge);
	  else
	    actions = g_slist_prepend (actions, &export_action_animate_or_flatten);
	}
      else if ( !(capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
	{
	  if (background_has_alpha || capabilities & GIMP_EXPORT_NEEDS_ALPHA)
	    actions = g_slist_prepend (actions, &export_action_merge);
	  else
	    actions = g_slist_prepend (actions, &export_action_merge_flat);
	}
    }

  /* check the image type */	  
  type = gimp_image_base_type (*image_ID);
  switch (type)
    {
    case GIMP_RGB:
       if ( !(capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) )
	{
	  if ((capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED) && (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
	    actions = g_slist_prepend (actions, &export_action_convert_indexed_or_grayscale);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
	    actions = g_slist_prepend (actions, &export_action_convert_indexed);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
	    actions = g_slist_prepend (actions, &export_action_convert_grayscale);
	}
      break;
    case GIMP_GRAY:
      if ( !(capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY) )
	{
	  if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) && (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED))
	    actions = g_slist_prepend (actions, &export_action_convert_rgb_or_indexed);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
	    actions = g_slist_prepend (actions, &export_action_convert_rgb);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
	    actions = g_slist_prepend (actions, &export_action_convert_indexed);
	}
      break;
    case GIMP_INDEXED:
       if ( !(capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED) )
	{
	  if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) && (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
	    actions = g_slist_prepend (actions, &export_action_convert_rgb_or_grayscale);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
	    actions = g_slist_prepend (actions, &export_action_convert_rgb);
	  else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
	    actions = g_slist_prepend (actions, &export_action_convert_grayscale);
	}
      break;
    }
  
  if (actions)
    {
      actions = g_slist_reverse (actions);
      dialog_return = export_dialog (actions, format_name);
    }
  else
    dialog_return = GIMP_EXPORT_IGNORE;

  if (dialog_return == GIMP_EXPORT_EXPORT)
    {
      *image_ID = gimp_image_duplicate (*image_ID);
      *drawable_ID = gimp_image_get_active_layer (*image_ID);
      gimp_image_undo_disable (*image_ID);
      for (list = actions; list; list = list->next)
	{
	  action = (ExportAction*)(list->data);
	  if (action->choice == 0 && action->default_action)  
	    action->default_action (*image_ID, drawable_ID);
	  else if (action->choice == 1 && action->alt_action)
	    action->alt_action (*image_ID, drawable_ID);
	}
    }
  g_slist_free (actions);

  return dialog_return;
}
