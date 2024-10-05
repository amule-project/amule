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

#include "config.h"			// Needed for HAVE_SYS_RESOURCE_H, etc


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

#ifndef __WINDOWS__
	#ifdef  HAVE_SYS_WAIT_H
		#include <sys/wait.h> // Do_not_auto_remove
	#endif
	#include <wx/ffile.h>
#endif

BEGIN_EVENT_TABLE(CamuleDaemonApp, wxAppConsole)

#ifndef ASIO_SOCKETS
	//
	// Socket handlers
	//

	// Listen Socket
	EVT_SOCKET(ID_LISTENSOCKET_EVENT, CamuleDaemonApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(ID_SERVERUDPSOCKET_EVENT, CamuleDaemonApp::UDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(ID_CLIENTUDPSOCKET_EVENT, CamuleDaemonApp::UDPSocketHandler)
#endif

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

#ifdef __WINDOWS__
//
// CTRL-C-Handler
// see http://msdn.microsoft.com/en-us/library/windows/desktop/ms685049%28v=vs.85%29.aspx
//
static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType) {
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_BREAK_EVENT:
			// handle these
			AddLogLineNS(wxT("Received break event, exit main loop"));
			theApp->ExitMainLoop();
			return TRUE;
			break;
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		default:
			// don't handle these
			return FALSE;
			break;
	}
}

#endif // __WINDOWS__


int CamuleDaemonApp::OnRun()
{
	if (!thePrefs::AcceptExternalConnections()) {
		AddLogLineCS(_("ERROR: aMule daemon cannot be used when external connections are disabled. To enable External Connections, use either a normal aMule, start amuled with the option --ec-config or set the key \"AcceptExternalConnections\" to 1 in the file ~/.aMule/amule.conf"));
		return 0;
	} else if (thePrefs::ECPassword().IsEmpty()) {
		AddLogLineCS(_("ERROR: A valid password is required to use external connections, and aMule daemon cannot be used without external connections. To run aMule daemon, you must set the \"ECPassword\" field in the file ~/.aMule/amule.conf with an appropriate value. Execute amuled with the flag --ec-config to set the password. More information can be found at http://wiki.amule.org"));
		return 0;
	}

#ifdef __WINDOWS__
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE);
#endif // __WINDOWS__

	return wxApp::OnRun();
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
#ifndef __WINDOWS__
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

bool CamuleDaemonApp::Initialize(int& argc_, wxChar **argv_)
{
	if ( !wxAppConsole::Initialize(argc_, argv_) ) {
		return false;
	}

#ifdef __UNIX__
	wxString encName;
#if wxUSE_INTL
	// if a non default locale is set,
	// assume that the user wants his
        // filenames in this locale too
        encName = wxLocale::GetSystemEncodingName().Upper();

        // But don't consider ASCII in this case.
	if ( !encName.empty() ) {
		if ( encName == wxT("US-ASCII") ) {
			// This means US-ASCII when returned
			// from GetEncodingFromName().
			encName.clear();
		}
        }
#endif // wxUSE_INTL

	// in this case, UTF-8 is used by default.
        if ( encName.empty() ) {
		encName = wxT("UTF-8");
	}

	static wxConvBrokenFileNames fileconv(encName);
	wxConvFileName = &fileconv;
#endif // __UNIX__

	return true;
}

int CamuleDaemonApp::OnExit()
{
	ShutDown();
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
