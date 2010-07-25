/*
 * histogram_U16.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  histogram_U16.cpp
*/

#include "HistogramData.hpp"
#include "br_Image.hpp"
#include "histogram_U16.hpp"
#include "Rgb.hpp"
#include "br_macros.hpp"                // IF_FAIL_DO()


using namespace TNT;


/**+*************************************************************************\n
  Compute the histogram of an RGB_U16 image for the channel `channel´.
  
  We count the 65536 possible values 0...65535 of an U16 in 256 intervalls:
    z=  0..255  -->  item 0 
    z=256..511  -->  item 1,  etc.
  We get the items via `z >> 8´.  
******************************************************************************/
void
compute_histogram_RGB_U16 (HistogramData & histo, const br::Image & img, int channel)
{
    CHECK_IMG_TYPE_RGB_U16(img);
    IF_FAIL_DO (0 <= channel && channel < 3, return);

    histo.init (256);
    
    const Rgb<uint16>* buf = (Rgb<uint16>*) img.buffer();
    int n_pixel = img.n_pixel();
    
    for (int i=0; i < n_pixel; i++)
      ++ histo.data[ buf[i][channel] >> 8 ];
      
    histo.findMaxValue();       // Wozu?
}

/**+*************************************************************************\n
  Test: Faster without channel dereferencing at every point?
******************************************************************************/
void
compute_histogram_RGB_U16_switch (HistogramData & histo, const br::Image & img, int channel)
{
    CHECK_IMG_TYPE_RGB_U16(img);
    IF_FAIL_DO (0 <= channel && channel < 3, return);

    histo.init (256);
    
    const Rgb<uint16>* buf = (Rgb<uint16>*) img.buffer();
    int n_pixel = img.n_pixel();
    
    if (channel == 0) {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].r >> 8 ];
    }
    else if (channel == 1) {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].g >> 8 ];
    }
    else {
      for (int i=0; i < n_pixel; i++)
        ++ histo.data[ buf[i].b >> 8 ];
    }
    histo.findMaxValue();  
}

/**+*************************************************************************\n
  Compute the histogram of an RGB_U16 image for all three channels.
  
  @param histos: Address of an array `HistogramData[3]´ of three histograms.
******************************************************************************/
void
compute_histogram_RGB_U16 (HistogramData * histos, const br::Image & img)
{
    CHECK_IMG_TYPE_RGB_U16 (img);

    histos[0].init (256);
    histos[1].init (256);
    histos[2].init (256);
    
    const Rgb<uint16>* buf = (Rgb<uint16>*) img.buffer();
    int n_pixel = img.n_pixel();
    
    for (int i=0; i < n_pixel; i++)
    {
      ++ histos[0].data[ buf[i][0] >> 8 ];
      ++ histos[1].data[ buf[i][1] >> 8 ];
      ++ histos[2].data[ buf[i][2] >> 8 ];
    }
      
    histos[0].findMaxValue();       // Wozu?
    histos[1].findMaxValue();       // Wozu?
    histos[2].findMaxValue();       // Wozu?
}

// END OF FILE
