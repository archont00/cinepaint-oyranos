/*
 * br_macros_varia.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_macros_varia.hpp  --  Macros intented to be changable from file to file
 
   - TRACE() -  
   - SPUR()  - 
   - DBG     - general debug info: file, line number, function
 
  Switch on by `#define BR_DEBUG', switch off by `#undef BR_DEBUG'.
 
  Diese Datei soll mehrfach eingelesen werden koennen, um einige der Makros
   fuer jede Quelldatei neu definieren (an- und abschalten) zu koennen, daher
   hier kein `#ifndef br_macros_varia_hpp'-Schalter. Damit die Umdefinitionen
   keine "redefine"-Warnungen produzieren, werden die Makros zuerst entdefiniert.
*/
// #ifndef br_macros_hpp     // No! We want to allow multiple includings!
// #define br_macros_hpp


// First we undefine all macros to avoid "redefine" warnings
#undef TRACE
#undef SPUR
#undef DBG
#undef DBG_CALL


#ifdef BR_DEBUG  
#  include "br_macros.hpp"   // br_vprintf()

   // define active macros
#  define TRACE(args)    printf ("%s(): ",__func__); br_vprintf args;
#  define SPUR(args)     br_vprintf args;
#  define DBG            printf("[%s, Line %d]:  %s()...\n", __FILE__,__LINE__,__func__);
#  define DBG_CALL(arg)  arg;
#else
   // define empty macros
#  define TRACE(args)     
#  define SPUR(args)      
#  define DBG    
#  define DBG_CALL(arg)
#endif  // BR_DEBUG


// #endif  // br_macros_varia_hpp

// END OF FILE
