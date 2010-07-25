/*
 * Scheme2D_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Scheme2D_utils.hpp
*/
#ifndef Scheme2D_utils_hpp
#define Scheme2D_utils_hpp


#include <ostream>
#include <typeinfo>

#include "Scheme2D.hpp"


/**+*************************************************************************\n
  Output of a 2D-Scheme on an std::ostream.
******************************************************************************/
template <typename T>
std::ostream & operator<< (std::ostream &s, const Scheme2D<T> &A)
{
    int M = A.dim1();
    int N = A.dim2();

    s << "Scheme2D<" << typeid(T).name()<< "> [ " 
      << M << " x " << N << " ]:\n";

    for (int i=0; i<M; i++)
    {
        s << " [" << i << "]\t";
        for (int j=0; j<N; j++)
        {
            s << A[i][j] << " ";
        }
        s << "\n";
    }

    return s;
}

/**+*************************************************************************\n
  Spezialisierung fuer unsigned chars, die ueblicherweise nicht als chars, 
   sondern als Zahlen ausgegeben werden sollen; der Einfachheit hier inline, 
   da -- vollstaendige Spezialisierung! -- sonst *.cpp Datei vonnoeten. Lohnt
   fuer so kleine Funktion nicht.
******************************************************************************/
template <> inline
std::ostream & operator<< (std::ostream &s, const Scheme2D<unsigned char> &A)
{
    int M = A.dim1();
    int N = A.dim2();

    s << "Scheme2D<" << typeid(unsigned char).name() << "> [ " 
      << M << " x " << N << " ]:\n";

    for (int i=0; i<M; i++)
    {
        s << " [" << i << "]\t";
        for (int j=0; j<N; j++)
        {
            s << (int)A[i][j] << " ";
        }
        s << "\n";
    }

    return s;
}


#endif  // Scheme2D_utils_hpp

// END OF FILE
