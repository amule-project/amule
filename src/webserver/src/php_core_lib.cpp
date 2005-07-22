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
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <list>
#include <map>
#include <string>

#ifdef AMULEWEB_SCRIPT_EN
	#include "WebServer.h"
#endif

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
void php_native_var_dump(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
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
void php_native_strlen(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
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

void php_native_substr(PHP_VALUE_NODE * /*result*/)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( si_str ) {
		cast_value_str(str);
	} else {
		php_report_error("Invalid or missing argument 'str' for 'substr'", PHP_ERROR);
		return;
	}
	PHP_SCOPE_ITEM *si_start = get_scope_item(g_current_scope, "start");
	PHP_VALUE_NODE *start = &si_start->var->value;
	if ( si_start ) {
		cast_value_dnum(start);
	} else {
		php_report_error("Invalid or missing argument 'start' for 'substr'", PHP_ERROR);
		return;
	}
	// 3-rd is optional
	PHP_SCOPE_ITEM *si_end = get_scope_item(g_current_scope, "end");
	PHP_VALUE_NODE end = { PHP_VAL_INT, 0 };
	if ( si_end ) {
		end = si_end->var->value;
	}
	cast_value_dnum(&end);


}

/*
 * Load amule variables into interpreter scope.
 *  "varname" will tell us, what kind of variables need to load:
 *    "downloads", "uploads", "searchresult", "servers", "options" etc
 */
#ifdef AMULEWEB_SCRIPT_EN

void amule_load_downloads(PHP_VALUE_NODE *result)
{
	DownloadFileInfo *downloads = CPhPLibContext::g_curr_context->AmuleDownloads();
	downloads->ReQuery();
	for (std::list<DownloadFile>::const_iterator i = downloads->GetBeginIterator(); i != downloads->GetEndIterator(); i++) {
		PHP_VAR_NODE *var = array_push_back(result);
		var->value.type = PHP_VAL_OBJECT;
		var->value.obj_val.class_name = "AmuleDownloadFile";
		const DownloadFile *cur_item = &(*i);
		var->value.obj_val.inst_ptr = (void *)cur_item;
	}
}

#else

void amule_load_downloads(PHP_VALUE_NODE *result)
{
	for (int i = 0; i < 10; i++) {
		PHP_VAR_NODE *var = array_push_back(result);
		var->value.type = PHP_VAL_OBJECT;
		var->value.obj_val.class_name = "AmuleDownloadFile";
		var->value.obj_val.inst_ptr = 0;
	}
}

#endif

void php_native_load_amule_vars(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	if ( !si_str  ) {
		php_report_error("Missing argument 'varname' for 'load_amule_vars'", PHP_ERROR);
		return;
	}
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( str->type != PHP_VAL_STRING ) {
		php_report_error("Argument 'varname' for 'load_amule_vars' must be string", PHP_ERROR);
		return;
	}
	char *varname = str->str_val;
	if ( strcmp(varname, "downloads") == 0 ) {
		cast_value_array(result);
		amule_load_downloads(result);
	} else if ( strcmp(varname, "uploads") == 0 ) {
	} else if ( strcmp(varname, "searchresult") == 0 ) {
	} else if ( strcmp(varname, "servers") == 0 ) {
	} else {
		php_report_error("This type of amule variable is unknown", PHP_ERROR);
	}
}

/*
 * Amule objects implementations
 */
#ifdef AMULEWEB_SCRIPT_EN
void amule_download_file_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	printf("DEBUG: obj %p -> %s \n", ptr, prop_name);
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	DownloadFile *obj = (DownloadFile *)ptr;
	result->type = PHP_VAL_INT;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sFileName));
	} else if ( strcmp(prop_name, "status") == 0 ) {
		result->int_val = obj->nFileStatus;
	} else if ( strcmp(prop_name, "size") == 0 ) {
		result->int_val = obj->lFileSize;
	} else if ( strcmp(prop_name, "size_done") == 0 ) {
		result->int_val = obj->lFileCompleted;
	} else if ( strcmp(prop_name, "size_xfer") == 0 ) {
		result->int_val = obj->lFileTransferred;
	} else if ( strcmp(prop_name, "speed") == 0 ) {
		result->int_val = obj->lFileSpeed;
	} else if ( strcmp(prop_name, "src_count") == 0 ) {
		result->int_val = obj->lSourceCount;
	} else if ( strcmp(prop_name, "src_count_not_curr") == 0 ) {
		result->int_val = obj->lNotCurrentSourceCount;
	} else if ( strcmp(prop_name, "src_count_a4af") == 0 ) {
		result->int_val = obj->lSourceCountA4AF;
	} else if ( strcmp(prop_name, "src_count_xfer") == 0 ) {
		result->int_val = obj->lTransferringSourceCount;
	} else if ( strcmp(prop_name, "prio") == 0 ) {
		result->int_val = obj->lFilePrio;
	} else if ( strcmp(prop_name, "prio_auto") == 0 ) {
		result->int_val = obj->bFileAutoPriority;
	} else {
		php_report_error("This property is unknown", PHP_ERROR);
	}
}
#else
void amule_download_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	printf("DEBUG: obj %p -> %s \n", obj, prop_name);
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup("some_str");
	} else {
		result->type = PHP_VAL_INT;
		result->int_val = 10;
	}
}
#endif
/*
 * Set of "native" functions to access amule data
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

PHP_BLTIN_FUNC_DEF core_lib_funcs[] = {
	{
		"var_dump", 
		{ 0, 1, { PHP_VAL_NONE } },
		1,
		php_native_var_dump,
	},
	{
		"strlen",
		{ 0, 0, { PHP_VAL_NONE } },
		1, php_native_strlen,
	},
	{
		"load_amule_vars",
		{ 0, 0, { PHP_VAL_NONE } },
		1, php_native_load_amule_vars,
	},
	{ 0 },
};

void php_init_core_lib()
{
	// load function definitions
	PHP_BLTIN_FUNC_DEF *curr_def = core_lib_funcs;
	while ( curr_def->name ) {
		printf("PHP_LIB: adding function '%s'\n", curr_def->name);
		php_add_native_func(curr_def);
		curr_def++;
	}
	// load object definitions
	php_add_native_class("AmuleDownloadFile", amule_download_file_prop_get);
}

CPhPLibContext::CPhPLibContext(CWebServerBase *server, const char *file)
{
	g_curr_context = this;
#ifdef AMULEWEB_SCRIPT_EN
	m_downloads = &server->m_DownloadFileInfo;
	m_servers = &server->m_ServersInfo;
#endif
	php_engine_init();
	yyin = fopen(file, "r");
	yyparse();
	m_syn_tree_top = g_syn_tree_top;
	m_global_scope = g_global_scope;
}

CPhPLibContext::~CPhPLibContext()
{
	SetContext();
	php_engine_free();
}

void CPhPLibContext::SetContext()
{
	g_syn_tree_top = m_syn_tree_top;
	g_global_scope = m_global_scope;
}

void CPhPLibContext::Execute(CWriteStrBuffer *buf)
{
	m_curr_str_buffer = buf;
	
	PHP_VALUE_NODE val;
	php_execute(g_syn_tree_top, &val);
}

CPhPLibContext *CPhPLibContext::g_curr_context = 0;

/*
 * For simplicity and performance sake, this function can
 * only handle limited-length printf's. In should be NOT be used
 * for string concatenation like printf("xyz %s %s", s1, s2).
 * 
 * Engine will call Print for "print" and "echo"
 */
void CPhPLibContext::Printf(const char *str, ...)
{
	va_list args;
        
	va_start(args, str);
	if ( !g_curr_context || !g_curr_context->m_curr_str_buffer ) {
		printf(str, args);
	} else {
		char buf[4096];
		sprintf(buf, str, args);
		g_curr_context->m_curr_str_buffer->Write(buf);
	}
}

void CPhPLibContext::Print(const char *str)
{
	if ( !g_curr_context || !g_curr_context->m_curr_str_buffer ) {
		printf(str);
	} else {
		g_curr_context->m_curr_str_buffer->Write(str);
	}
}

/*
 * String buffer: almost same as regular 'string' class, but,
 * without reallocation when full. Instead, new buffer is
 * allocated, and added to list
 */
CWriteStrBuffer::CWriteStrBuffer()
{
	m_alloc_size = 16;
	m_total_length = 0;
	
	AllocBuf();
}

CWriteStrBuffer::~CWriteStrBuffer()
{
	for(std::list<char *>::iterator i = m_buf_list.begin(); i != m_buf_list.end(); i++) {
		delete [] *i;
	}
	delete [] m_curr_buf;
}

void CWriteStrBuffer::AllocBuf()
{
	m_curr_buf = new char [m_alloc_size];
	m_buf_ptr = m_curr_buf;
	m_curr_buf_left = m_alloc_size;
}

void CWriteStrBuffer::Write(const char *s)
{
	int len = strlen(s);
	m_total_length += len;
	
	while ( len ) {
		if ( (len + 1) <= m_curr_buf_left ) {
			strcpy(m_buf_ptr, s);
			m_buf_ptr += len;
			m_curr_buf_left -= len;
			len = 0;
		} else {
			memcpy(m_buf_ptr, s, m_curr_buf_left);
			int rem_len = len - m_curr_buf_left;
			s += m_curr_buf_left;
						
			len = rem_len;
			m_buf_list.push_back(m_curr_buf);
			AllocBuf();
		}
	}
}

void CWriteStrBuffer::CopyAll(char *dst_buffer)
{
	char *curr_ptr = dst_buffer;
	int rem_size = m_total_length;
	for(std::list<char *>::iterator i = m_buf_list.begin(); i != m_buf_list.end(); i++) {
		memcpy(curr_ptr, *i, m_alloc_size);
		rem_size -= m_alloc_size;
		curr_ptr += m_alloc_size;
	}
	int copy_size = m_alloc_size - m_curr_buf_left;
	memcpy(curr_ptr, m_curr_buf, copy_size);
	*(curr_ptr + copy_size) = 0;
}

