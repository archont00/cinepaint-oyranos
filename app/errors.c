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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef WIN32

#else
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#endif

#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include "appenv.h"
#include "app_procs.h"
#include "interface.h"
#include "errors.h"

extern char *prog_name;

#if GTK_MAJOR_VERSION > 1
void
message_func_cb (const gchar    *log_domain,
                   GLogLevelFlags  log_level,
                   const gchar    *str,
                   gpointer        data)
{
  message_func(str);
}
#endif


void
message_func (const char *str)
{
  if ((console_messages == FALSE) && (message_handler == MESSAGE_BOX))
      message_box (str, NULL, NULL);
  else
      fprintf (stderr, "%s: %s\n", prog_name, str);
}

void
fatal_error (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf ("%s fatal error: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  g_on_error_query (prog_name);
  app_exit (1);
}

void
terminate (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf ("%s terminated: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  if (use_debug_handler)
    g_on_error_query (prog_name);

  app_exit (1);
  gdk_exit (1);

}

