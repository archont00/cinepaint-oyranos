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

#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wire/datadir.h"

#ifdef WIN32
#include <crtdbg.h>
#include "../win32/config.h"
#include "../win32/extra.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#endif

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "plugin_main.h"
#include "wire/protocol.h"
#include "wire/wire.h"
/* rsr: License problem? Files from app in lib! */
#include "../app/plugin_loader.h"
#include "../app/plug_in.h"
#include "wire/wirebuffer.h"
#include "wire/taskswitch.h"
#include "wire/iodebug.h"
#include "version.h"

/* Rowe: workaround for config.h clash when making win32 and Linux simultaneously. 
   Normally, the Linux config.h should first be removed */
#ifdef WIN32
#undef HAVE_SHM_H
#endif


void gimp_extension_process (guint timeout);
void gimp_extension_ack     (void);

static int gimp_main_                (int   argc, char *argv[]);
static RETSIGTYPE gimp_signal        (int signum);
static void       gimp_loop          (void);
static void       gimp_config        (GPConfig *config);
static void       gimp_proc_run      (GPProcRun *proc_run);
static void       gimp_temp_proc_run (GPProcRun *proc_run);
#if GTK_MAJOR_VERSION > 1
DLL_API void      gimp_message_func  (const gchar   *log_domain,
                                                 GLogLevelFlags log_level,
                                                 const gchar   *message,
                                                 gpointer       user_data);
#else
DLL_API void      gimp_message_func  (char *str);
#endif

#ifdef WIN32
extern int _fmode;
#define _readfd 0
#define _writefd 0
#define _shm_ID 1
#define _shm_addr 0
#else
int _readfd = 0;
int _writefd = 0;
int _shm_ID = -1;
guchar *_shm_addr = NULL;
#endif

const guint gimp_major_version = MAJOR_VERSION;
const guint gimp_minor_version = MINOR_VERSION;
const guint gimp_micro_version = MICRO_VERSION;

int   argc = 0;
char **argv = {""};

static gdouble _gamma_val;
static gint _install_cmap;
static gint _use_xshm;
static guchar _color_cube[4];

static char *progname = NULL;
static guint8 write_buffer[WIRE_BUFFER_SIZE];
static guint write_buffer_index = 0;

static GHashTable *temp_proc_ht = NULL;
static int is_quitting;

GPlugInInfo PLUG_IN_INFO_LIB;

int plugin_main (int argc_, char **argv_, const GPlugInInfo *p)
{
  argc = argc_;
  argv = argv_;

# ifdef DEBUG
  printf("%s:%d\n",__FILE__,__LINE__);
# endif
  { /* allow for plug-in debugging */
    int dbg = 0;
    if(!getenv("CP_DBG_PLUG_INS") &&
       getenv("CP_DBG_PLUG_IN"))
    {
      const char *plugin_name = getenv("CP_DBG_PLUG_IN");
      if(plugin_name)
      {
        if( strstr( argv[0], plugin_name ) != NULL )
          dbg = 1;
      } else
        dbg = 1;
    }
    if(dbg)
    {
      fprintf(stderr, "%s:%d %s() running %s in GDB:  attach ",
              __FILE__, __LINE__, __func__, strrchr(argv[0],'/') );
      system("echo $PPID");
    }
    while(dbg)
    {
      sleep(1);
      ++dbg; if(dbg > 10) dbg = 0;
    }
  }

# if GTK_MAJOR_VERSION > 1
  /* What better idea to prevent gtk2 from taking hands on locale? beku */
  setlocale(LC_NUMERIC, getenv("LC_NUMERIC"));
# ifdef DEBUG
  {
    char *p[3];
    p[0] = getenv("LC_NUMERIC");
    p[1] = setlocale(LC_NUMERIC,NULL);
    p[2] = getenv("TEXTDOMAINDIR");
    printf("%s:%d %s() %s %s %s\n",__FILE__,__LINE__,__func__,
      p[0]?p[0]:"", p[1]?p[1]:"", p[2]?p[2]:"");
  }
# endif
# endif

  gtk_init (&argc, &argv);

  set_gimp_PLUG_IN_INFO(p);
  return gimp_main_ (argc, argv);
}

void set_gimp_PLUG_IN_INFO (const GPlugInInfo *p)
{
  PLUG_IN_INFO_LIB = *p;
}

int
gimp_main_ (int   argc,
	    char *argv[])
{
    if ((argc < 4) || (strcmp (argv[1], "-gimp") != 0))
    {
      g_print ("%s is a gimp plug-in and must be run by the gimp to be used\n", argv[0]);
      return 1;
    }

	progname = argv[0];
#ifdef WIN32
	set_gimp_PLUG_IN_INFO(wire_buffer->plugin->PLUG_IN_INFO_LIB);
	_fmode = _O_BINARY;
#ifdef _DEBUG
/* This creates a console window and joins it to a GUI process: */
/*	AllocConsole();*/
	freopen("CONIN$","rb",stdin);
	freopen("CONOUT$","wb",stdout);
	freopen("CONOUT$","wb",stderr);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
#else
	puts(progname);
  signal (SIGHUP, gimp_signal);
  signal (SIGINT, gimp_signal);
  signal (SIGQUIT, gimp_signal);
  signal (SIGBUS, gimp_signal);
  signal (SIGSEGV, gimp_signal);
  signal (SIGPIPE, gimp_signal);
  signal (SIGTERM, gimp_signal);
  signal (SIGFPE, gimp_signal);

  _readfd = atoi (argv[2]);
  _writefd = atoi (argv[3]);
#endif
  gp_init ();
#ifdef WIN32
  wire_set_reader(wire_memory_read);
  wire_set_writer(wire_memory_write);
  wire_set_flusher(wire_memory_flush);

  if ((strcmp (argv[4], "-query") == 0))
#else
  wire_set_reader(wire_file_read);
  wire_set_writer(wire_file_write);
  wire_set_flusher(wire_file_flush);

  if ((argc == 5) && (strcmp (argv[4], "-query") == 0))
#endif
    {
	if (PLUG_IN_INFO_LIB.query_proc)
        {
		(* PLUG_IN_INFO_LIB.query_proc)();
	}
	else
	{	e_puts("ERROR: PLUG_IN_INFO_LIB.query_proc is <null>");
	}
	gimp_quit ();
	return 0;
    }

#if GTK_MAJOR_VERSION > 1
  g_log_set_handler("Gdk",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);

  g_log_set_handler("Gtk",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
		    
  g_log_set_handler("GLib",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
		    
  g_log_set_handler("GLib-GObject",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
		    
  g_log_set_handler("GModule",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
		    
  g_log_set_handler("GThread",
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
  g_log_set_handler(NULL,
		    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
		    gimp_message_func, NULL);
#else
  g_set_message_handler ((GPrintFunc) gimp_message_func);
#endif

  temp_proc_ht = g_hash_table_new (&g_str_hash, &g_str_equal);
	is_quitting=0;
  gimp_loop ();
  return 0;
}

void G_GNUC_NORETURN
gimp_quit ()
{
  if (PLUG_IN_INFO_LIB.quit_proc)
    (* PLUG_IN_INFO_LIB.quit_proc) ();

#ifdef HAVE_SHM_H
  if ((_shm_ID != -1) && _shm_addr)
    shmdt ((char*) _shm_addr);
#endif
	d_puts(" gimp_quit");
  gp_quit_write (_writefd);
/* Don't: TaskSwitchToWire();*/
#ifdef WIN32
  is_quitting=1;
#else
  exit (0);
#endif
}

void
gimp_set_data (gchar *  id,
	       gpointer data,
	       guint32  length)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_procedural_db_set_data",
				    &nreturn_vals,
				    PARAM_STRING, id,
				    PARAM_INT32, length,
				    PARAM_INT8ARRAY, data,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_get_data (gchar *  id,
	       gpointer data)
{
  GParam *return_vals;
  int nreturn_vals;
  int length;
  gchar *returned_data;

  return_vals = gimp_run_procedure ("gimp_procedural_db_get_data",
				    &nreturn_vals,
				    PARAM_STRING, id,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      length = return_vals[1].data.d_int32;
      returned_data = (gchar *) return_vals[2].data.d_int8array;

      memcpy (data, returned_data, length);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_message (const char *message)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_message",
				    &nreturn_vals,
				    PARAM_STRING, message,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

#if GTK_MAJOR_VERSION > 1
void
gimp_message_func (const gchar    *log_domain,
		   GLogLevelFlags  log_level,
		   const gchar    *str,
		   gpointer        data)
#else
void
gimp_message_func (char *str)
#endif
{
  gimp_message (str);
}

void
gimp_query_database (char   *name_regexp,
		     char   *blurb_regexp,
		     char   *help_regexp,
		     char   *author_regexp,
		     char   *copyright_regexp,
		     char   *date_regexp,
		     char   *proc_type_regexp,
		     int    *nprocs,
		     char ***proc_names)
{
  GParam *return_vals;
  int nreturn_vals;
  int i;

  return_vals = gimp_run_procedure ("gimp_procedural_db_query",
				    &nreturn_vals,
				    PARAM_STRING, name_regexp,
				    PARAM_STRING, blurb_regexp,
				    PARAM_STRING, help_regexp,
				    PARAM_STRING, author_regexp,
				    PARAM_STRING, copyright_regexp,
				    PARAM_STRING, date_regexp,
				    PARAM_STRING, proc_type_regexp,
				    PARAM_END);

  *nprocs = 0;
  *proc_names = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nprocs = return_vals[1].data.d_int32;
      *proc_names = g_new (gchar*, *nprocs);

      for (i = 0; i < *nprocs; i++)
	(*proc_names)[i] = g_strdup (return_vals[2].data.d_stringarray[i]);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint
gimp_query_procedure (char      *proc_name,
		      char     **proc_blurb,
		      char     **proc_help,
		      char     **proc_author,
		      char     **proc_copyright,
		      char     **proc_date,
		      int       *proc_type,
		      int       *nparams,
		      int       *nreturn_vals,
		      GParamDef **params,
		      GParamDef **return_vals)
{
  GParam *ret_vals;
  int nret_vals;
  int i;
  int success = TRUE;

  ret_vals = gimp_run_procedure ("gimp_procedural_db_proc_info",
				 &nret_vals,
				 PARAM_STRING, proc_name,
				 PARAM_END);

  if (ret_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *proc_blurb = g_strdup (ret_vals[1].data.d_string);
      *proc_help = g_strdup (ret_vals[2].data.d_string);
      *proc_author = g_strdup (ret_vals[3].data.d_string);
      *proc_copyright = g_strdup (ret_vals[4].data.d_string);
      *proc_date = g_strdup (ret_vals[5].data.d_string);
      *proc_type = ret_vals[6].data.d_int32;
      *nparams = ret_vals[7].data.d_int32;
      *nreturn_vals = ret_vals[8].data.d_int32;
      *params = g_new (GParamDef, *nparams);
      *return_vals = g_new (GParamDef, *nreturn_vals);

      for (i = 0; i < *nparams; i++)
	{
	  GParam *rvals;
	  int nrvals;

	  rvals = gimp_run_procedure ("gimp_procedural_db_proc_arg",
				      &nrvals,
				      PARAM_STRING, proc_name,
				      PARAM_INT32, i,
				      PARAM_END);

	  if (rvals[0].data.d_status == STATUS_SUCCESS)
	    {
	      (*params)[i].type = rvals[1].data.d_int32;
	      (*params)[i].name = g_strdup (rvals[2].data.d_string);
	      (*params)[i].description = g_strdup (rvals[3].data.d_string);
	    }
	  else
	    {
	      g_free (*params);
	      g_free (*return_vals);
	      gimp_destroy_params (rvals, nrvals);
	      return FALSE;
	    }

	  gimp_destroy_params (rvals, nrvals);
	}

      for (i = 0; i < *nreturn_vals; i++)
	{
	  GParam *rvals;
	  int nrvals;

	  rvals = gimp_run_procedure ("gimp_procedural_db_proc_val",
				      &nrvals,
				      PARAM_STRING, proc_name,
				      PARAM_INT32, i,
				      PARAM_END);

	  if (rvals[0].data.d_status == STATUS_SUCCESS)
	    {
	      (*return_vals)[i].type = rvals[1].data.d_int32;
	      (*return_vals)[i].name = g_strdup (rvals[2].data.d_string);
	      (*return_vals)[i].description = g_strdup (rvals[3].data.d_string);
	    }
	  else
	    {
	      g_free (*params);
	      g_free (*return_vals);
	      gimp_destroy_params (rvals, nrvals);
	      return FALSE;
	    }

	  gimp_destroy_params (rvals, nrvals);
	}
    }
  else
    success = FALSE;

  gimp_destroy_params (ret_vals, nret_vals);

  return success;
}

gint32*
gimp_query_images (int *nimages)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 *images;

  return_vals = gimp_run_procedure ("gimp_list_images",
				    &nreturn_vals,
				    PARAM_END);

  images = NULL;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nimages = return_vals[1].data.d_int32;
      images = g_new (gint32, *nimages);
      memcpy (images, return_vals[2].data.d_int32array, *nimages * 4);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return images;
}

void
gimp_install_procedure (char     *name,
			char     *blurb,
			char     *help,
			char     *author,
			char     *copyright,
			char     *date,
			char     *menu_path,
			char     *image_types,
			int       type,
			int       nparams,
			int       nreturn_vals,
			GParamDef *params,
			GParamDef *return_vals)
{
  GPProcInstall proc_install;

  proc_install.name = name;
  proc_install.blurb = blurb;
  proc_install.help = help;
  proc_install.author = author;
  proc_install.copyright = copyright;
  proc_install.date = date;
  proc_install.menu_path = menu_path;
  proc_install.image_types = image_types;
  proc_install.type = type;
  proc_install.nparams = nparams;
  proc_install.nreturn_vals = nreturn_vals;
  proc_install.params = (GPParamDef*) params;
  proc_install.return_vals = (GPParamDef*) return_vals;
  if (!gp_proc_install_write (_writefd, &proc_install))
    gimp_quit ();
  TaskSwitchToWire();
}

void
gimp_install_temp_proc (char     *name,
			char     *blurb,
			char     *help,
			char     *author,
			char     *copyright,
			char     *date,
			char     *menu_path,
			char     *image_types,
			int       type,
			int       nparams,
			int       nreturn_vals,
			GParamDef *params,
			GParamDef *return_vals,
			GRunProc   run_proc)
{
  gimp_install_procedure (name, blurb, help, author, copyright, date,
			  menu_path, image_types, type,
			  nparams, nreturn_vals, params, return_vals);

  /*  Insert the temp proc run function into the hash table  */
  g_hash_table_insert (temp_proc_ht, (gpointer) name, (gpointer) run_proc);
}

void
gimp_uninstall_temp_proc (char *name)
{
  GPProcUninstall proc_uninstall;

  proc_uninstall.name = name;
  if (!gp_proc_uninstall_write (_writefd, &proc_uninstall))
    gimp_quit ();
  g_hash_table_remove (temp_proc_ht, (gpointer) name);
}

void
gimp_register_magic_load_handler (char *name,
				  char *extensions,
				  char *prefixes,
				  char *magics)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_magic_load_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_STRING, magics,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_register_load_handler (char *name,
			    char *extensions,
			    char *prefixes)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_load_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_register_save_handler (char *name,
			    char *extensions,
			    char *prefixes)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_register_save_handler",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_STRING, extensions,
				    PARAM_STRING, prefixes,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

GParam*
gimp_run_procedure (char *name,
		    int  *nreturn_vals,
		    ...)
{
  GPProcRun proc_run;
  GPProcReturn *proc_return;
  WireMessage msg;
  GParamType param_type;
  GParam *return_vals;
  va_list args;
  guchar *color;
  unsigned i;
  d_printf("gimp_run_procedure: %s\n",name);

  proc_run.name = name;
  proc_run.nparams = 0;
  proc_run.params = NULL;

  va_start (args, nreturn_vals);
  param_type = va_arg (args, GParamType);

  while (param_type != PARAM_END)
    {
      switch (param_type)
	{
	case PARAM_INT32:
        case PARAM_DISPLAY:
        case PARAM_IMAGE:
        case PARAM_LAYER:
        case PARAM_CHANNEL:
        case PARAM_DRAWABLE:
        case PARAM_SELECTION:
        case PARAM_BOUNDARY:
        case PARAM_PATH:
        case PARAM_STATUS:
	  (void) va_arg (args, int);
	  break;
	case PARAM_INT16:
	  (void) va_arg (args, int);
	  break;
	case PARAM_INT8:
	  (void) va_arg (args, int);
	  break;
        case PARAM_FLOAT:
          (void) va_arg (args, double);
          break;
        case PARAM_STRING:
          (void) va_arg (args, gchar*);
          break;
        case PARAM_INT32ARRAY:
          (void) va_arg (args, gint32*);
          break;
        case PARAM_INT16ARRAY:
          (void) va_arg (args, gint16*);
          break;
        case PARAM_INT8ARRAY:
          (void) va_arg (args, gint8*);
          break;
        case PARAM_FLOATARRAY:
          (void) va_arg (args, gdouble*);
          break;
        case PARAM_STRINGARRAY:
          (void) va_arg (args, gchar**);
          break;
        case PARAM_COLOR:
          (void) va_arg (args, guchar*);
          break;
        case PARAM_REGION:
          break;
	case PARAM_END:
	  break;
	}

      proc_run.nparams += 1;
      param_type = va_arg (args, GParamType);
    }

  va_end (args);

  proc_run.params = g_new (GPParam, proc_run.nparams);

  va_start (args, nreturn_vals);

  for (i = 0; i < proc_run.nparams; i++)
    {
      proc_run.params[i].type = va_arg (args, GParamType);

      switch (proc_run.params[i].type)
	{
	case PARAM_INT32:
	  proc_run.params[i].data.d_int32 = (gint32) va_arg (args, int);
	  break;
	case PARAM_INT16:
	  proc_run.params[i].data.d_int16 = (gint16) va_arg (args, int);
	  break;
	case PARAM_INT8:
	  proc_run.params[i].data.d_int8 = (gint8) va_arg (args, int);
	  break;
        case PARAM_FLOAT:
          proc_run.params[i].data.d_float = (gdouble) va_arg (args, double);
          break;
        case PARAM_STRING:
          proc_run.params[i].data.d_string = va_arg (args, gchar*);
          break;
        case PARAM_INT32ARRAY:
          proc_run.params[i].data.d_int32array = va_arg (args, gint32*);
          break;
        case PARAM_INT16ARRAY:
          proc_run.params[i].data.d_int16array = va_arg (args, gint16*);
          break;
        case PARAM_INT8ARRAY:
          proc_run.params[i].data.d_int8array = va_arg (args, gint8*);
          break;
        case PARAM_FLOATARRAY:
          proc_run.params[i].data.d_floatarray = va_arg (args, gdouble*);
          break;
        case PARAM_STRINGARRAY:
          proc_run.params[i].data.d_stringarray = va_arg (args, gchar**);
          break;
        case PARAM_COLOR:
	  color = va_arg (args, guchar*);
          proc_run.params[i].data.d_color.red = color[0];
          proc_run.params[i].data.d_color.green = color[1];
          proc_run.params[i].data.d_color.blue = color[2];
          break;
        case PARAM_REGION:
          break;
        case PARAM_DISPLAY:
	  proc_run.params[i].data.d_display = va_arg (args, gint32);
          break;
        case PARAM_IMAGE:
	  proc_run.params[i].data.d_image = va_arg (args, gint32);
          break;
        case PARAM_LAYER:
	  proc_run.params[i].data.d_layer = va_arg (args, gint32);
          break;
        case PARAM_CHANNEL:
	  proc_run.params[i].data.d_channel = va_arg (args, gint32);
          break;
        case PARAM_DRAWABLE:
	  proc_run.params[i].data.d_drawable = va_arg (args, gint32);
          break;
        case PARAM_SELECTION:
	  proc_run.params[i].data.d_selection = va_arg (args, gint32);
          break;
        case PARAM_BOUNDARY:
	  proc_run.params[i].data.d_boundary = va_arg (args, gint32);
          break;
        case PARAM_PATH:
	  proc_run.params[i].data.d_path = va_arg (args, gint32);
          break;
        case PARAM_STATUS:
	  proc_run.params[i].data.d_status = va_arg (args, gint32);
          break;
	case PARAM_END:
	  break;
	}
    }

  va_end (args);

  if (!gp_proc_run_write (_writefd, &proc_run))
  {  g_error("ERROR: gp_proc_run_write failed");
	gimp_quit ();
  }
    TaskSwitchToWire();
  if (!wire_read_msg(_readfd, &msg))
  { g_error("ERROR: wire_read_msg failed");
    gimp_quit ();
  }
  if (msg.type != GP_PROC_RETURN)
  {  g_error ("unexpected message[1]: %d %s\n", msg.type,Get_gp_name(msg.type));
  }
  proc_return = msg.data;
  *nreturn_vals = proc_return->nparams;
  return_vals = (GParam*) proc_return->params;

  switch (return_vals[0].data.d_status)
    {
    case STATUS_EXECUTION_ERROR:
      /*g_warning ("an execution error occured while trying to run: \"%s\"", name);*/
      break;
    case STATUS_CALLING_ERROR:
      g_warning ("a calling error occured while trying to run: \"%s\"", name);
      break;
    default:
      break;
    }

  g_free (proc_run.params);
  g_free (proc_return->name);
  g_free (proc_return);

  return return_vals;
}

GParam*
gimp_run_procedure2 (char   *name,
		     int    *nreturn_vals,
		     int     nparams,
		     GParam *params)
{
  GPProcRun proc_run;
  GPProcReturn *proc_return;
  WireMessage msg;
  GParam *return_vals;
  proc_run.name = name;
  proc_run.nparams = nparams;
  proc_run.params = (GPParam *) params;

  if (!gp_proc_run_write (_writefd, &proc_run))
    gimp_quit ();
	TaskSwitchToWire();
  if(!wire_read_msg(_readfd, &msg))
    gimp_quit ();

  if (msg.type != GP_PROC_RETURN)
  {  g_error ("unexpected message[2]: %d %s\n", msg.type,Get_gp_name(msg.type));
  }
  proc_return = msg.data;
  *nreturn_vals = proc_return->nparams;
  return_vals = (GParam*) proc_return->params;

  switch (return_vals[0].data.d_status)
    {
    case STATUS_EXECUTION_ERROR:

      /*g_warning ("an execution error occured while trying to run: \"%s\"", name);*/
      break;
    case STATUS_CALLING_ERROR:
      g_warning ("a calling error occured while trying to run: \"%s\"", name);
      break;
    default:
      break;
    }

  g_free (proc_return);

  return return_vals;
}

void
gimp_destroy_params (GParam *params,
		     int    nparams)
{
  extern void _gp_params_destroy (GPParam *params, int nparams);

  _gp_params_destroy ((GPParam*) params, nparams);
}

gdouble
gimp_gamma ()
{
  return _gamma_val;
}

gint
gimp_install_cmap ()
{
  return _install_cmap;
}

gint
gimp_use_xshm ()
{
  return _use_xshm;
}

guchar*
gimp_color_cube ()
{
  return _color_cube;
}

gchar*
gimp_gtkrc ()
{
  static char filename[MAXPATHLEN];
  const char *home_dir;

  home_dir = GetDirHome();
  if (!home_dir)
    return NULL;

  sprintf (filename, "%s/%s/gtkrc", home_dir, GetDirDot());

  return filename;
}

void
gimp_extension_process (guint timeout)
{
  WireMessage msg;
  fd_set readfds;
  int select_val;
  struct timeval tv;
  struct timeval *tvp;
  if (timeout)
    {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;
      tvp = &tv;
    }
  else
    tvp = NULL;

  FD_ZERO (&readfds);
#ifdef WIN32

#else
  FD_SET (_readfd, &readfds);
#endif
  if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
    {/*	TaskSwitchToWire();*/
    if (!wire_read_msg(_readfd, &msg))
	gimp_quit ();
      switch (msg.type)
	{
	case GP_QUIT:
	  gimp_quit ();
	  break;
	case GP_CONFIG:
	  gimp_config (msg.data);
	  break;
	case GP_TILE_REQ:
	case GP_TILE_ACK:
	case GP_TILE_DATA:
	  g_warning ("unexpected tile message received (should not happen)\n");
	  break;
	case GP_PROC_RUN:
	  g_warning ("unexpected proc run message received (should not happen)\n");
	  break;
	case GP_PROC_RETURN:
	  g_warning ("unexpected proc return message received (should not happen)\n");
	  break;
	case GP_TEMP_PROC_RUN:
	  gimp_temp_proc_run (msg.data);
	  break;
	case GP_TEMP_PROC_RETURN:
	  g_warning ("unexpected temp proc return message received (should not happen)\n");
	  break;
	case GP_PROC_INSTALL:
	  g_warning ("unexpected proc install message received (should not happen)\n");
	  break;
	}

      wire_destroy (&msg);
    }
  else if (select_val == -1)
    {
      perror ("gimp_process");
      gimp_quit ();
    }
}

void
gimp_extension_ack ()
{
  /*  Send an extension initialization acknowledgement  */
  if (! gp_extension_ack_write (_writefd))
    gimp_quit ();
}

static RETSIGTYPE
gimp_signal (int signum)
{
  static int caught_fatal_sig = 0;

  if (caught_fatal_sig)
    kill (getpid (), signum);
  caught_fatal_sig = 1;

  fprintf (stderr, "\n%s: %s caught\n", progname, g_strsignal (signum));

  switch (signum)
    {
    case SIGBUS:
    case SIGSEGV:
    case SIGFPE:
      g_on_error_query (progname);
      break;
    default:
      break;
    }

  gimp_quit ();
}

static void
gimp_loop ()
{  /* listening to a plug-in */
  WireMessage msg;

  while(!is_quitting)
    {
      if (!wire_read_msg (_readfd, &msg))
		gimp_quit ();
      switch (msg.type)
	{
	case GP_QUIT:
	  gimp_quit ();
	  break;
	case GP_CONFIG:
	  gimp_config (msg.data);
	  break;
	case GP_TILE_REQ:
	case GP_TILE_ACK:
	case GP_TILE_DATA:
	  g_warning ("unexpected tile message received (should not happen)\n");
	  break;
	case GP_PROC_RUN:
	  gimp_proc_run (msg.data);
	  gimp_quit ();
	  break;
	case GP_PROC_RETURN:
	  g_warning ("unexpected proc return message received (should not happen)\n");
	  break;
	case GP_TEMP_PROC_RUN:
	  g_warning ("unexpected temp proc run message received (should not happen\n");
	  break;
	case GP_TEMP_PROC_RETURN:
	  g_warning ("unexpected temp proc return message received (should not happen\n");
	  break;
	case GP_PROC_INSTALL:
	  g_warning ("unexpected proc install message received (should not happen)\n");
	  break;
	}

      wire_destroy (&msg);
    }
}

static void
gimp_config (GPConfig *config)
{
  if (config->version < GP_VERSION)
    {
      g_message ("%s: the gimp is using an older version of the "
		 "plug-in protocol than this plug-in\n", progname);
      gimp_quit ();
    }
  else if (config->version > GP_VERSION)
    {
      g_message ("%s: the gimp is using a newer version of the "
		 "plug-in protocol than this plug-in\n", progname);
      gimp_quit ();
    }

  lib_tile_width = config->tile_width;
  lib_tile_height = config->tile_height;
#ifdef WIN32

#else
  _shm_ID = config->shm_ID;
#endif
  _gamma_val = config->gamma;
  _install_cmap = config->install_cmap;
  _use_xshm = config->use_xshm;
  _color_cube[0] = config->color_cube[0];
  _color_cube[1] = config->color_cube[1];
  _color_cube[2] = config->color_cube[2];
  _color_cube[3] = config->color_cube[3];

#ifdef HAVE_SHM_H
  if (_shm_ID != -1)
    {
      _shm_addr = (guchar*) shmat (_shm_ID, 0, 0);

      if (_shm_addr == (guchar*) -1)
	g_error ("could not attach to gimp shared memory segment\n");
    }
#endif
}

static void
gimp_proc_run (GPProcRun *proc_run)
{
  GPProcReturn proc_return;
  GParam *return_vals;
  int nreturn_vals;
  if (PLUG_IN_INFO_LIB.run_proc)
    {
      (* PLUG_IN_INFO_LIB.run_proc) (proc_run->name,
				 proc_run->nparams,
				 (GParam*) proc_run->params,
				 &nreturn_vals,
				 &return_vals);

      proc_return.name = proc_run->name;
      proc_return.nparams = nreturn_vals;
      proc_return.params = (GPParam*) return_vals;
      if (!gp_proc_return_write (_writefd, &proc_return))
		gimp_quit ();
    }
}

static void
gimp_temp_proc_run (GPProcRun *proc_run)
{
  GPProcReturn proc_return;
  GParam *return_vals;
  int nreturn_vals;
  GRunProc run_proc;

  run_proc = (GRunProc)g_hash_table_lookup (temp_proc_ht, (gpointer) proc_run->name);

  if (run_proc)
    {
      (* run_proc) (proc_run->name,
		    proc_run->nparams,
		    (GParam*) proc_run->params,
		    &nreturn_vals,
		    &return_vals);

      proc_return.name = proc_run->name;
      proc_return.nparams = nreturn_vals;
      proc_return.params = (GPParam*) return_vals;
      if (!gp_temp_proc_return_write (_writefd, &proc_return))
		gimp_quit ();
    }
}

