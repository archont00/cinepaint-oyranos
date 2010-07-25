/*
 * fl_translate_menu.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  fl_translate_menu.cpp
  
  We don't put this file in our general FLTK directory ./FL_adds/, because we
   depend on "i18n.hpp", which is Br2HDR specific.

*/
#include <FL/Fl_Menu_Item.H>
#include "fl_translate_menu.hpp"    // prototype
#include "../br_core/i18n.hpp"      // macro _()
//#include <cstdio>                 // printf() for debug output

/**+*************************************************************************\n
  Because FLTK's menu data are often stored in arrays initalized by string
   literals (in particular FLTK's visual editor FLUID generates such code), the
   strings therein are not "seen" by gettext's runtime translation. To get this
   translation, the strings need an extra runtime touching. This does 
   fl_translate_menu(). A good place for usage in FLUID is the "ExtraCode" field
   of a menu widget \a o in the way <pre>
     fl_translate_menu((Fl_Menu_Item*)o->menu());</pre>
   the cast to \a Fl_Menu_Item* is needed here because \a o->menu() returns a
   const pointer.
  
  @note The gettext docu offers for such cases the pair \a gettext_noop() and
   \a gettext(), ordinarily shortened by the macros \a N_() and \a _(). \a N_()
   -- with a string literal as argument -- shall be used at the moment of
   definition and causes only the entry of the string in the .po-file without
   asking for runtime translation; \a _() -- with a string variable as argument
   -- shall be used at the moment(s) of usage and causes the runtime translation.
   In opposite of that, FLUID marks also the menu string literals by \a _()
   instead of by \a N_(). The gettext docu makes the insinuation (§3.6), this
   could have in rare cases unpredictable results. It's not clear to me, whether
   we could get trouble.
******************************************************************************/
void
fl_translate_menu (Fl_Menu_Item* menu)
{
  for (int i=0; i < menu->size(); i++) {
    const char* text = menu[i].label();
    if (text) {
      menu[i].label( _(text) );
      //printf("%2d:  %s : %s\n", i, text, _(text));
    }
  }
}
