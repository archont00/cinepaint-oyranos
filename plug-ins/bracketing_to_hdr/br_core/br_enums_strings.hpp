/*
 * br_enums_strings.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_enums_strings.hpp
*/
#ifndef br_enums_strings_hpp
#define br_enums_strings_hpp

#include "br_enums.hpp"                 // DataType etc.

namespace br {

const char* const DATA_TYPE_SHORT_STR[] = {
        "none",
        "U8",
        "U16",
        "F16",
        "F32",
        "F64",
        "unknown"
};

const char* const DATA_TYPE_STR[] = {
        "none",
        "8-bit Unsigned Integer",
        "16-bit Unsigned Integer",
        "16-bit Float",
        "32-bit IEEE-Float",
        "64-bit IEEE-Float",
        "unknown"
};

const char* const STORING_SCHEME_STR[] = {
        "none",
        "interleave (RGBRGB..)",
        "channel-wise (RR..GG..BB..)",
        "unknown"
};

const char* const IMAGE_TYPE_SHORT_STR[] = {
        "none",
        "RGB-U8",
        "RGB-U16",
        "RGB-F32",
        "RGB-F64",
        "unknown"
};

const char* const IMAGE_TYPE_STR[] = {
        "none",
        "RGB - 8-bit Unsigned Integer",
        "RGB - 16-bit Unsigned Integer",
        "RGB - 32-bit IEEE-Float",
        "RGB - 64-bit IEEE-Float",
        "unknown"
};


/**+*************************************************************************\n
  Returns the above strings including a range check of the enum arguments.
   Eigentlich ueberfluessig, da wir ja hoffentlich nie auf die Enums casten.
******************************************************************************/
const char* data_type_str        (DataType);
const char* data_type_short_str  (DataType);
const char* storing_scheme_str   (StoringScheme);
const char* image_type_str       (ImageType);
const char* image_type_short_str (ImageType);

}  // namespace "br"

#endif    // br_enums_strings_hpp

// END OF FILE

