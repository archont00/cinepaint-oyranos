#ifndef __GIMP_MINIMIZE_H__
#define __GIMP_MINIMIZE_H__

#include <gtk/gtkwidget.h>

/* These functions set up the GIMP so that when the toolbox is 
 * minimized, the registered windows will also be minimized.  
 * Currently, when the toobox is un-minimzed, the other windows
 * are *not* un-minimized as well.  
 * This may or may not be desirable UI depending on the user's preferences.
 * Currently, this depends on X11 calls, which is ok for R&H's purposes - jcohen */

/* Regsiter a new widget to be minimized if the toolbox is minimized */
void minimize_register(GtkWidget *widget);

/* Force all registered windows to be minimized */
void minimize_all();

/* Register the toolbox widget (or any other widget for that matter) so
 * that when it is minimized, all other registered windows will be minimized as well. */
void minimize_track_toolbox(GtkWidget *toolbox);

#endif

