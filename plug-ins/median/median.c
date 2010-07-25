/*
 *
 *   Simple median filter for The GIMP -- an image manipulation program
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
 *   median()                      - median an image using a median filter.
 *
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

#define PLUG_IN_NAME		"plug_in_median"
#define PLUG_IN_VERSION		"July 2005"


/*
 * Local functions...
 */

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static void	median(TileDrawable *drawable);
static void     filter_u8(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
static void     filter_u16(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
static void     filter_float(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);
/*static void     filter_float16(gint, gint, gint, guchar *, guchar *, guchar *, guchar *);*/


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
      "statistica median of a 3x3 grid of pixels",
      "This plug-in performs median filtering on an image.",
      "Nicola Montecchiari",
      "",
      PLUG_IN_VERSION,
      "<Image>/Filters/Statistic/Median", 
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
  * median the image...
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

      median(drawable);

     /*
      * If run mode is interactive, flush displays...
      */

      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

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
 * 'median()' - median an image using a convolution filter.
 */

static void
median(TileDrawable * drawable)
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
  gimp_progress_init("median..");

 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      FALSE, FALSE);
  gimp_pixel_rgn_init(&dst_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height,
                      TRUE, TRUE);

  bytes = sel_width * bpp;

  /* set up previous, current, next, dst_row */

  previous = g_malloc(bytes * sizeof(guchar));
  current = g_malloc(bytes * sizeof(guchar));
  next = g_malloc(bytes * sizeof(guchar));
  dest = g_malloc(bytes * sizeof(guchar));

  /* do the first row*/ 
  gimp_pixel_rgn_get_row(&src_rgn, previous, sel_x1, sel_y1, sel_width);
  gimp_pixel_rgn_set_row(&dst_rgn, previous, sel_x1, sel_y1, sel_width);

 /*
  * Load the current row for the filter...
  */
  
  gimp_pixel_rgn_get_row(&src_rgn, current, sel_x1, sel_y1+1, sel_width);


  for (y = sel_y1+1; y <= sel_y2-2; y ++)
  {

   /*
    * Load next row.
    */
     gimp_pixel_rgn_get_row(&src_rgn, next, sel_x1, y + 1, sel_width);
  
   /*
    * median
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
          /*filter_float16(sel_width, num_channels, has_alpha, dest, previous, current, next);*/
	  break;
	case PRECISION_NONE:
	return;
    }

   
   /*
    * Save the dest row. 
    */

    gimp_pixel_rgn_set_row(&dst_rgn, dest, sel_x1, y, sel_width);

   
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

/* Functions to compare different values */

static int compare_u8 (const void *elem1,const void *elem2)
{
	return ((*(guint8 *)elem1)-(*(guint8 *)elem2));
}

static int compare_u16 (const void *elem1,const void *elem2)
{
	return ((*(guint16 *)elem1)-(*(guint16 *)elem2));
}

static int compare_float (const void *elem1,const void *elem2)
{
	return ((*(gfloat *)elem1)-(*(gfloat *)elem2));
}

/*
 * filter() - Calculate the median of a 3x3 window. 
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
  guint8 *poff,*coff,*noff,*doff;
  gint index=0,ch=0,j=0,k=0,l=0;
	guint8 sorting_array[9];
	guint8 median=0;

	/* for each channel */

	for (ch=0;ch<num_channels;ch++)
	{
		poff=(guint8 *)prev+ch;
		coff=(guint8 *)current+ch;
		noff=(guint8 *)next+ch;
		doff=(guint8 *)dest+ch;

		/* first pixel */

		*doff=*((guint8 *)current+ch);

		for (index=1;index<width-1;index++)
		{

			/* get a 3x3 matrix */
			j=(index-1)*num_channels;
			k=index*num_channels;
			l=(index+1)*num_channels;
			sorting_array[0]=*(poff+j);
			sorting_array[1]=*(poff+k);
			sorting_array[2]=*(poff+l);
			sorting_array[3]=*(coff+j);
			sorting_array[4]=*(coff+k);
			sorting_array[5]=*(coff+l);
			sorting_array[6]=*(noff+j);
			sorting_array[7]=*(noff+k);
			sorting_array[8]=*(noff+l);

			/* calculate median */

			qsort(sorting_array,9,sizeof(guint8),compare_u8);
			median=sorting_array[4];

			/* copy to destination pixel layer */

			*(doff+index*num_channels)=median;

		}

		/* last pixel */

		*(doff+index*num_channels)=*(coff+index*num_channels);
	}
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
  guint16 *poff,*coff,*noff,*doff;
  gint index=0,ch=0,j=0,k=0,l=0;
	guint16 sorting_array[9];
	guint16 median=0;

	/* for each channel */

	for (ch=0;ch<num_channels;ch++)
	{
		poff=(guint16 *)prev+ch;
		coff=(guint16 *)current+ch;
		noff=(guint16 *)next+ch;
		doff=(guint16 *)dest+ch;

		/* first pixel */

		*doff=*((guint16 *)current+ch);

		for (index=1;index<width-1;index++)
		{

			/* get a 3x3 matrix */
			j=(index-1)*num_channels;
			k=index*num_channels;
			l=(index+1)*num_channels;
			sorting_array[0]=*(poff+j);
			sorting_array[1]=*(poff+k);
			sorting_array[2]=*(poff+l);
			sorting_array[3]=*(coff+j);
			sorting_array[4]=*(coff+k);
			sorting_array[5]=*(coff+l);
			sorting_array[6]=*(noff+j);
			sorting_array[7]=*(noff+k);
			sorting_array[8]=*(noff+l);

			/* calculate median */

			qsort(sorting_array,9,sizeof(guint16),compare_u16);
			median=sorting_array[4];

			/* copy to destination pixel layer */

			*(doff+index*num_channels)=median;

		}

		/* last pixel */

		*(doff+index*num_channels)=*(coff+index*num_channels);
	}
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
  gfloat *poff,*coff,*noff,*doff;
  gint index=0,ch=0,j=0,k=0,l=0;
	gfloat sorting_array[9];
	gfloat median=0;

	/* for each channel */

	for (ch=0;ch<num_channels;ch++)
	{
		poff=(gfloat *)prev+ch;
		coff=(gfloat *)current+ch;
		noff=(gfloat *)next+ch;
		doff=(gfloat *)dest+ch;

		/* first pixel */

		*doff=*((gfloat *)current+ch);

		for (index=1;index<width-1;index++)
		{

			/* get a 3x3 matrix */
			j=(index-1)*num_channels;
			k=index*num_channels;
			l=(index+1)*num_channels;
			sorting_array[0]=*(poff+j);
			sorting_array[1]=*(poff+k);
			sorting_array[2]=*(poff+l);
			sorting_array[3]=*(coff+j);
			sorting_array[4]=*(coff+k);
			sorting_array[5]=*(coff+l);
			sorting_array[6]=*(noff+j);
			sorting_array[7]=*(noff+k);
			sorting_array[8]=*(noff+l);

			/* calculate median */

			qsort(sorting_array,9,sizeof(gfloat),compare_float);
			median=sorting_array[4];

			/* copy to destination pixel layer */

			*(doff+index*num_channels)=median;

		}

		/* last pixel */

		*(doff+index*num_channels)=*(coff+index*num_channels);
	}
}

/*
static void
filter_float16   (gint    width,
	    gint    num_channels,
	    gint    has_alpha,
            guchar *dest,
            guchar *prev,
            guchar *current,
            guchar *next)
{
}
*/
