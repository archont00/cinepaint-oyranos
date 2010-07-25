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

/* define this so these routines get compiled */
#define DEBUG_TAGS

#include "../tag.h"
#include "../../lib/wire/enums.h"

Tag
tag_new (
         Precision  p,
         Format     f,
         Alpha      a
         )
{
  return (
          ((p & MASK_PRECISION) << SHIFT_PRECISION) |
          ((f & MASK_FORMAT) << SHIFT_FORMAT) |
          ((a & MASK_ALPHA) << SHIFT_ALPHA)
          );
    
}


Precision
tag_precision (
               Tag x
               )
{
  return ((x >> SHIFT_PRECISION) & MASK_PRECISION);
}


Format
tag_format (
            Tag x
            )
{
  return ((x >> SHIFT_FORMAT) & MASK_FORMAT);
}


Alpha
tag_alpha (
           Tag x
           )
{
  return ((x >> SHIFT_ALPHA) & MASK_ALPHA);
}


Tag
tag_set_precision (
                   Tag y,
                   Precision x
                   )
{
  switch (x)
    {
    case PRECISION_NONE:
    case PRECISION_U8:
    case PRECISION_U16:
    case PRECISION_FLOAT:
    case PRECISION_FLOAT16:
    case PRECISION_BFP:
      break;
    default:
      x = PRECISION_NONE;
      break;
    }
  y &= ~(MASK_PRECISION << SHIFT_PRECISION);
  y |= ((x & MASK_PRECISION) << SHIFT_PRECISION);
  return y;
}


Tag
tag_set_format (
                Tag y,
                Format x
                )
{
  switch (x)
    {
    case FORMAT_NONE:
    case FORMAT_RGB:
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      break;
    default:
      x = FORMAT_NONE;
      break;
    }
  y &= ~(MASK_FORMAT << SHIFT_FORMAT);
  y |= ((x & MASK_FORMAT) << SHIFT_FORMAT);
  return y;
}


Tag
tag_set_alpha (
               Tag y,
               Alpha x
               )
{
  switch (x)
    {
    case ALPHA_NONE:
    case ALPHA_NO:
    case ALPHA_YES:
      break;
    default:
      x = ALPHA_NONE;
      break;
    }
  y &= ~(MASK_ALPHA << SHIFT_ALPHA);
  y |= ((x & MASK_ALPHA) << SHIFT_ALPHA);
  return y;
}


guint
tag_num_channels (
                  Tag x
                  )
{
  int y;
  
  switch ((x >> SHIFT_FORMAT) & MASK_FORMAT) /* (tag_format (x)) */
    {
    case FORMAT_RGB:
      y = 3;
      break;
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      y = 1;
      break;
    case FORMAT_NONE:
    default:
      return 0;
    }

  switch ((x >> SHIFT_ALPHA) & MASK_ALPHA) /* (tag_alpha(x)) */
    {
    case ALPHA_YES:
      y++;
      break;
    case ALPHA_NO:
      break;
    case ALPHA_NONE:
    default:
      return 0;
    }

  return y;
}

guint
tag_bytes (
           Tag x
           )
{
  int y;
  
  /* y = tag_num_channels (x); */
  switch ((x >> SHIFT_FORMAT) & MASK_FORMAT) /* (tag_format (x)) */
    {
    case FORMAT_RGB:
      y = 3;
      break;
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      y = 1;
      break;
    case FORMAT_NONE:
    default:
      return 0;
    }

  switch ((x >> SHIFT_ALPHA) & MASK_ALPHA) /* (tag_alpha(x)) */
    {
    case ALPHA_YES:
      y++;
      break;
    case ALPHA_NO:
      break;
    case ALPHA_NONE:
    default:
      return 0;
    }

  switch ((x >> SHIFT_PRECISION) & MASK_PRECISION) /* (tag_precision (x)) */
    {
    case PRECISION_U8:
      y *= sizeof(guint8);
      break;
    case PRECISION_BFP:
    case PRECISION_U16:
      y *= sizeof(guint16);
      break;
    case PRECISION_FLOAT:
      y *= sizeof(gfloat);
      break;
    case PRECISION_FLOAT16:
      y *= sizeof(guint16);
      break;
    case PRECISION_NONE:
      return 0;
    }
  
  return y;
}

guint
tag_alpha_index (
		  Tag t
		)
{
  switch (tag_format (t)) 
    {
    case FORMAT_RGB:
      return ALPHA_PIX;
    case FORMAT_GRAY:
      return ALPHA_G_PIX;
    case FORMAT_INDEXED:
      return ALPHA_I_PIX;
    case FORMAT_NONE:
    default:
      return -1;
    }
}

guint
tag_equal (
           Tag  a,
           Tag  b
           )
{
  if ((tag_format (a) == tag_format (b)) &&
      (tag_precision (a) == tag_precision (b)) &&
      (tag_alpha (a) == tag_alpha (b)))
    return TRUE;
  return FALSE;
}


gchar* tag_string_precision(Precision x)
{	switch (x)
	{	case PRECISION_NONE:
			return "<uninit precision>";
		case PRECISION_U8:
			return PRECISION_U8_STRING;
		case PRECISION_U16:
			return PRECISION_U16_STRING;
		case PRECISION_FLOAT:
			return PRECISION_FLOAT_STRING;
		case PRECISION_FLOAT16:
			return PRECISION_FLOAT16_STRING;
		case PRECISION_BFP:
			return PRECISION_BFP_STRING;
	}
	return "<invalid precision!>";
}


gchar *
tag_string_format (
                   Format x
                   )
{
  switch (x)
    {
    case FORMAT_NONE:
      return "<uninit format>";
    case FORMAT_RGB:
      return "RGB";
    case FORMAT_GRAY:
      return "grayscale";
    case FORMAT_INDEXED:
      return "indexed";
    }
  return "<invalid format!>";
}


gchar *
tag_string_alpha (
                  Alpha x
                  )
{
  switch (x)
    {
    case ALPHA_NONE:
      return "<uninit alpha>";
    case ALPHA_NO:
      return "noalpha";
    case ALPHA_YES:
      return "alpha";
    }
  return "<invalid alpha!>";
}

Tag
tag_null (
          void
          )
{
  return tag_new (PRECISION_NONE, FORMAT_NONE, ALPHA_NONE);
}


gint 
tag_valid (
	   Tag t 
	  )
{
  Alpha a = tag_alpha (t);
  Precision p = tag_precision (t);
  Format f = tag_format (t);
  gint valid = TRUE;
 
  switch (p)
  {
    case PRECISION_FLOAT16:
      if (f == FORMAT_INDEXED) valid = FALSE;
    case PRECISION_FLOAT:
      if (f == FORMAT_INDEXED) valid = FALSE;
    case PRECISION_U16:
    case PRECISION_U8:
    case PRECISION_BFP:
      break;
    default:
      valid = FALSE;
      break;
  } 
  
  switch (f)
  {
    case FORMAT_RGB:
    case FORMAT_GRAY:
    case FORMAT_INDEXED:
      break;
    default:
      valid = FALSE;
      break;
  }
  
  switch (a)
  {
    case ALPHA_YES:
    case ALPHA_NO:
      break;
    default:
      valid = FALSE;
      break;
  }
 return valid;
}



gint  
tag_to_drawable_type (
             Tag t 
            )
{
  Alpha a = tag_alpha (t);
  Precision p = tag_precision (t);
  Format f = tag_format (t);
  
  if ( !tag_valid (t) )
    return -1;
  
  switch (p)
    {
    case PRECISION_U8:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  RGBA_GIMAGE: RGB_GIMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  GRAYA_GIMAGE: GRAY_GIMAGE;
        case FORMAT_INDEXED:      
          return (a == ALPHA_YES)?  INDEXEDA_GIMAGE: INDEXED_GIMAGE;
        default:
          break; 
	} 
      break;
    
    case PRECISION_U16:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  U16_RGBA_GIMAGE: U16_RGB_GIMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  U16_GRAYA_GIMAGE: U16_GRAY_GIMAGE;
        case FORMAT_INDEXED:      
          return (a == ALPHA_YES)?  U16_INDEXEDA_GIMAGE: U16_INDEXED_GIMAGE;
        default:
          break; 
	} 
      break;
    
    case PRECISION_FLOAT:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  FLOAT_RGBA_GIMAGE: FLOAT_RGB_GIMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  FLOAT_GRAYA_GIMAGE: FLOAT_GRAY_GIMAGE;
        default:
          break; 
	} 
      break;

    case PRECISION_FLOAT16:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  FLOAT16_RGBA_GIMAGE: FLOAT16_RGB_GIMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  FLOAT16_GRAYA_GIMAGE: FLOAT16_GRAY_GIMAGE;
        default:
          break; 
	} 
      break;

    case PRECISION_BFP:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  BFP_RGBA_GIMAGE: BFP_RGB_GIMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  BFP_GRAYA_GIMAGE: BFP_GRAY_GIMAGE;
        default:
          break; 
	} 
      break;

    default:
      break;
    }
    return -1;
}

gint  
tag_to_image_type (
             Tag t 
            )
{
  Precision p = tag_precision (t);
  Format f = tag_format (t);
  
  switch (p)
    {
    case PRECISION_U8:
//      d_printf ("8\n"); 
      switch (f)
	{
        case FORMAT_RGB:
          return  RGB;
        case FORMAT_GRAY:
          return  GRAY;
        case FORMAT_INDEXED:      
          return  INDEXED;
        default:
          break; 
	} 
      break;
    
    case PRECISION_U16:
//      d_printf ("16\n"); 
      switch (f)
	{
        case FORMAT_RGB:
          return  U16_RGB;
        case FORMAT_GRAY:
          return  U16_GRAY;
        case FORMAT_INDEXED:      
          return  U16_INDEXED;
        default:
          break; 
	} 
      break;
    
    case PRECISION_FLOAT:
//      d_printf ("float\n"); 
      switch (f)
	{
        case FORMAT_RGB:
          return  FLOAT_RGB;
        case FORMAT_GRAY:
          return  FLOAT_GRAY;
        default:
          break; 
	} 
      break;
    
    case PRECISION_FLOAT16:

//      d_printf ("float16\n"); 
      switch (f)
	{
        case FORMAT_RGB:
          return  FLOAT16_RGB;
        case FORMAT_GRAY:
          return  FLOAT16_GRAY;
        default:
          break; 
	} 
      break;

    case PRECISION_BFP:
//      d_printf ("bfp\n"); 
      switch (f)
	{
        case FORMAT_RGB:
          return  BFP_RGB;
        case FORMAT_GRAY:
          return  BFP_GRAY;
        default:
          break; 
	} 
      break;

    default:
      break;
    }
    return -1;
}

Tag 
tag_from_drawable_type (
               gint type 
              )
{
  Precision p;
  Format f;
  Alpha a = ALPHA_NO;
  
  switch (type)
    {
    /* The 8bit data cases */
    case RGBA_GIMAGE:
      a = ALPHA_YES;
    case RGB_GIMAGE:
      p = PRECISION_U8;
      f = FORMAT_RGB;
      break;
    case GRAYA_GIMAGE:
      a = ALPHA_YES;
    case GRAY_GIMAGE:
      p = PRECISION_U8;
      f = FORMAT_GRAY;
      break;
    case INDEXEDA_GIMAGE:
      a = ALPHA_YES;
    case INDEXED_GIMAGE:
      p = PRECISION_U8;
      f = FORMAT_INDEXED;
      break;
    
    /* The 16bit data cases */
    case U16_RGBA_GIMAGE:
      a = ALPHA_YES;
    case U16_RGB_GIMAGE:
      p = PRECISION_U16;
      f = FORMAT_RGB;
      break;
    case U16_GRAYA_GIMAGE:
      a = ALPHA_YES;
    case U16_GRAY_GIMAGE:
      p = PRECISION_U16;
      f = FORMAT_GRAY;
      break;
    case U16_INDEXEDA_GIMAGE:
      a = ALPHA_YES;
    case U16_INDEXED_GIMAGE:
      p = PRECISION_U16;
      f = FORMAT_INDEXED;
      break;
    
    /* The float data cases */
    case FLOAT_RGBA_GIMAGE:
      a = ALPHA_YES;
    case FLOAT_RGB_GIMAGE:
      p = PRECISION_FLOAT;
      f = FORMAT_RGB;
      break;
    case FLOAT_GRAYA_GIMAGE:
      a = ALPHA_YES;
    case FLOAT_GRAY_GIMAGE:
      p = PRECISION_FLOAT;
      f = FORMAT_GRAY;
      break;
     
    /* The float 16 data cases */
    case FLOAT16_RGBA_GIMAGE:
      a = ALPHA_YES;
    case FLOAT16_RGB_GIMAGE:
      p = PRECISION_FLOAT16;
      f = FORMAT_RGB;
      break;
    case FLOAT16_GRAYA_GIMAGE:
      a = ALPHA_YES;
    case FLOAT16_GRAY_GIMAGE:
      p = PRECISION_FLOAT16;
      f = FORMAT_GRAY;
      break;

    /* The bfp data cases */
    case BFP_RGBA_GIMAGE:
      a = ALPHA_YES;
    case BFP_RGB_GIMAGE:
      p = PRECISION_BFP;
      f = FORMAT_RGB;
      break;
    case BFP_GRAYA_GIMAGE:
      a = ALPHA_YES;
    case BFP_GRAY_GIMAGE:
      p = PRECISION_BFP;
      f = FORMAT_GRAY;
      break;

    /* any undefined cases */
    default:
      p = PRECISION_NONE;
      f = FORMAT_NONE;
      a = ALPHA_NONE;
      break;
    }
  return tag_new (p, f, a);
}


Tag 
tag_from_image_type (
               gint type 
              )
{
  Precision p;
  Format f;
  Alpha a = ALPHA_NO;
  
  switch (type)
    {
    /* The 8bit data cases */
    case RGB:
      p = PRECISION_U8;
      f = FORMAT_RGB;
      break;
    case GRAY:
      p = PRECISION_U8;
      f = FORMAT_GRAY;
      break;
    case INDEXED:
      p = PRECISION_U8;
      f = FORMAT_INDEXED;
      break;
    
    /* The 16bit data cases */
    case U16_RGB:
      p = PRECISION_U16;
      f = FORMAT_RGB;
      break;
    case U16_GRAY:
      p = PRECISION_U16;
      f = FORMAT_GRAY;
      break;
    case U16_INDEXED:
      p = PRECISION_U16;
      f = FORMAT_INDEXED;
      break;
    
    /* The float data cases */
    case FLOAT_RGB:
      p = PRECISION_FLOAT;
      f = FORMAT_RGB;
      break;
    case FLOAT_GRAY:
      p = PRECISION_FLOAT;
      f = FORMAT_GRAY;
      break;
     
    /* The float 16 data cases */
    case FLOAT16_RGB:
      p = PRECISION_FLOAT16;
      f = FORMAT_RGB;
      break;
    case FLOAT16_GRAY:
      p = PRECISION_FLOAT16;
      f = FORMAT_GRAY;
      break;
    
    /* The bfp data cases */
    case BFP_RGB:
      p = PRECISION_BFP;
      f = FORMAT_RGB;
      break;
    case BFP_GRAY:
      p = PRECISION_BFP;
      f = FORMAT_GRAY;
      break;

    /* any undefined cases */
    default:
      p = PRECISION_NONE;
      f = FORMAT_NONE;
      a = ALPHA_NONE;
      break;
    }
  return tag_new (p, f, a);
}

