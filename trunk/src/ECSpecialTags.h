//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( http://www.amule.org )
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

#ifndef ECSPECIALTAGS_H
#define ECSPECIALTAGS_H

#include "Types.h"	// Needed for uint* types
#include <wx/string.h>	// Needed for wxString
#include "ECcodes.h"	// Needed for EC types
#include "ECPacket.h"	// Needed for CECTag
#include "CMD4Hash.h"	// Needed for CMD4Hash

#include <vector>

#ifndef EC_REMOTE
#include "Statistics.h"	// Needed for StatsTree
#endif

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ECSpecialTags.h"
#endif

/*
 * Specific tags for specific requests
 *
 * \note EC remote end does not need to create these packets,
 * only using the getter functions.
 *
 * Regarding this, those classes are removed from remote build,
 * that have only a constructor.
 */

class CServer;
class CKnownFile;
class CPartFile;
class CSearchFile;
class CUpDownClient;

class CEC_Prefs_Packet : public CECPacket {
 	public:
 		CEC_Prefs_Packet(uint32 selection, EC_DETAIL_LEVEL);
};

class CEC_Server_Tag : public CECTag {
 	public:
 		CEC_Server_Tag(CServer *, EC_DETAIL_LEVEL);
 		
 		wxString ServerName() { return GetTagByNameSafe(EC_TAG_SERVER_NAME)->GetStringData(); }
 		wxString ServerDesc() { return GetTagByNameSafe(EC_TAG_SERVER_DESC)->GetStringData(); }

 		uint8 GetPrio() { return GetTagByNameSafe(EC_TAG_SERVER_PRIO)->GetInt8Data(); }
 		uint8 GetStatic() { return GetTagByNameSafe(EC_TAG_SERVER_STATIC)->GetInt8Data(); }

 		uint32 GetPing() { return GetTagByNameSafe(EC_TAG_SERVER_PING)->GetInt32Data(); }
 		uint8 GetFailed() { return GetTagByNameSafe(EC_TAG_SERVER_FAILED)->GetInt8Data(); }

 		uint32 GetFiles() { return GetTagByNameSafe(EC_TAG_SERVER_FILES)->GetInt32Data(); }
 		uint32 GetUsers() { return GetTagByNameSafe(EC_TAG_SERVER_USERS)->GetInt32Data(); }
 		uint32 GetMaxUsers() { return GetTagByNameSafe(EC_TAG_SERVER_USERS_MAX)->GetInt32Data(); }
};


class CEC_ConnState_Tag : public CECTag {
 	public:
 		CEC_ConnState_Tag(EC_DETAIL_LEVEL);
 		
 		bool IsConnected() { return ClientID() && (ClientID() != 0xffffffff); }
 		bool IsConnecting() {return (ClientID() == 0xffffffff); }
 		bool HaveLowID() { return ClientID() < 16777216; }
 		
 		// 0  : disconnected
 		// 0xffffffff : connecting
 		// other: client ID
 		uint32 ClientID() { return GetInt32Data(); }
};

class CEC_Stats_Tag : public CECTag {
	public:
		CEC_Stats_Tag();
		
		uint32 UpSpeed() { return GetTagByNameSafe(EC_TAG_STATS_UL_SPEED)->GetInt32Data(); }
		uint32 DownSpeed() { return GetTagByNameSafe(EC_TAG_STATS_DL_SPEED)->GetInt32Data(); }
		
		uint32 ClientsInQueue() { return GetTagByNameSafe(EC_TAG_STATS_UL_QUEUE_LEN)->GetInt32Data(); }
		uint32 UsersOnServer() { return GetTagByNameSafe(EC_TAG_STATS_USERS_ON_SERVER)->GetInt32Data(); }
};

class CEC_PartFile_Tag : public CECTag {
 	public:
 		CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level);
 		
		// template needs it
		CMD4Hash		ID()	{ return GetMD4Data(); }

 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString() { return GetMD4Data().Encode(); }

 		wxString	FileName()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32		SizeFull()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
 		uint32		SizeXfer()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_XFER)->GetInt32Data(); }
  		uint32		SizeDone()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_DONE)->GetInt32Data(); }
 		wxString	FileEd2kLink()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }
 		uint8		FileStatus()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_STATUS)->GetInt8Data(); }
  		uint32		SourceCount()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT)->GetInt32Data(); }
  		uint32		SourceNotCurrCount()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT)->GetInt32Data(); }
  		uint32		SourceXferCount()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT_XFER)->GetInt32Data(); }
  		uint32		SourceCountA4AF()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT_A4AF)->GetInt32Data(); }
  		uint32		Speed()		{ return GetTagByNameSafe(EC_TAG_PARTFILE_SPEED)->GetInt32Data(); }
  		uint32		Prio()		{ return GetTagByNameSafe(EC_TAG_PARTFILE_PRIO)->GetInt32Data(); }
  		wxString	PartStatus()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_PART_STATUS)->GetStringData(); }
 		uint8		FileCat()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_CAT)->GetInt8Data(); }
		time_t		LastSeenComplete() { return (time_t)GetTagByNameSafe(EC_TAG_PARTFILE_LAST_SEEN_COMP)->GetInt32Data(); }
		#ifdef EC_REMOTE
		wxString	GetFileStatusString();
		#endif /* EC_REMOTE */
};

class CEC_SharedFile_Tag : public CECTag {
	public:
		CEC_SharedFile_Tag(const CKnownFile *file, EC_DETAIL_LEVEL detail_level);

		// template needs it
 		CMD4Hash	ID()		{ return GetMD4Data(); }
		
 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString(){ return GetMD4Data().Encode(); }

 		wxString	FileName()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32		SizeFull()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
  		uint32		Prio()		{ return GetTagByNameSafe(EC_TAG_PARTFILE_PRIO)->GetInt32Data(); }
 		wxString	FileEd2kLink()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }

 		uint32		GetRequests()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_REQ_COUNT)->GetInt32Data(); }
 		uint32		GetAllRequests()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_REQ_COUNT_ALL)->GetInt32Data(); }

 		uint32		GetAccepts()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_ACCEPT_COUNT)->GetInt32Data(); }
 		uint32		GetAllAccepts()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL)->GetInt32Data(); }

 		uint32		GetXferred()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_XFERRED)->GetInt32Data(); }
 		uint32		GetAllXferred()	{ return GetTagByNameSafe(EC_TAG_KNOWNFILE_XFERRED_ALL)->GetInt32Data(); }
};

class CEC_UpDownClient_Tag : public CECTag {
	public:
		CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level);

		uint32 ID() { return GetInt32Data(); }
		
 		CMD4Hash FileID() { return GetTagByNameSafe(EC_TAG_KNOWNFILE)->GetMD4Data(); }
 		bool HaveFile() { return GetTagByName(EC_TAG_KNOWNFILE) != NULL; }

 		wxString ClientName() { return GetTagByNameSafe(EC_TAG_CLIENT_NAME)->GetStringData(); }
 		uint32 SpeedUp() { return GetTagByNameSafe(EC_TAG_CLIENT_UP_SPEED)->GetInt32Data(); }
 		uint32 SpeedDown() { return GetTagByNameSafe(EC_TAG_CLIENT_DOWN_SPEED)->GetInt32Data(); }
 		
 		uint64 XferUp() { return GetTagByNameSafe(EC_TAG_CLIENT_UPLOAD_TOTAL)->GetInt64Data(); };
 		uint64 XferDown() { return GetTagByNameSafe(EC_TAG_CLIENT_DOWNLOAD_TOTAL)->GetInt64Data(); }
 		uint32 XferUpSession() { return GetTagByNameSafe(EC_TAG_CLIENT_UPLOAD_SESSION)->GetInt32Data(); }
 		
 		bool IsFriend() { return (GetTagByName(EC_TAG_CLIENT_FRIEND) != 0); }
 		
 		uint8 ClientSoftware() { return GetTagByNameSafe(EC_TAG_CLIENT_SOFTWARE)->GetInt8Data(); }
 		
 		uint8 ClientState() { return GetTagByNameSafe(EC_TAG_CLIENT_STATE)->GetInt16Data(); }
 		
 		uint32 WaitTime() { return GetTagByNameSafe(EC_TAG_CLIENT_WAIT_TIME)->GetInt32Data(); }
 		uint32 XferTime() { return GetTagByNameSafe(EC_TAG_CLIENT_XFER_TIME)->GetInt32Data(); }
 		uint32 LastReqTime() { return GetTagByNameSafe(EC_TAG_CLIENT_LAST_TIME)->GetInt32Data(); }
 		uint32 QueueTime() { return GetTagByNameSafe(EC_TAG_CLIENT_QUEUE_TIME)->GetInt32Data(); }
};

class CEC_SearchFile_Tag : public CECTag {
	public:
		CEC_SearchFile_Tag(CSearchFile *file, EC_DETAIL_LEVEL detail_level);

		// template needs it
 		CMD4Hash	ID()	{ return GetMD4Data(); }

 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString() { return GetMD4Data().Encode(); }

 		wxString	FileName()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32		SizeFull()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
  		uint32		SourceCount()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT)->GetInt32Data(); }
  		uint32		CompleteSourceCount()	{ return GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_COUNT_XFER)->GetInt32Data(); }
  		bool		AlreadyHave()	{ return GetTagByName(EC_TAG_KNOWNFILE) != 0; }
};

class CEC_Search_Tag : public CECTag {
	public:
		// search request
		CEC_Search_Tag(wxString &name, EC_SEARCH_TYPE search_type, wxString &file_type,
			wxString &extension, uint32 avail, uint32 min_size, uint32 max_size);
			
		wxString SearchText() { return GetTagByNameSafe(EC_TAG_SEARCH_NAME)->GetStringData(); }
		EC_SEARCH_TYPE SearchType() { return (EC_SEARCH_TYPE)GetInt32Data(); }
		uint32 MinSize() { return GetTagByNameSafe(EC_TAG_SEARCH_MIN_SIZE)->GetInt32Data(); }
		uint32 MaxSize() { return GetTagByNameSafe(EC_TAG_SEARCH_MAX_SIZE)->GetInt32Data(); }
		uint32 Avail() { return GetTagByNameSafe(EC_TAG_SEARCH_AVAILABILITY)->GetInt32Data(); }
		wxString SearchExt() { return GetTagByNameSafe(EC_TAG_SEARCH_EXTENSION)->GetStringData(); }
		wxString SearchFileType() { return GetTagByNameSafe(EC_TAG_SEARCH_FILE_TYPE)->GetStringData(); }
};

#ifndef EC_REMOTE
class CEC_Tree_Tag : public CECTag {
	public:
		CEC_Tree_Tag(const StatsTreeSiblingIterator& tr);
};
#endif

#endif /* ECSPEACIALTAGS_H */
