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
#ifndef __APP_PROCS_H__
#define __APP_PROCS_H__

#ifdef WIN32
typedef int key_t;
#endif

#include <gtk/gtk.h>

/* Function declarations */
void gimp_init (int, char **);
void app_init (void);
void app_exit (int);
void app_exit_finish (void);
int app_exit_finish_done (void);
void app_init_update_status(char *label1val, char *label2val, float pct_progress);
int splash_logo_load (GtkWidget *window);
extern GdkPixmap *logo_pixmap;

#define SPLASH_FILE "spot.splash.ppm"

#endif /* APP_PROCS_H */
