/*
 * input_statistics.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  input_statistics.cpp
*/

#include "input_statistics.hpp"
#include "br_Image.hpp"
#include "br_ImgScheme2D.hpp"
#include "Rgb.hpp"


namespace br {

/**+*************************************************************************\n
  compute_input_stats_...()
  
  TIP: Fuer die Aufgaben hier brauchen wir keine 2D-Sicht.
******************************************************************************/

void compute_input_stats_RGB_U8 (BrImage & img)
{
    CHECK_IMG_TYPE_RGB_U8 (img);

    Rgb<double> s(0);     
    //Rgb<uint64> s(0);   // uint64 statt double beschleunigt unwesentlich
                          //  (but ISO C++ does not support `long long´)
    int n_low  = 0;
    int n_high = 0;
    
    int n_pixel = img.n_pixel();
    const Rgb<uint8>*  buf = (Rgb<uint8>*) img.buffer();

    for (int i=0; i < n_pixel; i++, buf++)
    {
      // brightness...
      s += *buf;

      // working range...
      if ((*buf).r == 0 || (*buf).g == 0 || (*buf).b == 0) 
      {
        n_low ++; 
      }
      else if ((*buf).r==255 || (*buf).g==255 || (*buf).b==255) 
      {
        n_high ++;
      }
    }
    
    ImgStatisticsData &E = img.statistics;

    E.brightness = double(s.r + s.g + s.b) / n_pixel;
    E.n_low = n_low;
    E.n_high = n_high;
    E.r_wk_range = 1.0 - double(n_low + n_high) / n_pixel;
}


void compute_input_stats_RGB_U8_ref (BrImage & img)
{
    CHECK_IMG_TYPE_RGB_U8(img);
    ImgScheme2DView< Rgb<uint8> >  A(img);  

    Rgb<double> s(0);     
    //Rgb<uint64> s(0);   // uint64 statt double beschleunigt unwesentlich
                          //  (but ISO C++ does not support `long long´)
    int n_low = 0;
    int n_high = 0;

    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)
    {
      const Rgb<uint8> & val = A[i][j];
      
      // brightness...
      s += val;
      
      // working range...
      if (val.r == 0 || val.g == 0 || val.b == 0) 
      {
        n_low ++; 
      }
      else if (val.r==255 || val.g==255 || val.b==255) 
      {
        n_high ++;
      }
    }
    
    ImgStatisticsData &E = img.statistics;

    E.brightness = double(s.r + s.g + s.b) / (A.dim1() * A.dim2());
    E.n_low = n_low;
    E.n_high = n_high;
    E.r_wk_range = 1.0 - double(n_low + n_high) / (A.dim1() * A.dim2());
}


void compute_input_stats_RGB_U8_A2 (BrImage & img)
{
    CHECK_IMG_TYPE_RGB_U8(img);
    ImgArray2D< Rgb<uint8> >  A(img);  

    Rgb<double> s(0);     
    //Rgb<uint64> s(0);   // uint64 statt double beschleunigt unwesentlich
                          //  (but ISO C++ does not support `long long´)
    int n_low  = 0;
    int n_high = 0;

    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)
    {
      // brightness...
      s += A[i][j];
      
      // working range...
      if (A[i][j].r == 0 || A[i][j].g == 0 || A[i][j].b == 0) 
      {
        n_low ++; 
      }
      else if (A[i][j].r==255 || A[i][j].g==255 || A[i][j].b==255) 
      {
        n_high ++;
      }
    }

    ImgStatisticsData &E = img.statistics;

    E.brightness = double(s.r + s.g + s.b) / (A.dim1() * A.dim2());
    E.n_low = n_low;
    E.n_high = n_high;
    E.r_wk_range = 1.0 - double(n_low + n_high) / (A.dim1() * A.dim2());
}



/**+*************************************************************************\n
  compute_input_stats_RGB_U16()
  
  TIP: Fuer die Aufgaben hier brauchen wir keine 2D-Sicht.
******************************************************************************/
void compute_input_stats_RGB_U16 (BrImage & img)
{
    CHECK_IMG_TYPE_RGB_U16 (img);

    Rgb<double> s(0);
    //Rgb<uint64> s(0);   // uint64 statt double beschleunigt unwesentlich
    
    const uint16 maxval = 65535;
    int n_low   = 0;
    int n_high  = 0;
    int n_pixel = img.n_pixel();
    const Rgb<uint16>*  buf = (Rgb<uint16>*) img.buffer();
    
    for (int i=0; i < n_pixel; i++, buf++)
    {
      // brightness...
      s += *buf;

      // working range...
      if ((*buf).r == 0 || (*buf).g == 0 || (*buf).b == 0) 
      {
        n_low ++; 
      }
      else if ((*buf).r==maxval || (*buf).g==maxval || (*buf).b==maxval) 
      {
        n_high ++;
      }
    }
    
    ImgStatisticsData &E = img.statistics;

    E.brightness = double(s.r + s.g + s.b) / n_pixel;
    E.n_low = n_low;
    E.n_high = n_high;
    E.r_wk_range = 1.0 - double(n_low + n_high) / n_pixel;
}

}  // namespac "br"

// END OF FILE
