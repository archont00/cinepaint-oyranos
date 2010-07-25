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
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "general.h"

#include "interface.h"
#include "layer.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "undo.h"
#include "rc.h"
#include "layout.h"
#include "minimize.h"

#include "global_edit.h"

#define TileManager Canvas 
#define tile_manager_destroy canvas_delete

/*  The named paste dialog  */
typedef struct PasteNamedDlg PasteNamedDlg;
struct PasteNamedDlg
{
  GtkWidget   *shell;
  GtkWidget   *list;
  int          paste_into;
  GDisplay    *gdisp;
};

/*  The named buffer structure...  */
typedef struct NamedBuffer NamedBuffer;

struct NamedBuffer
{
  TileManager * buf;
  char *        name;
};


/*  The named buffer list  */
GSList * named_buffers = NULL;

/*  The global edit buffer  */
TileManager * global_buf = NULL;


/*  Crop the buffer to the size of pixels with non-zero transparency */

TileManager *
crop_buffer (TileManager *tiles,
	     int          border)
{
  PixelArea PR;
  TileManager *new_tiles;
  int num_channels, alpha;
  guchar * data;
  int empty;
  int x1, y1, x2, y2;
  int x, y;
  int ex, ey;
  int found;
  void * pr;
  Tag    tag          = tag_alpha (canvas_tag (tiles));
  gint   has_alpha    = (tag==ALPHA_YES) ? TRUE: FALSE;

  COLOR16_NEW (black_color, canvas_tag(tiles) );
  COLOR16_INIT (black_color);
  palette_get_black (&black_color);

  //num_channels = tag_num_channels (canvas_tag (tiles))*2;
  //IMAGEWORKS aland 1/4/02 I don't know why they were multiplying this by 2....
  num_channels = tag_num_channels (canvas_tag (tiles));
  //alpha = num_channels - 1;
  //IMAGEWORKS aland 1/4/02  This checks to see if there really is an alpha rather than assume....
  alpha = has_alpha ? num_channels - 1 : num_channels;
 

  /*  go through and calculate the bounds  */
  x1 = canvas_width (tiles);
  y1 = canvas_height (tiles);
  x2 = 0;
  y2 = 0;

  pixelarea_init (&PR, tiles, 
	0, 0, x1, y1, FALSE);
  for (pr = pixelarea_register (1, &PR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&PR) + alpha;
      ex = pixelarea_x (&PR) + pixelarea_width (&PR);
      ey = pixelarea_y (&PR) + pixelarea_height (&PR);
      
      for (y = pixelarea_y (&PR); y < ey; y++)
        {
          found = FALSE;
          for (x = pixelarea_x (&PR); x < ex; x++, data+=num_channels)
            if (*data)
              {
            if (x < x1)
              x1 = x;
            if (x > x2)
              x2 = x;
            found = TRUE;
              }
          if (found)
            {
              if (y < y1)
            y1 = y;
              if (y > y2)
            y2 = y;
            }
        }
    }

#if 0
  /* THIS WAS HERE, BUT CAUSED THE TEXT TOOL TO FAIL!  - SR*/
  x2 = BOUNDS (x2-x1 + 1, 0, canvas_width (tiles));
  y2 = BOUNDS (y2-y1 + 1, 0, canvas_height (tiles));
#endif
  x2 = CLAMP (x2 + 1, 0, canvas_width(tiles));
  y2 = CLAMP (y2 + 1, 0, canvas_height(tiles));

  empty = (x1 == canvas_width (tiles) && y1 == canvas_height (tiles));

  /*  If there are no visible pixels, return NULL */
  if (empty)
    new_tiles = NULL;
  /*  If no cropping, return original buffer  */
  else if (x1 == 0 && y1 == 0 && x2 == canvas_width (tiles) &&
	   y2 == canvas_height (tiles) && border == 0)
    new_tiles = tiles;
  /*  Otherwise, crop the original area  */
  else
    {
      PixelArea srcPR, destPR;
      int new_width, new_height;

      new_width = (x2 - x1) + border * 2;
      new_height = (y2 - y1) + border * 2;
      new_tiles = canvas_new (canvas_tag (tiles), new_width, new_height, STORAGE_FLAT);

      /*  If there is a border, make sure to clear the new tiles first  */
      if (border)
	{
	  pixelarea_init (&destPR, new_tiles, 
		0, 0, new_width, border, TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_tiles, 
		0, border, border, (y2 - y1), TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_tiles,
		new_width - border, border, border, (y2 - y1), TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_tiles,
		0, new_height - border, new_width, border, TRUE);
	  color_area (&destPR, &black_color);
	}


      pixelarea_init (&srcPR, tiles, 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&destPR, new_tiles, 
		border, border, (x2 - x1), (y2 - y1), TRUE);

      copy_area (&srcPR, &destPR);
/*
	we dont have a way of setting this in canvas....
      tiles->x = x1;
      tiles->y = y1;
*/
    }

  return new_tiles;
}

TileManager *
edit_cut (GImage *gimage,
	  CanvasDrawable *drawable)
{
  TileManager *cut;
  TileManager *cropped_cut;
  int empty;

  if (!gimage || drawable == NULL)
    return NULL;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_CUT_UNDO);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  Next, cut the mask portion from the gimage  */
  cut = gimage_mask_extract (gimage, drawable, TRUE, FALSE);

  /*  Only crop if the gimage mask wasn't empty  */
  if (cut && empty == FALSE)
    {
      cropped_cut = crop_buffer (cut, 0);

      if (cropped_cut != cut)
	tile_manager_destroy (cut);
    }
  else if (cut)
    cropped_cut = cut;
  else
    cropped_cut = NULL;

  /*  end the group undo  */
  undo_push_group_end (gimage);

  if (cropped_cut)
    {
      /*  Free the old global edit buffer  */
      if (global_buf)
	tile_manager_destroy (global_buf);
      /*  Set the global edit buffer  */
      global_buf = cropped_cut;

      return cropped_cut;
    }
  else
    return NULL;
}

TileManager *
edit_copy (GImage *gimage,
	   CanvasDrawable *drawable)
{
  TileManager * copy;
  TileManager * cropped_copy;
  int empty;

  if (!gimage || drawable == NULL)
    return NULL;

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  First, copy the masked portion of the gimage  */
  copy = gimage_mask_extract (gimage, drawable, FALSE, FALSE);

  /*  Only crop if the gimage mask wasn't empty  */
  if (copy && empty == FALSE)
    {
      cropped_copy = crop_buffer (copy, 0);

      if (cropped_copy != copy)
       tile_manager_destroy (copy);
    }
  else if (copy)
    cropped_copy = copy;
  else
    cropped_copy = NULL;

  if (cropped_copy)
    {
      /*  Free the old global edit buffer  */
      if (global_buf)
	tile_manager_destroy (global_buf);
      /*  Set the global edit buffer  */
      global_buf = cropped_copy;

      return cropped_copy;
    }
  else
    return NULL;
}

int
edit_paste (GImage      *gimage,
	    CanvasDrawable *drawable,
	    TileManager *paste,
	    int          paste_into)
{
  Layer * float_layer;
  int x1, y1, x2, y2;
  int cx, cy;

  /*  Make a new floating layer  */
  float_layer = layer_from_tiles (gimage, drawable, paste, _("Pasted Layer"), OPAQUE_OPACITY, NORMAL);

  if (float_layer)
    {
      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      /*  Set the offsets to the center of the image  */
      drawable_offsets ( (drawable), &cx, &cy);
      drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
      
      if (!enable_paste_c_disp)
	{
      cx += (x1 + x2) >> 1;
      cy += (y1 + y2) >> 1;
#if 0
      GIMP_DRAWABLE(float_layer)->offset_x = cx - (drawable_width (GIMP_DRAWABLE(float_layer)) >> 1);
      GIMP_DRAWABLE(float_layer)->offset_y = cy - (drawable_height (GIMP_DRAWABLE(float_layer)) >> 1);
#endif
	}
      else
	{
	  
	  GDisplay *disp = gdisplay_active ();

      cx = (float) (disp->offset_x + disp->disp_width /2) / ((float)SCALEDEST (disp) / (float)SCALESRC (disp));
      cy = (float) (disp->offset_y + disp->disp_height /2 )/ ((float)SCALEDEST (disp) / (float)SCALESRC (disp));
	}
      GIMP_DRAWABLE(float_layer)->offset_x = cx - (drawable_width (GIMP_DRAWABLE(float_layer)) >> 1);
      GIMP_DRAWABLE(float_layer)->offset_y = cy - (drawable_height (GIMP_DRAWABLE(float_layer)) >> 1);

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimage_mask_is_empty (gimage) && !paste_into)
	channel_clear (gimage_get_mask (gimage));

      /*  add a new floating selection  */
      floating_sel_attach (float_layer, drawable);

      /*  end the group undo  */
      undo_push_group_end (gimage);

      return GIMP_DRAWABLE(float_layer)->ID;
    }
  else
    return 0;
}

int
edit_clear (GImage *gimage,
	    CanvasDrawable *drawable)
{
  TileManager *buf_tiles;
  PixelArea bufPR;
  int x1, y1, x2, y2;
  Tag d_tag = drawable_tag (drawable);
  COLOR16_NEW (background_color, d_tag);
  COLOR16_INIT (background_color);
  palette_get_background (&background_color);

  if (!gimage || drawable == NULL)
    return FALSE;

  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return FALSE;

  buf_tiles = canvas_new (d_tag, (x2 - x1), (y2 - y1), STORAGE_FLAT);
  pixelarea_init (&bufPR, buf_tiles, 
	0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_area (&bufPR, &background_color);

  gimage_apply_painthit(gimage, drawable,
                        NULL, buf_tiles,
                        0, 0,
                        (x2 - x1), (y2 - y1),
                        1, 1.0, 
			ERASE_MODE, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}

int
edit_fill (GImage *gimage,
	   CanvasDrawable *drawable)
{
  TileManager *buf_tiles;
  PixelArea bufPR;
  int x1, y1, x2, y2;
  Tag d_tag = drawable_tag (drawable);
  COLOR16_NEW (background_color, d_tag);
  COLOR16_INIT (background_color);
  palette_get_background (&background_color);

  if (!gimage || drawable == NULL)
    return FALSE;

  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return FALSE;

  buf_tiles = canvas_new (d_tag, (x2 - x1), (y2 - y1), STORAGE_FLAT);
  pixelarea_init (&bufPR, buf_tiles, 
	0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_area (&bufPR, &background_color);

  gimage_apply_painthit (gimage, drawable,
                         NULL, buf_tiles,
                         0, 0,
                         (x2 - x1), (y2 - y1),
                         1, 1.0, NORMAL_MODE, x1, y1); 

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}

int
global_edit_cut (void *gdisp_ptr)
{
  GDisplay *gdisp;

  /*  stop any active tool  */
  gdisp = (GDisplay *) gdisp_ptr;
  active_tool_control (HALT, gdisp_ptr);

  if (!edit_cut (gdisp->gimage, gimage_active_drawable (gdisp->gimage)))
    return FALSE;
  else
    {
      /*  flush the display  */
      gdisplays_flush ();
      return TRUE;
    }
}

int
global_edit_copy (void *gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  if (!edit_copy (gdisp->gimage, gimage_active_drawable (gdisp->gimage)))
    return FALSE;
  else
    return TRUE;
}

int
global_edit_paste (void *gdisp_ptr,
		   int   paste_into)
{
  GDisplay *gdisp;

  /*  stop any active tool  */
  gdisp = (GDisplay *) gdisp_ptr;
  active_tool_control (HALT, gdisp_ptr);

  if (!edit_paste (gdisp->gimage, gimage_active_drawable (gdisp->gimage), global_buf, paste_into))
    return FALSE;
  else
    {
      /*  flush the display  */
      gdisplays_flush ();
      return TRUE;
    }
}

void
global_edit_free ()
{
  if (global_buf)
    tile_manager_destroy (global_buf);

  global_buf = NULL;
}

/*********************************************/
/*        Named buffer operations            */

static void
set_list_of_named_buffers (GtkWidget *list_widget)
{
  GSList *list;
  NamedBuffer *nb;
  GtkWidget *list_item;

  gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);
  list = named_buffers;

  while (list)
    {
      nb = (NamedBuffer *) list->data;
      list = g_slist_next (list);

      list_item = gtk_list_item_new_with_label (nb->name);
      gtk_container_add (GTK_CONTAINER (list_widget), list_item);
      gtk_widget_show (list_item);
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) nb);
    }
}

static void
named_buffer_paste_foreach (GtkWidget *w,
			    gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  NamedBuffer *nb;

  if (w->state == GTK_STATE_SELECTED)
    {
      pn_dlg = (PasteNamedDlg *) client_data;
      nb = (NamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));
      edit_paste (pn_dlg->gdisp->gimage,
		  gimage_active_drawable (pn_dlg->gdisp->gimage),
		  nb->buf, pn_dlg->paste_into);
    }
}

static void
named_buffer_paste_callback (GtkWidget *w,
			     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 named_buffer_paste_foreach, client_data);

  /*  Destroy the box  */
  gtk_widget_destroy (pn_dlg->shell);

  g_free (pn_dlg);
      
  /*  flush the display  */
  gdisplays_flush ();
}

static void
named_buffer_delete_foreach (GtkWidget *w,
			     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  NamedBuffer * nb;

  if (w->state == GTK_STATE_SELECTED)
    {
      pn_dlg = (PasteNamedDlg *) client_data;
      nb = (NamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));
      named_buffers = g_slist_remove (named_buffers, (void *) nb);
      g_free (nb->name);
      tile_manager_destroy (nb->buf);
      g_free (nb);
    }
}

static void
named_buffer_delete_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;
  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 named_buffer_delete_foreach, client_data);
  set_list_of_named_buffers (pn_dlg->list);
}

static void
named_buffer_cancel_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  /*  Destroy the box  */
  gtk_widget_destroy (pn_dlg->shell);

  g_free (pn_dlg);
}

static gint
named_buffer_dialog_delete_callback (GtkWidget *w,
				     GdkEvent  *e,
				     gpointer   client_data)
{
  named_buffer_cancel_callback (w, client_data);

  return TRUE;
}

static void
named_buffer_paste_into_update (GtkWidget *w,
				gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    pn_dlg->paste_into = FALSE;
  else
    pn_dlg->paste_into = TRUE;
}

static void
paste_named_buffer (GDisplay *gdisp)
{
  static ActionAreaItem action_items[3] =
  {
    { "Paste", named_buffer_paste_callback, NULL, NULL },
    { "Delete", named_buffer_delete_callback, NULL, NULL },
    { "Cancel", named_buffer_cancel_callback, NULL, NULL }
  };
  PasteNamedDlg *pn_dlg;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *paste_into;
  GtkWidget *listbox;

  { int i;
    for(i = 0; i < 3; ++i)
      action_items[i].label = _(action_items[i].label);
  }

  pn_dlg = (PasteNamedDlg *) g_malloc_zero (sizeof (PasteNamedDlg));
  pn_dlg->gdisp = gdisp;

  pn_dlg->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (pn_dlg->shell), "paste_named_buffer", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (pn_dlg->shell), _("Paste Named Buffer"));
  gtk_widget_set_uposition(pn_dlg->shell, generic_window_x, generic_window_y);
  layout_connect_window_position(pn_dlg->shell, &generic_window_x, &generic_window_y, FALSE);
  minimize_register(pn_dlg->shell);

  gtk_signal_connect (GTK_OBJECT (pn_dlg->shell), "delete_event",
		      GTK_SIGNAL_FUNC (named_buffer_dialog_delete_callback),
		      pn_dlg);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pn_dlg->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Select a buffer to paste:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 150);
  gtk_widget_show (listbox);

  pn_dlg->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (pn_dlg->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox), pn_dlg->list);
  set_list_of_named_buffers (pn_dlg->list);
  gtk_widget_show (pn_dlg->list);

  paste_into = gtk_check_button_new_with_label (_("Replace Current Selection"));
  gtk_box_pack_start (GTK_BOX (vbox), paste_into, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (paste_into), "toggled",
		      (GtkSignalFunc) named_buffer_paste_into_update,
		      pn_dlg);
  gtk_widget_show (paste_into);

  action_items[0].user_data = pn_dlg;
  action_items[1].user_data = pn_dlg;
  action_items[2].user_data = pn_dlg;
  build_action_area (GTK_DIALOG (pn_dlg->shell), action_items, 3, 0);

  gtk_widget_show (pn_dlg->shell);
}

static void
new_named_buffer (TileManager *tiles,
                 char        *name)
{
  PixelArea srcPR, destPR;
  NamedBuffer *nb;

  if (! tiles) return;

  nb = (NamedBuffer *) g_malloc (sizeof (NamedBuffer));

  nb->buf  = canvas_new (canvas_tag (tiles), 
	 		canvas_width (tiles), canvas_height (tiles), 
			STORAGE_FLAT);
  pixelarea_init (&srcPR, tiles, 
		0, 0, canvas_width (tiles), canvas_height (tiles), FALSE);
  pixelarea_init (&destPR, nb->buf, 
		0, 0, canvas_width (tiles), canvas_height (tiles), TRUE);
  copy_area (&srcPR, &destPR);

  nb->name = g_strdup ((char *) name);
  named_buffers = g_slist_append (named_buffers, (void *) nb);
}

static void
cut_named_buffer_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  TileManager *new_tiles;
  GDisplay *gdisp;
  char *name;

  gdisp = (GDisplay *) client_data;
  name = g_strdup ((char *) call_data);
  
  new_tiles = edit_cut (gdisp->gimage, gimage_active_drawable (gdisp->gimage));
  if (new_tiles) 
    new_named_buffer (new_tiles, name);
  gdisplays_flush ();
}

int
named_edit_cut (void *gdisp_ptr)
{
  GDisplay *gdisp;

  /*  stop any active tool  */
  gdisp = (GDisplay *) gdisp_ptr;
  active_tool_control (HALT, gdisp_ptr);

  query_string_box (_("Cut Named"), _("Enter a name for this buffer"), NULL,
		    cut_named_buffer_callback, gdisp);
  return TRUE;
}

static void
copy_named_buffer_callback (GtkWidget *w,
			    gpointer   client_data,
			    gpointer   call_data)
{
  TileManager *new_tiles;
  GDisplay *gdisp;
  char *name;

  gdisp = (GDisplay *) client_data;
  name = g_strdup ((char *) call_data);
  
  new_tiles = edit_copy (gdisp->gimage, gimage_active_drawable (gdisp->gimage));
  if (new_tiles) 
    new_named_buffer (new_tiles, name);
}

int
named_edit_copy (void *gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  
  query_string_box (_("Copy Named"), _("Enter a name for this buffer"), NULL,
		    copy_named_buffer_callback, gdisp);
  return TRUE;
}

int
named_edit_paste (void *gdisp_ptr)
{
  paste_named_buffer ((GDisplay *) gdisp_ptr);

  gdisplays_flush();

  return TRUE;
}

void
named_buffers_free ()
{
  GSList *list;
  NamedBuffer * nb;

  list = named_buffers;

  while (list)
    {
      nb = (NamedBuffer *) list->data;
      tile_manager_destroy (nb->buf);
      g_free (nb->name);
      g_free (nb);
      list = g_slist_next (list);
    }

  g_slist_free (named_buffers);
  named_buffers = NULL;
}
