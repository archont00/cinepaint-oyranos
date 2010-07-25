/*
 * spline.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  spline.cpp 
  
  Two functions for cubic spline interpolation. Adapted from "Numerical
   Recipes in C", §3.3.
*/

#include <cstdio>       // fprintf()
#include <cstdlib>      // malloc(), free(), exit()
#include "spline.hpp"   // prototypes

/**+*************************************************************************\n
  spline()  --  adapted from "Numerical Recipes in C", §3.3.
  
  Given arrays \a x[] and \a y[] containing a tabulated function, i.e. \a yi=f(xi),
   with <i>x1 .<. x2 .<. ... .<. xN,</i> and given values \a yp1 and \a ypn for
   the first derivative of the interpolating function at points 1 and n, respectively,
   this routine returns an array \a y2[] that contains the second derivatives of the 
   interpolating function at the tabulated points \a xi. If \a yp1 and/or \a ypn are
   equal to 1 x 10^30 or larger, the routine is signaled to set the corresponding 
   boundary condition for a natural spline, with zero second derivative on that
   boundary.
******************************************************************************/
void spline (const double x[], const double y[], int n, double yp1, double ypn, double y2[])
{
    double p, qn, sig, un;
    //double u[n-1];      // local aux. vector; ISO C++ forbids variable size arrays
    double *u = (double*)malloc ((n-1)*sizeof(double));
    
    //  The lower boundary condition is set either to be "natural" or else to
    //   have a specified first derivative
    if (yp1 > 0.99e30)
      y2[0] = u[0] = 0.0;
    else {
      y2[0] = -0.5;
      u[0] = (3.0 / (x[1]-x[0])) * ((y[1]-y[0]) / (x[1]-x[0]) - yp1);
    }
    //  This is the decomposition loop of the tridiagonal algorithm. `y2´ and
    //   `u´ are used for temporary storage of the decomposed factors.
    for (int i=1; i < n-1; i++) 
    {
      sig = (x[i]-x[i-1]) / (x[i+1]-x[i-1]);
      p = sig * y2[i-1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = (y[i+1]-y[i]) / (x[i+1]-x[i]) - (y[i]-y[i-1]) / (x[i]-x[i-1]);
      u[i] = (6.0*u[i] / (x[i+1] - x[i-1]) - sig*u[i-1]) / p;
    }
    //  The upper boundary condition is set either to be "natural" or else to
    //   have a specified first derivative
    if (ypn > 0.99e30)
      qn = un = 0.0;
    else {
      qn = 0.5;
      un = (3.0 / (x[n-1]-x[n-2])) * (ypn - (y[n-1]-y[n-2]) / (x[n-1]-x[n-2]));
    }
    y2[n-1] = (un - qn*u[n-2]) / (qn*y2[n-2] + 1.0);
    //  This is the back substitution loop of the tridiagnonal algorithm
    for (int k=n-2; k >= 0; k--)
      y2[k] = y2[k]*y2[k+1] + u[k];
      
    free (u);
}

/**+*************************************************************************\n
  splint()  --  adapted from "Numerical Recipes in C", §3.3.
  
  Given the arrays \a xa[] and \a ya[], which tabulate a function (with the
   \a xa's in order), and given the array \a y2a[], which is the output from
   spline() above, and given a value of \a x, this routine returns a cubic
   spline interpolated value \a y.
******************************************************************************/
//void splint (double xa[], double ya[], double y2a[], int n, double x, double* y)
double splint (const double xa[], const double ya[], const double y2a[], int n, double x)
{
    //  We will find the right place in the table by means of bisection. This
    //   is optimal if sequential calls to this routine are at random values of
    //   x. If sequential calls are in order, and closely spaced, one would do
    //   better to store previous values of `klo´ and `khi´ and test if they
    //   remain appropriate on the next call.
    int klo = 0;
    int khi = n-1;
    int k;
    while (khi - klo > 1) 
    {
      k = (khi+klo) >> 1;
      if (xa[k] > x) khi = k;
      else           klo = k;
    }
    //  `klo´ and `khi´ now bracket the input value of x.
    double h = xa[khi] - xa[klo];
    //  The xa's must be distinct
    if (h == 0.0) {fprintf(stderr, "Bad `xa[]´ input to splint()\n"); exit(1);}
    //  Cubic spline polynomial is now evaluated
    double A = (xa[khi] - x) / h;
    double B = (x - xa[klo]) / h;
    //*y = A*ya[klo] + B*ya[khi] + ((A*A*A-A)*y2a[klo] + (B*B*B-B)*y2a[khi]) * (h*h) / 6.0;
    return A*ya[klo] + B*ya[khi] + ((A*A*A-A)*y2a[klo] + (B*B*B-B)*y2a[khi]) * (h*h) / 6.0;
}


// END OF FILE
