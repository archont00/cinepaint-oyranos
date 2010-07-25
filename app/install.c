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

/* Modified on 20 January 2003 to update the "Personal Film Gimp
 * Installation" help screen to reflect that we are running Film
 * Gimp and not GIMP.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtktext.h>
#ifndef GTK_ENABLE_BROKEN
#define GTK_ENABLE_BROKEN
#endif

#include "config.h"
#include "libgimp/gimpintl.h"
#include "../lib/version.h"
#include "appenv.h"
#include "actionarea.h"
#include "install.h"
#include "interface.h"
#include "rc.h"
#include "../lib/wire/datadir.h"
#ifdef WIN32
#include "../win32/UserInstall.h"
#endif

static void install_run (InstallCallback);
static void install_help (InstallCallback);
static void help_install_callback (GtkWidget *, gpointer);
static void help_ignore_callback (GtkWidget *, gpointer);
static void help_quit_callback (GtkWidget *, gpointer);
static void install_continue_callback (GtkWidget *, gpointer);
static void install_quit_callback (GtkWidget *, gpointer);

static GtkWidget *help_widget;
static GtkWidget *install_widget;

#if GTK_MAJOR_VERSION > 1
#define CP_TEXT_INSERT( font, txt ) \
  gtk_text_buffer_insert (text_buf, &iter, txt, -1);
#else
#define CP_TEXT_INSERT( font, txt ) \
  gtk_text_insert (GTK_TEXT (text), font, NULL, NULL, txt, -1 );
#endif


void
install_verify (InstallCallback install_callback)
{
  int properly_installed = TRUE;
  char *filename;
  struct stat stat_buf;

  filename = gimp_directory ();
  if ('\000' == filename[0])
    {
      g_message ("No home directory--skipping GIMP user installation.");
      (* install_callback) ();
    }

  if (stat (filename, &stat_buf) != 0)
    properly_installed = FALSE;

  /*  If there is already a proper installation, invoke the callback  */
  if (properly_installed)
    {
      (* install_callback) ();
    }
  /*  Otherwise, prepare for installation  */
  else if (no_interface)
    {
      g_print (PROGRAM_NAME " is not properly installed for the current user\n");
      g_print ("User installation was skipped because the '--nointerface' flag was encountered\n");
      g_print ("To perform user installation, run the GIMP without the '--nointerface' flag\n");

      (* install_callback) ();
    }
  else
    {
      install_help (install_callback);
    }
}


/*********************/
/*  Local functions  */

static void
install_help (InstallCallback callback)
{
  GtkWidget *text;
#if GTK_MAJOR_VERSION > 1
  GtkTextBuffer * text_buf;
  GtkTextIter   iter;
#endif
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font_emphasis;
  GdkFont   *font;
  static ActionAreaItem action_items[] =
  {
    { "Install", help_install_callback, NULL, NULL },
    { "Ignore", help_ignore_callback, NULL, NULL },
    { "Quit", help_quit_callback, NULL, NULL }
  };

  action_items[0].label = _("Install");
  action_items[1].label = _("Ignore");
  action_items[2].label = _("Quit");

  help_widget = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (help_widget), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_true),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (help_widget), 
			  PROGRAM_NAME "_installation", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (help_widget), 
			PROGRAM_NAME " Installation");
  gtk_window_position (GTK_WINDOW (help_widget), GTK_WIN_POS_CENTER);

  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb  = gtk_vscrollbar_new (vadj);
#if GTK_MAJOR_VERSION > 1
  text = gtk_text_view_new();
  text_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_get_iter_at_offset (text_buf, &iter, 0);  
#else
  text = gtk_text_new (NULL, vadj);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
#endif
  gtk_widget_set_usize (text, 450, 475);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

  action_items[0].user_data = (void *) callback;
  action_items[1].user_data = (void *) callback;
  action_items[2].user_data = (void *) callback;
  build_action_area (GTK_DIALOG (help_widget), action_items, 3, 0);


  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (help_widget)->vbox), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    0, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_container_border_width (GTK_CONTAINER (table), 2);

#if GTK_MAJOR_VERSION < 2
  font_strong = gdk_font_load ("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*");
  font_emphasis = gdk_font_load ("-*-helvetica-medium-o-normal-*-*-100-*-*-*-*-*-*");
  font = gdk_font_load ("-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*");
#else
  font_strong = gdk_font_load ("helvetica 12");
  font_emphasis = gdk_font_load ("helvetica r 10");
  font = gdk_font_load ("helvetica");
#endif

  /*  Realize the widget before allowing new text to be inserted  */
  gtk_widget_realize (text);

  CP_TEXT_INSERT( font_strong, PROGRAM_NAME "\n\n");
  CP_TEXT_INSERT( font_emphasis, COPYRIGHT "\n");
  CP_TEXT_INSERT( font, "\n");
  CP_TEXT_INSERT( font, "This program is free software; you can redistribute it and/or modify\n");
  CP_TEXT_INSERT( font, "it under the terms of the GNU General Public License as published by\n");
  CP_TEXT_INSERT( font, "the Free Software Foundation; either version 2 of the License, or\n");
  CP_TEXT_INSERT( font, "(at your option) any later version.\n");
  CP_TEXT_INSERT( font, "\n");
  CP_TEXT_INSERT( font, "This program is distributed in the hope that it will be useful,\n");
  CP_TEXT_INSERT( font, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  CP_TEXT_INSERT( font, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
  CP_TEXT_INSERT( font, "See the GNU General Public License for more details.\n");
 CP_TEXT_INSERT( font,
		   "\n");
  CP_TEXT_INSERT( font,
		   "You should have received a copy of the GNU General Public License\n");
  CP_TEXT_INSERT( font,
		   "along with this program; if not, write to the Free Software\n");
  CP_TEXT_INSERT( font,
		   "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n");
  CP_TEXT_INSERT( font,
		   "\n\n");

  CP_TEXT_INSERT( font_strong,
		   "Personal " PROGRAM_NAME " Installation\n\n");
  CP_TEXT_INSERT( font,
		   "For a proper " PROGRAM_NAME " installation, a subdirectory called\n");
  CP_TEXT_INSERT( font_emphasis,
		   gimp_directory ());
  CP_TEXT_INSERT( font,
		   " needs to be created.  This\n");
  CP_TEXT_INSERT( font,
		   "subdirectory will contain a number of important files:\n\n");
  CP_TEXT_INSERT( font_emphasis,
		   "gimprc\n");
  CP_TEXT_INSERT( font,
		   "\t\tThe gimprc is used to store personal preferences\n");
  CP_TEXT_INSERT( font,
		   "\t\tsuch as default " PROGRAM_NAME " behaviors & plug-in hotkeys.\n");
  CP_TEXT_INSERT( font,
		   "\t\tPaths to search for brushes, palettes, gradients\n");
  CP_TEXT_INSERT( font,
		   "\t\tpatterns, and plug-ins are also configured here.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "pluginrc\n");
  CP_TEXT_INSERT( font,
		   "\t\tPlug-ins and extensions are extern programs run by\n");
  CP_TEXT_INSERT( font,
		   "\t\t" PROGRAM_NAME " which provide additional functionality.\n");
  CP_TEXT_INSERT( font,
		   "\t\tThese programs are searched for at run-time and\n");
  CP_TEXT_INSERT( font,
		   "\t\tinformation about their functionality and mod-times\n");
  CP_TEXT_INSERT( font,
		   "\t\tis cached in this file.  This file is intended to\n");
  CP_TEXT_INSERT( font,
		   "\t\tbe " PROGRAM_NAME " readable only, and should not be edited.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "brushes\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined brushes.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks this subdirectory in addition to the system-\n");
  CP_TEXT_INSERT( font,
		   "\t\twide " PROGRAM_NAME " brushes installation when searching for\n");
  CP_TEXT_INSERT( font,
		   "\t\tbrushes.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "curves\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined curves.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks this subdirectory in addition to the system-\n");
  CP_TEXT_INSERT( font,
		   "\t\twide " PROGRAM_NAME " curves installation when searching for\n");
  CP_TEXT_INSERT( font,
		   "\t\tcurves.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "gradients\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined gradients.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks this subdirectory in addition to the system-\n");
  CP_TEXT_INSERT( font,
		   "\t\twide " PROGRAM_NAME " gradients installation when searching for\n");
  CP_TEXT_INSERT( font,
		   "\t\tgradients.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "gfig\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined figures to be used by the gfig plug-in.\n");
  CP_TEXT_INSERT( font,
		   "\t\tThe default gimprc file checks this subdirectory in\n");
  CP_TEXT_INSERT( font,
		   "\t\taddition to the systemwide gimp gfig installation\n");
  CP_TEXT_INSERT( font,
		   "\t\twhen searching for gfig figures.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "gflares\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined gflares to be used by the gflare plug-in.\n");
  CP_TEXT_INSERT( font,
		   "\t\tThe default gimprc file checks this subdirectory in\n");
  CP_TEXT_INSERT( font,
		   "\t\taddition to the systemwide gimp gflares installation\n");
  CP_TEXT_INSERT( font,
		   "\t\twhen searching for gflares.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "palettes\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined palettes.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks only this subdirectory (not the system-wide\n");
  CP_TEXT_INSERT( font,
		   "\t\tinstallation) when searching for palettes.  During\n");
  CP_TEXT_INSERT( font,
		   "\t\tinstallation, the system palettes will be copied\n");
  CP_TEXT_INSERT( font,
		   "\t\there.  This is done to allow modifications made to\n");
  CP_TEXT_INSERT( font,
		   "\t\tpalettes during " PROGRAM_NAME " execution to persist across\n");
  CP_TEXT_INSERT( font,
		   "\t\tsessions.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "patterns\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser defined patterns.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks this subdirectory in addition to the system-\n");
  CP_TEXT_INSERT( font,
		   "\t\twide " PROGRAM_NAME " patterns installation when searching for\n");
  CP_TEXT_INSERT( font,
		   "\t\tpatterns.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "plug-ins\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis is a subdirectory which can be used to store\n");
  CP_TEXT_INSERT( font,
		   "\t\tuser created, temporary, or otherwise non-system-\n");
  CP_TEXT_INSERT( font,
		   "\t\tsupported plug-ins.  The default gimprc file\n");
  CP_TEXT_INSERT( font,
		   "\t\tchecks this subdirectory in addition to the system-\n");
  CP_TEXT_INSERT( font,
		   "\t\twide " PROGRAM_NAME " plug-in directories when searching for\n");
  CP_TEXT_INSERT( font,
		   "\t\tplug-ins.\n");
  CP_TEXT_INSERT( font_emphasis,
		   "scripts\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis subdirectory is used by the GIMP to store \n");
  CP_TEXT_INSERT( font,
		   "\t\tuser created and installed scripts. The default gimprc\n");
  CP_TEXT_INSERT( font,
		   "\t\tfile checks this subdirectory in addition to the system\n");
  CP_TEXT_INSERT( font,
		   "\t\t-wide " PROGRAM_NAME " scripts subdirectory when searching for scripts\n");
  CP_TEXT_INSERT( font_emphasis,
		   "tmp\n");
  CP_TEXT_INSERT( font,
		   "\t\tThis subdirectory is used by " PROGRAM_NAME " to temporarily\n");
  CP_TEXT_INSERT( font,
		   "\t\tstore undo buffers to reduce memory usage.  If " PROGRAM_NAME " is\n");
  CP_TEXT_INSERT( font,
		   "\t\tunceremoniously killed, files may persist in this directory\n");
  CP_TEXT_INSERT( font,
		   "\t\tof the form: gimp<#>.<#>.  These files are useless across\n");
  CP_TEXT_INSERT( font,
		   "\t\t" PROGRAM_NAME " sessions and can be destroyed with impunity.\n");

  gtk_widget_show (vsb);
  gtk_widget_show (text);
  gtk_widget_show (table);
  gtk_widget_show (help_widget);
}

static void
help_install_callback (GtkWidget *w,
		       gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  install_run (callback);
}

static void
help_ignore_callback (GtkWidget *w,
		      gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (help_widget);
  (* callback) ();
}

static void
help_quit_callback (GtkWidget *w,
		    gpointer   client_data)
{
  gtk_widget_destroy (help_widget);
  gtk_exit (0);
}

static void
install_run (InstallCallback callback)
{
  GtkWidget *text;
#if GTK_MAJOR_VERSION > 1
  GtkTextBuffer * text_buf;
  GtkTextIter   iter;
#endif
  GtkWidget *table;
  GtkWidget *vsb;
  GtkAdjustment *vadj;
  GdkFont   *font_strong;
  GdkFont   *font;
  FILE *pfp;
  char buffer[2048];
  struct stat stat_buf;
  int err;
  int executable = TRUE;
  static ActionAreaItem action_items[] =
  {
    { "Continue", install_continue_callback, NULL, NULL },
    { "Quit", install_quit_callback, NULL, NULL }
  };

  action_items[0].label = _("Continue");
  action_items[1].label = _(action_items[1].label);

  install_widget = gtk_dialog_new ();
  gtk_signal_connect (GTK_OBJECT (install_widget), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_true),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (install_widget), "installation_log", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (install_widget), _("Installation Log"));
  gtk_window_position (GTK_WINDOW (install_widget), GTK_WIN_POS_CENTER);
  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb  = gtk_vscrollbar_new (vadj);
#if GTK_MAJOR_VERSION > 1
  text = gtk_text_view_new();
  text_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  gtk_text_buffer_get_iter_at_offset (text_buf, &iter, 0);  
#else
  text = gtk_text_new (NULL, vadj);
#endif
  gtk_widget_set_usize (text, 384, 256);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

  action_items[0].user_data = (void *) callback;
  action_items[1].user_data = (void *) callback;
  build_action_area (GTK_DIALOG (install_widget), action_items, 2, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (install_widget)->vbox), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    0, 0);

  gtk_container_border_width (GTK_CONTAINER (table), 2);

#if GTK_MAJOR_VERSION > 1
#else
  font_strong = gdk_font_load ("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*");
  font = gdk_font_load ("-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*");
#endif

  /*  Realize the text widget before inserting text strings  */
  gtk_widget_realize (text);


  CP_TEXT_INSERT( font_strong, _("User Installation Log\n\n") );

#ifdef WIN32
	{	const char* msg = UserInstall(gimp_directory());
		if(msg)
		{
                        CP_TEXT_INSERT( font, msg );
			msg=_("\nInstallation failed!\n");
		}
		else
		{	msg=_("\nInstallation successful!\n");
		}
                CP_TEXT_INSERT( font, msg );
	}
#else
  /*  Generate output  */
  sprintf (buffer, "%s/user_install", GetDirData());
  if ((err = stat (buffer, &stat_buf)) != 0)
    {
      CP_TEXT_INSERT( font, buffer );
      CP_TEXT_INSERT( font, _(" does not exist.  Cannot install.\n") );
      executable = FALSE;
    }
  else if (! (S_IXUSR & stat_buf.st_mode) || ! (S_IRUSR & stat_buf.st_mode))
    {
      CP_TEXT_INSERT( font, buffer );
      CP_TEXT_INSERT( font, _(" has invalid permissions.\nCannot install.") );
      executable = FALSE;
    }

  if (executable == TRUE)
    { /* user_install is a script that copies files */
      sprintf (buffer, "%s/user_install %s %s", GetDirData(), GetDirData(),
	       gimp_directory ());
      if ((pfp = popen (buffer, "r")) != NULL)
	{
	  while (fgets (buffer, 2048, pfp))
            CP_TEXT_INSERT( font, buffer );
	  pclose (pfp);

          CP_TEXT_INSERT( font, _("\nInstallation successful!\n") );
	}
      else
	executable = FALSE;
    }
  if (executable == FALSE)
    CP_TEXT_INSERT( font, _("\nInstallation failed.  Contact system administrator.\n") );
#endif
  gtk_widget_show (vsb);
  gtk_widget_show (text);
  gtk_widget_show (table);
  gtk_widget_show (install_widget);
}

static void
install_continue_callback (GtkWidget *w,
			   gpointer   client_data)
{
  InstallCallback callback;

  callback = (InstallCallback) client_data;
  gtk_widget_destroy (install_widget);
  (* callback) ();
}

static void
install_quit_callback (GtkWidget *w,
		       gpointer   client_data)
{
  gtk_widget_destroy (install_widget);
  gtk_exit (0);
}
