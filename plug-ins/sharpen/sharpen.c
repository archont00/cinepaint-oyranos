/*
 * "$Id: sharpen.c,v 1.1 2004/02/10 00:43:07 robinrowe Exp $"
 *
 *   Sharpen filters for The GIMP -- an image manipulation program
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the filter...
 *   sharpen()                   - Sharpen an image using a median filter.
 *
 * Revision History:
 *
 *   $Log: sharpen.c,v $
 *   Revision 1.1  2004/02/10 00:43:07  robinrowe
 *   add 0.18-1
 *
 *   Revision 1.2  2003/01/28 07:37:03  samrichards
 *   Fixed reference to MAIN() which was causing these plugins to disapear.
 *
 *   Revision 1.1.1.1  2002/09/28 21:31:02  samrichards
 *   Filmgimp-0.5 initial release to CVS
 *
 *   Revision 1.1.1.1.2.1  2000/09/06 21:29:53  yosh
 *   Merge with R&H tree
 *
 *   -Yosh
 *
 * Revision 1.1.1.1  2000/02/17  00:54:07  calvin
 * Importing the v1.0 r & h hollywood branch. No longer cvs-ed at gnome.
 *
 *   Revision 1.6  1998/04/28 03:50:19  yosh
 *   * plug-ins/animationplay/animationplay.c
 *   * plug-ins/CEL/CEL.c
 *   * plug-ins/psd/psd.c
 *   * plug-ins/xd/xd.c: applied gimp-joke-980427-0, warning cleanups
 *
 *   * app/temp_buf.c: applied gimp-entity-980427-0, temp_buf swap speedups and
 *   more robust tempfile handling
 *
 *   -Yosh
 *
 *   Revision 1.5  1998/04/27 22:01:01  neo
 *   Updated sharpen and despeckle. Wow, sharpen is balzingly fast now, while
 *   despeckle is still sort of lame...
 *
 *
 *   --Sven
 *
 *   Revision 1.13  1998/04/27  15:55:38  mike
 *   Sharpen would shift the image down one pixel; was using the wrong "source"
 *   row...
 *
 *   Revision 1.12  1998/04/27  15:45:27  mike
 *   OK, put the shadow buffer stuff back in - without shadowing the undo stuff
 *   will *not* work...  sigh...
 *   Doubled tile cache to avoid cache thrashing with shadow buffer.
 *
 *   Revision 1.11  1998/04/27  15:33:45  mike
 *   Updated to use LUTs for coefficients.
 *   Broke out filter code for GRAY, GRAYA, RGB, RGBA modes.
 *   Fixed destination region code - was using a shadow buffer when it wasn't
 *   needed.
 *   Now add 1 to the number of tiles needed in the cache to avoid possible
 *   rounding error and resulting cache thrashing.
 *
 *   Revision 1.10  1998/04/23  14:39:47  mike
 *   Whoops - wasn't copying the preview image over for RGB mode...
 *
 *   Revision 1.9  1998/04/23  13:56:02  mike
 *   Updated preview to do checkerboard pattern for transparency (thanks Yosh!)
 *   Added gtk_window_set_wmclass() call to make sure this plug-in gets to use
 *   the standard GIMP icon if none is otherwise created...
 *
 *   Revision 1.8  1998/04/22  16:25:45  mike
 *   Fixed RGBA preview problems...
 *
 *   Revision 1.7  1998/03/12  18:48:52  mike
 *   Fixed pixel errors around the edge of the bounding rectangle - the
 *   original pixels weren't being written back to the image...
 *
 *   Revision 1.6  1997/11/14  17:17:59  mike
 *   Updated to dynamically allocate return params in the run() function.
 *
 *   Revision 1.5  1997/10/17  13:56:54  mike
 *   Updated author/contact information.
 *
 *   Revision 1.4  1997/09/29  17:16:29  mike
 *   To average 8 numbers you do *not* divide by 9!  This caused the brightening
 *   problem when sharpening was "turned up".
 *
 *   Revision 1.2  1997/06/08  22:27:35  mike
 *   Updated sharpen code for hard-coded 3x3 convolution matrix.
 *
 *   Revision 1.1  1997/06/08  16:46:07  mike
 *   Initial revision
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <gtk/gtk.h>
#include "lib/plugin_main.h"
#include "lib/ui.h"


/*
 * Macros...
 */

#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
#define ROUND(x) 		(gint)((x)+.5)


/*
 * Constants...
 */

#define PLUG_IN_NAME		"plug_in_sharpen"
#define PLUG_IN_VERSION		"1.3 - 27 April 1998"
#define PREVIEW_SIZE		128
#define SCALE_WIDTH		64
#define ENTRY_WIDTH		64

#define CHECK_SIZE		8
#define CHECK_DARK		85
#define CHECK_LIGHT		170


/*
 * Local functions...
 */

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static void	sharpen(TileDrawable *drawable);
static void     filter_u8(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
static void     filter_u16(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
static void     filter_float(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
static void     filter_float16(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);


/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =
		{
		  NULL,		/* init_proc */
		  NULL,		/* quit_proc */
		  query,	/* query_proc */
		  run		/* run_proc */
		};

GtkWidget	*preview;		/* Preview widget */
int		preview_width,		/* Width of preview widget */
		preview_height,		/* Height of preview widget */
		preview_x1,		/* Upper-left X of preview */
		preview_y1,		/* Upper-left Y of preview */
		preview_x2,		/* Lower-right X of preview */
		preview_y2;		/* Lower-right Y of preview */
guchar		*preview_src,		/* Source pixel image */
		*preview_neg,		/* Negative coefficient pixels */
		*preview_dst,		/* Destination pixel image */
		*preview_image;		/* Preview RGB image */
GtkObject	*hscroll_data,		/* Horizontal scrollbar data */
		*vscroll_data;		/* Vertical scrollbar data */

TileDrawable	*drawable = NULL;	/* Current image */
int		sel_x1,			/* Selection bounds */
		sel_y1,
		sel_x2,
		sel_y2;
int		sel_width,		/* Selection width */
		sel_height;		/* Selection height */
int		img_bpp;		/* Bytes-per-pixel in image */
int		sharpen_percent = 50;	/* Percent of sharpening */
gfloat		boost_factor;	        /* boost_factor */
gint		run_filter = FALSE;	/* True if we should run the filter */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query(void)
{
  static GParamDef	args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" }
  };
  static GParamDef	*return_vals = NULL;
  static int		nargs        = sizeof(args) / sizeof(args[0]),
			nreturn_vals = 0;


  gimp_install_procedure(PLUG_IN_NAME,
      "Sharpen the image",
      "This plug-in performs a sharpen convolution filter on an image.",
      "Michael Sweet <mike@easysw.com>",
      "Copyright 1997-1998 by Michael Sweet",
      PLUG_IN_VERSION,
      "<Image>/Filters/Enhance/Sharpen", 
      "RGB*, GRAY*, U16_RGB*, U16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
      PROC_PLUG_IN, nargs, nreturn_vals, args, return_vals);
}


/*
 * 'run()' - Run the filter...
 */

static void
run(char   *name,		/* I - Name of filter program. */
    int    nparams,		/* I - Number of parameters passed in */
    GParam *param,		/* I - Parameter values */
    int    *nreturn_vals,	/* O - Number of return values */
    GParam **return_vals)	/* O - Return values */
{
  GRunModeType	run_mode;	/* Current run mode */
  GStatusType	status;		/* Return status */
  GParam	*values;	/* Return values */


 /*
  * Initialize parameter data...
  */

  status   = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values = g_new(GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

 /*
  * Get drawable information...
  */

  drawable = gimp_drawable_get(param[2].data.d_drawable);

 /*
  * See how we will run
  */

  switch (run_mode)
  {
    case RUN_INTERACTIVE :
    case RUN_NONINTERACTIVE :
    case RUN_WITH_LAST_VALS :
	break;
    default :
        status = STATUS_CALLING_ERROR;
        break;
  }

 /*
  * Sharpen the image...
  */

  if (status == STATUS_SUCCESS)
  {
    if ((gimp_drawable_color(drawable->id) ||
	 gimp_drawable_gray(drawable->id)))
    {
     /*
      * Set the tile cache size...
      */

      gimp_tile_cache_ntiles(2 * (drawable->width + gimp_tile_width() - 1) /
                             gimp_tile_width() + 1);

     /*
      * Run!
      */

      sharpen(drawable);

     /*
      * If run mode is interactive, flush displays...
      */

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

     /*
      * Store data...
      */

      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data(PLUG_IN_NAME, &sharpen_percent, sizeof(sharpen_percent));
    }
    else
      status = STATUS_EXECUTION_ERROR;
  }

 /*
  * Reset the current run status...
  */

  values[0].data.d_status = status;

 /*
  * Detach from the drawable...
  */

  gimp_drawable_detach(drawable);
}


/*
 * 'sharpen()' - Sharpen an image using a convolution filter.
 */

static void
sharpen(TileDrawable * drawable)
{
  GPixelRgn	src_rgn,	/* Source image region */
		dst_rgn;	/* Destination image region */
  guchar        *current,       /* data for current row */
		*previous,      /* data for prev row */ 
		*next, 		/* data for next row */ 
		*dest,          /* data for dest row */
		*tmp;           /* tmp pointer for data */
  gint  y; 			/* scanline to process */
  gint	bytes;		        /* Byte width of the image */

  gint num_channels = gimp_drawable_num_channels (drawable->id);
  gint bpp = gimp_drawable_bpp (drawable->id);
  gint sel_width, sel_height, sel_x1, sel_y1, sel_x2, sel_y2;
  gint has_alpha = gimp_drawable_has_alpha(drawable->id);
  GPrecisionType precision = gimp_drawable_precision (drawable->id);

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width     = sel_x2 - sel_x1;
  sel_height    = sel_y2 - sel_y1;

  /* bail if too small */
  if (sel_width < 3 || sel_height < 3)
	return;

 /*
  * Let the user know what we're doing...
  */
  gimp_progress_init("Sharpening...");

 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      FALSE, FALSE);
  gimp_pixel_rgn_init(&dst_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      TRUE, TRUE);

  boost_factor = 1.0 + .02 * sharpen_percent; 
  
  bytes = sel_width * bpp;

  /* set up previous, current, next, dst_row */

  previous = g_malloc(bytes * sizeof(guchar));
  current = g_malloc(bytes * sizeof(guchar));
  next = g_malloc(bytes * sizeof(guchar));
  dest = g_malloc(bytes * sizeof(guchar));

  /* do the first row*/ 
  gimp_pixel_rgn_get_row(&src_rgn, previous, sel_x1, sel_y1, sel_width);
  gimp_pixel_rgn_set_row(&dst_rgn, previous, sel_x1, sel_y1, sel_width);

#if 0
  d_printf("row %d set to %d %d %d %d %d\n", sel_y1, 
      current[0], current[1], current[2], current[3], current[4]);
#endif

 /*
  * Load the current row for the filter...
  */
  
  gimp_pixel_rgn_get_row(&src_rgn, current, sel_x1, sel_y1+1, sel_width);

 /*
  * Sharpen...
  */
 

  for (y = sel_y1+1; y <= sel_y2-2; y ++)
  {

   /*
    * Load next row.
    */
     gimp_pixel_rgn_get_row(&src_rgn, next, sel_x1, y + 1, sel_width);
  
   /*
    * Sharpen.
    */

    switch(precision)
    {
	case PRECISION_U8:
          filter_u8(sel_width, num_channels, has_alpha, dest, previous, current, next);
	  break;
	case PRECISION_U16:
          filter_u16(sel_width, num_channels, has_alpha, dest, previous, current, next);
	  break;
	case PRECISION_FLOAT:
          filter_float(sel_width, num_channels, has_alpha, dest, previous, current, next);
	  break;
	case PRECISION_FLOAT16: 
          filter_float16(sel_width, num_channels, has_alpha, dest, previous, current, next);
	  break;
    }

   
   /*
    * Save the dest row. 
    */

    gimp_pixel_rgn_set_row(&dst_rgn, dest, sel_x1, y, sel_width);
     /*gimp_pixel_rgn_set_row(&dst_rgn, current, sel_x1, y, sel_width);*/
#if 0
     d_printf("row %d set to %d %d %d %d %d\n", y, 
	  current[0], current[1], current[2], current[3], current[4]);
#endif
   
   /* 
    * Shuffle the rows
   */ 
    tmp = previous; 
    previous = current; 
    current = next;
    next = tmp;

    if ((y & 15) == 0)
      gimp_progress_update((double)(y - sel_y1) / (double)sel_height);
  }

  /* and the last one */
  gimp_pixel_rgn_set_row(&dst_rgn, current, sel_x1, sel_y2-1, sel_width);
#if 0
  d_printf("row %d set to %d %d %d %d %d\n", sel_y2-1, 
      next[0], next[1], next[2], next[3], next[4]);
#endif

 /*
  * OK, we're done.  Free all memory used...
  */

  g_free(dest);
  g_free(previous);
  g_free(current);
  g_free(next);

 /*
  * Update the screen...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}



/*
 * filter() - Sharpen a row. 
 */

static void
filter_u8   (gint    width,
	    gint    num_channels,
	    gint    has_alpha,
            guchar *dest,
            guchar *prev,
            guchar *current,
            guchar *next)
{
  gint sum;
  gint pixel;
  gint count;
  gint s0, s1, s2;
  guint8 *p;
  guint8 *c;
  guint8 *n;
  guint8 *d;
  guint8 *src;
  gint i;
  
  /* 
    We split the computation like this:

    high_boost = boost_factor * orig - low_pass 

 					   1 1 1  
               = boost_factor * orig - 1/9 1 1 1 
					   1 1 1 

    When the boost_value = 1, we get the standard highpass:

    high_pass = orig - low_pass

    where

		          -1 -1 -1 
	   highpass = 1/9 -1  8 -1 
		          -1 -1 -1

    When the boost_factor = 2, we get

    2 * orig - low_pass = orig + highpass 

    which is what "sharpen" means for most people. 
  */

  i = num_channels;
  while(i--)
  {

    p = (guint8*)prev + i; 
    c = (guint8*)current + i;
    n = (guint8*)next + i;
    d = (guint8*)dest + i;
    src = (guint8*)current + i;

    s0 = *p + *c + *n;   /* first partial sum */
    *d = *src;           /*copy first pixel*/
	
    p += num_channels;   /* go to second pixel */
    c += num_channels;
    n += num_channels;
    d += num_channels;
    src += num_channels;
   
    s1 = *p + *c + *n;  /* second partial sum */

    p += num_channels;   /* go to third pixel */
    c += num_channels;
    n += num_channels;

    count = width - 2;
    while (count--)
    {
      s2 = *p + *c + *n; 
      sum = s0 + s1 + s2;
      s0 = s1;
      s1 = s2;

      pixel = ROUND(boost_factor * *src - .111111*sum); 

      if (pixel < 0)
	*d = 0;
      else if (pixel < 255)
	*d = pixel;
      else
	*d = 255;

      p+=num_channels;
      c+=num_channels;
      n+=num_channels;
      d+=num_channels;
      src+=num_channels; 
    }
  } 

  /* 
     Set the last pixel on the row. 
  */

  c = (guint8*)current;                                                      
  c += (width-1)*num_channels;                                               
  d = (guint8*)dest;                                                      
  d += (width-1)*num_channels;                                               
                                                                             
  i = num_channels;                                                          
  while(i--)                                                                 
   *d++ = *c++;
}

static void
filter_u16   (gint    width,
	    gint    num_channels,
	    gint    has_alpha,
            guchar *dest,
            guchar *prev,
            guchar *current,
            guchar *next)
{
  gint sum;
  gint pixel;
  gint count;
  gint s0, s1, s2;
  guint16 *p;
  guint16 *c;
  guint16 *n;
  guint16 *d;
  guint16 *src;
  gint i;
  
  i = num_channels;
  while(i--)
  {

    p = (guint16*)prev + i; 
    c = (guint16*)current + i;
    n = (guint16*)next + i;
    d = (guint16*)dest + i;
    src = (guint16*)current + i;

    s0 = *p + *c + *n;   /* first partial sum */
    *d = *src;           /*copy first pixel*/
	
    p += num_channels;   /* go to second pixel */
    c += num_channels;
    n += num_channels;
    d += num_channels;
    src += num_channels;
   
    s1 = *p + *c + *n;  /* second partial sum */

    p += num_channels;   /* go to third pixel */
    c += num_channels;
    n += num_channels;

    count = width - 2;
    while (count--)
    {
      s2 = *p + *c + *n; 
      sum = s0 + s1 + s2;
      s0 = s1;
      s1 = s2;

      pixel = ROUND(boost_factor * *src - .111111*sum); 

      if (pixel < 0)
	*d = 0;
      else if (pixel < 65535)
	*d = pixel;
      else
	*d = 65535;

      p+=num_channels;
      c+=num_channels;
      n+=num_channels;
      d+=num_channels;
      src+=num_channels; 
    }
  } 

  /* 
     Set the last pixel on the row. 
  */

  c = (guint16*)current;                                                      
  c += (width-1)*num_channels;                                               
                                                                             
  d = (guint16*)dest;                                                      
  d += (width-1)*num_channels;                                               

  i = num_channels;                                                          
  while(i--)                                                                 
   *d++ = *c++;
}

static void
filter_float   (gint    width,
	    gint    num_channels,
	    gint    has_alpha,
            guchar *dest,
            guchar *prev,
            guchar *current,
            guchar *next)
{

  gfloat sum;
  gfloat pixel;
  gfloat s0, s1, s2;
  gfloat *p;
  gfloat *c;
  gfloat *n;
  gfloat *d;
  gfloat *src;

  gint count;
  gint i;
  
  i = num_channels;
  while(i--)
  {

    p = (gfloat*)prev + i; 
    c = (gfloat*)current + i;
    n = (gfloat*)next + i;
    d = (gfloat*)dest + i;
    src = (gfloat*)current + i;

    s0 = *p + *c + *n;   /* first partial sum */
    *d = *src;           /*copy first pixel*/
	
    p += num_channels;   /* go to second pixel */
    c += num_channels;
    n += num_channels;
    d += num_channels;
    src += num_channels;
   
    s1 = *p + *c + *n;  /* second partial sum */

    p += num_channels;   /* go to third pixel */
    c += num_channels;
    n += num_channels;

    count = width - 2;
    while (count--)
    {
      s2 = *p + *c + *n; 
      sum = s0 + s1 + s2;
      s0 = s1;
      s1 = s2;

      *d = boost_factor * *src - .111111*sum; 

      p+=num_channels;
      c+=num_channels;
      n+=num_channels;
      d+=num_channels;
      src+=num_channels; 
    }
  } 

  /* 
     Set the last pixel on the row. 
  */

  c = (gfloat*)current;                                                      
  c += (width-1)*num_channels;                                               
                                                                             
  d = (gfloat*)dest;                                                      
  d += (width-1)*num_channels;                                               

  i = num_channels;                                                          
  while(i--)                                                                 
   *d++ = *c++;
}

static void
filter_float16  (gint    width,
	    gint    num_channels,
	    gint    has_alpha,
            guchar *dest,
            guchar *prev,
            guchar *current,
            guchar *next)
{

  gfloat sum;
  gfloat pixel;
  gfloat s0, s1, s2;

  guint16 *p;
  guint16 *c;
  guint16 *n;
  guint16 *d;
  guint16 *src;

  gint count;
  gint i;
  ShortsFloat u,v,w;
  
  i = num_channels;
  while(i--)
  {

    p = (guint16*)prev + i; 
    c = (guint16*)current + i;
    n = (guint16*)next + i;
    d = (guint16*)dest + i;
    src = (guint16*)current + i;

    s0 = FLT(*p,u) + FLT(*c,v) + FLT(*n,w);   /* first partial sum */
    *d = *src;           /*copy first pixel*/
	
    p += num_channels;   /* go to second pixel */
    c += num_channels;
    n += num_channels;
    d += num_channels;
    src += num_channels;
   
    s1 = FLT(*p,u) + FLT(*c,v) + FLT(*n,w);  /* second partial sum */

    p += num_channels;   /* go to third pixel */
    c += num_channels;
    n += num_channels;

    count = width - 2;
    while (count--)
    {
      s2 = FLT(*p,u) + FLT(*c,v) + FLT(*n,w); 
      sum = s0 + s1 + s2;
      s0 = s1;
      s1 = s2;

      *d = FLT16 (boost_factor * FLT (*src,v) - .111111*sum, u); 

      p+=num_channels;
      c+=num_channels;
      n+=num_channels;
      d+=num_channels;
      src+=num_channels; 
    }
  } 

  /* 
     Set the last pixel on the row. 
  */

  c = (guint16*)current;                                                      
  c += (width-1)*num_channels;                                               
  d = (guint16*)dest;                                                      
  d += (width-1)*num_channels;                                               
                                                                             
  i = num_channels;                                                          
  while(i--)                                                                 
   *d++ = *c++;
}
