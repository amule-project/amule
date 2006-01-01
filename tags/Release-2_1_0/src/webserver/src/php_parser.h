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
   Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA  02110-1301, USA.  */

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
#define PRINT 298
#define SR_EQ 299
#define SL_EQ 300
#define XOR_EQ 301
#define OR_EQ 302
#define AND_EQ 303
#define MOD_EQ 304
#define CONCAT_EQ 305
#define DIV_EQ 306
#define MUL_EQ 307
#define MINUS_EQ 308
#define PLUS_EQ 309
#define LOG_OR 310
#define LOG_XOR 311
#define LOG_AND 312
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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 67 "php_parser.y"
typedef union YYSTYPE {
	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];
} YYSTYPE;
/* Line 1318 of yacc.c.  */
#line 224 "php_parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



