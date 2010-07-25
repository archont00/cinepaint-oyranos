/*
 * IntInput_Slider.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  IntInput_Slider.cpp
  
  With some changes adapted from Greg Ercolano's...
  // sliderinput -- simple example of tying an fltk slider and input widget together
  // 1.00 erco 10/17/04
*/
#include <stdio.h>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Slider.H>
#include "IntInput_Slider.hpp"


/*!
  In Greg's value(int val) Variante wurde zum Setzen der Slider-Callback
   \a cb_slider_i() aufgerufen, der den \a val Wert an \a input mitteilte. 
   Weil ich am Ende der Kinder-Callbacks den Klassen-Callback aufrufe, waere
   bei jedem value(int) auch der Callback der Klasse gestartet worden, was kein
   vernuenftiges Verhalten. Es kam dadurch zudem zu zirkulaeren Aufrufen bei
   Konstruktionen der Art
  
    - 1::cb() ==ruft==> 2::value(v)  und
    - 2::cb() ==ruft==> 1::value(v)
  
   wegen<pre>
     1::cb() --> 2::value(v) --> 2::cb_slider(v) --> 2::cb()
         \----- 1::cb_slider(v) <---- 1::value(v) <-----/  </pre>
   
   weshalb eine \a recurse2 Variable einzufuehren gewesen waere. Daher eine
   ordentliche value() Funktion hier ohne Rekurs auf einen Callback. Eruebrigt
   dann auch Greg's \a recurse Variablen in cb_slider_i() und cb_input_i(), 
   deren Sinn mir ohnehin schleierhaft.
*/
void IntInput_Slider::value (int val)
{
    //fprintf(stderr, "\"%s\": VALUE(%d)\n", label(), val);
    slider->value (val);     // pass val to slider
    char s[80];
    sprintf (s, "%d", val);
    input->value (s);        // pass val to input
}
    

// CALLBACK HANDLERS
//    These 'attach' the input and slider's values together.
//
void IntInput_Slider::cb_slider_i() 
{
    char s[80];
    sprintf (s, "%d", (int)(slider->value() + .5));
    //fprintf (stderr, "\"%s\": SPRINTF(%d) -> '%s'\n", label(), (int)slider->value(), s);
    input->value (s);    // pass slider's value to input
    do_callback();      // call base Group's (=IntInput_Slider's) callback
}

void IntInput_Slider::cb_input_i() 
{
    int val = 0;
    if ( sscanf(input->value(), "%d", &val) != 1 ) {
      val = 0;
    }
    //fprintf(stderr, "\"%s\": SCANF('%s') -> %d\n", label(), input->value(), val);
    slider->value (val);  // pass input's value to slider
    do_callback();        // call base Group's (=IntInput_Slider's) callback
}
    
// CTOR
IntInput_Slider::IntInput_Slider (int x, int y, int w, int h, const char *l) 
  : Fl_Group(x,y,w,h,l) 
{
    int in_w = 35;        // width of the Fl_Int_Input widget
        
    input  = new Fl_Int_Input (x, y, in_w, h);
    input->callback (cb_input, (void*)this);
    input->when (FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);

    slider = new Fl_Slider (x+in_w, y, w - in_w, h);
    slider->type (1);
    slider->callback (cb_slider, (void*)this);

    bounds (1, 100);      // some usable default
    value (1);            // some usable default
    end();                // close the group
}


// END OF FILE
