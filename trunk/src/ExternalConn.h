// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef EXTERNALCONN_H
#define EXTERNALCONN_H

//Needed to avoid multiple instances of amule
#include <wx/ipc.h>	// Needed for wxConnection


//ExternalConn: listening server using wxSockets
#include <wx/socket.h>

class CPartFile;

class ExternalConn : public wxEvtHandler {
	public:
		ExternalConn();
		~ExternalConn();
	
		// event handlers (these functions should _not_ be virtual)
		void OnServerEvent(wxSocketEvent& event);
		void OnAuthEvent(wxSocketEvent& event);
		void OnSocketEvent(wxSocketEvent& event);
	
		wxString Authenticate(const wxString& item);
		wxString ProcessRequest(const wxString& item);
	
	private:
		wxString GetDownloadFileInfo(CPartFile* file);
	
		wxSocketServer *m_ECServer;
		int             m_numClients;
  	
	DECLARE_EVENT_TABLE()
};

#endif // EXTERNALCONN_H
