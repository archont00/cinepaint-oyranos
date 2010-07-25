/*
 *   Cineon image file library test program.
 *
 *   Copyright 2001 David Hodson <hodsond@acm.org>
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
 *
 *   Rev 0.1 Jan 2001.
 *   Rev 0.2 Apr 2001 - more stuff.
 */

#include <stdio.h>
#include <stdlib.h>
/* #include <signal.h> */
#include <string.h>
#include <math.h>

#include "logImageLib.h"

static void
load_image(char* filename, LogImageByteConversionParameters* conversion) {

  int width, height, depth;
  int y;
  LogImageFile* logImage;
  unsigned short* pixels;

  d_printf("Trying Cineon format:\n");
  logImage = logImageOpen(filename, 1);
  if (logImage == NULL) {
    d_printf("Trying DPX format:\n");
    logImage = logImageOpen(filename, 0);
    if (logImage == NULL) {
      d_printf("Can't open image file <%s>\n", filename);
      exit(1);
    } else {
      d_printf("File seems to be DPX format.\n");
    }
  } else {
    d_printf("File seems to be Cineon format.\n");
  }

  logImageSetByteConversion(logImage, conversion);

  if (strrchr(filename, '/') != NULL) {
    d_printf("Loading %s:", strrchr(filename, '/') + 1);
  } else {
    d_printf("Loading %s:", filename);
  }

  logImageGetSize(logImage, &width, &height, &depth);
  d_printf("Image size %d x %d x %d.\n", width, height, depth);

  pixels = malloc(width * depth);

 /*
  * Load the image...
  */

  for (y = 0; y < height; ++y) {
    if (logImageGetRowBytes(logImage, pixels, y) != 0) {
        d_printf("logImageGetRowBytes(logImage, pixels, %d) failed!\n", y);
    }
  }

  logImageClose(logImage);
  free(pixels);
}

int
main(int argc, char* argv[]) {

  LogImageByteConversionParameters conversion;
  int image_id;

  if (argc != 2) {
    d_printf("Usage: %s <logImageFileName>\n", argv[0]);
    exit(1);
  }

#if 1
  logImageSetVerbose(1);
  logImageGetByteConversionDefaults(&conversion);
  load_image(argv[1], &conversion);
#endif

  logImageDump(argv[1]);

  return 0;  
}
