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

#include <unistd.h>
#include <stdio.h>

#include "../version.h"
#include "libgimp/gimpintl.h"
#include "../fl_i18n_cinepaint.h"
#include "fl_i18n.H"


/* beku */
void
cinepaint_init_i18n (const char *exec_path)
{
  const char *locale_paths[3];
  const char *domain[] = {"cinepaint","cinepaint-std-plugins"};
  int         paths_n = 3, i;

  int path_nr = 0;

#if defined(_Xutf8_h) || HAVE_FLTK_UTF8
  FL_I18N_SETCODESET set_charset = FL_I18N_SETCODESET_UTF8;
#else
  FL_I18N_SETCODESET set_charset = FL_I18N_SETCODESET_SELECT;
#endif

  locale_paths[0] = LOCALEDIR;
  locale_paths[1] = "../po";
  locale_paths[2] = exec_path;
  path_nr = fl_search_locale_path  ( paths_n,
                                     locale_paths,
                                     "de",
                                     domain[0]);

  if ( path_nr >= 0 )
  {
    int err = 0;
    err = fl_initialise_locale( domain[0], locale_paths[path_nr], set_charset );
    err = fl_initialise_locale( domain[1], locale_paths[path_nr], set_charset );
    textdomain (domain[1]);
    if(err) {
      fprintf( stderr,"i18n initialisation failed");
    } else
      fprintf( stderr, "Locale found in %s\n", locale_paths[path_nr]);
  } else
  {
    for (i=0; i < paths_n; ++i)
      fprintf( stderr, "Locale not found in %s\n", locale_paths[i]);
  }

  fl_translate_file_chooser();
}
