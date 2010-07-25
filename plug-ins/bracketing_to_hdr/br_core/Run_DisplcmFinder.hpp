/*
 * Run_DisplcmFinder.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
/*!
  Run_DisplcmFinder.hpp
*/
#ifndef Run_DisplcmFinder_hpp
#define Run_DisplcmFinder_hpp

/*! 
  Klasse nur zum Zusammenfassen (Namensbereich waere Alternative), alle Funktionen
   statisch.
*/
struct Run_DisplcmFinder
{
    static void  run_single (int iA, int iB);
    static void  run_for_all();
    static void  run_for_all_actives();
};

#endif  // Run_DisplcmFinder_hpp
