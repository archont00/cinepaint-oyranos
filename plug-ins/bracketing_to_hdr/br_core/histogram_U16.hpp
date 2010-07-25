/*
 * histogram_U16.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  histogram_U16.hpp
  
  Computation of histograms for U16 images.
*/
#ifndef histogram_U16_hpp
#define histogram_U16_hpp

#include "HistogramData.hpp"
#include "br_Image.hpp"


void
compute_histogram_RGB_U16 (HistogramData &, const br::Image &, int channel);

void
compute_histogram_RGB_U16_switch (HistogramData &, const br::Image &, int channel);

void
compute_histogram_RGB_U16 (HistogramData *, const br::Image &);


#endif  // histogram_U16_hpp

// END OF FILE
