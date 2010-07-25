/*
 * StopvalueChoicer.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  StopvalueChoicer.cpp
*/   

#include "StopvalueChoicer.hpp"
#include "../br_core/i18n.hpp"  // macros _N(), _()


/*  
  Der Tooltip. Gewoehnlich in FLUID eingetragen, hier 'mal separat. Warum?
   - vermutlich, weil mehrere Lancierungen in FLUID geplant waren und der
   Tooltip nur einmal niedergelegt werden sollte. Zur Internationalisierung muss
   dann aber auch hier "i18n.hpp" inkludiert werden. 
   Mir noch unklar ist, warum fuer einen globalen String das _()-Makro keine
   Laufzeituebersetzung bewirkt, fuer einen STATIC String im Ctor (also einer
   Funktion) aber sehr wohl. Die Laufzeituebersetzung fuer den globalen String
   bewirkt ein _() bei seiner VERWENDUNG: tooltip(_(tooltip_stopvalue_)). Zu
   seiner Befoerderung ins .po-File ist dann das N_()-Makro angebracht. */
static const char* tooltip_stopvalue_ = 
N_("Here you can generate exposure times increasing by a constant factor F=2^(stop). F is stated traditionally by specifying the exponent \"stop\", which corresponds to differences of aperture values.");
   //de: "Hier koennen Belichtungszeiten generiert werden, die nach einem festen Faktor F=2^(stop) wachsen. Traditionell wird dabei nicht F, sondern der 2er-Exponent \"stop\" angegeben, der den Differenzen der Blendenzahlen entspricht. Bsp: stop=1 --> F=2, stop=2 --> F=4.";


/**+*************************************************************************\n
  Ctor:  
 
  FLUID haengt umgewidmeten "Class"en nach dem Ctor standardmaessig einen Codeblock
   an, der u.a. when() und align() setzt, so dass unsere eigenen diesbzgl.
   Default-Setzungen ueberschrieben werden. Sie sind in FLUID haendisch neu zu
   setzen! Oder wir stellen hier eine Funktion extra_init() bereit, die ins Feld
   "ExtraCode" einzutragen waere.
******************************************************************************/
StopvalueChoicerLocal::StopvalueChoicerLocal 
                         (int X, int Y, int W, int H, const char* la)
  : Fl_Choice(X,Y,W,H,la)
{
    menu (menu_);
    value (3);                          // stop = "1" as default
    callback ((Fl_Callback*)cb_fltk_, this);
    when (FL_WHEN_RELEASE_ALWAYS); 
    align (FL_ALIGN_LEFT);
    tooltip (_(tooltip_stopvalue_));    // _()-Makro bei der VERWENDUNG
}


void StopvalueChoicerLocal::cb_fltk_i (Fl_Menu_* w) 
{
    if (!pBr2Hdr) return;       // not initalized
    
    /*  Generate new times accordingly to the menu's `stopvalue' (this sets
         also pBr2Hdr's stopvalue_ to `value'!) */
    pBr2Hdr -> setTimesByStop (menuMap_[ w->value() ].value);
}


/*  Static class elements: */
StopvalueChoicerLocal::MenuMapEntry  StopvalueChoicerLocal::menuMap_[] = {
    {"0.33...", 1./3.   }, 
    {"0.5"    , 0.5     },
    {"0.66...", 2./3.   },
    {"1"      , 1.      },
    {"1.33...", 1.+1./3.},
    {"1.5"    , 1.5     },
    {"1.66...", 1.+2./3.},
    {"2"      , 2.      }
};  
Fl_Menu_Item  StopvalueChoicerLocal::menu_[] = {
    {menuMap_[0].text},
    {menuMap_[1].text},
    {menuMap_[2].text},
    {menuMap_[3].text},
    {menuMap_[4].text},
    {menuMap_[5].text},
    {menuMap_[6].text},
    {menuMap_[7].text},
    {0}
};


// END OF FILE
