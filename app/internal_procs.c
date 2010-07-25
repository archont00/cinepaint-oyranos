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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "app_procs.h"
#include "airbrush.h"
#include "blend.h"
#include "bucket_fill.h"
#include "brightness_contrast.h"
#include "brush.h"
#include "brushlist.h"
#include "by_color_select.h"
#include "channel_cmds.h"
#include "channel_ops.h"
#include "clone.h"
#include "color_balance.h"
#include "color_picker.h"
#include "convert.h"
#include "convolve.h"
#include "crop.h"
#include "curves.h"
#include "desaturate.h"
#include "drawable_cmds.h"
#include "edit_cmds.h"
#include "ellipse_select.h"
#include "equalize.h"
#include "eraser.h"
#include "flip_tool.h"
#include "floating_sel_cmds.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "gdisplay_cmds.h"
#include "gimage_cmds.h"
#include "gimage_mask_cmds.h"
#include "rc.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "invert.h"
#include "layer_cmds.h"
#include "levels.h"
#include "internal_procs.h"
#include "paintbrush.h"
#include "palette.h"
#include "patterns.h"
#include "pencil.h"
#include "perspective_tool.h"
#include "posterize.h"
#include "rect_select.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#ifdef BUILD_SHM
#include "shmbuf.h"
#endif
#include "base_frame_manager.h"

#include "threshold.h"
#include "undo_cmds.h"
#include "procedural_db.h"


void
internal_procs_init ()
{
  gfloat pcount = 0;
  /* grep -c procedural_db_register internal_procs.c */
  gfloat total_pcount = 209;
  app_init_update_status(_("Internal Procedures"), _("Tool procedures"),
			 pcount/total_pcount);

  /*  Tool procedures  */
  procedural_db_register (&airbrush_proc); pcount++;
  procedural_db_register (&blend_proc); pcount++;
  procedural_db_register (&bucket_fill_proc); pcount++;
  procedural_db_register (&by_color_select_proc); pcount++;
  procedural_db_register (&clone_proc); pcount++;
  procedural_db_register (&color_picker_proc); pcount++;
  procedural_db_register (&convolve_proc); pcount++;
  procedural_db_register (&crop_proc); pcount++;
  procedural_db_register (&ellipse_select_proc); pcount++;
  procedural_db_register (&eraser_proc); pcount++;
  procedural_db_register (&eraser_extended_proc); pcount++;
  procedural_db_register (&flip_proc); pcount++;
  procedural_db_register (&free_select_proc); pcount++;
  procedural_db_register (&fuzzy_select_proc); pcount++;
  procedural_db_register (&paintbrush_proc); pcount++;
  procedural_db_register (&paintbrush_extended_proc); pcount++;
  procedural_db_register (&pencil_proc); pcount++;
  procedural_db_register (&perspective_proc); pcount++;
  procedural_db_register (&rect_select_proc); pcount++;
  procedural_db_register (&rotate_proc); pcount++;
  procedural_db_register (&scale_proc); pcount++;
  procedural_db_register (&shear_proc); pcount++;

  app_init_update_status(NULL, _("GDisplay procedures"),
			 pcount/total_pcount);

  /*  GDisplay procedures  */
  procedural_db_register (&gdisplay_new_proc); pcount++;
  procedural_db_register (&gdisplay_fm_proc); pcount++;
  procedural_db_register (&bfm_set_dir_src_proc); pcount++;
  procedural_db_register (&bfm_set_dir_dest_proc); pcount++;
  procedural_db_register (&gdisplay_active_proc); pcount++;
  procedural_db_register (&gdisplay_delete_proc); pcount++;
  procedural_db_register (&gdisplays_delete_image_proc); pcount++;
  procedural_db_register (&gdisplays_flush_proc); pcount++;
  procedural_db_register (&gdisplay_get_image_id_proc); pcount++;

  app_init_update_status(NULL, _("Edit procedures"),
			 pcount/total_pcount);

  /*  Edit procedures  */
  procedural_db_register (&edit_cut_proc); pcount++;
  procedural_db_register (&edit_copy_proc); pcount++;
  procedural_db_register (&edit_paste_proc); pcount++;
  procedural_db_register (&edit_clear_proc); pcount++;
  procedural_db_register (&edit_fill_proc); pcount++;
  procedural_db_register (&edit_stroke_proc); pcount++;

  app_init_update_status(NULL, _("GImage procedures"),
			 pcount/total_pcount);

  /*  GImage procedures  */
  procedural_db_register (&gimage_list_images_proc); pcount++;
  procedural_db_register (&gimage_new_proc); pcount++;
  procedural_db_register (&gimage_resize_proc); pcount++;
  procedural_db_register (&gimage_scale_proc); pcount++;
  procedural_db_register (&gimage_delete_proc); pcount++;
  procedural_db_register (&gimage_free_shadow_proc); pcount++;
  procedural_db_register (&gimage_is_layered_proc); pcount++;
  procedural_db_register (&gimage_dirty_flag_proc); pcount++;
  procedural_db_register (&gimage_get_layers_proc); pcount++;
  procedural_db_register (&gimage_get_channels_proc); pcount++;
  procedural_db_register (&gimage_get_active_layer_proc); pcount++;
  procedural_db_register (&gimage_get_active_channel_proc); pcount++;
  procedural_db_register (&gimage_get_selection_proc); pcount++;
  procedural_db_register (&gimage_get_component_active_proc); pcount++;
  procedural_db_register (&gimage_get_component_visible_proc); pcount++;
  procedural_db_register (&gimage_set_active_layer_proc); pcount++;
  procedural_db_register (&gimage_set_active_channel_proc); pcount++;
  procedural_db_register (&gimage_unset_active_channel_proc); pcount++;
  procedural_db_register (&gimage_set_component_active_proc); pcount++;
  procedural_db_register (&gimage_set_component_visible_proc); pcount++;
  procedural_db_register (&gimage_pick_correlate_layer_proc); pcount++;
  procedural_db_register (&gimage_raise_layer_proc); pcount++;
  procedural_db_register (&gimage_lower_layer_proc); pcount++;
  procedural_db_register (&gimage_merge_visible_layers_proc); pcount++;
  procedural_db_register (&gimage_flatten_proc); pcount++;
  procedural_db_register (&gimage_add_layer_proc); pcount++;
  procedural_db_register (&gimage_remove_layer_proc); pcount++;
  procedural_db_register (&gimage_add_layer_mask_proc); pcount++;
  procedural_db_register (&gimage_remove_layer_mask_proc); pcount++;
  procedural_db_register (&gimage_raise_channel_proc); pcount++;
  procedural_db_register (&gimage_lower_channel_proc); pcount++;
  procedural_db_register (&gimage_add_channel_proc); pcount++;
  procedural_db_register (&gimage_remove_channel_proc); pcount++;
  procedural_db_register (&gimage_active_drawable_proc); pcount++;
  procedural_db_register (&gimage_base_type_proc); pcount++;
  procedural_db_register (&gimage_get_filename_proc); pcount++;
  procedural_db_register (&gimage_set_filename_proc); pcount++;
  procedural_db_register (&gimage_width_proc); pcount++;
  procedural_db_register (&gimage_height_proc); pcount++;
  procedural_db_register (&gimage_get_cmap_proc); pcount++;
  procedural_db_register (&gimage_set_cmap_proc); pcount++;
  procedural_db_register (&gimage_enable_undo_proc); pcount++;
  procedural_db_register (&gimage_disable_undo_proc); pcount++;
  procedural_db_register (&gimage_clean_all_proc); pcount++;
  procedural_db_register (&gimage_floating_sel_proc); pcount++;
  procedural_db_register (&gimage_thumbnail_proc); pcount++;
  procedural_db_register (&gimage_set_icc_profile_by_name_proc); pcount++;
  procedural_db_register (&gimage_set_icc_profile_by_mem_proc); pcount++;
  procedural_db_register (&gimage_set_lab_profile_proc); pcount++;
  procedural_db_register (&gimage_set_xyz_profile_proc); pcount++;
  procedural_db_register (&gimage_set_srgb_profile_proc); pcount++;
  procedural_db_register (&gimage_has_icc_profile_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_by_mem_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_info_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_description_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_pcs_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_color_space_name_proc); pcount++;
  procedural_db_register (&gimage_get_icc_profile_device_class_name_proc); pcount++;
  procedural_db_register (&gdisplay_get_cms_intent_proc); pcount++;
  procedural_db_register (&gdisplay_set_cms_intent_proc); pcount++;
  procedural_db_register (&gdisplay_get_cms_flags_proc); pcount++;
  procedural_db_register (&gdisplay_set_cms_flags_proc); pcount++;
  procedural_db_register (&gdisplay_is_colormanaged_proc); pcount++;
  procedural_db_register (&gdisplay_set_colormanaged_proc); pcount++;
  procedural_db_register (&gdisplay_all_set_colormanaged_proc); pcount++;
  procedural_db_register (&gdisplay_image_set_colormanaged_proc); pcount++;

  app_init_update_status(NULL, _("GImage mask procedures"),
			 pcount/total_pcount);

  /*  GImage mask procedures  */
  procedural_db_register (&gimage_mask_bounds_proc); pcount++;
  procedural_db_register (&gimage_mask_value_proc); pcount++;
  procedural_db_register (&gimage_mask_is_empty_proc); pcount++;
  procedural_db_register (&gimage_mask_translate_proc); pcount++;
  procedural_db_register (&gimage_mask_float_proc); pcount++;
  procedural_db_register (&gimage_mask_clear_proc); pcount++;
  procedural_db_register (&gimage_mask_invert_proc); pcount++;
  procedural_db_register (&gimage_mask_sharpen_proc); pcount++;
  procedural_db_register (&gimage_mask_all_proc); pcount++;
  procedural_db_register (&gimage_mask_none_proc); pcount++;
  procedural_db_register (&gimage_mask_feather_proc); pcount++;
  procedural_db_register (&gimage_mask_border_proc); pcount++;
  procedural_db_register (&gimage_mask_grow_proc); pcount++;
  procedural_db_register (&gimage_mask_shrink_proc); pcount++;
  procedural_db_register (&gimage_mask_layer_alpha_proc); pcount++;
  procedural_db_register (&gimage_mask_load_proc); pcount++;
  procedural_db_register (&gimage_mask_save_proc); pcount++;

  app_init_update_status(NULL, _("Layer procedures"),
			 pcount/total_pcount);

  /*  Layer procedures  */
  procedural_db_register (&layer_new_proc); pcount++;
  procedural_db_register (&layer_copy_proc); pcount++;
  procedural_db_register (&layer_create_mask_proc); pcount++;
  procedural_db_register (&layer_scale_proc); pcount++;
  procedural_db_register (&layer_resize_proc); pcount++;
  procedural_db_register (&layer_delete_proc); pcount++;
  procedural_db_register (&layer_translate_proc); pcount++;
  procedural_db_register (&layer_add_alpha_proc); pcount++;
  procedural_db_register (&layer_get_name_proc); pcount++;
  procedural_db_register (&layer_set_name_proc); pcount++;
  procedural_db_register (&layer_get_visible_proc); pcount++;
  procedural_db_register (&layer_set_visible_proc); pcount++;
  procedural_db_register (&layer_get_preserve_trans_proc); pcount++;
  procedural_db_register (&layer_set_preserve_trans_proc); pcount++;
  procedural_db_register (&layer_get_apply_mask_proc); pcount++;
  procedural_db_register (&layer_set_apply_mask_proc); pcount++;
  procedural_db_register (&layer_get_show_mask_proc); pcount++;
  procedural_db_register (&layer_set_show_mask_proc); pcount++;
  procedural_db_register (&layer_get_edit_mask_proc); pcount++;
  procedural_db_register (&layer_set_edit_mask_proc); pcount++;
  procedural_db_register (&layer_get_opacity_proc); pcount++;
  procedural_db_register (&layer_set_opacity_proc); pcount++;
  procedural_db_register (&layer_get_mode_proc); pcount++;
  procedural_db_register (&layer_set_mode_proc); pcount++;
  procedural_db_register (&layer_set_offsets_proc); pcount++;
  procedural_db_register (&layer_mask_proc); pcount++;
  procedural_db_register (&layer_is_floating_sel_proc); pcount++;

  app_init_update_status(NULL, _("Channel procedures"),
			 pcount/total_pcount);

  /*  Channel procedures  */
  procedural_db_register (&channel_new_proc); pcount++;
  procedural_db_register (&channel_copy_proc); pcount++;
  procedural_db_register (&channel_delete_proc); pcount++;
  procedural_db_register (&channel_get_name_proc); pcount++;
  procedural_db_register (&channel_set_name_proc); pcount++;
  procedural_db_register (&channel_get_visible_proc); pcount++;
  procedural_db_register (&channel_set_visible_proc); pcount++;
  procedural_db_register (&channel_get_show_masked_proc); pcount++;
  procedural_db_register (&channel_set_show_masked_proc); pcount++;
  procedural_db_register (&channel_get_opacity_proc); pcount++;
  procedural_db_register (&channel_set_opacity_proc); pcount++;
  procedural_db_register (&channel_get_color_proc); pcount++;
  procedural_db_register (&channel_set_color_proc); pcount++;

  app_init_update_status(NULL, _("Drawable procedures"),
			 pcount/total_pcount);

  /*  Drawable procedures  */
  procedural_db_register (&drawable_merge_shadow_proc); pcount++;
  procedural_db_register (&drawable_fill_proc); pcount++;
  procedural_db_register (&drawable_update_proc); pcount++;
  procedural_db_register (&drawable_mask_bounds_proc); pcount++;
  procedural_db_register (&drawable_gimage_proc); pcount++;
  procedural_db_register (&drawable_type_proc); pcount++;
  procedural_db_register (&drawable_has_alpha_proc); pcount++;
  procedural_db_register (&drawable_type_with_alpha_proc); pcount++;
  procedural_db_register (&drawable_color_proc); pcount++;
  procedural_db_register (&drawable_gray_proc); pcount++;
  procedural_db_register (&drawable_indexed_proc); pcount++;
  procedural_db_register (&drawable_bytes_proc); pcount++;
  procedural_db_register (&drawable_precision_proc); pcount++;
  procedural_db_register (&drawable_num_channels_proc); pcount++;
  procedural_db_register (&drawable_width_proc); pcount++;
  procedural_db_register (&drawable_height_proc); pcount++;
  procedural_db_register (&drawable_offsets_proc); pcount++;
  procedural_db_register (&drawable_layer_proc); pcount++;
  procedural_db_register (&drawable_layer_mask_proc); pcount++;
  procedural_db_register (&drawable_channel_proc); pcount++;
  procedural_db_register (&drawable_set_pixel_proc); pcount++;
  procedural_db_register (&drawable_get_pixel_proc); pcount++;

  app_init_update_status(NULL, _("Floating selections"),
			 pcount/total_pcount);

  /*  Floating Selections  */
  procedural_db_register (&floating_sel_remove_proc); pcount++;
  procedural_db_register (&floating_sel_anchor_proc); pcount++;
  procedural_db_register (&floating_sel_to_layer_proc); pcount++;

  app_init_update_status(NULL, _("Undo"),
			 pcount/total_pcount);

  /*  Undo  */
  procedural_db_register (&undo_push_group_start_proc); pcount++;
  procedural_db_register (&undo_push_group_end_proc); pcount++;

  app_init_update_status(NULL, _("Palette"),
			 pcount/total_pcount);

  /*  Palette  */
  procedural_db_register (&palette_get_foreground_proc); pcount++;
  procedural_db_register (&palette_get_background_proc); pcount++;
  procedural_db_register (&palette_set_foreground_proc); pcount++;
  procedural_db_register (&palette_set_background_proc); pcount++;
  procedural_db_register (&palette_set_default_colors_proc); pcount++;
  procedural_db_register (&palette_swap_colors_proc); pcount++; 
  procedural_db_register (&palette_refresh_proc); pcount++;

  app_init_update_status(NULL, _("Interface procedures"),
			 pcount/total_pcount);

  /*  Interface procs  */
  procedural_db_register (&brushes_get_brush_proc); pcount++;
  procedural_db_register (&brushes_refresh_brush_proc); pcount++;
  procedural_db_register (&brushes_set_brush_proc); pcount++;
  procedural_db_register (&brushes_get_opacity_proc); pcount++;
  procedural_db_register (&brushes_set_opacity_proc); pcount++;
  procedural_db_register (&brushes_get_spacing_proc); pcount++;
  procedural_db_register (&brushes_set_spacing_proc); pcount++;
  procedural_db_register (&brushes_get_paint_mode_proc); pcount++;
  procedural_db_register (&brushes_set_paint_mode_proc); pcount++;
  procedural_db_register (&brushes_list_proc); pcount++;
  procedural_db_register (&patterns_get_pattern_proc); pcount++;
  procedural_db_register (&patterns_set_pattern_proc); pcount++;
  procedural_db_register (&patterns_list_proc); pcount++;

  procedural_db_register (&gradients_get_list_proc); pcount++;
  procedural_db_register (&gradients_get_active_proc); pcount++;
  procedural_db_register (&gradients_set_active_proc); pcount++;
  procedural_db_register (&gradients_sample_uniform_proc); pcount++;
  procedural_db_register (&gradients_sample_custom_proc); pcount++;

  app_init_update_status(NULL, _("Image procedures"),
			 pcount/total_pcount);

  /*  Image procedures  */
  procedural_db_register (&desaturate_proc); pcount++;
  procedural_db_register (&equalize_proc); pcount++;
  procedural_db_register (&invert_proc); pcount++;

  procedural_db_register (&brightness_contrast_proc); pcount++;
#if 0
  procedural_db_register (&curves_spline_proc); pcount++;
  procedural_db_register (&curves_explicit_proc); pcount++;
#endif
  procedural_db_register (&color_balance_proc); pcount++;
  procedural_db_register (&histogram_proc); pcount++;
  procedural_db_register (&hue_saturation_proc); pcount++;
  procedural_db_register (&levels_proc); pcount++;
  procedural_db_register (&posterize_proc); pcount++;
  procedural_db_register (&threshold_proc); pcount++;

  procedural_db_register (&convert_rgb_proc); pcount++;
  procedural_db_register (&convert_grayscale_proc); pcount++;
  procedural_db_register (&convert_indexed_proc); pcount++;
  procedural_db_register (&convert_indexed_palette_proc); pcount++;

  app_init_update_status(NULL, _("Channel ops"),
			 pcount/total_pcount);

  /*  Channel Ops procedures  */
  procedural_db_register (&channel_ops_duplicate_proc); pcount++;
  procedural_db_register (&channel_ops_offset_proc); pcount++;

  /*  Gimprc procedures  */
  procedural_db_register (&gimprc_query_proc); pcount++;

  app_init_update_status(NULL, _("Procedural database"),
			 pcount/total_pcount);

  /*  Procedural Database  */
  procedural_db_register (&procedural_db_dump_proc); pcount++;
  procedural_db_register (&procedural_db_query_proc); pcount++;
  procedural_db_register (&procedural_db_proc_info_proc); pcount++;
  procedural_db_register (&procedural_db_proc_arg_proc); pcount++;
  procedural_db_register (&procedural_db_proc_val_proc); pcount++;
  procedural_db_register (&procedural_db_get_data_proc); pcount++;
  procedural_db_register (&procedural_db_set_data_proc); pcount++;

  
  app_init_update_status(NULL, _("Shared Memory Images"),
			 pcount/total_pcount);

#ifdef BUILD_SHM
  procedural_db_register (&shmseg_new_proc); pcount++;
  procedural_db_register (&shmseg_delete_proc); pcount++;
  procedural_db_register (&shmseg_attach_proc); pcount++;
  procedural_db_register (&shmseg_detach_proc); pcount++;
  procedural_db_register (&shmseg_status_proc); pcount++;
#endif
}
