/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NAME = 258,
     NUMBER = 259,
     INP = 260,
     OUTP = 261,
     SQRT = 262,
     ABS = 263,
     SIN = 264,
     COS = 265,
     TAN = 266,
     LOG = 267,
     LOG10 = 268,
     POW = 269,
     CLIP = 270,
     MIN = 271,
     MAX = 272,
     SIZE = 273,
     POS = 274,
     IF = 275,
     ELSE = 276,
     EQUALS = 277,
     NOTEQUALS = 278,
     GR_EQUALS = 279,
     LT_EQUALS = 280,
     UMINUS = 281
   };
#endif
#define NAME 258
#define NUMBER 259
#define INP 260
#define OUTP 261
#define SQRT 262
#define ABS 263
#define SIN 264
#define COS 265
#define TAN 266
#define LOG 267
#define LOG10 268
#define POW 269
#define CLIP 270
#define MIN 271
#define MAX 272
#define SIZE 273
#define POS 274
#define IF 275
#define ELSE 276
#define EQUALS 277
#define NOTEQUALS 278
#define GR_EQUALS 279
#define LT_EQUALS 280
#define UMINUS 281




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 44 "parser.y"
typedef union YYSTYPE {
	float val;
	int sptr;
} YYSTYPE;
/* Line 1249 of yacc.c.  */
#line 93 "parser.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



