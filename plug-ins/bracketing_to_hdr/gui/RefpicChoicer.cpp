/*
 * RefpicChoicer.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
   RefpicChoicer.cpp
*/
#include <FL/Fl_Choice.H>
#include "RefpicChoicer.hpp"
#include "../br_core/br_defs.hpp"      // BR_DEBUG_RECEIVER, BR_EVENT_HANDLED()
#include "../br_core/br_macros.hpp"    // CTOR()
#include "../br_core/Br2Hdr.hpp"       // br:Br2Hdr::Instance()


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();


RefpicChoicer::RefpicChoicer (int X, int Y, int W, int H, const char* la)
  : Fl_Choice (X,Y,W,H,la),
    choice_ (*this)
{
    CTOR(la);  
    rebuild();
}  


void RefpicChoicer::handle_Refpic (int refpic)
{
#ifdef BR_DEBUG_RECEIVER
    printf("RefpicChoicer::"); 
    RefpicReceiver::handle_Refpic (refpic);  // debug output
#endif    
    choice_.value (refpic);
}      


void RefpicChoicer::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    printf("RefpicChoicer::");
    EventReceiver::handle_Event (e);         // debug output
#endif
    switch (e) 
    {
      case Br2Hdr::IMG_VECTOR_SIZE:
      case Br2Hdr::CALCTOR_INIT:
      case Br2Hdr::CALCTOR_DELETED:
        BR_EVENT_HANDLED(e);
        rebuild(); 
        break;
      
      default: BR_EVENT_NOT_HANDLED(e);
    }
}      

void RefpicChoicer::rebuild()
{
    choice_.clear();                         // no redraw()
    if (theBr2Hdr.calctor()) {
      for (int i=0; i < theBr2Hdr.calctor()->size(); i++) 
      {
        char s[4];
        snprintf(s,4,"%d", theBr2Hdr.getInputIndex(i)+1); // begin with "1"
        choice_.add(s,0,0);
      }
      choice_.value (theBr2Hdr.refpic());    // implies a redraw()
    }
    else
      choice_.redraw();                      // redraw() to clear()
}

// END OF FILE
