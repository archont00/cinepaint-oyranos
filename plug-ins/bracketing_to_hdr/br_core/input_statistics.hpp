/*
 * input_statistics.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file input_statistics.hpp
  
  Content:
   - compute_input_stats_RGB_U8()
   - compute_input_stats_RGB_U8_ref()   [test only]
   - compute_input_stats_RGB_U8_A2()    [test only]
  
  Notes about speed: 
   - "-O2":\n 
     o "const Rgb<> &val = A[i][j]" as fast as multiple used "A[i][j]"
        as fast as pointer variant \n
     o using a real value (copy): slower
   - "without -O":\n
     o reference as fast as copied value \n
     o multiple used "A[i][j]" much more slower (inline not inlined)\n
     o pointer variant fastest
*/
#ifndef input_statistics_hpp
#define input_statistics_hpp


#include "br_Image.hpp"


namespace br {


/**+*************************************************************************\n
  compute_input_stats_RGB_U8()
 
   - average brightness per pixel ("(R+G+B)/npixel", i.e. max is not
      255, but 3*255 = 765).
   - number of pixels at the lower and the upper limit
   - ratio of pixels within the working range
 
  Welcher Summen-Typ bei welchem Unsign-Typ?
   - Unsign == uint8:\n
     10 Megapixel: 10^7 * 255 = 2.55*10^9 <--> max(uint32) == 4.295 * 10^9. 
     Also uint32-Summe reichte nur bis 10 Megapixeln.
   - Unsign == uint16:\n
     10 Megapixel: 10^7 * 65353 = 6.5*10^11 <--> max(uint64) == 1.84 * 10^19.
     Also uint64 fuer beide notwendig, aber auch ausreichend.
 
  Moegl. waere auch, Kanaele zunaechst getrennt und dann Plausibilitaets-Check.
   Summe in uint64-Typ statt double ist genauer, aber auf meiner Maschine
   (32-bit, GCC) kaum schneller (0.14 vs. 0.16)

******************************************************************************/
void compute_input_stats_RGB_U8     (br::BrImage &);
void compute_input_stats_RGB_U8_ref (br::BrImage &);
void compute_input_stats_RGB_U8_A2  (br::BrImage &);

void compute_input_stats_RGB_U16    (br::BrImage &);

}  // namespac "br"

#endif  // input_statistics_hpp

// END OF FILE
