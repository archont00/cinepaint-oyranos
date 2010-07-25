/* wirebuffer.h
// In-memory buffer used instead of pipe to plug-ins
// Copyright Mar 6, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef WIRE_BUFFER_H
#define WIRE_BUFFER_H

#include "wire.h"
#include "../lib/dll_api.h"

/* Tile buffer uses 64*64 tiles:
	8-bit: 12k
	16-bit: 24k */

#ifdef WIN32
#define WIRE_BUFFER_SIZE  2*64*1024
#else
#define WIRE_BUFFER_SIZE 1024
#endif

struct WireBuffer
{	guint8 buffer[WIRE_BUFFER_SIZE];
	guint buffer_write_index;
	guint buffer_read_index;
	guint buffer_count;
	struct PlugIn* plugin;
#ifdef WIN32
	HANDLE thread_wire;
	HANDLE thread_plugin;
	Event event_wire;
	Event event_plugin;
#endif
/* old stuff: */
	GSList *plug_in_defs;
	GSList *gimprc_proc_defs;
	GSList *proc_defs;
	GSList *open_plug_ins;
	GSList *blocked_plug_ins;

	GSList *plug_in_stack;
	struct PlugIn *current_plug_in;
	int current_readfd;
	int current_writefd;
	int current_buffer_index;
	char *current_buffer;
	Argument *current_return_vals;
	int current_return_nvals;

	ProcRecord *last_plug_in;

	int shm_ID;
	guchar *shm_addr;

	int write_pluginrc;
};

DLL_API int WireBufferOpen(WireBuffer* wb);
void WireBufferClose(WireBuffer* wb);


extern DLL_API WireBuffer* wire_buffer;

#endif
