/*
 * path_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
/*!
  path_utils.hpp
*/
#ifndef path_utils_hpp
#define path_utils_hpp


const char*  
getFileExtension (const char* path);

const char*
getTmpDir();

bool
buildTmpPath (char* destbuf, const char* fname, int maxlen = 512); 

const char* 
get_dcraw_path();

const char* 
get_jhead_path();


#endif  // path_utils_hpp
