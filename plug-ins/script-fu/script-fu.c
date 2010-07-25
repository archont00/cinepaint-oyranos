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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "siod.h"
#include "script-fu-console.h"
#include "script-fu-constants.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"

#include "script-fu-intl.h"

extern FILE *siod_output;

/* External functions
 */
extern void  gimp_extension_process (guint timeout);
extern void  gimp_extension_ack     (void);

extern void  init_subrs   (void);
extern void  init_trace   (void);
extern void  init_regex   (void);

extern long  nlength      (LISP   obj);
extern LISP  leval_define (LISP   args,
			   LISP   env);

extern void  fput_st      (FILE  *f,
			   gchar *st);

/* Declare local functions.
 */
static void  sfquit (void);
static void  query  (void);
static void  run    (gchar        *name,
		     gint          nparams,
		     GParam    *param,
		     gint         *nreturn_vals,
		     GParam   **return_vals);

static gint  init_interp              (void);
static void  init_gimp                (void);
static void  init_procedures          (void);
static void  init_constants           (void);
static void  convert_string           (gchar *str);

static gint  sputs_fcn                (gchar *st,
				       void  *dest);
static LISP  lprin1s                  (LISP   exp,
				       gchar *dest);

static LISP  marshall_proc_db_call    (LISP a);
static LISP  script_fu_register_call  (LISP a);
static LISP  script_fu_quit_call      (LISP a);

static void  script_fu_auxillary_init (void);
static void  script_fu_refresh_proc   (gchar      *name,
				       gint        nparams,
				       GParam  *params,
				       gint       *nreturn_vals,
				       GParam **return_vals);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  sfquit, /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

static gchar *siod_argv[] =
{
  "siod",
  "-h100000:10",
  "-g0",
  "-o1000",
  "-s200000",
  "-n2048",
  "-v0",
};

static gint script_fu_base = TRUE;
extern gint server_mode;

gint script_fu_done = FALSE;  /*  declared extern in script-fu-server.c  */


MAIN ()

static void
sfquit (void)
{
}

static void
query (void)
{
  static GParamDef console_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode", "Interactive, [non-interactive]" }
  };
  static gint nconsole_args = sizeof (console_args) / sizeof (console_args[0]);

  static GParamDef eval_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode", "[Interactive], non-interactive" },
    { GIMP_PDB_STRING, "code",     "The code to evaluate" }
  };
  static gint neval_args = sizeof (eval_args) / sizeof (eval_args[0]);

  static GParamDef server_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode", "[Interactive], non-interactive" },
    { GIMP_PDB_INT32,  "port",     "The port on which to listen for requests" },
    { GIMP_PDB_STRING, "logfile",  "The file to log server activity to" }
  };
  static gint nserver_args = sizeof (server_args) / sizeof (server_args[0]);

  gimp_plugin_domain_register ("gimp-script-fu", NULL);

  gimp_install_procedure ("extension_script_fu",
			  "A scheme interpreter for scripting GIMP operations",
			  "More help here later",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  NULL,
			  NULL,
			  GIMP_EXTENSION,
			  0, 0, NULL, NULL);

  gimp_install_procedure ("extension_script_fu_console",
			  "Provides a console mode for script-fu development",
			  "Provides an interface which allows interactive scheme development.",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Scripts/Script-Fu/Console",
			  /*N_("<Toolbox>/Xtns/Script-Fu/Console..."),*/
			  NULL,
			  GIMP_EXTENSION,
			  nconsole_args, 0,
			  console_args, NULL);

#ifndef G_OS_WIN32
  gimp_install_procedure ("extension_script_fu_server",
			  "Provides a server for remote script-fu operation",
			  "Provides a server for remote script-fu operation",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Scripts/Script-Fu/Server",
			  /*N_("<Toolbox>/Xtns/Script-Fu/Server..."),*/
			  NULL,
			  GIMP_EXTENSION,
			  nserver_args, 0,
			  server_args, NULL);
#endif

  gimp_install_procedure ("extension_script_fu_eval",
			  "Evaluate scheme code",
			  "Evaluate the code under the scheme interpeter (primarily for batch mode)",
			  "Manish Singh",
			  "Manish Singh",
			  "1998",
			  NULL,
			  NULL,
			  GIMP_EXTENSION,
			  neval_args, 0,
			  eval_args, NULL);
}

static void
run (gchar      *name,
     gint        nparams,
     GParam  *param,
     gint       *nreturn_vals,
     GParam **return_vals)
{
  siod_output = stdout;

  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension_script_fu") == 0)
    {
      /*  Setup auxillary temporary procedures for the base extension  */
      script_fu_auxillary_init ();

      script_fu_base = TRUE;
    }
  else
    script_fu_base = FALSE;

  /*  Init the interpreter  */
  init_interp ();

  /*  Load all of the available scripts  */
  script_fu_find_scripts ();

  /*
   *  The main, automatically installed script fu extension.
   *  For things like logos and effects that are runnable from GIMP menus.
   */
  if (strcmp (name, "extension_script_fu") == 0)
    {
      static GParam  values[1];
      GimpPDBStatusType status = GIMP_PDB_SUCCESS;

      /*  Acknowledge that the extension is properly initialized  */
      gimp_extension_ack ();

      while (TRUE)
	gimp_extension_process (0);

      *nreturn_vals = 1;
      *return_vals  = values;

      values[0].type          = GIMP_PDB_STATUS;
      values[0].data.d_status = status;
    }
  /*
   *  The script-fu console for interactive SIOD development
   */
  else if (strcmp (name, "extension_script_fu_console") == 0)
    {
      script_fu_console_run (name, nparams, param, nreturn_vals, return_vals);
    }
#ifndef G_OS_WIN32
  /*
   *  The script-fu server for remote operation
   */
  else if (strcmp (name, "extension_script_fu_server") == 0)
    {
      script_fu_server_run (name, nparams, param, nreturn_vals, return_vals);
    }
#endif
  /*
   *  A non-interactive "console" (for batch mode)
   */
  else if (strcmp (name, "extension_script_fu_eval") == 0)
    {
      script_fu_eval_run (name, nparams, param, nreturn_vals, return_vals);
    }
}

static gint
init_interp (void)
{
  process_cla (sizeof (siod_argv) / sizeof (char *), siod_argv, 1);

  init_storage ();

  init_subrs ();
  init_trace ();
  init_regex ();
  init_gimp  ();

  return 0;
}

static void
init_gimp (void)
{
  init_procedures ();
  init_constants ();
}

static void
init_procedures (void)
{
  gchar          **proc_list;
  gchar           *proc_name;
  gchar           *arg_name;
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  int /*GimpPDBProcType*/  proc_type;
  gint             nparams;
  gint             nreturn_vals;
  GParamDef    *params;
  GParamDef    *return_vals;
  gint             num_procs;
  gint             i;

  /*  register the database execution procedure  */
  init_lsubr ("gimp-proc-db-call",  marshall_proc_db_call);
  init_lsubr ("script-fu-register", script_fu_register_call);
  init_lsubr ("script-fu-quit",     script_fu_quit_call);

  gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			    &num_procs, &proc_list);

  /*  Register each procedure as a scheme func  */
  for (i = 0; i < num_procs; i++)
    {
      proc_name = g_strdup (proc_list[i]);

      /*  lookup the procedure  */
      if (gimp_query_procedure (proc_name, 
					&proc_blurb, 
					&proc_help, 
					&proc_author,
					&proc_copyright, 
					&proc_date, 
					&proc_type, 
					&nparams, &nreturn_vals,
					&params, &return_vals))
	{
	  LISP args = NIL;
	  LISP code = NIL;
	  gint j;

	  /*  convert the names to scheme-like naming conventions  */
	  convert_string (proc_name);

	  /*  create a new scheme func that calls gimp-proc-db-call  */
	  for (j = 0; j < nparams; j++)
	    {
	      arg_name = g_strdup (params[j].name);
	      convert_string (arg_name);
	      args = cons (cintern (arg_name), args);
	      code = cons (cintern (arg_name), code);
	    }

	  /*  reverse the list  */
	  args = nreverse (args);
	  code = nreverse (code);

	  /*  set the scheme-based procedure name  */
	  args = cons (cintern (proc_name), args);

	  /*  set the acture pdb procedure name  */
	  code = cons (cons (cintern ("quote"), 
			     cons (cintern (proc_list[i]), NIL)), 
		       code);
	  code = cons (cintern ("gimp-proc-db-call"), code);

	  leval_define (cons (args, cons (code, NIL)), NIL);

	  /*  free the queried information  */
	  g_free (proc_blurb);
	  g_free (proc_help);
	  g_free (proc_author);
	  g_free (proc_copyright);
	  g_free (proc_date);
	  g_free (params);
	  g_free (return_vals);
	  /*gimp_destroy_paramdefs (params, nparams);
	  gimp_destroy_paramdefs (return_vals, nreturn_vals);*/
	}
    }

  g_free (proc_list);
}

static void
init_constants (void)
{
  /*gchar *gimp_plugin_dir;*/
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_data_dir",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-data-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);
  /*setvar (cintern ("gimp-data-dir"), 
	  strcons (-1, gimp_data_directory ()), NIL);

  gimp_plugin_dir = gimp_gimprc_query ("gimp_plugin_dir");
  if (gimp_plugin_dir)
    {
      setvar (cintern ("gimp-plugin-dir"), 
	      strcons (-1, gimp_plugin_dir), NIL);
      g_free (gimp_plugin_dir);
    }*/
  
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_plugin_dir",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-plugin-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);

  /* Generated constants */
  init_generated_constants ();

  /* These are for backwards compatibility; they should be removed sometime */
  setvar (cintern ("NORMAL"),         flocons (GIMP_NORMAL_MODE),       NIL);
  setvar (cintern ("DISSOLVE"),       flocons (GIMP_DISSOLVE_MODE),     NIL);
  setvar (cintern ("BEHIND"),         flocons (2),       NIL);
  setvar (cintern ("MULTIPLY"),       flocons (GIMP_MULTIPLY_MODE),     NIL);
  setvar (cintern ("SCREEN"),         flocons (GIMP_SCREEN_MODE),       NIL);
  setvar (cintern ("OVERLAY"),        flocons (5),      NIL);
  setvar (cintern ("DIFFERENCE"),     flocons (GIMP_DIFFERENCE_MODE),   NIL);
  setvar (cintern ("ADDITION"),       flocons (GIMP_ADDITION_MODE),     NIL);
  setvar (cintern ("SUBTRACT"),       flocons (8),     NIL);
  setvar (cintern ("DARKEN-ONLY"),    flocons (GIMP_DARKEN_ONLY_MODE),  NIL);
  setvar (cintern ("LIGHTEN-ONLY"),   flocons (GIMP_LIGHTEN_ONLY_MODE), NIL);
  setvar (cintern ("HUE"),            flocons (GIMP_HUE_MODE),          NIL);
  setvar (cintern ("SATURATION"),     flocons (GIMP_SATURATION_MODE),   NIL);
  setvar (cintern ("COLOR"),          flocons (GIMP_COLOR_MODE),        NIL);
  setvar (cintern ("VALUE"),          flocons (GIMP_VALUE_MODE),        NIL);
/*  setvar (cintern ("DIVIDE"),         flocons (GIMP_DIVIDE_MODE),       NIL);

  setvar (cintern ("BLUR"),           flocons (GIMP_BLUR_CONVOLVE),     NIL);
  setvar (cintern ("SHARPEN"),        flocons (GIMP_SHARPEN_CONVOLVE),  NIL);*/

  setvar (cintern ("RGB_IMAGE"),      flocons (GIMP_RGB_IMAGE),         NIL);
  setvar (cintern ("RGBA_IMAGE"),     flocons (GIMP_RGBA_IMAGE),        NIL);
  setvar (cintern ("GRAY_IMAGE"),     flocons (GIMP_GRAY_IMAGE),        NIL);
  setvar (cintern ("GRAYA_IMAGE"),    flocons (GIMP_GRAYA_IMAGE),       NIL);
  setvar (cintern ("INDEXED_IMAGE"),  flocons (GIMP_INDEXED_IMAGE),     NIL);
  setvar (cintern ("INDEXEDA_IMAGE"), flocons (GIMP_INDEXEDA_IMAGE),    NIL);
  setvar (cintern ("U16_RGB_IMAGE"), flocons (6), NIL);
  setvar (cintern ("U16_RGBA_IMAGE"), flocons (7), NIL);
  setvar (cintern ("U16_GRAY_IMAGE"), flocons (8), NIL);
  setvar (cintern ("U16_GRAYA_IMAGE"), flocons (9), NIL);
  setvar (cintern ("U16_INDEXED_IMAGE"), flocons (10), NIL);
  setvar (cintern ("U16_INDEXEDA_IMAGE"), flocons (11), NIL);
  setvar (cintern ("FLOAT_RGB_IMAGE"), flocons (12), NIL);
  setvar (cintern ("FLOAT_RGBA_IMAGE"), flocons (13), NIL);
  setvar (cintern ("FLOAT_GRAY_IMAGE"), flocons (14), NIL);
  setvar (cintern ("FLOAT_GRAYA_IMAGE"), flocons (15), NIL);

  /* Useful misc stuff */
  setvar (cintern ("TRUE"),           flocons (TRUE),  NIL);
  setvar (cintern ("FALSE"),          flocons (FALSE), NIL);

  /*  Script-fu types  */
  setvar (cintern ("SF-IMAGE"),       flocons (SF_IMAGE),      NIL);
  setvar (cintern ("SF-DRAWABLE"),    flocons (SF_DRAWABLE),   NIL);
  setvar (cintern ("SF-LAYER"),       flocons (SF_LAYER),      NIL);
  setvar (cintern ("SF-CHANNEL"),     flocons (SF_CHANNEL),    NIL);
  setvar (cintern ("SF-COLOR"),       flocons (SF_COLOR),      NIL);
  setvar (cintern ("SF-TOGGLE"),      flocons (SF_TOGGLE),     NIL);
  setvar (cintern ("SF-VALUE"),       flocons (SF_VALUE),      NIL);
  setvar (cintern ("SF-STRING"),      flocons (SF_STRING),     NIL);
  setvar (cintern ("SF-FILENAME"),    flocons (SF_FILENAME),   NIL);
  setvar (cintern ("SF-ADJUSTMENT"),  flocons (SF_ADJUSTMENT), NIL);
  setvar (cintern ("SF-FONT"),        flocons (SF_FONT),       NIL);
  setvar (cintern ("SF-PATTERN"),     flocons (SF_PATTERN),    NIL);
  setvar (cintern ("SF-BRUSH"),       flocons (SF_BRUSH),      NIL);
  setvar (cintern ("SF-GRADIENT"),    flocons (SF_GRADIENT),   NIL);
  setvar (cintern ("SF-OPTION"),      flocons (SF_OPTION),     NIL);

  /* for SF_ADJUSTMENT */
  setvar (cintern ("SF-SLIDER"),      flocons (SF_SLIDER),     NIL);
  setvar (cintern ("SF-SPINNER"),     flocons (SF_SPINNER),    NIL);
}

static void
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static gint
sputs_fcn (gchar *st,
	   void  *dest)
{
  strcpy (*((gchar**)dest), st);
  *((gchar**)dest) += strlen (st);

  return (1);
}

static LISP
lprin1s (LISP   exp,
	 gchar *dest)
{
  struct gen_printio s;

  s.putc_fcn    = NULL;
  s.puts_fcn    = sputs_fcn;
  s.cb_argument = &dest;

  lprin1g (exp, &s);

  return (NIL);
}


static LISP
marshall_proc_db_call (LISP a)
{
  GParam       *args;
  GParam       *values = NULL;
  gint             nvalues;
  gchar           *proc_name;
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  int/*GimpPDBProcType*/  proc_type;
  gint             nparams;
  gint             nreturn_vals;
  GParamDef    *params;
  GParamDef    *return_vals;
  gchar  error_str[256];
  gint   i;
  gint   success = TRUE;
  LISP   color_list;
  LISP   intermediate_val;
  LISP   return_val = NIL;
  gchar *string;
  gint   string_len;
  LISP   a_saved;

  /* Save a in case it is needed for an error message. */
  a_saved = a;

  /*  Make sure there are arguments  */
  if (a == NIL)
    return my_err ("Procedure database argument marshaller was called with no arguments. "
		   "The procedure to be executed and the arguments it requires "
		   "(possibly none) must be specified.", NIL);

  /*  Derive the pdb procedure name from the argument 
      or first argument of a list  */
  if (TYPEP (a, tc_cons))
    proc_name = get_c_string (car (a));
  else
    proc_name = get_c_string (a);

  /*  report the current command  */
  script_fu_report_cc (proc_name);

  /*  Attempt to fetch the procedure from the database  */
  if (! gimp_query_procedure/*gimp_procedural_db_proc_info*/ (proc_name, 
				      &proc_blurb, 
				      &proc_help, 
				      &proc_author,
				      &proc_copyright,
				      &proc_date,
				      &proc_type,
				      &nparams, &nreturn_vals,
				      &params, &return_vals))
    return my_err ("Invalid procedure name specified.", NIL);


  /*  Free unused query information.  */

  g_free (proc_blurb);
  g_free (proc_help);
  g_free (proc_author);
  g_free (proc_copyright);
  g_free (proc_date);

  for (i = 0; i < nparams; i++)
    {
      g_free (params[i].name);
      g_free (params[i].description);
    }

  for (i = 0; i < nreturn_vals; i++)
    {
      g_free (return_vals[i].name);
      g_free (return_vals[i].description);
    }


  /*  Check the supplied number of arguments  */
  if ((nlength (a) - 1) != nparams)
    {
      g_snprintf (error_str, sizeof (error_str), 
		  "Invalid arguments supplied to %s--(# args: %ld, expecting: %d)",
		  proc_name, (nlength (a) - 1), nparams);
      return my_err (error_str, NIL);
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = g_new (GParam, nparams);
  else
    args = NULL;

  a = cdr (a);
  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
	{
	case GIMP_PDB_INT32:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT32;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_INT16:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT16;
	      args[i].data.d_int16 = (gint16) get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_INT8:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT8;
	      args[i].data.d_int8 = (gint8) get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_FLOAT:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_FLOAT;
	      args[i].data.d_float = get_c_double (car (a));
	    }
	  break;

	case GIMP_PDB_STRING:
	  if (!TYPEP (car (a), tc_string))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_STRING;
	      args[i].data.d_string = get_c_string (car (a));
	    }
	  break;

	case GIMP_PDB_INT32ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT32ARRAY;
	      args[i].data.d_int32array = 
		(gint32*) (car (a))->storage_as.long_array.data;
	    }
	  break;

	case GIMP_PDB_INT16ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT16ARRAY;
	      args[i].data.d_int16array = 
		(gint16*) (car (a))->storage_as.long_array.data;
	    }
	  break;

	case GIMP_PDB_INT8ARRAY:
	  if (!TYPEP (car (a), tc_byte_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_INT8ARRAY;
	      args[i].data.d_int8array = 
		(gint8*) (car (a))->storage_as.string.data;
	    }
	  break;

	case GIMP_PDB_FLOATARRAY:
	  if (!TYPEP (car (a), tc_double_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_FLOATARRAY;
	      args[i].data.d_floatarray = (car (a))->storage_as.double_array.data;
	    }
	  break;

	case GIMP_PDB_STRINGARRAY:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_STRINGARRAY;

	      /*  Set the array  */
	      {
		gint j;
		gint num_strings;
		gchar **array;
		LISP list;

		list = car (a);
		num_strings = args[i - 1].data.d_int32;
		if (nlength (list) != num_strings)
		  return my_err ("String array argument has incorrectly specified length", NIL);
		array = args[i].data.d_stringarray = 
		  g_new (char *, num_strings);

		for (j = 0; j < num_strings; j++)
		  {
		    array[j] = get_c_string (car (list));
		    list = cdr (list);
		  }
	      }
	    }
	  break;

	case GIMP_PDB_COLOR:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_COLOR;
	      color_list = car (a);
	      args[i].data.d_color.red = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.green = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.blue = get_c_long (car (color_list));
	    }
	  break;

	case GIMP_PDB_REGION:
	  return my_err ("Regions are currently unsupported as arguments", 
			 car (a));
	  break;

	case GIMP_PDB_DISPLAY:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_DISPLAY;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_IMAGE:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_IMAGE;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_LAYER:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_LAYER;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_CHANNEL:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_CHANNEL;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_DRAWABLE:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_DRAWABLE;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_SELECTION:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_SELECTION;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;

	case GIMP_PDB_BOUNDARY:
	  return my_err ("Boundaries are currently unsupported as arguments", 
			 car (a));
	  break;

	case GIMP_PDB_PATH:
	  return my_err ("Paths are currently unsupported as arguments", 
			 car (a));
	  break;
#if 0
	case GIMP_PDB_PARASITE:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = GIMP_PDB_PARASITE;
	      /* parasite->name */
	      intermediate_val = car (a);
	      args[i].data.d_parasite.name = 
		get_c_string (car (intermediate_val));
	      
	      /* parasite->flags */
	      intermediate_val = cdr (intermediate_val);
	      args[i].data.d_parasite.flags = get_c_long (car(intermediate_val));

	      /* parasite->size */
	      intermediate_val = cdr (intermediate_val);
	      args[i].data.d_parasite.size =
		(car (intermediate_val))->storage_as.string.dim;

	      /* parasite->data */
	      args[i].data.d_parasite.data =
		(void*) (car (intermediate_val))->storage_as.string.data;
	    }
	  break;
#endif
	case GIMP_PDB_STATUS:
	  return my_err ("Status is for return types, not arguments", car (a));
	  break;

	default:
	  return my_err ("Unknown argument type", NIL);
	}

      a = cdr (a);
    }

  if (success)
    values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
  else
    return my_err ("Invalid types specified for arguments", NIL);

  /*  Check the return status  */
  if (! values)
    {
      strcpy (error_str, "Procedural database execution did not return a status:\n    ");
      lprin1s (a_saved, error_str + strlen(error_str));
      
      return my_err (error_str, NIL);
    }

  switch (values[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
	  strcpy (error_str, "Procedural database execution failed:\n    ");
	  lprin1s (a_saved, error_str + strlen(error_str));
      return my_err (error_str, NIL);
      break;

    case GIMP_PDB_CALLING_ERROR:
	  strcpy (error_str, "Procedural database execution failed on invalid input arguments:\n    ");
	  lprin1s (a_saved, error_str + strlen(error_str));
      return my_err (error_str, NIL);
      break;

    case GIMP_PDB_SUCCESS:
      return_val = NIL;

      for (i = 0; i < nvalues - 1; i++)
	{
	  switch (return_vals[i].type)
	    {
	    case GIMP_PDB_INT32:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_INT16:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_INT8:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_FLOAT:
	      return_val = cons (flocons (values[i + 1].data.d_float), 
				 return_val);
	      break;

	    case GIMP_PDB_STRING:
	      string = (gchar *) values[i + 1].data.d_string;
	      string_len = strlen (string);
	      return_val = cons (strcons (string_len, string), return_val);
	      break;

	    case GIMP_PDB_INT32ARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_long_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  {
		    array->storage_as.long_array.data[j] = 
		      values[i + 1].data.d_int32array[j];
		  }
		return_val = cons (array, return_val);
	      }
	      break;

	    case GIMP_PDB_INT16ARRAY:
	      return my_err ("Arrays are currently unsupported as return values", NIL);
	      break;

	    case GIMP_PDB_INT8ARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_byte_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  {
		    array->storage_as.string.data[j] = 
		      values[i + 1].data.d_int8array[j];
		  }
		return_val = cons (array, return_val);
	      }
	      break;

	    case GIMP_PDB_FLOATARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_double_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  {
		    array->storage_as.double_array.data[j] = 
		      values[i + 1].data.d_floatarray[j];
		  }
		return_val = cons (array, return_val);
	      }
	      break;

	    case GIMP_PDB_STRINGARRAY:
	      /*  string arrays are always implemented such that the previous
	       *  return value contains the number of strings in the array
	       */
	      {
		int j;
		int num_strings = values[i].data.d_int32;
		LISP string_array = NIL;
		char **array = (char **) values[i + 1].data.d_stringarray;

		for (j = 0; j < num_strings; j++)
		  {
		    string_len = strlen (array[j]);
		    string_array = cons (strcons (string_len, array[j]), 
					 string_array);
		  }

		return_val = cons (nreverse (string_array), return_val);
	      }
	      break;

	    case GIMP_PDB_COLOR:
	      intermediate_val = cons (flocons ((int) values[i + 1].data.d_color.red),
				       cons (flocons ((int) values[i + 1].data.d_color.green),
					     cons (flocons ((int) values[i + 1].data.d_color.blue),
						   NIL)));
	      return_val = cons (intermediate_val, return_val);
	      break;

	    case GIMP_PDB_REGION:
	      return my_err ("Regions are currently unsupported as return values", NIL);
	      break;

	    case GIMP_PDB_DISPLAY:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_IMAGE:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_LAYER:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_CHANNEL:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_DRAWABLE:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_SELECTION:
	      return_val = cons (flocons (values[i + 1].data.d_int32), 
				 return_val);
	      break;

	    case GIMP_PDB_BOUNDARY:
	      return my_err ("Boundaries are currently unsupported as return values", NIL);
	      break;

	    case GIMP_PDB_PATH:
	      return my_err ("Paths are currently unsupported as return values", NIL);
	      break;
#if 0
	    case GIMP_PDB_PARASITE:
	      {
		LISP name, flags, data;

		if (values[i + 1].data.d_parasite.name == NULL)
		  {
		    return_val = my_err("Error: null parasite", NIL);
		  }
		else
		  {
		    string_len = strlen (values[i + 1].data.d_parasite.name);
		    name    = strcons (string_len,
				       values[i + 1].data.d_parasite.name);
		    
		    flags   = flocons (values[i + 1].data.d_parasite.flags);
		    data    = arcons (tc_byte_array,
				      values[i+1].data.d_parasite.size, 0);
		    memcpy(data->storage_as.string.data,
			   values[i+1].data.d_parasite.data, 
			   values[i+1].data.d_parasite.size); 
		    
		    intermediate_val = cons (name, 
					     cons(flags, cons(data, NIL)));
		    return_val = cons (intermediate_val, return_val);
		  }
	      }
	      break;
#endif
	    case GIMP_PDB_STATUS:
	      return my_err ("Procedural database execution returned multiple status values", NIL);
	      break;

	    default:
	      return my_err ("Unknown return type", NIL);
	    }
	}
      break;

    case GIMP_PDB_PASS_THROUGH:
    case GIMP_PDB_CANCEL:   /*  should we do something here?  */
      break;
    }

  /*  free up the executed procedure return values  */
  gimp_destroy_params (values, nvalues);

  /*  free up arguments and values  */
  g_free (args);

  g_free (params);
  g_free (return_vals);

  /*  reverse the return values  */
  return_val = nreverse (return_val);
#ifndef G_OS_WIN32
  /*  if we're in server mode, listen for additional commands for 10 ms  */
  if (server_mode)
    script_fu_server_listen (10);
#endif

#ifdef GDK_WINDOWING_WIN32
  /* This seems to help a lot on Windoze. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
#endif

  return return_val;
}

static LISP
script_fu_register_call (LISP a)
{
  if (script_fu_base)
    return script_fu_add_script (a);
  else
    return NIL;
}

static LISP
script_fu_quit_call (LISP a)
{
  script_fu_done = TRUE;

  return NIL;
}

static void
script_fu_auxillary_init (void)
{
  static GParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "[Interactive], non-interactive" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N_UI();
  textdomain("cinepaint-script-fu");

  gimp_install_temp_proc ("script_fu_refresh",
			  _("Re-read all available scripts"),
			  _("Re-read all available scripts"),
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Scripts/Script-Fu/Refresh",
			  /*N_("<Toolbox>/Xtns/Script-Fu/Refresh"),*/
			  NULL,
			  GIMP_TEMPORARY,
			  nargs, 0,
			  args, NULL,
			  script_fu_refresh_proc);
}

static void
script_fu_refresh_proc (gchar       *name,
			gint         nparams,
			GParam   *params,
			gint        *nreturn_vals,
			GParam  **return_vals)
{
  static GParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /*  Reload all of the available scripts  */
  script_fu_find_scripts ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
