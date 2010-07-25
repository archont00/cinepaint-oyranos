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
#include "../gamma_expose.h"
#include "../canvas.h"
#include "../drawable.h"
#include "float16.h"
#include "../general.h"
#include "../gimage_mask.h"
#include "../gdisplay.h"
#include "../image_map.h"
#include "../interface.h"
#include "../pixelarea.h"
#include "../pixelrow.h"

#define TEXT_WIDTH 45
#define TEXT_HEIGHT 25
#define SLIDER_WIDTH 200
#define SLIDER_HEIGHT 35

#define GAMMA_SLIDER 0x1
#define EXPOSE_SLIDER 0x2
#define GAMMA_TEXT   0x4
#define EXPOSE_TEXT  0x8
#define ALL          0xF

typedef struct GammaExpose GammaExpose;

struct GammaExpose
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct GammaExposeDialog GammaExposeDialog;

struct GammaExposeDialog
{
  GtkWidget   *shell;
  GtkWidget   *gimage_name;
  GtkWidget   *gamma_text;
  GtkWidget   *expose_text;
  GtkAdjustment  *gamma_data;
  GtkAdjustment  *expose_data;

  CanvasDrawable *drawable;
  ImageMap     image_map;

  double       gamma;
  double       expose;
  double       old_gamma;
  double       old_expose;

  gint         preview;
  gint         forceexpose;

  GDisplay    *gdisp;
};

/* gamma luts */
static guint16 *gamma_lut16 ;
static guint8  *gamma_lut8  ;

/* static PixelRow expose_luti */

/*  gamma expose action functions  */

static void   gamma_expose_button_press   (Tool *, GdkEventButton *, gpointer);
static void   gamma_expose_button_release (Tool *, GdkEventButton *, gpointer);
static void   gamma_expose_motion         (Tool *, GdkEventMotion *, gpointer);
static void   gamma_expose_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   gamma_expose_control        (Tool *, int, gpointer);

static void   gamma_expose_new_dialog              (GammaExposeDialog *);
static void   gamma_expose_update                  (GammaExposeDialog *, int);
static void   gamma_expose_preview                 (GammaExposeDialog *);
static void   gamma_expose_ok_callback             (GtkWidget *, gpointer);
static void   gamma_expose_cancel_callback         (GtkWidget *, gpointer);
static gint   gamma_expose_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void   gamma_expose_preview_update          (GtkWidget *, gpointer);
static void   gamma_expose_force_update          (GtkWidget *, gpointer);
static void   gamma_expose_gamma_scale_update (GtkAdjustment *, gpointer);
static void   gamma_expose_expose_scale_update   (GtkAdjustment *, gpointer);
static void   gamma_expose_gamma_text_update  (GtkWidget *, gpointer);
static void   gamma_expose_expose_text_update    (GtkWidget *, gpointer);
static gint   gamma_expose_gamma_text_check (char *, GammaExposeDialog *);
static gint   gamma_expose_expose_text_check (char *, GammaExposeDialog *);

static void *gamma_expose_options = NULL;
static GammaExposeDialog *gamma_expose_dialog = NULL;

static Argument * gamma_expose_invoker  (Argument *);

static void gamma_expose_funcs (Tag); 

/* data type function pointers */
typedef void (*GammaExposeInitTransfersFunc)(void *);
static GammaExposeInitTransfersFunc gamma_expose_init_transfers;
typedef void (*GammaExposeFunc)(PixelArea *, PixelArea *, void *);
static GammaExposeFunc gamma_expose;

static void gamma_expose_init_transfers_u8 (void *);
static void gamma_expose_u8 (PixelArea *, PixelArea *, void *);

static void gamma_expose_init_transfers_u16 (void *);
static void gamma_expose_u16 (PixelArea *, PixelArea *, void *);

static void gamma_expose_float (PixelArea *, PixelArea *, void *);
static void gamma_expose_float16 (PixelArea *, PixelArea *, void *);

/* static void gamma_expose_init_transfers_bfp (void *); */
static void gamma_expose_bfp (PixelArea *, PixelArea *, void *);

/*  Initalize gamma look up tables for integer data types */

static void
gamma_expose_funcs (Tag drawable_tag)
{
  gamma_lut16 = NULL ;
  gamma_lut8 =  NULL ;

  switch (tag_precision (drawable_tag))
  {
  case PRECISION_U8:
    gamma_expose = gamma_expose_u8;
    gamma_expose_init_transfers = gamma_expose_init_transfers_u8;
    break;
  case PRECISION_U16:
    gamma_expose = gamma_expose_u16;
    gamma_expose_init_transfers = gamma_expose_init_transfers_u16;
    break;
  case PRECISION_FLOAT:
    gamma_expose = gamma_expose_float;
    gamma_expose_init_transfers = NULL ; 
    break;
  case PRECISION_FLOAT16:
    gamma_expose = gamma_expose_float16;
    gamma_expose_init_transfers = NULL ; 
    break;
  case PRECISION_BFP:
    gamma_expose = gamma_expose_bfp;
    gamma_expose_init_transfers = NULL ; 
    break;
  default:
    gamma_expose = NULL;
    gamma_expose_init_transfers = NULL;
    break; 
  }
}


static void 
gamma_expose_init_transfers_u8 (void * user_data)
{
  gint i;
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  gfloat gamma = 1.0f/(gfloat)bcd->gamma;
  gfloat expose = powf(2.0f,(gfloat)bcd->expose)/255.0f;
  gfloat tmp ;
  
  gamma_lut8 = (guint8 *)g_malloc ( sizeof (guint8) * 256 );

  for (i = 0; i < 256; i++)
  {
    tmp = 0.5f + 255.0f * powf((gfloat)i*expose, gamma); 
    
    if (tmp < 0.5f)
      gamma_lut8[i] = 0 ;
    else if (tmp >= 255.0f)
      gamma_lut8[i] = 255 ;
    else
      gamma_lut8[i] = (guint8)(tmp) ;
  }
}

static void 
gamma_expose_init_transfers_u16 (void * user_data)
{
  gint i;
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  gfloat gamma = 1.0f/(gfloat)bcd->gamma;
  gfloat expose = powf(2.0f,(gfloat)bcd->expose)/65535.0f;
  gfloat tmp ;
 
  gamma_lut16 = (guint16 *)g_malloc ( sizeof (guint16) * 65536 );

  for (i = 0; i < 65536; i++)
  {
    tmp = 0.5f + 65535.f * powf( (gfloat)i*expose, gamma); 
    
    if (tmp < 0.5f)
      gamma_lut16[i] = 0 ;
    else if (tmp > 65535.0f)
      gamma_lut16[i] = 65535 ;
    else
      gamma_lut16[i] = (guint16)(tmp) ;
  } 
}

#if 0 

static void 
gamma_expose_init_transfers_bfp (void * user_data)
{
  gint i;
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  guint16 *gamma_data = (guint16*) pixelrow_data (&gamma_lut);
  guint16 *expose_data = (guint16*) pixelrow_data (&expose_lut);
  Tag lut_tag = tag_new (PRECISION_BFP, FORMAT_GRAY, ALPHA_NO);
  gfloat gamma = bcd->gamma;
  gfloat expose = bcd->expose;
 
  if (!gamma_data && !expose_data)
  {
    gamma_data = (guint16 *)g_malloc ( sizeof (guint16) * 65536 );
    expose_data = (guint16 *)g_malloc ( sizeof (guint16) * 65536 );
  }
  
  for (i = 0; i < 65536; i++)
    gamma_data[i] = (guint16) (65535.0 * gamma_func ( (gdouble)i/65535.0,
				      gamma*2.0));
  for (i = 0; i < 65536; i++)
    expose_data[i] = (guint16) (65535.0 * expose_func ( (double)i/65535.0,
				      expose*2.0));
  
  pixelrow_init (&gamma_lut, lut_tag, (guchar*)gamma_data, 65536); 
  pixelrow_init (&expose_lut, lut_tag, (guchar*)expose_data, 65536); 
}
#endif 

static void gamma_expose_free_transfers (void)
{

  if (gamma_lut8) 
  {
    g_free (gamma_lut8);
    gamma_lut8 = NULL ;
  }

  if (gamma_lut16)
  {
    g_free(gamma_lut16) ;
    gamma_lut16 = NULL ;
  }
}


static void
gamma_expose_u8 (PixelArea *src_area,
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

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);

  while (h--)
    {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);

      while (w--)
      {
          /* Added by IMAGEWORKS thedoug (01/02) */
          for (b = 0; b < alpha; b++)
            d[b] = gamma_lut8[s[b]] ; 

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
gamma_expose_u16 (PixelArea *src_area,
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

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);

  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);

      while (w--)
      {
          // Added by IMAGEWORKS thedoug (01/02) 
          for (b = 0; b < alpha; b++)
            d[b] = gamma_lut16[s[b]] ;

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
gamma_expose_float (PixelArea *src_area,
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
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  gfloat gamma = 1.f/(gfloat)bcd->gamma;
  gfloat expose = powf(2.0f,(gfloat)bcd->expose);

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (gfloat*)src;
      d = (gfloat*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  for (b = 0; b < alpha; b++) {
          d[b] = powf(s[b] * expose, gamma);

          if (bcd->forceexpose) { /* clamp */
              if (d[b] > 1.0f) d[b] = 1.0f;
              else if (d[b] < 0.0f) d[b] = 0.0f;
          }
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
gamma_expose_float16 (PixelArea *src_area,
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
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  gfloat gamma = 1.f/(gfloat)bcd->gamma;
  gfloat expose = powf(2.0f,(gfloat)bcd->expose);
  ShortsFloat u;
  gfloat sb, db;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);
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
	    db = powf(sb*expose, gamma);
	    d[b] = FLT16 (db, u);
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
gamma_expose_bfp (PixelArea *src_area,
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
  GammaExposeDialog *bcd = (GammaExposeDialog *) user_data;
  gfloat gamma = 1.f/(gfloat)bcd->gamma;
  gfloat expose = pow(2.0,(gfloat)bcd->expose)/32768.f ;
  gfloat tmp ;

  src = (guchar *)pixelarea_data (src_area);
  dest = (guchar *)pixelarea_data (dest_area);
  
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  h = pixelarea_height (src_area);

  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  // Added by IMAGEWORKS thedoug (01/02)
	  for (b = 0; b < alpha; b++)
      {
        tmp =32768.f * pow(s[b]*expose, gamma) ;
	    d[b] = CLAMP(tmp, 0, 65535);
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
gamma_expose_button_press (Tool           *tool,
				  GdkEventButton *bevent,
				  gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
gamma_expose_button_release (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr)
{
}

static void
gamma_expose_motion (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
}

static void
gamma_expose_cursor_update (Tool           *tool,
				   GdkEventMotion *mevent,
				   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
gamma_expose_control (Tool     *tool,
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
      if (gamma_expose_dialog)
        {
          active_tool->preserve = TRUE;
          image_map_abort (gamma_expose_dialog->image_map);
          active_tool->preserve = FALSE;
          gamma_expose_free_transfers();
          gamma_expose_dialog->image_map = NULL;
          gamma_expose_cancel_callback (NULL, (gpointer) gamma_expose_dialog);
        }
      break;
    }
}

Tool *
tools_new_gamma_expose ()
{
  Tool * tool;
  GammaExpose * private;

  /*  The tool options  */
  if (!gamma_expose_options)
    gamma_expose_options = tools_register_no_options (GAMMA_EXPOSE,
							     "gamma-expose Options");

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (GammaExpose *) g_malloc_zero (sizeof (GammaExpose));

  tool->type = GAMMA_EXPOSE;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = gamma_expose_button_press;
  tool->button_release_func = gamma_expose_button_release;
  tool->motion_func = gamma_expose_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = gamma_expose_cursor_update;
  tool->control_func = gamma_expose_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_gamma_expose (Tool *tool)
{
  GammaExpose * bc;

  bc = (GammaExpose *) tool->private;

  /*  Close the color select dialog  */
  if (gamma_expose_dialog)
    gamma_expose_cancel_callback (NULL, (gpointer) gamma_expose_dialog);

  g_free (bc);
}

void
gamma_expose_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  CanvasDrawable *drawable;
  int init_dialog = gamma_expose_dialog ? 0:1;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("gamma-expose does not operate on indexed drawables.");
      return;
    }

  drawable = gimage_active_drawable (gdisp->gimage);
  
  /* Set up the function pointers for our data type */
  gamma_expose_funcs (drawable_tag (drawable));
  

  /*  The gamma-expose dialog  */
  if (init_dialog)
  { gamma_expose_dialog = g_malloc_zero (sizeof (GammaExposeDialog));
    /*  Initialize dialog fields  */
    gamma_expose_dialog->preview = TRUE;
    gamma_expose_dialog->forceexpose = FALSE;
    gamma_expose_dialog->image_map = NULL;
    gamma_expose_dialog->gdisp = gdisp;
  }
  else
  {
    gamma_expose_dialog->gdisp = gdisp;

    if (!GTK_WIDGET_VISIBLE (gamma_expose_dialog->shell))
      gtk_widget_show (gamma_expose_dialog->shell);
  }

    gamma_expose_dialog->gamma = gamma_expose_dialog->old_gamma = gdisp->gamma;
    gamma_expose_dialog->expose = gamma_expose_dialog->old_expose = gdisp->expose;
    gdisp->gamma = 1.0;
    gdisp->expose = 0.0;

  gamma_expose_dialog->drawable = drawable;
  gamma_expose_dialog->image_map = image_map_create (gdisp_ptr,
							    gamma_expose_dialog->drawable);

  if (init_dialog)
    gamma_expose_new_dialog (gamma_expose_dialog);

  gamma_expose_update (gamma_expose_dialog, ALL);

  if (gamma_expose_dialog->preview)
    gamma_expose_preview (gamma_expose_dialog);
}


/********************************/
/*  gamma expose dialog  */
/********************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", gamma_expose_ok_callback, NULL, NULL },
  { "Cancel", gamma_expose_cancel_callback, NULL, NULL }
};

static void
gamma_expose_new_dialog (GammaExposeDialog * bcd)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkWidget *forceexpose;
  GtkObject *data;

  { int i = 0;
    for (;i<2;++i)
      action_items[i].label = _(action_items[i].label);
  }
  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "gamma_expose",PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (bcd->shell), _("Expose Image"));
  
  /* handle wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (gamma_expose_delete_callback),
		      bcd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the expose scale widget  */
  label = gtk_label_new (_("expose"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -20, 20, .01, .01, 0.0);
  bcd->expose_data = GTK_ADJUSTMENT (data);
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
		      (GtkSignalFunc) gamma_expose_expose_scale_update,
		      bcd);

  bcd->expose_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->expose_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->expose_text, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->expose_text), "changed",
		      (GtkSignalFunc) gamma_expose_expose_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->expose_text);
  gtk_widget_show (slider);


  /*  Create the gamma scale widget  */
  label = gtk_label_new (_("gamma"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (2.2, 0, 10, .001, .01, 0.0);
  bcd->gamma_data = GTK_ADJUSTMENT (data);
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
		      (GtkSignalFunc) gamma_expose_gamma_scale_update,
		      bcd);

  bcd->gamma_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->gamma_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->gamma_text, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->gamma_text), "changed",
		      (GtkSignalFunc) gamma_expose_gamma_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->gamma_text);
  gtk_widget_show (slider);


  /*  Horizontal box for preview and preserve luminosity toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gamma_expose_preview_update,
		      bcd);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), bcd->preview);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  forceexpose = gtk_check_button_new_with_label (_("Clamp"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (forceexpose), bcd->forceexpose);
  gtk_box_pack_start (GTK_BOX (hbox), forceexpose, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (forceexpose), "toggled",
              (GtkSignalFunc) gamma_expose_force_update,
              bcd);

  gtk_widget_show (forceexpose);

  /*  The action area  */
  action_items[0].user_data = bcd;
  action_items[1].user_data = bcd;
  build_action_area (GTK_DIALOG (bcd->shell), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);
}

static void
gamma_expose_update (GammaExposeDialog *bcd,
			    int                       update)
{
  char text[12];

  if (update & GAMMA_SLIDER)
    {
      bcd->gamma_data->value = bcd->gamma;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->gamma_data), "value_changed");
    }
  if (update & EXPOSE_SLIDER)
    {
      bcd->expose_data->value = bcd->expose;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->expose_data), "value_changed");
    }
  if (update & GAMMA_TEXT)
    {
      sprintf (text, "%0.3f", bcd->gamma);
      gtk_entry_set_text (GTK_ENTRY (bcd->gamma_text), text);
    }
  if (update & EXPOSE_TEXT)
    {
      sprintf (text, "%0.3f", bcd->expose);
      gtk_entry_set_text (GTK_ENTRY (bcd->expose_text), text);
    }
}

static void
gamma_expose_preview (GammaExposeDialog *bcd)
{
  if (!bcd->image_map)
    g_message ("gamma_expose_preview(): No image map");

  active_tool->preserve = TRUE;

  if (gamma_expose_init_transfers != NULL)
      (*gamma_expose_init_transfers) ((void *)bcd); 

  image_map_apply (bcd->image_map, gamma_expose, (void *) bcd);
  active_tool->preserve = FALSE;
}

static void
gamma_expose_ok_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  active_tool->preserve = TRUE;

  if (!bcd->preview)
  {
    if (gamma_expose_init_transfers != NULL)
      (*gamma_expose_init_transfers) ((void *)bcd); 
    
    image_map_apply (bcd->image_map, gamma_expose, (void *) bcd);
  }

  if (bcd->image_map)
    image_map_commit (bcd->image_map);

  gamma_expose_free_transfers();
  active_tool->preserve = FALSE;
  bcd->image_map = NULL;
}

static gint
gamma_expose_delete_callback (GtkWidget *w,
				     GdkEvent *e,
				     gpointer d)
{
  gamma_expose_cancel_callback (w, d);

  return TRUE;
}

static void
gamma_expose_cancel_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  if (bcd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (bcd->image_map);

      active_tool->preserve = FALSE;
    }

  gamma_expose_free_transfers();
  bcd->gdisp->gamma = bcd->old_gamma;
  bcd->gdisp->expose = bcd->old_expose;
  gdisplays_flush ();

  bcd->image_map = NULL;
}

static void
gamma_expose_preview_update (GtkWidget *w,
				    gpointer   data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      bcd->preview = TRUE;
      gamma_expose_preview (bcd);
    }
  else
    bcd->preview = FALSE;
}

static void
gamma_expose_force_update (GtkWidget *w,
				    gpointer   data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      bcd->forceexpose = TRUE;
      gamma_expose_preview (bcd);
    }
  else
    bcd->forceexpose = FALSE;
}


static void
gamma_expose_gamma_scale_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) data;

  if (bcd->gamma != adjustment->value)
    {
      bcd->gamma = adjustment->value;
      gamma_expose_update (bcd, GAMMA_TEXT);

      if (bcd->preview)
	gamma_expose_preview (bcd);
    }
}

static void
gamma_expose_expose_scale_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  GammaExposeDialog *bcd;

  bcd = (GammaExposeDialog *) data;

  if (bcd->expose != adjustment->value)
    {
      bcd->expose = adjustment->value;
      gamma_expose_update (bcd, EXPOSE_TEXT);

      if (bcd->preview)
	gamma_expose_preview (bcd);
    }
}

static void
gamma_expose_gamma_text_update (GtkWidget *w,
					    gpointer   data)
{
  GammaExposeDialog *bcd = (GammaExposeDialog *) data;
  char *str = gtk_entry_get_text (GTK_ENTRY (w));
  
  if (gamma_expose_gamma_text_check (str, bcd))
  {
      gamma_expose_update (bcd, GAMMA_SLIDER);

      if (bcd->preview)
	gamma_expose_preview (bcd);
  }
}

static gint 
gamma_expose_gamma_text_check (
				char *str, 
				GammaExposeDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), -20, 20);
  if (bcd->gamma != value)
  {
      bcd->gamma = value;
      return TRUE;
  }
  return FALSE;
}

static void
gamma_expose_expose_text_update (GtkWidget *w,
					  gpointer   data)
{
  GammaExposeDialog *bcd = (GammaExposeDialog *) data;
  char *str = gtk_entry_get_text (GTK_ENTRY (w));

  if (gamma_expose_expose_text_check (str, bcd))
    {
      gamma_expose_update (bcd, EXPOSE_SLIDER);

      if (bcd->preview)
	gamma_expose_preview (bcd);
    }
}

static gint 
gamma_expose_expose_text_check  (
				char *str, 
				GammaExposeDialog *bcd
				)
{
  gfloat value;

  value = BOUNDS (atof (str), -20, 20);
  if (bcd->expose != value)
  {
      bcd->expose = value;
      return TRUE;
  }
  return FALSE;
}


/*  The gamma_expose procedure definition  */
ProcArg gamma_expose_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "gamma",
    "gamma adjustment: (-20 <= gamma <= 20)"
  },
  { PDB_INT32,
    "exposure",
    "exposure adjustment: (-20 <= exposure <= 20)"
  }
};

ProcRecord gamma_expose_proc =
{
  "gimp_gamma_expose",
  "Modify gamma/exposure in the specified drawable",
  "This procedures allows the gamma and exposure of the specified drawable to be modified.  Both 'gamma' and 'exposure' parameters are defined between -20 and 20.",
  "Alan Davidson",
  "Alan Davidson",
  "2001",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  gamma_expose_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gamma_expose_invoker } },
};


static Argument *
gamma_expose_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  int int_value;
  GammaExposeDialog bcd;
  GImage *gimage;
  int gamma;
  int expose;
  int x1, y1, x2, y2;
  void *pr;
  CanvasDrawable *drawable;

  drawable  = NULL;
  gamma     = 1.0;
  expose    = 0;

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

  /*  gamma  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value < -127 || int_value > 127)
        success = FALSE;
      else
        gamma = int_value;
    }
  /*  expose  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value < -127 || int_value > 127)
        success = FALSE;
      else
        expose = int_value;
    }

  /*  arrange to modify the gamma/expose  */
  if (success)
    {
      bcd.gamma = gamma;
      bcd.expose = expose;
        
      gamma_expose_funcs(drawable_tag (drawable));

      if (gamma_expose_init_transfers != NULL)
          (*gamma_expose_init_transfers)(&bcd);
      
      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
         
      pixelarea_init (&src_area, drawable_data (drawable),
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
        (*gamma_expose) (&src_area, &dest_area, (void *) &bcd);

      gamma_expose_free_transfers();
      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&gamma_expose_proc, success);
}
