/*
 * Vec2_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file Vec2_utils.hpp
    
  Die Ausgabe der Vec2-Typen mittels std::ostream. Nicht in Vec2.hpp, weil 
   <ostream> als zusaetzliche Abhaengigkeit zu inkludieren.
*/
#ifndef Vec2_utils_hpp
#define Vec2_utils_hpp


#include <ostream>
#include "Vec2.hpp"


/**+*************************************************************************\n
  Das generelle Template fuer Vec2's
******************************************************************************/
template <class T>
std::ostream& operator<< (std::ostream & s, const Vec2<T> & V)
{
    return s << "Vec2 (" << V.x << ", " << V.y << ')';
}


/**+*************************************************************************\n
  Spezialisierung fuer unsigned char ("byte", "uint8"), die nicht als
   Zeichen, sondern gewoehnlich als Zahlen ausgegeben werden sollen.

  As a fully specialized function templates it counts as common function;
   inlined, to spare a separate .cpp-file. 
******************************************************************************/
template<> inline
std::ostream& operator<< (std::ostream & s, const Vec2<unsigned char> & V)
{
    return s << "Vec2 (" << (int)V.x << ", " << (int)V.y << ')';
}


#endif  // Vec2_utils_hpp

// END OF FILE
