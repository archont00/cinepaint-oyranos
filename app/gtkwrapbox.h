/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkWrapBox: Wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GTK_WRAP_BOX_H__
#define __GTK_WRAP_BOX_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkcontainer.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define GTK_TYPE_WRAP_BOX	     (gtk_wrap_box_get_type ())
#define GTK_WRAP_BOX(obj)	     (GTK_CHECK_CAST ((obj), GTK_TYPE_WRAP_BOX, GtkWrapBox))
#define GTK_WRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_WRAP_BOX, GtkWrapBoxClass))
#define GTK_IS_WRAP_BOX(obj)	     (GTK_CHECK_TYPE ((obj), GTK_TYPE_WRAP_BOX))
#define GTK_IS_WRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_WRAP_BOX))
#define GTK_WRAP_BOX_GET_CLASS(obj)  (GTK_WRAP_BOX_CLASS (GTK_OBJECT_GET_CLASS(obj)))

/* --- typedefs --- */
typedef struct GtkWrapBox      GtkWrapBox;
typedef struct GtkWrapBoxClass GtkWrapBoxClass;
typedef struct GtkWrapBoxChild GtkWrapBoxChild;

/* --- GtkWrapBox --- */
struct GtkWrapBox
{
  GtkContainer     container;
  
  guint            homogeneous : 1;
  guint            justify : 4;
  guint            line_justify : 4;
  guint8           hspacing;
  guint8           vspacing;
  guint16          n_children;
  GtkWrapBoxChild *children;
  gfloat           aspect_ratio; /* 1/256..256 */
  guint            child_limit;
};
struct GtkWrapBoxClass
{
  GtkContainerClass parent_class;

  GSList* (*rlist_line_children) (GtkWrapBox       *wbox,
				  GtkWrapBoxChild **child_p,
				  GtkAllocation    *area,
				  guint            *max_child_size,
				  gboolean         *expand_line);
};
struct GtkWrapBoxChild
{
  GtkWidget *widget;
  guint      hexpand : 1;
  guint      hfill : 1;
  guint      vexpand : 1;
  guint      vfill : 1;
  guint      forced_break : 1;
  
  GtkWrapBoxChild *next;
};
#define GTK_JUSTIFY_TOP    GTK_JUSTIFY_LEFT
#define GTK_JUSTIFY_BOTTOM GTK_JUSTIFY_RIGHT


/* --- prototypes --- */
GtkType	   gtk_wrap_box_get_type            (void);
void	   gtk_wrap_box_set_homogeneous     (GtkWrapBox      *wbox,
					     gboolean         homogeneous);
void	   gtk_wrap_box_set_hspacing        (GtkWrapBox      *wbox,
					     guint            hspacing);
void	   gtk_wrap_box_set_vspacing        (GtkWrapBox      *wbox,
					     guint            vspacing);
void	   gtk_wrap_box_set_justify         (GtkWrapBox      *wbox,
					     GtkJustification justify);
void	   gtk_wrap_box_set_line_justify    (GtkWrapBox      *wbox,
					     GtkJustification line_justify);
void	   gtk_wrap_box_set_aspect_ratio    (GtkWrapBox      *wbox,
					     gfloat           aspect_ratio);
void	   gtk_wrap_box_pack	            (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill);
void       gtk_wrap_box_reorder_child       (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gint             position);
void       gtk_wrap_box_query_child_packing (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean        *hexpand,
					     gboolean        *hfill,
					     gboolean        *vexpand,
					     gboolean        *vfill);
void       gtk_wrap_box_query_child_forced_break (GtkWrapBox *wbox,
						  GtkWidget  *child,
						  gboolean   *forced_break);
void       gtk_wrap_box_set_child_packing   (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill);
void       gtk_wrap_box_set_child_forced_break (GtkWrapBox   *wbox,
						GtkWidget    *child,
						gboolean      forced_break);
guint*	   gtk_wrap_box_query_line_lengths  (GtkWrapBox	     *wbox,
					     guint           *n_lines);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_WRAP_BOX_H__ */
