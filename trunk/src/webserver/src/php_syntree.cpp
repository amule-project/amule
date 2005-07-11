//
// This file is part of the aMule Project.
//
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
#include <assert.h>

#include <list>
#include <map>
#include <string>

#include "php_syntree.h"
#include "php_core_lib.h"

typedef std::map<std::string, PHP_VAR_NODE *>::iterator PHP_ARRAY_ITER_TYPE;
typedef struct {
	std::map<std::string, PHP_VAR_NODE *> array;
	PHP_ARRAY_ITER_TYPE current;
} PHP_ARRAY_TYPE;

PHP_SYN_NODE *g_syn_tree_top = 0;

/* scope table */
PHP_SCOPE_TABLE g_global_scope = 0;
PHP_SCOPE_TABLE g_current_scope = 0;
PHP_SCOPE_STACK g_scope_stack = 0;

//
// using std::string instead of "char *" so keys will be compared 
// by string value
typedef std::map<std::string, PHP_SCOPE_ITEM *> PHP_SCOPE_TABLE_TYPE;
typedef std::list<PHP_SCOPE_TABLE_TYPE *> PHP_SCOPE_STACK_TYPE;

PHP_EXP_NODE *make_zero_exp_node()
{
	PHP_EXP_NODE *node = new PHP_EXP_NODE;
	memset(node, 0, sizeof(PHP_EXP_NODE));
	return node;
}

PHP_EXP_NODE *make_const_exp_dnum(int number)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = PHP_OP_VAL;
	node->val_node.type = PHP_VAL_INT;
	node->val_node.int_val = number;
	
	return node;
}

PHP_EXP_NODE *make_const_exp_fnum(float number)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = PHP_OP_VAL;
	node->val_node.type = PHP_VAL_FLOAT;
	node->val_node.float_val = number;
	
	return node;
}

PHP_EXP_NODE *make_const_exp_str(char *s)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = PHP_OP_VAL;
	node->val_node.type = PHP_VAL_STRING;
	node->val_node.str_val = strdup(s);
	
	return node;
}

PHP_EXP_NODE *make_exp_1(PHP_EXP_OP op, PHP_EXP_NODE *operand)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = op;
	node->tree_node.left = operand;
	return node;
}

PHP_EXP_NODE *make_exp_2(PHP_EXP_OP op, PHP_EXP_NODE *left, PHP_EXP_NODE *right)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = op;
	node->tree_node.left = left;
	node->tree_node.right = right;
	return node;
}


/*
 * Syntax tree generation
 */
PHP_SYN_NODE *make_expr_syn_node(PHP_EXP_NODE *expr)
{
	PHP_SYN_NODE *syn_node = new PHP_SYN_NODE;
	memset(syn_node, 0, sizeof(PHP_SYN_NODE));
	
	syn_node->type = PHP_ST_EXPR;
	syn_node->node_expr = expr;
	
	return syn_node;
}

PHP_SYN_NODE *make_ifelse_syn_node(PHP_EXP_NODE *expr,
	PHP_SYN_NODE *then_node, PHP_SYN_NODE *elseif_list, PHP_SYN_NODE *else_node)
{
	PHP_SYN_NODE *syn_node = new PHP_SYN_NODE;
	memset(syn_node, 0, sizeof(PHP_SYN_NODE));
	
	syn_node->type = PHP_ST_IF;
	syn_node->node_if.cond = expr;
	syn_node->node_if.code_if = then_node;
	
	if ( elseif_list ) {
		syn_node->node_if.code_else = elseif_list;
	
		PHP_SYN_NODE *curr_if = elseif_list;
		while ( curr_if->node_if.code_else ) {
			curr_if = curr_if->node_if.code_else;
		}
		curr_if->node_if.code_else = else_node;
	} else {
		syn_node->node_if.code_else = else_node;
	}
	return syn_node;
}

PHP_SYN_NODE *make_while_loop_syn_node(PHP_EXP_NODE *cond, PHP_SYN_NODE *code, int do_while)
{
	PHP_SYN_NODE *syn_node = new PHP_SYN_NODE;
	memset(syn_node, 0, sizeof(PHP_SYN_NODE));
	
	syn_node->type = do_while ? PHP_ST_WHILE : PHP_ST_DO_WHILE;
	syn_node->node_while.cond = cond;
	syn_node->node_while.code = code;
	
	return syn_node;
}

PHP_SYN_NODE *make_func_decl_syn_node()
{
	PHP_SYN_NODE *syn_node = new PHP_SYN_NODE;
	memset(syn_node, 0, sizeof(PHP_SYN_NODE));
	
	syn_node->type = PHP_ST_FUNC_DECL;
	
	syn_node->func_decl = new PHP_SYN_FUNC_DECL_NODE;
	memset(syn_node->func_decl, 0, sizeof(PHP_SYN_FUNC_DECL_NODE));
	
	return syn_node;
}

PHP_VAR_NODE *make_var_node()
{
	PHP_VAR_NODE *node = new PHP_VAR_NODE;
	memset(node, 0, sizeof(PHP_VAR_NODE));
	node->value.type = PHP_VAL_NONE;
	
	return node;
}

PHP_VAR_NODE *make_array_var()
{
	PHP_VAR_NODE *node = make_var_node();
	cast_value_array(&node->value);
	
	return node;
}

/*
 * Called from lexer when ${IDENT} is recognized
 */
PHP_EXP_NODE *get_var_node(char *name)
{
	PHP_EXP_NODE *node = make_zero_exp_node();
	node->op = PHP_OP_VAR;
	
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, name);
	if ( si ) {
		if ( si->type == PHP_SCOPE_VAR ) {
			node->var_node = si->var;
		} else {
			//
			// Error: symbol already defined as different entity
			//
			php_report_error("symbol already defined as different entity", PHP_ERROR);
		}
	} else {
		node->var_node = make_var_node();
		add_var_2_scope(g_current_scope, node->var_node, name);
	}
	
	return node;
}


PHP_SCOPE_TABLE make_scope_table(PHP_SCOPE_TABLE ref_table, PHP_VALUE_NODE *arg_array)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = new PHP_SCOPE_TABLE_TYPE;
	
	if ( ref_table ) {
		PHP_SCOPE_TABLE_TYPE *ref_scope_map = (PHP_SCOPE_TABLE_TYPE *)ref_table;
			
		for(PHP_SCOPE_TABLE_TYPE::iterator i = ref_scope_map->begin(); i != ref_scope_map->end();i++) {
			PHP_SCOPE_ITEM *si = i->second;
			if ( si->type == PHP_SCOPE_VAR ) {
			} else if ( arg_array && (si->type == PHP_SCOPE_PARAM) ) {
				PHP_VAR_NODE *curr_arg_val = array_get_by_int_key(arg_array, si->param.num);

				PHP_VAR_NODE *new_arg_val;
				if ( si->param.var->flags & PHP_VARFLAG_BYREF ) {
					new_arg_val = php_expr_eval_lvalue((PHP_EXP_NODE *)curr_arg_val->value.ptr_val);
					if ( new_arg_val ) {
						new_arg_val->ref_count++;
					} else {
						//
						// Error (user): expression is not addressable variable
						//
					}
				} else {
					new_arg_val = make_var_node();
					php_expr_eval((PHP_EXP_NODE *)curr_arg_val->value.ptr_val, &new_arg_val->value);
				}
				if ( new_arg_val ) {
					add_var_2_scope(scope_map, new_arg_val, i->first.c_str());
				}
			}
		}
	}
	
	return scope_map;
}

void switch_push_scope_table(PHP_SCOPE_TABLE new_table)
{
	PHP_SCOPE_STACK_TYPE *scope_stack = (PHP_SCOPE_STACK_TYPE *)g_scope_stack;
	scope_stack->push_back((PHP_SCOPE_TABLE_TYPE *)g_current_scope);
	g_current_scope = new_table;
}

void switch_pop_scope_table()
{
	PHP_SCOPE_STACK_TYPE *scope_stack = (PHP_SCOPE_STACK_TYPE *)g_scope_stack;
	delete_scope_table(g_current_scope);
	if ( scope_stack->size() == 0 ) {
		php_report_error("Stack underrun - no valid scope", PHP_INTERNAL_ERROR);
	}
	g_current_scope = scope_stack->back();
}

void delete_scope_table(PHP_SCOPE_TABLE scope)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = (PHP_SCOPE_TABLE_TYPE *)scope;
	
	for(PHP_SCOPE_TABLE_TYPE::iterator i = scope_map->begin(); i != scope_map->end();i++) {
		if ( i->second->type == PHP_SCOPE_VAR ) {
			PHP_VAR_NODE *var = i->second->var;
			var->ref_count--;
			if ( var->ref_count == 0 ) {
				delete var;
			}
		}
		delete i->second;
	}
	delete scope_map;
}
	
void add_func_2_scope(PHP_SCOPE_TABLE scope, PHP_SYN_NODE *func)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = (PHP_SCOPE_TABLE_TYPE *)scope;
	PHP_SCOPE_ITEM *it = new PHP_SCOPE_ITEM;
	it->type = PHP_SCOPE_FUNC;
	it->func = func;
	std::string key(func->func_decl->name);
	if ( scope_map->count(key) ) {
		// error - function already defined
	} else {
		(*scope_map)[key] = it;
	}
}

PHP_SCOPE_ITEM *make_named_scope_item(PHP_SCOPE_TABLE scope, const char *name)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = (PHP_SCOPE_TABLE_TYPE *)scope;
	PHP_SCOPE_ITEM *it = new PHP_SCOPE_ITEM;
	std::string key(name);
	(*scope_map)[key] = it;
	return it;
}

void add_var_2_scope(PHP_SCOPE_TABLE scope, PHP_VAR_NODE *var, const char *name)
{
	PHP_SCOPE_ITEM *it = make_named_scope_item(scope, name);
	it->type = PHP_SCOPE_VAR;
	it->var = var;
	var->ref_count++;
}

void add_param_2_scope(PHP_SCOPE_TABLE scope, PHP_VAR_NODE *var, const char *name)
{
	PHP_SCOPE_ITEM *it = make_named_scope_item(scope, name);
	it->type = PHP_SCOPE_PARAM;
	it->var = var;
	var->ref_count++;
}

PHP_SCOPE_ITEM_TYPE get_scope_item_type(PHP_SCOPE_TABLE scope, const char *name)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = (PHP_SCOPE_TABLE_TYPE *)scope;
	std::string key(name);
	if ( scope_map->count(key) ) {
		PHP_SCOPE_ITEM *it = (*scope_map)[key];
		return it->type;
	}
	return PHP_SCOPE_NONE;
}

PHP_SCOPE_ITEM *get_scope_item(PHP_SCOPE_TABLE scope, char *name)
{
	PHP_SCOPE_TABLE_TYPE *scope_map = (PHP_SCOPE_TABLE_TYPE *)scope;
	std::string key(name);
	if ( scope_map->count(key) ) {
		PHP_SCOPE_ITEM *it = (*scope_map)[key];
		return it;
	}
	return 0;
}


/* array operations */
const std::string &array_get_ith_key(PHP_VALUE_NODE *array, int i)
{
	PHP_ARRAY_TYPE *arr_ptr = (PHP_ARRAY_TYPE *)array->ptr_val;
	PHP_ARRAY_ITER_TYPE it = arr_ptr->array.begin();
	while(i--) it++;
	return it->first;
}


PHP_VAR_NODE *array_get_by_str_key(PHP_VALUE_NODE *array, std::string key)
{
	if ( array->type != PHP_VAL_ARRAY ) {
		return 0;
	}
	PHP_ARRAY_TYPE *arr_ptr = (PHP_ARRAY_TYPE *)array->ptr_val;
	if ( arr_ptr->array.count(key) ) {
		return (arr_ptr->array)[key];
	} else {
		PHP_VAR_NODE *add_node = make_var_node();
		add_node->ref_count++;
		(arr_ptr->array)[key] = add_node;
		return add_node;
	}	
}

PHP_VAR_NODE *array_get_by_int_key(PHP_VALUE_NODE *array, int key)
{
	if ( array->type != PHP_VAL_ARRAY ) {
		return 0;
	}
	char s_key[32];
	sprintf(s_key, "%d", key);	
	return array_get_by_str_key(array, s_key);
}

PHP_VAR_NODE *array_get_by_key(PHP_VALUE_NODE *array, PHP_VALUE_NODE *key)
{
	if ( array->type != PHP_VAL_ARRAY ) {
		return 0;
	}
	PHP_VALUE_NODE s_key = *key;
	cast_value_str(&s_key);
	return array_get_by_str_key(array, s_key.str_val);
}

int array_is_key_here(PHP_VALUE_NODE *array, PHP_VALUE_NODE *key)
{
	if ( array->type != PHP_VAL_ARRAY ) {
		return 0;
	}
	PHP_ARRAY_TYPE *arr_ptr = (PHP_ARRAY_TYPE *)array->ptr_val;
	PHP_VALUE_NODE s_key = *key;
	cast_value_str(&s_key);
	std::string arr_key(s_key.str_val);
	
	return arr_ptr->array.count(arr_key);
}

int array_get_size(PHP_VALUE_NODE *array)
{
	if ( array->type != PHP_VAL_ARRAY ) {
		return 0;
	}
	PHP_ARRAY_TYPE *arr_ptr = (PHP_ARRAY_TYPE *)array->ptr_val;
	
	return arr_ptr->array.size();
}

PHP_VAR_NODE *array_push_back(PHP_VALUE_NODE *array)
{
	for(int i = 0; i < 0xffff;i++) {
		PHP_VAR_NODE *arr_var_node = array_get_by_int_key(array, i);
		if ( arr_var_node->value.type == PHP_VAL_NONE ) {
			return arr_var_node;
		}
	}
	// array size reached 64K ?!
	return 0;
}

/* casting functions */

void value_value_free(PHP_VALUE_NODE *val)
{
	switch(val->type) {
		case PHP_VAL_NONE:
		case PHP_VAL_BOOL:
		case PHP_VAL_INT:
		case PHP_VAL_FLOAT:
			break;
		case PHP_VAL_STRING: {
			free(val->str_val);
			val->str_val = 0;
			break;
		}
		case PHP_VAL_ARRAY: {
			for(PHP_ARRAY_ITER_TYPE i = ((PHP_ARRAY_TYPE *)val->ptr_val)->array.begin();
				i != ((PHP_ARRAY_TYPE *)val->ptr_val)->array.end(); i++) {
					i->second->ref_count--;
					if ( i->second->ref_count == 0 ) {
						value_value_free(&i->second->value);
						delete i->second;
					}
				}
			delete ((PHP_ARRAY_TYPE *)val->ptr_val);
			break;
		}
		case PHP_VAL_OBJECT: break;

		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: break;
	}
	val->type = PHP_VAL_NONE;
}

void cast_value_dnum(PHP_VALUE_NODE *val)
{
	switch(val->type) {
		case PHP_VAL_NONE: val->int_val = 0; break;
		case PHP_VAL_BOOL:
		case PHP_VAL_INT: break;
		case PHP_VAL_FLOAT: val->int_val = (int)val->float_val; break;
		case PHP_VAL_STRING: {
			char *str = val->str_val;
			val->int_val = atoi(val->str_val);
			free(str);
			break;
		}
		case PHP_VAL_ARRAY: 
		case PHP_VAL_OBJECT: val->int_val = 0; break;
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
	val->type = PHP_VAL_INT;
}

void cast_value_bool(PHP_VALUE_NODE *val)
{
	cast_value_dnum(val);
	val->type = PHP_VAL_BOOL;
}

void cast_value_fnum(PHP_VALUE_NODE *val)
{
	switch(val->type) {
		case PHP_VAL_NONE: val->float_val = 0; break;
		case PHP_VAL_BOOL:
		case PHP_VAL_INT: val->float_val = val->int_val; break;
		case PHP_VAL_FLOAT: break;
		case PHP_VAL_STRING: {
			char *str = val->str_val;
			val->float_val = atof(val->str_val);
			free(str);
			break;
		}
		case PHP_VAL_ARRAY: 
		case PHP_VAL_OBJECT: val->float_val = 0; break;
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
	val->type = PHP_VAL_FLOAT;
}

void cast_value_str(PHP_VALUE_NODE *val)
{
	char buff[256];
	switch(val->type) {
		case PHP_VAL_NONE: buff[0] = 0; break;
		case PHP_VAL_BOOL:
		case PHP_VAL_INT: sprintf(buff, "%d", val->int_val); break;
		case PHP_VAL_FLOAT: sprintf(buff, "%f", val->float_val); break;
		case PHP_VAL_STRING: return;
		case PHP_VAL_ARRAY: {
			delete ((PHP_ARRAY_TYPE *)val->ptr_val);
			strcpy(buff, "Array"); break;
		}
		case PHP_VAL_OBJECT: strcpy(buff, "Object"); break;
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
	val->str_val = strdup(buff);
	val->type = PHP_VAL_STRING;
}

void cast_value_array(PHP_VALUE_NODE *val)
{
	switch(val->type) {
		case PHP_VAL_NONE:
		case PHP_VAL_BOOL:
		case PHP_VAL_INT:
		case PHP_VAL_FLOAT: break;
		case PHP_VAL_STRING: free(val->str_val);
		case PHP_VAL_ARRAY: return;
		case PHP_VAL_OBJECT: ;/* must call to free_obj() */
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
	val->ptr_val = new PHP_ARRAY_TYPE;
	val->type = PHP_VAL_ARRAY;
}

/*
 * Function calls
 */
PHP_EXP_NODE *make_func_call_exp(char *func_name, PHP_EXP_NODE *args)
{
	PHP_EXP_NODE *call_node = make_zero_exp_node();

	call_node->op = PHP_OP_FUNC_CALL;
	// copy function name
	call_node->tree_node.left = make_zero_exp_node();
	call_node->tree_node.left->op = PHP_OP_VAL;
	call_node->tree_node.left->val_node.type = PHP_VAL_STRING;
	call_node->tree_node.left->val_node.str_val = strdup(func_name);
	// set params
	call_node->tree_node.right = args;
	
	return call_node;
}

PHP_EXP_NODE *make_func_call_param_list()
{
	PHP_VAR_NODE *params = make_array_var();

	PHP_EXP_NODE *exp_node = make_zero_exp_node();

	exp_node->op = PHP_OP_VAR;
	exp_node->var_node = params;
	
	return exp_node;
}

void func_call_add_expr(PHP_VAR_NODE *paramlist, PHP_EXP_NODE *arg, int byref)
{
	PHP_VAR_NODE *node = array_push_back(&paramlist->value);
	node->value.type = PHP_VAL_INT_DATA;
	node->value.ptr_val = arg;
	if ( byref ) {
		node->flags |= PHP_VARFLAG_BYREF;
	}
}

void php_add_native_func(PHP_BLTIN_FUNC_DEF *def)
{
	if ( get_scope_item_type(g_global_scope, def->name) != PHP_SCOPE_NONE ) {
		//
		// Error: something already defined by this name
		//
		php_report_error("Can't add scope item: symbol already defined", PHP_ERROR);
		return;
	}
	PHP_SCOPE_TABLE func_scope = make_scope_table(0, 0);
	
	PHP_SYN_NODE *decl_node = make_func_decl_syn_node();
	for(int i = 0; i < def->param_count;i++) {
		PHP_VAR_NODE *func_param = make_var_node();
		func_param->flags |= def->params[i].flags;
		add_param_2_scope(func_scope, func_param, def->params[i].name);
	}
	decl_node->func_decl->scope = func_scope;
	decl_node->func_decl->is_native = 1;
	decl_node->func_decl->native_ptr = def->func;
	decl_node->func_decl->name = strdup(def->name);
	
	add_func_2_scope(g_global_scope, decl_node);
}

/*
 * Set of "native" functions. Some doing same as their Zend counterparts
 * while others are amule specific.
 * 
 * The idea is to export internal amuleweb data into PHP script thru
 * set of built-in functions and objects:
 * 
 * $downloads = $aMule->GetDownloads();
 * 
 * $aMule->PauseFile($file_in_queue);
 * 
 * ...
 */
void php_engine_init()
{
	g_global_scope = make_scope_table(0, 0);
	
	g_current_scope = g_global_scope;
	
	g_scope_stack = new PHP_SCOPE_STACK_TYPE;
	
	// here built-in functions are supposed to be loaded
	//php_add_native_func("var_dump", php_native_var_dump);
	php_init_core_lib();
}

/*
 * This is heart of expression tree: evaluation. It's split into 2 functions
 * where 1 evaluates "value" of expression, and other evaluates "lvalue" i.e. assignable
 * entity from given subtree.
 */

void php_expr_eval(PHP_EXP_NODE *expr, PHP_VALUE_NODE *result)
{
	PHP_VALUE_NODE result_val, result_val_left;
	PHP_VAR_NODE *lval_node = 0;
	switch(expr->op) {
		case PHP_OP_VAL:
			result_val = expr->val_node;
			break;
		case PHP_OP_VAR:
			result_val = expr->var_node->value;
			break;
		case PHP_OP_ASS:
			php_expr_eval(expr->tree_node.right, &result_val);
			lval_node = php_expr_eval_lvalue(expr->tree_node.left);
			value_value_free(&lval_node->value);
			lval_node->value = result_val;
			break;
		case PHP_OP_ARRAY_BY_KEY:
			php_expr_eval(expr->tree_node.right, &result_val);
			lval_node = php_expr_eval_lvalue(expr->tree_node.left);
			cast_value_array(&lval_node->value);
			cast_value_str(&result_val);
			lval_node = array_get_by_key(&lval_node->value, &result_val);
			result_val = lval_node->value;
			break;
		case PHP_OP_FUNC_CALL:
			php_run_func_call(expr, &result_val);
			break;
		case PHP_OP_ADD:
		case PHP_OP_SUB:
		case PHP_OP_MUL:
		case PHP_OP_DIV:
			php_expr_eval(expr->tree_node.right, &result_val);
			php_expr_eval(expr->tree_node.left, &result_val_left);
			php_eval_simple_math(expr->op, &result_val_left, &result_val, &result_val);
			break;
    	case PHP_OP_SHL:
    	case PHP_OP_SHR:
    	case PHP_OP_OR:
    	case PHP_OP_AND:
    	case PHP_OP_XOR:
			php_expr_eval(expr->tree_node.right, &result_val);
			php_expr_eval(expr->tree_node.left, &result_val_left);
			php_eval_int_math(expr->op, &result_val_left, &result_val, &result_val);
			break;
		case PHP_OP_EQ:
		case PHP_OP_NEQ:
		case PHP_OP_GRT:
		case PHP_OP_LWR:
			php_expr_eval(expr->tree_node.right, &result_val);
			php_expr_eval(expr->tree_node.left, &result_val_left);
			php_eval_compare(expr->op, &result_val_left, &result_val, &result_val);
			break;
		case PHP_OP_PRINT:
			php_expr_eval(expr->tree_node.left, &result_val);
			cast_value_str(&result_val);
			//
			// I print to buffer
			printf(result_val.str_val);
			break;
		default: ;
			
	}
	if ( result ) {
		*result = result_val;
	}
}

PHP_VAR_NODE *php_expr_eval_lvalue(PHP_EXP_NODE *expr)
{
	PHP_VAR_NODE *lval_node = 0;

	PHP_VALUE_NODE index;
	index.type = PHP_VAL_NONE;
	
	switch(expr->op) {
		case PHP_OP_VAR:
			lval_node = expr->var_node;
			break;
		case PHP_OP_ARRAY_BY_KEY:
			lval_node = php_expr_eval_lvalue(expr->tree_node.left);
			if ( !lval_node ) {
				break;
			}

			cast_value_array(&lval_node->value);
			if ( expr->tree_node.right ) {
				php_expr_eval(expr->tree_node.right, &index);
				if ( index.type == PHP_VAL_NONE ) {
					// something got wrong: evaluation result is not a value
					return 0;
				}
				cast_value_str(&index);
				lval_node = array_get_by_key(&lval_node->value, &index);
			} else {
				// this is "$xxx[] = " construct.
				lval_node = array_push_back(&lval_node->value);
			}
		case PHP_OP_VAR_BY_EXP: // ${"xxx"}
			// should take variable from current scope
			break;
		case PHP_OP_OBJ_MEMBER: // $x->y
			// take variable from scope of current object
			break;
		case PHP_OP_CLASS_MEMBER: // A::y
			// take variable (static) from scope of current class
			break;
		default:
			//
			// Error: expression can not be taken as lvalue
			//
			php_report_error("This expression can't be used as lvalue", PHP_ERROR);
	}
	return lval_node;
}

PHP_VALUE_TYPE cast_type_resolve(PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2)
{
	if ( (op1->type == PHP_VAL_FLOAT) || (op2->type == PHP_VAL_FLOAT) ) {
		cast_value_fnum(op1);
		cast_value_fnum(op2);
		return PHP_VAL_FLOAT;
	} else {
		cast_value_dnum(op1);
		cast_value_dnum(op2);
		return PHP_VAL_INT;
	}
}

/*
 * Same as simple_math, but result is always bool
 */
void php_eval_compare(PHP_EXP_OP op, PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result)
{
	cast_type_resolve(op1, op2);
	result->type = PHP_VAL_BOOL;
	switch(op) {
		case PHP_OP_EQ:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->int_val = op1->int_val == op2->float_val;
			} else {
				result->int_val = op1->int_val == op2->int_val;
			}
			break;
		case PHP_OP_NEQ:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->int_val = op1->float_val != op2->float_val;
			} else {
				result->int_val = op1->int_val != op2->int_val;
			}
			break;
		case PHP_OP_GRT:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->int_val = op1->float_val > op2->float_val;
			} else {
				result->int_val = op1->int_val > op2->int_val;
			}
			break;
		case PHP_OP_LWR:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->int_val = op1->float_val < op2->float_val;
			} else {
				result->int_val = op1->int_val < op2->int_val;
			}
			break;
		default:
			php_report_error("This op is not compare op", PHP_INTERNAL_ERROR);
	}
}

void php_eval_simple_math(PHP_EXP_OP op, PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result)
{
	result->type = cast_type_resolve(op1, op2);
	switch(op) {
		case PHP_OP_ADD:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->float_val = op1->float_val + op2->float_val;
			} else {
				result->int_val = op1->int_val + op2->int_val;
			}
			break;
		case PHP_OP_SUB:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->float_val = op1->float_val - op2->float_val;
			} else {
				result->int_val = op1->int_val - op2->int_val;
			}
			break;
		case PHP_OP_MUL:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->float_val = op1->float_val * op2->float_val;
			} else {
				result->int_val = op1->int_val * op2->int_val;
			}
			break;
		case PHP_OP_DIV:
			if ( result->type == PHP_VAL_FLOAT ) {
				result->float_val = op1->float_val / op2->float_val;
			} else {
				result->int_val = op1->int_val / op2->int_val;
			}
			break;
		default:
			php_report_error("This op is not simple math", PHP_INTERNAL_ERROR);
	}
}

void php_eval_int_math(PHP_EXP_OP op, PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result)
{
	cast_value_dnum(op1);
	cast_value_dnum(op2);
	result->type = PHP_VAL_INT;
	switch(op) {
    	case PHP_OP_SHL:
    		result->int_val = op1->int_val << op2->int_val;
    		break;
    	case PHP_OP_SHR:
    		result->int_val = op1->int_val >> op2->int_val;
    		break;
    	case PHP_OP_OR:
    		result->int_val = op1->int_val | op2->int_val;
    		break;
    	case PHP_OP_AND:
    		result->int_val = op1->int_val & op2->int_val;
    		break;
    	case PHP_OP_XOR:
    		result->int_val = op1->int_val ^ op2->int_val;
		default:
			php_report_error("This op is not int math", PHP_INTERNAL_ERROR);
	}
}

//
// left = VAR(func_name), right=ARRAY(args)
void php_run_func_call(PHP_EXP_NODE *node, PHP_VALUE_NODE *result)
{
	PHP_EXP_NODE *l_node = node->tree_node.left;
	PHP_EXP_NODE *r_node = node->tree_node.right;
	if ( (l_node->op != PHP_OP_VAL) || (l_node->val_node.type != PHP_VAL_STRING) ||
		(r_node->op != PHP_OP_VAR) || (r_node->var_node->value.type != PHP_VAL_ARRAY) ) {
		//
		// Internal error: function name must be string value node, and
		// params must be an array
		//
		php_report_error("Function call node have wrong data", PHP_INTERNAL_ERROR);
		return ;
	}
	PHP_SCOPE_ITEM *si = get_scope_item(g_global_scope, l_node->val_node.str_val);
	if ( !si ) {
		//
		// Error: undeclared symbol
		//
		php_report_error("Function is not defined", PHP_ERROR);
		return;
	}
	if ( si->type != PHP_SCOPE_FUNC) {
		//
		// Error: defined, but wrong type !
		//
		php_report_error("This is not a function", PHP_ERROR);
		return;
	}
	PHP_SYN_NODE *func = si->func;
	if ( func->type != PHP_ST_FUNC_DECL ) {
		//
		// Internal error: node not a function
		//
		php_report_error("Wrong type in function decl node", PHP_INTERNAL_ERROR);
		return;
	}
	
	//
	// Switch stack and call function
	//
	PHP_SCOPE_TABLE new_table = make_scope_table(si->func->func_decl->scope, &r_node->var_node->value);
	switch_push_scope_table(new_table);
	if ( func->func_decl->is_native ) {
		func->func_decl->native_ptr(new_table, result);
	} else {
	}
	//
	// restore stack, free arg list
	//
	switch_pop_scope_table();
}

/*
 * Theoretically speaking this function must run on generated code. On the
 * practical side - I need it to debug syntax tree generation. Later, it can
 * be changes to generate code for some kind of bytecode for stack machine
 */
int php_execute(PHP_SYN_NODE *node, PHP_VALUE_NODE *result)
{
	if ( !node ) {
		php_report_error("Nothing to execute - top = 0", PHP_INTERNAL_ERROR);
		return 0;
	}
	int curr_exec_result;
	while ( node ) {
		curr_exec_result = 0;
		PHP_VALUE_NODE cond_result;
		switch (node->type) {
			case PHP_ST_EXPR:
				php_expr_eval(node->node_expr, 0);
				break;
			case PHP_ST_IF:
				php_expr_eval(node->node_if.cond, &cond_result);
				cast_value_bool(&cond_result);
				if ( cond_result.int_val ) {
					if ( node->node_if.code_if ) {
						curr_exec_result = php_execute(node->node_if.code_if, result);
					}
				} else {
					if ( node->node_if.code_else ) {
						curr_exec_result = php_execute(node->node_if.code_else, result);
					}
				}
				break;
			case PHP_ST_RET:
				if (  node->node_expr ) {
					php_expr_eval(node->node_expr, result);
				}
				if ( node->next_node ) {
					//
					// Warning: code after "return" statement
					//
					php_report_error("code after 'return'", PHP_WARNING);
				}
				// "return" is ultimate "break"
				curr_exec_result = -0xffff;
				break;
			case PHP_ST_CONTINUE:
			case PHP_ST_BREAK:
				if (  node->node_expr ) {
					php_expr_eval(node->node_expr, &cond_result);
				}
				break;
			case PHP_ST_WHILE:
				php_expr_eval(node->node_while.cond, &cond_result);
				cast_value_bool(&cond_result);
				while ( cond_result.int_val ) {
					curr_exec_result = php_execute(node->node_while.code, 0);
					if ( curr_exec_result) {
						break;
					}
					php_expr_eval(node->node_while.cond, &cond_result);
					cast_value_bool(&cond_result);
				}
			default: ;
		}
		if ( curr_exec_result != 0 ) {
			return curr_exec_result;
		}
		node = node->next_node;
	}
	// everything ok, keep going
	return 0;
}


//
// call it when something gone wrong
//
void php_report_error(char *msg, PHP_MSG_TYPE err_type)
{
	char *type_msg = 0;
	switch(err_type) {
		case PHP_MESAGE:
			type_msg = "PHP:";
			break;
		case PHP_WARNING:
			type_msg = "PHP Warning:";
			break;
		case PHP_ERROR:
			type_msg = "PHP Error:";
			break;
		case PHP_INTERNAL_ERROR:
			type_msg = "PHP Internal Error:";
			break;
	}
	
	printf("%s %s\n", type_msg, msg);
	assert(err_type != PHP_INTERNAL_ERROR);
}


void print_exp_node(PHP_EXP_NODE *node, int ident)
{
	for(int i = 0; i < ident;i++) {
		printf("\t");
	}
	switch(node->op) {
        case PHP_OP_VAR:
                printf("NODE VAR:  %p \n", node->var_node);
                php_var_dump(&node->var_node->value, ident + 1);
                break;
        case PHP_OP_VAL:
                printf("NODE VALUE: \n");
                php_var_dump(&node->val_node, ident + 1);
                break;
        case PHP_OP_ASS:
                printf("NODE: ASSIGN\n");
                print_exp_node(node->tree_node.left, ident + 1);
                print_exp_node(node->tree_node.right, ident + 1);
                break;
        default:
                printf("NODE: OP\n");
                print_exp_node(node->tree_node.left, ident + 1);
                print_exp_node(node->tree_node.right, ident + 1);
        }
}

void print_syn_node(PHP_SYN_NODE *node, int ident)
{
	for(int i = 0; i < ident;i++) {
		printf("\t");
	}
	switch(node->type) {
		default:
			printf("SYN_NODE: OP\n");
	}
}

int yyerror(char *s)
{
	printf("ERROR in grammar %s after %s\n", s, yytext);
	return 0;
}


int main()
{
	php_engine_init();
	yyin = fopen("test.php", "r");
	yydebug = 0;
	yyparse();
	
	PHP_VALUE_NODE val;
	php_execute(g_syn_tree_top, &val);

	return 0;
}
