/* iol.y - IOL parser
 *
 * Copyright (C) 2003 Sean Ridenour
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
 
/* To generate the C source code from this, do:
	byacc -d -b parser parser.y
 */
%{
#include "iol.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define C_STACK_SIZE 2048

int c_stack[C_STACK_SIZE];
int c_sp, i;

static void push_c(int addr);
static int pop_c(void);
static void reset(void);
static void yyerror(char *string);

%}

%union {
	float val;
	int sptr;
}

%token <sptr> NAME
%token <val> NUMBER
%token INP OUTP SQRT ABS SIN COS TAN LOG LOG10 POW CLIP MIN MAX SIZE POS
%token IF ELSE EQUALS NOTEQUALS GR_EQUALS LT_EQUALS
%left '-' '+'
%left '*' '/'
%nonassoc UMINUS

%%
statements:	statement
	|	statements statement
	;

statement:	INP NAME NAME NAME NAME ';'	{ inp($2,$3,$4,$5); }
	|	OUTP NAME NAME NAME NAME ';'	{ outp($2,$3,$4,$5); }
	|	SIZE NAME NAME ';'		{ size($2,$3); }
	|	POS NAME NAME ';'		{ pos($2,$3); }
	|	ifne
	|	if else
	|	NAME '=' expression ';' 	{ pop_var($1); }
	|	expression ';'			{ code[pc++].i = 0x17; }
	|	';'				{ /* so we can have empty ELSE statements */ }
	|	error ';'			{ yyerrok; yyclearin; }
	|	error '}'			{ yyerrok; yyclearin; }
	;

expression:	k_expression
	|	expression '+' expression	{ code[pc++].i = 0x01; }
	|	expression '-' expression	{ code[pc++].i = 0x02; }
	|	expression '*' expression	{ code[pc++].i = 0x03; }
	|	expression '/' expression	{ code[pc++].i = 0x04; }
	|	'-' expression %prec UMINUS	{ code[pc++].i = 0x16; }
	|	'(' expression ')'		{ ; }
	|	'(' error ')'			{ yyerrok; yyclearin; }
	|	NUMBER				{ push_val($1); }
	|	NAME				{ push_var($1); }
	;

k_expression:	SQRT '(' expression ')'		{ code[pc++].i = 0x0A; }
	|	ABS '(' expression ')'		{ code[pc++].i = 0x0B; }
	|	SIN '(' expression ')'		{ code[pc++].i = 0x0C; }
	|	COS '(' expression ')'		{ code[pc++].i = 0x0D; }
	|	TAN '(' expression ')'		{ code[pc++].i = 0x0E; }
	|	LOG '(' expression ')'		{ code[pc++].i = 0x0F; }
	|	LOG10 '(' expression ')'	{ code[pc++].i = 0x10; }
	|	POW '(' expression ',' expression ')'	{ code[pc++].i = 0x11; }
	|	CLIP '(' expression ')'			{ code[pc++].i = 0x12; }
	|	MIN '(' expression ',' expression ')'	{ code[pc++].i = 0x13; }
	|	MAX '(' expression ',' expression ')'	{ code[pc++].i = 0x14; }
	;

/* When comparing, we do the oposite conditional branch, because you only want
   to jump past the following code if the condition is not true. For example,
   the EQUALS comparison subtracts and then uses the Jump If Not Equal bytecode */
comparison:	expression EQUALS expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x1C;
							push_c(pc);
							code[pc++].i = 0x00; }
	|	expression NOTEQUALS expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x1B;
							push_c(pc);
							code[pc++].i = 0x00; }
	|	expression '>' expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x1D;
							push_c(pc);
							code[pc++].i = 0x00; }
	|	expression '<' expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x1E;
							push_c(pc);
							code[pc++].i = 0x00; }
	|	expression GR_EQUALS expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x19;
							push_c(pc);
							code[pc++].i = 0x00; }
	|	expression LT_EQUALS expression	{	code[pc++].i = 0x02;
							code[pc++].i = 0x1A;
							push_c(pc);
							code[pc++].i = 0x00; }
	;

ifne:	IF comparison '{' statements '}'	{ code[pop_c()].i = pc; }
	;

if:	IF comparison '{' statements '}'	{	code[pc++].i = 0x01F;
							i = pop_c();
							push_c(pc);
							code[pc++].i = 0x00;
							code[i].i = pc; }
	;

else:	ELSE '{' statements '}'			{ code[pop_c()].i = pc; }
	;
%%

/* look up an entry in the symbol table. If not found, add it */
int s_lookup(char *name)
{
	int x;

	for(x = 0; x < NUM_SYMBOLS; x++) {
		if(s_table[x].name && !strcmp(s_table[x].name,name))
			return x;

		if(!s_table[x].name) {
			s_table[x].name = strdup(name);
			return x;
		}
	}
	yyerror("Too many symbols");
	return 0;
}

void warning(char *string, ...)
{
	va_list ap;
	va_start(ap,string);
	fprintf(stderr,"IOL warning (line %d): ",lineno);
	vfprintf(stderr,string,ap);
	va_end(ap);
}

static void reset(void)
{
	iol_input_ptr = iol_input;
}

static void push_c(int addr)
{
	c_stack[++c_sp] = addr;
}

static int pop_c(void)
{
	int x;
	x = c_stack[c_sp];
	c_sp--;
	if(c_sp < 0) c_sp = 0;
	return x;
}

void push_val(float val)
{
	code[pc++].i = 0x15;
	code[pc++].f = val;
}

void push_var(int v)
{
	code[pc++].i = 0x08;
	code[pc++].i = v;
}

void pop_var(int v)
{
	code[pc++].i = 0x09;
	code[pc++].i = v;
}

void inp(int r, int g, int b, int a)
{
	code[pc++].i = 0x05;
	code[pc++].i = r;
	code[pc++].i = g;
	code[pc++].i = b;
	code[pc++].i = a;

}

void outp(int r, int g, int b, int a)
{
	code[pc++].i = 0x06;
	code[pc++].i = r;
	code[pc++].i = g;
	code[pc++].i = b;
	code[pc++].i = a;
}

void size(int x, int y)
{
	code[pc++].i = 0x07;
	code[pc++].i = x;
	code[pc++].i = y;
}

void pos(int x, int y)
{
	code[pc++].i = 0x18;
	code[pc++].i = x;
	code[pc++].i = y;
}

int compile(void)
{
	c_sp = 0;
	memset(c_stack,0,C_STACK_SIZE);
	pc = 0; lineno = 1;
	return yyparse();
}

void yyerror(char *string)
{
	fprintf(stderr,"IOL error (line %d): %s\n",lineno,string);
}

