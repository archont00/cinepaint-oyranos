/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpconvert_pdb.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* NOTE: This file is autogenerated by pdbgen.pl */

#include "plugin_main.h"
#include "convert_pdb.h"

/**
 * gimp_convert_rgb:
 * @image_ID: The image.
 *
 * Convert specified image to RGB color
 *
 * This procedure converts the specified image to RGB color. This
 * process requires an image of type GRAY or INDEXED. No image content
 * is lost in this process aside from the colormap for an indexed
 * image.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_convert_rgb (gint32 image_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_convert_rgb",
				    &nreturn_vals,
				    GIMP_PDB_IMAGE, image_ID,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_convert_grayscale:
 * @image_ID: The image.
 *
 * Convert specified image to grayscale (256 intensity levels)
 *
 * This procedure converts the specified image to grayscale with 8 bits
 * per pixel (256 intensity levels). This process requires an image of
 * type RGB or INDEXED.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_convert_grayscale (gint32 image_ID)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_convert_grayscale",
				    &nreturn_vals,
				    GIMP_PDB_IMAGE, image_ID,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_convert_indexed:
 * @image_ID: The image.
 * @dither_type: dither type (0=none, 1=fs, 2=fs/low-bleed 3=fixed).
 * @palette_type: The type of palette to use.
 * @num_cols: the number of colors to quantize to, ignored unless (palette_type == MAKE_PALETTE).
 * @alpha_dither: dither transparency to fake partial opacity.
 * @remove_unused: remove unused or duplicate colour entries from final palette, ignored if (palette_type == MAKE_PALETTE).
 * @palette: The name of the custom palette to use, ignored unless (palette_type == CUSTOM_PALETTE).
 *
 * Convert specified image to and Indexed image
 *
 * This procedure converts the specified image to 'indexed' color. This
 * process requires an image of type GRAY or RGB. The 'palette_type'
 * specifies what kind of palette to use, A type of '0' means to use an
 * optimal palette of 'num_cols' generated from the colors in the
 * image. A type of '1' means to re-use the previous palette (not
 * currently implemented). A type of '2' means to use the so-called
 * WWW-optimized palette. Type '3' means to use only black and white
 * colors. A type of '4' means to use a palette from the gimp palettes
 * directories. The 'dither type' specifies what kind of dithering to
 * use. '0' means no dithering, '1' means standard Floyd-Steinberg
 * error diffusion, '2' means Floyd-Steinberg error diffusion with
 * reduced bleeding, '3' means dithering based on pixel location
 * ('Fixed' dithering).
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_convert_indexed (gint32                  image_ID,
		      GimpConvertDitherType   dither_type,
		      GimpConvertPaletteType  palette_type,
		      gint                    num_cols,
		      gboolean                alpha_dither,
		      gboolean                remove_unused,
		      gchar                  *palette)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_convert_indexed",
				    &nreturn_vals,
				    GIMP_PDB_IMAGE, image_ID,
				    GIMP_PDB_INT32, dither_type,
				    GIMP_PDB_INT32, palette_type,
				    GIMP_PDB_INT32, num_cols,
				    GIMP_PDB_INT32, alpha_dither,
				    GIMP_PDB_INT32, remove_unused,
				    GIMP_PDB_STRING, palette,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}
