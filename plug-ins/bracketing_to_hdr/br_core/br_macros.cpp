/*
 * br_macros.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_macros.cpp  --  output functions for the macros in "br_macros.h".
*/

#include <cstdio>               // printf(),...
#include <cstdarg>              // va_start(), va_end(), va_list

#include "br_macros.hpp"
#include "br_enums.hpp"         // DataType, ImageType
#include "br_enums_strings.hpp" // data_type_str(), image_type_str() 


namespace br {


/** 
*   Function outputs a formated string on stderr.
*/
void e_printf (const char *fmt, ...) 
{
    va_list p;
    if (fmt)
    {   va_start (p,fmt);
        vfprintf (stderr, fmt, p);
        va_end (p);
    }
}

void
print_CTOR (const char* func, const char* s)
{
    if (!s || *s=='\0')  printf("CTOR %s()...\n", func);
    else                 printf("CTOR %s(\"%s\")...\n", func, s);
}

void
print_CTOR (const char* func, const char* s, void* pThis)
{
    if (!s || *s=='\0')  printf("CTOR %s()[ this=%p ]...\n", func, pThis);
    else                 printf("CTOR ~%s(\"%s\")[ this=%p ]...\n", func, s, pThis);
}

void
print_DTOR (const char* func, const char* s)
{
    if (!s || *s=='\0')  printf("DTOR ~%s()...\n", func);
    else                 printf("DTOR ~%s(\"%s\")...\n", func, s);
}

void
print_DTOR (const char* func, const char* s, void* pThis)
{
    if (!s || *s=='\0')  printf("DTOR ~%s()[ this=%p ]...\n", func, pThis);
    else                 printf("DTOR ~%s(\"%s\")[ this=%p ]...\n", func, s, pThis);
}

void
print_WINCALL (const char* s)
{
    if (!s || *s=='\0')  printf("window callback() called...\n");
    else                 printf("window callback(\"%s\") called...\n", s);
}

void
print_s_with_dots (const char* s)
{
    if (!s || *s=='\0')  printf("()...\n");
    else                 printf("(\"%s\")...\n", s);
}

void
fail_warning (const char* cond, 
                 const char* file, int line, const char* func)
{
    fprintf(stderr, "\n%s: `%s' failed. [%s:%d %s()]\n",
            warn_preamble__, cond, file, line, func);
}

void
assert_failed (const char* cond, 
                  const char* file, int line, const char* func)
{
    fprintf(stderr, "\n%s: `%s' failed. [%s:%d %s()]\n",
            warn_preamble__, cond, file, line, func);
}
                     
void
not_impl_image_type (const char* file, int line, const char* func, br::ImageType type)
{
    fprintf(stderr, "\n%s: [%s:%d] %s():\n\tNot implemented for image type '%s'.\n", 
            warn_preamble__, file, line, func, br::image_type_str(type));
}

void
not_impl_data_type (const char* file, int line, const char* func, br::DataType type)
{
    fprintf(stderr, "\n%s: [%s:%d] %s():\n\tNot implemented for data type '%s'.\n", 
            warn_preamble__, file, line, func, br::data_type_str(type));
}


}  // namespace "br"

// END OF FILE
