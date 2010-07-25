/*
 * This is a plugin for CinePaint/Film Gimp
 *
 * IOL (Image Operation Language) allows you to run user-created
 * programs that will modify an image. IOL is not intended as a
 * replacement for ordinary Film Gimp plug-ins. Rather, it is
 * meant to be a quick and dirty miniature programming
 * language that allows users, who aren't necessarily programmers,
 * to quickly and easily write small programs that modify images,
 * without having to delve into the complexities of writing
 * full-on plug-ins.
 *
 * Copyright (C) 2003 Sean Ridenour
 * <s_ridenour@distortedpictures.com>
 * The code that communicates with Film Gimp was borrowed from
 * Film Gimp's noisify plug-in.
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
 *
 * Read the file COPYING for the complete licensing terms. *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "fg.h"

#include "iol.h"
#include "interface.h"
#include "support.h"

#define PLUG_IN_NAME "plug_in_iol"
#define PLUG_IN_VERSION	"February 2003"

#define ENTRY_WIDTH  60
#define SCALE_WIDTH 125
#define TILE_CACHE_SIZE 16

#define DIV_U8(u,r) ( r = (float)u / 255.0f )

#define DIV_U16(u,r) ( r = (float)u / 65535.0f )

#define CLIP_U8(u,r) \
	if(u > 1.0) r = 255; \
	else if(u < 0.0) r = 0; \
	else r = (float)u * 255.0f

#define CLIP_U16(u,r) \
	if(u > 1.0) r = 65535; \
	else if(u < 0.0) r = 0; \
	else r = (float)u * 65535.0f

#define CLIP_F(u) \
	if(u > 1.0) u = 1.0; \
	if(u < 0.0) u = 0.0

/* Declare local functions.
 */
static void query(void);
static void run(char *name, int nparams, GParam * param, int *nreturn_vals,
		GParam ** return_vals);

static int iol_dialog(void);
int iol_process(void);
static int run_bytecode(void);

int exit_clean;

GPlugInInfo PLUG_IN_INFO = {
	NULL,			/* init_proc */
	NULL,			/* quit_proc */
	query,			/* query_proc */
	run,			/* run_proc */
};

MAIN()

static void query()
{
	static GParamDef args[] = {
		{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
		{PARAM_IMAGE, "image", "Input image (unused)"},
		{PARAM_DRAWABLE, "drawable", "Input drawable"},
		{PARAM_STRING, "file",
		 "The file containing the IOL script to run"}
	};
	static GParamDef *return_vals = NULL;
	static int nargs = sizeof(args) / sizeof(args[0]);
	static int nreturn_vals = 0;

	gimp_install_procedure(PLUG_IN_NAME,
			       "Run an IOL script",
			       "This plug-in runs an IOL script, which modifies the image",
			       "Sean Ridenour <s_ridenour@distortedpictures.com>",
			       "Copyright (C) 2003 Sean Ridenour",
			       PLUG_IN_VERSION,
			       "<Image>/Filters/IOL/IOL",
			       "RGB*, U16_RGB*, FLOAT_RGB*, FLOAT16_RGB*",
			       PROC_PLUG_IN,
			       nargs, nreturn_vals, args, return_vals);
}

static void
run(char *name,
    int nparams, GParam * param, int *nreturn_vals, GParam ** return_vals)
{
	static GParam values[1];
	GStatusType status = STATUS_SUCCESS;
	FILE *infile;
	char in[256];
	int unlnk;
	struct stat info;
	/* extern double sqrt(), fabsf(), sin(), cos(), tan(); */

        INIT_I18N_UI();

	run_mode = param[0].data.d_int32;

	*nreturn_vals = 1;
	*return_vals = values;

	values[0].type = PARAM_STATUS;
	values[0].data.d_status = status;

	drawable_ID = param[2].data.d_drawable;

	switch (run_mode) {
	case RUN_INTERACTIVE:
		/*  Possibly retrieve data  */
		gimp_get_data("plug_in_iol", &ivals);

		ivals.filename = strdup("iol_temp");
		unlnk = 1;
		/*printf("%s\n",ivals.filename);*/

		/*  First acquire information with a dialog  */
		break;

	case RUN_NONINTERACTIVE:
		/*  Make sure all the arguments are there!  */
		if (nparams != 4)
			status = STATUS_CALLING_ERROR;
		if (status == STATUS_SUCCESS) {
			ivals.filename = param[3].data.d_string,256;
		}
		unlnk = 0;
		break;

	case RUN_WITH_LAST_VALS:
		/*  Possibly retrieve data  */
		gimp_get_data("plug_in_iol", &ivals);
		unlnk = 0;
		break;

	default:
		unlnk = 0;
		break;
	}

	/* initialize */
	drawable = gimp_drawable_get(drawable_ID);
	gimp_tile_cache_ntiles(TILE_CACHE_SIZE);

		if(run_mode == RUN_INTERACTIVE) {
			if (!iol_dialog())
				status = STATUS_EXECUTION_ERROR;
			else
				status = STATUS_SUCCESS;

			gimp_set_data("plug_in_iol", &ivals, sizeof(iol_vals));
		} else {
			if(stat(ivals.filename,&info) < 0) {
				fprintf(stderr,"IOL error: couldn't stat %s\n",ivals.filename);
				values[0].data.d_status = STATUS_EXECUTION_ERROR;
				gimp_drawable_detach(drawable);
				free(iol_input);
				return;
			}

			infile = fopen(ivals.filename,"r");
			if(infile == NULL) {
				fprintf(stderr,"IOL error: couldn't open %s\n",ivals.filename);
				values[0].data.d_status = STATUS_EXECUTION_ERROR;
				gimp_drawable_detach(drawable);
				free(iol_input);
				return;
			}

			/* read the file */
			iol_input = (char *)malloc(info.st_size + 1);
			/* to be Windows-safe, we can't just do it the quick and easy
			   fread() way, we have to do this kludge */
			fgets(in,256,infile);
			strcpy(iol_input,in);
			while(fgets(in,256,infile))
				strncat(iol_input,in,info.st_size);

			fclose(infile);

			input_size = info.st_size;

			/* set up some things for the lexer */
			iol_input_ptr = iol_input;
			iol_input_lim = (unsigned int)iol_input + input_size;
			iol_input[input_size] = 0;

			if (!iol_process()) {
				status = STATUS_EXECUTION_ERROR;
			} else {
				status = STATUS_SUCCESS;
				if(unlnk)
					unlink(ivals.filename);
			}
		}

	if(!exit_clean)
		if(run_mode != RUN_NONINTERACTIVE)
			gimp_displays_flush();

	values[0].data.d_status = status;
	gimp_drawable_detach(drawable);
}

int iol_process(void)
{
	GPixelRgn src_rgn, dest_rgn;
	gint row, col;
	gint x1, y1, x2, y2;
	gpointer pr;
	int x, y;

	precision = gimp_drawable_precision (drawable->id);
	num_channels = drawable->num_channels;

	gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);
	gimp_pixel_rgn_init(&src_rgn, drawable, x1, y1, (x2 - x1),
			    (y2 - y1), FALSE, FALSE);
	gimp_pixel_rgn_init(&dest_rgn, drawable, x1, y1, (x2 - x1),
			    (y2 - y1), TRUE, TRUE);

	xsize = x2 - x1;
	ysize = y2 - y1;

	exit_clean = 0;

	/* compile the script */
	code = (code_t *)calloc(CODE_SIZE, sizeof(code_t));
	if(code == NULL)
		return -1;

	memset(code,0,CODE_SIZE * sizeof(code_t));
	memset(stack,0,STACK_SIZE * sizeof(float));
	memset(vars,0,NUM_SYMBOLS * sizeof(float));

	if(compile()) {
		fprintf(stderr,"IOL error: compile failed\n");
		free(code);
		free(iol_input);
		return -1;
	}

	free(iol_input);

#ifdef DEBUG
	{
		FILE *dbg;

		dbg = fopen("iol_bytecode_debug","w");
		if(dbg != NULL) {
			fwrite(code,sizeof(code_t),CODE_SIZE,dbg);
			fclose(dbg);
		}
	}
#endif

	xpos = 0; ypos = 0;
	x = 0; y = 0;

	for (pr = gimp_pixel_rgns_register(2, &src_rgn, &dest_rgn);
	     pr != NULL; pr = gimp_pixel_rgns_process(pr)) {
		src_row = src_rgn.data;
		dest_row = dest_rgn.data;

		switch(precision) {
		case PRECISION_U8:
			for (row = 0; row < src_rgn.h; row++) {
				srcu8 = (guint8 *)src_row;
				destu8 = (guint8 *)dest_row;

				for (col = 0; col < src_rgn.w; col++) {
					xpos = x + col;
					ypos = y + row;

					if(run_bytecode()) {
						free(code);
						gimp_drawable_flush(drawable);
						gimp_drawable_merge_shadow(drawable->id, TRUE);
						gimp_drawable_update(drawable->id, x1, y1,
									(x2 - x1), (y2 - y1));
						return -1;
					}

					srcu8 += num_channels;
					destu8 += num_channels;
				}

				src_row += src_rgn.rowstride;
				dest_row += dest_rgn.rowstride;
			}
			break;
		case PRECISION_U16:
			for (row = 0; row < src_rgn.h; row++) {
				srcu16 = (guint16 *)src_row;
				destu16 = (guint16 *)dest_row;

				for (col = 0; col < src_rgn.w; col++) {
					xpos = x + col;
					ypos = y + row;

					if(run_bytecode()) {
						free(code);
						gimp_drawable_flush(drawable);
						gimp_drawable_merge_shadow(drawable->id, TRUE);
						gimp_drawable_update(drawable->id, x1, y1,
									(x2 - x1), (y2 - y1));
						return -1;
					}

					srcu16 += num_channels;
					destu16 += num_channels;
				}

				src_row += src_rgn.rowstride;
				dest_row += dest_rgn.rowstride;
			}
			break;
		case PRECISION_FLOAT16:
			for (row = 0; row < src_rgn.h; row++) {
				srcf16 = (guint16 *)src_row;
				destf16 = (guint16 *)dest_row;

				for (col = 0; col < src_rgn.w; col++) {
					xpos = x + col;
					ypos = y + row;

					if(run_bytecode()) {
						free(code);
						gimp_drawable_flush(drawable);
						gimp_drawable_merge_shadow(drawable->id, TRUE);
						gimp_drawable_update(drawable->id, x1, y1,
									(x2 - x1), (y2 - y1));
						return -1;
					}

					srcf16 += num_channels;
					destf16 += num_channels;
				}

				src_row += src_rgn.rowstride;
				dest_row += dest_rgn.rowstride;
			}
			break;
		case PRECISION_FLOAT:
			for(row = 0; row < src_rgn.h; row++) {
				srcf32 = (gfloat *)src_row;
				destf32 = (gfloat *)dest_row;

				for(col = 0; col < src_rgn.w; col++) {
					xpos = x + col;
					ypos = y + row;

					if(run_bytecode()) {
						free(code);
						gimp_drawable_flush(drawable);
						gimp_drawable_merge_shadow(drawable->id, TRUE);
						gimp_drawable_update(drawable->id, x1, y1,
									(x2 - x1), (y2 - y1));
						return -1;
					}

					srcf32 += num_channels;
					destf32 += num_channels;
				}

				src_row += src_rgn.rowstride;
				dest_row += dest_rgn.rowstride;
			}
			break;
		default:
			fprintf(stderr,"IOL error: invalid precision type\n");
			return -1;
			break;
		}

		x += src_rgn.w;
		if(x >= xsize) {
			x = 0;
			y += src_rgn.h;
		}
	}

	for(x = 0; x < NUM_SYMBOLS; x++) {
		if(s_table[x].name != NULL) {
			free(s_table[x].name);
			s_table[x].name = NULL;
		}
	}

	free(code);
	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->id, TRUE);
	gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));
	if(run_mode != RUN_NONINTERACTIVE)
		gimp_displays_flush();

	exit_clean = 1;

	return 0;
}

static int iol_dialog(void)
{
	GtkWidget *window;
	gchar **argv;
	gint argc;

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("iol");

	gtk_init(&argc,&argv);

	window = create_iol_window();
	gtk_widget_show(window);

	dialog_status = 0;

	gtk_main();
	gdk_flush();

	return dialog_status;
}

int stackp;

inline static float pop(void)
{
	float f;

	f = stack[stackp++];
	if(stackp >= STACK_SIZE) stackp = STACK_SIZE - 1;
	return f;
}

inline static void push(float f)
{
	stackp--;
	if(stackp < 0) stackp = 0;
	stack[stackp] = f;
}

static void do_input(void)
{
	int b;
	ShortsFloat u;

	switch(precision) {
		case PRECISION_U8:
			for (b = 0; b < num_channels; b++)
				DIV_U8(srcu8[b],in_color.c[b]);
			break;
		case PRECISION_U16:
			for (b = 0; b < num_channels; b++)
				DIV_U16(srcu16[b],in_color.c[b]);
			break;
		case PRECISION_FLOAT16:
			for (b = 0; b < num_channels; b++)
				in_color.c[b] = FLT(srcf16[b],u);
			break;
		case PRECISION_FLOAT:
			for(b = 0; b < num_channels; b++)
				in_color.c[b] = (float)srcf32[b];
			break;
		default:
			break;
	}

	vars[code[++pc].i] = in_color.c[0];
	vars[code[++pc].i] = in_color.c[1];
	vars[code[++pc].i] = in_color.c[2];
	vars[code[++pc].i] = in_color.c[3];
}

static void do_output(void)
{
	int b;
	ShortsFloat u;

	out_color.c[0] = vars[code[++pc].i];
	out_color.c[1] = vars[code[++pc].i];
	out_color.c[2] = vars[code[++pc].i];
	out_color.c[3] = vars[code[++pc].i];

	switch(precision) {
		case PRECISION_U8:
			for(b = 0; b < num_channels; b++)
				CLIP_U8(out_color.c[b],destu8[b]);
			break;
		case PRECISION_U16:
			for(b = 0; b < num_channels; b++)
				CLIP_U16(out_color.c[b],destu16[b]);
			break;
		case PRECISION_FLOAT16:
			for(b = 0; b < num_channels; b++) {
				CLIP_F(out_color.c[b]);
				destf16[b] = FLT16(out_color.c[b],u);
			}
			break;
		case PRECISION_FLOAT:
			for(b = 0; b < num_channels; b++)
				destf32[b] = (gfloat)out_color.c[b];
			break;
		default:
			break;
	}
}

static int run_bytecode(void)
{
	float f1, f2;
	pc = 0;
	stackp = STACK_SIZE - 1;
	while(pc < CODE_SIZE) {
		switch(code[pc].i) {
			case 0x00:	/* halt */
				return 0;
			case 0x01:	/* add */
				push(pop() + pop());
				break;
			case 0x02:	/* subtract */
				f2 = pop();
				f1 = pop();
				push(f1 - f2);
				break;
			case 0x03:	/* multiply */
				push(pop() * pop());
				break;
			case 0x04:	/* divide */
				f2 = pop();
				f1 = pop();
				if(f2 == 0.0f)
					push(0.0f);
				else
					push(f1 / f2);
				break;
			case 0x05:	/* input */
				do_input();
				break;
			case 0x06:	/* output */
				do_output();
				break;
			case 0x07:	/* size */
				vars[code[++pc].i] = (float)xsize;
				vars[code[++pc].i] = (float)ysize;
				break;
			case 0x08:	/* push variable */
				push(vars[code[++pc].i]);
				break;
			case 0x09:	/* pop variable */
				vars[code[++pc].i] = pop();
				break;
			case 0x0a:	/* square root */
				push((float)sqrt((double)pop()));
				break;
			case 0x0b:	/* absolute value */
				push(fabsf(pop()));
				break;
			case 0x0c:	/* sine */
				push((float)sin((double)pop()));
				break;
			case 0x0d:	/* cosine */
				push((float)cos((double)pop()));
				break;
			case 0x0e:	/* tangent */
				push((float)tan((double)pop()));
				break;
			case 0x0f:	/* natural logarithm */
				push((float)log((double)pop()));
				break;
			case 0x10:	/* base-10 logarithm */
				push((float)log10((double)pop()));
				break;
			case 0x11:	/* power */
				push((float)pow((double)pop(),(double)pop()));
				break;
			case 0x12:	/* clip */
				f1 = pop();
				if(f1 < 0.0f) f1 = 0.0f;
				if(f1 > 1.0f) f1 = 1.0f;
				push(f1);
				break;
			case 0x13:	/* minimum */
				f1 = pop();
				f2 = pop();
				if(f1 < f2)
					push(f1);
				else if(f1 > f2)
					push(f2);
				else
					push(f1);
				break;
			case 0x14:	/* maximum */
				f1 = pop();
				f2 = pop();
				if(f1 > f2)
					push(f1);
				else if(f1 < f2)
					push(f2);
				else
					push(f1);
				break;
			case 0x15:	/* push value */
				push(code[++pc].f);
				break;
			case 0x16:	/* negate */
				push(-(pop()));
				break;
			case 0x17:	/* debug */
				f1 = pop();
				fprintf(stderr,"%f\n",f1);
				fflush(stderr);
				push(f1);
				break;
			case 0x18:	/* position */
				vars[code[++pc].i] = (float)xpos;
				vars[code[++pc].i] = (float)ypos;
				break;
			case 0x19:	/* jump if less */
				pc++;
				if(pop() < 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1A:	/* jump if greater */
				pc++;
				if(pop() > 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1B:	/* jump if equal */
				pc++;
				if(pop() == 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1C:	/* jump if not equal */
				pc++;
				if(pop() != 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1D:	/* jump if less or equal */
				pc++;
				if(pop() <= 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1E:	/* jump if greater or equal */
				pc++;
				if(pop() >= 0.0)
					pc = code[pc].i - 1;
				break;
			case 0x1F:	/* jump unconditionally */
				pc++;
				pc = code[pc].i - 1;
				break;
			default:	/* doh! */
				fprintf(stderr,"IOL bytecode error: %04XH is an invalid opcode\n",code[pc].i);
				fprintf(stderr,"PC = %d\nSP = %d\n",pc,stackp);
				return -1;
		}
		pc++;
	}

	/* we should never get here */
	fprintf(stderr,"IOL bytecode error: reached the end of code, didn't find a halt instruction\n");
	return -1;
}
