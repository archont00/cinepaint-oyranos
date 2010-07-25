/* iol.l - IOL lexer
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
 * Read the file COPYING for the complete licensing terms. */
 
/* To generate the C source code for the lexical analyzer from this file,
   after generating a new parser from parser.y (read that file for instructions
   on how to do it), do:
   	flex -f lexer.l
 */
%{
#include "iol.h"
#include "parser.tab.h"

int get_input(char *buf, int max_size);

/* we get our input from a string, for speed*/
/* FIXME: this only works with GNU Flex */
#undef YY_INPUT
#define YY_INPUT(b,r,ms) (r = get_input(b,ms))

%}

comment	#.*
number	([0-9]+|([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?)
name	[a-zA-Z][a-zA-Z0-9]*
ws	[ \t]+

%%

{ws}		;	/* whitespace */
{comment}	;
{number}	{ yylval.val = atof(yytext); return NUMBER; }
sqrt		{ return SQRT; }
abs		{ return ABS; }
sin		{ return SIN; }
cos		{ return COS; }
tan		{ return TAN; }
log10		{ return LOG10; }
log		{ return LOG; }
pow		{ return POW; }
clip		{ return CLIP; }
input		{ return INP; }
output		{ return OUTP; }
min		{ return MIN; }
max		{ return MAX; }
size		{ return SIZE; }
pos		{ return POS; }
"if"		{ return IF; }
"else"		{ return ELSE; }
"=="		{ return EQUALS; }
"!="		{ return NOTEQUALS; }
">="		{ return GR_EQUALS; }
"<="		{ return LT_EQUALS; }
{name}		{
			int sp = s_lookup(yytext);
			yylval.sptr = sp;
			return NAME;
		}
\n		{ lineno++;}
.		{ return yytext[0]; }

%%

#ifdef min
#undef min
#endif
static int min(int a, int b)
{
	if(a < b)
		return a;

	if(b < a)
		return b;

	return 0;
}

int get_input(char *buf, int max_size)
{
	int n;

	n = min(max_size,iol_input_lim - (unsigned int)iol_input_ptr);
	if(n > 0) {
		memcpy(buf, iol_input_ptr, n);
		iol_input_ptr += n;
	}

	return n;
}
