//
// This file is part of the aMule Project.
//
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

#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for HAVE_SYS_RESOURCE_H
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
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
#include <wx/stdpaths.h>

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "Server.h"			// Needed for GetListName
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "ServerConnect.h"		// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"		// Needed for CServerSocket
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "FriendList.h"			// Needed for CFriendList
#include "Packet.h"
#include "AICHSyncThread.h"
#include "Statistics.h"
#include "Logger.h"
#include <common/Format.h>
#include "InternalEvents.h"		// Needed for wxEVT_*

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifndef __WXMSW__
	#ifdef  HAVE_SYS_WAIT_H
		#include <sys/wait.h>
	#endif

	#include <wx/unix/execute.h>
#endif


BEGIN_EVENT_TABLE(CamuleDaemonApp, wxAppConsole)
	//
	// Socket handlers
	//
	
	// Listen Socket
	EVT_SOCKET(LISTENSOCKET_HANDLER, CamuleDaemonApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(SERVERUDPSOCKET_HANDLER, CamuleDaemonApp::UDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(CLIENTUDPSOCKET_HANDLER, CamuleDaemonApp::UDPSocketHandler)

	// Socket timers (TCP + UDO)
	EVT_MULE_TIMER(TM_TCPSOCKET, CamuleDaemonApp::OnTCPTimer)

	// Core timer
	EVT_MULE_TIMER(ID_CORETIMER, CamuleDaemonApp::OnCoreTimer)

	EVT_CUSTOM(wxEVT_MULE_NOTIFY_EVENT, -1, CamuleDaemonApp::OnNotifyEvent)

	// Async dns handling
	EVT_MULE_INTERNAL(wxEVT_CORE_UDP_DNS_DONE, -1, CamuleDaemonApp::OnUDPDnsDone)
	
	EVT_MULE_INTERNAL(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleDaemonApp::OnSourceDnsDone)

	EVT_MULE_INTERNAL(wxEVT_CORE_SERVER_DNS_DONE, -1, CamuleDaemonApp::OnServerDnsDone)

	// Hash ended notifier
	EVT_MULE_INTERNAL(wxEVT_CORE_FILE_HASHING_FINISHED, -1, CamuleDaemonApp::OnFinishedHashing)

	// Hashing thread finished and dead
	EVT_MULE_INTERNAL(wxEVT_CORE_FILE_HASHING_SHUTDOWN, -1, CamuleDaemonApp::OnHashingShutdown)

	// File completion ended notifier
	EVT_MULE_INTERNAL(wxEVT_CORE_FINISHED_FILE_COMPLETION, -1, CamuleDaemonApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_MULE_INTERNAL(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleDaemonApp::OnFinishedHTTPDownload)
END_EVENT_TABLE()

IMPLEMENT_APP(CamuleDaemonApp)

/*
 * Socket handling in wxBase
 * 
 */
class CSocketSet {
		int m_count;
		int m_fds[1024], m_fd_idx[1024];
		GSocket *m_gsocks[1024];

		fd_set m_set;
	public:
		CSocketSet();
		void AddSocket(GSocket *);
		void RemoveSocket(GSocket *);
		void FillSet(int &max_fd);
		
		void Detected(void (GSocket::*func)());
		
		fd_set *Set() { return &m_set; }
};

CSocketSet::CSocketSet()
{
	m_count = 0;
	for(int i = 0; i < 1024; i++) {
		m_fds[i] = 0;
		m_fd_idx[i] = 0xffff;
		m_gsocks[i] = 0;
	}
}

void CSocketSet::AddSocket(GSocket *socket)
{
	wxASSERT(socket);
	
	int fd = socket->m_fd;

	if ( fd == -1 ) {
		return;
	}

	wxASSERT( (fd > 2) && (fd < FD_SETSIZE) );
	
	if ( m_gsocks[fd] ) {
		return;
	}
	m_fds[m_count] = fd;
	m_fd_idx[fd] = m_count;
	m_gsocks[fd] = socket;
	m_count++;
}

void CSocketSet::RemoveSocket(GSocket *socket)
{
	wxASSERT(socket);
	
	int fd = socket->m_fd;

	if ( fd == -1 ) {
		return;
	}
	
	wxASSERT( (fd > 2) && (fd < FD_SETSIZE) );
	
	int i = m_fd_idx[fd];
	if ( i == 0xffff ) {
		return;
	}
	wxASSERT(m_fds[i] == fd);
	m_fds[i] = m_fds[m_count-1];
	m_gsocks[fd] = 0;
	m_fds[m_count-1] = 0;
	m_fd_idx[fd] = 0xffff;
	m_fd_idx[m_fds[i]] = i;
	m_count--;
}

void CSocketSet::FillSet(int &max_fd)
{
	FD_ZERO(&m_set);

	for(int i = 0; i < m_count; i++) {
	    FD_SET(m_fds[i], &m_set);
	    if ( m_fds[i] > max_fd ) {
	    	max_fd = m_fds[i];
	    }
	}
}

void CSocketSet::Detected(void (GSocket::*func)())
{
	for (int i = 0; i < m_count; i++) {
		int fd = m_fds[i];
		if ( FD_ISSET(fd, &m_set) ) {
			GSocket *socket = m_gsocks[fd];
			(*socket.*func)();
		}
	}
}

CAmuledGSocketFuncTable::CAmuledGSocketFuncTable() : m_lock(wxMUTEX_RECURSIVE)
{
	m_in_set = new CSocketSet;
	m_out_set = new CSocketSet;
	
	m_lock.Unlock();
}

void CAmuledGSocketFuncTable::AddSocket(GSocket *socket, GSocketEvent event)
{
	wxMutexLocker lock(m_lock);

	if ( event == GSOCK_INPUT ) {
		m_in_set->AddSocket(socket);
	} else {
		m_out_set->AddSocket(socket);
	}
}

void CAmuledGSocketFuncTable::RemoveSocket(GSocket *socket, GSocketEvent event)
{
	wxMutexLocker lock(m_lock);

	if ( event == GSOCK_INPUT ) {
		m_in_set->RemoveSocket(socket);
	} else {
		m_out_set->RemoveSocket(socket);
	}
}

void CAmuledGSocketFuncTable::RunSelect()
{
	wxMutexLocker lock(m_lock);

	int max_fd = -1;
	m_in_set->FillSet(max_fd);
	m_out_set->FillSet(max_fd);

    struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000; // 10ms
	
	int result = select(max_fd + 1, m_in_set->Set(), m_out_set->Set(), 0, &tv);
	if ( result > 0 ) {
		m_in_set->Detected(&GSocket::Detected_Read);
		m_out_set->Detected(&GSocket::Detected_Write);
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

bool CAmuledGSocketFuncTable::Init_Socket(GSocket *)
{
	return true;
}

void CAmuledGSocketFuncTable::Destroy_Socket(GSocket *)
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

CDaemonAppTraits::CDaemonAppTraits(CAmuledGSocketFuncTable *table) : m_lock(wxMUTEX_RECURSIVE)
{
	m_table = table;

	m_lock.Unlock();
}

void CDaemonAppTraits::ScheduleForDestroy(wxObject *object)
{
	wxMutexLocker lock(m_lock);

	//delete object;
	m_sched_delete.push_back(object);
}

void CDaemonAppTraits::RemoveFromPendingDelete(wxObject *object)
{
	wxMutexLocker lock(m_lock);

	for(std::list<wxObject *>::iterator i = m_sched_delete.begin();
		i != m_sched_delete.end(); i++) {
			if ( *i == object ) {
				m_sched_delete.erase(i);
				return;
			}
		}
}

void CDaemonAppTraits::DeletePending()
{
	wxMutexLocker lock(m_lock);

	while ( !m_sched_delete.empty() ) {
		std::list<wxObject *>::iterator i = m_sched_delete.begin();
		wxObject *object = *i;
		delete object;
	}
	//m_sched_delete.erase(m_sched_delete.begin(), m_sched_delete.end());
}


#ifndef __WXMSW__
int CDaemonAppTraits::WaitForChild(wxExecuteData& execData)
{
    if (execData.flags & wxEXEC_SYNC) {
	    int exitcode = 0;
    	if ( waitpid(execData.pid, &exitcode, 0) == -1 || !WIFEXITED(exitcode) ) {
        	wxLogSysError(_("Waiting for subprocess termination failed"));
	    }	

    	return exitcode;
	} else /** wxEXEC_ASYNC */ {
		// Give the process a chance to start or forked child to exit
		// 1 second is enough time to fail on "path not found"
		wxSleep(1);

		int status = 0, result = 0; 
		if ( (result = waitpid(execData.pid, &status, WNOHANG)) == -1) {
			printf("ERROR: waitpid call failed\n");
		} else if (status && WIFEXITED(status)) {
			return 0;
		}
		
		return execData.pid;
	}
}
#endif


#ifdef __WXMAC__
static wxStandardPathsCF gs_stdPaths;
wxStandardPathsBase& CDaemonAppTraits::GetStandardPaths()
{
	return gs_stdPaths;
}
#endif


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
	
	if (!thePrefs::AcceptExternalConnections()) {
		wxString warning = _("ERROR: aMule daemon cannot be used when external connections are disabled. "
			"To enable External Connections, use either a normal aMule or set the key"
			"\"AcceptExternalConnections\" to 1 in the file ~/.aMule/amule.conf");
		
		AddLogLineM(true, warning);
		printf("\n%s\n\n", (const char*)unicode2char(warning));
		return 0;
	} else if (thePrefs::ECPassword().IsEmpty()) {
		wxString warning = wxT("ERROR: A valid password is required to use "
			"external connections, and aMule daemon cannot be used without "
			"external connections. To run aMule deamon, you must set the "
			"\"ECPassword\" field in the file ~/.aMule/amule.conf with an "
			"appropriate value. More information can be found at "
			"http://wiki.amule.org");
	
		AddLogLineM(true, warning);
		printf("\n%s\n\n", (const char*)unicode2char(warning));
		return 0;
	}
	
	while ( !m_Exit ) {
		m_table->RunSelect();
		ProcessPendingEvents();
		((CDaemonAppTraits *)GetTraits())->DeletePending();
	}
	
	ShutDown();

	return 0;
}

bool CamuleDaemonApp::OnInit()
{
	printf("amuled: OnInit - starting timer\n");
	if ( !CamuleApp::OnInit() ) {
		return false;
	}
	core_timer = new CTimer(this,ID_CORETIMER);
	
	core_timer->Start(300);
	
	glob_prefs->GetCategory(0)->title = GetCatTitle(thePrefs::GetAllcatType());
	glob_prefs->GetCategory(0)->incomingpath = thePrefs::GetIncomingDir();
	
	return true;
}

int CamuleDaemonApp::InitGui(bool ,wxString &)
{
	#ifndef __WXMSW__
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
  	
	#endif
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

	delete core_timer;
	
	return CamuleApp::OnExit();
}


void CamuleDaemonApp::ShowAlert(wxString msg, wxString title, int flags)
{
	if ( flags | wxICON_ERROR ) {
		title = CFormat(_("ERROR: %s")) % title;
	}
	
	// Ensure that alerts are always visible on the console (when possible).
	if ((not enable_stdout_log) and (not enable_daemon_fork)) {
		puts((const char*)unicode2char(title + wxT(" ") + msg));
	}
	
	AddLogLine(title + wxT(" ") + msg);
}


void CamuleDaemonApp::NotifyEvent(const GUIEvent& event)
{
	switch (event.ID) {
		// GUI->CORE events
		// it's daemon, so gui isn't here, but macros can be used as function calls
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
