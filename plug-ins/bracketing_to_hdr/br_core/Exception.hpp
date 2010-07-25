/*
 * Exception.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Exception.hpp
*/
#ifndef Exception_hpp
#define Exception_hpp

#include <exception>     // std::exception
    

namespace br {

/**===========================================================================

  @class Exception    (beginning of an exception concept)
  
  Note: Fltk und Ausnahmen scheinen sich nicht zu vertragen, aus einer 
   Callback-Routine geworfen kam nichts an.

*============================================================================*/
class Exception : public std::exception
{
public:

  enum Mask         // bit masks (under development)
    {
      MASK_NONE = 0,    ///< not set
      MASK_FAIL = 1,    ///< `what' string contains the condition which failed
      MASK_FILE = 2     ///< `what' contains a file name (not othogonal to 1)
    };
    
  enum ID           // classification [type-ID] of exceptions (under devel.)
    {
      NONE,             ///< not set
      FOPEN,            ///< exception while opening a file
      RESPONSE,         ///< while computation of response curves [?]
      UNKNOWN           ///< marks also the end
    };
    
  typedef unsigned int  Flag;   // combination of Masks
    
  
public:    
    
    ID          id_;
    Flag        flags_;
    int         line_;
    const char* func_;
    const char* file_;
    const char* what_;
    
    Exception (ID,                          // "type" number of the exception
               Flag = MASK_NONE,            // flags (e.g. content of `what_')
               const char* what="Exception",// info string
               const char* file=0,          // file the exception was thrown
               int         line=-1,         // line the exception was thrown
               const char* func=0);         // function the exception was thrown
    
    const char* what() const throw();       // virtual std::exception heritage
    virtual void report() const throw();    // Output on stderr what happend    
};

}  // namespace


/*!
 *  macro for easier throwing
*/
#define IF_FAIL_EXCEPTION(cond,id) \
    if (!(cond)) throw br::Exception (id, br::Exception::MASK_FAIL, #cond,\
        __FILE__,__LINE__,__func__);
    

#endif  // Exception_hpp

// END OF FILE
