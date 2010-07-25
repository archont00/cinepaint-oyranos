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
#include <math.h>
#include "config.h"
#include "libgimp/gimpintl.h"
#include "../appenv.h"
#include "../brush.h"
#include "../brushlist.h"
#include "../canvas.h"
#include "../drawable.h"
#include "../errors.h"
#include "float16.h"
#include "../dodgeburn.h"
#include "../gdisplay.h"
#include "../paint_funcs_area.h"
#include "../paint_core_16.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../selection.h"
#include "../tools.h"
#include "../devices.h"

void dodgeburn_area ( PixelArea *, PixelArea *, gint,gint,gfloat  );
typedef void  (*DodgeburnRowFunc) (PixelArea*,PixelArea*,gint,gfloat);

//u16
void dodge_highlights_row_u16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row_u16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row_u16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);

//bfp
void dodge_highlights_row_bfp ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row_bfp ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row_bfp ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);

//float16
void dodge_highlights_row_float16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row_float16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row_float16 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);

//float
void dodge_highlights_row_float ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row_float ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row_float ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);

// u8
void dodge_highlights_row_u8 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_midtones_row_u8 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);
void dodge_shadows_row_u8 ( PixelRow *src_row, PixelRow *dest_row, 
	gint type, gfloat exposure);


DodgeburnRowFunc dodgeburn_row_func (Tag, gint);
/*  the dodgeburn structures  */


typedef enum
{
  DODGE,
  BURN 
} DodgeBurnType;

typedef enum
{
  DODGEBURN_HIGHLIGHTS,
  DODGEBURN_MIDTONES,
  DODGEBURN_SHADOWS 
} DodgeBurnMode;

typedef struct DodgeBurnOptions DodgeBurnOptions;
struct DodgeBurnOptions
{
  DodgeBurnType  type;
  DodgeBurnType  type_d;
  GtkWidget    *type_w[2];

  DodgeBurnMode  mode;     /*highlights,midtones,shadows*/
  DodgeBurnMode  mode_d;
  GtkWidget      *mode_w[3];

  double        exposure;
  double        exposure_d;
  GtkObject    *exposure_w;
};

/*  the dodgeburn tool options  */
static DodgeBurnOptions * dodgeburn_options = NULL;

static void dodgeburn_painthit_setup (PaintCore *,Canvas *);
static void dodgeburn_scale_update (GtkAdjustment *, double *);
static DodgeBurnOptions * create_dodgeburn_options(void);
static void * dodgeburn_paint_func (PaintCore *,  CanvasDrawable *, int);

static GtkWidget*
dodgeburn_radio_buttons_new (gchar*      label,
                                gpointer    toggle_val,
                                GtkWidget*  button_widget[],
                                gchar*      button_label[],
                                gint        button_value[],
                                gint        num);
static void
dodgeburn_radio_buttons_update (GtkWidget *widget,
                                   gpointer   data);

static void         dodgeburn_motion 	(PaintCore *, CanvasDrawable *);
static void 	    dodgeburn_init   	(PaintCore *, CanvasDrawable *);
static void         dodgeburn_finish    (PaintCore *, CanvasDrawable *);

#if 0
static gfloat dodgeburn_highlights_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_midtones_lut_func(void *, int, int, gfloat);
static gfloat dodgeburn_shadows_lut_func(void *, int, int, gfloat);

/* The dodge burn lookup tables */
gfloat dodgeburn_highlights(void *, int, int, gfloat);
gfloat dodgeburn_midtones(void *, int, int, gfloat);
gfloat dodgeburn_shadows(void *, int, int, gfloat);
#endif

#define INT_MULT_16(a,b,t) ((t) = (a) * (b) + 0x8000, ((((t) >> 16) + (t)) >> 16))

static void
dodgeburn_scale_update (GtkAdjustment *adjustment,
			 double        *scale_val)
{
  *scale_val = adjustment->value;
}

static DodgeBurnOptions *
create_dodgeburn_options(void)
{
  DodgeBurnOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;

  gchar* type_label[2] = { _("Dodge"), _("Burn") };
  gint   type_value[2] = { DODGE, BURN };

  gchar* mode_label[3] = { _("Highlights"), 
			   _("Midtones"),
			   _("Shadows") };

  gint   mode_value[3] = { DODGEBURN_HIGHLIGHTS, 
			   DODGEBURN_MIDTONES, 
			   DODGEBURN_SHADOWS };

  /*  the new dodgeburn tool options structure  */
  options = (DodgeBurnOptions *) g_malloc_zero (sizeof (DodgeBurnOptions));
  options->type     = options->type_d     = DODGE;
  options->exposure = options->exposure_d = 50.0;
  options->mode = options->mode_d = DODGEBURN_HIGHLIGHTS;

  /*  the main vbox  */
  vbox = gtk_vbox_new(FALSE, 1);

  /*  the exposure scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Exposure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->exposure_w =
    gtk_adjustment_new (options->exposure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->exposure_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

  gtk_signal_connect (GTK_OBJECT (options->exposure_w), "value_changed",
		      (GtkSignalFunc) dodgeburn_scale_update,
		      &options->exposure);

  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  /* the type (dodge or burn) */
  frame = dodgeburn_radio_buttons_new (_("Type"), 
					  &options->type,
					   options->type_w,
					   type_label,
					   type_value,
					   2);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  mode (highlights, midtones, or shadows)  */
  frame = dodgeburn_radio_buttons_new (_("Mode"), 
					  &options->mode,
					   options->mode_w,
					   mode_label,
					   mode_value,
					   3);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (options->mode_w[options->mode_d]), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tools_register_options(DODGEBURN, vbox);

  return options;
}

static GtkWidget*
dodgeburn_radio_buttons_new (gchar*      label,
                                gpointer    toggle_val,
                                GtkWidget*  button_widget[],
                                gchar*      button_label[],
                                gint        button_value[],
                                gint        num)
{ 
  GtkWidget *frame;
  GtkWidget *vbox;                        
  GSList *group = NULL;                    
  gint i;                                  

  frame = gtk_frame_new (label);        

  g_return_val_if_fail (toggle_val != NULL, frame); 

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  for (i=0; i<num; i++)                    
    {                                      
      button_widget[i] = gtk_radio_button_new_with_label (group,
                                                          button_label[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button_widget[i]));
      gtk_box_pack_start (GTK_BOX (vbox), button_widget[i], FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button_widget[i]), "toggled",
                          (GtkSignalFunc) dodgeburn_radio_buttons_update,
                          toggle_val);
      gtk_object_set_data (GTK_OBJECT (button_widget[i]), "toggle_value",
                           (gpointer)button_value[i]);                                        
      gtk_widget_show (button_widget[i]);                                                     
    }                                                                                         
  gtk_widget_show (vbox);                                                                     

  return (frame);                                                                             
}     

static void
dodgeburn_radio_buttons_update (GtkWidget *widget,
                                   gpointer   data)
{
  int *toggle_val;
    
  toggle_val = (int *) data;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = (int) gtk_object_get_data (GTK_OBJECT (widget), "toggle_value");
}


void *
dodgeburn_paint_func (PaintCore    *paint_core,
		     CanvasDrawable *drawable,
		     int           state)
{
  switch (state)
    {
    case INIT_PAINT:
      dodgeburn_init (paint_core, drawable);
      break;
    case MOTION_PAINT:
      dodgeburn_motion (paint_core, drawable);
      break;
    case FINISH_PAINT:
      dodgeburn_finish (paint_core, drawable);
      break;
    }

  return NULL;
}

static void
dodgeburn_finish ( PaintCore *paint_core,
		 CanvasDrawable * drawable)
{
  /*d_printf("In dodgeburn_finish\n");*/
}

static void
dodgeburn_init ( PaintCore *paint_core,
		 CanvasDrawable * drawable)
{
  /*d_printf("In dodgeburn_init\n");*/
}

Tool *
tools_new_dodgeburn ()
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! dodgeburn_options)
      dodgeburn_options = create_dodgeburn_options();

  tool = paint_core_new (DODGEBURN);

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) dodgeburn_paint_func;
  private->painthit_setup = dodgeburn_painthit_setup;

  return tool;
}

void
tools_free_dodgeburn (Tool *tool)
{
  paint_core_free (tool);
}

static void
dodgeburn_painthit_setup (PaintCore * paint_core,Canvas * painthit)
{
  Canvas * temp_canvas;
  Canvas * orig_canvas;
  PixelArea srcPR, destPR, tempPR;
  gfloat exposure;
  gfloat brush_opacity;
  Tag tag;
  gint x1, y1, x2, y2;

  if (!painthit)
    return;

  if (paint_core->setup_mode == NORMAL_SETUP)
    {
  tag = drawable_tag(paint_core->drawable);
      x1 = BOUNDS (paint_core->x, 0, drawable_width (paint_core->drawable));
      y1 = BOUNDS (paint_core->y, 0, drawable_height (paint_core->drawable));
      x2 = BOUNDS (paint_core->x + paint_core->w, 0, drawable_width (paint_core->drawable));
      y2 = BOUNDS (paint_core->y + paint_core->h, 0, drawable_height (paint_core->drawable));

      if (!(x2 - x1) || !(y2 - y1))
	return;

      /*  get the original untouched image  */

#if 1 
      orig_canvas = paint_core_16_area_original (paint_core, 
	  paint_core->drawable, x1, y1, x2, y2);

      pixelarea_init (&srcPR, orig_canvas, 
	  0, 0, 
	  x2-x1, y2-y1, 
	  FALSE);
#endif

#if 0 
      pixelarea_init (&srcPR, drawable_data (paint_core->drawable), 
	  x1, y1, 
	  x2, y2,
	  FALSE);
#endif
      /* Get a temp buffer to hold the result of dodgeburning the orig */
      temp_canvas = canvas_new(tag, paint_core->w, paint_core->h, STORAGE_FLAT); 
      pixelarea_init (&tempPR, temp_canvas, 
	  0, 0, 
	  canvas_width (temp_canvas), canvas_height(temp_canvas),
	  TRUE);

      brush_opacity = gimp_brush_get_opacity();
      exposure = (dodgeburn_options->exposure)/100.0;

#if 0
      d_printf("dodgeburn_motion: brush_opacity is %f\n", brush_opacity);
      d_printf("dodgeburn_motion: exposure is %f\n", exposure);
#endif

      dodgeburn_area (&srcPR, &tempPR,
	  dodgeburn_options->type,dodgeburn_options->mode,exposure);

      pixelarea_init (&tempPR, temp_canvas, 
	  0, 0, 
	  canvas_width (temp_canvas), canvas_height(temp_canvas),
	  FALSE);

      pixelarea_init (&destPR, painthit, 
	  0, 0, 
	  paint_core->w, paint_core->h, 
	  TRUE);  

      /*if (!drawable_has_alpha (paint_core->drawable))
	add_alpha_area (&tempPR, &destPR);
	else*/
      copy_area(&tempPR, &destPR);

      canvas_delete(temp_canvas);
    }
  else
    if (paint_core->setup_mode == LINKED_SETUP)
      {
  tag = drawable_tag(paint_core->linked_drawable);
	x1 = BOUNDS (paint_core->x, 0, drawable_width (paint_core->drawable));
	y1 = BOUNDS (paint_core->y, 0, drawable_height (paint_core->drawable));
	x2 = BOUNDS (paint_core->x + paint_core->w, 0, drawable_width (paint_core->drawable));
	y2 = BOUNDS (paint_core->y + paint_core->h, 0, drawable_height (paint_core->drawable));

	if (!(x2 - x1) || !(y2 - y1))
	  return;

	/*  get the original untouched image  */

#if 1 
	orig_canvas = paint_core_16_area_original (paint_core, 
	    paint_core->linked_drawable, x1, y1, x2, y2);

	pixelarea_init (&srcPR, orig_canvas, 
	    0, 0, 
	    x2-x1, y2-y1, 
	    FALSE);
#endif

#if 0 
	pixelarea_init (&srcPR, drawable_data (paint_core->drawable), 
	    x1, y1, 
	    x2, y2,
	    FALSE);
#endif
	/* Get a temp buffer to hold the result of dodgeburning the orig */
	temp_canvas = canvas_new(tag, paint_core->w, paint_core->h, STORAGE_FLAT); 
	pixelarea_init (&tempPR, temp_canvas, 
	    0, 0, 
	    canvas_width (temp_canvas), canvas_height(temp_canvas),
	    TRUE);

	brush_opacity = gimp_brush_get_opacity();
	exposure = (dodgeburn_options->exposure)/100.0;

#if 0
	d_printf("dodgeburn_motion: brush_opacity is %f\n", brush_opacity);
	d_printf("dodgeburn_motion: exposure is %f\n", exposure);
#endif

	dodgeburn_area (&srcPR, &tempPR,
	    dodgeburn_options->type,dodgeburn_options->mode,exposure);

	pixelarea_init (&tempPR, temp_canvas, 
	    0, 0, 
	    canvas_width (temp_canvas), canvas_height(temp_canvas),
	    FALSE);

	pixelarea_init (&destPR, painthit, 
	    0, 0, 
	    paint_core->w, paint_core->h, 
	    TRUE);  

	/*if (!drawable_has_alpha (paint_core->drawable))
	  add_alpha_area (&tempPR, &destPR);
	  else*/
	copy_area(&tempPR, &destPR);

	canvas_delete(temp_canvas);

      }
}

void
dodgeburn_area (
    PixelArea * src_area,
    PixelArea * dest_area,
    gint type,
    gint mode,
    gfloat exposure
	       )
{
  DodgeburnRowFunc dodgeburn_row = dodgeburn_row_func (pixelarea_tag (src_area), mode);
  void * pag;

  if (type == BURN)
    exposure = -exposure; 

  for (pag = pixelarea_register (2, src_area, dest_area);
      pag != NULL;
      pag = pixelarea_process (pag))
    {
      PixelRow src_row;
      PixelRow dest_row;
      gint h = pixelarea_height (src_area);
      while (h--)
        {
          pixelarea_getdata (src_area, &src_row, h);
          pixelarea_getdata (dest_area, &dest_row, h);
          (*dodgeburn_row) (&src_row, &dest_row, type, exposure);
        }
    }
}

DodgeburnRowFunc
dodgeburn_row_func (
                   Tag tag,
                   gint mode
                   )
{
  switch (tag_precision (tag))
    {
        case PRECISION_U8:
            if (mode == DODGEBURN_HIGHLIGHTS)
               return dodge_highlights_row_u8; 
            else if (mode == DODGEBURN_MIDTONES)
               return dodge_midtones_row_u8; 
            else 
               return dodge_shadows_row_u8; 
        case PRECISION_U16:
            if (mode == DODGEBURN_HIGHLIGHTS)
               return dodge_highlights_row_u16; 
            else if (mode == DODGEBURN_MIDTONES)
               return dodge_midtones_row_u16; 
            else 
               return dodge_shadows_row_u16; 
        case PRECISION_FLOAT:
            if (mode == DODGEBURN_HIGHLIGHTS)
               return dodge_highlights_row_float; 
            else if (mode == DODGEBURN_MIDTONES)
               return dodge_midtones_row_float; 
            else 
               return dodge_shadows_row_float; 
        case PRECISION_FLOAT16:
            if (mode == DODGEBURN_HIGHLIGHTS)
               return dodge_highlights_row_float16; 
            else if (mode == DODGEBURN_MIDTONES)
               return dodge_midtones_row_float16; 
            else 
               return dodge_shadows_row_float16; 
        case PRECISION_BFP:
            if (mode == DODGEBURN_HIGHLIGHTS)
               return dodge_highlights_row_bfp; 
            else if (mode == DODGEBURN_MIDTONES)
               return dodge_midtones_row_bfp; 
            else 
               return dodge_shadows_row_bfp; 
        case PRECISION_NONE:
        default:
          g_warning ("dodgeburn_row_func: bad precision");
    }
  return NULL;
}

static void
dodgeburn_motion (PaintCore *paint_core,
		 CanvasDrawable *drawable)
{
  GImage *gimage;
  Tag tag = drawable_tag (drawable);
 
  if (! (gimage = drawable_gimage (drawable)))
    return;
  if (tag_format (tag) == FORMAT_INDEXED )
   return; 

  /* Set up the painthit with the right thing*/ 
  paint_core_16_area_setup (paint_core, drawable);                                            
  /* Apply the painthit to the drawable */                                                    
  paint_core_16_area_replace (paint_core, drawable, 
	current_device ? paint_core->curpressure : 1.0, 
	gimp_brush_get_opacity(), SOFT, CONSTANT, NORMAL_MODE);
}

static void *
dodgeburn_non_gui_paint_func (PaintCore *paint_core,
			     CanvasDrawable *drawable,
			     int        state)
{
  dodgeburn_motion (paint_core, drawable);

  return NULL;
}

gboolean
dodgeburn_non_gui (CanvasDrawable *drawable,
    		  double        pressure,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = (PaintFunc16) dodgeburn_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	dodgeburn_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup (&non_gui_paint_core);
      return TRUE;
    }
  else
    return FALSE;
}


// added by IMAGEWORKS (thedoug Aug 2002)
////////////////////////////////////////////////////////////////////////////////
// float
//

void dodge_highlights_row_float (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  gfloat *src          = (gfloat*) pixelrow_data (src_row);
  gfloat *dest         = (gfloat*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  d_printf("num_color_chans is %d\n", num_color_chans);
  d_printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
          dest[b] = factor * src[b];
	}
	
      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_midtones_row_float (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  gfloat *src          = (gfloat*) pixelrow_data (src_row);
  gfloat *dest         = (gfloat*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);
 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  if (src[b] < 1)
	    dest[b] = pow (src[b],factor);
	  else
	    dest[b] = src[b];
	}

      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_shadows_row_float (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  gfloat *src          = (gfloat*) pixelrow_data (src_row);
  gfloat *dest         = (gfloat*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    if (src[b] < 1)
	      dest[b] = factor + src[b] - factor*src[b];
 	    else
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
 	    if (src[b] < factor)
	      dest[b] = 0;
	    else if (src[b] <1)
	      dest[b] = (src[b] - factor)/(1-factor); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
}



////////////////////////////////////////////////////////////////////////////////
// float16
//

void dodge_highlights_row_float16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat  sb;
  ShortsFloat u;
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  d_printf("num_color_chans is %d\n", num_color_chans);
  d_printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
          sb = FLT(src[b], u);
          dest[b] = FLT16 (factor * sb, u);
	}
	
      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_midtones_row_float16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat  sb;
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);
 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
          sb = FLT(src[b], u);
	  if (sb < 1)
	    dest[b] = FLT16 (pow (sb,factor), u);
	  else
	    dest[b] = src[b];
	}

      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_shadows_row_float16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat  sb;
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    sb = FLT( src[b], u);
	    if (sb < 1)
	      dest[b] = FLT16 (factor + sb - factor*sb, u);
 	    else
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    sb = FLT( src[b], u);
 	    if (sb < factor)
	      dest[b] = FLT16(0,u);
	    else if (sb <1)
	      dest[b] = FLT16((sb - factor)/(1-factor), u); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
}

///////////////////////////////////////////////////////////////////////////
// 8 bit dodge and burn code
//
void dodge_highlights_row_u8 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint8 *src          = (guint8*) pixelrow_data (src_row);
  guint8 *dest         = (guint8*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  d_printf("num_color_chans is %d\n", num_color_chans);
  d_printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(factor * src[b], 0, 255);
	}
	
      if(has_alpha) {
          dest[b] = src[b];
      }

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_midtones_row_u8 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint8 *src          = (guint8*) pixelrow_data (src_row);
  guint8 *dest         = (guint8*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0) 
    factor = 1.0 - exposure * (.333333);
  else 
    factor = 1/(1.0 + exposure);

 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(255 * pow(src[b]/255., factor), 0, 255);
	}

      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_shadows_row_u8 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint8 *src          = (guint8*) pixelrow_data (src_row);
  guint8 *dest         = (guint8*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    // added by IMAGEWORKS thedoug (01/02)
	    dest[b] = CLAMP(255 * (factor + src[b]/255. - factor*src[b]/255.), 0, 255);
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
 	    if (src[b]/255. < factor)
	      dest[b] = 0;
	    else if (src[b]/255. < 1)
	      dest[b] = CLAMP(255 * (src[b]/255. - factor)/(1-factor), 0, 255); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
}

///////////////////////////////////////////////////////////
// 16 bit dodge and burn code
//
void dodge_highlights_row_u16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint32  s, t;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  d_printf("num_color_chans is %d\n", num_color_chans);
  d_printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(factor * src[b], 0, 65535);
	}
	
      if(has_alpha){
          dest[b] = src[b];
      }

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_midtones_row_u16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);
 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(65535 * pow(src[b]/65535., factor), 0, 65535);
	}

      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_shadows_row_u16 (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint32  s, t;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    // added by IMAGEWORKS thedoug (01/02)
	    dest[b] = CLAMP(65535 * (factor + src[b]/65535. - factor*src[b]/65535.), 0, 65535);
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
 	    if (src[b]/65535. < factor)
	      dest[b] = 0;
	    else if (src[b]/65535. < 1)
	      dest[b] = CLAMP(65535 * (src[b]/65535. - factor)/(1-factor), 0, 65535); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
}


///////////////////////////////////////////////////////////
// bfp dodge and burn code
//
void dodge_highlights_row_bfp (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint32  s, t;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor = 1.0 + exposure * (.333333);
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

#if 0
  d_printf("num_color_chans is %d\n", num_color_chans);
  d_printf("num_chans is %d\n", num_channels);
#endif
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(factor * src[b], 0, 65535);
	}
	
      if(has_alpha){
          dest[b] = src[b];
      }

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_midtones_row_bfp (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  ShortsFloat u;
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure < 0)
    factor = 1.0 - exposure * (.333333);
  else
    factor = 1/(1.0 + exposure);
 
  while (width--)
    {
      for (b = 0; b < num_color_chans; b++)
	{
	  // added by IMAGEWORKS thedoug (01/02)
          dest[b] = CLAMP(65535 * pow(src[b]/65535., factor), 0, 65535);
	}

      if(has_alpha)
          dest[b] = src[b];

      src += num_channels;
      dest += num_channels1;
    }
}

void dodge_shadows_row_bfp (
			    PixelRow *src_row, 
			    PixelRow *dest_row, 
			    gint type, 
			    gfloat exposure
			   )
{
  gint    b;
  guint32  s, t;
  guint16 *src          = (guint16*) pixelrow_data (src_row);
  guint16 *dest         = (guint16*) pixelrow_data (dest_row);
  gint    num_channels  = tag_num_channels (pixelrow_tag (src_row));
  gint    num_channels1 = tag_num_channels (pixelrow_tag (dest_row));
  Tag     tag           = pixelrow_tag (dest_row);
  Tag     tag2          = pixelrow_tag (src_row);
  gint    width         = pixelrow_width (dest_row);  
  gfloat factor;
  gint    num_color_chans;
  gint    has_alpha = (tag_alpha(tag) == ALPHA_YES && tag_alpha(tag2) == ALPHA_YES) ? TRUE: FALSE;

  if (has_alpha)
    num_color_chans = num_channels-1;
  else
    num_color_chans = num_channels;

  if (exposure >= 0)
  {
    factor = .333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
	    // added by IMAGEWORKS thedoug (01/02)
	    dest[b] = CLAMP(65535 * (factor + src[b]/65535. - factor*src[b]/65535.), 0, 65535);
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
  else   /*exposure < 0*/ 
  {
    factor = -.333333 * exposure;

    while (width--)
      {
	for (b = 0; b < num_color_chans; b++)
	  {
 	    if (src[b]/65535. < factor)
	      dest[b] = 0;
	    else if (src[b]/65535. < 1)
	      dest[b] = CLAMP(65535 * (src[b]/65535. - factor)/(1-factor), 0, 65535); 
	    else 
	      dest[b] = src[b];
	  }

	if(has_alpha)
	    dest[b] = src[b];

	src += num_channels;
	dest += num_channels1;
      }
  }
}


