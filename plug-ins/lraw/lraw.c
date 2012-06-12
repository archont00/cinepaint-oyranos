/*
 *   CameraRAW lraw filter for CinePaint
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <oyranos.h>
#include <alpha/oyranos_alpha.h>
#include <alpha/oyranos_cmm.h>   /* for hacking into module API */

/** Function oyConversion_FromImageFileName
 *  @brief   generate a Oyranos graph from a image file name
 *
 *  @param[in]     file_name           name of image file
 *  @param[in]     flags               for inbuild defaults |
 *                                     oyOPTIONSOURCE_FILTER;
 *                                     for options marked as advanced |
 *                                     oyOPTIONATTRIBUTE_ADVANCED |
 *                                     OY_SELECT_FILTER |
 *                                     OY_SELECT_COMMON
 *  @param[in]     data_type           the desired data type for output
 *  @param[in]     obj                 Oyranos object (optional)
 *  @return                            generated new graph, owned by caller
 *
 *  @version Oyranos: 0.3.0
 *  @since   2011/04/06 (Oyranos: 0.3.0)
 *  @date    2011/04/06
 */
oyConversion_s * oyConversion_FromImageFileName  (
                                       const char        * file_name,
                                       uint32_t            flags,
                                       oyDATATYPE_e        data_type,
                                       oyObject_s          obj )
{
  oyFilterNode_s * in, * out;
  int error = 0;
  oyConversion_s * conversion = 0;
  oyOptions_s * options = 0;

  if(!file_name)
    return NULL;

  /* start with an empty conversion object */
  conversion = oyConversion_New( obj );
  /* create a filter node */
  in = oyFilterNode_NewWith( "//" OY_TYPE_STD "/file_read.meta", 0, obj );
  /* set the above filter node as the input */
  oyConversion_Set( conversion, in, 0 );

  /* add a file name argument */
  /* get the options of the input node */
  if(in)
  options = oyFilterNode_GetOptions( in, OY_SELECT_FILTER );
  /* add a new option with the appropriate value */
  error = oyOptions_SetFromText( &options, "//" OY_TYPE_STD "/file_read/filename",
                                 file_name, OY_CREATE_NEW );
  /* release the options object, this means its not any more refered from here*/
  oyOptions_Release( &options );

  /* add a closing node */
  out = oyFilterNode_NewWith( "//" OY_TYPE_STD "/output", 0, obj );
  error = oyFilterNode_Connect( in, "//" OY_TYPE_STD "/data",
                                out, "//" OY_TYPE_STD "/data", 0 );
  /* set the output node of the conversion */
  oyConversion_Set( conversion, 0, out );

  return conversion;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <plugin_main.h>

#define PLUG_IN_VERSION  "1.0 - 28 August 2011 (cp 0.25 2011)"

static void query(void);
static void run(gchar *name,
		gint nparams,
		GimpParam *param,
		gint *nreturn_vals,
		GimpParam **return_vals);

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
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,      "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,     "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,     "raw_filename", "The name of the file to load" },
  };
  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE,      "image",        "Output image" },
  };

  static gint n_args = sizeof args / sizeof args[0];
  static gint n_return_vals = sizeof return_vals / sizeof return_vals[0];
  int dependency_error = 0;

  #ifndef WIN32
  char *text = NULL;
  char *command = (char*) calloc(sizeof(char), 1024);
  if(!command){fprintf(stderr,"%s:%d Memory error\n",__FILE__,__LINE__);return;}

  if(strrchr(argv[0], G_DIR_SEPARATOR))
  {
    char *ptr = NULL;
    text = (char*)malloc(1024);
    sprintf(text, "%s", argv [0]);
    ptr = strrchr(text, G_DIR_SEPARATOR);
    ptr[1] = 0;
  }

  snprintf( command, 1024, "export PATH=~/.cinepaint/plug-ins:%s:$PATH; ufraw-cinepaint", text ? text : "");

  dependency_error = system(command);

  free(command);

  if(!(dependency_error > 0x200))
    fprintf(stderr, "The camera RAW converter UFRaw (ufraw-cinepaint) is installed. Disabling distributed lraw plug-in.\n");
  else
    fprintf(stderr, "UFRaw for CinePaint (ufraw-cinepaint) is not found. Enabling lraw plug-in.\n");
  #endif


  if(dependency_error > 0x200)
  {
    gimp_install_procedure ("file_lraw_load",
			  "Load cameraRAW files",
			  "This plug-in loads raw digital camera files using lraw.",
			  "ku.b@gmx.de",
			  "Copyright 2011 Kai-Uwe Behrmann",
			  PLUG_IN_VERSION,
			  "<Load>/lraw",
			  NULL,
			  GIMP_PLUGIN,
			  n_args,
			  n_return_vals,
			  args,
			  return_vals);

    gimp_register_load_handler ("file_lraw_load",
    "bay,bmq,cr2,crw,cs1,dc2,dcr,dng,fff,jpg,k25,kdc,mos,mrw,nef,orf,pef,raf,raw,rdc,srf,sti,tif,tiff,x3f", "");
  }
}


static void run (gchar *name,
		gint nparams,
		GimpParam *param,
		gint *nreturn_vals,
		GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpPDBStatusType status;
  gint32 image_id = -1;

  *nreturn_vals = 1;
  *return_vals = values;

  status = GIMP_PDB_CALLING_ERROR;
  if (strcmp (name, "file_lraw_load"))
    goto clean;

  status = GIMP_PDB_EXECUTION_ERROR;
  image_id = load_image (param[1].data.d_string);

  if (image_id == -1)
    goto clean;
  *nreturn_vals = 2;
  values[1].type = GIMP_PDB_IMAGE;
  values[1].data.d_image = image_id;
  status = GIMP_PDB_SUCCESS;
  gimp_set_data ("plug_in_lraw", &cfg, sizeof cfg);

  clean:
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static gint32 load_image (gchar *filename)
{
  int           w, h, y, ny;
  GImageType    image_type;
  GDrawableType layer_type;
  gint32        image,
                layer;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_region;
  guchar       *pixel;

  oyProfile_s    * p;
  oyConversion_s * c;
  oyImage_s      * oy_image;

  char * text = (char*)malloc(strlen(filename)+24);

  sprintf (text, "%s ...", filename);
  gimp_progress_init (text);
  free(text);

  gimp_progress_update( 0.0 );

  c = oyConversion_FromImageFileName( filename, 0, oyUINT16, 0 );
  gimp_progress_update( .1 );
  oy_image = oyConversion_GetImage( c, OY_OUTPUT );
  gimp_progress_update( .5 );


  if(!oy_image)
  {
    g_message ("Can't obtain new image.\n");
    return -1;
  }

  w = oy_image->width;
  h = oy_image->height;
  if(oyToDataType_m(oyImage_GetPixelLayout(oy_image)) == oyUINT16)
  {
    image_type = U16_RGB;
    layer_type = U16_RGB_IMAGE;
  } else {
    image_type = RGB;
    layer_type = RGB_IMAGE;
  }

  image = gimp_image_new (w, h, image_type);
  if (image == -1) {
    g_message ("Can't create new image.\n");
    return -1;
  }

  gimp_image_set_filename (image, filename);

  /* Create the "background" layer to hold the image... */
  layer = gimp_layer_new (image, _("Background"), w, h, layer_type,
			  100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image, layer, 0);

  /* Get the drawable and set the pixel region for our load... */
  drawable = gimp_drawable_get (layer);
  gimp_progress_update( .6 );
  gimp_pixel_rgn_init (&pixel_region, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);

  {
    int is_allocated = 0;
    y = 0;
    gimp_progress_update( .7 );
    pixel = (guchar*)oy_image->getLine( oy_image, y, &ny,
                                        -1, &is_allocated );

    /* this is a hack, but should work prefectly as long as lraw is used. */
    ny = h;
    gimp_progress_update( .8 );
    gimp_pixel_rgn_set_rect (&pixel_region, pixel, 0, y, w, ny);
    if(is_allocated)
      free( pixel );
  }

  gimp_progress_update( .9 );
  p = oyImage_GetProfile( oy_image );
  /* fallback */
  if(!p)
    p = oyProfile_FromStd( oyASSUMED_RGB, 0 );
    
  if(p)
  {
    size_t size = 0;
    oyPointer data = oyProfile_GetMem( p, &size, 0, malloc );
    if(size && data)
      gimp_image_set_icc_profile_by_mem(image, size, (gchar*)data,
                                        ICC_IMAGE_PROFILE);
  }

  oyImage_Release( &oy_image );
  oyConversion_Release( &c );

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  gimp_progress_update( 1.0 );

  return image;
}

