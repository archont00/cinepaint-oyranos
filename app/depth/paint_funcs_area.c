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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../boundary.h"
#include "float16.h"
#include "../rc.h"
#include "../paint_funcs_area.h"
#include "paint_funcs_row_u8.h"
#include "paint_funcs_row_u16.h"
#include "paint_funcs_row_float.h"
#include "paint_funcs_row_float16.h"
#include "paint_funcs_row_bfp.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../tag.h"

#define EPSILON            0.0001

/* only for scale_area et al. */
typedef enum
{
  MinifyX_MinifyY,
  MinifyX_MagnifyY,
  MagnifyX_MinifyY,
  MagnifyX_MagnifyY
} ScaleType;


/*  Layer modes information.  only the affect_alpha is used */
typedef struct LayerMode LayerMode;
struct LayerMode
{
  int affect_alpha;   /*  does the layer mode affect the alpha channel  */
  char *name;         /*  layer mode specification  */
};

LayerMode layer_modes_16[] =
{
  { 1, "Normal" },
  { 1, "Dissolve" },
  { 1, "Behind" },
  { 0, "Multiply" },
  { 0, "Screen" },
  { 0, "Overlay" },
  { 0, "Difference" },
  { 0, "Addition" },
  { 0, "Subtraction" },
  { 0, "Darken Only" },
  { 0, "Lighten Only" },
  { 0, "Hue" },
  { 0, "Saturation" },
  { 0, "Color" },
  { 0, "Value" },
  { 1, "Erase" },
  { 1, "Replace" }
};

/* convolve_area */

typedef void  (*ConvolveAreaFunc) (PixelArea*,PixelArea*,gfloat*,gint, gfloat, gint, gint, gint);
static ConvolveAreaFunc convolve_area_funcs (Tag);
static void convolve_area_u8 (PixelArea*, PixelArea*, gfloat*, gint, gfloat, gint, gint, gint);
static void convolve_area_u16 (PixelArea*, PixelArea*, gfloat*, gint, gfloat, gint, gint, gint);
static void convolve_area_float (PixelArea*, PixelArea*, gfloat*, gint, gfloat, gint, gint, gint);
static void convolve_area_float16 (PixelArea*, PixelArea*, gfloat*, gint, gfloat, gint, gint, gint);
static void convolve_area_bfp (PixelArea*, PixelArea*, gfloat*, gint, gfloat, gint, gint, gint);

/* gaussian_blur_area */
static void gaussian_blur_area_funcs (Tag);

typedef gdouble*  (*GaussianCurveFunc) (gint,gint*);
static GaussianCurveFunc gaussian_curve;
static gdouble* gaussian_curve_u8 (gint, gint*); 
static gdouble* gaussian_curve_u16 (gint, gint*); 
static gdouble* gaussian_curve_float (gint, gint*); 
static gdouble* gaussian_curve_float16 (gint, gint*); 
static gdouble* gaussian_curve_bfp (gint, gint*); 

typedef void  (*RleRowFunc) (PixelRow*,gint*,PixelRow*);
static RleRowFunc rle_row; 
static void rle_row_u8 (PixelRow*, gint*, PixelRow*);
static void rle_row_u16 (PixelRow*, gint*, PixelRow*);
static void rle_row_float (PixelRow*, gint*, PixelRow*);
static void rle_row_float16 (PixelRow*, gint*, PixelRow*);
static void rle_row_bfp (PixelRow*, gint*, PixelRow*);

typedef void  (*GaussianBlurRowFunc) (PixelRow*,PixelRow*,gint*, PixelRow*,gint,gfloat*,gfloat);
static GaussianBlurRowFunc gaussian_blur_row;
static void gaussian_blur_row_u8 (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void gaussian_blur_row_u16 (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void gaussian_blur_row_float (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void gaussian_blur_row_float16 (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 
static void gaussian_blur_row_bfp (PixelRow*, PixelRow*, gint*, PixelRow*, gint, gfloat*, gfloat); 

/* scale_area_no_resample */
static void scale_area_no_resample_funcs (Tag);

typedef void (*ScaleRowNoResampleFunc) (PixelRow*,PixelRow*,gint*);
ScaleRowNoResampleFunc scale_row_no_resample;
static void scale_row_no_resample_u8 (PixelRow*, PixelRow*, gint*);
static void scale_row_no_resample_u16 (PixelRow*, PixelRow*, gint*);
static void scale_row_no_resample_float (PixelRow*, PixelRow*, gint*);
static void scale_row_no_resample_float16 (PixelRow*, PixelRow*, gint*);
static void scale_row_no_resample_bfp (PixelRow*, PixelRow*, gint*);

/* scale_area */

typedef void (*ScaleRowResampleFunc) (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static void scale_row_resample_u8    (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static void scale_row_resample_u16   (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static void scale_row_resample_float (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static void scale_row_resample_float16 (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static void scale_row_resample_bfp (guchar*, guchar*, guchar*, guchar*, 
                                      gdouble, gdouble*, gdouble, gdouble, 
                                      gdouble*, gint, gint, gint, gint); 
static ScaleRowResampleFunc scale_row_resample_funcs (Tag);


typedef void (*ScaleSetDestRowFunc)  (PixelArea*, guint, guint, gdouble*);
static void scale_set_dest_row_u8    (PixelArea*, guint, guint, gdouble*);
static void scale_set_dest_row_u16   (PixelArea*, guint, guint, gdouble*);
static void scale_set_dest_row_float (PixelArea*, guint, guint, gdouble*);
static void scale_set_dest_row_float16 (PixelArea*, guint, guint, gdouble*);
static void scale_set_dest_row_bfp (PixelArea*, guint, guint, gdouble*);
static ScaleSetDestRowFunc scale_set_dest_funcs     (Tag);


/* thin_area */
static void thin_area_funcs (Tag); 

typedef gint (*ThinRowFunc) (PixelRow*,PixelRow*,PixelRow*,PixelRow*,gint);
static ThinRowFunc thin_row;
static gint thin_row_u8 (PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);
static gint thin_row_u16 ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);
static gint thin_row_float ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);
static gint thin_row_float16 ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);
static gint thin_row_bfp ( PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);


/* gaussian_blur_area */
static gdouble * make_curve                (gdouble, gint*, gdouble);

/* border_area */
static void      draw_segments             (PixelArea*, BoundSeg*,
                                            gint, gint, gint, gfloat );

/* scale_area */
static inline gdouble   cubic              (gdouble, gdouble, gdouble, gdouble,
                                            gdouble, gdouble, gdouble);

/* combine_areas */
static int       apply_layer_mode          (PixelRow*, PixelRow*, PixelRow*,
                                            gint, gint, gfloat, gint, gint*);
static int       apply_indexed_layer_mode  (PixelRow*, PixelRow*, PixelRow*,
                                            gint);

/* combine_areas_replace */
static void      apply_layer_mode_replace  (PixelRow*, PixelRow*, PixelRow*,
                                            PixelRow*, gfloat , gint*); 



/*  ColorHash structure  */
typedef struct ColorHash ColorHash;

struct ColorHash
{
  int pixel;           /*  R << 16 | G << 8 | B  */
  int index;           /*  colormap index        */
  int colormap_ID;     /*  colormap ID           */
};

#define MAXDIFF            195076
#define HASH_TABLE_SIZE    1021
static ColorHash color_hash_table [HASH_TABLE_SIZE];
static int color_hash_misses;
static int color_hash_hits;

/* for dissolve mode */
#define RANDOM_TABLE_SIZE  4096
#define RANDOM_SEED        314159265
static int random_table [RANDOM_TABLE_SIZE];


void 
paint_funcs_area_setup  (
                         void
                         )
{
  int i;

  /*  initialize the color hash table--invalidate all entries  */
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    color_hash_table[i].colormap_ID = -1;
  color_hash_misses = 0;
  color_hash_hits = 0;

  /*  generate a table of random seeds  */
#if 0
  srand48 (time (NULL) * RANDOM_SEED);
#endif
  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = drand48 ();

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    {
      int tmp;
      int swap = i + ((int)(drand48()*RANDOM_SEED)) % (RANDOM_TABLE_SIZE - i);
      tmp = random_table[i];
      random_table[i] = random_table[swap];
      random_table[swap] = tmp;
    }
}


void 
paint_funcs_area_free  (
                        void
                        )
{
  /*  print out the hash table statistics
      d_printf ("RGB->indexed hash table lookups: %d\n", color_hash_hits + color_hash_misses);
      d_printf ("RGB->indexed hash table hits: %d\n", color_hash_hits);
      d_printf ("RGB->indexed hash table misses: %d\n", color_hash_misses);
      d_printf ("RGB->indexed hash table hit rate: %f\n",
      100.0 * color_hash_hits / (color_hash_hits + color_hash_misses));
      */
}



/**************************************************/
/*    AREA FUNCTIONS                              */
/**************************************************/

static int
check_precision (
                 PixelArea * a1,
                 PixelArea * a2,
                 PixelArea * a3
                 )
{
  Tag t1 = pixelarea_tag (a1);

  if (a2)
    if (tag_precision (pixelarea_tag (a2)) != tag_precision (t1))
      return 1;

  if (a3)
    if (tag_precision (pixelarea_tag (a3)) != tag_precision (t1))
      return 1;

  return 0;
}
  
typedef void (*x_addFunc) (PixelRow*, PixelRow*);
static x_addFunc x_add_area_funcs (Tag);
static x_addFunc
x_add_area_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return x_add_row_u8;
    case PRECISION_U16:
      return x_add_row_u16;
    case PRECISION_FLOAT:
      return x_add_row_float;
    case PRECISION_FLOAT16:
      return x_add_row_float16;
    case PRECISION_BFP:
      return x_add_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("x_add: bad precision");
    }
  return NULL;
}

void
x_add_area (
             PixelArea * src_area,
             PixelArea * dest_area 
             )
{
  x_addFunc x_add_row = x_add_area_funcs (pixelarea_tag (src_area));
  void * pag;
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow r1;
      PixelRow r2;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &r1, h);
          pixelarea_getdata (dest_area, &r2, h);
          (*x_add_row) (&r1, &r2);
        }
    }
}

typedef void (*x_subFunc) (PixelRow*, PixelRow*);
static x_subFunc x_sub_area_funcs (Tag);
static x_subFunc
x_sub_area_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return x_sub_row_u8;
    case PRECISION_U16:
      return x_sub_row_u16;
    case PRECISION_FLOAT:
      return x_sub_row_float;
    case PRECISION_FLOAT16:
      return x_sub_row_float16;
    case PRECISION_BFP:
      return x_sub_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("x_sub: bad precision");
    }
  return NULL;
}

void
x_sub_area (
             PixelArea * src_area,
             PixelArea * dest_area 
             )
{
  x_subFunc x_sub_row = x_sub_area_funcs (pixelarea_tag (src_area));
  void * pag;
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow r1;
      PixelRow r2;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &r1, h);
          pixelarea_getdata (dest_area, &r2, h);
          (*x_sub_row) (&r1, &r2);
        }
    }
}

typedef void (*x_minFunc) (PixelRow*, PixelRow*);
static x_minFunc x_min_area_funcs (Tag);
static x_minFunc
x_min_area_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return x_min_row_u8;
    case PRECISION_U16:
      return x_min_row_u16;
    case PRECISION_FLOAT:
      return x_min_row_float;
    case PRECISION_FLOAT16:
      return x_min_row_float16;
    case PRECISION_BFP:
      return x_min_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("x_min: bad precision");
    }
  return NULL;
}

void
x_min_area (
             PixelArea * src_area,
             PixelArea * dest_area 
             )
{
  x_minFunc x_min_row = x_min_area_funcs (pixelarea_tag (src_area));
  void * pag;
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow r1;
      PixelRow r2;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &r1, h);
          pixelarea_getdata (dest_area, &r2, h);
          (*x_min_row) (&r1, &r2);
        }
    }
}

typedef void (*invertFunc) (PixelRow*, PixelRow*);
static invertFunc invert_area_funcs (Tag);

static invertFunc
invert_area_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return invert_row_u8;
    case PRECISION_U16:
      return invert_row_u16;
    case PRECISION_FLOAT:
      return invert_row_float;
    case PRECISION_FLOAT16:
      return invert_row_float16;
    case PRECISION_BFP:
      return invert_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("invert: bad precision");
    }
  return NULL;
}

void
invert_area (
             PixelArea * src_area,
             PixelArea * dest_area 
             )
{
  invertFunc invert_row = invert_area_funcs (pixelarea_tag (src_area));
  void * pag;
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow r1;
      PixelRow r2;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &r1, h);
          pixelarea_getdata (dest_area, &r2, h);
          (*invert_row) (&r1, &r2);
        }
    }
}

/* ---------------------------------------------------------*/

typedef void (*AbsDiffFunc) (PixelRow*, PixelRow*, PixelRow*, gfloat, int);
static AbsDiffFunc absdiff_area_funcs (Tag);


static AbsDiffFunc 
absdiff_area_funcs (
                    Tag tag
                    )
	  
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return absdiff_row_u8;
    case PRECISION_U16:
      return absdiff_row_u16;
    case PRECISION_FLOAT:
      return absdiff_row_float;
    case PRECISION_FLOAT16:
      return absdiff_row_float16;
    case PRECISION_BFP:
      return absdiff_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("qq bad precision");
    } 

  return NULL;
}
 
void
absdiff_area (
              PixelArea * image,
              PixelArea * mask,
              PixelRow * color,
              gfloat threshold,
              int antialias
              )
{
  void * pag;
  Tag tag = pixelarea_tag (image); 
  AbsDiffFunc absdiff_row = absdiff_area_funcs (tag);

  if (check_precision (image, mask, NULL))
    {
      g_warning ("aq bad precision");
      return;
    }

  for (pag = pixelarea_register (2, image, mask);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow irow;
      PixelRow mrow;
      gint h = pixelarea_height (image);
      while (h--)
        {
          pixelarea_getdata (image, &irow, h);
          pixelarea_getdata (mask, &mrow, h);
          (*absdiff_row) (&irow, &mrow, color, threshold, antialias);
        }
    }
}

/* extract */
typedef void (*ExtractChannelRowFunc) (PixelRow*, PixelRow*, gint);
static ExtractChannelRowFunc extract_channel_area_funcs (Tag);

static ExtractChannelRowFunc
extract_channel_area_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return extract_channel_row_u8;
    case PRECISION_U16:
      return extract_channel_row_u8;
    case PRECISION_FLOAT:
      return extract_channel_row_float;
    case PRECISION_FLOAT16:
      return extract_channel_row_float16;
    case PRECISION_BFP:
      return extract_channel_row_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("extract_channel_area_funcs: bad precision");
    }
  return NULL;
}

void
extract_channel_area (
		       PixelArea * src_area,
		       PixelArea * dest_area, 
		       gint channel
                      )
{
  ExtractChannelRowFunc extract_channel_row =
	 extract_channel_area_funcs (pixelarea_tag (src_area));
  void * pag;
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow r1;
      PixelRow r2;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &r1, h);
          pixelarea_getdata (dest_area, &r2, h);
          (*extract_channel_row) (&r1, &r2, channel);
        }
    }
}


/* ---------------------------------------------------------------

                          color area

   the precision specific functions assume that the color has
   an identical tag to the area/row being colored.  they take:

     void * to dest buffer
     void * to color buffer
     width and height of buffer
     pixel and row stride of buffer

*/

typedef void (*ColorRowFunc) (void *, void *, guint, guint, guint, guint);
static ColorRowFunc color_area_funcs (Tag);

static ColorRowFunc 
color_area_funcs (
                  Tag tag
                  )
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return color_row_u8;
  case PRECISION_U16:
    return color_row_u16;
  case PRECISION_FLOAT:
    return color_row_float;
  case PRECISION_FLOAT16:
    return color_row_float16;
  case PRECISION_BFP:
    return color_row_bfp;
  default:
    return NULL;
  } 
}


void
color_row (
           PixelRow * row,
           PixelRow * col
           )
{
  Tag t = pixelrow_tag (row);
  ColorRowFunc func = color_area_funcs (t);
  PixelRow color;
  guchar color_data[TAG_MAX_BYTES];

  g_return_if_fail (row != NULL);
  g_return_if_fail (col != NULL);
  g_return_if_fail (func != NULL);
  
  if (tag_equal (t, pixelrow_tag (col)) != TRUE)
    {
      pixelrow_init (&color, t, color_data, 1);
      copy_row (col, &color);
      col = &color;
    }

  func ((void *) pixelrow_data (row),
        (void *) pixelrow_data (col),
        pixelrow_width (row),
        1,
        tag_num_channels (t),
        0);
}


void 
color_area  (
             PixelArea * area,
             PixelRow * col
             )
{
  Tag t = pixelarea_tag (area);
  ColorRowFunc func = color_area_funcs (t);
  PixelRow color;
  guchar color_data[TAG_MAX_BYTES];

  g_return_if_fail (area != NULL);
  g_return_if_fail (col != NULL);
  g_return_if_fail (func != NULL);
  
  if (tag_equal (t, pixelrow_tag (col)) != TRUE)
    {
      pixelrow_init (&color, t, color_data, 1);
      copy_row (col, &color);
      col = &color;
    }

  {
    guint pixelstride = tag_num_channels (t);
    void * col_data = (void *) pixelrow_data (col);
    void * pag;
    
    for (pag = pixelarea_register (1, area);
         pag != NULL;
         pag = pixelarea_process (pag))
      {
        func ((void*) pixelarea_data (area),
              col_data,
              pixelarea_width (area),
              pixelarea_height (area),
              pixelstride,
              pixelarea_rowstride (area));
      }
  }
}




/* ---------------------------------------------------------*/

typedef void (*BlendRowFunc) (PixelRow*, PixelRow*, PixelRow*, gfloat, gint);
static BlendRowFunc blend_area_funcs (Tag);

static BlendRowFunc 
blend_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return blend_row_u8;
  case PRECISION_U16:
    return blend_row_u16;
  case PRECISION_FLOAT:
    return blend_row_float;
  case PRECISION_FLOAT16:
    return blend_row_float16;
  case PRECISION_BFP:
    return blend_row_bfp;
  default:
    return NULL;
  } 
}

void 
blend_area  (
             PixelArea * src1_area,
             PixelArea * src2_area,
             PixelArea * dest_area,
             gfloat blend,
	     gint alpha
             )
{
  void * pag;
  Tag dest_tag = pixelarea_tag (dest_area); 
  BlendRowFunc blend_row = blend_area_funcs (dest_tag); 
  /* put in tags check */
  
  for (pag = pixelarea_register (3, src1_area, src2_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src1_area);
      while (h--)
        {
          pixelarea_getdata (src1_area, &src1_row, h);
          pixelarea_getdata (src2_area, &src2_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
         /*blend_pixels (s1, s2, d, blend, src1->w, src1->bytes, src1_has_alpha); */
           (*blend_row) (&src1_row, &src2_row, &dest_row, blend, alpha);
        }
    }
}

typedef void (*ShadeRowFunc) (PixelRow*, PixelRow*, PixelRow*, gfloat);
static ShadeRowFunc shade_area_funcs (Tag);

static ShadeRowFunc 
shade_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return shade_row_u8;
  case PRECISION_U16:
    return shade_row_u16;
  case PRECISION_FLOAT:
    return shade_row_float;
  case PRECISION_FLOAT16:
    return shade_row_float16;
  case PRECISION_BFP:
    return shade_row_bfp;
  default:
    return NULL;
  } 
}

void 
shade_area  (
             PixelArea * src_area,
             PixelArea * dest_area,
             PixelRow * col,
             gfloat blend
             )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  ShadeRowFunc shade_row = shade_area_funcs (src_tag);
  /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
          (*shade_row) (&src_row, &dest_row, col, blend);
        }
    }
}



typedef void (*CopyRowFunc) (PixelRow*, PixelRow*);
static CopyRowFunc copy_area_funcs (Tag);

static CopyRowFunc 
copy_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return copy_row_u8;
  case PRECISION_U16:
    return copy_row_u16;
  case PRECISION_FLOAT:
    return copy_row_float;
  case PRECISION_FLOAT16:
    return copy_row_float16;
  case PRECISION_BFP:
  	return copy_row_bfp;
  default:
    return NULL;
  } 
}

void 
copy_row  (
           PixelRow * src_row,
           PixelRow * dest_row
           )
{
  (*(copy_area_funcs (pixelrow_tag (src_row)))) (src_row, dest_row);
}



void 
copy_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            )
{
  Tag src_tag = pixelarea_tag (src_area);
  PixelRow srow;
  PixelRow drow;
  void * pag;
  CopyRowFunc copyrow = copy_area_funcs (src_tag);

  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      int h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &srow, h);
          pixelarea_getdata (dest_area, &drow, h);
          copyrow (&srow, &drow);
        }
    }
}


typedef void (*AddAlphaRowFunc) (PixelRow*, PixelRow*);
static AddAlphaRowFunc add_alpha_area_funcs (Tag);

static AddAlphaRowFunc 
add_alpha_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return add_alpha_row_u8;
  case PRECISION_U16:
    return add_alpha_row_u16;
  case PRECISION_FLOAT:
    return add_alpha_row_float;
  case PRECISION_FLOAT16:
    return add_alpha_row_float16;
  case PRECISION_BFP:
    return add_alpha_row_bfp;
  default:
    return NULL;
  } 
}


void 
add_alpha_area  (
                 PixelArea * src_area,
                 PixelArea * dest_area
                 )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  AddAlphaRowFunc add_alpha_row = add_alpha_area_funcs (src_tag);

   /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
  	 /*add_alpha_pixels (s, d, src->w, src->bytes);*/
          (*add_alpha_row) (&src_row, &dest_row);
        }
    }
}

typedef void (*FlattenRowFunc) (PixelRow*, PixelRow*, PixelRow*);
static FlattenRowFunc flatten_area_funcs (Tag);

static FlattenRowFunc 
flatten_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return flatten_row_u8;
  case PRECISION_U16:
    return flatten_row_u16;
  case PRECISION_FLOAT:
    return flatten_row_float;
  case PRECISION_FLOAT16:
    return flatten_row_float16;
  case PRECISION_BFP:
    return flatten_row_bfp;
  default:
    return NULL;
  } 
}

/* UNUSED */
void 
flatten_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               PixelRow * bg
               )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  FlattenRowFunc flatten_row = flatten_area_funcs (src_tag);

   /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
          /*flatten_pixels (s, d, bg, src->w, src->bytes);*/
          (*flatten_row) (&src_row, &dest_row, bg);
        }
    }
}

typedef void (*ExtractAlphaRowFunc) (PixelRow*, PixelRow*, PixelRow*);
static ExtractAlphaRowFunc extract_alpha_area_funcs (Tag);

static ExtractAlphaRowFunc 
extract_alpha_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return extract_alpha_row_u8;
  case PRECISION_U16:
    return extract_alpha_row_u16;
  case PRECISION_FLOAT:
    return extract_alpha_row_float;
  case PRECISION_FLOAT16:
    return extract_alpha_row_float16;
  case PRECISION_BFP:
    return extract_alpha_row_bfp;
  default:
    return NULL;
  } 
}

void 
extract_alpha_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     PixelArea * dest_area
                     )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  ExtractAlphaRowFunc extract_alpha_row = extract_alpha_area_funcs (src_tag);

   /*put in tags check*/
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
  	 /* extract_alpha_pixels (s, m, d, src->w, src->bytes); */
          (*extract_alpha_row) (&src_row, &mask_row, &dest_row);
        }
    }
}

typedef void (*ExtractFromIntenRowFunc) (PixelRow*, PixelRow*, PixelRow*, PixelRow*, gint);
typedef void (*ExtractFromIndexedRowFunc) (PixelRow*, PixelRow*, PixelRow*, unsigned char *, PixelRow*, gint);
static ExtractFromIntenRowFunc extract_from_inten_row;
static ExtractFromIndexedRowFunc extract_from_indexed_row;
static void extract_from_area_funcs (Tag);

static void 
extract_from_area_funcs (
			Tag tag
		    )
		  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    extract_from_inten_row = extract_from_inten_row_u8; 
    extract_from_indexed_row = extract_from_indexed_row_u8; 
    break;
  case PRECISION_U16:
    extract_from_inten_row = extract_from_inten_row_u16; 
    extract_from_indexed_row = extract_from_indexed_row_u16; 
    break;
  case PRECISION_FLOAT:
    extract_from_inten_row = extract_from_inten_row_float; 
    extract_from_indexed_row = NULL; 
    break;
  case PRECISION_FLOAT16:
    extract_from_inten_row = extract_from_inten_row_float16; 
    extract_from_indexed_row = NULL; 
    break;
  case PRECISION_BFP:
    extract_from_inten_row = extract_from_inten_row_bfp; 
    extract_from_indexed_row = extract_from_indexed_row_bfp; 
    break;
  default:
    extract_from_inten_row = NULL; 
    extract_from_indexed_row = NULL; 
    break;
  } 
}

void 
extract_from_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area,
                    PixelArea * mask_area,
                    PixelRow * bg,
                    unsigned char * cmap,
                    gint cut
                    )
{
  void * pag;
  Tag src_tag = pixelarea_tag (src_area); 
  Format format = tag_format (src_tag);

  extract_from_area_funcs (src_tag);
   /*put in tags check*/
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
	  switch (format)
	    {
            case FORMAT_NONE:
              break;
	    case FORMAT_RGB:
	    case FORMAT_GRAY:
  	      /*extract_from_inten_pixels (s, d, m, bg, cut, src->w, src->bytes,src_has_alpha);*/
              (*extract_from_inten_row) (&src_row, &dest_row, &mask_row, bg, cut);
	      break;
	    case FORMAT_INDEXED:
  	      /*extract_from_indexed_pixels (s, d, m, cmap--wheres the cmap?, bg, cut, src->w, src  bytes, src_has_alpha);*/
              (*extract_from_indexed_row) (&src_row, &dest_row, &mask_row, cmap, bg, cut);
	      break;
	    }
        }
    }
}

typedef void (*MultiplyAlphaRowFunc) (PixelRow*);
static MultiplyAlphaRowFunc  multiply_alpha_area_funcs (Tag);

static MultiplyAlphaRowFunc 
multiply_alpha_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return multiply_alpha_row_u8;
  case PRECISION_U16:
    return multiply_alpha_row_u16;
  case PRECISION_FLOAT:
    return multiply_alpha_row_float;
  case PRECISION_FLOAT16:
    return multiply_alpha_row_float16;
  case PRECISION_BFP:
    return multiply_alpha_row_bfp;
  default:
    return NULL;
  } 
}


/* Convert from unmultiplied to premultiplied alpha. */ 
void 
multiply_alpha_area  (
                      PixelArea * src_area
                      )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  MultiplyAlphaRowFunc multiply_alpha_row = multiply_alpha_area_funcs (src_tag);
 
   /*put in tags check*/
  
  for (pag = pixelarea_register (1, src_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
            
          (*multiply_alpha_row) (&src_row);
        }
    }
}


typedef void (*SeparateAlphaRowFunc) (PixelRow*);
static SeparateAlphaRowFunc separate_alpha_area_funcs (Tag);

static SeparateAlphaRowFunc 
separate_alpha_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return separate_alpha_row_u8;
  case PRECISION_U16:
    return separate_alpha_row_u16;
  case PRECISION_FLOAT:
    return separate_alpha_row_float;
  case PRECISION_FLOAT16:
    return separate_alpha_row_float16;
  case PRECISION_BFP:
    return separate_alpha_row_bfp;
  default:
    return NULL;
  } 
}


/* Convert from premultiplied alpha to unpremultiplied */
void 
separate_alpha_area  (
                      PixelArea * src_area
                      )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  SeparateAlphaRowFunc separate_alpha_row = separate_alpha_area_funcs (src_tag);

   /*put in tags check*/
  
  for (pag = pixelarea_register (1, src_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
            
          (*separate_alpha_row) (&src_row);
        }
    }
}


static ConvolveAreaFunc 
convolve_area_funcs (
			Tag tag
		    )
		  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return convolve_area_u8; 
  case PRECISION_U16:
    return convolve_area_u16; 
  case PRECISION_FLOAT:
    return convolve_area_float; 
  case PRECISION_FLOAT16:
    return convolve_area_float16; 
  case PRECISION_BFP:
    return convolve_area_bfp; 
  default:
    return NULL; 
  } 
}

void 
convolve_area  (
                PixelArea * src_area,
                PixelArea * dest_area,
                gfloat * matrix,
                guint matrix_size,
                gfloat divisor,
                gint mode,
		gint alpha
                )
{
  void * pag;
  gint offset;
  guint src_area_height = pixelarea_areaheight (src_area);  
  guint src_area_width = pixelarea_areawidth (src_area);  
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  ConvolveAreaFunc convolve_area_func = convolve_area_funcs (dest_tag);

  if ( !tag_equal (src_tag, dest_tag) )
     return;

  /*  If the mode is NEGATIVE, the offset should be 128  */
  if (mode == NEGATIVE)
    {
      offset = 128;
      mode = 0;
    }
  else
    offset = 0;
  
  /*  check for the boundary cases  */
  if (src_area_width < (matrix_size - 1) || 
      src_area_height < (matrix_size - 1))
    return;

  /* the processing loop is needed so the pixelareas will be
     reffed properly, even though both buffers are flat */
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      (*convolve_area_func) (src_area, dest_area, matrix, 
                             matrix_size, divisor, mode, offset, alpha);
    }
}

static void
convolve_area_u8 (
		  PixelArea   *src_area,
		  PixelArea   *dest_area,
		  gfloat         *matrix,
		  gint          size,
		  gfloat        divisor,
		  gint          mode,
                  gint          offset,
		  gint		alpha
		  )
{
  guint8 *src, * s;
  guint8 *dest;
  gfloat * m;
  gfloat total [4];
  gint b, num_channels;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
  
  /*  Initialize some values  */
  src_width = pixelarea_areawidth (src_area);
  src_height = pixelarea_areaheight (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
  
  src_rowstride =  pixelarea_rowstride (src_area);
  dest_rowstride = pixelarea_rowstride (dest_area);

  margin = size / 2;
  src = (guint8 *) pixelarea_data (src_area);
  dest = (guint8 *) pixelarea_data (dest_area);
  
  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  /* then do the middle  scanlines*/ 
  for (y = margin; y < src_height - margin; y++)
    {
      /* first handle the left margin pixels */
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
     
      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  /* go to the beginning of the src under the kernal */
	  s = src - margin * (num_channels + src_rowstride);

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      /* go to the next line of the src under the kernal */
	      s += src_rowstride - size * num_channels;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*dest++ = 0;
	      else
		*dest++ = (total [b] > 255) ? 255 : (guint8) total [b];
	    }
	  src += num_channels;
	}
      
      /* now the right margin pixels*/
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
    }

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
convolve_area_u16 (
		   PixelArea   *src_area,
		   PixelArea   *dest_area,
		   gfloat       *matrix,
		   gint          size,
		   gfloat        divisor,
		   gint          mode,
                   gint          offset,
		   gint		alpha
		   )
{
  guint16 *src, * s;
  guint16 *dest;
  gfloat * m;
  gfloat total [4];
  gint b, num_channels;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
  
  /*  Initialize some values  */
  src_width = pixelarea_areawidth (src_area);
  src_height = pixelarea_areaheight (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
 
  src_rowstride =  pixelarea_rowstride (src_area) / sizeof (guint16);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof (guint16);

  margin = size / 2;
  src = (guint16 *) pixelarea_data (src_area);
  dest = (guint16 *) pixelarea_data (dest_area);
  
  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  /* then do the middle  scanlines*/ 
  for (y = margin; y < src_height - margin; y++)
    {
      /* first handle the left margin pixels */
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
     
      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  /* go to the beginning of the src under the kernal */
	  s = src - margin * (num_channels + src_rowstride);

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      /* go to the next line of the src under the kernal */
	      s += src_rowstride - size * num_channels;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*dest++ = 0;
	      else
		*dest++ = (total [b] > 65535) ? 65535 : (guint16) total [b];
	    }
	  src += num_channels;
	}
      
      /* now the right margin pixels*/
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
    }

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
convolve_area_float (PixelArea *src_area,
		 PixelArea    *dest_area,
		 gfloat        *matrix,
		 gint          size,
		 gfloat        divisor,
		 gint          mode,
                 gint          offset,
		 gint		alpha)
{
  gfloat *src, * s;
  gfloat *dest;
  gfloat * m;
  gfloat total [4];
  gint b, num_channels;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
  
  /*  Initialize some values  */
  src_width = pixelarea_areawidth (src_area);
  src_height = pixelarea_areaheight (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
 
  src_rowstride =  pixelarea_rowstride (src_area) / sizeof (gfloat);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof (gfloat);

  margin = size / 2;
  src = (gfloat *) pixelarea_data (src_area);
  dest = (gfloat *) pixelarea_data (dest_area);
  
  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  /* then do the middle  scanlines*/ 
  for (y = margin; y < src_height - margin; y++)
    {
      /* first handle the left margin pixels */
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
     
      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  /* go to the beginning of the src under the kernal */
	  s = src - margin * (num_channels + src_rowstride);

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      /* go to the next line of the src under the kernal */
	      s += src_rowstride - size * num_channels;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;
               /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

		*dest++ = total [b];
	    }
	  src += num_channels;
	}
      
      /* now the right margin pixels*/
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
    }

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
convolve_area_float16 (PixelArea *src_area,
		 PixelArea    *dest_area,
		 gfloat        *matrix,
		 gint          size,
		 gfloat        divisor,
		 gint          mode,
                 gint          offset,
		 gint 		alpha)
{
  guint16 *src, * s;
  guint16 *dest;
  gfloat * m, a;
  gfloat total [4], atotal;
  gint b, num_channels;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
  gfloat val;
  ShortsFloat u;
  gint a_index = tag_alpha_index (src_tag);

  /*  Initialize some values  */
  src_width = pixelarea_areawidth (src_area);
  src_height = pixelarea_areaheight (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
 
  src_rowstride =  pixelarea_rowstride (src_area) / sizeof (guint16);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof (guint16);

  margin = size / 2;
  src = (guint16 *) pixelarea_data (src_area);
  dest = (guint16 *) pixelarea_data (dest_area);
  
  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  /* then do the middle  scanlines*/ 
  for (y = margin; y < src_height - margin; y++)
    {
      /* first handle the left margin pixels */
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
     
      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  /* go to the beginning of the src under the kernal */
	  s = src - margin * (num_channels + src_rowstride);

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  atotal = 0;
	  i = size;  /* row */
	  while (i --)
	    {
	      j = size;
	      while (j --) /* column */
		{
	      	  atotal += a = FLT (s[a_index], u);
		  for (b = 0; b < num_channels; b++)
			{
			  val = FLT (*s++, u);
		          total [b] += b != a_index && alpha ?  (*m) * val * a : (*m) * val * a;
			}
		  m ++;
		}

	      /* go to the next line of the src under the kernal */
	      s += src_rowstride - size * num_channels;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = b != a_index && alpha ? 
		      (total[a_index] ? total [b]  / (total [a_index]) + offset : 0) :
		      total [b] / divisor + offset;
               /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

		*dest++ = FLT16 (total [b], u);
	    }
	  src += num_channels;
	}
      
      /* now the right margin pixels*/
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
    }

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}


static void
convolve_area_bfp (
		   PixelArea   *src_area,
		   PixelArea   *dest_area,
		   gfloat       *matrix,
		   gint          size,
		   gfloat        divisor,
		   gint          mode,
                   gint          offset,
		   gint		alpha
		   )
{
  guint16 *src, * s;
  guint16 *dest;
  gfloat * m;
  gfloat total [4];
  gint b, num_channels;
  gint margin;      /*  margin imposed by size of conv. matrix  */
  gint i, j;
  gint x, y;
  gint src_rowstride;
  gint dest_rowstride;
  gint src_height;
  gint src_width;
  gint bytes;
  Tag src_tag = pixelarea_tag (src_area); 
  
  /*  Initialize some values  */
  src_width = pixelarea_areawidth (src_area);
  src_height = pixelarea_areaheight (src_area);
  bytes = tag_bytes (src_tag);  /* per pixel */ 
  num_channels = tag_num_channels (src_tag); /* per pixel */
 
  src_rowstride =  pixelarea_rowstride (src_area) / sizeof (guint16);
  dest_rowstride = pixelarea_rowstride (dest_area) / sizeof (guint16);

  margin = size / 2;
  src = (guint16 *) pixelarea_data (src_area);
  dest = (guint16 *) pixelarea_data (dest_area);
  
  /* copy the first (size / 2) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes);
      src += src_rowstride;
      dest += dest_rowstride;
    }

  /* then do the middle  scanlines*/ 
  for (y = margin; y < src_height - margin; y++)
    {
      /* first handle the left margin pixels */
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
     
      /* now, handle the center pixels */
      x = src_width - margin*2;
      while (x--)
	{
	  /* go to the beginning of the src under the kernal */
	  s = src - margin * (num_channels + src_rowstride);

	  m = matrix;
	  total [0] = total [1] = total [2] = total [3] = 0;
	  i = size;
	  while (i --)
	    {
	      j = size;
	      while (j --)
		{
		  for (b = 0; b < num_channels; b++)
		    total [b] += *m * *s++;
		  m ++;
		}

	      /* go to the next line of the src under the kernal */
	      s += src_rowstride - size * num_channels;
	    }

	  for (b = 0; b < num_channels; b++)
	    {
	      total [b] = total [b] / divisor + offset;

	      /*  only if mode was ABSOLUTE will mode by non-zero here  */
	      if (total [b] < 0 && mode)
		total [b] = - total [b];

	      if (total [b] < 0)
		*dest++ = 0;
	      else
		*dest++ = (total [b] > 65535) ? 65535 : (guint16) total [b];
	    }
	  src += num_channels;
	}
      
      /* now the right margin pixels*/
      b = num_channels * margin;
      while (b --)
	*dest++ = *src++;
    }

  /* copy the last (margin) scanlines of the src image... */
  for (i = 0; i < margin; i++)
    {
      memcpy (dest, src, src_width * bytes) ;
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

/**************************************************/
/*    GAUSSIAN_BLUR                               */
/**************************************************/

static void 
gaussian_blur_area_funcs (
			Tag tag
		    )
		  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    gaussian_curve = gaussian_curve_u8;	
    gaussian_blur_row = gaussian_blur_row_u8;	
    rle_row = rle_row_u8; 
    break;
  case PRECISION_U16:
    gaussian_curve = gaussian_curve_u16;	
    gaussian_blur_row = gaussian_blur_row_u16;	
    rle_row = rle_row_u16; 
    break;
  case PRECISION_FLOAT:
    gaussian_curve = gaussian_curve_float;	
    gaussian_blur_row = gaussian_blur_row_float;	
    rle_row = rle_row_float; 
    break;
  case PRECISION_FLOAT16:
    gaussian_curve = gaussian_curve_float16;	
    gaussian_blur_row = gaussian_blur_row_float16;	
    rle_row = rle_row_float16; 
    break;
  case PRECISION_BFP:
    gaussian_curve = gaussian_curve_bfp;	
    gaussian_blur_row = gaussian_blur_row_bfp;	
    rle_row = rle_row_bfp; 
    break;
  default:
    gaussian_curve = NULL;	
    gaussian_blur_row = NULL;	
    rle_row = NULL; 
    break;
  } 
}

#ifdef WIN32
#undef g_free
#define g_free g2_free
#endif

void 
gaussian_blur_area  (
                     PixelArea * src_area,
                     gdouble radius
                     )
{
  gdouble *curve;
  gfloat *sum, total;
  gint length;
  guint i, col, row;
  PixelRow src_row, src_col, dest_row, rle_values;
  guchar *src_row_data, *src_col_data, *dest_row_data, *rle_values_data;
  gint *rle_count;  
  Tag rle_values_tag;
  Tag src_tag = pixelarea_tag (src_area);
  guint width = pixelarea_areawidth (src_area);
  guint height = pixelarea_areaheight (src_area);
  guint bytes_per_pixel = tag_bytes (src_tag);
  guint num_channels = tag_num_channels (src_tag);  /*per pixel*/
  guint bytes_per_channel = bytes_per_pixel / num_channels;
  gint src_x = pixelarea_x (src_area);
  gint src_y = pixelarea_y (src_area);
  /* add a tag check here to look for an alpha channel */

  gaussian_blur_area_funcs (src_tag);
 
  if (radius == 0.0) return;		
  
  /* get a gaussian  */
  curve = (*gaussian_curve)( 2 * radius + 1, &length ); 

  /* compute a sum for the gaussian */ 
  sum = g_malloc (sizeof (gfloat) * (2 * length + 1));
  sum[0] = 0.0;
  for (i = 1; i <= length*2; i++)
    sum[i] = curve[i-1] + sum[i-1];
  sum += length;
  total = sum[length] - sum[-length];

  /* create src column and row pixel rows */
  src_col_data = (guchar *) g_malloc (height * bytes_per_pixel);
  src_row_data = (guchar *) g_malloc (width * bytes_per_pixel);
  dest_row_data = (guchar *) g_malloc (width * bytes_per_pixel); 
  pixelrow_init (&src_col, src_tag, src_col_data, height);
  pixelrow_init (&src_row, src_tag, src_row_data, width);
  pixelrow_init (&dest_row, src_tag, dest_row_data, width);

  /* create rle arrays */
  rle_count = g_malloc (sizeof (gint) * MAXIMUM (width, height)); 
  
  rle_values_tag = tag_new (tag_precision (src_tag), FORMAT_GRAY, ALPHA_NO); 
  rle_values_data = (guchar *) g_malloc (MAXIMUM (width, height) * bytes_per_channel); 
  pixelrow_init (&rle_values, rle_values_tag , rle_values_data, MAXIMUM(width, height));
 
  /* first do the columns */
  for (col = 0; col < width; col++)
    {
      /* get a copy of the column from src */
      /*pixel_region_get_col (srcR, col + srcR->x, srcR->y, height, src, 1);*/
      pixelarea_copy_col (src_area, &src_col, src_x + col, src_y, height, 1);
      
      /* get an "rle" of the columns alpha */
      (*rle_row) (&src_col, rle_count, &rle_values);
      
      /* run a 1d gaussian on the column */
      (*gaussian_blur_row) (&src_col, &src_col, rle_count, 
					 &rle_values, length, sum, total); 

      /* copy the result back into the src */
      /*pixel_region_set_col (srcR, col + srcR->x, srcR->y, height, src);*/
      pixelarea_write_col (src_area, &src_col, src_x + col, src_y, height);
    }

  /* do the rows next */
  for (row = 0; row < height; row++)
    {
      /*pixel_region_get_row (srcR, srcR->x, row + srcR->y, width, src, 1);*/
      
      /* make copies of the row for src and dest */
 
      pixelarea_copy_row (src_area, &src_row, src_x, src_y + row, width, 1);
      pixelarea_copy_row (src_area, &dest_row, src_x, src_y + row, width, 1); 
      
      /* get an "rle" of the rows alpha */
      (*rle_row) (&src_row, rle_count, &rle_values);
      
      /* run a 1d gaussian on the row */
      (*gaussian_blur_row) (&src_row, &dest_row, rle_count, 
					 &rle_values, length, sum, total); 

      /* copy the result into src */ 	
      /*pixel_region_set_row (srcR, srcR->x, row + srcR->y, width, dest);*/
      pixelarea_write_row (src_area, &dest_row, src_x, src_y + row, width);
    }
    g_free (sum - length);
    g_free (curve);
    g_free (rle_values_data);
    g_free (src_row_data);
    g_free (src_col_data);
    g_free (dest_row_data);
}

static gdouble * 
gaussian_curve_u8  (
                    gint radius,
                    gint * length
                    )
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0/255.0)));
  return make_curve (std_dev, length, 1.0/255.0);
}

static gdouble * 
gaussian_curve_u16  (
                     gint radius,
                     gint * length
                     )
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0/65535.0)));
  return make_curve (std_dev, length, 1.0/65535.0);
}

static gdouble * 
gaussian_curve_float  (
                       gint radius,
                       gint * length
                       )
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (.000001)));
  return make_curve (std_dev, length, .000001 );
}

static gdouble * 
gaussian_curve_float16  (
                       gint radius,
                       gint * length
                       )
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2. * log (.000001)));
  return make_curve (std_dev, length, .000001 );
}

static gdouble * 
gaussian_curve_bfp  (
                     gint radius,
                     gint * length
                     )
{
  gdouble std_dev;
  std_dev = sqrt (-(radius * radius) / (2 * log (1.0/65535.0)));
  return make_curve (std_dev, length, 1.0/65535.0);
}

static void 
rle_row_u8  (
             PixelRow * src_row    /* in  -- encode this row */,
             gint * how_many       /* out -- how many of each code */,
             PixelRow * values_row /* out -- the codes */
             )
{
  gint 	  start;
  gint    i;
  gint    j;
  gint    pixels_equal;
  gint    b;
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint8 *src          = (guint8*)pixelrow_data (src_row);
  guint8 *values       = (guint8*)pixelrow_data (values_row);
  guint8 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	         values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
       for( b = 0; b < num_channels; b++)	
	 values[b]  = last[b];
       values += num_channels;
    }
}

static void 
rle_row_u16  (
              PixelRow * src_row     /* in  -- encode this row */,
              gint * how_many        /* out -- how many of each code */,
              PixelRow * values_row  /* out -- the codes */
              )
{
  gint start;
  gint i;
  gint j;
  gint pixels_equal;
  gint b;
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 *src = (guint16*)pixelrow_data (src_row);
  guint16 *values = (guint16*)pixelrow_data (values_row);
  guint16 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void 
rle_row_float  (
                PixelRow * src_row     /* in  -- encode this row */,
                gint * how_many        /* out -- how many of each code */,
                PixelRow * values_row  /* out -- the codes */
                )
{
  int start;
  int i;
  int j;
  int pixels_equal;
  int b;
  int num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  gfloat *src = (gfloat*)pixelrow_data (src_row);
  gfloat *values = (gfloat*)pixelrow_data (values_row);
  gfloat *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      
	if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void 
rle_row_float16  (
                PixelRow * src_row     /* in  -- encode this row */,
                gint * how_many        /* out -- how many of each code */,
                PixelRow * values_row  /* out -- the codes */
                )
{
  int start;
  int i;
  int j;
  int pixels_equal;
  int b;
  int num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 *src = (guint16*)pixelrow_data (src_row);
  guint16 *values = (guint16*)pixelrow_data (values_row);
  guint16 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      
	if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void 
rle_row_bfp  (
              PixelRow * src_row     /* in  -- encode this row */,
              gint * how_many        /* out -- how many of each code */,
              PixelRow * values_row  /* out -- the codes */
              )
{
  gint start;
  gint i;
  gint j;
  gint pixels_equal;
  gint b;
  gint num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 *src = (guint16*)pixelrow_data (src_row);
  guint16 *values = (guint16*)pixelrow_data (values_row);
  guint16 *last;
  
  last = src;
  src += num_channels;
  start = 0;
  for (i = 1; i < width; i++)
    {
      b = num_channels;
      pixels_equal = TRUE;
      for( b = 0; b < num_channels; b++)	
	{
	  if( src[b] != last[b] ) 
	  { 
	    pixels_equal = FALSE;
	    break;
	  } 
	}
      if (!pixels_equal)
	{
	  for (j = start; j < i; j++)
	    {
	      *how_many++ = (i - j);
               for( b = 0; b < num_channels; b++)	
	          values[b]  = last[b];
               values += num_channels;
	    }
	  start = i;
	  last = src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *how_many++ = (i - j);
      for( b = 0; b < num_channels; b++)	
	values[b]  = last[b];
      values += num_channels;
    }
}

static void 
gaussian_blur_row_u8  (
                       PixelRow * src,
                       PixelRow * dest,
                       gint * rle_count,
                       PixelRow * rle_values,
                       gint length,
                       gfloat * sum,
                       gfloat total
                       )
{  
  gint val, x, i, start, end, pixels;
  guint8 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint8 *src_data = (guint8*)pixelrow_data (src);
  guint8 *dest_data = (guint8*)pixelrow_data (dest);
  guint8 *rle_values_data = (guint8*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint8 *sp = src_data + alpha;
  guint8 *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint8*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

static void 
gaussian_blur_row_u16  (
                        PixelRow * src,
                        PixelRow * dest,
                        gint * rle_count,
                        PixelRow * rle_values,
                        gint length,
                        gfloat * sum,
                        gfloat total
                        )
{  
  gint val, x, i, start, end, pixels;
  guint16 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint16 *src_data = (guint16*)pixelrow_data (src);
  guint16 *dest_data = (guint16*)pixelrow_data (dest);
  guint16 *rle_values_data = (guint16*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint16 *sp = src_data + alpha;
  guint16 *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint16*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

static void 
gaussian_blur_row_float  (
                          PixelRow * src,
                          PixelRow * dest,
                          gint * rle_count,
                          PixelRow * rle_values,
                          gint length,
                          gfloat * sum,
                          gfloat total
                          )
{  
  gfloat val; 
  gint x, i, start, end, pixels;
  gfloat *rle_values_ptr;
  gint   *rle_count_ptr;
  gfloat *src_data = (gfloat *)pixelrow_data (src);
  gfloat *dest_data = (gfloat *)pixelrow_data (dest);
  gfloat *rle_values_data = (gfloat*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  gfloat *sp = src_data + alpha;
  gfloat *dp = dest_data + alpha;
  gfloat initial_p = sp[0];
  gfloat initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (gfloat*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

static void 
gaussian_blur_row_float16  (
                          PixelRow * src,
                          PixelRow * dest,
                          gint * rle_count,
                          PixelRow * rle_values,
                          gint length,
                          gfloat * sum,
                          gfloat total
                          )
{  
  gfloat val; 
  gint    x, i, start, end, pixels;
  guint16 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint16 *src_data = (guint16 *)pixelrow_data (src);
  guint16 *dest_data = (guint16 *)pixelrow_data (dest);
  guint16 *rle_values_data = (guint16*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint16 *sp = src_data + alpha;
  guint16 *dp = dest_data + alpha;
  ShortsFloat u;
  gfloat initial_p = FLT (sp[0], u);
  gfloat initial_m = FLT (sp[(width -1) * num_channels], u);
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint16*)rle_values_data + (x+i); 

      if (start != -length)
	val += (initial_p) * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += FLT (*rle_values_ptr, u) * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = FLT16 (val / total, u);
    }
}

static void 
gaussian_blur_row_bfp  (
                        PixelRow * src,
                        PixelRow * dest,
                        gint * rle_count,
                        PixelRow * rle_values,
                        gint length,
                        gfloat * sum,
                        gfloat total
                        )
{  
  gint val, x, i, start, end, pixels;
  guint16 *rle_values_ptr;
  gint   *rle_count_ptr;
  guint16 *src_data = (guint16*)pixelrow_data (src);
  guint16 *dest_data = (guint16*)pixelrow_data (dest);
  guint16 *rle_values_data = (guint16*)pixelrow_data (rle_values);
  Tag src_tag = pixelrow_tag (src);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (src);
  gint alpha = num_channels - 1;  /*same for both */
  guint16 *sp = src_data + alpha;
  guint16 *dp = dest_data + alpha;
  gint initial_p = sp[0];
  gint initial_m = sp[(width -1) * num_channels];
  
  for (x = 0; x < width; x++)
    {
      start = (x < length) ? -x : -length;
      end = (width <= (x + length)) ? (width - x - 1) : length;
      val = 0;
      i = start;
      rle_count_ptr = rle_count + (x + i);
      rle_values_ptr = (guint16*)rle_values_data + (x+i); 

      if (start != -length)
	val += initial_p * (sum[start] - sum[-length]);

      while (i < end)
	{
	  pixels = *rle_count_ptr;
	  i += pixels;
	  if (i > end)
	    i = end;
	  val += *rle_values_ptr * (sum[i] - sum[start]);
	  rle_values_ptr += pixels;
	  rle_count_ptr += pixels;
	  start = i;
	}

      if (end != length)
	val += initial_m * (sum[length] - sum[end]);

      dp[x * num_channels] = val / total;
    }
}

void 
border_area  (
              PixelArea * dest_area,
              void * bs_ptr,
              gint bs_segs,
              gint radius
              )
{
  BoundSeg * bs;
  gint r, i, j;
  gfloat opacity;
  
  /* should do a check on the dest_tag to make sure grayscale */
 
  bs = (BoundSeg *) bs_ptr;

  /*  draw the border  */
  for (r = 0; r <= radius; r++)
    {
      opacity = (gfloat) (r + 1) / (gfloat) (radius + 1);

      j = radius - r;

      for (i = 0; i <= j; i++)
	{
	  /*  northwest  */
	  draw_segments (dest_area, bs, bs_segs, -i, -(j - i), opacity);

	  /*  only draw the rest if they are different  */
	  if (j)
	    {
	      /*  northeast  */
	      draw_segments (dest_area, bs, bs_segs, i, -(j - i), opacity);
	      /*  southeast  */
	      draw_segments (dest_area, bs, bs_segs, i, (j - i), opacity);
	      /*  southwest  */
	      draw_segments (dest_area, bs, bs_segs, -i, (j - i), opacity);
	    }
	}
    }
}


/* non-interpolating scale_region.  [adam]
 */

static void
scale_area_no_resample_funcs (Tag tag)
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    scale_row_no_resample = scale_row_no_resample_u8;	
    break;
  case PRECISION_U16:
    scale_row_no_resample = scale_row_no_resample_u16;	
    break;
  case PRECISION_FLOAT:
    scale_row_no_resample = scale_row_no_resample_float;	
    break;
  case PRECISION_FLOAT16:
    scale_row_no_resample = scale_row_no_resample_float16;	
    break;
  case PRECISION_BFP:
    scale_row_no_resample = scale_row_no_resample_bfp;	
    break;
  default:
    scale_row_no_resample = NULL;	
    break;
  } 
}


void 
scale_area_no_resample  (
                         PixelArea * src_area,
                         PixelArea * dest_area
                         )
{
  gint *x_src_offsets, *y_src_offsets;
  gint last_src_y;
  gint x,y,b;
  PixelRow src_row, dest_row;
  guchar *dest_row_data,  *src_row_data;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint orig_width = pixelarea_areawidth (src_area);
  gint orig_height = pixelarea_areaheight (src_area); 
  gint width = pixelarea_areawidth (dest_area);
  gint height = pixelarea_areaheight (dest_area); 
  gint bytes_per_pixel = tag_bytes (src_tag);
  gint num_channels = tag_num_channels (src_tag);
  gint row_num_channels;

  scale_area_no_resample_funcs (src_tag);

  /* get a dest row to copy src pixels to */ 
  dest_row_data = (guchar *) g_malloc (width * bytes_per_pixel); 
  pixelrow_init (&dest_row, dest_tag,(guchar *)dest_row_data, width);
  
  /* get a src row */
  src_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel); 
  pixelrow_init (&src_row, src_tag,(guchar *)src_row_data, orig_width);
  
  /* x and y scale tables  */
  x_src_offsets = (int *) g_malloc (width * num_channels * sizeof(int));
  y_src_offsets = (int *) g_malloc (height * sizeof(int));
  
  /*  pre-calc the scale tables  */
  for (b = 0; b < num_channels; b++)
    {
      for (x = 0; x < width; x++)
	{
	  x_src_offsets [b + x * num_channels] = b + num_channels * ((x * orig_width + orig_width / 2) / width);
	}
    }
  for (y = 0; y < height; y++)
    {
      y_src_offsets [y] = (y * orig_height + orig_height / 2) / height;
    }
  
  /*  do the scaling  */
  row_num_channels = width * num_channels;
  last_src_y = -1;
  for (y = 0; y < height; y++)
    {
      /* if the source of this line was the same as the source
       *  of the last line, there's no point in re-rescaling.
       */
      if (y_src_offsets[y] != last_src_y)
	{
          /* pixel_region_get_row (srcPR, 0, y_src_offsets[y], orig_width, src, 1); */
          pixelarea_copy_row (src_area, &src_row, 0, y_src_offsets[y], orig_width,  1);
          (*scale_row_no_resample) (&src_row, &dest_row, x_src_offsets);
	  last_src_y = y_src_offsets[y];
	}
      /* pixel_region_set_row (destPR, 0 , y, width, dest); */
      pixelarea_write_row (dest_area, &dest_row, 0, y, width );
    }
  
  g_free (x_src_offsets);
  g_free (y_src_offsets);
  g_free (dest_row_data);
  g_free (src_row_data);
}

static void 
scale_row_no_resample_u8  (
                           PixelRow * src_row,
                           PixelRow * dest_row,
                           gint * x_src_offsets
                           )
{
  gint x;
  guint8 *dest = (guint8*) pixelrow_data (dest_row);
  guint8 *src = (guint8*) pixelrow_data (src_row);		
  gint num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint row_num_channels = num_channels * pixelrow_width (dest_row);
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}
			
static void 
scale_row_no_resample_u16  (
                            PixelRow * src_row,
                            PixelRow * dest_row,
                            gint * x_src_offsets
                            )
{
  gint x;
  guint16 *dest = (guint16*) pixelrow_data (dest_row);
  guint16 *src = (guint16*) pixelrow_data (src_row);		
  gint num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint row_num_channels = num_channels * pixelrow_width (dest_row);
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}

static void 
scale_row_no_resample_float  (
                              PixelRow * src_row,
                              PixelRow * dest_row,
                              gint * x_src_offsets
                              )
{
  gint x;
  gfloat *dest = (gfloat*) pixelrow_data (dest_row);
  gfloat *src = (gfloat*) pixelrow_data (src_row);		
  gint num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint row_num_channels = num_channels * pixelrow_width (dest_row);
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}

static void 
scale_row_no_resample_float16  (
                              PixelRow * src_row,
                              PixelRow * dest_row,
                              gint * x_src_offsets
                              )
{
  gint x;
  guint16 *dest = (guint16*) pixelrow_data (dest_row);
  guint16 *src = (guint16*) pixelrow_data (src_row);		
  gint num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint row_num_channels = num_channels * pixelrow_width (dest_row);
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}

			
static void 
scale_row_no_resample_bfp  (
                            PixelRow * src_row,
                            PixelRow * dest_row,
                            gint * x_src_offsets
                            )
{
  gint x;
  guint16 *dest = (guint16*) pixelrow_data (dest_row);
  guint16 *src = (guint16*) pixelrow_data (src_row);		
  gint num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint row_num_channels = num_channels * pixelrow_width (dest_row);
  for (x = 0; x < row_num_channels ; x++)
    dest[x] = src[x_src_offsets[x]];
}

static ScaleRowResampleFunc
scale_row_resample_funcs (
                          Tag tag
                          )
     
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return scale_row_resample_u8;	
  case PRECISION_U16:
    return scale_row_resample_u16;	
  case PRECISION_FLOAT:
    return scale_row_resample_float;	
  case PRECISION_FLOAT16:
    return scale_row_resample_float16;	
  case PRECISION_BFP:
    return scale_row_resample_bfp;	
  case PRECISION_NONE:
  default:
    g_warning ("bad precision");
    break;
  }
  return NULL;
}

static ScaleSetDestRowFunc
scale_set_dest_funcs (
                      Tag tag
                      )
		  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return scale_set_dest_row_u8;	
  case PRECISION_U16:
    return scale_set_dest_row_u16;	
  case PRECISION_FLOAT:
    return scale_set_dest_row_float;	
  case PRECISION_FLOAT16:
    return scale_set_dest_row_float16;	
  case PRECISION_BFP:
    return scale_set_dest_row_bfp;	
  case PRECISION_NONE:
  default:
    break;
  }
  return NULL;
}


void 
scale_area  (
             PixelArea * src_area,
             PixelArea * dest_area
             )
{
  gdouble *row_accum;
  gint src_row_num, dest_row_num, src_col_num;
  gdouble x_rat, y_rat;
  gdouble x_cum, y_cum;
  gdouble x_last, y_last;
  gdouble * x_frac, y_frac, tot_frac;
  gfloat  dy;
  gint i, x;
  gint advance_dest_y;
  ScaleType scale_type;
  guchar *src_row_data, *src_m1_row_data, *src_p1_row_data, *src_p2_row_data;
  PixelRow src_row, src_m1_row, src_p1_row, src_p2_row;
  /* guchar *dest_row_data; */
  guchar *src, *src_m1, *src_p1, *src_p2, *s;
  gint orig_width = pixelarea_areawidth (src_area);
  gint orig_height = pixelarea_areaheight (src_area);
  gint width = pixelarea_areawidth (dest_area);
  gint height = pixelarea_areaheight (dest_area);
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint num_channels = tag_num_channels (dest_tag);
  gint bytes_per_pixel = tag_bytes (dest_tag);

 
  /* set up the function pointers for scale */ 
  ScaleRowResampleFunc scale_row_resample = scale_row_resample_funcs (src_tag);
  ScaleSetDestRowFunc  scale_set_dest_row = scale_set_dest_funcs (dest_tag);

  /* set up the src rows and data  */
  src_m1_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_row_data    = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_p1_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  src_p2_row_data = (guchar *) g_malloc (orig_width * bytes_per_pixel);
  
  pixelrow_init (&src_m1_row, src_tag,(guchar *)src_m1_row_data, orig_width);
  pixelrow_init (&src_row, src_tag,(guchar *)src_row_data, orig_width);
  pixelrow_init (&src_p1_row, src_tag,(guchar *)src_p1_row_data, orig_width);
  pixelrow_init (&src_p2_row, src_tag,(guchar *)src_p2_row_data, orig_width);
  
  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  determine the scale type  */
  if (x_rat < 1.0 && y_rat < 1.0)
    scale_type = MagnifyX_MagnifyY;
  else if (x_rat < 1.0 && y_rat >= 1.0)
    scale_type = MagnifyX_MinifyY;
  else if (x_rat >= 1.0 && y_rat < 1.0)
    scale_type = MinifyX_MagnifyY;
  else
    scale_type = MinifyX_MinifyY;

  /*  allocate an array to help with the calculations  */
  row_accum  = (double *) g_malloc (sizeof (double) * width * num_channels);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col_num = 0;
  x_cum = 0.0;
  x_last = 0.0;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col_num ++;
	  x_frac[i] = src_col_num - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row_accum, 0, sizeof (double) * width * num_channels);

  /*  counters zero...  */
  src_row_num = 0;
  dest_row_num = 0;
  y_cum = 0.0;
  y_last = 0.0;

  /*  Get the first src row  */
  /*pixel_region_get_row (srcPR, 0, src_row_num, orig_width, src, 1);*/
  pixelarea_copy_row (src_area, &src_row, 0, src_row_num, orig_width, 1); 

  /*  Get the next two if possible  */
  if (src_row_num < (orig_height - 1))
    /*pixel_region_get_row (srcPR, 0, (src_row_num + 1), orig_width, src_p1, 1);*/
    pixelarea_copy_row (src_area, &src_p1_row, 0, src_row_num + 1, orig_width, 1); 
  if ((src_row_num + 1) < (orig_height - 1))
    /*pixel_region_get_row (srcPR, 0, (src_row_num + 2), orig_width, src_p2, 1);*/
    pixelarea_copy_row (src_area, &src_p2_row, 0, src_row_num + 2, orig_width, 1); 
  /*  Scale the selected region  */
  i = height;
  while (i)
    {

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row_num + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  dy = y_cum - src_row_num;
	  y_frac = y_cum - y_last;
	  advance_dest_y = TRUE;
	}
      else
	{
	  y_frac = (src_row_num + 1) - y_last;
	  dy = 1.0;
	  advance_dest_y = FALSE;
	}

      y_last += y_frac;

      /* set up generic guchar pointers to correct data */ 
      src = pixelrow_data (&src_row);
     
      if (src_row_num > 0)
	src_m1 = pixelrow_data (&src_m1_row);
      else
	src_m1 = pixelrow_data (&src_row);
      
      if (src_row_num < (orig_height - 1))
	src_p1 = pixelrow_data (&src_p1_row);
      else
	src_p1 = pixelrow_data (&src_row);

      if ((src_row_num + 1) < (orig_height - 1))
	src_p2 = pixelrow_data (&src_p2_row);
      else
        src_p2 = src_p1;
      
      /* scale and accumulate into row_accum array */ 
      (*scale_row_resample)(src, src_m1, src_p1, src_p2, dy, x_frac, y_frac, x_rat, 
			row_accum, num_channels, orig_width, width, scale_type); 

      if (advance_dest_y)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);
	  
          /*  copy row_accum to "dest"  */
          for( x= 0; x < width * num_channels; x++)
            row_accum[x] *= tot_frac; 

          /* this sets dest row from array row_accum */
          (*scale_set_dest_row) (dest_area, dest_row_num, width, row_accum);
                          
          /*  clear row_accum  */
	  memset (row_accum, 0, sizeof (double) * width * num_channels);
          dest_row_num++;
	  i--;
	}
      else
	{
	  /*  Shuffle data in the rows  */

	  s = pixelrow_data (&src_m1_row); 
	  pixelrow_init (&src_m1_row, src_tag, pixelrow_data (&src_row), orig_width);
	  pixelrow_init (&src_row, src_tag, pixelrow_data (&src_p1_row), orig_width);
	  pixelrow_init (&src_p1_row, src_tag, pixelrow_data (&src_p2_row), orig_width);
	  pixelrow_init (&src_p2_row, src_tag, s, orig_width);
	  
	  src_row_num++;
  	  if ((src_row_num + 1) < (orig_height - 1))
	    /*pixel_region_get_row (srcPR, 0, (src_row_num + 2), orig_width, src_p2, 1);*/
            pixelarea_copy_row (src_area, &src_p2_row, 0, src_row_num + 2, orig_width, 1);
	}
    }

  /*  free up temporary arrays  */
  g_free (row_accum);
  g_free (x_frac);
  g_free (src_m1_row_data);
  g_free (src_row_data);
  g_free (src_p1_row_data);
  g_free (src_p2_row_data);
  /* g_free (dest_row_data); */
}


static void 
scale_set_dest_row_u8  (
                        PixelArea * area,
                        guint row,
                        guint width,
                        gdouble * row_accum
                        )
{
  Tag tag = pixelarea_tag (area);
  guint chan = tag_num_channels (tag);
  guint8 * dest = g_malloc (width * chan * sizeof (guint8));
  PixelRow temp;
  guint x;

  pixelrow_init (&temp, tag, dest, width);
  for (x = 0; x < width * chan; x++)
    {
      dest[x] = (guint8) row_accum[x];
    }
  pixelarea_write_row (area, &temp, area->area.x1, area->area.y1 + row, width);
  g_free (dest);
}


static void 
scale_set_dest_row_u16  (
                         PixelArea * area,
                         guint row,
                         guint width,
                         gdouble * row_accum
                         )
{
  Tag tag = pixelarea_tag (area);
  guint chan = tag_num_channels (tag);
  guint16 * dest = g_malloc (width * chan * sizeof (guint16));
  PixelRow temp;
  gint x;

  pixelrow_init (&temp, tag, (guchar *) dest, width);
  for (x = 0; x < width * chan; x++)
    {
      dest[x] = (guint16) row_accum[x];
    }
  pixelarea_write_row (area, &temp, area->area.x1, area->area.y1 + row, width);
  g_free (dest);
}


static void 
scale_set_dest_row_float  (
                           PixelArea * area,
                           guint row,
                           guint width,
                           gdouble * row_accum
                           )
{
  Tag tag = pixelarea_tag (area);
  guint chan = tag_num_channels (tag);
  gfloat * dest = g_malloc (width * chan * sizeof (gfloat));
  PixelRow temp;
  gint x;

  pixelrow_init (&temp, tag, (guchar *)dest, width);
  for (x = 0; x < width * chan; x++)
    {
      dest[x] = (gfloat) row_accum[x];
    }
  pixelarea_write_row (area, &temp, area->area.x1, area->area.y1 + row, width);
  g_free (dest);
}

static void 
scale_set_dest_row_float16  (
                           PixelArea * area,
                           guint row,
                           guint width,
                           gdouble * row_accum
                           )
{
  Tag tag = pixelarea_tag (area);
  guint chan = tag_num_channels (tag);
  guint16 * dest = g_malloc (width * chan * sizeof (guint16));
  PixelRow temp;
  gint x;
  ShortsFloat u;

  pixelrow_init (&temp, tag, (guchar *)dest, width);
  for (x = 0; x < width * chan; x++)
    {
      dest[x] = FLT16 (row_accum[x], u);
    }
  pixelarea_write_row (area, &temp, area->area.x1, area->area.y1 + row, width);
  //pixelarea_write_row (area, &temp, 0, row, width);
  g_free (dest);
}

static void 
scale_set_dest_row_bfp  (
                         PixelArea * area,
                         guint row,
                         guint width,
                         gdouble * row_accum
                         )
{
  Tag tag = pixelarea_tag (area);
  guint chan = tag_num_channels (tag);
  guint16 * dest = g_malloc (width * chan * sizeof (guint16));
  PixelRow temp;
  gint x;

  pixelrow_init (&temp, tag, (guchar *) dest, width);
  for (x = 0; x < width * chan; x++)
    {
      dest[x] = (guint16) row_accum[x];
    }
  pixelarea_write_row (area, &temp, area->area.x1, area->area.y1 + row, width);
  g_free (dest);
}


static void 
scale_row_resample_u8  (
               guchar * src,
               guchar * src_m1,
               guchar * src_p1,
               guchar * src_p2,
               gdouble dy,
               gdouble * x_frac,
               gdouble y_frac,
               gdouble x_rat,
               gdouble * row_accum,
               gint num_channels,
               gint orig_width,
               gint width,
               gint scale_type
               )
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint8 *s    = (guint8*)src;
  guint8 *s_m1 = (guint8*)src_m1;
  guint8 *s_p1 = (guint8*)src_p1;
  guint8 *s_p2 = (guint8*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, 
	           cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], 255.0, 0.0),
	           cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 255.0, 0.0),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], 255.0, 0.0),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], 255.0, 0.0), 
		   255.0, 
		   0.0) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 255.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], 255.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
        {
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
	}

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_resample_u16  (
                guchar * src,
                guchar * src_m1,
                guchar * src_p1,
                guchar * src_p2,
                gdouble dy,
                gdouble * x_frac,
                gdouble y_frac,
                gdouble x_rat,
                gdouble * row_accum,
                gint num_channels,
                gint orig_width,
                gint width,
                gint scale_type
                )
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint16 *s    = (guint16*)src;
  guint16 *s_m1 = (guint16*)src_m1;
  guint16 *s_p1 = (guint16*)src_p1;
  guint16 *s_p2 = (guint16*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic ( dy, 
                   cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], 65535.0, 0.0), 
		   65535.0, 
		   0.0) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_resample_float  (
                  guchar * src,
                  guchar * src_m1,
                  guchar * src_p1,
                  guchar * src_p2,
                  gdouble dy,
                  gdouble * x_frac,
                  gdouble y_frac,
                  gdouble x_rat,
                  gdouble * row_accum,
                  gint num_channels,
                  gint orig_width,
                  gint width,
                  gint scale_type
                  )
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  gfloat *s    = (gfloat*)src;
  gfloat *s_m1 = (gfloat*)src_m1;
  gfloat *s_p1 = (gfloat*)src_p1;
  gfloat *s_p2 = (gfloat*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic ( dy, 
                   cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], G_MAXFLOAT, G_MINFLOAT),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], G_MAXFLOAT, G_MINFLOAT), 
		   G_MAXFLOAT, 
		   G_MINFLOAT) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], G_MAXFLOAT ,G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], G_MAXFLOAT, G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_resample_float16  (
                  guchar * src,
                  guchar * src_m1,
                  guchar * src_p1,
                  guchar * src_p2,
                  gdouble dy,
                  gdouble * x_frac,
                  gdouble y_frac,
                  gdouble x_rat,
                  gdouble * row_accum,
                  gint num_channels,
                  gint orig_width,
                  gint width,
                  gint scale_type
                  )
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint16 *s    = (guint16*)src;
  guint16 *s_m1 = (guint16*)src_m1;
  guint16 *s_p1 = (guint16*)src_p1;
  guint16 *s_p2 = (guint16*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  ShortsFloat t, u, v, w;
  gfloat cubic1, cubic2, cubic3, cubic4;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      {
		   cubic1 = cubic (dx, FLT (s_m1[b+minus_x], u), FLT (s_m1[b], v), 
				FLT (s_m1[b+plus_x], w), FLT (s_m1[b+plus2_x], t), G_MAXFLOAT, G_MINFLOAT);
		   cubic2 = cubic (dx, FLT (s[b+minus_x], u), FLT (s[b], v), 
				FLT (s[b+plus_x], w), FLT (s[b+plus2_x], t), G_MAXFLOAT, G_MINFLOAT);
		   cubic3 = cubic (dx, FLT (s_p1[b+minus_x], u), FLT (s_p1[b], v), 
				FLT (s_p1[b+plus_x], w), FLT (s_p1[b+plus2_x], t), G_MAXFLOAT, G_MINFLOAT);
		   cubic4 = cubic (dx, FLT (s_p2[b+minus_x], u), FLT (s_p2[b], v), 
				FLT (s_p2[b+plus_x], w), FLT (s_p2[b+plus2_x], t), G_MAXFLOAT, G_MINFLOAT); 
	           r[b] += cubic ( dy, cubic1, cubic2, cubic3, cubic4, G_MAXFLOAT, G_MINFLOAT) * tot_frac;
	      }
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, FLT (s[b+minus_x], u), FLT (s[b], v), 
				FLT (s[b+plus_x], w), FLT (s[b+plus2_x], t), 
				G_MAXFLOAT ,G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, FLT (s_m1[b], u), FLT (s[b], v), 
				FLT (s_p1[b], w), FLT (s_p2[b], t), 
				G_MAXFLOAT, G_MINFLOAT) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += FLT (s[b], u) * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * FLT (s[b], u) + dx * FLT (s[b+plus_x], v)) +
		       dy  * ((1 - dx) * FLT (s_p1[b], w) + dx * FLT (s_p1[b+plus_x], t))) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (FLT (s[b], u) * (1 - dx) + FLT (s[b+plus_x], v) * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (FLT (s[b], u) * (1 - dy) + FLT (s_p1[b], v) * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += FLT (s[b], u) * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}

static void 
scale_row_resample_bfp  (
                guchar * src,
                guchar * src_m1,
                guchar * src_p1,
                guchar * src_p2,
                gdouble dy,
                gdouble * x_frac,
                gdouble y_frac,
                gdouble x_rat,
                gdouble * row_accum,
                gint num_channels,
                gint orig_width,
                gint width,
                gint scale_type
                )
{
  gdouble dx;
  gint b;
  gint advance_dest_x;
  gint minus_x, plus_x, plus2_x;
  gdouble tot_frac;
  
  /* cast the data to the right type */
  guint16 *s    = (guint16*)src;
  guint16 *s_m1 = (guint16*)src_m1;
  guint16 *s_p1 = (guint16*)src_p1;
  guint16 *s_p2 = (guint16*)src_p2;
  gint src_col_num = 0;
  gdouble x_cum = 0.0;
  gdouble *r = row_accum;   /*the acumulated array so far*/ 
  gint frac = 0;
  gint j = width;
  
  /* loop through the x values in dest */
 
  while (j)
    {
      if (x_cum + x_rat <= (src_col_num + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  dx = x_cum - src_col_num;
	  advance_dest_x = TRUE;
	}
      else
	{
	  dx = 1.0;
	  advance_dest_x = FALSE;
	}

      tot_frac = x_frac[frac++] * y_frac;

      minus_x = (src_col_num > 0) ? -num_channels : 0;
      plus_x = (src_col_num < (orig_width - 1)) ? num_channels : 0;
      plus2_x = ((src_col_num + 1) < (orig_width - 1)) ? num_channels * 2 : plus_x;

      if (cubic_interpolation)
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic ( dy, 
                   cubic (dx, s_m1[b+minus_x], s_m1[b], s_m1[b+plus_x], s_m1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s[b+minus_x], s[b],	s[b+plus_x], s[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p1[b+minus_x], s_p1[b], s_p1[b+plus_x], s_p1[b+plus2_x], 65535.0, 0.0),
		   cubic (dx, s_p2[b+minus_x], s_p2[b], s_p2[b+plus_x], s_p2[b+plus2_x], 65535.0, 0.0), 
		   65535.0, 
		   0.0) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dx, s[b+minus_x], s[b], s[b+plus_x], s[b+plus2_x], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += cubic (dy, s_m1[b], s[b], s_p1[b], s_p2[b], 65535.0, 0.0) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }
      else
	switch (scale_type)
	  {
	  case MagnifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
		       dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	    break;
	  case MagnifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	    break;
	  case MinifyX_MagnifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	    break;
	  case MinifyX_MinifyY:
	    for (b = 0; b < num_channels; b++)
	      r[b] += s[b] * tot_frac;
	    break;
	  }

      if (advance_dest_x)
	{
	  r += num_channels;
	  j--;
	}
      else
	{
	  s_m1 += num_channels;
	  s    += num_channels;
	  s_p1 += num_channels;
	  s_p2 += num_channels;
	  src_col_num++;
	}
    }
}


float 
shapeburst_area  (
                  PixelArea * srcPR,
                  PixelArea * distPR
                  )
{
#define FIXME
#if 0
  Tile *tile;
  unsigned char *tile_data;
  float max_iterations;
  float *distp_cur;
  float *distp_prev;
  float *tmp;
  float min_prev;
  int min;
  int min_left;
  int length;
  int i, j, k;
  int src;
  int fraction;
  int prev_frac;
  int x, y;
  int end;
  int boundary;
  int inc;

  src = 0;

  max_iterations = 0.0;

  length = distPR->w + 1;
  distp_prev = (float *) paint_funcs_get_buffer (sizeof (float) * length * 2);
  for (i = 0; i < length; i++)
    distp_prev[i] = 0.0;

  distp_prev += 1;
  distp_cur = distp_prev + length;

  for (i = 0; i < srcPR->h; i++)
    {
      /*  set the current dist row to 0's  */
      for (j = 0; j < length; j++)
	distp_cur[j - 1] = 0.0;

      for (j = 0; j < srcPR->w; j++)
	{
	  min_prev = MIN (distp_cur[j-1], distp_prev[j]);
	  min_left = MIN ((srcPR->w - j - 1), (srcPR->h - i - 1));
	  min = (int) MIN (min_left, min_prev);
	  fraction = 255;

	  /*  This might need to be changed to 0 instead of k = (min) ? (min - 1) : 0  */
	  for (k = (min) ? (min - 1) : 0; k <= min; k++)
	    {
	      x = j;
	      y = i + k;
	      end = y - k;

	      while (y >= end)
		{
		  tile = tile_manager_get_tile (srcPR->tiles, x, y, 0);
		  tile_ref (tile);
		  tile_data = tile->data + (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH));
		  boundary = MIN ((y % TILE_HEIGHT), (tile->ewidth - (x % TILE_WIDTH) - 1));
		  boundary = MIN (boundary, (y - end)) + 1;
		  inc = 1 - tile->ewidth;

		  while (boundary--)
		    {
		      src = *tile_data;
		      if (src == 0)
			{
			  min = k;
			  y = -1;
			  break;
			}
		      if (src < fraction)
			fraction = src;

		      x++;
		      y--;
		      tile_data += inc;
		    }

		  tile_unref (tile, FALSE);
		}
	    }

	  if (src != 0)
	    {
	      /*  If min_left != min_prev use the previous fraction
	       *   if it is less than the one found
	       */
	      if (min_left != min)
		{
		  prev_frac = (int) (255 * (min_prev - min));
		  if (prev_frac == 255)
		    prev_frac = 0;
		  fraction = MIN (fraction, prev_frac);
		}
	      min++;
	    }

	  distp_cur[j] = min + fraction / 256.0;

	  if (distp_cur[j] > max_iterations)
	    max_iterations = distp_cur[j];
	}

      /*  set the dist row  */
      pixel_region_set_row (distPR, distPR->x, distPR->y + i, distPR->w, (unsigned char *) distp_cur);

      /*  swap pointers around  */
      tmp = distp_prev;
      distp_prev = distp_cur;
      distp_cur = tmp;
    }

  return max_iterations;
#endif
  return 0.0;
}


static void 
thin_area_funcs (Tag tag)
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    thin_row = thin_row_u8;
    break;
  case PRECISION_U16:
    thin_row = thin_row_u16;
    break;
  case PRECISION_FLOAT:
    thin_row = thin_row_float;
    break;
  case PRECISION_FLOAT16:
    thin_row = thin_row_float16;
    break;
  case PRECISION_BFP:
    thin_row = thin_row_bfp;
    break;
  default:
    thin_row = NULL;
    break;
  } 
}

gint 
thin_area  (
            PixelArea * src_area,
            gint type
            )
{
  guint i;
  int found_one;
  guchar *temp_data;

  PixelRow prev_row;
  PixelRow cur_row;
  PixelRow next_row;
  PixelRow dest_row;
  
  Tag src_tag = pixelarea_tag (src_area);
  gint bytes_per_pixel = tag_bytes (src_tag);
  guint width = pixelarea_areawidth (src_area);
  guint height = pixelarea_areaheight (src_area);
  gint src_x = pixelarea_x (src_area); 
  gint src_y = pixelarea_y (src_area); 

  /* get buffers for data for prev, cur and next rows , 2 extra pixels for ends */
  guchar *prev_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  guchar *next_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  guchar *cur_row_data = (guchar *) g_malloc ((width + 2) * bytes_per_pixel); 
  
  /* get buffer for the dest row */
  guchar *dest_row_data = (guchar *) g_malloc (width  * bytes_per_pixel); 
  pixelrow_init (&dest_row, src_tag, dest_row_data, width);
 
  thin_area_funcs (src_tag);
 
  /* clear buffers to 0 */
  memset (prev_row_data, 0, (width + 2) * bytes_per_pixel);
  memset (next_row_data, 0, (width + 2) * bytes_per_pixel);
  memset (cur_row_data, 0, (width + 2) * bytes_per_pixel);
  
  /* set the PixelRow buffers, inset one pixel */ 
  pixelrow_init (&prev_row, src_tag, prev_row_data + bytes_per_pixel, width);
  pixelrow_init (&cur_row, src_tag, cur_row_data + bytes_per_pixel, width);
  pixelrow_init (&next_row, src_tag, next_row_data + bytes_per_pixel, width);
 
  /* load the first row of src_area into cur_row */
  pixelarea_copy_row (src_area, &cur_row, src_x, src_y, width, 1);      
  
  found_one = FALSE;
  for (i = 0; i < height; i++)
    {
      if (i < (height - 1))
        /*pixel_region_get_row (src,src->x,src->y + i + 1, src->w, cur_row, 1);*/
        pixelarea_copy_row (src_area, &next_row, src_x, src_y + i + 1, width,1);
      else
        memset (pixelrow_data (&next_row), 0, width  * bytes_per_pixel);
      

      found_one = (*thin_row) (&cur_row, &next_row, &prev_row, &dest_row, type);

      pixelarea_write_row (src_area, &dest_row, src_x, src_y + i, width);
      
      /* shuffle prev, cur, next buffers around */
      
      temp_data = pixelrow_data (&prev_row);
      pixelrow_init (&prev_row, src_tag, pixelrow_data (&cur_row), width);
      pixelrow_init (&cur_row, src_tag, pixelrow_data (&next_row), width);
      pixelrow_init (&next_row, src_tag, temp_data, width);
    }
  g_free (prev_row_data);
  g_free (next_row_data);
  g_free (cur_row_data);
  g_free (dest_row_data);
  return (found_one == FALSE);  /*  is the area empty yet?  */
}

static gint 
thin_row_u8  (
              PixelRow * cur_row,
              PixelRow * next_row,
              PixelRow * prev_row,
              PixelRow * dest_row,
              gint type
              )
{
  gint j;
  gint found_one = FALSE;
  guint8 *cur = (guint8*) pixelrow_data (cur_row);
  guint8 *next =(guint8*) pixelrow_data (next_row);
  guint8 *prev =(guint8*) pixelrow_data (prev_row);
  guint8 *dest =(guint8*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);
  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 255)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

static gint 
thin_row_u16  (
               PixelRow * cur_row,
               PixelRow * next_row,
               PixelRow * prev_row,
               PixelRow * dest_row,
               gint type
               )
{
  gint j;
  gint found_one = FALSE;
  guint16 *cur = (guint16*) pixelrow_data (cur_row);
  guint16 *next =(guint16*) pixelrow_data (next_row);
  guint16 *prev =(guint16*) pixelrow_data (prev_row);
  guint16 *dest =(guint16*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 65535)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

static gint 
thin_row_float  (
                 PixelRow * cur_row,
                 PixelRow * next_row,
                 PixelRow * prev_row,
                 PixelRow * dest_row,
                 gint type
                 )
{
  gint j;
  gint found_one = FALSE;
  gfloat *cur = (gfloat*) pixelrow_data (cur_row);
  gfloat *next =(gfloat*) pixelrow_data (next_row);
  gfloat *prev =(gfloat*) pixelrow_data (prev_row);
  gfloat *dest =(gfloat*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0.0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 1.0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

static gint 
thin_row_float16  (
                 PixelRow * cur_row,
                 PixelRow * next_row,
                 PixelRow * prev_row,
                 PixelRow * dest_row,
                 gint type
                 )
{
  gint j;
  gint found_one = FALSE;
  guint16 *cur = (guint16 *) pixelrow_data (cur_row);
  guint16 *next =(guint16 *) pixelrow_data (next_row);
  guint16 *prev =(guint16 *) pixelrow_data (prev_row);
  guint16 *dest =(guint16 *) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);
  ShortsFloat u;
  gfloat destj;

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != ZERO_FLOAT16)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];
	      destj = FLT (dest[j], u);

	      if (FLT (cur[j - 1], u) < destj)
		dest[j] = cur[j - 1];
	      if (FLT (cur[j + 1], u) < destj)
		dest[j] = cur[j + 1];
	      if (FLT (prev[j], u) < destj)
		dest[j] = prev[j];
	      if (FLT (next[j], u) < destj)
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != ONE_FLOAT16)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];
	      destj = FLT (dest[j], u);

	      if (FLT (cur[j - 1], u) > destj)
		dest[j] = cur[j - 1];
	      if (FLT (cur[j + 1], u) > destj)
		dest[j] = cur[j + 1];
	      if (FLT (prev[j], u) > destj)
		dest[j] = prev[j];
	      if (FLT (next[j], u) > destj)
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

static gint 
thin_row_bfp  (
               PixelRow * cur_row,
               PixelRow * next_row,
               PixelRow * prev_row,
               PixelRow * dest_row,
               gint type
               )
{
  gint j;
  gint found_one = FALSE;
  guint16 *cur = (guint16*) pixelrow_data (cur_row);
  guint16 *next =(guint16*) pixelrow_data (next_row);
  guint16 *prev =(guint16*) pixelrow_data (prev_row);
  guint16 *dest =(guint16*) pixelrow_data (dest_row);
  gint width = pixelrow_width (cur_row);

  for (j = 0; j < width; j++)
    {
      if (type == SHRINK_REGION)
	{
	  if (cur[j] != 0)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] < dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] < dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] < dest[j])
		dest[j] = prev[j];
	      if (next[j] < dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
      else
	{
	  if (cur[j] != 65535)
	    {
	      found_one = TRUE;
	      dest[j] = cur[j];

	      if (cur[j - 1] > dest[j])
		dest[j] = cur[j - 1];
	      if (cur[j + 1] > dest[j])
		dest[j] = cur[j + 1];
	      if (prev[j] > dest[j])
		dest[j] = prev[j];
	      if (next[j] > dest[j])
		dest[j] = next[j];
	    }
	  else
	    dest[j] = cur[j];
	}
    }
   return found_one;
}

typedef void (*SwapRowFunc) (PixelRow*, PixelRow*);
static SwapRowFunc swap_area_funcs (Tag);

static SwapRowFunc 
swap_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return swap_row_u8;
  case PRECISION_U16:
    return swap_row_u16;
  case PRECISION_FLOAT:
    return swap_row_float;
  case PRECISION_FLOAT16:
    return swap_row_float16;
  case PRECISION_BFP:
    return swap_row_bfp;
  default:
    return NULL;
  } 
}

void 
swap_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            )
{
  void * pag;
  Tag src_tag = pixelarea_tag (src_area); 
  SwapRowFunc swap_row = swap_area_funcs (src_tag);
   /* put in tags check */
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
          /* swap_pixels (s, d, src->w * bytes); */
          (*swap_row) (&src_row, &dest_row);
        }
    }
}

typedef void (*ApplyMaskToAlphaChannelRowFunc) (PixelRow*, PixelRow*, gfloat);
static ApplyMaskToAlphaChannelRowFunc apply_mask_to_area_funcs (Tag);

static ApplyMaskToAlphaChannelRowFunc 
apply_mask_to_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return apply_mask_to_alpha_channel_row_u8;
  case PRECISION_U16:
    return apply_mask_to_alpha_channel_row_u16;
  case PRECISION_FLOAT:
    return apply_mask_to_alpha_channel_row_float;
  case PRECISION_FLOAT16:
    return apply_mask_to_alpha_channel_row_float16;
  case PRECISION_BFP:
    return apply_mask_to_alpha_channel_row_bfp;
  default:
    return NULL;
  } 
}

void 
apply_mask_to_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     gfloat opacity
                     )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  ApplyMaskToAlphaChannelRowFunc apply_mask_to_alpha_channel_row = apply_mask_to_area_funcs (src_tag);

  /* put in tags check */
  
  for (pag = pixelarea_register (2, src_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow srcrow;
      PixelRow maskrow;
      int h;
      
      h = pixelarea_height (src_area);
      while (h--)
	{
	  pixelarea_getdata (src_area, &srcrow, h);
	  pixelarea_getdata (mask_area, &maskrow, h);

          /*apply_mask_to_alpha_channel(s, m, opacity, src-> w, srcPR->bytes);*/
          (*apply_mask_to_alpha_channel_row) (&srcrow, &maskrow, opacity);
	}
    }
}

typedef void (*CombineMaskAndAlphaChannelRowFunc) (PixelRow*, PixelRow*, gfloat);
static CombineMaskAndAlphaChannelRowFunc  combine_mask_and_area_funcs(Tag);

static CombineMaskAndAlphaChannelRowFunc 
combine_mask_and_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return combine_mask_and_alpha_channel_row_u8;
  case PRECISION_U16:
    return combine_mask_and_alpha_channel_row_u16;
  case PRECISION_FLOAT:
    return combine_mask_and_alpha_channel_row_float;
  case PRECISION_FLOAT16:
    return combine_mask_and_alpha_channel_row_float16;
  case PRECISION_BFP:
    return combine_mask_and_alpha_channel_row_bfp;
  default:
    return NULL;
  } 
}

void 
combine_mask_and_area  (
                        PixelArea * src_area,
                        PixelArea * mask_area,
                        gfloat opacity
                        )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  CombineMaskAndAlphaChannelRowFunc combine_mask_and_alpha_channel_row = 
			combine_mask_and_area_funcs (src_tag);

  /*put in tags check */
  
  for (pag = pixelarea_register (2, src_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow srcrow;
      PixelRow maskrow;
      int h;
      
      h = pixelarea_height (src_area);
      while (h--)
	{
	  pixelarea_getdata (src_area, &srcrow, h);
	  pixelarea_getdata (mask_area, &maskrow, h);

  	  /*combine_mask_and_alpha_channel (s, m, opacity, src->w,src->bytes);*/
	  (*combine_mask_and_alpha_channel_row) (&srcrow, &maskrow, opacity);
	}
    }
}

typedef void (*CopyGrayToIntenARowFunc) (PixelRow*, PixelRow*);
static CopyGrayToIntenARowFunc  copy_gray_to_area_funcs(Tag);

static CopyGrayToIntenARowFunc  
copy_gray_to_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return copy_gray_to_inten_a_row_u8;
  case PRECISION_U16:
    return copy_gray_to_inten_a_row_u16;
  case PRECISION_FLOAT:
    return copy_gray_to_inten_a_row_float;
  case PRECISION_FLOAT16:
    return copy_gray_to_inten_a_row_float16;
  case PRECISION_BFP:
    return copy_gray_to_inten_a_row_bfp;
  default:
    return NULL;
  } 
}

#define FIXME /* this can go */
void 
copy_gray_to_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area
                    )
{
  void *  pag;
  Tag src_tag = pixelarea_tag (src_area); 
  CopyGrayToIntenARowFunc copy_gray_to_inten_a_row = 
			copy_gray_to_area_funcs (src_tag);


   /*put in tags check*/
  
  for (pag = pixelarea_register (2, src_area, dest_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
 
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
            
  	  /*copy_gray_to_inten_a_pixels (s, d, src->w, dest->bytes);*/
         (*copy_gray_to_inten_a_row) (&src_row, &dest_row);
        }
    }
}


static void initial_area_funcs (Tag);
typedef void (*InitialChannelRowFunc) (PixelRow *,PixelRow *);
typedef void (*InitialIndexedRowFunc) (PixelRow *, PixelRow *, unsigned char *);
typedef void (*InitialIndexedARowFunc) (PixelRow *, PixelRow *, PixelRow *, unsigned char *, gfloat);
typedef void (*DissolveRowFunc) (PixelRow *, PixelRow *, gint , gint, gfloat);
typedef void (*InitialIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*InitialIntenARowFunc) (PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
static InitialChannelRowFunc initial_channel_row;
static InitialIndexedRowFunc initial_indexed_row;
static InitialIndexedARowFunc initial_indexed_a_row;
static DissolveRowFunc dissolve_row;
static InitialIntenRowFunc initial_inten_row;
static InitialIntenARowFunc initial_inten_a_row;

static void 
initial_area_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    initial_channel_row = initial_channel_row_u8; 
    initial_indexed_row = initial_indexed_row_u8; 
    initial_indexed_a_row = initial_indexed_a_row_u8; 
    dissolve_row = dissolve_row_u8; 
    initial_inten_row = initial_inten_row_u8; 
    initial_inten_a_row = initial_inten_a_row_u8; 
    break;
  case PRECISION_U16:
    initial_channel_row = initial_channel_row_u16; 
    initial_indexed_row = initial_indexed_row_u16; 
    initial_indexed_a_row = initial_indexed_a_row_u16; 
    dissolve_row = dissolve_row_u16; 
    initial_inten_row = initial_inten_row_u16; 
    initial_inten_a_row = initial_inten_a_row_u16; 
    break;
  case PRECISION_FLOAT:
    initial_channel_row = initial_channel_row_float; 
    initial_indexed_row = NULL; 
    initial_indexed_a_row = NULL; 
    dissolve_row = dissolve_row_float; 
    initial_inten_row = initial_inten_row_float; 
    initial_inten_a_row = initial_inten_a_row_float; 
    break;
  case PRECISION_FLOAT16:
    initial_channel_row = initial_channel_row_float16; 
    initial_indexed_row = NULL; 
    initial_indexed_a_row = NULL; 
    dissolve_row = dissolve_row_float16; 
    initial_inten_row = initial_inten_row_float16; 
    initial_inten_a_row = initial_inten_a_row_float16; 
    break;
  case PRECISION_BFP:
    initial_channel_row = initial_channel_row_bfp; 
    initial_indexed_row = initial_indexed_row_bfp; 
    initial_indexed_a_row = initial_indexed_a_row_bfp; 
    dissolve_row = dissolve_row_bfp; 
    initial_inten_row = initial_inten_row_bfp; 
    initial_inten_a_row = initial_inten_a_row_bfp; 
    break;
  default:
    initial_channel_row = NULL; 
    initial_indexed_row = NULL; 
    initial_indexed_a_row = NULL; 
    dissolve_row = NULL; 
    initial_inten_row = NULL; 
    initial_inten_a_row = NULL; 
    break;
  } 
}


void 
initial_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               PixelArea * mask_area,
               unsigned char * data,   /* data is a cmap or color if needed */
               gfloat opacity,
               gint mode,
               gint * affect,
               gint type
               )
{
  void *  pag;
  gint buf_size;
  PixelRow buf_row;
  guchar *buf_row_data = 0; 
  Tag src_tag = pixelarea_tag (src_area); 
  gint src_width = pixelarea_areawidth (src_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint src_bytes = tag_bytes (src_tag); /* per pixel */
  gint src_bytes_per_channel = src_bytes / src_num_channels;
  Precision prec = tag_precision(src_tag); 
  /*put in tags check*/
  initial_area_funcs (src_tag);
  /* get a buffer for dissolve if needed */
  
  if ( (type == INITIAL_INTENSITY && mode == DISSOLVE_MODE) ||
       (type == INITIAL_INTENSITY_ALPHA && mode == DISSOLVE_MODE) ) 
  {	 
    Tag buf_tag = tag_new (prec, tag_format (src_tag), ALPHA_YES);
    
    if ( tag_alpha(src_tag) == ALPHA_YES )
      buf_size = src_width * src_bytes;
    else
      buf_size = src_width * (src_bytes + src_bytes_per_channel);
    
    /* allocate the buf_rows data */  
    buf_row_data = (guchar *) g_malloc (buf_size);
    pixelrow_init (&buf_row, buf_tag , buf_row_data, src_width); 
  } 
  
  for (pag = pixelarea_register (3, src_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src_area);
      gint src_x = pixelarea_x (src_area);
      gint src_y = pixelarea_y (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
            
	  switch (type)
	    {
	    case INITIAL_CHANNEL_MASK:
	    case INITIAL_CHANNEL_SELECTION:
  	      /*initial_channel_pixels (s, d, src->w, dest->bytes);*/
              (*initial_channel_row) (&src_row, &dest_row);
	      break;

	    case INITIAL_INDEXED:
  	      /*initial_indexed_pixels (s, d, data, src->w);*/
              (*initial_indexed_row) (&src_row, &dest_row, data);
	      break;

	    case INITIAL_INDEXED_ALPHA:
  	      /*initial_indexed_a_pixels (s, d, m, data, opacity, src->w);*/
              (*initial_indexed_a_row) (&src_row, &dest_row, &mask_row, data, opacity);
	      break;

	    case INITIAL_INTENSITY:
	      if (mode == DISSOLVE_MODE)
		{
  		  /*dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes, src->bytes + 1, 0);*/
  		  /*initial_inten_pixels (buf, d, m, opacity, affect, src->w,  src->bytes);*/
 		  (*dissolve_row) (&src_row, &buf_row, src_x, src_y + h, opacity);
                  (*initial_inten_row) (&buf_row, &dest_row, &mask_row, opacity, affect);
		}
	      else
		/*initial_inten_pixels (s, d, m, opacity, affect, src->w, pixel_region_bytes (src));*/
                (*initial_inten_row) (&src_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case INITIAL_INTENSITY_ALPHA:
	      if (mode == DISSOLVE_MODE)
		{
		  /* dissolve_pixels (s, buf, src->x, src->y + h, opacity, src->w, src->bytes, src->bytes, 1);*/
		  /* initial_inten_a_pixels (buf, d, m, opacity, affect, src->w, src->bytes);*/
                  (*dissolve_row) (&src_row, &buf_row, src_x, src_y + h, opacity);
                  (*initial_inten_a_row) (&buf_row, &dest_row, &mask_row, opacity, affect);
		}
	      else
		/*initial_inten_a_pixels (s, d, m, opacity, affect, src->w, src->bytes);*/
                (*initial_inten_a_row) (&src_row, &dest_row, &mask_row, opacity, affect);
	      break;
	    }
        }
    } 
    if (buf_row_data)
      g_free (buf_row_data);
}

static void combine_areas_funcs (Tag);
typedef void (*CombineIndexedAndIndexedRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*CombineIndexedAndIndexedARowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*CombineIndexedAAndIndexedARowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*CombineIntenAAndIndexedARowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, unsigned char *, gfloat);
typedef void (*CombineIntenAAndChannelMaskRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat);
typedef void (*CombineIntenAAndChannelSelectionRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat);
typedef void (*CombineIntenAndIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*CombineIntenAndIntenARowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*CombineIntenAAndIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *,gint);
typedef void (*CombineIntenAAndIntenARowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *,gint);
typedef void (*BehindIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*BehindIndexedRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*ReplaceIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*ReplaceIndexedRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*EraseIntenRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
typedef void (*EraseIndexedRowFunc) (PixelRow *, PixelRow *, PixelRow *, PixelRow *, gfloat, gint *);
static CombineIndexedAndIndexedRowFunc combine_indexed_and_indexed_row;
static CombineIndexedAndIndexedARowFunc combine_indexed_and_indexed_a_row;
static CombineIndexedAAndIndexedARowFunc combine_indexed_a_and_indexed_a_row;
static CombineIntenAAndIndexedARowFunc combine_inten_a_and_indexed_a_row;
static CombineIntenAAndChannelMaskRowFunc combine_inten_a_and_channel_mask_row;
static CombineIntenAAndChannelSelectionRowFunc combine_inten_a_and_channel_selection_row;
static CombineIntenAndIntenRowFunc combine_inten_and_inten_row;
static CombineIntenAndIntenARowFunc combine_inten_and_inten_a_row;
static CombineIntenAAndIntenRowFunc combine_inten_a_and_inten_row;
static CombineIntenAAndIntenARowFunc combine_inten_a_and_inten_a_row;
static BehindIntenRowFunc behind_inten_row;
static BehindIndexedRowFunc behind_indexed_row;
static ReplaceIntenRowFunc replace_inten_row;
static ReplaceIndexedRowFunc replace_indexed_row;
static EraseIntenRowFunc erase_inten_row;
static EraseIndexedRowFunc erase_indexed_row;

static void 
combine_areas_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    combine_indexed_and_indexed_row = combine_indexed_and_indexed_row_u8;
    combine_indexed_and_indexed_a_row = combine_indexed_and_indexed_a_row_u8; 
    combine_indexed_a_and_indexed_a_row = combine_indexed_a_and_indexed_a_row_u8;
    combine_inten_a_and_indexed_a_row = combine_inten_a_and_indexed_a_row_u8; 
    combine_inten_a_and_channel_mask_row = combine_inten_a_and_channel_mask_row_u8; 
    combine_inten_a_and_channel_selection_row = combine_inten_a_and_channel_selection_row_u8; 
    combine_inten_and_inten_row = combine_inten_and_inten_row_u8; 
    combine_inten_and_inten_a_row = combine_inten_and_inten_a_row_u8; 
    combine_inten_a_and_inten_row = combine_inten_a_and_inten_row_u8; 
    combine_inten_a_and_inten_a_row = combine_inten_a_and_inten_a_row_u8; 
    behind_inten_row = behind_inten_row_u8; 
    behind_indexed_row = behind_indexed_row_u8; 
    replace_inten_row = replace_inten_row_u8; 
    replace_indexed_row = replace_indexed_row_u8; 
    erase_inten_row = erase_inten_row_u8; 
    erase_indexed_row = erase_indexed_row_u8;
    break;
  case PRECISION_U16:
    combine_indexed_and_indexed_row = combine_indexed_and_indexed_row_u16;
    combine_indexed_and_indexed_a_row = combine_indexed_and_indexed_a_row_u16; 
    combine_indexed_a_and_indexed_a_row = combine_indexed_a_and_indexed_a_row_u16;
    combine_inten_a_and_indexed_a_row = combine_inten_a_and_indexed_a_row_u16; 
    combine_inten_a_and_channel_mask_row = combine_inten_a_and_channel_mask_row_u16; 
    combine_inten_a_and_channel_selection_row = combine_inten_a_and_channel_selection_row_u16; 
    combine_inten_and_inten_row = combine_inten_and_inten_row_u16; 
    combine_inten_and_inten_a_row = combine_inten_and_inten_a_row_u16; 
    combine_inten_a_and_inten_row = combine_inten_a_and_inten_row_u16; 
    combine_inten_a_and_inten_a_row = combine_inten_a_and_inten_a_row_u16; 
    behind_inten_row = behind_inten_row_u16; 
    behind_indexed_row = behind_indexed_row_u16; 
    replace_inten_row = replace_inten_row_u16; 
    replace_indexed_row = replace_indexed_row_u16; 
    erase_inten_row = erase_inten_row_u16; 
    erase_indexed_row = erase_indexed_row_u16;
    break;
  case PRECISION_FLOAT:
    combine_indexed_and_indexed_row = NULL;
    combine_indexed_and_indexed_a_row = NULL; 
    combine_indexed_a_and_indexed_a_row = NULL;
    combine_inten_a_and_indexed_a_row = NULL; 
    combine_inten_a_and_channel_mask_row = combine_inten_a_and_channel_mask_row_float; 
    combine_inten_a_and_channel_selection_row = combine_inten_a_and_channel_selection_row_float; 
    combine_inten_and_inten_row = combine_inten_and_inten_row_float; 
    combine_inten_and_inten_a_row = combine_inten_and_inten_a_row_float; 
    combine_inten_a_and_inten_row = combine_inten_a_and_inten_row_float; 
    combine_inten_a_and_inten_a_row = combine_inten_a_and_inten_a_row_float; 
    behind_inten_row = behind_inten_row_float; 
    behind_indexed_row = NULL; 
    replace_inten_row = replace_inten_row_float; 
    replace_indexed_row = NULL; 
    erase_inten_row = erase_inten_row_float; 
    erase_indexed_row = NULL;
    break;
  case PRECISION_FLOAT16:
    combine_indexed_and_indexed_row = NULL;
    combine_indexed_and_indexed_a_row = NULL; 
    combine_indexed_a_and_indexed_a_row = NULL;
    combine_inten_a_and_indexed_a_row = NULL; 
    combine_inten_a_and_channel_mask_row = combine_inten_a_and_channel_mask_row_float16; 
    combine_inten_a_and_channel_selection_row = combine_inten_a_and_channel_selection_row_float16; 
    combine_inten_and_inten_row = combine_inten_and_inten_row_float16; 
    combine_inten_and_inten_a_row = combine_inten_and_inten_a_row_float16; 
    combine_inten_a_and_inten_row = combine_inten_a_and_inten_row_float16; 
    combine_inten_a_and_inten_a_row = combine_inten_a_and_inten_a_row_float16; 
    behind_inten_row = behind_inten_row_float16; 
    behind_indexed_row = NULL; 
    replace_inten_row = replace_inten_row_float16; 
    replace_indexed_row = NULL; 
    erase_inten_row = erase_inten_row_float16; 
    erase_indexed_row = NULL;
    break;
  case PRECISION_BFP:
    combine_indexed_and_indexed_row = combine_indexed_and_indexed_row_bfp;
    combine_indexed_and_indexed_a_row = combine_indexed_and_indexed_a_row_bfp; 
    combine_indexed_a_and_indexed_a_row = combine_indexed_a_and_indexed_a_row_bfp;
    combine_inten_a_and_indexed_a_row = combine_inten_a_and_indexed_a_row_bfp; 
    combine_inten_a_and_channel_mask_row = combine_inten_a_and_channel_mask_row_bfp; 
    combine_inten_a_and_channel_selection_row = combine_inten_a_and_channel_selection_row_bfp; 
    combine_inten_and_inten_row = combine_inten_and_inten_row_bfp; 
    combine_inten_and_inten_a_row = combine_inten_and_inten_a_row_bfp; 
    combine_inten_a_and_inten_row = combine_inten_a_and_inten_row_bfp; 
    combine_inten_a_and_inten_a_row = combine_inten_a_and_inten_a_row_bfp; 
    behind_inten_row = behind_inten_row_bfp; 
    behind_indexed_row = behind_indexed_row_bfp; 
    replace_inten_row = replace_inten_row_bfp; 
    replace_indexed_row = replace_indexed_row_bfp; 
    erase_inten_row = erase_inten_row_bfp; 
    erase_indexed_row = erase_indexed_row_bfp;
    break;
  default:
    combine_indexed_and_indexed_row = NULL;
    combine_indexed_and_indexed_a_row = NULL; 
    combine_indexed_a_and_indexed_a_row = NULL;
    combine_inten_a_and_indexed_a_row = NULL; 
    combine_inten_a_and_channel_mask_row = NULL; 
    combine_inten_a_and_channel_selection_row = NULL; 
    combine_inten_and_inten_row = NULL; 
    combine_inten_and_inten_a_row = NULL; 
    combine_inten_a_and_inten_row = NULL; 
    combine_inten_a_and_inten_a_row = NULL; 
    behind_inten_row = NULL; 
    behind_indexed_row = NULL; 
    replace_inten_row = NULL; 
    replace_indexed_row = NULL; 
    erase_inten_row = NULL; 
    erase_indexed_row = NULL;
    break;
  } 
}

int
combine_areas_type (
                    Tag src,
                    Tag dst
                    )
{
  int s_index;
  int d_index;
  static int valid_combos[][6] =
  {
    {
      COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A,
      -1, -1,
      -1, -1
    },
    {
      COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A,
      -1, -1,
      -1, -1
    },
    {
      -1, -1,
      COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A,
      -1, -1
    },
    {
      -1, -1,
      COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A,
      -1, -1
    },
    {
      -1, -1,
      -1, -1,
      COMBINE_INDEXED_INDEXED, COMBINE_INDEXED_INDEXED_A
    },
    {
      -1, -1,
      -1, -1,
      -1, COMBINE_INDEXED_A_INDEXED_A
    }
  };

  switch (tag_format (src))
    {
    case FORMAT_RGB:      s_index = 0; break;
    case FORMAT_GRAY:     s_index = 1; break;
    case FORMAT_INDEXED:  s_index = 2; break;
    case FORMAT_NONE:
    default:              return -1;
    }
  
  switch (tag_format (dst))
    {
    case FORMAT_RGB:      d_index = 0; break;
    case FORMAT_GRAY:     d_index = 1; break;
    case FORMAT_INDEXED:  d_index = 2; break;
    case FORMAT_NONE:
    default:              return -1;
    }

  s_index = 2 * s_index + (tag_alpha (src) == ALPHA_YES ? 1 : 0);
  d_index = 2 * d_index + (tag_alpha (dst) == ALPHA_YES ? 1 : 0);

  return valid_combos[s_index][d_index];
  
}

void 
combine_areas  (
                PixelArea * src1_area,
                PixelArea * src2_area,
                PixelArea * dest_area,
                PixelArea * mask_area,
                unsigned char * data,   /* a colormap or a color --if needed */
                gfloat opacity,
                gint mode,
                gint * affect,
                gint type,
                gint ignore_alpha
                )
{
  void *  pag;
  gint combine = type;
  gint mode_affect;
  PixelRow buf_row;
  gint buf_size;
  guchar *buf_row_data = 0; 
  Tag src1_tag = pixelarea_tag (src1_area); 
  Tag src2_tag = pixelarea_tag (src2_area); 
  Tag dest_tag = pixelarea_tag (dest_area); 
  Tag mask_tag = pixelarea_tag (mask_area); 
  Tag buf_tag = src2_tag;
  gint src2_width = pixelarea_areawidth (src2_area);
  gint src2_bytes = tag_bytes (src2_tag);
 
  if ((tag_precision (src1_tag) != tag_precision (src2_tag)) ||
      (tag_precision (src1_tag) != tag_precision (dest_tag)) ||
      (tag_format    (src1_tag) != tag_format    (dest_tag)) ||
      (mask_area &&
       (tag_precision (src1_tag) != tag_precision (mask_tag))))
    {
      g_warning ("combine_areas: bad tags!");
      return;
    }

  combine_areas_funcs (src1_tag); 

  buf_size = src2_width * src2_bytes;
  buf_row_data = (guchar *) g_malloc (buf_size);
  pixelrow_init (&buf_row, buf_tag , buf_row_data, src2_width); 

  for (pag = pixelarea_register (4, src1_area, src2_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
      PixelRow mask_row;
 
      gint h = pixelarea_height (src1_area);
      gint src1_x = pixelarea_x (src1_area);
      gint src1_y = pixelarea_y (src1_area);

      while (h--)
        {
          pixelarea_getdata (src1_area, &src1_row, h);
          pixelarea_getdata (src2_area, &src2_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          pixelarea_getdata (mask_area, &mask_row, h);
           
	  /*  apply the paint mode based on the combination type & mode  */
	  switch (type)
	    {
	    case COMBINE_INTEN_A_INDEXED_A:
	    case COMBINE_INTEN_A_CHANNEL_MASK:
	    case COMBINE_INTEN_A_CHANNEL_SELECTION:
	      combine = type;
	      break;

	    case COMBINE_INDEXED_INDEXED:
	    case COMBINE_INDEXED_INDEXED_A:
	    case COMBINE_INDEXED_A_INDEXED_A:
	      /*  Now, apply the paint mode--for indexed images  */
	      combine = apply_indexed_layer_mode (&src1_row, &src2_row, &buf_row, mode);
	      break;

	    case COMBINE_INTEN_INTEN_A:
	    case COMBINE_INTEN_A_INTEN:
	    case COMBINE_INTEN_INTEN:
	    case COMBINE_INTEN_A_INTEN_A:
	      /*  Now, apply the paint mode  */
	      combine = apply_layer_mode (&src1_row, &src2_row, &buf_row, src1_x, src1_y+ h, opacity, mode, &mode_affect);
              if(ignore_alpha)
                combine = 6;
	      break;

	    default:
	      break;
	    }
	 
  
          /*  based on the type of the initial image...  */
	  switch (combine)
	    {
	    case COMBINE_INDEXED_INDEXED:
              (*combine_indexed_and_indexed_row) (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INDEXED_INDEXED_A:
              (*combine_indexed_and_indexed_a_row) (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INDEXED_A_INDEXED_A:
              (*combine_indexed_a_and_indexed_a_row) (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_A_INDEXED_A:
	      /*  assume the data passed to this procedure is the
	       *  indexed layer's colormap
	       */
              (*combine_inten_a_and_indexed_a_row) (&src1_row, &src2_row, &dest_row, &mask_row, data, opacity );
	      break;

	    case COMBINE_INTEN_A_CHANNEL_MASK:
	      /*  assume the data passed to this procedure is the channels color
	       * 
	       */
              (*combine_inten_a_and_channel_mask_row) (&src1_row, &src2_row, &dest_row, (PixelRow*)data, opacity );
	      break;

	    case COMBINE_INTEN_A_CHANNEL_SELECTION:
              (*combine_inten_a_and_channel_selection_row) (&src1_row, &src2_row, &dest_row, (PixelRow*)data, opacity );
	      break;


	    case COMBINE_INTEN_INTEN:
              (*combine_inten_and_inten_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_INTEN_A:
              (*combine_inten_and_inten_a_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case COMBINE_INTEN_A_INTEN:
              (*combine_inten_a_and_inten_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect, mode_affect);
	      break;

	    case COMBINE_INTEN_A_INTEN_A:
              (*combine_inten_a_and_inten_a_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect, mode_affect);
	      break;

	    case BEHIND_INTEN:
              (*behind_inten_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case BEHIND_INDEXED:
              (*behind_indexed_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case REPLACE_INTEN:
              (*replace_inten_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case REPLACE_INDEXED:
              (*replace_indexed_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case ERASE_INTEN:
              (*erase_inten_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case ERASE_INDEXED:
              (*erase_indexed_row) (&src1_row, &buf_row, &dest_row, &mask_row, opacity, affect);
	      break;

	    case NO_COMBINATION:
	      break;

	    default:
	      break;
	    }
        }
    } 
    if (buf_row_data)
      g_free (buf_row_data);
}


void 
combine_areas_replace  (
                        PixelArea * src1_area,
                        PixelArea * src2_area,
                        PixelArea * dest_area,
                        PixelArea * mask_area,
                        guchar * data,
                        gfloat opacity,
                        gint * affect,
                        gint type
                        )
{
  void *  pag;
  
   /*put in tags check*/
  
  for (pag = pixelarea_register (4, src1_area, src2_area, dest_area, mask_area);
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      PixelRow src1_row;
      PixelRow src2_row;
      PixelRow dest_row;
      PixelRow mask_row;
	
 
      gint h = pixelarea_height (src1_area);
      while (h--)
        {
	  pixelarea_getdata (src1_area, &src1_row, h);
	  pixelarea_getdata (src2_area, &src2_row, h);
	  pixelarea_getdata (dest_area, &dest_row, h);
	  pixelarea_getdata (mask_area, &mask_row, h);
         /*apply_layer_mode_replace (s1, s2, d, m, src1->x, src1->y + h, opacity, src1->w, src1->bytes, src2->bytes, affect);*/
         apply_layer_mode_replace (&src1_row, &src2_row, &dest_row, &mask_row, opacity, affect);
	}
    }
}


/*========================================================================*/

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 *    return curve is mapping of [-length,length]-->[0.0,1.0]
 */

static gdouble *
make_curve (
	    gdouble  sigma,
	    gint      *length,
	    gdouble  precision_cutoff
	    )
{
  gdouble *curve;
  gdouble sigma2;
  gdouble l;
  gdouble temp;
  gint i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (precision_cutoff));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_malloc (sizeof (gdouble) * n);

  *length = n / 2;
  curve[*length] = 1.0;

  for (i = 1; i <= *length; i++)
    {
      temp = (gdouble) exp (- (i * i) / sigma2); 
      curve[*length + i] = temp;
      curve[*length - i] = temp;
    }
  return curve;
}

static void
draw_segments (
	       PixelArea   *dest_area,
	       BoundSeg    *bs,
	       gint          num_segs,
	       gint          off_x,
	       gint          off_y,
	       gfloat opacity
               )
{
  gint x1, y1, x2, y2;
  gint tmp, i, length;
  PixelRow line;
  guchar *line_data;
  Tag dest_tag = pixelarea_tag (dest_area);
  gint width = pixelarea_areawidth (dest_area);
  gint height = pixelarea_areaheight (dest_area);
  gint bytes_per_pixel = tag_bytes (dest_tag); 
  
  length = MAXIMUM (width , height);

  /* get a row with same tag type as dest */
  line_data = (guchar *) g_malloc (length * bytes_per_pixel); 
  pixelrow_init (&line, dest_tag, line_data, length); 
  
  /* initialize line with opacity */
  {
    Tag t = tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO);
    PixelRow paint;
    pixelrow_init (&paint, t, (guchar*)&opacity, 1);
    color_row (&line, &paint);
  }

  for (i = 0; i < num_segs; i++)
    {
      x1 = bs[i].x1 + off_x;
      y1 = bs[i].y1 + off_y;
      x2 = bs[i].x2 + off_x;
      y2 = bs[i].y2 + off_y;

      if (bs[i].open == 0)
	{
	  /*  If it is vertical  */
	  if (x1 == x2)
	    {
	      x1 -= 1;
	      x2 -= 1;
	    }
	  else
	    {
	      y1 -= 1;
	      y2 -= 1;
	    }
	}

      /*  render segment  */
      x1 = BOUNDS (x1, 0, width - 1);
      y1 = BOUNDS (y1, 0, height - 1);
      x2 = BOUNDS (x2, 0, width - 1);
      y2 = BOUNDS (y2, 0, height - 1);

      if (x1 == x2)
	{
	  if (y2 < y1)
	    {
	      tmp = y1;
	      y1 = y2;
	      y2 = tmp;
	    }
	  /*pixel_region_set_col (destPR, x1, y1, (y2 - y1), line);*/
          pixelarea_write_col (dest_area, &line, x1, y1, (y2 - y1)); 
	}
      else
	{
	  if (x2 < x1)
	    {
	      tmp = x1;
	      x1 = x2;
	      x2 = tmp;
	    }
	  /*pixel_region_set_row (destPR, x1, y1, (x2 - x1), line);*/
          pixelarea_write_row (dest_area, &line, x1, y1, (y2 - y1)); 
	}
    }
    g_free( line_data );
}

static inline gdouble
cubic (gdouble     dx,
       gdouble     jm1,
       gdouble    j,
       gdouble    jp1,
       gdouble    jp2,
       gdouble  hi,    /* the hi code for data type */
       gdouble  lo     /* the lo code for data type */)
{
  gdouble result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
                       ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
                     ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;

  /* Catmull-Rom - not bad */
  if (result < lo)
    result = lo;
  if (result > hi)
    result = hi;

  return result;
}
#if 0
static gdouble
cubic (
       gdouble  dx,
       gdouble  jm1,
       gdouble  j,
       gdouble  jp1,
       gdouble  jp2,
       gdouble  hi,    /* the hi code for data type */
       gdouble  lo     /* the lo code for data type */
       )
{
  double dx1, dx2, dx3;
  double h1, h2, h3, h4;
  double result;

  /*  constraint parameter = -1  */
  dx1 = fabs (dx);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h1 = dx3 - 2 * dx2 + 1;
  result = h1 * j;

  dx1 = fabs (dx - 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h2 = dx3 - 2 * dx2 + 1;
  result += h2 * jp1;

  dx1 = fabs (dx - 2.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h3 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h3 * jp2;

  dx1 = fabs (dx + 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h4 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h4 * jm1;

  if (result < lo)
    result = lo;
  if (result > hi)
    result = hi;

  return result;
}
#endif
/************************************/
/*       apply layer modes          */
/************************************/
static void apply_layer_mode_funcs (Tag);

typedef void (*MultiplyRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*ScreenRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*OverlayRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*DifferenceRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*AddRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*SubtractRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*DarkenRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*LightenRowFunc) (PixelRow *, PixelRow *, PixelRow *);
typedef void (*HsvOnlyRowFunc) (PixelRow *, PixelRow *, PixelRow *, gint);
typedef void (*ColorOnlyRowFunc) (PixelRow *, PixelRow *, PixelRow *, gint);
static AddAlphaRowFunc add_alpha_row;
static MultiplyRowFunc multiply_row;
static ScreenRowFunc screen_row;
static OverlayRowFunc overlay_row;
static DifferenceRowFunc difference_row;
static AddRowFunc add_row;
static SubtractRowFunc subtract_row;
static DarkenRowFunc darken_row;
static LightenRowFunc lighten_row;
static HsvOnlyRowFunc hsv_only_row;
static ColorOnlyRowFunc color_only_row;

static void 
apply_layer_mode_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    add_alpha_row = add_alpha_row_u8;
    dissolve_row = dissolve_row_u8; 
    multiply_row = multiply_row_u8; 
    screen_row = screen_row_u8; 
    overlay_row = overlay_row_u8; 
    difference_row = difference_row_u8; 
    add_row = add_row_u8; 
    subtract_row = subtract_row_u8; 
    darken_row = darken_row_u8; 
    lighten_row = lighten_row_u8; 
    hsv_only_row = hsv_only_row_u8; 
    color_only_row = color_only_row_u8; 
    break;
  case PRECISION_U16:
    add_alpha_row = add_alpha_row_u16;
    dissolve_row = dissolve_row_u16; 
    multiply_row = multiply_row_u16; 
    screen_row = screen_row_u16; 
    overlay_row = overlay_row_u16; 
    difference_row = difference_row_u16; 
    add_row = add_row_u16; 
    subtract_row = subtract_row_u16; 
    darken_row = darken_row_u16; 
    lighten_row = lighten_row_u16; 
    hsv_only_row = hsv_only_row_u16; 
    color_only_row = color_only_row_u16; 
    break;
  case PRECISION_FLOAT:
    add_alpha_row = add_alpha_row_float;
    dissolve_row = dissolve_row_float; 
    multiply_row = multiply_row_float; 
    screen_row = screen_row_float; 
    overlay_row = overlay_row_float; 
    difference_row = difference_row_float; 
    add_row = add_row_float; 
    subtract_row = subtract_row_float; 
    darken_row = darken_row_float; 
    lighten_row = lighten_row_float; 
    hsv_only_row = hsv_only_row_float; 
    color_only_row = color_only_row_float; 
    break;
  case PRECISION_FLOAT16:
    add_alpha_row = add_alpha_row_float16;
    dissolve_row = dissolve_row_float16; 
    multiply_row = multiply_row_float16; 
    screen_row = screen_row_float16; 
    overlay_row = overlay_row_float16; 
    difference_row = difference_row_float16; 
    add_row = add_row_float16; 
    subtract_row = subtract_row_float16; 
    darken_row = darken_row_float16; 
    lighten_row = lighten_row_float16; 
    hsv_only_row = hsv_only_row_float16; 
    color_only_row = color_only_row_float16; 
    break;
  case PRECISION_BFP:
    add_alpha_row = add_alpha_row_bfp;
    dissolve_row = dissolve_row_bfp; 
    multiply_row = multiply_row_bfp;
    screen_row = screen_row_bfp;
    overlay_row = overlay_row_bfp;
    difference_row = difference_row_bfp;
    add_row = add_row_bfp;
    subtract_row = subtract_row_bfp;
    darken_row = darken_row_bfp;
    lighten_row = lighten_row_bfp;
    hsv_only_row = hsv_only_row_bfp;
    color_only_row = color_only_row_bfp;
    break;
  default:
    add_alpha_row = NULL;
    dissolve_row = NULL; 
    multiply_row = NULL; 
    screen_row = NULL; 
    overlay_row = NULL; 
    difference_row = NULL; 
    add_row = NULL; 
    subtract_row = NULL; 
    darken_row = NULL; 
    lighten_row = NULL; 
    hsv_only_row = NULL; 
    color_only_row = NULL; 
    break;
  } 
}


static int 
apply_layer_mode  (
                   PixelRow *src1_row,
                   PixelRow *src2_row,
                   PixelRow *dest_row,
                   gint x,
                   gint y,
                   gfloat opacity,
                   gint mode,
                   gint * mode_affect
                   )
{
  gint combine;
  Tag src1_tag = pixelrow_tag (src1_row);
  Tag src2_tag = pixelrow_tag (src2_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint ha1  = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint ha2  = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guchar *src2_data = pixelrow_data (src2_row); 
  gint width = pixelrow_width (dest_row); 
  Format src1_format = tag_format (src1_tag);
  
  apply_layer_mode_funcs (src1_tag);
 
  if (!ha1 && !ha2)
    combine = COMBINE_INTEN_INTEN;
  else if (!ha1 && ha2)
    combine = COMBINE_INTEN_INTEN_A;
  else if (ha1 && !ha2)
    combine = COMBINE_INTEN_A_INTEN;
  else
    combine = COMBINE_INTEN_A_INTEN_A;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case NORMAL_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      break;

    case DISSOLVE_MODE:
      /*  Since dissolve requires an alpha channels...  */
      if (! ha2)
	/*add_alpha_pixels (src2, *dest, length, b2);*/
      (*add_alpha_row) (src2_row, dest_row);

      /*dissolve_pixels (src2, *dest, x, y, opacity, length, b2, ((ha2) ? b2 : b2 + 1), ha2);*/
      (*dissolve_row) (src2_row, dest_row, x, y, opacity);
      combine = (ha1) ? COMBINE_INTEN_A_INTEN_A : COMBINE_INTEN_INTEN_A;
      break;

    case MULTIPLY_MODE:
      /*multiply_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*multiply_row) (src1_row, src2_row, dest_row);
      break;

    case SCREEN_MODE:
      /*screen_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*screen_row) (src1_row, src2_row, dest_row);
      break;

    case OVERLAY_MODE:
      /*overlay_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*overlay_row) (src1_row, src2_row, dest_row);
      break;

    case DIFFERENCE_MODE:
      /*difference_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*difference_row) (src1_row, src2_row, dest_row);
      break;

    case ADDITION_MODE:
      /*add_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*add_row) (src1_row, src2_row, dest_row);
      break;

    case SUBTRACT_MODE:
      /*subtract_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*subtract_row) (src1_row, src2_row, dest_row);
      break;

    case DARKEN_ONLY_MODE:
      /*darken_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*darken_row) (src1_row, src2_row, dest_row);
      break;

    case LIGHTEN_ONLY_MODE:
      /*lighten_pixels (src1, src2, *dest, length, b1, b2, ha1, ha2);*/
      (*lighten_row) (src1_row, src2_row, dest_row);
      break;

    case HUE_MODE: case SATURATION_MODE: case VALUE_MODE:
      /*  only works on RGB color images  */
      /*if (b1 > 2) */
	/*hsv_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2);*/
      if( src1_format == FORMAT_RGB )
        (*hsv_only_row) (src1_row, src2_row, dest_row, mode);
      /*else*/
	/* *dest = src2; */
         /*pixelrow_init (dest_row, dest_tag, src2_data, width);
      */break;

    case COLOR_MODE:
      /*  only works on RGB color images  */
      /* if (b1 > 2) */
	 /* color_only_pixels (src1, src2, *dest, mode, length, b1, b2, ha1, ha2); */
      if( src1_format == FORMAT_RGB )
        (*color_only_row) (src1_row, src2_row, dest_row, mode);
      else
         /* *dest = src2; */
         pixelrow_init (dest_row, dest_tag, src2_data, width);
      break;

    case BEHIND_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      if (ha1)
	combine = BEHIND_INTEN;
      else
	combine = NO_COMBINATION;
      break;

    case REPLACE_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      combine = REPLACE_INTEN;
      break;

    case ERASE_MODE:
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (ha1 && ha2) ? ERASE_INTEN : combine;
      break;

    default :
        e_printf ("ERROR: Mode not implemented\n");
      break;
    }

  /*  Determine whether the alpha channel of the destination can be affected
   *  by the specified mode--This keeps consistency with varying opacities
   */
  *mode_affect = layer_modes_16[mode].affect_alpha;

  return combine;
}


static int 
apply_indexed_layer_mode  (
                           PixelRow * src1_row,
                           PixelRow * src2_row,
                           PixelRow * dest_row,
                           gint mode
                           )
{

  int combine;
  Tag src1_tag = pixelrow_tag (src1_row);
  Tag src2_tag = pixelrow_tag (src2_row);
  Tag dest_tag = pixelrow_tag (dest_row);
  gint ha1  = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint ha2  = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guchar *src2_data = pixelrow_data (src2_row); 
  gint width = pixelrow_width (dest_row); 

  if (!ha1 && !ha2)
    combine = COMBINE_INDEXED_INDEXED;
  else if (!ha1 && ha2)
    combine = COMBINE_INDEXED_INDEXED_A;
  else if (ha1 && ha2)
    combine = COMBINE_INDEXED_A_INDEXED_A;
  else
    combine = NO_COMBINATION;

  /*  assumes we're applying src2 TO src1  */
  switch (mode)
    {
    case REPLACE_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      combine = REPLACE_INDEXED;
      break;

    case BEHIND_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      if (ha1)
	combine = BEHIND_INDEXED;
      else
	combine = NO_COMBINATION;
      break;

    case ERASE_MODE:
      /* make the data of dest point to src2 */
      /* *dest = src2; */
      pixelrow_init (dest_row, dest_tag, src2_data, width);
      /*  If both sources have alpha channels, call erase function.
       *  Otherwise, just combine in the normal manner
       */
      combine = (ha1 && ha2) ? ERASE_INDEXED : combine;
      break;

    default:
      break;
    }

  return combine;
}


typedef void (*ReplaceRowFunc) (PixelRow*, PixelRow*, PixelRow*, PixelRow*, gfloat, gint*);
static ReplaceRowFunc apply_layer_mode_replace_funcs (Tag);

static ReplaceRowFunc 
apply_layer_mode_replace_funcs (
		Tag tag
		)
	  
{
  switch (tag_precision (tag))
  {
  case PRECISION_U8:
    return replace_row_u8;
  case PRECISION_U16:
    return replace_row_u16;
  case PRECISION_FLOAT:
    return replace_row_float;
  case PRECISION_FLOAT16:
    return replace_row_float16;
  case PRECISION_BFP:
    return replace_row_bfp;
  default:
    return NULL;
  } 
}
 
static void 
apply_layer_mode_replace  (
                           PixelRow * src1_row,
                           PixelRow * src2_row,
                           PixelRow * dest_row,
                           PixelRow * mask_row,
                           gfloat opacity,
                           gint * affect
                           )
{
  Tag src1_tag = pixelrow_tag (src1_row);
  ReplaceRowFunc replace_row = apply_layer_mode_replace_funcs (src1_tag);
  /*replace_pixels (src1, src2, dest, mask, length, opacity, affect, b1, b2);*/
  (*replace_row) (src1_row, src2_row, dest_row, mask_row, opacity, affect);
}


/*********************************
 *   color conversion routines   *
 *********************************/
void
rgb_to_hsv (gfloat *r,
    gfloat *g,
    gfloat *b)
{
  gfloat red, green, blue;
  gfloat h, s, v;
  gfloat min, max;
  gfloat delta;

  h = 0.0;

  red = *r;
  green = *g;
  blue = *b;

  max = red;
  max = max < green ? green : max;
  max = max < blue ? blue : max;
  min = red;
  min = min > green ? green : min;
  min = min > blue ? blue : min;

  v = max;

  s = max ? (max - min) / max : max;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2.0 + (blue - red) / delta;
      else if (blue == max)
	h = 4.0 + (red - green) / delta;
      h *= 60.0;

      if (h < 0)
	h += 360;
      if (h > 360)
	h -= 360;
    }

  *r = h;
  *g = s;
  *b = v;
  
}

void
hsv_to_rgb (gfloat *h,
    gfloat *s,
    gfloat *v)

{
  gfloat hue, saturation, value;
  gfloat f, p, q, t;

  if (*s == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h; 
      saturation = *s;
      value = *v;

      hue = hue == 360 ? 0 : hue; 
      hue = hue / 60.0;
      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;
	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;
	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;
	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;
	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;
	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	}
    }
}

void
rgb_to_hls (int *r,
	    int *g,
	    int *b)
{
  int red, green, blue;
  float h, l, s;
  int min, max;
  int delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
	s = 255 * (float) delta / (float) (max + min);
      else
	s = 255 * (float) delta / (float) (511 - max - min);

      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else
	h = 4 + (red - green) / (float) delta;

      h = h * 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = l;
  *b = s;
}


static int
hls_value (float n1,
	   float n2,
	   float hue)
{
  float value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;
  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return (int) (value * 255);
}


void
hls_to_rgb (int *h,
	    int *l,
	    int *s)
{
  float hue, lightness, saturation;
  float m1, m2;

  hue = *h;
  lightness = *l;
  saturation = *s;

  if (saturation == 0)
    {
      /*  achromatic case  */
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      if (lightness < 128)
	m2 = (lightness * (255 + saturation)) / 65025.0;
      else
	m2 = (lightness + saturation - (lightness * saturation)/255.0) / 255.0;

      m1 = (lightness / 127.5) - m2;

      /*  chromatic case  */
      *h = hls_value (m1, m2, hue + 85);
      *l = hls_value (m1, m2, hue);
      *s = hls_value (m1, m2, hue - 85);
    }
}

