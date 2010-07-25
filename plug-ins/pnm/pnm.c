/*
   pnm.c - is a pnm/pfm converter for CinePaint

   Copyright (c) 2005-2007 Kai-Uwe Behrmann

   Author: Kai-Uwe Behrmann <ku.b at gmx.de>

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation.

   This file is provided AS IS with no warranties of any kind.  The author
   shall have no liability with respect to the infringement of copyrights,
   trade secrets or any patents by this file or any part thereof.  In no
   event will the author be liable for any lost revenue or profits or
   other special, indirect and consequential damages.

   * initial version - ku.b 06.11.2005 : v0.1 
   * bug fix - ku.b 08.11.2005
   * add saving - ku.b 30.3.2006 : v0.2 
   * support alpha images, pam spec: http://netpbm.sourceforge.net/doc/pam.html
   * 8-bit save bugfix - ku.b 07.06.2007 : v0.3

*/


#include "libgimp/gimp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/***   some configuration switches   ***/

/*** macros ***/

  /* --- Helpers --- */

/* m_free (void*) */
#define m_free(x) {                                         \
  if (x != NULL) {                                          \
    free (x); x = NULL;                                     \
  } else {                                                  \
    g_print ("%s:%d %s() nothing to delete " #x "\n",       \
    __FILE__,__LINE__,__func__);                            \
  }                                                         \
}

/* m_new (void*, type, size_t, action) */
#define m_new(ptr,type,size,action) {                       \
  if (ptr != NULL)                                          \
    m_free (ptr)                                            \
  if ((size) <= 0) {                                        \
    g_print ("%s:%d %s() nothing to allocate %d\n",         \
    __FILE__,__LINE__,__func__, (int)size);                 \
  } else {                                                  \
    ptr = calloc (sizeof (type), (size_t)size);             \
  }                                                         \
  if (ptr == NULL) {                                        \
    g_message ("%s:%d %s() %s %d %s %s .",__FILE__,__LINE__,\
         __func__, _("Can not allocate"),(int)size,         \
         _("bytes of  memory for"), #ptr);                  \
    action;                                                 \
  }                                                         \
}



/***   versioning   ***/

static char plug_in_version_[64];
static char Version_[] = "v0.3";


/***   include files   ***/

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*** struct definitions ***/

/*** declaration of local functions ***/

  /* --- plug-in API specific functions --- */
static void   query      (void);
static void   run        (gchar     *name,
                          gint             nparams,
                          GimpParam *param,
                          gint            *nreturn_vals,
                          GimpParam      **return_vals);

  /* --- pnm functions --- */
static gint32 load_image    (	gchar		*filename);


static gint   save_image    (   gchar           *filename,
                                gint32           image_ID,
                                gint32           drawable_ID);


  /* --- helpers --- */
int wread ( unsigned char   *data,    /* read a word */
            size_t  pos,
            size_t  max,
            size_t *start,
            size_t *length );

/*** global variables ***/


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
  sprintf (plug_in_version_, "%s %s %s", "PNM plug-in for", 
  "CinePaint",
  Version_);

  return &plug_in_version_[0];
}

static void
query (void)
{
  char *date=NULL;
  
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to load." },
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
  };
  static int nsave_args = (int)sizeof (save_args) / (int)sizeof (save_args[0]);

  m_new (date, char,(200 + strlen(version())), gimp_quit())
  sprintf (date,"%s\n2005-2007", version()); 
  gimp_install_procedure ("file_pnm_load",
                          "loads files of the pnm file format",
                          "This plug-in loades PNM files. "
                          "The RAW_WIDTH, RAW_HEIGHT, RAW_TYPE and RAW_MAXVAL "
                          "variables can be used to load raw data ending with "
                          "\".raw\" .",
                          "Kai-Uwe Behrmann",
                          "2005-2007 Kai-Uwe Behrmann",
                          date,
                          "<Load>/PNM",
                          NULL,
                          GIMP_PLUGIN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);
  gimp_register_load_handler ("file_pnm_load", "pgm,pnm,ppm,pfm,raw", "");

  gimp_install_procedure ("file_pnm_save",
                          "saves files in the pnm file format",
                          "This plug-in saves PNM files.",
                          "Kai-Uwe Behrmann",
                          "2005-2007 Kai-Uwe Behrmann",
                          date,
                          "<Save>/PNM",
                          "RGB*, GRAY*, U16_RGB*, U16_GRAY*",
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_save_handler ("file_pnm_save", "pgm,pnm,ppm", "");

  gimp_install_procedure ("file_pfm_save",
                          "saves files in the pfm file format",
                          "This plug-in saves PFM files.",
                          "Kai-Uwe Behrmann",
                          "2005-2007 Kai-Uwe Behrmann",
                          date,
                          "<Save>/PFM",
                          "FLOAT_RGB*, FLOAT_GRAY*",
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_save_handler ("file_pfm_save", "pfm", "");

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

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;


  if (strcmp (name, "file_pnm_load") == 0)
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
  else if (strcmp (name, "file_pnm_save") == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there! */
	  if (nparams != 5)
	    status = GIMP_PDB_CALLING_ERROR;
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32,
                      param[2].data.d_int32) == 1)
	{
	  values[0].data.d_status = GIMP_PDB_SUCCESS;
	}
      else
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;


    }
}

int wread ( unsigned char* data, size_t pos, size_t max, size_t *start, size_t *end )
{
  int end_found = 0;
  
  if( max <= 1 ) return 0;
  
  while(pos < max && isspace( data[pos] )) ++pos;
  *start = pos;
  
  while(pos < max && !end_found) {
    if( isspace( data[pos] ) ) { 
      end_found = 1;
      break;
    } else
      ++pos;
  }
  *end = pos;
  
  return end_found;
} 


static gint32
load_image ( gchar *filename)
{
  gchar  *name;
  gint32  image_ID;
  GImageType    image_type = 0;
  GDrawableType layer_type = 0;
  GimpDrawable *drawable = NULL;

  FILE   *fp = NULL;
  int     fsize = 0;
  size_t  fpos = 0;
  unsigned char *data = 0;
  size_t  mem_n = 0;  /* needed memory in bytes */

  int info_good = 1;

  int type = 0;       /* PNM type */
  int width = 0;
  int height = 0;
  int spp = 0;        /* samples per pixel */
  int byteps = 1;        /* byte per sample */
  double maxval = 0;

  size_t start, end;

# ifdef DEBUG
  printf("%s:%d %s() 0x%x\n",__FILE__,__LINE__,__func__,(intptr_t)filename);
#endif

  name = g_strdup_printf( _("Opening '%s'..."), filename);
  gimp_progress_init (name);
  m_free (name)


  fp = fopen (filename, "rm");
  if (!fp) {
    g_message (_("Could not open '%s' for reading: %s"),
                 filename, g_strerror (errno));
    return FALSE;
  }
  fseek(fp,0L,SEEK_END);
  fsize = ftell(fp);
  rewind(fp);

  m_new (data, char, fsize, return FALSE)

  fpos = fread( data, sizeof(char), fsize, fp );
  if( fpos < fsize ) {
    g_message (_("Could not read '%s' : %s %d/%d"),
                 filename, g_strerror (errno), fsize,(int)fpos);
    m_free( data )
    fclose (fp);
    return FALSE;
  }
  fpos = 0;
  fclose (fp);
  fp = NULL;


  /* parse Infos */
  if(data[fpos] == 'P')
  {
    if(isdigit(data[++fpos])) {
      char tmp[2] = {0, 0};
      tmp[0] = data[fpos];
      type = atoi(tmp);
    } else
    if (!isspace(data[fpos]))
    {
      if(data[fpos] == 'F') /* PFM rgb */
        type = -6;
      else if (data[fpos] == 'f') /* PFM gray */
        type = -5;
      else
        info_good = 0;
    }
    else
      info_good = 0;
  }
  fpos++;

  /* parse variables */
  {
    int in_c = 0;    /* within comment */
    int v_read = 0;  /* number of variables allready read */
    int v_need = 3;  /* number of needed variable; start with three */
    int l_end = 0;   /* line end position */
    int l_pos = 0;   /* line position */
    int l_rdg = 1;   /* line reading */

    if(type == 1 || type == 4)
           v_need = 2;
    if(type == 7) /* pam  */
           v_need = 12;

    while(v_read < v_need && info_good)
    {
      l_pos = l_end = fpos;
      l_rdg = 1;

      /* read line */
      while(fpos < fsize && l_rdg)
      {
        if(data[fpos] == '#') {
          in_c = 1;
          l_end = fpos-1;
        } else if(data[fpos] == 10 || data[fpos] == 13) { /* line break */
          l_rdg = 0;
        } else if(data[fpos] != 0) {
          if(!in_c)
            ++l_end;
        } else {
          l_rdg = 0;
        }
        if(!l_rdg) {
          in_c = 0;
        }
        ++fpos;
      }

      /* parse line */
      while(info_good &&
            v_read < v_need &&
            l_pos < l_end)
      {
        if( info_good )
        {
          double var = -2;
          char var_s[64];
          int l = 0;
          wread ( data, l_pos, l_end, &start, &end );
          l = end - start;
          if ( l < 63 ) {
            memcpy(var_s, &data[start], l);
            var_s[l] = 0;
            var = atof(var_s);
#           ifdef DEBUG
            printf("var = \"%s\"  %d\n",var_s, l);
#           endif
          }
          l_pos = end + 1;
          if(type == 7)
          {
            if(height == -1)
              height = (int)var;
            if(width == -1)
              width = (int)var;
            if(spp == -1)
              spp = (int)var;
            if(maxval == -0.5)
              maxval = var;

            if(strcmp(var_s, "HEIGHT") == 0)
              height = -1; /* expecting the next token is the val */
            if(strcmp(var_s, "WIDTH") == 0)
              width = -1;
            if(strcmp(var_s, "DEPTH") == 0)
              spp = -1;
            if(strcmp(var_s, "MAXVAL") == 0)
              maxval = -0.5;
            if(strcmp(var_s, "TUPLTYPE") == 0)
              ; /* ?? */
            if(strcmp(var_s, "ENDHDR") == 0)
              v_need = v_read;
          }
          else
          {
            if (!var)
              info_good = 0;
            if(v_read == 0)
              width = (int)var;
            else if(v_read == 1)
              height = (int)var;
            else if(v_read == 2)
              maxval = var;
          }

          ++v_read;

        }
      }
    }
  }

  if(strstr(strrchr(filename, '.')+1, "raw"))
  {
    info_good = 1;
    width = atoi(getenv("RAW_WIDTH"));
    height = atoi(getenv("RAW_HEIGHT"));
    type = atoi(getenv("RAW_TYPE"));
    fpos = 0;
    maxval = atoi(getenv("RAW_MAXVAL"));
  }


  if(info_good)
    switch(type) {
      case 1:
      case 4:
           image_type = GIMP_GRAY;
           layer_type = GRAY_IMAGE;
           spp = 1;
           info_good = 0;
           break;
      case 2:
      case 5:
           if(maxval <= 255) {
             image_type = GIMP_GRAY;
             layer_type = GRAY_IMAGE;
             byteps        = 1;
           } else if (maxval <= 65535) {
             image_type = U16_GRAY;
             layer_type = U16_GRAY_IMAGE;
             byteps        = 2;
           }
           spp = 1;
           break;
      case 3:
      case 6:
           if(maxval <= 255) {
             image_type = GIMP_RGB;
             layer_type = RGB_IMAGE;
             byteps        = 1;
           } else if (maxval <= 65535) {
             image_type = U16_RGB;
             layer_type = U16_RGB_IMAGE;
             byteps        = 2;
           }
           spp = 3;
           break;
      case -5:
           image_type = FLOAT_GRAY;
           layer_type = FLOAT_GRAY_IMAGE;
           byteps = 4;
           spp = 1;
           break;
      case -6:
           byteps = 4;
           spp = 3;
           image_type = FLOAT_RGB;
           layer_type = FLOAT_RGB_IMAGE;
           break;
      case 7: /* pam */
           if (maxval == 1.0 || maxval == -1.0) {
             byteps        = 4;
             switch(spp)
             {
               case 1:
                   {
                     image_type = FLOAT_GRAY;
                     layer_type = FLOAT_GRAY_IMAGE;
                   }
                    break;
               case 2:
                   {
                     image_type = FLOAT_GRAY;
                     layer_type = FLOAT_GRAYA_IMAGE;
                   }
                    break;
               case 3:
                   {
                     image_type = FLOAT_RGB;
                     layer_type = FLOAT_RGB_IMAGE;
                   }
                    break;
               case 4:
                   {
                     image_type = FLOAT_RGB;
                     layer_type = FLOAT_RGBA_IMAGE;
                   }
                    break;
             }
           } else if(maxval <= 255) {
                     byteps        = 1;
             switch(spp)
             {
               case 1:
                   {
                     image_type = GIMP_GRAY;
                     layer_type = GRAY_IMAGE;
                   }
                    break;
               case 2:
                   {
                     image_type = GIMP_GRAY;
                     layer_type = GRAYA_IMAGE;
                   }
                    break;
               case 3:
                   {
                     image_type = GIMP_RGB;
                     layer_type = RGB_IMAGE;
                   }
                    break;
               case 4:
                   {
                     image_type = GIMP_RGB;
                     layer_type = RGBA_IMAGE;
                   }
                    break;
             }
           } else if (maxval <= 65535) {
                     byteps        = 2;
             switch(spp)
             {
               case 1:
                   {
                     image_type = U16_GRAY;
                     layer_type = U16_GRAY_IMAGE;
                   }
                    break;
               case 2:
                   {
                     image_type = U16_GRAY;
                     layer_type = U16_GRAYA_IMAGE;
                   }
                    break;
               case 3:
                   {
                     image_type = U16_RGB;
                     layer_type = U16_RGB_IMAGE;
                   }
                    break;
               case 4:
                   {
                     image_type = U16_RGB;
                     layer_type = U16_RGBA_IMAGE;
                   }
                    break;
             }
           }
           break;
      default:
           info_good = 0;
    }

# ifdef DEBUG
  printf("wxh %dx%d  type: %d|%d|%d   bytes/samples %d %d \n",width,height,image_type,layer_type,type,
                                       byteps, spp);
# endif

  if( !info_good )
  {
    g_message ("%s:%d failed to get info of %s",__FILE__,__LINE__,filename);
    m_free( data )
    return FALSE;
  }

  /* check if the file can hold the expected data (for raw only) */
  mem_n = width*height*byteps*spp;
  if(type == 5 || type == 6 || type == -5 || type == -6 || type == 7)
  {
    if (mem_n > fsize-fpos)
    {
      g_message ("%s:%d  storage size of %s is too small: %d",__FILE__,__LINE__,filename , (int)mem_n-fsize-fpos);
      m_free( data )
      return FALSE;
    }
  } else {
    if (type == 2 || type == 3) {
      g_message ("%s:%d  %s contains ascii data, which are not handled by this pnm reader",__FILE__,__LINE__,filename);
    } else if (type == 1 || type == 4) {
      g_message ("%s:%d  %s contains bitmap data, which are not handled by this pnm reader",__FILE__,__LINE__,filename);
    }
    m_free( data )
    return FALSE;
  }

  if ((image_ID = gimp_image_new ((guint)width, (guint)height,
                  image_type)) == -1) {
      g_message("PNM can't create a new image\n%dx%d %d",
                 width, height, image_type);
      m_free (data)
    return FALSE;
  }

  gimp_image_set_filename (image_ID, filename);

  {
    int layer = 0;
    layer = gimp_layer_new      (image_ID, _("background"), 
                                (guint)width,
                                (guint)height,
                                (guint)layer_type,
                                100.0, NORMAL_MODE);

    gimp_image_add_layer (image_ID, layer, 0);
    drawable = gimp_drawable_get (layer);
  }

  {
    int   h, p, n_samples, n_bytes;
    int   j_h,         /* jump_height */
          c_h;         /* current_height */
    unsigned char *buf = 0,
                  *d_8 = 0;
    unsigned char *src;

    guint16 *d_16;
    gfloat  *d_f;
    GimpPixelRgn pixel_rgn;

    int byte_swap = 0;
    size_t sn = width * gimp_tile_height() * byteps * spp;

    gimp_pixel_rgn_init (&pixel_rgn, drawable,0,0, width, height, TRUE,FALSE);
    m_new ( buf, char, sn, return FALSE)
#   ifdef DEBUG
    printf("wxh %dx%d  bytes|samples: %d|%d buf size: %d\n", width,height, byteps, spp, sn);
#   endif

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    if( (byteps == 2) ||
        (maxval > 0 && byteps == 4)  ) {
      byte_swap = 1;
    }
#else
    if(maxval < 0 && byteps == 4)
      byte_swap = 1;
#endif
    src = &data[fpos];

    maxval = abs(maxval);

    c_h = gimp_tile_height();
    for (j_h = 0; j_h < height; j_h += gimp_tile_height())
    {
      if(j_h + gimp_tile_height() > height)
        c_h = height - j_h;

      for (h = 0; h < c_h; ++h)
      {
        n_samples = 1 * width * spp;
        n_bytes = n_samples * byteps;

        d_8  = buf;
        d_16 = (guint16*)buf;
        d_f  = (gfloat*)buf;

        /*  TODO 1 bit raw and ascii */
        if (type == 1 || type == 4) {

        /*  TODO ascii  */
        } else if (type == 2 || type == 3) {
        

        /*  raw and floats */
        } else if (type == 5 || type == 6 ||
                   type == -5 || type == -6 ||
                   type == 7 ) {
          if(byteps == 1) {
            d_8 = &src[ h * width * spp * byteps ];
          } else if(byteps == 2) {
            d_16 = (guint16*)& src[ h * width * spp * byteps ];
          } else if(byteps == 4) {
            d_f = (gfloat*)&src[ h * width * spp * byteps ];
          }
          memcpy (&buf[ h * width * spp * byteps ],
                  &src[ (j_h + h) * width * spp * byteps ],
                  1 * width * spp * byteps);
        }

        /* normalise and byteswap */
        if( byte_swap ) {
          unsigned char *c_buf = &buf[ h * width * spp * byteps ];
          char  tmp;
          if (byteps == 2) {         /* 16 bit */
            for (p = 0; p < n_bytes; p += 2)
            {
              tmp = c_buf[p];
              c_buf[p] = c_buf[p+1];
              c_buf[p+1] = tmp;
            }
          } else if (byteps == 4) {  /* float */
            for (p = 0; p < n_bytes; p += 4)
            {
              tmp = c_buf[p];
              c_buf[p] = c_buf[p+3];
              c_buf[p+3] = tmp;
              tmp = c_buf[p+1];
              c_buf[p+1] = c_buf[p+2];
              c_buf[p+2] = tmp;
            }
          }
        }

        if (byteps == 1 && maxval < 255) {         /*  8 bit */
          for (p = 0; p < n_samples; ++p)
            d_8[p] = (d_8[p] * 255) / maxval;
        } else if (byteps == 2 && maxval < 65535) {/* 16 bit */
          for (p = 0; p < n_samples; ++p)
            d_16 [p] = (d_16[p] * 65535) / maxval;
        } else if (byteps == 4 && maxval != 1.0) {  /* float */
          for (p = 0; p < n_samples; ++p)
            d_f[p] = d_f[p] * maxval;
        }

        gimp_progress_update ((double) (j_h + h) / (double) height);
      }
      gimp_pixel_rgn_set_rect(&pixel_rgn,
                              buf, 0, j_h, width, c_h);
    }
    m_free( buf )
  }

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);
  m_free (data)

  return image_ID;
}


static gint
save_image                  (   gchar           *filename,
                                gint32           image_ID,
                                gint32           drawable_ID)
{
  guint    width, height;	/* view sizes */
  gint32 *layers;
  gint nlayers;
  FILE *fp=NULL;

# ifdef DEBUG
  printf("%s:%d %s() image_ID: %d\n",__FILE__,__LINE__,__func__, image_ID);
# endif
  layers = gimp_image_get_layers (image_ID, &nlayers);

  if ( nlayers == -1 ) {
    g_message("no image data to save");
    gimp_quit ();
  }

  {
    int byte_swap = 0;
    int drawable_type;
    int byteps = 1;        /* byte per sample */
    int bps = 8;
    int spp;
    int alpha;
    size_t size;

             char *colourspacename = 0;
    unsigned char *src = NULL;

    GimpPixelRgn pixel_rgn;
    GDrawable *drawable;

    drawable      = gimp_drawable_get (drawable_ID);
    drawable_type = gimp_drawable_type (drawable_ID);

    width         = drawable->width;
    height        = drawable->height;

    switch (drawable_type)
        {
        case RGB_IMAGE:
          spp = 3;
          bps = 8;
          alpha = 0;
          break;
        case GRAY_IMAGE:
          spp = 1;
          bps = 8;
          alpha = 0;
          break;
        case RGBA_IMAGE:
          spp = 4;
          bps = 8;
          alpha = 1;
          break;
        case GRAYA_IMAGE:
          spp = 2;
          bps = 8;
          alpha = 1;
          break;
        case U16_RGB_IMAGE:
          spp = 3;
          bps = 16;
          alpha = 0;
          break;
        case U16_GRAY_IMAGE:
          spp = 1;
          bps = 16;
          alpha = 0;
          break;
        case U16_RGBA_IMAGE:
          spp = 4;
          bps = 16;
          alpha = 1;
          break;
        case U16_GRAYA_IMAGE:
          spp = 2;
          bps = 16;
          alpha = 1;
          break;
        case FLOAT_RGB_IMAGE:
          spp = 3;
          bps = 32;
          alpha = 0;
          break;
       case FLOAT_GRAY_IMAGE:
          spp = 1;
          bps = 32;
          alpha = 0;
          break;
        case FLOAT_RGBA_IMAGE:
          spp = 4;
          bps = 32;
          alpha = 1;
          break;
        case FLOAT_GRAYA_IMAGE:
          spp = 2;
          bps = 32;
          alpha = 1;
          break;
        default:
          g_print ("Can't save this image type\n");
          m_free (drawable)
          return 0;
      }

    byteps = bps/8;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    if( (byteps == 2) ||
        (byteps == 4)  ) {
      byte_swap = 1;
    }
#endif

#if 0
    if (alpha)
    {
      g_message (_("Alpha is not supported\n"));
      m_free (drawable)
      return 0;
    }
#endif

    if (gimp_image_has_icc_profile(image_ID, ICC_IMAGE_PROFILE))
    {
      /*if (strcmp (gimp_image_get_icc_profile_color_space_name(image_ID, ICC_IMAGE_PROFILE) , "Rgb")
        != 0)
      {
          g_message (_("This Colourspace is not supported\n"));
          m_free (drawable)
          return 0;
      }*/
      colourspacename = gimp_image_get_icc_profile_description( image_ID, ICC_IMAGE_PROFILE );
    }

    size = width * height * spp * byteps;
    m_new ( src, char, size, return FALSE )

    gimp_pixel_rgn_init (&pixel_rgn, drawable,0,0, width, height, TRUE,FALSE);
    gimp_pixel_rgn_get_rect (&pixel_rgn, src, 0,0, width, height);

    if ((fp=fopen(filename, "wb")) != NULL &&
        src && size)
    {
      size_t pt = 0;
      char text[128];
      int  len = 0;
      int  i = 0;
      char bytes[48];

      fputc( 'P', fp );
      if(alpha)
            fputc( '7', fp );
      else
      {
        if(byteps == 1 ||
           byteps == 2)
        {
          if(spp == 1)
            fputc( '5', fp );
          else
            fputc( '6', fp );
        } else
        if (byteps == 4)
        {
          if(spp == 1)
            fputc( 'f', fp ); /* PFM gray */
          else
            fputc( 'F', fp ); /* PFM rgb */
        }
      }

      fputc( '\n', fp );

      {
        time_t	cutime;         /* Time since epoch */
        struct tm	*gmt;
        gchar time_str[24];

        cutime = time(NULL); /* time right NOW */
        gmt = gmtime(&cutime);
        strftime(time_str, 24, "%Y/%m/%d %H:%M:%S", gmt);
        snprintf( text, 84, "# CREATOR: %s : %s\n",
                  version(), time_str );
      }
      len = strlen( text );
      do { fputc ( text[pt++] , fp);
      } while (--len); pt = 0;

      snprintf( text, 84, "# COLORSPACE: %s\n", colourspacename ?
                colourspacename : "--" );
      len = strlen( text );
      do { fputc ( text[pt++] , fp);
      } while (--len); pt = 0;

        if(byteps == 1)
          snprintf( bytes, 84, "255" );
        else
        if(byteps == 2)
          snprintf( bytes, 84, "65535" );
        else
        if (byteps == 4)
        {
          if(G_BYTE_ORDER == G_LITTLE_ENDIAN)
            snprintf( bytes, 84, "-1.0" );
          else
            snprintf( bytes, 84, "1.0" );
        }
        else
          g_message ("Error: byteps: %d", byteps);

      if(alpha)
      {
        const char *tupl = "RGB_ALPHA";

        if(spp == 2)
          tupl = "GRAYSCALE_ALPHA";
        snprintf( text, 128, "WIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL %s\nTUPLTYPE %s\nENDHDR\n", width, height, spp, bytes, tupl );
        len = strlen( text );
        do { fputc ( text[pt++] , fp);
        } while (--len); pt = 0;

      }
      else
      {
        snprintf( text, 84, "%d %d\n", width, height );
        len = strlen( text );
        do { fputc ( text[pt++] , fp);
        } while (--len); pt = 0;

        snprintf( text, 84, "%s\n", bytes );
        len = strlen( text );
        do { fputc ( text[pt++] , fp);
        } while (--len); pt = 0;
      }

#     ifdef DEBUG
      printf("%s:%d %s() size: %d\n",__FILE__,__LINE__,__func__, (int)size);
#     endif
      do {
        if(byte_swap && (byteps == 2))
        { if(size%2)
            fputc ( src[pt++ - 1] , fp);
          else
            fputc ( src[pt++ + 1] , fp);
        } else
          fputc ( src[pt++] , fp);
        ++i;
      } while (--size); pt = 0;

#     ifdef DEBUG
      printf("%s:%d %s() size: %d\n",__FILE__,__LINE__,__func__, i);
#     endif

    }


    if (fp)
    {
      fflush( fp );
      fclose (fp);
    }

    gimp_drawable_detach (drawable);
    m_free (src)
  }
  return 0;
}

