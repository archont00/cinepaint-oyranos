/* fg.h - Some stuff that requires Film Gimp's headers
 *
 * Copyright (C) 2003 Sean Ridenour.
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
 *
 * Read the file COPYING for the complete licensing terms.
 */

#ifndef _FG_H_
#define _FG_H_

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

GDrawable *drawable;
GPrecisionType precision;
gint num_channels;
gint32 drawable_ID;
GRunModeType run_mode;

guchar *src_row, *dest_row;
guint8 *srcu8, *destu8;
guint16 *srcu16, *destu16;
guint16 *srcf16, *destf16;
gfloat *srcf32, *destf32;

#endif /* _FG_H_ */
