/*
 *   Colour handling for CinePaint
 *
 *   Copyright 2004 Kai-Uwe Behrmann
 *
 *   separate stuff
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  March 2004
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
/*#  include <config.h> */
#endif

#include <math.h>

#ifndef DEBUG
/* #define DEBUG */
#endif

#ifdef DEBUG
 #define DBG(text) printf("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,text);
#else
 #define DBG(text)
#endif

/*** function definitions ***/

  /* --- cmm functions --- */

void
fXYZtoColour                 (  float           xyz[3],
                                float          *c_32,
                                float           stonits) 
{

  /* assume CCIR-709 primaries (matrix from tif_luv.c)
     LOG Luv XYZ (D65) -> sRGB (CIE Illuminant E)
   */
  c_32[0] =  2.690*xyz[0] + -1.276*xyz[1] + -0.414*xyz[2];
  c_32[1] = -1.022*xyz[0] +  1.978*xyz[1] +  0.044*xyz[2];
  c_32[2] =  0.061*xyz[0] + -0.224*xyz[1] +  1.163*xyz[2];

  if (stonits != 0.0) {
      c_32[0] = c_32[0] * stonits;
      c_32[1] = c_32[1] * stonits;
      c_32[2] = c_32[2] * stonits;
  } else {
  #if 0
      c_32[0] = xyz[0];
      c_32[1] = xyz[1];
      c_32[2] = xyz[2];
  #endif
  }
}

void
fsRGBToXYZ                    ( float           rgb[3],
                                float          *c_32) 
{
    /*
     * assume CCIR-709 primaries, whitpoint x = 1/3 y = 1/3 (D_E)
     * "The LogLuv Encoding for Full Gamut, High Dynamic Range Images" <G.Ward>
     * sRGB ( CIE Illuminant E ) -> LOG Luv XYZ (D65)
     */
    rgb[0] = pow(rgb[0], 2.2);
    rgb[1] = pow(rgb[1], 2.2);
    rgb[2] = pow(rgb[2], 2.2);
    c_32[0] =  0.497*rgb[0] +  0.339*rgb[1] +  0.164*rgb[2];
    c_32[1] =  0.256*rgb[0] +  0.678*rgb[1] +  0.066*rgb[2];
    c_32[2] =  0.023*rgb[0] +  0.113*rgb[1] +  0.864*rgb[2];
}




