/*
 * OffsetTable.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  OffsetTable.hpp
*/
#ifndef OffsetTable_hpp
#define OffsetTable_hpp


#include <FL/Fl_Input.H>
#include "../FL_adds/Fl_Table.H"
#include "../br_core/Br2Hdr.hpp"        // br::Br2Hdr::Event
#include "../br_core/EventReceiver.hpp" // br::EventReceiver      
#include "../br_core/br_macros.hpp"     // DTOR()


/*!  Define this to allow editing of offset values (input handled accordingly
   to Ercolano's "singleinput.cxx" example.) */
#define BR_OFFSET_INPUT


/**===========================================================================

  @class  OffsetTable

  The offset table in the "Offsets" tab in the main window. 
  
  Tabelle praesentiert Offsetdaten des ImgVectors des globalen, singlaeren
   Manager-Objektes Br2Hdr::Instance(). 
  
  @note Im Ctor wird Zeilenzahl der Tabelle gesetzt, gewoehnlich 0 (zum 
   Programmstart). (Nach)Laden von Bildern erfordert stets Zeilenzahl anzupassen!

*============================================================================*/
class OffsetTable : public Fl_Table, public br::EventReceiver
{
# ifdef BR_OFFSET_INPUT    
    Fl_Input*  input_;                  // for input of "%d %d" offset values
    int  row_edit_;                         // to store the input row
    int  col_edit_;                         // to store the input col
    const static int  col_loc_offset_ = 0;  // col we allow to edit
    const static int  col_act_offset_ = 2;  // col we allow to edit
# endif    

protected:
    void draw_cell (TableContext context,   // table cell drawing
                    int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);

public:
    OffsetTable (int X, int Y, int W, int H, const char *la=0); 
    ~OffsetTable()   {DTOR(label());}
    
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:      
    void        cb_event ();
    static void cb_event_(Fl_Widget*, void* u)    {((OffsetTable*) u)->cb_event();}

# ifdef BR_OFFSET_INPUT    
    void        cb_input ();
    static void cb_input_(Fl_Widget*, void* u)    {((OffsetTable*) u)->cb_input();}
# endif      
};


#endif  // OffsetTable_hpp

// END OF FILE
