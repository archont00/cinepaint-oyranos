#include "../lib/version.h"
#include "appenv.h"
#include "cms.h"
#include "color_correction.h"
#include "pixelarea.h"
#include "pixelrow.h"



/* look up tables for sh,mid,hi weights */
gdouble *color_correction_shadows_transfer_LUT = NULL;
gdouble *color_correction_midtones_transfer_LUT = NULL;
gdouble *color_correction_highlights_transfer_LUT = NULL;

/* Lab conversion transformations */
CMSTransform *color_correction_to_lab_transform = NULL;
CMSTransform *color_correction_from_lab_transform = NULL;


/******************************
 *                            *
 *     PRIVATE INTERFACE      *
 *                            *
 ******************************/

/* auxiliary used by color_correction_hls_to_rgb */
static float _color_correction_hls_value (float n1,
					  float n2,
					  float hue);





/***************************
 *                         *
 *     IMPLEMENTATION      *
 *                         *
 ***************************/

void 
color_correction_init_gaussian_transfer_LUTs (void)
{   gint i;
    if (color_correction_shadows_transfer_LUT == NULL) 
    {   color_correction_shadows_transfer_LUT = g_new(gdouble, 256);
    }
    if (color_correction_midtones_transfer_LUT == NULL) 
    {   color_correction_midtones_transfer_LUT = g_new(gdouble, 256);
    }
    if (color_correction_highlights_transfer_LUT == NULL) 
    {   color_correction_highlights_transfer_LUT = g_new(gdouble, 256);
    }


    for (i = 0; i < 256; i++)
    {   color_correction_shadows_transfer_LUT[i] = color_correction_highlights_transfer_LUT[255-i] = exp(-pow(i/255.,2)*6);
    }
    for (i = 0; i < 256; i+=2)
    {   color_correction_midtones_transfer_LUT[i/2] = color_correction_highlights_transfer_LUT[i];
    }
    for (i = 0; i < 256; i+=2)
    {   color_correction_midtones_transfer_LUT[i/2 + 128] = color_correction_shadows_transfer_LUT[i];
    }    
}


void 
color_correction_init_bernstein_transfer_LUTs (void)
{   gint i;
    if (color_correction_shadows_transfer_LUT == NULL) 
    {   color_correction_shadows_transfer_LUT = g_new(gdouble, 256);
    }
    if (color_correction_midtones_transfer_LUT == NULL) 
    {   color_correction_midtones_transfer_LUT = g_new(gdouble, 256);
    }
    if (color_correction_highlights_transfer_LUT == NULL) 
    {   color_correction_highlights_transfer_LUT = g_new(gdouble, 256);
    }

    for (i = 0; i < 256; i++)
    {   color_correction_shadows_transfer_LUT[i] = (1-(i/255.)) * (1-(i/255.)); /*2nd bernstein poly of deg 2*/
    }
    for (i = 0; i < 256; i++)
    {   color_correction_midtones_transfer_LUT[i] = 2 * (i/255.) * (1-(i/255.)); /*1st bernstein poly of deg 2*/
    }
    for (i = 0; i < 256; i++)
    {   color_correction_highlights_transfer_LUT[i] = (i/255.) * (i/255.); /*0th bernstein poly of deg 2*/
    }    
}


void 
color_correction_free_transfer_LUTs (void)
{   g_free(color_correction_shadows_transfer_LUT);
    color_correction_shadows_transfer_LUT = NULL; 
    g_free(color_correction_midtones_transfer_LUT);
    color_correction_midtones_transfer_LUT = NULL; 
    g_free(color_correction_highlights_transfer_LUT); 
    color_correction_highlights_transfer_LUT = NULL; 
}


void 
color_correction_init_lab_transforms (GImage *image)
{   CMSProfile *image_profile = gimage_get_cms_profile(image);
    CMSProfile *lab_profile = cms_get_lab_profile(NULL);
    GSList *profiles = NULL;
    Tag t = canvas_tag(drawable_data(gimage_active_drawable(image)));
    DWORD lcms_format;

    if (!image_profile) 
    {   image_profile = cms_get_srgb_profile();
    }

    profiles = g_slist_append(profiles, image_profile);
    profiles = g_slist_append(profiles, lab_profile);
    lcms_format = cms_get_lcms_format(t, image_profile);
    color_correction_to_lab_transform = cms_get_transform(profiles, lcms_format, TYPE_Lab_16, INTENT_PERCEPTUAL, 0, 0, t, t, NULL);
    
    profiles = g_slist_remove(profiles, image_profile);
    profiles = g_slist_append(profiles, image_profile);
    color_correction_from_lab_transform = cms_get_transform(profiles, TYPE_Lab_16, lcms_format, INTENT_PERCEPTUAL, 0, 0, t, t, NULL);
    
    g_slist_free(profiles);   
}


void 
color_correction_free_lab_transforms ()
{   cms_return_transform(color_correction_to_lab_transform);
    color_correction_to_lab_transform = NULL;
    cms_return_transform(color_correction_from_lab_transform);
    color_correction_from_lab_transform = NULL;
}


void
color_correction (PixelArea *src_area,
		  PixelArea *dest_area,
		  gpointer user_data)
{   ColorCorrectionSettings *settings =  (ColorCorrectionSettings *)user_data;
    gint width = pixelarea_width(src_area);
    gint height = pixelarea_height(src_area);
    gint num_channels = tag_num_channels(pixelarea_tag(src_area));
    /* overhead is in pixels */
    gint row_overhead = pixelarea_rowstride(src_area)/pixelarea_bytes(src_area)
                        - width;
    gboolean has_alpha = tag_alpha(pixelarea_tag(src_area)) == ALPHA_YES ?
                         TRUE : FALSE;
    /* buffers to hold a Lab colour value, used with Lab */
    guint16 Lab[3],new_Lab[3];
    gint16 new_col[3];
    /* buffers to hold a colour value in float, used with HLS */
    float fcol[3], fnew_col[3];

    guint8 *s = (guint8 *)pixelarea_data(src_area);
    guint8 *d = (guint8 *)pixelarea_data(dest_area);

    /* r,g,b values of the colour we are interpolating with, sat and lightness */
    gfloat rgb[3][3];
    int i=0;

    /* get the fully saturated colour of given hue in rgb 
       this is the colour we are moving towards/interpolating with */
    while (i<=HIGHLIGHTS)
    {   rgb[i][RED_PIX]=settings->values[i][HUE];
	rgb[i][GREEN_PIX]=0.5;
	rgb[i][BLUE_PIX]=1.0;
	color_correction_hls_to_rgb(&rgb[i][RED_PIX], &rgb[i][GREEN_PIX], &rgb[i][BLUE_PIX]);
	rgb[i][RED_PIX] *= 255.;
	rgb[i][GREEN_PIX] *= 255.;
	rgb[i][BLUE_PIX] *= 255.;
	i++;
    }

    
    while (height--)
    {   
      while (width--) {
	/* get lightness */
	guint8 L = .3*s[0] + 0.6*s[1] + 0.1*s[2];
	/* lab implementation */	
        if (settings->model == LAB)
        {   /* convert to Lab to get lightness for transfer functions */
  	    for (i=RED_PIX; i<=BLUE_PIX; i++)
	    {   new_col[i] = s[i];
	        
	        /* interpolating betwen old and new, weighted by saturation */
	        new_col[i] += (rgb[SHADOWS][i]-s[i]) * settings->values[SHADOWS][SATURATION] * color_correction_shadows_transfer_LUT[L];
		new_col[i] += (rgb[MIDTONES][i]-s[i]) * settings->values[MIDTONES][SATURATION] * color_correction_midtones_transfer_LUT[L];
		new_col[i] += (rgb[HIGHLIGHTS][i]-s[i]) * settings->values[HIGHLIGHTS][SATURATION] * color_correction_highlights_transfer_LUT[L];

		d[i] = BOUNDS(new_col[i],0,255);
	    }

	    /* convert new colour to Lab for lightness adjustment */
	    if (settings->lightness_modified || settings->preserve_lightness) 
	    {
	    cms_transform_uint(color_correction_to_lab_transform, d, new_Lab, 1);	  

	    /* set new lightness equal to old lightness */
	    
	    if (settings->preserve_lightness)
	    {    cms_transform_uint(color_correction_to_lab_transform, s, Lab, 1);
	         new_Lab[0] = Lab[0];
	    }

	    /* adjust lightness by user selection */
	    if (settings->values[SHADOWS][LIGHTNESS] > 0)
	    {   new_Lab[0] += (65535. - Lab[0]) * (settings->values[SHADOWS][LIGHTNESS] * color_correction_shadows_transfer_LUT[L]);
	    }
	    else if (settings->values[SHADOWS][LIGHTNESS] < 0)
	    {   new_Lab[0] *= 1.+ settings->values[SHADOWS][LIGHTNESS] * color_correction_shadows_transfer_LUT[L];
	    }
	    if (settings->values[MIDTONES][LIGHTNESS] > 0)
	    {   new_Lab[0] += (65535. - Lab[0]) * (settings->values[MIDTONES][LIGHTNESS] * color_correction_midtones_transfer_LUT[L]);
	    }
	    else if (settings->values[MIDTONES][LIGHTNESS] < 0)
	    {   new_Lab[0] *= 1.+settings->values[MIDTONES][LIGHTNESS] * color_correction_midtones_transfer_LUT[L];
	    }
	    if (settings->values[HIGHLIGHTS][LIGHTNESS] > 0)
	    {   new_Lab[0] += (65535. - Lab[0]) * (settings->values[HIGHLIGHTS][LIGHTNESS] * color_correction_highlights_transfer_LUT[L]);
	    }
	    else if (settings->values[HIGHLIGHTS][LIGHTNESS]<0)
	    {   new_Lab[0] *= 1.+settings->values[HIGHLIGHTS][LIGHTNESS] * color_correction_highlights_transfer_LUT[L];
	    }
	
	    /* transform Lab with adjusted lightness back to rgb */
	    cms_transform_uint(color_correction_from_lab_transform, new_Lab, d, 1);	
	    }
	}
	else /* model == HLS */
	{   /* standard hls implementation after Foley, van Dam et al */


	    /* convert to float */
	    for (i=RED_PIX; i<=BLUE_PIX; i++)
	    {   fcol[i]=s[i]/255.;
	    }

	    /* convert to HLS to get lightness for transfer functions */
	    color_correction_rgb_to_hls (&fcol[0], &fcol[1], &fcol[2]);
	    
	    /* fcol[1] is lightness */
	    for (i=RED_PIX; i<=BLUE_PIX; i++)
	    {   fnew_col[i] = s[i]/255.;
	        fnew_col[i] += ((rgb[SHADOWS][i]-s[i])/255.) * settings->values[SHADOWS][SATURATION] * color_correction_shadows_transfer_LUT[L];
		fnew_col[i] += ((rgb[MIDTONES][i]-s[i])/255.) * settings->values[MIDTONES][SATURATION] * color_correction_midtones_transfer_LUT[L];
		fnew_col[i] += ((rgb[HIGHLIGHTS][i]-s[i])/255.) * settings->values[HIGHLIGHTS][SATURATION] * color_correction_highlights_transfer_LUT[L];

		fnew_col[i] = BOUNDS(fnew_col[i],0,1.0);
	    }

	    /* convert new colour to hls for lightness adjustment */
	    color_correction_rgb_to_hls (&fnew_col[0], &fnew_col[1], &fnew_col[2]);

	    /* set new lightness equal to old lightness */
	    if (settings->preserve_lightness)
	    {   fnew_col[1] = fcol[1];
	    }

	    /* adjust lightness by user selection */
	    if (settings->values[SHADOWS][LIGHTNESS] > 0)
	    {   fnew_col[1] += (1. - fcol[1]) * (settings->values[SHADOWS][LIGHTNESS] * color_correction_shadows_transfer_LUT[L]);
	    }
	    else if (settings->values[SHADOWS][LIGHTNESS] < 0)
	    {   fnew_col[1] *= 1.+ settings->values[SHADOWS][LIGHTNESS] * color_correction_shadows_transfer_LUT[L];
	    }
	    if (settings->values[MIDTONES][LIGHTNESS] > 0)
	    {   fnew_col[1] += (1. - fcol[1]) * (settings->values[MIDTONES][LIGHTNESS] * color_correction_midtones_transfer_LUT[L]);
	    }
	    else if (settings->values[MIDTONES][LIGHTNESS] < 0)
	    {   fnew_col[1] *= 1.+ settings->values[MIDTONES][LIGHTNESS] * color_correction_midtones_transfer_LUT[L];
	    }
	    if (settings->values[HIGHLIGHTS][LIGHTNESS] > 0)
	    {   fnew_col[1] += (1. - fcol[1]) * (settings->values[HIGHLIGHTS][LIGHTNESS] * color_correction_highlights_transfer_LUT[L]);
	    }
	    else if (settings->values[HIGHLIGHTS][LIGHTNESS]<0)
	    {   fnew_col[1] *= 1.+ settings->values[HIGHLIGHTS][LIGHTNESS] * color_correction_highlights_transfer_LUT[L];
	    }

	    /* convert colour with adjusted lightness back to rgb */
	    color_correction_hls_to_rgb (&fnew_col[0], &fnew_col[1], &fnew_col[2]);

	    /* and from float back to uint8 */
	    d[RED_PIX] = BOUNDS(fnew_col[RED_PIX], 0, 1.0) * 255;
	    d[GREEN_PIX] = BOUNDS(fnew_col[GREEN_PIX], 0, 1.0) * 255;
	    d[BLUE_PIX] = BOUNDS(fnew_col[BLUE_PIX], 0, 1.0) * 255;
	} /* else (model == HLS) */
	if (has_alpha)
	{   d[ALPHA_PIX] = s[ALPHA_PIX];
	}

	s += num_channels;
	d += num_channels;


      }


       width = pixelarea_width(src_area);
       s += row_overhead * num_channels;
       d += row_overhead * num_channels;	
    }  /* while (h--) */
}


void 
color_correction_hls_to_rgb(gfloat *h, gfloat *l, gfloat *s)
{   gfloat hue = *h;
    gfloat lightness = *l;
    gfloat saturation = *s;
    gfloat m2,m1;

    if (saturation == 0)
    {   /*  achromatic case  */
        *h = lightness;
	*l = lightness;
	*s = lightness;
    }
    else
    {   if (lightness < 0.5)
        {   m2 = (lightness * (1.0 + saturation));
	}
        else
        {   m2 = (lightness + saturation - (lightness * saturation));
	}

        m1 = (2*lightness) - m2;

	/*  chromatic case  */
	*h = _color_correction_hls_value (m1, m2, hue + 120);
	*l = _color_correction_hls_value (m1, m2, hue);
	*s = _color_correction_hls_value (m1, m2, hue - 120);
    }
}


void
color_correction_rgb_to_hls (float *r,
			     float *g,
			     float *b)
{   float red, green, blue;
    float h, l, s;
    float min, max, delta;

    red = *r;
    green = *g;
    blue = *b;

    if (red > green)
    {   if (red > blue)
        {   max = red;
	}
        else
	{   max = blue;
	}
        if (green < blue)
	{   min = green;
	}
	else
	{   min = blue;
	}
    }
    else
    {   if (green > blue)
        {   max = green;
	}
	else
	{   max = blue;
	}

	if (red < blue)
	{   min = red;
	}
	else
	{   min = blue;
	}
    }

    l = (max + min) / 2.0;

    /* achromatic case */
    if (max == min)
    {   s = 0.0;
        h = 0.0;
    }
    else  /* chromatic case */
    {   delta = (max - min);

        if (l < 0.5)
	{   s =  delta / (max + min);
	}
        else
	{   s = delta / (2 - max - min);
	}

	if (red == max)
	{   h = (green - blue) / delta;
	}
	else if (green == max)
	{   h = 2 + (blue - red) / delta;
	}
	else
	{   h = 4 + (red - green) / delta;
	}

	h = h * 60;

	if (h < 0)
	{   h += 360;
	}
    }

    *r = h;
    *g = l;
    *b = s;
}
 

static float
_color_correction_hls_value (float n1,
			     float n2,
			     float hue)
{
  float value;

  if (hue > 360)
    hue -= 360;
  else if (hue < 0)
    hue += 360;
  if (hue < 60)
    value = n1 + (n2 - n1) * (hue / 60);
  else if (hue < 180)
    value = n2;
  else if (hue < 240)
    value = n1 + (n2 - n1) * (240 - hue) / 60;
  else
    value = n1;

  return (value);
}

