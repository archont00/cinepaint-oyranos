/*
 * ProgressBar.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ProgressBar.hpp
   
  Several variants of FLTK realisations of ProgressInfo.
*/
#ifndef ProgressBar_hpp
#define ProgressBar_hpp


#include <FL/Fl_Progress.H>
#include <FL/Fl_Window.H>
#include "../br_core/ProgressInfo.hpp"  // the abstract base class


/*  Define this to check whether the resize() function is called for our
     Progress windows */
//#define BR_CHECK_PROGRESS_RESIZE


/*  Zum Setzen und Abfragen der internen ~.cpp Variable `resizing_support__' */
void progress_resizing_support (bool b);
bool progress_resizing_support();



/*============================================================================
  
  class ProgressBar  
  
=============================================================================*/
class ProgressBar : public ProgressInfo, public Fl_Progress
{
    bool with_cursor_;
    int  counter_flush_;

protected:
    void flush_value (float v);

public:
    ProgressBar (int X, int Y, int W, int H, const char* la=0)
      : Fl_Progress(X,Y,W,H,la),
        with_cursor_(false),
        counter_flush_(0)
      { 
        Fl_Progress::minimum(0.0);    // Corresponding to ProgressInfo's
        Fl_Progress::maximum(1.0);    //  default setting max=1.0
      }
    
    void start (float vmax);
    void start (float vmax, const char* text);
    void text (const char*);
    void finish();
    
    void with_cursor (bool b)   {with_cursor_ = b;}
    bool with_cursor() const    {return with_cursor_;}
};


/*============================================================================

  class ProgressWindow
    
=============================================================================*/
class ProgressWindow : public ProgressInfo, public Fl_Window
{
    Fl_Progress progress_;
    bool with_cursor_;
    
    void init_() { 
        progress_.minimum(0.0);    // Corresponding to ProgressInfo's
        progress_.maximum(1.0);    //  default setting max=1.0
        end(); 
      }

protected:
    void flush_value (float v);

public:
    ProgressWindow (int X, int Y, int W, int H, const char* la=0)
      : Fl_Window (X,Y,W,H,la),
        progress_(0,0,W,H)
      { init_(); } 
  
    ProgressWindow (int W, int H, const char* la=0)
      : Fl_Window (W,H,la),
        progress_(0,0,W,H)
      { init_(); } 
    
    void start (float vmax);
    void start (float vmax, const char* text);
    void text (const char*);
    void finish();
    
    void with_cursor (bool b)   {with_cursor_ = b;}
    bool with_cursor() const    {return with_cursor_;}

# ifdef BR_CHECK_PROGRESS_RESIZE
    void resize (int X, int Y, int W, int H) {
        printf("resize (X=%d, Y=%d)\n", X,Y);
        Fl_Window::resize(X,Y,W,H);
      }
# endif      
};


/*============================================================================

  class ProgressWin

=============================================================================*/
class ProgressWin : public ProgressInfo
{
    Fl_Window*    window_;
    Fl_Progress*  progress_;
    bool          with_cursor_;

protected:
    void flush_value (float v);

public:
    ProgressWin (int X, int Y, int W, int H, const char* la=0);
    ProgressWin (int W, int H, const char* la=0);
    
    void start (float vmax);
    void start (float vmax, const char* text);
    void text (const char*);
    void finish();
    
    void with_cursor (bool b)   {with_cursor_ = b;}
    bool with_cursor() const    {return with_cursor_;}
};

#if 0
/*============================================================================

  class ProgressNewWin  

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
    
    void start (float vmax);
    void value (float v);
    void text (const char* s);
    void finish();
};
#endif  // if 0


#endif  // ProgressBar_hpp

// END OF FILE
