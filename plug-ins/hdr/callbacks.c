#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>

#include "callbacks.h"
#include "interface.h"
#include "lib/plugin_main.h"

extern int status;
extern int expadj;
extern gint32 load_image(gchar * filename, gint32 image_id, gint32 drawable_id);
extern GtkWidget* load_dialog;

void
on_Load_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkOptionMenu *optionmenu;
	GtkWidget *menu;
	GtkWidget *menuitem;
	int opt;
	gtk_widget_hide(load_dialog);
	optionmenu = GTK_OPTION_MENU(gtk_object_get_data (GTK_OBJECT (load_dialog),"optionmenu1"));
	menu = gtk_option_menu_get_menu (optionmenu);
	menuitem =  gtk_menu_get_active (GTK_MENU(menu));
	opt = g_list_index (GTK_MENU_SHELL (menu)->children,menuitem);
	switch(opt)
	{
		case 0:
			expadj = 0;
			break;
		case 1:
			expadj = -1;
			break;
		case 2:
			expadj = -2;
			break;
		case 3:
			expadj = -3;
			break;
		case 4:
			expadj = -4;
			break;
		case 5:
			expadj = -5;
			break;
		case 6:
			expadj = -6;
			break;
	}
	gtk_main_quit();
	status = STATUS_SUCCESS;
}


void
on_Cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(load_dialog);
	gtk_main_quit();
	status = STATUS_PASS_THROUGH;
}

