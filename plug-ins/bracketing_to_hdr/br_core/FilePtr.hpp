/*
 * FilePtr.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  FilePtr.hpp
*/
#ifndef FilePtr_hpp
#define FilePtr_hpp

#include <cstdio>       // fclose(), FILE

/**+===========================================================================  
  
  @class  FilePtr  
  
  FILE* as automatic object. Ctor opens file, Dtor closes it automatically
   at scope end. Ctor can throw an Exception.

=============================================================================*/
class FilePtr 
{
  FILE* f;

public:
  FilePtr (const char *fname, const char *mode);
  FilePtr (FILE* p)     { f = p; }          // take over an extern FILE*
  ~FilePtr()            { if (f) fclose (f); }
  operator FILE*()      { return f; }       // cast to FILE*
};



#endif  // FilePtr_hpp

// END OF FILE
