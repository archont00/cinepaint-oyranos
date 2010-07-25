/*
 * Vec2.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Vec2.hpp
*/
#ifndef Vec2_hpp
#define Vec2_hpp

/**===========================================================================

  @class  Vec2
 
  2D vector template.

=============================================================================*/
template <typename T>
struct Vec2 
{
    T   x,y;
    
    //-----------------------
    //  Type of the elements 
    //-----------------------
    typedef T   ValueType;
    
    //---------------
    //  Constructors
    //---------------
    Vec2() {}                   // Default Ctor, no zeroing - efficiency!
    
    explicit Vec2 (T _t)        // Init vector is (_t,_t)
      : x(_t), y(_t)  {}

    Vec2 (T _x, T _y)           // Init vector is (_x,_y)
      : x(_x), y(_y)  {}
    
    template <typename S>       // Converting Vec2<T> <-- Vec2<S>
    Vec2 (const Vec2<S> & V)
      : x(V.x), y(V.y) {}

    //---------------
    //  Index syntax
    //---------------
    T& operator[] (int i)               {return (&x)[i];}
    const T& operator[] (int i) const   {return (&x)[i];}
    
    //-----------------------
    //  Zuweisungsoperatoren
    //-----------------------
/*    template <typename S>             // done by (converting) Copy-Ctor
    Vec2<T>& operator= (const Vec2<S>& V)
      {  x = V.x;  y = V.y;
         return *this;
      }*/
    
    Vec2<T>& operator= (T t)            // Vec2<T> = T
      {  x=t;  y=t;
         return *this;
      }
    
    //---------------------------
    //  arithmetische Operatoren 
    //---------------------------
    Vec2<T> operator- ()                // unary "-"
      {  return Vec2<T> (-x, -y); 
      }
    
    template <typename S>               // Vec2<T> += Vec2<S>
    Vec2<T>& operator+= (const Vec2<S> & V)
      {  x += V.x; y += V.y;
         return *this; 
      }
    
    template <typename S>
    Vec2<T>& operator-= (const Vec2<S> & V)
      {  x -= V.x; y -= V.y;
         return *this; 
      }
    
    template <typename S>
    Vec2<T>& operator*= (const Vec2<S> & V)
      {  x *= V.x; y *= V.y;
         return *this; 
      }
    
    template <typename S>
    Vec2<T>& operator/= (const Vec2<S> & V)
      {  x /= V.x; y /= V.y;
         return *this; 
      }
    
    Vec2<T>& operator+= (T t)           // Vec2<T> += T
      {  x += t; y += t;
         return *this;
      }
    Vec2<T>& operator-= (T t)
      {  x -= t; y -= t;
         return *this;
      }
    Vec2<T>& operator*= (T t)
      {  x *= t; y *= t;
         return *this;
      }
    Vec2<T>& operator/= (T t)
      {  x /= t; y /= t;
         return *this;
      }
    
    //---------------------
    //  Extended interface
    //---------------------
    T trace() const
      {  return (x + y);  }
    
    T length2() const
      {  return (x*x + y*y);  } 
    
}; 


/////////////////////////////////////////////////////
///
///  Two-argument operators as non-member functions
///
/////////////////////////////////////////////////////

//-------------
//  Gleichheit
//-------------
template <typename T> inline
bool operator== (const Vec2<T> & V, const Vec2<T> & W)
{
    return (V.x == W.x  &&  V.y == W.y);
}

template <typename T> inline
bool operator!= (const Vec2<T> & V, const Vec2<T> & W)
{
    return (V.x != W.x  ||  V.y != W.y);
}

//----------------------------------
//  Zweistellige arithm. Operatoren
//----------------------------------
//-----------------
//  a) Vec2 @ Vec2
//-----------------                                
template <typename T> inline
Vec2<T> operator+ (const Vec2<T> & a, const Vec2<T> & b)
{
    return Vec2<T>(a) += b;
}

template <typename T> inline
Vec2<T> operator- (const Vec2<T> & a, const Vec2<T> & b)
{
    return Vec2<T>(a) -= b;
}

template <typename T> inline
Vec2<T> operator* (const Vec2<T> & a, const Vec2<T> & b)
{
    return Vec2<T>(a) *= b;
}

template <typename T> inline
Vec2<T> operator/ (const Vec2<T> & a, const Vec2<T> & b)
{
    return Vec2<T>(a) /= b;
}

//----------------------------------
//  b) Vec2<T> @ T  and T @ Vec2<T>
//----------------------------------
template <typename T> inline
Vec2<T> operator+ (const Vec2<T> & a, T t)
{
    return Vec2<T>(a) += t;
}

template <typename T> inline
Vec2<T> operator+ (T t, const Vec2<T> & a)
{
    return Vec2<T>(a) += t;
}

template <typename T> inline
Vec2<T> operator- (const Vec2<T> & a, T t)
{
    return Vec2<T>(a) -= t;
}

template <typename T> inline
Vec2<T> operator- (T t, const Vec2<T> & a)
{
    return Vec2<T>(t - a.r, t - a.g, t - a.b);
}

template <typename T> inline
Vec2<T> operator* (const Vec2<T> & a, T t)
{
    return Vec2<T>(a) *= t;
}

template <typename T> inline
Vec2<T> operator* (T t, const Vec2<T> & a)
{
    return Vec2<T>(a) *= t;
}

template <typename T> inline
Vec2<T> operator/ (const Vec2<T> & a, T t)
{
    return Vec2<T>(a) /= t;
}

template <typename T> inline
Vec2<T> operator/ (T t, const Vec2<T> & a)
{
    return Vec2<T>(t) /= a;
}


//--------------
//  dot product
//--------------
template <typename T> inline
T dot (const Vec2<T> & V, const Vec2<T> & W)
{
    return (V.x * W.x + V.y * W.y);
}


//////////////////////////////////////////////
///
///  typedefs for convinience
///
//////////////////////////////////////////////

typedef  Vec2 <short>           Vec2_short;  // "Vec2short" waere als "Vec *to* short"
typedef  Vec2 <unsigned short>  Vec2_ushort; //   missverstaendlich.
typedef  Vec2 <int>             Vec2_int;
typedef  Vec2 <unsigned>        Vec2_uint;
typedef  Vec2 <float>           Vec2_float;
typedef  Vec2 <double>          Vec2_double;


#endif  // Vec2_hpp
