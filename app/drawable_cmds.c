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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "general.h"
#include "gimage.h"
#include "drawable.h"
#include "drawable_cmds.h"
#include "canvas.h"
#include "paint_funcs_area.h"
#include "pixelrow.h"

static int int_value;
static int success;


/***************************/
/*  DRAWABLE_MERGE_SHADOW  */

static Argument *
drawable_merge_shadow_invoker (Argument *args)
{
  CanvasDrawable *drawable;
  int undo;

  success = TRUE;
  if ((drawable = drawable_get_ID (args[0].value.pdb_int)) == NULL)
    success = FALSE;

  if (success)
    undo = (args[1].value.pdb_int) ? TRUE : FALSE;

  if (success)
    drawable_merge_shadow (drawable, undo);

  return procedural_db_return_args (&drawable_merge_shadow_proc, success);
}

/*  The procedure definition  */
ProcArg drawable_merge_shadow_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "undo",
    "push merge to undo stack?"
  }
};

ProcRecord drawable_merge_shadow_proc =
{
  "gimp_drawable_merge_shadow",
  "Merge the shadow buffer with the specified drawable",
  "This procedure combines the contents of the image's shadow buffer (for temporary processing) with the specified drawable.  The \"undo\" parameter specifies whether to add an undo step for the operation.  Requesting no undo is useful for such applications as 'auto-apply'.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  drawable_merge_shadow_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { drawable_merge_shadow_invoker } },
};


/*******************/
/*  DRAWABLE_FILL  */

static Argument *
drawable_fill_invoker (Argument *args)
{
  CanvasDrawable *drawable;
  int fill_type;

  fill_type = WHITE_FILL;

  success = TRUE;
  if (success)
    drawable = drawable_get_ID (args[0].value.pdb_int);
  if (success)
    {
      int_value = args[1].value.pdb_int;

      switch (int_value)
	{
	case 0: fill_type = BACKGROUND_FILL; break;
	case 1: fill_type = WHITE_FILL; break;
	case 2: fill_type = TRANSPARENT_FILL; break;
	case 3: fill_type = NO_FILL; break;
	default: success = FALSE;
	}
    }

  if (success)
    drawable_fill (drawable, fill_type);

  return procedural_db_return_args (&drawable_fill_proc, success);
}

/*  The procedure definition  */
ProcArg drawable_fill_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "fill_type",
    "type of fill: { BG-IMAGE-FILL (0), WHITE-IMAGE-FILL (1), TRANS-IMAGE-FILL (2) }"
  }
};

ProcRecord drawable_fill_proc =
{
  "gimp_drawable_fill",
  "Fill the drawable with the specified fill mode.",
  "This procedure fills the drawable with the fill mode.  If the fill mode is background, the current background color is used.  If the fill type is white, then white is used.  Transparent fill only affects layers with an alpha channel, in which case the alpha channel is set to transparent.  If the drawable has no alpha channel, it is filled to white.  No fill leaves the drawable's contents undefined.  This procedure is unlike the bucket fill tool because it fills regardless of a selection",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  drawable_fill_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { drawable_fill_invoker } },
};


/*********************/
/*  DRAWABLE_UPDATE  */

static Argument *
drawable_update_invoker (Argument *args)
{
  int drawable_id;
  int x, y, w, h;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    {
      x = args[1].value.pdb_int;
      y = args[2].value.pdb_int;
      w = args[3].value.pdb_int;
      h = args[4].value.pdb_int;
    }

  if (success)
    drawable_update (drawable_get_ID (drawable_id), x, y, w, h);

  return procedural_db_return_args (&drawable_update_proc, success);
}

/*  The procedure definition  */
ProcArg drawable_update_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "x",
    "x coordinate of upper left corner of update region"
  },
  { PDB_INT32,
    "y",
    "y coordinate of upper left corner of update region"
  },
  { PDB_INT32,
    "w",
    "width of update region"
  },
  { PDB_INT32,
    "h",
    "height of update region"
  }
};

ProcRecord drawable_update_proc =
{
  "gimp_drawable_update",
  "Update the specified region of the drawable.",
  "This procedure updates the specified region of the drawable.  The (x, y) coordinate pair is relative to the drawable's origin, not to the image origin.  Therefore, the entire drawable can be updated with: {x->0, y->0, w->width, h->height}.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  drawable_update_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { drawable_update_invoker } },
};


/**************************/
/*  DRAWABLE_MASK_BOUNDS  */

static Argument *
drawable_mask_bounds_invoker (Argument *args)
{
  int drawable_id;
  int x1, y1, x2, y2;
  int non_empty;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;

  if (success)
    non_empty = drawable_mask_bounds (drawable_get_ID (drawable_id), &x1, &y1, &x2, &y2);

  return_args = procedural_db_return_args (&drawable_mask_bounds_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = non_empty;
      return_args[2].value.pdb_int = x1;
      return_args[3].value.pdb_int = y1;
      return_args[4].value.pdb_int = x2;
      return_args[5].value.pdb_int = y2;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_mask_bounds_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_mask_bounds_out_args[] =
{
  { PDB_INT32,
    "non-empty",
    "true if there is a selection"
  },
  { PDB_INT32,
    "x1",
    "x coordinate of upper left corner of selection bounds"
  },
  { PDB_INT32,
    "y1",
    "y coordinate of upper left corner of selection bounds"
  },
  { PDB_INT32,
    "x2",
    "x coordinate of lower right corner of selection bounds"
  },
  { PDB_INT32,
    "y2",
    "y coordinate of lower right corner of selection bounds"
  }
};

ProcRecord drawable_mask_bounds_proc =
{
  "gimp_drawable_mask_bounds",
  "Find the bounding box of the current selection in relation to the specified drawable",
  "This procedure returns the whether there is a selection.  If there is one, the upper left and lower righthand corners of its bounding box are returned.  These coordinates are specified relative to the drawable's origin, and bounded by the drawable's extents.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_mask_bounds_args,

  /*  Output arguments  */
  5,
  drawable_mask_bounds_out_args,

  /*  Exec method  */
  { { drawable_mask_bounds_invoker } },
};


/*********************/
/*  DRAWABLE_GIMAGE  */

static Argument *
drawable_gimage_invoker (Argument *args)
{
  int drawable_id;
  GImage *gimage;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    success = ((gimage = drawable_gimage (drawable_get_ID (drawable_id))) != NULL);

  return_args = procedural_db_return_args (&drawable_gimage_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimage->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_gimage_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_gimage_out_args[] =
{
  { PDB_IMAGE,
    "image",
    "the drawable's image"
  }
};

ProcRecord drawable_gimage_proc =
{
  "gimp_drawable_image",
  "Returns the drawable's image",
  "This procedure returns the drawable's image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_gimage_args,

  /*  Output arguments  */
  1,
  drawable_gimage_out_args,

  /*  Exec method  */
  { { drawable_gimage_invoker } },
};


/*******************/
/*  DRAWABLE_TYPE  */

static Argument *
drawable_type_invoker (Argument *args)
{
  CanvasDrawable *drawable;
  int type = -1;
  Argument *return_args;

  success = TRUE;

  if ((drawable = drawable_get_ID (args[0].value.pdb_int)) == NULL)
    success = FALSE;

  if (success)
    {
#define DRAWABLE_CMDS_C_1_cw
      type = tag_to_drawable_type (drawable_tag (drawable));
    }

  return_args = procedural_db_return_args (&drawable_type_proc, success);

  if (success)
    return_args[1].value.pdb_int = type;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_type_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_type_out_args[] =
{
  { PDB_INT32,
    "type",
    "the drawable's type: { RGB (0), RGBA (1), GRAY (2), GRAYA (3), INDEXED (4), INDEXEDA (5), U16_RGB (6), U16_RGBA (7), U16_GRAY (8), U16_GRAYA (9), U16_INDEXED (10), U16_INDEXEDA (11), FLOAT_RGB (12), FLOAT_RGBA (13), FLOAT_GRAY (14), FLOAT_GRAYA (15) }"
  }
};

ProcRecord drawable_type_proc =
{
  "gimp_drawable_type",
  "Returns the drawable's type",
  "This procedure returns the drawable's type.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_type_args,

  /*  Output arguments  */
  1,
  drawable_type_out_args,

  /*  Exec method  */
  { { drawable_type_invoker } },
};

/************************/
/*  DRAWABLE_HAS_ALPHA  */

static Argument *
drawable_has_alpha_invoker (Argument *args)
{
  int drawable_id;
  int has_alpha;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    has_alpha = drawable_has_alpha (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_has_alpha_proc, success);

  if (success)
    return_args[1].value.pdb_int = has_alpha;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_has_alpha_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_has_alpha_out_args[] =
{
  { PDB_INT32,
    "has_alpha",
    "does the drawable have an alpha channel?"
  }
};

ProcRecord drawable_has_alpha_proc =
{
  "gimp_drawable_has_alpha",
  "Returns non-zero if the drawable has an alpha channel",
  "This procedure returns whether the specified drawable has an alpha channel.  This can only be true for layers, and the associated type will be one of: { RGBA, GRAYA, INDEXEDA }.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_has_alpha_args,

  /*  Output arguments  */
  1,
  drawable_has_alpha_out_args,

  /*  Exec method  */
  { { drawable_has_alpha_invoker } },
};


/******************************/
/*  DRAWABLE_TYPE_WITH_ALPHA  */

static Argument *
drawable_type_with_alpha_invoker (Argument *args)
{
  CanvasDrawable *drawable;
  int type_with_alpha = -1;
  Argument *return_args;
  
  success = TRUE;
  if ((drawable = drawable_get_ID (args[0].value.pdb_int)) == NULL)
    success = FALSE;
  if (success)
    {
      Tag t = drawable_tag (drawable);
      t = tag_set_alpha (t, ALPHA_YES);
      type_with_alpha = tag_to_drawable_type (t);
    }
  
  return_args = procedural_db_return_args (&drawable_type_with_alpha_proc, success);

  if (success)
    return_args[1].value.pdb_int = type_with_alpha;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_type_with_alpha_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_type_with_alpha_out_args[] =
{
  { PDB_INT32,
    "type_with_alpha",
    "the drawable's type with alpha: { RGBA (0), GRAYA (1), INDEXEDA (2) }"
  }
};

ProcRecord drawable_type_with_alpha_proc =
{
  "gimp_drawable_type_with_alpha",
  "Returns the drawable's type with alpha",
  "This procedure returns the drawable's type if an alpha channel were added.  If the type is currently Gray, for instance, the returned type would be GrayA.  If the drawable already has an alpha channel, the drawable's type is simply returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_type_with_alpha_args,

  /*  Output arguments  */
  1,
  drawable_type_with_alpha_out_args,

  /*  Exec method  */
  { { drawable_type_with_alpha_invoker } },
};


/********************/
/*  DRAWABLE_COLOR  */

static Argument *
drawable_color_invoker (Argument *args)
{
  int drawable_id;
  int color;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    color = drawable_color (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_color_proc, success);

  if (success)
    return_args[1].value.pdb_int = color;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_color_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_color_out_args[] =
{
  { PDB_INT32,
    "color",
    "non-zero if the drawable is an RGB type"
  }
};

ProcRecord drawable_color_proc =
{
  "gimp_drawable_color",
  "Returns whether the drawable is an RGB based type",
  "This procedure returns non-zero if the specified drawable is of type { RGB, RGBA }.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_color_args,

  /*  Output arguments  */
  1,
  drawable_color_out_args,

  /*  Exec method  */
  { { drawable_color_invoker } },
};


/********************/
/*  DRAWABLE_GRAY  */

static Argument *
drawable_gray_invoker (Argument *args)
{
  int drawable_id;
  int gray;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    gray = drawable_gray (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_gray_proc, success);

  if (success)
    return_args[1].value.pdb_int = gray;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_gray_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_gray_out_args[] =
{
  { PDB_INT32,
    "gray",
    "non-zero if the drawable is a grayscale type"
  }
};

ProcRecord drawable_gray_proc =
{
  "gimp_drawable_gray",
  "Returns whether the drawable is an grayscale type",
  "This procedure returns non-zero if the specified drawable is of type { Gray, GrayA }.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_gray_args,

  /*  Output arguments  */
  1,
  drawable_gray_out_args,

  /*  Exec method  */
  { { drawable_gray_invoker } },
};


/**********************/
/*  DRAWABLE_INDEXED  */

static Argument *
drawable_indexed_invoker (Argument *args)
{
  int drawable_id;
  int indexed;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    indexed = drawable_indexed (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_indexed_proc, success);

  if (success)
    return_args[1].value.pdb_int = indexed;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_indexed_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_indexed_out_args[] =
{
  { PDB_INT32,
    "indexed",
    "non-zero if the drawable is a indexed type"
  }
};

ProcRecord drawable_indexed_proc =
{
  "gimp_drawable_indexed",
  "Returns whether the drawable is an indexed type",
  "This procedure returns non-zero if the specified drawable is of type { Indexed, IndexedA }.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_indexed_args,

  /*  Output arguments  */
  1,
  drawable_indexed_out_args,

  /*  Exec method  */
  { { drawable_indexed_invoker } },
};

/********************/
/*  DRAWABLE_NUM_CHANNELS  */

static Argument *
drawable_num_channels_invoker (Argument *args)
{
  int drawable_id;
  int num_channels;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    num_channels = tag_num_channels (drawable_tag (drawable_get_ID (drawable_id)));

  return_args = procedural_db_return_args (&drawable_num_channels_proc, success);

  if (success)
    return_args[1].value.pdb_int = num_channels;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_num_channels_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_num_channels_out_args[] =
{
  { PDB_INT32,
    "num_channels",
    "num_channels per pixel"
  }
};

ProcRecord drawable_num_channels_proc =
{
  "gimp_drawable_num_channels",
  "Returns the num_channels per pixel",
  "This procedure returns the number of num_channels per pixel (or the number of channels) for the specified drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_num_channels_args,

  /*  Output arguments  */
  1,
  drawable_num_channels_out_args,

  /*  Exec method  */
  { { drawable_num_channels_invoker } },
};


/********************/
/*  DRAWABLE_PRECSION  */

static Argument *
drawable_precision_invoker (Argument *args)
{
  int drawable_id;
  Precision precision;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    precision = tag_precision (drawable_tag (drawable_get_ID (drawable_id)));

  return_args = procedural_db_return_args (&drawable_precision_proc, success);

  if (success)
    return_args[1].value.pdb_int = (int)precision;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_precision_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_precision_out_args[] =
{
  { PDB_INT32,
    "precision",
    "precision of data"
  }
};

ProcRecord drawable_precision_proc =
{
  "gimp_drawable_precision",
  "Returns the precision per pixel",
  "This procedure returns the precision (U8, U16, FLOAT, FLOAT16) for the specified drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_precision_args,

  /*  Output arguments  */
  1,
  drawable_precision_out_args,

  /*  Exec method  */
  { { drawable_precision_invoker } },
};


/********************/
/*  DRAWABLE_BYTES  */

static Argument *
drawable_bytes_invoker (Argument *args)
{
  int drawable_id;
  int bytes;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    bytes = drawable_bytes (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_bytes_proc, success);

  if (success)
    return_args[1].value.pdb_int = bytes;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_bytes_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_bytes_out_args[] =
{
  { PDB_INT32,
    "bytes",
    "bytes per pixel"
  }
};

ProcRecord drawable_bytes_proc =
{
  "gimp_drawable_bytes",
  "Returns the bytes per pixel",
  "This procedure returns the number of bytes per pixel (or the number of channels) for the specified drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_bytes_args,

  /*  Output arguments  */
  1,
  drawable_bytes_out_args,

  /*  Exec method  */
  { { drawable_bytes_invoker } },
};


/********************/
/*  DRAWABLE_WIDTH  */

static Argument *
drawable_width_invoker (Argument *args)
{
  int drawable_id;
  int width;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    width = drawable_width (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_width_proc, success);

  if (success)
    return_args[1].value.pdb_int = width;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_width_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_width_out_args[] =
{
  { PDB_INT32,
    "width",
    "width of drawable"
  }
};

ProcRecord drawable_width_proc =
{
  "gimp_drawable_width",
  "Returns the width of the drawable",
  "This procedure returns the specified drawable's width in pixels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_width_args,

  /*  Output arguments  */
  1,
  drawable_width_out_args,

  /*  Exec method  */
  { { drawable_width_invoker } },
};

/********************/
/*  DRAWABLE_HEIGHT  */

static Argument *
drawable_height_invoker (Argument *args)
{
  int drawable_id;
  int height;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    height = drawable_height (drawable_get_ID (drawable_id));

  return_args = procedural_db_return_args (&drawable_height_proc, success);

  if (success)
    return_args[1].value.pdb_int = height;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_height_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_height_out_args[] =
{
  { PDB_INT32,
    "height",
    "height of drawable"
  }
};

ProcRecord drawable_height_proc =
{
  "gimp_drawable_height",
  "Returns the height of the drawable",
  "This procedure returns the height of the specified drawable in pixels",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_height_args,

  /*  Output arguments  */
  1,
  drawable_height_out_args,

  /*  Exec method  */
  { { drawable_height_invoker } },
};


/**********************/
/*  DRAWABLE_OFFSETS  */

static Argument *
drawable_offsets_invoker (Argument *args)
{
  int drawable_id;
  int offset_x;
  int offset_y;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    drawable_offsets (drawable_get_ID (drawable_id), &offset_x, &offset_y);

  return_args = procedural_db_return_args (&drawable_offsets_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = offset_x;
      return_args[2].value.pdb_int = offset_y;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_offsets_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_offsets_out_args[] =
{
  { PDB_INT32,
    "offset_x",
    "x offset of drawable"
  },
  { PDB_INT32,
    "offset_y",
    "y offset of drawable"
  }
};

ProcRecord drawable_offsets_proc =
{
  "gimp_drawable_offsets",
  "Returns the offsets for the drawable",
  "This procedure returns the specified drawable's offsets.  This only makes sense if the drawable is a layer since channels are anchored.  The offsets of a channel will be returned as 0.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_offsets_args,

  /*  Output arguments  */
  2,
  drawable_offsets_out_args,

  /*  Exec method  */
  { { drawable_offsets_invoker } },
};


/********************/
/*  DRAWABLE_LAYER  */

static Argument *
drawable_layer_invoker (Argument *args)
{
  int drawable_id;
  int layer;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    layer = (drawable_layer (drawable_get_ID (drawable_id))) ? TRUE : FALSE;

  return_args = procedural_db_return_args (&drawable_layer_proc, success);

  if (success)
    return_args[1].value.pdb_int = layer;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_layer_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_layer_out_args[] =
{
  { PDB_INT32,
    "layer",
    "non-zero if the drawable is a layer"
  }
};

ProcRecord drawable_layer_proc =
{
  "gimp_drawable_layer",
  "Returns whether the drawable is a layer",
  "This procedure returns non-zero if the specified drawable is a layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_layer_args,

  /*  Output arguments  */
  1,
  drawable_layer_out_args,

  /*  Exec method  */
  { { drawable_layer_invoker } },
};


/*************************/
/*  DRAWABLE_LAYER_MASK  */

static Argument *
drawable_layer_mask_invoker (Argument *args)
{
  int drawable_id;
  int layer_mask;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    layer_mask = (drawable_layer_mask (drawable_get_ID (drawable_id))) ? TRUE : FALSE;

  return_args = procedural_db_return_args (&drawable_layer_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = layer_mask;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_layer_mask_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_layer_mask_out_args[] =
{
  { PDB_INT32,
    "layer_mask",
    "non-zero if the drawable is a layer mask"
  }
};

ProcRecord drawable_layer_mask_proc =
{
  "gimp_drawable_layer_mask",
  "Returns whether the drawable is a layer mask",
  "This procedure returns non-zero if the specified drawable is a layer mask.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_layer_mask_args,

  /*  Output arguments  */
  1,
  drawable_layer_mask_out_args,

  /*  Exec method  */
  { { drawable_layer_mask_invoker } },
};


/**********************/
/*  DRAWABLE_CHANNEL  */

static Argument *
drawable_channel_invoker (Argument *args)
{
  int drawable_id;
  int channel;
  Argument *return_args;

  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  if (success)
    channel = (drawable_channel (drawable_get_ID (drawable_id))) ? TRUE : FALSE;

  return_args = procedural_db_return_args (&drawable_channel_proc, success);

  if (success)
    return_args[1].value.pdb_int = channel;

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_channel_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  }
};

ProcArg drawable_channel_out_args[] =
{
  { PDB_INT32,
    "channel",
    "non-zero if the drawable is a channel"
  }
};

ProcRecord drawable_channel_proc =
{
  "gimp_drawable_channel",
  "Returns whether the drawable is a channel",
  "This procedure returns non-zero if the specified drawable is a channel.  Even though a layer mask is technically considered a channel, this call will return 0 on a layer mask.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  drawable_channel_args,

  /*  Output arguments  */
  1,
  drawable_channel_out_args,

  /*  Exec method  */
  { { drawable_channel_invoker } },
};

/**********************/
/*  SET/GET PIXEL     */

static Argument *
drawable_set_pixel_invoker (Argument *args)
{
  CanvasDrawable *drawable;
  int x, y;
  int num_channels;
  unsigned char *pixel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || drawable_gimage (drawable) == NULL)
	success = FALSE;
    }
  if (success)
    {
      x = args[1].value.pdb_int;
      y = args[2].value.pdb_int;

      if (x < 0 || x >= drawable_width (drawable))
	success = FALSE;
      if (y < 0 || y >= drawable_height (drawable))
	success = FALSE;
    }
  if (success)
    {
      num_channels = args[3].value.pdb_int;
      if (num_channels != drawable_bytes (drawable))
	success = FALSE;
    }
  if (success)
    pixel = (unsigned char *) args[4].value.pdb_pointer;

  if (success)
    {
      Canvas * c = drawable_data (drawable);

      if (canvas_portion_refrw (c, x, y) == REFRC_OK)
        {
          PixelRow from, to;
          Format f = FORMAT_NONE;
          Alpha a = ALPHA_NONE;

          switch (drawable_bytes (drawable))
            {
            case 1:
              a = ALPHA_NO; f = FORMAT_GRAY; break;
            case 2:
              a = ALPHA_YES; f = FORMAT_GRAY; break;
            case 3:
              a = ALPHA_NO; f = FORMAT_RGB; break;
            case 4:
              a = ALPHA_YES; f = FORMAT_RGB; break;
            }
          
          pixelrow_init (&from,
                         tag_new (PRECISION_U8, f, a),
                         pixel,
                         1);

          pixelrow_init (&to,
                         canvas_tag (c),
                         canvas_portion_data (c, x, y),
                         1);

          copy_row (&from, &to);
          
          canvas_portion_unref (c, x, y);
        }
      else
        {
          success = FALSE;
        }
    }

  return procedural_db_return_args (&drawable_set_pixel_proc, success);
}

/*  The procedure definition  */
ProcArg drawable_set_pixel_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "x coordinate",
    "the x coordinate"
  },
  { PDB_INT32,
    "y coordinate",
    "the y coordinate"
  },
  { PDB_INT32,
    "num_channels",
    "the number of channels for the pixel"
  },
  { PDB_INT8ARRAY,
    "pixel",
    "the pixel value"
  }
};

ProcRecord drawable_set_pixel_proc =
{
  "gimp_drawable_set_pixel",
  "Sets the value of the pixel at the specified coordinates",
  "This procedure sets the pixel value at the specified coordinates.  The 'num_channels' argument must always be equal to the bytes-per-pixel value for the specified drawable.",
  "Spencer Kimball & Peter Mattis & Josh MacDonald",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  drawable_set_pixel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { drawable_set_pixel_invoker } },
};


static Argument *
drawable_get_pixel_invoker (Argument *args)
{
  int drawable_id;
  CanvasDrawable *drawable;
  int x, y;
  unsigned char *pixel;
  Argument *return_args;
  Canvas * c;
  
  pixel = NULL;
  c = NULL;
  
  success = TRUE;
  if (success)
    drawable_id = args[0].value.pdb_int;
  /*  Make sure the drawable exists  */
  if (success)
    {
      drawable = drawable_get_ID (drawable_id);
      if (drawable == NULL)
	success = FALSE;
    }
  if (success)
    {
      x = args[1].value.pdb_int;
      y = args[2].value.pdb_int;

      if (x < 0 || x >= drawable_width (drawable))
	success = FALSE;
      if (y < 0 || y >= drawable_height (drawable))
	success = FALSE;
    }
  
  if (success)
    {
      c = drawable_data (drawable);
      pixel = (unsigned char *) g_new (unsigned char, TAG_MAX_BYTES);
      
      if (canvas_portion_refro (c, x, y) == REFRC_OK)
        {
          PixelRow from, to;
          
          pixelrow_init (&from,
                         canvas_tag (c),
                         canvas_portion_data (c, x, y),
                         1);

          pixelrow_init (&to,
                         tag_set_precision (canvas_tag (c), PRECISION_U8),
                         pixel,
                         1);

          copy_row (&from, &to);
          
          canvas_portion_unref (c, x, y);
        } 
    }
  
  return_args = procedural_db_return_args (&drawable_get_pixel_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = tag_bytes (tag_set_precision (canvas_tag (c), PRECISION_U8));
      return_args[2].value.pdb_pointer = pixel;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg drawable_get_pixel_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "x coordinate",
    "the x coordinate"
  },
  { PDB_INT32,
    "y coordinate",
    "the y coordinate"
  }
};

ProcArg drawable_get_pixel_out_args[] =
{
  { PDB_INT32,
    "num_channels",
    "the number of channels for the pixel"
  },
  { PDB_INT8ARRAY,
    "pixel",
    "the pixel value"
  }
};

ProcRecord drawable_get_pixel_proc =
{
  "gimp_drawable_get_pixel",
  "Gets the value of the pixel at the specified coordinates",
  "This procedure gets the pixel value at the specified coordinates.  The 'num_channels' argument must always be equal to the bytes-per-pixel value for the specified drawable.",
  "Spencer Kimball & Peter Mattis & Josh MacDonald",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  drawable_get_pixel_args,

  /*  Output arguments  */
  2,
  drawable_get_pixel_out_args,

  /*  Exec method  */
  { { drawable_get_pixel_invoker } },
};
