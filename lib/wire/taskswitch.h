/* taskswitch.h
// Task switches between threads when running plug-ins
// Copyright March 5, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef TASK_SWITCH_H
#define TASK_SWITCH_H

#include "../lib/dll_api.h"

void TaskSwitchToWire();
DLL_API void TaskSwitchToPlugin();

#endif

