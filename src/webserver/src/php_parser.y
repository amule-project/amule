%{
//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (C) 2005-2006Froenchenko Leonid ( lfroen@amule.org )
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
%type <syn_node> while_statement foreach_statement for_statement elseif_list else_statement 

%type <exp_node> VARIABLE variable deref_variable global_var static_var
%type <exp_node> parameter_list array_pair_list array_elem
%type <exp_node> switch_case_list case_list case_list_item

%type <exp_node> expr expr_list for_expr exit_expr const_value function_call func_param_list assignment_list assignment_list_element
%type <exp_node> FNUMBER DNUMBER STRING

%type <str_val> IDENT optional_class_type

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
%right PRINT
%left '=' PLUS_EQ MINUS_EQ MUL_EQ DIV_EQ CONCAT_EQ MOD_EQ AND_EQ OR_EQ XOR_EQ SL_EQ SR_EQ
%left '?' ':'
%left LOG_OR
%left LOG_XOR
%left LOG_AND
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
%right '~' INC DEC INT_CAST DOUBLE_CAST STRING_CAST ARRAY_CAST OBJECT_CAST BOOL_CAST UNSET_CAST '@'
%right '['
%nonassoc NEW CLONE

%left ELSEIF
%left ELSE
%left ENDIF

%right STATIC ABSTRACT FINAL PRIVATE PROTECTED PUBLIC

%token START_SCRIPT END_SCRIPT

%% /* Rules */

program_tree: START_SCRIPT top_statement_list END_SCRIPT { g_syn_tree_top = $2; }
;

top_statement_list:
		top_statement_list  top_statement { $$ = add_statement_2_list($1, $2); }
	|	START_SCRIPT top_statement_list END_SCRIPT { $$ = 0; }
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
	|	GLOBAL global_var_list ';' 							{ $$ = 0; }
	|	STATIC static_var_list ';'							{ $$ = $2; }
	|	IF '(' expr ')' statement elseif_list else_statement	{ $$ = make_ifelse_syn_node($3, $5, $6, $7); }
	|	WHILE '(' expr  ')' while_statement					{ $$ = make_while_loop_syn_node($3, $5, 1); }
	|	DO statement WHILE '(' expr ')' ';'					{ $$ = make_while_loop_syn_node($5, $2, 0); }
	|	FOR '(' for_expr ';' for_expr ';' for_expr ')' for_statement { $$ = make_for_syn_node($3, $5, $7, $9); }
	|	SWITCH '(' expr ')' switch_case_list				{ $$ = make_switch_syn_node($3, $5); }
	|	CONTINUE ';'										{ $$ = make_expr_syn_node(PHP_ST_CONTINUE, 0); }
	|	CONTINUE expr ';'									{ $$ = make_expr_syn_node(PHP_ST_CONTINUE, $2); }
	|	BREAK ';'											{ $$ = make_expr_syn_node(PHP_ST_BREAK, 0); }
	|	BREAK expr ';'										{ $$ = make_expr_syn_node(PHP_ST_BREAK, $2); }
	|	RETURN ';'											{ $$ = make_expr_syn_node(PHP_ST_RET, 0); }
	|	RETURN expr ';'										{ $$ = make_expr_syn_node(PHP_ST_RET, $2); }
	|	ECHO expr_list ';'									{ $$ = make_expr_syn_node(PHP_ST_ECHO, $2); }
	|	UNSET '(' variable_list ')' ';'						{  }
	|	FOREACH '(' expr AS variable ')' foreach_statement 	{
				$$ = make_foreach_loop_syn_node($3, 0, $5, $7, 0);
			}
	|	FOREACH '(' expr AS variable HASH_ASSIGN variable ')' foreach_statement {
				$$ = make_foreach_loop_syn_node($3, $5, $7, $9, 0);
			}
	|	FOREACH '(' expr AS variable HASH_ASSIGN '&' variable ')' foreach_statement {
				$$ = make_foreach_loop_syn_node($3, $5, $8, $10, 1);
			}
	|	DECLARE '(' decl_list ')' statement					{ }
	|	DECLARE '(' decl_list ')' ':' top_statement_list ENDDECLARE { }
	|	';'													{ $$ = 0; }
;

decl_list: IDENT '=' const_value			{  }
	|	decl_list ',' IDENT '=' const_value	{  }
;

expr_list: expr				{ $$ = make_exp_1(PHP_OP_LIST, 0); $$->exp_node = $1; }
	|	expr_list ',' expr	{
				PHP_EXP_NODE *last = $1;
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = $3;
				$$ = $1;
			}
;

variable_list: variable
	|	variable_list ',' variable
;

/*
 This IS implemented. global_var/static itself initialize ptrs as needed
*/
global_var_list: global_var				{ $$ = 0; }
	|	global_var_list ',' global_var	{  }
;


global_var: VARIABLE	{
		const char *varname = get_scope_var_name(g_current_scope, $1->var_si_node->var);
		PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, varname);
		PHP_SCOPE_ITEM *gsi = get_scope_item(g_global_scope, varname);
		if ( gsi && (gsi->type == PHP_SCOPE_VAR) ) {
			free_var_node(si->var);
			php_exp_tree_free($1);
			gsi->var->ref_count++;
			si->var = gsi->var;
		} else {
			php_report_error(PHP_ERROR, "There is no such global var");
		}
	}
;

static_var_list: static_var				{ $$ = 0; }
	|	static_var_list ',' static_var	{  }
;

static_var : VARIABLE				{ $1->var_node->flags |= PHP_VARFLAG_STATIC; $$ = $1; }
	|	VARIABLE '=' const_value	{
			$1->var_node->flags |= PHP_VARFLAG_STATIC; $$ = $1;
			value_value_assign(&$1->var_node->value, &$3->val_node);
		}
;


function_decl_statement:
		FUNCTION IDENT {
				switch_push_scope_table(make_scope_table())
			} '(' parameter_list ')' '{' top_statement_list '}' {
				$$ = make_func_decl_syn_node($2, $5);
				$$->func_decl->scope = g_current_scope;
				$$->func_decl->is_native = 0;
				$$->func_decl->code = $8;
				switch_pop_scope_table(0);
				add_func_2_scope(g_current_scope, $$);
				$$ = 0;
			}
	|	FUNCTION '&' IDENT '(' parameter_list ')' '{' top_statement_list '}' {  }
;

parameter_list: 
		optional_class_type VARIABLE						{ $$ = make_func_param(0, $2, $1, 0); }
	|	optional_class_type '&' VARIABLE					{ $$ = make_func_param(0, $3, $1, 1); }
	|	parameter_list ',' optional_class_type VARIABLE 	{ $$ = make_func_param($1, $4, $3, 0); }
	|	parameter_list ',' optional_class_type '&' VARIABLE	{ $$ = make_func_param($1, $5, $3, 1); }
	|	/* empty */											{ $$ = 0; }
;

optional_class_type:
		/* empty */	{ $$[0] = 0; }
	|	IDENT
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
	|	case_list case_list_item case_separator top_statement_list {
			$2->tree_node.syn_right = $4;
			if ( $1 ) {
				PHP_EXP_NODE *last = $1;
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = $2;
				$$ = $1;
			} else {
				$$ = make_exp_1(PHP_OP_LIST, 0);
				$$->exp_node = $2;
			}
		}
;

case_list_item: CASE expr	{ $$ = make_exp_2(PHP_OP_LIST, $2, 0); }
	| DEFAULT				{ $$ = make_exp_2(PHP_OP_LIST, 0, 0); }
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
	|	IDENT CLASS_SCOPE IDENT 				{ $$ = make_exp_2(PHP_OP_CLASS_DEREF, make_const_exp_str($1, 0), make_const_exp_str($3, 0)); }
	|	deref_variable OBJECT_OPERATOR IDENT	{ $$ = make_exp_2(PHP_OP_OBJECT_DEREF, $1, make_const_exp_str($3, 0)); }
;


deref_variable: VARIABLE
	|	deref_variable '[' ']' 			{ $$ = make_exp_2(PHP_OP_ARRAY_BY_KEY, $1, 0); }
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
	|	variable '=' '&' variable	{ $$ = make_exp_2(PHP_MAKE_REF, $1, $4); }
/*
	|	NEW class_name_reference ctor_arguments { }
	|	CLONE expr {  }
*/
	|	variable PLUS_EQ expr 		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_ADD, $1, $3)); }
	|	variable MINUS_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SUB, $1, $3)); }
	|	variable MUL_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_MUL, $1, $3)); }
	|	variable DIV_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_DIV, $1, $3)); }
	|	variable CONCAT_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_CAT, $1, $3)); }
	|	variable MOD_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_REM, $1, $3)); }
	|	variable AND_EQ expr		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_AND, $1, $3)); }
	|	variable OR_EQ expr 		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_OR, $1, $3)); }
	|	variable XOR_EQ expr 		{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_XOR, $1, $3)); }
	|	variable SL_EQ expr			{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SHL, $1, $3)); } 
	|	variable SR_EQ expr			{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SHR, $1, $3)); } 
	/* ++var and var++ looks same to me */
	|	variable INC 				{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_ADD, $1, make_const_exp_dnum(1))); }
	|	INC variable 				{ $$ = make_exp_2_self(PHP_OP_ASS, $2, make_exp_2(PHP_OP_ADD, $2, make_const_exp_dnum(1))); }
	|	variable DEC 				{ $$ = make_exp_2_self(PHP_OP_ASS, $1, make_exp_2(PHP_OP_SUB, $1, make_const_exp_dnum(1))); }
	|	DEC variable 				{ $$ = make_exp_2_self(PHP_OP_ASS, $2, make_exp_2(PHP_OP_SUB, $2, make_const_exp_dnum(1))); }
	
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
	|	'!' expr					{ $$ = make_exp_1(PHP_OP_LOG_NOT, $2); }
	|	'~' expr					{ $$ = make_exp_1(PHP_OP_NOT, $2); }
	|	expr IS_IDENTICAL expr		{ $$ = make_exp_2(PHP_OP_SAME, $1, $3); }
	|	expr IS_NOIDENTICAL expr	{ $$ = make_exp_2(PHP_OP_NOT_SAME, $1, $3); }
	|	expr IS_EQ expr				{ $$ = make_exp_2(PHP_OP_EQ, $1, $3); }
	|	expr IS_NOEQUAL expr 		{ $$ = make_exp_2(PHP_OP_NEQ, $1, $3); }
	|	expr '<' expr 				{ $$ = make_exp_2(PHP_OP_LWR, $1, $3); }
	|	expr IS_SMALLER_OR_EQ expr 	{ $$ = make_exp_2(PHP_OP_LWR_EQ, $1, $3); }
	|	expr '>' expr 				{ $$ = make_exp_2(PHP_OP_GRT, $1, $3); }
	|	expr IS_GREATER_OR_EQ expr 	{ $$ = make_exp_2(PHP_OP_GRT_EQ, $1, $3); }
	|	'(' expr ')' 				{ $$ = $2; }
	|	expr '?' expr ':' expr		{ $$ = make_exp_2(PHP_OP_MUX, $3, $5); $$->exp_node = $1; }
	|	INT_CAST expr 				{ $$ = make_exp_1(PHP_OP_CAST_INT, $2); }
	|	DOUBLE_CAST expr 			{ $$ = make_exp_1(PHP_OP_CAST_FLOAT, $2); }
	|	STRING_CAST expr			{ $$ = make_exp_1(PHP_OP_CAST_STR, $2); } 
	|	BOOL_CAST expr				{ $$ = make_exp_1(PHP_OP_CAST_BOOL, $2); }
/*	|	ARRAY_CAST expr 	{  } */
/*	|	OBJECT_CAST expr 	{  } */
	|	UNSET_CAST expr	{  }
	|	EXIT exit_expr	{  }
	|	'@' expr 					{ $$ = $2; }

	|	const_value					{ $$ = $1; }
	|	ARRAY '(' array_pair_list ')' { $$ = make_exp_1(PHP_OP_ARRAY, $3); }
	|	PRINT expr  				{ $$ = make_exp_1(PHP_OP_PRINT, $2); }
;

exit_expr: '(' expr ')'	{ $$ = $2; }
	|	'(' ')'			{ $$ = 0; }	
	|	/* empty */		{ $$ = 0; }	
;

assignment_list: assignment_list_element
	|	assignment_list ',' assignment_list_element
;


assignment_list_element: variable		{ /*$$ = make_assign_node($1);*/ }
	|	LIST '(' assignment_list ')'	{ $$ = $3; }
	|	/* empty */						{ /*$$ = make_assign_node(0);*/ }
;

array_pair_list: array_elem				{ $$ = make_exp_1(PHP_OP_LIST, 0); $$->exp_node = $1; }
	| array_pair_list ',' array_elem	{
				PHP_EXP_NODE *last = $1;
				while ( last->next) last = last->next;
				last->next = make_exp_1(PHP_OP_LIST, 0);
				last->next->exp_node = $3;
				$$ = $1;
			}
;

array_elem : expr						{ $$ = make_exp_1(PHP_OP_ARRAY_PAIR, $1); }
	| expr HASH_ASSIGN expr				{ $$ = make_exp_2(PHP_OP_ARRAY_PAIR, $1, $3); }
	| expr HASH_ASSIGN '&' variable		{ $$ = make_exp_2(PHP_OP_ARRAY_REF_PAIR, $1, $4); }
	| '&' variable						{ $$ = make_exp_1(PHP_OP_ARRAY_REF_PAIR, $2); }
;

	
