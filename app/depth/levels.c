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
#include "../buildmenu.h"
#include "../ops_buttons.h"
#include "../canvas.h"
#include "../colormaps.h"
#include "../drawable.h"
#include "float16.h"
#include "bfp.h"
#include "../general.h"
#include "../gdisplay.h"
#include "../histogram.h"
#include "../image_map.h"
#include "../interface.h"
#include "../levels.h"
#include "../pixelarea.h"
#include "../pixelrow.h"
#include "../minimize.h"

#include "../buttons/picker_white.xpm"
#include "../buttons/picker_white_is.xpm"
#include "../buttons/picker_gray.xpm"
#include "../buttons/picker_gray_is.xpm"
#include "../buttons/picker_black.xpm"
#include "../buttons/picker_black_is.xpm"
#include "../buttons/picker_white"
#include "../buttons/picker_gray"
#include "../buttons/picker_black"
#include "../buttons/picker_mask"

#define LOW_INPUT          0x1
#define GAMMA              0x2
#define HIGH_INPUT         0x4
#define LOW_OUTPUT         0x8
#define HIGH_OUTPUT        0x10
#define INPUT_LEVELS       0x20
#define OUTPUT_LEVELS      0x40
#define INPUT_SLIDERS      0x80
#define OUTPUT_SLIDERS     0x100
#define DRAW               0x200
#define ALL                0xFFF

#define TEXT_WIDTH       50
#define DA_WIDTH         256
#define DA_HEIGHT        25
#define GRADIENT_HEIGHT  15
#define CONTROL_HEIGHT   DA_HEIGHT - GRADIENT_HEIGHT
#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150

#define LEVELS_DA_MASK  GDK_EXPOSURE_MASK | \
                        GDK_ENTER_NOTIFY_MASK | \
			GDK_BUTTON_PRESS_MASK | \
			GDK_BUTTON_RELEASE_MASK | \
			GDK_BUTTON1_MOTION_MASK | \
			GDK_POINTER_MOTION_HINT_MASK



//#define DEBUG
typedef struct Levels Levels;

struct Levels
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct LevelsDialog LevelsDialog;

typedef enum PICKER {
  WHITE_PICKER,
  GRAY_PICKER,
  BLACK_PICKER
} PICKER;

struct LevelsDialog
{
  GtkWidget *    shell;
  GtkWidget *    low_input_text;
  GtkWidget *    gamma_text;
  GtkWidget *    high_input_text;
  GtkWidget *    low_output_text;
  GtkWidget *    high_output_text;
  GtkWidget *    input_levels_da[2];
  GtkWidget *    output_levels_da[2];
  GtkWidget *    channel_menu;
  Histogram *    histogram;

  CanvasDrawable * drawable;
  ImageMap     image_map;
  int            color;
  int            channel;
  gint           preview;
 
  PixelRow       high_input_pr;  /* length 5 */
  gfloat         gamma[5];
  PixelRow 	 low_input_pr;   /* length 5 */
  PixelRow       low_output_pr;  /* length 5 */
  PixelRow 	 high_output_pr; /* length 5 */

  cmsHTRANSFORM  xform;
  cmsHTRANSFORM  bform;

  int            active_slider;
  PICKER         active_picker;
  int            slider_pos[5];      /*  positions for the five sliders  */
  guchar         input_ui[5][256];   /*  input transfer lut for ui */
};

/* transfer luts for input and output */
PixelRow input[5];
PixelRow output[5];


/*  levels action functions  */

static void   levels_button_press   (Tool *, GdkEventButton *, gpointer);
static void   levels_button_release (Tool *, GdkEventButton *, gpointer);
static void   levels_motion         (Tool *, GdkEventMotion *, gpointer);
static void   levels_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   levels_control        (Tool *, int, gpointer);

static LevelsDialog *  levels_new_dialog              (int bins);
static void            levels_update                  (LevelsDialog *, int);
static void            levels_preview                 (LevelsDialog *);
static void            levels_value_callback          (GtkWidget *, gpointer);
static void            levels_red_callback            (GtkWidget *, gpointer);
static void            levels_green_callback          (GtkWidget *, gpointer);
static void            levels_blue_callback           (GtkWidget *, gpointer);
static void            levels_alpha_callback          (GtkWidget *, gpointer);
static void            levels_picker_white_callback   (GtkWidget *, gpointer);
static void            levels_picker_gray_callback    (GtkWidget *, gpointer);
static void            levels_picker_black_callback   (GtkWidget *, gpointer);
static void            levels_reset_callback          (GtkWidget *, gpointer);
static void            levels_auto_levels_callback    (GtkWidget *, gpointer);
static void            levels_ok_callback             (GtkWidget *, gpointer);
static void            levels_cancel_callback         (GtkWidget *, gpointer);
static gint            levels_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void            levels_preview_update          (GtkWidget *, gpointer);
static void            levels_low_input_text_update   (GtkWidget *, gpointer);
static void            levels_gamma_text_update       (GtkWidget *, gpointer);
static void            levels_high_input_text_update  (GtkWidget *, gpointer);
static void            levels_low_output_text_update  (GtkWidget *, gpointer);
static void            levels_high_output_text_update (GtkWidget *, gpointer);
static gint            levels_input_da_events         (GtkWidget *, GdkEvent *, LevelsDialog *);
static gint            levels_output_da_events        (GtkWidget *, GdkEvent *, LevelsDialog *);
static void            levels_free_transfers          (LevelsDialog *);
static void            levels_calculate_transfers_ui  (LevelsDialog *);
static void            levels_destroy                 (GtkWidget *, gpointer);

static gfloat compute_input(gfloat val, gfloat low, gfloat high, gfloat gamma, gint clip);
static gfloat compute_output(gfloat val, gfloat low, gfloat high, gint clip);

static void *levels_options = NULL;
static LevelsDialog *levels_dialog = NULL;

static Argument * levels_invoker (Argument *);

static void  levels_funcs (Tag);

#define LOW_INPUT_STRING   0 
#define HIGH_INPUT_STRING  1
#define LOW_OUTPUT_STRING  2
#define HIGH_OUTPUT_STRING 3

static gchar *ui_strings[4];

typedef void (*LevelsFunc)(PixelArea *, PixelArea *, void *);
typedef void (*LevelsCalculateTransfersFunc)(LevelsDialog *);
typedef void (*LevelsInputDaSetuiValuesFunc)(LevelsDialog *, gint);
typedef void (*LevelsOutputDaSetuiValuesFunc)(LevelsDialog *, gint);
typedef gint (*LevelsHighOutputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsLowOutputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsHighInputTextCheckFunc)(LevelsDialog *, char *);
typedef gint (*LevelsLowInputTextCheckFunc)(LevelsDialog *, char *);
typedef void (*LevelsAllocTransfersFunc)(LevelsDialog *);
typedef void (*LevelsBuildInputDaTransferFunc)(LevelsDialog *, guchar *);
typedef void (*LevelsUpdateLowInputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateHighInputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateLowOutputSetTextFunc)(LevelsDialog *, char *);
typedef void (*LevelsUpdateHighOutputSetTextFunc)(LevelsDialog *, char *);
typedef gfloat (*LevelsGetLowInputFunc)(LevelsDialog *);
typedef gfloat (*LevelsGetHighInputFunc)(LevelsDialog *);
typedef gfloat (*LevelsGetLowOutputFunc)(LevelsDialog *);
typedef gfloat (*LevelsGetHighOutputFunc)(LevelsDialog *);
typedef void (*LevelsInitHighLowInputOutputFunc)(LevelsDialog *);

static LevelsFunc levels;
static LevelsCalculateTransfersFunc levels_calculate_transfers;
static LevelsInputDaSetuiValuesFunc levels_input_da_setui_values;
static LevelsOutputDaSetuiValuesFunc levels_output_da_setui_values;
static LevelsHighOutputTextCheckFunc levels_high_output_text_check;
static LevelsLowOutputTextCheckFunc levels_low_output_text_check;
static LevelsHighInputTextCheckFunc levels_high_input_text_check;
static LevelsLowInputTextCheckFunc levels_low_input_text_check;
static LevelsAllocTransfersFunc levels_alloc_transfers;
static LevelsBuildInputDaTransferFunc levels_build_input_da_transfer;
static LevelsUpdateLowInputSetTextFunc levels_update_low_input_set_text;
static LevelsUpdateHighInputSetTextFunc levels_update_high_input_set_text;
static LevelsUpdateLowOutputSetTextFunc levels_update_low_output_set_text;
static LevelsUpdateHighOutputSetTextFunc levels_update_high_output_set_text;
static LevelsGetLowInputFunc levels_get_low_input;
static LevelsGetHighInputFunc levels_get_high_input;
static LevelsGetLowOutputFunc levels_get_low_output;
static LevelsGetHighOutputFunc levels_get_high_output;
static LevelsInitHighLowInputOutputFunc levels_init_high_low_input_output;

static gchar ui_strings_u8[4][8] = {"0","255","0","255"};

static void       levels_u8 (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_u8 (LevelsDialog *ld);
static void       levels_input_da_setui_values_u8(LevelsDialog *, gint);
static void       levels_output_da_setui_values_u8(LevelsDialog *, gint);
static gint       levels_high_output_text_check_u8(LevelsDialog *, char *);
static gint       levels_low_output_text_check_u8(LevelsDialog *, char *);
static gint       levels_high_input_text_check_u8(LevelsDialog *, char *);
static gint       levels_low_input_text_check_u8 (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_u8 (LevelsDialog *);
static void       levels_build_input_da_transfer_u8(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_u8 ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_u8 ( LevelsDialog *, char *);
static gfloat     levels_get_low_input_u8 ( LevelsDialog *);
static gfloat     levels_get_high_input_u8 ( LevelsDialog *); 
static gfloat     levels_get_low_output_u8 ( LevelsDialog *);
static gfloat     levels_get_high_output_u8 ( LevelsDialog *);
static void       levels_init_high_low_input_output_u8 (LevelsDialog *ld);

static gchar ui_strings_u16[4][8] = {"0","65535","0","65535"};

static void       levels_u16 (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_u16 (LevelsDialog *ld);
static void       levels_input_da_setui_values_u16(LevelsDialog *, gint);
static void       levels_output_da_setui_values_u16(LevelsDialog *, gint);
static gint       levels_high_output_text_check_u16(LevelsDialog *, char *);
static gint       levels_low_output_text_check_u16(LevelsDialog *, char *);
static gint       levels_high_input_text_check_u16(LevelsDialog *, char *);
static gint       levels_low_input_text_check_u16 (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_u16 (LevelsDialog *);
static void       levels_build_input_da_transfer_u16(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_u16 ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_u16 ( LevelsDialog *, char *);
static gfloat     levels_get_low_input_u16 ( LevelsDialog *);
static gfloat     levels_get_high_input_u16 ( LevelsDialog *); 
static gfloat     levels_get_low_output_u16 ( LevelsDialog *);
static gfloat     levels_get_high_output_u16 ( LevelsDialog *);
static void       levels_init_high_low_input_output_u16 (LevelsDialog *ld);

static gchar ui_strings_float[4][8] = {"0.0","1.0","0.0","1.0"};

static void       levels_float (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_float (LevelsDialog *ld);
static void       levels_input_da_setui_values_float(LevelsDialog *, gint);
static void       levels_output_da_setui_values_float(LevelsDialog *, gint);
static gint       levels_high_output_text_check_float(LevelsDialog *, char *);
static gint       levels_low_output_text_check_float(LevelsDialog *, char *);
static gint       levels_high_input_text_check_float(LevelsDialog *, char *);
static gint       levels_low_input_text_check_float (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_float (LevelsDialog *);
static void       levels_build_input_da_transfer_float(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_float ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_float ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_float ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_float ( LevelsDialog *, char *);
static gfloat     levels_get_low_input_float ( LevelsDialog *);
static gfloat     levels_get_high_input_float ( LevelsDialog *); 
static gfloat     levels_get_low_output_float ( LevelsDialog *);
static gfloat     levels_get_high_output_float ( LevelsDialog *);
static void       levels_init_high_low_input_output_float (LevelsDialog *ld);

static gchar ui_strings_float16[4][8] = {"0.0","1.0","0.0","1.0"};

static void       levels_float16 (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_float16 (LevelsDialog *ld);
static void       levels_input_da_setui_values_float16(LevelsDialog *, gint);
static void       levels_output_da_setui_values_float16(LevelsDialog *, gint);
static gint       levels_high_output_text_check_float16(LevelsDialog *, char *);
static gint       levels_low_output_text_check_float16(LevelsDialog *, char *);
static gint       levels_high_input_text_check_float16(LevelsDialog *, char *);
static gint       levels_low_input_text_check_float16 (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_float16 (LevelsDialog *);
static void       levels_build_input_da_transfer_float16(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_float16 ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_float16 ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_float16 ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_float16 ( LevelsDialog *, char *);
static gfloat     levels_get_low_input_float16 ( LevelsDialog *);
static gfloat     levels_get_high_input_float16 ( LevelsDialog *); 
static gfloat     levels_get_low_output_float16 ( LevelsDialog *);
static gfloat     levels_get_high_output_float16 ( LevelsDialog *);
static void       levels_init_high_low_input_output_float16 (LevelsDialog *ld);


static gchar ui_strings_bfp[4][8] = {"0.0","1.9999","0.0","1.9999"};

static void       levels_bfp (PixelArea *, PixelArea *, void *);
static void       levels_calculate_transfers_bfp (LevelsDialog *ld);
static void       levels_input_da_setui_values_bfp(LevelsDialog *, gint);
static void       levels_output_da_setui_values_bfp(LevelsDialog *, gint);
static gint       levels_high_output_text_check_bfp(LevelsDialog *, char *);
static gint       levels_low_output_text_check_bfp(LevelsDialog *, char *);
static gint       levels_high_input_text_check_bfp(LevelsDialog *, char *);
static gint       levels_low_input_text_check_bfp (LevelsDialog *, char *);
static void 	  levels_alloc_transfers_bfp (LevelsDialog *);
static void       levels_build_input_da_transfer_bfp(LevelsDialog *, guchar *);
static void       levels_update_low_input_set_text_bfp ( LevelsDialog *, char *);
static void       levels_update_high_input_set_text_bfp ( LevelsDialog *, char *);
static void       levels_update_low_output_set_text_bfp ( LevelsDialog *, char *);
static void       levels_update_high_output_set_text_bfp ( LevelsDialog *, char *);
static gfloat     levels_get_low_input_bfp ( LevelsDialog *);
static gfloat     levels_get_high_input_bfp ( LevelsDialog *); 
static gfloat     levels_get_low_output_bfp ( LevelsDialog *);
static gfloat     levels_get_high_output_bfp ( LevelsDialog *);
static void       levels_init_high_low_input_output_bfp (LevelsDialog *ld);

void  interpolate_mid_between_old_new (gfloat red_in, gfloat green_in, gfloat blue_in, gfloat *red_out, gfloat *green_out, gfloat *blue_out);

/*#define DO_FLOAT_CLIPPING */
/*
    Interpolate the middle value between the high/low values.  (Strictly linear -- No gamma or color-weight correction.)
*/
void
interpolate_mid_between_old_new (gfloat red_in, gfloat green_in, gfloat blue_in, gfloat *red_out, gfloat *green_out, gfloat *blue_out)
{
//   Interpolate new mid between new low/high (Note: Invalid if high-low = 0.)
#define levels_interpolate_mid_between_old_new_interp( low, mid, high, newlow, newhigh ) \
    (newlow + (mid - low) * (newhigh - newlow) / (high - low))

//     Note: Mid interpolation can't be overlarge, so long as high-low != 0
    if ((red_in < green_in) && (green_in < blue_in))  
            *green_out = levels_interpolate_mid_between_old_new_interp(red_in, green_in, blue_in, *red_out, *blue_out); 
    else if ((blue_in < green_in) && (green_in < red_in)) 
            *green_out = levels_interpolate_mid_between_old_new_interp(blue_in, green_in, red_in, *blue_out, *red_out); 
    else if ((green_in < blue_in) && (blue_in < red_in)) 
            *blue_out = levels_interpolate_mid_between_old_new_interp(green_in, blue_in, red_in, *green_out, *red_out); 
    else if ((red_in < blue_in) && (blue_in < green_in)) 
            *blue_out = levels_interpolate_mid_between_old_new_interp(red_in, blue_in, green_in, *red_out, *green_out); 
    else if ((blue_in < red_in) && (red_in < green_in)) 
            *red_out = levels_interpolate_mid_between_old_new_interp(blue_in, red_in, green_in, *blue_out, *green_out); 
    else if ((green_in < red_in) && (red_in < blue_in)) 
            *red_out = levels_interpolate_mid_between_old_new_interp(green_in, red_in, blue_in, *green_out, *blue_out); 
}
/*
    Read red/green/blue values from curve_value array, and interpolate middle result value.

    Input symbols:
    type_tmp                     -- data type to cast results to.
    red_in, green_in, blue_in    -- input symbols for red/green/blue.
    red_out, green_out, blue_out -- output symbols for red/green/blue.
    redd, greend, blued          -- temp symbols for red/green/blue.
*/
#define levels_apply_level_and_adjust_mid( type_tmp, red_in, green_in, blue_in, red_out, green_out, blue_out, redd, greend, blued) \
    gfloat redd = (gfloat) output_val[input_val[red_in]]; \
    gfloat greend = (gfloat) output_val[input_val[green_in]]; \
    gfloat blued = (gfloat) output_val[input_val[blue_in]]; \
     \
    interpolate_mid_between_old_new( (gfloat) red_in, (gfloat) green_in, (gfloat) blue_in, &redd, &greend, &blued); \
    red_out = (type_tmp) redd; \
    green_out = (type_tmp) greend; \
    blue_out = (type_tmp) blued;

#define levels_apply_level_and_adjust_mid_float( red_in, green_in, blue_in, red_out, green_out, blue_out, redd, greend, blued, input_tmp) \
    gfloat input_tmp; \
    input_tmp = compute_input (red_in, \
                                low_input[HISTOGRAM_VALUE], \
                                high_input[HISTOGRAM_VALUE], \
                                ld->gamma[HISTOGRAM_VALUE], FALSE); \
    gfloat redd = (gfloat) compute_output (input_tmp, \
                                low_output[HISTOGRAM_VALUE], \
                                high_output[HISTOGRAM_VALUE], TRUE); \
    \
    input_tmp = compute_input (green_in, \
                                low_input[HISTOGRAM_VALUE], \
                                high_input[HISTOGRAM_VALUE], \
                                ld->gamma[HISTOGRAM_VALUE], FALSE); \
    gfloat greend = (gfloat) compute_output (input_tmp, \
                                low_output[HISTOGRAM_VALUE], \
                                high_output[HISTOGRAM_VALUE], TRUE); \
    \
    input_tmp = compute_input (blue_in, \
                                low_input[HISTOGRAM_VALUE], \
                                high_input[HISTOGRAM_VALUE], \
                                ld->gamma[HISTOGRAM_VALUE], FALSE); \
    gfloat blued = (gfloat) compute_output (input_tmp, \
                                low_output[HISTOGRAM_VALUE], \
                                high_output[HISTOGRAM_VALUE], TRUE); \
    \
    interpolate_mid_between_old_new( red_in, green_in, blue_in, &redd, &greend, &blued); \
    red_out = redd; \
    green_out = greend; \
    blue_out = blued;


static void
levels_u8 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld;
  guchar *src, *dest;
  guint8 *s, *d;
  int has_alpha, alpha;
  int w, h;
  guint8 *output_r, *output_g, *output_b, *output_val, *output_a = NULL;
  guint8 *input_r, *input_g, *input_b, *input_val, *input_a = NULL;

  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  output_r = (guint8*)pixelrow_data (&output[HISTOGRAM_RED]);
  output_g = (guint8*)pixelrow_data (&output[HISTOGRAM_GREEN]);
  output_b = (guint8*)pixelrow_data (&output[HISTOGRAM_BLUE]);
  output_val = (guint8*)pixelrow_data (&output[HISTOGRAM_VALUE]);
  if (has_alpha)
    output_a = (guint8*)pixelrow_data (&output[HISTOGRAM_ALPHA]);
  
  input_r = (guint8*)pixelrow_data (&input[HISTOGRAM_RED]);
  input_g = (guint8*)pixelrow_data (&input[HISTOGRAM_GREEN]);
  input_b = (guint8*)pixelrow_data (&input[HISTOGRAM_BLUE]);
  input_val = (guint8*)pixelrow_data (&input[HISTOGRAM_VALUE]);
  if (has_alpha)
    input_a = (guint8*)pixelrow_data (&input[HISTOGRAM_ALPHA]);
  
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = output_r[input_r[s[RED_PIX]]];
	      d[GREEN_PIX] = output_g[input_g[s[GREEN_PIX]]];
	      d[BLUE_PIX] = output_b[input_b[s[BLUE_PIX]]];

	      /*  The overall changes  */
                levels_apply_level_and_adjust_mid( guint8, d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], \
                    d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], redd, greend, blued)
	    }
	  else
	    d[GRAY_PIX] = output_val[input_val[s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = output_a[input_a[s[alpha]]];
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_init_high_low_input_output_u8 (LevelsDialog *ld)
{
  gint i;
    guint8 *low_input = (guint8*)pixelrow_data (&levels_dialog->low_input_pr);
    guint8 *high_input = (guint8*)pixelrow_data (&levels_dialog->high_input_pr);
    guint8 *low_output = (guint8*)pixelrow_data (&levels_dialog->low_output_pr);
    guint8 *high_output = (guint8*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 255;
	low_output[i] = 0;
	high_output[i] = 255;
      }
}

static void
levels_calculate_transfers_u8 (LevelsDialog *ld)
{
  int i, j;
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      guint8 *out = (guint8*)pixelrow_data (&output[j]);
      guint8 *in = (guint8*)pixelrow_data (&input[j]); 
      for (i = 0; i < 256; i++)
	{
	  in[i] = 255.0 * compute_input (i/255.0, low_input[j]/255.0, high_input[j]/255.0, ld->gamma[j], TRUE) + .5;
	  out[i] = 255.0 * compute_output (i/255.0, low_output[j]/255.0, high_output[j]/255.0, TRUE) + .5;
	}
    }
}

static void
levels_input_da_setui_values_u8 (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  gfloat width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((gfloat) x / (gfloat) DA_WIDTH) * 255.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);
      break;

    case 1:  /*  gamma  */
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (gfloat) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 255.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 255);
      break;
    }
}

static void
levels_output_da_setui_values_u8(
				LevelsDialog *ld,
				gint x	
				)
{
  gfloat tmp;
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 255.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 255);
      break;

    case 4:  /*  high output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 255.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 255);
      break;
    }
}

static gint 
levels_high_output_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 255);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* low_output = (guint8*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 255);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), low_input[ld->channel], 255);
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_u8 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint8* high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8* low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), 0, high_input[ld->channel]);
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_u8 (LevelsDialog *ld)
{
  gint i;
  guint8* data;
  Tag tag = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
  
  
  /* alloc the high and low input and output arrays*/
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (guint8*) g_malloc (sizeof(guint8) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);
	
  /* allocate the input and output transfer arrays */
  for (i = 0; i < 5; i++)
  { 
       data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&input[i], tag, (guchar*)data, 256);
  }
  
  for (i = 0; i < 5; i++)
  { 
       data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&output[i], tag, (guchar*)data, 256);
  }
}

static void
levels_build_input_da_transfer_u8 (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  guint8 *input_data = (guint8 *)pixelrow_data (&input[ld->channel]); 
  for(i = 0; i < DA_WIDTH; i++)
	buf[i] = input_data[i]; 
}

static void
levels_update_low_input_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%d", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%d", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%d", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_u8 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%d", high_output[ld->channel]);
}

static gfloat 
levels_get_low_input_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
 return (gfloat)low_input[ld->channel]/255.0;
}

static gfloat 
levels_get_high_input_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
 return (gfloat)high_input[ld->channel]/255.0;
}

static gfloat 
levels_get_low_output_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
 return (gfloat)low_output[ld->channel]/255.0;
}

static gfloat 
levels_get_high_output_u8 (
			 LevelsDialog *ld
			)
{
 guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);
 return (gfloat)high_output[ld->channel]/255.0;
}

static void
levels_float (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld = (LevelsDialog *) user_data;
  guchar *src, *dest;
  gfloat *s, *d;
  int has_alpha, alpha;
  int w, h;
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
  gfloat input;


  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
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
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      input = compute_input (s[RED_PIX], 
					low_input[HISTOGRAM_RED], 
					high_input[HISTOGRAM_RED], 
					ld->gamma[HISTOGRAM_RED], FALSE); 

	      d[RED_PIX] = compute_output (input, 
					low_output[HISTOGRAM_RED], 
					high_output[HISTOGRAM_RED], FALSE); 

	      input = compute_input (s[GREEN_PIX], 
					low_input[HISTOGRAM_GREEN], 
					high_input[HISTOGRAM_GREEN], 
					ld->gamma[HISTOGRAM_GREEN], FALSE); 

	      d[GREEN_PIX] = compute_output (input, 
					low_output[HISTOGRAM_GREEN], 
					high_output[HISTOGRAM_GREEN], FALSE); 

	      input = compute_input (s[BLUE_PIX], 
					low_input[HISTOGRAM_BLUE], 
					high_input[HISTOGRAM_BLUE], 
					ld->gamma[HISTOGRAM_BLUE], FALSE); 

	      d[BLUE_PIX] = compute_output (input, 
					low_output[HISTOGRAM_BLUE], 
					high_output[HISTOGRAM_BLUE], FALSE); 
		
		/* now do the value contribution */

#ifdef DO_FLOAT_CLIPPING
/*stub xyzxyz -- currently clips following last calculation. Apply to Gray/Alpha calc's too?  Front-clip instead??  Neither??? */
            levels_apply_level_and_adjust_mid_float( d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], redd, greend, blued, input)
#else
	      input = compute_input (d[RED_PIX], 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      d[RED_PIX] = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      input = compute_input (d[GREEN_PIX], 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      d[GREEN_PIX] = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      input = compute_input (d[BLUE_PIX], 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      d[BLUE_PIX] = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 
#endif
	    }
	  else
	    {
	      input = compute_input (s[GRAY_PIX], 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      d[GRAY_PIX] = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 
	    }

	  if (has_alpha)
	    {
	      input = compute_input (s[alpha], 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      d[alpha] = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	    }
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_init_high_low_input_output_float (LevelsDialog *ld)
{
  gint i;
    gfloat *low_input = (gfloat*)pixelrow_data (&levels_dialog->low_input_pr);
    gfloat *high_input = (gfloat*)pixelrow_data (&levels_dialog->high_input_pr);
    gfloat *low_output = (gfloat*)pixelrow_data (&levels_dialog->low_output_pr);
    gfloat *high_output = (gfloat*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 1.0;
	low_output[i] = 0;
	high_output[i] = 1.0;
      }
}

static void
levels_calculate_transfers_float (LevelsDialog *ld)
{
}

static void
levels_input_da_setui_values_float (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);

	/* round the input value to 6 places */
      sprintf (text, "%.6f", low_input[ld->channel]);
      low_input[ld->channel] = atof (text);
      break;

    case 1:  /*  gamma  */
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (gfloat) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 1.0);

	/* round the value to 6 places */
      sprintf (text, "%.6f", high_input[ld->channel]);
      high_input[ld->channel] = atof (text);
      break;
    }
}

static void
levels_output_da_setui_values_float(
				LevelsDialog *ld,
				gint x	
				)
{
  gfloat tmp;
  char text[12];
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 1.0);

	/* round the value to 6 places */
      sprintf (text, "%.6f", low_output[ld->channel]);
      low_output[ld->channel] = atof (text);
      break;

    case 4:  /*  high output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 1.0);

	/* round the value to 6 places */
      sprintf (text, "%.6f", high_output[ld->channel]);
      high_output[ld->channel] = atof (text);
      break;
    }
}

static gint 
levels_high_output_text_check_float (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS ( atof (str), 0, 1.0);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_float (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS ( atof (str), 0, 1.0);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_float (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  value = atof (str) > low_input[ld->channel] ? atof (str) : low_input[ld->channel];
  /*BOUNDS ( atof (str), low_input[ld->channel], 1.0);*/
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_float (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  value = atof (str) < high_input[ld->channel] ? atof (str) : high_input[ld->channel];
  /*BOUNDS ( atof (str), 0, high_input[ld->channel]);*/
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_float (LevelsDialog *ld)
{
  gint i;
  gfloat* data;
  Tag tag = tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO);
  
  /* alloc the high and low input and output arrays*/
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);

  for (i = 0; i < 5; i++)
  { 
     pixelrow_init (&output[i], tag_null(), NULL, 0);
     pixelrow_init (&input[i], tag_null(), NULL, 0);
  }
  
}

static void
levels_build_input_da_transfer_float (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  gint j = ld->channel;
  gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  for(i = 0; i < DA_WIDTH; i++)
    buf[i] = 255.0 * compute_input(i/255.0, low_input[j], high_input[j], ld->gamma[j], TRUE); 
}

static void
levels_update_low_input_set_text_float (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%f", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_float (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%f", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_float (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%f", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_float (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%f", high_output[ld->channel]);
}

static gfloat 
levels_get_low_input_float (
			 LevelsDialog *ld
			)
{
 gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
 return (gfloat)low_input[ld->channel];
}

static gfloat 
levels_get_high_input_float (
			 LevelsDialog *ld
			)
{
 gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
 return (gfloat)high_input[ld->channel];
}

static gfloat 
levels_get_low_output_float (
			 LevelsDialog *ld
			)
{
 gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
 return (gfloat)low_output[ld->channel];
}

static gfloat 
levels_get_high_output_float (
			 LevelsDialog *ld
			)
{
 gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
 return (gfloat)high_output[ld->channel];
}

static void
levels_float16 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld = (LevelsDialog *) user_data;
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha, alpha;
  int w, h;
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
  gfloat input, output, red, green, blue;
  ShortsFloat u;


  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
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
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */

	      input = compute_input (FLT(s[RED_PIX], u), 
					low_input[HISTOGRAM_RED], 
					high_input[HISTOGRAM_RED], 
					ld->gamma[HISTOGRAM_RED], FALSE); 

	      red = compute_output (input, 
					low_output[HISTOGRAM_RED], 
					high_output[HISTOGRAM_RED], FALSE); 

	      input = compute_input (FLT(s[GREEN_PIX], u), 
					low_input[HISTOGRAM_GREEN], 
					high_input[HISTOGRAM_GREEN], 
					ld->gamma[HISTOGRAM_GREEN], FALSE); 

	      green = compute_output (input, 
					low_output[HISTOGRAM_GREEN], 
					high_output[HISTOGRAM_GREEN], FALSE); 

	      input = compute_input (FLT(s[BLUE_PIX], u), 
					low_input[HISTOGRAM_BLUE], 
					high_input[HISTOGRAM_BLUE], 
					ld->gamma[HISTOGRAM_BLUE], FALSE); 

	      blue = compute_output (input, 
					low_output[HISTOGRAM_BLUE], 
					high_output[HISTOGRAM_BLUE], FALSE); 
		
		/* now do the value contribution */

	      input = compute_input (red, 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      output = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      d[RED_PIX] = FLT16( output, u);

	      input = compute_input (green, 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      output = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      d[GREEN_PIX] = FLT16( output, u);

	      input = compute_input (blue, 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      output = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      d[BLUE_PIX] = FLT16( output, u);
	    }
	  else
	    {
	      input = compute_input (FLT(s[GRAY_PIX], u), 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      output = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      d[GRAY_PIX] = FLT16( output, u);
	    }

	  if (has_alpha)
	    {
	      input = compute_input ( FLT(s[alpha], u), 
					low_input[HISTOGRAM_VALUE], 
					high_input[HISTOGRAM_VALUE], 
					ld->gamma[HISTOGRAM_VALUE], FALSE); 

	      output = compute_output (input, 
					low_output[HISTOGRAM_VALUE], 
					high_output[HISTOGRAM_VALUE], FALSE); 

	      d[alpha] = FLT16( output, u);
	    }
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_init_high_low_input_output_float16 (LevelsDialog *ld)
{
  gint i;
    gfloat *low_input = (gfloat*)pixelrow_data (&levels_dialog->low_input_pr);
    gfloat *high_input = (gfloat*)pixelrow_data (&levels_dialog->high_input_pr);
    gfloat *low_output = (gfloat*)pixelrow_data (&levels_dialog->low_output_pr);
    gfloat *high_output = (gfloat*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 1.0;
	low_output[i] = 0;
	high_output[i] = 1.0;
      }
}

static void
levels_calculate_transfers_float16 (LevelsDialog *ld)
{
}

static void
levels_input_da_setui_values_float16 (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);

	/* round the input value to 6 places */
      sprintf (text, "%.6f", low_input[ld->channel]);
      low_input[ld->channel] = atof (text);
      break;

    case 1:  /*  gamma  */
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (gfloat) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 1.0);

	/* round the value to 6 places */
      sprintf (text, "%.6f", high_input[ld->channel]);
      high_input[ld->channel] = atof (text);
      break;
    }
}

static void
levels_output_da_setui_values_float16(
				LevelsDialog *ld,
				gint x	
				)
{
  gfloat tmp;
  char text[12];
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 1.0);

	/* round the value to 6 places */
      sprintf (text, "%.6f", low_output[ld->channel]);
      low_output[ld->channel] = atof (text);
      break;

    case 4:  /*  high output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 1.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 1.0); 

	/* round the value to 6 places */
      sprintf (text, "%.6f", high_output[ld->channel]);
      high_output[ld->channel] = atof (text);
      break;
    }
}

static gint 
levels_high_output_text_check_float16 (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  value = atof (str) > low_output[ld->channel] ? atof (str) : low_output[ld->channel];
  /* BOUNDS ( atof (str), 0, 1.0);*/
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_float16 (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat* high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  value = atof (str) < high_output[ld->channel] ? atof (str) :
high_output[ld->channel];
  /* BOUNDS ( atof (str), 0, 1.0); */
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_float16 (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  value = atof (str) > low_input[ld->channel] ? atof (str) : low_input[ld->channel];
  /*BOUNDS ( atof (str), low_input[ld->channel], 1.0); */
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_float16 (
				LevelsDialog *ld,
				char *str 
				)
{
  gfloat value;
  gfloat* high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat* low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  value =  atof (str) < high_input[ld->channel] ? atof (str) : high_input[ld->channel];
  /*BOUNDS ( atof (str), 0, high_input[ld->channel]);*/ 
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_float16 (LevelsDialog *ld)
{
  gint i;
  gfloat* data;
  Tag tag = tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO);
  
  /* alloc the high and low input and output arrays*/
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (gfloat*) g_malloc (sizeof(gfloat) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);

  for (i = 0; i < 5; i++)
  { 
     pixelrow_init (&output[i], tag_null(), NULL, 0);
     pixelrow_init (&input[i], tag_null(), NULL, 0);
  }
  
}

static void
levels_build_input_da_transfer_float16 (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  gint j = ld->channel;
  gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  for(i = 0; i < DA_WIDTH; i++)
    buf[i] = 255.0 * compute_input(i/255.0, low_input[j], high_input[j], ld->gamma[j], TRUE); 
}

static void
levels_update_low_input_set_text_float16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%f", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_float16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%f", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_float16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%f", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_float16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%f", high_output[ld->channel]);
}

static gfloat 
levels_get_low_input_float16 (
			 LevelsDialog *ld
			)
{
 gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
 return (gfloat)low_input[ld->channel];
}

static gfloat 
levels_get_high_input_float16 (
			 LevelsDialog *ld
			)
{
 gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
 return (gfloat)high_input[ld->channel];
}

static gfloat 
levels_get_low_output_float16 (
			 LevelsDialog *ld
			)
{
 gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
 return (gfloat)low_output[ld->channel];
}

static gfloat 
levels_get_high_output_float16 (
			 LevelsDialog *ld
			)
{
 gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);
 return (gfloat)high_output[ld->channel];
}

static void
levels_u16 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld;
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha, alpha;
  int w, h;
  guint16 *output_r, *output_g, *output_b, *output_val, *output_a = NULL;
  guint16 *input_r, *input_g, *input_b, *input_val, *input_a = NULL;

  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  output_r = (guint16*)pixelrow_data (&output[HISTOGRAM_RED]);
  output_g = (guint16*)pixelrow_data (&output[HISTOGRAM_GREEN]);
  output_b = (guint16*)pixelrow_data (&output[HISTOGRAM_BLUE]);
  output_val = (guint16*)pixelrow_data (&output[HISTOGRAM_VALUE]);
  if (has_alpha)
    output_a = (guint16*)pixelrow_data (&output[HISTOGRAM_ALPHA]);
  
  input_r = (guint16*)pixelrow_data (&input[HISTOGRAM_RED]);
  input_g = (guint16*)pixelrow_data (&input[HISTOGRAM_GREEN]);
  input_b = (guint16*)pixelrow_data (&input[HISTOGRAM_BLUE]);
  input_val = (guint16*)pixelrow_data (&input[HISTOGRAM_VALUE]);
  if (has_alpha)
    input_a = (guint16*)pixelrow_data (&input[HISTOGRAM_ALPHA]);
  
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = output_r[input_r[s[RED_PIX]]];
	      d[GREEN_PIX] = output_g[input_g[s[GREEN_PIX]]];
	      d[BLUE_PIX] = output_b[input_b[s[BLUE_PIX]]];

	      /*  The overall changes  */
#if 0
	      d[RED_PIX] = output_val[input_val[d[RED_PIX]]];
	      d[GREEN_PIX] = output_val[input_val[d[GREEN_PIX]]];
	      d[BLUE_PIX] = output_val[input_val[d[BLUE_PIX]]];
#else
                levels_apply_level_and_adjust_mid( guint16, d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], \
                    d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], redd, greend, blued)
#endif
	    }
	  else
	    d[GRAY_PIX] = output_val[input_val[s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = output_a[input_a[s[alpha]]];
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_calculate_transfers_u16 (LevelsDialog *ld)
{
  gint i, j;
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      guint16 *out = (guint16*)pixelrow_data (&output[j]);
      guint16 *in = (guint16*)pixelrow_data (&input[j]); 
      for (i = 0; i < 65536; i++)
	{
	  in[i] = 65535.0 * compute_input (i/65535.0, low_input[j]/65535.0, high_input[j]/65535.0, ld->gamma[j], TRUE) + .5;
	  out[i] = 65535.0 * compute_output (i/65535.0, low_output[j]/65535.0, high_output[j]/65535.0, TRUE) + .5;
	}
    }
}

static void
levels_init_high_low_input_output_u16 (LevelsDialog *ld)
{
  gint i;
    guint16 *low_input = (guint16*)pixelrow_data (&levels_dialog->low_input_pr);
    guint16 *high_input = (guint16*)pixelrow_data (&levels_dialog->high_input_pr);
    guint16 *low_output = (guint16*)pixelrow_data (&levels_dialog->low_output_pr);
    guint16 *high_output = (guint16*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 65535;
	low_output[i] = 0;
	high_output[i] = 65535;
      }
}

static void
levels_input_da_setui_values_u16 (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  gfloat width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);
      break;

    case 1:  /*  gamma  */
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (gfloat) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 65535);
      break;
    }
}

static void
levels_output_da_setui_values_u16(
				LevelsDialog *ld,
				gint x	
				)
{
  gfloat tmp;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;

    case 4:  /*  high output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;
    }
}

static gint 
levels_high_output_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 65535);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS (((int) atof (str)), 0, 65535);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), low_input[ld->channel], 65535);
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_u16 (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (((int) atof (str)), 0, high_input[ld->channel]);
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_u16 (LevelsDialog *ld)
{
  gint i;
  guint16* data;
  Tag tag = tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO);
  
  
  /* alloc the high and low input and output arrays*/
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);
	
  /* allocate the input and output transfer arrays */
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&input[i], tag, (guchar*)data, 65536);
  }
  
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&output[i], tag, (guchar*)data, 65536);
  }
}

static void
levels_build_input_da_transfer_u16 (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  guint16 *input_data = (guint16 *)pixelrow_data (&input[ld->channel]); 
  for(i = 0; i < DA_WIDTH; i++)
	buf[i] = (guchar) (input_data[i * 257]/257.0); 
}

static void
levels_update_low_input_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%d", low_input[ld->channel]);
}

static void
levels_update_high_input_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%d", high_input[ld->channel]);
}

static void
levels_update_low_output_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%d", low_output[ld->channel]);
}

static void
levels_update_high_output_set_text_u16 (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%d", high_output[ld->channel]);
}

static gfloat 
levels_get_low_input_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 return (gfloat)low_input[ld->channel]/65535.0;
}

static gfloat 
levels_get_high_input_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 return (gfloat)high_input[ld->channel]/65535.0;
}

static gfloat 
levels_get_low_output_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 return (gfloat)low_output[ld->channel]/65535.0;
}

static gfloat 
levels_get_high_output_u16 (
			 LevelsDialog *ld
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 return (gfloat)high_output[ld->channel]/65535.0;
}


static void
levels_bfp (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  LevelsDialog *ld;
  guchar *src, *dest;
  guint16 *s, *d;
  int has_alpha, alpha;
  int w, h;
  guint16 *output_r, *output_g, *output_b, *output_val, *output_a = NULL;
  guint16 *input_r, *input_g, *input_b, *input_val, *input_a = NULL;

  ld = (LevelsDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);
  has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  alpha = has_alpha ? src_num_channels - 1 : src_num_channels;

  output_r = (guint16*)pixelrow_data (&output[HISTOGRAM_RED]);
  output_g = (guint16*)pixelrow_data (&output[HISTOGRAM_GREEN]);
  output_b = (guint16*)pixelrow_data (&output[HISTOGRAM_BLUE]);
  output_val = (guint16*)pixelrow_data (&output[HISTOGRAM_VALUE]);
  if (has_alpha)
    output_a = (guint16*)pixelrow_data (&output[HISTOGRAM_ALPHA]);
  
  input_r = (guint16*)pixelrow_data (&input[HISTOGRAM_RED]);
  input_g = (guint16*)pixelrow_data (&input[HISTOGRAM_GREEN]);
  input_b = (guint16*)pixelrow_data (&input[HISTOGRAM_BLUE]);
  input_val = (guint16*)pixelrow_data (&input[HISTOGRAM_VALUE]);
  if (has_alpha)
    input_a = (guint16*)pixelrow_data (&input[HISTOGRAM_ALPHA]);
  
  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = output_r[input_r[s[RED_PIX]]];
	      d[GREEN_PIX] = output_g[input_g[s[GREEN_PIX]]];
	      d[BLUE_PIX] = output_b[input_b[s[BLUE_PIX]]];

	      /*  The overall changes  */
                levels_apply_level_and_adjust_mid( guint16, d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], \
                    d[RED_PIX], d[GREEN_PIX], d[BLUE_PIX], redd, greend, blued)
	    }
	  else
	    d[GRAY_PIX] = output_val[input_val[s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = output_a[input_a[s[alpha]]];
	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
levels_calculate_transfers_bfp (LevelsDialog *ld)
{
  gint i, j;
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      guint16 *out = (guint16*)pixelrow_data (&output[j]);
      guint16 *in = (guint16*)pixelrow_data (&input[j]); 
      for (i = 0; i < 65536; i++)
	{
	  in[i] = 65535.0 * compute_input (i/65535.0, low_input[j]/65535.0, high_input[j]/65535.0, ld->gamma[j], TRUE) + .5;
	  out[i] = 65535.0 * compute_output (i/65535.0, low_output[j]/65535.0, high_output[j]/65535.0, TRUE) + .5;
	}
    }
}

static void
levels_init_high_low_input_output_bfp (LevelsDialog *ld)
{
  gint i;
    guint16 *low_input = (guint16*)pixelrow_data (&levels_dialog->low_input_pr);
    guint16 *high_input = (guint16*)pixelrow_data (&levels_dialog->high_input_pr);
    guint16 *low_output = (guint16*)pixelrow_data (&levels_dialog->low_output_pr);
    guint16 *high_output = (guint16*)pixelrow_data (&levels_dialog->high_output_pr);
    for (i = 0; i < 5; i++)
      {
	low_input[i] = 0;
	levels_dialog->gamma[i] = 1.0;
	high_input[i] = 65535;
	low_output[i] = 0;
	high_output[i] = 65535;
      }
}

static void
levels_input_da_setui_values_bfp (
				LevelsDialog *ld,
				gint x	
				)
{
  char text[12];
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  gfloat width, mid, tmp;

  switch (ld->active_slider)
    {
    case 0:  /*  low input  */
      tmp =  ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      low_input[ld->channel] = BOUNDS (tmp, 0, high_input[ld->channel]);
      break;

    case 1:  /*  gamma  */
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;

      x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
      tmp = (gfloat) (x - mid) / width;
      ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      ld->gamma[ld->channel] = atof (text);
      break;

    case 2:  /*  high input  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      high_input[ld->channel] = BOUNDS (tmp, low_input[ld->channel], 65535);
      break;
    }
}

static void
levels_output_da_setui_values_bfp (
				LevelsDialog *ld,
				gint x	
				)
{
  gfloat tmp;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  switch (ld->active_slider)
    {
    case 3:  /*  low output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      low_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;

    case 4:  /*  high output  */
      tmp = ((gfloat) x / (gfloat) DA_WIDTH) * 65535.0;
      high_output[ld->channel] = BOUNDS (tmp, 0, 65535);
      break;
    }
}

static gint 
levels_high_output_text_check_bfp (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  value = BOUNDS (FLOAT_TO_BFP(atof (str)), 0, 65535);
  if (value != high_output[ld->channel])
  {
      high_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_output_text_check_bfp (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* low_output = (guint16*)pixelrow_data (&ld->low_output_pr);

  value = BOUNDS (FLOAT_TO_BFP(atof (str)), 0, 65535);
  if (value != low_output[ld->channel])
  {
      low_output[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_high_input_text_check_bfp (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (FLOAT_TO_BFP(atof (str)), low_input[ld->channel], 65535);
  if (value != high_input[ld->channel])
  {
      high_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static gint 
levels_low_input_text_check_bfp (
				LevelsDialog *ld,
				char *str 
				)
{
  int value;
  guint16* high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16* low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  value = BOUNDS (FLOAT_TO_BFP(atof (str)), 0, high_input[ld->channel]);
  if (value != low_input[ld->channel])
  {
      low_input[ld->channel] = value;
      return TRUE;
  }
  return FALSE;
}

static void
levels_alloc_transfers_bfp (LevelsDialog *ld)
{
  gint i;
  guint16* data;
  Tag tag = tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO);
  
  
  /* alloc the high and low input and output arrays*/
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_input_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->high_output_pr, tag, (guchar*)data, 5);
  data = (guint16*) g_malloc (sizeof(guint16) * 5 );
  pixelrow_init (&ld->low_output_pr, tag, (guchar*)data, 5);
	
  /* allocate the input and output transfer arrays */
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&input[i], tag, (guchar*)data, 65536);
  }
  
  for (i = 0; i < 5; i++)
  { 
       data = (guint16*) g_malloc (sizeof(guint16) * 65536);
       pixelrow_init (&output[i], tag, (guchar*)data, 65536);
  }
}

static void
levels_build_input_da_transfer_bfp (
				LevelsDialog *ld,
				guchar *buf 
				)
{
  gint i;
  guint16 *input_data = (guint16 *)pixelrow_data (&input[ld->channel]); 
  for(i = 0; i < DA_WIDTH; i++)
	buf[i] = (guchar) (input_data[i * 257]/257.0); 
}

static void
levels_update_low_input_set_text_bfp (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 sprintf (text, "%f", BFP_TO_FLOAT(low_input[ld->channel]));
}

static void
levels_update_high_input_set_text_bfp (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 sprintf (text, "%f", BFP_TO_FLOAT(high_input[ld->channel]));
}

static void
levels_update_low_output_set_text_bfp (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 sprintf (text, "%f", BFP_TO_FLOAT(low_output[ld->channel]));
}

static void
levels_update_high_output_set_text_bfp (
			 LevelsDialog *ld,
			 char * text
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 sprintf (text, "%f", BFP_TO_FLOAT(high_output[ld->channel]));
}

/* want this normalized to 0.0..1.0 rather than proper float conversion 
   used for slider positions */
static gfloat 
levels_get_low_input_bfp (
			 LevelsDialog *ld
			)
{
 guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
 return low_input[ld->channel]/65535.0;
}

static gfloat 
levels_get_high_input_bfp (
			 LevelsDialog *ld
			)
{
 guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
 return high_input[ld->channel]/65535.0;
}

static gfloat 
levels_get_low_output_bfp (
			 LevelsDialog *ld
			)
{
 guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
 return low_output[ld->channel]/65535.0;
}

static gfloat 
levels_get_high_output_bfp (
			 LevelsDialog *ld
			)
{
 guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);
 return high_output[ld->channel]/65535.0;
}


static void
levels_funcs (
		 Tag dest_tag
		)
{
  gint i;
  switch (tag_precision (dest_tag))
  {
  case PRECISION_U8:
	levels = levels_u8;
	levels_calculate_transfers = levels_calculate_transfers_u8;
	levels_input_da_setui_values = levels_input_da_setui_values_u8;
	levels_output_da_setui_values = levels_output_da_setui_values_u8;
	levels_high_output_text_check = levels_high_output_text_check_u8;
	levels_low_output_text_check = levels_low_output_text_check_u8;
	levels_high_input_text_check = levels_high_input_text_check_u8;
	levels_low_input_text_check = levels_low_input_text_check_u8;
	levels_alloc_transfers = levels_alloc_transfers_u8;
	levels_build_input_da_transfer = levels_build_input_da_transfer_u8; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_u8;
	levels_update_high_input_set_text = levels_update_high_input_set_text_u8;
	levels_update_low_output_set_text = levels_update_low_output_set_text_u8;
	levels_update_high_output_set_text = levels_update_high_output_set_text_u8;
	levels_get_low_input = levels_get_low_input_u8;
	levels_get_high_input = levels_get_high_input_u8;
	levels_get_low_output = levels_get_low_output_u8;
	levels_get_high_output = levels_get_high_output_u8;
        levels_init_high_low_input_output = levels_init_high_low_input_output_u8;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_u8[i];
	break;
  case PRECISION_U16:
	levels = levels_u16;
	levels_calculate_transfers = levels_calculate_transfers_u16;
	levels_input_da_setui_values = levels_input_da_setui_values_u16;
	levels_output_da_setui_values = levels_output_da_setui_values_u16;
	levels_high_output_text_check = levels_high_output_text_check_u16;
	levels_low_output_text_check = levels_low_output_text_check_u16;
	levels_high_input_text_check = levels_high_input_text_check_u16;
	levels_low_input_text_check = levels_low_input_text_check_u16;
	levels_alloc_transfers = levels_alloc_transfers_u16;
	levels_build_input_da_transfer = levels_build_input_da_transfer_u16; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_u16;
	levels_update_high_input_set_text = levels_update_high_input_set_text_u16;
	levels_update_low_output_set_text = levels_update_low_output_set_text_u16;
	levels_update_high_output_set_text = levels_update_high_output_set_text_u16;
	levels_get_low_input = levels_get_low_input_u16;
	levels_get_high_input = levels_get_high_input_u16;
	levels_get_low_output = levels_get_low_output_u16;
	levels_get_high_output = levels_get_high_output_u16;
        levels_init_high_low_input_output = levels_init_high_low_input_output_u16;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_u16[i];
	break;
  case PRECISION_FLOAT:
	levels = levels_float;
	levels_calculate_transfers = levels_calculate_transfers_float;
	levels_input_da_setui_values = levels_input_da_setui_values_float;
	levels_output_da_setui_values = levels_output_da_setui_values_float;
	levels_high_output_text_check = levels_high_output_text_check_float;
	levels_low_output_text_check = levels_low_output_text_check_float;
	levels_high_input_text_check = levels_high_input_text_check_float;
	levels_low_input_text_check = levels_low_input_text_check_float;
	levels_alloc_transfers = levels_alloc_transfers_float;
	levels_build_input_da_transfer = levels_build_input_da_transfer_float; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_float;
	levels_update_high_input_set_text = levels_update_high_input_set_text_float;
	levels_update_low_output_set_text = levels_update_low_output_set_text_float;
	levels_update_high_output_set_text = levels_update_high_output_set_text_float;
	levels_get_low_input = levels_get_low_input_float;
	levels_get_high_input = levels_get_high_input_float;
	levels_get_low_output = levels_get_low_output_float;
	levels_get_high_output = levels_get_high_output_float;
        levels_init_high_low_input_output = levels_init_high_low_input_output_float;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_float[i];
	break;
  case PRECISION_FLOAT16:
	levels = levels_float16;
	levels_calculate_transfers = levels_calculate_transfers_float16;
	levels_input_da_setui_values = levels_input_da_setui_values_float16;
	levels_output_da_setui_values = levels_output_da_setui_values_float16;
	levels_high_output_text_check = levels_high_output_text_check_float16;
	levels_low_output_text_check = levels_low_output_text_check_float16;
	levels_high_input_text_check = levels_high_input_text_check_float16;
	levels_low_input_text_check = levels_low_input_text_check_float16;
	levels_alloc_transfers = levels_alloc_transfers_float16;
	levels_build_input_da_transfer = levels_build_input_da_transfer_float16; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_float16;
	levels_update_high_input_set_text = levels_update_high_input_set_text_float16;
	levels_update_low_output_set_text = levels_update_low_output_set_text_float16;
	levels_update_high_output_set_text = levels_update_high_output_set_text_float16;
	levels_get_low_input = levels_get_low_input_float16;
	levels_get_high_input = levels_get_high_input_float16;
	levels_get_low_output = levels_get_low_output_float16;
	levels_get_high_output = levels_get_high_output_float16;
        levels_init_high_low_input_output = levels_init_high_low_input_output_float16;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_float16[i];
	break;
  case PRECISION_BFP:
	levels = levels_bfp;
	levels_calculate_transfers = levels_calculate_transfers_bfp;
	levels_input_da_setui_values = levels_input_da_setui_values_bfp;
	levels_output_da_setui_values = levels_output_da_setui_values_bfp;
	levels_high_output_text_check = levels_high_output_text_check_bfp;
	levels_low_output_text_check = levels_low_output_text_check_bfp;
	levels_high_input_text_check = levels_high_input_text_check_bfp;
	levels_low_input_text_check = levels_low_input_text_check_bfp;
	levels_alloc_transfers = levels_alloc_transfers_bfp;
	levels_build_input_da_transfer = levels_build_input_da_transfer_bfp; 
        levels_update_low_input_set_text = levels_update_low_input_set_text_bfp;
	levels_update_high_input_set_text = levels_update_high_input_set_text_bfp;
	levels_update_low_output_set_text = levels_update_low_output_set_text_bfp;
	levels_update_high_output_set_text = levels_update_high_output_set_text_bfp;
	levels_get_low_input = levels_get_low_input_bfp;
	levels_get_high_input = levels_get_high_input_bfp;
	levels_get_low_output = levels_get_low_output_bfp;
	levels_get_high_output = levels_get_high_output_bfp;
        levels_init_high_low_input_output = levels_init_high_low_input_output_bfp;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = ui_strings_bfp[i];
	break;
  default:
	levels = NULL;
	levels_calculate_transfers = NULL;
	levels_input_da_setui_values = NULL;
	levels_output_da_setui_values = NULL;
	levels_high_output_text_check = NULL;
	levels_low_output_text_check = NULL;
	levels_high_input_text_check = NULL;
	levels_low_input_text_check = NULL;
	levels_alloc_transfers = NULL;
	levels_build_input_da_transfer = NULL; 
        levels_update_low_input_set_text = NULL;
	levels_update_high_input_set_text = NULL;
	levels_update_low_output_set_text = NULL;
	levels_update_high_output_set_text = NULL;
	levels_get_low_input = NULL;
	levels_get_high_input = NULL;
	levels_get_low_output = NULL;
	levels_get_high_output = NULL;
        levels_init_high_low_input_output = NULL;
	for (i = 0; i < 4; i++)
	  ui_strings[i] = NULL;
	break;
  }
}

static gfloat
compute_input (gfloat val,
	     gfloat low,
	     gfloat high,
	     gfloat gamma,
		gint clip)
{
    gfloat inten;

    /*  determine input intensity  */
    if (high != low)
      inten =  (val - low) / (gfloat) (high - low);
    else
      inten = val - low;

    if (clip)
      inten = BOUNDS (inten, 0.0, 1.0);
    else
      if (inten < 0 ) inten = 0.0;

    if (gamma != 0.0) 
      inten = pow (inten, (1.0 / gamma));

    return inten;
}

gfloat
compute_output (gfloat val, 
	      gfloat low, 
	      gfloat high,
		gint clip) 
{
    gfloat inten;

    /*  determine the output intensity  */
    inten = val;
    if (high >= low)
      inten = inten * (high - low) + low;
    else if (high < low)
      inten =  low - inten * (low - high);

    if (clip)
      inten = BOUNDS (inten, 0.0, 1.0);

    return inten;
}


static void
levels_calculate_transfers_ui (LevelsDialog *ld)
{
  gfloat inten;
  int i, j;
  gint saved = ld->channel;

  for (j = 0; j < 5; j++)
    {
      ld->channel = j;
      {
	guint8 low_input = (*levels_get_low_input)(ld) * 255;
	guint8 high_input = (*levels_get_high_input)(ld) * 255;
	for (i = 0; i < 256; i++)
	  {
	    /*  determine input intensity  */
	    if (high_input != low_input)
	      inten = (gfloat) (i - low_input) /
		(gfloat) (high_input - low_input);
	    else
	      inten = (gfloat) (i - low_input);

	    inten = BOUNDS (inten, 0.0, 1.0);
	    if (ld->gamma[j] != 0.0)
	      inten = pow (inten, (1.0 / ld->gamma[j]));
	    ld->input_ui[j][i] = (guint8) (inten * 255.0 + 0.5);
	  }
       }
    }
  ld->channel = saved;
}

static void
levels_free_transfers (LevelsDialog *ld)
{
  gint i;
  guchar * data;
 
  data = pixelrow_data (&ld->high_input_pr);
  if (data) g_free (data);
  data = pixelrow_data (&ld->low_input_pr);
  if (data) g_free (data);
  data = pixelrow_data (&ld->high_output_pr);
  if (data) g_free (data);
  data = pixelrow_data (&ld->low_output_pr);
  if (data) g_free (data);

  for (i = 0; i < 5; i++)
  {
    data = pixelrow_data (&input[i]);
    if (data) g_free (data);
    data = pixelrow_data (&output[i]);
    if (data) g_free (data);
  }
}

/*  levels action functions  */
static int first = TRUE;

static void
levels_set_picker_values (int channel, double val)
{
  int success = FALSE;
  static char str[64];

  sprintf (str, "%f", val);

  #ifdef DEBUG
  printf ("%s:%d %s() %d:%s\n",__FILE__,__LINE__,__func__, channel-1,str);
  #endif
  levels_dialog->channel = channel;

  switch (levels_dialog->active_picker)
  { case BLACK_PICKER:
            success = (*levels_low_input_text_check)(levels_dialog, str);
            break;
    case GRAY_PICKER:
            levels_dialog->gamma[levels_dialog->channel] = val;
            success = TRUE;
            break;
    case WHITE_PICKER:
            success = (*levels_high_input_text_check)(levels_dialog, str);
            break;
  }

  if (success)
  {
    /*levels_update (levels_dialog, ALL);

    if (levels_dialog->preview)
      levels_preview (levels_dialog);*/
  }
}

static void
levels_color_picker_select (Tool           *tool,
                            GdkEvent       *event,
                            GDisplay       *gdisp,
                            int            x,
                            int            y,
                            int            select)
{
  LevelsDialog* ld = levels_dialog;

  Format format;
  Canvas *c;
  guchar *d;
  Alpha alpha;
  Tag tag;
  ShortsFloat u;
  gint offset_x, offset_y;
  CanvasDrawable *drawable = NULL;
  int i, end_channel, all_channels;
  int min=255, max=0;

  double Lab[3], color[16], gray[16], gamma, end_range, low_input, high_input;

  tool->drawable = gimage_active_drawable (gdisp->gimage);
  
  drawable = tool->drawable;

  if (drawable) 
  {   if (first)
      { c = drawable_data(drawable);
        first = FALSE;
      } else {
        c = image_map_get_src_canvas(ld->image_map);
      }
  } else {
      g_warning("No drawable");
      return;
  }

  tag = drawable_tag (drawable);
  format = tag_format (tag);
  alpha = tag_alpha (tag);

  drawable_offsets (drawable, &offset_x, &offset_y);
  
  /* color_values */
  if ( x >= 0 && x < gdisp->gimage->width && 
       y >= 0 && y < gdisp->gimage->height )
  {

  x -= offset_x;
  y -= offset_y;

  if ( !(x >= 0 && x < drawable_width(drawable) && 
         y >= 0 && y < drawable_height(drawable) ))
    return;

  d = canvas_portion_data (c, x, y);

  if (!d)
    d = canvas_portion_data (drawable_data(drawable), x, y);

  //printf("%s:%d %s()\n",__FILE__,__LINE__,__func__);
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      end_range = 255.0;
      {
	guint8* data = (guint8*)d;

	if (format == FORMAT_RGB) {
#if 0
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%3d, %3d, %3d, %3d",
		 (int)data[0], (int)data[1], (int)data[2], (int)data[3]);
	  else
#endif
          if (ld->active_picker == GRAY_PICKER
           && select)
          { // CIE*L for this colour?
            int ld_channel_orig = ld->channel;

            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
            for (; i <= end_channel; i++)
            { ld->channel = i;
              low_input = (*levels_get_low_input)(ld) * end_range;
              high_input = (*levels_get_high_input)(ld) * end_range;
              // normalice for high- and lowinput
              gray[i-1] =  (data[i-1]-low_input) / (high_input-low_input)
                           * end_range;
              color[i-1] = gray[i-1] / end_range * 100.0;
            }
            ld->channel = ld_channel_orig;

            if (ld->xform)
                cmsDoTransform (ld->xform, color, Lab, 1);
            Lab[1] = 0.0; // move color to gray axis
            Lab[2] = 0.0;
            if (ld->bform)
                cmsDoTransform (ld->bform, Lab, color, 1);
          }

          if (ld->channel == HISTOGRAM_VALUE)
          { all_channels = TRUE;
            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
          } else {
            all_channels = FALSE;
            i = ld->channel; end_channel = i;
          }

          for (; i <= end_channel; i++)
          {
            if (ld->active_picker == GRAY_PICKER && select) {
              gamma = log( gray[i-1] / end_range ) / log( color[i-1] / 100.0 );
              levels_set_picker_values ( i, gamma );
            } else if (select) {
              levels_set_picker_values (i, (double)data[i-1]);
            }
            if ((data[i-1] / end_range * 255.0) > max)
              max = (data[i-1] / end_range * 255.0);
            if ((data[i-1] / end_range * 255.0) < min)
              min = (data[i-1] / end_range * 255.0);
          }
          if (all_channels)
            ld->channel = HISTOGRAM_VALUE;

	} else if (format == FORMAT_GRAY) {
#if 0
	  if (alpha == ALPHA_YES)
	    sprintf (iwd->xy_color_values_str, "%3d, %3d",
		 (int)data[0], (int)data[1]);
	  else
#endif
          i = HISTOGRAM_RED;
          if (select)
            levels_set_picker_values (i, (double)data[i-1]);
          ld->channel = HISTOGRAM_VALUE;
          max = min = (data[i-1] / end_range * 255.0);
	} else if (format == FORMAT_INDEXED) {
        }
        histogram_range (levels_dialog->histogram, min, max);

      }
      if (select && levels_dialog->preview) {
        levels_preview (levels_dialog);
        levels_update (ld, ALL);
      }
      break;
      
    case PRECISION_U16:
      end_range = 65535.0;
      {
	guint16* data = (guint16*)d;

	if (format == FORMAT_RGB)
        {
          if (ld->active_picker == GRAY_PICKER && select)
          { // CIE*L for this colour?
            int ld_channel_orig = ld->channel;

            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
            for (; i <= end_channel; i++)
            { ld->channel = i;
              low_input = (*levels_get_low_input)(ld) * end_range;
              high_input = (*levels_get_high_input)(ld) * end_range;
              // normalice for high- and lowinput
              gray[i-1] =  (data[i-1]-low_input) / (high_input-low_input)
                           * end_range;
              color[i-1] = gray[i-1] / end_range * 100.0;
            }
            ld->channel = ld_channel_orig;

            if (ld->xform)
                cmsDoTransform (ld->xform, color, Lab, 1);
            Lab[1] = 0.0; // move color to gray axis
            Lab[2] = 0.0;
            if (ld->bform)
                cmsDoTransform (ld->bform, Lab, color, 1);
          }

          if (ld->channel == HISTOGRAM_VALUE)
          { all_channels = TRUE;
            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
          } else {
            all_channels = FALSE;
            i = ld->channel; end_channel = i;
          }

          for (; i <= end_channel; i++)
          {
            if (ld->active_picker == GRAY_PICKER
             && select) {
              gamma = log( gray[i-1] / end_range ) / log( color[i-1] / 100.0 );
              levels_set_picker_values ( i, gamma );
            } else if (select) {
              levels_set_picker_values (i, data[i-1]);
            }
            if ((data[i-1] / end_range * 255.0) > max)
              max = (data[i-1] / end_range * 255.0);
            if ((data[i-1] / end_range * 255.0) < min)
              min = (data[i-1] / end_range * 255.0);
          }
          if (all_channels)
            ld->channel = HISTOGRAM_VALUE;

	} else if (format == FORMAT_GRAY) {
          i = HISTOGRAM_RED;
          if (select)
            levels_set_picker_values (i, (double)(data[i-1]));
          ld->channel = HISTOGRAM_VALUE;
          max = min = (data[i-1] / end_range * 255.0);
	} else if (format == FORMAT_INDEXED) {
        }
        histogram_range (levels_dialog->histogram, min, max);
      }
      if (select && levels_dialog->preview) {
        levels_preview (levels_dialog);
        levels_update (ld, ALL);
      }
      break;
 
    case PRECISION_FLOAT:
      end_range = 1.0;
      {
	gfloat* data = (gfloat*)d;

	if (format == FORMAT_RGB) {
          if (ld->active_picker == GRAY_PICKER
           && select)
          { // CIE*L for this colour?
            int ld_channel_orig = ld->channel;

            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
            for (; i <= end_channel; i++)
            { ld->channel = i;
              low_input = (*levels_get_low_input)(ld) * end_range;
              high_input = (*levels_get_high_input)(ld) * end_range;
              // normalice for high- and lowinput
              gray[i-1] =  (data[i-1]-low_input) / (high_input-low_input)
                           * end_range;
              color[i-1] = gray[i-1] / end_range * 100.0;
            }
            ld->channel = ld_channel_orig;

            if (ld->xform)
                cmsDoTransform (ld->xform, color, Lab, 1);
            Lab[1] = 0.0; // move color to gray axis
            Lab[2] = 0.0;
            if (ld->bform)
                cmsDoTransform (ld->bform, Lab, color, 1);
          }

          if (ld->channel == HISTOGRAM_VALUE)
          { all_channels = TRUE;
            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
          } else {
            all_channels = FALSE;
            i = ld->channel; end_channel = i;
          }

          for (; i <= end_channel; i++)
          {
            if (ld->active_picker == GRAY_PICKER
             && select)
            { gamma = log( gray[i-1] / end_range ) / log( color[i-1] / 100.0 );
              levels_set_picker_values ( i, gamma );
            } else if (select) {
              levels_set_picker_values (i, (double)(data[i-1]));
            }
            if ((data[i-1] / end_range * 255.0) > max)
              max = (data[i-1] / end_range * 255.0);
            if ((data[i-1] / end_range * 255.0) < min)
              min = (data[i-1] / end_range * 255.0);
          }
          if (all_channels)
            ld->channel = HISTOGRAM_VALUE;

	} else if (format == FORMAT_GRAY) {
          i = HISTOGRAM_RED;
          if (select)
            levels_set_picker_values (i, (double)(data[i-1]));
          ld->channel = HISTOGRAM_VALUE;
          max = min = (data[i-1] / end_range * 255.0);
	} else if (format == FORMAT_INDEXED) {
        }
        histogram_range (levels_dialog->histogram, min, max);
      }
      if (select && levels_dialog->preview) {
        levels_preview (levels_dialog);
        levels_update (ld, ALL);
      }
      break;

    case PRECISION_FLOAT16:
      end_range = 1.0;
      {
	guint16* data = (guint16*)d;

	if (format == FORMAT_RGB) {
          if (ld->active_picker == GRAY_PICKER && select)
          { // CIE*L for this colour?
            int ld_channel_orig = ld->channel;

            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
            for (; i <= end_channel; i++)
            { ld->channel = i;
              low_input = (*levels_get_low_input)(ld) * end_range;
              high_input = (*levels_get_high_input)(ld) * end_range;
              // normalice for high- and lowinput
              gray[i-1] =  (FLT (data[i-1], u)-low_input)
                           / (high_input-low_input) * end_range;
              color[i-1] = gray[i-1] / end_range * 100.0;
            }
            ld->channel = ld_channel_orig;

            if (ld->xform)
                cmsDoTransform (ld->xform, color, Lab, 1);
            Lab[1] = 0.0; // move color to gray axis
            Lab[2] = 0.0;
            if (ld->bform)
                cmsDoTransform (ld->bform, Lab, color, 1);
          }

          if (ld->channel == HISTOGRAM_VALUE)
          { all_channels = TRUE;
            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
          } else {
            all_channels = FALSE;
            i = ld->channel; end_channel = i;
          }

          for (; i <= end_channel; i++)
          {
            if (ld->active_picker == GRAY_PICKER && select)
            { gamma = log( gray[i-1] / end_range ) / log( color[i-1] / 100.0 );
              levels_set_picker_values ( i, gamma );
            } else if (select) {
              levels_set_picker_values (i, FLT (data[i-1], u));
            }
            if (( FLT (data[i-1], u) / end_range * 255.0) > max)
              max = ( FLT (data[i-1], u) / end_range * 255.0);
            if (( FLT (data[i-1], u) / end_range * 255.0) < min)
              min = ( FLT (data[i-1], u) / end_range * 255.0);
          }
          if (all_channels)
            ld->channel = HISTOGRAM_VALUE;

	} else if (format == FORMAT_GRAY) {
          i = HISTOGRAM_RED;
          if (select)
            levels_set_picker_values (i, FLT (data[i-1], u));
          ld->channel = HISTOGRAM_VALUE;
          max = min = (FLT (data[i-1], u) / end_range * 255.0);
	} else if (format == FORMAT_INDEXED) {
        }
        histogram_range (levels_dialog->histogram, min, max);
      }
      if (select && levels_dialog->preview) {
        levels_preview (levels_dialog);
        levels_update (ld, ALL);
      }
      break;

    case PRECISION_BFP:
      end_range = 65535.0/2.0;
      {
	guint16* data = (guint16*)d;

	if (format == FORMAT_RGB) {
          if (ld->active_picker == GRAY_PICKER && select)
          { // CIE*L for this colour?
            int ld_channel_orig = ld->channel;

            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
            for (; i <= end_channel; i++)
            { ld->channel = i;
              low_input = (*levels_get_low_input)(ld) * end_range;
              high_input = (*levels_get_high_input)(ld) * end_range;
              // normalice for high- and lowinput
              gray[i-1] =  (BFP_TO_FLOAT (data[i-1])-low_input)
                           / (high_input-low_input) * end_range;
              color[i-1] = gray[i-1] / end_range * 100.0;
            }
            ld->channel = ld_channel_orig;

            if (ld->xform)
                cmsDoTransform (ld->xform, color, Lab, 1);
            Lab[1] = 0.0; // move color to gray axis
            Lab[2] = 0.0;
            if (ld->bform)
                cmsDoTransform (ld->bform, Lab, color, 1);
          }

          if (ld->channel == HISTOGRAM_VALUE)
          { all_channels = TRUE;
            i = HISTOGRAM_RED; end_channel = HISTOGRAM_BLUE;
          } else {
            all_channels = FALSE;
            i = ld->channel; end_channel = i;
          }

          for (; i <= end_channel; i++)
          {
            if (ld->active_picker == GRAY_PICKER && select)
            { gamma = log( gray[i-1] / end_range ) / log( color[i-1] / 100.0 );
              levels_set_picker_values ( i, gamma );
            } else if (select) {
              levels_set_picker_values (i, (double)(BFP_TO_FLOAT(data[i-1])));
            }
            if (( BFP_TO_FLOAT(data[i-1]) / end_range * 255.0) > max)
              max = ( BFP_TO_FLOAT(data[i-1]) / end_range * 255.0);
            if (( BFP_TO_FLOAT(data[i-1]) / end_range * 255.0) < min)
              min = ( BFP_TO_FLOAT(data[i-1]) / end_range * 255.0);
          }
          if (all_channels)
            ld->channel = HISTOGRAM_VALUE;

	} else if (format == FORMAT_GRAY) {
          i = HISTOGRAM_RED;
          if (select)
            levels_set_picker_values (i, (double)(BFP_TO_FLOAT(data[i-1])));
          ld->channel = HISTOGRAM_VALUE;
          max = min = (BFP_TO_FLOAT(data[i-1]) / end_range * 255.0);
	} else if (format == FORMAT_INDEXED) {
        }
        histogram_range (levels_dialog->histogram, min, max);
      }
      if (select && levels_dialog->preview) {
        levels_preview (levels_dialog);
        levels_update (ld, ALL);
      }
      break;

    case PRECISION_NONE:
      g_warning ("bad precision");
      break;
    }
  }
}

static void
levels_button_press (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  int x, y;

  LevelsDialog* ld = levels_dialog;

  gdisp = (GDisplay *) gdisp_ptr;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    (GDK_POINTER_MOTION_HINT_MASK |
		     GDK_BUTTON1_MOTION_MASK |
		     GDK_BUTTON_RELEASE_MASK),
		    NULL, NULL, bevent->time);
  #ifdef DEBUG
  printf ("%s:%d %s() %d %d\n",__FILE__,__LINE__,__func__,ld->active_picker, (int)ld->xform);
  #endif

  // prepare gray balancing
  if (ld->active_picker == GRAY_PICKER &&
      (ld->xform == NULL
       || tool->gdisp_ptr != gdisp_ptr
       || tool->state == INACTIVE) )
  {
    cmsHPROFILE hLab;
    cmsHPROFILE imageProfile;
    DWORD format;
    int channels;

    // Profile with lcms
    hLab = cmsCreateLabProfile(cmsD50_xyY());
    if (gdisp->gimage->cms_profile) {
      char* data;
      int size;
      data = cms_get_profile_data (gimage_get_cms_profile (gdisp->gimage),
                                   &size );
      imageProfile = cmsOpenProfileFromMem(data, size);
      if (imageProfile) {
        #ifdef DEBUG
        printf("%s:%d %s() init profile from image\n",__FILE__,__LINE__,__func__);
        #endif
      } else {
        g_warning("%s:%d %s() init profile from image failed\n",__FILE__,__LINE__,__func__);
      }
      free (data);
    } else {// assume sRGB
      imageProfile = cmsCreate_sRGBProfile();
  #ifdef DEBUG
      printf("%s:%d %s() init sRGB profile\n",__FILE__,__LINE__,__func__);
  #endif
    }
    // Transform
    channels = _cmsChannelsOf(cmsGetColorSpace(imageProfile));
    format = (COLORSPACE_SH(PT_ANY)| CHANNELS_SH(channels)| BYTES_SH(0));
    #ifdef DEBUG
  #ifdef DEBUG
    printf("%s:%d %s() channels %d format %d\n",__FILE__,__LINE__,__func__,channels,format);
  #endif
    #endif
     
    ld->xform = cmsCreateTransform (imageProfile, format,
                                    hLab, TYPE_Lab_DBL,
                                    INTENT_RELATIVE_COLORIMETRIC,
                                    cmsFLAGS_NOTPRECALC);
    ld->bform = cmsCreateTransform (hLab, TYPE_Lab_DBL,
                                    imageProfile, format,
                                    INTENT_RELATIVE_COLORIMETRIC,
                                    cmsFLAGS_NOTPRECALC);
    if (hLab) cmsCloseProfile (hLab);
    if (imageProfile) cmsCloseProfile (imageProfile);
  }

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp,CAST(int) bevent->x,CAST(int) bevent->y, &x, &y, FALSE, FALSE);

  levels_color_picker_select (tool, (GdkEvent*)bevent, gdisp, x,y, TRUE);
}

static void
levels_button_release (Tool           *tool,
		       GdkEventButton *bevent,
		       gpointer        gdisp_ptr)
{
}

static void
levels_motion (Tool           *tool,
	       GdkEventMotion *mevent,
	       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x,y;

  gdisp = gdisp_ptr;

  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);

  levels_color_picker_select (tool, (GdkEvent*)mevent, gdisp, x,y, TRUE);
  #ifdef DEBUG
  printf ("%s:%d %s() %d,%d\n",__FILE__,__LINE__,__func__,x,y);
  #endif
}

static void
levels_cursor_update (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;
  int width, height;
  char* picker_bits;
  GdkCursor *cursor;
  GdkPixmap *source, *mask;
  GdkColor fg = { 0, 0, 0, 0 };
  GdkColor bg = { 0, 65535, 65535, 65535 };

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp,CAST(int) mevent->x,CAST(int) mevent->y, &x, &y, FALSE, FALSE);

  
  if (gimage_pick_correlate_layer (gdisp->gimage, x, y))
    gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);

  switch (levels_dialog->active_picker)
  { case BLACK_PICKER:
         width = picker_black_width;
         height = picker_black_height;
         picker_bits = picker_black_bits;
         break;
    case GRAY_PICKER:
         width = picker_gray_width;
         height = picker_gray_height;
         bg.red = 32768;
         bg.green = 32768;
         bg.blue = 32768;
         picker_bits = picker_gray_bits;
         break;
    case WHITE_PICKER:
         width = picker_white_width;
         height = picker_white_height;
         picker_bits = picker_white_bits;
         break;
    default:
         return;
         break;
  }

  source = gdk_bitmap_create_from_data (NULL, picker_bits,
					width, height);
  mask = gdk_bitmap_create_from_data (NULL, picker_mask_bits,
				      width, height);
  cursor = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, 0, 23);
  gdk_pixmap_unref (source);
  gdk_pixmap_unref (mask);

  gdk_window_set_cursor (gdisp->canvas->window, cursor);
  levels_color_picker_select (tool, (GdkEvent*)mevent, gdisp, x,y, FALSE);
  #ifdef DEBUG
  printf ("%s:%d %s() %d,%d\n",__FILE__,__LINE__,__func__,x,y);
  #endif
}

static void
levels_control (Tool     *tool,
		int       action,
		gpointer  gdisp_ptr)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (levels_dialog)
	{
          active_tool->preserve = TRUE;
	  image_map_abort (levels_dialog->image_map);
          active_tool->preserve = FALSE;
	  levels_free_transfers(levels_dialog);
	  levels_dialog->image_map = NULL;
	  levels_cancel_callback (NULL, (gpointer) levels_dialog);
	}
      break;
    }
}

Tool *
tools_new_levels ()
{
  Tool * tool;
  Levels * private;

  /*  The tool options  */
  if (!levels_options)
    levels_options = tools_register_no_options (LEVELS, _("Levels Options"));

  tool = (Tool *) g_malloc_zero (sizeof (Tool));
  private = (Levels *) g_malloc_zero (sizeof (Levels));

  tool->type = LEVELS;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = levels_button_press;
  tool->button_release_func = levels_button_release;
  tool->motion_func = levels_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = levels_cursor_update;
  tool->control_func = levels_control;
  tool->preserve = TRUE;//FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_levels (Tool *tool)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  /*  Close the color select dialog  */
  if (levels_dialog)
    levels_destroy (NULL, (gpointer) levels_dialog);

  g_free (_levels);
}

static MenuItem color_option_items[] =
{
  { "Value", 0, 0, levels_value_callback, NULL, NULL, NULL },
  { "Red", 0, 0, levels_red_callback, NULL, NULL, NULL },
  { "Green", 0, 0, levels_green_callback, NULL, NULL, NULL },
  { "Blue", 0, 0, levels_blue_callback, NULL, NULL, NULL },
  { "Alpha", 0, 0, levels_alpha_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

void
levels_initialize (void *gdisp_ptr)
{
  GDisplay	   *gdisp;
  int                bins=-1;
  int           init = 0;
  CanvasDrawable *drawable;

  color_option_items[0].label =  _("Value");
  color_option_items[1].label =  _("Red");
  color_option_items[2].label =  _("Green");
  color_option_items[3].label =  _("Blue");
  color_option_items[4].label =  _("Alpha");

  gdisp = (GDisplay *) gdisp_ptr;
  drawable = gimage_active_drawable (gdisp->gimage);

  active_tool = tools_new_levels();

  if (drawable_indexed (drawable))
    {
      g_message (_("Levels for indexed drawables cannot be adjusted."));
      return;
    }

  histogram_histogram_funcs (gimage_tag (gdisp->gimage));
  switch( tag_precision( gimage_tag( gdisp->gimage ) ) )
  {
  case PRECISION_U8:
    bins = 256;
    break;

  case PRECISION_FLOAT:
    bins = 256; /* Histogram code assumes this right now */
    break;

  case PRECISION_U16:
    bins = 256; /* Histogram code assumes this right now, too */
    break;

  case PRECISION_BFP:
    bins = 256; /* Histogram code assumes this right now, too */
    break;

  case PRECISION_FLOAT16:
    g_warning ("precision not implemented\n");
    return;
  }

  levels_funcs ( drawable_tag (drawable));
  /*  The levels dialog  */
  if (!levels_dialog) {
    levels_dialog = levels_new_dialog (bins);
    init = 1;
  } else
    if (!GTK_WIDGET_VISIBLE (levels_dialog->shell))
      gtk_widget_show (levels_dialog->shell);

  /*  Initialize the values  */
  levels_dialog->channel = HISTOGRAM_VALUE;
  levels_dialog->active_picker = WHITE_PICKER;
   
   (*levels_alloc_transfers) (levels_dialog);
   (*levels_init_high_low_input_output) (levels_dialog);


  levels_dialog->drawable = drawable;
  levels_dialog->color = drawable_color (levels_dialog->drawable);
  levels_dialog->image_map = image_map_create (gdisp_ptr, levels_dialog->drawable);

  {
    LevelsDialog* dialog = levels_dialog;
    const char** color_space_channel_names = gimage_get_channel_names(gdisp->gimage);

    GtkOptionMenu *option_menu = GTK_OPTION_MENU (dialog->channel_menu);
    GtkMenu *m = GTK_MENU(gtk_option_menu_get_menu(option_menu));
    GtkWidget *w = gtk_menu_get_active(m);
    MenuItem *items = color_option_items;
    int pos = -1;
    int i;
    int is_Lab = 0;
    if(gdisp->gimage->cms_profile)
      is_Lab = strstr("Lab",cms_get_profile_info(gdisp->gimage->cms_profile)
                              ->color_space_name) ? 1 : 0;

    pos = update_color_menu_items (items,
                                   GTK_OPTION_MENU(dialog->channel_menu),
                                   color_space_channel_names);

    if (is_Lab) {
      if (pos == 0)
        ++pos;
    }
    else {
      if (pos == 5)
        pos = 0;
    }

    if (is_Lab) {
      gtk_widget_hide( items[0].widget);
    } else {
      gtk_widget_show( items[0].widget);
    }

    if (!init || is_Lab)
      items[pos].callback (w, dialog);
    /* set the current selection */
    /* the following line is causing segfault for some reason....*/
    gtk_option_menu_set_history ( GTK_OPTION_MENU (option_menu), pos);

    /* check for alpha channel */
    if (drawable_has_alpha ( (dialog->drawable)))
      gtk_widget_show( items[4].widget);
    else
      gtk_widget_hide( items[4].widget);

    /*  hide or show the channel menu based on image type  */
    if (dialog->color)
      for (i = 0; i < 4; i++)
         gtk_widget_set_sensitive( items[i].widget, TRUE);
    else
      for (i = 1; i < 4; i++)
         gtk_widget_set_sensitive( items[i].widget, FALSE);
  }


  levels_update (levels_dialog, LOW_INPUT | GAMMA | HIGH_INPUT | LOW_OUTPUT | HIGH_OUTPUT | DRAW);
  levels_update (levels_dialog, INPUT_LEVELS | OUTPUT_LEVELS);
#define FIXME
#if 1
  histogram_update (levels_dialog->histogram,
		    levels_dialog->drawable,
		    histogram_histogram_info,
		    (void *) levels_dialog->histogram);
  histogram_range (levels_dialog->histogram, -1, -1);
#endif
  levels_dialog->xform = NULL;
  levels_dialog->bform = NULL;
}

void
levels_free ()
{
  if (levels_dialog)
    {
      if (levels_dialog->image_map)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (levels_dialog->image_map);
	  active_tool->preserve = FALSE;
	  levels_free_transfers(levels_dialog);
	  levels_dialog->image_map = NULL;
	}
      gtk_widget_destroy (levels_dialog->shell);
    }
}

/****************************/
/*  Levels dialog  */
/****************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
#if 1
  { "Reset", levels_reset_callback, NULL, NULL },
  { "Auto", levels_auto_levels_callback, NULL, NULL },
#endif
  { "OK", levels_ok_callback, NULL, NULL },
  { "Cancel", levels_cancel_callback, NULL, NULL }
};

static OpsButton picker_button[] =
{
  { picker_black_xpm, picker_black_is_xpm, levels_picker_black_callback, "Select Blackpoint", NULL, NULL, NULL, NULL, NULL, NULL },
  { picker_gray_xpm, picker_gray_is_xpm, levels_picker_gray_callback, "Select Grayaxis", NULL, NULL, NULL, NULL, NULL, NULL },
  { picker_white_xpm, picker_white_is_xpm, levels_picker_white_callback, "Select Whitepoint", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*
 *  TBD - WRB -make work with float data
*/
LevelsDialog *
levels_new_dialog (int bins)
{
  LevelsDialog *ld;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *channel_hbox;
  GtkWidget *picker_box;
  GtkWidget *menu;
  int i;

  ld = g_malloc_zero (sizeof (LevelsDialog));
  ld->preview = TRUE;

  picker_button[0].tooltip = _("Select Blackpoint");
  picker_button[1].tooltip = _("Select Grayaxis");
  picker_button[2].tooltip = _("Select Whitepoint");

  for (i = 0; i < 5; i++)
    color_option_items [i].user_data = (gpointer) ld;

  /*  The shell and main vbox  */
  ld->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (ld->shell), "levels", PROGRAM_NAME);
  gtk_window_set_title (GTK_WINDOW (ld->shell), _("Levels"));
  minimize_register(ld->shell);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (ld->shell), "delete_event",
		      GTK_SIGNAL_FUNC (levels_delete_callback),
		      ld);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ld->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Modify Levels for Channel: "));
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);

  menu = build_menu (color_option_items, NULL);
  ld->channel_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (channel_hbox), ld->channel_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (ld->channel_menu);
  gtk_widget_show (channel_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ld->channel_menu), menu);

  /* The color pickers */
  picker_box = ops_button_box_new2 (ld->shell, tool_tips, picker_button, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), picker_box, FALSE, FALSE, 0);
  gtk_widget_show (picker_box);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Input Levels: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low input text  */
  ld->low_input_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), ui_strings[LOW_INPUT_STRING]);
  gtk_widget_set_usize (ld->low_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_input_text), "changed",
		      (GtkSignalFunc) levels_low_input_text_update,
		      ld);
  gtk_widget_show (ld->low_input_text);

  /* input gamma text  */
  ld->gamma_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), "1.0");
  gtk_widget_set_usize (ld->gamma_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->gamma_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->gamma_text), "changed",
		      (GtkSignalFunc) levels_gamma_text_update,
		      ld);
  gtk_widget_show (ld->gamma_text);
  gtk_widget_show (hbox);

  /* high input text  */
  ld->high_input_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), ui_strings[HIGH_INPUT_STRING]);
  gtk_widget_set_usize (ld->high_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_input_text), "changed",
		      (GtkSignalFunc) levels_high_input_text_update,
		      ld);
  gtk_widget_show (ld->high_input_text);
  gtk_widget_show (hbox);

  /*  The levels histogram  */

#if 1
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  ld->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT, bins,
				    histogram_histogram_range, (void *) ld);
  ld->histogram->channel_menu = ld->channel_menu;
  ld->histogram->color_option_items = color_option_items;
  gtk_container_add (GTK_CONTAINER (frame), ld->histogram->histogram_widget);
  gtk_widget_show (ld->histogram->histogram_widget);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);
#endif

  /*  The input levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  ld->input_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->input_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[0]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[0], FALSE, TRUE, 0);
  ld->input_levels_da[1] = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (ld->input_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[1]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->input_levels_da[0]);
  gtk_widget_show (ld->input_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Output Levels: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low output text  */
  ld->low_output_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), ui_strings[LOW_OUTPUT_STRING]);
  gtk_widget_set_usize (ld->low_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_output_text), "changed",
		      (GtkSignalFunc) levels_low_output_text_update,
		      ld);
  gtk_widget_show (ld->low_output_text);

  /*  high output text  */
  ld->high_output_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), ui_strings[HIGH_OUTPUT_STRING]);
  gtk_widget_set_usize (ld->high_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_output_text), "changed",
		      (GtkSignalFunc) levels_high_output_text_update,
		      ld);
  gtk_widget_show (ld->high_output_text);
  gtk_widget_show (hbox);

  /*  The output levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  ld->output_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[0]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[0], FALSE, TRUE, 0);
  ld->output_levels_da[1] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[1]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->output_levels_da[0]);
  gtk_widget_show (ld->output_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), ld->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) levels_preview_update,
		      ld);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = ld;
  action_items[1].user_data = ld;
#if 1
  action_items[2].user_data = ld;
  action_items[3].user_data = ld;
  { int i;
    for(i = 0; i < 4; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  build_action_area (GTK_DIALOG (ld->shell), action_items, 4, 1);
#else
  { int i;
    for(i = 0; i < 2; ++i)
      action_items[i].label = _(action_items[i].label);
  }
  build_action_area (GTK_DIALOG (ld->shell), action_items, 2, 0);
#endif

  gtk_widget_show (vbox);
  gtk_widget_show (ld->shell);

  return ld;
}

static void
levels_draw_slider (GdkWindow *window,
		    GdkGC     *border_gc,
		    GdkGC     *fill_gc,
		    int        xpos)
{
  int y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line(window, fill_gc, xpos - y / 2, y,
		  xpos + y / 2, y);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos - (CONTROL_HEIGHT - 1) / 2,  CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
}

static void
levels_erase_slider (GdkWindow *window,
		     int        xpos)
{
  gdk_window_clear_area (window, xpos - (CONTROL_HEIGHT - 1) / 2, 0,
			 CONTROL_HEIGHT - 1, CONTROL_HEIGHT);
}

static void
levels_update (LevelsDialog *ld,
	       int           update)
{
  char text[12];
  int i;
  
  /*  Calculate the ui transfer arrays  */
  levels_calculate_transfers_ui (ld);

  if (update & LOW_INPUT)
    {
      (*levels_update_low_input_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), text);
    }
  if (update & GAMMA)
    {
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), text);
    }
  if (update & HIGH_INPUT)
    {
      (*levels_update_high_input_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), text);
    }
  if (update & LOW_OUTPUT)
    {
      (*levels_update_low_output_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), text);
    }
  if (update & HIGH_OUTPUT)
    {
      (*levels_update_high_output_set_text) (ld, text);
      gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), text);
    }
  if (update & INPUT_LEVELS)
    {
      unsigned char buf[DA_WIDTH];
      /* use the ui input transfer for the da */
      for (i = 0; i < DA_WIDTH; i++)
	buf[i] = ld->input_ui[ld->channel][i];
      
      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->input_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->input_levels_da[0], NULL);
    }
  if (update & OUTPUT_LEVELS)
    {
      unsigned char buf[DA_WIDTH];

      for (i = 0; i < DA_WIDTH; i++)
	buf[i] = i;

      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->output_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->output_levels_da[0], NULL);
    }
  if (update & INPUT_SLIDERS)
    {
      gfloat width, mid, tmp;

      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[0]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[1]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[2]);

      ld->slider_pos[0] = DA_WIDTH * ((*levels_get_low_input) (ld));
      ld->slider_pos[2] = DA_WIDTH * ((*levels_get_high_input) (ld));
      
      width = (gfloat) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;
      tmp = log10 (1.0 / ld->gamma[ld->channel]);
      ld->slider_pos[1] = (int) (mid + width * tmp + 0.5);

      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->dark_gc[GTK_STATE_NORMAL],
			  ld->slider_pos[1]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->slider_pos[0]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->white_gc,
			  ld->slider_pos[2]);
    }
  if (update & OUTPUT_SLIDERS)
    {
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[3]);
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[4]);

      ld->slider_pos[3] = DA_WIDTH * ((*levels_get_low_output) (ld));
      ld->slider_pos[4] = DA_WIDTH * ((*levels_get_high_output) (ld));

      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->slider_pos[3]);
      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->white_gc,
			  ld->slider_pos[4]);
    }
}

static void
levels_preview (LevelsDialog *ld)
{
  if (!ld->image_map)
    g_warning ("No image map");
  (*levels_calculate_transfers) (ld);
  active_tool->preserve = TRUE;
  image_map_apply (ld->image_map, levels, (void *) ld);
  active_tool->preserve = FALSE;
}

static void
levels_value_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_VALUE)
    {
      ld->channel = HISTOGRAM_VALUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_red_callback (GtkWidget *w,
		     gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_RED)
    {
      ld->channel = HISTOGRAM_RED;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_green_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_GREEN)
    {
      ld->channel = HISTOGRAM_GREEN;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_blue_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_BLUE)
    {
      ld->channel = HISTOGRAM_BLUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_alpha_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_ALPHA)
    {
      ld->channel = HISTOGRAM_ALPHA;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_picker_white_callback (GtkWidget *w,
		              gpointer   client_data)
{
  LevelsDialog *ld;
  ld = levels_dialog;
  ld->active_picker = WHITE_PICKER;
}

static void
levels_picker_gray_callback  (GtkWidget *w,
		              gpointer   client_data)
{
  LevelsDialog *ld;
  ld = levels_dialog;
  ld->active_picker = GRAY_PICKER;
}

static void
levels_picker_black_callback (GtkWidget *w,
		              gpointer   client_data)
{
  LevelsDialog *ld;
  ld = levels_dialog;
  ld->active_picker = BLACK_PICKER;
}

#if 1
static void
levels_adjust_channel_u8 (LevelsDialog    *ld,
		       HistogramValues  values,
		       int              channel)
{
  int i;
  gfloat count, new_count, percentage, next_percentage;
  guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
  guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
  guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
  guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

  ld->gamma[channel]   = 1.0;
  low_output[channel]  = 0;
  high_output[channel] = 255;

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += values[channel][i];

  if (count == 0.0)
    {
      low_input[channel] = 0;
      high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 255; i++)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      low_input[channel] = i;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i >= 0; i--)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      high_input[channel] = i;
	      break;
	    }
	}
    }
}

static void
levels_adjust_channel_u16 (LevelsDialog    *ld,
		       HistogramValues  values,
		       int              channel)
{
  int i;
  gfloat count, new_count, percentage, next_percentage;
  guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  ld->gamma[channel]   = 1.0;
  low_output[channel]  = 0;
  high_output[channel] = 65535;

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += values[channel][i];

  if (count == 0.0)
    {
      low_input[channel] = 0;
      high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 256; i++)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      low_input[channel] = i * 258.0;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i >= 0; i--)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      high_input[channel] = i * 258.0;
	      break;
	    }
	}
    }
}


static void
levels_adjust_channel_float (LevelsDialog    *ld,
		       HistogramValues  values,
		       int              channel)
{
  int i;
  gfloat count, new_count, percentage, next_percentage;
  gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
  gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
  gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
  gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

  ld->gamma[channel]   = 1.0;
  low_output[channel]  = 0;
  high_output[channel] = 1.0; /*G_MAXFLOAT*/

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += values[channel][i];

  if (count == 0.0)
    {
      low_input[channel] = 0;
      high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 255; i++)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      low_input[channel] = i / 254.0;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i >= 0; i--)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      high_input[channel] = (i) / 254.0;
	      break;
	    }
	}
    }
}

static void
levels_adjust_channel_bfp (LevelsDialog    *ld,
		       HistogramValues  values,
		       int              channel)
{
  int i;
  gfloat count, new_count, percentage, next_percentage;
  guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
  guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
  guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
  guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

  ld->gamma[channel]   = 1.0;
  low_output[channel]  = 0;
  high_output[channel] = 65535;

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += values[channel][i];

  if (count == 0.0)
    {
      low_input[channel] = 0;
      high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 255; i++)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      low_input[channel] = i * 258.0;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i >= 0; i--)
	{
	  new_count += values[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + values[channel][i]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      high_input[channel] = i * 258.0;
	      break;
	    }
	}
    }
}


static void
levels_auto_levels_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  LevelsDialog *ld;
  HistogramValues *values;
  int channel;

  ld = (LevelsDialog *) client_data;
  values = histogram_values (ld->histogram);

  ld->channel = HISTOGRAM_VALUE;
  levels_reset_callback (widget, client_data);

  if (ld->color)
    {
      ld->gamma[HISTOGRAM_VALUE]       = 1.0;
      switch (tag_precision (drawable_tag (ld->drawable)))
	{
	case PRECISION_U8:
	  for (channel = 0; channel < 3; channel ++)
	    levels_adjust_channel_u8 (ld, *values, channel + 1);
	  break;
	case PRECISION_U16:
	  for (channel = 0; channel < 3; channel ++)
	    levels_adjust_channel_u16 (ld, *values, channel + 1);
	  break;
	case PRECISION_FLOAT:
	  for (channel = 0; channel < 3; channel ++)
	    levels_adjust_channel_float (ld, *values, channel + 1);
	  break;
	case PRECISION_BFP:
	  for (channel = 0; channel < 3; channel ++)
	    levels_adjust_channel_bfp (ld, *values, channel + 1);
	  break;
	}
      

    }
#if 0
  else
    levels_adjust_channel (ld, *values, HISTOGRAM_VALUE);
#endif

  levels_update (ld, ALL);
  if (ld->preview)
    levels_preview (ld);
}

static void
levels_reset_callback       (GtkWidget *widget,
			     gpointer   client_data)
{
  LevelsDialog *ld;
  HistogramValues *values;
  int single = FALSE,
      channel;

  ld = levels_dialog;
  values = histogram_values (ld->histogram);

  channel = ld->channel;

  if (channel != HISTOGRAM_VALUE)
    single = TRUE;

  do
  {
    if (ld->color)
    {
      ld->gamma[channel]       = 1.0;
      switch (tag_precision (drawable_tag (ld->drawable)))
	{
	case PRECISION_U8:
	  {
	    /*  Set the overall value to defaults  */
	    guint8 * low_input = (guint8*)pixelrow_data (&ld->low_input_pr);
	    guint8 * high_input = (guint8*)pixelrow_data (&ld->high_input_pr);
	    guint8 * low_output = (guint8*)pixelrow_data (&ld->low_output_pr);
	    guint8 * high_output = (guint8*)pixelrow_data (&ld->high_output_pr);

	    low_input[channel]   = 0;
	    high_input[channel]  = 255;
	    low_output[channel]  = 0;
	    high_output[channel] = 255;
	  }
	  break;
	case PRECISION_U16:
	  {
	    /*  Set the overall value to defaults  */
	    guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
	    guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
	    guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
	    guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

	    low_input[channel]   = 0;
	    high_input[channel]  = 65535;
	    low_output[channel]  = 0;
	    high_output[channel] = 65535;
	  }
	  break;
	case PRECISION_FLOAT:
	  {
	    /*  Set the overall value to defaults  */
	    gfloat * low_input = (gfloat*)pixelrow_data (&ld->low_input_pr);
	    gfloat * high_input = (gfloat*)pixelrow_data (&ld->high_input_pr);
	    gfloat * low_output = (gfloat*)pixelrow_data (&ld->low_output_pr);
	    gfloat * high_output = (gfloat*)pixelrow_data (&ld->high_output_pr);

	    low_input[channel]   = 0;
	    high_input[channel]  = 1.0;
	    low_output[channel]  = 0;
	    high_output[channel] = 1.0;
	  }
	  break;
	case PRECISION_BFP:
	  {
	    /*  Set the overall value to defaults  */
	    guint16 * low_input = (guint16*)pixelrow_data (&ld->low_input_pr);
	    guint16 * high_input = (guint16*)pixelrow_data (&ld->high_input_pr);
	    guint16 * low_output = (guint16*)pixelrow_data (&ld->low_output_pr);
	    guint16 * high_output = (guint16*)pixelrow_data (&ld->high_output_pr);

	    low_input[channel]   = 0;
	    high_input[channel]  = 65535;
	    low_output[channel]  = 0;
	    high_output[channel] = 65535;
	  }
	  break;
	}
      

    }

    levels_update (ld, ALL);
    if (ld->preview)
      levels_preview (ld);

    if (single == TRUE) {
      channel = FALSE;
    } else {
      if (channel < HISTOGRAM_ALPHA) {
        channel++;
      } else {
        channel = FALSE;
      }
    }
  } while (channel);
}
#endif

static void
levels_ok_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  active_tool->preserve = TRUE;

  if (!ld->preview)
  {
    (*levels_calculate_transfers) (ld);
    image_map_apply (ld->image_map, levels, (void *) ld);
  }

  if (ld->image_map)
  {
    image_map_commit (ld->image_map);
    levels_free_transfers(ld);
  }

  active_tool->preserve = FALSE;

  ld->image_map = NULL;

  tools_select (RECT_SELECT);
}

static gint
levels_delete_callback (GtkWidget *w,
			GdkEvent *e,
			gpointer client_data)
{
  levels_cancel_callback (w, client_data);

  return TRUE;
}

static void
levels_destroy         (GtkWidget *widget,
			gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  if (ld->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (ld->image_map);
      active_tool->preserve = FALSE;
      levels_free_transfers(ld);
      gdisplays_flush ();
    }

  ld->image_map = NULL;

  if (ld->xform) cmsDeleteTransform (ld->xform);
  if (ld->bform) cmsDeleteTransform (ld->bform);
}

static void
levels_cancel_callback (GtkWidget *widget,
			gpointer   client_data)
{
  tools_select (RECT_SELECT);
}

static void
levels_preview_update (GtkWidget *w,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      ld->preview = TRUE;
      levels_preview (ld);
    }
  else
    ld->preview = FALSE;
}

static void
levels_low_input_text_update (GtkWidget *w,
			      gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_low_input_text_check) (ld, str))
    {
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_gamma_text_update (GtkWidget *w,
			  gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  gfloat value;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS (atof (str), 0.1, 10.0);

  if (value != ld->gamma[ld->channel])
    {
      ld->gamma[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_input_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  
  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_high_input_text_check)(ld, str))
    {
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_low_output_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_low_output_text_check) (ld, str))
    {
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_output_text_update (GtkWidget *w,
				gpointer   data)
{
  LevelsDialog *ld;
  char *str;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;

  if ( (*levels_high_output_text_check)(ld, str))
    {
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static gint
levels_input_da_events (GtkWidget    *widget,
			GdkEvent     *event,
			LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int x, distance;
  int i;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->input_levels_da[1])
	levels_update (ld, INPUT_SLIDERS);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 3; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      (*levels_input_da_setui_values) (ld, x);
      levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (ld, LOW_INPUT | GAMMA | DRAW);
	  break;
	case 1:  /*  gamma  */
	  levels_update (ld, GAMMA);
	  break;
	case 2:  /*  high input  */
	  levels_update (ld, HIGH_INPUT | GAMMA | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      (*levels_input_da_setui_values) (ld, x);
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | LOW_INPUT | GAMMA | DRAW);
	  break;

	case 1:  /*  gamma  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | GAMMA | DRAW);
	  break;

	case 2:  /*  high input  */
	  levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | HIGH_INPUT | GAMMA | DRAW);
	  break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
levels_output_da_events (GtkWidget    *widget,
			 GdkEvent     *event,
			 LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int x, distance;
  int i;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->output_levels_da[1])
	levels_update (ld, OUTPUT_SLIDERS);
      break;


    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 3; i < 5; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      (*levels_output_da_setui_values) (ld, x);	
      levels_update (ld, OUTPUT_SLIDERS | DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (ld, LOW_OUTPUT | DRAW);
	  break;
	case 4:  /*  high output  */
	  levels_update (ld, HIGH_OUTPUT | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      (*levels_output_da_setui_values) (ld, x);	
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (ld, OUTPUT_SLIDERS | LOW_OUTPUT | DRAW);
	  break;

	case 4:  /*  high output  */
	  levels_update (ld, OUTPUT_SLIDERS | HIGH_OUTPUT | DRAW);
	  break;
	}
      break;

    default:
      break;
    }

  return FALSE;
}


/*  The levels procedure definition  */
ProcArg levels_args[] =
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
    "channel",
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), GRAY (0) }"
  },
  { PDB_INT32,
    "low_input",
    "intensity of lowest input: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_input",
    "intensity of highest input: (0 <= high_input <= 255)"
  },
  { PDB_FLOAT,
    "gamma",
    "gamma correction factor: (0.1 <= gamma <= 10)"
  },
  { PDB_INT32,
    "low_output",
    "intensity of lowest output: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_output",
    "intensity of highest output: (0 <= high_input <= 255)"
  }
};

ProcRecord levels_proc =
{
  "gimp_levels",
  "Modifies intensity levels in the specified drawable",
  "This tool allows intensity levels in the specified drawable to be remapped according to a set of parameters.  The low/high input levels specify an initial mapping from the source intensities.  The gamma value determines how intensities between the low and high input intensities are interpolated.  A gamma value of 1.0 results in a linear interpolation.  Higher gamma values result in more high-level intensities.  Lower gamma values result in more low-level intensities.  The low/high output levels constrain the final intensity mapping--that is, no final intensity will be lower than the low output level and no final intensity will be higher than the high output level.  This tool is only valid on RGB color and grayscale images.  It will not operate on indexed drawables.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  levels_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { levels_invoker } },
};


static Argument *
levels_invoker (Argument *args)
{
#define FIXME
#if 0
  PixelArea src_area, dest_area;
  int success = TRUE;
  LevelsDialog ld;
  GImage *gimage;
  CanvasDrawable *drawable;
  int channel;
  int low_input;
  int high_input;
  gfloat gamma;
  int low_output;
  int high_output;
  int int_value;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  void *pr;

  drawable = NULL;
  low_input   = 0;
  high_input  = 0;
  gamma       = 1.0;
  low_output  = 0;
  high_output = 0;

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
   
  /*  channel  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    {
	      if (int_value != 0)
		success = FALSE;
	    }
	  else if (drawable_color (drawable))
	    {
	      if (int_value < 0 || int_value > 3)
		success = FALSE;
	    }
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  low input  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_input = int_value;
      else
	success = FALSE;
    }
  /*  high input  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_input = int_value;
      else
	success = FALSE;
    }
  /*  gamma  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= 0.1 && fp_value <= 10.0)
	gamma = fp_value;
      else
	success = FALSE;
    }
  /*  low output  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_output = int_value;
      else
	success = FALSE;
    }
  /*  high output  */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_output = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      for (i = 0; i < 5; i++)
	{
	  ld.low_input[i] = 0;
	  ld.gamma[i] = 1.0;
	  ld.high_input[i] = 255;
	  ld.low_output[i] = 0;
	  ld.high_output[i] = 255;
	}

      ld.channel = channel;
      ld.color = drawable_color (drawable);
      ld.low_input[channel] = low_input;
      ld.high_input[channel] = high_input;
      ld.gamma[channel] = gamma;
      ld.low_output[channel] = low_output;
      ld.high_output[channel] = high_output;

      /*  calculate the transfer arrays  */
      levels_calculate_transfers (&ld);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	levels (&src_area, &dest_area, (void *) &ld);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&levels_proc, success);
#endif
  return NULL;
}
