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
#ifndef __PALETTE_H__
#define __PALETTE_H__

#include <glib.h>
#include "procedural_db.h"

/* the two basic colors */
#define FOREGROUND 0
#define BACKGROUND 1

/*  The states for updating a color in the palette via palette_set_* calls */
#define COLOR_NEW         0
#define COLOR_UPDATE_NEW  1
#define COLOR_UPDATE      2

/* handy macros for dealing with pixelrows */
#define COLOR16_NEWDATA(name, type, prec, format, alpha) \
        PixelRow name; \
        Tag name##_tag = tag_new (prec, format, alpha); \
        type name##_data[TAG_MAX_BYTES/sizeof(type)]

#define COLOR16_NEW(name, tag) \
        PixelRow name; \
        Tag name##_tag = tag; \
        guchar name##_data[TAG_MAX_BYTES]

#define COLOR16_INIT(name) \
        pixelrow_init (&name, name##_tag, (guchar *)name##_data, 1)

          
void palettes_init (int);
void palettes_free (void);
void palette_create (void);
void palette_free (void);

void palette_get_foreground  (PixelRow *);
void palette_get_background  (PixelRow *);
void palette_get_transparent (PixelRow *);
void palette_get_white       (PixelRow *);
void palette_get_black       (PixelRow *);

void palette_set_foreground  (PixelRow *);
void palette_set_background  (PixelRow *);

void palette_set_active_color (PixelRow *, int);
void palette_set_default_colors (void);
void palette_swap_colors (void);


struct PaletteEntries {
  char *name;
  char *filename;
  GSList *colors;
  int n_colors;
  int changed;
};
typedef struct PaletteEntries PaletteEntries, *PaletteEntriesP;

struct PaletteEntry {
  gfloat color[3];
  char *name;
  int position;
};
typedef struct PaletteEntry PaletteEntry, *PaletteEntryP;

extern GSList * palette_entries_list;
void palette_init_palettes (int);
void palette_free_palettes (void);

/*  Procedure definition and marshalling function  */
extern ProcRecord palette_get_foreground_proc;
extern ProcRecord palette_get_background_proc;
extern ProcRecord palette_set_foreground_proc;
extern ProcRecord palette_set_background_proc;
extern ProcRecord palette_set_default_colors_proc;
extern ProcRecord palette_swap_colors_proc;
extern ProcRecord palette_refresh_proc;

#endif /* __PALETTE_H__ */
