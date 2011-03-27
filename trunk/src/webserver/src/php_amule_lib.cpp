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


#include <string> // Do_not_auto_remove (g++-4.0.1)

#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "WebServer.h"
#include <ec/cpp/ECSpecialTags.h>

#include "php_syntree.h"
#include "php_core_lib.h"

#include <wx/datetime.h>

/*
 * Built-in php functions. Those are amule-specific funcions, accessing EC and internal
 * datastructre
 * 
 */

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


	CPhPLibContext::g_curr_context->WebServer()->Send_SharedFile_Cmd(wxString(char2unicode(str_hash)),
		wxString(char2unicode(cmd_name)),
		opt_param ? opt_param->value.int_val : 0);
}

void php_native_reload_shared_file_cmd(PHP_VALUE_NODE *)
{
	CPhPLibContext::g_curr_context->WebServer()->Send_ReloadSharedFile_Cmd();
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
		
	CPhPLibContext::g_curr_context->WebServer()->Send_DownloadFile_Cmd(wxString(char2unicode(str_hash)),
		wxString(char2unicode(cmd_name)),
		opt_param ? opt_param->value.int_val : 0);
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

	CECPacket req(EC_OP_KAD_BOOTSTRAP_FROM_IP);
	req.AddTag(CECTag(EC_TAG_BOOTSTRAP_IP, ipaddr));
	req.AddTag(CECTag(EC_TAG_BOOTSTRAP_PORT, ipport));
	CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(&req);
}

void php_native_kad_disconnect(PHP_VALUE_NODE *)
{
	CECPacket req(EC_OP_KAD_STOP);
	CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(&req);
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

	CPhPLibContext::g_curr_context->WebServer()->Send_AddServer_Cmd(wxString(char2unicode(addr)),
		wxString::Format(wxT("%d"), port), wxString(char2unicode(name)));
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

	CPhPLibContext::g_curr_context->WebServer()->Send_Server_Cmd(ip, port, wxString(char2unicode(cmd)));
}

/*
 * Query amule status. Return hash containing stats values
 */
void php_get_amule_stats(PHP_VALUE_NODE *result)
{
	CECPacket stat_req(EC_OP_STAT_REQ, EC_DETAIL_FULL);
	const CECPacket *stats = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&stat_req);
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
	id->value.int_val = tag->GetEd2kId();
	const CECTag *server = tag->GetTagByName(EC_TAG_SERVER);
	if ( server ) {
		PHP_VAR_NODE *srv_ip = array_get_by_str_key(result, "serv_addr");
		value_value_free(&srv_ip->value);
		srv_ip->value.type = PHP_VAL_STRING;
		srv_ip->value.str_val =strdup(unicode2UTF8(server->GetIPv4Data().StringIP()));

		const CECTag *sname = server->GetTagByName(EC_TAG_SERVER_NAME);
		if ( sname ) {
			PHP_VAR_NODE *srv_name = array_get_by_str_key(result, "serv_name");
			value_value_free(&srv_name->value);
			srv_name->value.type = PHP_VAL_STRING;
			srv_name->value.str_val = strdup(unicode2UTF8(sname->GetStringData()));
		}
		
		const CECTag *susers = server->GetTagByName(EC_TAG_SERVER_USERS);
		if ( susers ) {
			PHP_VAR_NODE *srv_users = array_get_by_str_key(result, "serv_users");
			value_value_free(&srv_users->value);
			srv_users->value.type = PHP_VAL_INT;
			srv_users->value.int_val = susers->GetInt();
		}
			
	}
	// kademlia
	PHP_VAR_NODE *kad = array_get_by_str_key(result, "kad_connected");
	value_value_free(&kad->value);
	kad->value.type = PHP_VAL_BOOL;
	if ( tag->IsConnectedKademlia() ) {
		kad->value.int_val = 1;
		PHP_VAR_NODE *kad_fwl = array_get_by_str_key(result, "kad_firewalled");
		kad_fwl->value.type = PHP_VAL_BOOL;
		kad_fwl->value.int_val = tag->IsKadFirewalled();
	} else {
		kad->value.int_val = 0;
	}
	// traffic stats
	PHP_VAR_NODE *speed;
	speed = array_get_by_str_key(result, "speed_up");
	value_value_free(&speed->value);
	speed->value.type = PHP_VAL_INT;
	speed->value.int_val = stats->GetTagByName(EC_TAG_STATS_UL_SPEED)->GetInt();
	
	speed = array_get_by_str_key(result, "speed_down");
	value_value_free(&speed->value);
	speed->value.type = PHP_VAL_INT;
	speed->value.int_val = stats->GetTagByName(EC_TAG_STATS_DL_SPEED)->GetInt();

	speed = array_get_by_str_key(result, "speed_limit_up");
	value_value_free(&speed->value);
	speed->value.type = PHP_VAL_INT;
	speed->value.int_val = stats->GetTagByName(EC_TAG_STATS_UL_SPEED_LIMIT)->GetInt();
	
	speed = array_get_by_str_key(result, "speed_limit_down");
	value_value_free(&speed->value);
	speed->value.type = PHP_VAL_INT;
	speed->value.int_val = stats->GetTagByName(EC_TAG_STATS_DL_SPEED_LIMIT)->GetInt();

	delete stats;
}

void php_get_amule_categories(PHP_VALUE_NODE *result)
{
	cast_value_array(result);

	CECPacket req(EC_OP_GET_PREFERENCES);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_CATEGORIES));
	const CECPacket *reply = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&req);
	if ( !reply ) {
		return ;
	}
	const CECTag *cats_tag = reply->GetFirstTagSafe();
	if (cats_tag->HasChildTags()) {
		int i = 0;
		for (CECTag::const_iterator it = cats_tag->begin(); it != cats_tag->end(); it++) {
			const CECTag *categoryTitle = it->GetTagByName(EC_TAG_CATEGORY_TITLE);
			PHP_VAR_NODE *cat = array_get_by_int_key(result, i++);
			value_value_free(&cat->value);
			cat->value.type = PHP_VAL_STRING;
			cat->value.str_val = strdup(unicode2UTF8(categoryTitle->GetStringData()));
		}
	} else {
		PHP_VAR_NODE *cat = array_get_by_int_key(result, 0);
		value_value_free(&cat->value);
		cat->value.type = PHP_VAL_STRING;
		cat->value.str_val = strdup("all");
	}
	delete reply;
}

typedef struct {
	const char *php_name;
	ECTagNames tagname;
	int opsize;
} PHP_2_EC_OPT_DEF;

PHP_2_EC_OPT_DEF g_connection_opt_defs[] = {
	{ "max_line_up_cap", EC_TAG_CONN_UL_CAP, 4}, { "max_line_down_cap", EC_TAG_CONN_DL_CAP, 4},
	{ "max_up_limit", EC_TAG_CONN_MAX_UL, 2}, { "max_down_limit", EC_TAG_CONN_MAX_DL, 2},
	{ "slot_alloc", EC_TAG_CONN_SLOT_ALLOCATION, 2},
	{ "tcp_port", EC_TAG_CONN_TCP_PORT, 2}, { "udp_port", EC_TAG_CONN_UDP_PORT, 2}, 
	{ "udp_dis", EC_TAG_CONN_UDP_DISABLE, 0},
	{ "max_file_src", EC_TAG_CONN_MAX_FILE_SOURCES, 2},
	{ "max_conn_total", EC_TAG_CONN_MAX_CONN, 2}, 
	{ "autoconn_en", EC_TAG_CONN_AUTOCONNECT, 0}, { "reconn_en", EC_TAG_CONN_RECONNECT, 0},
	{0, (ECTagNames)0, 0}
};

PHP_2_EC_OPT_DEF g_file_opt_defs[] = {
	{ "ich_en", EC_TAG_FILES_ICH_ENABLED, 0},
	{ "aich_trust", EC_TAG_FILES_AICH_TRUST, 0},
	{ "new_files_paused", EC_TAG_FILES_NEW_PAUSED, 0},
	{ "new_files_auto_dl_prio", EC_TAG_FILES_NEW_AUTO_DL_PRIO, 0},
	{ "preview_prio", EC_TAG_FILES_PREVIEW_PRIO, 0},
	{ "new_files_auto_ul_prio", EC_TAG_FILES_NEW_AUTO_UL_PRIO, 0},
	{ "upload_full_chunks", EC_TAG_FILES_UL_FULL_CHUNKS, 0},
	{ "first_last_chunks_prio", EC_TAG_FILES_PREVIEW_PRIO, 0},
	{ "start_next_paused", EC_TAG_FILES_START_NEXT_PAUSED, 0},
	{ "resume_same_cat", EC_TAG_FILES_RESUME_SAME_CAT, 0},
	{ "save_sources", EC_TAG_FILES_SAVE_SOURCES, 0},
	{ "extract_metadata", EC_TAG_FILES_EXTRACT_METADATA, 0},
	{ "alloc_full", EC_TAG_FILES_ALLOC_FULL_SIZE, 0},
	{ "check_free_space", EC_TAG_FILES_CHECK_FREE_SPACE, 0},
	{ "min_free_space", EC_TAG_FILES_MIN_FREE_SPACE, 4},
	{0, (ECTagNames)0, 0}
};

PHP_2_EC_OPT_DEF g_webserver_opt_defs[] = {
	{ "use_gzip", EC_TAG_WEBSERVER_USEGZIP, 0},
	{ "autorefresh_time", EC_TAG_WEBSERVER_REFRESH, 4},
	{0, (ECTagNames)0, 0}
};

PHP_2_EC_OPT_DEF g_coretweak_opt_defs[] = {
	{ "max_conn_5sec", EC_TAG_CORETW_MAX_CONN_PER_FIVE, 2},
	{0, (ECTagNames)0, 0}
};

void set_array_int_val(PHP_VALUE_NODE *array, std::string arrkey, int value)
{
	PHP_VAR_NODE *key = array_get_by_str_key(array, arrkey);
	PHP_VALUE_NODE intval;
	intval.type = PHP_VAL_INT;
	intval.int_val = value;
	value_value_assign(&key->value, &intval);
}

void ec_tag_2_php(const CECTag *cattag, PHP_2_EC_OPT_DEF *opts, PHP_VAR_NODE *catvar)
{
	for(PHP_2_EC_OPT_DEF *def = opts; def->php_name; def++) {
		int val;
		switch(def->opsize) {
			case 0: val = cattag->GetTagByName(def->tagname) ? 1 : 0; break;
			case 1: val = cattag->GetTagByNameSafe(def->tagname)->GetInt(); break;
			case 2: val = cattag->GetTagByNameSafe(def->tagname)->GetInt(); break;
			case 4: val = cattag->GetTagByNameSafe(def->tagname)->GetInt(); break;
			default: val = -1;
		}
		wxASSERT(val != -1);
		//printf("OPT_DEBUG: %s of size %d -> %d\n", def->php_name, def->opsize, val);
		set_array_int_val(&catvar->value, def->php_name, val);
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

	CECPacket req(EC_OP_GET_PREFERENCES);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)0xffffffff));
	const CECPacket *reply = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&req);
	if ( !reply || !reply->HasChildTags()) {
		return ;
	}
	const CECTag *cattag = 0;
	PHP_VALUE_NODE intval;
	intval.type = PHP_VAL_INT;
    if ((cattag = reply->GetTagByName(EC_TAG_PREFS_GENERAL)) != 0) {
		PHP_VAR_NODE *key = array_get_by_str_key(result, "nick");
		value_value_free(&key->value);
		key->value.type = PHP_VAL_STRING;
		key->value.str_val = strdup(unicode2UTF8(cattag->GetTagByNameSafe(EC_TAG_USER_NICK)->GetStringData()));
	}

	if ((cattag = reply->GetTagByName(EC_TAG_PREFS_CONNECTIONS)) != 0) {
		PHP_VAR_NODE *cat = array_get_by_str_key(result, "connection");
		cast_value_array(&cat->value);
		
		ec_tag_2_php(cattag, g_connection_opt_defs, cat);
	}
	if ((cattag = reply->GetTagByName(EC_TAG_PREFS_FILES)) != 0) {
		PHP_VAR_NODE *cat = array_get_by_str_key(result, "files");
		cast_value_array(&cat->value);

		ec_tag_2_php(cattag, g_file_opt_defs, cat);
	}

	if ((cattag = reply->GetTagByName(EC_TAG_PREFS_REMOTECTRL)) != 0) {
		PHP_VAR_NODE *cat = array_get_by_str_key(result, "webserver");
		cast_value_array(&cat->value);

		ec_tag_2_php(cattag, g_webserver_opt_defs, cat);
	}

	if ((cattag = reply->GetTagByName(EC_TAG_PREFS_CORETWEAKS)) != 0) {
		PHP_VAR_NODE *cat = array_get_by_str_key(result, "coretweaks");
		cast_value_array(&cat->value);

		ec_tag_2_php(cattag, g_coretweak_opt_defs, cat);
	}
}

bool php_2_ec_tag(CECTag *cattag, PHP_2_EC_OPT_DEF *opts, PHP_VALUE_NODE *catvar)
{
	for(PHP_2_EC_OPT_DEF *def = opts; def->php_name; def++) {
		PHP_VAR_NODE *opt_var = array_get_by_str_key(catvar, def->php_name);
		if ( opt_var->value.type == PHP_VAL_NONE ) {
			continue;
		}
		cast_value_dnum(&opt_var->value);
		switch(def->opsize) {
			case 0: // always transmit !
			case 1: cattag->AddTag(CECTag(def->tagname ,(uint8)opt_var->value.int_val)); break;
			case 2: cattag->AddTag(CECTag(def->tagname ,(uint16)opt_var->value.int_val)); break;
			case 4: cattag->AddTag(CECTag(def->tagname ,(uint32)opt_var->value.int_val)); break;
			default: return false;
		}
	}
	return true;
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

	CECPacket req(EC_OP_SET_PREFERENCES);
	PHP_VAR_NODE *opt_group_array = 0;

	// files
	opt_group_array = array_get_by_str_key(&si->var->value, "files");
	if ( opt_group_array->value.type == PHP_VAL_ARRAY ) {
		CECEmptyTag filePrefs(EC_TAG_PREFS_FILES);
		php_2_ec_tag(&filePrefs, g_file_opt_defs, &opt_group_array->value);
		req.AddTag(filePrefs);
	}
	// connection
	opt_group_array = array_get_by_str_key(&si->var->value, "connection");
	if ( opt_group_array->value.type == PHP_VAL_ARRAY ) {
		CECEmptyTag connPrefs(EC_TAG_PREFS_CONNECTIONS);
		php_2_ec_tag(&connPrefs, g_connection_opt_defs, &opt_group_array->value);
		req.AddTag(connPrefs);
	}
	// webserver
	opt_group_array = array_get_by_str_key(&si->var->value, "webserver");
	if ( opt_group_array->value.type == PHP_VAL_ARRAY ) {
		CECEmptyTag webPrefs(EC_TAG_PREFS_REMOTECTRL);
		php_2_ec_tag(&webPrefs, g_webserver_opt_defs, &opt_group_array->value);
		req.AddTag(webPrefs);
		// also apply settings localy
		PHP_VAR_NODE *pref = array_get_by_str_key(&opt_group_array->value, "use_gzip");
		cast_value_dnum(&pref->value);
		CPhPLibContext::g_curr_context->WebServer()->webInterface->m_UseGzip = pref->value.int_val != 0;
		pref = array_get_by_str_key(&opt_group_array->value, "autorefresh_time");
		cast_value_dnum(&pref->value);
		CPhPLibContext::g_curr_context->WebServer()->webInterface->m_PageRefresh = pref->value.int_val;
	}
	CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(&req);
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

	CPhPLibContext::g_curr_context->WebServer()->Send_DownloadSearchFile_Cmd(
		wxString(char2unicode(str_hash)), cat);
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

	EC_SEARCH_TYPE search_type;
	switch(si->var->value.int_val) {
		case 0: search_type = EC_SEARCH_LOCAL; break;
		case 1: search_type = EC_SEARCH_GLOBAL; break;
		case 2: search_type = EC_SEARCH_KAD; break;
		default: 
			php_report_error(PHP_ERROR, "Invalid search type %"PRIu64, si->var->value.int_val);
			return;
	}

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


	CPhPLibContext::g_curr_context->WebServer()->Send_Search_Cmd(
		wxString(char2unicode(search)), wxString(char2unicode(ext)), wxString(char2unicode(type)),
		search_type, avail, min_size, max_size);
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
	if ( rst ) {
		CECPacket req(EC_OP_RESET_LOG);
		CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(&req);
	}
	CECPacket req(EC_OP_GET_LOG);
	const CECPacket *response = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&req);
	if (response) {
		wxString serverInfoString(_SpecialChars(response->GetFirstTagSafe()->GetStringData()));
		delete response;
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(serverInfoString));
	}
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

	if ( rst ) {
		CECPacket req(EC_OP_CLEAR_SERVERINFO);
		CPhPLibContext::g_curr_context->WebServer()->Send_Discard_V2_Request(&req);
	}
	CECPacket req(EC_OP_GET_SERVERINFO);
	const CECPacket *response = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&req);
	if (response) {
		wxString serverInfoString(_SpecialChars(response->GetFirstTagSafe()->GetStringData()));
		delete response;
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(serverInfoString));
	}
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

	bool cmd_result = CPhPLibContext::g_curr_context->WebServer()->Send_DownloadEd2k_Cmd(
		wxString(char2unicode(str_link)), cat);
	if ( result ) {
		cast_value_bool(result);
		result->int_val = cmd_result;
	}
}


/*
 * Load amule variables into interpreter scope.
 *  "varname" will tell us, what kind of variables need to load:
 *    "downloads", "uploads", "searchresult", "servers", "options" etc
 */
template <class C, class T>
void amule_obj_array_create(const char *class_name, PHP_VALUE_NODE *result)
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

void amule_load_search(PHP_VALUE_NODE *result)
{
	amule_obj_array_create<SearchInfo, SearchFile>("AmuleSearchFile", result);
}

void amule_load_uploads(PHP_VALUE_NODE *result)
{
	amule_obj_array_create<UploadsInfo, UploadFile>("AmuleUploadFile", result);
}

void amule_load_stats()
{
	CPhPLibContext::g_curr_context->WebServer()->Reload_Stats();
}

/*
 * Convert CEC_StatTree_Node_Tag into php associative array
 * 
 * Since data structure is recoursive - we need helper function
 * to perform conversion
 */
void ecstats2php(CEC_StatTree_Node_Tag *root, PHP_VALUE_NODE *result)
{
	cast_value_array(result);
	std::string key(unicode2UTF8(root->GetDisplayString()));
	PHP_VAR_NODE *v_key = array_get_by_str_key(result, key);
	for (CECTag::const_iterator it = root->begin(); it != root->end(); it++) {
		CEC_StatTree_Node_Tag *tag = (CEC_StatTree_Node_Tag*) & *it;
		if (tag->GetTagName() == EC_TAG_STATTREE_NODE) {
			ecstats2php(tag, &v_key->value);
		}
	}

}

void amule_load_stats_tree(PHP_VALUE_NODE *result)
{
	if ( !result ) {
		return;
	}
	value_value_free(result);
	
	CECPacket req(EC_OP_GET_STATSTREE, EC_DETAIL_WEB);
	const CECPacket *response = CPhPLibContext::g_curr_context->WebServer()->webInterface->SendRecvMsg_v2(&req);
	if ( !response ) {
		return;
	}
	const CECTag *server_ver = response->GetTagByName(EC_TAG_SERVER_VERSION);
	const CECTag *user_nick = response->GetTagByName(EC_TAG_USER_NICK);
	if ( !server_ver || !user_nick ) {
		delete response;
		return;
	}
	CEC_StatTree_Node_Tag *stats_root = (CEC_StatTree_Node_Tag *)response->GetTagByName(EC_TAG_STATTREE_NODE);
	//ecstats2php(stats_root, result);
	for (CECTag::const_iterator it = stats_root->begin(); it != stats_root->end(); it++) {
		CEC_StatTree_Node_Tag *tag = (CEC_StatTree_Node_Tag*) & *it;
		if (tag->GetTagName() == EC_TAG_STATTREE_NODE) {
			ecstats2php(tag, result);
		}
	}
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
		result->str_val = strdup((const char *)unicode2UTF8(obj->sFileName));
	} else if ( strcmp(prop_name, "short_name") == 0 ) {
		result->type = PHP_VAL_STRING;
		wxString short_name(obj->sFileName.Length() > 60 ? (obj->sFileName.Left(60) + (wxT(" ..."))) : obj->sFileName);
		result->str_val = strdup((const char *)unicode2UTF8(short_name));
	} else if ( strcmp(prop_name, "hash") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sFileHash));
	} else if ( strcmp(prop_name, "progress") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->m_Image->GetHTML()));
	} else if ( strcmp(prop_name, "link") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sED2kLink));
	} else if ( strcmp(prop_name, "category") == 0 ) {
		result->int_val = obj->nCat;
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
	} else if ( strcmp(prop_name, "last_seen_complete") == 0 ) {
		result->int_val = obj->wxtLastSeenComplete.GetTicks();
	} else {
		php_report_error(PHP_ERROR, "'DownloadFile' property [%s] is unknown", prop_name);
	}
}

void amule_upload_file_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	UploadFile *obj = (UploadFile *)ptr;
	result->type = PHP_VAL_INT;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		SharedFile *sharedfile = SharedFile::GetContainerInstance()->GetByID(obj->nUploadFile);
		// uploading file we don't share ?! We are either out of sync with core or a shared file has been removed while uploading it
		if ( !sharedfile ) {
			SharedFile::GetContainerInstance()->ReQuery();
			sharedfile = SharedFile::GetContainerInstance()->GetByID(obj->nUploadFile);
		}
		result->str_val = strdup(sharedfile ? (const char *)unicode2UTF8(sharedfile->sFileName) : "???");
	} else if ( strcmp(prop_name, "short_name") == 0 ) {
		result->type = PHP_VAL_STRING;
		SharedFile *sharedfile = SharedFile::GetContainerInstance()->GetByID(obj->nUploadFile);
		if ( !sharedfile ) {
			SharedFile::GetContainerInstance()->ReQuery();
			sharedfile = SharedFile::GetContainerInstance()->GetByID(obj->nUploadFile);
		}
		wxString short_name;
		if (sharedfile) {
			short_name = sharedfile->sFileName.Length() > 60 ? (sharedfile->sFileName.Left(60) + (wxT(" ..."))) : sharedfile->sFileName;
		} else {
			short_name = wxT("???");
		}
		result->str_val = strdup((const char *)unicode2UTF8(short_name));
	} else if ( strcmp(prop_name, "user_name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sUserName));
	} else if ( strcmp(prop_name, "xfer_up") == 0 ) {
		result->int_val = obj->nTransferredUp;
	} else if ( strcmp(prop_name, "xfer_down") == 0 ) {
		result->int_val = obj->nTransferredDown;
	} else if ( strcmp(prop_name, "xfer_speed") == 0 ) {
		result->int_val = obj->nSpeed;
	} else {
		php_report_error(PHP_ERROR, "'UploadFile' property [%s] is unknown", prop_name);
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
		result->str_val = strdup((const char *)unicode2UTF8(obj->sServerName));
	} else if ( strcmp(prop_name, "desc") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sServerDescription));
	} else if ( strcmp(prop_name, "addr") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sServerIP));
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
		result->str_val = strdup((const char *)unicode2UTF8(obj->sFileName));
	} else if ( strcmp(prop_name, "short_name") == 0 ) {
		result->type = PHP_VAL_STRING;
		wxString short_name(obj->sFileName.Length() > 60 ? (obj->sFileName.Left(60) + (wxT(" ..."))) : obj->sFileName);
		result->str_val = strdup((const char *)unicode2UTF8(short_name));
	} else if ( strcmp(prop_name, "hash") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sFileHash));
	} else if ( strcmp(prop_name, "size") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->lFileSize;
	} else if ( strcmp(prop_name, "link") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup(unicode2UTF8(obj->sED2kLink));
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
	} else if ( strcmp(prop_name, "prio_auto") == 0 ) {
		result->type = PHP_VAL_BOOL;
		result->int_val = obj->bFileAutoPriority;
	} else {
		php_report_error(PHP_ERROR, "'SharedFile' property [%s] is unknown", prop_name);
	}
}

void amule_search_file_prop_get(void *ptr, char *prop_name, PHP_VALUE_NODE *result)
{
	if ( !ptr ) {
		value_value_free(result);
		return;
	}
	SearchFile *obj = (SearchFile *)ptr;
	if ( strcmp(prop_name, "name") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sFileName));
	} else if ( strcmp(prop_name, "short_name") == 0 ) {
		result->type = PHP_VAL_STRING;
		wxString short_name(obj->sFileName.Length() > 60 ? (obj->sFileName.Left(60) + (wxT(" ..."))) : obj->sFileName);
		result->str_val = strdup((const char *)unicode2UTF8(short_name));
	} else if ( strcmp(prop_name, "hash") == 0 ) {
		result->type = PHP_VAL_STRING;
		result->str_val = strdup((const char *)unicode2UTF8(obj->sHash));
	} else if ( strcmp(prop_name, "size") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->lFileSize;
	} else if ( strcmp(prop_name, "sources") == 0 ) {
		result->type = PHP_VAL_INT;
		result->int_val = obj->lSourceCount;
	} else if ( strcmp(prop_name, "present") == 0 ) {
		result->type = PHP_VAL_BOOL;
		result->int_val = obj->bPresent;
	} else {
		php_report_error(PHP_ERROR, "'SearchFile' property [%s] is unknown", prop_name);
	}
}

#ifndef PACKAGE_VERSION
#include <common/ClientVersion.h>
#define PACKAGE_VERSION (PACKAGE " " VERSION)
#endif

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

// File_checked_for_headers
