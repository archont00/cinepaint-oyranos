#ifndef __GIMPBRUSHLISTP_H__
#define __GIMPBRUSHLISTP_H__

#include "listP.h"
#include "brushlist.h"

struct GimpBrushList
{
  GimpList gimplist;
  int num_brushes;
};

struct GimpBrushListClass
{
  GimpListClass parent_class;
};

typedef struct GimpBrushListClass GimpBrushListClass;


#define BRUSH_LIST_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_BRUSH_LIST, GimpBrushListClass)

#endif /* __GIMPBRUSHLISTP_H__ */
