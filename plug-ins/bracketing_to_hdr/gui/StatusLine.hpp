/*
 * StatusLine.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  StatusLine.hpp  
  
  Implementation einer Statuszeile mittels Fltk.
  
  Die Ausgabefunktionen "out()" und "out_default()" genannt, wobei \a out() 
   einfach auf Fl_Output's \a value() abgebildet. Wegen public-Vererbung existiert
   dann auch die Funktion \a value() neben \a out(). Verbergen? -- Wie?
   Entweder durch private-Vererbung, aber dann muessten alle von \c Fl_Output sonst
   gebrauchten Methoden wie \a color() explizit durchgereicht werden, oder durch
   Deklaration einer privaten "value()"-Routine, die dann alle gleichnamigen
   verdeckt.
   
*/
#ifndef StatusLine_hpp
#define StatusLine_hpp


#include <FL/Fl_Output.H>
#include "../br_core/StatusLineBase.hpp"


/**===========================================================================

  @class  StatusLine
 
  Concretisation of a \a StatusLineBase for Fltk. Ctor has Fl_Widget interface,
   so usable in Fluid.

=============================================================================*/
class StatusLine : public StatusLineBase, public Fl_Output
{
public:
  StatusLine (int X, int Y, int W, int H, const char* la=0)
    : StatusLineBase(),
      Fl_Output(X,Y,W,H,la)
    {}
    
  virtual void out (const char* s)          {value (s); Fl::flush();} 
};


#endif  // StatusLine_hpp

// END OF FILE
