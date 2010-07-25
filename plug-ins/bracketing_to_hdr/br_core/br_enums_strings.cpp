/*
 * br_enums_strings.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_enums_strings.cpp
*/

#include "br_enums_strings.hpp"

namespace br {

static char out_of_range_[] = "(out of range)";


const char* data_type_str (DataType type) 
{
    if (type >= DATA_NONE && type <= DATA_UNKNOWN) 
      return DATA_TYPE_STR[type];
    else 
      return out_of_range_;
}
const char* data_type_short_str (DataType type)
{
    if (type >= DATA_NONE && type <= DATA_UNKNOWN) 
      return DATA_TYPE_SHORT_STR[type];
    else 
      return out_of_range_;
}
const char* storing_scheme_str (StoringScheme scheme)
{
    if (scheme >= STORING_NONE && scheme <= STORING_UNKNOWN) 
      return STORING_SCHEME_STR[scheme];
    else 
      return out_of_range_;
}
const char* image_type_str (ImageType type)
{
    if (type >= IMAGE_NONE && type <= IMAGE_UNKNOWN) 
      return IMAGE_TYPE_STR[type];
    else 
      return out_of_range_;
}
const char* image_type_short_str (ImageType type)
{
    if (type >= IMAGE_NONE && type <= IMAGE_UNKNOWN) 
      return IMAGE_TYPE_SHORT_STR[type];
    else 
      return out_of_range_;
}

}  // namespace

// END OF FILE
