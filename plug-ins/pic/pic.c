/* PIC Plugins v 0.1
 * Copyright (C) 1998 Halfdan Ingvarsson
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
 
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "readpic.h"
#include "writepic.h"

/* Local billops */
static void	query(void);
static void run(char *name, int nParams, GParam *param, int *nRetVal, GParam **retVal);


GPlugInInfo PLUG_IN_INFO = {
	NULL,		/* init_proc */
	NULL,		/* quit_proc */
	query,		/* query_proc */
	run			/* run_proc */
};

MAIN()


static void query(void)
{
	static GParamDef	loadArgs[] = {
		{PARAM_INT32,	"run_mode", "Interactive, non-interactive"}, 
		{PARAM_STRING,	"filename", "The name of the file to load"}, 
		{PARAM_STRING,	"raw_filename", "The name entered"}
	};
	static GParamDef loadRetVals[] = {
		{PARAM_IMAGE,	"image", "Output image"}
	};
	static int nLoadArgs = sizeof(loadArgs) / sizeof(loadArgs[0]);
	static int nLoadRetVals = sizeof(loadRetVals) / sizeof(loadRetVals[0]);
	
	static GParamDef saveArgs[] = {
		{PARAM_INT32,	"run_mode", "Interactive, non-interactive"}, 
		{PARAM_IMAGE,	"image", "Input image"}, 
		{PARAM_DRAWABLE, "drawable", "Drawable to save"}, 
		{PARAM_STRING, "filename", "The name of the file to save the image in"}, 
		{PARAM_STRING, "raw_filename", "The name of the file to save the image in"}, 
		{PARAM_INT32, "rle", "Enable RLE compression" }, 
		{PARAM_INT32, "alpha", "Save alpha channel"}
	};
	static int nSaveArgs = sizeof(saveArgs) / sizeof(saveArgs[0]);
	
	gimp_install_procedure("file_pic_load", 
						   "Loads files of the Softimage(R) format", 
						   "This plugin loads in image file in Softimage(R) PIC format. It also sets and alpha channel if the file includes one.", 
						   "Halfdan Ingvarsson", 
						   "Halfdan Ingvarsson", 
						   "1998-1999", 
						   "<Load>/Softimage PIC", 
						   NULL, 
						   PROC_PLUG_IN, 
						   nLoadArgs, nLoadRetVals, 
						   loadArgs, loadRetVals);
	gimp_install_procedure("file_pic_save", 
						   "Saves files in the Softimage(R) format", 
						   "No help available", 
						   "Halfdan Ingvarsson", 
						   "Halfdan Ingvarsson", 
						   "1998-1999", 
						   "<Save>/Softimage PIC", 
						   "RGB*", 
						   PROC_PLUG_IN, 
						   nSaveArgs, 0, 
						   saveArgs, NULL);
	gimp_register_magic_load_handler("file_pic_load", "pic", "<Load>/Softimage PIC", 
		"0,long,0x5380F634");
	gimp_register_save_handler("file_pic_save", "pic", "<Save>/Softimage PIC");
}

static gint saveDialog(PicWriteParam *param);

static void run(char *name,
				int nParam, GParam *param,
				int *nRetVals, GParam **retVals)
{
	static GParam	values[2];
	GStatusType		status = STATUS_SUCCESS;
	GRunModeType	runMode;
	gint32			imageID;
	PicWriteParam	picWriteParam;
	
	runMode = (GRunModeType)param[0].data.d_int32;
	
	*nRetVals = 1;
	*retVals = values;
	
	// Set default return status as an error
	values[0].type = PARAM_STATUS;
	values[0].data.d_status = STATUS_CALLING_ERROR;
	
	picWriteParam.rle = 1;
	picWriteParam.alpha = 1;
	
	if(strcmp(name, "file_pic_load") == 0) {
		if(readPicImage(param[1].data.d_string, &imageID)) {
			*nRetVals = 2;
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].type = PARAM_IMAGE;
			values[1].data.d_image = imageID;
		} else
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
	else if(strcmp(name, "file_pic_save") == 0) {
		switch(runMode) {
		case RUN_INTERACTIVE:
			// Get the previously used parameters
			gimp_get_data("file_pic_save", &picWriteParam);
			
			// Set up the save dialog box
			if(!saveDialog(&picWriteParam))
				return;
			break;
		case RUN_NONINTERACTIVE:
			// Check if we have the required parameters
			if(nParam >= 6)
				picWriteParam.rle = (param[5].data.d_int32)?(TRUE):(FALSE);
			if(nParam == 7)
				picWriteParam.alpha = (param[6].data.d_int32)?(TRUE):(FALSE);
			break;
		case RUN_WITH_LAST_VALS:
			gimp_get_data("file_pic_save", &picWriteParam);
			break;
		default:
			break;
		}
		
		*nRetVals = 1;
		if(writePicImage(param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32, &picWriteParam)) {
			gimp_set_data("file_pic_save", &picWriteParam, sizeof(picWriteParam));
			
			values[0].data.d_status = STATUS_SUCCESS;
		} else
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
	} else
		g_assert(FALSE);
}

static gint		glb_run;

static void saveCloseCB(GtkWidget *widget, gpointer data)
{
	widget = widget;
	data = data;
	gtk_main_quit();
}

static void saveOkCB(GtkWidget *widget, gpointer data)
{
	glb_run = TRUE;
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void saveToggleCB(GtkWidget *widget, gpointer data)
{
	int *toggle_val;

	toggle_val = (gint32 *) data;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		*toggle_val = TRUE;
	else
		*toggle_val = FALSE;
}

static gint saveDialog(PicWriteParam *param)
{
	GtkWidget	*dlg;
	GtkWidget	*button;
	GtkWidget	*rle, *alpha;
	GtkWidget	*frame;
	GtkWidget	*vbox;
	gchar		**argv;
	gint		argc;
	
	glb_run = FALSE;
	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("save");
	
	gtk_init (&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());
	
	dlg = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dlg), "Save as Softimage PIC");
	gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect (GTK_OBJECT (dlg), "destroy", (GtkSignalFunc) saveCloseCB,
	NULL);
	
	/*  Action area  */
	button = gtk_button_new_with_label ("OK");
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", (GtkSignalFunc) saveOkCB, dlg);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);
	
	button = gtk_button_new_with_label ("Cancel");
//	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object (GTK_OBJECT (button), "clicked", (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT (dlg));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show (button);
	
	// PIC Option box
	frame = gtk_frame_new ("PIC Options");
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_border_width (GTK_CONTAINER (frame), 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	// RLE
	rle = gtk_check_button_new_with_label ("RLE compression");
	gtk_box_pack_start (GTK_BOX (vbox), rle, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (rle), "toggled", (GtkSignalFunc) saveToggleCB, &param->rle);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rle), param->rle);
	gtk_widget_show (rle);
	
	// RLE
	alpha = gtk_check_button_new_with_label ("Save with alpha channel");
	gtk_box_pack_start(GTK_BOX (vbox), alpha, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT (alpha), "toggled", (GtkSignalFunc) saveToggleCB, &param->alpha);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(alpha), param->alpha);
	gtk_widget_show(alpha);

	gtk_widget_show (vbox);
	gtk_widget_show (frame);
	
	gtk_widget_show (dlg);
	
	gtk_main ();
	gdk_flush ();
	
	return glb_run;
}
