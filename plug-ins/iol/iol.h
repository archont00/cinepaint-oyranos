/* iol.h - Header for IOL, the Image Operation Language
 *
 * Copyright (C) 2003 Sean Ridenour.
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
 * Read the file COPYING for the complete licensing terms.
 */

#ifndef _IOL_H_
#define _IOL_H_

/* to allow gtktext.h */
#ifndef GTK_ENABLE_BROKEN
#define GTK_ENABLE_BROKEN
#endif

#include <string.h>

#define NUM_SYMBOLS	50
#define CODE_SIZE	32767
#define STACK_SIZE	512

typedef union {
	int i;
	float f;
} code_t;

typedef struct {
	float c[4];
} iol_color;

typedef struct {
	char *filename;
} iol_vals;

typedef struct {
	char *name;
} s_table_t;

code_t *code;
float stack[512], vars[NUM_SYMBOLS];
int pc;					/* the program counter */

s_table_t s_table[NUM_SYMBOLS];

int lineno;
char *iol_input, *iol_input_ptr;
unsigned int iol_input_lim;
int input_size;

iol_vals ivals;
iol_color in_color,out_color;

int dialog_status;
int do_out;

int xsize, ysize, xpos, ypos;

/* in main.c */
int iol_process(void);

/* in parser.y */
int s_lookup(char *name);
void warning(char *string, ...);
int compile(void);
void push_val(float val);
void push_var(int v);
void pop_var(int v);
void inp(int r, int g, int b, int a);
void outp(int r, int g, int b, int a);
void size(int x, int y);
void pos(int x, int y);

#endif /* _IOL_H_ */
