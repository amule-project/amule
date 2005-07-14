%{
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

%}

%union {
	PHP_SYN_NODE *syn_node;
	PHP_EXP_NODE *exp_node;

	char str_val[256];
}

%type <syn_node> program_tree statement top_statement function_decl_statement top_statement_list
%type <syn_node> global_var_list static_var_list
%type <syn_node> while_statement foreach_statement for_statement elseif_list else_statement switch_case_list case_list

%type <exp_node> VARIABLE variable deref_variable global_var

/* "for_expr" is list of expressions - a syntax node actaully */
%type <syn_node> expr_list for_expr

%type <exp_node> expr exit_expr const_value function_call func_param_list assignment_list assignment_list_element
%type <exp_node> FNUMBER DNUMBER STRING

%type <str_val> IDENT

/*
	All my tokens
*/
%token FNUMBER DNUMBER STRING IDENT VARIABLE

%token ECHO
%token EXIT

%token IF DO WHILE ENDWHILE FOR ENDFOR FOREACH ENDFOREACH

%token DECLARE ENDDECLARE AS CONST GLOBAL UNSET ISSET EMPTY

%token SWITCH ENDSWITCH CASE DEFAULT BREAK CONTINUE
%token FUNCTION RETURN
%token CLASS INTERFACE EXTENDS IMPLEMENTS OBJECT_OPERATOR
%token HASH_ASSIGN LIST ARRAY

%token CLASS_SCOPE

/*
	Things with precedence
*/
%left ','
%left LOG_OR
%left LOG_XOR
%left LOG_AND
%right PRINT
%left '=' PLUS_EQ MINUS_EQ MUL_EQ DIV_EQ CONCAT_EQ MOD_EQ AND_EQ OR_EQ XOR_EQ SL_EQ SR_EQ
%left '?' ':'
%left BOOLEAN_OR
%left BOOLEAN_AND
%left '|'
%left '^'
%left '&'
%nonassoc IS_EQ IS_NOEQUAL IS_IDENTICAL IS_NOIDENTICAL
%nonassoc '<' IS_SMALLER_OR_EQ '>' IS_GREATER_OR_EQ
%left SL SR
%left '+' '-' '.'
%left '*' '/' '%'
%right '!'
%nonassoc INSTANCEOF
%right '~' INC DEC INT_CAST DOUBLE_CAST STRING_CAST ARRAY_CAST OBJECCAST BOOL_CAST UNSECAST '@'
%right '['
%nonassoc NEW CLONE

%left ELSEIF
%left ELSE
%left ENDIF

%right STATIC ABSTRACT FINAL PRIVATE PROTECTED PUBLIC

%% /* Rules */

program_tree: top_statement_list { g_syn_tree_top = $1; }
;

top_statement_list:
		top_statement_list  top_statement { $$ = add_statement_2_list($1, $2); }
	|	/* empty */ { $$ = 0; }
;


top_statement:
		statement
	|	function_decl_statement
/*	|	class_decl_statement */
;

statement:
		'{' top_statement_list '}'							{ $$ = $2; }
	|	expr ';'											{ $$ = make_expr_syn_node(PHP_ST_EXPR, $1); }
	|	GLOBAL global_var_list ';' 							{ $$ = $2; }
	|	STATIC static_var_list ';'							{ $$ = $2; }
	|	IF '(' expr ')' statement elseif_list else_statement	{ $$ = make_ifelse_syn_node($3, $5, $6, $7); }
	|	WHILE '(' expr  ')' while_statement					{ $$ = make_while_loop_syn_node($3, $5, 1); }
	|	DO statement WHILE '(' expr ')' ';'					{ $$ = make_while_loop_syn_node($5, $2, 0); }
	|	FOR '(' for_expr ';' for_expr ';' for_expr ')' for_statement { }
	|	SWITCH '(' expr ')' switch_case_list				{ }
	|	CONTINUE ';'										{ $$ = make_expr_syn_node(PHP_ST_CONTINUE, 0); }
	|	CONTINUE expr ';'									{ $$ = make_expr_syn_node(PHP_ST_CONTINUE, $2); }
	|	BREAK ';'											{ $$ = make_expr_syn_node(PHP_ST_BREAK, 0); }
	|	BREAK expr ';'										{ $$ = make_expr_syn_node(PHP_ST_BREAK, $2); }
	|	RETURN ';'											{ $$ = make_expr_syn_node(PHP_ST_RET, 0); }
	|	RETURN expr ';'										{ $$ = make_expr_syn_node(PHP_ST_RET, $2); }
	|	ECHO expr_list ';'									{  }
	|	UNSET '(' variable_list ')' ';'						{  }
	|	FOREACH '(' expr AS variable ')' foreach_statement 	{
				$$ = make_foreach_loop_syn_node($3, 0, $5->var_node, $7, 0);
			}
	|	FOREACH '(' expr AS variable HASH_ASSIGN variable ')' foreach_statement {
				$$ = make_foreach_loop_syn_node($3, $5->var_node, $7->var_node, $9, 0);
			}
	|	FOREACH '(' expr AS variable HASH_ASSIGN '&' variable ')' foreach_statement {
				$$ = make_foreach_loop_syn_node($3, $5->var_node, $8->var_node, $10, 1);
			}
	|	DECLARE '(' decl_list ')' statement					{ }
	|	DECLARE '(' decl_list ')' ':' top_statement_list ENDDECLARE { }
	|	';'													{ $$ = 0; }
;

decl_list: IDENT '=' const_value			{  }
	|	decl_list ',' IDENT '=' const_value	{  }
;

expr_list: expr				{  }
	|	expr_list ',' expr	{  }
;

variable_list: variable
	|	variable_list ',' variable
;

static_var_list:
		static_var_list ',' VARIABLE {  }
	|	static_var_list ',' VARIABLE '=' const_value {  }
	|	VARIABLE  {  }
	|	VARIABLE '=' const_value {  }
;

global_var_list: global_var				{  }
	|	global_var_list ',' global_var	{  }
;


global_var: VARIABLE
	|	'$' variable		{ $$ = $2; }
	|	'$' '{' expr '}'	{ $$ = make_exp_1(PHP_OP_VAR_BY_EXP, $3); }
;

function_decl_statement:
		FUNCTION IDENT '(' parameter_list ')' '{' top_statement_list '}' {  }
	|	FUNCTION '&' IDENT '(' parameter_list ')' '{' top_statement_list '}' {  }
;

parameter_list: 
		optional_class_type VARIABLE				{  }
	|	optional_class_type '&' VARIABLE			{  }
	|	parameter_list ',' optional_class_type VARIABLE 	{  }
	|	parameter_list ',' optional_class_type '&' VARIABLE	{  }
	|	/* empty */
;

optional_class_type:
		/* empty */	{  }
	|	IDENT		{  }
;


for_statement: 	statement
	|	':' top_statement_list ENDFOR ';' { $$ = $2; }
;


foreach_statement: 	statement
	|	':' top_statement_list ENDFOREACH ';' { $$ = $2; }
;

for_expr: expr_list
	|	/* empty */			{ $$ = 0; }
;

elseif_list:
		elseif_list ELSEIF '(' expr ')' statement { $$ = add_branch_2_elseif($1, make_ifelse_syn_node($4, $6, 0, 0)); }
	|	/* empty */		{ $$ = 0; }
;

else_statement:
		/* empty */ { $$ = 0; }
	|	ELSE statement { $$ = $2; }
;

while_statement: statement
	|	':' top_statement_list ENDWHILE ';' { $$ = $2; }
;

switch_case_list:
		'{' case_list '}'				{ $$ = $2; }
	|	'{' ';' case_list '}'			{ $$ = $3; }
	|	':' case_list ENDSWITCH ';'		{ $$ = $2; }
	|	':' ';' case_list ENDSWITCH ';'	{ $$ = $3; }
;

case_list:
		/* empty */	{  $$ = 0; }
	|	case_list CASE expr case_separator top_statement_list {  }
	|	case_list DEFAULT case_separator top_statement_list {  }
;

case_separator:	':'
	|	';'
;

const_value:
		FNUMBER
	|	DNUMBER
	|   STRING
	|	IDENT { $$ = make_known_const($1); }
;

variable:	deref_variable
	|	IDENT CLASS_SCOPE IDENT 				{ $$ = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str($1), make_const_exp_str($3)); }
	|	deref_variable OBJECT_OPERATOR IDENT	{ $$ = make_exp_2(PHP_OP_OBJECT_DEREF, $1, make_const_exp_str($3)); }
;


deref_variable: VARIABLE
	|	deref_variable '[' ']' 			{ $$ = make_exp_1(PHP_OP_ARRAY_BY_KEY, 0); }
	|	deref_variable '[' expr ']'		{ $$ = make_exp_2(PHP_OP_ARRAY_BY_KEY, $1, $3);}
	|	deref_variable '{' expr '}'		{ $$ = make_exp_2(PHP_OP_ARRAY_BY_KEY, $1, $3);}
	|	'$' '{' expr '}'				{ $$ = make_exp_1(PHP_OP_VAR_BY_EXP, $3); }
;

function_call:
		IDENT	'(' func_param_list ')' 								{ $$ = make_func_call_exp($1, $3); }
	|	deref_variable CLASS_SCOPE IDENT '(' func_param_list ')' 		{ }
	|	deref_variable OBJECT_OPERATOR IDENT '(' func_param_list ')'	{ }
;

func_param_list: expr						{ $$ = make_func_call_param_list(); func_call_add_expr($$->var_node, $1, 0); }
	|	'&' variable 						{ $$ = make_func_call_param_list(); func_call_add_expr($$->var_node, $2, 1); }
	|	func_param_list ',' expr			{ $$ = $1; func_call_add_expr($$->var_node, $3, 0); }
	|	func_param_list ',' '&' variable	{ $$ = $1; func_call_add_expr($$->var_node, $4, 1); }
	|	/* empty */							{ $$ = make_func_call_param_list(); }
;


expr:	
		LIST '(' assignment_list ')' '=' expr { }
	|	variable
	|	variable '=' expr			{ $$ = make_exp_2(PHP_OP_ASS, $1, $3); }
	|	function_call 				{ $$ = $1; }
	|	variable '=' '&' variable	{  }
/*
	|	NEW class_name_reference ctor_arguments { }
	|	CLONE expr {  }
*/
	|	variable PLUS_EQ expr 		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_ADD, $1, $3)); }
	|	variable MINUS_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SUB, $1, $3)); }
	|	variable MUL_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_MUL, $1, $3)); }
	|	variable DIV_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_DIV, $1, $3)); }
	|	variable CONCAT_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_CAT, $1, $3)); }
	|	variable MOD_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_REM, $1, $3)); }
	|	variable AND_EQ expr		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_AND, $1, $3)); }
	|	variable OR_EQ expr 		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_OR, $1, $3)); }
	|	variable XOR_EQ expr 		{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_XOR, $1, $3)); }
	|	variable SL_EQ expr			{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SHL, $1, $3)); } 
	|	variable SR_EQ expr			{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SHR, $1, $3)); } 
	/* ++var and var++ looks same to me */
	|	variable INC 				{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_ADD, $1, make_const_exp_dnum(1))); }
	|	INC variable 				{ $$ = make_exp_2(PHP_OP_ASS, $2, make_exp_2(PHP_OP_ADD, $2, make_const_exp_dnum(1))); }
	|	variable DEC 				{ $$ = make_exp_2(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SUB, $1, make_const_exp_dnum(1))); }
	|	DEC variable 				{ $$ = make_exp_2(PHP_OP_ASS, $2, make_exp_2(PHP_OP_SUB, $2, make_const_exp_dnum(1))); }
	
	|	expr BOOLEAN_OR expr 		{ $$ = make_exp_2(PHP_OP_LOG_OR, $1, $3); }
	|	expr BOOLEAN_AND expr 		{ $$ = make_exp_2(PHP_OP_LOG_AND, $1, $3); }  
	|	expr LOG_OR expr 			{ $$ = make_exp_2(PHP_OP_LOG_OR, $1, $3); }
	|	expr LOG_AND expr 			{ $$ = make_exp_2(PHP_OP_LOG_AND, $1, $3); }
	|	expr LOG_XOR expr 			{ $$ = make_exp_2(PHP_OP_LOG_XOR, $1, $3); }
	|	expr '|' expr				{ $$ = make_exp_2(PHP_OP_OR, $1, $3); }
	|	expr '&' expr				{ $$ = make_exp_2(PHP_OP_AND, $1, $3); }
	|	expr '^' expr				{ $$ = make_exp_2(PHP_OP_XOR, $1, $3); }
	|	expr '.' expr 				{ $$ = make_exp_2(PHP_OP_CAT, $1, $3); }
	|	expr '+' expr 				{ $$ = make_exp_2(PHP_OP_ADD, $1, $3); }
	|	expr '-' expr 				{ $$ = make_exp_2(PHP_OP_SUB, $1, $3); }
	|	expr '*' expr				{ $$ = make_exp_2(PHP_OP_MUL, $1, $3); }
	|	expr '/' expr				{ $$ = make_exp_2(PHP_OP_DIV, $1, $3); }
	|	expr '%' expr 				{ $$ = make_exp_2(PHP_OP_REM, $1, $3); }
	| 	expr SL expr				{ $$ = make_exp_2(PHP_OP_SHL, $1, $3); }
	|	expr SR expr				{ $$ = make_exp_2(PHP_OP_SHR, $1, $3); }
	|	'+' expr 					{ $$ = $2; }
	|	'-' expr 					{ $$ = make_exp_2(PHP_OP_SUB, make_const_exp_dnum(0), $2); }
	|	'!' expr {  }
	|	'~' expr {  }
	|	expr IS_IDENTICAL expr		{  }
	|	expr IS_NOIDENTICAL expr	{  }
	|	expr IS_EQ expr				{ $$ = make_exp_2(PHP_OP_EQ, $1, $3); }
	|	expr IS_NOEQUAL expr 		{ $$ = make_exp_2(PHP_OP_NEQ, $1, $3); }
	|	expr '<' expr 				{ $$ = make_exp_2(PHP_OP_LWR, $1, $3); }
	|	expr IS_SMALLER_OR_EQ expr 	{ $$ = make_exp_2(PHP_OP_LWR_EQ, $1, $3); }
	|	expr '>' expr 				{ $$ = make_exp_2(PHP_OP_GRT, $1, $3); }
	|	expr IS_GREATER_OR_EQ expr 	{ $$ = make_exp_2(PHP_OP_GRT_EQ, $1, $3); }
	|	'(' expr ')' 				{ $$ = $2; }
	|	expr '?' expr ':' expr {  }
	|	INT_CAST expr 	{  }
	|	DOUBLE_CAST expr 	{  }
	|	STRING_CAST expr	{  } 
	|	ARRAY_CAST expr 	{  }
	|	OBJECCAST expr 	{  }
	|	BOOL_CAST expr	{  }
	|	UNSECAST expr	{  }
	|	EXIT exit_expr	{  }
	|	'@' expr 					{ $$ = $2; }

	|	const_value					{ $$ = $1; }
/*	|	ARRAY '(' array_pair_list ')' { } */
	|	PRINT expr  				{ $$ = make_exp_1(PHP_OP_PRINT, $2); }
;

exit_expr: '(' expr ')'	{ $$ = $2; }
	|	'(' ')'			{ $$ = 0; }	
	|	/* empty */		{ $$ = 0; }	
;

assignment_list: assignment_list_element
	|	assignment_list ',' assignment_list_element
;


assignment_list_element: variable
	|	LIST '(' assignment_list ')'	{ $$ = $3; }
	|	/* empty */						{ $$ = 0; }
;

