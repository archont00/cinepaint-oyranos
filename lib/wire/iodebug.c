/* iodebug.c
// Debugging and trace output
// Copyright Mar 23, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#include "iodebug.h"
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef DEBUG_
void d_printf( const char *format, ...)
{
  va_list ap;
  fprintf(stderr,"Debug: ");
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
}

void d_puts(const char* string)
{ printf("Debug: %s\n",string); }

#else
void d_printf( const char *format, ...)
{}

void d_puts(const char* string)
{}

#endif

void e_printf( const char *format, ...)
{
  va_list ap;
  fprintf(stderr,"Error: ");
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
  printf("\n");
}
void e_puts(const char* string)
{ printf("Error: %s\n",string); }


#ifdef _DEBUG

void d_heap()
{	int  heapstatus = _heapchk();
	switch( heapstatus )
	{	case _HEAPOK:
			return;
		case _HEAPEMPTY:
			return;
		case _HEAPBADBEGIN:
			printf( "ERROR - bad start of heap\n" );
		break;
		case _HEAPBADNODE:
			printf( "ERROR - bad node in heap\n" );
		break;
	}
	__asm INT 3;
}

#endif
