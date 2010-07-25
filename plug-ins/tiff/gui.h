/*
 * "$Id: gui.h,v 1.1 2004/11/23 12:17:14 beku Exp $"
 *
 *   gui stuff for CinePaint
 *
 *   Copyright 2003-2004   Kai-Uwe Behrmann (ku.b@gmx.de)
 *
 *   separate gui stuff March 2004
 *    beku <ku.b@gmx.de>
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

#ifndef _GUI_H_
#define _GUI_H_

/*** function definitions ***/

  /* --- Gui functions --- */
gint   save_dialog          (   ImageInfo        *info);

#if !(GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 3)
void   ok_callback          (   GtkWidget        *widget,
				gpointer          data);
void   cancel_callback     (   GtkWidget        *widget,
				gpointer          data);
void   text_entry_callback  (   GtkWidget        *widget,
                                gpointer          data);
#endif
#if !GIMP_MAJOR_VERSION >= 1
void   close_callback       (   GtkWidget        *widget,
                                gpointer          data);
#endif

#endif /*_GUI_H_ */
