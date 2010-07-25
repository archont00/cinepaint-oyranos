/*
 * Rgb_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
   @file Rgb_utils.hpp
    
   Die Ausgabe der Rgb-Typen mittels std::cout. Nicht in Rgb.hpp, weil hierzu
    <ostream> zu inkludieren, was manchem vielleicht schon aufstoesst.

   FRAGE: Abschliessendes '\n' einbauen oder besser (wie bei den Standardtypen)
    nicht? Besser nicht!!
*/
#ifndef Rgb_utils_hpp
#define Rgb_utils_hpp

#include <ostream>
#include "Rgb.hpp"


/**
 *  Das generelle Template fuer Rgb's
 */
template <class T>
std::ostream& operator<< (std::ostream &s, const Rgb<T> &z)
{
//  s << "(r: " << z.r << ", g: " << z.g << ", b: " << z.b << ')';
//  s << "(R: " << z.r << ", G: " << z.g << ", B: " << z.b << ')';
    s << "(RGB: " << z.r << ", " << z.g << ", " << z.b << ')';

    return s;
}


/**
 *  Spezialisierung fuer unsigned char ("byte", "uint8"), die nicht als
 *   Zeichen, sondern gewoehnlich als Zahlen ausgegeben werden sollen.
 *
 *  As a fully specialized function templates it counts as common function,
 *   inlined, to spare a separate .cpp-file. 
 */
template<> inline
std::ostream& operator<< (std::ostream &s, const Rgb<unsigned char> &z)
{
/*    s <<  "(r: " << (int)z.r 
      << ", g: " << (int)z.g 
      << ", b: " << (int)z.b << ')';*/
/*    s << "(R: " << (int)z.r 
      << ", G: " << (int)z.g 
      << ", B: " << (int)z.b << ')';*/
    s << "(RGB: " << (int)z.r << ", " << (int)z.g << ", " << (int)z.b << ')';

    return s;
}


#endif  // Rgb_utils_hpp

// END OF FILE
