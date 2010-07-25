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
#ifndef  __BRUSH_SELECT_H__
#define  __BRUSH_SELECT_H__

#include "../procedural_db.h"
#include "../buildmenu.h"
#include "../brush.h"

typedef struct BrushSelect BrushSelect, *BrushSelectP;

struct BrushSelect {
  GtkWidget *shell;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *brush_name;
  GtkWidget *brush_size;
  GtkWidget *options_box;
  GtkWidget *noise_options_box;
  GtkAdjustment *opacity_data;
  GtkAdjustment *spacing_data;
  GtkAdjustment *noise_freq_data;
  GtkAdjustment *noise_step_start_data;
  GtkAdjustment *noise_step_width_data;
  GtkAdjustment *sbar_data;
  GtkWidget *edit_button;
  GtkWidget *option_menu;
  int width, height;
  int cell_width, cell_height;
  int scroll_offset;
  int redraw;
  /*  Brush preview  */
  GtkWidget *brush_popup;
  GtkWidget *brush_preview;
  /* Call back function name */
  gchar * callback_name;
  /* current brush */
  GimpBrushP brush; 
  /* Stuff for current selection */
  int old_row;
  int old_col;
  gdouble opacity_value;
  gint spacing_value;
  gdouble noise_freq_value;
  gdouble noise_freq_variation;
  gdouble noise_step_start_value;
  gdouble noise_step_width_value;
  gint paint_mode;
  /* To calc column pos. */
  gint NUM_BRUSH_COLUMNS;
  gint NUM_BRUSH_ROWS;
};

BrushSelectP  brush_select_new     (gchar *, gchar *, gdouble, 
					gint, gdouble, gdouble, gdouble, gint);
void          brush_select_select  (BrushSelectP, int);
void          brush_select_free    (BrushSelectP);
void          brush_change_callbacks (BrushSelectP,gint);
void          brushes_check_dialogs(void);

/*  An interface to other dialogs which need to create a paint mode menu  */
GtkWidget *   create_paint_mode_menu (MenuItemCallback, gpointer);

/* Convert row of brush to 8bit display */
typedef void  (*DisplayBrushGetRowFunc) (guchar *, Canvas *, gint, gint, gint);
DisplayBrushGetRowFunc display_brush_get_row_funcs (Tag t);
void display_brush_get_row_u8     (guchar *, Canvas *, gint, gint, gint);
void display_brush_get_row_u16    (guchar *, Canvas *, gint, gint, gint);
void display_brush_get_row_float  (guchar *, Canvas *, gint, gint, gint);
void display_brush_get_row_float16  (guchar *, Canvas *, gint, gint, gint);
void display_brush_get_row_bfp  (guchar *, Canvas *, gint, gint, gint);

/* PDB entry */
extern ProcRecord brushes_popup_proc;
extern ProcRecord brushes_close_popup_proc;
extern ProcRecord brushes_set_popup_proc;
extern ProcRecord brushes_get_brush_data_proc;

#endif  /*  __BRUSH_SELECT_H__  */
