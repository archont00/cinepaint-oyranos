#include <gtk/gtk.h>

#ifdef DEBUG

#include <stdio.h>
#include "gtk_debug_helpers.h"


const char* const GDK_EVENT_TYPE_NAMES[] =
{
  "GDK_NOTHING",                /*  = -1  */
  "GDK_DELETE",                 /*  = 0   */
  "GDK_DESTROY",                /*  = 1   */
  "GDK_EXPOSE",                 /*  = 2   */
  "GDK_MOTION_NOTIFY",          /*  = 3   */
  "GDK_BUTTON_PRESS",           /*  = 4   */
  "GDK_2BUTTON_PRESS",          /*  = 5   */
  "GDK_3BUTTON_PRESS",          /*  = 6   */
  "GDK_BUTTON_RELEASE",         /*  = 7   */
  "GDK_KEY_PRESS",              /*  = 8   */
  "GDK_KEY_RELEASE",            /*  = 9   */
  "GDK_ENTER_NOTIFY",           /*  = 10  */
  "GDK_LEAVE_NOTIFY",           /*  = 11  */
  "GDK_FOCUS_CHANGE",           /*  = 12  */
  "GDK_CONFIGURE",              /*  = 13  */
  "GDK_MAP",                    /*  = 14  */
  "GDK_UNMAP",                  /*  = 15  */
  "GDK_PROPERTY_NOTIFY",        /*  = 16  */
  "GDK_SELECTION_CLEAR",        /*  = 17  */
  "GDK_SELECTION_REQUEST",      /*  = 18  */
  "GDK_SELECTION_NOTIFY",       /*  = 19  */
  "GDK_PROXIMITY_IN"            /*  = 20  */
  "GDK_PROXIMITY_OUT"           /*  = 21  */
  "GDK_DRAG_ENTER"              /*  = 22  */
  "GDK_DRAG_LEAVE"              /*  = 23  */
  "GDK_DRAG_MOTION"             /*  = 24  */
  "GDK_DRAG_STATUS"             /*  = 25  */
  "GDK_DROP_START"              /*  = 26  */
  "GDK_DROP_FINISHED"           /*  = 27  */
  "GDK_CLIENT_EVENT"            /*  = 28  */
  "GDK_VISIBILITY_NOTIFY"       /*  = 29  */
  "GDK_NO_EXPOSE"               /*  = 30  */
};

static const int num_gdk_event_type_names = 
  sizeof(GDK_EVENT_TYPE_NAMES) / sizeof(GDK_EVENT_TYPE_NAMES[0]);

  
const char* gdk_event_type_name (GdkEvent* e)
{
  if (!e) 
    return "(null)";
  
  if (-1 <= e->type && e->type <= num_gdk_event_type_names)
    return GDK_EVENT_TYPE_NAMES [e->type + 1];
  
  return "(event_type out of range)";
}
  

void print_gdk_event_type (GdkEvent* e)
{
  printf("\tGdkEvent: ");
  
  if (!e) 
    printf("(null)\n");
  else if (-1 <= e->type && e->type <= num_gdk_event_type_names)
    printf("#%d \"%s\"\n", e->type, GDK_EVENT_TYPE_NAMES [e->type + 1]);
  else
    printf("(#%d - out of range)\n", e->type);
}
 

/**
*  Report the content of a `GdkEventKey' structure (debug helper)
*  
*  In .../include/gtk-1.2/gdk/gdktypes.h:
*       struct _GdkEventKey {
*         GdkEventType type;
*         GdkWindow *window;
*         gint8 send_event;
*         guint32 time;
*         guint state;
*         guint keyval;
*         gint length;
*         gchar *string;
*       };
*/
void report_GdkEventKey (GdkEventKey* e)
{
  if (!e) {printf("\t(null)\n"); return;}
  printf("\ttype=%d, window=%p, send_event=%d, time=%d\n\tstate=%d, keyval=%d, length=%d, string=\"%s\"\n", 
    e->type, (void*)e->window, e->send_event, e->time, e->state, e->keyval, e->length, e->string);
  printf("\tstate & SHIFT_MASK   = %d\n", e->state & GDK_SHIFT_MASK); 
  printf("\tstate & LOCK_MASK    = %d\n", e->state & GDK_LOCK_MASK); 
  printf("\tstate & CONTROL_MASK = %d\n", e->state & GDK_CONTROL_MASK); 
  printf("\tstate & MOD1_MASK    = %d\n", e->state & GDK_MOD1_MASK); 
  printf("\tstate & MOD2_MASK    = %d\n", e->state & GDK_MOD2_MASK); 
  printf("\tstate & MOD3_MASK    = %d\n", e->state & GDK_MOD3_MASK); 
}


/**
*  Prints content of a GdkEventKey's supposed to be an arrow key
*/
void print_GdkEventKeyArrow (GdkEventKey* e)
{
  char s[64];  
  
  if (!e) {printf("(null)\n"); return;}

  *s = '\0';
  if (e->state & GDK_LOCK_MASK)    strcat(s,"Lock + ");
  if (e->state & GDK_SHIFT_MASK)   strcat(s,"Shift + ");
  if (e->state & GDK_CONTROL_MASK) strcat(s,"Ctrl + ");
  if (e->state & GDK_MOD1_MASK)    strcat(s,"Alt + ");
  if (e->state & GDK_MOD2_MASK)    strcat(s,"Mod2 + ");
  if (e->state & GDK_MOD3_MASK)    strcat(s,"Mod3 + ");
  printf("%s",s);
  
#if GTK_MAJOR_VERSION < 2
  switch (e->keyval)
  {
    case GDK_Left:  printf("Left\n");          break;
    case GDK_Right: printf("Right\n");         break;
    case GDK_Up:    printf("Up\n");            break;
    case GDK_Down:  printf("Down\n");          break;
    default:        printf("no arrow key\n");  break;
  }
#endif
}

#endif  /* ifdef DEBUG */

/* END OF FILE */
