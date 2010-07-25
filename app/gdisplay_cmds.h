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
#ifndef  __GDISPLAY_CMDS_H__
#define  __GDISPLAY_CMDS_H__

#include "procedural_db.h"

extern ProcRecord gdisplay_new_proc;
extern ProcRecord gdisplay_fm_proc;
extern ProcRecord gdisplay_active_proc;
extern ProcRecord gdisplay_delete_proc;
extern ProcRecord gdisplays_delete_image_proc;
extern ProcRecord gdisplay_update_proc;
extern ProcRecord gdisplays_flush_proc;
extern ProcRecord gdisplay_get_image_id_proc;
extern ProcRecord gdisplay_get_cms_intent_proc;
extern ProcRecord gdisplay_set_cms_intent_proc;
extern ProcRecord gdisplay_get_cms_flags_proc;
extern ProcRecord gdisplay_set_cms_flags_proc;
extern ProcRecord gdisplay_is_colormanaged_proc;
extern ProcRecord gdisplay_set_colormanaged_proc;
extern ProcRecord gdisplay_all_set_colormanaged_proc;
extern ProcRecord gdisplay_image_set_colormanaged_proc;

#endif  /*  __GDISPLAY_CMDS_H__  */
