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
  CHANGE-LOG (started 2006/03):
  ============================
  2006/03  hartmut.sbosny@gmx.de  [hsbo]
    - Some functions renamed for more readability:
       clone_paint()      -> clone_paint_func()
       clone_draw()       -> clone_core_draw_func()
       clone_x_scale()    -> clone_x_scale_cb()  etc. for all Gtk callbacks
       clone_set_offset() -> clone_options_set_offset()
    - clone_arrow_keys_func() added for offset adjustment via arrow keys
    - clone_set_offset() added; five other fncts led back to it
    - Tracing (attempt to understand the crazy code), debug helpers, many
       comments included, some bug fixes,
    - all source *image* (re)settings outside of the INIT case of 
       clone_paint_func() removed
    - INIT-behaviour changed: if (bfm && bg set) clone at once!
    - draw_duplex_line() - draws a correct two-piece connection line (in offset
       mode) between source and destination spot if they lie in different windows.
       Though not yet applied because for ENTER, LEAVE and EXPOSE events extra
       work is needed to avoid weird extra lines due to imperfect draw_core_start(),
       .._pause(), .._resume(), .._stop() strategie in the general clone code.
*/
/**
  @file clone.c   
  
  @todo Complete rewriting  ;-)
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h> 
#include <gdk/gdkkeysyms.h>     /* -hsbo */

#include "config.h"
#include "libgimp/gimpintl.h"
#include "appenv.h"
#include "brush.h"
#include "brushlist.h"
#include "canvas.h"
#include "clone.h"
#include "draw_core.h"
#include "drawable.h"
#include "gimage.h"
#include "gdisplay.h"
#include "interface.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "patterns.h"
#include "pixelarea.h"
#include "procedural_db.h"
#include "rc.h"
#include "tools.h"
#include "transform_core.h"
#include "base_frame_manager.h"
#include "store_frame_manager.h" /* sfm_pos_of_ID() -hsbo */
#include "devices.h"
#include "gtk_debug_helpers.h"  /* -hsbo */

#ifdef WIN32
#include "extra.h"
#endif


#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15


/**  Define DEBUG_CLONE to get debugging and tracing output. Trace depth is
     adjustable via `trace_depth_'. Since we need stuff which is available
     only if DEBUG is defined, we tie DEBUG_CLONE to DEBUG. */
#ifdef DEBUG
#  define DEBUG_CLONE           /* undefine it if unwanted */
#  undef DEBUG_CLONE
#else
#  undef DEBUG_CLONE
#endif

#ifdef DEBUG_CLONE
#  define TRACE_MACROS_ON
#  define ENTRY              printf("%s:%d: %s()...\n",__FILE__,__LINE__,__func__);
#  define PRINTF(args)       {printf("%4d: ",__LINE__); printf args;}
#else
#  undef TRACE_MACROS_ON
#  define ENTRY
#  define PRINTF(args)
#endif

#include "trace_macros.h"   /* active or empty macros accord. TRACE_MACROS_ON */


typedef enum
{
  AlignYes,
  AlignNo,
  AlignRegister
} AlignType;

typedef struct CloneOptions CloneOptions;

/*  forward function declarations  */
static void *       clone_cursor_func        (PaintCore *, CanvasDrawable *, int state);
static void *       clone_paint_func         (PaintCore *, CanvasDrawable *, int state);
static void         clone_core_draw_func     (Tool *);
static void         clone_painthit_setup     (PaintCore *, Canvas *);
static void         clone_motion             (PaintCore *, CanvasDrawable *, double, double);
static int          clone_line_image         (PaintCore *, Canvas *, CanvasDrawable *,
                                               CanvasDrawable *, double, double);
static Argument *   clone_invoker            (Argument *);
static gint	    clone_x_offset_cb        (GtkWidget *, gpointer); 
static gint	    clone_y_offset_cb        (GtkWidget *, gpointer); 
static gint	    clone_x_scale_cb         (GtkWidget *, gpointer); 
static gint	    clone_y_scale_cb         (GtkWidget *, gpointer); 
static gint	    clone_rotate_cb          (GtkWidget *, gpointer); 
static void         clone_options_set_offset (CloneOptions*, int, int); 
/*  special arrow_keys function for the clone tool -hsbo: */
static void         clone_arrow_keys_func    (Tool *, GdkEventKey *, gpointer);
static void         clone_set_offset         (int, int);
static void         clone_draw_duplex_line   (GdkGC *, GDisplay *, GDisplay *);

/*  Declarations of debug helper functions -hsbo */
#ifdef DEBUG_CLONE
static const char*  paint_state_name         (int state);
static void         report_coords            (const PaintCore *);
static void         report_switches          ();
static void         report_drawables         (const CanvasDrawable *);
static void         report_active_tool       ();
static void         report_PaintCore         (const PaintCore *);
static const char*  any_not_atooldisp_paintcore_drawable (const GDisplay*, const PaintCore*,
                                              const CanvasDrawable*);
static const char*  any_not_inactive_srcset_alignYesNo (int inactive, int src_set, int alignYesNo);
static const char*  any_not_paintcore_srcset_srcdrawimg (const PaintCore*, int src_set, const GImage*);
void                report_winpos_of_gdisp   (GDisplay*);
#endif

extern int middle_mouse_button;
struct CloneOptions
{
  AlignType aligned;
  GtkWidget *offset_frame;
  GtkWidget *trans_frame; 

  GtkWidget *x_offset_entry;
  GtkWidget *y_offset_entry;
  GtkWidget *x_scale_entry;
  GtkWidget *y_scale_entry;
  GtkWidget *rotate_entry;
};
GtkTooltips *tool_tips;

/*  local variables  */
static int 	    src_set = FALSE;
static double       saved_x = 0;                /*                         */
static double       saved_y = 0;                /*  position of clone src  */
static double       src_x = 0;                  /*                         */
static double       src_y = 0;                  /*  position of clone src  */
static double       dest_x = 0;                 /*                         */
static double       dest_y = 0;                 /*  position of clone dst  */
static double       dest2_y = 0;                /*                         */
static double       dest2_x = 0;                /*  position of clone dst  */
static int          offset_x = 0;               /*                         */
static int          offset_y = 0;               /*  offset for cloning     */
static double       off_err_x = 0;              /*                         */
static double       off_err_y = 0;              /*  offset for cloning     */
static double	    error_x = 0;		/*			   */ 
static double	    error_y = 0;		/*  error in x and y	   */
static double       scale_x = 1;                /*                         */
static double       scale_y = 1;                /*  offset for cloning     */
static double       SCALE_X = 1;                /*                         */
static double       SCALE_Y = 1;                /*  offset for cloning     */
static double       rotate = 0;                 /*  offset for cloning     */
static char         first_down = TRUE;          /*  offset or cloning mode */
static char         first_mv = TRUE;            /*  ?                      */
static char         second_mv = TRUE;           /*  ?                      */
static double       trans_tx, trans_ty;         /*  transformed target     */
static double       dtrans_tx, dtrans_ty;       /*  transformed target     */
static int          src_gdisp_ID = -1;          /*  ID of source image     */
static CloneOptions *clone_options = NULL;
static char	    rm_cross = 0;               /*  ? (below never read)   */
static char	    expose = 0;

static CanvasDrawable *source_drawable;
static CanvasDrawable *cursor_drawable;         /*  see clone_cursor_func() */
static int          setup_successful;           /*  ?                      */
static char         clean_up = 0;               /*  ?                      */
static char         inactive_clean_up = 0;      /*  ?                      */

#if 0
static CanvasDrawable *non_gui_source_drawable;
static int          non_gui_offset_x;
static int          non_gui_offset_y;
#endif

/**
  Meaning of the global switches (attempt -hsbo):

  src_set and first_down:
   With CTRL + left mouse click the source is set (src_set=1, first_down=1).
   The next click w/out CTRL sets the offset (first_down=0). So `src_set &&
   first_down' indicates the "set the offset" mode (no cloning at the next
   click, draw a connection line) and 'src_set && !first_down' indicates the
   cloning mode (clone every time the left mouse button is pressed).
   
  first_mv, second_mv, clean_up, inactive_clean_up:  -  ?
   Seems to me workarounds only for an imperfect draw_core_start(),
   .._pause(), .._resume(), .._stop() strategie.
*/


static void
align_type_callback (GtkWidget *w,
		     gpointer  client_data)
{ENTRY
  clone_options->aligned = (AlignType) client_data;
  src_set = FALSE;
  switch (clone_options->aligned)
    {
    case AlignYes:
      gtk_widget_set_sensitive (clone_options->trans_frame, TRUE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, TRUE); 
      break;
    case AlignNo:
      gtk_widget_set_sensitive (clone_options->trans_frame, FALSE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, FALSE); 
      break;
    case AlignRegister:
      gtk_widget_set_sensitive (clone_options->trans_frame, FALSE); 
      gtk_widget_set_sensitive (clone_options->offset_frame, FALSE); 
      break;
    default:
      break;
    }
}

static CloneOptions *
create_clone_options (void)
{
  CloneOptions *options;
  GtkWidget *vbox, *hbox, *box;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *entry; 
  GSList *group = NULL;
  int i;
  char *align_names[3] =
  {
    "Aligned",
    "Non Aligned",
    "Registered",
  };
  char *tip_text = NULL;

ENTRY

  align_names[0] = _("Aligned");
  align_names[1] = _("Non Aligned");
  align_names[2] = _("Registered");

  /*  the new options structure  */
  options = (CloneOptions *) g_malloc_zero (sizeof (CloneOptions));
  options->aligned = AlignYes;

  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();

  if (!show_tool_tips)
    gtk_tooltips_disable (tool_tips);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Clone Tool Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  frame = gtk_frame_new (_("Alignment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  /*  the radio buttons  */
  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);

  group = NULL;  
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, align_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) align_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (frame);
  
  /* add the offset entry */
  tip_text = _("Use the arrow keys to move the background around and change the offset to the foreground.\nThe 'Shift' and 'Alt/Meta' keys modify the progress to 5 and 25 pixels.");
  options->offset_frame = frame = gtk_frame_new (_("Offset (x,y)"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);
  
  options->x_offset_entry = entry = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (0, -G_MAXFLOAT, G_MAXFLOAT, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
      (GtkSignalFunc) clone_x_offset_cb,
      options);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_offset_cb,
      options);
  gtk_tooltips_set_tip (tool_tips, entry, tip_text, NULL);
  gtk_widget_show (entry);
  
  options->y_offset_entry = entry = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (0, -G_MAXFLOAT, G_MAXFLOAT, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
      (GtkSignalFunc) clone_y_offset_cb,
      options);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_y_offset_cb,
      options);
  gtk_tooltips_set_tip (tool_tips, entry, tip_text, NULL);
  gtk_widget_show (entry);
  
  /* transformation */
  options->trans_frame = frame = gtk_frame_new (_("Transformation"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
 
  
  box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);
  
  /* rotate */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Rotate\t")); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->rotate_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "0"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_rotate_cb, options);
  gtk_widget_show (entry);
 
  /* scale */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Scale\t\t")); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_show (label);

  options->x_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_x_scale_cb,
      options);
  gtk_widget_show (entry);

  options->y_scale_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (GTK_WIDGET (entry), 40, 0);  
  gtk_entry_set_text (GTK_ENTRY (entry), "1"); 
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
      (GtkSignalFunc) clone_y_scale_cb, options);
  gtk_widget_show (entry);
  
  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (CLONE, vbox);

  return options;
}


Tool *
tools_new_clone ()
{
  Tool * tool;
  PaintCore * private;
ENTRY
  if (! clone_options)
    clone_options = create_clone_options ();

  tool = paint_core_new (CLONE);
  
  tool->arrow_keys_func = clone_arrow_keys_func; /* hsbo */

  private = (PaintCore *) tool->private;
  private->paint_func = (PaintFunc16) clone_paint_func;
  private->cursor_func = (PaintFunc16) clone_cursor_func;
  private->painthit_setup = clone_painthit_setup;
  private->core->draw_func = clone_core_draw_func;

  src_set = FALSE;
  return tool;
}

/**
*   clone_cursor_func()
*
*   Called [by the PaintCore machinery] when mouse cursor is moving inside the
*       active_tool display and no button is pressed (== tool is inactive); 
*       also "now and then" during pressed button movement (tool is active);
*       further after a keystroke (e.g. arrow keys).
*   Task: Manage drawing of source marker and connection line. 
*       (Quits without action if tool is not INACTIVE (but ACTIVE || PAUSED).
*       So probably only updating of the inactive tool cursor is the plan.)
*
*   @param paint_core: ...
*   @param drawable: the drawable under the cursor (==destination); can be != 
*       source_drawable! We store it in the file-global `cursor_drawable' to 
*       inform clone_core_draw_func().
*   @param state: not used
*/
static void *
clone_cursor_func (PaintCore *      paint_core,
                   CanvasDrawable * drawable,
                   int              state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  double x1, y1, x2, y2, dist_x, dist_y; 
# ifdef DEBUG_CLONE  
  static int counter;
# endif  
  
  /*TRACE_ENTER(("clone_CURSOR_func"));*/
  TRACE_BEGIN(("clone_CURSOR_func( %d: %s ) - %d", state, paint_state_name(state), counter++ ));
    
  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  TRACE_PRINTF(("[ atool: %s dispID=%d (bfm=%d, bg=%d, fg=%d) ]",
      tool_state_name(active_tool->state), 
      gdisp ? gdisp->ID : -1, bfm_check(gdisp),
      bfm_get_bg_ID(gdisp), bfm_get_fg_ID(gdisp) ));
  
  /*printf("%4d:\tcur = (%.0f, %.0f)\n",__LINE__, paint_core->curx, paint_core->cury);*/
  /*printf("%4d:\tdrawableID=%d,  cur=(%.0f, %.0f)\n",__LINE__,drawable ? drawable->ID : -1,
      paint_core->curx, paint_core->cury);*/
  /*report_PaintCore (paint_core);*/

  if (middle_mouse_button || active_tool->state == ACTIVE || active_tool->state == PAUSED)
    {
      TRACE_PRINTF(("Return - middle_mouse || atool=(ACTIVE || PAUSED)"));
      TRACE_EXIT;
      return NULL;
    }
  if (!gdisp || !paint_core || !drawable)
    {
      src_set = 0; 
      TRACE_PRINTF(("Return - %s (Set src_set=0)", 
          any_not_atooldisp_paintcore_drawable (gdisp,paint_core,drawable)));
      TRACE_EXIT;
      return NULL;
    }

  cursor_drawable = drawable;   /* to inform clone_core_draw_func() */
  
  x1 = paint_core->curx;
  y1 = paint_core->cury;

  if (active_tool->state == INACTIVE && src_set && first_down 
      && (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo))
    {
      /*  src_set && first_down - movement during offset mode */
      /*TRACE_BEGIN(("<atool=INACTIVE && src_set && first_down && AlignYesNo>"));*/
      
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);    /* get disp from ID */
      /*  If gdisp has bfm && bg set && srcID==bg && gdisp!=bg, then the src
          disp lies under the dest display and gdisplay(srcID) gave 0. We draw
          the src markers then into the dest display (==gdisp). Note: The
          pitfall "srcID==bgID==-1" should be excluded by src_set==TRUE. */
      if (!src_gdisp && bfm_get_bg_ID(gdisp) == src_gdisp_ID)
        {
          src_gdisp = gdisp;
        } 
      
      if (src_gdisp && !first_mv)
	{ 
          TRACE_BEGIN(("<!first_mv>"));
          /*TRACE_PRINTF(("<!first_mv> - draw_core_pause()..."));*/
	  draw_core_pause (paint_core->core, active_tool);
          gdisplay_transform_coords_f (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (src_gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
          /*TRACE_PRINTF(("<!first_mv> - draw_core_resume()..."));*/
	  draw_core_resume (paint_core->core, active_tool);
          TRACE_END;
	}
      else if (src_gdisp && first_mv)
	{
          TRACE_BEGIN(("<first_mv> - Set first_mv=0"));
	  gdisplay_transform_coords_f (src_gdisp, saved_x, saved_y, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, x1, y1, &dtrans_tx, &dtrans_ty, 1);
          /*TRACE_PRINTF(("<first_mv> - draw_core_start()... and set first_mv=FALSE"));*/
	  draw_core_start (paint_core->core, src_gdisp->canvas->window, active_tool);
	  first_mv = FALSE;          /** the only place where set FALSE */
          /*PRINTF(("Cursor: Set first_mv = 0 ***\n"));*/
          TRACE_END;
	}
      else 
          TRACE_PRINTF(("<!src_disp> - draw nothing"));
      
      /*TRACE_END;*/
    }
  else 
  if (active_tool->state == INACTIVE && src_set && !first_down && 
      (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo)) 
    {
      /*  src_set && !first_down - inactive movement during cloning mode */
      /*TRACE_BEGIN(("<atool=INACTIVE && src_set && *!*first_down && AlignYesNo>"));*/
      if (clone_options->aligned == AlignYes)
	{
	  dist_x = ((x1 - dest2_x) * cos (rotate)) -
	           ((y1 - dest2_y) * sin (rotate));
	  dist_y = ((x1 - dest2_x) * sin (rotate)) + 
	           ((y1 - dest2_y) * cos (rotate));
	  
	  x2 = dest2_x + ((dist_x) * SCALE_X) + offset_x - off_err_x + off_err_x*SCALE_X;
	  y2 = dest2_y + ((dist_y) * SCALE_Y) + offset_y - off_err_y + off_err_y*SCALE_Y;
	}
      else
	{
	  x2 = saved_x;
	  y2 = saved_y;
	}
      
      src_gdisp = gdisplay_get_ID (src_gdisp_ID);   /* get disp from ID */
      /*  Comment see above */
      if (!src_gdisp && bfm_get_bg_ID(gdisp) == src_gdisp_ID)
        {
          src_gdisp = gdisp;
        } 
      
      if (src_gdisp && second_mv)
	{
          TRACE_BEGIN(("<second_mv>"));
          /*TRACE_PRINTF(("draw_core_pause()..."));*/
	  draw_core_pause (paint_core->core, active_tool);
	  gdisplay_transform_coords_f (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
          /*TRACE_PRINTF(("draw_core_resume()..."));*/
	  draw_core_resume (paint_core->core, active_tool);
          TRACE_END;
	}
      else if (src_gdisp && !second_mv)
	{
          TRACE_BEGIN(("<!second_mv> - Set second_mv=1"));
	  gdisplay_transform_coords_f (src_gdisp, x2, y2, &trans_tx, &trans_ty, 1);
	  gdisplay_transform_coords_f (gdisp, dest_x, dest_y, &dtrans_tx, &dtrans_ty, 1);
          /*TRACE_PRINTF(("draw_core_start()... and Set second_mv=TRUE)"));*/
	  draw_core_start (paint_core->core, src_gdisp->canvas->window, active_tool);
	  second_mv = TRUE;
          /*PRINTF(("Cursor: Set second_mv = 1\n"));*/
          TRACE_END;
	}
      else 
          TRACE_PRINTF(("<!src_disp> - draw nothing"));
      
      /*TRACE_END;*/
    }
  else 
      TRACE_PRINTF(("Do nothing - %s", any_not_inactive_srcset_alignYesNo (
         active_tool->state == INACTIVE, src_set,
         (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo)) ));

  /*PRINTF(("cur=(%.0f, %.0f)  P2=(%.0f, %.0f)  dest=(%.0f, %.0f)\n", x1,y1, x2,y2, dest_x,dest_y));
  PRINTF(("trans=(%.0f, %.0f)  dtrans=(%.0f, %.0f)\n", trans_tx,trans_ty, dtrans_tx,dtrans_ty));*/
  
  TRACE_EXIT;       
  return NULL; 
}

/**
*   clone_paint_func()  -  die Schreckliche
*
*   Called [by the PaintCore machinery], if a mouse click on the active_tool
*      display occured and as long in a following movement the mouse button
*      keeps pressed (== tool is active). Each click generates the call
*      sequence INIT-MOTION-...-FINISH with at least one MOTION call.
*   Task: ...
*      No cloning at INIT, there set only the source target. The cloning is 
*      done during the MOTION calls.
*        
*   @param paint_core: ...
*   @param drawable: drawable which got the mouse click
*   @param state: INIT_PAINT, MOTION_PAINT or FINISH_PAINT
*
*   If the active_tool display has a bfm, it becomes complex. We do this: 
*
*   For INIT + Ctrl: If bg was set, we set src_ID=bg. If no bg was set, we set
*    bg=fg and src_ID=bg. So after a Ctrl-click on a bfm-display always a bg is
*    set and is the source. And we are in the "offset mode" then as ordinary.
*
*   For INIT && !Ctrl we can distinguish 8 situations (apart from, that
*    "!src_set && !first_down" should not happend):
*    src_set  first_down  bg
*      0         0        0    -> msg 
*      0         0        1    Set src=bg and clone at once
*      0         1        0    -> msg
*      0         1        1    Set src=bg and clone at once
*      1         0        0    cloning mode, source is extern (non-bfm)
*      1         0        1    cloning mode, source can be bg or extern
*      1         1        0    offset mode, source is extern (non-bfm)
*      1         1        1    offset mode, source can be bg or extern
*
*   Madness: 'paint_core' is the painting core of a Tool *argument*, namely of
*    paint_core_16_button_press(Tool* tool), which calls our fnct with tool's
*    paint_core. But here this is mixed with the *global* `active_tool' in
*    calls like `draw_core_start(paint_core->core, active_tool)'. Seemingly
*    is supposed that active_tool->private == paint_core.
*/
static void *
clone_paint_func (PaintCore *paint_core,
                  CanvasDrawable *drawable,
                  int state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  double x1, y1, x2, y2; 
  double dist_x, dist_y;
  int dont_clone = 0;    /* to prevent cloning in offset mode */
# ifdef DEBUG_CLONE  
  static int counter;
# endif  
  
  /*  Only for debugging of active_tool->private->core */
  active_draw_core = ((PaintCore*)active_tool->private)->core;
  
  TRACE_BEGIN(("clone_PAINT_func( %s ) - %d", paint_state_name(state), counter++ ));
  
  gdisp = (GDisplay *) active_tool->gdisp_ptr;
  
  TRACE_PRINTF(("[ atool: %s dispID=%d (bfm=%d, bg=%d, fg=%d) ]",
      tool_state_name(active_tool->state), 
      gdisp ? gdisp->ID : -1, bfm_check(gdisp),
      bfm_get_bg_ID(gdisp), bfm_get_fg_ID(gdisp) ));
      
  cursor_drawable = drawable;   /* to be conseqent */
  
  /*  Reset, if no source drawable or the source image no more alive */
  if (!source_drawable || !drawable_gimage (source_drawable)) 
    { 
      /*TRACE_BEGIN(("<!src_drwble || !gimage(src_drwble)>"));*/
      src_set = 0;
      first_down = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      clean_up = 0;
      rm_cross = 0;
      /*PRINTF(("Paint: Reset !src: ")); report_switches();*/
      /*TRACE_END;*/
    }

  /*  If src_set && display has bfm && src \in bfm, BUT (no bg set || bgID !=
      srcID), then reset for "no source", because bg was [probably] unset or
      changed via Flipbook menu. Nicer would be a unset_bg() callback */
  if (src_set && bfm(gdisp) && sfm_pos_of_ID(gdisp, src_gdisp_ID) != -1 
      && (!bfm_get_bg(gdisp) || (bfm_get_bg_ID(gdisp) != src_gdisp_ID)))
    {
      printf("Reset !src - src was in bfm, but (!bg || bgID!=srcID)!\n");
      src_set = 0;
      first_down = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      clean_up = 0;
      rm_cross = 0;
    }  
      
  switch (state)
    {
    case INIT_PAINT :                   
      TRACE_BEGIN(("case INIT_PAINT"));
      
      if (bfm (gdisp))          /* atool->disp has frame manager */
        {
          /*TRACE_BEGIN(("<atool->disp has FrameManager>"));
          TRACE_PRINTF(("bg=%d  disp=%p,  fg=%d  disp=%p", 
              bfm_get_bg_ID(gdisp), gdisplay_get_ID(bfm_get_bg_ID(gdisp)),
              bfm_get_fg_ID(gdisp), gdisplay_get_ID(bfm_get_fg_ID(gdisp)) ));*/
          if ((paint_core->state & GDK_CONTROL_MASK))
            {
              /*TRACE_BEGIN(("<Ctrl> - set source"));*/
              /*  Set source = bg (if !bg, set bg=fg before) */
              src_gdisp_ID = bfm_getset_bg_ID (gdisp); /* sets bg=fg, if !bg */
              source_drawable = bfm_get_bg_drawable (gdisp);
              saved_x = src_x = paint_core->curx;
              saved_y = src_y = paint_core->cury;
              src_set = TRUE;
              first_down = TRUE;
              first_mv = TRUE;
              clean_up = 0;
              /*PRINTF(("Paint: Reset src: ")); report_switches();
              TRACE_PRINTF(("Got bg's src_ID=%d, src_drwbleID=%d", src_gdisp_ID,
                  source_drawable ? source_drawable->ID : -1));
              TRACE_END;*/
            }
          else if (!src_set && !gdisp->bfm->bg)   /* no Ctrl, no src, no bg */
            { 
              /*TRACE_PRINTF(("<!Ctrl && !src && !bg> - msg..."));*/
              g_message ("Set source clone point with CTRL + left mouse button\nor set Bg in Flipbook.");
              TRACE_END; /*TRACE_END;*/     /* we skip two BEGINs by 'return' */
              TRACE_EXIT;
              return NULL;
              break;
            }
          else if (!src_set && gdisp->bfm->bg)    /* !Ctrl, !src, bg is set */
            {
              /*  Set src = bg and start cloning at once (in the next MOTION
                  call) (no offset phase) */
              /*TRACE_BEGIN(("<!Ctrl && !src && bg set> - Set src and clone at once"));*/
              src_gdisp_ID = bfm_get_bg_ID (gdisp);
              source_drawable = bfm_get_bg_drawable (gdisp);
              saved_x = src_x = paint_core->curx;
              saved_y = src_y = paint_core->cury;
              src_set = TRUE;
              first_down = FALSE;  /* skip offset phase! */
              first_mv = TRUE;
              clean_up = 0;
              /*TRACE_PRINTF(("Got bg's srcID=%d, src_drwbleID=%d", src_gdisp_ID,
                  source_drawable ? source_drawable->ID : -1)); 
              TRACE_END;*/
            }
          else if (src_set && first_down)       /* offset mode (bg || !bg) */
            { 
              TRACE_PRINTF(("<src_set && first_down> - offset mode"));
              inactive_clean_up = 1;
            }
          else /* src_set && !first_down */     /* cloning mode (bg || !bg) */
            {
              /*TRACE_BEGIN(("<else == src_set && !first_down> - clone mode"));*/
              /*  copied from "!bfm && AlignYes" without verve */
              inactive_clean_up = 0;                /* ? */
              src_x = offset_x + paint_core->curx;
              src_y = offset_y + paint_core->cury; 
              /*TRACE_END;*/
            }    
          /*TRACE_END;*/
        }
      else if (paint_core->state & GDK_CONTROL_MASK)     /* no bfm, Ctrl */
        {
          /*  Ctrl -- Set source and source location */
          /*TRACE_BEGIN(("<!bfm && Ctrl>"));*/
          
          if (!first_down)   /* Ctrl click during cloning mode */
          {
            TRACE_PRINTF(("<!first_down> - draw_core_pause()..."));
            draw_core_pause (paint_core->core, active_tool);  /* Why? */
            /*TRACE_END;*/
          }
          src_gdisp_ID = gdisp->ID;                /* = atool->dispID */
          source_drawable = drawable;              /* = the fnct arg */
          saved_x = src_x = paint_core->curx;
          saved_y = src_y = paint_core->cury;
          src_set = TRUE;
          first_down = TRUE;
          first_mv = TRUE;
          clean_up = 0;
          /*TRACE_PRINTF(("Set source: ID=%d, drawableID=%d", src_gdisp_ID, drawable ? drawable->ID :-1));
          PRINTF(("Paint: Reset src: ")); report_switches();*/
          /*TRACE_END;*/
        }
      else if (!src_set)                /* no bfm, no Ctrl, no src set */        
        {
          /*TRACE_PRINTF(("<!bfm && !Ctrl && !src_set> - msg..."));*/
          g_message ("Set source clone point with CTRL + left mouse button.");
          TRACE_END; TRACE_EXIT;        /* we skip one BEGIN by `return' */
          return NULL;
          break;
        }
      else   /* no bfm, no Ctrl, src set -- click in offset or cloning mode */
        {
          /*TRACE_BEGIN(("<!bfm && !Ctrl && src_set> - offset or clone mode"));*/
          if (clone_options->aligned == AlignYes) 
            {
              inactive_clean_up = 1;                  /* Verstehen! */
              /*PRINTF(("Paint: Set inactive_clean_up = 1\n"));*/
            }
          /*  if !first_down, we are in the cloning mode */  
          if (!first_down && clone_options->aligned == AlignYes)
            {
              /*TRACE_BEGIN(("<!first_down && AlignYes>"));*/
              if (offset_x + paint_core->curx == src_x &&   /* cur-pos == old src-pos */
                  offset_y + paint_core->cury == src_y)
                inactive_clean_up = 0;              /* ??? */  
              inactive_clean_up = 0;                /* ??? */
              /*  find source pos from offset and curr.pos */
              src_x = offset_x + paint_core->curx;  
              src_y = offset_y + paint_core->cury; 
              /*PRINTF(("Paint: Set inactive_clean_up = 0\n"));*/
              /*TRACE_END;*/
            }
          else if (clone_options->aligned == AlignNo)
            {
              /*TRACE_BEGIN(("<AlignNo>"));*/
              /*  fixed source pos */
              src_x = saved_x;
              src_y = saved_y;
              first_down = TRUE;   /* for AlignNo always offset mode? */
              /*PRINTF(("Paint: Set first_down = 1\n"));*/
              /*TRACE_END;*/
            }
          /*PRINTF(("src=(%.0f, %.0f)\n", src_x, src_y));*/
          /*TRACE_END;*/
        }
      TRACE_END;
      break;
    
    case MOTION_PAINT :
      TRACE_BEGIN(("case MOTION_PAINT"));
      
      x1 = paint_core->curx;
      y1 = paint_core->cury;
      x2 = paint_core->lastx;
      y2 = paint_core->lasty;

      /*  If the control key is down, move the source pos and return. */
      if (paint_core->state & GDK_CONTROL_MASK)
	{
          /*TRACE_BEGIN(("<Ctrl>"));
          TRACE_PRINTF(("move the src pos and return"));*/
	  saved_x = src_x = x1;
	  saved_y = src_y = y1;
	  first_down = TRUE;           /* a new offset phase begins */
	  src_set = TRUE;
          /*PRINTF(("Paint: Set first_down = 1\n"));*/
          /* + before 'break': second_mv=FALSE and draw_core_pause() */
          /*TRACE_END;*/
	}
      /*  otherwise, update the dest target  */
      else if (src_set)
	{
          /*TRACE_BEGIN(("<!Ctrl && src_set> - update the dest target"));*/
	  dest_x = x1;
	  dest_y = y1;

          /*  If first_down, we are in the offset mode - this Click determines
              the offset and finishes the offset phase. */
	  if (clone_options->aligned == AlignRegister)
	    {
	      offset_x = 0;
	      offset_y = 0;
	      off_err_x = 0;
	      off_err_y = 0;
	    }
	  else if (first_down && clone_options->aligned == AlignNo)
	    {
	      offset_x = saved_x - dest_x;     /* offset is the diff */
	      offset_y = saved_y - dest_y;
	      off_err_x = dest_x - (int)dest_x;
	      off_err_y = dest_y - (int)dest_y;
	      dont_clone = 1;              /* don't clone in offset mode */
	      first_down = FALSE;          /* offset mode ended */
              /*PRINTF(("Paint: Set first_down=0, dont_clone=1\n"));*/
	    }
	  else if (first_down && clone_options->aligned == AlignYes)
	    {
	      dest2_x = dest_x;                /* Verstehen! */
	      dest2_y = dest_y;
	      if (bfm_onionskin (gdisp))
		{
		  offset_x += saved_x - dest_x;  /* add diff to offset - why? */
		  offset_y += saved_y - dest_y;
		  bfm_onionskin_set_offset (gdisp, offset_x, offset_y); 
		}
	      else
		{
		  offset_x = saved_x - dest_x;    /* offset is the diff */
		  offset_y = saved_y - dest_y;
		  off_err_x = dest_x - (int)dest_x;
		  off_err_y = dest_y - (int)dest_y;
		}
	      clone_options_set_offset (clone_options, offset_x, offset_y); 
	      dont_clone = 1;              /* don't clone in offset mode */
	      first_down = FALSE;          /* offset mode ended */
	    }

	  dist_x = ((dest_x - dest2_x) * cos (rotate)) -
	           ((dest_y - dest2_y) * sin (rotate));
	  dist_y = ((dest_x - dest2_x) * sin (rotate)) + 
	           ((dest_y - dest2_y) * cos (rotate));
	 
	  src_x = ((dest2_x + (dist_x) * SCALE_X) + offset_x) - off_err_x + off_err_x*SCALE_X;
	  src_y = ((dest2_y + (dist_y) * SCALE_Y) + offset_y) - off_err_y + off_err_y*SCALE_Y;

	  
          if (scale_x >= 1)
	    {
	      double tmp;
	      tmp = ((int)dest_x - (int)dest2_x);
	      tmp *= SCALE_X;
	      error_x = scale_x >= 1 ?  (tmp-(int)tmp) * scale_x: 0; 
	    }
	  if (scale_y >= 1)
	    {
	      double tmp;
	      tmp = ((int)dest_y - (int)dest2_y);
	      tmp *= SCALE_Y;
	      error_y = scale_y >= 1 ?  (tmp-(int)tmp) * scale_y: 0;
	    }
	  if (scale_x < 1)
	    {
	      double tmp;
	      tmp = (dest_x - (int)dest_x);
	      tmp *= SCALE_X;
	      error_x = scale_x < 1 ?  (int)tmp: 0; 
	      error_x = error_x < 0 ? scale_x + error_x : error_x;
	    }
	  if (scale_y < 1)
	    {
	      double tmp;
	      tmp = (dest_y - (int)dest_y);
	      tmp *= SCALE_Y;
	      error_y = scale_y < 1 ?  (int)tmp: 0;
	      error_y = error_y < 0 ? scale_y + error_y : error_y;
	    }

	  if (scale_x < 1 && (int)SCALE_X != SCALE_X)
	    {
	      if ((dest_x - dest2_x) - 
		  ((dest_x - dest2_x)/(int)(SCALE_X))*(int)(SCALE_X) != 0)
		{
                  printf("%s:%d: *** Return - SCALE_X\n",__func__,__LINE__);
		  return NULL;
		}
	    }
	  if (scale_y < 1 && (int)SCALE_Y != SCALE_Y)
	    {
	      if ((dest_y - dest2_y) - 
		  ((dest_y - dest2_y)/(int)(SCALE_X))*(int)(SCALE_X) != 0)
		{
                  printf("%s:%d: *** Return - SCALE_Y\n",__func__,__LINE__);
		  return NULL; 
		}
	    }
            
	  /*  Do the cloning */ 
          if (!dont_clone)
            {
              TRACE_BEGIN(("<!dont_clone> - clone_motion()..."));
	      clone_motion (paint_core, drawable, src_x, src_y); /* x,y unused */
              TRACE_END;
            }
          /*TRACE_END;*/
	}
      
      second_mv = FALSE;                                /* Verstehen */
      if (src_set) 
	{
          TRACE_PRINTF(("<src_set> - draw_core_pause()..."));
	  draw_core_pause (paint_core->core, active_tool);     /* Why? */
	}
      TRACE_END;  
      break;

    case FINISH_PAINT :
      TRACE_PRINTF(("case FINISH_PAINT - draw_core_stop()..."));
      draw_core_stop (paint_core->core, active_tool);
      TRACE_EXIT;
      return NULL;
      
    default :
      TRACE_PRINTF(("** unhandled case: state=%d **", state)); 
      break;
    }           /*  switch(state) */

  /*  Calculate the coordinates of the target  */
  
  src_gdisp = gdisplay_get_ID (src_gdisp_ID);  /* get disp from ID */
  /*  If gdisp has bfm && bg set && srcID==bg && gdisp(==fg)!=bg, then the src
      display lies under the dest display and gdisplay(srcID) gave 0. We draw 
      the src markers then into the dest display (==gdisp).     */
  if (!src_gdisp && src_set && bfm_get_bg_ID(gdisp) == src_gdisp_ID)
    {
      src_gdisp = gdisp;        /* take atools's display */
    } 
  if (!src_gdisp)
    {
      TRACE_PRINTF(("Return - !src_disp"));
      TRACE_EXIT;
      return NULL;
    }
  
  /*  Find the target cursor's location onscreen  */
  gdisplay_transform_coords_f (src_gdisp, src_x, src_y, &trans_tx, &trans_ty, 1);

  if (state == INIT_PAINT && src_set)  /* src_set -- offset or cloning mode */
    {
      /*TRACE_BEGIN(("<INIT_PAINT && src_set>"));*/
      /*  Initialize the tool drawing core  */
      if (inactive_clean_up)                    /* Intention? */
	{
          TRACE_BEGIN(("<inactive_clean_up>"));
	  gdisplay_transform_coords_f (src_gdisp, paint_core->curx, paint_core->cury, 
	      &dtrans_tx, &dtrans_ty, 1);
	  
          /*TRACE_PRINTF(("draw_core_pause()..."));*/
	  draw_core_pause (paint_core->core, active_tool);
          
	  inactive_clean_up = 0; 
          /*PRINTF(("Paint: Set inactive_clean_up = 0\n"));*/
	/*
	  draw_core_resume (paint_core->core, active_tool);
	  draw_core_stop (paint_core->core, active_tool);
	*/
          TRACE_END;
	}

      /** HACK -hsbo: If [paint_core->core->]draw_state!=INVISIBLE, then draw_core_start()
           calls at the very first draw_core_stop() and this calls `core->draw_func(tool)', 
           where 'core' and 'tool' are the draw_core_start() args `paint_core->core' and 
           `active_tool'. For the first Ctrl-click on a drawable the value `active_tool->win'
           contains the *previous* drawable; if this was closed, the call is invalid and
           CinePaint can crash. As work-around I set then already here draw_state=INVISIBLE,
           so that draw_core_stop() isn't called by draw_core_start(). Something better? 
           Probably somewhere a draw_core_stop() is missing for the previous drawable. */
      if (src_gdisp->canvas->window != ((PaintCore*)active_tool->private)->core->win 
          && paint_core->core->draw_state != INVISIBLE) 
        {
          printf("*** Hack ***: previous atool->win=%p [%s:%d]\n",
              (void*)((PaintCore*)active_tool->private)->core->win, __func__,__LINE__);
          paint_core->core->draw_state = INVISIBLE;  /* HACK */
        }
      
      TRACE_PRINTF(("draw_core_start()..."));
      draw_core_start (paint_core->core,
	  src_gdisp->canvas->window,
	  active_tool);
      /*TRACE_END;*/
    }
  else if (state == MOTION_PAINT && src_set)
    {
      TRACE_PRINTF(("<MOTION_PAINT && src_set> - draw_core_resume()..."));
      draw_core_resume (paint_core->core, active_tool);
    }
  TRACE_EXIT;  
  return NULL;
}


/**
*   clone_draw_duplex_line()  -  helper of clone_core_draw_func() 
*
*   Called if source_drwble != cursor_drwble. Draws connection lines in both
*    windows considering the positions of the windows on screen. The coords
*    are read from the globals `trans_tx|y' and `dtrans_tx|y'.
*   
*   @param gc: GC as given by tool->paint_core->core->gc
*   @param src_gdisp: source display
*   @param cur_gdisp: cursor display (dest display)
*/
static void 
clone_draw_duplex_line (GdkGC* gc, 
                        GDisplay* src_gdisp,    /* source display */
                        GDisplay* cur_gdisp )   /* cursor display */
{                 
  int src_win_x, src_win_y;  /* pos of source window on screen */
  int cur_win_x, cur_win_y;  /* pos of cursor window on screen */
  int dest_x, dest_y; 
  int src_x, src_y;
  
  if (!src_gdisp || !cur_gdisp) 
    return;
                      
  /*  Get positions of source and of cursor window on screen */
  gdk_window_get_origin (src_gdisp->canvas->window, &src_win_x, &src_win_y);
  gdk_window_get_origin (cur_gdisp->canvas->window, &cur_win_x, &cur_win_y);
                      
  /*  relative pos of dest point as seen from the src window */
  dest_x = CAST(gint) dtrans_tx + cur_win_x - src_win_x;  
  dest_y = CAST(gint) dtrans_ty + cur_win_y - src_win_y;
  
  /*  draw the line in the source window */   
  gdk_draw_line (src_gdisp->canvas->window, gc, 
      CAST(gint) trans_tx, CAST(gint) trans_ty, dest_x, dest_y); 
  
  /*  relative pos of src point as seen from the cursor window */
  src_x  = CAST(gint) trans_tx + src_win_x - cur_win_x;
  src_y  = CAST(gint) trans_ty + src_win_y - cur_win_y;
  
  /*  draw the line in the cursor window */    
  gdk_draw_line (cur_gdisp->canvas->window, gc,
      src_x, src_y, CAST(gint) dtrans_tx, CAST(gint) dtrans_ty);
}


/**
*   clone_core_draw_func()
*
*   Task: Draw source marker and (in the offset mode) connection line for given
*       coords (given via the file-globals trans_tx|y, dtrans_tx|y). 
*   Called by: core_draw_start(), ..._pause(), ..._resume(), ..., mediately 
*       by active_tool_control(), ...
*
*   Note: tool->drawable seems to be always that drawable, the tool was first 
*    initialized for, independently, whether source or/and dest drawable got
*    changed later. Useless.
*
*   FIXME In offset mode no src marker visible for bfm w/out onionskin (visible
*    only the connection line). Hoppla, back to the old inactive_cleanup is now
*    the line missing and the source marker flickers. 
*/
static void
clone_core_draw_func (Tool * tool)
{
  static int first=1;
  PaintCore * paint_core;       /* tool's PaintCore */
  GImage * src_gimage;
  
  TRACE_ENTER(("clone_core_DRAW_func"));
  
  if (!tool)
    {
      return;
    }
  paint_core = (PaintCore *) tool->private;
  
#if 0  
  TRACE_PRINTF(("[ tool=%s - %s - dispID=%d ]  (disp_activeID=%d, first=%d)", 
      tool_type_name(tool), tool_state_name(tool->state),
      tool->gdisp_ptr ? ((GDisplay*)tool->gdisp_ptr)->ID : -1,  
      gdisplay_active() ? gdisplay_active()->ID : -1, first ));
  TRACE_PRINTF(("[ tool->disp: bfm=%d, bg=%d, fg=%d ]", 
      bfm_check((GDisplay*)tool->gdisp_ptr),
      bfm_get_bg_ID((GDisplay*)tool->gdisp_ptr), 
      bfm_get_fg_ID((GDisplay*)tool->gdisp_ptr) ));
  TRACE_PRINTF(("[ source: set=%d, ID=%d, draw=%p, drawID=%d,  bfm=%d ]", 
      src_set, src_gdisp_ID, (void*)source_drawable,
      source_drawable ? source_drawable->ID : -1, 
      source_drawable ? bfm_check(gdisplay_get_ID(source_drawable->gimage_ID)) : 0 ));
#endif

  if (expose)
    {
      expose = 0;
      TRACE_PRINTF(("Return - expose (Set expose=0)"));
      TRACE_EXIT;
      return;   /* Why? */
    }
  if (!gdisplay_active ())
    {
      TRACE_PRINTF(("Return - no active display"));
      TRACE_EXIT;
      return; 
    }  

  if (middle_mouse_button)
    {
      if (!first)
        {
          TRACE_PRINTF(("Return - middle_mouse && !first"));
          TRACE_EXIT;
	  return;
        }  
      first = 0;
    }
  else
    {
      if (!first)
	{
	  first = 1;
          TRACE_PRINTF(("Return - !middle_mouse && !first (Set first=1)"));
          TRACE_EXIT;
	  return;
	}
    }
  
  /*  We do nothing if no paint_core or no src_set. And we make sure, that our 
      source image is still alive */
  if (paint_core && src_set && (src_gimage = drawable_gimage (source_drawable)))
    {
      static int radius;
      GDisplay *gdisp, *src_gdisp;
      
      /*TRACE_BEGIN(("<paintcore && src_set && src_image>"));*/
      gdisp = (GDisplay*) tool->gdisp_ptr;
      src_gdisp = gdisplay_get_ID (src_gimage->ID);
      /*  If tool->disp has bfm && bg set && srcID==bg && tool->disp(==fg)!=bg,
          then the src lies under the dest display and gdisplay(srcID) gave 0. 
          We draw the src markers then into the tool's (==dest) display. */
      if (!src_gdisp && bfm_get_bg_ID(gdisp) == src_gimage->ID)
        {
          src_gdisp = gdisp;        /* take tools's display */
        } 
      if (!src_gdisp)
        {
          TRACE_PRINTF(("Return - !src_gdisp"));
          /*TRACE_END;*/ TRACE_EXIT;
          return;
        }
      /*  If onionskin, don't draw source marker during *cloning mode*
          (is questionable, if the dest is extern, i.e not in that bfm) */
      if (!first_down && bfm_onionskin (src_gdisp))
        {
          TRACE_PRINTF(("Return - !first_down && onionskin"));
          /*TRACE_END;*/ TRACE_EXIT;
	  return;
        }
      if (get_active_brush() && get_active_brush()->mask)
	{
          radius = (canvas_width (get_active_brush()->mask) > canvas_height (get_active_brush()->mask) ?
	      canvas_width (get_active_brush()->mask) : canvas_height (get_active_brush()->mask))* 
	    ((double)SCALEDEST (src_gdisp) / 
	     (double)SCALESRC (src_gdisp));
          
          TRACE_PRINTF(("Radius=%d", radius));
	}
      else 
	{
	  radius = 0;
          TRACE_PRINTF(("!<active_brush && ->mask> - Set radius=0  000000000000"));
	}
    /*
      printf ("cursor_drawable = %p  ID=%d\n", (void*)cursor_drawable, 
          cursor_drawable ? cursor_drawable->ID : -1);
      printf ("source_drawable = %p  ID=%d\n", (void*)source_drawable, 
          source_drawable ? source_drawable->ID : -1);
      printf ("tool->drawable  = %p  ID=%d\n", (void*)tool->drawable, 
          tool->drawable ? ((CanvasDrawable*)tool->drawable)->ID : -1);
    */
      if (1)   /* ? */
      /*if (active_tool->state != INACTIVE && active_tool->state != INACT_PAUSED  && !inactive_clean_up)*/
	{ 
          /*TRACE_BEGIN(("<1 =(active tool && inactive_clean_up)> - Draw QuellMarker..."));*/
          
          /** Hacked Bug: Here paint_core->core->win was sometimes invalid, e.g. NULL */
          /*TRACE_PRINTF(("core->win = %p",(void*)paint_core->core->win));*/
          
          PRINTF(("\t\tDrawMarker(): trans=(%.0f, %.0f)\n",trans_tx,trans_ty));
          gdk_draw_line (paint_core->core->win, paint_core->core->gc,
              CAST(gint) trans_tx - (TARGET_WIDTH >> 1), CAST(gint) trans_ty,
              CAST(gint) trans_tx + (TARGET_WIDTH >> 1), CAST(gint) trans_ty);
          gdk_draw_line (paint_core->core->win, paint_core->core->gc,
	      CAST(gint) trans_tx, CAST(gint) trans_ty - (TARGET_HEIGHT >> 1),
	      CAST(gint) trans_tx, CAST(gint) trans_ty + (TARGET_HEIGHT >> 1));
	  
          gdk_draw_arc (paint_core->core->win, paint_core->core->gc,
	      0,
	      CAST(gint) trans_tx - (radius+1)/2, CAST(gint) trans_ty - (radius+1)/2,
	      radius, radius,
	      0, 23040);
          
          /*  In offset mode draw a line from src to dest point. Why restricted
               to AlignYes?
              IF-condition opak. The simpler and "more logical" variant leaves 
               behind an unremoved line after switch to clone mode. */
          if (((active_tool->state == INACTIVE || active_tool->state == INACT_PAUSED )
              && first_down  &&  clone_options->aligned == AlignYes)  ||
              (inactive_clean_up && first_down && clone_options->aligned == AlignYes))
/*          if (first_down && clone_options->aligned == AlignYes)*/
	    {
              /*TRACE_BEGIN(("<...> - Draw VerbindungsLinie"));*/
              PRINTF(("\t\tDrawLine(): dtrans=(%.0f, %.0f)\n",dtrans_tx,dtrans_ty));
              
              if (source_drawable == cursor_drawable)
                {
                  gdk_draw_line (paint_core->core->win, paint_core->core->gc,
                      CAST(gint) trans_tx, CAST(gint) trans_ty,
                      CAST(gint) dtrans_tx, CAST(gint) dtrans_ty); 
                }
              else
              /*  If cursor_drwble != src_drwble we draw lines in both windows.
                  My way from the cursor_drawable to the associated GdkWindow:
                  drawable => gimage => gdisplay => gdisplay->canvas->window.
                  Something shorter? */
                { 
                  clone_draw_duplex_line (paint_core->core->gc, src_gdisp,
                      gdisplay_get_from_gimage (drawable_gimage (cursor_drawable)) );
                }  
              /*TRACE_END;*/
	    }
              
          /*TRACE_END;*/
	}
      else      /* currently unreachable */
	{
          TRACE_BEGIN(("<0 - else> Draw SourceMarker... - Set inactive_clean=0"));
	  inactive_clean_up = 0;
	  gdk_draw_line (paint_core->core->win, paint_core->core->gc,
	      CAST(gint) trans_tx - (TARGET_WIDTH >> 1), CAST(gint) trans_ty - (TARGET_HEIGHT >> 1),
	      CAST(gint) trans_tx + (TARGET_WIDTH >> 1), CAST(gint) trans_ty + (TARGET_HEIGHT >> 1));
	  gdk_draw_line (paint_core->core->win, paint_core->core->gc,
	      CAST(gint) trans_tx + (TARGET_WIDTH >> 1), CAST(gint) trans_ty - (TARGET_HEIGHT >> 1),
	      CAST(gint) trans_tx - (TARGET_WIDTH >> 1), CAST(gint) trans_ty + (TARGET_HEIGHT >> 1));

	  gdk_draw_arc (paint_core->core->win, paint_core->core->gc,
	      0,
	      CAST(gint) trans_tx - (radius+1)/2, CAST(gint) trans_ty - (radius+1)/2,
	      radius, radius,
	      0, 23040);
	  
	  if (first_down && clone_options->aligned == AlignYes) /* offset mode && AlignYes */
            {
              TRACE_BEGIN(("<first_down && AlignYes> - Draw Zusatzlinie"));
              gdk_draw_line (paint_core->core->win, paint_core->core->gc,
                  CAST(gint) trans_tx, CAST(gint) trans_ty,
                  CAST(gint) dtrans_tx, CAST(gint) dtrans_ty); 
              TRACE_END;    
            }
          TRACE_END;
	}
      /*TRACE_END;*/
    }
  else
    { /*  debug output */
      TRACE_PRINTF(("Do nothing - %s", any_not_paintcore_srcset_srcdrawimg (
          paint_core, src_set, 
          source_drawable ? drawable_gimage(source_drawable) : NULL) ));
    }      
  
  TRACE_EXIT;  
}


void
tools_free_clone (Tool *tool)
{ENTRY
  src_set = FALSE;
  paint_core_free (tool);
}

void 
clone_leave_notify ()
{
  TRACE_ENTER(("clone_LEAVE_notify"));
  draw_core_pause (((PaintCore *) active_tool->private)->core, active_tool);
  TRACE_EXIT;
}

void 
clone_enter_notify ()
{
  TRACE_ENTER(("clone_ENTER_notify"));
  draw_core_resume (((PaintCore *) active_tool->private)->core, active_tool); 
  TRACE_EXIT;
}

void 
clone_expose ()                         /* called in disp_callbacks.c */
{ENTRY
  if (clone_options->aligned == AlignYes || clone_options->aligned == AlignNo)
    expose = 1;
}

void
clone_delete_image (GImage *gimage)     /** not used  */
{ENTRY
  if (src_gdisp_ID == gimage->ID)
    {
      src_set = FALSE;
      first_down = TRUE;
      first_mv = TRUE;
      second_mv = TRUE;
      rm_cross = 0;
      clean_up = 0;
    }
}

void
clone_flip_image ()                     /* used in sfm */
{ENTRY
  clean_up = 1;
  expose = 1; 
}

/**  
*   Task: ... [Ohne kein Stempeln, alles andere geht]
*   Called by: clone_paint_func()  
*/
static void 
clone_motion  (PaintCore * paint_core,
               CanvasDrawable * drawable,
               double offset_x,         /* unused */
               double offset_y)         /* unused */
{ENTRY
  /*  Get the working canvas */
  paint_core_16_area_setup (paint_core, drawable);

  /*  Apply it to the image */
  if (setup_successful)
    paint_core_16_area_paste (paint_core, drawable,
			      current_device ? paint_core->curpressure : 1.0,
                              (gfloat) gimp_brush_get_opacity (),
			      SOFT, 1.0, CONSTANT,
			      gimp_brush_get_paint_mode ());
}


/**
*   Task: ... [Ohne zwar alle Cursor, Marken, Linie, aber kein Stempeln]
*   Called by: ...
*
*   Wirr: Verhaeltnis von src_drawable und source_drawable
*/
static void
clone_painthit_setup (PaintCore *paint_core, Canvas * painthit)
{
  TRACE_ENTER((__func__));
  setup_successful = FALSE;
  if (painthit)
    {
      CanvasDrawable *src_drawable, *drawable;
      TRACE_BEGIN(("<painthit>"));
      drawable = paint_core->drawable;
      src_drawable = source_drawable;
      if (paint_core->setup_mode == NORMAL_SETUP)
	{
	  drawable = paint_core->drawable;
	  src_drawable = source_drawable;      /* ? set again */
	}
      else if (paint_core->setup_mode == LINKED_SETUP)
	{
	  GSList * channels_list = gimage_channels (drawable_gimage (source_drawable)); 
	  drawable = paint_core->linked_drawable;
	  src_drawable = paint_core->linked_drawable; /*source_drawable;*/ 
	  if (channels_list)
	    {
	      Channel *channel = (Channel *)(channels_list->data);
	      if (channel)
		{
		  src_drawable = GIMP_DRAWABLE(channel);
		}
	      else
		e_printf ("PROBLEM 2\n"); 
	      if (src_drawable == source_drawable)
		e_printf ("ERROR1\n"); 
	      if (drawable == source_drawable)
		e_printf ("ERROR\n"); 
	    }
	  else
	    e_printf ("PROBLEM 3\n"); 
	}
      else 
	e_printf ("PROBLEM\n"); 
      
      TRACE_PRINTF((">>>>>> clone_line_image()..."));
      setup_successful = clone_line_image (paint_core, painthit,
	  drawable, src_drawable,
	  paint_core->curx + ((int)src_x-dest_x),/*offset_x*/
	  paint_core->cury + ((int)src_y-dest_y)/*offset_y*/); 
      TRACE_END;    
    }
  TRACE_EXIT;
}

/**
*   Task: Stempelt.  
*   Called by: clone_painthit_setup()
*/
static int 
clone_line_image  (PaintCore * paint_core,
                   Canvas * painthit,
                   CanvasDrawable * drawable,
                   CanvasDrawable * src_drawable,
                   double x,
                   double y)
{
  GImage * src_gimage = drawable_gimage (src_drawable);
  GImage * gimage = drawable_gimage (drawable);
  int rc = FALSE;
  int w, h, width, height, xx=0, yy=0; 
ENTRY
  if (gimage && src_gimage)
    {
      Matrix matrix;
      Canvas * rot=NULL; 
      Canvas * orig;
      PixelArea srcPR, destPR;
      double x1, y1, x2, y2;

      if (src_drawable != drawable) /* different windows */
	{
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2.0; 
	     height = canvas_height (painthit) * 2.0; 
	    }
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }
          
	  if (scale_x >= 1)
	    {
	      w = width;
	      h = height;
	      xx = x - w;
	      yy = y - h;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h, 0, drawable_height (src_drawable));
	      error_x = error_x < 0 ? (scale_x + error_x)  + w + (int)(w * SCALE_X + .5): 
		error_x + w + (int)(w * SCALE_X + .5); 
	      error_y = error_y < 0 ? (scale_y + error_y)  + h + (int)(h * SCALE_Y +.5): 
		error_y + h + (int)(h * SCALE_Y + .5);
	      error_x = scale_x == 1 ? w/2.0 + .5 : error_x; 
	      error_y = scale_y == 1 ? h/2.0 + .5 : error_y;
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  else
	    {
	      w = (width * SCALE_X) * 0.5;
	      h = (height * SCALE_Y) * 0.5;
	      xx = x - w - error_x;
	      yy = y - h - error_y;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w - error_x, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h - error_y, 0, drawable_height (src_drawable));	   
	    
	      error_x = error_y = 0; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
	  
	  orig = paint_core_16_area_original2 (paint_core, src_drawable,CAST(int) x1,CAST(int) y1,CAST(int) x2,CAST(int) y2);

	}
      else      /* scr_drawable == drawable */
	{
	  if (rotate)
	    {
	     width = canvas_width (painthit) * 2.0; 
	     height = canvas_height (painthit) * 2.0; 
	     /*error_x += canvas_width (painthit) / 2.0 + .5;
	     error_y += canvas_height (painthit) / 2.0 + .5;
	    */}
	  else
	    {
	     width = canvas_width (painthit); 
	     height = canvas_height (painthit); 
	    }
          
	  if (scale_x >= 1)
	    {
	      w = width;
	      h = height;
	      xx = x - w;
	      yy = y - h;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h, 0, drawable_height (src_drawable));
	      error_x = error_x < 0 ? (scale_x + error_x)  + w + (int)(w * SCALE_X + .5): 
		error_x + w + (int)(w * SCALE_X + .5); 
	      error_y = error_y < 0 ? (scale_y + error_y)  + h + (int)(h * SCALE_Y +.5): 
		error_y + h + (int)(h * SCALE_Y + .5);
	      error_x = scale_x == 1 ? w/2.0 + .5 : error_x; 
	      error_y = scale_y == 1 ? h/2.0 + .5 : error_y; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  else
	    {
	      w = (width * SCALE_X) * 0.5;
	      h = (height * SCALE_Y) * 0.5;
	      xx = x - w - error_x;
	      yy = y - h - error_y;
	      x1 = BOUNDS (xx, 0, drawable_width (src_drawable));
	      y1 = BOUNDS (yy, 0, drawable_height (src_drawable));
	      x2 = BOUNDS (x + w - error_x, 0, drawable_width (src_drawable));
	      y2 = BOUNDS (y + h - error_y, 0, drawable_height (src_drawable));	   
	    
	      error_x = error_y = 0; 
	      xx = x1 == 0 ? -xx : 0; 
	      yy = y1 == 0 ? -yy : 0; 
	    }
	  if (!(x2 - x1) || !(y2 - y1))
	    return FALSE;
	  
	  orig = paint_core_16_area_original2 (paint_core, src_drawable,CAST(int) x1,CAST(int) y1,CAST(int) x2,CAST(int) y2);
        
        }

        
      if (scale_x == 1 && scale_y == 1 && rotate == 0)
      {
        pixelarea_init (&srcPR, orig,CAST(int) error_x,CAST(int) error_y,
          CAST(int)  x2-x1,CAST(int)  y2-y1, FALSE);
      }
      else
      {
        identity_matrix (matrix);
        rotate_matrix (matrix, -rotate);
        scale_matrix (matrix, scale_x, scale_y);
        rot = transform_core_do (src_gimage, src_drawable, 
            orig, 0, matrix);
        error_x = rotate ?  (canvas_width (rot) - canvas_width (painthit)) / 2.0 + 1: error_x;
        error_y = rotate ?  (canvas_height (rot) - canvas_height (painthit)) / 2.0 : error_y;
       
        pixelarea_init (&srcPR, rot,CAST(int) error_x,CAST(int) error_y,
           CAST(int) width,CAST(int)  height, FALSE);
      }     

      pixelarea_init (&destPR, painthit,
          xx, yy, x2-x1, y2-y1, TRUE);
      
      copy_area (&srcPR, &destPR);
      
      if (rot)
        canvas_delete (rot); 

      rc = TRUE;
    }
  return rc;
}

/**  Reads x-offset from CloneOptions and applies it to bfm */
static gint
clone_x_offset_cb (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;
  CloneOptions *co = (CloneOptions*) client_data;
ENTRY  
  offset_x = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(co->x_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y);
      list = g_slist_next (list);
    }

  return TRUE;
}

/**  Reads y-offset from CloneOptions and applies it to bfm */
static gint
clone_y_offset_cb (GtkWidget *w, gpointer client_data)
{
  extern GSList *display_list;
  GSList *list = display_list;
  GDisplay *d;
  CloneOptions *co = (CloneOptions*) client_data;
ENTRY  
  offset_y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(co->y_offset_entry));
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
	bfm_onionskin_set_offset (d, offset_x, offset_y);
      list = g_slist_next (list);
    }

  return TRUE;
}

static gint	    
clone_x_scale_cb (GtkWidget *w, gpointer client_data) 
{ENTRY
  scale_x = atof (gtk_entry_get_text(GTK_ENTRY(w)));
  scale_x = scale_x == 0 ? 1 : scale_x;
  SCALE_X = (double) 1.0 / scale_x;   
  return TRUE; 
}

static gint	    
clone_y_scale_cb (GtkWidget *w, gpointer client_data) 
{ENTRY
  scale_y = atof (gtk_entry_get_text (GTK_ENTRY(w)));
  scale_y = scale_y == 0 ? 1 : scale_y; 
  SCALE_Y = (double) 1.0 / scale_y;   
  return TRUE; 
}

static gint	    
clone_rotate_cb (GtkWidget *w, gpointer client_data) 
{ENTRY
  rotate = (atof (gtk_entry_get_text (GTK_ENTRY(w))) / 180.0)  * M_PI;
  return TRUE; 
}

/**  @return TRUE, if `gdisplay' refers to the same image as src_gdisp_ID */
gint 
clone_is_src (GDisplay *gdisplay)
{ENTRY
  if (src_gdisp_ID == gdisplay->ID)
    return 1;
  
  return 0; 
}

/**  Sets the offset values in the CloneOptions menu  */
static void         
clone_options_set_offset (CloneOptions *co, int x, int y)
{ENTRY
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(co->x_offset_entry), x);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(co->y_offset_entry), y);
} 


/** 
*  Sets the file-globals offset_x, offset_y to x,y, updates the CloneOptions
*   values and sets BaseFrameManager's offset values.
*  @param x,y: new offset values
*/
static void
clone_set_offset (int x, int y)
{
  extern GSList *display_list;
  GSList *list = display_list; 
  GDisplay *d;

  TRACE_BEGIN(("%s( x=%d, y=%d ) - Enter",__func__, x, y));
  
  offset_x = x;
  offset_y = y; 
  if (clone_options) 
  {
    clone_options_set_offset (clone_options, x,y);
  }   
  while (list)
    {
      if ((d=((GDisplay *) list->data))->bfm)
        bfm_onionskin_set_offset (d, offset_x, offset_y);
      list = g_slist_next (list);
    }
  TRACE_EXIT;  
}

/** 
*  Five (nearly identic) functions led back to the single, more general
*   clone_set_offset()... [hsbo]
*/
void
clone_x_offset_increase ()   {clone_set_offset (offset_x+1, offset_y);}

void
clone_x_offset_decrease ()   {clone_set_offset (offset_x-1, offset_y);}

void
clone_y_offset_increase ()   {clone_set_offset (offset_x, offset_y+1);}

void
clone_y_offset_decrease ()   {clone_set_offset (offset_x, offset_y-1);}

void
clone_reset_offset ()        {clone_set_offset (0,0);}



int
clone_get_x_offset ()
{ENTRY
  return offset_x;
}

int
clone_get_y_offset ()
{ENTRY
  return offset_y;
}

/**  
*  arrow_keys function [hsbo]: 
*
*  Changes the offset values via arrow keys (with live rendering in onionskin
*   mode of bfm). Tailbacks in the event queue (if an arrow key keeps pressed)
*   are resolved by recursive calls with an abbreviation on the second level.
*/
static void
clone_arrow_keys_func (Tool *         tool,
                       GdkEventKey *  kevent,
                       gpointer       gdisp_ptr)
{
  static gboolean first_call = TRUE;
  static int offset_x_wanted;
  static int offset_y_wanted;
  int step; 

  printf("tool->gdisp=%p\n", tool->gdisp_ptr);
  printf("gdisp_ptr  =%p\n", gdisp_ptr);
  
  if (first_call) 
  {
    offset_x_wanted = offset_x;
    offset_y_wanted = offset_y;
  }

# ifdef DEBUG_CLONE
  PRINTF(("%s( tool=%s, displayID=%d )... [first=%d, dx=%d, dy=%d]\n",
      __func__, tool_type_name(tool), gdisplay_to_ID(gdisp_ptr), first_call, offset_x_wanted - offset_x, offset_y_wanted - offset_y ));
  /*report_GdkEventKey(kevent);*/
  printf("\t\t"); print_GdkEventKeyArrow(kevent);
# endif
  
  /*  If onionskin, a contrawise direction is more intuitive */
  if (bfm_onionskin ((GDisplay*)gdisp_ptr))  step = -1; else  step = 1;

  if (kevent->state & GDK_SHIFT_MASK) 
  {
    switch (kevent->keyval)
      {
      case GDK_Left:  offset_x_wanted -= 5 * step; break;
      case GDK_Right: offset_x_wanted += 5 * step; break;
      case GDK_Up:    offset_y_wanted -= 5 * step; break;
      case GDK_Down:  offset_y_wanted += 5 * step; break;
      default:                                     break;
      }
  }
  else if (kevent->state & GDK_MOD1_MASK)  /* <Alt> */
  {
    switch (kevent->keyval)
      {
      case GDK_Left:  offset_x_wanted -= 25 * step; break;
      case GDK_Right: offset_x_wanted += 25 * step; break;
      case GDK_Up:    offset_y_wanted -= 25 * step; break;
      case GDK_Down:  offset_y_wanted += 25 * step; break;
      default:                                      break;
      }
  }  
  else {
    switch (kevent->keyval)
      {
      case GDK_Left:  offset_x_wanted -= step; break;
      case GDK_Right: offset_x_wanted += step; break;
      case GDK_Up:    offset_y_wanted -= step; break;
      case GDK_Down:  offset_y_wanted += step; break;
      default:                                 break;
      }
  }
    
  while (offset_x_wanted != offset_x || offset_y_wanted != offset_y)
  {
    /*  If we are in a second (=recursive) call, return here; so only
        the `offset_wanted' values are summarized without rendering  */
    if (!first_call) return;
    
    /*  We are in the first call. A first rendering we do always.
        (After clone_set_offset() is offset = offset_wanted)  */
    clone_set_offset (offset_x_wanted, offset_y_wanted);
    
    first_call = FALSE;
  
    /*  During the first rendering further arrow events could happend.
        Handle them by recursive second calls of myself:          */
    while (gtk_events_pending())
      gtk_main_iteration();   /* can call myself */
  
    /*  All (arrow) events worked up. Perform the contingent second 'while'
        loop with rendering - for the totalized offset_wanted values:  */
    first_call = TRUE; 
  } 
  /*  Function leaved on the first level always with first_call==TRUE, as 
      needed for the next "first call"  */
}


#if 0
static void *
clone_non_gui_paint_func (PaintCore *paint_core,
    CanvasDrawable *drawable,
    int        state)
{
  clone_type = non_gui_clone_type;
  source_drawable = non_gui_source_drawable;

  clone_motion (paint_core, drawable, non_gui_offset_x, non_gui_offset_y);
  return NULL;
}
#endif


/*  The clone procedure definition  */
ProcArg clone_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_DRAWABLE,
    "src_drawable",
    "the source drawable"
  },
  { PDB_INT32,
    "clone_type",
    "the type of clone: { IMAGE-CLONE (0), PATTERN-CLONE (1) }"
  },
  { PDB_FLOAT,
    "src_x",
    "the x coordinate in the source image"
  },
  { PDB_FLOAT,
    "src_y",
    "the y coordinate in the source image"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord clone_proc =
{
  "gimp_clone",
  "Clone from the source to the dest drawable using the current brush",
  "This tool clones (copies) from the source drawable starting at the specified source coordinates to the dest drawable.  If the \"clone_type\" argument is set to PATTERN-CLONE, then the current pattern is used as the source and the \"src_drawable\" argument is ignored.  Pattern cloning assumes a tileable pattern and mods the sum of the src coordinates and subsequent stroke offsets with the width and height of the pattern.  For image cloning, if the sum of the src coordinates and subsequent stroke offsets exceeds the extents of the src drawable, then no paint is transferred.  The clone tool is capable of transforming between any image types including RGB->Indexed--although converting from any type to indexed is significantly slower.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  clone_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { clone_invoker } },
};


static Argument *
clone_invoker (Argument *args)
{ENTRY
#if 0
  int success = TRUE;
  GImage *gimage;
  CanvasDrawable *drawable;
  CanvasDrawable *src_drawable;
  double src_x, src_y;
  int num_strokes;
  double *stroke_array;
  int i;

  drawable = NULL;
  num_strokes = 0;

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
  /*  the src drawable  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      src_drawable = drawable_get_ID (int_value);
      if (src_drawable == NULL || gimage != drawable_gimage (src_drawable))
	success = FALSE;
      else
	non_gui_source_drawable = src_drawable;
    }
  /*  the clone type  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: non_gui_clone_type = ImageClone; break;
	default: success = FALSE;
	}
    }
  /*  x, y offsets  */
  if (success)
    {
      src_x = args[4].value.pdb_float;
      src_y = args[5].value.pdb_float;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[7].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = clone_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core.startx);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core.starty);

      if (num_strokes == 1)
	clone_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup (&non_gui_paint_core);
    }

  return procedural_db_return_args (&clone_proc, success);
#endif
  return 0;
}


/** Local debug helper functions */

#ifdef DEBUG_CLONE

/*  Name of the Paint states */
static const char* 
paint_state_name (int state)
{
  if (state == MOTION_PAINT)    return "MOTION_PAINT";
  if (state == INIT_PAINT)      return "INIT_PAINT";
  if (state == FINISH_PAINT)    return "FINISH_PAINT";
  return "(not listed)";
}  

/*  Coordinates of a PaintCore structure  (unused) */
static void report_coords (const PaintCore * paint_core)  
{
  if (paint_core) 
  {
    printf("  +++ start x=%.0f, y=%.0f,  cur x=%.0f, y=%.0f,  last x=%.0f, y=%.0f\n",
        paint_core->startx, paint_core->starty,
        paint_core->curx,   paint_core->cury, 
        paint_core->lastx,  paint_core->lasty);
  }
}

static void report_switches()
{
  printf("src_set=%d, 1.down=%d, 1.mv=%d, 2.mv=%d, clean=%d, inclean=%d\n",
      src_set, first_down, first_mv, second_mv, clean_up, inactive_clean_up);      
}

static void report_drawables (const CanvasDrawable * drawable)
{
  int srcdrw_ID = source_drawable ? source_drawable->ID : -1;
  int srcimg_ID = source_drawable ? source_drawable->gimage_ID : -1;
  int destdrw_ID = drawable ? drawable->ID : -1;  
  int destimg_ID = drawable ? drawable->gimage_ID : -1;  
  int tool_drwID;
  int tool_dispID;
  int tool_imgID;
  printf("  --- source_drawable=%p ID=%d -> imageID=%d;  src_dispID=%d\n", 
      (void*)source_drawable, srcdrw_ID, srcimg_ID, src_gdisp_ID);
  printf("  ---   dest_drawable=%p ID=%d -> imageID=%d\n", 
      (void*)drawable, destdrw_ID, destimg_ID);
  if (!active_tool) return;
  tool_drwID  = active_tool->drawable ? ((CanvasDrawable*)active_tool->drawable)->ID : -1;
  tool_dispID = active_tool->gdisp_ptr ? ((GDisplay*)active_tool->gdisp_ptr)->ID : -1; 
  tool_imgID  = active_tool->gdisp_ptr ? ((GDisplay*)active_tool->gdisp_ptr)->gimage->ID : -1; 
  printf("  --- acttool:drawble=%p ID=%d,   imageID=%d <- displayID=%d\n", 
      active_tool->drawable, tool_drwID, tool_imgID, tool_dispID);
}

static void report_active_tool()
{
  int dispID, imgID, drawID;
  
  if (!active_tool) {
    printf("  *** active_tool=(null)\n");
    return;
  }
  drawID = active_tool->drawable ? ((CanvasDrawable*)active_tool->drawable)->ID : -1;
  dispID = active_tool->gdisp_ptr ? ((GDisplay*)active_tool->gdisp_ptr)->ID : -1; 
  imgID  = active_tool->gdisp_ptr ? ((GDisplay*)active_tool->gdisp_ptr)->gimage->ID : -1; 
  printf("  *** active_tool=%s - %s, displayID=%d -> imgID=%d, drawID=%d\n", 
      tool_type_name(active_tool), tool_state_name(active_tool->state), 
      dispID, imgID, drawID);
}

static void report_PaintCore (const PaintCore *paint)
{
  printf("PaintCore:%d", paint ? 1 : 0);
  if (!paint) {printf("\n"); return;}
  printf(",  draw_core:%d", paint->core ? 1 : 0);
  if (paint->core) printf("  win=%p", (void*)paint->core->win);
  printf(",  drawableID=%d\n", paint->drawable ? paint->drawable->ID : -1);
  printf("\tcur=(%.0f, %.0f),  start=(%.0f, %.0f), last=(%.0f, %.0f)\n",
      paint->curx, paint->cury, paint->startx, paint->starty, paint->lastx, paint->lasty);
  printf("\tpainthit: xy=(%d, %d),  wh=(%d, %d)\n", paint->x, paint->y, paint->w, paint->h);      
}

/** any_not..() - give the name of the first item, which is false */
static const char*
any_not_atooldisp_paintcore_drawable (const GDisplay* gdisp, 
                                      const PaintCore* paint, 
                                      const CanvasDrawable* drawable)
{
  if (!gdisp)    return "!atool_disp";
  if (!paint)    return "!paint_core";
  if (!drawable) return "!drawable";
  return "** (nothing false) **";
}

static const char* 
any_not_inactive_srcset_alignYesNo (int inactive, int src_set, int alignYesNo)
{
  if (!inactive)   return "!inactive";
  if (!src_set)    return "!src_set";
  if (!alignYesNo) return "!alignYesNo";
  return "** (nothing false) **";
}

static const char*
any_not_paintcore_srcset_srcdrawimg (const PaintCore* core, int src_set, 
        const GImage* gimage)
{    
  if (!core)    return "!paint_core";
  if (!src_set) return "!src_set";
  if (!gimage)  return "!gimage(src_drawable)";
  return "** (nothing false) **";
}  


void report_winpos_of_gdisp (GDisplay * gdisp)
{
  int x, y;
  
  printf("\t  shell=%p  NO_WIN=%d  TOP_LEVEL=%d   win=%p  parent-win=%p\n",
      (void*)gdisp->shell, 
      GTK_WIDGET_NO_WINDOW(gdisp->shell),
      GTK_WIDGET_TOPLEVEL(gdisp->shell),
      (void*)gdisp->shell->window,
      (void*)gtk_widget_get_parent_window (gdisp->shell));    
  printf("\t canvas=%p  NO_WIN=%d  TOP_LEVEL=%d   win=%p  parent-win=%p\n",
      (void*)gdisp->canvas,
      GTK_WIDGET_NO_WINDOW(gdisp->canvas),
      GTK_WIDGET_TOPLEVEL(gdisp->canvas),
      (void*)gdisp->canvas->window,
      (void*)gtk_widget_get_parent_window (gdisp->canvas));
  
  gdk_window_get_position (gtk_widget_get_parent_window (gdisp->canvas), &x, &y); 
  printf("\tposition parent of canvas at (%d, %d)\n", x, y); 
  gdk_window_get_position (gtk_widget_get_parent_window (gdisp->hsb), &x, &y); 
  printf("\tposition parent of hsb    at (%d, %d)\n", x, y); 
                  
  gdk_window_get_position (gdisp->shell->window, &x, &y); 
  printf("\tposition shell  at (%d, %d)\n", x, y); 
  gdk_window_get_position (gdisp->canvas->window, &x, &y); 
  printf("\tposition canvas at (%d, %d)\n", x, y); 
  gdk_window_get_position (gdisp->hsb->window, &x, &y); 
  printf("\tposition hsb    at (%d, %d)\n", x, y); 
          
  gdk_window_get_origin (gdisp->shell->window, &x, &y); 
  printf("\torigin shell  at (%d, %d)\n", x, y); 
  gdk_window_get_origin (gdisp->canvas->window, &x, &y); 
  printf("\torigin canvas at (%d, %d)\n", x, y); 
  gdk_window_get_origin (gdisp->hsb->window, &x, &y); 
  printf("\torigin hsb    at (%d, %d)\n", x, y); 
          
  gdk_window_get_root_origin (gdisp->shell->window, &x, &y); 
  printf("\troot_origin shell  at (%d, %d)\n", x, y); 
  gdk_window_get_root_origin (gdisp->canvas->window, &x, &y); 
  printf("\troot_origin canvas at (%d, %d)\n", x, y); 
  gdk_window_get_root_origin (gdisp->hsb->window, &x, &y); 
  printf("\troot_origin hsb    at (%d, %d)\n", x, y); 
          
  gdk_window_get_deskrelative_origin (gdisp->shell->window, &x, &y); 
  printf("\tdeskrelat_origin shell  at (%d, %d)\n", x, y); 
  gdk_window_get_deskrelative_origin (gdisp->canvas->window, &x, &y); 
  printf("\tdeskrelat_origin canvas at (%d, %d)\n", x, y); 
  gdk_window_get_deskrelative_origin (gdisp->hsb->window, &x, &y); 
  printf("\tdeskrelat_origin hsb    at (%d, %d)\n", x, y); 
}
#endif  /* DEBUG_CLONE */

/* END OF FILE */
