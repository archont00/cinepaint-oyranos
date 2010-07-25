/*
 * spline.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
 *
 * Copyright (c) 2005-2006  Hartmut Sbosny  <hartmut.sbosny@gmx.de>
 *
 * LICENSE:
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
/**
  spline.hpp  
  
  Two functions for cubic spline interpolation. Adapted from "Numerical
   Recipes in C", §3.3.
*/
#ifndef spline_hpp
#define spline_hpp

/****************************************************************************
  Description see: file "spline.cpp".
******************************************************************************/
void spline (const double x[], const double y[], int n, double yp1, double ypn, double y2[]);

/****************************************************************************
  Description see: file "spline.cpp".
******************************************************************************/
double splint (const double xa[], const double ya[], const double y2a[], int n, double x);


#endif  // spline_hpp

// END OF FILE
