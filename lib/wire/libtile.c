/* lib/libtile.c
// Tile and tile cache support in the wire and libcinepaint
// Copyright May 25, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
//
// Similar to functions in app/tile_cache.c.
// Total rewrite by rsr to eliminate tile memory corruption.
*/

#include <string.h>
#include "../lib/plugin_main.h"
#include "../lib/wire/protocol.h"
#include "../lib/wire/wire.h"
#include "../lib/wire/taskswitch.h"
#include "../lib/wire/iodebug.h"
#include "../lib/wire/wire_types.h"

#ifdef WIN32
#include "../lib/wire/event.h"
#define _writefd 0
#define _readfd 0
#define _shm_addr 0
#else
  extern int _writefd;
  extern int _readfd;
  extern guchar* _shm_addr;
#endif

/*  Percentage of the maximum cache size to clear
 *   from the cache when an eviction is necessary */

#define FREE_QUANTUM 0.1

static void  lib_tile_get_wire(GTile *tile);
static void  lib_tile_put_wire(GTile *tile);
static void  lib_tile_cache_insert(GTile *tile);
static void  lib_tile_cache_detach(GTile *tile);
static void  lib_tile_cache_shrink(void);

gint lib_tile_width = -1;
gint lib_tile_height = -1;

typedef struct LibGTileCache LibGTileCache;

struct LibGTileCache
{	DL_list dl_list;
	gulong max_tile_size;
	gulong cur_cache_size;
	gulong max_cache_size;
};

static LibGTileCache ltc;

static void LTC_Initialize()
{	if(ltc.max_tile_size)
	{	return;
	}
	ltc.max_tile_size = lib_tile_width * lib_tile_height * 4;
}

static void LTC_MoveTileToEnd(GTile* tile)
{	if(ltc.dl_list.tail == (DL_node*)tile)
	{	return;
	}
	DL_remove(&ltc.dl_list,(DL_node*)tile);
	DL_append(&ltc.dl_list,(DL_node*)tile);
}

#define LTC_is_empty() DL_is_empty(&ltc.dl_list)
#define LTC_is_heavy() ((ltc.cur_cache_size + ltc.max_cache_size * FREE_QUANTUM) > ltc.max_cache_size)

void lib_tile_flush(GTile *tile)
{	if (tile && tile->data && tile->dirty)
	{	lib_tile_put_wire(tile);
		tile->dirty = FALSE;
}	}

static void lib_tile_cache_shrink()
{	while(!LTC_is_empty() && LTC_is_heavy())
	{	GTile* tile = (GTile*) ltc.dl_list.head;
		lib_tile_cache_detach(tile);
		lib_tile_unref_free(tile,FALSE);
}	}

static void LTC_AddTile(GTile* tile)
{	if ((ltc.cur_cache_size + ltc.max_tile_size) > ltc.max_cache_size)
	{	lib_tile_cache_shrink();
		if((ltc.cur_cache_size + ltc.max_tile_size) > ltc.max_cache_size)
		{	d_puts("LTC_AddTile failed!");
			return;
	}	}
	DL_append(&ltc.dl_list,(DL_node*)tile);
	ltc.cur_cache_size += ltc.max_tile_size;
	/* Reference the tile so that it won't be returned to
	*  the main gimp application immediately.
	*/
	tile->ref_count += 1;
	if (tile->ref_count == 1)
	{	lib_tile_get_wire(tile);
		tile->dirty = FALSE;
}	}

static void lib_tile_cache_insert(GTile *tile)
{	LTC_Initialize();
	d_assert(1==tile->is_allocated);
	if(DL_is_used_node(&ltc.dl_list,(DL_node*)tile))
	{	LTC_MoveTileToEnd(tile);
	}
	else
	{	LTC_AddTile(tile);
}	}

static void lib_tile_put_wire(GTile *tile)
{	GPTileReq tile_req;
	GPTileData tile_data;
	GPTileData *tile_info;
	WireMessage msg;
#ifdef WIN32

#else
	extern int _writefd;
	extern int _readfd;
	extern guchar* _shm_addr;
#endif
	tile_req.drawable_ID = -1;
	tile_req.tile_num = 0;
	tile_req.shadow = 0;
	if (!gp_tile_req_write (_writefd, &tile_req))
	{	gimp_quit ();
	}
	TaskSwitchToWire();
	if (!wire_read_msg(_readfd, &msg))
	{	gimp_quit ();
	}
	if (msg.type != GP_TILE_DATA)
	{	g_message ("unexpected message[4]: %d %s\n",msg.type,Get_gp_name(msg.type));
		gimp_quit ();
	}
	tile_info = msg.data;
	tile_data.drawable_ID = tile->drawable->id;
	tile_data.tile_num = tile->tile_num;
	tile_data.shadow = tile->shadow;
	tile_data.bpp = tile->bpp;
	tile_data.width = tile->ewidth;
	tile_data.height = tile->eheight;
	tile_data.data = NULL;
#ifdef BUILD_SHM
	tile_data.use_shm = tile_info->use_shm;
	if (tile_info->use_shm)
	memcpy (_shm_addr, tile->data, tile->ewidth * tile->eheight * tile->bpp);
	else
#endif
	tile_data.data = tile->data;
	if (!gp_tile_data_write(_writefd, &tile_data))
	{	gimp_quit();
	}
	TaskSwitchToWire();
	if (!wire_read_msg(_readfd, &msg))
	{	gimp_quit();
	}
	if (msg.type != GP_TILE_ACK)
	{	g_message("unexpected message[5]: %d %s\n",msg.type,Get_gp_name(msg.type));
		gimp_quit();
	}
	wire_destroy(&msg);
	d_assert(1==tile->is_allocated);
}

void lib_tile_ref(GTile *tile)
{	if(!tile)
	{	return;
	}
	d_assert(1==tile->is_allocated);
	tile->ref_count += 1;
	if (tile->ref_count == 1)
	{	lib_tile_get_wire(tile);
		tile->dirty = FALSE;
	}
	lib_tile_cache_insert(tile);
	d_assert(1==tile->is_allocated);
}

void lib_tile_ref_zero(GTile *tile)
{	if(!tile)
	{	return;
	}	
	tile->ref_count += 1;
	if (tile->ref_count == 1)
	{	tile->data = g_new (guchar, tile->ewidth * tile->eheight * tile->bpp);
		memset(tile->data, 0, tile->ewidth * tile->eheight * tile->bpp);
	}
	lib_tile_cache_insert(tile);
	d_assert(1==tile->is_allocated);
}

void lib_tile_unref_free(GTile* tile,int dirty)
{	if(!tile)
	{	return;
	}
	d_assert(1==tile->is_allocated);
	tile->ref_count -= 1;
	tile->dirty |= dirty;
	if(tile->ref_count)
	{	return;
	}
	lib_tile_flush(tile);
	g_free(tile->data);
	tile->data = NULL;
	d_assert(1==tile->is_allocated);
}

void get_lib_tile_cache_size(gulong kilobytes)
{	ltc.max_cache_size = kilobytes * 1024;
}

void get_lib_tile_cache_ntiles(gulong ntiles)
{	get_lib_tile_cache_size((ntiles * lib_tile_width * lib_tile_height * 4) / 1024);
}

static void lib_tile_get_wire(GTile *tile)
{	GPTileReq tile_req;
	GPTileData *tile_data;
	WireMessage msg;
	tile_req.drawable_ID = tile->drawable->id;
	tile_req.tile_num = tile->tile_num;
	tile_req.shadow = tile->shadow;
	if (!gp_tile_req_write (_writefd, &tile_req))
	{	gimp_quit();
	}
	TaskSwitchToWire();
	if (!wire_read_msg(_readfd, &msg))
	{	gimp_quit ();
	}
	if (msg.type != GP_TILE_DATA)
	{	g_message ("unexpected message[3]: %d %s\n", msg.type,Get_gp_name(msg.type));
		gimp_quit ();
	}
	tile_data = msg.data;
	if ((tile_data->drawable_ID != tile->drawable->id) ||
		(tile_data->tile_num != tile->tile_num) ||
		(tile_data->shadow != tile->shadow) ||
		(tile_data->width != tile->ewidth) ||
		(tile_data->height != tile->eheight) ||
		(tile_data->bpp != tile->bpp))
	{	g_message ("received tile info did not match computed tile info\n");
		gimp_quit ();
	}
#ifdef BUILD_SHM
	if (tile_data->use_shm)
	{	tile->data = g_new (guchar, tile->ewidth * tile->eheight * tile->bpp);
		memcpy (tile->data, _shm_addr, tile->ewidth * tile->eheight * tile->bpp);
	}
	else
#endif
	{	tile->data = tile_data->data;
		tile_data->data = NULL;
	}
	if(!gp_tile_ack_write(_writefd))
	{	gimp_quit ();
	}
	wire_destroy (&msg);
	TaskSwitchToWire();
	d_assert(1==tile->is_allocated);
}

static void lib_tile_cache_detach(GTile* tile)
{	LTC_Initialize();
	if(!DL_is_used_node(&ltc.dl_list,(DL_node*)tile))
	{	return;
	}
	d_printf("<ltc:0x%x>",tile);
	DL_remove(&ltc.dl_list,(DL_node*) tile);
	d_assert(1==tile->is_allocated);
	ltc.cur_cache_size -= ltc.max_tile_size;
/*	lib_tile_unref(tile,FALSE);*/
}

void lib_tile_cache_purge(GTile* tile,int ntiles)
{	int i;
	for(i=0;i<ntiles;i++)
	{	lib_tile_cache_detach(tile);
		lib_tile_flush(tile);
		g_free(tile->data);
		tile->data = NULL;
		tile++;
}	}

guint get_lib_tile_width(void)
{	return lib_tile_width;
}

guint get_lib_tile_height(void)
{	return lib_tile_height;
}

