/*
 * Distributor.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file Distributor.cpp
   
  @bug (Affects debug output only) It seems impossible to output with \c printf()
   in modus "-Wall --pedantic" a function pointer WITHOUT any warning:
   \c printf("%p",fp) gives the warning: format »%p« expects type »void*«, but 
   argument has type »void (*)()«; and
   \c printf("%p",(void*)fp) gives the warning: ISO C++ forbids converting
   of function pointers to object pointers. And both is right: <i>object pointers
   and function pointers have not to be of the same size!</i> On the other side, 
   `cout << fp' gave (g++ 3.3.4) a simple "1" even for different function pointers!!? 
   A g++ bug? So I ended at the moment with `cout << (void*)fp' and the above
   ISO C++ warning, and what would be wrong on systems where object pointers and 
   function pointers are different.
*/

#include <iostream>
#include "Distributor.hpp"

//#define BR_DEBUG_DISTRIBUTOR

using std::cout;

//====================
//  DistributorBase...
//====================
/**+*************************************************************************\n
  Login a participant. 
  @param fp: pointer of the callback function, which shall be called.
  @param userdata: data, which shall be given as second argument in the 
    callback function.
  
  @note 
  - Double logins of identic (fp,userdata)-pairs are not prevented; would 
     lead to (ordinar undangerous) multiple callback calls.
  - Several participants can have identic \a fp's if static member functions
     are used (all instances have here the same \a fp!)
  - Several participants can have identic userdata if not the \a this pointer
     is used, but something others.
******************************************************************************/
void DistributorBase::login (Callback* fp, void* userdata)
{
    array_.push_back (Entry(fp,userdata));

#   ifdef BR_DEBUG_DISTRIBUTOR    
    cout << "DistributorBase::" <<__func__<< "( fp=" << (void*)fp 
         << ", userdata=" << userdata << " ): new size()=" << array_.size()<<'\n';      
    //report(0);
#   endif
}


/**+*************************************************************************\n
  Logout a participant identified by its callback pointer \a fp.
  @param fp: function pointer as it was logged in (should be unique in array).
  
  @note  Calls with non-registered or already removed Callbacks are harmless.
******************************************************************************/
void DistributorBase::logout (Callback* fp)
{
    for (unsigned i=0; i < array_.size(); i++) {
      if (array_[i].fp == fp) {
        array_.erase (array_.begin()+i);

#       ifdef BR_DEBUG_DISTRIBUTOR        
        cout << "DistributorBase::"<<__func__<<"( fp="<< (void*)fp 
             << " ): new size()=" << array_.size()<<'\n';
#       endif 
        return;
      }
    }
    cout << "DistributorBase::" <<__func__<< "(callback=" << (void*)fp 
         << ": not found): size()=" << array_.size()<<'\n';
}


/**+*************************************************************************\n
  Logout a participant identified by its \a userdata.
  @param userdata: userdata as it was logged in (should be unique in array).

  @note  Calls with non-registered or already removed Users are harmless.
******************************************************************************/
void DistributorBase::logout (void* userdata)
{
    for (unsigned i=0; i < array_.size(); i++) {  
      if (array_[i].userdata == userdata) {
        array_.erase (array_.begin()+i);

#       ifdef BR_DEBUG_DISTRIBUTOR        
        cout << "DistributorBase::"<<__func__<<"( userdata=" << userdata
             << " ): new size()=" << array_.size()<<'\n';
#       endif 
        return;
      }
    }
    cout << "DistributorBase::"<<__func__<<"( userdata=" << userdata
         << ": not found ): new size()=" << array_.size()<<'\n';
}


/**+*************************************************************************\n
  Call the callback functions `fp(pData,userdata)' for all logged-in
   participants with \a pData as first argument and the deposited \a userdata
   as second one.
  @param pData: void pointer to the data ("info"), which is to distribute.
******************************************************************************/
void DistributorBase::distribute (const void* pData)
{
    for (size_t i=0; i < array_.size(); i++)
      array_[i].fp (pData, array_[i].userdata);
}


/**+*************************************************************************\n
  List current array of callbacks and userdata on console.
  @param label: Prefix of the report title, e.g. "Event".
******************************************************************************/
void DistributorBase::report (const char* label) const
{
    cout << '\"' << label << "\"-Distributor-Report:\n";   
    
    for (size_t i=0; i < array_.size(); i++)
      cout << '\t' << i << ": callback = " << (void*)array_[i].fp 
           << ",  userdata = " << array_[i].userdata << '\n';
}


// END OF FILE
