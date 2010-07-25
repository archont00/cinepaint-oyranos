/*
 * load of Adobe PDF's;
 *  plug-in for cinepaint.
 *
 * Copyright (C) 2005-2008 Kai-Uwe Behrmann <ku.b@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * show a small settings dialog, with options for renderer( gs, xpdf ),
 * resolution, rgb/cmyk + default profile
 * open image in CinePaint
 *
 * Date: 15 December 2005
 */



#define PLUG_IN_NAME          "file_pdf_load"
#define PLUG_IN_BRIEF         "opens a PDF image."
#define PLUG_IN_DESCRIPTION   "Show settings befor opening a PDF image."
#define PLUG_IN_VERSION       "0.2 - 23 March 2006"
#define PLUG_IN_AUTHOR        "Kai-Uwe Behrmann <ku.b@gmx.de>"
#define PLUG_IN_COPYRIGHT     "Copyright 2005-2006 Kai-Uwe Behrmann"


#include <cstring>              // strlen strcmp strrchr
#include "pdf.h"
#include "pdf_dialog.h"

extern "C" {
#include <gtk/gtk.h>
#include "lib/plugin_main.h"
#include "lib/wire/libtile.h"
#include "plugin_pdb.h"
#include "libgimp/stdplugins-intl.h"

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
}

#define WARN_S(text) cout <<__FILE__<<":"<<__LINE__<<" "<< text << endl;

// Declare some local functions:

void   query		(void);
void   run		(char    	*name,
					 int      nparams,
					 GParam  *param,
					 int     *nreturn_vals,
					 GParam **return_vals);

int	load_dialog	(const char*     filename);
int	load_image	(const char*	 filename);
int erase_file  (const char*     filename);


GPlugInInfo	PLUG_IN_INFO =
{
    NULL,    /* init_proc */
    NULL,    /* quit_proc */
    query,   /* query_proc */
    run,     /* run_proc */
};


/***   file-global variables   ***/

static int n_return_vals_;
static int n_args_;
Vals vals = {0,"","",GHOSTSCRIPT,GHOSTSCRIPT,GS_TIFF,"",72,PDF_CMYK,1,1,0};

/*** Functions ***/

MAIN()				// defined in lib/plugin_main.h

char*
readFilePtrToMem_(FILE *fp, size_t *size)
{
  char* mem = 0;
  {
    if (fp)
    {
      /* allocate memory */
      mem = (char*) calloc (24, sizeof(char));

      /* check and read */
      if ((fp != 0)
       && mem
       && size)
      {
        size_t s = fread(mem, sizeof(char), 23, fp);
        /* check again */
        if (!s)
        {
          WARN_S("no file size" << s);
          *size = 0;
          if (mem) free (mem);
          mem = 0;
        } else *size = s;
      }
    } else {
      WARN_S( "no file provided" );
    }
  }

  return mem;
}

#define GS_VERSION_COMMAND "export PATH=$PATH:/opt/local/bin:/usr/local/bin; gs --version"

void
query ()
{
  static GParamDef args [] =
  {
    { PARAM_INT32,		"run_mode",		"Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef return_vals [] =
  {
    { PARAM_IMAGE, 	"image",		"Output Image" },
  };
  int dependency_error = 0;

  n_args_        = sizeof (args) / sizeof (args[0]);
  n_return_vals_ = sizeof (return_vals) / sizeof (return_vals[0]);

  #ifndef WIN32
  dependency_error = system(GS_VERSION_COMMAND);
  if(dependency_error > 0x200)
    WARN_S("The PDF interpreter Ghostscript is not installed.");
  #endif


  if(!dependency_error)
  {
    gimp_install_procedure (PLUG_IN_NAME,
                            PLUG_IN_BRIEF,
                            PLUG_IN_DESCRIPTION,
                            PLUG_IN_AUTHOR,
                            PLUG_IN_COPYRIGHT,
                            PLUG_IN_VERSION,
                            "<Load>/PDF",
                            NULL,
                            GIMP_PLUGIN,
                            n_args_, n_return_vals_,
                            args, return_vals);
    gimp_register_load_handler ("file_pdf_load", "pdf,eps,ps", "");
  }
}

void
run (char    *name,		// I - Name of called function resp. plug-in
     int      nparams,		// I - Number of parameters passed in
     GParam  *param,		// I - Parameter values
     int     *nreturn_vals,	// O - Number of return values
     GParam **return_vals)	// O - Return values
{
  GStatusType  status;		// return status,
  GRunModeType   run_mode;
  static GParam  values [2];	// return parameter
  int result = -1;
  gint image_ID = -1;


  status   = STATUS_SUCCESS;
  run_mode = (GRunModeType) param[0].data.d_int32;
  values[0].type 	        = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;


  *nreturn_vals = n_return_vals_;
  *return_vals  = values;

  if (strcmp (name, PLUG_IN_NAME) == 0)
  {
    gimp_get_data ("load_pdf", &vals);
    {
      FILE *fp = popen(GS_VERSION_COMMAND, "r");
      size_t size = 0;
      char *block = readFilePtrToMem_(fp, &size);
      if(block && size)
      {
        float v = 0;

#ifdef ENABLE_NLS
        setlocale(LC_NUMERIC, "C");
#endif
        v = atof(block);
        if(v > 8.4)
        {
          vals.gs_has_tiff32nc = 1;
          fprintf( stderr, "gs v%.02f has tiff32nc\n", v);
        } else
          fprintf( stderr, "disabling tiff32nc for gs v%.02f\n", v);
      } else WARN_S("popen failed with: " << GS_VERSION_COMMAND);
      if(fp) pclose(fp);
    }

    INIT_I18N_UI();
    INIT_FLTK1_CODESET();

    sprintf (vals.name, getenv("PWD"));

    snprintf(vals.filename, 1024, param[1].data.d_string);

    switch (run_mode)
    {
      case RUN_INTERACTIVE:
           result = load_dialog (param[1].data.d_string);
           #ifdef DEBUG
           printf("result: %d\n", result);
           #endif
           if (result < 0)
           { status = STATUS_EXECUTION_ERROR;
           }
           break;

      case RUN_NONINTERACTIVE:
           if (nparams != n_args_)
           { status = STATUS_CALLING_ERROR;
             break;
           }
           *nreturn_vals = n_return_vals_ + 1;
           break;

      case RUN_WITH_LAST_VALS:
           break;

      default:
           break;
    }
    if (status == STATUS_SUCCESS)
    {
           image_ID = load_image (param[1].data.d_string);
           if (image_ID != -1)
           {
             *nreturn_vals = 2;
             values[0].data.d_status = PARAM_IMAGE;
             values[1].type = PARAM_IMAGE;
             values[1].data.d_image = image_ID;
           }
           else
           { status = STATUS_EXECUTION_ERROR;
           }
    }
  }

  values[0].data.d_status = status;
  gimp_set_data ("load_pdf", &vals, sizeof(Vals));
}

#include <errno.h>
#include <sys/stat.h>
int
isFileFull (const char* fullFileName)
{
  struct stat status;
  int r = 0;
  const char* name = fullFileName;

  status.st_mode = 0;
  r = stat (name, &status);

  switch (r)
  {
    case EACCES:       WARN_S("EACCES = " << r); break;
    case EIO:          WARN_S("EIO = " << r); break;
    case ELOOP:        WARN_S("ELOOP = " << r); break;
    case ENAMETOOLONG: WARN_S("ENAMETOOLONG = " << r); break;
    case ENOENT:       WARN_S("ENOENT = " << r); break;
    case ENOTDIR:      WARN_S("ENOTDIR = " << r); break;
    case EOVERFLOW:    WARN_S("EOVERFLOW = " << r); break;
  }

  r = !r &&
       (   ((status.st_mode & S_IFMT) & S_IFREG)
        || ((status.st_mode & S_IFMT) & S_IFLNK));

  if (r)
  {
    FILE* fp = fopen (name, "r");
    if (!fp) {
      r = 0;
    } else {
      fclose (fp);
    }
  }

  return r;
}

#include <fstream>

char* 
ladeDatei ( std::string dateiname, size_t *size )
{
    char* data = 0;
    *size = 0;

    std::ifstream f ( dateiname.c_str(), std::ios::binary | std::ios::ate );

    if (dateiname == "" || !isFileFull(dateiname.c_str()) || !f)
    {
      if(dateiname == "") WARN_S( "no filename" );
      if(!isFileFull(dateiname.c_str())) WARN_S( "no file: " << dateiname );
      if(!f) WARN_S( "erroneous file: " << dateiname );
      dateiname = "";
      goto ERROR;
    }
  
    *size = (unsigned int)f.tellg();
    f.seekg(0);
    if(*size) {
      data = (char*)calloc (sizeof (char), *size+1);
      f.read ((char*)data, *size);
      f.close();
    } else {
      data = 0;
      WARN_S( "file size 0 for " << dateiname )
    }

  ERROR:

  return data;
}

void* myAllocFunc (size_t size)
{ DBG;
  return malloc (size);
}

int
load_image(const char* filename)
{
  int image_ID;
  GimpParam* return_vals;
  int n_retvals = 0;

    std::stringstream temp_file_name;
    if(getenv("TMPDIR"))
      temp_file_name << getenv("TMPDIR") << "/cinepaint_pdf" << time(0) ;
    else
      temp_file_name << "/tmp/cinepaint_pdf" << time(0) ;
    std::string ptn = temp_file_name.str();
    if(vals.colourspace == PDF_CMYK) ptn.append("_cmyk");
    else                            ptn.append("_rgb");
    switch(vals.export_format) {
          case GS_TIFF: ptn.append(".tif"); break;
          case GS_PSD: ptn.append(".psd"); break;
          case GS_PPM: ptn.append(".ppm"); vals.colourspace = PDF_RGB; break;
          case GS_PS: ptn.append(".ps"); break;
          case GS_PNG48: ptn.append(".png"); break;
    }

    snprintf( vals.name , 1024, ptn.c_str());

    DBG;

  char * profile_name = 0;

    // set a default profile for cmyk
#ifdef OYRANOS_H
# if OYRANOS_API > 12
    using namespace oyranos;

    if( vals.colourspace == PDF_CMYK )
      profile_name = oyGetDefaultProfileName (oyEDITING_CMYK, myAllocFunc);
    else
      profile_name = oyGetDefaultProfileName (oyASSUMED_RGB, myAllocFunc);
    if(!profile_name)
      WARN_S( "no profile found" )
    else
      WARN_S( "profile found " << profile_name )
# endif
#endif 

  {
    char* prof_mem = 0;
    size_t size = 0;
#ifdef OYRANOS_H
# if OYRANOS_API > 12
    if(	!oyCheckProfile (profile_name, 0) )
      prof_mem = (char*)oyGetProfileBlock( profile_name, &size, myAllocFunc );
# endif
#endif 

    std::stringstream ss;

    if( vals.interpreter == GHOSTSCRIPT )
    if( vals.colourspace == PDF_CMYK )
    {
#ifdef OYRANOS_H
      ss << "p=\"" << oyGetPathFromProfileName(profile_name, 0) << "/"
         << profile_name <<"\"; ";
      ss << "icc2ps -o \"$p\" -t 3 > " << vals.name << "_crd.ps; ";
      ss<<"echo \"/Current /ColorRendering findresource setcolorrendering\" >> "
         << vals.name << "_crd.ps; ";
#endif
    }

    int interpreter_ok = -1;

    if( vals.ok )
    {
      if (strlen(vals.command))
        ss << vals.command << " ";
      else if( vals.interpreter == GHOSTSCRIPT )
        ss << GS_COMMAND_BASE << " ";
      else
        ss << XPDF_COMMAND_BASE << " ";
      if( vals.interpreter == GHOSTSCRIPT )
      {
        switch(vals.export_format) {
          case GS_TIFF:
            if(vals.colourspace == PDF_CMYK) ss << "-sDEVICE=tiff32nc ";
            else                            ss << "-sDEVICE=tiff24nc ";
            break;
          case GS_PSD:
            if(vals.colourspace == PDF_CMYK) ss << "-sDEVICE=psdcmyk ";
            else                            ss << "-sDEVICE=psdrgb ";
            break;
          case GS_PPM:
            ss << "-sDEVICE=ppmraw ";
            break;
          case GS_PS:
            if(vals.colourspace == PDF_CMYK) ss << "-sDEVICE=pswrite ";
            else                            ss << "-sDEVICE=psrgb ";
            break;
          case GS_PNG48:
            ss << "-sDEVICE=png48 ";
            break;
        }
        if(vals.colourspace == PDF_CMYK)
          ss << "-sProcessColorModel=DeviceCMYK ";
        else
          ss << "-dUseCIEColor ";
        if(vals.resolution)
          ss << "-r" << vals.resolution << "x" << vals.resolution << " ";
        if( vals.aa_graphic == GS_AA4 )
          ss << "-dGraphicsAlphaBits=4 ";
        else if( vals.aa_graphic == GS_AA4INTERPOL)
          ss << "-dGraphicsAlphaBits=4 -dDOINTERPOLATE ";
        if( vals.aa_text )
          ss << "-dTextAlphaBits=4 -dAlignToPixels=1 -dNOPLATFONTS ";
        ss << "-sOutputFile=" << vals.name << " ";
#ifdef OYRANOS_H
        if( vals.colourspace == PDF_CMYK )
          ss << "-f " << vals.name << "_crd.ps ";
#endif
        ss << "'" << filename << "'";

        {
          std::string str = ss.str();
          const char* c = str.c_str();
          fputs (c, stderr);
          fputs ("  \n", stderr);
          interpreter_ok = system (c);
        }

      } else { // TODO XPDF
        if(vals.colourspace == PDF_CMYK)
          ss << " ";
        else
          ss << " ";
        if(vals.resolution)
          ss << " ";
        if( vals.aa_graphic )
          ss << " ";
        if( vals.aa_text )
          ss << " ";
        ss << " " << filename << " " << vals.name;

        fputs (ss.str().c_str(), stderr);
        fputs ("  \n", stderr);
        interpreter_ok = system (ss.str().c_str());
      }
      DBG;
      #ifdef DEBUG
      cout << ss.str() << endl;
      #endif
    } else
    {
      #ifdef DEBUG
      cout << "vals.ok " << vals.ok << endl;
      #endif
    }


    DBG;
    if(interpreter_ok == 0) {
    return_vals = gimp_run_procedure ("gimp_file_load",
                                    &n_retvals,
                                    GIMP_PDB_INT32, GIMP_RUN_INTERACTIVE,
                                    GIMP_PDB_STRING, vals.name,
                                    GIMP_PDB_STRING, vals.name,
                                    GIMP_PDB_END);
    } else
      fputs ("Interpreter failed.\n", stderr);
    if (n_retvals) {
      if (return_vals[0].data.d_status != GIMP_PDB_SUCCESS)
        image_ID = -1;
      else
        image_ID = return_vals[1].data.d_image;
      gimp_destroy_params (return_vals, n_retvals);
    }

    erase_file( vals.name );
    ss.str("");
#ifdef OYRANOS_H
    ss << vals.name << "_crd.ps";
    erase_file( ss.str().c_str() );
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

    gimp_image_set_filename( image_ID, (char*)filename );
  }

  return image_ID;
}

void
view_doc ()
{
  std::stringstream ss;
  char tmp[1024], file[1024], dir[1024];
  snprintf(tmp, 1023, vals.filename);
  snprintf(file, 1023, strrchr(tmp, '/'));
  char *ptr = strrchr(tmp, '/');
  const char* viewer = NULL;
  *ptr = 0;
  snprintf(dir, 1023, tmp);
  WARN_S( dir << file << tmp );

  // change the path to the picture
  ss << "(cd " << dir << "; ";

  // select the viewer
    switch(vals.viewer) {
          case GHOSTSCRIPT: viewer = "gv "; break;
          case XPDF: viewer = "xpdf "; break;
          case ACROREAD: viewer = "acroread "; break;
    }
  ss << viewer;
  // and execute it with the image name only, release the shell
  ss << &file[1] << " )&";
  // nu
  int interpreter_ok = system (ss.str().c_str());
  if(interpreter_ok != 0)
    fprintf(stderr, "could not start pdf viewer %s\n", viewer);
}

int
load_dialog (const char* filename)
{
	gint32		 result = 0;

    vals.ok = 0;
    if(!strlen(vals.command)) {
      sprintf(vals.command, GS_COMMAND_BASE);
      vals.interpreter = 0;
    }

    DBG;
    make_window();
    result = Fl::run();
    DBG;

    #ifdef DEBUG
    printf("result: %d ok: %d\n", result, vals.ok);
    printf("ok: %d res: %d\n",vals.ok, vals.resolution);
    #endif
    if( !result )
      return vals.ok ? 0 : -1;
    else
      return result;
}

int
erase_file (const char *file)
{
  FILE *fp;

  fp = fopen (file, "r");
  if (fp) {
    fclose (fp);
    remove (file);
    return 0;
  }
  cout << "Could not erase file: " << file << endl;
  return 1;
}


