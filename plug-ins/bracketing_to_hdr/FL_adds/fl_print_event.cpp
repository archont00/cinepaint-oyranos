/*
 * fl_print_event.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  fl_print_event.cpp  --  some debug utilities for Fltk
 
   o fl_print_event()       -- Ausgabe der Eventnamen mittels `printf()'.
   o fl_print_damage_mask() -- Ausgabe einer Damage-Maske.
   o fl_print_damages()     -- Ausgabe einer Damage-Mask-Hierarchie
   o fl_print_children()    -- Ausgabe der Kinderstruktur eines Widgets.
*/

#include <cstdio>
#include <cstring>          // strcat()

#include <FL/Fl_Group.H>

#include "fl_eventnames.hpp"
#include "fl_print_event.hpp"


const char* fl_eventname(int event)
{ 
  if (event < 0 || event >= fl_max_event)
    return "-- Unbekanntes Event --";
  else 
    return fl_eventnames[event];
}

/**----------------------------------------------------------------
 * fl_print_event()...
 * 
 *   @param s,p: erlauben Zusatzinformationen der Art ``s "p":''
 *     voranzustellen, insbesondere s==__func__ und p==label().
 * 
 * "Falsche" Arg-Reihenfolge wegen Default-Argumenten.
 *-----------------------------------------------------------------*/
void fl_print_event (int event, const char* s, const char* p)
{
  printf ("%s \"%s\":\t %s\n", s, p, fl_eventname(event));
}

void fl_print_all_events()
{
  for (int i=0; i < fl_max_event; i++)
    printf("%2d:  %s\n", i, fl_eventnames[i]);
}


const char* FL_damage_names[] =     // FL_DamageNames, fl_damagenames
{  
  "clean",              // 0
  "DAMAGE_CHILD",       // 0x01
  "DAMAGE_EXPOSE",      // 0x02
  "DAMAGE_SCROLL",      // 0x04
  "DAMAGE_OVERLAY",     // 0x08
  "DAMAGE_USER1",       // 0x10
  "DAMAGE_USER2",       // 0x20
  "DAMAGE_ALL"          // 0x80
};

//---------------------------------------------------
// print the content of a damage mask (no newline!)...
//---------------------------------------------------
void fl_print_damage_mask(unsigned char mask)
{
  char s[80] = "";
  if (mask == 0) sprintf(s," clean"); 
  else {
    if (mask & FL_DAMAGE_CHILD)   strcat(s," CHILD");
    if (mask & FL_DAMAGE_EXPOSE)  strcat(s," EXPOSE");
    if (mask & FL_DAMAGE_SCROLL)  strcat(s," SCROLL");
    if (mask & FL_DAMAGE_OVERLAY) strcat(s," OVERLAY");
    if (mask & FL_DAMAGE_USER1)   strcat(s," USER1");
    if (mask & FL_DAMAGE_USER2)   strcat(s," USER2");
    if (mask & FL_DAMAGE_ALL)     strcat(s," ALL"); 
  }
  printf("damage mask: %d: %s", mask, s);
}

/**------------------------------------------------------------------
 * fl_print_damages()...
 *
 *   @param w: widget the damage mask shall print for
 *   @param level: hierachy level (for recursive calls)
 *
 * If `w' is a Fl_Group, print the damage mask of a widget `w' including
 *   the damage masks of all of its children . Suitable for recursive
 *   calls, `level' describes the level.
 *------------------------------------------------------------------*/
void fl_print_damages (Fl_Widget* w, int level)
{
  for (int i=0; i < level; i++) printf("\t");   // indent for sub levels
  fl_print_damage_mask(w->damage());            // print damage mask and...
  printf("\t\"%s\"\n", w->label());             // the widget label as memo
  
  Fl_Group* g = dynamic_cast<Fl_Group*>(w);     // Is `w' a Fl_Group? 
  if (g)                                        // Yes? Then it has functions
    for (int i=0; i < g->children(); i++)       //  children() and child()
      fl_print_damages (g->child(i), level+1);  // recursive call
}


void fl_print_children (Fl_Widget* w, int level)
{
  if (level==0) printf("=== Struktur von \"%s\" ===\n", w->label());
  for (int i=0; i < level; i++) printf("   ");  // indent for sub levels
  printf("\"%s\"\n", w->label());               // widget label as memo
  
  Fl_Group* g = dynamic_cast<Fl_Group*>(w);     // Is `w' a Fl_Group? 
  if (g)                                        // Yes? Then it has functions
    for (int i=0; i < g->children(); i++)       //  children() and child()
      fl_print_children (g->child(i), level+1); // recursive call
}


// END OF FILE
