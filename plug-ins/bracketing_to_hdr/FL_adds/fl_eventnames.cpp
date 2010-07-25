/*
 * fl_eventnames.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  fl_eventnames.cpp  --  Namen der Fltk-Event-Konstanten 
*/

#include "fl_eventnames.hpp"

const char* fl_eventnames[] =
{   "FL_NO_EVENT",
    "FL_PUSH",
    "FL_RELEASE",
    "FL_ENTER",
    "FL_LEAVE",
    "FL_DRAG",
    "FL_FOCUS",
    "FL_UNFOCUS",
    "FL_KEYDOWN",
    "FL_KEYUP",
    "FL_CLOSE",
    "FL_MOVE",
    "FL_SHORTCUT",
    "FL_DEACTIVATE",
    "FL_ACTIVATE",
    "FL_HIDE",
    "FL_SHOW",
    "FL_PASTE",
    "FL_SELECTIONCLEAR",
    "FL_MOUSEWHEEL",
    "FL_DND_ENTER",
    "FL_DND_DRAG",
    "FL_DND_LEAVE",
    "FL_DND_RELEASE"
};

const int fl_max_event = sizeof(fl_eventnames) / sizeof(fl_eventnames[0]);

// END OF FILE
