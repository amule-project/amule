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

// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
#line 66 "php_parser.y"
typedef union YYSTYPE {
	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 328 "php_parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 340 "php_parser.c"

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
#define YYLAST   2990

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  117
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  33
/* YYNRULES -- Number of rules. */
#define YYNRULES  162
/* YYNRULES -- Number of states. */
#define YYNSTATES  374

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
     144,   148,   154,   156,   160,   162,   166,   170,   176,   178,
     182,   184,   188,   190,   191,   201,   211,   214,   218,   223,
     229,   230,   231,   233,   235,   240,   242,   247,   249,   250,
     257,   258,   259,   262,   264,   269,   273,   278,   283,   289,
     290,   296,   301,   303,   305,   307,   309,   311,   313,   315,
     319,   323,   325,   329,   334,   339,   344,   349,   356,   363,
     365,   368,   372,   377,   378,   385,   387,   391,   393,   398,
     402,   406,   410,   414,   418,   422,   426,   430,   434,   438,
     442,   445,   448,   451,   454,   458,   462,   466,   470,   474,
     478,   482,   486,   490,   494,   498,   502,   506,   510,   514,
     518,   521,   524,   527,   530,   534,   538,   542,   546,   550,
     554,   558,   562,   566,   572,   575,   578,   581,   584,   587,
     590,   593,   596,   599,   601,   604,   608,   611,   612,   614,
     618,   620,   625
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     118,     0,    -1,   108,   119,   109,    -1,   119,   120,    -1,
      -1,   121,    -1,   128,    -1,   110,   119,   111,    -1,   146,
     112,    -1,    22,   126,   112,    -1,   107,   125,   112,    -1,
      10,   113,   146,   114,   121,   135,   136,    -1,    12,   113,
     146,   114,   137,    -1,    11,   121,    12,   113,   146,   114,
     112,    -1,    14,   113,   134,   112,   134,   112,   134,   114,
     132,    -1,    26,   113,   146,   114,   138,    -1,    31,   112,
      -1,    31,   146,   112,    -1,    30,   112,    -1,    30,   146,
     112,    -1,    33,   112,    -1,    33,   146,   112,    -1,     8,
     123,   112,    -1,    23,   113,   124,   114,   112,    -1,    16,
     113,   146,    20,   142,   114,   133,    -1,    16,   113,   146,
      20,   142,    39,   142,   114,   133,    -1,    16,   113,   146,
      20,   142,    39,    66,   142,   114,   133,    -1,    18,   113,
     122,   114,   121,    -1,    18,   113,   122,   114,    61,   119,
      19,    -1,   112,    -1,     6,    48,   141,    -1,   122,    43,
       6,    48,   141,    -1,   146,    -1,   123,    43,   146,    -1,
     142,    -1,   124,    43,   142,    -1,   125,    43,     7,    -1,
     125,    43,     7,    48,   141,    -1,     7,    -1,     7,    48,
     141,    -1,   127,    -1,   126,    43,   127,    -1,     7,    -1,
      -1,    32,     6,   129,   113,   130,   114,   110,   119,   111,
      -1,    32,    66,     6,   113,   130,   114,   110,   119,   111,
      -1,   131,     7,    -1,   131,    66,     7,    -1,   130,    43,
     131,     7,    -1,   130,    43,   131,    66,     7,    -1,    -1,
      -1,     6,    -1,   121,    -1,    61,   119,    15,   112,    -1,
     121,    -1,    61,   119,    17,   112,    -1,   123,    -1,    -1,
     135,    99,   113,   146,   114,   121,    -1,    -1,    -1,   100,
     121,    -1,   121,    -1,    61,   119,    13,   112,    -1,   110,
     139,   111,    -1,   110,   112,   139,   111,    -1,    61,   139,
      27,   112,    -1,    61,   112,   139,    27,   112,    -1,    -1,
     139,    28,   146,   140,   119,    -1,   139,    29,   140,   119,
      -1,    61,    -1,   112,    -1,     3,    -1,     4,    -1,     5,
      -1,     6,    -1,   143,    -1,     6,    42,     6,    -1,   143,
      38,     6,    -1,     7,    -1,   143,    96,   115,    -1,   143,
      96,   146,   115,    -1,   143,   110,   146,   111,    -1,   116,
     110,   146,   111,    -1,     6,   113,   145,   114,    -1,   143,
      42,     6,   113,   145,   114,    -1,   143,    38,     6,   113,
     145,   114,    -1,   146,    -1,    66,   142,    -1,   145,    43,
     146,    -1,   145,    43,    66,   142,    -1,    -1,    40,   113,
     148,   114,    48,   146,    -1,   142,    -1,   142,    48,   146,
      -1,   144,    -1,   142,    48,    66,   142,    -1,   142,    59,
     146,    -1,   142,    58,   146,    -1,   142,    57,   146,    -1,
     142,    56,   146,    -1,   142,    55,   146,    -1,   142,    54,
     146,    -1,   142,    53,   146,    -1,   142,    52,   146,    -1,
     142,    51,   146,    -1,   142,    50,   146,    -1,   142,    49,
     146,    -1,   142,    95,    -1,    95,   142,    -1,   142,    94,
      -1,    94,   142,    -1,   146,    62,   146,    -1,   146,    63,
     146,    -1,   146,    44,   146,    -1,   146,    46,   146,    -1,
     146,    45,   146,    -1,   146,    64,   146,    -1,   146,    66,
     146,    -1,   146,    65,   146,    -1,   146,    79,   146,    -1,
     146,    77,   146,    -1,   146,    78,   146,    -1,   146,    80,
     146,    -1,   146,    81,   146,    -1,   146,    82,   146,    -1,
     146,    76,   146,    -1,   146,    75,   146,    -1,    77,   146,
      -1,    78,   146,    -1,    83,   146,    -1,    85,   146,    -1,
     146,    68,   146,    -1,   146,    67,   146,    -1,   146,    70,
     146,    -1,   146,    69,   146,    -1,   146,    71,   146,    -1,
     146,    74,   146,    -1,   146,    72,   146,    -1,   146,    73,
     146,    -1,   113,   146,   114,    -1,   146,    60,   146,    61,
     146,    -1,    93,   146,    -1,    92,   146,    -1,    91,   146,
      -1,    90,   146,    -1,    89,   146,    -1,    88,   146,    -1,
      87,   146,    -1,     9,   147,    -1,    86,   146,    -1,   141,
      -1,    47,   146,    -1,   113,   146,   114,    -1,   113,   114,
      -1,    -1,   149,    -1,   148,    43,   149,    -1,   142,    -1,
      40,   113,   148,   114,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   143,   143,   147,   148,   153,   154,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   179,   182,   185,   186,   187,
     190,   191,   194,   195,   198,   199,   203,   204,   205,   206,
     209,   210,   214,   229,   229,   239,   243,   244,   245,   246,
     247,   251,   252,   256,   257,   261,   262,   265,   266,   270,
     271,   275,   276,   279,   280,   284,   285,   286,   287,   291,
     292,   293,   296,   297,   301,   302,   303,   304,   307,   308,
     309,   313,   314,   315,   316,   317,   321,   322,   323,   326,
     327,   328,   329,   330,   335,   336,   337,   338,   339,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     356,   357,   358,   359,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   401,   403,   406,   407,   408,   411,   412,
     416,   417,   418
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
  "decl_list", "expr_list", "variable_list", "static_var_list",
  "global_var_list", "global_var", "function_decl_statement", "@1",
  "parameter_list", "optional_class_type", "for_statement",
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
     122,   122,   123,   123,   124,   124,   125,   125,   125,   125,
     126,   126,   127,   129,   128,   128,   130,   130,   130,   130,
     130,   131,   131,   132,   132,   133,   133,   134,   134,   135,
     135,   136,   136,   137,   137,   138,   138,   138,   138,   139,
     139,   139,   140,   140,   141,   141,   141,   141,   142,   142,
     142,   143,   143,   143,   143,   143,   144,   144,   144,   145,
     145,   145,   145,   145,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   147,   147,   147,   148,   148,
     149,   149,   149
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     2,     0,     1,     1,     3,     2,     3,
       3,     7,     5,     7,     9,     5,     2,     3,     2,     3,
       2,     3,     3,     5,     7,     9,    10,     5,     7,     1,
       3,     5,     1,     3,     1,     3,     3,     5,     1,     3,
       1,     3,     1,     0,     9,     9,     2,     3,     4,     5,
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
     151,     0,     0,     0,    58,     0,     0,    42,     0,    40,
       0,     0,    18,     0,    16,     0,    43,     0,    20,     0,
     162,   154,   130,   131,   132,   133,   152,   150,   149,   148,
     147,   146,   145,   144,     0,   113,    78,   111,    38,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   112,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,    79,     0,     0,
      89,     0,    22,   156,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     9,     0,    34,     0,    19,    17,
       0,     0,    21,     0,   160,     0,   158,     0,     0,     0,
      10,     7,   142,     0,     0,    96,   109,   108,   107,   106,
     105,   104,   103,   102,   101,   100,    99,    80,     0,    82,
       0,     0,   116,   118,   117,     0,   114,   115,   119,   121,
     120,   135,   134,   137,   136,   138,   140,   141,   139,   129,
     128,   123,   124,   122,   125,   126,   127,    90,     0,    86,
      33,   155,     0,     0,     0,    58,     0,     0,     0,     0,
      41,     0,     0,     0,    50,    50,   162,   162,     0,    80,
      77,    39,    36,    85,    98,    93,    93,    83,    84,     0,
       0,    91,    60,     0,     4,    63,    12,     0,     0,    30,
       0,     4,    27,    35,    23,    69,    69,    15,    52,     0,
       0,     0,     0,   159,     0,     0,     0,     0,   143,    92,
      61,     0,     0,    58,     0,     0,     0,     0,    69,     0,
      69,     0,    51,     0,    46,     0,     0,   161,    94,    37,
      88,    87,     0,     0,    11,    13,     0,     0,     0,     0,
       4,    55,    24,    31,    28,     0,     0,     0,     0,     0,
      65,     0,     4,    47,     4,     0,    62,    64,     0,     0,
       0,     0,     0,    67,     0,    72,    73,     4,    66,    48,
       0,     0,     0,     0,     4,    53,    14,     0,    25,     0,
      68,     4,    71,    49,    44,    45,     0,     0,    26,    56,
      70,    59,     0,    54
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     2,     3,    47,    48,   162,   158,   165,    99,    68,
      69,    49,   170,   279,   280,   356,   322,   159,   290,   314,
     266,   277,   299,   347,    50,    51,    52,    53,   149,    54,
      60,   175,   176
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -296
static const short int yypact[] =
{
     -74,  -296,    49,   329,  -296,  -296,  -296,  -296,   -32,  -296,
    2323,   -69,   -59,  1811,   -57,   -53,   -49,   -46,    70,    55,
      66,  1867,  1925,    13,  1981,   107,  2323,  2323,  2323,  2323,
    2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,    36,
      36,   140,  -296,  -296,  -296,  2323,    33,  -296,  -296,  -296,
    -296,   213,   -25,  -296,  1687,   150,  2039,    30,  2722,  2095,
    -296,  2323,   147,  2323,  2323,  2323,   222,  -296,    37,  -296,
      36,  2323,  -296,  2378,  -296,  2429,  -296,   225,  -296,  2480,
      34,  2605,   -34,   -34,  -296,  -296,  -296,  -296,  -296,  -296,
    -296,  -296,  -296,  -296,   191,  -296,   -35,  -296,   187,    51,
     443,   319,  2323,  2153,  2323,  2323,  2323,  2323,  2323,  2323,
    2323,  2323,  2323,  2323,  2323,  -296,  -296,   232,   234,  2209,
    2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,
    2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,  2323,
    2323,  2323,  2323,  2323,  2323,  2323,  -296,  -296,    36,   -42,
    2722,  2323,  -296,  -296,   433,   547,   128,   661,   208,   142,
    2644,   204,   -31,    70,  -296,    27,  -296,   775,  -296,  -296,
     144,   146,  -296,   164,  -296,    39,  -296,   249,   270,   253,
    -296,  -296,  -296,  2531,    36,  2605,  2605,  2605,  2605,  2605,
    2605,  2605,  2605,  2605,  2605,  2605,  2605,   167,   168,  -296,
     121,  2581,  2760,  2797,  2605,  2683,  2838,  2857,  2875,  2892,
    2317,  2908,  2908,  2908,  2908,   136,   136,   136,   136,    96,
      96,   -34,   -34,   -34,  -296,  -296,  -296,  -296,  2267,  -296,
    2722,  -296,  1811,  2323,  1241,  2323,    36,   270,   280,  1355,
    -296,    36,   176,   -47,   103,   103,    34,    34,   241,  -296,
    -296,  -296,   246,  -296,  -296,  2039,  2039,  -296,  -296,  2323,
      36,  2722,  -296,   889,  -296,  -296,  -296,   183,   -30,  -296,
     248,  -296,  -296,  -296,  -296,   185,   186,  -296,  -296,    43,
      88,    44,    46,  -296,  2323,   270,    48,    50,  2818,  -296,
     125,   189,   557,  2323,    45,  1469,   270,   671,  -296,   219,
    -296,   -23,   293,   192,  -296,   296,   194,  -296,  2605,  -296,
    -296,  -296,   196,  1811,  -296,  -296,   193,   197,    36,   198,
    -296,  -296,  -296,  -296,  -296,   256,   203,  2323,    94,   -21,
    -296,   105,  -296,  -296,  -296,  2323,  -296,  -296,  1583,   205,
    1469,   785,   206,  -296,  1117,  -296,  -296,  -296,  -296,  -296,
     303,   899,  1013,  1003,  -296,  -296,  -296,  1469,  -296,   210,
    -296,  -296,  1697,  -296,  -296,  -296,  1811,  1127,  -296,  -296,
    1697,  -296,   211,  -296
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -296,  -296,   -41,  -296,   -13,  -296,   307,  -296,  -296,  -296,
     161,  -296,  -296,    81,    26,  -296,  -295,  -224,  -296,  -296,
    -296,  -296,  -128,   -14,  -145,    -4,    -2,  -296,   -51,    -6,
    -296,    85,    95
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -52
static const short int yytable[] =
{
      62,   228,   100,   177,    58,   327,   328,   327,   328,   294,
      55,   267,   238,   117,   275,    73,    75,   118,    79,    76,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,   251,     1,    95,    97,    96,    96,   101,
      94,     9,    94,     9,    59,   358,   143,   144,   145,     4,
     150,    94,     9,   154,    61,   155,    63,   157,    58,   160,
      64,   119,   368,   276,    65,   167,   166,    66,    96,   317,
     241,   119,   229,   151,   173,   120,   174,    67,    96,    77,
     163,    56,   247,   239,   295,   120,   302,   302,   330,   247,
     348,   228,   269,   228,   179,   304,   183,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   278,
     -51,   318,   349,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     309,   242,   152,   102,   227,   230,    96,    98,   301,   164,
      46,   323,    46,   248,   305,   345,   147,   303,   306,   156,
     307,    46,   310,   180,   311,   121,   122,   123,    70,   -51,
     325,   350,   329,   140,   141,   142,   143,   144,   145,    71,
     254,   124,    96,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   286,   287,   346,   -52,   -52,   -52,
     -52,   138,   139,   140,   141,   142,   143,   144,   145,   262,
      80,   265,   261,   292,   312,   313,   272,   263,   161,    58,
     297,   171,   268,    55,    96,   178,   257,   273,   197,    96,
     198,   233,   174,   174,    96,    96,   326,   327,   328,   150,
     150,   151,   237,   288,   235,   249,   289,   244,    96,   245,
     252,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,     5,     6,     7,   250,   246,   308,   341,
     255,   256,   321,   342,   327,   328,   270,    58,   274,   284,
     319,   351,    96,   352,   285,   293,   296,   298,   300,   278,
     336,   315,   332,   333,   334,   337,   362,   115,   116,   335,
     363,   338,   340,   367,   339,   343,    96,    57,   360,   357,
     370,   344,   369,   373,   240,   355,   281,   321,   331,   353,
     361,   282,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,   283,    15,   321,    16,     0,    17,     0,     0,
       0,    18,    19,   371,     0,    20,     0,     0,     0,    21,
      22,    23,    24,   121,   122,   123,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,   124,
       0,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,   182,     0,     0,    41,     0,    42,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,    23,    24,   121,   122,   123,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,   124,     0,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,     0,     0,     0,     0,     0,   231,     0,     0,
      41,     0,     0,    43,   181,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
     316,    15,     0,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,   121,   122,   123,     0,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,   124,     0,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,   232,     0,     0,    41,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
     324,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,    23,    24,   121,   122,   123,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,   124,     0,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,     0,
       0,     0,     0,     0,     0,   234,     0,     0,    41,     0,
       0,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,   359,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,    23,    24,   121,
     122,   123,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,   124,     0,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,   243,
       0,     0,    41,     0,     0,    43,     0,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,   121,   122,   123,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,   124,
       0,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,   291,     0,     0,    41,     0,     0,    43,
     364,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,    23,    24,   121,   122,   123,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,   124,     0,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,     0,     0,     0,     0,     0,   366,     0,     0,
      41,     0,     0,    43,   365,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,   372,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,   121,   122,   123,     0,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,   124,   345,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,   346,
       0,     0,     0,     0,    41,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,     0,    24,     0,     0,     0,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   264,     0,     0,     0,     0,     0,     0,     0,
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
       0,     0,     0,     0,     0,     0,   271,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    41,     0,     0,    43,     0,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,     0,    24,     0,     0,     0,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     320,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    41,     0,     0,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,     0,    24,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   354,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      41,     0,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,   121,   122,   123,     0,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,   124,     0,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,     0,     0,     0,     0,   146,
       0,     0,     0,     0,    41,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,     0,    24,     0,     0,     0,     0,     0,
       0,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       5,     6,     7,     8,     9,     0,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,     0,    41,     0,
       0,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,     0,    11,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,     0,     0,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,     0,     0,    72,
      45,     0,     0,    46,     5,     6,     7,     8,     9,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    25,     0,     0,     0,     0,     0,     0,    26,     0,
       0,     0,     0,     0,     0,     0,     0,    74,    45,     0,
       0,    46,     5,     6,     7,     8,     9,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,    29,     0,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,     0,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
       0,     0,     0,    78,    45,     0,     0,    46,     5,     6,
       7,     8,     9,     0,    11,   148,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    25,     0,     0,     0,     0,
       0,     0,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,     0,     0,    46,     5,     6,     7,     8,
       9,     0,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,    29,     0,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,     0,     0,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,    45,   153,
       0,    46,     5,     6,     7,     8,     9,     0,    11,   184,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    25,
       0,     0,     0,     0,     0,     0,    26,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,     0,     0,    46,
       5,     6,     7,     8,     9,     0,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,    29,     0,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,     0,    25,     0,     0,
       0,     0,     0,     0,    26,     0,     0,     0,     0,     0,
       0,     0,    45,     0,   199,    46,     5,     6,     7,     8,
       9,     0,    11,   260,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
      29,     0,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    25,     0,     0,     0,     0,     0,     0,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,     0,     0,    46,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
      27,    28,     0,     0,     0,     0,    29,     0,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,     0,
       0,     0,   121,   122,   123,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,     0,   124,    46,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   121,   122,   123,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   124,
     168,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   121,   122,   123,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     124,   169,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   121,   122,   123,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   124,   172,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   121,   122,   123,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   124,   253,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   236,   124,     0,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   121,   122,
     123,     0,   258,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   124,     0,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   121,   122,   123,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   124,   259,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   121,   122,   123,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   124,     0,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   122,   123,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     124,     0,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   123,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   124,     0,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   -52,   -52,   -52,   -52,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145
};

static const short int yycheck[] =
{
      13,    43,    43,    38,    10,    28,    29,    28,    29,    39,
      42,   235,    43,    38,    61,    21,    22,    42,    24,     6,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,   178,   108,    39,    40,    39,    40,    45,
       6,     7,     6,     7,   113,   340,    80,    81,    82,     0,
      56,     6,     7,    59,   113,    61,   113,    63,    64,    65,
     113,    96,   357,   110,   113,    71,    70,   113,    70,   293,
      43,    96,   114,    43,    40,   110,    80,     7,    80,    66,
      43,   113,    43,   114,   114,   110,    43,    43,   111,    43,
     111,    43,   237,    43,    43,     7,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,     6,
       7,    66,     7,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     285,   114,   112,   110,   148,   151,   148,     7,   276,   112,
     116,   296,   116,   114,    66,    61,     6,   114,   114,    12,
     114,   116,   114,   112,   114,    44,    45,    46,   113,    66,
     298,    66,   300,    77,    78,    79,    80,    81,    82,   113,
     184,    60,   184,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,   255,   256,   112,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,   232,
     113,   234,   228,   264,    99,   100,   239,   233,     6,   235,
     271,     6,   236,    42,   236,    48,   115,   241,     6,   241,
       6,   113,   246,   247,   246,   247,    27,    28,    29,   255,
     256,    43,    48,   259,   112,     6,   260,   113,   260,   113,
       7,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,     3,     4,     5,     6,   113,   284,   320,
     113,   113,   295,    27,    28,    29,     6,   293,   112,    48,
     294,   332,   294,   334,    48,   112,    48,   112,   112,     6,
     313,   112,   110,     7,   110,   112,   347,    94,    95,   113,
       7,   114,   114,   354,   318,   112,   318,    10,   112,   114,
     361,   327,   112,   112,   163,   338,   245,   340,   302,   335,
     344,   246,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,   247,    14,   357,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,   366,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   107,    -1,   109,   110,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    44,    45,    46,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     107,    -1,    -1,   110,   111,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,
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
      19,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    44,    45,    46,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    60,    -1,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,   107,    -1,
      -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    17,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,    44,
      45,    46,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,
      -1,    -1,   107,    -1,    -1,   110,    -1,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    44,    45,    46,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,    -1,    -1,   107,    -1,    -1,   110,
     111,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    44,    45,    46,
      -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    60,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   114,    -1,    -1,
     107,    -1,    -1,   110,   111,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    15,    16,    -1,    18,    -1,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    44,    45,    46,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,   112,
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
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   107,    -1,    -1,   110,
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
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    44,    45,    46,    -1,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,   107,    -1,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,   107,    -1,
      -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    40,    -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,    -1,     9,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    40,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,   114,
      -1,   116,     3,     4,     5,     6,     7,    -1,     9,    66,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    -1,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,     3,     4,     5,     6,
       7,    -1,     9,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,    -1,    -1,   116,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    44,    45,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,    60,   116,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    45,    46,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,
     112,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,   112,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,   112,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,   111,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    20,    60,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    44,    45,
      46,    -1,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    -1,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    44,    45,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    44,    45,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    60,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    45,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   108,   118,   119,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    14,    16,    18,    22,    23,
      26,    30,    31,    32,    33,    40,    47,    77,    78,    83,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,   107,   109,   110,   112,   113,   116,   120,   121,   128,
     141,   142,   143,   144,   146,    42,   113,   123,   146,   113,
     147,   113,   121,   113,   113,   113,   113,     7,   126,   127,
     113,   113,   112,   146,   112,   146,     6,    66,   112,   146,
     113,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,     6,   142,   143,   142,     7,   125,
     119,   146,   110,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    94,    95,    38,    42,    96,
     110,    44,    45,    46,    60,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,   112,     6,    66,   145,
     146,    43,   112,   114,   146,   146,    12,   146,   123,   134,
     146,     6,   122,    43,   112,   124,   142,   146,   112,   112,
     129,     6,   112,    40,   142,   148,   149,    38,    48,    43,
     112,   111,   114,   146,    66,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,     6,     6,   115,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   142,    43,   114,
     146,   114,   114,   113,   114,   112,    20,    48,    43,   114,
     127,    43,   114,   114,   113,   113,   113,    43,   114,     6,
       6,   141,     7,   111,   142,   113,   113,   115,   111,    61,
      66,   146,   121,   146,    61,   121,   137,   134,   142,   141,
       6,    61,   121,   142,   112,    61,   110,   138,     6,   130,
     131,   130,   148,   149,    48,    48,   145,   145,   146,   142,
     135,   114,   119,   112,    39,   114,    48,   119,   112,   139,
     112,   139,    43,   114,     7,    66,   114,   114,   146,   141,
     114,   114,    99,   100,   136,   112,    13,   134,    66,   142,
      61,   121,   133,   141,    19,   139,    27,    28,    29,   139,
     111,   131,   110,     7,   110,   113,   121,   112,   114,   142,
     114,   119,    27,   112,   146,    61,   112,   140,   111,     7,
      66,   119,   119,   146,    61,   121,   132,   114,   133,    17,
     112,   140,   119,     7,   111,   111,   114,   119,   133,   112,
     119,   121,    15,   112
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
#line 143 "php_parser.y"
    { g_syn_tree_top = (yyvsp[-1].syn_node); ;}
    break;

  case 3:
#line 147 "php_parser.y"
    { (yyval.syn_node) = add_statement_2_list((yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 4:
#line 148 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 7:
#line 159 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 8:
#line 160 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_EXPR, (yyvsp[-1].exp_node)); ;}
    break;

  case 9:
#line 161 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 10:
#line 162 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 11:
#line 163 "php_parser.y"
    { (yyval.syn_node) = make_ifelse_syn_node((yyvsp[-4].exp_node), (yyvsp[-2].syn_node), (yyvsp[-1].syn_node), (yyvsp[0].syn_node)); ;}
    break;

  case 12:
#line 164 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 1); ;}
    break;

  case 13:
#line 165 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[-5].syn_node), 0); ;}
    break;

  case 14:
#line 166 "php_parser.y"
    { ;}
    break;

  case 15:
#line 167 "php_parser.y"
    { ;}
    break;

  case 16:
#line 168 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, 0); ;}
    break;

  case 17:
#line 169 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, (yyvsp[-1].exp_node)); ;}
    break;

  case 18:
#line 170 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, 0); ;}
    break;

  case 19:
#line 171 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, (yyvsp[-1].exp_node)); ;}
    break;

  case 20:
#line 172 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, 0); ;}
    break;

  case 21:
#line 173 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, (yyvsp[-1].exp_node)); ;}
    break;

  case 22:
#line 174 "php_parser.y"
    {  ;}
    break;

  case 23:
#line 175 "php_parser.y"
    {  ;}
    break;

  case 24:
#line 176 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-4].exp_node), 0, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 0);
			;}
    break;

  case 25:
#line 179 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-6].exp_node), (yyvsp[-4].exp_node)->var_node, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 0);
			;}
    break;

  case 26:
#line 182 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-7].exp_node), (yyvsp[-5].exp_node)->var_node, (yyvsp[-2].exp_node)->var_node, (yyvsp[0].syn_node), 1);
			;}
    break;

  case 27:
#line 185 "php_parser.y"
    { ;}
    break;

  case 28:
#line 186 "php_parser.y"
    { ;}
    break;

  case 29:
#line 187 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 30:
#line 190 "php_parser.y"
    {  ;}
    break;

  case 31:
#line 191 "php_parser.y"
    {  ;}
    break;

  case 32:
#line 194 "php_parser.y"
    {  ;}
    break;

  case 33:
#line 195 "php_parser.y"
    {  ;}
    break;

  case 36:
#line 203 "php_parser.y"
    {  ;}
    break;

  case 37:
#line 204 "php_parser.y"
    {  ;}
    break;

  case 38:
#line 205 "php_parser.y"
    {  ;}
    break;

  case 39:
#line 206 "php_parser.y"
    {  ;}
    break;

  case 40:
#line 209 "php_parser.y"
    {  ;}
    break;

  case 41:
#line 210 "php_parser.y"
    {  ;}
    break;

  case 42:
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
			php_report_error("There is no such global var", PHP_ERROR);
		}
	;}
    break;

  case 43:
#line 229 "php_parser.y"
    {
				switch_push_scope_table(make_scope_table())
			;}
    break;

  case 44:
#line 231 "php_parser.y"
    {
				(yyval.syn_node) = make_func_decl_syn_node((yyvsp[-7].str_val), (yyvsp[-4].exp_node));
				(yyval.syn_node)->func_decl->scope = g_current_scope;
				(yyval.syn_node)->func_decl->is_native = 0;
				(yyval.syn_node)->func_decl->code = (yyvsp[-1].syn_node);
				switch_pop_scope_table(0);
				add_func_2_scope(g_current_scope, (yyval.syn_node));
			;}
    break;

  case 45:
#line 239 "php_parser.y"
    {  ;}
    break;

  case 46:
#line 243 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node)->var_node, (yyvsp[-1].str_val), 0); ;}
    break;

  case 47:
#line 244 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node)->var_node, (yyvsp[-2].str_val), 1); ;}
    break;

  case 48:
#line 245 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[-3].exp_node), (yyvsp[0].exp_node)->var_node, (yyvsp[-1].str_val), 0); ;}
    break;

  case 49:
#line 246 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[-4].exp_node), (yyvsp[0].exp_node)->var_node, (yyvsp[-2].str_val), 1); ;}
    break;

  case 50:
#line 247 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 51:
#line 251 "php_parser.y"
    { (yyval.str_val)[0] = 0; ;}
    break;

  case 54:
#line 257 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 56:
#line 262 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 58:
#line 266 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 59:
#line 270 "php_parser.y"
    { (yyval.syn_node) = add_branch_2_elseif((yyvsp[-5].syn_node), make_ifelse_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0, 0)); ;}
    break;

  case 60:
#line 271 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 61:
#line 275 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 62:
#line 276 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[0].syn_node); ;}
    break;

  case 64:
#line 280 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 65:
#line 284 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 66:
#line 285 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-1].syn_node); ;}
    break;

  case 67:
#line 286 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 68:
#line 287 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[-2].syn_node); ;}
    break;

  case 69:
#line 291 "php_parser.y"
    {  (yyval.syn_node) = 0; ;}
    break;

  case 70:
#line 292 "php_parser.y"
    {  ;}
    break;

  case 71:
#line 293 "php_parser.y"
    {  ;}
    break;

  case 77:
#line 304 "php_parser.y"
    { (yyval.exp_node) = make_known_const((yyvsp[0].str_val)); ;}
    break;

  case 79:
#line 308 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str((yyvsp[-2].str_val)), make_const_exp_str((yyvsp[0].str_val))); ;}
    break;

  case 80:
#line 309 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OBJECT_DEREF, (yyvsp[-2].exp_node), make_const_exp_str((yyvsp[0].str_val))); ;}
    break;

  case 82:
#line 314 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_BY_KEY, 0); ;}
    break;

  case 83:
#line 315 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 84:
#line 316 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));;}
    break;

  case 85:
#line 317 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[-1].exp_node)); ;}
    break;

  case 86:
#line 321 "php_parser.y"
    { (yyval.exp_node) = make_func_call_exp((yyvsp[-3].str_val), (yyvsp[-1].exp_node)); ;}
    break;

  case 87:
#line 322 "php_parser.y"
    { ;}
    break;

  case 88:
#line 323 "php_parser.y"
    { ;}
    break;

  case 89:
#line 326 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 90:
#line 327 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 91:
#line 328 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-2].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); ;}
    break;

  case 92:
#line 329 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-3].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); ;}
    break;

  case 93:
#line 330 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); ;}
    break;

  case 94:
#line 335 "php_parser.y"
    { ;}
    break;

  case 96:
#line 337 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ASS, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 97:
#line 338 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 98:
#line 339 "php_parser.y"
    {  ;}
    break;

  case 99:
#line 344 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 100:
#line 345 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 101:
#line 346 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 102:
#line 347 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 103:
#line 348 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 104:
#line 349 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 105:
#line 350 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 106:
#line 351 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 107:
#line 352 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 108:
#line 353 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 109:
#line 354 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); ;}
    break;

  case 110:
#line 356 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 111:
#line 357 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 112:
#line 358 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 113:
#line 359 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[0].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 114:
#line 361 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 115:
#line 362 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 116:
#line 363 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 117:
#line 364 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 118:
#line 365 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 119:
#line 366 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 120:
#line 367 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 121:
#line 368 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 122:
#line 369 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 123:
#line 370 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 124:
#line 371 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 125:
#line 372 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 126:
#line 373 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 127:
#line 374 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 128:
#line 375 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 129:
#line 376 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 130:
#line 377 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 131:
#line 378 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), (yyvsp[0].exp_node)); ;}
    break;

  case 132:
#line 379 "php_parser.y"
    {  ;}
    break;

  case 133:
#line 380 "php_parser.y"
    {  ;}
    break;

  case 134:
#line 381 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 135:
#line 382 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NOT_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 136:
#line 383 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 137:
#line 384 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NEQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 138:
#line 385 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 139:
#line 386 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 140:
#line 387 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 141:
#line 388 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); ;}
    break;

  case 142:
#line 389 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 143:
#line 390 "php_parser.y"
    {  ;}
    break;

  case 144:
#line 391 "php_parser.y"
    {  ;}
    break;

  case 145:
#line 392 "php_parser.y"
    {  ;}
    break;

  case 146:
#line 393 "php_parser.y"
    {  ;}
    break;

  case 147:
#line 394 "php_parser.y"
    {  ;}
    break;

  case 148:
#line 395 "php_parser.y"
    {  ;}
    break;

  case 149:
#line 396 "php_parser.y"
    {  ;}
    break;

  case 150:
#line 397 "php_parser.y"
    {  ;}
    break;

  case 151:
#line 398 "php_parser.y"
    {  ;}
    break;

  case 152:
#line 399 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 153:
#line 401 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[0].exp_node); ;}
    break;

  case 154:
#line 403 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_PRINT, (yyvsp[0].exp_node)); ;}
    break;

  case 155:
#line 406 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 156:
#line 407 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 157:
#line 408 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 160:
#line 416 "php_parser.y"
    { /*$$ = make_assign_node($1);*/ ;}
    break;

  case 161:
#line 417 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[-1].exp_node); ;}
    break;

  case 162:
#line 418 "php_parser.y"
    { /*$$ = make_assign_node(0);*/ ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 2855 "php_parser.c"

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



