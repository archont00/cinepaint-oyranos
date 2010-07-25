/*
 * ys_dbg.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  ys_dbg.hpp  -  debug macros.
  
  Makros solcherart, die fuer ein ganzes Projekt ein oder ausgeschaltet werden
   sollten. (Im Gegensatz dazu in "ys_dbg_varia.hpp" Makros, die fuer jede
   einzelne Quelldatei aenderbar sein sollen.)
*/
#ifndef ys_dbg_hpp
#define ys_dbg_hpp


#include <cstdio>       // printf()
#include <typeinfo>     // typeid()

/*
*  helper functions defined in "ys_dbg.cpp"
*/
void ys_vprintf       (const char *fmt, ...);
void ys_print_CTOR    (const char* func, const char* s);
void ys_print_CTOR    (const char* func, const char* s, void* pThis);
void ys_print_DTOR    (const char* func, const char* s);
void ys_print_DTOR    (const char* func, const char* s, void* pThis);
void ys_print_WINCALL (const char* s);
void ys_print_s       (const char* s);

/*
*  Fuer auf die Schnelle
*/
#define DB_           printf("[%s:%d] %s()\n",__FILE__,__LINE__,__func__);
#define PRINTF(args)  {printf("[%s:%d] ",__FILE__,__LINE__); printf args;}

/*
*  Debugging of Ctor & Dtor calls; and window callbacks...
*   - CTOR()    - constructor call
*   - DTOR()    - destructor call
*   - CTOR_T()  - constructor call with type info (template)
*   - DTOR_T()  - destructor call with type info (template)
*   - WINCALL() - call of a window callback (FLTK)
*/
#ifdef YS_DEBUG_CTOR
#  define CTOR(s)     ys_print_CTOR(__func__,s,this);
#  define DTOR(s)     ys_print_DTOR(__func__,s,this);
#  define WINCALL(s)  ys_print_WINCALL(s);
#  define CTOR_T(t,s) {printf("Ctor %s <%s>",__func__, typeid(t).name());\
                      ys_print_s(s);}
#  define DTOR_T(t,s) {printf("Dtor %s <%s>",__func__, typeid(t).name());\
                      ys_print_s(s);}
#else
#  define CTOR(s)
#  define DTOR(s)
#  define WINCALL(s)
#  define CTOR_T(t,s)
#  define DTOR_T(t,s)
#endif


#endif  // ys_dbg_hpp

// END OF FILE
