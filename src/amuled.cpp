//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for HAVE_SYS_RESOURCE_H
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>                // Needed for wxFileName::GetPathSeparator()
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/utils.h>
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/textfile.h>		// Needed for wxTextFile
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/url.h>

#include "amule.h"				// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "Server.h"				// Needed for GetListName
#include "CFile.h"				// Needed for CFile
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "ServerConnect.h"			// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"	// Needed for CServerSocket
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "PartFile.h"		// Needed for CPartFile
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "FriendList.h"	// Needed for CFriendList
#include "Packet.h"
#include "AICHSyncThread.h"
#include "Statistics.h"
#include "Logger.h"
#include "Format.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


BEGIN_EVENT_TABLE(CamuleDaemonApp, wxAppConsole)
	//
	// Socket handlers
	//
	
	// Listen Socket
	EVT_SOCKET(LISTENSOCKET_HANDLER, CamuleDaemonApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(SERVERUDPSOCKET_HANDLER, CamuleDaemonApp::ServerUDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(CLIENTUDPSOCKET_HANDLER, CamuleDaemonApp::ClientUDPSocketHandler)

	// Socket timers (TCP + UDO)
	EVT_CUSTOM(wxEVT_AMULE_TIMER, TM_TCPSOCKET, CamuleDaemonApp::OnTCPTimer)

	// Core timer
	EVT_CUSTOM(wxEVT_AMULE_TIMER, ID_CORETIMER, CamuleDaemonApp::OnCoreTimer)

	EVT_CUSTOM(wxEVT_NOTIFY_EVENT, -1, CamuleDaemonApp::OnNotifyEvent)

	// Async dns handling
	EVT_CUSTOM(wxEVT_CORE_UDP_DNS_DONE, -1, CamuleDaemonApp::OnUDPDnsDone)
	
	EVT_CUSTOM(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleDaemonApp::OnSourceDnsDone)

	EVT_CUSTOM(wxEVT_CORE_SERVER_DNS_DONE, -1, CamuleDaemonApp::OnServerDnsDone)

	// Hash ended notifier
	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_FINISHED, -1, CamuleDaemonApp::OnFinishedHashing)

	// Hashing thread finished and dead
	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_SHUTDOWN, -1, CamuleDaemonApp::OnHashingShutdown)

	// File completion ended notifier
	EVT_CUSTOM(wxEVT_CORE_FINISHED_FILE_COMPLETION, -1, CamuleDaemonApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_CUSTOM(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleDaemonApp::OnFinishedHTTPDownload)
END_EVENT_TABLE()

IMPLEMENT_APP(CamuleDaemonApp)

/*
 * Socket handling in wxBase
 * 
 */
CAmuledGSocketFuncTable::CAmuledGSocketFuncTable()
{
	m_in_fds_count = m_out_fds_count = 0;
}

void CAmuledGSocketFuncTable::AddSocket(GSocket *socket, GSocketEvent event)
{
	wxASSERT(socket);
	
	int fd = socket->m_fd;

	if ( fd == -1 ) {
		return;
	}
	if ( event == GSOCK_INPUT ) {
		m_in_fds[m_in_fds_count++] = fd;
		m_in_gsocks[fd] = socket;
	} else {
		m_out_fds[m_out_fds_count++] = fd;
		m_out_gsocks[fd] = socket;
	}
}

void CAmuledGSocketFuncTable::RemoveSocket(GSocket *socket, GSocketEvent event)
{
	wxASSERT(socket);
	
	int fd = socket->m_fd;
	
	if ( fd == -1 ) {
		return;
	}
	
	if ( event == GSOCK_INPUT ) {
		for(int i = 0; i < m_in_fds_count; i++) {
			if ( m_in_fds[i] == fd ) {
				m_in_fds[i] = m_in_fds[m_in_fds_count-1];
				m_in_gsocks[fd] = 0;
				m_in_fds_count--;
				break;
			}
		}
	} else {
		for(int i = 0; i < m_out_fds_count; i++) {
			if ( m_out_fds[i] == fd ) {
				m_out_fds[i] = m_out_fds[m_out_fds_count-1];
				m_out_gsocks[fd] = 0;
				m_out_fds_count--;
				break;
			}
		}
	}
}

void CAmuledGSocketFuncTable::RunSelect()
{
	FD_ZERO(&m_readset);
	FD_ZERO(&m_writeset);

	int max_fd = -1;
	for(int i = 0; i < m_in_fds_count; i++) {
	    FD_SET(m_in_fds[i], &m_readset);
	    if ( m_in_fds[i] > max_fd ) {
	    	max_fd = m_in_fds[i];
	    }
	}
	for(int i = 0; i < m_out_fds_count; i++) {
	    FD_SET(m_out_fds[i], &m_writeset);
	    if ( m_out_fds[i] > max_fd ) {
	    	max_fd = m_out_fds[i];
	    }
	}

    struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000; // 10ms
	
	int result = select(max_fd + 1, &m_readset, &m_writeset, 0, &tv);
	if ( result > 0 ) {
		for (int i = 0; i < m_in_fds_count; i++) {
			int fd = m_in_fds[i];
			if ( FD_ISSET(fd, &m_readset) ) {
				GSocket *socket = m_in_gsocks[fd];
				if (socket) {
					socket->Detected_Read();
				}
			}
		}
		for (int i = 0; i < m_out_fds_count; i++) {
			int fd = m_out_fds[i];
			if ( FD_ISSET(fd, &m_writeset) ) {
				GSocket *socket = m_out_gsocks[fd];
				if (socket) {
					socket->Detected_Write();
				}
			}
		}
	}
	
}

GSocketGUIFunctionsTable *CDaemonAppTraits::GetSocketGUIFunctionsTable()
{
	return m_table;
}

bool CAmuledGSocketFuncTable::OnInit()
{
	return true;
}

void CAmuledGSocketFuncTable::OnExit()
{
}

bool CAmuledGSocketFuncTable::CanUseEventLoop()
{
	/*
	 * FIXME: (lfroen) Not sure whether it's right.
	 * I will review it later.
	 */
	return false;
}

bool CAmuledGSocketFuncTable::Init_Socket(GSocket *socket)
{
	return true;
}

void CAmuledGSocketFuncTable::Destroy_Socket(GSocket *socket)
{
}

void CAmuledGSocketFuncTable::Install_Callback(GSocket *sock, GSocketEvent e)
{
	AddSocket(sock, e);
}

void CAmuledGSocketFuncTable::Uninstall_Callback(GSocket *sock, GSocketEvent e)
{
	RemoveSocket(sock, e);
}

void CAmuledGSocketFuncTable::Enable_Events(GSocket *socket)
{
	Install_Callback(socket, GSOCK_INPUT);
	Install_Callback(socket, GSOCK_OUTPUT);
}

void CAmuledGSocketFuncTable::Disable_Events(GSocket *socket)
{
	Uninstall_Callback(socket, GSOCK_INPUT);
	Uninstall_Callback(socket, GSOCK_OUTPUT);
}

CDaemonAppTraits::CDaemonAppTraits(CAmuledGSocketFuncTable *table)
{
	m_table = table;
}

void CDaemonAppTraits::ScheduleForDestroy(wxObject *object)
{
	//
	// FIXME: this only present in gtk core lib. Must do myself
	//
	/*
	if ( !wxPendingDelete.Member(object) ) {
	    wxPendingDelete.Append(object);
	}
	*/
	delete object;
}

void CDaemonAppTraits::RemoveFromPendingDelete(wxObject *object)
{
}

CamuleDaemonApp::CamuleDaemonApp()
{
	wxPendingEventsLocker = new wxCriticalSection;

	m_table = new CAmuledGSocketFuncTable();
	
	m_Exit = false;
}

wxAppTraits *CamuleDaemonApp::CreateTraits()
{
	return new CDaemonAppTraits(m_table);
}

int CamuleDaemonApp::OnRun()
{
	AddDebugLogLineM( true, logGeneral, wxT("CamuleDaemonApp::OnRun()"));
	
	if ( !thePrefs::AcceptExternalConnections() ) {
		wxString warning = _("ERROR: amule daemon is useless when external connections disabled. "
			"Change configuration either from GUI or by editing the config file");
		AddLogLineM(true, warning);
		// Also to console.
		printf((const char*)unicode2char(warning + wxT("\n")));
		return 0;
	}
	
	while ( !m_Exit ) {
		m_table->RunSelect();
		ProcessPendingEvents();
	}

	return 0;
}

bool CamuleDaemonApp::OnInit()
{
	printf("amuled: OnInit - starting timer\n");
	if ( !CamuleApp::OnInit() ) {
		return false;
	}
	core_timer = new CTimer(this,ID_CORETIMER);
	
	core_timer->Start(100);
	
	return true;
}

int CamuleDaemonApp::InitGui(bool ,wxString &)
{
	if ( !enable_daemon_fork ) {
		return 0;
	}
	printf("amuled: forking to background - see you\n");
	//
	// fork to background and detouch from controlling tty
	// while redirecting stdout to /dev/null
	//
	for(int i_fd = 0;i_fd < 3; i_fd++) {
		close(i_fd);
	}
  	int fd = open("/dev/null",O_RDWR);
  	dup(fd);
  	dup(fd);
  	int pid = fork();

	wxASSERT(pid != -1);

  	if ( pid ) {
  		exit(0);
  	} else {
		setsid();
  	}
  	
	return 0;
}


int CamuleDaemonApp::OnExit()
{
	/*
	 * Stop all socket threads before entering
	 * shutdown sequence.
	 */
	delete listensocket;
	listensocket = 0;
	if (clientudp) {
		delete clientudp;
		clientudp = NULL;
	}
	
	ShutDown();
	
	// lfroen: delete socket threads
	if (ECServerHandler) {
		ECServerHandler = 0;
	}
	core_timer->Stop();
	
	return CamuleApp::OnExit();
}

void CamuleDaemonApp::ShowAlert(wxString msg, wxString title, int flags)
{
	if ( flags | wxICON_ERROR ) {
		title = CFormat(_("ERROR: %s")) % title;
	}
	AddLogLine(title + wxT(" ") + msg);
}


void CamuleDaemonApp::NotifyEvent(const GUIEvent& event)
{
	switch (event.ID) {
		// GUI->CORE events
		// it's daemon, so gui isn't here, but macros can be used as function calls
		case SHOW_CONN_STATE:
			if ( event.byte_value ) {
				const wxString id = theApp.serverconnect->IsLowID() ? _("with LowID") : _("with HighID");
				AddLogLine(CFormat(_("Connected to %s %s")) % event.string_value % id);
			} else {
				if ( theApp.serverconnect->IsConnecting() ) {
					AddLogLine(CFormat(_("Connecting to %s")) % event.string_value);
				} else {
					AddLogLine(_("Disconnected"));
				}
			}
			break;
		case SEARCH_ADD_TO_DLOAD:
			downloadqueue->AddSearchToDownload((CSearchFile *)event.ptr_value, event.byte_value);
			break;

		case SHAREDFILES_SHOW_ITEM:
			//printf("SHAREDFILES_SHOW_ITEM: %p\n", event.ptr_value);
			break;
			
		case DOWNLOAD_CTRL_ADD_SOURCE:
		/*
		printf("ADD_SOURCE: adding source %p to partfile %s\n",
		       event.ptr_aux_value, ((CPartFile*)event.ptr_value)->GetFullName().c_str());
		*/
			break;

		case ADDLOGLINE:
			AddLogLine(event.string_value);
			break;
		case ADDDEBUGLOGLINE:
			//printf("DEBUGLOG: %s\n", event.string_value.c_str());
			break;
		default:
			//printf("WARNING: event %d in daemon should not happen\n", event.ID);
			break;
	}
}
