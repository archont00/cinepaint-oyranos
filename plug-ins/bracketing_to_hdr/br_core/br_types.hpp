/*
 * br_types.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_types.hpp 
    
  Some common used types. Requires <stdint.h>.
  
  @todo Should be included in namespace "br".
*/
#ifndef br_types_hpp
#define br_types_hpp


#include <stdint.h>             // uint16_t, uint32_t etc.


//namespace br {


/**
 *  Integer types of definite size:
 */
typedef int32_t         int32;
typedef int64_t         int64;
typedef uint8_t         uint8;
typedef uint16_t        uint16;
typedef uint32_t        uint32;
typedef uint64_t        uint64;


/**
 *  Type of our ID-numbers to identify images:
 */ 
typedef unsigned        ImageID;


/**
 *  Genauigkeit, in der ernste Rechnungen intern ausgefuehrt werden:
 */
typedef double          RealNum;     // "RealNumeric"


/**
 *  Typ der Exposure Werte (noch am probieren)
 */
//typedef double        Texpose;  


/**
 *  Type of plot data (needed by abstract interfaces of br_core classes to GUI)
 */
typedef double          PlotValueT;


/**
 *  RGB structure with alpha channel.
 *   Must alpha be neccesary of the same type? Anyhow, in CinePaint is it.
 */
template <typename T>          
struct Rgba                 
{                   
    T   r,g,b, alpha;
};


//}  //  namespace "br"

#endif    // br_types_hpp
