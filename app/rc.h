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
#ifndef __GIMPRC_H__
#define __GIMPRC_H__

#include <glib.h>
#include "tag.h"
#include "procedural_db.h"
#include "../lib/wire/datadir.h"

/*  global gimprc variables  */
extern char *    plug_in_path;
extern char *    temp_path;
extern char *    swap_path;
extern char *    brush_path;
extern char *    brush_vbr_path;
extern char *    default_brush;
extern char *    pattern_path;
extern char *    default_pattern;
extern char *    palette_path;
extern char *    frame_manager_path;
extern char *    default_palette;
extern char *    default_frame_manager;
extern char *    gradient_path;
extern char *    default_gradient;
extern char *    pluginrc_path;
extern char *    cms_profile_path;
extern int       tile_cache_size;
extern int       marching_speed;
extern double    gamma_val;
extern int       transparency_type;
extern int       transparency_size;
extern int       levels_of_undo;
extern int       color_cube_shades[];
extern int       install_cmap;
extern int       cycled_marching_ants;
extern double    default_threshold;
extern int       stingy_memory_use;
extern int       allow_resize_windows;
extern int       no_cursor_updating;
extern int       preview_size;
extern int       show_rulers;
extern int       ruler_units;
extern int       auto_save;
extern int       cubic_interpolation;
extern int       toolbox_x, toolbox_y;
extern int       progress_x, progress_y;
extern int       info_x, info_y;
extern int       color_select_x, color_select_y;
extern int       tool_options_x, tool_options_y;

extern int       zoom_window_x, zoom_window_y;
extern int       frame_manager_x, frame_manager_y;
extern int       brush_select_x, brush_select_y;
extern int       brush_edit_x, brush_edit_y;
extern int       layer_channel_x, layer_channel_y;
extern int       palette_x, palette_y;
extern int       gradient_x, gradient_y;
extern int       image_x, image_y;
extern int       generic_window_x, generic_window_y;

extern int       confirm_on_close;
extern int       default_width, default_height;
extern Format    default_format;
extern Precision default_precision;
extern int       show_tips;
extern int       last_tip;
extern int       show_tool_tips;
extern int       gamut_alarm_red;
extern int       gamut_alarm_green;
extern int       gamut_alarm_blue;

extern float     monitor_xres;
extern float     monitor_yres;
extern int       using_xserver_resolution;
extern int       enable_rgbm_painting;
extern int       enable_brush_dialog;
extern int       enable_layer_dialog;
extern int       enable_paste_c_disp;
extern int       enable_tmp_saving;
extern int       enable_channel_revert;

extern int       tool_options_visible;
extern int       zoom_window_visible;
extern int       brush_select_visible;
extern int       brush_edit_visible;
extern int       layer_channel_visible;
extern int       color_visible;
extern int       palette_visible;
extern int       gradient_visible;
extern char *    cms_default_image_profile_name;
extern char *    cms_default_proof_profile_name;
extern char *    cms_display_profile_name;
extern char *    cms_workspace_profile_name;
extern int       cms_default_intent;
extern int       cms_bpc_by_default;
extern int       cms_default_proof_intent;
extern int       cms_default_flags;
extern int       cms_open_action;
extern int       cms_mismatch_action;
extern int       cms_manage_by_default;
extern int       cms_oyranos;
extern char *    look_profile_path;

extern int enable_rulers_on_start;

/*  function prototypes  */
char *  gimp_directory (void);
void    parse_gimprc (void);
void    parse_gimprc_file (char *filename);
void    save_gimprc (GList **updated_options, GList **conflicting_options);

/*  procedural database procs  */
extern ProcRecord gimprc_query_proc;

#endif  /*  __GIMPRC_H__  */
