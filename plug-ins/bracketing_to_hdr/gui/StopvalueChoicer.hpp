/*
 * StopvalueChoicer.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file StopvalueChoicer.hpp
   
  Contents:
   - StopvalueChoicerLocal
   - StopvalueChoicer

  Beispiel einer lokal->global-Hierarchie, bei der der allgemeine (=lokale)
   Fall mittels Zeiger und initDataRef() inplementiert ist (arbeiten muss),
   und die globale Variante durch Spezialisierung gewonnen wird. Sonst hatte
   ich nur die globale Variante notiert, die dann gleich mit einer Referenz
   arbeiten kann und ohne initDataRef() auskommt.
*/   
#ifndef StopvalueChoicer_hpp
#define StopvalueChoicer_hpp


#include <FL/Fl_Choice.H>
#include "../br_core/Br2HdrManager.hpp" // fuer die lokale Variante
#include "../br_core/Br2Hdr.hpp"        // fuer die globale Variante


/**===========================================================================

  @class  StopvalueChoicerLocal

  Choice-Menu for the `stopvalue' with Fl_Widget-like Ctor, so that usable in
   FLUID. Es ist dies mal das Beispiel einer "lokalen" Variante (mehrere 
   <tt>Br2HdrManager</tt>-Instanzen moegl.), bei der die Referenz des Managers nach
   der Widget-Konstruktion in FLUIDs "Extra Code" per initDataRef() mitzuteilen 
   waere.

  <tt>StopvalueChoicer</tt> unten ist die globale Variante, die auf das singulaere
   Manager-Objekt <tt>Br2Hdr::Instance()</tt> referenziert.

  @note Falls `stopvalue'-Aenderungen auch an solcher Widget-Instanz vorbei moeglich
   sind, z.B. \a mehrere StopvalueChoicer im Programm, muesste auch der Event-Verkehr
   mitgeschnitten werden (Ableiten noch von EventReceiver), um die Anzeige aktuell
   zu halten. Dazu braeuchte es dann allerdings eines ebenfalls <b>lokalen</b>
   <tt>EventReceiver</tt>s, der bisher nur als globaler vorliegt.

=============================================================================*/
class StopvalueChoicerLocal : public Fl_Choice 
{ 
    struct MenuMapEntry {const char* text; float value;};
    
    static MenuMapEntry  menuMap_[];
    static Fl_Menu_Item  menu_[];
    
    br::Br2HdrManager*  pBr2Hdr;   // the manager instance we refer to

public:
    StopvalueChoicerLocal (int X, int Y, int W, int H, const char* la=0);
    
    void initDataRef (br::Br2HdrManager* p)     {pBr2Hdr = p;}
    void initDataRef (br::Br2HdrManager& m)     {pBr2Hdr = &m;}

private:
    void        cb_fltk_i(Fl_Menu_*);
    static void cb_fltk_ (Fl_Menu_* w, void* u)  
       { ((StopvalueChoicerLocal*)u) -> cb_fltk_i (w); }
};      


/**===========================================================================

  @class  StopvalueChoicer

  Choice-Menu for the `stopvalue' with Fl_Widget-like Ctor, so that usable in
   FLUID. Globale Variante: Initialisiert sich im Ctor automatisch fuer das 
   singulaere Objekt <tt>Br2Hdr::Instance()</tt>, so dass kein explizites initDataRef() 
   noetig. Das geerbte initDataRef() sollte strenggenommen noch privat gemacht
   (verdeckt) werden, da oeffentlich damit allenfalls Schaden noch angerichtet 
   werden koennte.

=============================================================================*/
class StopvalueChoicer : public StopvalueChoicerLocal 
{ 
public:
    StopvalueChoicer (int X, int Y, int W, int H, const char* la=0)
      : StopvalueChoicerLocal(X,Y,W,H,la) 
      { initDataRef( br::Br2Hdr::Instance() ); }
};      



#endif  // StopvalueChoicer_hpp
// END OF FILE
