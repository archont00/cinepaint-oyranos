/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __PLUG_IN_H__
#define __PLUG_IN_H__

#include <sys/types.h>
#include "gtk/gtk.h"
#include "procedural_db.h"
#include "app/tag.h"
#include "../lib/wire/wirebuffer.h"
#include "../lib/wire/c_typedefs.h"

#ifdef WIN32
#include <time.h>
#else

#endif

#define RGB_IMAGE       0x01
#define GRAY_IMAGE      0x02
#define INDEXED_IMAGE   0x04
#define RGBA_IMAGE      0x08
#define GRAYA_IMAGE     0x10
#define INDEXEDA_IMAGE  0x20
#define U16_RGB_IMAGE       0x40
#define U16_GRAY_IMAGE      0x80
#define U16_RGBA_IMAGE      0x100 
#define U16_GRAYA_IMAGE     0x200
#define FLOAT_RGB_IMAGE       0x400
#define FLOAT_GRAY_IMAGE      0x800
#define FLOAT_RGBA_IMAGE      0x1000 
#define FLOAT_GRAYA_IMAGE     0x2000
#define FLOAT16_RGB_IMAGE       0x4000
#define FLOAT16_GRAY_IMAGE      0x8000
#define FLOAT16_RGBA_IMAGE      0x10000 
#define FLOAT16_GRAYA_IMAGE     0x20000
#define BFP_RGB_IMAGE       0x40000
#define BFP_GRAY_IMAGE      0x80000
#define BFP_RGBA_IMAGE      0x100000 
#define BFP_GRAYA_IMAGE     0x200000

typedef struct PlugInDef PlugInDef;
typedef struct PlugInProcDef PlugInProcDef;

#define PLUG_IN_ARGS 7

struct PlugIn
{
  unsigned int open : 1;                 /* Is the plug-in open */
  unsigned int destroy : 1;              /* Should the plug-in by destroyed */
  unsigned int query : 1;                /* Are we querying the plug-in? */
  unsigned int synchronous : 1;          /* Is the plug-in running synchronously or not */
  unsigned int recurse : 1;              /* Have we called 'gtk_main' recursively? */
  unsigned int busy : 1;                 /* Is the plug-in busy with a temp proc? */
#ifdef WIN32
  HINSTANCE handle;
#else
  pid_t pid;                             /* Plug-ins process id */
#endif
  char *args[PLUG_IN_ARGS];              /* Plug-ins command line arguments */

#ifdef WIN32
  PluginMain plugin_main;
  GPlugInInfo* PLUG_IN_INFO;
  WireBuffer* wire_buffer;
#else
  int my_read, my_write;                 /* Apps read and write file descriptors */
  int his_read, his_write;               /* Plug-ins read and write file descriptors */

  guint32 input_id;                      /* Id of input proc */
  char write_buffer[WIRE_BUFFER_SIZE];  /* Buffer for writing */
  int write_buffer_index;                /* Buffer index for writing */
#endif
/*  GSList *temp_proc_defs; */               /* Temporary procedures  */
  GtkWidget *progress;                   /* Progress dialog */
  GtkWidget *progress_label;
  GtkWidget *progress_bar;

  gpointer user_data;                    /* Handle for hanging data onto */

};

typedef struct PlugIn Plugin;

struct PlugInDef
{
  char *prog;
  GSList *proc_defs;
  time_t mtime;
  int query;
};

struct PlugInProcDef
{
  char *prog;
  char *menu_path;
  char *accelerator;
  char *extensions;
  char *prefixes;
  char *magics;
  char *image_types;
  int   image_types_val;
  ProcRecord db_info;
  GSList *extensions_list;
  GSList *prefixes_list;
  GSList *magics_list;
#ifdef WIN32
  PluginMain plugin_main;
#endif

};

void plug_in_handle_message(WireMessage* msg);

/* Initialize the plug-ins */
void plug_in_init (void);
#ifdef WIN32
DWORD WINAPI plug_in_init_thread(void*);
#endif
/* Kill all running plug-ins */
void plug_in_kill (void);

/* Add a plug-in to the list of valid plug-ins
 *  and query the plug-in for information if
 *  necessary.
 */
void plug_in_add (char *name,
		  char *menu_path,
		  char *accelerator);

/* Get the "image_types" the plug-in works on.
 */
char* plug_in_image_types (char *name);

/* Add in the file load/save handler fields procedure.
 */
struct PlugInProcDef* plug_in_file_handler (char *name,
				     char *extensions,
				     char *prefixes,
				     char *magics);

/* Add a plug-in definition.
 */
void plug_in_def_add (struct PlugInDef *plug_in_def);

/* Retrieve a plug-ins menu path
 */
char* plug_in_menu_path (char *name);

/* Create a new plug-in structure */
struct PlugIn* plug_in_new (char *name);

/* Destroy a plug-in structure. This will close the plug-in
 *  first if necessary.
 */
void plug_in_destroy (struct PlugIn *plug_in);

/* Open a plug-in. This cause the plug-in to run.
 * If returns 1, you must destroy the plugin.  If returns 0 you
 * may not destroy the plugin.
 */
int plug_in_open (struct PlugIn *plug_in);

/* Close a plug-in. This kills the plug-in and releases
 *  its resources.
 */
void plug_in_close (struct PlugIn *plug_in,
		    int     kill_it);

/* Run a plug-in as if it were a procedure database procedure */
Argument* plug_in_run (ProcRecord *proc_rec,
		       Argument   *args,
		       int         synchronous,
		       int         destroy_values);

/* Run the last plug-in again with the same arguments. Extensions
 *  are exempt from this "privelege".
 */
void plug_in_repeat (int with_interface);

/* Set the sensitivity for plug-in menu items based on the image
 *  base type.
 */
void plug_in_set_menu_sensitivity (Tag t);

/* Register an internal plug-in.  This is for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void plug_in_add_internal (struct PlugInProcDef* proc_def);
GSList* plug_in_extensions_parse  (char     *extensions);
int     plug_in_image_types_parse (char     *image_types);

gint  tag_to_plugin_image_type ( Tag t );
void  plug_in_proc_def_print (struct PlugInProcDef * proc); 

#endif /* __PLUG_IN_H__ */
