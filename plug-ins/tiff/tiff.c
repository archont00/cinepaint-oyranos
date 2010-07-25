/* tiff loading and saving for the GIMP
 *  -Peter Mattis
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * khk@khk.net -- 13 May 2000
 * image flipping for RGBA loader fixed
 * njl195@zepler.org.uk --  24 January 2002
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * tiff can read and write multipaged files
 *  web@tiscali.de -- 11.November 2002
 * merged with tiff.c functions from above (as in gimp-1.3.10)
 * better layerhandling 
 *  web@tiscali.de -- 03 Dezember 2002
 * sampleformat-tag for floats is written out, basic Lab loading
 * saving and loading of float16_gray without alpha
 *  web@tiscali.de -- 18 Dezember 2002
 * added dummy layer for marking gimps viewport while saving tif
 *  web@tiscali.de -- February 2003
 * bugfixes lossy strings
 *  web@tiscali.de -- Febr./March/April 2003
 * bugfixes for order missmatched function arguments and implementations
 *  web@tiscali.de -- 15 April 2003
 * infos stored in an list in _ImageInfoList_ to avoid double libtiff calls
 *  web@tiscali.de -- 22.April 2003
 * changed and added calls to set viewport in cinepaint while opening an tif
 *  web@tiscali.de -- 27.April 2003
 * <http://cch.loria.fr/documentation/IEEE754/> for 16bit floats no standard
 * using first R&H 16bit float - alpha therein is buggy
 * Put the TIFF structure to the end of save_image,
 *  to avoid exeeding of it's size.
 *  web@tiscali.de -- 05.Mai 2003
 * we differenciate CIELab and ICCLab. TODO ITULab
 * changed alpha handling - assoc must be detected - v.1.3.07
 * link against gimp-1.2 with help of the GIMP_VERSION macro
 * made image description editable for cinepaint
 * load CMYK, pipe through lcms if an ICC profile is embedded - v1.3.08
 * fixed extra channels bug - v1.3.09
 *  Kai-Uwe Behrmann (web@tiscali.de) -- September/October 2003
 * error message for downsampling in gimp added +
 * added handling of TIFFTAG_ORIENTATION tag (fixes #96611)
 *  Maurits Rijk  <lpeek.mrijk@consunet.nl> - 1. October 2003
 * save all layers and count only the visible ones - v1.3.10
 *  Kai-Uwe Behrmann (web@tiscali.de)
 * made compatible with gimp-1.3.22/23 - v1.3.11
 *  Pablo d'Angelo (pablo@mathematik.uni-ulm.de) / Kai-Uwe
 * fixed viewport and false profile bug, changed errorhandling of lcms - v1.3.12
 * added copyright tag - v1.3.13
 *  Kai-Uwe Behrmann (web@tiscali.de) -- November 2003
 * added 64-bit IEEE double support - v1.3.14
 * added support for non source tree compiling - v1.3.15
 * loading of SGI LOG Luv images into (XYZ->)sRGB colour space
 *  (taken from libtiff tif_luv.c - Greg Ward Larson) 
 * a compile time switch is included to load into 16-bit integer - v1.3.16
 *  Kai-Uwe Behrmann (web@tiscali.de) -- December 2003
 * add switch for colour transparency premultiplying or alpha (simple copy)
 * add artist entry - v1.3.17
 * modularize the save function - v1.3.18
 * gui is sensible to output type - v1.3.19
 * clean ImageInfo list for loading + bugfixes - v1.3.20
 * reading separate image planes in all bit depths - v1.4.0
 *  Kai-Uwe Behrmann (web@tiscali.de) -- January 2004
 * applying an embeded profile is now possible to all colour spaces
 * applying the stonits tag to SGI LOG Luv's
 * direct XYZ <-> CIE*Lab conversion;
 * writing of separate image planes in certain cases - v1.4.1
 * TODO more modularizing
 * memory allocation check,
 * alpha writing bug fixed - v1.4.2 
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- February 2004
 * added ICC conversion confirmation dialog for embedded profiles - v1.4.3
 * better 1-bit loading
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- March 2004
 * fixed type of sampleformat and ifdef'ed canonicalize for osX
 * added print_info for debugging - v1.4.4
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- April 2004
 * update to CinePaints CMS - v1.4.6
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- June 2004
 * TODO the stonits tag needs appropriate PDB calls
 * default profiles for known colorspaces - v1.4.8
 * correct handling of LOGLuv colorspace - v1.4.9
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- August 2004
 * support SGI Log L   Gray scale - v1.4.10
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- November 2004
 * fixed some bugs about int vs size_t for 64bit systems
 * fixed va_list argument bug
 * write by default msb2lsb as recommended for baseline tiffs
 * thanks to Bob Friesenhahn for pointing out all of the february tiff bugs
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- February 2005
 * try to hit a non reduced IFD version in the Tiff
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- October 2005
 * fix for non set HOSTNAME variable - v1.4.12
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- November 2005
 * load default cmyk profile if no embedded is found. - v1.5.0
 * use Oyranos therefore -- April 2006
 * fix saving of 16-bit floats, OpenEXR Half marking (IEEE) - v1.5.1
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- March 2007
 * fix loading of 16-bit floats with alpha
 *  Kai-Uwe Behrmann (ku.b@gmx.de) -- April 2007
 *
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 */

/*
** tifftopnm.c - converts a Tagged Image File to a portable anymap
**
** Derived by Jef Poskanzer from tif2ras.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
*
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/


#include "libgimp/gimp.h"

/***   some configuration switches   ***/

#if 0
  int write_separate_planes = TRUE;
#else
  int write_separate_planes = FALSE;
#endif

int use_icc = TRUE;

#ifdef DEBUG
 #define DBG(text) printf("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,text);
#else
 #define DBG(text)
#endif

/***   versioning   ***/

static char plug_in_version_[64];
static char Version_[] = "v1.5.1";


/***   include files   ***/

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tiff_plug_in.h"
#include "cmm_funcs.h"
#include "info.h"
#include "gui.h"

#if HAVE_OY
# if OYRANOS_NVERSION > 107
#include <oyranos.h>
# else
#include <oyranos/oyranos.h>
# endif
# ifndef OYRANOS_VERSION
# define OYRANOS_VERSION 0
# endif
#endif

/*** struct definitions ***/

/*** declaration of local functions ***/

  /* --- plug-in API specific functions --- */
static void   query      (void);
static void   run        (gchar     *name,
                          gint             nparams,
                          GimpParam *param,
                          gint            *nreturn_vals,
                          GimpParam      **return_vals);

  /* --- tiff functions --- */
static gint32 load_image    (	gchar		*filename);
static gint32 load_IFD 	    (	TIFF		*tif,
				gint32		image_ID,
				gint32		layer_ID,
				ImageInfo	*current_info);

static void   load_rgba     (	TIFF		*tif,
				ImageInfo	*current_info);
static void   load_lines    (	TIFF         	*tif,
				ImageInfo	*current_info);
static void   load_tiles    (	TIFF         	*tif,
				ImageInfo	*current_info);

static void   read_separate (   guchar          *source,
                                ImageInfo       *current_info,
                                gint32           startcol,
                                gint32           startrow,
                                gint32           cols,
                                gint32           rows,
                                gint             sample);
static void   read_64bit    (guchar       *source,
			     ChannelData  *channel,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align);
static void   read_32bit    (guchar       *source,
			     ChannelData  *channel,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align,
                             ImageInfo       *current_info);
static void   read_f16bit   (guchar       *source,
			     ChannelData  *channel,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align);
static void   read_u16bit   (guchar       *source,
			     ChannelData  *channel,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align);
static void   read_8bit     (guchar       *source,
			     ChannelData  *channel,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align);
static void   read_default  (guchar       *source,
			     ChannelData  *channel,
			     gushort       bps,
			     gushort       photomet,
			     gint32        startcol,
			     gint32        startrow,
			     gint32        cols,
			     gint32        rows,
			     gint          alpha,
			     gint          extra,
			     gint          assoc,
			     gint          align);

#if GIMP_MAJOR_VERSION >= 1
static gint   save_image    (   gchar           *filename,
                                gint32           image_ID,
                                gint32           drawable_ID,
                                gint32           orig_image_ID,
                                ImageInfo       *info);

#else
static gint   save_image    (   gchar           *filename,
			        gint32           image_ID,
			        gint32           drawable_ID,
                                ImageInfo       *info);
#endif /* gimp-1.2 */



/*** global variables ***/

char color_space_name_[12] = {"Rgb"};


TiffSaveVals tsvals_ =
{
  TRUE,                /* use_icc_profile */
  COMPRESSION_NONE,    /*  compression  */
  FILLORDER_MSB2LSB,   /*  fillorder    */
  EXTRASAMPLE_ASSOCALPHA,  /*  premultiplying the alpha channel */
};

TiffSaveInterface tsint =
{
  FALSE,                /*  run  */
  TRUE                  /*  show alpha */
};

gchar *image_comment=NULL;
gchar *image_copyright=NULL;
gchar *image_artist=NULL;


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


/*** functions ***/

MAIN()

/* Version_ */
static char* version(void)
{
  sprintf (plug_in_version_, "%s %s %s", "TIFF plug-in for", 
#if GIMP_MAJOR_VERSION >= 1
  "GIMP/(CinePaint):",
#else
  "CinePaint/(GIMP):",
#endif
  Version_);

  return &plug_in_version_[0];
}

static void
query (void)
{
  char *date=NULL;
#if GIMP_MAJOR_VERSION < 1
  char formats[] = "RGB*, GRAY*, INDEXED*, U16_RGB*, U16_GRAY*, FLOAT16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*";
#else  /* for gimp */
  char formats[] = "RGB*, GRAY*, INDEXED*";
#endif
  
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to load" },
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };
  static int nload_args = (int)sizeof (load_args) / (int)sizeof (load_args[0]);
  static int nload_return_vals = (int)sizeof (load_return_vals) / (int)sizeof (load_return_vals[0]);

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2), DEFLATE (3), JPEG (4), JP2000 (5), Adobe (6)" },
    { GIMP_PDB_INT32, "fillorder", "Fill Order: { MSB to LSB (0), LSB to MSB (1)" },
    { GIMP_PDB_INT32, "premultiply", "premultiply alpha: { unspecified (0), premultiply (1), NON premultiply (2)" }
    /*{ GIMP_PDB_STRING, "profile_name", "The name of the profile to embed." }*/
  };
  static int nsave_args = (int)sizeof (save_args) / (int)sizeof (save_args[0]);

  /*g_print ("nsave_args %d\n", nsave_args); */

  m_new (date, char,(200 + strlen(version())), gimp_quit())
  sprintf (date,"%s\n1995-1996,1998-2000,2002-2004", version()); 
  gimp_install_procedure ("file_tiff_load",
                          "loads files of the tiff file format",
                          "This plug-in loades TIFF files.\nIt can handle bit depths up to 16-bit integers per channel with conversion to 8-bit for gimp and up to 16-bit integers and 32-bit floats natively for cinepaint.\nSome color spaces are experimental such as Lab and separated. CMYK can be converted to Lab/RGB if an appropriate profile is embedded.",
                          "Spencer Kimball & Peter Mattis",
                          "Nick Lamb <njl195@zepler.org.uk>, Kai-Uwe Behrmann",
                          date,
                          "<Load>/Tiff",
                          NULL,
                          GIMP_PLUGIN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  /* The rawphoto plug-in is responsible to call TIFF if itself fails. */
  sprintf (date,"%s\n1995-1996,2002-2004", version()); 
  gimp_install_procedure ("file_tiff_save",
                          "saves files in the tiff file format",
                          "This plug-in saves TIFF files.\nBe carefully with negative offsets. They are not readable by other tiff readers.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis, Kai-Uwe Behrmann",
                          date,
                          "<Save>/Tiff",
                          formats,
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  // tdl for rsp.com.au
  gimp_register_load_handler ("file_tiff_load", "tif,tiff,tdl", "");
  gimp_register_save_handler ("file_tiff_save", "tif,tiff,tdl", "");

  m_free (date)
}

static void
run                      (gchar     *name,
                          gint             nparams,
                          GimpParam *param,
                          gint            *nreturn_vals,
                          GimpParam      **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32 image_ID;
  ImageInfo        *info=NULL;
#if GIMP_MAJOR_VERSION >= 1
  int               len=0;
  GimpParasite *parasite;
#endif /* GIMP_HAVE_PARASITES */

#if GIMP_MAJOR_VERSION >= 1
  gint32        orig_image_ID, drawable_ID;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;
#endif

# ifdef DEBUG
  { char    *dbg = malloc(2048);
    snprintf(dbg,2048, "%s:%d Locale path: %s", __FILE__,__LINE__,
             getenv("TEXTDOMAINDIR"));
    DBG( dbg )
    free (dbg);
  }
# endif
  INIT_I18N_UI();

  m_new (image_comment, char, 256, return)
  m_new (image_copyright, char, 256, return)
  m_new (image_artist, char, 256, return)
#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION > 0 && GIMP_MINOR_VERSION < 17
    sprintf (image_comment, "created with filmGIMP");
#else
  #ifdef GIMP_VERSION
    sprintf (image_comment, "created with GIMP");
  #else
    sprintf (image_comment, _("created with CinePaint"));
  #endif
#endif
  sprintf (image_copyright, "(c)");

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  printf("%s:%d %s\n",__FILE__,__LINE__, __func__);

#if GIMP_MAJOR_VERSION >= 1 /* gimp-1.2 */
  if (strcmp (name, "file_tiff_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
	  values[0].data.d_status = GIMP_PDB_SUCCESS;
          values[1].type          = GIMP_PDB_IMAGE;
          values[1].data.d_image  = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_tiff_save") == 0)
    {
      image_ID = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;


      /* Do this right this time, if POSSIBLE query for parasites, otherwise
         or if there isn't one, set the comment to an empty string */

      /*  eventually export the image  */
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
          #ifndef EXTERN_PLUG_IN
            INIT_I18N();
          #endif
	  gimp_ui_init ("tiff", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "TIFF", 
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED | 
				       GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                       GIMP_EXPORT_CAN_HANDLE_LAYERS ));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      parasite = gimp_image_parasite_find (orig_image_ID, "gimp-comment");
      if (parasite)
        len = strlen (parasite->data);
        sprintf (image_comment, parasite->data);
      gimp_parasite_free (parasite);

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data */
	  gimp_get_data ("file_tiff_save", &tsvals_);

	  parasite = gimp_image_parasite_find (orig_image_ID, 
                                               "tiff-save-options");
	  if (parasite)
	    {
	      tsvals_.compression =
		((TiffSaveVals *) parasite->data)->compression;
	    }
	  gimp_parasite_free (parasite);

	  /*  First acquire information with a dialog */
	  if (! save_dialog (info))
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
          #ifndef EXTERN_PLUG_IN
            INIT_I18N();
          #endif
	  /*  Make sure all the arguments are there! */
	  if (nparams != 8)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      switch (param[5].data.d_int32)
		{
		case 0: tsvals_.compression = COMPRESSION_NONE;     break;
		case 1: tsvals_.compression = COMPRESSION_LZW;      break;
		case 2: tsvals_.compression = COMPRESSION_PACKBITS; break;
		case 3: tsvals_.compression = COMPRESSION_DEFLATE;  break;
		case 4: tsvals_.compression = COMPRESSION_JPEG;  break;
		case 5: tsvals_.compression = COMPRESSION_JP2000;  break;
		case 6: tsvals_.compression = COMPRESSION_ADOBE_DEFLATE;  break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	      switch (param[6].data.d_int32)
		{
		case 0: tsvals_.fillorder = FILLORDER_MSB2LSB; break;
		case 1: tsvals_.fillorder = FILLORDER_LSB2MSB; break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	      switch (param[7].data.d_int32)
		{
		case 0: tsvals_.premultiply = EXTRASAMPLE_UNSPECIFIED; break;
		case 1: tsvals_.premultiply = EXTRASAMPLE_ASSOCALPHA; break;
		case 2: tsvals_.premultiply = EXTRASAMPLE_UNASSALPHA; break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data */
	  gimp_get_data ("file_tiff_save", &tsvals_);

	  parasite = gimp_image_parasite_find (orig_image_ID,
                                               "tiff-save-options");
	  if (parasite)
	    {
	      tsvals_.compression =
		((TiffSaveVals *) parasite->data)->compression;
	      tsvals_.premultiply =
		((TiffSaveVals *) parasite->data)->premultiply;
	    }
	  gimp_parasite_free (parasite);
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID,
                          orig_image_ID, info))
	    {
	      /*  Store mvals data */
	      gimp_set_data ("file_tiff_save", &tsvals_, sizeof (TiffSaveVals));
              values[0].data.d_status = GIMP_PDB_SUCCESS;
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

#else /* cinepaint */
  if (strcmp (name, "file_tiff_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = GIMP_PDB_SUCCESS;
	  values[1].type = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_tiff_save") == 0)
    {
      /*  Possibly retrieve data */
      gimp_get_data ("file_tiff_save", &tsvals_);

      m_free (image_artist)
      image_artist = g_get_real_name ();

      m_new (info, ImageInfo, 1, return)
      if (get_image_info (  param[1].data.d_int32, param[2].data.d_int32,
                            info)   != 1)
        {
          return;
        }

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Acquire information with a dialog */
	  if (save_dialog (info) != 1)
	    return;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there! */
	  if (nparams != 9)
	    status = GIMP_PDB_CALLING_ERROR;
	  if (status == GIMP_PDB_SUCCESS)
	    {
	      switch (param[5].data.d_int32)
		{
		case 0: tsvals_.compression = COMPRESSION_NONE;     break;
		case 1: tsvals_.compression = COMPRESSION_LZW;      break;
		case 2: tsvals_.compression = COMPRESSION_PACKBITS; break;
		case 3: tsvals_.compression = COMPRESSION_DEFLATE;  break;
		case 4: tsvals_.compression = COMPRESSION_JPEG;  break;
		case 5: tsvals_.compression = COMPRESSION_JP2000;  break;
		case 6: tsvals_.compression = COMPRESSION_ADOBE_DEFLATE;  break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	      switch (param[6].data.d_int32)
		{
		case 0: tsvals_.fillorder = FILLORDER_MSB2LSB; break;
		case 1: tsvals_.fillorder = FILLORDER_LSB2MSB; break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	      switch (param[7].data.d_int32)
		{
		case 0: tsvals_.premultiply = EXTRASAMPLE_UNSPECIFIED; break;
		case 1: tsvals_.premultiply = EXTRASAMPLE_ASSOCALPHA; break;
		case 2: tsvals_.premultiply = EXTRASAMPLE_UNASSALPHA; break;
		default: status = GIMP_PDB_CALLING_ERROR; break;
		}
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32,
                      param[2].data.d_int32, info) == 1)
	{
	  /*  Store mvals data */
	  gimp_set_data ("file_tiff_save", &tsvals_, sizeof (TiffSaveVals));
	  values[0].data.d_status = GIMP_PDB_SUCCESS;
	}
      else
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

      info = destroy_info (info);
    }
#endif /* cinepaint */

  m_free (image_artist)
  m_free (image_copyright)
  m_free (image_comment)
}

static void
tiff_warning(const char* module, const char* fmt, va_list ap)
{
#ifdef __unix__no
  vprintf( fmt, ap );
  printf( "      \n" );
#else
  g_logv ((gchar *)0/*G_LOG_DOMAIN*/, G_LOG_LEVEL_MESSAGE, fmt, ap);
#endif
}
  
static void
tiff_error(const char* module, const char* fmt, va_list ap)
{
#ifdef __unix__no
  vprintf( fmt, ap );
  printf( "       \n" );
#else
  g_logv ((gchar *)0/*G_LOG_DOMAIN*/, G_LOG_LEVEL_MESSAGE, fmt, ap);
#endif
}


char* 
ladeDatei ( const char *name, size_t *size )
{
  FILE *fp = 0;
  char* mem = 0;
  const char* filename = name;

  {
    fp = fopen(filename, "r");

    if (fp)
    {
      /* get size */
      fseek(fp,0L,SEEK_END);
      /* read file possibly partitial */
      if(!*size || *size > ftell(fp))
        *size = ftell (fp);
      rewind(fp);

      /* allocate memory */
      mem = (char*) calloc (*size, sizeof(char));

      /* check and read */
      if ((fp != 0)
       && mem
       && *size)
      {
        int s = fread(mem, sizeof(char), *size, fp);
        /* check again */
        if (s != *size)
        { *size = 0;
          if (mem) free (mem); mem = 0;
          mem = 0;
        }
      }
    } else {
      printf( "could not read %s\n", filename );
    }
  }

  /* clean up */
  if (fp) fclose (fp);

  return mem;
}

void* myAllocFunc (size_t size)
{ return malloc (size);
}


static gint32
load_image ( gchar *filename)
{
  TIFF    *tif;
  ImageInfo  *info = NULL,          /* top IFD info */
             *ref_info = NULL;      /* the selected reference IFD */
  gchar   *name;
  gint32 image_ID;
  long	  i, j;
  gint    layer=0;
  /* needed ?? -->  gimp_rgb_set (&color, 0.0, 0.0, 0.0); */
/* info->color[0] = '0'; info->color[1] = '0'; info->color[2] = '0'; */
  #if GIMP_MAJOR_VERSION >= 1
  GimpParasite *parasite;
  #endif

  /* Error handling */
  TIFFSetWarningHandler (tiff_warning);
  TIFFSetErrorHandler (tiff_error);

  name = g_strdup_printf( "%s '%s'...", _("Opening"), filename);
  gimp_progress_init (name);
  m_free (name)


  tif = TIFFOpen (filename, "rm");
  if (!tif) {
    g_message (_("Could not open '%s' for reading: %s"),
                 filename, g_strerror (errno));
    return FALSE;
  }

  m_new (info, ImageInfo, 1, return FALSE)

  if (!(get_tiff_info (tif, info)))
  {
    g_message ("%s:%d get_tiff_info(...) failed",__FILE__,__LINE__);
    TIFFClose (tif);
    return FALSE;
  }

  /* select a non reduced IFD to top */
  if( info->filetype & FILETYPE_REDUCEDIMAGE ) {
    int     reduced_found = 0;
#   ifdef DEBUG
    printf( "top IFD is FILETYPE_REDUCEDIMAGE\n select new IFD\n" );
#   endif
    info = info->top;	/* select the top of the infos of the first IFD */

    for ( i = 0 ; i < info->pagecount ; i++ ) { /* select the last IFD */
      if (!(info->filetype & FILETYPE_REDUCEDIMAGE) &&
          !reduced_found ) {
        ref_info = info;
        ref_info->im_t_sel = info->image_type;
        ref_info->bps_selected = info->bps;
        g_print ("%s:%d %s() non reduced IFD[%d] found; photomet:%d \n",
             __FILE__,__LINE__,__func__, (int)i, (int)info->photomet);
        reduced_found = 1;
      }
      if (i > 0)
        info = info->next;
    }
  }
  if (!ref_info) { /* no non reduced IFD found */
    ref_info = info->top;
#   ifdef DEBUG
    g_print ("%s:%d %s() non reduced IFD found; photomet:%d \n",
             __FILE__,__LINE__,__func__, (int)info->photomet);;
#   endif
  }

  ref_info->filename = g_strdup (filename);

  if ((image_ID = gimp_image_new ((guint)ref_info->image_width,
	 (guint)ref_info->image_height, ref_info->im_t_sel)) == -1) {
      g_message("TIFF Can't create a new image\n%dx%d %d",
	   ref_info->image_width, ref_info->image_height,ref_info->im_t_sel);
      TIFFClose (tif);
    return FALSE;
  }
#if GIMP_MAJOR_VERSION >= 1 /* gimp */
  if ( info->bps == 16 )
    g_message ("WARNING! This Image has %d bits per channel. GIMP "
		 "can only handle 8 bit, so it will be converted for you. "
		 "Information will be lost because of this conversion."
               ,info->bps);
  if ( info->logDataFMT == SGILOGDATAFMT_8BIT )
    g_message ("WARNING! This HDR image holds 16 bits per channel. GIMP "
		 "can only handle 8 bit LDR, so it will be converted for you. "
		 "Information will be lost because of this conversion."
               );
#else /* cinepaint */
  if (info->bps == 64)
    g_message (_("WARNING! This Image has %d bits per channel. CinePaint \n"
		 "can only handle 32 bit, so it will be converted for you.\n"
		 "Information will be lost because of this conversion.")
               ,info->bps);
#endif

  gimp_image_set_filename (image_ID, filename);


  /* The image is created. Now set informations and attach other data.
   * attach a parasite containing an ICC profile - if found in the TIFF file */

  #ifdef TIFFTAG_ICCPROFILE
  if (info->profile_size > 0) {
    gimp_image_set_icc_profile_by_mem (image_ID, (guint32)info->profile_size,
                                                 info->icc_profile,
                                                 ICC_IMAGE_PROFILE);
      #if GIMP_MAJOR_VERSION >= 1
      parasite = gimp_parasite_new("icc-profile", 0,
			      info->profile_size,
                              info->icc_profile);
      gimp_image_parasite_attach(image_ID, parasite);
      gimp_parasite_free(parasite);
      #endif
  } else {
#     ifdef DEBUG
      printf ("image %d has no profile embedded\n", image_ID);
#     endif
      /* comunicating color spaces from tiffsettings */
      switch (info->photomet)
      {
        case PHOTOMETRIC_LOGLUV:
          { /* creating an special profile with equal energy white point */
            cmsHPROFILE p = cmsCreateXYZProfile ();
            LPcmsCIEXYZ wtpt = calloc (sizeof (cmsCIEXYZ),1);
            size_t size = 0;
            char* buf;

            wtpt->X = 1.0; wtpt->Y = 1.0; wtpt->Z = 1.0;
            _cmsAddXYZTag (p, icSigMediaWhitePointTag, wtpt);

            _cmsSaveProfileToMem (p, NULL, &size);
            buf = (char*) calloc (sizeof(char), size);
            _cmsSaveProfileToMem (p, buf, &size);
            gimp_image_set_icc_profile_by_mem (image_ID,
                              (guint32)size, buf, ICC_IMAGE_PROFILE);

            cmsCloseProfile( p );
            free (buf);
#           ifdef DEBUG
            printf ("%s:%d set XYZ profile with DE\n",__FILE__,__LINE__);
#           endif
          }
          break;
        case PHOTOMETRIC_RGB:
          #if 0
          gimp_image_set_srgb_profile (image_ID);
          printf ("%s:%d set sRGB profile\n",__FILE__,__LINE__);
          #endif
          break;
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
          gimp_image_set_lab_profile (image_ID);
#         ifdef DEBUG
          printf ("%s:%d set Lab profile\n",__FILE__,__LINE__);
#         endif
          break;
        case PHOTOMETRIC_SEPARATED:
          {
            // set a default profile for cmyk
            char * profile_name = 0;
#ifdef OYRANOS_H
# if OYRANOS_API > 12

            profile_name = oyGetDefaultProfileName (oyASSUMED_CMYK, myAllocFunc);
# endif
#endif 
            {
              char* prof_mem = 0;
              size_t size = 0;
#ifdef OYRANOS_H
# if OYRANOS_API > 12
              if( !oyCheckProfile (profile_name, 0) )
                prof_mem = (char*)oyGetProfileBlock( profile_name, &size, myAllocFunc );
# endif
#endif 
              if( !prof_mem )
              {
                prof_mem = ladeDatei( "/usr/share/color/icc/ISOcoated.icc", &size );
              }

              if( prof_mem && size )
              { gimp_image_set_icc_profile_by_mem  (image_ID,
                                                    size,
                                                    prof_mem,
                                                    ICC_IMAGE_PROFILE);
                free ( prof_mem );
                size = 0;
              }
            }
          }
          break;
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
        /* TODO more specific GRAYSCALE handling */
          { size_t size = 0;
            char* buf;
            LPGAMMATABLE gamma = cmsBuildGamma(256, 1.0), g;
            cmsHPROFILE lp = 0;

            if (info->photomet == PHOTOMETRIC_MINISWHITE)
              g = cmsReverseGamma(256, gamma);
            else
              g = gamma;
            lp = cmsCreateGrayProfile(cmsD50_xyY(), g);
            _cmsSaveProfileToMem (lp, NULL, &size);
            buf = (char*) calloc (sizeof(char), size);
            _cmsSaveProfileToMem (lp, buf, &size);

            gimp_image_set_icc_profile_by_mem (image_ID,
                                               (guint32)size,
                                               buf, ICC_IMAGE_PROFILE);
            cmsCloseProfile( lp );
            cmsFreeGamma(gamma);
            free (buf);
#           ifdef DEBUG
            printf ("%s:%d set gray profile\n",__FILE__,__LINE__);
#           endif
          }
          break;
      }
  }
  #endif


  /* attach a parasite containing the compression */
  #if GIMP_MAJOR_VERSION >= 1
     parasite = gimp_parasite_new ("tiff-save-options", 0,
				sizeof (info->save_vals),
                                &info->save_vals);
     gimp_image_parasite_attach (image_ID, parasite);
     gimp_parasite_free (parasite);
  #endif /* GIMP_HAVE_PARASITES */


  /* Attach a parasite containing the image description.  Pretend to
   * be a gimp comment so other plugins will use this description as
   * an image comment where appropriate. */
  #if GIMP_MAJOR_VERSION >= 1
  {
    if (info->img_desc)
    {
      int len;

      len = strlen(info->img_desc) + 1;
      len = MIN(len, 241);
      info->img_desc[len-1] = '\000';

      parasite = gimp_parasite_new ("gimp-comment",
				    GIMP_PARASITE_PERSISTENT,
				    len, info->img_desc);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }
  }
  #endif /* GIMP_HAVE_PARASITES */

#ifdef GIMP_HAVE_RESOLUTION_INFO
  {
    /* any resolution info in the file? */
    /* now set the new image's resolution info */
    info->unit = GIMP_UNIT_PIXEL; /* invalid unit */

    if (info->res_unit) 
	{

	  switch (info->res_unit) 
	    {
	    case RESUNIT_NONE:
	      /* ImageMagick writes files with this silly resunit */
	      g_message ("TIFF warning: resolution units meaningless\n");
	      break;
		
	    case RESUNIT_INCH:
  	      info->unit = GIMP_UNIT_INCH;
	      break;
		
	    case RESUNIT_CENTIMETER:
	      info->xres *= (gfloat)2.54;
	      info->yres *= (gfloat)2.54;
	      info->unit = GIMP_UNIT_MM; /* as this is our default metric unit  */
	      break;
		
	    default:
	      g_message ("TIFF file error: unknown resolution unit type %d, "
			     "assuming dpi\n", info->res_unit);
	      break;
	    }
        } 
    else 
    { /* no res unit tag */
      /* old AppleScan software produces these */
      g_message ("TIFF warning: resolution specified without\n"
      "any units tag, assuming dpi\n");
    }
    if (info->res_unit != RESUNIT_NONE)
      {
         gimp_image_set_resolution (image_ID, info->xres,
                                              info->yres);
         if (info->res_unit != GIMP_UNIT_PIXEL)
           gimp_image_set_unit (image_ID, info->unit); 
      }
  }
#endif /* GIMP_HAVE_RESOLUTION_INFO */

  /* Here starts the directory loop.
   * We are starting with the last directory for the buttom layer 
   * and count the tiff-directories down to let the layerstack
   * in CinePaint grow up.
   */
  info = info->top;	/* select the top of the infos of the first IFD */

  for ( i = 0 ; i < info->pagecount ; i++ ) { /* select the last IFD */
    if (i > 0)
      info = info->next; 
  }

  for ( i = 0 ; i < info->pagecount ; i++ ) {
      /* We only accept pages with the same colordepth and photometric indent.
       * We could also test against (TIFFTAG_SUBFILETYPE, FILETYPE_PAGE)
       * but this would be insecure.
       * To test against page-numbers would be additional. */
      if (!TIFFSetDirectory( tif, (guint16) info->aktuelles_dir )) {
	g_message("IFD %d not found",(int)info->aktuelles_dir);
	TIFFClose (tif);
	gimp_quit();
      }
      if (info->filetype & FILETYPE_REDUCEDIMAGE) {/* skip */
        g_print ("%s:%d %s() IFD: %ld filetype: FILETYPE_REDUCEDIMAGE \n",
                  __FILE__,__LINE__,__func__, info->aktuelles_dir);
      }
      /* load the IFD after certain conditions, divergine IFDs are not converted*/
      if ( (ref_info->photomet == info->photomet)
        && (ref_info->bps == info->bps)
        && (&info->cols != NULL)
        && (&info->rows != NULL)
      #if 0
        && ((info->top->worst_case_nonreduced
                                   && (info->filetype & FILETYPE_REDUCEDIMAGE))
         || !(info->filetype & FILETYPE_REDUCEDIMAGE))
      #endif
         )
      {
        /*printf( print_info( info ) );*/
        /* Allocate ChannelData for all channels, even the background layer */
        m_new ( info->channel, ChannelData, 1 + info->extra, return -1)
        for (j=0; j <= info->extra ; j++) {
          info->channel[j].drawable = NULL;
          info->channel[j].pixels = NULL;
        }
        if (!load_IFD (tif, image_ID, layer, info))
        {
          g_message("load_IFD with IFD %d failed",
                        (int)info->aktuelles_dir);
          TIFFClose (tif);
          gimp_quit();
        }
      }
	else if ( strcmp (info->pagename, "viewport")
	       && (info->filetype & FILETYPE_MASK) )
      {
        g_message ("TIFF IFD %d not read\n", (int)info->aktuelles_dir);
      }
      if (info->aktuelles_dir > 0)
        info = info->parent;
  }
  TIFFClose (tif);

  /* TODO unclear mix of info->top and ref_info */
  gimp_image_resize ( image_ID, (guint)ref_info->image_width,
			     (guint)ref_info->image_height,
                            - info->top->viewport_offx,
	 		    - info->top->viewport_offy);

  info = destroy_info (info);
# ifdef DEBUG
  if (info)
    g_print ("%s:%d %s() error deleting info at %d \n",
             __FILE__,__LINE__,__func__, (size_t)info);;
# endif

  return image_ID;
}

static gint32
load_IFD 	(	TIFF		*tif,
			gint32		image_ID,
			gint32		layer,
			ImageInfo	*ci)
{
  gushort *redmap, *greenmap, *bluemap;
  guchar   cmap[768];

  gboolean flip_horizontal = FALSE;
  gboolean flip_vertical = FALSE;

  gint   i, j;

  ci->image_ID = image_ID;

  /* GimpParasite *parasite; */ /* not available in filmgimp */

  /* We need to repeat this after every directory change */
  if (ci->photomet == PHOTOMETRIC_LOGLUV
   || ci->photomet == PHOTOMETRIC_LOGL)
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, ci->logDataFMT);

    /* Install colormap for INDEXED images only */
    if (ci->image_type == INDEXED)
      {
        if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap)) 
          {
            g_message ("TIFF Can't get colormaps\n");
	    gimp_quit ();
	  }
	/* - let it here for maybe debugging */
/*  if (!TIFFGetField (tif, TIFFTAG_COLORMAP, &redcolormap,
                               &greencolormap, &bluecolormap))
              {
                g_print ("error getting colormaps\n");
                gimp_quit ();
              }
            ci->numcolors = ci->maxval + 1;
            ci->maxval = 255;
            ci->grayscale = 0;
 
            for (i = 0; i < ci->numcolors; i++)
              {
                redcolormap[i] >>= 8;
                greencolormap[i] >>= 8;
                bluecolormap[i] >>= 8;
              }
 
            if (ci->numcolors > 256) {
                ci->image_type = RGB;
              } else {
                ci->image_type = INDEXED;
              }*/
        for (i = 0, j = 0; i < (1 << ci->bps); i++) 
        {
          cmap[j++] = redmap[i] >> 8;
          cmap[j++] = greenmap[i] >> 8;
          cmap[j++] = bluemap[i] >> 8;
        }
          gimp_image_set_cmap (image_ID, cmap, (1 << ci->bps));
        }

    layer = gimp_layer_new      (image_ID, ci->pagename, 
                                (guint)ci->cols,
                                (guint)ci->rows,
                                (guint)ci->layer_type,
                                100.0, NORMAL_MODE);

    ci->channel[0].ID = layer;
    gimp_image_add_layer (image_ID, layer, 0);
    ci->channel[0].drawable = gimp_drawable_get (layer);

    if (ci->extra > 0 && !ci->worst_case) {
      /* Add alpha channels as appropriate */
      for (i= 1; i <= ci->extra; i++) {
        ci->channel[i].ID = gimp_channel_new(image_ID, _("TIFF Channel"),
                                             (guint)ci->cols, (guint)ci->rows,
                                             100.0,
                                             &ci->color[0]
                                             );
        gimp_image_add_channel(image_ID, ci->channel[i].ID, 0);
        ci->channel[i].drawable= gimp_drawable_get (ci->channel[i].ID);
      }
    }

    if (ci->worst_case) {
      load_rgba  (tif, ci);
    } else if (TIFFIsTiled(tif)) {
      load_tiles (tif, ci);
    } else { /* Load scanlines in tile_height chunks */
      load_lines (tif, ci);
    }

    if ( ci->offx > 0 || ci->offy > 0 ) { /* position the layer */ 
      gimp_layer_set_offsets( layer, ci->offx, ci->offy);
    }

  if (ci->pagecounted == 0
      && ci->top->pagecounting == 1) {
    gimp_layer_set_visible( layer, FALSE);
#   ifdef DEBUG
    g_print ("%d|%d layer:%d invisible\n",ci->pagecounted,ci->top->pagecounting, layer);
#   endif
  } else {
#   ifdef DEBUG
    g_print ("%d|%d layer:%d visible\n",ci->pagecounted,ci->top->pagecounting, layer);
#   endif
  }

  if (ci->orientation > -1)
    {
      switch (ci->orientation)
        {
        case ORIENTATION_TOPLEFT:
          flip_horizontal = FALSE;
          flip_vertical   = FALSE;
          break;
        case ORIENTATION_TOPRIGHT:
          flip_horizontal = TRUE;
          flip_vertical   = FALSE;
          break;
        case ORIENTATION_BOTRIGHT:
          flip_horizontal = TRUE;
          flip_vertical   = TRUE;
          break;
        case ORIENTATION_BOTLEFT:
          flip_horizontal = FALSE;
          flip_vertical   = TRUE;
          break;
        default:
          flip_horizontal = FALSE;
          flip_vertical   = FALSE;
          g_warning ("Orientation %d not handled yet!", ci->orientation);
          break;
        }

#if GIMP_MAJOR_VERSION >= 1 /* gimp TODO for cinepaint */
      if (flip_horizontal || flip_vertical)
        gimp_image_undo_disable (image_ID);

      if (flip_horizontal)
        gimp_flip (layer, GIMP_ORIENTATION_HORIZONTAL);

      if (flip_vertical)
        gimp_flip (layer, GIMP_ORIENTATION_VERTICAL);

      if (flip_horizontal || flip_vertical)
        gimp_image_undo_enable (image_ID);
#endif
    }

  for (i= 0; !ci->worst_case && i < (gint)ci->extra; ++i) {
    gimp_drawable_flush (ci->channel[i].drawable);
    gimp_drawable_detach (ci->channel[i].drawable);
  }
  return 1;
}

static void
load_rgba (TIFF *tif, ImageInfo	*ci)
{
  uint32 row;
  uint32 *buffer=NULL;

  gimp_pixel_rgn_init (&(ci->channel[0].pixel_rgn), ci->channel[0].drawable,0,0,
		 ci->cols, ci->rows,
		 TRUE, FALSE);
  m_new ( buffer, uint32, ci->cols * ci->rows, return)
  ci->channel[0].pixels = (guchar*) buffer;

  if (!TIFFReadRGBAImage(tif, ci->cols,
		 ci->rows, buffer, 0))
    g_message("TIFF Unsupported layout, no RGBA loader\n");

  for (row = 0; row < ci->rows; ++row) {
    gimp_pixel_rgn_set_rect(&(ci->channel[0].pixel_rgn),
                 ci->channel[0].pixels + row * ci->cols * 4, 0,
                 ci->rows -row -1,
		 ci->cols, 1);
    gimp_progress_update ((double) row / (double) ci->rows);
  }
}

static void
load_tiles (TIFF *tif,                /* the tiff data */ 
	    ImageInfo	 *ci)
{
  uint32 tileWidth=0, tileLength=0;
  uint32 x, y, rows, cols/*,  **tileoffsets */;
  guchar *buffer=NULL;
  double progress= 0.0, one_row;
  gushort i;

  TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
  TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);

  one_row = (double) tileLength / (double) ci->rows;
  m_new ( buffer, char, (size_t)TIFFTileSize(tif), return)

  for (i= 0; i <= ci->extra; ++i) {
    m_new (ci->channel[i].pixels, char, (size_t)
           (tileWidth * tileLength * ci->channel[i].drawable->bpp), return)
  }

  for (y = 0; y < (uint32)ci->rows; y += tileLength) {
    for (x = 0; x < (uint32)ci->cols; x += tileWidth) {
      gimp_progress_update (progress + one_row *
                            ( (double) x / (double) ci->cols));
      if (!TIFFReadTile(tif, buffer, x, y, 0, 0))
	g_message("TIFFReadTile failed");

      /* TODO set here an profile checking with conversion to color_type_ */

      cols= MIN(ci->cols - x, tileWidth);
      rows= MIN(ci->rows - y, tileLength);
      if          (ci->bps == 64) {
        read_64bit(buffer, ci->channel, ci->photomet,
		 (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols));
      } else if   (ci->bps == 32) {
        read_32bit(buffer, ci->channel, ci->photomet,
		 (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols), ci);
      } else if  ( ci->bps == 16
	 && (ci->sampleformat == SAMPLEFORMAT_IEEEFP
	  || ci->sampleformat == SAMPLEFORMAT_VOID) ) {
        read_f16bit(buffer, ci->channel, ci->photomet,
		  (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols));
      } else if  ( ci->bps == 16
	 && (ci->sampleformat == SAMPLEFORMAT_UINT) ) {
        read_u16bit(buffer, ci->channel, ci->photomet,
		  (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols));
      } else if   (ci->bps == 8) {
        read_8bit(buffer, ci->channel, ci->photomet,
		  (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols));
      } else {
        read_default(buffer, ci->channel, ci->bps, ci->photomet,
		  (gint)x, (gint)y, (gint)cols, (gint)rows, ci->alpha,
                  (gint)ci->extra, (gint)ci->assoc,
                  (gint)(tileWidth - cols));
      }
    }
    progress+= one_row;
  }
  for (i= 0; i <= ci->extra; ++i) {
    m_free (ci->channel[i].pixels)
  }

  m_free(buffer)
}

static void
load_lines( TIFF *tif,                /* the tiff data */ 
	    ImageInfo    *ci)
{
  gint32  lineSize,    /* needed for alocating memory */
          cols,        /* imageWidth in pixel */
          rows;        /* an variable for saying how many lines to load*/
         /* **stripoffsets ; */  /* determine the offset */
  guchar *buffer=NULL;
  gint32  g_row,        /* growing row */
          j_row,        /* jumping (tile-height) row */
          i, tile_height = (gint32)gimp_tile_height ();
  char   *text=NULL;
  
  cols = ci->cols;
    
  lineSize = TIFFScanlineSize(tif);
# ifdef DEBUG
  { char    dbg[64];
    snprintf(dbg,64, "lineSize %d",lineSize);
    DBG( dbg )
  }
# endif

  for (i= 0; i <= ci->extra; ++i) { 
    m_new (ci->channel[i].pixels, char,
           tile_height * cols * ci->channel[i].drawable->bpp, return)
  }

  m_new ( buffer, char, lineSize * tile_height, return)

  if (ci->planar == PLANARCONFIG_CONTIG) {
    if (ci->channel->drawable->bpp !=
                       (int)((float)ci->bps/8 * (float)ci->spp) && ci->bps < 64)
          g_warning ("%s:%d %s() drawables and tiffs byte depth are different %d != %01f(%01f*%01f)\n",__FILE__,__LINE__,__func__,
            ci->channel->drawable->bpp,
            (float)ci->bps/8.0* (float)ci->spp,
            (float)ci->bps/8.0, (float)ci->spp);

    for (j_row = 0; j_row < ci->rows; j_row+= tile_height ) { 
      /* jumping progress */
      gimp_progress_update ( (double) j_row / (double) ci->rows);
      /* is the remainder smaller? - then not tile_height  */
      rows = MIN(tile_height, ci->rows - j_row);
      /* walking along the rows within tile_height */
      for (g_row = 0; g_row < rows; ++g_row)
        /* buffer is only for one tile-height (* bpp * cols)  */
	if (!TIFFReadScanline(tif, buffer + g_row * lineSize,
                    (uint32)(j_row + g_row), 0))
	  g_message("Scanline %d not readable", (int)g_row);



      if        (ci->bps == 64) {
	read_64bit(buffer, ci->channel, ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0);
      } else if (ci->bps == 32) {
	read_32bit(buffer, ci->channel, ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0, ci);
      } else if (ci->bps == 16
	 && (ci->sampleformat == SAMPLEFORMAT_IEEEFP
	  || ci->sampleformat == SAMPLEFORMAT_VOID) ) {
        read_f16bit (buffer, ci->channel, ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0);
      } else if (ci->bps == 16) {
	read_u16bit (buffer, ci->channel, ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0);
      } else if (ci->bps == 8) {
	read_8bit   (buffer, ci->channel, ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0);
      } else {
	read_default(buffer, ci->channel, ci->bps,
		                      ci->photomet, 0, j_row,
		 cols, rows, ci->alpha, (gint)ci->extra,
                 (gint)ci->assoc, 0);
      }
    }
  } else { /* PLANARCONFIG_SEPARATE */
    uint16 sample;
    gint   start = TRUE;

    if (ci->channel->drawable->bpp !=
                       (int)((float)ci->bps/8 * (float)ci->spp) && ci->bps < 64)
          g_warning ("%s:%d %s() drawables and tiffs byte depth are different %d != %01f(%01f*%01f)\n",__FILE__,__LINE__,__func__,
            ci->channel->drawable->bpp,
            (float)ci->bps/8.0* (float)ci->spp,
            (float)ci->bps/8.0, (float)ci->spp);

    for (sample = 0; sample < (ci->spp + ci->extra); sample++) {
      m_new (text, char, 256, ;)
      sprintf (text, "%s '%s' plane %d ...",_("Opening"), ci->top->filename,
                      sample + 1);

      gimp_progress_init (text);

      m_free (text)

      for (j_row = 0; j_row < ci->rows; j_row += tile_height) {
        gimp_progress_update ( (double) j_row / (double) ci->rows);
        rows = MIN (tile_height, ci->rows - j_row);

        for (g_row = 0;  g_row < rows; ++g_row) {
          TIFFReadScanline(tif, buffer + g_row * lineSize,
                 (uint32)(j_row + g_row), sample);
        }

        read_separate (buffer, ci, 0, j_row, ci->cols, rows, (gint)sample);
        start = FALSE;
      }
    }
  }
  for (i= 0; i <= ci->extra; ++i) {
    m_free (ci->channel[i].pixels)
  }

  m_free(buffer)
}

static void
read_64bit (guchar       *source,   /*/ the libtiff buffer */
	    ChannelData  *channel,  /* struct including drawables and buffers */
	    gushort       photomet, /* how to interprete colors */
	    gint32        startcol, /* where to start? */
	    gint32        startrow,
	    gint32        cols,     /* how many cols? */
	    gint32        rows,     /* how many rows? */
	    gint          alpha,    /* transparency available? */
            gint          extra,    /* samples to store in an extra channel */
            gint          assoc,    /* shall alpha divided? */
            gint          align)
{
  guchar  *destination;
  gdouble  red64_val, green64_val, blue64_val, gray64_val, alpha64_val,
          *s_64 = (gdouble*)source; /* source don't changes any longer here */
  gfloat  *d_32;  /* 32bit variant of dest(ination) */
  gint32   col,   /* colums */
           g_row, /* growing row */
           i;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (g_row = 0; g_row < rows; ++g_row) { /* the rows to process */
    destination = channel[0].pixels + g_row * cols * channel[0].drawable->bpp;
    d_32 = (gfloat*)destination;

    for (i= 1; i <= extra; ++i) { /* don't leaf any extra-sample out */
      channel[i].pixel= channel[i].pixels + g_row * cols;
    }

    for (col = 0; col < cols; col++) { /* walking through all cols */
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray64_val= *s_64++ ;  /* assuming first is graysample */
            alpha64_val= *s_64++ ;  /* next is alphasample */
            if (alpha64_val) { /* detach associated alpha */
              gray64_val= MIN(gray64_val, alpha64_val); /* verify datas */
              *d_32++ = (gfloat)gray64_val  / (gfloat)alpha64_val ;
            } else /* there is no alpha: why pick up alpha16_val ?? */
              *d_32++ = 0.0;
              *d_32++ = alpha64_val;
          } else {
            *d_32++ = *s_64++ ;
          }
#if 0
                              for (k= 0; alpha + k < num_extra; ++k)
                              {
                                  NEXTSAMPLE;
                                  *channel[k].pixel++ = sample;
                              }
#endif
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray64_val= *s_64++ ; 
            alpha64_val= *s_64++ ; 
            if (alpha64_val) {
              gray64_val= MIN(gray64_val, alpha64_val);
              *d_32++ = ((gfloat)alpha64_val - (gfloat)gray64_val)  / (gfloat)alpha64_val ;
            } else 
              *d_32++ = 0.0;
              *d_32++ = alpha64_val;
          } else {
            *d_32++ = 1.0 - *s_64++ ; /* not tested */
          }
          break;

        case PHOTOMETRIC_PALETTE: 
          g_message("indexed images with 64bit ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_SEPARATED:
          g_message("separated images with 64bit ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_CIELAB:
          if (photomet == PHOTOMETRIC_CIELAB)  {
            /* for cinepaint we bring all values
             *    *L              0 - +100 -> 0 - 1
             *    *a and *b    -128 - +128 -> 0 - 1 */
            red64_val = *s_64++   / 100.0;
            green64_val = *s_64++ / 256.0;
            blue64_val = *s_64++  / 256.0; 
          } else {
            red64_val = *s_64++;
            green64_val = *s_64++;
            blue64_val = *s_64++; 
          }
          if (alpha) {
            alpha64_val = *s_64++;
            if (assoc == 1) {
              if (alpha64_val) {
              /* if (red64_val > alpha64_val) 
                red64_val = alpha64_val;
              if (green64_val > alpha64_val) 
                green64_val = alpha64_val;
              if (blue64_val > alpha64_val) 
                blue64_val = alpha64_val; */
                red64_val  = MIN(red64_val, alpha64_val);
                green64_val= MIN(green64_val, alpha64_val);
                blue64_val = MIN(blue64_val, alpha64_val);
                *d_32++ = red64_val    / alpha64_val ;
                *d_32++ = green64_val / alpha64_val ;
                *d_32++ = blue64_val / alpha64_val ;
              } else {
                *d_32++ = 0.0;
                if (photomet == PHOTOMETRIC_CIELAB
                 || photomet == PHOTOMETRIC_ICCLAB) {
                  *d_32++ = 0.5;
                  *d_32++ = 0.5;
                } else {
                  *d_32++ = 0.0;
                  *d_32++ = 0.0;
                }
              }
            } else {
              *d_32++ = red64_val;
              *d_32++ = green64_val;
              *d_32++ = blue64_val;
            }
            *d_32++ = alpha64_val;
          } else {
            *d_32++ = red64_val;
            *d_32++ = green64_val;
            *d_32++ = blue64_val;
          }
#if 0
                            for (k= 0; alpha + k < num_extra; ++k)
                            {
                              NEXTSAMPLE;
                              *channel[k].pixel++ = sample;
                            }
#endif
          break; 

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
          break; 
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source++;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra);
          break;
        case PHOTOMETRIC_RGB:
	case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
          source+= align * (3 + alpha + extra);
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

static void
read_32bit (guchar       *source,   /* the libtiff buffer */
	    ChannelData  *channel,  /* struct including drawables and buffers */
	    gushort       photomet, /* how to interprete colors */
	    gint32        startcol, /* where to start? */
	    gint32        startrow,
	    gint32        cols,     /* how many cols? */
	    gint32        rows,     /* how many rows? */
	    gint          alpha,    /* transparency available? */
            gint          extra,    /* samples to store in an extra channel */
            gint          assoc,    /* shall alpha divided? */
            gint          align,
            ImageInfo    *info)
{
  guchar *destination;
  gfloat  red32_val, green32_val, blue32_val, gray32_val, alpha32_val;
  gfloat *s_32 = (gfloat*)source; /* source don't changes any longer here */
  gfloat *d_32;  /* 32bit variant of dest(ination) */
  gint32  col,   /* colums */
          g_row, /* growing row */
          i;
  gfloat *c_32;   /* colour conversion variables */

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (g_row = 0; g_row < rows; ++g_row) { /* the rows to process */
    destination = channel[0].pixels + g_row * cols * channel[0].drawable->bpp;
    d_32 = (gfloat*)destination;

    for (i= 1; i <= extra; ++i) {   /* don't leaf any extra-sample out */
      channel[i].pixel= channel[i].pixels + g_row * cols;
    }

    for (col = 0; col < cols; col++) { /* walking through all cols */
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_LOGL:
          if (alpha) {
            gray32_val= *s_32++ ;   /* assuming first is graysample */
            alpha32_val= *s_32++ ;  /* next is alphasample */
            if (alpha32_val) {      /* detach associated alpha */
              gray32_val= MIN(gray32_val, alpha32_val); /* verify datas */
              *d_32++ = (gfloat)gray32_val  / (gfloat)alpha32_val ;
            } else /* there is no alpha: why pick up alpha16_val ?? */
              *d_32++ = 0.0;
              *d_32++ = alpha32_val;
          } else {
            *d_32++ = *s_32++ ;
          }
#if 0
                              for (k= 0; alpha + k < num_extra; ++k)
                              {
                                  NEXTSAMPLE;
                                  *channel[k].pixel++ = sample;
                              }
#endif
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray32_val= *s_32++ ; 
            alpha32_val= *s_32++ ; 
            if (alpha32_val) {
              gray32_val= MIN(gray32_val, alpha32_val);
              *d_32++ = ((gfloat)alpha32_val - (gfloat)gray32_val)  / (gfloat)alpha32_val ;
            } else 
              *d_32++ = 0.0;
              *d_32++ = alpha32_val;
          } else {
            *d_32++ = 1.0 - *s_32++ ; /* not tested */
          }
          break;

        case PHOTOMETRIC_PALETTE: 
          g_message("indexed images with 32bit ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_SEPARATED:
          g_message("separated images with 32bit ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_CIELAB:
          if (photomet == PHOTOMETRIC_CIELAB)  {
            /* for cinepaint we bring all values
             *    *L              0 - +100 -> 0.0 - 1.0
             *    *a and *b    -128 - +128 -> 0.0 - 1.0 */
            red32_val = *s_32++   / 100.0;
            green32_val = *s_32++ / 256.0 + .5;
            blue32_val =  *s_32++ / 256.0 + .5;
          } else {
            red32_val = *s_32++;
            green32_val = *s_32++;
            blue32_val = *s_32++;
          }
          if (alpha) {
            alpha32_val = *s_32++;
            if (assoc == 1) {
              if (alpha32_val) {
              /* if (red32_val > alpha32_val)
                red32_val = alpha32_val;
              if (green32_val > alpha32_val)
                green32_val = alpha32_val;
              if (blue32_val > alpha32_val)
                blue32_val = alpha32_val; */
                red32_val  = MIN(red32_val, alpha32_val);
                green32_val= MIN(green32_val, alpha32_val);
                blue32_val = MIN(blue32_val, alpha32_val);
                *d_32++ = red32_val    / alpha32_val ;
                *d_32++ = green32_val / alpha32_val ;
                *d_32++ = blue32_val / alpha32_val ;
              } else {
                *d_32++ = 0.0;
                if (photomet == PHOTOMETRIC_CIELAB
                 || photomet == PHOTOMETRIC_ICCLAB) {
                  *d_32++ = 0.5;
                  *d_32++ = 0.5;
                } else {
                  *d_32++ = 0.0;
                  *d_32++ = 0.0;
                }
              }
            } else {
              *d_32++ = red32_val;
              *d_32++ = green32_val;
              *d_32++ = blue32_val;
            }
            *d_32++ = alpha32_val;
          } else {
            *d_32++ = red32_val;
            *d_32++ = green32_val;
            *d_32++ = blue32_val;
          }
          if (photomet == PHOTOMETRIC_LOGLUV) {
            c_32 = d_32 - 3;
            if (alpha)
              c_32--;

            if (info->stonits != 0.0) {
            #if 0
              c_32[0] = c_32[0] * info->stonits;
              c_32[1] = c_32[1] * info->stonits;
              c_32[2] = c_32[2] * info->stonits;
            #endif
            }
            #if 0  /* reading into sRGB */
            if (photomet == PHOTOMETRIC_LOGLUV)  {
                float xyz[3];
                xyz[0] = c_32[0];
                xyz[1] = c_32[1];
                xyz[2] = c_32[2];
                fXYZtoColour (&xyz[0], c_32, 0.0);
            }
            #endif
          }
#if 0
                            for (k= 0; alpha + k < num_extra; ++k)
                            {
                              NEXTSAMPLE;
                              *channel[k].pixel++ = sample;
                            }
#endif
          break; 

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
          break; 
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source++;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra);
          break;
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
          source+= align * (3 + alpha + extra);
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

static void
read_f16bit (guchar       *source,
	    ChannelData *channel,
	    gushort       photomet,
	    gint          startcol, /* where to start? */
	    gint          startrow,
	    gint          cols,
	    gint          rows,
	    gint          alpha,
            gint          extra,
            gint          assoc,
            gint          align)
{
  guchar *destination;    /* pointer for setting target adress */
  guint16 *s_16 = (guint16*)source; /* source don't changes any longer here */
  guint16 *d_16;  /* 16bit variant of dest(ination) */
  gint    col, /* colums */
          g_row, /* growing row */
          i;


  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (g_row = 0; g_row < rows; ++g_row) { /* the rows to process */
    destination = channel[0].pixels + g_row * cols * channel[0].drawable->bpp;
    d_16 = (guint16*)destination;

    for (i= 1; i <= extra; ++i) { /* don't leaf any extra-sample out */
      channel[i].pixel= channel[i].pixels + g_row * cols;
    }

    for (col = 0; col < cols; col++) { /* walking through all cols */
      ShortsFloat u, u_a;		/* temporary */

      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            float g = FLT(*s_16++, u),
                  a = FLT(*s_16++, u);
            g = MIN(g, a);
            if (a) { 
              *d_16++ = FLT16(g / a, u);
            } else 
              *d_16++ = 0;
            *d_16++ = FLT16(a, u);
          } else {
            *d_16++ = *s_16++ ;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            float g = FLT(*s_16++, u),
                  a = 1. - FLT(*s_16++, u);
            g = MIN(g, a);
            if (a) {
              *d_16++ = FLT16(g / a, u);
            } else 
              *d_16++ = 0;
            *d_16++ = FLT16(a, u);
          } else {
            *d_16++ = FLT16(1. - FLT(*s_16++, u), u_a);
          }
          break;

        case PHOTOMETRIC_PALETTE:
          g_message("indexed images with 16bit ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_SEPARATED:
          g_message("separated images with 16bit float ignored\n");
          gimp_quit ();
          break;
  
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_RGB:
        {
          float a = 0,
                r = FLT(s_16[0], u),
                g = FLT(s_16[1], u),
                b = FLT(s_16[2], u);

          if (photomet == PHOTOMETRIC_CIELAB)  {
            /* for cinepaint we bring all values
             *    *L              0 - +100 -> 0.0 - 1.0
             *    *a and *b    -128 - +128 -> 0.0 - 1.0 */
            r = r / 100.0;
            g = g / 256.0 + .5;
            b = b / 256.0 + .5;
          }

          if (alpha) {
            a = FLT( s_16[3], u );
            if (assoc == 1) {
              if (a) {
                r = MIN(r, a);
                g = MIN(g, a);
                b = MIN(b, a);
                *d_16++ = FLT16(r / a, u);
                *d_16++ = FLT16(g / a, u);
                *d_16++ = FLT16(b / a, u);
              } else {
                *d_16++ = 0;
                if (photomet == PHOTOMETRIC_CIELAB
                 || photomet == PHOTOMETRIC_ICCLAB) {
                  *d_16++ = FLT16(0.5,u);
                  *d_16++ = FLT16(0.5,u);
                } else {
                  *d_16++ = 0;
                  *d_16++ = 0;
                }
              }
            } else {
              *d_16++ = FLT16(r,u);
              *d_16++ = FLT16(g,u);
              *d_16++ = FLT16(b,u);
            }
            *d_16++ = FLT16(a,u);
            s_16 += 4;
          } else {
            *d_16++ = FLT16(r,u);
            *d_16++ = FLT16(g,u);
            *d_16++ = FLT16(b,u);
            s_16 += 3;
          }
          
#if 0
                            for (k= 0; alpha + k < num_extra; ++k)
                            {
                              NEXTSAMPLE;
                              *channel[k].pixel++ = sample;
                            }
#endif
        }
          break; 

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source++;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra);
          break;
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
          source+= align * (3 + alpha + extra);
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

#if GIMP_MAJOR_VERSION >= 1 /* gimp */
static void
read_u16bit (guchar       *source,
             ChannelData *channel,
             gushort       photomet,
             gint          startcol,
             gint          startrow,
             gint          cols,
             gint          rows,
             gint          alpha,
             gint          extra,
             gint          assoc,
             gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray_val= *source; source+= 2;
            alpha_val= *source; source+= 2;
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = gray_val * 255 / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = *source; source+= 2;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray_val= *source; source+= 2;
            alpha_val= *source; source+= 2;
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ~(*source); source+= 2;
          }
          break;

        case PHOTOMETRIC_PALETTE:
          *dest++= *source; source+= 2;
          if (alpha) *dest++= *source; source+= 2;
          break;
  
#if 0
        case PHOTOMETRIC_CIELAB: 
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
#endif
        case PHOTOMETRIC_RGB:
          red_val= *source; source+= 2;
          green_val= *source; source+= 2;
          blue_val= *source; source+= 2;
          if (alpha) {
            alpha_val= *source; source+= 2;
            if (assoc == 1) {
              if (alpha_val) {
                red_val= MIN(red_val, alpha_val);
                green_val= MIN(green_val, alpha_val);
                blue_val= MIN(blue_val, alpha_val);
                *dest++ = (red_val * 255) / alpha_val;
                *dest++ = (green_val * 255) / alpha_val;
                *dest++ = (blue_val * 255) / alpha_val;
              } else {
                *dest++ = 0;
                *dest++ = 0;
                *dest++ = 0;
              }
	    } else {
              *dest++ = red_val;
              *dest++ = green_val;
              *dest++ = blue_val;
            } 
	    *dest++ = alpha_val;
	  } else {
	    *dest++ = red_val;
	    *dest++ = green_val;
	    *dest++ = blue_val;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source+= 2;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra) * 2;
          break;
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_SEPARATED:
          source+= align * (3 + alpha + extra) * 2;
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

#else

static void
read_u16bit (guchar       *source,
             ChannelData  *channel,
             gushort       photomet,
             gint          startcol, /* where to start? */
             gint          startrow,
             gint          cols,
             gint          rows,
             gint          alpha,
             gint          extra,
             gint          assoc,
             gint          align)
{
  guchar *destination;    /* pointer for setting target adress */
  guint16 red16_val, green16_val, blue16_val, gray16_val, alpha16_val;
  guint16 *s_16 = (guint16*)source; /* source don't changes any longer here */
  guint16 *d_16;  /* 16bit variant of dest(ination) */
  gint    col, /* colums */
          g_row, /* growing row */
          i;


  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (g_row = 0; g_row < rows; ++g_row) { /* the rows to process */
    destination = channel[0].pixels + g_row * cols * channel[0].drawable->bpp;
    d_16 = (guint16*)destination;

    for (i= 1; i <= extra; ++i) { /* don't leaf any extra-sample out */
      channel[i].pixel= channel[i].pixels + g_row * cols;
    }

    for (col = 0; col < cols; col++) { /* walking through all cols */
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray16_val= *s_16++ ; 
            alpha16_val= *s_16++ ;
            gray16_val= MIN(gray16_val, alpha16_val);
            if (alpha16_val) { 
              *d_16++ = ((guint32)gray16_val  * 65535) / (gfloat)alpha16_val + .5;
            } else 
              *d_16++ = 0;
              *d_16++ = alpha16_val;
          } else {
            *d_16++ = *s_16++ ;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray16_val= *s_16++ ; 
            alpha16_val= *s_16++ ; 
            gray16_val= MIN(gray16_val, alpha16_val);
            if (alpha16_val) {
              *d_16++ = ((guint32)gray16_val  * 65535) / (gfloat)alpha16_val + .5;
            } else 
              *d_16++ = 0;
              *d_16++ = alpha16_val;
          } else {
            *d_16++ = ~(*s_16++) ;
          }
          break;

        case PHOTOMETRIC_PALETTE:
          g_message("indexed images with 16bit ignored\n");
          gimp_quit ();
          break;
  
	case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_SEPARATED:
          red16_val = *s_16++;
          if (photomet == PHOTOMETRIC_CIELAB) {
            green16_val = *s_16++ - 32768;
            blue16_val = *s_16++ - 32768; 
          } else {
            green16_val = *s_16++;
            blue16_val = *s_16++; 
          }
          if (alpha) {
            alpha16_val = *s_16++;
            if (assoc == 1) {
              if (alpha16_val) {
                if (red16_val > alpha16_val) 
                  red16_val = alpha16_val;
                if (green16_val > alpha16_val) 
                  green16_val = alpha16_val;
                if (blue16_val > alpha16_val) 
                  blue16_val = alpha16_val;
                  *d_16++ = ((guint32)red16_val * 65535) / (gfloat)alpha16_val
                              + .5;
                  *d_16++ = ((guint32)green16_val * 65535) / (gfloat)alpha16_val
                              + .5;
                  *d_16++ = ((guint32)blue16_val * 65535) / (gfloat)alpha16_val
                              + .5;
              } else {
                *d_16++ = 0;
                if (photomet == PHOTOMETRIC_CIELAB
                 || photomet == PHOTOMETRIC_ICCLAB) {
                  *d_16++ = 32768;
                  *d_16++ = 32768;
                } else {
                  *d_16++ = 0;
                  *d_16++ = 0;
                }
              }
            } else {
              *d_16++ = red16_val;
              *d_16++ = green16_val;
              *d_16++ = blue16_val;
            } 
            *d_16++ = alpha16_val;
          } else {
            *d_16++ = red16_val;
            *d_16++ = green16_val;
            *d_16++ = blue16_val;
          }
#if 0
                            for (k= 0; alpha + k < num_extra; ++k)
                            {
                              NEXTSAMPLE;
                              *channel[k].pixel++ = sample;
                            }
#endif
          break; 

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source; source++;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra);
          break;
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
          source+= align * (3 + alpha + extra);
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}
#endif

static void
read_8bit (guchar       *source,
	   ChannelData *channel,
	   gushort       photomet,
	   gint32        startcol,
	   gint32        startrow,
	   gint32        cols,
	   gint32        rows,
	   gint          alpha,
	   gint          extra,
	   gint          assoc,
	   gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint32  col, row, i;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                          startcol, startrow, cols, rows, TRUE, FALSE);
  }

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          if (alpha) {
            gray_val= *source++;
            alpha_val= *source++;
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = gray_val * 255 / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = *source++;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          if (alpha) {
            gray_val= *source++;
            alpha_val= *source++;
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ~(*source++);
          }
          break;

        case PHOTOMETRIC_PALETTE:
          *dest++= *source++;
          if (alpha) *dest++= *source++;
          break;
  
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_SEPARATED:
          red_val= *source++;            /* unsigned integer */
          if (photomet == PHOTOMETRIC_CIELAB) {
            green_val= *source++ - 128;  /* signed integers */
            blue_val= *source++ - 128;   /*  */
          } else {
            green_val= *source++;        /* unsigned integers */
            blue_val= *source++;         /*  */
          }
          if (alpha) {
            alpha_val= *source++;        /* unsigned integer */
            if (assoc == 1) {
              red_val= MIN(red_val, alpha_val);
              blue_val= MIN(blue_val, alpha_val);
              green_val= MIN(green_val, alpha_val);
              if (alpha_val) {
                *dest++ = (gint)((gfloat)red_val * 255.0 / (gfloat)alpha_val);
                *dest++ = (gint)((gfloat)green_val *255.0 / (gfloat)alpha_val);
                *dest++ = (gint)((gfloat)blue_val * 255.0 / (gfloat)alpha_val);
              } else {
                *dest++ =   0;
                if (photomet == PHOTOMETRIC_CIELAB
                 || photomet == PHOTOMETRIC_ICCLAB) {
                  *dest++ = 128;
                  *dest++ = 128;
                } else {
                  *dest++ = 0;
                  *dest++ = 0;
                }
              }
            } else {
              *dest++ = red_val;
              *dest++ = green_val;
              *dest++ = blue_val;
            }
	    *dest++ = alpha_val;
	  } else {
            *dest++ = red_val;
            *dest++ = green_val;
            *dest++ = blue_val;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        *channel[i].pixel++ = *source++;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          source+= align * (1 + alpha + extra);
          break;
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_LOGLUV:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
          source+= align * (3 + alpha + extra);
          break;
      }
    }
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
	  source++;                           \
	  bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *source >> bitsleft ) & maxval; \
  }

static void
read_default (guchar       *source,
	      ChannelData  *channel,
	      gushort       bps,
	      gushort       photomet,
	      gint32        startcol,
	      gint32        startrow,
	      gint32        cols,
	      gint32        rows,
	      gint          alpha,
	      gint          extra,
              gint          assoc,
              gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint32  col, row, i;
  gint    bitsleft = 8, maxval = (1 << bps) - 1;

  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
		startcol, startrow, cols, rows, TRUE, FALSE);
  }

  for (row = 0; row < rows; ++row) {
    dest= channel[0].pixels + row * cols * channel[0].drawable->bpp;

    for (i= 1; i <= extra; ++i) {
      channel[i].pixel= channel[i].pixels + row * cols;
    }

    for (col = 0; col < cols; col++) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
          NEXTSAMPLE(gray_val);
          if (alpha) {
            NEXTSAMPLE(alpha_val);
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = (gray_val * 65025) / (alpha_val * maxval);
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = (gray_val * 255) / maxval;
          }
          break;

        case PHOTOMETRIC_MINISWHITE:
          NEXTSAMPLE(gray_val);
          if (alpha) {
            NEXTSAMPLE(alpha_val);
            gray_val= MIN(gray_val, alpha_val);
            if (alpha_val)
              *dest++ = ((maxval - gray_val) * 65025) / (alpha_val * maxval);
            else
              *dest++ = 0;
            *dest++ = alpha_val;
          } else {
            *dest++ = ((maxval - gray_val) * 255) / maxval;
          }
          break;

        case PHOTOMETRIC_PALETTE:
          NEXTSAMPLE(*dest++);
          if (alpha) {
            NEXTSAMPLE(*dest++);
          }
          break;
  
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
          NEXTSAMPLE(red_val)
          NEXTSAMPLE(green_val)
          NEXTSAMPLE(blue_val)
          if (alpha) {
            NEXTSAMPLE(alpha_val)
            red_val= MIN(red_val, alpha_val);
            blue_val= MIN(blue_val, alpha_val);
            green_val= MIN(green_val, alpha_val);
            if (alpha_val) {
              *dest++ = (red_val * 255) / alpha_val;
              *dest++ = (green_val * 255) / alpha_val;
              *dest++ = (blue_val * 255) / alpha_val;
            } else {
              *dest++ = 0;
              *dest++ = 0;
              *dest++ = 0;
	    }
	    *dest++ = alpha_val;
	  } else {
	    *dest++ = red_val;
	    *dest++ = green_val;
	    *dest++ = blue_val;
	  }
          break;

        default:
          /* This case was handled earlier */
          g_assert_not_reached();
      }
      for (i= 1; i <= extra; ++i) {
        NEXTSAMPLE(alpha_val);
        *channel[i].pixel++ = alpha_val;
      }
    }
    if (align) {
      switch (photomet) {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_PALETTE:
          for (i= 0; i < align * (1 + alpha + extra); ++i) {
            NEXTSAMPLE(alpha_val);
          }
          break;
        case PHOTOMETRIC_RGB:
        case PHOTOMETRIC_CIELAB:
        case PHOTOMETRIC_ICCLAB:
        case PHOTOMETRIC_ITULAB:
        case PHOTOMETRIC_SEPARATED:
          for (i= 0; i < align * (3 + alpha + extra); ++i) {
            NEXTSAMPLE(alpha_val);
          }
          break;
      }
    }
    bitsleft= 0;
  }
  for (i= 0; i <= extra; ++i) {
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                              startcol, startrow, cols, rows);
  }
}

static void
read_separate               (   guchar          *source,
                                ImageInfo       *ci,
                                gint32           startcol,
                                gint32           startrow,
                                gint32           cols,
                                gint32           rows,
                                gint             sample)
{
  guchar  *dest=NULL;
  gint32   col, row,
           c,                 /* channel */
           bps = ci->bps;
  gint     bitsleft = 8, maxval = (1 << bps) - 1;
  guint16 *s_16=NULL, *d_16=NULL;
  float   *s_32=NULL, *d_32=NULL;
  double  *s_64=NULL;
  gint     samples;    /* all samples  */

  if (sample < (gint)ci->channel[0].drawable->bpp) {
    c = 0;
  } else {
    c = (sample - ci->channel[0].drawable->bpp) + 4;
    ci->photomet = PHOTOMETRIC_MINISBLACK;
#   ifdef DEBUG
    g_print ("%s:%d %s() channel %d\n",__FILE__,__LINE__,__func__, c);
#   endif
  }

  samples = (int)(ci->channel[c].drawable->bpp / ((float)(bps == 64 ? 32 : bps) / 8.0));

  gimp_pixel_rgn_init (&(ci->channel[c].pixel_rgn), ci->channel[c].drawable,
                         startcol, startrow, cols, rows, TRUE, FALSE);

  gimp_pixel_rgn_get_rect(&(ci->channel[c].pixel_rgn), ci->channel[c].pixels,
                            startcol, startrow, cols, rows);

  /* set the buffers appropriate to each bit depth */
  for (row = 0; row < rows; row++) {
    if (bps <= 8) {         /* uint8 */
        dest = ci->channel[c].pixels +
                   (row * cols * ci->channel[c].drawable->bpp);
    } else if (bps == 16) { /* uint16 */
      d_16 = (guint16*) (ci->channel[c].pixels +
                   (row * cols * ci->channel[c].drawable->bpp));
      s_16 = (guint16*) source;
      s_16 += row * cols;
    } else if (bps == 32) { /* float */
      d_32 = (float*) (ci->channel[c].pixels +
                   (row * cols * ci->channel[c].drawable->bpp));
      s_32 = (float*) source;
      s_32 += row * cols;
    } else if (bps == 64) { /* double */
      d_32 = (float*) (ci->channel[c].pixels +
                   (row * cols * ci->channel[c].drawable->bpp));
      s_64 = (double*) source;
      s_64 += row * cols;
    }

    if (c == 0) {
      for (col = 0; col < cols; ++col) {
        if (bps <= 8) {         /* uint8 */
          NEXTSAMPLE(dest[col * ci->channel[0].drawable->bpp + sample]);
          dest[col * ci->channel[0].drawable->bpp + sample] =
             (dest[col * ci->channel[0].drawable->bpp + sample] * 255) / maxval;
        } else if (bps == 16) { /* uint16 */
          d_16 [col * samples + sample] = s_16[col];
        } else if (bps == 32) { /* float */
          d_32 [col * samples + sample] = s_32[col];
        } else if (bps == 64) { /* double */
          d_32 [col * samples + sample] = (float)( s_64[col]);
        }
      }
    } else {
      for (col = 0; col < cols; ++col)
        NEXTSAMPLE(dest[col]);
    }
    bitsleft= 0;
  }
  gimp_pixel_rgn_set_rect(&(ci->channel[c].pixel_rgn), ci->channel[c].pixels,
                            startcol, startrow, cols, rows);
}




/*
** pnmtotiff.c - converts a portable anymap to a Tagged Image File
**
** Derived by Jef Poskanzer from ras2tif.c, which is:
**
** Copyright (c) 1990 by Sun Microsystems, Inc.
**
** Author: Patrick J. Naughton
** naughton@wind.sun.com
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted,
** provided that the above copyright notice appear in all copies and that
** both that copyright notice and this permission notice appear in
** supporting documentation.
**
** This file is provided AS IS with no warranties of any kind.  The author
** shall have no liability with respect to the infringement of copyrights,
** trade secrets or any patents by this file or any part thereof.  In no
** event will the author be liable for any lost revenue or profits or
** other special, indirect and consequential damages.
*/

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID,
#if GIMP_MAJOR_VERSION >= 1
            gint32     orig_image_ID,
#endif
                                ImageInfo       *info)
{
  guint32  row, col, s_col, d_col;
  gfloat   xpos = (gfloat)0.0, ypos = (gfloat)0.0;  /* relative position */
  guint    width, height;	/* view sizes */
  long g3options;
  uint32 rowsperstrip;
  uint32 predictor;
  uint32 bytesperrow;
  guchar *t, *src=NULL, *data=NULL;
  int success = 0;
  GimpDrawable *drawable = 0;
  GimpImageType drawable_type;
  GimpPixelRgn pixel_rgn;
  int tile_height;
  int y, yend;
  char name_[256] = " ";
  char *name = &name_[0];
  gint nlayers;
  long dircount; 
  gint32 *layers;
 
  int lsample,                  /* local sample, for interwoven planes */
      sample,                   /* sample, for separated planes */
      planes;                   /* total number of planes to write */

  TIFF *tif;
 
  g3options = 0;
  predictor = 0;
  tile_height = gimp_tile_height ();
  rowsperstrip = 2;/*tile_height; */  DBG("")

  TIFFSetWarningHandler (tiff_warning);  DBG("")
  TIFFSetErrorHandler (tiff_error);

  sprintf (name, "%s '%s'...", _("Saving"), filename);  DBG("")
  name = g_strdup_printf( "%s '%s'...", _("Saving"), filename);  DBG("")
  gimp_progress_init (name);
   DBG("")

   DBG(print_info(info))

   DBG("")
  nlayers = -1;

  /* Die Zahl der Layer / number of layers */
  layers = gimp_image_get_layers (image_ID, &nlayers);

  if ( nlayers == -1 ) {
   g_message("no image data to save");
   gimp_quit ();
  }


#if 1
  tif = TIFFOpen (filename, "w");
#else /* speedup for windows ? */
  tif = TIFFOpen (filename, "wm");
#endif
  if (!tif)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 filename, g_strerror (errno));

      return FALSE;
    }

  info = info->top;

  for ( dircount=0 ; dircount < nlayers; dircount++) { /* from top to buttom like in dokuments*/
  /*for ( i=nlayers ; i >= 0; i--) {} *//* from buttom to top like in animations*/

      if (dircount > 0)
        info = info->next;

      if (info->aktuelles_dir != dircount) {
        g_print("%s:%d %s() error in counting layers",__FILE__,__LINE__,__func__);
        return FALSE;
      }

      TIFFCreateDirectory (tif);

      drawable_ID = layers [dircount];
      drawable = gimp_drawable_get (drawable_ID);
      drawable_type = gimp_drawable_type (drawable_ID);
      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, (int)drawable->width, (int)drawable->height, FALSE, FALSE);

      bytesperrow = info->cols * info->spp * (info->bps / 8);

      if (rowsperstrip == 0)
        rowsperstrip = (8 * 1024) / bytesperrow;
      if (rowsperstrip == 0)
        rowsperstrip = 1;

      if (info->save_vals.compression == COMPRESSION_DEFLATE) {
#ifdef TIFFTAG_ZIPQUALITY
        /*TIFFSetField (tif, TIFFTAG_ZIPQUALITY, 9); */
#endif
      } else if (info->save_vals.compression == COMPRESSION_JPEG) {
        rowsperstrip = info->rows;  /* for compatibility */
#ifdef TIFFTAG_JPEGPROC
        /*TIFFSetField (tif, TIFFTAG_JPEGPROC, JPEGPROC_LOSSLESS); */
#endif
      }
      if (info->planar == PLANARCONFIG_SEPARATE) {
        rowsperstrip = 1;
      }

      /* Set TIFF parameters. */
      TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, info->cols);
      TIFFSetField (tif, TIFFTAG_IMAGELENGTH, info->rows);
      TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, info->bps);
      if (info->sampleformat > 0)
        TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, info->sampleformat);
      TIFFSetField (tif, TIFFTAG_ORIENTATION, info->orientation);

      TIFFSetField (tif, TIFFTAG_COMPRESSION, info->save_vals.compression);
      if ((info->save_vals.compression == COMPRESSION_LZW) && (predictor != 0))
        TIFFSetField (tif, TIFFTAG_PREDICTOR, predictor);

      /* do we have an ICC profile? If so, write it to the TIFF file */
#if GIMP_MAJOR_VERSION >= 1
      {  DBG("")
  #ifdef TIFFTAG_ICCPROFILE
        GimpParasite *parasite;
        uint32 profile_size;
        guchar *icc_profile;

        parasite = gimp_image_parasite_find (orig_image_ID, "icc-profile");
        if (parasite)
          {
            profile_size = gimp_parasite_data_size(parasite);
            icc_profile = (guchar*) gimp_parasite_data (parasite);

            TIFFSetField(tif, TIFFTAG_ICCPROFILE, profile_size, icc_profile);
            gimp_parasite_free (parasite);
          }
  #endif
      }
#else
    #ifdef TIFFTAG_ICCPROFILE
   DBG("")
     if (info->profile_size > 0
       && dircount == 0
       && info->save_vals.use_icc_profile 
          /* LOGLuv compression and ICC other than XYZ -> ignore profile */
       && !((info->photomet == PHOTOMETRIC_LOGLUV)
           && !(strstr (&info->colorspace[0], "XYZ") != 0))) { 
        TIFFSetField(tif, TIFFTAG_ICCPROFILE, info->profile_size, info->icc_profile);
      }   DBG("")
   #endif
#endif


      if ( info->alpha
        && info->save_vals.premultiply != EXTRASAMPLE_UNSPECIFIED)
      {   DBG("")
        if (info->extra_types == NULL)
          m_new ( info->extra_types, unsigned short, 1, return FALSE)
        info->extra_types[0] = info->save_vals.premultiply;

        TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, info->alpha,info->extra_types);
      }
      TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, info->photomet);
      if (info->photomet == PHOTOMETRIC_SEPARATED && info->spp >= 4)
        TIFFSetField (tif, TIFFTAG_INKSET, INKSET_CMYK);
      TIFFSetField (tif, TIFFTAG_FILLORDER, info->save_vals.fillorder);

      if (info->photomet == PHOTOMETRIC_LOGLUV
       && info->stonits != 0.0 ) {
        TIFFSetField(tif, TIFFTAG_STONITS, info->stonits);
#       ifdef DEBUG
        g_print ("stonits: %f in candelas/meter^2 for Y = 1.0\n",
                 info->stonits);
#       endif
      }
      if ((info->save_vals.compression == COMPRESSION_PIXARLOG)) {
        TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, info->logDataFMT);
      } else if (info->save_vals.compression == COMPRESSION_SGILOG) {
        TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT);
      }

      TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, info->spp);
   /* TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, rows / rowsperstrip ); */
      TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
      TIFFSetField (tif, TIFFTAG_PLANARCONFIG, info->planar);
      if (gimp_drawable_type (drawable_ID) == INDEXED_IMAGE) {
        TIFFSetField (tif, TIFFTAG_COLORMAP, &info->red[0], &info->grn[0], &info->blu[0]);
      }
#if GIMP_MAJOR_VERSION >= 1 && GIMP_MINOR_VERSION >= 2
      /* resolution fields */ 
      if (info->xres > 1e-5 && info->yres > 1e-5)
      {
        TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, info->res_unit);
      }
#endif
      TIFFSetField (tif, TIFFTAG_XRESOLUTION, info->xres);
      TIFFSetField (tif, TIFFTAG_YRESOLUTION, info->yres);

      /* Handle offsets and resolution */ 
      if ( info->offx != info->top->max_offx ) {
        TIFFSetField (tif, TIFFTAG_XPOSITION, info->xpos);
      }
      if ( info->offy != info->top->max_offy ) {
        TIFFSetField (tif, TIFFTAG_YPOSITION, info->ypos);
      }
         DBG("")

      TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, /*g_basename (*/filename/*)*/);
      TIFFSetField (tif, TIFFTAG_PAGENAME, info->pagename );

      /* The TIFF spec explicitely says ASCII for the image description. */
      if ( image_comment != '\000' )
        {
          const gchar *c = image_comment;
          gint         len;

          for (len = strlen (c); len; c++, len--)
            {
              if ((guchar) *c > 127)
                {
                  g_message (_("The TIFF format only supports comments in\n"
                                "7bit ASCII encoding. No comment is saved."));

                  break;
                }
            }
        }   DBG("")

#if GIMP_MAJOR_VERSION >= 1
      /* do we have a comment?  If so, create a new parasite to hold it,
       * and attach it to the image. The attach function automatically
       * detaches a previous incarnation of the parasite. */
      if (image_comment && *image_comment != '\000')
        {
          GimpParasite *parasite;

          TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, image_comment);
          parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (image_comment), image_comment);
          gimp_image_parasite_attach (orig_image_ID, parasite);
          gimp_parasite_free (parasite);
        }
#else
      if (image_comment && *image_comment != '\000')
        TIFFSetField (tif, TIFFTAG_IMAGEDESCRIPTION, image_comment);
#endif /* GIMP_HAVE_PARASITES */

      if (image_copyright && *image_copyright != '\000'
          && strcmp(image_copyright,"(c)") != 0
          && strcmp(image_copyright,"") != 0)
        TIFFSetField (tif, TIFFTAG_COPYRIGHT, image_copyright);

   DBG("")
      TIFFSetField (tif, TIFFTAG_SOFTWARE, version()); DBG("")
      if (image_artist && *image_artist != '\000'
          && strlen (image_artist) > 1)
        TIFFSetField (tif, TIFFTAG_ARTIST, image_artist); DBG("")

      *name = '\000';
      if(g_getenv ("HOSTNAME"))
        sprintf( name, "%s",g_getenv ("HOSTNAME"));
      if(!strlen(name) && g_getenv ("HOST"))
        sprintf( name, "%s",g_getenv ("HOST"));
      if(strlen(name))
        TIFFSetField (tif, TIFFTAG_HOSTCOMPUTER, name); DBG("")

      {
	time_t	cutime;         /* Time since epoch */
	struct tm	*gmt;
	gchar time_str[24];

   DBG("")
	cutime = time(NULL); /* time right NOW */
	gmt = gmtime(&cutime);
	strftime(time_str, 24, "%Y/%m/%d %H:%M:%S", gmt);
	TIFFSetField (tif, TIFFTAG_DATETIME, time_str);
      }   DBG("")
      if ( info->pagecount > 1 ) { /* Tags for multipages */
        TIFFSetField (tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        if (info->visible)   /* write TIFFTAG_PAGENUMBER only for visible layers */
          TIFFSetField (tif, TIFFTAG_PAGENUMBER,
                      (unsigned short) info->aktuelles_dir, (unsigned short) info->pagecount);
      }

                    
      /* array to rearrange data */
      m_new ( src, guint8, (size_t)bytesperrow * tile_height, return FALSE)
      m_new ( data, guint8, (size_t)bytesperrow, return FALSE)

      if (info->planar == PLANARCONFIG_SEPARATE) {
        planes = (int)(info->spp);
      } else {
        planes = 1;
      }
#     ifdef DEBUG
      g_print ("%s:%d %s() write %d planes for %d samples, bytesperrow %d\n",__FILE__,__LINE__,__func__, planes, info->spp, (int)bytesperrow);
#     endif

      for (sample = 0 ; sample < planes ; sample++) {   DBG("")
        ShortsFloat u, u_a;		/* temporary */

        /* Now write the TIFF data as stripes. */
        for (y = 0; y < info->rows; y = yend) {
          yend = y + tile_height;
          yend = MIN (yend, info->rows);

          gimp_pixel_rgn_get_rect (&pixel_rgn, src, 0, y, info->cols, yend - y);

          for (row = y; row < yend; row++)
	    {
	      t = src + bytesperrow * (row - y);

	      switch (drawable_type)
	        {
	        case INDEXED_IMAGE:
	          success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case GRAY_IMAGE:
	          success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case GRAYA_IMAGE:
	          for (col = 0; col < info->cols*info->spp; col+=info->spp)
		    {
		      /* pre-multiply gray by alpha */
                      if ( info->save_vals.premultiply ==
                             EXTRASAMPLE_ASSOCALPHA ) {
 		        data[col + 0] = (t[col + 0] * t[col + 1]) / 255.0 + .5;
                      } else {
                        data[col + 0] = t[col + 0];
                      }
		      data[col + 1] = t[col + 1];  /* alpha channel */
		    }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
	        case RGB_IMAGE:
	        case RGBA_IMAGE:
		    {
		      guint8 * s = (guint8*)t;
		      guint8 * d = (guint8*)data;
      
		      for (col = 0; col < info->cols ; col++)
                        {
                          lsample = 0;
                          s_col = col * info->spp;
                          if (planes == 1) {
                            d_col = s_col;
                          } else {
                            d_col = col;
                          }

		          /* pre-multiply rgb by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA )
                          {
                            if (planes == 1 || sample == 0)
                              d[d_col + lsample] = s[s_col + 0] * s[s_col + 3]
                                                   / 255.0 + .5;
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 1)
                              d[d_col + lsample] = s[s_col + 1] * s[s_col + 3]
                                                   / 255.0 + .5;
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 2)
                              d[d_col + lsample] = s[s_col + 2] * s[s_col + 3]
                                                   / 255.0 + .5;
                          } else {
                            if (planes == 1 || sample == 0)
                              d[d_col + lsample] = s[s_col + 0];
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 1)
                              d[d_col + lsample] = s[s_col + 1];
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 2)
                              d[d_col + lsample] = s[s_col + 2];
                          }
                            if (planes == 1) lsample++;
                            if ((planes == 1 || sample == 3)
                             && info->spp >= 4)
		              d[d_col + lsample] = s[s_col + 3]; /* alpha */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, sample) >= 0);
                  /* g_print("    TIFFWriteScanline = %d\n", success); */
	          break;
#if GIMP_MAJOR_VERSION < 1
	        case U16_INDEXED_IMAGE:
	          break;
	        case U16_GRAY_IMAGE:
	          success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case U16_GRAYA_IMAGE:
		    { 
		      guint16 * s = (guint16*)t;
		      guint16 * d = (guint16*)data;
                      gint32 temp;
		      for (col = 0; col < info->cols*info->spp; col+=info->spp)
		        {
		          /* pre-multiply gray by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA ) {
		            d[col + 0] = INT_MULT_16 ((gint32)s[col + 0], s[col + 1], temp);
                          } else {
		            d[col + 0] = s[col + 0];
                          }
		          d[col + 1] = s[col + 1];  /* alpha channel */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
	        case U16_RGB_IMAGE:
	        case U16_RGBA_IMAGE:
		    {
		      guint16 * s = (guint16*)t;
		      guint16 * d = (guint16*)data;
		      guint32 temp;
      
		      for (col = 0; col < info->cols ; col++)
                        {
                          lsample = 0;
                          s_col = col * info->spp;
                          if (planes == 1) {
                            d_col = s_col;
                          } else {
                            d_col = col;
                          }

		          /* pre-multiply rgb by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA )
                          {
                            if (planes == 1 || sample == 0)
                              d[d_col + lsample] = INT_MULT_16 ((guint32)
                                           s[s_col + 0], s[s_col + 3], temp);
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 1)
                              d[d_col + lsample] = INT_MULT_16 ((guint32)
                                           s[s_col + 1], s[s_col + 3], temp);
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 2)
                              d[d_col + lsample] = INT_MULT_16 ((guint32)
                                           s[s_col + 2], s[s_col + 3], temp);
                          } else {
                            if (planes == 1 || sample == 0)
                              d[d_col + lsample] = s[s_col + 0];
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 1)
                              d[d_col + lsample] = s[s_col + 1];
                            if (planes == 1) lsample++;
                            if (planes == 1 || sample == 2)
                              d[d_col + lsample] = s[s_col + 2];
                          }
                            if (planes == 1) lsample++;
                            if ((planes == 1 || sample == 3)
                             && info->spp >= 4)
		              d[d_col + lsample] = s[s_col + 3]; /* alpha */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, sample) >= 0);
	          break;
	        case FLOAT16_GRAY_IMAGE:
	          success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case FLOAT16_GRAYA_IMAGE:
		    { 
		      guint16 *s = (guint16*)t;    /* source */
                      guint16 *d = (guint16*)data; /* destination */
		      gfloat tmp;
		      for (col = 0; col < info->cols*info->spp; col+=info->spp)
		        { /* TODO float16 */
		          /* pre-multiply gray by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA ) {
			    tmp = FLT( s[col + 0],u) * FLT( s[col + 1],u_a);
			    d[col + 0] = FLT16( tmp,u );
                          } else {
		            d[col + 0] = s[col + 0];
                          }
		          d[col + 1] = s[col + 1];  /* alpha channel */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
	        case FLOAT16_RGB_IMAGE:
                  success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case FLOAT16_RGBA_IMAGE:
		    {
		      guint16 *s = (guint16*)t;
                      guint16 *d = (guint16*)data;
		      gfloat tmp;
		      for (col = 0; col < info->cols*info->spp; col+=info->spp)
		        {
		          /* pre-multiply rgb by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA ) {
  			    tmp = FLT( s[col + 0],u) * FLT( s[col + 3],u_a);
 	  	  	    d[col + 0] = FLT16( tmp,u );
			    tmp = FLT( s[col + 1],u) * FLT( s[col + 3],u_a);
			    d[col + 1] = FLT16( tmp,u );
			    tmp = FLT( s[col + 2],u) * FLT( s[col + 3],u_a);
			    d[col + 2] = FLT16( tmp,u );
                          } else {
			    d[col + 0] = s[col + 0];
			    d[col + 1] = s[col + 1];
			    d[col + 2] = s[col + 2];
                          }
		          d[col+3] = s[col + 3];  /* alpha channel */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
	        case FLOAT_GRAY_IMAGE:
	          success = (TIFFWriteScanline (tif, t, row, 0) >= 0);
	          break;
	        case FLOAT_GRAYA_IMAGE:
		    {
		      gfloat *s = (gfloat*)t;
                      gfloat *d = (gfloat*)data;
		      for (col = 0; col < info->cols*info->spp; col+=info->spp)
		        {
		          /* pre-multiply gray by alpha */
                          if ( info->save_vals.premultiply ==
                                 EXTRASAMPLE_ASSOCALPHA ) {
		            d[col + 0] = s[col + 0] * s[col + 1];
                          } else {
		            d[col + 0] = s[col + 0];
                          }
		          d[col + 1] = s[col + 1];  /* alpha channel */
		        }
		    }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
	        case FLOAT_RGB_IMAGE:
	        case FLOAT_RGBA_IMAGE:
                {
	          gfloat *s = (gfloat*)t;
                  gfloat *d = (gfloat*)data;
                  float rgb[3];
		  for (col = 0; col < info->cols*info->spp; col+=info->spp)
		  {
                    if ( info->photomet == PHOTOMETRIC_LOGLUV
                      && info->save_vals.compression == COMPRESSION_SGILOG)
                    { /* assuming  sRGB for conversion to XYZ */
                      if ((strstr (&info->colorspace[0], "Rgb") != 0) 
                       || (strstr (&info->colorspace[0], "    ") != 0)) {
                        rgb[0] = s[col+0];
                        rgb[1] = s[col+1];
                        rgb[2] = s[col+2];
                        /* loosless matrix conversion */
                        fsRGBToXYZ (rgb , &s[col+0]);
                      }
                      if (info->stonits != 0.0) {
                        s[col+0] = s[col+0] / info->stonits;
                        s[col+1] = s[col+1] / info->stonits;
                        s[col+2] = s[col+2] / info->stonits;
                      }
                    }
                    /* pre-multiply rgb by alpha */
                    if (info->save_vals.premultiply == EXTRASAMPLE_ASSOCALPHA ){
                      /* for tifficc we bring all values 
                       *    *L           0 - 1 ->    0 - +100
                       *    *a and *b    0 - 1 -> -128 - +128 */
                      if ( info->photomet == PHOTOMETRIC_CIELAB ) {
                        d[col+0] = (s[col+0] *100.0) * s[col+3];
                        d[col+1] = ((s[col+1] -0.5) *256.0) * s[col+3];
                        d[col+2] = ((s[col+2] -0.5) *256.0) * s[col+3];
                      } else {
                        d[col+0] = s[col + 0] * s[col + 3];
                        d[col+1] = s[col + 1] * s[col + 3];
                        d[col+2] = s[col + 2] * s[col + 3];
                      }
                    } else {
                      if ( info->photomet == PHOTOMETRIC_CIELAB ) {
                        d[col+0] = s[col+0] * 100.0;
                        d[col+1] = (s[col+1] -0.5) *256.0;
                        d[col+2] = (s[col+2] -0.5) *256.0;
                      } else {
                        d[col+0] = s[col + 0];
                        d[col+1] = s[col + 1];
                        d[col+2] = s[col + 2];
                      } 
                    }
                    if (info->alpha)
                      d[col+3] = s[col + 3];  /* alpha channel */
                  }
	          success = (TIFFWriteScanline (tif, data, row, 0) >= 0);
	          break;
                }
#endif /* cinepaint */
	        default:
	          break;
	        }

	      if (success == 0)
	        {
	          g_print ("failed a scanline write on row %d\n", row);
	          return 0;
	        }
	    }

          gimp_progress_update ((double) row / (double) info->rows);
        }
      }

      TIFFWriteDirectory (tif); /* for multi-pages */

      TIFFFlushData (tif);
      m_free (src)
      m_free (data)
  
  }
  /* create dummy directory for viewport as background - Kai-Uwe_02-04/2003 */
  width = gimp_image_width(image_ID);
  height = gimp_image_height(image_ID);
  if (( info->top->max_offx != 0 )
   || ( info->top->max_offy != 0 )
   || ( info->cols != width )
   || ( info->rows != height ) ) {
    TIFFCreateDirectory (tif);
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 1);
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, filename);
    TIFFSetField (tif, TIFFTAG_PAGENAME, "viewport" );
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    /* Handle offsets and resolution */
    TIFFSetField (tif, TIFFTAG_XRESOLUTION, info->xres / (gfloat)width);
    TIFFSetField (tif, TIFFTAG_YRESOLUTION, info->yres / (gfloat)height);
    xpos = (gfloat)(- info->top->max_offx) / info->xres * (gfloat)width;
    ypos = (gfloat)(- info->top->max_offy) / info->yres * (gfloat)height;
    TIFFSetField (tif, TIFFTAG_XPOSITION, xpos);
    TIFFSetField (tif, TIFFTAG_YPOSITION, ypos);
    TIFFSetField (tif, TIFFTAG_SUBFILETYPE, FILETYPE_MASK);
    m_new (data, guchar, 2, return FALSE);
    info->cols = 1;
    for ( col=0; col < info->cols ; col++ ) {
      data[col] = 0;
    }
    info->rows = 1;
    for ( row=0 ; row < info->rows ; row++ ) {
      success = TIFFWriteScanline (tif, data, row, 0);
    }
    m_free (data)
    g_print ("The last layer in saved tiff is viewport area only.");
  }

  TIFFClose (tif);
  gimp_drawable_detach (drawable); drawable = NULL;
  m_free (name)

  return 1;
}


