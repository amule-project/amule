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

/*
 * Syntax tree implementation for amule-PHP interpreter.
 */

#ifndef _PHP_SYNTREE_H_
#define _PHP_SYNTREE_H_

typedef enum PHP_VALUE_TYPE {
	/* simple values */
	PHP_VAL_NONE, PHP_VAL_INT, PHP_VAL_FLOAT, PHP_VAL_STRING, PHP_VAL_BOOL,
	/* both point to same map<string, var> but meaning is different */
	PHP_VAL_ARRAY, PHP_VAL_OBJECT,
	/* Internally used (not ref counted) data */
	PHP_VAL_INT_DATA,
	/* ptr_val points to VAR_NODE */
	PHP_VAL_VAR_NODE,
} PHP_VALUE_TYPE;

typedef struct PHP_VALUE_NODE {
    PHP_VALUE_TYPE type;
    union {
        unsigned int int_val;
        double float_val;
        char *str_val;
        /* used for arrays and internal objects:
         * * array contain std::map of key:value pairs
         * * object contain internally interpreted data
         */
        void *ptr_val;
        struct {
        	void *inst_ptr;
        	char *class_name;
        } obj_val;
    };
} PHP_VALUE_NODE;

/*
 * Flags for different variable types/usages
 */

#define PHP_VARFLAG_STATIC		0x0001
#define PHP_VARFLAG_GLOBAL		0x0002
#define PHP_VARFLAG_BYREF		0x0004
#define PHP_VARFLAG_PRIVATE		0x0004
#define PHP_VARFLAG_BUILTIN		0x0004

/*
  Data about variable.
*/
typedef struct PHP_VAR_NODE {
    PHP_VALUE_NODE value;
    /* php support references */
    int ref_count;
    int flags;
} PHP_VAR_NODE;


/*
 Node in expression tree. Contain either (left op right) or (value)
*/
typedef enum PHP_EXP_OP {
    PHP_OP_VAR, PHP_OP_VAL, PHP_OP_ASS,
    
    /* dereference */
    PHP_OP_ARRAY_BY_KEY, PHP_OP_VAR_BY_EXP,

	/* object access "->" and "::" */
	PHP_OP_OBJECT_DEREF, PHP_OP_CLASS_DEREF,
 
 	/* casting */
 	PHP_OP_CAST_INT, PHP_OP_CAST_FLOAT, PHP_OP_CAST_BOOL, PHP_OP_CAST_STR,
 	
    /* arithmetics */
    PHP_OP_MUL, PHP_OP_DIV, PHP_OP_ADD, PHP_OP_SUB, PHP_OP_REM,
    /* str concat */
    PHP_OP_CAT,
    /* bits */
    PHP_OP_SHL, PHP_OP_SHR, PHP_OP_OR, PHP_OP_AND, PHP_OP_XOR, PHP_OP_NOT,
    /* logical */
    PHP_OP_LOG_OR, PHP_OP_LOG_AND, PHP_OP_LOG_XOR, PHP_OP_LOG_NOT,
    /* compare */
    PHP_OP_EQ, PHP_OP_NEQ, PHP_OP_SAME, PHP_OP_NOT_SAME,
    PHP_OP_GRT, PHP_OP_GRT_EQ, PHP_OP_LWR, PHP_OP_LWR_EQ,

	/* conditional assign (mux) */
	PHP_OP_MUX,
	
	/* specials */
	PHP_OP_FUNC_CALL, PHP_OP_PRINT, PHP_OP_ECHO,
	/* list of expressions */
	PHP_OP_LIST,
} PHP_EXP_OP;

struct PHP_EXP_NODE {
    PHP_EXP_OP op;
    union {
        struct {
            struct PHP_EXP_NODE *left, *right;
        } tree_node;
        struct PHP_EXP_NODE *next;
    };
    union {
        PHP_VALUE_NODE val_node;
        PHP_VAR_NODE *var_node;
        struct PHP_EXP_NODE *exp_node;
    };
};

typedef struct PHP_EXP_NODE PHP_EXP_NODE;

typedef struct PHP_LIST_ASSIGN_NODE PHP_LIST_ASSIGN_NODE;

struct PHP_LIST_ASSIGN_NODE {
	int is_list;
	union {
		PHP_VAR_NODE *var;
		PHP_LIST_ASSIGN_NODE *list;
	};
	PHP_LIST_ASSIGN_NODE *next_node;
};

typedef struct PHP_FUNC_PARAM_ITEM PHP_FUNC_PARAM_ITEM;

struct PHP_FUNC_PARAM_ITEM {
	char *name;
	char *class_name;
	int byref;
	PHP_FUNC_PARAM_ITEM *next_item;
};

typedef struct PHP_SYN_NODE PHP_SYN_NODE;

/*
 * Scope table: holding variable definition and declarations for
 * functions and classes
 * Can be present in several locations:
 *  1. Representing stack frame block, inside of called function
 *  2. As global scope - holder of global vars, classes and global functions
 *  3. At class scope - holder of class members
 *  4. Copied to class instanse
 * 
 */
typedef enum PHP_SCOPE_ITEM_TYPE {
	PHP_SCOPE_NONE,
	PHP_SCOPE_VAR, PHP_SCOPE_FUNC, PHP_SCOPE_CLASS,
	PHP_SCOPE_PARAM,
} PHP_SCOPE_ITEM_TYPE;


typedef struct PHP_SCOPE_ITEM {
	PHP_SCOPE_ITEM_TYPE type;
	union {
		PHP_VAR_NODE *var;
		PHP_SYN_NODE *func;
		PHP_SYN_NODE *class_decl;
		struct {
			PHP_VAR_NODE *var;
			int num;
		} param;
	};
} PHP_SCOPE_ITEM;

/* thre's stl object behind it */
typedef void *PHP_SCOPE_TABLE;
typedef void *PHP_SCOPE_STACK;

/*
 Syntax tree node, representing 1 statement.
*/
typedef enum PHP_STATMENT_TYPE {
	PHP_ST_EXPR, PHP_ST_IF,
	PHP_ST_WHILE, PHP_ST_DO_WHILE, PHP_ST_FOR, PHP_ST_FOREACH, PHP_ST_SWITCH,
	PHP_ST_CONTINUE, PHP_ST_BREAK, PHP_ST_RET,
	PHP_ST_FUNC_DECL, PHP_ST_CLASS_DECL,
	PHP_ST_ECHO,
} PHP_STATMENT_TYPE;

/* 
 * Syntax tree constructs: regular statements and declarations
 */
typedef struct PHP_SYN_IF_NODE {
    PHP_EXP_NODE *cond;
    PHP_SYN_NODE *code_if, *code_else;
} PHP_SYN_IF_NODE;

typedef struct PHP_SYN_WHILE_NODE {
    PHP_EXP_NODE *cond;
    PHP_SYN_NODE *code;
} PHP_SYN_WHILE_NODE;

typedef struct PHP_SYN_FOR_NODE {
    PHP_EXP_NODE *do_start, *cond, *do_next;
    PHP_SYN_NODE *code;
} PHP_SYN_FOR_NODE;

typedef struct PHP_SYN_FOREACH_NODE {
    PHP_EXP_NODE *elems;
  	PHP_VAR_NODE *i_key;
  	PHP_VAR_NODE *i_val;
    PHP_SYN_NODE *code;
    int byref;
} PHP_SYN_FOREACH_NODE;

/* for built-in or native functions */
typedef void (*PHP_NATIVE_FUNC_PTR)(PHP_VALUE_NODE *result);

typedef struct PHP_FUNC_PARAM_DEF {
	char *class_name;
	int byref;
	PHP_VALUE_NODE def_value;
	PHP_VAR_NODE *var;
} PHP_FUNC_PARAM_DEF;


typedef struct PHP_SYN_FUNC_DECL_NODE {
	char *name;
	PHP_SCOPE_TABLE scope;
	int is_native;
	union {
		PHP_SYN_NODE *code;
		PHP_NATIVE_FUNC_PTR native_ptr;
	};
	int param_count;
	PHP_FUNC_PARAM_DEF *params;
} PHP_SYN_FUNC_DECL_NODE;

/*
 * Evaluating $obj->some_field for built-in objects
 */
typedef void (*PHP_NATIVE_PROP_GET_FUNC_PTR)(void *obj, char *prop_name, PHP_VALUE_NODE *result);

typedef struct PHP_SYN_CLASS_DECL_NODE {
	int is_native;
	char *name;
	union {
		PHP_SCOPE_TABLE decl_scope;
		PHP_NATIVE_PROP_GET_FUNC_PTR native_prop_get_ptr;
	};
} PHP_SYN_CLASS_DECL_NODE;

struct PHP_SYN_NODE {
    PHP_STATMENT_TYPE type;
    union {
        PHP_EXP_NODE 			*node_expr;
        PHP_SYN_IF_NODE			node_if;
        PHP_SYN_WHILE_NODE		node_while;
        PHP_SYN_FOREACH_NODE	node_foreach;
        PHP_SYN_FOR_NODE		node_for;
        PHP_SYN_FUNC_DECL_NODE	*func_decl;
        PHP_SYN_CLASS_DECL_NODE *class_decl;
    };
    PHP_SYN_NODE *next_node;
};

/*
 * Interface to lib of built-in functions, classes, variables
 */
/*
 * Using fixed size array will allow "in-place" definition 
 * of built-in functions without pointer mess.
 * 
 */
#define PHP_MAX_FUNC_PARAM	 16

typedef struct PHP_BLTIN_FUNC_DEF {
	char *name;
	PHP_FUNC_PARAM_DEF params[PHP_MAX_FUNC_PARAM];
	int param_count;
	PHP_NATIVE_FUNC_PTR func;
} PHP_BLTIN_FUNC_DEF;

typedef enum PHP_MSG_TYPE {
	PHP_MESAGE, PHP_WARNING, PHP_ERROR, PHP_INTERNAL_ERROR 
} PHP_MSG_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * lex/yacc stuff
 */
	int yyerror(char *err);
	int yyparse();
	
	extern int yydebug;
	extern FILE *yyin;
	extern char *yytext;
	//extern int yylineno;

/* 
 * Syntax tree interface to parser
 */
 
/* 
 * Const expressions
 */
 	PHP_EXP_NODE *make_const_exp_dnum(int number);
 	PHP_EXP_NODE *make_const_exp_fnum(float number);
 	PHP_EXP_NODE *make_const_exp_str(char *s);

	// exp node for internally handled data
	PHP_EXP_NODE *make_const_exp_int_obj(void *obj);

	/* casting functions */
	void cast_value_dnum(PHP_VALUE_NODE *e);
	void cast_value_fnum(PHP_VALUE_NODE *e);
	void cast_value_str(PHP_VALUE_NODE *e);
	void cast_value_array(PHP_VALUE_NODE *e);
	void cast_value_bool(PHP_VALUE_NODE *e);
	
	void value_value_free(PHP_VALUE_NODE *val);
	void value_value_assign(PHP_VALUE_NODE *src, PHP_VALUE_NODE *dst);
	
	/* array operations */
	PHP_VAR_NODE *array_get_by_key(PHP_VALUE_NODE *array, PHP_VALUE_NODE *key);
	PHP_VAR_NODE *array_get_by_int_key(PHP_VALUE_NODE *array, int key);
	
	int array_is_key_here(PHP_VALUE_NODE *array, PHP_VALUE_NODE *key);
	int array_get_size(PHP_VALUE_NODE *array);
	PHP_VAR_NODE *array_push_back(PHP_VALUE_NODE *array);
	
	void array_add_to_int_key(PHP_VALUE_NODE *array, int key, PHP_VAR_NODE *node);
	
	PHP_VAR_NODE *make_array_var();
	
	// signle operand expression:
	// FIXME: prefix and postfix form not recognized
 	PHP_EXP_NODE *make_exp_1(PHP_EXP_OP op, PHP_EXP_NODE *operand);

 	PHP_EXP_NODE *make_exp_2(PHP_EXP_OP op, PHP_EXP_NODE *left, PHP_EXP_NODE *right);

	// this is for "OP=" forms
 	PHP_EXP_NODE *make_exp_2_self(PHP_EXP_OP op, PHP_EXP_NODE *self, PHP_EXP_NODE *right);

	PHP_EXP_NODE *make_known_const(char *name);
		
	PHP_EXP_NODE *make_func_call_exp(char *func_name, PHP_EXP_NODE *args);
	
	// create func call param list
	PHP_EXP_NODE *make_func_call_param_list();
	
	// add next argument to function call param list
	void func_call_add_expr(PHP_VAR_NODE *paramlist, PHP_EXP_NODE *arg, int byref);
	
	
	extern PHP_SYN_NODE *g_syn_tree_top;
	
	/* make syntax node for expression */
	PHP_SYN_NODE *make_expr_syn_node(PHP_STATMENT_TYPE type, PHP_EXP_NODE *node);
	
	PHP_SYN_NODE *make_ifelse_syn_node(PHP_EXP_NODE *expr,
		PHP_SYN_NODE *then_node, PHP_SYN_NODE *elseif_list, PHP_SYN_NODE *else_node);
		
	PHP_SYN_NODE *make_while_loop_syn_node(PHP_EXP_NODE *cond,
		PHP_SYN_NODE *code, int do_while);
		
	PHP_SYN_NODE *make_foreach_loop_syn_node(PHP_EXP_NODE *elems,
		PHP_VAR_NODE *i_key, PHP_VAR_NODE *i_val, PHP_SYN_NODE *code, int byref);

	PHP_SYN_NODE *make_for_syn_node(PHP_EXP_NODE *start, PHP_EXP_NODE *cond,
		PHP_EXP_NODE *next, PHP_SYN_NODE *code);
		
	PHP_SYN_NODE *make_class_decl_syn_node();
	
	PHP_SYN_NODE *make_func_decl_syn_node(char *name, PHP_EXP_NODE *param_list);
	
	//
	// add new item into function param list (in declaration )
	//
	PHP_EXP_NODE *make_func_param(PHP_EXP_NODE *list, PHP_EXP_NODE *var_exp_node,
		char *class_name, int byref);
	
	PHP_VAR_NODE *make_var_node();
	PHP_EXP_NODE *get_var_node(char *name);
	// C can't call "delete"
	void free_var_node(PHP_VAR_NODE *v);
	
	/* scope table manipulation */
	extern PHP_SCOPE_TABLE g_global_scope, g_current_scope;
	extern PHP_SCOPE_STACK g_scope_stack;
	
	PHP_SCOPE_TABLE make_scope_table();
	
	void delete_scope_table(PHP_SCOPE_TABLE scope);
	
	void switch_push_scope_table(PHP_SCOPE_TABLE new_table);
	
	void switch_pop_scope_table(int old_free);
	
	void scope_reset_nonstatics(PHP_SCOPE_TABLE scope);
	
	void add_func_2_scope(PHP_SCOPE_TABLE scope, PHP_SYN_NODE *func);
	
	void add_class_2_scope(PHP_SCOPE_TABLE scope, PHP_SYN_NODE *class_node);
	
	void add_var_2_scope(PHP_SCOPE_TABLE scope, PHP_VAR_NODE *var, const char *name);

	const char *get_scope_var_name(PHP_SCOPE_TABLE scope, PHP_VAR_NODE *var);
	
	PHP_SCOPE_ITEM_TYPE get_scope_item_type(PHP_SCOPE_TABLE scope, const char *name);
	
	PHP_SCOPE_ITEM *get_scope_item(PHP_SCOPE_TABLE scope, const char *name);
	
	/* engine */	
	void php_engine_init();
	void php_engine_free();
	
	void php_syn_tree_free(PHP_SYN_NODE *tree);
	
	/*
	 * Return code meaning:
	 *  0  : continue execution to the next statement
	 *  +x : return and skip current loop cycle, as "continue expr" means
	 *  -x : return and break as "break expr" means
	 *  In non-loop situation any != 0 code means "return"
	 */
	int php_execute(PHP_SYN_NODE *node, PHP_VALUE_NODE *result);
	
	void php_expr_eval(PHP_EXP_NODE *expr, PHP_VALUE_NODE *result);
	
	PHP_VAR_NODE *php_expr_eval_lvalue(PHP_EXP_NODE *expr);
	
	void php_eval_simple_math(PHP_EXP_OP op,
		PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result);
		
	void php_eval_int_math(PHP_EXP_OP op,
		PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result);
		
	void php_eval_compare(PHP_EXP_OP op,
		PHP_VALUE_NODE *op1, PHP_VALUE_NODE *op2, PHP_VALUE_NODE *result);

	void php_add_native_func(PHP_BLTIN_FUNC_DEF *def);
	void php_add_native_class(char *name, PHP_NATIVE_PROP_GET_FUNC_PTR prop_get_native_ptr);

	//
	// left = VAR(func_name), right=ARRAY(args)
	void php_run_func_call(PHP_EXP_NODE *node, PHP_VALUE_NODE *result);

	void php_report_error(PHP_MSG_TYPE mtype, char *msg, ...);
	
/*
 * Debugging
 */
	void print_val_node(PHP_VALUE_NODE *node, int ident);
	void print_exp_node(PHP_EXP_NODE *node, int ident);
	void print_syn_node(PHP_SYN_NODE *node, int ident);

#ifdef __cplusplus
}
#endif

/*
 * C++ only functions, type definitions
 */
#ifdef __cplusplus

typedef std::map<std::string, PHP_VAR_NODE *>::iterator PHP_ARRAY_ITER_TYPE;
typedef std::list<std::string>::iterator PHP_ARRAY_KEY_ITER_TYPE;
//
// In php arrays are behave like hashes (i.e. associative) and are sortable.
// STL std::map is not sortable.
//
typedef struct {
	std::map<std::string, PHP_VAR_NODE *> array;
	std::list<std::string> sorted_keys;
	PHP_ARRAY_KEY_ITER_TYPE current;
} PHP_ARRAY_TYPE;

//
// using std::string instead of "char *" so keys will be compared 
// by string value
typedef std::map<std::string, PHP_SCOPE_ITEM *> PHP_SCOPE_TABLE_TYPE;
typedef std::list<PHP_SCOPE_TABLE_TYPE *> PHP_SCOPE_STACK_TYPE;

const std::string &array_get_ith_key(PHP_VALUE_NODE *array, int i);
PHP_VAR_NODE *array_get_by_str_key(PHP_VALUE_NODE *array, std::string key);
void array_add_to_str_key(PHP_VALUE_NODE *array, std::string key, PHP_VAR_NODE *node);

void func_scope_init(PHP_FUNC_PARAM_DEF *params, int param_count,
	PHP_SCOPE_TABLE_TYPE *scope_map, PHP_VALUE_NODE *arg_array);


#endif

#endif //_PHP_SYNTREE_H_
