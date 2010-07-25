#ifndef gtk_debug_helpers_h
#define gtk_debug_helpers_h

#ifdef DEBUG

#include <gdk/gdkkeysyms.h>
#include <gdk/gdktypes.h>

extern const char* const GDK_EVENT_TYPE_NAMES[];

const char* gdk_event_type_name  (GdkEvent *);
void        print_gdk_event_type (GdkEvent *);

void  report_GdkEventKey     (GdkEventKey *);
void  print_GdkEventKeyArrow (GdkEventKey *);

#endif  /* DEBUG */
#endif  /* gtk_debug_helpers_h */
