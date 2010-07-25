/* wire/taskswitch.c
// Task switches between threads when running plug-ins
// Copyright March 5, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#include <stdio.h>
#include "wire.h"
#include "taskswitch.h"
#include "wirebuffer.h"
#include "iodebug.h"

void TaskSwitchToWire()
{
#ifdef WIN32
	if(!wire_buffer || !wire_buffer->plugin)
	{	e_printf("Thread wire_buffer->plugin error!");
		return;
	}
	d_puts("<Sleep Plugin>");
	EventSignal(wire_buffer->event_wire);
	EventWaitFor(wire_buffer->event_plugin);
	d_puts("<TaskSwitchToPlugin>");
#endif
}

void TaskSwitchToPlugin()
{
#ifdef WIN32
	if(!wire_buffer || !wire_buffer->plugin)
	{	e_printf("Thread wire_buffer->plugin error!");
		return;
	}
	d_puts("<Sleep Wire>");
	EventSignal(wire_buffer->event_plugin);
	EventWaitFor(wire_buffer->event_wire);
	d_puts("<TaskSwitchToWire>");
#endif
}

