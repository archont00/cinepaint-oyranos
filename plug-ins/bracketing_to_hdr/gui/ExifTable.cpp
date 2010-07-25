/*
 * ExifTable.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  ExifTable.cpp
*/
#include <cstdio>                       // sprintf() 

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>                 // fl_font(), fl_push_clip() etc.

#include "../FL_adds/Fl_Table.H"
#include "../br_core/Br2Hdr.hpp"        // br::Br2Hdr::Instance()
#include "../br_core/br_defs.hpp"       // BR_DEBUG_TABLE, BR_DEBUG_RECEIVER
#include "../br_core/br_macros.hpp"     // CTOR(), BR_WARNING()
#include "../br_core/i18n.hpp"          // macro _()
#include "ExifTable.hpp"                // prototype


/*!  Define this to get receiver debug output */
//#define BR_DEBUG_RECEIVER


/*!  Define this to get table debug output */
//#define BR_DEBUG_TABLE
#ifdef BR_DEBUG_TABLE
#  define DEBUG_PRINTF(args)     printf args
#  include "../FL_adds/fl_eventnames.hpp"   // fl_eventnames[]
#else
#  define DEBUG_PRINTF(args)
#endif


/*!  Define this to use C++ strings. Increases the size of the appl. by about
   50 kB. We use stringstreams for better formating of float numbers. */
#define USE_CPP_STRINGS
#ifdef USE_CPP_STRINGS
#  include <sstream>
#endif
 

using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

/////////////////////////
///
///   ExifTable
///
/////////////////////////
 
/**+*************************************************************************\n
  Constructor.
  
  @note If launched via FLUID, the box decision code comes after the Ctor and 
   we could work here with a wrong frame value (-> unneeded scrollbar if the
   box value used here is smaller than the final value, or the widget ends in
   a small distance (1-2 pixel) from the outer border if our value is too big.
   Therefore we set here NO_BOX and set in FLUID NO_BOX.
******************************************************************************/
ExifTable::ExifTable (int X, int Y, int W, int H, const char *la) 
  : 
    Fl_Table (X,Y,W,H,la)
{ 
    CTOR(la); 
    begin();
    
    box (FL_NO_BOX);  
    selection_color (FL_YELLOW);
    callback (cb_event_, this);
    when (FL_WHEN_RELEASE);  // handle table events on release
    //when (FL_WHEN_CHANGED|FL_WHEN_RELEASE);

# ifdef BR_EXIF_INPUT    
    input_ = new Fl_Float_Input (W/2, H/2, 0, 0);
    input_ -> hide();
    input_ -> callback (cb_input_, this);
    input_ -> when (FL_WHEN_ENTER_KEY_ALWAYS);
    input_ -> maximum_size (12);
# endif
    
    int frame      = Fl::box_dw (box()); // *after* box decision!
    int w_shutter  = 120;                // shutter speed
    int w_aperture = 100;                // aperture value
    int w_iso      = 100;                // ISO speed
    int w_time;                          // abstract time
    
    /*  Fit the last column into the rest of the table ('25' is the width
         of the activation button), but not smaller than... */
    w_time = W - (25 + w_shutter + w_aperture + w_iso) - frame;
    w_time = (w_time > 75) ? w_time : 75;
      
    cols (4);                 // shutter + aperture + ISO + time
    col_header (1);           // enable col header
    col_header_height (25);
    col_width (0, w_shutter);
    col_width (1, w_aperture);
    col_width (2, w_iso);
    col_width (3, w_time);
    col_resize (4);           // enable col resizing
    col_resize_min (25);

    rows (theBr2Hdr.size());  // number of images
    row_header (1);           // enable row header
    row_header_width (25);    // width of activation button
    row_height_all (25);
    row_resize (4);           // enable row resizing
    row_resize_min (10);
    
    end();  // FLUID auto-generates it if widget is launched as 'open group'
}                   


/**+*************************************************************************\n
  draw_cell()  -  Handle drawing of all cells of the table

  Funktion wird implizit auch durch einige der im Ctor benutzten Fl_Table-Methoden
   aufgerufen, dh. wenn Datenreferenz noch nicht initialisiert; anscheinend
   dort aber nicht in datenreferenzierendem Kontext (sondern context 64).
   
  In front of some shortened strings not always clear to translate we put a
   comment, which appears in the ~.po file and helps to translate.

  @todo Alignment at decimal points.
******************************************************************************/
void ExifTable::draw_cell (TableContext context, 
                             int R, int C, int X, int Y, int W, int H)
{
    DEBUG_PRINTF(("ExifTable::%s(): %s, R=%d, C=%d, X=%d, Y=%d, W=%d, H=%d\n",
        __func__, contextname(context), R,C, X,Y,W,H));
    
    static Fl_Color  col_inactive = fl_color_average (FL_WHITE, FL_GRAY, 0.66f);
    char  s[128];
    int  wcell;         // width of *visible* cell (not always == W)
    Fl_Align  align;    // alignment inside the visible cell
    float  shutter;
#ifdef USE_CPP_STRINGS    
    std::stringstream  ss;
    std::string  str;
#endif    

    switch ( context )
    {
    case CONTEXT_STARTPAGE:
        fl_font (FL_HELVETICA, 14);   // Besser waere globale fontsize policy
        return;

    case CONTEXT_COL_HEADER:
        fl_push_clip (X,Y,W,H);
        {
          fl_draw_box (FL_THIN_UP_BOX, X,Y,W,H, color());
          fl_color (FL_BLACK);
          wcell = wix + wiw - X;
          wcell = wcell < W ? wcell : W; // printable width never wider than W
          switch (C) 
          {
          case 0: fl_draw(_("Shutter speed [s]"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 1: fl_draw(_("Aperture"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 2: fl_draw(_("ISO"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 3: fl_draw(_("Time"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          /*  Something forgotten? */
          default: sprintf (s, "%d/%d", R,C);
               fl_draw (s, X,Y, wcell, H, FL_ALIGN_CENTER);
          }
        }
        fl_pop_clip();
        return;

    case CONTEXT_ROW_HEADER:  // the activation buttons 
        fl_push_clip (X,Y,W,H);
        {
          sprintf (s, "%d", R+1);  // we begin with "1", not "0"
          if (theBr2Hdr.isActive(R)) fl_draw_box (FL_DOWN_BOX, X,Y,W,H, FL_GREEN);
          else                       fl_draw_box (FL_UP_BOX,   X,Y,W,H, color());
          fl_color (FL_WHITE); 
          fl_draw (s, X,Y,W,H, FL_ALIGN_CENTER);
        }
        fl_pop_clip();
        return;
      
    case CONTEXT_CELL:
#ifdef BR_EXIF_INPUT
        /*  don't overwrite the input widget */
        if (input_->visible() && R == row_edit_ && C == col_edit_)
          return;
#endif    
        fl_push_clip (X,Y,W,H);
        {
          /*  BG COLOR  */
          //Fl_Color c = row_selected(R) ? selection_color() : FL_WHITE;
          Fl_Color col = FL_WHITE;
          if (! theBr2Hdr.isActive(R)) col = col_inactive;
          draw_box (FL_THIN_UP_BOX, X,Y,W,H, col);

          /*  TEXT  */
          theBr2Hdr.isActive(R) ? fl_color(FL_BLACK) : fl_color(fl_inactive(FL_BLACK));
          wcell = wix + wiw - X;
          wcell = wcell < W ? wcell : W; // printable width never wider than W
          switch (C)
          {
          case 0:  // shutter speed
              shutter = theBr2Hdr.image(R).exposure.shutter;
#ifdef USE_CPP_STRINGS
              if (shutter > 0.0f) {
                if (shutter > 1.0f) ss << "1 / " << shutter;
                else                ss << 1.0f / shutter;
                str = ss.str();     
              }
              else str = "---";
              
              if (wcell > fl_width(str.c_str())) align = FL_ALIGN_CENTER;
              else                               align = FL_ALIGN_LEFT;
              fl_draw (str.c_str(), X,Y, wcell, H, align);
#else
              if (shutter > 0.0f) {
                if (shutter > 1.0f) sprintf (s, "1 / %.2f", shutter);
                else                sprintf (s, "%.2f", 1.0f / shutter);
              }
              else sprintf (s, "---");
              
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
#endif
              break;
          
          case 1:  // aperture
#ifdef USE_CPP_STRINGS
              if (theBr2Hdr.image(R).exposure.aperture > 0.0f) {
                ss << theBr2Hdr.image(R).exposure.aperture;
                str = ss.str();
              }
              else str = "---";
              
              if (wcell > fl_width(str.c_str())) align = FL_ALIGN_CENTER; 
              else                               align = FL_ALIGN_LEFT;
              fl_draw (str.c_str(), X,Y, wcell, H, align);
#else
              if (theBr2Hdr.image(R).exposure.aperture > 0.0f)
                   sprintf (s, "%.1f", theBr2Hdr.image(R).exposure.aperture);
              else sprintf (s, "---");
              
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
#endif
              break;
          
          case 2:  // ISO speed
#ifdef USE_CPP_STRINGS          
              if (theBr2Hdr.image(R).exposure.ISO > 0.0f) {
                ss << theBr2Hdr.image(R).exposure.ISO;
                str = ss.str();
              }
              else str = "---";
              
              if (wcell > fl_width(str.c_str())) align = FL_ALIGN_CENTER; 
              else                               align = FL_ALIGN_LEFT;
              fl_draw (str.c_str(), X,Y, wcell, H, align);
#else              
              if (theBr2Hdr.image(R).exposure.ISO > 0.0f)
                   sprintf (s, "%.1f", theBr2Hdr.image(R).exposure.ISO);
              else sprintf (s, "---");
              
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
#endif              
              break;
          
          case 3:  // exposure time
              if (theBr2Hdr.time(R) <= 0.0f) fl_color(FL_RED);
              snprintf (s, 128, "%.6f ", theBr2Hdr.time(R));
              if (wcell > fl_width(s)) align = FL_ALIGN_RIGHT; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
              break;
          
          default: // something forgotten? 
              sprintf (s, "%d/%d", R, C);
              fl_draw (s, X,Y, wcell, H, FL_ALIGN_CENTER);
          }

          /*  BORDER (example)  */
          //fl_color (FL_LIGHT2); 
          //fl_rect (X,Y,W,H);
        }
        fl_pop_clip();
        return;

#ifdef BR_EXIF_INPUT
    case CONTEXT_RC_RESIZE:
        /*  Input widget mitscrollen...*/
        if (input_->visible()) { 
          find_cell (CONTEXT_TABLE, row_edit_, col_edit_, X,Y,W,H);
          if (X==input_->x() && Y==input_->y() && 
              W==input_->w() && H==input_->h())
            return;
          input_->resize (X,Y,W,H);
        }
        return;
#endif
        
    case CONTEXT_ENDPAGE:
        /* Draw a box in the "headers corner"; X,Y are in STARTPAGE/ENDPAGE
            those of the data table *without* headers, x() and y() are the
            outer coords; what we need here are wix, wiy, the (protected) 
            inner coords of the widget without border (same as X - row_header_width(), 
            Y - col_header_height)!). */
        draw_box (FL_EMBOSSED_BOX, wix, wiy, row_header_width(), col_header_height(), color());
        return;
        
    default:
        //printf("ExifTable::%s(): not processed context %d\n",__func__, context);
        return;
    }
}

/**+*************************************************************************\n
  cb_event() -  callback; called whenever someone clicks on different parts
                 of the table.
******************************************************************************/
void ExifTable::cb_event()
{
    DEBUG_PRINTF(("ExifTable::%s(): %s, row=%d col=%d, %s, clicks=%d\n",
        __func__, contextname (callback_context()),
        (int)callback_row(),
        (int)callback_col(),
        fl_eventnames[Fl::event()],
        (int)Fl::event_clicks()) );
    
    TableContext context = callback_context();
    int R = callback_row();
    int C = callback_col();

    switch (Fl::event())
    {
    case FL_PUSH:
        switch (context)
        {
        case CONTEXT_ROW_HEADER:
            if (theBr2Hdr.isActive(R)) theBr2Hdr.deactivate(R);
            else                       theBr2Hdr.activate(R);
            /*  Noetig waere eigentlich jetzt nur Neuzeichnen der Zeilenkoepfe,
                 doch das Untenstehende hat irgendeine Macke und ich weiss nicht
                 warum. Deshalb alles neu und raus. */
            redraw(); 
            return;
            /*  Table callback by default allready redraws *inner* table, i.e.
                 nessecary is here only to add the redrawing of row header.*/
            int XX,YY,WW,HH;
            find_cell (CONTEXT_ROW_HEADER, R,C, XX, YY, WW, HH);
            draw_cell (CONTEXT_ROW_HEADER, R,C, XX, YY, WW, HH);
            return;

#ifdef BR_EXIF_INPUT      
        case CONTEXT_CELL: {
            //printf("row=%d, col=%d,  rows()=%d,  cols()=%d\n", R,C, rows(),cols());
            if (input_->visible())  input_->do_callback();
            row_edit_ = R;
            col_edit_ = C;
            int XX,YY,WW,HH;
            find_cell (CONTEXT_CELL, R,C, XX, YY, WW, HH);
            input_ -> resize (XX,YY,WW,HH);
            char s[30];
            switch (C) {
              case 0: snprintf (s, 30, "%f", theBr2Hdr.image(R).exposure.shutter); break;
              case 1: snprintf (s, 30, "%f", theBr2Hdr.image(R).exposure.aperture); break;
              case 2: snprintf (s, 30, "%f", theBr2Hdr.image(R).exposure.ISO); break;
              case 3: snprintf (s, 30, "%f", theBr2Hdr.image(R).exposure.time); break;
              default:;
            }
            input_ -> value(s);
            input_ -> show();
            input_ -> take_focus();    
            return; }
#endif          
        default:           
            return;
        }

    default: ;
    }
}


#ifdef BR_EXIF_INPUT
/**+*************************************************************************\n
  cb_input()  -  callback for ExifTable's Float_Input widget
  
  Converts the current \c input->value() string into a float and sets corresponding 
   theBr2Hdr's data variable if conversion and value range are ok.
******************************************************************************/
void ExifTable::cb_input()
{
    //printf("%s(): row=%d  col=%d  value=\"%s\")...\n",__func__, row_edit_, col_edit_, input_->value());
    
    char*  endptr;
    float  res = strtof (input_->value(), &endptr);

    /*  Accept only correct values: */
    if (*endptr) {
      BR_WARNING (("wrong float format: \"%s\"", input_->value()));
      //  Data object remains unchanged.
    }
    else {
      if (res <= 0.0) {
        /*  No escape, we allow input for testing */
        BR_WARNING (("zero or negative values means 'not set'"));
      }
      switch (col_edit_) {
        case 0: theBr2Hdr.image(row_edit_).exposure.shutter = res; break;
        case 1: theBr2Hdr.image(row_edit_).exposure.aperture = res; break;
        case 2: theBr2Hdr.image(row_edit_).exposure.ISO = res; break;
        case 3: theBr2Hdr.setTime (row_edit_, res);
      }
    }
    input_-> hide();                    // hides input widget
}
#endif   


/**+*************************************************************************\n
  EventReceiver'c virtual callback routine:
******************************************************************************/
void ExifTable::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    printf("ExifTable::");  EventReceiver::handle_Event(e);
#endif
    
    switch (e)
    {
    case Br2Hdr::IMG_VECTOR_SIZE:       // Update ExifTable!  
        BR_EVENT_HANDLED(e);
        rows (theBr2Hdr.size());        // new row number
        redraw();
        break;
    
    case Br2Hdr::TIMES_CHANGED:
        BR_EVENT_HANDLED(e);
        redraw();
        break;
    
    default: 
        BR_EVENT_NOT_HANDLED(e);
    }
}    



// END OF FILE
