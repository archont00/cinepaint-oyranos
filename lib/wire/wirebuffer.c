/* wirebuffer.c
// In-memory buffer used instead of pipe to plug-ins
// Copyright Mar 6, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#include <stdio.h>
#include "wire.h"
#include "wirebuffer.h"
#include "iodebug.h"

static WireBuffer wb;
WireBuffer* wire_buffer=&wb;

/* These Wire functions have to go here or plugins won't link! */

void WireBufferReset(WireBuffer* wb)
{	memset(wb,0,sizeof(WireBuffer));
#ifdef WIN32
	wb->thread_wire = 0;
	wb->thread_plugin = 0;
	wb->event_wire = INVALID_HANDLE_VALUE;
	wb->event_plugin = INVALID_HANDLE_VALUE;
#endif
/* gimp.c */
	wb->buffer[0] = 0;
	wb->buffer_write_index = 0;
	wb->buffer_read_index = 0;
	wb->buffer_count = 0;
	wb->plugin = 0;
/* plugin.c */
	wb->plug_in_defs = NULL;
	wb->gimprc_proc_defs = NULL;
	wb->proc_defs = NULL;
	wb->open_plug_ins = NULL;
	wb->blocked_plug_ins = NULL;

	wb->plug_in_stack = NULL;
	wb->current_plug_in = NULL;
	wb->current_readfd = 0;
	wb->current_writefd = 0;
	wb->current_buffer_index = 0;
	wb->current_buffer = NULL;
	wb->current_return_vals = NULL;
	wb->current_return_nvals = 0;

	wb->last_plug_in = NULL;

	wb->shm_ID = -1;
	wb->shm_addr = NULL;

	wb->write_pluginrc = FALSE;
}

int WireBufferOpen(WireBuffer* wb)
{	WireBufferReset(wb);
#ifdef WIN32
	wb->event_wire=EventOpen();
	if(INVALID_HANDLE_VALUE==wb->event_wire)
	{	e_puts("ERROR: EventOpen event_wire");
		return FALSE;
	}
	wb->event_plugin=EventOpen();
	if(INVALID_HANDLE_VALUE==wb->event_plugin)
	{	e_puts("ERROR: EventOpen event_plugin");
		return FALSE;
	}
#endif
	return TRUE;
}

void WireBufferClose(WireBuffer* wb)
{
#ifdef WIN32
	EventClose(wb->event_wire);
	EventClose(wb->event_plugin);
	/* kill thread ... */
#endif
	WireBufferReset(wb);
}


