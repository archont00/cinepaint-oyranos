/*
 * "$Id: sgilib.c,v 1.2 2006/11/24 00:11:21 beku Exp $"
 *
 *   SGI image file format library routines.
 *
 *   Copyright 1997 Michael Sweet (mike@easysw.com)
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contents:
 *
 *   sgiClose()    - Close an SGI image file.
 *   sgiGetRow()   - Get a row of image data from a file.
 *   sgiOpen()     - Open an SGI image file for reading or writing.
 *   sgiPutRow()   - Put a row of image data to a file.
 *   getlong()     - Get a 32-bit big-endian integer.
 *   getshort()    - Get a 16-bit big-endian integer.
 *   putlong()     - Put a 32-bit big-endian integer.
 *   putshort()    - Put a 16-bit big-endian integer.
 *   read_rle8()   - Read 8-bit RLE data.
 *   read_rle16()  - Read 16-bit RLE data.
 *   write_rle8()  - Write 8-bit RLE data.
 *   write_rle16() - Write 16-bit RLE data.
 *
 * Revision History:
 *
 *   $Log: sgilib.c,v $
 *   Revision 1.2  2006/11/24 00:11:21  beku
 *   o little endian load fix
 *
 *   Revision 1.1  2004/02/10 00:43:07  robinrowe
 *   add 0.18-1
 *
 *   Revision 1.1.1.1  2002/09/28 21:31:02  samrichards
 *   Filmgimp-0.5 initial release to CVS
 *
 *   Revision 1.1  2002/01/23 22:14:28  aland
 *   Initial revision
 *
 *   Revision 1.1.1.1.2.2  2000/09/06 21:29:53  yosh
 *   Merge with R&H tree
 *
 *   -Yosh
 *
 * Revision 1.1.1.1  2000/02/17  00:54:05  calvin
 * Importing the v1.0 r & h hollywood branch. No longer cvs-ed at gnome.
 *
 *   Revision 1.1.1.1.2.1  1998/03/20 22:30:45  film
 *   sgi can save 16bit too.
 *   -calvin (cwilliamson@berlin.snafu.de)
 *
 *   Revision 1.1.1.1  1997/11/24 22:04:37  sopwith
 *   Let's try this import one last time.
 *
 *   Revision 1.3  1997/11/18 03:04:29  nobody
 *   fixed ugly comment-bugs introduced by evil darkwing
 *   keep out configuration empty dirs
 *   	--darkwing
 *
 *   Revision 1.2  1997/11/17 05:44:04  nobody
 *   updated ChangeLog
 *   dropped non-working doc/Makefile entries
 *   applied many fixes from the registry as well as the devel ML
 *   applied missing patches by Art Haas
 *
 *   	--darkwing
 *
 *   Revision 1.3  1997/07/02  16:40:16  mike
 *   sgiOpen() wasn't opening files with "rb" or "wb+".  This caused problems
 *   on PCs running Windows/DOS...
 *
 *   Revision 1.2  1997/06/18  00:55:28  mike
 *   Updated to hold length table when writing.
 *   Updated to hold current length when doing ARLE.
 *   Wasn't writing length table on close.
 *   Wasn't saving new line into arle_row when necessary.
 *
 *   Revision 1.1  1997/06/15  03:37:19  mike
 *   Initial revision
 */

#include "sgi.h"


/*
 * Local functions...
 */

static int    getlong(sgi_t*);
static int    getshort(sgi_t*);
static int    putlong(long, sgi_t*);
static int    putshort(unsigned short, sgi_t*);
static int    read_rle8(sgi_t*, unsigned short *, int);
static int    read_rle16(sgi_t*, unsigned short *, int);
static int    write_rle8(sgi_t*, unsigned short *, int);
static int    write_rle16(sgi_t*, unsigned short *, int);


/*
 * 'sgiClose()' - Close an SGI image file.
 */

int
sgiClose(sgi_t *sgip)	/* I - SGI image */
{
  int	i;		/* Return status */
  long	*offset;	/* Looping var for offset table */


  if (sgip == NULL)
    return (-1);

  if (sgip->mode == SGI_WRITE && sgip->comp != SGI_COMP_NONE)
  {
   /*
    * Write the scanline offset table to the file...
    */

    fseek(sgip->file, 512, SEEK_SET);

    for (i = sgip->ysize * sgip->zsize, offset = sgip->table[0];
         i > 0;
         i --, offset ++)
      if (putlong(offset[0], sgip) < 0)
        return (-1);

    for (i = sgip->ysize * sgip->zsize, offset = sgip->length[0];
         i > 0;
         i --, offset ++)
      if (putlong(offset[0], sgip) < 0)
        return (-1);
  };

  if (sgip->table != NULL)
  {
    free(sgip->table[0]);
    free(sgip->table);
  };

  if (sgip->length != NULL)
  {
    free(sgip->length[0]);
    free(sgip->length);
  };

  if (sgip->comp == SGI_COMP_ARLE)
    free(sgip->arle_row);

  i = fclose(sgip->file);
  free(sgip);

  return (i);
}


/*
 * 'sgiGetRow()' - Get a row of image data from a file.
 */

int
sgiGetRow(sgi_t *sgip,	/* I - SGI image */
          short *row,	/* O - Row to read */
          int   y,	/* I - Line to read */
          int   z)	/* I - Channel to read */
{
  int	x;		/* X coordinate */
  long	offset;		/* File offset */


  if (sgip == NULL ||
      row == NULL ||
      y < 0 || y >= sgip->ysize ||
      z < 0 || z >= sgip->zsize)
    return (-1);

  switch (sgip->comp)
  {
    case SGI_COMP_NONE :
       /*
        * Seek to the image row - optimize buffering by only seeking if
        * necessary...
        */

        offset = 512 + (y + z * sgip->ysize) * sgip->xsize * sgip->bpp;
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            *row = getc(sgip->file);
        }
        else
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            *row = getshort(sgip);
        };
        break;

    case SGI_COMP_RLE :
        offset = sgip->table[z][y];
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
          return (read_rle8(sgip, row, sgip->xsize));
        else
          return (read_rle16(sgip, row, sgip->xsize));
        break;
  };

  return (0);
}


/*
 * 'sgiOpen()' - Open an SGI image file for reading or writing.
 */

sgi_t *
sgiOpen(char *filename,	/* I - File to open */
        int  mode,	/* I - Open mode (SGI_READ or SGI_WRITE) */
        int  comp,	/* I - Type of compression */
        int  bpp,	/* I - Bytes per pixel */
        int  xsize,	/* I - Width of image in pixels */
        int  ysize,	/* I - Height of image in pixels */
        int  zsize,	/* I - Number of channels */
        int  minpixel,
        int  maxpixel)
{
  int	i, j;		/* Looping var */
  char	name[80];	/* Name of file in image header */
  short	magic;		/* Magic number */
  sgi_t	*sgip;		/* New image pointer */


  if ((sgip = calloc(sizeof(sgi_t), 1)) == NULL)
    return (NULL);

  switch (mode)
  {
    case SGI_READ :
        if (filename == NULL)
        {
          free(sgip);
          return (NULL);
        };

        if ((sgip->file = fopen(filename, "rb")) == NULL)
        {
          free(sgip);
          return (NULL);
        };

        sgip->mode = SGI_READ;

        magic = getshort(sgip);
        if (magic != SGI_MAGIC)
        {
          /* try little endian format */
          magic = ((magic >> 8) & 0x00ff) | ((magic << 8) & 0xff00);
          if(magic != SGI_MAGIC) {
            free(sgip);
            return (NULL);
          } else {
            sgip->swapBytes = 1;
          }
        }

        sgip->comp  = getc(sgip->file);
        sgip->bpp   = getc(sgip->file);
        getshort(sgip);		/* Dimensions */
        sgip->xsize = getshort(sgip);
        sgip->ysize = getshort(sgip);
        sgip->zsize = getshort(sgip);
        sgip->minpixel = getlong(sgip);		/* Minimum pixel */
        sgip->maxpixel = getlong(sgip);		/* Maximum pixel */

        if (sgip->comp)
        {
         /*
          * This file is compressed; read the scanline tables...
          */

          fseek(sgip->file, 512, SEEK_SET);

          sgip->table    = calloc(sgip->zsize, sizeof(long *));
          sgip->table[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
          for (i = 1; i < sgip->zsize; i ++)
            sgip->table[i] = sgip->table[0] + i * sgip->ysize;

          for (i = 0; i < sgip->zsize; i ++)
            for (j = 0; j < sgip->ysize; j ++)
              sgip->table[i][j] = getlong(sgip);
        };
        break;

    case SGI_WRITE :
	if (filename == NULL ||
	    xsize < 1 ||
	    ysize < 1 ||
	    zsize < 1 ||
	    bpp < 1 || bpp > 2 ||
	    comp < SGI_COMP_NONE || comp > SGI_COMP_ARLE)
        {
          free(sgip);
          return (NULL);
        };

        if ((sgip->file = fopen(filename, "wb+")) == NULL)
        {
          free(sgip);
          return (NULL);
        };

        sgip->mode = SGI_WRITE;

        putshort(SGI_MAGIC, sgip);
        putc((sgip->comp = comp) != 0, sgip->file);
        putc(sgip->bpp = bpp, sgip->file);
        putshort(3, sgip);		/* Dimensions */
        putshort(sgip->xsize = xsize, sgip);
        putshort(sgip->ysize = ysize, sgip);
        putshort(sgip->zsize = zsize, sgip);
        if (bpp == 1)
        {
          putlong(0, sgip);	/* Minimum pixel */
          putlong(255, sgip);	/* Maximum pixel */
        }
        else
        {
          putlong(minpixel, sgip);	/* Minimum pixel */
          putlong(maxpixel, sgip);	/* Maximum pixel */
        };
        putlong(0, sgip);		/* Reserved */

        memset(name, 0, sizeof(name));
        if (strrchr(filename, '/') == NULL)
          strncpy(name, filename, sizeof(name) - 1);
        else
          strncpy(name, strrchr(filename, '/') + 1, sizeof(name) - 1);
        fwrite(name, sizeof(name), 1, sgip->file);

        for (i = 0; i < 102; i ++)
          putlong(0, sgip);

        switch (comp)
        {
          case SGI_COMP_NONE : /* No compression */
             /*
              * This file is uncompressed.  To avoid problems with sparse files,
              * we need to write blank pixels for the entire image...
              */

              if (bpp == 1)
              {
        	for (i = xsize * ysize * zsize; i > 0; i --)
        	  putc(0, sgip->file);
              }
              else
              {
        	for (i = xsize * ysize * zsize; i > 0; i --)
        	  putshort(0, sgip);
              };
              break;

          case SGI_COMP_ARLE : /* Aggressive RLE */
              sgip->arle_row    = calloc(xsize, sizeof(short));
              sgip->arle_offset = 0;

          case SGI_COMP_RLE : /* Run-Length Encoding */
             /*
              * This file is compressed; write the (blank) scanline tables...
              */

              for (i = 2 * ysize * zsize; i > 0; i --)
        	putlong(0, sgip);

              sgip->firstrow = ftell(sgip->file);
              sgip->nextrow  = ftell(sgip->file);
              sgip->table    = calloc(sgip->zsize, sizeof(long *));
              sgip->table[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
              for (i = 1; i < sgip->zsize; i ++)
        	sgip->table[i] = sgip->table[0] + i * sgip->ysize;
              sgip->length    = calloc(sgip->zsize, sizeof(long *));
              sgip->length[0] = calloc(sgip->ysize * sgip->zsize, sizeof(long));
              for (i = 1; i < sgip->zsize; i ++)
        	sgip->length[i] = sgip->length[0] + i * sgip->ysize;
              break;
        };
        break;

    default :
        free(sgip);
        return (NULL);
  };

  return (sgip);
}


/*
 * 'sgiPutRow()' - Put a row of image data to a file.
 */

int
sgiPutRow(sgi_t *sgip,	/* I - SGI image */
          short *row,	/* I - Row to write */
          int   y,	/* I - Line to write */
          int   z)	/* I - Channel to write */
{
  int	x;		/* X coordinate */
  long	offset;		/* File offset */


  if (sgip == NULL ||
      row == NULL ||
      y < 0 || y >= sgip->ysize ||
      z < 0 || z >= sgip->zsize)
    return (-1);

  switch (sgip->comp)
  {
    case SGI_COMP_NONE :
       /*
        * Seek to the image row - optimize buffering by only seeking if
        * necessary...
        */

        offset = 512 + (y + z * sgip->ysize) * sgip->xsize * sgip->bpp;
        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            putc(*row, sgip->file);
        }
        else
        {
          for (x = sgip->xsize; x > 0; x --, row ++)
            putshort(*row, sgip);
        };
        break;

    case SGI_COMP_ARLE :
        if (sgip->table[z][y] != 0)
          return (-1);

       /*
        * First check the last row written...
        */

        if (sgip->arle_offset > 0)
        {
          for (x = 0; x < sgip->xsize; x ++)
            if (row[x] != sgip->arle_row[x])
              break;

          if (x == sgip->xsize)
          {
            sgip->table[z][y]  = sgip->arle_offset;
            sgip->length[z][y] = sgip->arle_length;
            return (0);
          };
        };

       /*
        * If that didn't match, search all the previous rows...
        */

        fseek(sgip->file, sgip->firstrow, SEEK_SET);

        if (sgip->bpp == 1)
        {
          do
          {
            sgip->arle_offset = ftell(sgip->file);
            if ((sgip->arle_length = read_rle8(sgip, sgip->arle_row, sgip->xsize)) < 0)
            {
              x = 0;
              break;
            };

            for (x = 0; x < sgip->xsize; x ++)
              if (row[x] != sgip->arle_row[x])
        	break;
          }
          while (x < sgip->xsize);
        }
        else
        {
          do
          {
            sgip->arle_offset = ftell(sgip->file);
            if ((sgip->arle_length = read_rle16(sgip, sgip->arle_row, sgip->xsize)) < 0)
            {
              x = 0;
              break;
            };

            for (x = 0; x < sgip->xsize; x ++)
              if (row[x] != sgip->arle_row[x])
        	break;
          }
          while (x < sgip->xsize);
        };

	if (x == sgip->xsize)
	{
          sgip->table[z][y]  = sgip->arle_offset;
          sgip->length[z][y] = sgip->arle_length;
          return (0);
	}
	else
	  fseek(sgip->file, 0, SEEK_END);	/* Clear EOF */

    case SGI_COMP_RLE :
        if (sgip->table[z][y] != 0)
          return (-1);

        offset = sgip->table[z][y] = sgip->nextrow;

        if (offset != ftell(sgip->file))
          fseek(sgip->file, offset, SEEK_SET);

        if (sgip->bpp == 1)
          x = write_rle8(sgip, row, sgip->xsize);
        else
          x = write_rle16(sgip, row, sgip->xsize);

        if (sgip->comp == SGI_COMP_ARLE)
        {
          sgip->arle_offset = offset;
          sgip->arle_length = x;
          memcpy(sgip->arle_row, row, sgip->xsize * sizeof(short));
        };

        sgip->nextrow      = ftell(sgip->file);
        sgip->length[z][y] = x;

        return (x);
  };

  return (0);
}


/*
 * 'getlong()' - Get a 32-bit big-endian integer.
 */

static int
getlong(sgi_t *sgip)	/* I - SGI image to read from */
{
  unsigned char	b[4];


  fread(b, 4, 1, sgip->file);
  if(sgip->swapBytes)
    return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0]);
  else
    return ((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
}


/*
 * 'getshort()' - Get a 16-bit big-endian integer.
 */

static int
getshort(sgi_t *sgip)	/* I - SGI image to read from */
{
  unsigned char	b[2];


  fread(b, 2, 1, sgip->file);
  if(sgip->swapBytes)
    return ((b[1] << 8) | b[0]);
  else
    return ((b[0] << 8) | b[1]);
}


/*
 * 'putlong()' - Put a 32-bit big-endian integer.
 */

static int
putlong(long n,		/* I - Long to write */
        sgi_t *sgip)	/* I - File to write to */
{
  if (putc(n >> 24, sgip->file) == EOF)
    return (EOF);
  if (putc(n >> 16, sgip->file) == EOF)
    return (EOF);
  if (putc(n >> 8, sgip->file) == EOF)
    return (EOF);
  if (putc(n, sgip->file) == EOF)
    return (EOF);
  else
    return (0);
}


/*
 * 'putshort()' - Put a 16-bit big-endian integer.
 */

static int
putshort(unsigned short n,/* I - Short to write */
         sgi_t *sgip)	/* I - File to write to */
{
  if (putc(n >> 8, sgip->file) == EOF)
    return (EOF);
  if (putc(n, sgip->file) == EOF)
    return (EOF);
  else
    return (0);
}


/*
 * 'read_rle8()' - Read 8-bit RLE data.
 */

static int
read_rle8(sgi_t *sgip,	/* I - File to read from */
          unsigned short *row,	/* O - Data */
          int   xsize)	/* I - Width of data in pixels */
{
  int	i,		/* Looping var */
	ch,		/* Current character */
	count,		/* RLE count */
	length;		/* Number of bytes read... */


  length = 0;

  while (xsize > 0)
  {
    if ((ch = getc(sgip->file)) == EOF)
      return (-1);
    length ++;

    count = ch & 127;
    if (count == 0)
      break;

    if (ch & 128)
    {
      for (i = 0; i < count; i ++, row ++, xsize --, length ++)
        *row = getc(sgip->file);
    }
    else
    {
      ch = getc(sgip->file);
      length ++;
      for (i = 0; i < count; i ++, row ++, xsize --)
        *row = ch;
    };
  };

  return (xsize > 0 ? -1 : length);
}


/*
 * 'read_rle16()' - Read 16-bit RLE data.
 */

static int
read_rle16(sgi_t *sgip,	/* I - File to read from */
           unsigned short *row,	/* O - Data */
           int   xsize)	/* I - Width of data in pixels */
{
  int	i,		/* Looping var */
	ch,		/* Current character */
	count,		/* RLE count */
	length;		/* Number of bytes read... */


  length = 0;

  while (xsize > 0)
  {
    if ((ch = getshort(sgip)) == EOF)
      return (-1);
    length ++;

    count = ch & 127;
    if (count == 0)
      break;

    if (ch & 128)
    {
      for (i = 0; i < count; i ++, row ++, xsize --, length ++)
        *row = getshort(sgip);
    }
    else
    {
      ch = getshort(sgip);
      length ++;
      for (i = 0; i < count; i ++, row ++, xsize --)
        *row = ch;
    };
  };

  return (xsize > 0 ? -1 : length * 2);
}


/*
 * 'write_rle8()' - Write 8-bit RLE data.
 */

static int
write_rle8(sgi_t *sgip,	/* I - File to write to */
           unsigned short *row,	/* I - Data */
           int   xsize)	/* I - Width of data in pixels */
{
  int	length,
	count,
	i,
	x;
  unsigned short	*start,
	repeat;


  for (x = xsize, length = 0; x > 0;)
  {
    start = row;
    row   += 2;
    x     -= 2;

    while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0]))
    {
      row ++;
      x --;
    };

    row -= 2;
    x   += 2;

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putc(128 | i, sgip->file) == EOF)
        return (-1);
      length ++;

      while (i > 0)
      {
	if (putc(*start, sgip->file) == EOF)
          return (-1);
        start ++;
        i --;
        length ++;
      };
    };

    if (x <= 0)
      break;

    start  = row;
    repeat = row[0];

    row ++;
    x --;

    while (x > 0 && *row == repeat)
    {
      row ++;
      x --;
    };

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putc(i, sgip->file) == EOF)
        return (-1);
      length ++;

      if (putc(repeat, sgip->file) == EOF)
        return (-1);
      length ++;
    };
  };

  length ++;

  if (putc(0, sgip->file) == EOF)
    return (-1);
  else
    return (length);
}


/*
 * 'write_rle16()' - Write 16-bit RLE data.
 */

static int
write_rle16(sgi_t *sgip,	/* I - File to write to */
            unsigned short *row,	/* I - Data */
            int   xsize)/* I - Width of data in pixels */
{
  int	length,
	count,
	i,
	x;
  unsigned short	*start,
	repeat;


  for (x = xsize, length = 0; x > 0;)
  {
    start = row;
    row   += 2;
    x     -= 2;

    while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0]))
    {
      row ++;
      x --;
    };

    row -= 2;
    x   += 2;

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putshort(128 | i, sgip) == EOF)
        return (-1);
      length ++;

      while (i > 0)
      {
	if (putshort(*start, sgip) == EOF)
          return (-1);
        start ++;
        i --;
        length ++;
      };
    };

    if (x <= 0)
      break;

    start  = row;
    repeat = row[0];

    row ++;
    x --;

    while (x > 0 && *row == repeat)
    {
      row ++;
      x --;
    };

    count = row - start;
    while (count > 0)
    {
      i     = count > 126 ? 126 : count;
      count -= i;

      if (putshort(i, sgip) == EOF)
        return (-1);
      length ++;

      if (putshort(repeat, sgip) == EOF)
        return (-1);
      length ++;
    };
  };

  length ++;

  if (putshort(0, sgip) == EOF)
    return (-1);
  else
    return (2 * length);
}


/*
 * End of "$Id: sgilib.c,v 1.2 2006/11/24 00:11:21 beku Exp $".
 */
