#ifndef __GIMP_OBJECT_P_H__
#define __GIMP_OBJECT_P_H__

#include <gtk/gtk.h>
#include <gtk/gtkobject.h>
#include "object.h"

struct GimpObject
{
#if GTK_MAJOR_VERSION > 1
  GtkObject  parent_instance;
  gchar   *name;

  /*<  private  >*/
  gchar   *normalized;
#else
	GtkObject object;
#endif
};

typedef struct
{
#if GTK_MAJOR_VERSION > 1
  GtkObjectClass  parent_class;

  /*  signals  */
  void    (* disconnect)   (GimpObject *object);
  void    (* name_changed) (GimpObject *object);

  /*  virtual functions  */
  gint64  (* get_memsize)  (GimpObject *object,
                            gint64     *gui_size);
#else
	GtkObjectClass parent_class;
#endif
} GimpObjectClass;

#define GIMP_OBJECT_CLASS(klass) \
GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_OBJECT, GimpObjectClass)

#if GTK_MAJOR_VERSION < 2
#define GIMP_TYPE_INIT(typevar, obtype, classtype, obinit, classinit, parent) \
if(!typevar){ \
	GtkTypeInfo _info={#obtype, \
			   sizeof(obtype), \
			   sizeof(classtype), \
			   (GtkClassInitFunc)classinit, \
			   (GtkObjectInitFunc)obinit, \
			   NULL, NULL} ; \
	typevar=gtk_type_unique(parent, &_info); \
}
#endif
#endif /* __GIMP_OBJECT_P_H__ */
