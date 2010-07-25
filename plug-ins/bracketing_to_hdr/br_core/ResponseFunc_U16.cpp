/*
 * ResponseFunc_U16.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ResponseFunc_U16.cpp
  
  Responsefunktion fuer U16-Werte, die aus 256 Stuetzstellen linear oder 
   spline-interpoliert.
*/

#include <iostream>             // cout
#include <cassert>
#include "ResponseFunc_U16.hpp"
#include "spline.hpp"           // spline(), splint()

using std::cout;
using std::cerr;

/**+*************************************************************************\n
  Ctor()
  
  @param ygrid: Array[256] of y-values - die 256 Stuetzstellen der zu 
    interpolierenden Response-Funktion. Die zugehoerigen Abzisssenwerte 
    unterstellen wir gleichabstaendig bei <i>x_i = 256*i + 127, i=0..255</i>
    (Mitte der FollowUp-Zaehlintervalle).
******************************************************************************/
ResponseInterpol::ResponseInterpol (const TNT::Array1D<double> & ygrid)
{
    //assert (ygrid.dim() > 1);         // 2 Stuetztstellen waere Minimum
    assert (ygrid.dim() == 256);        // Denn wir benutzen in value() fixe Indizes
    
    ygrid_ = ygrid;                     // ref. counting copy
    xgrid_ = TNT::Array1D<double> (ygrid_.dim());
    
    //  Wir unterstellen die Stuetzstellen-Abzissen bei 256*i + 127, i=0..255.
    for (int i=0; i < xgrid_.dim(); i++)
      xgrid_[i] = (i << 8) + 127;
}

/**+*************************************************************************\n
 \fn ResponseInterpol::value_linear(uint16) const
  Linear interpolierter Exposure-Wert zum Argument \a z.
  
  @param z: uint16 value in [0..65535]
  @return interpolierter Lichtmengenwert
  
  Wir unterstellen die Stuetzstellen-Abzissen bei <i>256*i + 127, i=0..255.</i>
   D.h. wir werten nicht \a xgrid_ aus (Suche in Tabelle), sondern ermitteln
   die benachtbarten Stuetzstellen von \a z direkter. Und wir rechnen gleich mit
   <i>xgrid[ir] - xgrid[il] == 256</i>.
******************************************************************************/
double ResponseInterpol::value_linear (uint16 z) const
{
    int il = (z - 127) >> 8;    // Index der linken Stuetzstelle
    int ir = il + 1;            // Index der rechten Stuetzstelle
    if (il < 0) {               // z < 127 --> Extrapolation
      //cout << "Extrapolation nach links\n";
      il = 0;
      ir = 1;
    }
    else if (ir > 255) {        // z > 65535 - 127 == 65408 --> Extrapolation
      //cout << "Extrapolation nach rechts\n";
      il = 254;
      ir = 255;
    }
    return (ygrid_[ir] - ygrid_[il]) / 256. * (z - xgrid_[il]) + ygrid_[il];
}


/**+*************************************************************************\n
 \fn ResponseInterpol::value_spline_fast (uint16) const
  Schnelleres Finden der Stuetzstellen.
   Wir unterstellen die Stueztstellenabzissen bei <i>256*i + 127.</i> Finden damit
   die benachtbarten Stuetzstellen zu \a z schneller als mittels Bisektion in 
   splint(). Und gehen von festem x-Abstand \a h=256 der Stuetzstellen aus.
******************************************************************************/
double ResponseInterpol::value_spline_fast (uint16 z) const
{
//    cout <<__func__<< "(uint16 z=" << z << ")...\n";
    int klo = (z - 127) >> 8;   // Index der linken Stuetzstelle
    int khi = klo + 1;          // Index der rechten Stuetzstelle
    if (klo < 0) {              // z < 127 --> Extrapolation
      //cout << "Extrapolation nach links\n";
      klo = 0;
      khi = 1;
    }
    else if (khi > 255) {       // z > 65535-127 == 65408 --> Extrapolation
      //cout << "Extrapolation nach rechts\n";
      klo = 254;
      khi = 255;
    }
    //  We have a apriori known constant x-distance '256'
    const double h = 256.;      // == xa[khi] - xa[klo];
    //  Cubic spline polynomial is now evaluated
    double A = (xgrid_[khi] - z) / h;
    double B = (z - xgrid_[klo]) / h;
    return A*ygrid_[klo] + B*ygrid_[khi] + ((A*A*A-A)*y2_[klo] + (B*B*B-B)*y2_[khi]) * (h*h) / 6.0;
}

/**+*************************************************************************\n
 \fn ResponseInterpol::init_spline()
  Init the spline stuff, i.e. evaluate the array of second derivatives \a y2.
******************************************************************************/
void ResponseInterpol::init_spline()
{
    int n = ygrid_.dim();
    
    y2_ = TNT::Array1D<double> (n);
    
    //spline (xgrid_, ygrid_, n, 0.0, 0.0, y2_);   // first derivatives = 0!
    spline (xgrid_, ygrid_, n, 1e30, 1e30, y2_);   // second derivatives = 0!
    
    //cout << y2_;
}

/**+*************************************************************************\n
 \fn ResponseInterpol::init_table()
  Allocate an array[65536] of doubles and fill it with the interpolated response
   values, i.e. for each possible z-value 0...65535. After that the function
   value_spline_table() is appropriable.
******************************************************************************/
void ResponseInterpol::init_table()
{
    table_ = TNT::Array1D<double>(65536);
    
    for (int i=0; i < table_.dim(); i++)
      table_[i] = value_spline_fast (uint16(i));
}

// END OF FILE
