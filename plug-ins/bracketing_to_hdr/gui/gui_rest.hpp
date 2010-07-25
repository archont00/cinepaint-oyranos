/*
 * gui_rest.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  gui_rest.hpp
  
  Some things we declare in FLUID (-> "gui.h"), but code the implementation
   outside of FLUID in "gui_rest.cpp" (i.e. not in "gui.cxx") . And some 
   things used in FLUID refer to "gui.h" stuff, but their declarations are 
   easier to do outside of FLUID. These things are declared here.
*/
#ifndef gui_rest_hpp
#define gui_rest_hpp


#include "../br_core/EventReceiver.hpp"


//======================================================
//  non-member functions used in gui.cxx (resp. gui.fl)
//======================================================
void save_response_curves_dialog (br::Br2Hdr::WhichCurves, int channel_like);
bool load_response_curve         (int channel, const char* fname);
void load_response_curve_dialog  (int channel);
void load_all_response_curves_dialog();


class MainWinClass;


#if 0
/**+**************************************************************************
  @class MainWinMenubarWatcher

  Private helper object of MainWinClass to handle de/activations of menubar
   items accordingly to state changes of Br2Hdr::Instance(). As inheritor of
   EventReceiver the Ctor logs-in automatically in the Event-Distributor of 
   Br2Hdr::Instance() and the (virtual) handle_Event() routine gets after it all
   Event messages. There we do the de|activation stuff. Meanwhile we also switch
   on the timebar at an SIZE event, if time are not complete. Name ---> 
   "MainWinEventHandler".
  
  An alternative would be inheriting the whole MainWinClass from EventReceiver
   and doing the job in a MainWinClass::handle_Event() routine. Andererseits ist
   es ein Beispiel, wie die Mehrfachvererbung vermieden werden kann: durch
   Halten eines von EventReceiver abgeleiteten *Objektes*. Das Hilfsobjekt
   wird vorteilhaft dann zum Freund erklaert, sonst nur Zugriff auf oeffentliche
   Elemente.
******************************************************************************/
class MainWinMenubarWatcher : public br::EventReceiver
{
    MainWinClass*  pMainWin_;
public:
    MainWinMenubarWatcher (MainWinClass* p) : pMainWin_(p)  {}
    void handle_Event (br::Br2Hdr::Event);  // virtual inheritance
};
#endif


#endif  // gui_rest_hpp

// END OF FILE
