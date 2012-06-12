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
#include "plugin_main.h"

void
gimp_displays_delete_image (gint32 image_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_displays_delete_image",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_display_new (gint32 image_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 display_ID;

  return_vals = gimp_run_procedure ("gimp_display_new",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  display_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    display_ID = return_vals[1].data.d_display;

  gimp_destroy_params (return_vals, nreturn_vals);

  return display_ID;
}

gint32
gimp_display_fm (gint32 image_ID, gint32 disp_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 display_ID;

  return_vals = gimp_run_procedure ("gimp_display_fm",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_DISPLAY, disp_ID,
                                    PARAM_END);

  display_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    display_ID = return_vals[1].data.d_display;

  gimp_destroy_params (return_vals, nreturn_vals);

  return display_ID;
}

gint32
gimp_display_active ()
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 display_ID = -1;

  return_vals = gimp_run_procedure ("gimp_display_active",
                                    &nreturn_vals,
                                    PARAM_END);
#ifdef DEBUG_
  printf("%s:%d %s() %d  0x%x|%d\n",__FILE__,__LINE__,__func__,
        display_ID, return_vals[1].data.d_display, return_vals[1].data.d_int32);
#endif
  display_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    display_ID = return_vals[1].data.d_display;

#ifdef DEBUG_
  printf("%s:%d %s() %d:%d ID:%d  0x%x|%d\n",__FILE__,__LINE__,__func__,
         return_vals[0].data.d_status, STATUS_SUCCESS, display_ID,
         return_vals[1].data.d_display, return_vals[1].data.d_int32);
#endif

  gimp_destroy_params (return_vals, nreturn_vals);

  return display_ID;
}

void
gimp_display_delete (gint32 display_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_delete",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_displays_flush ()
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_displays_flush",
                                    &nreturn_vals,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

int
gimp_display_get_image_id   (gint32 display_ID )
{
  GParam *return_vals;
  int nreturn_vals;
  int image_ID = -1;

  return_vals = gimp_run_procedure ("gimp_display_get_image_id",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    image_ID = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

int
gimp_display_get_cms_intent (gint32 display_ID, 
                             CMSProfileType type )
{
  GParam *return_vals;
  int nreturn_vals;
  int intent = -1;

  return_vals = gimp_run_procedure ("gimp_display_get_cms_intent",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, type,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    intent = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return intent;
}

void
gimp_display_set_cms_intent (gint32 display_ID, gint32 intent,
                             CMSProfileType type )
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_set_cms_intent",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, intent,
                                    PARAM_INT32, type,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

}

int
gimp_display_get_cms_flags (gint32 display_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  int flags = -1;

  return_vals = gimp_run_procedure ("gimp_display_get_cms_flags",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    flags = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return flags;
}

void
gimp_display_set_cms_flags (gint32 display_ID, gint32 flags)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_set_cms_flags",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, flags,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

}

int
gimp_display_is_colormanaged (gint32 display_ID, 
                              CMSProfileType type )
{
  GParam *return_vals;
  int nreturn_vals;
  int colormanaged = -1;

  return_vals = gimp_run_procedure ("gimp_display_is_colormanaged",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, type,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    colormanaged = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return colormanaged;
}

void
gimp_display_set_colormanaged (gint32 display_ID, gint32 colormanaged, 
                               CMSProfileType type )
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_set_colormanaged",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, colormanaged,
                                    PARAM_INT32, type,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

}

void
gimp_display_all_set_colormanaged (gint32 colormanaged)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_all_set_colormanaged",
                                    &nreturn_vals,
                                    PARAM_INT32, colormanaged,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

}

void
gimp_display_image_set_colormanaged (gint32 display_ID, gint32 colormanaged)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_image_set_colormanaged",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_INT32, colormanaged,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);

}


