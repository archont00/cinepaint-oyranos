/*
 * TheProgressBar.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  TheProgressBar.hpp
*/
#ifndef TheProgressBar_hpp
#define TheProgressBar_hpp


#include "../br_core/TheProgressInfo.hpp"   // TheProgressInfo
#include "ProgressBar.hpp"

/**=========================================================================== 
  @class TheProgressBar
  
  Class sets (in the moment of instantiation) the global pointer of TheProgressInfo
   to a ProgressBar object (to that instance), i.e. lets TheProgressInfo appear
   as this ProgressBar.
  
  Klasse setzt (im Moment einer Instanziierung) den globalen Zeiger von TheProgressInfo
   auf ein ProgressBar-Objekt (auf jene Instanz), d.h. gibt ihm das Aussehen
   eines solchen. TheProgressBar ist von ProgressBar abgeleitet und also auch ein
   Widget: Erfolgt Instanzierung innerhalb einer offenen Gruppe, wird dieses ProgressBar
   Objekt wie ueblich in diese Gruppe eingereiht. Deletion of the group deletes also
   then this TheProgressBar instance and sets (via Dtor) the global TheProgressInfo
   pointer back to NULL.
=============================================================================*/
class TheProgressBar : public ProgressBar
{
  public:
    TheProgressBar (int X, int Y, int W, int H, const char* la=0) 
      : ProgressBar (X,Y,W,H, la) 
      {
        TheProgressInfo::set_pointer (this);
      }  
    
    /*  Dtor: Reset TheProgressInfo's pointer to 0! */
    ~TheProgressBar()           {TheProgressInfo::set_pointer (0);}
    void reactivate()           {TheProgressInfo::set_pointer (this);}
};


/**=========================================================================== 
  @class TheProgressWindow
  
  Class sets (in the moment of instantiation) the global pointer of TheProgressInfo
   to a ProgressWindow object (to that instance), i.e. lets TheProgressInfo appear
   as this ProgressWindow.
  
  Klasse setzt (im Moment einer Instanziierung) den globalen Zeiger von TheProgressInfo
   auf ein ProgressWindow Objekt (auf jene Instanz), d.h. gibt ihm das Aussehen eines
   solchen.
  
  TheProgressWindow intendiert als toplevel window und sollte so gesehen nicht
   innerhalb einer offenen Gruppe instanziiert werden, sonst wuerde es ein 
   sub-window, zulaessig aber ist auch das.
=============================================================================*/
class TheProgressWindow : public ProgressWindow
{
  public:
    TheProgressWindow (int X, int Y, int W, int H, const char* la=0) 
      : ProgressWindow (X,Y,W,H, la) 
      {
        TheProgressInfo::set_pointer (this);
      }
    
    TheProgressWindow (int W, int H, const char* la=0) 
      : ProgressWindow (W,H, la) 
      {
        TheProgressInfo::set_pointer (this);
      }
    
    /*  Dtor: Reset TheProgressInfo's pointer to 0! */
    ~TheProgressWindow()        {TheProgressInfo::set_pointer (0);}
    void reactivate()           {TheProgressInfo::set_pointer (this);}
};

/**=========================================================================== 
  @class TheProgressWin
  
  Class sets (in the moment of instantiation) the global pointer of TheProgressInfo
   to a ProgressWin object (to that instance), i.e. lets TheProgressInfo appear
   as this ProgressWin.
  
  Klasse setzt (im Moment einer Instanziierung) den globalen Zeiger von TheProgressInfo
   auf ein ProgressWin-Objekt (auf jene Instanz), d.h. gibt ihm das Aussehen eines
   solchen.
  
  TheProgressWin ist - wie ProgressWin - kein Widget (nicht abgeleitete von einem 
   solchen).
=============================================================================*/
class TheProgressWin : public ProgressWin
{
  public:
    TheProgressWin (int X, int Y, int W, int H, const char* la=0) 
      : ProgressWin (X,Y,W,H, la) 
      {
        TheProgressInfo::set_pointer (this);
      }
    
    TheProgressWin (int W, int H, const char* la=0) 
      : ProgressWin (W,H, la) 
      {
        TheProgressInfo::set_pointer (this);
      }
    
    /*  Dtor: Reset TheProgressInfo's pointer to 0! */
    ~TheProgressWin()           {TheProgressInfo::set_pointer (0);}
    void reactivate()           {TheProgressInfo::set_pointer (this);}
};


#endif  // TheProgressBar_hpp
