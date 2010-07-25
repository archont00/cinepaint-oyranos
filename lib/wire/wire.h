/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMP_WIRE_H__
#define __GIMP_WIRE_H__


#include <glib.h>
#ifdef WIN32
#include <windows.h>
#include "event.h"
#endif
#include "../lib/dll_api.h"
#include "../app/procedural_db.h"
#include "c_typedefs.h"

typedef void (* WireReadFunc)    (int fd, WireMessage *msg);
typedef void (* WireWriteFunc)   (int fd, WireMessage *msg);
typedef void (* WireDestroyFunc) (WireMessage *msg);
typedef int  (* WireIOFunc)      (int fd, guint8 *buf, gulong count);
typedef int  (* WireFlushFunc)   (int fd);

const char* Get_gp_name(int param);

struct WireMessage
{	guint32 type;
	gpointer data;
};

void wire_register     (guint32          type,
		        WireReadFunc     read_func,
		        WireWriteFunc    write_func,
		        WireDestroyFunc  destroy_func);
DLL_API void wire_set_reader   (WireIOFunc       read_func);
DLL_API void wire_set_writer   (WireIOFunc       write_func);
DLL_API void wire_set_flusher  (WireFlushFunc    flush_func);
int  wire_read         (int              fd,
			guint8          *buf,
			gulong           count);
int  wire_write        (int              fd,
			guint8          *buf,
			gulong           count);
DLL_API int  wire_flush        (int              fd);
int  wire_error        (void);
DLL_API void wire_clear_error  (void);
DLL_API int  wire_read_msg     (int              fd,
		        WireMessage     *msg);


void wire_get_buffer(PlugIn* plugin);
	 

int  wire_write_msg    (int              fd,
		        WireMessage     *msg);
DLL_API void wire_destroy      (WireMessage     *msg);
int  wire_read_int32   (int              fd,
		        guint32         *data,
		        gint             count);
int  wire_read_int16   (int              fd,
		        guint16         *data,
		        gint             count);
int  wire_read_int8    (int              fd,
		        guint8          *data,
		        gint             count);
int  wire_read_double  (int              fd,
		        gdouble         *data,
		        gint             count);
int  wire_read_string  (int              fd,
			gchar          **data,
			gint             count);
int  wire_write_int32  (int              fd,
		        guint32         *data,
		        gint             count);
int  wire_write_int16  (int              fd,
		        guint16         *data,
		        gint             count);
int  wire_write_int8   (int              fd,
		        guint8          *data,
		        gint             count);
int  wire_write_double (int              fd,
			gdouble         *data,
			gint             count);
int  wire_write_string (int              fd,
			gchar          **data,
			gint             count);

#ifdef WIN32
DLL_API int wire_memory_read(int fd, guint8 *buf, gulong count);
DLL_API int wire_memory_write(int fd,guint8 *buf,gulong count);
DLL_API int wire_memory_flush(int fd);
#else
int wire_file_read(int fd, guint8 *buf, gulong count);
int wire_file_write(int fd, guint8 *buf, gulong count);
int wire_file_flush(int fd);
#endif

#endif /* __GIMP_WIRE_H__ */
