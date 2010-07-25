/*
 * HistogramData.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HistogramData.hpp
*/
#ifndef HistogramData_hpp
#define HistogramData_hpp

#include "TNT/tnt_array1d.hpp"


/**+*************************************************************************\n
  @struct HistogramData  -  Datentyp-unabh. Histogram-Struktur
  
  FRAGE: Als Histogrammwerte `unsigned´ oder ein Float-Typ? Unsigned sachlich
   angemessen, andererseits sind CurvePlot-Daten stets Float, dh. fuer Plot 
   waere zusaetzlich stets Bereitstellung eines Float-Feldes vonnoeten. Derzeit
   unsigned und in den HistogramPlot-Routinen wird auf `PlotValueT´ umkopiert.
   
  FEHLT noch Information ueber x-Bereich. 
******************************************************************************/

struct HistogramData
{
    unsigned  max_value;                // if 32-bit: until 4.29 x 10^9
    TNT::Array1D<unsigned>  data;
    
    HistogramData (int N=0)             {init(N);}
    
    void init (int N);
    void findMaxValue();
};


#endif  // HistogramData_hpp

// END OF FILE
