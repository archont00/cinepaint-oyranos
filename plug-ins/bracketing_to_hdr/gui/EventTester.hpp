/*
 * EventTester.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
#ifndef EventTester_hpp
#define EventTester_hpp


#include <FL/Fl_Group.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>

#include "../br_core/Br2Hdr.hpp"
#include "../br_core/EventReceiver.hpp"


// closed group
class EventTester : public Fl_Group, public br::EventReceiver
{
public:
    EventTester (int X, int Y, int W, int H, const char* la=0);
    ~EventTester();
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance
    void report_event_history (bool newest_on_top = true);

private:
    Fl_Output * output_receive_;
    Fl_Choice * choice_send_;
    br::Br2Hdr::Event * ring_buf_;      // array of Events
    int dim_ring_buf_;
    int pos_ring_buf_;
    bool ring_buf_filled_;
    
    void cb_choice_send__i(Fl_Choice*, void*);
    static void cb_choice_send_(Fl_Choice* o, void* v)
      {((EventTester*)(o->parent()))->cb_choice_send__i(o,v);}
      
    void cb_Print_i(Fl_Button*, void*)          {report_event_history();}
    static void cb_Print(Fl_Button* o, void* v) 
      {((EventTester*)(o->parent()))->cb_Print_i(o,v);}
};


#endif

// END OF FILE
