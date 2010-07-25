/*
 * br_types_plot.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_types_plot.hpp

  Types for plotting (of Curves) in the bracketing_to_hdr project. 
   Not in "br_core/br_types.hpp" because of dependency on "CurveTnt.hpp" and i.e.
   dependency of FLTK.
*/
#ifndef br_types_plot_hpp
#define br_types_plot_hpp

#include "../br_core/br_types.hpp"      // PlotValueT
#include "../FL_adds/CurveTnt.hpp"      // CurveTntPlot<>


namespace br {


//typedef ::TNT::Array1D<PlotValueT>       Array1DPlot;  
typedef ::TNT::CurveTntPlot<PlotValueT>  CurvePlotClass;


}  // namespace

#endif  // br_types_plot_hpp

// END OF FILE
