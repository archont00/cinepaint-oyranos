/* app/spline.c
   Adds Basic spline support to CinePaint.
   Copyright (c) 2004 Tom Huffman <huff8795@sbcglobal.net>
   License MIT (http://opensource.org/licenses/mit-license.php)
*/

/* Changes:
 * Versions 1&2 were internal unreleased proof of concepts.  They resulted in a total rewrite
 * which is Version 3 the first public release.
*/
/* Version 3 
 *    * First Public Release
*/


#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "brushlist.h"
#include "gdisplay.h"
#include "interface.h"
#include "layer_pvt.h"
#include "paintbrush.h"
#include "paint_core_16.h"
#include "pixel_region.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "rc.h"
#include "rect_select.h"
#include "spline.h"

#define WIDTH     8
#define HALFWIDTH 4
#define SUBDIV    250

typedef struct int_point int_point;
struct int_point
{
  int x, y;
};

void spline_button_press(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr);
void spline_button_release(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr);
void spline_motion(Tool *tool, GdkEventMotion *mevent, void *gdisp_ptr);
void spline_edit_button_press(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr);
void spline_edit_button_release(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr);
void spline_edit_motion(Tool *tool, GdkEventMotion *mevent, void *gdisp_ptr);
void spline_cursor_update(Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr);
void spline_edit_cursor_update(Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr);
void spline_draw(Tool *tool);
void spline_draw_linear(Tool *tool);
void spline_draw_ellipse(Tool *tool);
void spline_draw_curve(Tool *tool);
void spline_draw_points(Tool *tool);
void spline_stroke(Tool *tool);
void spline_stroke_linear(Tool *tool);
void spline_stroke_ellipse(Tool *tool);
void spline_stroke_curve(Tool *tool);
void spline_paint_core_init(PaintCore *paint_core);
int spline_create_options(void);
int spline_number_points(Spline *spline);
int spline_draw_curve_section (GDisplay *display, SplineTool *spline_tool);
int spline_draw_curve_controls(GDisplay *display, SplineTool *spline_tool);
int spline_point_linear_int(int_point *dest, int_point *a, int_point *b, float t);
int spline_calc_cubic_curve_point(int_point *dest, int_point a, int_point b,
				  int_point c, int_point d, float t);

extern PaintCore spline_paint_core;
extern GSList *toolbox_table_group;
extern GtkWidget *toolbox_shell;
extern Tool *active_tool;
extern GSList *display_list;

Tool*
tools_new_spline()
{
  Tool *tool;
  Layer *layer;
  Spline *spline;
  GDisplay *display;
  GSList* list;
  GdkColor fg, bg;

  tool=(Tool*)malloc(sizeof(Tool));
  tool->type=SPLINE;
  tool->state=INACTIVE;
  tool->scroll_lock=1;   /* Do not allow scrolling */
  tool->auto_snap_to=TRUE;
  tool->button_press_func=spline_button_press;
  tool->button_release_func=spline_button_release;
  tool->motion_func=spline_motion;
  tool->arrow_keys_func=standard_arrow_keys_func;
  tool->cursor_update_func=spline_cursor_update;
  tool->control_func=spline_control;
  tool->preserve=TRUE;
  tool->private=NULL;

  /* This is meant to draw the spline onto the GdkWindow when the tool is started
     so that we don't have to wait until an event occures to see the spline */
  list=display_list;
  while (list)
    {
      display=(GDisplay*)list->data;
      layer=display->gimage->active_layer;
      spline=((SplineTool*)layer->sl.spline_tool)->active_spline;
      if(!spline && !DL_is_empty(layer->sl.splines))
	{
	  spline=(Spline*)layer->sl.splines->head;
	  ((SplineTool*)layer->sl.spline_tool)->active_spline=spline;
	  ((SplineTool*)layer->sl.spline_tool)->active_point=(SplinePoint*)spline->points->head;
	  spline->drawn=0;
	  spline->window=display->canvas->window;
	  spline->gc=gdk_gc_new(spline->window);
	  gdk_gc_set_function(spline->gc, GDK_INVERT);
	  fg.pixel=0xFFFFFFFF;
	  bg.pixel=0x00000000;
	  gdk_gc_set_foreground(spline->gc, &fg);
	  gdk_gc_set_background(spline->gc, &bg);
	  gdk_gc_set_line_attributes(spline->gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
      tool->gdisp_ptr=display;
      tool->state=ACTIVE;
      tool->private=display->gimage->active_layer->sl.spline_tool;
      if(spline && !spline->drawn)
	spline_control(tool, RESUME, NULL);
      list=g_slist_next(list);
    }

  /* create spline tool options dialog */
  /*  spline_create_options(); */
  return tool;
}

Tool*
tools_new_spline_edit()
{
  Tool *tool=NULL;
  Spline *spline;
  GDisplay *display;
  GSList* list;

  tool=(Tool*)malloc(sizeof(Tool));
  tool->type=SPLINE;
  tool->state=INACTIVE;
  tool->scroll_lock=1;   /* Do not allow scrolling */
  tool->auto_snap_to=TRUE;
  tool->button_press_func=spline_edit_button_press;
  tool->button_release_func=spline_edit_button_release;
  tool->motion_func=spline_edit_motion;
  tool->arrow_keys_func=standard_arrow_keys_func;
  tool->cursor_update_func=spline_edit_cursor_update;
  tool->control_func=spline_control;
  tool->preserve=TRUE;
  tool->private=NULL;

  /* This is meant to draw the spline onto the GdkWindow when the tool is started
     so that we don't have to wait until an event occures to see the spline */
  list=display_list;
  while (list)
    {
      display=(GDisplay*)list->data;
      spline=((SplineTool*)display->gimage->active_layer->sl.spline_tool)->active_spline;
      tool->gdisp_ptr=display;
      tool->state=ACTIVE;
      tool->private=display->gimage->active_layer->sl.spline_tool;
      if(spline && !spline->drawn)
	spline_control(tool, RESUME, NULL);
      list=g_slist_next(list);
    }

  return tool;
}

void
tools_free_spline(Tool *tool)
{
  spline_control(tool, HALT, NULL);
  tool->private=NULL;
}

void
tools_free_spline_edit(Tool *tool)
{
}

/* Allocate and fill in a new Spline struct */
Spline*
spline_new_spline(int continuity, int keyframe, int active, int type, GdkWindow *window)
{
  Spline *spline;
  GdkColor fg, bg;

  spline=(Spline*)malloc(sizeof(Spline));
  spline->node=(DL_node*)malloc(sizeof(DL_node));
  DL_init((DL_node*)spline->node);
  spline->points=(DL_list*)malloc(sizeof(DL_list));
  DL_init((DL_node*)spline->points);
  spline->window=window;
  spline->gc=gdk_gc_new(window);
  spline->drawn=0;
  spline->active=active;
  spline->keyframe=keyframe;
  spline->continuity=continuity;
  spline->type=type;
  spline->draw_points=1;
  gdk_gc_set_function(spline->gc, GDK_INVERT);
  fg.pixel=0xFFFFFFFF;
  bg.pixel=0x00000000;
  gdk_gc_set_foreground(spline->gc, &fg);
  gdk_gc_set_background(spline->gc, &bg);
  gdk_gc_set_line_attributes(spline->gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  return spline;
}

/* returns the active spline of a layer,
   returns NULL if no active splines */
Spline* 
spline_get_active_spline(Layer *layer)
{
  Spline *spline;
  spline=(Spline*)layer->sl.splines->head;
  do
    {
      if(!spline->active)
	return spline;
      spline=(Spline*)(((DL_node*)spline)->next);
    } while(spline);
  return NULL;
}

/* callback function for draw_core button press event */
void
spline_button_press(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr)
{
  GDisplay *display;
  SplineTool *spline_tool;
  SplinePoint *point;
  int x, y;

  display=(GDisplay*)gdisp_ptr;

  /* If the tool was being used in another image reset it. */
  if(display!=tool->gdisp_ptr) 
    {
      tool->state=INACTIVE;
      tool->gdisp_ptr=(GDisplay*)display;
      tool->state=ACTIVE;
      spline_tool=tool->private=display->gimage->active_layer->sl.spline_tool;
      spline_control(tool, RESUME, NULL);
    }
  else
    spline_tool=tool->private=display->gimage->active_layer->sl.spline_tool;

  if(!spline_tool->active_spline)
    {
      spline_tool->active_spline=spline_new_spline(1, 0, 0, -1, display->canvas->window);
      DL_append(display->gimage->active_layer->sl.splines, (DL_node*)spline_tool->active_spline);
    }

  gdisplay_untransform_coords (display, bevent->x, bevent->y, &x, &y, TRUE, 0);
  /* check if initial point.  if so, then set type of spline.  */
  if(spline_tool->active_spline->type==-1)
    {
      tool->state=ACTIVE;
      tool->gdisp_ptr=(GDisplay*)display;
      if((bevent->state & GDK_CONTROL_MASK)&&(bevent->state & GDK_SHIFT_MASK))
	return;
      else if(bevent->state & GDK_CONTROL_MASK)
	spline_tool->active_spline->type=0;
      else if(bevent->state & GDK_SHIFT_MASK)
	spline_tool->active_spline->type=2;
      else
	spline_tool->active_spline->type=1;
      point=spline_new_splinepoint(x, y);
      DL_append(spline_tool->active_spline->points, (DL_node*)point);
      spline_tool->active_point=point;
      spline_tool->click_on_point=2;
      display->gimage->dirty++;
      spline_control(tool, RESUME, NULL);
    }
  else if(bevent->state & GDK_CONTROL_MASK && 
	  spline_point_check(display, spline_tool->active_spline->points, 
			     bevent->x, bevent->y, 0))
    {
      /* clicked on an anchor point.  Assume we want to modify the curve.  Set the
	 active point to the clicked on point. */
      spline_tool->active_point=spline_point_check(display, spline_tool->active_spline->points,  
						   bevent->x, bevent->y, 0);
      spline_tool->click_on_point=3;
    }
  /* Shift-Click an anchor deletes clicked on point. */
  else if(bevent->state & GDK_SHIFT_MASK && 
	  spline_point_check(display, spline_tool->active_spline->points, 
			     bevent->x, bevent->y, 0))
    {
      spline_control(tool, PAUSE, display);
      point=spline_point_check(display, spline_tool->active_spline->points,  
			       bevent->x, bevent->y, 0);
      DL_remove(spline_tool->active_spline->points, (DL_node*)point);
      spline_tool->click_on_point=0;
      spline_control(tool, RESUME, display);
    }
  else if(spline_point_check(display, spline_tool->active_spline->points, 
			     bevent->x, bevent->y, 0))
    {
      spline_tool->active_point=spline_point_check(display, spline_tool->active_spline->points,  
						   bevent->x, bevent->y, 0);
      spline_tool->click_on_point=1;
    }
  else if(spline_point_check(display, spline_tool->active_spline->points, 
			     bevent->x, bevent->y, 1))
    {
      /* clicked on a control point 1.  Assume we want to modify the curve.  Set the
	 active point to the clicked on point. */
      spline_tool->active_point=spline_point_check(display, spline_tool->active_spline->points, 
						   bevent->x, bevent->y, 1);
      spline_tool->click_on_point=4;
    }
  else if(spline_point_check(display, spline_tool->active_spline->points, 
			     bevent->x, bevent->y, 2))
    {
      /* clicked on a control point 2.  Assume we want to modify the curve.  Set the
	 active point to the clicked on point. */
      spline_tool->active_point=spline_point_check(display, spline_tool->active_spline->points, 
						   bevent->x, bevent->y, 2);
      spline_tool->click_on_point=5;
    }
  else if((bevent->state & GDK_CONTROL_MASK) || (bevent->state & GDK_SHIFT_MASK)) 
    {
      return;
    }
  else
    {
      /* ellipse splines can only have two points.  If we already have two
	 then ignore button presses */
      if(spline_tool->active_spline->type==2)
	{
	  if(spline_number_points(spline_tool->active_spline)>=2)
	    {
	      spline_tool->motion_active=-1;
	      return;
	    }
	}
      spline_control(tool, PAUSE, NULL);
      spline_tool->click_on_point=1;
      point=spline_new_splinepoint(x, y);
      DL_append(spline_tool->active_spline->points, (DL_node*)point);
      spline_tool->active_point=point;
      spline_control(tool, RESUME, NULL);
    }
}

/* callback function for draw_core motion events */
void
spline_motion(Tool *tool, GdkEventMotion *mevent, void * gdisp_ptr)
{
  GDisplay *display;
  SplineTool *spline_tool=(SplineTool*)tool->private;
  SplinePoint *temp;
  int x, y, oldx, oldy;

  if(tool->state!=ACTIVE)
      return;

  display=gdisp_ptr;
  if(!display)
      return;

  if(!spline_tool)
      return;

  if(!spline_tool->active_point)
      return;

  if(spline_tool->motion_active==-1)
      return;

  spline_control(tool, PAUSE, NULL);

  gdisplay_untransform_coords (display, mevent->x, mevent->y, &x, &y, TRUE, 0);
  switch(spline_tool->active_spline->type)
    {
    case 0:			/* linear */
      if(spline_tool->click_on_point && (mevent->state & GDK_CONTROL_MASK))
	{
	  oldx=spline_tool->active_point->x;
	  oldy=spline_tool->active_point->y;
	  spline_tool->active_point->x=x;
	  spline_tool->active_point->y=y;
	  temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->next;
	  while(temp)
	    {
	    temp->x += (x-oldx);
	    temp->y += (y-oldy);
	    temp=(SplinePoint*)((DL_node*)temp)->next;
	    }
	  temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	  while(temp)
	    {
	    temp->x += (x-oldx);
	    temp->y += (y-oldy);
	    temp=(SplinePoint*)((DL_node*)temp)->prev;
	    }
	}
      else if(spline_tool->click_on_point) /* move normally */
	{
	  spline_tool->active_point->x=x;
	  spline_tool->active_point->y=y;
	}
      break;
    case 1:			/* cubic bezier curve */
      if(!spline_tool->motion_active) /* not already in motion */
	{
	  if(spline_tool->click_on_point==3)
	    {
	      oldx=spline_tool->active_point->x;
	      oldy=spline_tool->active_point->y;
	      spline_tool->motion_active=1;
	      spline_tool->active_point->x=x;
	      spline_tool->active_point->y=y;
	      if(spline_tool->active_point->cx1!=-1)
		{
		  spline_tool->active_point->cx1 += (x-oldx);
		  spline_tool->active_point->cy1 += (y-oldy);
		}
	      if(spline_tool->active_point->cx2!=-1)
		{
		  spline_tool->active_point->cx2 += (x-oldx);
		  spline_tool->active_point->cy2 += (y-oldy);
		}
	    }
	  else if(spline_tool->click_on_point==5) /* move control point 2 */
	    {
	      temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	      spline_tool->motion_active=2;
	      spline_tool->active_point->cx2=x;
	      spline_tool->active_point->cy2=y;
	      if(temp)
		{
		  spline_tool->active_point->cx1=(2*spline_tool->active_point->x)-x;
		  spline_tool->active_point->cy1=(2*spline_tool->active_point->y)-y;
		}
	    }
	  else /* move control point */
	    {
	      temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	      spline_tool->motion_active=2;
	      spline_tool->active_point->cx1=x;
	      spline_tool->active_point->cy1=y;
	      if(temp)
		{
		  spline_tool->active_point->cx2=(2*spline_tool->active_point->x)-x;
		  spline_tool->active_point->cy2=(2*spline_tool->active_point->y)-y;
		}
	    }
	}
      else /* already in motion */
	{
	  if(spline_tool->motion_active==1) /* moving anchor */
	    {
	      oldx=spline_tool->active_point->x;
	      oldy=spline_tool->active_point->y;
	      spline_tool->active_point->x=x;
	      spline_tool->active_point->y=y;
	      if(spline_tool->active_point->cx1!=-1)
		{
		  spline_tool->active_point->cx1 += (x-oldx);
		  spline_tool->active_point->cy1 += (y-oldy);
		}
	      if(spline_tool->active_point->cx2!=-1)
		{
		  spline_tool->active_point->cx2 += (x-oldx);
		  spline_tool->active_point->cy2 += (y-oldy);
		}
	    
	    }
	  else if((spline_tool->motion_active==2) && (spline_tool->click_on_point==5))
	    /* moving control point 2 */ 
	    {
	      temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	      spline_tool->active_point->cx2=x;
	      spline_tool->active_point->cy2=y;
	      if(temp)
		{
		  spline_tool->active_point->cx1=(2*spline_tool->active_point->x)-x;
		  spline_tool->active_point->cy1=(2*spline_tool->active_point->y)-y;
		}
	    }
	  else        /* moving control point 1 */ 
	    {
	      temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	      spline_tool->active_point->cx1=x;
	      spline_tool->active_point->cy1=y;
	      if(temp)
		{
		  spline_tool->active_point->cx2=(2*spline_tool->active_point->x)-x;
		  spline_tool->active_point->cy2=(2*spline_tool->active_point->y)-y;
		}
	    }
	}
      break;
    case 2:			/* ellipse */
      if(spline_tool->click_on_point && (mevent->state & GDK_CONTROL_MASK))
	{
	  temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->prev;
	  if(!temp)
	    temp=(SplinePoint*)((DL_node*)spline_tool->active_point)->next;
	  oldx=spline_tool->active_point->x;
	  oldy=spline_tool->active_point->y;
	  spline_tool->active_point->x=x;
	  spline_tool->active_point->y=y;
	  temp->x += (x-oldx);
	  temp->y += (y-oldy);
	}
      /* constrain to a circle */
      else if(spline_tool->click_on_point && (mevent->state & GDK_SHIFT_MASK))
	{
	  printf("not yet implemented circle constraint\n");
	}
      else if(spline_tool->click_on_point) /* move normally */
	{
	  spline_tool->active_point->x=x;
	  spline_tool->active_point->y=y;
	}
      break;
    default:
      break;
    }
  spline_control(tool, RESUME, NULL);
}

/* callback function for draw_core button release events */
void
spline_button_release(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr)
{
  GDisplay *display;
  SplineTool *spline_tool;
  SplinePoint *point;

  spline_tool=(SplineTool*)tool->private;

  display=(GDisplay*)tool->gdisp_ptr;

  if(!spline_tool)
    return;

  if(spline_tool->click_on_point==4 || spline_tool->click_on_point==5) 
    {
      point=spline_point_check(display, spline_tool->active_spline->points, 
			       bevent->x, bevent->y, 0);
      if(point)
	{
	  spline_control(tool, PAUSE, display);
	  point->cx1=point->cy1=point->cx2=point->cy2=-1;
	  spline_control(tool, RESUME, display);
	}
    }

  spline_tool->motion_active=0;
  spline_tool->click_on_point=0;
  gdk_flush();
}

/* callback function for draw_core control events */
void
spline_control(Tool *tool, int action, void *disp_ptr)
{
  Spline *spline=NULL;
  GDisplay *display;

  display=tool->gdisp_ptr;
  if(display)
    {
      spline=((SplineTool*)display->gimage->active_layer->sl.spline_tool)->active_spline;
    }
  switch (action)
    {
    case PAUSE:
      if(spline && spline->drawn)
	{
	  spline_draw(tool);
	}
      break;
    case RESUME:
      if(spline && !spline->drawn)
	{
	  spline_draw(tool);
	}
      break;
    case HALT:
      if(spline && spline->drawn)
	spline_draw(tool);
      break;
    }
  gdk_flush();
}

/* sets the cursor for spline tool creation. */
void
spline_cursor_update(Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr)
{
  GDisplay *display;

  display=(GDisplay*)gdisp_ptr;
  gdisplay_install_tool_cursor(display, GDK_LEFT_PTR);
}

/* DrawCore drawing function */
void
spline_draw (Tool *tool)
{
  Spline *spline;
  SplineTool *spline_tool;
  GDisplay *display;

  display=tool->gdisp_ptr;
  if(!display)
      return;
  spline_tool=(SplineTool*)tool->private;
  if(!spline_tool)
      return;
  spline=spline_tool->active_spline;
  if(!spline)
      return;
  if(spline->drawn)
    {
      spline->drawn=0;
    }
  else
    {
      spline->drawn=1;
    }

  if(spline->draw_points)
    spline_draw_points(tool);

  switch(spline->type)
    {
    case 0:
      spline_draw_linear(tool);
      break;
    case 1:
      spline_draw_curve(tool);
      break;
    case 2:
      spline_draw_ellipse(tool);
      break;
    default:
      return;
      break;
    }
}

/* draw linear spline onto the screen */
void
spline_draw_linear (Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool;
  Spline *spline;
  SplinePoint *points;
  int x1, y1, x2, y2;

  display=tool->gdisp_ptr;
  if(!display)
    return;

  spline_tool=(SplineTool*)tool->private;
  if(!spline_tool)
    return;

  points=(SplinePoint*)(spline_tool->active_spline->points->head);
  if(!points)
      return;

  spline=spline_tool->active_spline;
  /* render the lines onto the screen */
  gdisplay_transform_coords(display, points->x, points->y, &x2, &y2, 0);
  while(points)
    {
      gdisplay_transform_coords(display, points->x, points->y, &x1, &y1, 0);
      gdk_draw_line(spline->window, spline->gc, x1, y1, x2, y2);
      x2=x1;
      y2=y1;
      points=(SplinePoint*)(((DL_node*)points)->next);
    }
}

/* draw curve spline onto the screen */
void
spline_draw_curve(Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool=(SplineTool*)tool->private;

  display=tool->gdisp_ptr;
  if(!display)
    return;

  if(!spline_tool)
    return;

  /* draw control arms, if any */
  spline_draw_curve_controls(display, spline_tool);

  /* Draw curve segment. */
  spline_draw_curve_section(display, spline_tool);
}

/* puts a curve section onto the screen.
   returns zero on success. */
int
spline_draw_curve_section(GDisplay *display, SplineTool *spline_tool)
{
  SplinePoint *point, *point2;
  Spline *spline;
  int_point a, b, c, d, p, q;
  int i;
  float t;

  point=(SplinePoint*)spline_tool->active_spline->points->head;
  if(!point)
    return 1;
  point2=point;
  spline=spline_tool->active_spline;
  /* set up control points and convert to screen space. */
  while(((DL_node*)point2)->next)
    {
      gdisplay_transform_coords(display, point->x, point->y, &a.x, &a.y, 0);
      if(point->cx1!=-1)
	gdisplay_transform_coords(display, point->cx1, point->cy1, &b.x, &b.y, 0);
      else
	{
	  b.x=a.x;
	  b.y=a.y;
	}
      point=(SplinePoint*)((DL_node*)point)->next;
      if(!point)
	return 1;
      gdisplay_transform_coords(display, point->x, point->y, &d.x, &d.y, 0);
      if(point->cx2!=-1)
	gdisplay_transform_coords(display, point->cx2, point->cy2, &c.x, &c.y, 0);
      else
	{
	  c.x=d.x;
	  c.y=d.y;
	}
      /* plot the curve to the screen */
      spline_calc_cubic_curve_point(&p, a, b, c, d, 0.0);
      for(i=1; i<SUBDIV; i++)
	{
	  t=(float)i/(float)SUBDIV;
	  spline_calc_cubic_curve_point(&q, a, b, c, d, t);
	  if((q.x!=p.x)||(q.y!=p.y))
	    {
	      gdk_draw_line(spline->window, spline->gc, p.x, p.y, q.x, q.y);
	      p=q;
	    }
	}
      point2=(SplinePoint*)((DL_node*)point2)->next;
      point=point2;
    }
  return 0;
}

/* puts a curves control arms onto the screen.
   assumes start point valid, but checks for valid point list.
   A valid list has a maximum of two anchor points and two control points.
   With the first point in the list always being the first anchor point, and
   no sequential control points.  All control points are assumed to belong to the
   anchor point which preceeded it.
   returns zero on success. */
int
spline_draw_curve_controls(GDisplay *display, SplineTool *spline_tool)
{
  Spline *spline;
  SplinePoint *point;
  int x1, x2, y1, y2;

  spline=spline_tool->active_spline;
  gdk_gc_set_line_attributes(spline->gc, 1, GDK_LINE_ON_OFF_DASH, 
			     GDK_CAP_BUTT, GDK_JOIN_MITER);

  point=(SplinePoint*)spline_tool->active_spline->points->head;
  if(!point)
    {
      gdk_gc_set_line_attributes(spline->gc, 1, GDK_LINE_SOLID, 
				 GDK_CAP_BUTT, GDK_JOIN_MITER);
      return 1;
    }

  while(point)
    {
      gdisplay_transform_coords(display, point->x, point->y, &x1, &y1, 0);
      if(point->cx1!=-1)
	{
	  gdisplay_transform_coords(display, point->cx1, point->cy1, &x2, &y2, 0);
	  gdk_draw_line(spline->window, spline->gc, x1, y1, x2, y2);
	}
      if(point->cx2!=-1)
	{
	  gdisplay_transform_coords(display, point->cx2, point->cy2, &x2, &y2, 0);
	  gdk_draw_line(spline->window, spline->gc, x1, y1, x2, y2);
	}
      point=(SplinePoint*)((DL_node*)point)->next;
    }

  gdk_gc_set_line_attributes(spline->gc, 1, GDK_LINE_SOLID, 
			     GDK_CAP_BUTT, GDK_JOIN_MITER);
  return 0;
}

/* draw linear spline onto the screen */
void
spline_draw_ellipse (Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool;
  Spline *spline;
  SplinePoint *point1, *point2;
  int cx, cy, rx, ry, dx, dy;

  display=tool->gdisp_ptr;
  if(!display)
    return;

  spline_tool=(SplineTool*)tool->private;
  if(!spline_tool)
    return;

  point1=(SplinePoint*)(spline_tool->active_spline->points->head);
  if(!point1)
    return;

  point2=(SplinePoint*)((DL_node*)point1)->next;
  if(!point2)
    return;

  spline=spline_tool->active_spline;

  /* determine bounding box */
  gdisplay_transform_coords(display, point1->x, point1->y, &cx, &cy, 0);
  gdisplay_transform_coords(display, point2->x, point2->y, &rx, &ry, 0);
  dx=MAXIMUM(cx-rx, rx-cx);
  dy=MAXIMUM(cy-ry, ry-cy);

  /* render the ellipse onto the screen */
  gdk_draw_line(spline->window, spline->gc, cx, cy, rx, ry); 
  gdk_draw_arc(spline->window, spline->gc, 0,  cx-dx, cy-dy, 2*dx, 2*dy, 0, 23040);
}

/* draw linear spline onto the screen */
void
spline_draw_points (Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool;
  Spline *spline;
  SplinePoint *points;
  int x, y;

  display=tool->gdisp_ptr;
  if(!display)
    return;

  spline_tool=(SplineTool*)tool->private;
  if(!spline_tool)
    return;

  points=(SplinePoint*)(spline_tool->active_spline->points->head);
  if(!points)
      return;

  spline=spline_tool->active_spline;

  while(points)
    {
      gdisplay_transform_coords(display, points->x, points->y, &x, &y, 0);
      gdk_draw_rectangle(spline->window, spline->gc, FALSE,
			 x-HALFWIDTH, y-HALFWIDTH, WIDTH, WIDTH);
      if(points->cx1!=-1)
	{
	  gdisplay_transform_coords(display, points->cx1, points->cy1, &x, &y, 0);
	  gdk_draw_arc(spline->window, spline->gc, FALSE,
		       x-HALFWIDTH, y-HALFWIDTH, WIDTH, WIDTH, 0, 23040);
	}
      if(points->cx2!=-1)
	{
	  gdisplay_transform_coords(display, points->cx2, points->cy2, &x, &y, 0);
	  gdk_draw_arc(spline->window, spline->gc, FALSE,
		       x-HALFWIDTH, y-HALFWIDTH, WIDTH, WIDTH, 0, 23040);
	}
      points=(SplinePoint*)(((DL_node*)points)->next);
    }
}

/* Create and Register Spline Options Dialog */
int 
spline_create_options(void)
{
  return 0;
}

/* create a new spline point */
SplinePoint* 
spline_new_splinepoint(int x, int y)
{
  SplinePoint *point;
  point=(SplinePoint*)malloc(sizeof(SplinePoint));
  point->node=(DL_node*)malloc(sizeof(DL_node));
  point->x=x;
  point->y=y;
  point->cx1=-1;
  point->cy1=-1;
  point->cx2=-1;
  point->cy2=-1;
  return point;
}

/* strokes active curve of tool with current brush and fg color. */
void 
spline_stroke(Tool *tool)
{
  SplineTool *spline_tool=(SplineTool*)tool->private;

  if(!spline_tool)
    return;

  switch (spline_tool->active_spline->type)
    {
    case 0:			/* Linear Curve */
      spline_stroke_linear(tool);
      break;
    case 1:			/* Cubic Bezier */
      spline_stroke_curve(tool);
      break;
    case 2:			/* Ellipse Curve */
      spline_stroke_ellipse(tool);
      break;
    default:
      break;
    }
}

void 
spline_stroke_linear(Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool=(SplineTool*)tool->private;
  SplinePoint *points;
  CanvasDrawable *drawable;
  CanvasDrawable *linked_drawable;

  display=tool->gdisp_ptr;
  if(!display || !spline_tool)
    return;
  points=(SplinePoint*)spline_tool->active_spline->points->head;
  drawable=gimage_active_drawable(display->gimage);
  if (GIMP_IS_LAYER (drawable))
    linked_drawable=gimage_linked_drawable(display->gimage);
  else 
    linked_drawable=NULL;

  spline_control(tool, PAUSE, display);
  /* set up the paint core */
  spline_paint_core.paint_func=(PaintFunc16)paintbrush_spline_non_gui_paint_func;
  spline_paint_core.painthit_setup=paintbrush_painthit_setup;
  /* go through and stroke between all anchor points */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, 
				linked_drawable, points->x, points->y))
    {
      return;
    }
  points=(SplinePoint*)((DL_node*)points)->next;
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  while(points)
    {
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, 
				    linked_drawable, points->x, points->y))
	{
	  return;
	}
      spline_paint_core.startx=spline_paint_core.lastx;
      spline_paint_core.starty=spline_paint_core.lasty;
      paint_core_16_interpolate(&spline_paint_core, drawable);
      spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.lasty=spline_paint_core.cury;
      points=(SplinePoint*)((DL_node*)points)->next;
    }

  spline_control(tool, RESUME, display);
  gdk_flush ();
  paint_core_16_finish(&spline_paint_core, gimage_active_drawable(display->gimage), tool->ID);
  gdisplay_flush(display);
}

void 
spline_stroke_ellipse(Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool=(SplineTool*)tool->private;
  SplinePoint *point1, *point2;
  CanvasDrawable *drawable;
  CanvasDrawable *ld;
  float b2, a, b, b2a2;
  int i;
  int *x, *y, spacing;

  display=tool->gdisp_ptr;
  if(!display || !spline_tool)
    return;
  point1=(SplinePoint*)spline_tool->active_spline->points->head;
  drawable=gimage_active_drawable(display->gimage);
  if (GIMP_IS_LAYER (drawable))
    ld=gimage_linked_drawable(display->gimage);
  else 
    ld=NULL;
  point1=(SplinePoint*)(spline_tool->active_spline->points->head);
  if(!point1)
      return;
  point2=(SplinePoint*)((DL_node*)point1)->next;
  if(!point2)
      return;
  /* Calculate the points on the elipse */
  a=abs(point1->x-point2->x);
  if(a==0)
    return;
  b=abs(point1->y-point2->y);

  x = (int *)malloc(sizeof(int)*((int)(a+1)));
  y = (int *)malloc(sizeof(int)*((int)(a+1)));

  b2=b*b;
  b2a2=b2/(a*a);
  for(i=0; i<a; i++)
    {
      x[i]=i+point1->x;
      y[i]=(int)sqrt(b2-(b2a2*i*i))+point1->y;
    }

  /* this is a temporary fix to force brush spacing to 1. */
  spacing=gimp_brush_get_spacing();
  gimp_brush_set_spacing(1);

  spline_control(tool, PAUSE, display);
  /* set up the paint core */
  spline_paint_core.paint_func=(PaintFunc16)paintbrush_spline_non_gui_paint_func;
  spline_paint_core.painthit_setup=paintbrush_painthit_setup;
  /* draw first quadrant */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				MAXIMUM(x[0], 0), 
				MAXIMUM(y[0]-2*(y[0]-point1->y), 0)))    
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  for(i=1; i<a; i++)
    {
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				    MAXIMUM(x[i], 0), 
				    MAXIMUM(y[i]-2*(y[i]-point1->y), 0)))
      {free(x);free(y); return;}
      spline_paint_core.startx=spline_paint_core.lastx;
      spline_paint_core.starty=spline_paint_core.lasty;
      paint_core_16_interpolate(&spline_paint_core, drawable);
      spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.lasty=spline_paint_core.cury;
    }
  /* draw second quadrant */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				MAXIMUM(x[0]-2*(x[0]-point1->x), 0), 
				MAXIMUM(y[0]-2*(y[0]-point1->y), 0)))
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  for(i=0; i<a; i++)
    {
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				    MAXIMUM(x[i]-2*(x[i]-point1->x), 0), 
				    MAXIMUM(y[i]-2*(y[i]-point1->y), 0)))
      {free(x);free(y); return;}
      spline_paint_core.startx=spline_paint_core.lastx;
      spline_paint_core.starty=spline_paint_core.lasty;
      paint_core_16_interpolate(&spline_paint_core, drawable);
      spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.lasty=spline_paint_core.cury;
    }
  /* draw third quadrant */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				MAXIMUM(x[0]-2*(x[0]-point1->x),0), 
				MAXIMUM(y[0], 0)))
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  for(i=1; i<a; i++)
    {
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				    MAXIMUM(x[i]-2*(x[i]-point1->x),0), 
				    MAXIMUM(y[i],0)))
      {free(x);free(y); return;}
      spline_paint_core.startx=spline_paint_core.lastx;
      spline_paint_core.starty=spline_paint_core.lasty;
      paint_core_16_interpolate(&spline_paint_core, drawable);
      spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.lasty=spline_paint_core.cury;
    }
  /* draw fourth quadrant */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				MAXIMUM(x[0], 0), MAXIMUM(y[0], 0)))
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  for(i=1; i<a; i++)
    {
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				    MAXIMUM(x[i], 0), MAXIMUM(y[i], 0)))
      {free(x);free(y); return;}
      spline_paint_core.startx=spline_paint_core.lastx;
      spline_paint_core.starty=spline_paint_core.lasty;
      paint_core_16_interpolate(&spline_paint_core, drawable);
      spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.lasty=spline_paint_core.cury;
    }
  /* join curves */
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, 
				MAXIMUM(x[(int)a-1], 0), MAXIMUM(y[(int)a-1], 0)))
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld,
				MAXIMUM(x[(int)a-1], 0), 
				MAXIMUM(y[(int)a-1]-2*(y[(int)a-1]-point1->y), 0)))    
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx;
  spline_paint_core.starty=spline_paint_core.lasty;
  paint_core_16_interpolate(&spline_paint_core, drawable);
  spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.lasty=spline_paint_core.cury;
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld,
				MAXIMUM(x[(int)a-1]-2*(x[(int)a-1]-point1->x),0), 
				MAXIMUM(y[(int)a-1],0)))
      {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
  if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld,
				    MAXIMUM(x[(int)a-1]-2*(x[(int)a-1]-point1->x), 0), 
				    MAXIMUM(y[(int)a-1]-2*(y[(int)a-1]-point1->y), 0)))
    {free(x);free(y); return;}
  spline_paint_core.startx=spline_paint_core.lastx;
  spline_paint_core.starty=spline_paint_core.lasty;
  paint_core_16_interpolate(&spline_paint_core, drawable);
  spline_paint_core.lastx=spline_paint_core.curx;
  spline_paint_core.lasty=spline_paint_core.cury;

  spline_control(tool, RESUME, display);
  gdk_flush ();
  paint_core_16_finish(&spline_paint_core, gimage_active_drawable(display->gimage), tool->ID);
  gdisplay_flush(display);
  gimp_brush_set_spacing(spacing);

  free(x);free(y);
}

void 
spline_stroke_curve(Tool *tool)
{
  GDisplay *display;
  SplineTool *spline_tool=(SplineTool*)tool->private;
  SplinePoint *point, *point2;
  CanvasDrawable *drawable;
  CanvasDrawable *ld;
  int_point a, b, c, d, p, q;
  int i, spacing;
  float t;

  display=tool->gdisp_ptr;
  if(!display || !spline_tool)
    return;
  drawable=gimage_active_drawable(display->gimage);
  if (GIMP_IS_LAYER (drawable))
    ld=gimage_linked_drawable(display->gimage);
  else 
    ld=NULL;

  /* this is a temporary fix to force brush spacing to 1. */
  spacing=gimp_brush_get_spacing();
  gimp_brush_set_spacing(1);

  point=(SplinePoint*)spline_tool->active_spline->points->head;
  if(!point)
    return;
  point2=point;

  spline_control(tool, PAUSE, display);
  /* set up the paint core */
  spline_paint_core.paint_func=(PaintFunc16)paintbrush_spline_non_gui_paint_func;
  spline_paint_core.painthit_setup=paintbrush_painthit_setup;

  /* set up control points and convert to screen space. */
  while(((DL_node*)point2)->next)
    {
      gdisplay_transform_coords(display, point->x, point->y, &a.x, &a.y, 0);
      if(point->cx1!=-1)
	gdisplay_transform_coords(display, point->cx1, point->cy1, &b.x, &b.y, 0);
      else
	{
	  b.x=a.x;
	  b.y=a.y;
	}
      point=(SplinePoint*)((DL_node*)point)->next;
      if(!point)
	return;
      gdisplay_transform_coords(display, point->x, point->y, &d.x, &d.y, 0);
      if(point->cx2!=-1)
	gdisplay_transform_coords(display, point->cx2, point->cy2, &c.x, &c.y, 0);
      else
	{
	  c.x=d.x;
	  c.y=d.y;
	}
      /* paint the curve to the screen */
      spline_calc_cubic_curve_point(&p, a, b, c, d, 0.0);
      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, p.x, p.y))
	return;
      spline_paint_core.startx=spline_paint_core.lastx=spline_paint_core.curx;
      spline_paint_core.starty=spline_paint_core.lasty=spline_paint_core.cury;
      for(i=1; i<SUBDIV; i++)
	{
	  t=(float)i/(float)SUBDIV;
	  spline_calc_cubic_curve_point(&q, a, b, c, d, t);
	  if((q.x!=p.x)||(q.y!=p.y))
	    {
	      if(!paint_core_16_init_linked(&spline_paint_core, drawable, ld, q.x, q.y))
		return;
	      spline_paint_core.startx=spline_paint_core.lastx;
	      spline_paint_core.starty=spline_paint_core.lasty;
	      paint_core_16_interpolate(&spline_paint_core, drawable);
	      spline_paint_core.lastx=spline_paint_core.curx;
	      spline_paint_core.lasty=spline_paint_core.cury;
	      p=q;
	    }
	}
      point2=(SplinePoint*)((DL_node*)point2)->next;
      point=point2;
    }
  spline_control(tool, RESUME, display);
  gdk_flush ();
  paint_core_16_finish(&spline_paint_core, gimage_active_drawable(display->gimage), tool->ID);
  gdisplay_flush(display);
  gimp_brush_set_spacing(spacing);
}

/* Initialize a PaintCore for spline stroking */
void
spline_paint_core_init(PaintCore *paint_core)
{
  paint_core->core=NULL;
  paint_core->noise_mode = 1.0;
  paint_core->undo_tiles=NULL;
  paint_core->canvas_tiles=NULL;
  paint_core->canvas_buf=NULL;
  paint_core->orig_buf=NULL;
  paint_core->canvas_buf_width=0;
  paint_core->canvas_buf_height=0;
  paint_core->noise_mask=NULL;
  paint_core->solid_mask=NULL;
  paint_core->linked_undo_tiles=NULL;
  paint_core->linked_canvas_buf=NULL;
  paint_core->painthit_setup=NULL;
  paint_core->paint_func=NULL;
  paint_core->linked_drawable=NULL;
  paint_core->drawable=NULL;
}

/* callback function for spline stroke menue event */
void 
spline_stroke_cmd_callback(GtkWidget *widget, gpointer data)
{
  if(active_tool->type==SPLINE)
    spline_stroke(active_tool);
  else
    g_warning("the spline tool must be activated for this to work.");
}

/* returns the number of points in a spline. */
int 
spline_number_points(Spline *spline)
{
  SplinePoint *points;
  int i=0;

  points=(SplinePoint*)spline->points->head;
  if(!points)
    return -1;
  while(points)
    {
      i++;
      points=(SplinePoint*)((DL_node*)points)->next;
    }
  return i;
}

/* checks coordinates x,y against points.
   returns point clicked on if point located at x,y
   returns NULL if not. */
SplinePoint*
spline_point_check(GDisplay *display, DL_list *points, int sx, int sy, int type)
{
  SplinePoint *point;
  int x, y;
  int status;

  point=(SplinePoint*)points->head;
  if(!point)
      return NULL;
  switch(type)
    {
    case 0:
      while(point)
	{
	  gdisplay_transform_coords(display, point->x, point->y, &x, &y, 0);
	  status=((sx>=x-HALFWIDTH)&&(sx<=x+HALFWIDTH)&&(sy>=y-HALFWIDTH)&&(sy<=y+HALFWIDTH));
	  if(status)
	    return point;;
	  point=(SplinePoint*)((DL_node*)point)->next;
	}
      break;
    case 1:
      while(point)
	{
	  if(point->cx1!=-1)
	    {
	      gdisplay_transform_coords(display, point->cx1, point->cy1, &x, &y, 0);
	      status=((sx>=x-HALFWIDTH)&&(sx<=x+HALFWIDTH)&&(sy>=y-HALFWIDTH)&&(sy<=y+HALFWIDTH));
	      if(status)
		return point;
	    }
	  point=(SplinePoint*)((DL_node*)point)->next;
	}
      break;
    case 2:
      while(point)
	{
	  if(point->cx2!=-1)
	    {
	      gdisplay_transform_coords(display, point->cx2, point->cy2, &x, &y, 0);
	      status=((sx>=x-HALFWIDTH)&&(sx<=x+HALFWIDTH)&&(sy>=y-HALFWIDTH)&&(sy<=y+HALFWIDTH));
	      if(status)
		return point;
	    }
	  point=(SplinePoint*)((DL_node*)point)->next;
	}
      break;
    default:
      break;
    }
  return NULL;
}

/* calculate point on a cubic bezier curve through subdivision */
int 
spline_calc_cubic_curve_point(int_point *dest, int_point a, int_point b,
			      int_point c, int_point d, float t)
{
  int_point ab, bc, cd, abbc, bccd;

  spline_point_linear_int(&ab, &a, &b, t);
  spline_point_linear_int(&bc, &b, &c, t);
  spline_point_linear_int(&cd, &c, &d, t);
  spline_point_linear_int(&abbc, &ab, &bc, t);
  spline_point_linear_int(&bccd, &bc, &cd, t);
  spline_point_linear_int(dest, &abbc, &bccd, t);

  return 0;
}

/* calculates midpoint between x1,y1 and x2,y2.
   results placed in mx,my.  returns zero on success.*/
int 
spline_point_linear_int(int_point *dest, int_point *a, int_point *b, float t)
{
  dest->x=a->x+(b->x-a->x)*t;
  dest->y=a->y+(b->y-a->y)*t;
  return 0;
}

/* copies contents of one spline into another.  It does NOT copy it's placement
   in a DL_list. It is up to the calling function to place it into a list.
   Returns pointer to new spline on success, NULL on failure. */
Spline* 
spline_copy_spline(Spline* spline)
{
  Spline *new_spline=NULL;
  SplinePoint *point1, *point2;

  if(spline)
    {
      new_spline=(Spline*)malloc(sizeof(Spline));
      new_spline->node=(DL_node*)malloc(sizeof(DL_node));
      DL_init((DL_node*)new_spline->node);
      new_spline->points=(DL_list*)malloc(sizeof(DL_list));
      DL_init((DL_node*)new_spline->points);
      new_spline->active=spline->active;
      new_spline->keyframe=spline->keyframe;
      new_spline->continuity=spline->continuity;
      new_spline->type=spline->type;
      new_spline->draw_points=spline->draw_points;
      if(!DL_is_empty(spline->points)) /* copy points if necessary */
	{
	  point1=(SplinePoint*)spline->points->head;
	  while(point1)
	    {
	      point2=(SplinePoint*)malloc(sizeof(SplinePoint));
	      point2->node=(DL_node*)malloc(sizeof(DL_node));
	      point2->x=point1->x;
	      point2->y=point1->y;
	      point2->cx1=point1->cx1;
	      point2->cy1=point1->cy1;
	      point2->cx2=point1->cx2;
	      point2->cy2=point1->cy2;
	      DL_append(new_spline->points, (DL_node*)point2);
	      point1=(SplinePoint*)((DL_node*)point1)->next;
	    }
	}
    }
  return new_spline;
}

void 
spline_edit_button_press(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr)
{
  printf("WARNING: This is currently a dead tool.\n");
}

void 
spline_edit_button_release(Tool *tool, GdkEventButton *bevent, void *gdisp_ptr)
{
}

void 
spline_edit_motion(Tool *tool, GdkEventMotion *mevent, void *gdisp_ptr)
{
}

void 
spline_edit_cursor_update(Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr)
{
  GDisplay *display;

  display=(GDisplay*)gdisp_ptr;
  gdisplay_install_tool_cursor(display, GDK_DRAFT_LARGE);
}
