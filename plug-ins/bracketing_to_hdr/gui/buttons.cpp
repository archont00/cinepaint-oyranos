/*
 * buttons.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  buttons.cpp
*/
#include <FL/fl_draw.H>                     // fl_color_average()
#include "buttons.hpp"
#include "../br_core/Br2Hdr.hpp"            // br::Br2Hdr::Instance()
#include "../br_core/br_defs.hpp"           // BR_DEBUG_RECEIVER
#include "../br_core/Run_DisplcmFinder.hpp" // Run_DispclmFinder
#include "../bracketing_to_hdr.hpp"         // cpaint_show_image()

//#ifdef BR_DEBUG_RECEIVER
#  include <iostream>
   using std::cout;
//#endif


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

static Fl_Color col_needed = fl_color_average (FL_YELLOW, FL_WHITE, 0.5);
static Fl_Color col_nonexist = fl_color_average (FL_BLUE, FL_WHITE, 0.25);


//=========================
//
//   InitCalctorButton
//
//=========================

InitCalctorButton::InitCalctorButton (int X, int Y, int W, int H, const char* L)
  : Fl_Button (X,Y,W,H,L)
{
    CTOR("");
    col_default = color();  // FLUID's changes would come behind (-> extra_init())
    callback (cb_fltk_, this);
}  

/**+*************************************************************************\n
  Call this in FLUID's "Extra Code" field to consider changes made in FLUID.
******************************************************************************/
void InitCalctorButton::extra_init() 
{
    col_default = color();
    handle_Event (Br2Hdr::NO_EVENT);  // sets start-color and activation
}


void InitCalctorButton::handle_Event (Br2Hdr::Event e) 
{
#ifdef BR_DEBUG_RECEIVER
    cout<<"InitCalctorButton::"; EventReceiver::handle_Event(e);
#endif
    
    //  Button color: depending on Calctor existence as well as the Event...
    if (! theBr2Hdr.calctor()) 
    {
      if (color() != col_nonexist) {color(col_nonexist); redraw();}
    }
    else switch (e)        // Calctor exists    
    {
      case Br2Hdr::CALCTOR_INIT:
      case Br2Hdr::CALCTOR_REDATED:
          BR_EVENT_HANDLED(e);
          color(col_default); redraw();
          break;
        
      case Br2Hdr::IMG_VECTOR_SIZE:   // all things makeing calctor out-of-date
      case Br2Hdr::SIZE_ACTIVE:
      case Br2Hdr::ACTIVE_OFFSETS:
          BR_EVENT_HANDLED(e);
          color(col_needed); redraw(); 
          break;

      default: BR_EVENT_NOT_HANDLED(e);
          break;
    }
  
    //  De|activation: depending on size_active():
    if (theBr2Hdr.size_active() > 1) activate(); else deactivate();
}      


//============================
//
//   ComputeResponseButton
//
//============================

ComputeResponseButton::ComputeResponseButton (int X, int Y, int W, int H, const char* L)
  : Fl_Button (X,Y,W,H,L)
{
    CTOR("");
    col_default = color();  // FLUID's changes would come behind (-> extra_init())
    callback (cb_fltk_, this);
}  

/**+*************************************************************************\n
  Call this in FLUID's "Extra Code" field to consider changes made in FLUID.
******************************************************************************/
void ComputeResponseButton::extra_init() 
{
    col_default = color();
    handle_Event (Br2Hdr::NO_EVENT);  // sets start-color and activation
}

/**+*************************************************************************\n
  "No-existence" color only, if computation possibly, i.e. if calctor exists.
  
  @note Falls gewuenscht, dass ein inaktiver Button keine Farbaenderungen erleidet,
   waere in den case-Zweigen die Abfrage "if (active())" vorzuschalten und im 
   De/Aktivierungs-Teil im "activate()"-Zweig die Farbe gemaess des momentanten 
   Zustandes einzustellen. Denn waehrend der Inaktivphase wurde Farbe ja nicht
   aktuell gehalten.
******************************************************************************/
void ComputeResponseButton::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    cout<<"ComputeResponseButton::"; EventReceiver::handle_Event(e);
#endif

    //  Button color: depending on Calctor, Response and the Event
    if (! theBr2Hdr.calctor())  {
      if (color() != col_default) {color(col_default); redraw();}
    }
    else if (! theBr2Hdr.isComputedResponseReady()) { // Calctor exists
      if (color() != col_nonexist) {color(col_nonexist); redraw();}
    }
    else switch (e)     // Calctor && Response curves exist
    {
      case Br2Hdr::CALCTOR_INIT:        // should be unreachable by cond above  
        BR_EVENT_HANDLED(e);
        color(col_nonexist); redraw();
        break;
      
      //case Br2Hdr::CALCTOR_OUTDATED: break; // ?
      
      case Br2Hdr::CCD_UPDATED:         
        BR_EVENT_HANDLED(e);
        color(col_default); redraw();
        break;
        
      case Br2Hdr::CCD_OUTDATED:        // i.e. num. params changed
        BR_EVENT_HANDLED(e);
        if (color()!=col_needed) {color(col_needed); redraw();}
        break;  
      
      case Br2Hdr::WEIGHT_CHANGED:      // weight for resp. and/or merging, but
        BR_EVENT_HANDLED(e);            //  only resp. outdates response curves
        if (! theBr2Hdr.isComputedResponseUptodate())
          if (color()!=col_needed) {color(col_needed); redraw();}
        break;  
      
      default: BR_EVENT_NOT_HANDLED(e);
        break;
    }
  
    //  De|activation: depending on isUseExternResponse and Calctor:
    if (theBr2Hdr.isUseExternResponse())
      deactivate();                     // denn dann wird nicht gerechnet
    else if (theBr2Hdr.calctor())
      activate();
    else                                // no calctor
      deactivate();
}      


//=========================
//
//   HDRButton
//
//=========================

HDRButton::HDRButton (int X, int Y, int W, int H, const char* L)
  : Fl_Button (X,Y,W,H,L)
{
    CTOR("");
    col_default = color();  // FLUID's changes would come behind (-> extra_init())
    callback (cb_fltk_, this);
}  

/**+*************************************************************************\n
  Call this in FLUID's "Extra Code" field to consider changes made in FLUID.
******************************************************************************/
void HDRButton::extra_init() 
{
    col_default = color();
    handle_Event (Br2Hdr::NO_EVENT);  // sets start-color and activation
}

/**+*************************************************************************\n
  Deactivate if: !calctor OR (use_extern_curves AND !ready_usenext_response).
   Denn fuer !use_extern werden fehlende Kurven automatisch berechnet.
   Test auf Calctor, solange Merging von Calctor ausgefuehrt.
******************************************************************************/
void HDRButton::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    cout<<"HDRButton::"; EventReceiver::handle_Event(e);
#endif

# if 0
    /*  Deactivate if no calctor OR extern response curves shall be used AND
         thes curves are not ready */
    if (! theBr2Hdr.calctor() || (
        theBr2Hdr.isUseExternResponse() && ! theBr2Hdr.isUsenextResponseReady()))
      deactivate();
    else 
      activate();
# else      
    //  De|activation: depending on Calctor's existence:
    if (theBr2Hdr.calctor()) activate(); else deactivate();
# endif      
}      

void HDRButton::cb_fltk_(Fl_Widget*, void*) 
{
    cpaint_show_image (Br2Hdr::Instance().complete_HDR());
}


//=========================
//
//   ComputeFllwUpButton
//
//=========================

ComputeFllwUpButton::ComputeFllwUpButton (int X, int Y, int W, int H, const char* L)
  : Fl_Button (X,Y,W,H,L)
{
    CTOR("");
    col_default = color();  // FLUID's changes would come behind (-> extra_init())
    callback (cb_fltk_, this);
}  

/**+*************************************************************************\n
  Call this in FLUID's "Extra Code" field to consider changes made in FLUID.
******************************************************************************/
void ComputeFllwUpButton::extra_init() 
{
    col_default = color();
    handle_Event (Br2Hdr::NO_EVENT);  // sets start-color and activation
}


void ComputeFllwUpButton::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    cout<<"ComputeFllwUpButton::"; EventReceiver::handle_Event(e);
#endif

    //  Button color: depending on calctor, curves and the Event...
    if (! theBr2Hdr.calctor()) {
      if (color() != col_default) {color(col_default); redraw();}
    }
    else if (!theBr2Hdr.haveFollowUpCurves()) {  // calctor exists
      if (color() != col_nonexist) {color(col_nonexist); redraw();}
    }
    else switch (e)     // Calctor && follow-up curves exist
    {
      case Br2Hdr::CALCTOR_INIT:        
        BR_EVENT_HANDLED(e);
        color(col_nonexist); redraw();  // should be unreachable by cond above
        break;
        
      case Br2Hdr::FOLLOWUP_UPDATED:         
        BR_EVENT_HANDLED(e);
        color(col_default); redraw();
        break;
        
      case Br2Hdr::FOLLOWUP_OUTDATED:
        BR_EVENT_HANDLED(e);
        if (color() != col_needed) {color(col_needed); redraw();}
        break;  
      
      default: BR_EVENT_NOT_HANDLED(e);
        break;
    }
  
    //  De|activation: depending on Calctor's existence
    if (theBr2Hdr.calctor()) activate(); else deactivate();
}      


//============================
//
//   ComputeOffsetsButton
//
//============================

ComputeOffsetsButton::ComputeOffsetsButton (int X, int Y, int W, int H, const char* L)
  : Fl_Button (X,Y,W,H,L)
{
    CTOR("");
    col_default = color();  // FLUID's changes would come behind (-> extra_init())
    callback (cb_fltk_, this);
    if (theBr2Hdr.size_active() < 2) deactivate(); else activate();
}  

/**+*************************************************************************\n
  Call this in FLUID's "Extra Code" field to consider changes made in FLUID.
******************************************************************************/
void ComputeOffsetsButton::extra_init() 
{
    col_default = color();
}

/**+*************************************************************************\n
  Deactivates the button if Br2Hdr::Instance().size_active() < 2.
******************************************************************************/
void ComputeOffsetsButton::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    cout<<"ComputeOffsetsButton::"; EventReceiver::handle_Event(e);
#endif

    if (theBr2Hdr.size_active() < 2) deactivate(); else activate();
}      

/**+*************************************************************************\n
  Default FLTK callback, can be overwritten in FLUID.
******************************************************************************/
void ComputeOffsetsButton::cb_fltk_(Fl_Widget*, void*)
{
    Run_DisplcmFinder::run_for_all_actives(); 
}


//=========================
//
//   ImageNumberChoice
//
//=========================

ImageNumberChoice::ImageNumberChoice (int X, int Y, int W, int H, const char* la)
  :  Fl_Choice (X,Y,W,H,la),
     last_set_value_(0)
{
     build_choice();
}

/** 
  Unten der Versuch, bei Aenderungen der Bildzahl im Container nicht immer bei 
   Anzeige 0 zu landen, sondern so nahemoeglich den zuletzt haendisch eingestellen
   Wert wieder einzunehmen. Krankt daran, dass unsere Werte fuer das Programm
   ja auch etwas bedeuten, da koennen wir nicht zu freizuegig jonglieren.
   Ausprobieren!
  FRAGE: Sollten nicht nur die aktiven Bilder angeboten werden? 
*/
void ImageNumberChoice::build_choice()
{
    //printf("ImageNumberChoice::build()  last_set_value = %d\n", last_set_value_);
    int nimages = Br2Hdr::Instance().size();
    clear();                        // no redraw()
    if (nimages) {
      for (int i=0; i < nimages; i++) {
        char s[4];
        snprintf (s,4,"%d", i+1);   // we begin with "1", not "0"
        add (s,0,0);
      }
      /*  Der Versuch */
      if (last_set_value_ >= 0)
        if (last_set_value_ > nimages - 1) value (nimages - 1);
        else value (last_set_value_);
      else value (0);               // implies a redraw()
    } 
    else redraw();                  // redraw to clear()
}

/*!  FRAGE: Sollten nicht nur die aktiven Bilder angeboten werden? Dann waere auf
   IMG_VECTOR_SIZE und SIZE_ACTIVE zu reagieren.
*/
void ImageNumberChoice::handle_Event (Br2Hdr::Event e)
{
    //cout<<"ImageNumberChoice::"; EventReceiver::handle_Event(e);
    if (theBr2Hdr.size() != size()-1)  // "-1" for Fl_Choice's terminator item
      build_choice();
}


// END OF FILE
