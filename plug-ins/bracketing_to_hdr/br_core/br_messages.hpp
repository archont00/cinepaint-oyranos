/*
 * br_messages.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_messages.hpp
  
  For messages from the ./br_core/ scope (in particular Br2HdrManager) which
   shall appear on the GUI screen in a modal window (if possibly), we provide
   some global function pointers. Per default they are directed to functions
   which output on stdout or stderr. In the GUI layer (../gui/br_messages_gui.cpp)
   they are redirected to appropriate GUI functions. 
  
  Alternatives? (1) Virtual message functions in Br2HdrManager with overwriting
   in a class Br2HdrManagerGUI. Long-winded. And the functions could not be 
   used outside of Br2HdrManager. (2)...
*/
#ifndef br_messages_hpp
#define br_messages_hpp


namespace br {


/*  Sometimes (e.g. for doubling of GUI messages to stderr or for fatal messages)
   we need to call the core message functions directly (and not only via an
   abstract pointer). That's why they are declared public.
*/
void core_msg_alert (const char* text);
void core_msg_message (const char* text);


/*  The types of the function pointers */
typedef void (*MsgFuncAlert) (const char* text);
typedef void (*MsgFuncMessage) (const char* text);
typedef void (*FmtMsgFuncAlert) (const char* fmt, ...);
typedef void (*FmtMsgFuncMessage) (const char* fmt, ...);
typedef void (*MsgFuncImageIgnored) (const char* name, const char* fmt, ...);


/*  Declaration of the function pointers */
extern MsgFuncAlert         alert;      ///< output a simple string
extern MsgFuncMessage       message;    ///< output a simple string
extern FmtMsgFuncAlert      v_alert;    ///< output a formated string (va_arg list)
extern FmtMsgFuncMessage    v_message;  ///< output a formated string (va_arg list)
extern MsgFuncImageIgnored  msg_image_ignored;
 
                        
}  // namespace "br"

#endif  // br_messsages_hpp
