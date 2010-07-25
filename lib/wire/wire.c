/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include "event.h"
#else
#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#include <unistd.h>
#include "wire.h"
#include "protocol.h"
#include "wirebuffer.h"
#include "../app/plug_in.h"

#include "iodebug.h"

#ifdef _DEBUG
/* #define DEBUG_WRITE_STRING
#define DEBUG_WIRE_BYTES */
#define DEBUG_WIRE
#endif

const char* gp_name[] = {
	"GP_ERROR",
	"GP_QUIT",
	"GP_CONFIG",
	"GP_TILE_REQ",
	"GP_TILE_ACK",
	"GP_TILE_DATA",
	"GP_PROC_RUN",
	"GP_PROC_RETURN",
	"GP_TEMP_PROC_RUN",
	"GP_TEMP_PROC_RETURN",
	"GP_PROC_INSTALL",
	"GP_PROC_UNINSTALL",
	"GP_EXTENSION_ACK"
};

const char* Get_gp_name(int param)
{	if(param<0)
	{	return "Garbage in Param Type";
	}
	if(param>12)
	{	return "Unknown Param Type";
	}
	return gp_name[param];
}

typedef struct WireHandler  WireHandler;

struct WireHandler
{
  guint32 type;
  WireReadFunc read_func;
  WireWriteFunc write_func;
  WireDestroyFunc destroy_func;
};


static void  wire_init    (void);
static guint wire_hash    (guint32 *key);
static gint  wire_compare (guint32 *a,
			   guint32 *b);


static GHashTable *wire_ht = NULL;
static WireIOFunc wire_read_func = NULL;
static WireIOFunc wire_write_func = NULL;
static WireFlushFunc wire_flush_func = NULL;
static int wire_error_val = FALSE;

void
wire_register (guint32         type,
	       WireReadFunc    read_func,
	       WireWriteFunc   write_func,
	       WireDestroyFunc destroy_func)
{
  WireHandler *handler;

  if (!wire_ht)
    wire_init ();

  handler = g_hash_table_lookup (wire_ht, &type);
  if (!handler)
    handler = g_new (WireHandler, 1);

  handler->type = type;
  handler->read_func = read_func;
  handler->write_func = write_func;
  handler->destroy_func = destroy_func;

  g_hash_table_insert (wire_ht, &handler->type, handler);
}

void wire_set_reader(WireIOFunc read_func)
{	wire_read_func = read_func;
}

void wire_set_writer(WireIOFunc write_func)
{	wire_write_func = write_func;
}

void wire_set_flusher(WireFlushFunc flush_func)
{	wire_flush_func = flush_func;
}

#ifdef WIN32

int wire_memory_read(int fd, guint8 *buf, gulong count)
{/*
#ifdef _DEBUG
	Plugin* plugin;
	plugin=(PlugIn*)fd;
#endif*/
	gulong buffer_bytes;
	guint8* buffer;
	buffer_bytes=wire_buffer->buffer_count;
#ifdef DEBUG_WIRE_BYTES
	d_printf("DEBUG_WIRE_BYTES wire_memory_read: %i - %i = %i\n",buffer_bytes,count,buffer_bytes-count);
#endif
	if(buffer_bytes<count)
	{	return FALSE;
	}
	buffer = wire_buffer->buffer + wire_buffer->buffer_read_index;
	memcpy(buf,buffer,count);
	wire_buffer->buffer_count -= count;
	wire_buffer->buffer_read_index += count;
	if(!wire_buffer->buffer_count)
	{	wire_buffer->buffer_read_index = 0;
		wire_buffer->buffer_write_index = 0;
	}
	return TRUE;
}

int wire_memory_write(int fd,guint8 *buf,gulong count)
{	gulong buffer_bytes;
	guint8* buffer;
	if(!wire_buffer)
	{	e_printf("wire_memory_write: error no wire_buffer\n");
		return FALSE;
	}
	buffer_bytes=wire_buffer->buffer_count;
#ifdef DEBUG_WIRE_BYTES
	d_printf("DEBUG_WIRE_BYTES wire_memory_write: %i + %i = %i (%i max)\n",buffer_bytes,count,buffer_bytes+count,WIRE_BUFFER_SIZE);
#endif
	if((buffer_bytes + count) >= WIRE_BUFFER_SIZE)
	{	e_printf("wire_memory_write: error WIRE_BUFFER_SIZE %i\n",buffer_bytes+count);
		return FALSE;
	}
	buffer = wire_buffer->buffer + wire_buffer->buffer_write_index;
	memcpy(buffer,buf,count);
	wire_buffer->buffer_write_index += count;
	wire_buffer->buffer_count += count;
	return TRUE;
}

int wire_memory_flush(int fd)
{	return TRUE;
}

#else

int wire_file_flush (int fd)
{
  int count;
  int bytes;
  if(!wire_buffer)
  {	return FALSE;
  }
  if (wire_buffer->buffer_count > 0)
    {
      count = 0;
      while (count != wire_buffer->buffer_count)
        {
	  do {
	    bytes = write (fd, &wire_buffer->buffer[count], (wire_buffer->buffer_count - count));
	  } while ((bytes == -1) && (errno == EAGAIN) && (errno != EPIPE));

	  if (bytes == -1)
	    return FALSE;

          count += bytes;
        }

      wire_buffer->buffer_count = 0;
    }
  return TRUE;
}

int wire_file_read(int fd, guint8 *buf, gulong count)
{
	gulong buffer_bytes;
	guint8* buffer;
	buffer_bytes=wire_buffer->buffer_count;
	if(buffer_bytes<count)
	{	return FALSE;
	}
	buffer = wire_buffer->buffer + wire_buffer->buffer_read_index;
	memcpy(buf,buffer,count);
	wire_buffer->buffer_count -= count;
	wire_buffer->buffer_read_index += count;
	if(!wire_buffer->buffer_count)
	{	wire_buffer->buffer_read_index = 0;
		wire_buffer->buffer_write_index = 0;
	}
	return TRUE;
}

int wire_file_write (int fd, guint8 *buf, gulong count)
{
  gulong bytes;

  while (count > 0)
    {
      if ((wire_buffer->buffer_write_index + count) >= WIRE_BUFFER_SIZE)
	{
	  bytes = WIRE_BUFFER_SIZE - wire_buffer->buffer_count;
	  memcpy (&wire_buffer->buffer[wire_buffer->buffer_count], buf, bytes);
	  wire_buffer->buffer_count += bytes;
	  if (!wire_flush (fd))
	    return FALSE;
	}
      else
	{
	  bytes = count;
	  memcpy (&wire_buffer->buffer[wire_buffer->buffer_count], buf, bytes);
	  wire_buffer->buffer_count += bytes;
	}

      buf += bytes;
      count -= bytes;
    }

  return TRUE;
}


#endif

int
wire_read (int fd,guint8 *buf, gulong count) 
{
#ifdef WIN32
	if(!wire_read_func)
	{	return FALSE;
	}
	if(wire_read_func(fd, buf, count))
	{	return TRUE;
	}
	g_print ("wire_read: error\n");
	wire_error_val = TRUE;
	return FALSE;
#else
    int bytes;

    while (count > 0) {
	do
	{
#	ifdef __APPLE__
	  struct pollfd ev;
	  int erg;

	  ev.fd = fd;
	  ev.events = POLLIN|POLLPRI;

	  //while (1)
	  {
	    erg = poll( &ev, (unsigned long)1, 10 );
/*	    if(erg < 0)
	    {
	      fprintf(stderr,"Error while polling: %s\n", strerror(errno));
	      return FALSE;
	    }

	    if((ev.revents&POLLIN) == POLLIN)
	      break;
	    if((ev.revents&POLLPRI) == POLLPRI)
	      break;*/
	  }
#	endif
	  bytes = read (fd, (char*) buf, count);
	} 
	while ((bytes == -1) && ((errno == EAGAIN) || (errno == EINTR)));

	if (bytes == -1) {
	    g_print ("wire_read: error2\n");
	    wire_error_val = TRUE;
	    return FALSE;
        }

	if (bytes == 0) {
	    g_print ("wire_read: unexpected EOF (plug-in crashed?)\n");
	    wire_error_val = TRUE;
	    return FALSE;
	}

        count -= bytes;
        buf += bytes;
    }

  return TRUE;
#endif
}

int
wire_write (int     fd,
	    guint8 *buf,
	    gulong  count)
{
#ifdef WIN32
	if(!wire_write_func)
	{	return FALSE;
	}
	/* calls gimp_write */
	if (wire_write_func(fd, buf, count))
	{	return TRUE;
	}
	g_print ("wire_write: write error 1\n");
	wire_error_val = TRUE;
	return FALSE;
#else
    int bytes;

    while (count > 0) {
	do {
	    bytes = write (fd, (char*) buf, count);
	} 
	while ((bytes == -1) && (errno != EPIPE) && ((errno == EAGAIN) || (errno == EINTR)));

        if (bytes == -1) {
	    g_print ("wire_write: write error 2\n");
	    wire_error_val = TRUE;
	    return FALSE;
        }

        count -= bytes;
        buf += bytes;
    }
  return TRUE;
#endif
}

int
wire_flush (int fd)
{/* calls gimp_flush */
  if (wire_flush_func)
    return (* wire_flush_func) (fd);
  return FALSE;
}

int
wire_error ()
{
  return wire_error_val;
}

void
wire_clear_error ()
{
  wire_error_val = FALSE;
}

int
wire_read_msg (int          fd,
	       WireMessage *msg)
{
  WireHandler *handler;

  if (wire_error_val)
    return !wire_error_val;

  if (!wire_read_int32 (fd, &msg->type, 1))
    return FALSE;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d\n", msg->type);

  (* handler->read_func) (fd, msg);
#ifdef DEBUG_WIRE
	d_printf("wire_read_msg[%i]: type %i %s",
		wire_buffer->buffer_count,msg->type,gp_name[msg->type]);
	switch(msg->type)
	{case GP_PROC_RETURN:
		{	GPProcReturn *proc=(GPProcReturn*) msg->data;
			d_printf(" %s",proc->name);
			break;
		}
	case GP_TILE_DATA:
		{/*	unsigned i;
			unsigned long* ptr;
			GPTileData* tile=(GPTileData*) msg->data;
			d_printf("\nTILE_DATA: ");
			d_printf("id=%i,tile_num=%i,width=%i,height=%i,bpp=%i
 tile->drawable->id) ||
  tile->tile_num) ||
 tile->shadow) ||
tile->width) ||
tile->height) ||
tile->bpp))*/
/*			ptr=(unsigned long*) tile->data;
			for(i=0;i<3*8;i++)
			{	d_printf("%x ",ptr[i]);
			}
*/		}
	}
	d_puts("");
#endif
  return !wire_error_val;
}

int
wire_write_msg (int          fd,
		WireMessage *msg)
{
  WireHandler *handler;
#ifdef DEBUG_WIRE
	d_printf("wire_write_msg: type %i %s",msg->type,gp_name[msg->type]);
	switch(msg->type)
	{case GP_PROC_INSTALL:
		{	GPProcInstall *proc=(GPProcInstall*) msg->data;
			d_printf(" %s \n",proc->name);
			break;
		}
	case GP_PROC_RUN:
		{	GPProcRun *proc=(GPProcRun*) msg->data;
			d_printf(" %s \n",proc->name);
			break;
		}
	case GP_PROC_RETURN:
		{	GPProcReturn *proc=(GPProcReturn*) msg->data;
			d_printf(" %s \n",proc->name);
			break;
		}
	default:
		d_puts("");
	}
#endif
  if (wire_error_val)
    return !wire_error_val;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d\n", msg->type);

  if (!wire_write_int32 (fd, &msg->type, 1))
    return FALSE;

  (* handler->write_func) (fd, msg);

  return !wire_error_val;
}

void
wire_destroy (WireMessage *msg)
{
	WireHandler *handler;

	handler = g_hash_table_lookup (wire_ht, &msg->type);
	if (!handler)
	{	g_error ("could not find handler for message: %d\n", msg->type);
		return;
	}
	(* handler->destroy_func) (msg);
}

int
wire_read_int32 (int      fd,
		 guint32 *data,
		 gint     count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (fd, (guint8*) data, count * 4))
	return FALSE;

      while (count--)
	{
	  *data = ntohl (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int16 (int      fd,
		 guint16 *data,
		 gint     count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (fd, (guint8*) data, count * 2))
	return FALSE;

      while (count--)
	{
	  *data = ntohs (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int8 (int     fd,
		guint8 *data,
		gint    count)
{
  return wire_read (fd, data, count);
}

int
wire_read_double (int      fd,
		  gdouble *data,
		  gint     count)
{
  char *str;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_string (fd, &str, 1))
	return FALSE;
      sscanf (str, "%le", &data[i]);
      g_free (str);
    }

  return TRUE;
}

int
wire_read_string (int     fd,
		  gchar **data,
		  gint    count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_int32 (fd, &tmp, 1))
	return FALSE;

      if (tmp > 0)
	{
	  data[i] = g_new (gchar, tmp);
	  if (!wire_read_int8 (fd, (guint8*) data[i], tmp))
	    {
	      g_free (data[i]);
	      return FALSE;
	    }
	}
      else
	{
	  data[i] = NULL;
	}
    }

  return TRUE;
}

int
wire_write_int32 (int      fd,
		  guint32 *data,
		  gint     count)
{
  guint32 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = htonl (data[i]);
          //d_printf("%u/%u  %ld %ld %ld \n", tmp, (guint32)data, sizeof(long), sizeof(int), sizeof(short));
	  if (!wire_write_int8 (fd, (guint8*) &tmp, 4))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int16 (int      fd,
		  guint16 *data,
		  gint     count)
{
  guint16 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = htons (data[i]);
	  if (!wire_write_int8 (fd, (guint8*) &tmp, 2))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int8 (int     fd,
		 guint8 *data,
		 gint    count)
{
  return wire_write (fd, data, count);
}

int
wire_write_double (int      fd,
		   gdouble *data,
		   gint     count)
{
  gchar *t, buf[128];
  int i;

  t = buf;
  for (i = 0; i < count; i++)
    {
      sprintf (buf, "%0.50e", data[i]);
      if (!wire_write_string (fd, &t, 1))
	return FALSE;
    }

  return TRUE;
}

int
wire_write_string (int     fd,
		   gchar **data,
		   gint    count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (data[i])
	tmp = strlen (data[i]) + 1;
      else
	tmp = 0;
#ifdef DEBUG_WIRE_WRITE_STRING
	if(tmp)
	{	d_printf("wire_write_string: %s\n",data[i]);
	}
#endif
      if (!wire_write_int32 (fd, &tmp, 1))
	return FALSE;
      if (tmp > 0)
	if (!wire_write_int8 (fd, (guint8*) data[i], tmp))
	  return FALSE;
    }

  return TRUE;
}

static void
wire_init ()
{
  if (!wire_ht)
    {
      wire_ht = g_hash_table_new ((GHashFunc) wire_hash,
				  (GCompareFunc) wire_compare);
    }
}

static guint
wire_hash (guint32 *key)
{
  return *key;
}

static gint
wire_compare (guint32 *a,
	      guint32 *b)
{
  return (*a == *b);
}

