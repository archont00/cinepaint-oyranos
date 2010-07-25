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
#include <math.h>
#include "../appenv.h"
#include "../app_procs.h"
#include "../brush.h"
#include "../colormaps.h"
#include "displaylut.h"
#include "../errors.h"
#include "../general.h"
#include "../rc.h"
#include "../gradient.h"
#include "../paint_funcs_area.h"
#include "../palette.h"
#include "../patterns.h"
#include "../pixelrow.h"
#include "../plug_in.h"
#include "../tile_swap.h"


GdkVisual *g_visual = NULL;
GdkColormap *g_cmap = NULL;

gulong g_black_pixel;
gulong g_gray_pixel;
gulong g_white_pixel;
gulong g_color_pixel;
gulong g_normal_guide_pixel;
gulong g_active_guide_pixel;

gulong foreground_pixel;
gulong background_pixel;

gulong old_color_pixel;
gulong new_color_pixel;

gulong marching_ants_pixels[8];

GtkDitherInfo *red_ordered_dither;
GtkDitherInfo *green_ordered_dither;
GtkDitherInfo *blue_ordered_dither;
GtkDitherInfo *gray_ordered_dither;

guchar ***ordered_dither_matrix;

/*  These arrays are calculated for quick 24 bit to 16 color conversions  */
gulong *g_lookup_red;
gulong *g_lookup_green;
gulong *g_lookup_blue;

gulong *color_pixel_vals;
gulong *gray_pixel_vals;

static void make_color (gulong *pixel_ptr,
			int     red,
			int     green,
			int     blue,
			int     readwrite);

static void
set_app_colors (void)
{
  cycled_marching_ants = FALSE;

  make_color (&g_black_pixel, 0, 0, 0, FALSE);
  make_color (&g_gray_pixel, 127, 127, 127, FALSE);
  make_color (&g_white_pixel, 255, 255, 255, FALSE);
  make_color (&g_color_pixel, 255, 255, 0, FALSE);
  make_color (&g_normal_guide_pixel, 0, 127, 255, FALSE);
  make_color (&g_active_guide_pixel, 255, 0, 0, FALSE);

  {
    PixelRow col;
    guchar d[TAG_MAX_BYTES];

    pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO), d, 1);

    palette_get_black (&col);
    store_color (&foreground_pixel, &col);
    store_color (&old_color_pixel, &col);

    palette_get_white (&col);
    store_color (&background_pixel, &col);
    store_color (&new_color_pixel, &col);
  }
}

/* poof can't use this anymore
static unsigned int
gamma_correct (int intensity, double gamma)
{
  unsigned int val;
  double ind;
  double one_over_gamma;

  if (gamma != 0.0)
    one_over_gamma = 1.0 / gamma;
  else
    one_over_gamma = 1.0;

  ind = (double) intensity / 256.0;
  val = (int) (256 * pow (ind, one_over_gamma));

  return val;
}
*/

/*************************************************************************/


gulong
get_color (PixelRow * col)
{
  PixelRow r;
  guchar d[TAG_MAX_BYTES];

  pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), d, 1);
  copy_row (col, &r);
  
  return gdk_rgb_xpixel_from_rgb ((d[0] << 16) | (d[1] << 8) | d[2]);
}


static void
make_color (gulong *pixel_ptr,
	    int     red,
            int     green,
            int     blue,
	    int     readwrite)
{
  *pixel_ptr = gdk_rgb_xpixel_from_rgb ((red << 16) | (green << 8) | blue);
}

void
store_color (gulong *pixel_ptr,
	     PixelRow * col)
{
  *pixel_ptr = get_color (col);
}

void
store_display_color (gulong *pixel_ptr,
	     PixelRow * col)
{
  guint8 d[3];
  switch (tag_precision ( pixelrow_tag (col)))
  {
    case PRECISION_FLOAT:
      {
	gfloat * float_data = (gfloat *) pixelrow_data (col);
	d[0] = display_u8_from_float(float_data[0]); 
	d[1] = display_u8_from_float(float_data[1]); 
	d[2] = display_u8_from_float(float_data[2]); 
      }
      break;
    case PRECISION_U8:
    case PRECISION_U16:
    case PRECISION_FLOAT16:
    case PRECISION_NONE:
    default:
      g_warning ("store_display_color: bad precision\n");
      break;
  }
  make_color (pixel_ptr, d[0], d[1], d[2], TRUE);
}

void
cp_pixel_to_gdkcolor(GdkColor *c, const gulong *pixel)
{
  const char *cc = (const char*)pixel;

  c->pixel = *pixel;
  c->red = cc[1]*255;
  c->green = cc[2]*255;
  c->blue = cc[3]*255;
}

void
get_standard_colormaps ()
{
  GtkPreviewInfo *info;

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  info = gtk_preview_get_info ();
  g_visual = gtk_preview_get_visual (); //info->visual;

  g_cmap = gtk_preview_get_cmap(); //info->cmap;

  set_app_colors ();
}
