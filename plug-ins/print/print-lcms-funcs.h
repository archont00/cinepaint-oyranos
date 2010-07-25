/*
 * "$Id: print-lcms-funcs.h,v 1.4 2006/04/08 08:35:52 beku Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 2003-2004   Kai-Uwe Behrmann (ku.b@gmx.de)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/*
 *   This sections is intended to set some options before converting an 
 *   cinepaint image into cmyk 16-bit colormode for gimp-print.
 */

#include <gtk/gtk.h>
#include <lcms.h>
#include "icc_common_funcs.h"

/*** struct definitions ***/

typedef struct
{
  gchar  i_profile[MAX_PATH];         // input profile
  gchar  o_profile[MAX_PATH];         // output profile
  gchar  l_profile[MAX_PATH];         // linearisation profile
  gchar  image_filename [MAX_PATH];         // name of the image
  gchar  tiff_file[MAX_PATH];         // tiff filename

  gint32 intent;            // rendering intent
  gint32 disp_intent;       // displays rendering intent
  gint32 matrix;            // conversion matrix
  gint32 precalc;           // precalculation
  gint32 flags;             // flags
  gint32 disp_flags;        // flags of the active display
  gint32 preserveblack;     // preserve black in cmyk -> cmyk transform
  gint32 bitpersample;      // bit per pixel
  gchar  color_type[64];    // color type
  gint32 display_ID;
  gint32 o_image_ID;        // original image
  gint32 e_image_ID;        // exported image
  gint32 s_image_ID;        // separated image
  gint32 o_drawable_ID;
  gint32 e_drawable_ID;
  gint32 s_drawable_ID;
  gint32 save_tiff;         // bool, print in an tiff file
  gint32 icc;               // bool, use an ICC profile for conversion
  gint32 show_image;        // bool, display the converted image
  gint32 color_space;       // colour space to use for source
  gint32 quit_app;
} LcmsVals;

extern LcmsVals vals;

typedef struct
{
  double   C_Min;
  double   M_Min;
  double   Y_Min;
  double   K_Min;
  double   C_Max;
  double   M_Max;
  double   Y_Max;
  double   K_Max;
  double   C_Gamma;
  double   M_Gamma;
  double   Y_Gamma;
  double   K_Gamma;
  gint     use_lin;
} Linearisation;

extern Linearisation linear;

/*** functions ***/

  // variable function
void          start_vals    (   void );

  // image functions
int           create_separated_image ( void );

  // convert function
int           convert_image (   void );

  // cms functions

char*         get_proof_name();
cmsHPROFILE   get_print_profile();
cmsHPROFILE   getLinearisation   ( void );

  // PDB functions
gint 
p_pdb_procedure_available (char *proc_name);
    
gint32  
p_gimp_save_tiff (gint32 image_ID,
		  gint32 drawable_ID,
                  gint   colour_space,
                  gchar* embedProfile);

  // helper funcs
void
set_tiff_filename                      (int              colour_space);

int
test_tiff_settings                     (void);

int
test_separation_profiles               (void);

int
test_print_settings                    (void);


