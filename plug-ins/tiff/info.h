/*
 * "$Id: info.h,v 1.2 2006/12/11 16:01:11 beku Exp $"
 *
 *   info header for the tiff plug-in in CinePaint
 *
 *   Copyright 2003-2004   Kai-Uwe Behrmann (ku.b@gmx.de)
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

#ifndef _INFO_H_
#define _INFO_H_

#include "tiff_plug_in.h"


/***   global variables   ***/

extern char color_space_name_[];

/*** struct definitions ***/

struct _ImageInfoList_ {
  struct   _ImageInfoList_ *parent, *next; /* neighbour list chains */
  struct   _ImageInfoList_ *top; /* top list chain for standard values */
  gint32   cols, rows,		/* count of pixel per row/col */
           rowsperstrip;        /* rows per strip */
  gfloat   xres, yres;		/* tif resolution */
  gushort  res_unit;		/* physical resolution unit */
  gfloat   xpos, ypos;		/* position corresponding to x[y]res */
  gint32   offx, offy,		/* offsets of IFD */
	   max_offx, max_offy,  /* offsets over whole tif */
           viewport_offx, viewport_offy; /* offsets for viewing area */
  gint     orientation;         /* need of flipping? */
  gint     visible;             /* is this layer visible in the application? */

  gint32   image_width, image_height;  /* size   over all or viewport  */
#if GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 2
  GimpUnit unit;                /* metric  */
#endif
  gushort  bps, spp, photomet,	/* infos from tif */
	   bps_selected;	/* bits per sample, samples per pixel */
				/* photometric - colortype ( gray, rgb, lab... ) */
  unsigned short  *red,         /* indexed colour map */
          *grn, *blu;
  double   stonits;             /* candelas/meter^2 for Y = 1.0 of XYZ */
  gint     logDataFMT;          /* data type target libtiff shall convert to */
  gint     image_type, im_t_sel;/* colortype and bitdepht information */
  gint     layer_type;		/* colortype / bitdepht / alpha */
  gushort  extra, *extra_types;	/* additional channels */
  gint     alpha;		/* extra == alpha channel? */
  gint     assoc;		/* alpha is premultiplied - associated ? */
  gint    *sign;		/* sign of integers for three channels */
#if GIMP_MAJOR_VERSION == 1 && GIMP_MINOR_VERSION == 3
  GimpRGB  color;  /* for gimp-1.3 , filmgimp needs: guchar color  */
#else
  guchar   color[3];		/* cinepaint */
#endif
  gint     worst_case,
           worst_case_nonreduced;/* fallback switch */

  TiffSaveVals save_vals;	/* storing the save settings */
  ChannelData  *channel;        /* some pixel related variables */
  gint32   image_ID;
#if GIMP_MAJOR_VERSION >= 1
  GimpParasite *parasite;       /* attachments  */
#endif
  guint16  tmp;			/* ? */
#ifdef TIFFTAG_ICCPROFILE
  uint32   profile_size;        /* size in byte */
  guchar  *icc_profile;         /* ICC profiles */
  guchar  *icc_profile_info;    /* ICC profile description */
#endif
  uint16   planar;		/* RGBRGBRGB versus RRRGGGBBB */
  int      grayscale;
  int maxval, minval, numcolors;/* grayscale related */
  guint16  sampleformat;	/* data type */
  long     pagecount;		/* count of all IFD's */
  long     aktuelles_dir;	/* local IFD counter  */
  guchar   filetype;		/* TIFFTAG_(O)SUBFILETYPE */
  guint16  pagenumber;          /* IFDs number */
  guint16  pagescount;          /* stored IFDs count */
  int      pagecounting;        /* are pagenumbers stored at all? */
  int      pagecounted;         /* is pagenumber stored for this IFD? */
  char    *pagename;		/* IFD page / gimp layer name */
  char    *img_desc;  		/* image description */
  char    *software;  		/* application who created the tiff */
  char    *filename;  		/* current filename */
  char     colorspace[5];
};

typedef struct _ImageInfoList_ ImageInfo;



/*** function definitions ***/

  /* --- information list functions --- */
gint   get_tiff_info         (  TIFF            *tif,
                                ImageInfo       *current_info);
gint   get_ifd_info          (  TIFF            *tif,
                                ImageInfo       *current_info);
gint   get_image_info        (  gint32           image_ID,
                                gint32           drawable_ID,
                                ImageInfo       *info);
gint   get_layer_info        (  gint32           image_ID,
                                gint32           drawable_ID,
                                ImageInfo       *info);
gint   photometFromICC       (  char*            color_space_name,
                                ImageInfo       *info);

  /* --- variable function --- */
void   init_info             (  ImageInfo       *info);
ImageInfo* destroy_info      (  ImageInfo       *info);
char*  print_info            (  ImageInfo       *info);

#endif /*_INFO_H_ */
