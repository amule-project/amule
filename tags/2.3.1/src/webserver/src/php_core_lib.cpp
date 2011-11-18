//
// This file is part of the aMule Project.

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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <string> // Do_not_auto_remove (g++-4.0.1)

#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif

#ifdef PHP_STANDALONE_EN
#	include <map>
#	include <string>
#	include <list>
#	include <regex.h>
#else
#	include "WebServer.h"
#	include <ec/cpp/ECSpecialTags.h>
#	include <wx/regex.h>
#	include <wx/datetime.h>
#endif

#include "php_syntree.h"
#include "php_core_lib.h"
#include <stdarg.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/*
 * Built-in php functions. Those are both library and core internals.
 * 
 * I'm not going event to get near to what Zend provide, but
 * at least base things must be here
 */

/*
 * Print info about variable: php var_dump()
 */
void php_var_dump(PHP_VALUE_NODE *node, int ident, int ref)
{
	for(int i = 0; i < ident;i++) {
		printf("\t");
	}
	if ( ref ) printf("&");
	switch(node->type) {
		case PHP_VAL_BOOL: printf("bool(%s)\n", node->int_val ? "true" : "false"); break;
		case PHP_VAL_INT: printf("int(%"PRIu64")\n", node->int_val); break;
		case PHP_VAL_FLOAT: printf("float(%f)\n", node->float_val); break;
		case PHP_VAL_STRING: printf("string(%d) \"%s\"\n", (int)strlen(node->str_val), node->str_val); break;
		case PHP_VAL_OBJECT: printf("Object(%s)\n", node->obj_val.class_name); break;
		case PHP_VAL_ARRAY: {
			int arr_size = array_get_size(node);
			printf("array(%d) {\n", arr_size);
			for(int i = 0; i < arr_size;i++) {
				const std::string &curr_key = array_get_ith_key(node, i);
				PHP_VAR_NODE *curr_val = array_get_by_str_key(node, curr_key);
				printf("\t[%s]=>\n", curr_key.c_str());
				php_var_dump(&curr_val->value, ident+1, curr_val->ref_count > 1);
			}
			for(int i = 0; i < ident;i++) {
				printf("\t");
			}
			printf("}\n");
			break;
		}
		case PHP_VAL_NONE: printf("NULL\n"); break;
		case PHP_VAL_VAR_NODE:
		case PHP_VAL_INT_DATA: assert(0); break;
	}
}

void php_native_var_dump(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( si ) {
		assert((si->type == PHP_SCOPE_VAR)||(si->type == PHP_SCOPE_PARAM));
		php_var_dump(&si->var->value, 0, 0);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
	}
}


/*
 * Sorting stl-way requires operator ">"
 */
class SortElem {
	public:
		SortElem() {}
		SortElem(PHP_VAR_NODE *p) { obj = p; }

		PHP_VAR_NODE *obj;
		static PHP_SYN_FUNC_DECL_NODE *callback;
		
		friend bool operator<(const SortElem &o1, const SortElem &o2);
};

PHP_SYN_FUNC_DECL_NODE *SortElem::callback = 0;

bool operator<(const SortElem &o1, const SortElem &o2)
{
	PHP_VALUE_NODE result;

	value_value_assign(&SortElem::callback->params[0].si_var->var->value, &o1.obj->value);
	value_value_assign(&SortElem::callback->params[1].si_var->var->value, &o2.obj->value);
	
	switch_push_scope_table((PHP_SCOPE_TABLE_TYPE *)SortElem::callback->scope);

	//
	// params passed by-value, all & notations ignored
	//
	result.type = PHP_VAL_NONE;
	php_execute(SortElem::callback->code, &result);
	cast_value_dnum(&result);
	//
	// restore stack, free arg list
	//
	switch_pop_scope_table(0);

	value_value_free(&SortElem::callback->params[0].si_var->var->value);
	value_value_free(&SortElem::callback->params[1].si_var->var->value);
	
	return result.int_val != 0;
}

void php_native_usort(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_ARRAY)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument (array)");
		return;
	}
	PHP_VAR_NODE *array = si->var;
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument (func name)");
		return;
	}
	char *cmp_func_name = si->var->value.str_val;
	si = get_scope_item(g_global_scope, cmp_func_name);
	if ( !si || (si->type != PHP_SCOPE_FUNC)) {
		php_report_error(PHP_ERROR, "Compare function [%s] not found", cmp_func_name);
		return;
	}
	PHP_SYN_FUNC_DECL_NODE *func_decl = si->func->func_decl;

	//
	// usort invalidates keys, and sorts values
	//
	PHP_ARRAY_TYPE *arr_obj = (PHP_ARRAY_TYPE *)array->value.ptr_val;
	//
	// create vector of values
	//
	if ( arr_obj->array.size() == 0 ) {
		php_report_error(PHP_WARNING, "Sorting array of size 0");
		return;
	}

	std::list<SortElem> sort_list;
	for(PHP_ARRAY_ITER_TYPE i = arr_obj->array.begin(); i != arr_obj->array.end(); i++) {
		sort_list.push_back(SortElem(i->second));
	}
	SortElem::callback = func_decl;
	sort_list.sort();

	arr_obj->array.clear();
	arr_obj->sorted_keys.clear();
	unsigned int key = 0;
	for(std::list<SortElem>::iterator i = sort_list.begin(); i != sort_list.end(); i++) {
		array_add_to_int_key(&array->value, key++, i->obj);
	}

}


/*
 * String functions
 */
void php_native_strlen(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( si ) {
		PHP_VALUE_NODE *param = &si->var->value;
		cast_value_str(param);
		if ( result ) {
			cast_value_dnum(result);
			result->int_val = strlen(param->str_val);
		}
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
	}
}

void php_native_count(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( si ) {
		PHP_VALUE_NODE *param = &si->var->value;
		if ( result ) {
			cast_value_dnum(result);
			if ( (si->var->value.type == PHP_VAL_NONE) || (si->var->value.type != PHP_VAL_ARRAY) ) {
				result->int_val = 0;
			} else {
				result->int_val = array_get_size(param);
			}
		}
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
	}
}

void php_native_isset(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( si ) {
		PHP_VALUE_NODE *param = &si->var->value;
		cast_value_str(param);
		if ( result ) {
			cast_value_bool(result);
			result->int_val = (si->var->value.type == PHP_VAL_NONE) ? 0 : 1;
		}
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
	}
}

void php_native_substr(PHP_VALUE_NODE * /*result*/)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( si_str ) {
		cast_value_str(str);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'str' for 'substr'");
		return;
	}
	PHP_SCOPE_ITEM *si_start = get_scope_item(g_current_scope, "__param_1");
	PHP_VALUE_NODE *start = &si_start->var->value;
	if ( si_start ) {
		cast_value_dnum(start);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'start' for 'substr'");
		return;
	}
	// 3-rd is optional
	PHP_SCOPE_ITEM *si_end = get_scope_item(g_current_scope, "__param_2");
	PHP_VALUE_NODE end = { PHP_VAL_INT, { 0 } };
	if ( si_end ) {
		end = si_end->var->value;
	}
	cast_value_dnum(&end);


}


void php_native_split(PHP_VALUE_NODE *result)
{
	if ( result ) {
		cast_value_array(result);
	} else {
		return; 
	}
	PHP_VALUE_NODE *pattern, *string_to_split, *split_limit;
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( si ) {
		pattern = &si->var->value;
		cast_value_str(pattern);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument: pattern");
		return;
	}
	si = get_scope_item(g_current_scope, "__param_1");
	if ( si ) {
		string_to_split = &si->var->value;
		cast_value_str(string_to_split);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument: string");
		return;
	}
	si = get_scope_item(g_current_scope, "__param_2");
	if ( si ) {
		split_limit = &si->var->value;
		cast_value_dnum(split_limit);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument: string");
		return;		
	}
#ifdef PHP_STANDALONE_EN
	regex_t preg;
	char error_buff[256];
	int reg_result = regcomp(&preg, pattern->str_val, REG_EXTENDED);
	if ( reg_result ) {
		regerror(reg_result, &preg, error_buff, sizeof(error_buff));
		php_report_error(PHP_ERROR, "Failed in regcomp: %s", error_buff);
#else
	wxRegEx preg;
	if (!preg.Compile(wxString(char2unicode(pattern->str_val)), wxRE_EXTENDED)) {
		php_report_error(PHP_ERROR, "Failed in Compile of: %s", pattern->str_val);
#endif
		return;
	}

#ifdef PHP_STANDALONE_EN
	size_t nmatch = strlen(string_to_split->str_val);
	regmatch_t *pmatch = new regmatch_t[nmatch];
#endif
	char *str_2_match = string_to_split->str_val;
	char *tmp_buff = new char[strlen(string_to_split->str_val)+1];
	
	while ( 1 ) {
//		printf("matching: %s\n", str_2_match);
#ifdef PHP_STANDALONE_EN
		reg_result = regexec(&preg, str_2_match, nmatch, pmatch, 0);
		if ( reg_result ) {
#else
		if (!preg.Matches(wxString(char2unicode(str_2_match)))) {
#endif
			// no match
			break;
		}
#ifndef PHP_STANDALONE_EN
		// get matching position
		size_t start, len;
		if (!preg.GetMatch(&start, &len)) {
			break;	// shouldn't happen
		}
#endif	
		/*
		 * I will use only first match, since I don't see any sense to have more
		 * then 1 match in split() call
		 */
#ifdef PHP_STANDALONE_EN
		for(int i = 0; i < pmatch[0].rm_so; i++) {
#else
		for(size_t i = 0; i < start; i++) {
#endif
			tmp_buff[i] = str_2_match[i];
		}
#ifdef PHP_STANDALONE_EN
		tmp_buff[pmatch[0].rm_so] = 0;
#else
		tmp_buff[start] = 0;
#endif
//		printf("Match added [%s]\n", tmp_buff);
		
		PHP_VAR_NODE *match_val = array_push_back(result);
		match_val->value.type = PHP_VAL_STRING;
		match_val->value.str_val = strdup(tmp_buff);

#ifdef PHP_STANDALONE_EN
		str_2_match += pmatch[0].rm_eo;
#else
		str_2_match += start + len;
#endif
	}

	PHP_VAR_NODE *match_val = array_push_back(result);
	match_val->value.type = PHP_VAL_STRING;
	match_val->value.str_val = strdup(str_2_match);
	
	delete [] tmp_buff;
#ifdef PHP_STANDALONE_EN
	delete [] pmatch;
	regfree(&preg);
#endif
}

#ifdef ENABLE_NLS

void php_native_gettext(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( si_str ) {
		cast_value_str(str);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'msgid' for 'gettext'");
		return;
	}
	if ( result ) {
		cast_value_dnum(result);
		result->type = PHP_VAL_STRING;
		result->str_val = strdup(gettext(str->str_val));
	}
}

void php_native_gettext_noop(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( si_str ) {
		cast_value_str(str);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'msgid' for 'gettext_noop'");
		return;
	}
	if ( result ) {
		cast_value_dnum(result);
		result->type = PHP_VAL_STRING;
		result->str_val = strdup(str->str_val);
	}
}

void php_native_ngettext(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_msgid = get_scope_item(g_current_scope, "__param_0");
	PHP_VALUE_NODE *msgid = &si_msgid->var->value;
	if ( si_msgid ) {
		cast_value_str(msgid);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'msgid' for 'ngettext'");
		return;
	}
	PHP_SCOPE_ITEM *si_msgid_plural = get_scope_item(g_current_scope, "__param_1");
	PHP_VALUE_NODE *msgid_plural = &si_msgid_plural->var->value;
	if ( si_msgid_plural ) {
		cast_value_str(msgid_plural);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'msgid_plural' for 'ngettext'");
		return;
	}
	PHP_SCOPE_ITEM *si_count = get_scope_item(g_current_scope, "__param_2");
	PHP_VALUE_NODE *count = &si_count->var->value;
	if ( si_count ) {
		cast_value_dnum(count);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'count' for 'ngettext'");
		return;
	}
	if ( result ) {
		cast_value_dnum(result);
		result->type = PHP_VAL_STRING;
		result->str_val = strdup(ngettext(msgid->str_val, msgid_plural->str_val, count->int_val));
	}
}
#endif

PHP_BLTIN_FUNC_DEF core_lib_funcs[] = {
	{
		"var_dump", 
		1,
		php_native_var_dump,
	},
	{
		"strlen",
		1, php_native_strlen,
	},
	{
		"count",
		1, php_native_count,
	},
	{
		"isset",
		1, php_native_isset,
	},
	{
		"usort",
		2,
		php_native_usort,
	},
	{
		"split",
		3,
		php_native_split,
	},
#ifdef ENABLE_NLS
	{
		"_",
		1, php_native_gettext,
	},
	{
		"gettext",
		1, php_native_gettext,
	},
	{
		"gettext_noop",
		1, php_native_gettext_noop,
	},
	{
		"ngettext",
		3, php_native_ngettext,
	},
#endif
	{ 0, 0, 0, },
};

void php_init_core_lib()
{
	// load function definitions
	PHP_BLTIN_FUNC_DEF *curr_def = core_lib_funcs;
	while ( curr_def->name ) {
		php_add_native_func(curr_def);
		curr_def++;
	}
}

//
// lexer has no include file
//
extern "C"
void php_set_input_buffer(char *buf, int len);

CPhPLibContext::CPhPLibContext(CWebServerBase *server, const char *file)
{
	g_curr_context = this;

	m_server = server;

	php_engine_init();
	phpin = fopen(file, "r");
	if ( !phpin ) {
		return;
	}

	phpparse();
	
	m_syn_tree_top = g_syn_tree_top;
	m_global_scope = g_global_scope;
}

CPhPLibContext::CPhPLibContext(CWebServerBase *server, char *php_buf, int len)
{
	g_curr_context = this;

	m_server = server;

	php_engine_init();

	m_global_scope = g_global_scope;

	php_set_input_buffer(php_buf, len);
	phpparse();
	
	m_syn_tree_top = g_syn_tree_top;
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
		vprintf(str, args);
	} else {
		char buf[4096];
		vsnprintf(buf, sizeof(buf), str, args);
		g_curr_context->m_curr_str_buffer->Write(buf);
	}
	va_end(args);
}

void CPhPLibContext::Print(const char *str)
{
	if ( !g_curr_context || !g_curr_context->m_curr_str_buffer ) {
		printf("%s", str);
	} else {
		g_curr_context->m_curr_str_buffer->Write(str);
	}
}


CPhpFilter::CPhpFilter(CWebServerBase *server, CSession *sess,
			const char *file, CWriteStrBuffer *buff)
{
	FILE *f = fopen(file, "r");
	if ( !f ) {
		printf("ERROR: php can not open source file [%s]\n", file);
		return;
	}
	if ( fseek(f, 0, SEEK_END) != 0 ) {
		printf("ERROR: fseek failed on php source file [%s]\n", file); 
		return;
	}
	int size = ftell(f);
	char *buf = new char [size+1];
	rewind(f);
	// fread may actually read less if it is a CR-LF-file in Windows
	size = fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);
	char *scan_ptr = buf;
	char *curr_code_end = buf;
	while ( strlen(scan_ptr) ) {
		scan_ptr = strstr(scan_ptr, "<?php");
		if ( !scan_ptr ) {
			buff->Write(curr_code_end);
			break;
		}
		if ( scan_ptr != curr_code_end ) {
			buff->Write(curr_code_end, scan_ptr - curr_code_end);
		}
		curr_code_end = strstr(scan_ptr, "?>");
		if ( !curr_code_end ) {
			break;
		}
		curr_code_end += 2; // include "?>" in buffer

		int len = curr_code_end - scan_ptr;

		CPhPLibContext *context = new CPhPLibContext(server, scan_ptr, len);

#ifndef PHP_STANDALONE_EN
		load_session_vars("HTTP_GET_VARS", sess->m_get_vars);
		load_session_vars("_SESSION", sess->m_vars);
#endif

		context->Execute(buff);

#ifndef PHP_STANDALONE_EN
		save_session_vars(sess->m_vars);
#endif

		delete context;
		
		scan_ptr = curr_code_end;
	}

#ifndef PHP_STANDALONE_EN
	sess->m_get_vars.clear();
#endif

	delete [] buf;
}


/*
 * String buffer: almost same as regular 'string' class, but,
 * without reallocation when full. Instead, new buffer is
 * allocated, and added to list
 */
CWriteStrBuffer::CWriteStrBuffer()
{
	m_alloc_size = 1024;
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

void CWriteStrBuffer::Write(const char *s, int len)
{
	if ( len == -1 ) {
		len = strlen(s);
	}
	m_total_length += len;
	
	while ( len ) {
		if ( (len + 1) <= m_curr_buf_left ) {
			strncpy(m_buf_ptr, s, len);
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
	if ( rem_size ) {
		memcpy(curr_ptr, m_curr_buf, rem_size);
	}
	*(curr_ptr + rem_size) = 0;
}

void load_session_vars(const char *target, std::map<std::string, std::string> &varmap)
{
	PHP_EXP_NODE *sess_vars_exp_node = get_var_node(target);
	PHP_VAR_NODE *sess_vars = sess_vars_exp_node->var_si_node->var;
	// i'm not building exp tree, node not needed
	delete sess_vars_exp_node;
	cast_value_array(&sess_vars->value);
	for(std::map<std::string, std::string>::iterator i = varmap.begin(); i != varmap.end(); i++) {
		PHP_VAR_NODE *curr_var = array_get_by_str_key(&sess_vars->value, i->first);
		PHP_VALUE_NODE val;
		val.type = PHP_VAL_STRING;
		val.str_val = const_cast<char *>(i->second.c_str());
		value_value_assign(&curr_var->value, &val);
	}
}

void save_session_vars(std::map<std::string, std::string> &varmap)
{
	PHP_EXP_NODE *sess_vars_exp_node = get_var_node("_SESSION");
	PHP_VAR_NODE *sess_vars = sess_vars_exp_node->var_si_node->var;

	delete sess_vars_exp_node;
	if ( sess_vars->value.type != PHP_VAL_ARRAY ) {
		return;
	}

	for(int i = 0; i < array_get_size(&sess_vars->value); i++) {
		std::string s = array_get_ith_key(&sess_vars->value, i);
		PHP_VAR_NODE *var = array_get_by_str_key(&sess_vars->value, s);
		cast_value_str(&var->value);
		varmap[s] = var->value.str_val;
	}
}
// File_checked_for_headers
