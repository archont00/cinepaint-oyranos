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
#include "signal_type.h"
#include "listP.h"
#include <gtk/gtk.h>
#if GTK_MAJOR_VERSION > 1
//#include "gimpcontainer.h"
#else
#include "compat.h"
#endif

/* code mostly ripped from nether's gimpset class  */

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static guint gimp_list_signals [LAST_SIGNAL];

static GimpObjectClass* parent_class = NULL;

static void gimp_list_add_func(GimpList *list, void *);
static void gimp_list_remove_func(GimpList *list, void *);

static void
gimp_list_destroy (GtkObject* ob)
{
	GimpList* list=GIMP_LIST(ob);
	while(list->list) /* ought to put a sanity check in here... */
	{
	  gimp_list_remove(list, list->list->data);
	}
	g_slist_free(list->list);
        if(parent_class)
          GTK_OBJECT_CLASS(parent_class)->destroy (ob);
}

static void
gimp_list_init (GimpList* list)
{
	list->list=NULL;
	list->type=GTK_TYPE_OBJECT;
}

static void
gimp_list_class_init (GimpListClass* klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
	GtkType type = GTK_CLASS_TYPE(object_class);

#if GTK_MAJOR_VERSION > 1
        parent_class = g_type_class_peek_parent (klass);
#else
	parent_class=gtk_type_parent_class(type);
#endif	
	object_class->destroy = gimp_list_destroy;
	
#if GTK_MAJOR_VERSION > 1
	gimp_list_signals[ADD]=
          g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpListClass, add),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	gimp_list_signals[REMOVE]=
          g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpListClass, add),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
#else
	gimp_list_signals[ADD]=
	  gimp_signal_new ("add", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_pointer);
	gimp_list_signals[REMOVE]=
	  gimp_signal_new ("remove", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_pointer);
	gtk_object_class_add_signals (object_class,
				      gimp_list_signals,
				      LAST_SIGNAL);
#endif
	klass->add    = gimp_list_add_func;
	klass->remove = gimp_list_remove_func;
}

GtkType gimp_list_get_type (void)
{
	static GtkType type = 0;
#if GTK_MAJOR_VERSION > 1
  if (! type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_list_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpList),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_list_init,
      };

      type = g_type_register_static (GIMP_TYPE_OBJECT,
                                          "GimpList",
                                          &list_info, 0);
    }
#else
	GIMP_TYPE_INIT(type,
		       GimpList,
		       GimpListClass,
		       gimp_list_init,
		       gimp_list_class_init,
		       GIMP_TYPE_OBJECT);
#endif
	return type;
}


GimpList*
gimp_list_new (GtkType type, gboolean weak){
	GimpList* list=gtk_type_new (gimp_list_get_type ());
	list->type=type;
	list->weak=weak;
	return list;
}

static void
gimp_list_destroy_cb (GtkObject* ob, gpointer data){
	GimpList* list=GIMP_LIST(data);
	gimp_list_remove(list, ob);
}

static void
gimp_list_add_func(GimpList* list, gpointer val)
{
  list->list=g_slist_prepend(list->list, val);
}

static void
gimp_list_remove_func(GimpList* list, gpointer val)
{
  list->list=g_slist_remove(list->list, val);
}

gboolean
gimp_list_add (GimpList* list, gpointer val)
{
  g_return_val_if_fail(list, FALSE);
  g_return_val_if_fail(GTK_CHECK_TYPE(val, list->type), FALSE);
	
  if(g_slist_find(list->list, val))
    return FALSE;
	
  if(list->weak)
    gtk_signal_connect(GTK_OBJECT(val), "destroy",
		       GTK_SIGNAL_FUNC(gimp_list_destroy_cb), list);
  else
    gtk_object_ref(GTK_OBJECT(val));
	
  GIMP_LIST_CLASS(G_OBJECT_GET_CLASS(list))->add(list, val);

#if GTK_MAJOR_VERSION < 2
  gtk_signal_emit (GTK_OBJECT(list), gimp_list_signals[ADD], val);
#endif
  return TRUE;
}

gboolean
gimp_list_remove (GimpList* list, gpointer val)
{
	g_return_val_if_fail(list, FALSE);

	if(!g_slist_find(list->list, val))
	{
	  fprintf (stderr, "gimp_list_remove: can't find val\n");
	  return FALSE;
	}
	GIMP_LIST_CLASS(GTK_OBJECT_GET_CLASS(list))->remove(list, val);

#if GTK_MAJOR_VERSION < 2
	gtk_signal_emit (GTK_OBJECT(list), gimp_list_signals[REMOVE], val);
#endif

	if(list->weak)
		gtk_signal_disconnect_by_func
			(GTK_OBJECT(val),
			 GTK_SIGNAL_FUNC(gimp_list_destroy_cb),
			 list);
	else
		gtk_object_unref(GTK_OBJECT(val));

	return TRUE;
}

gboolean
gimp_list_have (GimpList* list, gpointer val) {
	return g_slist_find(list->list, val)?TRUE:FALSE;
}

void
gimp_list_foreach(GimpList* list, GFunc func, gpointer user_data)
{
	g_slist_foreach(list->list, func, user_data);
}

GtkType
gimp_list_type (GimpList* list){
	return list->type;
}
