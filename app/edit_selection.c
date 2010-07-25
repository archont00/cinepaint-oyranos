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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "draw_core.h"
#include "drawable.h"
#include "tools.h"
#include "edit_selection.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "undo.h"

#define EDIT_SELECT_SCROLL_LOCK 0
#define ARROW_VELOCITY          25

typedef struct EditSelection EditSelection;

struct EditSelection
{
  int                 origx, origy;        /*  original x and y coords            */
  int                 x, y;                /*  current x and y coords             */

  int                 x1, y1;              /*  bounding box of selection mask     */
  int                 x2, y2;

  EditType            edit_type;           /*  translate the mask or layer?       */

  DrawCore *          core;                /*  selection core for drawing bounds  */

  ButtonReleaseFunc   old_button_release;  /*  old button press member func       */
  MotionFunc          old_motion;          /*  old motion member function         */
  ToolCtlFunc         old_control;         /*  old control member function        */
  CursorUpdateFunc    old_cursor_update;   /*  old cursor update function         */
  int                 old_scroll_lock;     /*  old value of scroll lock           */
  int                 old_auto_snap_to;    /*  old value of auto snap to          */

};


/*  static EditSelection structure--there is ever only one present  */
static EditSelection edit_select;


static void
edit_selection_snap (GDisplay *gdisp,
		     int       x,
		     int       y)
{
  int x1, y1;
  int x2, y2;
  int dx, dy;

  gdisplay_untransform_coords (gdisp, x, y, &x, &y, FALSE, TRUE);

  dx = x - edit_select.origx;
  dy = y - edit_select.origy;

  x1 = edit_select.x1 + dx;
  y1 = edit_select.y1 + dy;
  x2 = edit_select.x2 + dx;
  y2 = edit_select.y2 + dy;

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, TRUE);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, TRUE);
  gdisplay_snap_rectangle (gdisp, x1, y1, x2, y2, &x1, &y1);

  gdisplay_untransform_coords (gdisp, x1, y1, &x1, &y1, FALSE, TRUE);

  edit_select.x = x1 - (edit_select.x1 - edit_select.origx);
  edit_select.y = y1 - (edit_select.y1 - edit_select.origy);
}

void
init_edit_selection (Tool           *tool,
		     gpointer        gdisp_ptr,
		     GdkEventButton *bevent,
		     EditType        edit_type)
{
  GDisplay *gdisp;
  Layer *layer;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  Move the (x, y) point from screen to image space  */
  gdisplay_untransform_coords (gdisp,CAST(int) bevent->x,CAST(int) bevent->y, &x, &y, FALSE, TRUE);

  edit_select.x = edit_select.origx = x;
  edit_select.y = edit_select.origy = y;

  /*  Make a check to see if it should be a floating selection translation  */
  if (edit_type == LayerTranslate)
    {
      layer = gimage_get_active_layer (gdisp->gimage);
      if (layer_is_floating_sel (layer))
	edit_type = FloatingSelTranslate;
    }

  edit_select.edit_type = edit_type;

  edit_select.old_button_release = tool->button_release_func;
  edit_select.old_motion         = tool->motion_func;
  edit_select.old_control        = tool->control_func;
  edit_select.old_cursor_update  = tool->cursor_update_func;
  edit_select.old_scroll_lock    = tool->scroll_lock;
  edit_select.old_auto_snap_to   = tool->auto_snap_to;

  /*  find the bounding box of the selection mask--
   *  this is used for the case of a MaskToLayerTranslate,
   *  where the translation will result in floating the selection
   *  mask and translating the resulting layer
   */
  drawable_mask_bounds (gimage_active_drawable (gdisp->gimage),
			&edit_select.x1, &edit_select.y1,
			&edit_select.x2, &edit_select.y2);

  edit_selection_snap (gdisp,CAST(int) bevent->x,CAST(int) bevent->y);

  /*  reset the function pointers on the selection tool  */
  tool->button_release_func = edit_selection_button_release;
  tool->motion_func = edit_selection_motion;
  tool->control_func = edit_selection_control;
  tool->cursor_update_func = edit_selection_cursor_update;
  tool->scroll_lock = EDIT_SELECT_SCROLL_LOCK;
  tool->auto_snap_to = FALSE;

  /*  pause the current selection  */
  selection_pause (gdisp->select);

  /*  Create and start the selection core  */
  edit_select.core = draw_core_new (edit_selection_draw);
  draw_core_start (edit_select.core,
		   gdisp->canvas->window,
		   tool);
}


void
edit_selection_button_release (Tool           *tool,
			       GdkEventButton *bevent,
			       gpointer        gdisp_ptr)
{
  int x, y;
  GDisplay * gdisp;
  Layer *layer;
  Layer *floating_layer;
  GSList *layer_list;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  resume the current selection and ungrab the pointer  */
  selection_resume (gdisp->select);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  Stop and free the selection core  */
  draw_core_stop (edit_select.core, tool);
  draw_core_free (edit_select.core);
  edit_select.core = NULL;
  tool->state      = INACTIVE;

  tool->button_release_func = edit_select.old_button_release;
  tool->motion_func         = edit_select.old_motion;
  tool->control_func        = edit_select.old_control;
  tool->cursor_update_func  = edit_select.old_cursor_update;
  tool->scroll_lock         = edit_select.old_scroll_lock;
  tool->auto_snap_to        = edit_select.old_auto_snap_to;

  /*  If the cancel button is down...Do nothing  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      edit_selection_snap (gdisp,CAST(int) bevent->x,CAST(int) bevent->y);
      x = edit_select.x;
      y = edit_select.y;

      /* if there has been movement, move the selection  */
      if (edit_select.origx != x || edit_select.origy != y)
	{
	  switch (edit_select.edit_type)
	    {
	    case MaskTranslate:
	      /*  translate the selection  */
	      gimage_mask_translate (gdisp->gimage, (x - edit_select.origx),
				     (y - edit_select.origy));
	      break;

	    case MaskToLayerTranslate:
	      gimage_mask_float (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
				 (x - edit_select.origx),
				 (y - edit_select.origy));
	      break;

	    case LayerTranslate:
	      /*  Push a linked undo group  */
	      undo_push_group_start (gdisp->gimage, LINKED_LAYER_UNDO);

	      if ((floating_layer = gimage_floating_sel (gdisp->gimage)))
		floating_sel_relax (floating_layer, TRUE);

	      /*  translate the layer--and any "linked" layers as well  */
	      layer_list = gdisp->gimage->layers;
	      while (layer_list)
		{
		  layer = (Layer *) layer_list->data;
		  if (layer == gdisp->gimage->active_layer || 
		      layer_linked (layer))
		    layer_translate (layer, (x - edit_select.origx), (y - edit_select.origy));
		  layer_list = g_slist_next (layer_list);
		}

	      if (floating_layer)
		floating_sel_rigor (floating_layer, TRUE);

	      /*  End the linked undo group  */
	      undo_push_group_end (gdisp->gimage);
	      break;

	    case FloatingSelTranslate:
	      layer = gimage_get_active_layer (gdisp->gimage);

	      undo_push_group_start (gdisp->gimage, LINKED_LAYER_UNDO);

	      floating_sel_relax (layer, TRUE);
	      layer_translate (layer, (x - edit_select.origx), (y - edit_select.origy));
	      floating_sel_rigor (layer, TRUE);

	      undo_push_group_end (gdisp->gimage);
	      break;
	    }

	}
      /*  if no movement has occured, clear the current selection  */
      else if ((edit_select.edit_type == MaskTranslate) ||
	       (edit_select.edit_type == MaskToLayerTranslate))
	gimage_mask_clear (gdisp->gimage);

      /*  if no movement occured and the type is LayerTranslate,
       *  check if the layer is a floating selection.  If so, anchor.
       */
      else if (edit_select.edit_type == FloatingSelTranslate)
	{
	  layer = gimage_get_active_layer (gdisp->gimage);
	  if (layer_is_floating_sel (layer))
	    floating_sel_anchor (layer);
	}
    }

  gdisplays_flush ();
}


void
edit_selection_motion (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  GDisplay * gdisp;

  if (tool->state != ACTIVE)
    return;

  gdisp = (GDisplay *) gdisp_ptr;

  draw_core_pause (edit_select.core, tool);

  edit_selection_snap (gdisp,CAST(int) mevent->x,CAST(int) mevent->y);

  draw_core_resume (edit_select.core, tool);
}


void
edit_selection_draw (Tool *tool)
{
  int i;
  int diff_x, diff_y;
  GDisplay * gdisp;
  GdkSegment * seg;
  Selection * select;
  Layer *layer;
  GSList *layer_list;
  int floating_sel;
  int x1, y1, x2, y2;
  int x3, y3, x4, y4;
  int off_x, off_y;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  select = gdisp->select;

  diff_x = SCALE (gdisp, (edit_select.x - edit_select.origx));
  diff_y = SCALE (gdisp, (edit_select.y - edit_select.origy));

  switch (edit_select.edit_type)
    {
    case MaskTranslate:
      layer = gimage_get_active_layer (gdisp->gimage);
      floating_sel = layer_is_floating_sel (layer);

      /*  offset the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}
      seg = select->segs_out;
      for (i = 0; i < select->num_segs_out; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}

      if (! floating_sel)
	gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			   select->segs_in, select->num_segs_in);
      gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			 select->segs_out, select->num_segs_out);

      /*  reset the the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      seg = select->segs_out;
      for (i = 0; i < select->num_segs_out; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      break;

    case MaskToLayerTranslate:
      if (diff_x < 0)
/* rsr: seems to be a bug, not initialized. 7.5.03
	{
	  x1 = x1 + 1;
	  x1 --;
	}
*/
      gdisplay_transform_coords (gdisp, edit_select.x1, edit_select.y1, &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp, edit_select.x2, edit_select.y2, &x2, &y2, TRUE);
      gdk_draw_rectangle (edit_select.core->win,
			  edit_select.core->gc, 0,
			  x1 + diff_x, y1 + diff_y,
			  (x2 - x1) - 1, (y2 - y1) - 1);
      break;

    case LayerTranslate:
      gdisplay_transform_coords (gdisp, 0, 0, &x1, &y1, TRUE);
      gdisplay_transform_coords (gdisp,
				 drawable_width ( GIMP_DRAWABLE (gdisp->gimage->active_layer)),
				 drawable_height ( GIMP_DRAWABLE (gdisp->gimage->active_layer)),
				 &x2, &y2, TRUE);

      /*  Now, expand the rectangle to include all linked layers as well  */
      layer_list = gdisp->gimage->layers;
      while (layer_list)
	{
	  layer = (Layer *) layer_list->data;
	  if (((layer) != gdisp->gimage->active_layer) && layer_linked (layer))
	    {
	      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
	      gdisplay_transform_coords (gdisp, off_x, off_y, &x3, &y3, FALSE);
	      gdisplay_transform_coords (gdisp,
					 off_x + drawable_width (GIMP_DRAWABLE (layer)),
					 off_y + drawable_height (GIMP_DRAWABLE (layer)),
					 &x4, &y4, FALSE);
	      if (x3 < x1)
		x1 = x3;
	      if (y3 < y1)
		y1 = y3;
	      if (x4 > x2)
		x2 = x4;
	      if (y4 > y2)
		y2 = y4;
	    }
	  layer_list = g_slist_next (layer_list);
	}

      gdk_draw_rectangle (edit_select.core->win,
			  edit_select.core->gc, 0,
			  x1 + diff_x, y1 + diff_y,
			  (x2 - x1) - 1, (y2 - y1) - 1);
      break;

    case FloatingSelTranslate:
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 += diff_x;
	  seg->x2 += diff_x;
	  seg->y1 += diff_y;
	  seg->y2 += diff_y;
	  seg++;
	}

      /*  Draw the items  */
      gdk_draw_segments (edit_select.core->win, edit_select.core->gc,
			 select->segs_in, select->num_segs_in);

      /*  reset the the current selection  */
      seg = select->segs_in;
      for (i = 0; i < select->num_segs_in; i++)
	{
	  seg->x1 -= diff_x;
	  seg->x2 -= diff_x;
	  seg->y1 -= diff_y;
	  seg->y2 -= diff_y;
	  seg++;
	}
      break;
    }
}


void
edit_selection_control (Tool     *tool,
			int       action,
			gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      draw_core_pause (edit_select.core, tool);
      break;
    case RESUME :
      draw_core_resume (edit_select.core, tool);
      break;
    case HALT :
      draw_core_stop (edit_select.core, tool);
      draw_core_free (edit_select.core);
      break;
    }
}


void
edit_selection_cursor_update (Tool           *tool,
			      GdkEventMotion *mevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
}


void
edit_sel_arrow_keys_func (Tool        *tool,
			  GdkEventKey *kevent,
			  gpointer     gdisp_ptr)
{
  int inc_x, inc_y;
  GDisplay *gdisp;
  Layer *layer;
  Layer *floating_layer;
  GSList *layer_list;
  EditType edit_type;

  layer = NULL;

  gdisp = (GDisplay *) gdisp_ptr;

  inc_x = inc_y = 0;

  switch (kevent->keyval)
    {
    case GDK_Up    : inc_y = -1; break;
    case GDK_Left  : inc_x = -1; break;
    case GDK_Right : inc_x =  1; break;
    case GDK_Down  : inc_y =  1; break;
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & GDK_SHIFT_MASK)
    {
      inc_y *= ARROW_VELOCITY;
      inc_x *= ARROW_VELOCITY;
    }

  if (kevent->state & GDK_MOD1_MASK)
    edit_type = MaskTranslate;
  else
    {
      layer = gimage_get_active_layer (gdisp->gimage);
      if (layer_is_floating_sel (layer))
	edit_type = FloatingSelTranslate;
      else
	edit_type = LayerTranslate;
    }

  switch (edit_type)
    {
    case MaskTranslate:
      /*  translate the selection  */
      gimage_mask_translate (gdisp->gimage, inc_x, inc_y);
      break;

    case MaskToLayerTranslate:
      gimage_mask_float (gdisp->gimage,
			 gimage_active_drawable (gdisp->gimage),
			 inc_x, inc_y);
      break;

    case LayerTranslate:
      /*  Push a linked undo group  */
      undo_push_group_start (gdisp->gimage, LINKED_LAYER_UNDO);

      if ((floating_layer = gimage_floating_sel (gdisp->gimage)))
	floating_sel_relax (floating_layer, TRUE);

      /*  translate the layer--and any "linked" layers as well  */
      layer_list = gdisp->gimage->layers;
      while (layer_list)
	{
	  layer = (Layer *) layer_list->data;
	  if (((layer) == gdisp->gimage->active_layer) || layer_linked (layer))
	    layer_translate (layer, inc_x, inc_y);
	  layer_list = g_slist_next (layer_list);
	}

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      /*  End the linked undo group  */
      undo_push_group_end (gdisp->gimage);
      break;

    case FloatingSelTranslate:
      undo_push_group_start (gdisp->gimage, LINKED_LAYER_UNDO);

      floating_sel_relax (layer, TRUE);

      layer_translate (layer, inc_x, inc_y);

      floating_sel_rigor (layer, TRUE);

      undo_push_group_end (gdisp->gimage);
      break;

    default:
      /*  this won't occur  */
      break;
    }

  gdisplays_flush ();
}
