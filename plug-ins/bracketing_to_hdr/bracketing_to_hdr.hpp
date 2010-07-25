/*
 * bracketing_to_hdr.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file bracketing_to_hdr.hpp  
  
  Interface for CinePaint functionality. Explicite CinePaint dependencies only
   in "bracketing_to_hdr.cpp". Here the interface to the rest of the program.

*/
#ifndef bracketing_to_hdr_hpp
#define bracketing_to_hdr_hpp

#include "br_core/Br2HdrManager.hpp"


#ifdef NO_CINEPAINT

  inline bool cpaint_load_image   (const char* fname, br::Br2HdrManager &, bool=true) {return false;}
  inline void cpaint_show_image   (const br::Image &)                      {}

#else

  bool cpaint_load_image          (const char* fname, br::Br2HdrManager &, bool interactive=true);
  void cpaint_show_image          (const br::Image &);

#endif  // #else NO_CINEPAINT


#endif  // bracketing_to_hdr_hpp

// END OF FILE
