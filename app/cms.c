/* 
 * CinePaint Color Management System
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


#include "cms.h"

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <gtk/gtk.h>

#ifdef __FreeBSD__ 
# define USE_ALLOCA
#else
# undef USE_ALLOCA
# define MAX_NUM_PROFILES 1024 /* ARGH */
#endif

#ifdef USE_ALLOCA
# include <stdlib.h>
#endif

#ifdef HAVE_OY
#ifndef OYRANOS_VERSION
#define OYRANOS_VERSION 0
#endif
#if OYRANOS_VERSION < 108
#include <oyranos/oyranos.h>
#include <arpa/inet.h>  /* ntohl */
#include <oyranos/oyranos_monitor.h>
#else
#include <oyranos.h>
#if OYRANOS_VERSION < 110
#include <oyranos_alpha.h>
#else
#include <alpha/oyranos_alpha.h>
#endif
#endif
#if OYRANOS_VERSION > 107
#include "oyProfiles_s.h"
#endif
#endif

#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "depth/float16.h"
#include "convert.h"
#include "fileops.h"
#include "general.h"
#include "gdisplay.h"
#include "gimage_cmds.h"
#include "interface.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "rc.h"
#include "tag.h"


/* parameter for the transform cache */
/* how many transform cache entries can be kept without being used by anyone
 * (for efficiency, since likely to be used again soon)
 * (transforms can be really big, at least 200K, up to megs) */
#define MAX_UNUSED_TRANSFORMS_IN_MEM 1
/* (note that this does not restrict the number of transforms that can be in use
 * at the same time from calling cms_get_transform, only the number that is kept
 * in mem or on disc while not being used for efficiency,
 * also, pre-calculated transforms are always cached on disc, no matter whether the
 * memory cache is exceeded)
 */


/* definition of lcms constants */
#define TYPE_RGBA_DBL  (COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(0))
#define TYPE_GRAYA_DBL (COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(0))


/*** TYPE DECLARATIONS ***/
/* the func used to transform, depending on the data being float or uint */
typedef void  (*TransformFunc) (CMSTransform *transform, void *src_data, void *dest_data, int num_pixels);


/*** TYPES ***/
/* an entry in the profile cache */
typedef struct _ProfileCacheEntry
{   CMSProfile *profile;
    gint     ref_count;
} ProfileCacheEntry;

/* an entry in the profile cache,
   the transform is a transform in memory,
   the device_link_file is the file name of a temporary 
   pre-calculated transform stored on disc,
   device_link_handle is the handle to it */
typedef struct _TransformCacheEntry
{   CMSTransform *transform;
    char*         device_link_file;
    gint          ref_count;
} TransformCacheEntry;

/* a profile
 * cache_key is the key in the cache, handle the lcms handle
 */
struct _CMSProfile
{   gchar      *cache_key;
    cmsHPROFILE handle;
    char       *data;                   /* save original data for profile i/o */
    size_t      size;
    char        cspace[8];
};

/* same for transform */
struct _CMSTransform
{   gchar *cache_key;
    Tag src_tag;
    Tag dest_tag;
    cmsHTRANSFORM handle;
    icColorSpaceSignature colourspace_in;  /*!< source/image colour space */
    DWORD lcms_input_format;        /*!< put information about alpha ... */
    DWORD lcms_output_format;       /*!< put information about alpha ... */
};
    
/*** VARIABLE DECLARATIONS ***/

/* handler of display profile
   loaded in cms_init, according to user-preferences */ 
CMSProfile *display_profile = NULL;

/* caches for profiles and transforms */
/* contains profile filename as key, profile handle as value */
static GHashTable *profile_cache = NULL;
/* key build out of profile pointers + lcms parameters, 
   contains devicelink handle as value */
static GHashTable *transform_cache = NULL;
/* transform cache entries that are not in use, 
   but kept in memory for efficiency
   will be few, < MAX_TRANSFORMS_KEPT_IN_MEM */
static GList *unused_transforms_kept_in_mem = NULL;

/* buffer to return profile information in */
static CMSProfileInfo *profile_info_buffer = NULL;




/*** FUNCTION DECLARATIONS ***/
/* initialize the current transformation by calculating it or 
   reloading from disk */
gboolean cms_init_transform(GDisplay *display);

/* function to empty the profile/transform cache hashtables,
   close the profiles*/
gboolean cms_empty_transform_cache(gpointer key, gpointer value, gpointer user_data);

/* Auxiliaries to extract profile info */
const char* cms_get_long_profile_info (cmsHPROFILE hProfile);
const char* cms_get_profile_keyname   (cmsHPROFILE hProfile, char* mem);
const char* cms_get_pcs_name          (cmsHPROFILE hProfile);
const char* cms_get_color_space_name  (cmsHPROFILE hProfile);
const char** cms_get_color_space_channel_names (cmsHPROFILE hProfile);
const char* cms_get_device_class_name (cmsHPROFILE hProfile);

/* UI functions */
gboolean cms_convert_on_open_prompt(GImage *image);


/* workarround for some newer distributions with old lcms - beku */
#if (LCMS_VERSION <= 112)
int _cmsLCMScolorSpace(icColorSpaceSignature ProfileSpace);
#endif

/*** 
 * CACHE AUXILIARIES
 ***/

/* 
 * empty the profile cache hashtable, closes the profiles,
 * used by cms_free
 */
static gboolean 
cms_delete_profile_from_cache(gpointer key,
			      gpointer value,
			      gpointer user_data) 
{   ProfileCacheEntry *entry = (ProfileCacheEntry *)value;
    if (entry->profile->cache_key)
    {
      g_free(entry->profile->cache_key);
    }
    else
      g_warning (_("%s:%d %s() entry->profile->cache_key allready freed or empty"),
                  __FILE__,__LINE__,__func__);
    if (entry->profile->handle)
      cmsCloseProfile(entry->profile->handle);
    else
      g_warning (_("%s:%d %s() entry->profile->handle allready freed or empty"),
                  __FILE__,__LINE__,__func__);
    g_free(entry->profile);
    g_free(entry);

    return TRUE;
}

/* 
 * delete the transform from disc (and therefore also mem) cache
 * used by cms_free and cms_return_transform
 */
static gboolean 
cms_delete_transform_from_cache(gpointer key, 
				gpointer value, 
				gpointer user_data)
{    TransformCacheEntry *cache_entry = (TransformCacheEntry *)value;
     if (cache_entry->transform->handle != NULL) 
     {   cmsDeleteTransform(cache_entry->transform->handle);
     }

     g_free(cache_entry->transform->cache_key);
     g_free(cache_entry->transform);		

     g_free(cache_entry->device_link_file);
     cache_entry->device_link_file = NULL;
     g_free(cache_entry);

     return TRUE;
}


/*** 
 * INITIALIZATION AND TIDYING
 ***/

void cms_init_oyranos();

/* 
 * does initializations
 * loads input and display profile 
 * and calculates standard display transform (input->display)
 * called from app_procs.c:app_init()
 * called from the preferences dialog
 */
void cms_init()
{
    GList *remove_settings = NULL;
    GList *update_settings = NULL;
    cmsHPROFILE test = NULL;

    if(!profile_cache)
      profile_cache = g_hash_table_new(g_str_hash, g_str_equal);
    if(!transform_cache)
      transform_cache = g_hash_table_new(g_str_hash, g_str_equal);         
    if(!profile_info_buffer)
      profile_info_buffer = g_new(CMSProfileInfo, 1);
    profile_info_buffer->manufacturer = NULL;
    profile_info_buffer->description = NULL;
    profile_info_buffer->pcs = NULL;
    profile_info_buffer->color_space_name = NULL;
    profile_info_buffer->color_space_channel_names = NULL;
    profile_info_buffer->device_class_name = NULL;
    profile_info_buffer->long_info = NULL;    

#ifdef HAVE_OY
    if(cms_oyranos)
    {
      cms_init_oyranos();
      return;
    }
#endif

    /* sanity checks of settings, remove failed settings */
    /* suppress lcms errors while checking */
    cmsErrorAction(LCMS_ERROR_IGNORE);

    /* 1. image profile */
    if (cms_default_image_profile_name != NULL)
    {
        test = cmsOpenProfileFromFile(cms_default_image_profile_name, "r");
        if (test == NULL) 
        {   g_warning (_("Cannot open selected ICC assumed image profile: %s"), cms_default_image_profile_name);
            cms_default_image_profile_name = NULL;
            remove_settings = g_list_append(remove_settings, "cms-default-image-profile-name");
        }
        cmsCloseProfile(test);
    }

    /* 2. editing/workspace profile */
    if (cms_workspace_profile_name != NULL)
    {   test = cmsOpenProfileFromFile(cms_workspace_profile_name, "r");
        if (test == NULL) 
        {   g_warning (_("Cannot open selected ICC editing profile: %s"), cms_workspace_profile_name);
            cms_workspace_profile_name = NULL;
            remove_settings = g_list_append(remove_settings, "cms-workspace-profile-name");
        }
	cmsCloseProfile(test);
    }

    /* 3. display profile */
    if (cms_display_profile_name != NULL)
    {   test = cmsOpenProfileFromFile(cms_display_profile_name, "r");
        if (test == NULL) 
        {   g_warning (_("Cannot open selected ICC display profile: %s"), cms_display_profile_name);
            cms_display_profile_name = NULL;
            remove_settings = g_list_append(remove_settings, "cms-display-profile-name");
        }
	/* make sure it is a display profile */
	else if (cmsGetDeviceClass (test) != icSigDisplayClass)
	{   g_warning (_("Selected display profile is not a display profile: %s"), cms_display_profile_name);
	    cms_display_profile_name = NULL;
	    remove_settings = g_list_append(remove_settings, "cms-display-profile-name");
	}
	else 
	{   /* open the display profile */
	    display_profile = cms_get_profile_from_file(cms_display_profile_name);
	}

	cmsCloseProfile(test);
    }
    
    /* 4. proof profile */
    if (cms_default_proof_profile_name != NULL)
    {   test = cmsOpenProfileFromFile(cms_default_proof_profile_name, "r");
        if (test == NULL) 
        {   g_warning (_("Cannot open selected ICC default proof profile: %s"), cms_default_proof_profile_name);
            cms_default_proof_profile_name = NULL;
            remove_settings = g_list_append(remove_settings, "cms-default-proof-profile-name");
        }
        cmsCloseProfile(test);
    }

    if (g_list_length(remove_settings) != 0)
    {   save_gimprc(&update_settings, &remove_settings);
    }
    g_list_free(remove_settings);

    /* show lcms-errors, but don't stop the application */
    cmsErrorAction(LCMS_ERROR_SHOW);    
}

void* my_alloc_func( size_t size )
{ return calloc(1, size);
}

#ifdef HAVE_OY
# if OYRANOS_VERSION > 107
int iccMessageFunc( int code, const oyStruct_s * context, const char * format, ... )
{
  char* text = 0, *pos = 0;
  va_list list;
  const char * type_name = "";
  int id = -1;

  if(code == oyMSG_DBG)
    return 0;


  if(context && oyOBJECT_NONE < context->type_)
  {
    type_name = oyStruct_TypeToText( context );
    id = oyObject_GetId( context->oy_ );
  }

  text = (char*)calloc(sizeof(char), 4096);
  if(!text)
    return 1;
  text[0] = 0;

  switch(code)
  {
    case oyMSG_WARN:
         sprintf( &text[strlen(text)], _("WARNING: "));
         break;
    case oyMSG_ERROR:
         sprintf( &text[strlen(text)], _("!!! ERROR: "));
         break;
  }

  va_start( list, format);
  vsnprintf( &text[strlen(text)], 4096 - strlen(text), format, list);
  va_end  ( list );


  snprintf( &text[strlen(text)], 4096 - strlen(text), "%s[%d] ",type_name,id );

  pos = &text[strlen(text)];
  *pos = '\n';
  pos++;
  *pos = 0;

#ifndef DEBUG
  if(code == oyMSG_WARN)
  {
    int i = 0;
    while(text[i])
      fputc(text[i++], stderr);
    fprintf( stderr, "\n" );
  } else
#endif
  message_box( text, NULL, NULL );

  free( text );

  return 0;
}
/* just a wrapper */
int lcmsMessageFunc( int code, const char * txt )
{
  iccMessageFunc( code, 0, txt );
  return 0;
}
# endif
#endif


void cms_init_oyranos()
{
#ifdef HAVE_OY
    char *test = NULL;     /* profile buffer */
    size_t test_size = 0;  /* profile size */
    char *p_name = NULL;   /* profile name */

    /* messages */
# if OYRANOS_VERSION > 107
    oyMessageFuncSet( iccMessageFunc );
    cmsSetErrorHandler( lcmsMessageFunc );
# endif

    /* we dont touch or save the hand made settings */

    /* 1. assumed image profile */
    {
        p_name = oyGetDefaultProfileName( oyASSUMED_RGB, my_alloc_func );
        if( p_name && !oyCheckProfile( p_name, 0 ) )
          cms_default_image_profile_name = p_name;
        else
        {   g_warning (_("Cannot open selected ICC assumed image profile: %s"),
                       p_name);
            cms_default_image_profile_name = NULL;
          if(p_name) free (p_name);
        }
    }

    /* 2. editing/workspace profile */
    {
        p_name = oyGetDefaultProfileName( oyEDITING_RGB, my_alloc_func );
        if( p_name && !oyCheckProfile( p_name, 0 ) )
          cms_workspace_profile_name = p_name;
        else
        {   g_warning (_("Cannot open selected ICC editing profile: %s"),
                       p_name);
            cms_workspace_profile_name = NULL;
          if(p_name) free (p_name);
        }
    }

    /* 3. display profile */
    {
      const char *display_name = gdk_get_display ();

#if OYRANOS_VERSION > 110
      oyOptions_s * options = 0;
      oyConfig_s * device = 0;
      oyProfile_s * p = 0;
      oyConfig_s * monitors = 0;
      /* Tell about our display environment. */
      int error = oyOptions_SetFromText( &options,
                               "//"OY_TYPE_STD"/config/display_name",
                                     display_name, OY_CREATE_NEW );
      /* request all monitors */
      error = oyOptions_SetFromText( &options,
                               "//"OY_TYPE_STD"/config/command",
                                     "list", OY_CREATE_NEW );
      error = oyDevicesGet( 0, "monitor", options, &monitors );
      /* see how many are included */
      int n = oyConfigs_Count( monitors );
      /* Just select the first one, which CinePaint is considerd to run on. */
      device = oyConfigs_Get( monitors, 0 );
      /* release the list */
      oyConfigs_Release( &monitors );


      /* obtain the net-color spec ICC device profile */
      error = oyOptions_SetFromText( &options,
                               "//"OY_TYPE_STD"/config/x_color_region_target",
                                     "yes", OY_CREATE_NEW );
      error = oyDeviceGetProfile( device, options, &p );
      oyOptions_Release( &options );
      test = oyProfile_GetMem( p, &test_size, 0, my_alloc_func );
      oyProfile_Release( &p );
      oyConfig_Release( &device );
#else
      test = oyGetMonitorProfile( display_name, &test_size, my_alloc_func );
#endif
      printf("%s:%d %s() monitor profile size: %d\n",__FILE__,__LINE__,__func__,
              test_size );

      if (test == NULL || !test_size)
      {
        cmsHPROFILE *p = cmsCreate_sRGBProfile();
        if(cms_manage_by_default)
          message_box (_("Oyranos cannot find display ICC profile.\n  Using sRGB instead."), NULL, NULL);

        cmsAddTag(p, icSigCopyrightTag, "no copyright, use freely");
        _cmsSaveProfileToMem (p, NULL, &test_size);
        test = (char*) calloc (sizeof (char), test_size);
        _cmsSaveProfileToMem (p, test, &test_size);

        /* reopen due to limitations in _cmsSaveProfile/_cmsSaveProfileToMem */
        cmsCloseProfile (p);
        display_profile = cms_get_profile_from_mem(test, test_size);
      }
      /* make sure it is a display profile */
      else
        display_profile = cms_get_profile_from_mem(test, test_size);
    }
    
    /* 4. proof profile */
    {
      p_name = oyGetDefaultProfileName( oyPROFILE_PROOF, my_alloc_func );
      if( p_name && !oyCheckProfile( p_name, 0 ) )
        cms_default_proof_profile_name = p_name;
      else
      {   g_warning (_("Cannot open selected ICC default proof profile: %s"),
                     p_name);
          cms_default_proof_profile_name = NULL;
          if(p_name) free (p_name);
      }
    }

    /* 5. behaviour */
    cms_default_intent = oyGetBehaviour( oyBEHAVIOUR_RENDERING_INTENT );
    cms_default_flags = 0;
    cms_default_flags = oyGetBehaviour( oyBEHAVIOUR_PROOF_SOFT ) ?
                        cms_default_flags | cmsFLAGS_SOFTPROOFING :
                        cms_default_flags & (~cmsFLAGS_SOFTPROOFING) ;

    cms_bpc_by_default = 0;
    cms_bpc_by_default = oyGetBehaviour( oyBEHAVIOUR_RENDERING_BPC ) ?
                        cms_bpc_by_default | cmsFLAGS_WHITEBLACKCOMPENSATION :
                        cms_bpc_by_default & (~cmsFLAGS_WHITEBLACKCOMPENSATION) ;

    cms_default_proof_intent = oyGetBehaviour(
                                         oyBEHAVIOUR_RENDERING_INTENT_PROOF ) ?
                         INTENT_ABSOLUTE_COLORIMETRIC :
                         INTENT_RELATIVE_COLORIMETRIC ;

    switch( oyGetBehaviour( oyBEHAVIOUR_ACTION_UNTAGGED_ASSIGN ) )
    {
      case 0: cms_open_action = CMS_ASSIGN_NONE; break;
      case 1: cms_open_action = CMS_ASSIGN_DEFAULT; break;
      case 2: cms_open_action = CMS_ASSIGN_PROMPT; break;
    }

    switch( oyGetBehaviour( oyBEHAVIOUR_ACTION_OPEN_MISMATCH_RGB ) )
    {
      case 0: cms_mismatch_action = CMS_MISMATCH_KEEP; break;
      case 1: cms_mismatch_action = CMS_MISMATCH_AUTO_CONVERT; break;
      case 2: cms_mismatch_action = CMS_MISMATCH_PROMPT; break;
    }

    /* show lcms-errors, but don't stop the application */
    cmsErrorAction(LCMS_ERROR_SHOW);    
#endif
}

/*
 * tidies up, called from app_procs.c:app_exit_finish()
 */
void cms_free()
{   /* free the caches */    
    g_hash_table_foreach_remove(profile_cache, cms_delete_profile_from_cache, NULL);
    g_hash_table_destroy(profile_cache);
    g_hash_table_foreach_remove(transform_cache, cms_delete_transform_from_cache, NULL);
    g_hash_table_destroy(transform_cache);
    g_free(profile_info_buffer);
}


/***
 * AUXILIARIES FOR EXTRACTING PROFILE INFO
 ***/
const char*
cms_get_pcs_name (cmsHPROFILE hProfile)
{   static char pcs[4];
    switch (cmsGetPCS (hProfile))
    {
        case icSigXYZData: sprintf(pcs,_("XYZ")); break;
        case icSigLabData: sprintf(pcs,_("Lab")); break;
        default: pcs[0] = '\0';
    }

    return pcs;
}

const char* 
cms_get_color_space_name (cmsHPROFILE hProfile)
{   static gchar name[10];

    switch (cmsGetColorSpace (hProfile))
    {
        case icSigXYZData: sprintf(name, _("XYZ")); break;
        case icSigLabData: sprintf(name, _("Lab")); break;
        case icSigLuvData: sprintf(name, _("Luv")); break;
        case icSigYCbCrData: sprintf(name, _("YCbCr")); break;
        case icSigYxyData: sprintf(name, _("Yxy")); break;
        case icSigRgbData: sprintf(name, _("Rgb")); break;
        case icSigGrayData: sprintf(name, _("Gray")); break;
        case icSigHsvData: sprintf(name, _("Hsv")); break;
        case icSigHlsData: sprintf(name, _("Hls")); break;
        case icSigCmykData: sprintf(name, _("Cmyk")); break;
        case icSigCmyData: sprintf(name, _("Cmy")); break;
        case icSig2colorData: sprintf(name, _("2color")); break;
        case icSig3colorData: sprintf(name, _("3color")); break;
        case icSig4colorData: sprintf(name, _("4color")); break;
        case icSig5colorData: sprintf(name, _("5color")); break;
        case icSig6colorData: sprintf(name, _("6color")); break;
        case icSig7colorData: sprintf(name, _("7color")); break;
        case icSig8colorData: sprintf(name, _("8color")); break;
        case icSig9colorData: sprintf(name, _("9color")); break;
        case icSig10colorData: sprintf(name, _("10color")); break;
        case icSig11colorData: sprintf(name, _("11color")); break;
        case icSig12colorData: sprintf(name, _("12color")); break;
        case icSig13colorData: sprintf(name, _("13color")); break;
        case icSig14colorData: sprintf(name, _("14color")); break;
        case icSig15colorData: sprintf(name, _("15color")); break;
        default: name[0] = '\0';
    }

    return name;
}

const char**
cms_get_color_space_channel_names (cmsHPROFILE hProfile)
{   static char** name = 0;
    const char** ret = 0;
    if(!name)
    { int i;
      name=(char**)calloc(sizeof(char**),4);
      for(i = 0; i < 4; ++i)
        name[i] = (char*)calloc(sizeof(char*),36);
    }
    sprintf(name[1], "---");
    sprintf(name[2], "---");
    sprintf(name[3], "---");

    ret = (const char**) name;
    sprintf( name[3],_("Alpha"));

    switch (cmsGetColorSpace (hProfile))
    {
    case icSigXYZData: sprintf( name[0],_("CIE X"));
                       sprintf( name[1],_("CIE Y (Luminance)"));
                       sprintf( name[2],_("CIE Z")); break;
    case icSigLabData: sprintf( name[0],_("CIE *L"));
                       sprintf( name[1],_("CIE *a"));
                       sprintf( name[2],_("CIE *b")); break;
    case icSigLuvData: sprintf( name[0],_("CIE *L"));
                       sprintf( name[1],_("CIE *u"));
                       sprintf( name[2],_("CIE *v")); break;
    case icSigYCbCrData: sprintf( name[0],_("Luminance Y"));
                       sprintf( name[1],_("Colour b"));
                       sprintf( name[2],_("Colour r")); break;
    case icSigYxyData: sprintf( name[0],_("CIE Y (Luminance)"));
                       sprintf( name[1],_("CIE x"));
                       sprintf( name[2],_("CIE y")); break;
    case icSigRgbData: sprintf( name[0],_("Red"));
                       sprintf( name[1],_("Green"));
                       sprintf( name[2],_("Blue")); break;
    case icSigGrayData: sprintf( name[0],_("Black")); break;
    case icSigHsvData: sprintf( name[0],_("Hue"));
                       sprintf( name[1],_("Saturation"));
                       sprintf( name[2],_("Value")); break;
    case icSigHlsData: sprintf( name[0],_("Hue"));
                       sprintf( name[1],_("Lightness"));
                       sprintf( name[2],_("Saturation")); break;
    case icSigCmykData: sprintf( name[0],_("Cyan"));
                       sprintf( name[1],_("Magenta"));
                       sprintf( name[2],_("Yellow"));
                       sprintf( name[3],_("Black")); break;
    case icSigCmyData: sprintf( name[0],_("Cyan"));
                       sprintf( name[1],_("Magenta"));
                       sprintf( name[2],_("Yellow")); break;
    case icSig2colorData: sprintf( name[0],_("1. Colour"));
                       sprintf( name[1],_("2. Colour")); break;
    case icSig3colorData: sprintf( name[0],_("1. Colour"));
                       sprintf( name[1],_("2. Colour"));
                       sprintf( name[2],_("3. Colour")); break;
    case icSig4colorData:  sprintf( name[0],_("1. Colour"));
                       sprintf( name[1],_("2. Colour"));
                       sprintf( name[2],_("3. Colour"));
                       sprintf( name[3],_("4. Colour")); break;
    case icSig5colorData:
    case icSig6colorData:
    case icSig7colorData:
    case icSig8colorData:
    case icSig9colorData:
    case icSig10colorData:
    case icSig11colorData:
    case icSig12colorData:
    case icSig13colorData:
    case icSig14colorData:
    case icSig15colorData:
    default:           sprintf( name[0],_("1. Colour"));
                       sprintf( name[1],_("2. Colour"));
                       sprintf( name[2],_("3. Colour"));
                       sprintf( name[3],_("4. Colour"));
    }

    return ret;
}

const char*
cms_get_device_class_name (cmsHPROFILE hProfile)
{   static char class[15];

    switch (cmsGetDeviceClass (hProfile))
    {
        case icSigInputClass: sprintf(class, _("Input")); break;
        case icSigDisplayClass: sprintf(class, _("Display")); break;
        case icSigOutputClass: sprintf(class, _("Output")); break;
        case icSigLinkClass: sprintf(class, _("Link")); break;
        case icSigAbstractClass: sprintf(class, _("Abstract")); break;
        case icSigColorSpaceClass: sprintf(class, _("ColorSpace")); break;
        case icSigNamedColorClass: sprintf(class, _("NamedColor")); break;
        default: class[0] = '\0';
    }

    return class;
}

icUInt16Number
icUInt16Value (icUInt16Number val)
{ 
#if BYTE_ORDER == LITTLE_ENDIAN
  #define BYTES 2
  #define KORB  4
  unsigned char        *temp  = (unsigned char*) &val;
  static unsigned char  korb[KORB];
  int i;
  int klein = 0,
      gross = BYTES - 1;
  icUInt16Number *erg;

  for (i = 0; i < KORB ; i++ )
    korb[i] = (int) 0;  /* leeren */
    
  for (; klein < BYTES ; klein++ ) {
    korb[klein] = temp[gross--];
  } 
    
  erg = (icUInt16Number*) &korb[0];

  return (icUInt16Number)*erg;
#else
  return (icUInt16Number)val;
#endif
}

const char *
cms_get_profile_keyname (cmsHPROFILE  hProfile, char* mem)
{
    static char text[2048] = {0};
    const char *tmp = NULL;
    icDateTimeNumber *date = (icDateTimeNumber*)&mem[24];

    tmp = cms_get_long_profile_info( hProfile );
    if(tmp)
    {
      sprintf(text, "%s", tmp);
      if(!mem)
        fprintf(stderr, "%s:%d %s() Profile: %s \nno mem argument given.",
                __FILE__,__LINE__,__func__, tmp);
    }
    if(strlen(text) < 2000 && mem) {
        sprintf(&text[strlen(text)], "%d/%d/%d %d:%d:%d",
            icUInt16Value(date->day), icUInt16Value(date->month),
            icUInt16Value(date->year), icUInt16Value(date->hours),
            icUInt16Value(date->minutes), icUInt16Value(date->seconds));
    }
  return text;
}

const char *
cms_get_long_profile_info(cmsHPROFILE  hProfile)
{   static char profile_info[4096];

    gchar    *text;
    const char*tmp = NULL;
    cmsCIEXYZ WhitePt;
    int       first = FALSE,
              min_len = 24,  /* formatting */
              len, i;

    return profile_info;

    text = malloc(256);


#if LCMS_VERSION >= 113 /* formatting */
    if (0 && cmsIsTag(hProfile, icSigCopyrightTag)) {
        len = strlen (cmsTakeCopyright(hProfile)) /*rsr 16*/;
        if (len > min_len)
            min_len = len + 1;
    }
#endif
    profile_info[0] = '\000';
    if (cmsIsTag(hProfile, icSigProfileDescriptionTag)) {
      tmp = cmsTakeProductDesc(hProfile);
      if(tmp && strlen(tmp))
      {
        sprintf (text,_("Description:    "));
        sprintf (&profile_info[strlen(profile_info)],"%s %s", text, tmp);
        if (!first) {
            len = min_len - strlen (profile_info);

            for (i=0; i < len * 2.2; i++) {
                sprintf (&profile_info[strlen(profile_info)]," ");
            }
        }
        sprintf (&profile_info[strlen(profile_info)],"\n");
      }
    }
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      tmp = cmsTakeProductName(hProfile);
      if(tmp && strlen(tmp))
      {
        sprintf (text,_("Product:        "));
        sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text, tmp);
      }
    }
#if LCMS_VERSION >= 112
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      tmp = cmsTakeManufacturer(hProfile);
      if(tmp && strlen(tmp))
      {
        sprintf (text,_("Manufacturer:   "));
        sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text, tmp);
      }
    }
    if (cmsIsTag(hProfile, icSigDeviceModelDescTag)) {
      tmp = cmsTakeModel(hProfile);
      if(tmp && strlen(tmp))
      {
        sprintf (text,_("Model:          "));
        sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text, tmp);
      }
    }
#endif
#if LCMS_VERSION >= 113
    if (0 && cmsIsTag(hProfile, icSigCopyrightTag)) {
      tmp = cmsTakeCopyright(hProfile);
      if(tmp && strlen(tmp))
      {
        sprintf (text,_("Copyright:      "));
        sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text, tmp);
      }
    }
#endif
    sprintf (&profile_info[strlen(profile_info)],"\n");

    cmsTakeMediaWhitePoint (&WhitePt, hProfile);
    _cmsIdentifyWhitePoint (text, &WhitePt);
    sprintf (&profile_info[strlen(profile_info)], "%s\n", text);

    sprintf (text,_("Device Class:   "));
    sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text,
                              cms_get_device_class_name(hProfile));
    sprintf (text,_("Color Space:    "));
    sprintf (&profile_info[strlen(profile_info)],"%s %s\n", text,
                              cms_get_color_space_name(hProfile));
    sprintf (text,_("PCS Space:      "));
    sprintf (&profile_info[strlen(profile_info)],"%s %s", text,
                              cms_get_pcs_name(hProfile));

    free (text);
  
    return profile_info;
}

/***
 * INFORMATION FUNCTIONS
 ***/

/* 
 * compiles a list of filenames (including path) of profiles in the given directory, filters by
 * profile class, class==CMS_ANY_PROFILECLASS gives all profiles, does not recur over dirs
 * returns a list of char * or NULL in case of error
 */
GSList *
cms_read_icc_profile_dir(gchar *path, icProfileClassSignature class)
{   struct stat statbuf;
    DIR *dir;
    struct dirent *entry;
    GSList *return_list = NULL;

    if ((stat (path, &statbuf)) != 0)     
    {   printf("cms_read_icc_profile_dir: Cannot stat icc profile directory %s", path);
        return NULL;
    }
    
    if (!S_ISDIR (statbuf.st_mode))
    {   g_warning ("cms_read_icc_profile_dir: path is not a directory: %s", path);
        return NULL;
    }

    dir = opendir (path);
    if (!dir)
    {   g_warning ("cms_read_icc_profile_dir: cannot open icc profile directory %s", path);
        return NULL;
    }

    /* to avoid error messages for files that are not profiles/corrupt profiles
       stored in the profile directory */
    cmsErrorAction(LCMS_ERROR_IGNORE);

    while ((entry = readdir (dir)))
    {   char *file_name = entry->d_name;
        cmsHPROFILE profile;
        GString *file_path = g_string_new(NULL);
	if ((strcmp (file_name, "..") == 0) || (strcmp (file_name, ".") == 0)) 
	{   continue;
	}

	g_string_sprintf (file_path, "%s/%s", path, file_name);
#       ifdef DEBUG
        printf("%s:%d %s() open profile: %s\n",__FILE__,__LINE__,__func__,file_path->str);
#       endif

	/* Open the file and try to read it using the lcms library
	   - if this fails it was not a valid profile. */
	profile = cmsOpenProfileFromFile (file_path->str, "r");

	/* if it is a valid profile and of the given clas */
	if ((profile != NULL) && ((class == CMS_ANY_PROFILECLASS) || (class == cmsGetDeviceClass (profile))))
	{   return_list = g_slist_append(return_list, (gpointer) file_path->str);
	}
	    
	cmsCloseProfile (profile);
	g_string_free(file_path, FALSE);
    }	
    closedir(dir);

    cmsErrorAction(LCMS_ERROR_SHOW);

    return return_list;
}

/* 
 * compiles a list of filenames (including path) of all profiles in the global preference
 * cms_profile_path filters by
 * profile class, class==CMS_ANY_PROFILECLASS gives all profiles, does not recur over dirs
 * returns a list of char * or NULL in case of error
 */
//#define DEBUG 1
GSList *
cms_read_standard_profile_dirs(icColorSpaceSignature space) 
{   /* the temporary list to hold the names */
    GSList* return_list = NULL;    

    char *path = NULL;
    const char *home = GetDirHome();
    char *directories = NULL;
    GString *file_path = NULL;

    char *remaining_directories = NULL;
    char *token = NULL;
    GSList *sub_list = 0;

#ifdef HAVE_OY
    if(cms_oyranos)
    {
# if OYRANOS_VERSION > 107
      oyPROFILE_e type = oyDEFAULT_PROFILE_START;
      int size, i;
      oyProfile_s * temp_prof = 0;
      oyProfiles_s * iccs = 0;

      iccs = oyProfiles_ForStd( type, 0, 0 );

      size = oyProfiles_Count( iccs );
      for( i = 0; i < size; ++i)
      {
        temp_prof = oyProfiles_Get( iccs, i );
#  ifdef DEBUG
        printf("%d: \"%s\" %s\n", i,
               oyProfile_GetText( temp_prof, oyNAME_DESCRIPTION ),
               oyProfile_GetFileName(temp_prof, 0));
#  endif
        return_list = g_slist_append(return_list, 
                        (gpointer) strdup(oyProfile_GetFileName(temp_prof, 0)));
        oyProfile_Release( &temp_prof );
      }
      oyProfiles_Release( &iccs ); 
# else
      int count = 0, i;
      char profile_space[] = {0,0,0,0,0,0,0,0};
      char** names = /*(const char**)*/ 0;

      icColorSpaceSignature space_host = ntohl(space);

      memcpy( profile_space, &space_host, 4 );
      names = oyProfileListGet( profile_space, &count);

#  ifdef DEBUG
      printf("profiles: %d\n", count);
#  endif
      for(i = 0; i < count; ++i)
      {
#  ifdef DEBUG
        printf("%s profiles: %d %s %s\n", profile_space, i, names[i], oyGetPathFromProfileName(names[i], my_alloc_func));
#  endif
        return_list = g_slist_append(return_list, (gpointer) names[i]);
      }
# endif

      return return_list;
    }
#endif

    directories = g_strdup(cms_profile_path);
    file_path = g_string_new(NULL);
    /* process path by path, paths are separated by : */
    remaining_directories = directories;
    token = xstrsep(&remaining_directories, ":");

    while (token) 
    {   /* replace ~ by home dir */
        if (*token == '~')
	{   path = g_malloc(strlen(home) + strlen(token) + 2);
	    sprintf(path, "%s%s", home, token + 1);
	}
        else
        {   path = strdup(token);
	} 

	sub_list = cms_read_icc_profile_dir(path, space);
	return_list = g_slist_concat(return_list, sub_list);

	g_free(path);
	token = xstrsep(&remaining_directories, ":");
    }

    g_string_free(file_path, TRUE);
    g_free(directories);

    return return_list;
}
#undef DEBUG

/*
 * gets meta information about the profile
 * (only the file_name and description for the moment)
 * returned in a fixed buffer that is overwritten by 
 * subsequent calls
 */
CMSProfileInfo *
cms_get_profile_info(CMSProfile *profile)
{
      const char *dummy[] = {"","","",""};
#if defined(HAVE_OY) && OYRANOS_VERSION > 107
  char ** texts = 0;
  int32_t texts_n = 0;
  oyProfileTag_s * tag = 0;
  oyProfile_s * p = oyProfile_FromMem( profile->size, profile->data, 0, 0 );
#endif

      profile_info_buffer->manufacturer = dummy[0];
      profile_info_buffer->description = dummy[0];
      profile_info_buffer->pcs = dummy[0];
      profile_info_buffer->color_space_name = dummy[0];
      profile_info_buffer->color_space_channel_names = &dummy[0];
      profile_info_buffer->device_class_name = dummy[0];
      profile_info_buffer->long_info = dummy[0];

#if defined(HAVE_OY) && OYRANOS_VERSION > 107
      tag = oyProfile_GetTagById( p, icSigDeviceMfgDescTag );
      texts = oyProfileTag_GetText( tag, &texts_n, "", 0,0,0);
      if(texts_n && texts && texts[0])
        profile_info_buffer->manufacturer = texts[0];
      if(texts) free(texts);
      oyProfileTag_Release( &tag );

      profile_info_buffer->description = strdup( oyProfile_GetText( p,
                                                          oyNAME_DESCRIPTION ));
      oyProfile_Release( &p );
#else
      if(profile)
        profile_info_buffer->manufacturer= cmsTakeManufacturer(profile->handle);

      if(profile)
        profile_info_buffer->description = cmsTakeProductDesc(profile->handle);
#endif

      if(profile)
        profile_info_buffer->pcs = cms_get_pcs_name(profile->handle);

      if(profile)
        profile_info_buffer->color_space_name = cms_get_color_space_name(profile->handle);

      if(profile)
        profile_info_buffer->color_space_channel_names = cms_get_color_space_channel_names(profile->handle);
#     ifdef DEBUG
      /*printf("%s:%d ",__FILE__,__LINE__);
      printf("%s\n",profile_info_buffer->color_space_channel_names[2]);*/
#     endif      

      if(profile)
        profile_info_buffer->device_class_name = cms_get_device_class_name(profile->handle);

      if(profile)
        profile_info_buffer->long_info = cms_get_long_profile_info(profile->handle);

      return profile_info_buffer;
} 


/* returns the lcms format corresponding to this tag */
DWORD
cms_get_lcms_format(Tag tag, CMSProfile *profile)
{   int lcms_data_format = 0;
    
    /* lets be specific to color space */
    /* _cmsLCMScolorSpace is available since lcms-1.13 */
    int color_space = _cmsLCMScolorSpace ( cmsGetColorSpace (profile->handle) );
    int color_channels = _cmsChannelsOf ( cmsGetColorSpace (profile->handle) );
    int precision = 2;
    int extra = 0;
    int num_channels = tag_num_channels(tag);

    extra = num_channels - color_channels;

    if (color_channels > num_channels)
    {
      printf ("%s:%d %s(): Drawable cannot handle all colours.\n",__FILE__,__LINE__,__func__);
#ifndef DEBUG_
      return 0;
#endif
    }

    /* workaround for Lab float images and lcms <= 1.15
     * PT_ANY causes floats to range between 0.0 -> 100.0
     * this is more like sience, than practical colour management
     */
    if(color_space == PT_Lab)
      lcms_data_format |= COLORSPACE_SH(PT_ANY);
    else
      lcms_data_format |= COLORSPACE_SH(color_space);

    lcms_data_format |= CHANNELS_SH(color_channels);

    if(extra > 0)
      lcms_data_format |= EXTRA_SH( extra );


    /* what bitdepth ? */
    switch (tag_precision(tag))
    {
    case PRECISION_U8:
        precision = 1;
	break;
    case PRECISION_U16:
        precision = 2;
	break;
    case PRECISION_FLOAT16:
    case PRECISION_FLOAT:
        precision = 0;
	break;
    default:
        g_warning ("%s:%d %s(): precision not supported", __FILE__,__LINE__,__func__);
        return 0;
    }

    lcms_data_format = (lcms_data_format | BYTES_SH(precision));

#   ifdef DEBUG
    printf ("%s:%d %s(): extra: %d/%d chan: %d/%d %s\n",
            __FILE__,__LINE__,__func__, extra, T_EXTRA(lcms_data_format),
            color_channels, T_CHANNELS(lcms_data_format),
            cms_get_color_space_name(profile->handle));
#   endif

    return lcms_data_format;
}


/***
 * THE CORE: OBTAINING PROFILES AND TRANSFORMS AND TRANSFORMATION
 ***/

/*
 * loading an profile from file into memory
 */
char*
cms_load_profile_to_mem (char* filename, size_t *order_size)
{
    FILE *fp=NULL;
    char *ptr = NULL;
    size_t size = 0;

    if ((fp = fopen(filename, "r")) != NULL) {
        fseek(fp, 0, SEEK_END);
        size = ftell (fp);

        if (size > 0) {
             rewind (fp);
             if(*order_size > 0 && size > *order_size) {
                 ptr = calloc (sizeof (char), *order_size);
                 fread (ptr, sizeof (char), *order_size, fp);
             } else {
                 ptr = calloc (sizeof (char), size);
                 fread (ptr, sizeof (char), size, fp);
                 *order_size = size;
             }
        }
    }
    if(fp) fclose (fp);

    return ptr;
}

/*
 *  obtain a four char string of the ICC colour space, e.g. "Lab"
 */
const char * cms_get_profile_cspace ( CMSProfile         * profile )
{
  return profile->cspace;
}

/*
 * get a handle to the profile in file filename
 * either from the cache or from the file
 * returns NULL on error
 */
CMSProfile *
cms_get_profile_from_file(char *file_name)
{   CMSProfile *return_value;
    size_t size_order = 128;
    char *mem = 0;
    const char *keyname = 0;
    ProfileCacheEntry *cache_entry = 0; 
    char *fullFileName = file_name;
    cmsHPROFILE profile = NULL;

    if( !file_name || strcmp(file_name,_("[none]")) == 0 )
      return NULL;

    /* get profile information */ 
    cmsErrorAction(LCMS_ERROR_IGNORE);     
    profile = cmsOpenProfileFromFile (file_name, "r");
    cmsErrorAction(LCMS_ERROR_SHOW);

#ifdef HAVE_OY
    if ( profile == NULL )
    {
      char *pp_name = NULL;
      /* add path to non pathnamed file names */
      if(file_name && !strchr(file_name, OY_SLASH_C))
      {
        pp_name = oyGetPathFromProfileName( file_name, my_alloc_func );

        if(pp_name && strlen(pp_name))
        {
          fullFileName = (char*) calloc (MAX_PATH, sizeof(char));;
          sprintf(fullFileName, "%s%s%s", pp_name, OY_SLASH, file_name);;
        }
      } else
      /* catch non correct paths */
      if(file_name && strchr(file_name, OY_SLASH_C))
      {
        char *ptr = NULL;

        ptr = strrchr(file_name, OY_SLASH_C);
        ++ptr;
        if(ptr)
        {
          pp_name = oyGetPathFromProfileName( ptr, my_alloc_func );

          if(pp_name && strlen(pp_name))
          {
            fullFileName = (char*) calloc (MAX_PATH, sizeof(char));;
            sprintf(fullFileName, "%s%s%s", pp_name, OY_SLASH, ptr);;
          }
        }
      }

      /* give lcms */
      if(pp_name)
        profile = cmsOpenProfileFromFile (fullFileName, "r");
      if(pp_name) free(pp_name);
    }
#endif

    if (profile == NULL)
    {   g_warning("cms_get_profile_from_file: cannot open profile: %s", fullFileName);
        return NULL;
    }
    mem = cms_load_profile_to_mem (fullFileName, &size_order);
    keyname = cms_get_profile_keyname (profile, mem);
 
    /* check hash table for profile */ 
    cache_entry = g_hash_table_lookup(profile_cache, 
                  (gpointer) cms_get_profile_keyname(profile,mem));
    if (cache_entry != NULL) 
    {   cache_entry->ref_count ++;
        return_value = cache_entry->profile;
        cmsCloseProfile(profile);
        if(mem) free(mem);
        return return_value;
    }

    /* if not in cache, generate new profile */
    return_value = g_new(CMSProfile, 1);
    return_value->cache_key = strdup(cms_get_profile_keyname(profile,mem));
    return_value->handle = profile;
    strcpy( return_value->cspace,
             cms_get_color_space_name( return_value->handle ) );

    /* save an copy of the original icc profile to mem */
    return_value->size = 0;
    return_value->data = cms_load_profile_to_mem (fullFileName,
                                                  &(return_value->size));

    cache_entry = g_new(ProfileCacheEntry, 1);
    cache_entry->ref_count = 1;
    cache_entry->profile = return_value;
    g_hash_table_insert(profile_cache,
			(gpointer) return_value->cache_key,
			(gpointer) cache_entry);
    if(mem) free(mem);

    return return_value;    
}

/* registers a profile that lies in memory with the cms
 * (used for embedded profiles opened by plug-ins)
 * again we check the cache to see whether the profile is already
 * there, if it is, we use the cache handle and free the memory
 * (so never access it after the call to this function!)
 */
CMSProfile *
cms_get_profile_from_mem(void *mem_pointer, DWORD size)
{   CMSProfile *return_value;
    ProfileCacheEntry *cache_entry = 0; 
    
    /* get profile information */ 
    cmsHPROFILE profile = cmsOpenProfileFromMem (mem_pointer, size);
    if (profile == NULL)
    {   g_warning("cms_get_profile_from_mem: cannot open profile");
        return NULL;
    }   


    /* check hash table for profile */ 
    cache_entry = g_hash_table_lookup(profile_cache, 
		       (gpointer) cms_get_profile_keyname(profile,mem_pointer));
    if (cache_entry != NULL) 
    {   cache_entry->ref_count ++;
        return_value = cache_entry->profile;
        cmsCloseProfile(profile);
        /* free the memory */
        g_free(mem_pointer);
        return return_value;
    }

    /* if not in cache, generate new profile */
    return_value = g_new(CMSProfile, 1);
    return_value->cache_key = strdup(cms_get_profile_keyname(profile,mem_pointer));
    return_value->handle = profile;
    strcpy( return_value->cspace,
             cms_get_color_space_name( return_value->handle ) );

    cache_entry = g_new(ProfileCacheEntry, 1);
    cache_entry->ref_count = 1;
    cache_entry->profile = return_value;
    g_hash_table_insert(profile_cache,
			(gpointer) return_value->cache_key,
			(gpointer) cache_entry);

    /* copy profile data */
    return_value->size = size;
    return_value->data = (char*) calloc (sizeof (char), size);
    memcpy (return_value->data, mem_pointer, size);

    return return_value;    
}     
 
/* returns the ICC profile data from an cms internal profile
 * (used for embeddeding profiles by file i/o plug-ins)
 * check for the existance of an profile else return NULL
 */
char *
cms_get_profile_data(CMSProfile *profile, size_t *size)
{   char *return_value = NULL;

    /* get profile data */ 
    if (profile == NULL)
    {   g_print("cms_get_profile_data: no profile in image");
        return NULL;
    }   

    /* not relyable with internal lcms profiles:
       _cmsSaveProfileToMem (profile->handle, &return_value, size); */
    if (profile->size)
    { return_value = (char*) calloc (sizeof (char), profile->size);
      memcpy (return_value, profile->data, profile->size);
      *size = profile->size;
    }

    return return_value;    
}

CMSProfile *
cms_get_virtual_profile( void * mem, size_t size, const char * key_name )
{   CMSProfile *return_value = 0;
    ProfileCacheEntry *cache_entry = 0;
    GString *hash_key = g_string_new(NULL); 
    g_string_sprintf(hash_key, "%s", key_name);

    /* generate new profile */
    return_value = g_new(CMSProfile, 1);
    return_value->handle = cmsOpenProfileFromMem( mem, size );
    return_value->cache_key = hash_key->str;

    return_value->size = size;
    return_value->data = calloc (return_value->size, sizeof (char));
    memcpy( return_value->data, mem, size );

    if (!return_value->data) {
      return_value->size = 0;
      g_warning ("%s:%d %s() profile empty\n",__FILE__,__LINE__,__func__);
      g_string_free(hash_key, FALSE);
      return NULL;
    }

    cache_entry = g_new(ProfileCacheEntry, 1);
    cache_entry->ref_count = 1;
    cache_entry->profile = return_value;
    g_hash_table_insert(profile_cache,
			(gpointer) return_value->cache_key,
			(gpointer) cache_entry);

    return return_value;       
}

/* get a handle to a (lcms built-in) lab profile with given white_point */
CMSProfile *
cms_get_lab_profile(LPcmsCIExyY white_point) 
{
    CMSProfile *return_value = 0;
    ProfileCacheEntry *cache_entry = 0;
    GString *hash_key = g_string_new(NULL); 
    if (white_point) 
    {   g_string_sprintf(hash_key, "###LAB%f%f%f###", white_point->x, white_point->y, white_point->Y);
    }
    else
    {   g_string_sprintf(hash_key, "###LAB###");
    }

    cache_entry = g_hash_table_lookup(profile_cache, (gpointer) hash_key);
    if (cache_entry != NULL) 
    {   cache_entry->ref_count ++;
        return_value = cache_entry->profile;
	g_string_free(hash_key, TRUE);
        return return_value;
    }

    /* if not in cache, generate new profile */
    {
      cmsHPROFILE handle = cmsCreateLabProfile (white_point);
      void * mem = NULL;
      size_t size = 0;

      cmsAddTag(handle, icSigCopyrightTag,
                "no copyright, use freely");

      /* get first the profiles size and then allocate memory */
      _cmsSaveProfileToMem (handle, NULL, &size);
      mem = calloc (size, sizeof (char));
      _cmsSaveProfileToMem (handle, mem, &size);

      return_value = cms_get_virtual_profile( mem, size, hash_key->str);
      cmsCloseProfile (handle);
    }

    return return_value;       
}


/* get a handle to a (lcms built-in) xyz profile */
CMSProfile *
cms_get_xyz_profile() 
{   CMSProfile *return_value = 0;

    ProfileCacheEntry *cache_entry = g_hash_table_lookup(profile_cache, "###XYZ###");
    if (cache_entry != NULL) 
    {   cache_entry->ref_count ++;
        return_value = cache_entry->profile;
        return return_value;
    }

    /* if not in cache, generate new profile */
    {
      cmsHPROFILE handle = cmsCreateXYZProfile ();
      void * mem = NULL;
      size_t size = 0;

      cmsAddTag(handle, icSigCopyrightTag,
                "no copyright, use freely");

      /* get first the profiles size and then allocate memory */
      _cmsSaveProfileToMem (handle, NULL, &size);
      mem = calloc (size, sizeof (char));
      _cmsSaveProfileToMem (handle, mem, &size);

      return_value = cms_get_virtual_profile( mem, size, "###XYZ###");
      cmsCloseProfile (handle);
    }

    return return_value;       
}

/* get a handle to a (lcms built-in) srgb profile */
CMSProfile *
cms_get_srgb_profile() 
{   CMSProfile *return_value;

    ProfileCacheEntry *cache_entry = g_hash_table_lookup(profile_cache, "###SRGB###");
    if (cache_entry != NULL)
    {   cache_entry->ref_count ++;
        return_value = cache_entry->profile;
        return return_value;
    }

    /* if not in cache, generate new profile */
    {
      cmsHPROFILE handle = cmsCreate_sRGBProfile ();
      void * mem = NULL;
      size_t size = 0;

      cmsAddTag(handle, icSigCopyrightTag,
                "no copyright, use freely");

      /* get first the profiles size and then allocate memory */
      _cmsSaveProfileToMem (handle, NULL, &size);
      mem = calloc (size, sizeof (char));
      _cmsSaveProfileToMem (handle, mem, &size);

      return_value = cms_get_virtual_profile( mem, size, "###SRGB###");
      cmsCloseProfile (handle);
    }

    return return_value;
}


/* 
 * makes a 'copy' of the profile handle 
 * (essentially only admits a new user of the handle
 * by increasing the reference counter)
 * returns NULL if profile not found in cache (i.e. not open)
 */ 
CMSProfile *
cms_duplicate_profile(CMSProfile *profile)
{   ProfileCacheEntry *entry = g_hash_table_lookup(profile_cache, (gpointer)profile->cache_key);
    if (entry == NULL)
    {   g_warning("cms_duplicate_profile: profile not found in cache");
        return NULL;
    } 
    
    entry->ref_count++;
    return profile;
}


/* 
 * return a profile to the cache
 */ 
gboolean
cms_return_profile(CMSProfile *profile)
{   ProfileCacheEntry *entry = 0;
   /* search the cache for the profile 
     * decreate ref_counter + possibly close profile 
     */  
    if (profile == NULL)
    {   /*g_warning("cms_return_profile: profile is NULL");*/
        return FALSE;
    } 

    entry = g_hash_table_lookup(profile_cache, (gpointer)profile->cache_key);
    if (entry == NULL)
    {   g_warning("cms_return_profile: profile not found in cache");
        return FALSE;
    } 
    
    entry->ref_count--;
    if (entry->ref_count <= 0)
    {   g_hash_table_remove(profile_cache, (gpointer)profile->cache_key);
	g_free(profile->cache_key);
	cmsCloseProfile(profile->handle);
        if (profile->size)
            g_free(profile->data);
	g_free(profile);
    }

    return TRUE;
}


CMSTransform *
cms_create_transform(cmsHPROFILE *profile_array, int num_profiles,
		  DWORD lcms_input_format, DWORD lcms_output_format,
		  int lcms_intent, DWORD lcms_flags, int proof_intent,
                  Tag src_tag, Tag dest_tag,
                  CMSTransform **expensive_transform,
                  int * set_expensive_transform_b)
{
    cmsHTRANSFORM transform = NULL;
    /* if no cache hit, create transform , do a proofing exeption */
    if( lcms_flags & cmsFLAGS_SOFTPROOFING &&
        num_profiles >= 3)
    {
        /* For more than 3 profiles we have to create a new profile ... */
        if(num_profiles > 3)
        {
            cmsHPROFILE look = 0;
            transform = cmsCreateMultiprofileTransform  (profile_array,
                             num_profiles - 2,
                             lcms_input_format,
                             NOCOLORSPACECHECK(lcms_output_format),
                             lcms_intent,
                             lcms_flags);

            if (!transform) 
            {   g_warning ("%s:%d ICC profile not valid.",__FILE__,__LINE__);
                return NULL;
            }							     
            look = cmsTransform2DeviceLink( transform,
                                                   cmsFLAGS_GUESSDEVICECLASS );
            cmsDeleteTransform (transform);
            /* ... and include the new profile in the proofing transform */
            transform = cmsCreateProofingTransform(look, lcms_input_format,
                                           profile_array[num_profiles-1], lcms_output_format,
                                           profile_array[num_profiles-2],
                                           lcms_intent,
            /* TODO The INTENT_ABSOLUTE_COLORIMETRIC should lead to 
               paper simulation, but does take white point into account.
               Do we want this?
             */
                                           proof_intent,
                                           lcms_flags);
            /* an expensive transform should be cached here */
            if (expensive_transform)
              *set_expensive_transform_b = TRUE;
        } else
        {   /* just a proofing transform */
            transform = cmsCreateProofingTransform(profile_array[0], lcms_input_format,
                                           profile_array[2], lcms_output_format,
                                           profile_array[1],
                                           lcms_intent,
                                           proof_intent,
                                           lcms_flags);
            /* an expensive transform should be cached here */
            if (expensive_transform)
              *set_expensive_transform_b = TRUE;
        }
        if (!transform) 
        {   g_warning ("%s:%d ICC profile not valid?",__FILE__,__LINE__);
            return NULL;
        }							     
    } else
    {   /* no proofing */
        transform = cmsCreateMultiprofileTransform  (profile_array,
						     num_profiles,
						     lcms_input_format,
						     lcms_output_format,
						     lcms_intent,
						     lcms_flags);
        if (!transform) 
        {   g_warning ("%s:%d ICC profile not valid.",__FILE__,__LINE__);
            return NULL;
        }							     
    }

  return transform;
}

/*
 * get a transform based 
 * on input and output format
 * parameters as in lcms's createMultiTransform
 * returns NULL on error
 */
CMSTransform *
cms_get_transform(GSList *profiles,
		  DWORD lcms_input_format, DWORD lcms_output_format,
		  int lcms_intent, DWORD lcms_flags, int proof_intent,
                  Tag src_tag, Tag dest_tag,
                  CMSTransform **expensive_transform)
{   /* turn profiles into an array as needed by lcms + 
       check all profiles are registered with the profile cache + 
       create hash key to check transform cache */
    /* profiles list order:
       1. image profile
       2. look profiles (abstact profiles)
       3. proofing profile , only if proofing is set (cmsFLAGS_SOFTPROOFING)
       4. display profile
    */
    int num_profiles = g_slist_length(profiles);
#ifdef USE_ALLOCA
    cmsHPROFILE *profile_array;
#else
    cmsHPROFILE profile_array[MAX_NUM_PROFILES];
#endif
    GString *hash_key = g_string_new(NULL);
    int i;
    CMSProfile *current_profile;
    GSList *iterator = profiles;
    cmsHTRANSFORM transform;
    TransformCacheEntry *cache_entry = 0;
    int set_expensive_transform_b = 0;

#ifdef USE_ALLOCA
    profile_array = (cmsHPROFILE)alloca(num_profiles*sizeof(cmsHPROFILE));
#endif

    if (num_profiles == 0)
    {   g_warning("cms_get_transform: profile list is empty, cannot create transfrom");
        return NULL;
    }

    /* TODO debugging */
    #ifdef DEBUG_
    printf ("%s:%d %s() Format %d|%d intent %d|%d flags %d color %d|%d\n",
    __FILE__,__LINE__,__func__, (int)lcms_input_format, (int)lcms_output_format,
    lcms_intent, proof_intent, (int)lcms_flags,
    (int)T_COLORSPACE(lcms_input_format), (int)T_COLORSPACE(lcms_output_format));
    #endif

    if (!(int)lcms_output_format
     || !(int)lcms_output_format)
    {   g_warning("%s:%d %s(): lcms_format wrong %d|%d",
                  __FILE__,__LINE__,__func__, (int)lcms_input_format,
                  (int) lcms_output_format);
        return NULL;
    }

    /* collect profiles in a list */
    for (i=0; ((i<num_profiles) && (iterator !=NULL)); i++)
    {   current_profile = (CMSProfile *)iterator->data;
        if (g_hash_table_lookup(profile_cache, (gpointer)current_profile->cache_key) == NULL)
       {   g_warning("cms_get_transform: at least one of the profiles is not registered. Need to register profiles before creating transform");
           return NULL;
       }
       profile_array[i] = current_profile->handle;
       g_string_sprintfa(hash_key,"%p", current_profile->handle);
       iterator = g_slist_next(iterator);
    }
    
    g_string_sprintfa(hash_key,"%d%d%d%d%d%d%d%d",
                      (int)lcms_input_format, (int)lcms_output_format,
                      lcms_intent, (int)lcms_flags, proof_intent, num_profiles,
                      src_tag, dest_tag);

    /* now check the cache */
    cache_entry = g_hash_table_lookup(transform_cache, (gpointer) hash_key->str);

    /* check if provided is equal to CMSTransform */
    if( expensive_transform )
    if( *expensive_transform )
    {
      CMSTransform *exp_form = *expensive_transform;
      
      if (strstr(hash_key->str, exp_form->cache_key))
      {
        g_string_free(hash_key, FALSE);
        return exp_form;
      }
    }

    /* if it was in the disc cache */
    if (cache_entry != NULL && !(lcms_flags & cmsFLAGS_SOFTPROOFING))
    {   cache_entry->ref_count++;

        /* if it is not in memory, load it */
        if (cache_entry->transform->handle == NULL)
        {   cmsHPROFILE device_link = cmsOpenProfileFromFile(cache_entry->device_link_file, "r");
            cache_entry->transform->handle = cmsCreateTransform(device_link,
								lcms_input_format, 
								NULL,
								lcms_output_format,
								lcms_intent,
								lcms_flags);
            cmsCloseProfile(device_link);
        }
        /* else, if we are the first to use it again
           remove it from the list of unused transforms in mem */
        else if (cache_entry->ref_count == 1)
        {   unused_transforms_kept_in_mem = g_list_remove(unused_transforms_kept_in_mem,
                                                      cache_entry);
        }

        g_string_free(hash_key, TRUE);
        return cache_entry->transform;
    }

    transform = cms_create_transform( profile_array, num_profiles,
                                      lcms_input_format, lcms_output_format,
                                      lcms_intent, lcms_flags, proof_intent,
                                      src_tag, dest_tag, expensive_transform,
                                      &set_expensive_transform_b );

    {
        cmsHPROFILE devicelink = NULL;
        char *file_name = NULL;

        /* and store a reference in the cache */
        cache_entry = g_new(TransformCacheEntry, 1);
        cache_entry->ref_count = 1;
        cache_entry->transform = g_new(CMSTransform, 1);
        cache_entry->transform->src_tag = src_tag;
        cache_entry->transform->dest_tag = dest_tag;
        cache_entry->transform->cache_key = hash_key->str;
        cache_entry->transform->handle = transform;
        cache_entry->device_link_file = NULL;

        /* Float Lab hack */
        cache_entry->transform->colourspace_in = cmsGetColorSpace(profile_array[0]);
        cache_entry->transform->lcms_input_format = lcms_input_format;
        cache_entry->transform->lcms_output_format = lcms_output_format;
        #if DEBUG_
        printf ("%s:%d %s() colourspace_in %s\n",__FILE__,__LINE__,__func__, cms_get_color_space_name(profile_array[0]));
        #endif

        /* don't save into cache */
        if (set_expensive_transform_b)
        {
          CMSTransform *exp_form = *expensive_transform;

          if (exp_form)
          {
            if (exp_form->handle != NULL) 
              cmsDeleteTransform(exp_form->handle);

            g_free(exp_form->cache_key);
            g_free(exp_form);
            *expensive_transform = NULL;
          }

          *expensive_transform = cache_entry->transform;
          return cache_entry->transform;
        }

        /* save it to disk */
#if 1
        if(!transform)
          return NULL;
        devicelink = cmsTransform2DeviceLink(transform,0);
        file_name = file_temp_name("icc");
        if(devicelink)
        {
            int error = 0;
            error = _cmsSaveProfile( devicelink, file_name );
            cmsCloseProfile( devicelink );

            /* recreate the transform to not confuse lcms by _cmsSaveProfile */
            cmsDeleteTransform( transform );
            cache_entry->transform->handle = NULL;
            transform = cms_create_transform( profile_array, num_profiles,
                                      lcms_input_format, lcms_output_format,
                                      lcms_intent, lcms_flags, proof_intent,
                                      src_tag, dest_tag, expensive_transform,
                                      &set_expensive_transform_b );
            cache_entry->transform->handle = transform;

#           ifdef DEBUG
            printf("%s:%d %s() %x %x err: %d",__FILE__,__LINE__,__func__,
                   (intptr_t)devicelink,(intptr_t)file_name, error);
            printf(" %s\n", file_name);
#           endif
        }

        /* TODO debugging */
        #ifdef DEBUG
        printf ("%s:%d %s() %s written\n",__FILE__,__LINE__,__func__,file_name);
        #endif
        cache_entry->device_link_file = file_name;

        g_hash_table_insert(transform_cache,
    			(gpointer) hash_key->str,
	    		(gpointer) cache_entry);
#endif
    }
    g_string_free(hash_key, FALSE);

    return cache_entry->transform;
}


/*
 * for convenience: get a transform usable for a canvas
 * (format is determined by this function)
 * returns NULL on error
 */
CMSTransform *
cms_get_canvas_transform(GSList *profiles, Tag src_tag, Tag dest_tag, 
			 int lcms_intent, DWORD lcms_flags, int proof_intent,
             CMSTransform **expensive_transform)
{
    DWORD lcms_format_in, lcms_format_out;
    CMSTransform *transform = NULL;

    /* take the Input profile */
    lcms_format_in  = cms_get_lcms_format(src_tag,(CMSProfile *)profiles->data);
    lcms_format_out = cms_get_lcms_format(dest_tag, (CMSProfile *)
                                             (g_slist_last(profiles)->data));

    if (!lcms_format_in || !lcms_format_out)
    {   g_warning("%s:%d %s(): wrong lcms_format %d|%d",
                  __FILE__,__LINE__,__func__,
                  (int) lcms_format_in, (int)lcms_format_out);
        return NULL;
    } 

    #ifdef DEBUG_
    printf ("%s:%d %s() Format %d|%d intent %d|%d flags %d color %d\n",
    __FILE__,__LINE__,__func__,    lcms_format_in, lcms_format_out,
    lcms_intent, proof_intent, lcms_flags, T_COLORSPACE(lcms_format_in));
    #endif

    transform = cms_get_transform(profiles, lcms_format_in, lcms_format_out,
                                  lcms_intent, lcms_flags, proof_intent,
                                  src_tag, dest_tag, expensive_transform);

    return transform;
}


gboolean 
cms_return_transform(CMSTransform *transform)
{
    TransformCacheEntry *cache_entry = NULL;

    if (transform == NULL)
        return FALSE;

    cache_entry = g_hash_table_lookup(transform_cache, transform->cache_key);
    
    if (cache_entry == NULL)
        return FALSE;

    cache_entry->ref_count--; 

    /* this transform is not used anymore, but we keep in memory while there is space for efficiency */
    if (cache_entry->ref_count <= 0)      
    {   unused_transforms_kept_in_mem = g_list_append(unused_transforms_kept_in_mem, cache_entry);	
        /* if the cache is full, kick out the oldest one */
        if (g_list_length(unused_transforms_kept_in_mem) > MAX_UNUSED_TRANSFORMS_IN_MEM)
	{   TransformCacheEntry *old_cache_entry = (TransformCacheEntry *)unused_transforms_kept_in_mem->data;
	    cmsDeleteTransform(old_cache_entry->transform->handle);
	    old_cache_entry->transform->handle = NULL;
	    unused_transforms_kept_in_mem = g_list_remove(unused_transforms_kept_in_mem, old_cache_entry);
	}
    }	

    return TRUE;
}


/*
 * get the display profile, if set
 */
CMSProfile *
cms_get_display_profile()
{   return display_profile;
}

/*
 * set the display profile, return value indicates success
 */
gboolean 
cms_set_display_profile(CMSProfile *profile) 
{    /* profile has to be registered */
     if (g_hash_table_lookup(profile_cache, (gpointer)profile->cache_key) == NULL) 
     {   g_warning("cms_set_display_profile: profile is not registered. Cannot be set as display profile.");
	 return FALSE;
     }

     display_profile = profile;     
     gdisplays_update_full();
     gdisplays_flush();

     return TRUE;
}


/* transform a pixel buffer, src and dest have to be same precision */
void 
cms_transform_buffer (CMSTransform *transform, void *src_data, void *dest_data,
			  int num_pixels, Precision precision) 
{
    TransformFunc transform_func;
    double * dest_data_ = NULL;
    int len = 0;
    int bytes = 2;

    if (!transform || !transform->handle)
    {   g_warning ("%s:%d %s(): transform empty",__FILE__,__LINE__,__func__);
        return;
    }
    if (!src_data || !dest_data)
    {   g_warning ("%s:%d %s(): buffer not allocated",__FILE__,__LINE__,__func__);
        return;
    }

    switch (precision)
    {

    case PRECISION_U8:
         bytes = 1;
    case PRECISION_U16:  
        transform_func = cms_transform_uint;
        break;
    case PRECISION_FLOAT:
         bytes = 4;
    case PRECISION_FLOAT16:
        transform_func = cms_transform_float;
        break;
    default:
        g_warning ("%s:%d %s(): precision not supported", __FILE__,__LINE__,__func__);
        return;
    }

    if(src_data == dest_data &&
       tag_format(transform->dest_tag) == FORMAT_GRAY)
    {
      len = bytes * 4 * num_pixels;
      dest_data_ = malloc( len );
      dest_data = dest_data_;
    }

    (*transform_func) (transform, src_data, dest_data, num_pixels);

    if(dest_data_)
    {
      memcpy( src_data, dest_data_, len );
      free( dest_data_ ); dest_data_ = NULL;
      dest_data = src_data;
    }

        #ifdef DEBUG_
        printf("%s:%d diff = %d\n",__FILE__,__LINE__,num_pixels);
        #endif
}

/* transforms a pixel area, src and dest have to be same precision */
void 
cms_transform_area(CMSTransform *transform, PixelArea *src_area, PixelArea *dest_area) 
{   TransformFunc transform_func;
    Tag src_tag = pixelarea_tag(src_area); 
    Tag dest_tag = pixelarea_tag(dest_area); 
    PixelRow src_row_buffer, dest_row_buffer;
    guint h;
    void *src_data, *dest_data;
    guint num_pixels;
    void *pag;
    
    if (tag_precision(src_tag) != tag_precision(dest_tag)) 
    {
      int i; i = 0;
      /*g_warning("cms_transform: src and dest area have to have same precision");*/
    }

    if (!transform || !transform->handle)
    {   g_warning ("%s:%d %s(): transform empty",__FILE__,__LINE__,__func__);
        return;
    }

    switch (tag_precision(src_tag))
    {

    case PRECISION_U8:
    case PRECISION_U16:  
        transform_func = cms_transform_uint;
        break;
    case PRECISION_FLOAT16:
    case PRECISION_FLOAT:
        transform_func = cms_transform_float;
	break;
    default:
      g_warning ("%s:%d %s(): precision not supported", __FILE__,__LINE__,__func__);
      return;
    }


    for (pag = pixelarea_register (2, src_area, dest_area);
	 pag != NULL;
	 pag = pixelarea_process (pag))
    {   h = pixelarea_height(src_area);	  
        num_pixels = pixelarea_width(dest_area);  

	while (h--)
        {   pixelarea_getdata(src_area, &src_row_buffer, h);	    
	    pixelarea_getdata(dest_area, &dest_row_buffer, h);
            src_data = pixelrow_data(&src_row_buffer);            
            dest_data = pixelrow_data(&dest_row_buffer);            
            if (!dest_data || !src_data)
                g_warning ("%s:%d %s() buffer failed\n",
                           __FILE__,__LINE__,__func__);
            (*transform_func) (transform, src_data, dest_data, num_pixels);
	}
    } 
}

#if 0
void
cms_transform_uint_extra(CMSTransform *transform, void *src_data, void *dest_data, int num_pixels)
{
    int cchan_in = T_CHANNELS( transform->lcms_input_format );
    int cchan_out = T_CHANNELS( transform->lcms_output_format );
    int chan_in = tag_num_channels( transform->src_tag );
    int chan_out = tag_num_channels( transform->dest_tag );
    int extra_in = chan_in - cchan_in;
    int extra_out = chan_out - cchan_out;
    Precision prec_in = tag_precision( transform->src_tag );
    Precision prec_out = tag_precision( transform->dest_tag );
    int dest_alpha = tag_alpha( transform->dest_tag ) == ALPHA_YES ? 1:0;
    int i, j;
    ShortsFloat u;

    if(chan_out > cchan_out)
    for(i = 0 ; i < num_pixels; ++i)
    {
      int pos_in = i * chan_in + cchan_in;
      int pos_out = i * chan_out + cchan_out;

      switch (prec_out)
      {
        case PRECISION_U8:
        {
          guint8 *dest = (guint8 *)dest_data;
          float dest_white = 255;

          for(j = 0; j < extra_in && j < extra_out; ++j)
            switch( prec_in )
            {
              case PRECISION_U8:
              {
                guint8 *src =  (guint8 *)src_data;
                dest[pos_out + j] = src[pos_in + j];
              }
              break;
              case PRECISION_U16:
              {
                guint16 * src = (guint16*)src_data;
                dest[pos_out + j] = src[pos_in + j]/256;
              }  
              break;
              case PRECISION_FLOAT16:
              {
                guint16 * src = (guint16*)src_data;
                dest[pos_out + j] = FLT(src[pos_in + j], u) * dest_white;
              }  
              break;
              case PRECISION_FLOAT:
              {
                float * src = (float*)src_data;
                dest[pos_out + j] = src[pos_in + j] * dest_white;
              }  
              break;
            }
          for(j = extra_in; j < extra_out; ++j)
            dest[pos_out + j] = dest_white;
        }
        break;
        case PRECISION_U16:
        {
          guint16 *dest = (guint16 *)dest_data;
          float dest_white = 65535;

          for(j = 0; j < extra_in && j < extra_out; ++j)
            switch( prec_in )
            {
              case PRECISION_U8:
              {
                guint8 *src =  (guint8 *)src_data;
                dest[pos_out + j] = src[pos_in + j]*257;
              }
              break;
              case PRECISION_U16:
              {
                guint16 * src = (guint16*)src_data;
                dest[pos_out + j] = src[pos_in + j];
              }  
              break;
              case PRECISION_FLOAT16:
              {
                guint16 * src = (guint16*)src_data;
                dest[pos_out + j] = FLT(src[pos_in + j], u) * dest_white;
              }  
              break;
              case PRECISION_FLOAT:
              {
                float * src = (float*)src_data;
                dest[pos_out + j] = src[pos_in + j] * dest_white;
              }  
              break;
            }

          for(j = chan_in; j < chan_out; ++j)
            if(dest_alpha && j+1 == chan_out)
              dest[i*chan_out + j] = dest_white;
            else
              dest[i*chan_out + j] = dest[i*chan_out + j-1];
        }
        break;
      }
    }
}
#endif

void
cms_transform_uint(CMSTransform *transform, void *src_data, void *dest_data, int num_pixels) 
{
    int cchan_in = T_CHANNELS( transform->lcms_input_format );
    int cchan_out = T_CHANNELS( transform->lcms_output_format );
    int alpha = tag_alpha( transform->dest_tag ) == ALPHA_YES ? 1:0;
    int chan_in = tag_num_channels( transform->src_tag );
    int chan_out = tag_num_channels( transform->dest_tag );
    int extra_in = chan_in - cchan_in;
    int extra_out = chan_out - cchan_out;
    Precision prec_in = tag_precision( transform->src_tag );
    Precision prec_out = tag_precision( transform->dest_tag );
    int dest_alpha = tag_alpha( transform->dest_tag ) == ALPHA_YES ? 1:0;
    int i, j;
    ShortsFloat u;

    /* easy, no previous conversion, lcms does it all */
    if (!transform || !transform->handle)
        g_warning ("%s:%d %s() transform not allocated\n",
                   __FILE__,__LINE__,__func__);
    if (!src_data || !dest_data)
        g_warning ("%s:%d %s() array not allocated\n",
                   __FILE__,__LINE__,__func__);

    /* easy, no previous conversion, lcms does it all */
    cmsDoTransform(transform->handle,src_data,dest_data,num_pixels);

#   if 0
    cms_transform_uint_extra( transform, src_data, dest_data, num_pixels );
#   else
    for(i = 0 ; i < num_pixels; ++i)
    {
      switch (tag_precision( transform->dest_tag ))
      {
        case PRECISION_U8:
          {
            guint8 *dest = (guint8 *)dest_data;
            for(j = cchan_out; j < cchan_in; ++j)
              if(alpha && j+1 == chan_out)
                dest[i*chan_out + j] = 255;
              else
                dest[i*chan_out + j] = dest[i*chan_out + j-1];
          }
          break;
        case PRECISION_U16:
          {
            guint16 *dest = (guint16 *)dest_data;
            for(j = cchan_out; j < cchan_in; ++j)
              if(alpha && j+1 == chan_out)
                dest[i*chan_out + j] = 65535;
              else
                dest[i*chan_out + j] = dest[i*chan_out + j-1];
          }
          break;
      }
    }
#   endif
}

void
cms_transform_float(CMSTransform *transform, void *src_data, void *dest_data, int num_pixels) 
{   /* need to convert data to double for lcms's convenience */
    int cchan_in = T_CHANNELS( transform->lcms_input_format );
    int cchan_out = T_CHANNELS( transform->lcms_output_format );
    int chan_in = tag_num_channels( transform->src_tag );
    int chan_out = tag_num_channels( transform->dest_tag );
    int chan = chan_out;
    int alpha = tag_alpha( transform->dest_tag ) == ALPHA_YES ? 1:0;
    /*int extra_in = chan_in - cchan_in;
    int extra_out = chan_out - cchan_out; */
    int src_alpha = tag_alpha( transform->src_tag ) == ALPHA_YES ? 1:0;
    int dest_alpha = tag_alpha( transform->dest_tag ) == ALPHA_YES ? 1:0;
    int i, j, pix;
    float *src_fbuffer = (float *)src_data;
    float *dest_fbuffer = (float *)dest_data;
    guint16 *src_hbuffer = (guint16 *)src_data;
    guint16 *dest_hbuffer = (guint16 *)dest_data;
    double *src_dbuffer = NULL,
           *dest_dbuffer = NULL;
    void  * src_buffer = NULL,
          * dest_buffer = NULL;
    double* dbuffer = NULL;
    ShortsFloat u;
    Precision dest_p = tag_precision( transform->dest_tag );

    if (!transform || !transform->handle)
        g_warning ("%s:%d %s() transform not allocated\n",
                   __FILE__,__LINE__,__func__);
    if (!src_data || !dest_data)
        g_warning ("%s:%d %s() array not allocated\n",
                   __FILE__,__LINE__,__func__);

#   if 1
    src_dbuffer = malloc(sizeof(double) * num_pixels * 4);
    if( dest_p == PRECISION_FLOAT16 ||
        dest_p == PRECISION_FLOAT)
    {
      dest_dbuffer = malloc(sizeof(double) * num_pixels * 4);
      dest_buffer = dest_dbuffer;
    } else
      dest_buffer = dest_data;
    src_buffer = src_dbuffer;
#   else
    dbuffer = malloc(sizeof(double) * num_pixels * 4);
#   endif

#   ifdef DEBUG_
    printf ("%s:%d %s()  colourspace%d extra:%d channels:%d lcms_bytes%d \n", __FILE__,__LINE__,__func__, T_COLORSPACE(transform->lcms_input_format), T_EXTRA(transform->lcms_input_format), T_CHANNELS(transform->lcms_input_format), T_BYTES(transform->lcms_input_format) );
#   endif

#if 1
    switch (tag_precision( transform->src_tag ))
    {
      case PRECISION_FLOAT16:
        for (i=0; i < num_pixels * chan_in; ++i) 
        {
          if(T_COLORSPACE(transform->lcms_input_format) == PT_CMYK)
            src_dbuffer[i] = (double) FLT( src_hbuffer[i], u ) * 100.;
          else
            src_dbuffer[i] = (double) FLT( src_hbuffer[i], u );
        }
        break;
      case PRECISION_FLOAT:
# if 0
        // Lab floating point makes no sense
        if (transform->colourspace_in == icSigLabData)
        {
          for (pix=0; pix < num_pixels; ++pix)
          { i = pix * chan_in;
           /* The 100. multiplicator comes from PT_ANY in cms_get_lcms_format().
             */
            src_dbuffer[i+0] = (double) src_fbuffer[i+0]*100.;
            src_dbuffer[i+1] = (double) src_fbuffer[i+1]*100.;
            src_dbuffer[i+2] = (double) src_fbuffer[i+2]*100.;
            if(src_alpha)
              src_dbuffer[i+3] = (double)  src_fbuffer[i+3];
          }
        } else {
# endif
          for (i=0; i < num_pixels * chan_in; ++i) 
# if 0
            if(transform->colourspace_in == icSigCmykData)
              src_dbuffer[i]=(double)src_fbuffer[i] * 100.;
            else
# endif
              src_dbuffer[i]=(double)src_fbuffer[i];
/*        } */
        break;
      default:
        dest_buffer = dest_data;
        return;
    }

    cmsDoTransform( transform->handle, src_buffer, dest_buffer, num_pixels );

    /* and convert back */
    switch (tag_precision( transform->dest_tag ))
    {
      case PRECISION_FLOAT16:
          for (i=0; i < num_pixels; ++i) 
          {
            for(j = 0; j < cchan_out; ++j)
              /* copy colour channels */
              if(T_COLORSPACE(transform->lcms_output_format) == PT_CMYK)
                dest_hbuffer[i*chan_out + j] = (double) FLT16( dest_dbuffer[i*chan_out + j] / 100., u );
              else
                dest_hbuffer[i*chan_out + j] = (double) FLT16( dest_dbuffer[i*chan_out + j], u );
            for(j = chan_in; j < chan_out; ++j)
              if(dest_alpha && j+1 == chan_out)
                /* set CinePaints alpha channel to opace */
                dest_hbuffer[i*chan_out + j] = ONE_FLOAT16;
              else
                /* refill with previous values */
                dest_hbuffer[i*chan_out + j] = dest_hbuffer[i*chan_out + j-1];
              /* ignore other untouched channels */
          }
        break;
      case PRECISION_FLOAT:
          for (i=0; i < num_pixels; ++i) 
          {
            for(j = 0; j < cchan_out; ++j)
#             if 0
              if(T_COLORSPACE(transform->lcms_output_format) == PT_CMYK)
                dest_fbuffer[i*chan_out + j] = dest_dbuffer[i*chan_out + j]/100.0;
              else
#             endif
                dest_fbuffer[i*chan_out + j] = dest_dbuffer[i*chan_out + j];
            for(j = chan_in; j < chan_out; ++j)
              if(dest_alpha && j+1 == chan_out)
                dest_fbuffer[i*chan_out + j] = 1.0;
              else
                dest_fbuffer[i*chan_out + j] = dest_fbuffer[i*chan_out + j-1];
          }
        break;
      default:
        break;
    }

    if(dest_dbuffer) g_free(dest_dbuffer);
    if(src_dbuffer) g_free(src_dbuffer);
# else

    switch (tag_precision( transform->dest_tag ))
    {
      case PRECISION_FLOAT16:
        for (i=0; i < num_pixels * chan; ++i) 
        {
          if(T_COLORSPACE(transform->lcms_input_format) == PT_CMYK)
            dbuffer[i] = (double) FLT( src_hbuffer[i], u ) * 100.;
          else
            dbuffer[i] = (double) FLT( src_hbuffer[i], u );
        }
        break;
      case PRECISION_FLOAT:
        if (transform->colourspace_in == icSigLabData)
        {
          for (pix=0; pix < num_pixels; ++pix)
          { i = pix * chan;
           /* The 100. multiplicator comes from PT_ANY in cms_get_lcms_format().
             */
            dbuffer[i+0] = (double) src_fbuffer[i+0]*100.;
            dbuffer[i+1] = (double) src_fbuffer[i+1]*100.;
            dbuffer[i+2] = (double) src_fbuffer[i+2]*100.;
            if(alpha)
              dbuffer[i+3] = (double)  src_fbuffer[i+3];
          }
        } else {
          for (i=0; i < num_pixels * chan; ++i) 
            if(transform->colourspace_in == icSigCmykData)
              dbuffer[i]=(double)src_fbuffer[i] * 100.;
            else
              dbuffer[i]=(double)src_fbuffer[i];
        }
        break;
      default:
        g_warning ("cms_transform: precision not supported");
        return;
    }

    cmsDoTransform(transform->handle,dbuffer,dbuffer,num_pixels);

    /* and convert back */
    switch (tag_precision( transform->dest_tag ))
    {
      case PRECISION_FLOAT16:
          for (i=0; i < num_pixels; ++i) 
          {
            for(j = 0; j < cchan_out; ++j)
              /* copy colour channels */
              if(T_COLORSPACE(transform->lcms_output_format) == PT_CMYK)
                dest_hbuffer[i*chan + j] = (double) FLT16( dbuffer[i*chan + j] / 100., u );
              else
                dest_hbuffer[i*chan + j] = (double) FLT16( dbuffer[i*chan + j], u );
            for(j = cchan_out; j < cchan_in; ++j)
              if(alpha && j+1 == chan)
                /* set CinePaints alpha channel to opace */
                dest_hbuffer[i*chan + j] = ONE_FLOAT16;
              else
                /* refill with previous values */
                dest_hbuffer[i*chan + j] = dest_hbuffer[i*chan + j-1];
              /* ignore other untouched channels */
          }
        break;
      case PRECISION_FLOAT:
          for (i=0; i < num_pixels; ++i) 
          {
            for(j = 0; j < cchan_out; ++j)
              if(T_COLORSPACE(transform->lcms_output_format) == PT_CMYK)
                dest_fbuffer[i*chan + j] = dbuffer[i*chan + j]/100.0;
              else
                dest_fbuffer[i*chan + j] = dbuffer[i*chan + j];
            for(j = cchan_out; j < cchan_in; ++j)
              if(alpha && j+1 == chan)
                dest_fbuffer[i*chan + j] = 1.0;
              else
                dest_fbuffer[i*chan + j] = dest_fbuffer[i*chan + j-1];
          }
        break;
    }

    g_free(dbuffer);
# endif
}


/* workaround see declaration above - beku */
#if (LCMS_VERSION <= 112)
int _cmsLCMScolorSpace(icColorSpaceSignature ProfileSpace)
{    
       switch (ProfileSpace) {

       case icSigGrayData: return  PT_GRAY;
       case icSigRgbData:  return  PT_RGB;
       case icSigCmyData:  return  PT_CMY;
       case icSigCmykData: return  PT_CMYK;
       case icSigYCbCrData:return  PT_YCbCr;
       case icSigLuvData:  return  PT_YUV;
       case icSigXYZData:  return  PT_XYZ;
       case icSigLabData:  return  PT_Lab;
       case icSigLuvKData: return  PT_YUVK;
       case icSigHsvData:  return  PT_HSV;
       case icSigHlsData:  return  PT_HLS;
       case icSigYxyData:  return  PT_Yxy;

       case icSig6colorData:
       case icSigHexachromeData: return PT_HiFi;

       default:  return icMaxEnumData;
       }
}
#endif

void
cms_gimage_check_profile(GImage *image, CMSProfileType type)
{
  if(image)
  {
    if(type == ICC_IMAGE_PROFILE)
    {
      {
        Tag t = gimage_tag( image );
        switch(tag_format(t))
        {
          case FORMAT_RGB:
#ifdef HAVE_OY
               {
                 char *p_name = oyGetDefaultProfileName( oyASSUMED_RGB,
                                                         my_alloc_func ) ;
                 if(p_name) {
                   gimage_set_cms_profile( image, 
                                           cms_get_profile_from_file( p_name ));
                   free(p_name);
                 }
               }
#else
               if(cms_workspace_profile_name)
                 gimage_set_cms_profile( image, 
                         cms_get_profile_from_file(cms_workspace_profile_name));
               else
                 gimage_set_cms_profile(image, cms_get_srgb_profile());
#endif
               break;
          case FORMAT_GRAY:
#ifdef HAVE_OY
# if OYRANOS_VERSION > 107
               {
                 char *p_name = oyGetDefaultProfileName( oyASSUMED_GRAY,
                                                         my_alloc_func ) ;
                 gimage_set_cms_profile( image,
                                         cms_get_profile_from_file( p_name ));
                 free(p_name);
               }
# endif
#endif
               break;
          case FORMAT_NONE:
          case FORMAT_INDEXED:
               break;
        }
      }
    } else if(type == ICC_PROOF_PROFILE) {
#ifdef HAVE_OY
      char *p_name = oyGetDefaultProfileName( oyPROFILE_PROOF, my_alloc_func ) ;
      if(p_name) {
        gimage_set_cms_proof_profile( image,
                                      cms_get_profile_from_file( p_name ));
        free(p_name);
      }
#else
      if( cms_default_proof_profile_name )
        gimage_set_cms_proof_profile( image,
                   cms_get_profile_from_file( cms_default_proof_profile_name ));
#endif
    }
  }
}

void
cms_gimage_open (GImage         * gimage)
{
  if(gimage)
  {
    CMSProfileType type = ICC_IMAGE_PROFILE;

#ifdef HAVE_OY
      char * p_name = NULL;
      int action_open_mismatch = oyGetBehaviour( oyBEHAVIOUR_ACTION_OPEN_MISMATCH_RGB );

      Tag t = gimage_tag( gimage );
      switch(tag_format(t))
        {
          case FORMAT_RGB:
               p_name = oyGetDefaultProfileName( oyEDITING_RGB, my_alloc_func );
               break;
          case FORMAT_GRAY:
# if OYRANOS_VERSION > 107
               p_name = oyGetDefaultProfileName( oyASSUMED_GRAY, my_alloc_func);
# endif
               break;
          case FORMAT_NONE:
          case FORMAT_INDEXED:
               break;
        }


    if( !gimage_get_cms_profile( gimage ) )
    {
      if ((cms_open_action == CMS_ASSIGN_DEFAULT) &&
         (cms_default_image_profile_name != NULL)) 
        { cms_gimage_check_profile(gimage, type);
        }
        else if (cms_open_action == CMS_ASSIGN_PROMPT)
        { cms_open_assign_dialog(gimage);
        }
    }

#endif

#ifdef HAVE_OY
      if ((p_name != NULL) && (gimage_get_cms_profile(gimage) != NULL))
      {
        CMSProfile *editing_profile = cms_get_profile_from_file( p_name );
        if (gimage_get_cms_profile(gimage) != editing_profile &&
	    (action_open_mismatch == oyYES  || 
	     (action_open_mismatch == oyASK &&
              cms_convert_on_open_prompt(gimage) == TRUE)))
	{
          int intent = oyGetBehaviour( oyBEHAVIOUR_RENDERING_INTENT );
          int bpc = oyGetBehaviour( oyBEHAVIOUR_RENDERING_BPC );
          convert_image_colorspace(gimage, editing_profile, intent,
                                   bpc ? cmsFLAGS_WHITEBLACKCOMPENSATION : 0 );
	}
      }
#else        

      /* if a workspace profile is given and it's not equal the 
         image profile, check whether to convert */
      if ((cms_workspace_profile_name != NULL) && (gimage_get_cms_profile(gimage) != NULL))
      {   CMSProfile *workspace_profile = cms_get_profile_from_file(cms_workspace_profile_name);
          if (gimage_get_cms_profile(gimage) != workspace_profile &&
               cms_mismatch_action == CMS_MISMATCH_AUTO_CONVERT  || 
               (cms_mismatch_action == CMS_MISMATCH_PROMPT &&
                cms_convert_on_open_prompt(gimage) == TRUE))
          {   convert_image_colorspace(gimage, workspace_profile,
                       cms_default_intent,
                       cms_default_flags & cmsFLAGS_WHITEBLACKCOMPENSATION ?
                       cmsFLAGS_WHITEBLACKCOMPENSATION : 0 );
          }
      } 

#endif

    if(p_name)
      free(p_name); p_name = NULL;

    if(!gimage_get_cms_proof_profile( gimage ))
      cms_gimage_check_profile( gimage, ICC_PROOF_PROFILE );
  }
}


/*** 
 * INTERFACE - DIALOGS. SHOULD GO IN A DIFFERENT FILE
 ***/

/* 
 * COMMON INTERFACE ELEMENTS 
 * used in various dialogs
 */

/* private prototypes */
static GtkWidget *cms_profile_menu_new(GtkTable *table, guint left, guint top,
				       gboolean can_select_none,
                                       const char* prefered_name );
static GtkWidget *cms_intent_menu_new(GtkTable *table, guint left, guint top);
static void cms_ok_cancel_buttons_new(GtkBox *action_area,
				       GtkSignalFunc ok_callback, 
				       GtkSignalFunc cancel_callback, 
				       gpointer data);


/* private functions */
static GtkWidget *cms_profile_menu_new(GtkTable *table, guint left, guint top, 
				       gboolean can_select_none,
                                       const char* prefered_name)
{   GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *optionmenu;

    GSList *profile_file_names = 0;
    GSList *iterator = 0;
    gchar *current_long_fname;
    CMSProfile *current_profile;
    CMSProfileInfo *current_profile_info;      
    int pos = can_select_none ? 1 : 0, select_pos = -1;
    gchar * label_text = 0, * temp_label_text = 0;
    label_text = temp_label_text = calloc(1024,sizeof(char));

    menu = gtk_menu_new (); 

    profile_file_names = cms_read_standard_profile_dirs(0);

    iterator = profile_file_names;

    if (can_select_none)
    {   menuitem = gtk_menu_item_new_with_label(_("[none]"));
        gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_object_set_data(GTK_OBJECT(menuitem), "value", NULL);
    }

    while (iterator != NULL)
    {
        const char * file_name;
        current_long_fname = iterator->data;
        file_name = current_long_fname;
        if(strrchr(file_name,'/'))
          file_name = strrchr(file_name,'/') + 1;
        current_profile = cms_get_profile_from_file(current_long_fname);
	current_profile_info = cms_get_profile_info(current_profile);
        if(prefered_name && strlen(prefered_name) &&
           strcmp( file_name, prefered_name ) == 0 )
          select_pos = pos;
        sprintf(label_text, "%s (%s)", current_profile_info->description, current_long_fname);
        menuitem = gtk_menu_item_new_with_label (label_text);
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)current_long_fname, g_free);

	cms_return_profile(current_profile);

	iterator = g_slist_next(iterator);
        ++pos;
    }
    if (profile_file_names != NULL) 
    {   g_slist_free(profile_file_names);
    }
    if(temp_label_text) free(temp_label_text);

    if(select_pos >= 0)
      gtk_menu_set_active (GTK_MENU (menu), select_pos);

    optionmenu = gtk_option_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
    gtk_table_attach (GTK_TABLE (table), optionmenu, left, left+1, top, top+1,
	              GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0); 
    return menu;
}

static GtkWidget *cms_intent_menu_new(GtkTable *table, guint left, guint top)
{   GtkWidget *menu = NULL; 
    GtkWidget *menuitem = NULL;
    GtkWidget *optionmenu = NULL;
      
    guint8 *value = NULL;

    menu = gtk_menu_new ();	 
    menuitem = gtk_menu_item_new_with_label (_("Perceptual"));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    value = g_new(guint8, 1);
    *value = INTENT_PERCEPTUAL;
    gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      

    menuitem = gtk_menu_item_new_with_label (_("Relative Colorimetric"));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    value = g_new(guint8, 1);
    *value = INTENT_RELATIVE_COLORIMETRIC;
    gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      

    menuitem = gtk_menu_item_new_with_label (_("Saturation"));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    value = g_new(guint8, 1);
    *value = INTENT_SATURATION;
    gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      

    menuitem = gtk_menu_item_new_with_label (_("Absolute Colorimetric"));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    value = g_new(guint8, 1);
    *value = INTENT_ABSOLUTE_COLORIMETRIC;
    gtk_object_set_data_full(GTK_OBJECT(menuitem), "value", (gpointer)value, g_free);      

    gtk_menu_set_active (GTK_MENU (menu), cms_default_intent);
    
    optionmenu = gtk_option_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
    gtk_table_attach (GTK_TABLE (table), optionmenu, left, left+1, top, top+1,
	              GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);

    return menu;
}

static void
cms_ok_cancel_buttons_new(GtkBox *action_area,
			  GtkSignalFunc ok_callback,
			  GtkSignalFunc cancel_callback,
			  gpointer data)
{   GtkWidget *buttonbox = NULL;
    GtkWidget *button = NULL;
   
    buttonbox = gtk_hbutton_box_new();
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(buttonbox), 4);
    gtk_box_set_homogeneous(action_area, FALSE);
    gtk_box_pack_end (action_area, buttonbox, FALSE, FALSE, 0);

    button = gtk_button_new_with_label(_("OK"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(ok_callback), data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label(_("Cancel"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(cancel_callback), data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);
}

/* 
 * CONVERT DIALOG
 * menu: Image>Convert using ICC Profile...
 */

/* private types */
typedef struct _CMSConvertDialogData
{   GtkWidget *shell;
    GtkWidget *intent_menu;
    GtkWidget *profile_menu;
    GtkWidget *bpc_toggle;

    GImage *image;
} CMSConvertDialogData;

/* private prototypes */
static void cms_convert_dialog_ok_callback(GtkWidget*, CMSConvertDialogData*);
static void cms_convert_dialog_cancel_callback(GtkWidget*, CMSConvertDialogData*);

/* public functions */
void 
cms_convert_dialog(GImage *image) 
{   GtkWidget *table = NULL;
    GtkWidget *label = NULL; 
    GtkWidget *vbox = NULL;

    CMSConvertDialogData *data=g_new(CMSConvertDialogData, 1); 
    data->image = image;
    data->shell = gtk_dialog_new();

    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "cms_convert", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), _("Select Profile and Intent"));
    gtk_window_set_position(GTK_WINDOW(data->shell), GTK_WIN_POS_CENTER);

    vbox = gtk_vbox_new(FALSE, 0);   
    gtk_container_border_width(GTK_CONTAINER (vbox), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(data->shell)->vbox), vbox, TRUE, TRUE, 0);

    
    label = gtk_label_new(_("Please select the profile to convert to and the rendering intent to use:"));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 10);

    label = gtk_label_new(_("Profile:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
	              GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
    data->profile_menu = cms_profile_menu_new(GTK_TABLE(table), 1, 0, FALSE, 
                                              cms_workspace_profile_name );

    label = gtk_label_new(_("Rendering Intent:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
	              GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
    data->intent_menu = cms_intent_menu_new(GTK_TABLE(table), 1, 1);

    {
      /* whether to blackpoint compensate */
      data->bpc_toggle = gtk_check_button_new_with_label(_("Black Point Compensation"));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (data->bpc_toggle),
                                   cms_bpc_by_default);
      gtk_table_attach (GTK_TABLE (table), data->bpc_toggle,1, 2, 2, 3,
            GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
      gtk_widget_show(data->bpc_toggle);
    }

    cms_ok_cancel_buttons_new(GTK_BOX(GTK_DIALOG(data->shell)->action_area),
			      GTK_SIGNAL_FUNC(cms_convert_dialog_ok_callback),
			      GTK_SIGNAL_FUNC(cms_convert_dialog_cancel_callback),
			      data);
    
    gtk_widget_show_all(data->shell);
}

/* private functions */
static void 
cms_convert_dialog_ok_callback(GtkWidget *widget, CMSConvertDialogData *data)
{   GtkWidget *selected = NULL;
    char *profile = NULL; 
    guint8 intent = cms_default_intent;
    gboolean new_val;
    DWORD flags;

    /* get the dialog values */
    selected  = gtk_menu_get_active(GTK_MENU(data->profile_menu));
    profile = (char *)gtk_object_get_data(GTK_OBJECT(selected), "value");
    selected = gtk_menu_get_active(GTK_MENU(data->intent_menu));
    intent = *((guint8 *)gtk_object_get_data(GTK_OBJECT(selected), "value"));
    new_val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (data->bpc_toggle)) ? TRUE : FALSE;
    flags = new_val ? cmsFLAGS_WHITEBLACKCOMPENSATION : 0;

    /* convert */
    convert_image_colorspace(data->image, cms_get_profile_from_file(profile), intent, flags);
    
    /* destroy dialog */
    cms_convert_dialog_cancel_callback(widget, data);
}

static void 
cms_convert_dialog_cancel_callback(GtkWidget *widget, CMSConvertDialogData *data)
{   gtk_widget_destroy(data->shell);
    g_free(data); 
}

/* 
 * ASSIGN DIALOG
 * menu: Image>Assign ICC Profile 
 */

/* private types */
typedef struct _CMSAssignDialogData
{   GtkWidget *shell;
    GtkWidget *profile_menu;

    GImage *image;
    CMSProfileType type;
} CMSAssignDialogData;

/* private prototypes */    
static void cms_assign_dialog_ok_callback (GtkWidget*, CMSAssignDialogData*);
static void cms_assign_dialog_cancel_callback(GtkWidget*, CMSAssignDialogData*);

/* public functions */
void 
cms_assign_dialog(GImage *image, CMSProfileType type)
{   GtkWidget *table = NULL;
    GtkWidget *label = NULL; 
    GtkWidget *vbox = NULL;
    GtkWidget *alignment = NULL;
    CMSProfile *current_profile = NULL;

    CMSAssignDialogData *data=g_new(CMSAssignDialogData,1); 
    GString *profile_string = g_string_new(NULL);
    char title[2][32];

    snprintf(title[0], 32,_("Select Image Profile"));
    snprintf(title[1], 32,_("Select Proof Profile"));

    data->image = image;
    data->shell = gtk_dialog_new();
    data->type = type;

    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "cms_assign", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), title[type]);
    gtk_window_set_position(GTK_WINDOW(data->shell), GTK_WIN_POS_CENTER);

    vbox = gtk_vbox_new(FALSE, 0);   
    gtk_container_border_width(GTK_CONTAINER (vbox), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(data->shell)->vbox), vbox, TRUE, TRUE, 0);
    table = gtk_table_new(3, 1, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
    
    if (type ==  ICC_IMAGE_PROFILE)
      current_profile = gimage_get_cms_profile(image);
    else if (type == ICC_PROOF_PROFILE)
      current_profile = gimage_get_cms_proof_profile(image);
    
    if (current_profile == NULL)
    {   g_string_sprintf (profile_string, _("[The currently assigned profile is: [none]]"));
    }
    else
    {   g_string_sprintf (profile_string, _("[The currently assigned profile is: %s]"), 
			  cmsTakeProductDesc(current_profile->handle));
    }
    label = gtk_label_new(profile_string->str);
    g_string_free(profile_string, FALSE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

    alignment = gtk_alignment_new(0.0, 1.0, 0.0, 0.0);
    
    if (type ==  ICC_IMAGE_PROFILE)
      label = gtk_label_new(_("Please select the new image profile to assign:"));
    else if (type == ICC_PROOF_PROFILE)
      label = gtk_label_new(_("Please select the new proof profile to assign:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 1, 2, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 10);   

    data->profile_menu = cms_profile_menu_new(GTK_TABLE(table), 0, 2, TRUE,
                             type ==  ICC_IMAGE_PROFILE ?
                               cms_workspace_profile_name :
                               cms_default_proof_profile_name);
    
    cms_ok_cancel_buttons_new(GTK_BOX(GTK_DIALOG(data->shell)->action_area),
			      GTK_SIGNAL_FUNC(cms_assign_dialog_ok_callback),
			      GTK_SIGNAL_FUNC(cms_assign_dialog_cancel_callback),
			      data);
    
    gtk_widget_show_all(data->shell);
}

/* private functions */
static void 
cms_assign_dialog_ok_callback(GtkWidget *widget, CMSAssignDialogData *data)
{   GtkWidget *selected = NULL;
    char *profile_name = NULL; 

    /* get the dialog value */
    selected  = gtk_menu_get_active(GTK_MENU(data->profile_menu));
    profile_name = (char *)gtk_object_get_data(GTK_OBJECT(selected), "value");
    
    /* assign it */
    if (profile_name != NULL) 
    {
        if (data->type ==  ICC_IMAGE_PROFILE)
            gimage_set_cms_profile(data->image, cms_get_profile_from_file(profile_name));
        else if (data->type == ICC_PROOF_PROFILE)
            gimage_set_cms_proof_profile(data->image, cms_get_profile_from_file(profile_name));
    }
    else 
    {
        if        (data->type == ICC_IMAGE_PROFILE)
        {   g_message(_("You set the image profile to [none]. Colormanagement will be\nswitched off for all displays of this image."));
            gimage_set_cms_profile(data->image, NULL);
        } else if (data->type == ICC_PROOF_PROFILE)
        {   g_message(_("You set the proof profile to [none]. Proofing will be\nnot available off for all displays of this image."));
            gimage_set_cms_proof_profile(data->image, NULL);
        }
    }
    gdisplays_update_gimage (data->image->ID);
    gdisplays_flush();

    /* destroy dialog */
    cms_assign_dialog_cancel_callback(widget, data);
}

static void 
cms_assign_dialog_cancel_callback(GtkWidget *widget, CMSAssignDialogData *data)
{   gtk_widget_destroy(data->shell);
    g_free(data); 
}

/* 
 * OPEN ASSIGN DIALOG
 * modal dialog shown when opening image
 * if File>Preferences>Color Management>When Opening Image=prompt
 */

/* private types */
typedef struct _CMSOpenAssignDialogData
{   GtkWidget *shell;
    GtkWidget *default_radio;
    GtkWidget *profile_radio;
    GtkWidget *profile_menu;
    GtkWidget *always_default_check;
    GMainLoop *event_loop;
    
    GImage *image;
} CMSOpenAssignDialogData;

/* private prototypes */    
static void cms_open_assign_dialog_ok_callback (GtkWidget*, CMSOpenAssignDialogData*);
static void cms_open_assign_dialog_cancel_callback (GtkWidget*, CMSOpenAssignDialogData*);

/* public functions */
void 
cms_open_assign_dialog(GImage *image)
{   GtkWidget *table = NULL;
    GtkWidget *label = NULL; 
    GSList *radiogroup = NULL;
    GtkWidget *vbox = NULL;
    GtkWidget *alignment = NULL;

    GString *profile_string = g_string_new(NULL);    
    CMSOpenAssignDialogData *data=g_new(CMSOpenAssignDialogData,1); 

    data->image = image;
    data->shell = gtk_dialog_new();

    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "cms_open_assign", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), _("Select Profile"));
    gtk_window_set_position(GTK_WINDOW(data->shell), GTK_WIN_POS_CENTER);

    vbox = gtk_vbox_new(FALSE, 0);   
    gtk_container_border_width(GTK_CONTAINER (vbox), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(data->shell)->vbox), vbox, TRUE, TRUE, 0);
    
    /* message */
    label = gtk_label_new(_("Please select the image profile to assign to the image:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

    /* radio group */
    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 10);
  
    data->profile_radio = gtk_radio_button_new_with_label(NULL, _("Profile: "));
    radiogroup = gtk_radio_button_group(GTK_RADIO_BUTTON(data->profile_radio));
    gtk_table_attach(GTK_TABLE(table), data->profile_radio, 0, 1, 0, 1, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

    data->profile_menu = cms_profile_menu_new( GTK_TABLE(table), 1, 0, TRUE,
                                               cms_default_image_profile_name );

    if (cms_default_image_profile_name == NULL)
    {   g_string_sprintf (profile_string, _("The assumed image profile: [none]"));
    }
    else
    {   g_string_sprintf (profile_string, _("The assumed image profile: %s"), 
			  cmsTakeProductDesc(cms_get_profile_from_file(cms_default_image_profile_name)->handle));
    }

    data->default_radio = gtk_radio_button_new_with_label(radiogroup, profile_string->str);
    gtk_table_attach(GTK_TABLE(table), data->default_radio, 0, 2, 1, 2, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
    g_string_free(profile_string, FALSE);


    /* default check box */
    data->always_default_check = gtk_check_button_new_with_label(_("always use default profile"));
    alignment = gtk_alignment_new(0.0, 1.0, 0, 0);    
    gtk_container_add(GTK_CONTAINER(alignment), data->always_default_check);
    gtk_box_pack_start (GTK_BOX (vbox), alignment, TRUE, TRUE, 5);    

    
    cms_ok_cancel_buttons_new(GTK_BOX(GTK_DIALOG(data->shell)->action_area),
			      GTK_SIGNAL_FUNC(cms_open_assign_dialog_ok_callback), 
			      GTK_SIGNAL_FUNC(cms_open_assign_dialog_cancel_callback), 
			      data);    
    
    gtk_widget_show_all(data->shell);

    data->event_loop = g_main_new(FALSE);    
    g_main_run(data->event_loop);
    g_main_destroy(data->event_loop);
    g_free(data);
}

/* private functions */
static void 
cms_open_assign_dialog_ok_callback(GtkWidget *widget, CMSOpenAssignDialogData *data)
{   GtkWidget *selected = NULL;
    char *profile_name = NULL; 
    cmsHPROFILE profile_handle = NULL;

    /* get the profile */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->default_radio)))
    {   profile_name = cms_default_image_profile_name;
    }
    else 
    {   selected  = gtk_menu_get_active(GTK_MENU(data->profile_menu));
        profile_name = (char *)gtk_object_get_data(GTK_OBJECT(selected), "value");    
    }

    if (profile_name == NULL)
    {   profile_handle = NULL;
    }
    else
    {   profile_handle = cms_get_profile_from_file(profile_name);
    }

    /* assign it */
    gimage_set_cms_profile(data->image, profile_handle);
    gdisplays_update_gimage (data->image->ID)
;
    /* refresh the profile name in the title bar */
    gdisplays_update_title (data->image->ID);    
    
    /* update cms_open_action (global preference variable) */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->always_default_check)))
    {
        GList *update_settings = NULL;
        GList *remove_settings = NULL;

        cms_open_action = CMS_ASSIGN_DEFAULT;
	update_settings = g_list_append(update_settings, "cms-open-action");
	save_gimprc(&update_settings, &remove_settings); 
	g_list_free(update_settings);
    }     

    /* destroy dialog */
    cms_open_assign_dialog_cancel_callback(widget, data);
}

static void 
cms_open_assign_dialog_cancel_callback(GtkWidget *widget, CMSOpenAssignDialogData *data)
{   gtk_widget_destroy(data->shell);
    g_main_quit(data->event_loop);
}






/* 
 * CONVERT ON OPEN PROMPT
 * modal dialog, shown when opening image
 * asks whether to convert to working colorspace if mismatch
 */

/* private types*/
typedef struct _CMSConvertOnOpenPromptData
{   GtkWidget *shell;
    GtkWidget *auto_convert_check;
    GMainLoop *event_loop;
    gboolean return_value;
}
CMSConvertOnOpenPromptData;

/* private prototypes */
static void cms_convert_on_open_prompt_yes_callback (GtkWidget *widget, CMSConvertOnOpenPromptData *data);
static void cms_convert_on_open_prompt_no_callback (GtkWidget *widget, CMSConvertOnOpenPromptData *data);

/* public functions */
gboolean
cms_convert_on_open_prompt(GImage *image)
{   GtkWidget *label;
    GtkWidget *button;
    GtkWidget *buttonbox;
    GtkWidget *table;
    GtkWidget *vbox;
    GtkWidget *alignment;

    gboolean return_value;
    CMSConvertOnOpenPromptData *data = g_new(CMSConvertOnOpenPromptData, 1);

    data->shell = gtk_dialog_new();
    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "cms_convert_profile_prompt", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), _("Select Profile"));
    gtk_window_set_position(GTK_WINDOW(data->shell), GTK_WIN_POS_CENTER);

    vbox = gtk_vbox_new(FALSE, 0);   
    gtk_container_border_width(GTK_CONTAINER (vbox), 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(data->shell)->vbox), vbox, TRUE, TRUE, 0);

    /* message */
    label = gtk_label_new (_("The profile that was assigned to this image does not match your editing profile."));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);  

    /* current profiles */
    table = gtk_table_new(2,2,FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 10);  

    label = gtk_label_new(_("Editing profile:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);   

    label = gtk_label_new(strdup(cmsTakeProductDesc(cms_get_profile_from_file(cms_workspace_profile_name)->handle)));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);   

    label = gtk_label_new(_("Assigned profile:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);   

    label = gtk_label_new(strdup(cmsTakeProductDesc(gimage_get_cms_profile(image)->handle)));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, 
		     GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);   

    /* alway convert checkbox */
    data->auto_convert_check = gtk_check_button_new_with_label(_("Always convert automatically without asking"));
    alignment = gtk_alignment_new(0.0, 1.0, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(alignment), data->auto_convert_check);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 5);  

    /* buttons */
    buttonbox = gtk_hbutton_box_new();
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(buttonbox), 4);
    gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(data->shell)->action_area), FALSE);
    gtk_box_pack_end (GTK_BOX (GTK_DIALOG(data->shell)->action_area), buttonbox, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label(_("Yes"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       GTK_SIGNAL_FUNC(cms_convert_on_open_prompt_yes_callback), (gpointer)data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label(_("No"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       GTK_SIGNAL_FUNC(cms_convert_on_open_prompt_no_callback), (gpointer)data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);        


    gtk_widget_show_all(data->shell); 

    data->event_loop = g_main_new(FALSE);    
    g_main_run(data->event_loop);

    return_value = data->return_value;
    g_main_destroy(data->event_loop);
    g_free(data);

    return return_value;
}

/* private functions */
static void
cms_convert_on_open_prompt_yes_callback (GtkWidget *widget, CMSConvertOnOpenPromptData *data)
{   data->return_value = TRUE;  
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->auto_convert_check)))
    {   /* cms_auto_convert is global preference variable */
        GList *update_settings = NULL;
        GList *remove_settings = NULL;

        cms_mismatch_action = CMS_MISMATCH_AUTO_CONVERT;
	update_settings = g_list_append(update_settings, "cms-mismatch-action");
	save_gimprc(&update_settings, &remove_settings); 
	g_list_free(update_settings);
    }
    gtk_widget_destroy(data->shell);
    g_main_quit(data->event_loop);
}

static void
cms_convert_on_open_prompt_no_callback (GtkWidget *widget, CMSConvertOnOpenPromptData *data)
{   data->return_value = FALSE;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->auto_convert_check))) 
    {   /* cms_auto_convert is global preference variable */
        cms_mismatch_action = CMS_MISMATCH_AUTO_CONVERT;
    }
    gtk_widget_destroy(data->shell);
    g_main_quit(data->event_loop);
}



/*cms_add_profile(*profile_list, profile){

}

cms_remove_profile(*profile_list, profile) {
}

 g_frees the transform and possibly deletes the tempfile 
cms free_transform(){
}

*/
