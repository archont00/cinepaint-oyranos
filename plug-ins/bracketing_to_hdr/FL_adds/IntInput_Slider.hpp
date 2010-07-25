/*
 * IntInput_Slider.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  IntIntput_Slider.hpp
*/
#ifndef IntInput_Slider_hpp
#define IntInput_Slider_hpp


#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Slider.H>

/*!
  @class IntInput_Slider.hpp
  
  Following the idea in Greg Ercolano's 
    <a href=http://seriss.com/people/erco/fltk/#SliderInput>sliderinput</a>...
  <pre>
  // sliderinput -- simple example of tying an fltk slider and input widget together
  // 1.00 erco 10/17/04 </pre>
  
  Changes: <ol>
   <li> Frame removed, base group completely covered now by children Fl_Int_Input
       & Fl_Slider (same appearance as Fl_Value_Slider).
   <li> The set function value(int) does no longer call slider's callback; avoids
       circular calls in some situations concerning point 3.
   <li> Calling of base group's callback at the end of both children's callbacks
       added. Otherwise IntInput_Slider's callback would never be triggered by
       any user event. The \a recurse variables in that callbacks could be removed
       due to point 2.
   <li> Several widget functions added (overlaid), necessary for usage in FLUID.
   <li> Rounding `double -> int' enhanced to negative slider values.
  </ol>
  
  @note Waehrend Fl_[Value_]Slider's value() \a double returniert, returniert
   IntInput_Slider wie Fl_Int_Input \a int. Gleiches gilt fuer die Funktionen
   minimum(), maximum() und bounds(low,high). Wo IntInput_Slider probeweise
   gegen Fl_[Value_]Slider getauscht werden koennen soll, sollte Code auf den
   allgemeineren \a double Fall abgestellt sein.
   
  Why the name "IntInput_Slider"?
   Shall (should) appereances like a Fl_Value_Slider with `int' as value,
   and the value field works like a Fl_Int_Input: "Value" --> "IntInput".
  
  @todo Zur Zeit sind Zahlen allzeit so gross wie bei Fl_Int_Input, waehrend sie
   bei Fl_Value_Slider kleiner sind. IDEE: Waehrend normaler Darstellung so klein
   wie bei Fl_Value_Slider, klickt man aber in das Zahlenfeld, werden sie zur
   Eingabe so gross wie bei Fl_Int_Input. Dafuer vielleicht am besten von 
   Fl_Value_Slider ableiten und die handle()-Routine ueberschreiben.
  
  @todo Schwaeche: Bei Fl_Value_Slider sind Zahlen zentriert, bei Fl_Int_Input
   linksbuendig. Zentriert sieht besser aus. 
*/  
class IntInput_Slider : public Fl_Group {
    Fl_Int_Input *input;
    Fl_Slider    *slider;

    // CALLBACK HANDLERS
    void        cb_slider_i();
    static void cb_slider (Fl_Widget *w, void *data) {
        ((IntInput_Slider*)data)->
        cb_slider_i();
      }

    void        cb_input_i();
    static void cb_input (Fl_Widget *w, void *data) {
        ((IntInput_Slider*)data)->
        cb_input_i();
      }
    
    inline int round_(double v) const {
        return v < 0.0 ? (int)(v-0.5) : (int)(v+0.0);
      }
         
public:
    // CTOR
    IntInput_Slider(int x, int y, int w, int h, const char *l=0);
    
    // ACCESSORS --  Add your own as needed
    int  value() const           { return round_(slider->value()); }
    void value(int val);//          { slider->value(val); cb_slider_i(); }
    int  minimum() const         { return round_(slider->minimum()); }
    void minimum(int val)        { slider->minimum(val); }
    int  maximum() const         { return round_(slider->maximum()); }
    void maximum(int val)        { slider->maximum(val); }
    void bounds(int low, int high) { slider->bounds(low, high); }
    
    /*! The color(..), selection_color(..), box(..) functions are members already
        of the Fl_Group base class. If we define here e.g. a new color(..),
        all "color" named base functions are blanked. To reanimate the color()
        read function we have two possiblities: 1) We define here also a
        new color() read function. 2) We pass in our color(Fl_Color) the color
        value to Fl_Group::color(Fl_Color) and make via `using Fl_Group::color'
        the "color" name visible. Then Fl_Group's color() will return the right
        value. I did the first.
    */
    void color(Fl_Color c)       { slider->color(c); /*Fl_Group::color(c);*/ }
    Fl_Color color() const       { return slider->color(); }
    void selection_color(Fl_Color c) {slider->selection_color(c);
                                   /*Fl_Group::selection_color(c);*/ }
    Fl_Color selection_color() const { return slider->selection_color(); }                                   
    void box(Fl_Boxtype t)       { slider->box(t); input->box(t); /*Fl_Group::box(t);*/ }
    Fl_Boxtype box() const       { return slider->box(); }
    void step(double st)         { slider->step(st); }      // double or int? Was verlangt FLUID?
    double step() const          { return slider->step(); }
    void textsize(uchar sz)      { input->textsize(sz); }
    uchar textsize() const       { return input->textsize(); }
};

#endif // IntInput_Slider_hpp
