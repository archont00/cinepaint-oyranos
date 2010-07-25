/*  
 * BASE FRAME MANAGER
 *
  
  CHANGE-LOG (started 2006/03)
  ============================
  2006/03  -hsbo [hartmut.sbosny@gmx.de]
   - brief function descriptions (started)
   - clearing and renaming:
      -> bfm_get_fg_ID()     (the former bfm_get_fg_display())
      -> bfm_get_bg_ID()     (new)
      -> bfm_getset_bg_ID()  (the former bfm_get_bg_display())
   - bfm_check() has become public
*/ 

#include "base_frame_manager.h"
#include "store_frame_manager.h"
#include "clone.h"
#include "tools.h"
#include <stdio.h>
#include "general.h"
#include <string.h>
#include "layer_pvt.h"
#include "../lib/wire/iodebug.h"

static int success; 

/** @return 1, if display has bfm, 0 else. NULL-arg allowed. */
/*static*/ char
bfm_check (GDisplay *disp)
{
  if (!disp)
    return 0;
  if (disp->bfm)
    return 1;
  return 0;
}

/** @return 1, if display has an image, 0 else */
static char
bfm_check_disp (GDisplay *disp)
{
  if (!disp)
    return 0;
  if (!disp->gimage)
    return 0;
  return 1;             

  if (!disp->gimage->filename)  /* ?? - unreachable */
    return 0;
  return 1;
}

/*
 * CREATE & DELETE
 */
typedef void (*CheckCallback) (GtkWidget *, gpointer);

static void
bfm_init_varibles (GDisplay *disp)
{
  disp->bfm->onionskin_b = 0;
  disp->bfm->fg = 0;
  disp->bfm->bg = 0;
}

void
bfm_create_sfm (GDisplay *disp)
{
  double scale;
  
  if(!bfm_check_disp (disp))
    return;

  if (disp->bfm)
    {
      g_message("Only one Flipbook per display is allowed.");
      return;
    }
  
  scale = ((double) SCALESRC (disp) / (double)SCALEDEST (disp));

  disp->bfm = (base_frame_manager*) g_malloc_zero (sizeof(base_frame_manager)); 
  disp->bfm->sfm = (store_frame_manager*) g_malloc_zero (sizeof(store_frame_manager));
  disp->bfm->cfm = 0;
  bfm_init_varibles (disp);

  /* GUI */
  sfm_create_gui (disp);
}

void
bfm_create_cfm (GDisplay *disp)
{
}

void 
bfm_delete_sfm (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
}

void 
bfm_delete_cfm (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
}

  
void 
bfm_dirty (GDisplay *disp)
{
  if (!bfm_check (disp))
    return;
  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_set_offset (disp->bfm->cfm, x, y);*/
    }
  else
    if(disp->bfm->sfm)
      sfm_dirty (disp, disp->bfm->sfm->fg);
}

/** @return 1, if `disp' has bfm, 0 else. NULL-arg not allowed! */
int
bfm (GDisplay *disp)
{
  if (disp->bfm)
    return 1;
  return 0; 
}

/*
 * ONIONSKINNIG
 */

/** @return 1 if `disp' has bfm and onionskin is on, 0 else */
char 
bfm_onionskin (GDisplay *disp)
{
  if (!bfm_check (disp))
    return 0;
  return disp->bfm->onionskin_b;
}

void 
bfm_onionskin_set_offset (GDisplay *disp, int x, int y)
{
  if (!bfm_check (disp)  || !disp->bfm->onionskin_b)
    return;
  
  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_set_offset (disp->bfm->cfm, x, y);*/
    }
  else
    if(disp->bfm->sfm)
      sfm_onionskin_set_offset (disp, x, y);
}

void 
bfm_onionskin_rm (GDisplay *disp)
{
  if (!bfm_check (disp) || !disp->bfm->onionskin_b)
    return;

  if(disp->bfm->cfm)
    {
      /*cfm_onionskin_rm (disp->bfm->cfm);*/
    }
  else
    if(disp->bfm->sfm && disp->bfm->onionskin_b)
      {
        disp->bfm->sfm->old_offset_x = GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_x;
        disp->bfm->sfm->old_offset_y = GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_y;
        disp->bfm->sfm->old_ex = disp->bfm->sfm->ex;
        disp->bfm->sfm->old_ey = disp->bfm->sfm->ey;
        disp->bfm->sfm->old_opacity = disp->bfm->fg->active_layer->opacity;

	disp->bfm->fg->active_layer->opacity = 1;
	gimage_remove_layer (disp->bfm->fg, (disp->bfm->bg->active_layer));

	GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_x = 0;
	GIMP_DRAWABLE (disp->bfm->bg->active_layer)->offset_y = 0;
	GIMP_DRAWABLE(disp->bfm->bg->active_layer)->gimage_ID = disp->bfm->bg->ID;
	disp->bfm->onionskin_b = 0;
	sfm_onionskin_rm (disp);

      }
}

/** @return fg gimage if `disp' has bfm, NULL else */
GImage* 
bfm_get_fg (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return disp->bfm->fg;
}

/** @return bg gimage (can be NULL) if `disp' has bfm, NULL else */
GImage* 
bfm_get_bg (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return disp->bfm->bg;
}

/** @return fg drawable if `disp' has bfm, NULL else */
CanvasDrawable* 
bfm_get_fg_drawable (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  return GIMP_DRAWABLE (disp->bfm->fg->active_layer); 
}                               /* relies on fg!=NULL */

/** @return bg drawable if `disp' has bfm, NULL else */
CanvasDrawable* 
bfm_get_bg_drawable (GDisplay *disp)
{
  if (!bfm_check (disp))
    return NULL;
  if (!disp->bfm->bg) 
    return NULL;  
  return GIMP_DRAWABLE (disp->bfm->bg->active_layer);
}

/** @return fg imageID if `disp' has bfm, -1 else */
int 
bfm_get_fg_ID (GDisplay *disp)
{
   if (!bfm_check (disp))
        return -1;
   return disp->bfm->fg->ID;    /* relies on fg!=NULL */
}

/** @return bg imageID (can be -1) if `disp' has bfm, -1 else */
int 
bfm_get_bg_ID (GDisplay *disp)
{
   if (!bfm_check (disp))
        return -1;
   return disp->bfm->bg ? disp->bfm->bg->ID : -1;
}

/** If `disp' has bfm and bg not set, set bg=fg and update sfm.
    @return [old/new] bg imageID, if `disp' has bfm, -1 else. */
int 
bfm_getset_bg_ID (GDisplay *disp)
{
   if (!bfm_check (disp))
        return -1;
   if (!disp->bfm->bg)
   {
     disp->bfm->bg = disp->bfm->fg;
     sfm_set_bg (disp, -1);          /* unsets bg in sfm - if no bg was set in bfm??? */
     //sfm_set_bg (disp, sfm_pos_of_ID(disp, disp->bfm->bg->ID)); // disp->bfm->bg==NULL!
   }
   return disp->bfm->bg->ID;
}

/** If `disp' has bfm, set bfm->fg to fg */
void
bfm_set_fg (GDisplay *disp, GImage *fg)
{
   if (!bfm_check (disp))
        return;

   disp->bfm->fg = fg;
}

/** If `disp' has bfm, set bfm->bg to bg */
void
bfm_set_bg (GDisplay *disp, GImage *bg)
{
  if (!bfm_check (disp))
        return;

  disp->bfm->bg = bg;
}

void
bfm_onionskin_unset (GDisplay *disp)
{
  disp->bfm->onionskin_b = 0; 
}

void 
bfm_onionskin_display (GDisplay *disp, double val, int sx, int sy, int ex, int ey)
{
  Layer *layer;
  int x=0, y=0;

  if (!disp->bfm->fg || !disp->bfm->bg)
    return;

  disp->bfm->fg->active_layer->opacity = val;

  if (!disp->bfm->onionskin_b)
    {
      d_printf ("add layer \n");
      if (active_tool->type == CLONE)
	{
	  x = clone_get_x_offset ();
	  y = clone_get_y_offset ();
	}
      layer = disp->bfm->fg->active_layer;
      if (!gimage_add_layer2 (disp->bfm->fg, disp->bfm->bg->active_layer,
	  1, sx, sy, ex, ey))
	      {
	    sfm_onionskin_rm (disp);  
	      return;
	      }
      gimage_set_active_layer (disp->bfm->fg, layer);
      sfm_onionskin_set_offset (disp, x, y);
      disp->bfm->onionskin_b = 1;
    }

  drawable_update (GIMP_DRAWABLE(disp->bfm->fg->active_layer),
      sx, sy, ex, ey);
  gdisplays_flush ();

}

/*
 * FILE STUFF
 */

void
bfm_this_filename (char *filepath, char *filename, char *whole, char *raw, int frame)
{
  char *name, *num, *ext;
  char frame_num[10];
  int old_l, cur_l;

  name  = strtok (strdup(filename), ".");
  num = strtok (NULL, ".");
  ext = strtok (NULL, ".");

  sprintf (frame_num, "%d", frame);

  old_l = strlen (num);
  cur_l = strlen (frame_num);

  switch (old_l-cur_l)
    {
    case 0:
      break;
    case 1:
      sprintf (frame_num, "0%d", frame);
      break;
    case 2:
      sprintf (frame_num, "00%d", frame);
      break;
    case 3:
      sprintf (frame_num, "000%d", frame);
      break;
    case 4:
      sprintf (frame_num, "0000%d", frame);
      break;
    default:
      break; 
    }
  sprintf (raw, "%s.%s.%s", name, frame_num, ext);

  sprintf (whole, "%s/%s", filepath, raw);

}

char*
bfm_get_name (char *filename)
{

  char raw[256], whole[256];
  char *tmp;

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  tmp = strdup (strtok (raw, "."));

  return tmp;

}

char*
bfm_get_frame (char *filename)
{

  char raw[256], whole[256];

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");

  return strdup (strtok (NULL, "."));

}

char*
bfm_get_ext (char *filename)
{

  char raw[256], whole[256];

  strcpy (whole, filename);
  strcpy (raw, prune_filename (whole));

  strtok (raw, ".");
  strtok (NULL, ".");
  return strdup (strtok (NULL, "."));

}

GDisplay* 
bfm_load_image_into_fm (GDisplay *disp, GImage *img)
{
  if (!bfm_check (disp))
    return disp;
  if (disp->bfm->sfm)
    return sfm_load_image_into_fm (disp, img);
  return disp; 
}

/*
 * PLUG-IN STUFF
 */

void 
bfm_set_dir_dest (GDisplay *disp, char *filename)
{
  GString *pathtmp;
  if (!bfm_check (disp))
    return;
  #if 0
  disp->bfm->dest_dir = strdup (prune_pathname(filename));
  #else
  /* Rob Williams */
  if (strstr(filename, "cp_out"))
  /* we are already in the output dir */
  {
    disp->bfm->dest_dir = strdup (prune_pathname(filename));
  }
  else
  {
    pathtmp = g_string_new (prune_pathname(filename));
    g_string_append(pathtmp, "cp_out/");
  /* error conditions: already exists == good, cannot create == not our problem. */
    mkdir (pathtmp->str, 00777);
    disp->bfm->dest_dir = strdup (pathtmp->str);
  }
  #endif
  disp->bfm->dest_name = strdup (prune_filename(filename));

  if (disp->bfm->sfm)
    sfm_set_dir_dest (disp);
}

static Argument *
bfm_set_dir_dest_invoker (Argument *args)
{
  GDisplay *disp; 
  gint int_value;

  success = TRUE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (!(disp = gdisplay_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    bfm_set_dir_dest (disp, (char *) args[1].value.pdb_pointer);

  /*  create the new image  */
  return  procedural_db_return_args (&bfm_set_dir_dest_proc, success);

}

/*  The procedure definition  */
ProcArg bfm_set_dir_args[] =
{
  { PDB_DISPLAY, "display",  "The display" },
  { PDB_STRING,  "filename", "The name of the file to load." }
};

ProcArg bfm_set_dir_out_args[] =
{
  PDB_END
};
/* rower: added PDB_END to make compile in vc++ */

ProcRecord bfm_set_dir_dest_proc =
{
  "gimp_bfm_set_dir_dest",
  "Get a fileset from the user",
  "Get a fileset from the user",
  "Caroline Dahllof",
  "Caroline Dahllof",
  "2002",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  bfm_set_dir_args,

  /*  Output arguments  */
  0,
  bfm_set_dir_out_args,

  /*  Exec method  */
    { { bfm_set_dir_dest_invoker } },
};


void 
bfm_set_dir_src (GDisplay *disp, char *filename)
{
  if (!bfm_check (disp))
    return;

  disp->bfm->src_dir = strdup (prune_pathname(filename));
  disp->bfm->src_name = strdup (prune_filename(filename));

  if (disp->bfm->sfm)
    sfm_set_dir_src (disp);
}

static Argument *
bfm_set_dir_src_invoker (Argument *args)
{
  GDisplay *disp; 
  gint int_value;

  success = TRUE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (!(disp = gdisplay_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    bfm_set_dir_src (disp, (char *) args[1].value.pdb_pointer);

  /*  create the new image  */
  return  procedural_db_return_args (&bfm_set_dir_src_proc, success);

}

/*  The procedure definition  */

ProcRecord bfm_set_dir_src_proc =
{
  "gimp_bfm_set_dir_src",
  "Get a fileset from the user",
  "Get a fileset from the user",
  "Caroline Dahllof",
  "Caroline Dahllof",
  "2002",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  bfm_set_dir_args,

  /*  Output arguments  */
  0,
  bfm_set_dir_out_args,

  /*  Exec method  */
    { { bfm_set_dir_src_invoker } },
};

