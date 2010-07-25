#ifndef COMPAT_H
#define COMPAT_H

#include <gtk/gtk.h>

#if GTK_MAJOR_VERSION < 2

#ifndef GTK_OBJECT_GET_CLASS
#define GTK_OBJECT_GET_CLASS(obj_)   GTK_OBJECT(obj_)->klass
#endif
#ifndef G_OBJECT_GET_CLASS
#define G_OBJECT_GET_CLASS GTK_OBJECT_GET_CLASS
#endif

#define gdk_style_get_font(s_)        s_->font
#define gdk_style_set_font(s_,f_)    (s_->font = f_)
#define gtk_style_get_font(s_)        gdk_style_get_font(s_)
#define gtk_style_set_font(s_,f_)     gdk_style_set_font(s_,f_)
#define GTK_CLASS_TYPE(class_)        class_->type
#define G_TYPE_FROM_CLASS GTK_CLASS_TYPE

#endif

#endif /* COMPAT_H */
