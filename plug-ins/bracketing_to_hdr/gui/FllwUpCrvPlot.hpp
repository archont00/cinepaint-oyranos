/*
 * FllwUpCrvPlot.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  FllwUpCrvPlot.hpp
*/
#ifndef FllwUpCrvPlot_hpp
#define FllwUpCrvPlot_hpp


#include "br_types_plot.hpp"            // PlotValueT, CurvePlotClass
#include "../br_core/Br2Hdr.hpp"        // Br2Hdr::Event
#include "../br_core/EventReceiver.hpp" // EventReceiver


/**====================================================================
* 
*   @class  FollowUpCurvePlot
* 
*   Plot of the follow-up curves of the global Br2Hdr::Instance().
* 
*=====================================================================*/

class FollowUpCurvePlot : public br::CurvePlotClass, public br::EventReceiver
{
private:
    int  channel_;
    br::CurvePlotClass &  plot_;  // synonym to make the code clearer

public:
    FollowUpCurvePlot (int X, int Y, int W, int H, const char* la=0);
    ~FollowUpCurvePlot()        {DTOR(label())}
  
    void set_channel (int c)    {channel_ = c;}
    int  channel() const        {return channel_;}  // current channel to plot
  
    void build();
    void update()               {build(); redraw();}
    
    void handle_Event (br::Br2Hdr::Event);   // virtual EventReceiver inheritance
};


#endif  // FllwUpCrvPlot.hpp

// END OF FILE
