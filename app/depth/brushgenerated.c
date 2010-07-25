/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp_brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "config.h"
#include "../cursorutil.h"
#include "../gdisplay.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "libgimp/gimpintl.h"

#ifdef WIN32
#include "extra.h"
#endif

#ifndef HAVE_RINT
#define rint(x) floor (x + 0.5)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../appenv.h"
#include "../brushgenerated.h"
#include "../brushlist.h"
#include "float16.h"
#include "../paint_core_16.h"
#include "../rc.h"

#define OVERSAMPLING 5

double
smoothstep(double a, double b, double x);

static void
gimp_brush_generated_generate(GimpBrushGenerated *brush);


typedef void  
(*GimpBrushGeneratedComputeFunc) (GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_u8(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_u16(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_float(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_float16(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);
static void 
gimp_brush_generated_compute_bfp(GimpBrushGenerated *brush, 	
				gint width, gint height, gfloat *lookup);

static GimpBrushGeneratedComputeFunc gimp_brush_generated_compute_funcs (Tag t);

static GimpObjectClass* parent_class;

static void
gimp_brush_generated_destroy(GtkObject *object)
{
  GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
gimp_brush_generated_class_init (GimpBrushGeneratedClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (GIMP_TYPE_BRUSH);
  object_class->destroy =  gimp_brush_generated_destroy;
}


void
gimp_brush_generated_init(GimpBrushGenerated *brush)
{
  brush->radius        = 5.0;
  brush->hardness      = 0.0;
  brush->angle         = 0.0;
  brush->aspect_ratio  = 1.0;
  brush->freeze        = 0;
}

guint gimp_brush_generated_get_type(void)
{
  static GtkType type;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushGenerated",
      sizeof(GimpBrushGenerated),
      sizeof(GimpBrushGeneratedClass),
      (GtkClassInitFunc)gimp_brush_generated_class_init,
      (GtkObjectInitFunc)gimp_brush_generated_init,
      NULL,
      NULL };
    type=gtk_type_unique(GIMP_TYPE_BRUSH, &info);
  }
  return type;
}

GimpBrushGenerated *gimp_brush_generated_new(float radius, float hardness,
				    float angle, float aspect_ratio)
{
  GimpBrushGenerated *brush;

/* set up normal brush data */
  brush = GIMP_BRUSH_GENERATED(gimp_type_new(gimp_brush_generated_get_type ()));

  GIMP_BRUSH(brush)->name = g_strdup (_("Untitled"));
  GIMP_BRUSH(brush)->spacing = 20;

/* set up gimp_brush_generated data */
  brush->radius = radius;
  brush->hardness = hardness;
  brush->angle = angle;
  brush->aspect_ratio = aspect_ratio;

 /* render brush mask */
  gimp_brush_generated_generate(brush);

  return brush;
}

#if GTK_MAJOR_VERSION > 1
#include <errno.h>
GimpBrushGenerated *
gimp_brush_generated_load (const gchar  *filename)
{
  GimpBrushGenerated      *brush;
  FILE                    *file;
  gchar                    string[256];
  gchar                   *name       = NULL;
  /*GimpBrushGeneratedShape  shape      = GIMP_BRUSH_GENERATED_CIRCLE;*/
  gboolean                 have_shape = FALSE;
  gint                     spikes     = 2;
  gdouble                  spacing;
  gdouble                  radius;
  gdouble                  hardness;
  gdouble                  aspect_ratio;
  gdouble                  angle;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  /*g_return_val_if_fail (error == NULL || *error == NULL, NULL);*/

  file = fopen (filename, "rb");

  if (! file)
    {
      printf ( _("Could not open '%s' for reading: %s"),
                   /*gimp_filename_to_utf8*/ (filename), g_strerror (errno));
      return NULL;
    }

  /* make sure the file we are reading is the right type */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;

  if (strncmp (string, "GIMP-VBR", 8) != 0)
    {
      printf( _("Fatal parse error in brush file '%s': "
                     "Not a GIMP brush file."),
                   /*gimp_filename_to_utf8*/ (filename));
      goto failed;
    }

  /* make sure we are reading a compatible version */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;

  if (strncmp (string, "1.0", 3))
    {
      if (strncmp (string, "1.5", 3))
        {
          printf ( _("Fatal parse error in brush file '%s': "
                         "Unknown GIMP brush version."),
                       /*gimp_filename_to_utf8*/ (filename));
          goto failed;
        }
      else
        {
          have_shape = TRUE;
        }
    }

  /* read name */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;

  g_strstrip (string);
  name = strdup/*gimp_any_to_utf8 (string, -1,
                           _("Invalid UTF-8 string in brush file '%s'."),*/
                           /*gimp_filename_to_utf8*/ (filename);

  /*if (have_shape)
    {
      GEnumClass *enum_class;
      GEnumValue *shape_val;

      enum_class = g_type_class_peek (GIMP_TYPE_BRUSH_GENERATED_SHAPE);

      *//* read shape *//*
      errno = 0;
      if (! fgets (string, sizeof (string), file))
        goto failed;

      g_strstrip (string);
      shape_val = g_enum_get_value_by_nick (enum_class, string);

      if (!shape_val)
        {
          printf ( _("Fatal parse error in brush file '%s': "
                         "Unknown GIMP brush shape."),
                       gimp_filename_to_utf8 (filename));
          goto failed;
        }

      shape = shape_val->value;
    } */

  /* read brush spacing */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;
  spacing = g_ascii_strtod (string, NULL);

  /* read brush radius */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;
  radius = g_ascii_strtod (string, NULL);

  if (have_shape)
    {
      /* read brush radius */
      errno = 0;
      if (! fgets (string, sizeof (string), file))
        goto failed;
      spikes = CLAMP (atoi (string), 2, 20);
    }

  /* read brush hardness */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;
  hardness = g_ascii_strtod (string, NULL);

  /* read brush aspect_ratio */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;
  aspect_ratio = g_ascii_strtod (string, NULL);

  /* read brush angle */
  errno = 0;
  if (! fgets (string, sizeof (string), file))
    goto failed;
  angle = g_ascii_strtod (string, NULL);

  fclose (file);

  /* create new brush */
/*
  brush = g_object_new (GIMP_TYPE_BRUSH_GENERATED,
                        "name",         name,
                        "shape",        shape,
                        "radius",       radius,
                        "spikes",       spikes,
                        "hardness",     hardness,
                        "aspect-ratio", aspect_ratio,
                        "angle",        angle,
                        NULL);
  g_free (name);

  GIMP_BRUSH (brush)->spacing = spacing;
*/
  /* render brush mask */
  /*gimp_data_dirty (GIMP_DATA (brush));*/

/* create new brush */
  brush = GIMP_BRUSH_GENERATED(gimp_type_new(gimp_brush_generated_get_type()));
  GIMP_BRUSH(brush)->filename = g_strdup (filename);
  gimp_brush_generated_freeze(brush);

/* read name */
  GIMP_BRUSH(brush)->name = g_strdup (name);
/* read brush spacing */
  GIMP_BRUSH(brush)->spacing = spacing;
/* read brush radius */
  gimp_brush_generated_set_radius(brush, radius);
/* read brush hardness */
  gimp_brush_generated_set_hardness(brush, hardness);
/* read brush aspect_ratio */
  gimp_brush_generated_set_aspect_ratio(brush, aspect_ratio);
/* read brush angle */
  gimp_brush_generated_set_angle(brush, angle);


  gimp_brush_generated_thaw(brush);

#if 1 
  {
    char * basename = strrchr (filename, '/' );
    if (basename && default_brush) 
      {
	basename++; 
	if (strcmp(default_brush, basename) == 0) 
	  {
	    select_brush (GIMP_BRUSH(brush));
	    set_have_default_brush (TRUE); 
	  }
      } 
  }
#endif

  return brush;

 failed:

  fclose (file);

  if (name)
    g_free (name);

  /*if (error && *error == NULL)*/
    printf ( _("Error while reading brush file '%s': %s"),
                 /*gimp_filename_to_utf8*/ (filename),
                 errno ? g_strerror (errno) : _("File is truncated"));

  return NULL;
}
#else
GimpBrushGenerated *
gimp_brush_generated_load (const char *file_name)
{
  GimpBrushGenerated *brush;
  FILE *fp;
  char string[256];
  float fl;
  float version;
  if ((fp = fopen(file_name, "rb")) == NULL)
    return NULL;

/* make sure the file we are reading is the right type */
  fscanf(fp, "%8s", string);
  g_return_val_if_fail(strcmp(string, "GIMP-VBR") == 0, NULL);
/* make sure we are reading a compatible version */
  fscanf(fp, "%f", &version);
  g_return_val_if_fail(version < 2.0, NULL);

/* create new brush */
  brush = GIMP_BRUSH_GENERATED(gimp_type_new(gimp_brush_generated_get_type()));
  GIMP_BRUSH(brush)->filename = g_strdup (file_name);
  gimp_brush_generated_freeze(brush);

/* read name */
  fscanf(fp, "%255s", string);
  GIMP_BRUSH(brush)->name = g_strdup (string);
/* read brush spacing */
  fscanf(fp, "%f", &fl);
  GIMP_BRUSH(brush)->spacing = fl;
/* read brush radius */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_radius(brush, fl);
/* read brush hardness */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_hardness(brush, fl);
/* read brush aspect_ratio */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_aspect_ratio(brush, fl);
/* read brush angle */
  fscanf(fp, "%f", &fl);
  gimp_brush_generated_set_angle(brush, fl);

  fclose(fp);

  gimp_brush_generated_thaw(brush);

#if 1 
  {
    char * basename = strrchr (file_name, '/' );
    if (basename && default_brush) 
      {
	basename++; 
	if (strcmp(default_brush, basename) == 0) 
	  {
	    select_brush (GIMP_BRUSH(brush));
	    set_have_default_brush (TRUE); 
	  }
      } 
  }
#endif

  return brush;
}
#endif

void
gimp_brush_generated_save (GimpBrushGenerated *brush, 
			  const char *file_name)
{
  FILE *fp;
  char *lnc = 0, *tmp;
  if ((fp = fopen(file_name, "wb")) == NULL)
  {
    g_warning("Unable to save file %s", file_name);
    return;
  }
#ifdef ENABLE_NLS
  lnc = setlocale(LC_NUMERIC, 0);
  if(lnc) {
    tmp= strdup(lnc);
    lnc = tmp;
  }
  setlocale(LC_NUMERIC, "C");
#endif
/* write magic header */
  fprintf(fp, "GIMP-VBR\n");
/* write version */
  fprintf(fp, "1.0\n");
/* write name */
  fprintf(fp, "%s\n", GIMP_BRUSH(brush)->name);
/* write brush spacing */
  fprintf(fp, "%f\n", (float)GIMP_BRUSH(brush)->spacing);
/* write brush radius */
  fprintf(fp, "%f\n", brush->radius);
/* write brush hardness */
  fprintf(fp, "%f\n", brush->hardness);
/* write brush aspect_ratio */
  fprintf(fp, "%f\n", brush->aspect_ratio);
/* write brush angle */
  fprintf(fp, "%f\n", brush->angle);
#ifdef ENABLE_NLS
  setlocale(LC_NUMERIC, lnc);
  if(lnc) free(lnc);
#endif

  fclose(fp);
}

void
gimp_brush_generated_freeze(GimpBrushGenerated *brush)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  brush->freeze++;
}

void
gimp_brush_generated_thaw(GimpBrushGenerated *brush)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  if (brush->freeze > 0)
    brush->freeze--;
  if (brush->freeze == 0)
    gimp_brush_generated_generate(brush);
}

double
smoothstep(double a, double b, double x)
{
    if (x < a)
        return 0;
    if (x >= b)
        return 1;
    x = (x - a)/(b - a); /* normalize to [0:1] */
    return (x*x * (3 - 2*x));
}

/* This should be commented out to remove a compiler warning,
 * but I want to leave it in just in case it's ever useful again. --jcohen */

static
double gauss(double f)
{ 
    /* this aint' a real gauss function */
    if (f < -.5)
    {
      f = -1.0-f;
      return (2.0*f*f);
    }
    if (f < .5)
      return (1.0-2.0*f*f);
    f = 1.0 -f;
    return (2.0*f*f);
}

void
gimp_brush_generated_generate(GimpBrushGenerated *brush)
{
  register GimpBrush *gbrush = NULL;
  register int x;
  register double exponent;
  register int length;
  register float *lookup;
  double buffer[OVERSAMPLING];
  gdouble d;
  register double sum, c, s, tx, ty;
  int width, height;
  Tag tag;

  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (brush));
  
  if (brush->freeze) /* if we are frozen defer rerendering till later */
    return;

  gbrush = GIMP_BRUSH(brush);

  if (gbrush->mask)
  {
    canvas_delete(gbrush->mask);
  }
  /* compute the range of the brush. should do a better job than this? */
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);
  tx = MAXIMUM(fabs(c*ceil(brush->radius) - s*ceil(brush->radius)
		    /brush->aspect_ratio), 
	       fabs(c*ceil(brush->radius) + s*ceil(brush->radius)
		    /brush->aspect_ratio));
  ty = MAXIMUM(fabs(s*ceil(brush->radius) + c*ceil(brush->radius)
		    /brush->aspect_ratio),
	       fabs(s*ceil(brush->radius) - c*ceil(brush->radius)
		    /brush->aspect_ratio));
  if (brush->radius > tx)
    width = ceil(tx);
  else
    width = ceil(brush->radius);
  if (brush->radius > ty)
    height = ceil(ty);
  else
    height = ceil(brush->radius);

  /* compute the axis for spacing */
  GIMP_BRUSH(brush)->x_axis.x =      c*brush->radius;
  GIMP_BRUSH(brush)->x_axis.y = -1.0*s*brush->radius;

  GIMP_BRUSH(brush)->y_axis.x = (s*brush->radius / brush->aspect_ratio);
  GIMP_BRUSH(brush)->y_axis.y = (c*brush->radius / brush->aspect_ratio);
 

  if ((1.0 - brush->hardness) < 0.000001)
    exponent = 1000000; 
  else
    exponent = 1/(1.0 - brush->hardness);

/* set up lookup table */
  length = ceil(sqrt(2*ceil(brush->radius+1)*ceil(brush->radius+1))+1) * OVERSAMPLING;
  lookup = g_malloc(length * sizeof(float));
  sum = 0.0;
  for (x = 0; x < OVERSAMPLING; x++)
  {
    d = fabs((x+.5)/OVERSAMPLING - .5);
    if (d > brush->radius)
      buffer[x] = 0.0;
    else
/*      buffer[x] =  (1.0 - pow(d/brush->radius, exponent));*/
/*      buffer[x] =  gauss(pow(d/brush->radius, exponent));*/
	buffer[x] = 1.0 - smoothstep( brush->hardness, 1.0,d/brush->radius);
    sum += buffer[x];
  }
  for (x = 0; d < brush->radius || sum > .00001; d += 1.0/OVERSAMPLING)
  {
    sum -= buffer[x%OVERSAMPLING];
    if (d > brush->radius)
      buffer[x%OVERSAMPLING] = 0.0;
    else
/*      buffer[x%OVERSAMPLING] =  (1.0 - pow(d/brush->radius, exponent));*/
/*      buffer[x%OVERSAMPLING] =  gauss(pow(d/brush->radius, exponent));*/
	buffer[x%OVERSAMPLING] = 1.0 - smoothstep( brush->hardness, 1.0 ,d/brush->radius);
    sum += buffer[x%OVERSAMPLING];
    lookup[x++] =  sum/(float)OVERSAMPLING;
  }
  while (x < length)
  {
    lookup[x++] = 0.0;
  }

  
  tag = tag_new ( default_precision, FORMAT_GRAY, ALPHA_NO ); 
  { /* use the PRECISION_FLOAT16 variant only */
    int w = 0;
    if (w)
      (*gimp_brush_generated_compute_funcs(tag))(brush, width, height, lookup);
    else
      gimp_brush_generated_compute_float16 (brush, width, height, lookup);
  }

  g_free (lookup);
  gtk_signal_emit_by_name(GTK_OBJECT(brush), "dirty");
/*  brush_changed_notify(GIMP_BRUSH(brush)); */
}

static GimpBrushGeneratedComputeFunc 
gimp_brush_generated_compute_funcs (
                   Tag tag
                   )
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return gimp_brush_generated_compute_u8;
    case PRECISION_U16:
      return gimp_brush_generated_compute_u16;
    case PRECISION_FLOAT:
      return gimp_brush_generated_compute_float;
    case PRECISION_FLOAT16:
      return gimp_brush_generated_compute_float16;
    case PRECISION_BFP:
      return gimp_brush_generated_compute_bfp;
    case PRECISION_NONE:
    default:
      g_warning ("gimp_brush_generated_compute_func: bad precision");
    }
  return NULL;
}

static void
gimp_brush_generated_compute_float16( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guint16 *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint16 *data;
  guint16 val;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( PRECISION_FLOAT16, FORMAT_GRAY, ALPHA_NO ); 
  ShortsFloat u;

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint16*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a, 0, 1.0);
      val = FLT16 (a,u);
      centerp[   y*canvas_w + x] = val;
      centerp[-1*y*canvas_w - x] = val;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_float( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  gfloat *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  gfloat *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (gfloat*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a, 0, 1.0);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_u16( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guint16 *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint16 *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( PRECISION_U16, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint16*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a * 65535.0, 0, 65535);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}

static void
gimp_brush_generated_compute_u8( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guchar *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint8 *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( PRECISION_U8, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint8*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = CLAMP (a * 255.0, 0, 255);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}
				
static void
gimp_brush_generated_compute_bfp( 
				GimpBrushGenerated *brush,
				gint width,
				gint height,
			  	gfloat *lookup	 
				)
{
  GimpBrush *gbrush = GIMP_BRUSH(brush);
  guint16 *centerp;
  gdouble a, d, c, s, tx, ty;
  gint x,y;
  gint canvas_w, canvas_h;
  guint16 *data;
  gint w = width;
  gint h = height;
  Tag tag = tag_new ( PRECISION_BFP, FORMAT_GRAY, ALPHA_NO ); 

  /* make the canvas */
  gbrush->mask = canvas_new( tag, width*2 + 1, height*2 + 1, STORAGE_FLAT);
  canvas_portion_refrw (gbrush->mask,0,0);
  data = (guint16*)canvas_portion_data (gbrush->mask, 0, 0);  
  canvas_w = canvas_width (gbrush->mask); 
  canvas_h = canvas_height (gbrush->mask); 
  centerp = &data[height*canvas_w + width];
	
  s = sin(brush->angle*M_PI/180.0);
  c = cos(brush->angle*M_PI/180.0);

  /* compute one half and mirror it */
  for (y = 0; y <= h; y++)
  {
    for (x = -w; x <= w; x++)
    {
      tx = c*x - s*y;
      ty = c*y + s*x;
      ty *= brush->aspect_ratio;
      d = sqrt(tx*tx + ty*ty);
      if (d < brush->radius+1)
	a = lookup[(int)rint(d*OVERSAMPLING)];
      else
	a = 0.0;
      a = MIN(a * 65535.0,65535);
      centerp[   y*canvas_w + x] = a;
      centerp[-1*y*canvas_w - x] = a;
    }
  }
  canvas_portion_unref (gbrush->mask, 0, 0);
}


float
gimp_brush_generated_set_radius (GimpBrushGenerated* brush, float radius)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (radius < 0.0)
    radius = 0.0;
  else if(radius > 32767.0)
    radius = 32767.0;
  if (radius == brush->radius)
    return radius;
  brush->radius = radius;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->radius;
}

float
gimp_brush_generated_set_hardness (GimpBrushGenerated* brush, float hardness)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (hardness < 0.0)
    hardness = 0.0;
  else if(hardness > 1.0)
    hardness = 1.0;
  if (brush->hardness == hardness)
    return hardness;
  brush->hardness = hardness;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->hardness;
}

float
gimp_brush_generated_set_angle (GimpBrushGenerated* brush, float angle)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (angle < 0.0)
    angle = -1.0 * fmod(angle, 180.0);
  else if(angle > 180.0)
    angle = fmod(angle, 180.0);
  if (brush->angle == angle)
    return angle;
  brush->angle = angle;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->angle;
}

float
gimp_brush_generated_set_aspect_ratio (GimpBrushGenerated* brush, float ratio)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  if (ratio < 1.0)
    ratio = 1.0;
  else if(ratio > 1000)
    ratio = 1000;
  if (brush->aspect_ratio == ratio)
    return ratio;
  brush->aspect_ratio = ratio;
  if (!brush->freeze)
    gimp_brush_generated_generate(brush);
  return brush->aspect_ratio;
}

float 
gimp_brush_generated_get_radius (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->radius;
}

float
gimp_brush_generated_get_hardness (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->hardness;
}

float
gimp_brush_generated_get_angle (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->angle;
}

float
gimp_brush_generated_get_aspect_ratio (const GimpBrushGenerated* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_GENERATED(brush), -1.0);
  return brush->aspect_ratio;
}

void
gimp_brush_generated_increase_radius ()
{

  if (GIMP_IS_BRUSH_GENERATED(get_active_brush()))
    {
      int radius = gimp_brush_generated_get_radius (GIMP_BRUSH_GENERATED(get_active_brush ()));
      radius = radius < 100 ? radius + 1 : radius;

      gimp_brush_generated_set_radius (GIMP_BRUSH_GENERATED(get_active_brush ()), radius);
      create_win_cursor (gdisplay_active ()->canvas->window, 
	  radius*2*(SCALEDEST (gdisplay_active ()) / SCALESRC (gdisplay_active ()))); 
    }
  else
    e_printf ("ERROR : you can not edit this brush\n");
}

void
gimp_brush_generated_decrease_radius ()
{
  if (GIMP_IS_BRUSH_GENERATED(get_active_brush()))
    {
      int radius = gimp_brush_generated_get_radius (GIMP_BRUSH_GENERATED(get_active_brush ()));
      radius = radius > 1 ? radius - 1 : radius;
      gimp_brush_generated_set_radius (GIMP_BRUSH_GENERATED(get_active_brush ()), radius);
      create_win_cursor (gdisplay_active ()->canvas->window, 
	radius*2*(SCALEDEST (gdisplay_active ()) / SCALESRC (gdisplay_active ()))); 
    }
  else
    e_printf ("ERROR : you can not edit this brush\n");
}

