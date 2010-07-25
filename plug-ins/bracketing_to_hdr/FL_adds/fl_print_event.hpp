/*
 * fl_print_event.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  fl_print_event.hpp
*/
#ifndef fl_print_event_hpp
#define fl_print_event_hpp

#include <FL/Fl_Widget.H>

const char* fl_eventname (int event);
void fl_print_event (int event, const char* s="", const char* p="");
void fl_print_all_events();
void fl_print_damage_mask (unsigned char mask);
void fl_print_damages (Fl_Widget* w, int level=0);
void fl_print_children (Fl_Widget* w, int level=0);

#endif  // fl_print_evet_hpp

// END OF FILE
