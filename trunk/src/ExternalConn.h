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

class ExternalConn : public EXTERNAL_CONN_BASE {
	public:
		ExternalConn();
		~ExternalConn();
	
		wxString ProcessRequest(const wxString& item);
		CECPacket *ProcessRequest2(const CECPacket *request);
	
	public:
		CECPacket *Authenticate(const CECPacket *);
		ECSocket *m_ECServer;

	private:
		wxString GetDownloadFileInfo(const CPartFile* file);
#ifdef AMULE_DAEMON
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

