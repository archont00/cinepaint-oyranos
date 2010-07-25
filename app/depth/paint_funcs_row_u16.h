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
#ifndef __PAINT_FUNCS_ROW_U16_H__
#define __PAINT_FUNCS_ROW_U16_H__

void
x_add_row_u16 (
		PixelRow *, 
		PixelRow *
		);
void
x_sub_row_u16 (
		PixelRow *, 
		PixelRow *
		);
void
x_min_row_u16 (
		PixelRow *, 
		PixelRow *
		);
void
invert_row_u16 (
		PixelRow *, 
		PixelRow *
		);
void
absdiff_row_u16 (
                 PixelRow *,
                 PixelRow *,
                 PixelRow *,
                 gfloat,
                 int
                 );

void
copy_row_u16 (
	      PixelRow * src,
	      PixelRow * dest
	      );

void
extract_channel_row_u16 ( 
			PixelRow *src_row,
			PixelRow *dest_row, 
			gint channel
	);

void 
color_row_u16  (
                void *, void *,
                guint, guint,
                guint, guint
                );

void
blend_row_u16 (
              PixelRow *src1_row,
	      PixelRow *src2_row,
	      PixelRow *dest_row,
	      gfloat blend,
	      gint alpha
              );

void
shade_row_u16 (
		 PixelRow *src_row,
	         PixelRow *dest_row,
	         PixelRow    *color,
		 gfloat blend
	         );

void
extract_alpha_row_u16 (
		      PixelRow *src_row,
		      PixelRow *mask_row,
		      PixelRow *dest_row
		      );

void
darken_row_u16 (
	 	PixelRow *src1_row,
		PixelRow *src2_row,
		PixelRow *dest_row
	       );

void
lighten_row_u16 (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
                   PixelRow *dest_row
		   );

void
hsv_only_row_u16 (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row,
		    gint       mode
		    );

void
color_only_row_u16 (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row,
		      gint       mode
		     );

void
multiply_row_u16 (
		 PixelRow *src1_row,
		 PixelRow *src2_row,
		 PixelRow *dest_row
		 );

void
screen_row_u16 (
		  PixelRow *src1_row,
		  PixelRow *src2_row,
		  PixelRow *dest_row
	          );

void
overlay_row_u16 (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row
		   );

void
add_row_u16 ( 
	       PixelRow *src1_row,
	       PixelRow *src2_row,
	       PixelRow *dest_row
	      );

void
subtract_row_u16 (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row
		    );

void
difference_row_u16 (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row
		      );

void
dissolve_row_u16 (
		    PixelRow *src_row,
		    PixelRow *dest_row,
		    gint      x,
		    gint      y,
		    gfloat opac
		    );

void
replace_row_u16 (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row,
		   PixelRow *mask_row,
		   gfloat opac,
		   gint      *affect
		   );

void
swap_row_u16 (
                PixelRow *src_row,
	        PixelRow *dest_row
	        );

void
scale_row_u16 (
		 PixelRow *src_row,
		 PixelRow *dest_row,
		 gfloat      scale
		  );

void
add_alpha_row_u16 (
		  PixelRow *src_row,
		  PixelRow *dest_row
		  );

void
flatten_row_u16 (
		   PixelRow *src_row,
		   PixelRow *dest_row,
 		   PixelRow    *background
		  );

void
multiply_alpha_row_u16( 
		      PixelRow * src_row
		     );

void separate_alpha_row_u16( 
			    PixelRow *src_row
                           );

void
gray_to_rgb_row_u16 (
		       PixelRow *src_row,
		       PixelRow *dest_row
		       );

/*  apply the mask data to the alpha channel of the pixel data  */
void
apply_mask_to_alpha_channel_row_u16 (
				PixelRow *src_row,
				PixelRow *mask_row,
				gfloat opac
			       );

/*  combine the mask data with the alpha channel of the pixel data  */
void
combine_mask_and_alpha_channel_row_u16 (
				    PixelRow *src_row,
				    PixelRow *mask_row,
				    gfloat opac
				    );

/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE);
 */
void
copy_gray_to_inten_a_row_u16 (
				PixelRow *src_row,
				PixelRow *dest_row
			        );

/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void
initial_channel_row_u16 (
			   PixelRow *src_row,
			   PixelRow *dest_row
			  );

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_row_u16 (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    unsigned char *cmap
			   );

/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_a_row_u16 (
			     PixelRow *src_row,
			     PixelRow *dest_row,
			     PixelRow *mask_row,
			     unsigned char *cmap,
			     gfloat opac
			     );

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void
initial_inten_row_u16 (
			  PixelRow *src_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  gfloat opac,
			  gint      *affect
		         );

/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void
initial_inten_a_row_u16 (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    PixelRow *mask_row,
			    gfloat opac,
			    gint     *affect
			   );

/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void
combine_indexed_and_indexed_row_u16 (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					gfloat opac,
				        gint     *affect
				       );

/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void
combine_indexed_and_indexed_a_row_u16 (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					gfloat opac,
				        gint     *affect
				        ); 

/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void
combine_indexed_a_and_indexed_a_row_u16 (
					    PixelRow *src1_row,
					    PixelRow *src2_row,
					    PixelRow *dest_row,
					    PixelRow *mask_row,
					    gfloat opac,
					    gint      *affect
					   );

/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void
combine_inten_a_and_indexed_a_row_u16 (
					  PixelRow *src1_row,
					  PixelRow *src2_row,
					  PixelRow *dest_row,
					  PixelRow *mask_row,
					  unsigned char *cmap,
					  gfloat opac
					 );

/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void
combine_inten_and_inten_row_u16 (
				    PixelRow *src1_row,
				    PixelRow *src2_row,
				    PixelRow *dest_row,
				    PixelRow *mask_row,
				    gfloat opac,
				    gint      *affect
				   );

/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void
combine_inten_and_inten_a_row_u16 (
				      PixelRow *src1_row,
				      PixelRow *src2_row,
				      PixelRow *dest_row,
				      PixelRow *mask_row,
				      gfloat opac,
				      gint      *affect
				      );

/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void
combine_inten_a_and_inten_row_u16 (
				      PixelRow *src1_row,
				      PixelRow *src2_row,
				      PixelRow *dest_row,
				      PixelRow *mask_row,
				      gfloat opac,
				      gint      *affect,
				      gint       mode_affect /* how does the combination mode affect alpha?  */
				      );  

/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void
combine_inten_a_and_inten_a_row_u16 (
					PixelRow *src1_row,
					PixelRow *src2_row,
					PixelRow *dest_row,
					PixelRow *mask_row,
					gfloat opac,
					gint      *affect,
					gint       mode_affect
					);

/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void
combine_inten_a_and_channel_mask_row_u16 (
					    PixelRow *src_row,
					    PixelRow *channel_row,
					    PixelRow *dest_row,
					    PixelRow    *col,
					    gfloat opac
					    );

void
combine_inten_a_and_channel_selection_row_u16 (
						  PixelRow *src_row,
						  PixelRow *channel_row,
						  PixelRow *dest_row,
						  PixelRow    *col,
						  gfloat opac
						 );

/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void
behind_inten_row_u16 (
			PixelRow *src1_row,
			PixelRow *src2_row,
			PixelRow *dest_row,
			PixelRow *mask_row,
			gfloat opac,
			gint      *affect
                        );

/*  paint "behind" the existing pixel row (for indexed images);.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void
behind_indexed_row_u16 (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  gfloat opac,
			  gint     *affect
			  );

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void
replace_inten_row_u16 (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  gfloat opac,
			  gint     *affect
			 );

/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void
replace_indexed_row_u16 (
			    PixelRow *src1_row,
			    PixelRow *src2_row,
			    PixelRow *dest_row,
			    PixelRow *mask_row,
			    gfloat opac,
			    gint     *affect
			   );

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity);
 */
void
erase_inten_row_u16 (
			PixelRow *src1_row,
			PixelRow *src2_row,
			PixelRow *dest_row,
			PixelRow *mask_row,
			gfloat opac,
			gint     *affect
			);

/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed);
 */
void
erase_indexed_row_u16 (
			  PixelRow *src1_row,
			  PixelRow *src2_row,
			  PixelRow *dest_row,
			  PixelRow *mask_row,
			  gfloat opac,
			  gint     *affect
			 );

/*  extract information from intensity pixels based on
 *  a mask.
 */
void
extract_from_inten_row_u16 (
			      PixelRow *src_row,
			      PixelRow *dest_row,
			      PixelRow *mask_row,
			      PixelRow    *background,
			      gint      cut
			      );

/*  extract information from indexed pixels based on
 *  a mask.
 */
void
extract_from_indexed_row_u16 (
				PixelRow *src_row,
				PixelRow *dest_row,
				PixelRow *mask_row,
				unsigned char  *cmap,
				PixelRow *background,
				gint      cut
				);
#endif  
