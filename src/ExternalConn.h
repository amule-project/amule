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

class CPartFile;
class wxSocketServer;
class wxSocketEvent;
class ExternalConnServerThread;

class ExternalConn {
	public:
		ExternalConn();
		~ExternalConn();
	
		wxString Authenticate(const wxString& item);
		wxString ProcessRequest(const wxString& item);
	
	private:
		wxString GetDownloadFileInfo(const CPartFile* file);
		ExternalConnServerThread *server;
};

//
// lfroen: need 2 threads here - 1 listening and 1 per client
//
class ExternalConnServerThread : public wxThread {
		wxSocketServer *m_ECServer;
		int m_numClients;
		ExternalConn *owner;
		
		void *Entry();
	public:
		ExternalConnServerThread(ExternalConn *owner);
		~ExternalConnServerThread();
		bool Ready()
		{
			return ((m_ECServer != 0) && (m_ECServer->Ok())) ? true : false;
		}
};

class ExternalConnClientThread : public wxThread {
		ExternalConn *owner;
		wxSocketBase *sock;
		
		void *Entry();
	public:
		ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock);
		~ExternalConnClientThread();
};

#endif // EXTERNALCONN_H
