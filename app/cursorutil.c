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
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include "appenv.h"
#include "cursorutil.h"
#include "brushlist.h" 
#include "gdisplay.h"

void
create_win_cursor (GdkWindow *win, double radius)
{
  int i, j, k=0, char_num=0, num, c; 
  int cursor1_width = radius; 
  int cursor1_height = radius; 
  char *cursor1_bits;
  char *cursor1mask_bits;
  double tmp; 
  GdkCursor *cursor;
  GdkPixmap *source, *mask;
  GdkColor fg = { 0, 65535, 65535, 65535}; 
  GdkColor bg = { 0, 0, 0, 0}; 

  char flag=0, flip=0;
  num = ((cursor1_width) - (cursor1_width / 8) * 8 != 0 ? 1 : 0) + (cursor1_width / 8);

  if (num*8 < 33)
    flag = 1;
  else
    {
     radius = 32;
     cursor1_width = radius;
     cursor1_height = radius;
  num = ((cursor1_width) - (cursor1_width / 8) * 8 != 0 ? 1 : 0) + (cursor1_width / 8);
    } 

  c = radius/2; 

  cursor1_bits = (char*) malloc (sizeof (char) * num* cursor1_height);
  cursor1mask_bits = (char*) malloc (sizeof (char) * num* cursor1_height);


  for (i=0; i<num*cursor1_height; i++)
    {
      cursor1_bits[i] = 0; 
      cursor1mask_bits[i] = 0; 
    }

  if (flag)
    {
 for (j=0; j<cursor1_height; j++)
    for (i=0; i<num*8; i++)
      {
	if (i<cursor1_width)
	  {
	    tmp = (((i-c) * (i-c)) + ((j-c) * (j-c))); 
	    tmp = sqrt (tmp);

	    if (tmp - c >=  0 && tmp - c < 0.5)
	      {
		cursor1_bits [char_num] = cursor1_bits [char_num]  | 1 << k; 	
		cursor1mask_bits [char_num] = cursor1mask_bits [char_num] | 1 << k; 
	      }
	    if (tmp - c+1 >= 0 && tmp - c+1 < 0.5)
	      { 
		cursor1mask_bits [char_num] = cursor1mask_bits [char_num] | 1 << k;
	      }


	  }	
	k++;
	char_num = k==8 ? char_num + 1 : char_num;
	k = k==8 ? 0 : k; 
      }
    }
  /* create the cross */
  k = 0; char_num=0; 
  for (j=0; j<cursor1_height; j++)
    for (i=0; i<num*8; i++)
      {
	if ((i == c || j == c) && i<cursor1_width)
	  {
	   if (flip) 
	    cursor1_bits [char_num] = cursor1_bits [char_num]  | 1 << k;
flip = flip ? 0 : 1;
	    cursor1mask_bits [char_num] = cursor1mask_bits [char_num] | 1 << k; 
	  }	       
	k++;
	char_num = k==8 ? char_num + 1 : char_num;
	k = k==8 ? 0 : k;
      }	       

  source = gdk_bitmap_create_from_data (NULL, cursor1_bits,
      /*cursor1_width*/num*8, cursor1_height);


  mask = gdk_bitmap_create_from_data (NULL, cursor1mask_bits,
      /*cursor1_width*/num*8, cursor1_height);

  if (!source || !mask)
    {
      e_printf ("ERROR: unable to change cursor size\n");
      return; 
    }
  cursor = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, 
      c, c);

  gdk_pixmap_unref (source);
  gdk_pixmap_unref (mask);


  gdk_window_set_cursor (win, cursor);
  gdk_cursor_destroy (cursor);
}

/*
#define CURSOR_DEBUG
*/
void
change_win_cursor (win, cursortype)
     GdkWindow *win;
     GdkCursorType cursortype;
{
	GdkCursor *cursor;
#ifdef CURSOR_DEBUG
	GdkCursorPrivate* c;
#endif
	int width, height, radius;
	if (!win)
	return;

	if (cursortype == GDK_PENCIL)
	{
		if (!gdisplay_active ())
			return;
		width = canvas_width (get_active_brush ()->mask);
		height = canvas_height (get_active_brush ()->mask);
		radius = width > height ? width : height;
		if (gdisplay_active ())
			create_win_cursor (win, radius * ((double) SCALEDEST (gdisplay_active ()) / 
			(double)SCALESRC (gdisplay_active ())));
		return;
	}
	cursor = gdk_cursor_new (cursortype);


#ifdef CURSOR_DEBUG
	c=(GdkCursorPrivate*) cursor;
  d_printf("change_win_cursor set HCURSOR=%x prior=%x\n",c->xcursor,GDK_WINDOW_WIN32DATA(win)->xcursor);
#endif
  gdk_window_set_cursor (win, cursor);
#ifdef CURSOR_DEBUG
  d_printf("change_win_cursor destroy HCURSOR=%x e=%i\n",c->xcursor,GetLastError());
#endif
  gdk_cursor_destroy (cursor);
#ifdef CURSOR_DEBUG
  d_printf("change_win_cursor done e=%i\n",GetLastError());
#endif

}

void
unset_win_cursor (win)
     GdkWindow *win;
{
  gdk_window_set_cursor (win, NULL);
}
     



