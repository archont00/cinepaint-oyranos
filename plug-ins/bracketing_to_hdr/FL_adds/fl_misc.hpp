/*
 * fl_misc.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  fl_misc.hpp	-  little helpers for FLTK.
*/
#ifndef fl_misc_hpp
#define fl_misc_hpp

/**+*************************************************************************\n
  To enforce a window refresh, sometimes multiple Fl::check()s separeted by
  usleep(nano)'s are needed. By trial and error I found on my system n=7 and
  nano=10 to be enough. Let's hope, n=10, nano=15 should be enough overall.
******************************************************************************/
void fl_multi_check (int n=7, int nano=10);

/**+*************************************************************************\n
  Enforces a window refresh using fl_multi_check(). Values for \a n and \a nano
   should be chosen so that it works on every system.
******************************************************************************/
inline void fl_enforce_refresh() 	{fl_multi_check(7,10);}

#endif  // fl_misc_hpp

