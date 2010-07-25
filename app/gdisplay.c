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
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "buildmenu.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "disp_callbacks.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gdisplayP.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_mask.h"
#include "rc.h"
#include "gximage.h"
#include "image_render.h"
#include "info_window.h"
#include "interface.h"
#include "layer.h"
#include "layers_dialog.h"
#include "menus.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "plug_in.h"
#include "scale.h"
#include "scroll.h"
#include "tilebuf.h"
#include "tools.h"
#include "undo.h"
#include "base_frame_manager.h"

#include "layer_pvt.h"			/* ick. */

#define OVERHEAD          25  /*  in units of pixel area  */
#define EPSILON           5

/* variable declarations */
GSList *               display_list = NULL;
static int             display_num  = 1;
static GdkCursorType   default_gdisplay_cursor = GDK_TOP_LEFT_ARROW;
extern char 	       BRUSH_CHANGED;
CMSTransform *         transform_buffer=NULL;   /* the transform used for the current display flush */

#define ROUND(x) ((int) (x + 0.5))

#define MAX_TITLE_BUF 4096


/*  Local functions  */
static void       gdisplay_format_title     (GImage *, char *, GDisplay*);
static void       gdisplay_delete           (GDisplay *);
static GSList *   gdisplay_free_area_list   (GSList *);
static GSList *   gdisplay_process_area_list(GSList *, GArea *);
/*static void       gdisplay_add_update_area  (GDisplay *, int, int, int, int);
*/static void       gdisplay_add_display_area (GDisplay *, int, int, int, int);
static void       gdisplay_paint_area       (GDisplay *, int, int, int, int);
static void	  gdisplay_draw_cursor	    (GDisplay *);
static void       gdisplay_display_area     (GDisplay *, int, int, int, int);
static guint      gdisplay_hash             (GDisplay *);
/* updates the projection */
/*static void       gdisplay_project          (GDisplay *, int, int, int, int);*/

static GHashTable *display_ht = NULL;

GDisplay*
gdisplay_new (GImage       *gimage,
	      unsigned int  scale)
{
  GDisplay *gdisp;
  GtkItemFactoryItem * fac_item;
  GSList *cur_slist;
  GSList *cur_wid_list;
  char title [MAX_TITLE_BUF];
  int instance;

  /*  If there isn't an interface, never create a gdisplay  */
  if (no_interface)
    return NULL;


  instance = gimage->instance_count;
  gimage->instance_count++;
  gimage->ref_count++;

  /*
   *  Set all GDisplay parameters...
   */
  gdisp = (GDisplay *) g_malloc_zero (sizeof (GDisplay));

  gdisp->offset_x = gdisp->offset_y = 0;
  gdisp->scale = scale;
  gdisp->gimage = gimage;
  gdisp->window_info_dialog = NULL;
  gdisp->depth = g_visual->depth;
  gdisp->select = NULL;
  gdisp->ID = gimage->ID;
  gdisp->unique_id = display_num++;
  gdisp->instance = instance;
  gdisp->update_areas = NULL;
  gdisp->display_areas = NULL;
  gdisp->disp_xoffset = 0;
  gdisp->disp_yoffset = 0;
  gdisp->current_cursor = -1;
  gdisp->draw_guides = TRUE;
  gdisp->snap_to_guides = TRUE;
  gdisp->menubar = NULL;
  gdisp->menubar_fac = NULL;

  gdisp->framemanager = 0;
  gdisp->frame_manager = NULL;
  gdisp->bfm = NULL;
  gdisp->slide_show = 0;
  gdisp->look_profile_manager = NULL;

  /* format the title */
  gdisplay_format_title (gimage, title, gdisp);

  gdisp->draw_cursor = FALSE;
  gdisp->proximity = FALSE;
  gdisp->have_cursor = FALSE;

  gdisp->expose = EXPOSE_DEFAULT;
  gdisp->offset = OFFSET_DEFAULT;
  gdisp->gamma = GAMMA_DEFAULT;
  gdisp->cms_intent = cms_default_intent;
  gdisp->cms_flags = cms_default_flags;
  gdisp->look_profile_pipe = NULL;
  gdisp->cms_expensive_transf = calloc( sizeof(CMSTransform*), 1);
  *gdisp->cms_expensive_transf = NULL;

  /*  add the new display to the list so that it isn't lost  */
  display_list = g_slist_append (display_list, (void *) gdisp);

  /*  create the shell for the image  */
  create_display_shell (gdisp->unique_id, gimage->width, gimage->height,
			title, 0);

  /*  set the user data  */
  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) gdisplay_hash, NULL);

  /* Add the coorespondences for event qidget lookup */
  g_hash_table_insert (display_ht, gdisp->shell, gdisp);
  g_hash_table_insert (display_ht, gdisp->canvas, gdisp);
  g_hash_table_insert (display_ht, gdisp->menubar, gdisp);

  /* Add all of the top-level menus to the hash table as well,
     so they can be resolved on event widget lookup. */

  cur_slist = gdisp->menubar_fac->items;
  while (cur_slist != NULL) {
    fac_item = (GtkItemFactoryItem *) cur_slist->data; 
    cur_wid_list = fac_item->widgets;
    //printf("%s:%d %s() Adding factory component: %s %s |\n", __FILE__,__LINE__,__func__,
     //     fac_item->path);

    while (cur_wid_list != NULL) { 

      if( strcmp("GtkTearoffMenuItem",
#if GTK_MAJOR_VERSION > 1
          GTK_OBJECT_TYPE_NAME(cur_wid_list->data)
#else
          gtk_type_query(GTK_OBJECT_TYPE(cur_wid_list->data))
#endif
          ) != 0)
      {
#       ifdef DEBUG_
        printf("%s:%d %s() Adding factory component: %s %s |\n", __FILE__,__LINE__,__func__,
#if GTK_MAJOR_VERSION > 1
          GTK_OBJECT_TYPE_NAME(cur_wid_list->data),
#else
          gtk_type_query(GTK_OBJECT_TYPE(cur_wid_list->data))->type_name,
#endif
          ((GtkItemFactoryItem *) cur_wid_list->data)->path);
#       endif
        g_hash_table_insert(display_ht, cur_wid_list->data, gdisp); 
      }
      cur_wid_list = cur_wid_list->next;
    }
    cur_slist = cur_slist->next;
  }

  /*  set the current tool cursor  */
  gdisplay_install_tool_cursor (gdisp, default_gdisplay_cursor);

  gdisplays_update_title (gimage->ID); 

  if(cms_bpc_by_default)
    gdisp->cms_flags = gdisp->cms_flags | cmsFLAGS_WHITEBLACKCOMPENSATION;
  if(cms_manage_by_default)
    gdisplay_set_bpc(gdisp, cms_bpc_by_default);
  gdisp->cms_proof_flags = cms_default_proof_intent;

  if (gdisplay_set_colormanaged(gdisp, cms_manage_by_default) == FALSE)	 
  {   g_message(_(_("Cannot colormanage this display.\nEither image or display profile are undefined.\n. To avoid this message switch off automatic colormanagement in File/Preferences/Color Management")));
  }

  return gdisp;
}

GDisplay*
gdisplay_fm (GImage *gimage,
	      unsigned int  scale,
	      GDisplay *display)
{

 if (display->bfm)
   return bfm_load_image_into_fm (display, gimage);

 return display;
}


static void
gdisplay_format_title (GImage *gimage,
		       char   *title,
		       GDisplay *gdisp)
{
  int empty = gimage_is_empty (gimage);
  Tag t = gimage_tag (gimage);
  const char *colour = 0;
  char *alpha_msg = ""; /*"no alpha";*/
  const char *profile = NULL;
  char *proof = NULL;

  if (layer_has_alpha(gimage->active_layer)){
    alpha_msg = _("A"); /*"alpha";*/
  }

  if (gdisplay_is_colormanaged( gdisp ))
  {
    if (gimage_get_cms_profile(gimage) != NULL)
    { profile = cms_get_profile_info(gimage_get_cms_profile(gimage))->description;
      if (gdisplay_is_proofed (gdisp))
        proof = g_strdup(cms_get_profile_info(gimage_get_cms_proof_profile(gimage))->description);
      colour = cms_get_profile_info(gimage->cms_profile)->color_space_name;
      if(strstr(colour,"Cmyk"))
          alpha_msg = "";
    }
    else 
    { profile = _("[no icc profile assigned]");
    }
  }


  sprintf (title, "%s-%d.%d (%s - %s%s - %s%s%s)%s %d%%",
	   prune_filename (gimage_filename (gimage)),
	   gimage->ID, gimage->instance_count,
	   tag_string_precision (tag_precision (t)),
	   colour ? colour : tag_string_format (tag_format (t)),
	   alpha_msg,
	   profile ? profile : "",
           proof ? _(" | proof: ") : "",
           proof ? proof : "",
           empty ? _(" (empty)") : "",
	   100 * SCALEDEST (gdisp) / SCALESRC (gdisp));
  if (proof) g_free (proof);

  if (gdisp->current_cursor != -1 )
  change_win_cursor (gdisp->canvas->window, gdisp->current_cursor); 
}


static void
gdisplay_delete (GDisplay *gdisp)
{
  GDisplay *tool_gdisp;
  GSList *cur_slist;
  GSList *cur_wid_list;
  GtkItemFactoryItem * fac_item;
  int smr=1;

  g_hash_table_remove (display_ht, gdisp->shell);
  g_hash_table_remove (display_ht, gdisp->canvas);
  g_hash_table_remove (display_ht, gdisp->menubar);

  cur_slist = gdisp->menubar_fac->items;
  while (cur_slist != NULL) {
    fac_item = (GtkItemFactoryItem *) cur_slist->data; 
    cur_wid_list = fac_item->widgets;

    while (cur_wid_list != NULL)
    {
      char path[64] = "", *ptr=0;
      const char* type = 0;

      type = 
#if GTK_MAJOR_VERSION > 1
          GTK_OBJECT_TYPE_NAME(cur_wid_list->data);
#else
          gtk_type_query(GTK_OBJECT_TYPE(cur_wid_list->data))->type_name; //, fac_item->path;
#endif
#     ifdef DEBUG_
      printf("%s:%d %s() Removing factory component: %s\n",
      __FILE__,__LINE__,__func__, type?type:"");
#     endif
      g_hash_table_remove(display_ht, cur_wid_list->data);

      
      if(smr && (strstr(type,"MenuItem") || strstr(type,"GtkMenuItem")))
      {
        snprintf( path, 64, fac_item->path );
        ptr = strchr( fac_item->path, '/' );
        if(ptr) *ptr=0;
#     ifdef DEBUG
        printf("%s:%d %s() Removing %s\n",__FILE__,__LINE__,__func__, path);
#     endif
       /*menus_destroy (path); */
        smr = 0;
      }

      cur_wid_list = cur_wid_list->next;
    }
    cur_slist = cur_slist->next;
  }
# ifdef DEBUG
  printf("%s:%d %s() remaining items in display_ht: %d\n",__FILE__,__LINE__,__func__, 
      g_hash_table_size( display_ht ));
# endif

  /*  stop any active tool  */
  active_tool_control (HALT, (void *) gdisp);

  /*  clear out the pointer to this gdisp from the active tool */

  if (active_tool && active_tool->gdisp_ptr) {
    tool_gdisp = active_tool->gdisp_ptr;
    if (gdisp->ID == tool_gdisp->ID) {
      active_tool->drawable = NULL;
      active_tool->gdisp_ptr = NULL;
    }
  }

  /*  free the selection structure  */
  selection_free (gdisp->select);

  /* free the profile pipe */
  g_slist_free(gdisp->look_profile_pipe); 

  /* free the stored transform */
  if(*gdisp->cms_expensive_transf)
    cms_return_transform ( *gdisp->cms_expensive_transf );
  free (gdisp->cms_expensive_transf);
  gdisp->cms_expensive_transf = NULL;

  if (gdisp->scroll_gc)
    gdk_gc_destroy (gdisp->scroll_gc);

  /*  free the area lists  */
  gdisplay_free_area_list (gdisp->update_areas);
  gdisplay_free_area_list (gdisp->display_areas);
  gdisp->display_areas = NULL;

  /*  free the gimage  */
  gimage_delete (gdisp->gimage);
  gdisp->gimage = NULL;

  /*  insure that if a window information dialog exists, it is removed  */
  if (gdisp->window_info_dialog)
    info_window_free (gdisp->window_info_dialog);
  gdisp->window_info_dialog = NULL;

  /*  set popup_shell to NULL if appropriate */
  if (popup_shell == gdisp->shell)
    popup_shell= NULL;

  gtk_widget_unref (gdisp->shell);

# ifdef DEBUG
  printf("gdisp's in display_list: %d \n",g_slist_length( display_list ));
# endif

  g_free (gdisp);

}


static GSList *
gdisplay_free_area_list (GSList *list)
{
  GSList *l = list;
  GArea *ga;

  while (l)
    {
      /*  free the data  */
      ga = (GArea *) l->data;
      g_free (ga);

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return NULL;
}


static GSList *
gdisplay_process_area_list (GSList *list,
                            GArea  *ga1)
{
  GSList *new_list;
  GSList *l = list;
  int area1, area2, area3;
  GArea *ga2;

  /*  start new list off  */
  new_list = g_slist_prepend (NULL, ga1);
  while (l)
    {
      /*  process the data  */
      ga2 = (GArea *) l->data;
      area1 = (ga1->x2 - ga1->x1) * (ga1->y2 - ga1->y1) + OVERHEAD;
      area2 = (ga2->x2 - ga2->x1) * (ga2->y2 - ga2->y1) + OVERHEAD;
      area3 = (MAXIMUM (ga2->x2, ga1->x2) - MINIMUM (ga2->x1, ga1->x1)) *
        (MAXIMUM (ga2->y2, ga1->y2) - MINIMUM (ga2->y1, ga1->y1)) + OVERHEAD;

      if ((area1 + area2) < area3)
        new_list = g_slist_prepend (new_list, ga2);
      else
        {
          ga1->x1 = MINIMUM (ga1->x1, ga2->x1);
          ga1->y1 = MINIMUM (ga1->y1, ga2->y1);
          ga1->x2 = MAXIMUM (ga1->x2, ga2->x2);
          ga1->y2 = MAXIMUM (ga1->y2, ga2->y2);

          g_free (ga2);
        }

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return new_list;
}


void
gdisplay_flush (GDisplay *gdisp)
{
  GArea  *ga;
  GSList *list;
  static int flushing = FALSE;

  /*  Flush the items in the displays and updates lists--
   *  but only if gdisplay has been mapped and exposed
   */
  if (!gdisp->select)
    return;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    return;

  flushing = TRUE;

  /*  First the updates...  */
  list = gdisp->update_areas;

  while (list)
    {
      /*  Paint the area specified by the GArea  */
      ga = (GArea *) list->data;
      gdisplay_paint_area (gdisp, ga->x1, ga->y1,
                           (ga->x2 - ga->x1), (ga->y2 - ga->y1));

      list = g_slist_next (list);
    }
  /*  Free the update lists  */
  gdisp->update_areas = gdisplay_free_area_list (gdisp->update_areas);

  /* delete the transform */
  if (transform_buffer != NULL)
  {   cms_return_transform(transform_buffer);
      transform_buffer = NULL;
  }


  /*  Next the displays...  */
  list = gdisp->display_areas;

  if (list)
    {
      /*  stop the currently active tool  */
      active_tool_control (PAUSE, (void *) gdisp);

      while (list)
        {
          /*  Paint the area specified by the GArea  */
          ga = (GArea *) list->data;
          gdisplay_display_area (gdisp, ga->x1, ga->y1,
                                 (ga->x2 - ga->x1), (ga->y2 - ga->y1));

          list = g_slist_next (list);
        }
      /*  Free the update lists  */
      gdisp->display_areas = gdisplay_free_area_list (gdisp->display_areas);

      /* draw the guides */
      gdisplay_draw_guides (gdisp);

      /* and the cursor (if we have a software cursor */
      if (gdisp->have_cursor)
        gdisplay_draw_cursor (gdisp);

      /* restart (and recalculate) the selection boundaries */
      selection_start (gdisp->select, TRUE);

      /* start the currently active tool */
      active_tool_control (RESUME, (void *) gdisp);
    }

  /*  update the gdisplay's info dialog  */
  if (gdisp->window_info_dialog)
    info_window_update (gdisp->window_info_dialog,
                        (void *) gdisp);
  flushing = FALSE;
}


void
gdisplay_draw_guides (GDisplay *gdisp)
{
  GList *tmp_list;
  Guide *guide;

  if (gdisp->draw_guides)
    {
      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
        {
          guide = tmp_list->data;
          tmp_list = tmp_list->next;

          gdisplay_draw_guide (gdisp, guide, FALSE);
        }
    }
}

void
gdisplay_draw_guide (GDisplay *gdisp,
                     Guide    *guide,
                     int       active)
{
  static GdkGC *normal_hgc = NULL;
  static GdkGC *active_hgc = NULL;
  static GdkGC *normal_vgc = NULL;
  static GdkGC *active_vgc = NULL;
  static int initialize = TRUE;
  int x1, x2;
  int y1, y2;
  int w, h;
  int x, y;

  if (guide->position < 0)
    return;

  if (initialize)
    {
      GdkGCValues values;
      guchar stipple[8] =
      {
        0xF0,    /*  ####----  */
        0xE1,    /*  ###----#  */
        0xC3,    /*  ##----##  */
        0x87,    /*  #----###  */
        0x0F,    /*  ----####  */
        0x1E,    /*  ---####-  */
        0x3C,    /*  --####--  */
        0x78,    /*  -####---  */
      };

      initialize = FALSE;

      values.foreground.pixel = gdisplay_black_pixel (gdisp);
      values.background.pixel = g_normal_guide_pixel;
      values.fill = GDK_OPAQUE_STIPPLED;
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window, (char*) stipple, 8, 1);
      normal_hgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
                                          GDK_GC_FOREGROUND |
                                          GDK_GC_BACKGROUND |
                                          GDK_GC_FILL |
                                          GDK_GC_STIPPLE);

      values.background.pixel = g_active_guide_pixel;
      active_hgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
                                           GDK_GC_FOREGROUND |
                                           GDK_GC_BACKGROUND |
                                           GDK_GC_FILL |
                                           GDK_GC_STIPPLE);

      values.foreground.pixel = gdisplay_black_pixel (gdisp);
      values.background.pixel = g_normal_guide_pixel;
      values.fill = GDK_OPAQUE_STIPPLED;
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window, (char*) stipple, 1, 8);
      normal_vgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
                                          GDK_GC_FOREGROUND |
                                          GDK_GC_BACKGROUND |
                                          GDK_GC_FILL |
                                          GDK_GC_STIPPLE);

      values.background.pixel = g_active_guide_pixel;
      active_vgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
                                           GDK_GC_FOREGROUND |
                                           GDK_GC_BACKGROUND |
                                           GDK_GC_FILL |
                                           GDK_GC_STIPPLE);
    }

  gdisplay_transform_coords (gdisp, 0, 0, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, gdisp->gimage->width, gdisp->gimage->height, &x2, &y2, FALSE);
  gdk_window_get_size (gdisp->canvas->window, &w, &h);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > w) x2 = w;
  if (y2 > h) y2 = h;

  if (guide->orientation == HORIZONTAL_GUIDE)
    {
      gdisplay_transform_coords (gdisp, 0, guide->position, &x, &y, FALSE);

      if (active)
        gdk_draw_line (gdisp->canvas->window, active_hgc, x1, y, x2, y);
      else
        gdk_draw_line (gdisp->canvas->window, normal_hgc, x1, y, x2, y);
    }
  else if (guide->orientation == VERTICAL_GUIDE)
    {
      gdisplay_transform_coords (gdisp, guide->position, 0, &x, &y, FALSE);

      if (active)
        gdk_draw_line (gdisp->canvas->window, active_vgc, x, y1, x, y2);
      else
        gdk_draw_line (gdisp->canvas->window, normal_vgc, x, y1, x, y2);
    }
}

Guide*
gdisplay_find_guide (GDisplay *gdisp,
                     int       x,
                     int       y)
{
  GList *tmp_list;
  Guide *guide;
  double scale;
  int offset_x, offset_y;
  int pos;

  if (gdisp->draw_guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) : 1.0 / SCALESRC (gdisp);

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
        {
          guide = tmp_list->data;
          tmp_list = tmp_list->next;

          switch (guide->orientation)
            {
            case HORIZONTAL_GUIDE:
              pos = (int) (scale * guide->position - offset_y);
              if ((guide->position != -1) &&
                  (pos > (y - EPSILON)) &&
                  (pos < (y + EPSILON)))
                return guide;
              break;
            case VERTICAL_GUIDE:
              pos = (int) (scale * guide->position - offset_x);
              if ((guide->position != -1) &&
                  (pos > (x - EPSILON)) &&
                  (pos < (x + EPSILON)))
                return guide;
              break;
            }
        }
    }

  return NULL;
}

void
gdisplay_snap_point (GDisplay *gdisp,
                     gdouble   x ,
                     gdouble   y,
                     gdouble   *tx,
                     gdouble   *ty)
{
  GList *tmp_list;
  Guide *guide;
  double scale;
  int offset_x, offset_y;
  int minhdist, minvdist;
  int pos, dist;

  *tx = x;
  *ty = y;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) : 1.0 / SCALESRC (gdisp);

      minhdist = G_MAXINT;
      minvdist = G_MAXINT;

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
        {
          guide = tmp_list->data;
          tmp_list = tmp_list->next;

          switch (guide->orientation)
            {
            case HORIZONTAL_GUIDE:
              pos = (int) (scale * guide->position - offset_y);
              if ((pos > (y - EPSILON)) &&
                  (pos < (y + EPSILON)))
                {
                  dist = pos - y;
                  dist = ABS (dist);

                  if (dist < minhdist)
                    {
                      minhdist = dist;
                      *ty = pos;
                    }
                }
              break;
            case VERTICAL_GUIDE:
              pos = (int) (scale * guide->position - offset_x);
              if ((pos > (x - EPSILON)) &&
                  (pos < (x + EPSILON)))
                {
                  dist = pos - x;
                  dist = ABS (dist);

                  if (dist < minvdist)
                    {
                      minvdist = dist;
                      *tx = pos;
                    }
                }
              break;
            }
        }
    }
}

void
gdisplay_snap_rectangle (GDisplay *gdisp,
                         int       x1,
                         int       y1,
                         int       x2,
                         int       y2,
                         int      *tx1,
                         int      *ty1)
{
  double nx1, ny1;
  double nx2, ny2;

  *tx1 = x1;
  *ty1 = y1;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      gdisplay_snap_point (gdisp, x1, y1, &nx1, &ny1);
      gdisplay_snap_point (gdisp, x2, y2, &nx2, &ny2);

      if (x1 != (int)nx1)
        *tx1 = nx1;
      else if (x2 != (int)nx2)
        *tx1 = x1 + (nx2 - x2);

      if (y1 != (int)ny1)
        *ty1 = ny1;
      else if (y2 != (int)ny2)
        *ty1 = y1 + (ny2 - y2);
    }
}

void
gdisplay_draw_cursor (GDisplay *gdisp)
{
  int x = gdisp->cursor_x;
  int y = gdisp->cursor_y;
  
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->white_gc,
                 x - 7, y-1, x + 7, y-1);
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->black_gc,
                 x - 7, y, x + 7, y);
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->white_gc,
                 x - 7, y+1, x + 7, y+1);
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->white_gc,
                 x-1, y - 7, x-1, y + 7);
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->black_gc,
                 x, y - 7, x, y + 7);
  gdk_draw_line (gdisp->canvas->window,
                 gdisp->canvas->style->white_gc,
                 x+1, y - 7, x+1, y + 7);
}

void
gdisplay_update_cursor (GDisplay *gdisp, int x, int y)
{
  int new_cursor;

  new_cursor = gdisp->draw_cursor && gdisp->proximity;
  
  /* Erase old cursor, if necessary */

  if (gdisp->have_cursor && (!new_cursor || x != gdisp->cursor_x ||
                             y != gdisp->cursor_y))
    {
      gdisplay_expose_area (gdisp, gdisp->cursor_x - 7,
                            gdisp->cursor_y - 7,
                            15, 15);
      if (!new_cursor)
        {
          gdisp->have_cursor = FALSE;
          gdisplay_flush (gdisp);
        }
    }
  
  gdisp->have_cursor = new_cursor;
  gdisp->cursor_x = x;
  gdisp->cursor_y = y;

  if (new_cursor)
    gdisplay_flush (gdisp);
}

void
gdisplay_remove_and_delete (GDisplay *gdisp)
{
  /* remove the display from the list */
  display_list = g_slist_remove (display_list, (void *) gdisp);
  gdisplay_delete (gdisp);
}


void
gdisplay_add_update_area (GDisplay *gdisp,
                          int       x,
                          int       y,
                          int       w,
                          int       h)
{
  GArea * ga;

  ga = (GArea *) g_malloc (sizeof (GArea));
  ga->x1 = BOUNDS (x, 0, gdisp->gimage->width);
  ga->y1 = BOUNDS (y, 0, gdisp->gimage->height);
  ga->x2 = BOUNDS (x + w, 0, gdisp->gimage->width);
  ga->y2 = BOUNDS (y + h, 0, gdisp->gimage->height);

  gdisp->update_areas = gdisplay_process_area_list (gdisp->update_areas, ga);
}


static void
gdisplay_add_display_area (GDisplay *gdisp,
                           int       x,
                           int       y,
                           int       w,
                           int       h)
{
  GArea * ga;

  ga = (GArea *) g_malloc (sizeof (GArea));

  ga->x1 = BOUNDS (x, 0, gdisp->disp_width);
  ga->y1 = BOUNDS (y, 0, gdisp->disp_height);
  ga->x2 = BOUNDS (x + w, 0, gdisp->disp_width);
  ga->y2 = BOUNDS (y + h, 0, gdisp->disp_height);

  gdisp->display_areas = gdisplay_process_area_list (gdisp->display_areas, ga);
}


static void
gdisplay_paint_area (GDisplay *gdisp,
                     int       x,
                     int       y,
                     int       w,
                     int       h)
{
  int x1, y1, x2, y2;

  /*  Bounds check  */
  x1 = BOUNDS (x, 0, gdisp->gimage->width);
  y1 = BOUNDS (y, 0, gdisp->gimage->height);
  x2 = BOUNDS (x + w, 0, gdisp->gimage->width);
  y2 = BOUNDS (y + h, 0, gdisp->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /* composite the area */
  gimage_construct (gdisp->gimage, x, y, w, h);

  /*  display the area  */
  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, FALSE);
  gdisplay_expose_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
}


static void
gdisplay_display_area (GDisplay *gdisp,
		       int       x,
		       int       y,
		       int       w,
		       int       h)
{
  int sx, sy;
  int x1, y1;
  int x2, y2;
  int dx, dy;
  int i, j;

  if (!(gdisp->gimage))
    return;

  sx = SCALE (gdisp, gdisp->gimage->width);
  sy = SCALE (gdisp, gdisp->gimage->height);

  /*  Bounds check  */
  x1 = BOUNDS (x, 0, gdisp->disp_width);
  y1 = BOUNDS (y, 0, gdisp->disp_height);
  x2 = BOUNDS (x + w, 0, gdisp->disp_width);
  y2 = BOUNDS (y + h, 0, gdisp->disp_height);

  if (x1 < gdisp->disp_xoffset)
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, y, gdisp->disp_xoffset - x, h);

      x1 = gdisp->disp_xoffset;
    }

  if (y1 < gdisp->disp_yoffset)
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, y, w, gdisp->disp_yoffset - y);

      y1 = gdisp->disp_yoffset;
    }

  if (x2 > (gdisp->disp_xoffset + sx))
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     gdisp->disp_xoffset + sx, y,
			     x2 - (gdisp->disp_xoffset + sx), h);

      x2 = gdisp->disp_xoffset + sx;
    }

  if (y2 > (gdisp->disp_yoffset + sy))
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, gdisp->disp_yoffset + sy,
			     w, y2 - (gdisp->disp_yoffset + sy));

      y2 = gdisp->disp_yoffset + sy;
    }

  /*  display the image in GXIMAGE_WIDTH x GXIMAGE_HEIGHT sized chunks */
  for (i = y1; i < y2; i += GXIMAGE_HEIGHT)
    for (j = x1; j < x2; j += GXIMAGE_WIDTH)
      {
	dx = (x2 - j < GXIMAGE_WIDTH) ? x2 - j : GXIMAGE_WIDTH;
	dy = (y2 - i < GXIMAGE_HEIGHT) ? y2 - i : GXIMAGE_HEIGHT;
	render_image (gdisp, j - gdisp->disp_xoffset, i - gdisp->disp_yoffset, dx, dy);
	gximage_put (gdisp->canvas->window,
		     j, i, dx, dy, gdisp->offset_x, gdisp->offset_y);
      }
}


gfloat
gdisplay_mask_value (GDisplay *gdisp,
		     int       x,
		     int       y)
{
  /*  move the coordinates from screen space to image space  */
  gdisplay_untransform_coords (gdisp, x, y, &x, &y, FALSE, 0);

  return gimage_mask_value (gdisp->gimage, x, y);
}


int
gdisplay_mask_bounds (GDisplay *gdisp,
		      int      *x1,
		      int      *y1,
		      int      *x2,
		      int      *y2)
{
  Layer *layer;
  int off_x, off_y;

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimage_floating_sel (gdisp->gimage)))
    {
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
	{
	  *x1 = off_x;
	  *y1 = off_y;
	  *x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	  *y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
	}
      else
	{
	  *x1 = MINIMUM (off_x, *x1);
	  *y1 = MINIMUM (off_y, *y1);
	  *x2 = MAXIMUM (off_x + drawable_width (GIMP_DRAWABLE(layer)), *x2);
	  *y2 = MAXIMUM (off_y + drawable_height (GIMP_DRAWABLE(layer)), *y2);
	}
    }
  else if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
    return FALSE;

  gdisplay_transform_coords (gdisp, *x1, *y1, x1, y1, 0);
  gdisplay_transform_coords (gdisp, *x2, *y2, x2, y2, 0);

  /*  Make sure the extents are within bounds  */
  *x1 = BOUNDS (*x1, 0, gdisp->disp_width);
  *y1 = BOUNDS (*y1, 0, gdisp->disp_height);
  *x2 = BOUNDS (*x2, 0, gdisp->disp_width);
  *y2 = BOUNDS (*y2, 0, gdisp->disp_height);

  return TRUE;
}


void
gdisplay_transform_coords (GDisplay *gdisp,
			   int       x,
			   int       y,
			   int      *nx,
			   int      *ny,
			   int       use_offsets)
{
  double scale;
  int offset_x, offset_y;


  /*  transform from image coordinates to screen coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (int) (scale * (x + offset_x) - gdisp->offset_x);
  *ny = (int) (scale * (y + offset_y) - gdisp->offset_y);

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords (GDisplay *gdisp,
			     int       x,
			     int       y,
			     int      *nx,
			     int      *ny,
			     int       round,
			     int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to image coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  if (round)
    {
      *nx = ROUND ((x + gdisp->offset_x) / scale - offset_x);
      *ny = ROUND ((y + gdisp->offset_y) / scale - offset_y);
    }
  else
    {
      *nx = (int) ((x + gdisp->offset_x) / scale - offset_x);
      *ny = (int) ((y + gdisp->offset_y) / scale - offset_y);
    }
}


void
gdisplay_transform_coords_f (GDisplay *gdisp,
			     double    x,
			     double    y,
			     double   *nx,
			     double   *ny,
			     int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  /*  transform from gimp coordinates to screen coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = scale * (x + offset_x) - gdisp->offset_x;
  *ny = scale * (y + offset_y) - gdisp->offset_y;

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords_f (GDisplay *gdisp,
			       double    x,
			       double    y,
			       double   *nx,
			       double   *ny,
			       int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to gimp coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (x + gdisp->offset_x) / scale - offset_x;
  *ny = (y + gdisp->offset_y) / scale - offset_y;

}


/*  install and remove tool cursor from gdisplay...  */
void
gdisplay_install_tool_cursor (GDisplay      *gdisp,
			      GdkCursorType  cursor_type)
{
  if (gdisp->current_cursor != cursor_type || (gdisp->current_cursor == GDK_PENCIL && BRUSH_CHANGED))
    {
      if ((gdisp->current_cursor == GDK_PENCIL && BRUSH_CHANGED))
      gdisp->current_cursor = cursor_type;
      change_win_cursor (gdisp->canvas->window, cursor_type);
      BRUSH_CHANGED = 0; 
    }
}


void
gdisplay_remove_tool_cursor (GDisplay *gdisp)
{
  unset_win_cursor (gdisp->canvas->window);
}


void
gdisplay_set_menu_sensitivity (GDisplay *gdisp)
{
  Layer *layer;
  gint fs;
  gint aux;
  gint lm;
  gint lp;
  gint from_disk;
  gint alpha = FALSE;
  gchar buff[1024];
  CanvasDrawable *drawable;
  Format format;
  Precision precision;
  Tag t;

  if (gdisplay_to_ID (gdisp) < 0)
  {
    g_warning("%s:%d %s() Display not found. Maybe it is corrupt: 0x%x", __FILE__,__LINE__,__func__,(int)gdisp);
    return;
  }
#ifdef DEBUG
  else
    printf("Display found: %d  0x%x\n", gdisplay_to_ID (gdisp), (int)gdisp);
#endif

  fs = (gimage_floating_sel (gdisp->gimage) != NULL);
  aux = (gimage_get_active_channel (gdisp->gimage) != NULL);
  if ((layer = gimage_get_active_layer (gdisp->gimage)) != NULL)
      lm = (layer->mask) ? TRUE : FALSE;
  else
    lm = FALSE;
  format = tag_format (gimage_tag (gdisp->gimage));
  precision = tag_precision (gimage_tag (gdisp->gimage));
  lp = (gdisp->gimage->layers != NULL);
  alpha = layer && layer_has_alpha (layer);

  from_disk = (gdisp->gimage->has_filename && gdisp->gimage->filename != 0);

  t = tag_null ();
  if (lp)
    {
      drawable = gimage_active_drawable (gdisp->gimage);
      t = drawable_tag (drawable);
    }

  g_snprintf(buff, 1024, "<Image%d>/File/Revert to Saved", gdisp->unique_id);
  menus_set_sensitive (buff, from_disk);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Raise Layer", gdisp->unique_id);
  menus_set_sensitive (buff, !fs && !aux && lp && alpha);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Lower Layer", gdisp->unique_id);
  menus_set_sensitive (buff, !fs && !aux && lp && alpha);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Raise Layer", gdisp->unique_id);
  menus_set_sensitive (buff, fs && !aux && lp);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Merge Visible Layers", gdisp->unique_id);
  menus_set_sensitive (buff, !fs && !aux && lp);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Flatten Image", gdisp->unique_id);
  menus_set_sensitive (buff, !fs && !aux && lp);
  g_snprintf(buff, 1024, "<Image%d>/Layers/Mask To Selection", gdisp->unique_id);
  menus_set_sensitive (buff, !aux && lm && lp);
  g_snprintf(buff, 1024, "<Image%d>/Image/RGB", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_RGB));
  g_snprintf(buff, 1024, "<Image%d>/Image/Grayscale", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_GRAY));

  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Threshold", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Posterize", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Equalize", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Invert", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));

  g_snprintf(buff, 1024, "<Image%d>/Image/" PRECISION_U8_STRING, gdisp->unique_id);
  menus_set_sensitive (buff, (precision != PRECISION_U8));
  g_snprintf(buff, 1024, "<Image%d>/Image/" PRECISION_U16_STRING, gdisp->unique_id);
  menus_set_sensitive (buff, (precision != PRECISION_U16));
  g_snprintf(buff, 1024, "<Image%d>/Image/" PRECISION_FLOAT16_STRING, gdisp->unique_id);
  menus_set_sensitive (buff, (precision != PRECISION_FLOAT16));
  g_snprintf(buff, 1024, "<Image%d>/Image/" PRECISION_FLOAT_STRING, gdisp->unique_id);
  menus_set_sensitive (buff, (precision != PRECISION_FLOAT));
  g_snprintf(buff, 1024, "<Image%d>/Image/" PRECISION_BFP_STRING, gdisp->unique_id);
  menus_set_sensitive (buff, (precision != PRECISION_BFP));

  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Color Balance", gdisp->unique_id);
  menus_set_sensitive (buff, (format == FORMAT_RGB));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Color Correction", gdisp->unique_id);
  menus_set_sensitive (buff, (format == FORMAT_RGB));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Brightness-Contrast", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Hue-Saturation", gdisp->unique_id);
  menus_set_sensitive (buff, (format == FORMAT_RGB));
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors/Levels", gdisp->unique_id);
  menus_set_sensitive (buff, (format != FORMAT_INDEXED));

  g_snprintf(buff, 1024, "<Image%d>/Select", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Cut", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Copy", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Paste", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Paste Into", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Clear", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Fill", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Stroke", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Cut Named", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Copy Named", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Edit/Paste Named", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Image/Colors", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Image/Channel Ops/Offset", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Image/Histogram", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Filters", gdisp->unique_id);
  menus_set_sensitive (buff, lp);
  g_snprintf(buff, 1024, "<Image%d>/Select/Save To Channel", gdisp->unique_id);
  menus_set_sensitive (buff, !fs);
  g_snprintf(buff, 1024, "<Image%d>/View/Toggle Rulers", gdisp->unique_id);
  menus_set_state (buff, GTK_WIDGET_VISIBLE (gdisp->origin) ? 1 : 0);
  g_snprintf(buff, 1024, "<Image%d>/View/Toggle Guides", gdisp->unique_id);
  menus_set_state (buff, gdisp->draw_guides);
  g_snprintf(buff, 1024, "<Image%d>/View/Snap To Guides", gdisp->unique_id);
  menus_set_state (buff, gdisp->snap_to_guides);

  
  menus_set_sensitive ("<Image>/File/Revert to Saved", from_disk);
  menus_set_sensitive ("<Image>/Layers/Raise Layer", !fs && !aux && lp && alpha);
  menus_set_sensitive ("<Image>/Layers/Lower Layer", !fs && !aux && lp && alpha);
  menus_set_sensitive ("<Image>/Layers/Anchor Layer", fs && !aux && lp);
  menus_set_sensitive ("<Image>/Layers/Merge Visible Layers", !fs && !aux && lp);
  menus_set_sensitive ("<Image>/Layers/Flatten Image", !fs && !aux && lp);
#if 0
  menus_set_sensitive ("<Image>/Layers/Alpha To Selection", !aux && lp && alpha);
#endif
  menus_set_sensitive ("<Image>/Layers/Mask To Selection", !aux && lm && lp);
#if 0
  menus_set_sensitive ("<Image>/Layers/Add Alpha Channel", !fs && !aux && lp && !lm && !alpha);
#endif


  menus_set_sensitive ("<Image>/Image/RGB", (format != FORMAT_RGB));
  menus_set_sensitive ("<Image>/Image/Grayscale", (format != FORMAT_GRAY));
  menus_set_sensitive ("<Image>/Image/" PRECISION_U8_STRING, (precision != PRECISION_U8));
  menus_set_sensitive ("<Image>/Image/" PRECISION_U16_STRING, (precision != PRECISION_U16));
  menus_set_sensitive ("<Image>/Image/" PRECISION_FLOAT16_STRING, (precision != PRECISION_FLOAT16));
  menus_set_sensitive ("<Image>/Image/" PRECISION_FLOAT_STRING, (precision != PRECISION_FLOAT));
  menus_set_sensitive ("<Image>/Image/" PRECISION_BFP_STRING, (precision != PRECISION_BFP));  
#if 0
  menus_set_sensitive ("<Image>/Image/Indexed", (format != FORMAT_INDEXED));
#endif
  menus_set_sensitive ("<Image>/Image/Colors/Threshold", (format != FORMAT_INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Posterize", (format != FORMAT_INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Equalize", (format != FORMAT_INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Invert", (format != FORMAT_INDEXED));

  menus_set_sensitive ("<Image>/Image/Colors/Color Balance", (format == FORMAT_RGB));
  menus_set_sensitive ("<Image>/Image/Colors/Color Correction", (format == FORMAT_RGB));
  menus_set_sensitive ("<Image>/Image/Colors/Brightness-Contrast", (format != FORMAT_INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Hue-Saturation", (format == FORMAT_RGB));
#if 0
  menus_set_sensitive ("<Image>/Image/Colors/Curves", (format != FORMAT_INDEXED));
#endif
  menus_set_sensitive ("<Image>/Image/Colors/Levels", (format != FORMAT_INDEXED));
#if 0
  menus_set_sensitive ("<Image>/Image/Colors/Desaturate", (format == FORMAT_RGB));
  menus_set_sensitive ("<Image>/Image/Alpha/Add Alpha Channel", !fs && !aux && lp && !lm && !alpha);
#endif

  menus_set_sensitive ("<Image>/Select", lp);
  menus_set_sensitive ("<Image>/Edit/Cut", lp);
  menus_set_sensitive ("<Image>/Edit/Copy", lp);
  menus_set_sensitive ("<Image>/Edit/Paste", lp);
  menus_set_sensitive ("<Image>/Edit/Paste Into", lp);
  menus_set_sensitive ("<Image>/Edit/Clear", lp);
  menus_set_sensitive ("<Image>/Edit/Fill", lp);
  menus_set_sensitive ("<Image>/Edit/Stroke", lp);
  menus_set_sensitive ("<Image>/Edit/Cut Named", lp);
  menus_set_sensitive ("<Image>/Edit/Copy Named", lp);
  menus_set_sensitive ("<Image>/Edit/Paste Named", lp);
  menus_set_sensitive ("<Image>/Image/Colors", lp);
  menus_set_sensitive ("<Image>/Image/Channel Ops/Offset", lp);
  menus_set_sensitive ("<Image>/Image/Histogram", lp);
  menus_set_sensitive ("<Image>/Filters", lp);

  /* save selection to channel */
  menus_set_sensitive ("<Image>/Select/Save To Channel", !fs);

  if(gdisp->cms_intent == INTENT_PERCEPTUAL)
  {
      g_snprintf(buff, 1024, "<Image%d>/View/Rendering Intent/Perceptual", gdisp->unique_id); 
      menus_set_state (buff, TRUE);
      menus_set_state ("<Image>/View/Rendering Intent/Perceptual", TRUE);
  }
  if(gdisp->cms_intent == INTENT_RELATIVE_COLORIMETRIC)
  {
      g_snprintf(buff, 1024, "<Image%d>/View/Rendering Intent/Relative Colorimetric", gdisp->unique_id); 
      menus_set_state (buff, TRUE);
      menus_set_state ("<Image>/View/Rendering Intent/Relative Colorimetric", TRUE);
  }
  if(gdisp->cms_intent == INTENT_SATURATION)
  {
      g_snprintf(buff, 1024, "<Image%d>/View/Rendering Intent/Saturation", gdisp->unique_id); 
      menus_set_state (buff, TRUE);
      menus_set_state ("<Image>/View/Rendering Intent/Saturation", TRUE);
  }
  if(gdisp->cms_intent == INTENT_ABSOLUTE_COLORIMETRIC)
  {
      g_snprintf(buff, 1024, "<Image%d>/View/Rendering Intent/Absolute Colorimetric", gdisp->unique_id); 
      menus_set_state (buff, TRUE);
      menus_set_state ("<Image>/View/Rendering Intent/Absolute Colorimetric", TRUE);
  }
  g_snprintf(buff, 1024, "<Image%d>/View/Rendering Intent/Black Point Compensation", gdisp->unique_id);
  menus_set_state (buff,  (gdisp->cms_flags & cmsFLAGS_WHITEBLACKCOMPENSATION) != 0);
  menus_set_state ("<Image>/View/Rendering Intent/Black Point Compensation", (gdisp->cms_flags & cmsFLAGS_WHITEBLACKCOMPENSATION) != 0);
 g_snprintf(buff, 1024, "<Image%d>/View/Colormanage Display", gdisp->unique_id);
  menus_set_state (buff, gdisp->colormanaged /*&& format == FORMAT_RGB*/);
  menus_set_state ("<Image>/View/Colormanage Display", gdisp->colormanaged/* && format == FORMAT_RGB*/);
  g_snprintf(buff, 1024, "<Image%d>/View/Proof Display", gdisp->unique_id);
  menus_set_state (buff,  (gdisp->cms_flags & cmsFLAGS_SOFTPROOFING) != 0);
  menus_set_state ("<Image>/View/Proof Display", (gdisp->cms_flags & cmsFLAGS_SOFTPROOFING) != 0);
  g_snprintf(buff, 1024, "<Image%d>/View/Simulatate Paper", gdisp->unique_id);
  menus_set_state (buff, gdisp->cms_proof_flags == INTENT_ABSOLUTE_COLORIMETRIC);
  menus_set_state ("<Image>/View/Simulatate Paper", gdisp->cms_proof_flags == INTENT_ABSOLUTE_COLORIMETRIC);
  #ifdef DEBUG
    printf("%s:%d Simulate Paper?: %d\n",__FILE__,__LINE__, gdisp->cms_proof_flags == INTENT_ABSOLUTE_COLORIMETRIC);
  #endif
  g_snprintf(buff, 1024, "<Image%d>/View/Gamut Check Display", gdisp->unique_id);
  menus_set_state (buff, (gdisp->cms_flags & cmsFLAGS_GAMUTCHECK) != 0);
  menus_set_state ("<Image>/View/Gamut Check Display", (gdisp->cms_flags & cmsFLAGS_GAMUTCHECK) != 0);

  menus_set_state ("<Image>/View/Toggle Rulers", GTK_WIDGET_VISIBLE (gdisp->origin) ? 1 : 0);
  menus_set_state ("<Image>/View/Toggle Guides", gdisp->draw_guides);
  menus_set_state ("<Image>/View/Snap To Guides", gdisp->snap_to_guides);

  plug_in_set_menu_sensitivity (t);
}

void
gdisplay_expose_area (GDisplay *gdisp,
		      int       x,
		      int       y,
		      int       w,
		      int       h)
{
  gdisplay_add_display_area (gdisp, x, y, w, h);
}

void
gdisplay_expose_guide (GDisplay *gdisp,
		       Guide    *guide)
{
  int x, y;

  if (guide->position < 0)
    return;

  gdisplay_transform_coords (gdisp, guide->position,
			     guide->position, &x, &y, FALSE);

  switch (guide->orientation)
    {
    case HORIZONTAL_GUIDE:
      gdisplay_expose_area (gdisp, 0, y, gdisp->disp_width, 1);
      break;
    case VERTICAL_GUIDE:
      gdisplay_expose_area (gdisp, x, 0, 1, gdisp->disp_height);
      break;
    }
}

void
gdisplay_expose_full (GDisplay *gdisp)
{
  gdisplay_add_display_area (gdisp, 0, 0,
			     gdisp->disp_width,
			     gdisp->disp_height);
}

void
gdisplay_update_full (GDisplay *gdisp)
{
  gdisplays_update_title (gdisp-> ID);
  gdisplay_add_update_area (gdisp, 0, 0,
			     gdisp->gimage->width,
			     gdisp->gimage->height);
}


/**************************************************/
/*  Functions independent of a specific gdisplay  */
/**************************************************/

GDisplay *
gdisplay_active ()
{
  GtkWidget *event_widget;
  GtkWidget *cur_widget;
  GdkEvent *event;
  GDisplay *gdisp = NULL;
  static GDisplay *old_gdisp = NULL;

  /*  If the popup shell is valid, then get the gdisplay associated with that shell  */
  event = gtk_get_current_event ();
  event_widget = gtk_get_event_widget (event);
  if (event) gdk_event_free (event);

  if (event_widget == NULL)
  {
    return (gdisplay_to_ID( old_gdisp ) >= 0) ? old_gdisp : NULL;
  }

  cur_widget = event_widget;

  if (display_ht) {
    while (cur_widget != NULL) {
      gdisp = g_hash_table_lookup (display_ht, cur_widget);

      if (gdisp) {
#       ifdef DEBUG_
        printf("%s:%d %s() %s -> %d  0x%x\n",__FILE__,__LINE__,__func__, gtk_type_query(GTK_OBJECT_TYPE(cur_widget))->type_name, gdisp?gdisp->ID:-1, (int)gdisp);
#       endif
        old_gdisp = gdisp;
        return gdisp;
      }

      cur_widget = cur_widget->parent;
    }
  }

  if (popup_shell)
    {
      gdisp = gtk_object_get_user_data (GTK_OBJECT (popup_shell));
      old_gdisp = gdisp;
      return gdisp;
    }

# ifdef DEBUG
  printf("%s:%d %s() could not associate event %s with a gdisplay in display_ht with entries: %d\n",
  __FILE__,__LINE__,__func__,
#if GTK_MAJOR_VERSION > 1
          GTK_OBJECT_TYPE_NAME(event_widget)
#else
          gtk_type_query(GTK_OBJECT_TYPE(event_widget))->type_name
#endif
   ,g_hash_table_size(display_ht));
# endif
  return NULL;
}

void
gdisplays_delete_image (GImage *image)
{
GDisplay *gdisp;
  GSList *list = display_list;

  while (list)
    {
      gdisp = (GDisplay *) list->data;
      list = g_slist_next (list);

      if (gdisp->gimage == image)
	{
          gtk_widget_destroy (gdisp->shell);
          display_list = g_slist_remove (display_list, (void *) gdisp);
	}
    }
}

GDisplay * 
gdisplay_get_unique_id (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  Traverse the list of displays, returning the one that matches the ID  */
  /*  If no display in the list is a match, return NULL.                    */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->unique_id == ID)
	return gdisp;

      list = g_slist_next (list);
    }

  return NULL;
}

GDisplay *
gdisplay_get_ID (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  Traverse the list of displays, returning the one that matches the ID  */
  /*  If no display in the list is a match, return NULL.                    */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->ID == ID)
	return gdisp;

      list = g_slist_next (list);
    }

  return NULL;
}

int
gdisplay_to_ID (GDisplay *test_disp)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  Traverse the list of displays, returning the one that matches the ID  */
  /*  If no display in the list is a match, return -1.                    */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
#     ifdef DEBUG_
      printf("%s:%d %s() ID = %d  0x%x\n",__FILE__,__LINE__,__func__,gdisp->ID, (int)gdisp);
#     endif
      if (gdisp == test_disp)
        return gdisp->ID;

      list = g_slist_next (list);
    }

  return -1;
}

void
gdisplays_update_title (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  char title [MAX_TITLE_BUF];

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  /* format the title */
	  gdisplay_format_title (gdisp->gimage, title, gdisp);
/* rsr BUG: this is where rotate hangs: */
	  gdk_window_set_title (gdisp->shell->window, title);
	}

      list = g_slist_next (list);
    }
}


void
gdisplays_update_area (int ID,
		       int x,
		       int y,
		       int w,
		       int h)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  gdisplay_add_update_area (gdisp, x, y, w, h);
	}
      list = g_slist_next (list);
    }
}


void
gdisplays_expose_guides (int ID)
{
  GDisplay *gdisp;
  GList *tmp_list;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  tmp_list = gdisp->gimage->guides;
	  while (tmp_list)
	    {
	      gdisplay_expose_guide (gdisp, tmp_list->data);
	      tmp_list = tmp_list->next;
	    }
	}

      list = g_slist_next (list);
    }
}


void
gdisplays_expose_guide (int    ID,
			Guide *guide)
{
  GDisplay *gdisp;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	gdisplay_expose_guide (gdisp, guide);

      list = g_slist_next (list);
    }
}


void
gdisplays_update_gimage (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
          gdisplay_add_update_area (gdisp, 0, 0,
                                    gdisp->gimage->width,
                                    gdisp->gimage->height);
	}

      list = g_slist_next (list);
    }
}


GDisplay * 
gdisplay_get_from_gimage(GImage *image)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  while (list) {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == image) {
	 return gdisp;
      }

      list = g_slist_next (list);
  }
  return NULL;
}


void
gdisplays_shrink_wrap (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	shrink_wrap_display (gdisp);

      list = g_slist_next (list);
    }
}


void
gdisplays_update_full ()
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_update_full (gdisp);
      list = g_slist_next (list);
    }
}

void
gdisplays_expose_full ()
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_expose_full (gdisp);
      list = g_slist_next (list);
    }
}


void
gdisplays_selection_visibility (int              gimage_ID,
				SelectionControl function)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  int count = 0;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == gimage_ID && gdisp->select)
	{
	  switch (function)
	    {
	    case SelectionOff:
	      selection_invis (gdisp->select);
	      break;
	    case SelectionLayerOff:
	      selection_layer_invis (gdisp->select);
	      break;
	    case SelectionOn:
	      selection_start (gdisp->select, TRUE);
	      break;
	    case SelectionPause:
	      selection_pause (gdisp->select);
	      break;
	    case SelectionResume:
	      selection_resume (gdisp->select);
	      break;
	    }
	  count++;
	}

      list = g_slist_next (list);
    }
}


int
gdisplays_dirty ()
{
  int dirty = 0;
  GSList *list = display_list;

  /*  traverse the linked list of displays  */
  while (list)
    {
      if (((GDisplay *) list->data)->gimage->dirty != 0)
	dirty = 1;
      list = g_slist_next (list);
    }

  return dirty;
}


void
gdisplays_delete ()
{
  GSList *list = display_list;
  GDisplay *gdisp;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      list = g_slist_next (list);
      gtk_widget_destroy (gdisp->shell);
    }

  /*  free up linked list data  */
  g_slist_free (display_list);
}


void
gdisplays_flush ()
{
  static int flushing = FALSE;
  GSList *list = display_list;
  int lc_can_flush = 0;

  /*  no flushing necessary without an interface  */
  if (no_interface)
    return;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    return;

  flushing = TRUE;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisplay_flush ((GDisplay *) list->data);
      if(!sfm_plays((GDisplay *) list->data))
        lc_can_flush = 1;
      list = g_slist_next (list);
    }

  if (lc_can_flush)
  {
    /*  for convenience, we call the layers dialog flush here  */
    layers_dialog_flush ();
    /*  for convenience, we call the channels dialog flush here  */
    channels_dialog_flush ();
  }

  flushing = FALSE;
}

static guint
gdisplay_hash (GDisplay *display)
{
  return (gulong) display;
}

GDisplay *
gdisplay_find_display (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  Traverse the list of displays, returning the one that matches the ID  */
  /*  If no display in the list is a match, return NULL.                    */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	return gdisp;

      list = g_slist_next (list);
    }

  return NULL;
}

void       
gdisplay_set_cms_intent(GDisplay *display, int intent)
{
    if (display->colormanaged && display->cms_intent != intent)
    {
        display->cms_intent = intent;
        gdisplay_update_full(display);
        gdisplay_flush(display);
    }
}

void       
gdisplay_set_cms_flags(GDisplay *display, DWORD flags)
{   display->cms_flags = flags;
}

gboolean
gdisplay_set_colormanaged(GDisplay *display, gboolean colormanaged)
{   if (colormanaged != display->colormanaged)
    {   if ((colormanaged == TRUE) && (gimage_get_cms_profile(display->gimage) == NULL))
        {   g_warning("gdisplay_set_colormanaged: cannot colormanage. no image profile defined");  
   	    return FALSE;
	}
        if ((colormanaged == TRUE) && (cms_display_profile_name == NULL))
	{   g_warning("gdisplay_set_colormanaged: cannot colormanage. no display profile defined");
            return FALSE;
	}

	display->colormanaged = colormanaged;

	/* and update */
	gdisplay_update_full (display);
	gdisplay_flush (display);
    }

    lc_dialog_update( display->gimage );

    return TRUE;
}

gboolean
gdisplay_set_bpc(GDisplay *display, gboolean set)
{   if (set != (display->cms_flags & cmsFLAGS_WHITEBLACKCOMPENSATION))
    {   if(set == TRUE)
          if(gdisplay_set_colormanaged(display, set) == FALSE)
            return FALSE;
        if ((set == TRUE) && (cms_display_profile_name == NULL))
        {   g_warning("gdisplay_set_bpc: cannot colormanage. no display profile defined");  
            return FALSE;
        }

        display->cms_flags = set ? display->cms_flags | cmsFLAGS_WHITEBLACKCOMPENSATION :
                                 display->cms_flags & (~cmsFLAGS_WHITEBLACKCOMPENSATION) ;

        /* and update */
        gdisplay_update_full (display);
        gdisplay_flush (display);
    }

    return TRUE;
}

gboolean
gdisplay_set_paper_simulation(GDisplay *display, gboolean set)
{   if (set != gdisplay_has_paper_simulation(display) )
    {   if(set == TRUE)
          if(gdisplay_set_proofed(display, set) == FALSE)
            return FALSE;
        if ((set == TRUE) && (cms_display_profile_name == NULL))
        {   g_warning("gdisplay_set_paper_simulation: cannot colormanage. no display profile defined");  
            return FALSE;
        }

        display->cms_proof_flags = set ? INTENT_ABSOLUTE_COLORIMETRIC : INTENT_RELATIVE_COLORIMETRIC;

        /* and update */
        gdisplay_update_full (display);
        gdisplay_flush (display);
    }

    return TRUE;
}

gboolean
gdisplay_set_proofed(GDisplay *display, gboolean set)
{   if (set != (display->cms_flags & cmsFLAGS_SOFTPROOFING))
    {   if(set == TRUE)
          if(gdisplay_set_colormanaged(display, set) == FALSE)
            return FALSE;
        if ((set == TRUE) && (gimage_get_cms_proof_profile(display->gimage) == NULL))
        {   g_warning("gdisplay_set_proofed: cannot proof. no proof profile defined");
            return FALSE;
        }
        if ((set == TRUE) && (cms_display_profile_name == NULL))
        {   g_warning("gdisplay_set_proofed: cannot colormanage. no display profile defined");  
            return FALSE;
        }

        display->cms_flags = set ? display->cms_flags | cmsFLAGS_SOFTPROOFING :
                                 display->cms_flags & (~cmsFLAGS_SOFTPROOFING) ;

        /* and update */
        gdisplay_update_full (display);
        gdisplay_flush (display);
    }

    return TRUE;
}

gboolean
gdisplay_set_gamutchecked(GDisplay *display, gboolean set)
{   if (set != (display->cms_flags & cmsFLAGS_SOFTPROOFING))
    {   if(set == TRUE)
          if(gdisplay_set_colormanaged(display, set) == FALSE)
            return FALSE;
        if ((set == TRUE) && (gimage_get_cms_proof_profile(display->gimage) == NULL))
        {   g_warning("gdisplay_set_gamutchecked: cannot proof. no proof profile defined");
            return FALSE;
        }
        if ((set == TRUE) && (cms_display_profile_name == NULL))
        {   g_warning("gdisplay_set_gamutchecked: cannot colormanage. no display profile defined");  
            return FALSE;
        }

        display->cms_flags = set ? display->cms_flags | cmsFLAGS_GAMUTCHECK :
                                 display->cms_flags & (~cmsFLAGS_GAMUTCHECK) ;

        /* and update */
        gdisplay_update_full (display);
        gdisplay_flush (display);
    }

    return TRUE;
}

gboolean 
gdisplay_all_set_colormanaged(gboolean colormanaged)
{
    gboolean some_without_profile = FALSE; 
    GSList *list = display_list;
    GDisplay *current_display;
    GString *buffer = g_string_new(NULL);
    
    if ((colormanaged == TRUE) && (cms_display_profile_name == NULL))
    {   g_warning("gdisplay_all_set_colormanaged: cannot colormanage. no display profile defined");    
        return FALSE;
    }
    
    /*  traverse the linked list of displays  */
    while (list)
    {   current_display = (GDisplay *) list->data;
        list = g_slist_next (list);
        if ((colormanaged == TRUE) && (gimage_get_cms_profile(current_display->gimage) == NULL))
        {   some_without_profile = TRUE;
   	    continue;
	}
        
	current_display->colormanaged = colormanaged;

	/* and update */
	gdisplay_update_full (current_display);
	gdisplay_flush (current_display);

    }
    g_string_free(buffer, TRUE);
    
    if (some_without_profile) 
    {   g_warning("gdisplay_all_set_colormanaged: cannot colormanage some displays, because the image doesn't have a profile assigned.");
        return FALSE;
    }

    return TRUE;
}

gboolean
gdisplay_image_set_colormanaged(int image_id, gboolean colormanaged)
{
    gboolean some_without_profile = FALSE; 
    GSList *list = display_list;
    GDisplay *current_display;
    GString *buffer = g_string_new(NULL);

    if ((colormanaged == TRUE) && (cms_display_profile_name == NULL))
    {   g_warning("gdisplay_image_set_colormanaged: cannot colormanage. no display profile defined");    
        return FALSE;
    }
    
    /*  traverse the linked list of displays  */
    while (list)
    {   current_display = (GDisplay *) list->data;
        list = g_slist_next (list);
        if (current_display->gimage->ID == image_id)
        {   if ((colormanaged == TRUE) && (gimage_get_cms_profile(current_display->gimage) == NULL))
	    {   some_without_profile = TRUE;
   	        continue;
	    }
        
	    current_display->colormanaged = colormanaged;

	    /* and update */
	    gdisplay_update_full (current_display);
	    gdisplay_flush (current_display);
	}
    }
    g_string_free(buffer, TRUE);
    
    if (some_without_profile) 
    {   g_warning("gdisplay_image_set_colormanaged: cannot colormanage some displays, because the image doesn't have a profile assigned.");
        return FALSE;
    }

    return TRUE;
}

gboolean
gdisplay_image_set_proofed(int image_id, gboolean set)
{
    gboolean some_without_profile = FALSE;
    GSList *list = display_list;
    GDisplay *current_display;
    GString *buffer = g_string_new(NULL);

    if (set == TRUE)
      if (!gdisplay_image_set_colormanaged( image_id, set))
        return FALSE;

    /*  traverse the linked list of displays  */
    while (list)
    {   current_display = (GDisplay *) list->data;
        list = g_slist_next (list);
        if (current_display->gimage->ID == image_id)
        {   if ((set == TRUE) && (gimage_get_cms_proof_profile(current_display->gimage) == NULL))
            {   some_without_profile = TRUE;
                continue;
            }
        
            current_display->cms_flags = set ?
                         current_display->cms_flags | cmsFLAGS_SOFTPROOFING :
                         current_display->cms_flags & (~cmsFLAGS_SOFTPROOFING) ;

            /* and update */
            gdisplay_update_full (current_display);
            gdisplay_flush (current_display);
        }
    }
    g_string_free(buffer, TRUE);
    
    if (some_without_profile)
    {   g_warning("gdisplay_image_set_proofed: cannot colormanage some displays, because the image doesn't have a proof profile assigned.");
        return FALSE;
    }

    return TRUE;
}

gboolean
gdisplay_image_set_gamutchecked(int image_id, gboolean set)
{
    gboolean some_without_profile = FALSE;
    GSList *list = display_list;
    GDisplay *current_display;
    GString *buffer = g_string_new(NULL);

    if (set == TRUE)
      if (!gdisplay_image_set_colormanaged( image_id, set))
        return FALSE;

    /*  traverse the linked list of displays  */
    while (list)
    {   current_display = (GDisplay *) list->data;
        list = g_slist_next (list);
        if (current_display->gimage->ID == image_id)
        {   if ((set == TRUE) && (gimage_get_cms_proof_profile(current_display->gimage) == NULL))
            {   some_without_profile = TRUE;
                continue;
            }
        
            current_display->cms_flags = set ?
                           current_display->cms_flags | cmsFLAGS_GAMUTCHECK :
                           current_display->cms_flags & (~cmsFLAGS_GAMUTCHECK) ;

            /* and update */
            gdisplay_update_full (current_display);
            gdisplay_flush (current_display);
        }
    }
    g_string_free(buffer, TRUE);
    
    if (some_without_profile)
    {   g_warning("gdisplay_image_set_gamutchecked: cannot colormanage some displays, because the image doesn't have a proof profile assigned.");
        return FALSE;
    }

    return TRUE;
}


/* look profiles */
void
gdisplay_set_look_profile_pipe(GDisplay *display, GSList* pipe)
{   /* return all profiles and free the old list */
    GSList *iterator = display->look_profile_pipe;
    while (iterator != NULL)
    {   cms_return_profile((CMSProfile *)iterator->data);
        iterator = g_slist_next(iterator);
    }
    g_slist_free(display->look_profile_pipe);
    display->look_profile_pipe = pipe;

    gdisplay_update_full(display);
    gdisplay_flush(display);
}

/* gets its own reference to the profile,
   the caller has to return his reference himself */
void
gdisplay_add_look_profile(GDisplay *display, CMSProfile *profile, gint index)
{   /* there is no need to free the profiles themselves, 
       this is taken care of by cms */
    display->look_profile_pipe = g_slist_insert(display->look_profile_pipe, 
						(gpointer)cms_duplicate_profile(profile), 
						index);

    gdisplay_update_full(display);
    gdisplay_flush(display);
}

void
gdisplay_remove_look_profile(GDisplay *display, CMSProfile *profile)
{   /* return our reference to the profile and remove it from the list */
    display->look_profile_pipe = g_slist_remove(display->look_profile_pipe, (gpointer)profile);
    cms_return_profile(profile);

    gdisplay_update_full(display);
    gdisplay_flush(display);
}


CMSTransform*
gdisplay_cms_transform_get( GDisplay *gdisp,
                            Tag display_tag )
{
  CMSTransform *transform_buffer = NULL;
  GImage * image = gdisp->gimage;
  Canvas * src_canvas = gimage_projection (image);
  Tag dest_tag = display_tag;
  GSList *look_profile_pipe = gdisp->look_profile_pipe;
  int colormanaged = gdisplay_is_colormanaged( gdisp );
  int proofed = gdisplay_is_proofed( gdisp );
  int cms_intent = gdisplay_get_cms_intent( gdisp );
  int cms_flags = gdisplay_get_cms_flags( gdisp );
  int cms_proof_flags = gdisplay_get_cms_flags( gdisp );
  int cms_proof_intent = gdisplay_get_cms_proof_intent( gdisp );
  CMSTransform * cms_expensive_transf = gdisp->cms_expensive_transf;

  if (colormanaged)
  {   /* combine profiles */
      GSList *profiles = NULL;
      CMSProfile *image_profile = gimage_get_cms_profile(image);
      CMSProfile *proof_profile = NULL;
      CMSProfile *display_profile = cms_get_display_profile();

      if (!display_tag)
      {
        Format fp = FORMAT_RGB;
        Precision pp = PRECISION_U8;
        Alpha alpha = ALPHA_NONE;
        int dest_bpp = gximage_get_bpp ();
        int dest_bpl = gximage_get_bpl ();
        Tag t = 0, tag;

        fp = (dest_bpp == 3 || dest_bpp == 6) ? FORMAT_RGB : FORMAT_GRAY;
        pp = (dest_bpp == 1 || dest_bpp == 3) ? PRECISION_U8 : PRECISION_U16;
        t = tag_new (pp, fp, alpha);

        dest_tag = canvas_tag(src_canvas);

        if(t != dest_tag)
        {
#         ifdef DEBUG
          printf("%s:%d %s() display/gximage Tag: %d <-> %d\n",
                 __FILE__,__LINE__,__func__, dest_tag, t);
          tag = dest_tag;
          printf("%d: prec: %d format: %d alpha: %d channels: %d bytes: %d\n",
            tag,
            tag_precision(tag),
            tag_format(tag),
            tag_alpha(tag),
            tag_num_channels(tag),
            tag_bytes(tag)
             );
          tag = t;
          printf("%d: prec: %d format: %d alpha: %d channels: %d bytes: %d\n",
            tag,
            tag_precision(tag),
            tag_format(tag),
            tag_alpha(tag),
            tag_num_channels(tag),
            tag_bytes(tag)
             );

#         endif
        }
      }

      if (proofed)
      {
          proof_profile = gimage_get_cms_proof_profile(image);
      }
      if (image_profile == NULL)
      {   g_warning("gdisplay_project: cannot colourmanage, image has no profile assigned");
          transform_buffer = NULL;
      }
      else if (display_profile == NULL)
      {   g_warning("gdisplay_project: cannot colourmanage, no display profile set");
          transform_buffer = NULL;
      }
      else
      {   profiles = g_slist_append(profiles, (gpointer)image_profile);
          if(look_profile_pipe)
              profiles = g_slist_concat(profiles, g_slist_copy(look_profile_pipe));
          if (proof_profile)
              profiles = g_slist_append(profiles, (gpointer)proof_profile);
          else /* lcms crashes on softproofing */
              cms_flags = cms_flags & (~cmsFLAGS_SOFTPROOFING);
          profiles = g_slist_append(profiles, (gpointer)display_profile);

          transform_buffer = cms_get_canvas_transform (profiles,
                               canvas_tag(src_canvas), dest_tag,
                               cms_intent, cms_flags,
                               cms_proof_intent,
                               cms_expensive_transf);
          g_slist_free(profiles);
      }
  }
  /* if we have got look profiles ... */
  else if (!colormanaged && (look_profile_pipe != NULL))
  {   GSList *profiles = NULL;
      CMSProfile *image_profile = gimage_get_cms_profile(image);
      if (image_profile == NULL)
      {   image_profile = cms_get_srgb_profile();
      }
      profiles = g_slist_append(profiles, (gpointer)image_profile);
      profiles = g_slist_concat(profiles, g_slist_copy(look_profile_pipe));
      profiles = g_slist_append(profiles, (gpointer)image_profile);
      transform_buffer = cms_get_canvas_transform (profiles,
                               canvas_tag(src_canvas), dest_tag,
                               cms_intent, cms_flags,
                               cms_proof_intent,
                               cms_expensive_transf);
      g_slist_free(profiles);
  }

  if(!transform_buffer)
  {
    GDisplay *disp = gdisplay_get_from_gimage(image);
    if(disp)
      /* we must avoid updates in gdisplay_set_colormanaged  */
      disp->colormanaged = FALSE;
  }

  return transform_buffer;
}

