/* $RCSfile: utils.c,v $
 * 
 * $Author: robinrowe $
 * $Date: 2004/02/10 01:05:36 $
 * $Revision: 1.1 $
 * 
 * $Log: utils.c,v $
 * Revision 1.1  2004/02/10 01:05:36  robinrowe
 * add 0.18-1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "utils.h"

guint32
readInt(FILE *file)
{
	guint32	v;
	guint8	c;
	
	v = 0;
	c = fgetc(file);
	v |= (c << 24);
	c = fgetc(file);
	v |= (c << 16);
	c = fgetc(file);
	v |= (c << 8);
	c = fgetc(file);
	v |= c;
	
	return v;
}

guint32
readShort(FILE *file)
{
	gint32	v;
	guint8	c;

	v = 0;
	c = fgetc(file);
	v |= (c << 8);
	c = fgetc(file);
	v |= c;
	
	return v;
}

void
writeInt(FILE *file, guint32 v)
{
	fputc((v >> 24) & 0xFF, file);
	fputc((v >> 16) & 0xFF, file);
	fputc((v >> 8) & 0xFF, file);
	fputc(v & 0xFF, file);
}

void
writeShort(FILE *file, guint32 v)
{
	putc((v >> 8) & 0xFF, file);
	putc(v & 0xFF, file);
}
