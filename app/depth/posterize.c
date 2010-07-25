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
#include "../drawable.h"
#include "float16.h"
#include "../gdisplay.h"
#include "../image_map.h"
#include "../interface.h"
#include "../pixelarea.h"
#include "../posterize.h"
#include "../tag.h"
#include "../minimize.h"

#define TEXT_WIDTH 55

typedef struct Posterize Posterize;

struct Posterize
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct PosterizeDialog PosterizeDialog;

struct PosterizeDialog
{
  GtkWidget   *shell;
  GtkWidget   *levels_text;

  CanvasDrawable *drawable;
  ImageMap   image_map;
  int          levels;

  gint         preview;
};

/*  posterize action functions  */

static void   posterize_button_press   (Tool *, GdkEventButton *, gpointer);
static void   posterize_button_release (Tool *, GdkEventButton *, gpointer);
static void   posterize_motion         (Tool *, GdkEventMotion *, gpointer);
static void   posterize_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   posterize_control        (Tool *, int, gpointer);

static PosterizeDialog *  posterize_new_dialog          (void);
static void               posterize_preview             (PosterizeDialog *);
static void               posterize_ok_callback         (GtkWidget *, gpointer);
static void               posterize_cancel_callback     (GtkWidget *, gpointer);
static void               posterize_preview_update      (GtkWidget *, gpointer);
static void               posterize_levels_text_update  (GtkWidget *, gpointer);
static gint               posterize_delete_callback     (GtkWidget *, GdkEvent *, gpointer);

static void *posterize_options = NULL;
static PosterizeDialog *posterize_dialog = NULL;
static Argument * posterize_invoker (Argument *);

/* Posterize functions for different tags */
typedef void (*PosterizeFunc) (PixelArea *, PixelArea *, void *);
static PosterizeFunc posterize_func (Tag dest_tag);
static void posterize_u8 (PixelArea *, PixelArea *, void *);
static void posterize_u16 (PixelArea *, PixelArea *, void *);
static void posterize_float (PixelArea *, PixelArea *, void *);
static void posterize_float16 (PixelArea *, PixelArea *, void *);
static void posterize_bfp (PixelArea *, PixelArea *, void *);


/*  posterize machinery  */
static
PosterizeFunc
posterize_func (Tag dest_tag)
{
  switch (tag_precision (dest_tag))
  {
  case PRECISION_U8:
    return posterize_u8; 
  case PRECISION_U16:
    return posterize_u16; 
  case PRECISION_FLOAT:
    return posterize_float; 
  case PRECISION_FLOAT16:
    return posterize_float16;   
  case PRECISION_BFP:
    return posterize_bfp; 
  default:
    return NULL;
  } 
}

static void
posterize_u8 (PixelArea *src_area,
	   PixelArea *dest_area,
	   void        *user_data)
{
  PosterizeDialog *pd;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  guchar *src, *dest;
  guint8 *s, *d;
  gint alpha = tag_alpha (src_tag);
  gint has_alpha;
  gint s_num_channels = tag_num_channels (src_tag);
  gint d_num_channels = tag_num_channels (dest_tag);
  int w, h, b, i;
  double interval, half_interval;
  guint8 transfer[256];

  pd = (PosterizeDialog *) user_data;

  /*  Set the transfer array  */
  interval = 255.0 / (double) (pd->levels - 1);
  half_interval = interval / 2.0;

  for (i = 0; i < 256; i++)
    transfer[i] = (guint8) ((int) (((double) i + half_interval) / interval) * interval);

  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  while (h--)
    {
      w = pixelarea_width (src_area);
      s = (guint8*)src;
      d = (guint8*)dest;
      while (w--)
	{
	  for (b = 0; b < alpha; b++)
	    d[b] = transfer[s[b]];

	  if (has_alpha)
	    d[alpha] = s[alpha];
	   
	  s+= s_num_channels;
	  d+= d_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area); 
    }
}

static void
posterize_u16 (PixelArea *src_area,
	   PixelArea *dest_area,
	   void        *user_data)
{
  PosterizeDialog *pd;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  guchar *src, *dest;
  guint16 *s, *d;
  gint alpha = tag_alpha (src_tag);
  gint has_alpha;
  gint s_num_channels = tag_num_channels (src_tag);
  gint d_num_channels = tag_num_channels (dest_tag);
  int w, h, b, i;
  double interval, half_interval;
  guint16 transfer[65536];

  pd = (PosterizeDialog *) user_data;

  /*  Set the transfer array  */
  interval = 65535.0 / (double) (pd->levels - 1);
  half_interval = interval / 2.0;

  for (i = 0; i < 65536; i++)
    transfer[i] = (guint16) ((int) (((double) i + half_interval) / interval) * interval);

  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;
  
 
  while (h--)
    {
      w = pixelarea_width (src_area);
      s = (guint16*)src;
      d = (guint16*)dest;
      while (w--)
	{
	  for (b = 0; b < alpha; b++)
	    d[b] = transfer[s[b]];

	  if (has_alpha)
	    d[alpha] = s[alpha];
	   
	  s+= s_num_channels;
	  d+= d_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area); 
    }
}

static void
posterize_float (PixelArea *src_area,
	   PixelArea *dest_area,
	   void        *user_data)
{
  PosterizeDialog *pd;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  guchar *src, *dest;
  gfloat *s, *d;
  gint alpha = tag_alpha (src_tag);
  gint has_alpha;
  gint s_num_channels = tag_num_channels (src_tag);
  gint d_num_channels = tag_num_channels (dest_tag);
  int w, h, b;
  gfloat interval, half_interval;

  pd = (PosterizeDialog *) user_data;

  /*  Set the transfer array  */
  interval = 1.0/ (gfloat) (pd->levels - 1);
  half_interval = interval / 2.0;

  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  while (h--)
    {
      w = pixelarea_width (src_area);
      s = (gfloat*)src;
      d = (gfloat*)dest;
      while (w--)
	{
	  for (b = 0; b < alpha; b++)
	    {	
	      d[b] = ((int)((s[b] + half_interval)/interval) * interval);
	    }

	  if (has_alpha)
	    d[alpha] = s[alpha];
	   
	  s+= s_num_channels;
	  d+= d_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area); 
    }
}

static void
posterize_float16 (PixelArea *src_area,
	   PixelArea *dest_area,
	   void        *user_data)
{
  PosterizeDialog *pd;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  guchar *src, *dest;
  guint16 *s, *d;
  gint alpha = tag_alpha (src_tag);
  gint has_alpha;
  gint s_num_channels = tag_num_channels (src_tag);
  gint d_num_channels = tag_num_channels (dest_tag);
  int w, h, b;
  gfloat interval, half_interval;
  gfloat sb;
  ShortsFloat u;

  pd = (PosterizeDialog *) user_data;

  /*  Set the transfer array  */
  interval = 1.0/ (gfloat) (pd->levels - 1);
  half_interval = interval / 2.0;

  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  while (h--)
    {
      w = pixelarea_width (src_area);
      s = (guint16*)src;
      d = (guint16*)dest;
      while (w--)
	{
	  for (b = 0; b < alpha; b++)
	    {	
 	      sb = FLT (s[b], u);
	      d[b] = FLT16 (((int)((sb + half_interval)/interval) * interval), u);
	    }

	  if (has_alpha)
	    d[alpha] = s[alpha];
	   
	  s+= s_num_channels;
	  d+= d_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area); 
    }
}

static void
posterize_bfp (PixelArea *src_area,
	   PixelArea *dest_area,
	   void        *user_data)
{
  PosterizeDialog *pd;
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  guchar *src, *dest;
  guint16 *s, *d;
  gint alpha = tag_alpha (src_tag);
  gint has_alpha;
  gint s_num_channels = tag_num_channels (src_tag);
  gint d_num_channels = tag_num_channels (dest_tag);
  int w, h, b, i;
  double interval, half_interval;
  guint16 transfer[65536];

  pd = (PosterizeDialog *) user_data;

  /*  Set the transfer array  */
  interval = 65535.0 / (double) (pd->levels - 1);
  half_interval = interval / 2.0;

  for (i = 0; i < 65536; i++)
    transfer[i] = (guint16) ((int) (((double) i + half_interval) / interval) * interval);

  h = pixelarea_height (src_area);
  src = (guchar*)pixelarea_data (src_area);
  dest = (guchar*)pixelarea_data (dest_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;
  
 
  while (h--)
    {
      w = pixelarea_width (src_area);
      s = (guint16*)src;
      d = (guint16*)dest;
      while (w--)
	{
	  for (b = 0; b < alpha; b++)
	    d[b] = transfer[s[b]];

	  if (has_alpha)
	    d[alpha] = s[alpha];
	   
	  s+= s_num_channels;
	  d+= d_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area); 
    }
}

/*  posterize select action functions  */

static void
posterize_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
posterize_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
}

static void
posterize_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
}

static void
posterize_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
posterize_control (Tool     *tool,
		   int       action,
		   gpointer  gdisp_ptr)
{
  Posterize * post;

  post = (Posterize *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (posterize_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (posterize_dialog->image_map);
	  active_tool->preserve = FALSE;
	  posterize_dialog->image_map = NULL;
	  posterize_cancel_callback (NULL, (gpointer) posterize_dialog);
	}
      break;
    }
}

Tool *
tools_new_posterize ()
{
  Tool * tool;
  Posterize * private;

  /*  The tool options  */
  if (!posterize_options)
    posterize_options = tools_register_no_options (POSTERIZE, _("Posterize Options"));

  /*  The posterize dialog  */
  if (!posterize_dialog)
    posterize_dialog = posterize_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (posterize_dialog->shell))
      gtk_widget_show (posterize_dialog->shell);

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (Posterize *) g_malloc_zero (sizeof (Posterize));

  tool->type = POSTERIZE;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = posterize_button_press;
  tool->button_release_func = posterize_button_release;
  tool->motion_func = posterize_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = posterize_cursor_update;
  tool->control_func = posterize_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_posterize (Tool *tool)
{
  Posterize * post;

  post = (Posterize *) tool->private;

  /*  Close the color select dialog  */
  if (posterize_dialog)
    posterize_cancel_callback (NULL, (gpointer) posterize_dialog);

  g_free (post);
}

void
posterize_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Posterize does not operate on indexed drawables."));
      return;
    }

  /*  The posterize dialog  */
  if (!posterize_dialog)
    posterize_dialog = posterize_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (posterize_dialog->shell))
      gtk_widget_show (posterize_dialog->shell);
  posterize_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  posterize_dialog->image_map = image_map_create (gdisp_ptr, posterize_dialog->drawable);
  if (posterize_dialog->preview)
    posterize_preview (posterize_dialog);
}


/****************************/
/*  Select by Color dialog  */
/****************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", posterize_ok_callback, NULL, NULL },
  { "Cancel", posterize_cancel_callback, NULL, NULL }
};

static PosterizeDialog *
posterize_new_dialog ()
{
  PosterizeDialog *pd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *toggle;

  pd = g_malloc_zero (sizeof (PosterizeDialog));
  pd->preview = TRUE;
  pd->levels = 3;

  /*  The shell and main vbox  */
  pd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (pd->shell), "posterize", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (pd->shell), _("Posterize"));
  minimize_register(pd->shell);

  gtk_signal_connect (GTK_OBJECT (pd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (posterize_delete_callback),
		      pd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Posterize Levels: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  /*  levels text  */
  pd->levels_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (pd->levels_text), "3");
  gtk_widget_set_usize (pd->levels_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), pd->levels_text, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (pd->levels_text), "changed",
		      (GtkSignalFunc) posterize_levels_text_update,
		      pd);
  gtk_widget_show (pd->levels_text);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), pd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) posterize_preview_update,
		      pd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  action_items[0].user_data = pd;
  action_items[1].user_data = pd;
  build_action_area (GTK_DIALOG (pd->shell), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (pd->shell);

  return pd;
}

static void
posterize_preview (PosterizeDialog *pd)
{
  if (!pd->image_map)
  {
    g_message (_("posterize_preview(): No image map"));
  }
  else
  {
    PosterizeFunc posterize;
    posterize = posterize_func (drawable_tag (pd->drawable));
    active_tool->preserve = TRUE;
    image_map_apply (pd->image_map, posterize, (void *) pd);
    active_tool->preserve = FALSE;
  }
}

static void
posterize_ok_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (pd->shell))
    gtk_widget_hide (pd->shell);

  active_tool->preserve = TRUE;

  if (!pd->preview)
  {
    PosterizeFunc posterize;
    posterize = posterize_func (drawable_tag (pd->drawable));
    image_map_apply (pd->image_map, posterize, (void *) pd);
  }

  if (pd->image_map)
    image_map_commit (pd->image_map);

  active_tool->preserve = FALSE;

  pd->image_map = NULL;
}

static gint 
posterize_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  posterize_cancel_callback (w, data);

  return TRUE;
}

static void
posterize_cancel_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (pd->shell))
    gtk_widget_hide (pd->shell);

  if (pd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (pd->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  pd->image_map = NULL;
}

static void
posterize_preview_update (GtkWidget *w,
			  gpointer   data)
{
  PosterizeDialog *pd;

  pd = (PosterizeDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      pd->preview = TRUE;
      posterize_preview (pd);
    }
  else
{
    pd->preview = FALSE;
active_tool->preserve = TRUE;
    image_map_remove(pd->image_map);
    active_tool->preserve = FALSE;
    gdisplays_flush ();
}
}

static void
posterize_levels_text_update (GtkWidget *w,
			      gpointer   data)
{
  PosterizeDialog *pd;
  char *str;
  int value;

  pd = (PosterizeDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), 2, 256);

  if (value != pd->levels)
    {
      pd->levels = value;
      if (pd->preview)
	posterize_preview (pd);
    }
}


/*  The posterize procedure definition  */
ProcArg posterize_args[] =
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
    "levels",
    "levels of posterization: (2 <= levels <= 255)"
  }
};

ProcRecord posterize_proc =
{
  "gimp_posterize",
  "Posterize the specified drawable",
  "This procedures reduces the number of shades allows in each intensity channel to the specified 'levels' parameter.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  posterize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { posterize_invoker } },
};


static Argument *
posterize_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  PosterizeDialog pd;
  GImage *gimage;
  CanvasDrawable *drawable;
  int levels;
  int int_value;
  int x1, y1, x2, y2;
  void *pr;

  drawable = NULL;
  levels = 0;

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
    
  /*  levels  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 2 && int_value < 256)
	levels = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      PosterizeFunc posterize;
      pd.levels = levels;

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      pixelarea_init (&src_area, drawable_data (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      posterize = posterize_func (pixelarea_tag (&dest_area));
      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	(*posterize) (&src_area, &dest_area, (void *) &pd);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&posterize_proc, success);
}
