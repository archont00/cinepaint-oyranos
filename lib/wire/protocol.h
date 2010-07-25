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
#ifndef __GIMP_PROTOCOL_H__
#define __GIMP_PROTOCOL_H__


#include <glib.h>
#include "../lib/dll_api.h"

/* Increment every time the protocol changes
 */
#define GP_VERSION 0x0002


enum {
  GP_ERROR,
  GP_QUIT,
  GP_CONFIG,
  GP_TILE_REQ,
  GP_TILE_ACK,
  GP_TILE_DATA,
  GP_PROC_RUN,
  GP_PROC_RETURN,
  GP_TEMP_PROC_RUN,
  GP_TEMP_PROC_RETURN,
  GP_PROC_INSTALL,
  GP_PROC_UNINSTALL,
  GP_EXTENSION_ACK
};


typedef struct GPConfig       GPConfig;
typedef struct GPTileReq      GPTileReq;
typedef struct GPTileAck      GPTileAck;
typedef struct GPTileData     GPTileData;
typedef struct GPParam        GPParam;
typedef struct GPParamDef     GPParamDef;
typedef struct GPProcRun      GPProcRun;
typedef struct GPProcReturn   GPProcReturn;
typedef struct GPProcInstall  GPProcInstall;
typedef struct GPProcUninstall  GPProcUninstall;


struct GPConfig
{
  guint32 version;
  guint32 tile_width;
  guint32 tile_height;
  gint32 shm_ID;
  gdouble gamma;
  gint8 install_cmap;
  gint8 use_xshm;
  guint8 color_cube[4];
};

struct GPTileReq
{
  gint32 drawable_ID;
  guint32 tile_num;
  guint32 shadow;
};

struct GPTileData
{
  gint32 drawable_ID;
  guint32 tile_num;
  guint32 shadow;
  guint32 bpp;
  guint32 width;
  guint32 height;
#ifdef BUILD_SHM
  guint32 use_shm;
#endif
  guchar *data;
};

struct GPParam
{
  guint32 type;

  union {
    gint32 d_int32;
    gint16 d_int16;
    gint8 d_int8;
    gdouble d_float;
    gchar *d_string;
    gint32 *d_int32array;
    gint16 *d_int16array;
    gint8 *d_int8array;
    gdouble *d_floatarray;
    gchar **d_stringarray;
    struct {
      guint8 red;
      guint8 green;
      guint8 blue;
    } d_color;
    struct {
      gint32 x;
      gint32 y;
      gint32 width;
      gint32 height;
    } d_region;
    gint32 d_display;
    gint32 d_image;
    gint32 d_layer;
    gint32 d_channel;
    gint32 d_drawable;
    gint32 d_selection;
    gint32 d_boundary;
    gint32 d_path;
    gint32 d_status;
  } data;
};

struct GPParamDef
{
  guint32 type;
  char *name;
  char *description;
};

struct GPProcRun
{
  char *name;
  guint32 nparams;
  GPParam *params;
};

struct GPProcReturn
{
  char *name;
  guint32 nparams;
  GPParam *params;
};

struct GPProcInstall
{
  char *name;
  char *blurb;
  char *help;
  char *author;
  char *copyright;
  char *date;
  char *menu_path;
  char *image_types;
  guint32 type;
  guint32 nparams;
  guint32 nreturn_vals;
  GPParamDef *params;
  GPParamDef *return_vals;
};

struct GPProcUninstall
{
  char *name;
};


DLL_API void gp_init                   (void);
int  gp_quit_write             (int            fd);
DLL_API int  gp_config_write           (int            fd,
				GPConfig      *config);
int  gp_tile_req_write         (int            fd,
				GPTileReq     *tile_req);
DLL_API int  gp_tile_ack_write         (int            fd);
DLL_API int  gp_tile_data_write        (int            fd,
				GPTileData    *tile_data);
DLL_API int  gp_proc_run_write         (int            fd,
				GPProcRun     *proc_run);
DLL_API int  gp_proc_return_write      (int            fd,
				GPProcReturn  *proc_return);
DLL_API int  gp_temp_proc_run_write    (int            fd,
				GPProcRun     *proc_run);
int  gp_temp_proc_return_write (int            fd,
				GPProcReturn  *proc_return);
int  gp_proc_install_write     (int            fd,
				GPProcInstall *proc_install);
int  gp_proc_uninstall_write   (int            fd,
				GPProcUninstall *proc_uninstall);
int  gp_extension_ack_write    (int            fd);


#endif /* __GIMP_PROTOCOL_H__ */
