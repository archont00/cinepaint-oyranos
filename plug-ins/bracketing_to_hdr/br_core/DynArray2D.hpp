/*
 * DynArray2D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DynArray2D.hpp
*/
#ifndef DynArray2D_hpp
#define DynArray2D_hpp

/**============================================================================
  
  @class DynArray2D
  
  Template. Primitive Array2D class with memory allocation in the Ctor and deletion
   in the Dtor using new/delete. Without Copy-Ctor, =-Op or ref. counting copying.
   Intended for local arrays (since ISO C++ forbids variable-size arrays) with
   automatic deallocation at the scope end.
  
  FRAGE: Allokieren mittels malloc() oder new? New wirft, malloc wohl schneller.
   Wichtiger: New/delete beruecksichtigt eventuelle Ctor/Dtoren der Elemente,
   malloc/free erlaubt nur primitive Elemente. Daher new!
   
=============================================================================*/
template <typename T>
class DynArray2D
{
    int  dim1_;                 // number of rows (outer dimension)
    int  dim2_;                 // number of columns (inner dimension)
    T*   data_;                 // points to the data begin

  public:
    //  Ctor without init
    DynArray2D (int dim1, int dim2)
      : dim1_(dim1), dim2_(dim2)
      { data_ = new T[dim1 * dim2];
      }
    //  Ctor with value init. Sets all elements to \a t.
    DynArray2D (int dim1, int dim2, const T& t)
      : dim1_(dim1), dim2_(dim2)
      { data_ = new T[dim1 * dim2];
        for (int i=dim1*dim2-1; i >= 0; i--) data_[i]=t;
      }
    //  Dtor
    ~DynArray2D()                     {delete[] data_;}
    
    int dim1() const                  {return dim1_;}
    int dim2() const                  {return dim2_;}
    
    const T* operator[] (int i) const {return & data_[ i * dim2_ ];}
    T*       operator[] (int i)       {return & data_[ i * dim2_ ];}
    
    const T& ref (int i, int j) const {return data_[ i * dim2_ + j ];}
    T&       ref (int i, int j)       {return data_[ i * dim2_ + j ];}
    
    operator const T*() const         {return data_;}   // Cast op
    operator T*()                     {return data_;}   // Cast op
    
    const T* data() const             {return data_;}

  private:
    // Prevent copying and =-Op (unimplemented)
    DynArray2D (const DynArray2D &);
    DynArray2D & operator= (const DynArray2D &);
};
     

#endif  // DynArray2D_hpp
