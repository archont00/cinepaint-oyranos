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
#ifndef __BFP_H__
#define __BFP_H__

#include <glib.h>

#define ONE_BFP 32768
#define BFP_MAX_IN_FLOAT 65535/(gfloat)32768
#define BFP_MAX 65535

#define GAMMUT_ALARM_R 255
#define GAMMUT_ALARM_G 0
#define GAMMUT_ALARM_B 0

#define BFP_TO_U8(x) (x < ONE_BFP ? x >> 7 : 255)

#define BFP_TO_U16(x) (x < ONE_BFP ? x << 1 : 65535)
#define BFP_TO_FLOAT(x) (x / (gfloat) ONE_BFP)
#define FLOAT_TO_BFP(x) (x * ONE_BFP)

#endif
