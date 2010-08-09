/*
   Raw photo loader plugin for The GIMP
   by Dave Coffin at cybercom dot net, user dcoffin
   http://www.cybercom.net/~dcoffin/

   $Revision: 1.18 $
   $Date: 2008/05/22 04:22:40 $

   This code is licensed under the same terms as The GIMP.
   To simplify maintenance, it calls my command-line "dcraw"
   program to do the actual decoding.

   To install locally:
	gimptool --install rawphoto.c

   To install globally:
	gimptool --install-admin rawphoto.c

   To build without installing:
	gcc -o rawphoto rawphoto.c `gtk-config --cflags --libs` -lgimp -lgimpui
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#ifdef GIMP_VERSION
#error "NOT HERE"
#else
#include <lib/plugin_main.h>
#include <lib/ui.h>
#include <libgimp/stdplugins-intl.h>
#include <lib/dialog.h>
#include <lib/widgets.h>
#endif

#define PLUG_IN_VERSION  "1.1.5-dcr-8.37 - 19 April 2004 (cp 22. December 2006)"

static void query(void);
static void run(gchar *name,
		gint nparams,
		GimpParam *param,
		gint *nreturn_vals,
		GimpParam **return_vals);

static gint  load_dialog (gchar *name);
static gint32 load_image (gchar *filename);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_procedure */
  NULL,  /* quit_procedure */
  query, /* query_procedure */
  run,   /* run_procedure */
};

static struct {
  gboolean check_val[7];
  gfloat    spin_val[4];
  int      used_bits;
  gchar  i_profile[1024];         /* input profile */
  gchar  o_profile[1024];         /* output profile */
} cfg = {
  { FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE },
  { 2.2, 1.0, 1.0, 1.0 },
  16
};

MAIN ()

static void query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,      "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,     "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,     "raw_filename", "The name of the file to load" },
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,      "image",        "Output image" },
  };

  static gint num_load_args =
	sizeof load_args / sizeof load_args[0];
  static gint num_load_return_vals =
	sizeof load_return_vals / sizeof load_return_vals[0];
  int dependency_error = 0;

  #ifndef WIN32
  char *text = NULL;
  char *command = calloc(sizeof(char), 1024);
  if(!command){fprintf(stderr,"%s:%d Memory error\n",__FILE__,__LINE__);return;}

  if(strrchr(argv[0], G_DIR_SEPARATOR))
  {
    char *ptr = NULL;
    text = malloc(1024);
    sprintf(text, "%s", argv [0]);
    ptr = strrchr(text, G_DIR_SEPARATOR);
    ptr[1] = 0;
  }

  snprintf( command, 1024, "export PATH=~/.cinepaint/plug-ins:%s:$PATH; ufraw-cinepaint", text ? text : "");

  dependency_error = system(command);

  free(command);

  if(!(dependency_error > 0x200))
    fprintf(stderr, "The camera RAW converter UFRaw (ufraw-cinepaint) is installed. Disabling distributed rawphoto plug-in.\n");
  else
    fprintf(stderr, "UFRaw for CinePaint (ufraw-cinepaint) is not found. Enabling distributed rawphoto plug-in.\n");
  #endif


  if(dependency_error > 0x200)
  {
    gimp_install_procedure ("file_rawphoto_load",
			  "Loads raw digital camera files",
			  "This plug-in loads raw digital camera files.",
			  "Dave Coffin at cybercom dot net, user dcoffin",
			  "Copyright 2003 by Dave Coffin",
			  PLUG_IN_VERSION,
			  "<Load>/rawphoto",
			  NULL,
			  GIMP_PLUGIN,
			  num_load_args,
			  num_load_return_vals,
			  load_args,
			  load_return_vals);

    gimp_register_load_handler ("file_rawphoto_load",
    "bay,bmq,cr2,crw,cs1,dc2,dcr,dng,fff,jpg,k25,kdc,mos,mrw,nef,orf,pef,raf,raw,rdc,srf,sti,tif,tiff,x3f", "");
  }
}

char*
getTempFileName( const char* fname, const char* extension)
{
    char *tmpname = malloc(strlen(fname)+12);
    char *ptr = strrchr( fname, '/');
    sprintf(tmpname,"/tmp/%s", ptr ? ptr : fname);
    ptr = strchr( tmpname, '.' );
    if(ptr)
      ptr[1] = 0;
    strcpy( &tmpname[strlen(tmpname)], extension );
    return tmpname;
}

static char *dcraw_name = "dcraw";
static char *extra_path = NULL;

static void run (gchar *name,
		gint nparams,
		GimpParam *param,
		gint *nreturn_vals,
		GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType run_mode;
  GimpPDBStatusType status;
  gint32 image_id = -1;
  gchar *command, *fname;
  int stat;

  if(strrchr(argv[0], G_DIR_SEPARATOR))
  {
    char *ptr = NULL;
    extra_path = malloc(1024);
    sprintf(extra_path, "%s", argv [0]);
    ptr = strrchr(extra_path, G_DIR_SEPARATOR);
    ptr[1] = 0;
    sprintf( &extra_path[strlen(extra_path)], "..%sextra",
             G_DIR_SEPARATOR_S );
  } else
    fprintf(stderr, "no CinePaint %s found. (%s)\n", dcraw_name, argv[0]);

  INIT_I18N_UI();

  *nreturn_vals = 1;
  *return_vals = values;

  status = GIMP_PDB_CALLING_ERROR;
  if (strcmp (name, "file_rawphoto_load")) goto done;

  status = GIMP_PDB_EXECUTION_ERROR;
  fname = param[1].data.d_string;
  command = g_malloc (strlen(fname) + strlen(extra_path) + 128);
  if (!command) goto done;
/*
   Is the file really a raw photo?  If not, try loading it
   as a regular JPEG or TIFF.
 */
  sprintf (command, "PATH=~/.cinepaint/extra:%s:$PATH ; %s -i '%s'\n", extra_path, dcraw_name, fname);
  fputs (command, stderr); fputs ("\n", stderr);
  stat = system (command);
  g_free (command);
  if (stat) {
    if (stat > 0x200)
      g_message (_("The \"rawphoto\" plugin won't work because "
	"there is no \"%s\" executable in your path."), dcraw_name);
    if (!strcasecmp (fname + strlen(fname) - 4, ".jpg"))
      *return_vals = gimp_run_procedure2
	("file_jpeg_load", nreturn_vals, nparams, param);
    else
      *return_vals = gimp_run_procedure2
	("file_tiff_load", nreturn_vals, nparams, param);
    return;
  }
  gimp_get_data ("plug_in_rawphoto", &cfg);
  status = GIMP_PDB_CANCEL;
  run_mode = param[0].data.d_int32;
  if (run_mode == GIMP_RUN_INTERACTIVE)
    if (!load_dialog (param[1].data.d_string)) goto done;

  status = GIMP_PDB_EXECUTION_ERROR;
  image_id = load_image (param[1].data.d_string);

  if(cfg.check_val[6] || 1)
  {
    char *command = malloc(2048);
    GimpParam *vals = *return_vals;


    printf("%s %s\n", param[1].data.d_string, param[2].data.d_string);
    param[1].data.d_string = getTempFileName( param[2].data.d_string, "tiff" );
    printf("%s %s\n", param[1].data.d_string, param[2].data.d_string);
    *return_vals = gimp_run_procedure2
	("file_tiff_load", nreturn_vals, nparams, param);

    vals = *return_vals;
    if( vals[0].data.d_status == GIMP_PDB_SUCCESS )
      gimp_image_set_filename( vals[1].data.d_image, fname );

    sprintf(command, "rm -v %s", param[1].data.d_string);
    fputs (command, stderr); fputs ("\n", stderr);
    system( command );
    if(command) free(command);

    *nreturn_vals = 2;
    gimp_set_data ("plug_in_rawphoto", &cfg, sizeof cfg);
    return;
  } 

  if (image_id == -1) goto done;
  *nreturn_vals = 2;
  values[1].type = GIMP_PDB_IMAGE;
  values[1].data.d_image = image_id;
  status = GIMP_PDB_SUCCESS;
  gimp_set_data ("plug_in_rawphoto", &cfg, sizeof cfg);

done:
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

#define byteswap(c) do { t=(c)[0]; (c)[0]=(c)[1]; (c)[1]=t; } while(0)

static gint32 load_image (gchar *filename)
{
  int		tile_height, width, height, row, nrows, depth;
  int           bpp_gimp, bpp_ppm, px;
  FILE		*pfp;
  gint32	image, layer;
  GimpDrawable	*drawable;
  GimpPixelRgn	pixel_region;
  guchar	*pixel;
  char		*command, nl;
  guint16       *val;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  guchar        t;
#endif

  /*setlocale (LC_NUMERIC, "C");*/
  command = g_malloc (strlen(filename)+200);
  if (!command) return -1;
  /* gamma now is handled internally - Kai-Uwe */
  sprintf (command,
	"PATH=~/.cinepaint/extra:%s:$PATH ;%s -4 -v -c%s%s%s%s%s%s%s "/*-g %0.2f -b %0.2f -r %0.2f -l %0.2f*/" '%s'\n",extra_path, dcraw_name,
	cfg.check_val[0] ? " -q 0":"",
	cfg.check_val[1] ? " -h":"",
	cfg.check_val[2] ? " -f":"",
	cfg.check_val[3] ? " -d":"",
	cfg.check_val[4] ? " -a":"",
	cfg.check_val[5] ? " -w":"",
	cfg.check_val[6] ? " -o 3 -T":" -o 0 -T",
	/*cfg.spin_val[0], cfg.spin_val[1], cfg.spin_val[2], cfg.spin_val[3],*/
	filename );

  if(cfg.check_val[6] || 1)
  {
    char *fn = strrchr(filename,'/');
    char *ptr = getTempFileName( fn ? fn+1 : filename, "tiff" );

    sprintf( &command[strlen(command)-1], " > %s", ptr );
    fputs (command, stderr); fputs ("\n", stderr);
    system( command );

    if(ptr) free(ptr);
    return -1;
  }
  
  fputs (command, stderr); fputs ("\n", stderr);
  pfp = popen (command, "r");
  g_free (command);
  if (!pfp) {
    perror (dcraw_name);
    return -1;
  }

  if (fscanf (pfp, "P6 %d %d %d%c", &width, &height, &depth, &nl) != 4) {
    pclose (pfp);
    g_message ("Not a raw digital camera image.%d %d %d %d\n",width,height,depth,nl);
    return -1;
  }
 
  if(depth < 256) { bpp_ppm = 3; } else { bpp_ppm = 6; }

  image = gimp_image_new (width, height, bpp_ppm == 3 ? RGB : U16_RGB);
  if (image == -1) {
    pclose (pfp);
    g_message ("Can't allocate new image.\n");
    return -1;
  }

  gimp_image_set_filename (image, filename);

  /* Create the "background" layer to hold the image... */
  layer = gimp_layer_new (image, _("Background"), width, height,
			  bpp_ppm == 3 ? RGB_IMAGE : U16_RGB_IMAGE,
			  100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image, layer, 0);

  /* Get the drawable and set the pixel region for our load... */
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_region, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);
  bpp_gimp = pixel_region.bpp;

  if(bpp_gimp != bpp_ppm) {
    pclose (pfp);
    g_message ("Wrong depth for new image.\n");
    return -1;
  }

  /* Temporary buffers... */
  tile_height = get_lib_tile_height();
  pixel = g_new (guchar, tile_height * width * bpp_gimp);

  /* Load the image... */
  for (row = 0; row < height; row += tile_height) {
    nrows = height - row;
    if (nrows > tile_height)
	nrows = tile_height;
    fread (pixel, width * bpp_ppm, nrows, pfp);
    if(bpp_ppm == 6) for(px = 0; px < width * nrows * 3; px++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	byteswap(pixel + 2*px);
#endif
        /* changed brightness / gamma handling - Kai-Uwe */
        val = ((guint16*) &(pixel [2*px]));
        /* doing an simple bitshift here is is better undoable */
        if (cfg.used_bits < 16)
          *val = *val << (16 - cfg.used_bits);
        /* gamma */
        *val = pow ((float)*val/65535.0, 1.0/cfg.spin_val[0]) * 65535.0;
    }
    printf (".");
    gimp_pixel_rgn_set_rect (&pixel_region, pixel, 0, row, width, nrows);
  }

  pclose (pfp);
  g_free (pixel);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image;
}

/* this is set to true after OK click in any dialog */
gboolean result = FALSE;

static void callback_ok (GtkWidget * widget, gpointer data)
{
  result = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

#define NCHECK (sizeof cfg.check_val / sizeof (gboolean))

#include <gdk/gdkkeysyms.h>

gint load_dialog (gchar * name)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkObject *adj;
  GtkWidget *widget;
  int i;
  static const char *label[11];
  
  label[0] = _("Quick interpolation");
  label[1] = _("Half-size interpolation");
  label[2] = _("Four color interpolation");
  label[3] = _("Grayscale document");
  label[4] = _("Automatic white balance");
  label[5] = _("Camera white balance");
  label[6] = _("Convert to WideRGB");
  label[7] = _("Gamma");
  label[8] = _("Brightness");
  label[9] = _("Red Multiplier");
  label[10] = _("Blue Multiplier");


  if (!strcasecmp (name + strlen(name) - 4, ".dng"))
    cfg.used_bits = 16;

  gimp_ui_init ("rawphoto", TRUE);

  dialog = gimp_dialog_new (_("Raw Photo Loader 1.0"), "rawphoto",
			gimp_standard_help_func, "",
			GTK_WIN_POS_MOUSE,
			FALSE, TRUE, FALSE,
			_("OK"), callback_ok, NULL, NULL, NULL, TRUE,
			FALSE, _("Cancel"), gtk_widget_destroy, NULL,
			1, NULL, FALSE, TRUE, NULL);
  gtk_signal_connect
	(GTK_OBJECT(dialog), "destroy", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  table = gtk_table_new (8, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER(table), 6);
  gtk_box_pack_start
	(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (i=0; i < NCHECK; i++) {
    widget = gtk_check_button_new_with_label
	(label[i]);
    gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON (widget), cfg.check_val[i]);
    gtk_table_attach
	(GTK_TABLE(table), widget, 0, 2, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_signal_connect (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			&cfg.check_val[i]);
    gtk_widget_show (widget);
  }
# if 0
  for (i=NCHECK; i < NCHECK+4; i++) {
    widget = gtk_label_new (_(label[i]));
    gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);
    gtk_misc_set_padding   (GTK_MISC (widget), 10, 0);
    gtk_table_attach
	(GTK_TABLE(table), widget, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (widget);
    widget = gimp_spin_button_new
	(&adj, cfg.spin_val[i-NCHECK], 0.01, 4.0, 0.01, 0.1, 0.1, 0.1, 2);
    gtk_table_attach
	(GTK_TABLE(table), widget, 1, 2, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			GTK_SIGNAL_FUNC (gimp_float_adjustment_update),
			&cfg.spin_val[i-NCHECK]);
    gtk_widget_show (widget);
  }

  /* look in lib/widgets.h Kai-Uwe */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 11,
                              _("bit depth:"), 100, 0,
                              cfg.used_bits, 8, 16, 1, 1, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
                      &cfg.used_bits);
# endif


  gtk_widget_show (dialog);

  gtk_main();
  gdk_flush();

  return result;
}



