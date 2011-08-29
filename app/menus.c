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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "colormaps.h"
#include "commands.h"
#include "fileops.h"
#include "rc.h"
#include "interface.h"
#include "menus.h"
#include "procedural_db.h"
#include "scale.h"
#include "tools.h"
#include "gdisplay.h"
#include "brushgenerated.h"
#include "clone.h"
#include "spline.h"
#include "store_frame_manager.h"



/* rsr: ld error hack, this is a bug: */
/*#define sfm_slide_show 0*/

#if 1
#define TEAR_OFF
#endif

#if 1 
#define R_AND_H 1
#endif

static GList * g_plugin_menu_items = 0;
typedef struct {
   gchar path[1024];
   gchar accelerator[256];
   GtkMenuCallback callback;
   gpointer callback_data;
} PluginMenuEntry;


static void  menus_init (void);
static void
menus_strip_extra_entries(const char *tmp_filename, const char *filename);


static GtkItemFactoryEntry toolbox_entries[] =
{
#ifdef TEAR_OFF
  { "/File/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/New...", "<alt>N", file_new_cmd_callback, 0 },
  { "/File/New From", NULL, NULL, 0, "<Branch>" },
#ifdef TEAR_OFF
  { "/File/New From/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/Open...", "<alt>O", file_open_cmd_callback, 0 },
  { "/File/Acquire", NULL, NULL, 0, "<Branch>" },
#ifdef TEAR_OFF
  { "/File/Acquire/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  { "/File/Preferences...", NULL, file_pref_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/File/Dialogs/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/Dialogs/Brushes...", "<control>B", dialogs_brushes_cmd_callback, 0 },
#if 0
  { "/File/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
#endif
  { "/File/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, 0 },
  { "/File/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { "/File/Dialogs/Tool Options...", "<control>T", dialogs_tools_options_cmd_callback, 0 },
  { "/File/Dialogs/Device Status...", NULL, dialogs_device_status_cmd_callback, 0 },
  { "/File/Device Dialog", NULL, dialogs_input_devices_cmd_callback, 0}, 
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  { "/File/Quit", "<alt>Q", file_quit_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/Help/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Help/About...", NULL, about_dialog_cmd_callback, 0 },
  { "/Help/Bugs & Kudos...", NULL, bugs_dialog_cmd_callback, 0 },
  { "/Help/Tips...", NULL, tips_dialog_cmd_callback, 0 },
};

static guint n_toolbox_entries = sizeof (toolbox_entries) / sizeof (toolbox_entries[0]);
static GtkItemFactory *toolbox_factory = NULL;

static GtkItemFactoryEntry image_entries[] =
{
#ifdef TEAR_OFF
  { "/File/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/New...", "<alt>N", file_new_cmd_callback, 1 },
  { "/File/Revert to Saved", "<alt>R", file_reload_cmd_callback, 0 },
  { "/File/Open...", "<alt>O", file_open_cmd_callback, 0 },
#ifndef RnH_STYLE
#ifdef TEAR_OFF
  { "/File/Flipbook/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/File/Flipbook/Create", "<control>F", dialogs_store_frame_manager_cmd_callback, 0 },
  { "/File/Flipbook/Flip Forward", "E", dialogs_store_frame_manager_flip_forward_cmd_callback, 0 },
  { "/File/Flipbook/Flip Backward", "W", dialogs_store_frame_manager_flip_backwards_cmd_callback, 0 },
  { "/File/Flipbook/Frame Forward", "D", dialogs_store_frame_manager_adv_forward_cmd_callback, 0 },
  { "/File/Flipbook/Frame Backward", "A", dialogs_store_frame_manager_adv_backwards_cmd_callback, 0 },
  { "/File/Flipbook/Play Forward", "Y", dialogs_store_frame_manager_play_forward_cmd_callback, 0 },
  { "/File/Flipbook/Play Backward", NULL, dialogs_store_frame_manager_play_backwards_cmd_callback, 0 },
  { "/File/Flipbook/Stop", "X", dialogs_store_frame_manager_stop_cmd_callback, 0 },
#endif
  { "/File/Save", "<alt>S", file_save_cmd_callback, 0 },
  { "/File/Save as...", "<alt><shift>s", file_save_as_cmd_callback, 0 },
  { "/File/SaveFile copy as...", NULL, file_save_copy_as_cmd_callback, 0 }, 
#if 0
  { "/File/OpenPTS", "<alt>O", NULL, 0 },
  { "/File/SavePTS", "<alt>S", NULL, 0 },
  { "/File/SavePTS as", NULL, NULL, 0 },
#endif 
  { "/File/Preferences...", NULL, file_pref_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
  { "/File/Close", "<alt>W", file_close_cmd_callback, 0 },
  { "/File/Quit", "<alt>Q", file_quit_cmd_callback, 0 },
  { "/File/---", NULL, NULL, 0, "<Separator>" },
#ifdef TEAR_OFF
  { "/Edit/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Edit/Undo", "<alt>Z", edit_undo_cmd_callback, 0 },
  { "/Edit/Redo", "<alt><shift>z", edit_redo_cmd_callback, 0 },
  { "/Edit/---", NULL, NULL, 0, "<Separator>" },
  { "/Edit/Cut", "<alt>X", edit_cut_cmd_callback, 0 },
  { "/Edit/Copy", "<alt>C", edit_copy_cmd_callback, 0 },
  { "/Edit/Paste", "<alt>V", edit_paste_cmd_callback, 0 },
  { "/Edit/Paste Into", NULL, edit_paste_into_cmd_callback, 0 },
  { "/Edit/Clear", "<control>K", edit_clear_cmd_callback, 0 },
  { "/Edit/Fill", "<control>.", edit_fill_cmd_callback, 0 },
  { "/Edit/Stroke", NULL, edit_stroke_cmd_callback, 0 },
  { "/Edit/---", NULL, NULL, 0, "<Separator>" },
  { "/Edit/Cut Named", "<control><alt>X", edit_named_cut_cmd_callback, 0 },
  { "/Edit/Copy Named", "<control><alt>C", edit_named_copy_cmd_callback, 0 },
  { "/Edit/Paste Named", "<control><alt>V", edit_named_paste_cmd_callback, 0 },
  { "/Edit/---", NULL, NULL, 0, "<Separator>" },
#ifdef TEAR_OFF
  { "/Select/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Select/Toggle", "<control>T", select_toggle_cmd_callback, 0 },
  { "/Select/Invert", "<control>I", select_invert_cmd_callback, 0 },
  { "/Select/All", "<control>A", select_all_cmd_callback, 0 },
  { "/Select/None", "<control><shift>A", select_none_cmd_callback, 0 },
  { "/Select/Float", "<control><shift>L", select_float_cmd_callback, 0 },
#if 0
  { "/Select/Sharpen", "<control><shift>H", select_sharpen_cmd_callback, 0 },
  { "/Select/Border", "<control><shift>B", select_border_cmd_callback, 0 },
#endif
  { "/Select/Feather", "<control><shift>F", select_feather_cmd_callback, 0 },
  { "/Select/Grow", NULL, select_grow_cmd_callback, 0 },
  { "/Select/Shrink", NULL, select_shrink_cmd_callback, 0 },
  { "/Select/Save To Channel", NULL, select_save_cmd_callback, 0 },
  { "/Select/By Color...", NULL, select_by_color_cmd_callback, 0 },
  { "/View/", "equal", view_zoomin_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/View/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/View/Colormanage Display", NULL, view_colormanage_display_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Proof Display", NULL, view_proof_display_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Simulatate Paper", NULL, view_paper_simulation_display_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Gamut Check Display", NULL, view_gamutcheck_display_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Rendering Intent/Perceptual", NULL, view_rendering_intent_cmd_callback, INTENT_PERCEPTUAL, "<RadioItem>"},
  { "/View/Rendering Intent/Relative Colorimetric", NULL, view_rendering_intent_cmd_callback, INTENT_RELATIVE_COLORIMETRIC, "/View/Rendering Intent/Perceptual" },
  { "/View/Rendering Intent/Saturation", NULL, view_rendering_intent_cmd_callback, INTENT_SATURATION, "/View/Rendering Intent/Relative Colorimetric"},
  { "/View/Rendering Intent/Absolute Colorimetric", NULL, view_rendering_intent_cmd_callback, INTENT_ABSOLUTE_COLORIMETRIC, "/View/Rendering Intent/Saturation"},
  { "/View/Rendering Intent/Black Point Compensation", NULL, view_bpc_display_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Look Profiles...", NULL, view_look_profiles_cmd_callback, 0 },
  { "/View/Expose", NULL, view_expose_hdr_cmd_callback, 0 },
  { "/View/---", NULL, NULL, 0, "<Separator>" },

  { "/View/Pan Zoom Window", NULL, view_pan_zoom_window_cb, 0 },
#ifdef TEAR_OFF
  { "/View/Zoom Bookmarks/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/View/Zoom Bookmarks/1", "<control>1", view_zoom_bookmark0_cb, 0},
  { "/View/Zoom Bookmarks/2", "<control>2", view_zoom_bookmark1_cb, 0},
  { "/View/Zoom Bookmarks/3", "<control>3", view_zoom_bookmark2_cb, 0},
  { "/View/Zoom Bookmarks/4", "<control>4", view_zoom_bookmark3_cb, 0},
  { "/View/Zoom Bookmarks/5", "<control>5", view_zoom_bookmark4_cb, 0},
  { "/View/Zoom Bookmarks/Load", NULL, view_zoom_bookmark_load_cb, 0},
  { "/View/Zoom Bookmarks/Save", NULL, view_zoom_bookmark_save_cb, 0},
  { "/View/Zoom In", "plus", view_zoomin_cmd_callback, 0 },
  { "/View/Zoom Out", "minus", view_zoomout_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/View/Zoom/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/View/Zoom/16:1", NULL, view_zoom_16_1_callback, 0 },
  { "/View/Zoom/8:1", NULL, view_zoom_8_1_callback, 0 },
  { "/View/Zoom/4:1", NULL, view_zoom_4_1_callback, 0 },
  { "/View/Zoom/2:1", NULL, view_zoom_2_1_callback, 0 },
  { "/View/Zoom/1:1", "1", view_zoom_1_1_callback, 0 },
  { "/View/Zoom/1:2", NULL, view_zoom_1_2_callback, 0 },
  { "/View/Zoom/1:4", NULL, view_zoom_1_4_callback, 0 },
  { "/View/Zoom/1:8", NULL, view_zoom_1_8_callback, 0 },
  { "/View/Zoom/1:16", NULL, view_zoom_1_16_callback, 0 },
  { "/View/Window Info...", "<control><shift>I", view_window_info_cmd_callback, 0 },
  { "/View/Show Rulers", "<control><shift>R", view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Show Guides", "<control><shift>T", view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
  { "/View/Snap To Guides", NULL, view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
  { "/View/---", NULL, NULL, 0, "<Separator>" },
  { "/View/Slide Show", NULL, sfm_slide_show_callback, 0, NULL },
  { "/View/---", NULL, NULL, 0, "<Separator>" },
  { "/View/New View", NULL, view_new_view_cmd_callback, 0 }, 
  { "/View/Shrink Wrap", "<control>E", view_shrink_wrap_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/Image/tearoff1", NULL, NULL, 0, "<Tearoff>" },
  { "/Image/Colors/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Image/Colors/Equalize", NULL, image_equalize_cmd_callback, 0 },
  { "/Image/Colors/Invert", NULL, image_invert_cmd_callback, 0 },
  { "/Image/Colors/Posterize", NULL, image_posterize_cmd_callback, 0 },
  { "/Image/Colors/Threshold", NULL, image_threshold_cmd_callback, 0 },
  { "/Image/Colors/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Colors/Color Balance", NULL, image_color_balance_cmd_callback, 0 },
  { "/Image/Colors/Color Correction", "<control>C", image_color_correction_cmd_callback, 0 },
  { "/Image/Colors/Brightness-Contrast", NULL, image_brightness_contrast_cmd_callback, 0 },
  { "/Image/Colors/Hue-Saturation", NULL, image_hue_saturation_cmd_callback, 0 },
  { "/Image/Colors/Gamma-Expose", NULL, image_gamma_expose_cmd_callback, 0 },
#if 1 
  { "/Image/Colors/Curves", NULL, image_curves_cmd_callback, 0 },
#endif
  { "/Image/Colors/Levels", NULL, image_levels_cmd_callback, 0 },
#if 1
  { "/Image/Colors/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Colors/Desaturate", NULL, image_desaturate_cmd_callback, 0 },
#endif
  { "/Image/Channel Ops/Duplicate", "<control>D", channel_ops_duplicate_cmd_callback, 0 },
  { "/Image/Channel Ops/Offset", "<control><shift>O", channel_ops_offset_cmd_callback, 0 },
#if 1
  { "/Image/Alpha/Add Alpha Channel", NULL, layers_add_alpha_channel_cmd_callback, 0 },
#endif
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/RGB", NULL, image_convert_rgb_cmd_callback, 0 },
  { "/Image/Grayscale", NULL, image_convert_grayscale_cmd_callback, 0 },
#if 0
  { "/Image/Indexed", NULL, image_convert_indexed_cmd_callback, 0 },
#endif
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/" PRECISION_U8_STRING, NULL, image_convert_depth_cmd_callback, PRECISION_U8 },
  { "/Image/" PRECISION_U16_STRING, NULL, image_convert_depth_cmd_callback, PRECISION_U16 },
  { "/Image/" PRECISION_FLOAT16_STRING, NULL, image_convert_depth_cmd_callback, PRECISION_FLOAT16 },
  { "/Image/" PRECISION_BFP_STRING, NULL, image_convert_depth_cmd_callback, PRECISION_BFP },  
  { "/Image/" PRECISION_FLOAT_STRING, NULL, image_convert_depth_cmd_callback, PRECISION_FLOAT },
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Convert using ICC Profile...", NULL, image_convert_colorspace_cmd_callback, 0 },
  { "/Image/Assign ICC Profile...", NULL, image_assign_cms_profile_cmd_callback, ICC_IMAGE_PROFILE },
  { "/Image/Assign ICC Proof Profile...", NULL, image_assign_cms_profile_cmd_callback, ICC_PROOF_PROFILE },
  { "/Image/Resize", NULL, image_resize_cmd_callback, 0 },
  { "/Image/Scale", NULL, image_scale_cmd_callback, 0 },
  { "/Image/---", NULL, NULL, 0, "<Separator>" },
  { "/Image/Histogram", NULL, image_histogram_cmd_callback, 0 },
#ifdef TEAR_OFF
  { "/Layers/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Layers/Stroke Splines", NULL, spline_stroke_cmd_callback, 0 },
  { "/Layers/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
  { "/Layers/Raise Layer", "<shift>F", layers_raise_cmd_callback, 0 },
  { "/Layers/Lower Layer", "<shift>B", layers_lower_cmd_callback, 0 },
  { "/Layers/Anchor Layer", "<shift>H", layers_anchor_cmd_callback, 0 },
  { "/Layers/Merge Visible Layers", "<shift>M", layers_merge_cmd_callback, 0 },
  { "/Layers/Flatten Image", NULL, layers_flatten_cmd_callback, 0 },
#if 1
  { "/Layers/Alpha To Selection", NULL, layers_alpha_select_cmd_callback, 0 },
#endif
  { "/Layers/Mask To Selection", NULL, layers_mask_select_cmd_callback, 0 },
#if 1
  { "/Layers/Add Alpha Channel", NULL, layers_add_alpha_channel_cmd_callback, 0 },
#endif
  { "/Layers/Make Layer", NULL, layers_add_alpha_channel_cmd_callback, 0 },
#if 1
#ifdef TEAR_OFF
  { "/Tools/tearoff1", NULL, NULL, 0, "<Tearoff>" },
  { "/Tools/Select Tools/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Tools/Select Tools/Rect Select", NULL, tools_select_cmd_callback, RECT_SELECT },
  { "/Tools/Select Tools/Ellipse Select", NULL, tools_select_cmd_callback, ELLIPSE_SELECT },
  { "/Tools/Select Tools/Free Select", NULL, tools_select_cmd_callback, FREE_SELECT },
  { "/Tools/Select Tools/Fuzzy Select", NULL, tools_select_cmd_callback, FUZZY_SELECT },
  { "/Tools/Select Tools/Bezier Select", NULL, tools_select_cmd_callback, BEZIER_SELECT },
#if 1
  { "/Tools/Select Tools/Intelligent Scissors", "I", tools_select_cmd_callback, ISCISSORS },
#endif
#ifdef TEAR_OFF
  { "/Tools/Transform Tools/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Tools/Transform Tools/Move", NULL, tools_select_cmd_callback, MOVE },
  { "/Tools/Transform Tools/Magnify", NULL, tools_select_cmd_callback, MAGNIFY },
  { "/Tools/Transform Tools/Crop", NULL, tools_select_cmd_callback, CROP },
  { "/Tools/Transform Tools/Transform", NULL, tools_select_cmd_callback, ROTATE },
  { "/Tools/Transform Tools/Flip", NULL, tools_select_cmd_callback, FLIP_HORZ },
  { "/Tools/Text", "T", tools_select_cmd_callback, TEXT },
  { "/Tools/Color Picker", NULL, tools_select_cmd_callback, COLOR_PICKER },
  { "/Tools/Splines", "", tools_select_cmd_callback, SPLINE },
#if 0
  { "/Tools/Color Picker Down", "I", select_color_picker, NULL },
#endif
#ifdef TEAR_OFF
  { "/Tools/Paint Tools/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Tools/Paint Tools/Bucket Fill", NULL, tools_select_cmd_callback, BUCKET_FILL },
  { "/Tools/Paint Tools/Blend", NULL, tools_select_cmd_callback, BLEND },
  { "/Tools/Paint Tools/Paintbrush", NULL, tools_select_cmd_callback, PAINTBRUSH },
  { "/Tools/Paint Tools/Pencil", NULL, tools_select_cmd_callback, PENCIL },
  { "/Tools/Paint Tools/Eraser", NULL, tools_select_cmd_callback, ERASER },
  { "/Tools/Paint Tools/Airbrush", NULL, tools_select_cmd_callback, AIRBRUSH },
  { "/Tools/Paint Tools/Clone", "C", tools_select_cmd_callback, CLONE },
  { "/Tools/Paint Tools/Convolve", NULL, tools_select_cmd_callback, CONVOLVE },
  { "/Tools/Paint Tools/Dodge or Burn", NULL, tools_select_cmd_callback, DODGEBURN },
  { "/Tools/Paint Tools/Smudge", NULL, tools_select_cmd_callback, SMUDGE },
  { "/Tools/Default Colors", NULL, tools_default_colors_cmd_callback, 0 },
  { "/Tools/Swap Colors", NULL, tools_swap_colors_cmd_callback, 0 },  
  { "/Tools/---", NULL, NULL, 0, "<Separator>" },
  { "/Tools/Toolbox", NULL, toolbox_raise_callback, 0 },
#endif
#ifdef TEAR_OFF
  { "/Short cuts/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Short cuts/Increase Brush Radius (+1)", ".", brush_increase_radius, 0 },
  { "/Short cuts/Decrease Brush Radius (-1)", ",", brush_decrease_radius, 0 },
  { "/Short cuts/Increase clone x offset ", NULL, clone_x_offset_increase, 0 },
  { "/Short cuts/Decrease clone x offset ", NULL, clone_x_offset_decrease, 0 },
  { "/Short cuts/Increase clone y offset ", NULL, clone_y_offset_increase, 0 },
  { "/Short cuts/Decrease clone y offset ", NULL, clone_y_offset_decrease, 0 },
  { "/Short cuts/Toogle Onionskin ", "O", sfm_onionskin_toogle_callback, 0, "<ToggleItem>" },
  { "/Short cuts/Reset clone offset ", ",", clone_reset_offset, 0 },
#ifdef TEAR_OFF
  { "/Filters/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Filters/Repeat last", "<alt>F", filters_repeat_cmd_callback, 0x0 },
  { "/Filters/Re-show last", "<alt><shift>F", filters_repeat_cmd_callback, 0x1 },
  { "/Filters/---", NULL, NULL, 0, "<Separator>" },
#if 0
  { "/Parsley/", NULL, NULL, 0 },
#endif
#ifdef TEAR_OFF
  { "/Dialogs/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Dialogs/Brushes...", "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
#if 0
  { "/Dialogs/Patterns...", "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
#endif
  { "/Dialogs/Palette...", "<control>P", dialogs_palette_cmd_callback, 0 },
  { "/Dialogs/Gradient Editor...", "<control>G", dialogs_gradient_editor_cmd_callback, 0 },
  { "/Dialogs/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
#ifdef RnH_STYLE
#ifdef TEAR_OFF
  { "/Dialogs/Flipbook/tearoff1", NULL, NULL, 0, "<Tearoff>" },
#endif
  { "/Dialogs/Flipbook/create", "<control>F", dialogs_store_frame_manager_cmd_callback, 0 },
  { "/Dialogs/Flipbook/flip forward", "W", dialogs_store_frame_manager_flip_forward_cmd_callback, 0 },
  { "/Dialogs/Flipbook/flip backward", "Q", dialogs_store_frame_manager_flip_backwards_cmd_callback, 0 },
  { "/Dialogs/Flipbook/frame forward", "S", dialogs_store_frame_manager_adv_forward_cmd_callback, 0 },
  { "/Dialogs/Flipbook/frame backward", "A", dialogs_store_frame_manager_adv_backwards_cmd_callback, 0 },
  { "/Dialogs/Flipbook/play forward", "Q", dialogs_store_frame_manager_play_forward_cmd_callback, 0 },
  { "/Dialogs/Flipbook/play backward", "Q", dialogs_store_frame_manager_play_backwards_cmd_callback, 0 },
  { "/Dialogs/Flipbook/stop", "Q", dialogs_store_frame_manager_stop_cmd_callback, 0 },
#endif
#if 0
  { "/Dialogs/Indexed Palette...", NULL, dialogs_indexed_palette_cmd_callback, 0 },
#endif
  { "/Dialogs/Tool Options...", NULL, dialogs_tools_options_cmd_callback, 0 },
  { "/Dialogs/Layers & Channels...", "<control>L", dialogs_lc_cmd_callback, 0 },
};
static guint n_image_entries = sizeof (image_entries) / sizeof (image_entries[0]);
static GtkItemFactory *image_factory = NULL;

static GtkItemFactoryEntry load_entries[] =
{
  { "/Automatic", NULL, file_load_by_extension_callback, 0 },
  { "/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_load_entries = sizeof (load_entries) / sizeof (load_entries[0]);
static GtkItemFactory *load_factory = NULL;
 
static GtkItemFactoryEntry save_entries[] =
{
  { "/By extension", NULL, file_save_by_extension_callback, 0 },
  { "/---", NULL, NULL, 0, "<Separator>" },
};
static guint n_save_entries = sizeof (save_entries) / sizeof (save_entries[0]);
static GtkItemFactory *save_factory = NULL;

static GtkItemFactoryEntry sfm_store_entries[] =
{
    {"/Store/Add", NULL, sfm_store_add_callback, 0, NULL},
    {"/Store/Delete", NULL, sfm_store_delete_callback, 0, NULL},
    {"/Store/Raise", NULL, sfm_store_raise_callback, 0, NULL},
    {"/Store/Lower", NULL, sfm_store_lower_callback, 0, NULL},
    {"/Store/Save", NULL, sfm_store_save_callback, 0, NULL},
    {"/Store/Save All", NULL, sfm_store_save_all_callback, 0, NULL},
    {"/Store/Revert", NULL, sfm_store_revert_callback, 0, NULL},
    {"/Store/Change Frame", NULL, sfm_store_change_frame_callback, 0, NULL},
    {"/View/Slide Show", NULL, sfm_slide_show_callback, 0, NULL},
    {"/Dir/Recent Src", NULL, sfm_store_recent_src_callback, 0, NULL},
    {"/Dir/Recent Dest", NULL, sfm_store_recent_dest_callback, 0, NULL},
    {"/Dir/Load Smart", NULL, sfm_store_load_smart_callback, 0, "<ToggleItem>"},
    {"/Effects/Apply Last Filter", NULL, sfm_filter_apply_last_callback, 0, NULL},
    {"/Effects/New Layer", NULL, sfm_layer_new_callback, 0, NULL},
    {"/Effects/Copy Layer", NULL, sfm_layer_copy_callback, 0, NULL},
    {"/Effects/Layer Mode", NULL, sfm_layer_mode_callback, 0, NULL},
    {"/Effects/Merge Visible", NULL, sfm_layer_merge_visible_callback, 0, NULL},
};

static guint n_sfm_store_entries = sizeof (sfm_store_entries) / sizeof (sfm_store_entries[0]);
static GtkItemFactory *sfm_store_factory = NULL;
static int sfm_store_factory_init = TRUE;

static int initialize = TRUE;

static void
menus_dummy_translate()
{
  _("Help");
  _("About...");
  _("Bugs & Kudos...");
  _("Tips...");
  _("File");
  _("New...");
  _("New From");
  _("Open...");
  _("Acquire");
  _("Preferences...");
  _("Dialogs");
  _("Palette...");
  _("Gradient Editor...");
  _("Tool Options...");
  _("Device Status...");
  _("Device Dialog");
  _("Quit");
  _("Flipbook");
  _("Create");
  _("Flip Forward");
  _("Flip Backward");
  _("Frame Forward");
  _("Frame Backward");
  _("Play Forward");
  _("Play Backward");
  _("Stop");
  _("Save");
  _("Save as...");
  _("SaveFile copy as...");
  _("Close");
  _("Redo");
  _("Cut");
  _("Copy");
  _("Paste");
  _("Paste Into");
  _("Clear");
  _("Fill");
  _("Stroke");
  _("Paste Named");
  _("Select");
  _("Toggle");
  _("Invert");
  _("Float");
  _("Grow");
  _("Shrink");
  _("Save To Channel");
  _("By Color...");
  _("View");
  _("Colormanage Display");
  _("Proof Display");
  _("Simulatate Paper");
  _("Gamut Check Display");
  _("Rendering Intent");
  _("Perceptual");
  _("Relative Colorimetric");
  _("Saturation");
  _("Absolute Colorimetric");
  _("Black Point Compensation");
  _("Look Profiles...");
  _("Expose");
  _("Pan Zoom Window");
  _("Zoom Bookmarks");
  _("Load");
  _("Save");
  _("Zoom In");
  _("Zoom Out");
  _("Zoom");
  _("Window Info...");
  _("Show Guides");
  _("Show Rulers");
  _("Toggle Rulers");
  _("Toggle Guides");
  _("Snap To Guides");
  _("Slide Show");
  _("New View");
  _("Shrink Wrap");
  _("Image");
  _("Colors");
  _("Equalize");
  _("Invert");
  _("Posterize");
  _("Threshold");
  _("Color Balance");
  _("Color Correction");
  _("Brightness-Contrast");
  _("Hue-Saturation");
  _("Gamma-Expose");
  _("Curves");
  _("Levels");
  _("Desaturate");
  _("Channel Ops");
  _("Duplicate");
  _("Offset");
  _("Alpha");
  _("Add Alpha Channel");
  _("Image");
  _(PRECISION_U8_STRING);
  _(PRECISION_U16_STRING);
  _(PRECISION_FLOAT16_STRING);
  _(PRECISION_BFP_STRING);
  _(PRECISION_FLOAT_STRING);
  _("Convert using ICC Profile...");
  _("Assign ICC Profile...");
  _("Assign ICC Proof Profile...");
  _("Layers");
  _("Stroke Splines");
  _("Layers & Channels...");
  _("Raise Layer");
  _("Lower Layer");
  _("Anchor Layer");
  _("Merge Visible Layers");
  _("Flatten Image");
  _("Alpha To Selection");
  _("Mask To Selection");
  _("Add Alpha Channel");
  _("Make Layer");
  _("Rotate");
  _("270 degrees");
  _("180 degrees");
  _("90 degrees");
  _("Tools");
  _("Select Tools");
  _("Rect Select");
  _("Ellipse Select");
  _("Free Select");
  _("Fuzzy Select");
  _("Bezier Select");
  _("Intelligent Scissors");
  _("Transform Tools");
  _("Move");
  _("Magnify");
  _("Crop");
  _("Transform");
  _("Flip");
  _("Text");
  _("Color Picker");
  _("Splines");
  _("Paint Tools");
  _("Bucket Fill");
  _("Blend");
  _("Paintbrush");
  _("Pencil");
  _("Eraser");
  _("Airbrush");
  _("Clone");
  _("Convolve");
  _("Dodge or Burn");
  _("Smudge");
  _("Default Colors");
  _("Swap Colors");
  _("Increase Brush Radius (+1)");
  _("Decrease Brush Radius (-1)");
  _("Increase clone x offset ");
  _("Decrease clone x offset");
  _("Increase clone y offset ");
  _("Decrease clone y offset ");
  _("Toogle Onionskin ");
  _("Filters");
  _("Re-show last");
  _("Repeat last");
  _("Dialogs");
  _("Brushes...");
  _("Palette...");
  _("Gradient Editor...");
  _("Layers & Channels...");
  _("Tool Options...");
  _("Automatic");
  _("By extension");
  _("Store");
  _("Add");
  _("Delete");
  _("Raise");
  _("Lower");
  _("Save");
  _("Save All");
  _("Revert");
  _("Revert to Saved");
  _("Change Frame");
  _("Dir");
  _("Recent Src");
  _("Recent Dest");
  _("Load Smart");
  _("Effects");
  _("Apply Last Filter");
  _("New Layer");
  _("Copy Layer");
  _("Layer Mode");
  _("Merge Visible");
  _("none");
  _("All");
  _("Short cuts");
  _("Toolbox");
  _("Transforms");
}

static void
menu_remove_default_accel(const gchar *name)
{
   guint i=0;

   for (i=0; i < n_image_entries; i++) {
      if (strcmp(image_entries[i].path, name) == 0) {
          image_entries[i].accelerator = NULL;
          return;
      }
   } 
}

static void
menu_set_default_accel(const gchar *name, guint accel_key, GdkModifierType accel_mods)
{
   gchar buff[1024];
   gchar *alt;
   gchar *ctrl;
   gchar *shift;
   guint i=0;

   shift = (accel_mods & GDK_SHIFT_MASK) ? "<shift>" : "";
   alt   = (accel_mods & GDK_MOD1_MASK) ? "<alt>" : "";
   ctrl  = (accel_mods & GDK_CONTROL_MASK) ? "<control>" : "";
   g_snprintf(buff, 1024, "%s%s%s%c", ctrl, shift, alt, toupper(accel_key));

   for (i=0; i < n_image_entries; i++) {
      if (strcmp(image_entries[i].path, name) == 0) {
          // I know this is a memory leak, but it's really minor, and there's no
          // easy way to avoid it  So just ignore it.  -- jcohen
          image_entries[i].accelerator = strdup(buff);
          return;
      }
   } 

}

/* Essentially, we're using this variable as a mutex.  This isn't the best
 * way of doing it perhaps, but it will work. 
 * When an accelerator is added or deleted, we will get a callback (below)
 * and we will then propagate the changes to the other menus by hand. This
 * in turn triggers the callback again and so.  This variable is to keep
 * this from happening.  It makes the UI not MT safe, but I don't think that
 * matters.  -- jcohen */ 
static gint accel_inside_update = 0;

void        
sfm_menu_add_accel_cb(
  GtkWidget *widget,
  guint accel_signal_id,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  GtkAccelFlags accel_flags,
  gpointer user_data)
{
  gchar buff[1024];
  gchar *wid_path = 0;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

//  d_printf("inside pop_up_menu_add_accel_cb\n");
  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");
// d_printf("   %s\n", wid_path);

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     cur_gdisp = (GDisplay *)disp_list->data;
     
     // since corresponding menu entries in different menus have the same name,
     // we can take advantage of that to look up the related menu item from other menus.
     g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
     related_widget =  gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
                      
     if (related_widget) {
        gtk_widget_add_accelerator(
                related_widget,
#if GTK_MAJOR_VERSION > 1
                                "activate",
#else
                                gtk_signal_name(item->accelerator_signal),
#endif
                cur_gdisp->menubar_fac->accel_group, accel_key, accel_mods, accel_flags);
     }
     else {
        d_printf("Could not find widget %s\n", buff);
     }

     disp_list = disp_list->next;
  }

  // remove from future window pulldown menus
  menu_set_default_accel(wid_path + 1, accel_key, accel_mods);
  accel_inside_update = 0;
}

void        
pop_up_menu_add_accel_cb(
  GtkWidget *widget,
  guint accel_signal_id,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  GtkAccelFlags accel_flags,
  gpointer user_data)
{
  gchar buff[1024];
  gchar *wid_path = 0;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

//  d_printf("inside pop_up_menu_add_accel_cb\n");
  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");
// d_printf("   %s\n", wid_path);

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     cur_gdisp = (GDisplay *)disp_list->data;
     
     // since corresponding menu entries in different menus have the same name,
     // we can take advantage of that to look up the related menu item from other menus.
     g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
     related_widget =  gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
                      
     if (related_widget) {
        gtk_widget_add_accelerator(
                related_widget,
#if GTK_MAJOR_VERSION > 1
                                "activate",
#else
                                gtk_signal_name(item->accelerator_signal),
#endif
                cur_gdisp->menubar_fac->accel_group, accel_key, accel_mods, accel_flags);
     }
     else {
        d_printf("Could not find widget %s\n", buff);
     }

     disp_list = disp_list->next;
  }

  // remove from future window pulldown menus
  menu_set_default_accel(wid_path + 1, accel_key, accel_mods);
  accel_inside_update = 0;
}

void        
pull_down_menu_add_accel_cb(
  GtkWidget *widget,
  guint accel_signal_id,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  GtkAccelFlags accel_flags,
  gpointer user_data)
{
  gchar buff[1024];
  gchar *wid_path;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *this_gdisp;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

//  d_printf("inside pull_down_menu_add_accel_cb\n");
  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  this_gdisp = (GDisplay *)user_data;
  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");
//  d_printf("   %s\n", wid_path);

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     if (this_gdisp != disp_list->data) {

        cur_gdisp = (GDisplay *)disp_list->data;
        // since corresponding menu entries in different menus have the same name,
        // we can take advantage of that to look up the related menu item from other menus.
        g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
        related_widget = 
                gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
        if (related_widget) {
           gtk_widget_add_accelerator(
                related_widget,
#if GTK_MAJOR_VERSION > 1
                                "activate",
#else
                                gtk_signal_name(item->accelerator_signal),
#endif
                cur_gdisp->menubar_fac->accel_group, accel_key, accel_mods, accel_flags);
        }
        else {
           d_printf("Could not find widget %s\n", buff);
        }
     }

     disp_list = disp_list->next;
  }

  // do it for the popup
  g_snprintf(buff, 1024, "<Image%s", wid_path);
  related_widget = 
         gtk_item_factory_get_widget (image_factory, buff);
  if (related_widget) {
     gtk_widget_add_accelerator(
            related_widget,
#if GTK_MAJOR_VERSION > 1
                                "activate",
#else
                                gtk_signal_name(item->accelerator_signal),
#endif
            image_factory->accel_group, accel_key, accel_mods, accel_flags);
  }
  else {
     e_printf("Could not find widget %s\n", buff);
  }
 
  menu_set_default_accel(wid_path + 1, accel_key, accel_mods);
 
  accel_inside_update = 0;
}


void        
sfm_menu_rem_accel_cb(
  GtkWidget *widget,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  gpointer user_data)
{
  gchar buff[1024];
  gchar *wid_path = 0;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *this_gdisp;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

//  d_printf("inside pull_down_menu_rem_accel_cb\n");

  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  this_gdisp = (GDisplay *)user_data;
  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     if (this_gdisp != disp_list->data) {
        cur_gdisp = (GDisplay *)disp_list->data;
        // since corresponding menu entries in different menus have the same name,
        // we can take advantage of that to look up the related menu item from other menus.
        g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
        related_widget = 
                gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
        if (related_widget) {
#if GTK_MAJOR_VERSION < 2
           gtk_widget_remove_accelerators(
                related_widget, gtk_signal_name(item->accelerator_signal),TRUE);
#endif
        }
        else {
           d_printf("Could not find widget %s\n", buff);
        }
     }

     disp_list = disp_list->next;
  }

  // do it for the popup
  g_snprintf(buff, 1024, "<Image%s", wid_path);
  related_widget = 
         gtk_item_factory_get_widget (image_factory, buff);
  if (related_widget) {
#if GTK_MAJOR_VERSION < 2
     gtk_widget_remove_accelerators(
            related_widget, gtk_signal_name(item->accelerator_signal), TRUE);
#endif
  }
  else {
     d_printf("Could not find widget %s\n", buff);
  }
 
  menu_remove_default_accel(wid_path + 1);

  accel_inside_update = 0;
}

void        
pull_down_menu_rem_accel_cb(
  GtkWidget *widget,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  gpointer user_data)
{
  gchar buff[1024];
  gchar *wid_path = 0;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *this_gdisp;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

//  d_printf("inside pull_down_menu_rem_accel_cb\n");

  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  this_gdisp = (GDisplay *)user_data;
  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     if (this_gdisp != disp_list->data) {
        cur_gdisp = (GDisplay *)disp_list->data;
        // since corresponding menu entries in different menus have the same name,
        // we can take advantage of that to look up the related menu item from other menus.
        g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
        related_widget = 
                gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
        if (related_widget) {
#if GTK_MAJOR_VERSION < 2
           gtk_widget_remove_accelerators(
                related_widget, gtk_signal_name(item->accelerator_signal), TRUE);
#endif
        }
        else {
           d_printf("Could not find widget %s\n", buff);
        }
     }

     disp_list = disp_list->next;
  }

  // do it for the popup
  g_snprintf(buff, 1024, "<Image%s", wid_path);
  related_widget = 
         gtk_item_factory_get_widget (image_factory, buff);
  if (related_widget) {
#if GTK_MAJOR_VERSION < 2
     gtk_widget_remove_accelerators(
            related_widget, gtk_signal_name(item->accelerator_signal), TRUE);
#endif
  }
  else {
     d_printf("Could not find widget %s\n", buff);
  }
 
  menu_remove_default_accel(wid_path + 1);

  accel_inside_update = 0;
}


void        
pop_up_menu_rem_accel_cb(
  GtkWidget *widget,
  GtkAccelGroup *accel_group,
  guint accel_key,
  GdkModifierType accel_mods,
  gpointer user_data)
{ 
  gchar buff[1024];
  gchar *wid_path = 0;
  GtkMenuItem * item;
  GSList * disp_list;
  GDisplay *cur_gdisp;
  GtkWidget * related_widget;

  if (accel_inside_update)
     return;
  else 
     accel_inside_update = 1;

  //  d_printf("Entering pop_up_menu_rem_accel_cb\n");
  item = (GtkMenuItem *)widget;
  wid_path = strstr(gtk_widget_get_name(widget), ">/");

  // go to all other pulldown menus and the popup menu
  // propoate changes to them as well.
  disp_list = display_list;  // traverse global list of Gdisplays
  while (disp_list != 0) {
     cur_gdisp = (GDisplay *)disp_list->data;
     
     // since corresponding menu entries in different menus have the same name,
     // we can take advantage of that to look up the related menu item from other menus.
     g_snprintf(buff, 1024, "<Image%d%s", cur_gdisp->unique_id, wid_path);
     related_widget =  gtk_item_factory_get_widget (cur_gdisp->menubar_fac, buff);
                      
     if (related_widget) {
#if GTK_MAJOR_VERSION < 2
        gtk_widget_remove_accelerators(
                related_widget, gtk_signal_name(item->accelerator_signal), TRUE);
#endif
     }
     else {
        d_printf("Could not find widget %s\n", buff);
     }

     disp_list = disp_list->next;
  }

  // remove from future window pulldown menus
  menu_remove_default_accel(wid_path + 1);

  accel_inside_update = 0;
}

void plug_in_make_image_menu              (GtkItemFactory *fac_item, gchar *buff);
void
menus_get_image_menubar(GDisplay *gdisp)
{
  gchar buff[256];
  GtkItemFactoryItem * fac_item;
  GSList *cur_slist;
  GSList *cur_wid_list;

  g_snprintf(buff, 256, "<Image%d>", gdisp->unique_id);
  gdisp->menubar_fac = gtk_item_factory_new (GTK_TYPE_MENU_BAR, buff, NULL);

  { int i;
    for(i = 0 ; i < n_image_entries; ++i)
    {
# ifdef DEBUG_
      printf("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,image_entries[i].path);
# endif
    }
  }
  gtk_item_factory_create_items_ac (gdisp->menubar_fac , n_image_entries, image_entries, NULL, 2);
  plug_in_make_image_menu(gdisp->menubar_fac, buff);

  if (!gdisp->menubar_fac->widget) {
     d_printf("menus_get_image_menubar -- could not create widget\n");
     return;
  }
  gdisp->menubar = gdisp->menubar_fac->widget;

  // iterate through all GtkMenuItmes that were created
  cur_slist = gdisp->menubar_fac->items;
  while (cur_slist != NULL) {
    fac_item = (GtkItemFactoryItem *) cur_slist->data; 
    cur_wid_list = fac_item->widgets;

    while (cur_wid_list != NULL) { 
      /* The idea is that we'd like to keep the pop-up and pulldown menus
       * synchronized. To do this, we need to be notified when a menu
       * accelerator is added or removed from one of the menus.  We will then
       * propagate these changes to the other menus by hand */
#if GTK_MAJOR_VERSION < 2
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "add-accelerator", 
                  GTK_SIGNAL_FUNC(pull_down_menu_add_accel_cb), gdisp);
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "remove-accelerator", 
                  GTK_SIGNAL_FUNC(pull_down_menu_rem_accel_cb), gdisp);
#endif
      cur_wid_list = cur_wid_list->next;
    }
    cur_slist = cur_slist->next;
  }
  menus_propagate_plugins(gdisp);
}

void
menus_get_toolbox_menubar (GtkWidget           **menubar,
			   GtkAccelGroup       **accel_group)
{
  if (initialize)
    menus_init ();

  if (menubar)
    *menubar = toolbox_factory->widget;
  if (accel_group)
    *accel_group = toolbox_factory->accel_group;
}

void
menus_get_image_menu (GtkWidget           **menu,
		      GtkAccelGroup       **accel_group)
{
  if (initialize)
    {
    menus_init ();
    }

  if (menu)
    *menu = image_factory->widget;
  if (accel_group)
    *accel_group = image_factory->accel_group;
}

void
menus_get_load_menu (GtkWidget           **menu,
		     GtkAccelGroup       **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = load_factory->widget;
  if (accel_group)
    *accel_group = load_factory->accel_group;
}

void
menus_get_save_menu (GtkWidget           **menu,
		     GtkAccelGroup       **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = save_factory->widget;
  if (accel_group)
    *accel_group = save_factory->accel_group;
}

void
menus_get_sfm_store_menu (GtkWidget **menu, GtkAccelGroup **accel_group, GDisplay *disp)
{
  gchar *filename; 
  
  if (initialize)
    menus_init ();
  
  if (sfm_store_factory_init)
  {
	  gtk_item_factory_create_items_ac (sfm_store_factory,
			  n_sfm_store_entries,
			  sfm_store_entries,
			  disp, 2);


	  filename = g_strconcat (gimp_directory (), "/menurc", NULL);
#if GTK_MAJOR_VERSION > 1
          gtk_accel_map_load(filename);
#else
	  gtk_item_factory_parse_rc (filename);
#endif
	  g_free (filename);
	  sfm_store_factory_init = FALSE; 
  }
  if (menu)
    *menu = sfm_store_factory->widget;
  if (accel_group)
    *accel_group = sfm_store_factory->accel_group; 
}

void 
menus_propagate_plugins_all()
{
  GSList * disp_list;
  GDisplay *gdisp;
  for (disp_list = display_list; disp_list != NULL; disp_list = disp_list->next) {
    gdisp = (GDisplay *)disp_list->data;
    menus_propagate_plugins(gdisp);
  }
}

void
menus_propagate_plugins(
   GDisplay *gdisp)
{
  gchar buff[1024];
  GtkMenuEntry menuEntry;
  GList *node = NULL;
  PluginMenuEntry *entry = NULL;
  GtkItemFactory *ifactory = NULL;
  GtkWidget *widget = NULL;

  for (node = g_plugin_menu_items; node != NULL; node = node->next) {
    entry = (PluginMenuEntry *)node->data;
    g_snprintf(buff, 1024, "<Image%d>%s", gdisp->unique_id,
               menu_entry_translate(entry->path));
    menuEntry.path = buff;
    if (strcmp(entry->accelerator, "NULL") == 0)
      menuEntry.accelerator = NULL;
    else
      menuEntry.accelerator = entry->accelerator;

    menuEntry.callback= entry->callback;
    menuEntry.callback_data = entry->callback_data;
    menuEntry.widget = NULL;

    // check if it doesn't already exist
    ifactory = gtk_item_factory_from_path (menuEntry.path);

    if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, menuEntry.path);

      if (!widget)
        gtk_item_factory_create_menu_entries (1, &menuEntry);
    }
  }
}

void
menus_create (GtkMenuEntry *entries,
	      int           nmenu_entries)
{
  int i=0;
  PluginMenuEntry *item;
  if (initialize)
    menus_init ();
 
  for (i=0; i < nmenu_entries; i++)
  {
    entries[i].path = menu_entry_translate( entries[i].path );
    if (strncmp(entries[i].path, "<Image>", 7) == 0)
    {
      item = g_malloc_zero(sizeof(PluginMenuEntry));
      strcpy(item->path, menu_entry_translate( entries[i].path + 7 ));
#     ifdef DEBUG_
      printf("%s:%d %s %s\n",__FILE__,__LINE__,item->path, entries[i].path);
#     endif
      if (entries[i].accelerator)
      	strcpy(item->accelerator, entries[i].accelerator);
      else
      	strcpy(item->accelerator, "NULL");
      item->callback = entries[i].callback;
      item->callback_data = entries[i].callback_data;
      g_plugin_menu_items = g_list_append(g_plugin_menu_items, (gpointer) item);
    }
  }
 
  gtk_item_factory_create_menu_entries (nmenu_entries, entries);
}

void
menus_set_sensitive (char *path,
		     int   sensitive)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;
  char *tpath = menu_entry_translate(path);

  if (initialize)
    {
    menus_init ();
    }
  
  ifactory = gtk_item_factory_from_path (tpath);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, tpath);
    
      if (widget)
	gtk_widget_set_sensitive (widget, sensitive);
    }
#ifdef DEBUG_
  if (!ifactory || !widget)
    e_printf ("Unable to set sensitivity for menu which doesn't exist:\n%s", tpath);
#endif

  if(tpath) free(tpath);
}

void
menus_set_state (char *path,
		 int   state)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;
  char *tpath = menu_entry_translate(path);

  if (initialize)
    menus_init ();

  ifactory = gtk_item_factory_from_path (tpath);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, tpath);

      if (widget && GTK_IS_CHECK_MENU_ITEM (widget))
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (widget), state);
      else
	widget = NULL;
    }
#ifdef DEBUG_
  if (!ifactory || !widget)
    e_printf ("Unable to set state for menu which doesn't exist:\n%s", tpath);
#endif

  if(tpath) free(tpath);
}

void
menus_destroy (char *path)
{
  if (initialize)
    menus_init ();

  gtk_item_factories_path_delete (NULL, path);
}

void
menus_strip_extra_entries(const char *tmp_filename, const char *filename)
{
   FILE *fpr, *fpw;
   gchar buff[1024];
   gchar *ptr;
   int filter;

   fpr = fopen(tmp_filename, "rt");
   fpw = fopen(filename, "wt");
    
   if (!fpr || !fpw) {
      d_printf("menus_strip_extra_entries -- could not open files\n");
      return;
   }

   while (!feof(fpr)) {
      filter = 0;
      fgets(buff, 1024, fpr);
      ptr = strstr(buff, "<Image");
      if (ptr) {
         filter = (ptr[6] != '>');
      }
      if (!filter) {
         fputs(buff, fpw);
      }
   }

   fclose(fpw);
   fclose(fpr);
   remove(tmp_filename);
}

void
menus_quit ()
{
  gchar tmp_filename[1024];
  gchar filename[1024];

  sprintf(tmp_filename, "%s.%d", "/tmp/.menurc", getpid());
  sprintf(filename, "%s/menurc", gimp_directory ());
#if GTK_MAJOR_VERSION > 1
  gtk_accel_map_save(tmp_filename);
#else
  gtk_item_factory_dump_rc (tmp_filename, NULL, TRUE); 
#endif

  // run through the menurc file and remove all the extra entries
  menus_strip_extra_entries(tmp_filename, filename); 

  if (!initialize)
    {
      gtk_object_unref (GTK_OBJECT (toolbox_factory));
      gtk_object_unref (GTK_OBJECT (image_factory));
      gtk_object_unref (GTK_OBJECT (load_factory));
      gtk_object_unref (GTK_OBJECT (save_factory));
      gtk_object_unref (GTK_OBJECT (sfm_store_factory));
    }
  
}

static void
menus_setup_popup_callbacks(GtkItemFactory *fac)
{
  GtkItemFactoryItem * fac_item;
  GSList *cur_slist;
  GSList *cur_wid_list;

  // iterate through all created GtkMenuItems in the factory
  cur_slist = fac->items;
  while (cur_slist != NULL) {
    fac_item = (GtkItemFactoryItem *) cur_slist->data; 
    cur_wid_list = fac_item->widgets;

    while (cur_wid_list != NULL) { 

      /* The idea is that we'd like to keep the pop-up and pulldown menus
       * synchronized. To do this, we need to be notified when a menu
       * accelerator is added or removed from one of the menus.  We will then
       * propagate these changes to the other menus by hand */
#if GTK_MAJOR_VERSION < 2
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "add-accelerator", 
                  GTK_SIGNAL_FUNC(pop_up_menu_add_accel_cb), 0);
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "remove-accelerator", 
                  GTK_SIGNAL_FUNC(pop_up_menu_rem_accel_cb), 0);
#endif
      cur_wid_list = cur_wid_list->next;
    }
    cur_slist = cur_slist->next;
  }
}

static void
menus_setup_sfm_callbacks(GtkItemFactory *fac)
{
  GtkItemFactoryItem * fac_item;
  GSList *cur_slist;
  GSList *cur_wid_list;

  // iterate through all created GtkMenuItems in the factory
  cur_slist = fac->items;
  while (cur_slist != NULL) {
    fac_item = (GtkItemFactoryItem *) cur_slist->data; 
    cur_wid_list = fac_item->widgets;

    while (cur_wid_list != NULL) { 

      /* The idea is that we'd like to keep the pop-up and pulldown menus
       * synchronized. To do this, we need to be notified when a menu
       * accelerator is added or removed from one of the menus.  We will then
       * propagate these changes to the other menus by hand */
#if GTK_MAJOR_VERSION < 2
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "add-accelerator", 
                  GTK_SIGNAL_FUNC(sfm_menu_add_accel_cb), 0);
      gtk_signal_connect(GTK_OBJECT(cur_wid_list->data), "remove-accelerator", 
                  GTK_SIGNAL_FUNC(sfm_menu_rem_accel_cb), 0);
#endif
      cur_wid_list = cur_wid_list->next;
    }
    cur_slist = cur_slist->next;
  }
}

static void
menus_init ()
{
  if (initialize)
    {
      gchar *filename;
      int i;

      initialize = FALSE;      

      toolbox_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", NULL);

# define gtk_menu_translate(entries, n_entries) { \
      for(i = 0; i < n_entries; ++i) { \
        if(entries[i].path) \
          entries[i].path = menu_entry_translate( entries[i].path ); \
      } }

      gtk_menu_translate (toolbox_entries, n_toolbox_entries)
      for(i = 0; i < n_image_entries; ++i) {
        if(image_entries[i].item_type)
        if(strstr(image_entries[i].item_type,"/View/Rendering Intent/Relative Colorimetric") ||
           strstr(image_entries[i].item_type,"/View/Rendering Intent/Saturation") ||
           strstr(image_entries[i].item_type,"/View/Rendering Intent/Perceptual"))
          image_entries[i].item_type = menu_entry_translate( image_entries[i].item_type );
      }
      gtk_menu_translate (image_entries, n_image_entries)
      gtk_menu_translate (load_entries, n_load_entries)
      gtk_menu_translate (save_entries, n_save_entries)
      gtk_menu_translate (sfm_store_entries, n_sfm_store_entries)

      gtk_item_factory_create_items_ac (toolbox_factory,
					n_toolbox_entries,
					toolbox_entries,
					NULL, 2);
      image_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Image>", NULL);
      gtk_item_factory_create_items_ac (image_factory,
					n_image_entries,
					image_entries,
					NULL, 2);

      menus_setup_popup_callbacks(image_factory);

      load_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Load>", NULL);
      gtk_item_factory_create_items_ac (load_factory,
					n_load_entries,
					load_entries,
					NULL, 2);
      save_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Save>", NULL);
      gtk_item_factory_create_items_ac (save_factory,
					n_save_entries,
					save_entries,
					NULL, 2);
      
      sfm_store_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<sfm_menu>", NULL);
      /*gtk_item_factory_create_items_ac (sfm_store_factory,
					n_sfm_store_entries,
					sfm_store_entries,
					NULL, 2);
      */menus_setup_sfm_callbacks(sfm_store_factory);
      
      filename = g_strconcat (gimp_directory (), "/menurc", NULL);
#if GTK_MAJOR_VERSION > 1
      gtk_accel_map_load(filename);

#if 0
enum {
  ADD_ACCELERATOR,
  REMOVE_ACCELERATOR,
  LAST_SIGNAL
};

static gint menu_signals[LAST_SIGNAL] = { 0 };

      menu_signals[ADD_ACCELERATOR] =
      gtk_signal_new ("add-accelerator",
                    GTK_RUN_LAST,
                    G_TYPE_FROM_CLASS (sfm_store_factory),
                    GTK_SIGNAL_OFFSET (GtkObjectClass, add_accelerator),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);
#endif
#else
      gtk_item_factory_parse_rc (filename);
#endif
      g_free (filename);
    }
}

#define DIR_SEPARATOR '/'

char*
menu_plugins_translate (const char* entry)
{
  char* new_name;
  textdomain( PACKAGE "-std-plugins" );
  new_name = menu_entry_translate (entry);
# ifdef DEBUG_
  printf( "%s:%d: %s %s | %s\n", __FILE__,__LINE__, new_name, _(entry), entry );
# endif

  textdomain( PACKAGE );
  return new_name;
}

char*
menu_entry_translate (const char* entry)
{
  char *text = (char*) calloc (1024, sizeof(char)),
       *word = 0;
  const char *tmp = 0;

# ifdef DEBUG_
  printf( "%s:%d: %s | %s\n", __FILE__,__LINE__, _(entry), entry );
# endif

  if(strchr(entry, '<'))
    tmp = entry;

  tmp = entry;

  // segmentate the path
  while (strchr( tmp, DIR_SEPARATOR ))
  {
    char *sep = 0;
    int add_separator = 0;

    // prepend <xxx> part of pathname
    if( tmp[0] == DIR_SEPARATOR )
    {
      ++tmp;
      add_separator = 1;
    }

    // erase the separator
    word = strdup( tmp );
    sep = strchr( word, DIR_SEPARATOR );
    if( sep )
      *sep = 0;

    // translate and append
    snprintf( &text [strlen( text )], 1024, "%s%s",
              add_separator ? "/" : "",
              strlen(word) ? _(word) : "" );
#   ifdef DEBUG_
    printf( "%s:%d: %s | %s\n", __FILE__,__LINE__, _(word), text );
#   endif

    // proceede
    tmp = tmp + strlen(word);

    if(word)
      free(word);
  }

  // resize memory
  if(strlen(text))
  {
    word = (char*) calloc( strlen(text) + 1, sizeof(char) );
    memcpy( word, text, strlen( text ) + 1 );
    free (text);
    return word;
  }
  else
    return (char*) calloc(1,sizeof(char));
}
