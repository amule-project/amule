//
// This file is part of the aMule Project
//
// aMule Copyright (C) 2003 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ECSpecialTags.h"
#endif

#include "ECPacket.h"		// Needed for CECTag
#include "ECcodes.h"		// Needed for TAGnames
#include "ECSpecialTags.h"	// Needed for special EC tag creator classes

// Since there are only constructors defined here,
// removing everything from non-local builds.
#ifndef EC_REMOTE

#include "server.h"		// Needed for CServer
#include "PartFile.h"		// Needed for CPartFile
#include "sockets.h"		// Needed for CServerConnect
#include "amule.h"		// Needed for theApp

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
	(uint8) (theApp.serverconnect->IsConnected() ? (theApp.serverconnect->IsLowID() ? 2 : 3) : 
		theApp.serverconnect->IsConnecting() ? 1 : 0))
{
	if ( theApp.serverconnect->GetCurrentServer() ) {
		AddTag(CEC_Server_Tag(theApp.serverconnect->GetCurrentServer(), detail_level));
	}
}

CEC_PartFile_Tag::CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level)
	: CECTag(EC_TAG_PARTFILE, PTR_2_ID(file))
{
	AddTag(CECTag(EC_TAG_PARTFILE_STATUS, file->GetStatus()));

	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT, (uint32)file->GetSourceCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT, (uint32)file->GetNotCurrentSourcesCount()));
	AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, (uint32)file->GetTransferingSrcCount()));
	
	// FIXME: this will be replaced by RLE encoded data
	//AddTag(CEC_PartStatus_Tag(file, 200));
	
	if ( (file->GetTransferingSrcCount() > 0) || (detail_level != EC_DETAIL_UPDATE) ) {
		
		AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, (uint32)file->GetTransfered()));
		AddTag(CECTag(EC_TAG_PARTFILE_SIZE_DONE, (uint32)file->GetCompletedSize()));
		AddTag(CECTag(EC_TAG_PARTFILE_SPEED, (uint32)(file->GetKBpsDown()*1024)));
	}
	
	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	
	AddTag(CECTag(EC_TAG_PARTFILE_NAME,file->GetFileName()));

	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, (uint32)file->GetFileSize()));

	AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
		(uint32)(file->IsAutoDownPriority() ? 
						file->GetDownPriority() + 10 : file->GetDownPriority())));

	AddTag(CECTag(EC_TAG_PARTFILE_ED2K_LINK,
				(theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID()) ?
					theApp.CreateED2kSourceLink(file) : theApp.CreateED2kLink(file)));
}

CEC_PartStatus_Tag::CEC_PartStatus_Tag(CPartFile *file, int statussize) :
	CECTag(EC_TAG_PARTFILE_PART_STATUS, file->GetProgressString(statussize))
{
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

CEC_SharedFile_Tag::CEC_SharedFile_Tag(CKnownFile *file, EC_DETAIL_LEVEL detail_level) : CECTag(EC_TAG_KNOWNFILE, PTR_2_ID(file))
{
	AddTag(CECTag(EC_TAG_KNOWNFILE_REQ_COUNT, (uint32)file->statistic.GetRequests()));
	AddTag(CECTag(EC_TAG_KNOWNFILE_REQ_COUNT_ALL, (uint32)file->statistic.GetAllTimeRequests()));
	
	AddTag(CECTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT, (uint32)file->statistic.GetAccepts()));
	AddTag(CECTag(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL, (uint32)file->statistic.GetAllTimeAccepts()));

	if (detail_level == EC_DETAIL_UPDATE) {
			return;
	}
	
	AddTag(CECTag(EC_TAG_PARTFILE_NAME,file->GetFileName()));

	AddTag(CECTag(EC_TAG_PARTFILE_SIZE_FULL, (uint32)file->GetFileSize()));

	AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
		(uint32)(file->IsAutoUpPriority() ? 
						file->GetUpPriority() + 10 : file->GetUpPriority())));

	AddTag(CECTag(EC_TAG_PARTFILE_ED2K_LINK,
				(theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID()) ?
					theApp.CreateED2kSourceLink(file) : theApp.CreateED2kLink(file)));
}

#endif /* ! EC_REMOTE */
