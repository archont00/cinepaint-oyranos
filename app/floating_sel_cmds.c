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
#include "drawable.h"
#include "general.h"
#include "gimage.h"
#include "floating_sel.h"
#include "floating_sel_cmds.h"

static int int_value;
static int success;


/*************************/
/*  FLOATING_SEL_REMOVE  */

static Argument *
floating_sel_remove_invoker (Argument *args)
{
  Layer *floating_sel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((floating_sel = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  /*  Make sure it's a floating selection  */
  if (success)
    if (! layer_is_floating_sel (floating_sel))
      success = FALSE;

  if (success)
    floating_sel_remove (floating_sel);

  return procedural_db_return_args (&floating_sel_remove_proc, success);
}

/*  The procedure definition  */
ProcArg floating_sel_remove_args[] =
{
  { PDB_LAYER,
    "floating_sel",
    "the floating selection"
  }
};

ProcRecord floating_sel_remove_proc =
{
  "gimp_floating_sel_remove",
  "Remove the specified floating selection from its associated drawable",
  "This procedure removes the floating selection completely, without any side effects.  The associated drawable is then set to active.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  floating_sel_remove_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { floating_sel_remove_invoker } },
};


/*************************/
/*  FLOATING_SEL_ANCHOR  */

static Argument *
floating_sel_anchor_invoker (Argument *args)
{
  Layer *floating_sel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((floating_sel = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  /*  Make sure it's a floating selection  */
  if (success)
    if (! layer_is_floating_sel (floating_sel))
      success = FALSE;

  if (success)
    floating_sel_anchor (floating_sel);

  return procedural_db_return_args (&floating_sel_anchor_proc, success);
}

/*  The procedure definition  */
ProcArg floating_sel_anchor_args[] =
{
  { PDB_LAYER,
    "floating_sel",
    "the floating selection"
  }
};

ProcRecord floating_sel_anchor_proc =
{
  "gimp_floating_sel_anchor",
  "Anchor the specified floating selection to its associated drawable",
  "This procedure anchors the floating selection to its associated drawable.  This is similar to merging with a merge type of ClipToBottomLayer.  The floating selection layer is no longer valid after this operation.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  floating_sel_anchor_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { floating_sel_anchor_invoker } },
};


/***************************/
/*  FLOATING_SEL_TO_LAYER  */

static Argument *
floating_sel_to_layer_invoker (Argument *args)
{
  Layer *floating_sel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((floating_sel = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  /*  Make sure it's a floating selection  */
  if (success)
    if (! layer_is_floating_sel (floating_sel))
      success = FALSE;

  if (success)
    floating_sel_to_layer (floating_sel);

  return procedural_db_return_args (&floating_sel_to_layer_proc, success);
}

/*  The procedure definition  */
ProcArg floating_sel_to_layer_args[] =
{
  { PDB_LAYER,
    "floating_sel",
    "the floating selection"
  }
};

ProcRecord floating_sel_to_layer_proc =
{
  "gimp_floating_sel_to_layer",
  "Transforms the specified floating selection into a layer",
  "This procedure transforms the specified floating selection into a layer with the same offsets and extents.  The composited image will look precisely the same, but the floating selection layer will no longer be clipped to the extents of the drawable it was attached to.  The floating selection will become the active layer.  This procedure will not work if the floating selection has a different base type from the underlying image.  This might be the case if the floating selection is above an auxillary channel or a layer mask.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  floating_sel_to_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { floating_sel_to_layer_invoker } },
};
