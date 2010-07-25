/*
 *
 */

#ifndef STORE_FRAME_MANAGER
#define STORE_FRAME_MANAGER

#include "gdisplay.h"

void sfm_slide_show (GtkWidget *w, gpointer data);

void sfm_create_gui (GDisplay *);
void sfm_dialog_create ();
void sfm_onionskin_toogle_callback (GtkWidget *, gpointer);
void sfm_onionskin_set_offset (GDisplay*, int, int);
void sfm_onionskin_rm (GDisplay*);
void sfm_store_add_image (GImage *, GDisplay *, int, int, int);
void sfm_setting_aofi(GDisplay *);
void sfm_dirty (GDisplay *, int pos);
void sfm_set_bg (GDisplay *, int pos);
GDisplay* sfm_load_image_into_fm (GDisplay *, GImage *);

void sfm_store_add_callback (GtkWidget *, gpointer);
void sfm_store_delete_callback (GtkWidget *, gpointer);
void sfm_store_raise_callback (GtkWidget *, gpointer);
void sfm_store_lower_callback (GtkWidget *, gpointer);
void sfm_store_save_callback (GtkWidget *, gpointer);
void sfm_store_save_all_callback (GtkWidget *, gpointer);
void sfm_store_revert_callback (GtkWidget *, gpointer);
void sfm_store_change_frame_callback (GtkWidget *, gpointer);
void sfm_filter_apply_callback (GtkWidget *, gpointer);

void sfm_set_dir_src (GDisplay *);
void sfm_set_dir_dest (GDisplay *);

void sfm_store_recent_src_callback (GtkWidget *, gpointer);
void sfm_store_recent_dest_callback (GtkWidget *, gpointer);
void sfm_store_load_smart_callback (GtkWidget *, gpointer);
void sfm_slide_show_callback (GtkWidget *, gpointer);

gint sfm_adv_forward (GDisplay *);
gint sfm_adv_backwards (GDisplay *);
void sfm_flip_forward (GDisplay *);
void sfm_flip_backwards (GDisplay *);
void sfm_play_forward (GDisplay *);
void sfm_play_backwards (GDisplay *);
void sfm_stop (GDisplay *);
int  sfm_plays (GDisplay *);

void sfm_filter_apply_last_callback(GtkWidget *widget, void* data);
void sfm_layer_new_callback(GtkWidget *widget, void* data);
void sfm_layer_copy_callback(GtkWidget *widget, void* data);
void sfm_layer_mode_callback(GtkWidget *widget, void* data);
void sfm_layer_merge_visible_callback(GtkWidget *widget, void* data);

int  sfm_pos_of_ID (GDisplay *, int imageID); /* -hsbo (used in clone.c) */

#endif

