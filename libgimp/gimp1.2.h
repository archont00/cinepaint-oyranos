/* gimp1.2.h
// Helps GIMP 1.2 plug-ins build in CinePaint
// Copyright March 22, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef GIMP_1_2_H
#define GIMP_1_2_H

#ifdef WIN32
#include <win32/gdkwin32.h>
#endif
#include "wire/enums.h"
#include "wire/datadir.h"
#include "wire/c_typedefs.h"
#include "intl.h"
#include "dialog.h"
#include "export.h"
#include "image_convert.h"


typedef GPlugInInfo GimpPlugInInfo;
typedef GStatusType GimpPDBStatusType;

typedef GImageType GimpImageType;
typedef GImageType GimpImageBaseType;
typedef GLayerMode GimpLayerModeEffects;

#define GIMP_RUN_INTERACTIVE RUN_INTERACTIVE
#define GIMP_RUN_NONINTERACTIVE RUN_NONINTERACTIVE
#define GIMP_RUN_WITH_LAST_VALS RUN_WITH_LAST_VALS
#define GIMP_RGB RGB
#define GIMP_RGB_IMAGE RGB_IMAGE
#define GIMP_GRAYA_IMAGE GRAYA_IMAGE
#define GIMP_GRAY_IMAGE GRAY_IMAGE
#define GIMP_DARKEN_ONLY_MODE DARKEN_ONLY_MODE
#define GIMP_LIGHTEN_ONLY_MODE LIGHTEN_ONLY_MODE
#define GIMP_HUE_MODE HUE_MODE
#define GIMP_SATURATION_MODE SATURATION_MODE
#define GIMP_COLOR_MODE COLOR_MODE
#define GIMP_MULTIPLY_MODE MULTIPLY_MODE
#define GIMP_SCREEN_MODE SCREEN_MODE
#define GIMP_DISSOLVE_MODE DISSOLVE_MODE
#define GIMP_DIFFERENCE_MODE DIFFERENCE_MODE
#define GIMP_VALUE_MODE VALUE_MODE
#define GIMP_ADDITION_MODE ADDITION_MODE
#define GIMP_GRAY GRAY

#define GIMP_NORMAL_MODE NORMAL_MODE
#define GIMP_INDEXED INDEXED
#define GIMP_INDEXED_IMAGE INDEXED_IMAGE
#define GIMP_INDEXEDA_IMAGE INDEXEDA_IMAGE
#define GIMP_RGBA_IMAGE RGBA_IMAGE

/* rsr: this is backwards from how old plug-ins point to CanvasDrawable */
#define GimpDrawable TileDrawable

#define set_gimp_PLUG_IN_INFO_PTR set_gimp_PLUG_IN_INFO
#define gimp_image_undo_disable gimp_image_disable_undo
#define gimp_image_undo_enable gimp_image_enable_undo

#define gimp_tile_ref lib_tile_ref
/*lib_tile_ref_zero*/
#define gimp_tile_unref lib_tile_unref_free
/*lib_tile_flush*/
#define gimp_tile_cache_size get_lib_tile_cache_size
#define gimp_tile_cache_ntiles get_lib_tile_cache_ntiles
#define gimp_tile_width get_lib_tile_width
#define gimp_tile_height get_lib_tile_height

#define gimp_drawable_is_layer gimp_drawable_layer
#define gimp_drawable_is_gray gimp_drawable_gray		
#define gimp_drawable_is_rgb gimp_drawable_color		
#define gimp_drawable_get_image gimp_drawable_image_id

#ifdef WIN32
#define NATIVE_WIN32
#endif

#endif
