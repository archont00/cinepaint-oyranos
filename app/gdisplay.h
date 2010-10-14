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
#ifndef __GDISPLAY_H__
#define __GDISPLAY_H__

#include "gimage.h"
#include "info_dialog.h"
#include "look_profile.h"
#include "selection.h"

/*
 *  Global variables
 *
 */

/*  some useful macros  */

#define  SCALESRC(g)    (g->scale & 0x00ff)
#define  SCALEDEST(g)   (g->scale >> 8)
#define  SCALE(g,x)     ((x * SCALEDEST(g)) / SCALESRC(g))
#define  UNSCALE(g,x)   ((x * SCALESRC(g)) / SCALEDEST(g))

#define LOWPASS(x) ((x>0) ? x : 0)
#define HIGHPASS(x,y) ((x>y) ? y : x)


typedef enum
{
  SelectionOff,
  SelectionLayerOff,
  SelectionOn,
  SelectionPause,
  SelectionResume
} SelectionControl;

typedef struct frame_manager_t frame_manager_t;
typedef struct base_frame_manager base_frame_manager;

typedef struct GDisplay GDisplay;
struct GDisplay
{
  int ID;                         /**< @brief ID of the image referenced by this
				   *  display.  *Not unique*, since many
				   *  displaus can reference the same image.  */
  int unique_id;                  /**<@brief A *unique* id for this display   */

  GtkWidget *shell;               /**<@brief shell widget for this gdisplay  */
  GtkWidget *canvas;              /**<@brief canvas widget for this gdisplay */
  GtkWidget *hsb, *vsb;           /**<@brief widgets for scroll bars         */
  GtkWidget *hrule, *vrule;       /**<@brief widgets for rulers              */
  GtkWidget *origin;              /**<@brief widgets for rulers              */
  GtkWidget *popup;               /**<@brief widget for popup menu           */
  GtkWidget *menubar;             /**<@brief widget for menubar across the top*/
  GtkItemFactory *menubar_fac;    /**<@brief Factory for menubar             */
 
  GtkWidget *statusarea;          /**<@brief hbox holding the statusbar and stuff  */
  GtkWidget *statusbar;           /**<@brief widget for statusbar		      */

  InfoDialog *window_info_dialog; /**<@brief dialog box for image information */

  GtkAdjustment *hsbdata;         /**<@brief horizontal data information */
  GtkAdjustment *vsbdata;         /**<@brief vertical data information */

  GImage *gimage;	          /**<@brief pointer to the associated gimage struct */
  int instance;                   /**<@brief  the instance # of this gdisplay as    */
                                  /**<@brief taken from the gimage at creation*/

  int depth;   		          /**<@brief  depth of our drawables */ 
  int disp_width;                 /**<@brief  width of drawing area in the window   */
  int disp_height;                /**<@brief  height of drawing area in the window  */
  int disp_xoffset;
  int disp_yoffset;

  int old_disp_geometry[4];

  int offset_x, offset_y;         /**<@brief  offset of display image into raw image*/
  int scale;        	          /**<@brief  scale factor from original raw image  */

/* from gimp 1.2.4 */
  gboolean dot_for_dot;		  /**<@brief  is monitor resolution being ignored?*/

  short draw_guides;              /**<@brief  should the guides be drawn?     */
  short snap_to_guides;           /**<@brief  should the guides be snapped to?*/

  Selection *select;              /**<@brief  Selection object                */

  GdkGC *scroll_gc;               /**<@brief  GC for scrolling */

  GSList *update_areas;           /**<@brief  Update areas list               */
  GSList *display_areas;          /**<@brief  Display areas list              */

  GdkCursorType current_cursor;   /**<@brief  Currently installed cursor      */

 
   short draw_cursor;		  /**<@brief should we draw software cursor ? */
   int cursor_x;			  /**<@brief software cursor X value */
   int cursor_y;			  /**<@brief software cursor Y value */
   short proximity;		  /**<@brief is a device in proximity of gdisplay ? */
   short have_cursor;		  /**<@brief is cursor currently drawn ? */

  char framemanager;		  /**<@brief is one if the display has a frame_manager
				     associated with it */

  frame_manager_t *frame_manager; /**<@brief link to the frame manager */ 

  gint slide_show;		  /**<@brief mode of GDisplay 0=Normal 1=Slide Show */
  
  base_frame_manager *bfm; 	  /**<@brief link to the frame manager */ 
  LookProfileManager *look_profile_manager;   /* the look profile manager */

  float    expose;                 /**<@brief expose float HDR */ 
  float    offset;                 /**<@brief offset float HDR */ 
  float    gamma;                  /**<@brief gamma for float HDR */
  
  gboolean colormanaged;           /**<@brief is the display being colourmanaged ? */
  DWORD    cms_flags;              /**<@brief littlecms flags for display cms */
  int      cms_proof_flags;        /**<@brief simulate paper on proofing */
  int      cms_intent;             /**<@brief littlecms rendering intent */
  GSList  *look_profile_pipe;      /**<@brief the pipeline of look profiles 
                                              gimage data gets sent through */
  CMSTransform **cms_expensive_transf; /**<@brief expensive CMS data per disp */
  
};

extern GSList *display_list;


/* member function declarations */

GDisplay * gdisplay_new                    (GImage *, unsigned int);
GDisplay * gdisplay_fm                     (GImage *, unsigned int, GDisplay*);
void       gdisplay_remove_and_delete      (GDisplay *);
gfloat     gdisplay_mask_value             (GDisplay *, int, int);
int        gdisplay_mask_bounds            (GDisplay *, int *, int *, int *, int *);
void       gdisplay_transform_coords       (GDisplay *, int, int, int *, int *, int);
void       gdisplay_untransform_coords     (GDisplay *, int, int, int *,
					    int *, int, int);
void       gdisplay_transform_coords_f     (GDisplay *, double, double, double *,
					    double *, int);
void       gdisplay_untransform_coords_f   (GDisplay *, double, double, double *,
					    double *, int);
void       gdisplay_install_tool_cursor    (GDisplay *, GdkCursorType);
void       gdisplay_remove_tool_cursor     (GDisplay *);
void       gdisplay_set_menu_sensitivity   (GDisplay *);
void       gdisplay_expose_area            (GDisplay *, int, int, int, int);
void       gdisplay_expose_guide           (GDisplay *, Guide *);
void       gdisplay_update_full            (GDisplay *);
void       gdisplay_expose_full            (GDisplay *);
void       gdisplay_flush                  (GDisplay *);
void       gdisplay_draw_guides            (GDisplay *);
void       gdisplay_draw_guide             (GDisplay *, Guide *, int);
Guide*     gdisplay_find_guide             (GDisplay *, int, int);
void       gdisplay_snap_point             (GDisplay *, double , double, double *, double *);
void       gdisplay_snap_rectangle         (GDisplay *, int, int, int, int, int *, int *);
void	   gdisplay_update_cursor	   (GDisplay *, int, int);

/*  function declarations  */

GDisplay * gdisplay_get_from_gimage        (GImage *image);
GDisplay * gdisplay_active                 (void);
GDisplay * gdisplay_get_ID                 (int);
GDisplay * gdisplay_get_unique_id          (int);
void       gdisplays_update_title          (int);
void       gdisplay_add_update_area  (GDisplay *, int, int, int, int);
void       gdisplays_update_area           (int, int, int, int, int);
void       gdisplays_expose_guides         (int);
void       gdisplays_expose_guide          (int, Guide *);
void       gdisplays_shrink_wrap           (int);
/* complete update of all gdisplays showing gimage with given id */
void       gdisplays_update_gimage         (int);
/* complete update of all gdisplays */
void       gdisplays_update_full           (void);
/* complete exposure of all gdisplays */
void       gdisplays_expose_full           (void);
void       gdisplays_selection_visibility  (int, SelectionControl);
int        gdisplays_dirty                 (void);
void       gdisplays_delete                (void);
void 	   gdisplays_delete_image 	   (GImage *image);
void       gdisplays_flush                 (void);

GDisplay* gdisplay_find_display (int ID);
int        gdisplay_to_ID                  (GDisplay *);

/* color management getter/setter methods */
#define    gdisplay_get_cms_intent(display) (display)->cms_intent
void       gdisplay_set_cms_intent          (GDisplay *, int);
#define    gdisplay_get_cms_flags(display)  (display)->cms_flags
void       gdisplay_set_cms_flags           (GDisplay *, DWORD);
#define    gdisplay_is_colormanaged(display)(display)->colormanaged
#define    gdisplay_has_bpc(display)(((display)->cms_flags & cmsFLAGS_WHITEBLACKCOMPENSATION)?1:0)
#define    gdisplay_has_paper_simulation(display)(((display)->cms_proof_flags == INTENT_ABSOLUTE_COLORIMETRIC) ? INTENT_ABSOLUTE_COLORIMETRIC:0)
#define    gdisplay_get_cms_proof_intent(display)((display)->cms_proof_flags)
#define    gdisplay_is_proofed(display)(((display)->cms_flags & cmsFLAGS_SOFTPROOFING)?1:0)
#define    gdisplay_is_gamutchecked(display)(((display)->cms_flags & cmsFLAGS_GAMUTCHECK)?1:0)
gboolean   gdisplay_set_colormanaged        (GDisplay *, gboolean);
gboolean   gdisplay_set_bpc                 (GDisplay *, gboolean);
gboolean   gdisplay_set_paper_simulation    (GDisplay *, gboolean);
gboolean   gdisplay_set_proofed             (GDisplay *, gboolean);
gboolean   gdisplay_set_gamutchecked        (GDisplay *, gboolean);
/* change color management for all displays */
gboolean   gdisplay_all_set_colormanaged    (gboolean);
/* change color management for all displays of a given image */
gboolean   gdisplay_image_set_colormanaged  (int, gboolean);
gboolean   gdisplay_image_set_proofed       (int, gboolean);
gboolean   gdisplay_image_set_gamuchecked   (int, gboolean);
CMSTransform*   gdisplay_cms_transform_get  (GDisplay *,
                                             Tag display_tag );

/* look profiles */
#define    gdisplay_get_look_profile_pipe(display) (display)->look_profile_pipe
void       gdisplay_set_look_profile_pipe   (GDisplay *, GSList *);
void       gdisplay_add_look_profile        (GDisplay *, CMSProfile*, gint index);
void       gdisplay_remove_look_profile     (GDisplay *, CMSProfile*);
#endif /*  __GDISPLAY_H__  */
