/* iodebug.h
// Debugging and trace output
// Copyright Mar 23, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef IO_DEBUG_H
#define IO_DEBUG_H

#include "../app/trace.h"
#include "../app/errors.h"
#include "dll_api.h"

#define g_malloc_zero(size) memset(g_malloc(size),0,size)

#define s_printf printf
#define s_puts puts

#ifdef WIN32
#include <stdio.h>
#include <malloc.h>
#define g_message d_printf
#define g_error e_printf
#endif

#define debug printf("%s:%d\n",__FILE__,__LINE__)

#ifdef _DEBUG

#define d_printf printf
#define e_printf printf
#define d_puts puts
#define e_puts puts

#ifdef WIN32
#define d_assert(x) if(!(x)) __asm INT 3
DLL_API void d_heap();
#else
#define d_assert(x)
#endif

#else

DLL_API void d_printf( const char *format, ...);
DLL_API void e_printf( const char *format, ...);
DLL_API void d_puts(const char* string);
DLL_API void e_puts(const char* string);
#define d_assert(x)
#define d_heap()

#endif
#endif

