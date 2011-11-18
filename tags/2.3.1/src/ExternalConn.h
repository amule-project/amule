//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Kry ( elkry@users.sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Froenchenko Leonid (lfroen@gmail.com)
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

#ifndef EXTERNALCONN_H
#define EXTERNALCONN_H



#include <ec/cpp/ECSpecialTags.h>

#include "amuleIPV4Address.h"	// for amuleIPV4Address
#include "RLE.h"	// for RLE
#include "DownloadQueue.h"
#include "PartFile.h"			// for SourcenameItemMap

class wxSocketServer;
class wxSocketEvent;

template <class T, ec_tagname_t OP>
class CTagSet : public std::set<T> {
		void InSet(const CECTag *tag, uint32)
		{
			this->insert(tag->GetInt());		// don't remove this->
		}
		void InSet(const CECTag *tag, const CMD4Hash&)
		{
			this->insert(tag->GetMD4Data());	// don't remove this->
		}
	public:
		CTagSet(const CECPacket *request) : std::set<T>()
		{
			for (CECPacket::const_iterator it = request->begin(); it != request->end(); it++) {
				const CECTag *tag = & *it;
				if ( tag->GetTagName() == OP ) {
					InSet(tag, T());
				}
			}
		}
};


class CObjTagMap {
		std::map<uint32, CValueMap> m_obj_map;
	public:
		CValueMap &GetValueMap(uint32 ECID)
		{
			return m_obj_map[ECID];
		}
		
		size_t size()
		{
			return m_obj_map.size();
		}
};


class CECServerSocket;
class ECNotifier;

class ExternalConn : public wxEvtHandler
{
private:
	typedef std::set<CECServerSocket *> SocketSet;
	SocketSet socket_list;

public:
	ExternalConn(amuleIPV4Address addr, wxString *msg);
	~ExternalConn();
	
	wxSocketServer *m_ECServer;
	ECNotifier *m_ec_notifier;

	void AddSocket(CECServerSocket *s);
	void RemoveSocket(CECServerSocket *s);
	void KillAllSockets();
	void ResetAllLogs();

private:
	// event handlers (these functions should _not_ be virtual)
	void OnServerEvent(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
};

class ECUpdateMsgSource {
	public:
		virtual ~ECUpdateMsgSource()
		{
		}
		virtual CECPacket *GetNextPacket() = 0;
};

class ECPartFileMsgSource : public ECUpdateMsgSource {
		typedef struct {
			bool m_new;
			bool m_comment_changed;
			bool m_removed;
			bool m_finished;
			bool m_dirty;
			CPartFile *m_file;
		} PARTFILE_STATUS;
		std::map<CMD4Hash, PARTFILE_STATUS> m_dirty_status;
	public:
		ECPartFileMsgSource();
		
		void SetDirty(CPartFile *file);
		void SetNew(CPartFile *file);
		void SetCompleted(CPartFile *file);
		void SetRemoved(CPartFile *file);
		
		virtual CECPacket *GetNextPacket();
	
};

class ECKnownFileMsgSource : public ECUpdateMsgSource {
		typedef struct {
			bool m_new;
			bool m_comment_changed;
			bool m_removed;
			bool m_dirty;
			CKnownFile *m_file;
		} KNOWNFILE_STATUS;
		std::map<CMD4Hash, KNOWNFILE_STATUS> m_dirty_status;
	public:
		ECKnownFileMsgSource();

		void SetDirty(CKnownFile *file);
		void SetNew(CKnownFile *file);
		void SetRemoved(CKnownFile *file);
		
		virtual CECPacket *GetNextPacket();
};

class ECClientMsgSource : public ECUpdateMsgSource {
	public:
		virtual CECPacket *GetNextPacket();
};

class ECStatusMsgSource : public ECUpdateMsgSource {
		uint32 m_last_ed2k_status_sent;
		uint32 m_last_kad_status_sent;
		void *m_server;

		uint32 GetEd2kStatus();
		uint32 GetKadStatus();
	public:
		ECStatusMsgSource();
		
		virtual CECPacket *GetNextPacket();
};

class ECSearchMsgSource : public ECUpdateMsgSource {
		typedef struct {
			bool m_new;
			bool m_child_dirty;
			bool m_dirty;
			CSearchFile *m_file;
		} SEARCHFILE_STATUS;
		std::map<CMD4Hash, SEARCHFILE_STATUS> m_dirty_status;
	public:
		ECSearchMsgSource();
		
		void SetDirty(CSearchFile *file);
		void SetChildDirty(CSearchFile *file);
	
		void FlushStatus();
	
		virtual CECPacket *GetNextPacket();
};

class ECNotifier {
		//
		// designated priority for each type of update
		//
		enum EC_SOURCE_PRIO {
			EC_PARTFILE = 0,
			EC_SEARCH,
			EC_CLIENT,
			EC_STATUS,
			EC_KNOWN,
			
			EC_STATUS_LAST_PRIO
		};
		
		//ECUpdateMsgSource *m_msg_source[EC_STATUS_LAST_PRIO];
		std::map<CECServerSocket *, ECUpdateMsgSource **> m_msg_source;
		
		void NextPacketToSocket();
		
		CECPacket *GetNextPacket(ECUpdateMsgSource *msg_source_array[]);
		// Make class non assignable
		void operator=(const ECNotifier&);
		ECNotifier(const ECNotifier&);
	public:
		ECNotifier();
		~ECNotifier();
		
		void Add_EC_Client(CECServerSocket *sock);
		void Remove_EC_Client(CECServerSocket *sock);
		
		CECPacket *GetNextPacket(CECServerSocket *sock);
		
		//
		// Interface to notification macros
		//
		void DownloadFile_SetDirty(CPartFile *file);
		void DownloadFile_RemoveFile(CPartFile *file);
		void DownloadFile_RemoveSource(CPartFile *file);
		void DownloadFile_AddFile(CPartFile *file);
		void DownloadFile_AddSource(CPartFile *file);
		
		void SharedFile_AddFile(CKnownFile *file);
		void SharedFile_RemoveFile(CKnownFile *file);
		void SharedFile_RemoveAllFiles();

};


#endif // EXTERNALCONN_H
// File_checked_for_headers
