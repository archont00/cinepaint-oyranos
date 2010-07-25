/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMP_H__
#define __GIMP_H__


#include <glib.h>

#include "version.h"
#include "compat.h"
#include "wire/wire_types.h"
#include "wire/libtile.h"
#include "float16.h"
#include "unit.h"
#include "pdb.h"
#include "libgimp/gimp.h"
#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


extern const guint gimp_major_version;
extern const guint gimp_minor_version;
extern const guint gimp_micro_version;


DLL_API void set_gimp_PLUG_IN_INFO (const GPlugInInfo *p);

extern int   argc;
extern char **argv;

#define MAIN() int main (int argc, char *argv[]) { printf("%s:%d\n",__FILE__,__LINE__); return plugin_main (argc, argv, &PLUG_IN_INFO); }


/* The main procedure that should be called with the
 *  'argc' and 'argv' that are passed to "main".
 */
DLL_API int plugin_main (int argc, char **argv, const GPlugInInfo *p);

/* Forcefully causes the gimp library to exit and
 *  close down its connection to main gimp application.
 */
DLL_API void G_GNUC_NORETURN gimp_quit (void);

/* Specify a range of data to be associated with 'id'.
 *  The data will exist for as long as the main gimp
 *  application is running.
 */
DLL_API void gimp_set_data (gchar *  id,
		    gpointer data,
		    guint32  length);

/* Retrieve the piece of data stored within the main
 *  gimp application specified by 'id'. The data is
 *  stored in the supplied buffer.  Make sure enough
 *  space is allocated.
 */
DLL_API void gimp_get_data (gchar *  id,
		    gpointer data);


/* Pops up a dialog box with "message". Useful for status and
 * error reports. If "message" is NULL, do nothing.
 */
DLL_API void gimp_message (const char *message);

/* Query the gimp application's procedural database.
 *  The arguments are regular expressions which select
 *  which procedure names will be returned in 'proc_names'.
 */
DLL_API void gimp_query_database (char   *name_regexp,
			  char   *blurb_regexp,
			  char   *help_regexp,
			  char   *author_regexp,
			  char   *copyright_regexp,
			  char   *date_regexp,
			  char   *proc_type_regexp,
			  int    *nprocs,
			  char ***proc_names);

/* Query the gimp application's procedural database
 *  regarding a particular procedure.
 */
DLL_API gint gimp_query_procedure  (char       *proc_name,
			    char      **proc_blurb,
			    char      **proc_help,
			    char      **proc_author,
			    char      **proc_copyright,
			    char      **proc_date,
			    int        *proc_type,
			    int        *nparams,
			    int        *nreturn_vals,
			    GParamDef  **params,
			    GParamDef  **return_vals);

/* Query the gimp application regarding all open images.
 *  The list of open image id's is returned in 'image_ids'.
 */
gint32* gimp_query_images (int *nimages);


/* Install a procedure in the procedure database.
 */
DLL_API void gimp_install_procedure (char      *name,
			     char      *blurb,
			     char      *help,
			     char      *author,
			     char      *copyright,
			     char      *date,
			     char      *menu_path,
			     char      *image_types,
			     int        type,
			     int        nparams,
			     int        nreturn_vals,
			     GParamDef *params,
			     GParamDef *return_vals);

/* Install a temporary procedure in the procedure database.
 */
void gimp_install_temp_proc (char      *name,
			     char      *blurb,
			     char      *help,
			     char      *author,
			     char      *copyright,
			     char      *date,
			     char      *menu_path,
			     char      *image_types,
			     int        type,
			     int        nparams,
			     int        nreturn_vals,
			     GParamDef *params,
			     GParamDef *return_vals,
			     GRunProc   run_proc);

/* Uninstall a temporary procedure
 */
void gimp_uninstall_temp_proc (char *name);

/* Install a load file format handler in the procedure database.
 */
DLL_API void gimp_register_magic_load_handler (char *name,
				       char *extensions,
				       char *prefixes,
				       char *magics);

/* Install a load file format handler in the procedure database.
 */
DLL_API void gimp_register_load_handler (char *name,
				 char *extensions,
				 char *prefixes);

/* Install a save file format handler in the procedure database.
 */
DLL_API void gimp_register_save_handler (char *name,
				 char *extensions,
				 char *prefixes);

/* Run a procedure in the procedure database. The parameters are
 *  specified via the variable length argument list. The return
 *  values are returned in the 'GParam*' array.
 */
DLL_API GParam* gimp_run_procedure (char *name,
			    int  *nreturn_vals,
			    ...);

/* Run a procedure in the procedure database. The parameters are
 *  specified as an array of GParam.  The return
 *  values are returned in the 'GParam*' array.
 */
GParam* gimp_run_procedure2 (char   *name,
			     int    *nreturn_vals,
			     int     nparams,
			     GParam *params);

/* Destroy the an array of parameters. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_run_procedure'.
 */
DLL_API void gimp_destroy_params (GParam *params,
			  int     nparams);

DLL_API gdouble  gimp_gamma        (void);
DLL_API gint     gimp_install_cmap (void);
DLL_API gint     gimp_use_xshm     (void);
DLL_API guchar*  gimp_color_cube   (void);
DLL_API gchar*   gimp_gtkrc        (void);

char*      gimp_get_default_monitor_profile   ();
char*      gimp_get_default_image_profile     ();
char*      gimp_get_default_workspace_profile ();
gint32     gimp_get_default_intent_profile    ();
gint32     gimp_get_default_flags_profile     ();


/****************************************
 *              Images                  *
 ****************************************/

DLL_API gint32     gimp_image_new                   (guint      width,
					     guint      height,
					     GImageType type);
DLL_API void       gimp_image_delete                (gint32     image_ID);
DLL_API guint      gimp_image_width                 (gint32     image_ID);
DLL_API guint      gimp_image_height                (gint32     image_ID);
DLL_API GImageType gimp_image_base_type             (gint32     image_ID);
gint32     gimp_image_floating_selection    (gint32     image_ID);
DLL_API void       gimp_image_add_channel           (gint32     image_ID,
                                                     gint32     channel_ID,
                                                     gint       position);
DLL_API void       gimp_image_add_layer             (gint32     image_ID,
                                                     gint32     layer_ID,
                                                     gint       position);
DLL_API void       gimp_image_add_layer_mask        (gint32     image_ID,
                                                     gint32     layer_ID,
                                                     gint32     mask_ID);
DLL_API void       gimp_image_clean_all             (gint32     image_ID);
DLL_API void       gimp_image_disable_undo          (gint32     image_ID);
DLL_API void       gimp_image_enable_undo           (gint32     image_ID);
DLL_API gint32     gimp_image_flatten               (gint32     image_ID);
gint       gimp_image_is_layered   	    (gint32     image_ID);
gint       gimp_image_dirty_flag   	    (gint32     image_ID);
void       gimp_image_lower_channel         (gint32     image_ID,
                                             gint32     channel_ID);
void       gimp_image_lower_layer           (gint32     image_ID,
                                             gint32     layer_ID);
gint32     gimp_image_merge_visible_layers  (gint32     image_ID,
                                             GimpMergeType       merge_type);
gint32     gimp_image_pick_correlate_layer  (gint32     image_ID,
                                             gint       x,
                                             gint       y);
void       gimp_image_raise_channel         (gint32     image_ID,
                                             gint32     channel_ID);
void       gimp_image_raise_layer           (gint32     image_ID,
                                             gint32     layer_ID);
void       gimp_image_remove_channel        (gint32     image_ID,
                                             gint32     channel_ID);
void       gimp_image_remove_layer          (gint32     image_ID,
                                             gint32     layer_ID);
void       gimp_image_remove_layer_mask     (gint32     image_ID,
                                             gint32     layer_ID,
                                             gint       mode);
DLL_API void       gimp_image_resize        (gint32     image_ID,
                                             guint      new_width,
                                             guint      new_height,
                                             gint       offset_x,
                                             gint       offset_y);
gint32     gimp_image_get_active_channel    (gint32     image_ID);
DLL_API gint32     gimp_image_get_active_layer      (gint32     image_ID);
DLL_API gint32*    gimp_image_get_channels          (gint32     image_ID,
                                                     gint      *nchannels);
DLL_API guchar*    gimp_image_get_cmap              (gint32     image_ID,
                                                     gint      *ncolors);
gint       gimp_image_get_component_active  (gint32     image_ID,
                                             GimpChannelType       component);
gint       gimp_image_get_component_visible (gint32     image_ID,
                                             GimpChannelType       component);
gboolean   gimp_image_get_resolution        (gint32     image_ID,
                                             gdouble    *xresolution,
                                             gdouble    *yresolution);

DLL_API char*      gimp_image_get_filename          (gint32     image_ID);
DLL_API gint32*    gimp_image_get_layers            (gint32     image_ID,
                                                     gint      *nlayers);
gint32     gimp_image_get_selection         (gint32     image_ID);
void       gimp_image_set_active_channel    (gint32     image_ID,
                                             gint32     channel_ID);
void       gimp_image_set_active_layer      (gint32     image_ID,
                                             gint32     layer_ID);
DLL_API void       gimp_image_set_cmap      (gint32     image_ID,
                                             guchar    *cmap,
                                             gint       ncolors);
void       gimp_image_set_component_active  (gint32     image_ID,
                                             gint       component,
                                             gint       active);
void       gimp_image_set_component_visible (gint32     image_ID,
                                             gint       component,
                                             gint       visible);
DLL_API void       gimp_image_set_filename  (gint32     image_ID,
                                             char      *name);
void gimp_bfm_set_dir_src    (gint32     disp_ID, char *filename);
void gimp_bfm_set_dir_dest    (gint32     disp_ID, char *filename);

void       gimp_image_set_icc_profile_by_name (gint32   image_ID,
                                               gchar   *name,
                                               CMSProfileType type);
void       gimp_image_set_icc_profile_by_mem  (gint32   image_ID,
                                               gint     size,
                                               gchar   *data,
                                               CMSProfileType type);
void       gimp_image_set_lab_profile         (gint32   image_ID);
void       gimp_image_set_xyz_profile         (gint32   image_ID);
void       gimp_image_set_srgb_profile        (gint32   image_ID);
gboolean   gimp_image_has_icc_profile         (gint32   image_ID,
                                               CMSProfileType type);
char*      gimp_image_get_icc_profile_by_mem  (gint32   image_ID,
                                               gint    *size,
                                               CMSProfileType type);
char*      gimp_image_get_icc_profile_description (gint32  image_ID,
                                                   CMSProfileType type);
char*      gimp_image_get_icc_profile_info    (gint32   image_ID,
                                               CMSProfileType type);
char*      gimp_image_get_icc_profile_pcs     (gint32   image_ID,
                                               CMSProfileType type);
char*      gimp_image_get_icc_profile_color_space_name  (gint32   image_ID,
                                                         CMSProfileType type);
char*      gimp_image_get_icc_profile_device_class_name (gint32   image_ID,
                                                         CMSProfileType type);

/****************************************
 *             Displays                 *
 ****************************************/

DLL_API gint32 gimp_display_new    (gint32 image_ID);
gint32 gimp_display_fm    (gint32 image_ID, gint32 disp_ID);
gint32 gimp_display_active  ();
void   gimp_display_delete (gint32 display_ID);
void   gimp_displays_delete_image (gint32 image_ID);
DLL_API void   gimp_displays_flush (void);
int        gimp_display_get_image_id         ( gint32   display_ID );
gint32     gimp_display_get_cms_intent        (gint32   display_ID,
                                               CMSProfileType type);
void       gimp_display_set_cms_intent        (gint32   display_ID,
                                               gint32   intent,
                                               CMSProfileType type);
gint32     gimp_display_get_cms_flags         (gint32   display_ID);
void       gimp_display_set_cms_flags         (gint32   display_ID,
                                               gint32   flags);
gint32     gimp_display_is_colormanaged       (gint32   display_ID,
                                               CMSProfileType type);
void       gimp_display_set_colormanaged      (gint32   display_ID,
                                               gboolean True_False,
                                               CMSProfileType type);
void       gimp_display_all_set_colormanaged  (gboolean True_False);
void       gimp_display_image_set_colormanaged(gint32   display_ID,
                                               gboolean True_False);

/****************************************
 *              Layers                  *
 ****************************************/

DLL_API gint32        gimp_layer_new                       (gint32        image_ID,
						    char         *name,
						    guint         width,
						    guint         height,
						    GDrawableType  type,
						    gdouble       opacity,
						    GLayerMode    mode);
gint32        gimp_layer_copy                      (gint32        layer_ID);
DLL_API void          gimp_layer_delete                    (gint32        layer_ID);
guint         gimp_layer_width                     (gint32        layer_ID);
guint         gimp_layer_height                    (gint32        layer_ID);
guint         gimp_layer_bpp                       (gint32        layer_ID);
GDrawableType gimp_layer_type                      (gint32       layer_ID);
DLL_API void          gimp_layer_add_alpha                 (gint32        layer_ID);
DLL_API gint32        gimp_layer_create_mask               (gint32        layer_ID,
						    gint          mask_type);
DLL_API void          gimp_layer_resize                    (gint32        layer_ID,
						    guint         new_width,
						    guint         new_height,
						    gint          offset_x,
						    gint          offset_y);
void          gimp_layer_scale                     (gint32        layer_ID,
						    guint         new_width,
						    guint         new_height,
						    gint          local_origin);
DLL_API void          gimp_layer_translate                 (gint32        layer_ID,
						    gint          offset_x,
						    gint          offset_y);
DLL_API gint          gimp_layer_is_floating_selection     (gint32        layer_ID);
gint32        gimp_layer_get_image_id              (gint32        layer_ID);
gint32        gimp_layer_get_mask_id               (gint32        layer_ID);
gint          gimp_layer_get_apply_mask            (gint32        layer_ID);
gint          gimp_layer_get_edit_mask             (gint32        layer_ID);
DLL_API GLayerMode    gimp_layer_get_mode                  (gint32        layer_ID);
DLL_API char*         gimp_layer_get_name                  (gint32        layer_ID);
DLL_API gdouble       gimp_layer_get_opacity               (gint32        layer_ID);
DLL_API gint          gimp_layer_get_preserve_transparency (gint32        layer_ID);
gint          gimp_layer_get_show_mask             (gint32        layer_ID);
DLL_API gint          gimp_layer_get_visible               (gint32        layer_ID);
void          gimp_layer_set_apply_mask            (gint32        layer_ID,
						    gint          apply_mask);
void          gimp_layer_set_edit_mask             (gint32        layer_ID,
						    gint          edit_mask);
DLL_API void          gimp_layer_set_mode                  (gint32        layer_ID,
						    GLayerMode    mode);
DLL_API void          gimp_layer_set_name                  (gint32        layer_ID,
						    char         *name);
DLL_API void          gimp_layer_set_offsets               (gint32        layer_ID,
						    gint          offset_x,
						    gint          offset_y);
void          gimp_layer_set_opacity               (gint32        layer_ID,
						    gdouble       opacity);
DLL_API void          gimp_layer_set_preserve_transparency (gint32        layer_ID,
						    gint          preserve_transparency);
void          gimp_layer_set_show_mask             (gint32        layer_ID,
						    gint          show_mask);
DLL_API void          gimp_layer_set_visible               (gint32        layer_ID,
						    gint          visible);


/****************************************
 *             Channels                 *
 ****************************************/

DLL_API gint32  gimp_channel_new             (gint32   image_ID,
				      char    *name,
				      guint    width,
				      guint    height,
				      gdouble  opacity,
				      guchar  *color);
gint32  gimp_channel_copy            (gint32   channel_ID);
void    gimp_channel_delete          (gint32   channel_ID);
guint   gimp_channel_width           (gint32   channel_ID);
guint   gimp_channel_height          (gint32   channel_ID);
gint32  gimp_channel_get_image_id    (gint32   channel_ID);
gint32  gimp_channel_get_layer_id    (gint32   channel_ID);
void    gimp_channel_get_color       (gint32   channel_ID,
				      guchar  *red,
				      guchar  *green,
				      guchar  *blue);
DLL_API char*   gimp_channel_get_name        (gint32   channel_ID);
gdouble gimp_channel_get_opacity     (gint32   channel_ID);
gint    gimp_channel_get_show_masked (gint32   channel_ID);
gint    gimp_channel_get_visible     (gint32   channel_ID);
void    gimp_channel_set_color       (gint32   channel_ID,
				      guchar   red,
				      guchar   green,
				      guchar   blue);
void    gimp_channel_set_name        (gint32   channel_ID,
				      char    *name);
void    gimp_channel_set_opacity     (gint32   channel_ID,
				      gdouble  opacity);
void    gimp_channel_set_show_masked (gint32   channel_ID,
				      gint     show_masked);
DLL_API void    gimp_channel_set_visible     (gint32   channel_ID,
				      gint     visible);


/****************************************
 *             GDrawables                *
 ****************************************/

DLL_API TileDrawable*    gimp_drawable_get          (gint32     drawable_ID);
DLL_API void          gimp_drawable_detach       (TileDrawable *drawable);
DLL_API void          gimp_drawable_flush        (TileDrawable *drawable);
void          gimp_drawable_delete       (TileDrawable *drawable);
DLL_API void          gimp_drawable_update       (gint32     drawable_ID,
					  gint       x,
					  gint       y,
					  guint      width,
					  guint      height);
DLL_API void          gimp_drawable_merge_shadow (gint32     drawable_ID,
					  gint       undoable);
gint32        gimp_drawable_image_id     (gint32     drawable_ID);
char*         gimp_drawable_name         (gint32     drawable_ID);
DLL_API guint         gimp_drawable_width        (gint32     drawable_ID);
DLL_API guint         gimp_drawable_height       (gint32     drawable_ID);
DLL_API guint         gimp_drawable_bpp          (gint32     drawable_ID);
DLL_API guint         gimp_drawable_num_channels (gint32     drawable_ID);
DLL_API GDrawableType gimp_drawable_type         (gint32     drawable_ID);
DLL_API GPrecisionType gimp_drawable_precision    (gint32     drawable_ID);
gint          gimp_drawable_visible      (gint32     drawable_ID);
gint          gimp_drawable_channel      (gint32     drawable_ID);
DLL_API gint          gimp_drawable_color        (gint32     drawable_ID);
DLL_API gint          gimp_drawable_gray         (gint32     drawable_ID);
DLL_API gint          gimp_drawable_has_alpha    (gint32     drawable_ID);
gint          gimp_drawable_indexed      (gint32     drawable_ID);
DLL_API gint          gimp_drawable_layer        (gint32     drawable_ID);
gint          gimp_drawable_layer_mask   (gint32     drawable_ID);
DLL_API gint          gimp_drawable_mask_bounds  (gint32     drawable_ID,
					  gint      *x1,
					  gint      *y1,
					  gint      *x2,
					  gint      *y2);
DLL_API void          gimp_drawable_offsets      (gint32     drawable_ID,
					  gint      *offset_x,
					  gint      *offset_y);
void          gimp_drawable_fill         (gint32     drawable_ID,
					  gint       fill_type);
void          gimp_drawable_set_name     (gint32     drawable_ID,
					  char      *name);
void          gimp_drawable_set_visible  (gint32     drawable_ID,
					  gint       visible);
DLL_API GTile*        gimp_drawable_get_tile     (TileDrawable *drawable,
					  gint       shadow,
					  gint       row,
					  gint       col);
GTile*        gimp_drawable_get_tile2    (TileDrawable *drawable,
					  gint       shadow,
					  gint       x,
					  gint       y);

/****************************************
 *           Pixel Regions              *
 ****************************************/

DLL_API void     gimp_pixel_rgn_init      (GPixelRgn *pr,
				   TileDrawable *drawable,
				   int        x,
				   int        y,
				   int        width,
				   int        height,
				   int        dirty,
				   int        shadow);
void     gimp_pixel_rgn_resize    (GPixelRgn *pr,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
void     gimp_pixel_rgn_get_pixel (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y);
DLL_API void     gimp_pixel_rgn_get_row   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width);
DLL_API void     gimp_pixel_rgn_get_col   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        height);
DLL_API void     gimp_pixel_rgn_get_rect  (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
void     gimp_pixel_rgn_set_pixel (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y);
DLL_API void     gimp_pixel_rgn_set_row   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width);
DLL_API void     gimp_pixel_rgn_set_col   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        height);
DLL_API void     gimp_pixel_rgn_set_rect  (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
DLL_API gpointer gimp_pixel_rgns_register (int        nrgns,
				   ...);
DLL_API gpointer gimp_pixel_rgns_process  (gpointer   pri_ptr);


/****************************************
 *            The Palette               *
 ****************************************/

DLL_API void gimp_palette_get_background (guchar *red,
				  guchar *green,
				  guchar *blue);
void gimp_palette_get_foreground (guchar *red,
				  guchar *green,
				  guchar *blue);
void gimp_palette_set_background (guchar  red,
				  guchar  green,
				  guchar  blue);
void gimp_palette_set_foreground (guchar  red,
				  guchar  green,
				  guchar  blue);

/****************************************
 *            Gradients                 *
 ****************************************/

char**   gimp_gradients_get_list       (gint    *num_gradients);
char*    gimp_gradients_get_active     (void);
void     gimp_gradients_set_active     (char    *name);
gdouble* gimp_gradients_sample_uniform (gint     num_samples);
gdouble* gimp_gradients_sample_custom  (gint     num_samples,
					gdouble *positions);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_H__ */
