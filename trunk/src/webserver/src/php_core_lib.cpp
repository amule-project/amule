//
// This file is part of the aMule Project.

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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <list>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#ifdef AMULEWEB_SCRIPT_EN
	#include "WebServer.h"
	#include "ECSpecialTags.h"
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
		php_report_error(PHP_ERROR, "Invalid or missing argument");
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
		case PHP_VAL_OBJECT: printf("Object(%s)\n", node->obj_val.class_name); break;
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

	value_value_assign(&SortElem::callback->params[0].var->value, &o1.obj->value);
	value_value_assign(&SortElem::callback->params[1].var->value, &o2.obj->value);
	
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

	value_value_free(&SortElem::callback->params[0].var->value);
	value_value_free(&SortElem::callback->params[1].var->value);
	
	return result.int_val;
}

void php_native_usort(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_ARRAY)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
		return;
	}
	PHP_VAR_NODE *array = si->var;
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
		return;
	}
	char *cmp_func_name = si->var->value.str_val;
	si = get_scope_item(g_global_scope, cmp_func_name);
	if ( !si || (si->type != PHP_SCOPE_FUNC)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
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
 * 
 * Usage: php_native_download_file_cmd($file_hash, "command", $optional_arg)
 * 
 */
void php_native_download_file_cmd(PHP_VALUE_NODE *)
{
	PHP_SCOPE_ITEM *si = get_scope_item(g_current_scope, "__param_0");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
		return;
	}
	char *str_hash = si->var->value.str_val;
	
	si = get_scope_item(g_current_scope, "__param_1");
	if ( !si || (si->var->value.type != PHP_VAL_STRING)) {
		php_report_error(PHP_ERROR, "Invalid or missing argument");
		return;
	}
	char *cmd_name = si->var->value.str_val;
	si = get_scope_item(g_current_scope, "__param_2");
	PHP_VAR_NODE *opt_param = si ? si->var : 0;

#ifdef AMULEWEB_SCRIPT_EN
	DownloadFileInfo *container = DownloadFile::GetContainerInstance();

	CMD4Hash file_hash(wxString(char2unicode(str_hash)));
	DownloadFile *file = container->GetByID(file_hash);
	
	if ( !file ) {
		php_report_error(PHP_WARNING, "File has not been found in container. Hash=%s", si->var->value.str_val);
		return;
	}
	
	CECPacket *file_cmd = 0;
	CECTag hashtag(EC_TAG_PARTFILE, file_hash);
	
	if ( strcmp(cmd_name, "pause") == 0 ) {
		file_cmd = new CECPacket(EC_OP_PARTFILE_PAUSE);
	} else if ( strcmp(cmd_name, "resume") == 0 ) {
		file_cmd = new CECPacket(EC_OP_PARTFILE_RESUME);
	} else if ( strcmp(cmd_name, "cancel") == 0 ) {
		file_cmd = new CECPacket(EC_OP_PARTFILE_DELETE);
	} else if ( strcmp(cmd_name, "prio") == 0 ) {
		file_cmd = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
	} else if ( strcmp(cmd_name, "setcat") == 0 ) {
	} else {
		php_report_error(PHP_ERROR, "Invalid download command: [%s]", cmd_name);
		return;
	} 
	file_cmd->AddTag(hashtag);
	CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(file_cmd);
	delete file_cmd;

#else
	printf("php_native_download_file_cmd: hash=%s cmd=%s\n", str_hash, cmd_name);
#endif
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
	int ip = si->var->value.int_val;
	
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
#ifdef AMULEWEB_SCRIPT_EN
	CECPacket *req;
	if ( strcmp(cmd, "connect") == 0 ) {
		req = new CECPacket(EC_OP_SERVER_CONNECT);
	} else if ( strcmp(cmd, "remove") == 0 ) {
		req = new CECPacket(EC_OP_SERVER_REMOVE);
	} else {
		php_report_error(PHP_ERROR, "Invalid server command: [%s]", cmd);
		return;
	}
	req->AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));

	CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(req);
	
	delete req;
#else
	printf("php_native_server_cmd: ip=%08x port=%04d cmd=%s\n", ip, port, cmd);
#endif
}

/*
 * Query amule status. Return hash containing stats values
 */
void php_get_amule_stats(PHP_VALUE_NODE *result)
{
#ifdef AMULEWEB_SCRIPT_EN
	CECPacket stat_req(EC_OP_STAT_REQ, EC_DETAIL_CMD);
	CECPacket *stats = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&stat_req);
	if (!stats) {
		return ;
	}
	CEC_ConnState_Tag *tag = (CEC_ConnState_Tag *)stats->GetTagByName(EC_TAG_CONNSTATE);
	if (!tag) {
		return ;
	}
	
	cast_value_array(result);
	PHP_VAR_NODE *id = array_get_by_str_key(result, "id");
	cast_value_dnum(&id->value);
	id->value.int_val = tag->ClientID();
	CECTag *server = tag->GetTagByIndex(0);
	if ( server ) {
		PHP_VAR_NODE *srv_ip = array_get_by_str_key(result, "serv_addr");
		value_value_free(&srv_ip->value);
		srv_ip->value.type = PHP_VAL_STRING;
		srv_ip->value.str_val =strdup(unicode2char(server->GetIPv4Data().StringIP()));

		CECTag *sname = server->GetTagByName(EC_TAG_SERVER_NAME);
		if ( sname ) {
			PHP_VAR_NODE *srv_name = array_get_by_str_key(result, "serv_name");
			value_value_free(&srv_name->value);
			srv_name->value.type = PHP_VAL_STRING;
			srv_name->value.str_val = strdup(unicode2char(sname->GetStringData()));
		}
	}
#else
	cast_value_array(result);
	PHP_VAR_NODE *id = array_get_by_str_key(result, "id");
	cast_value_dnum(&id->value);
	id->value.int_val = 1234567;
#endif
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
	PHP_SCOPE_ITEM *si_start = get_scope_item(g_current_scope, "start");
	PHP_VALUE_NODE *start = &si_start->var->value;
	if ( si_start ) {
		cast_value_dnum(start);
	} else {
		php_report_error(PHP_ERROR, "Invalid or missing argument 'start' for 'substr'");
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

template <class C, class T>
void amule_obj_array_create(char *class_name, PHP_VALUE_NODE *result)
{
	if ( !result ) {
		return;
	}
	C *container = T::GetContainerInstance();

	container->ReQuery();

	typename std::list<T>::const_iterator it = container->GetBeginIterator();
	while ( it != container->GetEndIterator()) {
		PHP_VAR_NODE *var = array_push_back(result);
		var->value.type = PHP_VAL_OBJECT;
		var->value.obj_val.class_name = class_name;
		const T *cur_item = &(*it);
		var->value.obj_val.inst_ptr = (void *)cur_item;
		it++;
	}
}

void amule_load_downloads(PHP_VALUE_NODE *result)
{
	amule_obj_array_create<DownloadFileInfo, DownloadFile>("AmuleDownloadFile", result);
}

void amule_load_servers(PHP_VALUE_NODE *result)
{
	amule_obj_array_create<ServersInfo, ServerEntry>("AmuleServer", result);
}

void amule_load_shared(PHP_VALUE_NODE *result)
{
	amule_obj_array_create<SharedFileInfo, SharedFile>("AmuleSharedFile", result);
}

#else

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

#endif

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
	cast_value_array(result);
	if ( strcmp(varname, "downloads") == 0 ) {
		amule_load_downloads(result);
	} else if ( strcmp(varname, "uploads") == 0 ) {
	} else if ( strcmp(varname, "shared") == 0 ) {
		amule_load_shared(result);
	} else if ( strcmp(varname, "searchresult") == 0 ) {
	} else if ( strcmp(varname, "servers") == 0 ) {
		amule_load_servers(result);
	} else {
		value_value_free(result);
		php_report_error(PHP_ERROR, "This type of amule variable is unknown");
	}
}

/*
 * Amule objects implementations
 */
#ifdef AMULEWEB_SCRIPT_EN
void amule_download_file_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	DownloadFile *obj = (DownloadFile *)ptr;
	result->type = PHP_VAL_INT;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sFileName));
	} else if ( strcmp(prop_name, "hash") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sFileHash));
	} else if ( strcmp(prop_name, "progress") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->m_Image->GetHTML()));
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
		php_report_error(PHP_ERROR, "'DownloadFile' property [%s] is unknown", prop_name);
	}
}

void amule_server_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	ServerEntry *obj = (ServerEntry *)ptr;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sServerName));
	} else if ( strcmp(prop_name, "desc") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sServerDescription));
	} else if ( strcmp(prop_name, "addr") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sServerIP));
	} else if ( strcmp(prop_name, "users") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nServerUsers;
	} else if ( strcmp(prop_name, "ip") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nServerIP;
	} else if ( strcmp(prop_name, "port") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nServerPort;
	} else if ( strcmp(prop_name, "maxusers") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nServerMaxUsers;
	} else if ( strcmp(prop_name, "files") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nServerFiles;
	} else {
		php_report_error(PHP_ERROR, "'ServerEntry' property [%s] is unknown", prop_name);
	}
}

void amule_shared_file_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	SharedFile *obj = (SharedFile *)ptr;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sFileName));
	} else if ( strcmp(prop_name, "hash") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sFileHash));
	} else if ( strcmp(prop_name, "size") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->lFileSize;
	} else if ( strcmp(prop_name, "link") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2char(obj->sED2kLink));
	} else if ( strcmp(prop_name, "xfer") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileTransferred;
	} else if ( strcmp(prop_name, "xfer_all") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileAllTimeTransferred;
	} else if ( strcmp(prop_name, "req") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileRequests;
	} else if ( strcmp(prop_name, "req_all") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileAllTimeRequests;
	} else if ( strcmp(prop_name, "accept") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileAccepts;
	} else if ( strcmp(prop_name, "accept_all") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFileAllTimeAccepts;
	} else if ( strcmp(prop_name, "prio") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->nFilePriority;
	} else if ( strcmp(prop_name, "auto_prio") == 0 ) {
		result->type = PHP_VAL_BOOL;
		result->int_val = obj->bFileAutoPriority;
	} else {
		php_report_error(PHP_ERROR, "'SharedFile' property [%s] is unknown", prop_name);
	}
}


#else

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

void amule_server_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
}

void amule_shared_file_prop_get(void *obj, char *prop_name, PHP_VALUE_NODE *result)
{
	amule_fake_prop_get(obj, prop_name, result);
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
		{ 0, 1, { PHP_VAL_NONE, {0} }, 0 },
		1,
		php_native_var_dump,
	},
	{
		"strlen",
		{ 0, 0, { PHP_VAL_NONE, {0} }, 0 },
		1, php_native_strlen,
	},
	{
		"usort",
		{ { 0, 1, { PHP_VAL_NONE, {0} }, 0 }, { 0, 0, { PHP_VAL_NONE, {0} }, 0 } },
		2,
		php_native_usort,
	},
	{
		"amule_load_vars",
		{ 0, 0, { PHP_VAL_NONE, {0} }, 0 },
		1, php_native_load_amule_vars,
	},
	{
		"amule_get_stats",
		{ 0, 0, { PHP_VAL_NONE, {0} }, 0 },
		0, php_get_amule_stats,
	},
	{
		"amule_do_server_cmd",
		{ { 0, 0, { PHP_VAL_NONE, {0} }, 0 }, { 0, 0, { PHP_VAL_NONE, {0} } , 0}, { 0, 0, { PHP_VAL_NONE, {0} }, 0 }, }, 
		3,
		php_native_server_cmd,
	},
	{
		"amule_do_download_cmd",
		{ { 0, 0, { PHP_VAL_NONE, {0} }, 0 }, { 0, 0, { PHP_VAL_NONE, {0} } , 0}, { 0, 0, { PHP_VAL_NONE, {0} }, 0 }, }, 
		3,
		php_native_download_file_cmd,
	},
	{ 0, { 0, 0, { PHP_VAL_NONE, {0} }, 0 }, 0, 0, },
};

void php_init_core_lib()
{
	// load function definitions
	PHP_BLTIN_FUNC_DEF *curr_def = core_lib_funcs;
	while ( curr_def->name ) {
		php_add_native_func(curr_def);
		curr_def++;
	}
	// load object definitions
	php_add_native_class("AmuleDownloadFile", amule_download_file_prop_get);
	php_add_native_class("AmuleServer", amule_server_prop_get);
	php_add_native_class("AmuleSharedFile", amule_shared_file_prop_get);
}

//
// lexer has no include file
//
extern "C"
void php_set_input_buffer(char *buf, int len);

CPhPLibContext::CPhPLibContext(CWebServerBase *server, const char *file)
{
	g_curr_context = this;
#ifdef AMULEWEB_SCRIPT_EN
	m_server = server;
#endif
	php_engine_init();
	FILE *yyin = fopen(file, "r");
	if ( !yyin ) {
		return;
	}

	yyparse();
	
	m_syn_tree_top = g_syn_tree_top;
	m_global_scope = g_global_scope;
}

CPhPLibContext::CPhPLibContext(CWebServerBase *server, char *php_buf, int len)
{
	g_curr_context = this;
#ifdef AMULEWEB_SCRIPT_EN
	m_server = server;
#endif
	php_engine_init();

	m_global_scope = g_global_scope;

	php_set_input_buffer(php_buf, len);
	yyparse();
	
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

CPhpFilter::CPhpFilter(CWebServerBase *server, CSession *sess,
			const char *file, CWriteStrBuffer *buff)
{
	FILE *f = fopen(file, "r");
	if ( !f ) {
		return;
	}
	if ( fseek(f, 0, SEEK_END) != 0 ) {
		return;
	}
	int size = ftell(f);
	char *buf = new char [size+1];
	rewind(f);
	fread(buf, 1, size, f);
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
		yydebug = 0;

		CPhPLibContext *context = new CPhPLibContext(server, scan_ptr, len);

#ifdef AMULEWEB_SCRIPT_EN
		load_session_vars("HTTP_GET_VARS", sess->m_get_vars);
		load_session_vars("_SESSION", sess->m_vars);
#endif

		context->Execute(buff);

#ifdef AMULEWEB_SCRIPT_EN
		save_session_vars(sess->m_vars);
		sess->m_get_vars.clear();
#endif	

		delete context;
		
		scan_ptr = curr_code_end;
	}

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

void load_session_vars(char *target, std::map<std::string, std::string> &varmap)
{
	PHP_EXP_NODE *sess_vars_exp_node = get_var_node(target);
	PHP_VAR_NODE *sess_vars = sess_vars_exp_node->var_node;
	// i'm not building exp tree, node not needed
	delete sess_vars_exp_node;
	cast_value_array(&sess_vars->value);
	for(std::map<std::string, std::string>::iterator i = varmap.begin(); i != varmap.end(); i++) {
		PHP_VAR_NODE *curr_var = array_get_by_str_key(&sess_vars->value, i->first);
		PHP_VALUE_NODE val;
		val.type = PHP_VAL_STRING;
		val.str_val = (char *)i->second.c_str();
		value_value_assign(&curr_var->value, &val);
	}
}

void save_session_vars(std::map<std::string, std::string> &varmap)
{
	PHP_EXP_NODE *sess_vars_exp_node = get_var_node("_SESSION");
	PHP_VAR_NODE *sess_vars = sess_vars_exp_node->var_node;

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
