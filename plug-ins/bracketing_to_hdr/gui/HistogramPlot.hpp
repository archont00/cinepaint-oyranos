/*
 * HistogramPlot.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HistogramPlot.hpp
*/
#ifndef HistogramPlot_hpp
#define HistogramPlot_hpp


#include "br_types_plot.hpp"      // CurvePlotClass


/**===========================================================================

  @class  HistogramPlot

  Plot of a histogram of an image of the global Br2Hdr::Instance().
  
  @note Using in FLUID needs setting "Style" -> "Box" -> "FLAT BOX", otherwise
   the draw_box() in Fl_Cartesius::draw() doesn't clear the canvas.

*============================================================================*/

class HistogramPlot : public br::CurvePlotClass
{
private:
    int                  pic_;        // index of the image to plot
    int                  channel_;    // channel (-1 == "all channels")
    br::CurvePlotClass & plot_;       // synonym to make the code clearer

public:
    HistogramPlot (int X, int Y, int W, int H, const char* la=0);
    ~HistogramPlot()            {DTOR(label())}
  
    void set_image (int i)      {pic_ = i;}
    int  image() const          {return pic_;}     // current image to plot
    void set_channel (int c)    {channel_ = c;}
    int  channel() const        {return channel_;} // current channel to plot
  
    void build();
    void update()               {build(); redraw();}

private:    
    void build_single_channel();
    void build_all_channels();
};


#endif  // HistogramPlot_hpp

// END OF FILE
