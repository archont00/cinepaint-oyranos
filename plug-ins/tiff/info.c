/*
 *   info handling for the tiff plug-in in CinePaint
 *
 *   Copyright 2003-2004 Kai-Uwe Behrmann
 *
 *   separate stuff / tiff
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  March 2004
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
/*#  include <config.h> */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*#include <gdk/gdkkeysyms.h> */
#include <gtk/gtk.h>

#include "tiff_plug_in.h"
#include "info.h"

#include <lcms.h>

#ifndef DEBUG
 /*#define DEBUG*/
#endif

#ifdef DEBUG
 #define DBG(text) printf("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,text);
#else
 #define DBG(text) 
#endif

#ifndef INKSET_MULTIINK
#define     INKSET_MULTIINK             2       /* !multi-ink or hi-fi color */
#endif

/*** global variables ***/


/*** function definitions ***/

gint
get_tiff_info               (   TIFF           *tif,
                                ImageInfo      *info)
{
  long dircount = -1; 

  if (tif) {
    info->aktuelles_dir = -1; /* starting on the top */
    info->pagecount = -1;

    do { /* get all image informations we need */
      dircount++;

      if ( dircount == 0 )
      {
        info->top  = info;
        info->parent  = NULL;
        info->next  = NULL;
        init_info ( info);

        info->top->image_height = info->rows;		/* TODO needed ? */
        info->top->image_width = info->cols;

        info->pagecounting = 0;

      } else {
        m_new (info->next, ImageInfo, 1, TIFFClose (tif);return FALSE)

        info->next->top = info->top;
        info->next->parent = info;
        info->next->next = NULL;

        info = info->next;
        init_info ( info);

      }

      info->aktuelles_dir = dircount;
      info->top->pagecount = dircount + 1;

      if (!(get_ifd_info (tif, info)))
      {
        g_message ("%s:%d get_ifd_info(...) failed",__FILE__,__LINE__);
        TIFFClose (tif);
        return FALSE;
      }

    } while (TIFFReadDirectory(tif)) ;

    TIFFSetDirectory( tif, 0); /* go back to the first IFD */
  }

  info = info->top;

  for ( dircount=0 ; dircount < info->top->pagecount; dircount++)
  {
      if (dircount > 0) {
        info = info->next;
        info->pagecount = info->top->pagecount;
      }

      if (info->aktuelles_dir != dircount) {
        g_print("%s:%d %s() error in counting layers",__FILE__,__LINE__,__func__);
        return FALSE;
      }
  }

  info = info->top;

  return 1;
}

char*
getProfileInfo                   ( cmsHPROFILE  hProfile)
{
  gchar *text;
  gchar *profile_info;
  cmsCIEXYZ WhitePt;
  size_t    first = FALSE,
            min_len = 24,
            len = 0, i;

  profile_info = calloc( sizeof(char), 2048 );
  text = malloc(128);

  {  
#if LCMS_VERSION > 112
    if (cmsIsTag(hProfile, icSigCopyrightTag)) {
      len = strlen (cmsTakeCopyright(hProfile)) + 16;
      if (len > min_len)
        min_len = len + 1;
    }
#endif
    profile_info[0] = '\000';
    if (cmsIsTag(hProfile, icSigProfileDescriptionTag)) {
      sprintf (text,_("Description:    "));
      sprintf (profile_info,"%s%s %s",profile_info,text,
                              cmsTakeProductDesc(hProfile));
      if (!first) {
        int old_len = strlen (profile_info);
        if (old_len < min_len)
          len = min_len - old_len;

        for (i=0; i < len * 2.2; i++) {
          snprintf (profile_info,2048,"%s ",profile_info);
        }
      }
      sprintf (profile_info,"%s\n",profile_info);
    }
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      sprintf (text,_("Product:        "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeProductName(hProfile));
    }
#if LCMS_VERSION >= 112
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      sprintf (text,_("Manufacturer:   "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeManufacturer(hProfile));
    }
    if (cmsIsTag(hProfile, icSigDeviceModelDescTag)) {
      sprintf (text,_("Model:          "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeModel(hProfile));
    }
#endif
#if LCMS_VERSION >= 113
    if (cmsIsTag(hProfile, icSigCopyrightTag)) {
      sprintf (text,_("Copyright:      "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeCopyright(hProfile));
    }
#endif
    sprintf (profile_info,"%s\n",profile_info);

    cmsTakeMediaWhitePoint (&WhitePt, hProfile);
    _cmsIdentifyWhitePoint (text, &WhitePt);
    sprintf (profile_info, "%s%s\n", profile_info, text);

  }

#ifdef __unix__  /* reformatting the \n\r s of lcms */
  #if 0
        while (strchr(profile_info, '\r')) {
          len = (int)strchr (profile_info, '\r') - (int)profile_info;
          profile_info[len] = 32;
          profile_info[len+1] = 32;
          profile_info[len+2] = 32;
          profile_info[len+3] = '\n';
        }
  #endif
#endif
  free (text);
  
  return profile_info;
}

gint
get_ifd_info                (   TIFF           *tif,
                                ImageInfo      *ci)
{
  guint16 orientation;
  char *tiff_ptr;

  ci->worst_case = 0;

        /* compression */
  ci->save_vals.compression = COMPRESSION_NONE;
  if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &ci->tmp))
    ci->save_vals.compression = (gint)ci->tmp;

  if      (ci->save_vals.compression == COMPRESSION_SGILOG)  
    g_print ("32-bit quality SGI Log encoding\n");
  else if (ci->save_vals.compression == COMPRESSION_SGILOG24)  
    g_print ("24-bit SGI Log encoding\n");
  else if (ci->save_vals.compression == COMPRESSION_PIXARLOG)  
    g_print ("11-bit PIXAR Log encoding\n");
  else if (ci->save_vals.compression == COMPRESSION_PIXARFILM)  
    g_print ("10-bit PIXAR film encoding\n");


  if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &ci->photomet)) {
    g_message("TIFF Can't get photometric\nAssuming min-is-black\n");
    /* old AppleScan software misses out the photometric tag (and
     * incidentally assumes min-is-white, but xv assumes min-is-black,
     * so we follow xv's lead.  It's not much hardship to invert the
     * image later). */
    ci->photomet = PHOTOMETRIC_MINISBLACK;
  }

  if (ci->photomet == PHOTOMETRIC_LOGLUV
   || ci->photomet == PHOTOMETRIC_LOGL) {
    if (TIFFGetField(tif, TIFFTAG_STONITS, &ci->stonits))
      g_print ("stonits: %f in candelas/meter^2 for Y = 1.0\n",
               ci->stonits);
    else
      ci->stonits = 0.0;

    if (ci->save_vals.compression == COMPRESSION_PIXARLOG
     || ci->save_vals.compression == COMPRESSION_PIXARFILM) {
      if (SGILOGDATAFMT == SGILOGDATAFMT_8BIT) {
        ci->logDataFMT = PIXARLOGDATAFMT_8BIT;
      } else {
        ci->logDataFMT = PIXARLOGDATAFMT_FLOAT;
      }
      TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, ci->logDataFMT);
    } else {
      ci->logDataFMT = SGILOGDATAFMT;
      TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, ci->logDataFMT);
    }
  }
  if (ci->photomet == 32803) {/* Bayer pattern */
    ci->photomet = PHOTOMETRIC_MINISBLACK;
    printf("%s:%d %s() Bayer pattern detected.\n",__FILE__,__LINE__,__func__);
  }

  if (!TIFFGetField (tif, TIFFTAG_FILLORDER, &ci->save_vals.fillorder))
    ci->save_vals.fillorder = 0;

  if (!TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &ci->bps))
    ci->bps = 0;
  /* supported bps have to go here */
  if (ci->bps > 8
    	&& (ci->bps != 16
        && ci->bps != 32
        && ci->bps != 64)) {
    g_print("difficult samplevalue: %d\n", ci->bps);
    ci->worst_case = 1; /* Wrong sample width => RGBA */
  }

  if (!TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &ci->spp))
    ci->spp = 0;
  if (!TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &ci->rowsperstrip))
    ci->rowsperstrip = 1;

  if (!TIFFGetField (tif, TIFFTAG_SAMPLEFORMAT, &ci->sampleformat))
    ci->sampleformat = SAMPLEFORMAT_UINT;

  if (ci->logDataFMT == SGILOGDATAFMT_16BIT
     && ci->photomet == PHOTOMETRIC_LOGLUV)
    ci->sampleformat = SAMPLEFORMAT_UINT;

  /* HACKING!!! */
  if(ci->sampleformat == 196608) { /* ?? */
     ci->sampleformat = SAMPLEFORMAT_IEEEFP;
  }

  if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &ci->planar))
    ci->planar = PLANARCONFIG_CONTIG; /* PLANARCONFIG_SEPARATE */


  ci->alpha = 0;
  ci->assoc = 0;

  ci->extra = 0;
  TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &ci->extra, &tiff_ptr);
  if (ci->extra > 0) {
    m_new (ci->extra_types, gushort, ci->extra, return FALSE)
    memcpy (ci->extra_types, tiff_ptr, ci->extra * sizeof(gushort));
    ci->save_vals.premultiply = ci->extra_types[0];
  } else {
    ci->extra_types = NULL;
    ci->save_vals.premultiply = EXTRASAMPLE_UNSPECIFIED;
  }

  if (!TIFFGetField (tif, TIFFTAG_SUBFILETYPE, &ci->filetype))
    ci->filetype = 0x0 ;

    /* test if the extrasample represents an associated alpha channel... */
    /* These cases handle alpha for extra types not signed as associated alpha */

    if (ci->extra > 0	/* extra_types[0] is written libtiff */
	 && (ci->extra_types[0] == EXTRASAMPLE_ASSOCALPHA)) {
        ci->alpha = 1;
        ci->assoc = 1;
        ci->extra--;
#       ifdef DEBUG
        g_print ("alpha is premutliplied %s:%d\n",
                  __FILE__, __LINE__);
#       endif
    } else if (ci->extra > 0
         && (ci->extra_types[0] == EXTRASAMPLE_UNASSALPHA
          || ci->extra_types[0] == EXTRASAMPLE_UNSPECIFIED)) {
        ci->alpha = 1;
        ci->assoc = 0;
        ci->extra--;
        g_print ("alpha is not premutliplied for one of %d channels %s:%d\n",
                  ci->spp, __FILE__, __LINE__);
    } else if ((ci->photomet == PHOTOMETRIC_CIELAB
             || ci->photomet == PHOTOMETRIC_ICCLAB
             || ci->photomet == PHOTOMETRIC_ITULAB
             || ci->photomet == PHOTOMETRIC_LOGLUV
             || ci->photomet == PHOTOMETRIC_RGB 
             || ci->photomet == PHOTOMETRIC_SEPARATED)
	 && ci->spp > 3) {
        ci->alpha = 1;
        if (ci->extra > 0)
            ci->extra--;
        else
            ci->extra = ci->spp - 4; 
    } else if ((ci->photomet != PHOTOMETRIC_CIELAB
             && ci->photomet != PHOTOMETRIC_ICCLAB
             && ci->photomet != PHOTOMETRIC_ITULAB
             && ci->photomet != PHOTOMETRIC_LOGLUV
             && ci->photomet != PHOTOMETRIC_RGB
             && ci->photomet != PHOTOMETRIC_SEPARATED)
             && ci->spp > 1) {
        ci->alpha = 1;
        if (ci->extra > 0)
            ci->extra--;
        else
            ci->extra = ci->spp - 2;
    } else if ((ci->extra > 0)
         && (ci->photomet == PHOTOMETRIC_SEPARATED)) {
        g_message ("This plug-in supports CMYK only without alpha channel.");
    } else if (ci->photomet == PHOTOMETRIC_SEPARATED) {
        /* This is a workaround to allocate 4 channels. */
        ci->alpha = 1;
        ci->assoc = 0;
        ci->extra = 0;
    }
    
    if (    (ci->photomet == PHOTOMETRIC_RGB
          || ci->photomet == PHOTOMETRIC_LOGLUV
          || ci->photomet == PHOTOMETRIC_CIELAB
	  || ci->photomet == PHOTOMETRIC_ICCLAB
	  || ci->photomet == PHOTOMETRIC_ITULAB
          || ci->photomet == PHOTOMETRIC_SEPARATED)
	 && ci->spp == 3 ) {
        ci->alpha = 0;
        ci->extra = 0; 
        ci->assoc = 0;
    }

    ci->image_type = -1;
    ci->layer_type = -1;
    /* Gimp accepts layers with different alphas in an image. */
    ci->maxval = (1 << ci->bps) - 1;
    ci->minval = 0; 
    if (ci->maxval == 1 && ci->spp == 1)
      {
        ci->grayscale = 1;
        ci->image_type = GRAY;
        ci->layer_type = (ci->alpha) ? GRAYA_IMAGE : GRAY_IMAGE;
      }
    else
      {
      switch (ci->photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_LOGL:
          ci->grayscale = 1;
          if (ci->bps <= 8) {
            ci->image_type = GRAY;
          }
          else if (ci->bps == 16
             && ci->sampleformat == SAMPLEFORMAT_UINT) { /* uint16 */ 
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = U16_GRAY;
#endif
          }
          else if (ci->bps == 16
             && (ci->sampleformat == SAMPLEFORMAT_IEEEFP
              || ci->sampleformat == SAMPLEFORMAT_VOID)){/* floats16*/
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT16_GRAY; /* TODO */
#endif
          }
          else if (ci->bps == 32
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT_GRAY;
#endif
          }
          else if (ci->bps == 64
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT_GRAY;
#endif
          }
          break;
 
        case PHOTOMETRIC_MINISWHITE:
          ci->grayscale = 1;
          if (ci->bps <= 8) {
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = GRAY;
#endif
          }
          else if (ci->bps == 16
             && ci->sampleformat == SAMPLEFORMAT_UINT) { /* uint16 */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = U16_GRAY;
#endif
          }
          else if (ci->bps == 16
             && (ci->sampleformat == SAMPLEFORMAT_IEEEFP
              || ci->sampleformat == SAMPLEFORMAT_VOID)) {/*floats16*/
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT16_GRAY;
#endif
          }
          else if (ci->bps == 32
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT_GRAY;
#endif
          }
          else if (ci->bps == 64
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_GRAY;
#else
            ci->image_type = FLOAT_GRAY;
#endif
          }
          break;
 
        case PHOTOMETRIC_PALETTE:
          if (ci->alpha)
            g_print ("ignoring alpha channel on indexed color image\n");
          if (ci->bps <= 8) {
            /*if (!TIFFGetField (tif, TIFFTAG_COLORMAP,
			       &ci->redcolormap,
                               &ci->greencolormap,
			       &ci->bluecolormap))
              {
                g_print ("error getting colormaps\n");
                gimp_quit ();
              }*/
            ci->numcolors = ci->maxval + 1;
            ci->maxval = 255;
            ci->grayscale = 0;

            /*for (i = 0; i < ci->numcolors; i++)
              {
                ci->redcolormap[i] >>= 8;
                ci->greencolormap[i] >>= 8;
                ci->bluecolormap[i] >>= 8;
              }*/

            if (ci->numcolors > 256) {
                ci->image_type = RGB;
              } else {
#if GIMP_MAJOR_VERSION >= 1
                ci->image_type = GIMP_INDEXED;
#else
                ci->image_type = INDEXED;
#endif
              }
          }
          else if (ci->bps == 16)
            g_print("16bit indexed color image not implemented yet\n");
          break;
	  
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
#         ifdef DEBUG
          if (ci->photomet == PHOTOMETRIC_SEPARATED)
            g_print("separated datas; bps|spp = %d|%d, sampleformat = %d\n", ci->bps, ci->spp, ci->sampleformat);
          else
            g_print("coloured datas; bps|spp = %d|%d, sampleformat = %d\n", ci->bps, ci->spp, ci->sampleformat);
#         endif
          ci->grayscale = 0;
          if (ci->bps <= 8) {
            /*g_print("uint datas\n"); */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_RGB;
#else
            ci->image_type = RGB;
#endif
          }
          else if (ci->bps == 16
	    && ci->sampleformat == SAMPLEFORMAT_UINT) { /* uint16  */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_RGB;
#else
            ci->image_type = U16_RGB;
#endif
          }
          else if (ci->bps == 16
	    && ((ci->sampleformat == SAMPLEFORMAT_IEEEFP)
	     || (ci->sampleformat == SAMPLEFORMAT_VOID)) ) {/*floats16 */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_RGB;
#else
            ci->image_type = FLOAT16_RGB;
#endif
          }
          else if (ci->bps == 32
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_RGB;
#else
            ci->image_type = FLOAT_RGB;
#endif
          }
          else if (ci->bps == 64
             && (ci->sampleformat == SAMPLEFORMAT_UINT
              || ci->sampleformat == SAMPLEFORMAT_IEEEFP)){/* float */
#if GIMP_MAJOR_VERSION >= 1
            ci->image_type = GIMP_RGB;
#else
            ci->image_type = FLOAT_RGB;
#endif
          }
          break;

      default:
        ci->worst_case = 1;
        printf("photomet = %d\n",ci->photomet);
      }
  #if GIMP_MAJOR_VERSION >= 1
      if (ci->photomet == PHOTOMETRIC_SEPARATED
          && ci->bps <= 16)  /* This case is handled by libtiff. */
        ci->worst_case = 1;
  #endif
    }
#   ifdef DEBUG
    printf ("%s:%d IFD %ld: bps = %d\n",__FILE__,__LINE__,
             ci->aktuelles_dir, ci->bps);
#   endif
    switch (ci->image_type) {
      case GRAY:
        ci->layer_type = (ci->alpha) ? GRAYA_IMAGE : GRAY_IMAGE;
      break;
      case INDEXED:
        ci->layer_type = (ci->alpha) ? INDEXEDA_IMAGE : INDEXED_IMAGE;
      break;
      case RGB:
        ci->layer_type = (ci->alpha) ? RGBA_IMAGE : RGB_IMAGE;
      break;
#if GIMP_MAJOR_VERSION < 1
      case U16_GRAY:
        ci->layer_type = (ci->alpha) ? U16_GRAYA_IMAGE : U16_GRAY_IMAGE;
      break;
      case FLOAT16_GRAY:
        ci->layer_type = (ci->alpha) ? FLOAT16_GRAYA_IMAGE : FLOAT16_GRAY_IMAGE;
      break;
      case FLOAT_GRAY:
        ci->layer_type = (ci->alpha) ? FLOAT_GRAYA_IMAGE : FLOAT_GRAY_IMAGE;
      break;
      case U16_RGB:
        ci->layer_type = (ci->alpha) ? U16_RGBA_IMAGE : U16_RGB_IMAGE;
      break;
      case FLOAT16_RGB:
        ci->layer_type = (ci->alpha) ? FLOAT16_RGBA_IMAGE : FLOAT16_RGB_IMAGE;
      break;
      case FLOAT_RGB:
        ci->layer_type = (ci->alpha) ? FLOAT_RGBA_IMAGE : FLOAT_RGB_IMAGE;
      break;
#endif
    }

  if (ci->worst_case) {
        g_warning ("IFD: %ld fall back\n",ci->aktuelles_dir);
    ci->image_type = RGB_IMAGE;
    ci->layer_type = RGBA_IMAGE;
    if (!(ci->filetype & FILETYPE_REDUCEDIMAGE))
      ci->top->worst_case_nonreduced = TRUE;
  } else

  TIFFGetFieldDefaulted(tif,TIFFTAG_RESOLUTIONUNIT, &ci->res_unit);

  if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &ci->cols))
    ci->cols = 0;
  if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &ci->rows))
    ci->rows = 0;
  if (!TIFFGetField (tif, TIFFTAG_XPOSITION, &ci->xpos))
    ci->xpos = (gfloat)0.0;
  if (!TIFFGetField (tif, TIFFTAG_YPOSITION, &ci->ypos))
    ci->ypos = (gfloat)0.0;
  if (!TIFFGetField (tif, TIFFTAG_XRESOLUTION, &ci->xres))
    ci->xres = (gfloat)0.0;
  if (!TIFFGetField (tif, TIFFTAG_YRESOLUTION, &ci->yres))
    ci->yres = (gfloat)0.0;

  ci->image_width = ci->cols;
  ci->image_height = ci->rows;

	/* yres but no xres */
  if ( ( ci->xres == (gfloat)0.0 )
		 && ( ci->yres != (gfloat)0.0 ) ) {
      g_message("TIFF warning: no x resolution info, assuming same as y\n");
      ci->xres = ci->yres;
	/* xres but no yres */
  } else if ( ( ci->yres == (gfloat)0.0 )
		 && ( ci->xres != (gfloat)0.0 ) ) { 
    g_message("TIFF warning: no y resolution info, assuming same as x\n");
    ci->yres = ci->xres;
  } else {
      /* no res tags => we assume we have no resolution info, so we
       * don't care.  Older versions of this plugin used to write files
       * with no resolution tags at all. */

      /* If it is invalid, instead of forcing 72dpi, do not set the resolution 
       * at all. Gimp will then use the default set by the user */

      /*xres = 72.0;
      yres = 72.0;*/
  }  

	/* position information */
  ci->max_offx = 0;
  ci->max_offy = 0;
  ci->offx = (int)ROUND(ci->xpos * ci->xres);
  ci->offy = (int)ROUND(ci->ypos * ci->yres);

  if (!ci->worst_case) {
    /* size and resolution */
    if ( ci->offx > ci->top->max_offx ) {
      ci->top->max_offx = ci->offx;
    }
    if ( ci->offy > ci->top->max_offy ) {
      ci->top->max_offy = ci->offy;
    }
    /* bit depth */
    if (ci->bps > ci->top->bps_selected) {
      ci->top->im_t_sel = ci->image_type;
      ci->top->bps_selected = ci->bps;
    }
  }

  {     /* string informations */
    size_t  len = 0;
    char   *tiff_ptr = NULL;

    #define get_text_field(TIFFTAG,target) \
    TIFFGetField (tif, TIFFTAG, &tiff_ptr); \
    if (tiff_ptr != NULL) len = strlen (tiff_ptr); else len = 0; \
    if (len > 0) { \
      target = g_new (char, len + 1); \
      sprintf (target, "%s", tiff_ptr); \
      g_print (#target ": %s   %d\n",target ,(size_t)target); \
    } else { \
      target = NULL; \
    } \
    if (tiff_ptr != NULL) tiff_ptr[0] = '\000';

    get_text_field (TIFFTAG_IMAGEDESCRIPTION, ci->img_desc)
    get_text_field (TIFFTAG_PAGENAME, ci->pagename)
    get_text_field (TIFFTAG_SOFTWARE, ci->software)
    if (ci->pagename == NULL) {
      ci->pagename = g_new (gchar,32);
      sprintf (ci->pagename, "%s", _("layer") );
      sprintf (ci->pagename, "%s # %ld", ci->pagename, ci->aktuelles_dir );
    }

        /* ICC profile */
    if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &ci->profile_size,
                                               &tiff_ptr)) {
      m_new (ci->icc_profile, char, ci->profile_size, return FALSE)
      memcpy (ci->icc_profile, tiff_ptr, ci->profile_size);
      if (ci->profile_size > 0) {
        cmsHPROFILE hIn=NULL;

        hIn = cmsOpenProfileFromMem (ci->icc_profile, ci->profile_size);
        if (hIn) {
          ci->icc_profile_info = getProfileInfo (hIn);
          #ifdef DEBUG
          g_print ("%s\n",ci->icc_profile_info);
          #endif
          cmsCloseProfile (hIn);
          #ifdef DEBUG_
          saveProfileToFile ("/tmp/cinepaint_tiff_load.icc", ci->icc_profile,
                             ci->profile_size);
          DBG ("profile saved to /tmp/cinepaint_tiff_load.icc")
          #endif
        } else {
          ci->profile_size = 0;
          ci->icc_profile = NULL;
        }
      }
    } else {
      ci->profile_size = 0;
      ci->icc_profile = NULL;
    }
  }
     /* searching for viewport position and size */
  ci->viewport_offx = 0;
  ci->viewport_offy = 0;
  if (  ci->pagename != NULL
    && (strcmp (ci->pagename, "viewport") == 0) /* test for our own */
    && (ci->filetype & FILETYPE_MASK) )
  {  /*viewport definitions */
      ci->top->image_height = 
        ( ci->rows * ci->top->yres / ci->yres);
      ci->top->image_width = 
        ( ci->cols * ci->top->xres / ci->xres);
      ci->top->viewport_offx = ci->offx;
      ci->top->viewport_offy = ci->offy;
  } else if (!ci->worst_case) {
        /* show all IFD s */
      /* actual IFD more bottom? */
    if ( ( ci->rows + (guint16)(ci->ypos * ci->yres) )
          >  ci->top->image_height ) 
        ci->top->image_height = ci->rows
		 + (guint16)(ci->ypos * ci->yres);
      /* actual IFD more right? */
    if ( ( ci->cols + (guint16)(ci->xpos * ci->xres) )
           >  ci->top->image_width ) 
        ci->top->image_width = ci->cols
			 + (guint16)(ci->xpos * ci->xres);
  }

  if (TIFFGetField (tif, TIFFTAG_PAGENUMBER,
                        &ci->pagenumber, &ci->pagescount)) {
    if (strcmp (ci->pagename, "viewport") != 0)
      ci->top->pagecounting = 1;  /* IFDs are counted */
    ci->pagecounted = 1;          /* counted IFDs shall be visible */
  } else {
    ci->pagenumber = 0;
    ci->pagescount = 0;
    ci->pagecounted = 0;          /* if pagecounting==1 -> invisible */
  }

  if (TIFFGetField (tif, TIFFTAG_ORIENTATION, &orientation))
    ci->orientation = (int)orientation;
  else
    ci->orientation = -1;

  return 1;
}

gint
get_image_info               (  gint32           image_ID,
                                gint32           drawable_ID,
                                ImageInfo       *info)
{
  gint nlayers;
  long dircount; 
  gint *layers;

  nlayers = -1;

  /* Die Zahl der Layer / number of layers */
  layers = gimp_image_get_layers (image_ID, &nlayers);
  /*g_message ("nlayers = %d", nlayers); */

  if ( nlayers == -1 ) {
    g_message("no image data to save");
    return FALSE;
  }


  {
    info->aktuelles_dir = -1; /* starting on the top */
    info->pagecount = -1;

    for (dircount=0 ; dircount < nlayers ; dircount++)
    {
      drawable_ID = layers[dircount];

      if ( dircount == 0 )
      {
        info->top  = info;
        info->parent  = NULL;
        info->next  = NULL;

        init_info ( info);
        info->top->image_height = info->rows;		/* TODO needed ? */
        info->top->image_width = info->cols;

        info->pagecounting = 0;

      } else {
        m_new (info->next, ImageInfo, 1, return FALSE)

        info->next->top = info->top;
        info->next->parent = info;
        info->next->next = NULL;

        info = info->next;
        init_info ( info);

      }

      if (!(get_layer_info (image_ID, drawable_ID, info)))
      {
        g_message ("get_layer_info(...) failed");
        return FALSE;
      }

      info->aktuelles_dir = dircount;
      info->pagecount = nlayers;

    }
  }

  info = info->top;

  for ( dircount=0 ; dircount < nlayers; dircount++) {

      if (dircount > 0)
        info = info->next;

      if (info->aktuelles_dir != dircount) {
        g_print("%s:%d %s() error in counting layers",__FILE__,__LINE__,__func__);
        return FALSE;
      }

    /* Handle offsets and resolution */ 
    if ( info->offx != info->top->max_offx ) {
      info->xpos = (gfloat)(info->offx - info->top->max_offx) / info->xres;
    }
    if ( info->offy != info->top->max_offy ) {
      info->ypos = (gfloat)(info->offy - info->top->max_offy) / info->yres;
    }
  }

  info = info->top;

  m_free (layers)
  return 1;
}

int
photometFromICC (char* color_space_name, ImageInfo *info)
{
   int BitsPerSample = info->bps;
   int SamplesPerPixel = info->spp;
   int photomet = info->photomet, ink=0;
   int use_icc_profile = info->save_vals.use_icc_profile;

   
   if (strcmp (color_space_name, "Gray") == 0) {/*PT_GRAY */
           photomet = PHOTOMETRIC_MINISBLACK;
           DBG("")
   } else
   if (strcmp (color_space_name, "Rgb") == 0) {/*PT_RGB */
           if (SamplesPerPixel >= 3)
             photomet = PHOTOMETRIC_RGB;
           else {
             g_warning ("%s:%d %d SamplesPerPixel dont matches RGB",
                         __FILE__,__LINE__,SamplesPerPixel);
             use_icc_profile = FALSE;
           }
           DBG("")
   } else
   if (strcmp (color_space_name, "Cmy") == 0) {/*PT_CMY */
           if (SamplesPerPixel >= 3)
             photomet = PHOTOMETRIC_SEPARATED;
           else {
             g_warning ("%s:%d %d SamplesPerPixel dont matches CMY",
                         __FILE__,__LINE__,SamplesPerPixel);
             use_icc_profile = FALSE;
           }
           ink = INKSET_MULTIINK;
           DBG("")
           /*break; */
   } else
   if (strcmp (color_space_name, "Cmyk") == 0) { /*CMYK */
           if (SamplesPerPixel >= 4) {
             photomet = PHOTOMETRIC_SEPARATED;
             ink = INKSET_CMYK;
           } else {
             g_warning ("%s:%d %d SamplesPerPixel dont matches CMYK",
                         __FILE__,__LINE__,SamplesPerPixel);
             if (SamplesPerPixel == 3)
               photomet = PHOTOMETRIC_SEPARATED;
             use_icc_profile = FALSE;
           }
           DBG("")
           /*break; */
    } else
   if (strcmp (color_space_name, "Lab") == 0) { /*PT_Lab */
           if (SamplesPerPixel >= 3) {
             if (BitsPerSample <= 16) 
               photomet = PHOTOMETRIC_ICCLAB;
             else
               photomet = PHOTOMETRIC_CIELAB;
           } else {
             g_warning ("%s:%d %d SamplesPerPixel dont matches CIE*Lab",
                         __FILE__,__LINE__,SamplesPerPixel);
             use_icc_profile = FALSE;
           }
           DBG("")
           /* break; */
   } else
   if (strcmp (color_space_name, "Lab") == 0) { /*PT_XYZ */
          if (BitsPerSample == 32 && SamplesPerPixel >= 3) {
            info->photomet = PHOTOMETRIC_LOGLUV;
            info->save_vals.compression = COMPRESSION_SGILOG;
          } else {
            g_warning ("%s:%d %d XYZ is saveable in float bit depth only",
                        __FILE__,__LINE__,SamplesPerPixel);
          }
          DBG("")
          /*break; */
   }
   info->photomet = photomet;
   info->save_vals.use_icc_profile = use_icc_profile;

   return ink;
}

gint
get_layer_info               (  gint32           image_ID,
                                gint32           drawable_ID,
				ImageInfo	*info)
{
  guchar *cmap;
  int colors, i;
  GimpDrawable *drawable = NULL;
  GimpImageType drawable_type;
 
  /* Ist unser Layer sichbar? / is this layer visible? */
  info->visible = gimp_layer_get_visible( drawable_ID);

  /* switch for TIFFTAG_SAMPLEFORMAT ; default = 1 = uint */
  info->sampleformat = SAMPLEFORMAT_UINT; 


  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);

  info->cols = drawable->width;
  info->rows = drawable->height;

  info->planar = PLANARCONFIG_CONTIG;
  info->orientation = ORIENTATION_TOPLEFT;

  switch (drawable_type)
        {
        case RGB_IMAGE:
          info->spp = 3;
          info->bps = 8;
          info->photomet = PHOTOMETRIC_RGB;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 0;
          break;
        case GRAY_IMAGE:
          info->spp = 1;
          info->bps = 8;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 0;
          break;
        case RGBA_IMAGE:
          info->spp = 4;
          info->bps = 8;
          info->photomet = PHOTOMETRIC_RGB;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 1;
          break;
        case GRAYA_IMAGE:
          info->spp = 2;
          info->bps = 8;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 1;
          break;
        case INDEXED_IMAGE:
          info->spp = 1;
          info->bps = 8;
          info->photomet = PHOTOMETRIC_PALETTE;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 0;

          cmap = gimp_image_get_cmap (image_ID, &colors);
          m_new (info->red, unsigned short,colors, return FALSE)
          m_new (info->grn, unsigned short,colors, return FALSE)
          m_new (info->blu, unsigned short,colors, return FALSE)

          for (i = 0; i < colors; i++)
            {
              info->red[i] = *cmap << 8; cmap++;
              info->grn[i] = *cmap << 8; cmap++;
              info->blu[i] = *cmap << 8; cmap++;
	    }
          break;
        case INDEXEDA_IMAGE:
          g_print ("tiff save_image: can't save INDEXEDA_IMAGE\n");
	  m_free (drawable)
          return 0;
#if GIMP_MAJOR_VERSION < 1
        case U16_RGB_IMAGE:
          info->spp = 3;
          info->bps = 16;
          info->photomet = PHOTOMETRIC_ICCLAB;
          info->photomet = PHOTOMETRIC_RGB;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 0;
          break;
        case U16_GRAY_IMAGE:
          info->spp = 1;
          info->bps = 16;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->alpha = 0;
          break;
        case U16_RGBA_IMAGE:
          info->spp = 4;
          info->bps = 16;
          info->sampleformat = SAMPLEFORMAT_UINT; 
          info->photomet = PHOTOMETRIC_RGB;
          info->alpha = 1;
          break;
        case U16_GRAYA_IMAGE:
          info->spp = 2;
          info->bps = 16;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->alpha = 1;
          break;
        case U16_INDEXED_IMAGE:
          g_print ("tiff save_image: U16_INDEXED_IMAGE save not implemented\n");
	  m_free (drawable)
          return 0;
/* 
          info->spp = 1;
          info->bps = 16;
          info->photomet = PHOTOMETRIC_PALETTE;
          alpha = 0;
          cmap = gimp_image_get_cmap (image_ID, &colors);

          for (i = 0; i < colors; i++)
	    {
	      red[i] = *cmap++ << 8;
	      grn[i] = *cmap++ << 8;
	      blu[i] = *cmap++ << 8;
	    }
*/
          break;
        case U16_INDEXEDA_IMAGE:
          g_print ("tiff save_image: can't save U16_INDEXEDA_IMAGE\n");
	  m_free (drawable)
          return 0;
       case FLOAT16_GRAY_IMAGE:
          info->spp = 1;
          info->bps = 16;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->alpha = 0;
          break;
        case FLOAT16_RGB_IMAGE:
          info->spp = 3;
          info->bps = 16;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_RGB;
          info->alpha = 0;
          break;
       case FLOAT16_GRAYA_IMAGE:
          info->bps = 16;
          info->alpha = 1;
          info->spp = 1 + info->alpha;	/* 2 TODO */
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          break;
        case FLOAT16_RGBA_IMAGE:
          info->bps = 16;
          info->alpha = 1;
          info->spp = 3 + info->alpha; /* 4 */
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_RGB;
          break;
        case FLOAT_RGB_IMAGE:
          info->spp = 3;
          info->bps = 32;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_RGB;
          info->alpha = 0;
          break;
       case FLOAT_GRAY_IMAGE:
          info->spp = 1;
          info->bps = 32;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->alpha = 0;
          break;
        case FLOAT_RGBA_IMAGE:
          info->spp = 4;
          info->bps = 32;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_RGB;
          info->alpha = 1;
          break;
        case FLOAT_GRAYA_IMAGE:
          info->spp = 2;
          info->bps = 32;
          info->sampleformat = SAMPLEFORMAT_IEEEFP;
          info->photomet = PHOTOMETRIC_MINISBLACK;
          info->alpha = 1;
          break;
#endif /*  */
        default:
          g_print ("Can't save this image type\n");
	  m_free (drawable)
          return 0;
      }

#ifndef GIMP_MAJOR_VERSION
  if (gimp_image_has_icc_profile(image_ID, ICC_IMAGE_PROFILE)) {
    int size, ink;
    cmsHPROFILE profile=NULL;
    char *mem_profile=NULL;

    info->icc_profile = NULL;
    info->profile_size = 0;
    info->icc_profile_info = NULL;
    mem_profile = gimp_image_get_icc_profile_by_mem(image_ID, &size, ICC_IMAGE_PROFILE);
    if (mem_profile && size) {
      profile = cmsOpenProfileFromMem (mem_profile, size);
      sprintf (color_space_name_,
               gimp_image_get_icc_profile_color_space_name (image_ID, ICC_IMAGE_PROFILE));
      DBG (color_space_name_)
    
      ink = photometFromICC(color_space_name_, info);

      info->profile_size = size;
      info->icc_profile = mem_profile;
      info->icc_profile_info = calloc (sizeof (char),
          strlen( gimp_image_get_icc_profile_info(image_ID, ICC_IMAGE_PROFILE) )
          + 1);
      sprintf (info->icc_profile_info,
                  gimp_image_get_icc_profile_info(image_ID, ICC_IMAGE_PROFILE));
      memcpy (&info->colorspace[0], color_space_name_, 4);
      info->colorspace[4] = '\000';
      DBG (info->icc_profile_info)
    } else {
#     ifdef DEBUG
      printf ("%s:%d could not get an profile\n",__FILE__,__LINE__);
#     endif
      info->save_vals.use_icc_profile = FALSE;
    }
    if (strcmp (gimp_image_get_icc_profile_color_space_name(image_ID, ICC_IMAGE_PROFILE) , "XYZ")
        == 0) {
          if (info->bps == 32 && info->spp >= 3) {
            info->photomet = PHOTOMETRIC_LOGLUV;
            info->save_vals.compression = COMPRESSION_SGILOG;
            sprintf (color_space_name_, "XYZ");
          } else {
            g_warning ("%s:%d bps:%d spp:%d XYZ is saveable in float bit depth only",
                        __FILE__,__LINE__,info->bps,info->spp);
          }
    }
      
    #ifdef DEBUG
    printf ("%s:%d info->photomet = %d\n",__FILE__,__LINE__,info->photomet);
    #endif
  } else
    info->save_vals.use_icc_profile = -1;
  #ifdef DEBUG
  printf ("%s:%d save_vals.compression = %d\n",__FILE__,__LINE__,
                                         info->save_vals.compression);
  #endif
#endif

  if (info->alpha) {
    if ( (info->photomet == PHOTOMETRIC_MINISWHITE
       || info->photomet == PHOTOMETRIC_MINISBLACK
       || info->photomet == PHOTOMETRIC_RGB
       || info->photomet == PHOTOMETRIC_YCBCR )
      && info->save_vals.premultiply == EXTRASAMPLE_UNSPECIFIED )
    {
      info->save_vals.premultiply = EXTRASAMPLE_ASSOCALPHA;
    } else if (info->photomet == PHOTOMETRIC_SEPARATED && info->spp <= 4) {
      info->save_vals.premultiply = EXTRASAMPLE_UNSPECIFIED;
    } else if ( info->save_vals.premultiply == EXTRASAMPLE_UNSPECIFIED ) {
      info->save_vals.premultiply = EXTRASAMPLE_UNASSALPHA;
    }
  } else {
    info->save_vals.premultiply = EXTRASAMPLE_UNSPECIFIED;
  }



#if GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 2
  /* resolution fields */ 
  {
        GimpUnit unit;
        gfloat   factor;

        info->res_unit = RESUNIT_INCH;
    
        gimp_image_get_resolution (orig_image_ID, &info->xres, &info->yres);
        unit = gimp_image_get_unit (orig_image_ID);
        factor = gimp_unit_get_factor (unit);
    
        /*  if we have a metric unit, save the resolution as centimeters
         */
        if ((ABS (factor - 0.0254) < 1e-5) ||  /* m  */
            (ABS (factor - 0.254) < 1e-5) ||   /* dm */
            (ABS (factor - 2.54) < 1e-5) ||    /* cm */
            (ABS (factor - 25.4) < 1e-5))      /* mm */
          {
            info->res_unit = RESUNIT_CENTIMETER;
            info->xres /= 2.54;
            info->yres /= 2.54;
          }
    
        if (info->xres <= 1e-5 && info->yres <= 1e-5)
          {
            info->xres = 72.0;
            info->yres = 72.0;
          }
  }
#endif


  /* Handle offsets */ 
  info->max_offx = 0;
  info->max_offy = 0;
  gimp_drawable_offsets( drawable_ID, &info->offx, &info->offy);
  if ( info->offx < info->top->max_offx )
    info->top->max_offx = info->offx;
  if ( info->offy < info->top->max_offy )
    info->top->max_offy = info->offy;

      
  info->pagename = gimp_layer_get_name (drawable_ID);
  gimp_drawable_detach (drawable);

  return 1;
}


  /* --- variable function --- */

void
init_info                    (  ImageInfo       *info)
{
  info->photomet=0;
  info->spp=0;
  info->bps=0;
  info->save_vals.compression = tsvals_.compression;
  info->save_vals.fillorder = tsvals_.fillorder;
  info->save_vals.premultiply = tsvals_.premultiply;
  info->save_vals.use_icc_profile = tsvals_.use_icc_profile;

  info->cols=0, info->rows=0;
  info->offx = 0;
  info->offy = 0;
  info->max_offx = 0;
  info->max_offy = 0;
  info->xres = (gfloat)72.0;
  info->yres = (gfloat)72.0; /* resolution */
  info->xpos = (gfloat)0.0;
  info->ypos = (gfloat)0.0;  /* relative position */
  info->visible=0;
  info->stonits=0.0;  /*Test: 119.333333; */
  info->logDataFMT = SGILOGDATAFMT;
  info->profile_size=0;

  info->channel = NULL;
  info->software = NULL;
  info->img_desc = NULL;
  info->pagename = NULL;
  info->filename = NULL;
  info->extra_types = NULL;
  info->icc_profile = NULL;
  info->icc_profile_info = NULL;
  info->colorspace[0] = '\000';
}

ImageInfo*
destroy_info                 (  ImageInfo       *info)
{
  long dircount;

  info = info->top;

  for ( dircount=0 ; dircount < info->pagecount; dircount++) {

    if (dircount > 0) {
      info = info->next;
    }
  }

  for ( dircount = info->pagecount -1 ; dircount >= 0; dircount--) {

    if (info->img_desc != NULL) {
      m_free (info->img_desc);
    }
    if (info->software != NULL) {
      m_free (info->software);
    }
    if (info->pagename != NULL) {
      m_free (info->pagename);
    }
    if (info->filename != NULL) {
      m_free (info->filename);
    }
    if (info->extra_types != NULL) {
      m_free (info->extra_types);
    }
    if (info->channel != NULL) {
      m_free (info->channel);
    }
    if (info->profile_size > 0) {
      m_free (info->icc_profile)
      m_free (info->icc_profile_info)
      info->profile_size = 0;
    }

    if ( dircount > 0 ) {
      info = info->parent;
      m_free (info->next)
    } else {
      m_free (info)
    }
  }

  return info;
}

char*
print_info                    (  ImageInfo       *info)
{
  static char text[1024] = {0};
  char* ptr = (char*)&text;
  int i;

  for (i=0; i<1024; i++)
    text[i] = 0;

  #define txt_i_add( var ) sprintf (ptr, "%s" #var " = %d\n", ptr, (int)var);
  #define txt_f_add( var ) sprintf (ptr, "%s" #var " = %.4f\n", ptr,(float)var);
  #define txt_t_add( var ) sprintf (ptr, "%s" #var " = %.s\n", ptr,  var);

  txt_i_add ( info->photomet )
  txt_i_add ( info->spp )
  txt_i_add ( info->bps )
  txt_i_add ( info->top->bps_selected )
  txt_i_add ( info->rowsperstrip )

  txt_i_add ( info->visible )
  txt_i_add ( info->image_type )
  txt_i_add ( info->im_t_sel )
  txt_i_add ( info->layer_type )
  txt_i_add ( info->extra )
  txt_i_add ( info->alpha )
  txt_i_add ( info->assoc )
  txt_i_add ( info->worst_case )
  txt_i_add ( info->filetype )

  txt_i_add ( info->save_vals.compression )
  txt_i_add ( info->save_vals.fillorder )
  txt_i_add ( info->save_vals.premultiply )

  txt_i_add ( info->cols )
  txt_i_add ( info->rows )
  txt_i_add ( info->image_width )
  txt_i_add ( info->image_height )
  txt_i_add ( info->offx )
  txt_i_add ( info->offy )
  txt_i_add ( info->max_offx )
  txt_i_add ( info->max_offy )
  txt_f_add ( info->xres )
  txt_f_add ( info->yres ) /* resolution */
  txt_f_add ( info->xpos )
  txt_f_add ( info->ypos )  /* relative position */
  txt_i_add ( info->planar )
  txt_i_add ( info->grayscale )
  txt_i_add ( info->minval )
  txt_i_add ( info->maxval )
  txt_i_add ( info->numcolors )
  txt_i_add ( info->sampleformat )
  txt_i_add ( info->pagecount )
  txt_i_add ( info->aktuelles_dir )
  txt_i_add ( info->pagenumber )
  txt_f_add ( info->stonits )
  txt_i_add ( info->logDataFMT )

  /*txt_t_add ( info->channel ) */
  txt_t_add ( info->filename )
  txt_t_add ( info->software )
  txt_t_add ( info->img_desc )
  txt_t_add ( info->pagename )
  /*txt_t_add ( info->extra_types ) */
  /*txt_t_add ( info->icc_profile ) */
  txt_i_add ( info->profile_size )
  txt_t_add ( info->icc_profile_info )
  txt_t_add ( &info->colorspace[0] )

  sprintf (ptr , "%s-------------\n", ptr);

  return ptr;
}

