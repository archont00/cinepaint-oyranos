/*
 * DynArray1D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DynArray1D.hpp
*/
#ifndef DynArray1D_hpp
#define DynArray1D_hpp

/**============================================================================
  
  @class DynArray1D
  
  Primitive Array1D template with memory allocation in the Ctor and deletion
   in the Dtor using new/delete. Without Copy-Ctor, =-Op or ref. counting copying.
   Intended for local arrays (since ISO C++ forbids variable-size arrays) with
   automatic deallocation at its block end.
  
  FRAGE: Allokieren mittels malloc() oder new? New wirft, malloc wohl schneller.
   Wichtiger aber: New/delete beruecksichtigt eventuelle Ctor/Dtoren der Elemente,
   malloc/free erlaubt nur primitive Elemente. Daher new.
   
  @note Die new-Syntax `new T[n](t)´ ruft fuer T's mit passend. Init-Ctor diesen
   fuer jedes Feldelement auf. Leider funktioniert das nicht fuer eingebaute Typen
   al'a `new double[n](32.1)´. Daher im Init-Ctor expliztes Setzen noetig.

=============================================================================*/
template <typename T> 
class DynArray1D 
{
    int  dim_;                          // number of `T'-elements
    T*   data_;                         // points to the data begin

  public:
    
    /*!  Ctor without value init. */
    DynArray1D (int n)  : dim_(n)
      { data_ = new T[n];
      }
    
    /*!  Ctor with value init. Sets all elements to \a t. */
    DynArray1D (int n, const T& t)  : dim_(n)
      { data_ = new T[n];
        for (int i=n-1; i >= 0; i--) data_[i] = t;
      } 
    
    ~DynArray1D()                       {delete[] data_;}
    
    int dim() const                     {return dim_;}
    const T& operator[] (int i) const   {return data_[i];}
    T&       operator[] (int i)         {return data_[i];}
    operator const T*() const           {return data_;}   // Cast-Op
    operator T*()                       {return data_;}   // Cast-Op

  private:
    // Prevent copying and =-Op (unimplemented)
    DynArray1D (const DynArray1D &);
    DynArray1D & operator= (const DynArray1D &);
};

#endif  // DynArray1D_hpp

// END OF FILE
