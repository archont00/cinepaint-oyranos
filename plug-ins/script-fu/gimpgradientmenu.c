/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientmenu.c
 * Copyright (C) 1998 Andy Thomas                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "lib/image_limits.h"

#include "gimpmenu.h"
#include "gimpgradientmenu.h"
#include "gimpgradientselect.h"
#include "gimpgradientselect_pdb.h"

/* Idea is to have a function to call that returns a widget that 
 * completely controls the selection of a gradient.
 * you get a widget returned that you can use in a table say.
 * In:- Initial gradient name. Null means use current selection.
 *      pointer to func to call when gradient changes (GimpRunGradientCallback).
 * Returned:- Pointer to a widget that you can use in UI.
 * 
 * Widget simply made up of a preview widget (20x40) containing the gradient
 * which can be clicked on to changed the gradient selection.
 */


#define GSEL_DATA_KEY     "__gsel_data"
#define CELL_SIZE_HEIGHT  18
#define CELL_SIZE_WIDTH   84


struct __gradients_sel 
{
  gchar                   *dname;
  GimpRunGradientCallback  cback;
  GtkWidget               *gradient_preview;
  GtkWidget               *button;
  gchar                   *gradient_popup_pnt;
  gint                     width;
  gchar                   *gradient_name;      /* Local copy */
  gdouble                 *grad_data;          /* local copy */
  gint                     sample_size;
  gpointer                 data;
}; 

typedef struct __gradients_sel GSelect;

static void
gradient_pre_update (GtkWidget *gradient_preview,
		     gint       width_data,
		     gdouble   *grad_data)
{
  gint     x;
  gint     y;
  gdouble *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0;
  guchar  *p1;
  guchar  *even;
  guchar  *odd;
  gint     width;

  width = width_data / 4;

  /*  Draw the gradient  */
  src = grad_data;
  p0  = even = g_malloc (width * 3);
  p1  = odd  = g_malloc (width * 3);

  for (x = 0; x < width; x++) 
    {
      r =  src[x*4+0];
      g = src[x*4+1];
      b = src[x*4+2];
      a = src[x*4+3];
      
      if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	{
	  c0 = GIMP_CHECK_LIGHT;
	  c1 = GIMP_CHECK_DARK;
	} 
      else 
	{
	  c0 = GIMP_CHECK_DARK;
	  c1 = GIMP_CHECK_LIGHT;
	}
    
      *p0++ = (c0 + (r - c0) * a) * 255.0;
      *p0++ = (c0 + (g - c0) * a) * 255.0;
      *p0++ = (c0 + (b - c0) * a) * 255.0;
      
      *p1++ = (c1 + (r - c1) * a) * 255.0;
      *p1++ = (c1 + (g - c1) * a) * 255.0;
      *p1++ = (c1 + (b - c1) * a) * 255.0;
    }
  
  for (y = 0; y < CELL_SIZE_HEIGHT; y++)
    {
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (gradient_preview), 
			      (guchar *)odd, 0, y, 
			      (width < CELL_SIZE_WIDTH) ? width : CELL_SIZE_WIDTH);
      else
	  gtk_preview_draw_row (GTK_PREVIEW (gradient_preview), 
				(guchar *)even, 0, y, 
				(width < CELL_SIZE_WIDTH) ? width : CELL_SIZE_WIDTH);
    }

  g_free (even);
  g_free (odd);

  /*  Draw the brush preview  */
  gtk_widget_draw (gradient_preview, NULL);
}

static void
gradient_select_invoker (gchar    *name,
			 gint      width,
			 gdouble  *grad_data,
			 gint      closing,
			 gpointer  data)
{
  GSelect *gsel = (GSelect*)data;

  if (gsel->grad_data != NULL)
    g_free (gsel->grad_data);

  gsel->width = width;
  if (gsel->gradient_name)
    g_free (gsel->gradient_name);

  gsel->gradient_name = g_strdup (name);
  /* one row only each row has four doubles r,g,b,a */
  gsel->grad_data = g_new (gdouble, width);
  /*  printf("name = %s width = %d\n",name,width);*/
  g_memmove (gsel->grad_data, grad_data, width * sizeof (gdouble)); 
  gradient_pre_update (gsel->gradient_preview, gsel->width, gsel->grad_data);

  if (gsel->cback != NULL)
    (gsel->cback)(name, width, grad_data, closing, gsel->data);
  
  if (closing)
    gtk_widget_set_sensitive (gsel->button, TRUE);
}


static void
gradient_preview_callback (GtkWidget *widget,
			   gpointer   data)
{
  GSelect *gsel = (GSelect*)data;

  gtk_widget_set_sensitive (gsel->button, FALSE);

  gsel->gradient_popup_pnt = 
    gimp_interactive_selection_gradient ((gsel->dname) ? gsel->dname 
					               : "Gradient Plugin Selection",
					 gsel->gradient_name,
					 gsel->sample_size,
					 gradient_select_invoker,
					 gsel);
}

GtkWidget * 
gimp_gradient_select_widget (gchar                   *dname,
			     gchar                   *igradient, 
			     GimpRunGradientCallback  cback,
			     gpointer                 data)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *gradient;
  gint       width;
  gdouble   *grad_data;
  gchar     *gradient_name;
  GSelect   *gsel;
  
  gsel = g_new (GSelect, 1);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show(hbox);

  button = gtk_button_new ();

  gradient = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (gradient), CELL_SIZE_WIDTH, CELL_SIZE_HEIGHT); 
  gtk_widget_show (gradient);
  gtk_container_add (GTK_CONTAINER (button), gradient); 

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) gradient_preview_callback,
		      (gpointer)gsel);
  
  gtk_widget_show(button);

  gsel->button             = button;
  gsel->cback              = cback;
  gsel->data               = data;
  gsel->grad_data          = NULL;
  gsel->gradient_preview   = gradient;
  gsel->dname              = dname;
  gsel->gradient_popup_pnt = NULL;
  gsel->sample_size        = CELL_SIZE_WIDTH;

  /* Do initial gradient setup */
  gradient_name = 
    gimp_gradients_get_gradient_data (igradient, &width, CELL_SIZE_WIDTH, &grad_data);

  if (gradient_name)
    {
      gradient_pre_update (gsel->gradient_preview, width, grad_data);
      gsel->grad_data     = grad_data;
      gsel->gradient_name = gradient_name;
      gsel->width         = width;
    }
  else
    {
      gsel->gradient_name = g_strdup (igradient);
    }

  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  gtk_object_set_data (GTK_OBJECT (hbox), GSEL_DATA_KEY, (gpointer)gsel);

  return hbox;
}


void
gimp_gradient_select_widget_close_popup (GtkWidget *widget)
{
  GSelect  *gsel;
  
  gsel = (GSelect*) gtk_object_get_data (GTK_OBJECT (widget), GSEL_DATA_KEY);

  if (gsel && gsel->gradient_popup_pnt)
    {
      gimp_gradients_close_popup (gsel->gradient_popup_pnt);
      gsel->gradient_popup_pnt = NULL;
    }
}

void
gimp_gradient_select_widget_set_popup (GtkWidget *widget,
				       gchar     *gname)
{
  gint      width;
  gdouble  *grad_data;
  gchar    *gradient_name;
  GSelect  *gsel;
  
  gsel = (GSelect*) gtk_object_get_data (GTK_OBJECT (widget), GSEL_DATA_KEY);

  if (gsel)
    {
      gradient_name = 
	gimp_gradients_get_gradient_data (gname, &width, gsel->sample_size, &grad_data);
  
      if (gradient_name)
	{
	  gradient_select_invoker (gname, width, grad_data, 0, gsel);

	  if (gsel->gradient_popup_pnt)
	    gimp_gradients_set_popup (gsel->gradient_popup_pnt, gname);
	}
    }
}


