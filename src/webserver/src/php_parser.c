/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         phpparse
#define yylex           phplex
#define yyerror         phperror
#define yylval          phplval
#define yychar          phpchar
#define yydebug         phpdebug
#define yynerrs         phpnerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "php_parser.y"

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2005-2011 Froenchenko Leonid ( lfroen@gmail.com / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <stdio.h>
#include <string.h>

#include "php_syntree.h"

int phplex();

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



/* Line 189 of yacc.c  */
#line 147 "php_parser.c"

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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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
     T_ECHO = 263,
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
     PRINT = 298,
     SR_EQ = 299,
     SL_EQ = 300,
     XOR_EQ = 301,
     OR_EQ = 302,
     AND_EQ = 303,
     MOD_EQ = 304,
     CONCAT_EQ = 305,
     DIV_EQ = 306,
     MUL_EQ = 307,
     MINUS_EQ = 308,
     PLUS_EQ = 309,
     LOG_OR = 310,
     LOG_XOR = 311,
     LOG_AND = 312,
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 67 "php_parser.y"

	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];



/* Line 214 of yacc.c  */
#line 282 "php_parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 294 "php_parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3075

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  117
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  169
/* YYNRULES -- Number of states.  */
#define YYNSTATES  385

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   345

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,   116,    82,    66,     2,
     113,   114,    80,    77,    43,    78,    79,    81,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    58,   112,
      71,    45,    72,    57,    86,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    44,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      59,    60,    61,    62,    63,    67,    68,    69,    70,    73,
      74,    75,    76,    84,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     7,    10,    11,    13,    17,    19,    23,
      26,    30,    34,    42,    48,    56,    66,    72,    75,    79,
      82,    86,    89,    93,    97,   103,   111,   121,   132,   138,
     146,   148,   152,   158,   160,   164,   166,   170,   172,   176,
     178,   180,   184,   186,   190,   191,   201,   211,   214,   218,
     223,   229,   230,   231,   233,   235,   240,   242,   247,   249,
     250,   257,   258,   259,   262,   264,   269,   273,   278,   283,
     289,   290,   295,   298,   300,   302,   304,   306,   308,   310,
     312,   314,   318,   322,   324,   328,   333,   338,   343,   348,
     355,   362,   364,   367,   371,   376,   377,   384,   386,   390,
     392,   397,   401,   405,   409,   413,   417,   421,   425,   429,
     433,   437,   441,   444,   447,   450,   453,   457,   461,   465,
     469,   473,   477,   481,   485,   489,   493,   497,   501,   505,
     509,   513,   517,   520,   523,   526,   529,   533,   537,   541,
     545,   549,   553,   557,   561,   565,   571,   574,   577,   580,
     583,   586,   589,   592,   594,   599,   602,   606,   609,   610,
     612,   616,   618,   623,   624,   626,   630,   632,   636,   641
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     118,     0,    -1,   108,   119,   109,    -1,   119,   120,    -1,
      -1,   121,    -1,   108,   119,   109,    -1,   129,    -1,   110,
     119,   111,    -1,   148,   112,    -1,    22,   125,   112,    -1,
     107,   127,   112,    -1,    10,   113,   148,   114,   121,   136,
     137,    -1,    12,   113,   148,   114,   138,    -1,    11,   121,
      12,   113,   148,   114,   112,    -1,    14,   113,   135,   112,
     135,   112,   135,   114,   133,    -1,    26,   113,   148,   114,
     139,    -1,    31,   112,    -1,    31,   148,   112,    -1,    30,
     112,    -1,    30,   148,   112,    -1,    33,   112,    -1,    33,
     148,   112,    -1,     8,   123,   112,    -1,    23,   113,   124,
     114,   112,    -1,    16,   113,   148,    20,   144,   114,   134,
      -1,    16,   113,   148,    20,   144,    39,   144,   114,   134,
      -1,    16,   113,   148,    20,   144,    39,    66,   144,   114,
     134,    -1,    18,   113,   122,   114,   121,    -1,    18,   113,
     122,   114,    58,   119,    19,    -1,   112,    -1,     6,    45,
     143,    -1,   122,    43,     6,    45,   143,    -1,   148,    -1,
     123,    43,   148,    -1,   144,    -1,   124,    43,   144,    -1,
     126,    -1,   125,    43,   126,    -1,     7,    -1,   128,    -1,
     127,    43,   128,    -1,     7,    -1,     7,    45,   143,    -1,
      -1,    32,     6,   130,   113,   131,   114,   110,   119,   111,
      -1,    32,    66,     6,   113,   131,   114,   110,   119,   111,
      -1,   132,     7,    -1,   132,    66,     7,    -1,   131,    43,
     132,     7,    -1,   131,    43,   132,    66,     7,    -1,    -1,
      -1,     6,    -1,   121,    -1,    58,   119,    15,   112,    -1,
     121,    -1,    58,   119,    17,   112,    -1,   123,    -1,    -1,
     136,    99,   113,   148,   114,   121,    -1,    -1,    -1,   100,
     121,    -1,   121,    -1,    58,   119,    13,   112,    -1,   110,
     140,   111,    -1,   110,   112,   140,   111,    -1,    58,   140,
      27,   112,    -1,    58,   112,   140,    27,   112,    -1,    -1,
     140,   141,   142,   119,    -1,    28,   148,    -1,    29,    -1,
      58,    -1,   112,    -1,     3,    -1,     4,    -1,     5,    -1,
       6,    -1,   145,    -1,     6,    42,     6,    -1,   145,    38,
       6,    -1,     7,    -1,   145,    96,   115,    -1,   145,    96,
     148,   115,    -1,   145,   110,   148,   111,    -1,   116,   110,
     148,   111,    -1,     6,   113,   147,   114,    -1,   145,    42,
       6,   113,   147,   114,    -1,   145,    38,     6,   113,   147,
     114,    -1,   148,    -1,    66,   144,    -1,   147,    43,   148,
      -1,   147,    43,    66,   144,    -1,    -1,    40,   113,   150,
     114,    45,   148,    -1,   144,    -1,   144,    45,   148,    -1,
     146,    -1,   144,    45,    66,   144,    -1,   144,    56,   148,
      -1,   144,    55,   148,    -1,   144,    54,   148,    -1,   144,
      53,   148,    -1,   144,    52,   148,    -1,   144,    51,   148,
      -1,   144,    50,   148,    -1,   144,    49,   148,    -1,   144,
      48,   148,    -1,   144,    47,   148,    -1,   144,    46,   148,
      -1,   144,    95,    -1,    95,   144,    -1,   144,    94,    -1,
      94,   144,    -1,   148,    62,   148,    -1,   148,    63,   148,
      -1,   148,    59,   148,    -1,   148,    61,   148,    -1,   148,
      60,   148,    -1,   148,    64,   148,    -1,   148,    66,   148,
      -1,   148,    65,   148,    -1,   148,    79,   148,    -1,   148,
      77,   148,    -1,   148,    78,   148,    -1,   148,    80,   148,
      -1,   148,    81,   148,    -1,   148,    82,   148,    -1,   148,
      76,   148,    -1,   148,    75,   148,    -1,    77,   148,    -1,
      78,   148,    -1,    83,   148,    -1,    85,   148,    -1,   148,
      68,   148,    -1,   148,    67,   148,    -1,   148,    70,   148,
      -1,   148,    69,   148,    -1,   148,    71,   148,    -1,   148,
      74,   148,    -1,   148,    72,   148,    -1,   148,    73,   148,
      -1,   113,   148,   114,    -1,   148,    57,   148,    58,   148,
      -1,    93,   148,    -1,    92,   148,    -1,    91,   148,    -1,
      88,   148,    -1,    87,   148,    -1,     9,   149,    -1,    86,
     148,    -1,   143,    -1,    41,   113,   152,   114,    -1,    44,
     148,    -1,   113,   148,   114,    -1,   113,   114,    -1,    -1,
     151,    -1,   150,    43,   151,    -1,   144,    -1,    40,   113,
     150,   114,    -1,    -1,   153,    -1,   152,    43,   153,    -1,
     148,    -1,   148,    39,   148,    -1,   148,    39,    66,   144,
      -1,    66,   144,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   142,   142,   146,   147,   152,   153,   154,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   179,   182,   185,   186,
     187,   190,   191,   194,   195,   204,   205,   211,   212,   216,
     231,   232,   235,   236,   244,   244,   255,   259,   260,   261,
     262,   263,   267,   268,   272,   273,   277,   278,   281,   282,
     286,   287,   291,   292,   295,   296,   300,   301,   302,   303,
     307,   308,   323,   324,   327,   328,   332,   333,   334,   335,
     338,   339,   340,   344,   345,   346,   347,   348,   352,   353,
     354,   357,   358,   359,   360,   361,   366,   367,   368,   369,
     370,   375,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,   387,   388,   389,   390,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,   422,   423,   424,   425,
     428,   429,   430,   432,   433,   434,   437,   438,   439,   442,
     443,   447,   448,   449,   452,   453,   462,   463,   464,   465
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "FNUMBER", "DNUMBER", "STRING", "IDENT",
  "VARIABLE", "T_ECHO", "EXIT", "IF", "DO", "WHILE", "ENDWHILE", "FOR",
  "ENDFOR", "FOREACH", "ENDFOREACH", "DECLARE", "ENDDECLARE", "AS",
  "CONST", "GLOBAL", "UNSET", "ISSET", "EMPTY", "SWITCH", "ENDSWITCH",
  "CASE", "DEFAULT", "BREAK", "CONTINUE", "FUNCTION", "RETURN", "CLASS",
  "INTERFACE", "EXTENDS", "IMPLEMENTS", "OBJECT_OPERATOR", "HASH_ASSIGN",
  "LIST", "ARRAY", "CLASS_SCOPE", "','", "PRINT", "'='", "SR_EQ", "SL_EQ",
  "XOR_EQ", "OR_EQ", "AND_EQ", "MOD_EQ", "CONCAT_EQ", "DIV_EQ", "MUL_EQ",
  "MINUS_EQ", "PLUS_EQ", "'?'", "':'", "LOG_OR", "LOG_XOR", "LOG_AND",
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
  "$@1", "parameter_list", "optional_class_type", "for_statement",
  "foreach_statement", "for_expr", "elseif_list", "else_statement",
  "while_statement", "switch_case_list", "case_list", "case_list_item",
  "case_separator", "const_value", "variable", "deref_variable",
  "function_call", "func_param_list", "expr", "exit_expr",
  "assignment_list", "assignment_list_element", "array_pair_list",
  "array_elem", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,    44,   298,    61,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,    63,    58,   310,
     311,   312,   313,   314,   124,    94,    38,   315,   316,   317,
     318,    60,    62,   319,   320,   321,   322,    43,    45,    46,
      42,    47,    37,    33,   323,   126,    64,   324,   325,   326,
     327,   328,   329,   330,   331,   332,    91,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     123,   125,    59,    40,    41,    93,    36
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   117,   118,   119,   119,   120,   120,   120,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   122,   122,   123,   123,   124,   124,   125,   125,   126,
     127,   127,   128,   128,   130,   129,   129,   131,   131,   131,
     131,   131,   132,   132,   133,   133,   134,   134,   135,   135,
     136,   136,   137,   137,   138,   138,   139,   139,   139,   139,
     140,   140,   141,   141,   142,   142,   143,   143,   143,   143,
     144,   144,   144,   145,   145,   145,   145,   145,   146,   146,
     146,   147,   147,   147,   147,   147,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   149,   149,   149,   150,
     150,   151,   151,   151,   152,   152,   153,   153,   153,   153
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     2,     0,     1,     3,     1,     3,     2,
       3,     3,     7,     5,     7,     9,     5,     2,     3,     2,
       3,     2,     3,     3,     5,     7,     9,    10,     5,     7,
       1,     3,     5,     1,     3,     1,     3,     1,     3,     1,
       1,     3,     1,     3,     0,     9,     9,     2,     3,     4,
       5,     0,     0,     1,     1,     4,     1,     4,     1,     0,
       6,     0,     0,     2,     1,     4,     3,     4,     4,     5,
       0,     4,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     1,     3,     4,     4,     4,     4,     6,
       6,     1,     2,     3,     4,     0,     6,     1,     3,     1,
       4,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     2,     2,     2,     2,
       2,     2,     2,     1,     4,     2,     3,     2,     0,     1,
       3,     1,     4,     0,     1,     3,     1,     3,     4,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     4,     0,     0,     1,    76,    77,    78,    79,    83,
       0,   158,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     4,     2,     4,    30,     0,     0,     3,     5,     7,
     153,    97,    80,    99,     0,     0,    95,     0,    33,     0,
     151,     0,     0,     0,    59,     0,     0,    39,     0,    37,
       0,     0,    19,     0,    17,     0,    44,     0,    21,     0,
     163,     0,   155,   132,   133,   134,   135,   152,   150,   149,
     148,   147,   146,     0,   115,    80,   113,    42,     0,    40,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   114,   112,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     9,    81,     0,
       0,    91,     0,    23,   157,     0,     0,     0,     0,    58,
       0,     0,     0,     0,     0,    10,     0,    35,     0,    20,
      18,     0,     0,    22,     0,   161,     0,   159,     0,   166,
       0,   164,     0,     0,     0,    11,     6,     8,   144,     0,
       0,    98,   111,   110,   109,   108,   107,   106,   105,   104,
     103,   102,   101,    82,     0,    84,     0,     0,     0,   118,
     120,   119,   116,   117,   121,   123,   122,   137,   136,   139,
     138,   140,   142,   143,   141,   131,   130,   125,   126,   124,
     127,   128,   129,    92,     0,    88,    34,   156,     0,     0,
       0,    59,     0,     0,     0,     0,    38,     0,     0,     0,
      51,    51,   163,   163,     0,   169,     0,     0,   154,    82,
      79,    43,    41,    87,   100,    95,    95,    85,    86,     0,
       0,    93,    61,     0,     4,    64,    13,     0,     0,    31,
       0,     4,    28,    36,    24,    70,    70,    16,    53,     0,
       0,     0,     0,   160,     0,     0,   167,   165,     0,     0,
     145,    94,    62,     0,     0,    59,     0,     0,     0,     0,
      70,     0,    70,     0,    52,     0,    47,     0,     0,   162,
      96,   168,    90,    89,     0,     0,    12,    14,     0,     0,
       0,     0,     4,    56,    25,    32,    29,     0,     0,     0,
      73,     0,     0,    66,     0,     4,    48,     4,     0,    63,
      65,     0,     0,     0,     0,     0,    68,    72,    74,    75,
       4,    67,    49,     0,     0,     0,     0,     4,    54,    15,
       0,    26,     0,    69,    71,    50,    45,    46,     0,     0,
      27,    57,    60,     0,    55
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    47,    48,   163,   159,   166,    68,    69,
      98,    99,    49,   171,   289,   290,   369,   334,   160,   302,
     326,   276,   287,   311,   341,   360,    50,    51,    52,    53,
     150,    54,    60,   176,   177,   180,   181
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -309
static const yytype_int16 yypact[] =
{
     -75,  -309,    36,   371,  -309,  -309,  -309,  -309,   -30,  -309,
    2593,   -59,    58,  1967,    59,    69,    71,    98,    66,   113,
     123,  2023,  2081,     8,  2137,   124,   126,  2593,  2593,  2593,
    2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,    34,    34,
     147,  -309,  -309,  -309,  -309,  2593,    49,  -309,  -309,  -309,
    -309,   222,   -29,  -309,  1383,   234,  2195,     0,  2864,  2251,
    -309,  2593,   230,  2593,  2593,  2593,   238,  -309,    51,  -309,
      34,  2593,  -309,  2651,  -309,  2677,  -309,   239,  -309,  2705,
      31,  2309,  2864,   -33,   -33,  -309,  -309,  -309,  -309,  -309,
    -309,  -309,  -309,   204,  -309,   -28,  -309,   208,    52,  -309,
     485,   599,   585,  2593,  2365,  2593,  2593,  2593,  2593,  2593,
    2593,  2593,  2593,  2593,  2593,  2593,  -309,  -309,   241,   248,
    2423,  2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,
    2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,  2593,
    2593,  2593,  2593,  2593,  2593,  2593,  2593,  -309,  -309,    34,
      -1,  2864,  2593,  -309,  -309,   699,   813,   143,   927,   212,
     145,   128,   213,    29,    66,  -309,    35,  -309,  1041,  -309,
    -309,   148,   149,  -309,   151,  -309,    37,  -309,    34,  2812,
      41,  -309,   259,   172,   147,  -309,  -309,  -309,  -309,  2731,
      34,  2864,  2864,  2864,  2864,  2864,  2864,  2864,  2864,  2864,
    2864,  2864,  2864,   166,   167,  -309,   471,  2759,  2838,  2911,
    2933,  2954,  2974,  2993,  1505,   350,  1617,  1731,  1731,  1731,
    1731,   142,   142,   142,   142,    88,    88,   -33,   -33,   -33,
    -309,  -309,  -309,  -309,  2479,  -309,  2864,  -309,  1967,  2593,
    1511,  2593,    34,   172,   276,  1625,  -309,    34,   171,   -41,
      86,    86,    31,    31,   240,  -309,  2537,  2309,  -309,  -309,
    -309,  -309,  -309,  -309,  -309,  2195,  2195,  -309,  -309,  2593,
      34,  2864,  -309,  1155,  -309,  -309,  -309,   174,   -37,  -309,
     242,  -309,  -309,  -309,  -309,   177,   178,  -309,  -309,    42,
       4,    43,    44,  -309,  2593,    34,  2864,  -309,    46,    48,
    2888,  -309,   -39,   183,   713,  2593,    45,  1739,   172,   827,
    -309,   152,  -309,   -23,   278,   186,  -309,   290,   188,  -309,
    2864,  -309,  -309,  -309,   187,  1967,  -309,  -309,   189,   190,
      34,   192,  -309,  -309,  -309,  -309,  -309,   202,   191,  2593,
    -309,    -2,   -21,  -309,   146,  -309,  -309,  -309,  2593,  -309,
    -309,  1853,   194,  1739,   941,   197,  -309,  2864,  -309,  -309,
    -309,  -309,  -309,   303,  1055,  1169,  1269,  -309,  -309,  -309,
    1739,  -309,   199,  -309,  1397,  -309,  -309,  -309,  1967,  1283,
    -309,  -309,  -309,   201,  -309
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -309,  -309,   -40,  -309,   -13,  -309,   304,  -309,  -309,   154,
    -309,   131,  -309,  -309,    68,     7,  -309,  -308,  -209,  -309,
    -309,  -309,  -309,  -266,  -309,  -309,  -164,    -4,   103,  -309,
    -202,    -6,  -309,    70,    72,  -309,    67
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -53
static const yytype_int16 yytable[] =
{
      62,   100,   306,   101,    58,   339,   340,   339,   340,   118,
     182,   316,    55,   119,    76,    73,    75,   285,    79,   261,
     313,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,   277,     1,    94,    96,     4,    93,     9,   102,
      93,     9,   234,   152,   337,   371,   342,   144,   145,   146,
     151,    93,     9,   155,    59,   156,   358,   158,    58,   161,
     324,   325,   380,   298,   299,   168,   167,   120,   120,   286,
     317,   174,   244,    67,    77,   179,   175,   307,   247,   279,
     253,   121,   121,    56,   257,   314,   314,   253,   343,   234,
     361,   234,   288,   -52,   164,   184,   329,   189,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   202,
     359,   330,   153,   235,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,    95,    95,   245,   335,   233,   236,    46,   242,   248,
      46,   254,   -52,   362,    97,   258,   315,   318,   319,   103,
     322,    46,   323,   165,   185,   141,   142,   143,   144,   145,
     146,    61,    63,    95,   255,     5,     6,     7,   260,   338,
     339,   340,    64,    95,    65,   122,   264,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    66,   363,   -53,   -53,   -53,   -53,   139,   140,   141,
     142,   143,   144,   145,   146,   272,    70,   275,   271,   355,
     339,   340,   282,   273,   304,    58,    71,    80,   278,    81,
     148,   309,   157,   283,   162,   172,    55,   203,   175,   175,
     296,   179,    95,   183,   204,   152,   239,   241,   243,   151,
     151,   250,   251,   300,   252,   259,   301,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   265,
     266,    95,   280,   284,   288,   294,   305,   308,   320,   310,
     312,   321,   354,    95,   333,   327,   345,   346,   347,    58,
     348,   350,   331,   356,   351,   364,   353,   365,   370,   373,
     375,   381,   349,   384,    57,   262,   116,   117,   246,   291,
     374,   344,   292,     0,   297,   293,   352,   379,     0,     0,
       0,     0,     0,   357,     0,     0,     0,     0,   368,     0,
     333,     0,   366,     0,     0,    95,     0,     0,     0,     0,
      95,     0,     0,     0,     0,    95,    95,   333,     0,     0,
       0,     0,     0,     0,     0,   382,     0,     0,     0,     0,
       0,     0,     0,    95,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,    95,     0,
       0,    21,    22,    23,    24,     0,     0,     0,     0,    95,
       0,    25,    26,     0,     0,    27,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,    95,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
       0,     0,     0,     0,    30,     0,    31,    32,    33,    34,
       0,     0,    35,    36,    37,    38,    39,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40,    41,
      42,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,    23,    24,     0,
       0,     0,     0,     0,     0,    25,    26,     0,   122,    27,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,     0,     0,     0,     0,
       0,     0,    28,    29,     0,     0,     0,     0,    30,     0,
      31,    32,    33,    34,     0,     0,    35,    36,    37,    38,
      39,     0,     0,     0,     0,     0,   267,     0,     0,     0,
       0,     0,    40,    41,   186,    43,     0,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,     0,     0,     0,     0,     0,     0,    25,
      26,     0,   122,    27,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,     0,     0,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,     0,     0,
      35,    36,    37,    38,    39,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,    40,    41,     0,    43,
     187,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,   328,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,    23,    24,     0,     0,     0,
       0,     0,     0,    25,    26,     0,   122,    27,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,     0,
       0,     0,     0,   237,     0,     0,     0,     0,     0,     0,
      40,    41,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,     0,    17,   336,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,     0,     0,     0,     0,     0,     0,    25,    26,     0,
     122,    27,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      30,     0,    31,    32,    33,    34,     0,     0,    35,    36,
      37,    38,    39,     0,     0,     0,     0,   238,     0,     0,
       0,     0,     0,     0,    40,    41,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,   372,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,    23,    24,     0,     0,     0,     0,     0,
       0,    25,    26,     0,   122,    27,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
       0,     0,     0,     0,    30,     0,    31,    32,    33,    34,
       0,     0,    35,    36,    37,    38,    39,     0,     0,     0,
       0,   240,     0,     0,     0,     0,     0,     0,    40,    41,
       0,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,    23,    24,     0,
       0,     0,     0,     0,     0,    25,    26,     0,   122,    27,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,     0,     0,     0,     0,
       0,     0,    28,    29,     0,     0,     0,     0,    30,     0,
      31,    32,    33,    34,     0,     0,    35,    36,    37,    38,
      39,     0,     0,     0,     0,   249,     0,     0,     0,     0,
       0,     0,    40,    41,     0,    43,   376,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,    23,    24,     0,     0,     0,     0,     0,     0,    25,
      26,     0,   122,    27,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,     0,     0,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,     0,     0,
      35,    36,    37,    38,    39,     0,     0,     0,     0,   303,
       0,     0,     0,     0,     0,     0,    40,    41,     0,    43,
     377,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,   383,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,    23,    24,     0,     0,     0,
       0,     0,     0,    25,    26,     0,   122,    27,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,     0,
       0,     0,     0,   378,     0,     0,     0,     0,     0,     0,
      40,    41,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,    23,
      24,     0,     0,     0,     0,     0,     0,    25,    26,     0,
     122,    27,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      30,     0,    31,    32,    33,    34,     0,     0,    35,    36,
      37,    38,    39,     0,     0,   147,     0,     0,     0,     0,
       0,     0,     0,     0,    40,    41,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,     0,    15,     0,    16,     0,    17,
       0,     0,     0,    18,    19,     0,     0,    20,     0,     0,
       0,    21,    22,     0,    24,     0,     0,     0,     0,     0,
       0,    25,    26,     0,     0,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   274,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,    28,    29,
       0,     0,     0,     0,    30,     0,    31,    32,    33,    34,
       0,     0,    35,    36,    37,    38,    39,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40,     0,
       0,    43,     0,    44,    45,     0,     0,    46,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,     0,    15,
       0,    16,     0,    17,     0,     0,     0,    18,    19,     0,
       0,    20,     0,     0,     0,    21,    22,     0,    24,     0,
       0,     0,     0,     0,     0,    25,    26,     0,     0,    27,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   281,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,     0,    28,    29,     0,     0,     0,     0,    30,     0,
      31,    32,    33,    34,     0,     0,    35,    36,    37,    38,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    40,     0,     0,    43,     0,    44,    45,     0,
       0,    46,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,     0,    15,     0,    16,     0,    17,     0,     0,
       0,    18,    19,     0,     0,    20,     0,     0,     0,    21,
      22,     0,    24,     0,     0,     0,     0,     0,     0,    25,
      26,     0,     0,    27,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   332,   -53,   -53,
     -53,   -53,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,    28,    29,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,     0,     0,
      35,    36,    37,    38,    39,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    40,     0,     0,    43,
       0,    44,    45,     0,     0,    46,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     0,    15,     0,    16,
       0,    17,     0,     0,     0,    18,    19,     0,     0,    20,
       0,     0,     0,    21,    22,     0,    24,     0,     0,     0,
       0,     0,     0,    25,    26,     0,     0,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   367,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,     0,     0,    43,     0,    44,    45,     0,     0,    46,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       0,    15,     0,    16,     0,    17,     0,     0,     0,    18,
      19,     0,     0,    20,     0,     0,     0,    21,    22,     0,
      24,     0,     0,     0,     0,     0,     0,    25,    26,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     6,     7,     8,
       9,     0,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      30,     0,    31,    32,    33,    34,     0,     0,    35,    36,
      37,    38,    39,    25,    26,     0,     0,    27,     0,     0,
       0,     0,     0,     0,    40,     0,     0,    43,     0,    44,
      45,     0,     0,    46,     5,     6,     7,     8,     9,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,     0,
       0,    25,    26,     0,     0,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    72,    45,     0,     0,    46,
       5,     6,     7,     8,     9,     0,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
       0,     0,     0,     0,    30,     0,    31,    32,    33,    34,
       0,     0,    35,    36,    37,    38,    39,    25,    26,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    74,    45,     0,     0,    46,     5,     6,
       7,     8,     9,     0,    11,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      30,     0,    31,    32,    33,    34,     0,     0,    35,    36,
      37,    38,    39,     0,     0,    25,    26,     0,     0,    27,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    78,
      45,     0,     0,    46,     5,     6,     7,     8,     9,     0,
      11,   149,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    28,    29,     0,     0,     0,     0,    30,     0,
      31,    32,    33,    34,     0,     0,    35,    36,    37,    38,
      39,    25,    26,     0,     0,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,     0,
       0,    46,     5,     6,     7,     8,     9,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    29,
       0,     0,     0,     0,    30,     0,    31,    32,    33,    34,
       0,     0,    35,    36,    37,    38,    39,     0,     0,    25,
      26,     0,     0,    27,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    45,   154,     0,    46,     5,     6,
       7,     8,     9,     0,    11,   178,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,     0,     0,
      35,    36,    37,    38,    39,    25,    26,     0,     0,    27,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,     0,     0,    46,     5,     6,     7,     8,
       9,   190,    11,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    28,    29,     0,     0,     0,     0,    30,     0,
      31,    32,    33,    34,     0,     0,    35,    36,    37,    38,
      39,     0,     0,    25,    26,     0,     0,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    45,     0,
       0,    46,     5,     6,     7,     8,     9,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,    25,
      26,     0,     0,    27,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,     0,   205,    46,
       5,     6,     7,     8,     9,   270,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    28,    29,     0,     0,
       0,     0,    30,     0,    31,    32,    33,    34,     0,     0,
      35,    36,    37,    38,    39,     0,     0,    25,    26,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,     0,     0,    46,     5,     6,     7,     8,
       9,     0,    11,   295,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    28,    29,     0,     0,     0,     0,
      30,     0,    31,    32,    33,    34,     0,     0,    35,    36,
      37,    38,    39,    25,    26,     0,     0,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,     0,     0,    46,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    29,     0,     0,     0,     0,    30,     0,    31,    32,
      33,    34,     0,     0,    35,    36,    37,    38,    39,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,     0,   122,    46,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   122,     0,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
       0,     0,   122,   169,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   122,   170,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,     0,   122,   173,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   263,     0,     0,     0,     0,     0,     0,     0,
       0,   256,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   122,
     268,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   122,   269,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   122,     0,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146
};

static const yytype_int16 yycheck[] =
{
      13,    41,    39,    43,    10,    28,    29,    28,    29,    38,
      38,     7,    42,    42,     6,    21,    22,    58,    24,   183,
     286,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,   241,   108,    38,    39,     0,     6,     7,    45,
       6,     7,    43,    43,   310,   353,   312,    80,    81,    82,
      56,     6,     7,    59,   113,    61,    58,    63,    64,    65,
      99,   100,   370,   265,   266,    71,    70,    96,    96,   110,
      66,    40,    43,     7,    66,    81,    80,   114,    43,   243,
      43,   110,   110,   113,    43,    43,    43,    43,   111,    43,
     111,    43,     6,     7,    43,    43,   305,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     112,    66,   112,   114,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    38,    39,   114,   308,   149,   152,   116,    20,   114,
     116,   114,    66,     7,     7,   114,   114,   114,   114,   110,
     114,   116,   114,   112,   112,    77,    78,    79,    80,    81,
      82,   113,   113,    70,   178,     3,     4,     5,     6,    27,
      28,    29,   113,    80,   113,    57,   190,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,   113,    66,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   238,   113,   240,   234,    27,
      28,    29,   245,   239,   274,   241,   113,   113,   242,   113,
       6,   281,    12,   247,     6,     6,    42,     6,   252,   253,
     256,   257,   149,    45,     6,    43,   113,   112,    45,   265,
     266,   113,   113,   269,   113,     6,   270,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,   113,
     113,   178,     6,   112,     6,    45,   112,    45,   294,   112,
     112,   295,   332,   190,   307,   112,   110,     7,   110,   305,
     113,   112,   306,   112,   114,   345,   114,   347,   114,   112,
       7,   112,   325,   112,    10,   184,    94,    95,   164,   251,
     360,   314,   252,    -1,   257,   253,   330,   367,    -1,    -1,
      -1,    -1,    -1,   339,    -1,    -1,    -1,    -1,   351,    -1,
     353,    -1,   348,    -1,    -1,   242,    -1,    -1,    -1,    -1,
     247,    -1,    -1,    -1,    -1,   252,   253,   370,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   378,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   270,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,   295,    -1,
      -1,    30,    31,    32,    33,    -1,    -1,    -1,    -1,   306,
      -1,    40,    41,    -1,    -1,    44,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,   330,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      -1,    -1,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   107,   108,
     109,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    57,    44,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    -1,    -1,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,    -1,
      -1,    -1,   107,   108,   109,   110,    -1,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      41,    -1,    57,    44,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    -1,    -1,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,   114,
      -1,    -1,    -1,    -1,    -1,    -1,   107,   108,    -1,   110,
     111,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    41,    -1,    57,    44,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,
     107,   108,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    -1,    18,    19,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
      57,    44,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    -1,    -1,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,   114,    -1,    -1,
      -1,    -1,    -1,    -1,   107,   108,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    17,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    41,    -1,    57,    44,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      -1,    -1,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,   107,   108,
      -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    57,    44,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    -1,    -1,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,
      -1,    -1,   107,   108,    -1,   110,   111,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      41,    -1,    57,    44,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    -1,    -1,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,   114,
      -1,    -1,    -1,    -1,    -1,    -1,   107,   108,    -1,   110,
     111,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    15,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    41,    -1,    57,    44,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,   114,    -1,    -1,    -1,    -1,    -1,    -1,
     107,   108,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
      57,    44,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    -1,    -1,    91,    92,
      93,    94,    95,    -1,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   107,   108,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    16,    -1,    18,
      -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,
      -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      -1,    -1,    91,    92,    93,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   107,    -1,
      -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    -1,    -1,    -1,    30,    31,    -1,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    -1,    -1,    91,    92,    93,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   107,    -1,    -1,   110,    -1,   112,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    -1,    -1,
      91,    92,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   107,    -1,    -1,   110,
      -1,   112,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     107,    -1,    -1,   110,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,
      -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    -1,    -1,    91,    92,
      93,    94,    95,    40,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,   107,    -1,    -1,   110,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    -1,
      -1,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   112,   113,    -1,    -1,   116,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      -1,    -1,    91,    92,    93,    94,    95,    40,    41,    -1,
      -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   112,   113,    -1,    -1,   116,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    -1,    -1,    91,    92,
      93,    94,    95,    -1,    -1,    40,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,
     113,    -1,    -1,   116,     3,     4,     5,     6,     7,    -1,
       9,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    -1,    -1,    91,    92,    93,    94,
      95,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,
      -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,
      -1,    -1,    91,    92,    93,    94,    95,    -1,    -1,    40,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   113,   114,    -1,   116,     3,     4,
       5,     6,     7,    -1,     9,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    -1,    -1,
      91,    92,    93,    94,    95,    40,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,    66,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,
      85,    86,    87,    88,    -1,    -1,    91,    92,    93,    94,
      95,    -1,    -1,    40,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,
      -1,   116,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    40,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,   115,   116,
       3,     4,     5,     6,     7,    66,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,
      -1,    -1,    83,    -1,    85,    86,    87,    88,    -1,    -1,
      91,    92,    93,    94,    95,    -1,    -1,    40,    41,    -1,
      -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,    -1,   116,     3,     4,     5,     6,
       7,    -1,     9,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,
      83,    -1,    85,    86,    87,    88,    -1,    -1,    91,    92,
      93,    94,    95,    40,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,    -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,
      87,    88,    -1,    -1,    91,    92,    93,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,    57,   116,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    57,    -1,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      -1,    -1,    57,   112,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    57,   112,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    57,   112,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
     111,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    57,    -1,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   108,   118,   119,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    14,    16,    18,    22,    23,
      26,    30,    31,    32,    33,    40,    41,    44,    77,    78,
      83,    85,    86,    87,    88,    91,    92,    93,    94,    95,
     107,   108,   109,   110,   112,   113,   116,   120,   121,   129,
     143,   144,   145,   146,   148,    42,   113,   123,   148,   113,
     149,   113,   121,   113,   113,   113,   113,     7,   125,   126,
     113,   113,   112,   148,   112,   148,     6,    66,   112,   148,
     113,   113,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,     6,   144,   145,   144,     7,   127,   128,
     119,   119,   148,   110,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    94,    95,    38,    42,
      96,   110,    57,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   112,     6,    66,
     147,   148,    43,   112,   114,   148,   148,    12,   148,   123,
     135,   148,     6,   122,    43,   112,   124,   144,   148,   112,
     112,   130,     6,   112,    40,   144,   150,   151,    66,   148,
     152,   153,    38,    45,    43,   112,   109,   111,   114,   148,
      66,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,     6,     6,   115,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   148,
     148,   148,   148,   144,    43,   114,   148,   114,   114,   113,
     114,   112,    20,    45,    43,   114,   126,    43,   114,   114,
     113,   113,   113,    43,   114,   144,    39,    43,   114,     6,
       6,   143,   128,   111,   144,   113,   113,   115,   111,    58,
      66,   148,   121,   148,    58,   121,   138,   135,   144,   143,
       6,    58,   121,   144,   112,    58,   110,   139,     6,   131,
     132,   131,   150,   151,    45,    66,   148,   153,   147,   147,
     148,   144,   136,   114,   119,   112,    39,   114,    45,   119,
     112,   140,   112,   140,    43,   114,     7,    66,   114,   114,
     148,   144,   114,   114,    99,   100,   137,   112,    13,   135,
      66,   144,    58,   121,   134,   143,    19,   140,    27,    28,
      29,   141,   140,   111,   132,   110,     7,   110,   113,   121,
     112,   114,   144,   114,   119,    27,   112,   148,    58,   112,
     142,   111,     7,    66,   119,   119,   148,    58,   121,   133,
     114,   134,    17,   112,   119,     7,   111,   111,   114,   119,
     134,   112,   121,    15,   112
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
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
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
  YYUSE (yyvaluep);

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
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
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



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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

/* Line 1464 of yacc.c  */
#line 142 "php_parser.y"
    { g_syn_tree_top = (yyvsp[(2) - (3)].syn_node); ;}
    break;

  case 3:

/* Line 1464 of yacc.c  */
#line 146 "php_parser.y"
    { (yyval.syn_node) = add_statement_2_list((yyvsp[(1) - (2)].syn_node), (yyvsp[(2) - (2)].syn_node)); ;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 147 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 153 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (3)].syn_node); ;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 159 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (3)].syn_node); ;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 160 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_EXPR, (yyvsp[(1) - (2)].exp_node)); ;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 161 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 162 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (3)].syn_node); ;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 163 "php_parser.y"
    { (yyval.syn_node) = make_ifelse_syn_node((yyvsp[(3) - (7)].exp_node), (yyvsp[(5) - (7)].syn_node), (yyvsp[(6) - (7)].syn_node), (yyvsp[(7) - (7)].syn_node)); ;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 164 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[(3) - (5)].exp_node), (yyvsp[(5) - (5)].syn_node), 1); ;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 165 "php_parser.y"
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[(5) - (7)].exp_node), (yyvsp[(2) - (7)].syn_node), 0); ;}
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 166 "php_parser.y"
    { (yyval.syn_node) = make_for_syn_node((yyvsp[(3) - (9)].exp_node), (yyvsp[(5) - (9)].exp_node), (yyvsp[(7) - (9)].exp_node), (yyvsp[(9) - (9)].syn_node)); ;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 167 "php_parser.y"
    { (yyval.syn_node) = make_switch_syn_node((yyvsp[(3) - (5)].exp_node), (yyvsp[(5) - (5)].exp_node)); ;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 168 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, 0); ;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 169 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, (yyvsp[(2) - (3)].exp_node)); ;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 170 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, 0); ;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 171 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, (yyvsp[(2) - (3)].exp_node)); ;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 172 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, 0); ;}
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 173 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, (yyvsp[(2) - (3)].exp_node)); ;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 174 "php_parser.y"
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_ECHO, (yyvsp[(2) - (3)].exp_node)); ;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 175 "php_parser.y"
    {  ;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 176 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[(3) - (7)].exp_node), 0, (yyvsp[(5) - (7)].exp_node), (yyvsp[(7) - (7)].syn_node), 0);
			;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 179 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[(3) - (9)].exp_node), (yyvsp[(5) - (9)].exp_node), (yyvsp[(7) - (9)].exp_node), (yyvsp[(9) - (9)].syn_node), 0);
			;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 182 "php_parser.y"
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[(3) - (10)].exp_node), (yyvsp[(5) - (10)].exp_node), (yyvsp[(8) - (10)].exp_node), (yyvsp[(10) - (10)].syn_node), 1);
			;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 185 "php_parser.y"
    { ;}
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 186 "php_parser.y"
    { ;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 187 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 190 "php_parser.y"
    {  ;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 191 "php_parser.y"
    {  ;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 194 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0); (yyval.exp_node)->exp_node = (yyvsp[(1) - (1)].exp_node); ;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 195 "php_parser.y"
    {
				PHP_EXP_NODE *last = (yyvsp[(1) - (3)].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[(3) - (3)].exp_node);
				(yyval.exp_node) = (yyvsp[(1) - (3)].exp_node);
			;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 211 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 212 "php_parser.y"
    {  ;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 216 "php_parser.y"
    {
		const char *varname = get_scope_var_name(g_current_scope, (yyvsp[(1) - (1)].exp_node)->var_si_node->var);
		PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, varname);
		PHP_SCOPE_ITEM *gsi = get_scope_item(g_global_scope, varname);
		if ( gsi && (gsi->type == PHP_SCOPE_VAR) ) {
			free_var_node(si->var);
			php_exp_tree_free((yyvsp[(1) - (1)].exp_node));
			gsi->var->ref_count++;
			si->var = gsi->var;
		} else {
			php_report_error(PHP_ERROR, "There is no such global var");
		}
	;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 231 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 232 "php_parser.y"
    {  ;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 235 "php_parser.y"
    { (yyvsp[(1) - (1)].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[(1) - (1)].exp_node); ;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 236 "php_parser.y"
    {
			(yyvsp[(1) - (3)].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[(1) - (3)].exp_node);
			value_value_assign(&(yyvsp[(1) - (3)].exp_node)->var_node->value, &(yyvsp[(3) - (3)].exp_node)->val_node);
		;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 244 "php_parser.y"
    {
				switch_push_scope_table(make_scope_table())
			;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 246 "php_parser.y"
    {
				(yyval.syn_node) = make_func_decl_syn_node((yyvsp[(2) - (9)].str_val), (yyvsp[(5) - (9)].exp_node));
				(yyval.syn_node)->func_decl->scope = g_current_scope;
				(yyval.syn_node)->func_decl->is_native = 0;
				(yyval.syn_node)->func_decl->code = (yyvsp[(8) - (9)].syn_node);
				switch_pop_scope_table(0);
				add_func_2_scope(g_current_scope, (yyval.syn_node));
				(yyval.syn_node) = 0;
			;}
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 255 "php_parser.y"
    {  ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 259 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[(2) - (2)].exp_node), (yyvsp[(1) - (2)].str_val), 0); ;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 260 "php_parser.y"
    { (yyval.exp_node) = make_func_param(0, (yyvsp[(3) - (3)].exp_node), (yyvsp[(1) - (3)].str_val), 1); ;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 261 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[(1) - (4)].exp_node), (yyvsp[(4) - (4)].exp_node), (yyvsp[(3) - (4)].str_val), 0); ;}
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 262 "php_parser.y"
    { (yyval.exp_node) = make_func_param((yyvsp[(1) - (5)].exp_node), (yyvsp[(5) - (5)].exp_node), (yyvsp[(3) - (5)].str_val), 1); ;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 263 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 267 "php_parser.y"
    { (yyval.str_val)[0] = 0; ;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 273 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (4)].syn_node); ;}
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 278 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (4)].syn_node); ;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 282 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 286 "php_parser.y"
    { (yyval.syn_node) = add_branch_2_elseif((yyvsp[(1) - (6)].syn_node), make_ifelse_syn_node((yyvsp[(4) - (6)].exp_node), (yyvsp[(6) - (6)].syn_node), 0, 0)); ;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 287 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 291 "php_parser.y"
    { (yyval.syn_node) = 0; ;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 292 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (2)].syn_node); ;}
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 296 "php_parser.y"
    { (yyval.syn_node) = (yyvsp[(2) - (4)].syn_node); ;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 300 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (3)].exp_node); ;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 301 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(3) - (4)].exp_node); ;}
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 302 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (4)].exp_node); ;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 303 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(3) - (5)].exp_node); ;}
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 307 "php_parser.y"
    {  (yyval.exp_node) = 0; ;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 308 "php_parser.y"
    {
			(yyvsp[(2) - (4)].exp_node)->tree_node.syn_right = (yyvsp[(4) - (4)].syn_node);
			if ( (yyvsp[(1) - (4)].exp_node) ) {
				PHP_EXP_NODE *last = (yyvsp[(1) - (4)].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[(2) - (4)].exp_node);
				(yyval.exp_node) = (yyvsp[(1) - (4)].exp_node);
			} else {
				(yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0);
				(yyval.exp_node)->exp_node = (yyvsp[(2) - (4)].exp_node);
			}
		;}
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 323 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LIST, (yyvsp[(2) - (2)].exp_node), 0); ;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 324 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LIST, 0, 0); ;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 335 "php_parser.y"
    { (yyval.exp_node) = make_known_const((yyvsp[(1) - (1)].str_val)); ;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 339 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str((yyvsp[(1) - (3)].str_val), 0), make_const_exp_str((yyvsp[(3) - (3)].str_val), 0)); ;}
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 340 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OBJECT_DEREF, (yyvsp[(1) - (3)].exp_node), make_const_exp_str((yyvsp[(3) - (3)].str_val), 0)); ;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 345 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[(1) - (3)].exp_node), 0); ;}
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 346 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[(1) - (4)].exp_node), (yyvsp[(3) - (4)].exp_node));;}
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 347 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[(1) - (4)].exp_node), (yyvsp[(3) - (4)].exp_node));;}
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 348 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[(3) - (4)].exp_node)); ;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 352 "php_parser.y"
    { (yyval.exp_node) = make_func_call_exp((yyvsp[(1) - (4)].str_val), (yyvsp[(3) - (4)].exp_node)); ;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 353 "php_parser.y"
    { ;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 354 "php_parser.y"
    { ;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 357 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[(1) - (1)].exp_node), 0); ;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 358 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[(2) - (2)].exp_node), 1); ;}
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 359 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(1) - (3)].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[(3) - (3)].exp_node), 0); ;}
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 360 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(1) - (4)].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[(4) - (4)].exp_node), 1); ;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 361 "php_parser.y"
    { (yyval.exp_node) = make_func_call_param_list(); ;}
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 366 "php_parser.y"
    { ;}
    break;

  case 98:

/* Line 1464 of yacc.c  */
#line 368 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 99:

/* Line 1464 of yacc.c  */
#line 369 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(1) - (1)].exp_node); ;}
    break;

  case 100:

/* Line 1464 of yacc.c  */
#line 370 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_MAKE_REF, (yyvsp[(1) - (4)].exp_node), (yyvsp[(4) - (4)].exp_node)); ;}
    break;

  case 101:

/* Line 1464 of yacc.c  */
#line 375 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 376 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 377 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_MUL, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 378 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_DIV, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 379 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_CAT, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 380 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_REM, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 381 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_AND, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 382 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_OR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 383 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_XOR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 384 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_SHL, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 385 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (3)].exp_node), make_exp_2(PHP_OP_SHR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node))); ;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 387 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (2)].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[(1) - (2)].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 388 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(2) - (2)].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[(2) - (2)].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 114:

/* Line 1464 of yacc.c  */
#line 389 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(1) - (2)].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[(1) - (2)].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 390 "php_parser.y"
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[(2) - (2)].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[(2) - (2)].exp_node), make_const_exp_dnum(1))); ;}
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 392 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 393 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 394 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 395 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 396 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_XOR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 397 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_OR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 122:

/* Line 1464 of yacc.c  */
#line 398 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_AND, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 399 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_XOR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 400 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_CAT, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 401 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ADD, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 402 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 403 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUL, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 404 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_DIV, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 405 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_REM, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 406 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHL, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 407 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 408 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (2)].exp_node); ;}
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 409 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 410 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_LOG_NOT, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 411 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_NOT, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 412 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_SAME, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 413 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NOT_SAME, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 414 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 415 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_NEQ, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 416 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 417 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR_EQ, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 418 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 419 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT_EQ, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 420 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (3)].exp_node); ;}
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 421 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUX, (yyvsp[(3) - (5)].exp_node), (yyvsp[(5) - (5)].exp_node)); (yyval.exp_node)->exp_node = (yyvsp[(1) - (5)].exp_node); ;}
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 422 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_INT, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 423 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_FLOAT, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 424 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_STR, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 425 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_BOOL, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 428 "php_parser.y"
    {  ;}
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 429 "php_parser.y"
    {  ;}
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 430 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (2)].exp_node); ;}
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 432 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(1) - (1)].exp_node); ;}
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 433 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY, (yyvsp[(3) - (4)].exp_node)); ;}
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 434 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_PRINT, (yyvsp[(2) - (2)].exp_node)); ;}
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 437 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(2) - (3)].exp_node); ;}
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 438 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 439 "php_parser.y"
    { (yyval.exp_node) = 0; ;}
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 447 "php_parser.y"
    { /*$$ = make_assign_node($1);*/ ;}
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 448 "php_parser.y"
    { (yyval.exp_node) = (yyvsp[(3) - (4)].exp_node); ;}
    break;

  case 163:

/* Line 1464 of yacc.c  */
#line 449 "php_parser.y"
    { /*$$ = make_assign_node(0);*/ ;}
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 452 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0); (yyval.exp_node)->exp_node = (yyvsp[(1) - (1)].exp_node); ;}
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 453 "php_parser.y"
    {
				PHP_EXP_NODE *last = (yyvsp[(1) - (3)].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[(3) - (3)].exp_node);
				(yyval.exp_node) = (yyvsp[(1) - (3)].exp_node);
			;}
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 462 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_PAIR, (yyvsp[(1) - (1)].exp_node)); ;}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 463 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_PAIR, (yyvsp[(1) - (3)].exp_node), (yyvsp[(3) - (3)].exp_node)); ;}
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 464 "php_parser.y"
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_REF_PAIR, (yyvsp[(1) - (4)].exp_node), (yyvsp[(4) - (4)].exp_node)); ;}
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 465 "php_parser.y"
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_REF_PAIR, (yyvsp[(2) - (2)].exp_node)); ;}
    break;



/* Line 1464 of yacc.c  */
#line 3485 "php_parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
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
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



