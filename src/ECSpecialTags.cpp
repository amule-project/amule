//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ECSpecialTags.h"
#endif

#include <vector>

#include "ECPacket.h"		// Needed for CECTag
#include "ECcodes.h"		// Needed for TAGnames
#include "ECSpecialTags.h"	// Needed for special EC tag creator classes

// Since there are only constructors defined here,
// removing everything from non-local builds.
#ifndef EC_REMOTE

#include "amule.h"
#include "Server.h"		// Needed for CServer
#include "PartFile.h"		// Needed for CPartFile
#include "ServerConnect.h"		// Needed for CServerConnect
#include "updownclient.h"
#include "SharedFileList.h"
#include "SearchList.h"
#include "ClientCredits.h"

#else

#include "wx/intl.h"		// Needed for _()
#include "KnownFile.h"		// Needed for PS_*
#endif

#if !defined(EC_REMOTE) || defined(CLIENT_GUI)

#include "Preferences.h"
#include "amule.h"

CEC_Prefs_Packet::CEC_Prefs_Packet(uint32 selection, EC_DETAIL_LEVEL detail_level) : CECPacket(EC_OP_PREFERENCES)
{
	if (selection & EC_PREFS_CATEGORIES) {
		if (theApp.glob_prefs->GetCatCount() > 1) {
			CECEmptyTag cats(EC_TAG_PREFS_CATEGORIES);
			for (unsigned int i = 0; i < theApp.glob_prefs->GetCatCount(); ++i) {
				Category_Struct *cat = theApp.glob_prefs->GetCategory(i);
				CECTag catTag(EC_TAG_CATEGORY, (uint32)i);
				switch (detail_level) {
					case EC_DETAIL_UPDATE:
					case EC_DETAIL_GUI:
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_PATH, cat->incomingpath));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_COMMENT, cat->comment));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_COLOR, (uint32)cat->color));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_PRIO, cat->prio));
					case EC_DETAIL_WEB:
					case EC_DETAIL_CMD:
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_TITLE, cat->title));
				}
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
			rc_prefs.AddTag(CECTag(EC_TAG_PASSWD_HASH, CMD4Hash(thePrefs::GetWSPass())));
		}
		if (thePrefs::GetWSIsLowUserEnabled()) {
			CECEmptyTag lowUser(EC_TAG_WEBSERVER_GUEST);
			if (!thePrefs::GetWSLowPass().IsEmpty()) {
				lowUser.AddTag(CECTag(EC_TAG_PASSWD_HASH, CMD4Hash(thePrefs::GetWSLowPass())));
			}
			rc_prefs.AddTag(lowUser);
		}
		if (thePrefs::GetWebUseGzip()) {
			rc_prefs.AddTag(CECEmptyTag(EC_TAG_WEBSERVER_USEGZIP));
		}
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_REFRESH, thePrefs::GetWebPageRefresh()));
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
			filePrefs.AddTag(CECTag(EC_TAG_FILES_MIN_FREE_SPACE, thePrefs::GetMinFreeDiskSpace()));
		}
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
		if (thePrefs::CanSeeShares()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_SECURITY_CAN_SEE_SHARES));
		}
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_FILE_PERMISSIONS, (uint32)thePrefs::GetFilePermissions()));
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_DIR_PERMISSIONS, (uint32)thePrefs::GetDirPermissions()));
		if (thePrefs::GetIPFilterOn()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_ENABLED));
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

/*
 * This will set all preferences except of categories, which are work as following:
 *  -> On remote gui they are loaded on startup, and then changed on-command
 *  -> Webserver doesn't supposed to change it.
 */
void CEC_Prefs_Packet::Apply()
{
	const CECTag * thisTab;
	const CECTag * oneTag;

	if ((thisTab = GetTagByName(EC_TAG_PREFS_GENERAL)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_USER_NICK)) != NULL) {
			thePrefs::SetUserNick(oneTag->GetStringData());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_CONNECTIONS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UL_CAP)) != NULL) {
			thePrefs::SetMaxGraphUploadRate(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_DL_CAP)) != NULL) {
			thePrefs::SetMaxGraphDownloadRate(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_UL)) != NULL) {
			thePrefs::SetMaxUpload(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_DL)) != NULL) {
			thePrefs::SetMaxDownload(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_SLOT_ALLOCATION)) != NULL) {
			thePrefs::SetSlotAllocation(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_TCP_PORT)) != NULL) {
			thePrefs::SetPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UDP_PORT)) != NULL) {
			thePrefs::SetUDPPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UDP_DISABLE)) != NULL) {
			thePrefs::SetUDPDisable(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_FILE_SOURCES)) != NULL) {
			thePrefs::SetMaxSourcesPerFile(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_CONN)) != NULL) {
			thePrefs::SetMaxConnections(oneTag->GetInt16Data());
		}
		thePrefs::SetAutoConnect(thisTab->GetTagByName(EC_TAG_CONN_AUTOCONNECT) != NULL);
		thePrefs::SetReconnect(thisTab->GetTagByName(EC_TAG_CONN_RECONNECT) != NULL);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_MESSAGEFILTER)) != NULL) {
		thePrefs::SetMustFilterMessages(thisTab->GetTagByName(EC_TAG_MSGFILTER_ENABLED) != NULL);
		thePrefs::SetFilterAllMessages(thisTab->GetTagByName(EC_TAG_MSGFILTER_ALL) != NULL);
		thePrefs::SetMsgOnlyFriends(thisTab->GetTagByName(EC_TAG_MSGFILTER_FRIENDS) != NULL);
		thePrefs::SetMsgOnlySecure(thisTab->GetTagByName(EC_TAG_MSGFILTER_SECURE) != NULL);
		thePrefs::SetFilterByKeywords(thisTab->GetTagByName(EC_TAG_MSGFILTER_BY_KEYWORD) != NULL);

		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_KEYWORDS)) != NULL) {
			thePrefs::SetMessageFilterString(oneTag->GetStringData());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_REMOTECTRL)) != NULL) {
		thePrefs::SetWSIsEnabled(thisTab->GetTagByName(EC_TAG_WEBSERVER_AUTORUN) != NULL);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_PORT)) != NULL) {
			thePrefs::SetWSPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
			thePrefs::SetWSPass(oneTag->GetMD4Data().Encode());
		}
		thePrefs::SetWSIsLowUserEnabled(thisTab->GetTagByName(EC_TAG_WEBSERVER_GUEST) != 0);
		if ((oneTag->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
			thePrefs::SetWSLowPass(oneTag->GetTagByName(EC_TAG_PASSWD_HASH)->GetMD4Data().Encode());
		}
		thePrefs::SetWebUseGzip(thisTab->GetTagByName(EC_TAG_WEBSERVER_USEGZIP) != 0);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_REFRESH)) != NULL) {
			thePrefs::SetWebPageRefresh(oneTag->GetInt32Data());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_ONLINESIG)) != NULL) {
		thePrefs::SetOnlineSignatureEnabled(thisTab->GetTagByName(EC_TAG_ONLINESIG_ENABLED) != 0);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SERVERS)) != NULL) {
		thePrefs::SetDeadServer(thisTab->GetTagByName(EC_TAG_SERVERS_REMOVE_DEAD) != NULL);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_DEAD_SERVER_RETRIES)) != NULL) {
			thePrefs::SetDeadserverRetries(oneTag->GetInt16Data());
		}
		thePrefs::SetAutoServerlist(thisTab->GetTagByName(EC_TAG_SERVERS_AUTO_UPDATE) != NULL);
		// Here should come the URL list...
		thePrefs::SetAddServersFromServer(thisTab->GetTagByName(EC_TAG_SERVERS_ADD_FROM_SERVER) != NULL);
		thePrefs::SetAddServersFromClient(thisTab->GetTagByName(EC_TAG_SERVERS_ADD_FROM_CLIENT) != NULL);
		thePrefs::SetScoreSystem(thisTab->GetTagByName(EC_TAG_SERVERS_USE_SCORE_SYSTEM) != NULL);
		thePrefs::SetSmartIdCheck(thisTab->GetTagByName(EC_TAG_SERVERS_SMART_ID_CHECK) != NULL);
		thePrefs::SetSafeServerConnectEnabled(thisTab->GetTagByName(EC_TAG_SERVERS_SAFE_SERVER_CONNECT) != NULL);
		thePrefs::SetAutoConnectStaticOnly(thisTab->GetTagByName(EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY) != NULL);
		thePrefs::SetManualHighPrio(thisTab->GetTagByName(EC_TAG_SERVERS_MANUAL_HIGH_PRIO) != NULL);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_FILES)) != NULL) {
		thePrefs::SetICHEnabled(thisTab->GetTagByName(EC_TAG_FILES_ICH_ENABLED) != 0);
		thePrefs::SetTrustingEveryHash(thisTab->GetTagByName(EC_TAG_FILES_AICH_TRUST) != 0);
		thePrefs::SetAddNewFilesPaused(thisTab->GetTagByName(EC_TAG_FILES_NEW_PAUSED) != 0);
		thePrefs::SetNewAutoDown(thisTab->GetTagByName(EC_TAG_FILES_NEW_AUTO_DL_PRIO) != 0);
		thePrefs::SetPreviewPrio(thisTab->GetTagByName(EC_TAG_FILES_PREVIEW_PRIO) != 0);
		thePrefs::SetNewAutoUp(thisTab->GetTagByName(EC_TAG_FILES_NEW_AUTO_UL_PRIO) != 0);
		thePrefs::SetTransferFullChunks(thisTab->GetTagByName(EC_TAG_FILES_UL_FULL_CHUNKS) != 0);
		thePrefs::SetStartNextFile(thisTab->GetTagByName(EC_TAG_FILES_START_NEXT_PAUSED) != 0);
		thePrefs::SetSrcSeedsOn(oneTag->GetInt8Data() != 0);
		thePrefs::SetExtractMetaData(thisTab->GetTagByName(EC_TAG_FILES_EXTRACT_METADATA) != 0);
		thePrefs::SetAllocFullChunk(thisTab->GetTagByName(EC_TAG_FILES_ALLOC_FULL_CHUNKS) != 0);
		thePrefs::SetAllocFullPart(thisTab->GetTagByName(EC_TAG_FILES_ALLOC_FULL_SIZE) != 0);
		thePrefs::SetCheckDiskspaceEnabled(thisTab->GetTagByName(EC_TAG_FILES_CHECK_FREE_SPACE) != 0);
		
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_MIN_FREE_SPACE)) != NULL) {
			thePrefs::SetMinFreeDiskSpace(oneTag->GetInt32Data());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SRCDROP)) != NULL) {
		thePrefs::SetNoNeededSources(thisTab->GetTagByName(EC_TAG_SRCDROP_NONEEDED) != 0);
		thePrefs::SetDropFullQueueSources(thisTab->GetTagByName(EC_TAG_SRCDROP_DROP_FQS) != 0);
		thePrefs::SetDropHighQueueRankingSources(thisTab->GetTagByName(EC_TAG_SRCDROP_DROP_HQRS) != 0);
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_HQRS_VALUE)) != NULL) {
			thePrefs::SetHighQueueRanking(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_AUTODROP_TIMER)) != NULL) {
			thePrefs::SetAutoDropTimer(oneTag->GetInt16Data());
		}
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_DIRECTORIES)) != NULL) {
		#warning TODO
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_STATISTICS)) != NULL) {
		#warning TODO
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_SECURITY)) != NULL) {
		thePrefs::SetCanSeeShares(thisTab->GetTagByName(EC_TAG_SECURITY_CAN_SEE_SHARES) != 0);

		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_FILE_PERMISSIONS)) != NULL) {
			thePrefs::SetFilePermissions(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_DIR_PERMISSIONS)) != NULL) {
			thePrefs::SetDirPermissions(oneTag->GetInt32Data());
		}
		thePrefs::SetIPFilterOn(thisTab->GetTagByName(EC_TAG_IPFILTER_ENABLED) != 0);
		thePrefs::SetIPFilterAutoLoad(thisTab->GetTagByName(EC_TAG_IPFILTER_AUTO_UPDATE) != 0);

		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_UPDATE_URL)) != NULL) {
			thePrefs::SetIPFilterURL(oneTag->GetStringData());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_LEVEL)) != NULL) {
			thePrefs::SetIPFilterLevel(oneTag->GetInt8Data());
		}
		thePrefs::SetFilterLanIPs(thisTab->GetTagByName(EC_TAG_IPFILTER_FILTER_LAN) != 0);
		thePrefs::SetSecureIdentEnabled(thisTab->GetTagByName(EC_TAG_SECURITY_USE_SECIDENT) != 0);
	}

	if ((thisTab = GetTagByName(EC_TAG_PREFS_CORETWEAKS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_MAX_CONN_PER_FIVE)) != NULL) {
			thePrefs::SetMaxConsPerFive(oneTag->GetInt16Data());
		}

		thePrefs::SetVerbose(thisTab->GetTagByName(EC_TAG_CORETW_VERBOSE) != 0);

		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_FILEBUFFER)) != NULL) {
			thePrefs::SetFileBufferSize(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_UL_QUEUE)) != NULL) {
			thePrefs::SetQueueSize(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT)) != NULL) {
			thePrefs::SetServerKeepAliveTimeout(oneTag->GetInt32Data());
		}
	}
}

#endif

#ifndef EC_REMOTE

CEC_Server_Tag::CEC_Server_Tag(CServer *server, EC_DETAIL_LEVEL detail_level) :
	CECTag(EC_TAG_SERVER, EC_IPv4_t(server->GetIP(), server->GetPort()))
{
	wxString tmpStr;
	uint32 tmpInt;
	uint8 tmpShort;

	switch (detail_level) {
		case EC_DETAIL_UPDATE:
			if ((tmpInt = server->GetPing()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_PING, tmpInt));
			}
			if ((tmpShort = (uint8)server->GetFailedCount()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_FAILED, tmpShort));
			}
			break;
		case EC_DETAIL_GUI:
			if ((tmpInt = server->GetPing()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_PING, tmpInt));
			}
			if ((tmpShort = (uint8)server->GetPreferences()) != SRV_PR_NORMAL) {
				AddTag(CECTag(EC_TAG_SERVER_PRIO, tmpShort));
			}
			if ((tmpShort = (uint8)server->GetFailedCount()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_FAILED, tmpShort));
			}
			if ((tmpShort = (uint8)server->IsStaticMember()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_STATIC, tmpShort));
			}
			if (!(tmpStr = server->GetVersion()).IsEmpty()) {
				AddTag(CECTag(EC_TAG_SERVER_VERSION, tmpStr));
			}
		case EC_DETAIL_WEB:
			if (!(tmpStr = server->GetDescription()).IsEmpty()) {
				AddTag(CECTag(EC_TAG_SERVER_DESC, tmpStr));
			}
			if ((tmpInt = server->GetUsers()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_USERS, tmpInt));
			}
			if ((tmpInt = server->GetMaxUsers()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_USERS_MAX, tmpInt));
			}
			if ((tmpInt = server->GetFiles()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_FILES, tmpInt));
			}
		case EC_DETAIL_CMD:
			if (!(tmpStr = server->GetListName()).IsEmpty()) {
				AddTag(CECTag(EC_TAG_SERVER_NAME, tmpStr));
			}
	}
}

CEC_ConnState_Tag::CEC_ConnState_Tag(EC_DETAIL_LEVEL detail_level) : CECTag(EC_TAG_CONNSTATE,
	(uint32) (theApp.serverconnect->IsConnected() ? theApp.serverconnect->GetClientID() : 
		theApp.serverconnect->IsConnecting() ? 0xffffffff : 0))
{
	if ( theApp.serverconnect->GetCurrentServer() ) {
		AddTag(CEC_Server_Tag(theApp.serverconnect->GetCurrentServer(), detail_level));
	}
}

CEC_PartFile_Tag::CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level)
	: CECTag(EC_TAG_PARTFILE, file->GetFileHash())
{
	AddTag(CECTag(EC_TAG_PARTFILE_STATUS, file->GetStatus()));

	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT, (uint32)file->GetSourceCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT, (uint32)file->GetNotCurrentSourcesCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, (uint32)file->GetTransferingSrcCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_A4AF, (uint32)file->GetSrcA4AFCount()));
		
	if ( (file->GetTransferingSrcCount() > 0) || (detail_level != EC_DETAIL_UPDATE) ) {
		
		AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, (uint32)file->GetTransfered()));
		AddTag(CECTag(EC_TAG_PARTFILE_SIZE_DONE, (uint32)file->GetCompletedSize()));
		AddTag(CECTag(EC_TAG_PARTFILE_SPEED, (uint32)(file->GetKBpsDown()*1024)));
	}
	
	AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
		(uint32)(file->IsAutoDownPriority() ? 
						file->GetDownPriority() + 10 : file->GetDownPriority())));

	AddTag(CECTag(EC_TAG_PARTFILE_CAT, file->GetCategory()));
	AddTag(CECTag(EC_TAG_PARTFILE_LAST_SEEN_COMP, (uint32)file->lastseencomplete));

	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	
	AddTag(CECTag(EC_TAG_PARTFILE_NAME,file->GetFileName()));

	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, (uint32)file->GetFileSize()));

	AddTag(CECTag(EC_TAG_PARTFILE_ED2K_LINK,
				(theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID()) ?
					theApp.CreateED2kSourceLink(file) : theApp.CreateED2kLink(file)));
}


#else /* EC_REMOTE */
// Since this is only needed at the remote end

wxString CEC_PartFile_Tag::GetFileStatusString()
{
	uint8 nFileStatus = FileStatus();
	
        if ((nFileStatus == PS_HASHING) || (nFileStatus == PS_WAITINGFORHASH)) {
                return _("Hashing");
        } else {
                switch (nFileStatus) {
                        case PS_COMPLETING:
                                return _("Completing");
                        case PS_COMPLETE:
                                return _("Complete");
                        case PS_PAUSED:
                                return _("Paused");
                        case PS_ERROR:
                                return _("Erroneous");
                        default:
                                if (SourceXferCount() > 0) {
                                        return _("Downloading");
                                } else {
                                        return _("Waiting");
                                }
                }
                // if stopped
        }
}

#endif /* ! EC_REMOTE */
#ifndef EC_REMOTE

CEC_SharedFile_Tag::CEC_SharedFile_Tag(const CKnownFile *file, EC_DETAIL_LEVEL detail_level) : CECTag(EC_TAG_KNOWNFILE, file->GetFileHash())
{
	AddTag(CECTag(EC_TAG_KNOWNFILE_REQ_COUNT, (uint32)file->statistic.GetRequests()));
	AddTag(CECTag(EC_TAG_KNOWNFILE_REQ_COUNT_ALL, (uint32)file->statistic.GetAllTimeRequests()));
	
	AddTag(CECTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT, (uint32)file->statistic.GetAccepts()));
	AddTag(CECTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL, (uint32)file->statistic.GetAllTimeAccepts()));

	AddTag(CECTag(EC_TAG_KNOWNFILE_XFERRED, (uint32)file->statistic.GetTransfered()));
	AddTag(CECTag(EC_TAG_KNOWNFILE_XFERRED_ALL, (uint32)file->statistic.GetAllTimeTransfered()));
	
	AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
		(uint32)(file->IsAutoUpPriority() ? file->GetUpPriority() + 10 : file->GetUpPriority())));

	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	
	AddTag(CECTag(EC_TAG_PARTFILE_NAME,file->GetFileName()));

	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, (uint32)file->GetFileSize()));


	AddTag(CECTag(EC_TAG_PARTFILE_ED2K_LINK,
				(theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID()) ?
					theApp.CreateED2kSourceLink(file) : theApp.CreateED2kLink(file)));
}

CEC_SharedFile_Tag::CEC_SharedFile_Tag(const CKnownFile *file, CValueMap &valuemap) : CECTag(EC_TAG_KNOWNFILE, file->GetFileHash())
{
	valuemap.CreateTag(EC_TAG_KNOWNFILE_REQ_COUNT, (uint32)file->statistic.GetRequests(), this);
	valuemap.CreateTag(EC_TAG_KNOWNFILE_REQ_COUNT_ALL, (uint32)file->statistic.GetAllTimeRequests(), this);
	valuemap.CreateTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT, (uint32)file->statistic.GetAccepts(), this);
	valuemap.CreateTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL, (uint32)file->statistic.GetAllTimeAccepts(), this);
	valuemap.CreateTag(EC_TAG_KNOWNFILE_XFERRED, (uint32)file->statistic.GetTransfered(), this);
	valuemap.CreateTag(EC_TAG_KNOWNFILE_XFERRED_ALL, (uint32)file->statistic.GetAllTimeTransfered(), this);
	
	valuemap.CreateTag(EC_TAG_PARTFILE_PRIO,
		(uint32)(file->IsAutoUpPriority() ? file->GetUpPriority() + 10 : file->GetUpPriority()), this);
	
	valuemap.CreateTag(EC_TAG_PARTFILE_NAME, file->GetFileName(), this);
	valuemap.CreateTag(EC_TAG_PARTFILE_SIZE_FULL, (uint32)file->GetFileSize(), this);
	valuemap.CreateTag(EC_TAG_PARTFILE_ED2K_LINK,
		(theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID()) ?
					theApp.CreateED2kSourceLink(file) : theApp.CreateED2kLink(file), this);
}

CEC_UpDownClient_Tag::CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level) :
	CECTag(EC_TAG_UPDOWN_CLIENT, client->GetUserID())
{
	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, (uint32)client->GetTransferedDown()));
	
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_TOTAL, client->Credits()->GetUploadedTotal()));
	AddTag(CECTag(EC_TAG_CLIENT_DOWNLOAD_TOTAL, client->Credits()->GetDownloadedTotal()));
	
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_SESSION, (uint32)client->GetSessionUp()));
	
	AddTag(CECTag(EC_TAG_CLIENT_STATE,
		uint16((uint16)client->GetDownloadState() | (((uint16)client->GetUploadState()) << 8) )));

	AddTag(CECTag(EC_TAG_CLIENT_UP_SPEED, (uint32)(client->GetKBpsUp()*1024.0)));
	if ( client->GetDownloadState() == DS_DOWNLOADING ) {
		AddTag(CECTag(EC_TAG_CLIENT_DOWN_SPEED, (uint32)(client->GetKBpsDown()*1024.0)));
	}

	AddTag(CECTag(EC_TAG_CLIENT_WAIT_TIME, client->GetWaitTime()));
	AddTag(CECTag(EC_TAG_CLIENT_XFER_TIME, client->GetUpStartTimeDelay()));
	AddTag(CECTag(EC_TAG_CLIENT_QUEUE_TIME, ::GetTickCount() - client->GetWaitStartTime()));
	AddTag(CECTag(EC_TAG_CLIENT_LAST_TIME, ::GetTickCount() - client->GetLastUpRequest()));
	
	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}

	AddTag(CECTag(EC_TAG_CLIENT_HASH, client->GetUserHash()));
	AddTag(CECTag(EC_TAG_CLIENT_NAME, client->GetUserName()));
	AddTag(CECTag(EC_TAG_CLIENT_SOFTWARE, client->GetClientSoft()));
	
	CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	if (file) {
		AddTag(CECTag(EC_TAG_KNOWNFILE, file->GetFileHash()));
		AddTag(CECTag(EC_TAG_PARTFILE_NAME, file->GetFileName()));
	}
	
}

//
// Search reply
//
CEC_SearchFile_Tag::CEC_SearchFile_Tag(CSearchFile *file, EC_DETAIL_LEVEL detail_level) : CECTag(EC_TAG_SEARCHFILE, file->GetFileHash())
{
	if ( detail_level != EC_DETAIL_WEB ) {
		AddTag(CECTag(EC_TAG_PARTFILE_NAME, file->GetFileName()));
		AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, file->GetFileSize()));
		if ( theApp.sharedfiles->GetFileByID(file->GetFileHash()) ) {
			AddTag(CECTag(EC_TAG_KNOWNFILE, (uint8)0));
		}
	}
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT, file->GetSourceCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, file->GetCompleteSourceCount()));
}

//
// Tree
//
CEC_Tree_Tag::CEC_Tree_Tag(const StatsTreeSiblingIterator& tr) : CECTag(EC_TAG_TREE, *tr)
{
	StatsTreeSiblingIterator temp_it = tr.begin();
	while (temp_it != tr.end()) {
		AddTag(CEC_Tree_Tag(temp_it));
		++temp_it;
	}
}

#endif /* ! EC_REMOTE */

//
// Search request
//
CEC_Search_Tag::CEC_Search_Tag(wxString &name, EC_SEARCH_TYPE search_type, wxString &file_type,
			wxString &extension, uint32 avail, uint32 min_size, uint32 max_size) : CECTag(EC_TAG_SEARCH_TYPE, (uint32)search_type)
{
	AddTag(CECTag(EC_TAG_SEARCH_NAME, name));
	AddTag(CECTag(EC_TAG_SEARCH_FILE_TYPE, file_type));
	if ( !extension.IsEmpty() ) {
		AddTag(CECTag(EC_TAG_SEARCH_EXTENSION, extension));
	}
	if ( avail ) {
		AddTag(CECTag(EC_TAG_SEARCH_AVAILABILITY, avail));
	}
	if ( min_size != 0 ) {
		AddTag(CECTag(EC_TAG_SEARCH_MIN_SIZE, min_size));
	}
	if ( max_size != 0 ) {
		AddTag(CECTag(EC_TAG_SEARCH_MAX_SIZE, max_size));
	}
}
