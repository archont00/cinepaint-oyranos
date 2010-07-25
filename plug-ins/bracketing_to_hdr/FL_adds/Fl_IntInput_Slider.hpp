/*
 * Fl_IntInput_Slider.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Fl_IntInput_Slider.hpp
  Soll 'mal mein Slider-Widget mit Input-Feld fuer Integer werden. Zur Zeit
   Huelle fuer verschiedene Versuche.
*/  
#ifndef Fl_IntInput_Slider_hpp
#define Fl_IntInput_Slider_hpp

/*! Define this to use IntInput_Slider as Fl_IntInput_Slider */
#define USE_INT_INPUT_SLIDER

/*!
  @class Fl_IntInput_Slider
  
  Soll 'mal mein Slider-Widget mit Input-Feld fuer Integer werden. Zur Zeit
   Huelle fuer verschiedene Versuche.
   
  Wenn USE_INT_INPUT_SLIDER definiert, wird IntIntput_Slider genommen. 
  
  Als Rueckfallsicherung zur Zeit ein Fl_Value_Slider mit auf `int' 
   umgestellten value(), minimum() und maximum() Lese-Funktionen (ueberdecken
   die alten double-Funktionen). Die zugehoerigen Setz-Funktionen koenn(t)en
   ebenso wie bounds(double,double) bleiben (durch \a using wieder sichtbar zu
   machen!), da Konvertierung int -> double legal. Int-Argument haette gleichwohl
   Vorteil von Compiler-Warnungen bei irrtueml. double-Gebrauch. Ob Umstellung auf
   `int' ueberhaupt moeglich, davon abhaengig, ob FLUID da nicht generell im
   Double-Format reinschreibt. Scheint aber nicht der Fall. 
  
  Beim FLUID-Gebrauch zu beachten, dass die dortigen Setzungen z.B. fuer \a step 
   auf Ganzzahliges gestellt werden muessen, da unserem Ctor nachgeordnet, 
   und ein gebrochner Wert dann der aktuelle im Bauch der Basisklasse waere.
   Noch besser, wir ueberlueden auch step() mit Ganzzahligem, dann gaeb's Warnungen.
  
  Weil durch die neuen gleichnamigen Funktionen die alten verdeckt werden,
   waeren sie, falls gebraucht, durch `using' wieder sichtbar zu machen. Oder
   eben das Get/Set-Funktionspaar zum jeweiligen Namen vollstaendig neu
   definieren.
*/

#ifdef USE_INT_INPUT_SLIDER

# include "IntInput_Slider.hpp"
  typedef IntInput_Slider  Fl_IntInput_Slider;

#else

# include <FL/Fl_Value_Slider.H>
  class FL_EXPORT Fl_IntInput_Slider : public Fl_Value_Slider
  {
    inline int round_(double v) const {
        return v < 0.0 ? (int)(v-0.5) : (int)(v+0.0);
      }
  
  public:
    Fl_IntInput_Slider (int X, int Y, int W, int H, const char* la)
      : Fl_Value_Slider(X,Y,W,H,la)  {step(1.0);}
    
    int  value() const           { return round_(Fl_Value_Slider::value()); }
    void value(int val)          { Fl_Value_Slider::value(val); }
    int  minimum() const         { return round_(Fl_Value_Slider::minimum()); }
    void minimum(int val)        { Fl_Value_Slider::minimum(val); }
    int  maximum() const         { return round_(Fl_Value_Slider::maximum()); }
    void maximum(int val)        { Fl_Value_Slider::maximum(val); }
    //void bounds(int low, int high) { Fl_Value_Slider::bounds(low, high); }
  };      
#endif // USE_INT_INPUT_SLIDER
      
#endif // Fl_IntInput_Slider_hpp
