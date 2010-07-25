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
#ifndef __GIMPLISTP_H__
#define __GIMPLISTP_H__

#include <gtk/gtk.h>
#include "objectP.h"
#include "list.h"

struct GimpList
{
  GimpObject gobject;
  GtkType type;
  GSList* list;
  gboolean weak;
};

struct GimpListClass
{
  GimpObjectClass parent_class;

  void (* add)    (GimpList *list, void *data);
  void (* remove) (GimpList *list, void *data);
};

typedef struct GimpListClass GimpListClass;

#define GIMP_LIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_list_get_type(), GimpListClass)
     

#endif
