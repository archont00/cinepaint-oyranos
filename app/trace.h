/* trace.h */

#ifndef TRACE_H
#define TRACE_H

#include "wire/c_typedefs.h"

#ifdef WIN32

#else
#include <sys/time.h>
#endif

#include <unistd.h>


typedef struct
{
  struct timeval a;
  struct timeval b;
} TraceTimer;

extern int trace_depth_;        /*  depth of tracing  -hsbo */
extern int trace_line_no_;      /*  set by TRACE macros  -hsbo */

void trace_enter   (const char * func);
void trace_exit    (void); 
void trace_begin   (const char * format, ...);
void trace_end     (void);
void trace_printf  (const char * format, ...);

void trace_timer_init (TraceTimer * t);
void trace_timer_tick (TraceTimer * t, char * pre, char * post);
void trace_time_before ( TraceTimer *t );
void trace_time_after ( TraceTimer * t);
void print_elapsed (char *, TraceTimer *t ); 

void 
print_area (
		PixelArea *pa,
		int row,
		int how_many_rows
		);
void
pixelrow_print (
		 PixelRow *row
		); 

void 
print_area_init(
                 PixelArea * pa,
                 Canvas * c,
                 int x,
                 int y,
                 int w,
                 int h,
                 int will_dirty,
		 char * name
		);

#endif

