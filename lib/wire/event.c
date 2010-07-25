/* plug-ins/event.c
   Thread events handling
   Copyright Feb 10, 2000, Robin.Rowe@MovieEditor.com
   License MIT (http://opensource.org/licenses/mit-license.php) */

#include "event.h"

#ifdef WIN32

Event EventOpen()
{/*	if(INVALID_HANDLE_VALUE!=event)
	{	return INVALID_HANDLE_VALUE;
	}*/
	return CreateEvent(NULL,FALSE,FALSE,NULL);
}

void EventClose(Event event)
{	if(INVALID_HANDLE_VALUE!=event)
	{	CloseHandle(event);
		/*event=INVALID_HANDLE_VALUE*/
}	}

void EventWaitFor(Event event)
{	if(INVALID_HANDLE_VALUE!=event)
	{	WaitForSingleObject(event,INFINITE);
/*		ResetEvent(event);*/
}	}

int EventSignal(Event event)
{	if(INVALID_HANDLE_VALUE!=event)
	{	return SetEvent(event);
	}
	return 0;
}

int EventReset(Event event)
{	if(INVALID_HANDLE_VALUE==event)
	{	return 0;
	}
	return ResetEvent(event);
}

#else

/* Linux TO-DO --rsr */

#endif