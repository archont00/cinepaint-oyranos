#!/usr/bin/env python2

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#    This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import math
from gimpfu import *

have_gimp11 = 1 #gimp.major_version > 1 or \
	      #gimp.major_version == 1 and gimp.minor_version >= 1

def python_clothify(timg, tdrawable, bx=9, by=9,
		    azimuth=135, elevation=45, depth=3):
	bx = 9 ; by = 9 ; azimuth = 135 ; elevation = 45 ; depth = 3
	width = tdrawable.width
	height = tdrawable.height
	img = gimp.image(width, height, RGB)
	layer_one = gimp.layer(img, "X Dots", width, height, RGB_IMAGE,
			       100, NORMAL_MODE)
	img.disable_undo()
	if have_gimp11:
		pdb.gimp_edit_fill(layer_one, BG_IMAGE_FILL)
	else:
		pdb.gimp_edit_fill(img, layer_one)
	img.add_layer(layer_one, 0)
	pdb.plug_in_noisify(img, layer_one, 0, 0.7, 0.7, 0.7, 0.7)
	layer_two = layer_one.copy()
	layer_two.mode = MULTIPLY_MODE
	layer_two.name = "Y Dots"
	img.add_layer(layer_two, 0)
	pdb.plug_in_gauss_rle(img, layer_one, bx, 1, 0)
	pdb.plug_in_gauss_rle(img, layer_two, by, 0, 1)
	img.flatten()
	bump_layer = img.active_layer
	pdb.plug_in_c_astretch(img, bump_layer)
	pdb.plug_in_noisify(img, bump_layer, 0, 0.2, 0.2, 0.2, 0.2)
	pdb.plug_in_bump_map(img, tdrawable, bump_layer, azimuth,
			     elevation, depth, 0, 0, 0, 0, TRUE, FALSE, 0)
	gimp.delete(img)

register(
	"python_fu_clothify",
	"Make the specified layer look like it is printed on cloth",
	"Make the specified layer look like it is printed on cloth",
	"James Henstridge",
	"James Henstridge",
	"1997-1999",
	"<Image>/Filter/Alchemy/Clothify",
	"RGB*, GRAY*",
	[
		(PF_INT, "x_blur", "X Blur", 9),
		(PF_INT, "y_blur", "Y Blur", 9),
		(PF_INT, "azimuth", "Azimuth", 135),
		(PF_INT, "elevation", "elevation", 45),
		(PF_INT, "depth", "Depth", 3)
	],
	[],
	python_clothify)

main()
