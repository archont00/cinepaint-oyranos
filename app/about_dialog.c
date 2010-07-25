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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>
#include "config.h"
#include "../lib/version.h"
#include "../lib/compat.h"
#include "libgimp/gimpintl.h"

#include "about_dialog.h"
#include "interface.h"

#include "minimize.h"
#include "app_procs.h"
#include "../lib/wire/datadir.h"


#ifdef WIN32
#include "unistd.h"
#include "extra.h"
#endif

#define ANIMATION_STEPS 16
#define ANIMATION_SIZE 2

static int  about_dialog_load_logo (GtkWidget *window);
static void about_dialog_destroy (void);
static void about_dialog_unmap (void);
static int  about_dialog_logo_expose (GtkWidget *widget, GdkEventExpose *event);
static int  about_dialog_button (GtkWidget *widget, GdkEventButton *event);
#ifdef SCROLL_NAMES
static int  about_dialog_timer (gpointer data);
#endif

static GtkWidget *about_dialog = NULL;
static GtkWidget *logo_area = NULL;
static GtkWidget *scroll_area = NULL;
/*static GdkPixmap *logo_pixmap = NULL;*/
static GdkPixmap *scroll_pixmap = NULL;
static unsigned char *dissolve_map = NULL;
static int dissolve_width;
static int dissolve_height;
static int logo_width = 0;
static int logo_height = 0;
static int do_animation = 0;
static int do_scrolling = 0;
static int scroll_state = 0;
static int frame = 0;
static int offset = 0;
static int timer = 0;

#ifdef SCROLL_NAMES

static char *scroll_text[] =
{
  "The Film Gimp Team:",
  "Kai-Uwe Behrmann",
  "Jonathan Cohen",
  "Doug Creel",
  "Caroline Dahllof",
  "Allen Davidson",
  "Cyril Gyot",
  "Drew Hess",
  "David Hodson",
  "Andrew Lau",
  "Glenn Powers",
  "Andy Prock",
  "Karl Rasche",
  "Rene Rask",
  "Sam Richards",
  "Robin Rowe",
  "Nathan Wilson",
  "Thanks to:",
  "LinuxFund",
  "Rhythm & Hues",
  "Sony Pictures Imageworks",
  "SourceForge",
  "Based on the work of",
  "The Gimp 1.0.4 Team:",
  "Lauri Alanko",
  "Shawn Amundson",
  "John Beale",
  "Zach Beane",
  "Tom Bech",
  "Marc Bless",
  "Edward Blevins",
  "Roberto Boyd",
  "Seth Burgess",
  "Brent Burton",
  "Francisco Bustamante",
  "Ed Connel",
  "Andreas Dilger",
  "Misha Dynin",
  "Larry Ewing",
  "David Forsyth",
  "Jim Geuther",
  "Scott Goehring",
  "Heiko Goller",
  "Michael Hammel",
  "Christoph Hoegl",
  "Jan Hubicka",
  "Simon Janes",
  "Tim Janik",
  "Tuomas Kuosmanen",
  "Peter Kirchgessner", 
  "Nick Lamb",
  "Karl LaRocca",
  "Jens Lautenbacher",
  "Laramie Leavitt",
  "Elliot Lee",
  "Raph Levien",
  "Adrian Likins",
  "Ingo Luetkebohle",
  "Josh MacDonald",
  "Ed Mackey",
  "Marcelo Malheiros",
  "Ian Main",
  "Torsten Martinsen",
  "Federico Mena",
  "Adam D. Moss",
  "Shuji Narazaki",
  "Sven Neumann",
  "Stephen Robert Norris",
  "Erik Nygren",
  "Miles O'Neal",
  "Jay Painter",
  "Mike Phillips",
  "Raphael Quinet",
  "James Robinson",
  "Mike Schaeffer",
  "Tracy Scott",
  "Manish Singh",
  "Nathan Summers",
  "Mike Sweet",
  "Eiichi Takamori",
  "Tristan Tarrant",
  "Owen Taylor",
  "Ian Tester",
  "Andy Thomas",
  "James Wang",
  "Kris Wehner",
  "Matthew Wilson",
  "and many others...",
};


static int nscroll_texts = sizeof (scroll_text) / sizeof (scroll_text[0]);
static int scroll_text_widths[100] = { 0 };
static int cur_scroll_text = 0;
static int cur_scroll_index = 0;

static int shuffle_array[ sizeof(scroll_text) / sizeof(scroll_text[0]) ];
#endif

void
about_dialog_create (int timeout)
{
  GtkStyle *style;
  GtkWidget *vbox;
  GtkWidget *aboutframe;
  GtkWidget *label;
  GtkWidget *alignment;
  gint max_width;
  gint i;

  if (!about_dialog)
    {
      char title[256];
      snprintf( title, 256, "%s " PROGRAM_NAME " " PROGRAM_VERSION, _("About"));
#if GTK_MAJOR_VERSION > 1
      about_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_type_hint (GTK_WINDOW(about_dialog),
                                GDK_WINDOW_TYPE_HINT_DIALOG);
#else
      about_dialog = gtk_window_new (GTK_WINDOW_DIALOG);
#endif
      minimize_register(about_dialog);
      gtk_window_set_wmclass (GTK_WINDOW (about_dialog), "about_dialog", "cinepaint");
      gtk_window_set_title (GTK_WINDOW (about_dialog), title);
      gtk_window_set_policy (GTK_WINDOW (about_dialog), FALSE, FALSE, FALSE);
      gtk_window_position (GTK_WINDOW (about_dialog), GTK_WIN_POS_CENTER);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "destroy",
			  (GtkSignalFunc) about_dialog_destroy, NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "unmap_event",
			  (GtkSignalFunc) about_dialog_unmap, NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "button_press_event",
			  (GtkSignalFunc) about_dialog_button, NULL);
      gtk_widget_set_events (about_dialog, GDK_BUTTON_PRESS_MASK);

      if (!about_dialog_load_logo (about_dialog))
	{
	  gtk_widget_destroy (about_dialog);
	  about_dialog = NULL;
	  return;
	}

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (about_dialog), vbox);
      gtk_widget_show (vbox);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_box_pack_start (GTK_BOX (vbox), aboutframe, TRUE, TRUE, 0);
      gtk_widget_show (aboutframe);

      logo_area = gtk_drawing_area_new ();
      gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
			  (GtkSignalFunc) about_dialog_logo_expose, NULL);
      gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area), logo_width, logo_height);
      gtk_widget_set_events (logo_area, GDK_EXPOSURE_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), logo_area);
      gtk_widget_show (logo_area);

      gtk_widget_realize (logo_area);
      gdk_window_set_background (logo_area->window, &logo_area->style->black);


      style = gtk_style_new ();
#if 0 
#__APPLE__
      gdk_font_unref (gtk_style_get_font(style));
      gtk_style_set_font(style, gdk_font_load ("-Adobe-Helvetica-Medium-R-Normal--*-140-*-*-*-*-*-*"));
#endif
#if GTK_MAJOR_VERSION < 2
      gtk_widget_push_style (style);

      gtk_widget_pop_style ();
#endif

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);
      gtk_widget_show (alignment);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_container_add (GTK_CONTAINER (alignment), aboutframe);
      gtk_widget_show (aboutframe);

#ifdef SCROLL_NAMES

      max_width = 0;
      for (i = 0; i < nscroll_texts; i++)
	{
	  scroll_text_widths[i] = gdk_string_width (aboutframe->style->font, scroll_text[i]);
	  max_width = MAX (max_width, scroll_text_widths[i]);
	}

      scroll_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (scroll_area),
			     max_width + 10,
			     aboutframe->style->font->ascent +
			     aboutframe->style->font->descent);
      gtk_widget_set_events (scroll_area, GDK_BUTTON_PRESS_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), scroll_area);
      gtk_widget_show (scroll_area);
#endif
      label = gtk_label_new (_("To ask questions or report bugs visit www.CinePaint.org"));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);
#ifdef SCROLL_NAMES
      gtk_widget_realize (scroll_area);
      gdk_window_set_background (scroll_area->window, &scroll_area->style->white);
#endif
    }

  if (!GTK_WIDGET_VISIBLE (about_dialog))
    {
      gtk_widget_show (about_dialog);
#ifdef SCROLL_NAMES
      do_animation = TRUE;
      do_scrolling = FALSE;
      scroll_state = 0;
      frame = 0;
      offset = 0;

      for (i = 0; i < nscroll_texts; i++) 
	{
	  shuffle_array[i] = i;
	}

      for (i = 0; i < nscroll_texts; i++) 
	{
	  int j, k;
	  j = ((int)drand48()) % nscroll_texts;
	  k = ((int)drand48()) % nscroll_texts;
	  if (j != k) 
	    {
	      int t;
	      t = shuffle_array[j];
	      shuffle_array[j] = shuffle_array[k];
	      shuffle_array[k] = t;
	    }
	}
#endif
    }
  else 
    {
      gdk_window_raise(about_dialog->window);
    }
}

int load_splash_ppm(GtkWidget *window)
{ char buf[1024];
  FILE *fp;
  int count;
  GtkWidget *preview;
  GdkGC *gc;
  unsigned char *pixelrow;
  int i;

  sprintf (buf, "%s/" SPLASH_FILE, GetDirData());

  fp = fopen (buf, "rb");
  if (!fp)
    return 0;

  fgets (buf, 1024, fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  fgets (buf, 1024, fp);
  fgets (buf, 1024, fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, 1024, fp);
  if (strcmp (buf, "255\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, sizeof (unsigned char), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return FALSE;
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height, 
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);
  return TRUE;
}

static int
about_dialog_load_logo (GtkWidget *window)
{


  int i, j, k;

  if (logo_pixmap)
    return TRUE;
#if !0
  load_splash_ppm(window);
#else
  splash_logo_load(window);
#endif
  dissolve_width = (logo_width / ANIMATION_SIZE)+(logo_width % ANIMATION_SIZE ==0 ?0 : 1);
  dissolve_height = (logo_height / ANIMATION_SIZE)+(logo_height % ANIMATION_SIZE ==0 ?0 : 1);

  dissolve_map = g_new (guchar, dissolve_width * dissolve_height);

  for (i = 0, k = 0; i < dissolve_height; i++)
    for (j = 0; j < dissolve_width; j++, k++)
      dissolve_map[k] = ((int)drand48 ()) % ANIMATION_STEPS;

  return TRUE;
}

static void
about_dialog_destroy ()
{
  about_dialog = NULL;
  about_dialog_unmap ();
}

static void
about_dialog_unmap ()
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}

static int
about_dialog_logo_expose (GtkWidget      *widget,
			  GdkEventExpose *event)
{
#ifdef SCROLL_NAMES
  if (do_animation)
    {
      if (!timer)
	{
	  about_dialog_timer (widget);
	  timer = gtk_timeout_add (75, about_dialog_timer, NULL);
	}
    }
  else
#endif
    {
      /* If we draw beyond the boundaries of the pixmap, then X
	 will generate an expose area for those areas, starting
	 an infinite cycle. We now set allow_grow = FALSE, so
	 the drawing area can never be bigger than the preview.
         Otherwise, it would be necessary to intersect event->area
         with the pixmap boundary rectangle. */

      gdk_draw_pixmap (widget->window,
		       widget->style->black_gc,
		       logo_pixmap, 
		       event->area.x, event->area.y,
		       event->area.x, event->area.y,
		       event->area.width, event->area.height);
    }

  return FALSE;
}

static int
about_dialog_button (GtkWidget      *widget,
		     GdkEventButton *event)
{
  if (timer)
    gtk_timeout_remove (timer);
  timer = 0;
  frame = 0;

  gtk_widget_hide (about_dialog);

  return FALSE;
}

#ifdef SCROLL_NAMES

static int
about_dialog_timer (gpointer data)
{
  gint i, j, k;
  gint return_val;

  return_val = TRUE;

  if (do_animation)
    {
      if(logo_area->allocation.width != 1)
	{
	  for (i = 0, k = 0; i < dissolve_height; i++)
	    for (j = 0; j < dissolve_width; j++, k++)
	      if (frame == dissolve_map[k])
		{
		  gdk_draw_pixmap (logo_area->window,
				   logo_area->style->black_gc,
				   logo_pixmap,
				   j * ANIMATION_SIZE, i * ANIMATION_SIZE, j * ANIMATION_SIZE, i * ANIMATION_SIZE,
				   ANIMATION_SIZE, ANIMATION_SIZE);
		}

	  frame += 1;

	  if (frame == ANIMATION_STEPS)
	    {
	      do_animation = FALSE;
	      do_scrolling = TRUE;
	      frame = 0;

	      timer = gtk_timeout_add (75, about_dialog_timer, NULL);

	      return FALSE;
	    }
	}
    }

  if (do_scrolling)
    {
      if (!scroll_pixmap)
	scroll_pixmap = gdk_pixmap_new (scroll_area->window,
					scroll_area->allocation.width,
					scroll_area->allocation.height,
					-1);
  

      switch (scroll_state)
	{
	case 1:
	  scroll_state = 2;
	  timer = gtk_timeout_add (700, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	case 2:
	  scroll_state = 3;
	  timer = gtk_timeout_add (75, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	}

      if (offset > (scroll_text_widths[cur_scroll_text] + scroll_area->allocation.width))
	{
	  scroll_state = 0;
	  cur_scroll_index += 1;
	  if (cur_scroll_index == nscroll_texts)
	    cur_scroll_index = 0;
	  
	  cur_scroll_text = shuffle_array[cur_scroll_index];

	  offset = 0;
	}

      gdk_draw_rectangle (scroll_pixmap,
			  scroll_area->style->white_gc,
			  TRUE, 0, 0,
			  scroll_area->allocation.width,
			  scroll_area->allocation.height);
      gdk_draw_string (scroll_pixmap,
		       scroll_area->style->font,
		       scroll_area->style->black_gc,
		       scroll_area->allocation.width - offset,
		       scroll_area->style->font->ascent,
		       scroll_text[cur_scroll_text]);
      gdk_draw_pixmap (scroll_area->window,
		       scroll_area->style->black_gc,
		       scroll_pixmap, 0, 0, 0, 0,
		       scroll_area->allocation.width,
		       scroll_area->allocation.height);

      offset += 15;
      if (scroll_state == 0)
	{
	  if (offset > ((scroll_area->allocation.width + scroll_text_widths[cur_scroll_text]) / 2))
	    {
	      scroll_state = 1;
	      offset = (scroll_area->allocation.width + scroll_text_widths[cur_scroll_text]) / 2;
	    }
	}
    }

  return return_val;
}

#endif
