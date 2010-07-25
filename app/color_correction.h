#ifndef __COLOR_CORRECTION_H__
#define __COLOR_CORRECTION_H__

#include "pixelarea.h"

typedef enum 
{   SHADOWS = 0,
    MIDTONES = 1,   
    HIGHLIGHTS = 2
} ColorCorrectionLightnessRange;

typedef enum
{   HUE = 0,
    SATURATION = 1,
    LIGHTNESS = 2
} ColorCorrectionColorAttribute;

/* lightness range (i.e. sh,mi,hi) transfer function */
typedef enum
{   BERNSTEIN = 0,
    GAUSSIAN = 1
} ColorCorrectionRangeTransfer;

/* colour model used to calculate lightness */
typedef enum
{   HLS = 0,
    LAB = 1
} ColorCorrectionColorModel;


/* the data to pass to the color_correction function */
typedef struct ColorCorrectionSettings
{   gfloat values[3][3];                   /* first dim is range (sh,mi,hi)
					      second is color attribute (h,l,s),
					      h in [0.,360.], s in [0.,1.], l in  [-.5,.5] */
    gboolean preserve_lightness;
    gint lightness_modified;               /* whether or not the user has changed the 
					      lightness */
    ColorCorrectionColorModel model;       /* model to use for preserving lightness */
    ColorCorrectionRangeTransfer transfer; /* which transfer function to use for defining
					      lightness ranges */

} ColorCorrectionSettings;


/* fill the transfer LUTs either with gaussian functions or with
   bernstein polynomials of second degree, either one needs to be called
   before color_correction
   */
void color_correction_init_gaussian_transfer_LUTs (void); 
/* fills the transfer LUTs with bernstein polynomials of second degree*/
void color_correction_init_bernstein_transfer_LUTs (void); 
void color_correction_free_transfer_LUTs (void); 

/* loads the transforms to convert between image colour space and lab 
   for calculating lightness, needs to be called before color_correction
   if Lab is used */
void color_correction_init_lab_transforms (GImage *image);
void color_correction_free_lab_transforms (void);

/* the core function, which does the actual manipulation */
void color_correction (PixelArea *src_area, 
			      PixelArea *dest_area, 
			      gpointer user_data);

/* auxiliaries to convert between rgb and hls,
   algorithms after Foley, van Dam et al. */
void color_correction_hls_to_rgb(gfloat *h, gfloat *l, gfloat *s);
void color_correction_rgb_to_hls(gfloat *r, gfloat *g, gfloat *b);

#endif /* __COLOR_CORRECTION_H__ */
