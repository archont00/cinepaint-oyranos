/*
 *   Print plug-in for the CinePaint/GIMP.
 *
 *   Copyright 2003-2004 Kai-Uwe Behrmann
 *
 *   compiles for cinepaint
 *   added cms support with lcms (actually an separate dialog)
 *   supports 16-bit integers and 32-bit floats
 *    Kai-Uwe Behrman (web@tiscali.de) --  February - October 2003
 *   add capability to save tiffs with embedded profile
 *    Kai-Uwe Behrman (web@tiscali.de) --  December 2003
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

char version_[128] = "ICC print plug-in v0.4.2-alpha";

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <lcms.h>
#include "print_gimp.h"

#include "print.h"
#include "print-lcms-funcs.h"
#include "print-lcms-options.h"
#include "print-lcms-options-callbacks.h"

#ifndef DEBUG
// #define DEBUG
#endif

#ifdef DEBUG
 #define DBG printf("%s:%d %s()\n",__FILE__,__LINE__,__func__);
#else
 #define DBG 
#endif


// definitions for compatibillity
#if GIMP_MAJOR_VERSION == 0 && GIMP_MINOR_VERSION > 0 && GIMP_MINOR_VERSION < 17
                            // filmgimp
    #define GimpPDBStatusType GStatusType
    #define GIMP_PDB_SUCCESS STATUS_SUCCESS
    #define GIMP_PDB_STATUS PARAM_STATUS
    #define GIMP_PDB_CALLING_ERROR STATUS_CALLING_ERROR
    #define GIMP_PDB_EXECUTION_ERROR STATUS_EXECUTION_ERROR
    #define GIMP_RUN_INTERACTIVE RUN_INTERACTIVE
    #define GIMP_RUN_NONINTERACTIVE RUN_NONINTERACTIVE
    #define GIMP_RUN_WITH_LAST_VALS RUN_WITH_LAST_VALS
    #define GimpImageType GimpDrawableType
  #define _(x) x
#else
  #ifdef GIMP_VERSION          // gimp
    //#define GParamDef GimpParamDef
    //#define GParam GimpParam
    //#define GRunModeType GimpRunModeType
    //#define GStatusType GimpStatusType
    #define GRAY GIMP_GRAY 
    #define GRAY_IMAGE GIMP_GRAY_IMAGE 
    #define GRAYA_IMAGE GIMP_GRAYA_IMAGE 
    #define INDEXED GIMP_INDEXED 
    #define INDEXED_IMAGE GIMP_INDEXED_IMAGE 
    #define INDEXEDA_IMAGE GIMP_INDEXEDA_IMAGE 
    #define RGB GIMP_RGB
    #define RGB_IMAGE GIMP_RGB_IMAGE 
    #define RGBA_IMAGE GIMP_RGBA_IMAGE 
    #define NORMAL_MODE GIMP_NORMAL_MODE 
    #define DARKEN_ONLY_MODE GIMP_DARKEN_ONLY_MODE 
    #define LIGHTEN_ONLY_MODE GIMP_LIGHTEN_ONLY_MODE 
    #define HUE_MODE GIMP_HUE_MODE 
    #define SATURATION_MODE GIMP_SATURATION_MODE 
    #define COLOR_MODE GIMP_COLOR_MODE 
    #define MULTIPLY_MODE GIMP_MULTIPLY_MODE 
    #define SCREEN_MODE GIMP_SCREEN_MODE 
    #define DISSOLVE_MODE GIMP_DISSOLVE_MODE 
    #define DIFFERENCE_MODE GIMP_DIFFERENCE_MODE 
    #define VALUE_MODE GIMP_VALUE_MODE 
    #define ADDITION_MODE GIMP_ADDITION_MODE 
    #define GProcedureType GimpPDBProcType
  #else                     // cinepaint
    #define gimp_procedural_db_proc_info gimp_query_procedure
    // this is the default
  #endif
#endif

#define M_PI    3.14159265358979323846


//#define USE_ALL_OPTIONS

extern LcmsVals vals;
extern Linearisation linear;

int lcms_run = FALSE;

//char *profile_widget;
char profile_type[64];

GtkWidget *fileselection1;

//TODO Wie erreiche ich die Widgets global?
//typedef struct
//{
  GtkWidget *lcms_options;
  GtkWidget *lcms_vbox;
  GtkWidget *lcms_notebook;
  GtkWidget *print_options_vbox;
  GtkWidget *direct_print_hbox;
  GtkWidget *direct_checkbutton;
  GSList *direct_print_hbox_group = NULL;
  GtkWidget *color_mode_rgb_radiobutton;
  GtkWidget *color_mode_cmyk_radiobutton;
  GtkWidget *direct_print_hseparator;
  GtkWidget *frame_profile;
  GtkWidget *profiles_hbox;
  GtkWidget *profile_label_vbox;
  GtkWidget *wksp_profile_label;
  GtkWidget *sep_profile_label;
  GtkWidget *linear_profile_label;
  GtkWidget *profiles_vbox;
  GtkWidget *select_iprofile_entry;
  GtkWidget *select_oprofile_entry;
  GtkWidget *select_lprofile_entry;
  GtkWidget *select_profile_dir_vbox;
  GtkWidget *select_iprofile_button;
  GtkWidget *select_oprofile_button;
  GtkWidget *select_lprofile_button;
  GtkWidget *frame_intent;
  GtkWidget *intent_vbox;
  GSList *intent_vbox_group = NULL;
  GtkWidget *perc_radiobutton;
  GtkWidget *rel_color_radiobutton;
  GtkWidget *satur_radiobutton;
  GtkWidget *abs_color_radiobutton;
  GtkWidget *frame_options;
  GtkWidget *options_vbox;
  GtkWidget *matrix_vbox;
  GSList *matrix_vbox_group = NULL;
  GtkWidget *matrix_i_radiobutton;
  GtkWidget *matrix_o_radiobutton;
  GtkWidget *matrix_radiobutton;
  GtkWidget *null_transf_radiobutton;
  GtkWidget *matrix_hseparator;
  GtkWidget *precalc_vbox;
  GSList *precalc_vbox_group = NULL;
  GtkWidget *hires_prec_radiobutton;
  GtkWidget *midres_prec_radiobutton;
  GtkWidget *lowres_prec_radiobutton;
  GtkWidget *no_prec_radiobutton;
  GtkWidget *precalc_hseparator;
  GtkWidget *bpc_checkbutton;
  GtkWidget *preserveblack_checkbutton;
  GtkWidget *print_opts_label;
  GtkWidget *lin_opts_vbox;
  GtkWidget *use_linearisation_checkbutton;
  GtkWidget *frame_Min;
  GtkWidget *table_Min;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *C_min_hscale;
  GtkWidget *M_min_hscale;
  GtkWidget *Y_min_hscale;
  GtkWidget *K_min_hscale;
  GtkWidget *frame_Max;
  GtkWidget *table_Max;
  GtkWidget *label5;
  GtkWidget *label7;
  GtkWidget *label9;
  GtkWidget *label11;
  GtkWidget *C_max_hscale;
  GtkWidget *M_max_hscale;
  GtkWidget *Y_max_hscale;
  GtkWidget *K_max_hscale;
  GtkWidget *frame_gamma;
  GtkWidget *table_gamma;
  GtkWidget *label6;
  GtkWidget *label8;
  GtkWidget *label10;
  GtkWidget *label12;
  GtkWidget *C_g_hscale;
  GtkWidget *M_g_hscale;
  GtkWidget *Y_g_hscale;
  GtkWidget *K_g_hscale;
  GtkWidget *lin_opts_label;
  GtkWidget *action_hbox;
  GtkWidget *chancel_button;
  GtkWidget *ok_button;
  GtkWidget *reset_button;
  GtkWidget *save_tiff_button;
  GtkAccelGroup *accel_group;
  GtkTooltips *tooltips;

  GtkObject *adj;
//} LCMS_OPTIONS;


// function definitions

GtkWidget*
create_fileselection1 (GtkWidget *entry)
{
  GtkWidget *file_ok_button;
  GtkWidget *file_cancel_button;
  GtkWidget *file_help_button;

  if (strcmp (profile_type, _("Save as")) == 0)
    fileselection1 = gtk_file_selection_new (_("Select filename"));
  else
    fileselection1 = gtk_file_selection_new (_("Chose Profile"));
  
  gtk_object_set_data (GTK_OBJECT (fileselection1), "fileselection1", fileselection1);
  gtk_container_set_border_width (GTK_CONTAINER (fileselection1), 10);

  file_ok_button = GTK_FILE_SELECTION (fileselection1)->ok_button;
  gtk_object_set_data (GTK_OBJECT (fileselection1), "file_ok_button", file_ok_button);
  gtk_widget_show (file_ok_button);
  GTK_WIDGET_SET_FLAGS (file_ok_button, GTK_CAN_DEFAULT);

  file_cancel_button = GTK_FILE_SELECTION (fileselection1)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (fileselection1), "file_cancel_button", file_cancel_button);
  gtk_widget_show (file_cancel_button);
  GTK_WIDGET_SET_FLAGS (file_cancel_button, GTK_CAN_DEFAULT);

  file_help_button = gtk_button_new_with_label (_("Help"));
  gtk_widget_ref (file_help_button);
  gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION(fileselection1)->main_vbox),
			file_help_button, FALSE, FALSE, 5);
  gtk_object_set_data (GTK_OBJECT (fileselection1), "file_help_button",
                        file_help_button);
  gtk_container_set_border_width (GTK_CONTAINER (file_help_button), 2);
  //gtk_widget_show (file_help_button);
  gtk_tooltips_set_tip (tooltips, file_help_button, _("Shows some helpful information."), NULL);


  gtk_object_set_user_data (GTK_OBJECT (file_ok_button),entry);

  gtk_signal_connect (GTK_OBJECT (file_ok_button), "clicked",
                      GTK_SIGNAL_FUNC (on_file_ok_button_clicked),
                      entry);
  gtk_signal_connect (GTK_OBJECT (file_cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (on_file_cancel_button_clicked),
                      entry);
  gtk_signal_connect (GTK_OBJECT (file_help_button), "clicked",
                      GTK_SIGNAL_FUNC (on_file_help_button_clicked),
                      entry);

  return fileselection1;
}

GtkWidget*
create_lcms_options_window (void)
{

  gint   use_percept = (vals.intent == INTENT_PERCEPTUAL);
  gint   use_rel_color = (vals.intent == INTENT_RELATIVE_COLORIMETRIC);
  gint   use_satur = (vals.intent == INTENT_SATURATION);
  gint   use_abs_color = (vals.intent == INTENT_ABSOLUTE_COLORIMETRIC);
  gint   use_matrix_i = (vals.matrix == cmsFLAGS_MATRIXINPUT);
  gint   use_matrix_o = (vals.matrix == cmsFLAGS_MATRIXOUTPUT);
  gint   use_matrix = (vals.matrix == cmsFLAGS_MATRIXONLY);
  gint   use_null = (vals.matrix == cmsFLAGS_NULLTRANSFORM);
  gint   use_highres_prec = (vals.precalc == cmsFLAGS_HIGHRESPRECALC);
  gint   use_midres_prec = (vals.precalc == 0x00);
  gint   use_lowres_prec = (vals.precalc == cmsFLAGS_LOWRESPRECALC);
  gint   use_no_prec = (vals.precalc == cmsFLAGS_NOTPRECALC);
  gint   use_bpc = (vals.flags & cmsFLAGS_BLACKPOINTCOMPENSATION);
  gint   use_preserveblack = (vals.preserveblack == cmsFLAGS_PRESERVEBLACK);
  gchar     **argv;
  gint      argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("print");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  lcms_options = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (lcms_options), "lcms_options", lcms_options);
  gtk_container_set_border_width (GTK_CONTAINER (lcms_options), 4);
  gtk_window_set_title (GTK_WINDOW (lcms_options), version_);
  gtk_window_set_modal (GTK_WINDOW (lcms_options), TRUE);
  gtk_window_set_position (GTK_WINDOW (lcms_options), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (lcms_options), FALSE, FALSE, TRUE);

  tooltips = gtk_tooltips_new ();

  accel_group = gtk_accel_group_new ();

  lcms_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (lcms_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "lcms_vbox", lcms_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lcms_vbox);
  gtk_container_add (GTK_CONTAINER (lcms_options), lcms_vbox);

  lcms_notebook = gtk_notebook_new ();
  gtk_widget_ref (lcms_notebook);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "lcms_notebook", lcms_notebook,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lcms_notebook);
  gtk_box_pack_start (GTK_BOX (lcms_vbox), lcms_notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (lcms_notebook), 2);

  print_options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (print_options_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "print_options_vbox", print_options_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (print_options_vbox);
  gtk_container_add (GTK_CONTAINER (lcms_notebook), print_options_vbox);

  direct_print_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (direct_print_hbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "direct_print_hbox", direct_print_hbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (direct_print_hbox);
  gtk_box_pack_start (GTK_BOX (print_options_vbox), direct_print_hbox, TRUE, TRUE, 0);

  direct_checkbutton = gtk_check_button_new_with_label (_("Direct printing"));
  gtk_widget_ref (direct_checkbutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "direct_checkbutton", direct_checkbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (direct_checkbutton);
  gtk_box_pack_start (GTK_BOX (direct_print_hbox), direct_checkbutton, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (direct_checkbutton), 5);
  gtk_tooltips_set_tip (tooltips, direct_checkbutton, _("Do NOT separate this image by an ICC profile.\nThis button is useful in the following cases:\n- CMYK 8/16-bit - You have allready separated image data and dont need any separation\n- RGB 8/16-bit - to let gutenprint do the separation."), NULL);
  gtk_widget_add_accelerator (direct_checkbutton, "clicked", accel_group,
                              GDK_d, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  color_mode_rgb_radiobutton = gtk_radio_button_new_with_label (direct_print_hbox_group, _("RGB"));
  direct_print_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (color_mode_rgb_radiobutton));
  gtk_widget_ref (color_mode_rgb_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "color_mode_rgb_radiobutton", color_mode_rgb_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (color_mode_rgb_radiobutton);
  gtk_box_pack_start (GTK_BOX (direct_print_hbox), color_mode_rgb_radiobutton, FALSE, FALSE, 0);
  /*gtk_widget_add_accelerator (color_mode_rgb_radiobutton, "toggled", accel_group,
                              GDK_r, GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);*/

  color_mode_cmyk_radiobutton = gtk_radio_button_new_with_label (direct_print_hbox_group, _("CMYK"));
  direct_print_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (color_mode_cmyk_radiobutton));
  gtk_widget_ref (color_mode_cmyk_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "color_mode_cmyk_radiobutton", color_mode_cmyk_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (color_mode_cmyk_radiobutton);
  gtk_box_pack_start (GTK_BOX (direct_print_hbox), color_mode_cmyk_radiobutton, FALSE, FALSE, 0);
  /*gtk_widget_add_accelerator (color_mode_cmyk_radiobutton, "toggled", accel_group,
                              GDK_c, GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);*/

  direct_print_hseparator = gtk_hseparator_new ();
  gtk_widget_ref (direct_print_hseparator);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "direct_print_hseparator", direct_print_hseparator,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (direct_print_hseparator);
  gtk_box_pack_start (GTK_BOX (print_options_vbox), direct_print_hseparator, FALSE, TRUE, 0);

  frame_profile = gtk_frame_new (_("Profiles"));
  gtk_widget_ref (frame_profile);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_profile", frame_profile,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_profile);
  gtk_box_pack_start (GTK_BOX (print_options_vbox), frame_profile, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame_profile), 5);

  profiles_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (profiles_hbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "profiles_hbox", profiles_hbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (profiles_hbox);
  gtk_container_add (GTK_CONTAINER (frame_profile), profiles_hbox);

  profile_label_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (profile_label_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "profile_label_vbox", profile_label_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (profile_label_vbox);
  gtk_box_pack_start (GTK_BOX (profiles_hbox), profile_label_vbox, TRUE, TRUE, 0);

  wksp_profile_label = gtk_label_new (_("Editing Space Profile"));
  gtk_widget_ref (wksp_profile_label);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "wksp_profile_label", wksp_profile_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wksp_profile_label);
  gtk_box_pack_start (GTK_BOX (profile_label_vbox), wksp_profile_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (wksp_profile_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding (GTK_MISC (wksp_profile_label), 5, 5);

  sep_profile_label = gtk_label_new (_("Separation Profile"));
  gtk_widget_ref (sep_profile_label);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "sep_profile_label", sep_profile_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (sep_profile_label);
  gtk_box_pack_start (GTK_BOX (profile_label_vbox), sep_profile_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (sep_profile_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding (GTK_MISC (sep_profile_label), 5, 0);

  linear_profile_label = gtk_label_new (_("Linearisation Profile"));
  gtk_widget_ref (linear_profile_label);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "linear_profile_label", linear_profile_label,
                            (GtkDestroyNotify) gtk_widget_unref);
#ifdef USE_ALL_OPTIONS
  gtk_widget_show (linear_profile_label);
#endif
  gtk_box_pack_start (GTK_BOX (profile_label_vbox), linear_profile_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (linear_profile_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding (GTK_MISC (linear_profile_label), 5, 5);

  profiles_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (profiles_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "profiles_vbox", profiles_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (profiles_vbox);
  gtk_box_pack_start (GTK_BOX (profiles_hbox), profiles_vbox, TRUE, TRUE, 0);

  select_iprofile_entry = gtk_entry_new ();
  gtk_widget_ref (select_iprofile_entry);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_iprofile_entry", select_iprofile_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_iprofile_entry);
  gtk_box_pack_start (GTK_BOX (profiles_vbox), select_iprofile_entry, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_iprofile_entry, 135, -2);
  gtk_tooltips_set_tip (tooltips, select_iprofile_entry, _("This is the workspace profile.--- You dont need to change if Your image is in RGB mode. ---\nIf needed, select here an profile which matches to the current images workspace."), NULL);
  //gtk_entry_set_text (GTK_ENTRY (select_iprofile_entry), _("sRGB.icc"));

  select_oprofile_entry = gtk_entry_new ();
  gtk_widget_ref (select_oprofile_entry);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_oprofile_entry", select_oprofile_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_oprofile_entry);
  gtk_box_pack_start (GTK_BOX (profiles_vbox), select_oprofile_entry, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_oprofile_entry, 135, -2);
  gtk_tooltips_set_tip (tooltips, select_oprofile_entry, _("This is the separation profile. You need to select an Printer / CMYK profile to be able to do high quality separation of Your image data.\n--- Currently You need an profile here to be able to print at all. ---\nProfiles have the icc or icm extension. On mac/windows You can search for profiles in Your system folders. On any system You can download profiles from the Adobe website <http://www.adobe.com/support/downloads/main.html>.\nAs the quality of the data output depends highly on Your profile chosen, the best is to do an third party calibration of Your special printer. Otherwise You can use any generic output profile and may or may not have to tweak the settings in the second dialog."), NULL);
  //gtk_entry_set_text (GTK_ENTRY (select_oprofile_entry), _("cmyk.icc"));

  select_lprofile_entry = gtk_entry_new ();
  gtk_widget_ref (select_lprofile_entry);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_lprofile_entry", select_lprofile_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_lprofile_entry);
#ifndef USE_ALL_OPTIONS
  gtk_widget_hide (select_lprofile_entry);
#endif
  gtk_box_pack_start (GTK_BOX (profiles_vbox), select_lprofile_entry, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_lprofile_entry, 135, -2);
  gtk_tooltips_set_tip (tooltips, select_lprofile_entry, _("This is the linearisation profile.\n--- You need it only if You used CinePaints linearisation option while calibrating Your printer. ---\nIf needed, select here an profile which belongs to the current printer profile."), NULL);
  //gtk_entry_set_text (GTK_ENTRY (select_lprofile_entry), _("linearisation.icc"));

  select_profile_dir_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (select_profile_dir_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_profile_dir_vbox", select_profile_dir_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_profile_dir_vbox);
  gtk_box_pack_start (GTK_BOX (profiles_hbox), select_profile_dir_vbox, TRUE, TRUE, 0);

  select_iprofile_button = gtk_button_new_with_label (_("...\n"));
  gtk_widget_ref (select_iprofile_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_iprofile_button", select_iprofile_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_iprofile_button);
  gtk_box_pack_start (GTK_BOX (select_profile_dir_vbox), select_iprofile_button, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_iprofile_button, 45, 22);
  gtk_container_set_border_width (GTK_CONTAINER (select_iprofile_button), 2);
  gtk_tooltips_set_tip (tooltips, select_iprofile_button, _("Select a new profile for Your workspace."), NULL);
  gtk_widget_add_accelerator (select_iprofile_button, "clicked", accel_group,
                              GDK_o, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                              GTK_ACCEL_VISIBLE);

  select_oprofile_button = gtk_button_new_with_label (_("...\n"));
  gtk_widget_ref (select_oprofile_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_oprofile_button", select_oprofile_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_oprofile_button);
  gtk_box_pack_start (GTK_BOX (select_profile_dir_vbox), select_oprofile_button, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_oprofile_button, 45, 22);
  gtk_container_set_border_width (GTK_CONTAINER (select_oprofile_button), 2);
  gtk_tooltips_set_tip (tooltips, select_oprofile_button, _("Select a new profile for separation."), NULL);
  gtk_widget_add_accelerator (select_oprofile_button, "clicked", accel_group,
                              GDK_o, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  select_lprofile_button = gtk_button_new_with_label (_("...\n"));
  gtk_widget_ref (select_lprofile_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "select_lprofile_button", select_lprofile_button,
                            (GtkDestroyNotify) gtk_widget_unref);
#ifdef USE_ALL_OPTIONS
  gtk_widget_show (select_lprofile_button);
#endif
  gtk_box_pack_start (GTK_BOX (select_profile_dir_vbox), select_lprofile_button, FALSE, FALSE, 0);
  gtk_widget_set_usize (select_lprofile_button, 45, 22);
  gtk_container_set_border_width (GTK_CONTAINER (select_lprofile_button), 2);
  gtk_tooltips_set_tip (tooltips, select_lprofile_button, _("Select a new profile for linearisation."), NULL);
  gtk_widget_add_accelerator (select_lprofile_button, "clicked", accel_group,
                              GDK_l, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  frame_intent = gtk_frame_new (_("Select rendering intent:"));
  gtk_widget_ref (frame_intent);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_intent", frame_intent,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_intent);
  gtk_box_pack_start (GTK_BOX (print_options_vbox), frame_intent, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame_intent), 5);

  intent_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (intent_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "intent_vbox", intent_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (intent_vbox);
  gtk_container_add (GTK_CONTAINER (frame_intent), intent_vbox);

  perc_radiobutton = gtk_radio_button_new_with_label (intent_vbox_group, _("Perceptual"));
  intent_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (perc_radiobutton));
  gtk_widget_ref (perc_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "perc_radiobutton", perc_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (perc_radiobutton);
  gtk_box_pack_start (GTK_BOX (intent_vbox), perc_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, perc_radiobutton, _("Hue hopefully maintained (but not required), lightness and saturation sacrificed to maintain the perceived color. White point changed to result in neutral grays. Intended for images.\nIn lcms: Default intent of profiles is used"), NULL);

  rel_color_radiobutton = gtk_radio_button_new_with_label (intent_vbox_group, _("Relative colorimetric"));
  intent_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (rel_color_radiobutton));
  gtk_widget_ref (rel_color_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "rel_color_radiobutton", rel_color_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (rel_color_radiobutton);
  gtk_box_pack_start (GTK_BOX (intent_vbox), rel_color_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, rel_color_radiobutton, _("Within and outside gamut; same as Absolute Colorimetric. White point changed to result in neutral grays.\nIn lcms: If adequate table is present in profile, then, it is used. Else reverts to perceptual intent."), NULL);

  satur_radiobutton = gtk_radio_button_new_with_label (intent_vbox_group, _("Saturation"));
  intent_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (satur_radiobutton));
  gtk_widget_ref (satur_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "satur_radiobutton", satur_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (satur_radiobutton);
  gtk_box_pack_start (GTK_BOX (intent_vbox), satur_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, satur_radiobutton, _("Hue and saturation maintained with lightness sacrificed to maintain saturation. White point changed to result in neutral grays. Intended for business graphics (make it colorful charts, graphs, overheads, ...)\nIn lcms: If adequate table is present in profile, then, it is used. Else reverts to perceptual intent."), NULL);

  abs_color_radiobutton = gtk_radio_button_new_with_label (intent_vbox_group, _("Absolute colorimetric"));
  intent_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (abs_color_radiobutton));
  gtk_widget_ref (abs_color_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "abs_color_radiobutton", abs_color_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (abs_color_radiobutton);
  gtk_box_pack_start (GTK_BOX (intent_vbox), abs_color_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, abs_color_radiobutton, _("Within the destination device gamut; hue, lightness and saturation are maintained. Outside the gamut; hue and lightness are maintained, saturation is sacrificed. White point for source and destination; unchanged. Intended for spot colors (Pantone, TruMatch, logo colors, ...)\nIn lcms: relative colorimetric intent is used with undoing of chromatic adaptation."), NULL);

  frame_options = gtk_frame_new (_("Options:"));
  gtk_widget_ref (frame_options);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_options", frame_options,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_options);
  gtk_box_pack_start (GTK_BOX (print_options_vbox), frame_options, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame_options), 5);

  options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (options_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "options_vbox", options_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (options_vbox);
  gtk_container_add (GTK_CONTAINER (frame_options), options_vbox);

  matrix_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (matrix_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "matrix_vbox", matrix_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (matrix_vbox);
  gtk_box_pack_start (GTK_BOX (options_vbox), matrix_vbox, FALSE, FALSE, 0);

  matrix_i_radiobutton = gtk_radio_button_new_with_label (matrix_vbox_group, _("Matrix Input"));
  matrix_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (matrix_i_radiobutton));
  gtk_widget_ref (matrix_i_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "matrix_i_radiobutton", matrix_i_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (matrix_i_radiobutton);
  gtk_box_pack_start (GTK_BOX (matrix_vbox), matrix_i_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, matrix_i_radiobutton, _("CLUT ignored on input profile, matrix-shaper used instead (for speed, and debugging purposes)"), NULL);

  matrix_o_radiobutton = gtk_radio_button_new_with_label (matrix_vbox_group, _("Matrix Output"));
  matrix_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (matrix_o_radiobutton));
  gtk_widget_ref (matrix_o_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "matrix_o_radiobutton", matrix_o_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (matrix_o_radiobutton);
  gtk_box_pack_start (GTK_BOX (matrix_vbox), matrix_o_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, matrix_o_radiobutton, _("CLUT ignored on output profile, matrix-shaper used instead (for speed, and debugging purposes)"), NULL);

  matrix_radiobutton = gtk_radio_button_new_with_label (matrix_vbox_group, _("Matrix only"));
  matrix_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (matrix_radiobutton));
  gtk_widget_ref (matrix_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "matrix_radiobutton", matrix_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (matrix_radiobutton);
  gtk_box_pack_start (GTK_BOX (matrix_vbox), matrix_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, matrix_radiobutton, _("Both input and output are forced to matrix-shaper."), NULL);

  null_transf_radiobutton = gtk_radio_button_new_with_label (matrix_vbox_group, _("Null transform"));
  matrix_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (null_transf_radiobutton));
  gtk_widget_ref (null_transf_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "null_transf_radiobutton", null_transf_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (null_transf_radiobutton);
  gtk_box_pack_start (GTK_BOX (matrix_vbox), null_transf_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, null_transf_radiobutton, _("Don't transform anyway, only apply pack/unpack routines (usefull to deactivate color correction but keep formatting capabilities)"), NULL);

  matrix_hseparator = gtk_hseparator_new ();
  gtk_widget_ref (matrix_hseparator);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "matrix_hseparator", matrix_hseparator,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (matrix_hseparator);
  gtk_box_pack_start (GTK_BOX (options_vbox), matrix_hseparator, FALSE, TRUE, 0);

  precalc_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (precalc_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "precalc_vbox", precalc_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (precalc_vbox);
  gtk_box_pack_start (GTK_BOX (options_vbox), precalc_vbox, FALSE, FALSE, 0);

  hires_prec_radiobutton = gtk_radio_button_new_with_label (precalc_vbox_group, _("High resolution precalculation"));
  precalc_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (hires_prec_radiobutton));
  gtk_widget_ref (hires_prec_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "hires_prec_radiobutton", hires_prec_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hires_prec_radiobutton);
  gtk_box_pack_start (GTK_BOX (precalc_vbox), hires_prec_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, hires_prec_radiobutton, _("Use 48 points instead of 33 for device-link CLUT precalculation. Not needed but for the most extreme cases of mismatch of \"impedance\" between profiles."), NULL);

  midres_prec_radiobutton = gtk_radio_button_new_with_label (precalc_vbox_group, _("Middle resolution precalculation"));
  precalc_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (midres_prec_radiobutton));
  gtk_widget_ref (midres_prec_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "midres_prec_radiobutton", midres_prec_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (midres_prec_radiobutton);
  gtk_box_pack_start (GTK_BOX (precalc_vbox), midres_prec_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, midres_prec_radiobutton, _("Use 33 points for device-link CLUT precalculation. Standard."), NULL);

  lowres_prec_radiobutton = gtk_radio_button_new_with_label (precalc_vbox_group, _("Low resolution precalculation"));
  precalc_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (lowres_prec_radiobutton));
  gtk_widget_ref (lowres_prec_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "lowres_prec_radiobutton", lowres_prec_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lowres_prec_radiobutton);
  gtk_box_pack_start (GTK_BOX (precalc_vbox), lowres_prec_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, lowres_prec_radiobutton, _("Use lower resolution table. Usefull when memory is a preciated resource."), NULL);

  no_prec_radiobutton = gtk_radio_button_new_with_label (precalc_vbox_group, _("No precalculation"));
  precalc_vbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (no_prec_radiobutton));
  gtk_widget_ref (no_prec_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "no_prec_radiobutton", no_prec_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (no_prec_radiobutton);
  gtk_box_pack_start (GTK_BOX (precalc_vbox), no_prec_radiobutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, no_prec_radiobutton, _("By default, lcms smelt luts into a device-link CLUT. This speedup whole transform greatly. If you don't wanna this, and wish every value to be translated to PCS and back to output space, include this flag."), NULL);

  precalc_hseparator = gtk_hseparator_new ();
  gtk_widget_ref (precalc_hseparator);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "precalc_hseparator", precalc_hseparator,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (precalc_hseparator);
  gtk_box_pack_start (GTK_BOX (options_vbox), precalc_hseparator, FALSE, TRUE, 0);

  bpc_checkbutton = gtk_check_button_new_with_label (_("Black point compensation"));
  gtk_widget_ref (bpc_checkbutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "bpc_checkbutton", bpc_checkbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bpc_checkbutton);
  gtk_box_pack_start (GTK_BOX (options_vbox), bpc_checkbutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, bpc_checkbutton, _("BPC does scale full image across gray axis in order to accommodate the darkest tone origin media can render to darkest tone destination media can render."), NULL);

  preserveblack_checkbutton = gtk_check_button_new_with_label (_("Preserve Black"));
  gtk_widget_ref (preserveblack_checkbutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "preserveblack_checkbutton", preserveblack_checkbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  if(LCMS_VERSION >= 115)
    gtk_widget_show (preserveblack_checkbutton);
  gtk_box_pack_start (GTK_BOX (options_vbox), preserveblack_checkbutton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, preserveblack_checkbutton, _("Preserve Black channel of a Cmyk image to maintain K information. Useful only for Cmyk->Cmyk conversion."), NULL);

  print_opts_label = gtk_label_new (_("Print Options"));
  gtk_widget_ref (print_opts_label);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "print_opts_label", print_opts_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (print_opts_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (lcms_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (lcms_notebook), 0), print_opts_label);

  lin_opts_vbox = gtk_vbox_new (FALSE, 10);
  gtk_widget_ref (lin_opts_vbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "lin_opts_vbox", lin_opts_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  // TODO As long as we can do no linearisation here
  //gtk_widget_show (lin_opts_vbox);
#ifndef USE_ALL_OPTIONS
  gtk_widget_hide (lin_opts_vbox);
#endif
  gtk_container_add (GTK_CONTAINER (lcms_notebook), lin_opts_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (lin_opts_vbox), 5);

  use_linearisation_checkbutton = gtk_check_button_new_with_label (_("use printer linearisation"));
  gtk_widget_ref (use_linearisation_checkbutton);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "use_linearisation_checkbutton", use_linearisation_checkbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (use_linearisation_checkbutton);
  gtk_box_pack_start (GTK_BOX (lin_opts_vbox), use_linearisation_checkbutton, FALSE, TRUE, 0);
  gtk_widget_set_usize (use_linearisation_checkbutton, -2, 45);
  GTK_WIDGET_SET_FLAGS (use_linearisation_checkbutton, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, use_linearisation_checkbutton, _("This option is only usefull to linearise an printer before an calibration.  Print out an test chart and look for sufficiant coverage at bright and light patches. After this is good for all printing colors, set the gamma values by repeatedly printing out an usefull chart. You can use this sliders to give bright patches recognisable difference. The same should appear to the dark patches."), NULL);
  gtk_widget_add_accelerator (use_linearisation_checkbutton, "clicked", accel_group,
                              GDK_l, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  frame_Min = gtk_frame_new (_("MinBrightness settings"));
  gtk_widget_ref (frame_Min);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_Min", frame_Min,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_Min);
  gtk_box_pack_start (GTK_BOX (lin_opts_vbox), frame_Min, FALSE, TRUE, 0);

  table_Min = gtk_table_new (4, 2, FALSE);
  gtk_widget_ref (table_Min);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "table_Min", table_Min,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table_Min);
  gtk_container_add (GTK_CONTAINER (frame_Min), table_Min);

  // look in lib/widgets.h
  adj = gimp_scale_entry_new (GTK_TABLE (table_Min), 0, 0,
                              _("Cyan:"), 100, 0,
                              linear.C_Min, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.C_Min);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Min), 0, 1,
                              _("Magenta:"), 100, 0,
                              linear.M_Min, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.M_Min);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Min), 0, 2,
                              _("Yellow:"), 100, 0,
                              linear.Y_Min, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.Y_Min);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Min), 0, 3,
                              _("Black:"), 100, 0,
                              linear.K_Min, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.K_Min);


  frame_Max = gtk_frame_new (_("MaxBrightness settings"));
  gtk_widget_ref (frame_Max);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_Max", frame_Max,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_Max);
  gtk_box_pack_start (GTK_BOX (lin_opts_vbox), frame_Max, FALSE, TRUE, 0);

  table_Max = gtk_table_new (4, 2, FALSE);
  gtk_widget_ref (table_Max);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "table_Max", table_Max,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table_Max);
  gtk_container_add (GTK_CONTAINER (frame_Max), table_Max);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Max), 0, 0,
                              _("Cyan:"), 100, 0,
                              linear.C_Max, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.C_Max);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Max), 0, 1,
                              _("Magenta:"), 100, 0,
                              linear.M_Max, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.M_Max);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Max), 0, 2,
                              _("Yellow:"), 100, 0,
                              linear.Y_Max, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.Y_Max);

  adj = gimp_scale_entry_new (GTK_TABLE (table_Max), 0, 3,
                              _("Black:"), 100, 0,
                              linear.K_Max, 0.0, 1.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.K_Max);



  frame_gamma = gtk_frame_new (_("Gamma settings"));
  gtk_widget_ref (frame_gamma);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "frame_gamma", frame_gamma,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_gamma);
  gtk_box_pack_start (GTK_BOX (lin_opts_vbox), frame_gamma, FALSE, TRUE, 0);

  table_gamma = gtk_table_new (4, 2, FALSE);
  gtk_widget_ref (table_gamma);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "table_gamma", table_gamma,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table_gamma);
  gtk_container_add (GTK_CONTAINER (frame_gamma), table_gamma);

  adj = gimp_scale_entry_new (GTK_TABLE (table_gamma), 0, 0,
                              _("Cyan:"), 100, 0,
                              linear.C_Gamma, 0.0, 8.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.C_Gamma);

  adj = gimp_scale_entry_new (GTK_TABLE (table_gamma), 0, 1,
                              _("Magenta:"), 100, 0,
                              linear.M_Gamma, 0.0, 8.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.M_Gamma);

  adj = gimp_scale_entry_new (GTK_TABLE (table_gamma), 0, 2,
                              _("Yellow:"), 100, 0,
                              linear.Y_Gamma, 0.0, 8.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.Y_Gamma);

  adj = gimp_scale_entry_new (GTK_TABLE (table_gamma), 0, 3,
                              _("Black:"), 100, 0,
                              linear.K_Gamma, 0.0, 8.0, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                      &linear.K_Gamma);



  lin_opts_label = gtk_label_new (_("Linearisation Options"));
  gtk_widget_ref (lin_opts_label);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "lin_opts_label", lin_opts_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (lin_opts_label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (lcms_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (lcms_notebook), 1), lin_opts_label);

  action_hbox = gtk_hbox_new (TRUE, 10);
  gtk_widget_ref (action_hbox);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "action_hbox", action_hbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (action_hbox);
  gtk_box_pack_start (GTK_BOX (lcms_vbox), action_hbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (action_hbox), 10);

  chancel_button = gtk_button_new_with_label (_("Chancel"));
  gtk_widget_ref (chancel_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "chancel_button", chancel_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (chancel_button);
  gtk_box_pack_end (GTK_BOX (action_hbox), chancel_button, TRUE, TRUE, 0);
  gtk_widget_add_accelerator (chancel_button, "clicked", accel_group,
                              GDK_c, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_ref (ok_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "ok_button", ok_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ok_button);
  gtk_box_pack_end (GTK_BOX (action_hbox), ok_button, TRUE, TRUE, 0);
  gtk_widget_set_usize (ok_button, -2, 40);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

  reset_button = gtk_button_new_with_label (_("Reset"));
  gtk_widget_ref (reset_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "reset_button", reset_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (reset_button);
  gtk_box_pack_start (GTK_BOX (action_hbox), reset_button, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, reset_button, _("reset rendering intend and options to defaults"), NULL);

  save_tiff_button = gtk_button_new_with_label (_("Save Tiff"));
  gtk_widget_ref (save_tiff_button);
  gtk_object_set_data_full (GTK_OBJECT (lcms_options), "save_tiff_button", save_tiff_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save_tiff_button);
  gtk_box_pack_start (GTK_BOX (action_hbox), save_tiff_button, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, save_tiff_button, _("Save as separated tiff including the separation profile"), NULL);
  gtk_widget_add_accelerator (save_tiff_button, "clicked", accel_group,
                              GDK_t, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);



  gtk_signal_connect (GTK_OBJECT (direct_checkbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_direct_checkbutton_clicked),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (color_mode_rgb_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_color_mode_rgb_radiobutton_toggled),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (color_mode_cmyk_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_color_mode_cmyk_radiobutton_toggled),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (select_iprofile_entry), "changed",
                      GTK_SIGNAL_FUNC (on_select_iprofile_entry_insert_text),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (select_oprofile_entry), "changed",
                      GTK_SIGNAL_FUNC (on_select_oprofile_entry_insert_text),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (select_lprofile_entry), "changed",
                      GTK_SIGNAL_FUNC (on_select_lprofile_entry_changed),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (select_iprofile_button), "clicked",
                      GTK_SIGNAL_FUNC (on_select_iprofile_clicked),
                      select_iprofile_entry );
  gtk_signal_connect (GTK_OBJECT (select_oprofile_button), "clicked",
                      GTK_SIGNAL_FUNC (on_select_oprofile_button_clicked),
                      select_oprofile_entry );
  gtk_signal_connect (GTK_OBJECT (select_lprofile_button), "clicked",
                      GTK_SIGNAL_FUNC (on_select_lprofile_button_clicked),
                      select_lprofile_entry);
  gtk_signal_connect (GTK_OBJECT (chancel_button), "clicked",
                      GTK_SIGNAL_FUNC (on_chancel_button_clicked),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_button_clicked),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (reset_button), "clicked",
                      GTK_SIGNAL_FUNC (on_reset_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (save_tiff_button), "clicked",
                      GTK_SIGNAL_FUNC (on_save_tiff_button_clicked),
                      lcms_options);
  gtk_signal_connect (GTK_OBJECT (perc_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_perc_radiobutton_toggled),
                      &use_percept);
  gtk_signal_connect (GTK_OBJECT (rel_color_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_rel_color_radiobutton_toggled),
                      &use_rel_color);
  gtk_signal_connect (GTK_OBJECT (satur_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_satur_radiobutton_toggled),
                      &use_satur);
  gtk_signal_connect (GTK_OBJECT (abs_color_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_abs_color_radiobutton_toggled),
                      &use_abs_color);
  gtk_signal_connect (GTK_OBJECT (matrix_i_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_matrix_i_radiobutton_toggled),
                      &use_matrix_i);
  gtk_signal_connect (GTK_OBJECT (matrix_o_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_matrix_o_radiobutton_toggled),
                      &use_matrix_o);
  gtk_signal_connect (GTK_OBJECT (matrix_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_matrix_radiobutton_toggled),
                      &use_matrix);
  gtk_signal_connect (GTK_OBJECT (null_transf_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_null_transf_radiobutton_toggled),
                      &use_null);
  gtk_signal_connect (GTK_OBJECT (hires_prec_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_hires_prec_radiobutton_toggled),
                      &use_highres_prec);
  gtk_signal_connect (GTK_OBJECT (midres_prec_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_midres_prec_radiobutton_toggled),
                      &use_midres_prec);
  gtk_signal_connect (GTK_OBJECT (lowres_prec_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_lowres_prec_radiobutton_toggled),
                      &use_lowres_prec);
  gtk_signal_connect (GTK_OBJECT (no_prec_radiobutton), "toggled",
                      GTK_SIGNAL_FUNC (on_no_prec_radiobutton_toggled),
                      &use_no_prec);
  gtk_signal_connect (GTK_OBJECT (bpc_checkbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_bpc_checkbutton_clicked),
                      &use_bpc);
  gtk_signal_connect (GTK_OBJECT (preserveblack_checkbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_preserveblack_checkbutton_clicked),
                      &use_preserveblack);
  gtk_signal_connect (GTK_OBJECT (use_linearisation_checkbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_use_linearisation_checkbutton_clicked),
                      NULL);


  gtk_widget_grab_focus (ok_button);
  gtk_widget_grab_default (ok_button);
  gtk_object_set_data (GTK_OBJECT (lcms_options), "tooltips", tooltips);

  gtk_window_add_accel_group (GTK_WINDOW (lcms_options), accel_group);

  return lcms_options;
}

void init_lcms_options (void)
{
  char* ptr = get_proof_name();

  if(ptr == NULL)
  {
    sprintf(vals.o_profile,"");
  }
  else
    if ( (strcmp(vals.o_profile, "")) == 0)
      snprintf (vals.o_profile, MAX_PATH, "%s", ptr);

#ifndef USE_ALL_OPTIONS
  gtk_widget_hide (matrix_vbox);
  gtk_widget_hide (matrix_hseparator);
#endif

  // set all widgets
  if ((strcmp(vals.i_profile, "")) == 0)
    gtk_entry_set_text (GTK_ENTRY (select_iprofile_entry), "sRGB.icc");
  else
    gtk_entry_set_text (GTK_ENTRY (select_iprofile_entry), vals.i_profile);
  gtk_editable_set_position (GTK_EDITABLE(select_iprofile_entry),
                            GTK_ENTRY(select_iprofile_entry)->text_length);

  if ((strcmp(vals.o_profile, "")) == 0)
    gtk_entry_set_text (GTK_ENTRY (select_oprofile_entry), "");
  else
    gtk_entry_set_text (GTK_ENTRY (select_oprofile_entry), vals.o_profile);
  gtk_editable_set_position (GTK_EDITABLE(select_oprofile_entry),
                            GTK_ENTRY(select_oprofile_entry)->text_length);
  if ((strcmp(vals.l_profile, "")) != 0) {
    gtk_entry_set_text (GTK_ENTRY (select_lprofile_entry), vals.l_profile);
    gtk_editable_set_position (GTK_EDITABLE(select_lprofile_entry),
                              GTK_ENTRY(select_lprofile_entry)->text_length);
  }

  #define set_toogle_active(causa,widget) \
    case causa: \
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE); \
      break;
 
  switch (vals.color_space) {
    set_toogle_active (PT_RGB,  color_mode_rgb_radiobutton)
    set_toogle_active (PT_CMYK, color_mode_cmyk_radiobutton)
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (direct_checkbutton), !vals.icc);
  gtk_widget_set_sensitive (color_mode_rgb_radiobutton, !vals.icc);
  gtk_widget_set_sensitive (color_mode_cmyk_radiobutton, !vals.icc);
  if (vals.icc) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_linearisation_checkbutton), FALSE);
  }
  if ((vals.icc
    || vals.color_space != PT_CMYK)
   && linear.use_lin == FALSE) {
       gtk_widget_set_sensitive (frame_gamma, !vals.icc);
       gtk_widget_set_sensitive (frame_Max, !vals.icc);
       gtk_widget_set_sensitive (frame_Min, !vals.icc);
  }

  switch (vals.intent) {
    set_toogle_active (INTENT_PERCEPTUAL,           perc_radiobutton)
    set_toogle_active (INTENT_RELATIVE_COLORIMETRIC,rel_color_radiobutton)
    set_toogle_active (INTENT_SATURATION,           satur_radiobutton)
    set_toogle_active (INTENT_ABSOLUTE_COLORIMETRIC,abs_color_radiobutton)
  }
  switch (vals.matrix) {
    set_toogle_active (cmsFLAGS_MATRIXINPUT,  matrix_i_radiobutton)
    set_toogle_active (cmsFLAGS_MATRIXOUTPUT, matrix_o_radiobutton)
    set_toogle_active (cmsFLAGS_MATRIXONLY,   matrix_radiobutton)
    set_toogle_active (cmsFLAGS_NULLTRANSFORM,null_transf_radiobutton)
  }
  switch (vals.precalc) {
    set_toogle_active (cmsFLAGS_HIGHRESPRECALC, hires_prec_radiobutton)
    set_toogle_active (0x00,                    midres_prec_radiobutton)
    set_toogle_active (cmsFLAGS_LOWRESPRECALC,  lowres_prec_radiobutton)
    set_toogle_active (cmsFLAGS_NOTPRECALC,     no_prec_radiobutton)
  }

  if (vals.flags |= cmsFLAGS_BLACKPOINTCOMPENSATION)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bpc_checkbutton), TRUE);
  if (vals.preserveblack == cmsFLAGS_PRESERVEBLACK)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preserveblack_checkbutton), TRUE);

  if (gimp_image_has_icc_profile (vals.o_image_ID, ICC_IMAGE_PROFILE)) {
    char *color_name =
                   gimp_image_get_icc_profile_color_space_name (vals.o_image_ID,
                                                             ICC_IMAGE_PROFILE);
    gtk_entry_set_text (GTK_ENTRY (select_iprofile_entry), "intern");
    gtk_widget_hide (wksp_profile_label);
    gtk_widget_hide (select_iprofile_entry);
    gtk_widget_hide (select_iprofile_button);
    
    if (strstr (color_name, "Rgb") != 0)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (color_mode_rgb_radiobutton), TRUE);
    else if (strstr (color_name, "Cmyk") != 0)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (color_mode_cmyk_radiobutton), TRUE);
    else if (strstr (color_name, "   ") == 0) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (direct_checkbutton), FALSE);
      gtk_widget_set_sensitive (direct_checkbutton, FALSE);
      gtk_widget_set_sensitive (color_mode_rgb_radiobutton, FALSE);
      gtk_widget_set_sensitive (color_mode_cmyk_radiobutton, FALSE);
;
    }
    if(color_name) free(color_name);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (direct_checkbutton), TRUE);
  }
}


/*
 * Callbacks
 */


void
on_select_iprofile_entry_insert_text   (GtkEditable     *editable,
                                        gpointer         user_data)
{

  strcpy (vals.i_profile, gtk_editable_get_chars(editable,0,
                                      GTK_ENTRY(editable)->text_length));
  g_print ("%s %s:%d  %s\n",__func__,__FILE__,__LINE__,vals.i_profile);
}


void
on_select_oprofile_entry_insert_text   (GtkEditable     *editable,
                                        gpointer         user_data)
{

  sprintf (vals.o_profile, "%s", gtk_entry_get_text (GTK_ENTRY (editable)));
  g_print ("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,vals.o_profile);

      if (strlen (vals.l_profile) > 0) {
        char *point, *text;

        point = text = g_new (char,MAX_PATH);

        strcpy (text, vals.o_profile);
        g_print ("%s:%d %s\n",__FILE__,__LINE__,text);

        if (strchr(text, '.') && strlen (text) < MAX_PATH - 4) {
          point = strrchr(text, '.');
          sprintf (point,".icm");
          sprintf (vals.l_profile,"%s",text);
          g_print ("%s:%d vals.l_profile %s\n",__FILE__,__LINE__,vals.l_profile);
        }
        g_print ("%s:%d vals.l_profile %s\n",__FILE__,__LINE__,vals.l_profile);
        g_free (text);
      }

  gtk_entry_set_text (GTK_ENTRY (select_lprofile_entry), vals.l_profile);
  gtk_editable_set_position (GTK_EDITABLE(select_lprofile_entry),
                             GTK_ENTRY(select_lprofile_entry)->text_length);
}

void
on_select_lprofile_entry_changed       (GtkEditable     *editable,
                                        gpointer         user_data)
{

  sprintf (vals.l_profile, "%s", gtk_entry_get_text (GTK_ENTRY (editable)));
  g_print ("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,vals.l_profile);
}

static void
set_profile_selection (gpointer user_data, char *profile)
{
  fileselection1 = create_fileselection1 ( user_data );
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fileselection1));


  if ((strcmp (profile,"")) == 0 ||
      (strcmp (profile,"s")) == 0)
    sprintf (profile, "/usr/share/color/icc/");

  if ((strcmp (profile,"h")) == 0 ||
      (strcmp (profile,"u")) == 0)
    sprintf (profile, "~/.color/icc/");

  gtk_file_selection_set_filename (GTK_FILE_SELECTION(fileselection1),
                                     profile);

  gtk_file_selection_complete (GTK_FILE_SELECTION(fileselection1), "*.[icm,icc,ICC,ICM,Icc,Icm]");
  gtk_widget_show (fileselection1);
  gtk_widget_set_sensitive (gtk_widget_get_toplevel (GTK_WIDGET (user_data)), FALSE);
}

void
on_select_iprofile_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  sprintf (profile_type,_("Work Space"));
  set_profile_selection (user_data, vals.i_profile);
}

void
on_select_oprofile_button_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  sprintf (profile_type,_("Separation"));
  set_profile_selection (user_data, vals.o_profile);
}

void
on_select_lprofile_button_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  sprintf (profile_type,_("Linearisation"));
  set_profile_selection (user_data, vals.l_profile);
}


void
on_direct_checkbutton_clicked             (GtkButton       *button,
                                           gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (button)->active) {
    vals.icc = FALSE;
    if (vals.color_space == PT_CMYK
     && linear.use_lin) {
      gtk_widget_set_sensitive (frame_gamma, !vals.icc);
      gtk_widget_set_sensitive (frame_Max, !vals.icc);
      gtk_widget_set_sensitive (frame_Min, !vals.icc);
    }
  } else {
    vals.icc = TRUE;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                   (use_linearisation_checkbutton), FALSE);
    gtk_widget_set_sensitive (frame_gamma, !vals.icc);
    gtk_widget_set_sensitive (frame_Max, !vals.icc);
    gtk_widget_set_sensitive (frame_Min, !vals.icc);
  }

  gtk_widget_set_sensitive (frame_intent, vals.icc);
  gtk_widget_set_sensitive (frame_options, vals.icc);
  gtk_widget_set_sensitive (frame_profile, vals.icc);

  gtk_widget_set_sensitive (color_mode_rgb_radiobutton, !vals.icc);
  gtk_widget_set_sensitive (color_mode_cmyk_radiobutton, !vals.icc);
}

void
on_color_mode_cmyk_radiobutton_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active) {
    vals.color_space = PT_CMYK;
    if (linear.use_lin == TRUE) {
      gtk_widget_set_sensitive (frame_gamma, TRUE);
      gtk_widget_set_sensitive (frame_Max, TRUE);
      gtk_widget_set_sensitive (frame_Min, TRUE);
    }
  }
}

void
on_color_mode_rgb_radiobutton_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active) {
    vals.color_space = PT_RGB;
    gtk_widget_set_sensitive (frame_gamma, FALSE);
    gtk_widget_set_sensitive (frame_Max, FALSE);
    gtk_widget_set_sensitive (frame_Min, FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (
                                        use_linearisation_checkbutton), FALSE);
  }
}


void
on_chancel_button_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  vals.quit_app = TRUE;
  gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)) );
  gtk_main_quit ();
}


void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  int profiles_ok;

  profiles_ok = FALSE;
  lcms_run = TRUE;
  profiles_ok = test_print_settings ();

  g_print ("%s:%d %s() vals.i_profile %s vals.o_profile %s icc:%d color_space:%d\n",__FILE__,__LINE__,
                  __func__,vals.i_profile, vals.o_profile,
                  vals.icc, vals.color_space);

  if (profiles_ok) {   // Checking done; We can leave this dialog.
    gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)) );
    gtk_main_quit ();
  }
}

void
on_save_tiff_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  lcms_run = TRUE;
  g_print ("%s:%d vals.i_profile %s vals.o_profile %s icc:%d\n",__FILE__,__LINE__,vals.i_profile, vals.o_profile, vals.icc);


  if (test_tiff_settings() ) {

    vals.save_tiff = TRUE;

    // ... and ask for an filename
    sprintf (profile_type,_("Save as"));
    fileselection1 = create_fileselection1 ( user_data );

    g_print ("%s:%d vals.tiff_file %s icc:%d colour:%d==%d?\n",__FILE__,__LINE__,vals.tiff_file,vals.icc,vals.color_space, PT_CMYK);

    if ( vals.color_space == PT_CMYK
         || ( vals.icc && ( strstr (vals.o_profile,"cmyk") != NULL
                         || strstr (vals.o_profile,"CMYK") != NULL) ) ) {
      set_tiff_filename (PT_CMYK);
    } else if (!vals.icc && (strstr (vals.o_profile,"Lab") != NULL
                          || strstr (vals.o_profile,"lab") != NULL) ) {
      set_tiff_filename (PT_Lab);
      vals.color_space = PT_Lab;
    } else {
      set_tiff_filename (PT_RGB);
      vals.color_space = PT_RGB;
    }

    gtk_file_selection_set_filename (GTK_FILE_SELECTION(fileselection1),
                                     vals.tiff_file);
    gtk_widget_show (fileselection1);
    DBG

    gtk_widget_set_sensitive (user_data, FALSE);

    //vals.quit_app = TRUE;
    //gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)) );
    //gtk_main_quit ();
  }
}

void
on_reset_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  vals.intent = INTENT_PERCEPTUAL;
  vals.precalc = 0x00;
  vals.icc = TRUE;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (perc_radiobutton), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (midres_prec_radiobutton), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (direct_checkbutton), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_linearisation_checkbutton), FALSE);

  DBG
}


void
on_perc_radiobutton_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.intent = INTENT_PERCEPTUAL;
}


void
on_rel_color_radiobutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.intent = INTENT_RELATIVE_COLORIMETRIC;
}


void
on_satur_radiobutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.intent = INTENT_SATURATION;
}


void
on_abs_color_radiobutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.intent = INTENT_ABSOLUTE_COLORIMETRIC;
}


void
on_matrix_i_radiobutton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.matrix = cmsFLAGS_MATRIXINPUT;
}


void
on_matrix_o_radiobutton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.matrix = cmsFLAGS_MATRIXOUTPUT;
}


void
on_matrix_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.matrix = cmsFLAGS_MATRIXONLY;
}


void
on_null_transf_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.matrix = cmsFLAGS_NULLTRANSFORM;
}


void
on_hires_prec_radiobutton_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.precalc = cmsFLAGS_HIGHRESPRECALC;
}


void
on_midres_prec_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.precalc = 0x00;
}


void
on_lowres_prec_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.precalc = cmsFLAGS_LOWRESPRECALC;
}


void
on_no_prec_radiobutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    vals.precalc = cmsFLAGS_NOTPRECALC;
}


void
on_bpc_checkbutton_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (button)->active)
    vals.flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
  else
    vals.flags &= ~cmsFLAGS_BLACKPOINTCOMPENSATION;
  DBG_S(("flags (+-bpc):%d",vals.flags))
}


void
on_preserveblack_checkbutton_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON (button)->active)
    vals.preserveblack = cmsFLAGS_PRESERVEBLACK;
  else
    vals.preserveblack = 0x00;
}

// file dialog
void
on_file_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  cmsHPROFILE hProfile;
  gchar *filename;
  gchar *profile_info;
  int len;
  int check_ok = FALSE;
//  cmsCIEXYZ WhitePt;
  GtkWidget *entry = NULL;

  if (strcmp (profile_type , _("Separation")) == 0 ) {
    entry = select_oprofile_entry;
    check_ok = TRUE;
  } else if (strcmp (profile_type , _("Work Space")) == 0 ) {
    entry = select_iprofile_entry;
    check_ok = TRUE;
  } else if (strcmp (profile_type , _("Linearisation")) == 0 ) {
    entry = select_lprofile_entry;
    check_ok = TRUE;
  }



  filename = malloc(MAX_PATH);
  strcpy (filename, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselection1)) );


  // test for a valid profile and give some informations
  if (check_ok == TRUE) {
    hProfile = cmsOpenProfileFromFile(filename, "r");
    if (hProfile != NULL) {
      if ( strcmp (profile_type , _("Separation")) == 0
        || strcmp (profile_type , _("Work Space")) == 0
        || strcmp (profile_type , _("Linearisation")) == 0 )
      {
        check_ok = checkProfileDevice (profile_type, hProfile);
      }
      if (check_ok) {
        profile_info = getProfileInfo (hProfile);
        gtk_tooltips_set_tip (tooltips, entry, profile_info, NULL);

        // make clean here
        cmsCloseProfile (hProfile);
        g_free (profile_info);
      }

    } else { 
      // the lcms message should rise up
      check_ok = FALSE;
    }

    if (check_ok) {    
      gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET(fileselection1)));

      fileselection1 = NULL;

      // copy the filename to the actual text entry
      gtk_entry_set_text        (GTK_ENTRY(entry), filename);
      // workaround to make the right side with filename visible
      gtk_editable_set_position (GTK_EDITABLE(entry),
                                 GTK_ENTRY(entry)->text_length);
    }
  } else {
    if (vals.save_tiff == TRUE)
    {
      sprintf (vals.tiff_file,"%s",filename);
      g_print ("%s:%d %s -> %s\n",__FILE__,__LINE__,__func__,vals.tiff_file);
    } else {
      g_print ("%s:%d %s error\n",__FILE__,__LINE__,__func__);
    }

  }

  if (vals.save_tiff == TRUE) {
    gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (user_data)) );
    gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)) );
    gtk_main_quit ();
  }

  gtk_widget_set_sensitive(gtk_widget_get_toplevel(GTK_WIDGET(user_data)),TRUE);
  g_free (filename);
}


void
on_file_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  if (vals.save_tiff) {
    vals.save_tiff = FALSE;
  }

  gtk_widget_set_sensitive(gtk_widget_get_toplevel(GTK_WIDGET(user_data)),TRUE);
  gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)) );

  DBG
}

void
on_file_help_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_set_sensitive(gtk_widget_get_toplevel(GTK_WIDGET(user_data)),TRUE);

  DBG
}


// Linearisation

void
on_use_linearisation_checkbutton_clicked (GtkButton       *button,
                                          gpointer         user_data)
{
  char text[256] = "";
  int  thaw_widgets = FALSE;


  if (GTK_TOGGLE_BUTTON (button)->active && !vals.icc) {
    if (vals.color_space == PT_CMYK) {
      thaw_widgets = TRUE;
    }
  } else if (GTK_TOGGLE_BUTTON (button)->active && vals.icc) {
    sprintf (text, "%s%s%s%s%s", _("You need to be in \""), _("Direct print"),
             _("\" mode in order to linearise an printer.\nThis can be done in the tab \""), _("Print Options"), "\".");
    g_message ("%s", text);
  }

  linear.use_lin = thaw_widgets;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), thaw_widgets);
  gtk_widget_set_sensitive(GTK_WIDGET(frame_gamma), thaw_widgets);
  gtk_widget_set_sensitive(GTK_WIDGET(frame_Max), thaw_widgets);
  gtk_widget_set_sensitive(GTK_WIDGET(frame_Min), thaw_widgets);
  //gtk_widget_set_sensitive(GTK_WIDGET(lcms_options)->lcms_vbox, FALSE);
}





