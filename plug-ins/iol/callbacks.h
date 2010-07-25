/* callbacks.h - Function prototypes for callbacks.c
 *
 * Copyright (C) 2003 Sean Ridenour.
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
 *
 * Read the file COPYING for the complete licensing terms.
 */

#include "iol.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdk.h>

void
on_load_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_go_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);


gboolean
on_iol_window_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_load_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_load_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_script_text_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_iol_window_show                     (GtkWidget       *widget,
                                        gpointer         user_data);


void
on_iol_window_show                     (GtkWidget       *widget,
                                        gpointer         user_data);


void
on_help_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);


void
on_keepopen_check_button_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


void
on_help_close_button_clicked           (GtkButton       *button,
                                        gpointer         user_data);
