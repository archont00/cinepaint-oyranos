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

#include "config.h"

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include "appenv.h"
#include "actionarea.h"
#include "devices.h"
#include "interface.h"
#include "rc.h"
#include "palette.h"
#include "pixelrow.h"
#include "tools.h"
#include "brushlist.h"
#include "libgimp/gimpintl.h"

typedef struct DeviceInfo DeviceInfo;

struct DeviceInfo {
#if GTK_MAJOR_VERSION > 1
  GdkDevice *device;		/* a Gdk device */
#else
  guint32 device;		/* device ID */
#endif
  gchar *name;
  
  short is_init;		/* has the user used it? */
  short is_present;		/* is the device currently present */
  
  /* gdk_input options - for not present devices */

  GdkInputMode mode;
  gint num_axes;
  GdkAxisUse *axes;
  gint num_keys;
  GdkDeviceKey *keys;

  GimpBrushP brush;
  ToolType tool;
  unsigned char foreground[3];
};

typedef struct DeviceInfoDialog DeviceInfoDialog;

struct DeviceInfoDialog {
  int num_devices;
#if GTK_MAJOR_VERSION > 1
  GdkDevice *current;		/* Gdk device */
  GdkDevice **ids;
#else
  guint32 current;
  guint32 *ids;
#endif

  GtkWidget *shell;
  GtkWidget *table;

  GtkWidget **frames;
  GtkWidget **tools;
  GtkWidget **colors;
  GtkWidget **brushes;

};

/* Local Data */

GList *devices_info = NULL;
DeviceInfoDialog *deviceD = NULL;

/* If true, don't update device information dialog */
int suppress_update = FALSE;

/* Local functions */
static void input_dialog_destroy_callback (GtkWidget *, gpointer);
void input_dialog_able_callback (GtkWidget *w, 
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                                          deviceid, gpointer data);
static void devices_save_current_info (void);
static void devices_write_rc_device (DeviceInfo *device_info, FILE *fp);
static void devices_write_rc (void);
static void device_status_destroy_callback (void);
static void devices_close_callback (GtkWidget *, gpointer);
static void device_status_update_current ();

/* Global data */
#if GTK_MAJOR_VERSION > 1
GdkDevice *current_device = NULL;
#else
int current_device = GDK_CORE_POINTER;
#endif

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", devices_close_callback, NULL, NULL },
  { "Save", (ActionCallback)devices_write_rc, NULL, NULL },
};

void 
create_input_dialog (void)
{
  static GtkWidget *inputd = NULL;

  if (!inputd)
    {
      inputd = gtk_input_dialog_new();
      
      gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG(inputd)->action_area), 2);

      gtk_signal_connect (GTK_OBJECT(GTK_INPUT_DIALOG(inputd)->save_button),
			  "clicked",
			  GTK_SIGNAL_FUNC(devices_write_rc), NULL);
      gtk_signal_connect (GTK_OBJECT(GTK_INPUT_DIALOG(inputd)->close_button),
			  "clicked",
			  GTK_SIGNAL_FUNC(devices_close_callback), inputd);

      gtk_signal_connect (GTK_OBJECT(inputd), "destroy",
			  (GtkSignalFunc)input_dialog_destroy_callback, 
			  &inputd);
      gtk_signal_connect (GTK_OBJECT(inputd), "enable_device",
			  GTK_SIGNAL_FUNC(input_dialog_able_callback), NULL);
      gtk_signal_connect (GTK_OBJECT(inputd), "disable_device",
			  GTK_SIGNAL_FUNC(input_dialog_able_callback), NULL);
      gtk_widget_show (inputd);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED(inputd))
	gtk_widget_show(inputd);
      else
	gdk_window_raise(inputd->window);
    }
}

void
input_dialog_able_callback (GtkWidget *w, 
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                                          deviceid, gpointer data)
{
  device_status_update (deviceid);
}

static void
input_dialog_destroy_callback (GtkWidget *w, 
			       gpointer call_data)
{
  *((GtkWidget **)call_data) = NULL;
}

void 
devices_init (void)
{
  GList *tmp_list;
  char *gimp_dir;
  char filename[512];

  /* Create device info structures for present devices */

#if GTK_MAJOR_VERSION > 1
  {
    GdkDisplay *display = 0;
    tmp_list = gdk_devices_list ();
    display = gdk_display_get_default();;
    if(!current_device)
      current_device = gdk_display_get_core_pointer (display);
  }
#else
  tmp_list = gdk_input_list_devices ();
#endif
  
  while (tmp_list)
    {
      DeviceInfo *device_info = g_new (DeviceInfo, 1);
#if GTK_MAJOR_VERSION > 1
      GdkDevice *gdk_info = (GdkDevice *)tmp_list->data;

      device_info->device = gdk_info;
      device_info->name = g_strdup (gdk_info->name);

      device_info->mode = gdk_info->mode;
      device_info->num_axes = gdk_info->num_axes;
#else
      GdkDeviceInfo *gdk_info = (GdkDeviceInfo *)tmp_list->data;

      device_info->device = gdk_info->deviceid;
      device_info->name = g_strdup (gdk_info->name);

      device_info->mode = gdk_info->mode;
      device_info->num_axes = gdk_info->num_axes;
#endif
      device_info->is_init = FALSE;
      device_info->is_present = TRUE;

      device_info->axes = NULL;
      
      device_info->brush = NULL;
      device_info->tool = RECT_SELECT;
      device_info->foreground[0] = 0;
      device_info->foreground[1] = 0;
      device_info->foreground[2] = 0;

      devices_info = g_list_append (devices_info, device_info);
	       
      tmp_list = tmp_list->next;
    }
  
  /* Augment with information from rc file */

  gimp_dir = gimp_directory ();

  if (gimp_dir)
    {
      sprintf (filename, "%s/devicerc", gimp_dir);
      parse_gimprc_file (filename);
    }
}

void
devices_rc_update (gchar *name, DeviceValues values,
		   GdkInputMode mode, gint num_axes,
		   GdkAxisUse *axes, gint num_keys, GdkDeviceKey *keys,
		   gchar *brush_name, ToolType tool,
		   guchar foreground[])
{
  GList *tmp_list;
  DeviceInfo *device_info;

  /* Find device if we have it */

  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (!strcmp (((DeviceInfo *)tmp_list->data)->name, name))
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if (!device_info)
    {
      device_info = g_new (DeviceInfo, 1);
      device_info->name = g_strdup (name);
      device_info->is_present = FALSE;

      if (values & DEVICE_AXES)
	{
	  device_info->num_axes = num_axes;
	  device_info->axes = g_new (GdkAxisUse, num_axes);
	  memcpy (device_info->axes, axes, num_axes * sizeof (GdkAxisUse));
	}
      else
	{
	  device_info->num_axes = 0;
	  device_info->axes = NULL;
	}

      if (values & DEVICE_KEYS)
	{
	  device_info->num_keys = num_keys;
	  device_info->keys = g_new (GdkDeviceKey, num_keys);
	  memcpy (device_info->keys, keys, num_keys * sizeof (GdkDeviceKey));
	}

      if (values & DEVICE_MODE)
	device_info->mode = mode;
      else
	device_info->mode = GDK_MODE_DISABLED;

      device_info->brush = NULL;
      device_info->tool = RECT_SELECT;
      device_info->foreground[0] = 0;
      device_info->foreground[1] = 0;
      device_info->foreground[2] = 0;

      devices_info = g_list_append (devices_info, device_info);
    }
  else
    {
#if GTK_MAJOR_VERSION > 1
      GdkDevice *gdk_info = NULL;	/* Quiet gcc */
      tmp_list = gdk_devices_list();
#else
      GdkDeviceInfo *gdk_info = NULL;	/* Quiet gcc */
      tmp_list = gdk_input_list_devices();
#endif

      while (tmp_list)
	{
#if GTK_MAJOR_VERSION > 1
	  GdkDevice *info = (GdkDevice *)tmp_list->data;
	  if (info == device_info->device)
#else
	  GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;
	  if (info->deviceid == device_info->device)
#endif	  
	    {
	      gdk_info = info;
	      break;
	    }
	  tmp_list = tmp_list->next;
	}

      if (gdk_info != NULL)
	{
	  if (values & DEVICE_MODE)
#if GTK_MAJOR_VERSION > 1
	    gdk_device_set_mode (gdk_info, mode);
#else
	    gdk_input_set_mode (gdk_info->deviceid, mode);
#endif	  
	  if ((values & DEVICE_AXES) && num_axes >= gdk_info->num_axes)
#if GTK_MAJOR_VERSION > 1
	    /*gdk_device_set_axes (gdk_info, axes)*/;
#else
	    gdk_input_set_axes (gdk_info->deviceid, axes);
#endif
	  
	  if ((values & DEVICE_KEYS) && num_keys >= gdk_info->num_keys)
	    {
	      int i;
	      
	      for (i=0; i<MAX (num_keys, gdk_info->num_keys); i++)
#if GTK_MAJOR_VERSION > 1
		gdk_device_set_key (gdk_info, i,
				   keys[i].keyval, keys[i].modifiers);
#else
		gdk_input_set_key (gdk_info->deviceid, i,
				   keys[i].keyval, keys[i].modifiers);
#endif
	    }
	}
      else
	{
	  g_warning (_("devices_rc_update called multiple times for not present device\n"));
	  return;
	}
    }

  if (values & (DEVICE_BRUSH | DEVICE_TOOL | DEVICE_FOREGROUND))
    device_info->is_init = TRUE;

  if (values & DEVICE_BRUSH)
    {
      device_info->brush = gimp_brush_list_get_brush(brush_list, brush_name);;
    }

  if (values & DEVICE_TOOL)
    device_info->tool = tool;

  if (values & DEVICE_FOREGROUND)
    {
      device_info->foreground[0] = foreground[0];
      device_info->foreground[1] = foreground[1];
      device_info->foreground[2] = foreground[2];
    }
}

void 
select_device (
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                       new_device)
{
  GList *tmp_list;
  DeviceInfo *device_info;

  suppress_update = TRUE;
  
  /* store old information */

  devices_save_current_info();
  
  /* Now see if there is already information about the new device */

  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (((DeviceInfo *)tmp_list->data)->device == new_device)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  current_device = new_device;
  
  if (device_info->is_init)
    {
      if (device_info->brush)
	select_brush (device_info->brush);
      gtk_widget_activate (tool_widgets[tool_info[(int) device_info->tool].toolbar_position]);

        {
          PixelRow col;
          gfloat d[3];

          d[0] = device_info->foreground[0] / 255.;
          d[1] = device_info->foreground[1] / 255.;
          d[2] = device_info->foreground[2] / 255.;
          
          pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO), (guchar *) d, 1);

          palette_set_foreground (&col);
        }
    }

  suppress_update = FALSE;

  device_status_update_current ();
}

gint
devices_check_change (GdkEvent *event)
{
#if GTK_MAJOR_VERSION > 1
  GdkDevice *device = NULL;
  GtkWidget         *source;

  /* It is possible that the event was propagated from a widget that does not
     want extension events and therefore always sends core pointer events.
     This can cause a false switch to the core pointer device. */

  source = gtk_get_event_widget (event);

  if (source &&
      gtk_widget_get_extension_events (source) == GDK_EXTENSION_EVENTS_NONE)
    return FALSE;

#else
  int device;
#endif

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
#if GTK_MAJOR_VERSION > 1
      device = ((GdkEventMotion *)event)->device;
#else
      device = ((GdkEventMotion *)event)->deviceid;
#endif
      break;
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
#if GTK_MAJOR_VERSION > 1
      device = ((GdkEventButton *)event)->device;
#else
      device = ((GdkEventButton *)event)->deviceid;
#endif
      break;
    case GDK_PROXIMITY_IN:
    case GDK_PROXIMITY_OUT:
#if GTK_MAJOR_VERSION > 1
      device = ((GdkEventProximity *)event)->device;
#else
      device = ((GdkEventProximity *)event)->deviceid;
#endif
      break;
#if GTK_MAJOR_VERSION > 1
    case GDK_SCROLL:
      device = ((GdkEventScroll *) event)->device;
      break;
#endif
    default:
      device = current_device;
    }
  
  if (device != current_device)
    {
      select_device (device);
      return TRUE;
    }
  else
    return FALSE;
}

static void
devices_save_current_info (void)
{
  GList *tmp_list;
  DeviceInfo *device_info;
  
  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (
#if GTK_MAJOR_VERSION < 2
          CAST(int)
#endif
          ((DeviceInfo *)tmp_list->data)->device == current_device)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if(!device_info)
    return;

  device_info->is_init = TRUE;
  device_info->device = current_device;
  device_info->brush = get_active_brush ();
  if (active_tool_type >= 0)
    device_info->tool = active_tool_type;
  else
    device_info->tool = RECT_SELECT;

        {
          PixelRow col;
          gfloat d[3];

          pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO), (guchar *) d, 1);

          palette_get_foreground (&col);

          device_info->foreground[0] = d[0] * 255.;
          device_info->foreground[1] = d[1] * 255.;
          device_info->foreground[2] = d[2] * 255.;
        }
}

static void
devices_write_rc_device (DeviceInfo *device_info, FILE *fp)
{
#if GTK_MAJOR_VERSION > 1
  GdkDevice *gdk_info;
#else
  GdkDeviceInfo *gdk_info;
#endif
  GList *tmp_list;
  gchar *mode = NULL;		/* Quiet gcc */
  int i;

  gdk_info = NULL;
  if (device_info->is_present)
    {
      /* gdk_input_list_devices returns an internal list, so we shouldn't
	 free it afterwards */
#if GTK_MAJOR_VERSION > 1
  tmp_list = gdk_devices_list ();
#else
  tmp_list = gdk_input_list_devices ();
#endif
      while (tmp_list)
	{
#if GTK_MAJOR_VERSION > 1
	  GdkDevice *info = (GdkDevice *)tmp_list->data;
	  if (info == device_info->device)
#else
	  GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;
	  if (info->deviceid == device_info->device)
#endif	  
	    {
	      gdk_info = info;
	      break;
	    }
	  tmp_list = tmp_list->next;
	}
    }
  
  fprintf(fp, "(device \"%s\"",device_info->name);

  switch (gdk_info ? gdk_info->mode : device_info->mode)
    {
    case GDK_MODE_DISABLED:
      mode = "disabled";
      break;
    case GDK_MODE_SCREEN:
      mode = "screen";
      break;
    case GDK_MODE_WINDOW:
      mode = "window";
      break;
    }
  
  fprintf(fp, "\n        (mode %s)",mode);

  fprintf(fp, "\n        (axes %d",gdk_info ? gdk_info->num_axes : device_info->num_axes);

  for (i=0; i<(gdk_info ? gdk_info->num_axes : device_info->num_axes); i++)
    {
      gchar *axis_type = NULL;	/* Quiet gcc */
      
      switch (gdk_info ? gdk_info->axes[i]
#if GTK_MAJOR_VERSION > 1
                                           .use
#endif
                                                : device_info->axes[i])
	{
	case GDK_AXIS_IGNORE:
	  axis_type = "ignore";
	  break;
	case GDK_AXIS_X:
	  axis_type = "x";
	  break;
	case GDK_AXIS_Y:
	  axis_type = "y";
	  break;
	case GDK_AXIS_PRESSURE:
	  axis_type = "pressure";
	  break;
	case GDK_AXIS_XTILT:
	  axis_type = "xtilt";
	  break;
	case GDK_AXIS_YTILT:
	  axis_type = "ytilt";
	  break;
#if GTK_MAJOR_VERSION > 1
	case GDK_AXIS_WHEEL:
	  axis_type = "wheel";
	  break;
#endif
	default:
	  axis_type = "---";
	  break;
	}
      fprintf(fp, " %s",axis_type);
    }
  fprintf(fp,")");

  fprintf(fp, "\n        (keys %d", gdk_info ? gdk_info->num_keys : device_info->num_keys);

  for (i=0; i<(gdk_info ? gdk_info->num_keys : device_info->num_keys); i++)
    {
      GdkModifierType modifiers = gdk_info ? gdk_info->keys[i].modifiers :
	device_info->keys[i].modifiers;
      guint keyval = gdk_info ? gdk_info->keys[i].keyval :
	device_info->keys[i].keyval;
	
      if (keyval)
	{
	  /* FIXME: integrate this back with menus_install_accelerator */
	  char accel[64];
	  char t2[2];

	  accel[0] = '\0';
	  if (modifiers & GDK_CONTROL_MASK)
	    strcat (accel, "<control>");
	  if (modifiers & GDK_SHIFT_MASK)
	    strcat (accel, "<shift>");
	  if (modifiers & GDK_MOD1_MASK)
	    strcat (accel, "<alt>");
	  
	  t2[0] = keyval;
	  t2[1] = '\0';
	  strcat (accel, t2);
	  fprintf (fp, " \"%s\"",accel);
	}
      else
	fprintf (fp, " \"\"");
    }
  fprintf(fp,")");

  if (device_info->is_init)
    {
      fprintf(fp, "\n        (brush \"%s\")",device_info->brush->name);
      fprintf(fp, "\n        (tool \"%s\")",
	      tool_info[device_info->tool].tool_name);
      fprintf(fp, "\n        (foreground %d %d %d)",
	      device_info->foreground[0],
	      device_info->foreground[1],
	      device_info->foreground[2]);
    }
  fprintf(fp,")\n");
  
}

static void
devices_write_rc (void)
{
  char *gimp_dir;
  char filename[512];
  FILE *fp;

  devices_save_current_info();

  gimp_dir = gimp_directory ();
  if ('\000' != gimp_dir[0])
    {
      sprintf (filename, "%s/devicerc", gimp_dir);

      fp = fopen (filename, "wb");
      if (!fp)
	return;

      g_list_foreach (devices_info, (GFunc)devices_write_rc_device, fp);

      fclose (fp);
    }
}

void 
create_device_status (void)
{
  GtkWidget *label;
  GList *tmp_list;
  DeviceInfo *device_info;
  int i;
  
  if (deviceD == NULL)
    {
      deviceD = g_new (DeviceInfoDialog, 1);
      deviceD->shell = gtk_dialog_new ();

      gtk_window_set_title (GTK_WINDOW(deviceD->shell), _("Device Status"));
      gtk_window_set_policy (GTK_WINDOW (deviceD->shell), FALSE, FALSE, TRUE);

      deviceD->num_devices = 0;
      tmp_list = devices_info;
      while (tmp_list)
	{
	  if (((DeviceInfo *)tmp_list->data)->is_present)
	    deviceD->num_devices++;
	  tmp_list = tmp_list->next;
	}

      deviceD->table = gtk_table_new (deviceD->num_devices, 4, FALSE);
      gtk_container_border_width (GTK_CONTAINER(deviceD->table), 3);
      gtk_container_add (GTK_CONTAINER(GTK_DIALOG(deviceD->shell)->vbox),
			 deviceD->table);
      gtk_widget_realize (deviceD->table);
      gtk_widget_show (deviceD->table);

#if GTK_MAJOR_VERSION > 1
      deviceD->ids = g_new (GdkDevice*, deviceD->num_devices);
#else
      deviceD->ids = g_new (guint32, deviceD->num_devices);
#endif
      deviceD->frames = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->tools = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->colors = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->brushes = g_new (GtkWidget *, deviceD->num_devices);

      tmp_list = devices_info;
      i=0;
      while (tmp_list)
	{
	  if (!((DeviceInfo *)tmp_list->data)->is_present)
	    {
	      tmp_list = tmp_list->next;
	      i++;
	      continue;
	    }
	  
	  device_info = (DeviceInfo *)tmp_list->data;

	  deviceD->ids[i] = device_info->device;

	  deviceD->frames[i] = gtk_frame_new (NULL);
	  
	  gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), GTK_SHADOW_OUT);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->frames[i],
			    0, 1, i, i+1,
			    GTK_FILL, GTK_FILL, 2, 0);

	  label = gtk_label_new (device_info->name);
	  gtk_misc_set_padding (GTK_MISC(label), 2, 0);
	  gtk_container_add (GTK_CONTAINER(deviceD->frames[i]), label);
	  gtk_widget_show(label);

	  deviceD->tools[i] = gtk_pixmap_new (create_tool_pixmap(deviceD->table,
								 RECT_SELECT),
					      NULL);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->tools[i],
			    1, 2, i, i+1,
			    0, 0, 2, 2);

	  deviceD->colors[i] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (deviceD->colors[i]), 20, 20);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->colors[i],
			    2, 3, i, i+1,
			    0, 0, 2, 2);

	  deviceD->brushes[i] = gtk_label_new ("");
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->brushes[i],
			    3, 4, i, i+1,
			    0, 0, 2, 2);

	  if (device_info->is_present)
	    device_status_update (device_info->device);

	  
	  tmp_list = tmp_list->next;
	  i++;
	}

      action_items[0].label = _(action_items[0].label);
      action_items[1].label = _(action_items[1].label);
      action_items[0].user_data = deviceD->shell;
      build_action_area (GTK_DIALOG (deviceD->shell), action_items, 2, 0);

#if GTK_MAJOR_VERSION > 1
      deviceD->current = gdk_devices_list()->data; /* random, but doesn't matter */
#else
      deviceD->current = 0xffffffff; /* random, but doesn't matter */
#endif
      device_status_update_current ();

      gtk_widget_show (deviceD->shell);

      gtk_signal_connect (GTK_OBJECT (deviceD->shell), "destroy",
			  GTK_SIGNAL_FUNC(device_status_destroy_callback),
			  NULL);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED(deviceD->shell))
	gtk_widget_show(deviceD->shell);
      else
	gdk_window_raise(deviceD->shell->window);
    }
}

static void
device_status_destroy_callback (void)
{
  g_free(deviceD->ids);
  g_free(deviceD->frames);
  g_free(deviceD->tools);
  g_free(deviceD->colors);
  g_free(deviceD->brushes);

  g_free(deviceD);
  deviceD = NULL;
}

static void
devices_close_callback (GtkWidget *w,
			gpointer data)
{
  gtk_widget_hide (GTK_WIDGET(data));
}

static void
device_status_update_current ()
{
  int i;

  if (deviceD)
    {
      for (i=0; i<deviceD->num_devices; i++)
	{
	  if (deviceD->ids[i] == deviceD->current)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_OUT);
	  else if (
#if GTK_MAJOR_VERSION < 2
                   CAST(int)
#endif
                             deviceD->ids[i] == current_device)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_IN);
	}
      deviceD->current = current_device;
    }
}

void 
device_status_update (
#if GTK_MAJOR_VERSION > 1
               GdkDevice*
#else
               guint32
#endif
                                deviceid)
{
  int i, j;
  guchar buffer[20*3];
  GList *tmp_list;
  DeviceInfo *device_info;
#if GTK_MAJOR_VERSION > 1
               GdkDevice
#else
               GdkDeviceInfo
#endif
  *gdk_info;

  if (deviceD && !suppress_update)
    {
      if (
#if GTK_MAJOR_VERSION < 2
          CAST(int)
#endif
                    deviceid == current_device)
	{
	  devices_save_current_info();
	}
      
      tmp_list = devices_info;
      while (tmp_list)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  
	  if (device_info->device == deviceid)
	    break;

	  tmp_list = tmp_list->next;
	}
      
      g_return_if_fail (tmp_list != NULL);

#if GTK_MAJOR_VERSION > 1
      tmp_list = gdk_devices_list();
#else
      tmp_list = gdk_input_list_devices();
#endif

      while (tmp_list)
	{
#if GTK_MAJOR_VERSION > 1
	  gdk_info = (GdkDevice *)tmp_list->data;
	  if (gdk_info == device_info->device)
#else
	  gdk_info = (GdkDeviceInfo *)tmp_list->data;
	  if (gdk_info->deviceid == device_info->device)
#endif	  
	    break;

	  tmp_list = tmp_list->next;
	}
      
      g_return_if_fail (tmp_list != NULL);
      
      for (i=0; i<deviceD->num_devices; i++)
	{
	  if (deviceD->ids[i] == deviceid)
	    break;
	}
      
      g_return_if_fail (i<deviceD->num_devices);
      
      if (gdk_info->mode == GDK_MODE_DISABLED)
	{
	  gtk_widget_hide (deviceD->frames[i]);
	  gtk_widget_hide (deviceD->tools[i]);
	  gtk_widget_hide (deviceD->colors[i]);
	  gtk_widget_hide (deviceD->brushes[i]);
	}
      else
	{
	  gtk_widget_show (deviceD->frames[i]);

	  gtk_pixmap_set (GTK_PIXMAP (deviceD->tools[i]),
			  create_tool_pixmap (deviceD->table, 
					      device_info->tool),
			  NULL);
	  gtk_widget_show (deviceD->tools[i]);
	  
	  for (j=0;j<20*3;j+=3)
	    {
	      buffer[j] = device_info->foreground[0];
	      buffer[j+1] = device_info->foreground[1];
	      buffer[j+2] = device_info->foreground[2];
	    }
	  
	  for (j=0;j<20;j++)
	    gtk_preview_draw_row (GTK_PREVIEW(deviceD->colors[i]), buffer, 
				  0, j, 20);
	  gtk_widget_draw (deviceD->colors[i], NULL);
	  gtk_widget_show (deviceD->colors[i]);

	  if (device_info->brush)
	    gtk_label_set (GTK_LABEL (deviceD->brushes[i]), device_info->brush->name);
	  else
	    gtk_label_set (GTK_LABEL (deviceD->brushes[i]), "");
	  gtk_widget_show (deviceD->brushes[i]);
	}
    }
}
