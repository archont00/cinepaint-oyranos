/* 
 * Color Management System
 * Copyright (C) 2004 Stefan Klein (kleins <at> web <dot> de)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __CMS_H__
#define __CMS_H__

#include <lcms.h>

#ifndef cmsFLAGS_GUESSDEVICECLASS
#define cmsFLAGS_GUESSDEVICECLASS 0x0020
#endif

#include <gtk/gtk.h>

#include "canvas.h"

/* a profile */
typedef struct _CMSProfile CMSProfile;
/* a transform */
typedef struct _CMSTransform CMSTransform;

/* gimage uses CMSProfile + CMSTransform */
#include "gimage.h"

/* CMSProfileType */
#include "../lib/wire/enums.h"

/* 
 * meta info about a profile, 
 * returned by cms_get_profile_info
 */
typedef struct _CMSProfileInfo
{   const char *manufacturer;
    const char *description;
    const char *pcs;
    const char *color_space_name;
    const char **color_space_channel_names;
    const char *device_class_name;
    const char *long_info;
} CMSProfileInfo;

/* values for global preference cms_open_action */
#define CMS_ASSIGN_DEFAULT 0
#define CMS_ASSIGN_PROMPT  1
#define CMS_ASSIGN_NONE    2

/* values for global preference cms_space_mismatch_action */
#define CMS_MISMATCH_PROMPT 0
#define CMS_MISMATCH_AUTO_CONVERT   1
#define CMS_MISMATCH_KEEP           2

/* 
 * compiles a list of filenames (including path) of profiles in the given directory, filters by
 * profile class, class==CMS_ANY_PROFILECLASS gives all profiles, does not recur over dirs
 * returns a list of char * or NULL in case of error
 */
#define CMS_ANY_PROFILECLASS -1
GSList *cms_read_icc_profile_dir(gchar *path, icProfileClassSignature class);

/* 
 * compiles a list of filenames (including path) of all profiles in the global preference
 * cms_profile_path filters by
 * profile class, class==CMS_ANY_PROFILECLASS gives all profiles, does not recur over dirs
 * returns a list of char * or NULL in case of error
 */
GSList *cms_read_standard_profile_dirs(icColorSpaceSignature class);

/*
 * get a handle to the profile in file filename
 * either from the cache or from the file
 * returns NULL on error
 */
CMSProfile *cms_get_profile_from_file(char *file_name);

/* registers a profile that lies in memory with the cms
 * (used for embedded profiles opened by plug-ins)
 * again we check the cache to see whether the profile is already
 * there, if it is, we use the cache handle and free the memory
 * (so never access it after the call to this function!)
 */
CMSProfile *
cms_get_profile_from_mem(void *mem_pointer, DWORD size);

/*
 * returns the ICC profile else NULL
 */
char *
cms_get_profile_data(CMSProfile *profile, size_t* size);

/* get a handle to a (lcms built-in) lab profile with given white_point */
CMSProfile *cms_get_lab_profile(LPcmsCIExyY white_point);

/* get a handle to a (lcms built-in) xyz profile */
CMSProfile *cms_get_xyz_profile();

/* get a handle to a (lcms built-in) srgb profile */
CMSProfile *cms_get_srgb_profile();

/* 
 * makes a 'copy' of the profile handle 
 * (essentially only admits a new user of the handle
 * by increasing the reference counter)
 * returns NULL if profile not found in cache (i.e. not open)
 */ 
CMSProfile *cms_duplicate_profile(CMSProfile *profile);

/* 
 * return a profile to the cache
 */ 
gboolean cms_return_profile(CMSProfile *profile);

/*
 * gets meta information about the profile
 * (only the manufacturer and description for the moment)
 * returned in a fixed buffer that is overwritten by 
 * subsequent calls
 */
CMSProfileInfo *cms_get_profile_info(CMSProfile *profile);

const char * cms_get_profile_cspace ( CMSProfile         * profile );

/* returns the lcms format corresponding to this tag and profile */
DWORD cms_get_lcms_format(Tag tag, CMSProfile *profile);

/*
 * get a transform based 
 * on input and output format
 * parameters as in lcms's createMultiTransform
 * returns NULL on error
 */
/* profiles list order:
       1. (pos 0)    image profile
       2. (pos 1..n) look profiles (abstact profiles)
       3. (pos n+1)  proofing profile , only if proofing is set in lcms_flags
                                        cmsFLAGS_SOFTPROOFING
       4. (pos n+2 (n+1 - without proofing)) display profile
*/
CMSTransform *cms_get_transform(GSList *profiles,
				DWORD lcms_input_format,
                                DWORD lcms_output_format,
				int lcms_intent, DWORD lcms_flags,
                                int proof_intent,
                                Tag input, Tag output,
                                CMSTransform **expensive_transform);

/*
 * for convenience: get a transform usable for a canvas
 * (format is determined by this function)
 * returns NULL on error
 */
CMSTransform *cms_get_canvas_transform(GSList *profiles, Tag src, Tag dest, 
				       int lcms_intent, DWORD lcms_flags, int proof_intent,
                       CMSTransform **expensive_transform);

/*
 * Set if the source profile has 4 colors and is converted to less color
 * channnels, for erasing alpha in the resulting buffer (CMYK hack)
 */
void erase_alpha_from_4th_color (CMSTransform *transform);


/*
 * return a transform to the cache 
 */
gboolean cms_return_transform(CMSTransform *transform);


/*
 * get the display profile, if set
 */
CMSProfile *cms_get_display_profile();

/*
 * set the display profile, return value indicates success
 */
gboolean cms_set_display_profile(CMSProfile *profile);

/* 
 * does initializations
 * loads input and display profile 
 * and calculates standard display transform (input->display)
 * called from app_procs.c:app_init()
 * called from the preferences dialog
 */
void cms_init();

/*
 * tidies up, called from app_procs.c:app_exit_finish()
 */
void cms_free();


 /* transform a buffer, src and dest have to be same precision */
void cms_transform_buffer( CMSTransform *transform,
                           void *src, void *dest,
                           int n_pixel,
                           Precision precision);
/* transforms a pixel area, src and dest have to be same precision */
void cms_transform_area(   CMSTransform *transform,
                           PixelArea *src_area, PixelArea *dest_area);
/* transforms a whole image, layer by layer,
   src and dest have to be same precision */
void cms_transform_image(  CMSTransform *transform, GImage *image);

/* transforms data directly, depending on type, 
   these are made public, so they can be used for fast, direct transformations
   avoiding cms_transform_area
 */
void cms_transform_uint(CMSTransform *transform, void *src_data, void *dest_data, int num_pixels);
void cms_transform_float(CMSTransform *transform, void *src_data, void *dest_data, int num_pixels);

/* set a profile to a image, accept prof with NULL to select a default */
void cms_gimage_open          ( GImage * gimage );
void cms_gimage_check_profile ( GImage * gimage, CMSProfileType type);


/* INTERFACE*/
typedef struct _CMSProfileSelectionDialog CMSProfileSelectionDialog;

#define CMS_PROFILE_ONLY 0
#define CMS_PROFILE_INTENT 1

/*typedef enum
{
  ICC_IMAGE_PROFILE,
  ICC_PROOF_PROFILE
} CMSProfileType;*/

CMSProfileSelectionDialog *cms_profile_selection_dialog_new(GImage *image, int type, GtkSignalFunc ok_callback);
void cms_profile_selection_dialog_destroy(CMSProfileSelectionDialog *dialog);
GImage *cms_profile_selection_dialog_get_image(CMSProfileSelectionDialog *dialog);
char *cms_profile_selection_dialog_get_profile(CMSProfileSelectionDialog *dialog);
guint8 cms_profile_selection_dialog_get_intent(CMSProfileSelectionDialog *dialog);

void cms_convert_dialog(GImage *image);
void cms_assign_dialog(GImage *image, CMSProfileType type);
void cms_open_assign_dialog(GImage *image);


#endif /*  __CMS_H__  */

