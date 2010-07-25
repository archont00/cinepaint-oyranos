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
#include "appenv.h"
#include "buildmenu.h"
#include "interface.h"

GtkWidget *
build_menu (MenuItem            *items,
	    GtkAccelGroup       *accel_group)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  while (items->label)
    {
      if (items->label[0] == '-')
	{
	  menu_item = gtk_menu_item_new ();
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);
	}
      else
	{
	  menu_item = gtk_menu_item_new_with_label (items->label);
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);

	  if (items->accelerator_key && accel_group)
	    gtk_widget_add_accelerator (menu_item,
					"activate",
					accel_group,
					items->accelerator_key,
					items->accelerator_mods,
					GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	}

      if (items->callback)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) items->callback,
			    items->user_data);

      if (items->subitems)
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), build_menu (items->subitems, accel_group));

      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
    }

  return menu;
}

int
get_menu_num_items(MenuItem *items)
{ int i = 0;
  while (items->label)
  { ++i; ++items;
  }
  return i;
}

int
get_menu_item_current(MenuItem *items, GtkOptionMenu *option_menu)
{
    GtkMenu *m = GTK_MENU(gtk_option_menu_get_menu(option_menu));
    GtkWidget *w = gtk_menu_get_active(m);
    int pos = -1, i;

    i = 0;
    while (items->label)
    { if (w == items->widget)
        pos = i;
      ++i; ++items;
    }
  return pos;
}

int
update_color_menu_items              (MenuItem      *items,
                                      GtkOptionMenu *option_menu,
                                      const char**   color_space_channel_names)
{
    MenuItem *item_list = items;
    GtkMenu *m = GTK_MENU(gtk_option_menu_get_menu(option_menu));
    GtkWidget *w = gtk_menu_get_active(m);
    GtkWidget *menu = 0;
    int pos = -1, i;

    i = 0;
    while (items->label)
    { if (w == items->widget)
        pos = i;
      if(i && i <= 4)
        items->label = (char*)color_space_channel_names[i-1];
      ++i; ++items;
    }

    menu = build_menu (item_list, NULL);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  return pos;
}

