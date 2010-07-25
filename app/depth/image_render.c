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
#include <math.h>
#include "../appenv.h"
#include "../canvas.h"
#include "../channel.h"
#include "../colormaps.h"
#include "../errors.h"
#include "float16.h"
#include "bfp.h"
#include "../rc.h"
#include "../gximage.h"
#include "../image_render.h"
#include "../paint_funcs_area.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../scale.h"
#include "../tag.h"
#include "../tilebuf.h"
#include "../trace.h"
#include "displaylut.h"

#define MAX_CHANNELS 4

#define ALL_INTEN_VISIBLE 0
#define NO_INTEN_VISIBLE 1
#define EXACTLY_ONE_INTEN_VISIBLE 2
#define TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE 3  

#define EXACTLY_ONE_AUX_VISIBLE 0
#define NO_AUX_VISIBLE 1 
#define ONE_OR_MORE_AUX_VISIBLE 2 

TraceTimer trace_timer;

typedef struct RenderInfo  RenderInfo;
typedef void (*RenderFunc) (RenderInfo *info);

struct RenderInfo
{
  GDisplay *gdisp;
  Canvas *src_canvas;
  guint src_width;
  guint *alpha;
  guchar *scale;
  guchar *src;
  guchar *dest;
  int x, y;
  int w, h;
  int scalesrc;
  int scaledest;
  int src_x, src_y;
  int src_w, src_h;
  int src_bpp;
  int src_num_channels;
  int visible[MAX_CHANNELS];
  int intensity_visible;  
  int dest_bpp;
  int dest_bpl;
  int dest_width;
  GSList *channels;
  Channel *single_visible_channel;
  int aux_channels_visible;  
  CMSTransform *transform_buffer;
};

CMSTransform* render_image_get_cms_transform( GDisplay   * gdisp ,
                                              RenderInfo * info);

/*  accelerate transparency of image scaling  */
guchar *blend_dark_check = NULL;
guchar *blend_light_check = NULL;
guchar *tile_buf = NULL;
guchar *check_buf = NULL;
guchar *empty_buf = NULL;
guchar *temp_buf = NULL;

static guint   check_mod;
static guint   check_shift;
static guchar  check_combos[6][2] =
{
  { 204, 255 },
  { 102, 153 },
  { 0, 51 },
  { 255, 255 },
  { 127, 127 },
  { 0, 0 }
};

static guint force_expose = 0;
//static guint force_clamp = 0;

float image_render_get_force_expose()
{
    return (force_expose);
}
/*float image_render_get_clamp()
{
    return (force_clamp);
}*/


void image_render_set_force_expose(guint val)
{
    force_expose = val;
}
/*void image_render_set_force_clamp(guint val)
{
    force_clamp = val;
}*/

void
render_setup (int check_type,
	      int check_size)
{
  int i, j;

  /*  allocate a buffer for arranging information from a row of tiles  */
  if (!tile_buf)
    tile_buf = g_new (guchar, GXIMAGE_WIDTH * MAX_CHANNELS * 4);
  
  if (check_type < 0 || check_type > 5)
    g_error ("invalid check_type argument to render_setup: %d", check_type);
  if (check_size < 0 || check_size > 2)
    g_error ("invalid check_size argument to render_setup: %d", check_size);

  if (!blend_dark_check)
    blend_dark_check = g_new (guchar, 65536);
  if (!blend_light_check)
    blend_light_check = g_new (guchar, 65536);

  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
      {
	blend_dark_check [(i << 8) + j] = (guchar)
	  ((j * i + check_combos[check_type][0] * (255 - i)) / 255);
	blend_light_check [(i << 8) + j] = (guchar)
	  ((j * i + check_combos[check_type][1] * (255 - i)) / 255);
      }

  switch (check_size)
    {
    case SMALL_CHECKS:
      check_mod = 0x3;
      check_shift = 2;
      break;
    case MEDIUM_CHECKS:
      check_mod = 0x7;
      check_shift = 3;
      break;
    case LARGE_CHECKS:
      check_mod = 0xf;
      check_shift = 4;
      break;
    }

  /*  calculate check buffer for previews  */
  if (preview_size)
    {
      if (check_buf)
	g_free (check_buf);
      if (empty_buf)
	g_free (empty_buf);
      if (temp_buf)
	g_free (temp_buf);

      check_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
      for (i = 0; i < (preview_size + 4); i++)
	{
	  if (i & 0x4)
	    {
	      check_buf[i * 3 + 0] = blend_dark_check[0];
	      check_buf[i * 3 + 1] = blend_dark_check[0];
	      check_buf[i * 3 + 2] = blend_dark_check[0];
	    }
	  else
	    {
	      check_buf[i * 3 + 0] = blend_light_check[0];
	      check_buf[i * 3 + 1] = blend_light_check[0];
	      check_buf[i * 3 + 2] = blend_light_check[0];
	    }
	}
      empty_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
      memset (empty_buf, 0, (preview_size + 4) * 3);
      temp_buf = (unsigned char *) g_malloc ((preview_size + 4) * 3);
    }
  else
    {
      check_buf = NULL;
      empty_buf = NULL;
      temp_buf = NULL;
    }
}

void
render_free (void)
{
  g_free (tile_buf);
  g_free (check_buf);
}

/*  Render Image functions  */

static void    render_image_indexed   (RenderInfo *info);
static void    render_image_indexed_a (RenderInfo *info);
static void    render_image_gray      (RenderInfo *info);
static void    render_image_gray_a    (RenderInfo *info);
static void    render_image_rgb       (RenderInfo *info);
static void    render_image_rgb_a     (RenderInfo *info);

static void    render_image_gray_u16      (RenderInfo *info);
static void    render_image_gray_a_u16    (RenderInfo *info);
static void    render_image_rgb_u16       (RenderInfo *info);
static void    render_image_rgb_a_u16     (RenderInfo *info);

static void    render_image_gray_float      (RenderInfo *info);
static void    render_image_gray_a_float    (RenderInfo *info);
static void    render_image_rgb_float       (RenderInfo *info);
static void    render_image_rgb_a_float     (RenderInfo *info);

static void    render_image_gray_float16      (RenderInfo *info);
static void    render_image_gray_a_float16    (RenderInfo *info);
static void    render_image_rgb_float16       (RenderInfo *info);
static void    render_image_rgb_a_float16     (RenderInfo *info);

static void    render_image_gray_bfp      (RenderInfo *info);
static void    render_image_gray_a_bfp    (RenderInfo *info);
static void    render_image_rgb_bfp       (RenderInfo *info);
static void    render_image_rgb_a_bfp     (RenderInfo *info);

static void    render_image_init_info          (RenderInfo   *info,
						GDisplay     *gdisp,
						int           x,
						int           y,
						int           w,
						int           h);
static guint*  render_image_init_alpha         (gfloat        opacity);
static guchar* render_image_accelerate_scaling (int           width,
						int           start,
						int           bpp,
						int           scalesrc,
						int           scaledest);
static guchar* render_image_tile_fault         (RenderInfo   *info);


static RenderFunc render_funcs[6] =
{
  render_image_rgb,
  render_image_rgb_a,
  render_image_gray,
  render_image_gray_a,
  render_image_indexed,
  render_image_indexed_a,
};

static RenderFunc render_funcs_u16[4] =
{
  render_image_rgb_u16,
  render_image_rgb_a_u16,
  render_image_gray_u16,
  render_image_gray_a_u16,
};

static RenderFunc render_funcs_float[4] =
{
  render_image_rgb_float,
  render_image_rgb_a_float,
  render_image_gray_float,
  render_image_gray_a_float,
};

static RenderFunc render_funcs_float16[4] =
{
  render_image_rgb_float16,
  render_image_rgb_a_float16,
  render_image_gray_float16,
  render_image_gray_a_float16,
};

static RenderFunc render_funcs_bfp[4] =
{
  render_image_rgb_bfp,
  render_image_rgb_a_bfp,
  render_image_gray_bfp,
  render_image_gray_a_bfp,
};

/*****************************************************************/
/*  This function is the core of the display--it offsets and     */
/*  scales the image according to the current parameters in the  */
/*  gdisp object.  It handles color, grayscale, 8, 15, 16, 24,   */
/*  & 32 bit output depths.                                      */
/*****************************************************************/

void
render_image (GDisplay *gdisp,
	      int       x,
	      int       y,
	      int       w,
	      int       h)
{
  RenderInfo info;
  Tag t;
  
  render_image_init_info (&info, gdisp, x, y, w, h);

  t = canvas_tag (info.src_canvas);

  switch (tag_format (t))
    {
    case FORMAT_RGB:
    case FORMAT_GRAY:
      break;

    case FORMAT_INDEXED:      
      if (tag_precision (t) != PRECISION_U8)
        {
          g_warning ("indexed images only supported in 8 bit mode");
          return;
        }
        break;

    case FORMAT_NONE:
    default:
      g_warning ("unsupported gimage projection type");
      return;
    }

  if ((info.dest_bpp < 1) || (info.dest_bpp > 4))
    {
      g_message ("unsupported destination bytes per pixel: %d", info.dest_bpp);
      return;
    }
  
  {
    int image_type = tag_to_drawable_type (tag_set_precision (t, PRECISION_U8));
  
    switch (tag_precision (t))
      {
      case PRECISION_U8:
        (* render_funcs[image_type]) (&info);
        break;
      case PRECISION_U16:
        (* render_funcs_u16[image_type]) (&info);
  	break;
      case PRECISION_FLOAT:
        (* render_funcs_float[image_type]) (&info);
  	break;
      case PRECISION_FLOAT16:
        (* render_funcs_float16[image_type]) (&info);
  	break;
      case PRECISION_BFP:
         (* render_funcs_bfp[image_type]) (&info);
  	break;	
      case PRECISION_NONE:
        break;
      }
  }

  if(info.transform_buffer)
    cms_return_transform(info.transform_buffer);
}



/*************************/
/*  8 Bit functions      */
/*************************/

static void
render_image_indexed (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guchar *cmap;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  cmap = gimage_cmap (info->gdisp->gimage);

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);
	  
	  for (x = info->x; x < xe; x++)
	    {
	      val = src[INDEXED_PIX] * 3;
	      src += 1;

	      dest[0] = cmap[val+0];
	      dest[1] = cmap[val+1];
	      dest[2] = cmap[val+2];
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;
 
      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_indexed_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  guchar *cmap;
  gulong r, g, b;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  cmap = gimage_cmap (info->gdisp->gimage);
  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_I_PIX]];
	      val = src[INDEXED_PIX] * 3;
	      src += 2;

	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | cmap[val+0])];
		  g = blend_dark_check[(a | cmap[val+1])];
		  b = blend_dark_check[(a | cmap[val+2])];
		}
	      else
		{
		  r = blend_light_check[(a | cmap[val+0])];
		  g = blend_light_check[(a | cmap[val+1])];
		  b = blend_light_check[(a | cmap[val+2])];
		}

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;
	  
	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      val = src[GRAY_PIX];
	      src += 1;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_G_PIX]];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | src[GRAY_PIX])];
	      else
		val = blend_light_check[(a | src[GRAY_PIX])];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
          int i = (xe - info->x) * 3;
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);
	  
# if 0
	  /* replace this with memcpy, or better yet, avoid it altogether? */
	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = src[0];
	      dest[1] = src[1];
	      dest[2] = src[2];

	      src += 3;
	      dest += 3;	      
	    }
# else
          memcpy(dest, src, i);
          src += i;
          dest += i;
# endif
	}            
      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_PIX]];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | src[RED_PIX])];
		  g = blend_dark_check[(a | src[GREEN_PIX])];
		  b = blend_dark_check[(a | src[BLUE_PIX])];
		}
	      else
		{
		  r = blend_light_check[(a | src[RED_PIX])];
		  g = blend_light_check[(a | src[GREEN_PIX])];
		  b = blend_light_check[(a | src[BLUE_PIX])];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_init_info (RenderInfo *info,
			GDisplay   *gdisp,
			int         x,
			int         y,
			int         w,
			int         h)
{
  gint i;
  Tag src_canvas_tag;
  gint num_inten, num_visible_inten; 
  gint num_visible_aux_chans; 
  Channel *single_visible_channel = NULL;
  GSList * list;
  CMSTransform *         transform_buffer=NULL;

  info->gdisp = gdisp;
  info->x = x + gdisp->offset_x;
  info->y = y + gdisp->offset_y;
  info->w = w;
  info->h = h;
  info->scalesrc = SCALESRC (gdisp);
  info->scaledest = SCALEDEST (gdisp);
  info->src_x = UNSCALE (gdisp, info->x);
  info->src_y = UNSCALE (gdisp, info->y);
  info->src_w = UNSCALE (gdisp, (info->x + info->w)) - UNSCALE (gdisp, info->x) + 1;

  #if 0
  info->src_canvas = gdisplay_get_projection (gdisp);
  #else
  info->src_canvas = gimage_projection (gdisp->gimage);
  #endif
  info->channels = gimage_channels (gdisp->gimage);
  info->src_width = canvas_width (info->src_canvas);
  src_canvas_tag = canvas_tag (info->src_canvas);
  info->src_bpp = tag_bytes (src_canvas_tag);
  info->src_num_channels = tag_num_channels (src_canvas_tag);

  if (tag_alpha (src_canvas_tag) == ALPHA_YES )
    num_inten = info->src_num_channels - 1;
  else
    num_inten = info->src_num_channels;

  num_visible_inten = 0; 
  for(i = 0; i < num_inten; i++)
    {
      info->visible[i] = gimage_get_component_visible (gdisp->gimage, i); 
      if(info->visible[i]) 
	num_visible_inten++; 
    }

  if (num_visible_inten == num_inten )
    info->intensity_visible = ALL_INTEN_VISIBLE;
  else if (num_visible_inten == 0)
    info->intensity_visible = NO_INTEN_VISIBLE;
  else if (num_visible_inten == 1)
    info->intensity_visible = EXACTLY_ONE_INTEN_VISIBLE;
  else
    info->intensity_visible = TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE;

  list = info->channels;
  num_visible_aux_chans = 0; 
  while (list)
    {
      Channel *channel = (Channel *)(list->data);
      if (channel)
	{
	  gint visible = channel_visibility (channel);
	  if (visible)
	    {
	      single_visible_channel = channel;
	      num_visible_aux_chans++;  
	    }
	}
      list = g_slist_next (list);
    }

  if (num_visible_aux_chans == 0)
    info->aux_channels_visible = NO_AUX_VISIBLE;
  else if (num_visible_aux_chans == 1)
    info->aux_channels_visible = EXACTLY_ONE_AUX_VISIBLE;
  else if (num_visible_aux_chans >= 1)
    info->aux_channels_visible = ONE_OR_MORE_AUX_VISIBLE;

  if (num_visible_aux_chans == 1) 
    info->single_visible_channel = single_visible_channel;
  else
    info->single_visible_channel = NULL;

  info->dest = gximage_get_data ();
  info->dest_bpp = gximage_get_bpp ();
  info->dest_bpl = gximage_get_bpl ();
  info->dest_width = info->w * info->dest_bpp;

  info->scale = render_image_accelerate_scaling (w, info->x, info->src_bpp, info->scalesrc, info->scaledest);
  info->alpha = NULL;

  if (tag_alpha (canvas_tag (info->src_canvas)) == ALPHA_YES)
    info->alpha = render_image_init_alpha (gimage_projection_opacity (gdisp->gimage));

  /* if colormanaged, get the required transform
   */
  info->transform_buffer = render_image_get_cms_transform( info->gdisp, info );
}

CMSTransform*
render_image_get_cms_transform( GDisplay * gdisp, RenderInfo * info)
{
  CMSTransform *transform_buffer = NULL;

  GImage * image = gdisp->gimage;
  GSList *look_profile_pipe = gdisp->look_profile_pipe;
  int colormanaged = gdisplay_is_colormanaged(gdisp);
  int proofed = gdisplay_is_proofed(gdisp);
  int cms_intent = gdisplay_get_cms_intent(gdisp);
  int cms_flags = gdisplay_get_cms_flags(gdisp);
  int cms_proof_flags = gdisplay_get_cms_proof_intent(gdisp);
  CMSTransform **cms_expensive_transf = gdisp->cms_expensive_transf;

  Canvas * src_canvas = gimage_projection (image);
  Tag src_tag = canvas_tag( src_canvas );
  Tag dest_tag = 0;

  if(info->dest_bpp < 3)
    dest_tag = tag_new( tag_precision(src_tag), FORMAT_GRAY,
                        tag_alpha(src_tag));
  else
    dest_tag = tag_new( tag_precision(src_tag), FORMAT_RGB,
                        tag_alpha(src_tag));

  if (colormanaged)
  {   /* combine profiles */
      GSList *profiles = NULL;
      CMSProfile *image_profile = gimage_get_cms_profile(image);
      CMSProfile *proof_profile = NULL;
      CMSProfile *display_profile = cms_get_display_profile();
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
                               dest_tag, dest_tag,
                               cms_intent, cms_flags,
                               cms_proof_flags,
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
                               dest_tag, dest_tag,
                               cms_intent, cms_flags,
                               cms_proof_flags,
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

static guint*
render_image_init_alpha (gfloat mult)
{
  static guint *alpha_mult = NULL;
  static gfloat alpha_val = -1.0;
  int i;

  if (alpha_val != mult)
    {
      if (!alpha_mult)
	alpha_mult = g_new (guint, 256);

      alpha_val = mult;
      for (i = 0; i < 256; i++)
	alpha_mult[i] = ((i * (guint) (mult * 255)) / 255) << 8;
    }

  return alpha_mult;
}

static guchar*
render_image_accelerate_scaling (int width,
				 int start,
				 int  bpp,
				 int  scalesrc,
				 int  scaledest)
{
  static guchar *scale = NULL;
  static int swidth = -1;
  static int sstart = -1;
  guchar step;
  int i;

  if ((swidth != width) || (sstart != start))
    {
      if (!scale)
	scale = g_new (guchar, GXIMAGE_WIDTH + 1);

      step = scalesrc * bpp;

      for (i = 0; i <= width; i++)
	scale[i] = ((i + start + 1) % scaledest) ? 0 : step;
    }

  return scale;
}

void
set_visible_channels_row_u8(
             guchar * buffer,
             int num_pixels,
             int *vmask,
             int *smask,
	         int num_channels,
             int n,
             int is_lab
             )
{
  gint    b;
  guint8 *data         = (guint8*) buffer;
  gint    dark_color = 0;

  if (is_lab)
      dark_color = 128;

  while (num_pixels--)
    {
      for (b = 0; b < n; b++)
        data[b] =  vmask[b] ? data[smask[b]] : dark_color;
      data += num_channels;
    }
}

void
set_visible_channels_row_u16(
             guchar * buffer,
             int num_pixels,
             int *vmask,
             int *smask,
	         int num_channels,
             int n,
             int is_lab
             )
{
  gint    b;
  guint16 *data         = (guint16*) buffer;
  gint    dark_color = 0;

  if (is_lab)
      dark_color = 32768;

  while (num_pixels--)
    {
      for (b = 0; b < n; b++)
        data[b] =  vmask[b] ? data[smask[b]] : dark_color;
      data += num_channels;
    }
}

void
set_visible_channels_row_float(
             guchar * buffer,
             int num_pixels,
             int *vmask,
             int *smask,
	         int num_channels,
             int n,
             int is_lab
             )
{
  gint    b;
  gfloat *data         = (gfloat*) buffer;
  gfloat  dark_color = 0.0;

  if (is_lab)
      dark_color = 0.5;

  while (num_pixels--)
    {
      for (b = 0; b < n; b++)
        data[b] =  vmask[b] ? data[smask[b]] : dark_color;
      data += num_channels;
    }
}

void
set_visible_channels_row_float16(
             guchar * buffer,
             int num_pixels,
             int *vmask,
             int *smask,
	         int num_channels,
             int n,
             int is_lab
             )
{
  gint    b;
  guint16 *data         = (guint16*) buffer;

  while (num_pixels--)
    {
      for (b = 0; b < n; b++)
        data[b] =  vmask[b] ? data[smask[b]] : ZERO_FLOAT16;
      data += num_channels;
    }
}

void
set_visible_channels_row_bfp(
             guchar * buffer,
             int num_pixels,
             int *vmask,
             int *smask,
	         int num_channels,
             int n,
             int is_lab
             )
{
  gint    b;
  guint16 *data         = (guint16*) buffer;

  while (num_pixels--)
    {
      for (b = 0; b < n; b++)
        data[b] =  vmask[b] ? data[smask[b]] : 0;
      data += num_channels;
    }
}
 

typedef void (*SetVisibleChannelsFunc) (guchar*, int, int*, int*, int,int, int);
static SetVisibleChannelsFunc 
set_visible_channels_func (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return set_visible_channels_row_u8;
    case PRECISION_U16:
      return set_visible_channels_row_u16;
    case PRECISION_FLOAT16:
      return set_visible_channels_row_float16;
    case PRECISION_FLOAT:
      return set_visible_channels_row_float;
    case PRECISION_BFP:
      return set_visible_channels_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("set_visible_channels_func: bad precision");
    }
  return NULL;
}

void 
image_render_set_visible_channels(GImage *gimage,
				  int       x,
				  int       y,
				  int       w,
				  int       h)
{   int i;
    void * pag;
    Canvas *projection = gimage_projection(gimage);
    PixelArea projectionPA;
    Tag projection_tag = canvas_tag(projection);
    int num_inten = tag_num_channels(projection_tag);
    int visible[MAX_CHANNELS];
    SetVisibleChannelsFunc func=set_visible_channels_func(projection_tag);
    gint is_lab = 0, is_cmyk = 0;
    gint  smask[4] = {0,1,2,3},    /**<@brief source mask */
          vmask[4] = {0,1,2,3};    /**<@brief display colours mask */

    if (gimage->cms_profile)
    {   if (strstr("Lab", cms_get_profile_cspace( gimage->cms_profile )))
            is_lab = 1;
        else
        if (strstr("Cmyk", cms_get_profile_cspace( gimage->cms_profile )))
            is_cmyk = 1;
    }

    if (tag_alpha (projection_tag) == ALPHA_YES &&
        !strstr("Cmyk", cms_get_profile_cspace( gimage->cms_profile )))
      num_inten--;

    for(i = 0; i < num_inten; i++)
      visible[i] = gimage_get_component_visible (gimage, i); 

    /* set single CIE*a und CIE*b visible channel to gray output */
    if (is_lab &&
      num_inten == 3 &&
      !visible[0] && 
      (visible[1] + visible[2] == 1) )
    {
      smask[0] = visible[1] ? 1 : 2;
      vmask[0] = 1;
      vmask[1] = vmask[2] = 0;
    }
    else
    /* set single 3 colour image to gray output */
    if (!is_lab && !is_cmyk &&
      num_inten == 3 &&
      (visible[0] + visible[1] + visible[2]) == 1 )
    { for(i = 0; i < num_inten; ++i)
        {
          vmask[i] = 1;
          smask[i] = visible[0] ? 0 : (visible[1] ? 1 : 2);
        }
    }
    else
    /* set single 4 colour cmyk image to gray output */
    if (is_cmyk &&
      num_inten == 4 &&
      (visible[0] + visible[1] + visible[2]) + visible[3] == 1 )
    { for(i = 0; i < num_inten; ++i)
        {
          vmask[i] = 1;
          smask[i] = visible[0] ? 0 : (visible[1] ? 1 : (visible[2] ? 2 : 3));
        }
    }
    else
    /* simply show natural colour */
    { for(i = 0; i < num_inten; ++i)
          vmask[i] = visible[i];
    }

    pixelarea_init (&projectionPA, projection, x, y, w, h, TRUE);
    for (pag = pixelarea_register (1, &projectionPA);
	 pag != NULL;
	 pag = pixelarea_process (pag))
    {   guint h = pixelarea_height(&projectionPA);	  
        PixelRow row_buffer;
        guint8 *data;
	guint num_pixels;
	
	while (h--)
        {   pixelarea_getdata(&projectionPA, &row_buffer, h);
   	    data = pixelrow_data(&row_buffer);
	    num_pixels = pixelrow_width(&row_buffer);
	    (*func)(data,num_pixels,vmask,smask,
                tag_num_channels (projection_tag),num_inten,is_lab);
	}
    }
}

#if 0
      pixelarea_init (&aux_channel_area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE);
      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, FALSE); 
      d_printf("aux_channel before\n");
      print_area(&aux_channel_area,0,1); 
      d_printf("comp_area before\n");
      print_area(&comp_area,0,1); 
#endif
#if 0
      pixelarea_init (&aux_channel_area, aux_channel_canvas, src_x, src_y, src_w, 1, FALSE);
      pixelarea_init (&comp_area, comp_canvas, 0, 0, src_w, 1, FALSE); 
      d_printf("aux_channel after\n");
      print_area(&aux_channel_area,0,1); 
      d_printf("comp_area after\n");
      print_area(&comp_area,0,1); 
#define ALL_INTEN_VISIBLE 0
#define NO_INTEN_VISIBLE 1
#define EXACTLY_ONE_INTEN_VISIBLE 2
#define TWO_OR_MORE_NOT_ALL_INTEN_VISIBLE 3  

#define EXACTLY_ONE_AUX_VISIBLE 0
#define NO_AUX_VISIBLE 1 
#define ONE_OR_MORE_AUX_VISIBLE 2 
#endif


#if 1
static guchar*
render_image_tile_fault_expose (RenderInfo *info,
                                void *in, void *out, int samples,
                                float offset, float expose, float gamma,
                                int has_alpha, Precision prec)
{
  int i;
  float   *f, ff_;
  guint16 *u16;
  guint8  *u8;
  float    sb, db;
  ShortsFloat u;

  f = (float*)tile_buf;
  u16 = (guint16*)tile_buf;
  u8 = (guint8*)tile_buf;
  if(info->gdisp->expose != EXPOSE_DEFAULT ||
     info->gdisp->offset != OFFSET_DEFAULT ||
     info->gdisp->gamma  != GAMMA_DEFAULT)
  {
    switch (prec)
    {
      case PRECISION_FLOAT:
           if(has_alpha)
           {
             for(i = 0; i < samples; ++i)
             { if ((i+1)%info->src_num_channels)
                 *f = pow((*f-offset) * expose, gamma);
                 ++f;
             }
           } else
         { int n = 12;
           for(i = 0; i < samples/n; ++i)
           {
             f[0] -= offset;
             f[1] -= offset;
             f[2] -= offset;
             f[3] -= offset;
             f[4] -= offset;
             f[5] -= offset;
             f[6] -= offset;
             f[7] -= offset;
             f[8] -= offset;
             f[9] -= offset;
             f[10]-= offset;
             f[11]-= offset;
             f[0] *= expose;
             f[1] *= expose;
             f[2] *= expose;
             f[3] *= expose;
             f[4] *= expose;
             f[5] *= expose;
             f[6] *= expose;
             f[7] *= expose;
             f[8] *= expose;
             f[9] *= expose;
             f[10]*= expose;
             f[11]*= expose;
             f[0] = pow(  f[0], gamma);
             f[1] = pow(  f[1], gamma);
             f[2] = pow(  f[2], gamma);
             f[3] = pow(  f[3], gamma);
             f[4] = pow(  f[4], gamma);
             f[5] = pow(  f[5], gamma);
             f[6] = pow(  f[6], gamma);
             f[7] = pow(  f[7], gamma);
             f[8] = pow(  f[8], gamma);
             f[9] = pow(  f[9], gamma);
             f[10]= pow(  f[10],gamma);
             f[11]= pow(  f[11],gamma);
             f += n;
           }
           samples = samples % n;
           for (i = 0; i < samples; ++i)
           {
             f[0] -= offset;
             f[0] *= expose;
             f[0] = pow(  f[0], gamma);
             ++f;
           }
         }
           break;
      case PRECISION_FLOAT16:
           for(i = 0; i < samples; ++i)
           {
             if ((has_alpha && (i+1)%info->src_num_channels) ||
                 !has_alpha)
             {
               sb = FLT (*u16, u);
               db = pow (sb*expose, gamma);
               *u16++ = FLT16 (db, u);
             } else
               ++u16;
           }
           break;
      case PRECISION_BFP:
           for(i = 0; i < samples; ++i)
           { if ((has_alpha && (i+1)%info->src_num_channels) ||
                 !has_alpha)
               *u16 = CLAMP(32768.f * pow(*u16/32768.f*expose, 
                            gamma), 0, 65535);
             ++u16;
           }
           break;
      case PRECISION_U8:
           for(i = 0; i < samples; ++i)
           { if ((has_alpha && (i+1)%info->src_num_channels) ||
                 !has_alpha)
               *u8 =  CLAMP(255.f * pow(*u8/255.f * expose, gamma),
                            0, 255);
              ++u8;
           }
           break;
      case PRECISION_U16:
           if(has_alpha)
           {
             for(i = 0; i < samples; ++i)
               { if ((i+1)%info->src_num_channels)
                   *u16 = CLAMP(65535.f * pow(*u16/65535.f * expose,gamma),
                                0, 65535);
                   ++u16;
               }
           } else
           for(i = 0; i < samples; ++i)
           {
             ff_ = (float)*u16 / 65535.f;
             ff_ -= offset;
             ff_ *= expose;
             ff_ = pow(  ff_, gamma);
             ff_ *= 65535.f;
             ff_ = CLAMP(ff_, 0, 65535.f);
             *u16++ = (guint16)ff_;
           }
           break;
      case PRECISION_NONE:
      default:
           break;
    }
  }

  if(info->transform_buffer)
    cms_transform_buffer( info->transform_buffer, tile_buf, tile_buf, info->w,
                          tag_precision(gimage_tag(info->gdisp->gimage)) );

  return tile_buf;
}

static guchar*
render_image_tile_fault (RenderInfo *info)
{
  guchar *data;
  guchar *dest;
  guchar *scale;
  int width;
  int x_portion;
  int y_portion;
  int portion_width;
  int step;
  int x;
  int src_bpp;
  int bps = info->src_bpp/info->src_num_channels;
  int dest_sample_width = info->w*info->src_num_channels;
  Tag tag = canvas_tag (info->src_canvas);
  int has_alpha = tag_alpha (tag) == ALPHA_YES ? 1:0;
  int i;
  float   *f;
  guint16 *u16;
  guint8  *u8;
  float    sb, db;
  ShortsFloat u;

  float offset = pow(2.0,info->gdisp->offset )-1;
  float gamma = 1.0/info->gdisp->gamma;
  float expose = pow(2.0,info->gdisp->expose);

  /* the first portion x and y */
  x_portion = info->src_x;
  y_portion = info->src_y;
  
  /* fault in the first portion */ 
  canvas_portion_refro (info->src_canvas, x_portion, y_portion); 
  data = canvas_portion_data (info->src_canvas, info->src_x, info->src_y);
  if (!data)
    {
      canvas_portion_unref( info->src_canvas, x_portion, y_portion); 
      return NULL;
    }
  scale = info->scale;
  step = info->scalesrc * info->src_bpp;
  dest = tile_buf;
  
  /* the first portions width */ 
  portion_width = canvas_portion_width ( info->src_canvas, 
                                         x_portion,
                                         y_portion );
  x = info->src_x;
  width = info->w;
  src_bpp = info->src_bpp;

  while (width--)
    {
      /* RGB unscaled optimisation */
      if(!(info->gdisp->expose != EXPOSE_DEFAULT ||
           info->gdisp->offset != OFFSET_DEFAULT ||
           info->gdisp->gamma != GAMMA_DEFAULT) &&
       	   info->scalesrc == 1 && info->scaledest == 1)
      {
        memcpy(dest,data, src_bpp * portion_width);
        dest += src_bpp * portion_width;
        data += step * portion_width;
        x += portion_width;
        /*scale += portion_width;*/
      } else
        for (i = 0; i < info->src_bpp; i++)
          *dest++ = data[i];

      if (*scale++ != 0)
	{
          /* RGB unscaled optimisation */
          if((info->gdisp->expose != EXPOSE_DEFAULT ||
              info->gdisp->offset != OFFSET_DEFAULT ||
              info->gdisp->gamma != GAMMA_DEFAULT) ||
             (info->scalesrc != 1 || info->scaledest != 1))
          {
            x += info->scalesrc;
            data += step;
          }

	  if (x >= x_portion + portion_width)
	    {
	      canvas_portion_unref (info->src_canvas, x_portion, y_portion);
              if (x >= CAST(int) info->src_width)
              {
                f = (float*)tile_buf;
                render_image_tile_fault_expose (info,
                                tile_buf, f, dest_sample_width,
                                offset, expose, gamma,
                                has_alpha, tag_precision(tag));
                   
                return tile_buf;
              }
	      x_portion += portion_width;
              canvas_portion_refro (info->src_canvas, x_portion, y_portion ); 
              data = canvas_portion_data (info->src_canvas, x_portion, y_portion );
              if(!data)
                {
                  canvas_portion_unref (info->src_canvas, x_portion, y_portion ); 
                  return NULL;
                }
	      portion_width = canvas_portion_width ( info->src_canvas, 
                                              x_portion,
                                              y_portion );
            }
        }
    }

  f = (float*)tile_buf;
  render_image_tile_fault_expose (info,
                                tile_buf, f, dest_sample_width,
                                offset, expose, gamma,
                                has_alpha, tag_precision(tag));

  canvas_portion_unref (info->src_canvas, x_portion, y_portion);
  return tile_buf;
}
#endif

/*************************/
/*  16 Bit channel data functions */
/*************************/
#define U16_TO_U8(x)  ((x)>>8)

static void
render_image_gray_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      val = U16_TO_U8(src[GRAY_PIX]);
	      src += 1;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[U16_TO_U8(src[ALPHA_G_PIX])];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | U16_TO_U8(src[GRAY_PIX]))];
	      else
		val = blend_light_check[(a | U16_TO_U8(src[GRAY_PIX]))];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      dest[0] = U16_TO_U8(src[0]);
	      dest[1] = U16_TO_U8(src[1]);
	      dest[2] = U16_TO_U8(src[2]);

	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_u16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (guint16 *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[U16_TO_U8(src[ALPHA_PIX])];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | U16_TO_U8(src[RED_PIX]))];
		  g = blend_dark_check[(a | U16_TO_U8(src[GREEN_PIX]))];
		  b = blend_dark_check[(a | U16_TO_U8(src[BLUE_PIX]))];
		}
	      else
		{
		  r = blend_light_check[(a | U16_TO_U8(src[RED_PIX]))];
		  g = blend_light_check[(a | U16_TO_U8(src[GREEN_PIX]))];
		  b = blend_light_check[(a | U16_TO_U8(src[BLUE_PIX]))];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

/*************************/
/*  Float channel data functions */
/*************************/

static inline guchar
float_to_8bit (float x)
{
    return (guchar) (255 * (x > 1.0 ? 1.0 : (x < 0.0) ? 0.0 : x) + 0.5f);
}

static void
render_image_gray_float (RenderInfo *info)
{
  gfloat *src;
  gint src8bit;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = float_to_8bit( src[GRAY_PIX] );
	      src += 1;

	      dest[0] = src8bit;
	      dest[1] = src8bit;
	      dest[2] = src8bit;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[ float_to_8bit(src[ALPHA_G_PIX]) ];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | float_to_8bit(src[GRAY_PIX]))];
	      else
		val = blend_light_check[(a | float_to_8bit(src[GRAY_PIX]))];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		  dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

void
render_image_rgb_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(gfloat*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
#if 0
	      dest[0] = (guint8) (src[RED_PIX] * 255);
              dest[1] = (guint8) (src[GREEN_PIX] * 255);
              dest[2] = (guint8) (src[BLUE_PIX] * 255);
#else
              if ((src[RED_PIX] > 1.0) || (src[GREEN_PIX] > 1.0) || (src[BLUE_PIX] > 1.0))
              {
		dest[0] = gamut_alarm_red;
                dest[1] = gamut_alarm_green;
                dest[2] = gamut_alarm_blue;
              }
              else
	      {
	        dest[0] = float_to_8bit(src[0]);
	        dest[1] = float_to_8bit(src[1]);
	        dest[2] = float_to_8bit(src[2]);
              }
#endif 
	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}


static void
render_image_rgb_a_float (RenderInfo *info)
{
  gfloat *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (gfloat *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[float_to_8bit(src[ALPHA_PIX])];
              if ((src[RED_PIX] > 1.0) || (src[GREEN_PIX] > 1.0) || (src[BLUE_PIX] > 1.0))
              {
		r = gamut_alarm_red;
                g = gamut_alarm_green;
                b = gamut_alarm_blue;
              }
              else
	      {
	        if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | float_to_8bit(src[RED_PIX]))];
		  g = blend_dark_check[(a | float_to_8bit(src[GREEN_PIX]))];
		  b = blend_dark_check[(a | float_to_8bit(src[BLUE_PIX]))];
		}
	        else
		{
		  r = blend_light_check[(a | float_to_8bit(src[RED_PIX]))];
		  g = blend_light_check[(a | float_to_8bit(src[GREEN_PIX]))];
		  b = blend_light_check[(a | float_to_8bit(src[BLUE_PIX]))];
		}
              }
	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

/*************************/
/* 16bit float channel data functions */
/*************************/

static void
render_image_gray_float16 (RenderInfo *info)
{
  guint16 *src;
  gint src8bit;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = LUT_u8_from_float16[src[GRAY_PIX]] ;
	      src += 1;

              if (src[GRAY_PIX] > ONE_FLOAT16)
              {
		dest[RED_PIX] = gamut_alarm_red;
                dest[GREEN_PIX] = gamut_alarm_green;
                dest[BLUE_PIX] = gamut_alarm_blue;
              }
              else
	      {
	        dest[0] = src8bit;
	        dest[1] = src8bit;
	        dest[2] = src8bit;
              }
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_float16 (RenderInfo *info)
{
  guint8 src8bit;
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      src8bit = LUT_u8_from_float16[src[GRAY_PIX]]; 
	      a = alpha[ LUT_u8a_from_float16 [src[ALPHA_G_PIX]]];
	      if (dark_light & 0x1)
		val = blend_dark_check[(a | src8bit)];
	      else
		val = blend_light_check[(a | src8bit)];
	      src += 2;

              if (src[GRAY_PIX] > ONE_FLOAT16)
              {
		dest[RED_PIX] = gamut_alarm_red;
                dest[GREEN_PIX] = gamut_alarm_green;
                dest[BLUE_PIX] = gamut_alarm_blue;
              }
              else
	      {
     	        dest[0] = val;
	        dest[1] = val;
	        dest[2] = val;
              }
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_float16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
#if 0
	      ShortsFloat u; 
	      dest[0] = (guint8) (FLT (src[RED_PIX], u) * 255);
              dest[1] = (guint8) (FLT (src[GREEN_PIX], u) * 255);
              dest[2] = (guint8) (FLT (src[BLUE_PIX], u) * 255);
#else
              if ((src[RED_PIX] > ONE_FLOAT16) || (src[GREEN_PIX] > ONE_FLOAT16) || (src[BLUE_PIX] > ONE_FLOAT16))
              {
		dest[0] = gamut_alarm_red;
                dest[1] = gamut_alarm_green;
                dest[2] = gamut_alarm_blue;
              }
              else
	      {
	        dest[0] = LUT_u8_from_float16 [src[RED_PIX]];
	        dest[1] = LUT_u8_from_float16 [src[GREEN_PIX]];
	        dest[2] = LUT_u8_from_float16 [src[BLUE_PIX]];
              }
#endif
	      src += 3;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_float16 (RenderInfo *info)
{
  guint16 *src;
  guchar *dest, *last;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (guint16 *)info->src;
	  dest = info->dest;
          last = dest + (info->w * 3);

	  dark_light = (y >> check_shift) + (info->x >> check_shift);
          x = info->x;
          while (dest != last) 
	    {
              // NB: This assumes that RED_PIX = 0, GREEN_PIX = 1, BLUE_PIX = 2, ALPHA_PIX = 3
              if ((src[RED_PIX] > ONE_FLOAT16) || (src[GREEN_PIX] > ONE_FLOAT16) || (src[BLUE_PIX] > ONE_FLOAT16))
              {
		r = gamut_alarm_red;
                g = gamut_alarm_green;
                b = gamut_alarm_blue;
              }
              else
	      {
	        r = LUT_u8_from_float16 [*src];
                ++src;
    	        g = LUT_u8_from_float16 [*src];
                ++src;
	        b = LUT_u8_from_float16 [*src];
                ++src;
	        a = alpha[ LUT_u8a_from_float16 [*src] ];
                ++src;
              }
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[a | r];
		  g = blend_dark_check[a | g];
		  b = blend_dark_check[a | b];
		}
	      else
		{
		  r = blend_light_check[a | r];
		  g = blend_light_check[a | g];
		  b = blend_light_check[a | b];
		}


	      *dest = r;
              ++dest;
	      *dest = g;
              ++dest;
	      *dest = b; 
              ++dest;

              ++x;
	      if (((x + 1) & check_mod) == 0)
		dark_light++;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}


/*************************/
/* 16bit BFP channel data functions */
/*************************/

static void
render_image_gray_bfp (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  gulong val;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*)info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
	    {
              /* check for over-white*/
              if (src[GRAY_PIX] > ONE_BFP)
              {
		dest[RED_PIX] =   gamut_alarm_red;
                dest[GREEN_PIX] = gamut_alarm_green;
                dest[BLUE_PIX] =  gamut_alarm_blue;
              }
              else
	      {
	        val = BFP_TO_U8(src[GRAY_PIX]);
	        src += 1;

	        dest[RED_PIX] = val;
	        dest[GREEN_PIX] = val;
	        dest[BLUE_PIX] = val;
	        dest += 3;
              }
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_gray_a_bfp (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong val;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[BFP_TO_U8(src[ALPHA_G_PIX])];
              /* check for over-white*/
              if (src[GRAY_PIX] > ONE_BFP)
              {
  	        if (dark_light & 0x1)
		{  
		  dest[RED_PIX] = blend_dark_check[(a | gamut_alarm_red)];
                  dest[GREEN_PIX] = blend_dark_check[(a | gamut_alarm_green)];
                  dest[BLUE_PIX] = blend_dark_check[(a | gamut_alarm_blue)];
                }
                else 
                {
		  dest[RED_PIX] = blend_light_check[(a | gamut_alarm_red)];
                  dest[GREEN_PIX] = blend_light_check[(a | gamut_alarm_green)];
                  dest[BLUE_PIX] = blend_light_check[(a | gamut_alarm_blue)];       
                }
              }

              else 
              {
	        if (dark_light & 0x1)
		  val = blend_dark_check[(a | BFP_TO_U8(src[GRAY_PIX]))];
	        else
		  val = blend_light_check[(a | BFP_TO_U8(src[GRAY_PIX]))];
	        src += 2;

	        dest[0] = val;
	        dest[1] = val;
	        dest[2] = val;
	        dest += 3;
              }

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_bfp (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src =(guint16*) info->src;
	  dest = info->dest;

	  for (x = info->x; x < xe; x++)
          {
            /* check for over-white */
            if ((src[RED_PIX] > ONE_BFP) || 
                (src[GREEN_PIX] > ONE_BFP) || 
                (src[BLUE_PIX] > ONE_BFP))
	    {
	      dest[RED_PIX] = gamut_alarm_red;
	      dest[GREEN_PIX] = gamut_alarm_green;
	      dest[BLUE_PIX] = gamut_alarm_blue;
	    }
	    else 
	    {
	      dest[RED_PIX] = BFP_TO_U8(src[RED_PIX]);
	      dest[GREEN_PIX] = BFP_TO_U8(src[GREEN_PIX]);
	      dest[BLUE_PIX] = BFP_TO_U8(src[BLUE_PIX]);
	    }

	  src += 3;
	  dest += 3;
	  }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}

static void
render_image_rgb_a_bfp (RenderInfo *info)
{
  guint16 *src;
  guchar *dest;
  guint *alpha;
  gulong r, g, b;
  guint a;
  int dark_light;
  int y, ye;
  int x, xe;
  int initial;
  float error;
  float step;

  alpha = info->alpha;

  y = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  step = info->scalesrc;

  error = y * step;
  error -= ((int)error) - step;

  initial = TRUE;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      if (!initial && (error < 1.0) && (y & check_mod))
	memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
      else
	{
	  src = (guint16 *)info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  for (x = info->x; x < xe; x++)
          {
  	      /* check for over-white */
              if ((src[RED_PIX] > ONE_BFP) || (src[GREEN_PIX] > ONE_BFP) || (src[BLUE_PIX] > ONE_BFP)) 
              {
                r = gamut_alarm_red;
                g = gamut_alarm_green;
                b = gamut_alarm_blue;
              }
              else
	      {
	        r = BFP_TO_U8(src[RED_PIX]);
	        g = BFP_TO_U8(src[GREEN_PIX]);
	        b = BFP_TO_U8(src[BLUE_PIX]);
              }

	      a = alpha[BFP_TO_U8(src[ALPHA_PIX])];
	      if (dark_light & 0x1)
		{
		  r = blend_dark_check[(a | r)];
		  g = blend_dark_check[(a | g)];
		  b = blend_dark_check[(a | b)];
		}
	      else
		{
		  r = blend_light_check[(a | r)];
		  g = blend_light_check[(a | g)];
		  b = blend_light_check[(a | b)];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (((y + 1) % info->scaledest) == 0)
	{
	  info->src_y += info->scalesrc;
	  info->src = render_image_tile_fault (info);
	}

      error += step;
    }
}
