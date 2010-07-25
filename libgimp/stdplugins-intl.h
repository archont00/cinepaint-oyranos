/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * stdplugins-intl.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __STDPLUGINS_INTL_H__
#define __STDPLUGINS_INTL_H__

#include "config.h"
#include "gimpintl.h"

#if defined (HAVE_BIND_TEXTDOMAIN_CODESET) && defined (GDK_WINDOWING_WIN32)
#define BTDCS(d) bind_textdomain_codeset (d, "UTF-8")
#else
#define BTDCS(d)
#endif

#ifdef HAVE_LC_MESSAGES

#if GTK_MAJOR_VERSION < 2
#define SET_LC_NUMERIC   setlocale (LC_NUMERIC, "C"); 
#endif

#define INIT_I18N()	G_STMT_START{			\
  const char* tdd = getenv("TEXTDOMAINDIR"); \
  if(!tdd) \
    tdd = LOCALEDIR; \
  setlocale(LC_ALL, ""); 				\
  SET_LC_NUMERIC		\
  bindtextdomain("cinepaint-script-fu", tdd);            \
  BTDCS("cinepaint-script-fu");				\
  bindtextdomain("cinepaint-std-plugins", tdd);	\
  BTDCS("cinepaint-std-plugins");				\
  textdomain("cinepaint-std-plugins");			\
  			}G_STMT_END
#else
#define INIT_I18N()	G_STMT_START{			\
  const char* tdd = getenv("TEXTDOMAINDIR"); \
  if(!tdd) \
    tdd = LOCALEDIR; \
  bindtextdomain("cinepaint-script-fu", tdd);            \
  BTDCS("cinepaint-script-fu");				\
  bindtextdomain("cinepaint-std-plugins", tdd);	\
  BTDCS("cinepaint-std-plugins");				\
  textdomain("cinepaint-std-plugins");			\
  			}G_STMT_END
#endif

#define INIT_I18N_UI()	G_STMT_START{	\
  INIT_I18N();				\
			}G_STMT_END


/* For use with FLTK */
#include "fl_i18n_cinepaint.h"
#define INIT_FLTK1_CODESET() G_STMT_START{			\
  const char* tdd = getenv("TEXTDOMAINDIR"); \
  if(!tdd) \
    tdd = LOCALEDIR; \
	cinepaint_init_i18n( tdd );		\
 			}G_STMT_END

#endif /* __STDPLUGINS_INTL_H__ */
