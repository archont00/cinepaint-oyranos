/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "compat.h"
#include "brush.h"
#include "brushlist.h"
#include "signal_type.h"
#include "rc.h"
#include "brush_header.h"
#include "../lib/wire/iodebug.h"
#include "depth/float16.h"

/* we want old brushes be loaded as usual */
#if WORDS_BIGENDIAN 
#define RnHFLT( x, t ) (t.s[0] = (x), t.s[1]= 0, t.f) 
#define RnHFLT16( x, t ) (t.f = (x), t.s[0])   
#else
#define RnHFLT( x, t ) (t.s[1] = (x), t.s[0]= 0, t.f) 
#define RnHFLT16( x, t ) (t.f = (x), t.s[1])   
#endif

enum{
  DIRTY,
  RENAME,
  LAST_SIGNAL
};

static guint gimp_brush_signals[LAST_SIGNAL];
static GimpObjectClass* parent_class;

static void
gimp_brush_destroy(GtkObject *object)
{
  GimpBrush* brush=GIMP_BRUSH(object);
  if (brush->filename)
    g_free(brush->filename);
  if (brush->name)
    g_free(brush->name);
  if (brush->mask)
     canvas_delete(brush->mask);
  GTK_OBJECT_CLASS(parent_class)->destroy (object);

}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);
  
  type=GTK_CLASS_TYPE(object_class);

  object_class->destroy =  gimp_brush_destroy;

  gimp_brush_signals[DIRTY] =
    gimp_signal_new ("dirty", 0, type, 0, gimp_sigtype_void);

  gimp_brush_signals[RENAME] =
    gimp_signal_new ("rename", 0, type, 0, gimp_sigtype_void);

#if GTK_MAJOR_VERSION < 2
  gtk_object_class_add_signals (object_class, gimp_brush_signals, LAST_SIGNAL);
#endif
}

void
gimp_brush_init(GimpBrush *brush)
{
  brush->filename  = NULL;
  brush->name      = NULL;
  brush->spacing   = 20;
  brush->mask      = NULL;
  brush->x_axis.x    = 15.0;
  brush->x_axis.y    =  0.0;
  brush->y_axis.x    =  0.0;
  brush->y_axis.y    = 15.0;
}

GtkType gimp_brush_get_type(void)
{
  static GtkType type;
  if(!type){
#if GTK_MAJOR_VERSION > 1
      static const GTypeInfo info =
      {
        sizeof (GimpBrushClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_brush_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBrush),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_brush_init,
      };
      type = g_type_register_static (GIMP_TYPE_OBJECT,
                                              "GimpBrush",
                                              &info, 0);
#else
    GtkTypeInfo info={
      "GimpBrush",
      sizeof(GimpBrush),
      sizeof(GimpBrushClass),
      (GtkClassInitFunc)gimp_brush_class_init,
      (GtkObjectInitFunc)gimp_brush_init,
      NULL,
      NULL };
    type=gtk_type_unique(GIMP_TYPE_OBJECT, &info);
#endif
  }
  return type;
}

Canvas *
gimp_brush_get_mask (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->mask;
}

char *
gimp_brush_get_name (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->name;
}

void
gimp_brush_set_name (GimpBrush *brush, char *name)
{
  g_return_if_fail(GIMP_IS_BRUSH(brush));
  if (strcmp(brush->name, name) == 0)
    return;
  if (brush->name)
    g_free(brush->name);
  brush->name = g_strdup(name);
  gtk_signal_emit(GTK_OBJECT(brush), gimp_brush_signals[RENAME]);
}

GimpBrush *
gimp_brush_load(char *filename)
{
  FILE * fp;
  int bn_size;
  unsigned char buf [sz_BrushHeader];
  BrushHeader header;
  unsigned int * hp;
  unsigned int i;
  guint32 header_size;
  guint32 header_version;
  Tag tag;
  gint bytes;
  GimpBrush* brush;
  Canvas *brushmask;
  char *brushname;
  guchar *data, tmp;
  int RnH_float = 0;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "rb")))
    {
      return NULL;
    }

#if 1
  /*  Read in the header size and version */
  if ((fread (&header_size, 1, sizeof (guint32), fp)) < sizeof (guint32)) 
    {
      fclose (fp);
      return NULL;
    }


  if ((fread (&header_version, 1, sizeof (guint32), fp)) < sizeof (guint32)) 
    {
      fclose (fp);
      return NULL;
    }
	
  fseek (fp, - 2 * CAST(long) sizeof(guint32), SEEK_CUR);
#endif

  if ((fread (&buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader) 
    {
      fclose (fp);
      return NULL;
    }

  /*  rearrange the bytes in each unsigned int  */
#if 1
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4 ] << 24) + (buf [i * 4 + 1] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);
#else
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4 + 1] << 24) + (buf [i * 4 + 0] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);
#endif
  /*  Check for correct file format */
  if (header.magic_number != GBRUSH_MAGIC)
    {
      /*  One thing that can save this error is if the brush is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  return NULL;
	}
    }

  if (header.version == 1 || header.version == 2)
    {
      if (header.version == 1)
      {
	 /*  If this is a version 1 brush, set the fp back 8 bytes  */
	 fseek (fp, -8, SEEK_CUR);
	 header.header_size += 8;
	 /*  spacing is not defined in version 1  */
	 header.spacing = 25;
      }

      /* If its version 1 or 2 the bytes field contains just
         the number of bytes and must be converted to a drawable type. */    

      if (header.type == 1) 
	{
 	header.type = GRAY_GIMAGE;
	}	
      else if (header.type == 3)
	{
        header.type = RGB_GIMAGE;
	}	
    }
  else if (header.version != FILE_VERSION)
    {
      g_message (_("Unknown version %d in \"%s\"\n"), header.version, filename);
      fclose (fp);
      return NULL;
    }
  
    /* make a exception for IEEEHalf format since version 0.22-0 */
    if(header.type == IEEEHALF_GRAY_IMAGE)
    {
      header.type = FLOAT16_GRAY_GIMAGE;
      RnH_float = 0;
    } else
    if(header.type == FLOAT16_GRAY_GIMAGE)
    {
      RnH_float = 1;
    }

    tag = tag_from_drawable_type ( header.type);


    if (tag_precision (CAST(int) tag) != PRECISION_FLOAT16)
       { 
	fclose (fp);
	return NULL;
       }

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sz_BrushHeader)))
  {
    brushname = (char *) g_malloc (sizeof (char) * bn_size);
    if ((fread (brushname, 1, bn_size, fp)) < CAST(size_t) bn_size)
    {
      g_message (_("Error in brush file...aborting."));
      fclose (fp);
      return NULL;
    }
  }
  else
    brushname = g_strdup (_("Untitled"));

  switch(header.version)
  {
   case 1:
   case 2:
   case FILE_VERSION:
     /*  Get a new brush mask  */
     brushmask = canvas_new (tag,
				header.width,
                            	header.height,
                            	STORAGE_FLAT );


  /*  Read the brush mask data  */
     bytes = tag_bytes (tag);
     canvas_portion_refrw (brushmask, 0, 0);
     data = canvas_portion_data (brushmask,0,0);
     if ((fread (data, 1, header.width * header.height * bytes, fp))
	 < header.width * header.height * bytes)
     	g_message (_("Brush file appears to be truncated."));
#ifndef WORDS_BIGENDIAN
     if(RnH_float){
       for (i=0; i<header.width * header.height * bytes-1; i+=2)
       {
	 tmp = data[i];
	 data[i] = data[i+1];
	 data[i+1] = tmp;
       }
     }
#endif
     if (RnH_float)
     for (i=0; i<header.width * header.height * bytes-1; i+=2)
       {
         gfloat f;
         guint16 *f16 = (guint16*) &data[i];
         ShortsFloat u;
         /* old FLT macro */
         f = RnHFLT(*f16,u);
         *f16 = FLT16(f,u);
       }
     canvas_portion_unref (brushmask, 0, 0);
     break;
   default:
     g_message (_("Unknown brush format version #%d in \"%s\"\n"), header.version, filename);
     fclose (fp);
     return NULL;
  }

  fclose (fp);

  /* Okay we are successful, create the brush object */

  brush = GIMP_BRUSH(gimp_type_new(gimp_brush_get_type()));
  brush->filename = g_strdup (filename);
  brush->name = brushname;
  brush->mask = brushmask;
  brush->spacing = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  /* Check if the current brush is the default one */

/* lets see if it works with out this for now */

#if 1
  {
    char * basename = strrchr (filename, '/' );
    if (basename && default_brush)
      {
	basename++;
	if (strcmp(default_brush, basename) == 0)
	  {
	    select_brush (brush);
	    set_have_default_brush (TRUE);
	  }
      }
  }
#endif

  return brush;
}
