#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32

#else
#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "config.h"
#include "libgimp/gimpintl.h"
#include "../canvas.h"
#include "../floating_sel.h"
#include "../gimage.h"
#include "../gimage_mask.h"
#include "../rc.h"
#include "../interface.h"
#include "../paint_funcs_area.h"
#include "../palette.h"
#include "../pixelarea.h"
#include "../plug_in.h"
#include "../procedural_db.h"
#include "../tag.h"
#include "../tile_manager.h"
#include "../tile_swap.h"
#include "../xcf.h"
#include "../frac.h"

#include "../layer_pvt.h"
#include "../channel_pvt.h"
#include "../tile_manager_pvt.h"
#include "../spline.h"

#include "../../lib/wire/dl_list.h"

/* #define SWAP_FROM_FILE */


typedef enum
{
  PROP_END = 0,
  PROP_COLORMAP = 1,
  PROP_ACTIVE_LAYER = 2,
  PROP_ACTIVE_CHANNEL = 3,
  PROP_SELECTION = 4,
  PROP_FLOATING_SELECTION = 5,
  PROP_OPACITY = 6,
  PROP_MODE = 7,
  PROP_VISIBLE = 8,
  PROP_LINKED = 9,
  PROP_PRESERVE_TRANSPARENCY = 10,
  PROP_APPLY_MASK = 11,
  PROP_EDIT_MASK = 12,
  PROP_SHOW_MASK = 13,
  PROP_SHOW_MASKED = 14,
  PROP_OFFSETS = 15,
  PROP_COLOR = 16,
  PROP_COMPRESSION = 17,
  PROP_GUIDES = 18,
  PROP_SPLINES_START=19,
  PROP_SPLINES_CONTINUITY=20,
  PROP_SPLINES_KEYFRAME=21,
  PROP_SPLINES_ACTIVE=22,
  PROP_SPLINES_TYPE=23,
  PROP_SPLINES_DRAW_POINTS=24,
  PROP_SPLINES_POINTS_START=25,
  PROP_SPLINES_SAVE_POINT=26,
  PROP_SPLINES_POINTS_END=27,
  PROP_SPLINES_END=28
} PropType;

typedef enum
{
  COMPRESS_NONE = 0,
  COMPRESS_RLE = 1,
  COMPRESS_ZLIB = 2,
  COMPRESS_FRACTAL = 3
} CompressionType;

typedef GImage* XcfLoader(XcfInfo *info);

static Argument* xcf_load_invoker (Argument  *args);
static Argument* xcf_save_invoker (Argument  *args);

static gint xcf_save_image         (XcfInfo     *info,
				    GImage      *gimage);
static void xcf_save_choose_format (XcfInfo     *info,
				    GImage      *gimage);
static void xcf_save_image_props   (XcfInfo     *info,
				    GImage      *gimage);
static void xcf_save_layer_props   (XcfInfo     *info,
				    GImage      *gimage,
				    Layer       *layer);
static void xcf_save_channel_props (XcfInfo     *info,
				    GImage      *gimage,
				    Channel     *channel);
static void xcf_save_prop          (XcfInfo     *info,
				    PropType     prop_type,
				    ...);
static void xcf_save_color_prop    (XcfInfo *info, 
				    PixelRow *row);
static void xcf_save_layer         (XcfInfo     *info,
				    GImage      *gimage,
				    Layer       *layer);
static void xcf_save_channel       (XcfInfo     *info,
				    GImage      *gimage,
				    Channel     *channel);
static void xcf_save_hierarchy     (XcfInfo     *info,
				    Canvas *tiles);
static void xcf_save_row 	   (XcfInfo *info, 
				    PixelRow *row);

static GImage*  xcf_load_image         (XcfInfo     *info);
static gint     xcf_load_image_props   (XcfInfo     *info,
					GImage      *gimage);
static gint     xcf_load_layer_props   (XcfInfo     *info,
					GImage      *gimage,
					Layer       *layer);
static gint     xcf_load_channel_props (XcfInfo     *info,
					GImage      *gimage,
					Channel     *channel);
static gint     xcf_load_prop          (XcfInfo     *info,
					PropType    *prop_type,
					guint32     *prop_size);
static Layer*   xcf_load_layer         (XcfInfo     *info,
					GImage      *gimage);
static Channel* xcf_load_channel       (XcfInfo     *info,
					GImage      *gimage);
static LayerMask* xcf_load_layer_mask  (XcfInfo     *info,
					GImage      *gimage);
static gint     xcf_load_hierarchy     (XcfInfo     *info,
					Canvas *tiles);
static void xcf_load_row 	   (XcfInfo *info, 
				    PixelRow *row);
#ifdef SWAP_FROM_FILE
static int      xcf_swap_func          (int          fd,
					Tile        *tile,
					int          cmd,
					gpointer     user_data);
#endif


static void xcf_seek_pos (XcfInfo *info,
			  guint    pos);
static void xcf_seek_end (XcfInfo *info);

static guint xcf_read_float   (FILE     *fp,
			       gfloat  *data,
			       gint      count);
static guint xcf_read_int32   (FILE     *fp,
			       guint32  *data,
			       gint      count);
static guint xcf_read_int16    (FILE     *fp,
			       guint16   *data,
			       gint      count);
static guint xcf_read_int8    (FILE     *fp,
			       guint8   *data,
			       gint      count);
static guint xcf_read_string  (FILE     *fp,
			       gchar   **data,
			       gint      count);
static guint xcf_write_float  (FILE     *fp,
			       gfloat   *data,
			       gint      count);
static guint xcf_write_int32  (FILE     *fp,
			       guint32  *data,
			       gint      count);
static guint xcf_write_int16   (FILE     *fp,
			       guint16   *data,
			       gint      count);
static guint xcf_write_int8   (FILE     *fp,
			       guint8   *data,
			       gint      count);
static guint xcf_write_string (FILE     *fp,
			       gchar   **data,
			       gint      count);

Spline *xcf_spline=NULL;

static ProcArg xcf_load_args[] =
{
  { PDB_INT32,
    "dummy_param",
    "dummy parameter" },
  { PDB_STRING,
    "filename",
    "The name of the file to load" },
  { PDB_STRING,
    "raw_filename",
    "The name of the file to load" },
};

static ProcArg xcf_load_return_vals[] =
{
  { PDB_IMAGE,
    "image",
    "Output image" },
};

static PlugInProcDef xcf_plug_in_load_proc =
{
  "gimp_xcf_load",
  "<Load>/xcf",
  NULL,
  "xcf",
  "",
  "0,string,gimp\\040xcf\\040",
  NULL, /* ignored for load */
  0,    /* ignored for load */
  {
    "gimp_xcf_load",
    "loads file saved in the .xcf file format",
    "The xcf file format has been designed specifically for loading and saving tiled and layered images in the GIMP. This procedure will load the specified file.",
    "Spencer Kimball & Peter Mattis",
    "Spencer Kimball & Peter Mattis",
    "1995-1996",
    PDB_INTERNAL,
    3,
    xcf_load_args,
    1,
    xcf_load_return_vals,
    { { xcf_load_invoker } },
  },
  NULL, /* fill me in at runtime */
  NULL, /* fill me in at runtime */
  NULL
};

static ProcArg xcf_save_args[] =
{
  { PDB_INT32,
    "dummy_param",
    "dummy parameter" },
  { PDB_IMAGE,
    "image",
    "Input image" },
  { PDB_DRAWABLE,
    "drawable",
    "Active drawable of input image" },
  { PDB_STRING,
    "filename",
    "The name of the file to save the image in" },
  { PDB_STRING,
    "raw_filename",
    "The name of the file to load" },
};

static PlugInProcDef xcf_plug_in_save_proc =
{
  "gimp_xcf_save",
  "<Save>/xcf",
  NULL,
  "xcf",
  "",
  NULL,
  "RGB*, GRAY*, INDEXED*, U16_RGB*, U16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
  0, /* fill me in at runtime */
  {
    "gimp_xcf_save",
    "saves file in the .xcf file format",
    "The xcf file format has been designed specifically for loading and saving tiled and layered images in the GIMP. This procedure will save the specified image in the xcf file format.",
    "Spencer Kimball & Peter Mattis",
    "Spencer Kimball & Peter Mattis",
    "1995-1996",
    PDB_INTERNAL,
    5,
    xcf_save_args,
    0,
    NULL,
    { { xcf_save_invoker } },
  },
  NULL, /* fill me in at runtime */
  NULL /* fill me in at runtime */
};

static XcfLoader* xcf_loaders[] =
{
  xcf_load_image,			/* version 0 */
  xcf_load_image			/* version 1 */
};

static int N_xcf_loaders=(sizeof(xcf_loaders)/sizeof(xcf_loaders[0]));

void
xcf_init ()
{
  /* So this is sort of a hack, but its better than it was before.  To do this
   * right there would be a file load-save handler type and the whole interface
   * would change but there isn't, and currently the plug-in structure contains
   * all the load-save info, so it makes sense to use that for the XCF load/save
   * handlers, even though they are internal.  The only thing it requires is
   * using a PlugInProcDef struct.  -josh */
  procedural_db_register (&xcf_plug_in_save_proc.db_info);
  procedural_db_register (&xcf_plug_in_load_proc.db_info);
  xcf_plug_in_save_proc.image_types_val = plug_in_image_types_parse (xcf_plug_in_save_proc.image_types);
  xcf_plug_in_load_proc.image_types_val = plug_in_image_types_parse (xcf_plug_in_load_proc.image_types);
  plug_in_add_internal (&xcf_plug_in_save_proc);
  plug_in_add_internal (&xcf_plug_in_load_proc);
}

static Argument*
xcf_load_invoker (Argument *args)
{
  XcfInfo info;
  Argument *return_args;
  GImage *gimage;
  char *filename;
  int success;
  char id[14];

  gimage = NULL;

  success = FALSE;

  filename = args[1].value.pdb_pointer;

  info.fp = fopen (filename, "rb");
  if (info.fp)
    {
      info.cp = 0;
      info.filename = filename;
      info.active_layer = 0;
      info.active_channel = 0;
      info.floating_sel_drawable = NULL;
      info.floating_sel = NULL;
      info.floating_sel_offset = 0;
      info.swap_num = 0;
      info.ref_count = NULL;
      info.compression = COMPRESS_NONE;

      success = TRUE;
      info.cp += xcf_read_int8 (info.fp, (guint8*) id, 14);
      if (strncmp (id, "gimp xcf ", 9) != 0)
	success = FALSE;
      else if (strcmp (id+9, "file") == 0) 
	{
	  info.file_version = 0;
	} 
      else if (id[9] == 'v') 
	{
	  info.file_version = atoi(id + 10);
	}
      else 
	success = FALSE;
      
      if (success)
	{
	  if (info.file_version < N_xcf_loaders)
	    {
#         ifdef DEBUG
          printf("%s:%d %s() fileversion: %d\n",__FILE__,__LINE__,__func__, info.file_version);
#         endif
	      gimage = 0; /*(*(xcf_loaders[info.file_version])) (&info);*/
              g_message(_("This XCF file is not supported by CinePaint. You may convert to tiff and try again."));
	      if (!gimage)
		success = FALSE;
	    }
	  else if (info.file_version == 100)  /* a r & h version */
	     {
		gimage = xcf_load_image (&info);
	      if (!gimage)
		success = FALSE;
	     } 
	  else
	    {
	      g_message (_("XCF error: unsupported XCF file version %d encountered"), info.file_version);
	      success = FALSE;
	    }
	}
      fclose (info.fp);
    }
  
  return_args = procedural_db_return_args (&xcf_plug_in_load_proc.db_info, success);

  if (success)
    return_args[1].value.pdb_int = gimage->ID;

  return return_args;
}

static Argument*
xcf_save_invoker (Argument *args)
{
  XcfInfo info;
  Argument *return_args;
  GImage *gimage;
  char *filename;
  int success;

  success = FALSE;

  gimage = gimage_get_ID (args[1].value.pdb_int);
  filename = args[3].value.pdb_pointer;

  info.fp = fopen (filename, "wb");
  if (info.fp)
    {
      info.cp = 0;
      info.filename = filename;
      info.active_layer = 0;
      info.active_channel = 0;
      info.floating_sel_drawable = NULL;
      info.floating_sel = NULL;
      info.floating_sel_offset = 0;
      info.swap_num = 0;
      info.ref_count = NULL;
      info.compression = COMPRESS_NONE;

      xcf_save_choose_format (&info, gimage);

      success = xcf_save_image (&info, gimage);
      fclose (info.fp);
    }
  else
    {
      g_message (_("open failed on %s: %s\n"), filename, g_strerror(errno));
    }

  return_args = procedural_db_return_args (&xcf_plug_in_save_proc.db_info, success);

  return return_args;
}

static void
xcf_save_choose_format (XcfInfo *info,
			GImage  *gimage)
{
  int save_version = 0;			/* default to oldest */

  if (gimage->cmap) 
    save_version = 1;			/* need version 1 for colormaps */
  
  save_version = 100;                   /* version 100 -- first r & h version */ 

  info->file_version = save_version;
}

static gint
xcf_save_image (XcfInfo *info,
		GImage  *gimage)
{
  Layer *layer;
  Layer *floating_layer;
  Channel *channel;
  guint32 saved_pos;
  guint32 offset;
  guint nlayers;
  guint nchannels;
  GSList *list;
  int have_selection;
  int t1, t2, t3, t4;
  char version_tag[14];
  gint width = gimage->width;
  gint height = gimage->height;
  Tag tag = gimage->tag;

  floating_layer = gimage_floating_sel (gimage);
  if (floating_layer)
    floating_sel_relax (floating_layer, FALSE);

  /* write out the tag information for the image */
  if (info->file_version > 0) 
    {
      sprintf (version_tag, "gimp xcf v%03d", info->file_version);
    } 
  else 
    {
      strcpy (version_tag, "gimp xcf file");
    }
  info->cp += xcf_write_int8 (info->fp, (guint8*) version_tag, 14);

  /* write out the width, height and tag information for the image */

  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &tag, 1);

  /* determine the number of layers and channels in the image */
  nlayers = (guint) g_slist_length (gimage->layers);
  nchannels = (guint) g_slist_length (gimage->channels);

  /* check and see if we have to save out the selection */
  have_selection = gimage_mask_bounds (gimage, &t1, &t2, &t3, &t4);
  if (have_selection)
    nchannels += 1;

  /* write the property information for the image.
   */
  xcf_save_image_props (info, gimage);

  /* save the current file position as it is the start of where
   *  we place the layer offset information.
   */
  saved_pos = info->cp;

  /* Initialize the fractal compression saving routines
   */
  if (info->compression == COMPRESS_FRACTAL)
    xcf_save_compress_frac_init (2, 2);

  /* seek to after the offset lists */
  xcf_seek_pos (info, info->cp + (nlayers + nchannels + 2) * 4);

  list = gimage->layers;
  while (list)
    {
      layer = list->data;
      list = list->next;

      /* save the start offset of where we are writing
       *  out the next layer.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_save_layer (info, gimage, layer);

      /* seek back to where we are to write out the next
       *  layer offset and write it out.
       */
      xcf_seek_pos (info, saved_pos);
      info->cp += xcf_write_int32 (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next layer.
       */
      xcf_seek_end (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the layer offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;
  xcf_seek_end (info);


  list = gimage->channels;
  while (list || have_selection)
    {
      if (list)
	{
	  channel = list->data;
	  list = list->next;
	}
      else
	{
	  channel = gimage->selection_mask;
	  have_selection = FALSE;
	}

      /* save the start offset of where we are writing
       *  out the next channel.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_save_channel (info, gimage, channel);

      /* seek back to where we are to write out the next
       *  channel offset and write it out.
       */
      xcf_seek_pos (info, saved_pos);
      info->cp += xcf_write_int32 (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next channel.
       */
      xcf_seek_end (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the channel offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;

  if (floating_layer)
    floating_sel_rigor (floating_layer, FALSE);

  return !ferror(info->fp);
}

static void
xcf_save_image_props (XcfInfo *info,
		      GImage  *gimage)
{
  /* check and see if we should save the colormap property */
  if (gimage->cmap)
    xcf_save_prop (info, PROP_COLORMAP, gimage->num_cols, gimage->cmap);

/*  if (info->compression != COMPRESS_NONE) */
  xcf_save_prop (info, PROP_COMPRESSION, info->compression);

  if (gimage->guides)
    xcf_save_prop (info, PROP_GUIDES, gimage->guides);

  xcf_save_prop (info, PROP_END);
}

static void
xcf_save_layer_props (XcfInfo *info,
		      GImage  *gimage,
		      Layer   *layer)
{
  Spline *spline;
  SplinePoint *point;

  if (layer == gimage->active_layer)
    xcf_save_prop (info, PROP_ACTIVE_LAYER);

  if (layer == gimage_floating_sel (gimage))
    {
      info->floating_sel_drawable = layer->fs.drawable;
      xcf_save_prop (info, PROP_FLOATING_SELECTION);
    }

  xcf_save_prop (info, PROP_OPACITY, layer->opacity);
  xcf_save_prop (info, PROP_VISIBLE, GIMP_DRAWABLE(layer)->visible);
  xcf_save_prop (info, PROP_LINKED, layer->linked);
  xcf_save_prop (info, PROP_PRESERVE_TRANSPARENCY, layer->preserve_trans);
  xcf_save_prop (info, PROP_APPLY_MASK, layer->apply_mask);
  xcf_save_prop (info, PROP_EDIT_MASK, layer->edit_mask);
  xcf_save_prop (info, PROP_SHOW_MASK, layer->show_mask);
  xcf_save_prop (info, PROP_OFFSETS, GIMP_DRAWABLE(layer)->offset_x, GIMP_DRAWABLE(layer)->offset_y);
  xcf_save_prop (info, PROP_MODE, layer->mode);
  if(!DL_is_empty(layer->sl.splines))
    {
      spline=(Spline*)layer->sl.splines->head;
      while(spline)
	{
	  xcf_save_prop(info, PROP_SPLINES_START, 1);
	  xcf_save_prop(info, PROP_SPLINES_CONTINUITY, spline->continuity);
	  xcf_save_prop(info, PROP_SPLINES_KEYFRAME, spline->keyframe);
	  xcf_save_prop(info, PROP_SPLINES_ACTIVE, spline->active);
	  xcf_save_prop(info, PROP_SPLINES_TYPE, spline->type);
	  xcf_save_prop(info, PROP_SPLINES_DRAW_POINTS, spline->draw_points);
	  xcf_save_prop(info, PROP_SPLINES_POINTS_START, 1);
	  point=NULL;
	  if(!DL_is_empty(spline->points))
	    point=(SplinePoint*)spline->points->head;
	  xcf_save_prop(info, PROP_SPLINES_SAVE_POINT, point);
	  xcf_save_prop(info, PROP_SPLINES_POINTS_END, 1);
	  xcf_save_prop(info, PROP_SPLINES_END, 1);
	  spline=(Spline*)((DL_node*)spline)->next;
	}
    }
  xcf_save_prop (info, PROP_END);
}

static void
xcf_save_channel_props (XcfInfo *info,
			GImage  *gimage,
			Channel *channel)
{
  if (channel == gimage->active_channel)
    xcf_save_prop (info, PROP_ACTIVE_CHANNEL);

  if (channel == gimage->selection_mask)
    xcf_save_prop (info, PROP_SELECTION);

  xcf_save_prop (info, PROP_OPACITY, channel->opacity);
  xcf_save_prop (info, PROP_VISIBLE, GIMP_DRAWABLE(channel)->visible);
  xcf_save_prop (info, PROP_SHOW_MASKED, channel->show_masked);
  xcf_save_color_prop (info, &channel->col);
  xcf_save_prop (info, PROP_END);
}

static void
xcf_save_color_prop (XcfInfo *info, 
		     PixelRow *row)
{
  guint32 prop_type = PROP_COLOR;
  Tag t = pixelrow_tag (row);
  guint32 size = tag_bytes(t) + 8;   /*for tag , width , rgb data*/ 

  info->cp += xcf_write_int32 (info->fp, &prop_type, 1);
  info->cp += xcf_write_int32 (info->fp, &size, 1);

  xcf_save_row (info, row);
}

static void
xcf_save_prop (XcfInfo  *info,
	       PropType  prop_type,
	       ...)
{
  guint32 size, type;
  va_list args;

  va_start (args, prop_type);

  type = prop_type;
  switch (prop_type)
    {
    case PROP_COLOR:
       g_warning("xcf_save_prop -- bad prop_type PROP_COLOR");
       break;
    case PROP_END:
      size = 0;

      info->cp += xcf_write_int32 (info->fp, &type, 1);
      info->cp += xcf_write_int32 (info->fp, &size, 1);
      break;
    case PROP_COLORMAP:
      {
	guint32 ncolors;
	guchar *colors;

	ncolors = va_arg (args, guint32);
	colors = va_arg (args, guchar*);
	size = 4 + ncolors;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &ncolors, 1);
	info->cp += xcf_write_int8 (info->fp, colors, ncolors * 3);
      }
      break;
    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTION:
      size = 0;

      info->cp += xcf_write_int32 (info->fp, &type, 1);
      info->cp += xcf_write_int32 (info->fp, &size, 1);
      break;
    case PROP_FLOATING_SELECTION:
      {
	guint32 dummy;

	dummy = 0;
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->floating_sel_offset = info->cp;
	info->cp += xcf_write_int32 (info->fp, &dummy, 1);
      }
      break;
    case PROP_OPACITY:
      {
	gfloat opacity;
	opacity = va_arg (args, double);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_float (info->fp, (gfloat*) &opacity, 1);
      }
      break;
    case PROP_MODE:
      {
	gint32 mode;

	mode = va_arg (args, gint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, (guint32*) &mode, 1);
      }
      break;
    case PROP_VISIBLE:
      {
	guint32 visible;

	visible = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &visible, 1);
      }
      break;
    case PROP_LINKED:
      {
	guint32 linked;

	linked = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &linked, 1);
      }
      break;
    case PROP_PRESERVE_TRANSPARENCY:
      {
	guint32 preserve_trans;

	preserve_trans = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &preserve_trans, 1);
      }
      break;
    case PROP_APPLY_MASK:
      {
	guint32 apply_mask;

	apply_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &apply_mask, 1);
      }
      break;
    case PROP_EDIT_MASK:
      {
	guint32 edit_mask;

	edit_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &edit_mask, 1);
      }
      break;
    case PROP_SHOW_MASK:
      {
	guint32 show_mask;

	show_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &show_mask, 1);
      }
      break;
    case PROP_SHOW_MASKED:
      {
	guint32 show_masked;

	show_masked = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &show_masked, 1);
      }
      break;
    case PROP_OFFSETS:
      {
	gint32 offsets[2];

	offsets[0] = va_arg (args, gint32);
	offsets[1] = va_arg (args, gint32);
	size = 8;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, (guint32*) offsets, 2);
      }
      break;
    case PROP_COMPRESSION:
      {
	guint8 compression;

	compression = (guint8) va_arg (args, guint32);
	size = 1;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int8 (info->fp, &compression, 1);
      }
      break;
    case PROP_GUIDES:
      {
	GList *guides;
	Guide *guide;
	gint32 position;
	gint8 orientation;
	int nguides;

	guides = va_arg (args, GList*);
	nguides = g_list_length (guides);

	size = nguides * (4 + 1);

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);

	while (guides)
	  {
	    guide = guides->data;
	    guides = guides->next;

	    position = guide->position;
	    orientation = guide->orientation;

	    info->cp += xcf_write_int32 (info->fp, (guint32*) &position, 1);
	    info->cp += xcf_write_int8 (info->fp, (guint8*) &orientation, 1);
	  }
      }
      break;
    case PROP_SPLINES_START:
      {
	gint32 has_spline;
	has_spline=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&has_spline, 1);
      }
      break;
    case PROP_SPLINES_CONTINUITY:
      {
	gint32 continuity;
	continuity=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&continuity, 1);
      }
      break;
    case PROP_SPLINES_KEYFRAME:
      {
	gint32 keyframe;
	keyframe=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&keyframe, 1);
      }
      break;
    case PROP_SPLINES_ACTIVE:
      {
	gint32 active;
	active=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&active, 1);
      }
      break;
    case PROP_SPLINES_TYPE:
      {
	gint32 type;
	type=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&type, 1);
      }
      break;
    case PROP_SPLINES_DRAW_POINTS:
      {
	gint32 draw_points;
	draw_points=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&draw_points, 1);
      }
      break;
    case PROP_SPLINES_POINTS_START:
      break;
    case PROP_SPLINES_SAVE_POINT:
      {
	SplinePoint *point, *start_point;
	gint32 x, y, cx1, cx2, cy1, cy2;
	gint32 number_points=0;

	start_point=va_arg(args, SplinePoint*);
	point=start_point;
	while(point)
	  {
	    number_points++;
	    point=(SplinePoint*)((DL_node*)point)->next;
	  }
	point=start_point;
	size=28*number_points+4;

	info->cp += xcf_write_int32 (info->fp, &type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &number_points, 1);

	while (point)
	  {
	    x=point->x;
	    y=point->y;
	    cx1=point->cx1;
	    cy1=point->cy1;
	    cx2=point->cx2;
	    cy2=point->cy2;
	    info->cp += xcf_write_int32(info->fp, (guint32*) &x, 1);
	    info->cp += xcf_write_int32(info->fp, (guint32*) &y, 1);
	    info->cp += xcf_write_int32(info->fp, (guint32*) &cx1, 1);
	    info->cp += xcf_write_int32(info->fp, (guint32*) &cy1, 1);
	    info->cp += xcf_write_int32(info->fp, (guint32*) &cx2, 1);
	    info->cp += xcf_write_int32(info->fp, (guint32*) &cy2, 1);
	    point=(SplinePoint*)((DL_node*)point)->next;
	  }
      }
      break;
    case PROP_SPLINES_POINTS_END:
      break;
    case PROP_SPLINES_END:
      {
	gint32 has_spline;
	has_spline=va_arg(args, gint32);
	size=4;
	info->cp += xcf_write_int32(info->fp, &type, 1);
	info->cp += xcf_write_int32(info->fp, &size, 1);
	info->cp += xcf_write_int32(info->fp, (guint32*)&has_spline, 1);
      }
      break;
    }

  va_end (args);
}

static void
xcf_save_layer (XcfInfo *info,
		GImage  *gimage,
		Layer   *layer)
{
  guint32 saved_pos;
  guint32 offset;
  gint width = drawable_width (GIMP_DRAWABLE(layer));
  gint height = drawable_height (GIMP_DRAWABLE(layer));
  gint tag = drawable_tag (GIMP_DRAWABLE(layer));
  gchar * name = GIMP_DRAWABLE(layer)->name;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE(layer) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_seek_pos (info, info->floating_sel_offset);
      info->cp += xcf_write_int32 (info->fp, &saved_pos, 1);
      xcf_seek_pos (info, saved_pos);
    }

  /* write out the width, height and image type information for the layer */
  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1); 
  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &tag, 1); 

  if (info->compression == COMPRESS_FRACTAL)
    /* xcf_compress_frac_info (GIMP_DRAWABLE(layer)->type); */;

  /* write out the layers name */
  info->cp += xcf_write_string (info->fp, &name, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, gimage, layer);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the layer tile hierarchy */
  xcf_seek_pos (info, info->cp + 8);
  offset = info->cp;

  xcf_save_hierarchy (info, GIMP_DRAWABLE(layer)->tiles);

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;

  /* write out the layer mask */
  if (layer->mask)
    {
      xcf_seek_end (info);
      offset = info->cp;

      xcf_save_channel (info, gimage, GIMP_CHANNEL(layer->mask));
    }
  else
    offset = 0;

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
}

static void
xcf_save_channel (XcfInfo *info,
		  GImage  *gimage,
		  Channel *channel)
{
  guint32 saved_pos;
  guint32 offset;
  gint width = drawable_width (GIMP_DRAWABLE(channel));
  gint height = drawable_height (GIMP_DRAWABLE(channel));
  gchar * name = GIMP_DRAWABLE(channel)->name;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE(channel) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_seek_pos (info, info->floating_sel_offset);
      info->cp += xcf_write_int32 (info->fp, (guint32*) &info->cp, 1);
      xcf_seek_pos (info, saved_pos);
    }

  /* write out the width and height information for the channel */
  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1); 
  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);

  /* write out the channels name */
  info->cp += xcf_write_string (info->fp, &name, 1);

  /* write out the channel properties */
  xcf_save_channel_props (info, gimage, channel);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the channel tile hierarchy */
  xcf_seek_pos (info, info->cp + 4);
  offset = info->cp;

  xcf_save_hierarchy (info, GIMP_DRAWABLE(channel)->tiles);

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;
}

static void
xcf_save_hierarchy (XcfInfo     *info,
		    Canvas *tiles)
{
  gint w = canvas_width (tiles);
  gint h = canvas_height (tiles);
  Tag tag = canvas_tag (tiles);
  int bytes = canvas_bytes (tiles);
  StorageType storage = canvas_storage (tiles);
  guint32 stora = storage;
  AutoAlloc auto_alloc = canvas_autoalloc (tiles);
  guint32 auto_all = auto_alloc;

  info->cp += xcf_write_int32 (info->fp, (guint32*) &w, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &h, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &tag, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &bytes, 1);
  info->cp += xcf_write_int32 (info->fp, &stora, 1);
  info->cp += xcf_write_int32 (info->fp, &auto_all, 1);

  {
    PixelArea area;
    void * pag;

    pixelarea_init (&area, tiles, 0,0, w, h, TRUE); 
    for (pag = pixelarea_register (1, &area);
	 pag != NULL;
	 pag = pixelarea_process (pag))
      {
	PixelRow row;
	gint height = pixelarea_height (&area);
	while (height--)
	  {
	    pixelarea_getdata (&area, &row, height);
	    xcf_save_row ( info, &row);  
	  }
      }
  }
}

static void
xcf_save_row (XcfInfo *info,
	       PixelRow *row
		)
{
  Tag t = pixelrow_tag (row);
  Precision p = tag_precision (t);
  gint w = pixelrow_width (row);
  gint num_channels = tag_num_channels (t);

  /* write out the row tag and width*/
  info->cp += xcf_write_int32 (info->fp, (guint32*)&t, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*)&w, 1);
  
  /* write out the data for the row */
  switch (p) 
    {
    case PRECISION_U8:
      {
	guint8 * data = (guint8*)pixelrow_data (row);
	info->cp += xcf_write_int8 (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_BFP:
    case PRECISION_U16:
    case PRECISION_FLOAT16:
      {
	guint16 * data = (guint16*)pixelrow_data (row);
	info->cp += xcf_write_int16 (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_FLOAT:
      {
	gfloat * data = (gfloat*)pixelrow_data (row);
	info->cp += xcf_write_float (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_NONE:
    default:
      g_print ("xcf_save_row: unrecognized precision\n");	
      break;
    }
}

static GImage*
xcf_load_image (XcfInfo *info)
{
  GImage *gimage;
  Layer *layer;
  Channel *channel;
  guint32 saved_pos;
  guint32 offset;
  int width;
  int height;
  Tag tag;

  /* read in the image width, height and type */
  info->cp += xcf_read_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &tag, 1);

  /* create a new gimage */

  gimage = gimage_new (width, height, tag);
  if (!gimage)
    return NULL;

  /* read the image properties */
  if (!xcf_load_image_props (info, gimage))
    goto error;

  if (info->compression == COMPRESS_FRACTAL)
    xcf_load_compress_frac_init (1, 2);

  while (1)
    {
      /* read in the offset of the next layer */
      info->cp += xcf_read_int32 (info->fp, &offset, 1);

      /* if the offset is 0 then we are at the end
       *  of the layer list.
       */
      if (offset == 0)
	break;

      /* save the current position as it is where the
       *  next layer offset is stored.
       */
      saved_pos = info->cp;

      /* seek to the layer offset */
      xcf_seek_pos (info, offset);

      /* read in the layer */
      layer = xcf_load_layer (info, gimage);
      if (!layer)
	goto error;

      if (info->compression == COMPRESS_FRACTAL)
	/* xcf_compress_frac_info (GIMP_DRAWABLE(layer)->type); */;

      /* add the layer to the image if its not the floating selection */
      if (layer != info->floating_sel)
	gimage_add_layer (gimage, layer, g_slist_length (gimage->layers));

      /* restore the saved position so we'll be ready to
       *  read the next offset.
       */
      xcf_seek_pos (info, saved_pos);
    }

  while (1)
    {
      /* read in the offset of the next channel */
      info->cp += xcf_read_int32 (info->fp, &offset, 1);

      /* if the offset is 0 then we are at the end
       *  of the channel list.
       */
      if (offset == 0)
	break;

      /* save the current position as it is where the
       *  next channel offset is stored.
       */
      saved_pos = info->cp;

      /* seek to the channel offset */
      xcf_seek_pos (info, offset);

      /* read in the layer */
      channel = xcf_load_channel (info, gimage);
      if (!channel)
	goto error;

      /* add the channel to the image if its not the selection */
      if (channel != gimage->selection_mask)
	gimage_add_channel (gimage, channel, -1);

      /* restore the saved position so we'll be ready to
       *  read the next offset.
       */
      xcf_seek_pos (info, saved_pos);
    }

  if (info->active_layer)
    gimage_set_active_layer (gimage, info->active_layer);

  if (info->active_channel)
    gimage_set_active_channel (gimage, info->active_channel);

  gimage_set_filename (gimage, info->filename);

  return gimage;

error:
  gimage_delete (gimage);
  return NULL;
}

static gint
xcf_load_image_props (XcfInfo *info,
		      GImage  *gimage)
{
  PropType prop_type;
  guint32 prop_size;

  while (1)
    {
      if (!xcf_load_prop (info, &prop_type, &prop_size))
	return FALSE;

      switch (prop_type)
	{
	case PROP_END:
	  return TRUE;
	case PROP_COLORMAP:
	  if (info->file_version == 0) 
	    {
	      int i;
	      g_message (_("XCF warning: version 0 of XCF file format\n"
			 "did not save indexed colormaps correctly.\n"
			 "Substituting grayscale map."));
	      info->cp += xcf_read_int32 (info->fp, (guint32*) &gimage->num_cols, 1);
	      gimage->cmap = g_new (guchar, gimage->num_cols*3);
	      xcf_seek_pos (info, info->cp + gimage->num_cols);
	      for (i = 0; i<gimage->num_cols; i++) 
		{
		  gimage->cmap[i*3+0] = i;
		  gimage->cmap[i*3+1] = i;
		  gimage->cmap[i*3+2] = i;
		}
	    }
	  else 
	    {
	      info->cp += xcf_read_int32 (info->fp, (guint32*) &gimage->num_cols, 1);
	      gimage->cmap = g_new (guchar, gimage->num_cols*3);
	      info->cp += xcf_read_int8 (info->fp, (guint8*) gimage->cmap, gimage->num_cols*3);
	    }
	  break;
	case PROP_COMPRESSION:
	  {
	    guint8 compression;

	    info->cp += xcf_read_int8 (info->fp, (guint8*) &compression, 1);

	    if ((compression != COMPRESS_NONE) &&
		(compression != COMPRESS_RLE) &&
		(compression != COMPRESS_ZLIB) &&
		(compression != COMPRESS_FRACTAL))
	      {
		g_message (_("unknown compression type: %d"), (int) compression);
		return FALSE;
	      }

	    info->compression = compression;
	  }
	  break;
	case PROP_GUIDES:
	  {
	    Guide *guide;
	    gint32 position;
	    gint8 orientation;
	    int i, nguides;

	    nguides = prop_size / (4 + 1);
	    for (i = 0; i < nguides; i++)
	      {
		info->cp += xcf_read_int32 (info->fp, (guint32*) &position, 1);
		info->cp += xcf_read_int8 (info->fp, (guint8*) &orientation, 1);

		guide = g_new (Guide, 1);
		guide->position = position;
		guide->orientation = orientation;

		gimage->guides = g_list_prepend (gimage->guides, guide);
	      }

	    gimage->guides = g_list_reverse (gimage->guides);
	  }
	  break;
	default:
	  g_message (_("unexpected/unknown image property: %d (skipping)"), prop_type);

	  {
	    guint8 buf[16];
	    guint amount;

	    while (prop_size > 0)
	      {
		amount = MIN (16, prop_size);
		info->cp += xcf_read_int8 (info->fp, buf, amount);
		prop_size -= MIN (16, amount);
	      }
	  }
	  break;
	}
    }

  return FALSE;
}

static gint
xcf_load_layer_props (XcfInfo *info,
		      GImage  *gimage,
		      Layer   *layer)
{
  PropType prop_type;
  guint32 prop_size, temp;

  while (1)
    {
      if (!xcf_load_prop (info, &prop_type, &prop_size))
	return FALSE;

      switch (prop_type)
	{
	case PROP_END:
	  return TRUE;
	case PROP_ACTIVE_LAYER:
	  info->active_layer = layer;
	  break;
	case PROP_FLOATING_SELECTION:
	  info->floating_sel = layer;
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &info->floating_sel_offset, 1);
	  break;
	case PROP_OPACITY:
	  info->cp += xcf_read_float (info->fp, (gfloat*) &layer->opacity, 1);
	  break;
	case PROP_VISIBLE:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->visible, 1);
	  break;
	case PROP_LINKED:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->linked, 1);
	  break;
	case PROP_PRESERVE_TRANSPARENCY:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->preserve_trans, 1);
	  break;
	case PROP_APPLY_MASK:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->apply_mask, 1);
	  break;
	case PROP_EDIT_MASK:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->edit_mask, 1);
	  break;
	case PROP_SHOW_MASK:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->show_mask, 1);
	  break;
	case PROP_OFFSETS:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->offset_x, 1);
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->offset_y, 1);
	  break;
	case PROP_MODE:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &layer->mode, 1);
	  break;
	case PROP_SPLINES_START:
	  if(!xcf_spline)
	    {
	      info->cp += xcf_read_int32 (info->fp, (guint32*)&temp, 1);
	      xcf_spline=(Spline*)malloc(sizeof(Spline));
	      DL_init((DL_node*)xcf_spline);
	      xcf_spline->points=NULL;
	      DL_append(layer->sl.splines, (DL_node*)xcf_spline);
	    }
	  else
	    {
	      g_message(_("there is already a spline being init'd."));
	    }
	  break;
	case PROP_SPLINES_CONTINUITY:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&xcf_spline->continuity, 1);
	  break;
	case PROP_SPLINES_KEYFRAME:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&xcf_spline->keyframe, 1);
	  break;
	case PROP_SPLINES_ACTIVE:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&xcf_spline->active, 1);
	  break;
	case PROP_SPLINES_TYPE:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&xcf_spline->type, 1);
	  break;
	case PROP_SPLINES_DRAW_POINTS:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&xcf_spline->draw_points, 1);
	  break;
	case PROP_SPLINES_POINTS_START:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&temp, 1);
	  break;
	case PROP_SPLINES_SAVE_POINT:
	  {
	    SplinePoint *point;
	    gint32 x, y, cx1, cx2, cy1, cy2, number_points;
	    int i;

	    info->cp += xcf_read_int32(info->fp, (guint32*)&number_points, 1);
	    xcf_spline->points=(DL_list*)malloc(sizeof(DL_list));
	    DL_init((DL_node*)xcf_spline->points);
	    for (i = 0; i < number_points; i++)
	      {
		info->cp += xcf_read_int32(info->fp, (guint32*)&x, 1);
		info->cp += xcf_read_int32(info->fp, (guint32*)&y, 1);
		info->cp += xcf_read_int32(info->fp, (guint32*)&cx1, 1);
		info->cp += xcf_read_int32(info->fp, (guint32*)&cy1, 1);
		info->cp += xcf_read_int32(info->fp, (guint32*)&cx2, 1);
		info->cp += xcf_read_int32(info->fp, (guint32*)&cy2, 1);
		point=(SplinePoint*)malloc(sizeof(SplinePoint));
		point->x=x;
		point->y=y;
		point->cx1=cx1;
		point->cy1=cy1;
		point->cx2=cx2;
		point->cy2=cy2;
		DL_append(xcf_spline->points, (DL_node*)point);
	      }
	  }
	  break;
	case PROP_SPLINES_POINTS_END:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&temp, 1);
	  break;
	case PROP_SPLINES_END:
	  info->cp += xcf_read_int32(info->fp, (guint32*)&temp, 1);
	  xcf_spline=NULL;
	  break;
	default:
	  g_message (_("unexpected/unknown layer property: %d (skipping)"), prop_type);
	  {
	    guint8 buf[16];
	    guint amount;

	    while (prop_size > 0)
	      {
		amount = MIN (16, prop_size);
		info->cp += xcf_read_int8 (info->fp, buf, amount);
		prop_size -= MIN (16, amount);
	      }
	  }
	  break;
	}
    }

  return FALSE;
}

static gint
xcf_load_channel_props (XcfInfo *info,
			GImage  *gimage,
			Channel *channel)
{
  PropType prop_type;
  guint32 prop_size;

  while (1)
    {
      if (!xcf_load_prop (info, &prop_type, &prop_size))
	return FALSE;

      switch (prop_type)
	{
	case PROP_END:
	  return TRUE;
	case PROP_ACTIVE_CHANNEL:
	  info->active_channel = channel;
	  break;
	case PROP_SELECTION:
	  channel_delete (gimage->selection_mask);
	  gimage->selection_mask = channel;
	  channel->boundary_known = FALSE;
	  channel->bounds_known = FALSE;
	  break;
	case PROP_OPACITY:
	  info->cp += xcf_read_float (info->fp, (gfloat*) &channel->opacity, 1);
	  break;
	case PROP_VISIBLE:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(channel)->visible, 1);
	  break;
	case PROP_SHOW_MASKED:
	  info->cp += xcf_read_int32 (info->fp, (guint32*) &channel->show_masked, 1);
	  break;
	case PROP_COLOR:
	    {
	      Tag t = drawable_tag ( GIMP_DRAWABLE( channel) );
	      Tag col_tag = tag_new ( tag_precision (t), FORMAT_RGB, ALPHA_NO);
	      COLOR16_NEW (color, col_tag);
	      COLOR16_INIT (color);
	      xcf_load_row (info, &color);
	      copy_row (&color, &channel->col);
	    }

	  break;
	default:
	  g_message (_("unexpected/unknown channel property: %d (skipping)"), prop_type);
	  {
	    guint8 buf[16];
	    guint amount;

	    while (prop_size > 0)
	      {
		amount = MIN (16, prop_size);
		info->cp += xcf_read_int8 (info->fp, buf, amount);
		prop_size -= MIN (16, amount);
	      }
	  }
	  break;
	}
    }
  return FALSE;
}

static gint
xcf_load_prop (XcfInfo  *info,
	       PropType *prop_type,
	       guint32  *prop_size)
{
  info->cp += xcf_read_int32 (info->fp, (guint32*) prop_type, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) prop_size, 1);
  return TRUE;
}

static Layer*
xcf_load_layer (XcfInfo *info,
		GImage  *gimage)
{
  Layer *layer;
  LayerMask *layer_mask;
  guint32 hierarchy_offset;
  guint32 layer_mask_offset;
  int apply_mask;
  int edit_mask;
  int show_mask;
  int width;
  int height;
  Tag tag;
  int add_floating_sel;
  char *name;

  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment at
   *  the end of this function.
   */
  add_floating_sel = (info->cp == info->floating_sel_offset);

  /* read in the layer width, height, type and name */
  info->cp += xcf_read_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &tag, 1);
  info->cp += xcf_read_string (info->fp, &name, 1);

  /* create a new layer */

  layer = layer_new (gimage->ID, width, height, tag, 
#ifdef NO_TILES						  
						  STORAGE_FLAT,
#else
						  STORAGE_TILED,
#endif
		name, 1.0, NORMAL_MODE);
  if (!layer)
    {
      g_free (name);
      return NULL;
    }

  /* read in the layer properties */
  if (!xcf_load_layer_props (info, gimage, layer))
    goto error;

  if (info->compression == COMPRESS_FRACTAL)
    /* xcf_compress_frac_info (GIMP_DRAWABLE(layer)->type); */;

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->fp, &hierarchy_offset, 1);
  info->cp += xcf_read_int32 (info->fp, &layer_mask_offset, 1);

  /* read in the hierarchy */
  xcf_seek_pos (info, hierarchy_offset);
  if (!xcf_load_hierarchy (info, GIMP_DRAWABLE(layer)->tiles))
    goto error;

  /* read in the layer mask */
  if (layer_mask_offset != 0)
    {
      xcf_seek_pos (info, layer_mask_offset);

      layer_mask = xcf_load_layer_mask (info, gimage);
      if (!layer_mask)
	goto error;

      /* set the offsets of the layer_mask */
      GIMP_DRAWABLE(layer_mask)->offset_x = GIMP_DRAWABLE(layer)->offset_x;
      GIMP_DRAWABLE(layer_mask)->offset_y = GIMP_DRAWABLE(layer)->offset_y;

      apply_mask = layer->apply_mask;
      edit_mask = layer->edit_mask;
      show_mask = layer->show_mask;

      layer_add_mask (layer, layer_mask);

      layer->apply_mask = apply_mask;
      layer->edit_mask = edit_mask;
      layer->show_mask = show_mask;
    }

  /* attach the floating selection... */
  if (add_floating_sel)
    {
      Layer *floating_sel;

      floating_sel = info->floating_sel;
      floating_sel_attach (floating_sel, GIMP_DRAWABLE(layer));
    }

  return layer;

error:
  layer_delete (layer);
  return NULL;
}

static Channel*
xcf_load_channel (XcfInfo *info,
		  GImage  *gimage)
{
  Channel *channel;
  guint32 hierarchy_offset;
  int width;
  int height;
  int add_floating_sel;
  char *name;
  Tag tag = tag_new (default_precision, FORMAT_RGB, ALPHA_NO);
  COLOR16_NEW (black, tag);
  COLOR16_INIT (black);
  palette_get_black (&black);
 
  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment at
   *  the end of this function.
   */
  add_floating_sel = (info->cp == info->floating_sel_offset);

  /* read in the channel width, height and name */
  info->cp += xcf_read_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_read_string (info->fp, &name, 1);

  /* create a new channel */
  channel = channel_new (gimage->ID, width, height, default_precision, name, 1.0, &black);
  if (!channel)
    {
      g_free (name);
      return NULL;
    }

  /* read in the channel properties */
  if (!xcf_load_channel_props (info, gimage, channel))
    goto error;

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->fp, &hierarchy_offset, 1);

  /* read in the hierarchy */
  xcf_seek_pos (info, hierarchy_offset);
  if (!xcf_load_hierarchy (info, GIMP_DRAWABLE(channel)->tiles))
    goto error;

  /* attach the floating selection... */
  if (add_floating_sel)
    {
      Layer *floating_sel;

      floating_sel = info->floating_sel;
      floating_sel_attach (floating_sel, GIMP_DRAWABLE(channel));
    }

  return channel;

error:
  channel_delete (channel);
  return NULL;
}

static LayerMask*
xcf_load_layer_mask (XcfInfo *info,
		     GImage  *gimage)
{
  LayerMask *layer_mask;
  guint32 hierarchy_offset;
  int width;
  int height;
  int add_floating_sel;
  char *name;
  Tag tag = tag_new (default_precision, FORMAT_RGB, ALPHA_NO);
  COLOR16_NEW (black, tag);
  COLOR16_INIT (black);
  palette_get_black (&black);

  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment at
   *  the end of this function.
   */
  add_floating_sel = (info->cp == info->floating_sel_offset);

  /* read in the layer width, height and name */
  info->cp += xcf_read_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_read_string (info->fp, &name, 1);

  /* create a new layer mask */
  layer_mask = layer_mask_new (gimage->ID, width, height, default_precision, name, 1.0, &black);
  if (!layer_mask)
    {
      g_free (name);
      return NULL;
    }

  /* read in the layer_mask properties */
  if (!xcf_load_channel_props (info, gimage, GIMP_CHANNEL(layer_mask)))
    goto error;

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->fp, &hierarchy_offset, 1);

  /* read in the hierarchy */
  xcf_seek_pos (info, hierarchy_offset);
  if (!xcf_load_hierarchy (info, GIMP_DRAWABLE(layer_mask)->tiles))
    goto error;

  /* attach the floating selection... */
  if (add_floating_sel)
    {
      Layer *floating_sel;

      floating_sel = info->floating_sel;
      floating_sel_attach (floating_sel, GIMP_DRAWABLE(layer_mask));
    }

  return layer_mask;

error:
  layer_mask_delete (layer_mask);
  return NULL;
}

static gint
xcf_load_hierarchy (XcfInfo     *info,
		    Canvas *tiles)
{
  gint w;
  gint h;
  Tag tag;
  int bytes;
  StorageType storage;
  AutoAlloc auto_alloc;
  guint32 stora;
  guint32 auto_all;

  info->cp += xcf_read_int32 (info->fp, (guint32*) &w, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &h, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &tag, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &bytes, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &stora, 1);
  info->cp += xcf_read_int32 (info->fp, (guint32*) &auto_all, 1);
  storage = stora;
  auto_alloc = auto_all;

  {
    void * pag;
    PixelArea area;

    pixelarea_init (&area, tiles, 0,0, w, h, TRUE); 

    for (pag = pixelarea_register (1, &area);
	 pag != NULL;
	 pag = pixelarea_process (pag))
      {
	PixelRow row;
	gint height = pixelarea_height (&area);
	while (height--)
	  {
	    pixelarea_getdata (&area, &row, height);
	    xcf_load_row (info, &row);  
	  }
      }
  }

  return TRUE;
}

static void
xcf_load_row (XcfInfo *info,
	       PixelRow *row
		)
{
  Tag t;
  int w;
  Precision p;
  gint num_channels;

  /* read in the tag and width of the row */
  info->cp += xcf_read_int32 (info->fp, (guint32*)&t, 1);
  info->cp += xcf_read_int32 (info->fp, &w, 1);

  if (!tag_equal (pixelrow_tag (row), t))
    {
      g_print ("xcf_load_row: tags dont match for row\n");
      return;
    }

  if (pixelrow_width (row) != w)
    {
      g_print ("xcf_load_row: widths dont match for row\n");
      return;
    }

  p = tag_precision (t);
  num_channels = tag_num_channels (t);
 
  switch (p) 
    {
    case PRECISION_U8:
      {
	guint8 * data = (guint8*)pixelrow_data (row);
	info->cp += xcf_read_int8 (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_BFP:
    case PRECISION_U16:
    case PRECISION_FLOAT16:
      {
	guint16 * data = (guint16*)pixelrow_data (row);
	info->cp += xcf_read_int16 (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_FLOAT:
      {
	gfloat * data = (gfloat*)pixelrow_data (row);
	info->cp += xcf_read_float (info->fp, data, num_channels * w);
      }
      break;
    case PRECISION_NONE:
    default:
      g_print ("xcf_load_row: unrecognized precision\n");	
      break;
    }
}

#ifdef SWAP_FROM_FILE

static int
xcf_swap_func (int       fd,
	       Tile     *tile,
	       int       cmd,
	       gpointer  user_data)
{
  int bytes;
  int err;
  int nleft;
  int *ref_count;

  switch (cmd)
    {
    case SWAP_IN:
      lseek (fd, tile->swap_offset, SEEK_SET);

      bytes = tile_size (tile);
      tile_alloc (tile);

      nleft = bytes;
      while (nleft > 0)
	{
	  do {
	    err = read (fd, tile->data + bytes - nleft, nleft);
	  } while ((err == -1) && ((errno == EAGAIN) || (errno == EINTR)));

	  if (err <= 0)
	    {
	      g_message (_("unable to read tile data from xcf file: %d ( %d ) bytes read"), err, nleft);
	      return FALSE;
	    }

	  nleft -= err;
	}
      break;
    case SWAP_OUT:
    case SWAP_DELETE:
    case SWAP_COMPRESS:
      ref_count = user_data;
      *ref_count -= 1;
      if (*ref_count == 0)
	{
	  tile_swap_remove (tile->swap_num);
	  g_free (ref_count);
	}

      tile->swap_num = 1;
      tile->swap_offset = -1;

      return TRUE;
    }

  return FALSE;
}

#endif


static void
xcf_seek_pos (XcfInfo *info,
	      guint    pos)
{
  if (info->cp != pos)
    {
      info->cp = pos;
      fseek (info->fp, info->cp, SEEK_SET);
    }
}

static void
xcf_seek_end (XcfInfo *info)
{
  fseek (info->fp, 0, SEEK_END);
  info->cp = ftell (info->fp);
}

static guint
xcf_read_float (FILE     *fp,
		gfloat   *data,
		gint      count)
{
  guint total;
  guint32 * tmp_long;

  total = count;
  if (count > 0)
    {
      xcf_read_int8 (fp, (guint8*) data, count * 4);

      while (count--)
        {
	  tmp_long = (guint32 *)data;
          *tmp_long = ntohl (*tmp_long);
          data++;
        }
    }

  return total * 4;
}

static guint
xcf_read_int32 (FILE     *fp,
		guint32  *data,
		gint      count)
{
  guint total;

  total = count;
  if (count > 0)
    {
      xcf_read_int8 (fp, (guint8*) data, count * 4);

      while (count--)
        {
          *data = ntohl (*data);
          data++;
        }
    }

  return total * 4;
}

static guint
xcf_read_int16 (FILE     *fp,
		guint16  *data,
		gint      count)
{
  guint total;

  total = count;
  if (count > 0)
    {
      xcf_read_int8 (fp, (guint8*) data, count * 2);

      while (count--)
        {
          *data = ntohs (*data);
          data++;
        }
    }

  return total * 2;
}

static guint
xcf_read_int8 (FILE     *fp,
	       guint8   *data,
	       gint      count)
{
  guint total;
  int bytes;

  total = count;
  while (count > 0)
    {
      bytes = fread ((char*) data, sizeof (char), count, fp);
      if (bytes <= 0) /* something bad happened */
        break;
      count -= bytes;
      data += bytes;
    }

  return total;
}

static guint
xcf_read_string (FILE     *fp,
		 gchar   **data,
		 gint      count)
{
  guint32 tmp;
  guint total;
  int i;

  total = 0;
  for (i = 0; i < count; i++)
    {
      total += xcf_read_int32 (fp, &tmp, 1);
      if (tmp > 0)
        {
          data[i] = g_new (gchar, tmp);
          total += xcf_read_int8 (fp, (guint8*) data[i], tmp);
        }
      else
        {
          data[i] = NULL;
        }
    }

  return total;
}

static guint
xcf_write_float (FILE     *fp,
		 gfloat  *data,
		 gint      count)
{
  guint32 tmp;
  guint32 *tmp_long;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
	  tmp_long = (guint32 *) &data[i];
          tmp = htonl (*tmp_long);
          xcf_write_int8 (fp, (guint8*) &tmp, 4);
        }
    }

  return count * 4;
}

static guint
xcf_write_int32 (FILE     *fp,
		 guint32  *data,
		 gint      count)
{
  guint32 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          tmp = htonl (data[i]);
          xcf_write_int8 (fp, (guint8*) &tmp, 4);
        }
    }

  return count * 4;
}

static guint
xcf_write_int16 (FILE     *fp,
		 guint16  *data,
		 gint      count)
{
  guint16 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          tmp = htons (data[i]);
          xcf_write_int8 (fp, (guint8*) &tmp, 2);
        }
    }

  return count * 2;
}

static guint
xcf_write_int8 (FILE     *fp,
		guint8   *data,
		gint      count)
{
  guint total;
  int bytes;

  total = count;
  while (count > 0)
    {
      bytes = fwrite ((char*) data, sizeof (char), count, fp);
      count -= bytes;
      data += bytes;
    }

  return total;
}

static guint
xcf_write_string (FILE     *fp,
		  gchar   **data,
		  gint      count)
{
  guint32 tmp;
  guint total;
  int i;

  total = 0;
  for (i = 0; i < count; i++)
    {
      if (data[i])
        tmp = strlen (data[i]) + 1;
      else
        tmp = 0;

      xcf_write_int32 (fp, &tmp, 1);
      if (tmp > 0)
        xcf_write_int8 (fp, (guint8*) data[i], tmp);

      total += 4 + tmp;
    }

  return total;
}
