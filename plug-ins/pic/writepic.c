/* $RCSfile: writepic.c,v $
 * 
 * $Author: robinrowe $
 * $Date: 2004/02/10 01:05:36 $
 * $Revision: 1.1 $
 * 
 * $Log: writepic.c,v $
 * Revision 1.1  2004/02/10 01:05:36  robinrowe
 * add 0.18-1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "utils.h"

#include "writepic.h"

// ===============================================
// = #DEFINES ===================================
// =============================================

#define SAVE_ID_STRING "CREATOR: The GIMP's Softimage PIC filter. v1.0"

// ===============================================
// = DECLARATIONS ===============================
// =============================================

static gint32 writeRawScanline(FILE *file, int no, guint32 *scan, gint32 alpha)
{
	int		k;
	
	for(k = 0; k < no; k++) {
		fputc((scan[k] >> 24) & 0xFF, file);
		fputc((scan[k] >> 16) & 0xFF, file);
		fputc((scan[k] >> 8) & 0xFF, file);
	}
	if(ferror(file))
		return FALSE;
	if(alpha) {
		for(k = 0; k < no; k++)
			fputc(scan[k] && 0xFF, file);
	}
	if(ferror(file))
		return FALSE;
	return TRUE;
}

static gint32 writeMixedScanline(FILE *file, int no, guint32 *scan, gint32 alpha)
{
	int		same, seq_same, count;
	int		i, k;
	guint32	pixel[128], col;
	
	// Parameter assumptions
	g_assert(file != NULL);
	g_assert(no != 0);
	g_assert(no < 65535);
	g_assert(scan != NULL);
	
	// Do RGB first
	count = 0;
	seq_same = 0;			// Just to keep the compiler happy...
	
	for(k = 0; k < no; k++) {
		col = scan[k] & 0xFFFFFF00;
		
		if(count == 0) {
			pixel[0] = col;
			count++;
		} else
		if(count == 1) {
			seq_same = (col == pixel[0]);
			if(!seq_same)
				pixel[count] = col;

			count++;
		} else
		if(count > 1) {
			if(seq_same)
				same = (col == pixel[0]);
			else
				same = (col == pixel[count - 1]);
			
			if(same ^ seq_same) {
				if(!seq_same) {
					putc((guint8)(count - 2), file);
					for(i = 0; i < count - 1; i++) {
						putc(((pixel[i] >> 24) & 0xFF), file);	// R
						putc(((pixel[i] >> 16) & 0xFF), file);	// G
						putc(((pixel[i] >>  8) & 0xFF), file);	// B
					}
					pixel[0] = pixel[1] = col;
					count = 2;
					seq_same = 1;
				} else {
					if(count < 128)
						putc((guint8) (count + 127), file);
					else {
						putc(128, file);
						writeShort(file, count);
					}
					putc(((pixel[0] >> 24) & 0xFF), file);	// R
					putc(((pixel[0] >> 16) & 0xFF), file);	// G
					putc(((pixel[0] >>  8) & 0xFF), file);	// B
					
					pixel[0] = col;
					count = 1;
				}
			} else {
				if(!same)
					pixel[count] = col;
				
				count ++;
				if((count == 128) && !seq_same) {
					putc(127, file);
					
					for(i = 0; i < count; i++) {
						putc(((pixel[i] >> 24) & 0xFF), file);	// R
						putc(((pixel[i] >> 16) & 0xFF), file);	// G
						putc(((pixel[i] >>  8) & 0xFF), file);	// B
					}
					count = 0;
				}
				if((count == 65535) && seq_same) {
					putc(128, file);
					writeShort(file, count);
					putc(((pixel[0] >> 24) & 0xFF), file);	// R
					putc(((pixel[0] >> 16) & 0xFF), file);	// G
					putc(((pixel[0] >>  8) & 0xFF), file);	// B
					count = 0;
				}
			}
		}
		if(ferror(file))
			return FALSE;
	}
	if(count) {
		if((count == 1) || (!seq_same)) {
			putc((guint8)(count - 1), file);
			for(i = 0; i < count; i++) {
				putc(((pixel[i] >> 24) & 0xFF), file);	// R
				putc(((pixel[i] >> 16) & 0xFF), file);	// G
				putc(((pixel[i] >>  8) & 0xFF), file);	// B
			}
		} else {
			if(count < 128)
				putc((guint8)(count + 127), file);
			else {
				putc(128, file);
				writeShort(file, count);
			}
			putc(((pixel[0] >> 24) & 0xFF), file);	// R
			putc(((pixel[0] >> 16) & 0xFF), file);	// G
			putc(((pixel[0] >>  8) & 0xFF), file);	// B
		}
		if(ferror(file))
			return FALSE;
	}
	
	// Then alpha
	
	if(alpha) {
		count = 0;
		
		for(k = 0; k < no; k++) {
			col = scan[k] & 0xFF;			// Alpha
	
			if(count == 0) {
				pixel[0] = col;
				count++;
			} else
			if(count == 1) {
				seq_same  = (col == pixel[0]);
				
				if(!seq_same) {
					pixel[count] = col;
				}
				count++;
			} else
			if(count > 1) {
				if(seq_same) {
					same  = (col == pixel[0]);
				} else {
					same  = (col == pixel[count - 1]);
				}
				
				if(same ^ seq_same) {
					if(!seq_same) {
						putc((unsigned char)(count - 2), file);
						for(i = 0; i < count - 1; i++) {
							putc(pixel[i] & 0xFF, file);
						}
						pixel[0] = pixel[1] = col;
						count = 2;
						seq_same = 1;
					} else {
						if(count < 128)
							putc((guint8)(count + 127), file);
						else {
							putc(128, file);
							writeShort(file, count);
						}
						putc(pixel[0], file);
						
						pixel[0] = col;
						count = 1;
					}
				} else {
					if(!same) {
						pixel[count] = col;
					}
					count ++;
					if((count == 128) && !seq_same) {
						putc(127, file);
						
						for(i = 0; i < count; i++)
							putc(pixel[i] & 0xFF, file);
						count = 0;
					}
					if((count == 65535) && seq_same) {
						putc(128, file);
						writeShort(file, count);
						putc(pixel[0] & 0xFF, file);
						count = 0;
					}
				}
			}
			if(ferror(file))
				return FALSE;
		}
		if(count) {
			if((count == 1) || (!seq_same)) {
				putc((unsigned char)(count - 1), file);
				for(i = 0; i < count; i++)
					putc(pixel[i] & 0xFF, file);
			} else {
				if(count < 128)
					putc((guint8)(count + 127), file);
				else {
					putc(128, file);
					writeShort(file, count);
				}
				putc(pixel[0] & 0xFF, file);
			}
			if(ferror(file))
				return FALSE;
		}
	}
	
	return TRUE;
}

guint32 writePicImage(char *fileName, gint32 imageID, gint32 drawableID, PicWriteParam *param)
{
	GDrawable	*drawable;
	GPixelRgn	pixelRgn;
	FILE		*file;
	guint		width, height;
	gint32 hasAlpha = FALSE, doAlpha = FALSE;
	guint		i, j;
	guint8		*buf = NULL;
	guint32		*pix = NULL;
	guint32		status = FALSE;
	char		*str, id[80];
	
	
	switch(gimp_drawable_type(drawableID)) {
	case RGBA_IMAGE:
		hasAlpha = TRUE;
		doAlpha = (hasAlpha)?(param->alpha):(FALSE);
	case RGB_IMAGE:
		break;
	default:
		return FALSE;
	}
	
	drawable = gimp_drawable_get(drawableID);
	width	 = drawable->width;
	height	 = drawable->height;
	
	str = g_new(char, strlen(fileName) + 12);
	sprintf(str, "Saving %s:", fileName);
	gimp_progress_init(str);
	g_free(str);
	
	if((file = fopen(fileName, "wb")) == NULL)
		return FALSE;
	
	// Write file header
	writeInt(file, 0x5380F634);		// S + SI's phone no!
	writeInt(file, 0x406001A3);		// 3.5001 in float (SI Version)
	
	memset(id, 0, 80);
	sprintf(id, SAVE_ID_STRING);
	fwrite(str, 1, 80, file);
	
	// Write picture header
	writeInt(file, 0x50494354);		// PIC ID ('PICT')
	writeShort(file, width);
	writeShort(file, height);
	writeInt(file, 0x3F800000);		// Aspect ratio (= 1.000)
	
	writeShort(file, 3);			// Interlace type = FULL
	writeShort(file, 0);			// Padding
	
	// Info for RGB channels
	fputc((doAlpha)?(1):(0), file);		// Chained?
	fputc(8, file);						// Bits per channel
	fputc((param->rle)?(2):(0), file);	// Compression type
	fputc(0xE0, file);					// Channel type = R + G + B
	
	// Info for alpha channel
	if(doAlpha) {
		fputc(0, file);						// Chained = NO
		fputc(8, file);						// Bits per channel
		fputc((param->rle)?(2):(0), file);	// Compression type
		fputc(0x10, file);					// Channel type = A
	}
	if(ferror(file))
		goto error;
	
	if((buf = g_new(guint8, gimp_tile_height() * width * drawable->bpp)) == NULL)
		goto error;
	
	gimp_pixel_rgn_init(&pixelRgn, drawable, 0, 0, width, height, TRUE, FALSE);
	
	for(i = 0; i < height; i += gimp_tile_height()) {
		int		noScan;
		
		noScan = MIN(gimp_tile_height(), height - i);
		gimp_pixel_rgn_get_rect(&pixelRgn, (guint8 *)buf, 0, i, width, noScan);
		
		// If we're not dealing with alpha, we have to convert the pixel info in
		// 'buf' from 24 to 32 bits. Sheer lazyness, can't be bother to change
		// writeScanline
		if(hasAlpha) {
			if(doAlpha) {
				pix = (guint32 *)buf;
			} else {
				pix = g_new(guint32, gimp_tile_height() * width);
				for(j = 0; j < (noScan * width); j++) {
					pix[j] = (buf[j * 4 + 0] << 24) | 
							 (buf[j * 4 + 1] << 16) |
							 (buf[j * 4 + 2] <<  8);
				}
			}
		} else {
			pix = g_new(guint32, gimp_tile_height() * width);
			for(j = 0; j < (noScan * width); j++) {
				pix[j] = (buf[j * 3 + 0] << 24) | 
						 (buf[j * 3 + 1] << 16) |
						 (buf[j * 3 + 2] <<  8);
			}
		}
		
		for(j = 0; j < CAST(guint) noScan; j++) {
			if(param->rle) {
				if(!writeMixedScanline(file, width, pix + j * width, doAlpha))
					goto error;
			} else {
				if(!writeRawScanline(file, width, pix + j * width, doAlpha))
					goto error;
			}
			if(((i + j) % 10) == 0)
				gimp_progress_update((double)(i + j) / (double)height);
		}
		if(!hasAlpha || (hasAlpha && !doAlpha))
			g_free(pix);
		pix = NULL;
	}
	gimp_drawable_detach(drawable);
	
	status = TRUE;
	goto noerror;
error:
	status = FALSE;
noerror:
	if(pix && (!hasAlpha || (hasAlpha && !doAlpha)))
		g_free(pix);
	g_free(buf);
	fclose(file);

	return status;
}
