//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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



#include <ec/cpp/ECTag.h>		// Needed for CECTag
#include <ec/cpp/ECSpecialTags.h>	// Needed for special EC tag creator classes

#include "Preferences.h"
#include "amule.h"

CEC_Category_Tag::CEC_Category_Tag(uint32 cat_index, EC_DETAIL_LEVEL detail_level) : CECTag(EC_TAG_CATEGORY, cat_index)
{
	Category_Struct *cat = theApp->glob_prefs->GetCategory(cat_index);
	switch (detail_level) {
		case EC_DETAIL_UPDATE:
		case EC_DETAIL_INC_UPDATE:
		case EC_DETAIL_WEB:
		case EC_DETAIL_FULL:
			AddTag(CECTag(EC_TAG_CATEGORY_PATH, cat->incomingpath));
			AddTag(CECTag(EC_TAG_CATEGORY_COMMENT, cat->comment));
			AddTag(CECTag(EC_TAG_CATEGORY_COLOR, (uint32)cat->color));
			AddTag(CECTag(EC_TAG_CATEGORY_PRIO, cat->prio));
		case EC_DETAIL_CMD:
			AddTag(CECTag(EC_TAG_CATEGORY_TITLE, cat->title));
		}
}

CEC_Category_Tag::CEC_Category_Tag(uint32 cat_index, wxString name, wxString path,
			wxString comment, uint32 color, uint8 prio) : CECTag(EC_TAG_CATEGORY, cat_index)
{
	AddTag(CECTag(EC_TAG_CATEGORY_PATH, path));
	AddTag(CECTag(EC_TAG_CATEGORY_COMMENT, comment));
	AddTag(CECTag(EC_TAG_CATEGORY_COLOR, color));
	AddTag(CECTag(EC_TAG_CATEGORY_PRIO, prio));
	AddTag(CECTag(EC_TAG_CATEGORY_TITLE, name));
}

void CEC_Category_Tag::Apply()
{
	theApp->glob_prefs->UpdateCategory(GetInt(), Name(), Path(), Comment(), Color(), Prio());
}

void CEC_Category_Tag::Create()
{
	theApp->glob_prefs->CreateCategory(Name(), Path(), Comment(), Color(), Prio());
}

CEC_Prefs_Packet::CEC_Prefs_Packet(uint32 selection, EC_DETAIL_LEVEL pref_details, EC_DETAIL_LEVEL cat_details) : CECPacket(EC_OP_SET_PREFERENCES, pref_details)
{
	if (selection & EC_PREFS_CATEGORIES) {
		if (theApp->glob_prefs->GetCatCount() > 1) {
			CECEmptyTag cats(EC_TAG_PREFS_CATEGORIES);
			for (unsigned int i = 0; i < theApp->glob_prefs->GetCatCount(); ++i) {
				CEC_Category_Tag catTag(i, cat_details);
				cats.AddTag(catTag);
			}
			AddTag(cats);
		}
	}

	if (selection & EC_PREFS_GENERAL) {
		CECEmptyTag user_prefs(EC_TAG_PREFS_GENERAL);
		user_prefs.AddTag(CECTag(EC_TAG_USER_NICK, thePrefs::GetUserNick()));
		user_prefs.AddTag(CECTag(EC_TAG_USER_USERHASH, thePrefs::GetUserHash()));
		AddTag(user_prefs);
	}

	if (selection & EC_PREFS_CONNECTIONS) {
		CECEmptyTag connPrefs(EC_TAG_PREFS_CONNECTIONS);
		connPrefs.AddTag(CECTag(EC_TAG_CONN_UL_CAP, thePrefs::GetMaxGraphUploadRate()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_DL_CAP, thePrefs::GetMaxGraphDownloadRate()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_UL, thePrefs::GetMaxUpload()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_DL, thePrefs::GetMaxDownload()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_SLOT_ALLOCATION, thePrefs::GetSlotAllocation()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_TCP_PORT, thePrefs::GetPort()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_UDP_PORT, thePrefs::GetUDPPort()));
		if (thePrefs::IsUDPDisabled()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_UDP_DISABLE));
		}
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_FILE_SOURCES, thePrefs::GetMaxSourcePerFile()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_CONN, thePrefs::GetMaxConnections()));
		if (thePrefs::DoAutoConnect()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_AUTOCONNECT));
		}
		if (thePrefs::Reconnect()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_RECONNECT));
		}
		if (thePrefs::GetNetworkED2K()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_NETWORK_ED2K));
		}
		if (thePrefs::GetNetworkKademlia()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_NETWORK_KADEMLIA));
		}
		AddTag(connPrefs);
	}

	if (selection & EC_PREFS_MESSAGEFILTER) {
		CECEmptyTag msg_prefs(EC_TAG_PREFS_MESSAGEFILTER);
		if (thePrefs::MustFilterMessages()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_ENABLED));
		}
		if (thePrefs::IsFilterAllMessages()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_ALL));
		}
		if (thePrefs::MsgOnlyFriends()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_FRIENDS));
		}
		if (thePrefs::MsgOnlySecure()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_SECURE));
		}
		if (thePrefs::IsFilterByKeywords()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_BY_KEYWORD));
		}
		msg_prefs.AddTag(CECTag(EC_TAG_MSGFILTER_KEYWORDS, thePrefs::GetMessageFilterString()));
		AddTag(msg_prefs);
	}

	if (selection & EC_PREFS_REMOTECONTROLS) {
		CECEmptyTag rc_prefs(EC_TAG_PREFS_REMOTECTRL);
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_PORT, thePrefs::GetWSPort()));
		if (thePrefs::GetWSIsEnabled()) {
			rc_prefs.AddTag(CECEmptyTag(EC_TAG_WEBSERVER_AUTORUN));
		}
		if (!thePrefs::GetWSPass().IsEmpty()) {
			CMD4Hash passhash;
			wxCHECK2(passhash.Decode(thePrefs::GetWSPass()), /* Do nothing. */);
			rc_prefs.AddTag(CECTag(EC_TAG_PASSWD_HASH, passhash));
		}
		if (thePrefs::GetWSIsLowUserEnabled()) {
			CECEmptyTag lowUser(EC_TAG_WEBSERVER_GUEST);
			if (!thePrefs::GetWSLowPass().IsEmpty()) {
				CMD4Hash passhash;
				wxCHECK2(passhash.Decode(thePrefs::GetWSLowPass()), /* Do nothing. */);
				lowUser.AddTag(CECTag(EC_TAG_PASSWD_HASH, passhash));
			}
			rc_prefs.AddTag(lowUser);
		}
		if (thePrefs::GetWebUseGzip()) {
			rc_prefs.AddTag(CECEmptyTag(EC_TAG_WEBSERVER_USEGZIP));
		}
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_REFRESH, thePrefs::GetWebPageRefresh()));
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_TEMPLATE, thePrefs::GetWebTemplate()));
		AddTag(rc_prefs);
	}

	if (selection & EC_PREFS_ONLINESIG) {
		CECEmptyTag online_sig(EC_TAG_PREFS_ONLINESIG);
		if (thePrefs::IsOnlineSignatureEnabled()) {
			online_sig.AddTag(CECEmptyTag(EC_TAG_ONLINESIG_ENABLED));
		}
		AddTag(online_sig);
	}

	if (selection & EC_PREFS_SERVERS) {
		CECEmptyTag srv_prefs(EC_TAG_PREFS_SERVERS);
		if (thePrefs::DeadServer()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_REMOVE_DEAD));
		}
		srv_prefs.AddTag(CECTag(EC_TAG_SERVERS_DEAD_SERVER_RETRIES, (uint16)thePrefs::GetDeadserverRetries()));
		if (thePrefs::AutoServerlist()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_AUTO_UPDATE));
		}
		// Here should come the URL list...
		if (thePrefs::AddServersFromServer()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_ADD_FROM_SERVER));
		}
		if (thePrefs::AddServersFromClient()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_ADD_FROM_CLIENT));
		}
		if (thePrefs::Score()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_USE_SCORE_SYSTEM));
		}
		if (thePrefs::GetSmartIdCheck()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_SMART_ID_CHECK));
		}
		if (thePrefs::IsSafeServerConnectEnabled()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_SAFE_SERVER_CONNECT));
		}
		if (thePrefs::AutoConnectStaticOnly()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY));
		}
		if (thePrefs::IsManualHighPrio()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_MANUAL_HIGH_PRIO));
		}
		AddTag(srv_prefs);
	}

	if (selection & EC_PREFS_FILES) {
		CECEmptyTag filePrefs(EC_TAG_PREFS_FILES);
		if (thePrefs::IsICHEnabled()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ICH_ENABLED));
		}
		if (thePrefs::IsTrustingEveryHash()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_AICH_TRUST));
		}
		if (thePrefs::AddNewFilesPaused()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_PAUSED));
		}
		if (thePrefs::GetNewAutoDown()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_AUTO_DL_PRIO));
		}
		if (thePrefs::GetPreviewPrio()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_PREVIEW_PRIO));
		}
		if (thePrefs::GetNewAutoUp()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_AUTO_UL_PRIO));
		}
		if (thePrefs::TransferFullChunks()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_UL_FULL_CHUNKS));
		}
		if (thePrefs::StartNextFile()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_START_NEXT_PAUSED));
		}
		if (thePrefs::StartNextFileSame()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_RESUME_SAME_CAT));
		}
		if (thePrefs::GetSrcSeedsOn()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_SAVE_SOURCES));
		}
		if (thePrefs::GetExtractMetaData()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_EXTRACT_METADATA));
		}
		if (thePrefs::GetAllocFullChunk()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ALLOC_FULL_CHUNKS));
		}
		if (thePrefs::GetAllocFullPart()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ALLOC_FULL_SIZE));
		}
		if (thePrefs::IsCheckDiskspaceEnabled()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_CHECK_FREE_SPACE));
		}
		filePrefs.AddTag(CECTag(EC_TAG_FILES_MIN_FREE_SPACE, thePrefs::GetMinFreeDiskSpace()));
		AddTag(filePrefs);
	}

	if (selection & EC_PREFS_SRCDROP) {
		CECEmptyTag srcdrop(EC_TAG_PREFS_SRCDROP);
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_NONEEDED, (uint8)thePrefs::GetNoNeededSources()));
		if (thePrefs::DropFullQueueSources()) {
			srcdrop.AddTag(CECEmptyTag(EC_TAG_SRCDROP_DROP_FQS));
		}
		if (thePrefs::DropHighQueueRankingSources()) {
			srcdrop.AddTag(CECEmptyTag(EC_TAG_SRCDROP_DROP_HQRS));
		}
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_HQRS_VALUE, (uint16)thePrefs::HighQueueRanking()));
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_AUTODROP_TIMER, (uint16)thePrefs::GetAutoDropTimer()));
		AddTag(srcdrop);
	}

	if (selection & EC_PREFS_DIRECTORIES) {
		#warning TODO
	}

	if (selection & EC_PREFS_STATISTICS) {
		#warning TODO
	}

	if (selection & EC_PREFS_SECURITY) {
		CECEmptyTag secPrefs(EC_TAG_PREFS_SECURITY);
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_CAN_SEE_SHARES, thePrefs::CanSeeShares()));
		if (thePrefs::IsFilteringClients()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_CLIENTS));
		}
		if (thePrefs::IsFilteringServers()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_SERVERS));
		}
		if (thePrefs::IPFilterAutoLoad()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_AUTO_UPDATE));
		}
		secPrefs.AddTag(CECTag(EC_TAG_IPFILTER_UPDATE_URL, thePrefs::IPFilterURL()));
		secPrefs.AddTag(CECTag(EC_TAG_IPFILTER_LEVEL, thePrefs::GetIPFilterLevel()));
		if (thePrefs::FilterLanIPs()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_FILTER_LAN));
		}
		if (thePrefs::IsSecureIdentEnabled()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_SECURITY_USE_SECIDENT));
		}
		AddTag(secPrefs);
	}

	if (selection & EC_PREFS_CORETWEAKS) {
		CECEmptyTag cwPrefs(EC_TAG_PREFS_CORETWEAKS);
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_MAX_CONN_PER_FIVE, thePrefs::GetMaxConperFive()));
		if (thePrefs::GetVerbose()) {
			cwPrefs.AddTag(CECEmptyTag(EC_TAG_CORETW_VERBOSE));
		}
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_FILEBUFFER, thePrefs::GetFileBufferSize()));
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_UL_QUEUE, thePrefs::GetQueueSize()));
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT, thePrefs::GetServerKeepAliveTimeout()));
		AddTag(cwPrefs);
	}
}

/**
 * Applies a boolean value from the set_preferences request
 *
 * @param use_tag	If true, an unset variable means "leave unchanged". If false, an unset variable means false.
 * @param thisTab	The TAG that contains the TAG with a boolean value
 * @param applyFunc	The function to use for applying the value
 * @param tagName	The name of the TAG that holds the boolean value
 */
void ApplyBoolean(bool use_tag, const CECTag *thisTab, void (applyFunc)(bool), int tagName)
{
	const CECTag *boolTag = thisTab->GetTagByName(tagName);
	if (use_tag) {
		if (boolTag != NULL) {
			applyFunc(boolTag->GetInt() != 0);
		}
	} else {
		applyFunc(boolTag != NULL);
	}
}

/*
 * This will set all preferences except of categories, which are work as following:
 *  -> On remote gui they are loaded on startup, and then changed on-command
 *  -> Webserver doesn't supposed to change it.
 */
void CEC_Prefs_Packet::Apply()
{
	const CECTag *thisTab = NULL;
	const CECTag *oneTag = NULL;

	if ((thisTab = GetTagByName(EC_TAG_PREFS_GENERAL)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_USER_NICK)) != NULL) {
			thePrefs::SetUserNick(oneTag->GetStringData());
		}
	}

	//
	// webserver doesn't transmit all boolean values
	//
	bool use_tag = (GetDetailLevel() == EC_DETAIL_FULL);
	
	if ((thisTab = GetTagByName(EC_TAG_PREFS_CONNECTIONS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UL_CAP)) != NULL) {
			thePrefs::SetMaxGraphUploadRate(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_DL_CAP)) != NULL) {
			thePrefs::SetMaxGraphDownloadRate(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_UL)) != NULL) {
			thePrefs::SetMaxUpload(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_DL)) != NULL) {
			thePrefs::SetMaxDownload(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_SLOT_ALLOCATION)) != NULL) {
			thePrefs::SetSlotAllocation(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_TCP_PORT)) != NULL) {
			thePrefs::SetPort(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UDP_PORT)) != NULL) {
			thePrefs::SetUDPPort(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetUDPDisable, EC_TAG_CONN_UDP_DISABLE);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_FILE_SOURCES)) != NULL) {
			thePrefs::SetMaxSourcesPerFile(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_CONN)) != NULL) {
			thePrefs::SetMaxConnections(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAutoConnect, EC_TAG_CONN_AUTOCONNECT);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetReconnect, EC_TAG_CONN_RECONNECT);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetNetworkED2K, EC_TAG_NETWORK_ED2K);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetNetworkKademlia, EC_TAG_NETWORK_KADEMLIA);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_MESSAGEFILTER)) != NULL) {
		ApplyBoolean(use_tag, thisTab, thePrefs::SetMustFilterMessages, EC_TAG_MSGFILTER_ENABLED);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetFilterAllMessages, EC_TAG_MSGFILTER_ALL);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetMsgOnlyFriends, EC_TAG_MSGFILTER_FRIENDS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetMsgOnlySecure, EC_TAG_MSGFILTER_SECURE);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetFilterByKeywords, EC_TAG_MSGFILTER_BY_KEYWORD);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_KEYWORDS)) != NULL) {
			thePrefs::SetMessageFilterString(oneTag->GetStringData());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_REMOTECTRL)) != NULL) {
		ApplyBoolean(use_tag, thisTab, thePrefs::SetWSIsEnabled, EC_TAG_WEBSERVER_AUTORUN);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_PORT)) != NULL) {
			thePrefs::SetWSPort(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
			thePrefs::SetWSPass(oneTag->GetMD4Data().Encode());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetWSIsLowUserEnabled, EC_TAG_WEBSERVER_GUEST);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_GUEST)) != NULL) {
			if ((oneTag->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
				thePrefs::SetWSLowPass(oneTag->GetTagByName(EC_TAG_PASSWD_HASH)->GetMD4Data().Encode());
			}
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetWebUseGzip, EC_TAG_WEBSERVER_USEGZIP);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_REFRESH)) != NULL) {
			thePrefs::SetWebPageRefresh(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_TEMPLATE)) != NULL) {
			thePrefs::SetWebTemplate(oneTag->GetStringData());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_ONLINESIG)) != NULL) {
		ApplyBoolean(use_tag, thisTab, thePrefs::SetOnlineSignatureEnabled, EC_TAG_ONLINESIG_ENABLED);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SERVERS)) != NULL) {
		ApplyBoolean(use_tag, thisTab, thePrefs::SetDeadServer, EC_TAG_SERVERS_REMOVE_DEAD);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_DEAD_SERVER_RETRIES)) != NULL) {
			thePrefs::SetDeadserverRetries(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAutoServerlist, EC_TAG_SERVERS_AUTO_UPDATE);
		// Here should come the URL list...
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAddServersFromServer, EC_TAG_SERVERS_ADD_FROM_SERVER);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAddServersFromClient, EC_TAG_SERVERS_ADD_FROM_CLIENT);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetScoreSystem, EC_TAG_SERVERS_USE_SCORE_SYSTEM);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetSmartIdCheck, EC_TAG_SERVERS_SMART_ID_CHECK);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetSafeServerConnectEnabled, EC_TAG_SERVERS_SAFE_SERVER_CONNECT);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAutoConnectStaticOnly, EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetManualHighPrio, EC_TAG_SERVERS_MANUAL_HIGH_PRIO);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_FILES)) != NULL) {
		ApplyBoolean(use_tag, thisTab, thePrefs::SetICHEnabled, EC_TAG_FILES_ICH_ENABLED);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetTrustingEveryHash, EC_TAG_FILES_AICH_TRUST);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAddNewFilesPaused, EC_TAG_FILES_NEW_PAUSED);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetNewAutoDown, EC_TAG_FILES_NEW_AUTO_DL_PRIO);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetPreviewPrio, EC_TAG_FILES_PREVIEW_PRIO);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetNewAutoUp, EC_TAG_FILES_NEW_AUTO_UL_PRIO);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetTransferFullChunks, EC_TAG_FILES_UL_FULL_CHUNKS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetStartNextFile, EC_TAG_FILES_START_NEXT_PAUSED);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetStartNextFileSame, EC_TAG_FILES_RESUME_SAME_CAT);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetSrcSeedsOn, EC_TAG_FILES_SAVE_SOURCES);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetExtractMetaData, EC_TAG_FILES_EXTRACT_METADATA);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAllocFullChunk, EC_TAG_FILES_ALLOC_FULL_CHUNKS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetAllocFullPart, EC_TAG_FILES_ALLOC_FULL_SIZE);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetCheckDiskspaceEnabled, EC_TAG_FILES_CHECK_FREE_SPACE);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_MIN_FREE_SPACE)) != NULL) {
			thePrefs::SetMinFreeDiskSpace(oneTag->GetInt());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SRCDROP)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_NONEEDED)) != NULL) {
			thePrefs::SetNoNeededSources(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetDropFullQueueSources, EC_TAG_SRCDROP_DROP_FQS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetDropHighQueueRankingSources, EC_TAG_SRCDROP_DROP_HQRS);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_HQRS_VALUE)) != NULL) {
			thePrefs::SetHighQueueRanking(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_AUTODROP_TIMER)) != NULL) {
			thePrefs::SetAutoDropTimer(oneTag->GetInt());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_DIRECTORIES)) != NULL) {
		#warning TODO
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_STATISTICS)) != NULL) {
		#warning TODO
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SECURITY)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_CAN_SEE_SHARES)) != NULL) {
			thePrefs::SetCanSeeShares(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetFilteringClients, EC_TAG_IPFILTER_CLIENTS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetFilteringServers, EC_TAG_IPFILTER_SERVERS);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetIPFilterAutoLoad, EC_TAG_IPFILTER_AUTO_UPDATE);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_UPDATE_URL)) != NULL) {
			thePrefs::SetIPFilterURL(oneTag->GetStringData());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_LEVEL)) != NULL) {
			thePrefs::SetIPFilterLevel(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetFilterLanIPs, EC_TAG_IPFILTER_FILTER_LAN);
		ApplyBoolean(use_tag, thisTab, thePrefs::SetSecureIdentEnabled, EC_TAG_SECURITY_USE_SECIDENT);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_CORETWEAKS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_MAX_CONN_PER_FIVE)) != NULL) {
			thePrefs::SetMaxConsPerFive(oneTag->GetInt());
		}
		ApplyBoolean(use_tag, thisTab, thePrefs::SetVerbose, EC_TAG_CORETW_VERBOSE);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_FILEBUFFER)) != NULL) {
			thePrefs::SetFileBufferSize(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_UL_QUEUE)) != NULL) {
			thePrefs::SetQueueSize(oneTag->GetInt());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT)) != NULL) {
			thePrefs::SetServerKeepAliveTimeout(oneTag->GetInt());
		}
	}
	
	theApp->glob_prefs->Save();
}
// File_checked_for_headers
