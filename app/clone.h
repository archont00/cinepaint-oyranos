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
#ifndef __CLONE_H__
#define __CLONE_H__

#include "gimage.h"
#include "gdisplay.h"
#include "../lib/wire/c_typedefs.h"

Tool *  tools_new_clone   (void);
void            tools_free_clone  (Tool *);
void		clone_delete_image (GImage *gimage);
void		clone_flip_image ();
void		clone_expose ();
void		clone_leave_notify ();
void		clone_enter_notify ();
void		clone_x_offset_increase ();
void		clone_x_offset_decrease ();
void		clone_y_offset_increase ();
void		clone_y_offset_decrease ();
void		clone_reset_offset ();
int		clone_get_x_offset ();
int		clone_get_y_offset ();
int		clone_is_src (GDisplay*);

extern ProcRecord clone_proc;

#endif  /*  __CLONE_H__  */
