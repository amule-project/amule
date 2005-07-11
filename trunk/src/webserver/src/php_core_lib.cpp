#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <list>
#include <map>
#include <string>

#include "php_syntree.h"
#include "php_core_lib.h"

/*
 * Built-in php functions. Those are both library and core internals.
 * 
 * I'm not going event to get near to what Zend provide, but
 * at least base things must be here
 */

/*
 * Print info about variable: php var_dump()
 */
void php_native_var_dump(PHP_SCOPE_TABLE scope, PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(scope, "var");
	if ( si ) {
		assert(si->type == PHP_SCOPE_VAR);
		php_var_dump(&si->var->value, 0);
	} else {
		php_report_error("Invalid or missing argument", PHP_ERROR);
	}
}

void php_var_dump(PHP_VALUE_NODE *node, int ident)
{
	for(int i = 0; i < ident;i++) {
		printf("\t");
	}
	switch(node->type) {
		case PHP_VAL_BOOL: printf("bool(%s)\n", node->int_val ? "true" : "false"); break;
		case PHP_VAL_INT: printf("int(%d)\n", node->int_val); break;
		case PHP_VAL_FLOAT: printf("float(%f)\n", node->float_val); break;
		case PHP_VAL_STRING: printf("string(%d) \"%s\"\n", strlen(node->str_val), node->str_val); break;
		case PHP_VAL_OBJECT: printf("Object\n"); break;
		case PHP_VAL_ARRAY: {
			int arr_size = array_get_size(node);
			printf("array(%d) {\n", arr_size);
			for(int i = 0; i < arr_size;i++) {
				const std::string &curr_key = array_get_ith_key(node, i);
				PHP_VAR_NODE *curr_val = array_get_by_str_key(node, curr_key);
				printf("\t[%s]=>\n", curr_key.c_str());
				php_var_dump(&curr_val->value, ident+1);
			}
			printf("}\n");
			break;
		}
		case PHP_VAL_NONE: printf("NULL\n"); break;
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
}

/*
 * String functions
 */
void php_native_strlen(PHP_SCOPE_TABLE scope, PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(scope, "str");
	PHP_VALUE_NODE *param = &si->var->value;
	if ( si ) {
		assert(si->type == PHP_SCOPE_VAR);
		cast_value_str(param);
		result->int_val = strlen(param->str_val);
		result->type = PHP_VAL_INT;
	} else {
		php_report_error("Invalid or missing argument", PHP_ERROR);
	}
}

void php_native_substr(PHP_SCOPE_TABLE scope, PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(scope, "str");
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( si_str ) {
		cast_value_str(str);
	} else {
		php_report_error("Invalid or missing argument", PHP_ERROR);
		return;
	}
	PHP_SCOPE_ITEM *si_start = get_scope_item(scope, "start");
	PHP_VALUE_NODE *start = &si_start->var->value;
	if ( si_start ) {
		cast_value_dnum(start);
	} else {
		php_report_error("Invalid or missing argument", PHP_ERROR);
		return;
	}
	// 3-rd is optional
	PHP_SCOPE_ITEM *si_end = get_scope_item(scope, "end");
	PHP_VALUE_NODE end = { PHP_VAL_INT, 0 };
	if ( si_end ) {
		end = si_end->var->value;
	}
	cast_value_dnum(&end);


}

PHP_BLTIN_FUNC_DEF core_lib_funcs[] = {
	{
		"var_dump", 
		{ "var", 0, PHP_VARFLAG_BYREF, { PHP_VAL_NONE } },
		1,
		php_native_var_dump,
	},
	{
		"strlen",
		{ "str", 0, 0, { PHP_VAL_NONE } },
		1, php_native_strlen,
	},
	{ 0 },
};

void php_init_core_lib()
{
	PHP_BLTIN_FUNC_DEF *curr_def = core_lib_funcs;
	while ( curr_def->name ) {
		printf("PHP_LIB: adding function '%s'\n", curr_def->name);
		php_add_native_func(curr_def);
		curr_def++;
	}
}
