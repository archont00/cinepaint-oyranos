/*
 * br_types_array.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_types_array.hpp 
    
  File-transcend used types with TNT::Array dependency; that's why not
   in "br_types.hpp".
   
  Frage: In Namensbereich "br" stellen? 
*/
#ifndef br_types_array_hpp
#define br_types_array_hpp


#include "TNT/tnt_array2d.hpp"


/**===========================================================================
  
  Statistik-Struktur benoetigt von den Klassen `FollowUpValues_*' und 
   `Z_MatrixGenerator_*'.
 
  Die Summation muss in double oder uint64 ausgefuehrt werden, zur Aufbewahrung
   der Ergebnisse aber reichen float. Der Platz fuer sizeof(2*float + int) = 12
   ist sogar kleiner noch als der von  ChSum mit sizeof(2*double) = 16.

*============================================================================*/
struct ExpectFollowUpValue 
{
    float  z;     // Expectation (follow-up) value, = EX
    float  dz;    // Streuung, DX^2 = EX^2 - (EX)^2 (o. "Dz2" o. "sigma"?)
    size_t n;     // Number of values contributing to `z´ resp. `dz´.
  
    ExpectFollowUpValue() : z(0), dz(0), n(0)  {}
};


/**==========================================================================\n
  2d-Feld obiger statistischer Tripel.
*============================================================================*/
typedef TNT::Array2D <ExpectFollowUpValue>  ChannelFollowUpArray;



// Note: Da `n' (zu einem Hauptwert) fuer alle Arrays eines `Packs' identisch,
//  ist die bildweise Speicherung von `n' Verschwendung. Die bessere Struktur
//  waere
//
// struct EFV {
//   float  z [nlayer];   Feld der `nlayers' Erwartungsfolgewerte aller Arrays
//   float  dz [nlayer];  Feld der zugehoerigen Varianzen
//   size_t n;            Anzahl beitragender Punkte (Haupt- wie Folgewerte)
// };
// Array1D<EFV> (n_zvals); Feld dieser Daten fuer alle moeglichen z-Werte



#endif  // br_types_array_hpp

// END OF FILE

