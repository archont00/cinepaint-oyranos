#ifndef __COMMANDS_H__
#define __COMMANDS_H__


#include "gtk/gtk.h"

void file_new_cmd_callback (GtkWidget           *widget,
			    gpointer             callback_data,
			    guint                callback_action);
void file_reload_cmd_callback (GtkWidget *, gpointer);
void file_open_cmd_callback (GtkWidget *, gpointer);
void file_save_cmd_callback (GtkWidget *, gpointer);
void file_save_as_cmd_callback (GtkWidget *, gpointer);
void file_save_copy_as_cmd_callback (GtkWidget *, gpointer);
void file_pref_cmd_callback (GtkWidget *, gpointer);
void file_close_cmd_callback (GtkWidget *, gpointer);
void file_quit_cmd_callback (GtkWidget *, gpointer);
void edit_cut_cmd_callback (GtkWidget *, gpointer);
void edit_copy_cmd_callback (GtkWidget *, gpointer);
void edit_paste_cmd_callback (GtkWidget *, gpointer);
void edit_paste_into_cmd_callback (GtkWidget *, gpointer);
void edit_clear_cmd_callback (GtkWidget *, gpointer);
void edit_fill_cmd_callback (GtkWidget *, gpointer);
void edit_stroke_cmd_callback (GtkWidget *, gpointer);
void edit_undo_cmd_callback (GtkWidget *, gpointer);
void edit_redo_cmd_callback (GtkWidget *, gpointer);
void edit_named_cut_cmd_callback (GtkWidget *, gpointer);
void edit_named_copy_cmd_callback (GtkWidget *, gpointer);
void edit_named_paste_cmd_callback (GtkWidget *, gpointer);
void select_toggle_cmd_callback (GtkWidget *, gpointer);
void select_invert_cmd_callback (GtkWidget *, gpointer);
void select_all_cmd_callback (GtkWidget *, gpointer);
void select_none_cmd_callback (GtkWidget *, gpointer);
void select_float_cmd_callback (GtkWidget *, gpointer);
void select_sharpen_cmd_callback (GtkWidget *, gpointer);
void select_border_cmd_callback (GtkWidget *, gpointer);
void select_feather_cmd_callback (GtkWidget *, gpointer);
void select_grow_cmd_callback (GtkWidget *, gpointer);
void select_shrink_cmd_callback (GtkWidget *, gpointer);
void select_by_color_cmd_callback (GtkWidget *, gpointer);
void select_save_cmd_callback (GtkWidget *, gpointer);
void view_colormanage_display_cmd_callback(GtkWidget *, gpointer);
void view_bpc_display_cmd_callback(GtkWidget *, gpointer);
void view_paper_simulation_display_cmd_callback(GtkWidget *, gpointer);
void view_proof_display_cmd_callback(GtkWidget *, gpointer);
void view_gamutcheck_display_cmd_callback(GtkWidget *, gpointer);
void view_rendering_intent_cmd_callback(GtkWidget *, gpointer, guint);
void view_look_profiles_cmd_callback(GtkWidget *, gpointer);
void view_expose_hdr_cmd_callback (GtkWidget *, gpointer);
void view_zoomin_cmd_callback (GtkWidget *, gpointer);
void view_zoomout_cmd_callback (GtkWidget *, gpointer);
void view_zoom_16_1_callback (GtkWidget *, gpointer);
void view_zoom_8_1_callback (GtkWidget *, gpointer);
void view_zoom_4_1_callback (GtkWidget *, gpointer);
void view_zoom_2_1_callback (GtkWidget *, gpointer);
void view_zoom_1_1_callback (GtkWidget *, gpointer);
void view_zoom_1_2_callback (GtkWidget *, gpointer);
void view_zoom_1_4_callback (GtkWidget *, gpointer);
void view_zoom_1_8_callback (GtkWidget *, gpointer);
void view_zoom_1_16_callback (GtkWidget *, gpointer);
void view_pan_zoom_window_cb(GtkWidget *, gpointer);
void view_zoom_bookmark0_cb(GtkWidget *, gpointer);
void view_zoom_bookmark1_cb(GtkWidget *, gpointer);
void view_zoom_bookmark2_cb(GtkWidget *, gpointer);
void view_zoom_bookmark3_cb(GtkWidget *, gpointer);
void view_zoom_bookmark4_cb(GtkWidget *, gpointer);
void view_zoom_bookmark_save_cb(GtkWidget *, gpointer);
void view_zoom_bookmark_load_cb(GtkWidget *, gpointer);
void view_window_info_cmd_callback (GtkWidget *, gpointer);
void view_toggle_rulers_cmd_callback (GtkWidget *, gpointer);
void view_toggle_guides_cmd_callback (GtkWidget *, gpointer);
void view_snap_to_guides_cmd_callback (GtkWidget *, gpointer);
void view_new_view_cmd_callback (GtkWidget *, gpointer);
void view_shrink_wrap_cmd_callback (GtkWidget *, gpointer);
void image_equalize_cmd_callback (GtkWidget *, gpointer);
void image_invert_cmd_callback (GtkWidget *, gpointer);
void image_posterize_cmd_callback (GtkWidget *, gpointer);
void image_threshold_cmd_callback (GtkWidget *, gpointer);
void image_color_balance_cmd_callback (GtkWidget *, gpointer);
void image_color_correction_cmd_callback (GtkWidget *, gpointer);
void image_brightness_contrast_cmd_callback (GtkWidget *, gpointer);
void image_gamma_expose_cmd_callback (GtkWidget *, gpointer);
void image_hue_saturation_cmd_callback (GtkWidget *, gpointer);
void image_curves_cmd_callback (GtkWidget *, gpointer);
void image_levels_cmd_callback (GtkWidget *, gpointer);
void image_desaturate_cmd_callback (GtkWidget *, gpointer);
void image_convert_rgb_cmd_callback (GtkWidget *, gpointer);
void image_convert_grayscale_cmd_callback (GtkWidget *, gpointer);
void image_convert_indexed_cmd_callback (GtkWidget *, gpointer);
void image_convert_depth_cmd_callback (GtkWidget *, gpointer, guint);
void image_convert_colorspace_cmd_callback (GtkWidget *, gpointer, guint);
void image_assign_cms_profile_cmd_callback (GtkWidget *, gpointer, guint);
void image_resize_cmd_callback (GtkWidget *, gpointer);
void image_scale_cmd_callback (GtkWidget *, gpointer);
void image_histogram_cmd_callback (GtkWidget *, gpointer);
void channel_ops_duplicate_cmd_callback (GtkWidget *, gpointer);
void channel_ops_offset_cmd_callback (GtkWidget *, gpointer);
void layers_raise_cmd_callback (GtkWidget *, gpointer);
void layers_lower_cmd_callback (GtkWidget *, gpointer);
void layers_anchor_cmd_callback (GtkWidget *, gpointer);
void layers_merge_cmd_callback (GtkWidget *, gpointer);
void layers_flatten_cmd_callback (GtkWidget *, gpointer);
void layers_alpha_select_cmd_callback (GtkWidget *, gpointer);
void layers_mask_select_cmd_callback (GtkWidget *, gpointer);
void layers_add_alpha_channel_cmd_callback (GtkWidget *, gpointer);
void tools_default_colors_cmd_callback (GtkWidget *, gpointer);
void tools_swap_colors_cmd_callback (GtkWidget *, gpointer);
void tools_select_cmd_callback (GtkWidget           *widget,
				gpointer             callback_data,
				guint                callback_action);
void filters_repeat_cmd_callback (GtkWidget           *widget,
				  gpointer             callback_data,
				  guint                callback_action);
#if GIMP1_2
void dialogs_tool_options_cmd_callback    (GtkWidget *, gpointer);
void dialogs_gradients_cmd_callback       (GtkWidget *, gpointer);
void dialogs_error_console_cmd_callback   (GtkWidget *, gpointer);
void dialogs_document_index_cmd_callback  (GtkWidget *, gpointer);
#endif

void dialogs_brushes_cmd_callback (GtkWidget *, gpointer);
void dialogs_patterns_cmd_callback (GtkWidget *, gpointer);
void dialogs_palette_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_adv_forward_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_adv_backwards_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_play_forward_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_play_backwards_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_flip_forward_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_flip_backwards_cmd_callback (GtkWidget *, gpointer);
void dialogs_store_frame_manager_stop_cmd_callback (GtkWidget *, gpointer);

void dialogs_gradient_editor_cmd_callback (GtkWidget *, gpointer);
void dialogs_lc_cmd_callback (GtkWidget *, gpointer);
void dialogs_indexed_palette_cmd_callback (GtkWidget *, gpointer);
void dialogs_tools_options_cmd_callback (GtkWidget *, gpointer);
void dialogs_input_devices_cmd_callback (GtkWidget *, gpointer);
void dialogs_device_status_cmd_callback (GtkWidget *, gpointer);
void about_dialog_cmd_callback (GtkWidget *, gpointer);
void bugs_dialog_cmd_callback (GtkWidget *, gpointer);
void tips_dialog_cmd_callback (GtkWidget *, gpointer);
void dialogs_input_devices_cmd_callback   (GtkWidget *, gpointer);
void brush_increase_radius ();
void brush_decrease_radius (); 

#endif /* __COMMANDS_H__ */
