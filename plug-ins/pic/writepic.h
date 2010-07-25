/* $RCSfile: writepic.h,v $
 * 
 * $Author: robinrowe $
 * $Date: 2004/02/10 01:05:36 $
 * $Revision: 1.1 $
 * 
 * $Log: writepic.h,v $
 * Revision 1.1  2004/02/10 01:05:36  robinrowe
 * add 0.18-1
 *
 */

#ifndef WRITEPIC_H
#define WRITEPIC_H

typedef struct {
	gint32		rle;
	gint32		alpha;
} PicWriteParam;

guint32 writePicImage(char *fileName, gint32 imageID, gint32 drawableID, PicWriteParam *);

#endif // WRITEPIC_H
