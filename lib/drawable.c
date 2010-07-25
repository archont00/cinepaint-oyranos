/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#include "plugin_main.h"
#include "../lib/wire/libtile.h"

TileDrawable*
gimp_drawable_get (gint32 drawable_ID)
{
  TileDrawable *drawable;

  drawable = g_new (TileDrawable, 1);
  drawable->id = drawable_ID;
  drawable->width = gimp_drawable_width (drawable_ID);
  drawable->height = gimp_drawable_height (drawable_ID);
  drawable->bpp = gimp_drawable_bpp (drawable_ID);
  drawable->num_channels = gimp_drawable_num_channels (drawable_ID);
  drawable->ntile_rows = (drawable->height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  drawable->ntile_cols = (drawable->width + TILE_WIDTH - 1) / TILE_WIDTH;
  drawable->tiles = NULL;
  drawable->shadow_tiles = NULL;

  return drawable;
}

void
gimp_drawable_detach (TileDrawable *drawable)
{	if(!drawable)
	{	return;
	}
	gimp_drawable_flush (drawable);
	if (drawable->tiles)
	{	int ntiles = drawable->ntile_rows * drawable->ntile_cols;
		lib_tile_cache_purge(drawable->tiles,ntiles);
#ifdef _DEBUG
		memset(drawable->tiles,0,ntiles*sizeof(GTile));
#endif
		g_free (drawable->tiles);
		drawable->tiles=0;
	}	   
	if (drawable->shadow_tiles)
	{	int ntiles = drawable->ntile_rows * drawable->ntile_cols;
		lib_tile_cache_purge(drawable->shadow_tiles,ntiles);
#ifdef _DEBUG
		memset(drawable->shadow_tiles,0,ntiles*sizeof(GTile));
#endif
		g_free (drawable->shadow_tiles);
		drawable->shadow_tiles=0;
	}
	g_free (drawable);
	drawable=0;
}

void
gimp_drawable_flush (TileDrawable *drawable)
{
  GTile *tiles;
  int ntiles;
  int i;

  if (drawable)
    {
      if (drawable->tiles)
	{
	  tiles = drawable->tiles;
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;

	  for (i = 0; i < ntiles; i++)
	    if ((tiles[i].ref_count > 0) && tiles[i].dirty)
	      lib_tile_flush (&tiles[i]);
	}

      if (drawable->shadow_tiles)
	{
	  tiles = drawable->shadow_tiles;
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;

	  for (i = 0; i < ntiles; i++)
	    if ((tiles[i].ref_count > 0) && tiles[i].dirty)
	      lib_tile_flush (&tiles[i]);
	}
    }
}

void
gimp_drawable_delete (TileDrawable *drawable)
{
  if (drawable)
    {
      if (gimp_drawable_layer (drawable->id))
	gimp_layer_delete (drawable->id);
      else
	gimp_channel_delete (drawable->id);
    }
}

void
gimp_drawable_update (gint32 drawable_ID,
		      gint   x,
		      gint   y,
		      guint  width,
		      guint  height)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_update",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, x,
				    PARAM_INT32, y,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_merge_shadow (gint32 drawable_ID,
			    gint   undoable)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_merge_shadow",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, undoable,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_drawable_image_id (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 image_ID;

  return_vals = gimp_run_procedure ("gimp_drawable_image",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  image_ID = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

char*
gimp_drawable_name (gint32 drawable_ID)
{
  if (gimp_drawable_layer (drawable_ID))
    return gimp_layer_get_name (drawable_ID);
  return gimp_channel_get_name (drawable_ID);
}

guint
gimp_drawable_width (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  guint width;

  return_vals = gimp_run_procedure ("gimp_drawable_width",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  width = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    width = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return width;
}

guint
gimp_drawable_height (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  guint height;

  return_vals = gimp_run_procedure ("gimp_drawable_height",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  height = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    height = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return height;
}

guint
gimp_drawable_bpp (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  guint bpp;

  return_vals = gimp_run_procedure ("gimp_drawable_bytes",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  bpp = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    bpp = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return bpp;
}

guint
gimp_drawable_num_channels (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  guint num_channels;

  return_vals = gimp_run_procedure ("gimp_drawable_num_channels",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  num_channels = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    num_channels = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return num_channels; 
}


GPrecisionType
gimp_drawable_precision (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int precision;

  return_vals = gimp_run_procedure ("gimp_drawable_precision",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  precision = -1; 
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    precision = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return (GPrecisionType)precision; 
}

GDrawableType
gimp_drawable_type (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_type",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_visible (gint32 drawable_ID)
{
  if (gimp_drawable_layer (drawable_ID))
    return gimp_layer_get_visible (drawable_ID);
  return gimp_channel_get_visible (drawable_ID);
}

gint
gimp_drawable_channel (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_channel",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_color (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_color",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_gray (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_gray",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_has_alpha (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_has_alpha",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_indexed (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_indexed",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_layer (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_layer",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_layer_mask (gint32 drawable_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_layer_mask",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint
gimp_drawable_mask_bounds (gint32  drawable_ID,
			   gint   *x1,
			   gint   *y1,
			   gint   *x2,
			   gint   *y2)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_drawable_mask_bounds",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = return_vals[1].data.d_int32;
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

void
gimp_drawable_offsets (gint32  drawable_ID,
		       gint   *offset_x,
		       gint   *offset_y)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_offsets",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *offset_x = return_vals[1].data.d_int32;
      *offset_y = return_vals[2].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_fill (gint32 drawable_ID,
		    gint   fill_type)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_fill",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, fill_type,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_set_name (gint32  drawable_ID,
			char   *name)
{
  if (gimp_drawable_layer (drawable_ID))
    gimp_layer_set_name (drawable_ID, name);
  else
    gimp_channel_set_name (drawable_ID, name);
}

void
gimp_drawable_set_visible (gint32 drawable_ID,
			   gint   visible)
{
  if (gimp_drawable_layer (drawable_ID))
    gimp_layer_set_visible (drawable_ID, visible);
  else
    gimp_channel_set_visible (drawable_ID, visible);
}

GTile*
gimp_drawable_get_tile (TileDrawable *drawable,
			gint      shadow,
			gint      row,
			gint      col)
{
  GTile *tiles;
  guint right_tile;
  guint bottom_tile;
  int ntiles;
  int tile_num;
  unsigned i;
  unsigned j;
  unsigned k;
  tiles=0;
  if (drawable)
    {
      if (shadow)
	tiles = drawable->shadow_tiles;
      else
	tiles = drawable->tiles;

      if (!tiles)
	{
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;
	  tiles = g_new (GTile, ntiles);
		if(!tiles)
		{	return NULL;
		}
		memset(tiles,0,ntiles*sizeof(GTile));
	  right_tile = drawable->width - ((drawable->ntile_cols - 1) * TILE_WIDTH);
	  bottom_tile = drawable->height - ((drawable->ntile_rows - 1) * TILE_HEIGHT);

	  for (i = 0, k = 0; i < drawable->ntile_rows; i++)
	    {
	      for (j = 0; j < drawable->ntile_cols; j++, k++)
		{
		  tiles[k].bpp = drawable->bpp;
		  tiles[k].tile_num = k;
		  tiles[k].ref_count = 0;
		  tiles[k].dirty = FALSE;
		  tiles[k].shadow = shadow;
		  tiles[k].data = NULL;
		  tiles[k].drawable = drawable;
#ifdef _DEBUG
		tiles[k].is_allocated = 1;
#endif
		  if (j == (drawable->ntile_cols - 1))
		    tiles[k].ewidth = right_tile;
		  else
		    tiles[k].ewidth = TILE_WIDTH;

		  if (i == (drawable->ntile_rows - 1))
		    tiles[k].eheight = bottom_tile;
		  else
		    tiles[k].eheight = TILE_HEIGHT;
		}
	    }

	  if (shadow)
	    drawable->shadow_tiles = tiles;
	  else
	    drawable->tiles = tiles;
	}

      tile_num = row * drawable->ntile_cols + col;
      return &tiles[tile_num];
    }

  return NULL;
}

GTile*
gimp_drawable_get_tile2 (TileDrawable *drawable,
			 gint      shadow,
			 gint      x,
			 gint      y)
{
  gint row, col;

  col = x / TILE_WIDTH;
  row = y / TILE_HEIGHT;

  return gimp_drawable_get_tile (drawable, shadow, row, col);
}
