/*
 * path_utils.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  path_utils.cpp
*/

#include <cstdio>               // sprintf()
#include <cstring>              // strrchr(), strcasecmp(), strstr()
#include <cstdlib>              // system(), getenv()
#include "path_utils.hpp"       // prototypes
#include "br_macros.hpp"        // BR_WARNING()

#ifndef NO_CINEPAINT
extern "C" {
#  include "lib/plugin_main.h"  // plugin's argv
#  include <glib.h>             // G_DIR_SEPARATOR
}
#else
   char*  argv[] = {""};        // Notbehelf
#  define G_DIR_SEPARATOR   '/'
#endif



/**+*************************************************************************\n
  Returns the extension of \a path or NULL if none.
******************************************************************************/
const char*
getFileExtension (const char* path) {
    return strrchr (path, '.');
}


/**+*************************************************************************\n
  Returns the default tmp directory of the application. Provisionally.
  
  @todo Statt HOME-Verzeichnis wohl besser erst ~/tmp/ oder ~/.cinepaint/tmp/
   versuchen.
******************************************************************************/
const char*
getTmpDir() {
    const char*  home = getenv ("HOME");
    return home ? home : ".";
}


/**+*************************************************************************\n
  Writes into \a destbuf a tmp path consisting of the application's tmp dir
   given by getTmpDir() and the filename argument \a fname. It is assumed that
   the destination buffer is at least \a maxlen characters in length (default:
   512).
******************************************************************************/
bool
buildTmpPath (char* destbuf, const char* fname, int maxlen) 
{
    if (!getTmpDir() || !fname || !*fname)  
      return false;
    
    if (snprintf (destbuf, maxlen, "%s/%s", getTmpDir(), fname) >= maxlen) { 
      BR_WARNING(("Destination buffer too small to build temp path!"));
      return false;
    }
    
    return true;
}


/**+*************************************************************************\n
  Returns the path to an executable `jhead' program or NULL if none. At the
   first call the static variable \a path is set either 1.) to the absolute path
   of CinePaint's own jhead executable if such exists; or 2.) if none and an
   executable is found in the general bin path, simply to "jhead"; otherwise
   \a path is set to NULL. The eventually allocated dyn. \a path string gets
   automatically destroyed at plugin end.
   
  @todo Eventually a version check between CinePaint's jhead and the global one.
  
  @internal Nur system("jhead -h > /dev/null") erscheint NICHT auf der Konsole,
   system("jhead > /dev/null") dagegen sehr wohl. Aber wir nutzen ja `which'.
******************************************************************************/
const char* 
get_jhead_path()
{
    const char* const  PROG = "jhead";
    static bool  first_call = true;
    static const char*  path = 0;
    char  command [1024];
    
    if (first_call) {
      /*  Path to CinePaint's own `jhead'... */
      if (strrchr (argv[0], G_DIR_SEPARATOR))
      {
        /*  Size for `argv[0] - "bracketing_to_hdr" + "../extra/jhead"' */
        char*  cpaint_jhead = (char*)malloc (strlen(argv[0]) + 20);
        strcpy (cpaint_jhead, argv[0]);
        char*  p = strrchr (cpaint_jhead, G_DIR_SEPARATOR);
        sprintf (p+1, "..%cextra%c%s", G_DIR_SEPARATOR, G_DIR_SEPARATOR, PROG);
        /*  Does CinePaint's jhead exist? */
        snprintf (command, 1024, "which %s > /dev/null", cpaint_jhead);
        if (system (command)) {
          fprintf (stderr, "%s(): No CinePaint %s. Try global path...\n",__func__, PROG);
          free (cpaint_jhead);
        }
        else {
          path = cpaint_jhead;  // use it
          fprintf (stderr, "%s(): Use CinePaint's %s.\n",__func__, PROG);
        }
      }
      else
        fprintf (stderr, "%s(): Strange argv[0]: \"%s\".\n", __func__, argv[0]);
      
      /*  If no valid `path' we look for `jhead' in global path */
      if (! path) {
        snprintf (command, 1024, "which %s > /dev/null", PROG);
        if (system (command))
          fprintf (stderr, "%s(): No `%s' program found.\n",__func__, PROG);
        else {
          path = PROG;  // use it
          fprintf (stderr, "%s(): `%s' found in global path.\n",__func__, PROG);
        }
      }
      first_call = false;
    } // first_call
    
    return path;
}


/**+*************************************************************************\n
  Returns the path to an executable `dcraw' program or NULL if none. At the
   first call the static variable \a path is set either 1.) to the absolute path
   of CinePaint's own dcraw executable if such exists; or 2.) if none and an
   executable is found in the general bin path, simply to "dcraw"; otherwise
   \a path is set to NULL. The eventually allocated dyn. \a path string gets
   automatically destroyed at plugin end.
  
  @todo Eventually a version check between CinePaint's dcraw and the global one.
  
  @internal Nur system("dcraw -i DATEI > /dev/null") mit existierender DATEI
   erscheint NICHT auf der Konsole, system("dcraw -i > /dev/null") hingegen
   sehr wohl. Aber wir nutzen ja `which'.
******************************************************************************/
const char* 
get_dcraw_path()
{
    const char* const  PROG = "dcraw";
    static bool  first_call = true;
    static const char*  path = 0;
    char  command [1024];
    
    if (first_call) {
      /*  Path to CinePaint's own `dcraw'... */
      if (strrchr (argv[0], G_DIR_SEPARATOR))
      {
        /*  Size for `argv[0] - "bracketing_to_hdr" + "../extra/dcraw"' */
        char*  cpaint_dcraw = (char*)malloc (strlen(argv[0]) + 20);
        strcpy (cpaint_dcraw, argv[0]);
        char*  p = strrchr (cpaint_dcraw, G_DIR_SEPARATOR);
        sprintf (p+1, "..%cextra%c%s", G_DIR_SEPARATOR, G_DIR_SEPARATOR, PROG);
        /*  Does CinePaint's dcraw exist? */
        snprintf (command, 1024, "which %s > /dev/null", cpaint_dcraw);
        if (system (command)) {
          fprintf (stderr, "%s(): No CinePaint %s. Try global path...\n",__func__, PROG);
          free (cpaint_dcraw);
        }
        else {
          path = cpaint_dcraw;  // use it
          fprintf (stderr, "%s(): Use CinePaint's %s.\n",__func__, PROG);
        }
      }
      else
        fprintf (stderr, "%s(): Strange argv[0]: \"%s\".\n", __func__, argv[0]);
      
      /*  If no valid `path' we look for `dcraw' in global path */
      if (! path) {
        snprintf (command, 1024, "which %s > /dev/null", PROG);
        if (system (command))
          fprintf (stderr, "%s(): No `%s' program found.\n",__func__, PROG);
        else {
          path = PROG;  // use it
          fprintf (stderr, "%s(): `%s' found in global path.\n",__func__, PROG);
        }
      }
      first_call = false;
    } // first_call
    
    return path;
}


// END OF FILE
