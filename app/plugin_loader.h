// plug-ins/plugin_loader.h
// Launches plug-in as a thread
// Copyrith Dec 28, 2002, by Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#ifdef WIN32

#include <windows.h>
#include "../app/plug_in.h"
#include "../lib/wire/wire.h"
#include "../lib/wire/c_typedefs.h"

int PlugInLoader(PlugIn* plugin);
DWORD WINAPI PlugInRunThread(PlugIn* plugin);
int PlugInRun(PlugIn* plugin);
int WireMsgHandlerRun(WireBuffer* wire_buffer);
void GetWireBuffer(WireMessage* msg);

typedef int (*Plugin_reader)(int fd, guint8 *buf, gulong count);
typedef int (*PluginMain)(void* data);

#else

/* future *nix dlopen stuff */

#endif

#endif

