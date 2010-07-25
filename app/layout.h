#ifndef __GIMP_LAYOUT_H__
#define __GIMP_LAYOUT_H__

#include <gtk/gtkwidget.h>

// Call these to bind a window with resource state variables.
// (Note: the compute_offet parameter is currently ignored)
void layout_connect_window_position(GtkWidget *widget, int *x_var, int *y_var, int compute_offset);
void layout_connect_window_visible(GtkWidget *widget, int *visible);

// When the layout is "frozen" all state changes to the layout are ignored until
// it is "unfrozen."  This is useful, for example, for noticing which windows are open
// exactly when the user selects quit, so that when the shutdown code destroys windows,
// these changes are ignored.
void layout_freeze_current_layout();
void layout_unfreeze_current_layout();

// call after the gimprc file has been loaded.  Restores the layout to the state
// recorded in the resource variables.
void layout_restore(); 

// Call this to write out the current state of the resource variables to the gimprc file
int  layout_save();

#endif

