#ifndef __GIMP_ZOOM_H__
#define __GIMP_ZOOM_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkadjustment.h>
//#include <gdk/gdk.h>
#include "gdisplay.h"
#include "gimage.h"
#include "zoombookmark.h"

/* This struct holds the information for
 * the zoom/pan widget.  
 * Unresolved:  what is the relationship between zoom controls
 * and displays? */

typedef struct 
{
  GtkWidget *window;        // GtkDialog
  GtkWidget *pull_down;     // GtkCombo
  GtkWidget *slider;        // GtkHScale
  GtkWidget *preview;       // GtkDrawingArea
  GtkAdjustment * adjust;  
  GDisplay  *gdisp;         
  GdkPixmap *pixmap;
  gint       map_w, map_h;
  gint       top, bottom, left, right; // coordinates of the extents in the the preview widget
  gint preview_width; // coordinates of the preview image inside the actual preview widget
  gint preview_height;
  gint preview_x_offset;
  gint preview_y_offset;
  gint mouse_capture; // whether the preview widget is between mouse button 1 down and up.
  gint drag_offset_x; // intitial offset from the center of the extent box during a mouse drag
  gint drag_offset_y;
  GtkWidget *bookmark_button[ZOOM_BOOKMARK_NUM];

} ZoomControl;

ZoomControl * zoom_control_open();
ZoomControl * zoom_control_new();
void zoom_control_delete(ZoomControl *zoom);

// call this to notify the dialog that the actual pan/zoom parameters from the given
// display have changed.  The dialog will determine whether or not it cares (i.e., 
// whether it is currently tracking that view, although it always should be traching
// the display that had the most recent event).
void zoom_view_changed(GDisplay *disp);

void zoom_set_focus(GDisplay *gdisp);
void zoom_image_preview_changed(GImage *image);

void zoom_control_update_bookmark_ui(ZoomControl *zoom);

extern ZoomControl * zoom_control;
#endif
