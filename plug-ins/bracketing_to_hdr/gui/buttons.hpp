/*
 * buttons.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  buttons.hpp
  
  Diese Buttons kommunizieren direkt mit dem globalen, singulaeren Objekt
   Br2Hdr::Instance(). Dank Receiver-Abstammung loggen sie sich selbsttaetig
   im Ctor in die entsprechenden Instance()-Distributoren ein und im Dtor 
   wieder aus. Dadurch sind sie ueber Instance()-Aktivitaeten informiert und 
   wechseln z.B. selbstaendig die Farbe oder de/aktivieren sich. Ihre Fltk-callbacks
   tun bereits das Richtige. So sind sie in FLUID einfach nur zu plazieren.
   Lediglich in das "Extra Code" Feld ist noch `extra_init()' einzutragen,
   um einige Default-Setzungen von FLUID bei der Instanzierung zu korrigieren
   bzw. Nutzer-Setzungen zu uebernehmen, die im Ctor noch nicht mitzukriegen
   waren.
  
  Minimal reichte das Inkludieren von Br2HdrManager.hpp fuer den Enum-Typ 
   Br2HdrManager::Event fuer handle_Event(); die auf Br2Hdr::Instance()
   bezugnehmenden cb_fltk_()-Funktionen waeren dann erst in der ~.cpp-Datei
   zu definieren.
*/
#ifndef buttons_hpp
#define buttons_hpp


#include <FL/Fl_Button.H>
#include "../br_core/Br2Hdr.hpp"         // br::Br2Hdr::Event, ~::Instance()
#include "../br_core/EventReceiver.hpp"  // EventReceiver
#include "../br_core/br_macros.hpp"      // DTOR()


// non-group
class InitCalctorButton : public Fl_Button, public br::EventReceiver
{
    Fl_Color  col_default;

public:
    InitCalctorButton (int X, int Y, int W, int H, const char* la=0);
    ~InitCalctorButton()  {DTOR("")}
    
    void extra_init();                      // for FLUID's "Extra Code"
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:
    static void cb_fltk_(Fl_Widget*, void*)  
      { br::Br2Hdr::Instance().init_Calctor(); }
};


// non-group
class ComputeResponseButton : public Fl_Button, public br::EventReceiver
{
    Fl_Color  col_default;
    
public:
    ComputeResponseButton (int X, int Y, int W, int H, const char* la=0);
    ~ComputeResponseButton()  {DTOR("")}
    
    void extra_init();                      // for FLUID's "Extra Code"
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:
    static void cb_fltk_(Fl_Widget*, void*)
      { br::Br2Hdr::Instance().compute_Response(); }
};


// non-group
class HDRButton : public Fl_Button, public br::EventReceiver
{
    Fl_Color  col_default;
    
public:
    HDRButton (int X, int Y, int W, int H, const char* la=0);
    ~HDRButton()  {DTOR("")}
    
    void extra_init();                      // for FLUID's "Extra Code"
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:
    static void cb_fltk_(Fl_Widget*, void*);
};


// non-group
class ComputeFllwUpButton : public Fl_Button, public br::EventReceiver
{
    Fl_Color  col_default;
    
public:
    ComputeFllwUpButton (int X, int Y, int W, int H, const char* la=0);
    ~ComputeFllwUpButton()  {DTOR("")}
    
    void extra_init();                      // for FLUID's "Extra Code"
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:
    static void cb_fltk_(Fl_Widget*, void*)
      { br::Br2Hdr::Instance().compute_FollowUpValues(); }
};


// non-group
class ComputeOffsetsButton : public Fl_Button, public br::EventReceiver
{
    Fl_Color  col_default;

public:
    ComputeOffsetsButton (int X, int Y, int W, int H, const char* la=0);
    ~ComputeOffsetsButton()  {DTOR("")}
    
    void extra_init();                      // for FLUID's "Extra Code"
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:
    static void cb_fltk_(Fl_Widget*, void*);
};



#include <FL/Fl_Choice.H>
class ImageNumberChoice : public Fl_Choice, public br::EventReceiver
{
    int last_set_value_;
  public:
    ImageNumberChoice (int X, int Y, int W, int H, const char* la=0);
    void build_choice();
    void handle_Event (br::Br2Hdr::Event);
    void last_set_value (int v)         {last_set_value_ = v;}
    int  last_set_value() const         {return last_set_value_;}
};


#endif  // buttons_hpp

// END OF FILE
