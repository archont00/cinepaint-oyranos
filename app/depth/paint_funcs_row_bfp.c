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
#include <string.h>
#include <math.h>
#include "../appenv.h"
#include "float16.h"
#include "bfp.h"
#include "../paint_funcs_area.h"
#include "paint_funcs_row_bfp.h"
#include "../pixelrow.h"
#include "../tag.h"

#define EPSILON            0.0001
#define OPAQUE_BFP       32767
#define TRANSPARENT_BFP  0 
#define HALF_OPAQUE_BFP  32767 

/* Make sure this is used with a, b, t 32bit unsigned ints */
#define INT_MULT_16(a,b,t) ((t) = (a) * (b) + 0x8000, ((((t) >> 16) + (t)) >> 16))

static guint16 no_mask = OPAQUE_BFP;


void 
x_add_row_bfp  (
                PixelRow * src_row,
                PixelRow * dest_row
                )
{
  gint     b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint     num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint     width        = pixelrow_width (dest_row);  

  while (width--)
    {
      for (b = 0; b < num_channels; b++)
        dest[b] = MIN (65535, src[b] + dest[b]);

      src += num_channels;
      dest += num_channels;
    }
}


void 
x_sub_row_bfp  (
                PixelRow * src_row,
                PixelRow * dest_row
                )
{
  gint     b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint     num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint     width        = pixelrow_width (dest_row);  

  while (width--)
    {
      for (b = 0; b < num_channels; b++)
        dest[b] = MAX (0, dest[b] - src[b]);

      src += num_channels;
      dest += num_channels;
    }
}


void 
x_min_row_bfp  (
                PixelRow * src_row,
                PixelRow * dest_row
                )
{
  gint     b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint     num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint     width        = pixelrow_width (dest_row);  

  while (width--)
    {
      for (b = 0; b < num_channels; b++)
        dest[b] = MIN (src[b], dest[b]);

      src += num_channels;
      dest += num_channels;
    }
}


void 
invert_row_bfp  (
                 PixelRow * dest_row,
                 PixelRow * mask_row
                 )
{
  gint    b;
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  /*guint16 *mask         = (guint16*) pixelrow_data (mask_row);*/
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (dest_row);  

  while (width--)
    {
      for (b = 0; b < num_channels; b++)
        dest[b] = ONE_BFP - CLAMP(dest[b], 0, ONE_BFP);

      dest += num_channels;
    }
}


void 
absdiff_row_bfp  (
                  PixelRow * image,
                  PixelRow * mask,
                  PixelRow * col,
                  gfloat threshold,
                  int antialias
                  )
{
  guint16 *src           = (guint16*) pixelrow_data (image);
  guint16 *dest          = (guint16*) pixelrow_data (mask);
  guint16 *color         = (guint16*) pixelrow_data (col);

  /*Tag     tag           = pixelrow_tag (image);*/
  /*int     has_alpha     = (tag_alpha (tag) == ALPHA_YES) ? 1 : 0;*/

  gint    width         = pixelrow_width (image);  

  gint    src_channels  = tag_num_channels (pixelrow_tag (image));
  gint    dest_channels = tag_num_channels (pixelrow_tag (mask));

  threshold *= 65535;
  
  while (width--)
    {
      gint b;
      gint diff;
      gint max = 0;
          
      for (b = 0; b < src_channels; b++)
        {
          diff = src[b] - color[b];
          diff = abs (diff);
          if (diff > max)
            max = diff;
        }
      
      if (antialias && threshold > 0)
        {
          float aa;

          aa = 1.5 - ((float) max / threshold);
          if (aa <= 0)
            *dest = 0;
          else if (aa < 0.5)
            *dest = (aa * ONE_BFP * 2);
          else
            *dest = ONE_BFP;
        }
      else
        {
          if (max > threshold)
            *dest = 0;
          else
            *dest = ONE_BFP;
        }
          
      src += src_channels;
      dest += dest_channels;
    }
}


void
extract_channel_row_bfp ( 
			PixelRow *src_row,
			PixelRow *dest_row, 
			gint channel
	)
{
  gint i;
  Tag src_tag = pixelrow_tag (src_row);
  gint num_channels = tag_num_channels (src_tag);
  gint width = pixelrow_width (dest_row);
  guint16 * src = (guint16*) pixelrow_data (src_row);
  guint16 * dest = (guint16*) pixelrow_data (dest_row);
 
  for (i = 0; i < width; i++)
     *dest++ = src[ i * num_channels + channel];
}	

void 
color_row_bfp  (
               void * dest,
               void * color,
               guint width,
               guint height,
               guint pixelstride,
               guint rowstride
               )
{
  gint b, w;
  guint16 * c, *d;
  guchar * dest_ptr = (guchar *)dest; 

  c = (guint16*)color;
  while (height--)
    {
      d = (guint16*)dest_ptr;  
      w = width;

      while (w--)
        {
          b = pixelstride;
          while (b--)
            {
              d[b] = c[b];
            }

          d += pixelstride;
        }
      dest_ptr += rowstride;
    }
}

void
blend_row_bfp (
              PixelRow *src1_row,
	      PixelRow *src2_row,
	      PixelRow *dest_row,
	      gfloat blend,
	      gint alpha_unused
              )
/* BUG: alpha renamed to alpha_unused because in VC++ redefinition (hiding) alpha (see below)
   is a compile error. rsr */
{
  gint     alpha, b;
  Tag      src1_tag     = pixelrow_tag (src1_row); 
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint     width        = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint     has_alpha    = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint     num_channels = tag_num_channels (src1_tag);

  guint16  bl_comp   = (1.0 - CLAMP(blend, 0.0, 1.0)) * OPAQUE_BFP;
  guint16  bl   = CLAMP(blend, 0.0, 1.0) * OPAQUE_BFP;
  
  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src1[b] * bl_comp + src2[b] * bl) >> 15;

      if (has_alpha)
	dest[alpha] = src1[alpha];  /*  alpha channel--assume src2 has none  */

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


void
shade_row_bfp (
		 PixelRow *src_row,
	         PixelRow *dest_row,
	         PixelRow *color,
		 gfloat blend
	         )
{
  gint     alpha, b;
  Tag      src_tag      = pixelrow_tag (src_row); 
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  gint     width        = pixelrow_width (dest_row);
  gint     num_channels = tag_num_channels (src_tag);
  gint     has_alpha    = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16  bl_comp      = (1.0 - CLAMP(blend,0.0,1.0)) * OPAQUE_BFP;
  guint16  bl           =  CLAMP(blend,0.0,1.0) * OPAQUE_BFP;
  guint16 *col          = (guint16*) pixelrow_data (color);

  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = (src[b] * bl_comp + col[b] * bl) >> 15;

      if (has_alpha)
	dest[alpha] = src[alpha];  /* alpha channel */

      src += num_channels;
      dest += num_channels;
    }
}


void
extract_alpha_row_bfp (
		      PixelRow *src_row,
		      PixelRow *mask_row,
		      PixelRow *dest_row
		      )
{
  gint     alpha;
  guint16 *m;
  Tag      src_tag      = pixelrow_tag (src_row); 
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint     width        = pixelrow_width (dest_row);
  gint     num_channels = tag_num_channels (src_tag);

  if (mask)
    m = mask;
  else
    m = &no_mask;   // no_mask is set to OPAQUE_BFP

  alpha = num_channels - 1;
  while (width --)
    {
      *dest++ = (src[alpha] * *m) >> 15; 

      if (mask)
	m++;
      src += num_channels;
    }
}


void
darken_row_bfp (
	 	PixelRow *src1_row,
		PixelRow *src2_row,
		PixelRow *dest_row
	       )
{
  gint b, alpha;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width--)
    {
      for (b = 0; b < alpha; b++)
	{
	  dest[b] = (src1[b] < src2[b]) ? src1[b] : src2[b];
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
lighten_row_bfp (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
                   PixelRow *dest_row
		   )
{
  gint b, alpha;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  
  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width--)
    {
      for (b = 0; b < alpha; b++)
	{
	  dest[b] = (src1[b] < src2[b]) ? src2[b] : src1[b];
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
hsv_only_row_bfp (
          PixelRow *src1_row,
          PixelRow *src2_row,
          PixelRow *dest_row,
          int       mode
          )
{
  gfloat r1, g1, b1;
  gfloat r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  /*  assumes inputs are only 4 channel RGBA pixels  */
  while (width--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hsv ((float*)&r1, (float*)&g1, (float*)&b1);
      rgb_to_hsv ((float*)&r2, (float*)&g2, (float*)&b2);

      switch (mode)
   {
   case HUE_MODE:
     r1 = r2;
     break;
   case SATURATION_MODE:
     g1 = g2;
     break;
   case VALUE_MODE:
     b1 = b2;
     break;
   }

      /*  set the destination  */
      hsv_to_rgb ((float*)&r1, (float*)&g1, (float*)&b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (ha1 && ha2)
   dest[3] = MIN (src1[3], src2[3]);
      else if (ha2)
   dest[3] = src2[3];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
color_only_row_bfp (
            PixelRow *src1_row,
            PixelRow *src2_row,
            PixelRow *dest_row,
            int       mode
           )
{
  gint r1, g1, b1;
  gint r2, g2, b2;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  /*  assumes inputs are only 4 channel RGBA pixels  */
  while (width--)
    {
      r1 = src1[0]; g1 = src1[1]; b1 = src1[2];
      r2 = src2[0]; g2 = src2[1]; b2 = src2[2];
      rgb_to_hls (&r1, &g1, &b1);
      rgb_to_hls (&r2, &g2, &b2);

      /*  transfer hue and saturation to the source pixel  */
      r1 = r2;
      b1 = b2;

      /*  set the destination  */
      hls_to_rgb (&r1, &g1, &b1);

      dest[0] = r1; dest[1] = g1; dest[2] = b1;

      if (ha1 && ha2)
   dest[3] = MIN (src1[3], src2[3]);
      else if (ha2)
   dest[3] = src2[3];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
multiply_row_bfp (
		 PixelRow *src1_row,
		 PixelRow *src2_row,
		 PixelRow *dest_row
		 )
{
  gint alpha, b;
  guint32 t;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1  || ha2 ) ? 
	MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = INT_MULT_16 (src1[b], src2[b], t) ;

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
screen_row_bfp (
		  PixelRow *src1_row,
		  PixelRow *src2_row,
		  PixelRow *dest_row
	          )
{
  gint alpha, b;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = ONE_BFP - 
                  ((ONE_BFP - CLAMP(src1[b],0,ONE_BFP)) * (ONE_BFP - CLAMP(src2[b],0,ONE_BFP)) >> 15);

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
overlay_row_bfp (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row
		   )
{
  gint alpha, b;
  guint32 screen, mult;
  guint32 s;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
          screen = ONE_BFP - 
	    ((ONE_BFP - CLAMP(src1[b],0,ONE_BFP)) * (ONE_BFP - CLAMP(src2[b],0,ONE_BFP)) >> 15);
	  mult = INT_MULT_16 (src1[b], src2[b], s); 
	  dest[b] = INT_MULT_16 (screen , src1[b], s) + ((mult * (ONE_BFP - CLAMP(src1[b],0,ONE_BFP))) >> 15);
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
add_row_bfp ( 
	       PixelRow *src1_row,
	       PixelRow *src2_row,
	       PixelRow *dest_row
	      )
{
  gint alpha, b;
  guint32 sum;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  sum = src1[b] + src2[b];
	  dest[b] = CLAMP(sum, 0, BFP_MAX);
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
subtract_row_bfp (
		    PixelRow *src1_row,
		    PixelRow *src2_row,
		    PixelRow *dest_row
		    )
{
  gint alpha, b;
  gint32 diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = MIN(0, diff);
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void
difference_row_bfp (
		      PixelRow *src1_row,
		      PixelRow *src2_row,
		      PixelRow *dest_row
		      )
{
  gint alpha, b;
  gint32 diff;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  gint    width         = MIN(pixelrow_width (dest_row),pixelrow_width (src1_row));
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);

  alpha = (ha1 || ha2) ? MAXIMUM (num_channels1, num_channels2) - 1 : num_channels1;

  while (width --)
    {
      for (b = 0; b < alpha; b++)
	{
	  diff = src1[b] - src2[b];
	  dest[b] = (diff < 0) ? -diff : diff;
	}

      if (ha1 && ha2)
	dest[alpha] = MIN (src1[alpha], src2[alpha]);
      else if (ha2)
	dest[alpha] = src2[alpha];

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
    }
}


void 
dissolve_row_bfp  (
                   PixelRow * src_row,
                   PixelRow * dest_row,
                   int x,
                   int y,
                   gfloat opacity
                   )
{
  gint alpha, b;
  gint rand_val;
  gint rand_opac           = CLAMP(opacity,0.0,1.0) * 65535;
  Tag     src_tag          = pixelrow_tag (src_row); 
  Tag     dest_tag         = pixelrow_tag (dest_row); 
  gint    has_alpha        = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *dest             = (guint16*)pixelrow_data (dest_row);
  guint16 *src              = (guint16*)pixelrow_data (src_row);
  gint    width            = pixelrow_width (dest_row);
  gint    dest_num_channels = tag_num_channels (dest_tag);
  gint    src_num_channels = tag_num_channels (src_tag);

  alpha = dest_num_channels - 1;

  while (width --)
    {
      /*  preserve the intensity values  */
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      /*  dissolve if random value is > opacity  */
      rand_val = (((int)drand48()) & 0xFFFF);

      if (has_alpha)
	dest[alpha] = (rand_val > rand_opac) ? 0 : src[alpha];
      else
	dest[alpha] = (rand_val > rand_opac) ? 0 : OPAQUE_BFP;

      dest += dest_num_channels;
      src += src_num_channels;
    }
}


void
replace_row_bfp (
		   PixelRow *src1_row,
		   PixelRow *src2_row,
		   PixelRow *dest_row,
		   PixelRow *mask_row,
		   gfloat opacity,
		   int      *affect
		   )
{
  gint alpha;
  gint b;
  double a_val, a_recip, mask_val;
  gint32 s1_a, s2_a;
  gint32 new_val;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (dest_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  if (num_channels1 != num_channels2)
    {
      g_warning ("replace_row_bfp num_channels different");
      return;
    }

  alpha = num_channels1 - 1;

  while (width --)
    {
      mask_val = (CLAMP(mask[0],0,OPAQUE_BFP) * opac) >>15; 
      /* calculate new alpha first. */
      s1_a = CLAMP(src1[alpha], 0, OPAQUE_BFP);
      s2_a = CLAMP(src2[alpha], 0, OPAQUE_BFP);
      a_val = s1_a + mask_val * (s2_a - s1_a);
      if (a_val == 0)
	a_recip = 0;
      else
	a_recip = 1.0 / a_val;
      /* possible optimization: fold a_recip into s1_a and s2_a */
      for (b = 0; b < alpha; b++)
	{
	  new_val = 0.5 + a_recip * ((gdouble)src1[b] * s1_a + mask_val *
				     ((gdouble)src2[b] * s2_a - src1[b] * s1_a));
	  dest[b] = affect[b] ? MIN (new_val, 65535) : src1[b];
	}
      dest[alpha] = affect[alpha] ? a_val + 0.5: s1_a;
      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels2;
      mask++;
    }
}

void
swap_row_bfp (
                PixelRow *src_row,
	        PixelRow *dest_row
	        )
{
  guint16 *dest = (guint16*)pixelrow_data (dest_row);
  guint16 *src  = (guint16*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row) * tag_num_channels (pixelrow_tag (src_row));

  while (width--)
    {
      *src = *src ^ *dest;
      *dest = *dest ^ *src;
      *src = *src ^ *dest;
      src++;
      dest++;
    }
}


void
scale_row_bfp (
		 PixelRow *src_row,
		 PixelRow *dest_row,
		 gfloat       scale
		  )
{
  guint32 t;
  guint16 *dest  = (guint16*)pixelrow_data (dest_row);
  guint16 *src   = (guint16*)pixelrow_data (src_row);
  gint    width = pixelrow_width (dest_row);
  guint32 scale_int = scale * 65535;
  
  while (width --)
    *dest++ = INT_MULT_16 (*src++ , scale_int, t);
}


void
add_alpha_row_bfp (
		  PixelRow *src_row,
		  PixelRow *dest_row
		  )
{
  gint alpha, b;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);

  alpha = num_channels + 1;
  while (width --)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = src[b];

      dest[b] = OPAQUE_BFP;

      src += num_channels;
      dest += alpha;
    }
}


void
flatten_row_bfp (
		   PixelRow *src_row,
		   PixelRow *dest_row,
 		   PixelRow *background
		  )
{
  gint alpha, b;
  guint16 alpha_val;
  Tag     src_tag      = pixelrow_tag (src_row); 
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  gint    width        = pixelrow_width (dest_row);
  gint    num_channels = tag_num_channels (src_tag);
  guint16 *bg           = (guint16*) pixelrow_data (background);

  alpha = num_channels - 1;
  while (width --)
  {
    alpha_val = CLAMP(src[alpha], 0, OPAQUE_BFP);
    for (b = 0; b < alpha; b++)
      dest[b] = ((src[b] * alpha_val) + (bg[b] * (OPAQUE_BFP - alpha_val))) >> 15;

    src += num_channels;
    dest += alpha;
  }
}


void
multiply_alpha_row_bfp( 
		      PixelRow * src_row
		     )
{
  gint  b;
  guint16 alpha_val;
  guint16 *src          =(guint16*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  while (width --)
    {
      alpha_val = CLAMP(src[num_channels-1], 0, OPAQUE_BFP);
      for (b = 0; b < num_channels - 1; b++)
	src[b] = (alpha_val * src[b]) >> 15; 
      src += num_channels;
    }
}


void separate_alpha_row_bfp( 
			    PixelRow *src_row
                           )
{
  gint  b;
  gdouble alpha_recip;
  gint new_val;
  guint16 alpha_val;
  int alpha;
  guint16 *src          =(guint16*)pixelrow_data (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  
  alpha = num_channels-1;
  while( width --)
    {
      alpha_val = CLAMP(src[alpha], 0, OPAQUE_BFP);
      if (alpha_val != 0 && alpha_val != OPAQUE_BFP)
	{
	  alpha_recip = 32768.0 / alpha_val;  // 32768.0 is OPAQUE_BFP in float
	  for (b = 0; b < num_channels - 1; b++)
	    {
	      new_val = 0.5 + src[b] * alpha_recip;
	      new_val = MIN (new_val, 65535);
	      src[b] = new_val;
	    }
	}
      src += num_channels;
    }
} 


void
gray_to_rgb_row_bfp (
		       PixelRow *src_row,
		       PixelRow *dest_row
		       )

{
  gint b;
  gint dest_num_channels;
  gint has_alpha;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);

  has_alpha = (num_channels == 2) ? 1 : 0;
  dest_num_channels = (has_alpha) ? 4 : 3;

  while (width --)
    {
      for (b = 0; b < num_channels; b++)
	dest[b] = src[0];

      if (has_alpha)
	dest[3] = src[1];

      src += num_channels;
      dest += dest_num_channels;
    }
}


/*  apply the mask data to the alpha channel of the pixel data  */
void 
apply_mask_to_alpha_channel_row_bfp  (
                                      PixelRow * src_row,
                                      PixelRow * mask_row,
                                      gfloat opacity
                                      )
{
  gint alpha;
  guint16 mask_val;
  guint32 t;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 opac         = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  alpha = num_channels - 1;
  while (width --)
    {
      mask_val = (*mask++ * opac) >> 15;
      src[alpha] = (src[alpha] * mask_val) >> 15;
      src += num_channels;
    }
}


/*  combine the mask data with the alpha channel of the pixel data  */
void 
combine_mask_and_alpha_channel_row_bfp  (
                                         PixelRow * src_row,
                                         PixelRow * mask_row,
                                         gfloat opacity
                                         )
{
  guint16 mask_val;
  gint alpha;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  gint    width        = pixelrow_width (src_row);
  guint16 opac         = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  alpha = num_channels - 1;
  while (width --)
    {
      mask_val = (CLAMP(*mask++,0,OPAQUE_BFP) * opac) >> 15;
      src[alpha] = src[alpha] + (((OPAQUE_BFP - CLAMP(src[alpha], 0, OPAQUE_BFP)) * mask_val) >> 15);
      src += num_channels;
    }
}


/*  copy gray pixels to intensity-alpha pixels.  This function
 *  essentially takes a source that is only a grayscale image and
 *  copies it to the destination, expanding to RGB if necessary and
 *  adding an alpha channel.  (OPAQUE)
 */
void
copy_gray_to_inten_a_row_bfp (
				PixelRow *src_row,
				PixelRow *dest_row
			        )
{
  gint b;
  gint alpha;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = *src;
      dest[b] = OPAQUE_BFP;

      src ++;
      dest += num_channels;
    }
}


/*  lay down the initial pixels in the case of only one
 *  channel being visible and no layers...In this singular
 *  case, we want to display a grayscale image w/o transparency
 */
void
initial_channel_row_bfp (
			   PixelRow *src_row,
			   PixelRow *dest_row
			  )
{
  gint alpha, b;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (dest_row));
  gint    width        = pixelrow_width (src_row);

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[0];

      dest[alpha] = OPAQUE_BFP;

      dest += num_channels;
      src ++;
    }
}


/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void
initial_indexed_row_bfp (
			    PixelRow *src_row,
			    PixelRow *dest_row,
			    unsigned char *cmap
			   )
{
  gint col_index;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  gint    width        = pixelrow_width (src_row);

  /*  This function assumes always that we're mapping from
   *  an RGB colormap to an RGBA image...
   */
  while (width--)
    {
      col_index = *src++ * 3;
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = OPAQUE_BFP;
    }
}


/*  lay down the initial pixels in the case of an indexed image.
 *  This process obviously requires no composition
 */
void 
initial_indexed_a_row_bfp  (
                            PixelRow * src_row,
                            PixelRow * dest_row,
                            PixelRow * mask_row,
                            unsigned char * cmap,
                            gfloat opacity
                            )
{
  gint col_index;
  guint32 t;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);
  guint16 opac = opacity * 65535;  
  if (mask)
    m = mask;
  else
    m = &no_mask;   // no_mask is OPAQUE_BFP

  while (width --)
    {
      col_index = *src++ * 3;
      mask_val = INT_MULT_16 (*m, opac, t );
      new_alpha = INT_MULT_16 (*src++ , mask_val, t)  ;

      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      *dest++ = cmap[col_index++];
      /*  Set the alpha channel  */
      *dest++ = (new_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : TRANSPARENT_BFP;

      if (mask)
	m++;
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_row_bfp  (
                        PixelRow * src_row,
                        PixelRow * dest_row,
                        PixelRow * mask_row,
                        gfloat opacity,
                        int * affect
                        )
{
  gint b, dest_num_channels;
  guint16 * m;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  guint16 opac = OPAQUE_BFP * CLAMP(opacity, 0.0, 1.0);

  if (mask)
    m = mask;
  else
    m = &no_mask;  // no_mask is OPAQUE_BFP

  /*  This function assumes the source has no alpha channel and
   *  the destination has an alpha channel.  So dest_num_channels = num_channels + 1
   */
  dest_num_channels = num_channels + 1;

  if (mask)
    {
      while (width --)
	{
	  for (b = 0; b < num_channels; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? (CLAMP(*m, 0, OPAQUE_BFP) * opac) >> 15 : 0;
	    
	  m++;
	  dest += dest_num_channels;
	  src += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  for (b = 0; b < num_channels; b++)
	    dest [b] = affect [b] ? src [b] : 0;
	    
	  /*  Set the alpha channel  */
	  dest[b] = affect [b] ? opac : 0;
	    
	  dest += dest_num_channels;
	  src += num_channels;
	}
    }
}


/*  lay down the initial pixels for the base layer.
 *  This process obviously requires no composition.
 */
void 
initial_inten_a_row_bfp  (
                          PixelRow * src_row,
                          PixelRow * dest_row,
                          PixelRow * mask_row,
                          gfloat opacity,
                          int * affect
                          )
{
  gint alpha, b;
  guint16 * m;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src_row));
  guint16 opac = CLAMP(opacity,0.0,1.0) * OPAQUE_BFP;

  alpha = num_channels - 1;
  if (mask)
    {
      m = mask;
      while (width --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? 
            ((src[alpha] * opac * CLAMP(*m,0,OPAQUE_BFP)) >> 15) : 0;
	  
	  m++;
	  
	  dest += num_channels;
	  src += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  for (b = 0; b < alpha; b++)
	    dest[b] = src[b] * affect[b];
	  
	  /*  Set the alpha channel  */
	  dest[alpha] = affect [alpha] ? ((opac* src[alpha]) >> 15) : 0;
	  
	  dest += num_channels;
	  src += num_channels;
	}
    }
}


/*  combine indexed images with an optional mask which
 *  is interpreted as binary...destination is indexed...
 */
void 
combine_indexed_and_indexed_row_bfp  (
                                      PixelRow * src1_row,
                                      PixelRow * src2_row,
                                      PixelRow * dest_row,
                                      PixelRow * mask_row,
                                      gfloat opacity,
                                      int * affect
                                      )
{
  gint b;
  guint16 new_alpha;
  guint16 * m;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = CLAMP(opacity,0.0,1.0) * OPAQUE_BFP;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = CLAMP(*m,0.0,1.0) * opacity;
	  
	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP) ? src2[b] : src1[b];
	  
	  m++;
	  
	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = opac;
	  
	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP) ? src2[b] : src1[b];
	  
	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine indexed images with indexed-alpha images
 *  result is an indexed image
 */
void 
combine_indexed_and_indexed_a_row_bfp  (
                                        PixelRow * src1_row,
                                        PixelRow * src2_row,
                                        PixelRow * dest_row,
                                        PixelRow * mask_row,
                                        gfloat opacity,
                                        int * affect
                                        )
{
  gint b, alpha;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  gint src2_num_channels;
  guint32 t;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = opacity * 65535;

  alpha = 1;
  src2_num_channels = 2;

  if (mask)
    {
      m = mask;
      while (width --)
	{
          mask_val = INT_MULT_16 (*m , opac, t);
	  new_alpha = INT_MULT_16 (src2[alpha] , mask_val, t);

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP ) ? src2[b] : src1[b];

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = INT_MULT_16 (src2[alpha] , opac, t);

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP) ? src2[b] : src1[b];

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine indexed-alpha images with indexed-alpha images
 *  result is an indexed-alpha image.  use this for painting
 *  to an indexed floating sel
 */
void 
combine_indexed_a_and_indexed_a_row_bfp  (
                                          PixelRow * src1_row,
                                          PixelRow * src2_row,
                                          PixelRow * dest_row,
                                          PixelRow * mask_row,
                                          gfloat opacity,
                                          int * affect
                                          )
{
  gint b, alpha;
  guint16 new_alpha;
  guint16 * m;
  guint32 t;
  guint16 mask_val;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = opacity * 65535;

  alpha = 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  mask_val = INT_MULT_16 (*m , opac, t);
	  new_alpha = INT_MULT_16 (src2[alpha] , mask_val, t);

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : src1[alpha];

	  m++;

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = INT_MULT_16 (src2[alpha] , opac, t) ; 

	  for (b = 0; b < alpha; b++)
	    dest[b] = (affect[b] && new_alpha > HALF_OPAQUE_BFP) ? src2[b] : src1[b];

	  dest[alpha] = (affect[alpha] && new_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : src1[alpha];

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine intensity with indexed, destination is
 *  intensity-alpha...use this for an indexed floating sel
 */
void 
combine_inten_a_and_indexed_a_row_bfp  (
                                        PixelRow * src1_row,
                                        PixelRow * src2_row,
                                        PixelRow * dest_row,
                                        PixelRow * mask_row,
                                        unsigned char * cmap,
                                        gfloat opacity
                                        )
{
  gint b, alpha;
  guint16 new_alpha;
  guint16 mask_val;
  gint src2_num_channels;
  gint index;
  guint32 t;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = opacity * 65535;

  alpha = 1;
  src2_num_channels = 2;

  if (mask)
    {
      guint16 *m = mask;
      while (width --)
	{
	  mask_val = INT_MULT_16 (*m, opac, t);
	  new_alpha =INT_MULT_16 (src2[alpha] , mask_val, t) ;

	  index = src2[0] * 3;

	  for (b = 0; b < num_channels-1; b++)
	    dest[b] = (new_alpha > HALF_OPAQUE_BFP) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : src1[b];  /*  alpha channel is opaque  */

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = INT_MULT_16 (src2[alpha], opac, t);

	  index = src2[0] * 3;

	  for (b = 0; b < num_channels-1; b++)
	    dest[b] = (new_alpha > HALF_OPAQUE_BFP) ? cmap[index + b] : src1[b];

	  dest[b] = (new_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : src1[b];  /*  alpha channel is opaque  */

	  /* m++; /Per */

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine RGB image with RGB or GRAY with GRAY
 *  destination is intensity-only...
 */
void 
combine_inten_and_inten_row_bfp  (
                                  PixelRow * src1_row,
                                  PixelRow * src2_row,
                                  PixelRow * dest_row,
                                  PixelRow * mask_row,
                                  gfloat opacity,
                                  int * affect
                                  )
{
  gint b;
  guint16 new_alpha;
  guint16 * m;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = CLAMP(opacity,0.0,1.0) * OPAQUE_BFP;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  new_alpha = (CLAMP(*m,0,OPAQUE_BFP) * opac) >> 15;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ? 
              ((src2[b] * new_alpha) + (src1[b] * (OPAQUE_BFP - new_alpha))) >>15 :
	    src1[b];

	  m++;

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ? 
	      (src2[b] * opac + src1[b] * (OPAQUE_BFP - opac)) >> 15:
	    src1[b];

	  src1 += num_channels;
	  src2 += num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine an RGBA or GRAYA image with an RGB or GRAY image
 *  destination is intensity-only...
 */
void 
combine_inten_and_inten_a_row_bfp  (
                                    PixelRow * src1_row,
                                    PixelRow * src2_row,
                                    PixelRow * dest_row,
                                    PixelRow * mask_row,
                                    gfloat opacity,
                                    int * affect
                                    )
{
  gint alpha, b;
  gint src2_num_channels;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  alpha = num_channels;
  src2_num_channels = num_channels + 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
          mask_val = (CLAMP(*m,0,OPAQUE_BFP) * opac) >> 15;
	  new_alpha = (CLAMP(src2[alpha],0,OPAQUE_BFP) * mask_val) >> 15;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * new_alpha + src1[b] * (OPAQUE_BFP - new_alpha)) >> 15 :
	      src1[b];

	  m++;
	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  new_alpha = (CLAMP(src2[alpha],0,OPAQUE_BFP) * opac) >> 15;

	  for (b = 0; b < num_channels; b++)
	    dest[b] = (affect[b]) ?
	      (src2[b] * opac + src1[b] * (OPAQUE_BFP - opac)) >> 15:
	      src1[b];

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}

#define alphify(src2_alpha,new_alpha) \
	if (new_alpha == 0 || src2_alpha == 0)							\
	  {											\
	    for (b = 0; b < alpha; b++)								\
	      dest[b] = src1 [b];								\
	  }											\
	else if (src2_alpha == new_alpha){							\
	  for (b = 0; b < alpha; b++)								\
	    dest [b] = affect [b] ? src2 [b] : src1 [b];					\
	} else {										\
	  ratio = (float) src2_alpha / new_alpha;						\
	  compl_ratio = 1.0 - ratio;								\
	  											\
	  for (b = 0; b < alpha; b++)								\
	    dest[b] = affect[b] ?								\
	      (guint16) (src2[b] * ratio + src1[b] * compl_ratio + EPSILON) : src1[b];	\
	}

	
/*  combine an RGB or GRAY image with an RGBA or GRAYA image
 *  destination is intensity-alpha...
 */
void 
combine_inten_a_and_inten_row_bfp  (
                                    PixelRow * src1_row,
                                    PixelRow * src2_row,
                                    PixelRow * dest_row,
                                    PixelRow * mask_row,
                                    gfloat opacity,
                                    int * affect,
                                    int mode_affect
                                    )
{
  gint alpha, b;
  gint src2_num_channels;
  guint16 src2_alpha, src1_alpha;
  guint16 new_alpha;
  guint16 * m;
  gfloat ratio, compl_ratio;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = CLAMP(opacity,0.0,1.0) * OPAQUE_BFP;


  src2_num_channels = num_channels - 1;
  alpha = num_channels - 1;

  if (mask)
    {
      m = mask;
      while (width --)
	{
	  src1_alpha = CLAMP(src1[alpha],0,OPAQUE_BFP);
	  src2_alpha = (CLAMP(*m, 0, OPAQUE_BFP) *  opac) >> 15 ;
	  new_alpha = src1_alpha + (((OPAQUE_BFP - src1_alpha) * src2_alpha) >> 15);
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	  else
	    dest[alpha] = (src1_alpha) ? src1_alpha : (affect[alpha] ? new_alpha : src1_alpha);

	  m++;

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
  else
    {
      while (width --)
	{
	  src1_alpha = CLAMP(src1[alpha],0,OPAQUE_BFP);
	  src2_alpha = opac;
	  new_alpha = src1_alpha + (((OPAQUE_BFP - src1_alpha) * src2_alpha) >> 15);
	  alphify (src2_alpha, new_alpha);
	  
	  if (mode_affect)
	    dest[alpha] = (affect[alpha]) ? new_alpha : src1_alpha;
	  else
	    dest[alpha] = (src1_alpha) ? src1_alpha : (affect[alpha] ? new_alpha : src1_alpha);

	  src1 += num_channels;
	  src2 += src2_num_channels;
	  dest += num_channels;
	}
    }
}


/*  combine an RGBA or GRAYA image with an RGBA or GRAYA image
 *  destination is of course intensity-alpha...
 */
void 
combine_inten_a_and_inten_a_row_bfp  (
                                      PixelRow * src1_row,
                                      PixelRow * src2_row,
                                      PixelRow * dest_row,
                                      PixelRow * mask_row,
                                      gfloat opacity,
                                      int * affect,
                                      int mode_affect
                                      )
{
  gint alpha, b;
  guint16 src2_alpha, src1_alpha;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  float ratio, compl_ratio;
  guint16 *src1         = (guint16*)pixelrow_data (src1_row);
  guint16 *src2         = (guint16*)pixelrow_data (src2_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *mask         = (guint16*)pixelrow_data (mask_row);
  gint    width        = pixelrow_width (src1_row);
  gint    num_channels = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  alpha = num_channels - 1;
  if (mask){
    m = mask;
    while (width --)
      {
	src1_alpha = CLAMP(src1[alpha],0,OPAQUE_BFP);
    mask_val = (CLAMP(*m, 0, OPAQUE_BFP) * opac) >> 15;
	src2_alpha = (CLAMP(src2[alpha], 0, OPAQUE_BFP) * mask_val) >> 15;
	new_alpha = src1_alpha + (((OPAQUE_BFP - src1_alpha) * src2_alpha) >> 15);

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1_alpha;
	else
	  dest[alpha] = (src1_alpha) ? src1_alpha : (affect[alpha] ? new_alpha : src1_alpha);
	
	m++;
	
	src1 += num_channels;
	src2 += num_channels;
	dest += num_channels;
      }
  } else {
    while (width --)
      {
        src1_alpha = CLAMP(src1[alpha],0,OPAQUE_BFP);
	src2_alpha = (CLAMP(src2[alpha], 0, OPAQUE_BFP) * opac) >> 15;
	new_alpha = src1_alpha + ((OPAQUE_BFP - src1_alpha) >> 15);

	alphify (src2_alpha, new_alpha);
	
	if (mode_affect)
	  dest[alpha] = (affect[alpha]) ? new_alpha : src1[alpha];
	else
	  dest[alpha] = (src1_alpha) ? src1_alpha : (affect[alpha] ? new_alpha : src1_alpha);

	src1 += num_channels;
	src2 += num_channels;
	dest += num_channels;
      }
  }
}
#undef alphify


/*  combine a channel with intensity-alpha pixels based
 *  on some opacity, and a channel color...
 *  destination is intensity-alpha
 */
void 
combine_inten_a_and_channel_mask_row_bfp  (
                                           PixelRow * src_row,
                                           PixelRow * channel_row,
                                           PixelRow * dest_row,
                                           PixelRow * col,
                                           gfloat opacity
                                           )
{
  gint alpha, b;
  guint16 channel_alpha;
  guint16 new_alpha;
  guint16 compl_alpha;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *channel      = (guint16*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  Tag     src_tag      = pixelrow_tag (src_row);
  gint    num_channels = tag_num_channels (src_tag);
  gint    has_alpha     = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *color        = (guint16*) pixelrow_data (col);
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;
 
  if (has_alpha)
    alpha = num_channels - 1;
  else
    alpha = num_channels;
	
  while (width --)
    {
      channel_alpha = ((OPAQUE_BFP - CLAMP(*channel,0,OPAQUE_BFP)) * opac) >> 15;
      if (channel_alpha)
	{
	  if (has_alpha)
	    new_alpha = CLAMP(src[alpha], 0, OPAQUE_BFP) + 
                         (((OPAQUE_BFP - CLAMP(src[alpha], 0, OPAQUE_BFP)) * channel_alpha) >> 15);
	  else
	    new_alpha = OPAQUE_BFP;

	  if (new_alpha != OPAQUE_BFP)
	    channel_alpha = (channel_alpha * OPAQUE_BFP) / new_alpha;
	  compl_alpha = OPAQUE_BFP - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = (color[b] * channel_alpha + src[b] * compl_alpha) >> 15;
	  if (has_alpha)
	    dest[b] = new_alpha;
	}
      else
	for (b = 0; b < num_channels; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=num_channels;
      dest+=num_channels;
      channel++;
    }
}


void
combine_inten_a_and_channel_selection_row_bfp (
						  PixelRow *src_row,
						  PixelRow *channel_row,
						  PixelRow *dest_row,
						  PixelRow *col,
						  gfloat opacity
						 )
{
  gint alpha, b;
  guint16 channel_alpha;
  guint16 new_alpha;
  guint16 compl_alpha;
  guint16 *src          = (guint16*)pixelrow_data (src_row);
  guint16 *dest         = (guint16*)pixelrow_data (dest_row);
  guint16 *channel      = (guint16*)pixelrow_data (channel_row);
  gint    width        = pixelrow_width (src_row);
  Tag     src_tag      = pixelrow_tag (src_row);
  gint    num_channels = tag_num_channels (src_tag);
  gint    has_alpha     = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *color        = (guint16*) pixelrow_data (col);
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  if (has_alpha)
    alpha = num_channels - 1;
  else
    alpha = num_channels;
  while (width --)
    {
      channel_alpha = (CLAMP(*channel,0,OPAQUE_BFP) * opac) >> 15;
      if (channel_alpha)
	{
	  if (has_alpha)
	    new_alpha = CLAMP(src[alpha], 0, OPAQUE_BFP) + 
                         (((OPAQUE_BFP - CLAMP(src[alpha], 0, OPAQUE_BFP)) * channel_alpha) >> 15);
	  else
	    new_alpha = OPAQUE_BFP;

	  if (new_alpha != OPAQUE_BFP)
	    channel_alpha = (channel_alpha * OPAQUE_BFP) / new_alpha;
	  compl_alpha = OPAQUE_BFP - channel_alpha;

	  for (b = 0; b < alpha; b++)
	    dest[b] = (color[b] * channel_alpha + src[b] * compl_alpha) >> 15;
	  if (has_alpha)
	    dest[b] = new_alpha;
	}
      else
	for (b = 0; b < num_channels; b++)
	  dest[b] = src[b];

      /*  advance pointers  */
      src+=num_channels;
      dest+=num_channels;
      channel++;
    }
}


/*  paint "behind" the existing pixel row.
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void 
behind_inten_row_bfp  (
                       PixelRow * src1_row,
                       PixelRow * src2_row,
                       PixelRow * dest_row,
                       PixelRow * mask_row,
                       gfloat opacity,
                       int * affect
                       )
{
  gint alpha, b;
  guint16 src1_alpha;
  guint16 src2_alpha;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (pixelrow_tag (src1_row));
  gint    num_channels2 = tag_num_channels (pixelrow_tag (src2_row));
  guint16 opac = CLAMP(opacity, 0.0, 1.0) * OPAQUE_BFP;

  float ratio, compl_ratio;

  if (mask)
    m = mask;
  else
    m = &no_mask;  // no_mask is OPAQUE_BFP

  /*  the alpha channel  */
  alpha = num_channels1 - 1;

  while (width --)
    {
      src1_alpha = CLAMP(src1[alpha], 0, OPAQUE_BFP);
      mask_val = (CLAMP(*m, 0, OPAQUE_BFP) * opac) >> 15;
      src2_alpha = (CLAMP(src2[alpha], 0, OPAQUE_BFP) * mask_val) >> 15;
      new_alpha = src2_alpha + (((OPAQUE_BFP - src2_alpha) * src1_alpha) >> 15); 
      if (new_alpha)
	ratio = (float) src1_alpha / new_alpha;
      else
	ratio = 0.0;
      compl_ratio = 1.0 - ratio;

      for (b = 0; b < alpha; b++)
	dest[b] = (affect[b]) ?
	  (guint16) (src1[b] * ratio + src2[b] * compl_ratio + EPSILON) :
	src1[b];

      dest[alpha] = (affect[alpha]) ? new_alpha : src1_alpha;

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  paint "behind" the existing pixel row (for indexed images).
 *  This is similar in appearance to painting on a layer below
 *  the existing pixels.
 */
void 
behind_indexed_row_bfp  (
                         PixelRow * src1_row,
                         PixelRow * src2_row,
                         PixelRow * dest_row,
                         PixelRow * mask_row,
                         gfloat opacity,
                         int * affect
                         )
{
  gint alpha, b;
  guint16 src1_alpha;
  guint16 src2_alpha;
  guint16 new_alpha;
  guint16 * m;
  guint16 mask_val;
  guint32 t;
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (pixelrow_tag (src1_row));
  gint    num_channels2 = tag_num_channels (pixelrow_tag (src2_row));
  guint16 opac = opacity * 65535;

  if (mask)
    m = mask;
  else
    m = &no_mask;    // no_mask is OPAQUE_BFP

  /*  the alpha channel  */
  alpha = num_channels1 - 1;

  while (width --)
    {
      src1_alpha = src1[alpha]; 
      mask_val = INT_MULT_16 (*m, opac, t);
      src2_alpha = INT_MULT_16 (src2[alpha], mask_val, t);
      new_alpha = (src2_alpha > HALF_OPAQUE_BFP) ? OPAQUE_BFP : TRANSPARENT_BFP;

      for (b = 0; b < num_channels1; b++)
	dest[b] = (affect[b] && new_alpha == OPAQUE_BFP && (src1_alpha > HALF_OPAQUE_BFP)) ?
	  src2[b] : src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void 
replace_inten_row_bfp  (
                        PixelRow * src1_row,
                        PixelRow * src2_row,
                        PixelRow * dest_row,
                        PixelRow * mask_row,
                        gfloat opacity,
                        int * affect
                        )
{
  gint num_channels, b;
  guint16 mask_alpha;
  guint16 * m;

  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 opac = CLAMP(opacity, 0.0,1.0) * OPAQUE_BFP;

  if (mask)
    m = mask;
  else
    m = &no_mask;     // no_mask is OPAQUE_BFP

  num_channels = MINIMUM (num_channels1, num_channels2);
  while (width --)
    {
      mask_alpha = (CLAMP(*m, 0, OPAQUE_BFP) * opac) >> 15;

      for (b = 0; b < num_channels; b++)
	dest[b] = (affect[b]) ? (src2[b] * mask_alpha + src1[b] * (OPAQUE_BFP - mask_alpha)) >> 15 :
                src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  replace the contents of one pixel row with the other
 *  The operation is still bounded by mask/opacity constraints
 */
void 
replace_indexed_row_bfp  (
                          PixelRow * src1_row,
                          PixelRow * src2_row,
                          PixelRow * dest_row,
                          PixelRow * mask_row,
                          gfloat opacity,
                          int * affect
                          )
{
  gint num_channels, b;
  guint16 mask_alpha;
  guint16 * m;
  Tag     src1_tag      = pixelrow_tag (src1_row); 
  Tag     src2_tag      = pixelrow_tag (src2_row); 
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels1 = tag_num_channels (src1_tag);
  gint    num_channels2 = tag_num_channels (src2_tag);
  gint    ha1           = (tag_alpha (src1_tag)==ALPHA_YES)? TRUE: FALSE;
  gint    ha2           = (tag_alpha (src2_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 opac = opacity * 65535;


  if (mask)
    m = mask;
  else
    m = &no_mask;    // no_mask is OPAQUE_BFP

  num_channels = MINIMUM (num_channels1, num_channels2);
  while (width --)
    {
      mask_alpha = (CLAMP(*m, 0, OPAQUE_BFP) * opac) >> 15;

      for (b = 0; b < num_channels; b++)
	dest[b] = (affect[b] && mask_alpha) ?
	  src2[b] : src1[b];

      if (ha1 && !ha2)
	dest[b] = src1[b];

      if (mask)
	m++;

      src1 += num_channels1;
      src2 += num_channels2;
      dest += num_channels1;
    }
}


/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for intensity)
 */
void 
erase_inten_row_bfp  (
                      PixelRow * src1_row,
                      PixelRow * src2_row,
                      PixelRow * dest_row,
                      PixelRow * mask_row,
                      gfloat opacity,
                      int * affect
                      )
{
  gint alpha, b;
  guint16 src2_alpha;
  guint16 * m;
  guint32 t;
  guint16 mask_val;
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = opacity * 65535;

  if (mask)
    m = mask;
  else
    m = &no_mask;    // no_mask is OPAQUE_BFP

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];
      mask_val = INT_MULT_16 (*m, opac, t);
      src2_alpha = INT_MULT_16 (src2[alpha], mask_val, t);
      dest[alpha] = src1[alpha] - INT_MULT_16(src1[alpha], src2_alpha, t);

      if (mask)
	m++;

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


/*  apply source 2 to source 1, but in a non-additive way,
 *  multiplying alpha channels  (works for indexed)
 */
void 
erase_indexed_row_bfp  (
                        PixelRow * src1_row,
                        PixelRow * src2_row,
                        PixelRow * dest_row,
                        PixelRow * mask_row,
                        gfloat opacity,
                        int * affect
                        )
{
  gint alpha, b;
  guint16 src2_alpha;
  guint16 * m;
  guint16 mask_val;
  guint32 t;
  guint16 *src1          = (guint16*)pixelrow_data (src1_row);
  guint16 *src2          = (guint16*)pixelrow_data (src2_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src1_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src1_row));
  guint16 opac = opacity * 65535;

  if (mask)
    m = mask;
  else
    m = &no_mask;  // no_mask is OPAQUE_BFP

  alpha = num_channels - 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src1[b];
      mask_val = INT_MULT_16 (*m, opac, t);
      src2_alpha = INT_MULT_16 (src2[alpha], mask_val, t);
      dest[alpha] = (src2_alpha > HALF_OPAQUE_BFP) ? TRANSPARENT_BFP : src1[alpha];

      if (mask)
	m++;

      src1 += num_channels;
      src2 += num_channels;
      dest += num_channels;
    }
}


/*  extract information from intensity pixels based on
 *  a mask.
 */
void
extract_from_inten_row_bfp (
			      PixelRow *src_row,
			      PixelRow *dest_row,
			      PixelRow *mask_row,
			      PixelRow *background,
			      int       cut
			      )
{
  gint b, alpha;
  gint dest_num_channels;
  guint16 * m;
  guint16 mask_val;
  Tag     src_tag       = pixelrow_tag (src_row); 
  guint16 *src           = (guint16*)pixelrow_data (src_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src_row);
  gint    num_channels  = tag_num_channels (src_tag);
  gint    has_alpha      = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *bg            = (guint16*) pixelrow_data (background);

  if (mask)
    m = mask;
  else
    m = &no_mask;  // no_mask is OPAQUE_BFP

  alpha = (has_alpha) ? num_channels - 1 : num_channels;
  dest_num_channels = (has_alpha) ? num_channels : num_channels + 1;
  while (width --)
    {
      for (b = 0; b < alpha; b++)
	dest[b] = src[b];

      mask_val = CLAMP(*m, 0, OPAQUE_BFP);
      if (has_alpha)
	{
	  dest[alpha] = (mask_val * src[alpha]) >> 15 ;
	  if (cut)
	    src[alpha] = ((OPAQUE_BFP - mask_val) * src[alpha]) >> 15 ;
	}
      else
	{
	  dest[alpha] = *m;
	  if (cut)
	    for (b = 0; b < num_channels; b++)
	      src[b] = ((OPAQUE_BFP - mask_val) * bg[b]) >> 15;
	}

      if (mask)
	m++;

      src += num_channels;
      dest += dest_num_channels;
    }
}


/*  extract information from indexed pixels based on
 *  a mask.
 */
void
extract_from_indexed_row_bfp (
				PixelRow *src_row,
				PixelRow *dest_row,
				PixelRow *mask_row,
				unsigned char *cmap,
				PixelRow *background,
				int       cut
				)
{
  gint b;
  gint index;
  guint16 * m;
  int t;
  Tag     src_tag       = pixelrow_tag (src_row); 
  guint16 *src           = (guint16*)pixelrow_data (src_row);
  guint16 *dest          = (guint16*)pixelrow_data (dest_row);
  guint16 *mask          = (guint16*)pixelrow_data (mask_row);
  gint    width         = pixelrow_width (src_row);
  gint    num_channels  = tag_num_channels (src_tag);
  gint    has_alpha     = (tag_alpha (src_tag)==ALPHA_YES)? TRUE: FALSE;
  guint16 *bg            = (guint16*) pixelrow_data (background);

  if (mask)
    m = mask;
  else
    m = &no_mask;

  while (width --)
    {
      index = src[0] * 3;
      for (b = 0; b < 3; b++)
	dest[b] = cmap[index + b];

      if (has_alpha)
	{
	  dest[3] = INT_MULT_16 (*m, src[1], t);
	  if (cut)
	    src[1] = INT_MULT_16 (65535 - *m, src[1], t);
	}
      else
	{
	  dest[3] = *m;
	  if (cut)
	    src[0] = (*m > HALF_OPAQUE_BFP ) ? bg[0] : src[0];
	}

      if (mask)
	m++;

      src += num_channels;
      dest += 4;
    }
}


/*******************************************************
                     copy routines
********************************************************/

#define INTENSITY(r,g,b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11 + 0.001)

static void
copy_row_rgb_to_u8_rgb (
                        PixelRow * src_row,
                        PixelRow * dest_row
                        )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = BFP_TO_U8(s[1]);
            d[2] = BFP_TO_U8(s[2]);
            d[3] = BFP_TO_U8(s[3]);
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = BFP_TO_U8(s[1]);
            d[2] = BFP_TO_U8(s[2]);
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = BFP_TO_U8(s[1]);
            d[2] = BFP_TO_U8(s[2]);
            d[3] = 255;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = BFP_TO_U8(s[1]);
            d[2] = BFP_TO_U8(s[2]);
            s += 3;
            d += 3;
          }
    }
}



static void 
copy_row_rgb_to_u8_gray  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U8(s[0]), BFP_TO_U8(s[1]), BFP_TO_U8(s[2]));
            d[1] = BFP_TO_U8(s[3]);
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U8(s[0]), BFP_TO_U8(s[1]), BFP_TO_U8(s[2]));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U8(s[0]), BFP_TO_U8(s[1]), BFP_TO_U8(s[2]));
            d[1] = 255;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U8(s[0]), BFP_TO_U8(s[1]), BFP_TO_U8(s[2]));
            s += 3;
            d += 1;
          }
    }
}

static void 
copy_row_rgb_to_u16_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = BFP_TO_U16(s[1]);
            d[2] = BFP_TO_U16(s[2]);
            d[3] = BFP_TO_U16(s[2]);
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = BFP_TO_U16(s[1]);
            d[2] = BFP_TO_U16(s[2]);
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = BFP_TO_U16(s[1]);
            d[2] = BFP_TO_U16(s[2]);
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = BFP_TO_U16(s[1]);
            d[2] = BFP_TO_U16(s[2]);
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_u16_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U16(s[0]), BFP_TO_U16(s[1]), BFP_TO_U16(s[2]));
            d[1] = BFP_TO_U16(s[3]);
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U16(s[0]), BFP_TO_U16(s[1]), BFP_TO_U16(s[2]));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U16(s[0]), BFP_TO_U16(s[1]), BFP_TO_U16(s[2]));
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_U16(s[0]), BFP_TO_U16(s[1]), BFP_TO_U16(s[2]));
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_rgb_to_float_rgb  (
                            PixelRow * src_row,
                            PixelRow * dest_row
                            )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = BFP_TO_FLOAT(s[1]);
            d[2] = BFP_TO_FLOAT(s[2]);
            d[3] = BFP_TO_FLOAT(s[3]);
            s += 4;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = BFP_TO_FLOAT(s[1]);
            d[2] = BFP_TO_FLOAT(s[2]);
            s += 4;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = BFP_TO_FLOAT(s[1]);
            d[2] = BFP_TO_FLOAT(s[2]);
            d[3] = 1.0;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = BFP_TO_FLOAT(s[1]);
            d[2] = BFP_TO_FLOAT(s[2]);
            s += 3;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_float_gray  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2]));
            d[1] = BFP_TO_FLOAT(s[3]);
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2]));
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2]));
            d[1] = 1.0;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2]));
            s += 3;
            d += 1;
          }
    }
}


/****************************
 * Added to fix conversion
 * from u16 to float16
 * Kevin M. Murray 10/31/2002
 ****************************/
static void copy_row_rgb_to_float16_rgb(PixelRow* src_row, PixelRow * dest_row) {
   int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
   Tag      stag   = pixelrow_tag (src_row);
   Tag      dtag   = pixelrow_tag (dest_row);
   guint16* s    = (guint16*) pixelrow_data (src_row);
   guint16* d    = (guint16*) pixelrow_data (dest_row);
   ShortsFloat u;

   if (tag_alpha (stag) == ALPHA_YES) {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (BFP_TO_FLOAT(s[1]), u);
            d[2] = FLT16 (BFP_TO_FLOAT(s[2]), u);
            d[3] = FLT16 (BFP_TO_FLOAT(s[3]), u);
            s += 4;
            d += 4;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (BFP_TO_FLOAT(s[1]), u);
            d[2] = FLT16 (BFP_TO_FLOAT(s[2]), u);
            s += 4;
            d += 3;
            }
         }
      }
   else {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (BFP_TO_FLOAT(s[1]), u);
            d[2] = FLT16 (BFP_TO_FLOAT(s[2]), u);
            d[3] = FLT16 (1.0, u);
            s += 3;
            d += 4;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (BFP_TO_FLOAT(s[1]), u);
            d[2] = FLT16 (BFP_TO_FLOAT(s[2]), u);
            s += 3;
            d += 3;
            }
         }
      }
   }

/****************************
 * Added to fix conversion
 * from u16 to float16
 * Kevin M. Murray 10/31/2002
 ****************************/
static void copy_row_rgb_to_float16_gray (PixelRow* src_row, PixelRow* dest_row) {
   int      w    = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
   Tag      stag = pixelrow_tag (src_row);
   Tag      dtag = pixelrow_tag (dest_row);
   guint16*  s    = (guint16*) pixelrow_data (src_row);
   guint16* d    = (guint16*) pixelrow_data (dest_row);
   ShortsFloat u;

   if (tag_alpha (stag) == ALPHA_YES) {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2])), u);
            d[1] = FLT16 (BFP_TO_FLOAT(s[3]), u);
            s += 4;
            d += 2;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2])), u);
            s += 4;
            d += 1;
            }
         }
      }
   else {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2])), u);
            d[1] = FLT16 (1.0, u);
            s += 3;
            d += 2;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (INTENSITY (BFP_TO_FLOAT(s[0]), BFP_TO_FLOAT(s[1]), BFP_TO_FLOAT(s[2])), u);
            s += 3;
            d += 1;
            }
         }
      }
   }


static void 
copy_row_rgb_to_bfp_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == tag_alpha (dtag))
    {
      memcpy (d, s, w * tag_bytes (stag));
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = 65535;
            s += 3;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            s += 4;
            d += 3;
          }
    }
}


static void 
copy_row_rgb_to_bfp_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = s[3];
            s += 4;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 4;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            d[1] = 65535;
            s += 3;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = INTENSITY (s[0], s[1], s[2]);
            s += 3;
            d += 1;
          }
    }
}


static void 
copy_row_gray_to_u8_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U8(s[0]);
            d[3] = s[1] >> 8;
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U8(s[0]);
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U8(s[0]);
            d[3] = 255;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U8(s[0]);
            s += 1;
            d += 3;
          }
    }
}

static void 
copy_row_gray_to_u8_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint8 * d = (guint8*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = s[1] >> 8;
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            d[1] = 255;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U8(s[0]);
            s += 1;
            d += 1;
          }
    }
}


static void 
copy_row_gray_to_u16_rgb  (
                          PixelRow * src_row,
                          PixelRow * dest_row
                          )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U16(s[0]);
            d[3] = s[1];
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U16(s[0]);
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U16(s[0]);
            d[3] = 65535;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_U16(s[0]);
            s += 1;
            d += 3;
          }
    }
}

static void 
copy_row_gray_to_u16_gray  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = s[1];
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            d[1] = 65535;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_U16(s[0]);
            s += 1;
            d += 1;
          }
    }
}


static void 
copy_row_gray_to_float_rgb  (
                             PixelRow * src_row,
                             PixelRow * dest_row
                             )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_FLOAT(s[0]);
            d[3] = s[1] / (gfloat) 65535;
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_FLOAT(s[0]);
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_FLOAT(s[0]);
            d[3] = 1.0;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = BFP_TO_FLOAT(s[0]);
            s += 1;
            d += 3;
          }
    }
}


static void 
copy_row_gray_to_float_gray  (
                              PixelRow * src_row,
                              PixelRow * dest_row
                              )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  gfloat * d = (gfloat*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = s[1] / (gfloat) 65535;
            s += 2;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            s += 2;
            d += 1;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            d[1] = 1.0;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = BFP_TO_FLOAT(s[0]);
            s += 1;
            d += 1;
          }
    }
}


/****************************
 * Added to fix conversion
 * from u16 to float16
 * Kevin M. Murray 10/31/2002
 ****************************/
static void copy_row_gray_to_float16_rgb (PixelRow* src_row, PixelRow* dest_row) {
   int      w    = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
   Tag      stag = pixelrow_tag (src_row);
   Tag      dtag = pixelrow_tag (dest_row);
   guint16*  s   = (guint16*) pixelrow_data (src_row);
   guint16*  d   = (guint16*) pixelrow_data (dest_row);
   ShortsFloat u;

   if (tag_alpha (stag) == ALPHA_YES) {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = d[1] = d[2] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[3] = FLT16 (s[1] / (gfloat) 65535, u);
            s += 2;
            d += 4;
            }
         }
      else {
         while (w--) {
            d[0] = d[1] = d[2] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            s += 2;
            d += 3;
            }
         }
      }
   else {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = d[1] = d[2] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[3] = FLT16 (1.0, u);
            s += 1;
            d += 4;
            }
         }
      else {
         while (w--) {
            d[0] = d[1] = d[2] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            s += 1;
            d += 3;
            }
         }
      }
   }


/****************************
 * Added to fix conversion
 * from u16 to float16
 * Kevin M. Murray 10/31/2002
 ****************************/
static void copy_row_gray_to_float16_gray (PixelRow* src_row, PixelRow* dest_row){
   int      w    = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
   Tag      stag = pixelrow_tag (src_row);
   Tag      dtag = pixelrow_tag (dest_row);
/*
   gfloat*  s    = (gfloat*) pixelrow_data (src_row);
   guint16* d    = (guint16*) pixelrow_data (dest_row);
*/
   guint16*  s    = (guint16*) pixelrow_data (src_row);
   guint16* d    = (guint16*) pixelrow_data (dest_row);

   ShortsFloat u;

   if (tag_alpha (stag) == ALPHA_YES) {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (s[1] / (gfloat) 65535, u);
            s += 2;
            d += 2;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            s += 2;
            d += 1;
            }
         }
      }
   else {
      if (tag_alpha (dtag) == ALPHA_YES) {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            d[1] = FLT16 (1.0, u);
            s += 1;
            d += 2;
            }
         }
      else {
         while (w--) {
            d[0] = FLT16 (BFP_TO_FLOAT(s[0]), u);
            s += 1;
            d += 1;
            }
         }
      }
   }


static void 
copy_row_gray_to_bfp_rgb  (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == ALPHA_YES)
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = s[1];
            s += 2;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 2;
            d += 3;
          }
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            d[3] = 65535;
            s += 1;
            d += 4;
          }
      else
        while (w--)
          {
            d[0] = d[1] = d[2] = s[0];
            s += 1;
            d += 3;
          }
    }
}


static void
copy_row_gray_to_bfp_gray (
                           PixelRow * src_row,
                           PixelRow * dest_row
                           )
{
  int w = MIN (pixelrow_width (src_row), pixelrow_width (dest_row));
  Tag stag = pixelrow_tag (src_row);
  Tag dtag = pixelrow_tag (dest_row);
  guint16 * s = (guint16*) pixelrow_data (src_row);
  guint16 * d = (guint16*) pixelrow_data (dest_row);

  if (tag_alpha (stag) == tag_alpha(dtag))
    {
      memcpy (d, s, w * tag_bytes (stag));
    }
  else
    {
      if (tag_alpha (dtag) == ALPHA_YES)
        while (w--)
          {
            d[0] = s[0];
            d[1] = 65535;
            s += 1;
            d += 2;
          }
      else
        while (w--)
          {
            d[0] = s[0];
            s += 1;
            d += 1;
          }
    }
}


typedef void (*CopyRowFunc) (PixelRow *, PixelRow *);

// [source format][dest precision][dest format]
static CopyRowFunc funcs[2][5][2] = {
  {
    {
      copy_row_rgb_to_u8_rgb,
      copy_row_rgb_to_u8_gray
    },
    {
      copy_row_rgb_to_u16_rgb,
      copy_row_rgb_to_u16_gray
    },
    {
      copy_row_rgb_to_float_rgb,
      copy_row_rgb_to_float_gray
    },
    {
      copy_row_rgb_to_float16_rgb,
      copy_row_rgb_to_float16_gray
    },
    {
      copy_row_rgb_to_bfp_rgb,
      copy_row_rgb_to_bfp_gray
    }
  },
  
  {
    {
      copy_row_gray_to_u8_rgb,
      copy_row_gray_to_u8_gray
    },
    {
      copy_row_gray_to_u16_rgb,
      copy_row_gray_to_u16_gray
    },
    {
      copy_row_gray_to_float_rgb,
      copy_row_gray_to_float_gray
    },
    {
      copy_row_gray_to_float16_rgb,
      copy_row_gray_to_float16_gray
    },
    {
      copy_row_gray_to_bfp_rgb,
      copy_row_gray_to_bfp_gray
    }
  }
};


void
copy_row_bfp (
              PixelRow * src_row,
              PixelRow * dest_row
              )
{
  int x, y, z;
  
   switch (tag_format (pixelrow_tag (src_row))) {
      case FORMAT_RGB:      
         x = 0; 
         break;
      case FORMAT_GRAY:     
         x = 1; 
         break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
    default:
      g_warning ("unsupported src format in copy_row_bfp()");
      return;
    }

  switch (tag_precision (pixelrow_tag (dest_row))) {
     
      case PRECISION_U8:    
         y = 0; 
         break;
      case PRECISION_U16:   
         y = 1; 
         break;
      case PRECISION_FLOAT: 
         y = 2; 
         break;
      case PRECISION_FLOAT16:
         y = 3;
         break;
      case PRECISION_BFP:
         y = 4;
         break;
    case PRECISION_NONE:
    default:
        g_warning ("unsupported dest precision in copy_row_bfp() of [%d]", tag_precision (pixelrow_tag (dest_row)));
      return;
    }
  
  switch (tag_format (pixelrow_tag (dest_row)))
    {
    case FORMAT_RGB:      z = 0; break;
    case FORMAT_GRAY:     z = 1; break;
    case FORMAT_INDEXED:
    case FORMAT_NONE:
    default:
      g_warning ("unsupported dest format in copy_row_bfp()");
      return;
    }

  funcs[x][y][z] (src_row, dest_row);
}

