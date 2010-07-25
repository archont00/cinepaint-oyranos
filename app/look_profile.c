#include <gtk/gtk.h>
#include <lcms.h>

#include "../lib/version.h"
#include "actionarea.h"
#include "base_frame_manager.h"
#include "cms.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "interface.h"
#include "layer_pvt.h"
#include "look_profile.h"
#include "paint_funcs_area.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "rc.h"
#include "zoom.h"
#include "buttons/look_profile_open.xpm"
#include "buttons/look_profile_on.xpm"
#include "buttons/look_profile_apply.xpm"
#include "buttons/look_profile_apply_all.xpm"




/* changes the image height to new_height by adding rows at the bottom 
   Cannot use gimage_resize, because don't want to
   appear in the undo history */
static void look_profile_change_image_height(GImage *imae, gint new_height);
/* same thing for a layer */
static void look_profile_change_layer_height(Layer *layer, gint new_height);


static int look_profile_sampler(register WORD in[], 
				register WORD out[], 
				register LPVOID data);
#if 0
static void look_profile_new_dialog (void);
static void look_profile_ok_callback (GtkWidget *widget,
				      gpointer   client_data);
static gint look_profile_delete_callback (GtkWidget *widget,
					  GdkEvent *event,
					  gpointer client_data);
static void look_profile_cancel_callback (GtkWidget *widget,
					  gpointer   client_data);
#endif

struct _LookProfile
{   Canvas *canvas;
    guint sample_depth;
    GImage *image;
};

/* a struct to aid the sampling process in the callback function */
typedef struct _SampleInfo 
{   LookProfile  *profile;
    PixelArea     sample_area;
    void         *pag;    
    gint          col_index;    
    guint8       *current_pixel;
    CMSTransform *transform;
} SampleInfo;

/*  the action area structure  
static ActionAreaItem action_items[] =
{
  { "OK", look_profile_ok_callback, NULL, NULL },
  { "Cancel", look_profile_cancel_callback, NULL, NULL }
};
*/

static void 
look_profile_change_image_height(GImage *gimage, gint new_height)
{   Channel *channel;
    Layer *floating_layer;
    GSList *list;
	Canvas *new_canvas;
    PixelArea src_area, dest_area;

    /*  Get the floating layer if one exists  */
    floating_layer = gimage_floating_sel (gimage);
    
    /*  Relax the floating selection  */
    if (floating_layer)
    {   floating_sel_relax (floating_layer, TRUE);
    }

    /*  Resize all channels + selection mask */
    list = gimage->channels;
    list = g_slist_append(list, gimage->selection_mask);
    while (list)
    {   channel = (Channel *) list->data;

	/*  Update the old channel position  */
	drawable_update (GIMP_DRAWABLE(channel),
			 0, 0,
			 0, 0);

	/*  Allocate the new channel, configure dest region  */
	new_canvas = canvas_new (drawable_tag (GIMP_DRAWABLE (channel)), 
				 gimage->width, gimage->height,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

	
	/*  copy from the old to the new  */
	pixelarea_init (&src_area, drawable_data (GIMP_DRAWABLE(channel)),
			0, 0,
			gimage->width, gimage->height,
			FALSE);

	pixelarea_init (&dest_area, new_canvas,
			0, 0,
			gimage->width, new_height,
			TRUE);

	copy_area (&src_area, &dest_area);    
  

	/*  Configure the new channel  */
	GIMP_DRAWABLE(channel)->tiles = new_canvas;
	
	/*  bounds are now unknown  */
	channel->bounds_known = FALSE;
	
	/*  update the new channel area  */
	drawable_update (GIMP_DRAWABLE(channel),
			 0, 0,
			 0, 0);
       
        list = g_slist_next (list);
    }
    
    /* store the new image height */
    gimage->height = new_height;

    gimage_mask_invalidate (gimage);

    /*  Make sure the projection matches the gimage size  */
    gimage_projection_realloc (gimage);

    /*  Rigor the floating selection  */
    if (floating_layer)
    {   floating_sel_rigor (floating_layer, TRUE);
    }

    /*  shrink wrap and update all views  */
    channel_invalidate_previews (gimage->ID);
    layer_invalidate_previews (gimage->ID);
    gimage_invalidate_preview (gimage);
    gdisplays_update_gimage (gimage->ID);
    gdisplays_shrink_wrap (gimage->ID);
    
    zoom_view_changed(gdisplay_get_from_gimage(gimage));
}

static void 
look_profile_change_layer_height(Layer *layer, gint new_height)
{     PixelArea srcPR, destPR;
      gint old_width = drawable_width (GIMP_DRAWABLE(layer));
      gint old_height = drawable_height (GIMP_DRAWABLE(layer));
	  Channel* channel = 0;
	  PixelArea src_area, dest_area;
	  Canvas *new_canvas;

      /*  Update the old layer position  */
      drawable_update (GIMP_DRAWABLE(layer),
		       0, 0,
		       0, 0);

      /*  Allocate the new layer, configure dest region  */
      new_canvas = canvas_new (drawable_tag(GIMP_DRAWABLE(layer)), old_width, new_height,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif


      /*  copy from old to new  */
      pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
		      0, 0, old_width, old_height, FALSE);
      pixelarea_init (&destPR, new_canvas, 0, 0, old_width, new_height, TRUE);

      /*  copy from the old to the new  */
      copy_area (&srcPR, &destPR);

      /*  Configure the new layer  */
      GIMP_DRAWABLE(layer)->tiles = new_canvas;
    
      /*  If there is a layer mask, make sure it gets resized also  */
      if (layer->mask) 
      {   GIMP_DRAWABLE(layer->mask)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
          GIMP_DRAWABLE(layer->mask)->offset_y = GIMP_DRAWABLE(layer)->offset_y;


	  channel = GIMP_CHANNEL(layer->mask);

	  /*  Update the old channel position  */
	  drawable_update (GIMP_DRAWABLE(channel),
			   0, 0,
			   0, 0);

	  /*  Allocate the new channel, configure dest region  */
	  new_canvas = canvas_new (drawable_tag (GIMP_DRAWABLE (channel)), 
  	                           old_width, new_height,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

	  
	  /*  copy from the old to the new  */
	  pixelarea_init (&src_area, drawable_data (GIMP_DRAWABLE(channel)),
			  0, 0,
			  old_width, old_height,
			  FALSE);
	  
	  pixelarea_init (&dest_area, new_canvas,
			  0, 0,
			  old_width, new_height,
			  TRUE);
	  
	  copy_area (&src_area, &dest_area);    
	  
	  
	  /*  Configure the new channel  */
	  GIMP_DRAWABLE(channel)->tiles = new_canvas;
	  
	  /*  bounds are now unknown  */
	  channel->bounds_known = FALSE;
	  
	  /*  update the new channel area  */
	  drawable_update (GIMP_DRAWABLE(channel),
			   0, 0,
			   0, 0);
       
      }

      /*  update the new layer area  */
      drawable_update (GIMP_DRAWABLE(layer),
		       0, 0,
		       0, 0);
}

LookProfile *
look_profile_create(GImage *image, gint sample_depth)
{   LookProfile *look_profile;
    PixelArea sample_area;
    CMSProfile *lab_profile = cms_get_lab_profile(NULL);
    CMSProfile *input_profile = gimage_get_cms_profile(image);
    guint16 Lab[3];
    guint8 rgb[3];
    PixelRow row_buffer;
    void *pag;
    gint col_index;
    guint8 *row_data;
    gint sample_step;
    GSList *profiles = NULL;
    DWORD lcms_format_out;
    CMSTransform *transform;
    Tag tag;

    gint num_pix = pow(sample_depth, 3);

    look_profile = g_new(LookProfile, 1);
    look_profile->image = image;
    look_profile->sample_depth = sample_depth;
    tag = gimage_tag(image);
    look_profile->canvas= canvas_new (tag, num_pix, 1,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif

    
    pixelarea_init(&sample_area, look_profile->canvas, 
		   0, 0, canvas_width(look_profile->canvas), 1, TRUE);
  
    /* get a lab->rgb transform 
       (the reference strip is calculated iterating over Lab, the image is rgb) */
    if (!input_profile) 
    {   input_profile = cms_get_srgb_profile();
    }

    profiles = g_slist_append( profiles, lab_profile);
    profiles = g_slist_append( profiles, input_profile);
    lcms_format_out= cms_get_lcms_format( tag, input_profile );
    transform = cms_get_transform( profiles, TYPE_Lab_16,
						lcms_format_out,
						INTENT_PERCEPTUAL,
                                                cms_default_flags, 0,
                                                tag, tag, NULL);
    g_slist_free(profiles);

    /* iterate over Lab space, converting to rgb, filling into picture */
    pag = pixelarea_register (1, &sample_area);
    pixelarea_getdata (&sample_area, &row_buffer, 0);
    col_index = pixelrow_width(&row_buffer);
    row_data = pixelrow_data(&row_buffer);
    sample_step = 65535/(sample_depth-1);

    Lab[0]=Lab[1]=Lab[2]=0;
    while (TRUE)
    {   while (TRUE)        
	{   while(TRUE)
            {   if (!col_index) 
                {   if ((pag = pixelarea_process(pag)) != NULL)
                    {   pixelarea_getdata(&sample_area, &row_buffer, 0);
		        col_index = pixelrow_width(&row_buffer);
		        row_data = pixelrow_data(&row_buffer);	
		    }
		}

	        cms_transform_uint(transform, Lab, rgb, 1);           	    
	        row_data[0]=rgb[0];
		row_data[1]=rgb[1];
		row_data[2]=rgb[2];		
		row_data += 3;

		col_index--;

		if ((65535-Lab[2])>sample_step) 
		{   Lab[2]+=sample_step;
		}
		else 
		{   Lab[2]=0;
		    break;
		}
	    }
	    if ((65535-Lab[1])>sample_step) 
	    {   Lab[1]+=sample_step;
	    }
	    else 
	    {   Lab[1]=0;
	        break;
	    }
	}
        if ((65535-Lab[0])>sample_step) 
	{   Lab[0]+=sample_step;
	}
	else 
	{   break;
	}
    }  

    pixelarea_process_stop(pag);
    cms_return_transform(transform);
    cms_return_profile(lab_profile);

    return look_profile;
}

/*
static void
look_profile_new_dialog ()
{   GtkWidget *dialog;
    GtkWidget *vbox;

    dialog = gtk_dialog_new ();
    gtk_window_set_wmclass (GTK_WINDOW (dialog), "look_profiles", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (dialog), "Generate Look Profile");

  
    gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			GTK_SIGNAL_FUNC (look_profile_delete_callback),
			(gpointer) dialog);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_border_width (GTK_CONTAINER (vbox), 2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);



  
    action_items[0].user_data = dialog;
    action_items[1].user_data = dialog;
    build_action_area (GTK_DIALOG (dialog), action_items, 2, 0);

    gtk_widget_show (vbox);
    gtk_widget_show (dialog);
}
*/

void 
look_profile_include (LookProfile *profile, ImageMapApplyFunc func, void *data)
{   PixelArea src_area, dest_area;
    Canvas *new_canvas;
    void *pag;

    pixelarea_init(&src_area, profile->canvas, 0,0, canvas_width(profile->canvas), 1, FALSE);
    new_canvas = canvas_new (canvas_tag(profile->canvas), canvas_width(profile->canvas), 1,
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
    pixelarea_init(&dest_area, new_canvas, 0,0, canvas_width(profile->canvas), 1, TRUE);

    for (pag = pixelarea_register (2, &src_area, &dest_area);
	 pag != NULL;
	 pag = pixelarea_process (pag))
    {   (* func)(&src_area, &dest_area, data);
    }

    canvas_delete(profile->canvas);
    profile->canvas = new_canvas;
}

static int 
look_profile_sampler(register WORD in[], register WORD out[], register LPVOID data)
{   SampleInfo *sample_info = (SampleInfo *)data;

    if (!sample_info->col_index) 
    {   if ((sample_info->pag = pixelarea_process(sample_info->pag)) != NULL)
        {   PixelRow row_buffer;
   	    pixelarea_getdata(&sample_info->sample_area, &row_buffer, 0);
	    sample_info->col_index = pixelrow_width(&row_buffer);
	    sample_info->current_pixel = pixelrow_data(&row_buffer);
	}
    }
    	        	       
    /* sample data and transform to PCS */
    cms_transform_uint(sample_info->transform, sample_info->current_pixel, out, 1);           

    sample_info->current_pixel += 3;
    sample_info->col_index--;

    
    return TRUE;
} 

void
look_profile_save (LookProfile *profile,
		   gchar *file_name, int intent, DWORD flags)
{   /* create a new profile, abstract, Lab */
    cmsHPROFILE saved_profile = cmsOpenProfileFromFile(file_name, "w");
    LPLUT AToB0 = cmsAllocLUT();
    SampleInfo *sample_info = g_new(SampleInfo, 1);
    CMSProfile *input_profile;
    CMSProfile *lab_profile = cms_get_lab_profile(NULL);
    Tag t;
    GSList *profiles = NULL;
    PixelRow row_buffer;

    FILE *fp = fopen(file_name, "w");
    if (fp)
        fclose (fp);
    else
      { g_message ("Could not open file: %s", file_name);
        return;
      }

    sample_info->profile = profile;

    cmsSetColorSpace(saved_profile, icSigLabData);
    cmsSetPCS(saved_profile, icSigLabData);
    cmsAddTag(saved_profile, icSigProfileDescriptionTag, strdup(file_name));

    cmsSetDeviceClass(saved_profile, icSigAbstractClass);

    cmsAlloc3DGrid(AToB0, profile->sample_depth, 3, 3);
    

    /* create a transform from image space to lab (the space of the 
       new abstract profile, just the other way from  before)*/
    input_profile = gimage_get_cms_profile(sample_info->profile->image);
    if (!input_profile) 
    {   input_profile = cms_get_srgb_profile();
    }

    t = gimage_tag(sample_info->profile->image);
    profiles = g_slist_append(profiles, input_profile);
    profiles = g_slist_append(profiles, lab_profile);
    sample_info->transform = cms_get_transform(profiles,
                                       cms_get_lcms_format(t, input_profile),
                                       TYPE_Lab_16, intent, flags, 0,
                                       t, t, NULL);

    pixelarea_init(&sample_info->sample_area,
		   profile->canvas,
		   0, 0,
		   canvas_width(profile->canvas), 1,FALSE);

    sample_info->pag = pixelarea_register (1, &sample_info->sample_area);
    pixelarea_getdata (&sample_info->sample_area, &row_buffer, 0);
    sample_info->col_index = pixelrow_width (&row_buffer);
    sample_info->current_pixel = pixelrow_data(&row_buffer);

    cmsSample3DGrid(AToB0, look_profile_sampler, (LPVOID) sample_info, 0);
    pixelarea_process_stop(sample_info->pag);
    
    cmsAddTag(saved_profile, icSigAToB0Tag, AToB0);

    cmsFreeLUT(AToB0);
    cmsCloseProfile(saved_profile);

    cms_return_transform(sample_info->transform);
    cms_return_profile(lab_profile);
    free(sample_info);
}

void
look_profile_close (LookProfile *profile)
{   canvas_delete(profile->canvas);
    g_free(profile);
}



/*static gint
look_profile_delete_callback (GtkWidget *widget,
				  GdkEvent *event,
				  gpointer client_data)
{   look_profile_cancel_callback (widget, client_data);
    return TRUE;
}

static void
look_profile_cancel_callback (GtkWidget *widget,
				  gpointer   client_data)
{   GtkWidget *dialog;
    dialog = (GtkWidget *) client_data;

    gtk_widget_destroy (dialog);
}
*/


















/******************************
 *                            *
 *     PRIVATE INTERFACE      *
 *                            *
 ******************************/

/* the data */
typedef struct _LookProfileGui
{   GtkWidget *shell;
    GtkWidget *profile_list;  /* the clist or profiles */
    GtkWidget *file_selector;
    GDisplay  *display;     
    
    GdkPixmap *sunglasses;    /* the icon to symbolize an active layer */
    guint row_count;          /* the number of rows in the list */
} _LookProfileGui;


/* the data associated with each row in the clist */
typedef struct _LookProfileGuiRowData
{   gchar *file_name;         /* filename + path of profile */
    CMSProfile *handle;       /* profile handle if on, NULL if off */
} _LookProfileGuiRowData;

static void _look_profile_gui_free_row_data(gpointer user_data);

/* update the list of profiles by rereading the directory */
static void _look_profile_gui_update_list(_LookProfileGui *data);
/* switch a profile on or off (display sunglasses and include in display's
   look profile list) */
static void _look_profile_gui_profile_set_active(_LookProfileGui *data, gint row,
						 gboolean active);

/* apply the selected profiles to the image in the associated display */
void _look_profile_gui_apply(_LookProfileGui *data);
/* apply the selected profiles to all the image in the flipbook of the associated display */
void _look_profile_gui_apply_flipbook(_LookProfileGui *data);


/* callbacks */
static gint _look_profile_gui_delete_cb (GtkWidget *widget, GdkEvent *event, _LookProfileGui *data);
static void _look_profile_gui_row_selected_cb(GtkCList *w, gint row, gint col, 
				    GdkEventButton* event,
				    _LookProfileGui *data);
static void _look_profile_gui_open_cb (GtkWidget *widget, _LookProfileGui *data);
static void _look_profile_gui_open_ok_cb(GtkWidget *widget, _LookProfileGui *data);



/* public functions */
void
look_profile_gui(void *display) 
{   GtkWidget *buttonbox;
    GtkWidget *button;
    GdkPixmap *pixmap;
    GdkBitmap *mask;
    GtkWidget *pixmap_widget;
    GtkTooltips *tooltips;
 
    _LookProfileGui *data = g_new(_LookProfileGui, 1);
    ((GDisplay*)display)->look_profile_manager = data;
    data->display = display;
    data->shell = gtk_dialog_new();
   
    gtk_window_set_wmclass (GTK_WINDOW (data->shell), "look_profile", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (data->shell), "Look Profiles");
    gtk_window_set_position(GTK_WINDOW(data->shell), GTK_WIN_POS_CENTER);

    gtk_signal_connect (GTK_OBJECT (data->shell), "delete_event",
			GTK_SIGNAL_FUNC (_look_profile_gui_delete_cb),
			data);
    
    /* create the table */
    data->profile_list = gtk_clist_new(2);

    gtk_clist_set_column_justification (GTK_CLIST(data->profile_list), 0, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_justification (GTK_CLIST(data->profile_list), 1, GTK_JUSTIFY_LEFT);
  
    gtk_clist_set_column_width (GTK_CLIST(data->profile_list), 0, 15);
    gtk_clist_set_column_width (GTK_CLIST(data->profile_list), 1, 30);

    /*    gtk_clist_set_selection_mode (GTK_CLIST(data->profile_list), GTK_SELECTION_BROWSE);  */

    gtk_signal_connect(GTK_OBJECT(data->profile_list), "select_row",
		       GTK_SIGNAL_FUNC(_look_profile_gui_row_selected_cb),
		       data);

    gtk_widget_set_usize(data->profile_list, 150,100);
    _look_profile_gui_update_list(data);
    gtk_box_pack_start(GTK_BOX((GTK_DIALOG (data->shell))->vbox), 
		       data->profile_list, TRUE, TRUE, 0);


    /* the buttons */
    tooltips = gtk_tooltips_new();
    buttonbox = gtk_hbox_new(TRUE, 0);
    /*    gtk_button_box_set_spacing(GTK_BUTTON_BOX(buttonbox), 0); */
    gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(data->shell)->action_area), 0);
    gtk_box_pack_end (GTK_BOX(GTK_DIALOG(data->shell)->action_area), buttonbox, TRUE, TRUE, 0);    

    gtk_widget_show(data->shell);

    /* change store button */
    button = gtk_button_new();
    gtk_tooltips_set_tip(tooltips, button, "Change look profile directory", FALSE);
    gtk_widget_set_usize(button, 25,25);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(_look_profile_gui_open_cb),
		       data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);    
    pixmap = gdk_pixmap_create_from_xpm_d(data->shell->window, &mask, 
					  NULL,
					  look_profile_open_xpm);
    pixmap_widget = gtk_pixmap_new(pixmap, mask);
    gtk_container_add(GTK_CONTAINER(button), pixmap_widget);    

    /* apply profile button */
    button = gtk_button_new();
    gtk_tooltips_set_tip(tooltips, button, "Apply current look to this image/frame", FALSE);
    gtk_widget_set_usize(button, 25,25);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(_look_profile_gui_apply),
			      (gpointer)data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);    
    pixmap = gdk_pixmap_create_from_xpm_d(data->shell->window, &mask, 
					  NULL,
					  look_profile_apply_xpm);
    pixmap_widget = gtk_pixmap_new(pixmap, mask);
    gtk_container_add(GTK_CONTAINER(button), pixmap_widget);

    /* apply profile to all in flipbook button */
    button = gtk_button_new();
    gtk_tooltips_set_tip(tooltips, button, "Apply current look to all stores in flipbook", FALSE);
    gtk_widget_set_usize(button, 25,25);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(_look_profile_gui_apply_flipbook),
			      (gpointer)data);
    gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);    
    pixmap = gdk_pixmap_create_from_xpm_d(data->shell->window, &mask, 
					  NULL,
					  look_profile_apply_all_xpm);
    pixmap_widget = gtk_pixmap_new(pixmap, mask);
    gtk_container_add(GTK_CONTAINER(button), pixmap_widget);

    

    data->sunglasses = gdk_pixmap_create_from_xpm_d(data->shell->window, NULL, 
						    NULL,
						    look_profile_on_xpm);
    gtk_widget_show_all(data->shell);
}

void
look_profile_gui_refresh(void *display)    
{   GDisplay *_display = (GDisplay *)display;
    if (_display->look_profile_manager != NULL)
    {   _look_profile_gui_update_list(_display->look_profile_manager);
    }
}

static void 
_look_profile_gui_free_row_data(gpointer user_data)
{   _LookProfileGuiRowData *data = (_LookProfileGuiRowData *) user_data;
    g_free(data->file_name);
    if (data->handle != NULL)
    {   cms_return_profile(data->handle);
    }
    g_free(data);
}


static void 
_look_profile_gui_update_list(_LookProfileGui *data)
{
    GSList *profile_file_names;
    gint path_length = 0;
    gchar *file_path;
    /* row_data[0] contains the path of the profile,
       row_data[1] is NULL if off, profile handle if on */
    _LookProfileGuiRowData *row_data = NULL;
    gchar *row_text[2] = {NULL, NULL};
    GSList *iterator = 0;

    profile_file_names = cms_read_icc_profile_dir(look_profile_path, icSigAbstractClass);
    iterator =  profile_file_names; 

    if(!look_profile_path || !g_slist_length(profile_file_names))
    { return;
    }
    path_length = strlen(look_profile_path); 

    data->row_count=0;
    gtk_clist_clear(GTK_CLIST(data->profile_list));

    if (!profile_file_names)
    {   return;
    }

    while (iterator != NULL)      
    {   file_path = (gchar *)iterator->data;
        if(!file_path)
        { return;
        }
        row_data = g_new(_LookProfileGuiRowData, 1);
        row_data->file_name = file_path;
	row_data->handle = NULL;
	/* the text to display in the row, one element per column */
        /* strdup(file_path + path_length + 1) extracts the filename from the path */
	row_text[1] = strdup(file_path + path_length + 1);
        gtk_clist_insert(GTK_CLIST(data->profile_list), data->row_count, row_text);	    
	gtk_clist_set_selectable(GTK_CLIST(data->profile_list), data->row_count, FALSE);
	gtk_clist_set_row_data_full(GTK_CLIST(data->profile_list), data->row_count, row_data, 
				    _look_profile_gui_free_row_data);

	iterator = g_slist_next(iterator);
	data->row_count++;
    }

    if (profile_file_names != NULL)
    {   g_slist_free(profile_file_names);
    }
}


static void
_look_profile_gui_profile_set_active (_LookProfileGui *data, gint row, gboolean active)
{   _LookProfileGuiRowData *row_data;
    row_data  = (_LookProfileGuiRowData *) gtk_clist_get_row_data(GTK_CLIST(data->profile_list), row);
    
    if ((active==TRUE) && (row_data->handle == NULL))
    {   row_data->handle = cms_get_profile_from_file(row_data->file_name);
        gdisplay_add_look_profile(data->display, row_data->handle, -1);
	gtk_clist_set_pixmap(GTK_CLIST(data->profile_list), row, 0, data->sunglasses, NULL);	
    }
    else if ((active==FALSE) && (row_data->handle != NULL))
    {   gdisplay_remove_look_profile(data->display, row_data->handle);
    /*        cms_return_profile(row_data->handle); */
        row_data->handle = NULL;
        gtk_clist_set_text(GTK_CLIST(data->profile_list), row, 0, "");        
    }
}


static gint
_look_profile_gui_delete_cb (GtkWidget *widget,
			      GdkEvent *event,
			      _LookProfileGui *data) 
{   gtk_widget_destroy(data->shell);
    gdk_pixmap_unref(data->sunglasses);
    data->display->look_profile_manager = NULL;
    g_free(data);

    return TRUE;
}


static void 
_look_profile_gui_row_selected_cb(GtkCList *w, gint row, gint col, 
				  GdkEventButton* event,
				  _LookProfileGui *data)
{   if (col == 0)
    {   _LookProfileGuiRowData *row_data;
        row_data  = (_LookProfileGuiRowData *) gtk_clist_get_row_data(GTK_CLIST(data->profile_list), row);
        if (row_data->handle == NULL)
	{   _look_profile_gui_profile_set_active(data, row, TRUE);
	}
	else
	{   _look_profile_gui_profile_set_active(data, row, FALSE);
	}
    }

    /* avoid the row being hightlighted in blue */
    gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "select_row");
}


static void 
_look_profile_gui_open_cb (GtkWidget *widget, _LookProfileGui *data)
{   data->file_selector = gtk_file_selection_new("Change Look Profile Store");

    if (look_profile_path != NULL)
    {   gtk_file_selection_set_filename(GTK_FILE_SELECTION(data->file_selector), look_profile_path);
    }
    
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(data->file_selector)->ok_button),
			"clicked", GTK_SIGNAL_FUNC (_look_profile_gui_open_ok_cb), data); 
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(data->file_selector)->ok_button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->file_selector);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(data->file_selector)->cancel_button),
			       "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       (gpointer) data->file_selector);
     
    gtk_widget_show(data->file_selector);     
}

static void 
_look_profile_gui_open_ok_cb(GtkWidget *widget, _LookProfileGui *data) 
{
    GList *update_settings = NULL;
    GList *remove_settings = NULL;

    look_profile_path = strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(data->file_selector)));

    update_settings = g_list_append(update_settings, "look-profile-path");
    save_gimprc(&update_settings, &remove_settings); 
    g_list_free(update_settings);
     
    _look_profile_gui_update_list(data);
}


void 
_look_profile_gui_apply(_LookProfileGui *data)
{   GSList *profiles = NULL;
    CMSProfile *image_profile = gimage_get_cms_profile(data->display->gimage);
    DWORD lcms_format;
    CMSTransform *transform;
    int i;
    Tag tag = gimage_tag(data->display->gimage);

    if (image_profile == NULL)
    {   image_profile = cms_get_srgb_profile();
    }

    profiles = g_slist_append(profiles, (gpointer)image_profile);
    profiles = g_slist_concat(profiles, g_slist_copy(gdisplay_get_look_profile_pipe(data->display)));
    profiles = g_slist_append(profiles, (gpointer)image_profile);
    lcms_format = cms_get_lcms_format(gimage_tag(data->display->gimage),
                                            image_profile);
    transform = cms_get_transform(profiles,
                                   lcms_format, lcms_format,
                                   gdisplay_get_cms_intent(data->display),
                                   gdisplay_get_cms_flags(data->display),
                                   gdisplay_get_cms_proof_intent(data->display),
                                   tag, tag, NULL);
    g_slist_free(profiles);
    gimage_transform_colors(data->display->gimage, transform, TRUE);
    cms_return_transform(transform);

    /* resets all the selections */
    for (i=0; i<data->row_count; i++)
    {   _look_profile_gui_profile_set_active(data, i, FALSE);
    }
}

void 
_look_profile_gui_apply_flipbook (_LookProfileGui *data)
{   GSList *profiles = NULL;
    CMSProfile *image_profile = gimage_get_cms_profile(data->display->gimage);
    DWORD lcms_format;
    CMSTransform *transform;
    GtkWidget *progress_dialog;
    GtkWidget *vbox;
    GtkWidget *progress_bar;
    GSList *iterator;
    gint num_images = 0;
    gint count = 0;
    int i;
    Tag tag = gimage_tag(data->display->gimage);


    if (data->display->bfm == NULL)
    {   g_message("This display has no flipbook. Application to flipbook stores not possible");
        return;
    }

    if (image_profile == NULL)
    {   image_profile = cms_get_srgb_profile();
    }

    profiles = g_slist_append(profiles, (gpointer)image_profile);
    profiles = g_slist_concat(profiles, g_slist_copy(gdisplay_get_look_profile_pipe(data->display)));
    profiles = g_slist_append(profiles, (gpointer)image_profile);
    lcms_format = cms_get_lcms_format( tag, image_profile);
    transform = cms_get_transform(profiles,
                                   lcms_format, lcms_format,
                                   gdisplay_get_cms_intent(data->display),
                                   gdisplay_get_cms_flags(data->display),
                                   gdisplay_get_cms_proof_intent(data->display),
                                   tag, tag, NULL);
    g_slist_free(profiles);

    /* create progress dialog */
#if GTK_MAJOR_VERSION > 1
      progress_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_type_hint (GTK_WINDOW(progress_dialog),
                                GDK_WINDOW_TYPE_HINT_DIALOG);
#else
      progress_dialog = gtk_window_new (GTK_WINDOW_DIALOG);
#endif
    gtk_window_set_wmclass (GTK_WINDOW (progress_dialog), "progress", PROGRAM_NAME);
    gtk_window_set_title (GTK_WINDOW (progress_dialog), "Applying look to flipbook stores...");
    gtk_window_set_position(GTK_WINDOW(progress_dialog), GTK_WIN_POS_CENTER);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (progress_dialog), vbox);

    progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start (GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0);
    gtk_widget_set_usize(progress_bar, 250,-1);
    gtk_widget_show_all(progress_dialog);

    gtk_progress_set_percentage(GTK_PROGRESS(progress_bar),0);
    gtk_widget_draw(progress_bar, NULL);
    gdk_flush();

    iterator = data->display->bfm->sfm->stores;
    num_images = g_slist_length(iterator);
    count = 0;
    while (iterator != NULL)
    {   store *store = iterator->data;
	gimage_transform_colors(store->gimage, transform, FALSE);
        iterator = g_slist_next(iterator);
	/* set modified indicator of store */
    sfm_dirty (data->display, count);
	count ++;
	gtk_progress_set_percentage(GTK_PROGRESS(progress_bar), count/(gfloat)num_images);
 	gtk_widget_draw(progress_bar, NULL);
	gdk_flush();
    }
    
    gtk_widget_destroy(progress_dialog);

    cms_return_transform(transform);

    /* resets all the selections */
    for (i=0; i<data->row_count; i++)
    {   _look_profile_gui_profile_set_active(data, i, FALSE);
    }
}

