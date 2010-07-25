/* HDR(Radiance) image loader (and saver) plugin 3.1
 *
 * by tim copperfield [timecop@japan.co.jp]
 * http://www.ne.jp/asahi/linux/timecop  
 *
 * saving&alpha by Sabine Wolf (sabine@lythande.de) 14.2.04
 * This plugin is not based on any other plugin.
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


#include <math.h>      
#include <setjmp.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "gtk/gtk.h"
#include "lib/plugin_main.h"
#include "lib/ui.h"
#include <libgimp/stdplugins-intl.h>
#include "fromrad.h"
//#include "app/image_render.h"

extern float image_render_get_expose() ;

#define SCALE_WIDTH 125
#define MAXWIDTH 8192
COLR cbuf[MAXWIDTH];
COLR abuf[MAXWIDTH];

unsigned short rbuf[MAXWIDTH];
unsigned short gbuf[MAXWIDTH];
unsigned short bbuf[MAXWIDTH];

typedef enum {
    RAW_RGB,			/* RGB Image */
    RAW_RGBA,			/* RGB Image with an Alpha channel */
    RAW_PLANAR,			/* Planar RGB */
    RAW_INDEXED			/* Indexed image */
} raw_type;

typedef enum {
    RAW_PALETTE_RGB,		/* standard RGB */
    RAW_PALETTE_BGR		/* Windows BGRX */
} raw_palette_type;

typedef struct {
    gint32 file_offset;		/* offset to beginning of image in raw data */
    gint32 image_width;		/* width of the raw image */
    gint32 image_height;	/* height of the raw image */
    raw_type image_type;	/* type of image (RGB, INDEXED, etc) */
    gint32 palette_offset;	/* offset inside the palette file, if any */
    raw_palette_type palette_type;	/* type of palette (RGB/BGR) */
} raw_config;

typedef struct {
    FILE *fp;			/* pointer to the already open file */
    TileDrawable *drawable;	/* gimp drawable */
    GPixelRgn region;	/* gimp pixel region */
    gint32 image_id;		/* gimp image id */
    guchar cmap[768];		/* color map for indexed images */
} raw_gimp_data;

raw_config *runtime;

/* Declare local functions.
 */
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


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

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
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in"}
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_hdr_load",
                          "loads files of the jpeg file format",
			  "FIXME: write help for hdr_load",
                          "timecop/Sabine Wolf",
                          "timecop/Sabine Wolf",
                          "1995-1996",
                          "<Load>/HDR",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_hdr_save",
                          "saves files in the HDR file format",
                          "FIXME: write help for hdr_save",
                          "Sabine Wolf",
                          "Sabine Wolf",
                          "1995-1996",
                          "<Save>/HDR",
                          "FLOAT_RGB",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL); 

  gimp_register_magic_load_handler ("file_hdr_load", "hdr,pic", "", "0,string,#?");
  gimp_register_save_handler ("file_hdr_save", "hdr,pic", "");
}

GStatusType status = STATUS_SUCCESS;
GParam * params = NULL;
gint32 image_id;
gint32 drawable_id;
GtkWidget* load_dialog;
int bit_depth = 16;


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

  run_mode = param[0].data.d_int32;

  INIT_I18N_UI();

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_hdr_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_hdr_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  break;

	case RUN_NONINTERACTIVE:

	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}



gint32 load_image(gchar * filename)
{
	int i,j,k;
    /* main struct with the image info in it */
    raw_gimp_data *data;

    /* stuff for the gimp image, local to this function */
    gint32 layer_id = -1;
    GImageType itype = FLOAT_RGB;
    GDrawableType ltype = FLOAT_RGB_IMAGE;
    unsigned char*  bufPtr = 0;
    unsigned char* abufPtr = 0;
  
    gint bpp = 0;
    // radience handling 
    FILE *inf, *alpha_inf;
    int xsize, ysize, aysize, axsize;
    float f;
    float r,g,b,a;
    gfloat *rowc=NULL;
    guchar *pixel =NULL;
    int begin,end,num;
    int tile_height;
    int has_alpha;

    /* allocate the raw_gimp_data structure */
    data = (raw_gimp_data *)g_malloc0(sizeof(raw_gimp_data));

    inf = fopen(filename,"rb");
    if(!inf) {
        fprintf(stderr,"Can't open %s\n",filename);
        exit(1);
    }
    alpha_inf = fopen(g_strconcat(filename,"_a",NULL),"rb");
    has_alpha = (alpha_inf==NULL) ? FALSE : TRUE;
    
    if (has_alpha) {
        ltype = FLOAT_RGBA_IMAGE;
    }    

    if (checkheader(inf,COLRFMT,(FILE *)NULL) < 0 ||
		fgetresolu(&xsize, &ysize, inf) < 0) {
        fprintf(stderr,"Input not in HDR-format\n");
        exit(1);
    }

    if (has_alpha) {
      if (checkheader(alpha_inf,COLRFMT,(FILE *)NULL) < 0 ||
		fgetresolu(&axsize, &aysize, alpha_inf) < 0 ||
                axsize!=xsize || aysize!=ysize) {
        fprintf(stderr,"Alpha-file not matching\n");
        fclose(alpha_inf);
        has_alpha=FALSE;
      }
    }




    /* create new gimp image */
    data->image_id =
		gimp_image_new(xsize, ysize, itype);

    /* set image filename */
    gimp_image_set_filename(data->image_id, filename);

    /* create new layer - local variable */
    layer_id =
		gimp_layer_new(data->image_id, _("Background"),
		      xsize, ysize, ltype,
		      100, NORMAL_MODE);
    gimp_image_add_layer(data->image_id, layer_id, 0);

    /* create gimp drawable for the image */
    data->drawable = gimp_drawable_get(layer_id);

    bpp = data->drawable->bpp;
    /* initialize pixel region */
    gimp_pixel_rgn_init(&data->region, data->drawable, 0, 0,
			xsize, ysize,
			TRUE, FALSE);
 
    /* load image */
  
    gimp_progress_init (g_strconcat(_("Loading "),filename,NULL));

    tile_height = gimp_tile_height(); 

    if ((pixel = g_malloc(tile_height * xsize * bpp))==NULL) {
        fprintf(stderr,"Kein Speicher");
        return FALSE;
    }

    rowc = (gfloat *)pixel; 


    for (begin = 0, end = tile_height;
         begin < ysize;
         begin += tile_height, end += tile_height)
    {
	if (end > ysize)
	    end = ysize;

	num = end - begin;
	
	k = 0;

        for(i=0;i<num;i++) 
	{
	    freadcolrs(cbuf,xsize,inf);
    
	    if (has_alpha)
                freadcolrs(abuf,xsize,alpha_inf);
           
	    // Pointer to gimp framebuffer
	    // unpack data for gimp

	    bufPtr = ((unsigned char*)cbuf);
            abufPtr = ((unsigned char*)abuf);
 
	    for (j = 0; j < xsize; j++)
	    {
		if(bufPtr[3]) {
		    f = ldexp(1.0, bufPtr[3]-(int)(128+8));
                   
		    r = (bufPtr[0]+0.5) * f;
		    g = (bufPtr[1]+0.5) * f;
		    b = (bufPtr[2]+0.5) * f;
                 
		    rowc[k++] = r;
		    rowc[k++] = g;
		    rowc[k++] = b;
 
		} else {     
		    rowc[k++] = 0.0f;
		    rowc[k++] = 0.0f;
		    rowc[k++] = 0.0f;
		}
                if (has_alpha) 
		{
		    if(abufPtr[3]) 
		    {
			f = ldexp(1.0, abufPtr[3]-(int)(128+8));

			a = abufPtr[0] * f;
			
			rowc[k++] = a;

		    } else 
		    {     
			rowc[k++] = 0.0f;
		    }
                
		}
		bufPtr += 4;
                abufPtr += 4;
	    }
	}
        gimp_pixel_rgn_set_rect (&data->region,pixel, 0, begin, xsize, num);
	gimp_progress_update((gfloat) begin / (gfloat) ysize);
    }       
    
    gimp_drawable_flush(data->drawable);
    gimp_drawable_detach(data->drawable);
    
    g_free(data);
    g_free(pixel);

    return data->image_id;
}


static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  raw_gimp_data *data;
  TileDrawable *drawable;
  GDrawableType drawable_type;
  
  FILE *outfile=NULL, *alphafile=NULL;
  char *name;
  char *afn;
  int has_alpha;
  
  int row;
  int width,height;
  int begin,end,num;
  int tile_height;

  gint bpp = 16;
 
  guchar *pixel = NULL;
  
  /* allocate the raw_gimp_data structure */
  data = (raw_gimp_data *)g_malloc0(sizeof(raw_gimp_data));

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width = drawable->width;
  height = drawable->height;
  bpp = drawable->bpp;
  
  switch (drawable_type) {
    case FLOAT_RGBA_IMAGE:
    case FLOAT_RGB_IMAGE: break;
	  
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
    default:
         g_message (_("HDR supports only 32-bit IEEE Float RGB image type."));
         return FALSE;
         break;
    }

  gimp_pixel_rgn_init (&data->region, drawable, 0, 0, width, height, FALSE, FALSE);

  name = malloc (strlen (filename) + 11);

  sprintf (name, "%s %s:",_("Saving"), filename);
  gimp_progress_init (name);
  free (name);

  if ((outfile = fopen (filename, "wb")) == NULL)
    {
      fprintf (stderr, "can't open %s\n", filename);
      return FALSE;
    }
 
 
  switch (drawable_type)
    {
    case FLOAT_RGB_IMAGE:
      has_alpha = FALSE;
      break;
    case FLOAT_RGBA_IMAGE:
      has_alpha = TRUE;
      break;
    default:
      /*gimp_message ("hdr: cannot operate on unknown image types");*/
      return FALSE;
      break;
    }

  if (has_alpha) {
    afn = malloc (strlen (filename));
    if  ((alphafile = fopen (g_strconcat(strncpy(afn,filename,
          strlen(filename)-8),"_a",NULL), "wb")) == NULL)
      {
	fprintf (stderr, "can't open alpha %s\n", filename);
	return FALSE;
      }
    free(afn);
  }
    fprintf(outfile,"#?RADIANCE\n");
    fprintf(outfile,"FORMAT=32-bit_rle_rgbe\n");
    fprintf(outfile,"EXPOSURE=1.0\n\n");
    fprintf(outfile,"-Y %d +X %d\n",height,width);
 
    if (has_alpha) {
      fprintf(alphafile,"#?RADIANCE\n");
      fprintf(alphafile,"FORMAT=32-bit_rle_rgbe\n");
      fprintf(alphafile,"EXPOSURE=1.0\n\n");
      fprintf(alphafile,"-Y %d +X %d\n",height,width);
    }
    
    tile_height = gimp_tile_height();

    pixel = g_malloc(tile_height * width * bpp);
   
    for (begin = 0, end = tile_height;
         begin < drawable->height;
         begin += tile_height, end += tile_height)
    {
	if (end > height)
	  end = height;

	num = end - begin;
	gimp_pixel_rgn_get_rect (&data->region, pixel, 0, begin, width, num);

        for (row = 0; row < num; row++)
        {      
	    if (fwritescan_a((float *)(pixel+row*width*bpp), width, outfile, alphafile)==-1) 
	    {
	        fprintf(stderr,"Konvertierfehler!");
                return FALSE;
	    }
        }  
       
	gimp_progress_update ((double) begin / (double) height);
    }

    fclose (outfile);
    if (has_alpha)
        fclose(alphafile);

    g_free (pixel);
    g_free (data);

    gimp_drawable_detach (drawable);

    return TRUE;
}

