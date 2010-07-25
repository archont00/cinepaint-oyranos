#ifndef __XCF_H__
#define __XCF_H__


#include <stdio.h>
#include "../lib/wire/c_typedefs.h"

typedef struct XcfInfo  XcfInfo;

struct XcfInfo
{
  FILE *fp;
  guint cp;
  char *filename;
  Layer * active_layer;
  Channel * active_channel;
  CanvasDrawable * floating_sel_drawable;
  Layer * floating_sel;
  guint floating_sel_offset;
  int swap_num;
  int *ref_count;
  int compression;
  int file_version;
};


void xcf_init (void);


#endif /* __XCF_H__ */
