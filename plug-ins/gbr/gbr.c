/*
 * gbr plug-in version 1.00
 * Loads/saves version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 */

#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gtk/gtk.h"
#include "lib/plugin_main.h"
#include "app/brush_header.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif


/* Declare local data types
 */

typedef struct {
	char description[256];
	unsigned int spacing;
} t_info;

t_info info = {   /* Initialize to this, change if non-interactive later */
	"GIMP Brush",     
	10
};

int run_flag = 0;

/* Declare some local functions.  */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

static gint save_dialog ();
static gint drawable_num_channels (guint32 drawable_ID);

static guchar * buffer_to_network_order ( guchar *buffer, 
					gint width, 
					gint num_channels, 
					gint image_type);
static guchar * buffer_to_host_order ( guchar *buffer, 
					gint width, 
					gint num_channels, 
					gint image_type);

static void close_callback(GtkWidget * widget, gpointer data);
static void ok_callback(GtkWidget * widget, gpointer data);
static void entry_callback(GtkWidget * widget, gpointer data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "spacing", "Spacing of the brush" },
    { PARAM_STRING, "description", "Short description of the brush" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gbr_load",
                          "loads files of the .gbr file format",
                          "FIXME: write help",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Load>/GBR",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gbr_save",
                          "saves files in the .gbr file format",
                          "Yeah!",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Save>/GBR",
                          "GRAY, U16_GRAY, FLOAT_GRAY, FLOAT16_GRAY",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gbr_load", "gbr", "", "20,string,GIMP");
  gimp_register_save_handler ("file_gbr_save", "gbr", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;
	GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_image = -1;

  if (strcmp (name, "file_gbr_load") == 0) {
		image_ID = load_image (param[1].data.d_string);

		if (image_ID != -1) {
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].data.d_image = image_ID;
		} else {
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
		*nreturn_vals = 2;
	}
  else if (strcmp (name, "file_gbr_save") == 0) {
		switch (run_mode) {
			case RUN_INTERACTIVE:
				/*  Possibly retrieve data  */
				gimp_get_data("file_gbr_save", &info);
				if (!save_dialog())
					return;
				break;
			case RUN_NONINTERACTIVE:  /* FIXME - need a real RUN_NONINTERACTIVE */
				if (nparams != 7)
					status = STATUS_CALLING_ERROR;
				if (status == STATUS_SUCCESS)
					{
					info.spacing = (param[5].data.d_int32);
				    strncpy (info.description, param[6].data.d_string, 256);	
					}
			    break;
			case RUN_WITH_LAST_VALS:
				gimp_get_data ("file_gbr_save", &info);
				break;
		}

		if (save_image (param[3].data.d_string, param[1].data.d_int32,
				param[2].data.d_int32)) {
			gimp_set_data ("file_gbr_save", &info, sizeof(info));
			values[0].data.d_status = STATUS_SUCCESS;
		} else
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		*nreturn_vals = 1;
	}
}

/* taken from app/depth/bfp.h */
#define ONE_BFP 32768
#define BFP_MAX_IN_FLOAT 65535/(gfloat)32768
#define BFP_MAX 65535
#define BFP_TO_FLOAT(x) (x / (gfloat) ONE_BFP)
#define FLOAT_TO_BFP(x) (x * ONE_BFP)

void
invert( gchar* buf, GPrecisionType precision, gint n )
{
  gint i;
  guint8  * u8 = (guint8*) buf;
  guint16  * u16 = (guint16*) buf;
  guint16  * f16 = (guint16*) buf;
  ShortsFloat u,u2;
  gfloat  * f = (gfloat*) buf;
  guint16  * bf = (guint16*) buf;
  switch (precision)
  {
    case PRECISION_U8:
         for(i = 0; i < n; ++i)
           u8[i] = 255 - u8[i];
         break;
    case PRECISION_U16:
         for(i = 0; i < n; ++i)
           u16[i] = 65535 - u16[i];
         break;
    case PRECISION_FLOAT:
         for(i = 0; i < n; ++i)
           f[i] = 1.0 - f[i];
         break;
    case PRECISION_FLOAT16:
         for(i = 0; i < n; ++i)
           f16[i] = FLT16( 1.0 - FLT( f16[i], u ), u2);
         break;
    case PRECISION_BFP:
         for(i = 0; i < n; ++i)
           bf[i] = FLOAT_TO_BFP( 1.0 - BFP_TO_FLOAT(bf[i]) );
         break;
  }
}

/* we want old brushes be loaded as usual */
#if WORDS_BIGENDIAN  
#define RnHFLT( x, t ) (t.s[0] = (x), t.s[1]= 0, t.f) 
#define RnHFLT16( x, t ) (t.f = (x), t.s[0])   
#else
#define RnHFLT( x, t ) (t.s[1] = (x), t.s[0]= 0, t.f) 
#define RnHFLT16( x, t ) (t.f = (x), t.s[1])    
#endif 

static gint32 load_image (char *filename) {
	char *temp;
	int fd;
	BrushHeader ph;
	gchar *buffer;
	gint32 image_ID, layer_ID;
	TileDrawable *drawable;
	unsigned int line;
	GPixelRgn pixel_rgn;
	gint bytes;
	gint num_channels;
	gint layer_type, image_type;
        int RnH_float = 0;

	temp = g_malloc(strlen (filename) + 11);
	sprintf(temp, "Loading %s:", filename);
	gimp_progress_init(temp);
	g_free (temp);

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	if (read(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return -1;
	}

	/*  rearrange the bytes in each unsigned int  */
	ph.header_size = ntohl(ph.header_size);
	ph.version = ntohl(ph.version);
	ph.width = ntohl(ph.width);
	ph.height = ntohl(ph.height);
	ph.type = ntohl(ph.type);
	ph.magic_number = ntohl(ph.magic_number);
	ph.spacing = ntohl(ph.spacing);

	if (ph.magic_number != GBRUSH_MAGIC || ph.version < 2 ||
			ph.header_size <= sizeof(ph)) {
		close(fd);
		return -1;
	}

	if (lseek(fd, ph.header_size - sizeof(ph), SEEK_CUR) !=
		CAST(long) ph.header_size) {
		close(fd);
		return -1;
	}
 
	/* In version 2 the type field was bytes */ 
	if (ph.version == 2) 
	{ 
		if (ph.type == 1)      /*in version 2, 1 byte is 8bit gray*/
		{
			ph.type = GRAY_IMAGE;
		}
		else if (ph.type == 3) /*in version 2, 3 bytes is 8bit color */
		{
			ph.type = RGB_IMAGE;
		}
		else
		{
			close(fd);
			return -1; 
		}
	}

        /* make a exception for IEEEHalf format since version 0.22-0 */
        if(ph.type == IEEEHALF_GRAY_IMAGE)
        {
          ph.type = FLOAT16_GRAY_IMAGE;
          RnH_float = 0;
        } else 
        if(ph.type == FLOAT16_GRAY_IMAGE)
        {
          RnH_float = 1;
        }

	switch (ph.type)
	{
		case RGB_IMAGE:
			image_type = RGB;
			layer_type = RGB_IMAGE;
			num_channels = 3;
			break;
		case RGBA_IMAGE:
			image_type = RGB;
			layer_type = RGBA_IMAGE;
			num_channels = 4;
			break;
		case GRAY_IMAGE:
			image_type = GRAY;
			layer_type = GRAY_IMAGE;
			num_channels = 1;
			break;
		case GRAYA_IMAGE:
			image_type = GRAY;
			layer_type = GRAYA_IMAGE;
			num_channels = 2;
			break;
			break;
		case U16_RGB_IMAGE:
			image_type = U16_RGB;
			layer_type = U16_RGB_IMAGE;
			num_channels = 3;
			break;
		case U16_RGBA_IMAGE:
			image_type = U16_RGB;
			layer_type = U16_RGBA_IMAGE;
			num_channels = 4;
			break;
		case U16_GRAY_IMAGE:
			image_type = U16_GRAY;
			layer_type = U16_GRAY_IMAGE;
			num_channels = 1;
			break;
		case U16_GRAYA_IMAGE:
			image_type = U16_GRAY;
			layer_type = U16_GRAYA_IMAGE;
			num_channels = 2;
			break;
		case FLOAT_RGB_IMAGE:
			image_type = FLOAT_RGB;
			layer_type = FLOAT_RGB_IMAGE;
			num_channels = 3;
			break;
		case FLOAT_RGBA_IMAGE:
			image_type = FLOAT_RGB;
			layer_type = FLOAT_RGBA_IMAGE;
			num_channels = 4;
			break;
		case FLOAT_GRAY_IMAGE:
			image_type = FLOAT_GRAY;
			layer_type = FLOAT_GRAY_IMAGE;
			num_channels = 1;
			break;
		case FLOAT_GRAYA_IMAGE:
			image_type = FLOAT_GRAY;
			layer_type = FLOAT_GRAYA_IMAGE;
			num_channels = 2;
			break;
		case FLOAT16_RGB_IMAGE:
			image_type = FLOAT16_RGB;
			layer_type = FLOAT16_RGB_IMAGE;
			num_channels = 3;
			break;
		case FLOAT16_RGBA_IMAGE:
			image_type = FLOAT16_RGB;
			layer_type = FLOAT16_RGBA_IMAGE;
			num_channels = 4;
			break;
		case FLOAT16_GRAY_IMAGE:
			image_type = FLOAT16_GRAY;
			layer_type = FLOAT16_GRAY_IMAGE;
			num_channels = 1;
			break;
       		case FLOAT16_GRAYA_IMAGE:
			image_type = FLOAT16_GRAY;
			layer_type = FLOAT16_GRAYA_IMAGE;
			num_channels = 2;
			break;
		default:
			close (fd);
			return -1;
	}
	
       /* Now there's just raw data left. */

       /*
 	* Create a new image of the proper size and associate the filename with it.
        */
	image_ID = gimp_image_new(ph.width, ph.height, image_type);
	gimp_image_set_filename(image_ID, filename);

	layer_ID = gimp_layer_new(image_ID, "Background", ph.width, ph.height,
			layer_type, 100, NORMAL_MODE);
	gimp_image_add_layer(image_ID, layer_ID, 0);

	drawable = gimp_drawable_get(layer_ID);
  	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);

	bytes = gimp_drawable_bpp (layer_ID);
	buffer = g_malloc(ph.width * bytes);


	for (line = 0; line < ph.height; line++) {
		if (read(fd, buffer, ph.width * bytes ) != ph.width * bytes) {
			close(fd);
			g_free(buffer);
			return -1;
		}
		/*buffer_to_host_order (buffer, ph.width, num_channels, ph.type);*/
                if(RnH_float)
                {
                  guint8 *data = (guint8*) buffer;
                  guint8 tmp;
                  int i;
                  for (i = 0; i < num_channels * ph.width * bytes; i += 2)
                  {
#ifndef WORDS_BIGENDIAN
                    tmp = data[i];
                    data[i] = data[i+1];
                    data[i+1] = tmp;
#endif 
                    {
                      gfloat f;
                      guint16 *f16 = (guint16*) &data[i];
                      ShortsFloat u;
                      /* old FLT macro */
                      f = RnHFLT(*f16,u);
                      *f16 = FLT16(f,u);
                    }
                  }
                }
                invert( buffer, gimp_drawable_precision( drawable->id ),
                        num_channels * ph.width );
		gimp_pixel_rgn_set_row(&pixel_rgn, buffer, 0, line, ph.width);
		gimp_progress_update((double) line / (double) ph.height);
	}
	gimp_drawable_flush(drawable);

	return image_ID;
}

static gint save_image (char *filename, gint32 image_ID, gint32 drawable_ID) {
	int fd;
	BrushHeader ph;
	unsigned char *buffer, *data;
	TileDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	char *temp;
	gint type = gimp_drawable_type (drawable_ID);

        switch (type)
        {
		case GRAY_IMAGE:
		case U16_GRAY_IMAGE:
		case FLOAT_GRAY_IMAGE:
		case FLOAT16_GRAY_IMAGE:
       		case BFP_GRAY_IMAGE:
                     break;
       		case GRAYA_IMAGE:
		case U16_GRAYA_IMAGE:
		case FLOAT_GRAYA_IMAGE:
		case FLOAT16_GRAYA_IMAGE:
       		case BFP_GRAYA_IMAGE:
                     gimp_image_flatten(image_ID);
                     break;
                default:
                     g_message(_("Need a flat Gray image."));
                     return 0;
        }

	temp = g_malloc(strlen (filename) + 48);
	sprintf(temp, "Saving as 16-bit Half %s:", filename);
	gimp_progress_init(temp);
	g_free(temp);

	drawable = gimp_drawable_get(drawable_ID);
	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, FALSE, FALSE);

	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == -1) {
		d_printf("Unable to open %s\n", filename);
		return 0;
	}

	ph.header_size = htonl(sizeof(ph) + strlen(info.description) + 1);
	ph.version = htonl(3);
	ph.width = htonl(drawable->width);
	ph.height = htonl(drawable->height);
	ph.type = htonl(IEEEHALF_GRAY_IMAGE);
	ph.magic_number = htonl(GBRUSH_MAGIC);
	ph.spacing = htonl(info.spacing);

	if (write(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return 0;
	}

	if (write(fd, info.description, strlen(info.description) + 1) !=
			strlen(info.description) + 1) {
		close(fd);
		return 0;
	}

	buffer = g_malloc(drawable->width * drawable->bpp);
        data   = g_malloc(drawable->width * 2);
	if (buffer == NULL || data == NULL) {
		close(fd);
		return 0;
	}
	for (line = 0; line < drawable->height; line++) {
		gimp_pixel_rgn_get_row(&pixel_rgn, buffer, 0, line, drawable->width);
		/*buffer_to_network_order (buffer, drawable->width, drawable_num_channels (drawable_ID), type);*/
                invert( buffer, gimp_drawable_precision( drawable->id ),
                        gimp_drawable_num_channels( drawable->id )
                        * drawable->width );
                {                
                  gint i, n = drawable->width;
                  guint8  * u8 = (guint8*) buffer;
                  guint16  * u16 = (guint16*) buffer;
                  guint16  * s16 = (guint16*) buffer;
                  guint16  * f16 = (guint16*) data;
                  ShortsFloat u,u2;
                  gfloat  * f = (gfloat*) buffer;
                  guint16  * bf = (guint16*) buffer;
                  GPrecisionType precision = gimp_drawable_precision( drawable->id );
                  switch (precision)
                  {
                    case PRECISION_U8:
                         for(i = 0; i < n; ++i)
                           f16[i] = FLT16( u8[i]/255., u);
                         break;
                    case PRECISION_U16:
                         for(i = 0; i < n; ++i)
                           f16[i] = FLT16( u16[i]/65535., u);
                         break;
                    case PRECISION_FLOAT:
                         for(i = 0; i < n; ++i)
                           f16[i] = FLT16( f[i], u);
                         break;
                    case PRECISION_FLOAT16:
                         for(i = 0; i < n; ++i)
                           f16[i] = s16[i];
                         break;
                    case PRECISION_BFP:
                         for(i = 0; i < n; ++i)
                           f16[i] = FLT16( BFP_TO_FLOAT(bf[i]), u );
                         break;
                  }
                }
		if (write(fd, data, drawable->width * 2) !=
				drawable->width * 2) {
			close(fd);
			return 0;
		}
		gimp_progress_update((double) line / (double) drawable->height);
	}
	g_free(buffer);
	g_free(data);

	close(fd);

	return 1;
}

static gint drawable_num_channels (guint drawable_ID) 
{
  gint size; 
  gint type = gimp_drawable_type (drawable_ID);
  TileDrawable *drawable = gimp_drawable_get(drawable_ID);

  switch (type)
  {
    case RGB_IMAGE:
    case RGBA_IMAGE:
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
	size = sizeof(guint8);	
      break;
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
    case IEEEHALF_GRAY_IMAGE:
	size = sizeof(guint16);	
      break;
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
	size = sizeof(gfloat);	
      break;
   } 
   return drawable->bpp/size;
}

static guchar * buffer_to_host_order ( guchar *buffer, 
					gint width, 
					gint num_channels, 
					gint image_type)
{
  gint i, k;
  guchar *c,tmp;
  switch (image_type)
  {
    case RGB_IMAGE:
    case RGBA_IMAGE:
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
      break;
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
	{
	  guint16 *b = (guint16 *)buffer;
	  for(i = 0 ; i < width; i++)
	    for( k = 0 ; k < num_channels; k++)
            {
#ifndef WORDS_BIGENDIAN
              c = b;
              tmp = c[0];
              c[0] = c[1];
              c[1] = tmp;
#endif
	      /*ntohs (*b);*/
              ++b;
            }
	}
      break;
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
	{
	  gfloat *b = (gfloat *)buffer;
	  for(i = 0 ; i < width; i++)
	    for( k = 0 ; k < num_channels; k++)
	      {
		ntohl ((gint32) *b); b++;
	      }
	}
      break;
   } 
   return buffer;
}

static guchar * buffer_to_network_order ( guchar *buffer, 
					gint width, 
					gint num_channels, 
					gint image_type)
{
  gint i, k;
  switch (image_type)
  {
    case RGB_IMAGE:
    case RGBA_IMAGE:
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
      break;
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
	{
	  guint16 *b = (guint16 *)buffer;
	  for(i = 0 ; i < width; i++)
	    for( k = 0 ; k < num_channels; k++)
	      htons (*b++);
	}
      break;
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
	{
	  gfloat *b = (gfloat *)buffer;
	  for(i = 0 ; i < width; i++)
	    for( k = 0 ; k < num_channels; k++)
	      {
		htonl ((gint32) *b); b++;
	      }
	}
      break;
   } 
   return buffer;
}

static gint save_dialog()
{
	GtkWidget *dlg;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *table;
	gchar **argv;
	gint argc;
	gchar buffer[12];

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("plasma");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	dlg = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg), "Save As Brush");
	gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
										 (GtkSignalFunc) close_callback, NULL);

	/*  Action area  */
	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
										 (GtkSignalFunc) ok_callback,
										 dlg);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
														(GtkSignalFunc) gtk_widget_destroy,
														GTK_OBJECT(dlg));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* The main table */
	/* Set its size (y, x) */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	/**********************
	 * label
	 **********************/
	label = gtk_label_new("Spacing:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	/************************
	 * The entry
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 200, 0);
	sprintf(buffer, "%i", info.spacing);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &info.spacing);
	gtk_widget_show(entry);

	/**********************
	 * label
	 **********************/
	label = gtk_label_new("Description:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	/************************
	 * The entry
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 200, 0);
	gtk_entry_set_text(GTK_ENTRY(entry), info.description);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, info.description);
	gtk_widget_show(entry);

	gtk_widget_show(dlg);

	gtk_main();
	gdk_flush();

	return run_flag;
}

static void close_callback(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

static void ok_callback(GtkWidget * widget, gpointer data)
{
	run_flag = 1;
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void entry_callback(GtkWidget * widget, gpointer data)
{
	if (data == info.description)
		strncpy(info.description, gtk_entry_get_text(GTK_ENTRY(widget)), 256);
	else if (data == &info.spacing)
		info.spacing = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
}
