/* A Bison parser, made by GNU Bison 2.0.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
     FNUMBER = 258,
     DNUMBER = 259,
     STRING = 260,
     IDENT = 261,
     VARIABLE = 262,
     ECHO = 263,
     EXIT = 264,
     IF = 265,
     DO = 266,
     WHILE = 267,
     ENDWHILE = 268,
     FOR = 269,
     ENDFOR = 270,
     FOREACH = 271,
     ENDFOREACH = 272,
     DECLARE = 273,
     ENDDECLARE = 274,
     AS = 275,
     CONST = 276,
     GLOBAL = 277,
     UNSET = 278,
     ISSET = 279,
     EMPTY = 280,
     SWITCH = 281,
     ENDSWITCH = 282,
     CASE = 283,
     DEFAULT = 284,
     BREAK = 285,
     CONTINUE = 286,
     FUNCTION = 287,
     RETURN = 288,
     CLASS = 289,
     INTERFACE = 290,
     EXTENDS = 291,
     IMPLEMENTS = 292,
     OBJECT_OPERATOR = 293,
     HASH_ASSIGN = 294,
     LIST = 295,
     ARRAY = 296,
     CLASS_SCOPE = 297,
     LOG_OR = 298,
     LOG_XOR = 299,
     LOG_AND = 300,
     PRINT = 301,
     SR_EQ = 302,
     SL_EQ = 303,
     XOR_EQ = 304,
     OR_EQ = 305,
     AND_EQ = 306,
     MOD_EQ = 307,
     CONCAT_EQ = 308,
     DIV_EQ = 309,
     MUL_EQ = 310,
     MINUS_EQ = 311,
     PLUS_EQ = 312,
     BOOLEAN_OR = 313,
     BOOLEAN_AND = 314,
     IS_NOIDENTICAL = 315,
     IS_IDENTICAL = 316,
     IS_NOEQUAL = 317,
     IS_EQ = 318,
     IS_GREATER_OR_EQ = 319,
     IS_SMALLER_OR_EQ = 320,
     SR = 321,
     SL = 322,
     INSTANCEOF = 323,
     UNSECAST = 324,
     BOOL_CAST = 325,
     OBJECCAST = 326,
     ARRAY_CAST = 327,
     STRING_CAST = 328,
     DOUBLE_CAST = 329,
     INT_CAST = 330,
     DEC = 331,
     INC = 332,
     CLONE = 333,
     NEW = 334,
     ELSEIF = 335,
     ELSE = 336,
     ENDIF = 337,
     PUBLIC = 338,
     PROTECTED = 339,
     PRIVATE = 340,
     FINAL = 341,
     ABSTRACT = 342,
     STATIC = 343
   };
#endif
#define FNUMBER 258
#define DNUMBER 259
#define STRING 260
#define IDENT 261
#define VARIABLE 262
#define ECHO 263
#define EXIT 264
#define IF 265
#define DO 266
#define WHILE 267
#define ENDWHILE 268
#define FOR 269
#define ENDFOR 270
#define FOREACH 271
#define ENDFOREACH 272
#define DECLARE 273
#define ENDDECLARE 274
#define AS 275
#define CONST 276
#define GLOBAL 277
#define UNSET 278
#define ISSET 279
#define EMPTY 280
#define SWITCH 281
#define ENDSWITCH 282
#define CASE 283
#define DEFAULT 284
#define BREAK 285
#define CONTINUE 286
#define FUNCTION 287
#define RETURN 288
#define CLASS 289
#define INTERFACE 290
#define EXTENDS 291
#define IMPLEMENTS 292
#define OBJECT_OPERATOR 293
#define HASH_ASSIGN 294
#define LIST 295
#define ARRAY 296
#define CLASS_SCOPE 297
#define LOG_OR 298
#define LOG_XOR 299
#define LOG_AND 300
#define PRINT 301
#define SR_EQ 302
#define SL_EQ 303
#define XOR_EQ 304
#define OR_EQ 305
#define AND_EQ 306
#define MOD_EQ 307
#define CONCAT_EQ 308
#define DIV_EQ 309
#define MUL_EQ 310
#define MINUS_EQ 311
#define PLUS_EQ 312
#define BOOLEAN_OR 313
#define BOOLEAN_AND 314
#define IS_NOIDENTICAL 315
#define IS_IDENTICAL 316
#define IS_NOEQUAL 317
#define IS_EQ 318
#define IS_GREATER_OR_EQ 319
#define IS_SMALLER_OR_EQ 320
#define SR 321
#define SL 322
#define INSTANCEOF 323
#define UNSECAST 324
#define BOOL_CAST 325
#define OBJECCAST 326
#define ARRAY_CAST 327
#define STRING_CAST 328
#define DOUBLE_CAST 329
#define INT_CAST 330
#define DEC 331
#define INC 332
#define CLONE 333
#define NEW 334
#define ELSEIF 335
#define ELSE 336
#define ENDIF 337
#define PUBLIC 338
#define PROTECTED 339
#define PRIVATE 340
#define FINAL 341
#define ABSTRACT 342
#define STATIC 343




/* Copy the first part of user declarations.  */
#line 1 "php_parser.y"

#include <stdio.h>

#include "php_syntree.h"

int yylex();

// add item to syntree list
PHP_SYN_NODE *add_statement_2_list(PHP_SYN_NODE *list, PHP_SYN_NODE *st)
{
	if ( st && list) {
		PHP_SYN_NODE *last = list;
		while ( last->next_node ) {
			last = last->next_node;
		}
		last->next_node = st;
		return list;
	} else if ( st ) {
		return st;
	} else {
		return list;
	}
}

PHP_SYN_NODE *add_branch_2_elseif(PHP_SYN_NODE *list, PHP_SYN_NODE *branch)
{
	if ( list ) {
		PHP_SYN_NODE *curr_if = list;
		while ( curr_if->node_if.code_else ) {
			curr_if = curr_if->node_if.code_else;
		}
		curr_if->node_if.code_else = branch;
		return list;
	} else {
		return branch;
	}
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 41 "php_parser.y"
typedef union YYSTYPE {
	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 299 "php_parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 311 "php_parser.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2919

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  115
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  32
/* YYNRULES -- Number of rules. */
#define YYNRULES  162
/* YYNRULES -- Number of states. */
#define YYNSTATES  375

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   343

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,   113,    82,    66,     2,
     111,   112,    80,    77,    43,    78,    79,    81,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    61,   110,
      71,    48,    72,    60,    86,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,   114,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   108,    64,   109,    85,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    44,    45,
      46,    47,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    62,    63,    67,    68,    69,    70,    73,
      74,    75,    76,    84,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     8,     9,    11,    13,    17,    20,
      24,    28,    36,    42,    50,    60,    66,    69,    73,    76,
      80,    83,    87,    91,    97,   105,   115,   126,   132,   140,
     142,   146,   152,   154,   158,   160,   164,   168,   174,   176,
     180,   182,   186,   188,   191,   196,   205,   215,   218,   222,
     227,   233,   234,   235,   237,   239,   244,   246,   251,   253,
     254,   261,   262,   263,   266,   268,   273,   277,   282,   287,
     293,   294,   300,   305,   307,   309,   311,   313,   315,   317,
     321,   325,   327,   331,   336,   341,   346,   351,   358,   365,
     367,   370,   374,   379,   380,   387,   389,   393,   395,   400,
     404,   408,   412,   416,   420,   424,   428,   432,   436,   440,
     444,   447,   450,   453,   456,   460,   464,   468,   472,   476,
     480,   484,   488,   492,   496,   500,   504,   508,   512,   516,
     520,   523,   526,   529,   532,   536,   540,   544,   548,   552,
     556,   560,   564,   568,   574,   577,   580,   583,   586,   589,
     592,   595,   598,   601,   603,   606,   610,   613,   614,   616,
     620,   622,   627
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     116,     0,    -1,   117,    -1,   117,   118,    -1,    -1,   119,
      -1,   126,    -1,   108,   117,   109,    -1,   143,   110,    -1,
      22,   124,   110,    -1,   107,   123,   110,    -1,    10,   111,
     143,   112,   119,   132,   133,    -1,    12,   111,   143,   112,
     134,    -1,    11,   119,    12,   111,   143,   112,   110,    -1,
      14,   111,   131,   110,   131,   110,   131,   112,   129,    -1,
      26,   111,   143,   112,   135,    -1,    31,   110,    -1,    31,
     143,   110,    -1,    30,   110,    -1,    30,   143,   110,    -1,
      33,   110,    -1,    33,   143,   110,    -1,     8,   121,   110,
      -1,    23,   111,   122,   112,   110,    -1,    16,   111,   143,
      20,   139,   112,   130,    -1,    16,   111,   143,    20,   139,
      39,   139,   112,   130,    -1,    16,   111,   143,    20,   139,
      39,    66,   139,   112,   130,    -1,    18,   111,   120,   112,
     119,    -1,    18,   111,   120,   112,    61,   117,    19,    -1,
     110,    -1,     6,    48,   138,    -1,   120,    43,     6,    48,
     138,    -1,   143,    -1,   121,    43,   143,    -1,   139,    -1,
     122,    43,   139,    -1,   123,    43,     7,    -1,   123,    43,
       7,    48,   138,    -1,     7,    -1,     7,    48,   138,    -1,
     125,    -1,   124,    43,   125,    -1,     7,    -1,   113,   139,
      -1,   113,   108,   143,   109,    -1,    32,     6,   111,   127,
     112,   108,   117,   109,    -1,    32,    66,     6,   111,   127,
     112,   108,   117,   109,    -1,   128,     7,    -1,   128,    66,
       7,    -1,   127,    43,   128,     7,    -1,   127,    43,   128,
      66,     7,    -1,    -1,    -1,     6,    -1,   119,    -1,    61,
     117,    15,   110,    -1,   119,    -1,    61,   117,    17,   110,
      -1,   121,    -1,    -1,   132,    99,   111,   143,   112,   119,
      -1,    -1,    -1,   100,   119,    -1,   119,    -1,    61,   117,
      13,   110,    -1,   108,   136,   109,    -1,   108,   110,   136,
     109,    -1,    61,   136,    27,   110,    -1,    61,   110,   136,
      27,   110,    -1,    -1,   136,    28,   143,   137,   117,    -1,
     136,    29,   137,   117,    -1,    61,    -1,   110,    -1,     3,
      -1,     4,    -1,     5,    -1,   140,    -1,     6,    42,     6,
      -1,   140,    38,     6,    -1,     7,    -1,   140,    96,   114,
      -1,   140,    96,   143,   114,    -1,   140,   108,   143,   109,
      -1,   113,   108,   143,   109,    -1,     6,   111,   142,   112,
      -1,   140,    42,     6,   111,   142,   112,    -1,   140,    38,
       6,   111,   142,   112,    -1,   143,    -1,    66,   139,    -1,
     142,    43,   143,    -1,   142,    43,    66,   139,    -1,    -1,
      40,   111,   145,   112,    48,   143,    -1,   139,    -1,   139,
      48,   143,    -1,   141,    -1,   139,    48,    66,   139,    -1,
     139,    59,   143,    -1,   139,    58,   143,    -1,   139,    57,
     143,    -1,   139,    56,   143,    -1,   139,    55,   143,    -1,
     139,    54,   143,    -1,   139,    53,   143,    -1,   139,    52,
     143,    -1,   139,    51,   143,    -1,   139,    50,   143,    -1,
     139,    49,   143,    -1,   139,    95,    -1,    95,   139,    -1,
     139,    94,    -1,    94,   139,    -1,   143,    62,   143,    -1,
     143,    63,   143,    -1,   143,    44,   143,    -1,   143,    46,
     143,    -1,   143,    45,   143,    -1,   143,    64,   143,    -1,
     143,    66,   143,    -1,   143,    65,   143,    -1,   143,    79,
     143,    -1,   143,    77,   143,    -1,   143,    78,   143,    -1,
     143,    80,   143,    -1,   143,    81,   143,    -1,   143,    82,
     143,    -1,   143,    76,   143,    -1,   143,    75,   143,    -1,
      77,   143,    -1,    78,   143,    -1,    83,   143,    -1,    85,
     143,    -1,   143,    68,   143,    -1,   143,    67,   143,    -1,
     143,    70,   143,    -1,   143,    69,   143,    -1,   143,    71,
     143,    -1,   143,    74,   143,    -1,   143,    72,   143,    -1,
     143,    73,   143,    -1,   111,   143,   112,    -1,   143,    60,
     143,    61,   143,    -1,    93,   143,    -1,    92,   143,    -1,
      91,   143,    -1,    90,   143,    -1,    89,   143,    -1,    88,
     143,    -1,    87,   143,    -1,     9,   144,    -1,    86,   143,
      -1,   138,    -1,    47,   143,    -1,   111,   143,   112,    -1,
     111,   112,    -1,    -1,   146,    -1,   145,    43,   146,    -1,
     139,    -1,    40,   111,   145,   112,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   115,   115,   119,   120,   125,   126,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     156,   157,   160,   161,   164,   165,   169,   170,   171,   172,
     175,   176,   180,   181,   182,   186,   187,   191,   192,   193,
     194,   195,   199,   200,   204,   205,   209,   210,   213,   214,
     218,   219,   223,   224,   227,   228,   232,   233,   234,   235,
     239,   240,   241,   244,   245,   249,   250,   251,   254,   255,
     256,   260,   261,   262,   263,   264,   268,   269,   270,   273,
     274,   275,   276,   277,   282,   283,   284,   285,   286,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     303,   304,   305,   306,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   348,   350,   353,   354,   355,   358,   359,
     363,   364,   365
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FNUMBER", "DNUMBER", "STRING", "IDENT",
  "VARIABLE", "ECHO", "EXIT", "IF", "DO", "WHILE", "ENDWHILE", "FOR",
  "ENDFOR", "FOREACH", "ENDFOREACH", "DECLARE", "ENDDECLARE", "AS",
  "CONST", "GLOBAL", "UNSET", "ISSET", "EMPTY", "SWITCH", "ENDSWITCH",
  "CASE", "DEFAULT", "BREAK", "CONTINUE", "FUNCTION", "RETURN", "CLASS",
  "INTERFACE", "EXTENDS", "IMPLEMENTS", "OBJECT_OPERATOR", "HASH_ASSIGN",
  "LIST", "ARRAY", "CLASS_SCOPE", "','", "LOG_OR", "LOG_XOR", "LOG_AND",
  "PRINT", "'='", "SR_EQ", "SL_EQ", "XOR_EQ", "OR_EQ", "AND_EQ", "MOD_EQ",
  "CONCAT_EQ", "DIV_EQ", "MUL_EQ", "MINUS_EQ", "PLUS_EQ", "'?'", "':'",
  "BOOLEAN_OR", "BOOLEAN_AND", "'|'", "'^'", "'&'", "IS_NOIDENTICAL",
  "IS_IDENTICAL", "IS_NOEQUAL", "IS_EQ", "'<'", "'>'", "IS_GREATER_OR_EQ",
  "IS_SMALLER_OR_EQ", "SR", "SL", "'+'", "'-'", "'.'", "'*'", "'/'", "'%'",
  "'!'", "INSTANCEOF", "'~'", "'@'", "UNSECAST", "BOOL_CAST", "OBJECCAST",
  "ARRAY_CAST", "STRING_CAST", "DOUBLE_CAST", "INT_CAST", "DEC", "INC",
  "'['", "CLONE", "NEW", "ELSEIF", "ELSE", "ENDIF", "PUBLIC", "PROTECTED",
  "PRIVATE", "FINAL", "ABSTRACT", "STATIC", "'{'", "'}'", "';'", "'('",
  "')'", "'$'", "']'", "$accept", "program_tree", "top_statement_list",
  "top_statement", "statement", "decl_list", "expr_list", "variable_list",
  "static_var_list", "global_var_list", "global_var",
  "function_decl_statement", "parameter_list", "optional_class_type",
  "for_statement", "foreach_statement", "for_expr", "elseif_list",
  "else_statement", "while_statement", "switch_case_list", "case_list",
  "case_separator", "const_value", "variable", "deref_variable",
  "function_call", "func_param_list", "expr", "exit_expr",
  "assignment_list", "assignment_list_element", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,    44,   298,   299,   300,   301,    61,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
      63,    58,   313,   314,   124,    94,    38,   315,   316,   317,
     318,    60,    62,   319,   320,   321,   322,    43,    45,    46,
      42,    47,    37,    33,   323,   126,    64,   324,   325,   326,
     327,   328,   329,   330,   331,   332,    91,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   123,   125,
      59,    40,    41,    36,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   115,   116,   117,   117,   118,   118,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     120,   120,   121,   121,   122,   122,   123,   123,   123,   123,
     124,   124,   125,   125,   125,   126,   126,   127,   127,   127,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   134,   135,   135,   135,   135,
     136,   136,   136,   137,   137,   138,   138,   138,   139,   139,
     139,   140,   140,   140,   140,   140,   141,   141,   141,   142,
     142,   142,   142,   142,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   144,   144,   144,   145,   145,
     146,   146,   146
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     2,     0,     1,     1,     3,     2,     3,
       3,     7,     5,     7,     9,     5,     2,     3,     2,     3,
       2,     3,     3,     5,     7,     9,    10,     5,     7,     1,
       3,     5,     1,     3,     1,     3,     3,     5,     1,     3,
       1,     3,     1,     2,     4,     8,     9,     2,     3,     4,
       5,     0,     0,     1,     1,     4,     1,     4,     1,     0,
       6,     0,     0,     2,     1,     4,     3,     4,     4,     5,
       0,     5,     4,     1,     1,     1,     1,     1,     1,     3,
       3,     1,     3,     4,     4,     4,     4,     6,     6,     1,
       2,     3,     4,     0,     6,     1,     3,     1,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     2,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     2,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     2,     3,     2,     0,     1,     3,
       1,     4,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       4,     0,     2,     1,    75,    76,    77,     0,    81,     0,
     157,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     4,    29,     0,     0,     3,     5,     6,   153,    95,
      78,    97,     0,     0,    93,     0,    32,     0,   151,     0,
       0,     0,    59,     0,     0,    42,     0,     0,    40,     0,
       0,    18,     0,    16,     0,     0,     0,    20,     0,   162,
     154,   130,   131,   132,   133,   152,   150,   149,   148,   147,
     146,   145,   144,     0,   113,    78,   111,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   110,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,    79,     0,     0,    89,
       0,    22,   156,     0,     0,     0,     0,    58,     0,     0,
       0,     0,     0,    43,     0,     9,     0,    34,     0,    19,
      17,    51,     0,    21,     0,   160,     0,   158,     0,     0,
       0,    10,     7,   142,     0,     0,    96,   109,   108,   107,
     106,   105,   104,   103,   102,   101,   100,    99,    80,     0,
      82,     0,     0,   116,   118,   117,     0,   114,   115,   119,
     121,   120,   135,   134,   137,   136,   138,   140,   141,   139,
     129,   128,   123,   124,   122,   125,   126,   127,    90,     0,
      86,    33,   155,     0,     0,     0,    59,     0,     0,     0,
       0,     0,    41,     0,     0,     0,    53,     0,     0,    51,
     162,   162,     0,    80,    39,    36,    85,    98,    93,    93,
      83,    84,     0,     0,    91,    61,     0,     4,    64,    12,
       0,     0,    30,     0,     4,    27,    44,    35,    23,    70,
      70,    15,    52,     0,    47,     0,     0,     0,   159,     0,
       0,     0,     0,   143,    92,    62,     0,     0,    59,     0,
       0,     0,     0,    70,     0,    70,     0,     0,     4,    48,
       0,   161,    94,    37,    88,    87,     0,     0,    11,    13,
       0,     0,     0,     0,     4,    56,    24,    31,    28,     0,
       0,     0,     0,     0,    66,    49,     0,     0,     4,     0,
      63,    65,     0,     0,     0,     0,     0,    68,     0,    73,
      74,     4,    67,    50,    45,     0,     0,     4,    54,    14,
       0,    25,     0,    69,     4,    72,    46,     0,     0,    26,
      57,    71,    60,     0,    55
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,    45,    46,   161,   157,   166,    98,    67,
      68,    47,   247,   248,   359,   326,   158,   295,   318,   269,
     281,   304,   351,    48,    49,    50,    51,   148,    52,    58,
     176,   177
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -308
static const short int yypact[] =
{
    -308,    46,  1144,  -308,  -308,  -308,  -308,   -33,  -308,  2147,
     -70,   -60,  1699,   -56,   -47,   -34,    39,    -2,    54,    77,
    1755,  1811,     5,  1867,    79,  2147,  2147,  2147,  2147,  2147,
    2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,    36,    36,
      62,  -308,  -308,  2147,   -18,  -308,  -308,  -308,  -308,  2157,
      47,  -308,  2199,   174,  1923,   -31,  2635,  1979,  -308,  2147,
     202,  2147,  2147,  2147,   209,  -308,    38,    31,  -308,    36,
    2147,  -308,  2248,  -308,  2297,   111,   219,  -308,  2346,    41,
    2516,    87,    87,  -308,  -308,  -308,  -308,  -308,  -308,  -308,
    -308,  -308,  -308,   184,  -308,   -24,  -308,   179,    49,   367,
     357,  2147,  2035,  2147,  2147,  2147,  2147,  2147,  2147,  2147,
    2147,  2147,  2147,  2147,  -308,  -308,   224,   227,   273,  2147,
    2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,
    2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,  2147,
    2147,  2147,  2147,  2147,  2147,  -308,  -308,    36,   -26,  2635,
    2147,  -308,  -308,   468,   579,   124,   690,   193,   127,  2557,
     194,    30,  2147,  -308,    -2,  -308,    44,  -308,   801,  -308,
    -308,   146,   130,  -308,   134,  -308,    48,  -308,   240,    56,
     242,  -308,  -308,  -308,  2395,    36,  2516,  2516,  2516,  2516,
    2516,  2516,  2516,  2516,  2516,  2516,  2516,  2516,   139,   144,
    -308,   129,  2443,  2673,  2710,  2516,  2596,  2751,  2770,  2788,
    2805,  2821,  2837,  2837,  2837,  2837,   264,   264,   264,   264,
     188,   188,    87,    87,    87,  -308,  -308,  -308,  -308,  2091,
    -308,  2635,  -308,  1699,  2147,  1255,  2147,    36,    56,   250,
    1366,  2491,  -308,    36,   148,   105,  -308,    51,    -4,   146,
      41,    41,   211,  -308,  -308,   213,  -308,  -308,  1923,  1923,
    -308,  -308,  2147,    36,  2635,  -308,   912,  -308,  -308,  -308,
     152,   -29,  -308,   215,  -308,  -308,  -308,  -308,  -308,   161,
     162,  -308,   268,   167,  -308,   274,    52,    67,  -308,  2147,
      56,    69,   104,  2731,  -308,    71,   173,   478,  2147,    27,
    1477,    56,   589,  -308,   149,  -308,   -27,   151,  -308,  -308,
     177,  -308,  2516,  -308,  -308,  -308,   175,  1699,  -308,  -308,
     180,   182,    36,   183,  -308,  -308,  -308,  -308,  -308,   191,
     181,  2147,   -42,   -21,  -308,  -308,   280,   700,  -308,  2147,
    -308,  -308,  1588,   185,  1477,   811,   189,  -308,  1134,  -308,
    -308,  -308,  -308,  -308,  -308,   922,  1023,  -308,  -308,  -308,
    1477,  -308,   192,  -308,  -308,  1144,  -308,  1699,  1033,  -308,
    -308,  1144,  -308,   196,  -308
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -308,  -308,   -35,  -308,   -12,  -308,   283,  -308,  -308,  -308,
     137,  -308,    55,    16,  -308,  -307,  -223,  -308,  -308,  -308,
    -308,  -118,   -41,  -129,    -3,     1,  -308,   -75,    -5,  -308,
      58,    59
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -53
static const short int yytable[] =
{
      60,   331,   332,   284,    56,    65,    99,   331,   332,    53,
     299,    75,   150,   270,   178,    72,    74,   229,    78,   349,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,     8,    94,    96,   361,   100,    95,
      95,    57,    93,     8,    93,     8,     3,    93,     8,   149,
     254,    59,   153,   369,   154,    61,   156,    56,   159,     4,
       5,     6,   285,   163,    62,   168,   167,    95,   350,    97,
      95,    76,   118,   239,   164,   321,   175,    63,    54,   151,
      95,   174,   334,   300,   119,   116,   230,   243,   352,   117,
     101,   251,   180,   322,   282,   282,   184,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   272,
     251,    66,   229,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
      44,   165,   240,   118,   228,   231,   162,   229,    95,    44,
      64,    44,   246,   -52,    44,   119,   244,   241,   335,   181,
     252,   313,   306,   283,   310,    69,   279,   142,   143,   144,
     316,   317,   327,   120,   121,   122,   330,   331,   332,   311,
     146,   314,   257,   291,   292,   329,    95,   333,    70,   123,
      79,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   -52,   280,   155,   160,   315,   336,   346,   331,
     332,   265,   171,   268,   264,   172,    53,   179,   275,   266,
     198,    56,   297,   199,   271,   234,   150,   236,    95,   302,
     277,   249,   238,   260,    95,   250,   253,   175,   175,   255,
     258,    95,    95,   149,   149,   259,   273,   293,   278,   289,
     294,   290,   298,   301,    95,   139,   140,   141,   142,   143,
     144,   303,   305,   337,   246,   308,     4,     5,     6,     7,
       8,   309,    10,   319,   312,   338,   339,   353,   325,   345,
     341,   347,    55,    56,   342,   344,   323,   360,   307,   363,
      95,   242,   370,   355,   286,   340,   374,   364,   287,     0,
     288,     0,     0,    24,     0,     0,   365,     0,     0,   343,
      25,     0,   368,    95,     0,     0,   348,     0,     0,   371,
     358,     0,   325,     0,   356,   -53,   -53,   -53,   -53,   137,
     138,   139,   140,   141,   142,   143,   144,     0,   325,     0,
      26,    27,     0,     0,     0,   372,    28,     0,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,     0,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
       0,    14,     0,    15,    43,    16,    44,   200,     0,    17,
      18,     0,     0,    19,     0,     0,     0,    20,    21,    22,
      23,   120,   121,   122,     0,     0,     0,    24,     0,     0,
       0,     0,     0,     0,    25,     0,     0,   123,     0,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
       0,     0,     0,     0,    26,    27,     0,     0,     0,     0,
      28,     0,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,     0,     0,     0,     0,     0,     0,   183,
       0,     0,     0,     0,    40,    41,   182,    42,    43,     0,
      44,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,   320,    14,     0,    15,     0,    16,     0,     0,     0,
      17,    18,     0,     0,    19,     0,     0,     0,    20,    21,
      22,    23,   120,   121,   122,     0,     0,     0,    24,     0,
       0,     0,     0,     0,     0,    25,     0,     0,   123,     0,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,     0,     0,     0,     0,    26,    27,     0,     0,     0,
       0,    28,     0,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,     0,     0,     0,     0,     0,     0,
     232,     0,     0,     0,     0,    40,    41,     0,    42,    43,
       0,    44,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,     0,    14,     0,    15,     0,    16,   328,     0,
       0,    17,    18,     0,     0,    19,     0,     0,     0,    20,
      21,    22,    23,   120,   121,   122,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,    25,     0,     0,   123,
       0,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,     0,     0,     0,     0,    26,    27,     0,     0,
       0,     0,    28,     0,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,     0,     0,     0,     0,     0,
       0,   233,     0,     0,     0,     0,    40,    41,     0,    42,
      43,     0,    44,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,     0,    14,     0,    15,     0,    16,     0,
       0,     0,    17,    18,     0,     0,    19,     0,     0,     0,
      20,    21,    22,    23,   120,   121,   122,     0,     0,     0,
      24,     0,     0,     0,     0,     0,     0,    25,     0,     0,
     123,     0,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,     0,     0,     0,     0,    26,    27,     0,
       0,     0,     0,    28,     0,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,     0,     0,     0,     0,
       0,     0,   235,     0,     0,     0,     0,    40,    41,   354,
      42,    43,     0,    44,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,     0,    14,     0,    15,   362,    16,
       0,     0,     0,    17,    18,     0,     0,    19,     0,     0,
       0,    20,    21,    22,    23,   120,   121,   122,     0,     0,
       0,    24,     0,     0,     0,     0,     0,     0,    25,     0,
       0,   123,     0,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,     0,     0,     0,     0,    26,    27,
       0,     0,     0,     0,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,     0,     0,     0,
       0,     0,     0,   245,     0,     0,     0,     0,    40,    41,
       0,    42,    43,     0,    44,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,     0,    14,     0,    15,     0,
      16,     0,     0,     0,    17,    18,     0,     0,    19,     0,
       0,     0,    20,    21,    22,    23,   120,   121,   122,     0,
       0,     0,    24,     0,     0,     0,     0,     0,     0,    25,
       0,     0,   123,     0,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,     0,     0,     0,     0,    26,
      27,     0,     0,     0,     0,    28,     0,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,     0,     0,
       0,     0,     0,     0,   296,     0,     0,     0,     0,    40,
      41,   366,    42,    43,     0,    44,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,     0,    14,   373,    15,
       0,    16,     0,     0,     0,    17,    18,     0,     0,    19,
       0,     0,     0,    20,    21,    22,    23,   120,   121,   122,
       0,     0,     0,    24,     0,     0,     0,     0,     0,     0,
      25,     0,     0,   123,     0,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,     0,     0,     0,     0,
      26,    27,     0,     0,     0,     0,    28,     0,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,     0,
       0,     0,     0,     0,     0,   367,     0,     0,     0,     0,
      40,    41,     0,    42,    43,     0,    44,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,     0,    14,     0,
      15,     0,    16,     0,     0,     0,    17,    18,     0,     0,
      19,     0,     0,     0,    20,    21,    22,    23,   120,   121,
     122,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,    25,     0,     0,   123,   349,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,     0,     0,     0,
       0,    26,    27,     0,     0,     0,     0,    28,     0,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
       0,     0,     0,     0,   350,     0,     0,     0,     0,     0,
       0,    40,    41,     0,    42,    43,     0,    44,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,     0,    14,
       0,    15,     0,    16,     0,     0,     0,    17,    18,     0,
       0,    19,     0,     0,     0,    20,    21,     0,    23,     0,
       0,     0,     0,     0,     0,    24,     0,     0,     0,     0,
       0,     0,    25,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   267,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    26,    27,     0,     0,     0,     0,    28,     0,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    40,    41,     0,    42,    43,     0,    44,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,     0,
      14,     0,    15,     0,    16,     0,     0,     0,    17,    18,
       0,     0,    19,     0,     0,     0,    20,    21,     0,    23,
       0,     0,     0,     0,     0,     0,    24,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   274,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    27,     0,     0,     0,     0,    28,
       0,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    40,    41,     0,    42,    43,     0,    44,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
       0,    14,     0,    15,     0,    16,     0,     0,     0,    17,
      18,     0,     0,    19,     0,     0,     0,    20,    21,     0,
      23,     0,     0,     0,     0,     0,     0,    24,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   324,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    26,    27,     0,     0,     0,     0,
      28,     0,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    40,    41,     0,    42,    43,     0,
      44,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,     0,    14,     0,    15,     0,    16,     0,     0,     0,
      17,    18,     0,     0,    19,     0,     0,     0,    20,    21,
       0,    23,     0,     0,     0,     0,     0,     0,    24,     0,
       0,     0,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   357,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    26,    27,     0,     0,     0,
       0,    28,     0,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    40,    41,     0,    42,    43,
       0,    44,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,     0,    14,     0,    15,     0,    16,     0,     0,
       0,    17,    18,     0,     0,    19,     0,     0,     0,    20,
      21,     0,    23,     0,     0,     0,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,    25,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     4,     5,
       6,     7,     8,     0,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    26,    27,     0,     0,
       0,     0,    28,     0,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    24,     0,     0,     0,     0,
       0,     0,    25,     0,     0,     0,    40,    41,     0,    42,
      43,     0,    44,     0,     4,     5,     6,     7,     8,     0,
      10,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    26,    27,     0,     0,     0,     0,    28,     0,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    24,     0,     0,     0,     0,     0,     0,    25,     0,
       0,     0,     0,     0,     0,    71,    43,     0,    44,     0,
       4,     5,     6,     7,     8,     0,    10,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    26,    27,
       0,     0,     0,     0,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    24,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,     0,     0,
       0,    73,    43,     0,    44,     0,     4,     5,     6,     7,
       8,     0,    10,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    26,    27,     0,     0,     0,     0,
      28,     0,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    24,     0,     0,     0,     0,     0,     0,
      25,     0,     0,     0,     0,     0,     0,    77,    43,     0,
      44,     0,     4,     5,     6,     7,     8,     0,    10,   147,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      26,    27,     0,     0,     0,     0,    28,     0,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    24,
       0,     0,     0,     0,     0,     0,    25,     0,     0,     0,
       0,     0,     0,     0,    43,     0,    44,     0,     4,     5,
       6,     7,     8,     0,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    26,    27,     0,     0,
       0,     0,    28,     0,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    24,     0,     0,     0,     0,
       0,     0,    25,     0,     0,     0,     0,     0,     0,     0,
      43,   152,    44,     0,     4,     5,     6,     7,     8,     0,
      10,   185,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    26,    27,     0,     0,     0,     0,    28,     0,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    24,     0,     0,     0,     0,     0,     0,    25,     0,
       0,     0,     0,     0,     0,     0,    43,     0,    44,     0,
       4,     5,     6,     7,     8,     0,    10,   263,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    26,    27,
       0,     0,     0,     0,    28,     0,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    24,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,     0,     0,
       0,     0,    43,     0,    44,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,     0,     0,     0,
       0,     0,     0,     0,    26,    27,     0,     0,     0,     0,
      28,     0,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,   120,   121,   122,     0,     0,     0,     0,
       0,   114,   115,     0,     0,     0,     0,     0,    43,   123,
      44,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   120,   121,   122,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   123,   145,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   120,   121,   122,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   123,   169,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     120,   121,   122,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   123,   170,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   120,
     121,   122,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   123,   173,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   120,   121,   122,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   123,   256,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   120,   121,   122,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   123,   261,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,     0,     0,   123,   237,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,     0,
     276,   120,   121,   122,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   123,     0,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     120,   121,   122,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   123,   262,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   120,
     121,   122,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   123,     0,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   121,   122,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   123,     0,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   122,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     123,     0,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   -53,   -53,   -53,   -53,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144
};

static const short int yycheck[] =
{
      12,    28,    29,     7,     9,     7,    41,    28,    29,    42,
      39,     6,    43,   236,    38,    20,    21,    43,    23,    61,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,     6,     7,    38,    39,   344,    43,    38,
      39,   111,     6,     7,     6,     7,     0,     6,     7,    54,
     179,   111,    57,   360,    59,   111,    61,    62,    63,     3,
       4,     5,    66,    66,   111,    70,    69,    66,   110,     7,
      69,    66,    96,    43,    43,   298,    79,   111,   111,   110,
      79,    40,   109,   112,   108,    38,   112,    43,   109,    42,
     108,    43,    43,    66,    43,    43,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   238,
      43,   113,    43,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     113,   110,   112,    96,   147,   150,   108,    43,   147,   113,
     111,   113,     6,     7,   113,   108,   112,   162,     7,   110,
     112,   290,   280,   112,   112,   111,    61,    80,    81,    82,
      99,   100,   301,    44,    45,    46,    27,    28,    29,   112,
       6,   112,   185,   258,   259,   303,   185,   305,   111,    60,
     111,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    66,   108,    12,     6,   112,    66,    27,    28,
      29,   233,   111,   235,   229,     6,    42,    48,   240,   234,
       6,   236,   267,     6,   237,   111,    43,   110,   237,   274,
     243,   111,    48,   114,   243,   111,     6,   250,   251,     7,
     111,   250,   251,   258,   259,   111,     6,   262,   110,    48,
     263,    48,   110,    48,   263,    77,    78,    79,    80,    81,
      82,   110,   110,   308,     6,   108,     3,     4,     5,     6,
       7,     7,     9,   110,   289,   108,   111,     7,   300,   324,
     110,   110,     9,   298,   112,   112,   299,   112,   282,   110,
     299,   164,   110,   338,   249,   317,   110,   348,   250,    -1,
     251,    -1,    -1,    40,    -1,    -1,   351,    -1,    -1,   322,
      47,    -1,   357,   322,    -1,    -1,   331,    -1,    -1,   364,
     342,    -1,   344,    -1,   339,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,   360,    -1,
      77,    78,    -1,    -1,    -1,   367,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,   111,    18,   113,   114,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    44,    45,    46,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,   107,   108,   109,   110,   111,    -1,
     113,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,
      32,    33,    44,    45,    46,    -1,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
     112,    -1,    -1,    -1,    -1,   107,   108,    -1,   110,   111,
      -1,   113,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    19,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,   107,   108,    -1,   110,
     111,    -1,   113,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,
      -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,
      30,    31,    32,    33,    44,    45,    46,    -1,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,   112,    -1,    -1,    -1,    -1,   107,   108,   109,
     110,   111,    -1,   113,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    17,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    44,    45,    46,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,   107,   108,
      -1,   110,   111,    -1,   113,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    -1,    16,    -1,
      18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,
      -1,    -1,    30,    31,    32,    33,    44,    45,    46,    -1,
      -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,
      -1,    -1,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    -1,    -1,
      -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,   107,
     108,   109,   110,   111,    -1,   113,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    15,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    44,    45,    46,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,    -1,    -1,
     107,   108,    -1,   110,   111,    -1,   113,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    14,    -1,
      16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    32,    33,    44,    45,
      46,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    47,    -1,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,    -1,    -1,
      -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      -1,    -1,    -1,    -1,   110,    -1,    -1,    -1,    -1,    -1,
      -1,   107,   108,    -1,   110,   111,    -1,   113,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    -1,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   107,   108,    -1,   110,   111,    -1,   113,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,
      -1,    -1,    26,    -1,    -1,    -1,    30,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   107,   108,    -1,   110,   111,    -1,   113,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   107,   108,    -1,   110,   111,    -1,
     113,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   107,   108,    -1,   110,   111,
      -1,   113,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,   107,   108,    -1,   110,
     111,    -1,   113,    -1,     3,     4,     5,     6,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,   110,   111,    -1,   113,    -1,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,   110,   111,    -1,   113,    -1,     3,     4,     5,     6,
       7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,   110,   111,    -1,
     113,    -1,     3,     4,     5,     6,     7,    -1,     9,    66,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   111,    -1,   113,    -1,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     111,   112,   113,    -1,     3,     4,     5,     6,     7,    -1,
       9,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   111,    -1,   113,    -1,
       3,     4,     5,     6,     7,    -1,     9,    66,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   111,    -1,   113,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    44,    45,    46,    -1,    -1,    -1,    -1,
      -1,    94,    95,    -1,    -1,    -1,    -1,    -1,   111,    60,
     113,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,   110,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,   110,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,   110,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,
      45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,   110,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    60,   109,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,   109,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    60,    20,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
     109,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      44,    45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    44,
      45,    46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    45,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   116,   117,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    14,    16,    18,    22,    23,    26,
      30,    31,    32,    33,    40,    47,    77,    78,    83,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
     107,   108,   110,   111,   113,   118,   119,   126,   138,   139,
     140,   141,   143,    42,   111,   121,   143,   111,   144,   111,
     119,   111,   111,   111,   111,     7,   113,   124,   125,   111,
     111,   110,   143,   110,   143,     6,    66,   110,   143,   111,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,     6,   139,   140,   139,     7,   123,   117,
     143,   108,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    94,    95,    38,    42,    96,   108,
      44,    45,    46,    60,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   110,     6,    66,   142,   143,
      43,   110,   112,   143,   143,    12,   143,   121,   131,   143,
       6,   120,   108,   139,    43,   110,   122,   139,   143,   110,
     110,   111,     6,   110,    40,   139,   145,   146,    38,    48,
      43,   110,   109,   112,   143,    66,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,     6,     6,
     114,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   139,    43,
     112,   143,   112,   112,   111,   112,   110,    20,    48,    43,
     112,   143,   125,    43,   112,   112,     6,   127,   128,   111,
     111,    43,   112,     6,   138,     7,   109,   139,   111,   111,
     114,   109,    61,    66,   143,   119,   143,    61,   119,   134,
     131,   139,   138,     6,    61,   119,   109,   139,   110,    61,
     108,   135,    43,   112,     7,    66,   127,   145,   146,    48,
      48,   142,   142,   143,   139,   132,   112,   117,   110,    39,
     112,    48,   117,   110,   136,   110,   136,   128,   108,     7,
     112,   112,   143,   138,   112,   112,    99,   100,   133,   110,
      13,   131,    66,   139,    61,   119,   130,   138,    19,   136,
      27,    28,    29,   136,   109,     7,    66,   117,   108,   111,
     119,   110,   112,   139,   112,   117,    27,   110,   143,    61,
     110,   137,   109,     7,   109,   117,   143,    61,   119,   129,
     112,   130,    17,   110,   137,   117,   109,   112,   117,   130,
     110,   117,   119,    15,   110
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
#define YYERROR		goto yyerrorlab


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


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
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
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
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
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

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



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
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
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

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


  yyvsp[0] = yylval;

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
	short int *yyss1 = yyss;


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
	short int *yyss1 = yyss;
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
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

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
        case 2:
#line 115 "php_parser.y"
    { g_syn_tree_top = (yyvsp[0].syn_node); ;}
    break;

  case 3:
#line 119 "php_parser.y"
    { (yyval.syn_node) = add_statement_2_list((yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 4:
#line 120 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 7:
#line 131 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 8:
#line 132 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node((yyvsp[-1].exp_node)); ;}
    break;

  case 9:
#line 133 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 10:
#line 134 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 11:
#line 135 "php_parser.y"
    { (yyval.syn_node) = make_ifelse_syn_node((yyvsp[-4].exp_node), (yyvsp[-2].syn_node), (yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 12:
#line 136 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 1); ;}
    break;

  case 13:
#line 137 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[-5].syn_node), 0); ;}
    break;

  case 14:
#line 138 "php_parser.y"
    { ;}
    break;

  case 15:
#line 139 "php_parser.y"
    { ;}
    break;

  case 16:
#line 140 "php_parser.y"
    {  ;}
    break;

  case 17:
#line 141 "php_parser.y"
    {  ;}
    break;

  case 18:
#line 142 "php_parser.y"
    {  ;}
    break;

  case 19:
#line 143 "php_parser.y"
    {  ;}
    break;

  case 20:
#line 144 "php_parser.y"
    {  ;}
    break;

  case 21:
#line 145 "php_parser.y"
    {  ;}
    break;

  case 22:
#line 146 "php_parser.y"
    {  ;}
    break;

  case 23:
#line 147 "php_parser.y"
    {  ;}
    break;

  case 24:
#line 148 "php_parser.y"
    {  ;}
    break;

  case 25:
#line 149 "php_parser.y"
    {  ;}
    break;

  case 26:
#line 150 "php_parser.y"
    {  ;}
    break;

  case 27:
#line 151 "php_parser.y"
    { ;}
    break;

  case 28:
#line 152 "php_parser.y"
    { ;}
    break;

  case 29:
#line 153 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 30:
#line 156 "php_parser.y"
    {  ;}
    break;

  case 31:
#line 157 "php_parser.y"
    {  ;}
    break;

  case 32:
#line 160 "php_parser.y"
    {  ;}
    break;

  case 33:
#line 161 "php_parser.y"
    {  ;}
    break;

  case 36:
#line 169 "php_parser.y"
    {  ;}
    break;

  case 37:
#line 170 "php_parser.y"
    {  ;}
    break;

  case 38:
#line 171 "php_parser.y"
    {  ;}
    break;

  case 39:
#line 172 "php_parser.y"
    {  ;}
    break;

  case 40:
#line 175 "php_parser.y"
    {  ;}
    break;

  case 41:
#line 176 "php_parser.y"
    {  ;}
    break;

  case 43:
#line 181 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 44:
#line 182 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[-1].exp_node)); ;}
    break;

  case 45:
#line 186 "php_parser.y"
    {  ;}
    break;

  case 46:
#line 187 "php_parser.y"
    {  ;}
    break;

  case 47:
#line 191 "php_parser.y"
    {  ;}
    break;

  case 48:
#line 192 "php_parser.y"
    {  ;}
    break;

  case 49:
#line 193 "php_parser.y"
    {  ;}
    break;

  case 50:
#line 194 "php_parser.y"
    {  ;}
    break;

  case 52:
#line 199 "php_parser.y"
    {  ;}
    break;

  case 53:
#line 200 "php_parser.y"
    {  ;}
    break;

  case 55:
#line 205 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 57:
#line 210 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 59:
#line 214 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 60:
#line 218 "php_parser.y"
    { (yyval.syn_node) = add_branch_2_elseif((yyvsp[-5].syn_node), make_ifelse_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0, 0)); ;}
    break;

  case 61:
#line 219 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 62:
#line 223 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 63:
#line 224 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[0].syn_node); ;}
    break;

  case 65:
#line 228 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 66:
#line 232 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 67:
#line 233 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 68:
#line 234 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 69:
#line 235 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 70:
#line 239 "php_parser.y"
    {  (yyval.syn_node) = 0; ;}
    break;

  case 71:
#line 240 "php_parser.y"
    {  ;}
    break;

  case 72:
#line 241 "php_parser.y"
    {  ;}
    break;

  case 79:
#line 255 "php_parser.y"
    { ;}
    break;

  case 80:
#line 256 "php_parser.y"
    { ;}
    break;

  case 82:
#line 261 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_BY_KEY, 0); ;}
    break;

  case 83:
#line 262 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 84:
#line 263 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 85:
#line 264 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[-1].exp_node)); ;}
    break;

  case 86:
#line 268 "php_parser.y"
    { (yyval.exp_node) = make_func_call_exp((yyvsp[-3].str_val), (yyvsp[-1].exp_node)); ;}
    break;

  case 87:
#line 269 "php_parser.y"
    { ;}
    break;

  case 88:
#line 270 "php_parser.y"
    { ;}
    break;

  case 89:
#line 273 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 90:
#line 274 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 91:
#line 275 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-2].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 92:
#line 276 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-3].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 93:
#line 277 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); ;}
    break;

  case 94:
#line 282 "php_parser.y"
    { ;}
    break;

  case 96:
#line 284 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ASS, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 97:
#line 285 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 98:
#line 286 "php_parser.y"
    {  ;}
    break;

  case 99:
#line 291 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 100:
#line 292 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 101:
#line 293 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 102:
#line 294 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 103:
#line 295 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 104:
#line 296 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 105:
#line 297 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 106:
#line 298 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 107:
#line 299 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 108:
#line 300 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 109:
#line 301 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 110:
#line 303 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 111:
#line 304 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[0].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 112:
#line 305 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 113:
#line 306 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[0].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 114:
#line 308 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 115:
#line 309 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 116:
#line 310 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 117:
#line 311 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 118:
#line 312 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 119:
#line 313 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 120:
#line 314 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 121:
#line 315 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 122:
#line 316 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 123:
#line 317 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 124:
#line 318 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 125:
#line 319 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 126:
#line 320 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 127:
#line 321 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 128:
#line 322 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 129:
#line 323 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 130:
#line 324 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 131:
#line 325 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), (yyvsp[0].exp_node)); ;}
    break;

  case 132:
#line 326 "php_parser.y"
    {  ;}
    break;

  case 133:
#line 327 "php_parser.y"
    {  ;}
    break;

  case 134:
#line 328 "php_parser.y"
    {  ;}
    break;

  case 135:
#line 329 "php_parser.y"
    {  ;}
    break;

  case 136:
#line 330 "php_parser.y"
    {  ;}
    break;

  case 137:
#line 331 "php_parser.y"
    {  ;}
    break;

  case 138:
#line 332 "php_parser.y"
    {  ;}
    break;

  case 139:
#line 333 "php_parser.y"
    {  ;}
    break;

  case 140:
#line 334 "php_parser.y"
    {  ;}
    break;

  case 141:
#line 335 "php_parser.y"
    {  ;}
    break;

  case 142:
#line 336 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 143:
#line 337 "php_parser.y"
    {  ;}
    break;

  case 144:
#line 338 "php_parser.y"
    {  ;}
    break;

  case 145:
#line 339 "php_parser.y"
    {  ;}
    break;

  case 146:
#line 340 "php_parser.y"
    {  ;}
    break;

  case 147:
#line 341 "php_parser.y"
    {  ;}
    break;

  case 148:
#line 342 "php_parser.y"
    {  ;}
    break;

  case 149:
#line 343 "php_parser.y"
    {  ;}
    break;

  case 150:
#line 344 "php_parser.y"
    {  ;}
    break;

  case 151:
#line 345 "php_parser.y"
    {  ;}
    break;

  case 152:
#line 346 "php_parser.y"
    {  ;}
    break;

  case 153:
#line 348 "php_parser.y"
    { ;}
    break;

  case 154:
#line 350 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_PRINT, (yyvsp[0].exp_node)); ;}
    break;

  case 155:
#line 353 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 156:
#line 354 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 157:
#line 355 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 161:
#line 364 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 162:
#line 365 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 2773 "php_parser.c"

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
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
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
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {

		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 yydestruct ("Error: popping",
                             yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
  yydestruct ("Error: discarding lookahead",
              yytoken, &yylval);
  yychar = YYEMPTY;
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



