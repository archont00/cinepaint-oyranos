/* plug-ins/plugin_loader.c
// Loads plug-in dll in same process thread
// Copyright Dec 28, 2002, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifdef WIN32

#include <stdio.h>
#include "../lib/wire/wire.h"
#include "../lib/wire/wirebuffer.h"
#include "../lib/wire/taskswitch.h"
#include "../lib/wire/protocol.h"
#include "../lib/wire/enums.h"
#include "plugin_loader.h"

PluginMain GetPluginMain(const char* filename)
{	HINSTANCE handle;
	handle=LoadLibrary(filename); 
	if(!handle) 
	{	d_printf("ERROR: couldn't LoadLibrary %s\n",filename);
		return 0;
	}
	return (PluginMain) GetProcAddress(handle,"plugin_main"); 
}

DWORD WINAPI WireMsgHandlerThread(WireBuffer* wire_buffer)
{	WireMessage msg;
/* Already checked wire_buffer != 0 */
/* ?	plug_in_push (plugin);*/
	memset (&msg, 0, sizeof (WireMessage));
	EventWaitFor(wire_buffer->event_wire);
	d_puts("<TaskSwitchToWire>");
	for(;;)
	{	if(!wire_read_msg(0,&msg))
		{	wire_buffer->plugin=0;
			puts("WIRE: WireMsgHandlerThread wire_read_msg failed");
			return 1;
		}
		d_puts("WIRE: handle message");
		/* Set breakpoint here to trace into messages received from plug-in */
		plug_in_handle_message(&msg);
		TaskSwitchToPlugin();
}	}

int WireMsgHandlerRun(WireBuffer* wire_buffer)
{	DWORD thread_id;
	if(!wire_buffer)
	{	d_printf("WireMsgHandlerRun invalid arguments!\n");
		return 0;
	}
	thread_id=0;
	wire_buffer->thread_wire=CreateThread(0,0,WireMsgHandlerThread,wire_buffer,0,&thread_id);
	d_printf("CreatedThread wire #%i\n",wire_buffer->thread_wire);
	return 0!=wire_buffer->thread_wire;
}

int PlugInLoader(PlugIn* plugin)
{	PluginMain plugin_main;
	const char* filename=plugin->args[0];
	if(!plugin || !wire_buffer)
	{	e_puts("ERROR: PlugInLoader");
		return 0;
	}
	plugin_main=GetPluginMain(filename);
	if(!plugin_main)
	{	e_puts("ERROR: plugin_main=<null>");
		return 0;
	}
	plugin->wire_buffer=wire_buffer;
	/* Set breakpoint here to trace into plug-in at program startup */
	plugin_main(plugin); 
	return 1;
}

/*
DWORD WINAPI ThreadPlugInRun(PlugIn* plugin)
{	return 0==plugin_main(plugin);
}*/

int PlugInRun(PlugIn* plugin)
{	DWORD thread_id;
	WireBuffer* wire_buffer;
	PluginMain plugin_main;
	if(!plugin ||!plugin->wire_buffer)
	{	d_printf("ERROR: PlugInRun invalid args!\n");
		return 0;
	}
	thread_id=0;
	wire_buffer=plugin->wire_buffer;
	if(wire_buffer->thread_plugin)
	{	d_printf("ERROR: Another plugin is not done!\n");
		return 0;
	}
	plugin_main=plugin->plugin_main;
	if(!plugin_main)
	{	plugin_main=GetPluginMain(plugin->args[0]);
	}
	if(!plugin_main)
	{	return 0;
	}
	plugin->plugin_main=plugin_main;
	/* Set breakpoint here to trace into plug-in at run-time --rsr */
	return 0==plugin_main(plugin);/* plugin_main returns errorlevel! */
/*	CreateThread(0,0,plugin_main,plugin,0,&thread_id);
*/	
}

void GetWireBuffer(WireMessage* msg)
{
#ifdef _DEBUG
	WireBuffer* wb=wire_buffer;
#endif
	d_puts("GetWireBuffer()");
	msg->type=GP_ERROR;
	if(wire_buffer->buffer_count)
	{	if(!wire_read_msg(0,msg))
		{	wire_buffer->plugin=0;
			e_puts("WireMsgHandlerThread wire_read_msg failed");
			msg->type=GP_ERROR;
}	}	}


/*
		if (wire_buffer->current_return_vals && wire_buffer->current_return_nvals == nargs)
		
		if(!wire_read_msg(0,&msg))
		{	puts("ERROR: plug-in recurse 1");
			return 0;
		}
		if(GP_PROC_RETURN!=msg.type)
		{	puts("ERROR: plug-in recurse 2");
			return 0;
		}
	     plug_in_handle_proc_return (msg.data);
*/

#else

/* Linux */

#endif


