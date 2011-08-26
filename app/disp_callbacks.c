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
#include "colormaps.h"
#include "cursorutil.h"
#include "devices.h"
#include "disp_callbacks.h"
#include "gdisplay.h"
#include "general.h"
#include "rc.h"
#include "interface.h"
#include "layer_select.h"
#include "move.h"
#include "scale.h"
#include "scroll.h"
#include "tools.h"
#include "gimage.h"
#include "devices.h"
#include "tools.h"
#include "clone.h"
#include "zoom.h"
#include "info_window.h"
#include "layers_dialog.h"      /* hsbo */
#include "gtk_debug_helpers.h"  /* hsbo */

#ifdef GDK_WINDOWING_X11
#include "X11/Xcm/Xcm.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#endif

#define HORIZONTAL  1
#define VERTICAL    2

/* force a busy cursor */
#define BUSY_OFF   0
#define BUSY_UP    1
#define BUSY_ON    2
#define BUSY_DOWN  3

#ifdef DEBUG_
#define DISP_DEBUG
#define DEBUG_DISPLAY 1
#endif

static guint gdisp_busy = BUSY_OFF;

int middle_mouse_button; 

/* Function declarations */

static void gdisplay_check_device_cursor (GDisplay *gdisp);
static void gdisplay_set_colour_region( GDisplay * gdisp );

static void
redraw (GDisplay *gdisp,
	int       x,
	int       y,
	int       w,
	int       h)
{
  long x1, y1, x2, y2;    /*  coordinate of rectangle corners  */

  x1 = x;
  y1 = y;
  x2 = (x+w);
  y2 = (y+h);

  x1 = BOUNDS (x1, 0, gdisp->disp_width);
  y1 = BOUNDS (y1, 0, gdisp->disp_height);
  x2 = BOUNDS (x2, 0, gdisp->disp_width);
  y2 = BOUNDS (y2, 0, gdisp->disp_height);

  if ((x2 - x1) && (y2 - y1))
    {
      gdisplay_expose_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
      gdisplays_flush ();
    }
}

const char*
cp_event_type_name(GdkEvent *event)
{
  const char *t = "---";
# define CASE_TO_TEXT( num, text ) case num: text = #num; break;
  switch (event->type)
  {
  CASE_TO_TEXT ( GDK_PROXIMITY_OUT, t)
  CASE_TO_TEXT ( GDK_PROXIMITY_IN, t)
  CASE_TO_TEXT ( GDK_BUTTON_PRESS, t)
  CASE_TO_TEXT ( GDK_2BUTTON_PRESS, t)
  CASE_TO_TEXT ( GDK_3BUTTON_PRESS, t)
  CASE_TO_TEXT ( GDK_BUTTON_RELEASE, t)
  CASE_TO_TEXT ( GDK_MOTION_NOTIFY, t)
  CASE_TO_TEXT ( GDK_NOTHING ,t)
  CASE_TO_TEXT ( GDK_DELETE ,t)
  CASE_TO_TEXT ( GDK_DESTROY ,t)
  CASE_TO_TEXT ( GDK_EXPOSE ,t)
  CASE_TO_TEXT ( GDK_KEY_RELEASE ,t)
  CASE_TO_TEXT ( GDK_KEY_PRESS ,t)
  CASE_TO_TEXT ( GDK_ENTER_NOTIFY ,t)
  CASE_TO_TEXT ( GDK_LEAVE_NOTIFY ,t)
  CASE_TO_TEXT ( GDK_FOCUS_CHANGE ,t)
  CASE_TO_TEXT ( GDK_CONFIGURE ,t)
  CASE_TO_TEXT ( GDK_MAP ,t)
  CASE_TO_TEXT ( GDK_UNMAP ,t)
  CASE_TO_TEXT ( GDK_PROPERTY_NOTIFY ,t)
  CASE_TO_TEXT ( GDK_SELECTION_CLEAR ,t)
  CASE_TO_TEXT ( GDK_SELECTION_REQUEST ,t)
  CASE_TO_TEXT ( GDK_SELECTION_NOTIFY ,t)
  CASE_TO_TEXT ( GDK_DRAG_ENTER ,t)
  CASE_TO_TEXT ( GDK_DRAG_LEAVE ,t)
  CASE_TO_TEXT ( GDK_DRAG_MOTION ,t)
  CASE_TO_TEXT ( GDK_DRAG_STATUS ,t)
  CASE_TO_TEXT ( GDK_DROP_START ,t)
  CASE_TO_TEXT ( GDK_DROP_FINISHED ,t)
  CASE_TO_TEXT ( GDK_CLIENT_EVENT ,t)
  CASE_TO_TEXT ( GDK_VISIBILITY_NOTIFY ,t)
  CASE_TO_TEXT ( GDK_NO_EXPOSE ,t)
#if GTK_MAJOR_VERSION > 1
  CASE_TO_TEXT ( GDK_SCROLL ,t)
  CASE_TO_TEXT ( GDK_WINDOW_STATE ,t)
  CASE_TO_TEXT ( GDK_SETTING ,t)
#endif
#if GTK_MAJOR_VERSION > 1 && GTK_MINOR_VERSION > 6
  CASE_TO_TEXT ( GDK_OWNER_CHANGE ,t)
  CASE_TO_TEXT ( GDK_GRAB_BROKEN ,t)
#endif
  }
  return t;
}

gint
gdisplay_shell_events (GtkWidget *widget,
                       GdkEvent  *event,
                       GDisplay  *gdisp)
{
#if DEBUG_DISPLAY
  char *t = cp_event_type_name(event);
  int id = gdisplay_to_ID(gdisp);
  printf("%s (\"%s\", displayID=%d)...\n",__func__, t, id);
#endif         
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_KEY_PRESS:
#if 0
      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_context_get_user (), gdisp);
#endif 
      break;
    default:
      break;
    }

  return FALSE;
}

gboolean    
gdisplay_menubar_down (GtkWidget *widget,
                       GdkEventButton *event,
                       gpointer user_data)
{
  GDisplay * disp;
  disp = (GDisplay *)user_data;   

  if(gdisplay_to_ID(disp) >= -1) {
    gdisplay_set_menu_sensitivity (disp);
    zoom_set_focus(disp);
  } else
    g_warning("%s:%d %s() gdisplay not found",__FILE__,__LINE__,__func__);

  return FALSE;
}

static void
gdisplay_check_device_cursor (GDisplay *gdisp)
{
  GList *tmp_list;

  /* gdk_input_list_devices returns an internal list, so we shouldn't
     free it afterwards */

#if GTK_MAJOR_VERSION > 1
      tmp_list = gdk_devices_list();
#else
      tmp_list = gdk_input_list_devices();
#endif

  while (tmp_list)
    {
#if GTK_MAJOR_VERSION > 1
      GdkDevice *info = (GdkDevice *)tmp_list->data;
      if (info == current_device)
#else
      GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;
      if (CAST(int)info->deviceid == current_device)
#endif	  

	{
	  gdisp->draw_cursor = !info->has_cursor;
	  break;
	}
      
      tmp_list = tmp_list->next;
    }
}

#define OY_DBG_FORMAT_ "%s:%d %s() "
#define OY_DBG_ARGS_ __FILE__,__LINE__,strrchr(__func__,'/')?strrchr(__func__,'/')+1:__func__

static void gdisplay_set_colour_region(GDisplay * gdisp)
{
#ifdef GDK_WINDOWING_X11
  gint       i,j;

  if(!gdk_window_is_visible(gdisp->canvas->window))
    return;

  if( gdisp->old_disp_geometry[0] != gdisp->disp_xoffset ||
      gdisp->old_disp_geometry[1] != gdisp->disp_yoffset ||
      gdisp->old_disp_geometry[2] != gdisp->disp_width ||
      gdisp->old_disp_geometry[3] != gdisp->disp_height )
  {
    GdkDisplay *display = gdk_display_get_default ();
    GdkWindow * event_box = gtk_widget_get_window(gdisp->canvas);
    GdkWindow * top_window = gdk_window_get_toplevel(event_box);
    Window w = GDK_WINDOW_XID(top_window);
    Display    *xdisplay;
    GdkScreen  *screen;
    int offx = 0, offy = 0, offx2 = 0, offy2 = 0;

    gdk_window_get_origin( event_box, &offx, &offy );
    gdk_window_get_origin( top_window, &offx2, &offy2 );
    xdisplay = gdk_x11_display_get_xdisplay (display);
    screen  = gdk_screen_get_default ();

    {
      XRectangle rec[2] = { { 0,0,0,0 }, { 0,0,0,0 } },
               * rect = 0;
      int nRect = 0;
      XserverRegion reg = 0;
      XcolorRegion region, *old_regions = 0;
      unsigned long old_regions_n = 0;
      int pos = -1;
      const char * display_string = DisplayString(xdisplay);
      int dim_corr_x, dim_corr_y,
          inner_dis_x, inner_dist_y;
      int error;
      Atom netColorTarget;

      inner_dis_x = offx - offx2;
      inner_dist_y = offy - offy2;
      rec[0].x = gdisp->disp_xoffset + inner_dis_x;
      rec[0].y = gdisp->disp_yoffset + inner_dist_y;
      dim_corr_x = 2 * rec[0].x - 2 * inner_dis_x;
      dim_corr_y = 2 * rec[0].y - 2 * inner_dist_y;
      rec[0].width = gdisp->disp_width - dim_corr_x;
      rec[0].height = gdisp->disp_height - dim_corr_y;

      reg = XFixesCreateRegion( xdisplay, rec, 1);
      rect = XFixesFetchRegion( xdisplay, reg, &nRect );
      if(!nRect)
      {
        printf( OY_DBG_FORMAT_
                 "Display: %s Window id: %d  Could not load Xregion:%d\n",
                 OY_DBG_ARGS_,
                 display_string, (int)w, (int)reg );

      } else if(rect[0].x != rec[0].x ||
                rect[0].y != rec[0].y )
      {
        printf( OY_DBG_FORMAT_
                 "Display: %s Window id: %d  Xregion:%d has wrong position %d,%d\n",
                 OY_DBG_ARGS_,
                 display_string, (int)w, (int)reg, rect[0].x, rect[0].y );
      } else
        printf( OY_DBG_FORMAT_
                 "Display: %s Window id: %d  Xregion:%d uploaded %dx%d+%d+%d"
                 "  %d:%d %d:%d\n",
                 OY_DBG_ARGS_,
                 display_string, (int)w, (int)reg,
                 rect[0].width, rect[0].height, rect[0].x, rect[0].y,
                 offx, offx2, offy, offy2 );

      region.region = htonl(reg);
      memset( region.md5, 0, 16 );

      /* look for old regions */
      old_regions = XcolorRegionFetch( xdisplay, w, &old_regions_n );
     /* remove our own old region */
      for(i = 0; i < old_regions_n; ++i)
      {

        if(!old_regions[i].region || pos >= 0)
          break;

        rect = XFixesFetchRegion( xdisplay, ntohl(old_regions[i].region),
                                  &nRect );

        for(j = 0; j < nRect; ++j)
        {
          int * old_window_rectangle = gdisp->old_disp_geometry;

          printf( OY_DBG_FORMAT_
                 "reg[%d]: %dx%d+%d+%d %dx%d+%d+%d\n",
                 OY_DBG_ARGS_, i,
                 old_window_rectangle[2], old_window_rectangle[3],
                 old_window_rectangle[0], old_window_rectangle[1],
                 rect[j].width, rect[j].height, rect[j].x, rect[j].y
                );
          if(old_window_rectangle[0] == rect[j].x &&
             old_window_rectangle[1] == rect[j].y &&
             old_window_rectangle[2] == rect[j].width &&
             old_window_rectangle[3] == rect[j].height )
          {
            pos = i;
            break;
          }
        }
      }
      if(pos >= 0)
      {
        int undeleted_n = old_regions_n;
        XcolorRegionDelete( xdisplay, w, pos, 1 );
        old_regions = XcolorRegionFetch( xdisplay, w, &old_regions_n );
        if(undeleted_n - old_regions_n != 1)
          printf(  OY_DBG_FORMAT_"removed %d; have still %d\n", OY_DBG_ARGS_,
                   pos, (int)old_regions_n );
      }

      /* upload the new or changed region to the X server */
      error = XcolorRegionInsert( xdisplay, w, 0, &region, 1 );
      if(error)
        printf( OY_DBG_FORMAT_
                 "XcolorRegionInsert failed\n",
                 OY_DBG_ARGS_ );
      netColorTarget = XInternAtom( xdisplay, "_NET_COLOR_TARGET", True );
      if(!netColorTarget)
      {
        printf( OY_DBG_FORMAT_
                 "XInternAtom(..\"_NET_COLOR_TARGET\"..) failed\n",
                 OY_DBG_ARGS_ );
        error = 1;
      }
      if(!error)
      XChangeProperty( xdisplay, w, netColorTarget, XA_STRING, 8,
                       PropModeReplace,
                       (unsigned char*) display_string, strlen(display_string));

      XFlush(xdisplay);

      /* remember the old rectangle */
        gdisp->old_disp_geometry[0] = gdisp->disp_xoffset + offx - offx2;
        gdisp->old_disp_geometry[1] = gdisp->disp_yoffset + offy - offy2;
        gdisp->old_disp_geometry[2] = gdisp->disp_width - dim_corr_x;
        gdisp->old_disp_geometry[3] = gdisp->disp_height - dim_corr_y;
    }
  }
#endif
}

gboolean gdisplay_move_event_handler ( GtkWidget      *canvas,
                              GdkEventMotion *event,
                              gpointer        data )
{
  GDisplay *gdisp;
  GdkEventMotion *mevent;
  gdouble          tx            = 0;
  gdouble          ty            = 0;
  gint             tx_int, ty_int;
  GdkModifierType  tmask;                   /* unused */
  guint            state         = 0;
  gint             return_val    = TRUE;

  gdisp = (GDisplay *) gtk_object_get_user_data (GTK_OBJECT (canvas));

  mevent = (GdkEventMotion *) event;
  state = mevent->state;

  /*  Find out what device the event occurred upon  */
  if (!gdisp_busy && devices_check_change (event))
      gdisplay_check_device_cursor (gdisp);
#if 1
  /*  get the pointer position  */      /* reanimated -hsbo */
  if(gdk_window_is_visible( canvas->window ))
  {
    if (!current_device) {
      gdk_window_get_pointer (canvas->window, &tx_int, &ty_int, &tmask);
    }
    else
    {
      gdk_window_get_pointer (canvas->window, &tx_int, &ty_int, &tmask);
#if GTK_MAJOR_VERSION < 2
      gdk_input_window_get_pointer (canvas->window, current_device, 
        NULL, NULL, NULL, NULL, NULL, NULL);
#endif
    }
  }
  tx = tx_int;
  ty = ty_int;
#endif

  if (no_cursor_updating == 0)
  {
    if (gdisp_busy == BUSY_UP)
    {
      gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
      gdisp_busy = BUSY_ON;
    }
    else if (active_tool && !gimage_is_empty (gdisp->gimage) &&
        !(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    {
      int can_cursor = 0;  /* check whether we are not rejected later */
      GdkEventMotion me;
      Layer *layer;
      double x, y;
 
 
      me.x = tx;  me.y = ty;
      me.state = state;
      gdisplay_untransform_coords_f (gdisp,
                                me.x, me.y,
                                &x, &y,
                                FALSE);
   
      /* TODO move the checks below out of the active_tool->cursor_update_func's
       */
      if ((layer = gimage_get_active_layer (gdisp->gimage)))
      {
        int off_x, off_y;
        drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
        if (x >= off_x && y >= off_y &&
            x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
            y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
          can_cursor = 1;
      }
#     ifndef DISP_DEBUG
      /*if(active_tool->gdisp_ptr == gdisp)*/
#     endif
      if(can_cursor)
      {
        (* active_tool->cursor_update_func) (active_tool, &me, gdisp);
      }
    }
    else if (gdisp_busy == BUSY_DOWN)
    {
      gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
      gdisp_busy = BUSY_OFF;
    }
    else if (gimage_is_empty (gdisp->gimage))
    {
      gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
    }
  }
  return return_val;
}

/**
*   gdisplay_canvas_events()  -  handles the events concerning the 'canvas'
*                                       widget of a GDisplay
*   `gdisp != NULL' warranted?
*/
gint
gdisplay_canvas_events (GtkWidget *canvas,
                        GdkEvent  *event)
{
  GDisplay *gdisp;
  GdkEventExpose *eevent;
  GdkEventMotion *mevent;
  GdkEventButton *bevent;
  GdkEventKey *kevent;
  gdouble          tx            = 0;
  gdouble          ty            = 0;
  gint             tx_int, ty_int;
  GdkModifierType  tmask;                   /* unused */
  guint            state         = 0;
  gint             return_val    = FALSE;
  static gboolean  scrolled      = FALSE;
  static guint     key_signal_id = 0;

  gdisp = (GDisplay *) gtk_object_get_user_data (GTK_OBJECT (canvas));

#if DEBUG_DISPLAY
  printf("%4d: %s( \"%s\", dispID=%d )... [have window=%d]\n", __LINE__,
         __func__, cp_event_type_name(event), gdisplay_to_ID(gdisp), 
         canvas->window ? 1 : 0);
#endif 

  if (!canvas->window)
    {
    return FALSE;
    }
  /*  If this is the first event...  */
  if (!gdisp->select)
  {
    /*  create the selection object  */
    gdisp->select = selection_create (gdisp->canvas->window, gdisp,
        gdisp->gimage->height,
        gdisp->gimage->width, marching_speed);

    gdisp->disp_width = gdisp->canvas->allocation.width;
    gdisp->disp_height = gdisp->canvas->allocation.height;

    /*  create GC for scrolling */
    gdisp->scroll_gc = gdk_gc_new (gdisp->canvas->window);
    gdk_gc_set_exposures (gdisp->scroll_gc, TRUE);

    /*  set up the scrollbar observers  */
    gtk_signal_connect (GTK_OBJECT (gdisp->hsbdata), "value_changed",
        (GtkSignalFunc) scrollbar_horz_update,
        gdisp);
    gtk_signal_connect (GTK_OBJECT (gdisp->vsbdata), "value_changed",
        (GtkSignalFunc) scrollbar_vert_update,
        gdisp);

    /*  setup scale properly  */
    setup_scale (gdisp);

    /* set the zoom control's focus to this display since it is new */
    zoom_set_focus(gdisp);
    gdisplay_set_colour_region(gdisp);
  }

  /*  Find out what device the event occurred upon  */
  if (!gdisp_busy && devices_check_change (event))
      gdisplay_check_device_cursor (gdisp);

#ifdef DEBUG_
  print_gdk_event_type(event);
#endif
  switch (event->type)
  {
    case GDK_EXPOSE:
      eevent = (GdkEventExpose *) event;
      if (active_tool->type == CLONE)
	clone_expose (); 
#     ifdef DEBUG_
      printf("%s:%d %d+%d,%dx%d\n",__FILE__,__LINE__,eevent->area.x,
          eevent->area.y, eevent->area.width, eevent->area.height);
#     endif
      redraw (gdisp, eevent->area.x, eevent->area.y,
          eevent->area.width, eevent->area.height);
      if( gdisp->old_disp_geometry[0] == 0 &&
          gdisp->old_disp_geometry[1] == 0 &&
          gdisp->old_disp_geometry[2] == 0 &&
          gdisp->old_disp_geometry[3] == 0 )
        gdisplay_set_colour_region(gdisp);
      return_val = TRUE;
      break;

    case GDK_CONFIGURE:
      if ((gdisp->disp_width != gdisp->canvas->allocation.width) ||
          (gdisp->disp_height != gdisp->canvas->allocation.height))
      {
        gdisp->disp_width = gdisp->canvas->allocation.width;
        gdisp->disp_height = gdisp->canvas->allocation.height;
        resize_display (gdisp, 0, FALSE);
      }
      gdisplay_set_colour_region(gdisp);
      return_val = TRUE;
      break;

    case GDK_LEAVE_NOTIFY:
      if (((GdkEventCrossing*) event)->mode != GDK_CROSSING_NORMAL)
	return TRUE;
      if (active_tool->type == CLONE)
	clone_leave_notify ();
#if 0
      gdisplay_update_cursor (gdisp, 0, 0);
      gtk_label_set_text (GTK_LABEL (gdisp->cursor_label), "");
      info_window_update_RGB (gdisp, -1, -1);
#endif

    case GDK_PROXIMITY_OUT:
      gdisp->proximity = FALSE;
      gdisplay_update_cursor (gdisp, 0, 0);
      break;

    case GDK_ENTER_NOTIFY:
      if (((GdkEventCrossing*) event)->mode != GDK_CROSSING_NORMAL)
	return TRUE;
      if (active_tool->type == CLONE)
	clone_enter_notify ();
      lc_dialog_update (gdisp->gimage); /* laid-back if gimage==dialog_img */
      break;

    case GDK_FOCUS_CHANGE:              /* hsbo */
      break;
      
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      state = bevent->state;
      middle_mouse_button = 0;
      zoom_set_focus(gdisp);

      switch (bevent->button)
      {
        case 1:

          gtk_grab_add (canvas);

          /* This is a hack to prevent other stuff being run in the middle of
             a tool operation (like changing image types.... brrrr). We just
             block all the keypress event. A better solution is to implement
             some sort of locking for images.
             Note that this is dependent on specific GTK behavior, and isn't
             guaranteed to work in future versions of GTK.
             -Yosh
           */
          if (key_signal_id == 0)
            key_signal_id = gtk_signal_connect (GTK_OBJECT (canvas),
                "key_press_event",
                GTK_SIGNAL_FUNC (gtk_true),
                NULL);

          if (active_tool && ((active_tool->type == MOVE) ||
                !gimage_is_empty (gdisp->gimage)))
          {
            if (0&&active_tool->auto_snap_to)
            {

              /* IMAGEWORKS hack to force tablet bevent->x, bevent-y to gdk_window_get_pointer values Allen R.*/
              if ( CAST(int)current_device == 2 ) {
                bevent->x = tx;
                bevent->y = ty;
              }
              /* DONE hack Allen R.*/

              gdisplay_snap_point (gdisp, bevent->x, bevent->y, &tx, &ty);
	      
              bevent->x = tx;
              bevent->y = ty;
            }

            /* reset the current tool if we're changing gdisplays */
            /*
               if (active_tool->gdisp_ptr) {
               tool_gdisp = active_tool->gdisp_ptr;
               if (tool_gdisp->ID != gdisp->ID) {
               tools_initialize (active_tool->type, gdisp);
               active_tool->drawable = gimage_active_drawable(gdisp->gimage);
               }
               } else
             */
            /* reset the current tool if we're changing drawables */
            if (active_tool->drawable) {
              if (((gimage_active_drawable(gdisp->gimage)) !=
                    active_tool->drawable) &&
                  !active_tool->preserve)
                tools_initialize (active_tool->type, gdisp);
            } else
              active_tool->drawable = gimage_active_drawable(gdisp->gimage);

            /* hack hack hack */
            if (no_cursor_updating == 0)
            {
              switch (active_tool->type)
              {
                case BEZIER_SELECT:
                  gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                  break;

                case FUZZY_SELECT:                          
                  gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                  gdisp_busy = BUSY_UP;
                  break;

                default:
                  break;
              }
            }

            (* active_tool->button_press_func) (active_tool, bevent, gdisp);
          }
          break;

        case 2:
          middle_mouse_button = 1;
	  state |= GDK_BUTTON2_MASK;
	  scrolled = TRUE;
          gtk_grab_add (canvas);
          start_grab_and_scroll (gdisp, bevent);
          break;

        case 3:
          popup_shell = gdisp->shell;
          gdisplay_set_menu_sensitivity (gdisp);
          gtk_menu_popup (GTK_MENU (gdisp->popup), NULL, NULL, NULL, NULL, 3, bevent->time);
          break;

        default:
          break;
      }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      state = bevent->state;
      middle_mouse_button = 0;

      switch (bevent->button)
      {
        case 1:
          /* Lame hack. See above */
          if (key_signal_id)
          {
            gtk_signal_disconnect (GTK_OBJECT (canvas), key_signal_id);
            key_signal_id = 0;
          }

          gtk_grab_remove (canvas);
          gdk_pointer_ungrab (bevent->time);  /* fixes pointer grab bug */
          if (active_tool && ((active_tool->type == MOVE) ||
                !gimage_is_empty (gdisp->gimage)))
            if (active_tool->state == ACTIVE)
            {
    	      if (0&&active_tool->auto_snap_to)
              {
                gdisplay_snap_point (gdisp, bevent->x, bevent->y, &tx, &ty);
                bevent->x = tx;
                bevent->y = ty;
              }

              
              /* hack hack hack */
              if (no_cursor_updating == 0)
              {
                switch (active_tool->type)
                {
                  case ROTATE:
                  case SCALE:
                  case SHEAR:
                  case PERSPECTIVE:

                  case RECT_SELECT:
                  case ELLIPSE_SELECT:
                  case FREE_SELECT:
                  case BEZIER_SELECT:
                  case FUZZY_SELECT:

                  case FLIP_HORZ:
                  case FLIP_VERT:
                  case BUCKET_FILL:
                  case BLEND:

                    gdisplay_install_tool_cursor (gdisp, GDK_WATCH);
                    gdisp_busy = BUSY_DOWN;
                    break;

                  default:
                    break;
                }
              }

              (* active_tool->button_release_func) (active_tool, bevent, gdisp);
            }
          break;

        case 2:
          middle_mouse_button = 1;
	  state &= ~GDK_BUTTON2_MASK;
          scrolled = FALSE;
          gtk_grab_remove (canvas);
	  break;

        case 3:
          break;

        default:
          break;
      }
      break;

    case GDK_MOTION_NOTIFY:{
      GdkModifierType state;

      middle_mouse_button = 0;

      mevent = (GdkEventMotion *) event;
      state = mevent->state;

     /* Ask for the pointer position, but ignore it except for cursor
      * handling, so motion events sync with the button press/release events */

      if (mevent->is_hint)
	{
#if GTK_MAJOR_VERSION < 2
	  gdk_input_window_get_pointer (canvas->window, current_device, &tx, &ty,
#ifdef GTK_HAVE_SIX_VALUATORS
					NULL, NULL, NULL, NULL, NULL
#else /* !GTK_HAVE_SIX_VALUATORS */	
					NULL, NULL, NULL, NULL
#endif /* GTK_HAVE_SIX_VALUATORS */
					);
#endif
	  mevent->x = tx;
	  mevent->y = ty;
	}
      else
	{
	  tx = mevent->x;
	  ty = mevent->y;
	}

      if (!gdisp->proximity)
	{
	  gdisp->proximity = TRUE;
	  gdisplay_check_device_cursor (gdisp);
	}
      
      if (gdisp->window_info_dialog) 
      {
        int x,y;
        gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, 0);
        info_window_update_xy (gdisp->window_info_dialog, (void *)gdisp, x, y);
      }
      if (active_tool && ((active_tool->type == MOVE) ||
            !gimage_is_empty (gdisp->gimage)) &&
          (mevent->state & GDK_BUTTON1_MASK))
      {
        if (active_tool->state == ACTIVE)
        {
          /*  if the first mouse button is down, check for automatic
           *  scrolling...
           */
          if ((mevent->state & GDK_BUTTON1_MASK) && !active_tool->scroll_lock)
          {
            if (mevent->x < 0 || mevent->y < 0 ||
                mevent->x > gdisp->disp_width ||
                mevent->y > gdisp->disp_height)
              scroll_to_pointer_position (gdisp, mevent);
          }

          if (0&&active_tool->auto_snap_to)
          {
            gdisplay_snap_point (gdisp, mevent->x, mevent->y, &tx, &ty);
            mevent->x = tx;
            mevent->y = ty;
          }

#         ifndef DISP_DEBUG
          if(active_tool->gdisp_ptr == gdisp)
#         endif
            (* active_tool->motion_func) (active_tool, mevent, gdisp);
        }
      }
      else if ((mevent->state & GDK_BUTTON2_MASK) && scrolled)
      {
        middle_mouse_button = 1;
        grab_and_scroll (gdisp, mevent);
      }
      }
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;
      state = kevent->state;
      zoom_set_focus(gdisp);

      switch (kevent->keyval)
      {
        case GDK_Left: case GDK_Right:
        case GDK_Up: case GDK_Down:
          if (active_tool && !gimage_is_empty (gdisp->gimage)
#         ifndef DISP_DEBUG
              && active_tool->gdisp_ptr == gdisp
#         endif
             )
            (* active_tool->arrow_keys_func) (active_tool, kevent, gdisp);
          return_val = TRUE;
          break;

        case GDK_Tab:
          if (kevent->state & GDK_MOD1_MASK && !gimage_is_empty (gdisp->gimage))
            layer_select_init (gdisp->gimage, 1, kevent->time);
          if (kevent->state & GDK_CONTROL_MASK && !gimage_is_empty (gdisp->gimage))
            layer_select_init (gdisp->gimage, -1, kevent->time);
          return_val = TRUE;
          break;

        /*  Update the state based on modifiers being pressed  */
        case GDK_Alt_L: 
        case GDK_Alt_R:
          state |= GDK_MOD1_MASK;
          break;
        case GDK_Shift_L: 
        case GDK_Shift_R:
          state |= GDK_SHIFT_MASK;
          break;
        case GDK_Control_L: 
        case GDK_Control_R:
          state |= GDK_CONTROL_MASK;
          break;
      }

      /*  We need this here in case of accelerators  */
      gdisplay_set_menu_sensitivity (gdisp);
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;
      state = kevent->state;

      switch (kevent->keyval)
      {
        case GDK_Alt_L: 
        case GDK_Alt_R:
           state &= ~GDK_MOD1_MASK;
           break;
        case GDK_Shift_L: 
        case GDK_Shift_R:
           kevent->state &= ~GDK_SHIFT_MASK;
           break;
        case GDK_Control_L: 
        case GDK_Control_R:
           kevent->state &= ~GDK_CONTROL_MASK;
           break;
      }

      return_val = TRUE;
      break;

    default:
      break;
  }          /*  switch(event->type)  */

  return return_val;
}

gint
gdisplay_hruler_button_press (GtkWidget      *widget,
                              GdkEventButton *event,
                              gpointer        data)
{
  GDisplay *gdisp;

  if (event->button == 1)
  {
    gdisp = data;

    gtk_widget_activate (tool_widgets[tool_info[(int) MOVE].toolbar_position]);
    move_tool_start_hguide (active_tool, gdisp);
    gtk_grab_add (gdisp->canvas);
  }

  return FALSE;
}

gint
gdisplay_vruler_button_press (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
  GDisplay *gdisp;

  if (event->button == 1)
    {
      gdisp = data;

      gtk_widget_activate (tool_widgets[tool_info[(int) MOVE].toolbar_position]);
      move_tool_start_vguide (active_tool, gdisp);
      gtk_grab_add (gdisp->canvas);
    }

#if 0
  gdisplay_update_cursor (gdisp, tx, ty);
#endif

 return FALSE;
}



