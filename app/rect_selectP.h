#ifndef __RECT_SELECTP_H__
#define __RECT_SELECTP_H__

#include "draw_core.h"

typedef struct RectSelect RectSelect, EllipseSelect;

struct RectSelect
{
  DrawCore *      core;       /*  Core select object                      */

  int             x, y;       /*  upper left hand coordinate              */
  int             w, h;       /*  width and height                        */
  int             center;     /*  is the selection being created from the center out? */

  int             op;         /*  selection operation (ADD, SUB, etc)     */
};

#endif  /*  __RECT_SELECTP_H__  */
