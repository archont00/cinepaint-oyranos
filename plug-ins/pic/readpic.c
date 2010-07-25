/* $RCSfile: readpic.c,v $
 * 
 * $Author: robinrowe $
 * $Date: 2004/02/10 01:05:36 $
 * $Revision: 1.1 $
 * 
 * $Log: readpic.c,v $
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

#include "readpic.h"

// ===============================================
// = #DEFINES ===================================
// =============================================

// Data type
#define PIC_UNSIGNED_INTEGER	0x00
#define PIC_SIGNED_INTEGER		0x10	// XXX: Not implemented
#define PIC_SIGNED_FLOAT		0x20	// XXX: Not implemented


// Compression type
#define PIC_UNCOMPRESSED		0x00
#define PIC_PURE_RUN_LENGTH		0x01
#define PIC_MIXED_RUN_LENGTH	0x02

// Channel types (OR'd)
#define PIC_RED_CHANNEL			0x80
#define PIC_GREEN_CHANNEL		0x40
#define PIC_BLUE_CHANNEL		0x20
#define PIC_ALPHA_CHANNEL		0x10
#define PIC_SHADOW_CHANNEL		0x08	// XXX: Not implemented
#define PIC_DEPTH_CHANNEL		0x04	// XXX: Not implemented
#define PIC_AUXILIARY_1_CHANNEL	0x02	// XXX: Not implemented
#define PIC_AUXILIARY_2_CHANNEL	0x01	// XXX: Not implemented

// ===============================================
// = TYPEDEFS ===================================
// =============================================

#define Channel PicChannel

typedef struct __Channel {
	guint8	size;
	guint8	type;
	guint8	channels;
	struct __Channel *next;
} Channel;

// ===============================================
// = DECLARATIONS ===============================
// =============================================


static guint32 readScanlines(FILE *file, GDrawable *drawable, gint32 width, gint32 height, Channel *channel, guint32 alpha);
static guint32 readScanline(FILE *file, guint8 *scan, gint32 width, Channel *channel,  gint32 bytes);
static guint32 channelReadRaw(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes);
static guint32 channelReadPure(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes);
static guint32 channelReadMixed(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes);


// ===============================================
// = PIC READ FUNCTIONS =========================
// =============================================


guint32 readPicImage(char *fileName, gint32 *imageID)
{
	FILE		*file = NULL;
	gint32		layerID;
	guint32		tmp, status;
	guint32		width, height;
	guint32		alpha = FALSE;
	guint8		chained;
	Channel		*channel = NULL;
	GDrawable	*drawable;
	char		*str;
	
	*imageID = -1;
	
	// Write out status message
	str = g_malloc(strlen(fileName) + 12);
	sprintf(str, "Loading %s:", fileName);
	gimp_progress_init(str);
	g_free(str);
	
	if((file = fopen(fileName, "rb")) == NULL)
		goto error;
	
	// This check is almost unneccessary as we have our image certified
	// by the means of a magic number in gimp_load_file. Hey ho..
	
	tmp = readInt(file);
	if(tmp != 0x5380F634) {		// 'S' + 845-1636 (SI's phone no :-)
		g_message("'%s' has invalid magic number in the header.", fileName);
		goto error;
	}
	
	tmp = readInt(file);
	fseek(file, 88, SEEK_SET);	// Skip over creator string
	
	tmp = readInt(file);		// File identifier 'PICT'
	if(tmp != 'PICT') {
		g_message("'%s' is a Softimage file, but not a PIC file.\n", fileName);
		goto error;
	}
	
	width  = readShort(file);
	height = readShort(file);
	
	tmp = readInt(file);		// Aspect ratio (ignored)
	tmp = readShort(file);		// Interlace type
	if(tmp == 0)
		goto error;

	readShort(file);		// Read padding
	
	// Read channels
	
	do {
		Channel	*c;
		
		if(channel == NULL)
			channel = c = g_malloc(sizeof(Channel));
		else {
			c->next = g_malloc(sizeof(Channel));
			c = c->next;
		}
		c->next = NULL;
		
		chained = fgetc(file);
		c->size = fgetc(file);
		c->type = fgetc(file);
		c->channels = fgetc(file);
		
		// See if we have an alpha channel in there
		if(c->channels & PIC_ALPHA_CHANNEL)
			alpha = TRUE;
		
	} while(chained);
	
	// Create the image and the background layer
	*imageID = gimp_image_new(width, height, RGB);
	gimp_image_set_filename(*imageID, fileName);
	layerID = gimp_layer_new(*imageID, "Background", width, height, 
							 (alpha)?(RGBA_IMAGE):(RGB_IMAGE), 100.0, NORMAL_MODE);
	gimp_image_add_layer(*imageID, layerID, 0);
	drawable = gimp_drawable_get(layerID);
	
	if(!readScanlines(file, drawable, width, height, channel, alpha))
		goto error;
	
	gimp_drawable_detach(drawable);
	
	status = TRUE;
	goto noerror;

error:
	status = FALSE;
noerror:
	
	while(channel) {
		Channel		*prev;

		prev = channel;
		channel = channel->next;
		g_free(prev);
	}
	if(!status) {
		if(*imageID != -1)
			gimp_image_delete(*imageID);
		*imageID = 0;
	}
	
	fclose(file);
	return status;
}

static guint32 readScanlines(FILE *file, GDrawable *drawable,
							 gint32 width, gint32 height, 
							 Channel *channel, guint32 alpha)
{
	GPixelRgn	pixelRgn;
	gint32		tileHeight, bytes;
	gint32		i, j;
	guint32		status;
	guint8		*buf;
	
	gimp_pixel_rgn_init(&pixelRgn, drawable, 0, 0, width, height, TRUE, FALSE);
	
	bytes = (alpha)?(4):(3);
	
	tileHeight = gimp_tile_height();
	buf = g_new(guchar, tileHeight * width * bytes);
	
	for(i = 0; i < height; i+= tileHeight) {
		gint32	scanlines = MIN(tileHeight, height - i);
		
		for(j = 0; j < scanlines; j++) {
			guint8	*scan = buf + j * width * bytes;
			
			if(!readScanline(file, scan, width, channel, bytes))
				goto error;

			if(((i + j) % 10) == 0)
				gimp_progress_update((double)(i + j) / (double)height);
		}
		gimp_pixel_rgn_set_rect(&pixelRgn, buf, 0, i, drawable->width, scanlines);
	}
	
	status = TRUE;
	goto noerror;
error:
	status = FALSE;
noerror:
	g_free(buf);
	
	return status;
}

static guint32 readScanline(FILE *file, guint8 *scan, gint32 width, Channel *channel,  gint32 bytes)
{
	gint32		noCol, status;
	gint32		off[4];
	
	while(channel) {
		noCol = 0;
		if(channel->channels & PIC_RED_CHANNEL) {
			off[noCol] = 0;
			noCol++;
		}
		if(channel->channels & PIC_GREEN_CHANNEL) {
			off[noCol] = 1;
			noCol++;
		}
		if(channel->channels & PIC_BLUE_CHANNEL) {
			off[noCol] = 2;
			noCol++;
		}
		if(channel->channels & PIC_ALPHA_CHANNEL) {
			off[noCol] = 3;
			noCol++;
		}
		
		switch(channel->type & 0x0F) {
			case PIC_UNCOMPRESSED:
				status = channelReadRaw(file, scan, width, noCol, off, bytes);
				break;
			case PIC_PURE_RUN_LENGTH:
				status = channelReadPure(file, scan, width, noCol, off, bytes);
				break;
			case PIC_MIXED_RUN_LENGTH:
				status = channelReadMixed(file, scan, width, noCol, off, bytes);
				break;
		}
		if(!status)
			break;
		
		channel = channel->next;
	}
	return status;
}

static guint32 channelReadRaw(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes)
{
	int			i, j;
	
	for(i = 0; i < width; i++) {
		if(feof(file))
			return FALSE;
		for(j = 0; j < noCol; j++)
			scan[off[j]] = (guint8)getc(file);
		scan += bytes;
	}
	return TRUE;
}

static guint32 channelReadPure(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes)
{
	guint8		col[4];
	gint32		count;
	int			i, j, k;
	
	for(i = width; i > 0; ) {
		count = (unsigned char)getc(file);
		if(count > width)
			count = width;
		i -= count;
		
		if(feof(file))
			return FALSE;
		
		for(j = 0; j < noCol; j++)
			col[j] = (guint8)getc(file);
		
		for(k = 0; k < count; k++, scan += bytes) {
			for(j = 0; j < noCol; j++)
				scan[off[j] + k] = col[j];
		}
	}
	return TRUE;
}

static guint32 channelReadMixed(FILE *file, guint8 *scan, gint32 width, gint32 noCol, gint32 *off, gint32 bytes)
{
	gint32	count;
	int		i, j, k;
	guint8	c, col[4];
	
	for(i = 0; i < width; i += count) {
		if(feof(file))
			return FALSE;
		
		count = (guint8)fgetc(file);
		
		if(count >= 128) {		// Repeated sequence
			if(count == 128)	// Long run
				count = readShort(file);
			else
				count -= 127;
			
			// We've run past...
			if((i + count) > width) {
				printf("Overrun scanline (Repeat) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				return FALSE;
			}
			
			for(j = 0; j < noCol; j++)
				col[j] = (guint8)fgetc(file);
			
			for(k = 0; k < count; k++, scan += bytes) {
				for(j = 0; j < noCol; j++)
					scan[off[j]] = col[j];
			}
		} else {				// Raw sequence
			count++;
			if((i + count) > width) {
				printf("Overrun scanline (Raw) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				return FALSE;
			}
			
			for(k = count; k > 0; k--, scan += bytes) {
				for(j = 0; j < noCol; j++)
					scan[off[j]] = (guint8)fgetc(file);
			}
		}
	}
	return TRUE;
}
