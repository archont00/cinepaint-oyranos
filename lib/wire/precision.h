/* precision.h */
/* Bit-stuff for wire protocol, resolves conflict between tag.h and enums.h */
/* Copyright Jan 1, 2003, Robin.Rowe@MovieEditor.com */
/* License MIT (http://opensource.org/licenses/mit-license.php) */

#ifndef PRECISION_H
#define PRECISION_H

#include "dll_api.h"

/* supported precisions */
typedef enum
{  
  PRECISION_NONE,
  PRECISION_U8             = 1,
  PRECISION_U16           = 2,
  PRECISION_FLOAT       = 3,
  PRECISION_FLOAT16   = 4,
  PRECISION_BFP           = 5
} Precision;

typedef Precision GPrecisionType;

#define PRECISION_U8_STRING "8-bit Unsigned Integer"
#define PRECISION_U16_STRING "16-bit Unsigned Integer"
#ifdef RNH_FLOAT
#define PRECISION_FLOAT16_STRING "16-bit RnH Short Float"
#else
#define PRECISION_FLOAT16_STRING "16-bit OpenEXR Half Float"
#endif
#define PRECISION_FLOAT_STRING "32-bit IEEE Float"
#define PRECISION_BFP_STRING "16-bit Fixed Point 0-2.0"

#endif



