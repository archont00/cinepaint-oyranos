/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "filename.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int filename_parse(const char*,
  const char**, const char**, const char**);

/* is this function available everywhere? should ifdef it... */
static char* local_strndup(const char* src, size_t len);

static char* filename_change_frame(const char*, int, int);

char*
filename_set_frame(const char* filename, int value)
{
  return filename_change_frame(filename, value, 0);
}

char*
filename_inc_frame(const char* filename, int value)
{
  return filename_change_frame(filename, value, 1);
}

int
filename_get_frame_number(const char* filename)
{
  const char* frame;
  int ok = filename_parse(filename, 0, &frame, 0);
  if (! ok) {
    return 0;
  }
  return atoi(frame);
}

char*
filename_get_pre(const char* filename)
{
  const char* pre;
  const char* frame;
  int ok = filename_parse(filename, &pre, &frame, 0);
  if (! ok) {
    return 0;
  }
  return local_strndup(pre, frame - pre);
}

char*
filename_get_frame(const char* filename)
{
  const char* frame;
  const char* post;
  int ok = filename_parse(filename, 0, &frame, &post);
  if (! ok) {
    return 0;
  }
  return local_strndup(frame, post - frame);
}

char*
filename_get_post(const char* filename)
{
  const char* post;
  int ok = filename_parse(filename, 0, 0, &post);
  if (! ok) {
    return 0;
  }
  return strdup(post);
}

char*
filename_build_path(const char* dir, const char* name, int value)
{
  char* temp;
  char* result;

  if ((dir == 0) || (name == 0)) {
    return 0;
  }

  temp = (char*)malloc(strlen(dir) + strlen(name) + 1);
  if (temp == 0) {
    return 0;
  }
  sprintf(temp, "%s%s", dir, name);
  result = filename_set_frame(temp, value);
  free(temp);
  return result;
}

char*
filename_build_label(const char* dir, const char* name)
{
  const char* frame;
  const char* post;
  size_t preLength;
  const char* tag = "[xxx]";
  char* result;
  int ok;

  if ((dir == 0) || (name == 0)) {
    return 0;
  }

  ok = filename_parse(name, 0, &frame, &post);
  if (! ok) {
    return 0;
  }

  preLength = frame - name;
  result = (char*)malloc(strlen(dir)+preLength+strlen(tag)+strlen(post)+1);
  if (result == 0) {
    return 0;
  }

  sprintf(result, "%s%*.*s%s%s",
    dir, preLength, preLength, name, tag, post);
  return result;
}

/* used by filename_set_frame and filename_inc_frame */
static char*
filename_change_frame(const char* filename, int value, int relative)
{
  const char* frame;
  const char* post;
  char* result;
  size_t preLength;
  size_t frameLength;
  int frameNumber;

  int ok = filename_parse(filename, 0, &frame, &post);
  if (! ok) {
    return 0;
  }

  preLength = frame - filename;
  frameLength = post - frame;

  if (relative) {
    frameNumber = atoi(frame) + value;
  } else {
    frameNumber = value;
  }
  if (frameNumber < 0) return 0;

  /* worst case: original number is 1, replacement is 99999999 or so... */
  result = (char*)malloc(strlen(filename) + 10);
  if (result == 0) {
    return 0;
  }

  sprintf(result, "%*.*s%*.*d%s",
    preLength, preLength, filename, frameLength, frameLength, frameNumber, post);

  return result;
}

/* parse a filename into [path]/[pre][frame][post] */
/* pre points to first character after last slash */
/* frame points to first character of last numeric group */
/* post points to first character after last numeric group */
/* sets pointers into original filename */
/* ant return parameter can be null if not required */
/* returns false if the filename contains no digits after the path */

static int
filename_parse(const char* filename,
  const char** prePtr, const char** framePtr, const char** postPtr)
{
  const char* pre = filename;
  const char* frame = 0;
  const char* post = 0;
  const char* extn;
  const char* str;

  if (filename == 0) {
    return 0;
  }

  /* look for end of path */
  str = strrchr(filename, '/');
  if (str == 0) {
    str = filename;
  } else {
    pre = str + 1;
  }

  /* look for extension */
  extn = strrchr(filename, '.');
  if (extn == 0) {
    extn = filename + strlen(filename);
  }

  while (1) {
    size_t numDigits;
    /* skip any non-numerics */
    str += strcspn(str, "0123456789");
    /* ignore any digits in extn */
    if (str >= extn) {
      break;
    }
    /* check for numerics */
    numDigits = strspn(str, "0123456789");
    if (numDigits == 0) {
      break;
    }
    frame = str;
    post = frame + numDigits;
    str = post;
  }

  if (frame == 0) {
    return 0;
  }

  if (prePtr) *prePtr = pre;
  if (framePtr) *framePtr = frame;
  if (postPtr) *postPtr = post;
  return 1;
}

/* this local version assumes source string is at least len chars long */
static char* local_strndup(const char* src, size_t len)
{
  char* result = (char*)malloc(len + 1);
  strncpy(result, src, len);
  result[len] = 0;
  return result;
}
