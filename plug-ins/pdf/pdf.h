/*
 * load of Adobe PDF's;
 *  plug-in for cinepaint.
 *
 * Copyright (C) 2005 Kai-Uwe Behrmann <ku.b@gmx.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * show a small settings dialog, with options for renderer( gs, xpdf ),
 * resolution, rgb/cmyk + default profile
 * open image in CinePaint
 */

#include <string>
#include <sstream>
#include <iostream>
#define cout std::cout
#define endl std::endl

#ifdef DEBUG
#define DBG cout << __FILE__ <<":"<< __LINE__ <<" "<< __func__ << "()\n";
#else
#define DBG
#endif


typedef struct {
  int  ok;
  char filename[1024];
  char name[1024];
  int  interpreter;
  int  viewer;
  int  export_format;
  char command[1024];
  int  resolution;
  int  colourspace;
  int  aa_graphic;
  int  aa_text;
  int  gs_has_tiff32nc;
} Vals;
extern Vals vals;

#define GS_COMMAND_BASE "gs -q -dPARANOIDSAFER -dNOPAUSE -dBATCH"
#define XPDF_COMMAND_BASE "pdftops"

enum { PDF_CMYK, PDF_RGB };
enum { GHOSTSCRIPT, XPDF, ACROREAD };
enum { GS_TIFF, GS_PSD, GS_PPM, GS_PS, GS_PNG48 };
enum { GS_AANONE, GS_AA4, GS_AA4INTERPOL, GS_AA4HINT };

void view_doc ();
