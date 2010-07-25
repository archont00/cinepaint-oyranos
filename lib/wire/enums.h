/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMP_ENUMS_H__
#define __GIMP_ENUMS_H__

#include "iodebug.h"
#include "c_typedefs.h"
#include "precision.h"

typedef enum 
{
  RGB         = 0,
  GRAY        = 1,
  INDEXED     = 2,
  U16_RGB     = 3,
  U16_GRAY    = 4,
  U16_INDEXED = 5,
  FLOAT_RGB   = 6,
  FLOAT_GRAY  = 7,
  FLOAT16_RGB   = 8,
  FLOAT16_GRAY  = 9, 
  BFP_RGB       = 10,
  BFP_GRAY      = 11 
} GImageType;

typedef enum 
{ RGB_IMAGE          = 0,
  RGBA_IMAGE         = 1,
  GRAY_IMAGE         = 2,
  GRAYA_IMAGE        = 3,
  INDEXED_IMAGE      = 4,
  INDEXEDA_IMAGE     = 5,
  U16_RGB_IMAGE      = 6,
  U16_RGBA_IMAGE     = 7,
  U16_GRAY_IMAGE     = 8,
  U16_GRAYA_IMAGE    = 9,
  U16_INDEXED_IMAGE  = 10,
  U16_INDEXEDA_IMAGE = 11, 
  FLOAT_RGB_IMAGE    = 12,
  FLOAT_RGBA_IMAGE   = 13,
  FLOAT_GRAY_IMAGE   = 14,
  FLOAT_GRAYA_IMAGE  = 15,
  FLOAT16_RGB_IMAGE    = 16,
  FLOAT16_RGBA_IMAGE   = 17,
  FLOAT16_GRAY_IMAGE   = 18,
  FLOAT16_GRAYA_IMAGE  = 19,
  BFP_RGB_IMAGE    = 20,
  BFP_RGBA_IMAGE   = 21,
  BFP_GRAY_IMAGE   = 22,
  BFP_GRAYA_IMAGE  = 23 
} GDrawableType;

typedef enum 
{
  NORMAL_MODE       = 0,
  DISSOLVE_MODE     = 1,
  MULTIPLY_MODE     = 3,
  SCREEN_MODE       = 4,
  OVERLAY_MODE      = 5,
  DIFFERENCE_MODE   = 6,
  ADDITION_MODE     = 7,
  SUBTRACT_MODE     = 8,
  DARKEN_ONLY_MODE  = 9,
  LIGHTEN_ONLY_MODE = 10,
  HUE_MODE          = 11,
  SATURATION_MODE   = 12,
  COLOR_MODE        = 13,
  VALUE_MODE        = 14
} GLayerMode;

typedef enum
{
  BG_IMAGE_FILL,
  WHITE_IMAGE_FILL,
  TRANS_IMAGE_FILL
} GFillType;

typedef enum
{
  PROC_PLUG_IN = 1,
  PROC_EXTENSION = 2,
  PROC_TEMPORARY = 3
} GProcedureType;

/* This enum is mirrored in "app/plug_in.c", make sure
 *  they are identical or bad things will happen.
 */
typedef enum
{
  RUN_INTERACTIVE    = 0x0,
  RUN_NONINTERACTIVE = 0x1,
  RUN_WITH_LAST_VALS = 0x2
} GRunModeType;

typedef enum
{
  GIMP_PDB_EXECUTION_ERROR,
  GIMP_PDB_CALLING_ERROR,
  GIMP_PDB_PASS_THROUGH,
  GIMP_PDB_SUCCESS,
  GIMP_PDB_CANCEL
} GStatusType;

typedef enum
{
  GIMP_INTERNAL,
  GIMP_PLUGIN,
  GIMP_EXTENSION,
  GIMP_TEMPORARY
} GimpPDBProcType;

typedef enum
{
  GIMP_HORIZONTAL,
  GIMP_VERTICAL,
  GIMP_UNKNOWN
} GimpOrientationType;

typedef enum
{
  GIMP_PDB_INT32,
  GIMP_PDB_INT16,
  GIMP_PDB_INT8,
  GIMP_PDB_FLOAT,
  GIMP_PDB_STRING,
  GIMP_PDB_INT32ARRAY,
  GIMP_PDB_INT16ARRAY,
  GIMP_PDB_INT8ARRAY,
  GIMP_PDB_FLOATARRAY,
  GIMP_PDB_STRINGARRAY,
  GIMP_PDB_COLOR,
  GIMP_PDB_REGION,
  GIMP_PDB_DISPLAY,
  GIMP_PDB_IMAGE,
  GIMP_PDB_LAYER,
  GIMP_PDB_CHANNEL,
  GIMP_PDB_DRAWABLE,
  GIMP_PDB_SELECTION,
  GIMP_PDB_BOUNDARY,
  GIMP_PDB_PATH,
  /*  GIMP_PDB_PARASITE, */
  GIMP_PDB_STATUS,
  GIMP_PDB_END
} GimpPDBArgType;

typedef enum
{
  GIMP_APPLY,
  GIMP_DISCARD
} GimpMaskApplyMode;

typedef enum
{
  GIMP_EXPAND_AS_NECESSARY,
  GIMP_CLIP_TO_IMAGE,
  GIMP_CLIP_TO_BOTTOM_LAYER,
  GIMP_FLATTEN_IMAGE
} GimpMergeType;

typedef enum
{
  GIMP_RED_CHANNEL,
  GIMP_GREEN_CHANNEL,
  GIMP_BLUE_CHANNEL,
  GIMP_GRAY_CHANNEL,
  GIMP_INDEXED_CHANNEL,
  GIMP_AUXILLARY_CHANNEL
} GimpChannelType;


typedef enum
{
  GIMP_NO_DITHER,
  GIMP_FS_DITHER,
  GIMP_FSLOWBLEED_DITHER,
  GIMP_FIXED_DITHER,
  GIMP_NODESTRUCT_DITHER
} GimpConvertDitherType;

typedef enum
{
  GIMP_MAKE_PALETTE,
  GIMP_REUSE_PALETTE,
  GIMP_WEB_PALETTE,
  GIMP_MONO_PALETTE,
  GIMP_CUSTOM_PALETTE
} GimpConvertPaletteType;


typedef enum
{
  GIMP_OFFSET_BACKGROUND,
  GIMP_OFFSET_TRANSPARENT
} GimpChannelOffsetType;

typedef enum
{
  ICC_IMAGE_PROFILE,
  ICC_PROOF_PROFILE
} CMSProfileType;

#define PARAM_INT32 GIMP_PDB_INT32
#define PARAM_INT16 GIMP_PDB_INT16
#define PARAM_INT8 GIMP_PDB_INT8
#define PARAM_FLOAT GIMP_PDB_FLOAT
#define PARAM_STRING GIMP_PDB_STRING
#define PARAM_INT32ARRAY GIMP_PDB_INT32ARRAY
#define PARAM_INT16ARRAY GIMP_PDB_INT16ARRAY
#define PARAM_INT8ARRAY GIMP_PDB_INT8ARRAY
#define PARAM_FLOATARRAY GIMP_PDB_FLOATARRAY
#define PARAM_STRINGARRAY GIMP_PDB_STRINGARRAY
#define PARAM_COLOR GIMP_PDB_COLOR
#define PARAM_REGION GIMP_PDB_REGION
#define PARAM_DISPLAY GIMP_PDB_DISPLAY
#define PARAM_IMAGE GIMP_PDB_IMAGE
#define PARAM_LAYER GIMP_PDB_LAYER
#define PARAM_CHANNEL GIMP_PDB_CHANNEL
#define PARAM_DRAWABLE GIMP_PDB_DRAWABLE
#define PARAM_SELECTION GIMP_PDB_SELECTION
#define PARAM_BOUNDARY GIMP_PDB_BOUNDARY
#define PARAM_PATH GIMP_PDB_PATH
#define PARAM_PARASITE GIMP_PDB_PARASITE
#define PARAM_STATUS GIMP_PDB_STATUS
#define PARAM_END GIMP_PDB_END

#define STATUS_EXECUTION_ERROR GIMP_PDB_EXECUTION_ERROR
#define STATUS_CALLING_ERROR GIMP_PDB_CALLING_ERROR
#define STATUS_PASS_THROUGH GIMP_PDB_PASS_THROUGH
#define STATUS_SUCCESS GIMP_PDB_SUCCESS
#define STATUS_CANCEL GIMP_PDB_CANCEL

typedef void   (* GRunProc) (char    *name,
			     int      nparams,
			     GParam  *param,
			     int     *nreturn_vals,
			     GParam **return_vals);

struct GPlugInInfo
{
  /* called when the gimp application initially starts up */
  void (*init_proc) (void);

  /* called when the gimp application exits */
  void (*quit_proc) (void);

  /* called by the gimp so that the plug-in can inform the
   *  gimp of what it does. (ie. installing a procedure database
   *  procedure).
   */
  void (*query_proc) (void);

  /* called to run a procedure the plug-in installed in the
   *  procedure database.
   */
  void (*run_proc) (char    *name,
		    int      nparams,
		    GParam  *param,
		    int     *nreturn_vals,
		    GParam **return_vals);
};

#endif /* __GIMP_ENUMS_H__ */


