/*
 * STORE FRAME MANAGER
 *
 * ChangeLog:
 *  ku.b@gmx.de at February 2006
 *  * reorganise API; function renaming, cleaning of functions
 *  * overhaul of sfm_advance, remove old code, structurize, fg/bg onion remains
 *  * get ride of many bugs in the flipbook
 *  * dont allow of double calling memory critical functions : sfm_set_veto
 *  * start documentation ready for doxygen
 */

/**@file store_frame_manager.c
 * @brief Flipbook source together with base_frame_manager.c
 *
 * Typology:<br>
 *
 * Frame:     an user visible image ,to draw on, onionskin ...<br>
 * Flipbook:  Gui with a list for navigating through frames and for settings<br>
 * Store:     an image with settings; the basic datas for a frame<br>
 * Onionskin: let a background image shine through the forgeground image<br>
 *            implemented with one for speed reasons<br>
 * Play:      show each of the flipbook frames in a loop - film playing<br>
 * Flip:      walk through the flipbook list - one step<br>
 * Advance:   remove frames from one end, add the same number on the other end
 *
 * I tried to reflect the above terms in the API.<br>
 */

#include "config.h"
#include "../lib/version.h"
#include "libgimp/gimpintl.h"
#include "filename.h"
#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include "stdio.h"
#include "layers_dialog.h"      /* hsbo - lc_dialog_update() */
#include "interface.h"
#include "ops_buttons.h"
#include "general.h"
#include "actionarea.h"
#include "fileops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "rect_selectP.h"
#include "menus.h"
#include "../lib/wire/iodebug.h"
#include "../lib/wire/wirebuffer.h"
#include "../lib/wire/enums.h"
#include "plug_in.h"
#include "undo.h"
#include "../lib/wire/wirebuffer.h"
#include "../lib/wire/enums.h"
#include "plug_in.h"
#include "spline.h"
#include "layer_pvt.h"

#include "buttons/play_forward.xpm"
#include "buttons/play_forward_is.xpm"
#include "buttons/play_backwards.xpm"
#include "buttons/play_backwards_is.xpm"
#include "buttons/forward.xpm"
#include "buttons/forward_is.xpm"
#include "buttons/backwards.xpm"
#include "buttons/backwards_is.xpm"
#include "buttons/stop.xpm"
#include "buttons/stop_is.xpm"

extern GSList *display_list;
extern GSList *image_list;
extern int initial_frames_loaded;
extern Tool *active_tool;
extern GSList *display_list;

typedef gint (*CallbackAction) (GtkWidget *, gpointer);
#define STORE_LIST_WIDTH        200
#define STORE_LIST_HEIGHT       150
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

static gint   type_to_add_var;

static gint   sfm_onionskin_callback (GtkWidget *, gpointer);
static gint   sfm_onionskin_on_callback (GtkWidget *, gpointer);
static gint   sfm_onionskin_fgbg_callback (GtkWidget *, gpointer);

static gint   sfm_auto_save_callback (GtkWidget *, gpointer);
static gint   sfm_set_aofi_callback (GtkWidget *, gpointer);
static gint   sfm_aofi_callback (GtkWidget *, gpointer);

static void   sfm_update_extern_image_observers (GImage*);  /* hsbo */
static void   sfm_update_extern_image_observers (GImage* gimage)  /* hsbo */
{
# ifdef DEBUG
  printf("%s()...\n",__func__);
# endif
  lc_dialog_update (gimage);
}

static void   sfm_unset_aofi (GDisplay *);

static void   sfm_play_backwards_callback (GtkWidget *, gpointer);
static void   sfm_play_forward_callback (GtkWidget *, gpointer);
static void   sfm_stop_callback (GtkWidget *, gpointer);
static void   sfm_flip_backwards_callback (GtkWidget *, gpointer);
static void   sfm_flip_forward_callback (GtkWidget *, gpointer);

static gint   sfm_adv_backwards_callback    (GtkWidget *, gpointer);
static gint   sfm_adv_forward_callback      (GtkWidget *, gpointer);

/* manipulate data */
static store* sfm_frame_create  (GDisplay*, GImage*, int row, int active,
                                 int readonly, int special, int update);
static void   sfm_frame_remove  (GDisplay *disp, int num, int update);

static void   sfm_frames_remove (GDisplay*);
static void   sfm_gimage_save   (GDisplay *disp, store *, int delete);
static GImage*sfm_gimage_load   (GDisplay *disp, store* reference, int adv);
static GImage*sfm_file_load_without_display (const char * full_name,
                                 const char * base_name, 
                                 GDisplay * disp);
static void   sfm_store_remove              (GDisplay *disp, store *);
static void   sfm_stores_add                (GDisplay* disp);

/* get informations */
static store* sfm_store_get                 (GDisplay *disp, int num);

/* UI handling */
static void   sfm_frame_flipbook_update     (GDisplay *, gint, gint);
static void   sfm_frame_make_cur            (GDisplay *, int pos);
static void   sfm_store_make_cur            (GDisplay *, int pos);
static void   sfm_flipbook_store_update     (GDisplay *, int pos);
static void   sfm_flipbook_store_update_full(GDisplay *);

static void   sfm_store_add_dialog_create   (GDisplay *disp);

static void   sfm_frame_change_dialog_create(GDisplay *disp);
static void   sfm_frame_change              (GDisplay* disp);


static gint   sfm_advance                   (GDisplay* disp, int num, int adv);

/* more callbacks */
static void   sfm_store_select_callback     (GtkCList *, gint, gint,
                                             GdkEventButton *, gpointer);
static void   sfm_store_unselect_callback   (GtkCList *, gint, gint,
                                             GdkEventButton *, gpointer);
static void   sfm_store_set_option_callback (GtkCList *, gint, gpointer);


/*
static MenuItem sfm_store_entries[] =
{
	{"/Store/Add", 0, 0, sfm_store_add, NULL, NULL, NULL},
	{"/Store/Delete", 0, 0, sfm_store_remove, NULL, NULL, NULL},
	{"/Store/Raise", 0, 0, sfm_store_raise, NULL, NULL, NULL},
	{"/Store/Lower", 0, 0, sfm_store_lower, NULL, NULL, NULL},
	{"/Store/Save", 0, 0, sfm_store_save, NULL, NULL, NULL},
	{"/Store/Save All", 0, 0, sfm_store_save_all, NULL, NULL, NULL},
	{"/Store/Revert", 0, 0, sfm_store_revert, NULL, NULL, NULL},
	{"/Store/Change Frame", 0, 0, sfm_store_change_frame, NULL, NULL, NULL},
	{"/Dir/Recent Src", 0, 0, sfm_store_recent_src, NULL, NULL, NULL},
	{"/Dir/Recent Dest", 0, 0, sfm_store_recent_dest, NULL, NULL, NULL},
	{"/Dir/Load Smart", 0, 0, sfm_store_load_smart, NULL, NULL, NULL},
        { NULL, 0, 0, NULL, NULL, NULL, NULL }, 
};
*/
static gint
sfm_check (GDisplay *disp)
{
   if (!disp->bfm) {
           g_message(_("There is no Flipbook. Add with <Imagemenu>->File->Flipbook->Create"));
	   return 1;
   }
   if (!disp->bfm->sfm)
	   return 1;
   return 0;
}

static gint
sfm_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  GDisplay *disp = (GDisplay*)data;
  store_frame_manager *sfm = disp->bfm->sfm;
  bfm_onionskin_rm (disp);
  sfm_frames_remove (disp);

  if (sfm->add_dialog) gtk_widget_hide (sfm->add_dialog);
  if (sfm->chg_frame_dialog) gtk_widget_hide (sfm->chg_frame_dialog);
  gtk_widget_hide (sfm->shell);
  gtk_widget_destroy (GTK_WIDGET(sfm));
  return TRUE; 
}

void
sfm_create_gui (GDisplay *disp)
{

  GtkWidget *check_button, *button, *hbox, *flip_box, *scrolled_window,
  *utilbox, *slider, *menubar, *event_box, *label, *vbox;
  GtkTooltips *tooltip;

  char tmp[256];
  char *check_names[3] =
    {
      "Auto Save",
      "Set AofI",
      "AofI"
    };
  CallbackAction check_callbacks[3] =
    {
      sfm_auto_save_callback,
      sfm_set_aofi_callback,
      sfm_aofi_callback
    };
  OpsButton flip_button[] =
    {
	{ play_backwards_xpm, play_backwards_is_xpm, sfm_play_backwards_callback, "Play Backwards", NULL, NULL, NULL, NULL, NULL, NULL },
	{ backwards_xpm, backwards_is_xpm, sfm_flip_backwards_callback, "Flip Backward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ stop_xpm, stop_is_xpm, sfm_stop_callback, "Stop", NULL, NULL, NULL, NULL, NULL, NULL },
	{ forward_xpm, forward_is_xpm, sfm_flip_forward_callback, "Flip Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ play_forward_xpm, play_forward_is_xpm, sfm_play_forward_callback, "Play Forward", NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, 0, 0, NULL, NULL, NULL, NULL }
    };

  gchar *store_col_name[6] = {"RO", "Adv", "F", "Bg", "*", "Filename"};


  check_names[0] = _("Auto Save");
  check_names[1] = _("Set AofI");
  /* Area of Interesst */
  check_names[2] = _("AofI");

  flip_button[0].tooltip = _("Play Backwards");
  flip_button[1].tooltip = _("Flip Backward");
  flip_button[2].tooltip = _("Stop");
  flip_button[3].tooltip = _("Flip Forward");
  flip_button[4].tooltip = _("Play Forward");

  /* "readonly" abbreviation */
  store_col_name[0] = _("RO");
  /* "advance" abbreviation */
  store_col_name[1] = _("Adv");
  store_col_name[2] = _("F");
  /* "background" abbreviation */
  store_col_name[3] = _("Bg");
  store_col_name[4] = "*";
  store_col_name[5] = _("Filename");

  if (disp->gimage->filename == NULL){
     g_message (_("You need to have saved the filename out first, also it should probably be of the form FILE.0001.EXT"));
     return;
   }
 
  /* init variables */
  disp->bfm->sfm->stores = 0;
  disp->bfm->sfm->bg = -1;
  disp->bfm->sfm->fg = 0;
  disp->bfm->sfm->add_dialog = 0;
  disp->bfm->sfm->chg_frame_dialog = 0;
  disp->bfm->sfm->readonly = 0; 
  disp->bfm->sfm->play = 0; 
  disp->bfm->sfm->load_smart = 1; 

  disp->bfm->sfm->s_x = disp->bfm->sfm->sx = 0;
  disp->bfm->sfm->s_y = disp->bfm->sfm->sy = 0;
  disp->bfm->sfm->e_x = disp->bfm->sfm->ex = disp->gimage->width;
  disp->bfm->sfm->e_y = disp->bfm->sfm->ey = disp->gimage->height;
  disp->bfm->sfm->old_offset_x = 0;
  disp->bfm->sfm->old_offset_y = 0;
  disp->bfm->sfm->old_ex = disp->bfm->sfm->e_x;
  disp->bfm->sfm->old_ey = disp->bfm->sfm->e_y;
  disp->bfm->sfm->old_opacity = 1.;


  disp->bfm->sfm->accel_group = gtk_accel_group_new ();

  /* the shell */
  disp->bfm->sfm->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (disp->bfm->sfm->shell), "sfm", PROGRAM_NAME);
  gtk_window_set_policy (GTK_WINDOW (disp->bfm->sfm->shell), FALSE, TRUE, FALSE);
  sprintf (tmp, _("Flipbook for %s"), disp->gimage->filename);
  gtk_window_set_title (GTK_WINDOW (disp->bfm->sfm->shell), tmp);

  /*
  gtk_window_set_transient_for (GTK_WINDOW (disp->bfm->sfm->shell),
      GTK_WINDOW (disp->shell));
  */

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->shell), "delete_event",
      GTK_SIGNAL_FUNC (sfm_delete_callback),
      disp);

  /* pull down menu */
#if 0
  menubar = build_menu (sfm_store_entries, disp->bfm->sfm->accel_group);
#endif

  menus_get_sfm_store_menu (&menubar, &disp->bfm->sfm->accel_group, disp);
  if(disp->bfm->sfm->load_smart)
    menus_set_state("<sfm_menu>/Dir/Load Smart", TRUE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_reparent (menubar, GTK_WIDGET (GTK_DIALOG (disp->bfm->sfm->shell)->vbox));
  gtk_widget_show (menubar);

  /* gtk_window_add_accel_group (GTK_DIALOG (disp->shell), disp->bfm->sfm->accel_group); */
  
  /* check buttons */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
      
  disp->bfm->sfm->autosave_cbtn = check_button = gtk_check_button_new_with_label (check_names[0]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[0],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      _("Saves all stores not marked RO before removing them from list"), NULL);

  check_button = gtk_check_button_new_with_label (check_names[1]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[1],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      _("Sets the area of interest"), NULL);

  disp->bfm->sfm->aofi_cbtn = check_button = gtk_check_button_new_with_label (check_names[2]);
  gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
      (GtkSignalFunc) check_callbacks[2],
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, check_button, 
      _("Enable area of interest"), NULL);
  
  /* advance */
  button = gtk_button_new_with_label ("<");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_adv_backwards_callback,
      disp);
  gtk_widget_show (button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      _("Loads a new frame (current frame number - step size) into each store marked A."), NULL);

  disp->bfm->sfm->num_to_adv_spin = gtk_spin_button_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 100, 1, 1, 0)), 1.0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->num_to_adv_spin, FALSE, FALSE, 2);
  gtk_widget_show (disp->bfm->sfm->num_to_adv_spin);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, disp->bfm->sfm->num_to_adv_spin, 
      _("Step size"), NULL);
  
  button = gtk_button_new_with_label (">");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_adv_forward_callback,
      disp);
  gtk_widget_show (button);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      _("Loads a new frame (current frame number + step size) into each store marked A."), NULL);
  
  /* store window */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);


  gtk_widget_set_usize (scrolled_window, 200, 150); 
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), scrolled_window, 
	TRUE, TRUE, 0);
  
  event_box = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_window), event_box);
  gtk_widget_show (event_box);

  disp->bfm->sfm->store_list = gtk_clist_new_with_titles( 6, store_col_name);

  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 0, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 1, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 2, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 3, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 4, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification (GTK_CLIST(disp->bfm->sfm->store_list), 5, GTK_JUSTIFY_LEFT);

  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 0);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 1);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 2);
  gtk_clist_column_title_active (GTK_CLIST(disp->bfm->sfm->store_list), 3);
  
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 0, 20);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 1, 30);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 2, 10);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 3, 20);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 4, 10);
  gtk_clist_set_column_width (GTK_CLIST(disp->bfm->sfm->store_list), 5, 
      strlen (prune_pathname (disp->gimage->filename))*10);
  
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "select_row",
      GTK_SIGNAL_FUNC(sfm_store_select_callback),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "unselect_row",
      GTK_SIGNAL_FUNC(sfm_store_unselect_callback),
      disp);
  gtk_signal_connect(GTK_OBJECT(disp->bfm->sfm->store_list), "click-column",
      GTK_SIGNAL_FUNC(sfm_store_set_option_callback),
      disp);

  gtk_clist_set_selection_mode (GTK_CLIST(disp->bfm->sfm->store_list), GTK_SELECTION_BROWSE); 

  /* It isn't necessary to shadow the border, but it looks nice :) */
  gtk_clist_set_shadow_type (GTK_CLIST(disp->bfm->sfm->store_list), GTK_SHADOW_OUT);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      _("RO - read only. This store will not be saved\n"
      "A - advance. This store is affected by the advance controls\n"
      "F - flip. This store is affected by the flip book controls\n"
      "Bg - background. This is the src in a clone and the bg in the onionskin\n"
      "* - modified. This store has been modified"), NULL);

  /* Add the CList widget to the vertical box and show it. */
  gtk_container_add(GTK_CONTAINER(event_box), disp->bfm->sfm->store_list);
  gtk_widget_show(disp->bfm->sfm->store_list);
  gtk_widget_show (scrolled_window);

  /* flip buttons */
  flip_box = ops_button_box_new2 (disp->bfm->sfm->shell, tool_tips, flip_button, disp);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), flip_box, FALSE, FALSE, 0);
  gtk_widget_show (flip_box);

  /* onion skinning */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);


  disp->bfm->sfm->onionskin_cbtn = gtk_check_button_new_with_label (_("onionskin"));
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->onionskin_cbtn), "clicked",
      (GtkSignalFunc) sfm_onionskin_on_callback,
      disp);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (disp->bfm->sfm->onionskin_cbtn), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (disp->bfm->sfm->onionskin_cbtn));
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, disp->bfm->sfm->onionskin_cbtn, 
      _("Checked when onionskining is on"), NULL);
  
  
  utilbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), utilbox, TRUE, TRUE, 0);
  gtk_widget_show (utilbox);

  disp->bfm->sfm->onionskin_vslider = (GtkWidget*) gtk_adjustment_new (1.0, 0.0, 1.0, .1, .1, 0.0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (disp->bfm->sfm->onionskin_vslider));
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (utilbox), slider, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (disp->bfm->sfm->onionskin_vslider), "value_changed",
      (GtkSignalFunc) sfm_onionskin_callback,
      disp);
  gtk_widget_show (slider);
  
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, slider, 
      _("Sets the opacity for onion skinning.\n"
      "LMB to drag slider\n"
      "MMB to jump"), NULL);

  button = gtk_button_new_with_label (_("Bg/Fg"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      (GtkSignalFunc) sfm_onionskin_fgbg_callback,
      disp);
  gtk_widget_show (button);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, 
      _("Flips between the fg and bg store"), NULL);

  /* src & dest labels */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);
  
  label = gtk_label_new (_("SRC DIR"));
  gtk_box_pack_start(GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  label = gtk_label_new (_("DEST DIR"));
  gtk_box_pack_start(GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  scrolled_window =  gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
  gtk_box_pack_start(GTK_BOX (hbox), scrolled_window,
      TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);
  
  hbox = gtk_vbox_new (FALSE, 1);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_window), hbox);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_widget_show (hbox);
 
  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), event_box, FALSE, FALSE, 0);
  gtk_widget_show (event_box);
  
  
  disp->bfm->sfm->src_dir_label = gtk_label_new (_("SRC DIR"));
  gtk_label_set_justify (GTK_LABEL(disp->bfm->sfm->src_dir_label), GTK_JUSTIFY_LEFT);
  gtk_container_add (GTK_CONTAINER (event_box), 
      disp->bfm->sfm->src_dir_label);
  gtk_widget_show (disp->bfm->sfm->src_dir_label);

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      _("Src dir and file name"), NULL);
  
  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), event_box, FALSE, FALSE, 0);
  gtk_widget_show (event_box);

  disp->bfm->sfm->dest_dir_label = gtk_label_new (_("DEST DIR"));
  gtk_label_set_justify (GTK_LABEL(disp->bfm->sfm->dest_dir_label), GTK_JUSTIFY_LEFT);
  gtk_container_add (GTK_CONTAINER(event_box), disp->bfm->sfm->dest_dir_label);
  gtk_widget_show (disp->bfm->sfm->dest_dir_label);

#if GTK_MAJOR_VERSION > 1
  disp->bfm->sfm->status_label = gtk_label_new ("");
  gtk_widget_set_size_request ( disp->bfm->sfm->status_label, 40, 10);
  gtk_label_set_justify       ( disp->bfm->sfm->status_label,
                                GTK_JUSTIFY_LEFT );
  gtk_widget_set_size_request ( GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->action_area), 20, 10);
//  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (disp->bfm->sfm->shell)->action_area),
//                      event_box,
                      disp->bfm->sfm->status_label,
                      FALSE, FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER (GTK_DIALOG (disp->bfm->sfm->shell)->action_area), 0);
  //gtk_widget_show (event_box);

//  gtk_container_add (GTK_CONTAINER(event_box), disp->bfm->sfm->status_label);
  gtk_widget_show (disp->bfm->sfm->status_label);
#endif

  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, event_box, 
      _("Dest dir and file name"), NULL);


  /* add store */
  sfm_frame_create (disp, disp->gimage, 0, 1, 0, 0, 1);
  bfm_set_fg (disp, disp->gimage);
  sfm_unset_aofi (disp);
  sfm_store_make_cur(disp, 0);
    
  gtk_widget_show (disp->bfm->sfm->shell); 
}


/*
 * FLIP BOOK CONTROLS
 */

static gint
sfm_backwards (GDisplay* disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  gint row = fm->fg - 1 < 0 ? g_slist_length (fm->stores)-1 : fm->fg - 1;

  while (!sfm_store_get(disp, row)->flip && row != fm->fg)
    {
      row = row - 1 < 0 ? g_slist_length (fm->stores) - 1: row - 1;
    }

  sfm_frame_make_cur (disp, row);
  
  return TRUE;
}

static void
sfm_play_backwards_callback (GtkWidget *w, gpointer data)
{
 sfm_play_backwards ((GDisplay*) data);
}

void 
sfm_play_backwards (GDisplay* gdisplay)
{
  store_frame_manager *fm;
  gint row=0;
  if (sfm_check (gdisplay)) return;

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  if (g_slist_length (fm->stores) <= 1)
    {
      g_message(_("Nothing to Play."));
      return;
    }

  bfm_onionskin_rm (gdisplay);

  if (!fm->play)
    {
      fm->play = gtk_timeout_add (100, (GtkFunction) sfm_backwards, gdisplay);
    }

}

static gint
sfm_forward (GDisplay* disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  gint row = fm->fg + 1 == g_slist_length (fm->stores) ? 0 : fm->fg + 1;

  while (!sfm_store_get(disp, row)->flip && row != fm->fg)
    {
      row = row + 1 == g_slist_length (fm->stores)  ? 0 : row + 1;
    }

  sfm_frame_make_cur (disp, row);

  return TRUE;
}

static void
sfm_play_forward_callback (GtkWidget *w, gpointer data)
{
 sfm_play_forward((GDisplay*) data);
}

/* helpers for timing */
#          if defined(_POSIX_C_SOURCE)
# define   ZEIT_TEILER 10000
#          else // WINDOWS TODO
# define   ZEIT_TEILER CLOCKS_PER_SEC;
#          endif

# include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
  time_t zeit()
  {
           time_t zeit_;
           double teiler = ZEIT_TEILER;
#          if defined(_POSIX_C_SOURCE)
           struct timeval tv;
           gettimeofday( &tv, NULL );
           double tmp_d;
           zeit_ = tv.tv_usec/(1000000/(time_t)teiler)
                   + (time_t)(modf( (double)tv.tv_sec / teiler, &tmp_d )
                     * teiler*teiler);
#          else // WINDOWS TODO
           zeit_ = clock();
#          endif
    return zeit_;
  }
  double zeitSekunden()
  {
           time_t zeit_ = zeit();
           double teiler = ZEIT_TEILER;
           double dzeit = zeit_ / teiler;
    return dzeit;
  }

void 
sfm_play_forward (GDisplay* gdisplay)
{
  store_frame_manager *fm;
  gint row=0;
  if (sfm_check (gdisplay)) return;
 
  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm;
  if (g_slist_length (fm->stores) <= 1)
    {
      g_message(_("Nothing to Play."));
      return;
    }

  bfm_onionskin_rm (gdisplay);

  if (!fm->play)
    {
      double ms_timeout = 0.041;
      double cdiff = 0;
      char text[64] = {""};

      fm->play = 1;
      while(fm->play)
      {
        /* meshure the first time */
        double c1 = zeitSekunden(), c2, c3, sl, l_err;
        
        /* do something */
        sfm_forward( gdisplay );

        /* time after displaying */
        c2 = zeitSekunden();
        /* expected time for sleeping */
        sl = ms_timeout - (c2 - c1) - cdiff;

        if( sl > 0.001 )
          usleep( sl * 1000000 );
        /* time after sleep */
        c3 = zeitSekunden();
        /* time needed for sleep */
        cdiff = c3 - c2 - (sl>0 ? sl : 0);
        /* the last error */
        l_err = cdiff + sl;

        /* show the frame rate in the bottom of the flipbook */
        fm->fps = 1./ (c3 - c1);
        sprintf( text,"%s: %.02f", _("Fps"), fm->fps);
        gtk_label_set_text( fm->status_label, text );
      }
#if 0
      /* seems not the right thing */
      fm->play = gtk_timeout_add (ms_timeout,
                                  (GtkFunction) sfm_forward, gdisplay);
#endif
      ms_timeout = 0;
      gtk_label_set_text( fm->status_label, "" );
    }
}

static void
sfm_flip_backwards_callback (GtkWidget *w, gpointer data)
{
  sfm_flip_backwards ((GDisplay*) data);
}

void 
sfm_flip_backwards (GDisplay* gdisplay)
{
  store_frame_manager *fm;
  gint row=0;
  int num_to_adv;

  if (sfm_check (gdisplay))
    {
      return;
    }

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  if (g_slist_length (fm->stores) <= 1)
    {
      g_message(_("Nothing to Flip."));
      return;
    }

  num_to_adv = - gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv_spin));
  sfm_advance( gdisplay, num_to_adv, 0);
}

static void
sfm_flip_forward_callback (GtkWidget *w, gpointer data)
{
  sfm_flip_forward ((GDisplay*) data);
}

void 
sfm_flip_forward (GDisplay* gdisplay)
{
  store_frame_manager *fm;
  gint row=0;
  int num_to_adv;

  if (sfm_check (gdisplay))
    {
      return;
    }

  if (!gdisplay || row == -1)
    return;

  fm = gdisplay->bfm->sfm; 
  if (g_slist_length (fm->stores) <= 1)
    {
      g_message(_("Nothing to Flip."));
      return;
    }

  num_to_adv = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv_spin));
  sfm_advance( gdisplay, num_to_adv, 0);
}

static void
sfm_stop_callback (GtkWidget *w, gpointer data)
{
  sfm_stop ((GDisplay*) data);
}

void 
sfm_stop (GDisplay* gdisplay)
{
  store_frame_manager *fm;
  if (sfm_check (gdisplay)) return;
  fm = gdisplay->bfm->sfm;
  
  bfm_onionskin_rm (gdisplay);

  if (fm->play > 1)
    {
      gtk_timeout_remove (fm->play);
    }
  fm->play = 0;

  /*  Since observer updating was off while playing, inform the observers 
      now of the last (=current) image [hsbo]  */
  sfm_update_extern_image_observers (gdisplay->gimage);
}

/*
 * ADVANCE
 */

static gint /* ku.b */
sfm_set_veto(gint set_veto)
{
  static int has_veto = 0;

  if(set_veto && has_veto) return 0;

  has_veto = set_veto;

  return 1;
}

/**@brief advance frames; update UI */
static gint /* ku.b */
sfm_advance (GDisplay* disp, int num_to_adv, int adv)
{
  store_frame_manager *fm;
  store *store_fg = NULL, *s = NULL;
  int i = 0,
      old_bg, old_fg,
      old_onionskin;
  double old_onionskin_val;
  GImage *img=NULL;
  int  stores_old_n, stores_n;

  if (sfm_check (disp)) 
    { 
      return 1;
    }

  /* prohibit double calling */
  if(!sfm_set_veto(1)) return 0;

  fm = disp->bfm->sfm; 
  old_bg = fm->bg;
  old_fg = fm->fg;
  old_onionskin = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn));
  old_onionskin_val = GTK_ADJUSTMENT(fm->onionskin_vslider)->value;
  stores_old_n = g_slist_length(fm->stores);
  store_fg = sfm_store_get(disp, old_fg);
  bfm_onionskin_rm (disp);

  /* flip */
  if(!adv)
  {
    int new_fg = old_fg + num_to_adv;
    int new_bg = old_bg + num_to_adv;
    if(0 > new_fg)
      old_fg = stores_old_n - 1 + num_to_adv + old_fg + 1;
    else if( new_fg >= stores_old_n )
      old_fg = stores_old_n - 1 - old_fg + num_to_adv - 1;
    else
      old_fg += num_to_adv;
    if(0 > new_bg)
      old_bg = stores_old_n - 1 + num_to_adv + old_bg + 1;
    else if( new_bg >= stores_old_n )
      old_bg = stores_old_n - 1 - old_bg + num_to_adv - 1;
    else
      old_bg += num_to_adv;
  }

  /* A - insert new frames on begin or end */
  if (adv && num_to_adv > 0)
    for (i = stores_old_n; i < stores_old_n + num_to_adv; ++i)
      { int active = (fm->fg == i), readonly = store_fg->readonly,
            special = 0, update = 1, offset = i - old_fg;
        if ((img = sfm_gimage_load (disp, store_fg, offset)) == NULL)
          {
            g_message (_("Could not load image."));
            break;
          }
        sfm_frame_create (disp, img, i, active, readonly, special, update);
      }
  else if (adv)
    for (i = -1; i >= num_to_adv; --i)
      { int active = 0/*(fm->fg == stores_old_n-1)*/, readonly = store_fg->readonly, 
            special = 0, update = 1, offset = i - old_fg;
        if ((img = sfm_gimage_load (disp, store_fg, offset)) == NULL)
          {
            g_message (_("Could not load image."));
            break;
          }
        sfm_frame_create (disp, img, 0, active, readonly, special, update);
      }
  stores_n = g_slist_length (fm->stores);

  /* B - remove remaining frames */
  if (adv && num_to_adv > 0)
    { int rm_n = stores_n - stores_old_n;
      sfm_frame_make_cur (disp, old_fg + rm_n);
      for (i = 0; i < rm_n; ++i)
        { 
          sfm_frame_remove  (disp, 0, 0);
        }
      sfm_frame_make_cur (disp, old_fg);
    }
  else if (adv)
    for (i = stores_n - 1; i > stores_old_n - 1; --i)
      { 
        sfm_frame_remove  (disp, i, 1);
      }

  /* Flip */
  if (!adv)
  {
    sfm_store_make_cur( disp, old_fg );
    sfm_frame_make_cur( disp, old_fg );
  }

  /* C - update fg/bg, UI */
  fm->fg = old_fg;
  if (fm->fg >= 0 && sfm_store_get (disp, old_fg))
    bfm_set_fg (disp, sfm_store_get (disp, old_fg)->gimage);
  fm->bg = old_bg;
  if (fm->bg >= 0 && sfm_store_get (disp, old_bg))
    bfm_set_bg (disp, sfm_store_get (disp, old_bg)->gimage);
  if (fm->bg >= 0)
    {
      sfm_set_bg (disp, fm->bg);
      s = sfm_store_get (disp, fm->bg);
      bfm_set_bg (disp, s->gimage);
    }
  sfm_flipbook_store_update_full (disp);

  if (old_onionskin)
    {
      /* HACK: set UI and then update */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(fm->onionskin_cbtn), TRUE); 
      gtk_adjustment_set_value (GTK_ADJUSTMENT(fm->onionskin_vslider), old_onionskin_val);
      sfm_set_veto(0);
        sfm_onionskin_on_callback (fm->onionskin_cbtn, disp);
      sfm_set_veto(1);
    }

  sfm_set_veto(0);
  return 1;
}

static gint
sfm_adv_backwards_callback (GtkWidget *w, gpointer data)
{
  return sfm_adv_backwards ((GDisplay*)data);
}

gint 
sfm_adv_backwards (GDisplay *disp)
{
  store_frame_manager *fm = 0;
  int num_to_adv;

  if (sfm_check (disp)) return 0;
  fm = disp->bfm->sfm;
  num_to_adv = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv_spin));

  return sfm_advance (disp, -num_to_adv, 1);
}

static gint
sfm_adv_forward_callback (GtkWidget *w, gpointer data)
{
  return sfm_adv_forward ((GDisplay*)data);
}

gint 
sfm_adv_forward (GDisplay* disp)
{
  store_frame_manager *fm = 0;
  int num_to_adv;

  if (sfm_check (disp)) return 0;
  fm = disp->bfm->sfm;
  num_to_adv = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_adv_spin));

  return sfm_advance (disp, num_to_adv, 1);
}


/*
 * ONION SKIN
 */
      
static char onionskin_off = 1;
static gint 
sfm_onionskin_callback (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;

  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      e_printf ("ERROR: you must select a bg\n");
      if (((GtkAdjustment*)fm->onionskin_vslider)->value != 1)
	gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_vslider), 1);
      return 1;
    }
  
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin_cbtn)) &&
      onionskin_off) 
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin_cbtn), TRUE); 

  bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_vslider)->value,
      fm->s_x, fm->s_y, fm->e_x, fm->e_y); 

  return 1;
}

void
sfm_onionskin_toogle_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = gdisplay_active ();
  store_frame_manager *fm = NULL;

  if (!disp || !disp->bfm || !disp->bfm->sfm)
    return;

  fm = disp->bfm->sfm;

  if (fm->bg >= 0 && fm->bg != fm->fg)
    {
      if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn), TRUE);
      else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn), FALSE);
    }
  else
       return;

  sfm_onionskin_on_callback (w, data);
}

static gint 
sfm_onionskin_on_callback (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm;

  GDisplay *disp = (GDisplay*) data;
  if (!disp)
    return 0;

  /* prohibit double calling */
  if(!sfm_set_veto(1)) return 0;

  fm = disp->bfm->sfm;

  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      e_printf ("ERROR: you must select a bg\n");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn), FALSE); 
      sfm_set_veto(0);
      return 1;
    }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn)))
    {
      onionskin_off = 1;
      /* rm onionskin */
      if (GTK_ADJUSTMENT(disp->bfm->sfm->onionskin_vslider)->value != 1)
	{
	  gtk_adjustment_set_value (GTK_ADJUSTMENT(disp->bfm->sfm->onionskin_vslider), 1);
	}
	bfm_onionskin_rm (disp); 
    }
  else
    {
      /* set onionskin to 0.7 */
      if (GTK_ADJUSTMENT(disp->bfm->sfm->onionskin_vslider)->value == 1)
	{
	  gtk_adjustment_set_value (GTK_ADJUSTMENT(disp->bfm->sfm->onionskin_vslider), .7);
	}
        onionskin_off = 0;
        bfm_onionskin_display (disp, ((GtkAdjustment*)fm->onionskin_vslider)->value,
                                fm->s_x, fm->s_y, fm->e_x, fm->e_y);
    }

  sfm_set_veto(0);
  return 1;
}

static gint 
sfm_onionskin_fgbg_callback (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;
  
  if (fm->bg == -1 || fm->bg == fm->fg)
    {
      e_printf ("ERROR: you must select a bg\n");
      return 1;
    }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin_cbtn)))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->onionskin_cbtn), TRUE); 

/*      bfm_onionskin_display (((GDisplay*) data), ((GtkAdjustment*)fm->onionskin_vslider)->value,
	  fm->s_x, fm->s_y, fm->e_x, fm->e_y); 
 */   }  

  if (((GtkAdjustment*)((GDisplay*)data)->bfm->sfm->onionskin_vslider)->value)
    gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_vslider), 0);
  else
    gtk_adjustment_set_value (GTK_ADJUSTMENT(((GDisplay*)data)->bfm->sfm->onionskin_vslider), 1);

  return 1;
}

void 
sfm_onionskin_set_offset (GDisplay* disp, int x, int y)
{
  layer_translate2 (disp->bfm->bg->active_layer, -x, -y,
                    disp->bfm->sfm->s_x, disp->bfm->sfm->s_y, 
		    disp->bfm->sfm->e_x, disp->bfm->sfm->e_y);
}

/**@brief Just set check button */
void 
sfm_onionskin_rm (GDisplay* disp)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(disp->bfm->sfm->onionskin_cbtn), FALSE); 
}



/*
 * STORE
 */
/**@brief get store by number */
static store*
sfm_store_get (GDisplay *disp, int num)
{
  store_frame_manager *fm = disp->bfm->sfm;
  store* store_item = (store*)(g_slist_nth (fm->stores, num))->data; 
  return store_item;
}

/**@brief removes all stores from fm->stores */
static void 
sfm_frames_remove (GDisplay* disp)
{
  store_frame_manager *fm;
  store *item;
  gint l;
  
  fm = disp->bfm->sfm;
  l = g_slist_length(fm->stores);

  while (l > 0)
    {
      item = sfm_store_get(disp, 0);  
      if (strcmp(item->gimage->filename, disp->gimage->filename) != 0) 
      {
        sfm_gimage_save (disp, item, TRUE);
      }

      sfm_store_remove (disp, sfm_store_get(disp, 0)); 
      l = g_slist_length(fm->stores); 
   }

  /* sfm is freed in the sfm_delete_callback */
  g_free(disp->bfm);
  disp->bfm=NULL;
}

void 
sfm_store_add_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay*)data;

  if(!disp)
    return;
  bfm_onionskin_rm (disp);
  sfm_store_add_dialog_create (disp); 
}

/**@brief delete store from stores list */
static void
sfm_store_remove (GDisplay *disp, store *store_item)
{
  store_frame_manager *fm = disp->bfm->sfm;
  fm->stores = g_slist_remove (fm->stores, store_item);  
}

/**@brief load and prepare according to reference an image in adv distance */
static GImage*
sfm_gimage_load (GDisplay *disp, store* store_item, int adv)
{
  store_frame_manager *fm = disp->bfm->sfm;
  int  f = filename_get_frame_number(store_item->gimage->filename);
  char* temp_path = NULL;
  int load_from_src_path = 0;
  GImage *img = NULL;
	  /* build filename to load */
	  if (fm->load_smart)
	    {
              FILE *fp;
              
	      temp_path = filename_build_path(
		  disp->bfm->dest_dir,
		  disp->bfm->dest_name,
		  f+adv);

              /* test for file existence */
              fp = fopen(temp_path,"r");
              if(fp)
                fclose(fp);
              else
                load_from_src_path = 1;
	    }
	  else
            load_from_src_path = 1;

          if (load_from_src_path)
            {
              if (temp_path) free(temp_path);
	      temp_path = filename_build_path(
		  disp->bfm->src_dir,
		  disp->bfm->src_name,
		  f+adv);
            }

	  /* find the current */
	  if (temp_path ==NULL)
	    {
	      g_message(_("Frame numbers can not be negative."));
	    }
          
  if ((temp_path!=NULL)&&(img=file_load_without_display (temp_path, prune_filename(temp_path), disp)))
    {
       free(temp_path);
    }

  return img;
}

/**@brief load from filename after testing for existance */
static GImage*
sfm_file_load_without_display (const char * full_name, const char * bn, 
                               GDisplay * disp)
{
  FILE *fp;
              
  /* test for file existence */
  fp = fopen(full_name,"r");
  if(fp) {
    fclose(fp);
    return file_load_without_display (full_name, bn, disp);
  }

  return NULL;
}

/**@brief save store at row; optionally delete gimage */
static void
sfm_gimage_save (GDisplay *disp, store *store_item, int delete)
{
  char* temp_path = 0;
  store_frame_manager *fm = disp->bfm->sfm;

  if (!store_item->readonly && store_item->gimage->dirty &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->autosave_cbtn)))
    {
      if (temp_path) free(temp_path);
      temp_path = filename_build_path(
                        disp->bfm->dest_dir,
                        disp->bfm->dest_name,
                        filename_get_frame_number(
                                      store_item->gimage->filename));
      file_save (store_item->gimage->ID, temp_path, prune_filename(temp_path));
    }

  if (delete)
    {    
      -- store_item->gimage->ref_count;
      gimage_delete(store_item->gimage);
    }

  if (temp_path) free(temp_path);
}

/**@brief remove frame (gimage, store, view list) at row and update UI */
static void
sfm_frame_remove (GDisplay *disp, int num, int update)
{
  store_frame_manager *fm = disp->bfm->sfm; 
  store *store_item;
  gint new_fg, i, old_fg;

  gint l = g_slist_length (fm->stores);

  if (l == 1)
    {
      e_printf ("ERROR: cannot rm last store\n");
      return;
    }
  
  if (update)
    bfm_onionskin_rm (disp);

  if (num >= l || num < 0)
    num = fm->fg;
  store_item = sfm_store_get(disp, num);

  /* last but not least */
  new_fg = fm->fg < l-1 ? fm->fg : fm->fg - 1;

  /* keep background */
  if (fm->fg == fm->bg)
    {
      store *si;

      fm->bg = new_fg;
      si = sfm_store_get(disp, fm->bg);
      si->bg = 1;
      #if 0
      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->bg, 3, "Bg"); 
      #endif
    }

  /* remove from list window */
  old_fg = fm->fg;
  if (update)
    {
      if (num <= new_fg)
        sfm_frame_make_cur (disp, new_fg + 1);
      else
        sfm_frame_make_cur (disp, new_fg);
    }
  gtk_clist_remove (GTK_CLIST(fm->store_list), num);

  sfm_gimage_save (disp, store_item, TRUE);

  sfm_store_remove (disp, store_item);

  fm->fg = old_fg;

  /* load foreground into display */
  if(update && num == fm->fg)
    sfm_store_make_cur (disp, new_fg);

  for (i=0; i<l-1; i++)
    {
      if ((sfm_store_get(disp, i))->bg)
        fm->bg = 1;
    } 
  
  /* update the gui */
  if (update)
    { /* update wait run */
      while (gtk_events_pending())
        gtk_main_iteration();

      sfm_flipbook_store_update(disp, new_fg);
    }
}



void 
sfm_store_delete_callback (GtkWidget *w, gpointer data)
{ int update = TRUE;
  sfm_frame_remove ((GDisplay *)data, -1, update); 
}

/**@brief move to earlier position in flipbook list */ 
void 
sfm_store_raise_callback (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay *)data)->bfm->sfm; 
  store *item;
  bfm_onionskin_rm ((GDisplay*)data);

  if (!fm->fg)
    return;

  gtk_clist_swap_rows (GTK_CLIST(fm->store_list), fm->fg-1, fm->fg);

  item = sfm_store_get((GDisplay *)data, fm->fg);
  sfm_store_remove ((GDisplay*)data, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg-1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg - 1 : fm->bg == fm->fg - 1 ? fm->fg : fm->bg;
  fm->fg --;
  
}

/**@brief move to later position in flipbook list */ 
void 
sfm_store_lower_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item;

  gint l = g_slist_length (fm->stores);
  bfm_onionskin_rm (disp);
  
  if (fm->fg == l-1)
    return;

  
  gtk_clist_swap_rows (GTK_CLIST(fm->store_list), fm->fg+1, fm->fg);

  item = sfm_store_get(disp, fm->fg);
  sfm_store_remove (disp, item);
 
  fm->stores = g_slist_insert (fm->stores, item, fm->fg+1);
  
  fm->bg = fm->bg == fm->fg ? fm->fg + 1 : fm->bg == fm->fg + 1 ? fm->fg : fm->bg;
  fm->fg ++;
}

void 
sfm_store_save_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;
  store_frame_manager *fm = disp->bfm->sfm; 
  char* temp_path;

  store *item = sfm_store_get(disp, fm->fg);
  bfm_onionskin_rm (disp);

  if (item->readonly)
    {
      e_printf ("ERROR: trying to save a readonly file \n");
      return;
    }

  temp_path = filename_build_path(
      ((GDisplay *)data)->bfm->dest_dir,
      ((GDisplay *)data)->bfm->dest_name,
      filename_get_frame_number(item->gimage->filename)
      );
  if (file_save (item->gimage->ID, temp_path, prune_filename(temp_path)))
	  gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 4, ""); 
  free(temp_path);
}

void 
sfm_store_save_all_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item;
  gint i, l = g_slist_length (fm->stores);
  char* temp_path;

  bfm_onionskin_rm (disp);

  for (i=0; i<l; i++)
    {
      item = sfm_store_get(disp, i);
      if (!item->readonly)
	{
	  temp_path = filename_build_path(
	      disp->bfm->dest_dir,
	      disp->bfm->dest_name,
	      filename_get_frame_number(item->gimage->filename)
	      );
	  if (file_save (item->gimage->ID, temp_path, prune_filename(temp_path)))
		 gtk_clist_set_text (GTK_CLIST (fm->store_list), i, 4, "");  
	  free(temp_path);
	}
    }
}

/**@brief revert to original from SRC_DIR */
void 
sfm_store_revert_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item = sfm_store_get(disp, fm->fg);

  if (file_load (item->gimage->filename,
	prune_filename (item->gimage->filename), (GDisplay *)data))
    {
      disp->ID = disp->gimage->ID;
      item->gimage = disp->gimage;
      sfm_store_make_cur (disp, fm->fg);

      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 5, prune_filename (item->gimage->filename));
      gtk_clist_set_text (GTK_CLIST (fm->store_list), fm->fg, 4, "");
    }
}

void
sfm_store_change_frame_callback (GtkWidget *w, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;
  store_frame_manager *fm = disp->bfm->sfm; 
  store *item = sfm_store_get(disp, fm->fg);
  char* temp_path;
  bfm_onionskin_rm (disp);


  if (item->gimage->dirty && !item->readonly)
    {
     
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->autosave_cbtn)))
	{
	  temp_path = filename_build_path(
	      disp->bfm->dest_dir,
	      disp->bfm->dest_name,
	      filename_get_frame_number(item->gimage->filename)
	      );
	  file_save (item->gimage->ID, temp_path, prune_filename(temp_path)); 
	  free(temp_path);
	}
      else
	printf ("WARNING: you just lost your changes to the file\n"); 
    }


  sfm_frame_change_dialog_create (disp);

}

void 
sfm_set_dir_src (GDisplay *disp)
{
  char* temp;
  store_frame_manager *fm = disp->bfm->sfm;
  temp = filename_build_label(disp->bfm->src_dir, disp->bfm->src_name);
  gtk_label_set_text(GTK_LABEL (fm->src_dir_label), temp);
  free(temp);
}

void 
sfm_store_recent_src_callback (GtkWidget *w, gpointer data)
{
}

void 
sfm_set_dir_dest (GDisplay *disp)
{
  char* temp;
  store_frame_manager *fm = disp->bfm->sfm;
  temp = filename_build_label(disp->bfm->dest_dir, disp->bfm->dest_name);
  gtk_label_set_text(GTK_LABEL (fm->dest_dir_label), temp);
  free(temp);
}

void 
sfm_store_recent_dest_callback (GtkWidget *w, gpointer data)
{
  bfm_onionskin_rm ((GDisplay*)data);
}

void 
sfm_store_load_smart_callback (GtkWidget *widget, gpointer data)
{
  GDisplay *disp = (GDisplay *)data;

  if(GTK_CHECK_MENU_ITEM (widget)->active)
    disp->bfm->sfm->load_smart = 1;
  else
    disp->bfm->sfm->load_smart = 0;
  bfm_onionskin_rm (disp);
}

/**@brief initialize variables, put in list, update view */
static store* 
sfm_frame_create (GDisplay* disp, GImage* img, int num, int active,
                  int readonly, int special, int update)
{
  store *new_store;
  store_frame_manager *fm = disp->bfm->sfm;
  char *text[6]={NULL, NULL, NULL, NULL, NULL, NULL}; 

  if(num < 0)
    num = fm->fg + 1;

  gtk_clist_insert (GTK_CLIST (fm->store_list), num, text);

  new_store = (store*) g_malloc (sizeof (store));
  new_store->readonly = readonly ? 1 : 0;
  new_store->advance = 1;
  new_store->flip = 1;
  new_store->bg = 0;
  new_store->gimage = img;
  new_store->special = special ? 1 : 0;
  new_store->remove = 1;


  if (active)
    sfm_frame_make_cur(disp,num);

  if (!g_slist_length (fm->stores))
    {
      bfm_set_dir_src (disp, img->filename);
      bfm_set_dir_dest (disp, img->filename);

      sfm_set_dir_src (disp);
      sfm_set_dir_dest (disp);
    }

  fm->stores = g_slist_insert (fm->stores, new_store, num);

  if (update)
    sfm_flipbook_store_update(disp, num);

  return new_store;
}

/**@brief set frame to row; update list view and display from store */
static void
sfm_frame_make_cur (GDisplay *disp, int row)
{
  int i;

  gtk_clist_select_row (GTK_CLIST(disp->bfm->sfm->store_list), row, 5);

  /* update wait run */
#if GTK_MAJOR_VERSION < 2
  for( i = 0; i < 12; ++i)
#endif
    while (gtk_events_pending())
      gtk_main_iteration();
}

/**@brief set fm->fg; update display from store */
static void
sfm_store_make_cur (GDisplay *gdisplay, int row)
{
  store *item;
  store_frame_manager *fm;
  char tmp[256];
  SplineTool *spline_tool=NULL;

  if (row == -1)
    return;

  fm = gdisplay->bfm->sfm;
  fm->fg = row;

  if (g_slist_length (fm->stores) <= row)
    return;

  item = sfm_store_get(gdisplay, row);

  item->fg = 1;

  /* shut down any spline tools associated with the current display's gimage. */
  /* added 21December2004 - twh */
  if(active_tool->type==SPLINE)
    {
      if(active_tool->private)
	{
	  spline_tool=(SplineTool*)active_tool->private;
	  if(spline_tool->active_spline && spline_tool->active_spline->drawn)
	      spline_control(active_tool, PAUSE, gdisplay);
	}
    }

  /* display the new store */
  gdisplay->gimage = item->gimage;
  gdisplay->ID = item->gimage->ID;
  gdisplays_update_title (gdisplay->gimage->ID);
  gdisplay_add_update_area (gdisplay, fm->s_x, fm->s_y, fm->e_x, fm->e_y);
#if 0
  if (active_tool->type == CLONE)
    clone_flip_image ();
#endif
  bfm_set_fg (gdisplay, sfm_store_get(gdisplay, fm->fg)->gimage);

  sprintf (tmp, _("Flipbook for %s"), gdisplay->gimage->filename);
  gtk_window_set_title (GTK_WINDOW (gdisplay->bfm->sfm->shell), tmp);
  /* restart any spline tools associated with the current display's new gimage. */
  /* added 21December2004 - twh */
  if(active_tool->type==SPLINE)
    {
      if(item->gimage->active_layer->sl.spline_tool)
	{
	  spline_tool=(SplineTool*)item->gimage->active_layer->sl.spline_tool;
	  active_tool->private=spline_tool;
	  if(spline_tool->active_spline && !spline_tool->active_spline->drawn)
	      spline_control(active_tool, RESUME, gdisplay);
	}
    }

  /*  inform the observers of the new current image [hsbo]  */
  if (!fm->play)
    sfm_update_extern_image_observers (gdisplay->gimage);

  gdisplays_flush ();
}

gint
sfm_plays (GDisplay *disp)
{
   if (!disp->bfm) {
	   return 0;
   }
   if (!disp->bfm->sfm)
	   return 0;
   return disp->bfm->sfm->play;
}

static void 
sfm_store_select_callback (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;
  
  if (!gdisplay || row == -1)
    return;

  bfm_onionskin_rm (gdisplay);

  if (col != 5 && col != -1)
    {
      sfm_frame_flipbook_update (gdisplay, row, col); 
      sfm_frame_make_cur (gdisplay, gdisplay->bfm->sfm->fg);
      return; 
    }

  sfm_store_make_cur (gdisplay, row);
}

static void 
sfm_store_unselect_callback (GtkCList *w, gint row, gint col, 
    GdkEventButton *event, gpointer client_pointer) 
{

  GDisplay *gdisplay = (GDisplay*) client_pointer;
  store_frame_manager *fm;
  bfm_onionskin_rm (gdisplay);
 
  if (!gdisplay)
    return;
  
  if (!gdisplay->bfm)
    return;

  fm = gdisplay->bfm->sfm;

 /* 
  sfm_frame_make_cur (gdisplay, fm->fg); 
*/
  if (col == 5)
    {
      sfm_store_get(gdisplay, fm->fg)->fg = 0;
  fm->fg = -1;
    }
}

/**@brief update list view */
void
sfm_flipbook_store_update_full (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  int l = g_slist_length (fm->stores), num;

  if (l <= 0) return;

  for (num = 0; num < l; ++num)
      sfm_flipbook_store_update(disp, num);
}

/**@brief update list view for image at a row;  < 0 means fg */
void
sfm_flipbook_store_update (GDisplay *disp, int num)
{
  store_frame_manager *fm = disp->bfm->sfm;
  int l = g_slist_length (fm->stores);
  store *store_item;

  if (l <= 0)
    return;

  if (num >= l || num < 0)
    num = fm->fg;
  store_item = sfm_store_get(disp, num);

  if (store_item->readonly)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, _("RO"));
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 0, "");
  if (store_item->advance) 
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 1, "A");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 1, "");
  if (store_item->flip)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 2, "F");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 2, "");
  if (store_item->bg)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 3, _("Bg"));
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 3, "");
  if (store_item->gimage->dirty)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 4, "");
  gtk_clist_set_text (GTK_CLIST (fm->store_list), num, 5,
                      prune_filename (store_item->gimage->filename));

}

/**@brief set list view of image at pos dirty;  < 0 means fg */
void
sfm_dirty (GDisplay* disp, int pos)
{
  int l = g_slist_length (disp->bfm->sfm->stores);

  if (pos >= l || pos < 0)
    pos = disp->bfm->sfm->fg;

  if (disp->gimage->dirty)
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), pos, 4, "*");
  else
    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), pos, 4, "");

}

GDisplay *
sfm_load_image_into_fm (GDisplay* disp, GImage *img)
{
  store_frame_manager *fm = disp->bfm->sfm; 
  int special = 1;

  sfm_frame_create (disp, img, fm->fg+1, 1, 1, special, 1);
  return disp;
}

/**@brief set background at pos; -1 means unset */
void
sfm_set_bg (GDisplay *disp, int pos)
{
  store_frame_manager *fm = disp->bfm->sfm;
  store *store_item;
  int l = g_slist_length (fm->stores), i;

  for (i = 0; i < l; ++i)
    {
      store_item = sfm_store_get(disp, i);
      if (i == pos)
        store_item->bg = 1;
      else
        store_item->bg = 0;
    }

  if (pos >= 0)
    gtk_clist_set_text (GTK_CLIST (fm->store_list), pos, 3, _("Bg"));
  fm->bg = pos;
}

/**@brief parse user settings from the flipbook, update gui */ 
static void 
sfm_frame_flipbook_update (GDisplay *gdisplay, gint row, gint col)
{
  store_frame_manager *fm;
  store *item;
  int bg = -1;
  
  fm = gdisplay->bfm->sfm;

# if 0
      /* update wait run */
      while (gtk_events_pending())
        gtk_main_iteration();
# endif

  if(-1 == row)
    return;
  
  item = sfm_store_get(gdisplay, row);
  
  switch (col)
    {
    case 0:
     item->readonly = item->readonly ? 0 : 1;
      break;
    case 1:
     item->advance = item->advance ? 0 : 1; 
      break;
    case 2:
     item->flip = item->flip ? 0 : 1; 
      break;
    case 3:
      if (fm->bg != -1 && fm->bg != row /*fm->fg*/)
	{
	  sfm_store_get(gdisplay, fm->bg)->bg = 0;
          sfm_flipbook_store_update (gdisplay, fm->bg);
	  fm->bg = bg = -1;
	  bfm_set_bg (gdisplay, 0); 
	}
      item->bg = item->bg ? 0 : 1; 
      if (item->bg)
	{
	  fm->bg = row;
      
	  bfm_set_bg (gdisplay, sfm_store_get(gdisplay, fm->bg)->gimage);
	  
	}
      else
	{
	  fm->bg = -1;
	  bfm_set_bg (gdisplay, 0); 
	}
      break;
    default:
      break;
      
    }
  sfm_flipbook_store_update (gdisplay, row);
}

static void 
sfm_store_set_option_callback (GtkCList *w, gint col, gpointer client_pointer)
{
  GDisplay *gdisplay = (GDisplay*) client_pointer;

  bfm_onionskin_rm (gdisplay);

  if (!gdisplay)
    return;

  sfm_frame_flipbook_update (gdisplay, gdisplay->bfm->sfm->fg, col);

}


/*
 *
 */
static gint 
sfm_auto_save_callback (GtkWidget *w, gpointer data)
{
  bfm_onionskin_rm ((GDisplay*)data);
  return 1;
}

static ToolType tool=-1;

static gint 
sfm_set_aofi_callback (GtkWidget *w, gpointer data)
{

  GDisplay *disp;
      
  disp = (GDisplay*)data;
  bfm_onionskin_rm (disp);

  if (tool == -1)
    {
     if (disp->select) 
      disp->select->hidden = FALSE;
      tool = active_tool->type;
      tools_select (RECT_SELECT);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(((GDisplay*)data)->bfm->sfm->aofi_cbtn), TRUE);
    }
  else
    {
      tools_select (tool);

      tool = -1; 

      selection_invis (disp->select);
      selection_layer_invis (disp->select);
      disp->select->hidden = disp->select->hidden ? FALSE : TRUE;
      
      selection_start (disp->select, TRUE);

      gdisplays_flush ();
    }
  return 1; 
}

void
sfm_setting_aofi(GDisplay *disp)
{
  RectSelect * rect_sel;
  double scale;
  gint s_x, s_y, e_x, e_y;
  store_frame_manager *fm;
  bfm_onionskin_rm (disp);

  if (tool == -1)
    return;

  if (active_tool->type == RECT_SELECT)
    {
      /* get position */
      rect_sel = (RectSelect *) active_tool->private;
      s_x = MIN (rect_sel->x, rect_sel->x + rect_sel->w);
      s_y = MIN (rect_sel->y, rect_sel->y + rect_sel->h);
      e_x = MAX (rect_sel->x, rect_sel->x + rect_sel->w) - s_x;
      e_y = MAX (rect_sel->y, rect_sel->y + rect_sel->h) - s_y;

      if ((!s_x && !s_y && !e_x && !e_y) ||
	  (!rect_sel->w && !rect_sel->h))
	{
	  scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));
	  s_x = 0;
	  s_y = 0;
	  e_x = disp->gimage->width;
	  e_y = disp->gimage->height;
	}
      fm = disp->bfm->sfm;

      fm->s_x = fm->sx = s_x;
      fm->s_y = fm->sy = s_y;
      fm->e_x = fm->ex = e_x;
      fm->e_y = fm->ey = e_y;
    }

}

static gint 
sfm_aofi_callback (GtkWidget *w, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*)data)->bfm->sfm;
  bfm_onionskin_rm ((GDisplay*)data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fm->aofi_cbtn)))
    {
      fm->s_x = fm->sx;
      fm->s_y = fm->sy;
      fm->e_x = fm->ex;
      fm->e_y = fm->ey;
    }
  else
    {
    sfm_unset_aofi ((GDisplay*)data);
    }
  return 1;
}

static void
sfm_unset_aofi (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  double scale;
  bfm_onionskin_rm (disp);

  scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));
  fm->s_x = 0;
  fm->s_y = 0;
  fm->e_x = disp->gimage->width;
  fm->e_y = disp->gimage->height;
}

/*
 * ADD STORE
 */
/**@brief add frames to flipbook; update UI */
static void
sfm_stores_add (GDisplay* disp)
{
  gint row, f, i;
  GImage *gimage; 
  store_frame_manager *fm = disp->bfm->sfm;
  gint num_to_add = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(fm->num_to_add_spin));
  store *item, *s;
  char* temp_path = 0;

  row = fm->fg;
  item = sfm_store_get(disp, row);

  switch (type_to_add_var)
    {
    case 1:
      f = filename_get_frame_number(item->gimage->filename);
      for (i=0; i<num_to_add; i++)
	{

	  if (fm->load_smart)
	    {
	      if (temp_path) free(temp_path);
	      temp_path = filename_build_path(
		  disp->bfm->dest_dir,
		  disp->bfm->dest_name,
		  f-(i+1));
	    }
	  else 
	    {
	      if (temp_path) free(temp_path);
	      temp_path = filename_build_path(
		  disp->bfm->src_dir,
		  disp->bfm->src_name,
		  f-(i+1));
	    }


	  /* find the current */
	  if (temp_path ==NULL)
	    {
	      g_message(_("Frame numbers can not be negative."));
	    }
	  if ((temp_path!=NULL)&&(gimage=file_load_without_display (temp_path, prune_filename(temp_path), disp)))
	    {
	      sfm_frame_create (disp, gimage, row+(i+1), 0, fm->readonly, 0, 1);
	    }
	  else
	    if (fm->load_smart)
	      {
		if (temp_path) free(temp_path);
		temp_path = filename_build_path(
		    disp->bfm->src_dir,
		    disp->bfm->src_name,
		    f-(i+1));
		if ((gimage=file_load_without_display (temp_path, prune_filename(temp_path), disp)))
		  {
		    sfm_frame_create (disp, gimage, row+(i+1), 0, fm->readonly, 0, 1);
		  }
	      }
	}
      break;
    case 0:
      f = filename_get_frame_number(item->gimage->filename);

      for (i=0; i<num_to_add; i++)
	{
	  if (fm->load_smart)
	    {
	      if (temp_path) free(temp_path);
	      temp_path = filename_build_path(
		  disp->bfm->dest_dir,
		  disp->bfm->dest_name,
		  f+(i+1));
	    }
	  else 
	    {
	      if (temp_path) free(temp_path);
	      temp_path = filename_build_path(
		  disp->bfm->src_dir,
		  disp->bfm->src_name,
		  f+(i+1));
	    }

	  /* find the current */
	  if (temp_path ==NULL)
	    {
	      g_message(_("Frame numbers can not be negative."));
	    }

	  if ((temp_path!=NULL)&&(gimage=sfm_file_load_without_display (temp_path, prune_filename(temp_path), disp)))
	    {
	      sfm_frame_create (disp, gimage, row+(i+1), 0, fm->readonly, 0, 1);
	    }
	  else
	    if (fm->load_smart)
	      {
		if (temp_path) free(temp_path);
		temp_path = filename_build_path(
		    disp->bfm->src_dir,
		    disp->bfm->src_name,
		    f+(i+1));
		if ((gimage=sfm_file_load_without_display (temp_path, prune_filename(temp_path), disp)))
		  {
		    sfm_frame_create (disp, gimage, row+(i+1), 0, fm->readonly, 0,1);
		  }
	      }

	}
      break;
    case 2:
      gimage = item->gimage;

      /* copy selected frame and continuously increase file number */
      for (i=0; i<num_to_add; i++)
	{
          char *new_fn;
          int len;
          GImage *gim = gimage_copy (gimage);

          gimage_flatten( gim );
	  sfm_frame_create (disp, gim, row+i+1, 0, fm->readonly,0,1);

          f = filename_get_frame_number(item->gimage->filename);
          if (temp_path) free(temp_path);
          temp_path = filename_build_path(disp->bfm->src_dir,
					  disp->bfm->src_name,
					  f+(i+1));
          new_fn = prune_filename(temp_path);
          s = sfm_store_get( disp, row+i+1 );
          len = strlen( s->gimage->filename ) + 1;
          if(s->gimage->filename) free( s->gimage->filename );
          s->gimage->filename = malloc( len );
          sprintf( s->gimage->filename, new_fn );
          sfm_flipbook_store_update( disp, row+i+1 );
	}
      break;
    }
  if (temp_path) free(temp_path);

}

static void
sfm_stores_add_dialog_ok_callback (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->add_dialog))
	{
	  gtk_widget_hide (fm->add_dialog);
	  sfm_stores_add ((GDisplay*) client_data);
	}
    }
}

static gint
sfm_store_add_dialog_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;


  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->add_dialog))
	{
	  gtk_widget_hide (fm->add_dialog);
	}
    }
  return TRUE;
}

static void
sfm_store_add_readonly_callback (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;
  fm->readonly = fm->readonly ? 0 : 1; 
}

static void
frame_manager_add_dialog_option_callback (GtkWidget *w, gpointer client_data)
{
  type_to_add_var = (int) client_data;
}


static void
sfm_store_add_dialog_create (GDisplay *disp)
{
  store_frame_manager *fm = NULL;
  int i;
  GtkWidget *hbox, *radio_button, *label, *check_button;

  GSList *group = NULL;
  char *options[3] =
    {
      "Load next frames",
      "Load previous frames",
      "Load copies of current frame"
    };

  static ActionAreaItem offset_action_items[1] =
    {
	{ "OK", sfm_stores_add_dialog_ok_callback, NULL, NULL },
    };

  if (sfm_check (disp)) return;
  fm = disp->bfm->sfm;

  options[0] = _("Load next frames");
  options[1] = _("Load previous frames");
  options[2] = _("Load copies of current frame");
  offset_action_items[0].label = _("OK");

  if (!fm->add_dialog)
    {
      fm->add_dialog = gtk_dialog_new ();
      gtk_object_ref(GTK_OBJECT(fm->add_dialog));
      gtk_window_set_wmclass (GTK_WINDOW (fm->add_dialog), "Store Option", PROGRAM_NAME);
      gtk_window_set_policy (GTK_WINDOW (fm->add_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->add_dialog), _("New Store Option"));

      gtk_signal_connect (GTK_OBJECT (fm->add_dialog), "delete_event",
	  GTK_SIGNAL_FUNC (sfm_store_add_dialog_delete_callback),
	  disp);

#if 0
      gtk_widget_set_uposition(fm->add_dialog, generic_window_x, generic_window_y);
      layout_connect_window_position(fm->add_dialog, &generic_window_x, &generic_window_y);
      minimize_register(fm->add_dialog);
#endif

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox),
	  hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
  
      /* readonly */
      check_button = gtk_check_button_new_with_label (_("readonly"));
      gtk_signal_connect (GTK_OBJECT (check_button), "clicked",
	  (GtkSignalFunc) sfm_store_add_readonly_callback,
	  disp);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox), check_button, FALSE, FALSE, 0);
      gtk_widget_show (check_button);

      /* num of store */
      label = gtk_label_new (_("Add"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      disp->bfm->sfm->num_to_add_spin = gtk_spin_button_new (
	  GTK_ADJUSTMENT(gtk_adjustment_new (1, 1, 100, 1, 1, 0)), 1.0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), disp->bfm->sfm->num_to_add_spin, FALSE, FALSE, 2);

      gtk_widget_show (disp->bfm->sfm->num_to_add_spin);
      
      label = gtk_label_new (_("stores"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);
      
      /* radio buttons */
      for (i = 0; i < 3; i++)
	{
	  radio_button = gtk_radio_button_new_with_label (group, options[i]);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
	  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
	      (GtkSignalFunc) frame_manager_add_dialog_option_callback,
	      (void *)((long) i));
	  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->add_dialog)->vbox), radio_button, FALSE, FALSE, 0);
	  gtk_widget_show (radio_button);
	}

      offset_action_items[0].user_data = disp;
      build_action_area (GTK_DIALOG (fm->add_dialog), offset_action_items, 1, 0);

      gtk_widget_show (fm->add_dialog);
    }
  else
    {
    if (!GTK_WIDGET_VISIBLE (fm->add_dialog))
      {
	gtk_widget_show (fm->add_dialog);
      }
    }
}

void 
sfm_slide_show_callback (GtkWidget *w, gpointer data)
{
  GDisplay *display=NULL;
  GtkWidget *window;
  static gint win_x, win_y, win_w, win_h, win_d;

  display = gdisplay_active();

  if(display)
    {
      static int rulers_visible = 0;
      window = display->shell;
      display->slide_show = display->slide_show ? 0 : 1;
      if(display->slide_show)
	{
          if(GTK_WIDGET_VISIBLE(display->hrule))
            rulers_visible = 1;
	  gdk_window_get_geometry(window->window, &win_x, &win_y, &win_w, &win_h, &win_d);

	  gtk_widget_hide(display->menubar);
	  gtk_widget_hide(display->hsb);
	  gtk_widget_hide(display->vsb);
	  gtk_widget_hide(display->hrule);
	  gtk_widget_hide(display->vrule);

	  gdk_window_move_resize (window->window, -50, -75, gdk_screen_width()+92, 
				  gdk_screen_height()+105);
	  gtk_window_set_default_size (GTK_WINDOW (window), gdk_screen_width()+92,
				       gdk_screen_height()+105);
	  gtk_widget_set_usize (GTK_WIDGET(window), gdk_screen_width()+92, 
				gdk_screen_height()+105);
	}
      else
	{
	  gdk_window_move_resize (window->window, win_x, win_y, win_w, win_h);
	  gtk_window_set_default_size (GTK_WINDOW (window), win_w, win_h);
	  gtk_widget_set_usize (GTK_WIDGET(window), win_w, win_h);

	  gtk_widget_show(display->menubar);
	  gtk_widget_show(display->hsb);
	  gtk_widget_show(display->vsb);
          if(rulers_visible)
          {
            gtk_widget_show(display->hrule);
	    gtk_widget_show(display->vrule);
          }
	}
      gdisplays_flush();
    }
}

/*
 * CHANGE FRAME NUMBER 
 */

/**@brief load a new image into this frame */
static void
sfm_frame_change (GDisplay* disp)
{
  char* temp_path = 0;
  char* temp_name;

  store_frame_manager *fm = disp->bfm->sfm;
  store *item = sfm_store_get(disp, fm->fg);
  int new_frame = atoi (gtk_editable_get_chars (GTK_EDITABLE(fm->change_to_entry), 0, -1));

  if (fm->load_smart) 
    temp_path = filename_build_path(
        disp->bfm->dest_dir,
	disp->bfm->dest_name,
	new_frame);
  else
    temp_path = filename_build_path(
        disp->bfm->src_dir,
	disp->bfm->src_name,
	new_frame);
  temp_name = prune_filename(temp_path);
  if (file_load (temp_path, temp_name, disp))
    {
      disp->ID = disp->gimage->ID;
      item->gimage = disp->gimage;
      sfm_store_make_cur (disp, fm->fg);
      gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg, 
	  5, temp_name);

    }
  else
    if (fm->load_smart)
      {
	if (temp_path) free(temp_path);
	temp_path = filename_build_path(
            disp->bfm->src_dir,
	    disp->bfm->src_name,
	    new_frame);
	temp_name = prune_filename(temp_path);
	if (file_load (temp_path, temp_name, disp))
	  {
	    disp->ID = disp->gimage->ID;
	    item->gimage = disp->gimage;
	    sfm_store_make_cur (disp, fm->fg);

	    gtk_clist_set_text (GTK_CLIST (disp->bfm->sfm->store_list), disp->bfm->sfm->fg,
		5, temp_name);

	  }

     } 
  
  if (temp_path) free(temp_path);
}

static void
sfm_store_change_frame_dialog_ok_callback (GtkWidget *w, gpointer client_data)
{
  store_frame_manager *fm = ((GDisplay*) client_data)->bfm->sfm;

  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
	{
	  gtk_widget_hide (fm->chg_frame_dialog);
	  sfm_frame_change ((GDisplay*) client_data);
	}
    }
}

void 
sfm_filter_apply_callback (GtkWidget *w, gpointer data)
{
  Argument *args;
  GDisplay *display=(GDisplay*)data;
  int i;
  gint start_frame;

  bfm_onionskin_rm ((GDisplay*)data);

  start_frame=display->bfm->sfm->fg;

  if (wire_buffer->last_plug_in)
    {
      /* construct the procedures arguments */
      args = g_new (Argument, wire_buffer->last_plug_in->num_args);
      memset (args, 0, (sizeof (Argument) * wire_buffer->last_plug_in->num_args));

      /* initialize the argument types */
      for (i = 0; i < wire_buffer->last_plug_in->num_args; i++)
	args[i].arg_type = wire_buffer->last_plug_in->args[i].arg_type;

      sfm_flip_forward(display);
      while(start_frame != display->bfm->sfm->fg)
	{
	  /* initialize the first 3 plug-in arguments  */
	  args[0].value.pdb_int = RUN_WITH_LAST_VALS;
	  args[1].value.pdb_int = display->gimage->ID;
	  args[2].value.pdb_int = drawable_ID (gimage_active_drawable (display->gimage));

	  /* run the plug-in procedure */
	  plug_in_run (wire_buffer->last_plug_in, args, FALSE, TRUE);

	  /* advance to the next store */
	  sfm_flip_forward(display);
	}
      g_free (args);
    }
} 

static gint
sfm_store_change_frame_dialog_delete_callback (GtkWidget *w, GdkEvent *e, gpointer data)
{
  store_frame_manager *fm = ((GDisplay*) data)->bfm->sfm;


  if (fm)
    {
      if (GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
	{
	  gtk_widget_hide (fm->chg_frame_dialog);
	}
    }
  return TRUE;
}

/**@brief dialog to load a image from disk into existing frame */
static void
sfm_frame_change_dialog_create (GDisplay *disp)
{
  store_frame_manager *fm = disp->bfm->sfm;
  GtkWidget *hbox, *label;
  char *temp;
  store *item = sfm_store_get(disp, fm->fg);

  static ActionAreaItem offset_action_items[1] =
    {
	{ "OK", sfm_store_change_frame_dialog_ok_callback, NULL, NULL },
    };

  offset_action_items[0].label = _("OK");

  if (!fm)
    return;

  if (!fm->chg_frame_dialog)
    {
      fm->chg_frame_dialog = gtk_dialog_new ();
      gtk_object_ref(GTK_OBJECT(fm->chg_frame_dialog));
      gtk_window_set_wmclass (GTK_WINDOW (fm->chg_frame_dialog), _("Store Option"), PROGRAM_NAME);
      gtk_window_set_policy (GTK_WINDOW (fm->chg_frame_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (fm->chg_frame_dialog), _("New Store Option"));

      gtk_signal_connect (GTK_OBJECT (fm->chg_frame_dialog), "delete_event",
	  GTK_SIGNAL_FUNC (sfm_store_change_frame_dialog_delete_callback),
	  disp);

#if 0
      gtk_widget_set_uposition(fm->add_dialog, generic_window_x, generic_window_y);
      layout_connect_window_position(fm->add_dialog, &generic_window_x, &generic_window_y);
      minimize_register(fm->add_dialog);
#endif

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fm->chg_frame_dialog)->vbox),
	  hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
  

      temp = filename_get_pre(item->gimage->filename);
      label = gtk_label_new (temp);
      free(temp);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      fm->change_to_entry = gtk_entry_new ();
      temp = filename_get_frame(item->gimage->filename);
      gtk_entry_set_text (GTK_ENTRY (fm->change_to_entry), temp);
      free(temp);
      gtk_box_pack_start (GTK_BOX (hbox), fm->change_to_entry, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (fm->change_to_entry), "activate",
	  (GtkSignalFunc) sfm_store_change_frame_dialog_ok_callback,
	  disp);
      gtk_widget_show (fm->change_to_entry);

      temp = filename_get_post(item->gimage->filename);
      label = gtk_label_new (temp);
      free(temp);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);

      offset_action_items[0].user_data = disp;
      build_action_area (GTK_DIALOG (fm->chg_frame_dialog), offset_action_items, 1, 0);

      gtk_widget_show (fm->chg_frame_dialog);
    }
  else
    {
    if (!GTK_WIDGET_VISIBLE (fm->chg_frame_dialog))
      {
	gtk_widget_show (fm->chg_frame_dialog);
      }
    }
}

void 
sfm_dialog_create ()
{
  GDisplay *display;
  GImage *gimage; 
  char* temp_path = 0;
  gint row, f, i;
  store_frame_manager *fm;
  store *item;

  if(display_list != NULL)
    {
      display=(GDisplay *)display_list->data;
      bfm_create_sfm(display);
      fm = display->bfm->sfm;
      row = fm->fg;
      item = sfm_store_get(display, row);
      bfm_onionskin_rm(display);
      f = filename_get_frame_number(item->gimage->filename);
      for (i=0; i<(initial_frames_loaded-1); i++)
	{
	  if (temp_path) free(temp_path);
	  temp_path = filename_build_path(display->bfm->src_dir,
					  display->bfm->src_name,
					  f+(i+1));

	  /* find the current */
	  if ((gimage=file_load_without_display (temp_path, prune_filename(temp_path), display)))
	    {
	      sfm_frame_create (display, gimage, row+(i+1),0, fm->readonly,0,1);
	    }
	}       
    }
  else
    {
    printf("No gdisplays available.\n");
    printf("Starting CinePaint without Frame Manger.\n");
    printf("You may manually create a Frame Manager once you have loaded an image.\n");
    }  
}


void
sfm_filter_apply_last_callback(GtkWidget *widget, void* data)
{
  GSList *list=image_list;
  GImage *image;
  Argument *args;
  int i;

  while(list)
    {
      if (wire_buffer->last_plug_in)
   {
     image=(GImage*)list->data;
     /* construct the procedures arguments */
     args = g_new (Argument, wire_buffer->last_plug_in->num_args);
     memset (args, 0, (sizeof (Argument) * wire_buffer->last_plug_in->num_args));

     /* initialize the argument types */
     for (i = 0; i < wire_buffer->last_plug_in->num_args; i++)
       args[i].arg_type = wire_buffer->last_plug_in->args[i].arg_type;

     /* initialize the first 3 plug-in arguments  */
     args[0].value.pdb_int = (RUN_WITH_LAST_VALS);
     args[1].value.pdb_int = image->ID;
     args[2].value.pdb_int = drawable_ID(gimage_active_drawable (image));

     /* run the plug-in procedure */
     plug_in_run (wire_buffer->last_plug_in, args, FALSE, TRUE);
     
     g_free (args);
   }
      list=g_slist_next(list);
    }  
  gdisplays_flush();
}

void
sfm_layer_new_callback(GtkWidget *widget, void* data)
{
  GSList *list=image_list;
  Layer *new_layer;
  GImage *image;

  while(list)
    {
      image=(GImage*)list->data;
      new_layer=layer_new(image->ID, image->width, image->height,
             tag_set_alpha(gimage_tag(image), ALPHA_YES),
#ifdef NO_TILES                          
             STORAGE_FLAT,
#else
             STORAGE_TILED,
#endif
             _("New Layer"), 
             1.0f, NORMAL_MODE);
      if(new_layer)
   {
     drawable_fill(GIMP_DRAWABLE(new_layer), TRANSPARENT_FILL);
     gimage_add_layer(image, new_layer, -1);
   }
      list=g_slist_next(list);
    }

  gdisplays_flush();
}

void
sfm_layer_copy_callback(GtkWidget *widget, void* data)
{
  GSList *list=image_list;
  Layer *new_layer;
  GImage *image;

  while(list)
    {
      image=(GImage*)list->data;
      new_layer=layer_copy(image->active_layer, TRUE);
      if(new_layer)
      {
        gimage_add_layer(image, new_layer, -1);
      }
      list=g_slist_next(list);
    }

  gdisplays_flush();
}

void
sfm_layer_mode_callback(GtkWidget *widget, void* data)
{
  GSList *list=image_list;
  Layer *layer;
  GImage *image;

  while(list)
    {
      image=(GImage*)list->data;
      layer=image->active_layer;
      if(layer)
      {
        layer->mode=ADDITION_MODE;
        drawable_update(GIMP_DRAWABLE(layer), 0, 0, 0, 0);
      }
      list=g_slist_next(list);
    }

  gdisplays_flush();
}

void
sfm_layer_merge_visible_callback(GtkWidget *widget, void* data)
{
  GSList *list=image_list;
  GImage *image;

  while(list)
    {
      image=(GImage*)list->data;
      if(image)
      {
        gimage_merge_visible_layers(image, GIMP_CLIP_TO_IMAGE, 0);
        drawable_update(GIMP_DRAWABLE(image->active_layer), 0, 0, 0, 0);
      }
      list=g_slist_next(list);
    }

  gdisplays_flush();
}

/**@brief Position of image with `ID' in sfm, or -1 if not present  -hsbo */
int 
sfm_pos_of_ID (GDisplay* gdisp, int imageID)
{
  store_frame_manager* fm;
  int i;
  
  if (!gdisp || !gdisp->bfm) return -1;
    
  fm = gdisp->bfm->sfm;
  
  for (i=0; i < g_slist_length (fm->stores); i++)
    {
      if (sfm_store_get(gdisp, i)->gimage->ID == imageID)
        return i;
    }
  return -1;  
}

