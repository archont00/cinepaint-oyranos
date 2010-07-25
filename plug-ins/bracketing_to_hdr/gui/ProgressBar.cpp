/*
 * ProgressBar.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file ProgressBar.cpp
   
  Several variants of FLTK realisations of ProgressInfo:
   - ProgressBar     - derived from Fl_Progress, i.e. no toplevel window.
   - ProgressWindow  - derived from Fl_Window, Fl_Progress as element.
   - ProgressWin     - no Fltk-Widget, Fl_Window as element.
   - ProgressNewWin  - no Fltk-Widget, Fl_Window as elements;
                        each show() creates a Window, each() hide() deletes it.
*/

#include <FL/Fl.H>                      // Fl::flush(), Fl::check()
#include <FL/Fl_Progress.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>                 // fl_cursor()
#include "../br_core/ProgressInfo.hpp"  // abstract base class
#include "../FL_adds/fl_misc.hpp"       // fl_enforce_refresh()
#include "ProgressBar.hpp"              // declarations
#include <iostream>                     // debug output

using std::cout;


/** 
  Zumindest fuer X/SuSE galt:
  Wurden Progress-Fenster waehrend der Ausfuehrung mit der Maus verschoben,
   wurde die resize()-Funktion der Fenster nur angelaufen, wenn nach jedem
   Fl_Progress::value() ein Fl::check() und nicht lediglich ein Fl::flush()
   erfolgte. Nur dann wurde beim naechsten show() die neue Position eingenommen,
   sonst erschien das Progress-Fenster an alter Stelle. Weil Fl::check() etwas
   teurer als Fl::flush(), das hier einstellbar. 
   resizing_support__== True --> Fl::check(); sonst Fl::flush().
*/
static bool resizing_support__ = false;

/**
  Sets the internal \a resizing_support__ variable to value \a b. If set TRUE,
   then in our progress Window classes each value() is finished by a
   Fl::check() and not only by a Fl::flush(). Because under X only then the
   window's resize() function was called (by the system) if a (working)
   progress window was moved with the mouse.
*/
void progress_resizing_support (bool b)      {resizing_support__ = b;}
bool progress_resizing_support()             {return resizing_support__;}



/*  Define this to get debug output */
//#define BR_DEBUG_PROGRESS

/*  Define in ~.hpp BR_CHECK_PROGRESS_RESIZE to check whether the resize()
     function is called for our progress windows */


#ifdef BR_CHECK_PROGRESS_RESIZE
/**
  Ersetze zu Pruefzwecken in ProgressWin das Fl_Window durch DebugWindow
*/
class DebugWindow : public Fl_Window
{
  public:
    DebugWindow (int X, int Y, int W, int H, const char* la=0)
      : Fl_Window (X,Y,W,H, la) {}
    
    DebugWindow (int W, int H, const char* la=0)
      : Fl_Window (W,H, la) {}
    
    void resize (int X, int Y, int W, int H) {
        printf("resize (X=%d, Y=%d)\n", X,Y);
        Fl_Window::resize(X,Y,W,H);
      }
};
#endif  //  BR_CHECK_PROGRESS_RESIZE


/**===========================================================================
  
  @class ProgressBar  
  
  FLTK realisation of ProgressInfo as an Fl_Progress widget: inherits from
   ProgressInfo the below specified abstract functions and from Fl_Progress the
   rest. Derived from Fl_Progress - ordinary widget, no toplevel window.
   
  Derived from a Widget + Ctor with Widget interface --> usable in FLUID.
  
  Mit with_cursor() kann eingestellt werden, ob zwischen start() und finish()
   zudem ein WAIT-Cursor gezeigt werden soll. Voreingestellt ist Nein.
  
  Beachte die Besonderheit: Muss von ProgressInfo abgeleitet sein, um gueltiger
   Adressat eines abstrakten ProgressInfo Zeigers sein zu koennen, und von einem
   FLTK-Widget, um in FLUID lancierbar zu sein und einbaubar in FLTK's Gruppen-
   Hierarchien.

  @note The label() function (inherited from Fl_Progress) sets the progress bar
   text, but should not used for this purpose; for setting of text use text().

  @note A simple Fl_Progress::value() does not force the drawing, an Fl::flush() 
   or Fl::check() is needed after it! Sometimes seems to help only an 
   Fl::check(), but Fl::flush() is less expansive, so I use it here until
   further notice. The \a resizing_support__ variable has currently no effect
   on the value() routine of this class!

  @todo Und was, wenn \a with_cursor_ waehrend der Progression geaendert wird?
   (Geht das?) Dann wuerde eventuell Cursor nicht zurueckgesetzt. Man koennte eine
   \a working Variable einfuehren, die in start() true gesetzt wird und in 
   finish() false, um dergleichen zu wissen.
   
=============================================================================*/
  
void ProgressBar::start (float vmax) 
{
    ProgressInfo::start (vmax);    // updates base class values!!
    Fl_Progress::maximum (vmax);
    Fl_Progress::value (0.0);
    ProgressInfo::delta_min_flush (5.0 * vmax / w());  // 5 pixel 
    if (with_cursor_) fl_cursor (FL_CURSOR_WAIT);
    //printf("ProgressBar::start(): visible = %d\n", visible());
    if (! visible()) Fl_Progress::show();
    Fl::flush(); //Fl::check();
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressBar::start( "<< vmax <<" );  delta_min_flush = " 
           << ProgressInfo::delta_min_flush() << '\n'; 
      counter_flush_ = 0;
#   endif      
}
void ProgressBar::start (float vmax, const char* text) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressBar::start( "<< vmax << ", '" << text << "' )\n"; 
#   endif
    Fl_Progress::label (text);
    start (vmax);
}
    
void ProgressBar::text (const char* s) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressBar::text(\""<< s <<"\")\n";
#   endif
    Fl_Progress::label(s); 
    //Fl::flush();
}

void ProgressBar::flush_value (float v) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressBar::flush_value( "<< v <<" )  #" << ++ counter_flush_ << '\n';
#   endif
    Fl_Progress::value(v); 
    Fl::flush();
    //if (resizing_support__) Fl::check(); else Fl::flush(); // testweise
}
      
void ProgressBar::finish() 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressBar::finish();   flushes = " << counter_flush_ <<'\n';
#   endif
    Fl_Progress::hide();
    Fl_Progress::label(0);
    if (with_cursor_) fl_cursor (FL_CURSOR_DEFAULT);  // or previous cursor
    //Fl::flush();            // not needed if hide()
}


/**===========================================================================

  @class ProgressWindow
    
  FLTK realisation of ProgressInfo as a progress bar in window (derived from
   Fl_Window). Intended for using as an autonom toplevel window.
   
  Derived from a Widget + Ctor with Widget interface --> usable in FLUID.
  
  Mit with_cursor() kann eingestellt werden, ob zwischen start() und finish()
   zudem ein WAIT-Cursor gezeigt werden soll. Voreingestellt ist Nein.
  
  Beachte die Besonderheit: Muss von ProgressInfo abgeleitet sein, um gueltiger
   Adressat eines abstrakten ProgressInfo Zeigers sein zu koennen, und von einem
   FLTK-Widget, um in FLUID lancierbar zu sein und einbaubar in FLTK's Gruppen-
   Hierarchien.

  @note The label() function (inherited from Fl_Window) sets the title
   of the window; it does not set the progress bar text.
 
  @note A simple progress.value() does not force the drawing, an Fl::flush() 
   or Fl::check() is needed after it. To get resize() calls, an Fl::check()
   is required (adjustable via the global \a resizing_support__ variable).
   For a new created ProgressWindow a single Fl::flush() or Fl::check() is
   not enough to see the bar (the window was transparent), we need an 
   fl_multi_check() (done by fl_enforce_refresh()) in start()! After it a
   flush() after a progress.value() is enough for drawing (but not to get
   resize() calls).
   
  @note Wenn resize-Verhalten wichtig, konstruiere ProgressWindow ueber 
   (X,Y,W,H)-Ctor! Denn die ueber (W,H)-Ctor konstruierten Fenster werden von
   einigen Window-Managern bei einem show() nach Gutduenken noch umpositioniert.

=============================================================================*/
    
void ProgressWindow::start (float vmax)
{
    ProgressInfo::start (vmax);  // updates base class values
    progress_.maximum (vmax);
    progress_.value (0.0);
    ProgressInfo::delta_min_flush (5.0 * vmax / w());  // 5 pixel 
    /*  That way ProgressWindow could appear at mouse position: */
    //position (Fl::event_x_root(), Fl::event_y_root());
    if (with_cursor_) fl_cursor (FL_CURSOR_WAIT);
    Fl_Window::show();
    fl_enforce_refresh();
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWindow::start( "<< vmax <<" );  delta_min_flush = " 
                << ProgressInfo::delta_min_flush() << '\n'; 
#   endif      
}
void ProgressWindow::start (float vmax, const char* text)
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWindow::start( "<< vmax << ", '" << text << "' )\n"; 
#   endif
    progress_.label (text);
    start (vmax);
}
      
void ProgressWindow::text (const char* s) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWindow::text(\""<< s <<"\")\n";
#   endif       
    progress_.label(s); //Fl::flush();
}
    
void ProgressWindow::flush_value (float v) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWindow::flush_value( "<< v <<" )\n"; 
#   endif
    progress_.value(v); 
    if (resizing_support__) Fl::check(); else Fl::flush();
}
      
void ProgressWindow::finish()
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWindow::finish()\n";
#   endif
    printf("window finish pos x,y = %d, %d\n", x(), y());
    Fl_Window::hide();
    progress_.label(0);
    if (with_cursor_) fl_cursor (FL_CURSOR_DEFAULT);
    //Fl::flush();            // not needed if hide()
}


/**===========================================================================

  @class ProgressWin

  Abgeleitet von ProgressInfo, um gueltiger Adressat eines abstrakten ProgressInfo
   Zeigers zu sein. Aber nicht abgeleitet auch von einem Widget. Dadurch in
   FLUID nicht als Widget lancierbar.
  
  Mit with_cursor() kann eingestellt werden, ob zwischen start() und finish()
   zudem ein WAIT-Cursor gezeigt werden soll. Voreingestellt ist Nein.

=============================================================================*/
ProgressWin::ProgressWin (int X, int Y, int W, int H, const char* la) 
{
    window_ = new Fl_Window (X,Y,W,H,la);
      progress_ = new Fl_Progress (0,0,W,H);
      progress_->minimum(0.0);    // corresponding to ProgressInfo's
      progress_->maximum(1.0);    //  default setting max=1.0
    window_->end();
} 
ProgressWin::ProgressWin (int W, int H, const char* la) 
{
    window_ = new Fl_Window (W,H,la);
      progress_ = new Fl_Progress (0,0,W,H);
      progress_->minimum(0.0);    // corresponding to ProgressInfo's
      progress_->maximum(1.0);    //  default setting max=1.0
    window_->end();
} 

void ProgressWin::start (float vmax)
{
    ProgressInfo::start (vmax);   // updates base class values
    progress_->maximum (vmax);
    progress_->value (0.0);
    ProgressInfo::delta_min_flush (5.0 * vmax / window_->w());  // 5 pixel 
    if (with_cursor_) fl_cursor (FL_CURSOR_WAIT);
    window_->show();
    fl_enforce_refresh();
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWin::start( "<< vmax <<" );  delta_min_flush = " 
                << ProgressInfo::delta_min_flush() << '\n'; 
#   endif      
}
void ProgressWin::start (float vmax, const char* text)
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWin::start( "<< vmax << ", '" << text << "' )\n"; 
#   endif
    progress_->label (text);
    start (vmax);
}
    
void ProgressWin::text (const char* s) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWin::text(\""<< s <<"\")\n";
#   endif
    progress_->label(s); //Fl::flush();
}
    
void ProgressWin::flush_value (float v) 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWin::flush_value( "<< v <<" )\n"; 
#   endif
    progress_->value(v); 
    if (resizing_support__) Fl::check(); else Fl::flush();
}
    
void ProgressWin::finish() 
{
#   ifdef BR_DEBUG_PROGRESS    
      cout << "\tProgressWin::finish()\n";
#   endif
    printf("window finish pos x,y = %d, %d\n", window_->x(), window_->y());
    window_->hide();
    progress_->label(0);
    if (with_cursor_) fl_cursor (FL_CURSOR_DEFAULT);
    //Fl::flush();            // not needed if hide()
}


#if 0
/**===========================================================================

  @class ProgressNewWin  

  Each show() creates a new Window, and each hide() delete()s it.

  Ctor with Widget interface -- usable via make_window() in FLUID.

=============================================================================*/
class ProgressNewWin : public ProgressInfo
{
    int           w_, h_;
    const char*   label_;
    Fl_Window*    window_;
    Fl_Progress*  progress_;
    
public:
    ProgressNewWin (int W, int H, const char* la=0)
      : w_(W), h_(H), label_(la),
        window_ (0),
        progress_(0)
      {} 
    
    void show() {
#       ifdef BR_DEBUG_PROGRESS    
          cout << "\tProgressNewWin::show()\n";
#       endif       
        if (!window_) {
          window_ = new Fl_Window(w_,h_,label_);
          { progress_ = new Fl_Progress(0,0,w_,h_);
            progress_->minimum(0.0);    // corresponding to ProgressInfo's
            progress_->maximum(1.0);    //  default setting max=1.0
          }
          window_->end();
        }
        window_->show(); 
      }
    
    void hide() {
#       ifdef BR_DEBUG_PROGRESS    
          cout << "\tProgressNewWin::hide()\n";
#       endif
        if (window_) {
          window_->hide(); delete window_; window_=0; // Fl::flush();
        } 
      }
    
    void start (float vmax)      {
#       ifdef BR_DEBUG_PROGRESS    
          cout << "\tProgressBar::start( "<< vmax <<" )\n"; 
#       endif
        ProgressInfo::start (vmax);
        progress_->maximum (vmax);
      }
    
    void value (float v) {
#       ifdef BR_DEBUG_PROGRESS    
          cout << "\tProgressNewWin::value( "<< v <<" )\n"; 
#       endif
        value (v);
        progress_->value(v); window_->redraw(); Fl::flush(); 
      }
    
    void text (const char* s) {
#       ifdef BR_DEBUG_PROGRESS    
          cout << "\tProgressNewWin::text(\""<< s <<"\")\n";
#       endif
        progress_->label(s); Fl::flush(); 
      }
};
#endif  // if 0

// END OF FILE
