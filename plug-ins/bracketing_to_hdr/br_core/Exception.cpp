/*
 * Exception.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Exception.cpp
*/

#include <cstdio>
#include "Exception.hpp"
#include "br_macros.hpp"        // e_printf


namespace br {


/**+*************************************************************************\n
  Ctor

  @param id: our type-ID for exceptions
  @param flags: exception flags (combination of bitmasks)
  @param what: info string, e.g. a condition which failed
  @param file: file the exception was thrown from; default NULL
  @param line: line the exception was thrown from; default -1
  @param func: function the exception was thrown from; default NULL
******************************************************************************/
Exception::Exception (ID id, Flag flags, const char* what, 
                      const char* file, int line, const char* func)
{
    id_    = id;
    flags_ = flags;
    what_  = what;
    file_  = file;
    line_  = line;
    func_  = func;
}

/**+*************************************************************************\n
  Return an info string of what happend. Upgrading of the derived what().
******************************************************************************/
const char* Exception::what() const throw()   
{
    static char text[256];
    
    if (flags_ & MASK_FAIL) {
      snprintf(text, 256, "#%d: '%s' failed", id_, what_);
    }
    else if (flags_ & MASK_FILE) {
      snprintf(text, 256, "#%d: File \"%s\"", id_, what_);
    }  
    else { 
      snprintf(text, 256, "#%d: what info: '%s'\n", id_, what_);
    }
    return text;
}

/**+*************************************************************************\n
  Output the exception infos on stderr.             Provisionally.
******************************************************************************/
void Exception::report() const throw()
{
    if (flags_ & MASK_FAIL) {
      e_printf("\nBr2Hdr-EXCEPTION *** #%d: '%s' failed\n", id_, what_);
      e_printf("\tthrown in [%s: %d] %s()\n", file_, line_, func_);
    }  
    else if (flags_ & MASK_FILE) {
      e_printf("\nBr2Hdr-EXCEPTION *** #%d: File \"%s\"\n", id_, what_);
      e_printf("\tthrown in [%s: %d] %s()\n", file_, line_, func_);
    }  
    else { 
      e_printf("\nBr2Hdr-EXCEPTION *** #%d: %s\n", id_, what_);
      e_printf("\tthrown in [%s: %d] %s()\n", file_, line_, func_);
    }
}


}  // namespace

// END OF FILE
