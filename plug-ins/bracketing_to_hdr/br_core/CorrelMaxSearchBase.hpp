/*
 * CorrelMaxSearchBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  CorrelMaxSearchBase.hpp
*/
#ifndef CorrelMaxSearchBase_hpp
#define CorrelMaxSearchBase_hpp


#include "Vec2.hpp"                  // Vec2_int
#include "TNT/tnt_array2d.hpp"       // TNT::Array2D<>  (canyon feld)


/*!
  Kuenftig braucht nur die search()-Funktion noch virtuell sein, search_sym()
   ist als Huelle schon hier in der Basisklasse definierbar. Nur zur Zeit dort
   noch Tests.
*/
class CorrelMaxSearchBase {

  protected:
    
    TNT::Array2D <float>        canyon_;
    float                       rho_max_;
    float                       min_gradient_;
    bool                        border_reached_;
    
    void eval_result (const Vec2_int & d);

  public:
    //--------------
    //  Constructor
    //--------------
    CorrelMaxSearchBase()
       : rho_max_(-2.0f), min_gradient_(0.0f), border_reached_(false)
      {}
    
    virtual ~CorrelMaxSearchBase()  {}  // Virtuality NEEDED, we do a `delete basePtr'!

    //----------------------------------
    //  Abstract: the search routines
    //----------------------------------
    virtual   
    Vec2_int  search          ( int xA, int yA, int Nx, int Ny,
                                int xB, int yB, int Mx, int My ) = 0;
    
    virtual                                
    Vec2_int  search_sym      ( int nx, int ny, int mx, int my,
                                int xcA, int ycA, int xcB, int ycB ) = 0;
    
    //-----------------------------------
    //  Wrappers with Vec2_int arguments
    //-----------------------------------                            
    Vec2_int  search          ( const Vec2_int & origA, const Vec2_int & N,
                                const Vec2_int & origB, const Vec2_int & M )
       { return search (origA.x, origA.y, N.x, N.y, origB.x, origB.y, M.x, M.y); }
    
    Vec2_int  search_sym      ( const Vec2_int & n, 
                                const Vec2_int & m,
                                const Vec2_int & cntrA, 
                                const Vec2_int & cntrB )
       { return search_sym (n.x, n.y, m.x, m.y, cntrA.x, cntrA.y, cntrB.x, cntrB.y); }
    
    //------------------
    //  Publish results 
    //------------------
    float rho_max() const
       { return rho_max_; }
    
    const TNT::Array2D <float>& canyon() const 
       { return canyon_; }
    
    float min_gradient() const
       { return min_gradient_; }
    
    bool border_reached() const
       { return border_reached_; }
    
    bool has_gradient() const
       { return (min_gradient_ != 0.0f); }
};


#endif // CorrelMaxSearchBase_hpp
