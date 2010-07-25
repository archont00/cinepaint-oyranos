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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DEVICES_H__
#define __DEVICES_H__

#include "tools.h"

typedef enum
{
  DEVICE_MODE       = 1 << 0,
  DEVICE_AXES       = 1 << 1,
  DEVICE_KEYS       = 1 << 2,
  DEVICE_TOOL       = 1 << 3,
  DEVICE_FOREGROUND = 1 << 4,
  DEVICE_BACKGROUND = 1 << 5,
  DEVICE_BRUSH      = 1 << 6,
  DEVICE_PATTERN    = 1 << 7,
  DEVICE_GRADIENT   = 1 << 8
} DeviceValues;

/*  Create device info dialog  */
void   create_input_dialog  (void);

/*  Initialize the input devices  */
void   devices_init         (void);

/*  Restores device settings from rc file  */
void   devices_restore      (void);

/*  Returns TRUE, and makes necessary global changes
 *  event is not for current_device
 */
gint   devices_check_change (GdkEvent     *event);

/*  Loads stored device settings (tool, cursor, ...)  */
void   select_device        (
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                                           device);

/*  Create the device status dialog  */
void   create_device_status (void);

/* Update the device status dialog for a device */
void device_status_update (
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                                           deviceid);

/* Add information about one tool from rc file */
void   devices_rc_update    (gchar        *name,
			     DeviceValues  values,
			     GdkInputMode  mode,
			     gint          num_axes,
			     GdkAxisUse   *axes,
			     gint          num_keys,
			     GdkDeviceKey *keys,
			     gchar *brush_name, ToolType tool,
			     guchar foreground[]);

/*  Current device id  */
#if GTK_MAJOR_VERSION > 1
extern GdkDevice *current_device;
#else
extern gint current_device;
#endif

#endif /* __DEVICES_H__ */
