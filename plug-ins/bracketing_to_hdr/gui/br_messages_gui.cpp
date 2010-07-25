/*
 * br_messages_gui.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_messages_gui.cpp
  
  Upgrading of the br_core message functions to GUI message functions.
*/

#include <cstdarg>                      // va_start(), va_end(), va_list
#include <cstdio>                       // vsnprintf()
#include <FL/fl_ask.H>
#include "../br_core/br_messages.hpp"   // funct. pointer `br::alert'...
#include "../br_core/i18n.hpp"          // macro _()


static void 
gui_msg_alert__ (const char* text) 
{
    br::core_msg_alert (text);          // doubles the msg to stderr
    fl_alert ("%s", text);
}

static void
gui_msg_message__ (const char* text) 
{
    //br::core_msg_message (text);      // doubles the msg to stderr
    fl_message ("%s", text);
}

/**+*************************************************************************\n
  Because an ellipse can not be transfered directly to another, we build the
   message string first.
******************************************************************************/
static void 
gui_msg_v_alert__ (const char* fmt, ...)
{
    if (!fmt) return;
    
    const int  LEN = 1024;
    char       text [LEN];
    
    va_list p;
    va_start (p,fmt);
    int n = vsnprintf (text, LEN, fmt, p);
    va_end (p);
      
    if (n < -1) {                     // vsnprintf() error   
      br::core_msg_alert ("vsnprintf() error in gui_msg_v_alert()"); 
      return;    
    }
    //else if (n >= LEN) {}           // truncated

    br::core_msg_alert (text);        // doubles the msg to stderr
    fl_alert ("%s", text);
}
/**+*************************************************************************\n
  Because an ellipse can not be transfered directly to another, we build the
   message string first.
******************************************************************************/
static void 
gui_msg_v_message__ (const char* fmt, ...)
{
    if (!fmt) return;
    
    const int  LEN = 1024;
    char       text [LEN];
    
    va_list p;
    va_start (p,fmt);
    int n = vsnprintf (text, LEN, fmt, p);
    va_end (p);
      
    if (n < -1) {                     // vsnprintf() error   
      br::core_msg_alert ("vsnprintf() error in gui_msg_v_message()"); 
      return;    
    }
    //else if (n >= LEN) {}           // truncated
      
    //br::core_msg_message (text);    // doubles the msg to stderr
    fl_message ("%s", text);
}


/**+*************************************************************************\n
  Because an ellipse can not be transfered directly to another, we build the
   message string first.
******************************************************************************/
static void 
gui_msg_image_ignored__ (const char* name, const char* fmt, ...)
{
    if (!fmt) return;
    
    const char* str_ignore = _("Image ignored.");
    const int   LEN = 1024;
    char        text [LEN];
    
    va_list p;
    va_start (p,fmt);
    int n = vsnprintf (text, LEN, fmt, p);
    va_end (p);
      
    if (n < -1) {                     // vsnprintf() error   
      br::core_msg_alert ("vsnprintf() error in gui_msg_image_ignored()"); 
      return;    
    }
    //else if (n >= LEN) {}           // truncated

    fl_alert ("\"%s\":\n\n%s\n\n%s", name, text, str_ignore);
}

/**+*************************************************************************\n
  Set the global message function pointers to GUI functions.
******************************************************************************/
void init_gui_messages()
{
    br::alert = gui_msg_alert__;
    br::message = gui_msg_message__;
    br::v_alert = gui_msg_v_alert__;
    br::v_message = gui_msg_v_message__;
    br::msg_image_ignored = gui_msg_image_ignored__;
}


/*============================================================================
  Idea for automatic initialization: A global object is instantiated automat.
   and its Ctor can set the function pointer. 
  PROBLEM: Objekt muss *nach* Standardsetzung der Zeiger in "br_message.cpp"
   instanziiert werden, sonst wuerden Funktionszeiger wieder ueberschrieben!
   Sicherer ist expliziter Aufruf einer Funktion init_gui_messages().
=============================================================================*/
#if 0
class MessageInitializer 
{
public:
    MessageInitializer() 
    {
      br::alert = gui_msg_alert__;
      br::message = gui_msg_message__;
      br::v_alert = gui_msg_v_alert__;
      br::v_message = gui_msg_v_message__;
      br::msg_image_ignored = gui_msg_image_ignored__;
    }  
};

MessageInitializer  msg_initializer;
#endif


// END OF FILE
