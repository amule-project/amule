#ifndef _PHP_CORE_LIB_H_
#define _PHP_CORE_LIB_H_

void php_init_core_lib();

void php_native_var_dump(PHP_SCOPE_TABLE scope, PHP_VALUE_NODE *result);
void php_var_dump(PHP_VALUE_NODE *node, int ident);

#endif //_PHP_CORE_LIB_H_
