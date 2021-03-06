/*
 * EventTesterWinClass.h  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef EventTesterWinClass_h
#define EventTesterWinClass_h
#include <FL/Fl.H>
#include "WidgetWrapper.hpp"
#include <FL/Fl_Window.H>

class EventTesterWinClass : public WidgetWrapper {
public:
  EventTesterWinClass();
private:
  Fl_Window *window_;
  void cb_window__i(Fl_Window*, void*);
  static void cb_window_(Fl_Window*, void*);
public:
  ~EventTesterWinClass();
  Fl_Window* window();
};
#endif
