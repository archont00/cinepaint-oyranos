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
#include <string.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../actionarea.h"
#include "../canvas.h"
#include "../colormaps.h"
#include "../info_dialog.h"
#include "../info_window.h"
#include "float16.h"
#include "bfp.h"
#include "../gdisplay.h"
#include "../general.h"
#include "../gximage.h"
#include "../interface.h"
#include "../tag.h"

#define MAX_BUF 256
#define SHOW_ALPHA 1

typedef struct InfoWinData InfoWinData;
struct InfoWinData
{
  char dimensions_str[MAX_BUF];
  char scale_str[MAX_BUF];
  char color_type_str[MAX_BUF];
  char visual_class_str[MAX_BUF];
  char visual_depth_str[MAX_BUF];
  char shades_str[MAX_BUF];
  char xy_values_str[MAX_BUF];
  char xy_color_values_str[MAX_BUF];
};

/*  The different classes of visuals  */
static char *visual_classes[6];


static void
get_shades (GDisplay *gdisp,
	    char     *buf)
{
  sprintf(buf, _("Using GdkRgb - we'll get back to you"));
#if 0
  GtkPreviewInfo *info;

  info = gtk_preview_get_info ();

  switch (gimage_format (gdisp->gimage))
    {
    case FORMAT_GRAY:
      sprintf (buf, "%d", info->ngray_shades);
      break;

    case FORMAT_RGB:
      switch (gdisp->depth)
	{
	case 8 :
	  sprintf (buf, "%d / %d / %d",
		   info->nred_shades,
		   info->ngreen_shades,
		   info->nblue_shades);
	  break;
	case 15 : case 16 :
	  sprintf (buf, "%d / %d / %d",
		   (1 << (8 - info->visual->red_prec)),
		   (1 << (8 - info->visual->green_prec)),
		   (1 << (8 - info->visual->blue_prec)));
	  break;
	case 24 :
	  sprintf (buf, "256 / 256 / 256");
	  break;
	}
      break;

    case FORMAT_INDEXED:
      sprintf (buf, "%d", gdisp->gimage->num_cols);
      break;
      
    case FORMAT_NONE:
      sprintf (buf, "invalid format");
      break;
    }
#endif
}

static void
info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}

  /*  displays information:
   *    image name
   *    image width, height
   *    zoom ratio
   *    image color type
   *    Display info:
   *      visual class
   *      visual depth
   *      shades of color/gray
   */

static ActionAreaItem action_items[] =
{
  { "Close", info_window_close_callback, NULL, NULL },
};

InfoDialog *
info_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay *gdisp;
  InfoWinData *iwd;
  char * title, * title_buf;
  Format format;

  visual_classes[0] = _("Static Gray");
  visual_classes[1] = _("Grayscale");
  visual_classes[2] = _("Static Color");
  visual_classes[3] = _("Pseudo Color");
  visual_classes[4] = _("True Color");
  visual_classes[5] = _("Direct Color");


  gdisp = (GDisplay *) gdisp_ptr;

  title = prune_filename (gimage_filename (gdisp->gimage));
  format = gimage_format (gdisp->gimage);

  /*  allocate the title buffer  */
  title_buf = (char *) g_malloc (sizeof (char) * (strlen (title) + 15));
  sprintf (title_buf, _("%s: Window Info"), title);

  /*  create the info dialog  */
  info_win = info_dialog_new (title_buf,
					    gimp_standard_help_func,
					    _("UNKNOWN"));
  g_free (title_buf);

  iwd = (InfoWinData *) g_malloc_zero (sizeof (InfoWinData));
  info_win->user_data = iwd;
  iwd->dimensions_str[0] = '\0';
  iwd->scale_str[0] = '\0';
  iwd->color_type_str[0] = '\0';
  iwd->visual_class_str[0] = '\0';
  iwd->visual_depth_str[0] = '\0';
  iwd->shades_str[0] = '\0';
  iwd->xy_values_str[0] = '\0';
  iwd->xy_color_values_str[0] = '\0';

  /*  add the information fields  */
  info_dialog_add_field (info_win, _("Dimensions (w x h): "), iwd->dimensions_str);
  info_dialog_add_field (info_win, _("Scale Ratio: "), iwd->scale_str);
#if 0
  info_dialog_add_field (info_win, _("Visual Class: "), iwd->visual_class_str);
  info_dialog_add_field (info_win, _("Visual Depth: "), iwd->visual_depth_str);
  if (format == FORMAT_RGB)
    info_dialog_add_field (info_win, _("Shades of Color: "), iwd->shades_str);
  else if (format == FORMAT_INDEXED)
    info_dialog_add_field (info_win, _("Shades: "), iwd->shades_str);
  else if (format == FORMAT_GRAY)
    info_dialog_add_field (info_win, _("Shades of Gray: "), iwd->shades_str);
#endif
  info_dialog_add_field (info_win, "(x,y): ", iwd->xy_values_str);
  info_dialog_add_field (info_win, _("Value at xy: "), iwd->xy_color_values_str);
  info_dialog_add_field (info_win, _("Color Channels: "), iwd->color_type_str);

  /*  update the fields  */
  info_window_update (info_win, gdisp_ptr);

  /* Create the action area  */
  action_items[0].label = _(action_items[0].label);
  action_items[0].user_data = info_win;
  build_action_area (GTK_DIALOG (info_win->shell), action_items, 1, 0);

  return info_win;
}

void
info_window_free (InfoDialog *info_win)
{
  g_free (info_win->user_data);
  info_dialog_free (info_win);
}

Alpha
has_alpha(void       *gdisp_ptr)
{
  GDisplay *gdisp = (GDisplay *) gdisp_ptr;
  Channel *active_channel = gimage_get_active_channel(gdisp->gimage);
  Layer *active_layer = gimage_get_active_layer (gdisp->gimage);  
  CanvasDrawable *drawable = NULL;
  Tag tag;

  if (active_channel)
  {
    drawable = GIMP_DRAWABLE (active_channel);
  }
  else if (active_layer) 
  {
    drawable = GIMP_DRAWABLE (active_layer);
  }
  else
    return 0;
 
  tag = drawable_tag (drawable);
  return tag_alpha (tag);
}

void
info_window_update_xy (InfoDialog *info_win,
		    void       *gdisp_ptr,
		    int x,
		    int y)
{
  GDisplay *gdisp = (GDisplay *) gdisp_ptr;
  InfoWinData *iwd = (InfoWinData *) info_win->user_data;
  Format format;
  Canvas *c;
  Canvas *channel_canvas = NULL;
  guchar *d;
  guchar *channel_data; 
  Alpha alpha;
  Tag tag;
  ShortsFloat u, v, s, t;
  gint offset_x, offset_y;
  Channel *channel = NULL;
  Channel *active_channel = gimage_get_active_channel(gdisp->gimage);
  Layer *active_layer = gimage_get_active_layer (gdisp->gimage);  
  CanvasDrawable *drawable = NULL;
  CanvasDrawable *channel_drawable = NULL;

  /* (x,y) */
  sprintf (iwd->xy_values_str, "%d, %d",
	   (int)x, (int)y);
  
  iwd->xy_color_values_str[0] = '\0'; //truncate string
//  sprintf (iwd->xy_color_values_str, "");

  if (active_channel)
  {
    drawable = GIMP_DRAWABLE (active_channel);
  }
  else if (active_layer) 
  {
    GSList * channels_list = gimage_channels (gdisp->gimage); 
    drawable = GIMP_DRAWABLE (active_layer);
    if (channels_list)
      {
	channel = (Channel *)(channels_list->data);
	if (channel)
	   channel_drawable = GIMP_DRAWABLE(channel); 
      }
  }
  else
    return;
 
  if (drawable) 
   {
    c = drawable_data(drawable);
     if (channel_drawable)
       channel_canvas = drawable_data(channel_drawable);
   }
  else
    {
      iwd->xy_color_values_str[0] = '\0'; //truncate string
      //sprintf (iwd->xy_color_values_str, "");
      return;
    }
  
  
  tag = drawable_tag (drawable);
  format = tag_format (tag);
  alpha = tag_alpha (tag);

  drawable_offsets (drawable, &offset_x, &offset_y);
  
  /* color_values */
  if ( x >= 0 && x < gdisp->gimage->width && 
       y >= 0 && y < gdisp->gimage->height )
  {
	
  d = canvas_portion_data (c, x-offset_x, y-offset_y);
  if (channel_drawable)
       channel_data = canvas_portion_data(channel_canvas, x, y);

  if (!d)
    {
      iwd->xy_color_values_str[0] = '\0'; //truncate string
      //sprintf (iwd->xy_color_values_str, "");
      return;
    }

  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      {
	guint8* data = (guint8*)d;
	if (format == FORMAT_RGB)
#if (SHOW_ALPHA == 1)
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%3d, %3d, %3d, %3d",
		 (int)data[0], (int)data[1], (int)data[2], (int)data[3]);
	  else
#endif
	    sprintf (iwd->xy_color_values_str, "%3d, %3d, %3d",
		 (int)data[0], (int)data[1], (int)data[2]);
	else if (format == FORMAT_GRAY)
#if (SHOW_ALPHA == 1)
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%3d, %3d",
		 (int)data[0], (int)data[1]);
	  else
#endif
	    sprintf (iwd->xy_color_values_str, "%3d",
		 (int)data[0]);
	else if (format == FORMAT_INDEXED)
      iwd->xy_color_values_str[0] = '\0'; //truncate string
	   //sprintf (iwd->xy_color_values_str, "");
     }
      break;
      
    case PRECISION_U16:
      {
	guint16* data = (guint16*)d;
	if (format == FORMAT_RGB)
#if (SHOW_ALPHA == 1)
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%5d, %5d, %5d, %5d",
		 (int)data[0], (int)data[1], (int)data[2], (int)data[3]);
	  else
#endif
	    sprintf (iwd->xy_color_values_str, "%5d, %5d, %5d",
		 (int)data[0], (int)data[1], (int)data[2]);
	else if (format == FORMAT_GRAY)
#if (SHOW_ALPHA == 1)
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%5d, %5d",
		 (int)data[0], (int)data[1]);
	  else
#endif
	    sprintf (iwd->xy_color_values_str, "%5d",
		 (int)data[0]);
	else if (format == FORMAT_INDEXED)
      iwd->xy_color_values_str[0] = '\0'; //truncate string
	   //sprintf (iwd->xy_color_values_str, "");
     }
      break;
 
    case PRECISION_FLOAT:
      {
        gfloat* data = (gfloat*)d;
        if (format == FORMAT_RGB)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f, %10f, %10f",
            data[0], data[1], data[2], data[3]);
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f, %10f",
            data[0], data[1], data[2]);
          }
        } else if (format == FORMAT_GRAY)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f",
            data[0], data[1]);
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%10f",
            data[0]);
          }
        }
      }
      break;

    case PRECISION_FLOAT16:
      {
        guint16* data = (guint16*)d;
        if (format == FORMAT_RGB)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f, %10f, %10f",
            FLT(data[0],u), FLT (data[1],v), FLT (data[2],s), FLT (data[3],t) );
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f, %10f",
        	 FLT (data[0], u), FLT (data[1], v), FLT (data[2], s));
          }
        } else if (format == FORMAT_GRAY)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%10f, %10f",
        	 FLT (data[0], u), FLT (data[1], s));
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%10f",
        	 FLT (data[0], u));
          }
        }
      }
      break;

    case PRECISION_BFP:
      {
        guint16* data = (guint16*)d;
        if (format == FORMAT_RGB)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%9f, %9f, %9f, %9f",
            BFP_TO_FLOAT(data[0]), BFP_TO_FLOAT(data[1]), BFP_TO_FLOAT(data[2]), BFP_TO_FLOAT(data[3]));
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%9f, %9f, %9f",
        	BFP_TO_FLOAT(data[0]), BFP_TO_FLOAT(data[1]), BFP_TO_FLOAT(data[2]));
          }
        } else if (format == FORMAT_GRAY)
        {
#if (SHOW_ALPHA == 1)
          if (alpha == ALPHA_YES)
          {
            sprintf (iwd->xy_color_values_str, "%9f, %9f",
            BFP_TO_FLOAT(data[0]), BFP_TO_FLOAT(data[1]));
          } else
#endif
          {
            sprintf (iwd->xy_color_values_str, "%9f",
            BFP_TO_FLOAT(data[0]));
          }
        } else if (format == FORMAT_INDEXED)
          iwd->xy_color_values_str[0] = '\0'; //truncate string
	   //sprintf (iwd->xy_color_values_str, "");
      }
      break;

    case PRECISION_NONE:
      g_warning ("info_window_update_xy: bad precision");
      break;
    }
  }
  else 
    iwd->xy_color_values_str[0] = '\0'; //truncate string
    //sprintf (iwd->xy_color_values_str, "");

  info_dialog_update (info_win);
}


void
info_window_update (InfoDialog *info_win,
		    void       *gdisp_ptr)
{
  GDisplay *gdisp;
  InfoWinData *iwd;
  Format format;
  const char** color_space_channel_names = 0;

  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  /*  width and height  */
  sprintf (iwd->dimensions_str, "%d x %d",
	   (int) gdisp->gimage->width, (int) gdisp->gimage->height);

  /*  zoom ratio  */
  sprintf (iwd->scale_str, "%d:%d",
	   SCALEDEST (gdisp), SCALESRC (gdisp));

  format = gimage_format (gdisp->gimage);

  /*  color type  */
# if 0
  if (format == FORMAT_RGB)
    sprintf (iwd->color_type_str, "%s", "RGB Color");
  else if (format == FORMAT_GRAY)
    sprintf (iwd->color_type_str, "%s", "Grayscale");
  else if (format == FORMAT_INDEXED)
    sprintf (iwd->color_type_str, "%s", "Indexed Color");
# else
  color_space_channel_names = gimage_get_channel_names(gdisp->gimage);
  if (format == FORMAT_RGB) {
    sprintf (iwd->color_type_str, "%s, %s, %s", color_space_channel_names[RED_PIX],color_space_channel_names[GREEN_PIX],color_space_channel_names[BLUE_PIX]);
  } else if (format == FORMAT_GRAY)
    sprintf (iwd->color_type_str, "%s", _("Gray"));
  else if (format == FORMAT_INDEXED)
    sprintf (iwd->color_type_str, "%s", _("Indexed Color"));
  if(has_alpha(gdisp) == ALPHA_YES)
    sprintf (&iwd->color_type_str[strlen(iwd->color_type_str)], ", %s", color_space_channel_names[ALPHA_PIX]);
# endif

  /*  visual class  */
  if (format == FORMAT_RGB ||
      format == FORMAT_INDEXED)
    sprintf (iwd->visual_class_str, "%s", visual_classes[g_visual->type]);
  else if (format == FORMAT_GRAY)
    sprintf (iwd->visual_class_str, "%s", visual_classes[g_visual->type]);

  /*  visual depth  */
  sprintf (iwd->visual_depth_str, "%d", gdisp->depth);

  /*  pure color shades  */
  get_shades (gdisp, iwd->shades_str);
#if 0  
  /*  xy_color_values  */
  sprintf (iwd->xy_color_values_str, "%d, %d, %d, %d",
	   (int)0, (int)0, (int)0, (int)0 );
#endif
  info_dialog_update (info_win);
}
