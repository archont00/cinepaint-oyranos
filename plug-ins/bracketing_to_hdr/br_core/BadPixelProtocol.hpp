/*
 * BadPixelProtocol.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  BadPixelProtocol.hpp
*/
#ifndef BadPixelProtocol_hpp
#define BadPixelProtocol_hpp


#include <fstream>
#include "Rgb.hpp"


/**===========================================================================
  
  @class BadPixelProtocol
  
  Output a protocol of bad (unresolved) HDR pixels to file and/or stdout. Used
   by the merge_HDR_***() functions.
  
=============================================================================*/
class BadPixelProtocol 
{
    static const char*  default_fname_;
    
    std::ofstream  file_;
    bool           to_stdout_;          // output to stdout
    unsigned       lines_;              // counts the recorded "bad pixel" lines

public:
    BadPixelProtocol (const char* fname, bool to_stdout = false);
    
    void out (int x, int y, const Rgb<double>& sum_w, 
              const Rgb<float>& hdr_pixel,
              const Rgb<float>& val_min, 
              const Rgb<float>& val_max  );
              
    const std::ofstream & file() const          {return file_;}
    std::ofstream &       file()                {return file_;}
    
    static const char* default_fname()          {return default_fname_;}
};


#endif  // BadPixelProtocol_hpp

// END OF FILE
