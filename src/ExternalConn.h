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

#include <wx/thread.h>		// For ExitCode
#include <wx/event.h>		// For ExitCode

#include <map>
#include <list>

#include "ECSocket.h"
#include "ECPacket.h"

#include "otherfunctions.h" // for RLE

class CPartFile;
class wxSocketServer;
class wxSocketEvent;

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
		static uint32 *m_gap_buffer;
		static int m_gap_buffer_size;
		
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

typedef std::map<CPartFile *, CPartFile_Encoder> CPartFile_Encoder_Map;

#ifdef AMULE_DAEMON
#define EXTERNAL_CONN_BASE wxThread
#else
#define EXTERNAL_CONN_BASE wxEvtHandler
#endif

class ExternalConn : public EXTERNAL_CONN_BASE {
	public:
		ExternalConn();
		~ExternalConn();
	
		wxString ProcessRequest(const wxString& item);
		CECPacket *ProcessRequest2(const CECPacket *request, CPartFile_Encoder_Map &enc_map);
	
		CECPacket *Authenticate(const CECPacket *);
		ECSocket *m_ECServer;

	private:
#ifdef AMULE_DAEMON
		void *Entry();
#else
		int m_numClients;
		//
		// encoder container must be created per EC client
		std::map<wxSocketBase *, CPartFile_Encoder_Map> m_encoders;

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
		CPartFile_Encoder_Map m_encoders;

		ExternalConn *m_owner;
		wxSocketBase *m_sock;
};

#endif // EXTERNALCONN_H
