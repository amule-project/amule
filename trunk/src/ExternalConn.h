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

#include "ECSocket.h"
#include "ECPacket.h"

class CPartFile;
class wxSocketServer;
class wxSocketEvent;

#ifdef AMULE_DAEMON
#define EXTERNAL_CONN_BASE wxThread
#else
#define EXTERNAL_CONN_BASE wxEvtHandler
#endif

/*!
 * PartStatus strings are quite long - RLE encoding will help.
 * 
 * Instead of sending each time full part-status string, send
 * RLE encoded difference from previous one.
 */
#define RLE_CONTROL_CHAR 0
class RLE_Data {
		unsigned char *m_buff, *m_enc_buff;
		int m_len, m_enc_len;
	public:
		RLE_Data(int len);
		~RLE_Data();
		
		CECTag *Encode(unsigned char *);
		const unsigned char *Decode(CECTag *);	
};

/*!
 * Container for encoder per CPartFile. Each EC client must have
 * instance of such encoder
 */
class RLE_Encoder {
		std::map<CPartFile *, RLE_String> m_encoders;
		int m_len;
	public:
		RLE_Encoder(int buff_len);
		
		CECTag *EncodePartStatus(CPartFile *file);
};

class ExternalConn : public EXTERNAL_CONN_BASE {
	public:
		ExternalConn();
		~ExternalConn();
	
		wxString ProcessRequest(const wxString& item);
		CECPacket *ProcessRequest2(const CECPacket *request);
	
		CECPacket *Authenticate(const CECPacket *);
		ECSocket *m_ECServer;

#ifdef AMULE_DAEMON
	private:
		void *Entry();
#else
	private:
		int m_numClients;

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

	private:
		ExternalConn *m_owner;
		wxSocketBase *m_sock;
};

#endif // EXTERNALCONN_H
