/* trace_macros.h */

/*  Macros for removable tracing and with line numbering  -hsbo
 *   
 *  No ``#ifndef __TRACE_MACROS_H_'' switch, because file shall be includable
 *   multiple times! 
 *
 *  Usage: 
 *  
 *  foo.c:
 *  | #define TRACE_MACROS_ON   // makes sure to have active macros
 *  | #undef TRACE_MACROS_ON    // makes sure to have deactive macros
 *  | #include "trace_macros.h"
 *  |
 *  | ....(use the macros)...
 */
     
#include "trace.h"


/*  First we undefine all macros to avoid "redefine" warnings */
#undef TRACE_ENTER
#undef TRACE_EXIT
#undef TRACE_BEGIN
#undef TRACE_END
#undef TRACE_PRINTF   

#ifdef TRACE_MACROS_ON
   /*  Define macros for tracing and with line numbering */
#  define TRACE_ENTER(args)     {trace_line_no_=__LINE__; trace_enter args ;}
#  define TRACE_EXIT            {trace_line_no_=__LINE__; trace_exit();}
#  define TRACE_BEGIN(args)     {trace_line_no_=__LINE__; trace_begin args ;}
#  define TRACE_END             {trace_line_no_=__LINE__; trace_end();}
#  define TRACE_PRINTF(args)    {trace_line_no_=__LINE__; trace_printf args ;}
#else
   /*  Define empty macros */
#  define TRACE_ENTER(args)
#  define TRACE_EXIT
#  define TRACE_BEGIN(args)
#  define TRACE_END
#  define TRACE_PRINTF(args)
#endif

/* END OF FILE */
