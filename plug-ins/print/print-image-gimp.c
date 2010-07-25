/*
 * "$Id: print-image-gimp.c,v 1.7 2007/01/13 21:59:12 beku Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
//#include "../../lib/libprintut.h"

#include "print_gimp.h"

#include "print-intl.h"


#include "print.h"
#include "xmalloc.h"

#ifdef DEBUG
 #define DBG printf("%s:%d %s()\n",__FILE__,__LINE__,__func__);
#else
 #define DBG 
#endif

/*
 * "Image" ADT
 *
 * This file defines an abstract data type called "Image".  An Image wraps
 * a Gimp drawable (or some other application-level image representation)
 * for presentation to the low-level printer drivers (which do CMYK
 * separation, dithering and weaving).  The Image ADT has the ability
 * to perform any combination of flips and rotations on the image,
 * and then deliver individual rows to the driver code.
 *
 * Stuff which might be useful to do in this layer:
 *
 * - Scaling, optionally with interpolation/filtering.
 *
 * - Colour-adjustment.
 *
 * - Multiple-image composition.
 *
 * Also useful might be to break off a thin application-dependent
 * sublayer leaving this layer (which does the interesting stuff)
 * application-independent.
 */


/* Concrete type to represent image */
typedef struct
{
  GimpDrawable *drawable;
  GimpPixelRgn  rgn;

  /*
   * Transformations we can impose on the image.  The transformations
   * are considered to be performed in the order given here.
   */

  /* 1: Transpose the x and y axes (flip image over its leading diagonal) */
  int columns;		/* Set if returning columns instead of rows. */

  /* 2: Translate (ox,oy) to the origin */
  int ox, oy;		/* Origin of image */

  /* 3: Flip vertically about the x axis */
  int increment;	/* +1 or -1 for offset of row n+1 from row n. */

  /* 4: Crop to width w, height h */
  int w, h;		/* Width and height of output image */

  /* 5: Flip horizontally about the vertical centre-line of the image */
  int mirror;		/* Set if mirroring rows end-for-end. */

  gint32 image_ID;
  gint32 ncolors;
  gint32 real_bpp;
  gint32 samples;
  GimpImageBaseType base_type;
  guint16 *cmap;
  guint16 *alpha_table;
  guint16 *tmp;
  gint last_printed_percent;
  gint initialized;
  gint is_cmyk;
  gint _8to16_b; /* shall the image be converted from 8 -> 16-bit ? */
} Gimp_Image_t;

static const char *Image_get_appname(stp_image_t *image);
static void Image_conclude(stp_image_t *image);
static stp_image_status_t
Image_get_row             ( stp_image_t       *image,
                            unsigned char     *data,
                            size_t             byte_limit,
                            int                row);
static int Image_height(stp_image_t *image);
static int Image_width(stp_image_t *image);
static void Image_reset(stp_image_t *image);
static void Image_init(stp_image_t *image);
static void Image_transpose(stpui_image_t *image);
static void Image_hflip(stpui_image_t *image);
static void Image_vflip(stpui_image_t *image);
static void Image_rotate_ccw(stpui_image_t *image);
static void Image_rotate_cw(stpui_image_t *image);
static void Image_rotate_180(stpui_image_t *image);
static void Image_crop(stpui_image_t *image, int, int, int, int);

static stpui_image_t theImage =
{
  {
    Image_init,
    Image_reset,
    Image_width,
    Image_height,
    Image_get_row,
    Image_get_appname,
    Image_conclude,
    NULL,
  },
  Image_transpose,
  Image_hflip,
  Image_vflip,
  Image_rotate_ccw,
  Image_rotate_cw,
  Image_rotate_180,
  Image_crop
};

static void
compute_alpha_table(Gimp_Image_t *image)
{
  unsigned val, alpha;
  if (image->real_bpp == 2 || image->real_bpp == 4) {
    image->alpha_table = xmalloc(65536 * sizeof(unsigned char));
    for (val = 0; val < 256; val++)
      for (alpha = 0; alpha < 256; alpha++)
        image->alpha_table[(val * 256) + alpha] =
	  val * alpha / 255 + 255 - alpha;
  } else { // might be broken - not shure if we go this way
    image->alpha_table = xmalloc(65536 * sizeof(guint16));
    for (val = 0; val < 65536; val++)
      for (alpha = 0; alpha < 65536; alpha++)
        image->alpha_table[(val * 65536) + alpha] =
	  val * alpha / 65535 + 65535 - alpha;
  }
}

static inline unsigned char
alpha_lookup(Gimp_Image_t *image, int val, int alpha)
{
  return image->alpha_table[(val * 65536) + alpha];
}

stpui_image_t *
Image_GimpDrawable_new  ( GimpDrawable *drawable,
                          gint32        image_ID)
{
  Gimp_Image_t *im = xmalloc(sizeof(Gimp_Image_t));
  memset(im, 0, sizeof(Gimp_Image_t));
  im->drawable = drawable;
  gimp_pixel_rgn_init(&(im->rgn), drawable, 0, 0,
                      drawable->width, drawable->height, FALSE, FALSE);
  im->image_ID = image_ID;
  im->base_type = gimp_image_base_type(drawable->id);
  im->is_cmyk = 0;
  im->initialized = 0;
  theImage.im.rep = im;
  theImage.im.reset(&(theImage.im));

  /*switch (im->base_type)
    {
    case GIMP_INDEXED:
      im->cmap = gimp_image_get_cmap(image_ID, &(im->ncolors));
      im->real_bpp = 3;
      break;
    case GIMP_GRAY:
      im->real_bpp = 1;
      break;
    case GIMP_RGB:
      im->real_bpp = 3;
      break;
    case U16_GRAY:
    case U16_RGB:   // we use CMYK stored in an RGBA drawable
      break;
    default:
      g_print ("%s:%d %s() return NULL\n",__FILE__,__LINE__,__func__);
      return NULL;
      break;
    }*/

  im->real_bpp = gimp_drawable_bpp (drawable->id);

  {
    char* colourspace = "";
    if( gimp_image_has_icc_profile (image_ID, ICC_IMAGE_PROFILE))
      colourspace = gimp_image_get_icc_profile_color_space_name ( image_ID,
                                                         ICC_IMAGE_PROFILE);
    if((strlen(colourspace) && strcmp ("Cmyk",colourspace) == 0)) {
      im->is_cmyk = 1;
      printf("%s:%d is a Cmyk image\n",__FILE__,__LINE__);
    } else
      printf("is not a Cmyk image\n");
  }

  // any conversion must be done before
  switch (gimp_drawable_type (drawable->id))
    {
    case RGB_IMAGE:
      im->samples = 3;
      break;
    case RGBA_IMAGE:
      {
        im->real_bpp = 3;
        im->samples = 3;
        if( im->is_cmyk )
        {
          im->real_bpp = 8;
          im->samples = 4;
        } else
          compute_alpha_table(im);
      }
      break;
    case GRAY_IMAGE:
      im->real_bpp = 1;
      im->samples = 1;
      break;
    case GRAYA_IMAGE:
      compute_alpha_table(im);
      im->real_bpp = 1;
      im->samples = 2;
      break;
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
      im->cmap = gimp_image_get_cmap(image_ID, &(im->ncolors));
      im->real_bpp = 3;
      im->samples = im->real_bpp/2;
      break;
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
      im->samples = im->real_bpp/2;
      break;
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    default:
      g_message ("%s:%d %s()\n Drawable typ is not supported by this version of print. \n",__FILE__,__LINE__,__func__);
      return NULL;
      break;
  }

  g_print ("%s:%d %s() image_ID %d im->base_type %d\n  im->real_bpp %d|%d im->alpha_table %d\n",__FILE__,__LINE__,__func__,image_ID, im->base_type, im->real_bpp, im->samples, (int)im->alpha_table);

  return &theImage;
}

static void
Image_init(stp_image_t *image)
{
  /* Nothing to do. */
}

static void
Image_reset(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  im->columns = FALSE;
  im->ox = 0;
  im->oy = 0;
  im->increment = 1;
  im->w = im->drawable->width;
  im->h = im->drawable->height;
  im->mirror = FALSE;
}

static void
Image_hflip(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  im->mirror = !im->mirror;
}

static void
Image_vflip(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  im->oy += (im->h-1) * im->increment;
  im->increment = -im->increment;
}

/*
 * Image_crop:
 *
 * Crop the given number of pixels off the LEFT, TOP, RIGHT and BOTTOM
 * of the image.
 */

static void
Image_crop(stpui_image_t *image, int left, int top, int right, int bottom)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  int xmax = (im->columns ? im->drawable->height : im->drawable->width) - 1;
  int ymax = (im->columns ? im->drawable->width : im->drawable->height) - 1;

  int nx = im->ox + im->mirror ? right : left;
  int ny = im->oy + top * (im->increment);

  int nw = im->w - left - right;
  int nh = im->h - top - bottom;

  int wmax, hmax;

  if (nx < 0)         nx = 0;
  else if (nx > xmax) nx = xmax;

  if (ny < 0)         ny = 0;
  else if (ny > ymax) ny = ymax;

  wmax = xmax - nx + 1;
  hmax = im->increment ? ny + 1 : ymax - ny + 1;

  if (nw < 1)         nw = 1;
  else if (nw > wmax) nw = wmax;

  if (nh < 1)         nh = 1;
  else if (nh > hmax) nh = hmax;

  im->ox = nx;
  im->oy = ny;
  im->w = nw;
  im->h = nh;
}

static void
Image_rotate_ccw(stpui_image_t *image)
{
  Image_transpose(image);
  Image_vflip(image);
}

static void
Image_rotate_cw(stpui_image_t *image)
{
  Image_transpose(image);
  Image_hflip(image);
}

static void
Image_rotate_180(stpui_image_t *image)
{
  Image_vflip(image);
  Image_hflip(image);
}

static int
Image_width(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  return im->w;
}

static int
Image_height(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  return im->h;
}

static stp_image_status_t
Image_get_row             ( stp_image_t       *image,
                            unsigned char     *data,
                            size_t             byte_limit,
                            int                row)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  int last_printed_percent;
  guint16 *inter,
          *dest;

  dest = (guint16*) data;

  if (!im->initialized)
    {
      gimp_progress_init(_("Printing..."));
      im->_8to16_b = 0;

#if 0
      switch (im->base_type)
	{
	case GIMP_INDEXED:
	  im->tmp = xmalloc(im->drawable->bpp * im->w);
	  break;
	case GIMP_GRAY:
	  if (im->drawable->bpp == 2)
	    im->tmp = xmalloc(im->drawable->bpp * im->w);
	  break;
	case GIMP_RGB:
	  if (im->drawable->bpp == 4)
	    im->tmp = xmalloc(im->drawable->bpp * im->w);
	  break;
	}
#else
      switch (im->base_type)
        {
        case GIMP_INDEXED:
          im->tmp = xmalloc(im->drawable->bpp * im->w);
          break;
        case GIMP_GRAY:
          if (im->drawable->bpp == 2)
            im->tmp = xmalloc(im->drawable->bpp * im->w);
          break;
        case GIMP_RGB:
          if (im->drawable->bpp == 4) {
            if(im->is_cmyk) {
              im->tmp = xmalloc(im->drawable->bpp * 2 * im->w);
              im->_8to16_b = 1;
            } else {
              im->tmp = xmalloc(im->drawable->bpp * im->w);
            }
          }
          break;
        case U16_GRAY:
          if (im->drawable->bpp == 4)
            im->tmp = xmalloc(im->drawable->bpp * im->w);
          break;
        case U16_RGB:
          if (im->drawable->bpp == 8)
            im->tmp = xmalloc(im->drawable->bpp * im->w);
          g_print ("%s:%d %s() im->base_type %d\nim->real_bpp %d im->alpha_table %d im->cmap %d im->mirror %d im->tmp %d im->columns %d\n",__FILE__,__LINE__,__func__, im->base_type, im->real_bpp, (int)im->alpha_table, (int)im->cmap, im->mirror, (int)im->tmp, im->columns);
          break;
        case U16_INDEXED:
        case FLOAT16_GRAY:
        case FLOAT16_RGB:
        case FLOAT_GRAY:
        case FLOAT_RGB:
        default:
            g_message ("%s:%d %s() should not been reached\n",
                        __FILE__,__LINE__,__func__);
          break;
        g_print ("%s:%d %s() im->base_type %d\nim->real_bpp %d im->alpha_table %d im->cmap %d im->mirror %d im->tmp %d im->columns %d\n",__FILE__,__LINE__,__func__, im->base_type, im->real_bpp, (int)im->alpha_table, (int)im->cmap, im->mirror, (int)im->tmp, im->columns);

        }
#endif

      im->initialized = 1;
    }    
  if (im->tmp)
    inter = im->tmp;
  else
    inter = dest;

  if (im->columns)
    gimp_pixel_rgn_get_col(&(im->rgn), (guchar*) inter,
                           im->oy + row * im->increment, im->ox, im->w);
  else
    gimp_pixel_rgn_get_row(&(im->rgn), (guchar*) inter,
                           im->ox, im->oy + row * im->increment, im->w);

  if(im->_8to16_b)
  {
    int i = 0,
        samples = im->w * im->samples;
    guchar *tmp = (guchar*) inter;

    for(i = samples-1; i >= 0; --i)
    {
      inter[i] = (guint16)tmp[i]*257;
    }
  }
  //g_print ("%s:%d %s() im->base_type %d\nim->real_bpp %d im->alpha_table %d im->cmap %d im->mirror %d im->tmp %d im->columns %d drawable->id %d\n",__FILE__,__LINE__,__func__, im->base_type, im->real_bpp, (int)im->alpha_table, (int)im->cmap, im->mirror, (int)im->tmp, im->columns, im->drawable->id);

  if (im->cmap)
    {
      int i;
      printf ("%s:%d %s(): using cmap\n",__FILE__,__LINE__,__func__);
      if (im->alpha_table)
	{
	  for (i = 0; i < im->w; i++)
	    {
	      int j;
	      for (j = 0; j < 4; j++)
		{
		  gint32 tval = im->cmap[(4 * inter[2 * i]) + j];
		  dest[(4 * i) + j] = alpha_lookup(im, tval,
						   inter[(2 * i) + 1]);
		}

	    }
	}
      else
	{
	  for (i = 0; i < im->w; i++)
	    {
	      dest[(4 * i) + 0] = im->cmap[(4 * inter[i]) + 0];
	      dest[(4 * i) + 1] = im->cmap[(4 * inter[i]) + 1];
	      dest[(4 * i) + 2] = im->cmap[(4 * inter[i]) + 2];
	    }
	}
    }
  else if (im->alpha_table)
    {
      int i;
      printf ("%s:%d %s(): using alpha_table\n",__FILE__,__LINE__,__func__);
      for (i = 0; i < im->w; i++)
	{
	  int j;
	  for (j = 0; j < im->samples; j++)
	    {
	      gint32 tval = inter[(i * im->drawable->bpp) + j];
	      dest[(i * im->real_bpp) + j] =
		alpha_lookup(im, tval,
			     inter[((i + 1) * im->drawable->bpp) - 1]);
	    }

	}
    }

  if (im->mirror)
    {
      /* Flip row -- probably inefficiently */
      int f; 
      int l;
      int samples = im->samples;
      static int mirror=1;
      for (f = 0, l = im->w - 1; f < l; f++, l--)
      {
        int c;
        if((im->real_bpp >= 4 && im->samples <= 2) ||
           (im->real_bpp > 4 && im->samples > 2 ))
        {
          guint16 tmp16;
          if(mirror){printf("16bit spiegeln\n");mirror=0;};
          for (c = 0; c < samples; c++)
	      {
	        tmp16 = dest[f*samples + c];
	        dest[f*samples + c] = dest[l*samples + c];
	        dest[l*samples + c] = tmp16;
          }
        } else {
          guint8 tmp8;
          guint8 *dest8 = (guint8*)dest;
          if(mirror){printf("8bit spiegeln %d %d\n", im->real_bpp, im->samples);mirror=0;};
          for (c = 0; c < samples; c++)
	      {
	        tmp8 = dest8[f*samples + c];
	        dest8[f*samples + c] = dest8[l*samples + c];
	        dest8[l*samples + c] = tmp8;
          }
        }
      }
    }

  last_printed_percent = row * 100 / im->h;
  if (last_printed_percent > im->last_printed_percent)
    {
      gimp_progress_update((double) row / (double) im->h);
      im->last_printed_percent = last_printed_percent;
    }
  return STP_IMAGE_STATUS_OK;
}

static void
Image_transpose(stpui_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->im.rep);
  int tmp;

  if (im->mirror) im->ox += im->w - 1;

  im->columns = !im->columns;

  tmp = im->ox;
  im->ox = im->oy;
  im->oy = tmp;

  tmp = im->mirror;
  im->mirror = im->increment < 0;
  im->increment = tmp ? -1 : 1;

  tmp = im->w;
  im->w = im->h;
  im->h = tmp;

  if (im->mirror) im->ox -= im->w - 1;
}


static void
Image_conclude(stp_image_t *image)
{
  Gimp_Image_t *im = (Gimp_Image_t *) (image->rep);
  gimp_progress_update(1);
  if (im->alpha_table)
    g_free(im->alpha_table);
  if (im->tmp)
    g_free(im->tmp);
}

static const char *
Image_get_appname(stp_image_t *image)
{
  static char pluginname[256];

  sprintf (pluginname ,"%s%s", version_ ,
//  static char pluginname[] = "Print plug-in V" VERSION " - " RELEASE_DATE
    " for CinePaint");
  return pluginname;
}


/*
 * End of "$Id: print-image-gimp.c,v 1.7 2007/01/13 21:59:12 beku Exp $".
 */

