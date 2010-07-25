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

#include <glib.h>		/* Include early for obscure Win32
				   build reasons */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <ctype.h>		/* For toupper() */

#ifndef GTK_ENABLE_BROKEN
#define GTK_ENABLE_BROKEN
#endif
#include <gtk/gtk.h>
#include <gtk/gtktext.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpmenu.h"
#include "gimpcolorbutton.h"
#include "gimpfileselection.h"

#include "plugin_main.h"
#include "ui.h"

#include "siod.h"
#include "script-fu-scripts.h"

#include "script-fu-intl.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>

#include <io.h>

#ifndef W_OK
#define W_OK 2
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) ((m) & _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m) & _S_IFREG)
#endif

#endif /* G_OS_WIN32 */

#if GTK_MAJOR_VERSION > 1
#define ESCAPE(string) g_strescape (string, NULL)
#else
#define ESCAPE(string) g_strescape (string)
#endif

#define TEXT_WIDTH           100
#define TEXT_HEIGHT           25
#define COLOR_SAMPLE_WIDTH   100
#define COLOR_SAMPLE_HEIGHT   15
#define SLIDER_WIDTH         100
#define SPINNER_WIDTH         75
#define FONT_PREVIEW_WIDTH   100

#define DEFAULT_FONT_SIZE    240

#define MAX_STRING_LENGTH   4096


typedef struct
{
  GtkAdjustment    *adj;
  gfloat            value;
  gfloat            lower;
  gfloat            upper;
  gfloat            step;
  gfloat            page;
  gint              digits;
  SFAdjustmentType  type;
} SFAdjustment;

typedef struct
{
  GtkWidget *preview;
  GtkWidget *dialog;
  gchar     *fontname;
} SFFont;

typedef struct
{
  GtkWidget *fileselection;
  gchar     *filename;
} SFFilename;

typedef struct 
{
  gchar   *name;
  gdouble  opacity;
  gint     spacing;
  gint     paint_mode;
} SFBrush;

typedef struct 
{
  GSList  *list;
  guint    history;
} SFOption;

typedef union
{
  gint32         sfa_image;
  gint32         sfa_drawable;
  gint32         sfa_layer;
  gint32         sfa_channel;
  guchar         sfa_color[3];
  gint32         sfa_toggle;
  gchar         *sfa_value;
  SFAdjustment   sfa_adjustment;
  SFFont         sfa_font;
  SFFilename     sfa_file;
  gchar         *sfa_pattern;
  gchar         *sfa_gradient;
  SFBrush        sfa_brush;
  SFOption       sfa_option;
} SFArgValue;

typedef struct
{
  gchar         *script_name;
  gchar         *pdb_name;
  gchar         *description;
  gchar         *help;
  gchar         *author;
  gchar         *copyright;
  gchar         *date;
  gchar         *img_types;
  gint           num_args;
  SFArgType     *arg_types;
  gchar        **arg_labels;
  SFArgValue    *arg_defaults;
  SFArgValue    *arg_values;
  gint32         image_based;
  GimpParamDef  *args;     /*  used only temporary until installed  */
} SFScript;

typedef struct
{
  GtkWidget    **args_widgets;
  GtkWidget     *status;
  GtkWidget     *about_dialog;
  gchar         *window_title;
  gchar         *last_command;
  gint           command_count;
  gint           consec_command_count;
} SFInterface;


/* External functions
 */
extern long  nlength (LISP obj);

/*
 *  Local Functions
 */

static gint       script_fu_install_script   (gpointer    foo,
					      SFScript   *script,
					      gpointer    bar);
static gint       script_fu_remove_script    (gpointer    foo,
					      SFScript   *script,
					      gpointer    bar);
static void       script_fu_script_proc      (gchar      *name,
					      gint        nparams,
					      GimpParam  *params,
					      gint       *nreturn_vals,
					      GimpParam **return_vals);

static SFScript * script_fu_find_script      (gchar      *script_name);
static void       script_fu_free_script      (SFScript   *script);
static void       script_fu_interface        (SFScript   *script);
static void       script_fu_interface_quit   (SFScript   *script);
static void       script_fu_error_msg        (gchar      *command);

static void       script_fu_ok_callback             (GtkWidget *widget,
						     gpointer   data);
static void       script_fu_about_callback          (GtkWidget *widget,
						     gpointer   data);
static void       script_fu_reset_callback          (GtkWidget *widget,
						     gpointer   data);
static void       script_fu_menu_callback           (gint32     id,
						     gpointer   data);

static void       script_fu_toggle_update    (GtkWidget *widget,
					      gpointer   data);

static void       script_fu_file_selection_callback (GtkWidget *widget,
						     gpointer   data);

static void       script_fu_font_preview_callback   (GtkWidget *widget,
						     gpointer   data);

static void       script_fu_font_dialog_ok          (GtkWidget *widget,
						     gpointer   data);
static void       script_fu_font_dialog_cancel      (GtkWidget *widget,
						     gpointer   data);
static gint       script_fu_font_dialog_delete      (GtkWidget *widget,
						     GdkEvent  *event,
						     gpointer   data);

static void       script_fu_font_preview            (GtkWidget *preview,
						     gchar     *fontname);
static void       script_fu_pattern_preview         (gchar     *name,
						     gint       width,
						     gint       height,
						     gint       bytes,
						     gchar     *mask_data,
						     gint       closing,
						     gpointer   data);
static void       script_fu_gradient_preview        (gchar     *name,
						     gint       width,
						     gdouble   *mask_data,
						     gint       closing,
						     gpointer   data);
static void       script_fu_brush_preview           (gchar     *name,
						     gdouble    opacity,
						     gint       spacing,
						     gint       paint_mode,
						     gint       width,
						     gint       height,
						     gchar     *mask_data,
						     gint       closing,
						     gpointer   data);



GtkWidget * gimp_gradient_select_widget           (gchar      *gname,
						   gchar      *igradient, 
						   GimpRunGradientCallback  cback,
						   gpointer    data);
void      gimp_gradient_select_widget_close_popup (GtkWidget  *widget);
void      gimp_gradient_select_widget_set_popup   (GtkWidget  *widget,
						   gchar      *gname);

gchar   * gimp_interactive_selection_gradient     (gchar      *dialogtitle,
						   gchar      *gradient_name,
						   gint        sample_sz,
						   GimpRunGradientCallback  callback,
						   gpointer    data);
  
GtkWidget * gimp_gradient_select_widget           (gchar      *gname,
						   gchar      *igradient, 
						   GimpRunGradientCallback  cback,
						   gpointer    data);
gchar    * gimp_interactive_selection_brush    (gchar     *dialogname,
						gchar     *brush_name,
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode,
						GimpRunBrushCallback  callback,
						gpointer   data);
  
GtkWidget * gimp_brush_select_widget           (gchar     *dname,
						gchar     *ibrush, 
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode,
						GimpRunBrushCallback  cback,
						gpointer   data);
  
void      gimp_brush_select_widget_set_popup   (GtkWidget *widget,
						gchar     *pname,
						gdouble    opacity,
						gint       spacing,
						gint       paint_mode);
void      gimp_brush_select_widget_close_popup (GtkWidget *widget);
/*
 *  Local variables
 */

static struct stat  filestat;
static GTree       *script_list = NULL;
static SFInterface *sf_interface = NULL;  /*  there can only be at most one
					      interactive interface  */
					      
extern gchar        siod_err_msg[];

/*
 *  Function definitions
 */

/*
 *  Helper Functions
 */

#if 0
/**
 * gimp_table_attach_aligned:
 * @table:      The #GtkTable the widgets will be attached to.
 * @column:     The column to start with.
 * @row:        The row to attach the eidgets.
 * @label_text: The text for the #GtkLabel which will be attached left of the
 *              widget.
 * @xalign:     The horizontal alignment of the #GtkLabel.
 * @yalign:     The vertival alignment of the #GtkLabel.
 * @widget:     The #GtkWidget to attach right of the label.
 * @colspan:    The number of columns the widget will use.
 * @left_align: #TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be #NULL and that the widget will be attached
 * starting at (@column + 1) in this case, too.
 **/
void
gimp_table_attach_aligned (GtkTable    *table,
			   gint         column,
			   gint         row,
			   const gchar *label_text,
			   gfloat       xalign,
			   gfloat       yalign,
			   GtkWidget   *widget,
			   gint         colspan,
			   gboolean     left_align)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, label,
			column, column + 1,
			row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  if (left_align)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
      gtk_widget_show (widget);

      widget = alignment;
    }

  gtk_table_attach (table, widget,
		    column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (widget);
}
#endif

/**
 * gimp_path_runtime_fix:
 * @path: A pointer to a string (allocated with g_malloc) that is (or could be) a pathname.
 *
 * On Windows, this function checks if the string pointed to by @path
 * starts with the compile-time prefix, and in that case, replaces the
 * prefix with the run-time one.  @path should be a pointer to a
 * dynamically allocated (with g_malloc, g_strconcat, etc) string. If
 * the replacement takes place, the original string is deallocated,
 * and *@path is replaced with a pointer to a new string with the
 * run-time prefix spliced in.
 */

void
gimp_path_runtime_fix (gchar **path)
{
#if defined (G_OS_WIN32) && defined (PREFIX)
  gchar *p;

  /* Yes, I do mean forward slashes below */
  if (strncmp (*path, PREFIX "/", strlen (PREFIX "/")) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine. */
      p = *path;
      *path = g_strconcat (gimp_toplevel_directory (),
			   "\\",
			   *path + strlen (PREFIX "/"),
			   NULL);
      g_free (p);
    }
  /* Replace forward slashes with backslashes, just for
   * completeness */
  p = *path;
  while ((p = strchr (p, '/')) != NULL)
    {
      *p = '\\';
      p++;
    }
#endif
}


/**
 * gimp_path_parse:
 * @path: A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths: The maximum number of directories to return.
 * @check: #TRUE if you want the directories to be checked.
 * @check_failed: Returns a #GList of path elements for which the
 *                check failed. Each list element is guaranteed
 *		  to end with a #G_PATH_SEPARATOR.
 *
 * Returns: A #GList of all directories in @path. Each list element
 *	    is guaranteed to end with a #G_PATH_SEPARATOR.
 **/
GList *
gimp_path_parse (gchar     *path,
		 gint       max_paths,
		 gboolean   check,
		 GList    **check_failed)
{
  gchar  *home;
  gchar **patharray;
  GList  *list = NULL;
  GList  *fail_list = NULL;
  gint    i;

  struct stat filestat;
  gint        err = FALSE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  home = g_get_home_dir ();

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (!patharray[i])
	break;

#ifndef G_OS_WIN32
      if (*patharray[i] == '~')
	{
	  dir = g_string_new (home);
	  g_string_append (dir, patharray[i] + 1);
	}
      else
#endif
	{
	  gimp_path_runtime_fix (&patharray[i]);
	  dir = g_string_new (patharray[i]);
	}

#ifdef __EMX__
      _fnslashify (dir);
#endif

      if (check)
	{
	  /*  check if directory exists  */
	  err = stat (dir->str, &filestat);

	  if (!err && S_ISDIR (filestat.st_mode))
	    {
	      if (dir->str[dir->len - 1] != G_DIR_SEPARATOR)
		g_string_append_c (dir, G_DIR_SEPARATOR);
	    }
	}

      if (!err)
	list = g_list_prepend (list, g_strdup (dir->str));
      else if (check_failed)
	fail_list = g_list_prepend (fail_list, g_strdup (dir->str));

      g_string_free (dir, TRUE);
    }

  g_strfreev (patharray);

  list = g_list_reverse (list);

  if (check && check_failed)
    {
      fail_list = g_list_reverse (fail_list);
      *check_failed = fail_list;
    }

  return list;
}


void
script_fu_find_scripts (void)
{
  GList *path_list;
  GList *list;
  gchar *path_str;
  gchar *path;
  gchar *filename;
  gchar *command;
  gint   my_err;
  DIR   *dir;
  struct dirent *dir_ent;
  GParam *return_vals;
  gint nreturn_vals;


  /*  Make sure to clear any existing scripts  */
  if (script_list != NULL)
    {
      g_tree_traverse (script_list, 
		       (GTraverseFunc)script_fu_remove_script, 
		       G_IN_ORDER, 
		       NULL);
      g_tree_destroy (script_list);
    }

#ifdef ENABLE_NLS
  script_list = g_tree_new ((GCompareFunc)strcoll);
#else
  script_list = g_tree_new ((GCompareFunc)strcmp);
#endif

  /*path_str = gimp_gimprc_query ("script-fu-path");*/
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "script-fu-path",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
      path_str = return_vals[1].data.d_string;

  if (path_str == NULL)
    return;

  /* Search through all directories in script-fu-path */
  
  path_list = gimp_path_parse (path_str, 16, TRUE, NULL);

  gimp_destroy_params (return_vals, nreturn_vals);


  list = path_list;
  while (list)
    {
      path = list->data;
      list = list->next;

      /* Open directory */
      dir = opendir (path);
      
      if (!dir)
	g_message ("error reading script directory \"%s\"", path);
      else
	{
	  while ((dir_ent = readdir (dir)))
	    {
	      filename = g_strdup_printf ("%s%s", path, dir_ent->d_name);
		  
	      if (g_strcasecmp (filename + strlen (filename) - 4, ".scm") == 0)
		{
		  /* Check the file and see that it is not a sub-directory */
		  my_err = stat (filename, &filestat);
		  
		  if (!my_err && S_ISREG (filestat.st_mode))
		    {
		      gchar *qf = ESCAPE (filename);
#ifdef __EMX__
		      _fnslashify(qf);
#endif
		      command = g_strdup_printf ("(load \"%s\")", qf);
		      g_free (qf);
		      
		      if (repl_c_string (command, 0, 0, 1) != 0)
			script_fu_error_msg (command);
#ifdef G_OS_WIN32
		      /* No, I don't know why, but this is 
		       * necessary on NT 4.0.
		       */
		      Sleep(0);
#endif
		      g_free (command);
		    }
		}
	      
	      g_free (filename);
	    } /* while */
	  
	  closedir (dir);
	} /* else */
    } /* while */
  
  /*g_free (path_str);*/

  /*  now that all scripts are read in and sorted, tell gimp about them  */
  g_tree_traverse (script_list, 
		   (GTraverseFunc)script_fu_install_script, G_IN_ORDER, NULL);
}

LISP
script_fu_add_script (LISP a)
{
  GimpParamDef *args;
  SFScript     *script;
  gchar        *val;
  gint          i;
  guchar        color[3];
  LISP          color_list;
  LISP          adj_list;
  /*LISP          brush_list;*/
  LISP          option_list;
  gchar        *s;

  /*  Check the length of a  */
  if (nlength (a) < 7)
    return my_err ("Too few arguments to script-fu-register", NIL);

  /*  Create a new script  */
  script = g_new0 (SFScript, 1);

  /*  Find the script name  */
  val = get_c_string (car (a));
  script->script_name = g_strdup (val);
  a = cdr (a);

  /* transform the function name into a name containing "_" for each "-".
   * this does not hurt anybody, yet improves the life of many... ;)
   */
  script->pdb_name = g_strdup (val);

  for (s = script->pdb_name; *s; s++)
    if (*s == '-')
      *s = '_';

  /*  Find the script description  */
  val = get_c_string (car (a));
  script->description = g_strdup (val);
  a = cdr (a);

  /*  Find the script help  */
  val = get_c_string (car (a));
  script->help = g_strdup (val);
  a = cdr (a);

  /*  Find the script author  */
  val = get_c_string (car (a));
  script->author = g_strdup (val);
  a = cdr (a);

  /*  Find the script copyright  */
  val = get_c_string (car (a));
  script->copyright = g_strdup (val);
  a = cdr (a);

  /*  Find the script date  */
  val = get_c_string (car (a));
  script->date = g_strdup (val);
  a = cdr (a);

  /*  Find the script image types  */
  if (TYPEP (a, tc_cons))
    {
      val = get_c_string (car (a));
      a = cdr (a);
    }
  else
    {
      val = get_c_string (a);
      a = NIL;
    }
  script->img_types = g_strdup (val);

  /*  Check the supplied number of arguments  */
  script->num_args = nlength (a) / 3;

  args = g_new (GimpParamDef, script->num_args + 1);
  args[0].type = GIMP_PDB_INT32;
  args[0].name = "run_mode";
  args[0].description = "Interactive, non-interactive";

  script->arg_types    = g_new (SFArgType, script->num_args);
  script->arg_labels   = g_new (gchar *, script->num_args);
  script->arg_defaults = g_new0 (SFArgValue, script->num_args);
  script->arg_values   = g_new0 (SFArgValue, script->num_args);

  if (script->num_args > 0)
    {
      for (i = 0; i < script->num_args; i++)
	{
	  if (a != NIL)
	    {
	      if (!TYPEP (car (a), tc_flonum))
		return my_err ("script-fu-register: argument types must be integer values", NIL);
	      script->arg_types[i] = get_c_long (car (a));
	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing type specifier", NIL);

	  if (a != NIL)
	    {
	      if (!TYPEP (car (a), tc_string))
		return my_err ("script-fu-register: argument labels must be strings", NIL);
	      script->arg_labels[i] = g_strdup (get_c_string (car (a)));
	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing arguments label", NIL);

	  if (a != NIL)
	    {
	      switch (script->arg_types[i])
		{
		case SF_IMAGE:
		case SF_DRAWABLE:
		case SF_LAYER:
		case SF_CHANNEL:
		  if (!TYPEP (car (a), tc_flonum))
		    return my_err ("script-fu-register: drawable defaults must be integer values", NIL);
		  script->arg_defaults[i].sfa_image = get_c_long (car (a));
		  script->arg_values[i].sfa_image = script->arg_defaults[i].sfa_image;

		  switch (script->arg_types[i])
		    {
		    case SF_IMAGE:
		      args[i + 1].type = GIMP_PDB_IMAGE;
		      args[i + 1].name = "image";
		      break;

		    case SF_DRAWABLE:
		      args[i + 1].type = GIMP_PDB_DRAWABLE;
		      args[i + 1].name = "drawable";
		      break;

		    case SF_LAYER:
		      args[i + 1].type = GIMP_PDB_LAYER;
		      args[i + 1].name = "layer";
		      break;

		    case SF_CHANNEL:
		      args[i + 1].type = GIMP_PDB_CHANNEL;
		      args[i + 1].name = "channel";
		      break;

		    default:
		      break;
		    }

		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_COLOR:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: color defaults must be a list of 3 integers", NIL);
		  color_list = car (a);
		  color[0] = 
		    (guchar)(CLAMP (get_c_long (car (color_list)), 0, 255));
		  color_list = cdr (color_list);
		  color[1] = 
		    (guchar)(CLAMP (get_c_long (car (color_list)), 0, 255));
		  color_list = cdr (color_list);
		  color[2] = 
		    (guchar)(CLAMP (get_c_long (car (color_list)), 0, 255));
		  memcpy (script->arg_defaults[i].sfa_color, 
			  color, sizeof (guchar) * 3);
		  memcpy (script->arg_values[i].sfa_color, 
			  color, sizeof (guchar) * 3);

		  args[i + 1].type = GIMP_PDB_COLOR;
		  args[i + 1].name = "color";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_TOGGLE:
		  if (!TYPEP (car (a), tc_flonum))
		    return my_err ("script-fu-register: toggle default must be an integer value", NIL);
		  script->arg_defaults[i].sfa_toggle = 
		    (get_c_long (car (a))) ? TRUE : FALSE;
		  script->arg_values[i].sfa_toggle = 
		    script->arg_defaults[i].sfa_toggle;

		  args[i + 1].type = GIMP_PDB_INT32;
		  args[i + 1].name = "toggle";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_VALUE:  
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: value defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value = 
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =  
		    g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_STRING:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: string defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_value = 
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_value =  
		    g_strdup (script->arg_defaults[i].sfa_value);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "string";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_ADJUSTMENT:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: adjustment defaults must be a list", NIL);
		  adj_list = car (a);
		  script->arg_defaults[i].sfa_adjustment.value = 
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.lower = 
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.upper = 
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.step = 
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.page = 
		    get_c_double (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.digits = 
		    get_c_long (car (adj_list));
		  adj_list = cdr (adj_list);
		  script->arg_defaults[i].sfa_adjustment.type = 
		    get_c_long (car (adj_list));
		  script->arg_values[i].sfa_adjustment.adj = NULL;
		  script->arg_values[i].sfa_adjustment.value = 
		    script->arg_defaults[i].sfa_adjustment.value;

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "value";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_FILENAME:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: filename defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_file.filename = 
		    g_strdup (get_c_string (car (a)));

#ifdef G_OS_WIN32
		  /* Replace POSIX slashes with Win32 backslashes. This
		   * is just so script-fus can be written with only
		   * POSIX directory separators.
		   */
		  val = script->arg_defaults[i].sfa_file.filename;
		  while (*val)
		    {
		      if (*val == '/')
			*val = '\\';
		      val++;
		    }
#endif
		  script->arg_values[i].sfa_file.filename =  
		    g_strdup (script->arg_defaults[i].sfa_file.filename);
		  script->arg_values[i].sfa_file.fileselection = NULL;

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "filename";
		  args[i + 1].description = script->arg_labels[i];
		 break;

		case SF_FONT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: font defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_font.fontname = 
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_font.fontname =  
		    g_strdup (script->arg_defaults[i].sfa_font.fontname);
		  script->arg_values[i].sfa_font.preview = NULL;
		  script->arg_values[i].sfa_font.dialog = NULL;
		  
		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "font";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_PATTERN:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: pattern defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_pattern = 
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_pattern =  
		    g_strdup (script->arg_defaults[i].sfa_pattern);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "pattern";
		  args[i + 1].description = script->arg_labels[i];
		  break;

#if 0
		case SF_BRUSH:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: brush defaults must be a list", NIL);
		  brush_list = car (a);
		  script->arg_defaults[i].sfa_brush.name = 
		    g_strdup (get_c_string (car (brush_list)));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.opacity = 
		    get_c_double (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.spacing = 
		    get_c_long (car (brush_list));
		  brush_list = cdr (brush_list);
		  script->arg_defaults[i].sfa_brush.paint_mode = 
		    get_c_long (car (brush_list));
		  script->arg_values[i].sfa_brush = 
		    script->arg_defaults[i].sfa_brush;
		  /* Need this since we need a copy of the string
		   * in the values area. We could free it later but the
		   * default one must hang around.
		   */
		  script->arg_values[i].sfa_brush.name = 
		    g_strdup(script->arg_defaults[i].sfa_brush.name);

		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "brush";
		  args[i + 1].description = script->arg_labels[i];
		  break;
#endif

		case SF_GRADIENT:
		  if (!TYPEP (car (a), tc_string))
		    return my_err ("script-fu-register: gradient defaults must be string values", NIL);
		  script->arg_defaults[i].sfa_gradient = 
		    g_strdup (get_c_string (car (a)));
		  script->arg_values[i].sfa_gradient =  
		    g_strdup (script->arg_defaults[i].sfa_pattern);
		  
		  args[i + 1].type = GIMP_PDB_STRING;
		  args[i + 1].name = "gradient";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		case SF_OPTION:
		  if (!TYPEP (car (a), tc_cons))
		    return my_err ("script-fu-register: option defaults must be a list", NIL);
		  for (option_list = car (a); 
		       option_list; 
		       option_list = cdr (option_list))
		    {
		      script->arg_defaults[i].sfa_option.list = 
			g_slist_append (script->arg_defaults[i].sfa_option.list, 
					g_strdup (get_c_string (car (option_list))));
		    }
		  script->arg_defaults[i].sfa_option.history = 0;
		  script->arg_values[i].sfa_option.history = 0;

		  args[i + 1].type = GIMP_PDB_INT32;
		  args[i + 1].name = "option";
		  args[i + 1].description = script->arg_labels[i];
		  break;

		default:
		  break;
		}

	      a = cdr (a);
	    }
	  else
	    return my_err ("script-fu-register: missing default argument", NIL);
	}
    }

  script->args = args;
  g_tree_insert (script_list, gettext (script->description), script);

  return NIL;
}

void
script_fu_report_cc (gchar *command)
{
  if (sf_interface == NULL)
    return;

  if (sf_interface->last_command && 
      strcmp (sf_interface->last_command, command) == 0)
    {
      gchar *new_command;

      sf_interface->command_count++;

      new_command = g_strdup_printf ("%s <%d>", 
				     command, sf_interface->command_count);
      gtk_entry_set_text (GTK_ENTRY (sf_interface->status), new_command);
      g_free (new_command);
    }
  else
    {
      sf_interface->command_count = 1;
      gtk_entry_set_text (GTK_ENTRY (sf_interface->status), command);
      g_free (sf_interface->last_command);
      sf_interface->last_command = g_strdup (command);
    }
  
  gdk_flush ();
}


/* 
 *  The following function is a GTraverseFunction, Please 
 *  note that it frees the script->args structure.  --Sven 
 */
static gint
script_fu_install_script (gpointer  foo,
			  SFScript *script,
			  gpointer  bar)
{
  gchar *menu_path = NULL;

  /* Allow scripts with no menus */
  if (strncmp (script->description, "<None>", 6) != 0)
    menu_path = script->description;

  gimp_install_temp_proc (script->pdb_name,
                          script->description,
                          script->help,
                          script->author,
                          script->copyright,
                          script->date,
                          menu_path,
                          script->img_types,
                          GIMP_TEMPORARY,
                          script->num_args + 1, 0,
                          script->args, NULL,
                          script_fu_script_proc);

  g_free (script->args);
  script->args = NULL;

  return FALSE;
}

/* 
 *  The following function is a GTraverseFunction.
 */
static gint
script_fu_remove_script (gpointer  foo,
			 SFScript *script,
			 gpointer  bar)
{
  script_fu_free_script (script);

  return FALSE;
}



static void
script_fu_script_proc (gchar       *name,
		       gint         nparams,
		       GimpParam   *params,
		       gint        *nreturn_vals,
		       GimpParam  **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunModeType   run_mode;
  SFScript         *script;
  gint              min_args;
  gchar            *escaped;

  run_mode = params[0].data.d_int32;

  if (! (script = script_fu_find_script (name)))
    status = GIMP_PDB_CALLING_ERROR;
  else
    {
      if (script->num_args == 0)
	run_mode = GIMP_RUN_NONINTERACTIVE;

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Determine whether the script is image based (runs on an image) */
	  if (strncmp (script->description, "<Image>", 7) == 0)
	    {
	      script->arg_values[0].sfa_image    = params[1].data.d_image;
	      script->arg_values[1].sfa_drawable = params[2].data.d_drawable;
	      script->image_based = TRUE;
	    }
	  else
	    script->image_based = FALSE;

	  /*  First acquire information with a dialog  */
	  /*  Skip this part if the script takes no parameters */ 
	  min_args = (script->image_based) ? 2 : 0;
	  if (script->num_args > min_args) 
	    {
	      script_fu_interface (script); 
	      break;
	    }  
	  /*  else fallthrough  */

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != (script->num_args + 1))
	    status = GIMP_PDB_CALLING_ERROR;
	  if (status == GIMP_PDB_SUCCESS)
	    {
	      gchar *text = NULL;
	      gchar *command;
	      gchar *c;
	      gchar  buffer[MAX_STRING_LENGTH];
	      gint   length;
	      gint   i;

	      length = strlen (script->script_name) + 3;

	      for (i = 0; i < script->num_args; i++)
		switch (script->arg_types[i])
		  {
		  case SF_IMAGE:
		  case SF_DRAWABLE:
		  case SF_LAYER:
		  case SF_CHANNEL:
		    length += 12;  /*  Maximum size of integer value will not exceed this many characters  */
		    break;

		  case SF_COLOR:
		    length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
		    break;

		  case SF_TOGGLE:
		    length += 6;   /*  Maximum size of (TRUE, FALSE)  */
		    break;

		  case SF_VALUE:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;

		  case SF_STRING:
		  case SF_FILENAME:
		    escaped = ESCAPE (params[i + 1].data.d_string);
		    length += strlen (escaped) + 3;
		    g_free (escaped);
		    break;

		  case SF_ADJUSTMENT:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;

		  case SF_FONT:
		  case SF_PATTERN:
		  case SF_GRADIENT:
		  case SF_BRUSH:
		    length += strlen (params[i + 1].data.d_string) + 3;
		    break;		    

		  case SF_OPTION:
		    length += strlen (params[i + 1].data.d_string) + 1;
		    break;

		  default:
		    break;
		  }

	      c = command = g_new (gchar, length);

	      if (script->num_args)
                {
                  sprintf (command, "(%s ", script->script_name);
                  c += strlen (script->script_name) + 2;

                  for (i = 0; i < script->num_args; i++)
                    {
                      switch (script->arg_types[i])
                        {
                        case SF_IMAGE:
                        case SF_DRAWABLE:
                        case SF_LAYER:
                        case SF_CHANNEL:
                          g_snprintf (buffer, sizeof (buffer), "%d",
				      params[i + 1].data.d_image);
                          text = buffer;
                          break;

                        case SF_COLOR:
                          g_snprintf (buffer, sizeof (buffer), "'(%d %d %d)",
				      params[i + 1].data.d_color.red,
				      params[i + 1].data.d_color.green,
				      params[i + 1].data.d_color.blue);
                          text = buffer;
                          break;

                        case SF_TOGGLE:
                          g_snprintf (buffer, sizeof (buffer), "%s",
				      (params[i + 1].data.d_int32) ? "TRUE" 
				                                   : "FALSE");
                          text = buffer;
                          break;

                        case SF_VALUE:
                          text = params[i + 1].data.d_string;
                          break;

                        case SF_STRING:
                        case SF_FILENAME:
                          escaped = ESCAPE (params[i + 1].data.d_string);
                          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
				      escaped);
                          g_free (escaped);
                          text = buffer;
                          break;

                        case SF_ADJUSTMENT:
                          text = params[i + 1].data.d_string;
                          break;

                        case SF_FONT:
                        case SF_PATTERN:
                        case SF_GRADIENT:
                        case SF_BRUSH:
                          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
				      params[i + 1].data.d_string);
                          text = buffer;
                          break;

                        case SF_OPTION:
                          text = params[i + 1].data.d_string;
                          break;

                        default:
                          break;
                        }

                      if (i == script->num_args - 1)
                        sprintf (c, "%s)", text);
                      else
                        sprintf (c, "%s ", text);

                      c += strlen (text) + 1;
                    }
                }
	      else
		sprintf (command, "(%s)", script->script_name);

	      /*  run the command through the interpreter  */
	      if (repl_c_string (command, 0, 0, 1) != 0)
		script_fu_error_msg (command);
     
	      g_free (command);
	    }
	  break;

	default:
	  break;
	}
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

/* this is a GTraverseFunction */
static gint
script_fu_lookup_script (gpointer  *foo,
			 SFScript  *script,
			 gchar    **name)
{
  if (strcmp (script->pdb_name, *name) == 0)
    {  
      /* store the script in the name pointer and stop the traversal */
      *name = (gchar *)script;
      return TRUE;
    }
  else
    return FALSE;
}

static SFScript *
script_fu_find_script (gchar *pdb_name)
{
  gchar *script;
  
  script = pdb_name;
  g_tree_traverse (script_list, 
		   (GTraverseFunc)script_fu_lookup_script, G_IN_ORDER, &script);
  if (script == pdb_name)
    return NULL;
  else
    return (SFScript *)script;
}

static void
script_fu_free_script (SFScript *script)
{
  gint i;

  /*  Uninstall the temporary procedure for this script  */
  gimp_uninstall_temp_proc (script->script_name);

  if (script)
    {
      g_free (script->script_name);
      g_free (script->description);
      g_free (script->help);
      g_free (script->author);
      g_free (script->copyright);
      g_free (script->date);
      g_free (script->img_types);
      g_free (script->arg_types);

      for (i = 0; i < script->num_args; i++)
	{
	  g_free (script->arg_labels[i]);
	  switch (script->arg_types[i])
	    {
	    case SF_IMAGE:
	    case SF_DRAWABLE:
	    case SF_LAYER:
	    case SF_CHANNEL:
	    case SF_COLOR:
	      break;

	    case SF_VALUE:
	    case SF_STRING:
	      g_free (script->arg_defaults[i].sfa_value);
	      g_free (script->arg_values[i].sfa_value);
	      break;

	    case SF_ADJUSTMENT:
	      break;

	    case SF_FILENAME:
	      g_free (script->arg_defaults[i].sfa_file.filename);
	      g_free (script->arg_values[i].sfa_file.filename);
	      break;

	    case SF_FONT:
	      g_free (script->arg_defaults[i].sfa_font.fontname);
	      g_free (script->arg_values[i].sfa_font.fontname);
	      break;

	    case SF_PATTERN:
	      g_free (script->arg_defaults[i].sfa_pattern);
	      g_free (script->arg_values[i].sfa_pattern);
	      break;

	    case SF_GRADIENT:
	      g_free (script->arg_defaults[i].sfa_gradient);
	      g_free (script->arg_values[i].sfa_gradient);
	      break;

	    case SF_BRUSH:
	      g_free (script->arg_defaults[i].sfa_brush.name);
	      g_free (script->arg_values[i].sfa_brush.name);
	      break;

	    case SF_OPTION:
	      g_slist_foreach (script->arg_defaults[i].sfa_option.list, 
			       (GFunc)g_free, NULL);
	      if (script->arg_defaults[i].sfa_option.list)
		g_slist_free (script->arg_defaults[i].sfa_option.list);
	      break;

	    default:
	      break;
	    }
	}

      g_free (script->arg_labels);
      g_free (script->arg_defaults);
      g_free (script->arg_values);

      g_free (script);
    }
}

static void
script_fu_interface (SFScript *script)
{
  GtkWidget *dlg;
  GtkWidget *main_box;
  GtkWidget *frame;
  GtkWidget *sep;
  GtkWidget *button;
  GtkWidget *menu;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *bbox;
  GtkWidget *menu_item;
  GSList    *list;
  gchar     *buf;
  gint       start_args;
  gint       i;
  guint      j;

  static gboolean gtk_initted = FALSE;

  /*  Simply return if there is already an interface. This is an
      ugly workaround for the fact that we can not process two
      scripts at a time.  */
  if (sf_interface != NULL)
    return;

  g_return_if_fail (script != NULL);

  if (!gtk_initted)
    {
      INIT_I18N_UI();
      textdomain("cinepaint-script-fu");

      gimp_ui_init ("script-fu", TRUE);

      gtk_initted = TRUE;
    }

  sf_interface = g_new0 (SFInterface, 1);
  sf_interface->args_widgets = g_new0 (GtkWidget *, script->num_args);
  
  /* strip the first part of the menupath if it contains _("/Script-Fu/") */
  buf = strstr (gettext (script->description), _("/Script-Fu/"));
  if (buf)
    sf_interface->window_title = g_strdup_printf (_("Script-Fu: %s"), 
						  (buf + strlen (_("/Script-Fu/"))));
  else 
    sf_interface->window_title = g_strdup_printf (_("Script-Fu: %s"), 
					    gettext (script->description));

  buf = strstr (sf_interface->window_title, "...");
  if (buf)
    *buf = '\0';

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_quit_add_destroy (1, GTK_OBJECT (dlg));
  gtk_window_set_title (GTK_WINDOW (dlg), sf_interface->window_title);
  gtk_window_set_wmclass (GTK_WINDOW (dlg), "script_fu", "Gimp");
  gtk_signal_connect_object (GTK_OBJECT (dlg), "delete_event",
			     GTK_SIGNAL_FUNC (script_fu_interface_quit),
			     (GtkObject *) script);
			     
  gimp_help_connect_help_accel (dlg, gimp_standard_help_func,
				"filters/script-fu.html");
  
  /* the vbox holding all widgets */
  main_box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dlg), main_box);
 
  /* the script arguments frame */
  frame = gtk_frame_new (_("Script Arguments"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (main_box), frame, TRUE, TRUE, 0);

  /* the vbox holding all widgets */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  /*  The argument table  */
  table = gtk_table_new (script->num_args + 1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  start_args = (script->image_based) ? 2 : 0;

  for (i = start_args; i < script->num_args; i++)
    {
      /*  we add a colon after the label; 
	  some languages want an extra space here */
      gchar     *label_text = 
	g_strdup_printf (_("%s:"), gettext (script->arg_labels[i]));
      gfloat     label_yalign = 0.5;
      gboolean   widget_leftalign = TRUE;

      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  sf_interface->args_widgets[i] = gtk_option_menu_new ();
	  switch (script->arg_types[i])
	    {
	    case SF_IMAGE:
	      menu = gimp_image_menu_new (NULL, script_fu_menu_callback,
					  &script->arg_values[i].sfa_image,
					  script->arg_values[i].sfa_image);
	      break;

	    case SF_DRAWABLE:
	      menu = gimp_drawable_menu_new (NULL, script_fu_menu_callback,
					     &script->arg_values[i].sfa_drawable,
					     script->arg_values[i].sfa_drawable);
	      break;

	    case SF_LAYER:
	      menu = gimp_layer_menu_new (NULL, script_fu_menu_callback,
					  &script->arg_values[i].sfa_layer,
					  script->arg_values[i].sfa_layer);
	      break;

	    case SF_CHANNEL:
	      menu = gimp_channel_menu_new (NULL, script_fu_menu_callback,
					    &script->arg_values[i].sfa_channel,
					    script->arg_values[i].sfa_channel);
	      break;

	    default:
	      menu = NULL;
	      break;
	    }
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (sf_interface->args_widgets[i]),
				    menu);
	  break;

	case SF_COLOR:
	  sf_interface->args_widgets[i] =
	    gimp_color_button_new (_("Script-Fu Color Selection"),
				   COLOR_SAMPLE_WIDTH, COLOR_SAMPLE_HEIGHT,
				   script->arg_values[i].sfa_color, 3);
	  break;

	case SF_TOGGLE:
	  g_free (label_text);
	  label_text = NULL;
	  sf_interface->args_widgets[i] =
	    gtk_check_button_new_with_label (gettext (script->arg_labels[i]));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sf_interface->args_widgets[i]),
				       script->arg_values[i].sfa_toggle);
	  gtk_signal_connect (GTK_OBJECT (sf_interface->args_widgets[i]), "toggled",
			      GTK_SIGNAL_FUNC (/*gimp_toggle_button_update*/script_fu_toggle_update),
			      &script->arg_values[i].sfa_toggle);
	  break;

	case SF_VALUE:
	case SF_STRING:
	  widget_leftalign = FALSE;

	  sf_interface->args_widgets[i] = gtk_entry_new ();
	  gtk_widget_set_usize (sf_interface->args_widgets[i], TEXT_WIDTH, 0);
	  gtk_entry_set_text (GTK_ENTRY (sf_interface->args_widgets[i]),
			      script->arg_values[i].sfa_value);
	  break;

	case SF_ADJUSTMENT:
	  script->arg_values[i].sfa_adjustment.adj = (GtkAdjustment *)
	    gtk_adjustment_new (script->arg_values[i].sfa_adjustment.value, 
				script->arg_defaults[i].sfa_adjustment.lower, 
				script->arg_defaults[i].sfa_adjustment.upper, 
				script->arg_defaults[i].sfa_adjustment.step, 
				script->arg_defaults[i].sfa_adjustment.page, 0);
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      label_yalign = 1.0;
	      widget_leftalign = FALSE;

	      sf_interface->args_widgets[i] =
		gtk_hscale_new (script->arg_values[i].sfa_adjustment.adj);
	      gtk_widget_set_usize (GTK_WIDGET (sf_interface->args_widgets[i]), 
				    SLIDER_WIDTH, -1);
	      gtk_scale_set_digits (GTK_SCALE (sf_interface->args_widgets[i]), 
				    script->arg_defaults[i].sfa_adjustment.digits);
	      gtk_scale_set_draw_value (GTK_SCALE (sf_interface->args_widgets[i]), TRUE);
	      gtk_range_set_update_policy (GTK_RANGE (sf_interface->args_widgets[i]), 
					   GTK_UPDATE_DELAYED);
	      break;

	    case SF_SPINNER:
	      sf_interface->args_widgets[i] =
		gtk_spin_button_new (script->arg_values[i].sfa_adjustment.adj, 0, 0);
	      gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (sf_interface->args_widgets[i]), TRUE);
	      gtk_widget_set_usize (sf_interface->args_widgets[i], SPINNER_WIDTH, 0);
	      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (sf_interface->args_widgets[i]),
					  script->arg_defaults[i].sfa_adjustment.digits);
	      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sf_interface->args_widgets[i]), TRUE);
	      break;
	    default: /* this shouldn't happen */
	      sf_interface->args_widgets[i] = NULL;
	      break;
	    }
	  break;

	case SF_FILENAME:
	  widget_leftalign = FALSE
;
	  sf_interface->args_widgets[i] =
	    gimp_file_selection_new (_("Script-Fu File Selection"),
				     script->arg_values[i].sfa_file.filename,
				     FALSE, TRUE);
	  script->arg_values[i].sfa_file.fileselection = sf_interface->args_widgets[i];

	  gtk_signal_connect (GTK_OBJECT (sf_interface->args_widgets[i]),
			      "filename_changed",
			      (GtkSignalFunc) script_fu_file_selection_callback,
			      &script->arg_values[i].sfa_file);
	  break;

	case SF_FONT:
	  widget_leftalign = FALSE;

	  sf_interface->args_widgets[i] = gtk_button_new ();
	  script->arg_values[i].sfa_font.preview = gtk_label_new ("");
	  script->arg_values[i].sfa_font.dialog = NULL;
	  gtk_widget_set_usize (sf_interface->args_widgets[i], FONT_PREVIEW_WIDTH, 0);
	  gtk_container_add (GTK_CONTAINER (sf_interface->args_widgets[i]),
			     script->arg_values[i].sfa_font.preview);
	  gtk_widget_show (script->arg_values[i].sfa_font.preview);

	  script_fu_font_preview (script->arg_values[i].sfa_font.preview,
				  script->arg_values[i].sfa_font.fontname);

	  gtk_signal_connect (GTK_OBJECT (sf_interface->args_widgets[i]), "clicked",
			      (GtkSignalFunc) script_fu_font_preview_callback,
			      &script->arg_values[i].sfa_font);	  
	  break;
#if 0
	case SF_PATTERN:
	  sf_interface->args_widgets[i] =
	    gimp_pattern_select_widget(_("Script-fu Pattern Selection"),
				       script->arg_values[i].sfa_pattern, 
				       script_fu_pattern_preview,
				       &script->arg_values[i].sfa_pattern);
	  break;
#endif
	case SF_GRADIENT:
	  sf_interface->args_widgets[i] =
	    gimp_gradient_select_widget(_("Script-Fu Gradient Selection"),
					script->arg_values[i].sfa_gradient, 
					script_fu_gradient_preview,
					&script->arg_values[i].sfa_gradient);
	  break;
#if 0
	case SF_BRUSH:
	  sf_interface->args_widgets[i] = 
	    gimp_brush_select_widget(_("Script-Fu Brush Selection"),
				     script->arg_values[i].sfa_brush.name, 
				     script->arg_values[i].sfa_brush.opacity, 
				     script->arg_values[i].sfa_brush.spacing, 
				     script->arg_values[i].sfa_brush.paint_mode, 
				     script_fu_brush_preview,
				     &script->arg_values[i].sfa_brush);
	  break;
#endif
	case SF_OPTION:
	  sf_interface->args_widgets[i] = gtk_option_menu_new ();
	  menu = gtk_menu_new ();
	  for (list = script->arg_defaults[i].sfa_option.list, j = 0; 
	       list; 
	       list = g_slist_next (list), j++)
	    {
	      menu_item = gtk_menu_item_new_with_label (gettext ((gchar *)list->data));
	      gtk_object_set_user_data (GTK_OBJECT (menu_item), 
					GUINT_TO_POINTER (j));
	      gtk_menu_append (GTK_MENU (menu), menu_item);
	      gtk_widget_show (menu_item);
	    }
	  gtk_option_menu_set_menu (GTK_OPTION_MENU (sf_interface->args_widgets[i]),
				    menu);
	  gtk_option_menu_set_history (GTK_OPTION_MENU (sf_interface->args_widgets[i]), 
							script->arg_values[i].sfa_option.history);
	  break;
	  
	default:
	  break;
	}
      gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				 label_text, 1.0, label_yalign,
				 sf_interface->args_widgets[i], 1, 
				 widget_leftalign);
      g_free (label_text);
    }

  gtk_widget_show (table);

  /*  Reset to defaults */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

  button = gtk_button_new_with_label (_("Reset to Defaults"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_reset_callback,
		      script);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /*  Separator  */
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_box), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  /*  Action area  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (main_box), hbox, FALSE, TRUE, 0);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 4);
  gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("About"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (script_fu_about_callback),
                      script);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_show (button);

  gtk_widget_show (bbox);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 4);
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (script_fu_ok_callback),
		      script);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (script_fu_interface_quit),
			     (GtkObject *) script);
  gtk_container_add (GTK_CONTAINER (bbox), button);  
  gtk_widget_show (button);

  gtk_widget_show (bbox);
  gtk_widget_show (hbox);

  /* The statusbar (well it's a faked statusbar...) */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);

  sf_interface->status = gtk_entry_new ();
  gtk_entry_set_editable (GTK_ENTRY (sf_interface->status), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), sf_interface->status, TRUE, TRUE, 2);
  gtk_entry_set_text (GTK_ENTRY (sf_interface->status), 
		      sf_interface->window_title);
  gtk_widget_show (sf_interface->status);

  gtk_widget_show (main_box);
  gtk_widget_show (dlg);

  gtk_main ();
}

static void
script_fu_interface_quit (SFScript *script)
{
  gint i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (sf_interface != NULL);  

  g_free (sf_interface->window_title);

  if (sf_interface->about_dialog)
    gtk_widget_destroy (sf_interface->about_dialog);

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_FONT:
	if (script->arg_values[i].sfa_font.dialog != NULL)
	  {
	    gtk_widget_destroy (script->arg_values[i].sfa_font.dialog);
	    script->arg_values[i].sfa_font.dialog = NULL;
	  }
	break;
#if 0
      case SF_PATTERN:
  	gimp_pattern_select_widget_close_popup (sf_interface->args_widgets[i]); 
	break;
#endif
      case SF_GRADIENT:
  	gimp_gradient_select_widget_close_popup (sf_interface->args_widgets[i]); 
	break;
#if 0
      case SF_BRUSH:
  	gimp_brush_select_widget_close_popup (sf_interface->args_widgets[i]); 
	break;
#endif
      default:
	break;
      }

  g_free (sf_interface->args_widgets);
  g_free (sf_interface->last_command);

  g_free (sf_interface);
  sf_interface = NULL;

  /*  
   *  We do not call gtk_main_quit() earlier to reduce the possibility
   *  that script_fu_script_proc() is called from gimp_extension_process()
   *  while we are not finished with the current script. This sucks!
   */

  gtk_main_quit ();
}

static void
script_fu_pattern_preview (gchar    *name,
			   gint      width,
			   gint      height,
			   gint      bytes,
			   gchar    *mask_data,
			   gint      closing,
			   gpointer  data)
{
  gchar **pname;

  pname = (gchar **) data;

  g_free (*pname);
  *pname = g_strdup (name);
}

static void
script_fu_gradient_preview (gchar    *name,
			    gint      width,
			    gdouble  *mask_data,
			    gint      closing,
			    gpointer  data)
{
  gchar **gname;

  gname = (gchar **) data;

  g_free (*gname);
  *gname = g_strdup (name);
}

static void      
script_fu_brush_preview (gchar    *name,
			 gdouble   opacity,
			 gint      spacing,
			 gint      paint_mode,
			 gint      width,
			 gint      height,
			 gchar    *mask_data,
			 gint      closing,
			 gpointer  data)
{
  SFBrush *brush;

  brush = (SFBrush *) data;

  g_free (brush->name);
  brush->name       = g_strdup (name);
  brush->opacity    = opacity;
  brush->spacing    = spacing;
  brush->paint_mode = paint_mode;
}

static void
script_fu_font_preview (GtkWidget *preview,
			gchar     *data)
{
  GdkFont *font;
  gchar *fontname;
  gchar *family;

  if (data == NULL) 
    return;

  fontname = g_strdup (data);

  /* Check if the fontname is valid and the font is present */
  font = gdk_font_load (fontname);

  if (font != NULL)
    {
      gdk_font_unref (font);

      strtok (fontname, "-");
      family = strtok (NULL, "-");
      g_strup (family);

      gtk_label_set_text (GTK_LABEL (preview), family);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (preview), _("NOT SET"));
    }
  
  g_free (fontname);
}

static void
script_fu_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GdkFont   *font;
  GtkWidget *menu_item;
  gchar     *escaped;
  gchar     *text = NULL;
  gchar     *command;
  gchar     *c;
  gchar      buffer[MAX_STRING_LENGTH];
  gint       length;
  gint       i;

  SFScript  *script = (SFScript *) data;
  
  /* Check if choosen fonts are there */
  for (i = 0; i < script->num_args; i++)
    if (script->arg_types[i] == SF_FONT)
      {
	font = gdk_font_load (script->arg_values[i].sfa_font.fontname);
	if (font == NULL)
	  {
	    g_message (_("At least one font you've choosen is invalid.\n"
			 "Please check your settings.\n"));
	    return;
	  }
	else
	  gdk_font_unref (font);
      }
  
  length = strlen (script->script_name) + 3;

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_IMAGE:
      case SF_DRAWABLE:
      case SF_LAYER:
      case SF_CHANNEL:
	length += 12;  /*  Maximum size of integer value will not exceed this many characters  */
	break;

      case SF_COLOR:
	length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
	break;

      case SF_TOGGLE:
	length += 6;   /*  Maximum size of (TRUE, FALSE)  */
	break;

      case SF_VALUE:
	length += strlen (gtk_entry_get_text (GTK_ENTRY (sf_interface->args_widgets[i]))) + 1;
	break;

      case SF_STRING:
	escaped = ESCAPE (gtk_entry_get_text (GTK_ENTRY (sf_interface->args_widgets[i])));
	length += strlen (escaped) + 3;
	g_free (escaped);
	break;

      case SF_ADJUSTMENT:
	length += 24;  /*  Maximum size of float value should not exceed this many characters  */
	break;

      case SF_FILENAME:
	escaped = ESCAPE (script->arg_values[i].sfa_file.filename);
	length += strlen (escaped) + 3;
	g_free (escaped);
	break;

      case SF_FONT:
	length += strlen (script->arg_values[i].sfa_font.fontname) + 3;
	break;

      case SF_PATTERN:
	length += strlen (script->arg_values[i].sfa_pattern) + 3;
	break;

      case SF_GRADIENT:
	length += strlen (script->arg_values[i].sfa_gradient) + 3;
	break;

      case SF_BRUSH:
	length += strlen (script->arg_values[i].sfa_brush.name) + 3;
	length += 36; /* Maximum size of three ints for opacity, spacing,mode*/
	break;

      case SF_OPTION:
	length += 12;  /*  Maximum size of integer value will not exceed this many characters  */
	break;

      default:
	break;
      }

  c = command = g_new (gchar, length);

  sprintf (command, "(%s ", script->script_name);
  c += strlen (script->script_name) + 2;
  for (i = 0; i < script->num_args; i++)
    {
      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  g_snprintf (buffer, sizeof (buffer), "%d",
		      script->arg_values[i].sfa_image);
	  text = buffer;
	  break;

 	case SF_COLOR:
	  g_snprintf (buffer, sizeof (buffer), "'(%d %d %d)",
		      script->arg_values[i].sfa_color[0],
		      script->arg_values[i].sfa_color[1],
		      script->arg_values[i].sfa_color[2]);
	  text = buffer;
	  break;

	case SF_TOGGLE:
	  g_snprintf (buffer, sizeof (buffer), "%s",
		      (script->arg_values[i].sfa_toggle) ? "TRUE" : "FALSE");
	  text = buffer;
	  break;

	case SF_VALUE:
	  text = gtk_entry_get_text (GTK_ENTRY (sf_interface->args_widgets[i]));
	  g_free (script->arg_values[i].sfa_value);
	  script->arg_values[i].sfa_value = g_strdup (text); 
	  break;

	case SF_STRING:
	  text = gtk_entry_get_text (GTK_ENTRY (sf_interface->args_widgets[i]));
	  g_free (script->arg_values[i].sfa_value);
	  script->arg_values[i].sfa_value = g_strdup (text); 
	  escaped = ESCAPE (text);
	  g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
	  g_free (escaped);
	  text = buffer;
	  break;

	case SF_ADJUSTMENT:
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      script->arg_values[i].sfa_adjustment.value =
		script->arg_values[i].sfa_adjustment.adj->value;
	      g_snprintf (buffer, sizeof (buffer), "%f",
			  script->arg_values[i].sfa_adjustment.value);
	      text = buffer;
	      break;

	    case SF_SPINNER:
	      script->arg_values[i].sfa_adjustment.value = 
		gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (sf_interface->args_widgets[i]));
	      g_snprintf (buffer, sizeof (buffer), "%f",
			  script->arg_values[i].sfa_adjustment.value);
	      text = buffer;
	      break;

	    default:
	      break;
	    }
	  break;

	case SF_FILENAME:
	  escaped = ESCAPE (script->arg_values[i].sfa_file.filename);
	  g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
	  g_free (escaped);
	  text = buffer;
	  break;

	case SF_FONT:
	  g_snprintf (buffer, sizeof (buffer), "\"%s\"",
		      script->arg_values[i].sfa_font.fontname);
	  text = buffer;
	  break;

	case SF_PATTERN:
	  g_snprintf (buffer, sizeof (buffer), "\"%s\"",
		      script->arg_values[i].sfa_pattern);
	  text = buffer;
	  break;

	case SF_GRADIENT:
	  g_snprintf (buffer, sizeof (buffer), "\"%s\"",
		      script->arg_values[i].sfa_gradient);
	  text = buffer;
	  break;

	case SF_BRUSH:
	  g_snprintf (buffer, sizeof (buffer), "'(\"%s\" %f %d %d)",
		      script->arg_values[i].sfa_brush.name,
		      script->arg_values[i].sfa_brush.opacity,
		      script->arg_values[i].sfa_brush.spacing,
		      script->arg_values[i].sfa_brush.paint_mode);
	  text = buffer;
	  break;

	case SF_OPTION:
	  menu_item = 
	    gtk_menu_get_active (GTK_MENU (gtk_option_menu_get_menu (GTK_OPTION_MENU (sf_interface->args_widgets[i]))));
	  script->arg_values[i].sfa_option.history = 
	    GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (menu_item))); 
	  g_snprintf (buffer, sizeof (buffer), "%d",
		      script->arg_values[i].sfa_option.history);
	  text = buffer;
	  break;

	default:
	  break;
	}

      if (i == script->num_args - 1)
	sprintf (c, "%s)", text);
      else
	sprintf (c, "%s ", text);
      c += strlen (text) + 1;
    }

  /*  run the command through the interpreter  */
  if (repl_c_string (command, 0, 0, 1) != 0)
    script_fu_error_msg (command);

  g_free (command);

  script_fu_interface_quit (script);
}

static void
script_fu_about_callback (GtkWidget *widget,
			  gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *text;
  GtkWidget *vscrollbar;

  SFScript  *script = (SFScript *) data;

  if (sf_interface->about_dialog == NULL)
    {
      dialog = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (dialog), sf_interface->window_title);
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
      gtk_signal_connect_object (GTK_OBJECT (dialog), "delete_event",
				 GTK_SIGNAL_FUNC (gtk_widget_destroy),
				 GTK_OBJECT (dialog));

      gimp_help_connect_help_accel (dialog, gimp_standard_help_func,
				    "filters/script-fu.html");
  
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
			  TRUE, TRUE, 0);
      gtk_widget_show (frame);
      
      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_widget_show (vbox);

      /* the name */
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
      
      label = gtk_label_new (script->script_name);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);
 
      /* the help display */
      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      gtk_widget_show (table);

      text = gtk_text_new (NULL, NULL);
      gtk_text_set_editable (GTK_TEXT (text), FALSE);
      gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);      
      gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_set_usize (text, 200, 60);
      gtk_widget_show (text);

      vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
      gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (vscrollbar);

      gtk_widget_realize (text);
      gtk_text_freeze (GTK_TEXT (text));
      gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL,
		       script->help, -1);
   
      /* author, copyright, etc. */
      table = gtk_table_new (2, 4, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (script->author);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Author:"), 1.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->copyright);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Copyright:"), 1.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->date);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				 _("Date:"), 1.0, 0.5,
				 label, 1, FALSE);

      if (strlen (script->img_types) > 0)
	{
	  label = gtk_label_new (script->img_types);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 

	  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
				     _("Image Types:"), 1.0, 0.5,
				     label, 1, FALSE);
	}

      gtk_widget_show (frame);
      gtk_text_thaw (GTK_TEXT (text));

      /*  action area  */
      gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
      button = gtk_button_new_with_label (_("Close"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_destroy),
				 GTK_OBJECT (dialog));
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      sf_interface->about_dialog = dialog;

      gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &sf_interface->about_dialog);
    }

  gtk_window_set_position (GTK_WINDOW (sf_interface->about_dialog),
			   GTK_WIN_POS_MOUSE);  
  gtk_widget_show (sf_interface->about_dialog);
}

static void
script_fu_reset_callback (GtkWidget *widget,
			  gpointer   data)
{
  gint i, j;

  SFScript *script = (SFScript *) data;

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_IMAGE:
      case SF_DRAWABLE:
      case SF_LAYER:
      case SF_CHANNEL:
	break;

      case SF_COLOR:
	for (j = 0; j < 3; j++)
	  {
	    script->arg_values[i].sfa_color[j] = 
	      script->arg_defaults[i].sfa_color[j];
	  }
	gimp_color_button_update (GIMP_COLOR_BUTTON (sf_interface->args_widgets[i]));
	break;

      case SF_TOGGLE:
	script->arg_values[i].sfa_toggle = script->arg_defaults[i].sfa_toggle;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sf_interface->args_widgets[i]),
				      script->arg_values[i].sfa_toggle);
	break;

      case SF_VALUE:
      case SF_STRING:
	g_free (script->arg_values[i].sfa_value);
	script->arg_values[i].sfa_value =
	  g_strdup (script->arg_defaults[i].sfa_value); 
	gtk_entry_set_text (GTK_ENTRY (sf_interface->args_widgets[i]), 
			    script->arg_values[i].sfa_value);
	break;

      case SF_ADJUSTMENT:
	script->arg_values[i].sfa_adjustment.value =
	  script->arg_defaults[i].sfa_adjustment.value;
	gtk_adjustment_set_value (script->arg_values[i].sfa_adjustment.adj, 
				  script->arg_values[i].sfa_adjustment.value);
	break;

      case SF_FILENAME:
	g_free (script->arg_values[i].sfa_file.filename);
	script->arg_values[i].sfa_file.filename =
	  g_strdup (script->arg_defaults[i].sfa_file.filename);
	gimp_file_selection_set_filename
	  (GIMP_FILE_SELECTION (script->arg_values[i].sfa_file.fileselection),
	   script->arg_values[i].sfa_file.filename);
	break;

      case SF_FONT:
	g_free (script->arg_values[i].sfa_font.fontname);
	script->arg_values[i].sfa_font.fontname =
	  g_strdup (script->arg_defaults[i].sfa_font.fontname);
	if (script->arg_values[i].sfa_font.dialog)
	  {
	    gtk_font_selection_dialog_set_font_name
	      (GTK_FONT_SELECTION_DIALOG (script->arg_values[i].sfa_font.dialog),
	       script->arg_values[i].sfa_font.fontname);
	  }	
	script_fu_font_preview (script->arg_values[i].sfa_font.preview,
				script->arg_values[i].sfa_font.fontname);
	break;
#if 0
      case SF_PATTERN:
  	gimp_pattern_select_widget_set_popup
	  (sf_interface->args_widgets[i], script->arg_defaults[i].sfa_pattern);  
	break;
#endif
      case SF_GRADIENT:
  	gimp_gradient_select_widget_set_popup
	  (sf_interface->args_widgets[i], script->arg_defaults[i].sfa_gradient);  
	break;

      case SF_BRUSH:
#if 0
  	gimp_brush_select_widget_set_popup
	  (sf_interface->args_widgets[i],
	   script->arg_defaults[i].sfa_brush.name,
	   script->arg_defaults[i].sfa_brush.opacity, 
	   script->arg_defaults[i].sfa_brush.spacing, 
	   script->arg_defaults[i].sfa_brush.paint_mode);  
#endif
	break;

      case SF_OPTION:
	script->arg_values[i].sfa_option.history = 
	  script->arg_defaults[i].sfa_option.history;
	gtk_option_menu_set_history (GTK_OPTION_MENU (sf_interface->args_widgets[i]), 
				     script->arg_values[i].sfa_option.history);

      default:
	break;
      }
}

static void
script_fu_menu_callback  (gint32   id,
			  gpointer data)
{
  *((gint32 *) data) = id;
}

static void
script_fu_toggle_update (GtkWidget *widget,
			 gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
script_fu_file_selection_callback (GtkWidget *widget,
				   gpointer   data)
{
  SFFilename *file;

  file = (SFFilename *) data;

  if (file->filename)
    g_free (file->filename);

  file->filename = 
    gimp_file_selection_get_filename (GIMP_FILE_SELECTION (file->fileselection));
}

static void
script_fu_font_preview_callback (GtkWidget *widget,
				 gpointer   data)
{
  GtkFontSelectionDialog *fsd;
  SFFont *font;

  font = (SFFont *) data;

  if (!font->dialog)
    {
      font->dialog = gtk_font_selection_dialog_new (_("Script-Fu Font Selection"));
      fsd = GTK_FONT_SELECTION_DIALOG (font->dialog);

      gtk_signal_connect (GTK_OBJECT (fsd->ok_button), "clicked",
			  GTK_SIGNAL_FUNC (script_fu_font_dialog_ok),
			  font);
      gtk_signal_connect (GTK_OBJECT (fsd), "delete_event",
			  GTK_SIGNAL_FUNC (script_fu_font_dialog_delete),
			  font);
      gtk_signal_connect (GTK_OBJECT (fsd), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &font->dialog);
      gtk_signal_connect (GTK_OBJECT (fsd->cancel_button), "clicked",
			  GTK_SIGNAL_FUNC (script_fu_font_dialog_cancel),
			  font);
    }
  else
    fsd = GTK_FONT_SELECTION_DIALOG (font->dialog);

  gtk_font_selection_dialog_set_font_name (fsd, font->fontname);
  gtk_window_set_position (GTK_WINDOW (font->dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_show (font->dialog);
}

static void
script_fu_font_dialog_ok (GtkWidget *widget,
			  gpointer   data)
{
  SFFont *font;
  gchar  *fontname;

  font = (SFFont *) data;

  fontname = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (font->dialog));
  if (fontname != NULL)
    {
      g_free (font->fontname);
      font->fontname = fontname;
    }
  gtk_widget_hide (font->dialog);

  script_fu_font_preview (font->preview, font->fontname);
}

static void
script_fu_font_dialog_cancel (GtkWidget *widget,
			      gpointer   data)
{
  SFFont *font;

  font = (SFFont *) data;

  gtk_widget_hide (font->dialog);
}

static gint
script_fu_font_dialog_delete (GtkWidget *widget,
			      GdkEvent  *event,
			      gpointer   data)
{
  script_fu_font_dialog_cancel (widget, data);
  return TRUE;
}

static void
script_fu_error_msg (gchar *command)
{
  g_message (_("Script-Fu Error while executing\n %s\n%s"), 
	     command, siod_err_msg);
}
