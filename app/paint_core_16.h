/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PAINT_CORE_16_H__
#define __PAINT_CORE_16_H__

#include "../lib/wire/c_typedefs.h"

/* the different states that the painting function can be called with  */
typedef enum
{
  INIT_PAINT,
  MOTION_PAINT,
  PAUSE_PAINT,
  RESUME_PAINT,
  FINISH_PAINT
} PaintCoreState;


/* brush application types  */
typedef enum
{
  HARD,    /* pencil */
  SOFT,    /* paintbrush */
  EXACT    /* no modification of brush mask */
} BrushHardness;

/* paint application modes  */
typedef enum
{
  CONSTANT,    /* pencil, paintbrush, airbrush, clone */
  INCREMENTAL  /* convolve, smudge */
} ApplyMode;

/* init mode  */
typedef enum
{
  NORMAL_SETUP,    
  LINKED_SETUP,
  NORMAL_ALPHA_SETUP,
  LINKED_ALPHA_SETUP,
  NOT_SETUP
} SetUpMode;

typedef enum
{
  PRE_MULT_ALPHA=0,
  UNPRE_MULT_ALPHA=1
  
} PreMultAlpha;
/* structure definitions */
typedef struct PaintCore16 PaintCore16;
typedef void (*PaintFunc16)(PaintCore16 *, CanvasDrawable *, int);
typedef void (*PainthitSetup)(PaintCore16 *, Canvas *); 

struct PaintCore16
{
  /* core select object */
  DrawCore * core;

  /* various coords */
  double   startx, starty;
  double   curx, cury;
  double   lastx, lasty;

  /* variables for wacom */
  double          curpressure;   /*  current pressure           */
  double          curxtilt;      /*  current xtilt              */
  double          curytilt;      /*  current ytilt              */
  double    startpressure; /*  starting pressure          */
  double    startxtilt;    /*  starting xtilt             */
  double    startytilt;    /*  starting ytilt             */
  double    lastpressure;  /*  last pressure              */
  double    lastxtilt;     /*  last xtilt                 */
  double    lastytilt;     /*  last ytilt		  */ 

  /* state of buttons and keys */
  int      state;

  /* fade out and spacing info */
  double   distance;
  double   spacing;
  double   spacing_scale;

  /* noise information */
  int noise_mode;

  /* undo extents in image space coords */
  int      x1, y1;
  int      x2, y2;

  /* size and location of paint hit */
  int      x, y;
  int      w, h;

  CanvasDrawable * drawable;

  /* mask for current brush --dont delete */
  Canvas * brush_mask;

  /* stuff for a noise brush --delete every painthit */
  Canvas * noise_mask;

  /* the solid brush --delete every painthit if noise, else delete on mouse up*/
  Canvas * solid_mask;

  /* the subsampled brush --delete every painthit*/
  Canvas * subsampled_mask;

  /* the portions of the original drawable which have been modified */
  Canvas *  undo_tiles;

  /* a mask holding the cumulative brush stroke */
  Canvas *  canvas_tiles;

  /* the paint hit to mask and apply */
  Canvas *  canvas_buf;
  guint canvas_buf_height;
  guint canvas_buf_width;

  /* original image for clone tool */
  Canvas *  orig_buf; 

  /* tool-specific paint function */
  PaintFunc16     paint_func;

  PaintFunc16     cursor_func;

  /* linked drawable stuff*/ 
  CanvasDrawable * linked_drawable;
  Canvas *  linked_undo_tiles;
  Canvas *  linked_canvas_buf;
 
  /* tool-specific painthit set-up function */
  PainthitSetup painthit_setup; 

  SetUpMode setup_mode;

  PreMultAlpha pre_mult;
};


/* create and destroy a paint_core based tool */
int                paint_core_16_init            (PaintCore16 *, CanvasDrawable *, double x, double y);
Tool *     paint_core_16_new             (int type);

void               paint_core_16_free            (Tool *);


/* high level tool control functions */

void               paint_core_16_interpolate     (PaintCore16 *, CanvasDrawable *);
void               paint_core_16_interpolate2     (PaintCore16 *,
                                                  CanvasDrawable **, int);

void               paint_core_16_finish          (PaintCore16 *,
                                                  CanvasDrawable *,
                                                  int toolid);

void               paint_core_16_cleanup         (PaintCore16 *);

void               paint_core_16_setnoise (PaintCore16 *, int apply_noise); 
void               paint_core_16_setnoise_params (PaintCore16 *, gdouble, gdouble, gdouble); 
			


/* painthit buffer functions */
void   paint_core_16_area_setup                  (PaintCore16 *,
                                                  CanvasDrawable *);

Canvas *   paint_core_16_area_original   (PaintCore16 *,
                                                  CanvasDrawable *,
                                                  int x1,
                                                  int y1,
                                                  int x2,
                                                  int y2);

Canvas *   paint_core_16_area_original2   (PaintCore16 *,
                                                  CanvasDrawable *,
                                                  int x1,
                                                  int y1,
                                                  int x2,
                                                  int y2);

void               paint_core_16_area_paste      (PaintCore16 *,
                                                  CanvasDrawable *,
                                                  gfloat brush_opacity,
                                                  gfloat image_opacity,
                                                  BrushHardness brush_hardness,
						  gdouble brush_scale,
                                                  ApplyMode apply_mode,
                                                  int paint_mode
                                                  );

void               paint_core_16_area_replace    (PaintCore16 *,
                                                  CanvasDrawable *,
                                                  gfloat brush_opacity,
                                                  gfloat image_opacity,
                                                  BrushHardness brush_hardness,
                                                  ApplyMode apply_mode,
                                                  int paint_mode);
int paint_core_16_init_linked (PaintCore16 *, CanvasDrawable *, 
			       CanvasDrawable *, double, double);







/* paintcore for PDB functions to use */
extern PaintCore16  non_gui_paint_core_16;


/* minimize the context diffs */
#define PaintCore PaintCore16
#define paint_core_new paint_core_16_new
#define paint_core_free paint_core_16_free
#define paint_core_init paint_core_16_init
#define paint_core_interpolate paint_core_16_interpolate
#define paint_core_finish paint_core_16_finish
#define paint_core_cleanup paint_core_16_cleanup
#define non_gui_paint_core non_gui_paint_core_16


/*=======================================

  this should be private with undo

  */

/*  Special undo type  */
typedef struct PaintUndo PaintUndo;

struct PaintUndo
{
  int             tool_ID;
  double          lastx;
  double          lasty;
  double          lastpressure;
  double          lastxtilt;
  double          lastytilt;

};




#endif  /*  __PAINT_CORE_16_H__  */
