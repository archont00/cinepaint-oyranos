/* $RCSfile: utils.h,v $
 * 
 * $Author: robinrowe $
 * $Date: 2004/02/10 01:05:36 $
 * $Revision: 1.1 $
 * 
 * $Log: utils.h,v $
 * Revision 1.1  2004/02/10 01:05:36  robinrowe
 * add 0.18-1
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>

guint32
readInt(FILE *file);

guint32
readShort(FILE *file);

void
writeInt(FILE *file, guint32 v);

void
writeShort(FILE *file, guint32 v);

#endif // UTILS_H
