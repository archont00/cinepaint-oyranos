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
#ifndef __FLOAT16_H__
#define __FLOAT16_H__

#include <glib.h>
//#include "config.h"

/*#define RNH_FLOAT 1*/
#ifdef RNH_FLOAT

#define ONE_FLOAT16 16256
#define HALF_FLOAT16 16128
#define ZERO_FLOAT16 0

typedef union
{
  guint16 s[2];
  gfloat f;
}ShortsFloat;

#if WORDS_BIGENDIAN 
#define FLT( x, t ) (t.s[0] = (x), t.s[1]= 0, t.f) 
#define FLT16( x, t ) (t.f = (x), t.s[0])   
#else
#define FLT( x, t ) (t.s[1] = (x), t.s[0]= 0, t.f) 
#define FLT16( x, t ) (t.f = (x), t.s[1])   
#endif

#else /* OpenEXR/Nvidia half float */

#include "libhalf/cinepaint_half.h"

#define ONE_FLOAT16 15360
#define HALF_FLOAT16 14336
#define ZERO_FLOAT16 0

typedef union
{
  guint32 i;
  guint16 s[2];
  gfloat  f;
} ShortsFloat;


#if 0
/* the following macros are not as relyable as the library itself */
#define FLT( x, t ) ( \
  /* s */ t.i =(((((guint16)(x) >> 15) & 0x00000001) << 31) | \
  /* e */      (((((guint16)(x) >> 10) & 0x0000001f) + (127 - 15)) << 23) | \
  /* m */         ((guint16)(x)        & 0x000003ff)               << 13), \
 \
               ( \
                  (((guint16)(x) == 32768)                   ? \
                           -0.0 \
                                                             : \
                  (((guint16)(x) >> 10) & 0x0000001f)==0 && \
                   ((guint16)(x)        & 0x000003ff)==0)      ? \
                 ((((guint16)(x) >> 15) & 0x00000001) << 31) \
                                                               : \
                            t.f)                                   ) 
#define FLT16( x, t ) ( t.f = x, \
                        (x == 0.0) ? 0 : \
                 /* s */ (((t.i >> 16) & 0x00008000) | \
                 /* e */((((t.i >> 23) & 0x000000ff) - (127 - 15)) << 10) | \
                 /* m */  ((t.i        & 0x007fffff)               >> 13)))
#else
#define FLT( x, t )   ( ImfHalf2Float(x) ) 
#define FLT16( x, t ) ( ImfFloat2Half(x) )
#endif

#endif /* RNH_FLOAT */

#endif /* __FLOAT16_H__ */
