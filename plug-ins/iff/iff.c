/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Maya IFF image reading and writing code 
 * Copyright (C) 1997-1999 Mike Taylor
 * (email: mtaylor@aw.sgi.com, WWW: http://reality.sgi.com/mtaylor)
 *
 *
 *  Modified by Søren 'kurgan' Jacobsen
 *  1 February 2002 for use with gtk+1.2 on Linux
 *  not thoroughly tested, but seems to work ok with GIMP 1.2 on Redhat 7.2
 *
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
 *
 */

/*
 * This loads and saves a version of the IFF (interchange file format) 
 * image format.  This is a tile based image format used primarily by
 * Alias|Wavefront's Maya line of products.  
 *
 * This will not load the versions of the IFF format supported by the 
 * Commodore Amiga.  If my inclusion of the (.iff) extension for my
 * plug-in confuses any Amiga-ites or an ILBM iff reader wishes to use
 * it, then let me know an I'll drop it from my plug-in.  
 *
 * Bug reports or suggestions should be e-mailed to mtaylor@aw.sgi.com
 */

/* Event history:
 * V 1.0, MT, 18-dec-97: initial version of plug-in
 * V 1.1, MT, 21-may-98: added magic cookie support (thanks to Nicholas Lamb)
 * V 1.2, MT, 21-jun-99: removed check to allow uncompressed files to load
 * V 1.3, MT, 21-dec-99: renamed to MayaIFF because it is a more descriptive
 *                       name for users.  
 *                       Fixed bug in Z-buffer code.
 * V 1.4, MT, 11-jan-00: added some performance enhancements
 */

/* Features
 *  - loads and saves
 *    - RGBA and RGB files 
 *    - Z-Buffer info (read only)
 *
 * NOTE: z-buffer data is read into a separate layer.  z-buffer data is not 
 *       written out though, so data can be lost in loading and saving a file.
 *
 */
 
#define SAVE_ID_STRING  "GIMP Maya IFF (Alias|Wavefront) image file-plugin v1.4 1-feb-2002"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "lib/plugin_main.h"
#include <string.h>
#include <assert.h>
#include <libgimp/stdplugins-intl.h>

/* #define IFF_DEBUG */

#ifdef IFF_DEBUG
static FILE * debugFile = NULL; 
#    define IFF_DEBUG_PRINT(a,b) fprintf(debugFile,a,b);fflush(debugFile)
#else
#    define NDEBUG
#    define IFF_DEBUG_PRINT(a,b) 
#endif

/***************
 * Definitions *
 ***************/
 
typedef struct {
    guint32 tag;
    guint32 start;
    guint32 size;
    guint32 chunkType;
} iff_chunk;

#define RGB_FLAG     1
#define ALPHA_FLAG   2
#define ZBUFFER_FLAG 4

/**************
 * Prototypes *
 **************/

/* Standard Plug-in Functions */

static void   query ();
static void   run   ( char *name, int nparams, GParam *param, 
					  int *nRetVal, GParam **retVal );

/* Local Helper Functions */ 

static gint32   load_image( char *filename );
static gint     save_image( char *filename, gint32 image_ID, gint32 drawable_ID );

static guint16  get_short( FILE * file );
static void     put_short( guint16 value, FILE * file );
static guint32  get_long( FILE * file );
static void     put_long( guint32 value, FILE * file );
static guchar   get_char( FILE * file );
static void     put_char( guchar value, FILE * file );

/******************
 * Implementation *
 ******************/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

static void query (void)
/*
 * Description:
 *     Register the services provided by this plug-in 
 */
{
	static GParamDef load_args[] = {
		{ PARAM_INT32,  "run_mode", "Interactive, non-interactive" },
		{ PARAM_STRING, "filename", "The name of the file to load" },
		{ PARAM_STRING, "raw_filename", "The name entered" },
	};
	static GParamDef load_retVal[] = {
		{ PARAM_IMAGE, "image", "Output image" },
	};
	static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
	static int nload_retVal = sizeof (load_retVal) / 
		                           sizeof (load_retVal[0]);

	static GParamDef save_args[] = {
		{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
		{ PARAM_IMAGE, "image", "Input image" },
		{ PARAM_DRAWABLE, "drawable", "Drawable to save" },
		{ PARAM_STRING, "filename", "The name of the file to save the image in" },
		{ PARAM_STRING, "raw_filename",
		  "The name of the file to save the image in" }
	};
	static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

	gimp_install_procedure ("file_maya_iff_load",
							"loads files of the Maya IFF (Alias|Wavefront) file format",
							"loads files of the Maya IFF (Alias|Wavefront) file format",
							"Michael Taylor",
							"Michael Taylor",
							"1999",
							"<Load>/MayaIFF",
							NULL,
							PROC_PLUG_IN,
							nload_args, nload_retVal,
							load_args, load_retVal);

  gimp_install_procedure ("file_maya_iff_save",
                          "save file in the Maya IFF (Alias|Wavefront) file format",
                          "save file in the Maya IFF (Alias|Wavefront) file format",
                          "Michael Taylor",
                          "Michael Taylor",
                          "1999",
                          "<Save>/MayaIFF",
                          "RGB*, GRAY*",
							PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

	gimp_register_magic_load_handler ("file_maya_iff_load", "iff,tdi", "", 
                                      "8,string,CIMG");
	gimp_register_save_handler ("file_maya_iff_save", "iff,tdi", "");
}

static void run ( char *name, int nparams, GParam *param, int *nRetVal,
				  GParam **retVal )
/* 
 *  Description:
 *      perform registered plug-in function 
 *
 *  Arguments:
 *      name         - name of the function to perform
 *      nparams      - number of parameters passed to the function
 *      param        - parameters passed to the function
 *      nRetVal - number of parameters returned by the function
 *      retVal  - parameters returned by the function
 */
{
	static GParam values[2];
	GStatusType status = STATUS_SUCCESS;
	gint32 image_ID;

	*nRetVal = 1;
	*retVal = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status =STATUS_CALLING_ERROR;

        INIT_I18N_UI();

	if (strcmp (name, "file_maya_iff_load") == 0) {
		/* Perform the image load */
		image_ID = load_image (param[1].data.d_string);
		if (image_ID != -1) {
			/* The image load was successful */
			*nRetVal = 2;
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].type = PARAM_IMAGE;
			values[1].data.d_image = image_ID;
		} else {
			/* The image load falied */
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
	} else if (strcmp (name, "file_maya_iff_save") == 0) {
		if (status == STATUS_SUCCESS) {
			if ( !save_image ( param[3].data.d_string, param[1].data.d_int32,
							   param[2].data.d_int32))
			{
				status = STATUS_EXECUTION_ERROR;
			}
		}
		values[0].data.d_status = status;
		
	} else {
		g_assert (FALSE);
	}
}

	
static guchar get_char( FILE * file )
/* 
 * Description:
 *     Reads an 8-bit character from a file
 */
{
	guchar buf;
	size_t result = 0;
    
	result = fread( &buf, 1, 1, file);
    assert( result == 1 );
	return buf;
}	

static void put_char( guchar value, FILE * file )
/* 
 * Description:
 *     Writes an 8-bit character to a file
 */
{
	fwrite( &value, 1, 1, file);
}
	
static guint16 get_short( FILE * file )
/* 
 * Description:
 *     Reads a 16-bit integer from a file in such a way that the machine's
 *     bit ordering should not matter 
 */
{
	guchar buf[2];
	size_t result = 0;
    
	result = fread( buf, 2, 1, file);
    assert( result == 1 );
	return ( buf[0] << 8 ) + ( buf[1] << 0 );
}
	
static void put_short( guint16 value, FILE * file )
/* 
 * Description:
 *     Writes a 16-bit integer from a file in such a way that the machine's
 *     bit ordering should not matter 
 */
{
	guchar buf[2];
	buf[0] = (guchar)( value >> 8 );
	buf[1] = (guchar)( value % 256 );

	fwrite( buf, 2, 1, file);
}

static guint32 get_long( FILE * file )
/* 
 * Description:
 *     Reads a 16-bit integer from a file in such a way that the machine's
 *     bit ordering should not matter 
 */
{
	guchar buf[4];
	size_t result = 0;

	result = fread( buf, 4, 1, file );
    assert( result == 1 );
	return ( buf[0] << 24 ) + ( buf[1] << 16 ) 
         + ( buf[2] << 8 ) + ( buf[3] << 0 );
}
	
static void put_long( guint32 value, FILE * file )
/* 
 * Description:
 *     Writes a 32-bit integer from a file in such a way that the machine's
 *     bit ordering should not matter 
 */
{
	guchar buf[4];
	buf[0] = (guchar)( ( value >> 24 ) % 256 );
	buf[1] = (guchar)( ( value >> 16 ) % 256 );
	buf[2] = (guchar)( ( value >>  8 ) % 256 );
	buf[3] = (guchar)( value % 256 );

	fwrite( buf, 4, 1, file);
}


/*************************
 * IFF Chunking Routines *
 *************************/

#define CHUNK_STACK_SIZE 32

static gint chunkDepth = -1;
static iff_chunk chunkStack[CHUNK_STACK_SIZE];

static iff_chunk begin_read_chunk( FILE * file )
/*
 *  Description:
 *      begin reading an iff chunk.  The file should be queued up to the 
 *      start of the chunk 
 * 
 *  Arguments:
 *      file      - the file to read from
 *
 *  Return Value:
 *      record containing chunk info
 *      
 */
{
    chunkDepth++;
    assert( chunkDepth < CHUNK_STACK_SIZE );
    
    chunkStack[chunkDepth].start = ftell( file );
    chunkStack[chunkDepth].tag = get_long( file );
    chunkStack[chunkDepth].size = get_long( file );
    
    if ( chunkStack[chunkDepth].tag == 'FOR4' ) {
        /* We have a form, so read the form type tag as well */
        chunkStack[chunkDepth].chunkType = get_long( file );
    } else {
        chunkStack[chunkDepth].chunkType = 0;
    }
    
    IFF_DEBUG_PRINT("Beginning Chunk: %4s",(char*)(&chunkStack[chunkDepth].tag));
    IFF_DEBUG_PRINT("  start: %lu", chunkStack[chunkDepth].start );
    IFF_DEBUG_PRINT("  size: %lu", chunkStack[chunkDepth].size );
    if ( chunkStack[chunkDepth].chunkType != 0 ) {
        IFF_DEBUG_PRINT("  type: %4s",(char*)(&chunkStack[chunkDepth].chunkType));
    }
    IFF_DEBUG_PRINT("  depth: %d\n", chunkDepth );
    return chunkStack[chunkDepth];
}
 
static void end_read_chunk( FILE * file )
/*
 *  Description:
 *      end reading an iff chunk.  Seeks to the end of the chunk.
 * 
 *  Arguments:
 *      file      - the file to read from
 */
{
    guint32 end = chunkStack[chunkDepth].start + chunkStack[chunkDepth].size + 8;
    gint part;
    
    if ( chunkStack[chunkDepth].chunkType != 0 ) {
        end += 4;
    }
    /* Add padding */
    part = end % 4;
    if ( part != 0 ) {
        end += 4 - part;   
    }
    
    fseek( file, end, SEEK_SET );
    IFF_DEBUG_PRINT("Closing Chunk: %4s\n",(char*)(&chunkStack[chunkDepth].tag));
    
    chunkDepth--; 
    assert( chunkDepth >= -1 );
}

static void begin_write_chunk( FILE * file, guint32 tag, guint32 type )
/*
 *  Description:
 *      begin writing an iff chunk.
 * 
 *  Arguments:
 *      file      - the file to write to
 *
 *  Return Value:
 *      record containing chunk info
 *      
 */
{
    chunkDepth++;
    assert( chunkDepth < CHUNK_STACK_SIZE );
    
    chunkStack[chunkDepth].start = ftell( file );
    chunkStack[chunkDepth].tag = tag;
    chunkStack[chunkDepth].chunkType = type;
    put_long( tag, file );
    put_long( 0L, file ); /* Space for size info */
    
    if ( tag == 'FOR4' ) {
        put_long( type, file );
    }
    
    IFF_DEBUG_PRINT("Begin Writing Chunk: %4s",(char*)(&chunkStack[chunkDepth].tag));
    IFF_DEBUG_PRINT("  start: %lu", chunkStack[chunkDepth].start );
    if ( chunkStack[chunkDepth].chunkType != 0 ) {
        IFF_DEBUG_PRINT("  type: %4s",(char*)(&chunkStack[chunkDepth].chunkType));
    }
    IFF_DEBUG_PRINT("  depth: %d\n", chunkDepth );
}

static void end_write_chunk( FILE * file )
/*
 *  Description:
 *      end writing an iff chunk.
 * 
 *  Arguments:
 *      file      - the file to write to
 *      
 */
{
    guint32 end = ftell( file );
    gint part, i;
    
    chunkStack[chunkDepth].size = ( end - chunkStack[chunkDepth].start ) - 8;
    
    /* Add padding */
    part = end % 4;
    if ( part != 0 ) {
        for( i = 0; i < ( 4 - part ); i++ ) {
            put_char( 0, file );
        }
        end += 4 - part;   
        IFF_DEBUG_PRINT("Padding: %d\n",part);
    }
    
    fseek( file, ( chunkStack[chunkDepth].start + 4 ), SEEK_SET );
    put_long( chunkStack[chunkDepth].size, file );
    fseek( file, end, SEEK_SET );
   
    IFF_DEBUG_PRINT("Closing Chunk: %4s\n",(char*)(&chunkStack[chunkDepth].tag));
    IFF_DEBUG_PRINT("End: %u\n",end);
    IFF_DEBUG_PRINT("End pos: %u\n",ftell(file));
    IFF_DEBUG_PRINT("Size: %u\n",chunkStack[chunkDepth].size);
    
    chunkDepth--; 
    assert( chunkDepth >= -1 );
}

static guchar * read_data( FILE * file, int size )
{
	guchar * buf = g_new(guchar, size );
    
    IFF_DEBUG_PRINT("read_data  size: %d\n", size );
    
    fread( buf, size, 1, file);
    
	return buf;
}

/************************
 * Compression Routines *
 ************************/
    
static guchar * decompress_rle( guint32 numBytes,
                                guchar * compressedData,
                                guint32 compressedDataSize, 
						        guint32 * compressedIndex)
/*
 *  Description:
 *      decompress rle encoded data
 * 
 *  Arguments:
 *      file               - the file to read from
 *      numBytes           - the number of bytes to decompress
 *      compressedData     - buffer with the compressed data
 *      compressedDataSize - size of the compressed data
 *      compressedIndex    - index into the compressed data
 *      
 *  Return Value:
 *      pointer to decompressed data (must be freed by client)
 */
{
    guchar * data = g_new(guchar, numBytes );
    guchar nextChar, count;
    guint32 i;
    guint32 byteCount = 0; 
    memset(data,0,numBytes);
/*    for ( i = 0; i<  numBytes; ++i ) {
        data[i] = 0;
    }  
*/    
    IFF_DEBUG_PRINT("Decompressing data %d\n",numBytes);
    
    while ( byteCount < numBytes ) {
		if ( *compressedIndex >= compressedDataSize ) break;
        
        nextChar = compressedData[ *compressedIndex ];
		(*compressedIndex)++;
        
        count = (nextChar & 0x7f) + 1;
        if ( ( byteCount + count ) > numBytes ) break;
        if ( nextChar & 0x80 ) {
            /* We have a duplication run */
 
            IFF_DEBUG_PRINT("duplicate run %d of ",(int)count);
            IFF_DEBUG_PRINT("%2x\n",(int)nextChar);
            
            nextChar = compressedData[ *compressedIndex ];
		    (*compressedIndex)++;
            
            assert( ( byteCount + count ) <= numBytes ); 
            for ( i = 0; i < count; ++i ) {
                data[byteCount] = nextChar;
                byteCount++;
            }
        } else {
            /* We have a verbatim run */
            IFF_DEBUG_PRINT("verbatim run %d\n",(int)count);
            for ( i = 0; i < count; ++i ) {
                data[byteCount] = compressedData[ *compressedIndex ];
		        (*compressedIndex)++;
                byteCount++;
            }
        }
        assert( byteCount <= numBytes ); 
    }
    return data;
    
}

static guchar * decompress_tile_rle( guint16 width,  
                                     guint16 height, guint16 depth,
                                     guchar * compressedData,
                                     guint32 compressedDataSize )
/*
 *  Description:
 *      decompress a tile of the image into a gimp compatible format
 * 
 *  Arguments:
 *      width              - width of the tile
 *      height             - height of the tile
 *      depth              - number of channels in the tile
 *      compressedData     - buffer with the compressed data
 *      compressedDataSize - size of the compressed data
 *      
 *  Return Value:
 *      pointer to decompressed data (must be freed by client)
 *      
 */
{
    guchar * channels[4], * data;
    gint i, j, k, row, column;
	guint32 compressedIndex = 0;
    
    IFF_DEBUG_PRINT("Decompressing tile [ %hd, ",width);
    IFF_DEBUG_PRINT("%hd, ",height);
    IFF_DEBUG_PRINT("%hd ]\n",depth);
    
    /* Decompress the different channels (RGBA) */
    assert(depth<=4);
    for ( i = ( depth - 1 ); i >= 0; --i ) {
        channels[i] = decompress_rle( width * height, compressedData, 
			                          compressedDataSize, &compressedIndex);
    }
          
    /* Merge the channels into the order gimp expects */ 
    data = g_new(guchar, width * height * depth );
    for ( row = 0; row < height; ++row ) {
        i = ( height - row - 1 ) * width;
        j = row * width * depth;
        for ( column = 0; column < width; ++column ) {
            for ( k = 0; k < depth; k++ ) {
                data[j] = channels[k][i];
                j++;
            }
            i++;
        }
    }
    
    for ( i = 0; i < depth; i++ ) {
        g_free( channels[i] );
    }
    
    return data;
}

static guchar * read_uncompressed_tile( FILE * file, guint16 width,  
                                        guint16 height, guint16 depth )
/*
 *  Description:
 *      read a tile that is not compressed.  This occurs in the case where the
 *      a tile has so much change in it that rle encoding only makes it larger.
 * 
 *  Arguments:
 *      file      - the file to read from
 *      width     - width of the tile
 *      height    - height of the tile
 *      depth     - number of channels in the tile
 *      
 *  Return Value:
 *      pointer to pixel data (must be freed by client)
 *      
 */
{
    guchar * data = NULL; 
    guchar pixel[4];
    gint i, j, d, index;
    data = g_new(guchar, width * height * depth );
    
    IFF_DEBUG_PRINT("Begin reading uncompressed tile\n",NULL);
    
    for ( i = ( height - 1 ); i >= 0; --i ) {  
        index = i * width * depth;
        for ( j = 0; j < width; ++j ) { 
            fread( pixel, depth, 1, file ); 
            for ( d = ( depth - 1 ); d >= 0; --d ) { 
                data[index] = pixel[d];
                ++index;
            }
        }
    }
    IFF_DEBUG_PRINT("End reading uncompressed tile\n",NULL);
    
    return data;
}

static guint32 compress_rle( guchar * data, guint32 numBytes,
                             guchar * dest, guint32 destSize )
/*
 *  Description:
 *      compress data using rle encoding and write it to a buffer.  If the
 *      data will not fit into the buffer then 0 is returned
 * 
 *  Arguments:
 *      data      - the data to compress
 *      numBytes  - the number of bytes to compress
 *      dest      - buffer to encode into
 *      destSize  - size of buffer
 * 
 *  Return Value:
 *      size of the compressed data (0 if failed)
 */
 {
    gint state = 0;
    guint32 index = 0;
    gint runCount, i, last;
    guchar nextChar, firstChar, lastChar, run[128];
    guint32 destIndex = 0; 
    
    /* This process is implemented with a finite state machine with
     * the following states:
     *   0 - virgin state...beginning new run
     *   1 - have first character for next run
     *   2 - processing duplicate run
     *   3 - processing verbatim run
     *   4 - in verbatim run, have found two in a row that match
     */
    while ( index < numBytes ) {
        nextChar = data[index];
        
        /* Automatic failure if we run out of destination buffer space */
        /* We need at least two bytes to write another record */
        if ( ( destIndex +2 ) > destSize ) return 0;
        
        switch ( state ) {
            case 0:
                firstChar = nextChar;
                state = 1;
                break;
            case 1:
                if ( nextChar == firstChar ) {
                    /* beginning duplicate run */
                    state = 2;
                    runCount = 2;
                } else {
                    /* beginning verbatim run */
                    state = 3;
                    runCount = 2;
                    lastChar = nextChar;
                    run[0] = firstChar;
                    run[1] = nextChar;
                }
                break;
            case 2:
                if ( ( nextChar == firstChar ) && ( runCount < 128 ) ) {
                    runCount++;
                } else {
                    /* run has ended, write if to the file */
                    dest[destIndex++] = 0x80 | (guchar)( runCount - 1 );
                    dest[destIndex++] = firstChar;
                    
                    IFF_DEBUG_PRINT("duplicate run %d of ",runCount);
                    IFF_DEBUG_PRINT("%2x\n",(int)firstChar);
                    /* nextChar is start of the next run */
                    state = 1;
                    firstChar = nextChar;
                }
                break;
            case 3:
                if ( ( nextChar != lastChar ) && ( runCount < 128 ) ) {
                    run[runCount] = nextChar;
                    lastChar = nextChar;
                    runCount++;
                } else if ( runCount == 127 ) {
                    /* Always better to switch states in this case */
                    /* Found duplicates, end verbatim run */
                    if ( ( destIndex + runCount - 1 ) > destSize ) return 0;
                    dest[destIndex++] = (guchar)( runCount - 2 );
                    last = runCount - 1;
                    for ( i = 0; i < last; ++i ) {
                        dest[destIndex++] = run[i];
                    }
                    IFF_DEBUG_PRINT("verbatim run %d\n",(runCount-1));
                    /* nextChar is start of the next run */
                    state = 2;
                    firstChar = nextChar;
                    runCount = 2;
                } else if ( runCount < 128 ) {
                    /* Found duplicates, possibly end verbatim run */
                    run[runCount] = nextChar;
                    lastChar = nextChar;
                    runCount++;
                    state = 4;
                } else {
                    /* Hit maximum run size */
                    /* Found duplicates, end verbatim run */
                    if ( ( destIndex + runCount ) > destSize ) return 0;
                    dest[destIndex++] = (guchar)( runCount - 1 );
                    for ( i = 0; i < runCount; ++i ) {
                        dest[destIndex++] = run[i];
                    }
                    IFF_DEBUG_PRINT("verbatim run %d\n",runCount);
                    /* nextChar is start of the next run */
                    state = 1;
                    firstChar = nextChar;
                }
                break;
            case 4:
                if ( nextChar != lastChar ) {
                    /* Only run of two, need three to start a duplicate run */
                    run[runCount] = nextChar;
                    lastChar = nextChar;
                    runCount++;
                    state = 3;
                } else {
                    /* We have three in a row, start a duplicate run */
                    if ( ( destIndex + runCount - 2 ) > destSize ) return 0;
                    dest[destIndex++] = (guchar)( runCount - 3 );
                    last = runCount - 2;
                    for ( i = 0; i < last; ++i ) {
                        dest[destIndex++] = run[i];
                    }
                    IFF_DEBUG_PRINT("verbatim run %d\n",(runCount-2));
                    /* nextChar is start of the next run */
                    state = 2;
                    firstChar = nextChar;
                    runCount = 3;
                } 
                break;
        }
        index++;
    }
    
    if ( ( destIndex + 2 ) > destSize ) return 0;
    
    /* clean up at end of input */
    switch ( state ) {
        case 1:
            dest[destIndex++] = 0;
            dest[destIndex++] = firstChar;
            break;
        case 2:
            /* run has ended, write if to the file */
            dest[destIndex++] = 0x80 | (guchar)( runCount - 1 );
            dest[destIndex++] = firstChar;
            break;
        case 3:
        case 4:
            if ( ( destIndex + runCount ) > destSize ) return 0;
            dest[destIndex++] = (guchar)( runCount - 1 );
            for ( i = 0; i < runCount; ++i ) {
                dest[destIndex++] = run[i];
            }
            break;
        default:
            assert(FALSE);
    }
    
    /* Make sure at least some compression is achieved */
    if ( destIndex >= destSize ) return 0;
    
    return destIndex;
}

static void write_uncompressed_tile( FILE * file, guchar * data, 
                                         guint16 width, guint16 height, 
                                         guint16 depth )
/*
 *  Description:
 *      write an uncompressed tile.  This makes sense in the cases where 
 *      the data has no runs in it and rle encoding actually makes the data
 *      larger.
 * 
 *  Arguments:
 *      file      - the file to write to
 *      data      - data in gimp format
 *      width     - width of the tile
 *      height    - height of the tile
 *      depth     - number of channels in the tile
 *      
 */
{ 
    guchar pixel[4];
    gint i, j, d, index;
    
    IFF_DEBUG_PRINT("Begin writing uncompressed tile\n",NULL);
    
    for ( i = ( height - 1 ); i >= 0; --i ) {  
        index = i * width * depth;
        for ( j = 0; j < width; ++j ) {  
            for ( d = ( depth - 1 ); d >= 0; --d ) { 
                pixel[d] = data[index];
                ++index;
            }
            fwrite( pixel, depth, 1, file ); 
        }
    }
    
    IFF_DEBUG_PRINT("End writing uncompressed tile\n",NULL);
}

static void compress_tile_rle( FILE * file, guchar * data, guint16 width,  
                               guint16 height, guint16 depth )
/*
 *  Description:
 *      Compress a tile into the given file.
 * 
 *  Arguments:
 *      file      - the file to write too
 *      data      - data in gimp format
 *      width     - width of the tile
 *      height    - height of the tile
 *      depth     - number of channels in the tile
 *      
 */
{
    guchar * cdata = NULL; 
    guchar * tile = NULL; 
    guint32 cdataIndex = 0;
    guint32 size, maxSize;
    gint i, j, d, index;
    
    tile = g_new( guchar, width * height );
    
    /* The data will never be larger than this */
    maxSize = width * height * depth;
    cdata = g_new(guchar, maxSize );
          
    for ( d = ( depth - 1 ); d >= 0; --d ) {
        index = 0;
        for ( i = ( height - 1 ); i >= 0; --i ) {
            for ( j = 0; j < width; ++j ) {
                tile[index] = data[ ( ( ( i * width ) + j ) * depth ) + d];
                index++;
            }
        }
        IFF_DEBUG_PRINT( "Layer Data Begins at %u\n", ftell(file) );
        size = compress_rle( tile, ( height * width ), 
                             cdata + cdataIndex, maxSize - cdataIndex );
        if ( size == 0 ) {
            /* compressed data is larger than the original */
            write_uncompressed_tile( file, data, width, height, depth );
            return;
        }
        IFF_DEBUG_PRINT( "Layer Data Ends at %u\n", ftell(file) );
        cdataIndex += size;
    }
    fwrite( cdata, cdataIndex, 1, file );
    g_free( tile );
    g_free( cdata );
}   



/****************************
 * Main Plug-in IO Routines *
 ****************************/

static gint32 load_image (char *filename)
/*
 *  Description:
 *      load the given image into gimp
 * 
 *  Arguments:
 *      filename      - name on the file to read
 *
 *  Return Value:
 *      Image id for the loaded image  
 *      
 */
{
	gint        j;
	FILE      * file = NULL;
	gchar     * progMessage;
	TileDrawable * drawable;
	TileDrawable * z_drawable;
	gint32      image_ID;
	gint32      layer_ID, z_layer_ID;
	GPixelRgn   pixel_rgn, z_pixel_rgn;
    iff_chunk   chunkInfo;
    
    /* Header info */
	guint32     width, height, flags, compress;
    guint16     tiles, depth;
    
	GImageType  imgtype;
	GImageType gdtype; //skulle nok være gimpdrawabletype
    
#ifdef IFF_DEBUG    
    debugFile = fopen("IffDebug.txt","wb");
#endif

	/* Set up progress display */
	progMessage = g_malloc(strlen (filename) + 12);
	if (!progMessage) gimp_quit ();
	sprintf (progMessage, "%s %s:",_("Loading"), filename);
	gimp_progress_init (progMessage);
	g_free (progMessage);

	IFF_DEBUG_PRINT("Loading file: %s\n", filename);

	/* Open the file */
	file = fopen( filename, "rb" );
	if ( NULL == file ) {
		return -1;
	}
 
    /* File should begin with a FOR4 chunk of type CIMG */
    chunkInfo = begin_read_chunk( file );
    if ( chunkInfo.chunkType != 'CIMG' ) {
        return -1;
    }
    IFF_DEBUG_PRINT("Magic cookie found\n",NULL);
    
    /* Read the image header */
    while ( TRUE ) {
        chunkInfo = begin_read_chunk( file );
                
        if ( chunkInfo.tag == 'TBHD' ) {
            /* Header chunk found */
            width = get_long( file );
            height = get_long( file );
            get_short( file ); /* Don't support */
            get_short( file ); /* Don't support */
            flags = get_long( file );
            get_short( file ); /* Don't support */
            tiles = get_short( file );
            compress = get_long( file );
            
            IFF_DEBUG_PRINT("Width: %u\n",width);
            IFF_DEBUG_PRINT("Height: %u\n",height);
            IFF_DEBUG_PRINT("flags: %u\n",flags);
            IFF_DEBUG_PRINT("tiles: %hu\n",tiles);
            IFF_DEBUG_PRINT("compress: %u\n",compress);
            end_read_chunk( file );    
            
            if ( compress > 1 ) {
                g_print("Unsupported Compression Type\n");
                return -1;
            }
            
            break;
        } else {
            /* Skipping unused data */
            IFF_DEBUG_PRINT("Skipping chunk: %4s\n",(char*)&(chunkInfo.tag));
            end_read_chunk( file );
        }
    }
    
    depth = 0;
    
    if ( flags & RGB_FLAG ) {
        depth += 3;
    } 
    if ( flags & ALPHA_FLAG ) {
        depth += 1;
    }
    
    switch ( depth ) {
        case 1:
		    /* Just alpha or zbuffer data.  Load as grey scale */
		    imgtype = GRAY;
		    gdtype = GRAY_IMAGE;
            break;
        case 3:
		    /* Just RGB data. */
		    imgtype = RGB;
		    gdtype = RGB_IMAGE;
            break;
        case 4:
        case 0:
		    /* RGB data with alpha. */
		    imgtype = RGB;
		    gdtype = RGBA_IMAGE;
            break;
    }
            
    image_ID = gimp_image_new ( width, height, imgtype );
	gimp_image_set_filename ( image_ID, filename );
    
    if ( depth > 0 ) {
	    layer_ID = gimp_layer_new ( image_ID, "Background",
	    							width,
	    							height,
	    							gdtype, 100, NORMAL_MODE );
	    gimp_image_add_layer ( image_ID, layer_ID, 0);
	    drawable = gimp_drawable_get ( layer_ID );
	    gimp_pixel_rgn_init ( &pixel_rgn, drawable, 0, 0, drawable->width,
	    					  drawable->height, TRUE, FALSE );
    } 
     
    if ( flags & ZBUFFER_FLAG ) {
	    z_layer_ID = gimp_layer_new ( image_ID, _("Z-Buffer"),
	    							  width,
	    							  height,
	    							  RGBA_IMAGE, 100, NORMAL_MODE );
	    gimp_image_add_layer ( image_ID, z_layer_ID, 1);
	    z_drawable = gimp_drawable_get ( z_layer_ID );
	    gimp_pixel_rgn_init ( &z_pixel_rgn, z_drawable, 0, 0, z_drawable->width,
	    					  z_drawable->height, TRUE, FALSE );
           
    } 
    
    /* Read the tiled image data */
    while ( TRUE ) {
        guint16 tile_area;
        chunkInfo = begin_read_chunk( file );
               
        if ( chunkInfo.chunkType == 'TBMP' ) {
            /* Image data found */
            guint16 x1, x2, y1, y2, reverse_y, tile_width, tile_height;
            guint32 zmin, zmax;
            gdouble znormalfactor;
            guint32 tile = 0;
            guint32 ztile = 0;
            size_t fpos;
            gboolean is_zbuffer_normization_pass, iscompressed;
            guchar * zdata, * tileData;
			guint32 remainingDataSize;
            
            IFF_DEBUG_PRINT("Reading image tiles\n",NULL);
            
            /* Do initialization of zbuffer info */
            if ( !( flags & ZBUFFER_FLAG ) ) {
                /* No zbuffer data */
                ztile = tiles;
                is_zbuffer_normization_pass = FALSE;
            } else {
                /* Initialize for normalization path */
                zmin = ~0; zmax = 0;
                fpos = ftell( file );
                is_zbuffer_normization_pass = TRUE;
            }  
            
            if ( depth == 0 ) tile = tiles;
            
            /* Read tiles */
            while( ( tile < tiles ) || ( ztile < tiles ) ) {
                chunkInfo = begin_read_chunk( file );
                assert(chunkInfo.tag=='RGBA'||chunkInfo.tag=='ZBUF');
                
                /* Get tile size and location info */
                x1 = get_short( file );
                y1 = get_short( file );
                x2 = get_short( file );
                y2 = get_short( file );
                
				remainingDataSize = chunkInfo.size - 8;
                
                tile_width = x2 - x1 + 1;
                tile_height = y2 - y1 + 1;
                tile_area = tile_width * tile_height;
                IFF_DEBUG_PRINT("Tile x1: %hu  ",x1);
                IFF_DEBUG_PRINT("y1: %hu  ",y1);
                IFF_DEBUG_PRINT("x2: %hu  ",x2);
                IFF_DEBUG_PRINT("y2: %hu\n",y2);
                
                if ( chunkInfo.size >= 
                     ( tile_width * tile_height * depth + 8 ) ) {
                    /* Compression was not used for this tile */
                    iscompressed = FALSE;
                } else {
                    iscompressed = TRUE;   
                }
                
                if ( is_zbuffer_normization_pass ) {
                    /* File actually stores 24-bits worth of data for Z, so to 
                     * use as much as we can in an 8-bit buffer, we'll 
                     * normalize it 
                     */
                    if ( chunkInfo.tag != 'ZBUF' ) {
                        end_read_chunk( file );
                        continue;
                    }
                    if ( iscompressed ) {
                        guchar * data = read_data( file, remainingDataSize );
                        zdata = decompress_tile_rle( tile_width, 
                                                     tile_height, 4,
                                                     data, remainingDataSize );
                        g_free( data );
                    } else {
                        zdata = read_uncompressed_tile( file, tile_width, 
                                                        tile_height, 4 );
                    }
                    
                    /* Scan the data looking for new max/min values */
                    for ( j = 0; j < tile_area; ++j ) {
                        guint32 value, index;
                        index = ( j * 4 );
                        
                        if ( zdata[index+3] > 0 ) {
                            /* Pixel has Z value */
                            value = ( zdata[index+3] << 24 ) +
									( zdata[index+2] << 16 ) +
                                    ( zdata[index+1] << 8 ) +
                                    ( zdata[index] << 0 );
                            zmin = MIN(zmin,value);
                            zmax = MAX(zmax,value);
                        }
                    }
                    g_free( zdata );
                    end_read_chunk( file );
                    ztile++;
                    if ( ztile >= tiles ) {
                        /* Finished normalization pass, reset to start */
                        ztile = 0;
                        is_zbuffer_normization_pass = FALSE;
                        znormalfactor = (gdouble)( zmax - zmin ) / 256.0;
                        fseek( file, fpos, SEEK_SET );
                        
                        IFF_DEBUG_PRINT("Z-Buffer Range [ %f, ",
                                        ((double)zmin/0.16777216e8));
                        IFF_DEBUG_PRINT("%f ]\n",
                                        ((double)zmax/0.16777216e8));
                    }
                    continue;
                } 
                           
                if ( chunkInfo.tag == 'RGBA' ) {
                    /* We have an RGBA chunk */
                    if ( depth == 0 ) {
                        end_read_chunk( file );
                        continue;
                    }
                    if ( iscompressed ) {
                        guchar * data = read_data( file, remainingDataSize );
                        tileData = decompress_tile_rle( tile_width, 
                                                        tile_height, depth,
                                                        data, 
                                                        remainingDataSize );
                        g_free( data );
                    } else {
                        tileData = read_uncompressed_tile( file, tile_width,
                                                           tile_height, depth );
                    }
 
                    if ( NULL != tileData ) {
                        /* Format reverses y coord compared to gimp */
                        reverse_y = ( height - y1 ) - tile_height;
 
                        gimp_pixel_rgn_set_rect( &pixel_rgn, tileData, x1,
			                                     reverse_y,
            	    					         tile_width, tile_height );
			            g_free( tileData );
		            }
                    end_read_chunk( file );
                    tile++;
                } else if ( chunkInfo.tag == 'ZBUF' ) {
                    /* We have an ZBUF chunk */
                    if ( iscompressed ) {
                        guchar * data = read_data( file, remainingDataSize );
                        tileData = decompress_tile_rle( tile_width, 
                                                        tile_height, 4,
                                                        data, 
                                                        remainingDataSize );
                        g_free( data );
                    } else {
                        tileData = read_uncompressed_tile( file, tile_width,  
                                                           tile_height, 4 );
                    }
                    for ( j = 0; j < tile_area; ++j ) {
                        guint32 value, index;
                        guchar normalized;
                        index = ( j * 4 );
                        
                        if ( tileData[index+3] > 0 ) {
                            value = ( tileData[index+3] << 24 ) +
									( tileData[index+2] << 16 ) +
                                    ( tileData[index+1] << 8 ) +
                                    ( tileData[index] << 0 );
                          
                            /* normalize value */
                            if ( !((value>=zmin)&&(value<=zmax)) ) {
                                fprintf(stderr,"Bad value: %u (%u,%u)\n",value,zmin,zmax);
                            }
                            assert((value>=zmin)&&(value<=zmax));
                            
							value = (guint32)( (gdouble)( value - zmin ) / 
                                      znormalfactor );
							value = MIN(value,(guint32)0xff);
                            normalized = (guint8)value;
                        } else {
                            normalized = 0;
                        }
                        tileData[index] = normalized;
                        tileData[index+1] = normalized;
                        tileData[index+2] = normalized;
                        tileData[index+3] = 0xFF;
                    }
                    
                    if ( NULL != tileData ) {
                        /* Format reverses y coord compared to gimp */
                        reverse_y = ( height - y1 ) - tile_height;
 
                        gimp_pixel_rgn_set_rect( &z_pixel_rgn, tileData, x1,
			                                     reverse_y,
            	    					         tile_width, tile_height );
			            g_free( tileData );
		            }
                    end_read_chunk( file );
                    ztile++;
                }
 
                gimp_progress_update ((double)(MIN(tile,ztile)) / (double) tiles);
			}
            end_read_chunk( file ); /* End TBMP */
 
            if ( depth > 0 ) {
	            gimp_drawable_flush (drawable);
	            gimp_drawable_detach (drawable);
            }
            if ( flags & ZBUFFER_FLAG ) {
	            gimp_drawable_flush (z_drawable);
	            gimp_drawable_detach (z_drawable);
            } 
 
            break;
        } else {
            /* Skipping unused data */
            IFF_DEBUG_PRINT("Skipping chunk: %4s\n",(char*)&(chunkInfo.tag));
            end_read_chunk( file );
        }
    }
    
    end_read_chunk( file ); /* End FORM4 */
    
            
	fclose( file );
    
    IFF_DEBUG_PRINT("Returning image ID: %d\n",image_ID);
    
    
#ifdef IFF_DEBUG    
    fclose(debugFile);
#endif
    
	return image_ID;
}


static gint save_image( char *filename,  gint32 image_ID, gint32 drawable_ID )
/* 
 *  Description:
 *      save the given file out as an alias IFF or matte file
 * 
 *  Arguments: 
 *      filename    - name of file to save to
 *      image_ID    - ID of image to save
 *      drawable_ID - current drawable
 */
{
	gint        depth, row, column, tile_x, tile_y;
	guchar    * src; 
	gchar     * progMessage;
	TileDrawable * drawable;
	GPixelRgn   pixel_rgn;
	FILE      * file;
    guint32     flags, tile_size, tiles_in_x, tiles_in_y, tiles;
    guint32     tile_width, tile_height;
    guint16     x1, y1, x2, y2;

#ifdef IFF_DEBUG    
    debugFile = fopen("IffDebug.txt","wb");
#endif
    
	/* Get info about image */
	drawable = gimp_drawable_get(drawable_ID);

	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
						drawable->height, FALSE, FALSE);

	switch (gimp_drawable_type(drawable_ID))
	{
		case GRAY_IMAGE:
		case GRAYA_IMAGE:
            g_print(_("Greyscale images are not supported\n"));
		    return (FALSE);
		case RGB_IMAGE:
			depth = 3;
            flags = RGB_FLAG;
			break;
		case RGBA_IMAGE:
            flags = RGB_FLAG | ALPHA_FLAG;
			depth = 4;
			break;
		default:
			return FALSE;
	};

	
	/* Open the output file. */
	file = fopen (filename, "wb");
	if ( !file ) {
		return (FALSE);
	}

	/* Set up progress display */
	progMessage = g_malloc(strlen (filename) + 12);
	if (!progMessage) gimp_quit ();
	sprintf (progMessage, "%s %s:", _("Saving"), filename);
	gimp_progress_init (progMessage);
	g_free (progMessage);

    begin_write_chunk( file, 'FOR4', 'CIMG' );
    
    /* Write version number */
    begin_write_chunk( file, 'FVER', 0L );
	put_short( 1, file ); 
	put_short( 1, file ); 
    end_write_chunk( file );
    
	/* Write the image header */
    begin_write_chunk( file, 'TBHD', 0L );
    
	IFF_DEBUG_PRINT( "Width %hu\n", drawable->width );
	IFF_DEBUG_PRINT( "Height %hu\n", drawable->height );
	put_long( (guint32)(drawable->width), file );
	put_long( (guint32)(drawable->height), file );
	put_short( 1, file ); /* don't support */
	put_short( 1, file ); /* don't support */
    put_long( flags, file );
	put_short( 0, file ); /* don't support */
    
    /* Use the gimp's tile size for efficiency and simplicity */
    tile_size = gimp_tile_height();
	IFF_DEBUG_PRINT( "Tile size: %u\n", tile_size );
    tiles_in_x = ( drawable->width + ( tile_size - 1 ) ) / tile_size;
    tiles_in_y = ( drawable->height + ( tile_size - 1 ) ) / tile_size;
	tiles = tiles_in_x * tiles_in_y;
    put_short( (guint16)tiles, file );
    
    put_long( 1, file ); /* compression type */
	put_long( 0, file ); /* don't support */
	put_long( 0, file ); /* don't support */
    
    end_write_chunk( file ); /* BMHD */
	/* End Header */
    
    /* Write Fields Block */
    begin_write_chunk( file, 'FLDS', 0L );
    begin_write_chunk( file, 0x464d5401, 0L );
    put_long( 0x53474900, file );
    end_write_chunk( file ); /* FMT */
    put_long( 0L, file );
    put_long( 0L, file );
    end_write_chunk( file ); /* FLDS */
   
    /* Write Image Tiles */
    begin_write_chunk( file, 'FOR4', 'TBMP' );
    
    src = g_new( guchar, tile_size * tile_size * depth );
    
    for ( row = ( tiles_in_y - 1 ); row >= 0; --row ) {
        for ( column = 0; column < tiles_in_x; column++ ) {
            tile_x = column * tile_size;
            tile_width = MIN( tile_size, ( drawable->width - tile_x ) );

            tile_y = drawable->height - ( ( tiles_in_y - row ) * tile_size );
            tile_y = MAX( tile_y, 0 );
            if ( ( row == 0 ) && ( ( drawable->height % tile_size ) != 0 ) ) {
                tile_height = drawable->height % tile_size;
            } else {
                tile_height = tile_size;
            }
            
            IFF_DEBUG_PRINT( "Writing Tile - x: %d  ",(int)tile_x);
            IFF_DEBUG_PRINT( "y: %d  ",(int)tile_y);
            IFF_DEBUG_PRINT( "width: %d  ",(int)tile_width);
            IFF_DEBUG_PRINT( "height: %d\n",(int)tile_height);
            
            gimp_pixel_rgn_get_rect( &pixel_rgn, src, tile_x, tile_y, 
									 tile_width, tile_height );
            
            begin_write_chunk( file, 'RGBA', 0L );
            x1 = (guint16)tile_x;
            y1 = (guint16)( drawable->height - ( tile_y + tile_height ) );
            x2 = (guint16)( tile_x + tile_width - 1 );
            y2 = (guint16)( drawable->height - tile_y - 1 );
            put_short( x1, file ); put_short( y1, file );
            put_short( x2, file ); put_short( y2, file );
            
            IFF_DEBUG_PRINT( "Writing - x1: %hu  ", x1 );
            IFF_DEBUG_PRINT( "y1: %hu  ", y1 );
            IFF_DEBUG_PRINT( "x2: %hu  ", x2 );
            IFF_DEBUG_PRINT( "y2: %hu\n", y2 );
            
            compress_tile_rle( file, src, tile_width, tile_height, depth );
            
            end_write_chunk( file ); /* RGBA */
        }
        gimp_progress_update ((double)(tiles_in_y - row) / (double)tiles_in_y);
    } 
    
    g_free( src );
    end_write_chunk( file ); /* FOR4 - TBMP */

    end_write_chunk( file ); /* FOR4 - CIMG */
    
	fclose( file );
    
#ifdef IFF_DEBUG    
    fclose(debugFile);
#endif
    
	return (1);
}
