/*
 * HistogramData.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HistogramData.cpp
*/

#include "HistogramData.hpp"

using namespace TNT;


/**+*************************************************************************\n
  Init the histogram for `N' entrees (recreate if nessecary; preset with 0).
******************************************************************************/
void HistogramData::init (int N)
{
    if (data.dim() != N)
      data = Array1D<unsigned> (N, unsigned(0));
    else
      for (int i=0; i < data.dim(); i++)  data[i]=0;  
    
    max_value = 0;    
}

/**+*************************************************************************\n
  Find the max value of the histogram data.
******************************************************************************/
void HistogramData::findMaxValue()
{
    max_value = 0;
    for (int i=0; i < data.dim(); i++)  // save also for data.is_null()
      if (data[i] > max_value) 
        max_value = data[i];
}


// END OF FILE
