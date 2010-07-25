/*
 * Scheme1D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Scheme1D.hpp
*/
#ifndef Scheme1D_hpp
#define Scheme1D_hpp


/**============================================================================

  @class Scheme1D
 
  Typecasted 1D view on a void* buffer. Provides \c "[]" access syntax. Copying
   allowed (default Copy-Ctor and =-Op do the right).

=============================================================================*/
template <typename T>
class Scheme1D
{
    int  dim_;                  // number of T-elements in `data´
    T*   data_;                 // points to the data begin

public:
    //-----------------------
    //  Type of the elements
    //-----------------------
    typedef T   ValueType;

    //---------------
    //  Constructors
    //---------------
    Scheme1D() : dim_(0), data_(0)    // Default Ctor: NULL scheme
      {}           
    Scheme1D (int dim, void* buf) 
      : dim_ (dim),
        data_ ((T*)buf)
      {}
    Scheme1D (int dim, const void* buf) 
      : dim_ (dim),
        data_ ((T*)buf)
      {}
    
    //------------
    //  Dimension
    //------------  
    int dim() const                     {return dim_;}
    
    //---------------
    //  Index access
    //---------------
    const T& operator[] (int i) const   {return data_[i];}
    T&       operator[] (int i)         {return data_[i];}
    
    //----------------------------
    //  Cast operators and data()
    //----------------------------
    operator const T*() const           {return data_;}  // Cast op
    operator T*()                       {return data_;}  // Cast op
    const T* data() const               {return data_;}
};


#endif  // Scheme1D_hpp

// END OF FILE
