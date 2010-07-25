/*
 * ys_dbg.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ys_dbg.cpp  --  output functions for the debug makros in "ys_dbg.h".
*/

#include <cstdarg>      // v_start(), v_end(), va_list
#include <cstdio>       // vprintf()
#include "ys_dbg.hpp"


void ys_vprintf (const char *fmt, ...) 
{
    va_list p;
    if (fmt)
    {   va_start (p,fmt);
        vprintf (fmt, p);
        va_end (p);
    }
}
void ys_print_CTOR (const char* func, const char* s)
{
    if (!s || *s=='\0')  printf("CTOR %s()...\n", func);
    else                 printf("CTOR %s(\"%s\")...\n", func, s);
}
void ys_print_CTOR (const char* func, const char* s, void* pThis)
{
    if (!s || *s=='\0')  printf("CTOR %s()[ this=%p ]...\n", func, pThis);
    else                 printf("CTOR ~%s(\"%s\")[ this=%p ]...\n", func, s, pThis);
}
void ys_print_DTOR (const char* func, const char* s)
{
    if (!s || *s=='\0')  printf("DTOR ~%s()...\n", func);
    else                 printf("DTOR ~%s(\"%s\")...\n", func, s);
}
void ys_print_DTOR (const char* func, const char* s, void* pThis)
{
    if (!s || *s=='\0')  printf("DTOR ~%s()[ this=%p ]...\n", func, pThis);
    else                 printf("DTOR ~%s(\"%s\")[ this=%p ]...\n", func, s, pThis);
}
void ys_print_WINCALL (const char* s)
{
    if (!s || *s=='\0')  printf("window callback() called...\n");
    else                 printf("window callback(\"%s\") called...\n", s);
}
void ys_print_s (const char* s)
{
    if (!s || *s=='\0')  printf("()...\n");
    else                 printf("(\"%s\")...\n", s);
}


// END OF FILE
