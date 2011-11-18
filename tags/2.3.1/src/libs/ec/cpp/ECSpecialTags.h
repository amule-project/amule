//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef ECSPECIALTAGS_H
#define ECSPECIALTAGS_H

//#warning Kry - Preferences packet derived from packet, and that's ok, but shouldn't be here because this is a tag file and forces a stupid include
#include "ECPacket.h"					// Needed for CECPacket
#include "../../../NetworkFunctions.h"	// Needed for IsLowID
#include "../../../ClientCredits.h"		// Needed for EIdentState
#include <common/Format.h>	// Needed for CFormat


#include <map>

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
class CFriend;

/*
 * EC tags encoder. Idea: if for an object <X>, client <Z> tag <Y> have value equal to previous
 * request, skip this tag.
 */

class CValueMap {
		/*
		 * Tag -> LastValue map. Hold last value that transmitted to remote side
		 */
		std::map<ec_tagname_t, uint8> m_map_uint8;
		std::map<ec_tagname_t, uint16> m_map_uint16;
		std::map<ec_tagname_t, uint32> m_map_uint32;
		std::map<ec_tagname_t, uint64> m_map_uint64;
		std::map<ec_tagname_t, CMD4Hash> m_map_md4;
		std::map<ec_tagname_t, wxString> m_map_string;
		std::map<ec_tagname_t, CECTag> m_map_tag;
		
		template <class T>
		void CreateTagT(ec_tagname_t tagname, T value, std::map<ec_tagname_t, T> &map, CECTag *parent)
		{
			if ( (map.count(tagname) == 0) || (map[tagname] != value) ) {
				parent->AddTag(CECTag(tagname, value));
				map[tagname] = value;
			}
		}
	public:
		CValueMap()
		{
		}
		
		CValueMap(const CValueMap &valuemap)
		{
			m_map_uint8 = valuemap.m_map_uint8;
			m_map_uint16 = valuemap.m_map_uint16;
			m_map_uint32 = valuemap.m_map_uint32;
			m_map_uint64 = valuemap.m_map_uint64;
			m_map_md4 = valuemap.m_map_md4;
			m_map_string = valuemap.m_map_string;
			m_map_tag = valuemap.m_map_tag;
		}
		
		void CreateTag(ec_tagname_t tagname, uint8 value, CECTag *parent)
		{
			CreateTagT<uint8>(tagname, value, m_map_uint8, parent);
		}
		
		void CreateTag(ec_tagname_t tagname, uint16 value, CECTag *parent)
		{
			CreateTagT<uint16>(tagname, value, m_map_uint16, parent);
		}
		
		void CreateTag(ec_tagname_t tagname, uint32 value, CECTag *parent)
		{
			CreateTagT<uint32>(tagname, value, m_map_uint32, parent);
		}
		
		void CreateTag(ec_tagname_t tagname, uint64 value, CECTag *parent)
		{
			CreateTagT<uint64>(tagname, value, m_map_uint64, parent);
		}
		
		void CreateTag(ec_tagname_t tagname, CMD4Hash value, CECTag *parent)
		{
			CreateTagT<CMD4Hash>(tagname, value, m_map_md4, parent);
		}
		
		void CreateTag(ec_tagname_t tagname, wxString value, CECTag *parent)
		{
			CreateTagT<wxString>(tagname, value, m_map_string, parent);
		}
		
		bool AddTag(const CECTag &tag, CECTag *parent)
		{
			bool ret = false;
			ec_tagname_t tagname = tag.GetTagName();
			if (m_map_tag.count(tagname) == 0 || m_map_tag[tagname] != tag) {
				m_map_tag[tagname] = tag;
				parent->AddTag(tag);
				ret = true;
			}
			return ret;
		}

		void ForgetTag(ec_tagname_t tagname)
		{
			m_map_tag.erase(tagname);
		}
};

class CEC_Category_Tag : public CECTag {
 	public:
 		CEC_Category_Tag(uint32 cat_index, EC_DETAIL_LEVEL detail_level = EC_DETAIL_FULL);
 		// for create-upate commands
 		CEC_Category_Tag(uint32 cat_index, wxString name, wxString path,
			wxString comment, uint32 color, uint8 prio);
 		
 		bool Apply();
 		bool Create();
 		
 		wxString Name() const { return GetTagByNameSafe(EC_TAG_CATEGORY_TITLE)->GetStringData(); }
 		wxString Path() const { return GetTagByNameSafe(EC_TAG_CATEGORY_PATH)->GetStringData(); }
 		wxString Comment() const { return GetTagByNameSafe(EC_TAG_CATEGORY_COMMENT)->GetStringData(); }
 		uint8 Prio() const { return GetTagByNameSafe(EC_TAG_CATEGORY_PRIO)->GetInt(); }
 		uint32 Color() const { return GetTagByNameSafe(EC_TAG_CATEGORY_COLOR)->GetInt(); }
 		
};

class CEC_Prefs_Packet : public CECPacket {
 	public:
 		CEC_Prefs_Packet(uint32 selection, EC_DETAIL_LEVEL prefs_detail = EC_DETAIL_FULL, EC_DETAIL_LEVEL cat_details = EC_DETAIL_FULL);

 		void Apply();
};

class CEC_Server_Tag : public CECTag {
 	public:
 		CEC_Server_Tag(const CServer *, EC_DETAIL_LEVEL);
 		CEC_Server_Tag(const CServer *, CValueMap *valuemap);
 		
 		wxString ServerName(wxString * target = 0)	const { return AssignIfExist(EC_TAG_SERVER_NAME, target); }
 		wxString ServerDesc(wxString * target = 0)	const { return AssignIfExist(EC_TAG_SERVER_DESC, target); }
 		wxString ServerVersion(wxString * target = 0)	const { return AssignIfExist(EC_TAG_SERVER_VERSION, target); }

 		uint32 GetPrio(uint32 * target = 0)			const { return AssignIfExist(EC_TAG_SERVER_PRIO, target); }
 		bool GetStatic(bool * target = 0)			const { return AssignIfExist(EC_TAG_SERVER_STATIC, target); }

 		uint32 GetPing(uint32 * target = 0)			const { return AssignIfExist(EC_TAG_SERVER_PING, target); }
 		uint32 GetFailed(uint32 * target = 0)		const { return AssignIfExist(EC_TAG_SERVER_FAILED, target); }

 		uint32 GetFiles(uint32 * target = 0)		const { return AssignIfExist(EC_TAG_SERVER_FILES, target); }
 		uint32 GetUsers(uint32 * target = 0)		const { return AssignIfExist(EC_TAG_SERVER_USERS, target); }
 		uint32 GetMaxUsers(uint32 * target = 0)		const { return AssignIfExist(EC_TAG_SERVER_USERS_MAX, target); }
 		
 		uint32 ID() const { return GetInt(); }
};


class CEC_ConnState_Tag : public CECTag {
 	public:
 		CEC_ConnState_Tag(EC_DETAIL_LEVEL);

		uint32	GetEd2kId()			const { return GetTagByNameSafe(EC_TAG_ED2K_ID)->GetInt(); }
		uint32	GetClientId()		const { return GetTagByNameSafe(EC_TAG_CLIENT_ID)->GetInt(); }
 		bool	HasLowID()			const { return GetEd2kId() < HIGHEST_LOWID_ED2K_KAD; }
 		bool	IsConnected()		const { return IsConnectedED2K() || IsConnectedKademlia(); }
 		bool	IsConnectedED2K()	const { return (GetInt() & 0x01) != 0; }
 		bool	IsConnectingED2K()	const { return (GetInt() & 0x02) != 0; }
		bool	IsConnectedKademlia()	const { return (GetInt() & 0x04) != 0; }
		bool	IsKadFirewalled()	const { return (GetInt() & 0x08) != 0; }
		bool	IsKadRunning()	const { return (GetInt() & 0x10) != 0; }
};

class CEC_SharedFile_Tag : public CECTag {
	public:
 		CEC_SharedFile_Tag(const CKnownFile *file, EC_DETAIL_LEVEL detail_level, 
							CValueMap *valuemap = NULL, ec_tagname_t name = EC_TAG_KNOWNFILE);
 		
		// template needs it
 		uint32		ID()		const { return GetInt(); }
		
 		CMD4Hash	FileHash()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_HASH)->GetMD4Data(); }
		wxString	FileHashString() const { return FileHash().Encode(); }

 		wxString	FileName()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		bool		FileName(wxString &target)	const { return AssignIfExist(EC_TAG_PARTFILE_NAME, target); }
 		wxString	FilePath()	const { return GetTagByNameSafe(EC_TAG_KNOWNFILE_FILENAME)->GetStringData(); }
 		bool		FilePath(wxString &target)	const { return AssignIfExist(EC_TAG_KNOWNFILE_FILENAME, target); }
 		uint64		SizeFull()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_FULL)->GetInt(); }
 		wxString	FileEd2kLink()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }

  		uint8		UpPrio(uint8 *target = 0)			const { return AssignIfExist(EC_TAG_KNOWNFILE_PRIO, target); }
 		uint16		GetRequests(uint16 *target = 0)		const { return AssignIfExist(EC_TAG_KNOWNFILE_REQ_COUNT, target); }
 		uint32		GetAllRequests(uint32 *target = 0)	const { return AssignIfExist(EC_TAG_KNOWNFILE_REQ_COUNT_ALL, target); }

 		uint16		GetAccepts(uint16 *target = 0)		const { return AssignIfExist(EC_TAG_KNOWNFILE_ACCEPT_COUNT, target); }
 		uint32		GetAllAccepts(uint32 *target = 0)	const { return AssignIfExist(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL, target); }

 		uint64		GetXferred(uint64 *target = 0)		const { return AssignIfExist(EC_TAG_KNOWNFILE_XFERRED, target); }
 		uint64		GetAllXferred(uint64 *target = 0)	const { return AssignIfExist(EC_TAG_KNOWNFILE_XFERRED_ALL, target); }

 		uint16		GetCompleteSourcesLow(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_KNOWNFILE_COMPLETE_SOURCES_LOW, target); }
 		uint16		GetCompleteSourcesHigh(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_KNOWNFILE_COMPLETE_SOURCES_HIGH, target); }
 		uint16		GetCompleteSources(uint16 *target = 0)		const { return AssignIfExist(EC_TAG_KNOWNFILE_COMPLETE_SOURCES, target); }

		uint16		GetOnQueue(uint16 *target = 0)		const { return AssignIfExist(EC_TAG_KNOWNFILE_ON_QUEUE, target); }

		bool		GetComment(wxString &target)	const { return AssignIfExist(EC_TAG_KNOWNFILE_COMMENT, target); }
		bool		GetRating(uint8 &target)		const { return AssignIfExist(EC_TAG_KNOWNFILE_RATING, target); }

		bool		GetAICHHash(wxString &target)	const { return AssignIfExist(EC_TAG_KNOWNFILE_AICH_MASTERHASH, target); }
	private:
		CMD4Hash	GetMD4Data();	// Block it, because it doesn't work anymore! 
};

class CEC_PartFile_Tag : public CEC_SharedFile_Tag {
 	public:
 		CEC_PartFile_Tag(const CPartFile *file, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap = NULL);
 		
		uint16		PartMetID()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_PARTMETID)->GetInt(); }

 		uint64		SizeXfer(uint64 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SIZE_XFER, target); }
  		uint64		SizeDone(uint64 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SIZE_DONE, target); }
 		uint8		FileStatus(uint8 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_STATUS, target); }
 		bool		Stopped(bool *target = 0)		const { return AssignIfExist(EC_TAG_PARTFILE_STOPPED, target); }
  		uint16		SourceCount(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT, target); }
  		uint16		SourceNotCurrCount(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT, target); }
  		uint16		SourceXferCount(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, target); }
  		uint16		SourceCountA4AF(uint16 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT_A4AF, target); }
  		uint32		Speed(uint32 *target = 0)		const { return AssignIfExist(EC_TAG_PARTFILE_SPEED, target); }
  		uint8		DownPrio(uint8 *target = 0)		const { return AssignIfExist(EC_TAG_PARTFILE_PRIO, target); }
 		uint8		FileCat(uint8 *target = 0)		const { return AssignIfExist(EC_TAG_PARTFILE_CAT, target); }
		time_t		LastSeenComplete(time_t *target = 0)const { return AssignIfExist(EC_TAG_PARTFILE_LAST_SEEN_COMP, target); }
		time_t		LastDateChanged(time_t *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_LAST_RECV, target); }
		uint32		DownloadActiveTime(uint32 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_DOWNLOAD_ACTIVE, target); }
		uint16		AvailablePartCount(uint16 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_AVAILABLE_PARTS, target); }
 		bool		Shared(bool *target = 0)		const { return AssignIfExist(EC_TAG_PARTFILE_SHARED, target); }
 		bool		A4AFAuto(bool &target)			const { return AssignIfExist(EC_TAG_PARTFILE_A4AFAUTO, target); }

		uint64		GetLostDueToCorruption(uint64 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_LOST_CORRUPTION, target); }
		uint64		GetGainDueToCompression(uint64 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_GAINED_COMPRESSION, target); }
		uint32		TotalPacketsSavedDueToICH(uint32 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_SAVED_ICH, target); }

		wxString	PartMetName() const
		{
			uint16 id = PartMetID();
			return id ? (CFormat(wxT("%03u.part.met")) % id) : wxEmptyString;
		}

		wxString	GetFileStatusString() const;
};

class CEC_UpDownClient_Tag : public CECTag {
	public:
		CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level, CValueMap* valuemap);

		uint32 ID() const { return GetInt(); }
		
 		CMD4Hash UserHash(CMD4Hash * target = 0) const { return AssignIfExist(EC_TAG_CLIENT_HASH, target); }
 		uint32 UserID(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_USER_ID, target); }
 		
 		wxString ClientName(wxString *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_NAME, target); }
 		uint32 SpeedUp(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_UP_SPEED, target); }
 		float SpeedDown(float *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_DOWN_SPEED, target); }
 		
 		uint64 XferUp(uint64 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_UPLOAD_TOTAL, target); };
 		uint64 XferDown(uint64 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_DOWNLOAD_TOTAL, target); }
 		uint64 XferUpSession(uint64 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_UPLOAD_SESSION, target); }
 		uint64 XferDownSession(uint64 *target = 0) const { return AssignIfExist(EC_TAG_PARTFILE_SIZE_XFER, target); }
 		
 		bool FriendSlot(bool &target) const { return AssignIfExist(EC_TAG_CLIENT_FRIEND_SLOT, target); }
 		
 		bool ClientSoftware(uint8 &target) const { return AssignIfExist(EC_TAG_CLIENT_SOFTWARE, target); }
		bool SoftVerStr(wxString &target) const { return AssignIfExist(EC_TAG_CLIENT_SOFT_VER_STR, target); }
 		
 		bool ClientUploadState(uint8 &target) const { return AssignIfExist(EC_TAG_CLIENT_UPLOAD_STATE, target); }
 		bool ClientDownloadState(uint8 &target) const { return AssignIfExist(EC_TAG_CLIENT_DOWNLOAD_STATE, target); }
 		
 		//uint32 WaitTime(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_WAIT_TIME, target); }
 		//uint32 XferTime(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_XFER_TIME, target); }
 		//uint32 LastReqTime(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_LAST_TIME, target); }
 		//uint32 QueueTime(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_QUEUE_TIME, target); }
		bool GetSourceFrom(uint8 &target) const { return AssignIfExist(EC_TAG_CLIENT_FROM, target); }

		bool   UserIP(uint32 &target) const { return AssignIfExist(EC_TAG_CLIENT_USER_IP, target); }
		uint16 UserPort(uint16 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_USER_PORT, target); }
		uint32 ServerIP(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_SERVER_IP, target); }
		uint16 ServerPort(uint16 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_SERVER_PORT, target); }
		wxString ServerName(wxString *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_SERVER_NAME, target); }
		bool KadPort(uint16 &target) const { return AssignIfExist(EC_TAG_CLIENT_KAD_PORT, target); }

		uint32 Score(uint32 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_SCORE, target); }
		uint16 WaitingPosition(uint16 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_WAITING_POSITION, target); }
		uint16 RemoteQueueRank(uint16 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_REMOTE_QUEUE_RANK, target); }
		uint16 OldRemoteQueueRank(uint16 *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_OLD_REMOTE_QUEUE_RANK, target); }

		EIdentState GetCurrentIdentState(EIdentState * target = 0) const { return (EIdentState) AssignIfExist(EC_TAG_CLIENT_IDENT_STATE, (uint32 *) target); }
		bool ObfuscationStatus(uint8 &target) const { return AssignIfExist(EC_TAG_CLIENT_OBFUSCATION_STATUS, target); }
		bool HasExtendedProtocol(bool *target = 0) const { return AssignIfExist(EC_TAG_CLIENT_EXT_PROTOCOL, target); }
		bool NextRequestedPart(uint16 &target) const { return AssignIfExist(EC_TAG_CLIENT_NEXT_REQUESTED_PART, target); }
		bool LastDownloadingPart(uint16 &target) const { return AssignIfExist(EC_TAG_CLIENT_LAST_DOWNLOADING_PART, target); }

		bool UploadFile(uint32 &target) const { return AssignIfExist(EC_TAG_CLIENT_UPLOAD_FILE, target); }
		bool RequestFile(uint32 &target) const { return AssignIfExist(EC_TAG_CLIENT_REQUEST_FILE, target); }
		bool RemoteFilename(wxString &target) const { return AssignIfExist(EC_TAG_CLIENT_REMOTE_FILENAME, target); }
 		bool DisableViewShared(bool &target) const { return AssignIfExist(EC_TAG_CLIENT_DISABLE_VIEW_SHARED, target); }
 		bool Version(uint32 &target) const { return AssignIfExist(EC_TAG_CLIENT_VERSION, target); }
 		bool ModVersion(wxString &target) const { return AssignIfExist(EC_TAG_CLIENT_MOD_VERSION, target); }
 		bool OSInfo(wxString &target) const { return AssignIfExist(EC_TAG_CLIENT_OS_INFO, target); }
 		bool AvailableParts(uint16 &target) const { return AssignIfExist(EC_TAG_CLIENT_AVAILABLE_PARTS, target); }
	private:
		CMD4Hash	GetMD4Data();	// Block it, because it doesn't work anymore! 
};

class CEC_SearchFile_Tag : public CECTag {
	public:
		CEC_SearchFile_Tag(CSearchFile *file, EC_DETAIL_LEVEL detail_level, CValueMap *valuemap = NULL);

		// template needs it
 		uint32		ID()	const { return GetInt(); }
		uint32		ParentID()	const { return GetTagByNameSafe(EC_TAG_SEARCH_PARENT)->GetInt(); }

 		CMD4Hash	FileHash()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_HASH)->GetMD4Data(); }
		wxString	FileHashString() const { return FileHash().Encode(); }

 		wxString	FileName()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint64		SizeFull()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_SIZE_FULL)->GetInt(); }
  		uint32		SourceCount(uint32 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT, target); }
  		uint32		CompleteSourceCount(uint32 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_SOURCE_COUNT_XFER, target); }
		bool		AlreadyHave()	const { return GetTagByNameSafe(EC_TAG_PARTFILE_STATUS)->GetInt() != 0; /* == CSearchFile::NEW */ }
		uint32		DownloadStatus(uint32 *target = 0)	const { return AssignIfExist(EC_TAG_PARTFILE_STATUS, target); }
	private:
		CMD4Hash	GetMD4Data();	// Block it, because it doesn't work anymore! 
};

class CEC_Search_Tag : public CECTag {
	public:
		// search request
		CEC_Search_Tag(const wxString &name, EC_SEARCH_TYPE search_type, const wxString &file_type,
			const wxString &extension, uint32 avail, uint64 min_size, uint64 max_size);
			
		wxString SearchText() const { return GetTagByNameSafe(EC_TAG_SEARCH_NAME)->GetStringData(); }
		EC_SEARCH_TYPE SearchType() const { return (EC_SEARCH_TYPE)GetInt(); }
		uint64 MinSize() const { return GetTagByNameSafe(EC_TAG_SEARCH_MIN_SIZE)->GetInt(); }
		uint64 MaxSize() const { return GetTagByNameSafe(EC_TAG_SEARCH_MAX_SIZE)->GetInt(); }
		uint32 Avail() const { return GetTagByNameSafe(EC_TAG_SEARCH_AVAILABILITY)->GetInt(); }
		wxString SearchExt() const { return GetTagByNameSafe(EC_TAG_SEARCH_EXTENSION)->GetStringData(); }
		wxString SearchFileType() const { return GetTagByNameSafe(EC_TAG_SEARCH_FILE_TYPE)->GetStringData(); }
};

class CEC_StatTree_Node_Tag : public CECTag {
	public:
		CEC_StatTree_Node_Tag();	// just to keep compiler happy
		wxString GetDisplayString() const;
};

class CEC_Friend_Tag : public CECTag {
	public:
		CEC_Friend_Tag(const CFriend* Friend, CValueMap* valuemap);

 		uint32		ID() const							{ return GetInt(); }
		bool		Name(wxString &target) const		{ return AssignIfExist(EC_TAG_FRIEND_NAME, target); }
		bool		UserHash(CMD4Hash &target) const	{ return AssignIfExist(EC_TAG_FRIEND_HASH, target); }
		bool		IP(uint32 &target) const			{ return AssignIfExist(EC_TAG_FRIEND_IP, target); }
		bool		Port(uint16 &target) const			{ return AssignIfExist(EC_TAG_FRIEND_PORT, target); }
		bool		Client(uint32 &target) const		{ return AssignIfExist(EC_TAG_FRIEND_CLIENT, target); }
};

#endif /* ECSPECIALTAGS_H */
// File_checked_for_headers
