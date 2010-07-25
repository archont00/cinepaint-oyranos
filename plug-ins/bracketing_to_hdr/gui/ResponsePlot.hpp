/*
 * ResponsePlot.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ResponsePlot.hpp
  
*/
#ifndef ResponsePlot_hpp
#define ResponsePlot_hpp


#include "br_types_plot.hpp"            // PlotValueT, CurvePlotClass
#include "../br_core/Br2Hdr.hpp"        // br::Br2Hdr::Event
#include "../br_core/WeightFunc_U8.hpp"
#include "../br_core/WeightFunc_U16.hpp"
#include "../br_core/br_macros.hpp"     // CTOR(), DTOR()


/**===========================================================================
 
  @class  ResponsePlot
 
  Plot of response curves of the global Br2Hdr::Instance().
 
=============================================================================*/
class ResponsePlot : public br::CurvePlotClass
{
    typedef br::Br2Hdr::WhichCurves  WhichCurves;
    
    static const char*   title_str_[4];
    
    br::CurvePlotClass &  plot_;        // synonym to make code clearer
    WhichCurves  which_curves_;         // Which curves shall be plotted?
    bool  any_response_curve_;          // Is any response curve shown?
   
public:
    ResponsePlot (int X, int Y, int W, int H, const char* la=0);
    ~ResponsePlot()                             {DTOR(label())}
    
    void         build();
    void         update()                       {build(); redraw();}
    bool         add_outdated_box();
    WhichCurves  which_curves() const           {return which_curves_;}
    void         which_curves (WhichCurves w)   {which_curves_ = w;}

protected:
    void draw();        // virtual CurvePlotClass inheritance
  
private:
    void add_weight_U8  (const WeightFunc_U8 &,  Fl_Color, bool like_U16 = false);
    void add_weight_U16 (const WeightFunc_U16 &, Fl_Color);

    void draw_missing_info (const TNT::Array1D<double> & crvR,
                            const TNT::Array1D<double> & crvG, 
                            const TNT::Array1D<double> & crvB);

    TNT::Array1D<double>  xgrid256_for_U16();
};



#endif  // ResponsePlot_hpp

// END OF FILE
