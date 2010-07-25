/*
 *   Print plug-in for the CinePaint/GIMP.
 *
 *   Copyright 2003-2004 Kai-Uwe Behrmann
 *
 *   add some function declaration
 *    Kai-Uwe Behrman (web@tiscali.de) --  December 2003
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



/* --- Switches ... --- */
#define SHOW_CMYK


/*
 *  Global macros
 */
#ifndef cmsFLAGS_PRESERVEBLACK
#define cmsFLAGS_PRESERVEBLACK            0x8000
#endif
#ifndef LCMS_VERSION
#define LCMS_VERSION 0
#endif

#ifdef DEBUG
 #define DBG printf("%s:%d %s()\n",__FILE__,__LINE__,__func__);
 #define DBG_S(text) { printf("%s:%d %s() ",__FILE__,__LINE__,__func__); \
                       printf text; printf("\n"); }
#else
 #define DBG 
 #define DBG_S(text)
#endif


/*
 * Global variables...
 */
extern char version_[];

extern int lcms_run;

/*
 * Global functions...
 */



