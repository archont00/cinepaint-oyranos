/*
 * DisplcmFinderCorrel_RGB.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderCorrel_RGB.hpp
*/
#ifndef DisplcmFinderCorrel_RGB_hpp
#define DisplcmFinderCorrel_RGB_hpp


#include "DisplcmFinderCorrel.hpp"      // DispclmFinderCorrel
#include "CorrelMaxSearch_RGB.hpp"      // CorrelMaxSearch_RGB<>
#include "br_Image.hpp"                 // br:Image
#include "Scheme2D.hpp"                 // Scheme2D<>
#include "Rgb.hpp"                      // Rgb<>



/**===========================================================================
  
  @class DisplcmFinderCorrel_RGB_U8

  Task: Define the abstract function new_search_tool() for RGB_U8 data.

=============================================================================*/
class DisplcmFinderCorrel_RGB_U8 : public DisplcmFinderCorrel
{
  public:
    DisplcmFinderCorrel_RGB_U8()  {}

  protected:    
    //------------------------------
    //  Define the abstract routine
    //------------------------------
    CorrelMaxSearchBase* new_search_tool
                              ( const VoidScheme2D & schemeA,
                                const VoidScheme2D & schemeB )
      {
         Scheme2D <Rgb<uint8> >  A (schemeA);
         Scheme2D <Rgb<uint8> >  B (schemeB);
         
         return new CorrelMaxSearch_RGB <uint8> (A,B);
      }
};


/**===========================================================================
  
  @class DisplcmFinderCorrel_RGB_U16

  Task: Define the abstract function new_search_tool() for RGB_U16 data.

=============================================================================*/
class DisplcmFinderCorrel_RGB_U16 : public DisplcmFinderCorrel
{
  public:
    DisplcmFinderCorrel_RGB_U16()  {}

  protected:    
    //------------------------------
    //  Define the abstract routine
    //------------------------------
    CorrelMaxSearchBase* new_search_tool
                              ( const VoidScheme2D & schemeA,
                                const VoidScheme2D & schemeB )
      {
         Scheme2D <Rgb<uint16> >  A (schemeA);
         Scheme2D <Rgb<uint16> >  B (schemeB);
         
         return new CorrelMaxSearch_RGB <uint16> (A,B);
      }
};

#endif // DisplcmFinderCorrel_RGB_hpp
