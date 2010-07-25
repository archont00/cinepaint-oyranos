#ifndef __GIMPSIGNAL_H__
#define __GIMPSIGNAL_H__

#include <gtk/gtksignal.h>

/* This is the gtk "signal id" */
typedef guint GimpSignalID;

		
typedef const struct GimpSignalType GimpSignalType;
/* The arguments are encoded in the names.. */

extern GimpSignalType* const gimp_sigtype_void;
typedef void (*GimpHandlerVoid)(GtkObject*, gpointer);

extern GimpSignalType* const gimp_sigtype_pointer;
typedef void (*GimpHandlerPointer)(GtkObject*, gpointer, gpointer);

extern GimpSignalType* const gimp_sigtype_int;
typedef void (*GimpHandlerInt)(GtkObject*, gint, gpointer);

extern GimpSignalType* const gimp_sigtype_int_int_int_int;
typedef void (*GimpHandlerIntIntIntInt) (GtkObject*, gint, gint, gint, gint,
					 gpointer);

GimpSignalID gimp_signal_new(const gchar* name,
			     GtkSignalRunType signal_flags,
			     GtkType object_type,
			     guint function_offset,
			     GimpSignalType* sig_type);


			     

#endif
