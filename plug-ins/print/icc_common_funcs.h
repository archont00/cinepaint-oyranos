/*
 * "$Id: icc_common_funcs.h,v 1.1 2005/02/13 22:15:23 beku Exp $"
 *
 *   ICC profile stuff for CinePaint
 *
 *   Copyright 2003-2004   Kai-Uwe Behrmann (ku.b@gmx.de)
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

#ifndef _ICC_COMMON_FUNCS_H_
#define _ICC_COMMON_FUNCS_H_

#include <gtk/gtk.h>

#include <lcms.h>

#if LCMS_VERSION < 112
#define cmsFLAGS_BLACKPOINTCOMPENSATION 0x2000
#endif

/*** function definitions ***/

  // --- variable function ---

  // --- image functions ---

  // --- cms functions ---

void          lcms_error    (   int             ErrorCode,
                                const char*     ErrorText);

char*         getProfileInfo(   cmsHPROFILE     hProfile);

char*         getPCSName    (   cmsHPROFILE     hProfile);

char*         getColorSpaceName ( cmsHPROFILE   hProfile);

int           getLcmsColorSpace ( cmsHPROFILE   hProfile);

char*         getDeviceClassName ( cmsHPROFILE  hProfile);

char*         getSigTagName (   icTagSignature  sig );

int           checkProfileDevice ( char*        type,
                                cmsHPROFILE     hProfile);

cmsHPROFILE   copyProfile   (   cmsHPROFILE     hIn,
                                cmsHPROFILE     hOut);

// debugging
void          printProfileTags ( cmsHPROFILE    hProfile );
void          printLut      (   LPLUT           Lut,
                                int             sig);


void          saveProfileToFile ( char         *name,
                                char           *profile,
                                int             size );

#endif //_ICC_COMMON_FUNCS_H_
