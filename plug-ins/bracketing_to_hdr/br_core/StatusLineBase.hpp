/*
 * StatusLineBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  StatusLineBase.hpp
*/
#ifndef StatusLineBase_hpp
#define StatusLineBase_hpp


#include <cstring>      // strdup()
#include <cstdlib>      // free()


class StatusLineBase
{
    char* default_text_;

public:
    virtual ~StatusLineBase()           {if (default_text_) free(default_text_);}
    
    virtual void out (const char*) = 0;
    virtual void out_default()          {out(default_text_);}
  
    void default_text (const char* t) {
        if (default_text_) free(default_text_);
        if (t) default_text_ = strdup(t); else default_text_ = 0;
      }
 
    const char* default_text() const    {return default_text_;}

protected:    
    /*  Protected Ctor to avoid attempted instances of abstract class */
    StatusLineBase() : default_text_(0) {}
};


#endif  // StatusLineBase_hpp

// END OF FILE
