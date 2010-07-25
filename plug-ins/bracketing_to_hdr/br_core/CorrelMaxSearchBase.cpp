/*
 * CorrelMaxSearchBase.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  CorrelMaxSearchBase.cpp
*/

#include "CorrelMaxSearchBase.hpp"

/**+*************************************************************************\n
  Hilfsroutine der LO-search()-Funktionen, die aufzurufen ist, nachdem das
   klassenglobale Canyonfeld \a canyon_ bestellt wurde. Bestimmt, ob die
   ermittelte Verrueckung \a d (LO-Angabe!) auf dem Rand des Suchgebietes liegt
   (setzt \a border_reached_), und ermittelt den minimalen Canyonfeld-Gradienten
   in der [+1,-1]-Umgebung der Verrueckung \a d (setzt \a min_gradient_).
******************************************************************************/
void  CorrelMaxSearchBase :: eval_result (const Vec2_int & d)
{
    int X = d.x;
    int Y = d.y;
    float val = canyon_[Y][X];
    float mi = 1e30;  // maximal canyon value is (should be) +1 
    border_reached_ = (X == 0 || X == canyon_.dim2()-1 ||
                       Y == 0 || Y == canyon_.dim1()-1 );
    if (X > 0) mi = std::min (mi, canyon_[Y][X-1] - val);
    if (Y > 0) mi = std::min (mi, canyon_[Y-1][X] - val);
    if (X < canyon_.dim2()-1) mi = std::min (mi, canyon_[Y][X+1] - val);
    if (Y < canyon_.dim1()-1) mi = std::min (mi, canyon_[Y+1][X] - val);
    min_gradient_ = mi;
}


// END OF FILE
