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
#include <string.h>
#include <math.h>
#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../actionarea.h"
#include "../brightness_contrast.h"
#include "../canvas.h"
#include "../drawable.h"
#include "float16.h"
#include "bfp.h"
#include "../general.h"
#include "../gimage_mask.h"
#include "../gdisplay.h"
#include "../image_map.h"
#include "../interface.h"
#include "../pixelrow.h"
#include "../minimize.h"
#include "../pixelarea.h"

#define TEXT_WIDTH 45
#define TEXT_HEIGHT 25
#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 40

#define BRIGHTNESS_SLIDER 0x1
#define CONTRAST_SLIDER   0x2
#define BRIGHTNESS_TEXT   0x4
#define CONTRAST_TEXT     0x8
#define ALL               0xF
  
typedef struct BrightnessContrast BrightnessContrast;

struct BrightnessContrast
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct BrightnessContrastDialog BrightnessContrastDialog;

struct BrightnessContrastDialog
{
  GtkWidget   *shell;
  GtkWidget   *gimage_name;
  GtkWidget   *brightness_text;
  GtkWidget   *contrast_text;
  GtkAdjustment  *brightness_data;
  GtkAdjustment  *contrast_data;

  CanvasDrawable *drawable;
  ImageMap     image_map;

  double       brightness;
  double       contrast;

  gint         preview;
};

/*  brightness contrast action functions  */

static void   brightness_contrast_button_press   (Tool *, GdkEventButton *, gpointer);
static void   brightness_contrast_button_release (Tool *, GdkEventButton *, gpointer);
static void   brightness_contrast_motion         (Tool *, GdkEventMotion *, gpointer);
static void   brightness_contrast_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   brightness_contrast_control        (Tool *, int, gpointer);

static BrightnessContrastDialog *  brightness_contrast_new_dialog  (void);
static void   brightness_contrast_update                  (BrightnessContrastDialog *, int);
static void   brightness_contrast_preview                 (BrightnessContrastDialog *);
static void   brightness_contrast_ok_callback             (GtkWidget *, gpointer);
static void   brightness_contrast_cancel_callback         (GtkWidget *, gpointer);
static gint   brightness_contrast_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void   brightness_contrast_preview_update          (GtkWidget *, gpointer);
static void   brightness_contrast_brightness_scale_update (GtkAdjustment *, gpointer);
static void   brightness_contrast_contrast_scale_update   (GtkAdjustment *, gpointer);
static void   brightness_contrast_brightness_text_update  (GtkWidget *, gpointer);
static void   brightness_contrast_contrast_text_update    (GtkWidget *, gpointer);
static gint   brightness_contrast_brightness_text_check (char *, BrightnessContrastDialog *);
static gint   brightness_contrast_contrast_text_check (char *, BrightnessContrastDialog *);

static void *brightness_contrast_options = NULL;
static BrightnessContrastDialog *brightness_contrast_dialog = NULL;

static Argument * brightness_contrast_invoker  (Argument *);

static void brightness_contrast_funcs (Tag); 

/* data type function pointers */
typedef void (*BrightnessContrastFunc)(PixelArea *, PixelArea *, void *);
static BrightnessContrastFunc brightness_contrast;

static void brightness_contrast_u8 (PixelArea *, PixelArea *, void *);

static void brightness_contrast_u16 (PixelArea *, PixelArea *, void *);

static void brightness_contrast_float (PixelArea *, PixelArea *, void *);

static void brightness_contrast_float16 (PixelArea *, PixelArea *, void *);

static void brightness_contrast_bfp (PixelArea *, PixelArea *, void *);

static void
brightness_contrast_funcs (Tag drawable_tag)
{
  switch (tag_precision (drawable_tag))
  {
  case PRECISION_U8:
    brightness_contrast = brightness_contrast_u8;
    break;
  case PRECISION_U16:
    brightness_contrast = brightness_contrast_u16;
    break;
  case PRECISION_FLOAT:
    brightness_contrast = brightness_contrast_float;
    break;
  case PRECISION_FLOAT16:
    brightness_contrast = brightness_contrast_float16;
    break;
  case PRECISION_BFP:
    brightness_contrast = brightness_contrast_bfp;
    break;
  default:
    brightness_contrast = NULL;
    break; 
  }
}

/* 
Brightness Contrast Functions

Note on the calculation of the contrast
   
Gain is computed from the contrast setting as follows:

     if -1 <= contrast < 0 ,  0 <= gain < 1
     if contrast = 0 ; gain = 1  or no change
     if 0 > contrast < 1 , 1 < gain < infinity
     
*/

static void
brightness_contrast_u8 (PixelArea *src_area,
		        PixelArea *dest_area,
		        void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  guchar *src, *dest;
  guint8 *s, *d;
  int has_alpha;
  int alpha;
  int w, h, b;
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) user_data;
  gfloat brightness = 255.0f * 0.392f * (gfloat)bcd->brightness;
  gfloat contrast = (gfloat)bcd->contrast;
  gfloat tmp, gain = 1.0f ;
  gfloat dist_shift;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);
  
  if (contrast != 0.0f)
  {
     if (contrast > 0.0f)
     {
        gain = 1.0f / (1.0f - contrast) ;
     }
     else if (contrast < 0.0f)
     {
        gain = 1.0f + contrast ;
     }
  }
  
  /* shift after gain application is 1/2(gain * max_value - max_value), max_value = 2^8-1 */
  /* we also pre-add the rounding factor */
  dist_shift = (gain * 127.5f - 127.5) - 0.5f ;

  while (h--)
  {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);

      while (w--)
	  {
        for (b = 0; b < alpha; b++)
        {
            tmp = brightness + s[b] ;         /* brightness correction */
            tmp = gain*tmp - dist_shift ;     /* contrast correction */
             
            if (tmp < 0.0f)                   
              tmp = 0.0f ;
            else if (tmp > 255.0f)
              tmp = 255.0f ;

            d[b] = (guint8)(tmp) ;
        } 

	    if (has_alpha)
	       d[alpha] = s[alpha];

	    s += src_num_channels;
	    d += dest_num_channels; 
	  }

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
  }
}

static void
brightness_contrast_u16 (PixelArea *src_area,
		        PixelArea *dest_area,
		        void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha;
  int alpha;
  int w, h, b;
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) user_data;
  gfloat brightness = 65535.0f * 0.392f * (gfloat)bcd->brightness;
  gfloat contrast = (gfloat)bcd->contrast;
  gfloat tmp, gain;
  gfloat dist_shift;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);

  gain = 1.0f ;

  if (contrast != 0.0f)
  {
     if (contrast > 0.0f)
     {
        gain = 1.0f / (1.0f - contrast) ;
     }
     else if (contrast < 0.0f)
     {
        gain = 1.0f + contrast ;
     }
  }
  
  /* shift after gain application is 1/2(gain * max_value - max_value), max_value = 2^16-1 */
  /* we also pre-add the rounding factor */
  dist_shift = (gain * 32767.5f - 32767.5f) - 0.5f ;

  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);

      while (w--)
      {
          for (b = 0; b < alpha; b++)
          {
              tmp = brightness + s[b] ;         /* brightness correction */
              tmp = gain*tmp - dist_shift ;     /* contrast correction */
             
              if (tmp < 0.0f)                   
                tmp = 0.0f ;
              else if (tmp > 65535.0f)
                  tmp = 65535.0f ;

              d[b] = (guint16)(tmp) ;
          } 

          if (has_alpha)
            d[alpha] = s[alpha];

          s += src_num_channels;
          d += dest_num_channels; 
      }

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
brightness_contrast_float (PixelArea *src_area,
		        PixelArea *dest_area,
		        void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  guchar *src, *dest;
  gfloat *s, *d;
  int has_alpha;
  int alpha;
  int w, h, b;
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) user_data;
  gfloat brightness = 0.392f * (gfloat)bcd->brightness;
  gfloat contrast = (gfloat)bcd->contrast;
  gfloat tmp, gain = 1.0f ;
  gfloat dist_shift;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);


  if (contrast != 0.0f)
  {
     if (contrast > 0.0f)
     {
        gain = 1.0f / (1.0f - contrast) ;
     }
     else if (contrast < 0.0f)
     {
        gain = 1.0f + contrast ;
     }
  }
  
  /* shift after gain application is 1/2(gain * max_value - max_value), max_value = 1.0 */
  dist_shift = gain * 0.5f - 0.5f ;

  while (h--)
  {
      s = (gfloat*)src;
      d = (gfloat*)dest;
      w = pixelarea_width (src_area);

      while (w--)
	  {
        for (b = 0; b < alpha; b++)
        {
           tmp = brightness + s[b] ;         /* brightness correction */
           tmp = gain*tmp - dist_shift ;     /* contrast correction */
             
           if (tmp < 0.0f)                   
             tmp = 0.0f ;
           else if (tmp > 1.0f)
              tmp = 1.0f ;

           d[b] = tmp ;
        } 

	  if (has_alpha)
	    d[alpha] = s[alpha];

	  s += src_num_channels;
	  d += dest_num_channels; 
 	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
  }
}

static void
brightness_contrast_float16 (PixelArea *src_area,
		        PixelArea *dest_area,
		        void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha;
  int alpha;
  int w, h, b;
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) user_data;
  gfloat brightness = 0.392f * (gfloat)bcd->brightness;
  gfloat contrast = (gfloat)bcd->contrast;
  ShortsFloat u;
  gfloat sb ;
  gfloat tmp, gain = 1.0f ;
  gfloat dist_shift;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);


  if (contrast != 0.0f)
  {
     if (contrast > 0.0f)
     {
        gain = 1.0f / (1.0f - contrast) ;
     }
     else if (contrast < 0.0f)
     {
        gain = 1 + contrast ;
     }
  }
  
  /* shift after gain application is 1/2(gain * max - max), max = 1.0 */
  dist_shift = gain * 0.5f - 0.5f ;

  while (h--)
  {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);

      while (w--)
	  {
         for (b = 0; b < alpha; b++)
         {
           sb = FLT (s[b], u);
           tmp = brightness + sb ;         /* brightness correction */
           tmp = gain*tmp - dist_shift ;   /* contrast correction */
                 
           if (tmp < 0.0f)                   
              tmp = 0.0f ;
           else if (tmp > 1.0f)
              tmp = 1.0f ;

           d[b] = FLT16 (tmp, u);

          if (has_alpha)
            d[alpha] = s[alpha];

          s += src_num_channels;
          d += dest_num_channels; 
        }

        src += pixelarea_rowstride (src_area);
        dest += pixelarea_rowstride (dest_area);
      }
  }
}

static void
brightness_contrast_bfp (PixelArea *src_area,
		        PixelArea *dest_area,
		        void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha;
  int alpha;
  int w, h, b;
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) user_data;
  gfloat brightness = BFP_MAX_IN_FLOAT * 0.392f * (gfloat)bcd->brightness;
  gfloat contrast = (gfloat)bcd->contrast;
  gfloat tmp, gain = 1.0f ;
  gfloat dist_shift;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);

  if (contrast != 0.0f)
  {
     if (contrast > 0.0f)
     {
        gain = 1.0f / (1.0f - contrast) ;
     }
     else if (contrast < 0.0f)
     {
        gain = 1 + contrast ;
     }
  }
  
  dist_shift = BFP_MAX_IN_FLOAT*(gain * 0.5f - 0.5f) ;

  while (h--)
  {
     s = (guint16*)src;
     d = (guint16*)dest;
     w = pixelarea_width (src_area);

     while (w--)
	 {
          /* note: 32768. is ONE_BFP in float */
        for (b = 0; b < alpha; b++)
        {
              tmp = brightness + s[b] ;         /* brightness correction */
              tmp = gain*tmp - dist_shift ;     /* contrast correction */
             
              if (tmp < 0.0f)                   
                tmp = 0.0f ;
              else if (tmp > BFP_MAX)
                  tmp = BFP_MAX ;

              d[b] = (guint16)(tmp) ;

               if (has_alpha)
	              d[alpha] = s[alpha];

	           s += src_num_channels;
	           d += dest_num_channels; 
	    }

        src += pixelarea_rowstride (src_area);
        dest += pixelarea_rowstride (dest_area);
     }
  }
}

static void
brightness_contrast_button_press (Tool           *tool,
				  GdkEventButton *bevent,
				  gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
brightness_contrast_button_release (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr)
{
}

static void
brightness_contrast_motion (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
}

static void
brightness_contrast_cursor_update (Tool           *tool,
				   GdkEventMotion *mevent,
				   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
brightness_contrast_control (Tool     *tool,
			     int       action,
			     gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (brightness_contrast_dialog)
	{
	  active_tool->preserve = TRUE;
          image_map_abort (brightness_contrast_dialog->image_map);
          active_tool->preserve = FALSE;
	  brightness_contrast_dialog->image_map = NULL;
	  brightness_contrast_cancel_callback (NULL, (gpointer) brightness_contrast_dialog);
	}
      break;
    }
}

Tool *
tools_new_brightness_contrast ()
{
  Tool * tool;
  BrightnessContrast * private;

  /*  The tool options  */
  if (!brightness_contrast_options)
    brightness_contrast_options = tools_register_no_options (BRIGHTNESS_CONTRAST,
							     _("Brightness-Contrast Options"));

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (BrightnessContrast *) g_malloc_zero (sizeof (BrightnessContrast));

  tool->type = BRIGHTNESS_CONTRAST;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = brightness_contrast_button_press;
  tool->button_release_func = brightness_contrast_button_release;
  tool->motion_func = brightness_contrast_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = brightness_contrast_cursor_update;
  tool->control_func = brightness_contrast_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_brightness_contrast (Tool *tool)
{
  BrightnessContrast * bc;

  bc = (BrightnessContrast *) tool->private;

  /*  Close the color select dialog  */
  if (brightness_contrast_dialog)
    brightness_contrast_cancel_callback (NULL, (gpointer) brightness_contrast_dialog);

  g_free (bc);
}

void
brightness_contrast_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  CanvasDrawable *drawable;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Brightness-Contrast does not operate on indexed drawables."));
      return;
    }

  /*  The brightness-contrast dialog  */
  if (!brightness_contrast_dialog)
    brightness_contrast_dialog = brightness_contrast_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (brightness_contrast_dialog->shell))
      gtk_widget_show (brightness_contrast_dialog->shell);

  drawable = gimage_active_drawable (gdisp->gimage);
  
  /* Set up the function pointers for our data type */
  brightness_contrast_funcs (drawable_tag (drawable)); 
  
  /*  Initialize dialog fields  */
  brightness_contrast_dialog->image_map = NULL;
  brightness_contrast_dialog->brightness = 0.0;
  brightness_contrast_dialog->contrast = 0.0;

  brightness_contrast_dialog->drawable = drawable;
  brightness_contrast_dialog->image_map = image_map_create (gdisp_ptr,
							    brightness_contrast_dialog->drawable);

  brightness_contrast_update (brightness_contrast_dialog, ALL);
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", brightness_contrast_ok_callback, NULL, NULL },
  { "Cancel", brightness_contrast_cancel_callback, NULL, NULL }
};

static BrightnessContrastDialog *
brightness_contrast_new_dialog ()
{
  BrightnessContrastDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkObject *data;

  bcd = g_malloc_zero (sizeof (BrightnessContrastDialog));
  bcd->preview = TRUE;

  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "brightness_contrast", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (bcd->shell), _("Brightness-Contrast"));
  minimize_register(bcd->shell);
  
  /* handle wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (brightness_contrast_delete_callback),
		      bcd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the brightness scale widget  */
  label = gtk_label_new (_("Brightness"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -1.0, 1.0, .001, .01, .01);
  bcd->brightness_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) brightness_contrast_brightness_scale_update,
		      bcd);

  bcd->brightness_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->brightness_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->brightness_text, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->brightness_text), "changed",
		      (GtkSignalFunc) brightness_contrast_brightness_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->brightness_text);
  gtk_widget_show (slider);


  /*  Create the contrast scale widget  */
  label = gtk_label_new (_("Contrast"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -1.0, 1.00, .001, .01, .01);
  bcd->contrast_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 3);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) brightness_contrast_contrast_scale_update,
		      bcd);

  bcd->contrast_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->contrast_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->contrast_text, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->contrast_text), "changed",
		      (GtkSignalFunc) brightness_contrast_contrast_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->contrast_text);
  gtk_widget_show (slider);


  /*  Horizontal box for preview and preserve luminosity toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), bcd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) brightness_contrast_preview_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);


  /*  The action area  */
  action_items[0].label = _(action_items[0].label);
  action_items[1].label = _(action_items[1].label);
  action_items[0].user_data = bcd;
  action_items[1].user_data = bcd;
  build_action_area (GTK_DIALOG (bcd->shell), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
brightness_contrast_update (BrightnessContrastDialog *bcd,
			    int                       update)
{
  char text[12];

  if (update & BRIGHTNESS_SLIDER)
    {
      bcd->brightness_data->value = bcd->brightness;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->brightness_data), "value_changed");
    }
  if (update & CONTRAST_SLIDER)
    {
      bcd->contrast_data->value = bcd->contrast;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->contrast_data), "value_changed");
    }
  if (update & BRIGHTNESS_TEXT)
    {
      sprintf (text, "%0.3f", bcd->brightness);
      gtk_entry_set_text (GTK_ENTRY (bcd->brightness_text), text);
    }
  if (update & CONTRAST_TEXT)
    {
      sprintf (text, "%0.3f", bcd->contrast);
      gtk_entry_set_text (GTK_ENTRY (bcd->contrast_text), text);
    }
}

static void
brightness_contrast_preview (BrightnessContrastDialog *bcd)
{
  if (!bcd->image_map)
    g_message (_("brightness_contrast_preview(): No image map"));
  active_tool->preserve = TRUE;
  image_map_apply (bcd->image_map, brightness_contrast, (void *) bcd);
  active_tool->preserve = FALSE;
}

static void
brightness_contrast_ok_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  active_tool->preserve = TRUE;

  if (!bcd->preview)
  {
    /*(*brightness_contrast_init_transfers) ((void *)bcd); */
    image_map_apply (bcd->image_map, brightness_contrast, (void *) bcd);
  }

  if (bcd->image_map)
    image_map_commit (bcd->image_map);

  active_tool->preserve = FALSE;

  bcd->image_map = NULL;
}

static gint
brightness_contrast_delete_callback (GtkWidget *w,
				     GdkEvent *e,
				     gpointer d)
{
  brightness_contrast_cancel_callback (w, d);

  return TRUE;
}

static void
brightness_contrast_cancel_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  if (bcd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (bcd->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  bcd->image_map = NULL;
}

static void
brightness_contrast_preview_update (GtkWidget *w,
				    gpointer   data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      bcd->preview = TRUE;
      brightness_contrast_preview (bcd);
    }
  else
  {
    bcd->preview = FALSE;
    active_tool->preserve = TRUE;
    image_map_remove(bcd->image_map);
    active_tool->preserve = FALSE;
    gdisplays_flush ();
  }
}

static void
brightness_contrast_brightness_scale_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->brightness != adjustment->value)
    {
      bcd->brightness = adjustment->value;
      brightness_contrast_update (bcd, BRIGHTNESS_TEXT);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_contrast_scale_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->contrast != adjustment->value)
    {
      bcd->contrast = adjustment->value;
      brightness_contrast_update (bcd, CONTRAST_TEXT);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_brightness_text_update (GtkWidget *w,
					    gpointer   data)
{
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) data;
  char *str = gtk_entry_get_text (GTK_ENTRY (w));
  
  if (brightness_contrast_brightness_text_check (str, bcd))
  {
      brightness_contrast_update (bcd, BRIGHTNESS_SLIDER);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
  }
}

static gint 
brightness_contrast_brightness_text_check (
				char *str, 
				BrightnessContrastDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), -1.0, 1.0);
  if (bcd->brightness != value)
  {
      bcd->brightness = value;
      return TRUE;
  }
  return FALSE;
}

static void
brightness_contrast_contrast_text_update (GtkWidget *w,
					  gpointer   data)
{
  BrightnessContrastDialog *bcd = (BrightnessContrastDialog *) data;
  char *str = gtk_entry_get_text (GTK_ENTRY (w));

  if (brightness_contrast_contrast_text_check (str, bcd))
    {
      brightness_contrast_update (bcd, CONTRAST_SLIDER);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static gint 
brightness_contrast_contrast_text_check  (
				char *str, 
				BrightnessContrastDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), -1.0, 1.0);
  if (bcd->contrast != value)
  {
      bcd->contrast = value;
      return TRUE;
  }
  return FALSE;
}


/*  The brightness_contrast procedure definition  */
ProcArg brightness_contrast_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The Drawable"
  },
  { PDB_INT32,
    "brightness",
    "brightness adjustment: (-127 <= brightness <= 127)"
  },
  { PDB_INT32,
    "contrast",
    "constrast adjustment: (-127 <= contrast <= 127)"
  }
};

ProcRecord brightness_contrast_proc =
{
  "gimp_brightness_contrast",
  "Modify brightness/contrast in the specified drawable",
  "This procedures allows the brightness and contrast of the specified drawable to be modified.  Both 'brightness' and 'contrast' parameters are defined between -127 and 127.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  brightness_contrast_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brightness_contrast_invoker } },
};


static Argument *
brightness_contrast_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  int int_value;
  BrightnessContrastDialog bcd;
  GImage *gimage;
  int brightness;
  int contrast;
  int x1, y1, x2, y2;
  void *pr;
  CanvasDrawable *drawable;

  drawable    = NULL;
  brightness  = 0;
  contrast    = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  /*  brightness  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value < -127 || int_value > 127)
	success = FALSE;
      else
	brightness = int_value;
    }
  /*  contrast  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value < -127 || int_value > 127)
	success = FALSE;
      else
	contrast = int_value;
    }

  /*  arrange to modify the brightness/contrast  */
  if (success)
    {
      bcd.brightness = brightness;
      bcd.contrast = contrast;
        
      brightness_contrast_funcs(drawable_tag (drawable));
      
      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
         
      pixelarea_init (&src_area, drawable_data (drawable),
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	(*brightness_contrast) (&src_area, &dest_area, (void *) &bcd);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&brightness_contrast_proc, success);
}
