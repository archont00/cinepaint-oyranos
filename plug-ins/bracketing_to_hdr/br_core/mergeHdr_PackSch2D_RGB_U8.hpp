/*
 * mergeHdr_PackSch2D_RGB_U8.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file mergeHdr_PackSch2D_RGB_U8.hpp
  
  Merging of a Pack of typed \a ImgScheme2D schemes of RGB_U8 image buffers
   into one single HDR(float) Rgb-image using three given response curves, 
   a given vector of exposure times and a given weight function.
*/
#ifndef mergeHdr_PackSch2D_RGB_U8_hpp
#define mergeHdr_PackSch2D_RGB_U8_hpp


#include "TNT/tnt_array1d.hpp"          // TNT::Array1D<>
#include "br_PackImgScheme2D.hpp"       // PackImgScheme2D_RGB_U8
#include "br_Image.hpp"                 // br::Image
#include "WeightFunc_U8.hpp"            // WeightFunc_U8


namespace br {

/****************************************************************************\n
  merge_Hdr_RGB_U8(). Description see: ~.cpp file.
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_Hdr_RGB_U8 (      const br::PackImgScheme2D_RGB_U8 & pack,
                        const TNT::Array1D<double> & logX_R,
                        const TNT::Array1D<double> & logX_G,
                        const TNT::Array1D<double> & logX_B,
                        const WeightFunc_U8        & weight,
                        bool                         mark_bad_pixel     = false,
                        bool                         protocol_to_file   = false, 
                        bool                         protocol_to_stdout = false );


/****************************************************************************\n
  merge_LogHdr_RGB_U8(). Description see: ~.cpp file.  
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_LogHdr_RGB_U8 (   const br::PackImgScheme2D_RGB_U8 & pack,
                        const TNT::Array1D<double> & logX_R,
                        const TNT::Array1D<double> & logX_G,
                        const TNT::Array1D<double> & logX_B,
                        const WeightFunc_U8        & weight,
                        bool                         mark_bad_pixel     = false,
                        bool                         protocol_to_file   = false, 
                        bool                         protocol_to_stdout = false );


}  // namespace "br"

#endif // mergeHdr_PackSch2D_RGB_U8_hpp

// END OF FILE
