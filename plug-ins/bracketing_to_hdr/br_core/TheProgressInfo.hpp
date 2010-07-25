/*
 * TheProgressInfo.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  TheProgressInfo.hpp
*/
#ifndef TheProgressInfo_hpp
#define TheProgressInfo_hpp


#include "ProgressInfo.hpp"

/**=========================================================================== 

  @class TheProgressInfo
  
  Class maintains a global (==static member) abstract ProgressInfo pointer \a pr_.
   We provide all public ProgressInfo functions and map them to \a pr_ calls,
   wrapped by a <i>pr_ != NULL</i> check. Class is intended to be THE global
   ProgressInfo instance in a program. The abstract \a pr_ pointer can be adressed
   via set_pointer() to a concrete progress object. This object can be a child
   widget within a parent group like ProgressBar, or a toplevel window object
   like ProgressWindow or something else.
  
  We do NOT delete in Dtor the ProgressInfo object, \a pr_ is pointing to,
   because we have not allocated it! Only who delivered the pointer can know
   what to do. E.g. \a pr_ could reference an automatic object; or it references
   an FLTK widget which is child of a group - the FLTK group Dtor will delete
   these children!

=============================================================================*/
class TheProgressInfo 
{
    static ProgressInfo* pr_;

  public:
    static void  start (float vmax)     {if (pr_) pr_->start(vmax);}
    static void  start (float vmax, const char* text)
                                        {if (pr_) pr_->start(vmax,text);}
    static void  direct_value (float v) {if (pr_) pr_->direct_value(v);}
    static void  value (float v)        {if (pr_) pr_->value(v);}
    static float value()                {if (pr_) return pr_->value();}
    static float maximum()              {if (pr_) return pr_->maximum();}
    static void  forward (float dv)     {if (pr_) pr_->forward(dv);}
    static void  text (const char* t)   {if (pr_) pr_->text(t);}
    static void  finish()               {if (pr_) pr_->finish();}
   
    static void  set_pointer (ProgressInfo* p)  {pr_ = p;}
    static const ProgressInfo* pointer()        {return pr_;}
    
    TheProgressInfo()                   {}
}; 

#endif  // TheProgressInfo_hpp
