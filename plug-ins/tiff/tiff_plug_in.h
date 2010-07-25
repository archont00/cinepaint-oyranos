/*
 * "$Id: tiff_plug_in.h,v 1.3 2006/12/11 16:01:11 beku Exp $"
 *
 *   header for the tiff plug-in in CinePaint
 *
 *   Copyright 2002-2004   Kai-Uwe Behrmann (ku.b@gmx.de)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _TIFF_PLUG_IN_H_
#define _TIFF_PLUG_IN_H_

#include <tiffconf.h>
#include <tiffio.h>
#include <stdlib.h>

/***   some configuration switches   ***/

#if 0     /* set i/o stream to 16-bit Log or 32-bit float for SGI Log HDR format*/
  #define SGILOGDATAFMT = SGILOGDATAFMT_16BIT_LOG
#else
  #define SGILOGDATAFMT SGILOGDATAFMT_FLOAT
#endif

#if GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 2
  #define SGILOGDATAFMT SGILOGDATAFMT_8BIT;
#endif


/***   needed includes   ***/


#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION > 0 && GIMP_MINOR_VERSION < 17
    #include "gtk/gtk.h"
    #ifndef EXTERN_PLUG_IN
      #include "libgimp/config.h"
    #endif
    #include "libgimp/gimp.h"
#else
  #ifdef GIMP_VERSION
    #ifndef EXTERN_PLUG_IN
      #include "config.h"
    #endif
    #include <libgimp/gimp.h>
    #include <libgimp/gimpui.h>
    #if GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 2
      #ifdef EXTERN_PLUG_IN
        #include "libgimp/gimpintl.h"
      #else
        #include "libgimp/stdplugins-intl.h"
      #endif
    #endif
  #else
    #ifndef EXTERN_PLUG_IN
      #include "config.h"
    #endif
    #include "lib/plugin_main.h"
    #include "lib/float16.h"
    #include "lib/ui.h"
    #include "lib/intl.h"
    #include "lib/dialog.h"
    #include "lib/widgets.h"
    #include "lib/export.h"
    #include "libgimp/stdplugins-intl.h"

    #include <gdk/gdkkeysyms.h>
    #include <gtk/gtk.h>
  #endif
#endif
#include <lcms.h>
#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION == 15
  #include "libgimp/gimpcms.h"
#endif


/*** definitions for compatibillity ***/

#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION > 0 && GIMP_MINOR_VERSION < 17
                            /* filmgimp */
    #define GimpPDBStatusType GStatusType
    #define GIMP_PDB_SUCCESS STATUS_SUCCESS
    #define GIMP_PDB_STATUS PARAM_STATUS
    #define GIMP_PDB_CALLING_ERROR STATUS_CALLING_ERROR
    #define GIMP_PDB_EXECUTION_ERROR STATUS_EXECUTION_ERROR
    #define GIMP_RUN_INTERACTIVE RUN_INTERACTIVE
    #define GIMP_RUN_NONINTERACTIVE RUN_NONINTERACTIVE
    #define GIMP_RUN_WITH_LAST_VALS RUN_WITH_LAST_VALS
    #define GimpImageType GimpDrawableType
    #define _(x) x
    #define ROUND(x) ((int) ((x) + 0.5))
    #define gimp_image_undo_disable(x) x
    #define gimp_image_undo_enable(x) x
#else
  #ifdef GIMP_VERSION          /* gimp
    #define GParamDef GimpParamDef
    #define GParam GimpParam
    #define GRunModeType GimpRunModeType
    #define GStatusType GimpStatusType */
    #define GRAY GIMP_GRAY 
    #define GRAY_IMAGE GIMP_GRAY_IMAGE 
    #define GRAYA_IMAGE GIMP_GRAYA_IMAGE 
    #define INDEXED GIMP_INDEXED 
    #define INDEXED_IMAGE GIMP_INDEXED_IMAGE 
    #define INDEXEDA_IMAGE GIMP_INDEXEDA_IMAGE 
    #define RGB GIMP_RGB
    #define RGB_IMAGE GIMP_RGB_IMAGE 
    #define RGBA_IMAGE GIMP_RGBA_IMAGE 
    #define NORMAL_MODE GIMP_NORMAL_MODE 
    #define DARKEN_ONLY_MODE GIMP_DARKEN_ONLY_MODE 
    #define LIGHTEN_ONLY_MODE GIMP_LIGHTEN_ONLY_MODE 
    #define HUE_MODE GIMP_HUE_MODE 
    #define SATURATION_MODE GIMP_SATURATION_MODE 
    #define COLOR_MODE GIMP_COLOR_MODE 
    #define MULTIPLY_MODE GIMP_MULTIPLY_MODE 
    #define SCREEN_MODE GIMP_SCREEN_MODE 
    #define DISSOLVE_MODE GIMP_DISSOLVE_MODE 
    #define DIFFERENCE_MODE GIMP_DIFFERENCE_MODE 
    #define VALUE_MODE GIMP_VALUE_MODE 
    #define ADDITION_MODE GIMP_ADDITION_MODE 
    #if GIMP_MINOR_VERSION >= 3
      #define GimpRunModeType GimpRunMode
      #define GimpExportReturnType GimpExportReturn
    #else
      #define GIMP_ORIENTATION_HORIZONTAL GIMP_HORIZONTAL
      #define GIMP_ORIENTATION_VERTICAL GIMP_VERTICAL
    #endif
  #else                     /* cinepaint */
    #define GIMP_ORIENTATION_HORIZONTAL GIMP_HORIZONTAL
    #define GIMP_ORIENTATION_VERTICAL GIMP_VERTICAL
    #define ROUND(x) ((int) ((x) + 0.5))
  #endif
#endif

#if (!__unix__ && !__APPLE__)
typedef struct tiff TIFF;
#endif

#ifndef COMPRESSION_JP2000
#define COMPRESSION_JP2000 34712
#endif
#ifndef PHOTOMETRIC_ICCLAB
#define PHOTOMETRIC_ICCLAB 9
#endif
#ifndef PHOTOMETRIC_ITULAB
#define PHOTOMETRIC_ITULAB 10
#endif

/***   versioning   ***/

extern char plug_in_version_[64];
extern char Version_[];


/*** struct definitions ***/

typedef struct
{
  gint  use_icc_profile;        /* use ICC Profile data for image saving */
  gint  compression;		/* compression type */
  gint  fillorder;		/* msb versus lsb */
  gint  premultiply;		/* assoc versus unassociated alpha */
  gint  planar_separate;        /* write separate planes per sample */
} TiffSaveVals;

typedef struct
{
  gint  run;
  gint  alpha;
} TiffSaveInterface;

typedef struct {
  gint32 ID;
  GimpDrawable *drawable;
  GimpPixelRgn pixel_rgn;
  guchar *pixels,               /* raw pixel buffer for colors */
         *pixel;                /* pointer to actual position in pixels */
} ChannelData;

/*TIFF *tif; */
  /*unsigned short photomet;
  int  alpha;
  int  num_extra; */
  /*gint align; */


/***   global variables   ***/

extern int write_separate_planes;

extern int use_icc;

extern int color_type_;

extern gchar *image_comment;
extern gchar *image_copyright;
extern gchar *image_artist;

extern TiffSaveVals tsvals_;

extern TiffSaveInterface tsint;

/*** macros ***/

  /* --- Helpers --- */

/* Make sure this is used with a, b, t 32bit unsigned ints */
#define INT_MULT_16(a,b,t) ((t) = (a) * (b) + 0x8000, ((((t) >> 16) + (t)) >> 16))

/* m_free (void*) */
#define m_free(x) {                                         \
  if (x != NULL) {                                          \
    free (x); x = NULL;                                     \
  } else {                                                  \
    g_print ("%s:%d %s() nothing to delete " #x "\n",       \
    __FILE__,__LINE__,__func__);                            \
  }                                                         \
}

/* m_new (void*, type, size_t, action) */
#define m_new(ptr,type,size,action) {                       \
  if (ptr != NULL)                                          \
    m_free (ptr)                                            \
  if ((size) <= 0) {                                        \
    g_print ("%s:%d %s() nothing to allocate %d\n",         \
    __FILE__,__LINE__,__func__, (int)size);                 \
  } else {                                                  \
    ptr = calloc (sizeof (type), (size_t)size);             \
  }                                                         \
  if (ptr == NULL) {                                        \
    g_message ("%s:%d %s() %s %d %s %s .",__FILE__,__LINE__,\
         __func__, _("Can not allocate"),(int)size,         \
         _("bytes of  memory for"), #ptr);                  \
    action;                                                 \
  }                                                         \
}



#endif /*_TIFF_PLUG_IN_H_*/

