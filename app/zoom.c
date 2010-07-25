#include <stdlib.h>
#include <float.h>
#include <string.h>
#include "scale.h"
#include "zoom.h"
#include "stdio.h"
#include "scroll.h"
#include "zoombookmark.h"
#include "layout.h"
#include "rc.h"
#include "minimize.h"

/************************************************************/
/*     Internal Function Declarations                       */
/************************************************************/
static void zoom_control_close(GtkObject *wid, gpointer data);
static void zoom_update_pulldown_slider(GDisplay *disp);
static void zoom_update_slider(GDisplay *disp);
static void zoom_update_pulldown(GDisplay *disp);
static void zoom_update_scale(GDisplay *gdisp, gint scale_val);
static void zoom_update_extents(GDisplay *gdisp);
static GDisplay *zoom_get_active_display();
static int zoom_control_activate();
static void zoom_slider_value_changed(
   GtkAdjustment *adjustment,
   gpointer user_data);
static void zoom_pulldown_value_changed(
   GtkList *list,
   GtkWidget *widget,
   gpointer user_data);
/*
static gboolean zoom_control_button_expose(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data);
*/
static gboolean zoom_preview_expose_event(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data);
static gboolean zoom_preview_configure_event(
   GtkWidget *widget,
   GdkEventConfigure *event,
   gpointer user_data); 
static gboolean zoom_preview_motion_notify_event(
   GtkWidget *widget,
   GdkEventMotion *event,
   gpointer user_data);
static gboolean zoom_preview_button_press_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data);
static gboolean zoom_preview_button_release_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data);
static void zoom_preview_draw();
static void zoom_preview_clear_pixmap();
static void zoom_preview_render_image();
static void zoom_preview_draw_extents();
static void zoom_preview_jump_to(gfloat x, gfloat y);
static void zoom_control_set_event(GtkButton *, gpointer user_data);
static void zoom_control_jump_event(GtkButton *, gpointer user_data);
static void zoom_enable_button(GtkButton *b);
static void zoom_disable_button(GtkButton *b);
static void zoom_highlight_current_bookmark();

/************************************************************/
/*     Global variables (yikes!)                            */
/************************************************************/
static int zoom_external_generated = 0;
static int zoom_updating_ui = 0;
static int zoom_updating_scale = 0;
ZoomControl * zoom_control = 0;
static GDisplay *zoom_gdisp = 0;

/************************************************************/
/*     Externally called functions (Notifications)          */
/************************************************************/

void zoom_control_update_bookmark_ui(ZoomControl *zoom)
{
   int i=0;

   if (!zoom)
      return;

   for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
      if (zoom->bookmark_button[i]) {
         if (zoom_bookmarks[i].is_set) {
            zoom_enable_button(GTK_BUTTON(zoom->bookmark_button[i]));
         }
         else {
            zoom_disable_button(GTK_BUTTON(zoom->bookmark_button[i]));
         }
      }
   }

   zoom_highlight_current_bookmark();
}

void zoom_view_changed(GDisplay *disp)
{
   // ignore this event if we are updating scale, because that indicates
   // that we caused the event to occur.
   if (zoom_updating_scale)
      return;

   // first get the active display if there is none
   if (!zoom_control || !zoom_control->gdisp || zoom_control->gdisp != disp) {
      return;
   }

   // indicate this event was generated externally
   zoom_external_generated = 1;
   
   // notify dialog's widgets to update zoom
   zoom_update_pulldown_slider(disp); 
   // notify drawing area to update rect.
   zoom_update_extents(disp);

   zoom_highlight_current_bookmark();
   
   // reset state
   zoom_external_generated = 0;
}

void zoom_image_preview_changed(GImage *image)
{
   // need to redraw, since the image we are previewing has changed
   zoom_preview_draw();
}

void zoom_set_focus(GDisplay *gdisp)
{
   GDisplay *old_disp;
   old_disp = zoom_gdisp;
   zoom_gdisp = gdisp;
   if (zoom_control && zoom_gdisp && zoom_gdisp != old_disp) {
      zoom_control_activate();
   }
}

void zoom_control_close(GtkObject *wid, gpointer data)
{
   if (zoom_control) {
      zoom_control_delete(zoom_control);
      zoom_control = 0;
   }
}

ZoomControl * zoom_control_open()
{
   if (zoom_control && GTK_WIDGET_VISIBLE(zoom_control->window)) {
      gdk_window_raise(zoom_control->window->window);
      return zoom_control;
   }

   // delete it if it exists
   if (zoom_control) {
      zoom_control_delete(zoom_control);
      zoom_control = 0;
   }

   // make a new one
   zoom_control = zoom_control_new();
   zoom_control_activate();
   
   // display it
   gtk_widget_show_all(zoom_control->window);
   return zoom_control;
}

ZoomControl * zoom_control_new()
{
  GtkWidget * vbox;
  ZoomControl *zoom;
  GList *items = NULL;
  GtkWidget *table;
  GtkWidget *button;
  gchar buff[16];
  int i=0;

  zoom = (ZoomControl *)malloc(sizeof(ZoomControl));
  zoom->gdisp = NULL;
  zoom->pixmap = NULL;
  zoom->window = NULL;  
  zoom->pull_down = NULL;
  zoom->slider = NULL;
  zoom->preview = NULL;
  zoom->adjust = NULL;  
  zoom->map_w = zoom->map_h = 0;
  zoom->top = zoom->bottom = zoom->left = zoom->right = 0;
  zoom->preview_width = zoom->preview_height = zoom->preview_x_offset = zoom->preview_y_offset = 0;
  zoom->mouse_capture = 0;
  for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
     zoom->bookmark_button[i] = 0;
  }

  zoom->window = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(zoom->window), "destroy", 
                  GTK_SIGNAL_FUNC(zoom_control_close), 0);
  gtk_window_set_policy (GTK_WINDOW (zoom->window), FALSE, FALSE, FALSE);
  gtk_window_set_title (GTK_WINDOW (zoom->window), "Zoom/Pan Control");
  gtk_widget_set_uposition(zoom->window, zoom_window_x, zoom_window_y);
  layout_connect_window_position(zoom->window, &zoom_window_x, &zoom_window_y, TRUE);
  layout_connect_window_visible(zoom->window, &zoom_window_visible);
  minimize_register(zoom->window);

  gtk_object_sink(GTK_OBJECT(zoom->window));
  gtk_object_ref(GTK_OBJECT(zoom->window));

  zoom->pull_down = gtk_combo_new();

  items = g_list_append (items, "1:16");
  items = g_list_append (items, "1:15");
  items = g_list_append (items, "1:14");
  items = g_list_append (items, "1:13");
  items = g_list_append (items, "1:12");
  items = g_list_append (items, "1:11");
  items = g_list_append (items, "1:10");
  items = g_list_append (items, "1:9");
  items = g_list_append (items, "1:8");
  items = g_list_append (items, "1:7");
  items = g_list_append (items, "1:6");
  items = g_list_append (items, "1:5");
  items = g_list_append (items, "1:4");
  items = g_list_append (items, "1:3");
  items = g_list_append (items, "1:2");
  items = g_list_append (items, "1:1");
  items = g_list_append (items, "2:1");
  items = g_list_append (items, "3:1");
  items = g_list_append (items, "4:1");
  items = g_list_append (items, "5:1");
  items = g_list_append (items, "6:1");
  items = g_list_append (items, "7:1");
  items = g_list_append (items, "8:1");
  items = g_list_append (items, "9:1");
  items = g_list_append (items, "10:1");
  items = g_list_append (items, "11:1");
  items = g_list_append (items, "12:1");
  items = g_list_append (items, "13:1");
  items = g_list_append (items, "14:1");
  items = g_list_append (items, "15:1");
  items = g_list_append (items, "16:1");
  gtk_combo_set_popdown_strings (GTK_COMBO (zoom->pull_down), items);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(zoom->pull_down)->list), "select-child", 
                  GTK_SIGNAL_FUNC(zoom_pulldown_value_changed), 0);

  zoom->adjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, -16, 17, -1, 1, 1));
  zoom->slider = gtk_hscrollbar_new(zoom->adjust);
  gtk_signal_connect(GTK_OBJECT(zoom->adjust), "value-changed", 
                  GTK_SIGNAL_FUNC(zoom_slider_value_changed), 0);

  zoom->preview = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(zoom->preview), 100, 100);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "expose-event",
		      (GtkSignalFunc) zoom_preview_expose_event, NULL);
  gtk_signal_connect (GTK_OBJECT(zoom->preview),"configure-event",
		      (GtkSignalFunc) zoom_preview_configure_event, NULL);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "motion-notify-event",
		      (GtkSignalFunc) zoom_preview_motion_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "button-press-event",
		      (GtkSignalFunc) zoom_preview_button_press_event, NULL);
  gtk_signal_connect (GTK_OBJECT (zoom->preview), "button-release-event",
		      (GtkSignalFunc) zoom_preview_button_release_event, NULL);
  gtk_widget_set_events(zoom->preview, GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | 
                                       GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_RELEASE_MASK);

  table = gtk_table_new(2, ZOOM_BOOKMARK_NUM, TRUE);
  for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
     button = gtk_button_new_with_label("Set");
     gtk_table_attach(GTK_TABLE(table), button, i, i+1, 0, 1, 
		     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 1);
     gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) zoom_control_set_event, (gpointer)i);
     gtk_widget_show(button);

     g_snprintf(buff, 16, "%d", i + 1); 
     button = gtk_button_new_with_label(buff);
     gtk_table_attach(GTK_TABLE(table), button, i, i+1, 1, 2, 
		     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 1);
//     gtk_signal_connect(GTK_OBJECT (button), "expose-event",
//		      (GtkSignalFunc) zoom_control_button_expose, (gpointer)i);
     gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) zoom_control_jump_event, (gpointer)i);
     zoom->bookmark_button[i] = button;
     gtk_widget_show(button);
  }

  vbox = gtk_vbox_new(FALSE, 10); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->action_area), vbox, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox), zoom->pull_down, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox), zoom->slider   , TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox), table          , TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(zoom->window)->vbox), zoom->preview, TRUE, TRUE, 0); 

  // make sure the bookmark buttons reflect the bookmark state
  zoom_control_update_bookmark_ui(zoom);

  gtk_widget_show(zoom->pull_down);
  gtk_widget_show(zoom->slider);
  gtk_widget_show(zoom->preview);
  // don't show it, it is up to the caller to manage visibility state information.

  return zoom;
}

void zoom_control_delete(ZoomControl *zoom)
{
   gtk_object_unref(GTK_OBJECT(zoom->window));
   free(zoom);
}

/************************************************************/
/*     Utility Functions                                    */
/************************************************************/
void zoom_highlight_current_bookmark()
{
   int i;
   gchar buff[16];


   if (!zoom_control || !zoom_control->gdisp)
      return;

   for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
      if (zoom_control->bookmark_button[i]) {
         if (zoom_bookmarks[i].is_set &&
             zoom_bookmarks[i].image_offset_x == zoom_control->gdisp->offset_x &&
             zoom_bookmarks[i].image_offset_y == zoom_control->gdisp->offset_y &&
             zoom_bookmarks[i].zoom == 
               (SCALEDEST(zoom_control->gdisp) * 100 + SCALESRC(zoom_control->gdisp))) {

            g_snprintf(buff, 16, "<%d>", i+1);
         }
         else {
            g_snprintf(buff, 16, "%d", i+1);
         }
         gtk_label_set_text(
            GTK_LABEL(GTK_BIN(zoom_control->bookmark_button[i])->child),buff);
      }
   }
}

void zoom_enable_button(GtkButton *b)
{
   gtk_widget_set_sensitive(GTK_WIDGET(b), TRUE);
//   gtk_button_set_relief(b, GTK_RELIEF_NORMAL);
}

void zoom_disable_button(GtkButton *b)
{
   gtk_widget_set_sensitive(GTK_WIDGET(b), FALSE);
   //gtk_button_set_relief(b, GTK_RELIEF_NONE);
}

void zoom_preview_jump_to(gfloat x, gfloat y)
{
   gfloat x_dist, y_dist;
   int xx, yy;
   gfloat scale;

   if (!zoom_control || !zoom_control->gdisp)
      return;

   
   // figure out the distance translated
   x_dist = zoom_control->drag_offset_x + x - .5 * (zoom_control->left + zoom_control->right);
   y_dist = zoom_control->drag_offset_y + y - .5 * (zoom_control->top + zoom_control->bottom);

   // convert from zoom control window to image units
   scale = ((float)zoom_control->gdisp->gimage->width) / ((float)zoom_control->preview_width);
  
   // convert from image units to screen units
   xx = (int) (x_dist * scale);
   yy = (int) (y_dist * scale);
   xx = SCALE(zoom_control->gdisp, xx);
   yy = SCALE(zoom_control->gdisp, yy);
   scroll_display(zoom_control->gdisp, xx, yy); 
}

void zoom_update_extents(GDisplay *gdisp)
{
   // recompute the visualized extents, redraw the offscreen pixmap, then force a redraw
   gint disp_scale, image_scale;
   float image_width, image_height;
   float disp_width, disp_height;
   float fleft, fright, ftop, fbottom;  // position of the view bounds normalized by the size of the image

   float preview_aratio;
   float image_aratio;

   disp_scale = SCALESRC(gdisp);
   image_scale = SCALEDEST(gdisp);
   image_width = image_scale * gdisp->gimage->width; 
   image_height = image_scale * gdisp->gimage->height; 
   disp_width = disp_scale * gdisp->disp_width;
   disp_height = disp_scale * gdisp->disp_height;

   ftop  = gdisp->offset_y <= 0 ? 0 : ((float)(gdisp->offset_y * disp_scale)) / image_height;
   fleft = gdisp->offset_x <= 0 ? 0 : ((float)(gdisp->offset_x * disp_scale)) / image_width;
   fright = (disp_width + disp_scale * gdisp->offset_x) / image_width;
   fbottom= (disp_height + disp_scale * gdisp->offset_y) / image_height;
	    
   if (fright > 1) fright = 1;
   if (fbottom > 1) fbottom = 1;

   // figure out the placement of the actual preview image in the preview window
   preview_aratio = ((float)zoom_control->map_w)/((float)zoom_control->map_h);
   image_aratio =   ((float)gdisp->gimage->width)/((float)gdisp->gimage->height);

   if (image_aratio < preview_aratio) {
      zoom_control->preview_height = zoom_control->map_h;
      zoom_control->preview_y_offset = 0;
      zoom_control->preview_width = (int) ((image_aratio / preview_aratio) * zoom_control->map_w);
      zoom_control->preview_x_offset = (zoom_control->map_w - zoom_control->preview_width) / 2;
   }
   else {
      zoom_control->preview_width = zoom_control->map_w;
      zoom_control->preview_x_offset = 0;
      zoom_control->preview_height = (int) ((preview_aratio / image_aratio) * zoom_control->map_h);
      zoom_control->preview_y_offset = (zoom_control->map_h - zoom_control->preview_height) / 2;
   }

   // compute coordinates of the bounding box in the preview window
   zoom_control->left  = (int) (zoom_control->preview_x_offset+ fleft * zoom_control->preview_width); 
   zoom_control->right = (int) (zoom_control->preview_x_offset+ fright * zoom_control->preview_width);
   zoom_control->top =   (int) (zoom_control->preview_y_offset+ ftop * zoom_control->preview_height); 
   zoom_control->bottom= (int) (zoom_control->preview_y_offset+ fbottom* zoom_control->preview_height); 

   // draw it
   zoom_preview_draw();
}

static void zoom_preview_draw()
{
   GdkRectangle rect;
   if (!zoom_control || !zoom_control->pixmap)
      return;
   
   zoom_preview_clear_pixmap();
   zoom_preview_render_image();
   zoom_preview_draw_extents();

   rect.x = 0;
   rect.y = 0;
   rect.width = zoom_control->map_w;
   rect.height= zoom_control->map_h;
   gtk_widget_draw(zoom_control->preview, &rect);
}

static void zoom_preview_clear_pixmap()
{
   if (!zoom_control || !zoom_control->pixmap)
      return;

   gdk_draw_rectangle(zoom_control->pixmap, zoom_control->preview->style->mid_gc[0], TRUE, 0, 0, 
		   zoom_control->map_w, zoom_control->map_h);
}

static void zoom_preview_render_image()
{
   if (!zoom_control || !zoom_control->pixmap)
      return;

   // for now, just draw a white rectangle.  Ideally, we'd actually draw a mini version of the
   // image, but that would be a serious pain to implement, and arguably isn't that important.
   gdk_draw_rectangle(zoom_control->pixmap, zoom_control->preview->style->white_gc, TRUE, 
		   zoom_control->preview_x_offset, zoom_control->preview_y_offset, 
		   zoom_control->preview_width, 
		   zoom_control->preview_height);
}

static void zoom_preview_draw_extents()
{
   if (!zoom_control || !zoom_control->pixmap)
      return;

   gdk_draw_line(zoom_control->pixmap, zoom_control->preview->style->black_gc, 
      zoom_control->left, zoom_control->top, zoom_control->left, zoom_control->bottom);
   gdk_draw_line(zoom_control->pixmap, zoom_control->preview->style->black_gc,
      zoom_control->left, zoom_control->bottom, zoom_control->right, zoom_control->bottom);
   gdk_draw_line(zoom_control->pixmap, zoom_control->preview->style->black_gc, 
      zoom_control->right, zoom_control->bottom, zoom_control->right, zoom_control->top);
   gdk_draw_line(zoom_control->pixmap, zoom_control->preview->style->black_gc,
      zoom_control->right, zoom_control->top, zoom_control->left, zoom_control->top);
}

void zoom_update_pulldown_slider(GDisplay *disp)
{
   zoom_update_pulldown(disp);
   zoom_update_slider(disp);
}

void zoom_update_slider(GDisplay *disp)
{
   gfloat val;
   gint src, dst;

   zoom_updating_ui = 1;

   // extract the scale from the display, update the slider's value accordingly
   src = SCALESRC(disp);
   dst = SCALEDEST(disp);

   if (src == 1) {
      val = (-dst + 1); 
   }
   else {
      val = src - 1;
   }

   gtk_adjustment_set_value(zoom_control->adjust, -val);
   zoom_updating_ui = 0;
}

void zoom_update_pulldown(GDisplay *disp)
{
   GtkList *list = 0;
   gint src, dst;
   gint item;

   zoom_updating_ui = 1;
   list = GTK_LIST(GTK_COMBO(zoom_control->pull_down)->list);
   
   // extract the scale from the display, update the pulldown's value accordingly
   src = SCALESRC(disp);
   dst = SCALEDEST(disp);

   if (src == 1) {
      item = 14 + dst; 
   }
   else {
      item = 16 - src;
   }

   gtk_list_unselect_all(list);
   gtk_list_select_item(list, item);
   zoom_updating_ui = 0;
}

void zoom_update_scale(GDisplay *gdisp, gint scale_val)
{
   zoom_updating_scale = 1;
   change_scale(zoom_control->gdisp, scale_val);
   zoom_updating_scale = 0;
}



static int 
zoom_control_activate()
{
   GDisplay *disp;

   if (!zoom_control)
      return 0;
   
   // first get the active display if there is none
   disp = zoom_get_active_display();
   if (disp != zoom_control->gdisp) {
      zoom_control->gdisp = disp;
      zoom_view_changed(disp);
   }

   // if that failed, do nothing.
   if (!zoom_control->gdisp) {
      return 0;
   }

   return 1;
}

// Returns which display the zoom control should refer to
static GDisplay *
zoom_get_active_display()
{
   // if there are no displays, return 0
   if (!display_list)
      return NULL;

   // if we think we have one, check to make sure its still valid
   // if it's not still valid, forget about it
   if (zoom_gdisp && g_slist_find(display_list, (gpointer)zoom_gdisp)) {
      return zoom_gdisp; 
   }
   else {
      zoom_gdisp = 0;
   }

   // otherwise there's nothing to do, so just return the first one we find.
   zoom_gdisp = (GDisplay *)display_list->data;
   return zoom_gdisp; 
}

/************************************************************/
/*     Callback functions for the zoom dialog               */
/************************************************************/

// called when the value of the zoom slider changes
static void 
zoom_slider_value_changed(
   GtkAdjustment *adjustment,
   gpointer user_data)
{
   int scale_val;

   if (zoom_updating_ui || zoom_external_generated || zoom_updating_scale)
      return;

   if (!zoom_control_activate())
      return;

   // convert to an integer value, and round
   scale_val = (int) (-adjustment->value + .5);

   // need to pack scale_val into the weird format used by change_scale().  
   scale_val = scale_val < 0 ? (-scale_val + 1) * 100 + 1 :
                               (scale_val + 1) + 100;

    // now set the zoom factor for the display
   zoom_update_scale(zoom_control->gdisp, scale_val);
   zoom_update_extents(zoom_control->gdisp);
   zoom_highlight_current_bookmark();
   zoom_update_pulldown(zoom_control->gdisp);
}

static void zoom_pulldown_value_changed(
   GtkList *list,
   GtkWidget *widget,
   gpointer user_data)
{
   int scale_val;
   int src_val;
   int dst_val;
   gchar src_buf[256];
   gchar dst_buf[256];
   gchar * text;

   if (zoom_updating_ui || zoom_external_generated || zoom_updating_scale)
      return;

   if (!widget)
      return;

   if (!zoom_control_activate())
      return;

   gtk_label_get(GTK_LABEL(GTK_BIN(widget)->child), &text);
   if (text[1] == ':') {
      dst_buf[0] = text[0];
      dst_buf[1] = '\0';
      strcpy(src_buf, text + 2);
   }
   else {
      dst_buf[0] = text[0];
      dst_buf[1] = text[1];
      dst_buf[2] = '\0';
      strcpy(src_buf, text + 3);
   }

   src_val = atoi(src_buf);
   dst_val = atoi(dst_buf);
   scale_val = dst_val * 100 + src_val;

   zoom_update_scale(zoom_control->gdisp, scale_val);
   zoom_update_extents(zoom_control->gdisp);
   zoom_highlight_current_bookmark();
   zoom_update_slider(zoom_control->gdisp);
}


/************************************************************/
/*     Callback functions for preview widget events         */
/************************************************************/

/*
static gboolean 
zoom_control_button_expose(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data)
{
   gint id;
   ZoomBookmark * bm;

   if (!zoom_control)
      return FALSE;

   id = (gint) user_data;
   bm = &zoom_bookmarks[id];

   if (bm->is_set) {
      gdk_draw_rectangle(widget->window,
		      widget->style->black_gc,
		      TRUE, 5, 5, 15, 15);
   }

   return FALSE;
}
*/

static gboolean zoom_preview_expose_event(
   GtkWidget *widget,
   GdkEventExpose *event,
   gpointer user_data)
{
   if (!zoom_control || !zoom_control->pixmap) 
      return FALSE;

   gdk_draw_pixmap(widget->window,
                   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                   zoom_control->pixmap,
                   event->area.x, event->area.y,
                   event->area.x, event->area.y,
                   event->area.width, event->area.height);
   return FALSE;
}

static gboolean zoom_preview_configure_event(
   GtkWidget *widget,
   GdkEventConfigure *event,
   gpointer user_data)
{
   if (!zoom_control)
      return FALSE;

   if (!zoom_control->pixmap) {
      // make one
      zoom_control->pixmap = 
	 gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
      zoom_control->map_w = widget->allocation.width;
      zoom_control->map_h = widget->allocation.height;
      zoom_preview_clear_pixmap();
      if (zoom_control->gdisp) {
         zoom_update_extents(zoom_control->gdisp);
      }
   }

   return TRUE;
}

static gboolean zoom_preview_motion_notify_event(
   GtkWidget *widget,
   GdkEventMotion *event,
   gpointer user_data)
{
   gint x,y;
   GdkModifierType state;

   if (!zoom_control || !zoom_control->gdisp)
      return FALSE;

   if (!zoom_control->mouse_capture)
      return FALSE;

   if (event->is_hint) {
      gdk_window_get_pointer (event->window, &x, &y, &state);
   }
   else {
      x = event->x;
      y = event->y;
   }

   zoom_preview_jump_to(x, y);
   return FALSE;
}

static gboolean zoom_preview_button_release_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data)
{
   if (!zoom_control || !zoom_control->gdisp)
      return FALSE;

   if (event->button == 1) {
      zoom_control->mouse_capture = 0;

   }
   return FALSE;
}


static gboolean zoom_preview_button_press_event(
   GtkWidget *widget,
   GdkEventButton *event,
   gpointer user_data)
{
   if (!zoom_control || !zoom_control->gdisp)
      return FALSE;

   if (event->button == 1) {
      zoom_control->mouse_capture = 1;

      // if the click was outside the box, jump there 
      if (event->x < zoom_control->left || event->x > zoom_control->right ||
	  event->y < zoom_control->top || event->y > zoom_control->bottom) {
	 zoom_control->drag_offset_x = zoom_control->drag_offset_y = 0;
         zoom_preview_jump_to(event->x, event->y);
      }
      else {
         zoom_control->drag_offset_x = (int) (((zoom_control->left + zoom_control->right) / 2) - event->x);
         zoom_control->drag_offset_y = (int) (((zoom_control->top  + zoom_control->bottom) / 2) - event->y);
      }

   }
   return FALSE;
}

static void zoom_control_set_event(GtkButton *button, gpointer user_data)
{
   int i;
   i = (int)user_data;

   if (!zoom_control || !zoom_control->gdisp)
      return;

   zoom_bookmark_set(&zoom_bookmarks[i], zoom_control->gdisp);
}

static void zoom_control_jump_event(GtkButton *button, gpointer user_data)
{
   int i;
   i = (int)user_data;

   if (!zoom_control || !zoom_control->gdisp)
      return;

   if (zoom_bookmarks[i].is_set)
      zoom_bookmark_jump_to(&zoom_bookmarks[i], zoom_control->gdisp);
}

