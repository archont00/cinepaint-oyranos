/*
 * br_macros.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_macros.hpp  
  
  Debugging and other macros.
  
  Makros, die fuer das ganze Projekt ein oder ausgeschaltet werden sollten. (Im
   Gegensatz dazu in "br_macros_varia.hpp" Makros, die fuer jede einzelne 
   Quelldatei aenderbar sein sollen.) Weil etliche (oeffentliche) Makros
   Funktionen aufrufen, waren diese Funktionen hier mit zu deklarieren.
*/
#ifndef br_macros_hpp
#define br_macros_hpp


#include <cstdio>               // printf()
#include <typeinfo>             // typeid()
#include "br_enums.hpp"         // ImageType, DataType


namespace br {


/*****************************************************************************
  Strings required as public because used in macros.
******************************************************************************/
const char* const  warn_preamble__ = "Br2Hdr-WARNING **";
const char* const  error_preamble__ = "Br2Hdr-ERROR ***";


/*****************************************************************************
  Helper functions for the macros - i.e. they are needed to be public. 
******************************************************************************/
void print_CTOR    (const char* func, const char* s);
void print_CTOR    (const char* func, const char* s, void* pThis);
void print_DTOR    (const char* func, const char* s);
void print_DTOR    (const char* func, const char* s, void* pThis);
void print_WINCALL (const char* s);
void print_s_with_dots (const char* s);
void fail_warning  (const char* cond, 
                    const char* file, int line, const char* func);
void assert_failed (const char* cond, 
                    const char* file, int line, const char* func);
void not_impl_image_type (const char* file, int line, const char* func, br::ImageType);
void not_impl_data_type  (const char* file, int line, const char* func, br::DataType);
void e_printf      (const char *fmt, ...);


/*****************************************************************************
  Fuer auf die Schnelle
******************************************************************************/
#define DB_             printf("[%s:%d] %s()\n",__FILE__,__LINE__,__func__);
#define PRINTF(args)    {printf("[%s:%d] ",__FILE__,__LINE__); printf args;}

/*****************************************************************************
  Error message (a trailing '\n' is added here, so avoid it in `args')
******************************************************************************/
#define BR_ERROR(args)  {\
        fprintf (stderr, "\n%s: [%s:%d] %s():\n",\
                 br::error_preamble__, __FILE__,__LINE__,__func__);\
        br::e_printf args;\
        fputc ('\n', stderr); }

/*****************************************************************************
 Warnings (a trailing '\n' is added here, so avoid it in `args')
  - BR_WARNING()  - inclusive FILE, LINE and func
  - BR_WARNING1() - inclusive func but without FILE or LINE
  - BR_WARNING2() - pure message without FILE, LINE, func.
******************************************************************************/
#define BR_WARNING(args)        {\
        printf("\n%s: ", br::warn_preamble__);\
        printf args;\
        printf(" [%s:%d %s()]\n",__FILE__,__LINE__,__func__);}

#define BR_WARNING1(args)       {\
        printf("\n%s: %s(): ", br::warn_preamble__,__func__);\
        printf args; putchar('\n');}
        
#define BR_WARNING2(args)       {\
        printf("\n%s: ", br::warn_preamble__);\
        printf args; putchar('\n');}

/*****************************************************************************
 Debugging of Ctor & Dtor calls; and window callbacks...
  - CTOR()    - constructor call
  - DTOR()    - destructor call
  - CTOR_T()  - constructor call with type info (template)
  - DTOR_T()  - destructor call with type info (template)
  - WINCALL() - call of a window callback (FLTK)
******************************************************************************/
#ifdef BR_DEBUG_CTOR
#  define CTOR(s)       br::print_CTOR(__func__,s,this);
#  define DTOR(s)       br::print_DTOR(__func__,s,this);
#  define WINCALL(s)    br::print_WINCALL(s);

#  define CTOR_T(t,s)   printf("Ctor %s <%s>",__func__, typeid(t).name()); \
                        br::print_s_with_dots(s);

#  define DTOR_T(t,s)   printf("Dtor %s <%s>",__func__, typeid(t).name()); \
                        br::print_s_with_dots(s);
#else
#  define CTOR(s)
#  define DTOR(s)
#  define WINCALL(s)
#  define CTOR_T(t,s)
#  define DTOR_T(t,s)
#endif

/*****************************************************************************
 Macros which give a msg on stderr if a condition failed + do something:
  - IF_FAIL_DO (cond,action)
  - IF_FAIL_RETURN (cond,obj)
  - IF_FAIL_THROW (cond,obj)
  - ASSERT (cond,action)
******************************************************************************/
#define IF_FAIL_DO(cond,action) \
  if (!(cond)) {br::fail_warning(#cond,__FILE__,__LINE__,__func__); action;}

#define IF_FAIL_RETURN(cond,obj) \
  if (!(cond)) {br::fail_warning(#cond,__FILE__,__LINE__,__func__); return obj;}

#define IF_FAIL_THROW(cond,obj) \
  if (!(cond)) {br::fail_warning(#cond,__FILE__,__LINE__,__func__); throw obj;}

#define ASSERT(cond,action) \
  if (!(cond)) {br::assert_failed(#cond,__FILE__,__LINE__,__func__); action;}

//  Usable as `action' parameter for "do nothing"
#define RELAX      {}
 
/*****************************************************************************
  Macros to debug Br2Hdr::Event handler
******************************************************************************/
#ifdef BR_DEBUG_RECEIVER
#  define BR_EVENT_HANDLED(e)      printf("\thandled\n");
#  define BR_EVENT_NOT_HANDLED(e)  printf("\tnot handled\n");
#else
#  define BR_EVENT_HANDLED(e)
#  define BR_EVENT_NOT_HANDLED(e)  
#endif

/*****************************************************************************
  Macros to give a "not implemented" messages
******************************************************************************/
#define NOT_IMPL_IMAGE_TYPE(type) \
   br::not_impl_image_type(__FILE__,__LINE__,__func__,type);

#define NOT_IMPL_DATA_TYPE(type)  \
   br::not_impl_data_type (__FILE__,__LINE__,__func__,type);
/**
  @note Ausserdem existiert in "br_Image.hpp" das Makro "NOT_IMPL_IMAGE_CASE(img)"
   mit einem \c Image als Argument, das detailiertere Meldungen ueber DataType,
   StoringScheme etc. eines nicht behandelbaren Falles erlaubt. Es sollte generell
   eingesetzt werden, wo eine \c Image Variable zur Verfuegung steht. In "br_macros.hpp"
   anzusiedeln nicht moeglich, da dafuer Header "br_Image.hpp" zu inkludieren, welcher
   seinerseits "br_macros.hpp" inkludiert (z.B. fuer ASSERT() und CTOR()) - zirkulaer!
*/


}  // namespace "br"

#endif  // br_macros_hpp

// END OF FILE
