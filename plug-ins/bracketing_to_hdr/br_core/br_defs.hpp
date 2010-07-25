/*
 * br_defs.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_defs.hpp
  
  Some global compiler switches, in particular debug switches.

*/
#ifndef br_defs_hpp
#define br_defs_hpp


/** 
   With correction of small displacements (currently without effect)
*/
//#define BR_WITH_SHIFT

/** 
   Trace br_RefHandle<> operations (old)
*/
//#define BR_DEBUG_HANDLE

/**
   Trace Ctor and Dtor calls
*/ 
#define BR_DEBUG_CTOR

/**
   Debug Br2HdrManager::Events
*/
#define BR_DEBUG_EVENT 

/**
   Debugging of `Receiver's
*/
//#define BR_DEBUG_RECEIVER

/**
   Debugging of Table(s)
*/
//#define BR_DEBUG_TABLE



#endif  // br_defs_hpp

// END OF FILE
