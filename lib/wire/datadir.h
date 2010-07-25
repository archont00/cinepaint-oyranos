/* wire/datadir.h 
// Fixes odd linker bug, variable called in libgimp, but in plug-in
// Copyright Nov 10, 2002, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef DATADIR_H
#define DATADIR_H

#ifdef WIN32
#include "../win32/unistd.h"
#endif

#include "dll_api.h"


DLL_API const char* GetDirDot();
DLL_API const char* GetDirHome();
DLL_API const char* GetDirData();
DLL_API const char* GetDirPrefix();
DLL_API const char* GetDirStaticPrefix();
DLL_API void        SetDirPrefix(const char* prefix);
DLL_API const char* GetDirPluginSuffix();
DLL_API const char* GetDirDataSuffix();
DLL_API const char* GetDirSeparator();
DLL_API char*       GetDirAbsoluteExec(const char *filename);
DLL_API int         IsDir(const char *dirname);

/* a function for obtaining an valid path with an given rcfile name */
const char* gimp_personal_rc_file (const char* rcfile);

/*#else

#define DATA_DIR "share"
#define GetDirData() DATA_DIR
#define GetDirDot() ".cinepaint"
 getenv("GIMP_DIRECTORY") 
#define GetDirHome() getenv("HOME")

#endif
*/

/*
gimpdatadir = ${prefix}/share/filmgimp/0.7
gimpdir = .filmgimp
gimpplugindir = ${exec_prefix}/bin
gimpsysconfdir = 

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

*/

#endif

