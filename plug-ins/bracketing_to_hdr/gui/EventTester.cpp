/*
 * EventTester.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  EventTester.cpp
*/
#include <iostream>                         // if console output
#include "EventTester.hpp"
#include "../br_core/br_eventnames.hpp"     // br::EVENT_NAMES[]


using namespace br;
using std::cout;


//  Ctor:
EventTester::EventTester(int X, int Y, int W, int H, const char *L)
    : Fl_Group (0,0,W,H,L) 
{
    output_receive_ = new Fl_Output(70, 40, 150, 25, "received:");
    { Fl_Choice* o = choice_send_ = new Fl_Choice(70, 10, 150, 25, "Send:");
      o->down_box(FL_BORDER_BOX);
      o->callback((Fl_Callback*)cb_choice_send_);
      o->when(FL_WHEN_RELEASE_ALWAYS);
      for (int i=0; i <= Br2Hdr::EVENT_UNKNOWN; i++) 
        o->add(br::EVENT_NAMES[i]);
    }
    { Fl_Button* o = new Fl_Button(75, 75, 138, 20, "List Event-History");
      o->callback((Fl_Callback*)cb_Print);
    }    
    position(X,Y);
    end();  // Fluid generiert's, wenn als 'open group' lanciert
    
    //  Initiate the ring buffer
    dim_ring_buf_    = 15;
    pos_ring_buf_    = -1;      // one before the first pos '0'
    ring_buf_        = new Br2Hdr::Event [dim_ring_buf_];
    ring_buf_filled_ = false;
}

//  Dtor:
EventTester::~EventTester()
{
    DTOR("")
    delete[] ring_buf_;
}

//  Fltk callback:
void EventTester::cb_choice_send__i(Fl_Choice* o, void*) 
{
    cout << "menu.value="<< o->value() <<": "<< o->text() <<'\n';
    Br2Hdr::Instance().distribEvent().distribute(Br2Hdr::Event(o->value()));
}

//  EventReceiver's virtual handle routine:
void EventTester::handle_Event (Br2Hdr::Event e)
{
    cout <<"EventTester::"<<__func__<<"(): Event="<< e 
         <<" "<< br::EVENT_NAMES[e] <<'\n';
         
    output_receive_->value (br::EVENT_NAMES[e]);
    
    if (++pos_ring_buf_ >= dim_ring_buf_) {
      pos_ring_buf_ = 0;
      ring_buf_filled_ = true;
    }
    ring_buf_[ pos_ring_buf_ ] = e;
}

void EventTester::report_event_history (bool newest_on_top)
{
    printf("Event-History (buf pos=%d, filled=%d)\n", pos_ring_buf_, ring_buf_filled_);
    
    if (newest_on_top) {    
      //  newest on top, oldest on bottom..
      int k=0;
      for (int i=pos_ring_buf_; i >= 0; i--)
        printf("  %d  %s\n", k++, br::EVENT_NAMES[ ring_buf_[i] ]);
    
      if (ring_buf_filled_)
        for (int i=dim_ring_buf_-1; i > pos_ring_buf_; i--)
          printf("  %d  %s\n", k++, br::EVENT_NAMES[ ring_buf_[i] ]);
    }
    else {                  
      //  oldest on top, newest on bottom..
      int k = ring_buf_filled_ ? -(dim_ring_buf_ -1) : -pos_ring_buf_;
      if (ring_buf_filled_) 
        for (int i=pos_ring_buf_+1; i < dim_ring_buf_; i++)   
          printf("  % d  %s\n", k++, br::EVENT_NAMES[ ring_buf_[i] ]);
    
      for (int i=0; i <= pos_ring_buf_; i++)    
        printf("  % d  %s\n", k++, br::EVENT_NAMES[ ring_buf_[i] ]);
    }
}

// END OF FILE
