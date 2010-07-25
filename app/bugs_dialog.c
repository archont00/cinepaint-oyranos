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

/* based on about_dialog.c */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>
#include "../lib/version.h"
#include "../lib/compat.h"

#include "bugs_dialog.h"
#include "interface.h"

#include "config.h"
#include "minimize.h"
#include "app_procs.h"
#include "../lib/wire/datadir.h"

#ifdef WIN32
#include "unistd.h"
#include "extra.h"
#endif

static void bugs_dialog_destroy (void);
static void bugs_dialog_unmap (void);
static int  bugs_dialog_button (GtkWidget *widget, GdkEventButton *event);

static GtkWidget *bugs_dialog = NULL;
static int frame = 0;
static int offset = 0;
static int timer = 0;

void
bugs_dialog_create (int timeout)
{
  GtkStyle *style;
  GtkWidget *vbox;
  GtkWidget *aboutframe;
  GtkWidget *label;
  GtkWidget *alignment;
/*  gint max_width;
  gint i;
*/
  const char* info= "www.cinepaint.org\n\n"
	"All non-trivial software contains bugs.\n"
	" CinePaint developers are typically unpaid volunteers which \n"
	"may imply greater or lesser care than other programs.\n\n"
	"Please help us improve CinePaint by reporting bugs to \n"
	"our developers mailing list at\n"
	"cinepaint-developers@lists.sourceforge.net.\n"
	"For further details about posting to the list see\n" 
	"http://cinepaint.sourceforge.net/docs/mailing.list.html\n\n"
	"If you are unable or unwilling to post to a public list\n"
	"send email to Robin Rowe at rower@movieeditor.com.\n\n"
	"Kudos, especially those describing CinePaint in use\n"
	"in motion picture production, are appreciated.\n\n"
	"Thanks!\n";

  if (!bugs_dialog)
    {
#if GTK_MAJOR_VERSION > 1
      bugs_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_type_hint (GTK_WINDOW(bugs_dialog),
                                GDK_WINDOW_TYPE_HINT_DIALOG);
#else
      bugs_dialog = gtk_window_new (GTK_WINDOW_DIALOG);
#endif
      minimize_register(bugs_dialog);
      gtk_window_set_wmclass (GTK_WINDOW (bugs_dialog), "bugs_dialog", PROGRAM_NAME);
      gtk_window_set_title (GTK_WINDOW (bugs_dialog), "Where to send bug reports and kudos");
      gtk_window_set_policy (GTK_WINDOW (bugs_dialog), FALSE, FALSE, FALSE);
      gtk_window_position (GTK_WINDOW (bugs_dialog), GTK_WIN_POS_CENTER);
      gtk_signal_connect (GTK_OBJECT (bugs_dialog), "destroy",
			  (GtkSignalFunc) bugs_dialog_destroy, NULL);
      gtk_signal_connect (GTK_OBJECT (bugs_dialog), "unmap_event",
			  (GtkSignalFunc) bugs_dialog_unmap, NULL);
      gtk_signal_connect (GTK_OBJECT (bugs_dialog), "button_press_event",
			  (GtkSignalFunc) bugs_dialog_button, NULL);
      gtk_widget_set_events (bugs_dialog, GDK_BUTTON_PRESS_MASK);

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (bugs_dialog), vbox);
      gtk_widget_show (vbox);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_box_pack_start (GTK_BOX (vbox), aboutframe, TRUE, TRUE, 0);
      gtk_widget_show (aboutframe);

      style = gtk_style_new ();
#if 0
      gdk_font_unref (gtk_style_get_font(style));
      gtk_style_set_font(style, gdk_font_load ("-Adobe-Helvetica-Medium-R-Normal--*-140-*-*-*-*-*-*"));
#endif
#if GTK_MAJOR_VERSION < 2
      gtk_widget_push_style (style);

      gtk_widget_pop_style ();
#endif

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);
      gtk_widget_show (alignment);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_container_add (GTK_CONTAINER (alignment), aboutframe);
      gtk_widget_show (aboutframe);

      label = gtk_label_new (info);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);
    }

  if (!GTK_WIDGET_VISIBLE (bugs_dialog))
    {
      gtk_widget_show (bugs_dialog);
    }
  else 
    {
      gdk_window_raise(bugs_dialog->window);
    }
}


static void
bugs_dialog_destroy ()
{
  bugs_dialog = NULL;
  bugs_dialog_unmap ();
}

static void
bugs_dialog_unmap ()
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}


static int
bugs_dialog_button (GtkWidget      *widget,
		     GdkEventButton *event)
{
  if (timer)
    gtk_timeout_remove (timer);
  timer = 0;
  frame = 0;

  gtk_widget_hide (bugs_dialog);

  return FALSE;
}

