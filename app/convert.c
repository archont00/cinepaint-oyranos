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

  /*
   * TODO for Convert:
   *
   *   Use palette of another open INDEXED image
   *
   *   Different dither types
   *
   *   Alpha dithering
   */

/*
 * 04/03/30 - added function for colourspace conversion 
 *  [Stefan Klein, kleins at web dot de] 
 *
 * 98/04/13 - avoid a division by zero when converting an empty gray-scale
 *  image (who would like to do such a thing anyway??)  [Sven ] 
 *
 * 98/03/23 - fixed a longstanding fencepost - hopefully the *right*
 *  way, *again*.  (anyone ELSE want a go?  okay, just kidding... :))
 *  [Adam]
 *
 * 97/11/14 - added a proper pdb interface and support for dithering
 *  to custom palettes (based on a patch by Eric Hernes) [Yosh]
 *
 * 97/11/04 - fixed the accidental use of the colour-counting case
 *  when palette_type is WEB or MONO. [Adam]
 *
 * 97/10/25 - colour-counting implemented (could use some hashing, but
 *  performance actually seems okay) - now RGB->INDEXED conversion isn't
 *  destructive if it doesn't have to be. [Adam]
 *
 * 97/10/14 - fixed divide-by-zero when converting a completely transparent
 *  RGB image to indexed. [Adam]
 *
 * 97/07/01 - started todo/revision log.  Put code back in to
 *  eliminate full-alpha pixels from RGB histogram.
 *  [Adam D. Moss - adam@gimp.org]
 */



#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "cms.h"
#include "convert.h"
#include "drawable.h"
#include "floating_sel.h"
#include "fsdither.h"
#include "gdisplay.h"
#include "indexed_palette.h"
#include "interface.h"
#include "undo.h"
#include "palette.h"
#include "pixelarea.h"
#include "minimize.h"
#include "layer_pvt.h"
#include "../lib/wire/enums.h"
#include "paint_funcs_area.h"
#include "rc.h"

static Argument * convert_rgb_invoker               (Argument *);
static Argument * convert_grayscale_invoker         (Argument *);
static Argument * convert_indexed_invoker           (Argument *);
static Argument * convert_indexed_palette_invoker   (Argument *);
static void       convert_image                     (GImage *, Format, int, int, int);


void
convert_to_rgb (void *gimage_ptr)
{
  GImage *gimage;

  gimage = (GImage *) gimage_ptr;
  convert_image (gimage, FORMAT_RGB, 0, 0, 0);
  gdisplays_flush ();
}

void
convert_to_grayscale (void *gimage_ptr)
{
  GImage *gimage;

  gimage = (GImage *) gimage_ptr;
  convert_image (gimage, FORMAT_GRAY, 0, 0, 0);
  gdisplays_flush ();
}

void
convert_to_indexed (void *gimage_ptr)
{
  g_message (_("indexed image support has been dropped for the moment"));
  return;
}

static void
convert_image (GImage *gimage,
	       Format  new_format,
	       int     num_cols,     /*  used only for new_type == INDEXED  */
	       int     dither,       /*  used only for new_type == INDEXED  */
	       int     palette_type) /*  used only for new_type == INDEXED  */
{
  GSList * list;
  Layer * floating_layer;

  
  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  floating_layer = gimage_floating_sel (gimage);
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);
  
  /*  Set the new gimage tag */
  gimage->tag = tag_set_format (gimage->tag, new_format);

  /*  Convert all layers  */
  for (list = gimage->layers;
       list;
       list = g_slist_next (list))
    {
      PixelArea old, new;

      /* get the original layer */
      Layer * layer = (Layer *) list->data;

      /* get the original image buffer */
      Canvas * old_tiles = drawable_data (GIMP_DRAWABLE (layer));

      /* make a new image buffer */
      Canvas * new_tiles = canvas_new (tag_set_format (canvas_tag (old_tiles),
                                                       new_format),
                                       canvas_width (old_tiles),
                                       canvas_height (old_tiles),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
        
      /* convert the image data */
      pixelarea_init (&old, old_tiles,
                      0, 0,
                      0, 0,
                      FALSE);
          
      pixelarea_init (&new, new_tiles,
                      0, 0,
                      0, 0,
                      TRUE);

      copy_area (&old, &new);
        
      /*  Push the layer on the undo stack  */
      undo_push_layer_mod (gimage, layer);

      /* save the converted layer */
      GIMP_DRAWABLE(layer)->tiles = new_tiles;
    }

  /*  Make sure the projection is up to date  */
  gimage_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  layer_invalidate_previews (gimage->ID);
  gimage_invalidate_preview (gimage);
  gdisplays_update_title (gimage->ID);
  gdisplays_update_gimage (gimage->ID);
}

void
convert_image_precision (void *gimage_ptr,
	       Precision  new_precision)
{
  GSList * list;
  Layer * floating_layer;
  GImage *gimage;
  Tag orig_tag;
  Channel * sel_mask;
  PixelArea oldmaskPR, maskPR;

  gimage = (GImage *) gimage_ptr;
  orig_tag = gimage->tag;
  
  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  floating_layer = gimage_floating_sel (gimage);
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);
  
  /*  Push the image size to the stack  */
  undo_push_gimage_mod (gimage);

  /*  Set the new gimage tag */
  gimage->tag = tag_set_precision (gimage->tag, new_precision); 

  /*  Convert all layers  */
  for (list = gimage->layers;
       list;
       list = g_slist_next (list))
    {
      PixelArea old, new;

      /* get the original layer */
      Layer * layer = (Layer *) list->data;

      /* get the original image buffer */
      Canvas * old_tiles = drawable_data (GIMP_DRAWABLE (layer));

      /* make a new image buffer */
      Canvas * new_tiles = canvas_new (tag_set_precision (canvas_tag (old_tiles),
                                                       new_precision),
                                       canvas_width (old_tiles),
                                       canvas_height (old_tiles),
#ifdef NO_TILES						  
						  STORAGE_FLAT);
#else
						  STORAGE_TILED);
#endif
        
      /* convert the image data */
      pixelarea_init (&old, old_tiles,
                      0, 0,
                      0, 0,
                      FALSE);
          
      pixelarea_init (&new, new_tiles,
                      0, 0,
                      0, 0,
                      TRUE);

      copy_area (&old, &new);
        
      /*  Push the layer on the undo stack  */
      undo_push_layer_mod (gimage, layer);

      /* save the converted layer */
      GIMP_DRAWABLE(layer)->tiles = new_tiles;
    }

  /* Change the mask */
  /* Make a new one */
  sel_mask = channel_new_mask (gimage->ID, 
                                             gimage->width, gimage->height,
                                             tag_precision (gimage->tag));

  /* Copy the data over */
  pixelarea_init (&oldmaskPR, GIMP_DRAWABLE(gimage->selection_mask)->tiles, 0, 0, 0, 0, FALSE);
  pixelarea_init (&maskPR, GIMP_DRAWABLE(sel_mask)->tiles, 0, 0, 0, 0, TRUE);
  copy_area (&oldmaskPR, &maskPR);
  
  channel_delete(gimage->selection_mask); /* Delete the old one */
  gimage->selection_mask = sel_mask;

  /*  Make sure the projection is up to date  */
  gimage_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */

  layer_invalidate_previews (gimage->ID);
  gimage_invalidate_preview (gimage);
  gdisplays_update_title (gimage->ID);
  gdisplays_update_gimage (gimage->ID);
}


/* converts the image data to the colorspace given by new_profile
   only works if gimage has a profile
   lcms_intent is littlecms intent parameter */
void
convert_image_colorspace (void *gimage_ptr, CMSProfile *new_profile,
                          int lcms_intent, int flags)
{   GImage *image = (GImage *) gimage_ptr;
    DWORD lcms_format_in;
    DWORD lcms_format_out;
    CMSTransform *transform;
    DWORD cms_flags = 0;
    Tag tag;

    /*  get the profiles */
    GSList *profiles = NULL;
    CMSProfile *image_profile = gimage_get_cms_profile(image);
    if (image_profile == NULL)
    {   g_warning("convert_image_colourspace: No cms profile assigned to gimage. Cannot convert.");
        return;
    } 
    profiles = g_slist_append(profiles, (gpointer)image_profile);
    profiles = g_slist_append(profiles, (gpointer)new_profile);

    /* add alpha if needed */
    if( strstr(cms_get_profile_info(new_profile)->color_space_name,"Cmyk") &&
        !layer_has_alpha( image->active_layer ))
    { layer_add_alpha( image->active_layer );
    }

    tag = drawable_tag(GIMP_DRAWABLE(image->active_layer));

    /* assuming all layers have homogenous alpha */
    lcms_format_in  = cms_get_lcms_format( tag, image_profile  );
    lcms_format_out = cms_get_lcms_format( tag, new_profile  );
    if  (!(int)lcms_format_in || !(int)lcms_format_out)
    {   g_warning("%s:%d %s(): wrong lcms_format %d|%d",
                  __FILE__,__LINE__,__func__,
                  (int) lcms_format_in, (int) lcms_format_out);
        return;
    }

    cms_flags = cms_default_flags;
    if(flags & cmsFLAGS_WHITEBLACKCOMPENSATION)
      cms_flags = cms_flags | cmsFLAGS_WHITEBLACKCOMPENSATION;
    else
      cms_flags = cms_flags & (~cmsFLAGS_WHITEBLACKCOMPENSATION);

    cms_flags = cms_flags & (~cmsFLAGS_SOFTPROOFING);

    transform = cms_get_transform(profiles, lcms_format_in, lcms_format_out,
                                            lcms_intent, cms_flags,
                                            0, tag, tag, NULL);
    g_slist_free(profiles);
    if(!transform)
    {   g_warning("%s:%d %s(): transform failed", __FILE__,__LINE__,__func__);
        return;
    }
    gimage_transform_colors(image, transform, TRUE);
    cms_return_transform(transform);

    gimage_set_cms_profile(image, new_profile);

    /* refresh the profile name in the title bar */
    gdisplays_update_title (image->ID);
}

#define MAXNUMCOLORS 256

/* adam's extra palette stuff */
#define MAKE_PALETTE 0
#define REUSE_PALETTE 1
#define WEB_PALETTE 2
#define MONO_PALETTE 3
#define CUSTOM_PALETTE 4
static PaletteEntriesP theCustomPalette = NULL;


#if 0

#define NODITHER 0
#define FSDITHER 1
#define NODESTRUCTDITHER 2


#define PRECISION_R 6
#define PRECISION_G 6
#define PRECISION_B 5

#define HIST_R_ELEMS (1<<PRECISION_R)
#define HIST_G_ELEMS (1<<PRECISION_G)
#define HIST_B_ELEMS (1<<PRECISION_B)

#define MR HIST_G_ELEMS*HIST_B_ELEMS
#define MG HIST_B_ELEMS

#define BITS_IN_SAMPLE 8

#define R_SHIFT  (BITS_IN_SAMPLE-PRECISION_R)
#define G_SHIFT  (BITS_IN_SAMPLE-PRECISION_G)
#define B_SHIFT  (BITS_IN_SAMPLE-PRECISION_B)

#define R_SCALE 30               /*  scale R distances by this much  */
#define G_SCALE 59               /*  scale G distances by this much  */
#define B_SCALE 11               /*  and B by this much  */
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

unsigned char webpal[] =
{
  255,255,255,255,255,204,255,255,153,255,255,102,255,255,51,255,255,0,255,
  204,255,255,204,204,255,204,153,255,204,102,255,204,51,255,204,0,255,153,
  255,255,153,204,255,153,153,255,153,102,255,153,51,255,153,0,255,102,255,
  255,102,204,255,102,153,255,102,102,255,102,51,255,102,0,255,51,255,255,
  51,204,255,51,153,255,51,102,255,51,51,255,51,0,255,0,255,255,0,
  204,255,0,153,255,0,102,255,0,51,255,0,0,204,255,255,204,255,204,
  204,255,153,204,255,102,204,255,51,204,255,0,204,204,255,204,204,204,204,
  204,153,204,204,102,204,204,51,204,204,0,204,153,255,204,153,204,204,153,
  153,204,153,102,204,153,51,204,153,0,204,102,255,204,102,204,204,102,153,
  204,102,102,204,102,51,204,102,0,204,51,255,204,51,204,204,51,153,204,
  51,102,204,51,51,204,51,0,204,0,255,204,0,204,204,0,153,204,0,
  102,204,0,51,204,0,0,153,255,255,153,255,204,153,255,153,153,255,102,
  153,255,51,153,255,0,153,204,255,153,204,204,153,204,153,153,204,102,153,
  204,51,153,204,0,153,153,255,153,153,204,153,153,153,153,153,102,153,153,
  51,153,153,0,153,102,255,153,102,204,153,102,153,153,102,102,153,102,51,
  153,102,0,153,51,255,153,51,204,153,51,153,153,51,102,153,51,51,153,
  51,0,153,0,255,153,0,204,153,0,153,153,0,102,153,0,51,153,0,
  0,102,255,255,102,255,204,102,255,153,102,255,102,102,255,51,102,255,0,
  102,204,255,102,204,204,102,204,153,102,204,102,102,204,51,102,204,0,102,
  153,255,102,153,204,102,153,153,102,153,102,102,153,51,102,153,0,102,102,
  255,102,102,204,102,102,153,102,102,102,102,102,51,102,102,0,102,51,255,
  102,51,204,102,51,153,102,51,102,102,51,51,102,51,0,102,0,255,102,
  0,204,102,0,153,102,0,102,102,0,51,102,0,0,51,255,255,51,255,
  204,51,255,153,51,255,102,51,255,51,51,255,0,51,204,255,51,204,204,
  51,204,153,51,204,102,51,204,51,51,204,0,51,153,255,51,153,204,51,
  153,153,51,153,102,51,153,51,51,153,0,51,102,255,51,102,204,51,102,
  153,51,102,102,51,102,51,51,102,0,51,51,255,51,51,204,51,51,153,
  51,51,102,51,51,51,51,51,0,51,0,255,51,0,204,51,0,153,51,
  0,102,51,0,51,51,0,0,0,255,255,0,255,204,0,255,153,0,255,
  102,0,255,51,0,255,0,0,204,255,0,204,204,0,204,153,0,204,102,
  0,204,51,0,204,0,0,153,255,0,153,204,0,153,153,0,153,102,0,
  153,51,0,153,0,0,102,255,0,102,204,0,102,153,0,102,102,0,102,
  51,0,102,0,0,51,255,0,51,204,0,51,153,0,51,102,0,51,51,
  0,51,0,0,0,255,0,0,204,0,0,153,0,0,102,0,0,51,0,0,0
};
typedef struct Color Color;
typedef struct QuantizeObj QuantizeObj;
typedef void (* Pass1_Func) (QuantizeObj *);
typedef void (* Pass2_Func) (QuantizeObj *, Layer *, TileManager *);
typedef void (* Cleanup_Func) (QuantizeObj *);
typedef unsigned long ColorFreq;
typedef ColorFreq *Histogram;

struct Color
{
  int red;
  int green;
  int blue;
};

struct QuantizeObj
{
  Pass1_Func first_pass;		/*  first pass over image data creates colormap  */
  Pass2_Func second_pass;	        /*  second pass maps from image data to colormap */
  Cleanup_Func delete_func;             /*  function to clean up data associated with private */

  int desired_number_of_colors;         /*  Number of colors we will allow               */
  int actual_number_of_colors;          /*  Number of colors actually needed             */
  Color cmap[256];  			/*  colormap created by quantization             */
  Histogram histogram;                  /*  holds the histogram                          */
};

typedef struct
{
  /*  The bounds of the box (inclusive); expressed as histogram indexes  */
  int   Rmin, Rmax;
  int   Gmin, Gmax;
  int   Bmin, Bmax;

  /*  The volume (actually 2-norm) of the box  */
  int   volume;

  /*  The number of nonzero histogram cells within this box  */
  long  colorcount;
} box, *boxptr;

typedef struct
{
  long ncolors;
  long dither;
} Options;

typedef struct
{
  GtkWidget *  shell;
  void *       gimage_ptr;

  int          gimage_ID;
  int          dither;
  int          num_cols;
  int          palette;
  int          makepal_flag;
  int          webpal_flag;
  int          custompal_flag;
  int          monopal_flag;
  int          reusepal_flag;
} IndexedDialog;


static void indexed_ok_callback     (GtkWidget *, gpointer);
static void indexed_cancel_callback (GtkWidget *, gpointer);
static gint indexed_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void indexed_num_cols_update (GtkWidget *, gpointer);
static void indexed_radio_update    (GtkWidget *, gpointer);
static void indexed_dither_update   (GtkWidget *, gpointer);


static void zero_histogram_gray     (Histogram);
static void zero_histogram_rgb      (Histogram);
static void generate_histogram_gray (Histogram, Layer *);
static void generate_histogram_rgb  (Histogram, Layer *, int col_limit);

static QuantizeObj* initialize_median_cut (int, int, int, int);

static unsigned char found_cols[MAXNUMCOLORS][3];
static int num_found_cols;
static gboolean needs_quantize;

static GtkWidget *build_palette_menu(int *default_palette);
static void palette_entries_callback(GtkWidget *w, gpointer client_data);

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { _("OK"), indexed_ok_callback, NULL, NULL },
  { _("Cancel"), indexed_cancel_callback, NULL, NULL }
};


void
convert_to_indexed (void *gimage_ptr)
{
  GImage *gimage;
  IndexedDialog *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *text;
  GtkWidget *frame;
  GtkWidget *toggle;
  GSList *group = NULL;
  static gboolean shown_message_already = False;

  gimage = (GImage *) gimage_ptr;
  dialog = (IndexedDialog *) g_malloc_zero (sizeof (IndexedDialog));
  dialog->gimage_ptr = gimage_ptr;
  dialog->gimage_ID = gimage->ID;
  dialog->num_cols = 256;
  dialog->dither = TRUE;

  /* if the image isn't non-alpha/layered, set the default number of
     colours to one less than max, to leave room for a transparent index
     for transparent/animated GIFs */
  if ((!gimage_is_empty (gimage))
      &&
      (
       gimage->layers->next
       ||
       layer_has_alpha((Layer *) gimage->layers->data)
       )
      )
    {
      dialog->num_cols = 255;
      if (!shown_message_already)
	{
	  g_message (_("Note:  You are attempting to convert an image\nwith alpha/layers.  It is recommended that you quantize\nto no more than 255 colors if you wish to make\na transparent or animated GIF from it.\n \nThis message will not be shown again\nuntil after restart."));
	  shown_message_already = True;
	}
    }
  dialog->makepal_flag = TRUE;
  dialog->webpal_flag = FALSE;
  dialog->custompal_flag = FALSE;
  dialog->monopal_flag = FALSE;
  dialog->reusepal_flag = FALSE;
  dialog->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog->shell), "indexed_color_conversion", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (dialog->shell), _("Indexed Color Conversion"));
  gtk_signal_connect (GTK_OBJECT (dialog->shell), "delete_event",
		      GTK_SIGNAL_FUNC (indexed_delete_callback),
		      dialog);
  minimize_register(dialog_shell);

  frame = gtk_frame_new (_("Palette Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->shell)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_border_width (GTK_CONTAINER (GTK_BOX (GTK_DIALOG (dialog->shell)->vbox)), 4);
  /* put the vbox in the frame */
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);

  /*  'generate palette'  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_radio_button_new_with_label (group, _("Generate optimal palette: "));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) indexed_radio_update,
		      &(dialog->makepal_flag));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dialog->makepal_flag);
  gtk_widget_show (toggle);
  label = gtk_label_new (_("# of colors: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  text = gtk_entry_new ();
  if ((!gimage_is_empty (gimage))
      &&
      (
       gimage->layers->next
       ||
       layer_has_alpha((Layer *) gimage->layers->data)
       )
      )
    gtk_entry_set_text (GTK_ENTRY (text), "255");
  else
    gtk_entry_set_text (GTK_ENTRY (text), "256");
  gtk_widget_set_usize (text, 50, 25);
  gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (text), "changed",
		      (GtkSignalFunc) indexed_num_cols_update,
		      dialog);
  gtk_widget_show (text);
  gtk_widget_show (hbox);

  if (gimage_format (gimage) == FORMAT_RGB)
    {
		GtkWidget *menu;
		GtkWidget *palette_option_menu;
		int default_palette;
      /*  'web palette'  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      toggle =
	gtk_radio_button_new_with_label (group, _("Use WWW-optimised palette"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  (GtkSignalFunc) indexed_radio_update,
			  &(dialog->webpal_flag));
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dialog->webpal_flag);
      gtk_widget_show (toggle);
      gtk_widget_show (hbox);

      menu = build_palette_menu(&default_palette);
      if (menu) {
          /* 'custom' palette from dialog */
          hbox = gtk_hbox_new (FALSE, 1);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          toggle = gtk_radio_button_new_with_label (group, _("Use custom palette"));
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
          gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
          gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			      (GtkSignalFunc) indexed_radio_update,
			      &(dialog->custompal_flag));
          gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
				   dialog->custompal_flag);
          gtk_widget_show (toggle);

          palette_option_menu = gtk_option_menu_new();
          gtk_option_menu_set_menu (GTK_OPTION_MENU(palette_option_menu), menu);
          gtk_option_menu_set_history(GTK_OPTION_MENU(palette_option_menu),
				      default_palette);
          gtk_box_pack_start(GTK_BOX(hbox), palette_option_menu, TRUE, TRUE, 2);
	  gtk_widget_show(palette_option_menu);
          gtk_widget_show (hbox);
      }
    }

  /*  'mono palette'  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  toggle =
    gtk_radio_button_new_with_label (group, _("Use black/white (1-bit) palette"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) indexed_radio_update,
		      &(dialog->monopal_flag));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dialog->monopal_flag);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (_("Dither Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->shell)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  /* put the vbox in the frame */
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(vbox);
  /*  The dither toggle  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_check_button_new_with_label (_("Enable Floyd-Steinberg dithering"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), dialog->dither);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) indexed_dither_update,
		      dialog);
  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = dialog;
  action_items[1].user_data = dialog;
  build_action_area (GTK_DIALOG (dialog->shell), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (dialog->shell);
}

static GtkWidget *
build_palette_menu(int *default_palette){
  GtkWidget *menu;
  GtkWidget *menu_item;
  GSList *list;
  PaletteEntriesP entries;
  int i;


  if(!palette_entries_list) {
    /* fprintf(stderr, "no palette_entries_list, building...\n");*/
     palette_init_palettes(FALSE);
  }

  list = palette_entries_list;

  if (!list)
    return NULL;

  menu = gtk_menu_new();

  for(i=0,list = palette_entries_list,*default_palette=-1;
      list;
      i++,list = g_slist_next (list))
    {
      entries = (PaletteEntriesP) list->data;
      /*      fprintf(stderr, "(palette %s)\n", entries->filename);*/

      /* We can't dither to > 256 colors */
      if (entries->n_colors <= 256) {
	menu_item = gtk_menu_item_new_with_label (entries->name);
	gtk_signal_connect( GTK_OBJECT(menu_item), "activate",
			    (GtkSignalFunc) palette_entries_callback,
			    (gpointer)entries);
	gtk_container_add(GTK_CONTAINER(menu), menu_item);
	gtk_widget_show(menu_item);
	if (theCustomPalette == entries) *default_palette = i;
      }
    }

   /* default to first one */
   if(*default_palette==-1) {
     theCustomPalette = (PaletteEntriesP) palette_entries_list->data;
     /* fprintf(stderr, "setting default to `%s'\n", theCustomPalette->name);*/
     *default_palette = 0;
   }
   return menu;
}

static void
palette_entries_callback(GtkWidget *w, gpointer client_data){
		  theCustomPalette = (PaletteEntriesP)client_data;
}

static void
indexed_ok_callback (GtkWidget *widget,
		     gpointer   client_data)
{
  IndexedDialog *dialog;
  int palette_type;

  dialog = (IndexedDialog *) client_data;

  if (dialog->webpal_flag) palette_type = WEB_PALETTE;
  else
    if (dialog->custompal_flag) palette_type = CUSTOM_PALETTE;
    else
      if (dialog->monopal_flag) palette_type = MONO_PALETTE;
      else
        if (dialog->makepal_flag) palette_type = MAKE_PALETTE;
        else
          palette_type = REUSE_PALETTE;
  /*  Convert the image to indexed color  */
  if (gimage_get_ID (dialog->gimage_ID))
    {
      convert_image ((GImage *) dialog->gimage_ptr, INDEXED, dialog->num_cols,
		     dialog->dither, palette_type);
      gdisplays_flush ();
    }

  gtk_widget_destroy (dialog->shell);
  g_free (dialog);
  dialog = NULL;
}

static gint
indexed_delete_callback (GtkWidget *w,
			 GdkEvent *e,
			 gpointer client_data)
{
  indexed_cancel_callback (w, client_data);

  return TRUE;
}

static void
indexed_cancel_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  IndexedDialog *dialog;

  dialog = (IndexedDialog *) client_data;
  gtk_widget_destroy (dialog->shell);
  g_free (dialog);
  dialog = NULL;
}

static void
indexed_num_cols_update (GtkWidget *w,
			 gpointer   data)
{
  IndexedDialog *dialog;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  dialog = (IndexedDialog *) data;
  dialog->num_cols = BOUNDS(((int) atof (str)), 1, 256);
}

static void
indexed_radio_update (GtkWidget *widget,
		      gpointer   data)
{
  gint *toggle_val;
  toggle_val = (int *) data;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}
static void
indexed_dither_update (GtkWidget *w,
		       gpointer   data)
{
  IndexedDialog *dialog;

  dialog = (IndexedDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    dialog->dither = TRUE;
  else
    dialog->dither = FALSE;
}


/*
 *  Indexed color conversion machinery
 */

static void
zero_histogram_gray (Histogram histogram)
{
  int i;

  for (i = 0; i < 256; i++)
    histogram[i] = 0;
}


static void
zero_histogram_rgb (Histogram histogram)
{
  int r, g, b;

  for (r = 0; r < HIST_R_ELEMS; r++)
    for (g = 0; g < HIST_G_ELEMS; g++)
      for (b = 0; b < HIST_B_ELEMS; b++)
	histogram[r*MR + g*MG + b] = 0;
}


static void
generate_histogram_gray (Histogram  histogram,
			 Layer     *layer)
{
  PixelRegion srcPR;
  unsigned char *data;
  int size;
  void *pr;
  gboolean has_alpha;

  has_alpha = (gboolean) layer_has_alpha(layer);

  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  for (pr = pixel_regions_register (1, &srcPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = srcPR.data;
      size = srcPR.w * srcPR.h;
      while (size--)
	{
	  histogram[*data] ++;
	  data += srcPR.bytes;
	}
    }
}


static void
generate_histogram_rgb (Histogram  histogram,
			Layer     *layer,
			int        col_limit)
{
  PixelRegion srcPR;
  unsigned char *data;
  int size;
  void *pr;
  ColorFreq *col;
  gboolean has_alpha;
  int nfc_iter;

  has_alpha = (gboolean) layer_has_alpha(layer);

/*  g_print ("col_limit = %d, nfc = %d\n", col_limit, num_found_cols);*/

  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0,
		     GIMP_DRAWABLE(layer)->width,
		     GIMP_DRAWABLE(layer)->height,
		     FALSE);
  for (pr = pixel_regions_register (1, &srcPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = srcPR.data;
      size = srcPR.w * srcPR.h;

      if (needs_quantize)
	{
	  while (size--)
	    {
	      if ((has_alpha && (data[ALPHA_PIX]&128)) || (!has_alpha))
		{
		  col = & histogram[(data[RED_PIX] >> R_SHIFT) * MR +
				    (data[GREEN_PIX] >> G_SHIFT) * MG +
				    (data[BLUE_PIX] >> B_SHIFT)];
		  (*col)++;
		}
	      data += srcPR.bytes;
	    }
	}
      else
	{
	  while (size--)
	    {
	      if ((has_alpha && (data[ALPHA_PIX]&128)) || (!has_alpha))
		{
		  col = & histogram[(data[RED_PIX] >> R_SHIFT) * MR +
				    (data[GREEN_PIX] >> G_SHIFT) * MG +
				    (data[BLUE_PIX] >> B_SHIFT)];
		  (*col)++;

		  if (!needs_quantize)
		    {
		      for (nfc_iter = 0;
			   nfc_iter < num_found_cols;
			   nfc_iter++)
			{
			  if (
			      (data[RED_PIX] == found_cols[nfc_iter][0])
			      &&
			      (data[GREEN_PIX] == found_cols[nfc_iter][1])
			      &&
			      (data[BLUE_PIX] == found_cols[nfc_iter][2])
			      )
			    goto already_found;
			}
			  
		      /* Colour was not in the table of
		       * existing colours
		       */
		      
		      num_found_cols++;
		      
		      if (num_found_cols > col_limit)
			{
			  /* There are more colours in the image
			   *  than were allowed.  We switch to plain
			   *  histogram calculation with a view to
			   *  quantizing at a later stage.
			   */
			  needs_quantize = TRUE;
/*			  g_print ("\nmax colours exceeded - needs quantize.\n");*/
			  goto already_found;
			}
		      else
			{
			  /* Remember the new colour we just found.
			   */
			  found_cols[num_found_cols-1][0] = data[RED_PIX];
			  found_cols[num_found_cols-1][1] = data[GREEN_PIX];
			  found_cols[num_found_cols-1][2] = data[BLUE_PIX];
			}
		    }
		}
	    already_found:
	      data += srcPR.bytes;	      
	    }
	}
    }

/*  g_print ("O: col_limit = %d, nfc = %d\n", col_limit, num_found_cols);*/
}


static boxptr
find_biggest_color_pop (boxptr boxlist,
			int    numboxes)
/* Find the splittable box with the largest color population */
/* Returns NULL if no splittable boxes remain */
{
  boxptr boxp;
  int i;
  long maxc = 0;
  boxptr which = NULL;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++)
    {
      if (boxp->colorcount > maxc && boxp->volume > 0)
	{
	  which = boxp;
	  maxc = boxp->colorcount;
	}
    }

  return which;
}


static boxptr
find_biggest_volume (boxptr boxlist,
		     int    numboxes)
/* Find the splittable box with the largest (scaled) volume */
/* Returns NULL if no splittable boxes remain */
{
  boxptr boxp;
  int i;
  int maxv = 0;
  boxptr which = NULL;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++)
    {
      if (boxp->volume > maxv)
	{
	  which = boxp;
	  maxv = boxp->volume;
	}
    }

  return which;
}


static void
update_box_gray (Histogram histogram,
		 boxptr    boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its volume and population */
{
  int i, min, max, dist;
  long ccount;

  min = boxp->Rmin;
  max = boxp->Rmax;

  if (max > min)
    for (i = min; i <= max; i++)
      {
	if (histogram[i] != 0)
	  {
	    boxp->Rmin = min = i;
	    break;
	  }
      }

  if (max > min)
    for (i = max; i >= min; i--)
      {
	if (histogram[i] != 0)
	  {
	    boxp->Rmax = max = i;
	    break;
	  }
      }

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0.
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist = max - min;
  boxp->volume = dist * dist;

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (i = min; i <= max; i++)
    if (histogram[i] != 0)
      ccount++;

  boxp->colorcount = ccount;
}

static void
update_box_rgb (Histogram histogram,
		boxptr    boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its volume and population */
{
  ColorFreq * histp;
  int R,G,B;
  int Rmin,Rmax,Gmin,Gmax,Bmin,Bmax;
  int dist0,dist1,dist2;
  long ccount;

  Rmin = boxp->Rmin;  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;  Bmax = boxp->Bmax;

  if (Rmax > Rmin)
    for (R = Rmin; R <= Rmax; R++)
      for (G = Gmin; G <= Gmax; G++)
	{
	  histp = histogram + R*MR + G*MG + Bmin;
	  for (B = Bmin; B <= Bmax; B++)
	    if (*histp++ != 0)
	      {
		boxp->Rmin = Rmin = R;
		goto have_Rmin;
	      }
	}
 have_Rmin:
  if (Rmax > Rmin)
    for (R = Rmax; R >= Rmin; R--)
      for (G = Gmin; G <= Gmax; G++)
	{
	  histp = histogram + R*MR + G*MG + Bmin;
	  for (B = Bmin; B <= Bmax; B++)
	    if (*histp++ != 0)
	      {
		boxp->Rmax = Rmax = R;
		goto have_Rmax;
	      }
	}
 have_Rmax:
  if (Gmax > Gmin)
    for (G = Gmin; G <= Gmax; G++)
      for (R = Rmin; R <= Rmax; R++)
	{
	  histp = histogram + R*MR + G*MG + Bmin;
	  for (B = Bmin; B <= Bmax; B++)
	    if (*histp++ != 0)
	      {
		boxp->Gmin = Gmin = G;
		goto have_Gmin;
	      }
	}
 have_Gmin:
  if (Gmax > Gmin)
    for (G = Gmax; G >= Gmin; G--)
      for (R = Rmin; R <= Rmax; R++)
	{
	  histp = histogram + R*MR + G*MG + Bmin;
	  for (B = Bmin; B <= Bmax; B++)
	    if (*histp++ != 0)
	      {
		boxp->Gmax = Gmax = G;
		goto have_Gmax;
	      }
	}
 have_Gmax:
  if (Bmax > Bmin)
    for (B = Bmin; B <= Bmax; B++)
      for (R = Rmin; R <= Rmax; R++)
	{
	  histp = histogram + R*MR + Gmin*MG + B;
	  for (G = Gmin; G <= Gmax; G++, histp += MG)
	    if (*histp != 0)
	      {
		boxp->Bmin = Bmin = B;
		goto have_Bmin;
	      }
	}
 have_Bmin:
  if (Bmax > Bmin)
    for (B = Bmax; B >= Bmin; B--)
      for (R = Rmin; R <= Rmax; R++)
	{
	  histp = histogram + R*MR + Gmin*MG + B;
	  for (G = Gmin; G <= Gmax; G++, histp += MG)
	    if (*histp != 0)
	      {
		boxp->Bmax = Bmax = B;
		goto have_Bmax;
	      }
	}
 have_Bmax:

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0.
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist0 = ((Rmax - Rmin) << R_SHIFT) * R_SCALE;
  dist1 = ((Gmax - Gmin) << G_SHIFT) * G_SCALE;
  dist2 = ((Bmax - Bmin) << B_SHIFT) * B_SCALE;
  boxp->volume = dist0*dist0 + dist1*dist1 + dist2*dist2;

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++)
      {
	histp = histogram + R*MR + G*MG + Bmin;
	for (B = Bmin; B <= Bmax; B++, histp++)
	  if (*histp != 0)
	    {
	      ccount++;
	    }
      }

  boxp->colorcount = ccount;
}


static int
median_cut_gray (Histogram histogram,
		 boxptr    boxlist,
		 int       numboxes,
		 int       desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
  int lb;
  boxptr b1, b2;

  while (numboxes < desired_colors)
    {
      /* Select box to split.
       * Current algorithm: by population for first half, then by volume.
       */
      if (numboxes*2 <= desired_colors)
	{
	  b1 = find_biggest_color_pop (boxlist, numboxes);
	}
      else
	{
	  b1 = find_biggest_volume (boxlist, numboxes);
	}

      if (b1 == NULL)		/* no splittable boxes left! */
	break;

      b2 = boxlist + numboxes;	/* where new box will go */
      /* Copy the color bounds to the new box. */
      b2->Rmax = b1->Rmax;
      b2->Rmin = b1->Rmin;

      /* Current algorithm: split at halfway point.
       * (Since the box has been shrunk to minimum volume,
       * any split will produce two nonempty subboxes.)
       * Note that lb value is max for lower box, so must be < old max.
       */
      lb = (b1->Rmax + b1->Rmin) / 2;
      b1->Rmax = lb;
      b2->Rmin = lb + 1;

      /* Update stats for boxes */
      update_box_gray (histogram, b1);
      update_box_gray (histogram, b2);
      numboxes++;
    }

  return numboxes;
}

static int
median_cut_rgb (Histogram histogram,
		boxptr    boxlist,
		int       numboxes,
		int       desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
  int n,lb;
  int R,G,B,cmax;
  boxptr b1,b2;

  while (numboxes < desired_colors) {
    /* Select box to split.
     * Current algorithm: by population for first half, then by volume.
     */
    if (numboxes*2 <= desired_colors)
      {
	b1 = find_biggest_color_pop (boxlist, numboxes);
      }
    else
      {
	b1 = find_biggest_volume (boxlist, numboxes);
      }

    if (b1 == NULL)		/* no splittable boxes left! */
      break;
    b2 = boxlist + numboxes;	/* where new box will go */
    /* Copy the color bounds to the new box. */
    b2->Rmax = b1->Rmax; b2->Gmax = b1->Gmax; b2->Bmax = b1->Bmax;
    b2->Rmin = b1->Rmin; b2->Gmin = b1->Gmin; b2->Bmin = b1->Bmin;
    /* Choose which axis to split the box on.
     * Current algorithm: longest scaled axis.
     * See notes in update_box about scaling distances.
     */
    R = ((b1->Rmax - b1->Rmin) << R_SHIFT) * R_SCALE;
    G = ((b1->Gmax - b1->Gmin) << G_SHIFT) * G_SCALE;
    B = ((b1->Bmax - b1->Bmin) << B_SHIFT) * B_SCALE;
    /* We want to break any ties in favor of green, then red, blue last.
     */
    cmax = G; n = 1;
    if (R > cmax) { cmax = R; n = 0; }
    if (B > cmax) { n = 2; }

    /* Choose split point along selected axis, and update box bounds.
     * Current algorithm: split at halfway point.
     * (Since the box has been shrunk to minimum volume,
     * any split will produce two nonempty subboxes.)
     * Note that lb value is max for lower box, so must be < old max.
     */
    switch (n)
      {
      case 0:
	lb = (b1->Rmax + b1->Rmin) / 2;
	b1->Rmax = lb;
	b2->Rmin = lb+1;
	break;
      case 1:
	lb = (b1->Gmax + b1->Gmin) / 2;
	b1->Gmax = lb;
	b2->Gmin = lb+1;
	break;
      case 2:
	lb = (b1->Bmax + b1->Bmin) / 2;
	b1->Bmax = lb;
	b2->Bmin = lb+1;
	break;
      }
    /* Update stats for boxes */
    update_box_rgb (histogram, b1);
    update_box_rgb (histogram, b2);
    numboxes++;
  }
  return numboxes;
}


static void
compute_color_gray (QuantizeObj *quantobj,
		    Histogram    histogram,
		    boxptr       boxp,
		    int          icolor)
/* Compute representative color for a box, put it in colormap[icolor] */
{
  int i, min, max;
  long count;
  long total;
  long gtotal;

  min = boxp->Rmin;
  max = boxp->Rmax;

  total = 0;
  gtotal = 0;

  for (i = min; i <= max; i++)
    {
      count = histogram[i];
      if (count != 0)
	{
	  total += count;
	  gtotal += i * count;
	}
    }

  if (total != 0)
    {
      quantobj->cmap[icolor].red = (gtotal + (total >> 1)) / total;
      quantobj->cmap[icolor].green = quantobj->cmap[icolor].red;
      quantobj->cmap[icolor].blue = quantobj->cmap[icolor].red;
    }
   else /* The only situation where total==0 is if the image was null or
	*  all-transparent.  In that case we just put a dummy value in
	*  the colourmap.
	*/
    {
      quantobj->cmap[icolor].red =
	quantobj->cmap[icolor].green =
	quantobj->cmap[icolor].blue = 0;
    }
}

static void
compute_color_rgb (QuantizeObj *quantobj,
		   Histogram    histogram,
		   boxptr       boxp,
		   int          icolor)
/* Compute representative color for a box, put it in colormap[icolor] */
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  ColorFreq * histp;
  int R, G, B;
  int Rmin, Rmax;
  int Gmin, Gmax;
  int Bmin, Bmax;
  long count;
  long total = 0;
  long Rtotal = 0;
  long Gtotal = 0;
  long Btotal = 0;

  Rmin = boxp->Rmin;  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;  Bmax = boxp->Bmax;

  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++)
      {
	histp = histogram + R*MR + G*MG + Bmin;
	for (B = Bmin; B <= Bmax; B++)
	  {
	    if ((count = *histp++) != 0)
	      {
		total += count;
		Rtotal += ((R << R_SHIFT) + ((1<<R_SHIFT)>>1)) * count;
		Gtotal += ((G << G_SHIFT) + ((1<<G_SHIFT)>>1)) * count;
		Btotal += ((B << B_SHIFT) + ((1<<B_SHIFT)>>1)) * count;
	      }
	  }
      }

  if (total != 0)
    {
      quantobj->cmap[icolor].red = (Rtotal + (total>>1)) / total;
      quantobj->cmap[icolor].green = (Gtotal + (total>>1)) / total;
      quantobj->cmap[icolor].blue = (Btotal + (total>>1)) / total;
    }
  else /* The only situation where total==0 is if the image was null or
	*  all-transparent.  In that case we just put a dummy value in
	*  the colourmap.
	*/
    {
      quantobj->cmap[icolor].red =
	quantobj->cmap[icolor].green =
	quantobj->cmap[icolor].blue = 0;
    }
}


static void
select_colors_gray (QuantizeObj *quantobj,
		    Histogram    histogram)
/* Master routine for color selection */
{
  boxptr boxlist;
  int numboxes;
  int desired = quantobj->desired_number_of_colors;
  int i;

  /* Allocate workspace for box list */
  boxlist = (boxptr) g_malloc ( desired * sizeof(box) );

  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].Rmin = 0;
  boxlist[0].Rmax = 255;
  /* Shrink it to actually-used volume and set its statistics */
  update_box_gray (histogram, boxlist);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut_gray (histogram, boxlist, numboxes, desired);

  quantobj->actual_number_of_colors = numboxes;
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    compute_color_gray (quantobj, histogram, boxlist + i, i);
}


static void
select_colors_rgb (QuantizeObj *quantobj,
		   Histogram    histogram)
/* Master routine for color selection */
{
  boxptr boxlist;
  int numboxes;
  int desired = quantobj->desired_number_of_colors;
  int i;

  /* Allocate workspace for box list */
  boxlist = (boxptr) g_malloc ( desired * sizeof(box) );

  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].Rmin = 0;
  boxlist[0].Rmax = (1 << PRECISION_R) - 1;
  boxlist[0].Gmin = 0;
  boxlist[0].Gmax = (1 << PRECISION_G) - 1;
  boxlist[0].Bmin = 0;
  boxlist[0].Bmax = (1 << PRECISION_B) - 1;
  /* Shrink it to actually-used volume and set its statistics */
  update_box_rgb (histogram, boxlist);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut_rgb (histogram, boxlist, numboxes, desired);

  quantobj->actual_number_of_colors = numboxes;
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    compute_color_rgb (quantobj, histogram, boxlist + i, i);
}


/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be ints, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.
 *
 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  Furthermore, we need not fill subboxes that are never
 * referenced in pass2; many images use only part of the color gamut, so a
 * fair amount of work is saved.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */


/* log2(histogram cells in update box) for each axis; this can be adjusted */
#define BOX_R_LOG  (PRECISION_R-3)
#define BOX_G_LOG  (PRECISION_G-3)
#define BOX_B_LOG  (PRECISION_B-3)

#define BOX_R_ELEMS  (1<<BOX_R_LOG) /* # of hist cells in update box */
#define BOX_G_ELEMS  (1<<BOX_G_LOG)
#define BOX_B_ELEMS  (1<<BOX_B_LOG)

#define BOX_R_SHIFT  (R_SHIFT + BOX_R_LOG)
#define BOX_G_SHIFT  (G_SHIFT + BOX_G_LOG)
#define BOX_B_SHIFT  (B_SHIFT + BOX_B_LOG)


/*
 * The next three routines implement inverse colormap filling.  They could
 * all be folded into one big routine, but splitting them up this way saves
 * some stack space (the mindist[] and bestdist[] arrays need not coexist)
 * and may allow some compilers to produce better code by registerizing more
 * inner-loop variables.
 */

static int
find_nearby_colors (QuantizeObj *quantobj,
		    int          minR,
		    int          minG,
		    int          minB,
		    int          colorlist[])
/* Locate the colormap entries close enough to an update box to be candidates
 * for the nearest entry to some cell(s) in the update box.  The update box
 * is specified by the center coordinates of its first cell.  The number of
 * candidate colormap entries is returned, and their colormap indexes are
 * placed in colorlist[].
 * This routine uses Heckbert's "locally sorted search" criterion to select
 * the colors that need further consideration.
 */
{
  int numcolors = quantobj->actual_number_of_colors;
  int maxR, maxG, maxB;
  int centerR, centerG, centerB;
  int i, x, ncolors;
  int minmaxdist, min_dist, max_dist, tdist;
  int mindist[MAXNUMCOLORS];	/* min distance to colormap entry i */

  /* Compute true coordinates of update box's upper corner and center.
   * Actually we compute the coordinates of the center of the upper-corner
   * histogram cell, which are the upper bounds of the volume we care about.
   * Note that since ">>" rounds down, the "center" values may be closer to
   * min than to max; hence comparisons to them must be "<=", not "<".
   */
  maxR = minR + ((1 << BOX_R_SHIFT) - (1 << R_SHIFT));
  centerR = (minR + maxR) >> 1;
  maxG = minG + ((1 << BOX_G_SHIFT) - (1 << G_SHIFT));
  centerG = (minG + maxG) >> 1;
  maxB = minB + ((1 << BOX_B_SHIFT) - (1 << B_SHIFT));
  centerB = (minB + maxB) >> 1;

  /* For each color in colormap, find:
   *  1. its minimum squared-distance to any point in the update box
   *     (zero if color is within update box);
   *  2. its maximum squared-distance to any point in the update box.
   * Both of these can be found by considering only the corners of the box.
   * We save the minimum distance for each color in mindist[];
   * only the smallest maximum distance is of interest.
   */
  minmaxdist = 0x7FFFFFFFL;

  for (i = 0; i < numcolors; i++) {
    /* We compute the squared-R-distance term, then add in the other two. */
    x = quantobj->cmap[i].red;
    if (x < minR) {
      tdist = (x - minR) * R_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - maxR) * R_SCALE;
      max_dist = tdist*tdist;
    } else if (x > maxR) {
      tdist = (x - maxR) * R_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - minR) * R_SCALE;
      max_dist = tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      min_dist = 0;
      if (x <= centerR) {
	tdist = (x - maxR) * R_SCALE;
	max_dist = tdist*tdist;
      } else {
	tdist = (x - minR) * R_SCALE;
	max_dist = tdist*tdist;
      }
    }

    x = quantobj->cmap[i].green;
    if (x < minG) {
      tdist = (x - minG) * G_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - maxG) * G_SCALE;
      max_dist += tdist*tdist;
    } else if (x > maxG) {
      tdist = (x - maxG) * G_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - minG) * G_SCALE;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerG) {
	tdist = (x - maxG) * G_SCALE;
	max_dist += tdist*tdist;
      } else {
	tdist = (x - minG) * G_SCALE;
	max_dist += tdist*tdist;
      }
    }

    x = quantobj->cmap[i].blue;
    if (x < minB) {
      tdist = (x - minB) * B_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - maxB) * B_SCALE;
      max_dist += tdist*tdist;
    } else if (x > maxB) {
      tdist = (x - maxB) * B_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - minB) * B_SCALE;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerB) {
	tdist = (x - maxB) * B_SCALE;
	max_dist += tdist*tdist;
      } else {
	tdist = (x - minB) * B_SCALE;
	max_dist += tdist*tdist;
      }
    }

    mindist[i] = min_dist;	/* save away the results */
    if (max_dist < minmaxdist)
      minmaxdist = max_dist;
  }

  /* Now we know that no cell in the update box is more than minmaxdist
   * away from some colormap entry.  Therefore, only colors that are
   * within minmaxdist of some part of the box need be considered.
   */
  ncolors = 0;
  for (i = 0; i < numcolors; i++) {
    if (mindist[i] <= minmaxdist)
      colorlist[ncolors++] = i;
  }
  return ncolors;
}


static void
find_best_colors (QuantizeObj *quantobj,
		  int          minR,
		  int          minG,
		  int          minB,
		  int          numcolors,
		  int          colorlist[],
		  int          bestcolor[])
/* Find the closest colormap entry for each cell in the update box,
 * given the list of candidate colors prepared by find_nearby_colors.
 * Return the indexes of the closest entries in the bestcolor[] array.
 * This routine uses Thomas' incremental distance calculation method to
 * find the distance from a colormap entry to successive cells in the box.
 */
{
  int iR, iG, iB;
  int i, icolor;
  int * bptr;		/* pointer into bestdist[] array */
  int * cptr;		/* pointer into bestcolor[] array */
  int dist0, dist1;	/* initial distance values */
  int dist2;		/* current distance in inner loop */
  int xx0, xx1;		/* distance increments */
  int xx2;
  int inR, inG, inB;	/* initial values for increments */

  /* This array holds the distance to the nearest-so-far color for each cell */
  int bestdist[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

  /* Initialize best-distance for each cell of the update box */
  bptr = bestdist;
  for (i = BOX_R_ELEMS*BOX_G_ELEMS*BOX_B_ELEMS-1; i >= 0; i--)
    *bptr++ = 0x7FFFFFFFL;

  /* For each color selected by find_nearby_colors,
   * compute its distance to the center of each cell in the box.
   * If that's less than best-so-far, update best distance and color number.
   */

  /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_R  ((1 << R_SHIFT) * R_SCALE)
#define STEP_G  ((1 << G_SHIFT) * G_SCALE)
#define STEP_B  ((1 << B_SHIFT) * B_SCALE)

  for (i = 0; i < numcolors; i++) {
    icolor = colorlist[i];
    /* Compute (square of) distance from minR/G/B to this color */
    inR = (minR - quantobj->cmap[icolor].red) * R_SCALE;
    dist0 = inR*inR;
    inG = (minG - quantobj->cmap[icolor].green) * G_SCALE;
    dist0 += inG*inG;
    inB = (minB - quantobj->cmap[icolor].blue) * B_SCALE;
    dist0 += inB*inB;
    /* Form the initial difference increments */
    inR = inR * (2 * STEP_R) + STEP_R * STEP_R;
    inG = inG * (2 * STEP_G) + STEP_G * STEP_G;
    inB = inB * (2 * STEP_B) + STEP_B * STEP_B;
    /* Now loop over all cells in box, updating distance per Thomas method */
    bptr = bestdist;
    cptr = bestcolor;
    xx0 = inR;
    for (iR = BOX_R_ELEMS-1; iR >= 0; iR--) {
      dist1 = dist0;
      xx1 = inG;
      for (iG = BOX_G_ELEMS-1; iG >= 0; iG--) {
	dist2 = dist1;
	xx2 = inB;
	for (iB = BOX_B_ELEMS-1; iB >= 0; iB--) {
	  if (dist2 < *bptr) {
	    *bptr = dist2;
	    *cptr = icolor;
	  }
	  dist2 += xx2;
	  xx2 += 2 * STEP_B * STEP_B;
	  bptr++;
	  cptr++;
	}
	dist1 += xx1;
	xx1 += 2 * STEP_G * STEP_G;
      }
      dist0 += xx0;
      xx0 += 2 * STEP_R * STEP_R;
    }
  }
}


static void
fill_inverse_cmap_gray (QuantizeObj *quantobj,
			Histogram    histogram,
			int          pixel)
/* Fill the inverse-colormap entries in the update box that contains */
/* histogram cell R/G/B.  (Only that one cell MUST be filled, but */
/* we can fill as many others as we wish.) */
{
  Color *cmap;
  long dist;
  long mindist;
  int mindisti;
  int i;

  cmap = quantobj->cmap;

  mindist = 65536;
  mindisti = -1;

  for (i = 0; i < quantobj->actual_number_of_colors; i++)
    {
      dist = pixel - cmap[i].red;
      dist *= dist;

      if (dist < mindist)
	{
	  mindist = dist;
	  mindisti = i;
	}
    }

  if (i >= 0)
    histogram[pixel] = mindisti + 1;
}


static void
fill_inverse_cmap_rgb (QuantizeObj *quantobj,
		       Histogram    histogram,
		       int          R,
		       int          G,
		       int          B)
/* Fill the inverse-colormap entries in the update box that contains */
/* histogram cell R/G/B.  (Only that one cell MUST be filled, but */
/* we can fill as many others as we wish.) */
{
  int minR, minG, minB;	/* lower left corner of update box */
  int iR, iG, iB;
  int * cptr;   	/* pointer into bestcolor[] array */
  ColorFreq * cachep;	/* pointer into main cache array */
  /* This array lists the candidate colormap indexes. */
  int colorlist[MAXNUMCOLORS];
  int numcolors;		/* number of candidate colors */
  /* This array holds the actually closest colormap index for each cell. */
  int bestcolor[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

  /* Convert cell coordinates to update box id */
  R >>= BOX_R_LOG;
  G >>= BOX_G_LOG;
  B >>= BOX_B_LOG;

  /* Compute true coordinates of update box's origin corner.
   * Actually we compute the coordinates of the center of the corner
   * histogram cell, which are the lower bounds of the volume we care about.
   */
  minR = (R << BOX_R_SHIFT) + ((1 << R_SHIFT) >> 1);
  minG = (G << BOX_G_SHIFT) + ((1 << G_SHIFT) >> 1);
  minB = (B << BOX_B_SHIFT) + ((1 << B_SHIFT) >> 1);

  /* Determine which colormap entries are close enough to be candidates
   * for the nearest entry to some cell in the update box.
   */
  numcolors = find_nearby_colors (quantobj, minR, minG, minB, colorlist);

  /* Determine the actually nearest colors. */
  find_best_colors (quantobj, minR, minG, minB, numcolors, colorlist,
		    bestcolor);

  /* Save the best color numbers (plus 1) in the main cache array */
  R <<= BOX_R_LOG;		/* convert id back to base cell indexes */
  G <<= BOX_G_LOG;
  B <<= BOX_B_LOG;
  cptr = bestcolor;
  for (iR = 0; iR < BOX_R_ELEMS; iR++) {
    for (iG = 0; iG < BOX_G_ELEMS; iG++) {
      cachep = & histogram[(R+iR)*MR+(G+iG)*MG+B];
      for (iB = 0; iB < BOX_B_ELEMS; iB++) {
	*cachep++ = (*cptr++) + 1;
      }
    }
  }
}


/*  This is pass 1  */

static void
median_cut_pass1_gray (QuantizeObj *quantobj)
{
  select_colors_gray (quantobj, quantobj->histogram);
}


static void
median_cut_pass1_rgb (QuantizeObj *quantobj)
{
  select_colors_rgb (quantobj, quantobj->histogram);
}


static void
monopal_pass1 (QuantizeObj *quantobj)
{
  quantobj -> actual_number_of_colors = 2;
  quantobj -> cmap[0].red = 0;
  quantobj -> cmap[0].green = 0;
  quantobj -> cmap[0].blue = 0;
  quantobj -> cmap[1].red = 255;
  quantobj -> cmap[1].green = 255;
  quantobj -> cmap[1].blue = 255;
}
static void
webpal_pass1 (QuantizeObj *quantobj)
{
  int i;
  quantobj -> actual_number_of_colors = 216;
  for (i=0;i<216;i++)
    {
      quantobj->cmap[i].red = webpal[i*3];
      quantobj->cmap[i].green = webpal[i*3 +1];
      quantobj->cmap[i].blue = webpal[i*3 +2];
    }
}
static void
custompal_pass1 (QuantizeObj *quantobj)
{
  int i;
  GSList *list;
  PaletteEntryP entry;

  /*  fprintf(stderr, "custompal_pass1: using (theCustomPalette %s) from (file %s)\n",
			 theCustomPalette->name, theCustomPalette->filename);*/

  for (i=0,list=theCustomPalette->colors;
       list;
       i++,list=g_slist_next(list))
    {
      entry=(PaletteEntryP)list->data;
      quantobj->cmap[i].red = entry->color[0];
      quantobj->cmap[i].green = entry->color[1];
      quantobj->cmap[i].blue = entry->color[2];
    }
  quantobj -> actual_number_of_colors = i;
}

/*
 * Map some rows of pixels to the output colormapped representation.
 */

static void
median_cut_pass2_no_dither_gray (QuantizeObj *quantobj,
				 Layer       *layer,
				 TileManager *new_tiles)
{
  PixelRegion srcPR, destPR;
  Histogram histogram = quantobj->histogram;
  ColorFreq * cachep;
  unsigned char *src, *dest;
  int row, col;
  int pixel;
  int has_alpha;
  void *pr;

  zero_histogram_gray (histogram);

  has_alpha = layer_has_alpha (layer);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  pixel_region_init (&destPR, new_tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      for (row = 0; row < srcPR.h; row++)
	{
	  for (col = 0; col < srcPR.w; col++)
	    {
	      /* get pixel value and index into the cache */
	      pixel = src[GRAY_PIX];
	      cachep = &histogram[pixel];
	      /* If we have not seen this color before, find nearest colormap entry */
	      /* and update the cache */
	      if (*cachep == 0)
		fill_inverse_cmap_gray (quantobj, histogram, pixel);
	      /* Now emit the colormap index for this cell */
	      dest[INDEXED_PIX] = *cachep - 1;
	      if (has_alpha)
		dest[ALPHA_I_PIX] = (src[ALPHA_G_PIX] > 127) ? 255 : 0;

	      src += srcPR.bytes;
	      dest += destPR.bytes;
	    }
	}
    }
}

static void
median_cut_pass2_no_dither_rgb (QuantizeObj *quantobj,
				Layer       *layer,
				TileManager *new_tiles)
{
  PixelRegion srcPR, destPR;
  Histogram histogram = quantobj->histogram;
  ColorFreq * cachep;
  unsigned char *src, *dest;
  int R, G, B;
  int row, col;
  int has_alpha;
  void *pr;
  int red_pix = RED_PIX;
  int green_pix = GREEN_PIX;
  int blue_pix = BLUE_PIX;
  int alpha_pix = ALPHA_PIX;

  /*  In the case of web/mono palettes, we actually force
   *   grayscale drawables through the rgb pass2 functions
   */
  if (drawable_gray (GIMP_DRAWABLE(layer)))
    red_pix = green_pix = blue_pix = GRAY_PIX;

  zero_histogram_rgb (histogram);

  has_alpha = layer_has_alpha (layer);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  pixel_region_init (&destPR, new_tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      for (row = 0; row < srcPR.h; row++)
	{
	  for (col = 0; col < srcPR.w; col++)
	    {
	      /* get pixel value and index into the cache */
	      R = (src[red_pix]) >> R_SHIFT;
	      G = (src[green_pix]) >> G_SHIFT;
	      B = (src[blue_pix]) >> B_SHIFT;
	      cachep = &histogram[R*MR + G*MG + B];
	      /* If we have not seen this color before, find nearest colormap entry */
	      /* and update the cache */
	      if (*cachep == 0)
		fill_inverse_cmap_rgb (quantobj, histogram, R, G, B);
	      /* Now emit the colormap index for this cell */
	      dest[INDEXED_PIX] = *cachep - 1;

	      if (has_alpha)
		dest[ALPHA_I_PIX] = (src[alpha_pix] > 127) ? 255 : 0;

	      src += srcPR.bytes;
	      dest += destPR.bytes;
	    }
	}
    }
}

static void
median_cut_pass2_nodestruct_dither_rgb (QuantizeObj *quantobj,
					Layer       *layer,
					TileManager *new_tiles)
{
  PixelRegion srcPR, destPR;
  unsigned char *src, *dest;
  int row, col;
  int has_alpha;
  void *pr;
  int red_pix = RED_PIX;
  int green_pix = GREEN_PIX;
  int blue_pix = BLUE_PIX;
  int alpha_pix = ALPHA_PIX;
  int i;
  int lastindex = 0;
  int lastred = -1;
  int lastgreen = -1;
  int lastblue = -1;


  has_alpha = layer_has_alpha (layer);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  pixel_region_init (&destPR, new_tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      for (row = 0; row < srcPR.h; row++)
	{
	  for (col = 0; col < srcPR.w; col++)
	    {
	      if ((has_alpha && (src[alpha_pix]>127))
		  || !has_alpha)
		{
		  if ((lastred == src[red_pix]) &&
		      (lastgreen == src[green_pix]) &&
		      (lastblue == src[blue_pix]))
		    {
		      /*  same pixel colour as last time  */
		      dest[INDEXED_PIX] = lastindex;
		      if (has_alpha)
			dest[ALPHA_I_PIX] = 255;
		    }
		  else
		    {
		      for (i = 0 ;
			   i < quantobj->actual_number_of_colors;
			   i++)
			{
			  if (
			      (quantobj->cmap[i].red == src[red_pix]) &&
			      (quantobj->cmap[i].green == src[green_pix]) &&
			      (quantobj->cmap[i].blue == src[blue_pix])
			      )
			  {
			    lastred = src[red_pix];
			    lastgreen = src[green_pix];
			    lastblue = src[blue_pix];
			    lastindex = i;
			    goto got_colour;
			  }
			}
		      g_error ("Non-existant colour was expected to "
			       "be in non-destructive colourmap.");
		    got_colour:
		      dest[INDEXED_PIX] = lastindex;
		      if (has_alpha)
			dest[ALPHA_I_PIX] = 255;
		    }
		}
	      else
		{ /*  have alpha, and transparent  */
		  dest[ALPHA_I_PIX] = 0;
		}

	      src += srcPR.bytes;
	      dest += destPR.bytes;
	    }
	}
    }
}


/*
 * Initialize the error-limiting transfer function (lookup table).
 * The raw F-S error computation can potentially compute error values of up to
 * +- MAXJSAMPLE.  But we want the maximum correction applied to a pixel to be
 * much less, otherwise obviously wrong pixels will be created.  (Typical
 * effects include weird fringes at color-area boundaries, isolated bright
 * pixels in a dark area, etc.)  The standard advice for avoiding this problem
 * is to ensure that the "corners" of the color cube are allocated as output
 * colors; then repeated errors in the same direction cannot cause cascading
 * error buildup.  However, that only prevents the error from getting
 * completely out of hand; Aaron Giles reports that error limiting improves
 * the results even with corner colors allocated.
 * A simple clamping of the error values to about +- MAXJSAMPLE/8 works pretty
 * well, but the smoother transfer function used below is even better.  Thanks
 * to Aaron Giles for this idea.
 */

static int *
init_error_limit (void)
/* Allocate and fill in the error_limiter table */
{
  int *table;
  int in, out;

  table = g_malloc (sizeof (int) * (255 * 2 + 1));
  table += 255;                 /* so we can index -255 ... +255 */

#define STEPSIZE 16

  /* Map errors 1:1 up to +- 16 */
  out = 0;
  for (in = 0; in < STEPSIZE; in++, out++)
    {
      table[in] = out;
      table[-in] = -out;
    }

  /* Map errors 1:2 up to +- 3*16 */
  for (; in < STEPSIZE*3; in++, out += (in&1) ? 0 : 1)
    {
      table[in] = out;
      table[-in] = -out;
    }

  /* Clamp the rest to final out value (which is 32) */
  for (; in <= 255; in++)
    {
      table[in] = out;
      table[-in] = -out;
    }

#undef STEPSIZE

  return table;
}


/*
 * Map some rows of pixels to the output colormapped representation.
 * Perform floyd-steinberg dithering.
 */

static void
median_cut_pass2_fs_dither_gray (QuantizeObj *quantobj,
				 Layer       *layer,
				 TileManager *new_tiles)
{
  PixelRegion srcPR, destPR;
  Histogram histogram = quantobj->histogram;
  ColorFreq *cachep;
  Color *color;
  int *error_limiter;
  short *fs_err1, *fs_err2;
  short *fs_err3, *fs_err4;
  short *range_limiter;
  int src_bytes, dest_bytes;
  unsigned char *src, *dest;
  unsigned char *src_buf, *dest_buf;
  int *next_row, *prev_row;
  int *nr, *pr;
  int *tmp;
  int pixel;
  int pixele;
  int row, col;
  int index;
  int step_dest, step_src;
  int odd_row;
  int has_alpha;
  int width, height;

  zero_histogram_gray (histogram);

  has_alpha = layer_has_alpha (layer);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  pixel_region_init (&destPR, new_tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
  src_bytes = drawable_bytes (GIMP_DRAWABLE(layer));
  dest_bytes = new_tiles->levels[0].bpp;
  width = GIMP_DRAWABLE(layer)->width;
  height = GIMP_DRAWABLE(layer)->height;

  error_limiter = init_error_limit ();
  range_limiter = range_array + 256;

  src_buf = g_malloc (width * src_bytes);
  dest_buf = g_malloc (width * dest_bytes);
  next_row = g_malloc (sizeof (int) * (width + 2));
  prev_row = g_malloc (sizeof (int) * (width + 2));

  memset (prev_row, 0, (width + 2) * sizeof (int));

  fs_err1 = floyd_steinberg_error1 + 511;
  fs_err2 = floyd_steinberg_error2 + 511;
  fs_err3 = floyd_steinberg_error3 + 511;
  fs_err4 = floyd_steinberg_error4 + 511;

  odd_row = 0;

  for (row = 0; row < height; row++)
    {
      pixel_region_get_row (&srcPR, 0, row, width, src_buf, 1);

      src = src_buf;
      dest = dest_buf;

      nr = next_row;
      pr = prev_row + 1;

      if (odd_row)
	{
	  step_dest = -dest_bytes;
	  step_src = -src_bytes;

	  src += (width * src_bytes) - src_bytes;
	  dest += (width * dest_bytes) - dest_bytes;

	  nr += width + 1;
	  pr += width;

	  *(nr - 1) = 0;
	}
      else
	{
	  step_dest = dest_bytes;
	  step_src = src_bytes;

	  *(nr + 1) = 0;
	}

      *nr = 0;

      for (col = 0; col < width; col++)
	{
	  pixel = range_limiter[src[GRAY_PIX] + error_limiter[*pr]];

	  cachep = &histogram[pixel];
	  /* If we have not seen this color before, find nearest colormap entry */
	  /* and update the cache */
	  if (*cachep == 0)
	    fill_inverse_cmap_gray (quantobj, histogram, pixel);

	  index = *cachep - 1;
	  dest[INDEXED_PIX] = index;

	  if (has_alpha)
	    dest[ALPHA_I_PIX] = (src[ALPHA_G_PIX] > 127) ? 255 : 0;

	  color = &quantobj->cmap[index];
	  pixele = pixel - color->red;

	  if (odd_row)
	    {
	      *(--pr) += fs_err1[pixele];
	      *nr-- += fs_err2[pixele];
	      *nr += fs_err3[pixele];
	      *(nr-1) = fs_err4[pixele];
	    }
	  else
	    {
	      *(++pr) += fs_err1[pixele];
	      *nr++ += fs_err2[pixele];
	      *nr += fs_err3[pixele];
	      *(nr+1) = fs_err4[pixele];
	    }

	  dest += step_dest;
	  src += step_src;
	}

      tmp = next_row;
      next_row = prev_row;
      prev_row = tmp;

      odd_row = !odd_row;

      pixel_region_set_row (&destPR, 0, row, width, dest_buf);
    }

  g_free (error_limiter - 255);
  g_free (next_row);
  g_free (prev_row);
  g_free (src_buf);
  g_free (dest_buf);
}

static void
median_cut_pass2_fs_dither_rgb (QuantizeObj *quantobj,
				Layer       *layer,
				TileManager *new_tiles)
{
  PixelRegion srcPR, destPR;
  Histogram histogram = quantobj->histogram;
  ColorFreq *cachep;
  Color *color;
  int *error_limiter;
  short *fs_err1, *fs_err2;
  short *fs_err3, *fs_err4;
  short *range_limiter;
  int src_bytes, dest_bytes;
  unsigned char *src, *dest;
  unsigned char *src_buf, *dest_buf;
  int *red_n_row, *red_p_row;
  int *grn_n_row, *grn_p_row;
  int *blu_n_row, *blu_p_row;
  int *rnr, *rpr;
  int *gnr, *gpr;
  int *bnr, *bpr;
  int *tmp;
  int r, g, b;
  int re, ge, be;
  int row, col;
  int index;
  int step_dest, step_src;
  int odd_row;
  int has_alpha;
  int width, height;
  int red_pix = RED_PIX;
  int green_pix = GREEN_PIX;
  int blue_pix = BLUE_PIX;
  int alpha_pix = ALPHA_PIX;

  /*  In the case of web/mono palettes, we actually force
   *   grayscale drawables through the rgb pass2 functions
   */
  if (drawable_gray (GIMP_DRAWABLE(layer)))
    red_pix = green_pix = blue_pix = GRAY_PIX;

  zero_histogram_rgb (histogram);

  has_alpha = layer_has_alpha (layer);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
  pixel_region_init (&destPR, new_tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
  src_bytes = drawable_bytes (GIMP_DRAWABLE(layer));
  dest_bytes = new_tiles->levels[0].bpp;
  width = GIMP_DRAWABLE(layer)->width;
  height = GIMP_DRAWABLE(layer)->height;

  error_limiter = init_error_limit ();
  range_limiter = range_array + 256;

  src_buf = g_malloc (width * src_bytes);
  dest_buf = g_malloc (width * dest_bytes);
  red_n_row = g_malloc (sizeof (int) * (width + 2));
  red_p_row = g_malloc (sizeof (int) * (width + 2));
  grn_n_row = g_malloc (sizeof (int) * (width + 2));
  grn_p_row = g_malloc (sizeof (int) * (width + 2));
  blu_n_row = g_malloc (sizeof (int) * (width + 2));
  blu_p_row = g_malloc (sizeof (int) * (width + 2));

  memset (red_p_row, 0, (width + 2) * sizeof (int));
  memset (grn_p_row, 0, (width + 2) * sizeof (int));
  memset (blu_p_row, 0, (width + 2) * sizeof (int));

  fs_err1 = floyd_steinberg_error1 + 511;
  fs_err2 = floyd_steinberg_error2 + 511;
  fs_err3 = floyd_steinberg_error3 + 511;
  fs_err4 = floyd_steinberg_error4 + 511;

  odd_row = 0;

  for (row = 0; row < height; row++)
    {
      pixel_region_get_row (&srcPR, 0, row, width, src_buf, 1);

      src = src_buf;
      dest = dest_buf;

      rnr = red_n_row;
      gnr = grn_n_row;
      bnr = blu_n_row;
      rpr = red_p_row + 1;
      gpr = grn_p_row + 1;
      bpr = blu_p_row + 1;

      if (odd_row)
	{
	  step_dest = -dest_bytes;
	  step_src = -src_bytes;

	  src += (width * src_bytes) - src_bytes;
	  dest += (width * dest_bytes) - dest_bytes;

	  rnr += width + 1;
	  gnr += width + 1;
	  bnr += width + 1;
	  rpr += width;
	  gpr += width;
	  bpr += width;

	  *(rnr - 1) = *(gnr - 1) = *(bnr - 1) = 0;
	}
      else
	{
	  step_dest = dest_bytes;
	  step_src = src_bytes;

	  *(rnr + 1) = *(gnr + 1) = *(bnr + 1) = 0;
	}

      *rnr = *gnr = *bnr = 0;

      for (col = 0; col < width; col++)
	{
	  r = range_limiter[src[red_pix] + error_limiter[*rpr]];
	  g = range_limiter[src[green_pix] + error_limiter[*gpr]];
	  b = range_limiter[src[blue_pix] + error_limiter[*bpr]];

	  re = r >> R_SHIFT;
	  ge = g >> G_SHIFT;
	  be = b >> B_SHIFT;

	  cachep = &histogram[re*MR + ge*MG + be];
	  /* If we have not seen this color before, find nearest colormap entry */
	  /* and update the cache */
	  if (*cachep == 0)
	    fill_inverse_cmap_rgb (quantobj, histogram, re, ge, be);
	  index = *cachep - 1;
	  dest[INDEXED_PIX] = index;

	  if (has_alpha)
	    dest[ALPHA_I_PIX] = (src[alpha_pix] > 127) ? 255 : 0;

          /* Abortive initial attempt at alpha-dithering :) */
	  /*	  if (has_alpha)
	    dest[ALPHA_I_PIX] = (src[alpha_pix] <= (random()&255)) ? 0 : 255;*/

	  color = &quantobj->cmap[index];
	  re = r - color->red;
	  ge = g - color->green;
	  be = b - color->blue;

	  if (odd_row)
	    {
	      *(--rpr) += fs_err1[re];
	      *(--gpr) += fs_err1[ge];
	      *(--bpr) += fs_err1[be];

	      *rnr-- += fs_err2[re];
	      *gnr-- += fs_err2[ge];
	      *bnr-- += fs_err2[be];

	      *rnr += fs_err3[re];
	      *gnr += fs_err3[ge];
	      *bnr += fs_err3[be];

	      *(rnr-1) = fs_err4[re];
	      *(gnr-1) = fs_err4[ge];
	      *(bnr-1) = fs_err4[be];
	    }
	  else
	    {
	      *(++rpr) += fs_err1[re];
	      *(++gpr) += fs_err1[ge];
	      *(++bpr) += fs_err1[be];

	      *rnr++ += fs_err2[re];
	      *gnr++ += fs_err2[ge];
	      *bnr++ += fs_err2[be];

	      *rnr += fs_err3[re];
	      *gnr += fs_err3[ge];
	      *bnr += fs_err3[be];

	      *(rnr+1) = fs_err4[re];
	      *(gnr+1) = fs_err4[ge];
	      *(bnr+1) = fs_err4[be];
	    }

	  dest += step_dest;
	  src += step_src;
	}

      tmp = red_n_row;
      red_n_row = red_p_row;
      red_p_row = tmp;

      tmp = grn_n_row;
      grn_n_row = grn_p_row;
      grn_p_row = tmp;

      tmp = blu_n_row;
      blu_n_row = blu_p_row;
      blu_p_row = tmp;

      odd_row = !odd_row;

      pixel_region_set_row (&destPR, 0, row, width, dest_buf);
    }

  g_free (error_limiter - 255);
  g_free (red_n_row);
  g_free (red_p_row);
  g_free (grn_n_row);
  g_free (grn_p_row);
  g_free (blu_n_row);
  g_free (blu_p_row);
  g_free (src_buf);
  g_free (dest_buf);
}


static void
delete_median_cut (QuantizeObj *quantobj)
{
  g_free (quantobj->histogram);
  g_free (quantobj);
}


/**************************************************************/


static QuantizeObj*
initialize_median_cut (int type,
		       int num_colors,
		       int dither_type,
		       int palette_type)
{
  QuantizeObj * quantobj;

  /* Initialize the data structures */
  quantobj = g_malloc_zero (sizeof (QuantizeObj));

  if (type == GRAY && palette_type == MAKE_PALETTE)
    quantobj->histogram = g_malloc (sizeof (ColorFreq) * 256);
  else
    quantobj->histogram = g_malloc (sizeof (ColorFreq) *
				    HIST_R_ELEMS *
				    HIST_G_ELEMS *
				    HIST_B_ELEMS);

  quantobj->desired_number_of_colors = num_colors;

  switch (type)
    {
    case GRAY:
      switch (palette_type)
	{
	case MAKE_PALETTE:
	  quantobj->first_pass = median_cut_pass1_gray;
	  break;
	case WEB_PALETTE:
	  quantobj->first_pass = webpal_pass1;
	  break;
	case CUSTOM_PALETTE:
	  quantobj->first_pass = custompal_pass1;
	  needs_quantize=TRUE;
	  break;
	case MONO_PALETTE:
	default:
	  quantobj->first_pass = monopal_pass1;
	}
      if (palette_type == WEB_PALETTE ||
	  palette_type == MONO_PALETTE || palette_type == CUSTOM_PALETTE)
	switch (dither_type)
	  {
	  case NODITHER:
	    quantobj->second_pass = median_cut_pass2_no_dither_rgb;
	    break;
	  case FSDITHER:
	    quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
	    break;
	  }
      else
	switch (dither_type)
	  {
	  case NODITHER:
	    quantobj->second_pass = median_cut_pass2_no_dither_gray;
	    break;
	  case FSDITHER:
	    quantobj->second_pass = median_cut_pass2_fs_dither_gray;
	    break;
	  }
      break;
    case RGB:
      switch (palette_type)
	{
	case MAKE_PALETTE:
	  quantobj->first_pass = median_cut_pass1_rgb;
	  break;
	case WEB_PALETTE:
	  quantobj->first_pass = webpal_pass1;
	  needs_quantize=TRUE;
	  break;
	case CUSTOM_PALETTE:
	  quantobj->first_pass = custompal_pass1;
	  needs_quantize=TRUE;
	  break;
	case MONO_PALETTE:
	default:
	  quantobj->first_pass = monopal_pass1;
	}
      switch (dither_type)
	{
	case NODITHER:
	  quantobj->second_pass = median_cut_pass2_no_dither_rgb;
	  break;
	case FSDITHER:
	  quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
	  break;
	case NODESTRUCTDITHER:
	  quantobj->second_pass = median_cut_pass2_nodestruct_dither_rgb;
	  break;
	}
      break;
    }
  quantobj->delete_func = delete_median_cut;

  return quantobj;
}

#endif


/*
 *  Procedure database functions and data structures
 */

/*  The convert-rgb procedure definition  */
ProcArg convert_rgb_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  }
};

ProcRecord convert_rgb_proc =
{
  "gimp_convert_rgb",
  "Convert specified image to RGB color",
  "This procedure converts the specified image to RGB color.  This process requires an image of type GRAY or INDEXED.  No image content is lost in this process aside from the colormap for an indexed image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  convert_rgb_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { convert_rgb_invoker } },
};


static Argument *
convert_rgb_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  make sure the drawable is not RGB color  */
  if (success)
    success = (tag_format (gimage_tag (gimage)) != FORMAT_RGB);

  if (success)
    convert_image ((void *) gimage, FORMAT_RGB, 0, 0, 0);

  return procedural_db_return_args (&convert_rgb_proc, success);
}



/*  The convert-grayscale procedure definition  */
ProcArg convert_grayscale_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  }
};

ProcRecord convert_grayscale_proc =
{
  "gimp_convert_grayscale",
  "Convert specified image to grayscale (256 intensity levels)",
  "This procedure converts the specified image to grayscale with 8 bits per pixel (256 intensity levels).  This process requires an image of type RGB or INDEXED.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  convert_grayscale_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { convert_grayscale_invoker } },
};


static Argument *
convert_grayscale_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  make sure the drawable is not GRAYSCALE color  */
  if (success)
    success = (tag_format (gimage_tag (gimage)) != FORMAT_GRAY);

  if (success)
    convert_image ((void *) gimage, FORMAT_GRAY, 0, 0, 0);

  return procedural_db_return_args (&convert_grayscale_proc, success);
}



/*  The convert-indexed procedure definition  */
ProcArg convert_indexed_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_INT32,
    "dither",
    "Floyd-Steinberg dithering"
  },
  { PDB_INT32,
    "num_cols",
    "the number of colors to quantize to"
  }
};

ProcRecord convert_indexed_proc =
{
  "gimp_convert_indexed",
  "Convert specified image to indexed color",
  "This procedure converts the specified image to indexed color.  This process requires an image of type GRAY or RGB.  The 'num_cols' arguments specifies how many colors the resulting image should be quantized to (1-256).",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  convert_indexed_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { convert_indexed_invoker } },
};


static Argument *
convert_indexed_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  int dither;
  int num_cols;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }
  /*  make sure the drawable is not INDEXED color  */
  if (success)
    success = (tag_format (gimage_tag (gimage)) != FORMAT_INDEXED);
  if (success)
    dither = (args[1].value.pdb_int) ? TRUE : FALSE;
  if (success)
    {
      num_cols = args[2].value.pdb_int;
      if (num_cols < 1 || num_cols > MAXNUMCOLORS)
        success = FALSE;
    }

  if (success)
    convert_image ((void *) gimage, FORMAT_INDEXED, num_cols, dither, 0);

  return procedural_db_return_args (&convert_indexed_proc, success);
}



/*  The convert-indexed-palette procedure definition  */
ProcArg convert_indexed_palette_args[] =
{
  { PDB_IMAGE,
    "image",
    "The Image"
  },
  { PDB_INT32,
    "dither",
    "Floyd-Steinberg dithering"
  },
  { PDB_INT32,
    "palette_type",
    "The type of palette to use, (0 optimal) (1 reuse) (2 WWW) (3 Mono) (4 Custom)"
  },
  { PDB_INT32,
    "num_cols",
    "the number of colors to quantize to, ignored unless (palette_type == 0)"
  },
  { PDB_STRING,
    "palette",
    "The name of the custom palette to use, ignored unless (palette_type == 4)"
  }
};

ProcRecord convert_indexed_palette_proc =
{
  "gimp_convert_indexed_palette",
  "Convert specified image to indexed color",
  "This procedure converts the specified image to indexed color.  This process requires an image of type GRAY or RGB.  The `palette_type' specifies what kind of palette to use, A type of `0' means to use an optimal palette of `num_cols' generated from the colors in the image.  A type of `1' means to re-use the previous palette.  A type of `2' means to use the WWW-optimized palette.  Type `3' means to use only black and white colors.  A type of `4' means to use a palette from the gimp palettes directories.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  convert_indexed_palette_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { convert_indexed_palette_invoker } },
};


static Argument *
convert_indexed_palette_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  int dither;
  int num_cols=0;
  int palette_type;
  char *palette_name;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  make sure the drawable is not INDEXED color  */
  if (success)
    success = (gimage_format (gimage) != FORMAT_INDEXED);
  if (success)
    dither = (args[1].value.pdb_int) ? TRUE : FALSE;
  if (success)
    {
      PaletteEntriesP entries, the_palette = NULL;
      GSList *list;

		palette_type = args[2].value.pdb_int;
		switch(palette_type) {
		  case MAKE_PALETTE:
          num_cols = args[3].value.pdb_int;
          if (num_cols < 1 || num_cols > MAXNUMCOLORS)
            success = FALSE;
		  break;
		  case REUSE_PALETTE:
		  break;
		  case WEB_PALETTE:
		  break;
		  case MONO_PALETTE:
		  break;
		  case CUSTOM_PALETTE:
          palette_name = args[4].value.pdb_pointer;
 /*         fprintf(stderr, "looking for palette `%s'\n", palette_name); */
          if (!palette_entries_list) palette_init_palettes(FALSE);
		    for(list = palette_entries_list;
              list;
              list = g_slist_next(list)) {
                entries = (PaletteEntriesP) list->data;
                if (strcmp(palette_name, entries->name)==0) {
	/*					fprintf(stderr, "found it!\n"); */
                  the_palette = entries;
                  break;
					 }
			 }
          if (the_palette == NULL) {
	/*			fprintf(stderr, "didn't find it\n"); */
            success = FALSE;
			 }
          else
            theCustomPalette = the_palette;
		  break;
		  default:
          success = FALSE;
		}
	 }
    if (success)
       convert_image ((void *) gimage, INDEXED, num_cols, dither, palette_type);
  return procedural_db_return_args (&convert_indexed_palette_proc, success);
}
