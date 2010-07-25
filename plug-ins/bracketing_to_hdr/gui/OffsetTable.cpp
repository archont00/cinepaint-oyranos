/*
 * OffsetTable.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  OffsetTable.cpp
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
#include "OffsetTable.hpp"              // OffsetTable


/*  Define this to get receiver debug output */
//#define BR_DEBUG_RECEIVER

/*  Define this to get table debug output */
//#define BR_DEBUG_TABLE
#ifdef BR_DEBUG_TABLE
#  define DEBUG_PRINTF(args)     printf args
#  include "../FL_adds/fl_eventnames.hpp"   // fl_eventnames[]
#else
#  define DEBUG_PRINTF(args)
#endif


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

/////////////////////////
///
///   OffsetTable
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
OffsetTable::OffsetTable (int X, int Y, int W, int H, const char *la) 
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

# ifdef BR_OFFSET_INPUT    
    input_ = new Fl_Input (W/2, H/2, 0, 0);
    input_ -> hide();
    input_ -> callback (cb_input_, this);
    input_ -> when (FL_WHEN_ENTER_KEY_ALWAYS);
    input_ -> maximum_size (12);
# endif
    
    int frame        = Fl::box_dw (box()); // *after* box decision!
    int w_loc_offset = 100;                // local offset, computed
    int w_to_pos     = 60;                 // computed to image with pos
    int w_act_offset = 100;                // active offset
    int w_intersect;                       // intersection area
    
    /*  Fit the last column into the rest of the table ('25' is the width
       of the activation button), but not smaller than... */
    w_intersect = W - (25 + w_loc_offset + w_to_pos + w_act_offset) - frame;
    w_intersect = (w_intersect > 140) ? w_intersect : 140;
      
    cols (4);                 // loc_offset + to_pos + act_offset + intersect
    col_header (1);           // enable col header
    col_header_height (25);
    col_width (0, w_loc_offset);
    col_width (1, w_to_pos);
    col_width (2, w_act_offset);
    col_width (3, w_intersect);
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
void OffsetTable::draw_cell (TableContext context, 
                             int R, int C, int X, int Y, int W, int H)
{
    DEBUG_PRINTF(("OffsetTable::%s(): %s, R=%d, C=%d, X=%d, Y=%d, W=%d, H=%d\n",
        __func__, contextname(context), R,C, X,Y,W,H));
    
    static Fl_Color  col_inactive = fl_color_average (FL_WHITE, FL_GRAY, 0.66f);
    char  s[128];
    int  wcell;         // width of *visible* cell (not always == W)
    Fl_Align  align;    // alignment inside the visible cell
    const Rectangle &  sec = theBr2Hdr.imgVector().intersection();
    Vec2_int  d;        // for offset vectors
    int  id;            // for image IDs 

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
          case 0: fl_draw(_("computed"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 1: fl_draw(_("to pos"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 2: fl_draw(_("active offset"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          case 3: fl_draw(_("Intersection area"), X,Y, wcell, H, FL_ALIGN_CENTER); break;
          /*  Something forgotten? */
          default: sprintf(s, "%d/%d", R,C);
                   fl_draw(s, X,Y, wcell, H, FL_ALIGN_CENTER);
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
#ifdef BR_OFFSET_INPUT
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
          if (! theBr2Hdr.image(R).active_offset_ok() && theBr2Hdr.isActive(R))
            fl_color(FL_RED);
          wcell = wix + wiw - X;
          wcell = wcell < W ? wcell : W; // printable width never wider than W
          switch (C)
          {
          case 0:  // the computed local offset
              if (theBr2Hdr.image(R).offset_ID())
                snprintf (s, 128, "(%2d,%2d)", theBr2Hdr.image(R).offset().x, 
                                               theBr2Hdr.image(R).offset().y );
              else sprintf (s, "(--,--)");
              
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
              break;
          
          case 1:  // above offset was computed to image at position...
              id = theBr2Hdr.image(R).offset_ID();
              //  we output internal position 0 as 1 etc.
              if (id) sprintf (s, "%d", theBr2Hdr.imgVector().pos_of_ID(id) + 1);
              else    sprintf (s, "--");  // offset not computed
              
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
              break;
          
          case 2:  // the active (global) offset
              snprintf (s, 128, "(%2d,%2d)", theBr2Hdr.image(R).active_offset().x, 
                                             theBr2Hdr.image(R).active_offset().y );
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
              else                     align = FL_ALIGN_LEFT;
              fl_draw (s, X,Y, wcell, H, align);
              break;
          
          case 3:  // intersection area
              d = theBr2Hdr.image(R).active_offset();
              snprintf (s, 128, "[%d - %d]  x  [%d - %d]", 
                  sec.x0()+d.x, sec.xlast()+d.x, 
                  sec.y0()+d.y, sec.ylast()+d.y);
              if (wcell > fl_width(s)) align = FL_ALIGN_CENTER; 
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

#ifdef BR_OFFSET_INPUT         
    case CONTEXT_RC_RESIZE:
        /*  Input widget mitscrollen */
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
        //printf("OffsetTable::%s(): not processed context %d\n",__func__, context);
        return;
    }
}

/**+*************************************************************************\n
  cb_event() -  callback; called whenever someone clicks on different parts
                 of the table.
******************************************************************************/
void OffsetTable::cb_event()
{
    DEBUG_PRINTF(("OffsetTable::%s(): %s, row=%d col=%d, %s, clicks=%d\n",
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

#ifdef BR_OFFSET_INPUT      
        case CONTEXT_CELL:    
            if (C == col_loc_offset_ || C == col_act_offset_) {
              if (input_->visible())  input_->do_callback();
              row_edit_ = R;
              col_edit_ = C;
              int XX,YY,WW,HH;
              find_cell (CONTEXT_CELL, R,C, XX, YY, WW, HH);
              input_ -> resize (XX,YY,WW,HH);
              char s[30];
              switch (col_edit_) {
                case col_loc_offset_:
                  snprintf (s, 30, "%d %d", theBr2Hdr.image(R).offset().x,
                                            theBr2Hdr.image(R).offset().y);
                  break;
                case col_act_offset_:
                  snprintf (s, 30, "%d %d", theBr2Hdr.image(R).active_offset().x,
                                            theBr2Hdr.image(R).active_offset().y);
                  break;
              }
              input_ -> value(s);
              input_ -> show();
              input_ -> take_focus();    
            }
            return;
#endif          
        default:           
            return;
        }

    default: ;
    }
}


#ifdef BR_OFFSET_INPUT
/**+*************************************************************************\n
  cb_input()  -  callback for OffsetTable's Fl_Input widget. 
  
  Tries to convert the current \c input->value() string into a "%d %d" format
   and sets theBr2Hdr's offset variable if conversion was ok.
******************************************************************************/
void OffsetTable::cb_input()
{
    printf("%s(): row=%d  col=%d  value=\"%s\")...\n",__func__, row_edit_, col_edit_, input_->value());
    
    int  x, y;
    if (sscanf (input_->value(), "%d %d", &x, &y) < 2) {
      BR_WARNING(("no \"%%d %%d\" format in \"%s\"", input_->value()));
      //  Data remain unchanged.
    }
    else {                              // ok, take over
      switch (col_edit_) {
        case col_loc_offset_:
          theBr2Hdr.image(row_edit_).offset (Vec2_int(x,y));
          break;
        case col_act_offset_:
          theBr2Hdr.image(row_edit_).active_offset (Vec2_int(x,y));
          break;
      }  
    }
    input_-> hide();                    // hides input widget
}
#endif   


/**+*************************************************************************\n
  EventReceiver'c virtual callback routine:
******************************************************************************/
void OffsetTable::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    printf("OffsetTable::");  EventReceiver::handle_Event(e);
#endif
    
    switch (e)
    {
    case Br2Hdr::IMG_VECTOR_SIZE:        // Update OffsetTable!  
        BR_EVENT_HANDLED(e);
        rows (theBr2Hdr.size());        // new row number
        redraw();
        break;
    
    case Br2Hdr::ACTIVE_OFFSETS:        // active offsets changed
    case Br2Hdr::OFFSET_COMPUTED:       // any offset computed
        BR_EVENT_HANDLED(e);
        redraw();
        break;
        
    default: 
        BR_EVENT_NOT_HANDLED(e);
    }
}    



// END OF FILE
