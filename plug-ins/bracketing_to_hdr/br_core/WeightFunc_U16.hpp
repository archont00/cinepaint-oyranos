/*
 * WeightFunc_U16.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  WeightFunc_U16.hpp  --  weight function objects for U16 data.
  
  Contents:
   - WeightFunc_U16      --  abstract base class of U16 weight function objects
   - WeightFunc_U16_Identity
   - WeightFunc_U16_Identity_0
   - WeightFunc_U16_Triangle
   - WeightFunc_U16_Sinus
   - WeightFunc_U16_Identity_0_Param
   - WeightFunc_U16_Triangle_Param
   
   - new_WeightFunc_U16()
   - list()
*/
#ifndef WeightFunc_U16_hpp
#define WeightFunc_U16_hpp

#include <cmath>                // sin(), M_PI

#include "WeightFuncBase.hpp"   // WeightFuncBase, WeightFunc<>
#include "br_types.hpp"         // uint16



/**==========================================================================\n

  @class WeightFunc_U16
  
  Abstract base class of the U16 weight objects.

*============================================================================*/
class WeightFunc_U16 : public WeightFunc <uint16>
{
protected:
    /*  Ctor protected to avoid attempted instances */
    WeightFunc_U16 (Shape shape) : WeightFunc<uint16> (shape, 65535)   {}
};


/**==========================================================================\n
  @class WeightFunc_U16_Identity
  
  Identity function: w(z)=1 for all z.
*============================================================================*/
class WeightFunc_U16_Identity : public WeightFunc_U16
{
public:
    WeightFunc_U16_Identity() : WeightFunc_U16 (IDENTITY)
      {
        set_minval (1.0);
        set_maxval (1.0);
        set_what(__func__);
      }
    
    Tweight operator() (uint16 z) const          // virtual Identity function
      {
        return 1.;
      }
};

/**==========================================================================\n
  @class WeightFunc_U16_Identity_0
  
  Identity function with 0-borders at z=0 and z=zmax.
*============================================================================*/
class WeightFunc_U16_Identity_0 : public WeightFunc_U16
{
public:
    WeightFunc_U16_Identity_0() : WeightFunc_U16 (IDENTITY_0)
      {
        set_minval (0.0);
        set_maxval (1.0);
        set_what (__func__);
      }
    
    Tweight operator() (uint16 z) const          // virtual Identity function
      {
        if (z==0 || z==65535) return 0.; else return 1.0;
      }
};

/**==========================================================================\n
  @class WeightFunc_U16_Triangle
  
  Triangle shape function (symmetric) with 0-borders at z=0 and z=zmax.
*============================================================================*/
class WeightFunc_U16_Triangle : public WeightFunc_U16
{
public:
    WeightFunc_U16_Triangle() :  WeightFunc_U16 (TRIANGLE)  // Ctor
      {
        set_minval (0.0);
        set_maxval ((*this) (32767));           // maximum at the middle
        set_what (__func__);
      }
    
    Tweight operator() (uint16 z) const         // virtual Triangle function; could
      {                                         //  be simplified by zmin==0.
        //return (z <= zmid_) ? (z - zmin_) : (zmax_ - z);
        return ((z <= 32767) ? (z - 0) : (65535 - z)) / Tweight(32767);
      }
};

/**==========================================================================\n
  @class WeightFunc_U16_Sinus     
  
  Sinus shape function. Only for tests.
*============================================================================*/
class WeightFunc_U16_Sinus : public WeightFunc_U16
{
    Tweight factor_;
    
public:
    WeightFunc_U16_Sinus() :  WeightFunc_U16 (SINUS)  // Ctor
      {
        factor_ = M_PI / 65535.;                // before any use of (*this)()!
        set_minval (0.0);
        set_maxval ((*this) (32767));           // maximum at the middle
        set_what (__func__);
      }
    
    Tweight operator() (uint16 z) const         // virtual Sinus function
      {
        return sin (z * factor_);               // Ergebnis '0' an den Raendern         
      }                                         //  sollte garantiert werden!
};

/**==========================================================================\n
  @class WeightFunc_U16_Identity_0_Param     
  
  Identity function with parametric 0-borders: w(z)=0 for z in [0...z_left],
   [z_right...zmax].
*============================================================================*/
class WeightFunc_U16_Identity_0_Param : public WeightFunc_U16
{
    uint  z_left_;   /// Last left z with w(z)=0. `uint' faster than Tweight.
    uint  z_right_;  /// First right z with w(z)=0. `uint' faster than Tweight.
    
public:
    WeightFunc_U16_Identity_0_Param() : WeightFunc_U16 (IDENTITY_0_PARAM)
      {
        set_param (1, 65534);                   // before any use of (this*)()!
        set_minval (0.0);
        set_maxval ((*this) (32767));           // maximum at the middle
        set_what (__func__);
      }
      
    /** In dieser Form (mit "<=" und ">=") Nicht-0-Raender unmoeglich, z_left, z_right
        werden interpretiert als die innersten 0-Punkte. Mit "<" und ">" waeren 
        sie die aeussersten Nicht-0-Punkte, und Nicht-0-Raender waeren moeglich.
    */
    Tweight operator() (uint16 z) const         // virtual function
      {
        if (z <= z_left_ || z >= z_right_) return 0.0; else return 1.0;
      }
    
    /** @param z_left: From left the innermost z-value with w(z)=0.
        @param z_right: From right the innermost z-value with w(z)=0.
     */  
    void set_param (uint16 z_left, uint16 z_right)
      {
        z_left_  = z_left;
        z_right_ = z_right;
      }
};

/**==========================================================================\n
  @class WeightFunc_U16_Triangle_Param
  
  Parametric triangle shape function. 
*============================================================================*/
class WeightFunc_U16_Triangle_Param : public WeightFunc_U16
{
    Tweight  z_left_;   /// Last left z with w(z)=0. Tweight faster than uint[8].
    Tweight  z_right_;  /// First right z with w(z)=0. Tweight faster than uint[8].
    Tweight  d_left_;
    Tweight  d_right_;
    
public:
    WeightFunc_U16_Triangle_Param() : WeightFunc_U16 (TRIANGLE_PARAM)
      {
        set_param (1, 65534);                   // before any use of (*this)()!
        set_minval (0.0);
        set_maxval ((*this) (32767));           // maximum at the middle
        set_what (__func__);
      }
      
    /*! In dieser Form - "(z-zLeft)/d" - Nicht-0-Raender nur moeglich mit zLeft<0
         bzw. zRight>zmax. Mit unserem Tweight statt Unsign sogar moeglich. 
     */
    Tweight operator() (uint16 z) const          // virtual triangle function
      {
        Tweight res;
        
        if (z <= 32767) 
          if (z > z_left_) res = (z - z_left_) / d_left_;
          else             res = 0.0;
        else
          if (z < z_right_) res = (z_right_ - z) / d_right_;
          else              res = 0.0;  
        
        return res;
      }
      
    /*! @param z_left: From left the innermost z-value with w(z)=0.
        @param z_right: From right the innermost z-value with w(z)=0.
        Type `Tweight' instead of `uint16' to allow zLeft<0 or zRight>zmax.
     */  
    void set_param (Tweight z_left, Tweight z_right)
      {
        z_left_  = z_left;
        z_right_ = z_right;
        d_left_  = 32767 - z_left;
        d_right_ = z_right - 32768;
      }
};



/*========================================
                utilities
=========================================*/   

/**==========================================================================\n
  Create a dynamic WeightFunc_U16 object of a given shape. 
  @return Pointer to the object or NULL, if creation fails.
*============================================================================*/
WeightFunc_U16* new_WeightFunc_U16 (WeightFuncBase::Shape);


/**==========================================================================\n
  List a WeightFunc_U16 object on stdout.
*============================================================================*/
void list (const WeightFunc_U16 &w, uint from=0, uint to=5);


#endif // WeightFunc_U16_hpp

// END OF FILE
