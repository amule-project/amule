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
	
		wxString Authenticate(const wxString& item);
		wxString ProcessRequest(const wxString& item);
	
		void Read(wxSocketBase *sock, wxString& s);
		void Write(wxSocketBase *sock, const wxString& s);
	
	private:
		wxString GetDownloadFileInfo(const CPartFile* file);
		
		
#ifdef AMULE_DAEMON
		void *Entry();
#else
		// event handlers (these functions should _not_ be virtual)
		DECLARE_EVENT_TABLE()
		void OnServerEvent(wxSocketEvent& event);
		void OnSocketEvent(wxSocketEvent& event);
#endif
		wxSocketServer *m_ECServer;
		int m_numClients;
};

class ExternalConnClientThread : public wxThread {
	public:
		ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock);
		~ExternalConnClientThread();
	
	private:
		ExitCode Entry();
		

		ExternalConn *m_owner;
		wxSocketBase *m_sock;
};

#endif // EXTERNALCONN_H

