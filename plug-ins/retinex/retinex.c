/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Retinex plug-in -- 
 * Copyright (C) 2003 Fabien Pelisson <Fabien.Pelisson@inrialpes.fr>
 * Modified by (C) Edward Kok, 2007 for Cinepaint 0.21
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
    
#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

#ifndef GIMP_VERSION
#include "rint.h"
#include "widgets.h"
#endif

#define PLUG_IN_VERSION "0.10"

/* Set max allowable no of filters. Bottom 3 
are unique to the GIMP version */
#define MAX_RETINEX_SCALES 8
#define SCALE_WIDTH         150
#define MIN_GAUSSIAN_SCALE   16
#define MAX_GAUSSIAN_SCALE  250
#define ENTRY_WIDTH           4


/* Constants... */
/* Define which filters and functions to set off 
   the level (approximately close to Gaussian) */
#define RETINEX_UNIFORM 0
#define RETINEX_LOW 1
#define RETINEX_HIGH 2


/* The structure utilises the normalisation of color to manage 
   the parameters of algorithm. */
typedef struct 
{
  gint scale;
  gint nscales; 
  gint scales_mode; 
  gdouble cvar;  /* gboolean preview; */
} RetinexParams;

typedef struct 
{
  /* Indicate the number of the coefficients utilizing the algorithm. 
     The recursive filter utilises 3 coefficients but there are 
     implementations with 5 coefficients. The number is important to 
     augment the quality of the filter but especially so for the small 
     scales. */
  gint N; 
  /* Internal parameters for filters */
  gfloat sigma; 
  gdouble B;
  gdouble b[4]; 
} gauss3_coefs;

typedef struct
{
  gint run;
} RetinexInterface;

/* local function prototypes */
static gfloat RetinexScales[MAX_RETINEX_SCALES]; 
static void compute_coefs3 (gauss3_coefs *c, gfloat sigma);


/*   Functions of interface compulsory for the plugin to start  */
static void plugin_query(void);

static void plugin_run(gchar *name,
		       gint nparams,
		       GimpParam *param,
		       gint *nreturn_vals,
		       GimpParam **return_vals);

static gint retinex_dialog(void);

static void retinex(TileDrawable *drawable);

static void retinex_scales_distribution(gfloat *scales, gint nscales, gint mode, gint s);

static void     gausssmooth                 (gfloat       *in,
                                             gfloat       *out,
                                             gint          size,
                                             gint          rowtride,
                                             gauss3_coefs *c);

static void compute_mean_var(gfloat *src, gfloat *mean, gfloat *var, gint size, gint channels_n);


/* These functions are associated with the algorithm for the normalisation of color */
static void     MSRCR                       (gfloat       *src,
                                             gint          width,
                                             gint          height,
                                             gint          channels_n);

static void retinex_ok_callback(GtkWidget *widget, gpointer data);
static void retinex_button_callback(GtkWidget *widget, gpointer value);


/* Global variables */
/** To set default values for Retinex dialog**/
static RetinexParams rvals = 
  {
    240, /* Scale */
    3, /* Number of scales*/
    RETINEX_UNIFORM, /* Scales set to Uniform */
    1.2 /* Dynamic */
    /*True Default is to update the preview. Preview unsupported in Cinepaint */
  };

static RetinexInterface retint = {  FALSE }; 

GPlugInInfo PLUG_IN_INFO = 
  {
    NULL, /* init_proc */
    NULL, /* quit_proc */
    plugin_query, /* query_proc */
    plugin_run, /* run_proc */
  };


/*  Plug-in Interfaces  */
/*  Macro function defines the function call
main (gimp_main) in libgimp/gimp.h  */
MAIN()


/*  All the necessary commands to record the plugin in the PDB 
    (procedural database) of GIMP. Ported over to Cinepaint and emulated.

    The important function is gimp_install_procedure which indicates to 
    Cinepaint, the name of the plugin, the place in the menus, the plugin/version
    installed, the type of images allowable.. etc.
*/
static void plugin_query(void)
{
  static GimpParamDef args[] = 
    {
      /* The first three parameters are necessary for sub-menu placement */
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "(unused)" },
      { PARAM_DRAWABLE, "drawable", "Drawable input"},
      { PARAM_INT32, "scale", "Largest scale value"},
      { PARAM_INT32, "nscales", "Number of retinex scales"},
      { PARAM_INT32, "scales_mode", "Retinex distribution through scales"},
      { PARAM_FLOAT, "cvar", "variance value"}
    };

  static GimpParamDef *return_vals = NULL;
  static gint nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;


  /** Installation of procedure into procedure database **/
  gimp_install_procedure("plug_in_retinex",
			 "Retinex Image Enhancement Algorithm",
                         "The Retinex Image Enhancement Algorithm is an "
                         "automatic image enhancement method that enhances "
                         "a digital image in terms of dynamic range "
                         "compression, color independence from the spectral "
                         "distribution of the scene illuminant, and "
                         "color/lightness rendition.",
			             "Fabien Pelisson",
                                   "2003",
                         "Modified by Edward Kok for Cinepaint, 2007",
			             /* Position in the menus, sub-menus */
			             "<Image>/Filters/Render/Retinex",
			             /* One can modify the algo to work to take into account alpha
                         channel (transparency) RGBA and/or only the information of 
                         intensity for the format. Modified and commented out: GRAY, GRAYA. */
                         "RGB*, GRAY*, U16_RGB*, U16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
			             PROC_PLUG_IN,
               			 nargs, nreturn_vals,
			             args, return_vals);
}


/* Actual Retinex function */
static void plugin_run(gchar *name,
		       gint nparams,
		       GimpParam *param,
		       gint *nreturn_vals,
		       GimpParam **return_vals)
{
  static GimpParam values[1];
  TileDrawable *active_drawable;
  gint32 run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32; 

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;
  

  /* Recover the identifier of the current image. 
    This information is provided to us directly by Cinepaint. */
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);

  gimp_tile_cache_ntiles (2 * (active_drawable->width / gimp_tile_width () + 1));

  /* Execution method? By the menu (INTERACTIVE) or the direct 
  call via the PDB (NONINTERACTIVE)   */
  switch(run_mode)
    {
    case RUN_INTERACTIVE:
      /* retrieve data */
      gimp_get_data("plug_in_retinex", &rvals); 
      /* Launch the graphic interface and check input */
      if(!retinex_dialog()) 
          return;
      break;

      /* Check for num of parameters */
    case RUN_NONINTERACTIVE:
      if(nparams != 7)
	    {
	        status = STATUS_CALLING_ERROR;
	    }
      else
	    {
            rvals.scale   = param[3].data.d_int32;
	        rvals.nscales = param[4].data.d_int32;
	        rvals.scales_mode = param[5].data.d_int32;
	        rvals.cvar = param[6].data.d_float;
	    }
      break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data("plug_in_retinex", &rvals); 
      break;

    default:
      break;
    }

  /*  Verify if the plugin has been registered and the format of 
    the image is correct to launch the algorithm on the current image.  */
    if((status == STATUS_SUCCESS) && (gimp_drawable_is_rgb (active_drawable->id)))
    {
      gimp_progress_init ("Retinex...");
      retinex (active_drawable);
      gimp_progress_init ("Retinex (4/4): Updated...");

      /*updated of posting only in graphic mode*/
      if(run_mode != RUN_NONINTERACTIVE)
	  gimp_displays_flush();

      /* Store data */
      if(run_mode == RUN_INTERACTIVE)
      {
	   gimp_set_data("plug_in_retinex", &rvals, sizeof(RetinexParams));  
	  }
    }
  else
      /*Bad formatting error handler */
    { 
      g_message("Retinex: Incorrect image format.");
      status = STATUS_CALLING_ERROR;
    }

  values[0].type = PARAM_STATUS;
  /* allow access to current image */
  values[0].data.d_status = status;
  gimp_drawable_detach(active_drawable);
}

/* Apply the algorithm to the image */
static void retinex(TileDrawable* drawable)		    
{
  gint x1, y1, x2, y2;
  gint width , height;
  gint          size, bytes;
  guchar       *src  = NULL;
  float        *fl   = NULL;
  GimpPixelRgn  dst_rgn, src_rgn;
  GPrecisionType precision;
  int channels_n = gimp_drawable_num_channels(drawable->id);
  int j;

  bytes = drawable->bpp;

  /* Obtain size of the current image or its selection. 
     Transformation of the image/selection into float.  */
  gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
  width = (x2 - x1);
  height = (y2 - y1);

  /* Memory allocation */
  size = width * height;
  src = g_malloc (sizeof (char) * size * bytes);
  fl = g_malloc( sizeof(float) * size * channels_n );

  //memset (src, 0, sizeof (guchar) * size);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  
  precision = gimp_drawable_precision(drawable->id);
  /* convert to float 1...256 (??? strange) 
     TODO: Normally we would go to 0...1, but then we would take hands on the 
     algorythm, which I am not keen at this point. (kai-uwe)
   */
  {
      guint16 *s16 = (guint16*)src;
      gfloat *f32 = (gfloat*)src;
      gfloat f;
      ShortsFloat u;
      guint16 bfp;
      guint8 *s8 = (guint8*)src;
      int csize = size * channels_n;

/* taken from app/depth/bfp.h */
#define ONE_BFP 32768
#define BFP_MAX 65535
#define BFP_TO_FLOAT(x) (x / (gfloat) ONE_BFP)
#define FLOAT_TO_BFP(x) (x * ONE_BFP)

      switch(precision)
      {
        case PRECISION_U8:
  	    for (j = 0; j < csize; j++)
	      fl[j] = (float)*s8++;
          break;
        case PRECISION_U16:
  	    for (j = 0; j < csize; j++)
	      fl[j] = (float)((*s16++)/257);
          break;
        case PRECISION_FLOAT16:
  	    for (j = 0; j < csize; j++)
              fl[j] = FLT(*s16++,u) * 255.;
          break;
        case PRECISION_FLOAT:
  	    for (j = 0; j < csize; j++)
              fl[j] = *f32++ * 255.;
          break;
        case PRECISION_BFP:
  	    for (j = 0; j < csize; j++)
            {
              bfp = *s16++;
              fl[j] = BFP_TO_FLOAT(bfp) * 255.;
            }
          break;
      }
  }

  /* Algorithm for MSRCR */
  /* As the algorythm is completely in the float domain we simply convert to and
     from. 
     For instance in blur.c every bit depth has its own implementation.
     (kai-uwe)
   */
  MSRCR (fl, width, height, channels_n);

  /* and back to native precision (kai-uwe) */
  {
      guint16 *s16 = (guint16*)src;
      gfloat *f32 = (gfloat*)src;
      gfloat f;
      ShortsFloat u;
      guint16 bfp;
      guint8 *s8 = (guint8*)src;
      int csize = size * channels_n;
      printf("%s:%d %d\n",__FILE__,__LINE__,channels_n);

      switch(precision)
      {
        case PRECISION_U8:
  	    for (j = 0; j < csize; j++)
	      *s8++ = CLAMP(fl[j], 0, 255);
          break;
        case PRECISION_U16:
 	    for (j = 0; j < csize; j++)
	      *s16++ = CLAMP(fl[j]*257, 0, 65535);
          break;
        case PRECISION_FLOAT16:
  	    for (j = 0; j < csize; j++)
              *s16++ = (FLT16(fl[j]/255.,u));
          break;
        case PRECISION_FLOAT:
  	    for (j = 0; j < csize; j++)
              *f32++ = fl[j] / 255.;
          break;
        case PRECISION_BFP:
  	    for (j = 0; j < csize; j++)
	      *s16++ = FLOAT_TO_BFP(CLAMP(fl[j] / 255., 0.0, 2.0));
          break;
      }
  }
  gimp_pixel_rgn_init (&dst_rgn, drawable, x1, y1, width, height, TRUE, TRUE);
  gimp_pixel_rgn_set_rect (&dst_rgn, src, x1, y1, width, height);

  gimp_progress_update (1.0);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, width, height);

  g_free (src);
  g_free (fl);
}
 
/*  This function calculates the values of the various scale values according to the desired distribution. */
static void retinex_scales_distribution(gfloat* scales, gint nscales, gint mode, gint s)
{
  if(nscales == 1)
    { /* For one filter, median scale used */
      scales[0] = (gint) s / 2;
    }
  else if(nscales == 2)
    { /* For two filters, median + maximum scales used */
      scales[0] = (gint) s / 2;
      scales[1] = (gint) s;
    }
  else
    {
      gfloat size_step = (gfloat) s / (gfloat) nscales;
      gint i;

      switch(mode)
	  {	  
	    case RETINEX_UNIFORM:
	    {
	       for(i = 0; i < nscales; ++i)
	       {
		    scales[i] = 2. + (gfloat) i * size_step;
	       }
	    }
	    break;
	
        case RETINEX_LOW:
	    {
	       size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
	       for(i = 0; i < nscales; ++i)
	       {
		    scales[i] = 2. + pow(10,(i*size_step)/log(10));
	       }
	    }
	    break;
	    
        case RETINEX_HIGH:
	    {
	       size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
	       for(i = 0; i < nscales; ++i)
	       {
		    scales[i] = s - pow(10,(i*size_step)/log(10));
	       }
	    }
	    break;
	
        default:
	    break;
	  }
    }  
}



static void gausssmooth (gfloat *in, gfloat *out, gint size, gint rowstride, gauss3_coefs *c)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 9a        forward filter
   *          9b        backward filter
   *          fig7      algorithm
   */
  gint i,n, bufsize;
  gfloat *w1,*w2;

  /* first pass done in the forward direction*/
  bufsize = size+3;
  size -= 1;
  w1 = (gfloat *) g_malloc (bufsize * sizeof (gfloat));
  w2 = (gfloat *) g_malloc (bufsize * sizeof (gfloat));
  w1[0] = in[0];
  w1[1] = in[0];
  w1[2] = in[0];
  for ( i = 0 , n=3; i <= size ; i++, n++)
    {
      w1[n] = (gfloat)(c->B*in[i*rowstride] +
                       ((c->b[1]*w1[n-1] +
                         c->b[2]*w1[n-2] +
                         c->b[3]*w1[n-3] ) / c->b[0]));
    }

  /* backward pass */
  w2[size+1]= w1[size+3];
  w2[size+2]= w1[size+3];
  w2[size+3]= w1[size+3];
  for (i = size, n = i; i >= 0; i--, n--)
    {
      w2[n]= out[i * rowstride] = (gfloat)(c->B*w1[n] +
                                           ((c->b[1]*w2[n+1] +
                                             c->b[2]*w2[n+2] +
                                             c->b[3]*w2[n+3] ) / c->b[0]));
    }

  g_free (w1);
  g_free (w2);
}


/* MSRCR Algorithm
 * (a)  Filterings at several scales and sumarize the results.
 * (b)  Calculation of the final values. */
static void MSRCR (gfloat *src, gint width, gint height, gint channels_n)
{

  gint          scale,row,col;
  gint          i,j;
  gint          size;
  gint          pos;
  gint          channel;
  gfloat       *psrc = NULL;            /* backup pointer for src buffer */
  gfloat       *dst  = NULL;            /* float buffer for algorithm */
  gfloat       *pdst = NULL;            /* backup pointer for float buffer */
  gfloat       *in, *out;
  gint          channelsize;            /* Float memory cache for one channel */
  gfloat        weight;
  gauss3_coefs  coef;
  gfloat        mean, var;
  gfloat        mini, range, maxi;
  gfloat        alpha;
  gfloat        gain;
  gfloat        offset;
  gdouble       max_preview = 0.0;

  gimp_progress_init (_("Retinex: Filtering..."));
  max_preview = 3 * rvals.nscales;

  /* Allocates all the memory req by algorithm*/
  size = width * height * channels_n;
  dst = g_malloc (size * sizeof (gfloat));

  //memset (dst, 0, size * sizeof (gfloat));

  channelsize  = (width * height);
  in  = (gfloat *) g_malloc (channelsize * sizeof (gfloat));

  out  = (gfloat *) g_malloc (channelsize * sizeof (gfloat));

  /* Calculate the scales of filtering according to 
  the number of filters and their distribution. */
  retinex_scales_distribution (RetinexScales, rvals.nscales, rvals.scales_mode, rvals.scale);

  /*  Filtering set according to the various scales.
      Summarize the results of the various filters according to a
      specific weight(here equivalent for all). */
  weight = 1./ (gfloat) rvals.nscales;

  /* The recursive filtering algorithm needs different coefficients according
    to the selected scale (~ = standard deviation of Gaussian).   */
  pos = 0;
  for (channel = 0; channel < 3; channel++)
    {
      for (i = 0, pos = channel; i < channelsize ; i++, pos += channels_n)
         {
            /* Normalise scale fr. 0-255 => 1-256 */
            in[i] = src[pos] + 1.0;
         }
      for (scale = 0; scale < rvals.nscales; scale++)
        {
          compute_coefs3 (&coef, RetinexScales[scale]);
          /*  Filtering (smoothing) Gaussian recursive.
              Filter rows first  */
          for (row=0 ;row < height; row++)
            {
              pos =  row * width;
              gausssmooth (in + pos, out + pos, width, 1, &coef);
            }

          memcpy(in,  out, channelsize * sizeof(gfloat));
          memset(out, 0  , channelsize * sizeof(gfloat));

          /*  Filtering (smoothing) Gaussian recursive.
           *  Second columns  */
          for (col=0; col < width; col++)
            {
              pos = col;
              gausssmooth(in + pos, out + pos, height, width, &coef);
            }

          /*  Summarize the filtered values.
              In fact one calculates a ratio between the original values and the filtered values.           */
          for (i = 0, pos = channel; i < channelsize; i++, pos += channels_n)
            {
              dst[pos] += weight * (log (src[pos] + 1.) - log (out[i]));
            }

           gimp_progress_update ((channel * rvals.nscales + scale) / max_preview);
        }
    }
  g_free(in);
  g_free(out);

  /*  Final calculation with original value and cumulated filter values.
      The parameters gain, alpha and offset are constants. */
  /* Ci(x,y)=log[a Ii(x,y)]-log[ Ei=1-s Ii(x,y)] */

  alpha  = 128.;
  gain   = 1.;
  offset = 0.;

  for (i = 0; i < size; i += channels_n)
    {
      gfloat logl;

      psrc = src+i;
      pdst = dst+i;

      logl = log(psrc[0] + psrc[1] + psrc[2] + 3.);

      pdst[0] = gain * ((log(alpha * (psrc[0]+1.)) - logl) * pdst[0]) + offset;
      pdst[1] = gain * ((log(alpha * (psrc[1]+1.)) - logl) * pdst[1]) + offset;
      pdst[2] = gain * ((log(alpha * (psrc[2]+1.)) - logl) * pdst[2]) + offset;
    }

  /*  Adapt the dynamics of the colors according to the statistics of the first and second order.
      The use of the variance makes it possible to control the degree of saturation of the colors. */
  pdst = dst;

  compute_mean_var (pdst, &mean, &var, size, channels_n);
  mini = mean - rvals.cvar*var;
  maxi = mean + rvals.cvar*var;
  range = maxi - mini;

  if (!range)
    range = 1.0;

  for (i = 0; i < size; i+= channels_n)
    {
      psrc = src + i;
      pdst = dst + i;

      for (j = 0 ; j < 3 ; j++)
        {
          gfloat c = 255 * ( pdst[j] - mini ) / range;

          psrc[j] = c; /* CLAMP (c, 0, 255); */
        }
    }

  g_free (dst);
}

 
/* Calculation of the average and the variance */
static void compute_mean_var(gfloat* src, gfloat* mean, gfloat* var, gint size, gint channels_n)
{
  gfloat vsquared;
  gint i,j;
  gfloat *psrc;

  vsquared = 0;
  *mean = 0;
  for (i = 0; i < size; i+= channels_n)
    {
       psrc = src+i;
       for (j = 0 ; j < 3 ; j++)
         {
            *mean += psrc[j];
            vsquared += psrc[j] * psrc[j];
         }
    }

  *mean /= (gfloat) size; /* mean */
  vsquared /= (gfloat) size; /* mean (x^2) */
  *var = ( vsquared - (*mean * *mean) );
  *var = sqrt(*var); /* var */
}

/* These functions are independent from the main program. 
The functions associate with the algorithm and filters recursively */
static void compute_coefs3 (gauss3_coefs *c, gfloat sigma)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 11b       computation of q
   *          8c        computation of b0..b1
   *          10        alpha is normalization constant B
   */
  gfloat q, q2, q3;

  q = 0;

  if (sigma >= 2.5)
    {
      q = 0.98711 * sigma - 0.96330;
    }
  else if ((sigma >= 0.5) && (sigma < 2.5))
    {
      q = 3.97156 - 4.14554 * (gfloat) sqrt ((double) 1 - 0.26891 * sigma);
    }
  else
    {
      q = 0.1147705018520355224609375;
    }

  q2 = q * q;
  q3 = q * q2;
  c->b[0] = (1.57825+(2.44413*q)+(1.4281 *q2)+(0.422205*q3));
  c->b[1] = (        (2.44413*q)+(2.85619*q2)+(1.26661 *q3));
  c->b[2] = (                   -((1.4281*q2)+(1.26661 *q3)));
  c->b[3] = (                                 (0.422205*q3));
  c->B = 1.0-((c->b[1]+c->b[2]+c->b[3])/c->b[0]);
  c->sigma = sigma;
  c->N = 3;
}

/* User Interface: This function creates the graphic interface allowing
the user to modify the parameters of the algo for the standardization of 
color. Image preview coded out. NOT supported by GTK 1.2 codebase! */

static gint retinex_dialog(void)
{
  /* The management of the pointers and the memory is made by gtk.  */
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *frame1;
  GtkWidget *table;
  GtkWidget *table1;
  GtkWidget *button;
  GtkWidget *hbox;
  GSList *group = NULL;
  GtkObject *adj;

  gimp_ui_init ("retinex", TRUE);

  window = gimp_dialog_new ("Retinex", "retinex",
			    gimp_standard_help_func, "filters/mosaic.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    "OK", retinex_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    "Cancel", gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  //Paramter setting
  frame = gtk_frame_new ("Parameter Settings");
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), frame,
		      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  //Table for alignment of widgets
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);
    
  //Level setting
  frame1 = gtk_frame_new ("Level");
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), frame1,
		      FALSE, FALSE, 0);
  gtk_widget_show (frame1);

  //Table for radio buttons
  table1 = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 4);
  gtk_container_add (GTK_CONTAINER (frame1), table1);
  gtk_widget_show (table1);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      "Scale:", SCALE_WIDTH, 0,
			      rvals.scale, MIN_GAUSSIAN_SCALE, MAX_GAUSSIAN_SCALE, 1.0, 1.0, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &rvals.scale);


  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      "Scale division:", SCALE_WIDTH, 0,
			      rvals.nscales, 0.0, 8.0, 1.0, 1.0, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &rvals.nscales);


  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      "Dynamic:", SCALE_WIDTH, 0,
			      rvals.cvar, 0.0, 4.0, 0.1, 1.0, 1,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &rvals.cvar);

  /*  Radio buttons..  */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_table_attach (GTK_TABLE (table1), hbox, 0, 3, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);

  button = gtk_radio_button_new_with_label (group, "Uniform");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
               rvals.scales_mode == RETINEX_UNIFORM ? 1 : 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) retinex_button_callback,
		      (gpointer)RETINEX_UNIFORM);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Low");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),  
              rvals.scales_mode == RETINEX_LOW ? 1 : 0);  
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) retinex_button_callback,
		      (gpointer)RETINEX_LOW);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "High");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
			       rvals.scales_mode == RETINEX_HIGH ? 1 : 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		     (GtkSignalFunc) retinex_button_callback,
		     (gpointer)RETINEX_HIGH);

  /* With final completion one launches the principal loop */
  gtk_widget_show (window);

  gtk_widget_show (button);
  gtk_widget_show (hbox);
  gtk_main();
  gdk_flush();

  return retint.run;
}


/* Callback traps */
static void retinex_ok_callback(GtkWidget* widget,
				gpointer data)
{
  retint.run = TRUE;
  gtk_widget_destroy(GTK_WIDGET(data));
}

static void retinex_button_callback(GtkWidget* widget, 
				    gpointer data)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
      rvals.scales_mode = (gint) data;
    }
}
