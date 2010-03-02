//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 Kry ( elkry@users.sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008 Froenchenko Leonid (lfroen@gmail.com)
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

//
// T - type of item
// E - type of encoder
// C - type of container in theApp
template <class T, class E, class C>
class CFileEncoderMap : public std::map<T *, E> {
	public:
		void UpdateEncoders(C *container)
		{
			// check if encoder contains files that no longer in container
			// or, we have new files without encoder yet
			std::set<T *> curr_files, dead_files;
			for (unsigned int i = 0; i < container->GetFileCount(); i++) {
				// cast for case of 'const'
				T *cur_file = (T *)container->GetFileByIndex(i);
				curr_files.insert(cur_file);
				if ( this->count(cur_file) == 0 ) {
					this->operator [](cur_file) = E(cur_file);
				}
			}
			//
			// curr_files set is created to minimize lookup time in download queue,
			// since GetFileByID have loop inside leading to O(n), in this case
			// it will mean O(n^2)
			typename std::map<T *, E>::iterator i;
			for(i = this->begin(); i != this->end(); i++) {
				if ( curr_files.count(i->first) == 0 ) {
					dead_files.insert(i->first);
				}
			}
			typename std::set<T *>::iterator j;
			for(j = dead_files.begin(); j != dead_files.end(); j++) {
				this->erase(*j);
			}
		}
};

/*!
 * PartStatus strings and gap lists are quite long - RLE encoding will help.
 * 
 * Instead of sending each time full part-status string, send
 * RLE encoded difference from previous one.
 *
 * PartFileEncoderData class is used for decode only,
 * while CPartFile_Encoder is used for encode only.
 */
class CPartFile_Encoder : public PartFileEncoderData {
		CPartFile *m_file;
		SourcenameItemMap m_sourcenameItemMap;
		int m_sourcenameID;
	public:
		// encoder side
		CPartFile_Encoder(CPartFile *file = 0) { m_file = file; m_sourcenameID = 0; }
		
		// encode - take data from m_file
		void Encode(CECTag *parent_tag);

		// Encoder may reset history if full info requested
		void ResetEncoder();
};

typedef CFileEncoderMap<CPartFile , CPartFile_Encoder, CDownloadQueue> CPartFile_Encoder_Map;

/*
 * Encode 'obtained parts' info to be sent to remote gui
 */
class CKnownFile_Encoder {
		RLE_Data m_enc_data;
		CKnownFile *m_file;
	public:
		CKnownFile_Encoder(CKnownFile *file = 0) { m_file = file; }

		// encode - take data from m_file
		void Encode(CECTag *parent_tag);

		void ResetEncoder()
		{
			m_enc_data.ResetEncoder();
		}
};

typedef CFileEncoderMap<CKnownFile , CKnownFile_Encoder, CSharedFileList> CKnownFile_Encoder_Map;

template <class T, ec_tagname_t OP>
class CTagSet : public std::set<T> {
		void InSet(const CECTag *tag, uint32)
		{
			this->insert(tag->GetInt());
		}
		void InSet(const CECTag *tag, CMD4Hash)
		{
			this->insert(tag->GetMD4Data());
		}
	public:
		CTagSet(const CECPacket *request) : std::set<T>()
		{
			for (unsigned int i = 0;i < request->GetTagCount();i++) {
				const CECTag *tag = request->GetTagByIndex(i);
				if ( tag->GetTagName() == OP ) {
					this->InSet(tag, T());
				}
			}
		}
};


class CObjTagMap {
		std::map<void *, CValueMap> m_obj_map;
	public:
		CValueMap &GetValueMap(void *object)
		{
			return m_obj_map[object];
		}
		
		size_t size()
		{
			return m_obj_map.size();
		}
		
		void RemoveDeleted(std::set<void *>& WXUNUSED(current_set))
		{/*
			for(std::map<void *, CValueMap>::iterator i = m_obj_map.begin(); i != m_obj_map.end(); i++) {
				if ( !current_set.count(i->first) ) {
					m_obj_map.erase(i->first);
				}
			}
			*/
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
		
		void Status_ConnectionState();
		void Status_QueueCount();
		void Status_UserCount();
		
		void SharedFile_AddFile(CKnownFile *file);
		void SharedFile_RemoveFile(CKnownFile *file);
		void SharedFile_RemoveAllFiles();

};


#endif // EXTERNALCONN_H
// File_checked_for_headers
