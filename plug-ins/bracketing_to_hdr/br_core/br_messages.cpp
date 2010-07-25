/*
 * br_messages.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_messages.cpp
*/

#include <cstdarg>          // va_start(), va_end(), va_list
#include <cstdio>           // printf(), vfprintf()
#include "br_messages.hpp"


namespace br {


/**+*************************************************************************\n
  Outputs \a text on stderr as a Br2Hdr alert message. 
  \note A final '\n' is added here, so avoid it in \a text.
******************************************************************************/
void core_msg_alert (const char* text)
{
    fprintf (stderr, "Br2Hdr-ALERT **: %s\n", text);
}

/**+*************************************************************************\n
  Outputs \a text on stderr as a normal Br2Hdr message. 
  \note A final '\n' is added here, so avoid it in \a text.
******************************************************************************/
void core_msg_message (const char* text)
{
    fprintf (stderr, "Br2Hdr-MESSAGE: %s\n", text);
}

/**+*************************************************************************\n
  Outputs on stderr the formated text \a fmt as a Br2Hdr alert message. 
  \note A final '\n' is added here, so avoid it in \a fmt.
******************************************************************************/
static void 
core_msg_v_alert (const char* fmt, ...)
{
    fprintf (stderr, "Br2Hdr-ALERT **: ");
    va_list p;
    if (fmt) {   
      va_start (p,fmt);
      vfprintf (stderr, fmt, p);
      va_end (p);
    }
    fputc ('\n', stderr);
}

/**+*************************************************************************\n
  Outputs on stderr the formated text \a fmt as a normal Br2Hdr message. 
  \note A final '\n' is added here, so avoid it in \a fmt.
******************************************************************************/
static void 
core_msg_v_message (const char* fmt, ...)
{
    fprintf (stderr, "Br2Hdr-MESSAGE: ");
    va_list p;
    if (fmt) {   
      va_start (p,fmt);
      vfprintf (stderr, fmt, p);
      va_end (p);
    }
    fputc ('\n', stderr);
}

/**+*************************************************************************\n
  Outputs on stdout the message, that the image \a name could not be added to
   the input container; \a fmt should give the reason. \a fmt can be a multi-line
   text including of line breaks and arbitrary parameters, but it should not
   contain a final '\n', because which is added here. At the end we output the
   string "Image ignored", so this should also not be part of \a fmt.
******************************************************************************/
static void 
core_msg_image_ignored (const char* name, const char* fmt, ...)
{
    printf ("\"%s\":\n", name);
    va_list p;
    if (fmt) {   
      va_start (p,fmt);
      vprintf (fmt, p);
      va_end (p);
    }
    puts ("\nImage ignored.\n");
}


/*****************************************************************************
  Default setting for the global message function pointers: 
******************************************************************************/
MsgFuncAlert         alert              =  core_msg_alert;
MsgFuncMessage       message            =  core_msg_message;
FmtMsgFuncAlert      v_alert            =  core_msg_v_alert;
FmtMsgFuncMessage    v_message          =  core_msg_v_message;
MsgFuncImageIgnored  msg_image_ignored  =  core_msg_image_ignored;


}  // namespace "br"

// END OF FILE
