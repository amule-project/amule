//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 Kry ( elkry@sourceforge.net / http://www.amule-project.net )
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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
	//AddTag(CECTag(EC_TAG_PARTFILE_LAST_SEEN_COMP, file->lastseencomplete));

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

CEC_UpDownClient_Tag::CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level) :
	CECTag(EC_TAG_UPDOWN_CLIENT, client->GetUserID())
{
	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, (uint32)client->GetTransferedDown()));
	
	AddTag(CECTag(EC_TAG_CLIENT_UPLOAD_TOTAL, (uint32)client->Credits()->GetUploadedTotal()));
	AddTag(CECTag(EC_TAG_CLIENT_DOWNLOAD_TOTAL, (uint32)client->Credits()->GetDownloadedTotal()));
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
