/*
 * FilePtr.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  FilePtr.cpp
*/

#include <cstdio>               // printf()
#include "FilePtr.hpp"
#include "Exception.hpp"


using br::Exception;

//=================
//  class FilePtr
//=================

//  Ctor...
FilePtr::FilePtr (const char *fname, const char *mode)
{
    if (!(f = fopen (fname, mode)))
    {
      fprintf (stderr, "%s(): Could not open file \"%s\" (mode='%s')\n", __func__, fname, mode);
      
      //  Throw (file, line, func uninteressting, we are HERE).
      throw Exception (Exception::FOPEN, Exception::MASK_FILE, fname);
    }
}


// END OF FILE
