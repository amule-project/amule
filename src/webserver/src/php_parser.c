/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         phpparse
#define yylex           phplex
#define yyerror         phperror
#define yydebug         phpdebug
#define yynerrs         phpnerrs

#define yylval          phplval
#define yychar          phpchar

/* Copy the first part of user declarations.  */
#line 1 "php_parser.y" /* yacc.c:339  */

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

int phplex(void);

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


#line 140 "php_parser.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "php_parser.h".  */
#ifndef YY_PHP_PHP_PARSER_H_INCLUDED
# define YY_PHP_PHP_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int phpdebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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
    PLUS_EQ = 299,
    MINUS_EQ = 300,
    MUL_EQ = 301,
    DIV_EQ = 302,
    CONCAT_EQ = 303,
    MOD_EQ = 304,
    AND_EQ = 305,
    OR_EQ = 306,
    XOR_EQ = 307,
    SL_EQ = 308,
    SR_EQ = 309,
    LOG_OR = 310,
    LOG_XOR = 311,
    LOG_AND = 312,
    BOOLEAN_OR = 313,
    BOOLEAN_AND = 314,
    IS_EQ = 315,
    IS_NOEQUAL = 316,
    IS_IDENTICAL = 317,
    IS_NOIDENTICAL = 318,
    IS_SMALLER_OR_EQ = 319,
    IS_GREATER_OR_EQ = 320,
    SL = 321,
    SR = 322,
    INSTANCEOF = 323,
    INC = 324,
    DEC = 325,
    INT_CAST = 326,
    DOUBLE_CAST = 327,
    STRING_CAST = 328,
    ARRAY_CAST = 329,
    OBJECT_CAST = 330,
    BOOL_CAST = 331,
    UNSET_CAST = 332,
    NEW = 333,
    CLONE = 334,
    ELSEIF = 335,
    ELSE = 336,
    ENDIF = 337,
    STATIC = 338,
    ABSTRACT = 339,
    FINAL = 340,
    PRIVATE = 341,
    PROTECTED = 342,
    PUBLIC = 343,
    START_SCRIPT = 344,
    END_SCRIPT = 345
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 67 "php_parser.y" /* yacc.c:355  */

	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];

#line 278 "php_parser.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE phplval;

int phpparse (void);

#endif /* !YY_PHP_PHP_PARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 295 "php_parser.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3048

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  117
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  37
/* YYNRULES -- Number of rules.  */
#define YYNRULES  169
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  385

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   345

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,     2,     2,   116,    82,    66,     2,
     113,   114,    80,    77,    43,    78,    79,    81,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    58,   112,
      71,    45,    73,    57,    95,     2,     2,     2,     2,     2,
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
      59,    60,    61,    62,    63,    67,    68,    69,    70,    72,
      74,    75,    76,    84,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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

#if YYDEBUG || YYERROR_VERBOSE || 0
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
  "LIST", "ARRAY", "CLASS_SCOPE", "','", "PRINT", "'='", "PLUS_EQ",
  "MINUS_EQ", "MUL_EQ", "DIV_EQ", "CONCAT_EQ", "MOD_EQ", "AND_EQ", "OR_EQ",
  "XOR_EQ", "SL_EQ", "SR_EQ", "'?'", "':'", "LOG_OR", "LOG_XOR", "LOG_AND",
  "BOOLEAN_OR", "BOOLEAN_AND", "'|'", "'^'", "'&'", "IS_EQ", "IS_NOEQUAL",
  "IS_IDENTICAL", "IS_NOIDENTICAL", "'<'", "IS_SMALLER_OR_EQ", "'>'",
  "IS_GREATER_OR_EQ", "SL", "SR", "'+'", "'-'", "'.'", "'*'", "'/'", "'%'",
  "'!'", "INSTANCEOF", "'~'", "INC", "DEC", "INT_CAST", "DOUBLE_CAST",
  "STRING_CAST", "ARRAY_CAST", "OBJECT_CAST", "BOOL_CAST", "UNSET_CAST",
  "'@'", "'['", "NEW", "CLONE", "ELSEIF", "ELSE", "ENDIF", "STATIC",
  "ABSTRACT", "FINAL", "PRIVATE", "PROTECTED", "PUBLIC", "START_SCRIPT",
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
  "array_elem", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,    44,   298,    61,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,    63,    58,   310,
     311,   312,   313,   314,   124,    94,    38,   315,   316,   317,
     318,    60,   319,    62,   320,   321,   322,    43,    45,    46,
      42,    47,    37,    33,   323,   126,   324,   325,   326,   327,
     328,   329,   330,   331,   332,    64,    91,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     123,   125,    59,    40,    41,    93,    36
};
# endif

#define YYPACT_NINF -335

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-335)))

#define YYTABLE_NINF -53

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-53)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -94,  -335,    54,   344,  -335,  -335,  -335,  -335,   -32,  -335,
    2566,   -64,   -57,  1940,   -49,   -44,   -37,    42,    59,    70,
      78,  1996,  2054,    14,  2110,    88,    90,  2566,  2566,  2566,
    2566,  2566,    34,    34,  2566,  2566,  2566,  2566,  2566,  2566,
      61,  -335,  -335,  -335,  -335,  2566,    56,  -335,  -335,  -335,
    -335,   123,    49,  -335,  1356,   157,  2168,     0,  2837,  2224,
    -335,  2566,   192,  2566,  2566,  2566,   205,  -335,    36,  -335,
      34,  2566,  -335,  2624,  -335,  2650,  -335,   208,  -335,  2678,
      31,  2282,  2837,   118,   118,  -335,  -335,   173,  -335,   -36,
    -335,  -335,  -335,  -335,  -335,  -335,  -335,   171,    50,  -335,
     458,   572,   558,  2566,  2338,  2566,  2566,  2566,  2566,  2566,
    2566,  2566,  2566,  2566,  2566,  2566,  -335,  -335,   211,   212,
    2396,  2566,  2566,  2566,  2566,  2566,  2566,  2566,  2566,  2566,
    2566,  2566,  2566,  2566,  2566,  2566,  2566,  2566,  2566,  2566,
    2566,  2566,  2566,  2566,  2566,  2566,  2566,  -335,  -335,    34,
     -31,  2837,  2566,  -335,  -335,   672,   786,   106,   900,   177,
     109,   451,   178,    -1,    59,  -335,    27,  -335,  1014,  -335,
    -335,   111,   113,  -335,   116,  -335,    30,  -335,    34,  2785,
      35,  -335,   216,   191,    61,  -335,  -335,  -335,  -335,  2704,
      34,  2837,  2837,  2837,  2837,  2837,  2837,  2837,  2837,  2837,
    2837,  2837,  2837,   117,   124,  -335,   330,  2732,  2811,  2884,
    2906,  2927,  2947,  2966,  1478,  1591,  1704,  1818,  1818,  1818,
    1818,   202,   202,   202,   202,   108,   108,   118,   118,   118,
    -335,  -335,  -335,  -335,  2452,  -335,  2837,  -335,  1940,  2566,
    1484,  2566,    34,   191,   225,  1598,  -335,    34,   126,   -47,
     146,   146,    31,    31,   194,  -335,  2510,  2282,  -335,  -335,
    -335,  -335,  -335,  -335,  -335,  2168,  2168,  -335,  -335,  2566,
      34,  2837,  -335,  1128,  -335,  -335,  -335,   128,   -30,  -335,
     197,  -335,  -335,  -335,  -335,   131,   133,  -335,  -335,    43,
     136,    46,    51,  -335,  2566,    34,  2837,  -335,    53,    67,
    2861,  -335,   -73,   134,   686,  2566,    45,  1712,   191,   800,
    -335,    17,  -335,   -23,   230,   137,  -335,   241,   142,  -335,
    2837,  -335,  -335,  -335,   140,  1940,  -335,  -335,   144,   150,
      34,   151,  -335,  -335,  -335,  -335,  -335,   179,   149,  2566,
    -335,   -45,   -21,  -335,   147,  -335,  -335,  -335,  2566,  -335,
    -335,  1826,   152,  1712,   914,   158,  -335,  2837,  -335,  -335,
    -335,  -335,  -335,   251,  1028,  1142,  1242,  -335,  -335,  -335,
    1712,  -335,   159,  -335,  1370,  -335,  -335,  -335,  1940,  1256,
    -335,  -335,  -335,   174,  -335
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
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
     163,     0,   155,   132,   133,   134,   135,     0,   113,    80,
     115,   146,   147,   148,   149,   150,   152,    42,     0,    40,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,   114,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     9,    81,     0,
       0,    91,     0,    23,   157,     0,     0,     0,     0,    58,
       0,     0,     0,     0,     0,    10,     0,    35,     0,    20,
      18,     0,     0,    22,     0,   161,     0,   159,     0,   166,
       0,   164,     0,     0,     0,    11,     6,     8,   144,     0,
       0,    98,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,    82,     0,    84,     0,     0,     0,   118,
     120,   119,   116,   117,   121,   123,   122,   138,   139,   136,
     137,   140,   141,   142,   143,   130,   131,   125,   126,   124,
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

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -335,  -335,   -40,  -335,   -13,  -335,   259,  -335,  -335,   125,
    -335,   103,  -335,  -335,    39,   -19,  -335,  -334,  -149,  -335,
    -335,  -335,  -335,  -128,  -335,  -335,  -166,     2,    15,  -335,
    -204,    -6,  -335,    41,    38,  -335,    44
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,    47,    48,   163,   159,   166,    68,    69,
      98,    99,    49,   171,   289,   290,   369,   334,   160,   302,
     326,   276,   287,   311,   341,   360,    50,    51,    52,    53,
     150,    54,    60,   176,   177,   180,   181
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      62,   100,   182,   101,    58,   339,   340,   339,   340,   306,
      55,   285,   234,   358,     1,    73,    75,   261,    79,   371,
      76,    82,    83,    84,    85,    86,   324,   325,    91,    92,
      93,    94,    95,    96,    88,    90,   380,    87,     9,   102,
      87,     9,   244,   152,   338,   339,   340,    89,    89,    59,
     151,    87,     9,   155,     4,   156,    61,   158,    58,   161,
     120,   298,   299,   286,    63,   168,    67,   359,    97,    64,
     247,   174,   167,   253,   121,   179,    65,   279,   257,   164,
      77,    56,   175,   235,   307,    89,   314,   118,   343,   314,
     361,   119,   277,   184,   253,    89,   234,   189,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   202,
     234,   330,   153,   245,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   248,   335,   316,   254,   120,   236,    46,   165,   258,
      46,   233,   288,   -52,   362,    66,   329,   315,   313,   121,
     318,    46,   185,   148,    89,   319,   103,   322,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     255,   323,   337,    70,   342,   141,   142,   143,   144,   145,
     146,    71,   264,    89,     5,     6,     7,   260,   144,   145,
     146,    80,   317,    81,   157,    89,   355,   339,   340,   116,
     117,   162,   -52,   363,   172,    55,   183,   203,   204,   239,
     152,   241,   259,   243,   250,   272,   251,   275,   271,   252,
     265,   280,   282,   273,   304,    58,   288,   266,   284,   294,
     305,   309,   308,   310,   278,   312,   327,   345,   346,   283,
     296,   179,   347,   348,   175,   175,   350,    89,   375,   151,
     151,   356,    89,   300,   351,   353,   370,    89,    89,    57,
     373,   381,   301,   -53,   -53,   -53,   -53,   139,   140,   141,
     142,   143,   144,   145,   146,    89,   384,   262,   320,   246,
     291,   293,   354,   292,   333,   344,     0,   321,     0,    58,
       0,   297,     0,     0,     0,   364,     0,   365,   331,     0,
      89,     0,   349,     0,     0,     0,     0,     0,     0,     0,
     374,    89,     0,     0,     0,     0,     0,   379,     0,     0,
       0,     0,   352,   357,     0,     0,     0,     0,   368,     0,
     333,     0,   366,     0,     0,    89,     0,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,   333,    15,     0,
      16,     0,    17,     0,     0,   382,    18,    19,     0,     0,
      20,     0,     0,     0,    21,    22,    23,    24,     0,     0,
       0,     0,     0,     0,    25,    26,     0,   122,    27,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,     0,     0,    37,    38,    39,
       0,     0,     0,     0,     0,   267,    40,     0,     0,     0,
       0,     0,    41,    42,    43,     0,    44,    45,     0,     0,
      46,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,   242,    15,     0,    16,     0,    17,     0,     0,     0,
      18,    19,     0,     0,    20,     0,     0,     0,    21,    22,
      23,    24,     0,     0,     0,     0,     0,     0,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,   122,     0,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,    28,    29,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,     0,
       0,    37,    38,    39,     0,     0,     0,     0,     0,     0,
      40,     0,     0,     0,     0,     0,    41,   186,    43,     0,
      44,    45,     0,     0,    46,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,    15,     0,    16,     0,
      17,     0,     0,     0,    18,    19,     0,     0,    20,     0,
       0,     0,    21,    22,    23,    24,     0,     0,     0,     0,
       0,     0,    25,    26,     0,   122,    27,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,     0,     0,     0,     0,     0,     0,     0,     0,    28,
      29,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,     0,     0,    37,    38,    39,     0,     0,
       0,     0,   188,     0,    40,     0,     0,     0,     0,     0,
      41,     0,    43,   187,    44,    45,     0,     0,    46,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,   328,
      15,     0,    16,     0,    17,     0,     0,     0,    18,    19,
       0,     0,    20,     0,     0,     0,    21,    22,    23,    24,
       0,     0,     0,     0,     0,     0,    25,    26,     0,   122,
      27,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,     0,     0,     0,     0,   237,     0,    40,     0,
       0,     0,     0,     0,    41,     0,    43,     0,    44,    45,
       0,     0,    46,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,     0,    15,     0,    16,     0,    17,   336,
       0,     0,    18,    19,     0,     0,    20,     0,     0,     0,
      21,    22,    23,    24,     0,     0,     0,     0,     0,     0,
      25,    26,     0,   122,    27,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    29,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,     0,     0,    37,    38,    39,     0,     0,     0,     0,
     238,     0,    40,     0,     0,     0,     0,     0,    41,     0,
      43,     0,    44,    45,     0,     0,    46,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,     0,    15,     0,
      16,   372,    17,     0,     0,     0,    18,    19,     0,     0,
      20,     0,     0,     0,    21,    22,    23,    24,     0,     0,
       0,     0,     0,     0,    25,    26,     0,   122,    27,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,     0,     0,    37,    38,    39,
       0,     0,     0,     0,   240,     0,    40,     0,     0,     0,
       0,     0,    41,     0,    43,     0,    44,    45,     0,     0,
      46,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,    15,     0,    16,     0,    17,     0,     0,     0,
      18,    19,     0,     0,    20,     0,     0,     0,    21,    22,
      23,    24,     0,     0,     0,     0,     0,     0,    25,    26,
       0,   122,    27,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,     0,
       0,     0,     0,     0,     0,    28,    29,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,     0,
       0,    37,    38,    39,     0,     0,     0,     0,   249,     0,
      40,     0,     0,     0,     0,     0,    41,     0,    43,   376,
      44,    45,     0,     0,    46,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,    15,     0,    16,     0,
      17,     0,     0,     0,    18,    19,     0,     0,    20,     0,
       0,     0,    21,    22,    23,    24,     0,     0,     0,     0,
       0,     0,    25,    26,     0,   122,    27,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,     0,     0,     0,     0,     0,     0,     0,     0,    28,
      29,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,     0,     0,    37,    38,    39,     0,     0,
       0,     0,   303,     0,    40,     0,     0,     0,     0,     0,
      41,     0,    43,   377,    44,    45,     0,     0,    46,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,     0,
      15,   383,    16,     0,    17,     0,     0,     0,    18,    19,
       0,     0,    20,     0,     0,     0,    21,    22,    23,    24,
       0,     0,     0,     0,     0,     0,    25,    26,     0,   122,
      27,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,     0,     0,     0,     0,   378,     0,    40,     0,
       0,     0,     0,     0,    41,     0,    43,     0,    44,    45,
       0,     0,    46,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,     0,    15,     0,    16,     0,    17,     0,
       0,     0,    18,    19,     0,     0,    20,     0,     0,     0,
      21,    22,    23,    24,     0,     0,     0,     0,     0,     0,
      25,    26,     0,   122,    27,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    29,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,     0,     0,    37,    38,    39,     0,     0,   147,     0,
       0,     0,    40,     0,     0,     0,     0,     0,    41,     0,
      43,     0,    44,    45,     0,     0,    46,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,     0,    15,     0,
      16,     0,    17,     0,     0,     0,    18,    19,     0,     0,
      20,     0,     0,     0,    21,    22,     0,    24,     0,     0,
       0,     0,     0,     0,    25,    26,     0,     0,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   274,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,    28,    29,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,     0,     0,    37,    38,    39,
       0,     0,     0,     0,     0,     0,    40,     0,     0,     0,
       0,     0,     0,     0,    43,     0,    44,    45,     0,     0,
      46,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     0,    15,     0,    16,     0,    17,     0,     0,     0,
      18,    19,     0,     0,    20,     0,     0,     0,    21,    22,
       0,    24,     0,     0,     0,     0,     0,     0,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   281,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,     0,    28,    29,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,     0,
       0,    37,    38,    39,     0,     0,     0,     0,     0,     0,
      40,     0,     0,     0,     0,     0,     0,     0,    43,     0,
      44,    45,     0,     0,    46,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,     0,    15,     0,    16,     0,
      17,     0,     0,     0,    18,    19,     0,     0,    20,     0,
       0,     0,    21,    22,     0,    24,     0,     0,     0,     0,
       0,     0,    25,    26,     0,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     332,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,    28,
      29,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,     0,     0,    37,    38,    39,     0,     0,
       0,     0,     0,     0,    40,     0,     0,     0,     0,     0,
       0,     0,    43,     0,    44,    45,     0,     0,    46,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,     0,
      15,     0,    16,     0,    17,     0,     0,     0,    18,    19,
       0,     0,    20,     0,     0,     0,    21,    22,     0,    24,
       0,     0,     0,     0,     0,     0,    25,    26,     0,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   367,   -53,   -53,   -53,   -53,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,     0,     0,     0,     0,     0,     0,    40,     0,
       0,     0,     0,     0,     0,     0,    43,     0,    44,    45,
       0,     0,    46,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,     0,    15,     0,    16,     0,    17,     0,
       0,     0,    18,    19,     0,     0,    20,     0,     0,     0,
      21,    22,     0,    24,     0,     0,     0,     0,     0,     0,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     5,
       6,     7,     8,     9,     0,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    29,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,     0,     0,    37,    38,    39,    25,    26,     0,     0,
      27,     0,    40,     0,     0,     0,     0,     0,     0,     0,
      43,     0,    44,    45,     0,     0,    46,     5,     6,     7,
       8,     9,     0,    11,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,     0,     0,    25,    26,     0,     0,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    72,    45,
       0,     0,    46,     5,     6,     7,     8,     9,     0,    11,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,     0,     0,    37,    38,    39,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    74,    45,     0,     0,
      46,     5,     6,     7,     8,     9,     0,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    29,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,     0,     0,    37,    38,    39,     0,     0,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    78,    45,     0,     0,    46,     5,     6,     7,
       8,     9,     0,    11,   149,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    28,    29,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,     0,
       0,    37,    38,    39,    25,    26,     0,     0,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    45,     0,     0,    46,     5,     6,     7,     8,     9,
       0,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,     0,    30,     0,    31,
      32,    33,    34,    35,    36,     0,     0,    37,    38,    39,
       0,     0,    25,    26,     0,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    45,   154,     0,
      46,     5,     6,     7,     8,     9,     0,    11,   178,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    28,
      29,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,     0,     0,    37,    38,    39,    25,    26,
       0,     0,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,     0,     0,    46,     5,
       6,     7,     8,     9,   190,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    28,    29,     0,     0,     0,
       0,    30,     0,    31,    32,    33,    34,    35,    36,     0,
       0,    37,    38,    39,     0,     0,    25,    26,     0,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    45,     0,     0,    46,     5,     6,     7,     8,     9,
       0,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,    25,    26,     0,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
       0,   205,    46,     5,     6,     7,     8,     9,   270,    11,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    28,
      29,     0,     0,     0,     0,    30,     0,    31,    32,    33,
      34,    35,    36,     0,     0,    37,    38,    39,     0,     0,
      25,    26,     0,     0,    27,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,     0,     0,    46,     5,
       6,     7,     8,     9,     0,    11,   295,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    29,     0,
       0,     0,     0,    30,     0,    31,    32,    33,    34,    35,
      36,     0,     0,    37,    38,    39,    25,    26,     0,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    45,     0,     0,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    28,    29,     0,     0,     0,     0,    30,
       0,    31,    32,    33,    34,    35,    36,     0,     0,    37,
      38,    39,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    45,
       0,   122,    46,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   122,     0,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,     0,     0,   122,   169,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   122,   170,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,   122,
     173,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   263,     0,     0,     0,     0,
       0,     0,     0,     0,   256,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   122,   268,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   122,   269,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   122,     0,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146
};

static const yytype_int16 yycheck[] =
{
      13,    41,    38,    43,    10,    28,    29,    28,    29,    39,
      42,    58,    43,    58,   108,    21,    22,   183,    24,   353,
       6,    27,    28,    29,    30,    31,    99,   100,    34,    35,
      36,    37,    38,    39,    32,    33,   370,     6,     7,    45,
       6,     7,    43,    43,    27,    28,    29,    32,    33,   113,
      56,     6,     7,    59,     0,    61,   113,    63,    64,    65,
      96,   265,   266,   110,   113,    71,     7,   112,     7,   113,
      43,    40,    70,    43,   110,    81,   113,   243,    43,    43,
      66,   113,    80,   114,   114,    70,    43,    38,   111,    43,
     111,    42,   241,    43,    43,    80,    43,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
      43,    66,   112,   114,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   114,   308,     7,   114,    96,   152,   116,   112,   114,
     116,   149,     6,     7,     7,   113,   305,   114,   286,   110,
     114,   116,   112,     6,   149,   114,   110,   114,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
     178,   114,   310,   113,   312,    77,    78,    79,    80,    81,
      82,   113,   190,   178,     3,     4,     5,     6,    80,    81,
      82,   113,    66,   113,    12,   190,    27,    28,    29,    86,
      87,     6,    66,    66,     6,    42,    45,     6,     6,   113,
      43,   112,     6,    45,   113,   238,   113,   240,   234,   113,
     113,     6,   245,   239,   274,   241,     6,   113,   112,    45,
     112,   281,    45,   112,   242,   112,   112,   110,     7,   247,
     256,   257,   110,   113,   252,   253,   112,   242,     7,   265,
     266,   112,   247,   269,   114,   114,   114,   252,   253,    10,
     112,   112,   270,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   270,   112,   184,   294,   164,
     251,   253,   332,   252,   307,   314,    -1,   295,    -1,   305,
      -1,   257,    -1,    -1,    -1,   345,    -1,   347,   306,    -1,
     295,    -1,   325,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     360,   306,    -1,    -1,    -1,    -1,    -1,   367,    -1,    -1,
      -1,    -1,   330,   339,    -1,    -1,    -1,    -1,   351,    -1,
     353,    -1,   348,    -1,    -1,   330,    -1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,   370,    14,    -1,
      16,    -1,    18,    -1,    -1,   378,    22,    23,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    32,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    57,    44,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    -1,    -1,    93,    94,    95,
      -1,    -1,    -1,    -1,    -1,   115,   102,    -1,    -1,    -1,
      -1,    -1,   108,   109,   110,    -1,   112,   113,    -1,    -1,
     116,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    20,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,
      32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    57,    -1,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    -1,
      -1,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
     102,    -1,    -1,    -1,    -1,    -1,   108,   109,   110,    -1,
     112,   113,    -1,    -1,   116,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    -1,    16,    -1,
      18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,
      -1,    -1,    30,    31,    32,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    -1,    57,    44,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    95,    -1,    -1,
      -1,    -1,   114,    -1,   102,    -1,    -1,    -1,    -1,    -1,
     108,    -1,   110,   111,   112,   113,    -1,    -1,   116,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,
      -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    57,
      44,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    -1,    -1,    -1,    -1,   114,    -1,   102,    -1,
      -1,    -1,    -1,    -1,   108,    -1,   110,    -1,   112,   113,
      -1,    -1,   116,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    16,    -1,    18,    19,
      -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,
      30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    -1,    57,    44,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    -1,    -1,    93,    94,    95,    -1,    -1,    -1,    -1,
     114,    -1,   102,    -1,    -1,    -1,    -1,    -1,   108,    -1,
     110,    -1,   112,   113,    -1,    -1,   116,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    14,    -1,
      16,    17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    32,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    57,    44,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    -1,    -1,    93,    94,    95,
      -1,    -1,    -1,    -1,   114,    -1,   102,    -1,    -1,    -1,
      -1,    -1,   108,    -1,   110,    -1,   112,   113,    -1,    -1,
     116,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,
      32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,
      -1,    57,    44,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    -1,
      -1,    93,    94,    95,    -1,    -1,    -1,    -1,   114,    -1,
     102,    -1,    -1,    -1,    -1,    -1,   108,    -1,   110,   111,
     112,   113,    -1,    -1,   116,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    -1,    16,    -1,
      18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,
      -1,    -1,    30,    31,    32,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    -1,    57,    44,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    95,    -1,    -1,
      -1,    -1,   114,    -1,   102,    -1,    -1,    -1,    -1,    -1,
     108,    -1,   110,   111,   112,   113,    -1,    -1,   116,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    15,    16,    -1,    18,    -1,    -1,    -1,    22,    23,
      -1,    -1,    26,    -1,    -1,    -1,    30,    31,    32,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    57,
      44,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    -1,    -1,    -1,    -1,   114,    -1,   102,    -1,
      -1,    -1,    -1,    -1,   108,    -1,   110,    -1,   112,   113,
      -1,    -1,   116,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,
      -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,
      30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    -1,    57,    44,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    -1,    -1,    93,    94,    95,    -1,    -1,   112,    -1,
      -1,    -1,   102,    -1,    -1,    -1,    -1,    -1,   108,    -1,
     110,    -1,   112,   113,    -1,    -1,   116,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    14,    -1,
      16,    -1,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    -1,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    40,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    -1,    -1,    93,    94,    95,
      -1,    -1,    -1,    -1,    -1,    -1,   102,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   110,    -1,   112,   113,    -1,    -1,
     116,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    -1,    16,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    26,    -1,    -1,    -1,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    58,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    -1,
      -1,    93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
     102,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   110,    -1,
     112,   113,    -1,    -1,   116,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    -1,    16,    -1,
      18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,    -1,
      -1,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    95,    -1,    -1,
      -1,    -1,    -1,    -1,   102,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   110,    -1,   112,   113,    -1,    -1,   116,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    -1,    16,    -1,    18,    -1,    -1,    -1,    22,    23,
      -1,    -1,    26,    -1,    -1,    -1,    30,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    41,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,   102,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   110,    -1,   112,   113,
      -1,    -1,   116,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    16,    -1,    18,    -1,
      -1,    -1,    22,    23,    -1,    -1,    26,    -1,    -1,    -1,
      30,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    -1,    -1,    93,    94,    95,    40,    41,    -1,    -1,
      44,    -1,   102,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     110,    -1,   112,   113,    -1,    -1,   116,     3,     4,     5,
       6,     7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    -1,    -1,    40,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,   113,
      -1,    -1,   116,     3,     4,     5,     6,     7,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    -1,    -1,    93,    94,    95,
      40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   112,   113,    -1,    -1,
     116,     3,     4,     5,     6,     7,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    -1,    -1,    93,    94,    95,    -1,    -1,    40,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   112,   113,    -1,    -1,   116,     3,     4,     5,
       6,     7,    -1,     9,    66,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    -1,
      -1,    93,    94,    95,    40,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,    -1,    -1,   116,     3,     4,     5,     6,     7,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    78,    -1,    -1,    -1,    -1,    83,    -1,    85,
      86,    87,    88,    89,    90,    -1,    -1,    93,    94,    95,
      -1,    -1,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,   114,    -1,
     116,     3,     4,     5,     6,     7,    -1,     9,    66,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    95,    40,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,    -1,    -1,   116,     3,
       4,     5,     6,     7,    66,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    77,    78,    -1,    -1,    -1,
      -1,    83,    -1,    85,    86,    87,    88,    89,    90,    -1,
      -1,    93,    94,    95,    -1,    -1,    40,    41,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,    -1,    -1,   116,     3,     4,     5,     6,     7,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    40,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
      -1,   115,   116,     3,     4,     5,     6,     7,    66,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,
      78,    -1,    -1,    -1,    -1,    83,    -1,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    95,    -1,    -1,
      40,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,    -1,    -1,   116,     3,
       4,     5,     6,     7,    -1,     9,    66,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    77,    78,    -1,
      -1,    -1,    -1,    83,    -1,    85,    86,    87,    88,    89,
      90,    -1,    -1,    93,    94,    95,    40,    41,    -1,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   113,    -1,    -1,   116,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    83,
      -1,    85,    86,    87,    88,    89,    90,    -1,    -1,    93,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
      -1,    57,   116,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    57,    -1,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    57,   112,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    57,   112,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    -1,    -1,    57,
     112,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    57,   111,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    57,    -1,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   108,   118,   119,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    14,    16,    18,    22,    23,
      26,    30,    31,    32,    33,    40,    41,    44,    77,    78,
      83,    85,    86,    87,    88,    89,    90,    93,    94,    95,
     102,   108,   109,   110,   112,   113,   116,   120,   121,   129,
     143,   144,   145,   146,   148,    42,   113,   123,   148,   113,
     149,   113,   121,   113,   113,   113,   113,     7,   125,   126,
     113,   113,   112,   148,   112,   148,     6,    66,   112,   148,
     113,   113,   148,   148,   148,   148,   148,     6,   144,   145,
     144,   148,   148,   148,   148,   148,   148,     7,   127,   128,
     119,   119,   148,   110,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    86,    87,    38,    42,
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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 142 "php_parser.y" /* yacc.c:1646  */
    { g_syn_tree_top = (yyvsp[-1].syn_node); }
#line 2181 "php_parser.c" /* yacc.c:1646  */
    break;

  case 3:
#line 146 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = add_statement_2_list((yyvsp[-1].syn_node), (yyvsp[0].syn_node)); }
#line 2187 "php_parser.c" /* yacc.c:1646  */
    break;

  case 4:
#line 147 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2193 "php_parser.c" /* yacc.c:1646  */
    break;

  case 6:
#line 153 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-1].syn_node); }
#line 2199 "php_parser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 159 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-1].syn_node); }
#line 2205 "php_parser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 160 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_EXPR, (yyvsp[-1].exp_node)); }
#line 2211 "php_parser.c" /* yacc.c:1646  */
    break;

  case 10:
#line 161 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2217 "php_parser.c" /* yacc.c:1646  */
    break;

  case 11:
#line 162 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-1].syn_node); }
#line 2223 "php_parser.c" /* yacc.c:1646  */
    break;

  case 12:
#line 163 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_ifelse_syn_node((yyvsp[-4].exp_node), (yyvsp[-2].syn_node), (yyvsp[-1].syn_node), (yyvsp[0].syn_node)); }
#line 2229 "php_parser.c" /* yacc.c:1646  */
    break;

  case 13:
#line 164 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 1); }
#line 2235 "php_parser.c" /* yacc.c:1646  */
    break;

  case 14:
#line 165 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_while_loop_syn_node((yyvsp[-2].exp_node), (yyvsp[-5].syn_node), 0); }
#line 2241 "php_parser.c" /* yacc.c:1646  */
    break;

  case 15:
#line 166 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_for_syn_node((yyvsp[-6].exp_node), (yyvsp[-4].exp_node), (yyvsp[-2].exp_node), (yyvsp[0].syn_node)); }
#line 2247 "php_parser.c" /* yacc.c:1646  */
    break;

  case 16:
#line 167 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_switch_syn_node((yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2253 "php_parser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 168 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, 0); }
#line 2259 "php_parser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 169 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_CONTINUE, (yyvsp[-1].exp_node)); }
#line 2265 "php_parser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 170 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, 0); }
#line 2271 "php_parser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 171 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_BREAK, (yyvsp[-1].exp_node)); }
#line 2277 "php_parser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 172 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, 0); }
#line 2283 "php_parser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 173 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_RET, (yyvsp[-1].exp_node)); }
#line 2289 "php_parser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 174 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = make_expr_syn_node(PHP_ST_ECHO, (yyvsp[-1].exp_node)); }
#line 2295 "php_parser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 175 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2301 "php_parser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 176 "php_parser.y" /* yacc.c:1646  */
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-4].exp_node), 0, (yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0);
			}
#line 2309 "php_parser.c" /* yacc.c:1646  */
    break;

  case 26:
#line 179 "php_parser.y" /* yacc.c:1646  */
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-6].exp_node), (yyvsp[-4].exp_node), (yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0);
			}
#line 2317 "php_parser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 182 "php_parser.y" /* yacc.c:1646  */
    {
				(yyval.syn_node) = make_foreach_loop_syn_node((yyvsp[-7].exp_node), (yyvsp[-5].exp_node), (yyvsp[-2].exp_node), (yyvsp[0].syn_node), 1);
			}
#line 2325 "php_parser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 185 "php_parser.y" /* yacc.c:1646  */
    { }
#line 2331 "php_parser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 186 "php_parser.y" /* yacc.c:1646  */
    { }
#line 2337 "php_parser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 187 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2343 "php_parser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 190 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2349 "php_parser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 191 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2355 "php_parser.c" /* yacc.c:1646  */
    break;

  case 33:
#line 194 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0); (yyval.exp_node)->exp_node = (yyvsp[0].exp_node); }
#line 2361 "php_parser.c" /* yacc.c:1646  */
    break;

  case 34:
#line 195 "php_parser.y" /* yacc.c:1646  */
    {
				PHP_EXP_NODE *last = (yyvsp[-2].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[0].exp_node);
				(yyval.exp_node) = (yyvsp[-2].exp_node);
			}
#line 2373 "php_parser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 211 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2379 "php_parser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 212 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2385 "php_parser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 216 "php_parser.y" /* yacc.c:1646  */
    {
		const char *varname = get_scope_var_name(g_current_scope, (yyvsp[0].exp_node)->var_si_node->var);
		PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, varname);
		PHP_SCOPE_ITEM *gsi = get_scope_item(g_global_scope, varname);
		if ( gsi && (gsi->type == PHP_SCOPE_VAR) ) {
			free_var_node(si->var);
			php_exp_tree_free((yyvsp[0].exp_node));
			gsi->var->ref_count++;
			si->var = gsi->var;
		} else {
			php_report_error(PHP_ERROR, "There is no such global var");
		}
	}
#line 2403 "php_parser.c" /* yacc.c:1646  */
    break;

  case 40:
#line 231 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2409 "php_parser.c" /* yacc.c:1646  */
    break;

  case 41:
#line 232 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2415 "php_parser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 235 "php_parser.y" /* yacc.c:1646  */
    { (yyvsp[0].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[0].exp_node); }
#line 2421 "php_parser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 236 "php_parser.y" /* yacc.c:1646  */
    {
			(yyvsp[-2].exp_node)->var_node->flags |= PHP_VARFLAG_STATIC; (yyval.exp_node) = (yyvsp[-2].exp_node);
			value_value_assign(&(yyvsp[-2].exp_node)->var_node->value, &(yyvsp[0].exp_node)->val_node);
		}
#line 2430 "php_parser.c" /* yacc.c:1646  */
    break;

  case 44:
#line 244 "php_parser.y" /* yacc.c:1646  */
    {
				switch_push_scope_table(make_scope_table());
			}
#line 2438 "php_parser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 246 "php_parser.y" /* yacc.c:1646  */
    {
				(yyval.syn_node) = make_func_decl_syn_node((yyvsp[-7].str_val), (yyvsp[-4].exp_node));
				(yyval.syn_node)->func_decl->scope = g_current_scope;
				(yyval.syn_node)->func_decl->is_native = 0;
				(yyval.syn_node)->func_decl->code = (yyvsp[-1].syn_node);
				switch_pop_scope_table(0);
				add_func_2_scope(g_current_scope, (yyval.syn_node));
				(yyval.syn_node) = 0;
			}
#line 2452 "php_parser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 255 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 2458 "php_parser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 259 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node), (yyvsp[-1].str_val), 0); }
#line 2464 "php_parser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 260 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_param(0, (yyvsp[0].exp_node), (yyvsp[-2].str_val), 1); }
#line 2470 "php_parser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 261 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_param((yyvsp[-3].exp_node), (yyvsp[0].exp_node), (yyvsp[-1].str_val), 0); }
#line 2476 "php_parser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 262 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_param((yyvsp[-4].exp_node), (yyvsp[0].exp_node), (yyvsp[-2].str_val), 1); }
#line 2482 "php_parser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 263 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = 0; }
#line 2488 "php_parser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 267 "php_parser.y" /* yacc.c:1646  */
    { (yyval.str_val)[0] = 0; }
#line 2494 "php_parser.c" /* yacc.c:1646  */
    break;

  case 55:
#line 273 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-2].syn_node); }
#line 2500 "php_parser.c" /* yacc.c:1646  */
    break;

  case 57:
#line 278 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-2].syn_node); }
#line 2506 "php_parser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 282 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = 0; }
#line 2512 "php_parser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 286 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = add_branch_2_elseif((yyvsp[-5].syn_node), make_ifelse_syn_node((yyvsp[-2].exp_node), (yyvsp[0].syn_node), 0, 0)); }
#line 2518 "php_parser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 287 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2524 "php_parser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 291 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = 0; }
#line 2530 "php_parser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 292 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[0].syn_node); }
#line 2536 "php_parser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 296 "php_parser.y" /* yacc.c:1646  */
    { (yyval.syn_node) = (yyvsp[-2].syn_node); }
#line 2542 "php_parser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 300 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-1].exp_node); }
#line 2548 "php_parser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 301 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-1].exp_node); }
#line 2554 "php_parser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 302 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-2].exp_node); }
#line 2560 "php_parser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 303 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-2].exp_node); }
#line 2566 "php_parser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 307 "php_parser.y" /* yacc.c:1646  */
    {  (yyval.exp_node) = 0; }
#line 2572 "php_parser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 308 "php_parser.y" /* yacc.c:1646  */
    {
			(yyvsp[-2].exp_node)->tree_node.syn_right = (yyvsp[0].syn_node);
			if ( (yyvsp[-3].exp_node) ) {
				PHP_EXP_NODE *last = (yyvsp[-3].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[-2].exp_node);
				(yyval.exp_node) = (yyvsp[-3].exp_node);
			} else {
				(yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0);
				(yyval.exp_node)->exp_node = (yyvsp[-2].exp_node);
			}
		}
#line 2590 "php_parser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 323 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LIST, (yyvsp[0].exp_node), 0); }
#line 2596 "php_parser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 324 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LIST, 0, 0); }
#line 2602 "php_parser.c" /* yacc.c:1646  */
    break;

  case 79:
#line 335 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_known_const((yyvsp[0].str_val)); }
#line 2608 "php_parser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 339 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str((yyvsp[-2].str_val), 0), make_const_exp_str((yyvsp[0].str_val), 0)); }
#line 2614 "php_parser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 340 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_OBJECT_DEREF, (yyvsp[-2].exp_node), make_const_exp_str((yyvsp[0].str_val), 0)); }
#line 2620 "php_parser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 345 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-2].exp_node), 0); }
#line 2626 "php_parser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 346 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));}
#line 2632 "php_parser.c" /* yacc.c:1646  */
    break;

  case 86:
#line 347 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_BY_KEY, (yyvsp[-3].exp_node), (yyvsp[-1].exp_node));}
#line 2638 "php_parser.c" /* yacc.c:1646  */
    break;

  case 87:
#line 348 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_VAR_BY_EXP, (yyvsp[-1].exp_node)); }
#line 2644 "php_parser.c" /* yacc.c:1646  */
    break;

  case 88:
#line 352 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_call_exp((yyvsp[-3].str_val), (yyvsp[-1].exp_node)); }
#line 2650 "php_parser.c" /* yacc.c:1646  */
    break;

  case 89:
#line 353 "php_parser.y" /* yacc.c:1646  */
    { }
#line 2656 "php_parser.c" /* yacc.c:1646  */
    break;

  case 90:
#line 354 "php_parser.y" /* yacc.c:1646  */
    { }
#line 2662 "php_parser.c" /* yacc.c:1646  */
    break;

  case 91:
#line 357 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); }
#line 2668 "php_parser.c" /* yacc.c:1646  */
    break;

  case 92:
#line 358 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_call_param_list(); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); }
#line 2674 "php_parser.c" /* yacc.c:1646  */
    break;

  case 93:
#line 359 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-2].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 0); }
#line 2680 "php_parser.c" /* yacc.c:1646  */
    break;

  case 94:
#line 360 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-3].exp_node); func_call_add_expr((yyval.exp_node)->var_node, (yyvsp[0].exp_node), 1); }
#line 2686 "php_parser.c" /* yacc.c:1646  */
    break;

  case 95:
#line 361 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_func_call_param_list(); }
#line 2692 "php_parser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 366 "php_parser.y" /* yacc.c:1646  */
    { }
#line 2698 "php_parser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 368 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ASS, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2704 "php_parser.c" /* yacc.c:1646  */
    break;

  case 99:
#line 369 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[0].exp_node); }
#line 2710 "php_parser.c" /* yacc.c:1646  */
    break;

  case 100:
#line 370 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_MAKE_REF, (yyvsp[-3].exp_node), (yyvsp[0].exp_node)); }
#line 2716 "php_parser.c" /* yacc.c:1646  */
    break;

  case 101:
#line 375 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2722 "php_parser.c" /* yacc.c:1646  */
    break;

  case 102:
#line 376 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2728 "php_parser.c" /* yacc.c:1646  */
    break;

  case 103:
#line 377 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2734 "php_parser.c" /* yacc.c:1646  */
    break;

  case 104:
#line 378 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2740 "php_parser.c" /* yacc.c:1646  */
    break;

  case 105:
#line 379 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2746 "php_parser.c" /* yacc.c:1646  */
    break;

  case 106:
#line 380 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2752 "php_parser.c" /* yacc.c:1646  */
    break;

  case 107:
#line 381 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2758 "php_parser.c" /* yacc.c:1646  */
    break;

  case 108:
#line 382 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2764 "php_parser.c" /* yacc.c:1646  */
    break;

  case 109:
#line 383 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2770 "php_parser.c" /* yacc.c:1646  */
    break;

  case 110:
#line 384 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2776 "php_parser.c" /* yacc.c:1646  */
    break;

  case 111:
#line 385 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-2].exp_node), make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node))); }
#line 2782 "php_parser.c" /* yacc.c:1646  */
    break;

  case 112:
#line 387 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); }
#line 2788 "php_parser.c" /* yacc.c:1646  */
    break;

  case 113:
#line 388 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_ADD, (yyvsp[0].exp_node), make_const_exp_dnum(1))); }
#line 2794 "php_parser.c" /* yacc.c:1646  */
    break;

  case 114:
#line 389 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[-1].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[-1].exp_node), make_const_exp_dnum(1))); }
#line 2800 "php_parser.c" /* yacc.c:1646  */
    break;

  case 115:
#line 390 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2_self(PHP_OP_ASS, (yyvsp[0].exp_node), make_exp_2(PHP_OP_SUB, (yyvsp[0].exp_node), make_const_exp_dnum(1))); }
#line 2806 "php_parser.c" /* yacc.c:1646  */
    break;

  case 116:
#line 392 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2812 "php_parser.c" /* yacc.c:1646  */
    break;

  case 117:
#line 393 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2818 "php_parser.c" /* yacc.c:1646  */
    break;

  case 118:
#line 394 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2824 "php_parser.c" /* yacc.c:1646  */
    break;

  case 119:
#line 395 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2830 "php_parser.c" /* yacc.c:1646  */
    break;

  case 120:
#line 396 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LOG_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2836 "php_parser.c" /* yacc.c:1646  */
    break;

  case 121:
#line 397 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_OR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2842 "php_parser.c" /* yacc.c:1646  */
    break;

  case 122:
#line 398 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_AND, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2848 "php_parser.c" /* yacc.c:1646  */
    break;

  case 123:
#line 399 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_XOR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2854 "php_parser.c" /* yacc.c:1646  */
    break;

  case 124:
#line 400 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_CAT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2860 "php_parser.c" /* yacc.c:1646  */
    break;

  case 125:
#line 401 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ADD, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2866 "php_parser.c" /* yacc.c:1646  */
    break;

  case 126:
#line 402 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2872 "php_parser.c" /* yacc.c:1646  */
    break;

  case 127:
#line 403 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2878 "php_parser.c" /* yacc.c:1646  */
    break;

  case 128:
#line 404 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_DIV, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2884 "php_parser.c" /* yacc.c:1646  */
    break;

  case 129:
#line 405 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_REM, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2890 "php_parser.c" /* yacc.c:1646  */
    break;

  case 130:
#line 406 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHL, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2896 "php_parser.c" /* yacc.c:1646  */
    break;

  case 131:
#line 407 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_SHR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2902 "php_parser.c" /* yacc.c:1646  */
    break;

  case 132:
#line 408 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[0].exp_node); }
#line 2908 "php_parser.c" /* yacc.c:1646  */
    break;

  case 133:
#line 409 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), (yyvsp[0].exp_node)); }
#line 2914 "php_parser.c" /* yacc.c:1646  */
    break;

  case 134:
#line 410 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_LOG_NOT, (yyvsp[0].exp_node)); }
#line 2920 "php_parser.c" /* yacc.c:1646  */
    break;

  case 135:
#line 411 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_NOT, (yyvsp[0].exp_node)); }
#line 2926 "php_parser.c" /* yacc.c:1646  */
    break;

  case 136:
#line 412 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2932 "php_parser.c" /* yacc.c:1646  */
    break;

  case 137:
#line 413 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_NOT_SAME, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2938 "php_parser.c" /* yacc.c:1646  */
    break;

  case 138:
#line 414 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2944 "php_parser.c" /* yacc.c:1646  */
    break;

  case 139:
#line 415 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_NEQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2950 "php_parser.c" /* yacc.c:1646  */
    break;

  case 140:
#line 416 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2956 "php_parser.c" /* yacc.c:1646  */
    break;

  case 141:
#line 417 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_LWR_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2962 "php_parser.c" /* yacc.c:1646  */
    break;

  case 142:
#line 418 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2968 "php_parser.c" /* yacc.c:1646  */
    break;

  case 143:
#line 419 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_GRT_EQ, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 2974 "php_parser.c" /* yacc.c:1646  */
    break;

  case 144:
#line 420 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-1].exp_node); }
#line 2980 "php_parser.c" /* yacc.c:1646  */
    break;

  case 145:
#line 421 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_MUX, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); (yyval.exp_node)->exp_node = (yyvsp[-4].exp_node); }
#line 2986 "php_parser.c" /* yacc.c:1646  */
    break;

  case 146:
#line 422 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_INT, (yyvsp[0].exp_node)); }
#line 2992 "php_parser.c" /* yacc.c:1646  */
    break;

  case 147:
#line 423 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_FLOAT, (yyvsp[0].exp_node)); }
#line 2998 "php_parser.c" /* yacc.c:1646  */
    break;

  case 148:
#line 424 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_STR, (yyvsp[0].exp_node)); }
#line 3004 "php_parser.c" /* yacc.c:1646  */
    break;

  case 149:
#line 425 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_CAST_BOOL, (yyvsp[0].exp_node)); }
#line 3010 "php_parser.c" /* yacc.c:1646  */
    break;

  case 150:
#line 428 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 3016 "php_parser.c" /* yacc.c:1646  */
    break;

  case 151:
#line 429 "php_parser.y" /* yacc.c:1646  */
    {  }
#line 3022 "php_parser.c" /* yacc.c:1646  */
    break;

  case 152:
#line 430 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[0].exp_node); }
#line 3028 "php_parser.c" /* yacc.c:1646  */
    break;

  case 153:
#line 432 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[0].exp_node); }
#line 3034 "php_parser.c" /* yacc.c:1646  */
    break;

  case 154:
#line 433 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY, (yyvsp[-1].exp_node)); }
#line 3040 "php_parser.c" /* yacc.c:1646  */
    break;

  case 155:
#line 434 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_PRINT, (yyvsp[0].exp_node)); }
#line 3046 "php_parser.c" /* yacc.c:1646  */
    break;

  case 156:
#line 437 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-1].exp_node); }
#line 3052 "php_parser.c" /* yacc.c:1646  */
    break;

  case 157:
#line 438 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = 0; }
#line 3058 "php_parser.c" /* yacc.c:1646  */
    break;

  case 158:
#line 439 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = 0; }
#line 3064 "php_parser.c" /* yacc.c:1646  */
    break;

  case 161:
#line 447 "php_parser.y" /* yacc.c:1646  */
    { /*$$ = make_assign_node($1);*/ }
#line 3070 "php_parser.c" /* yacc.c:1646  */
    break;

  case 162:
#line 448 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = (yyvsp[-1].exp_node); }
#line 3076 "php_parser.c" /* yacc.c:1646  */
    break;

  case 163:
#line 449 "php_parser.y" /* yacc.c:1646  */
    { /*$$ = make_assign_node(0);*/ }
#line 3082 "php_parser.c" /* yacc.c:1646  */
    break;

  case 164:
#line 452 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_LIST, 0); (yyval.exp_node)->exp_node = (yyvsp[0].exp_node); }
#line 3088 "php_parser.c" /* yacc.c:1646  */
    break;

  case 165:
#line 453 "php_parser.y" /* yacc.c:1646  */
    {
				PHP_EXP_NODE *last = (yyvsp[-2].exp_node);
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = (yyvsp[0].exp_node);
				(yyval.exp_node) = (yyvsp[-2].exp_node);
			}
#line 3100 "php_parser.c" /* yacc.c:1646  */
    break;

  case 166:
#line 462 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_PAIR, (yyvsp[0].exp_node)); }
#line 3106 "php_parser.c" /* yacc.c:1646  */
    break;

  case 167:
#line 463 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_PAIR, (yyvsp[-2].exp_node), (yyvsp[0].exp_node)); }
#line 3112 "php_parser.c" /* yacc.c:1646  */
    break;

  case 168:
#line 464 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_2(PHP_OP_ARRAY_REF_PAIR, (yyvsp[-3].exp_node), (yyvsp[0].exp_node)); }
#line 3118 "php_parser.c" /* yacc.c:1646  */
    break;

  case 169:
#line 465 "php_parser.y" /* yacc.c:1646  */
    { (yyval.exp_node) = make_exp_1(PHP_OP_ARRAY_REF_PAIR, (yyvsp[0].exp_node)); }
#line 3124 "php_parser.c" /* yacc.c:1646  */
    break;


#line 3128 "php_parser.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
