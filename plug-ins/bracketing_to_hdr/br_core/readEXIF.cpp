/*
 * readEXIF.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
/*!
  readEXIF.cpp
*/
#include <cstdio>               // fprintf(), sscanf()
#include <cstring>              // strcasecmp(), strstr()
#include <cstdlib>              // system()

#include "path_utils.hpp"       // getFileExtension(), get_jhead_path(), get_dcraw_path()
#include "readEXIF.hpp"         // prototypes


static const char* const  http_jhead = "http://www.sentex.net/~mwandel/jhead/";
static const char* const  http_dcraw = "http://www.cybercom.net/~dcoffin/dcraw/";

//==================
//  local functions
//==================

static void errormsg (const char* prog, const char* imgName) {
    fprintf (stderr, "`%s' failed in reading of EXIF data from \"%s\"\n", 
        prog, imgName);
}

static void errormsg_no_prog (const char* prog, const char* http=0) {
    fprintf (stderr, "Program `%s' is required to read EXIF data (not found "
         "in path).\n", prog);
    if (http) 
      fprintf (stderr, "You can download it from: %s.\n", http);
}



/**+*************************************************************************\n
  Calls `dcraw' to output the EXIF data of image file \a fname and browses that
   output. The path to `dcraw' is taken from get_dcraw_path().
******************************************************************************/
static bool
read_with_dcraw (const char* fname, float* shutter, float* aperture, float* iso)
{
#if 0
    /*  Previous approach: */
    /*  Have we `dcraw' in path and can it decode the file? */
    snprintf (command, 1024, "dcraw -i %s", fname);
    int stat = system (command);
    if (stat != 0) {
      if (stat > 0x200)
        errormsg_no_prog ("dcraw"); 
      else {
        errormsg ("dcraw", fname);
      }
      return false;
    }
#endif    
    
    const char* const  PROG = "dcraw";
    const char*  prog_path = get_dcraw_path();
    char  command [1024];
    
    //printf("prog_name = %s\n", prog_name);
    if (!prog_path) {
      errormsg_no_prog (PROG, http_dcraw); 
      return false;
    }

    snprintf (command, 1024, "%s -i -v %s", prog_path, fname);
    puts (command);
    FILE* pfp = popen (command, "r");
    
    if (!pfp) {
      perror (command);
      return false;
    }
    
    char  buf [128];
    while (fgets (buf, 128, pfp)) {
      //printf ("\"%s\"\n", buf);
      if (sscanf (buf, "Shutter: 1/%f", shutter) == 1) {
      }
      else if (sscanf (buf, "Shutter: %f", shutter) == 1) {
        *shutter = 1.0f / *shutter;
      }
      else if (sscanf (buf, "Aperture: f/%f", aperture) == 1) {
      }
      else if (sscanf (buf, "ISO speed: %f", iso) == 1) {
      }
    }
    pclose (pfp);
    return true;
}

/**+*************************************************************************\n
  Calls `jhead' to output the EXIF data of image file \a fname and browses that
   output. The path to `jhead' is taken from get_jhead_path().
******************************************************************************/
static bool
read_with_jhead (const char* fname, float* shutter, float* aperture, float* iso)
{
#if 0
    /*  Previous approach: Have we `jhead' in general bin path? */
    snprintf (command, 1024, "jhead -h > /dev/null");
    if (system (command) > 0x200) {
      errormsg_no_prog ("jhead", http_jhead); 
      return false;
    }
#endif
    
    const char* const  PROG = "jhead";
    const char*  prog_path = get_jhead_path();
    char  command [1024];
    
    //printf("prog_path = %s\n", prog_path);
    if (!prog_path) {
      errormsg_no_prog (PROG, http_jhead); 
      return false;
    }
    
    snprintf (command, 1024, "%s %s", prog_path, fname);
    puts (command);
    FILE* pfp = popen (command, "r");
    
    if (!pfp) {
      perror (command);
      return false;
    }
    
    char  buf [128];
    while (fgets (buf, 128, pfp)) { 
      //printf ("\"%s\"\n", buf);
      char*  p;
      if (strstr (buf, "Exposure time")) {
        if ((p = strchr (buf, ':'))) {
          sscanf (p+1, "%f", shutter);
          if (*shutter != 0.0f) *shutter = 1.0f / *shutter;
        }
      }
      else if (strstr (buf, "Aperture")) {
        if ((p = strstr (buf, "f/"))) {
          sscanf (p+2, "%f", aperture);
        }
      }
      else if (strstr (buf, "ISO equiv")) {
        if ((p = strchr (buf, ':'))) {
          sscanf (p+1, "%f", iso);
        }
      }
    }
    pclose (pfp);
    return true;
}


//==================
//  public function
//==================

/**+*************************************************************************\n  
  Tries to read the EXIF data: shutter speed, aperture and ISO sensitivity from
   file \a fname. If a data could not recognized, a zero value is returned.
   
  @pre  For EXIF reading we use the external programs `jhead' and `dcraw'. They
   are expected either in CinePaint's "extra" directory or (if not there) in the
   general binary path. Their existence is checked by the helpers get_jhead_path()
   and get_dcraw_path() and a console message is given if something is missed.
********************************************************************************/
bool
readEXIF (const char* fname, float* shutter, float* aperture, float* iso)
{
    bool  ret = false;
    
    *shutter = 0.0f;    // zero indicates 'not read'
    *aperture = 0.0f;
    *iso = 0.0f;

    const char*  ext = getFileExtension (fname);
    if (*ext) ext++;  // next behind '.'
    
    /*  Dismiss some senseless attempts */
    if (strcasecmp (ext,"bmp") == 0) return false;
    if (strcasecmp (ext,"png") == 0) return false;
    if (strcasecmp (ext,"xbm") == 0) return false;
    
    if (strcasecmp (ext,"jpg") == 0)
      ret = read_with_jhead (fname, shutter, aperture, iso);
    else
      ret = read_with_dcraw (fname, shutter, aperture, iso);
    
    printf ("%s(): shutter = %f,  aperture = %f  ISO = %f\n",__func__, *shutter, *aperture, *iso);
    return ret;
}

// END OF FILE
