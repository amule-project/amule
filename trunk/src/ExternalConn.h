//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
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

#ifndef EXTERNALCONN_H
#define EXTERNALCONN_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ExternalConn.h"
#endif

#include <wx/thread.h>		// For ExitCode
#include <wx/event.h>		// For ExitCode

#include <map>
#include <set>
#include <list>
#include <vector>

#include "ECSocket.h"
#include "ECPacket.h"

#include "amuleIPV4Address.h"	// for amuleIPV4Address
#include "OtherFunctions.h"	// for RLE
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
		otherfunctions::PartFileEncoderData m_enc_data;
		
		// gaps are also RLE encoded, but list have variable size by it's nature.
		// so realloc buffer when needed.
		// This buffer only needed on core-side, where list is turned into array
		// before passing to RLE. Decoder will just use RLE internal buffer
		// Buffer can be static, since it is accessed with mutex locked
		typedef std::vector<uint32> GapBuffer;
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
		// decode - take data from tag
		void Decode(CECTag *tag);

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
		otherfunctions::RLE_Data m_enc_data;
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
		// decode - take data from tag
		void Decode(CECTag *tag);

		void ResetEncoder()
		{
			m_enc_data.ResetEncoder();
		}
};

typedef CFileEncoderMap<CKnownFile , CKnownFile_Encoder, CSharedFileList> CKnownFile_Encoder_Map;

#ifdef AMULE_DAEMON
#define EXTERNAL_CONN_BASE wxThread
#else
#define EXTERNAL_CONN_BASE wxEvtHandler
#endif

class ExternalConn : public EXTERNAL_CONN_BASE {
	public:
		ExternalConn(amuleIPV4Address addr, wxString *msg);
		~ExternalConn();
	
		CECPacket *ProcessRequest2(const CECPacket *request,
			CPartFile_Encoder_Map &, CKnownFile_Encoder_Map &);
	
		CECPacket *Authenticate(const CECPacket *);
		ECSocket *m_ECServer;

	private:
#ifdef AMULE_DAEMON
		void *Entry();
#else
		int m_numClients;
		//
		// encoder container must be created per EC client
		std::map<wxSocketBase *, CPartFile_Encoder_Map> m_part_encoders;
		std::map<wxSocketBase *, CKnownFile_Encoder_Map> m_shared_encoders;

		// event handlers (these functions should _not_ be virtual)
		void OnServerEvent(wxSocketEvent& event);
		void OnSocketEvent(wxSocketEvent& event);
		DECLARE_EVENT_TABLE()
#endif
};

class ExternalConnClientThread : public wxThread {
	public:
		ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock);
		~ExternalConnClientThread();
	
	private:
		ExitCode Entry();

		//
		// encoder container must be created per EC client
		CPartFile_Encoder_Map m_part_encoders;
		CKnownFile_Encoder_Map m_shared_encoders;
		
		ExternalConn *m_owner;
		wxSocketBase *m_sock;
};

#endif // EXTERNALCONN_H
