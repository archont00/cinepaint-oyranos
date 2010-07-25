/*
 * i18n.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
 *
 * Copyright (c) 2005-2006  Hartmut Sbosny  <hartmut.sbosny@gmx.de>
 *
 * LICENSE:
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
/**
  @file i18n.hpp
  
  Wrapper file for the internationalization (gettext) stuff. Essentially we
   have to provide here the macros \a _() and \a N_(); further is needed an
   initialisation procedure or macro, but this is used only once in main() and
   may be provided by a special file. For the (Gtk)CinePaint plug-in we use
   gracefully the realization provided by CinePaint in the file 
     
       ${cpaint_root_dir}/libgimp/stdplugins-intl.h 
       
   which we simply include here and which defines \a _() and \a N_().
  
  For other host programs (Wirtsprogramme) an adaption could be made here
   centrally. That's why each Br2Hdr source file using \a _() or \a N_()
   should include this file and should not refer directly to "stdplugins-intl.h".
   
*/
#ifndef i18n_hpp__
#define i18n_hpp__


#ifdef NO_CINEPAINT

#  define _(String)     (String)
#  define N_(String)    String

#else

//  I am not shure whether the `extern "C"' is needed here for C++ progs
extern "C" {
#  include "../../../libgimp/stdplugins-intl.h"
}

#endif  // NO_CINEPAINT


#endif  // i18n_hpp__

// END OF FILE
