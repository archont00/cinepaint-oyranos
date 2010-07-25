/*
 * EventReceiver.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
#ifndef EventReceiver_hpp
#define EventReceiver_hpp


#include <cstdio>               // printf()
#include "Br2Hdr.hpp"           // br::Br2Hdr
#include "br_eventnames.hpp"    // br::EVENT_NAMES[]
#include "br_macros.hpp"        // CTOR(), DTOR()

namespace br {

/**===========================================================================

  @class  EventReceiver

  Receiving of the "Event" broadcasts of the singular <tt>Br2HdrManager</tt>
   instance Br2Hdr::Instance(). In Ctor the class logs in automatically into
   the Event distributor of Br2Hdr::Instance() and logs out in Dtor. For Event 
   handling the virtual function handle_Event(Br2Hdr::Event) is provided, which
   is to overwrite by heirs.
   
  The class is dedicated to simplify (automate) the login-logout-process for
   classes, which need to receive the above Events: Classes <i>derived</i> from 
   EventReceiver log-in and log-out automatically, their only remaining task is
   to overwrite handle_Event().

*============================================================================*/
class EventReceiver
{
public:

    EventReceiver()
      { CTOR("")  Br2Hdr::Instance().distribEvent().login(cb_, this); }

    virtual ~EventReceiver()  
      { DTOR("")  Br2Hdr::Instance().distribEvent().logout(this); }
    
    /*  This Event handle function is to overwrite by heirs. As default we give
         here a debug output. */
    virtual void handle_Event (Br2Hdr::Event e) 
      { printf("%s( this=%p ): %d : %s\n", __func__, (void*)this, e, br::EVENT_NAMES[e]);}

private:
    /*  Callback function for the login into the distributor in Ctor */
    static void cb_ (const Br2Hdr::Event* e, void* userdata)  
      { ((EventReceiver*)userdata) -> handle_Event(*e); }
};


}  // namespace

#endif  // EventReceiver_hpp

// END OF FILE
