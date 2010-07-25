/*
 * "$Id: print-lcms-options-callbacks.h,v 1.2 2005/12/29 10:45:09 beku Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 2004   Kai-Uwe Behrmann (web@tiscali.de)
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


/*
 *   This sections is intendet to set some options before converting an 
 *   cinepaint image into cmyk 16-bit colormode for gimp-print.
 */

void
on_fileselection1_set_focus            (GtkWindow       *window,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_file_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_file_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_direct_checkbutton_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_color_mode_rgb_radiobutton_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_color_mode_cmyk_radiobutton_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_select_iprofile_entry_insert_text   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_oprofile_entry_insert_text   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_lprofile_entry_changed       (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_iprofile_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_select_oprofile_button_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_select_lprofile_button_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_perc_radiobutton_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rel_color_radiobutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_satur_radiobutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_abs_color_radiobutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_matrix_i_radiobutton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_matrix_o_radiobutton_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_matrix_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_null_transf_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hires_prec_radiobutton_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_midres_prec_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_lowres_prec_radiobutton_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_no_prec_radiobutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bpc_checkbutton_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_preserveblack_checkbutton_clicked   (GtkButton       *button,
                                        gpointer         user_data);

void
on_use_linearisation_checkbutton_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data);


void
on_chancel_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_reset_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_tiff_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

