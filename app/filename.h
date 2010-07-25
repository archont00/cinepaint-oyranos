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

#ifndef __FILENAME_H__
#define __FILENAME_H__

/*
 * Image sequence filename functions.
 * These all assume that the image full path can be broken into:
 *  [path][pre][frame][post]
 * Path ends with the last slash in the name.
 * Pre starts at the first character after the last slash.
 * Frame is the last group of digits and is always positive.
 * Name is [pre][frame][post]
 */


/* return the frame number value, 0 if none */
int filename_get_frame_number(const char* filename);

/* these all return a malloced string, or 0 on failure */

/* replace the current frame number with value */
char* filename_set_frame(const char* filename, int value);

/* increment the current frame number by value */
char* filename_inc_frame(const char* filename, int value);

/* copy the filename up to the frame number */
char* filename_get_pre(const char* filename);

/* copy the frame number part of the filename */
char* filename_get_frame(const char* filename);

/* copy everything after the frame number */
char* filename_get_post(const char* filename);

/* build path as [dir][name], replace frame with value */
char* filename_build_path(const char* dir, const char* name, int value);

/* build path as [dir][name], replace frame with generic symbol */
char* filename_build_label(const char* dir, const char* name);


#endif
