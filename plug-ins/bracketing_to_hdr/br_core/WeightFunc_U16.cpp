/*
 * WeightFunc_U16.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  WeightFunc_U16.cpp
*/

#include <iostream>
#include "WeightFunc_U16.hpp"


using std::cout;


/**==========================================================================\n
  new_WeightFunc_U16()
*============================================================================*/
WeightFunc_U16* new_WeightFunc_U16 (WeightFuncBase::Shape shape)
{
    WeightFunc_U16* p;

    switch (shape) 
    {
    case WeightFuncBase::IDENTITY:
        p = new WeightFunc_U16_Identity; break;
      
    case WeightFuncBase::IDENTITY_0:
        p = new WeightFunc_U16_Identity_0; break;
      
    case WeightFuncBase::TRIANGLE:
        p = new WeightFunc_U16_Triangle; break;
      
    case WeightFuncBase::SINUS:
        p = new WeightFunc_U16_Sinus; break;
      
    case WeightFuncBase::IDENTITY_0_PARAM:
        p = new WeightFunc_U16_Identity_0_Param; break;
    
    case WeightFuncBase::TRIANGLE_PARAM:
        p = new WeightFunc_U16_Triangle_Param; break;
                
    default:
        p = NULL;
        cout <<"WARNING **: "<<__func__<<"(): Unhandled `Shape' enum: '"<< shape <<"'\n";
    }    
    return p;
}


/**==========================================================================\n
  list()
*============================================================================*/
void list (const WeightFunc_U16 &w, uint from, uint to)
{
    cout << w.what() << ":  minval=" << w.minval()<<", maxval=" << w.maxval() 
         << "  (" << (w.have_table() ? "have table" : "no table") << ")\n";
    
    if (to > w.zmax())  to = w.zmax();    
    
    if (w.have_table())
      for (uint i=from; i <= to; i++) 
      {
        cout << i << ":  w()=" << w(i) << "  w[]=" << w[i];
        if (w(i) != w[i])  
             cout << "    Diff=" << w(i)-w[i] << '\n';
        else cout << '\n';
      }
    else
      for (uint i=from; i <= to; i++)
        cout << i << ":  w()=" << w(i) << '\n';
}      

// END OF FILE
