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

//
// This file is NOT part of aMule build. It's used solely for testing PHP engine
// in separate build
//
#include <string> // Do_not_auto_remove (g++-4.0.1)
#include <map>

#include <sys/types.h>
#include <regex.h>

#define PACKAGE_VERSION "standalone"

#include <map>
#include <list>
#include <stdarg.h>

#include "php_syntree.h"
#include "php_core_lib.h"


void php_native_shared_file_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1");
		return;
	}
	char *str_hash = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 2");
		return;
	}
	char *cmd_name = si->var->value.str_val;
	si = get_scope_item(g_current_scope, "__param_2");
	PHP_VAR_NODE *opt_param = si ? si->var : 0;

	if ( !strcmp(cmd_name, "prio") && !opt_param ) {
		php_report_error(PHP_ERROR, "Command 'prio' need 3-rd argument");
		return;
	}

	printf("php_native_shared_file_cmd: hash=%s cmd=%s\n", str_hash, cmd_name);
}

void php_native_reload_shared_file_cmd(PHP_VALUE_NODE *)
{
	printf("php_native_reload_shared_file_cmd\n");
}

/*
 * 
 * Usage: php_native_download_file_cmd($file_hash, "command", $optional_arg)
 * 
 */
void php_native_download_file_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1");
		return;
	}
	char *str_hash = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 2");
		return;
	}
	char *cmd_name = si->var->value.str_val;
	si = get_scope_item(g_current_scope, "__param_2");
	PHP_VAR_NODE *opt_param = si ? si->var : 0;

	if ( (!strcmp(cmd_name, "prio") || !strcmp(cmd_name, "setcat")) && !opt_param ) {
		php_report_error(PHP_ERROR, "Commands 'prio' and 'setcat' needs 3-rd argument");
		return;
	}
		
	printf("php_native_download_file_cmd: hash=%s cmd=%s\n", str_hash, cmd_name);
}

/*
 * Usage amule_kad_connect($bootstrap_ip, $bootstrap_port)
 */
void php_native_kad_connect(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si ) {
		php_report_error(PHP_ERROR, "Missing or bad argument 1: $bootstrap_ip_addr");
		return;
	}
	cast_value_dnum(&si->var->value);
	unsigned int ipaddr = si->var->value.int_val;

	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si ) {
		php_report_error(PHP_ERROR, "Missing or bad argument 2: $bootstrap_ip_port");
		return;
	}
	cast_value_dnum(&si->var->value);
	unsigned int ipport = si->var->value.int_val;

	printf("php_native_kad_connect: ip=%08x port=%d\n", ipaddr, ipport);
}

void php_native_kad_disconnect(PHP_VALUE_NODE *)
{
	printf("php_native_kad_disconnect\n");
}

/*
 * Usage amule_add_server_cmd($server_addr, $server_port, $server_name);
 */
void php_native_add_server_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING) ) {
		php_report_error(PHP_ERROR, "Missing or bad argument 1: $server_addr");
		return;
	}
	char *addr = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si ) {
		php_report_error(PHP_ERROR, "Missing argument 2: $server_port");
		return;
	}
	cast_value_dnum(&si->var->value);
	int port = si->var->value.int_val;

	si = get_scope_item(g_current_scope, "__param_2");
	if ( !si || (si->var->value.type != PHP_VAL_STRING) ) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 3: $server_name");
		return;
	}
	char *name = si->var->value.str_val;

	printf("php_native_add_server_cmd: addr=%s port=%04d name=%s\n", addr, port, name);
}

/*
 * Usage amule_server_cmd($server_ip, $server_port, "command");
 */
void php_native_server_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si ) {
		php_report_error(PHP_ERROR, "Missing argument 1: $server_ip");
		return;
	}
	cast_value_dnum(&si->var->value);
	uint32_t ip = si->var->value.int_val;

	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si ) {
		php_report_error(PHP_ERROR, "Missing argument 2: $server_port");
		return;
	}
	cast_value_dnum(&si->var->value);
	int port = si->var->value.int_val;

	si = get_scope_item(g_current_scope, "__param_2");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 3: $command");
		return;
	}
	char *cmd = si->var->value.str_val;

	printf("php_native_server_cmd: ip=%08x port=%04d cmd=%s\n", ip, port, cmd);
}

/*
 * Query amule status. Return hash containing stats values
 */
void php_get_amule_stats(PHP_VALUE_NODE *result)
{
	cast_value_array(result);
	PHP_VAR_NODE *id = array_get_by_str_key(result, "id");
	cast_value_dnum(&id->value);
	id->value.int_val = 1234567;
}

void php_get_amule_categories(PHP_VALUE_NODE *result)
{
	cast_value_array(result);
	for (int i = 0; i < 5; i++) {
		PHP_VAR_NODE *cat = array_get_by_int_key(result, i);
		value_value_free(&cat->value);
		cat->value.type = PHP_VAL_STRING;
		cat->value.str_val = strdup("some_cat");
	}
}

/*
 * Return hash of amule options.
 *  Key: option name
 *  Value: option value (string)
 */
void php_get_amule_options(PHP_VALUE_NODE *result)
{
	cast_value_array(result);
}


/*
 * Set amule options from given array. Argument looks like "amule_get_options" result
 */
void php_set_amule_options(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_ARRAY)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1 (options array)");
		return;
	}
}

/*
 * Download 1 of search results. Params: hash, category (default=0)
 */
void php_native_search_download_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1 (file hash)");
		return;
	}
	char *str_hash = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 2 (category)");
		return;
	}

	cast_value_dnum(&si->var->value);
	int cat = si->var->value.int_val;

	printf("php_native_search_download_cmd: hash=%s category=%d\n", str_hash, cat);
}

void php_native_search_start_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1 (search term)");
		return;
	}
	char *search = si->var->value.str_val;

	if ( !(si = get_scope_item(g_current_scope, "__param_1")) || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 2 (file extension)");
		return;
	}
	char *ext = si->var->value.str_val;

	if ( !(si = get_scope_item(g_current_scope, "__param_2")) || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 3 (file type)");
		return;
	}
	char *type = si->var->value.str_val;

	if ( !(si = get_scope_item(g_current_scope, "__param_3")) ) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 4 (search type)");
		return;
	}
	cast_value_dnum(&si->var->value);

	if ( !(si = get_scope_item(g_current_scope, "__param_4")) ) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 5 (availability)");
		return;
	}
	cast_value_dnum(&si->var->value);
	int avail = si->var->value.int_val;

	if ( !(si = get_scope_item(g_current_scope, "__param_5")) ) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 6 (min size)");
		return;
	}
	cast_value_dnum(&si->var->value);
	int min_size = si->var->value.int_val;

	if ( !(si = get_scope_item(g_current_scope, "__param_6")) ) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 7 (max size)");
		return;
	}
	cast_value_dnum(&si->var->value);
	int max_size = si->var->value.int_val;

	printf("php_native_search_start_cmd: search=%s \n", search);
}

/*
 * Request contents of log
 */
void php_get_log(PHP_VALUE_NODE *result)
{
	value_value_free(result);
	
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	bool rst;
	if ( !si ) {
		rst = false;
	} else {
		cast_value_dnum(&si->var->value);
		rst = si->var->value.int_val != 0;
	}

	printf("php_get_log: reset=%d\n", rst);
}

/*
 * Request contents of server info
 */
void php_get_serverinfo(PHP_VALUE_NODE *result)
{
	value_value_free(result);
	
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	bool rst;
	if ( !si ) {
		rst = false;
	} else {
		cast_value_dnum(&si->var->value);
		rst = si->var->value.int_val != 0;
	}
	printf("php_get_serverinfo: reset=%d\n", rst);
}


/*
 * Download ed2k link. Params: link, category (default=0)
 */
void php_native_ed2k_download_cmd(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 1 (file link)");
		return;
	}
	char *str_link = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument 2 (category)");
		return;
	}

	cast_value_dnum(&si->var->value);
	int cat = si->var->value.int_val;

	printf("php_native_search_download_cmd: hash=%s category=%d\n", str_link, cat);
}


void amule_fake_obj_array_create(int count, char *class_name, PHP_VALUE_NODE *result)
{
	for (int i = 0; i < count; i++) {
		PHP_VAR_NODE *var = array_push_back(result);
		var->value.type = PHP_VAL_OBJECT;
		var->value.obj_val.class_name = class_name;
		var->value.obj_val.inst_ptr = 0;
	}
}

void amule_load_downloads(PHP_VALUE_NODE *result)
{
	amule_fake_obj_array_create(10, "AmuleDownloadFile", result);
}

void amule_load_servers(PHP_VALUE_NODE *result)
{
	amule_fake_obj_array_create(20, "AmuleServer", result);
}

void amule_load_shared(PHP_VALUE_NODE *result)
{
	amule_fake_obj_array_create(15, "AmuleSharedFile", result);
}

void amule_load_search(PHP_VALUE_NODE *result)
{
	amule_fake_obj_array_create(35, "AmuleSearchFile", result);
}

void amule_load_uploads(PHP_VALUE_NODE *result)
{
	amule_fake_obj_array_create(17, "AmuleUploadFile", result);
}

void amule_load_stats()
{
}

void amule_load_stats_tree(PHP_VALUE_NODE *)
{
}


void php_native_load_amule_vars(PHP_VALUE_NODE *result)
{
	PHP_SCOPE_ITEM *si_str = get_scope_item(g_current_scope, "__param_0");
	if ( !si_str  ) {
		php_report_error(PHP_ERROR, "Missing argument 'varname' for 'load_amule_vars'");
		return;
	}
	PHP_VALUE_NODE *str = &si_str->var->value;
	if ( str->type != PHP_VAL_STRING ) {
		php_report_error(PHP_ERROR, "Argument 'varname' for 'load_amule_vars' must be string");
		return;
	}
	char *varname = str->str_val;
	if ( result ) {
		cast_value_array(result);
	}
	if ( strcmp(varname, "downloads") == 0 ) {
		amule_load_downloads(result);
	} else if ( strcmp(varname, "uploads") == 0 ) {
		amule_load_uploads(result);
	} else if ( strcmp(varname, "shared") == 0 ) {
		amule_load_shared(result);
	} else if ( strcmp(varname, "searchresult") == 0 ) {
		amule_load_search(result);
	} else if ( strcmp(varname, "servers") == 0 ) {
		amule_load_servers(result);
	} else if ( strcmp(varname, "stats_graph") == 0 ) {
		amule_load_stats();
	} else if ( strcmp(varname, "stats_tree") == 0 ) {
		amule_load_stats_tree(result);
	} else {
		value_value_free(result);
		php_report_error(PHP_ERROR, "This type of amule variable is unknown");
	}
}

/*
 * Amule objects implementations
 */
void amule_fake_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !strcmp(prop_name, "name") || !strcmp(prop_name, "hash") ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup("some_str");
	} else {
		result->type = PHP_VAL_INT;
		result->int_val = 10;
	}
}

void amule_download_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_upload_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_server_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_shared_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_search_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_version(PHP_VALUE_NODE *val)
{
	if ( !val ) {
		return;
	}
	value_value_free(val);

	val->type = PHP_VAL_STRING;
	val->str_val = strdup(PACKAGE_VERSION);
}


PHP_BLTIN_FUNC_DEF amule_lib_funcs[] = {
	{
		"amule_load_vars",
		1, php_native_load_amule_vars,
	},
	{
		"amule_get_stats",
		0, php_get_amule_stats,
	},
	{
		"amule_get_categories",
		0, php_get_amule_categories,
	},
	{
		"amule_get_options",
		0, php_get_amule_options,
	},
	{
		"amule_set_options",
		1, php_set_amule_options,
	},
	{
		"amule_do_server_cmd",
		3,
		php_native_server_cmd,
	},
	{
		"amule_do_add_server_cmd",
		3,
		php_native_add_server_cmd,
	},
	{
		"amule_do_download_cmd",
		3,
		php_native_download_file_cmd,
	},
	{
		"amule_do_shared_cmd",
		3,
		php_native_shared_file_cmd,
	},
	{
		"amule_do_reload_shared_cmd",
		0, php_native_reload_shared_file_cmd,
	},
	{
		"amule_do_search_download_cmd",
		2,
		php_native_search_download_cmd,
	},
	{
		"amule_do_search_start_cmd",
		7,
		php_native_search_start_cmd,
	},
	{
		"amule_do_ed2k_download_cmd",
		2,
		php_native_ed2k_download_cmd,
	},
	{
		"amule_get_log",
		1, php_get_log,
	},
	{
		"amule_get_serverinfo",
		1, php_get_serverinfo,
	},
	{
		"amule_get_version",
		0, amule_version,
	},
	{ 0, 0, 0, },
};

void php_init_amule_lib()
{
	// load function definitions
	PHP_BLTIN_FUNC_DEF *curr_def = amule_lib_funcs;
	while ( curr_def->name ) {
		php_add_native_func(curr_def);
		curr_def++;
	}
	// load object definitions
	php_add_native_class("AmuleDownloadFile", amule_download_file_prop_get);
	php_add_native_class("AmuleUploadFile", amule_upload_file_prop_get);
	php_add_native_class("AmuleServer", amule_server_prop_get);
	php_add_native_class("AmuleSharedFile", amule_shared_file_prop_get);
	php_add_native_class("AmuleSearchFile", amule_search_file_prop_get);
}

int main(int argc, char *argv[])
{
	const char *filename = ( argc == 2 ) ? argv[1] : "test.php";

	CWriteStrBuffer buffer;
	
	//phpdebug = 0;

	CPhpFilter php_filter((CWebServerBase*)0, (CSession *)0,filename, &buffer);
	
	int size = buffer.Length();
	char *buf = new char [size+1];
	buffer.CopyAll(buf);
	printf("%s", buf);
	delete [] buf;
	
	return 0;
}

// File_checked_for_headers
