/*
 * "$Id: print.c,v 1.16 2006/12/15 22:20:22 beku Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   compiles for cinepaint
 *   added cms support with lcms (actually an separate dialog)
 *   supports 8/16-bit integers and 32-bit floats
 *    Kai-Uwe Behrman (web@tiscali.de) --  February - October 2003
 *   direct printing of allready separated images
 *    Kai-Uwe Behrman (web@tiscali.de) --  January 2004
 *   updated to CinePaints internal CMS - v0.4.1-alpha
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  August 2004
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
//#include "../../lib/libprintut.h"

#include "print_gimp.h"

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "print-intl.h"


#include "lcms.h"
#include "print.h"

#include "print-lcms-funcs.h"
#include "print-lcms-options.h"

/*
 * Local functions...
 */

static void	query (void);
static void	run (char *, int, GimpParam *, int *, GimpParam **);
static int	do_lcms_dialog (void);
static int	do_print_dialog (char *proc_name, gint32 image_ID);


/*
 * Work around GIMP library not being const-safe.  This is a very ugly
 * hack, but the excessive warnings generated can mask more serious
 * problems.
 */

#define BAD_CONST_CHAR char *

/*
 * Globals...
 */

GimpPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static stpui_plist_t gimp_vars;

extern LcmsVals vals;

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static /* const */ GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "run_mode",	(BAD_CONST_CHAR) "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	(BAD_CONST_CHAR) "image",	(BAD_CONST_CHAR) "Input image" },
    { GIMP_PDB_DRAWABLE,	(BAD_CONST_CHAR) "drawable",	(BAD_CONST_CHAR) "Input drawable" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "output_to",	(BAD_CONST_CHAR) "Print command or filename (| to pipe to command)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "driver",	(BAD_CONST_CHAR) "Printer driver short name" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ppd_file",	(BAD_CONST_CHAR) "PPD file" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "output_type",	(BAD_CONST_CHAR) "Output type (0 = gray, 1 = color)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "resolution",	(BAD_CONST_CHAR) "Resolution (\"300\", \"720\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_size",	(BAD_CONST_CHAR) "Media size (\"Letter\", \"A4\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_type",	(BAD_CONST_CHAR) "Media type (\"Plain\", \"Glossy\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_source",	(BAD_CONST_CHAR) "Media source (\"Tray1\", \"Manual\", etc.)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "brightness",	(BAD_CONST_CHAR) "Brightness (0-400%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "scaling",	(BAD_CONST_CHAR) "Output scaling (0-100%, -PPI)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "orientation",	(BAD_CONST_CHAR) "Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "left",	(BAD_CONST_CHAR) "Left offset (points, -1 = centered)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "top",		(BAD_CONST_CHAR) "Top offset (points, -1 = centered)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "gamma",	(BAD_CONST_CHAR) "Output gamma (0.1 - 3.0)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "contrast",	(BAD_CONST_CHAR) "Contrast" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "cyan",	(BAD_CONST_CHAR) "Cyan level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "magenta",	(BAD_CONST_CHAR) "Magenta level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "yellow",	(BAD_CONST_CHAR) "Yellow level" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "linear",	(BAD_CONST_CHAR) "Linear output (0 = normal, 1 = linear)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "image_type",	(BAD_CONST_CHAR) "Image type (0 = line art, 1 = solid tones, 2 = continuous tone, 3 = monochrome)"},
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "saturation",	(BAD_CONST_CHAR) "Saturation (0-1000%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "density",	(BAD_CONST_CHAR) "Density (0-200%)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ink_type",	(BAD_CONST_CHAR) "Type of ink or cartridge" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "dither_algorithm", (BAD_CONST_CHAR) "Dither algorithm" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "unit",	(BAD_CONST_CHAR) "Unit 0=Inches 1=Metric" },
  };
  static gint nargs = sizeof(args) / sizeof(args[0]);

  static const gchar *blurb = "This plug-in prints images from cinepaint.";
  static const gchar *help  = "Prints images to PostScript, PCL, or ESC/P2 printers.";
  static const gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static const gchar *copy  = "Copyright 1997-2000 by Michael Sweet and Robert Krawitz, 2003-2006 Kai-Uwe Behrmann";
  static const gchar *types = "RGB*,U16_RGB*,FLOAT_RGB*";

  sprintf (&version_[strlen(version_)] , " (gutenprint %d.%d.%d)",STP_MAJOR_VERSION,
                              STP_MINOR_VERSION,STP_MICRO_VERSION); 

  gimp_plugin_domain_register ((BAD_CONST_CHAR) "gutenprint", (BAD_CONST_CHAR) "/usr/local/share/locale");

  gimp_install_procedure ((BAD_CONST_CHAR) "file_print_gimp",
			  (BAD_CONST_CHAR) blurb,
			  (BAD_CONST_CHAR) help,
			  (BAD_CONST_CHAR) auth,
			  (BAD_CONST_CHAR) copy,
			  (BAD_CONST_CHAR) &version_[0],
			  (BAD_CONST_CHAR) N_("<Image>/File/Print..."),
			  (BAD_CONST_CHAR) types,
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
  _("Print...");
}

static guchar *gimp_thumbnail_data = NULL;

static guchar *
stpui_get_thumbnail_data_function(void *image_ID, gint *width, gint *height,
				  gint *bpp, gint page)
{ guchar *temp = 0;
  int i = 0, x, y;

  if (gimp_thumbnail_data)
    free(gimp_thumbnail_data);
  gimp_thumbnail_data = 
           gimp_image_get_thumbnail_data((intptr_t)image_ID, width,height, bpp);

  DBG_S( ("bpp %d %dx%d", *bpp, *width, *height) )
  return gimp_thumbnail_data;
}

/*
 * 'run()' - Run the plug-in...
 */

volatile int SDEBUG = 1;

static void
run (char   *name,		/* I - Name of print program. */
     int    nparams,		/* I - Number of parameters passed in */
     GimpParam *param,		/* I - Parameter values */
     int    *nreturn_vals,	/* O - Number of return values */
     GimpParam **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable=NULL;	/* Drawable for image */
  GimpRunModeType	 run_mode;	/* Current run mode */
  GimpParam	*values;	/* Return values */
  GimpExportReturnType export = GIMP_EXPORT_CANCEL; /* return value of gimp_export_image() */
  GDrawableType drawable_type;
  gdouble xres, yres;
  stpui_image_t *image;
  gint32 base_type;

  INIT_I18N_UI();
  gimp_ui_init("print", TRUE);

#ifdef LCMS_VERSION
  // Error handling
  cmsSetErrorHandler ((cmsErrorHandlerFunction) lcms_error);
  cmsErrorAction (LCMS_ERROR_SHOW); //LCMS_ERROR_ABORT LCMS_ERROR_IGNORE
#endif

  //  Possibly retrieve data
  gimp_get_data ("cms-print", &vals);
  start_vals ();

  /*
   * Initialize parameter data...
   */

  run_mode = (GimpRunModeType)param[0].data.d_int32;

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  vals.o_image_ID = vals.e_image_ID = vals.s_image_ID = param[1].data.d_int32;
  vals.o_drawable_ID = vals.e_drawable_ID = vals.s_drawable_ID = param[2].data.d_int32;

  {
        char *text, *optr, *filename;

        optr = text = g_new (char,MAX_PATH);
        filename = gimp_image_get_filename (vals.o_image_ID);
        sprintf(text,"%s", filename);
        if (strchr(text, G_DIR_SEPARATOR))
          text = strrchr(text, G_DIR_SEPARATOR) + 1;

        sprintf (vals.image_filename,"%s",text);
        sprintf (vals.tiff_file,"%s",text);
        //g_print ("%s:%d vals.image_filename %s|%s\n",__FILE__,__LINE__,vals.image_filename,vals.tiff_file);
        g_free (optr);
        g_free (filename);
  }

  /*
   * detect image type
   */

  drawable_type = gimp_drawable_type(vals.s_drawable_ID);

  //g_print ("gimp_drawable_type %d %d\n",drawable_type, vals.s_drawable_ID);

  switch (drawable_type)
    {
    case GRAY_IMAGE:
    case GRAYA_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:
    case U16_GRAY_IMAGE:
    case U16_GRAYA_IMAGE:
    case U16_INDEXED_IMAGE:
    case U16_INDEXEDA_IMAGE:
    case FLOAT_GRAY_IMAGE:
    case FLOAT_GRAYA_IMAGE:
    case FLOAT16_GRAY_IMAGE:
    case FLOAT16_GRAYA_IMAGE:
    case FLOAT16_RGB_IMAGE:
    case FLOAT16_RGBA_IMAGE:
      g_message("drawable type %d is currently not supported.", drawable_type);
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;
      break;
    case RGB_IMAGE:
    case RGBA_IMAGE:
    case U16_RGB_IMAGE:
    case U16_RGBA_IMAGE:   // we use CMYK stored in an u16 RGBA drawable
    case FLOAT_RGB_IMAGE:
    case FLOAT_RGBA_IMAGE:
      break;
    }

  //g_print ("%s:%d %s() imag/drawable_ID:%d/%d type:%d\n",__FILE__,__LINE__,__func__,vals.s_image_ID,vals.s_drawable_ID, drawable_type);

  if (gimp_image_has_icc_profile (vals.o_image_ID, ICC_IMAGE_PROFILE))
  {
    char *csp = gimp_image_get_icc_profile_color_space_name (vals.o_image_ID,
                                                             ICC_IMAGE_PROFILE);
    vals.display_ID = gimp_display_active();
    vals.intent = vals.disp_intent =
                                gimp_display_get_cms_intent( vals.display_ID,
                                                             ICC_IMAGE_PROFILE);
    vals.flags = vals.disp_flags = gimp_display_get_cms_flags( vals.display_ID);

    DBG_S( ("%d: intent: %d flags: %d", vals.display_ID, vals.intent,
            vals.flags) )
  }

  if (!name) {
    char n = (char) "print";
    name = &n;
  }

  // gimp_ui_init
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (name, TRUE);
      break;
    default:
      break;
    }

  if (getenv("STP_DEBUG_STARTUP"))
    while (SDEBUG)
      ;

  // do_lcms_dialog
  if ( !do_lcms_dialog() ) {
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    return;
  }

  if (vals.quit_app) {
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    return;
  }



  // gimp_export_image ?
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      {
        int alpha_capable = FALSE;
        //g_print ("%s:%d %s() drawable %d of type %d with %d bpp, colour space %d.\n",__FILE__,__LINE__,__func__, vals.s_drawable_ID, drawable_type,gimp_drawable_bpp(vals.s_drawable_ID),vals.color_space);
        if (vals.icc == FALSE
         && vals.color_space == PT_CMYK
         && ((gimp_drawable_bpp(vals.s_drawable_ID) <= 4
           && gimp_drawable_type (vals.s_drawable_ID) != U16_GRAY ))) {
          alpha_capable = GIMP_EXPORT_CAN_HANDLE_ALPHA;
        }
        if ((vals.color_space == PT_CMYK && !vals.icc)
         || linear.use_lin) {
          alpha_capable = GIMP_EXPORT_CAN_HANDLE_ALPHA;
        }
        if (gimp_image_has_icc_profile (vals.o_image_ID, ICC_IMAGE_PROFILE))
        {
          char *csp = gimp_image_get_icc_profile_color_space_name  (
                                                             vals.o_image_ID,
                                                             ICC_IMAGE_PROFILE);
          if (strstr( csp, "Cmyk" ) != 0) {
            vals.color_space = PT_CMYK;
            alpha_capable = GIMP_EXPORT_CAN_HANDLE_ALPHA;
            DBG_S (("is CMYK\n"));
          }
        }
        //g_print ("alpha_capable: %d\n",alpha_capable);
        export = gimp_export_image (&vals.s_image_ID, &vals.s_drawable_ID, "Print",
                                    (GIMP_EXPORT_CAN_HANDLE_RGB |
                                     GIMP_EXPORT_CAN_HANDLE_GRAY |
                                     GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                     alpha_capable));
        if (export == GIMP_EXPORT_CANCEL)
        {
	  *nreturn_vals = 1;
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	  return;
	} else if (export == GIMP_EXPORT_EXPORT) {
          vals.e_image_ID = vals.s_image_ID;
          vals.e_drawable_ID = vals.s_drawable_ID;
          drawable_type = gimp_drawable_type(vals.e_drawable_ID);
        }
      }
      break;
    default:
      break;
    }

  // convert_image   ...  do all colour conversions and report status
  if (vals.icc || linear.use_lin) {
    values[0].data.d_status = create_separated_image ();
  } else {
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    vals.s_image_ID = vals.e_image_ID;
    vals.s_drawable_ID = vals.e_drawable_ID;
  }

  // p_gimp_save_tiff
  if (vals.save_tiff == TRUE) {
    char *profile=NULL;

    if (vals.icc) {
      if(gimp_image_has_icc_profile (vals.o_image_ID, ICC_PROOF_PROFILE))
      {
        char *description = gimp_image_get_icc_profile_description( vals.o_image_ID,
                                                             ICC_PROOF_PROFILE);
        if(description)
        {
          strcpy (vals.o_profile, description);
          free( description );
        }
      }
      profile = vals.o_profile;
    } else {
      profile = malloc (MAX_PATH);
      if (vals.color_space == PT_CMYK) 
        sprintf (profile, "cmyk");
      else if (vals.color_space == PT_Lab)
        sprintf (profile, "lab");
      else
        sprintf (profile, "rgb");
    }
  
    p_gimp_save_tiff(vals.s_image_ID, vals.s_drawable_ID, 7, profile);
    values[0].data.d_status = GIMP_PDB_SUCCESS;

    if (!vals.icc)
      free (profile);
  } else {

    if (values[0].data.d_status == GIMP_PDB_SUCCESS) {
      drawable = gimp_drawable_get ( vals.s_drawable_ID );
    } else {
      drawable = NULL;
      return;
    }

    base_type = gimp_image_base_type(vals.s_image_ID);
    switch (base_type)
      {
      case GIMP_INDEXED:
      case GIMP_RGB:
      case U16_INDEXED:
      case U16_RGB:
      case FLOAT16_RGB:
      case FLOAT_RGB:
        sprintf (vals.color_type, "RGB");
        break;
      case GIMP_GRAY:
      case U16_GRAY:
      case FLOAT16_GRAY:
      case FLOAT_GRAY:
        sprintf (vals.color_type, "Whitescale");
        break;
      default:
          g_message ("%s:%d %s() should not been reached\n",
                        __FILE__,__LINE__,__func__);
        break;
      }

    if (vals.color_space == PT_CMYK) {
      //if (drawable->bpp > 4 ) {
        sprintf (vals.color_type , "CMYK");
      //}
    }

    if (  vals.color_space == PT_CMYK
       && drawable->bpp < 8 ) {
      g_message ("Your CMYK image has not 16-bit per channel as is needed by gutenprint at the moment.\nHint: You can switch manually to \"16-bit Unsigned Integer\" in the \"Image\" menu of CinePaint."
                  );
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      goto cleanup;
    }
     
    if (  vals.color_space == PT_CMYK
       && !gimp_drawable_color(vals.s_drawable_ID)
       && !gimp_drawable_has_alpha(vals.s_drawable_ID)) {
      g_message ("%s:%d %s() drawable %d is impossible an CMYK image.\n",
                __FILE__,__LINE__,__func__,vals.s_drawable_ID);
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      goto cleanup;
    }
     

    // initialise libgimpprint

    stp_init();

#if GTK_MAJOR_VERSION > 1
    stp_set_output_codeset("UTF-8");
    INIT_I18N_UI();
    gimp_ui_init("print", TRUE);
#endif

    stpui_printer_initialize(&gimp_vars);

    stpui_set_image_filename(vals.image_filename);

    stpui_set_image_dimensions(drawable->width, drawable->height);
    //gimp_image_get_resolution (vals.s_image_ID, &xres, &yres);
    xres = 180.0; yres = 180.0;
    stpui_set_image_resolution(xres, yres);

    DBG_S( ("%d: %dx%d mit %dbyte\n",drawable->id,
             drawable->width, drawable->height, drawable->bpp) )

    drawable_type = gimp_drawable_type(vals.s_drawable_ID);
    switch (drawable_type)
      {
      case GRAY_IMAGE:
      case GRAYA_IMAGE:
      case INDEXED_IMAGE:
      case INDEXEDA_IMAGE:
      case RGB_IMAGE:
      case RGBA_IMAGE:
        vals.bitpersample = 8;
        break;
      case U16_GRAY_IMAGE:
      case U16_GRAYA_IMAGE:
      case U16_INDEXED_IMAGE:
      case U16_INDEXEDA_IMAGE:
      case U16_RGB_IMAGE:
      case U16_RGBA_IMAGE:   // we use CMYK stored in an u16 RGBA drawable
        vals.bitpersample = 16;
        break;
      case FLOAT_GRAY_IMAGE:
      case FLOAT_GRAYA_IMAGE:
      case FLOAT16_GRAY_IMAGE:
      case FLOAT16_GRAYA_IMAGE:
      case FLOAT16_RGB_IMAGE:
      case FLOAT16_RGBA_IMAGE:
      case FLOAT_RGB_IMAGE:
      case FLOAT_RGBA_IMAGE:
        vals.bitpersample = 32;
        break;
      }
    if(vals.bitpersample == 8 &&
       vals.color_space == PT_CMYK)
      stpui_set_image_channel_depth (16);
    else
      stpui_set_image_channel_depth (vals.bitpersample);

    stpui_set_image_type (vals.color_type);

    DBG_S( ("%d-bit %s type\n",vals.bitpersample,vals.color_type) )

    image = Image_GimpDrawable_new(drawable, (int)vals.s_image_ID);
    stp_set_float_parameter(gimp_vars.v, "AppGamma", gimp_gamma());
  
    if (!image)
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;


    switch (drawable_type)
      {
      case GRAY_IMAGE:
      case GRAYA_IMAGE:
      case U16_GRAY_IMAGE:
      case U16_GRAYA_IMAGE:
      case FLOAT_GRAY_IMAGE:
      case FLOAT_GRAYA_IMAGE:
      case FLOAT16_GRAY_IMAGE:
      case FLOAT16_GRAYA_IMAGE:
        stp_set_string_parameter(gimp_vars.v, "PrintingMode", "BW");
        break;
      case INDEXED_IMAGE:
      case INDEXEDA_IMAGE:
      case RGB_IMAGE:
      case RGBA_IMAGE:
      case U16_INDEXED_IMAGE:
      case U16_INDEXEDA_IMAGE:
      case U16_RGB_IMAGE:
      case U16_RGBA_IMAGE:   // we use CMYK stored in an u16 RGBA drawable
      case FLOAT16_RGB_IMAGE:
      case FLOAT16_RGBA_IMAGE:
      case FLOAT_RGB_IMAGE:
      case FLOAT_RGBA_IMAGE:
        stp_set_string_parameter(gimp_vars.v, "PrintingMode", "Color");
        break;
      }

    if (stp_verify(gimp_vars.v))
      g_message ("error");

    switch (run_mode)
      {
      case GIMP_RUN_INTERACTIVE:

        // --- do_print_dialog ---
        if (!do_print_dialog (name, vals.s_image_ID))
  	goto cleanup;

        stpui_plist_copy(&gimp_vars, stpui_get_current_printer());
        break;

      case GIMP_RUN_NONINTERACTIVE:
        /*
         * Make sure all the arguments are present...
         */
        if (nparams < 11)
  	values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        else
  	{
#if 0
          /* What do we do with old output_to?  Probably best ignore it. */
          stpui_plist_set_output_to(&gimp_vars, param[3].data.d_string);
#endif
  	  stp_set_driver(gimp_vars.v, param[4].data.d_string);
  	  stp_set_file_parameter(gimp_vars.v, "PPDFile", param[5].data.d_string);
	  switch (param[6].data.d_int32)
	    {
	    case 0:
	    default:
	      stp_set_string_parameter(gimp_vars.v, "PrintingMode", "BW");
	    case 1:
	      stp_set_string_parameter(gimp_vars.v, "PrintingMode", "Color");
	    }
  	  stp_set_string_parameter(gimp_vars.v, "Resolution", param[7].data.d_string);
  	  stp_set_string_parameter(gimp_vars.v, "PageSize", param[8].data.d_string);
  	  stp_set_string_parameter(gimp_vars.v, "MediaType", param[9].data.d_string);
  	  stp_set_string_parameter(gimp_vars.v, "InputSlot", param[10].data.d_string);

            if (nparams > 11)
  	    stp_set_float_parameter(gimp_vars.v, "Brightness", param[11].data.d_float);

            if (nparams > 12)
  	    gimp_vars.scaling = param[12].data.d_float;

            if (nparams > 13)
  	    gimp_vars.orientation = param[13].data.d_int32;

            if (nparams > 14)
              stp_set_left(gimp_vars.v, param[14].data.d_int32);

            if (nparams > 15)
              stp_set_top(gimp_vars.v, param[15].data.d_int32);

            if (nparams > 16)
              stp_set_float_parameter(gimp_vars.v, "Gamma", param[16].data.d_float);

            if (nparams > 17)
  	    stp_set_float_parameter(gimp_vars.v, "Contrast", param[17].data.d_float);

            if (nparams > 18)
  	    stp_set_float_parameter(gimp_vars.v, "Cyan", param[18].data.d_float);

            if (nparams > 19)
  	    stp_set_float_parameter(gimp_vars.v, "Magenta", param[19].data.d_float);

            if (nparams > 20)
  	    stp_set_float_parameter(gimp_vars.v, "Yellow", param[20].data.d_float);

            if (nparams > 21)
  	    stp_set_string_parameter(gimp_vars.v, "ImageOptimization", param[21].data.d_string);

            if (nparams > 22)
              stp_set_float_parameter(gimp_vars.v, "Saturation", param[23].data.d_float);

            if (nparams > 23)
              stp_set_float_parameter(gimp_vars.v, "Density", param[24].data.d_float);

  	  if (nparams > 24)
  	    stp_set_string_parameter(gimp_vars.v, "InkType", param[25].data.d_string);

  	  if (nparams > 25)
  	    stp_set_string_parameter(gimp_vars.v, "DitherAlgorithm",
  			      param[26].data.d_string);

            if (nparams > 26)
  	    gimp_vars.unit = param[27].data.d_int32;
  	}

        break;

      case GIMP_RUN_WITH_LAST_VALS:
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        break;

      default:
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        break;
      }

    if (gimp_thumbnail_data)
      free(gimp_thumbnail_data);

    /*
     * Print the image...
     */
    if (values[0].data.d_status == GIMP_PDB_SUCCESS)
      {
        /*
         * Set the tile cache size...
         */

        if (drawable->height > drawable->width)
  	gimp_tile_cache_ntiles ((drawable->height + gimp_tile_width () - 1) /
  				gimp_tile_width () + 1);
        else
  	gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
  				gimp_tile_width () + 1);

        if (! stpui_print(&gimp_vars, image))
  	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;


      }

    //stp_vars_free(gimp_vars.v);
  }

  if (values[0].data.d_status == GIMP_PDB_SUCCESS) {
        if (run_mode == GIMP_RUN_INTERACTIVE)
  	//gimp_set_data (PLUG_IN_NAME, vars, sizeof (vars));
          gimp_set_data ("cms-print", &vals, sizeof (LcmsVals));
  }

 cleanup:
  if (vals.icc == TRUE && !vals.show_image) {
    gimp_drawable_detach (drawable);
    gimp_image_delete (vals.s_image_ID);
    gimp_image_clean_all (vals.s_image_ID);
  }

  if (export == GIMP_EXPORT_EXPORT) {
    gimp_image_delete (vals.e_image_ID);
    gimp_image_clean_all (vals.e_image_ID);
  }
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static void
gimp_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static void
gimp_errfunc(void *file, const char *buf, size_t bytes)
{
  char formatbuf[32];
  snprintf(formatbuf, 31, "%%%ds", bytes);
  g_message(formatbuf, buf);
}

static gint
do_print_dialog (gchar *proc_name, gint32 image_ID)
{
 /*
  * Generate the filename for the current user...
  */
  char *filename = (gchar*) gimp_personal_rc_file ((BAD_CONST_CHAR) "printrc");

  //g_print ("%s:%d %s() \n%s\n",__FILE__,__LINE__,__func__, proc_name);
  stpui_set_printrc_file(filename);
  g_free(filename);
  if (! getenv("STP_PRINT_MESSAGES_TO_STDERR"))
    stpui_set_errfunc(gimp_errfunc);
  stpui_set_thumbnail_func(stpui_get_thumbnail_data_function);
  stpui_set_thumbnail_data((void *) vals.s_image_ID);
  //textdomain("gutenprint");
  return stpui_do_print_dialog();
}


static gint
do_lcms_dialog (void)
{
  GtkWidget *lcms_options;

  //textdomain("cinepaint-std-plugins");
  lcms_options = create_lcms_options_window ();
  init_lcms_options ();

  gtk_widget_show (lcms_options);

  gtk_main ();
  gdk_flush ();

  return lcms_run;
}



