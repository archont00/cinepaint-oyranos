/* 
 * OpenEXR RGBA file plug-in for filmgimp.
 * Based loosely on the Cineon and SGI plug-ins.
 *
 * Copyright (C) 2002 Drew Hess <dhess@ilm.com>
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
 * OpenEXR images are typically linear.  Filmgimp needs a "display gamma"
 * concept so that linear images can be displayed properly on a CRT or
 * LCD.  For now, however, you actually have to set the gamma of the
 * image using the Gamma/Expose control if you want the image to
 * look "right."  Don't forget to invert the gamma before you write the 
 * image back out (should be written out as linear data).
 */

#define PLUGIN_VERSION "0.5.0 - 10 Jan 2007"
#define PLUGIN_NAME    "CinePaint OpenEXR plug-in"
#define PLUGIN_COMPLETENAME PLUGIN_NAME " " PLUGIN_VERSION

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ImfRgbaFile.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfFrameBuffer.h>
#include <ImfStandardAttributes.h>
#include <Iex.h>
#include <ImathBox.h>
#include <half.h>
#include <vector>
#include "gtk/gtk.h"
#include "chroma_icc.h"
extern "C" {
#include "lib/plugin_main.h"
#include "lib/wire/libtile.h"
#include "lib/plugin_pdb.h"
#include "libgimp/stdplugins-intl.h"
}

#ifdef __cplusplus
extern "C" {
#endif

static void   query      (void);
static void   run        (char    *name,
			  int      nparams,
			  GParam  *param,
			  int     *nreturn_vals,
			  GParam **return_vals);
static gint32 load_image (char    *filename);
static gint   save_image (char    *filename,
			  gint32   image_ID,
			  gint32   drawable_ID);
char** extractPass       (const char *channel_name, int *count );
int    getChannelPos     (int channel_in_layer_pos,
                          std::vector<std::string> & channels,
                          int* chan_pos );
char * getChannelName    (int position, const char *layer_name, int nlayers,
                          int max, int & exr_channels );

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

MAIN()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_openexr_load",
                          "Loads files in the OpenEXR file format",
			  "Loads OpenEXR image files.",
                          "Drew Hess <dhess@ilm.com>, Kai-Uwe Behrmann <ku.b@gmx.de",
                          "Copyright 2002 Lucas Digital Ltd.",
                          PLUGIN_VERSION,
                          "<Load>/OpenEXR",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_openexr_save",
                          "Saves files in the OpeneXR file format",
                          "Saves OpenEXR image files.",
                          "Drew Hess <dhess@ilm.com>, Kai-Uwe Behrmann <ku.b@gmx.de",
                          "Copyright 2002 Lucas Digital Ltd.",
                          PLUGIN_VERSION,
                          "<Save>/OpenEXR",
                          "FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler("file_openexr_load", "exr", "", "");
  gimp_register_save_handler ("file_openexr_save", "exr", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  gint32 image_ID;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  INIT_I18N_UI();

  if (strcmp (name, "file_openexr_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_openexr_save") == 0)
    {
      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	  values[0].data.d_status = STATUS_SUCCESS;
      else
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}

static gint32
load_image (char *filename)
{
    float * scanline = 0;
    guint16 * hscanline = 0;
    gfloat ** pixels = 0;
    guint16 ** hpixels = 0;

    try
    {
	Imf::/*Rgba*/InputFile exr_full (filename);
        Imf::Header exr_full_header = exr_full.header();
	Imf::InputFile exr (filename);
        
	// Set up progress meter.

	char progress[PATH_MAX];
	if (strrchr (filename, '/') != NULL)
	    snprintf (progress, PATH_MAX, _("Loading %s"),
		      strrchr (filename, '/') + 1);
	else
	    snprintf (progress, PATH_MAX, _("Loading %s"), filename);
	gimp_progress_init(progress);

	// Get image dimensions and set up gimp image/layer variables
	
	const Imath::Box2i & dataWindow = exr_full_header.dataWindow ();
	const Imath::Box2i & displayWindow = exr_full_header.displayWindow ();

	int width = dataWindow.max.x - dataWindow.min.x + 1;
	int height = dataWindow.max.y - dataWindow.min.y + 1;
	int iwidth = displayWindow.max.x - displayWindow.min.x + 1;
	int iheight = displayWindow.max.y - displayWindow.min.y + 1;
	GImageType image_type = FLOAT_RGB;
	GDrawableType layer_type = FLOAT_RGBA_IMAGE;
	int nchan=4;

        Imf::ChannelList cl = exr_full_header.channels();
        nchan = 0;
        std::vector<Imf::Channel> c_channels;
        std::vector<std::string> c_names;
        int hasZ = -1;
        Imf::PixelType etype = Imf::UINT;
        
        for (Imf::ChannelList::ConstIterator i = cl.begin(); i != cl.end(); ++i)
          {
            c_names.push_back( i.name() );
            c_channels.push_back( i.channel() );

            if(c_names[nchan] == "Z")
              hasZ = nchan;

            if(i.channel().type > etype)
              etype = i.channel().type;

            std::cout << i.name();
            switch(i.channel().type)
            {
              case Imf::UINT: std::cout <<"u "; break;
              case Imf::HALF: std::cout <<"h "; break;
              case Imf::FLOAT: std::cout <<"f "; break;
              default: std::cout <<"? "; break;
            }
            ++nchan;
          }
        std::cout << std::endl;

        if(etype == Imf::UINT)
          etype = Imf::FLOAT;

        const Imf::StringAttribute *str_attr =
          exr_full_header.findTypedAttribute <Imf::StringAttribute>("BlenderMultiChannel");
        if(str_attr)
          std::cout << "BlenderMultiChannel: " << str_attr->value() << std::endl;
        int nlayers = 1;
        int **chan_per_layer = NULL;
        char **lnames = NULL;
        int max_chan = nchan;
        /*if (str_attr ||
            (strchr(c_names[0].c_str(), '.') &&
             c_names[0].c_str() != strchr(c_names[0].c_str(), '.')) )*/
        {

          nlayers = 0;
          max_chan = 0;
          chan_per_layer = (int**)calloc (sizeof(int*),256);
          lnames = (char**)calloc(sizeof(char*), 256);
          for (int i = 0; i < nchan; ++i)
          {
            char *lname = (char*)calloc( sizeof(char),
                                         strlen( c_names[i].c_str() ) + 12 );
            sprintf(lname, "%s", c_names[i].c_str() );
            int pass = 0;
            char **ptr = extractPass( lname, &pass );
            if( ptr && pass > 1 )
            {
# ifdef DEBUG
              for( int k = 0; k < pass; ++k )
                printf( ":%d %d:%s\n", __LINE__,k, ptr[k] );
# endif
              sprintf( lname, "%s", ptr[1] );
            } else
              sprintf( lname, "%s", ptr[0] );

            int found = -1;
            if( i )
              for( int j = 0; j < nlayers; ++j )
                if( strstr( lnames[j], lname ) != NULL )
                  found = j;

            if((found < 0 && pass > 1) ||
               !i)
            {
              lnames[nlayers] = lname;
              ++nlayers;
              chan_per_layer[nlayers-1] = (int*)calloc (sizeof(int),32);
              chan_per_layer[nlayers-1][0] = 0;
# ifdef DEBUG
              printf(":%d %d: \"%s\"\n", __LINE__,i, lnames[nlayers-1]);
# endif
            } else
              free(lname);

            sprintf( &lnames[nlayers-1][strlen(lnames[nlayers-1])], ".%s", ptr[0] );
            ++chan_per_layer[nlayers-1][0];
            chan_per_layer[nlayers-1][chan_per_layer[nlayers-1][0]] = i;

            free(ptr[0]); if(ptr[1]) free(ptr[1]); free(ptr);
          }
          for( int i = 0; i < nlayers; ++i)
          {
            if(chan_per_layer[i][0] > max_chan)
              max_chan = chan_per_layer[i][0];
# ifdef DEBUG
            printf(":%d %d:%d %s\n", __LINE__,i,chan_per_layer[i][0],lnames[i]);
# endif
          }
        }

        str_attr =
          exr_full_header.findTypedAttribute <Imf::StringAttribute>("comments");
        if (str_attr)
          std::cout << "comments: " << str_attr->value() << std::endl;


	// Set up temporary buffers

        int chan = nchan;
        int cchan = 0; // channels in CinePaint
        if(nlayers > 1)
          chan = max_chan;
	scanline = new float [( dataWindow.max.x + 1) * chan];
        hscanline = (guint16*) scanline;
        char *n = "A";
        const char* cname = NULL;
        if(hasZ >= 0)
        {
       	  fprintf(stderr, "load Z buffer into alpha channel\n");
          n = "Z";
        }
        if(chan == 1 || chan == 2)
        {
          layer_type = (etype == Imf::FLOAT) ? FLOAT_GRAY_IMAGE : FLOAT16_GRAY_IMAGE;
          image_type = (etype == Imf::FLOAT) ? FLOAT_GRAY : FLOAT16_GRAY;
          cchan = 1;
	  fprintf(stderr, "1 channel image %s\n", cname);
        }
        if(chan == 2)
        {
          layer_type = (etype == Imf::FLOAT) ? FLOAT_GRAYA_IMAGE : FLOAT16_GRAYA_IMAGE;
          cchan = 2;
	  fprintf(stderr, "2 channel image\n");
        }
        if(chan >= 3)
        {
          layer_type = (etype == Imf::FLOAT) ? FLOAT_RGB_IMAGE : FLOAT16_RGB_IMAGE;
          image_type = (etype == Imf::FLOAT) ? FLOAT_RGB : FLOAT16_RGB;
       	  fprintf(stderr, "3 channel image\n");
          cchan = 3;
        }
        if(chan >= 4)
        {
          layer_type = (etype == Imf::FLOAT) ? FLOAT_RGBA_IMAGE : FLOAT16_RGBA_IMAGE;
	  fprintf(stderr, "alpha channel\n");
          cchan = 4;
        }
        /*for(int c = 5; c < chan; ++c)
	  fb.insert (c_names[c].c_str(), Imf::Slice (c_channels[c].type,
					(char *) scanline + 3 * sizeof (float), 
					xstride, 0));*/

	int tile_height = gimp_tile_height();

	pixels = (gfloat**) malloc(tile_height*sizeof(gfloat*));
	if (!pixels)
	    THROW (Iex::BaseExc, "Out of memory");

	pixels[0] = g_new(gfloat, tile_height * width * cchan);
	if (!pixels[0])
	    THROW (Iex::BaseExc, "Out of memory");

        int type_size = (etype == Imf::FLOAT) ? sizeof(float) : sizeof(guint16);
	
 hpixels = (guint16**)pixels;
        hpixels[0] = (guint16*)pixels[0];
	for (int i = 1; i < tile_height; i++) {
          if(type_size == 2)
	    hpixels[i] = hpixels[i - 1] + width * cchan;
          else
	    pixels[i] = pixels[i - 1] + width * cchan;
	}


	gint32 image_ID = (GImageType) gimp_image_new (iwidth, iheight, 
							   image_type);
	gimp_image_set_filename (image_ID, filename);
        for( int i = 0; i < nlayers; ++i )
        {
# define InsertSlice_m( position ) \
            pos = getChannelPos( position, c_names, chan_per_layer[i] ); \
            if(pos >= 0) \
            { \
              cname = c_names[ pos ].c_str(); \
              fb.insert (cname, Imf::Slice (etype, (char *) scanline \
                         + position * type_size, xstride, 0)); \
            }
          int pos = -1;
          size_t xstride = type_size * chan;
          Imf::FrameBuffer fb;
          if(chan_per_layer[i][0] == 1 || chan_per_layer[i][0] == 2)
          {
            InsertSlice_m( 0 )
          }
          if(chan_per_layer[i][0] == 2)
          {
            InsertSlice_m( 1 )
          }
          if(chan_per_layer[i][0] >= 3)
          {
            InsertSlice_m( 0 )
            InsertSlice_m( 1 )
            InsertSlice_m( 2 )
          }
          if(chan_per_layer[i][0] >= 4)
          {
            InsertSlice_m( 3 )
          }
          // ignoring all following channels; TODO copy them elsewhere
# undef   InsertSlice_m
          // possibly insert non existing slices
          /*for( int j = chan_per_layer[i][0]; j < chan; ++j )
            fb.insert ("NOT_AVAILABLE", Imf::Slice (Imf::FLOAT,
                                        (char *) scanline + j * sizeof (float),
                                        xstride, 0));*/
          exr.setFrameBuffer (fb);


          char *ln = _("Background");
          // the technical layer.pass.channels name from our OpenEXR file
          // or need a gui with predefined selectors and storing in metablocks
          if(nlayers > 1 && lnames)
            ln = lnames[i];
          gint32 layer_ID = (GDrawableType) gimp_layer_new(image_ID, ln, 
 							        width, height,
							        layer_type, 100, NORMAL_MODE);
          gimp_image_add_layer(image_ID, layer_ID, 0);
          gimp_layer_set_offsets( layer_ID, dataWindow.min.x - displayWindow.min.x,
                                          dataWindow.min.y - displayWindow.min.y );
          GDrawable * drawable = gimp_drawable_get (layer_ID);

	  GPixelRgn pixel_rgn;
          gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, 
	                       drawable->height, TRUE, FALSE);


          int y = dataWindow.min.y;
	  while (y < dataWindow.max.y)
	  {
	    int start = y;
	    int end = MIN (y + tile_height, dataWindow.max.y);
	    int scanlines = end - start;
	    for (int k = 0; k != scanlines; ++k)
	    {
              int line = y;
              if(exr_full_header.lineOrder() == Imf::DECREASING_Y)
                ;//line = dataWindow.min.y + dataWindow.max.y - y;

              exr.readPixels (line);
              if(chan <= 4)
              {
                if(type_size == 2)
                {
                  memcpy( hpixels[k], hscanline + dataWindow.min.x*chan, type_size * width * cchan);
                } else {
                  memcpy( pixels[k], scanline + dataWindow.min.x*chan, type_size * width * cchan);
                }
              }
              else
              for (int x = 0; x != (width); ++x)
              {
		    gfloat * floatPixel = pixels[0] + width*k*cchan + x * cchan;
		    float * fl = scanline + (x + dataWindow.min.x)*chan;
                    for(int c = 0; c < chan; ++c)
                      if( c < 4 )
                      {
                        // copy normally
                        if( c < chan_per_layer[i][0] )
                          *floatPixel++ = fl[c];
                        // fill all remaining channels to last colour value
                        else if( chan > 2 && c < 3 )
                          *floatPixel++ = fl[0];
                        // fill with one as we get random values otherwise
                        else
                          *floatPixel++ = 1.0;
                        // ignore, as it does not fit in the old CinePaint sheme
                      } else
                        *floatPixel++;
	      }
	      ++y;
	    }
	    gimp_pixel_rgn_set_rect(&pixel_rgn, (guchar *)pixels[0], 0, start-dataWindow.min.y, 
				    drawable->width, scanlines);
	    gimp_progress_update((double)(y-dataWindow.min.y) / (double)height);
	  }

          gimp_drawable_flush(drawable);
          gimp_drawable_detach(drawable);
        }

        // Clean up

        g_free(pixels[0]);
        g_free(pixels);
        delete [] scanline;

        // colour space description -> ICC profile
	Imf::Chromaticities image_prim;
	if(hasChromaticities(exr.header()))
	{
	  const Imf::Chromaticities &prims = chromaticities (exr.header());
          image_prim = prims;
#if DEBUG
          printf("%s:%d\nred xy: %f %f\ngreen xy: %f %f\nblue xy: %f %f\nwhite xy: %f %f gamma assumed: 1.0\n",__FILE__,__LINE__,
                 image_prim.red[0],image_prim.red[1],
                 image_prim.green[0],image_prim.green[1],
                 image_prim.blue[0],image_prim.blue[1],
                 image_prim.white[0],image_prim.white[1]);
        } else {
          printf("%s:%d standard chromaticity  gamma assumed: 1.0\n",
                 __FILE__,__LINE__);
#endif
        }

        if( cchan > 2 ) {
          size_t size = 0;
          char* mem_profile = 0;
          mem_profile = writeICCprims (image_prim, size);
          if(mem_profile && size)
            gimp_image_set_icc_profile_by_mem (image_ID,
                                (guint32)size, mem_profile, ICC_IMAGE_PROFILE);

          free (mem_profile);
        }

        // Update the display
        return image_ID;
    }
    catch (Iex::BaseExc & e)
    {
	delete [] scanline;
	if (pixels && pixels[0])
	    g_free (pixels[0]);
	if (pixels)
	    g_free (pixels);

	g_print (e.what ());
	gimp_quit ();
    }
}

/** extract the layer name hierarchy in a null terminated array, maximal 2 */
char**
extractPass ( const char *channel_name, int *count )
{
  char ** names = (char**) calloc(sizeof(char*), 4);
  char *text = strdup( channel_name );
  char *ptr;
  int i;
  i = 0;
  if(count) *count = 0;
  if((ptr = strrchr( text, '.' )) != NULL)
  {
    names[i] = strdup(ptr+1);
    ptr[0] = '\0';
    ++i;
  }
  names[i] = text;
  if(count) *count = i+1;
  return names;
}

/** returns the position of channel_name in channels from possible positions in
    chan_pos */
int
findChannelType ( const char* channel_type,
                  std::vector<std::string> & channels,
                  int* chan_pos )
{
  for( int i = 1; i <= chan_pos[0]; ++i )
  {
    const char *ptr = channels[chan_pos[i]].c_str();
    if( ptr[strlen(ptr)-1] == channel_type[0] )
      return chan_pos[i];
  }
  return -1;
}

// sort some layouts into our RGBA system
int
getChannelPos     (int channel_in_layer_pos,
                   std::vector<std::string> & channels,
                   int* chan_pos )
{
  int pos = 0;
  if(findChannelType( "R", channels, chan_pos ) >= 0)
    switch(channel_in_layer_pos)
    {
      case 0: return findChannelType( "R", channels, chan_pos );
      case 1: return findChannelType( "G", channels, chan_pos );
      case 2: return findChannelType( "B", channels, chan_pos );
      case 3:
        pos = findChannelType( "A", channels, chan_pos );
        if(pos >= 0)
          return pos;
        else
          return findChannelType( "Z", channels, chan_pos );
    }
  else if(findChannelType( "X", channels, chan_pos ) >= 0)
    switch(channel_in_layer_pos)
    {
      case 0: return findChannelType( "X", channels, chan_pos );
      case 1: return findChannelType( "Y", channels, chan_pos );
      case 2: return findChannelType( "Z", channels, chan_pos );
      case 3:
        pos = findChannelType( "A", channels, chan_pos );
        if(pos >= 0)
          return pos;
        else
          return findChannelType( "Z", channels, chan_pos );
    }
  else if(findChannelType( "Y", channels, chan_pos ) >= 0)
    switch(channel_in_layer_pos)
    {
      case 0: return findChannelType( "Y", channels, chan_pos );
      case 1: return findChannelType( "U", channels, chan_pos );
      case 2: return findChannelType( "V", channels, chan_pos );
      case 3:
        pos = findChannelType( "A", channels, chan_pos );
        if(pos >= 0)
          return pos;
        else
          return findChannelType( "Z", channels, chan_pos );
    }
  else if(findChannelType( "U", channels, chan_pos ) >= 0)
    switch(channel_in_layer_pos)
    {
      case 0: return findChannelType( "U", channels, chan_pos );
      case 1: return findChannelType( "V", channels, chan_pos );
      case 2:
        pos = findChannelType( "A", channels, chan_pos );
        if(pos >= 0)
          return pos;
        else
          return findChannelType( "Z", channels, chan_pos );
    }
  else if(channel_in_layer_pos < chan_pos[0])
      return chan_pos[ channel_in_layer_pos + 1 ];

  return -1;
}

// per CinePaint layer information
typedef struct {
  gint32 drawable_ID;
  GDrawableType drawable_type;
  GDrawable * drawable;
  int width;
  int height;
  int dx, dy;     // CinePaint offsets
  gint32 cchan;   // CinePaint channels in layer
  gint32 echan;   // channels to write in exr
  char *cname;    // CinePaint layer name
  char **enames;  // exr channel names
  int dtype;      // data type 0 - half, 1 - IEEE float, 2 - int32
  GPixelRgn pixel_rgn;
  gfloat * cp_pixels;
  gfloat * pixels;
  guint16 * hcp_pixels;
  guint16 * hpixels;
} clayer_s;

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  int nlayers = 0;
  gint32 *layers = gimp_image_get_layers (image_ID, &nlayers);

  // start with CinePaint
  clayer_s * alayers = (clayer_s*) calloc (sizeof(clayer_s), nlayers);

  // parse the CinePaint image
  int all_echan = 0; // all channels
  int mcchan = 0;   // maximum channels in a CinePaint layer
  int tile_height = gimp_tile_height();
  Imf::PixelType etype = Imf::FLOAT;

  for( int l = 0; l < nlayers; ++l)
  {
      alayers[l].drawable_ID = layers[l];
      alayers[l].cchan = gimp_drawable_num_channels (alayers[l].drawable_ID);
      alayers[l].drawable_type = gimp_drawable_type( alayers[l].drawable_ID);
      alayers[l].drawable = gimp_drawable_get( alayers[l].drawable_ID);
      alayers[l].width = alayers[l].drawable->width;
      alayers[l].height = alayers[l].drawable->height;
      gimp_drawable_offsets( alayers[l].drawable_ID,
                             &alayers[l].dx, &alayers[l].dy );
      alayers[l].cname = gimp_layer_get_name( alayers[l].drawable_ID );

      switch (alayers[l].drawable_type) {
      case FLOAT16_GRAY_IMAGE:
           etype = Imf::HALF;
      case FLOAT_GRAY_IMAGE:
	  alayers[l].cchan = 1;
	  break;

      case FLOAT16_GRAYA_IMAGE:
           etype = Imf::HALF;
      case FLOAT_GRAYA_IMAGE:
	  alayers[l].cchan = 2;
	  break;

      case FLOAT16_RGB_IMAGE:
           etype = Imf::HALF;
      case FLOAT_RGB_IMAGE:
	  alayers[l].cchan = 3;
	  break;

      case FLOAT16_RGBA_IMAGE:
           etype = Imf::HALF;
      case FLOAT_RGBA_IMAGE:
	  alayers[l].cchan = 4;
	  break;

	  // support other types later

      default:
	g_message (_("Please convert the image to float before saving as OpenEXR\n"));
	return FALSE;
	break;
      }


      // geometry consistency check
      if( l > 0 &&
          alayers[l].width  != alayers[0].width ||
          alayers[l].height != alayers[0].height ||
          alayers[l].dx     != alayers[0].dx ||
          alayers[l].dy     != alayers[0].dy )
      {
        gimp_message(_("OpenEXR dont supports different layer geometries!"));
        return FALSE;
      }

      alayers[l].enames = (char**) calloc(sizeof(char*), alayers[l].cchan);
      for( int position = 0; position < alayers[l].cchan; ++position )
      {
        alayers[l].enames[position] = getChannelName( position,alayers[l].cname,
                                   nlayers, alayers[l].cchan, alayers[l].echan);
      }
      all_echan += alayers[l].echan;

      if(alayers[l].cchan > mcchan)
        mcchan = alayers[l].cchan;

      fprintf(stderr, "%s:%d %d:%d Writing %d|%d channels to %s\n",
              __FILE__,__LINE__, image_ID, alayers[l].drawable_ID,
              alayers[l].cchan,
              gimp_drawable_num_channels (alayers[l].drawable_ID), filename);
    
      gimp_pixel_rgn_init(&alayers[l].pixel_rgn, alayers[l].drawable, 0, 0, 
                          alayers[l].width, alayers[l].height, FALSE, FALSE);
  }

  int type_size = (etype == Imf::FLOAT) ? sizeof(float) : sizeof(guint16);
  int iwidth = gimp_image_width( image_ID );
  int iheight = gimp_image_height( image_ID );

  try
  {
    // allow PIZ compression - taken from shake plug-in
    Imf::Compression compress = Imf::PIZ_COMPRESSION;
    Imath::Box2i dataWindow (Imath::V2i (0, 0), Imath::V2i (alayers[0].width - 1, alayers[0].height - 1));
    Imath::Box2i displayWindow (Imath::V2i (-alayers[0].dx, -alayers[0].dy),
                                Imath::V2i (iwidth - alayers[0].dx - 1,
                                            iheight - alayers[0].dy - 1));

    Imf::Header header = Imf::Header(
             displayWindow, dataWindow,
             1, Imath::V2f(0,0), 1,
             Imf::INCREASING_Y,
             //Imf::DECREASING_Y,
             compress);
    header.insert ("software", Imf::StringAttribute (PLUGIN_COMPLETENAME));
    //Imf::Header header (width, height);

    if (gimp_image_has_icc_profile(image_ID, ICC_IMAGE_PROFILE))
    {
        int size;
        char *mem_profile=NULL;
        double g[3];

        mem_profile = gimp_image_get_icc_profile_by_mem(image_ID, &size,
                                                        ICC_IMAGE_PROFILE);
        Imf::Chromaticities xyY = writeEXRprims ( mem_profile,  size, g );

        // We test if we obtain gamma 1.0 encoded data and exit otherwise
        if((int)(g[0]*100+.5) != 100 ||
           (int)(g[1]*100+.5) != 100 ||
           (int)(g[2]*100+.5) != 100 )
        {
          char text[1024];
          sprintf(text,
                    _("Found non supported gamma of %.02f %.02f %.02f!\nYour ICC image profile needs a gamma of 1.0 ."),
                    g[0], g[1], g[2]);
          gimp_message( text );
          return FALSE;
        }

        addChromaticities(header, xyY);
        if (!hasChromaticities (header))
          g_message(_("Adding of colour primaries to EXR failed."));
    }

    Imf::FrameBuffer fb;

    for( int l = 0; l < nlayers; ++l)
    {
      // Set up temp buffers
      alayers[l].pixels = g_new(gfloat, alayers[l].width * alayers[l].cchan );
      alayers[l].cp_pixels = g_new(gfloat, tile_height * alayers[l].width *
                                           alayers[l].cchan );
      alayers[l].hpixels = (guint16*) alayers[l].pixels;
      alayers[l].hcp_pixels = (guint16*) alayers[l].cp_pixels;

      size_t xstride = type_size * alayers[l].cchan;
      size_t ystride = xstride * alayers[l].width;

      for( int position = 0; position < alayers[l].echan; ++position )
      {
#ifdef DEBUG
        fprintf(stderr, "%s:%d %d [%d:%d] Writing channel %s\n",
              __FILE__,__LINE__, image_ID, alayers[l].drawable_ID, position,
              alayers[l].enames[position] );
#endif
        // set all the channel names in the OpenEXR header
        header.channels().insert (alayers[l].enames[position],
                                  Imf::Channel (etype));

	// Write a bunch of scanlines at a time to save memory.  We do this by
	// setting the frame buffer y stride to 0 if using one scanline.
        fb.insert (alayers[l].enames[position],
                   Imf::Slice (etype, (char *) alayers[l].pixels
                               + position * type_size,
                               xstride, 0));
      }
    }

    Imf::OutputFile exr (filename, header);
    exr.setFrameBuffer (fb);

    char progress[PATH_MAX];
    if (strrchr (filename, '/') != NULL)
      snprintf (progress, PATH_MAX, _("Saving %s"),
                strrchr (filename, '/') + 1);
    else
      snprintf (progress, PATH_MAX, _("Saving %s"), filename);
    gimp_progress_init(progress);

    // collect CP's layer channels into the framebuffer
    int y = 0;
    while (y < alayers[0].height)
    {
      int start = y;
      int end = MIN(y + tile_height, alayers[0].height);
      int scanlines = end - start;

      for( int l = 0; l < nlayers; ++l)
      {
        gimp_pixel_rgn_get_rect(&alayers[l].pixel_rgn,
                                (guchar*) alayers[l].cp_pixels, 
                                0, start, alayers[l].width, scanlines);
      }

      for( int i = 0; i < scanlines; ++i)
      {
        for( int l = 0; l < nlayers; ++l)
        {
          size_t ystride = alayers[l].width * alayers[l].cchan;
          if(type_size == 2)
            memcpy( alayers[l].pixels, &alayers[l].hcp_pixels[ystride * i],
                    ystride * type_size);
          else
            memcpy( alayers[l].pixels, &alayers[l].cp_pixels[ystride * i],
                    ystride * type_size);
        }

        exr.writePixels (1);

        ++y;
        gimp_progress_update((double)y / (double)alayers[0].height);
      }
    }

    // clean up

    for( int l = 0; l < nlayers; ++l)
    {
      g_free (alayers[l].pixels);    alayers[l].pixels = NULL;
      free (alayers[l].cp_pixels);   alayers[l].cp_pixels = NULL;
    }
  }

  catch (Iex::BaseExc & e)
  {
	gimp_message( e.what() );

	return FALSE;
  }

  return TRUE;
}

char*
getChannelName    (int position, const char *layer_name, int nlayers,
                   int max_channels, int & exr_chan )
{
  if( position >= max_channels )
    printf("%s:%d %s() missmatch %d|%d\n",__FILE__,__LINE__,__func__,
           position, max_channels);

  // 1. test if the layer name contains hints about the original layout
  //    Z-buffer in a RGBA drawable
  // 2. match the found channel name to a requested position or generate a name
  // 3. test if the file is multi layered and extract the correct layer name
  // 4. merge the layername with the channel name and return it

  char *name = strdup(layer_name);

  exr_chan = max_channels;
  int points = 0;
  char ** names = (char**) calloc(sizeof(char*),8);
  
  // 1.
  for( int i = 0; i < max_channels; ++i)
  {
    char *ptr = strrchr(name,'.');
    if(ptr)
    {
      names[i] = strdup(ptr+1);
      if( strlen(names[i]) == 1 &&
          points == i )
      {
        ++points;
        *ptr = '\0';
      } else
        break;
    }
  }
  // copy the remainder into names[points]
  names[points] = strdup( name );
  free( name ); name = NULL;

  // 2.
  if(nlayers > 1 && points)
  {
    exr_chan = points;
    name = (char*)calloc(sizeof(char), 64);
    if(position < points)
      snprintf(name, 64, "%s.%s",names[points], names[position]);
    else
      return NULL;

  } else {

    switch(max_channels)
    {
      case 1:
      case 2:
        switch(position)
        {
          case 0: name = "Y"; break;
          case 1: name = "A"; break;
        }
        break;
      case 3:
      case 4:
        switch(position)
        {
          case 0: name = "R"; break;
          case 1: name = "G"; break;
          case 2: name = "B"; break;
          case 3: name = "A"; break;
        }
        break;
    }
  }

  // 3. and 4.
  if(nlayers > 1 && !points)
  {
    char *tmp = (char*)calloc(sizeof(char), 64);
    snprintf( tmp, 64, "%s.%s", layer_name, name);
    name = tmp;
  }

  return name;
}

