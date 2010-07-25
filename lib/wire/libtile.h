/* wire/libtile.h
// Tile and tile cache support in the wire and libcinepaint
// Copyright May 25, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef LIB_TILE_H
#define LIB_TILE_H

#include <glib.h>
#include "c_typedefs.h"
#include "dl_list.h"

struct GTile
{ DL_node dl_node;
  guint ewidth;        /* the effective width of the tile */
  guint eheight;       /* the effective height of the tile */
  guint bpp;           /* the bytes per pixel (1, 2, 3 or 4 ) */
  guint tile_num;      /* the number of this tile within the drawable */
  guint16 ref_count;   /* reference count for the tile */
  guint dirty : 1;     /* is the tile dirty? has it been modified? */
  guint shadow: 1;     /* is this a shadow tile */
  guchar *data;        /* the pixel data for the tile */
  TileDrawable *drawable; /* the drawable this tile came from */
#ifdef _DEBUG
  int is_allocated;
#endif
};

#define TILE_WIDTH   lib_tile_width
#define TILE_HEIGHT  lib_tile_height

extern gint lib_tile_width;
extern gint lib_tile_height;

DLL_API void lib_tile_ref(GTile* tile);
void lib_tile_ref_zero(GTile* tile);
DLL_API void lib_tile_unref_free(GTile* tile,int dirty);
void lib_tile_flush(GTile *tile);
DLL_API void get_lib_tile_cache_size(gulong kilobytes);
DLL_API void get_lib_tile_cache_ntiles(gulong ntiles);
DLL_API guint get_lib_tile_width(void);
DLL_API guint get_lib_tile_height(void);
DLL_API void lib_tile_cache_purge(GTile* tiles,int ntiles);

#endif


