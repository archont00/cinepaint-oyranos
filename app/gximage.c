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
#include "appenv.h"
#include "colormaps.h"
#include "gimage.h"
#include "gximage.h"
#include "errors.h"

typedef struct GXImage  GXImage;

struct GXImage
{
  long width, height;		/*  width and height of ximage structure    */

  GdkVisual *visual;		/*  visual appropriate to our depth         */
  GdkGC *gc;			/*  graphics context                        */

  guchar *data;
};


/*  The static gximages for drawing to windows  */
static GXImage *gximage = NULL;

#define QUANTUM   32

/*  STATIC functions  */

static GXImage *
create_gximage (GdkVisual *visual, int width, int height)
{
  GXImage * gximage;

  gximage = (GXImage *) g_malloc_zero (sizeof (GXImage));

  gximage->visual = visual;
  gximage->gc = NULL;

  gximage->data = g_malloc (width * height * 3);

  return gximage;
}

static void
delete_gximage (GXImage *gximage)
{
  g_free (gximage->data);
  if (gximage->gc)
    gdk_gc_destroy (gximage->gc);
  g_free (gximage);
}

/****************************************************************/


/*  Function definitions  */

void
gximage_init ()
{
  gximage = create_gximage (g_visual, GXIMAGE_WIDTH, GXIMAGE_HEIGHT);
}

void
gximage_free ()
{
  delete_gximage (gximage);
}

guchar*
gximage_get_data ()
{
  return gximage->data;
}

int
gximage_get_bpp ()
{
  return 3;
}

int
gximage_get_bpl ()
{
  return 3 * GXIMAGE_WIDTH;
}

int
gximage_get_byte_order ()
{
  return GDK_MSB_FIRST;
}

void
gximage_put (GdkWindow *win, int x, int y, int w, int h, int xdith, int ydith)
{
    /*  create the GC if it doesn't yet exist  */
  if (!gximage->gc)
    {
      gximage->gc = gdk_gc_new (win);
      gdk_gc_set_exposures (gximage->gc, TRUE);
    }

#ifdef DEBUG
    if(0) {
    FILE * fp = fopen( "dbg_test.ppm","w" );
    int samples = 4;
    int w_ = w-x,
        h_ = h-y;
    char *buf_ = gximage->data;
    char * text = malloc(1024);
    size_t pt = 0;
    sprintf( text, "P7\n# %s:%d %s()\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", __FILE__,__LINE__,__func__, w_, h_, samples );
    int len = strlen( text ), i;
    do { fputc ( text[pt++] , fp);
    } while (--len); pt = 0;
    for( i = 0; i < h_ * w_ * samples; ++i )
      fputc ( buf_[pt++], fp);
    fflush( fp ); fclose( fp ); free( text );
    }
#endif

  gdk_draw_rgb_image_dithalign (win,
				gximage->gc,
				x,
				y,
				w,
				h,
				/* todo: make configurable */
				GDK_RGB_DITHER_MAX,
				gximage->data,
				GXIMAGE_WIDTH * 3,
				xdith, ydith);
}


