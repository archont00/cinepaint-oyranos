/*
 * fl_misc.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  fl_misc.cpp	-  little helpers for FLTK.
*/
//extern "C" {
#include <unistd.h>	// usleep()
//}
#include <FL/Fl.H>	// Fl::check()
#include "fl_misc.hpp"  // prototypes

/**+*************************************************************************\n
  To enforce a window refresh, sometimes multiple Fl::check()s separeted by
  usleep(nano)'s are needed. By trial and error I found on my system n=7 and
  nano=10 to be enough. Let's hope, n=10, nano=15 should be enough overall.
******************************************************************************/
void fl_multi_check (int n, int nano)
{
  for (int i=0; i < n; i++)  {Fl::check(); usleep(nano);}
  Fl::check();
}

