/*
 * ExifTable.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ExifTable.hpp
*/
#ifndef ExifTable_hpp
#define ExifTable_hpp


#include <FL/Fl_Float_Input.H>
#include "../FL_adds/Fl_Table.H"
#include "../br_core/Br2Hdr.hpp"        // br::Br2Hdr::Event
#include "../br_core/EventReceiver.hpp" // br::EventReceiver      
#include "../br_core/br_macros.hpp"     // DTOR()


/*!  Define this to allow editing of EXIF data (input handled accordingly to
   Ercolano's "singleinput.cxx" example.) */
#define BR_EXIF_INPUT


/**===========================================================================

  @class  ExifTable

  The EXIF-data table in the "EXIF" tab in the main window. 
  
  Tabelle praesentiert EXIF-Daten des ImgVectors des globalen, singlaeren
   Manager-Objektes Br2Hdr::Instance(). 
  
  @note Im Ctor wird Zeilenzahl der Tabelle gesetzt, gewoehnlich 0 (zum 
   Programmstart). (Nach)Laden von Bildern erfordert stets Zeilenzahl anzupassen!

*============================================================================*/
class ExifTable : public Fl_Table, public br::EventReceiver
{
# ifdef BR_EXIF_INPUT    
    Fl_Float_Input*  input_;    // for input of EXIF data
    int  row_edit_;             // to store the input row
    int  col_edit_;             // to store the input column
# endif    

protected:
    void draw_cell (TableContext context,   // table cell drawing
                    int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);

public:
    ExifTable (int X, int Y, int W, int H, const char *la=0); 
    ~ExifTable()   {DTOR(label());}
    
    void handle_Event (br::Br2Hdr::Event);  // virtual Receiver inheritance

private:      
    void        cb_event ();
    static void cb_event_(Fl_Widget*, void* u)    {((ExifTable*) u)->cb_event();}

# ifdef BR_EXIF_INPUT    
    void        cb_input ();
    static void cb_input_(Fl_Widget*, void* u)    {((ExifTable*) u)->cb_input();}
# endif      
};


#endif  // ExifTable_hpp

// END OF FILE
