/* app/spline.h
   header file for splines in CinePaint
   Copyright (c) 2004 Tom Huffman <huff8795@sbcglobal.net>
   License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef __SPLINE_H__
#define __SPLINE_H__

#include "tools.h"
#include "../lib/wire/dl_list.h"
#include "paint_core_16.h"

#define SPLINE_ANCHOR    1
#define SPLINE_CONTROL   2

typedef struct SplineTool SplineTool;
typedef struct Spline Spline;
typedef struct SplinePoint SplinePoint;

struct SplineTool
{
  Spline *active_spline;	/* Pointer to current spline */
  SplinePoint *active_point;	/* Active point.  Used by motion events to edit spline */
  int motion_active;		/* 0-no motion; 1- currently in motion; -1 motion not allowed */
  int click_on_point;		/* 0-not clicked on point; non-zero clicked on point */
};

struct Spline
{
  DL_node *node;		/* Double linked list of splines */
  int reserved;	        	/* reserved spot, because I don't quite understand it. */
  GdkGC *gc;                    /* Grahpics context for drawing functions  */
  GdkWindow *window;            /* Window to draw draw operation to      */
  int drawn;			/* 0-not drawn; non-zero drawn */
  DL_list *points;		/* Double linked list of Control Points. */
  int continuity;       	/* 0-C0 Continuity 1-C1 Continuity */
  int keyframe;  		/* 0-not keyframe; 1-keyframe */
  int active;	        	/* 0-Active Spline; non-zero is inactive spline */
  int type;			/* 0-Linear; 1-Cubic Bezier; 2-Ellipse */
  int draw_points;		/* flag to draw box around points. 0-no box; 1-draw box*/
};

struct SplinePoint
{
  DL_node *node;		/* Double linked list member data */
  int reserved;
  int x, y;			/* Point Coordinates in Image Space */
  int cx1, cy1;	                /* Coordinates of first control point in image space,
				   Set to -1 if no control point.*/
  int cx2, cy2;	                /* Coordinates of second control point in image space,
				   Set to -1 if no control point.*/
};

/*  spline functions  */
Tool* tools_new_spline(void);
void tools_free_spline(Tool *tool);
Tool* tools_new_spline_edit(void);
void tools_free_spline_edit(Tool *tool);
Spline* spline_new_spline(int continuity, int keyframe, int active, int type, GdkWindow *window);
SplinePoint* spline_new_splinepoint(int x, int y);
Spline* spline_get_active_spline(Layer *layer);
void spline_stroke_cmd_callback(GtkWidget *widget, gpointer data);
SplinePoint* spline_point_check(GDisplay *display, DL_list *points, int sx, int sy, int type);
Spline* spline_copy_spline(Spline* spline);
void spline_reset_splinetool(SplineTool *spline_tool, GDisplay *display);
void spline_control(Tool *tool, int action, void *disp_ptr);

#endif /* __SPLINE_H__ */
