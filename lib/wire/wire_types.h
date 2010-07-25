/* wire_types.h
   Robin.Rowe@MovieEditor.com 5/25/03
   Previously part of app/plugin_main.h */

#ifndef WIRE_TYPES_H
#define WIRE_TYPES_H

#include <glib.h>
#include "enums.h"

/* rsr changed from GDrawable */

struct TileDrawable
{ gint32 id;            /* drawable ID */
  guint width;          /* width of drawble */
  guint height;         /* height of drawble */
  guint bpp;            /* bytes per pixel of drawable */
  guint num_channels;   /* number of channels of drawable */ 
  guint ntile_rows;     /* # of tile rows */
  guint ntile_cols;     /* # of tile columns */
  GTile *tiles;         /* the normal tiles */
  GTile *shadow_tiles;  /* the shadow tiles */
};

struct GPixelRgn
{
  guchar *data;         /* pointer to region data */
  TileDrawable *drawable;  /* pointer to drawable */
  guint bpp;            /* bytes per pixel */
  guint rowstride;      /* bytes per pixel row */
  guint x, y;           /* origin */
  guint w, h;           /* width and height of region */
  guint dirty : 1;      /* will this region be dirtied? */
  guint shadow : 1;     /* will this region use the shadow or normal tiles */
  guint process_count;  /* used internally */
};

struct GParamDef
{
  GParamType type;
  char *name;
  char *description;
};

struct GParamColor
{
  guint8 red;
  guint8 green;
  guint8 blue;
};

struct GParamRegion
{
  gint32 x;
  gint32 y;
  gint32 width;
  gint32 height;
};

union GParamData
{
  gint32 d_int32;
  gint16 d_int16;
  gint8 d_int8;
  gdouble d_float;
  gchar *d_string;
  gint32 *d_int32array;
  gint16 *d_int16array;
  gint8 *d_int8array;
  gdouble *d_floatarray;
  gchar **d_stringarray;
  GParamColor d_color;
  GParamRegion d_region;
  gint32 d_display;
  gint32 d_image;
  gint32 d_layer;
  gint32 d_layer_mask;
  gint32 d_channel;
  gint32 d_drawable;
  gint32 d_selection;
  gint32 d_boundary;
  gint32 d_path;
  gint32 d_unit;
  gint32 d_status;
};

struct GParam
{
  GParamType type;
  GParamData data;
};

#endif
