/*
 *   gui stuff for CinePaint
 *
 *   Copyright 2002-2004 Kai-Uwe Behrmann
 *
 *   separate gui stuff
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  March 2004
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


#ifdef HAVE_CONFIG_H
/*#  include <config.h> */
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>

#include "tiff_plug_in.h"
#include "info.h"
#include "gui.h"

#ifndef DEBUG
/* #define DEBUG */
#endif

#ifdef DEBUG
 #define DBG(text) printf("%s:%d %s() %s\n",__FILE__,__LINE__,__func__,text);
#else
 #define DBG(text) 
#endif

/*** gloabal variables ***/



/*** function definitions ***/

  /* --- gui functions --- */

#if GIMP_MAJOR_VERSION >= 1  /* GIMP */
static gint   save_dialog    (  ImageInfo       *info)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;

#if GIMP_MINOR_VERSION > 2
  dlg = gimp_dialog_new ( _("Save as TIFF"), "tiff",
                         NULL, 0,
			 gimp_standard_help_func, "filters/tiff.html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);
#else
  dlg = gimp_dialog_new ( _("Save as TIFF"), "tiff",
			 gimp_standard_help_func, "filters/tiff.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);
#endif

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, FALSE, TRUE, 0);

  /*  compression  */
#if GIMP_MINOR_VERSION > 2
  frame = gimp_int_radio_group_new (TRUE, _("Compression"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &tsvals_.compression, tsvals.compression,

                                    _("_None"),      COMPRESSION_NONE,     NULL,
                                    _("_LZW"),       COMPRESSION_LZW,      NULL,
                                    _("_Pack Bits"), COMPRESSION_PACKBITS, NULL,
                                    _("_Deflate"),   COMPRESSION_DEFLATE,  NULL,
                                    _("_JPEG"),      COMPRESSION_JPEG,     NULL,
                                    _("_JP2000"),    COMPRESSION_JP2000,   NULL,
                                    _("_Adobe"),     COMPRESSION_ADOBE_DEFLATE,     NULL,

				    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  frame = gimp_int_radio_group_new (TRUE, _("Fill Order"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &tsvals_.fillorder, tsvals.fillorder,

                                    _("_LSB to MSB"),FILLORDER_LSB2MSB,    NULL,
                                    _("_MSB to LSB"),FILLORDER_MSB2LSB,    NULL,

				    NULL);
#else
  frame =
    gimp_radio_group_new2 (TRUE, _("Compression"),
			   gimp_radio_button_update,
			   &tsvals_.compression, GINT_TO_POINTER (tsvals.compression),

			   _("None"),      GINT_TO_POINTER (COMPRESSION_NONE), NULL,
			   _("LZW"),       GINT_TO_POINTER (COMPRESSION_LZW), NULL,
			   _("Pack Bits"), GINT_TO_POINTER (COMPRESSION_PACKBITS), NULL,
			   _("Deflate"),   GINT_TO_POINTER (COMPRESSION_DEFLATE), NULL,
			   _("JPEG"),      GINT_TO_POINTER (COMPRESSION_JPEG), NULL,
			   _("JP2000"),      GINT_TO_POINTER (COMPRESSION_JP2000), NULL,
			   _("Adobe"),      GINT_TO_POINTER (COMPRESSION_ADOBE_DEFLATE), NULL,

			   NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  frame =
    gimp_radio_group_new2 (TRUE, _("Fill Order"),
			   gimp_radio_button_update,
			   &tsvals_.fillorder, GINT_TO_POINTER (tsvals.fillorder),

                           _("LSB to MSB"),      FILLORDER_LSB2MSB,    NULL,
                           _("MSB to LSB"),      FILLORDER_MSB2LSB,    NULL,

			   NULL);
#endif

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* comment entry */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ( _("Comment:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  if (image_comment && strlen (image_comment) > 1)
    gtk_entry_set_text (GTK_ENTRY (entry), image_comment);
#if GIMP_MINOR_VERSION <= 2
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      GTK_SIGNAL_FUNC (text_entry_callback),
                      image_comment);
#endif
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (dlg);

#if GIMP_MINOR_VERSION > 2
  tsint.run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);
#else
  gtk_main ();
#endif
  gdk_flush ();

  return tsint.run;
}

#if GIMP_MINOR_VERSION <= 2
static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}
#endif
#else  /* CinePaint */

#define LIST_FRAME(name) \
  frame = gtk_frame_new (name); \
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN); \
  gtk_container_border_width (GTK_CONTAINER (frame), 10);\
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, TRUE, 0);\
  toggle_vbox = gtk_vbox_new (FALSE, 5); \
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5); \
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

#define LIST_TOOGLE( name, variable ) \
  toggle = gtk_radio_button_new_with_label (group, name); \
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle)); \
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0); \
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled", \
		      (GtkSignalFunc) gimp_toggle_button_update, \
		      &variable); \
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), variable); \
  gtk_widget_show (toggle);

#define TEXT_ENTRY( name , callback , text ) \
  hbox = gtk_hbox_new (FALSE, 1); \
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0); \
  gtk_widget_show (hbox); \
  \
  label = gtk_label_new ( name ); \
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0); \
  gtk_widget_show (label); \
  \
  entry = gtk_entry_new (); \
  gtk_widget_show (entry); \
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0); \
  if (image_artist && strlen (text) > 1) \
    gtk_entry_set_text (GTK_ENTRY (entry), text); \
  gtk_signal_connect (GTK_OBJECT (entry), "changed", \
                      (GtkSignalFunc) callback, \
                      text);

gint
save_dialog                  (  ImageInfo       *info)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *frame;
  GtkWidget *toggle_vbox;
  GSList *group;
  gint use_none = (info->save_vals.compression == COMPRESSION_NONE);
  gint use_lzw = (info->save_vals.compression == COMPRESSION_LZW);
  gint use_packbits = (info->save_vals.compression == COMPRESSION_PACKBITS);
  gint use_deflate = (info->save_vals.compression == COMPRESSION_DEFLATE);
  gint use_sgilog = (info->save_vals.compression == COMPRESSION_SGILOG);
  gint use_pixarlog = (info->save_vals.compression == COMPRESSION_PIXARLOG);
  gint use_jpeg = (info->save_vals.compression == COMPRESSION_JPEG);
  gint use_jp2000 = (info->save_vals.compression == COMPRESSION_JP2000);
  gint use_adobe = (info->save_vals.compression == COMPRESSION_ADOBE_DEFLATE);
  gint use_lsb2msb = (info->save_vals.fillorder == FILLORDER_LSB2MSB);
  gint use_msb2lsb = (info->save_vals.fillorder == FILLORDER_MSB2LSB);
  gint use_sep_planar = (info->planar == PLANARCONFIG_SEPARATE);
  gint use_premultiply =(info->save_vals.premultiply == EXTRASAMPLE_ASSOCALPHA);
  gint use_notpremultiply = (info->save_vals.premultiply == EXTRASAMPLE_UNASSALPHA
                    || info->save_vals.premultiply == EXTRASAMPLE_UNSPECIFIED );
  gint use_icc_profile = (info->save_vals.use_icc_profile == TRUE);
  gint use_no_icc      = (info->save_vals.use_icc_profile == FALSE);
  long dircount;
  gchar text[256];

  gimp_ui_init ("save", TRUE);

  dlg = gimp_dialog_new (_("Save as Tiff"), "save",
                            gimp_standard_help_func, "filters/tiff.html",
                            GTK_WIN_POS_MOUSE,
                            FALSE, TRUE, FALSE,

                            _("OK"), ok_callback,
                            NULL, NULL, NULL, TRUE, FALSE,
                            _("Cancel"), cancel_callback,
                            NULL, 1, NULL, FALSE, TRUE,
 
                            NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback,
		      NULL);

  /*  attach profile  */
  //LIST_FRAME (_("ICC Profile data"))
  frame = gtk_frame_new (_("ICC Profile data"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, FALSE, TRUE, 0);
  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  group = NULL;

  LIST_TOOGLE ( _("embedd ICC Profile"), use_icc_profile )
  LIST_TOOGLE ( _("dont attach ICC color data"), use_no_icc )

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);
  if (info->save_vals.use_icc_profile == -1)
    gtk_widget_set_sensitive (frame, FALSE); 

  /*  compression  */
  LIST_FRAME (_("Compression"))
  group = NULL;

  LIST_TOOGLE (_("None"), use_none)
#ifdef LZW_SUPPORT
  LIST_TOOGLE (_("LZW"), use_lzw)
#endif
#ifdef PACKBITS_SUPPORT
  LIST_TOOGLE (_("Pack Bits"), use_packbits)
#endif
/*#ifdef ZIP_SUPPORT */
  LIST_TOOGLE ( _("Deflate"), use_deflate)
/*#endif */
/*#ifdef LOGLUV_SUPPORT */
  LIST_TOOGLE (_("SGILOG (32-bit)") , use_sgilog)
  gtk_widget_set_sensitive (toggle, FALSE);
  if (info->bps >= 32 && SGILOGDATAFMT == SGILOGDATAFMT_FLOAT) {
    if (info->profile_size) {
      cmsHPROFILE p = cmsOpenProfileFromMem (info->icc_profile,
                                             info->profile_size);
      if (cmsGetColorSpace(p) == icSigXYZData
       || cmsGetColorSpace(p) == icSigRgbData)
        gtk_widget_set_sensitive (toggle, TRUE);
    }
  }
/*#endif */
#if 0
/*#ifdef PIXARLOG_SUPPORT */
  LIST_TOOGLE ( _("PixarLOG (11-bit)"), use_pixarlog)
  if (info->bps < 32 && SGILOGDATAFMT == SGILOGDATAFMT_FLOAT)
    gtk_widget_set_sensitive (toggle, FALSE); 
/*#endif */
#endif
/*#ifdef JPEG_SUPPORT */
  LIST_TOOGLE ( _("JPEG"), use_jpeg)
  if (info->bps > 8)
    gtk_widget_set_sensitive (toggle, FALSE); 
#if 0
  LIST_TOOGLE ( _("JP2000"), use_jp2000)
#endif
/*#endif */
  LIST_TOOGLE ( _("Adobe"), use_adobe)

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  fillorder  */
  LIST_FRAME (_("Fill Order"))
  group = NULL;

  LIST_TOOGLE ( _("LSB to MSB"), use_lsb2msb )
  LIST_TOOGLE ( _("MSB to LSB"), use_msb2lsb )

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /*  premultiply  */
  LIST_FRAME (_("Transparency Computation"))
  group = NULL;

  sprintf (text,_("Premultiply Colour (associate)"));
  if ( info->photomet == PHOTOMETRIC_RGB )
    sprintf (text,"%s - Standard", text);
  LIST_TOOGLE ( text, use_premultiply )
  LIST_TOOGLE ( _("Dont Premultiply"), use_notpremultiply )

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /* separate planes */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);
 
  toggle = gtk_check_button_new_with_label (_("Write Separate Planes per Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &use_sep_planar);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), use_sep_planar);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, FALSE, TRUE, 0);

  if (info->save_vals.premultiply == EXTRASAMPLE_UNSPECIFIED)
    gtk_widget_set_sensitive (frame, FALSE); 


  TEXT_ENTRY ( _("Artist:"), text_entry_callback , image_artist )
  TEXT_ENTRY ( _("Copyright:"), text_entry_callback , image_copyright )
  TEXT_ENTRY ( _("Comment:"), text_entry_callback , image_comment )

  gtk_widget_show (vbox);

  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  if (use_icc_profile)
    tsvals_.use_icc_profile = TRUE;
  else if (use_no_icc)
    tsvals_.use_icc_profile = FALSE;

  if (use_none)
    tsvals_.compression = COMPRESSION_NONE;
  else if (use_lzw)
    tsvals_.compression = COMPRESSION_LZW;
  else if (use_packbits)
    tsvals_.compression = COMPRESSION_PACKBITS;
  else if (use_deflate)
    tsvals_.compression = COMPRESSION_DEFLATE;
  else if (use_sgilog)
    tsvals_.compression = COMPRESSION_SGILOG;
  else if (use_pixarlog)
    tsvals_.compression = COMPRESSION_PIXARLOG;
  else if (use_jpeg)
    tsvals_.compression = COMPRESSION_JPEG;
  else if (use_jp2000)
    tsvals_.compression = COMPRESSION_JP2000;
  else if (use_adobe)
    tsvals_.compression = COMPRESSION_ADOBE_DEFLATE;

  if (use_lsb2msb)
    tsvals_.fillorder = FILLORDER_LSB2MSB;
  else if (use_msb2lsb)
    tsvals_.fillorder = FILLORDER_MSB2LSB;

  if(use_sep_planar)
    tsvals_.planar_separate = PLANARCONFIG_SEPARATE;
  else
    tsvals_.planar_separate = PLANARCONFIG_CONTIG;

  if (use_premultiply
   && info->save_vals.premultiply != EXTRASAMPLE_UNSPECIFIED) {
    tsvals_.premultiply = EXTRASAMPLE_ASSOCALPHA;
  } else if (use_notpremultiply
        && info->save_vals.premultiply != EXTRASAMPLE_UNSPECIFIED) {
    tsvals_.premultiply = EXTRASAMPLE_UNASSALPHA;
  }

  info = info->top;
  for (dircount=0; dircount < info->pagecount ; dircount++) {
    if (dircount > 0)
      info = info->next;
 
    info->save_vals.use_icc_profile = tsvals_.use_icc_profile;
    info->save_vals.compression = tsvals_.compression;
    info->save_vals.fillorder = tsvals_.fillorder;
    if (info->save_vals.premultiply != EXTRASAMPLE_UNSPECIFIED) {
      info->save_vals.premultiply = tsvals_.premultiply;
    }
    if (use_sgilog || use_pixarlog) {
      if (info->spp > 2)
        info->photomet = PHOTOMETRIC_LOGLUV;
      else
        info->photomet = PHOTOMETRIC_LOGL;
      if (use_sgilog)
        info->logDataFMT = SGILOGDATAFMT;
      else
        info->logDataFMT = PIXARLOGDATAFMT_FLOAT;
    }
    info->planar = tsvals_.planar_separate;
  }

  info = info->top;

  /*g_message ("%d",tsint.run); */
  return tsint.run;
}


/*  interface functions  */

void
close_callback      (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

void
ok_callback      (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

void
cancel_callback  (GtkWidget *widget,
		  gpointer   data)
{
  tsint.run = FALSE;
  gtk_main_quit ();
  gtk_widget_destroy (GTK_WIDGET (data));
}


#endif /* cinepaint */


void
text_entry_callback   ( GtkWidget *widget,
			gpointer   data)
{
  gint  len;
  gchar *text=NULL,
       *string_ptr = (char*) data;

  text = gtk_entry_get_text (GTK_ENTRY (widget));
  len = strlen ( text );
  if (len > 0) {
    if (len > 240)
    {
      g_message (_("Your string is too long.\n"));
      return;
    } else {
      strcpy ( string_ptr, text);
    }
  } else {
    string_ptr[0] = '\000';
  }
}




