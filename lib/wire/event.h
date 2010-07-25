/* plug-ins/event.h
   Thread events handling
   Copyright Feb 10, 2000, Robin.Rowe@MovieEditor.com
   License MIT (http://opensource.org/licenses/mit-license.php) */

#ifndef EVENT_H
#define EVENT_H

#ifdef WIN32

#include <windows.h>
#include "../lib/dll_api.h"

typedef HANDLE Event;

Event EventOpen();
void EventClose(Event event);
DLL_API void EventWaitFor(Event event);
int EventSignal(Event event);
int EventReset(Event event);

#else

/* Rowe: bug, not implemented, *nix to do!
   currently using old fork/pipe/shmget architecture here */

#endif

#endif

