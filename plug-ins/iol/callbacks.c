/* callbacks.c - GTK+ callbacks for the Film Gimp IOL plug-in
 *
 * Copyright (C) 2003 Sean Ridenour.
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
 *
 * Read the file COPYING for the complete licensing terms.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "iol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "fg.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"


GtkText *script_text;
GdkFont *script_font;

char default_dir[256];
int stay_open;

void on_load_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *load_file_select;

	load_file_select = create_load_file_select();
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(load_file_select),default_dir);
	gtk_widget_show(load_file_select);
}

void on_save_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *save_file_select;

	save_file_select = create_save_file_selection();
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(save_file_select),default_dir);
	gtk_widget_show(save_file_select);
}

void on_go_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *window;
	gchar *text;

	window = (GtkWidget *)user_data;

	gtk_text_freeze(script_text);

	text = gtk_editable_get_chars(GTK_EDITABLE(script_text),0,-1);

	input_size = strlen((char *)text);
	iol_input = (char *)malloc(input_size + 1);
	if(iol_input == NULL) {
		dialog_status = 0;
		fprintf(stderr,"IOL error: callbacks.c: insufficient memory\n");
		gtk_widget_destroy(window);
		gtk_main_quit();
		return;
	}

	strncpy(iol_input, text, input_size);
	iol_input[input_size] = 0;
	iol_input_ptr = iol_input;
	iol_input_lim = (unsigned int)iol_input + input_size;

	g_free(text);

	gtk_text_thaw(script_text);

	if(!stay_open) {
		gtk_widget_destroy(window);
		gtk_main_quit();
	}

	if(iol_process())
		dialog_status = 0;
	else
		dialog_status = 1;
}

gboolean
on_iol_window_delete_event(GtkWidget *widget, GdkEvent *event,
				gpointer user_data)
{
	gtk_main_quit();
	return FALSE;
}

void on_load_ok_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *load_select;
	FILE *infile;
	char *filename;
	char text[512];

	load_select = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_FILE_SELECTION);
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(load_select));
	gtk_widget_hide(load_select);

	infile = fopen(filename,"r");
	if(infile == NULL) {
		fprintf(stderr,"IOL: can't open %s\n",filename);
		gtk_widget_destroy(load_select);
		return;
	}

	gtk_widget_destroy(load_select);

	gtk_text_freeze(script_text);

	/* since GTK+ (lamely) has no gtk_text_empty() or whatever,
	   so we have to revert to kludges :( */
	gtk_text_set_point(script_text,0);
	gtk_text_forward_delete(script_text,gtk_text_get_length(script_text));

	while(fgets(text,512,infile) != NULL)
		gtk_text_insert(script_text,script_font,NULL,NULL,text,strlen(text));

	fclose(infile);

	gtk_text_set_point(script_text,0);
	gtk_text_thaw(script_text);
}

void on_load_cancel_button_clicked(GtkButton * button, gpointer user_data)
{
	GtkWidget *load_select;

	load_select = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_FILE_SELECTION);
	gtk_widget_destroy(load_select);
}

void on_save_ok_button_clicked(GtkButton * button, gpointer user_data)
{
	GtkWidget *save_select;
	FILE *outfile;
	gchar *file, *text;

	save_select = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_FILE_SELECTION);
	file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(save_select));
	gtk_widget_hide(save_select);

	outfile = fopen(file,"w");
	if(outfile == NULL) {
		fprintf(stderr,"IOL: couldn't open %s\n",file);
		gtk_widget_destroy(save_select);
		return;
	}

	gtk_widget_destroy(save_select);

	gtk_text_freeze(script_text);

	text = gtk_editable_get_chars(GTK_EDITABLE(script_text),0,-1);
	fprintf(outfile,"%s",text);
	fclose(outfile);
	g_free(text);

	gtk_text_thaw(script_text);
}

void on_save_cancel_button_clicked(GtkButton * button, gpointer user_data)
{
	GtkWidget *save_select;
	save_select = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_FILE_SELECTION);
	gtk_widget_destroy(save_select);
}

void on_script_text_changed(GtkEditable *editable, gpointer user_data)
{

}

void on_iol_window_show(GtkWidget *widget, gpointer user_data)
{
	char *hdir;
	char string[] = { "input red green blue alpha;\n\n#put your program here\n\noutput red green blue alpha;\n" };
	script_text = GTK_TEXT(user_data);

	script_font = gdk_font_load("-*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*");

	gtk_text_freeze(script_text);
	gtk_text_insert(script_text,script_font,NULL,NULL,string,strlen(string));
	gtk_text_thaw(script_text);

	stay_open = 0;

	hdir = getenv("IOL_SCRIPTS");
	if(hdir == NULL) {
		fprintf(stderr,"IOL: $IOL_SCRIPTS environment variable not set, checking $HOME...\n");
		hdir = getenv("HOME");
		if(hdir == NULL) {
			fprintf(stderr,"IOL: $HOME environment variable not set\n");
			strcpy(default_dir,"/usr");
		} else {
			strncpy(default_dir,hdir,256);
			strncat(default_dir,"/.cinepaint/iol/",256);
			fprintf(stderr,"IOL: using %s as the default directory for scripts\n",default_dir);
		}
	} else {
		strncpy(default_dir,hdir,256);
		strncat(default_dir,"/",256);	/* the file selection dialog needs
						   it to end with a "/" so that it
						   will know it's a directory we
						   want */
	}
}

void on_help_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *help;

	help = create_iol_help_window();
	gtk_widget_show(help);
}

void on_keepopen_check_button_toggled(GtkToggleButton *togglebutton,
					gpointer user_data)
{
	if(!stay_open)
		stay_open = 1;
	else
		stay_open = 0;
}

void on_help_close_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *help;

	help = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_WINDOW);
	gtk_widget_destroy(help);
}

