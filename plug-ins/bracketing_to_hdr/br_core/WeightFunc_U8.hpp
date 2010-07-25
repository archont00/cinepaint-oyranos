/*
 * WeightFunc_U8.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  WeightFunc_U8.hpp  --  weight function objects for U8 data.
  
  Contents:
   - WeightFunc_U8      --  abstract base class of U8 weight function objects
   - WeightFunc_U8_Identity
   - WeightFunc_U8_Identity_0
   - WeightFunc_U8_Triangle
   - WeightFunc_U8_Sinus
   - WeightFunc_U8_Identity_0_Param
   - WeightFunc_U8_Triangle_Param
   
   - new_WeightFunc_U8()
   - list()
*/
#ifndef WeightFunc_U8_hpp
#define WeightFunc_U8_hpp

#include <cmath>                // sin(), M_PI

#include "WeightFuncBase.hpp"   // WeightFuncBase, WeightFunc<>
#include "br_types.hpp"         // uint8



/**===========================================================================
  
  @class WeightFunc_U8
  
  Abstract base class of the U8 weight objects.

*============================================================================*/
class WeightFunc_U8 : public WeightFunc <uint8>
{
protected:
    /*  Ctor protected to avoid attempted instances */
    WeightFunc_U8 (Shape shape) : WeightFunc<uint8> (shape, 255)   {}
};


/**==========================================================================\n
  @class WeightFunc_U8_Identity     
  
  Identity function: w(z)=1 for all z.
*============================================================================*/
class WeightFunc_U8_Identity : public WeightFunc_U8
{
public:
    WeightFunc_U8_Identity() : WeightFunc_U8 (IDENTITY)  // Ctor
      {
        set_minval (1.0);
        set_maxval (1.0);
        set_what (__func__);
      }
    
    Tweight operator() (uint8 z) const          // virtual Identity function
      {
        return 1.0;
      }
};

/**==========================================================================\n
  @class WeightFunc_U8_Identity_0
       
  Identity function with 0-borders at z=0 and z=zmax.
*============================================================================*/
class WeightFunc_U8_Identity_0 : public WeightFunc_U8
{
public:
    WeightFunc_U8_Identity_0() : WeightFunc_U8 (IDENTITY_0)  // Ctor
      {
        set_minval (0.0);
        set_maxval (1.0);
        set_what (__func__);
      }
    
    Tweight operator() (uint8 z) const          // virtual Identity_0 function
      {
        if (z==0 || z==255) return 0.0; else return 1.0;
      }
};

/**==========================================================================\n
  @class WeightFunc_U8_Triangle
  
  Triangle shape function (symmetric) with 0-borders at z=0 and z=zmax.
*============================================================================*/
class WeightFunc_U8_Triangle : public WeightFunc_U8
{
public:
    WeightFunc_U8_Triangle() : WeightFunc_U8 (TRIANGLE)  // Ctor
      {
        set_minval (0.0);
        set_maxval ((*this) (127));             // maximum at the middle
        set_what (__func__);
      }
    
    Tweight operator() (uint8 z) const          // virtual triangle function; could
      {                                         //  be simplified by zmin==0.
        //return (z <= zmid_) ? (z - zmin_) : (zmax_ - z);
        return ((z <= 127) ? (z - 0) : (255 - z)) / 127.;
      }
};

/**==========================================================================\n
  @class WeightFunc_U8_Sinus      
  
  Sinus shape function. Only for tests.
*============================================================================*/
class WeightFunc_U8_Sinus : public WeightFunc_U8
{
    Tweight factor_;

public:
    WeightFunc_U8_Sinus() : WeightFunc_U8 (SINUS)
      {
        factor_ = M_PI / 255.;                  // before any use of (*this)()!
        set_minval (0.0);
        set_maxval ((*this) (127));             // maximum at the middle
        set_what (__func__);
      }
    
    Tweight operator() (uint8 z) const          // virtual Sinus function
      {
        return sin (z * factor_);               // Ergebnis '0' an den Raendern
      }                                         //  sollte garantiert werden!
};


/**==========================================================================\n
  @class WeightFunc_U8_Identity_0_Param     
  
  Identity function with parametric 0-borders: w(z)=0 for z in [0...z_left], 
   [z_right...zmax].
*============================================================================*/
class WeightFunc_U8_Identity_0_Param : public WeightFunc_U8
{
    uint  z_left_;   /// Last left z with w(z)=0. `uint' faster than Tweight.
    uint  z_right_;  /// First right z with w(z)=0. `uint' faster than Tweight.
    
public:
    WeightFunc_U8_Identity_0_Param() : WeightFunc_U8 (IDENTITY_0_PARAM)
      {
        set_param (1, 254);                     // before any use of (this*)()!
        set_minval (0.0);
        set_maxval ((*this) (127));             // maximum at the middle
        set_what (__func__);
      }
      
    /** In dieser Form (mit "<=" und ">=") Nicht-0-Raender unmoeglich, z_left, z_right
        werden interpretiert als die innersten 0-Punkte. Mit "<" und ">" waeren 
        sie die aeussersten Nicht-0-Punkte, und Nicht-0-Raender waeren moeglich.
    */
    Tweight operator() (uint8 z) const          // virtual function
      {
        if (z <= z_left_ || z >= z_right_) return 0.0; else return 1.0;
      }
    
    /** @param z_left: From left the innermost z-value with w(z)=0.
        @param z_right: From right the innermost z-value with w(z)=0.
     */  
    void set_param (uint8 z_left, uint8 z_right)
      {
        z_left_  = z_left;
        z_right_ = z_right;
      }
};

/**==========================================================================\n
  @class WeightFunc_U8_Triangle_Param     
  
  Parametric triangle shape function. 
*============================================================================*/
class WeightFunc_U8_Triangle_Param : public WeightFunc_U8
{
    Tweight  z_left_;   /// Last left z with w(z)=0. Tweight faster than uint[8].
    Tweight  z_right_;  /// First right z with w(z)=0. Tweight faster than uint[8].
    Tweight  d_left_;
    Tweight  d_right_;
    
public:
    WeightFunc_U8_Triangle_Param() : WeightFunc_U8 (TRIANGLE_PARAM)
      {
        set_param (1, 254);                     // before any use of (this*)()!
        set_minval (0.0);
        set_maxval ((*this) (127));             // maximum at the middle
        set_what (__func__);
      }
      
    /**  In dieser Form - "(z-zLeft)/d" -  Nicht-0-Raender nur moeglich mit zLeft<0
         bzw. zRight>zmax. Mit unserem Tweight statt Unsign sogar moeglich. 
     */
    Tweight operator() (uint8 z) const          // virtual triangle function
      {
        Tweight res;
        
        if (z <= 127) 
          if (z > z_left_) res = (z - z_left_) / d_left_;
          else             res = 0.0;
        else
          if (z < z_right_) res = (z_right_ - z) / d_right_;
          else              res = 0.0;  
        
        return res;
      }
    
    /** @param z_left: From left the innermost z-value with w(z)=0.
        @param z_right: From right the innermost z-value with w(z)=0.
        Type `Tweight' instead of `uint8' to allow zLeft<0 or zRight>zmax.
     */  
    void set_param (Tweight z_left, Tweight z_right)
      {
        z_left_  = z_left;
        z_right_ = z_right;
        d_left_  = 127 - z_left;
        d_right_ = z_right - 128;
      }
};



/*========================================
                utilities
=========================================*/   

/**==========================================================================\n
  Create a dynamic WeightFunc_U8 object of a given shape. 
  @return Pointer to the object or NULL, if creation fails.
*============================================================================*/
WeightFunc_U8*  new_WeightFunc_U8  (WeightFuncBase::Shape);


/**==========================================================================\n
  List a WeightFunc_U8 object on stdout.
*============================================================================*/
void list (const WeightFunc_U8 &w, uint from=0, uint to=5);


#endif // WeightFunc_U8_hpp

// END OF FILE
