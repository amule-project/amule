//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <csignal>
#include <cstring>
#include <wx/process.h>
#include <wx/sstream.h>
#include "config.h"			// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
					//   HAVE_SYS_RESOURCE_H, HAVE_SYS_STATVFS_H, VERSION
					//   and ENABLE_NLS
#include <common/ClientVersion.h>

#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#ifndef AMULE_DAEMON
	#include <wx/choicdlg.h>	// Needed for wxMultiChoiceDialog (GUI-only)
#endif
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>
#include <wx/regex.h>			// Needed for wxRegEx (version check JSON parse)
#include <wx/socket.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/stopwatch.h>		// Needed for wxStopWatch
#ifdef __WINDOWS__
#include <wx/stdpaths.h>		// Needed for wxStandardPaths (CA bundle lookup)
#include <wx/filename.h>		// Needed for wxFileName (CA bundle lookup)
#endif

#ifdef __WXGTK__
#include <glib.h>			// g_set_prgname() — wl_app_id / WM_CLASS binding
#endif


#include <common/Format.h>		// Needed for CFormat
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "CanceledFileList.h"
#include "ClientCreditsList.h"		// Needed for CClientCreditsList
#include "ClientList.h"			// Needed for CClientList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket & CMuleUDPSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include <common/FileFunctions.h>	// Needed for CDirIterator
#include "FriendList.h"			// Needed for CFriendList
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "InternalEvents.h"		// Needed for CMuleInternalEvent
#include "IPFilter.h"			// Needed for CIPFilter
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "ListenSocket.h"		// Needed for CListenSocket
#include "Logger.h"			// Needed for CLogger // Do_not_auto_remove
#include "MagnetURI.h"			// Needed for CMagnetURI
#include "OtherFunctions.h"
#include "PartFile.h"			// Needed for CPartFile
#include "PlatformSpecific.h"   // Needed for PlatformSpecific::AllowSleepMode();
#include "Preferences.h"		// Needed for CPreferences
#include "SearchList.h"			// Needed for CSearchList
#include "Server.h"			// Needed for GetListName
#include "ServerList.h"			// Needed for CServerList
#include "ServerConnect.h"              // Needed for CServerConnect
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "Statistics.h"			// Needed for CStatistics
#include "TerminationProcessAmuleweb.h"	// Needed for CTerminationProcessAmuleweb
#include "ThreadTasks.h"
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "PartFileWriteThread.h"	// Needed for CPartFileWriteThread
#include "PartFileHashThread.h"		// Needed for CPartFileHashThread
#include "UploadBandwidthThrottler.h"
#include "UploadDiskIOThread.h"
#include "UserEvents.h"
#include "ScopedPtr.h"

#ifdef ENABLE_UPNP
#include "UPnPBase.h"			// Needed for UPnP
#endif

#ifdef __WXMAC__
#include <wx/sysopt.h>			// Do_not_auto_remove
#endif

#ifndef AMULE_DAEMON
	#ifdef __WXMAC__
		#include <CoreFoundation/CFBundle.h>  // Do_not_auto_remove
		#include <wx/osx/core/cfstring.h>  // Do_not_auto_remove
	#endif
	#include <wx/msgdlg.h>

	#include "amuleDlg.h"
#endif


#ifdef HAVE_SYS_RESOURCE_H
	#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
	#include <sys/statvfs.h>  // Do_not_auto_remove
#endif


#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

#ifdef AMULE_DAEMON
CamuleDaemonApp *theApp;
#else
CamuleGuiApp *theApp;
#endif

static void UnlimitResource(RLIMIT_RESOURCE resType)
{
#if defined(HAVE_GETRLIMIT) && defined(HAVE_SETRLIMIT)
	struct rlimit rl;
	getrlimit(resType, &rl);
	rl.rlim_cur = rl.rlim_max;
	setrlimit(resType, &rl);
#endif
}


static void SetResourceLimits()
{
#ifdef HAVE_SYS_RESOURCE_H
	UnlimitResource(RLIMIT_DATA);
#ifndef __UCLIBC__
	UnlimitResource(RLIMIT_FSIZE);
#endif
	UnlimitResource(RLIMIT_NOFILE);
#ifdef RLIMIT_RSS
	UnlimitResource(RLIMIT_RSS);
#endif
#endif
}

// We store the received signal in order to avoid race-conditions
// in the signal handler.
bool g_shutdownSignal = false;

void OnShutdownSignal( int /* sig */ )
{
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	g_shutdownSignal = true;

	// The actual shutdown trigger is driven from OnCoreTimer (normal
	// context) since calling ExitMainLoop() from signal context isn't
	// async-signal-safe and silently no-ops on macOS's wxAppConsole
	// event loop.
}


CamuleApp::CamuleApp()
{
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	StartTickTimer();

	// Initialization
	m_app_state = APP_STATE_STARTING;

	theApp = &wxGetApp();

	clientlist	= NULL;
	searchlist	= NULL;
	knownfiles	= NULL;
	canceledfiles	= NULL;
	serverlist	= NULL;
	serverconnect	= NULL;
	sharedfiles	= NULL;
	listensocket	= NULL;
	clientudp	= NULL;
	clientcredits	= NULL;
	friendlist	= NULL;
	downloadqueue	= NULL;
	uploadqueue	= NULL;
	ipfilter	= NULL;
	ECServerHandler = NULL;
	glob_prefs	= NULL;
	m_statistics	= NULL;
	uploadBandwidthThrottler = NULL;
	uploadDiskIOThread = NULL;
#ifdef ENABLE_UPNP
	m_upnp		= NULL;
	m_upnpMappings.resize(4);
#endif
	core_timer	= NULL;
	partFileHashThread = NULL;

	m_localip	= 0;
	m_dwPublicIP	= 0;
	webserver_pid	= 0;

	enable_daemon_fork = false;

	// Apparently needed for *BSD
	SetResourceLimits();

#ifdef _MSC_VER
	_CrtSetDbgFlag(0);		// Disable useless memleak debugging
#endif
}

CamuleApp::~CamuleApp()
{
	// Closing the log-file as the very last thing, since
	// wxWidgets log-events are saved in it as well.
	theLogger.CloseLogfile();
}

int CamuleApp::OnExit()
{
	// Guard against double-entry: on macOS the EVT_END_SESSION handler calls
	// OnExit() explicitly (so the destructor chain runs before Cocoa
	// terminates the process), and wxEntry may also call it on event-loop
	// teardown. Without the guard the queues would be double-freed.
	static bool s_exitDone = false;
	if (s_exitDone) {
		return 0;
	}
	s_exitDone = true;

	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("Now, exiting main app..."));
	}

	// From wxWidgets docs, wxConfigBase:
	// ...
	// Note that you must delete this object (usually in wxApp::OnExit)
	// in order to avoid memory leaks, wxWidgets won't do it automatically.
	//
	// As it happens, you may even further simplify the procedure described
	// above: you may forget about calling Set(). When Get() is called and
	// there is no current object, it will create one using Create() function.
	// To disable this behaviour DontCreateOnDemand() is provided.
	delete wxConfigBase::Set((wxConfigBase *)NULL);

	// Save credits
	clientcredits->SaveList();

	// Kill amuleweb if running
	if (webserver_pid) {
		AddLogLineNS(CFormat(_("Terminating amuleweb instance with pid '%ld' ... ")) % webserver_pid);
		wxKillError rc;
		if (wxKill(webserver_pid, wxSIGTERM, &rc) == -1) {
			AddLogLineNS(CFormat(_("Killing amuleweb instance with pid '%ld' ... ")) % webserver_pid);
			if (wxKill(webserver_pid, wxSIGKILL, &rc) == -1) {
				AddLogLineNS(_("Failed"));
			}
		}
	}

	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("aMule OnExit: Terminating core."));
	}

	delete serverlist;
	serverlist = NULL;

	delete searchlist;
	searchlist = NULL;

	delete clientcredits;
	clientcredits = NULL;

	delete friendlist;
	friendlist = NULL;

	// Destroying CDownloadQueue calls destructor for CPartFile
	// calling CSharedFileList::SafeAddKFile occasionally.
	delete sharedfiles;
	sharedfiles = NULL;

	delete serverconnect;
	serverconnect = NULL;

	delete listensocket;
	listensocket = NULL;

	delete clientudp;
	clientudp = NULL;

	delete knownfiles;
	knownfiles = NULL;

	delete canceledfiles;
	canceledfiles = NULL;

	delete clientlist;
	clientlist = NULL;

	// Stop upload disk I/O thread before deleting uploadqueue — the thread
	// iterates uploadqueue->GetUploadingList() and will crash if it runs
	// after uploadqueue is freed.
	if (uploadDiskIOThread) {
		uploadDiskIOThread->EndThread();
		delete uploadDiskIOThread;
		uploadDiskIOThread = NULL;
	}

	delete uploadqueue;
	uploadqueue = NULL;

	// Stop hash thread first so any in-flight HashSinglePart finishes
	// and m_pendingHashes drops to 0 before ~CPartFile waits on it.
	if (partFileHashThread) {
		partFileHashThread->EndThread();
		delete partFileHashThread;
		partFileHashThread = NULL;
	}

	// Stop write thread before deleting downloadqueue — must drain pending writes.
	if (partFileWriteThread) {
		partFileWriteThread->EndThread();
		delete partFileWriteThread;
		partFileWriteThread = NULL;
	}

	delete downloadqueue;
	downloadqueue = NULL;

	delete ipfilter;
	ipfilter = NULL;

#ifdef ENABLE_UPNP
	delete m_upnp;
	m_upnp = NULL;
#endif

	delete ECServerHandler;
	ECServerHandler = NULL;

	delete m_statistics;
	m_statistics = NULL;

	delete glob_prefs;
	glob_prefs = NULL;
	CPreferences::EraseItemList();

	// Shut down disk I/O thread before throttler — eMule ref: emule.cpp shutdown order
	if (uploadDiskIOThread) {
		uploadDiskIOThread->EndThread();
		delete uploadDiskIOThread;
		uploadDiskIOThread = NULL;
	}

	delete uploadBandwidthThrottler;
	uploadBandwidthThrottler = NULL;

#ifdef ASIO_SOCKETS
	delete m_AsioService;
	m_AsioService = NULL;
#endif

	wxSocketBase::Shutdown();	// needed because we also called Initialize() manually

	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("aMule shutdown completed."));
	}

#if wxUSE_MEMORY_TRACING
	AddLogLineNS(_("Memory debug results for aMule exit:"));
	// Log mem debug messages to wxLogStderr
	wxLog* oldLog = wxLog::SetActiveTarget(new wxLogStderr);
	//AddLogLineNS("**************Classes**************";
	//wxDebugContext::PrintClasses();
	//AddLogLineNS("***************Dump***************";
	//wxDebugContext::Dump();
	AddLogLineNS("***************Stats**************");
	wxDebugContext::PrintStatistics(true);

	// Set back to wxLogGui
	delete wxLog::SetActiveTarget(oldLog);
#endif

	StopTickTimer();

#if defined(__APPLE__)
	// wx 3.3.2 has a bug in wxWebSessionURLSession::~wxWebSessionURLSession:
	// it releases the NSURLSession and the delegate separately without
	// first calling -invalidateAndCancel. NSURLSession retains its
	// delegate strongly, so the session's dealloc already drops the
	// delegate ref — wx's subsequent release hits a freed object and the
	// process aborts with "pointer being freed was not allocated". This
	// fires in wx module cleanup / atexit / __cxa_finalize on any Mac
	// build after any HTTP download (version check, server.met, ...).
	//
	// By this point in OnExit we have saved state, joined threads, and
	// flushed logs — nothing aMule-owned remains to clean up. _Exit
	// bypasses atexit and static destructors, so the buggy wx dtor never
	// runs and the process terminates cleanly. Linux / Windows continue
	// through wx's normal cleanup; remove this block once the upstream
	// fix lands in a wx release we depend on.
	std::_Exit(0);
#endif

	// Return 0 for successful program termination
	return AMULE_APP_BASE::OnExit();
}


int CamuleApp::InitGui(bool, wxString &)
{
	return 0;
}


//
// Application initialization
//
bool CamuleApp::OnInit()
{
#if wxUSE_MEMORY_TRACING
	// any text before call of Localize_mule needs not to be translated.
	AddLogLineNS("Checkpoint set on app init for memory debug");	// debug output
	wxDebugContext::SetCheckpoint();
#endif

#ifdef __WXGTK__
	// Set the GTK program name to the canonical app id. On Wayland,
	// GTK derives wl_app_id (xdg_toplevel.set_app_id) from
	// g_get_prgname(); compositors match wl_app_id against the
	// .desktop filename to bind windows to launcher icons. Without
	// this the binding falls back to argv[0], which differs across
	// packaging formats (AppImage's argv[0] is "aMule", distro
	// installs use "amule", Flatpak renames the .desktop entirely).
	// On X11 the same value also feeds into WM_CLASS, matching
	// StartupWMClass=org.amule.aMule in the .desktop file. Must run
	// before any GTK window is created.
	g_set_prgname("org.amule.aMule");
#endif

	// Forward wxLog events to CLogger
	wxLog::SetActiveTarget(new CLoggerTarget);

	m_localip = StringHosttoUint32(::wxGetFullHostName());

#ifndef __WINDOWS__
	// get rid of sigpipe
	signal(SIGPIPE, SIG_IGN);
#else
	// Handle CTRL-Break
	signal(SIGBREAK, OnShutdownSignal);
#endif
	// Handle sigint and sigterm
	signal(SIGINT, OnShutdownSignal);
	signal(SIGTERM, OnShutdownSignal);

#ifdef __WXMAC__
	// For listctrl's to behave on Mac
	wxSystemOptions::SetOption("mac.listctrl.always_use_generic", 1);
#endif

#ifdef __WINDOWS__
	// wxWebRequest is backed by libcurl on MSYS2 (MINGW64 / CLANGARM64)
	// builds. MSYS2 libcurl is compiled with `--with-ca-bundle=` pointing
	// at an absolute MSYS2 path that does not exist on end-user machines,
	// so HTTPS (and any HTTP→HTTPS redirect — e.g. SourceForge) fails
	// with "libcurl error 77: Problem with the SSL CA cert". CMake's
	// install step ships a ca-bundle.crt next to the .exe; point
	// CURL_CA_BUNDLE at it here if the user has not set one explicitly.
	{
		wxString existing;
		if (!wxGetEnv("CURL_CA_BUNDLE", &existing) || existing.IsEmpty()) {
			wxFileName caFile(wxStandardPaths::Get().GetExecutablePath());
			caFile.SetFullName("ca-bundle.crt");
			if (caFile.FileExists()) {
				wxSetEnv("CURL_CA_BUNDLE", caFile.GetFullPath());
			}
		}
	}
#endif

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	if (!InitCommon(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv)) {
		return false;
	}

	glob_prefs = new CPreferences();

	CPath outDir;
	if (CheckMuleDirectory("temp", thePrefs::GetTempDir(), thePrefs::GetConfigDir() + "Temp", outDir)) {
		thePrefs::SetTempDir(outDir);
	} else {
		return false;
	}

	if (CheckMuleDirectory("incoming", thePrefs::GetIncomingDir(), thePrefs::GetConfigDir() + "Incoming", outDir)) {
		thePrefs::SetIncomingDir(outDir);
	} else {
		return false;
	}

	// Initialize wx sockets (needed for http download in background with Asio sockets)
	wxSocketBase::Initialize();

#if defined(__WXGTK__) && !defined(WITH_LIBAYATANA_APPINDICATOR)
	// On Linux without libayatana-appindicator3 the tray icon falls
	// back to the legacy GtkStatusIcon backend, which GNOME Shell
	// dropped in 3.26 and wlroots-based compositors never picked up
	// — the icon is silently invisible. Force the pref off so users
	// don't end up with the window hidden via HideOnClose and no
	// surface to bring it back. The existing sanity check below
	// will then cascade MinToTray off as well.
	thePrefs::SetUseTrayIcon(false);
#endif

#ifdef __WXGTK__
	// xdg-shell intentionally doesn't deliver iconified-state
	// notifications to clients, so on Wayland the system minimize
	// button cannot trigger our Show(false) hide-to-tray path.
	// The same gap is documented in qBittorrent #17265, Telegram
	// #2123, KeePassXC #6502 and others. Force MinToTray off when
	// running under a Wayland session so the option doesn't appear
	// to "do nothing" — the prefs panel also greys the checkbox.
	if (CamuleAppCommon::IsWaylandSession()) {
		thePrefs::SetMinToTray(false);
	}
#endif

	// Some sanity check
	if (!thePrefs::UseTrayIcon()) {
		thePrefs::SetMinToTray(false);
	}

	// Build the filenames for the two OS files
	SetOSFiles(thePrefs::GetOSDir().GetRaw());

	// Check if we have the old style locale config
	bool old_localedef = false;
	wxString langId = thePrefs::GetLanguageID();
	if (!langId.IsEmpty() && (langId.GetChar(0) >= '0' && langId.GetChar(0) <= '9')) {
		old_localedef = true;
		thePrefs::SetLanguageID(wxLang2Str(wxLANGUAGE_DEFAULT));
		glob_prefs->Save();
	}

#ifdef ENABLE_NLS
	// Load localization settings
	Localize_mule();

	if (old_localedef) {
		ShowAlert(_("Your locale has been changed to System Default due to a configuration change. Sorry."), _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
	}
#endif

	// Configure EC for amuled when invoked with ec-config
	if (ec_config) {
		AddLogLineNS(_("\nEC configuration"));
		thePrefs::SetECPass(GetPassword(false).Encode());
		thePrefs::EnableExternalConnections(true);
		AddLogLineNS(_("Password set and external connections enabled."));
	}

#ifndef __WINDOWS__
	if (getuid() == 0) {
		wxString msg =
			"Warning! You are running aMule as root.\n"
			"Doing so is not recommended for security reasons,\n"
			"and you are advised to run aMule as an normal\n"
			"user instead.";

		ShowAlert(msg, _("WARNING"), wxCENTRE | wxOK | wxICON_ERROR);

		fprintf(stderr, "\n--------------------------------------------------\n");
		fprintf(stderr, "%s", (const char*)unicode2UTF8(msg));
		fprintf(stderr, "\n--------------------------------------------------\n\n");
	}
#endif

	// Display notification on new version or first run
	wxTextFile vfile( thePrefs::GetConfigDir() + "lastversion" );
	wxString newMule(VERSION);

	if ( !wxFileExists( vfile.GetName() ) ) {
		vfile.Create();
	}

	if ( vfile.Open() ) {
		// Check if this version has been run before
		bool found = false;
		for ( size_t i = 0; i < vfile.GetLineCount(); i++ ) {
			// Check if this version has been run before
			if ( vfile.GetLine(i) == newMule ) {
				found = true;
				break;
			}
		}

		// We haven't run this version before?
		if ( !found ) {
			// Insert new at top to provide faster searches
			vfile.InsertLine( newMule, 0 );

			Trigger_New_version( newMule );
		}

		// Keep at most 10 entries
		while ( vfile.GetLineCount() > 10 )
			vfile.RemoveLine( vfile.GetLineCount() - 1 );

		vfile.Write();
		vfile.Close();
	}

	m_statistics = new CStatistics();

	clientlist	= new CClientList();
	friendlist	= new CFriendList();
	searchlist	= new CSearchList();
	knownfiles	= new CKnownFileList();
	canceledfiles	= new CCanceledFileList;
	serverlist	= new CServerList();

	sharedfiles	= new CSharedFileList(knownfiles);
	clientcredits	= new CClientCreditsList();

	// bugfix - do this before creating the uploadqueue
	downloadqueue	= new CDownloadQueue();
	uploadqueue	= new CUploadQueue();
	partFileWriteThread = new CPartFileWriteThread();
	partFileHashThread = new CPartFileHashThread();
	ipfilter	= new CIPFilter();

	// Creates all needed listening sockets
	wxString msg;
	if (!ReinitializeNetwork(&msg)) {
		AddLogLineNS("\n");
		AddLogLineNS(msg);
	}

	// Test if there's any new version. The URL is the GitHub Releases
	// "latest" endpoint, which returns JSON describing the most recent
	// non-prerelease, non-draft Release.  We parse the `tag_name` field
	// in CheckNewVersion() below.  This replaces the legacy SourceForge
	// `lastversion` text file, which has been unmaintained since the
	// project moved to GitHub years ago.
	if (thePrefs::GetCheckNewVersion()) {
		// We use the thread base because I don't want a dialog to pop up.
		CHTTPDownloadThread* version_check =
			new CHTTPDownloadThread("https://api.github.com/repos/amule-org/amule/releases/latest",
				thePrefs::GetConfigDir() + "last_version_check", thePrefs::GetConfigDir() + "last_version", HTTP_VersionCheck, false, false);
		version_check->Create();
		version_check->Run();
	}

	// Create main dialog, or fork to background (daemon).
	InitGui(m_geometryEnabled, m_geometryString);

#ifdef AMULE_DAEMON
	// Need to refresh wxSingleInstanceChecker after the daemon fork() !
	if (enable_daemon_fork) {
		RefreshSingleInstanceChecker();
		// No need to check IsAnotherRunning() - we've done it before.
	}
#endif

	// Has to be created after the call to InitGui, as fork
	// (when using posix threads) only replicates the mainthread,
	// and the UBT constructor creates a thread.
	uploadBandwidthThrottler = new UploadBandwidthThrottler();

	// Start disk I/O thread — must be after uploadBandwidthThrottler.
	// eMule ref: emule.cpp:748
	uploadDiskIOThread = new CUploadDiskIOThread();

#ifdef ASIO_SOCKETS
	m_AsioService = new CAsioService;
#endif

	// Start performing background tasks
	// This will start loading the IP filter. It will start right away.
	// Log is confusing, because log entries from background will only be printed
	// once foreground becomes idle, and that will only be after loading
	// of the partfiles has finished.
	CThreadScheduler::Start();

	// These must be initialized after the gui is loaded.
	if (thePrefs::GetNetworkED2K()) {
		serverlist->Init();
	}
	downloadqueue->LoadMetFiles(thePrefs::GetTempDir());
	sharedfiles->Reload();

	// Start the fs-watcher after the initial scan so directories exist
	// in shareddir_list before Add() runs. The watcher itself is cheap
	// when no events fire; gating it on the user pref keeps inotify
	// watches off the books on hosts where the user doesn't want them.
	if (thePrefs::AutoRescanSharedDirs()) {
		sharedfiles->EnableDirectoryWatcher(true);
	}

	// Ensure that the up/down ratio is used
	CPreferences::CheckUlDlRatio();

	// Load saved friendlist (now, so it can update in GUI right away)
	friendlist->LoadList();

	// The user can start pressing buttons like mad if he feels like it.
	m_app_state = APP_STATE_RUNNING;

	{
		const bool needServerMet = !serverlist->GetServerCount() && thePrefs::GetNetworkED2K();
		const bool needNodesDat  = thePrefs::GetNetworkKademlia()
			&& !wxFileExists(thePrefs::GetConfigDir() + "nodes.dat");

		if (needServerMet || needNodesDat) {
#ifndef AMULE_DAEMON
			wxArrayString choices;
			if (needServerMet) choices.Add(_("eD2k server list (server.met)"));
			if (needNodesDat)  choices.Add(_("Kad bootstrap nodes (nodes.dat)"));

			wxArrayInt defaults;
			for (size_t i = 0; i < choices.GetCount(); ++i) defaults.Add(i);

			wxMultiChoiceDialog dlg(
				static_cast<wxWindow*>(theApp->amuledlg),
				_("aMule has detected missing network bootstrap files.\nSelect which ones to download:"),
				_("Network bootstrap"),
				choices);
			dlg.SetSelections(defaults);

			if (dlg.ShowModal() == wxID_OK) {
				const wxArrayInt sel = dlg.GetSelections();
				int idx = 0;
				if (needServerMet) {
					if (sel.Index(idx++) != wxNOT_FOUND) {
						serverlist->UpdateServerMetFromURL(thePrefs::GetEd2kServersUrl());
					}
				}
				if (needNodesDat) {
					if (sel.Index(idx++) != wxNOT_FOUND) {
						UpdateNotesDat(thePrefs::GetKadNodesUrl());
					}
				}
			}
#else
			if (needServerMet) {
				serverlist->UpdateServerMetFromURL(thePrefs::GetEd2kServersUrl());
			}
			if (needNodesDat) {
				UpdateNotesDat(thePrefs::GetKadNodesUrl());
			}
#endif
		}
	}


	// Autoconnect if that option is enabled
	if (thePrefs::DoAutoConnect()) {
		// IP filter is still loading and will be finished on event.
		// Tell it to autoconnect.
		if (thePrefs::GetNetworkED2K()) {
			ipfilter->ConnectToAnyServerWhenReady();
		}
		if (thePrefs::GetNetworkKademlia()) {
			ipfilter->StartKADWhenReady();
		}
	}

	// Enable GeoIP
#ifdef ENABLE_IP2COUNTRY
	theApp->amuledlg->EnableIP2Country();
#endif

	// Run webserver?
	if (thePrefs::GetWSIsEnabled()) {
		wxString aMuleConfigFile = thePrefs::GetConfigDir() + m_configFile;
		wxString amulewebPath = thePrefs::GetWSPath();

#if defined(__WXMAC__) && !defined(AMULE_DAEMON)
		// For the Mac GUI application, look for amuleweb in the bundle
		CFURLRef amulewebUrl = CFBundleCopyAuxiliaryExecutableURL(
			CFBundleGetMainBundle(), CFSTR("amuleweb"));

		if (amulewebUrl) {
			CFURLRef absoluteUrl = CFURLCopyAbsoluteURL(amulewebUrl);
			CFRelease(amulewebUrl);

			if (absoluteUrl) {
				CFStringRef amulewebCfstr = CFURLCopyFileSystemPath(absoluteUrl, kCFURLPOSIXPathStyle);
				CFRelease(absoluteUrl);
				amulewebPath = wxCFStringRef(amulewebCfstr).AsString();
			}
		}
#endif

#ifdef __WINDOWS__
#	define QUOTE	"\""
#else
#	define QUOTE	"\'"
#endif

		wxString cmd =
			QUOTE +
			amulewebPath +
			QUOTE " " QUOTE "--amule-config-file=" +
			aMuleConfigFile +
			QUOTE;
		CTerminationProcessAmuleweb *p = new CTerminationProcessAmuleweb(cmd, &webserver_pid);
		webserver_pid = wxExecute(cmd, wxEXEC_ASYNC, p);
		bool webserver_ok = webserver_pid > 0;
		if (webserver_ok) {
			AddLogLineC(CFormat(_("web server running on pid %d")) % webserver_pid);
		} else {
			delete p;
			ShowAlert(_(
				"You requested to run web server on startup, but the amuleweb binary cannot be run. Please install the package containing aMule web server, or compile aMule using --enable-webserver and run make install"),
				_("ERROR"), wxOK | wxICON_ERROR);
		}
	}

	return true;
}

bool CamuleApp::ReinitializeNetwork(wxString* msg)
{
	bool ok = true;
	static bool firstTime = true;

	if (!firstTime) {
		// TODO: Destroy previously created sockets
	}
	firstTime = false;

	// Some sanity checks first
	if (thePrefs::ECPort() == thePrefs::GetPort()) {
		// Select a random usable port in the range 1025 ... 2^16 - 1
		uint16 port = thePrefs::ECPort();
		while ( port < 1024 || port  == thePrefs::GetPort() ) {
			port = (uint16)rand();
		}
		thePrefs::SetECPort( port );

		wxString err =
			"Network configuration failed! You cannot use the same port\n"
			"for the main TCP port and the External Connections port.\n"
			"The EC port has been changed to avoid conflict, see the\n"
			"preferences for the new value.\n";
		*msg << err;

		AddLogLineN("" );
		AddLogLineC(err );
		AddLogLineN("" );

		ok = false;
	}

	if (thePrefs::GetUDPPort() == thePrefs::GetPort() + 3) {
		// Select a random usable value in the range 1025 ... 2^16 - 1
		uint16 port = thePrefs::GetUDPPort();
		while ( port < 1024 || port == thePrefs::GetPort() + 3 ) {
			port = (uint16)rand();
		}
		thePrefs::SetUDPPort( port );

		wxString err =
			"Network configuration failed! You set your UDP port to\n"
			"the value of the main TCP port plus 3.\n"
			"This port has been reserved for the Server-UDP port. The\n"
			"port value has been changed to avoid conflict, see the\n"
			"preferences for the new value\n";
		*msg << err;

		AddLogLineN("" );
		AddLogLineC(err );
		AddLogLineN("" );

		ok = false;
	}

	// Create the address where we are going to listen
	// TODO: read this from configuration file
	amuleIPV4Address myaddr[4];

	// Create the External Connections Socket.
	// Default is 4712.
	// Get ready to handle connections from apps like amulecmd
	if (thePrefs::GetECAddress().IsEmpty() || !myaddr[0].Hostname(thePrefs::GetECAddress())) {
		myaddr[0].AnyAddress();
	}
	myaddr[0].Service(thePrefs::ECPort());
	ECServerHandler = new ExternalConn(myaddr[0], msg);

	// Create the UDP socket TCP+3.
	// Used for source asking on servers.
	if (thePrefs::GetAddress().IsEmpty()) {
		myaddr[1].AnyAddress();
	} else if (!myaddr[1].Hostname(thePrefs::GetAddress())) {
		myaddr[1].AnyAddress();
		AddLogLineC(CFormat(_("Could not bind ports to the specified address: %s"))
			% thePrefs::GetAddress());
	}

	wxString ip = myaddr[1].IPAddress();
	myaddr[1].Service(thePrefs::GetPort()+3);
	serverconnect = new CServerConnect(serverlist, myaddr[1]);
	*msg << CFormat( "*** Server UDP socket (TCP+3) at %s:%u\n" )
		% ip % ((unsigned int)thePrefs::GetPort() + 3u);

	// Create the ListenSocket (aMule TCP socket).
	// Used for Client Port / Connections from other clients,
	// Client to Client Source Exchange.
	// Default is 4662.
	myaddr[2] = myaddr[1];
	myaddr[2].Service(thePrefs::GetPort());
	listensocket = new CListenSocket(myaddr[2]);
	*msg << CFormat( "*** TCP socket (TCP) listening on %s:%u\n" )
		% ip % (unsigned int)(thePrefs::GetPort());
	// Notify(true) has already been called to the ListenSocket, so events may
	// be already coming in.
	if (!listensocket->IsOk()) {
		// If we weren't able to start listening, we need to warn the user
		wxString err;
		err = CFormat(_("Port %u is not available. You will be LOWID\n")) %
			(unsigned int)(thePrefs::GetPort());
		*msg << err;
		AddLogLineC(err);
		err.Clear();
		err = CFormat(
			_("Port %u is not available!\n\nThis means that you will be LOWID.\n\nCheck your network to make sure the port is open for output and input.")) %
			(unsigned int)(thePrefs::GetPort());
		ShowAlert(err, _("ERROR"), wxOK | wxICON_ERROR);
	}

	// Create the UDP socket.
	// Used for extended eMule protocol, Queue Rating, File Reask Ping.
	// Also used for Kademlia.
	// Default is port 4672.
	myaddr[3] = myaddr[1];
	myaddr[3].Service(thePrefs::GetUDPPort());
	clientudp = new CClientUDPSocket(myaddr[3], thePrefs::GetProxyData());
	if (!thePrefs::IsUDPDisabled()) {
		*msg << CFormat( "*** Client UDP socket (extended eMule) at %s:%u" )
			% ip % thePrefs::GetUDPPort();
	} else {
		*msg << "*** Client UDP socket (extended eMule) disabled on preferences";
	}

#ifdef ENABLE_UPNP
	if (thePrefs::GetUPnPEnabled()) {
		try {
			m_upnpMappings[0] = CUPnPPortMapping(
				myaddr[0].Service(),
				"TCP",
				thePrefs::GetUPnPECEnabled(),
				"aMule TCP External Connections Socket");
			m_upnpMappings[1] = CUPnPPortMapping(
				myaddr[1].Service(),
				"UDP",
				thePrefs::GetUPnPEnabled(),
				"aMule UDP socket (TCP+3)");
			m_upnpMappings[2] = CUPnPPortMapping(
				myaddr[2].Service(),
				"TCP",
				thePrefs::GetUPnPEnabled(),
				"aMule TCP Listen Socket");
			m_upnpMappings[3] = CUPnPPortMapping(
				myaddr[3].Service(),
				"UDP",
				thePrefs::GetUPnPEnabled(),
				"aMule UDP Extended eMule Socket");
			m_upnp = new CUPnPControlPoint(thePrefs::GetUPnPTCPPort());

			wxStopWatch count; // Wait UPnP service responses for 3s before add port mappings
			while (count.Time() < 3000 && !m_upnp->WanServiceDetected());

			m_upnp->AddPortMappings(m_upnpMappings);
		} catch(CUPnPException &e) {
			wxString error_msg;
			error_msg << e.what();
			AddLogLineC(error_msg);
			fprintf(stderr, "%s\n", (const char *)unicode2char(error_msg));
		}
	}
#endif

	return ok;
}

/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */)
{
	// Do not do anything if online signature is disabled in Preferences
	if (!thePrefs::IsOnlineSignatureEnabled() || m_emulesig_path.IsEmpty()) {
		// We do not need to check m_amulesig_path because if m_emulesig_path is empty,
		// that means m_amulesig_path is empty too.
		return;
	}

	// Remove old signature files
	if ( wxFileExists( m_emulesig_path ) ) { wxRemoveFile( m_emulesig_path ); }
	if ( wxFileExists( m_amulesig_path ) ) { wxRemoveFile( m_amulesig_path ); }


	wxTextFile amulesig_out;
	wxTextFile emulesig_out;

	// Open both files if needed
	if ( !emulesig_out.Create( m_emulesig_path) ) {
		AddLogLineC(_("Failed to create OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	if ( !amulesig_out.Create(m_amulesig_path) ) {
		AddLogLineC(_("Failed to create aMule OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	wxString emulesig_string;
	wxString temp;

	if (zero) {
		emulesig_string = L"0\xA0.0|0.0|0";
		amulesig_out.AddLine("0\n0\n0\n0\n0\n0\n0.0\n0.0\n0\n0");
	} else {
		if (IsConnectedED2K()) {

			temp = CFormat("%d") % serverconnect->GetCurrentServer()->GetPort();

			// We are online
			emulesig_string =
				// Connected
				"1|"
				//Server name
				+ serverconnect->GetCurrentServer()->GetListName()
				+ "|"
				// IP and port of the server
				+ serverconnect->GetCurrentServer()->GetFullIP()
				+ "|"
				+ temp;


			// Now for amule sig

			// Connected. State 1, full info
			amulesig_out.AddLine("1");
			// Server Name
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetListName());
			// Server IP
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetFullIP());
			// Server Port
			amulesig_out.AddLine(temp);

			if (serverconnect->IsLowID()) {
				amulesig_out.AddLine("L");
			} else {
				amulesig_out.AddLine("H");
			}

		} else if (serverconnect->IsConnecting()) {
			emulesig_string = L"0";

			// Connecting. State 2, No info.
			amulesig_out.AddLine("2\n0\n0\n0\n0");
		} else {
			// Not connected to a server
			emulesig_string = L"0";

			// Not connected, state 0, no info
			amulesig_out.AddLine("0\n0\n0\n0\n0");
		}
		if (IsConnectedKad()) {
			if(Kademlia::CKademlia::IsFirewalled()) {
				// Connected. Firewalled. State 1.
				amulesig_out.AddLine("1");
			} else {
				// Connected. State 2.
				amulesig_out.AddLine("2");
			}
		} else {
			// Not connected.State 0.
			amulesig_out.AddLine("0");
		}
		emulesig_string += "\xA";

		// Datarate for downloads
		temp = CFormat("%.1f") % (theStats::GetDownloadRate() / 1024.0);

		emulesig_string += temp + "|";
		amulesig_out.AddLine(temp);

		// Datarate for uploads
		temp = CFormat("%.1f") % (theStats::GetUploadRate() / 1024.0);

		emulesig_string += temp + "|";
		amulesig_out.AddLine(temp);

		// Number of users waiting for upload
		temp = CFormat("%d") % theStats::GetWaitingUserCount();

		emulesig_string += temp;
		amulesig_out.AddLine(temp);

		// Number of shared files (not on eMule)
		amulesig_out.AddLine(CFormat("%d") % theStats::GetSharedFileCount());
	}

	// eMule signature finished here. Write the line to the wxTextFile.
	emulesig_out.AddLine(emulesig_string);

	// Now for aMule signature extras

	// Nick on the network
	amulesig_out.AddLine(thePrefs::GetUserNick());

	// Total received in bytes
	amulesig_out.AddLine(CFormat("%llu") % theStats::GetTotalReceivedBytes());

	// Total sent in bytes
	amulesig_out.AddLine(CFormat("%llu") % theStats::GetTotalSentBytes());

	// amule version
#ifdef SVNDATE
	amulesig_out.AddLine(VERSION " " SVNDATE);
#else
	amulesig_out.AddLine(VERSION);
#endif

	if (zero) {
		amulesig_out.AddLine("0");
		amulesig_out.AddLine("0");
		amulesig_out.AddLine("0");
	} else {
        // Total received bytes in session
		amulesig_out.AddLine( CFormat( "%llu" ) %
			theStats::GetSessionReceivedBytes() );

        // Total sent bytes in session
		amulesig_out.AddLine( CFormat( "%llu" ) %
			theStats::GetSessionSentBytes() );

		// Uptime
		amulesig_out.AddLine(CFormat("%llu") % theStats::GetUptimeSeconds());
	}

	// Flush the files
	emulesig_out.Write();
	amulesig_out.Write();
} //End Added By Bouc7


#if wxUSE_ON_FATAL_EXCEPTION
// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
	/* Print the backtrace */
	wxString msg;
	msg	<< "\n--------------------------------------------------------------------------------\n"
		<< "A fatal error has occurred and aMule has crashed.\n"
		<< "Please assist us in fixing this problem by reporting the backtrace below as a\n"
		<< "GitHub issue, including as much information as possible regarding the\n"
		<< "circumstances of this crash. Issue tracker:\n"
		<< "    https://github.com/amule-org/amule/issues\n"
		<< "If possible, please try to generate a real backtrace of this crash:\n"
		<< "    https://github.com/amule-org/amule/wiki/Backtraces\n\n"
		<< "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n"
		<< "Current version is: " << FullMuleVersion
		<< "\nRunning on: " << OSDescription
		<< "\n\n"
		<< get_backtrace(1) // 1 == skip this function.
		<< "\n--------------------------------------------------------------------------------\n";

	theLogger.EmergencyLog(msg, true);
}
#endif


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));
	if (!m_locale.IsOk()) {
		AddLogLineN(_("The selected locale seems not to be installed on your box. (Note: I'll try to set it anyway)"));
	}
}


// Displays information related to important changes in aMule.
// Is called when the user runs a new version of aMule
void CamuleApp::Trigger_New_version(wxString new_version)
{
	wxString info = wxString(" --- ") + wxString(CFormat(_("This is the first time you run aMule %s")) % new_version) + " ---\n\n";
	if (new_version == "SVN") {
		info += _("This version is a testing version, updated daily, and\n");
		info += _("we give no warranty it won't break anything, burn your house,\n");
		info += _("or kill your dog. But it *should* be safe to use anyway.\n");
	}

	// General info
	info += "\n";
	info += _("More information, support and new releases can found at our homepage,\n");
	info += _("at https://amule-org.github.io, or in our IRC channel #aMule at irc.freenode.net.\n");
	info += "\n";
	info += _("Feel free to report any bugs to https://github.com/amule-org/amule/issues");

	ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
}


void CamuleApp::SetOSFiles(const wxString& new_path)
{
	if ( thePrefs::IsOnlineSignatureEnabled() ) {
		if ( ::wxDirExists(new_path) ) {
			m_emulesig_path = JoinPaths(new_path, "onlinesig.dat");
			m_amulesig_path = JoinPaths(new_path, "amulesig.dat");
		} else {
			ShowAlert(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("ERROR"), wxOK | wxICON_ERROR);
			m_emulesig_path.Clear();
			m_amulesig_path.Clear();
		}
	} else {
		m_emulesig_path.Clear();
		m_amulesig_path.Clear();
	}
}


#ifndef wxUSE_STACKWALKER
#define wxUSE_STACKWALKER 0
#endif
void CamuleApp::OnAssertFailure(const wxChar* file, int line,
				const wxChar* func, const wxChar* cond, const wxChar* msg)
{
	wxString errmsg = CFormat( "Assertion failed: %s:%s:%d: Assertion '%s' failed. %s\nBacktrace follows:\n%s\n" )
		% file % func % line % cond % ( msg ? wxString(msg) : wxString() )
		% get_backtrace(2);		// Skip the function-calls directly related to the assert call.
	theLogger.EmergencyLog(errmsg, false);

	// --disable-fatal: skip the wxApp dialog and abort directly so a
	// supervisor (systemd, watchdog script) sees a non-zero exit and
	// can restart aMule. The errmsg above is already on stderr and in
	// the log; nothing useful would be lost by skipping the dialog.
	if (m_disableFatal) {
		raise(SIGABRT);
		return; // unreachable
	}

	if (wxThread::IsMain() && IsRunning()) {
		AMULE_APP_BASE::OnAssertFailure(file, line, func, cond, msg);
	} else {
#ifdef _MSC_VER
		wxString s = CFormat("%s in %s") % cond % func;
		if (msg) {
			s << " : " << msg;
		}
		_wassert(s.wc_str(), file, line);
#else
		// Abort, allows gdb to catch the assertion
		raise( SIGABRT );
#endif
	}
}


void CamuleApp::OnUDPDnsDone(CMuleInternalEvent& evt)
{
	CServerUDPSocket* socket =reinterpret_cast<CServerUDPSocket*>(evt.GetClientData());
	socket->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnSourceDnsDone(CMuleInternalEvent& evt)
{
	downloadqueue->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnServerDnsDone(CMuleInternalEvent& evt)
{
	AddLogLineNS(_("Server hostname notified"));
	serverconnect->OnServerHostnameResolved(evt.GetClientData(), evt.GetExtraLong());
}


void CamuleApp::OnTCPTimer(CTimerEvent& WXUNUSED(evt))
{
	if(!IsRunning()) {
		return;
	}
	serverconnect->StopConnectionTry();
	if (IsConnectedED2K() ) {
		return;
	}
	serverconnect->ConnectToAnyServer();
}


void CamuleApp::OnCoreTimer(CTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section
	static uint64 msPrev1, msPrev5, msPrevSave, msPrevHist, msPrevOS, msPrevKnownMet;
	uint64 msCur = theStats::GetUptimeMillis();
	TheTime = msCur / 1000;

	if (!IsRunning()) {
		return;
	}

	// Check if we should terminate the app. OnShutdownSignal only sets
	// the flag; the actual exit trigger runs from here (normal context)
	// every CORE_TIMER_PERIOD ms.
	if ( g_shutdownSignal ) {
#ifdef AMULE_DAEMON
#if defined(__APPLE__)
		// wxBase 3.3.2's wxAppConsole event loop on macOS doesn't
		// honour ExitMainLoop without a top-level window driving the
		// close (the way wxApp does for the GUI build below). Run
		// OnExit() directly here for clean shutdown of all subsystems,
		// then _exit() to terminate before wx's own static destructors
		// hit the NSURLSession-cleanup crash also handled in OnExit's
		// __APPLE__ block.
		static bool s_alreadyExiting = false;
		if (!s_alreadyExiting) {
			s_alreadyExiting = true;
			OnExit();
			_exit(0);
		}
#else
		ExitMainLoop();
#endif
#else
		wxWindow* top = GetTopWindow();

		if ( top ) {
			top->Close(true);
		} else {
			// No top-window, have to force termination.
			wxExit();
		}
#endif
	}

	// There is a theoretical chance that the core time function can recurse:
	// if an event function gets blocked on a mutex (communicating with the
	// UploadBandwidthThrottler) wx spawns a new event loop and processes more events.
	// If CPU load gets high a new core timer event could be generated before the last
	// one was finished and so recursion could occur, which would be bad.
	// Detect this and do an early return then.
	static bool recurse = false;
	if (recurse) {
		return;
	}
	recurse = true;

	uploadqueue->Process();
	downloadqueue->Process();
	//theApp->clientcredits->Process();
	theStats::CalculateRates();

	if (msCur-msPrevHist > 1000) {
		// unlike the other loop counters in this function this one will sometimes
		// produce two calls in quick succession (if there was a gap of more than one
		// second between calls to TimerProc) - this is intentional!  This way the
		// history list keeps an average of one node per second and gets thinned out
		// correctly as time progresses.
		msPrevHist += 1000;

		m_statistics->RecordHistory();

	}


	if (msCur-msPrev1 > 1000) {  // approximately every second
		msPrev1 = msCur;
		clientcredits->Process();
		clientlist->Process();

		// Publish files to server if needed.
		sharedfiles->Process();

		if( Kademlia::CKademlia::IsRunning() ) {
			Kademlia::CKademlia::Process();
			if(Kademlia::CKademlia::GetPrefs()->HasLostConnection()) {
				StopKad();
				clientudp->Close();
				clientudp->Open();
				if (thePrefs::Reconnect()) {
					StartKad();
				}
			}
		}

		if( serverconnect->IsConnecting() && !serverconnect->IsSingleConnect() ) {
			serverconnect->TryAnotherConnectionrequest();
		}
		if (serverconnect->IsConnecting()) {
			serverconnect->CheckForTimeout();
		}
		listensocket->UpdateConnectionsStatus();

	}


	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		listensocket->Process();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		theStats::Save();
	}

	// Special
	if (msCur - msPrevOS >= thePrefs::GetOSUpdate() * 1000ull) {
		OnlineSig(); // Added By Bouc7
		msPrevOS = msCur;
	}

	if (msCur - msPrevKnownMet >= 30*60*1000/*There must be a prefs option for this*/) {
		// Save Shared Files data
		knownfiles->Save();
		msPrevKnownMet = msCur;
	}


	// Recommended by lugdunummaster himself - from emule 0.30c
	serverconnect->KeepConnectionAlive();

	// Disarm recursion protection
	recurse = false;
}


void CamuleApp::OnFinishedHashing(CHashingEvent& evt)
{
	wxCHECK_RET(evt.GetResult(), "No result of hashing");

	CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
	CKnownFile* result = evt.GetResult();

	if (owner) {
		// Check if the partfile still exists, as it might have
		// been deleted in the mean time.
		if (downloadqueue->IsPartFile(owner)) {
			// This cast must not be done before the IsPartFile
			// call, as dynamic_cast will barf on dangling pointers.
			dynamic_cast<CPartFile*>(owner)->PartFileHashFinished(result);
		}
	} else {
		static uint64 bytecount = 0;

		if (knownfiles->SafeAddKFile(result, true)) {
			AddDebugLogLineN(logKnownFiles,
				CFormat("Safe adding file to sharedlist: %s") % result->GetFileName());
			sharedfiles->SafeAddKFile(result);

			bytecount += result->GetFileSize();
			// If we have added files with a total size of ~3000mb
			if (bytecount >= wxULL(3145728000)) {
				AddDebugLogLineN(logKnownFiles, "Failsafe for crash on file hashing creation");
				if ( m_app_state != APP_STATE_SHUTTINGDOWN ) {
					knownfiles->Save();
					bytecount = 0;
				}
			}
		} else {
			AddDebugLogLineN(logKnownFiles,
				CFormat("File not added to sharedlist: %s") % result->GetFileName());
			delete result;
		}
	}
}


void CamuleApp::OnPartFileHashResult(CPartFileHashResultEvent& evt)
{
	if (m_app_state == APP_STATE_SHUTTINGDOWN || !theApp || !theApp->IsRunning()) {
		return;
	}

	// Look up the file by hash. If it was removed from the download
	// queue between enqueue and dispatch (cancelled, completed early)
	// the lookup returns NULL and we drop the event safely.
	CPartFile* file = downloadqueue->GetFileByID(evt.FileHash());
	if (!file) {
		AddDebugLogLineN(logPartFile, CFormat(
			"Hash result for part %u: file no longer in download queue, dropping")
			% evt.PartNumber());
		return;
	}

	file->OnAsyncHashComplete(evt.PartNumber(), evt.Ok(),
		evt.FromAICHRecoveryDataAvailable());
}


void CamuleApp::OnFinishedAICHHashing(CHashingEvent& evt)
{
	wxCHECK_RET(evt.GetResult(), "No result of AICH-hashing");

	CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
	CScopedPtr<CKnownFile> result(evt.GetResult());

	if (result->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE) {
		CAICHHashSet* oldSet = owner->GetAICHHashset();
		CAICHHashSet* newSet = result->GetAICHHashset();

		owner->SetAICHHashset(newSet);
		newSet->SetOwner(owner);

		result->SetAICHHashset(oldSet);
		oldSet->SetOwner(result.get());
	}
}


void CamuleApp::OnFinishedCompletion(CCompletionEvent& evt)
{
	CPartFile* completed = const_cast<CPartFile*>(evt.GetOwner());
	wxCHECK_RET(completed, "Completion event sent for unspecified file");
	wxASSERT_MSG(downloadqueue->IsPartFile(completed), "CCompletionEvent for unknown partfile.");

	completed->CompleteFileEnded(evt.ErrorOccurred(), evt.GetFullPath());
	if (evt.ErrorOccurred()) {
		CUserEvents::ProcessEvent(CUserEvents::ErrorOnCompletion, completed);
	}

	// Check if we should execute an script/app/whatever.
	CUserEvents::ProcessEvent(CUserEvents::DownloadCompleted, completed);
}

void CamuleApp::OnFinishedAllocation(CAllocFinishedEvent& evt)
{
	CPartFile *file = evt.GetFile();
	wxCHECK_RET(file, "Allocation finished event sent for unspecified file");
	wxASSERT_MSG(downloadqueue->IsPartFile(file), "CAllocFinishedEvent for unknown partfile");

	file->SetStatus(PS_EMPTY);

	if (evt.Succeeded()) {
		if (evt.IsPaused()) {
			file->StopFile();
		} else {
			file->ResumeFile();
		}
	} else {
		AddLogLineN(CFormat(_("Disk space preallocation for file '%s' failed: %s")) % file->GetFileName() % wxString(UTF82unicode(std::strerror(evt.GetResult()))));
		file->StopFile();
	}

	file->AllocationFinished();
};

void CamuleApp::OnNotifyEvent(CMuleGUIEvent& evt)
{
#ifdef AMULE_DAEMON
	evt.Notify();
#else
	if (theApp->amuledlg) {
		evt.Notify();
	}
#endif
}


void CamuleApp::ShutDown()
{
	// Just in case
	PlatformSpecific::AllowSleepMode();

	// Log
	AddDebugLogLineN(logGeneral, "CamuleApp::ShutDown() has started.");

	// Signal the hashing thread to terminate
	m_app_state = APP_STATE_SHUTTINGDOWN;

	// Stop ASIO thread
#ifdef ASIO_SOCKETS			// only needed to suppress the log message in non-Asio build
	AddDebugLogLineN(logGeneral, "Terminate ASIO thread.");
	m_AsioService->Stop();
#endif

	StopKad();

	// Kry - Save the sources seeds on app exit
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->SaveSourceSeeds();
	}

	OnlineSig(true); // Added By Bouc7

	// Exit HTTP downloads
	CHTTPDownloadThread::StopAll();

	// Exit thread scheduler and upload thread
	CThreadScheduler::Terminate();

	AddDebugLogLineN(logGeneral, "Terminate upload thread.");
	uploadBandwidthThrottler->EndThread();

	// Close sockets to avoid new clients coming in
	if (listensocket) {
		listensocket->Close();
		listensocket->KillAllSockets();
	}

	if (serverconnect) {
		serverconnect->Disconnect();
	}

	ECServerHandler->KillAllSockets();

#ifdef ENABLE_UPNP
	if (thePrefs::GetUPnPEnabled()) {
		if (m_upnp) {
			m_upnp->DeletePortMappings(m_upnpMappings);
		}
	}
#endif

	// saving data & stuff
	if (knownfiles) {
		knownfiles->Save();
	}

	theStats::Save();

	CPath configFileName = CPath(thePrefs::GetConfigDir() + m_configFile);
	CPath::BackupFile(configFileName, ".bak");

	if (clientlist) {
		clientlist->DeleteAll();
	}

	// Log
	AddDebugLogLineN(logGeneral, "CamuleApp::ShutDown() has ended.");
}


bool CamuleApp::AddServer(CServer *srv, bool fromUser)
{
	if ( serverlist->AddServer(srv, fromUser) ) {
		Notify_ServerAdd(srv);
		return true;
	}
	return false;
}


uint32 CamuleApp::GetPublicIP(bool ignorelocal) const
{
	if (m_dwPublicIP == 0) {
		if (Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::GetIPAddress() ) {
			return wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetIPAddress());
		} else {
			return ignorelocal ? 0 : m_localip;
		}
	}

	return m_dwPublicIP;
}


void CamuleApp::SetPublicIP(const uint32 dwIP)
{
	wxASSERT((dwIP == 0) || !IsLowID(dwIP));

	if (dwIP != 0 && dwIP != m_dwPublicIP && serverlist != NULL) {
		m_dwPublicIP = dwIP;
		serverlist->CheckForExpiredUDPKeys();
	} else {
		m_dwPublicIP = dwIP;
	}
}


wxString CamuleApp::GetLog(bool reset)
{
	wxFile logfile;
	logfile.Open(thePrefs::GetConfigDir() + "logfile");
	if ( !logfile.IsOpened() ) {
		return _("ERROR: can't open logfile");
	}
	int len = logfile.Length();
	if ( len == 0 ) {
		return _("WARNING: logfile is empty. Something is wrong.");
	}
	char *tmp_buffer = new char[len + sizeof(wxChar)];
	logfile.Read(tmp_buffer, len);
	memset(tmp_buffer + len, 0, sizeof(wxChar));

	// try to guess file format
	wxString str;
	if (tmp_buffer[0] && tmp_buffer[1]) {
		str = wxString::FromUTF8(tmp_buffer); 
	} else {
		str = wxString(tmp_buffer); 
	}
	delete [] tmp_buffer;
	if ( reset ) {
		theLogger.CloseLogfile();
		if (theLogger.OpenLogfile(thePrefs::GetConfigDir() + "logfile")) {
			AddLogLineN(_("Log has been reset"));
		}
		ECServerHandler->ResetAllLogs();
	}
	return str;
}


wxString CamuleApp::GetServerLog(bool reset)
{
	wxString ret = server_msg;
	if ( reset ) {
		server_msg.Clear();
	}
	return ret;
}

wxString CamuleApp::GetDebugLog(bool reset)
{
	return GetLog(reset);
}


void CamuleApp::AddServerMessageLine(wxString &msg)
{
	server_msg += msg + "\n";
	AddLogLineN(CFormat(_("ServerMessage: %s")) % msg);
}



void CamuleApp::OnFinishedHTTPDownload(CMuleInternalEvent& event)
{
	switch (event.GetInt()) {
		case HTTP_IPFilter:
			ipfilter->DownloadFinished(event.GetExtraLong());
			break;
		case HTTP_ServerMet:
			if (serverlist->DownloadFinished(event.GetExtraLong()) && !IsConnectedED2K()) {
				// If successfully downloaded a server list, and are not connected at the moment, try to connect.
				// This happens when no server met is available on startup.
				serverconnect->ConnectToAnyServer();
			}
			break;
		case HTTP_ServerMetAuto:
			serverlist->AutoDownloadFinished(event.GetExtraLong());
			break;
		case HTTP_VersionCheck:
			CheckNewVersion(event.GetExtraLong());
			break;
		case HTTP_NodesDat:
			if (event.GetExtraLong() == HTTP_Success) {

				wxString file = thePrefs::GetConfigDir() + "nodes.dat";
				if (wxFileExists(file)) {
					wxRemoveFile(file);
				}

				if ( Kademlia::CKademlia::IsRunning() ) {
					Kademlia::CKademlia::Stop();
				}

				wxRenameFile(file + ".download",file);

				Kademlia::CKademlia::Start();
				theApp->ShowConnectionState();
			// cppcheck-suppress duplicateBranch
			} else if (event.GetExtraLong() == HTTP_Skipped) {
				AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % "nodes.dat");
			} else {
				AddLogLineC(_("Failed to download the nodes list."));
			}
			break;
#ifdef ENABLE_IP2COUNTRY
		case HTTP_GeoIP:
			theApp->amuledlg->IP2CountryDownloadFinished(event.GetExtraLong());
			// If we updated, the dialog is already up. Redraw it to show the flags.
			theApp->amuledlg->Refresh();
			break;
#endif
	}
}

void CamuleApp::CheckNewVersion(uint32 result)
{
	if (result == HTTP_Success) {
		wxString filename = thePrefs::GetConfigDir() + "last_version_check";
		wxTextFile file;

		if (!file.Open(filename)) {
			AddLogLineC(_("Failed to open the downloaded version check file") );
			return;
		} else if (!file.GetLineCount()) {
			AddLogLineC(_("Corrupted version check file"));
		} else {
			// The downloaded file is the GitHub Releases /latest JSON
			// response.  Concatenate all lines so the regex below
			// matches across the pretty-printed payload.
			wxString jsonContent;
			for (size_t i = 0; i < file.GetLineCount(); ++i) {
				jsonContent += file.GetLine(i);
			}

			// Extract the `tag_name` string from the JSON.  The
			// /releases/latest endpoint excludes pre-releases by
			// design, so any tag_name we see here represents the
			// latest stable Release.  We don't need a full JSON
			// parser for one well-known field — a regex on the
			// `"tag_name": "..."` pair is robust against
			// whitespace and field-order changes.
			wxRegEx tagRe("\"tag_name\"[[:space:]]*:[[:space:]]*\"([^\"]+)\"");
			if (!tagRe.IsValid() || !tagRe.Matches(jsonContent)) {
				AddLogLineC(_("Corrupted version check file"));
				file.Close();
				wxRemoveFile(filename);
				return;
			}
			wxString versionLine = tagRe.GetMatch(jsonContent, 1);

			// Strip the optional `v` prefix (aMule's tags are bare
			// semver, but be tolerant in case a future maintainer
			// switches to vX.Y.Z).
			if (versionLine.StartsWith("v") || versionLine.StartsWith("V")) {
				versionLine = versionLine.Mid(1);
			}

			// Strip any pre-release / build-metadata suffix so the
			// integer comparison sees only MAJOR.MINOR.UPDATE.
			// /releases/latest already excludes pre-releases, but
			// be defensive: a stable tag like `3.0.0+build42`
			// should still parse.
			size_t suffixPos = versionLine.find_first_of(wxT("-+"));
			if (suffixPos != wxString::npos) {
				versionLine = versionLine.Mid(0, suffixPos);
			}

			// Catch degenerate inputs where the prefix/suffix
			// strip leaves nothing behind (e.g. tag_name "v",
			// "-foo", "v-rc1").  Without this guard the
			// tokenizer returns no tokens, all three fields
			// stay at 0, and the comparison silently reports
			// "up to date" against an unparseable input.
			if (versionLine.IsEmpty()) {
				AddLogLineC(_("Corrupted version check file"));
				file.Close();
				wxRemoveFile(filename);
				return;
			}

			wxStringTokenizer tkz(versionLine, ".");

			AddDebugLogLineN(logGeneral, wxString("Running: ") + VERSION + ", Version check: " + versionLine);

			long fields[] = {0, 0, 0};
			for (int i = 0; i < 3; ++i) {
				if (!tkz.HasMoreTokens()) {
					// Tags with fewer than three components (e.g.
					// a maintainer tagging "3.1" instead of "3.1.0")
					// are valid; treat the missing field as 0.
					break;
				}
				wxString token = tkz.GetNextToken();

				if (!token.ToLong(&fields[i])) {
					AddLogLineC(_("Corrupted version check file"));
					file.Close();
					wxRemoveFile(filename);
					return;
				}
			}

			long curVer = make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE);
			long newVer = make_full_ed2k_version(fields[0], fields[1], fields[2]);

			if (curVer < newVer) {
				AddLogLineC(_("You are using an outdated version of aMule!"));
				// cppcheck-suppress zerodiv
				AddLogLineN(CFormat(_("Your aMule version is %i.%i.%i and the latest version is %li.%li.%li")) % VERSION_MJR % VERSION_MIN % VERSION_UPDATE % fields[0] % fields[1] % fields[2]);
				AddLogLineN(_("The latest version can always be found at https://github.com/amule-org/amule/releases/latest"));
				#ifdef AMULE_DAEMON
				AddLogLineCS(CFormat(_("WARNING: Your aMuled version is outdated: %i.%i.%i < %li.%li.%li"))
					% VERSION_MJR % VERSION_MIN % VERSION_UPDATE % fields[0] % fields[1] % fields[2]);
				#endif
			} else {
				AddLogLineN(_("Your copy of aMule is up to date."));
			}
		}

		file.Close();
		wxRemoveFile(filename);
	} else {
		AddLogLineC(_("Failed to download the version check file"));
	}

}


bool CamuleApp::IsConnected() const
{
	return (IsConnectedED2K() || IsConnectedKad());
}


bool CamuleApp::IsConnectedED2K() const
{
	return serverconnect && serverconnect->IsConnected();
}


bool CamuleApp::IsConnectedKad() const
{
	return Kademlia::CKademlia::IsConnected();
}


bool CamuleApp::IsFirewalled() const
{
	if (theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID()) {
		return false; // we have an eD2K HighID -> not firewalled
	}

	return IsFirewalledKad(); // If kad says ok, it's ok.
}

bool CamuleApp::IsFirewalledKad() const
{
	return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
			|| Kademlia::CKademlia::IsFirewalled();
}

bool CamuleApp::IsFirewalledKadUDP() const
{
	return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
			|| Kademlia::CUDPFirewallTester::IsFirewalledUDP(true);
}

bool CamuleApp::IsKadRunning() const
{
	return Kademlia::CKademlia::IsRunning();
}

bool CamuleApp::IsKadRunningInLanMode() const
{
	return Kademlia::CKademlia::IsRunningInLANMode();
}

// Kad stats
uint32 CamuleApp::GetKadUsers() const
{
	return Kademlia::CKademlia::GetKademliaUsers();
}

uint32 CamuleApp::GetKadFiles() const
{
	return Kademlia::CKademlia::GetKademliaFiles();
}

uint32 CamuleApp::GetKadIndexedSources() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexSource;
}

uint32 CamuleApp::GetKadIndexedKeywords() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexKeyword;
}

uint32 CamuleApp::GetKadIndexedNotes() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexNotes;
}

uint32 CamuleApp::GetKadIndexedLoad() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexLoad;
}


// True IP of machine
uint32 CamuleApp::GetKadIPAddress() const
{
	return wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetPrefs()->GetIPAddress());
}

// Buddy status
uint8	CamuleApp::GetBuddyStatus() const
{
	return clientlist->GetBuddyStatus();
}

uint32	CamuleApp::GetBuddyIP() const
{
	return clientlist->GetBuddyIP();
}

uint32	CamuleApp::GetBuddyPort() const
{
	return clientlist->GetBuddyPort();
}

const Kademlia::CUInt128& CamuleApp::GetKadID() const
{
	return Kademlia::CKademlia::GetKadID();
}

bool CamuleApp::CanDoCallback(uint32 clientServerIP, uint16 clientServerPort)
{
	if (Kademlia::CKademlia::IsConnected()) {
		if (IsConnectedED2K()) {
			if (serverconnect->IsLowID()) {
				if (Kademlia::CKademlia::IsFirewalled()) {
					//Both Connected - Both Firewalled
					return false;
				} else {
					if (clientServerIP == theApp->serverconnect->GetCurrentServer()->GetIP() &&
					   clientServerPort == theApp->serverconnect->GetCurrentServer()->GetPort()) {
						// Both Connected - Server lowID, Kad Open - Client on same server
						// We prevent a callback to the server as this breaks the protocol
						// and will get you banned.
						return false;
					} else {
						// Both Connected - Server lowID, Kad Open - Client on remote server
						return true;
					}
				}
			} else {
				//Both Connected - Server HighID, Kad don't care
				return true;
			}
		} else {
			if (Kademlia::CKademlia::IsFirewalled()) {
				//Only Kad Connected - Kad Firewalled
				return false;
			} else {
				//Only Kad Connected - Kad Open
				return true;
			}
		}
	} else {
		if (IsConnectedED2K()) {
			if (serverconnect->IsLowID()) {
				//Only Server Connected - Server LowID
				return false;
			} else {
				//Only Server Connected - Server HighID
				return true;
			}
		} else {
			//We are not connected at all!
			return false;
		}
	}
}

void CamuleApp::ShowUserCount() {
	uint32 totaluser = 0, totalfile = 0;

	theApp->serverlist->GetUserFileStatus( totaluser, totalfile );

	wxString buffer;

	static const wxString s_singlenetstatusformat = _("Users: %s | Files: %s");
	static const wxString s_bothnetstatusformat = _("Users: E: %s K: %s | Files: E: %s K: %s");

	if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_bothnetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(totalfile) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
	} else if (thePrefs::GetNetworkED2K()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(totalfile);
	} else if (thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
	} else {
		buffer = _("No networks selected");
	}

	Notify_ShowUserCount(buffer);
}


#ifndef ASIO_SOCKETS
void CamuleApp::ListenSocketHandler(wxSocketEvent& event)
{
	{ wxCHECK_RET(listensocket, "Connection-event for NULL'd listen-socket"); }
	{ wxCHECK_RET(event.GetSocketEvent() == wxSOCKET_CONNECTION,
		"Invalid event received for listen-socket"); }

	if (m_app_state == APP_STATE_RUNNING) {
		listensocket->OnAccept();
	} else if (m_app_state == APP_STATE_STARTING) {
		// When starting up, connection may be made before we are able
		// to handle them. However, if these are ignored, no further
		// connection-events will be triggered, so we have to accept it.
		CLibSocket* socket = listensocket->Accept(false);

		wxCHECK_RET(socket, "NULL returned by Accept() during startup");

		socket->Destroy();
	}
}
#endif


void CamuleApp::ShowConnectionState(bool forceUpdate)
{
	static uint8 old_state = (1<<7); // This flag doesn't exist

	uint8 state = 0;

	if (theApp->serverconnect->IsConnected()) {
		state |= CONNECTED_ED2K;
	}

	if (Kademlia::CKademlia::IsRunning()) {
		if (Kademlia::CKademlia::IsConnected()) {
			if (!Kademlia::CKademlia::IsFirewalled()) {
				state |= CONNECTED_KAD_OK;
			} else {
				state |= CONNECTED_KAD_FIREWALLED;
			}
		} else {
			state |= CONNECTED_KAD_NOT;
		}
	}

	if (old_state != state) {
		// Get the changed value
		int changed_flags = old_state ^ state;

		if (changed_flags & CONNECTED_ED2K) {
			// ED2K status changed
			wxString connected_server;
			CServer* ed2k_server = theApp->serverconnect->GetCurrentServer();
			if (ed2k_server) {
				connected_server = ed2k_server->GetListName();
			}
			if (state & CONNECTED_ED2K) {
				// We connected to some server
				const wxString id = theApp->serverconnect->IsLowID() ? _("with LowID") : _("with HighID");

				AddLogLineC(CFormat(_("Connected to %s %s")) % connected_server % id);
			} else {
				// cppcheck-suppress duplicateBranch
				if ( theApp->serverconnect->IsConnecting() ) {
					AddLogLineC(CFormat(_("Connecting to %s")) % connected_server);
				} else {
					AddLogLineC(_("Disconnected from eD2k"));
				}
			}
		}

		if (changed_flags & CONNECTED_KAD_NOT) {
			// cppcheck-suppress duplicateBranch
			if (state & CONNECTED_KAD_NOT) {
				AddLogLineC(_("Kad started."));
			} else {
				AddLogLineC(_("Kad stopped."));
			}
		}

		if (changed_flags & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
			if (state & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
				// cppcheck-suppress duplicateBranch
				if (state & CONNECTED_KAD_OK) {
					AddLogLineC(_("Connected to Kad (ok)"));
				} else {
					AddLogLineC(_("Connected to Kad (firewalled)"));
				}
			} else {
				AddLogLineC(_("Disconnected from Kad"));
			}
		}

		old_state = state;

		theApp->downloadqueue->OnConnectionState(IsConnected());
	}

	ShowUserCount();
	Notify_ShowConnState(forceUpdate);
}


#ifndef ASIO_SOCKETS
void CamuleApp::UDPSocketHandler(wxSocketEvent& event)
{
	CMuleUDPSocket* socket = reinterpret_cast<CMuleUDPSocket*>(event.GetClientData());
	wxCHECK_RET(socket, "No socket owner specified.");

	if (IsOnShutDown() || thePrefs::IsUDPDisabled()) return;

	if (!IsRunning()) {
		if (event.GetSocketEvent() == wxSOCKET_INPUT) {
			// Back to the queue!
			theApp->AddPendingEvent(event);
			return;
		}
	}

	switch (event.GetSocketEvent()) {
		case wxSOCKET_INPUT:
			socket->OnReceive(0);
			break;

		case wxSOCKET_OUTPUT:
			socket->OnSend(0);
			break;

		case wxSOCKET_LOST:
			socket->OnDisconnected(0);
			break;

		default:
			wxFAIL;
			break;
	}
}
#endif


void CamuleApp::OnUnhandledException()
{
	// Call the generic exception-handler.
	fprintf(stderr, "\taMule Version: %s\n", (const char*)unicode2char(GetFullMuleVersion()));
	::OnUnhandledException();
}

void CamuleApp::StartKad()
{
	if (!Kademlia::CKademlia::IsRunning() && thePrefs::GetNetworkKademlia()) {
		// Kad makes no sense without the Client-UDP socket.
		if (!thePrefs::IsUDPDisabled()) {
			if (ipfilter->IsReady()) {
				Kademlia::CKademlia::Start();
			} else {
				ipfilter->StartKADWhenReady();
			}
		} else {
			AddLogLineC(_("Kad network cannot be used if UDP port is disabled on preferences, not starting."));
		}
	} else if (!thePrefs::GetNetworkKademlia()) {
		AddLogLineC(_("Kad network disabled on preferences, not connecting."));
	}
}

void CamuleApp::StopKad()
{
	// Stop Kad if it's running
	if (Kademlia::CKademlia::IsRunning()) {
		Kademlia::CKademlia::Stop();
	}
}


void CamuleApp::BootstrapKad(uint32 ip, uint16 port)
{
	if (!Kademlia::CKademlia::IsRunning()) {
		Kademlia::CKademlia::Start();
		theApp->ShowConnectionState();
	}

	Kademlia::CKademlia::Bootstrap(ip, port);
}


void CamuleApp::UpdateNotesDat(const wxString& url)
{
	wxString strTempFilename(thePrefs::GetConfigDir() + "nodes.dat.download");

	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(url, strTempFilename, thePrefs::GetConfigDir() + "nodes.dat", HTTP_NodesDat, true, false);
	downloader->Create();
	downloader->Run();
}


void CamuleApp::DisconnectED2K()
{
	// Stop ED2K if it's running
	if (IsConnectedED2K()) {
		serverconnect->Disconnect();
	}
}

bool CamuleApp::CryptoAvailable() const
{
	return clientcredits && clientcredits->CryptoAvailable();
}

uint32 CamuleApp::GetED2KID() const {
	return serverconnect ? serverconnect->GetClientID() : 0;
}

uint32 CamuleApp::GetID() const {
	uint32 ID;

	if( Kademlia::CKademlia::IsConnected() && !Kademlia::CKademlia::IsFirewalled() ) {
		// We trust Kad above ED2K
		ID = ENDIAN_NTOHL(Kademlia::CKademlia::GetIPAddress());
	} else if( theApp->serverconnect->IsConnected() ) {
		ID = theApp->serverconnect->GetClientID();
	} else if ( Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::IsFirewalled() ) {
		// A firewalled Kad client gets a "1"
		ID = 1;
	} else {
		ID = 0;
	}

	return ID;
}

wxDEFINE_EVENT(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, wxEvent);
wxDEFINE_EVENT(wxEVT_CORE_SOURCE_DNS_DONE, wxEvent);
wxDEFINE_EVENT(wxEVT_CORE_UDP_DNS_DONE, wxEvent);
wxDEFINE_EVENT(wxEVT_CORE_SERVER_DNS_DONE, wxEvent);// File_checked_for_headers
