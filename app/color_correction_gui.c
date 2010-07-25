#include <gdk/gdkkeysyms.h>

#include "../lib/version.h"
#include "../lib/compat.h"
#include "color_correction.h"
#include "color_correction_gui.h"
#include "image_map.h"
#include "look_profile.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "rc.h"


#define SQR(x) ((x) * (x))
#define X 0
#define Y 1

/* the initial radius of the wheel */
#define WHEEL_RADIUS  90
/* the width of the lightness slider 
   the initial height is WHEEL_RADIUS*2 */
#define LSLIDER_WIDTH  15

#define WHEEL_EVENT_MASK   GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
                           GDK_BUTTON_MOTION_MASK



/******************************
 *                            *
 *     PRIVATE INTERFACE      *
 *                            *
 ******************************/


/****
 *  PRIVATE INTERFACE 1/5: THE MAIN DIALOG
 ****/

/* the data of the dialog */
typedef struct _ColorCorrectionGui
{   GtkWidget *shell;
    GtkWidget *color_wheels[3];
    GtkWidget *lsliders[3]; 
    GtkWidget *spins[3][3];                /* first dimension is range (sh,mi,hi), 
				              second is color attribute (h,l,s) */
    gboolean active;                       /* is the whole wheels tab active (toggle on tab) */  

    ColorCorrectionSettings *settings;     /* the entered settings */
    GDisplay *display;                     /* the associated display */
    ImageMap image_map;  
} _ColorCorrectionGui;


/* data for the spins for hue, sat and lightness */
typedef struct _ColorCorrectionGuiSpin
{   ColorCorrectionLightnessRange range;      /* which of the three wheels */
    ColorCorrectionColorAttribute attribute;  /* which attribute */  
    gfloat old_value;                          /* previous value, to check for change in focus-out 
                                                  and key-released to avoid unnecessary refreshs */
} _ColorCorrectionGuiSpin;


static void _color_correction_gui_create (_ColorCorrectionGui *data);
/* free the data associated with the dialog */
static void _color_correction_gui_free (_ColorCorrectionGui *data);
/* update the preview */
static void _color_correction_gui_preview(_ColorCorrectionGui *data);
/* update handles or spins from  _ColorCorrectionGui.values
   (to synchronize them after one has changed),
   range parameter specifies which one to update */
static void _color_correction_gui_update_wheel_handle (_ColorCorrectionGui *dialog, 
						       ColorCorrectionLightnessRange range);
static void _color_correction_gui_update_lslider_handle (_ColorCorrectionGui *dialog, 
							 ColorCorrectionLightnessRange range);
static void _color_correction_gui_update_spins (_ColorCorrectionGui *data, 
						ColorCorrectionLightnessRange range);
/* all the callbacks */
static void _color_correction_gui_ok_cb (GtkWidget *widget,
					 _ColorCorrectionGui *data);
static void _color_correction_gui_cancel_cb (GtkWidget *widget,
					     _ColorCorrectionGui *data);
static gint _color_correction_gui_delete_cb (GtkWidget *widget,
					     GdkEvent *event,
					     gpointer user_data);
static void _color_correction_gui_wheels_tab_toggled_cb (GtkWidget *widget, 
							 _ColorCorrectionGui *data);
static gint _color_correction_gui_wheel_events_cb (GtkWidget *widget,
					       GdkEvent  *event,
					       _ColorCorrectionGui *data);
static gint _color_correction_gui_lslider_events_cb (GtkWidget *widget,
						     GdkEvent  *event,
						     _ColorCorrectionGui *data);
static void _color_correction_gui_spin_changed_cb (GtkWidget *widget, 
						   _ColorCorrectionGui *data);
static gint _color_correction_gui_spin_events_cb (GtkWidget *widget, 
						  GdkEvent *event, 
						  _ColorCorrectionGui *data);



/****
 *  PRIVATE INTERFACE 2/5: THE COLOR WHEELS
 ****/

/* screen labels for the wheels (just so they can be drawn in a for loop) */
const char *color_correction_wheel_labels[3] = {"Sh ","Mid","Hi "};

/* data for color wheel */
typedef struct _ColorCorrectionGuiWheel
{   ColorCorrectionLightnessRange range;      /* which of the three wheels */
    GdkPixmap *pixmap;         /* contains only the wheel, necessary
				  to erase/move the handle */
    GdkPixmap *display_buffer; /* contains the wheel+handle as
				  visible on screen */
    GdkGC *gc;                 /* used for drawing the wheel */	    
    gint width, height;        /* width and height of drawing area,
				  so we know the former values when
				  it is resized */
    gint x, y;                 /* top-left corner box round circle */
    gint radius;               /* circle radius */

    gint position[2];          /* (x,y) of knob centre */
    gint mouse_offset[2];      /* offset of mouse cursor from handle centre,
				  important for smooth dragging, cause
				  mouse can be anywhere in active area of handle */
    gboolean active;           /* the wheel is active (toggle in upper-left
				  corner) */
    gboolean dragging;         /* the handle is being dragged 
				  (used on GDK_MOTION_NOTIFY) */
} _ColorCorrectionGuiWheel;

/* frees the memory for the wheel data */
static void _color_correction_gui_free_wheel(gpointer user_data);
/* draws the wheel (without handle) onto its pixmap,
   lightness of the wheel, used in HLS->RGB conversion,
   makes a copy into display_buffer */
static void _color_correction_gui_draw_wheel (GtkWidget *drawing_area, gfloat lightness);
/* draws or erases the lslider handle onto its display_buffer and
   refreshes the drawing area */
static void _color_correction_gui_draw_wheel_handle (GtkWidget *drawing_area, _ColorCorrectionGuiWheel *data);
static void _color_correction_gui_erase_wheel_handle (GtkWidget *drawing_area, _ColorCorrectionGuiWheel *data);
/* draws the wheel toggle (upper-left corner) onto its display_buffer, the state
   is taken from _ColorCorrectionGuiWheel.active */
static void _color_correction_gui_draw_wheel_toggle(GtkWidget *drawing_area);
/* auxiliary 1/2: calculates hue and saturation from a position on the wheel,
   hue is 0.-360., saturation is 0.-1., returns FALSE if position is outside wheel,
   TRUE otherwise */
static gboolean _color_correction_gui_wheel_pos_to_hs (gint     x,  gint     y,
						  gint  cx, gint cy,
						  gfloat *h,  gfloat *s);
/* auxiliary 2/2: the inverse function,
   returns FALSE, if h and s are not within limits 0.-360., and 0.-1. */
static gboolean _color_correction_gui_wheel_hs_to_pos (gfloat h,  gfloat s,
						  gint  cx, gint cy,
						  gint*     x,  gint* y);




/****
 *  PRIVATE INTERFACE 3/5: THE LIGHTNESS SLIDERS
 ****/

/* data for lightness slider */
typedef struct _ColorCorrectionGuiLSlider
{   ColorCorrectionLightnessRange range;      /* which of the three sliders */
    GdkPixmap *pixmap;         /* contains only the slider/gradient, necessary
				  to erase/move the handle */
    GdkPixmap *display_buffer; /* contains the slider+handle as 
				  visible on screen */
    GdkGC *gc;                 /* used for drawing the wheel */

    gint height;               /* height of drawing_area, so we know
				  the former value when resized,
				  width is always LSLIDER_WIDTH */
    gint position;             /* positions of the handle */
    gboolean dragging;         /* the handle is being dragged 
				  (used at GDK_MOTION_NOTIFY) */
} _ColorCorrectionGuiLSlider;

/* frees the memory for the slider data */
static void _color_correction_gui_free_lslider(gpointer user_data);
/* draws the slider (without handle) onto its pixmap, makes a copy
   into display_buffer */
static void _color_correction_gui_draw_lslider(GtkWidget *drawing_area);
/* draws or erases the lslider handle onto its display_buffer and
   refreshes the drawing area */
static void _color_correction_gui_draw_lslider_handle(GtkWidget *drawing_area, _ColorCorrectionGuiLSlider *data);
static void _color_correction_gui_erase_lslider_handle(GtkWidget *drawing_area, _ColorCorrectionGuiLSlider *data);



/****
 *  PRIVATE INTERFACE 4/5: THE OPTIONS DIALOG
 ****/

typedef struct _ColorCorrectionGuiOptions
{   GtkWidget *shell;
    GtkWidget *preserve;
    GtkWidget *model_frame;
    GtkWidget *use_hls;
    GtkWidget *use_lab;
    GtkWidget *use_bernstein;
    GtkWidget *use_gaussian;
    
    _ColorCorrectionGui *parent;  /* the data of the main dialog this belongs to */ 
} _ColorCorrectionGuiOptions;

/* callbacks */
static void _color_correction_gui_options_create (_ColorCorrectionGui *parent);
static void _color_correction_gui_options_ok_cb(_ColorCorrectionGuiOptions *data);



/****
 *  PRIVATE INTERFACE 5/5: THE SAVE PROFILE DIALOG
 ****/
/* data for the dialog for saving a profile */
typedef struct _ColorCorrectionGuiSaveProfile
{   GtkWidget *shell;
    GtkWidget *sample_depth_entry;     /* the entry to enter number of sample points */
 
    _ColorCorrectionGui *parent; /* the data of the main dialog this belongs to */ 
    GImage *image;
} _ColorCorrectionGuiSaveProfile;

/* callback */
static void _color_correction_gui_save_profile_create (_ColorCorrectionGui *parent);
static void _color_correction_gui_save_profile_ok_cb(_ColorCorrectionGuiSaveProfile *data);






/***************************
 *                         *
 *     IMPLEMENTATION      *
 *                         *
 ***************************/


/******
 * IMPLEMENTATION 1/5: THE MAIN DIALOG
 ******/

void 
color_correction_gui (GDisplay *display)
{  _ColorCorrectionGui *data = 0;
   if (! drawable_color (gimage_active_drawable (display->gimage)))
    {   g_message ("Color correction operates only on RGB color drawables.");
        return;
    }

    if( tag_precision (drawable_tag (gimage_active_drawable(display->gimage))) != PRECISION_U8)
    {   g_message ("Color correction operates only on 8 bit RGB color drawables.");
        return;
    }

    data = g_new(_ColorCorrectionGui, 1);
    data->display = display;
    data->active=TRUE;
    data->image_map = image_map_create (data->display, 
					gimage_active_drawable(data->display->gimage));  

    data->settings = g_new(ColorCorrectionSettings, 1);
    data->settings->model = LAB;
    data->settings->transfer = GAUSSIAN;
    data->settings->preserve_lightness = TRUE;
    data->settings->lightness_modified = 0;

    color_correction_init_gaussian_transfer_LUTs ();
    color_correction_init_lab_transforms(display->gimage);

    _color_correction_gui_create(data); 
}
    

static void
_color_correction_gui_create (_ColorCorrectionGui *data)
{   GtkWidget *vbox = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *mainhbox = NULL;
    GtkWidget *buttonbox = NULL;
    GtkWidget *button = NULL;
    GtkWidget *label =NULL;
    GtkWidget *notebook = NULL;
    GtkWidget *frame = NULL;
	GtkObject *hue_adjustment;
	GtkObject *saturation_adjustment;
	GtkObject *lightness_adjustment;
	_ColorCorrectionGuiSpin *spin_data;
	_ColorCorrectionGuiWheel *wheel_data;
	_ColorCorrectionGuiLSlider *lslider_data;
    int i;
       

    /*** MAIN SHELL AND NOTEBOOK */
    data->shell = gtk_dialog_new ();
    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "color_correction", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), "Color Correction");

    gtk_signal_connect (GTK_OBJECT (data->shell), "delete_event",
			GTK_SIGNAL_FUNC (_color_correction_gui_delete_cb),
			data);

    notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (data->shell)->vbox),
			notebook, TRUE, TRUE, 0);


    /*** WHEELS TAB ***/
    mainhbox = gtk_hbox_new(FALSE,0);
    button = gtk_check_button_new_with_label("Colour Wheels");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", _color_correction_gui_wheels_tab_toggled_cb, data);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mainhbox, button);

    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_check_button_new_with_label("Curves");        
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, button);

    /*** WHEELS + SLIDERS + INPUTS ***/
    for (i=SHADOWS; i<=HIGHLIGHTS; i++)
    {   data->settings->values[i][HUE]=0.0;      
        data->settings->values[i][SATURATION]=0.0;      
	data->settings->values[i][LIGHTNESS]=0.0;      
      
        /* vertical box for wheel + inputs */
        vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
	gtk_box_pack_start(GTK_BOX(mainhbox), vbox, TRUE, TRUE, 5);    

	/* horizontal box for colour wheel and slider */    
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 3);

	hbox = gtk_hbox_new(FALSE, 0);
	/*	GtkStyle *style= gtk_style_new();
	style->fg[GTK_STATE_NORMAL]=style->dark[GTK_STATE_NORMAL];
	gtk_widget_set_style(hbox, style); */

	gtk_container_add(GTK_CONTAINER(frame), hbox);    

	/* WHEEL */
	data->color_wheels[i] = gtk_drawing_area_new ();
	gtk_drawing_area_size(GTK_DRAWING_AREA(data->color_wheels[i]), WHEEL_RADIUS*2+2,WHEEL_RADIUS*2+2); 
    wheel_data = g_new(_ColorCorrectionGuiWheel, 1);
	wheel_data->range = i;
	wheel_data->pixmap = NULL;
	wheel_data->display_buffer = NULL;
	wheel_data->gc = NULL;
	/* -1 is so we know it is the first time the wheel is drawn
	   cause after resizing we need to readjust the handle position */
	wheel_data->width = -1;
	wheel_data->height = -1;
	wheel_data->x = 1;
	wheel_data->y = 1;
	wheel_data->radius = WHEEL_RADIUS;
	wheel_data->position[X]=wheel_data->position[Y]=WHEEL_RADIUS+1;
	wheel_data->mouse_offset[X]=wheel_data->mouse_offset[Y]=0;
	wheel_data->active = TRUE;
	wheel_data->dragging = FALSE;
	gtk_object_set_data_full(GTK_OBJECT(data->color_wheels[i]), "data", (gpointer)wheel_data, _color_correction_gui_free_wheel);

	gtk_widget_set_events (data->color_wheels[i], WHEEL_EVENT_MASK);
	gtk_signal_connect (GTK_OBJECT (data->color_wheels[i]), "event",
			    GTK_SIGNAL_FUNC (_color_correction_gui_wheel_events_cb), data);
	gtk_box_pack_start(GTK_BOX(hbox), data->color_wheels[i], TRUE, TRUE, 0);   	


	/* SLIDER */    
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_widget_set_usize(frame, LSLIDER_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
	/*	gtk_widget_set_style(frame, style); */

	data->lsliders[i] = gtk_drawing_area_new();    
    lslider_data = g_new(_ColorCorrectionGuiLSlider, 1);
	lslider_data->range= i;
	lslider_data->gc = NULL;
	lslider_data->pixmap = NULL;
	lslider_data->display_buffer = NULL;
	lslider_data->height = WHEEL_RADIUS*2;
	lslider_data->position = WHEEL_RADIUS;
	lslider_data->dragging = FALSE;
	gtk_object_set_data_full(GTK_OBJECT(data->lsliders[i]), "data", (gpointer)lslider_data, _color_correction_gui_free_lslider);

	gtk_drawing_area_size (GTK_DRAWING_AREA(data->lsliders[i]), LSLIDER_WIDTH, WHEEL_RADIUS*2);
	gtk_widget_set_events (data->lsliders[i], WHEEL_EVENT_MASK);    
	gtk_signal_connect (GTK_OBJECT (data->lsliders[i]), "event",
			    GTK_SIGNAL_FUNC (_color_correction_gui_lslider_events_cb), data);
	gtk_container_add(GTK_CONTAINER(frame), data->lsliders[i]);


	/* INPUTS */
	hue_adjustment = gtk_adjustment_new(0.0, 0.0, 360.0, 1.0, 0.0, 0.0);
	saturation_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 0.0, 0.0);
	lightness_adjustment = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 0.0, 0.0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);    

	spin_data = g_new(_ColorCorrectionGuiSpin, 1);
	spin_data->range = i;
	spin_data->attribute = HUE;
	label = gtk_label_new("H");
	spin_data->old_value = 0.0;
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);    
	data->spins[i][HUE] = gtk_spin_button_new(GTK_ADJUSTMENT(hue_adjustment), 1.0, 2);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(data->spins[i][HUE]), 
				 TRUE);
	gtk_widget_set_usize (data->spins[i][HUE], 60, 17);
	gtk_object_set_data_full(GTK_OBJECT(data->spins[i][HUE]), "data", (gpointer)spin_data, g_free);      
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[i][HUE]), 0.0);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][HUE]), "changed",
				  GTK_SIGNAL_FUNC (_color_correction_gui_spin_changed_cb), data);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][HUE]), "event",
			    GTK_SIGNAL_FUNC (_color_correction_gui_spin_events_cb), data);
	gtk_box_pack_start(GTK_BOX(hbox), data->spins[i][HUE], FALSE, FALSE, 0);    


	spin_data = g_new(_ColorCorrectionGuiSpin, 1);
	spin_data->range = i;
	spin_data->attribute = SATURATION;
	spin_data->old_value = 0.0;
	label = gtk_label_new("S");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);    
	data->spins[i][SATURATION] = gtk_spin_button_new(GTK_ADJUSTMENT(saturation_adjustment), 1.0, 2);
	gtk_widget_set_usize (data->spins[i][SATURATION], 60, 17);
	gtk_object_set_data_full(GTK_OBJECT(data->spins[i][SATURATION]), "data", (gpointer)spin_data, g_free);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[i][SATURATION]), 0.0);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][SATURATION]), "changed",
				  GTK_SIGNAL_FUNC (_color_correction_gui_spin_changed_cb), data);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][SATURATION]), "event",
				  GTK_SIGNAL_FUNC (_color_correction_gui_spin_events_cb), data);
	gtk_box_pack_start(GTK_BOX(hbox), data->spins[i][SATURATION], FALSE, FALSE, 0);    

	
	spin_data = g_new(_ColorCorrectionGuiSpin, 1);
	spin_data->range = i;
	spin_data->attribute = LIGHTNESS;
	spin_data->old_value = 50.0;
	label = gtk_label_new("L");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);    
	data->spins[i][LIGHTNESS] = gtk_spin_button_new(GTK_ADJUSTMENT(lightness_adjustment), 1.0, 2);
	gtk_widget_set_usize (data->spins[i][LIGHTNESS], 60, 17);
	gtk_object_set_data_full(GTK_OBJECT(data->spins[i][LIGHTNESS]), "data", (gpointer)spin_data, g_free);      
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[i][LIGHTNESS]), 50.0);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][LIGHTNESS]), "changed",
				  GTK_SIGNAL_FUNC (_color_correction_gui_spin_changed_cb), data);
	gtk_signal_connect_after (GTK_OBJECT (data->spins[i][LIGHTNESS]), "event",
			    GTK_SIGNAL_FUNC (_color_correction_gui_spin_events_cb), data);
	gtk_box_pack_start(GTK_BOX(hbox), data->spins[i][LIGHTNESS], FALSE, FALSE, 0);    
    }



    /*** BUTTONS ***/
    buttonbox = gtk_hbutton_box_new();
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(buttonbox), 4);
    gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(data->shell)->action_area), FALSE);
    gtk_box_pack_end (GTK_BOX(GTK_DIALOG(data->shell)->action_area), buttonbox, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label("OK");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       GTK_SIGNAL_FUNC(_color_correction_gui_ok_cb), data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label("Cancel");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		       GTK_SIGNAL_FUNC(_color_correction_gui_cancel_cb), data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);           

    button = gtk_button_new_with_label("Options...");
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked", 
		       GTK_SIGNAL_FUNC(_color_correction_gui_options_create), (gpointer)data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);           

    button = gtk_button_new_with_label("Save Profile...");
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);    
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked", 
			      GTK_SIGNAL_FUNC(_color_correction_gui_save_profile_create), (gpointer)data);

    gtk_widget_show_all (data->shell);
}


static void
_color_correction_gui_free(_ColorCorrectionGui *data)
{   gtk_widget_destroy (data->shell);
    g_free(data->settings); 
    g_free(data);

    color_correction_free_transfer_LUTs();
    color_correction_free_lab_transforms();
}


static void
_color_correction_gui_preview(_ColorCorrectionGui *data)
{   
    if (data->active == TRUE) 
    {    image_map_apply (data->image_map, color_correction, data->settings);    
    }
}


static void
_color_correction_gui_update_wheel_handle (_ColorCorrectionGui *dialog,
					   ColorCorrectionLightnessRange range)
{   _ColorCorrectionGuiWheel *data =
     (_ColorCorrectionGuiWheel *)gtk_object_get_data(GTK_OBJECT(dialog->color_wheels[range]), 
								"data");
     gint new_pos[2]; 

     _color_correction_gui_wheel_hs_to_pos(dialog->settings->values[range][HUE],
					   dialog->settings->values[range][SATURATION],
					   data->radius, data->radius,
					   &new_pos[X],
					   &new_pos[Y]);
    _color_correction_gui_erase_wheel_handle(dialog->color_wheels[range], data);    
    data->position[X] = data->x + new_pos[X];
    data->position[Y] = data->y + new_pos[Y];	       
    _color_correction_gui_draw_wheel_handle(dialog->color_wheels[range], data);                      
}


static void
_color_correction_gui_update_lslider_handle (_ColorCorrectionGui *dialog,
					     ColorCorrectionLightnessRange range)
{   _ColorCorrectionGuiLSlider *data =
    (_ColorCorrectionGuiLSlider *)gtk_object_get_data(GTK_OBJECT(dialog->lsliders[range]), 
								  "data");
    gint new_position =  (0.5-dialog->settings->values[range][LIGHTNESS]) * data->height;

    if (new_position != data->position)
    {   _color_correction_gui_erase_lslider_handle(dialog->lsliders[range], data);
        data->position = new_position;
	_color_correction_gui_draw_lslider_handle(dialog->lsliders[range], data);                      
	
	_color_correction_gui_draw_wheel(dialog->color_wheels[data->range], 
				    dialog->settings->values[data->range][LIGHTNESS] + 0.5);
	gtk_widget_draw(dialog->color_wheels[data->range], NULL);
    }
}


static void
_color_correction_gui_update_spins (_ColorCorrectionGui *data,
				    ColorCorrectionLightnessRange range)
{    gtk_signal_handler_block_by_func(GTK_OBJECT(data->spins[range][HUE]), _color_correction_gui_spin_changed_cb, data);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[range][HUE]), 
				data->settings->values[range][HUE]);        
     gtk_signal_handler_unblock_by_func(GTK_OBJECT(data->spins[range][HUE]), _color_correction_gui_spin_changed_cb, data);
 
     gtk_signal_handler_block_by_func(GTK_OBJECT(data->spins[range][SATURATION]), 
				      _color_correction_gui_spin_changed_cb, data);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[range][SATURATION]), 
				data->settings->values[range][SATURATION]*100);        
     gtk_signal_handler_unblock_by_func(GTK_OBJECT(data->spins[range][SATURATION]), 
					_color_correction_gui_spin_changed_cb, data);

     gtk_signal_handler_block_by_func(GTK_OBJECT(data->spins[range][LIGHTNESS]), 
				      _color_correction_gui_spin_changed_cb, data);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->spins[range][LIGHTNESS]), 
				(data->settings->values[range][LIGHTNESS]+0.5)*100);        
     gtk_signal_handler_unblock_by_func(GTK_OBJECT(data->spins[range][LIGHTNESS]), 
					_color_correction_gui_spin_changed_cb, data);     
}


static void
_color_correction_gui_ok_cb (GtkWidget *widget,
			      _ColorCorrectionGui *data)
{   image_map_commit(data->image_map);
    _color_correction_gui_free(data);
}


static void
_color_correction_gui_cancel_cb (GtkWidget *widget,
				  _ColorCorrectionGui *data)
{   image_map_abort(data->image_map);
    gdisplay_flush(data->display); 
    _color_correction_gui_free(data);
}


static gint
_color_correction_gui_delete_cb (GtkWidget *widget,
				 GdkEvent *event,
				 gpointer user_data)
{   _color_correction_gui_cancel_cb (widget, user_data);    
    return TRUE;
}


static void _color_correction_gui_wheels_tab_toggled_cb(GtkWidget *widget, 
							 _ColorCorrectionGui *data)
{   data->active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? TRUE : FALSE;
    if (data->active == FALSE) 
    {   image_map_remove(data->image_map);  
        gdisplay_flush(data->display);
    }
    else _color_correction_gui_preview(data);
}


static gint
_color_correction_gui_wheel_events_cb (GtkWidget *widget,
				   GdkEvent  *event,
				   _ColorCorrectionGui *dialog)
{   _ColorCorrectionGuiWheel *data = (_ColorCorrectionGuiWheel *)gtk_object_get_data(GTK_OBJECT(widget), "data");
    _ColorCorrectionGuiLSlider *slider_data;
  
    switch (event->type)
    {
    case GDK_CONFIGURE:
      _color_correction_gui_draw_wheel(widget, dialog->settings->values[data->range][LIGHTNESS] + 0.5);      
      break;
    case GDK_EXPOSE:
      gdk_gc_set_function(data->gc, GDK_COPY);
      gdk_draw_pixmap(widget->window,
		      data->gc,
		      data->display_buffer,
		      ((GdkEventExpose*)event)->area.x,((GdkEventExpose*)event)->area.y,
		      ((GdkEventExpose*)event)->area.x,((GdkEventExpose*)event)->area.y,
		      ((GdkEventExpose*)event)->area.width,((GdkEventExpose*)event)->area.height);
      break;
    case GDK_BUTTON_PRESS:
      /* if button click within switch */
      if ((event->button.x > 2) && (event->button.x < 28) &&
	  (event->button.y > 2) && (event->button.y < 21))
      {   data->active = data->active ? FALSE : TRUE;
	  if (data->active) 
	  {
   	     /* reset values from positions */
  	     _color_correction_gui_wheel_pos_to_hs(data->position[X]-data->x, 
					      data->position[Y]-data->y, 
					      data->radius, data->radius, 
					      &dialog->settings->values[data->range][HUE], 
					      &dialog->settings->values[data->range][SATURATION]); 
	     slider_data = (_ColorCorrectionGuiLSlider *)gtk_object_get_data(GTK_OBJECT(dialog->lsliders[data->range]), 
										 "data");
	     dialog->settings->values[data->range][LIGHTNESS] = (1-slider_data->position/(gfloat)slider_data->height) - 0.5;	     
          }
          else 
	  {
 	     dialog->settings->values[data->range][HUE] = 0.0;
	     dialog->settings->values[data->range][SATURATION] = 0.0;
	     dialog->settings->values[data->range][LIGHTNESS] = 0.0;	     
	  }

          _color_correction_gui_draw_wheel_toggle(widget);
	  _color_correction_gui_preview(dialog);
      }
      /* if button click onto handle */
      else if ((event->button.x > (data->position[X]-5)) &&
	  (event->button.x < (data->position[X]+5)) &&
	  (event->button.y > (data->position[Y]-5)) &&
	  (event->button.y < (data->position[Y]+5)))
      {   data->dragging = TRUE;
	  data->mouse_offset[X] =event->button.x - data->position[X];
	  data->mouse_offset[Y] =event->button.y - data->position[Y];
      }

      break;

    case GDK_MOTION_NOTIFY:
      /* data->x (left edge of circle hull) + data->radius is x of circle centre */
      if ((data->dragging) && (sqrt(SQR(event->button.x - (data->x+data->radius)) + 
				    SQR(event->button.y - (data->y+data->radius))) <= data->radius))
      {   _color_correction_gui_erase_wheel_handle(widget, data);

          data->position[X] = event->button.x - data->mouse_offset[X];
          data->position[Y] = event->button.y - data->mouse_offset[Y];
          _color_correction_gui_wheel_pos_to_hs(data->position[X]-data->x, 
					   data->position[Y]-data->y, 
					   data->radius, data->radius, 
					   &dialog->settings->values[data->range][HUE], 
					   &dialog->settings->values[data->range][SATURATION]); 
	  _color_correction_gui_update_spins(dialog, data->range);

	  _color_correction_gui_draw_wheel_handle(widget, data);
      }
      break;

    case GDK_BUTTON_RELEASE:
      if (data->dragging == TRUE) 
      {   data->dragging=FALSE;	  
      	  _color_correction_gui_preview(dialog);
      }
      break;
    default:
      break;
    }


  return (FALSE);
}


static gint
_color_correction_gui_lslider_events_cb (GtkWidget *widget,
				 GdkEvent  *event,
				 _ColorCorrectionGui *dialog)
{   _ColorCorrectionGuiLSlider *data = (_ColorCorrectionGuiLSlider *)gtk_object_get_data(GTK_OBJECT(widget), "data");
    gfloat new_lightness;

    switch (event->type)
    {
    case GDK_CONFIGURE:
      _color_correction_gui_draw_lslider(widget);      
      break;
    case GDK_EXPOSE:
      gdk_gc_set_function(data->gc, GDK_COPY);
      gdk_draw_pixmap(widget->window,
		      data->gc,
		      data->display_buffer,
		      ((GdkEventExpose*)event)->area.x,((GdkEventExpose*)event)->area.y,
		      ((GdkEventExpose*)event)->area.x,((GdkEventExpose*)event)->area.y,
		      ((GdkEventExpose*)event)->area.width,((GdkEventExpose*)event)->area.height);
      break;
    case GDK_BUTTON_PRESS:
      _color_correction_gui_erase_lslider_handle(widget, data);

      data->position=event->button.y;
      new_lightness = (1-data->position/(gfloat)data->height) -0.5;
      if (dialog->settings->values[data->range][LIGHTNESS] != new_lightness) 
      {   if(dialog->settings->values[data->range][LIGHTNESS] == 0)
	  {   dialog->settings->lightness_modified ++;
	  }
          else if(new_lightness == 0)
	  {   dialog->settings->lightness_modified --;
	  }
          dialog->settings->values[data->range][LIGHTNESS] = new_lightness;
      }
	
      _color_correction_gui_update_spins(dialog, data->range);

      _color_correction_gui_draw_lslider_handle(widget, data);
      _color_correction_gui_draw_wheel(dialog->color_wheels[data->range], 
				  dialog->settings->values[data->range][LIGHTNESS] + 0.5);
      gtk_widget_draw(dialog->color_wheels[data->range], NULL);

      data->dragging = TRUE;
      break;
    case GDK_MOTION_NOTIFY:
      if (data->dragging)
      {   _color_correction_gui_erase_lslider_handle(widget, data);
          data->position=event->button.y;
	  new_lightness = (1-data->position/(gfloat)data->height) -0.5;
	  if (dialog->settings->values[data->range][LIGHTNESS] != new_lightness) 
          {   if(dialog->settings->values[data->range][LIGHTNESS] == 0)
	      {   dialog->settings->lightness_modified ++;
	      }
              else if(new_lightness == 0)
	      {   dialog->settings->lightness_modified --;
	      }

              dialog->settings->values[data->range][LIGHTNESS] = new_lightness;
	  }
	  _color_correction_gui_draw_lslider_handle(widget, data);
	  _color_correction_gui_update_spins(dialog, data->range);
      }
      break;
    case GDK_BUTTON_RELEASE:
      if (data->dragging) 
      {   data->dragging=FALSE;	  
	  _color_correction_gui_draw_wheel(dialog->color_wheels[data->range], 
				      dialog->settings->values[data->range][LIGHTNESS] + 0.5);
          gtk_widget_draw(dialog->color_wheels[data->range], NULL);
      	  _color_correction_gui_preview(dialog);
      }
      break;
    default:
      break;
    }
    return FALSE;
}      


static void 
_color_correction_gui_spin_changed_cb (GtkWidget *widget, 
				       _ColorCorrectionGui *dialog)
{   _ColorCorrectionGuiSpin *data = (_ColorCorrectionGuiSpin *)gtk_object_get_data(GTK_OBJECT(widget), "data");

    gfloat value = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));    
    if (data->attribute == HUE)
    {   dialog->settings->values[data->range][data->attribute] = value;
        _color_correction_gui_update_wheel_handle(dialog, data->range);
    }
    else if (data->attribute == SATURATION)
    {   dialog->settings->values[data->range][data->attribute] = value/100.;
        _color_correction_gui_update_wheel_handle(dialog, data->range);
    }
    if (data->attribute == LIGHTNESS) 
    {   gfloat new_lightness = value/100. - .5;
	if (dialog->settings->values[data->range][LIGHTNESS] != new_lightness) 
        {   if(dialog->settings->values[data->range][LIGHTNESS] == 0)
	    {   dialog->settings->lightness_modified ++;
	    }
            else if(new_lightness == 0)
	    {   dialog->settings->lightness_modified --;
	    }

            dialog->settings->values[data->range][LIGHTNESS] = new_lightness;
	}
        _color_correction_gui_update_lslider_handle(dialog, data->range);
    }
}

static gint
_color_correction_gui_spin_events_cb (GtkWidget *widget, 
				      GdkEvent *event, 
				      _ColorCorrectionGui *dialog)
{   _ColorCorrectionGuiSpin *data = (_ColorCorrectionGuiSpin *)gtk_object_get_data(GTK_OBJECT(widget), "data");

    if (((event->type == GDK_FOCUS_CHANGE) && !((GdkEventFocus*)event)->in) ||
	((event->type == GDK_KEY_RELEASE) && (((GdkEventKey *)event)->keyval == GDK_Return)) ||
	(event->type == GDK_BUTTON_RELEASE))
    {   gfloat value = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));    
        if (value != data->old_value) 
	{   data->old_value = value;
            _color_correction_gui_preview(dialog);
	}
    }

    return FALSE;
}




/****
 *  IMPLEMENTATION 2/5: THE COLOR WHEELS
 ****/

static void
_color_correction_gui_free_wheel(gpointer user_data)
{   _ColorCorrectionGuiWheel *data = (_ColorCorrectionGuiWheel *) user_data;
    gdk_pixmap_unref(data->display_buffer);
    gdk_pixmap_unref(data->pixmap);
    gdk_gc_unref(data->gc);
    g_free(data);    
}


static void
_color_correction_gui_draw_wheel (GtkWidget *drawing_area, gfloat lightness)
{   gint x, y;
    gfloat h,s,l;
    guchar color[3];
    _ColorCorrectionGuiWheel *data = (_ColorCorrectionGuiWheel *)gtk_object_get_data(GTK_OBJECT(drawing_area), "data");
    int xmax, ymax;
    
    /* translate the marker to the new size */
    gint new_width, new_height;

    new_width = drawing_area->allocation.width;
    new_height = drawing_area->allocation.height;
    if (data->width != -1)
    {   data->position[X]=data->position[X]/(gfloat)data->width * new_width;
        data->position[Y]=data->position[Y]/(gfloat)data->height * new_height;
    }

    data->width = new_width;
    data->height = new_height;

    data->radius = data->width < data->height ? data->width/2 : data->height/2-2;

    if (data->gc == NULL)
    {   data->gc = gdk_gc_new (drawing_area->window);
    }
    if (data->pixmap) 
    {   gdk_pixmap_unref(data->pixmap);
    }
    if (data->display_buffer) 
    {   gdk_pixmap_unref(data->display_buffer);
    }
    data->pixmap=gdk_pixmap_new(drawing_area->window, data->width, data->height, -1);
    data->display_buffer=gdk_pixmap_new(drawing_area->window, data->width, data->height, -1);

    gdk_gc_set_function(data->gc, GDK_COPY);
    gdk_draw_rectangle(data->pixmap, drawing_area->style->dark_gc[GTK_STATE_NORMAL],
		       TRUE, 0,0, data->width, data->height);

    /* iterate over wheel calculating hue and saturation */
    data->x = data->width/2-data->radius;
    data->y = data->height/2-data->radius;
    xmax = data->x + 2 * data->radius;
    ymax = data->y + 2 * data->radius;
    
    for (y = data->y; y < ymax; y++)
    {   for (x = data->x; x < xmax; x++)
	{   if (_color_correction_gui_wheel_pos_to_hs (x-data->x, y-data->y, data->radius, data->radius, &h, &s))
  	    {   l = lightness;
	        color_correction_hls_to_rgb (&h,&l,&s);
	        color[0]=h*255;
		color[1]=l*255;		
		color[2]=s*255;		
 	        gdk_rgb_gc_set_foreground(data->gc, ((color[0]<<16) | (color[1]<<8) | color[2]));
	        gdk_draw_point (data->pixmap, data->gc, x, y);		
	    }
	}
    }		      

    gdk_draw_pixmap(data->display_buffer, data->gc, data->pixmap, 0, 0, 0, 0, data->width, data->height);
    _color_correction_gui_draw_wheel_toggle(drawing_area);
    _color_correction_gui_draw_wheel_handle(drawing_area, data);    
}


static void
_color_correction_gui_draw_wheel_handle (GtkWidget *drawing_area, _ColorCorrectionGuiWheel *data)
{   gdk_gc_set_foreground(data->gc, &GTK_WIDGET(drawing_area)->style->black);
   
    gdk_draw_arc(data->display_buffer, data->gc, FALSE, 
		 data->position[X]-5, data->position[Y]-5,
		 10,10,0,23040); /* last two params are angle in 1/64th of degree */
    gdk_draw_point(data->display_buffer, data->gc, 
		   data->position[X], data->position[Y]);
    {
      GdkRectangle update_area = {data->position[X]-5, data->position[Y]-5, 11, 11};
      update_area.x = data->position[X]-5;
      update_area.y = data->position[Y]-5;
      gtk_widget_draw(drawing_area, &update_area);
    }
}


static void
_color_correction_gui_erase_wheel_handle (GtkWidget *drawing_area, _ColorCorrectionGuiWheel *data)
{   GdkRectangle update_area = {data->position[X]-5, data->position[Y]-5, 11, 11};
    update_area.x = data->position[X]-5;
    update_area.y = data->position[Y]-5;
    /* overwrite the handle with the corresponding bit of the original pixmap */
    gdk_draw_pixmap(data->display_buffer, data->gc,
		    data->pixmap, 
		    update_area.x, update_area.y,
		    update_area.x, update_area.y,
		    update_area.width, update_area.height);
    gtk_widget_draw(drawing_area, &update_area);
}


static void
_color_correction_gui_draw_wheel_toggle(GtkWidget *drawing_area)
{   _ColorCorrectionGuiWheel *data = (_ColorCorrectionGuiWheel *)gtk_object_get_data(GTK_OBJECT(drawing_area), "data");
    if (data->active) 
    {   gdk_rgb_gc_set_foreground(data->gc, 0x777777);
        gdk_draw_rectangle(data->display_buffer, data->gc, TRUE, 3,3, 26, 19);
	gdk_rgb_gc_set_foreground(data->gc, 0xffffff);
	gdk_draw_line(data->display_buffer, data->gc, 4,20,27,20);
	gdk_draw_line(data->display_buffer, data->gc, 27,4,27,20);
	gdk_rgb_gc_set_foreground(data->gc, 0x444444);
	gdk_draw_line(data->display_buffer, data->gc, 3,3,27,3);
	gdk_draw_line(data->display_buffer, data->gc, 3,3,3,20);
	gdk_rgb_gc_set_foreground(data->gc, 0x222222);
	gdk_draw_line(data->display_buffer, data->gc, 4,4,26,4);
	gdk_draw_line(data->display_buffer, data->gc, 4,4,4,19);
	gdk_gc_set_foreground(data->gc, &GTK_WIDGET(drawing_area)->style->mid[GTK_STATE_NORMAL]);
	gdk_draw_text(data->display_buffer, gtk_style_get_font(GTK_WIDGET(drawing_area)->style),
		      GTK_WIDGET(drawing_area)->style->light_gc[GTK_STATE_NORMAL], 5, 17, 
		      color_correction_wheel_labels[data->range], 3);
    }
    else
    {   gdk_rgb_gc_set_foreground(data->gc, 0x999999);
        gdk_draw_rectangle(data->display_buffer, data->gc, TRUE, 3,3, 26, 19);
	gdk_rgb_gc_set_foreground(data->gc, 0x444444);
	gdk_draw_line(data->display_buffer, data->gc, 4,20,27,20);
	gdk_draw_line(data->display_buffer, data->gc, 27,4,27,20);
	gdk_rgb_gc_set_foreground(data->gc, 0xffffff);
	gdk_draw_line(data->display_buffer, data->gc, 3,3,27,3);
	gdk_draw_line(data->display_buffer, data->gc, 3,3,3,20);
	gdk_rgb_gc_set_foreground(data->gc, 0x222222);
	gdk_draw_line(data->display_buffer, data->gc, 4,21,27,21);
	gdk_draw_line(data->display_buffer, data->gc, 27,4,27,21);
	gdk_gc_set_foreground(data->gc, &GTK_WIDGET(drawing_area)->style->mid[GTK_STATE_NORMAL]);
	gdk_draw_text(data->display_buffer, gtk_style_get_font( GTK_WIDGET(drawing_area)->style ),
		      GTK_WIDGET(drawing_area)->style->light_gc[GTK_STATE_NORMAL], 5, 17, 
		      color_correction_wheel_labels[data->range], 3);
    }
    {
      GdkRectangle update_area = {3,3,27,19};    
      gtk_widget_draw(drawing_area, &update_area);
    }
}


static gboolean
_color_correction_gui_wheel_pos_to_hs (gint     x,  gint     y,
				  gint  cx, gint cy,
				  gfloat *h,  gfloat *s)
{   gfloat rx, ry;

    /* get x,y distance from centre */
    rx = ((gfloat) x - cx);
    ry = ((gfloat) y - cy);

    /* normalize to 0..1 */
    rx = rx/cx;
    ry = ry/cy;

    /* calculate s as distance  from centre along r */
    *s = sqrt (rx*rx + ry*ry);

    /* h is the angle */
    if (*s != 0.0)
    {   *h = atan2 (rx / (*s), ry / (*s));
    }
    else 
    {   *h = 0.0;
    }

    /* convert from radiant between -pi and pi to degrees 0-360 */
    *h = (180.0 * (*h)) /  M_PI + 180;

    if (*s == 0.0)
    {   *s = 0.00001;
    }
    else if (*s > 1.0)
    {   *s = 1.0;
        return FALSE;
    }

    return TRUE;
}


static gboolean
_color_correction_gui_wheel_hs_to_pos (gfloat h,  gfloat s,
				  gint  cx, gint cy,
                                  gint* x,  gint* y)
{   gfloat rx, ry;

    /* sanity checks */ 
    if (!((0<=h) && (h<=360)))
    {   g_warning("_color_correction_gui_wheel_hs_to_pos: illegal value for hue, should be 0<=h<=360");
        return FALSE;
    }
    if (!((0<=s) && (s<=1.0)))
    {   g_warning("_color_correction_gui_wheel_hs_to_pos: illegal value for saturation, should be 0<=s<=1.0");
        return FALSE;
    }
    
    
    /* turn degrees into radians between -pi and pi */
    h = (h/180) * M_PI - M_PI;

    /* saturation is centre distance, get x and y component */
    rx = sin(h) * s;
    ry = cos(h) * s;

    /* scale to circle size and translate centre */
    *x = rx * cx + cx;
    *y = ry * cy + cy;

    return TRUE;
}




/****
 *  IMPLEMENTATION 3/5: THE LIGHTNESS SLIDERS
 ****/

static void
_color_correction_gui_free_lslider(gpointer user_data)
{   _ColorCorrectionGuiLSlider *data = (_ColorCorrectionGuiLSlider *) user_data;
    gdk_pixmap_unref(data->pixmap);
    gdk_pixmap_unref(data->display_buffer);
    gdk_gc_unref(data->gc);
    g_free(data);    
}


static void
_color_correction_gui_draw_lslider (GtkWidget *drawing_area)
{   _ColorCorrectionGuiLSlider *data = (_ColorCorrectionGuiLSlider *)gtk_object_get_data(GTK_OBJECT(drawing_area), "data");
    gint y;
    int step_size;
    int gray;
    
    data->height = drawing_area->allocation.height;

    if (data->gc == NULL)
    {   data->gc = gdk_gc_new (drawing_area->window);
    }
    if (data->pixmap) 
    {   gdk_pixmap_unref(data->pixmap);
    }
    if (data->display_buffer) 
    {   gdk_pixmap_unref(data->display_buffer);
    }
    data->pixmap=gdk_pixmap_new(drawing_area->window, LSLIDER_WIDTH, data->height, -1);
    data->display_buffer=gdk_pixmap_new(drawing_area->window, LSLIDER_WIDTH, data->height, -1);

    gdk_gc_set_function(data->gc, GDK_COPY);
    gdk_draw_rectangle(data->pixmap, drawing_area->style->white_gc,
		       TRUE, 0,0, LSLIDER_WIDTH, data->height);

    /* draw the slider */
    gdk_gc_set_function(data->gc, GDK_COPY);
    step_size = 255/data->height;
    for (y = 0; y < data->height; y++)
    {   gray=255-(y*step_size);
        gdk_rgb_gc_set_foreground(data->gc, (gray<<16) | (gray<<8) | gray);
        gdk_draw_line (data->pixmap, data->gc, 0, y, LSLIDER_WIDTH, y);		
    }        

    gdk_draw_pixmap(data->display_buffer, data->gc, data->pixmap, 0, 0, 0, 0, LSLIDER_WIDTH, data->height);
    /* draw the marker */
    gdk_gc_set_foreground(data->gc, &drawing_area->style->black);
    _color_correction_gui_draw_lslider_handle(drawing_area, data);

}


static void
_color_correction_gui_draw_lslider_handle (GtkWidget *drawing_area, _ColorCorrectionGuiLSlider *data)
{   gdk_gc_set_foreground(data->gc, &drawing_area->style->white);
    gdk_draw_line(data->display_buffer, data->gc,
		  0, data->position-2, LSLIDER_WIDTH, data->position-2);
    gdk_gc_set_foreground(data->gc, &drawing_area->style->mid[GTK_STATE_NORMAL]);
    gdk_draw_rectangle(data->display_buffer, data->gc, TRUE,
		       0, data->position-1, LSLIDER_WIDTH, 3);
    gdk_gc_set_foreground(data->gc, &drawing_area->style->dark[GTK_STATE_NORMAL]);
    gdk_draw_line(data->display_buffer, data->gc,
		  0, data->position+2, LSLIDER_WIDTH, data->position+2);
    {
      GdkRectangle update_area = {0, data->position-2, LSLIDER_WIDTH, 5};
      update_area.y = data->position-2;
      gtk_widget_draw(drawing_area, &update_area);
    }
}


static void
_color_correction_gui_erase_lslider_handle (GtkWidget *drawing_area, _ColorCorrectionGuiLSlider *data)
{   GdkRectangle update_area = {0, data->position-2, LSLIDER_WIDTH, 5};
    update_area.y = data->position-2;
    gdk_draw_pixmap(data->display_buffer, data->gc,
		    data->pixmap, 
		    update_area.x, update_area.y,
		    update_area.x, update_area.y,
		    update_area.width, update_area.height);
    gtk_widget_draw(drawing_area, &update_area);		    ;
}






/****
 *  IMPLEMENTATION 4/5: THE OPTIONS DIALOG
 ****/


static void 
_color_correction_gui_options_create (_ColorCorrectionGui *parent)
{   GtkWidget *buttonbox = NULL; 
    GtkWidget *button = NULL;
    GtkWidget *frame = NULL;
    GSList *radiogroup = NULL;
    GtkWidget *vbox = NULL;

    _ColorCorrectionGuiOptions *data = g_new(_ColorCorrectionGuiOptions, 1);
    data->parent = parent;
    data->shell = gtk_dialog_new(); 
    gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(data->shell)->vbox), 5);

    /*** PRESERVE ***/
    data->preserve = gtk_check_button_new_with_label("preserve lightness when shifting across wheel");
    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(data->shell)->vbox), data->preserve, FALSE, FALSE, 0);    

    /*** LIGHTNESS MODEL ***/
    data->model_frame = gtk_frame_new("color model for lightness adjustments");
    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(data->shell)->vbox), data->model_frame, FALSE, FALSE, 2);        
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(data->model_frame), vbox);

    data->use_hls = gtk_radio_button_new_with_label(NULL, "HLS (fast, but often less accurate)");
    radiogroup = gtk_radio_button_group(GTK_RADIO_BUTTON(data->use_hls));    
    gtk_box_pack_start (GTK_BOX(vbox), data->use_hls, FALSE, FALSE, 0);    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->use_hls),(parent->settings->model == HLS));

    data->use_lab = gtk_radio_button_new_with_label(radiogroup, "Lab (slower, but usually better)");
    radiogroup = gtk_radio_button_group(GTK_RADIO_BUTTON(data->use_hls));    
    gtk_box_pack_start (GTK_BOX(vbox), data->use_lab, FALSE, FALSE, 0);    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->use_lab),(parent->settings->model == LAB));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->preserve), (parent->settings->preserve_lightness == TRUE));    

    /*** TRANSFER ***/
    frame = gtk_frame_new("transfer functions for shadows, mids and highs");
    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(data->shell)->vbox), frame, FALSE, FALSE, 2);        
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    data->use_bernstein = gtk_radio_button_new_with_label(NULL, "Bernstein polynomials (smoother, but wider)");
    radiogroup = gtk_radio_button_group(GTK_RADIO_BUTTON(data->use_bernstein));    
    gtk_box_pack_start (GTK_BOX(vbox), data->use_bernstein, FALSE, FALSE, 0);    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->use_bernstein),(parent->settings->transfer == BERNSTEIN));

    data->use_gaussian = gtk_radio_button_new_with_label(radiogroup, "Gaussian (more confined)");
    gtk_box_pack_start (GTK_BOX(vbox), data->use_gaussian, FALSE, FALSE, 0);    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->use_gaussian),(parent->settings->transfer == GAUSSIAN));
    
    /*** BUTTONS ***/
    buttonbox = gtk_hbutton_box_new();
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(buttonbox), 4);
    gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(data->shell)->action_area), FALSE);
    gtk_box_pack_end (GTK_BOX(GTK_DIALOG(data->shell)->action_area), buttonbox, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label("OK");
    gtk_signal_connect_object (GTK_OBJECT(button), "clicked", 
			       GTK_SIGNAL_FUNC(_color_correction_gui_options_ok_cb), (gpointer)data);
    gtk_signal_connect_object (GTK_OBJECT (button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->shell);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);    

    button = gtk_button_new_with_label("Cancel");
    gtk_signal_connect_object (GTK_OBJECT (button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->shell);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);           

    gtk_signal_connect_object (GTK_OBJECT(data->shell), "destroy-event",
			       GTK_SIGNAL_FUNC(g_free), (gpointer) data);

    gtk_widget_show_all(data->shell);
}


static void
_color_correction_gui_options_ok_cb(_ColorCorrectionGuiOptions *data)
{   if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->use_hls)))
    {   data->parent->settings->model = HLS;
    }
    else
    {   data->parent->settings->model = LAB;
    }
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->use_bernstein)))
    {   data->parent->settings->transfer = BERNSTEIN;
        color_correction_init_bernstein_transfer_LUTs();
    }
    else
    {   data->parent->settings->transfer = GAUSSIAN;
        color_correction_init_gaussian_transfer_LUTs();
    }

    data->parent->settings->preserve_lightness = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->preserve));
    _color_correction_gui_preview(data->parent);
}




/****
 *  IMPLEMENTATION 5/5: THE SAVE PROFILE DIALOG
 ****/


static void 
_color_correction_gui_save_profile_create (_ColorCorrectionGui *parent)
{   GtkWidget *optionbox = NULL;  
    GtkWidget *label = NULL;

    /* the sample point option at the bottom */
    _ColorCorrectionGuiSaveProfile *data = g_new(_ColorCorrectionGuiSaveProfile, 1);

    data->parent = parent;
    data->shell = gtk_file_selection_new("Save Look Profile");
    if (look_profile_path != NULL)
    {   gtk_file_selection_set_filename(GTK_FILE_SELECTION(data->shell), look_profile_path);
    }
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(data->shell)->ok_button),
			       "clicked", GTK_SIGNAL_FUNC (_color_correction_gui_save_profile_ok_cb), (gpointer)data); 
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(data->shell)->ok_button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->shell);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(data->shell)->cancel_button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->shell);
    gtk_signal_connect_object (GTK_OBJECT(data->shell), "destroy-event",
			       GTK_SIGNAL_FUNC(g_free), (gpointer) data);

    /* the extra box for entering the sample depth */
    optionbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Sample points per channel");
    data->sample_depth_entry = gtk_entry_new();
    gtk_widget_set_usize(data->sample_depth_entry, 20, -1);
    gtk_entry_set_text(GTK_ENTRY(data->sample_depth_entry),"13");
    gtk_box_pack_start(GTK_BOX(optionbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(optionbox), data->sample_depth_entry, FALSE, FALSE, 5);    
    
     
    gtk_widget_show(data->shell);     
    gtk_box_pack_end(GTK_BOX(GTK_FILE_SELECTION(data->shell)->main_vbox), optionbox, FALSE, FALSE, 5);
    gtk_widget_show_all(optionbox);
}

static void 
_color_correction_gui_save_profile_ok_cb(_ColorCorrectionGuiSaveProfile *data) 
{   gchar *file_name = gtk_file_selection_get_filename (GTK_FILE_SELECTION(data->shell));
    guint sample_depth = atoi(gtk_entry_get_text(GTK_ENTRY(data->sample_depth_entry)));
    LookProfile *profile = look_profile_create(data->parent->display->gimage, sample_depth);
    look_profile_include(profile, color_correction, data->parent->settings);
    look_profile_save(profile, file_name,
                      gdisplay_get_cms_intent (data->parent->display),
                      gdisplay_get_cms_flags  (data->parent->display)  );
    look_profile_close(profile);    
    look_profile_gui_refresh(data->parent->display);
}
