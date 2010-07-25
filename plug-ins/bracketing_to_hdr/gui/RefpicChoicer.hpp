/*
 * RefpicChoicer.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  RefpicChoicer.hpp
*/
#ifndef RefpicChoicer_hpp
#define RefpicChoicer_hpp


#include <FL/Fl_Choice.H>
#include "../br_core/EventReceiver.hpp"
#include "../br_core/RefpicReceiver.hpp"
#include "../br_core/br_macros.hpp"        // CTOR(), DTOR()


// non-group
class RefpicChoicer : public Fl_Choice,
                      public br::RefpicReceiver,
                      public br::EventReceiver
{
    Fl_Choice &  choice_;       // to make code more readable
    
public:
    RefpicChoicer (int X, int Y, int W, int H, const char* la=0);
    ~RefpicChoicer()  {DTOR("")}

    void handle_Refpic (int refpic);         // virtual inheritance
    void handle_Event (br::Br2Hdr::Event);   // virtual inheritance

private:
    void rebuild();
};


#endif  // RefpicChoicer_hpp

// END OF FILE
