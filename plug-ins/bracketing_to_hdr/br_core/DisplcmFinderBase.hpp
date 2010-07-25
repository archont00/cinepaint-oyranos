/*
 * DisplcmFinderBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderBase.hpp
*/
#ifndef DisplcmFinderBase_hpp
#define DisplcmFinderBase_hpp


#include "br_Image.hpp"         // br:Image
#include "Vec2.hpp"             // Vec2<>, Vec2_int, 


/**===========================================================================
  
  @class DisplcmFinderBase
  
  Enthaelt alles was unabhaengig vom Datentyp der br::Image Daten (U8/U16) als
   auch von der Kanalzahl (RGB). Als auch von der speziellen Such-Methode 
   (CorrelMaxSearch oder CorrelMaxSearch_spec oder auch Folgewerte-Optimierung).
   
  Leistungen:
   - Funktionen zum Justieren von Suchgebieten.
   - Wartung einer Pattern-Variable
   - Funktionen zum Ueberpruefen der Groesse und der (Zentrums)Koordinaten von 
      Suchgebieten.

  @internal Wir und wollen hier unabhaengig sein von br::Image.

=============================================================================*/
class DisplcmFinderBase
{
  public:
    /*!  Constants concerning the offered arrangements of search areas. Obsolete. */
    enum Pattern 
    {  PATTERN_2x2,
       PATTERN_3x3
    };
    
  private:
    Pattern pattern_;  // obsolete
    
  public: 
    DisplcmFinderBase()
       : pattern_(PATTERN_2x2) 
      {}
    
    void pattern (Pattern p)            {pattern_ = p;}
    Pattern pattern() const             {return pattern_;}
    
  protected:  
#if 0  
    bool check_area_dims_     ( const Vec2_int &  sizeA,
                                const Vec2_int &  sizeB, 
                                int nx, int ny, int mx, int my )  const;
    
    bool check_center_coords_ ( const Vec2_int &  sizeA,
                                const Vec2_int &  sizeB, 
                                int  n_center, 
                                const Vec2_int*  centerA,
                                const Vec2_int*  centerB,
                                int nx, int ny, int mx, int my )  const;
#endif
    
    void justify_1d_centers   ( int width, int hw, int num, int* cntr );
    void justify_1d_origins   ( int width, int w, int num, int* orig );
    
    void justify_centers_for_PxQ      ( const Vec2_int &  sizeB, 
                                        int hwx, int hwy,
                                        int P, int Q,
                                        Vec2_int*  centerB );
    
    void justify_centers_for_PxQ      ( const Vec2_int &  sizeB, 
                                        const Vec2_int &  hw,
                                        int P, int Q,
                                        Vec2_int*  centerB )
      { justify_centers_for_PxQ (sizeB, hw.x, hw.y, P,Q, centerB); }
    
    void justify_origins_for_PxQ      ( const Vec2_int &  sizeB, 
                                        int wx, int wy, 
                                        int P, int Q,
                                        Vec2_int* originB );
    
    void justify_origins_for_PxQ      ( const Vec2_int &  sizeB,  // image size
                                        const Vec2_int &  w,      // area size
                                        int P, int Q,
                                        Vec2_int*  originB )
      { justify_origins_for_PxQ (sizeB, w.x, w.y, P,Q, originB); }
};


#endif // DisplcmFinderBase_hpp
