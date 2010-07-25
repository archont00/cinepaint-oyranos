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
#include "config.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef WIN32
#include "extra.h"
#define index strchr
#else
#define index strchr
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#endif

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef BUILD_SHM
#include <sys/shm.h>
extern int use_shm;
#endif

#include "../lib/wire/protocol.h"
#include "../lib/wire/wire.h"
#include "../../lib/wire/enums.h"


#include "../app_procs.h"
#include "../appenv.h"
#include "../canvas.h"
#include "../drawable.h"
#include "../datafiles.h"
#include "../errors.h"
#include "../gdisplay.h"
#include "../general.h"
#include "../gimage.h"
#include "../rc.h"
#include "../interface.h"
#include "../menus.h"
#include "../pixelarea.h"
#include "../plug_in.h"
#include "../tag.h"
#include "../layout.h"
#include "../minimize.h"
#include "../plugin_loader.h"
#include "../lib/wire/wirebuffer.h"
#include "../lib/wire/taskswitch.h"
#include "../lib/wire/event.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "../regex_fake.h"
#include "float16.h"

#define FIXME
#define TILE_WIDTH 64
#define TILE_HEIGHT 64
#ifndef WIN32
/* rsr: can't run sep progress bar -- event starvation */
#define SEPARATE_PROGRESS_BAR
#endif
typedef struct PlugInBlocked  PlugInBlocked;

/*typedef enum
{
  RUN_INTERACTIVE    = 0x0,
  RUN_NONINTERACTIVE = 0x1,
  RUN_WITH_LAST_VALS = 0x2
} RunModeType;*/

struct PlugInBlocked
{
  PlugIn *plug_in;
  char *proc_name;
};

#define COPY_AREA_TO_BUFFER 0
#define COPY_BUFFER_TO_AREA 1

static void plug_in_copyarea             (PixelArea         *a, 
					   guchar            *buf, 
					   gint               direction);
static void plug_in_push                  (PlugIn            *plug_in);
static void plug_in_pop                   (void);
static void plug_in_recv_message          (gpointer           data,
				           gint               id,
				           GdkInputCondition  cond);

static void plug_in_handle_quit           (void);
static void plug_in_handle_tile_req       (GPTileReq         *tile_req);
static void plug_in_handle_proc_run       (GPProcRun         *proc_run);
static void plug_in_handle_proc_return    (GPProcReturn      *proc_return);

static void plug_in_handle_proc_install   (GPProcInstall     *proc_install);
static void plug_in_handle_proc_uninstall (GPProcUninstall   *proc_uninstall);
static void plug_in_write_rc              (char              *filename);
static void plug_in_init_file             (char              *filename);
static void plug_in_query                 (char              *filename,
				           PlugInDef         *plug_in_def);
static void plug_in_add_to_db             (void);
static void plug_in_make_menu             (void);
void plug_in_make_image_menu              (GtkItemFactory *fac, char *buf);
static void plug_in_callback              (GtkWidget         *widget,
					   gpointer           client_data);
static void plug_in_proc_def_insert       (PlugInProcDef     *proc_def);
static void plug_in_proc_def_remove       (PlugInProcDef     *proc_def);
static void plug_in_proc_def_destroy      (PlugInProcDef     *proc_def,
					   int                data_only);

static Argument* plug_in_temp_run       (ProcRecord *proc_rec,
  					 Argument   *args);
static Argument* plug_in_params_to_args (GPParam   *params,
  					 int        nparams,
  					 int        full_copy);
static GPParam*  plug_in_args_to_params (Argument  *args,
  					 int        nargs,
  					 int        full_copy);
static void      plug_in_params_destroy (GPParam   *params,
  					 int        nparams,
  					 int        full_destroy);
static void      plug_in_args_destroy   (Argument  *args,
  					 int        nargs,
  					 int        full_destroy);

static Argument* progress_init_invoker   (Argument *args);
static Argument* progress_update_invoker (Argument *args);

static Argument* message_invoker         (Argument *args);

static Argument* message_handler_get_invoker (Argument *args);
static Argument* message_handler_set_invoker (Argument *args);

static ProcArg progress_init_args[] =
{
  { PDB_STRING,
    "message",
    "Message to use in the progress dialog." }
};

static ProcRecord progress_init_proc =
{
  "gimp_progress_init",
  "Initializes the progress bar for the current plug-in",
  "Initializes the progress bar for the current plug-in. It is only valid to call this procedure from a plug-in.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  progress_init_args,
  0,
  NULL,
  { { progress_init_invoker } },
};

static ProcArg progress_update_args[] =
{
  { PDB_FLOAT,
    "percentage",
    "Percentage of progress completed" }
};

static ProcRecord progress_update_proc =
{
  "gimp_progress_update",
  "Updates the progress bar for the current plug-in",
  "Updates the progress bar for the current plug-in. It is only valid to call this procedure from a plug-in.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  progress_update_args,
  0,
  NULL,
  { { progress_update_invoker } },
};


static ProcArg message_args[] =
{
  { PDB_STRING,
    "message",
    "Message to display in the dialog." }
};

static ProcRecord message_proc =
{
  "gimp_message",
  "Displays a dialog box with a message",
  "Displays a dialog box with a message. Useful for status or error reporting.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  1,
  message_args,
  0,
  NULL,
  { { message_invoker } },
};


static ProcArg message_handler_get_out_args[] =
{
  { PDB_INT32,
    "handler",
    "the current handler type: { MESSAGE_BOX (0), CONSOLE (1) }" }
};

static ProcRecord message_handler_get_proc =
{
  "gimp_message_handler_get",
  "Returns the current state of where warning messages are displayed.",
  "This procedure returns the way g_message warnings are displayed. They can be shown in a dialog box or printed on the console where gimp was started.",
  "Manish Singh",
  "Manish Singh",
  "1998",
  PDB_INTERNAL,
  0,
  NULL,
  1,
  message_handler_get_out_args,
  { { message_handler_get_invoker } },
};

static ProcArg message_handler_set_args[] =
{
  { PDB_INT32,
    "handler",
    "the new handler type: { MESSAGE_BOX (0), CONSOLE (1) }" }
};

static ProcRecord message_handler_set_proc =
{
  "gimp_message_handler_set",
  "Controls where warning messages are displayed.",
  "This procedure controls how g_message warnings are displayed. They can be shown in a dialog box or printed on the console where gimp was started.",
  "Manish Singh",
  "Manish Singh",
  "1998",
  PDB_INTERNAL,
  1,
  message_handler_set_args,
  0,
  NULL,
  { { message_handler_set_invoker } },
};


static int
match_strings (regex_t *preg,
               gchar   *a)
{
  return regexec (preg, a, 0, NULL, 0);
}

static ProcRecord temp_PDB_name_proc;
static ProcRecord plugins_query_proc;
static ProcRecord plugin_domain_register_proc;
static ProcRecord plugin_help_register_proc;

static Argument *
temp_PDB_name_invoker (Argument *args)
{
  Argument *return_args;
  gchar *temp_name;
  static gint proc_number = 0;

  temp_name = g_strdup_printf ("temp_plugin_number_%d", proc_number++);

  return_args = procedural_db_return_args (&temp_PDB_name_proc, TRUE);
  return_args[1].value.pdb_pointer = temp_name;

  return return_args;
}

static ProcArg temp_PDB_name_outargs[] =
{
  {
    PDB_STRING,
    "temp_name",
    "A unique temporary name for a temporary PDB entry"
  }
};

static ProcRecord temp_PDB_name_proc =
{
  "gimp_temp_PDB_name",
  "Generates a unique temporary PDB name.",
  "This procedure generates a temporary PDB entry name that is guaranteed to be unique. It is many used by the interactive popup dialogs to generate a PDB entry name.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,
  0,
  NULL,
  1,
  temp_PDB_name_outargs,
  { { temp_PDB_name_invoker } }
};

static Argument *
plugins_query_invoker (Argument *args)
{
#ifdef REGEX_FAKE_H
	return 0;
#else
  Argument *return_args;
  gchar *search_str;
  gint32 num_plugins = 0;
  gchar **menu_strs;
  gchar **accel_strs;
  gchar **prog_strs;
  gchar **types_strs;
  gint32 *time_ints;
  gchar **realname_strs;
  PlugInProcDef *proc_def;
  GSList *tmp = NULL;
  gint i = 0;
  regex_t sregex;

  search_str = (gchar *) args[0].value.pdb_pointer;

  if (search_str && strlen (search_str))
    regcomp (&sregex, search_str, REG_ICASE);
  else
    search_str = NULL;

  /* count number of plugin entries, then allocate 4 arrays of correct size
   * where we can store the strings.
   */

  tmp = proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && proc_def->menu_path)
	{
	  gchar *name = strrchr (proc_def->menu_path, '/');

	  if (name)
	    name = name + 1;
	  else
	    name = proc_def->menu_path;

	  if (search_str && match_strings (&sregex, name))
	    continue;

	  num_plugins++;
	}
    }

  menu_strs     = g_new (gchar *, num_plugins);
  accel_strs    = g_new (gchar *, num_plugins);
  prog_strs     = g_new (gchar *, num_plugins);
  types_strs    = g_new (gchar *, num_plugins);
  realname_strs = g_new (gchar *, num_plugins);
  time_ints     = g_new (gint   , num_plugins);

  tmp = proc_defs;
  while (tmp)
    {
      if (i > num_plugins)
	g_error ("Internal error counting plugins");

      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && proc_def->menu_path)
	{
	  ProcRecord *pr = &proc_def->db_info;

	  gchar *name = strrchr (proc_def->menu_path, '/');

	  if (name)
	    name = name + 1;
	  else
	    name = proc_def->menu_path;

	  if (search_str && match_strings(&sregex,name))
	    continue;

	  menu_strs[i]     = g_strdup (proc_def->menu_path);
	  accel_strs[i]    = g_strdup (proc_def->accelerator);
	  prog_strs[i]     = g_strdup (proc_def->prog);
	  types_strs[i]    = g_strdup (proc_def->image_types);
	  realname_strs[i] = g_strdup (pr->name);
#if 0
	  time_ints[i]     = proc_def->mtime;
#endif

	  i++;
	}
    }

  /* This I hope frees up internal stuff */
  if (search_str)
    free (sregex.buffer);

  return_args = procedural_db_return_args (&plugins_query_proc, TRUE);

  return_args[1].value.pdb_int = num_plugins;
  return_args[2].value.pdb_pointer = menu_strs;
  return_args[3].value.pdb_int = num_plugins;
  return_args[4].value.pdb_pointer = accel_strs;
  return_args[5].value.pdb_int = num_plugins;
  return_args[6].value.pdb_pointer = prog_strs;
  return_args[7].value.pdb_int = num_plugins;
  return_args[8].value.pdb_pointer = types_strs;
  return_args[9].value.pdb_int = num_plugins;
  return_args[10].value.pdb_pointer = time_ints;
  return_args[11].value.pdb_int = num_plugins;
  return_args[12].value.pdb_pointer = realname_strs;

  return return_args;
#endif
}

static ProcArg plugins_query_inargs[] =
{
  {
    PDB_STRING,
    "search_string",
    "If not an empty string then use this as a search pattern"
  }
};

static ProcArg plugins_query_outargs[] =
{
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_STRINGARRAY,
    "menu_path",
    "The menu path of the plugin"
  },
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_STRINGARRAY,
    "plugin_accelerator",
    "String representing keyboard accelerator (could be empty string)"
  },
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_STRINGARRAY,
    "plugin_location",
    "Location of the plugin program"
  },
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_STRINGARRAY,
    "plugin_image_type",
    "Type of image that this plugin will work on"
  },
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_INT32ARRAY,
    "plugin_install_time",
    "Time that the plugin was installed"
  },
  {
    PDB_INT32,
    "num_plugins",
    "The number of plugins"
  },
  {
    PDB_STRINGARRAY,
    "plugin_real_name",
    "The internal name of the plugin"
  }
};

static ProcRecord plugins_query_proc =
{
  "gimp_plugins_query",
  "Queries the plugin database for its contents.",
  "This procedure queries the contents of the plugin database.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,
  1,
  plugins_query_inargs,
  12,
  plugins_query_outargs,
  { { plugins_query_invoker } }
};

static Argument *
plugin_domain_register_invoker (Argument *args)
{
  gboolean success = TRUE;
  gchar *domain_name;
  gchar *domain_path;
  PlugInDef *plug_in_def;

  domain_name = (gchar *) args[0].value.pdb_pointer;
  if (domain_name == NULL)
    success = FALSE;

  domain_path = (gchar *) args[1].value.pdb_pointer;

  if (success)
    {
      if (wire_buffer->current_plug_in && wire_buffer->current_plug_in->query)
	{
	  plug_in_def = wire_buffer->current_plug_in->user_data;
    
#if 0
	  if (plug_in_def->locale_domain)
	    g_free (plug_in_def->locale_domain);      
	  plug_in_def->locale_domain = g_strdup (domain_name);
	  if (plug_in_def->locale_path);
	    g_free (plug_in_def->locale_path);
	  plug_in_def->locale_path = domain_path ? g_strdup (domain_path) : NULL;
#endif    
	}
    }

  return procedural_db_return_args (&plugin_domain_register_proc, success);
}

static ProcArg plugin_domain_register_inargs[] =
{
  {
    PDB_STRING,
    "domain_name",
    "The name of the textdomain (must be unique)."
  },
  {
    PDB_STRING,
    "domain_path",
    "The absolute path to the compiled message catalog (may be NULL)."
  }
};

static ProcRecord plugin_domain_register_proc =
{
  "gimp_plugin_domain_register",
  "Registers a textdomain for localisation.",
  "This procedure adds a textdomain to the list of domains Gimp searches for strings when translating its menu entries. There is no need to call this function for plug-ins that have their strings included in the gimp-std-plugins domain as that is used by default. If the compiled message catalog is not in the standard location, you may specify an absolute path to another location. This procedure can only be called in the query function of a plug-in and it has to be called before any procedure is installed.",
  "Sven Neumann",
  "Sven Neumann",
  "2000",
  PDB_INTERNAL,
  2,
  plugin_domain_register_inargs,
  0,
  NULL,
  { { plugin_domain_register_invoker } }
};

static Argument *
plugin_help_register_invoker (Argument *args)
{
  gboolean success = TRUE;
  gchar *help_path;
  PlugInDef *plug_in_def;

  help_path = (gchar *) args[0].value.pdb_pointer;
  if (help_path == NULL)
    success = FALSE;

  if (success)
    {
      if (wire_buffer->current_plug_in && wire_buffer->current_plug_in->query)
	{
	  plug_in_def = wire_buffer->current_plug_in->user_data;
    
#if 0
	  if (plug_in_def->help_path)
	    g_free (plug_in_def->help_path);
	  plug_in_def->help_path = g_strdup (help_path);
#endif
	}
    }

  return procedural_db_return_args (&plugin_help_register_proc, success);
}

static ProcArg plugin_help_register_inargs[] =
{
  {
    PDB_STRING,
    "help_path",
    "The rootdir of the plug-in's help pages"
  }
};

static ProcRecord plugin_help_register_proc =
{
  "gimp_plugin_help_register",
  "Register a help path for a plug-in.",
  "This procedure changes the help rootdir for the plug-in which calls it. All subsequent calls of gimp_help from this plug-in will be interpreted relative to this rootdir. This procedure can only be called in the query function of a plug-in and it has to be called before any procedure is installed.",
  "Michael Natterer <mitch@gimp.org>",
  "Michael Natterer <mitch@gimp.org>",
  "2000",
  PDB_INTERNAL,
  1,
  plugin_help_register_inargs,
  0,
  NULL,
  { { plugin_help_register_invoker } }
};



void
plug_in_init ()
{
  char filename[MAXPATHLEN];
  GSList *tmp, *tmp2;
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  unsigned nplugins, nth;

/*	WireBufferOpen(wire_buffer);*/
#ifdef WIN32
	if(!WireMsgHandlerRun(wire_buffer))
	{	e_printf("WireMsgHandlerRun failed!\n");
	}
#else
/* Rowe: bug, not implemented */
#endif

  /* initialize the progress init and update procedure db calls. */
  procedural_db_register (&progress_init_proc);
  procedural_db_register (&progress_update_proc);
  procedural_db_register (&temp_PDB_name_proc);
  procedural_db_register (&plugins_query_proc);
  procedural_db_register (&plugin_domain_register_proc);
  procedural_db_register (&plugin_help_register_proc);

  /* initialize the message box procedural db calls */
  procedural_db_register (&message_proc);
  procedural_db_register (&message_handler_get_proc);
  procedural_db_register (&message_handler_set_proc);

  /* initialize the gimp protocol library and set the read and
   *  write handlers.
   */
  gp_init ();
#ifdef WIN32
	wire_set_reader(wire_memory_read);
	wire_set_writer(wire_memory_write);
	wire_set_flusher(wire_memory_flush);
#else
  wire_set_reader(wire_file_read);
  wire_set_writer(wire_file_write);
  wire_set_flusher(wire_file_flush);
#endif

#ifdef BUILD_SHM
  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (use_shm)
    {
#define PLUG_IN_C_3_cw 
      shm_ID = shmget (IPC_PRIVATE, TILE_WIDTH * TILE_HEIGHT * TAG_MAX_BYTES, IPC_CREAT | 0777);
      if (shm_ID == -1)
	g_message (_("shmget failed...disabling shared memory tile transport\n"));
      else
	{
	  shm_addr = (guchar*) shmat (shm_ID, 0, 0);
	  if (shm_addr == (guchar*) -1)
	    {
	      g_message (_("shmat failed...disabling shared memory tile transport\n"));
	      shm_ID = -1;
	    }

/* I dont think this will work, as it makes the shmat of the plugin fail*/
# ifdef	IPC_RMID_DEFERRED_RELEASE
	  if (shm_addr != (guchar*) -1)
	    shmctl (shm_ID, IPC_RMID, 0);
# endif
	}
    }
#endif

  printf(_("Searching plug-ins in path: %s\n"),plug_in_path);
  datafiles_read_directories (plug_in_path, plug_in_init_file, MODE_EXECUTABLE);

  /* read the pluginrc file for cached data */
  if (pluginrc_path)
    {
      if (*pluginrc_path == '/')
        strcpy(filename, pluginrc_path);
      else
        sprintf(filename, "%s/%s", gimp_directory(), pluginrc_path);
    }
  else
    sprintf (filename, "%s/pluginrc", gimp_directory ());

  app_init_update_status(_("Resource configuration"), filename, -1);
  parse_gimprc_file (filename);
  /* query any plug-ins that have changed since we last wrote out
   *  the pluginrc file.
   */
  tmp = wire_buffer->plug_in_defs;
  app_init_update_status(_("Plug-ins"), "", 0);
  nplugins = g_slist_length(tmp);
  s_printf(_("plugin count = %i\n"),g_slist_length(wire_buffer->plug_in_defs));
  nth = 0;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      if (plug_in_def->query)
	{
	  wire_buffer->write_pluginrc = TRUE;
	  if ((be_verbose == TRUE) || (no_splash == TRUE))
	    g_print (_("query plug-in: \"%s\"\n"), plug_in_def->prog);
	  plug_in_query (plug_in_def->prog, plug_in_def);
	}
      app_init_update_status(NULL, plug_in_def->prog, nth/nplugins);
      nth++;
    }

  /* insert the proc defs */
  tmp = wire_buffer->gimprc_proc_defs;
  while (tmp)
    {
      proc_def = g_new (PlugInProcDef, 1);
      *proc_def = *((PlugInProcDef*) tmp->data);
      plug_in_proc_def_insert (proc_def);
      tmp = tmp->next;
    }

  tmp = wire_buffer->plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      tmp2 = plug_in_def->proc_defs;
      while (tmp2)
	{
	  proc_def = tmp2->data;
	  tmp2 = tmp2->next;

	  plug_in_proc_def_insert (proc_def);
	}
    }

  /* write the pluginrc file if necessary */
  if (wire_buffer->write_pluginrc)
    {
      if ((be_verbose == TRUE) || (no_splash == TRUE))
	g_print ("writing \"%s\"\n", filename);
      plug_in_write_rc (filename);
    }

  /* add the plug-in procs to the procedure database */
  plug_in_add_to_db ();

  /* make the menu */
  plug_in_make_menu ();

  /* run the available extensions */
  tmp = wire_buffer->proc_defs;
  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print ("Starting extensions: ");
  app_init_update_status(_("Extensions"), "", 0);
  nplugins = g_slist_length(tmp); nth = 0;

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog &&
	  (proc_def->db_info.num_args == 0) &&
	  (proc_def->db_info.proc_type == PDB_EXTENSION))
	{
	  if ((be_verbose == TRUE) || (no_splash == TRUE))
	    g_print ("%s ", proc_def->db_info.name);
	  app_init_update_status(NULL, proc_def->db_info.name,
				 nth/nplugins);

	  plug_in_run (&proc_def->db_info, NULL, FALSE, TRUE);
	}
    }
  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print ("\n");

  /* free up stuff */
  tmp = wire_buffer->plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      g_slist_free (plug_in_def->proc_defs);
      g_free (plug_in_def->prog);
      g_free (plug_in_def);
    }
  g_slist_free (wire_buffer->plug_in_defs);
#ifdef WIN32_EXPERIMENTAL
	return 1;
#endif
}

void
plug_in_kill ()
{
  GSList *tmp;
  PlugIn *plug_in;
 
#ifdef BUILD_SHM
# ifndef	IPC_RMID_DEFERRED_RELEASE
  if (shm_ID != -1)
    {
      shmdt ((char*) shm_addr);
      shmctl (shm_ID, IPC_RMID, 0);
    }
# else	/* IPC_RMID_DEFERRED_RELEASE */
  if (shm_ID != -1)
    shmdt ((char*) shm_addr);
# endif
#endif

  tmp = wire_buffer->open_plug_ins;
  while (tmp)
    {
      plug_in = tmp->data;
      tmp = tmp->next;
      plug_in_destroy (plug_in);
    }
}

void
plug_in_add (char *prog,
	     char *menu_path,
	     char *accelerator)
{
  PlugInProcDef *proc_def;
  GSList *tmp;

  if (strncmp ("plug_in_", prog, 8) != 0)
    {
      char *t = g_new (char, strlen (prog) + 9);
      sprintf (t, "plug_in_%s", prog);
      g_free (prog);
      prog = t;
    }

  tmp = wire_buffer->gimprc_proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, prog) == 0)
	{
	  if (proc_def->db_info.name)
	    g_free (proc_def->db_info.name);
	  if (proc_def->menu_path)
	    g_free (proc_def->menu_path);
	  if (proc_def->accelerator)
	    g_free (proc_def->accelerator);
	  if (proc_def->extensions)
	    g_free (proc_def->extensions);
	  if (proc_def->prefixes)
	    g_free (proc_def->prefixes);
	  if (proc_def->magics)
	    g_free (proc_def->magics);
	  if (proc_def->image_types)
	    g_free (proc_def->image_types);

	  proc_def->db_info.name = prog;
	  proc_def->menu_path = menu_path;
	  proc_def->accelerator = accelerator;
	  proc_def->prefixes = NULL;
	  proc_def->extensions = NULL;
	  proc_def->magics = NULL;
	  proc_def->image_types = NULL;
	  return;
	}
    }

  proc_def = g_new0 (PlugInProcDef, 1);
  proc_def->db_info.name = prog;
  proc_def->menu_path = menu_path;
  proc_def->accelerator = accelerator;

  wire_buffer->gimprc_proc_defs = g_slist_prepend (wire_buffer->gimprc_proc_defs, proc_def);
}

char*
plug_in_image_types (char *name)
{
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp;

  if (wire_buffer->current_plug_in)
    {
      plug_in_def = wire_buffer->current_plug_in->user_data;
      tmp = plug_in_def->proc_defs;
    }
  else
    {
      tmp = wire_buffer->proc_defs;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, name) == 0)
	return proc_def->image_types;
    }

  return NULL;
}

GSList*
plug_in_extensions_parse (char *extensions)
{
  GSList *list;
  char *extension;
  list = NULL;
  /* EXTENSIONS can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      extensions = g_strdup (extensions);
      extension = strtok (extensions, " \t,");
      while (extension)
	{
	  list = g_slist_prepend (list, g_strdup (extension));
	  extension = strtok (NULL, " \t,");
	}
      g_free (extensions);
    }

  return g_slist_reverse (list);
}

void
plug_in_add_internal (PlugInProcDef *proc_def)
{	GSList *proc_defs=wire_buffer->proc_defs;
	proc_defs = g_slist_prepend(proc_defs,proc_def);
	wire_buffer->proc_defs = proc_defs;
}

PlugInProcDef*
plug_in_file_handler (char *name,
		      char *extensions,
		      char *prefixes,
		      char *magics)
{
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp;

  if (wire_buffer->current_plug_in)
    {
      plug_in_def = wire_buffer->current_plug_in->user_data;
      tmp = plug_in_def->proc_defs;
    }
  else
    {
      tmp = wire_buffer->proc_defs;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, name) == 0)
	{
	  /* EXTENSIONS can be proc_def->extensions  */
	  if (proc_def->extensions != extensions)
	    {
	      if (proc_def->extensions)
		g_free (proc_def->extensions);
	      proc_def->extensions = g_strdup (extensions);
	    }
	  proc_def->extensions_list = plug_in_extensions_parse (proc_def->extensions);

	  /* PREFIXES can be proc_def->prefixes  */
	  if (proc_def->prefixes != prefixes)
	    {
	      if (proc_def->prefixes)
		g_free (proc_def->prefixes);
	      proc_def->prefixes = g_strdup (prefixes);
	    }
	  proc_def->prefixes_list = plug_in_extensions_parse (proc_def->prefixes);

	  /* MAGICS can be proc_def->magics  */
	  if (proc_def->magics != magics)
	    {
	      if (proc_def->magics)
		g_free (proc_def->magics);
	      proc_def->magics = g_strdup (magics);
	    }
	  proc_def->magics_list = plug_in_extensions_parse (proc_def->magics);
	  return proc_def;
	}
    }

  return NULL;
}

void
plug_in_def_add (PlugInDef *plug_in_def)
{
  GSList *tmp;
  PlugInDef *tplug_in_def;
  char *t1, *t2;

  t1 = strrchr (plug_in_def->prog, '/');
  if (t1)
    t1 = t1 + 1;
  else
    t1 = plug_in_def->prog;

  tmp = wire_buffer->plug_in_defs;
  while (tmp)
    {
      tplug_in_def = tmp->data;

      t2 = strrchr (tplug_in_def->prog, '/');
      if (t2)
	t2 = t2 + 1;
      else
	t2 = tplug_in_def->prog;

      if (strcmp (t1, t2) == 0)
	{
	  if ((strcmp (plug_in_def->prog, tplug_in_def->prog) == 0) &&
	      (plug_in_def->mtime == tplug_in_def->mtime))
	    {
	      /* Use cached plug-in entry */
	      tmp->data = plug_in_def;
	      g_free (tplug_in_def->prog);
	      g_free (tplug_in_def);
	    }
	  else
	    {
	      g_free (plug_in_def->prog);
	      g_free (plug_in_def);
	    }
	  return;
	}

      tmp = tmp->next;
    }

  wire_buffer->write_pluginrc = TRUE;
  g_print ("\"%s\" executable not found\n", plug_in_def->prog);
  g_free (plug_in_def->prog);
  g_free (plug_in_def);
}

char*
plug_in_menu_path (char *name)
{
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp, *tmp2;

  tmp = wire_buffer->plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      tmp2 = plug_in_def->proc_defs;
      while (tmp2)
	{
	  proc_def = tmp2->data;
	  tmp2 = tmp2->next;

	  if (strcmp (proc_def->db_info.name, name) == 0)
	    return proc_def->menu_path;
	}
    }

  tmp = wire_buffer->proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, name) == 0)
	return proc_def->menu_path;
    }

  return NULL;
}

PlugIn*
plug_in_new (char *name)
{
  PlugIn *plug_in;
  char *path;
#ifndef WIN32
  if (name[0] != '/')
    {
      path = search_in_path (plug_in_path, name);
      if (!path)
	{
	  g_message (_("unable to locate plug-in: \"%s\""), name);
	  return NULL;
	}
    }
  else
#endif
    {
      path = name;
    }

  plug_in = g_new (PlugIn, 1);

  plug_in->open = FALSE;
  plug_in->destroy = FALSE;
  plug_in->query = FALSE;
  plug_in->synchronous = FALSE;
  plug_in->recurse = FALSE;
  plug_in->busy = FALSE;
#ifdef WIN32
  plug_in->handle=INVALID_HANDLE_VALUE;
  plug_in->plugin_main=0;
  plug_in->PLUG_IN_INFO=0;
  plug_in->wire_buffer=0;
#else
  plug_in->pid = 0;
  plug_in->my_read = 0;
  plug_in->my_write = 0;
  plug_in->his_read = 0;
  plug_in->his_write = 0;
  plug_in->input_id = 0;
#endif
  plug_in->args[0] = g_strdup (path);
  plug_in->args[1] = g_strdup ("-gimp");
  plug_in->args[2] = g_new (char, 16);
  plug_in->args[3] = g_new (char, 16);
  plug_in->args[4] = NULL;
  plug_in->args[5] = NULL;
  plug_in->args[6] = NULL;

/*  plug_in->temp_proc_defs = NULL;
*/  plug_in->progress = NULL;
  plug_in->progress_label = NULL;
  plug_in->progress_bar = NULL;
  plug_in->user_data = NULL;
#ifdef WIN32
	wire_buffer->plugin=plug_in;
	wire_buffer->current_plug_in=plug_in;
#endif
  return plug_in;
}

void
plug_in_destroy (PlugIn *plug_in)
{
  if (plug_in)
    {
      plug_in_close (plug_in, TRUE);

      if (plug_in->args[0])
	g_free (plug_in->args[0]);
      if (plug_in->args[1])
	g_free (plug_in->args[1]);
      if (plug_in->args[2])
	g_free (plug_in->args[2]);
      if (plug_in->args[3])
	g_free (plug_in->args[3]);
      if (plug_in->args[4])
	g_free (plug_in->args[4]);
      if (plug_in->args[5])
	g_free (plug_in->args[5]);

      if (plug_in == wire_buffer->current_plug_in)
	plug_in_pop ();

      if (!plug_in->destroy)
	g_free (plug_in);
    }
}

int
plug_in_open (PlugIn *plug_in)
{
#ifndef WIN32
  int my_read[2];
  int my_write[2];
#endif

  if (plug_in)
    {
#ifndef WIN32
      /* Open two pipes. (Bidirectional communication).
       */
      if ((pipe (my_read) == -1) || (pipe (my_write) == -1))
	{
	  g_message (_("unable to open pipe"));
	  return 0;
	}

      plug_in->my_read = my_read[0];
      plug_in->my_write = my_write[1];
      plug_in->his_read = my_write[0];
      plug_in->his_write = my_read[1];

      /* Remember the file descriptors for the pipes.
       */
      sprintf (plug_in->args[2], "%d", plug_in->his_read);
      sprintf (plug_in->args[3], "%d", plug_in->his_write);
#endif
      /* Set the rest of the command line arguments.
       */
      if (plug_in->query)
	{
	  plug_in->args[4] = g_strdup ("-query");
	}
      else
	{
	  plug_in->args[4] = g_new (char, 16);
	  plug_in->args[5] = g_new (char, 16);

	  sprintf (plug_in->args[4], "%d", TILE_WIDTH);
	  sprintf (plug_in->args[5], "%d", TILE_WIDTH);
	}
#ifdef WIN32
/*	This is how it would be if plug-ins were Windows executables
      plug_in->pid = spawnv (_P_NOWAIT, plug_in->args[0], plug_in->args);
      if (plug_in->pid == -1)*/
	plug_in->wire_buffer=wire_buffer;
	if(!plug_in->query)
	{	return 1;
	}
/*	if(!PlugInRun(plug_in)) 
	Run plug-in in main thread so we don't need to 
	explicitly wait on it to complete */
	if(!PlugInLoader(plug_in))
	{	e_printf(_("PlugInRun failed!\n"));
/*          plug_in_destroy(plug_in); -- don't, causes double delete!*/
	}
/*	WaitForPlugin(plug-in);*/
#else

      /* Fork another process. We'll remember the process id
       *  so that we can later use it to kill the filter if
       *  necessary.
       */
      plug_in->pid = fork ();

#ifdef PYTHONDIR
      {
        char *pydir = "PYTHONPATH=" PYTHONDIR;
        putenv( pydir );
      }
#endif

      if (plug_in->pid == 0)
	{
	  close(plug_in->my_read);
	  close(plug_in->my_write);
          /* Execute the filter. The "_exit" call should never
           *  be reached, unless some strange error condition
           *  exists.
           */
          {
            char* plug_in_wrapper = getenv("CP_DBG_PLUG_INS");
            const char* plugin_name = getenv("CP_DBG_PLUG_IN");
            int pick = 1;

            if(plugin_name && !strstr( plug_in->args[0], plugin_name ))
              pick = 0;

            if(plug_in_wrapper && pick)
            {
              char *args[8];
              args[0] = plug_in_wrapper;
              args[1] = plug_in->args[0];
              args[2] = plug_in->args[1];
              args[3] = plug_in->args[2];
              args[4] = plug_in->args[3];
              args[5] = plug_in->args[4];
              args[6] = plug_in->args[5];
              args[7] = NULL;

              printf("%s:%d %s() running: %s %s %s %s %s %s %s\n",__FILE__,
                     __LINE__,__func__,
                     args[0],args[1],args[2],args[3],args[4],args[5],args[6]);

              execvp (args[0], args);
            } else
              execvp (plug_in->args[0], plug_in->args);
          }
          _exit (1);
	}
      else if (plug_in->pid == -1)
	{
          g_message (_("unable to run plug-in: %s\n"), plug_in->args[0]);
          plug_in_destroy (plug_in);
          return 0;
	}
      close(plug_in->his_read);  plug_in->his_read  = -1;
      close(plug_in->his_write); plug_in->his_write = -1;

      if (!plug_in->synchronous)
	{
	  plug_in->input_id = gdk_input_add (plug_in->my_read,
					     GDK_INPUT_READ,
					     plug_in_recv_message,
					     plug_in);
/*	  plug_in->wire_buffer->open_plug_ins = g_slist_prepend (plug_in->wire_buffer->open_plug_ins, plug_in);
*/
/*	extern WireBuffer* wire_buffer;*/
	wire_buffer->open_plug_ins = g_slist_prepend (wire_buffer->open_plug_ins, plug_in);


	}
#endif
      plug_in->open = TRUE;
      return 1;
    }

  return 0;
}

void
plug_in_close (PlugIn *plug_in,
	       int     kill_it)
{
  int status;
  struct timeval tv;

  if (plug_in && plug_in->open)
    {
      plug_in->open = FALSE;
#ifdef WIN32
	FreeLibrary(plug_in->handle);
	plug_in->handle=INVALID_HANDLE_VALUE;
	plug_in->plugin_main=0;
	plug_in->PLUG_IN_INFO=0;
#else
      /* Ask the filter to exit gracefully
       */
      if (kill_it && plug_in->pid)
	{
	  plug_in_push (plug_in);
/*	  gp_quit_write (plug_in->wire_buffer->current_writefd);*/
	  gp_quit_write (wire_buffer->current_writefd);
	  plug_in_pop ();

	  /*  give the plug-in some time (10 ms)  */
	  tv.tv_sec = 0;
	  tv.tv_usec = 100;
	  select (0, NULL, NULL, NULL, &tv);
	}

      /* If necessary, kill the filter.
       */
      if (kill_it && plug_in->pid)
	status = kill (plug_in->pid, SIGKILL);

      /* Wait for the process to exit. This will happen
       *  immediately if it was just killed.
       */
      if (plug_in->pid)
        waitpid (plug_in->pid, &status, 0);

      /* Remove the input handler.
       */
      if (plug_in->input_id)
        gdk_input_remove (plug_in->input_id);

      /* Close the pipes.
       */
      if (plug_in->my_read)
        close (plug_in->my_read);
      if (plug_in->my_write)
        close (plug_in->my_write);
      if (plug_in->his_read)
        close (plug_in->his_read);
      if (plug_in->his_write)
        close (plug_in->his_write);
#endif

      wire_clear_error();

      /* Destroy the progress dialog if it exists
       */
#ifdef SEPARATE_PROGRESS_BAR
      if (plug_in->progress)
	{
	  gtk_signal_disconnect_by_data (GTK_OBJECT (plug_in->progress), plug_in);
	  gtk_widget_destroy (plug_in->progress);

	  plug_in->progress = NULL;
	  plug_in->progress_label = NULL;
	  plug_in->progress_bar = NULL;
	}
#else
      if (plug_in->progress)
	{
	  progress_end ();
	  plug_in->progress = NULL;
	}
#endif

      /* Set the fields to null values.
       */
#ifdef WIN32

#else
      plug_in->pid = 0;
      plug_in->input_id = 0;
      plug_in->my_read = 0;
      plug_in->my_write = 0;
      plug_in->his_read = 0;
      plug_in->his_write = 0;

      if (plug_in->recurse)
	gtk_main_quit ();
#endif
      plug_in->synchronous = FALSE;
      plug_in->recurse = FALSE;

/* rsr: this was killing xcf handlers */
#ifdef UNREGISTER_TEMP_PROCEDURES
      /* Unregister any temporary procedures
       */
/*      if (plug_in->temp_proc_defs)
*/	if(wire_buffer->proc_defs)
	{
	  GSList *list;
	  PlugInProcDef *proc_def;

/*	  list = plug_in->temp_proc_defs;
*/	list=wire_buffer->proc_defs;
	while (list)
	    {
	      proc_def = (PlugInProcDef *) list->data;
	      plug_in_proc_def_remove (proc_def);
	      list = list->next;
	    }

/*	  g_slist_free (plug_in->temp_proc_defs);
*/	  g_slist_free(wire_buffer->proc_defs);
/*	plug_in->temp_proc_defs = NULL;*/
	wire_buffer->proc_defs=0;
	}
#endif
      wire_buffer->open_plug_ins = g_slist_remove (wire_buffer->open_plug_ins,plug_in);
    }
}

static Argument* plug_in_get_current_return_vals(ProcRecord *proc_rec)
{
  Argument *return_vals;
  int nargs;

  /* Return the status code plus the current return values. */
  nargs = proc_rec->num_values + 1;
  if (wire_buffer->current_return_vals && wire_buffer->current_return_nvals == nargs)
    return_vals = wire_buffer->current_return_vals;
  else if (wire_buffer->current_return_vals)
    {
      /* Allocate new return values of the correct size. */
      return_vals = procedural_db_return_args (proc_rec, FALSE);

      /* Copy all of the arguments we can. */
      memcpy (return_vals, wire_buffer->current_return_vals,
	      sizeof (Argument) * MIN (wire_buffer->current_return_nvals, nargs));

      /* Free the old argument pointer.  This will cause a memory leak
	 only if there were more values returned than we need (which
	 shouldn't ever happen). */
      g_free (wire_buffer->current_return_vals);
    }
  else
    {
      /* Just return a dummy set of values. */
      return_vals = procedural_db_return_args (proc_rec, FALSE);
    }

  /* We have consumed any saved values, so clear them. */
  wire_buffer->current_return_nvals = 0;
  wire_buffer->current_return_vals = NULL;

  return return_vals;
}

Argument*
plug_in_run (ProcRecord *proc_rec,
	     Argument   *args,
	     int         synchronous,
	     int         destroy_values)
{
  GPConfig config;
  GPProcRun proc_run;
  Argument *return_vals;
  PlugIn *plug_in;
  WireMessage msg;
  return_vals = NULL;

  if (proc_rec->proc_type == PDB_TEMPORARY)
    {
      return_vals = plug_in_temp_run (proc_rec, args);
      goto done;
    }

  plug_in = plug_in_new (proc_rec->exec_method.plug_in.filename);
  if (plug_in)
    { 
     if (plug_in_open (plug_in))
	{	
		plug_in->open = TRUE;
#ifdef WIN32
		plug_in->wire_buffer=wire_buffer;
		wire_buffer->plugin=plug_in;
		plug_in->synchronous=TRUE;
#endif
	  plug_in->recurse = synchronous;
	  plug_in_push (plug_in);

	  config.version = GP_VERSION;
	  config.tile_width = TILE_WIDTH;
	  config.tile_height = TILE_HEIGHT;
	  config.shm_ID = wire_buffer->shm_ID;
	  config.gamma = gamma_val;
	  config.install_cmap = install_cmap;
	  config.use_xshm = gdk_get_use_xshm ();
	  config.color_cube[0] = color_cube_shades[0];
	  config.color_cube[1] = color_cube_shades[1];
	  config.color_cube[2] = color_cube_shades[2];
	  config.color_cube[3] = color_cube_shades[3];

	  proc_run.name = proc_rec->name;
	  proc_run.nparams = proc_rec->num_args;
	  proc_run.params = plug_in_args_to_params (args, proc_rec->num_args, FALSE);
	  if (!gp_config_write (wire_buffer->current_writefd, &config) ||
	      !gp_proc_run_write (wire_buffer->current_writefd, &proc_run) ||
	      !wire_flush (wire_buffer->current_writefd))
	    {
	      return_vals = procedural_db_return_args (proc_rec, FALSE);
	      goto done;
	    }
#ifdef WIN32
		if(!PlugInRun(plug_in))
		{	e_printf("PlugInRun failed!\n");
		}
#else
	  plug_in_pop ();
#endif
	  plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

	  /*
	   *  If this is an automatically installed extension, wait for an
	   *   installation-confirmation message
	   */
	  if ((proc_rec->proc_type == PDB_EXTENSION) && (proc_rec->num_args == 0))
#ifdef WIN32
		e_puts("ERROR: PDB_EXTENSION not implemented");
#else
	    gtk_main ();
#endif
#ifdef WIN32
		msg.type=GP_ERROR;
		while(GP_QUIT!=msg.type)
		{	GetWireBuffer(&msg);
			if(GP_QUIT==msg.type)
			{	break;
			}
			plug_in_handle_message(&msg);
			if(GP_PROC_RETURN==msg.type)
			{      return_vals = plug_in_get_current_return_vals(proc_rec);
		}	}
		plug_in_pop();
#endif
	  if (plug_in->recurse)
	    {
#ifdef WIN32
/*		DrainWireBuffer();*/
#else
	      gtk_main ();
#endif
	      return_vals = plug_in_get_current_return_vals (proc_rec);
	    }

	}
    }
done:
  if (return_vals && destroy_values)
    {
      procedural_db_destroy_args (return_vals, proc_rec->num_values);
      return_vals = NULL;
    }
  return return_vals;
}

void
plug_in_repeat (int with_interface)
{
  GDisplay *gdisplay;
  Argument *args;
  int i;

  if (wire_buffer->last_plug_in)
    {
      gdisplay = gdisplay_active ();

      /* construct the procedures arguments */
      args = g_new (Argument, wire_buffer->last_plug_in->num_args);
      memset (args, 0, (sizeof (Argument) * wire_buffer->last_plug_in->num_args));

      /* initialize the argument types */
      for (i = 0; i < wire_buffer->last_plug_in->num_args; i++)
	args[i].arg_type = wire_buffer->last_plug_in->args[i].arg_type;

      /* initialize the first 3 plug-in arguments  */
      args[0].value.pdb_int = (with_interface ? RUN_INTERACTIVE : RUN_WITH_LAST_VALS);
      args[1].value.pdb_int = gdisplay->gimage->ID;
      args[2].value.pdb_int = drawable_ID (gimage_active_drawable (gdisplay->gimage));

      /* run the plug-in procedure */
      plug_in_run (wire_buffer->last_plug_in, args, FALSE, TRUE);

      g_free (args);
    }
}

void
plug_in_set_menu_sensitivity (Tag t)
{
  PlugInProcDef *proc_def;
  GSList *tmp;
  int sensitive = FALSE;
  int base_type = tag_to_drawable_type (t);
  
  tmp = wire_buffer->proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->image_types_val && proc_def->menu_path)
	{
	  switch (base_type)
	    {
	    case -1:
	      sensitive = FALSE;
	      break;
	    case RGB_GIMAGE:
	      sensitive = proc_def->image_types_val & RGB_IMAGE;
	      break;
	    case RGBA_GIMAGE:
	      sensitive = proc_def->image_types_val & RGBA_IMAGE;
	      break;
	    case GRAY_GIMAGE:
	      sensitive = proc_def->image_types_val & GRAY_IMAGE;
	      break;
	    case GRAYA_GIMAGE:
	      sensitive = proc_def->image_types_val & GRAYA_IMAGE;
	      break;
	    case INDEXED_GIMAGE:
	      sensitive = proc_def->image_types_val & INDEXED_IMAGE;
	      break;
	    case INDEXEDA_GIMAGE:
	      sensitive = proc_def->image_types_val & INDEXEDA_IMAGE;
	      break;
	    case U16_RGB_GIMAGE:
	      sensitive = proc_def->image_types_val & U16_RGB_IMAGE;
	      break;
	    case U16_RGBA_GIMAGE:
	      sensitive = proc_def->image_types_val & U16_RGBA_IMAGE;
	      break;
	    case U16_GRAY_GIMAGE:
	      sensitive = proc_def->image_types_val & U16_GRAY_IMAGE;
	      break;
	    case U16_GRAYA_GIMAGE:
	      sensitive = proc_def->image_types_val & U16_GRAYA_IMAGE;
	      break;
	    case FLOAT_RGB_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT_RGB_IMAGE;
	      break;
	    case FLOAT_RGBA_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT_RGBA_IMAGE;
	      break;
	    case FLOAT_GRAY_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT_GRAY_IMAGE;
	      break;
	    case FLOAT_GRAYA_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT_GRAYA_IMAGE;
	      break;
	    case FLOAT16_RGB_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT16_RGB_IMAGE;
	      break;
	    case FLOAT16_RGBA_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT16_RGBA_IMAGE;
	      break;
	    case FLOAT16_GRAY_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT16_GRAY_IMAGE;
	      break;
	    case FLOAT16_GRAYA_GIMAGE:
	      sensitive = proc_def->image_types_val & FLOAT16_GRAYA_IMAGE;
	      break;
	    }

	  menus_set_sensitive (proc_def->menu_path, sensitive);
          if (wire_buffer->last_plug_in && (wire_buffer->last_plug_in == &(proc_def->db_info)))
	    {
	      menus_set_sensitive ("<Image>/Filters/Repeat last", sensitive);
	      menus_set_sensitive ("<Image>/Filters/Re-show last", sensitive);
	    }
	}
    }
}

static void
plug_in_recv_message (gpointer          data,
		      gint              id,
		      GdkInputCondition cond)
{
  WireMessage msg;
  plug_in_push ((PlugIn*) data);
  memset (&msg, 0, sizeof (WireMessage));
  if (!wire_read_msg (wire_buffer->current_readfd, &msg))
    plug_in_close (wire_buffer->current_plug_in, TRUE);
  else
    {
      plug_in_handle_message (&msg);
      wire_destroy (&msg);
    }
  if (!wire_buffer->current_plug_in->open)
    plug_in_destroy (wire_buffer->current_plug_in);
  else
    plug_in_pop ();
}

static void
plug_in_handle_proc_return (GPProcReturn *proc_return);

void plug_in_handle_message (WireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      plug_in_handle_quit ();
      break;
    case GP_CONFIG:
      g_message (_("plug_in_handle_message(): received a config message (should not happen)\n"));
      plug_in_close (wire_buffer->current_plug_in, TRUE);
      break;
    case GP_TILE_REQ:
      plug_in_handle_tile_req (msg->data);
      break;
    case GP_TILE_ACK:
      g_message (_("plug_in_handle_message(): received a config message (should not happen)\n"));
      plug_in_close (wire_buffer->current_plug_in, TRUE);
      break;
    case GP_TILE_DATA:
      g_message (_("plug_in_handle_message(): received a config message (should not happen)\n"));
      plug_in_close (wire_buffer->current_plug_in, TRUE);
      break;
    case GP_PROC_RUN:
      plug_in_handle_proc_run (msg->data);
      break;
    case GP_PROC_RETURN:
      plug_in_handle_proc_return (msg->data);
      plug_in_close (wire_buffer->current_plug_in, FALSE);
      break;
    case GP_TEMP_PROC_RUN:
      g_message (_("plug_in_handle_message(): received a temp proc run message (should not happen)\n"));
      plug_in_close (wire_buffer->current_plug_in, TRUE);
      break;
    case GP_TEMP_PROC_RETURN:
      plug_in_handle_proc_return (msg->data);
      gtk_main_quit ();
      break;
    case GP_PROC_INSTALL:
      plug_in_handle_proc_install (msg->data);
      break;
    case GP_PROC_UNINSTALL:
      plug_in_handle_proc_uninstall (msg->data);
      break;
    case GP_EXTENSION_ACK:
      gtk_main_quit ();
      break;
    }
}

static void
plug_in_handle_quit ()
{
  plug_in_close (wire_buffer->current_plug_in, FALSE);
}

static void
plug_in_handle_tile_req (GPTileReq *tile_req)
{
  GPTileData tile_data;
  GPTileData *tile_info;
  WireMessage msg;
  PixelArea a;
  Canvas *canvas;
  CanvasDrawable *drawable;
  gint ntile_cols, ntile_rows;
  gint ewidth, eheight;
  gint i, j;
  gint x, y;

  if (tile_req->drawable_ID == -1)
    {
      tile_data.drawable_ID = -1;
      tile_data.tile_num = 0;
      tile_data.shadow = 0;
      tile_data.bpp = 0;
      tile_data.width = 0;
      tile_data.height = 0;
#ifdef BUILD_SHM
      tile_data.use_shm = (wire_buffer->shm_ID == -1) ? FALSE : TRUE;
#endif
      tile_data.data = NULL;

      if (!gp_tile_data_write (wire_buffer->current_writefd, &tile_data))
	{
	  g_message (_("plug_in_handle_tile_req: ERROR"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

	TaskSwitchToPlugin();

      if (!wire_read_msg(wire_buffer->current_readfd, &msg))
	{
	  g_message (_("plug_in_handle_tile_req: ERROR"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

      if (msg.type != GP_TILE_DATA)
	{
	  g_message (_("expected tile data and received: %d\n"), msg.type);
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

      tile_info = msg.data;
      drawable = drawable_get_ID (tile_info->drawable_ID);
      
      if (tile_info->shadow)
        canvas = drawable_shadow (drawable);
      else
	canvas = drawable_data (drawable);

      if (!canvas)
	{
	  g_message (_("plug-in requested invalid drawable (killing)\n"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}
        
        /* Find the upper left corner (x,y) of the plugins tile with index tile_num. */ 
	ntile_cols = ( drawable_width (drawable) + TILE_WIDTH - 1 ) / TILE_WIDTH;
        ntile_rows = ( drawable_height (drawable) + TILE_HEIGHT - 1 ) / TILE_HEIGHT;
	i = tile_info->tile_num % ntile_cols;
	j = tile_info->tile_num / ntile_cols;
	x = TILE_WIDTH * i;
	y = TILE_HEIGHT * j; 
        /*d_printf ("Get tilenum: %d, (i,j)=(%d,%d) (x,y)=(%d,%d) (rows,cols)=(%d,%d)\n",
			tile_info->tile_num, i, j, x, y, ntile_rows, ntile_cols);*/
        
        /* Make sure there is some valid canvas data there on the gimp side */
       canvas_portion_refrw (canvas, x, y);
       if (!canvas_portion_data( canvas, x, y))
	{
	  g_message (_("plug-in requested invalid tile (killing)\n"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
          canvas_portion_unref (canvas, x, y);
	  return;
	}
        
        /* Now find the plugins tile ewidth and eheight */
	if (i == (ntile_cols - 1))
	  ewidth = drawable_width(drawable) - ((ntile_cols - 1) * TILE_WIDTH);
	else
	  ewidth = TILE_WIDTH;

	if (j == (ntile_rows - 1))
	  eheight = drawable_height(drawable) - ((ntile_rows - 1) * TILE_HEIGHT);
	else
	  eheight = TILE_HEIGHT;
        
        /* Now copy the data from plugins tile to the gimp's canvas */ 
	pixelarea_init (&a, canvas, x, y, ewidth, eheight, TRUE);
#ifdef BUILD_SHM	
	if (tile_data.use_shm)
	  plug_in_copyarea (&a, wire_buffer->shm_addr, COPY_BUFFER_TO_AREA);  /* shm to gimp */
	else
#endif
	  plug_in_copyarea (&a, tile_info->data, COPY_BUFFER_TO_AREA); /* pipe buffer to gimp */
        canvas_portion_unref (canvas, x, y);
      
      wire_destroy (&msg);
      if (!gp_tile_ack_write (wire_buffer->current_writefd))
	{
	  g_message (_("plug_in_handle_tile_req: ERROR"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}
    }
  else
    {
      if (tile_req->shadow)
        canvas = drawable_shadow (drawable_get_ID (tile_req->drawable_ID));
      else
	canvas = drawable_data (drawable_get_ID (tile_req->drawable_ID));
      
      if (!canvas)
	{
	  g_message (_("plug-in requested invalid drawable (killing)\n"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}
     
      drawable = drawable_get_ID (tile_req->drawable_ID);
      
      /* Find the lower left corner (x,y) of the plugins tile with index tile_num. */ 
      ntile_cols = ( drawable_width (drawable) + TILE_WIDTH - 1 ) / TILE_WIDTH;
      ntile_rows = ( drawable_height (drawable) + TILE_HEIGHT - 1 ) / TILE_HEIGHT;
      i = tile_req->tile_num % ntile_cols;
      j = tile_req->tile_num / ntile_cols;
      x = TILE_WIDTH * i;
      y = TILE_HEIGHT * j;
      /*d_printf ("Put tilenum: %d, (i,j)=(%d,%d) (x,y)=(%d,%d) (rows,cols)=(%d,%d)\n",
			tile_req->tile_num, i, j, x, y, ntile_rows, ntile_cols);*/
      
      canvas_portion_refro (canvas, x, y);
      if (!canvas_portion_data( canvas, x, y))
	{
	  g_message (_("plug-in requested invalid tile (killing)\n"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
          canvas_portion_unref (canvas, x, y);
	  return;
	}

      tile_data.drawable_ID = tile_req->drawable_ID;
      tile_data.tile_num = tile_req->tile_num;
      tile_data.shadow = tile_req->shadow;
      tile_data.bpp = tag_bytes (canvas_tag (canvas));
      
      /* Find the plugins notion of ewidth and eheight of the tile */
      if (i == (ntile_cols - 1))
	ewidth = drawable_width(drawable) - ((ntile_cols - 1) * TILE_WIDTH);
      else
	ewidth = TILE_WIDTH;

      if (j == (ntile_rows - 1))
	eheight = drawable_height(drawable) - ((ntile_rows - 1) * TILE_HEIGHT);
      else
	eheight = TILE_HEIGHT;
      
      tile_data.height = eheight;
      tile_data.width = ewidth; 
#ifdef BUILD_SHM
      tile_data.use_shm = (wire_buffer->shm_ID == -1) ? FALSE : TRUE;
#endif
      /*d_printf(" ewidth = %d, eheight = %d \n", ewidth, eheight);*/
      pixelarea_init (&a, canvas, x, y, ewidth, eheight, FALSE);
#ifdef BUILD_SHM
      if (tile_data.use_shm)
	plug_in_copyarea (&a, wire_buffer->shm_addr, COPY_AREA_TO_BUFFER);  /* gimp to shm*/
      else
#endif
      {/* rsr: g_malloc, not malloc!!! */
		tile_data.data = g_malloc( tile_data.height * tile_data.width * tile_data.bpp );
		plug_in_copyarea (&a, tile_data.data, COPY_AREA_TO_BUFFER); /* gimp to pipe buffer */
	  }
      
      canvas_portion_unref (canvas, x, y);
      if (!gp_tile_data_write (wire_buffer->current_writefd, &tile_data))
	{
	  g_message (_("plug_in_handle_tile_req: ERROR"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

	TaskSwitchToPlugin();

      if (!wire_read_msg(wire_buffer->current_readfd, &msg))
	{
	  g_message (_("plug_in_handle_tile_req: ERROR"));
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

      if (msg.type != GP_TILE_ACK)
	{
	  g_message ("expected tile ack and received: %d\n", msg.type);
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}
#ifdef BUILD_SHM
      if (!tile_data.use_shm)
#endif
      if(tile_data.data)
        g_free( tile_data.data );
      wire_destroy (&msg);
    }
}


static void
plug_in_copyarea (PixelArea *a, guchar *buf, gint direction)
{
  void *pag;
  Tag a_tag = pixelarea_tag (a);
  gint bpp = tag_bytes (a_tag);
  guchar *d, *s;
  gint d_rowstride, s_rowstride;
  gint width, height;
  
  for (pag = pixelarea_register (1, a) ;
       pag != NULL;
       pag = pixelarea_process (pag))
    {
      height = pixelarea_height (a);
      width = pixelarea_width (a);
      
      if (direction == COPY_AREA_TO_BUFFER)
      {
        /* copy from a to buf */
	s = pixelarea_data (a);                           
        s_rowstride = pixelarea_rowstride (a);
        d = buf;
        d_rowstride = width * bpp;
/*        d_printf("putting data : s_rowstride: %d, d_rowstride: %d\n",
			s_rowstride, d_rowstride); 
      d_printf("pixelarea width : %d, height: %d\n", width, height);*/
      }
      else 
      {
        /* copy from buf to a */
        s = buf;
        s_rowstride = width * bpp;
	d = pixelarea_data (a);
        d_rowstride = pixelarea_rowstride (a);
/*        d_printf("getting data : s_rowstride: %d, d_rowstride: %d\n", 
			s_rowstride, d_rowstride); 
      d_printf("pixelarea width : %d, height: %d\n", width, height);*/
      } 
       while ( height --)
        {/* rsr: scanline-based region copy */
          memcpy(d, s, width * bpp);
	  d += d_rowstride;
	  s += s_rowstride;
        } 
    }
}


static void
plug_in_handle_proc_run (GPProcRun *proc_run)
{
  GPProcReturn proc_return;
  ProcRecord *proc_rec;
  Argument *args;
  Argument *return_vals;
  PlugInBlocked *blocked;

  args = plug_in_params_to_args (proc_run->params, proc_run->nparams, FALSE);
  proc_rec = procedural_db_lookup (proc_run->name);

  if (!proc_rec)
    {
      /* THIS IS PROBABLY NOT CORRECT -josh */
      g_message (_("PDB lookup failed on %s\n"), proc_run->name);
      plug_in_args_destroy (args, proc_run->nparams, FALSE);
      return;
    }
#ifdef CRASHES_UNSHARP_PLUGIN
  d_printf("plug_in_handle_proc_run: %s %s\n",proc_run->name,proc_run->params->data.d_string);
#endif
  return_vals = procedural_db_execute (proc_run->name, args);

  if (return_vals)
    {
      proc_return.name = proc_run->name;

      if (proc_rec)
	{
	  proc_return.nparams = proc_rec->num_values + 1;
	  proc_return.params = plug_in_args_to_params (return_vals, proc_rec->num_values + 1, FALSE);
	}
      else
	{
	  proc_return.nparams = 1;
	  proc_return.params = plug_in_args_to_params (return_vals, 1, FALSE);
	}

      if (!gp_proc_return_write (wire_buffer->current_writefd, &proc_return))
	{
	  g_message ("plug_in_handle_proc_run: ERROR");
	  plug_in_close (wire_buffer->current_plug_in, TRUE);
	  return;
	}

      plug_in_args_destroy (args, proc_run->nparams, FALSE);
      plug_in_args_destroy (return_vals, (proc_rec ? (proc_rec->num_values + 1) : 1), TRUE);
      plug_in_params_destroy (proc_return.params, proc_return.nparams, FALSE);
    }
  else
    {
      blocked = g_new (PlugInBlocked, 1);
      blocked->plug_in = wire_buffer->current_plug_in;
      blocked->proc_name = g_strdup (proc_run->name);
      wire_buffer->blocked_plug_ins = g_slist_prepend (wire_buffer->blocked_plug_ins, blocked);
    }
}

static void
plug_in_handle_proc_return (GPProcReturn *proc_return)
{	
  PlugInBlocked *blocked;
  GSList *tmp;
if(!wire_buffer || !wire_buffer->current_plug_in)
{	e_puts("ERROR: plug_in_handle_proc_return <null>");
	return;
}
  if (wire_buffer->current_plug_in->recurse)
    {
      wire_buffer->current_return_vals = plug_in_params_to_args (proc_return->params,
						    proc_return->nparams,
						    TRUE);
      wire_buffer->current_return_nvals = proc_return->nparams;
    }
  else
    {
      tmp = wire_buffer->blocked_plug_ins;
      while (tmp)
	{
	  blocked = tmp->data;
	  tmp = tmp->next;

	  if (strcmp (blocked->proc_name, proc_return->name) == 0)
	    {
	      plug_in_push (blocked->plug_in);
	      if (!gp_proc_return_write (wire_buffer->current_writefd, proc_return))
		{
		  g_message (_("plug_in_handle_proc_run: ERROR"));
		  plug_in_close (wire_buffer->current_plug_in, TRUE);
		  return;
		}
	      plug_in_pop ();
	      wire_buffer->blocked_plug_ins = g_slist_remove (wire_buffer->blocked_plug_ins, blocked);
	      g_free (blocked->proc_name);
	      g_free (blocked);
	      break;
	    }
	}
    }
}

static void
plug_in_handle_proc_install (GPProcInstall *proc_install)
{
  PlugInDef *plug_in_def = NULL;
  PlugInProcDef *proc_def;
  ProcRecord *proc = NULL;
  GSList *tmp = NULL;
  GtkMenuEntry entry;
  char *prog = NULL;
  int add_proc_def;
  guint32 i;

  /*
   *  Argument checking
   *   --only sanity check arguments when the procedure requests a menu path
   */

  if (proc_install->menu_path)
    {
      if (strncmp (proc_install->menu_path, "<Toolbox>", 9) == 0)
	{
	  if ((proc_install->nparams < 1) ||
	      (proc_install->params[0].type != PDB_INT32))
	    {
	      g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" which "
			 "does not take the standard plug-in args"),
			 wire_buffer->current_plug_in->args[0], proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Image>", 7) == 0)
	{
	  if ((proc_install->nparams < 3) ||
	      (proc_install->params[0].type != PDB_INT32) ||
	      (proc_install->params[1].type != PDB_IMAGE) ||
	      (proc_install->params[2].type != PDB_DRAWABLE))
	    {
	      g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" which "
			 "does not take the standard plug-in args"),
			 wire_buffer->current_plug_in->args[0], proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<sfm_menu>", 10) == 0)
	{
	  if (((proc_install->nparams < 1) ||
	      (proc_install->params[0].type != PDB_DISPLAY)) &&
	      ((proc_install->nparams < 3) ||
	      (proc_install->params[0].type != PDB_INT32) ||
	      (proc_install->params[1].type != PDB_IMAGE) ||
	      (proc_install->params[2].type != PDB_DRAWABLE)))
	    {
	      g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" which "
			 "does not take the standard plug-in args"),
			 wire_buffer->current_plug_in->args[0], proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Load>", 6) == 0)
	{
	  if ((proc_install->nparams < 3) ||
	      (proc_install->params[0].type != PDB_INT32) ||
	      (proc_install->params[1].type != PDB_STRING) ||
	      (proc_install->params[2].type != PDB_STRING) )
	    {
	      g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" which "
			 "does not take the standard plug-in args"),
			 wire_buffer->current_plug_in->args[0], proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Save>", 6) == 0)
	{
	  if ((proc_install->nparams < 5) ||
	      (proc_install->params[0].type != PDB_INT32) ||
	      (proc_install->params[1].type != PDB_IMAGE) ||
	      (proc_install->params[2].type != PDB_DRAWABLE) ||
	      (proc_install->params[3].type != PDB_STRING) ||
	      (proc_install->params[4].type != PDB_STRING))
	    {
	      g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" which "
			 "does not take the standard plug-in args"),
			 wire_buffer->current_plug_in->args[0], proc_install->name);
	      return;
	    }
	}
      else
	{
	  g_message (_("plug-in \"%s\" attempted to install procedure \"%s\" in "
		     "an invalid menu location.  Use either \"<Toolbox>\", \"<Image>\", "
		     "\"<Load>\", or \"<Save>\"."),
		     wire_buffer->current_plug_in->args[0], proc_install->name);
	  return;
	}
    }

  /*
   *  Sanity check for array arguments
   */

  for (i = 1; i < proc_install->nparams; i++)
    {
      if ((proc_install->params[i].type == PDB_INT32ARRAY ||
	   proc_install->params[i].type == PDB_INT8ARRAY ||
	   proc_install->params[i].type == PDB_FLOATARRAY ||
	   proc_install->params[i].type == PDB_STRINGARRAY) &&
	  proc_install->params[i-1].type != PDB_INT32)
	{
	  g_message (_("plug_in \"%s\" attempted to install procedure \"%s\" "
		     "which fails to comply with the array parameter "
		     "passing standard.  Argument %d is noncompliant."),
		     wire_buffer->current_plug_in->args[0], proc_install->name, i);
	  return;
	}
    }


  /*
   *  Initialization
   */
  proc_def = NULL;

  switch (proc_install->type)
    {
    case PDB_PLUGIN:
    case PDB_EXTENSION:
      plug_in_def = wire_buffer->current_plug_in->user_data;
      prog = plug_in_def->prog;

      tmp = plug_in_def->proc_defs;
      break;

    case PDB_TEMPORARY:
      prog = "none";

/*      tmp = wire_buffer->current_plug_in->temp_proc_defs;
*/	tmp=wire_buffer->proc_defs;
      break;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, proc_install->name) == 0)
	{
	  if (proc_install->type == PDB_TEMPORARY)
	    plug_in_proc_def_remove (proc_def);
	  else
	    plug_in_proc_def_destroy (proc_def, TRUE);
	  break;
	}

      proc_def = NULL;
    }

  add_proc_def = FALSE;
  if (!proc_def)
    {
      add_proc_def = TRUE;
      proc_def = g_new (PlugInProcDef, 1);
    }

  proc_def->prog = g_strdup (prog);

  proc_def->menu_path = g_strdup (proc_install->menu_path);
  proc_def->accelerator = NULL;
  proc_def->extensions = NULL;
  proc_def->prefixes = NULL;
  proc_def->magics = NULL;
  proc_def->image_types = g_strdup (proc_install->image_types);
  proc_def->image_types_val = plug_in_image_types_parse (proc_def->image_types);

  proc = &proc_def->db_info;

  /*
   *  The procedural database procedure
   */
  proc->name = g_strdup (proc_install->name);
  proc->blurb = g_strdup (proc_install->blurb);
  proc->help = g_strdup (proc_install->help);
  proc->author = g_strdup (proc_install->author);
  proc->copyright = g_strdup (proc_install->copyright);
  proc->date = g_strdup (proc_install->date);
  proc->proc_type = proc_install->type;

  proc->num_args = proc_install->nparams;
  proc->num_values = proc_install->nreturn_vals;

  proc->args = g_new (ProcArg, proc->num_args);
  proc->values = g_new (ProcArg, proc->num_values);

  for (i = 0; i < proc->num_args; i++)
    {
      proc->args[i].arg_type = proc_install->params[i].type;
      proc->args[i].name = g_strdup (proc_install->params[i].name);
      proc->args[i].description = g_strdup (proc_install->params[i].description);
    }

  for (i = 0; i < proc->num_values; i++)
    {
      proc->values[i].arg_type = proc_install->return_vals[i].type;
      proc->values[i].name = g_strdup (proc_install->return_vals[i].name);
      proc->values[i].description = g_strdup (proc_install->return_vals[i].description);
    }

  switch (proc_install->type)
    {
    case PDB_PLUGIN:
    case PDB_EXTENSION:
      if (add_proc_def)
	plug_in_def->proc_defs = g_slist_prepend (plug_in_def->proc_defs, proc_def);
      break;

    case PDB_TEMPORARY:
      if (add_proc_def)
	{	/*wire_buffer->current_plug_in->temp_proc_defs = g_slist_prepend (wire_buffer->current_plug_in->temp_proc_defs, proc_def);
		*/
		 wire_buffer->proc_defs=g_slist_prepend(wire_buffer->proc_defs, proc_def);
/*      wire_buffer->proc_defs = g_slist_append (wire_buffer->proc_defs, proc_def);*/
      }
	 proc->exec_method.temporary.plug_in = (void *) wire_buffer->current_plug_in;
      procedural_db_register (proc);

      /*  If there is a menu path specified, create a menu entry  */
      if (proc_install->menu_path)
	{
	  entry.path = menu_plugins_translate( proc_install->menu_path );
	  entry.accelerator = NULL;
	  entry.callback = plug_in_callback;
	  entry.callback_data = proc;
	  menus_create (&entry, 1);
	}
      break;
    }
}

static void
plug_in_handle_proc_uninstall (GPProcUninstall *proc_uninstall)
{
  PlugInProcDef *proc_def;
  GSList *tmp;

/*  tmp = wire_buffer->current_plug_in->temp_proc_defs;
*/	tmp=wire_buffer->proc_defs;
	while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, proc_uninstall->name) == 0)
	{
/*	  wire_buffer->current_plug_in->temp_proc_defs = g_slist_remove (wire_buffer->current_plug_in->temp_proc_defs, proc_def);
*/
	  wire_buffer->proc_defs = g_slist_remove (wire_buffer->proc_defs, proc_def);

	  plug_in_proc_def_remove (proc_def);
	  break;
	}
    }
}

static void
plug_in_push (PlugIn *plug_in)
{
  if (plug_in)
    {
      wire_buffer->current_plug_in = plug_in;
      wire_buffer->plug_in_stack = g_slist_prepend (wire_buffer->plug_in_stack, wire_buffer->current_plug_in);
#ifdef WIN32
      wire_buffer->current_buffer_index = wire_buffer->buffer_write_index;
      wire_buffer->current_buffer = wire_buffer->buffer;
#else
      wire_buffer->current_readfd = wire_buffer->current_plug_in->my_read;
      wire_buffer->current_writefd = wire_buffer->current_plug_in->my_write;
      wire_buffer->current_buffer_index = wire_buffer->current_plug_in->write_buffer_index;
      wire_buffer->current_buffer = wire_buffer->current_plug_in->write_buffer;
#endif
    }
  else
    {
      wire_buffer->current_readfd = 0;
      wire_buffer->current_writefd = 0;
      wire_buffer->current_buffer_index = 0;
      wire_buffer->current_buffer = NULL;
    }
}

static void
plug_in_pop ()
{
  GSList *tmp;

  if (wire_buffer->current_plug_in)
    {
#ifdef _DEBUG
		WireBuffer* wb=wire_buffer;
#endif
#ifndef WIN32
      wire_buffer->buffer_write_index = wire_buffer->current_buffer_index;
#endif	
      tmp = wire_buffer->plug_in_stack;
	 if(tmp)
	 {	wire_buffer->plug_in_stack = wire_buffer->plug_in_stack->next;
		tmp->next = NULL;
		g_slist_free (tmp);
    } }

  if (wire_buffer->plug_in_stack)
    {
      wire_buffer->current_plug_in = wire_buffer->plug_in_stack->data;
#ifndef WIN32
      wire_buffer->current_readfd = wire_buffer->current_plug_in->my_read;
      wire_buffer->current_writefd = wire_buffer->current_plug_in->my_write;
#endif
      wire_buffer->current_buffer_index = wire_buffer->buffer_write_index;
      wire_buffer->current_buffer = wire_buffer->buffer;
    }
  else
    {
      wire_buffer->current_plug_in = NULL;
      wire_buffer->current_readfd = 0;
      wire_buffer->current_writefd = 0;
      wire_buffer->current_buffer_index = 0;
      wire_buffer->current_buffer = NULL;
    }
}

static void
plug_in_write_rc_string (FILE *fp,
			 char *str)
{
  fputc ('"', fp);

  if (str)
    while (*str)
      {
	if ((*str == '"') || (*str == '\\'))
	  fputc ('\\', fp);
	fputc (*str, fp);
	str += 1;
      }

  fputc ('"', fp);
}

static void
plug_in_write_rc (char *filename)
{
#ifdef WIN32

#else
  FILE *fp;
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp, *tmp2;
  int i;

  fp = fopen (filename, "wb");
  if (!fp)
    return;

  tmp = wire_buffer->plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      if (plug_in_def->proc_defs)
	{
	  fprintf (fp, "(plug-in-def \"%s\" %ld",
		   plug_in_def->prog,
		   (long) plug_in_def->mtime);

	  tmp2 = plug_in_def->proc_defs;
	  if (tmp2)
	    fprintf (fp, "\n");

	  while (tmp2)
	    {
	      proc_def = tmp2->data;
	      tmp2 = tmp2->next;

	      fprintf (fp, "             (proc-def \"%s\" %d\n",
		       proc_def->db_info.name, proc_def->db_info.proc_type);
	      fprintf (fp, "                       ");
	      plug_in_write_rc_string (fp, proc_def->db_info.blurb);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->db_info.help);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->db_info.author);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->db_info.copyright);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->db_info.date);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->menu_path);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->extensions);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->prefixes);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->magics);
	      fprintf (fp, "\n                       ");
	      plug_in_write_rc_string (fp, proc_def->image_types);
	      fprintf (fp, "\n                       %d %d\n",
		       proc_def->db_info.num_args, proc_def->db_info.num_values);

	      for (i = 0; i < proc_def->db_info.num_args; i++)
		{
		  fprintf (fp, "                       (proc-arg %d ",
			   proc_def->db_info.args[i].arg_type);

		  plug_in_write_rc_string (fp, proc_def->db_info.args[i].name);
		  plug_in_write_rc_string (fp, proc_def->db_info.args[i].description);

		  fprintf (fp, ")%s",
			   (proc_def->db_info.num_values ||
			    (i < (proc_def->db_info.num_args - 1))) ? "\n" : "");
		}

	      for (i = 0; i < proc_def->db_info.num_values; i++)
		{
		  fprintf (fp, "                       (proc-arg %d ",
			   proc_def->db_info.values[i].arg_type);

		  plug_in_write_rc_string (fp, proc_def->db_info.values[i].name);
		  plug_in_write_rc_string (fp, proc_def->db_info.values[i].description);

		  fprintf (fp, ")%s", (i < (proc_def->db_info.num_values - 1)) ? "\n" : "");
		}

	      fprintf (fp, ")");

	      if (tmp2)
		fprintf (fp, "\n");
	    }

	  fprintf (fp, ")\n");

	  if (tmp)
	    fprintf (fp, "\n");
	}
    }

  fclose (fp);
#endif
}

static void
plug_in_init_file (char *filename)
{
  GSList *tmp;
  PlugInDef *plug_in_def;
  char *plug_in_name;
  char *name;

  name = strrchr (filename, '/');
  if (name)
    name = name + 1;
  else
    name = filename;
	s_printf(_("Loading plug-in: %s\n"),filename);
  plug_in_def = NULL;
  tmp = wire_buffer->plug_in_defs;

  while (tmp) /* skip if same plug-in already loaded */
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      plug_in_name = strrchr (plug_in_def->prog, '/');
      if (plug_in_name)
	plug_in_name = plug_in_name + 1;
      else
	plug_in_name = plug_in_def->prog;

      if (strcmp (name, plug_in_name) == 0)
	{
	  g_print ("duplicate plug-in: \"%s\" (skipping)\n", filename);
	  return;
	}

      plug_in_def = NULL;
    }

  plug_in_def = g_new (PlugInDef, 1);
  plug_in_def->prog = g_strdup (filename);
  plug_in_def->proc_defs = NULL;
  plug_in_def->mtime = datafile_mtime ();
  plug_in_def->query = TRUE;

  wire_buffer->plug_in_defs = g_slist_append (wire_buffer->plug_in_defs, plug_in_def);

}

static void
plug_in_query (char      *filename,
	       PlugInDef *plug_in_def)
{
  PlugIn *plug_in;
  WireMessage msg;

  plug_in = plug_in_new (filename);
  if (plug_in)
    { plug_in->query = TRUE;
      plug_in->synchronous = TRUE;
      plug_in->user_data = plug_in_def;
      if (plug_in_open (plug_in))
	{
#ifdef WIN32
/* This is a hack because WIN32 has no pipes. Pass the wire_buffer itself. Robin */
	wire_buffer->current_readfd=(int)wire_buffer;
#endif
	  plug_in_push (plug_in);
	  while (plug_in->open)
	    {
	      if (!wire_read_msg (wire_buffer->current_readfd, &msg))
		plug_in_close (wire_buffer->current_plug_in, TRUE);
	      else
		{
		  plug_in_handle_message (&msg);
		  wire_destroy (&msg);
		}
	    }
	  plug_in_pop ();
	  plug_in_destroy (plug_in);
	}
    }
}

static void
plug_in_add_to_db ()
{
  PlugInProcDef *proc_def;
  Argument args[4];
  Argument *return_vals;
  GSList *tmp;

  tmp = wire_buffer->proc_defs;

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && (proc_def->db_info.proc_type != PDB_INTERNAL))
	{
#ifdef WIN32
	  proc_def->db_info.exec_method.plug_in.plugin_main = 0;
#endif
	  proc_def->db_info.exec_method.plug_in.filename = proc_def->prog;
	  procedural_db_register (&proc_def->db_info);
	}
    }

  for (tmp = wire_buffer->proc_defs; tmp; tmp = tmp->next)
    {
      proc_def = tmp->data;

      if (proc_def->extensions || proc_def->prefixes || proc_def->magics)
        {
          args[0].arg_type = PDB_STRING;
          args[0].value.pdb_pointer = proc_def->db_info.name;

          args[1].arg_type = PDB_STRING;
          args[1].value.pdb_pointer = proc_def->extensions;

	  args[2].arg_type = PDB_STRING;
	  args[2].value.pdb_pointer = proc_def->prefixes;

	  args[3].arg_type = PDB_STRING;
	  args[3].value.pdb_pointer = proc_def->magics;

          if (proc_def->image_types)
            {
              return_vals = procedural_db_execute ("gimp_register_save_handler", args);
              g_free (return_vals);
            }
          else
            {
              return_vals = procedural_db_execute ("gimp_register_magic_load_handler", args);
              g_free (return_vals);
            }
	}
    }
}

static void
plug_in_make_menu ()
{
  GtkMenuEntry entry;
  PlugInProcDef *proc_def;
  GSList *tmp;

  tmp = wire_buffer->proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && proc_def->menu_path && (!proc_def->extensions &&
						    !proc_def->prefixes &&
						    !proc_def->magics))
	{
	  entry.path = menu_plugins_translate( proc_def->menu_path );
	  entry.accelerator = proc_def->accelerator;
	  entry.callback = plug_in_callback;
	  entry.callback_data = &proc_def->db_info;

	  menus_create (&entry, 1);
	}
    }
  menus_propagate_plugins_all();
}



void
plug_in_make_image_menu (GtkItemFactory *fac, gchar *buff)
{
  GtkItemFactoryEntry entry;
  PlugInProcDef *proc_def;
  GSList *tmp;

  tmp = wire_buffer->proc_defs;
  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && proc_def->menu_path && (!proc_def->extensions &&
						    !proc_def->prefixes &&
						    !proc_def->magics && proc_def->image_types))
	{
	  if (strncmp(proc_def->menu_path, "<Image>", 6) == 0){
	    entry.path = menu_plugins_translate( index(proc_def->menu_path, '/') );
	    entry.accelerator = proc_def->accelerator;
	    entry.callback = plug_in_callback;
	    entry.item_type = "<Item>";
#           if 0
	    menus_create (&entry, 1);
#           else
            entry.path = menu_entry_translate( entry.path );
#           ifdef DEBUG_
            printf( "%s:%d: %s | %s\n", __FILE__,__LINE__, _(entry.path), entry.path );
#           endif
	    gtk_item_factory_create_items_ac (fac , 1, &entry, (gpointer)&proc_def->db_info, 2);
#           endif
	  }
      //gtk_item_factory_create_item (fac , &entry, NULL, 2);
	}
    }
}

static void
plug_in_callback (GtkWidget *widget,
		  gpointer   client_data)
{
  GDisplay *gdisplay;
  ProcRecord *proc_rec;
  Argument *args;
  int i;

  if(!client_data)
    g_warning("%s:%d %s() No ProcRecord deliverd.",__FILE__,__LINE__,__func__);

  /* get the active gdisplay */
  gdisplay = gdisplay_active ();

  proc_rec = (ProcRecord*) client_data;

  /* construct the procedures arguments */
  args = g_new (Argument, proc_rec->num_args);
  memset (args, 0, (sizeof (Argument) * proc_rec->num_args));

  /* initialize the argument types */
  for (i = 0; i < proc_rec->num_args; i++)
    args[i].arg_type = proc_rec->args[i].arg_type;

  switch (proc_rec->proc_type)
    {
    case PDB_EXTENSION:
      /* initialize the first argument  */
      args[0].value.pdb_int = RUN_INTERACTIVE;
      break;

    case PDB_PLUGIN:
      if (proc_rec->num_args && args[0].arg_type == PDB_DISPLAY)
	{
	  args[0].value.pdb_int = gdisplay->ID;
	}
      else if (gdisplay)
	{
	  /* initialize the first 3 plug-in arguments  */
	  args[0].value.pdb_int = RUN_INTERACTIVE;
	  args[1].value.pdb_int = gdisplay->gimage->ID;
	  args[2].value.pdb_int = drawable_ID (gimage_active_drawable (gdisplay->gimage));
	}
      else
	{
	  g_message (_("Uh-oh, no active gdisplay for the plug-in!"));
	  g_free (args);
	  return;
	}
      break;

    case PDB_TEMPORARY:
      args[0].value.pdb_int = RUN_INTERACTIVE;
      if (proc_rec->num_args >= 3 &&
	  proc_rec->args[1].arg_type == PDB_IMAGE &&
	  proc_rec->args[2].arg_type == PDB_DRAWABLE)
	{
	  if (gdisplay)
	    {
	      args[1].value.pdb_int = gdisplay->gimage->ID;
	      args[2].value.pdb_int = drawable_ID (gimage_active_drawable (gdisplay->gimage));
	    }
	  else
	    {
	      g_message (_("Uh-oh, no active gdisplay for the temporary procedure!"));
	      g_free (args);
	      return;
	    }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      break;
    }

  /* run the plug-in procedure */
  plug_in_run (proc_rec, args, FALSE, TRUE);

  if (proc_rec->proc_type == PDB_PLUGIN)
    wire_buffer->last_plug_in = proc_rec;

  g_free (args);
}

static void
plug_in_proc_def_insert (PlugInProcDef *proc_def)
{
  PlugInProcDef *tmp_proc_def;
  GSList *tmp, *prev;
  GSList *list;

  prev = NULL;
  tmp = wire_buffer->proc_defs;

  while (tmp)
    {
      tmp_proc_def = tmp->data;

      if (strcmp (proc_def->db_info.name, tmp_proc_def->db_info.name) == 0)
	{
	  tmp->data = proc_def;

	  if (proc_def->menu_path)
	    g_free (proc_def->menu_path);
	  if (proc_def->accelerator)
	    g_free (proc_def->accelerator);

	  proc_def->menu_path = tmp_proc_def->menu_path;
	  proc_def->accelerator = tmp_proc_def->accelerator;

	  tmp_proc_def->menu_path = NULL;
	  tmp_proc_def->accelerator = NULL;

	  plug_in_proc_def_destroy (tmp_proc_def, FALSE);
	  return;
	}
      else if (!proc_def->menu_path ||
	       (tmp_proc_def->menu_path &&
		(strcmp (proc_def->menu_path, tmp_proc_def->menu_path) < 0)))
	{
	  list = g_slist_alloc ();
	  list->data = proc_def;

	  list->next = tmp;
	  if (prev)
	    prev->next = list;
	  else
	    wire_buffer->proc_defs = list;
	  return;
	}

      prev = tmp;
      tmp = tmp->next;
    }

  wire_buffer->proc_defs = g_slist_append (wire_buffer->proc_defs, proc_def);
}

static void
plug_in_proc_def_remove (PlugInProcDef *proc_def)
{	if(!proc_def)
	{	return;
	}
  /*  Destroy the menu item  */
  if (proc_def->menu_path)
    menus_destroy (proc_def->menu_path);

  /*  Unregister the procedural database entry  */
  procedural_db_unregister (proc_def->db_info.name);

  /*  Remove the defintion from the global list  */
  wire_buffer->proc_defs = g_slist_remove (wire_buffer->proc_defs, proc_def);

  /*  Destroy the definition  */
  plug_in_proc_def_destroy (proc_def, TRUE);
}

static void
plug_in_proc_def_destroy (PlugInProcDef *proc_def,
			  int            data_only)
{
  int i;

  if (proc_def->prog)
    g_free (proc_def->prog);
  if (proc_def->menu_path)
    g_free (proc_def->menu_path);
  if (proc_def->accelerator)
    g_free (proc_def->accelerator);
  if (proc_def->extensions)
    g_free (proc_def->extensions);
  if (proc_def->prefixes)
    g_free (proc_def->prefixes);
  if (proc_def->magics)
    g_free (proc_def->magics);
  if (proc_def->image_types)
    g_free (proc_def->image_types);
  if (proc_def->db_info.name)
    g_free (proc_def->db_info.name);
  if (proc_def->db_info.blurb)
    g_free (proc_def->db_info.blurb);
  if (proc_def->db_info.help)
    g_free (proc_def->db_info.help);
  if (proc_def->db_info.author)
    g_free (proc_def->db_info.author);
  if (proc_def->db_info.copyright)
    g_free (proc_def->db_info.copyright);
  if (proc_def->db_info.date)
    g_free (proc_def->db_info.date);

  for (i = 0; i < proc_def->db_info.num_args; i++)
    {
      if (proc_def->db_info.args[i].name)
	g_free (proc_def->db_info.args[i].name);
      if (proc_def->db_info.args[i].description)
	g_free (proc_def->db_info.args[i].description);
    }

  for (i = 0; i < proc_def->db_info.num_values; i++)
    {
      if (proc_def->db_info.values[i].name)
	g_free (proc_def->db_info.values[i].name);
      if (proc_def->db_info.values[i].description)
	g_free (proc_def->db_info.values[i].description);
    }

  if (proc_def->db_info.args)
    g_free (proc_def->db_info.args);
  if (proc_def->db_info.values)
    g_free (proc_def->db_info.values);

  if (!data_only)
    g_free (proc_def);
}

static Argument *
plug_in_temp_run (ProcRecord *proc_rec,
		  Argument   *args)
{
  Argument *return_vals;
  PlugIn *plug_in;
  GPProcRun proc_run;
  gint old_recurse;

  return_vals = NULL;

  plug_in = (PlugIn *) proc_rec->exec_method.temporary.plug_in;

  if (plug_in)
    {
      if (plug_in->busy)
	{
	  return_vals = procedural_db_return_args (proc_rec, FALSE);
	  goto done;
	}

      plug_in->busy = TRUE;
      plug_in_push (plug_in);
      proc_run.name = proc_rec->name;
      proc_run.nparams = proc_rec->num_args;
      proc_run.params = plug_in_args_to_params (args, proc_rec->num_args, FALSE);

      if (!gp_temp_proc_run_write (wire_buffer->current_writefd, &proc_run) ||
	  !wire_flush (wire_buffer->current_writefd))
	{
	  return_vals = procedural_db_return_args (proc_rec, FALSE);
	  goto done;
	}
      plug_in_pop ();
      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      old_recurse = plug_in->recurse;
      plug_in->recurse = TRUE;

      gtk_main ();

      return_vals = plug_in_get_current_return_vals (proc_rec);
      plug_in->recurse = old_recurse;
      plug_in->busy = FALSE;
    }

done:
  return return_vals;
}

static Argument*
plug_in_params_to_args (GPParam *params,
			int      nparams,
			int      full_copy)
{
  Argument *args;
  gchar **stringarray;
  guchar *colorarray;
  int count;
  int i, j;

  if (nparams == 0)
    return NULL;

  args = g_new (Argument, nparams);

  for (i = 0; i < nparams; i++)
    {
      args[i].arg_type = params[i].type;

      switch (args[i].arg_type)
	{
	case PDB_INT32:
	  args[i].value.pdb_int = params[i].data.d_int32;
	  break;
	case PDB_INT16:
	  args[i].value.pdb_int = params[i].data.d_int16;
	  break;
	case PDB_INT8:
	  args[i].value.pdb_int = params[i].data.d_int8;
	  break;
	case PDB_FLOAT:
	  args[i].value.pdb_float = params[i].data.d_float;
	  break;
	case PDB_STRING:
	  if (full_copy)
	    args[i].value.pdb_pointer = g_strdup (params[i].data.d_string);
	  else
	    args[i].value.pdb_pointer = params[i].data.d_string;
	  break;
	case PDB_INT32ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint32, count);
	      memcpy (args[i].value.pdb_pointer, params[i].data.d_int32array, count * 4);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int32array;
	    }
	  break;
	case PDB_INT16ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint16, count);
	      memcpy (args[i].value.pdb_pointer, params[i].data.d_int16array, count * 2);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int16array;
	    }
	  break;
	case PDB_INT8ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint8, count);
	      memcpy (args[i].value.pdb_pointer, params[i].data.d_int8array, count);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int8array;
	    }
	  break;
	case PDB_FLOATARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gdouble, count);
	      memcpy (args[i].value.pdb_pointer, params[i].data.d_floatarray, count * 8);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_floatarray;
	    }
	  break;
	case PDB_STRINGARRAY:
	  if (full_copy)
	    {
	      args[i].value.pdb_pointer = g_new (gchar*, args[i-1].value.pdb_int);
	      stringarray = args[i].value.pdb_pointer;

	      for (j = 0; j < args[i-1].value.pdb_int; j++)
		stringarray[j] = g_strdup (params[i].data.d_stringarray[j]);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_stringarray;
	    }
	  break;
	case PDB_COLOR:
	  args[i].value.pdb_pointer = g_new (guchar, 3);
	  colorarray = args[i].value.pdb_pointer;
	  colorarray[0] = params[i].data.d_color.red;
	  colorarray[1] = params[i].data.d_color.green;
	  colorarray[2] = params[i].data.d_color.blue;
	  break;
	case PDB_REGION:
	  g_message (_("the \"region\" arg type is not currently supported"));
	  break;
	case PDB_DISPLAY:
	  args[i].value.pdb_int = params[i].data.d_display;
	  break;
	case PDB_IMAGE:
	  args[i].value.pdb_int = params[i].data.d_image;
	  break;
	case PDB_LAYER:
	  args[i].value.pdb_int = params[i].data.d_layer;
	  break;
	case PDB_CHANNEL:
	  args[i].value.pdb_int = params[i].data.d_channel;
	  break;
	case PDB_DRAWABLE:
	  args[i].value.pdb_int = params[i].data.d_drawable;
	  break;
	case PDB_SELECTION:
	  args[i].value.pdb_int = params[i].data.d_selection;
	  break;
	case PDB_BOUNDARY:
	  args[i].value.pdb_int = params[i].data.d_boundary;
	  break;
	case PDB_PATH:
	  args[i].value.pdb_int = params[i].data.d_path;
	  break;
	case PDB_STATUS:
	  args[i].value.pdb_int = params[i].data.d_status;
	  break;
	case PDB_END:
	  break;
	}
    }

  return args;
}

static GPParam*
plug_in_args_to_params (Argument *args,
			int       nargs,
			int       full_copy)
{
  GPParam *params;
  gchar **stringarray;
  guchar *colorarray;
  int i, j;

  if (nargs == 0)
    return NULL;

  params = g_new (GPParam, nargs);

  for (i = 0; i < nargs; i++)
    {
      params[i].type = args[i].arg_type;

      switch (args[i].arg_type)
	{
	case PDB_INT32:
	  params[i].data.d_int32 = args[i].value.pdb_int;
	  break;
	case PDB_INT16:
	  params[i].data.d_int16 = args[i].value.pdb_int;
	  break;
	case PDB_INT8:
	  params[i].data.d_int8 = args[i].value.pdb_int;
	  break;
	case PDB_FLOAT:
	  params[i].data.d_float = args[i].value.pdb_float;
	  break;
	case PDB_STRING:
	  if (full_copy)
	    params[i].data.d_string = g_strdup (args[i].value.pdb_pointer);
	  else
	    params[i].data.d_string = args[i].value.pdb_pointer;
	  break;
	case PDB_INT32ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int32array = g_new (gint32, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int32array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 4);
	    }
	  else
	    {
	      params[i].data.d_int32array = args[i].value.pdb_pointer;
	    }
	  break;
	case PDB_INT16ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int16array = g_new (gint16, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int16array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 2);
	    }
	  else
	    {
	      params[i].data.d_int16array = args[i].value.pdb_pointer;
	    }
	  break;
	case PDB_INT8ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int8array = g_new (gint8, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int8array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32);
	    }
	  else
	    {
	      params[i].data.d_int8array = args[i].value.pdb_pointer;
	    }
	  break;
	case PDB_FLOATARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_floatarray = g_new (gdouble, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_floatarray,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 8);
	    }
	  else
	    {
	      params[i].data.d_floatarray = args[i].value.pdb_pointer;
	    }
	  break;
	case PDB_STRINGARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_stringarray = g_new (gchar*, params[i-1].data.d_int32);
	      stringarray = args[i].value.pdb_pointer;

	      for (j = 0; j < params[i-1].data.d_int32; j++)
		params[i].data.d_stringarray[j] = g_strdup (stringarray[j]);
	    }
	  else
	    {
	      params[i].data.d_stringarray = args[i].value.pdb_pointer;
	    }
	  break;
	case PDB_COLOR:
	  colorarray = args[i].value.pdb_pointer;
	  if( colorarray )
	    {
	      params[i].data.d_color.red = colorarray[0];
	      params[i].data.d_color.green = colorarray[1];
	      params[i].data.d_color.blue = colorarray[2];
	    }
	  else
	    {
	      params[i].data.d_color.red = 0;
	      params[i].data.d_color.green = 0;
	      params[i].data.d_color.blue = 0;
	    }
	  break;
	case PDB_REGION:
	  g_message (_("the \"region\" arg type is not currently supported"));
	  break;
	case PDB_DISPLAY:
	  params[i].data.d_display = args[i].value.pdb_int;
	  break;
	case PDB_IMAGE:
	  params[i].data.d_image = args[i].value.pdb_int;
	  break;
	case PDB_LAYER:
	  params[i].data.d_layer = args[i].value.pdb_int;
	  break;
	case PDB_CHANNEL:
	  params[i].data.d_channel = args[i].value.pdb_int;
	  break;
	case PDB_DRAWABLE:
	  params[i].data.d_drawable = args[i].value.pdb_int;
	  break;
	case PDB_SELECTION:
	  params[i].data.d_selection = args[i].value.pdb_int;
	  break;
	case PDB_BOUNDARY:
	  params[i].data.d_boundary = args[i].value.pdb_int;
	  break;
	case PDB_PATH:
	  params[i].data.d_path = args[i].value.pdb_int;
	  break;
	case PDB_STATUS:
	  params[i].data.d_status = args[i].value.pdb_int;
	  break;
	case PDB_END:
	  break;
	}
    }

  return params;
}

static void
plug_in_params_destroy (GPParam *params,
			int      nparams,
			int      full_destroy)
{
  int i, j;

  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
	{
	case PDB_INT32:
	case PDB_INT16:
	case PDB_INT8:
	case PDB_FLOAT:
	  break;
	case PDB_STRING:
	  if (full_destroy)
	    g_free (params[i].data.d_string);
	  break;
	case PDB_INT32ARRAY:
	  if (full_destroy)
	    g_free (params[i].data.d_int32array);
	  break;
	case PDB_INT16ARRAY:
	  if (full_destroy)
	    g_free (params[i].data.d_int16array);
	  break;
	case PDB_INT8ARRAY:
	  if (full_destroy)
	    g_free (params[i].data.d_int8array);
	  break;
	case PDB_FLOATARRAY:
	  if (full_destroy)
	    g_free (params[i].data.d_floatarray);
	  break;
	case PDB_STRINGARRAY:
	  if (full_destroy)
	    {
	      for (j = 0; j < params[i-1].data.d_int32; j++)
		g_free (params[i].data.d_stringarray[j]);
	      g_free (params[i].data.d_stringarray);
	    }
	  break;
	case PDB_COLOR:
	  break;
	case PDB_REGION:
	  g_message (_("the \"region\" arg type is not currently supported"));
	  break;
	case PDB_DISPLAY:
	case PDB_IMAGE:
	case PDB_LAYER:
	case PDB_CHANNEL:
	case PDB_DRAWABLE:
	case PDB_SELECTION:
	case PDB_BOUNDARY:
	case PDB_PATH:
	case PDB_STATUS:
	  break;
	case PDB_END:
	  break;
	}
    }

  g_free (params);
}

static void
plug_in_args_destroy (Argument *args,
		      int       nargs,
		      int       full_destroy)
{
  gchar **stringarray;
  int count;
  int i, j;

  for (i = 0; i < nargs; i++)
    {
      switch (args[i].arg_type)
	{
	case PDB_INT32:
	case PDB_INT16:
	case PDB_INT8:
	case PDB_FLOAT:
	  break;
	case PDB_STRING:
	  if (full_destroy)
	    g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_INT32ARRAY:
	  if (full_destroy)
	    g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_INT16ARRAY:
	  if (full_destroy)
	    g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_INT8ARRAY:
	  if (full_destroy)
	    g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_FLOATARRAY:
	  if (full_destroy)
	    g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_STRINGARRAY:
	  if (full_destroy)
	    {
	      count = args[i-1].value.pdb_int;
	      stringarray = args[i].value.pdb_pointer;

	      for (j = 0; j < count; j++)
		g_free (stringarray[j]);

	      g_free (args[i].value.pdb_pointer);
	    }
	  break;
	case PDB_COLOR:
	  g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_REGION:
	  g_message (_("the \"region\" arg type is not currently supported"));
	  break;
	case PDB_DISPLAY:
	case PDB_IMAGE:
	case PDB_LAYER:
	case PDB_CHANNEL:
	case PDB_DRAWABLE:
	case PDB_SELECTION:
	case PDB_BOUNDARY:
	case PDB_PATH:
	case PDB_STATUS:
	  break;
	case PDB_END:
	  break;
	}
    }

  g_free (args);
}

int
plug_in_image_types_parse (char *image_types)
{
  int types;

  if (!image_types)
    return (RGB_IMAGE | GRAY_IMAGE | INDEXED_IMAGE | 
		U16_RGB_IMAGE | U16_GRAY_IMAGE |
		FLOAT_RGB_IMAGE | FLOAT_GRAY_IMAGE |
		FLOAT16_RGB_IMAGE | FLOAT16_GRAY_IMAGE );

  types = 0;

  while (*image_types)
    {
      while (*image_types &&
	     ((*image_types == ' ') ||
	      (*image_types == '\t') ||
	      (*image_types == ',')))
	image_types++;

      if (*image_types)
	{
	  if (strncmp (image_types, "RGBA", 4) == 0)
	    {
	      types |= RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB*", 4) == 0)
	    {
	      types |= RGB_IMAGE | RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB", 3) == 0)
	    {
	      types |= RGB_IMAGE;
	      image_types += 3;
	    }
	  else if (strncmp (image_types, "GRAYA", 5) == 0)
	    {
	      types |= GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY*", 5) == 0)
	    {
	      types |= GRAY_IMAGE | GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY", 4) == 0)
	    {
	      types |= GRAY_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "INDEXEDA", 8) == 0)
	    {
	      types |= INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED*", 8) == 0)
	    {
	      types |= INDEXED_IMAGE | INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED", 7) == 0)
	    {
	      types |= INDEXED_IMAGE;
	      image_types += 7;
	    }
	  else if (strncmp (image_types, "U16_RGBA", 8) == 0)
	    {
	      types |= U16_RGBA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "U16_RGB*", 8) == 0)
	    {
	      types |= U16_RGB_IMAGE | U16_RGBA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "U16_RGB", 7) == 0)
	    {
	      types |= U16_RGB_IMAGE;
	      image_types += 7;
	    }
	  else if (strncmp (image_types, "U16_GRAYA", 9) == 0)
	    {
	      types |= U16_GRAYA_IMAGE;
	      image_types += 9;
	    }
	  else if (strncmp (image_types, "U16_GRAY*", 9) == 0)
	    {
	      types |= U16_GRAY_IMAGE | U16_GRAYA_IMAGE;
	      image_types += 9;
	    }
	  else if (strncmp (image_types, "U16_GRAY", 8) == 0)
	    {
	      types |= U16_GRAY_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "FLOAT_RGBA", 10) == 0)
	    {
	      types |= FLOAT_RGBA_IMAGE;
	      image_types += 10;
	    }
	  else if (strncmp (image_types, "FLOAT_RGB*", 10) == 0)
	    {
	      types |= FLOAT_RGB_IMAGE | FLOAT_RGBA_IMAGE;
	      image_types += 10;
	    }
	  else if (strncmp (image_types, "FLOAT_RGB", 9) == 0)
	    {
	      types |= FLOAT_RGB_IMAGE;
	      image_types += 9;
	    }
	  else if (strncmp (image_types, "FLOAT_GRAYA", 11) == 0)
	    {
	      types |= FLOAT_GRAYA_IMAGE;
	      image_types += 11;
	    }
	  else if (strncmp (image_types, "FLOAT_GRAY*", 11) == 0)
	    {
	      types |= FLOAT_GRAY_IMAGE | FLOAT_GRAYA_IMAGE;
	      image_types += 11;
	    }
	  else if (strncmp (image_types, "FLOAT_GRAY", 10) == 0)
	    {
	      types |= FLOAT_GRAY_IMAGE;
	      image_types += 10;
	    }
	  else if (strncmp (image_types, "FLOAT16_RGBA", 12) == 0)
	    {
	      types |= FLOAT16_RGBA_IMAGE;
	      image_types += 12;
	    }
	  else if (strncmp (image_types, "FLOAT16_RGB*", 12) == 0)
	    {
	      types |= FLOAT16_RGB_IMAGE | FLOAT16_RGBA_IMAGE;
	      image_types += 12;
	    }
	  else if (strncmp (image_types, "FLOAT16_RGB", 11) == 0)
	    {
	      types |= FLOAT16_RGB_IMAGE;
	      image_types += 11;
	    }
	  else if (strncmp (image_types, "FLOAT16_GRAYA", 13) == 0)
	    {
	      types |= FLOAT16_GRAYA_IMAGE;
	      image_types += 13;
	    }
	  else if (strncmp (image_types, "FLOAT16_GRAY*", 13) == 0)
	    {
	      types |= FLOAT16_GRAY_IMAGE | FLOAT16_GRAYA_IMAGE;
	      image_types += 13;
	    }
	  else if (strncmp (image_types, "FLOAT16_GRAY", 12) == 0)
	    {
	      types |= FLOAT16_GRAY_IMAGE;
	      image_types += 12;
	    }
	  else
	    {
	      while (*image_types &&
		     (*image_types != 'R') &&
		     (*image_types != 'G') &&
		     (*image_types != 'I') &&
		     (*image_types != 'U') &&
		     (*image_types != 'F'))
		image_types++;
	    }
	}
    }

  return types;
}

static void
plug_in_progress_cancel (GtkWidget *widget,
			 PlugIn    *plug_in)
{
  plug_in->progress = NULL;
#ifdef WIN32

#else
  plug_in_destroy (plug_in);
#endif
}

static void
plug_in_progress_init (PlugIn *plug_in,
		       char   *message)
{
  GtkWidget *vbox;
  GtkWidget *button;

  if (!message)
    message = plug_in->args[0];

#ifdef SEPARATE_PROGRESS_BAR
  if (!plug_in->progress)
    {
      plug_in->progress = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (plug_in->progress), "plug_in_progress", PROGRAM_NAME);
      gtk_window_set_title (GTK_WINDOW (plug_in->progress), prune_filename (plug_in->args[0]));
      gtk_widget_set_uposition (plug_in->progress, progress_x, progress_y);
      layout_connect_window_position(plug_in->progress, &progress_x, &progress_y, TRUE);
      minimize_register(plug_in->progress);
      gtk_signal_connect (GTK_OBJECT (plug_in->progress), "destroy",
			  (GtkSignalFunc) plug_in_progress_cancel,
			  plug_in);
      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (plug_in->progress)->action_area), 2);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_border_width (GTK_CONTAINER (vbox), 2);
	 gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plug_in->progress)->vbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show (vbox);

      plug_in->progress_label = gtk_label_new (message);
      gtk_misc_set_alignment (GTK_MISC (plug_in->progress_label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), plug_in->progress_label, FALSE, TRUE, 0);
      gtk_widget_show (plug_in->progress_label);

      plug_in->progress_bar = gtk_progress_bar_new ();
      gtk_widget_set_usize (plug_in->progress_bar, 150, 20);
      gtk_box_pack_start (GTK_BOX (vbox), plug_in->progress_bar, TRUE, TRUE, 0);
      gtk_widget_show (plug_in->progress_bar);

      button = gtk_button_new_with_label (_("Cancel"));
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                                 (GtkSignalFunc) gtk_widget_destroy,
                                 GTK_OBJECT (plug_in->progress));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plug_in->progress)->action_area), button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      gtk_widget_show (plug_in->progress);
    }
  else
    {
      gtk_label_set (GTK_LABEL (plug_in->progress_label), message);
    }
#else
  if (!plug_in->progress)
    {
      plug_in->progress = (GtkWidget*) 0x1;
      progress_update (0.0);
      progress_start ();
    }
#endif
}

static void
plug_in_progress_update (PlugIn *plug_in,
			 double  percentage)
{
#ifdef SEPARATE_PROGRESS_BAR
  if (!(percentage >= 0.0 && percentage <= 1.0))
    return;

  if (!plug_in->progress)
    plug_in_progress_init (plug_in, NULL);

  gtk_progress_bar_update (GTK_PROGRESS_BAR (plug_in->progress_bar), percentage);
#else
  progress_update (percentage);
#endif
}

static Argument*
progress_init_invoker (Argument *args)
{
  int success = FALSE;

  if (wire_buffer->current_plug_in && wire_buffer->current_plug_in->open)
    {
      success = TRUE;
      if (no_interface == FALSE)
	plug_in_progress_init (wire_buffer->current_plug_in, args[0].value.pdb_pointer);
    }

  return procedural_db_return_args (&progress_init_proc, success);
}

static Argument*
progress_update_invoker (Argument *args)
{
  int success = FALSE;

  if (wire_buffer->current_plug_in && wire_buffer->current_plug_in->open)
    {
      success = TRUE;
      if (no_interface == FALSE)
	plug_in_progress_update (wire_buffer->current_plug_in, args[0].value.pdb_float);
    }

  return procedural_db_return_args (&progress_update_proc, success);
}

static Argument*
message_invoker (Argument *args)
{
  g_message (args[0].value.pdb_pointer, NULL, NULL);
  return procedural_db_return_args (&message_proc, TRUE);
}

static Argument*
message_handler_get_invoker (Argument *args)
{
  Argument *return_args;

  return_args = procedural_db_return_args (&message_handler_get_proc, TRUE);
  return_args[1].value.pdb_int = message_handler;
  return return_args;
}

static Argument*
message_handler_set_invoker (Argument *args)
{
  int success = TRUE;

  if ((args[0].value.pdb_int >= MESSAGE_BOX) &&
      (args[0].value.pdb_int <= CONSOLE))
    message_handler = args[0].value.pdb_int;
  else
    success = FALSE;

  return procedural_db_return_args (&message_handler_set_proc, success);
}

gint  
tag_to_plugin_image_type (
             Tag t 
            )
{
  Alpha a = tag_alpha (t);
  Precision p = tag_precision (t);
  Format f = tag_format (t);
  
  if ( !tag_valid (t) )
    return -1;
  
  switch (p)
    {
    case PRECISION_U8:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  RGBA_IMAGE: RGB_IMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  GRAYA_IMAGE: GRAY_IMAGE;
        case FORMAT_INDEXED:      
          return (a == ALPHA_YES)?  INDEXEDA_IMAGE: INDEXED_IMAGE;
        default:
          break; 
	} 
      break;
    
    case PRECISION_U16:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  U16_RGBA_IMAGE: U16_RGB_IMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  U16_GRAYA_IMAGE: U16_GRAY_IMAGE;
        default:
          break; 
	} 
      break;
    
    case PRECISION_FLOAT:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  FLOAT_RGBA_IMAGE: FLOAT_RGB_IMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  FLOAT_GRAYA_IMAGE: FLOAT_GRAY_IMAGE;
        default:
          break; 
	} 
      break;

    case PRECISION_FLOAT16:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  FLOAT16_RGBA_IMAGE: FLOAT16_RGB_IMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  FLOAT16_GRAYA_IMAGE: FLOAT16_GRAY_IMAGE;
        default:
          break; 
	} 
      break;

    case PRECISION_BFP:
      switch (f)
	{
        case FORMAT_RGB:
          return (a == ALPHA_YES)?  BFP_RGBA_IMAGE: BFP_RGB_IMAGE;
        case FORMAT_GRAY:
          return (a == ALPHA_YES)?  BFP_GRAYA_IMAGE: BFP_GRAY_IMAGE;
        default:
          break; 
	} 
      break;
    default:
      break;
    }
    return -1;
}
