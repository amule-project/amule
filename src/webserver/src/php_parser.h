/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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
#line 67 "php_parser.y" /* yacc.c:1909  */

	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];

#line 152 "php_parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE phplval;

int phpparse (void);

#endif /* !YY_PHP_PHP_PARSER_H_INCLUDED  */
