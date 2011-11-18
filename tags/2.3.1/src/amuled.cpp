//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "amule.h"			// Interface declarations.

#include <include/common/EventIDs.h>

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for HAVE_SYS_RESOURCE_H, etc
#endif

// Include the necessary headers for select(2), properly guarded
#if defined HAVE_SYS_SELECT_H && !defined __IRIX__
#	include <sys/select.h>
#else
#	ifdef HAVE_SYS_TIME_H
#		include <sys/time.h>
#	endif
#	ifdef HAVE_SYS_TYPES_H
#		include <sys/types.h>
#	endif
#	ifdef HAVE_UNISTD_H
#		include <unistd.h>
#	endif
#endif

#include <wx/utils.h>

#include "Preferences.h"		// Needed for CPreferences
#include "PartFile.h"			// Needed for CPartFile
#include "Logger.h"
#include <common/Format.h>
#include "InternalEvents.h"		// Needed for wxEVT_*
#include "ThreadTasks.h"
#include "GuiEvents.h"			// Needed for EVT_MULE_NOTIFY
#include "Timer.h"			// Needed for EVT_MULE_TIMER

#include "ClientUDPSocket.h"		// Do_not_auto_remove (forward declaration not enough)
#include "ListenSocket.h"		// Do_not_auto_remove (forward declaration not enough)


#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h> // Do_not_auto_remove
#endif

#ifndef __WXMSW__
	#ifdef  HAVE_SYS_WAIT_H
		#include <sys/wait.h> // Do_not_auto_remove 
	#endif

	#include <wx/unix/execute.h>
#endif

BEGIN_EVENT_TABLE(CamuleDaemonApp, wxAppConsole)
	//
	// Socket handlers
	//
	
	// Listen Socket
	EVT_SOCKET(ID_LISTENSOCKET_EVENT, CamuleDaemonApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(ID_SERVERUDPSOCKET_EVENT, CamuleDaemonApp::UDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(ID_CLIENTUDPSOCKET_EVENT, CamuleDaemonApp::UDPSocketHandler)

	// Socket timer (TCP)
	EVT_MULE_TIMER(ID_SERVER_RETRY_TIMER_EVENT, CamuleDaemonApp::OnTCPTimer)

	// Core timer
	EVT_MULE_TIMER(ID_CORE_TIMER_EVENT, CamuleDaemonApp::OnCoreTimer)

	EVT_MULE_NOTIFY(CamuleDaemonApp::OnNotifyEvent)

	// Async dns handling
	EVT_MULE_INTERNAL(wxEVT_CORE_UDP_DNS_DONE, -1, CamuleDaemonApp::OnUDPDnsDone)
	
	EVT_MULE_INTERNAL(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleDaemonApp::OnSourceDnsDone)

	EVT_MULE_INTERNAL(wxEVT_CORE_SERVER_DNS_DONE, -1, CamuleDaemonApp::OnServerDnsDone)

	// Hash ended notifier
	EVT_MULE_HASHING(CamuleDaemonApp::OnFinishedHashing)
	EVT_MULE_AICH_HASHING(CamuleDaemonApp::OnFinishedAICHHashing)
	
	// File completion ended notifier
	EVT_MULE_FILE_COMPLETED(CamuleDaemonApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_MULE_INTERNAL(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleDaemonApp::OnFinishedHTTPDownload)

	// Disk space preallocation finished
	EVT_MULE_ALLOC_FINISHED(CamuleDaemonApp::OnFinishedAllocation)
END_EVENT_TABLE()

IMPLEMENT_APP(CamuleDaemonApp)

#ifdef AMULED28
/*
 * Socket handling in wxBase
 * 
 */
class CSocketSet {
		int m_count;
		int m_fds[FD_SETSIZE], m_fd_idx[FD_SETSIZE];
		GSocket *m_gsocks[FD_SETSIZE];

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
	for(int i = 0; i < FD_SETSIZE; i++) {
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

#endif	// AMULED28

#ifndef __WXMSW__

#ifdef AMULED28

CDaemonAppTraits::CDaemonAppTraits(CAmuledGSocketFuncTable *table)
:
wxConsoleAppTraits(),
m_oldSignalChildAction(),
m_newSignalChildAction(),
m_table(table),
m_lock(wxMUTEX_RECURSIVE),
m_sched_delete()
{
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

wxAppTraits *CamuleDaemonApp::CreateTraits()
{
	return new CDaemonAppTraits(m_table);
}

#else	// AMULED28

CDaemonAppTraits::CDaemonAppTraits()
:
wxConsoleAppTraits(),
m_oldSignalChildAction(),
m_newSignalChildAction()
{
}

wxAppTraits *CamuleDaemonApp::CreateTraits()
{
	return new CDaemonAppTraits();
}

#endif	// !AMULED28

#endif	// __WXMSW__

#if defined(__WXMAC__) && !wxCHECK_VERSION(2, 9, 0)
#include <wx/stdpaths.h> // Do_not_auto_remove (guess)
static wxStandardPathsCF gs_stdPaths;
wxStandardPathsBase& CDaemonAppTraits::GetStandardPaths()
{
	return gs_stdPaths;
}
#endif


#ifdef AMULED28

CamuleDaemonApp::CamuleDaemonApp()
:
m_Exit(false),
m_table(new CAmuledGSocketFuncTable())
{
	wxPendingEventsLocker = new wxCriticalSection;
}

#endif	// !AMULED28


#ifndef __WXMSW__


static EndProcessDataMap endProcDataMap;


int CDaemonAppTraits::WaitForChild(wxExecuteData &execData)
{
	int status = 0;
	pid_t result = 0;
	// Build the log message
	wxString msg;
	msg << wxT("WaitForChild() has been called for child process with pid `") <<
		execData.pid <<
		wxT("'. ");

	if (execData.flags & wxEXEC_SYNC) {
		result = AmuleWaitPid(execData.pid, &status, 0, &msg);
		if (result == -1 || (!WIFEXITED(status) && !WIFSIGNALED(status))) {
			msg << wxT(" Waiting for subprocess termination failed.");
			AddDebugLogLineN(logGeneral, msg);
		}	
	} else {
		/** wxEXEC_ASYNC */
		// Give the process a chance to start or forked child to exit
		// 1 second is enough time to fail on "path not found"
		wxSleep(1);
		result = AmuleWaitPid(execData.pid, &status, WNOHANG, &msg);
		if (result == 0) {
			// Add a WxEndProcessData entry to the map, so that we can
			// support process termination
			wxEndProcessData *endProcData = new wxEndProcessData();
			endProcData->pid = execData.pid;
			endProcData->process = execData.process;
			endProcData->tag = 0;
			endProcDataMap[execData.pid] = endProcData;

			status = execData.pid;
		} else {
			// if result != 0, then either waitpid() failed (result == -1)
			// and there is nothing we can do, or the child has changed 
			// status, which means it is probably dead.
			status = 0;
		}
	}

	// Log our passage here
	AddDebugLogLineN(logGeneral, msg);

	return status;
}


void OnSignalChildHandler(int /*signal*/, siginfo_t *siginfo, void * /*ucontext*/)
{
	// Build the log message
	wxString msg;
	msg << wxT("OnSignalChildHandler() has been called for child process with pid `") <<
		siginfo->si_pid <<
		wxT("'. ");
	// Make sure we leave no zombies by calling waitpid()
	int status = 0;
	pid_t result = AmuleWaitPid(siginfo->si_pid, &status, WNOHANG, &msg);
	if (result != 1 && result != 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
		// Fetch the wxEndProcessData structure corresponding to this pid
		EndProcessDataMap::iterator it = endProcDataMap.find(siginfo->si_pid);
		if (it != endProcDataMap.end()) {
			wxEndProcessData *endProcData = it->second;
			// Remove this entry from the process map
			endProcDataMap.erase(siginfo->si_pid);
			// Save the exit code for the wxProcess object to read later
			endProcData->exitcode = result != -1 && WIFEXITED(status) ?
				WEXITSTATUS(status) : -1;
			// Make things work as in wxGUI
			wxHandleProcessTermination(endProcData);

			// wxHandleProcessTermination() will "delete endProcData;"
			// So we do not delete it again, ok? Do not uncomment this line.
			//delete endProcData;
		} else {
			msg << wxT(" Error: the child process pid is not on the pid map.");
		}
	}

	// Log our passage here
	AddDebugLogLineN(logGeneral, msg);
}


pid_t AmuleWaitPid(pid_t pid, int *status, int options, wxString *msg)
{
	*status = 0;
	pid_t result = waitpid(pid, status, options);
	if (result == -1) {
		*msg << CFormat(wxT("Error: waitpid() call failed: %m."));
	} else if (result == 0) {
		if (options & WNOHANG)  {
			*msg << wxT("The child is alive.");
		} else {
			*msg << wxT("Error: waitpid() call returned 0 but "
				"WNOHANG was not specified in options.");
		}
	} else {
		if (WIFEXITED(*status)) {
			*msg << wxT("Child has terminated with status code `") <<
				WEXITSTATUS(*status) <<
				wxT("'.");
		} else if (WIFSIGNALED(*status)) {
			*msg << wxT("Child was killed by signal `") <<
				WTERMSIG(*status) <<
				wxT("'.");
			if (WCOREDUMP(*status)) {
				*msg << wxT(" A core file has been dumped.");
			}
		} else if (WIFSTOPPED(*status)) {
			*msg << wxT("Child has been stopped by signal `") <<
				WSTOPSIG(*status) <<
				wxT("'.");
#ifdef WIFCONTINUED /* Only found in recent kernels. */
		} else if (WIFCONTINUED(*status)) {
			*msg << wxT("Child has received `SIGCONT' and has continued execution.");
#endif
		} else {
			*msg << wxT("The program was not able to determine why the child has signaled.");
		}
	}

	return result;
}


#endif // __WXMSW__


int CamuleDaemonApp::OnRun()
{
	if (!thePrefs::AcceptExternalConnections()) {
		AddLogLineCS(_("ERROR: aMule daemon cannot be used when external connections are disabled. To enable External Connections, use either a normal aMule, start amuled with the option --ec-config or set the key \"AcceptExternalConnections\" to 1 in the file ~/.aMule/amule.conf"));
		return 0;
	} else if (thePrefs::ECPassword().IsEmpty()) {
		AddLogLineCS(_("ERROR: A valid password is required to use external connections, and aMule daemon cannot be used without external connections. To run aMule deamon, you must set the \"ECPassword\" field in the file ~/.aMule/amule.conf with an appropriate value. Execute amuled with the flag --ec-config to set the password. More information can be found at http://wiki.amule.org"));
		return 0;
	}

#ifndef __WXMSW__
	// Process the return code of dead children so that we do not create 
	// zombies. wxBase does not implement wxProcess callbacks, so no one
	// actualy calls wxHandleProcessTermination() in console applications.
	// We do our best here.
	int ret = 0;
	ret = sigaction(SIGCHLD, NULL, &m_oldSignalChildAction);
	m_newSignalChildAction = m_oldSignalChildAction;
	m_newSignalChildAction.sa_sigaction = OnSignalChildHandler;
	m_newSignalChildAction.sa_flags |=  SA_SIGINFO;
	m_newSignalChildAction.sa_flags &= ~SA_RESETHAND;
	ret = sigaction(SIGCHLD, &m_newSignalChildAction, NULL);
	if (ret == -1) {
		AddDebugLogLineC(logStandard, CFormat(wxT("CamuleDaemonApp::OnRun(): Installation of SIGCHLD callback with sigaction() failed: %m.")));
	} else {
		AddDebugLogLineN(logGeneral, wxT("CamuleDaemonApp::OnRun(): Installation of SIGCHLD callback with sigaction() succeeded."));
	}
#endif // __WXMSW__
	
#ifdef AMULED28

	while ( !m_Exit ) {
		m_table->RunSelect();
		ProcessPendingEvents();
		((CDaemonAppTraits *)GetTraits())->DeletePending();
	}
	
	// ShutDown is beeing called twice. Once here and again in OnExit().
	ShutDown();

	return 0;

#else

#ifdef AMULED_DUMMY
	return 0;
#else
	return wxApp::OnRun();
#endif

#endif
}

bool CamuleDaemonApp::OnInit()
{
	if ( !CamuleApp::OnInit() ) {
		return false;
	}
	AddLogLineNS(_("amuled: OnInit - starting timer"));
	core_timer = new CTimer(this,ID_CORE_TIMER_EVENT);
	core_timer->Start(CORE_TIMER_PERIOD);
	glob_prefs->GetCategory(0)->title = GetCatTitle(thePrefs::GetAllcatFilter());
	glob_prefs->GetCategory(0)->path = thePrefs::GetIncomingDir();
	
	return true;
}

int CamuleDaemonApp::InitGui(bool ,wxString &)
{
#ifndef __WXMSW__
	if ( !enable_daemon_fork ) {
		return 0;
	}
	AddLogLineNS(_("amuled: forking to background - see you"));
	theLogger.SetEnabledStdoutLog(false);
	//
	// fork to background and detach from controlling tty
	// while redirecting stdout to /dev/null
	//
	for(int i_fd = 0;i_fd < 3; i_fd++) {
		close(i_fd);
	}
  	int fd = open("/dev/null",O_RDWR);
	if (dup(fd)){}	// prevent GCC warning
	if (dup(fd)){}
  	pid_t pid = fork();

	wxASSERT(pid != -1);

  	if ( pid ) {
  		exit(0);
  	} else {
		pid = setsid();
		//
		// Create a Pid file with the Pid of the Child, so any daemon-manager
		// can easily manage the process
		//
		if (!m_PidFile.IsEmpty()) {
			wxString temp = CFormat(wxT("%d\n")) % pid;
			wxFFile ff(m_PidFile, wxT("w"));
			if (!ff.Error()) {
				ff.Write(temp);
				ff.Close();
			} else {
				AddLogLineNS(_("Cannot Create Pid File"));
			}
		}
  	}
  	
#endif
	return 0;
}


int CamuleDaemonApp::OnExit()
{
#ifdef AMULED28
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
#endif

	ShutDown();

#ifndef __WXMSW__
	int ret = sigaction(SIGCHLD, &m_oldSignalChildAction, NULL);
	if (ret == -1) {
		AddDebugLogLineC(logStandard, CFormat(wxT("CamuleDaemonApp::OnRun(): second sigaction() failed: %m.")));
	} else {
		AddDebugLogLineN(logGeneral, wxT("CamuleDaemonApp::OnRun(): Uninstallation of SIGCHLD callback with sigaction() succeeded."));
	}
#endif // __WXMSW__
	
	// lfroen: delete socket threads
	if (ECServerHandler) {
		ECServerHandler = 0;
	}

	delete core_timer;
	
	return CamuleApp::OnExit();
}


int CamuleDaemonApp::ShowAlert(wxString msg, wxString title, int flags)
{
	if ( flags | wxICON_ERROR ) {
		title = CFormat(_("ERROR: %s")) % title;
	}
	AddLogLineCS(title + wxT(" ") + msg);

	return 0;	// That's neither yes nor no, ok, cancel
}

// File_checked_for_headers
