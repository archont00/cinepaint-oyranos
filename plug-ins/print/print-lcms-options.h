/*
 * "$Id: print-lcms-options.h,v 1.1 2005/02/13 22:15:23 beku Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 2003-2004   Kai-Uwe Behrmann (web@tiscali.de)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/*
 *   This sections is intendet to set some options before converting an 
 *   cinepaint image into cmyk 16-bit colormode for gimp-print.
 */

#include <gtk/gtk.h>


GtkWidget* create_fileselection1 (GtkWidget *);
GtkWidget* create_lcms_options_window (void);
void       init_lcms_options (void);

void
on_file_help_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

