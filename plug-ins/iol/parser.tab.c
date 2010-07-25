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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 25 "parser.y"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 44 "parser.y"
typedef union YYSTYPE {
	float val;
	int sptr;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 151 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 163 "parser.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  53
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   309

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  9
/* YYNRULES -- Number of rules. */
#define YYNRULES  44
/* YYNRULES -- Number of states. */
#define YYNSTATES  128

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   281

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      34,    35,    28,    27,    36,    26,     2,    29,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    31,
      38,    32,    37,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    39,     2,    33,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    30
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     8,    15,    22,    27,    32,    34,
      37,    42,    45,    47,    50,    53,    55,    59,    63,    67,
      71,    74,    78,    82,    84,    86,    91,    96,   101,   106,
     111,   116,   121,   128,   133,   140,   147,   151,   155,   159,
     163,   167,   171,   177,   183
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      41,     0,    -1,    42,    -1,    41,    42,    -1,     5,     3,
       3,     3,     3,    31,    -1,     6,     3,     3,     3,     3,
      31,    -1,    18,     3,     3,    31,    -1,    19,     3,     3,
      31,    -1,    46,    -1,    47,    48,    -1,     3,    32,    43,
      31,    -1,    43,    31,    -1,    31,    -1,     1,    31,    -1,
       1,    33,    -1,    44,    -1,    43,    27,    43,    -1,    43,
      26,    43,    -1,    43,    28,    43,    -1,    43,    29,    43,
      -1,    26,    43,    -1,    34,    43,    35,    -1,    34,     1,
      35,    -1,     4,    -1,     3,    -1,     7,    34,    43,    35,
      -1,     8,    34,    43,    35,    -1,     9,    34,    43,    35,
      -1,    10,    34,    43,    35,    -1,    11,    34,    43,    35,
      -1,    12,    34,    43,    35,    -1,    13,    34,    43,    35,
      -1,    14,    34,    43,    36,    43,    35,    -1,    15,    34,
      43,    35,    -1,    16,    34,    43,    36,    43,    35,    -1,
      17,    34,    43,    36,    43,    35,    -1,    43,    22,    43,
      -1,    43,    23,    43,    -1,    43,    37,    43,    -1,    43,
      38,    43,    -1,    43,    24,    43,    -1,    43,    25,    43,
      -1,    20,    45,    39,    41,    33,    -1,    20,    45,    39,
      41,    33,    -1,    21,    39,    41,    33,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    58,    58,    59,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,   103,   107,   111,   115,
     119,   123,   129,   132,   139
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NAME", "NUMBER", "INP", "OUTP", "SQRT", 
  "ABS", "SIN", "COS", "TAN", "LOG", "LOG10", "POW", "CLIP", "MIN", "MAX", 
  "SIZE", "POS", "IF", "ELSE", "EQUALS", "NOTEQUALS", "GR_EQUALS", 
  "LT_EQUALS", "'-'", "'+'", "'*'", "'/'", "UMINUS", "';'", "'='", "'}'", 
  "'('", "')'", "','", "'>'", "'<'", "'{'", "$accept", "statements", 
  "statement", "expression", "k_expression", "comparison", "ifne", "if", 
  "else", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    45,    43,    42,    47,
     281,    59,    61,   125,    40,    41,    44,    62,    60,   123
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    40,    41,    41,    42,    42,    42,    42,    42,    42,
      42,    42,    42,    42,    42,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    44,    44,    44,    44,    44,
      44,    44,    44,    44,    44,    44,    45,    45,    45,    45,
      45,    45,    46,    47,    48
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     2,     6,     6,     4,     4,     1,     2,
       4,     2,     1,     2,     2,     1,     3,     3,     3,     3,
       2,     3,     3,     1,     1,     4,     4,     4,     4,     4,
       4,     4,     6,     4,     6,     6,     3,     3,     3,     3,
       3,     3,     5,     5,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,    24,    23,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,     0,     0,     2,     0,    15,     8,     0,    13,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    24,     0,     0,
      20,     0,     0,     1,     3,     0,     0,     0,     0,    11,
       0,     9,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,    21,    17,    16,    18,
      19,     0,    10,     0,     0,    25,    26,    27,    28,    29,
      30,    31,     0,    33,     0,     0,     6,     7,    36,    37,
      40,    41,    38,    39,     0,     0,     0,     0,     0,     0,
       0,    42,    44,     4,     5,    32,    34,    35
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    23,    24,    25,    26,    49,    27,    28,    61
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -82
static const short yypact[] =
{
     193,   -22,   -20,   -82,    11,    28,    24,    25,    37,    38,
      39,    42,    44,    49,    51,    55,    56,    72,    92,   222,
     222,   -82,    40,    93,   -82,   268,   -82,   -82,    97,   -82,
     -82,   222,   117,   118,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   119,   122,   -82,   235,   111,
     -82,   126,     1,   -82,   -82,   222,   222,   222,   222,   -82,
     113,   -82,   274,   150,   151,    53,    88,   120,   154,   188,
     214,   218,   -21,   239,     6,    41,   153,   155,   222,   222,
     222,   222,   222,   222,   193,   -82,   -82,    17,    17,   -82,
     -82,   193,   -82,   184,   185,   -82,   -82,   -82,   -82,   -82,
     -82,   -82,   222,   -82,   222,   222,   -82,   -82,   280,   280,
     280,   280,   280,   280,   125,   159,   160,   164,   249,   253,
     263,   136,   -82,   -82,   -82,   -82,   -82,   -82
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -82,   -81,   -23,   -18,   -82,   -82,   -82,   -82,   -82
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -44
static const yysigned_char yytable[] =
{
      54,    48,    50,   114,    52,    55,    56,    57,    58,    29,
     115,    30,    31,    62,    32,   102,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    55,    56,    57,
      58,    33,    55,    56,    57,    58,    86,    87,    88,    89,
      90,    51,   104,    47,     3,    57,    58,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    34,    35,
     108,   109,   110,   111,   112,   113,    20,    55,    56,    57,
      58,    36,    37,    38,    22,    45,    39,   105,    40,    55,
      56,    57,    58,    41,   118,    42,   119,   120,    95,    43,
      44,    54,    54,    53,     1,    46,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    55,    56,    57,    58,    60,    20,
      63,    64,    76,    96,    21,    77,     1,    22,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    55,    56,    57,    58,
      84,    20,    91,    93,    94,    97,    21,   -43,   121,    22,
       1,    85,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      55,    56,    57,    58,   106,    20,   107,   116,   117,    98,
      21,   123,   122,    22,     1,   124,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    55,    56,    57,    58,     0,    20,
       0,     0,     0,    99,    21,    47,     3,    22,     0,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      55,    56,    57,    58,    55,    56,    57,    58,    20,   100,
       0,     0,     0,   101,     0,     0,    22,    78,    79,    80,
      81,    55,    56,    57,    58,    55,    56,    57,    58,     0,
       0,     0,    82,    83,   103,    55,    56,    57,    58,    55,
      56,    57,    58,     0,   125,     0,     0,     0,   126,    55,
      56,    57,    58,     0,    55,    56,    57,    58,   127,    59,
      55,    56,    57,    58,     0,    92,    55,    56,    57,    58
};

static const yysigned_char yycheck[] =
{
      23,    19,    20,    84,    22,    26,    27,    28,    29,    31,
      91,    33,    32,    31,     3,    36,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    26,    27,    28,
      29,     3,    26,    27,    28,    29,    35,    55,    56,    57,
      58,     1,    36,     3,     4,    28,    29,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    34,    34,
      78,    79,    80,    81,    82,    83,    26,    26,    27,    28,
      29,    34,    34,    34,    34,     3,    34,    36,    34,    26,
      27,    28,    29,    34,   102,    34,   104,   105,    35,    34,
      34,   114,   115,     0,     1,     3,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    26,    27,    28,    29,    21,    26,
       3,     3,     3,    35,    31,     3,     1,    34,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    26,    27,    28,    29,
      39,    26,    39,     3,     3,    35,    31,    21,    33,    34,
       1,    35,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      26,    27,    28,    29,    31,    26,    31,     3,     3,    35,
      31,    31,    33,    34,     1,    31,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    26,    27,    28,    29,    -1,    26,
      -1,    -1,    -1,    35,    31,     3,     4,    34,    -1,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      26,    27,    28,    29,    26,    27,    28,    29,    26,    35,
      -1,    -1,    -1,    35,    -1,    -1,    34,    22,    23,    24,
      25,    26,    27,    28,    29,    26,    27,    28,    29,    -1,
      -1,    -1,    37,    38,    35,    26,    27,    28,    29,    26,
      27,    28,    29,    -1,    35,    -1,    -1,    -1,    35,    26,
      27,    28,    29,    -1,    26,    27,    28,    29,    35,    31,
      26,    27,    28,    29,    -1,    31,    26,    27,    28,    29
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     1,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      26,    31,    34,    41,    42,    43,    44,    46,    47,    31,
      33,    32,     3,     3,    34,    34,    34,    34,    34,    34,
      34,    34,    34,    34,    34,     3,     3,     3,    43,    45,
      43,     1,    43,     0,    42,    26,    27,    28,    29,    31,
      21,    48,    43,     3,     3,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,     3,     3,    22,    23,
      24,    25,    37,    38,    39,    35,    35,    43,    43,    43,
      43,    39,    31,     3,     3,    35,    35,    35,    35,    35,
      35,    35,    36,    35,    36,    36,    31,    31,    43,    43,
      43,    43,    43,    43,    41,    41,     3,     3,    43,    43,
      43,    33,    33,    31,    31,    35,    35,    35
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 62 "parser.y"
    { inp(yyvsp[-4].sptr,yyvsp[-3].sptr,yyvsp[-2].sptr,yyvsp[-1].sptr); }
    break;

  case 5:
#line 63 "parser.y"
    { outp(yyvsp[-4].sptr,yyvsp[-3].sptr,yyvsp[-2].sptr,yyvsp[-1].sptr); }
    break;

  case 6:
#line 64 "parser.y"
    { size(yyvsp[-2].sptr,yyvsp[-1].sptr); }
    break;

  case 7:
#line 65 "parser.y"
    { pos(yyvsp[-2].sptr,yyvsp[-1].sptr); }
    break;

  case 10:
#line 68 "parser.y"
    { pop_var(yyvsp[-3].sptr); }
    break;

  case 11:
#line 69 "parser.y"
    { code[pc++].i = 0x17; }
    break;

  case 12:
#line 70 "parser.y"
    { /* so we can have empty ELSE statements */ }
    break;

  case 13:
#line 71 "parser.y"
    { yyerrok; yyclearin; }
    break;

  case 14:
#line 72 "parser.y"
    { yyerrok; yyclearin; }
    break;

  case 16:
#line 76 "parser.y"
    { code[pc++].i = 0x01; }
    break;

  case 17:
#line 77 "parser.y"
    { code[pc++].i = 0x02; }
    break;

  case 18:
#line 78 "parser.y"
    { code[pc++].i = 0x03; }
    break;

  case 19:
#line 79 "parser.y"
    { code[pc++].i = 0x04; }
    break;

  case 20:
#line 80 "parser.y"
    { code[pc++].i = 0x16; }
    break;

  case 21:
#line 81 "parser.y"
    { ; }
    break;

  case 22:
#line 82 "parser.y"
    { yyerrok; yyclearin; }
    break;

  case 23:
#line 83 "parser.y"
    { push_val(yyvsp[0].val); }
    break;

  case 24:
#line 84 "parser.y"
    { push_var(yyvsp[0].sptr); }
    break;

  case 25:
#line 87 "parser.y"
    { code[pc++].i = 0x0A; }
    break;

  case 26:
#line 88 "parser.y"
    { code[pc++].i = 0x0B; }
    break;

  case 27:
#line 89 "parser.y"
    { code[pc++].i = 0x0C; }
    break;

  case 28:
#line 90 "parser.y"
    { code[pc++].i = 0x0D; }
    break;

  case 29:
#line 91 "parser.y"
    { code[pc++].i = 0x0E; }
    break;

  case 30:
#line 92 "parser.y"
    { code[pc++].i = 0x0F; }
    break;

  case 31:
#line 93 "parser.y"
    { code[pc++].i = 0x10; }
    break;

  case 32:
#line 94 "parser.y"
    { code[pc++].i = 0x11; }
    break;

  case 33:
#line 95 "parser.y"
    { code[pc++].i = 0x12; }
    break;

  case 34:
#line 96 "parser.y"
    { code[pc++].i = 0x13; }
    break;

  case 35:
#line 97 "parser.y"
    { code[pc++].i = 0x14; }
    break;

  case 36:
#line 103 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x1C;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 37:
#line 107 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x1B;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 38:
#line 111 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x1D;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 39:
#line 115 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x1E;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 40:
#line 119 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x19;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 41:
#line 123 "parser.y"
    {	code[pc++].i = 0x02;
							code[pc++].i = 0x1A;
							push_c(pc);
							code[pc++].i = 0x00; }
    break;

  case 42:
#line 129 "parser.y"
    { code[pop_c()].i = pc; }
    break;

  case 43:
#line 132 "parser.y"
    {	code[pc++].i = 0x01F;
							i = pop_c();
							push_c(pc);
							code[pc++].i = 0x00;
							code[i].i = pc; }
    break;

  case 44:
#line 139 "parser.y"
    { code[pop_c()].i = pc; }
    break;


    }

/* Line 991 of yacc.c.  */
#line 1376 "parser.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__) \
    && !defined __cplusplus
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 141 "parser.y"


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


