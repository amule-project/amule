//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 Kry ( elkry@users.sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/thread.h>		// For ExitCode
#include <wx/event.h>		// For ExitCode

#include <map>
#include <set>
#include <list>
#include <vector>

#include <ec/ECSpecialTags.h>

#include "amuleIPV4Address.h"	// for amuleIPV4Address
#include "RLE.h"	// for RLE
#include "DownloadQueue.h"
#include "SharedFileList.h"

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
 * PartStatus strings are quite long - RLE encoding will help.
 * 
 * Instead of sending each time full part-status string, send
 * RLE encoded difference from previous one.
 * 
 * However, gap status is different - it's already kind of RLE
 * encoding, so futher compression will help a litter (Shannon
 * theorem). Instead, calculate diff between list of gaps.
 */
class CPartFile_Encoder {
		//
		// List of gaps sent to particular client. Since clients
		// can request lists in different time, they can get
		// different results
		PartFileEncoderData m_enc_data;
		
		// gaps are also RLE encoded, but list have variable size by it's nature.
		// so realloc buffer when needed.
		// This buffer only needed on core-side, where list is turned into array
		// before passing to RLE. Decoder will just use RLE internal buffer
		// Buffer can be static, since it is accessed with mutex locked
		typedef std::vector<uint64> GapBuffer;
		static GapBuffer m_gap_buffer;
		
		CPartFile *m_file;
	public:
		// encoder side
		CPartFile_Encoder(CPartFile *file);
		
		// decoder side
		CPartFile_Encoder(int size);

		~CPartFile_Encoder();
		
		// stl side :)
		CPartFile_Encoder();
		
		CPartFile_Encoder(const CPartFile_Encoder &obj);

		CPartFile_Encoder &operator=(const CPartFile_Encoder &obj);
		
		// encode - take data from m_file
		void Encode(CECTag *parent_tag);
		
		void ResetEncoder()
		{
			m_enc_data.ResetEncoder();
		}
};

typedef CFileEncoderMap<CPartFile , CPartFile_Encoder, CDownloadQueue> CPartFile_Encoder_Map;

/*
 * Encode 'obtained parts' info to be sent to remote gui
 */
class CKnownFile_Encoder {
		RLE_Data m_enc_data;
		CKnownFile *m_file;
	public:
		CKnownFile_Encoder(CKnownFile *file);
		~CKnownFile_Encoder();

		// stl side :)
		CKnownFile_Encoder();
		
		CKnownFile_Encoder(const CKnownFile_Encoder &obj);

		CKnownFile_Encoder &operator=(const CKnownFile_Encoder &obj);
		// encode - take data from m_file
		void Encode(CECTag *parent_tag);

		void ResetEncoder()
		{
			m_enc_data.ResetEncoder();
		}
};

typedef CFileEncoderMap<CKnownFile , CKnownFile_Encoder, CSharedFileList> CKnownFile_Encoder_Map;

template <class T, ec_opcode_t OP>
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
			for (int i = 0;i < request->GetTagCount();i++) {
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

class ExternalConn : public wxEvtHandler {
	public:
		ExternalConn(amuleIPV4Address addr, wxString *msg);
		~ExternalConn();
	
		static CECPacket *ProcessRequest2(const CECPacket *request,
			CPartFile_Encoder_Map &, CKnownFile_Encoder_Map &, CObjTagMap &);
	
		static CECPacket *Authenticate(const CECPacket *);
		wxSocketServer *m_ECServer;

	private:
		// event handlers (these functions should _not_ be virtual)
		void OnServerEvent(wxSocketEvent& event);
		DECLARE_EVENT_TABLE()
};

#endif // EXTERNALCONN_H
