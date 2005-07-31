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
     UNSET_CAST = 324,
     BOOL_CAST = 325,
     OBJECT_CAST = 326,
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
     STATIC = 343,
     START_SCRIPT = 344,
     END_SCRIPT = 345
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
#define UNSET_CAST 324
#define BOOL_CAST 325
#define OBJECT_CAST 326
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
#define START_SCRIPT 344
#define END_SCRIPT 345




/* Copy the first part of user declarations.  */
#line 1 "php_parser.y"

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2005 Froenchenko Leonid ( lfroen@amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <stdio.h>
#include <string.h>

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
#line 67 "php_parser.y"
typedef union YYSTYPE {
	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 329 "php_parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 341 "php_parser.c"

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3042

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  117
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  34
/* YYNRULES -- Number of rules. */
#define YYNRULES  162
/* YYNRULES -- Number of states. */
#define YYNSTATES  373

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   345

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,   116,    82,    66,     2,
     113,   114,    80,    77,    43,    78,    79,    81,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    61,   112,
      71,    48,    72,    60,    86,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,   115,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   110,    64,   111,    85,     2,     2,     2,
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
     104,   105,   106,   107,   108,   109
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     7,    10,    11,    13,    15,    19,    22,
      26,    30,    38,    44,    52,    62,    68,    71,    75,    78,
      82,    85,    89,    93,    99,   107,   117,   128,   134,   142,
     144,   148,   154,   156,   160,   162,   166,   168,   172,   174,
     176,   180,   182,   186,   187,   197,   207,   210,   214,   219,
     225,   226,   227,   229,   231,   236,   238,   243,   245,   246,
     253,   254,   255,   258,   260,   265,   269,   274,   279,   285,
     286,   292,   297,   299,   301,   303,   305,   307,   309,   311,
     315,   319,   321,   325,   330,   335,   340,   345,   352,   359,
     361,   364,   368,   373,   374,   381,   383,   387,   389,   394,
     398,   402,   406,   410,   414,   418,   422,   426,   430,   434,
     438,   441,   444,   447,   450,   454,   458,   462,   466,   470,
     474,   478,   482,   486,   490,   494,   498,   502,   506,   510,
     514,   517,   520,   523,   526,   530,   534,   538,   542,   546,
     550,   554,   558,   562,   568,   571,   574,   577,   580,   583,
     586,   589,   592,   595,   597,   600,   604,   607,   608,   610,
     614,   616,   621
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     118,     0,    -1,   108,   119,   109,    -1,   119,   120,    -1,
      -1,   121,    -1,   129,    -1,   110,   119,   111,    -1,   147,
     112,    -1,    22,   125,   112,    -1,   107,   127,   112,    -1,
      10,   113,   147,   114,   121,   136,   137,    -1,    12,   113,
     147,   114,   138,    -1,    11,   121,    12,   113,   147,   114,
     112,    -1,    14,   113,   135,   112,   135,   112,   135,   114,
     133,    -1,    26,   113,   147,   114,   139,    -1,    31,   112,
      -1,    31,   147,   112,    -1,    30,   112,    -1,    30,   147,
     112,    -1,    33,   112,    -1,    33,   147,   112,    -1,     8,
     123,   112,    -1,    23,   113,   124,   114,   112,    -1,    16,
     113,   147,    20,   143,   114,   134,    -1,    16,   113,   147,
      20,   143,    39,   143,   114,   134,    -1,    16,   113,   147,
      20,   143,    39,    66,   143,   114,   134,    -1,    18,   113,
     122,   114,   121,    -1,    18,   113,   122,   114,    61,   119,
      19,    -1,   112,    -1,     6,    48,   142,    -1,   122,    43,
       6,    48,   142,    -1,   147,    -1,   123,    43,   147,    -1,
     143,    -1,   124,    43,   143,    -1,   126,    -1,   125,    43,
     126,    -1,     7,    -1,   128,    -1,   127,    43,   128,    -1,
       7,    -1,     7,    48,   142,    -1,    -1,    32,     6,   130,
     113,   131,   114,   110,   119,   111,    -1,    32,    66,     6,
     113,   131,   114,   110,   119,   111,    -1,   132,     7,    -1,
     132,    66,     7,    -1,   131,    43,   132,     7,    -1,   131,
      43,   132,    66,     7,    -1,    -1,    -1,     6,    -1,   121,
      -1,    61,   119,    15,   112,    -1,   121,    -1,    61,   119,
      17,   112,    -1,   123,    -1,    -1,   136,    99,   113,   147,
     114,   121,    -1,    -1,    -1,   100,   121,    -1,   121,    -1,
      61,   119,    13,   112,    -1,   110,   140,   111,    -1,   110,
     112,   140,   111,    -1,    61,   140,    27,   112,    -1,    61,
     112,   140,    27,   112,    -1,    -1,   140,    28,   147,   141,
     119,    -1,   140,    29,   141,   119,    -1,    61,    -1,   112,
      -1,     3,    -1,     4,    -1,     5,    -1,     6,    -1,   144,
      -1,     6,    42,     6,    -1,   144,    38,     6,    -1,     7,
      -1,   144,    96,   115,    -1,   144,    96,   147,   115,    -1,
     144,   110,   147,   111,    -1,   116,   110,   147,   111,    -1,
       6,   113,   146,   114,    -1,   144,    42,     6,   113,   146,
     114,    -1,   144,    38,     6,   113,   146,   114,    -1,   147,
      -1,    66,   143,    -1,   146,    43,   147,    -1,   146,    43,
      66,   143,    -1,    -1,    40,   113,   149,   114,    48,   147,
      -1,   143,    -1,   143,    48,   147,    -1,   145,    -1,   143,
      48,    66,   143,    -1,   143,    59,   147,    -1,   143,    58,
     147,    -1,   143,    57,   147,    -1,   143,    56,   147,    -1,
     143,    55,   147,    -1,   143,    54,   147,    -1,   143,    53,
     147,    -1,   143,    52,   147,    -1,   143,    51,   147,    -1,
     143,    50,   147,    -1,   143,    49,   147,    -1,   143,    95,
      -1,    95,   143,    -1,   143,    94,    -1,    94,   143,    -1,
     147,    62,   147,    -1,   147,    63,   147,    -1,   147,    44,
     147,    -1,   147,    46,   147,    -1,   147,    45,   147,    -1,
     147,    64,   147,    -1,   147,    66,   147,    -1,   147,    65,
     147,    -1,   147,    79,   147,    -1,   147,    77,   147,    -1,
     147,    78,   147,    -1,   147,    80,   147,    -1,   147,    81,
     147,    -1,   147,    82,   147,    -1,   147,    76,   147,    -1,
     147,    75,   147,    -1,    77,   147,    -1,    78,   147,    -1,
      83,   147,    -1,    85,   147,    -1,   147,    68,   147,    -1,
     147,    67,   147,    -1,   147,    70,   147,    -1,   147,    69,
     147,    -1,   147,    71,   147,    -1,   147,    74,   147,    -1,
     147,    72,   147,    -1,   147,    73,   147,    -1,   113,   147,
     114,    -1,   147,    60,   147,    61,   147,    -1,    93,   147,
      -1,    92,   147,    -1,    91,   147,    -1,    90,   147,    -1,
      89,   147,    -1,    88,   147,    -1,    87,   147,    -1,     9,
     148,    -1,    86,   147,    -1,   142,    -1,    47,   147,    -1,
     113,   147,   114,    -1,   113,   114,    -1,    -1,   150,    -1,
     149,    43,   150,    -1,   143,    -1,    40,   113,   149,   114,
      -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   141,   141,   145,   146,   151,   152,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   177,   180,   183,   184,   185,
     188,   189,   192,   193,   202,   203,   209,   210,   214,   228,
     229,   232,   233,   241,   241,   252,   256,   257,   258,   259,
     260,   264,   265,   269,   270,   274,   275,   278,   279,   283,
     284,   288,   289,   292,   293,   297,   298,   299,   300,   304,
     305,   306,   309,   310,   314,   315,   316,   317,   320,   321,
     322,   326,   327,   328,   329,   330,   334,   335,   336,   339,
     340,   341,   342,   343,   348,   349,   350,   351,   352,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     369,   370,   371,   372,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   414,   416,   419,   420,   421,   424,   425,
     429,   430,   431
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
  "'!'", "INSTANCEOF", "'~'", "'@'", "UNSET_CAST", "BOOL_CAST",
  "OBJECT_CAST", "ARRAY_CAST", "STRING_CAST", "DOUBLE_CAST", "INT_CAST",
  "DEC", "INC", "'['", "CLONE", "NEW", "ELSEIF", "ELSE", "ENDIF", "PUBLIC",
  "PROTECTED", "PRIVATE", "FINAL", "ABSTRACT", "STATIC", "START_SCRIPT",
  "END_SCRIPT", "'{'", "'}'", "';'", "'('", "')'", "']'", "'$'", "$accept",
  "program_tree", "top_statement_list", "top_statement", "statement",
  "decl_list", "expr_list", "variable_list", "global_var_list",
  "global_var", "static_var_list", "static_var", "function_decl_statement",
  "@1", "parameter_list", "optional_class_type", "for_statement",
  "foreach_statement", "for_expr", "elseif_list", "else_statement",
  "while_statement", "switch_case_list", "case_list", "case_separator",
  "const_value", "variable", "deref_variable", "function_call",
  "func_param_list", "expr", "exit_expr", "assignment_list",
  "assignment_list_element", 0
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
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     123,   125,    59,    40,    41,    93,    36
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   117,   118,   119,   119,   120,   120,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     122,   122,   123,   123,   124,   124,   125,   125,   126,   127,
     127,   128,   128,   130,   129,   129,   131,   131,   131,   131,
     131,   132,   132,   133,   133,   134,   134,   135,   135,   136,
     136,   137,   137,   138,   138,   139,   139,   139,   139,   140,
     140,   140,   141,   141,   142,   142,   142,   142,   143,   143,
     143,   144,   144,   144,   144,   144,   145,   145,   145,   146,
     146,   146,   146,   146,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   148,   148,   148,   149,   149,
     150,   150,   150
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     2,     0,     1,     1,     3,     2,     3,
       3,     7,     5,     7,     9,     5,     2,     3,     2,     3,
       2,     3,     3,     5,     7,     9,    10,     5,     7,     1,
       3,     5,     1,     3,     1,     3,     1,     3,     1,     1,
       3,     1,     3,     0,     9,     9,     2,     3,     4,     5,
       0,     0,     1,     1,     4,     1,     4,     1,     0,     6,
       0,     0,     2,     1,     4,     3,     4,     4,     5,     0,
       5,     4,     1,     1,     1,     1,     1,     1,     1,     3,
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
       0,     4,     0,     0,     1,    74,    75,    76,    77,    81,
       0,   157,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     2,     4,    29,     0,     0,     3,     5,     6,
     153,    95,    78,    97,     0,     0,    93,     0,    32,     0,
     151,     0,     0,     0,    58,     0,     0,    38,     0,    36,
       0,     0,    18,     0,    16,     0,    43,     0,    20,     0,
     162,   154,   130,   131,   132,   133,   152,   150,   149,   148,
     147,   146,   145,   144,     0,   113,    78,   111,    41,     0,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,   110,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,    79,     0,
       0,    89,     0,    22,   156,     0,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     9,     0,    34,     0,    19,
      17,     0,     0,    21,     0,   160,     0,   158,     0,     0,
       0,    10,     7,   142,     0,     0,    96,   109,   108,   107,
     106,   105,   104,   103,   102,   101,   100,    99,    80,     0,
      82,     0,     0,   116,   118,   117,     0,   114,   115,   119,
     121,   120,   135,   134,   137,   136,   138,   140,   141,   139,
     129,   128,   123,   124,   122,   125,   126,   127,    90,     0,
      86,    33,   155,     0,     0,     0,    58,     0,     0,     0,
       0,    37,     0,     0,     0,    50,    50,   162,   162,     0,
      80,    77,    42,    40,    85,    98,    93,    93,    83,    84,
       0,     0,    91,    60,     0,     4,    63,    12,     0,     0,
      30,     0,     4,    27,    35,    23,    69,    69,    15,    52,
       0,     0,     0,     0,   159,     0,     0,     0,   143,    92,
      61,     0,     0,    58,     0,     0,     0,     0,    69,     0,
      69,     0,    51,     0,    46,     0,     0,   161,    94,    88,
      87,     0,     0,    11,    13,     0,     0,     0,     0,     4,
      55,    24,    31,    28,     0,     0,     0,     0,     0,    65,
       0,     4,    47,     4,     0,    62,    64,     0,     0,     0,
       0,     0,    67,     0,    72,    73,     4,    66,    48,     0,
       0,     0,     0,     4,    53,    14,     0,    25,     0,    68,
       4,    71,    49,    44,    45,     0,     0,    26,    56,    70,
      59,     0,    54
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     2,     3,    47,    48,   163,   159,   166,    68,    69,
      99,   100,    49,   171,   280,   281,   355,   321,   160,   290,
     313,   267,   278,   299,   346,    50,    51,    52,    53,   150,
      54,    60,   176,   177
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -294
static const short int yypact[] =
{
     -54,  -294,    56,   351,  -294,  -294,  -294,  -294,   -33,  -294,
    2345,   -53,   -49,  1833,   -26,    39,    55,    59,    65,    62,
      68,  1889,  1947,    13,  2003,    69,  2345,  2345,  2345,  2345,
    2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,    31,
      31,   177,  -294,  -294,  -294,  2345,   111,  -294,  -294,  -294,
    -294,   215,   -28,  -294,  2400,   219,  2061,     0,  2795,  2117,
    -294,  2345,   214,  2345,  2345,  2345,   223,  -294,    46,  -294,
      31,  2345,  -294,  2451,  -294,  2502,  -294,   233,  -294,  2553,
      29,  2678,   152,   152,  -294,  -294,  -294,  -294,  -294,  -294,
    -294,  -294,  -294,  -294,   199,  -294,   -35,  -294,   194,    48,
    -294,   465,   455,  2345,  2175,  2345,  2345,  2345,  2345,  2345,
    2345,  2345,  2345,  2345,  2345,  2345,  -294,  -294,   238,   239,
    2231,  2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,
    2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,  2345,
    2345,  2345,  2345,  2345,  2345,  2345,  2345,  -294,  -294,    31,
     -31,  2795,  2345,  -294,  -294,   569,   683,   133,   797,   204,
     141,  2717,   207,   -30,    65,  -294,    -1,  -294,   911,  -294,
    -294,   143,   144,  -294,   145,  -294,    27,  -294,   253,   174,
     177,  -294,  -294,  -294,  2604,    31,  2678,  2678,  2678,  2678,
    2678,  2678,  2678,  2678,  2678,  2678,  2678,  2678,   147,   148,
    -294,   125,  2654,  2833,  2870,  2678,  2756,  2890,  2909,  2927,
    2944,  2339,  2960,  2960,  2960,  2960,   137,   137,   137,   137,
      85,    85,   152,   152,   152,  -294,  -294,  -294,  -294,  2289,
    -294,  2795,  -294,  1833,  2345,  1263,  2345,    31,   174,   269,
    1377,  -294,    31,   164,    49,    88,    88,    29,    29,   229,
    -294,  -294,  -294,  -294,  -294,  -294,  2061,  2061,  -294,  -294,
    2345,    31,  2795,  -294,  1025,  -294,  -294,  -294,   168,   -38,
    -294,   235,  -294,  -294,  -294,  -294,   169,   172,  -294,  -294,
      30,    26,    34,    35,  -294,  2345,    42,    43,   337,  -294,
     -55,   173,   579,  2345,    45,  1491,   174,   693,  -294,    20,
    -294,   -23,   280,   179,  -294,   284,   183,  -294,  2678,  -294,
    -294,   181,  1833,  -294,  -294,   184,   186,    31,   187,  -294,
    -294,  -294,  -294,  -294,   208,   185,  2345,   -50,   -21,  -294,
      89,  -294,  -294,  -294,  2345,  -294,  -294,  1605,   188,  1491,
     807,   191,  -294,  1709,  -294,  -294,  -294,  -294,  -294,   291,
     921,  1035,  1139,  -294,  -294,  -294,  1491,  -294,   192,  -294,
    -294,  1719,  -294,  -294,  -294,  1833,  1149,  -294,  -294,  1719,
    -294,   195,  -294
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -294,  -294,   -41,  -294,   -13,  -294,   296,  -294,  -294,   149,
    -294,   128,  -294,  -294,    70,     9,  -294,  -293,  -219,  -294,
    -294,  -294,  -294,  -124,   -29,  -145,     1,   103,  -294,  -190,
      -6,  -294,    74,    67
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -52
static const short int yytable[] =
{
      62,   294,   101,   178,    58,   326,   327,   326,   327,    55,
     118,   344,   229,   239,   119,    73,    75,   268,    79,    76,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,   304,   252,    94,     9,    94,     9,   102,
      95,    97,   242,   152,   311,   312,   357,   325,   326,   327,
     151,    94,     9,   155,     1,   156,     4,   158,    58,   161,
      59,   120,   345,   367,    61,   168,   286,   287,   120,   174,
     248,   167,    67,   302,   316,   121,   295,   302,   248,    77,
      56,   175,   121,   230,   240,   229,   229,    63,   329,   164,
     347,   180,   305,   270,   279,   -51,   348,   184,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     276,   317,   153,   243,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   249,    96,    96,   303,    46,   231,    46,   306,   307,
     228,   322,    64,   301,   -51,   349,   309,   310,   165,   277,
     181,    46,   141,   142,   143,   144,   145,   146,    65,   122,
     123,   124,    66,    96,   324,    70,   328,     5,     6,     7,
     251,    71,    80,    96,    98,   125,   255,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   -52,   -52,
     -52,   -52,   139,   140,   141,   142,   143,   144,   145,   146,
     263,   103,   266,   262,   292,   148,   157,   273,   264,   162,
      58,   297,   144,   145,   146,   341,   326,   327,   269,   172,
     258,    55,   179,   274,   198,   199,   234,   152,   175,   175,
     151,   151,    96,   236,   288,   238,   245,   246,   247,   250,
     256,   257,   289,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   271,   275,   285,   340,   308,
     293,   298,   320,   296,   300,   314,   279,    58,    96,   331,
     350,   332,   351,   333,   334,   318,   336,   342,   362,   335,
     337,   339,   356,   359,   368,   361,    57,   372,   253,   116,
     117,   330,   366,   241,   360,   284,   282,     0,   338,   369,
     343,   283,     0,     0,   354,     0,   320,     0,   352,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      96,     0,     0,   320,     0,    96,     0,     0,     0,     0,
      96,    96,   370,     0,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    96,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,    23,    24,     0,     0,     0,     0,     0,
       0,    25,     0,     0,     0,     0,     0,    96,    26,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
      96,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    41,     0,
      42,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,    23,    24,   122,
     123,   124,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,   125,     0,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,   183,
       0,     0,    41,     0,     0,    43,   182,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,   315,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,   122,   123,   124,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,   125,
       0,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,   232,     0,     0,    41,     0,     0,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,   323,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,    23,    24,   122,   123,   124,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,   125,     0,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,     0,     0,     0,     0,     0,   233,     0,     0,
      41,     0,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,   358,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,   122,   123,   124,     0,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,   125,     0,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,   235,     0,     0,    41,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,    23,    24,   122,   123,   124,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,   125,     0,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,     0,
       0,     0,     0,     0,     0,   244,     0,     0,    41,     0,
       0,    43,   363,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,    23,    24,   122,
     123,   124,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,   125,     0,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,   291,
       0,     0,    41,     0,     0,    43,   364,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,   371,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,   122,   123,   124,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,   125,
       0,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,   365,     0,     0,    41,     0,     0,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,     0,    24,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   265,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      41,     0,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,     0,
      24,     0,     0,     0,     0,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   272,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    41,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,     0,    24,     0,     0,     0,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   319,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    41,     0,
       0,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,     0,    24,     0,
       0,     0,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   353,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    41,     0,     0,    43,     0,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,   122,   123,   124,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,   125,
     344,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,   345,     0,     0,     0,     0,    41,     0,     0,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,     0,    24,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     5,     6,     7,     8,     9,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
      41,     0,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,     0,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,     0,     0,     0,
       0,    72,    45,     0,     0,    46,     5,     6,     7,     8,
       9,     0,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,    74,
      45,     0,     0,    46,     5,     6,     7,     8,     9,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,    78,    45,     0,     0,    46,
       5,     6,     7,     8,     9,     0,    11,   149,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    45,     0,     0,    46,     5,     6,
       7,     8,     9,     0,    11,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,     0,     0,     0,
      45,   154,     0,    46,     5,     6,     7,     8,     9,     0,
      11,   185,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,     0,
       0,    46,     5,     6,     7,     8,     9,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
       0,     0,     0,     0,    45,     0,   200,    46,     5,     6,
       7,     8,     9,     0,    11,   261,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,     0,     0,    46,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,   122,   123,   124,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,     0,
     125,    46,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   122,   123,   124,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   125,   147,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   122,   123,   124,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   125,   169,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   122,   123,   124,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   125,   170,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   122,   123,
     124,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   125,   173,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   122,   123,
     124,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   125,   254,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   237,   125,     0,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   122,   123,   124,     0,   259,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   125,     0,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     122,   123,   124,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   125,   260,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   122,
     123,   124,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   125,     0,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   123,   124,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   125,     0,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   124,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     125,     0,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   -52,   -52,   -52,
     -52,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146
};

static const short int yycheck[] =
{
      13,    39,    43,    38,    10,    28,    29,    28,    29,    42,
      38,    61,    43,    43,    42,    21,    22,   236,    24,     6,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,     7,   179,     6,     7,     6,     7,    45,
      39,    40,    43,    43,    99,   100,   339,    27,    28,    29,
      56,     6,     7,    59,   108,    61,     0,    63,    64,    65,
     113,    96,   112,   356,   113,    71,   256,   257,    96,    40,
      43,    70,     7,    43,   293,   110,   114,    43,    43,    66,
     113,    80,   110,   114,   114,    43,    43,   113,   111,    43,
     111,    43,    66,   238,     6,     7,     7,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
      61,    66,   112,   114,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   114,    39,    40,   114,   116,   152,   116,   114,   114,
     149,   296,   113,   277,    66,    66,   114,   114,   112,   110,
     112,   116,    77,    78,    79,    80,    81,    82,   113,    44,
      45,    46,   113,    70,   298,   113,   300,     3,     4,     5,
       6,   113,   113,    80,     7,    60,   185,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
     233,   110,   235,   229,   265,     6,    12,   240,   234,     6,
     236,   272,    80,    81,    82,    27,    28,    29,   237,     6,
     115,    42,    48,   242,     6,     6,   113,    43,   247,   248,
     256,   257,   149,   112,   260,    48,   113,   113,   113,     6,
     113,   113,   261,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,     6,   112,    48,   319,   285,
     112,   112,   295,    48,   112,   112,     6,   293,   185,   110,
     331,     7,   333,   110,   113,   294,   112,   112,     7,   312,
     114,   114,   114,   112,   112,   346,    10,   112,   180,    94,
      95,   302,   353,   164,   343,   248,   246,    -1,   317,   360,
     326,   247,    -1,    -1,   337,    -1,   339,    -1,   334,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     237,    -1,    -1,   356,    -1,   242,    -1,    -1,    -1,    -1,
     247,   248,   365,    -1,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,   261,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,   294,    47,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
     317,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   107,    -1,
     109,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,    44,
      45,    46,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   107,    -1,    -1,   110,   111,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   107,    -1,    -1,   110,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    19,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    44,    45,    46,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     107,    -1,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    17,    18,    -1,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    44,    45,    46,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,    -1,   107,    -1,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    44,    45,    46,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   107,    -1,
      -1,   110,   111,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,    44,
      45,    46,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   107,    -1,    -1,   110,   111,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    15,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   107,    -1,    -1,   110,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     107,    -1,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
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
      -1,    -1,    -1,    -1,   107,    -1,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   107,    -1,
      -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
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
      -1,    -1,   107,    -1,    -1,   110,    -1,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,   107,    -1,    -1,   110,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
     107,    -1,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,    -1,     9,    66,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,   114,    -1,   116,     3,     4,     5,     6,     7,    -1,
       9,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   113,    -1,   115,   116,     3,     4,
       5,     6,     7,    -1,     9,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,    -1,   116,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,
      60,   116,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,   112,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    60,   112,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    60,   112,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,   112,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,   111,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    20,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    44,    45,    46,    -1,   111,    -1,    -1,    -1,    -1,
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
      80,    81,    82,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   108,   118,   119,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    14,    16,    18,    22,    23,
      26,    30,    31,    32,    33,    40,    47,    77,    78,    83,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,   107,   109,   110,   112,   113,   116,   120,   121,   129,
     142,   143,   144,   145,   147,    42,   113,   123,   147,   113,
     148,   113,   121,   113,   113,   113,   113,     7,   125,   126,
     113,   113,   112,   147,   112,   147,     6,    66,   112,   147,
     113,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,     6,   143,   144,   143,     7,   127,
     128,   119,   147,   110,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    94,    95,    38,    42,
      96,   110,    44,    45,    46,    60,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   112,     6,    66,
     146,   147,    43,   112,   114,   147,   147,    12,   147,   123,
     135,   147,     6,   122,    43,   112,   124,   143,   147,   112,
     112,   130,     6,   112,    40,   143,   149,   150,    38,    48,
      43,   112,   111,   114,   147,    66,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,     6,     6,
     115,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   143,    43,
     114,   147,   114,   114,   113,   114,   112,    20,    48,    43,
     114,   126,    43,   114,   114,   113,   113,   113,    43,   114,
       6,     6,   142,   128,   111,   143,   113,   113,   115,   111,
      61,    66,   147,   121,   147,    61,   121,   138,   135,   143,
     142,     6,    61,   121,   143,   112,    61,   110,   139,     6,
     131,   132,   131,   149,   150,    48,   146,   146,   147,   143,
     136,   114,   119,   112,    39,   114,    48,   119,   112,   140,
     112,   140,    43,   114,     7,    66,   114,   114,   147,   114,
     114,    99,   100,   137,   112,    13,   135,    66,   143,    61,
     121,   134,   142,    19,   140,    27,    28,    29,   140,   111,
     132,   110,     7,   110,   113,   121,   112,   114,   143,   114,
     119,    27,   112,   147,    61,   112,   141,   111,     7,    66,
     119,   119,   147,    61,   121,   133,   114,   134,    17,   112,
     141,   119,     7,   111,   111,   114,   119,   134,   112,   119,
     121,    15,   112
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
#line 141 "php_parser.y"
    { g_syn_tree_top = (yyvsp[-1].syn_node); ;}
    break;

  case 3:
#line 145 "php_parser.y"
    { (yyval.syn_node) = add_statement_2_list((yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 4:
#line 146 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 7:
#line 157 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 8:
#line 158 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_EXPR, (yyvsp[-1].exp_node)); ;}
    break;

  case 9:
#line 159 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 10:
#line 160 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 11:
#line 161 "php_parser.y"
    { (yyval.syn_node) = make_ifelse_syn_node((yyvsp[-4].exp_node), (yyvsp[-2].syn_node), (yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 12:
#line 162 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 1); ;}
    break;

  case 13:
#line 163 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[-5].syn_node), 0); ;}
    break;

  case 14:
#line 164 "php_parser.y"
    { (yyval.syn_node) = make_for_syn_node((yyvsp[-6].exp_node), (yyvsp[-4].exp_node), (yyvsp[-2].exp_node), (yyvsp[0].syn_node)); ;}
    break;

  case 15:
#line 165 "php_parser.y"
    { ;}
    break;

  case 16:
#line 166 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, 0); ;}
    break;

  case 17:
#line 167 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, (yyvsp[-1].exp_node)); ;}
    break;

  case 18:
#line 168 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, 0); ;}
    break;

  case 19:
#line 169 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, (yyvsp[-1].exp_node)); ;}
    break;

  case 20:
#line 170 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, 0); ;}
    break;

  case 21:
#line 171 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, (yyvsp[-1].exp_node)); ;}
    break;

  case 22:
#line 172 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_ECHO, (yyvsp[-1].exp_node)); ;}
    break;

  case 23:
#line 173 "php_parser.y"
    {  ;}
    break;

  case 24:
#line 174 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-4].exp_node), 0, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 0);
			;}
    break;

  case 25:
#line 177 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-6].exp_node), (yyvsp[-4].exp_node)->var_node, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 0);
			;}
    break;

  case 26:
#line 180 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-7].exp_node), (yyvsp[-5].exp_node)->var_node, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 1);
			;}
    break;

  case 27:
#line 183 "php_parser.y"
    { ;}
    break;

  case 28:
#line 184 "php_parser.y"
    { ;}
    break;

  case 29:
#line 185 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 30:
#line 188 "php_parser.y"
    {  ;}
    break;

  case 31:
#line 189 "php_parser.y"
    {  ;}
    break;

  case 32:
#line 192 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0); (yyval.exp_node)->exp_node = (yyvsp[0].exp_node); ;}
    break;

  case 33:
#line 193 "php_parser.y"
    {
				PHP_EXP_NODE *last = (yyvsp[-2].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[0].exp_node);
				(yyval.exp_node) = (yyvsp[-2].exp_node);
			;}
    break;

  case 36:
#line 209 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 37:
#line 210 "php_parser.y"
    {  ;}
    break;

  case 38:
#line 214 "php_parser.y"
    {
		const char *varname = get_scope_var_name(g_current_scope, (yyvsp[0].exp_node)->var_node);
		PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, varname);
		PHP_SCOPE_ITEM *gsi = get_scope_item(g_global_scope, varname);
		if ( gsi && (gsi->type == PHP_SCOPE_VAR) ) {
			free_var_node(si->var);
			gsi->var->ref_count++;
			si->var = gsi->var;
		} else {
			php_report_error(PHP_ERROR, "There is no such global var");
		}
	;}
    break;

  case 39:
#line 228 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 40:
#line 229 "php_parser.y"
    {  ;}
    break;

  case 41:
#line 232 "php_parser.y"
    { (yyvsp[0].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 42:
#line 233 "php_parser.y"
    {
			(yyvsp[-2].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[-2].exp_node);
			value_value_assign(&(yyvsp[-2].exp_node)->var_node->value, &(yyvsp[0].exp_node)->val_node);
		;}
    break;

  case 43:
#line 241 "php_parser.y"
    {
				switch_push_scope_table(make_scope_table())
			;}
    break;

  case 44:
#line 243 "php_parser.y"
    {
				(yyval.syn_node) = make_func_decl_syn_node((yyvsp[-7].str_val), (yyvsp[-4].exp_node));
				(yyval.syn_node)->func_decl->scope = g_current_scope;
				(yyval.syn_node)->func_decl->is_native = 0;
				(yyval.syn_node)->func_decl->code = (yyvsp[-1].syn_node);
				switch_pop_scope_table(0);
				add_func_2_scope(g_current_scope, (yyval.syn_node));
				(yyval.syn_node) = 0;
			;}
    break;

  case 45:
#line 252 "php_parser.y"
    {  ;}
    break;

  case 46:
#line 256 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node), (yyvsp[-1].str_val), 0); ;}
    break;

  case 47:
#line 257 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node), (yyvsp[-2].str_val), 1); ;}
    break;

  case 48:
#line 258 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[-3].exp_node), (yyvsp[0].exp_node), (yyvsp[-1].str_val), 0); ;}
    break;

  case 49:
#line 259 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[-4].exp_node), (yyvsp[0].exp_node), (yyvsp[-2].str_val), 1); ;}
    break;

  case 50:
#line 260 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 51:
#line 264 "php_parser.y"
    { (yyval.str_val)[0] = 0; ;}
    break;

  case 54:
#line 270 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 56:
#line 275 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 58:
#line 279 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 59:
#line 283 "php_parser.y"
    { (yyval.syn_node) = add_branch_2_elseif((yyvsp[-5].syn_node), make_ifelse_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0, 0)); ;}
    break;

  case 60:
#line 284 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 61:
#line 288 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 62:
#line 289 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[0].syn_node); ;}
    break;

  case 64:
#line 293 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 65:
#line 297 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 66:
#line 298 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 67:
#line 299 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 68:
#line 300 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 69:
#line 304 "php_parser.y"
    {  (yyval.syn_node) = 0; ;}
    break;

  case 70:
#line 305 "php_parser.y"
    {  ;}
    break;

  case 71:
#line 306 "php_parser.y"
    {  ;}
    break;

  case 77:
#line 317 "php_parser.y"
    { (yyval.exp_node) = make_known_const((yyvsp[0].str_val)); ;}
    break;

  case 79:
#line 321 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str((yyvsp[-2].str_val)), make_const_exp_str((yyvsp[0].str_val))); ;}
    break;

  case 80:
#line 322 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OBJECT_DEREF, (yyvsp[-2].exp_node), make_const_exp_str((yyvsp[0].str_val))); ;}
    break;

  case 82:
#line 327 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_BY_KEY, 0); ;}
    break;

  case 83:
#line 328 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 84:
#line 329 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 85:
#line 330 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[-1].exp_node)); ;}
    break;

  case 86:
#line 334 "php_parser.y"
    { (yyval.exp_node) = make_func_call_exp((yyvsp[-3].str_val), (yyvsp[-1].exp_node)); ;}
    break;

  case 87:
#line 335 "php_parser.y"
    { ;}
    break;

  case 88:
#line 336 "php_parser.y"
    { ;}
    break;

  case 89:
#line 339 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 90:
#line 340 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 91:
#line 341 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-2].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 92:
#line 342 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-3].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 93:
#line 343 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); ;}
    break;

  case 94:
#line 348 "php_parser.y"
    { ;}
    break;

  case 96:
#line 350 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ASS, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 97:
#line 351 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 98:
#line 352 "php_parser.y"
    {  ;}
    break;

  case 99:
#line 357 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 100:
#line 358 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 101:
#line 359 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 102:
#line 360 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 103:
#line 361 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 104:
#line 362 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 105:
#line 363 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 106:
#line 364 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 107:
#line 365 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 108:
#line 366 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 109:
#line 367 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 110:
#line 369 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 111:
#line 370 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 112:
#line 371 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 113:
#line 372 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 114:
#line 374 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 115:
#line 375 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 116:
#line 376 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 117:
#line 377 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 118:
#line 378 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 119:
#line 379 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 120:
#line 380 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 121:
#line 381 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 122:
#line 382 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 123:
#line 383 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 124:
#line 384 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 125:
#line 385 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 126:
#line 386 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 127:
#line 387 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 128:
#line 388 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 129:
#line 389 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 130:
#line 390 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 131:
#line 391 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), (yyvsp[0].exp_node)); ;}
    break;

  case 132:
#line 392 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_LOG_NOT, (yyvsp[0].exp_node)); ;}
    break;

  case 133:
#line 393 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_NOT, (yyvsp[0].exp_node)); ;}
    break;

  case 134:
#line 394 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 135:
#line 395 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NOT_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 136:
#line 396 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 137:
#line 397 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NEQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 138:
#line 398 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 139:
#line 399 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 140:
#line 400 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 141:
#line 401 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 142:
#line 402 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 143:
#line 403 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUX, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); (yyval.exp_node)->exp_node = (yyvsp[-4].exp_node); ;}
    break;

  case 144:
#line 404 "php_parser.y"
    {  ;}
    break;

  case 145:
#line 405 "php_parser.y"
    {  ;}
    break;

  case 146:
#line 406 "php_parser.y"
    {  ;}
    break;

  case 147:
#line 407 "php_parser.y"
    {  ;}
    break;

  case 148:
#line 408 "php_parser.y"
    {  ;}
    break;

  case 149:
#line 409 "php_parser.y"
    {  ;}
    break;

  case 150:
#line 410 "php_parser.y"
    {  ;}
    break;

  case 151:
#line 411 "php_parser.y"
    {  ;}
    break;

  case 152:
#line 412 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 153:
#line 414 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 154:
#line 416 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_PRINT, (yyvsp[0].exp_node)); ;}
    break;

  case 155:
#line 419 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 156:
#line 420 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 157:
#line 421 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 160:
#line 429 "php_parser.y"
    { /*$$ = make_assign_node($1);*/ ;}
    break;

  case 161:
#line 430 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 162:
#line 431 "php_parser.y"
    { /*$$ = make_assign_node(0);*/ ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 2876 "php_parser.c"

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



