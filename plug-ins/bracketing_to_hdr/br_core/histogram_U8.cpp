/*
 * histogram_U8.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  histogram_U8.cpp
*/

#include "HistogramData.hpp"
#include "br_Image.hpp"
#include "Rgb.hpp"
#include "histogram_U8.hpp"
#include "br_macros.hpp"                // IF_FAIL_DO()


using namespace TNT;


/**+*************************************************************************\n
  Compute the histogram of an RGB_U8 image for channel `channel'.
******************************************************************************/
void
compute_histogram_RGB_U8 (HistogramData & histo, const br::Image & img, int channel)
{
    CHECK_IMG_TYPE_RGB_U8(img);
    IF_FAIL_DO (0 <= channel && channel < 3, return);

    histo.init (256);
    
    const Rgb<uint8>* buf = (Rgb<uint8>*) img.buffer();
    int n_pixel = img.n_pixel();
    
    for (int i=0; i < n_pixel; i++)
      ++ histo.data[ buf[i][channel] ];
      
    histo.findMaxValue();       // Wozu?
}

/**+*************************************************************************\n
  Test: Faster without channel dereferencing at every point?
******************************************************************************/
void
compute_histogram_RGB_U8_switch (HistogramData & histo, const br::Image & img, int channel)
{
    CHECK_IMG_TYPE_RGB_U8(img);
    IF_FAIL_DO (0 <= channel && channel < 3, return);

    histo.init (256);
    
    const Rgb<uint8>* buf = (Rgb<uint8>*) img.buffer();
    int n_pixel = img.n_pixel();
    
/*    switch (channel)
    {
    case 0:
        for (int i=0; i < n_pixel; i++)
          ++ histo.data[ buf[i].r ];
        break;
    case 1:
        for (int i=0; i < n_pixel; i++)
          ++ histo.data[ buf[i].g ];
        break;
    default:
        for (int i=0; i < n_pixel; i++)
          ++ histo.data[ buf[i].b ];
        break;
    }*/
    if (channel == 0) {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].r ];
    }
    else if (channel == 1) {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].g ];
    }
    else {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].b ];
    }
    histo.findMaxValue();  
}

/**+*************************************************************************\n
  Compute the histogram of an RGB_U8 image for all three channels.
  
  @param histos: Address of an array `HistogramData[3]´ of three histograms.
******************************************************************************/
void
compute_histogram_RGB_U8 (HistogramData * histos, const br::Image & img)
{
    CHECK_IMG_TYPE_RGB_U8 (img);

    histos[0].init (256);
    histos[1].init (256);
    histos[2].init (256);
    
    const Rgb<uint8>* buf = (Rgb<uint8>*) img.buffer();
    int n_pixel = img.n_pixel();
    
    for (int i=0; i < n_pixel; i++)
    {
      ++ histos[0].data[ buf[i][0] ];
      ++ histos[1].data[ buf[i][1] ];
      ++ histos[2].data[ buf[i][2] ];
    }
      
    histos[0].findMaxValue();       // Wozu?
    histos[1].findMaxValue();       // Wozu?
    histos[2].findMaxValue();       // Wozu?
}


// END OF FILE
