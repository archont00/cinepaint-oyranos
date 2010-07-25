#include <stdio.h>
#include "zoombookmark.h"
#include "scale.h"
#include "scroll.h"
#include "zoom.h"
#include "../lib/wire/iodebug.h"

ZoomBookmark zoom_bookmarks[ZOOM_BOOKMARK_NUM] =
{
   {0,0,101,FALSE},
   {0,0,101,FALSE},
   {0,0,101,FALSE},
   {0,0,101,FALSE},
   {0,0,101,FALSE}
};

static int zoom_bookmark_is_init = FALSE;

static void zoom_bookmark_check_init();
static void zoom_bookmark_save_doit(GtkWidget *button, gpointer data);
static void zoom_bookmark_load_doit(GtkWidget *button, gpointer data);


void zoom_bookmark_check_init()
{
   int i=0;

   if (!zoom_bookmark_is_init) {
      for (i = 0; i < ZOOM_BOOKMARK_NUM; i++) {
         zoom_bookmarks[i].image_offset_x = 0;
         zoom_bookmarks[i].image_offset_y = 0;
         zoom_bookmarks[i].zoom = 101;
	 zoom_bookmarks[i].is_set = FALSE;
      }

      zoom_bookmark_is_init = TRUE;
   }

}

void zoom_bookmark_jump_to(const ZoomBookmark* mark, GDisplay *gdisp)
{
   zoom_bookmark_check_init();

   change_scale(gdisp, mark->zoom);
   scroll_display(gdisp, mark->image_offset_x - gdisp->offset_x, 
		         mark->image_offset_y - gdisp->offset_y);
}

void zoom_bookmark_set(ZoomBookmark* mark, const GDisplay *gdisp)
{
   zoom_bookmark_check_init();

   mark->image_offset_x = gdisp->offset_x;
   mark->image_offset_y = gdisp->offset_y;
   mark->zoom = SCALEDEST(gdisp) * 100 + SCALESRC(gdisp);
   mark->is_set = TRUE;

   zoom_control_update_bookmark_ui(zoom_control);
}

int zoom_bookmark_save(const char *filename)
{
   int i=0;
   FILE *file = 0;
   zoom_bookmark_check_init();

   file = fopen(filename, "wt");
   if (!file) {
      d_printf("Could not open file %s\n", filename);
      return 0;
   }

   fprintf(file, "%d\n", ZOOM_BOOKMARK_NUM);
   for (i=0; i < ZOOM_BOOKMARK_NUM; i++) {
      fprintf(file, "%d %d %d %d\n", zoom_bookmarks[i].zoom, zoom_bookmarks[i].image_offset_x,
		      zoom_bookmarks[i].image_offset_y, zoom_bookmarks[i].is_set);
   }

   fclose(file);

   //make sure the ui reflects the new bookmarks
   return 1;
}

int zoom_bookmark_load(const char *filename)
{
   int i=0,num=0;
   FILE *file =0;
   zoom_bookmark_check_init();

   file = fopen(filename, "rt");
   if (!file) {
      d_printf("Could not open file %s\n", filename);
      return 0;
   }

   fscanf(file, "%d\n", &num);
   if (ZOOM_BOOKMARK_NUM < num)
      num = ZOOM_BOOKMARK_NUM;

   for (i=0; i < num; i++) {
      fscanf(file, "%d %d %d %d\n", &zoom_bookmarks[i].zoom, &zoom_bookmarks[i].image_offset_x,
		      &zoom_bookmarks[i].image_offset_y, &zoom_bookmarks[i].is_set);
   }

   fclose(file);
   zoom_control_update_bookmark_ui(zoom_control);
   return 1;
}

void zoom_bookmark_save_requestor()
{
   GtkWidget *file_selector;
   file_selector = gtk_file_selection_new("Save the current bookmarks");

   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			"clicked", GTK_SIGNAL_FUNC (zoom_bookmark_save_doit), 
			(gpointer) file_selector);

   /* Ensure that the dialog box is destroyed when the user clicks a button. */
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
                        "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        (gpointer) file_selector);

   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
                        "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        (gpointer) file_selector);

   /* Display that dialog */
   gtk_widget_show (file_selector);
}

void zoom_bookmark_load_requestor()
{
   GtkWidget *file_selector;
   file_selector = gtk_file_selection_new("Select a bookmark file to load");

   gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			"clicked", GTK_SIGNAL_FUNC (zoom_bookmark_load_doit), 
			(gpointer) file_selector);

   /* Ensure that the dialog box is destroyed when the user clicks a button. */
   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
                        "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        (gpointer) file_selector);

   gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
                        "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        (gpointer) file_selector);

   /* Display that dialog */
   gtk_widget_show (file_selector);
}

void zoom_bookmark_load_doit(GtkWidget *button, gpointer data)
{
   GtkFileSelection *sel;
   sel = (GtkFileSelection *)data;
   zoom_bookmark_load(gtk_file_selection_get_filename (GTK_FILE_SELECTION(sel)));
}

void zoom_bookmark_save_doit(GtkWidget *button, gpointer data)
{
   GtkFileSelection *sel;
   sel = (GtkFileSelection *)data;
   zoom_bookmark_save(gtk_file_selection_get_filename (GTK_FILE_SELECTION(sel)));
}


