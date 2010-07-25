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
#include <string.h>
#include "appenv.h"
#include "general.h"
#include "gdisplay.h"
#include "gdisplay_cmds.h"

static int int_value;
static int success;
static Argument *return_args;


/******************/
/*  GDISPLAY_NEW  */

static Argument *
gdisplay_new_invoker (Argument *args)
{
  GImage *gimage;
  GDisplay *gdisp;
  unsigned int scale = 0x101;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;

      /*  make sure the image has layers before displaying it  */
      if (success && gimage->layers == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gdisp = gdisplay_new (gimage, scale)) != NULL);

  /*  create the new image  */
  return_args = procedural_db_return_args (&gdisplay_new_proc, success);


  if (success)
    return_args[1].value.pdb_int = gdisp->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_new_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
    { PDB_DISPLAY,
      "display",
      "the new display"
    }

};

ProcArg gdisplay_new_out_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the new display"
  }

};

ProcRecord gdisplay_new_proc =
{
  "gimp_display_new",
  "Creates a new display for the specified image",
  "Creates a new display for the specified image.  If the image already has a display, another is added.  Multiple displays are handled transparently by the GIMP.  The newly created display is returned and can be subsequently destroyed with a call to 'gimp_display_delete'.  This procedure only makes sense for use with the GIMP UI.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_new_args,

  /*  Output arguments  */
  1,
  gdisplay_new_out_args,

  /*  Exec method  */
  { { gdisplay_new_invoker } },
};

/******************/
/*  GDISPLAY_FM   */

static Argument *
gdisplay_fm_invoker (Argument *args)
{
  GImage *gimage;
  GDisplay *gdisp, *disp;
  unsigned int scale = 0x101;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
      
      int_value = args[1].value.pdb_int;
      if ((disp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;

      /*  make sure the image has layers before displaying it  */
      if (success && gimage->layers == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gdisp = gdisplay_fm (gimage, scale, disp)) != NULL);

  /*  create the new image  */
  return_args = procedural_db_return_args (&gdisplay_new_proc, success);


  if (success)
    return_args[1].value.pdb_int = gdisp->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_fm_args[] =
{
    { PDB_IMAGE,
      "image",
      "the image"
    },
    { PDB_DISPLAY,
      "display",
      "the display"
    }
};

ProcArg gdisplay_fm_out_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the new display"
  }
};

ProcRecord gdisplay_fm_proc =
{
  "gimp_display_fm",
  "Creates a new display for the specified image",
  "Creates a new display for the specified image.  If the image already has a display, another is added.  Multiple displays are handled transparently by the GIMP.  The newly created display is returned and can be subsequently destroyed with a call to 'gimp_display_delete'.  This procedure only makes sense for use with the GIMP UI.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_fm_args,

  /*  Output arguments  */
  1,
  gdisplay_fm_out_args,

  /*  Exec method  */
  { { gdisplay_fm_invoker } },
};


/******************/
/*GDISPLAY_ACTIVE */

static Argument *
gdisplay_active_invoker (Argument *args)
{
  GDisplay *gdisp;

  gdisp = NULL;

  success = ((gdisp = gdisplay_active ()) != NULL);

# ifdef DEBUG
  printf("%s:%d %s() disp %d success %d\n", __FILE__,__LINE__,__func__,(intptr_t)gdisp, success);
  if(gdisp)
    printf("%s:%d %s() disp->ID %d\n", __FILE__,__LINE__,__func__,gdisp->ID);
# endif
  
  /*  create the new image  */
  return_args = procedural_db_return_args (&gdisplay_active_proc, success);


  if (success)
    return_args[1].value.pdb_int = gdisp->ID;

  return return_args;
}


ProcArg gdisplay_active_out_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the new display"
  }
};

ProcRecord gdisplay_active_proc =
{
  "gimp_display_active",
  "Creates a new display for the specified image",
  "Creates a new display for the specified image.  If the image already has a display, another is added.  Multiple displays are handled transparently by the GIMP.  The newly created display is returned and can be subsequently destroyed with a call to 'gimp_display_delete'.  This procedure only makes sense for use with the GIMP UI.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  gdisplay_active_out_args,

  /*  Exec method  */
  { { gdisplay_active_invoker } },
};


/*********************/
/*  GDISPLAY_DELETE  */

static Argument *
gdisplay_delete_invoker (Argument *args)
{
  GDisplay *gdisplay;

  success = TRUE;

  int_value = args[0].value.pdb_int;
  if ((gdisplay = gdisplay_get_ID (int_value)))
    gtk_widget_destroy (gdisplay->shell);
  else
    success = FALSE;

  return procedural_db_return_args (&gdisplay_delete_proc, success);
}

/*  The procedure definition  */
ProcArg gdisplay_delete_args[] =
{
  { PDB_DISPLAY,
    "display",
    "The display"
  }
};

ProcRecord gdisplay_delete_proc =
{
  "gimp_display_delete",
  "Delete the specified display",
  "This procedure removes the specified display.  If this is the last remaining display for the underlying image, then the image is deleted also.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_delete_invoker } },
};

/*********************/
/*  GDISPLAYS_DELETE_IMAGE  */

static Argument *
gdisplays_delete_image_invoker (Argument *args)
{
  success = TRUE;

  int_value = args[0].value.pdb_int;
  gdisplays_delete_image (gimage_get_ID(int_value)); 

  return procedural_db_return_args (&gdisplays_delete_image_proc, success);
}

/*  The procedure definition  */
ProcArg gdisplays_delete_image_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcRecord gdisplays_delete_image_proc =
{
  "gimp_displays_delete_image",
  "Delete all the displays for this image",
  "This procedure removes the specified image  When the last display is removed the image is deleted.",
  "Calvin Williamson",
  "Calvin Williamson",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplays_delete_image_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplays_delete_image_invoker } },
};

/*********************/
/*  GDISPLAYS_FLUSH  */

static Argument *
gdisplays_flush_invoker (Argument *args)
{
  success = TRUE;
  gdisplays_flush ();

  return procedural_db_return_args (&gdisplays_flush_proc, success);
}

ProcRecord gdisplays_flush_proc =
{
  "gimp_displays_flush",
  "Flush all internal changes to the user interface",
  "This procedure takes no arguments and returns nothing except a success status.  Its purpose is to flush all pending updates of image manipulations to the user interface.  It should be called whenever appropriate.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplays_flush_invoker } },
};


/*****************************/
/*  GDISPLAY_GET_IMAGE_ID  */

static Argument *
gdisplay_get_image_id_invoker (Argument *args)
{
  GDisplay *gdisp;

  gdisp = NULL;

  success = TRUE;
  if (success)
  {
    int_value  = args[0].value.pdb_int;
    if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
      success = FALSE;
  }

  return_args = procedural_db_return_args(&gdisplay_get_image_id_proc, success);


  if (success)
    return_args[1].value.pdb_int = gdisp->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_get_image_id_in_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the display"
  }
};

ProcArg gdisplay_get_image_id_out_args[] =
{
  { PDB_INT32,
    "image_ID",
    "the image"
  }
};

ProcRecord gdisplay_get_image_id_proc =
{
  "gimp_display_get_image_id",
  "Gives the apropriate image_ID for a given display_ID",
  "Gives the apropriate image_ID for a given display_ID",
  "Kai-Uwe Behrmann",
  "Kai-Uwe Behrmann",
  "2007",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_get_image_id_in_args,

  /*  Output arguments  */
  1,
  gdisplay_get_image_id_out_args,

  /*  Exec method  */
  { { gdisplay_get_image_id_invoker } },
};




/*****************************/

/*****************************/
/*  GDISPLAY_GET_CMS_INTENT  */

static Argument *
gdisplay_get_cms_intent_invoker (Argument *args)
{
  GDisplay *gdisp;
  gint type = 0;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value  = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;

      type       = args[1].value.pdb_int;
    }

  return_args = procedural_db_return_args (&gdisplay_get_cms_intent_proc, success);


  if (success)
    switch(type)
      {
      case 0: /* Image Profile */
            return_args[1].value.pdb_int = gdisp->cms_intent;
            break;
      case 1: /* Proof Profile */
            return_args[1].value.pdb_int = gdisplay_has_paper_simulation(gdisp)?
                INTENT_ABSOLUTE_COLORIMETRIC : INTENT_RELATIVE_COLORIMETRIC;
            break;
      }

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_get_cms_intent_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the display"
  },
  { PDB_INT32,
    "type",
    "The type of the Profile. 0 = Image Profile / 1 = Proof Profile"
  }
};

ProcArg gdisplay_get_cms_intent_out_args[] =
{
  { PDB_INT32,
    "intent",
    "the lcms rendering intent"
  }
};

ProcRecord gdisplay_get_cms_intent_proc =
{
  "gimp_display_get_cms_intent",
  "Returns the rendering intent for the specified display",
  "Returns the rendering intent for the specified display",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_get_cms_intent_args,

  /*  Output arguments  */
  1,
  gdisplay_get_cms_intent_out_args,

  /*  Exec method  */
  { { gdisplay_get_cms_intent_invoker } },
};



/*****************************/
/*  GDISPLAY_SET_CMS_INTENT  */

static Argument *
gdisplay_set_cms_intent_invoker (Argument *args)
{
  GDisplay *gdisp;
  int old_intent;
  gint type = 0;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value  = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;
      old_intent = gdisp->cms_intent;
      type       = args[2].value.pdb_int;
    }

  if (success)
    switch(type)
      {
      case 0: /* Image Profile */
            gdisplay_set_cms_intent (gdisp, args[1].value.pdb_int);
            break;
      case 1: /* Proof Profile */
            gdisplay_set_paper_simulation (gdisp,
            (args[1].value.pdb_int & INTENT_ABSOLUTE_COLORIMETRIC) ?
             TRUE : FALSE);
            break;
      }

  return_args = procedural_db_return_args (&gdisplay_set_cms_intent_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_set_cms_intent_args[] =
{
    { PDB_DISPLAY,
      "display",
      "the display"
    },
    { PDB_INT32,
      "intent",
      "the new rendering intent"
    },
  { PDB_INT32,
    "type",
    "The type of the Profile. 0 = Image Profile / 1 = Proof Profile"
  }

};

ProcRecord gdisplay_set_cms_intent_proc =
{
  "gimp_display_set_cms_intent",
  "Sets the rendering intent for the specified display and profile.",
  "Sets the rendering intent for the specified display and profile.",
  "Stefan Klein, Kai-Uwe Behrmann",
  "Stefan Klein, Kai-Uwe Behrmann",
  "2004-2006",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gdisplay_set_cms_intent_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_set_cms_intent_invoker } },
};



/****************************/
/*  GDISPLAY_GET_CMS_FLAGS  */

static Argument *
gdisplay_get_cms_flags_invoker (Argument *args)
{
  GDisplay *gdisp;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gdisplay_get_cms_flags_proc, success);


  if (success)
    return_args[1].value.pdb_int = gdisp->cms_flags;

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_get_cms_flags_args[] =
{
    { PDB_DISPLAY,
      "display",
      "the display"
    }

};

ProcArg gdisplay_get_cms_flags_out_args[] =
{
  { PDB_INT32,
    "flags",
    "the cms flags"
  }

};

ProcRecord gdisplay_get_cms_flags_proc =
{
  "gimp_display_get_cms_flags",
  "Returns the lcms flags of the specified display",
  "Returns the lcms flags of the specified display",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_get_cms_flags_args,

  /*  Output arguments  */
  1,
  gdisplay_get_cms_flags_out_args,

  /*  Exec method  */
  { { gdisplay_get_cms_flags_invoker } },
};



/****************************/
/*  GDISPLAY_SET_CMS_FLAGS  */

static Argument *
gdisplay_set_cms_flags_invoker (Argument *args)
{
  GDisplay *gdisp;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gdisplay_set_cms_flags (gdisp, args[1].value.pdb_int);

  return_args = procedural_db_return_args (&gdisplay_set_cms_flags_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_set_cms_flags_args[] =
{
    { PDB_DISPLAY,
      "display",
      "the display"
    },
    { PDB_INT32,
      "flags",
      "the cms flags"
    },
};

ProcRecord gdisplay_set_cms_flags_proc =
{
  "gimp_display_set_cms_flags",
  "Returns cms flags for the specified display",
  "Returns cms flags for the specified display.",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_set_cms_flags_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_set_cms_flags_invoker } },
};



/******************************/
/*  GDISPLAY_IS_COLORMANAGED  */

static Argument *
gdisplay_is_colormanaged_invoker (Argument *args)
{
  GDisplay *gdisp;
  gint type = 0;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;
      type       = args[1].value.pdb_int;
    }

  return_args = procedural_db_return_args (&gdisplay_is_colormanaged_proc, success);


  if (success)
    switch(type)
      {
      case 0: /* Image Profile */
            return_args[1].value.pdb_int = gdisp->colormanaged;
            break;
      case 1: /* Proof Profile */
            return_args[1].value.pdb_int = gdisplay_is_proofed (gdisp);
            break;
      }

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_is_colormanaged_args[] =
{
    { PDB_DISPLAY,
      "display",
      "the display"
    },
  { PDB_INT32,
    "type",
    "The type of the Profile. 0 = Image Profile / 1 = Proof Profile"
  }

};

ProcArg gdisplay_is_colormanaged_out_args[] =
{
  { PDB_INT32,
    "bool",
    "colormanaged"
  }

};

ProcRecord gdisplay_is_colormanaged_proc =
{
  "gimp_display_is_colormanaged",
  "Returns true if the specified display is colormanaged.",
  "Returns true if the specified display is colormanaged.",
  "Stefan Klein, Kai-Uwe Behrmann",
  "Stefan Klein, Kai-Uwe Behrmann",
  "2004-2006",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_is_colormanaged_args,

  /*  Output arguments  */
  1,
  gdisplay_is_colormanaged_out_args,

  /*  Exec method  */
  { { gdisplay_is_colormanaged_invoker } },
};



/*******************************/
/*  GDISPLAY_SET_COLORMANAGED  */

static Argument *
gdisplay_set_colormanaged_invoker (Argument *args)
{
  GDisplay *gdisp;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gdisp = gdisplay_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gdisplay_set_colormanaged (gdisp, args[1].value.pdb_int);

  return_args = procedural_db_return_args (&gdisplay_set_colormanaged_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_set_colormanaged_args[] =
{
    { PDB_DISPLAY,
      "display",
      "the display"
    },
    { PDB_INT32,
      "bool",
      "colormanaged"
    }
};

ProcRecord gdisplay_set_colormanaged_proc =
{
  "gimp_display_set_colormanaged",
  "Sets if the specified display shall appear colormanaged or not.",
  "Sets if the specified display shall appear colormanaged or not.",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_set_colormanaged_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_set_colormanaged_invoker } },
};



/***********************************/
/*  GDISPLAY_ALL_SET_COLORMANAGED  */

static Argument *
gdisplay_all_set_colormanaged_invoker (Argument *args)
{
  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      gdisplay_all_set_colormanaged (int_value);
    }

  return_args = procedural_db_return_args (&gdisplay_all_set_colormanaged_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_all_set_colormanaged_args[] =
{
  { PDB_IMAGE,
    "bool",
    "colormanaged"
  }
};

ProcRecord gdisplay_all_set_colormanaged_proc =
{
  "gimp_display_all_set_colormanaged",
  "Sets all display colormanaged or not",
  "",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_all_set_colormanaged_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_all_set_colormanaged_invoker } },
};



/******************/
/*  GDISPLAY_IMAGE_SET_COLORMANAGED  */

static Argument *
gdisplay_image_set_colormanaged_invoker (Argument *args)
{
  GImage *gimage;
  int set_colormanaged = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
      set_colormanaged = args[1].value.pdb_int;
    }

  if (success)
    gdisplay_image_set_colormanaged (int_value, set_colormanaged);

  return_args = procedural_db_return_args (&gdisplay_image_set_colormanaged_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_image_set_colormanaged_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "bool",
    "colormanaged"
  }
};

ProcRecord gdisplay_image_set_colormanaged_proc =
{
  "gimp_display_image_set_colormanaged",
  "Sets an display colormanaged for the specified image",
  "",
  "Stefan Klein",
  "Stefan Klein",
  "2004",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gdisplay_image_set_colormanaged_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_image_set_colormanaged_invoker } },
};




