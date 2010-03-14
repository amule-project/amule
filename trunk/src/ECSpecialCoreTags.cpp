//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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


// Since there are only constructors defined here,
// removing everything from non-local builds.

#include "amule.h"
#include "Server.h"		// Needed for CServer
#include "PartFile.h"		// Needed for CPartFile
#include "ServerConnect.h"	// Needed for CServerConnect
#include "updownclient.h"
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SharedFileList.h"
#include "SearchList.h"

#include "kademlia/kademlia/Kademlia.h"


CEC_Server_Tag::CEC_Server_Tag(const CServer *server, EC_DETAIL_LEVEL detail_level) :
	CECTag(EC_TAG_SERVER, EC_IPv4_t(server->GetIP(), server->GetPort()))
{
	wxString tmpStr;
	uint32 tmpInt;
	uint8 tmpShort;

	switch (detail_level) {
		case EC_DETAIL_INC_UPDATE:
			// should not get here
			wxFAIL;
			break;
		case EC_DETAIL_UPDATE:
			if ((tmpInt = server->GetPing()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_PING, tmpInt));
			}
			if ((tmpShort = (uint8)server->GetFailedCount()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_FAILED, tmpShort));
			}
			break;
		case EC_DETAIL_WEB:
		case EC_DETAIL_FULL:
			if ((tmpInt = server->GetPing()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_PING, tmpInt));
			}
			if ((tmpShort = (uint8)server->GetPreferences()) != SRV_PR_NORMAL) {
				AddTag(CECTag(EC_TAG_SERVER_PRIO, tmpShort));
			}
			if ((tmpShort = (uint8)server->GetFailedCount()) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_FAILED, tmpShort));
			}
			if ((tmpShort = (server->IsStaticMember() ? 1 : 0)) != 0) {
				AddTag(CECTag(EC_TAG_SERVER_STATIC, tmpShort));
			}
			if (!(tmpStr = server->GetVersion()).IsEmpty()) {
				AddTag(CECTag(EC_TAG_SERVER_VERSION, tmpStr));
			}
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
	(uint8)(
			(theApp->IsConnectedED2K() ? 0x01 : 0x00)
			|
			(theApp->serverconnect->IsConnecting() ? 0x02 : 0x00)
			|
			(theApp->IsConnectedKad() ? 0x04 : 0x00)
			|
			(Kademlia::CKademlia::IsFirewalled() ? 0x08 : 0x00)
			| 
			(Kademlia::CKademlia::IsRunning() ? 0x10 : 0x00)
		))
{
	if (theApp->IsConnectedED2K()) {
		if ( theApp->serverconnect->GetCurrentServer() ) {
			AddTag(CEC_Server_Tag(theApp->serverconnect->GetCurrentServer(), detail_level));
		}
		AddTag(CECTag(EC_TAG_ED2K_ID, theApp->GetED2KID()));
	}

	AddTag(CECTag(EC_TAG_CLIENT_ID, theApp->GetID()));	
}

CEC_PartFile_Tag::CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap)
:
CECTag(EC_TAG_PARTFILE, file->ECID())
{
	AddTag(EC_TAG_PARTFILE_STATUS, file->GetStatus(), valuemap);
	AddTag(EC_TAG_PARTFILE_STOPPED, file->IsStopped(), valuemap);

	AddTag(EC_TAG_PARTFILE_SOURCE_COUNT, file->GetSourceCount(), valuemap);
	AddTag(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT, file->GetNotCurrentSourcesCount(), valuemap);
	AddTag(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, file->GetTransferingSrcCount(), valuemap);
	AddTag(EC_TAG_PARTFILE_SOURCE_COUNT_A4AF, file->GetSrcA4AFCount(), valuemap);
		
	if ( (file->GetTransferingSrcCount() > 0) || (detail_level != EC_DETAIL_UPDATE) || valuemap) {
		
		AddTag(EC_TAG_PARTFILE_SIZE_XFER, file->GetTransferred(), valuemap);
		AddTag(EC_TAG_PARTFILE_SIZE_DONE, file->GetCompletedSize(), valuemap);
		AddTag(EC_TAG_PARTFILE_SPEED, file->GetKBpsDown()*1024, valuemap);
	}
	
	AddTag(EC_TAG_PARTFILE_PRIO, (file->IsAutoDownPriority() ? 
		file->GetDownPriority() + 10 : file->GetDownPriority()), valuemap);

	AddTag(EC_TAG_PARTFILE_CAT, file->GetCategory(), valuemap);
	AddTag(EC_TAG_PARTFILE_LAST_SEEN_COMP, file->lastseencomplete, valuemap);
	AddTag(EC_TAG_PARTFILE_LAST_RECV, file->GetLastChangeDatetime(), valuemap);
	AddTag(EC_TAG_PARTFILE_DOWNLOAD_ACTIVE, file->GetDlActiveTime(), valuemap);
	AddTag(EC_TAG_PARTFILE_AVAILABLE_PARTS, file->GetAvailablePartCount(), valuemap);

	AddTag(EC_TAG_PARTFILE_LOST_CORRUPTION, file->GetLostDueToCorruption(), valuemap);
	AddTag(EC_TAG_PARTFILE_GAINED_COMPRESSION, file->GetGainDueToCompression(), valuemap);
	AddTag(EC_TAG_PARTFILE_SAVED_ICH, file->TotalPacketsSavedDueToICH(), valuemap);

	// Tag for comments
	CECEmptyTag sc(EC_TAG_PARTFILE_COMMENTS);

	FileRatingList list;
	file->GetRatingAndComments(list);
	for (FileRatingList::const_iterator it = list.begin(); it != list.end(); ++it) {
		// Tag children are evaluated by index, not by name
		sc.AddTag(CECTag(EC_TAG_PARTFILE_COMMENTS, it->UserName));
		sc.AddTag(CECTag(EC_TAG_PARTFILE_COMMENTS, it->FileName));
		sc.AddTag(CECTag(EC_TAG_PARTFILE_COMMENTS, (uint64) it->Rating));
		sc.AddTag(CECTag(EC_TAG_PARTFILE_COMMENTS, it->Comment));
	}
	AddTag(sc, valuemap);

	if (detail_level == EC_DETAIL_UPDATE) {
		return;
	}
	
	AddTag(EC_TAG_PARTFILE_NAME, file->GetFileName().GetPrintable(), valuemap);

	AddTag(EC_TAG_PARTFILE_HASH, file->GetFileHash(), valuemap);

	AddTag(EC_TAG_PARTFILE_PARTMETID, file->GetPartMetNumber(), valuemap);

	AddTag(EC_TAG_PARTFILE_SIZE_FULL, file->GetFileSize(), valuemap);

	AddTag(EC_TAG_PARTFILE_ED2K_LINK,
		theApp->CreateED2kLink(file, (theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID())), valuemap);
}

CEC_SharedFile_Tag::CEC_SharedFile_Tag(const CKnownFile *file, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap)
: CECTag(EC_TAG_KNOWNFILE, file->ECID())
{
	AddTag(EC_TAG_KNOWNFILE_REQ_COUNT, file->statistic.GetRequests(), valuemap);
	AddTag(EC_TAG_KNOWNFILE_REQ_COUNT_ALL, file->statistic.GetAllTimeRequests(), valuemap);
	
	AddTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT, file->statistic.GetAccepts(), valuemap);
	AddTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL, file->statistic.GetAllTimeAccepts(), valuemap);

	AddTag(EC_TAG_KNOWNFILE_XFERRED, file->statistic.GetTransferred(), valuemap);
	AddTag(EC_TAG_KNOWNFILE_XFERRED_ALL, file->statistic.GetAllTimeTransferred(), valuemap);
	AddTag(EC_TAG_KNOWNFILE_AICH_MASTERHASH, file->GetAICHMasterHash(), valuemap);
	
	AddTag(EC_TAG_PARTFILE_PRIO,
		(uint8)(file->IsAutoUpPriority() ? file->GetUpPriority() + 10 : file->GetUpPriority()), valuemap);

	AddTag(EC_TAG_KNOWNFILE_COMPLETE_SOURCES_LOW, file->m_nCompleteSourcesCountLo, valuemap);
	AddTag(EC_TAG_KNOWNFILE_COMPLETE_SOURCES_HIGH, file->m_nCompleteSourcesCountHi, valuemap);

	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	
	AddTag(EC_TAG_PARTFILE_NAME,file->GetFileName().GetPrintable(), valuemap);
	AddTag(EC_TAG_PARTFILE_HASH, file->GetFileHash(), valuemap);
	AddTag(EC_TAG_KNOWNFILE_FILENAME, 
		file->IsPartFile()	? wxString(CFormat(wxT("%s")) % ((CPartFile*)file)->GetPartMetFileName().RemoveExt())
							: file->GetFilePath().GetPrintable(),
		valuemap);

	AddTag(EC_TAG_PARTFILE_SIZE_FULL, file->GetFileSize(), valuemap);

	AddTag(EC_TAG_PARTFILE_ED2K_LINK,
			theApp->CreateED2kLink(file, (theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID())), valuemap);
}

CEC_UpDownClient_Tag::CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap) :
	CECTag(EC_TAG_CLIENT, client->ECID())
{
	// General
	AddTag(CECTag(EC_TAG_CLIENT_NAME, client->GetUserName()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_HASH, client->GetUserHash()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_USER_ID, client->GetUserIDHybrid()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SCORE, client->GetScore(false, client->IsDownloading(), false)), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_RATING, client->GetRating()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SOFTWARE, client->GetClientSoft()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SOFT_VER_STR, client->GetSoftVerStr()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_USER_IP, client->GetConnectIP()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_USER_PORT, client->GetUserPort()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_FROM, (uint64)client->GetSourceFrom()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SERVER_IP, client->GetServerIP()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SERVER_PORT, client->GetServerPort()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_SERVER_NAME, client->GetServerName()), valuemap);
	
	// Transfers to Client
	AddTag(CECTag(EC_TAG_CLIENT_UP_SPEED, client->GetUploadDatarate()), valuemap);
	if (client->GetDownloadState() == DS_DOWNLOADING || valuemap) {
		AddTag(CECTag(EC_TAG_CLIENT_DOWN_SPEED, (double)(client->GetKBpsDown())), valuemap);
	}
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_SESSION, client->GetSessionUp()), valuemap);
	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, client->GetTransferredDown()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_TOTAL, client->GetUploadedTotal()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_DOWNLOAD_TOTAL, client->GetDownloadedTotal()), valuemap);
	
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_STATE, client->GetUploadState()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_DOWNLOAD_STATE, client->GetDownloadState()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_IDENT_STATE, (uint64) client->GetCurrentIdentState()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_OBFUSCATED_CONNECTION, client->HasObfuscatedConnectionBeenEstablished()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_EXT_PROTOCOL, client->ExtProtocolAvailable()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_WAIT_TIME, client->GetWaitTime()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_XFER_TIME, client->GetUpStartTimeDelay()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_QUEUE_TIME, (uint64)(::GetTickCount() - client->GetWaitStartTime())), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_LAST_TIME, (uint64)(::GetTickCount() - client->GetLastUpRequest())), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_WAITING_POSITION, theApp->uploadqueue->GetWaitingPosition(client)), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_REMOTE_QUEUE_RANK, client->IsRemoteQueueFull() ? (uint16)0xffff : client->GetRemoteQueueRank()), valuemap);
	AddTag(CECTag(EC_TAG_CLIENT_ASKED_COUNT, client->GetAskedCount()), valuemap);
	
	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	const CKnownFile* file = client->GetUploadFile();
	if (file) {
		AddTag(CECTag(EC_TAG_PARTFILE_NAME, file->GetFileName().GetPrintable()), valuemap);
		AddTag(CECTag(EC_TAG_KNOWNFILE, file->ECID()), valuemap);
	}
	
}

//
// Search reply
//
CEC_SearchFile_Tag::CEC_SearchFile_Tag(CSearchFile *file, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap) : CECTag(EC_TAG_SEARCHFILE, file->GetFileHash())
{
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT, file->GetSourceCount()), valuemap);
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, file->GetCompleteSourceCount()), valuemap);
	AddTag(CECTag(EC_TAG_PARTFILE_STATUS, (uint32)file->GetDownloadStatus()), valuemap);

	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}

	AddTag(CECTag(EC_TAG_PARTFILE_NAME, file->GetFileName().GetPrintable()), valuemap);
	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, file->GetFileSize()), valuemap);
}
// File_checked_for_headers
