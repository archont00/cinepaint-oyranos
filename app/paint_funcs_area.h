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

#ifndef __PAINT_FUNCS_AREA_H__
#define __PAINT_FUNCS_AREA_H__

#include "tag.h"

/* FIXME - this stuff needs to be cleaned up */
void copy_row (PixelRow *, PixelRow *);

#define OPAQUE_OPACITY 1.0
#define TRANSPARENT_OPACITY 0.0

/*  Color conversion routines  */
void  rgb_to_hsv            (gfloat *, gfloat *, gfloat *);
void  hsv_to_rgb            (gfloat *, gfloat *, gfloat *);
void  rgb_to_hls            (int *, int *, int *);
void  hls_to_rgb            (int *, int *, int *);


/* end FIXME */


/*  The types of convolutions  */
#define NORMAL     0   /*  Negative numbers truncated  */
#define ABSOLUTE   1   /*  Absolute value              */
#define NEGATIVE   2   /*  add 127 to values           */

/*  The types of thinning  */
#define SHRINK_REGION 0
#define GROW_REGION   1

/*  Lay down the groundwork for layer construction...
 *  This includes background images for indexed or non-alpha
 *  images, floating selections, selective display of intensity
 *  channels, and display of arbitrary mask channels
 */
#define INITIAL_CHANNEL_MASK         0
#define INITIAL_CHANNEL_SELECTION    1
#define INITIAL_INDEXED              2
#define INITIAL_INDEXED_ALPHA        3
#define INITIAL_INTENSITY            4
#define INITIAL_INTENSITY_ALPHA      5

/*  Combine two source regions with the help of an optional mask
 *  region into a destination region.  This is used for constructing
 *  layer projections, and for applying image patches to an image
 */
#define COMBINE_INDEXED_INDEXED           0
#define COMBINE_INDEXED_INDEXED_A         1
#define COMBINE_INDEXED_A_INDEXED_A       2
#define COMBINE_INTEN_A_INDEXED_A         3
#define COMBINE_INTEN_A_CHANNEL_MASK      4
#define COMBINE_INTEN_A_CHANNEL_SELECTION 5
#define COMBINE_INTEN_INTEN               6
#define COMBINE_INTEN_INTEN_A             7
#define COMBINE_INTEN_A_INTEN             8
#define COMBINE_INTEN_A_INTEN_A           9

/*  Non-conventional combination modes  */
#define BEHIND_INTEN                      20
#define BEHIND_INDEXED                    21
#define REPLACE_INTEN                     22
#define REPLACE_INDEXED                   23
#define ERASE_INTEN                       24
#define ERASE_INDEXED                     25
#define NO_COMBINATION                    26


/*  Layer Modes  */
#define NORMAL_MODE        0
#define DISSOLVE_MODE      1
#define BEHIND_MODE        2
#define MULTIPLY_MODE      3
#define SCREEN_MODE        4
#define OVERLAY_MODE       5
#define DIFFERENCE_MODE    6
#define ADDITION_MODE      7
#define SUBTRACT_MODE      8
#define DARKEN_ONLY_MODE   9
#define LIGHTEN_ONLY_MODE  10
#define HUE_MODE           11
#define SATURATION_MODE    12
#define COLOR_MODE         13
#define VALUE_MODE         14
#define ERASE_MODE         20
#define REPLACE_MODE       21


void paint_funcs_area_setup (void);
void paint_funcs_area_free (void);

void
x_add_area (
		PixelArea *, 
		PixelArea *
		);
void
x_sub_area (
		PixelArea *, 
		PixelArea *
		);
void
x_min_area (
		PixelArea *, 
		PixelArea *
		);
void
invert_area (
		PixelArea *, 
		PixelArea *
		);

void
absdiff_area (
              PixelArea * image,
              PixelArea * mask,
              PixelRow * color,
              gfloat threshold,
              int antialias
              );

void
extract_channel_area (
		       PixelArea * src_area,
		       PixelArea * dest_area, 
		       gint channel
                      );
void 
color_area  (
             PixelArea * src,
             PixelRow * color
             );

void 
blend_area  (
             PixelArea * src1_area,
             PixelArea * src2_area,
             PixelArea * dest_area,
             gfloat blend,
	     gint alpha
            );

void
shade_area  (
             PixelArea * src_area,
             PixelArea * dest_area,
             PixelRow * col,
             gfloat blend
             );

void 
copy_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            );

void 
add_alpha_area  (
                 PixelArea * src_area,
                 PixelArea * dest_area
                 );

void 
flatten_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               PixelRow * bg
               );

void 
extract_alpha_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     PixelArea * dest_area
                     );

void 
extract_from_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area,
                    PixelArea * mask_area,
                    PixelRow *bg,
                    unsigned char *cmap,
                    gint cut
                    );

void
convolve_area (
	 	PixelArea   *src_area,
		PixelArea   *dest_area,
		gfloat       *matrix,
		guint         matrix_size,
		gfloat        divisor,
		gint          mode,
		gint	      alpha
		);
void 
multiply_alpha_area  (
                      PixelArea * src_area
                      );

void 
separate_alpha_area  (
                      PixelArea * src_row
                      );

void
gaussian_blur_area (
		    PixelArea    *src_area,
		    gdouble       radius
		   );

void
border_area (
	     PixelArea *dest_area,
	     void      *bs_ptr,
	     gint       bs_segs,
	     gint       radius
	    );

void
scale_area_no_resample (
			PixelArea *src_area,
			PixelArea *dest_area
		       );

void
scale_area_exact (
	    PixelArea *src_area,
	    PixelArea *dest_area,
            double     pos_x,
            double     pos_y,
            double     ratio_x,
            double     ratio_y
           );

void
scale_area (
	    PixelArea *src_area,
	    PixelArea *dest_area
           );

float
shapeburst_area (
                 PixelArea *srcPR,
                 PixelArea *distPR
                 );

gint
thin_area (
	   PixelArea *src_area,
	   gint          type
          );

void 
swap_area  (
            PixelArea * src_area,
            PixelArea * dest_area
            );

void 
apply_mask_to_area  (
                     PixelArea * src_area,
                     PixelArea * mask_area,
                     gfloat opacity
                     );

void 
combine_mask_and_area  (
                        PixelArea * src_area,
                        PixelArea * mask_area,
                        gfloat opacity
                        );

void 
copy_gray_to_area  (
                    PixelArea * src_area,
                    PixelArea * dest_area
                    );

void 
initial_area  (
               PixelArea * src_area,
               PixelArea * dest_area,
               PixelArea * mask_area,
               unsigned char * data,    /*data is a cmap or color if needed*/
               gfloat opacity,
               gint mode,
               gint * affect,
               gint type
               );

/* temp until gimage.c operations table is moved here */
int
combine_areas_type (
                    Tag src,
                    Tag dst
                    );

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
                gint type, /* this will eventually be two tags and the operations table in
                             gimage.c will be internal to this module */
                gint ignore_alpha
                );

void 
combine_areas_replace  (
                        PixelArea *src1_area,
                        PixelArea *src2_area,
                        PixelArea *dest_area,
                        PixelArea *mask_area,
                        guchar * data,
                        gfloat opacity,
                        gint * affect,
                        gint type
                        );
#endif  /*  __PAINT_FUNCS_AREA_H__  */
