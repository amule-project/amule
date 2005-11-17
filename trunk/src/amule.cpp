//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <cerrno>
#include <cmath>
#include <csignal>
#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <wx/defs.h>
#include <wx/process.h>

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
					//   HAVE_SYS_RESOURCE_H, HAVE_SYS_STATVFS_H and VERSION
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThreadBase
#include "Server.h"			// Needed for GetListName
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "IPFilter.h"			// Needed for CIPFilter
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCreditsList.h"	// Needed for CClientCreditsList
#include "ServerSocket.h"		// Needed for CServerSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "ServerConnect.h"		// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerUDPSocket.h"	// Needed for CServerUDPSocket
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket & CMuleUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "FriendList.h"			// Needed for CFriendList
#include "updownclient.h"		// Needed for CUpDownClient
#include <common/StringFunctions.h>	// Needed for validateURI
#include "Packet.h"
#include "Statistics.h"			// Needed for CStatistics
#include "AICHSyncThread.h"
#include "Logger.h"
#include <common/Format.h>			// Needed for CFormat
#include "UploadBandwidthThrottler.h"
#include "InternalEvents.h"		// Needed for wxMuleInternalEvent
#include "FileFunctions.h"		// Needed for CDirIterator
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"

#ifndef AMULE_DAEMON
	#include <wx/splash.h>			// Needed for wxSplashScreen
	#include <wx/gauge.h>
	#include <wx/textctrl.h>
	#include <wx/clipbrd.h>			// Needed for wxClipBoard	
	#include <wx/msgdlg.h>			// Needed for wxMessageBox

	#ifdef __WXMAC__
		#include <CoreFoundation/CFBundle.h>
		#include <wx/mac/corefoundation/cfstring.h>
	#endif

	#include "TransferWnd.h"		// Needed for CTransferWnd
	#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
	#include "ServerWnd.h"			// Needed for CServerWnd
	#include "StatisticsDlg.h"		// Needed for CStatisticsDlg
#endif


#ifdef HAVE_SYS_RESOURCE_H
	#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
	#include <sys/statvfs.h>
#endif

#ifdef  HAVE_SYS_WAIT_H
	#include <sys/wait.h>
#endif

#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
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
	UnlimitResource(RLIMIT_FSIZE);
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
	
	printf("Shutdown requested, terminating in next event loop.\n");
	
	g_shutdownSignal = true;

#ifdef AMULE_DAEMON
	theApp.ExitMainLoop();
#endif
}


CamuleApp::CamuleApp()
{
	
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	StartTickTimer();
	
	// Initialization	
#if !wxCHECK_VERSION(2,5,1) && defined(__WXGTK20__)
	wxString msg;
	
	msg << wxT("You have attempted to use a version of wxGTK older than\n")
		<< wxT("the 2.5.1 release, compiled against GTK2! This combination is not\n")
		<< wxT("supported by aMule due to many known problems. If you wish to use\n")
		<< wxT("wxGTK compiled against GTK2, please upgrade to a more recent version\n")
		<< wxT("of wxGTK.\n\n")

		<< wxT("More information can be found at:\n")
		<< wxT(" - http://www.amule.org\n")
		<< wxT(" - http://wiki.amule.org\n\n")

		<< wxT("Current version is: aMule ") << GetMuleVersion() << wxT("\n");

	printf("FATAL ERROR! %s\n", (const char*)unicode2char(msg));

	exit(1);
#endif

	m_app_state = APP_STATE_STARTING;
	
	clientlist	= NULL;
	searchlist	= NULL;
	knownfiles	= NULL;
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
	localserver	= NULL;
	glob_prefs	= NULL;
	
	m_dwPublicIP	=  0;

	webserver_pid	= 0;
	
	// Apprently needed for *BSD
	SetResourceLimits();

}

CamuleApp::~CamuleApp()
{
}

int CamuleApp::OnExit()
{
	if (m_app_state!=APP_STATE_STARTING) {
		printf("Now, exiting main app...\n");
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
		printf("Killing amuleweb instance...\n");
		wxKillError rc;
		wxKill(webserver_pid,wxSIGKILL, &rc);
		printf("Killed!\n");
	}


	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule OnExit: Terminating core.\n");
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
	
	delete clientlist;
	clientlist = NULL;
	
	delete uploadqueue;
	uploadqueue = NULL;
	
	delete downloadqueue;
	downloadqueue = NULL;
	
	delete ipfilter;
	ipfilter = NULL;
	
	delete ECServerHandler;
	ECServerHandler = NULL;

	delete statistics;
	statistics = NULL;

	delete glob_prefs;
	glob_prefs = NULL;
	CPreferences::EraseItemList();

	delete localserver;
	localserver = NULL;
	
	delete applog; // deleting a wxFFileOutputStream closes it
	applog = NULL;
	
	delete uploadBandwidthThrottler;
	uploadBandwidthThrottler = NULL;
	
	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown completed.\n");
	}
	
#if wxUSE_MEMORY_TRACING
	printf("Memory debug results for aMule exit:\n");
	// Log mem debug mesages to wxLogStderr
	wxLog* oldLog = wxLog::SetActiveTarget(new wxLogStderr);
	/*
	printf("**************Classes**************\n");
	wxDebugContext::PrintClasses();
	*/
	//printf("***************Dump***************\n");
	//wxDebugContext::Dump();
	printf("***************Stats**************\n");
	wxDebugContext::PrintStatistics(true);

	// Set back to wxLogGui
	delete wxLog::SetActiveTarget(oldLog);
#endif

	StopTickTimer();
	
	// Return 0 for succesful program termination
	return AMULE_APP_BASE::OnExit();
}

int CamuleApp::InitGui(bool ,wxString &)
{
	return 0;
}

//
// Static variables initialization and application initialization
//
const wxString CamuleApp::FullMuleVersion = GetFullMuleVersion();
const wxString CamuleApp::OSDescription = wxGetOsDescription();
char *CamuleApp::strFullMuleVersion = new char[GetFullMuleVersion().Length()+1];
char *CamuleApp::strOSDescription = new char[wxGetOsDescription().Length()+1];

bool CamuleApp::OnInit()
{
	m_localip = StringHosttoUint32(::wxGetFullHostName());

#if wxUSE_MEMORY_TRACING
	printf("Checkpoint set on app init for memory debug\n");
	wxDebugContext::SetCheckpoint();
#endif
	
#ifndef __WXMSW__
	// get rid of sigpipe
	signal(SIGPIPE, SIG_IGN);
	// Handle sigint and sigterm
	signal(SIGINT, OnShutdownSignal);
	signal(SIGTERM, OnShutdownSignal);
#endif

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	// This can't be on constructor or wx2.4.2 doesn't set it.	
	SetVendorName(wxT("TikuWarez"));
	SetAppName(wxT("aMule"));

	ConfigDir = GetConfigDir();
	
	strcpy(strFullMuleVersion, (const char *)unicode2char(FullMuleVersion));
	strcpy(strOSDescription, (const char *)unicode2char(OSDescription));
	OSType = OSDescription.BeforeFirst( wxT(' ') );
	if ( OSType.IsEmpty() ) {
		OSType = wxT("Unknown");
	}	
	
	// Parse cmdline arguments.
	wxCmdLineParser cmdline(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv);

	// Handle these arguments.
	cmdline.AddSwitch(wxT("v"), wxT("version"), wxT("Displays the current version number."));
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("Displays this information."));
	cmdline.AddSwitch(wxT("i"), wxT("enable-stdin"), wxT("Does not disable stdin."));
#ifdef AMULE_DAEMON
	cmdline.AddSwitch(wxT("f"), wxT("full-daemon"), wxT("Fork to background."));
#else
	cmdline.AddOption(wxT("geometry"), wxEmptyString,
		wxT(	"Sets the geometry of the app.\n"
			"\t\t\t<str> uses the same format as standard X11 apps:\n"
			"\t\t\t[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"));
#endif
	cmdline.AddSwitch(wxT("d"), wxT("disable-fatal"), wxT("Does not handle fatal exception."));
	cmdline.AddSwitch(wxT("o"), wxT("log-stdout"), wxT("Print log messages to stdout."));

	// Show help on --help or invalid commands
	if ( cmdline.Parse() ) {
		return false;		
	} else if ( cmdline.Found(wxT("help")) ) {
		cmdline.Usage();
		return false;
	}	

	if ( !cmdline.Found(wxT("disable-fatal")) ) {
#ifndef __WXMSW__
	// catch fatal exceptions
	wxHandleFatalExceptions(true);
#endif
	}

	enable_stdout_log = cmdline.Found(wxT("log-stdout"));
#ifdef AMULE_DAEMON		
	enable_daemon_fork = cmdline.Found(wxT("full-daemon"));
#else
	enable_daemon_fork = false;
#endif	
	if ( enable_stdout_log ) {
		if ( enable_daemon_fork ) {
			printf("Daemon will fork to background - log to stdout disabled\n");
			enable_stdout_log = false;
		} else {
			printf("Logging to stdout enabled\n");
		}
	}
	
	if ( cmdline.Found(wxT("version")) ) {
#ifndef AMULE_DAEMON		
		printf("aMule %s (OS: %s)\n",
			(const char *)unicode2char(GetMuleVersion()),
			(const char*)unicode2char(OSType));
#else
		printf("aMule Daemon %s (OS: %s)\n",
			(const char *)unicode2char(GetMuleVersion()),
			(const char*)unicode2char(OSType));
#endif
		return false;
	}
	
	// Default geometry of the GUI. Can be changed with a cmdline argument...
	bool geometry_enabled = false;
	wxString geom_string;
	#ifndef AMULE_DAEMON
	if ( cmdline.Found(wxT("geometry"), &geom_string) ) {
		geometry_enabled = true;
	}
	#endif


	printf("Initialising aMule\n");

	/* Check for old aMule configs, or for old lmule/xmule config if no aMule configs found. */
	CheckConfig();

	printf("Checking if there is an instance already running...\n");
	// see if there is another instance running
	wxString server =
#if !wxUSE_DDE_FOR_IPC
		ConfigDir +
#endif
		wxT("muleconn");
	wxString host = wxT("localhost");
	wxString IPC = wxT("aMule IPC TESTRUN");

	{
		wxClient client;
		
		// Log to stderr
		wxLog* oldLog = wxLog::SetActiveTarget(new wxLogStderr);
		wxConnectionBase* conn = client.MakeConnection(host, server, IPC);
		delete wxLog::SetActiveTarget(oldLog); // Restore old log
		
		// If the connection failed, conn is NULL
		if ( conn ) {
			// An instance is already running!
			printf("There is an instance of aMule already running\n");
			// This is very tricky. The most secure way to communicate is via ED2K links file
			FILE *ed2kfile;
			char filename[1024];
	
			sprintf(filename,"%s/.aMule/ED2KLinks",(const char*)unicode2char(wxGetHomeDir()));
			ed2kfile = fopen(filename,"a");
			if (ed2kfile != NULL) {
				fprintf(ed2kfile,"RAISE_DIALOG");
				printf("Raised current running aMule\n");
				fclose(ed2kfile);
			} else {
				printf("Error opening file %s.Cannot raise active aMule\n", filename);
			}
			
			conn->Disconnect();
	
			printf("aMule already running: exiting\n");
			delete conn;
			return false;
		}
	}

	// If there was no server, start one
	localserver = new wxServer();
	localserver->Create(server);

	// Close standard-input
	if ( !cmdline.Found(wxT("enable-stdin")) ) {
		close(0);
	}

	// This creates the CFG file we shall use
	wxConfigBase* cfg = new wxFileConfig( wxEmptyString, wxEmptyString, ConfigDir + wxT("amule.conf") );
	
	// Set the config object as the global cfg file
	wxConfig::Set( cfg );


#if wxCHECK_VERSION(2,5,3)
	applog = new wxFFileOutputStream(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
	if ( !applog->Ok() ) {
#else
	applog = new wxFile();
	applog->Create(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"), true);
	if ( !applog->IsOpened() ) {
#endif
		// use std err as last resolt to indicate problem
		fputs("ERROR: unable to open log file\n", stderr);
		delete applog;
		applog = NULL;
		// failure to open log is serious problem
		return false;
	}

	// Load Preferences
	CPreferences::BuildItemList( theApp.ConfigDir);
	CPreferences::LoadAllItems( wxConfigBase::Get() );
	glob_prefs = new CPreferences();

	// Some sanity check
	if (!thePrefs::UseTrayIcon()) {
		thePrefs::SetMinToTray(false);
	}

	// Build the filenames for the two OS files
	SetOSFiles(thePrefs::GetOSDir());

	// Load localization settings
	Localize_mule();

	// Display notification on new version or first run
	wxTextFile vfile( ConfigDir + wxFileName::GetPathSeparator() + wxT("lastversion") );
	wxString newMule(wxT(VERSION));
	
	// Test if there's any new version
	if (thePrefs::CheckNewVersion()) {
		// We use the thread base because I don't want a dialog to pop up.
		CHTTPDownloadThreadBase* version_check = 
			new CHTTPDownloadThreadBase(wxT("http://amule.sourceforge.net/lastversion"),
				theApp.ConfigDir + wxT("last_version_check"), HTTP_VersionCheck);
		version_check->Create();
		version_check->Run();
	}

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

		// We havent run this version before?
		if ( !found ) {
			// Insert new at top to provide faster searches
			vfile.InsertLine( newMule, 0 );
			
			Trigger_New_version( newMule );
		}
		
		// Keep at most 10 entires
		while ( vfile.GetLineCount() > 10 )
			vfile.RemoveLine( vfile.GetLineCount() - 1 );
			
		vfile.Write();
		vfile.Close();
	}

	// Check if we have the old style locale config
	wxString langId = thePrefs::GetLanguageID();
	if (!langId.IsEmpty() && (langId.GetChar(0) >= '0' && langId.GetChar(0) <= '9')) {
		wxString info(_("Your locale has been changed to System Default due to a configuration change. Sorry."));
		thePrefs::SetLanguageID(wxLang2Str(wxLANGUAGE_DEFAULT));
		ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
	}

	statistics = new CStatistics();
	
	// Ready file-hasher
	CAddFileThread::Start();

	clientlist	= new CClientList();
	friendlist	= new CFriendList();
	searchlist	= new CSearchList();
	knownfiles	= new CKnownFileList();
	serverlist	= new CServerList();
	
	sharedfiles	= new CSharedFileList(knownfiles);
	clientcredits	= new CClientCreditsList();
	
	// bugfix - do this before creating the uploadqueue
	downloadqueue	= new CDownloadQueue();
	uploadqueue	= new CUploadQueue();
	ipfilter	= new CIPFilter();

	// Create main dialog
	InitGui(geometry_enabled, geom_string);
	
	uploadBandwidthThrottler = new UploadBandwidthThrottler();

	// Load and update IPFilter.dat files. Loading the
	// existing ipfilter.dat file should be done before
	// loading the server.met, to ensure that servers are
	// filtered.
	AddLogLineM(false, _("Loading ipfilter.dat files."));
	ipfilter->Reload();
	
	serverlist->Init();

	// init downloadqueue
	downloadqueue->LoadMetFiles( thePrefs::GetTempDir() );

	// Creates all needed listening sockets
	wxString msg;
	bool ok;
	ok = ReinitializeNetwork(&msg);
	if (!msg.IsEmpty()) {
		printf("\n%s\n", (const char *)unicode2char(msg));
	}

	// reload shared files
	sharedfiles->Reload(true);

	
	if (thePrefs::IPFilterAutoLoad()) {
		ipfilter->Update(thePrefs::IPFilterURL());
	}	

	
	// Ensure that the up/down ratio is used
	CPreferences::CheckUlDlRatio();

	// The user can start pressing buttons like mad if he feels like it.
	m_app_state = APP_STATE_RUNNING;
	
	// Kry - Load the sources seeds on app init
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->LoadSourceSeeds();
	}
	
	// Autoconnect if that option is enabled
	if (thePrefs::DoAutoConnect() && (thePrefs::GetNetworkED2K() || thePrefs::GetNetworkKademlia())) {
		AddLogLineM(true, _("Connecting"));
		if (thePrefs::GetNetworkED2K()) {
			theApp.serverconnect->ConnectToAnyServer();
		}

		StartKad();

	}

	// No webserver on Win at all (yet)
#ifndef __WXMSW__
	// Run webserver?
	if (thePrefs::GetWSIsEnabled()) {
		wxString aMuleConfigFile(ConfigDir + wxT("amule.conf"));
#ifdef AMULE_DAEMON
		int pid = fork();
		if ( pid == -1 ) {
			printf("ERROR: fork failed with code %d\n", errno);
		} else {
			if ( pid == 0 ) {
				execlp("amuleweb", "amuleweb", (const char *)unicode2char(wxT("--amule-config-file=") + aMuleConfigFile), NULL);
				printf("execlp failed with code %d\n", errno);
				exit(0);
			} else {
				webserver_pid = pid;
			}
		}
#else
		wxString amulewebPath = wxT("amuleweb");

#ifdef __WXMAC__
		// For the Mac GUI application, look for amuleweb in the bundle
		CFURLRef amulewebUrl = CFBundleCopyAuxiliaryExecutableURL(
			CFBundleGetMainBundle(), CFSTR("amuleweb"));

		if (amulewebUrl) {
			CFURLRef absoluteUrl = CFURLCopyAbsoluteURL(amulewebUrl);
			CFRelease(amulewebUrl);

			if (absoluteUrl) {
				CFStringRef amulewebCfstr = CFURLCopyFileSystemPath(absoluteUrl, kCFURLPOSIXPathStyle);
				CFRelease(absoluteUrl);
				amulewebPath = wxMacCFStringHolder(amulewebCfstr).AsString(wxLocale::GetSystemEncoding());
			}
		}
#endif

		webserver_pid = wxExecute(wxT("'") + amulewebPath + wxT("' '--amule-config-file=") + aMuleConfigFile + wxT("'"));
#endif
		// give amuleweb chance to start or forked child to exit
		// 1 second if enough time to fail on "path not found"
		wxSleep(1);
		int status, result;
//#ifdef HAVE_SYS_WAIT_H
		if ( (result = waitpid(webserver_pid, &status, WNOHANG)) == -1 ) {
			printf("ERROR: waitpid call failed\n");
		} else {
			if ( status && WIFEXITED(status) ) {
				webserver_pid = 0;
			}
		}
//#else
//#warning wtf to do here?
//#endif
		if (webserver_pid) {
			AddLogLineM(true, CFormat(_("webserver running on pid %d")) % webserver_pid);
		} else {
			ShowAlert(_(
				"You requested to run webserver from startup, "
				"but the amuleweb binary cannot be run. "
				"Please install the package containing aMule webserver, "
				"or compile aMule using --enable-webserver and run make install"),
				_("Error"), wxOK | wxICON_ERROR);
		}
	}
#endif /* ! __WXMSW__ */

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
		
		wxString err = wxT("Network configuration failed! You cannot use the same port\n"
					"for the main TCP port and the External Connections port.\n"
					"The EC port has been changed to avoid conflict, see the\n"
					"preferences for the new value.\n");

		*msg << err;

		AddLogLineM( false, wxEmptyString );
		AddLogLineM( true, err );
		AddLogLineM( false, wxEmptyString );

		ok = false;
	}
	
	if (thePrefs::GetUDPPort() == thePrefs::GetPort() + 3) {
		// Select a random usable value in the range 1025 ... 2^16 - 1
		uint16 port = thePrefs::GetUDPPort();
		while ( port < 1024 || port == thePrefs::GetPort() + 3 ) {
			port = (uint16)rand();
		}

		thePrefs::SetUDPPort( port );

		wxString err = wxT("Network configuration failed! You can't use the port which\n"
					"has the value of the main TCP port plus 3 for the UDP port.\n"
					"This port has been reserved for the Server-UDP port. The\n"
					"port value has been changed to avoid conflict, see the\n"
					"preferences for the new value\n");

		*msg << err;

		AddLogLineM( false, wxEmptyString );
		AddLogLineM( true, err );
		AddLogLineM( false, wxEmptyString );
		
		ok = false;
	}
	
	// Create the address where we are going to listen
	// TODO: read this from configuration file
	amuleIPV4Address myaddr;
	//myaddr.AnyAddress();
	if (thePrefs::GetECAddress().IsEmpty() || !myaddr.Hostname(thePrefs::GetECAddress())) {
		myaddr.AnyAddress();
	}
	myaddr.Service(thePrefs::ECPort());
	
	// Get ready to handle connections from apps like amulecmd
	ECServerHandler = new ExternalConn(myaddr, msg);

	if (thePrefs::GetAddress().IsEmpty() || !myaddr.Hostname(thePrefs::GetAddress())) {
		myaddr.AnyAddress();
	}
	wxString ip = myaddr.IPAddress();
	// Creates the UDP socket TCP+3.
	// Used for source asking on servers.
	myaddr.Service(thePrefs::GetPort()+3);
	serverconnect = new CServerConnect(serverlist, myaddr);

	*msg << CFormat( wxT("*** Server UDP socket (TCP+3) at %s:%u\n") )
		% ip % ((unsigned int)thePrefs::GetPort() + 3u);
	
	// Create the ListenSocket (aMule TCP socket).
	// Used for Client Port / Connections from other clients,
	// Client to Client Source Exchange.
	// Default is 4662.
	*msg << CFormat( wxT("*** TCP socket (TCP) listening on %s:%u\n") )
		% ip % (unsigned int)(thePrefs::GetPort());
	
	myaddr.Service(thePrefs::GetPort());
	listensocket = new CListenSocket(myaddr);
	
	// This command just sets a flag to control maximun number of connections.
	// Notify(true) has already been called to the ListenSocket, so events may
	// be already comming in.
	if (listensocket->Ok()) {
		listensocket->StartListening();
	} else {
		// If we wern't able to start listening, we need to warn the user
		wxString err;
		err = CFormat(_("Port %u is not available. You will be LOWID\n")) % (unsigned int)(thePrefs::GetPort());
		*msg << err;
		AddLogLineM(true, err);
		err.Clear();
		err = CFormat(_("Port %u is not available!\n\n"
				"This means that you will be LOWID.\n\n"
				"Check your network to make sure the port is open for output and input.")) % 
				(unsigned int)(thePrefs::GetPort());
		ShowAlert(err, _("Error"), wxOK | wxICON_ERROR);
	}

	// Create the UDP socket.
	// Used for extended eMule protocol, Queue Rating, File Reask Ping.
	// Default is port 4672.
	myaddr.Service(thePrefs::GetUDPPort());
	clientudp = new CClientUDPSocket(myaddr, thePrefs::GetProxyData());
	
	if (!thePrefs::IsUDPDisabled()) {
		*msg << CFormat( wxT("*** Client UDP socket (extended eMule) at %s:%u") )
			% ip % thePrefs::GetUDPPort();
	} else {
		*msg << wxT("*** Client UDP socket (extended eMule) disabled on preferences");
	}	
	
	return ok;
}


// Returns a ed2k file URL
wxString CamuleApp::CreateED2kLink(const CAbstractFile *f)
{
	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	wxString strURL	= wxString(wxT("ed2k://|file|")) <<
		CleanupFilename(f->GetFileName(), true) << wxT("|") <<
		f->GetFileSize() << wxT("|") <<
		f->GetFileHash().Encode() << wxT("|/");
	return strURL;
}

// Returns a ed2k source URL
wxString CamuleApp::CreateED2kSourceLink(const CAbstractFile *f)
{
	if ( !IsConnectedED2K() || serverconnect->IsLowID() ) {
		return wxEmptyString;
	}
	uint32 clientID = GetED2KID();
	// Create the first part of the URL
	// And append the source information: "|sources,<ip>:<port>|/"
	wxString strURL = CreateED2kLink(f) <<
		wxT("|sources,") <<
		(uint8) clientID << wxT(".") <<
		(uint8)(clientID >> 8) << wxT(".") <<
		(uint8)(clientID >> 16) << wxT(".") <<
		(uint8)(clientID >> 24) << wxT(":") <<
		thePrefs::GetPort() << wxT("|/");
	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,<ip>:<port>|/"
	return strURL;
}

// Returns a ed2k link with AICH info if available
wxString CamuleApp::CreateED2kAICHLink(const CKnownFile* f)
{
	// Create the first part of the URL
	wxString strURL = CreateED2kLink(f);
	// Append the AICH info
	if (f->GetAICHHashset()->HasValidMasterHash() && 
		(
	      f->GetAICHHashset()->GetStatus() == AICH_VERIFIED || 
		 f->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
	     )) {
		strURL << wxT("|h=") << f->GetAICHHashset()->GetMasterHash().GetString();
	}	

	strURL << wxT("|/");
	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|h=<AICH master hash>|/"
	return strURL;
}

// Returns a ed2k source URL using a hostname rather than IP. Currently, the
// hostname doesn't appear to be set, thus this function wont work as intended.
wxString CamuleApp::CreateED2kHostnameSourceLink(const CAbstractFile* f)
{
	// Create the first part of the URL
	// Append the source information: "|sources,<host>:port|/"
	wxString strURL = CreateED2kLink(f) <<
		wxT("|sources,") <<
		thePrefs::GetYourHostname() << wxT(":") <<
		thePrefs::GetPort() << wxT("|/");
	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,<host>:<port>|/"
	return strURL;
}

// Creates a ED2k hyperlink
wxString CamuleApp::CreateHTMLED2kLink(const CAbstractFile* f)
{
	wxString strCode = wxT("<a href=\"") + 
		CreateED2kLink(f) + wxT("\">") + 
		CleanupFilename(f->GetFileName(), true) + wxT("</a>");
	return strCode;
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
		AddLogLineM(true, _("Failed to create OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	if ( !amulesig_out.Create(m_amulesig_path) ) {
		AddLogLineM(true, _("Failed to create aMule OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	wxString emulesig_string;

	if (zero) {
		emulesig_string = wxT("0\xA0.0|0.0|0");
		amulesig_out.AddLine(wxT("0\n0\n0\n0\n0\n0.0\n0.0\n0\n0"));
	} else {
		if (IsConnectedED2K()) {

			// We are online
			emulesig_string =
				// Connected
				wxT("1|")
				//Server name
				+ serverconnect->GetCurrentServer()->GetListName()
				+ wxT("|")
				// IP and port of the server
				+ serverconnect->GetCurrentServer()->GetFullIP()
				+ wxT("|")
				+ wxString::Format(wxT("%d"),serverconnect->GetCurrentServer()->GetPort());


			// Now for amule sig

			// Connected. State 1, full info
			amulesig_out.AddLine(wxT("1"));
			// Server Name
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetListName());
			// Server IP
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetFullIP());
			// Server Port
			amulesig_out.AddLine(wxString::Format(wxT("%d"),serverconnect->GetCurrentServer()->GetPort()));

			if (serverconnect->IsLowID()) {
				amulesig_out.AddLine(wxT("L"));
			} else {
				amulesig_out.AddLine(wxT("H"));
			}

		} else if (serverconnect->IsConnecting()) {
			emulesig_string = wxT("0");

			// Connecting. State 2, No info.
			amulesig_out.AddLine(wxT("2\n0\n0\n0\n0"));
		} else {
			// Not connected to a server
			emulesig_string = wxT("0");

			// Not connected, state 0, no info
			amulesig_out.AddLine(wxT("0\n0\n0\n0\n0"));
		}

		emulesig_string += wxT("\xA");

		wxString temp;

		// Datarate for downloads
		temp = wxString::Format(wxT("%.1f"), theStats::GetDownloadRate() / 1024.0);

		emulesig_string += temp + wxT("|");
		amulesig_out.AddLine(temp);

		// Datarate for uploads
		temp = wxString::Format(wxT("%.1f"), theStats::GetUploadRate() / 1024.0);

		emulesig_string += temp + wxT("|");
		amulesig_out.AddLine(temp);

		// Number of users waiting for upload
		temp = wxString::Format(wxT("%d"), theStats::GetWaitingUserCount());

		emulesig_string += temp; 
		amulesig_out.AddLine(temp);

		// Number of shared files (not on eMule)
		amulesig_out.AddLine(wxString::Format(wxT("%d"), theStats::GetSharedFileCount()));
	}

	// eMule signature finished here. Write the line to the wxTextFile.
	emulesig_out.AddLine(emulesig_string);

	// Now for aMule signature extras

	// Nick on the network
	amulesig_out.AddLine(thePrefs::GetUserNick());

	// Total received in bytes
	amulesig_out.AddLine( CFormat( wxT("%llu") ) % (theStats::GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded()) );

	// Total sent in bytes
	amulesig_out.AddLine( CFormat( wxT("%llu") ) % (theStats::GetSessionSentBytes() + thePrefs::GetTotalUploaded()) );

	// amule version
#ifdef CVSDATE
	amulesig_out.AddLine(wxT(VERSION " " CVSDATE));
#else
	amulesig_out.AddLine(wxT(VERSION));
#endif

	if (zero) {
		amulesig_out.AddLine(wxT("0"));
		amulesig_out.AddLine(wxT("0"));
		amulesig_out.AddLine(wxT("0"));
	} else {
        // Total received bytes in session
		amulesig_out.AddLine( CFormat( wxT("%llu") ) %
			theStats::GetSessionReceivedBytes() );

        // Total sent bytes in session
		amulesig_out.AddLine( CFormat( wxT("%llu") ) %
			theStats::GetSessionSentBytes() );

		// Uptime
		amulesig_out.AddLine(wxString::Format(wxT("%llu"), theStats::GetUptimeSeconds()));
	}

	// Flush the files
	emulesig_out.Write();
	amulesig_out.Write();
} //End Added By Bouc7


// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
	/* Print the backtrace */
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");	
	fprintf(stderr, "A fatal error has occurred and aMule has crashed.\n");
	fprintf(stderr, "Please assist us in fixing this problem by posting the backtrace below in our\n");
	fprintf(stderr, "'aMule Crashes' forum and include as much information as possible regarding the\n");
	fprintf(stderr, "circumstances of this crash. The forum is located here:\n");
	fprintf(stderr, "    http://forum.amule.org/board.php?boardid=67\n");
	fprintf(stderr, "If possible, please try to generate a real backtrace of this crash:\n");
	fprintf(stderr, "    http://www.amule.org/wiki/index.php/Backtraces\n\n");
	fprintf(stderr, "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n");
	fprintf(stderr, "Current version is: %s\n", strFullMuleVersion);
	fprintf(stderr, "Running on: %s\n\n", strOSDescription);
	
	print_backtrace(1); // 1 == skip this function.
	
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");	
}


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));
	if (!m_locale.IsOk()) {
		AddLogLineM(false,_("The selected locale seems not to be installed on your box."
				    " (Note: I'll try to set it anyway)"));
	}
}


// Displays information related to important changes in aMule.
// Is called when the user runs a new version of aMule
void CamuleApp::Trigger_New_version(wxString new_version)
{
	wxString info = wxT(" --- ") + CFormat(_("This is the first time you run aMule %s")) % new_version + wxT(" ---\n\n");
	if (new_version == wxT("CVS")) {
		info += _("This version is a testing version, updated daily, and\n");
		info += _("we give no warranty it won't break anything, burn your house,\n");
		info += _("or kill your dog. But it *should* be safe to use anyway.\n");
	} 
	
	// General info
	info += wxT("\n");
	info += _("More information, support and new releases can found at our homepage,\n");
	info += _("at www.aMule.org, or in our IRC channel #aMule at irc.freenode.net.\n");
	info += wxT("\n");
	info += _("Feel free to report any bugs to http://forum.amule.org");

	ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
}


void CamuleApp::SetOSFiles(const wxString new_path)
{
	if ( thePrefs::IsOnlineSignatureEnabled() ) {
		if ( ::wxDirExists(new_path) ) {
			m_emulesig_path = new_path + wxFileName::GetPathSeparator() + wxT("onlinesig.dat");
			m_amulesig_path = new_path + wxFileName::GetPathSeparator() + wxT("amulesig.dat");
		} else {
			ShowAlert(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("Error"), wxOK | wxICON_ERROR);
			m_emulesig_path.Clear();
			m_amulesig_path.Clear();
		}
	} else {
		m_emulesig_path.Clear();
		m_amulesig_path.Clear();
	}
}


#ifdef __WXDEBUG__
#ifndef wxUSE_STACKWALKER
#define wxUSE_STACKWALKER 0
#endif
void CamuleApp::OnAssert(const wxChar *file, int line, 
						 const wxChar *cond, const wxChar *msg)
{
	if (!wxUSE_STACKWALKER || !wxThread::IsMain() || !IsRunning()) {
		wxString errmsg = CFormat( wxT("%s:%d: Assertion '%s' failed. %s") )
			% file % line % cond % ( msg ? msg : wxT("") );

		fprintf(stderr, "Assertion failed: %s\n", (const char*)unicode2char(errmsg));
		
		// Skip the function-calls directly related to the assert call.
		fprintf(stderr, "\nBacktrace follows:\n");
		print_backtrace(3);
		fprintf(stderr, "\n");
	}
		
	if (wxThread::IsMain() && IsRunning()) {
		AMULE_APP_BASE::OnAssert(file, line, cond, msg);
	} else {	
		// Abort, allows gdb to catch the assertion
		raise( SIGABRT );
	}
}
#endif


void CamuleApp::OnUDPDnsDone(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);
	CServerUDPSocket* socket =(CServerUDPSocket*)evt.GetClientData();	
	socket->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnSourceDnsDone(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);	
	downloadqueue->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnServerDnsDone(wxEvent& e)
{
	printf("Server hostname notified\n");
	wxMuleInternalEvent&	evt = *((wxMuleInternalEvent*)&e);	
	CServerSocket* socket=(CServerSocket*)evt.GetClientData();	
	socket->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnNotifyEvent(wxEvent& e)
{
	GUIEvent& evt = *((GUIEvent*)&e);
	NotifyEvent(evt);
}


void CamuleApp::OnTCPTimer(AMULE_TIMER_EVENT_CLASS& WXUNUSED(evt))
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


void CamuleApp::OnCoreTimer(AMULE_TIMER_EVENT_CLASS& WXUNUSED(evt))
{
	// Former TimerProc section
	static uint64	msPrev1, msPrev5, msPrevSave, msPrevHist, msPrevOS, msPrevKnownMet;
	uint64 msCur = theStats::GetUptimeMillis();

	if (!IsRunning()) {
		return;
	}

#ifndef AMULE_DAEMON
	// Check if we should terminate the app
	if ( g_shutdownSignal ) {
		wxWindow* top = GetTopWindow();

		if ( top ) {
			top->Close(true);
		} else {
			// No top-window, have to force termination.
			wxExit();
		}
	}
#endif

	CLogger::FlushPendingEntries();
	
	uploadqueue->Process();
	downloadqueue->Process();
	//theApp.clientcredits->Process();
	theStats::CalculateRates();

	if (msCur-msPrevHist > 1000) {
		// unlike the other loop counters in this function this one will sometimes
		// produce two calls in quick succession (if there was a gap of more than one
		// second between calls to TimerProc) - this is intentional!  This way the
		// history list keeps an average of one node per second and gets thinned out
		// correctly as time progresses.
		msPrevHist += 1000;
		
		statistics->RecordHistory();
		
	}
	
	
	if (msCur-msPrev1 > 1000) {  // approximately every second
		msPrev1 = msCur;
		clientcredits->Process();
		clientlist->Process();
		
		// Publish files to server if needed.
		sharedfiles->Process();
		
		if( Kademlia::CKademlia::isRunning() ) {
			Kademlia::CKademlia::process();
			if(Kademlia::CKademlia::getPrefs()->hasLostConnection()) {
				StopKad();
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
		wxString buffer;
		
		// Save total upload/download to preferences
		wxConfigBase* cfg = wxConfigBase::Get();
		buffer = wxString::Format( wxT("%llu"), theStats::GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded() );
		cfg->Write(wxT("/Statistics/TotalDownloadedBytes"), buffer);

		buffer = wxString::Format( wxT("%llu"), theStats::GetSessionSentBytes() + thePrefs::GetTotalUploaded() );
		cfg->Write(wxT("/Statistics/TotalUploadedBytes"), buffer);

		// Write changes to file
		cfg->Flush();

	}

	// Special
	if (msCur-msPrevOS >= thePrefs::GetOSUpdate()) {
		OnlineSig(); // Added By Bouc7		
		msPrevOS = msCur;
	}
	
	if (msCur - msPrevKnownMet >= 30*60*1000/*There must be a prefs option for this*/) {
		// Save Shared Files data
		knownfiles->Save();
		msPrevKnownMet = msCur;
	}

	
	// Recomended by lugdunummaster himself - from emule 0.30c
	serverconnect->KeepConnectionAlive();

}


void CamuleApp::OnHashingShutdown(wxEvent& WXUNUSED(evt))
{
	if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
		// Save the known.met file
		knownfiles->Save();
		
		// Known.met changed, AICH sync thread start
		RunAICHThread();
	} 
}


void CamuleApp::OnFinishedHashing(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);
	static int filecount;
	static int bytecount;

	CKnownFile* result = (CKnownFile*)evt.GetClientData();
	if (evt.GetExtraLong()) {
		CPartFile* requester = (CPartFile*)evt.GetExtraLong();
		if (downloadqueue->IsPartFile(requester)) {
			requester->PartFileHashFinished(result);
		}
	} else {
		if (knownfiles->SafeAddKFile(result)) {
			AddDebugLogLineM(false, logKnownFiles, wxT("Safe adding file to sharedlist: ") + result->GetFileName());			
			sharedfiles->SafeAddKFile(result);

			filecount++;
			bytecount += result->GetFileSize();
			// If we have added 30 files or files with a total size of ~300mb
			if ( ( filecount == 30 ) || ( bytecount >= 314572800 ) ) {
				AddDebugLogLineM(false, logKnownFiles, wxT("Failsafe for crash on file hashing creation"));
				if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
					knownfiles->Save();
					filecount = 0;
					bytecount = 0;
				}
			}
		} else {
			AddDebugLogLineM(false, logKnownFiles, wxT("File not added to sharedlist: ") + result->GetFileName());
			delete result;
		}
	}
}


void CamuleApp::OnFinishedCompletion(wxEvent& e)
{
	wxMuleInternalEvent& evt = dynamic_cast<wxMuleInternalEvent&>(e);
	CPartFile* completed = (CPartFile*)evt.GetClientData();
	wxCHECK_RET(completed, wxT("Completion event sent for unspecified file"));
	completed->CompleteFileEnded(evt.GetInt(), (wxString*)evt.GetExtraLong());

	// Check if we should execute an script/app/whatever.
	if (thePrefs::CommandOnCompletion()) {
		wxString command = thePrefs::GetCommandOnCompletion();

		command.Replace(wxT("%FILE"), completed->GetFullName());
		command.Replace(wxT("%HASH"), completed->GetFileHash().Encode());

		if (::wxExecute(command, wxEXEC_ASYNC) == 0) {
			AddLogLineM(true, CFormat(_("Failed to execute on-completion command. Template is: %s")) % command);
		}
	}	
}


void CamuleApp::ShutDown()
{
	// Signal the hashing thread to terminate
	m_app_state = APP_STATE_SHUTINGDOWN;
	
	StopKad();
	
	// Kry - Save the sources seeds on app exit
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->SaveSourceSeeds();
	}

	OnlineSig(true); // Added By Bouc7

	// Close sockets to avoid new clients coming in
	if (listensocket) {
		listensocket->Close();
		listensocket->KillAllSockets();	
	}
	
	if (serverconnect) {
		serverconnect->Disconnect();
	}

	// saving data & stuff
	if (knownfiles) {
		knownfiles->Save();
	}

	thePrefs::Add2TotalDownloaded(theStats::GetSessionReceivedBytes());
	thePrefs::Add2TotalUploaded(theStats::GetSessionSentBytes());

	if (glob_prefs) {
		glob_prefs->Save();
	}

	if (clientlist) {
		clientlist->DeleteAll();
	}
	if (CAddFileThread::IsRunning()) {
		CAddFileThread::Stop();
	}
	if (CAICHSyncThread::IsRunning()) {
		CAICHSyncThread::Stop();
	}

    theApp.uploadBandwidthThrottler->EndThread();
}

bool CamuleApp::AddServer(CServer *srv, bool fromUser)
{
	if ( serverlist->AddServer(srv, fromUser) ) {
		Notify_ServerAdd(srv);
		return true;
	}
	return false;
}

//
// Kry Yay, unicoding via streams
#if wxCHECK_VERSION(2,5,3)
	#include <wx/sstream.h>	
#endif
void CamuleApp::AddLogLine(const wxString &msg)
{
	// At most one trailing new-line, which we add
	wxString message = msg;
	while ( !message.IsEmpty() && message.Last() == wxT('\n') ) {
		message.RemoveLast();
	}
	
	wxString full_line = wxDateTime::Now().FormatISODate() + wxT(" ") + 
		wxDateTime::Now().FormatISOTime() + wxT(": ") + message + wxT("\n");
	
#if wxCHECK_VERSION(2,5,3)
	wxStringInputStream stream(full_line);
	
	(*applog) << stream;
	applog->Sync();
#else
	applog->Write(full_line);
	applog->Flush();
#endif
	
	if (enable_stdout_log) { 
		printf("%s", (const char*)unicode2char(full_line));
	}
}


uint32 CamuleApp::GetPublicIP(bool ignorelocal) const
{
	if (m_dwPublicIP == 0) {
		if (Kademlia::CKademlia::isConnected() && Kademlia::CKademlia::getIPAddress() ) {
			return wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::getIPAddress());
		} else {
			return ignorelocal ? 0 : m_localip;
		}
	}
	
	return m_dwPublicIP;	
}


void CamuleApp::SetPublicIP(const uint32 dwIP)
{
	wxASSERT((dwIP == 0) or !IsLowID(dwIP));
	
	m_dwPublicIP = dwIP;
}


wxString CamuleApp::GetLog(bool reset)
{
	ConfigDir = GetConfigDir();
	wxFile logfile;
	logfile.Open(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
	if ( !logfile.IsOpened() ) {
		return wxTRANSLATE("ERROR: can't open logfile");
	}
	int len = logfile.Length();
	if ( len == 0 ) {
		return wxTRANSLATE("WARNING: logfile is empty. Something is wrong.");
	}
	char *tmp_buffer = new char[len + sizeof(wxChar)];
	logfile.Read(tmp_buffer, len);
	memset(tmp_buffer + len, 0, sizeof(wxChar));
#if wxUSE_UNICODE
	// try to guess file format
	wxString str;
	if (tmp_buffer[0] && tmp_buffer[1]) {
		str = wxString(UTF82unicode(tmp_buffer));
	} else {
		str = wxString((wxWCharBuffer&)tmp_buffer);
	}
#else
	wxString str(tmp_buffer);
#endif
	delete [] tmp_buffer;
	if ( reset ) {
#if wxCHECK_VERSION(2,5,3)
		delete applog;
		applog = new wxFFileOutputStream(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
		if ( applog->Ok() ) {
#else
		applog->Close();
		applog->Create(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"), true);
		if ( applog->IsOpened() ) {
#endif
			AddLogLine(_("Log has been reset"));
		} else {
			delete applog;
			applog = 0;
		}
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
	server_msg += msg + wxT("\n");
	AddLogLine(CFormat(_("ServerMessage: %s")) % msg);
}


void CamuleApp::RunAICHThread()
{
	if ( !CAICHSyncThread::IsRunning() )
		CAICHSyncThread::Start();
}

void CamuleApp::OnFinishedHTTPDownload(wxEvent& evt)
{
	wxMuleInternalEvent& event = *((wxMuleInternalEvent*)&evt);
	switch (event.GetInt()) {
		case HTTP_IPFilter:
			ipfilter->DownloadFinished(event.GetExtraLong());
			break;
		case HTTP_ServerMet:
			serverlist->DownloadFinished(event.GetExtraLong());
			break;
		case HTTP_ServerMetAuto:
			serverlist->AutoDownloadFinished(event.GetExtraLong());
			break;
		case HTTP_VersionCheck:
			CheckNewVersion(event.GetExtraLong());
			break;
		case HTTP_NodesDat:
			if (event.GetExtraLong() != -1) {
				
				wxString file = ConfigDir + wxT("nodes.dat");
				if (wxFileExists(file)) {
					wxRemoveFile(file);
				}
				wxRenameFile(file + wxT(".download"),file);
				if ( Kademlia::CKademlia::isRunning() ) {
					Kademlia::CKademlia::stop();
				}
				
				Kademlia::CKademlia::start();
				theApp.ShowConnectionState();
				
			} else {
				AddLogLineM(true, _("Failed to download the nodes list."));
			}
			break;
	}
}

void CamuleApp::CheckNewVersion(uint32 result)
{	
	if (result == 1) {
		wxString filename = theApp.ConfigDir + wxT("last_version_check");
		wxTextFile file;
		
		if (!file.Open(filename)) {
			AddLogLineM(true, _("Failed to open the downloaded version check file") );
			return;
		} else if (!file.GetLineCount()) {
			AddLogLineM(true, _("Corrupted version check file"));
		} else {
			wxString versionLine = file.GetFirstLine();
			wxStringTokenizer tkz(versionLine, wxT("."));
			
			AddDebugLogLineM(false, logGeneral, wxString(wxT("Running: ")) + wxT(VERSION) + wxT(", Version check: ") + versionLine);
			
			long fields[] = {0, 0, 0};
			for (int i = 0; i < 3; ++i) {
				if (!tkz.HasMoreTokens()) {
					AddLogLineM(true, _("Corrupted version check file"));
					return;
				} else {
					wxString token = tkz.GetNextToken();
					
					if (!token.ToLong(&fields[i])) {
						AddLogLineM(true, _("Corrupted version check file"));
						return;
					}
				}
			}

			long curVer = make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE);
			long newVer = make_full_ed2k_version(fields[0], fields[1], fields[2]);
			
			if (curVer < newVer) {
				AddLogLineM(true, _("You are using an outdated version of aMule!"));
				AddLogLineM(false, wxString::Format(_("Your aMule version is %i.%i.%i and the latest version is %i.%i.%i"), VERSION_MJR, VERSION_MIN, VERSION_UPDATE, fields[0], fields[1], fields[2]));
				AddLogLineM(false, _("The latest version can always be found at http://www.amule.org"));
				#ifdef AMULE_DAEMON
				printf(unicode2UTF8(wxString::Format(_("WARNING: Your aMuled version is outdated: %i.%i.%i < %i.%i.%i\n"), VERSION_MJR, VERSION_MIN, VERSION_UPDATE, fields[0], fields[1], fields[2])));
				#endif
			} else {
				AddLogLineM(false, _("Your copy of aMule is up to date."));
			}
		}
		
		file.Close();
		wxRemoveFile(filename);
	} else {
		AddLogLineM(true, _("Failed to download the version check file") );
	}	
	
}


bool CamuleApp::IsConnected()
{
	return (IsConnectedED2K() || IsConnectedKad());
}


bool CamuleApp::IsConnectedED2K()
{
	return serverconnect && serverconnect->IsConnected();
}


bool CamuleApp::IsConnectedKad()
{
	return Kademlia::CKademlia::isConnected(); 
}


bool CamuleApp::IsFirewalled()
{
	if (theApp.IsConnectedED2K() && !theApp.serverconnect->IsLowID()) {
		return false; // we have an eD2K HighID -> not firewalled
	}

	if (Kademlia::CKademlia::isConnected() && !Kademlia::CKademlia::isFirewalled()) {
		return false; // we have an Kad HighID -> not firewalled
	}

	return true; // firewalled
	
}

bool CamuleApp::DoCallback( CUpDownClient *client )
{
	if(Kademlia::CKademlia::isConnected()) {
		if(IsConnectedED2K()) {
			if(serverconnect->IsLowID()) {
				if(Kademlia::CKademlia::isFirewalled()) {
					//Both Connected - Both Firewalled
					return false;
				} else {
					if(client->GetServerIP() == theApp.serverconnect->GetCurrentServer()->GetIP() &&
					   client->GetServerPort() == theApp.serverconnect->GetCurrentServer()->GetPort()) {
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
			if(Kademlia::CKademlia::isFirewalled()) {
				//Only Kad Connected - Kad Firewalled
				return false;
			} else {
				//Only Kad Conected - Kad Open
				return true;
			}
		}
	} else {
		if( IsConnectedED2K() ) {
			if( serverconnect->IsLowID() ) {
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
	
	theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	
	wxString buffer = 
		CFormat(_("Users: E: %s K: %s | Files E: %s K: %s")) % CastItoIShort(totaluser) % 
		CastItoIShort(Kademlia::CKademlia::getKademliaUsers()) % CastItoIShort(totalfile) % CastItoIShort(Kademlia::CKademlia::getKademliaFiles());
	
	Notify_ShowUserCount(buffer);
}

void CamuleApp::ListenSocketHandler(wxSocketEvent& event)
{
	//
	// There is only one ListenSocket in the whole application,
	// so there is no need to discover on the fly with:
	//
	// CListenSocket *socket = (CListenSocket *) event.GetSocket();
	//
	// Also, now with proxy, CListenSocket is no longer derived from
	// wxSocketServer, so event.GetSocket() is actually
	// theApp.listensocket->m_SocketServer
	//
	CListenSocket *socket = theApp.listensocket;
	if(!socket) {
		// This should never happen, anyway, there is nothing to do.
		wxASSERT(0);
		return;
	}
	
	if (!IsRunning()) {
		// Even if we are not ready to start listening, we must
		// accept the connection, otherwise no other connection
		// events will happen. So we Accept() it and destroy the
		// socket imediately.
		wxSocketBase *s = socket->Accept(false);
		s->Destroy();
		// Kry - Woops, we don't want to accept a destroying socket
		return;
	}
	
	switch(event.GetSocketEvent()) {
		case wxSOCKET_CONNECTION:
			socket->OnAccept(0);
			break;
		default:
			// shouldn't get other than connection events...
			wxASSERT(0);
			break;
	}
}

void CamuleApp::ShowConnectionState() {
	static uint8 old_state = (1<<7); // This flag doesn't exist
	
	uint8 state = 0;
	
	if (theApp.serverconnect->IsConnected()) {
		state |= CONNECTED_ED2K;
	}
	
	if (Kademlia::CKademlia::isRunning()) {
		if (!Kademlia::CKademlia::isFirewalled()) {
			state |= CONNECTED_KAD_OK;
		} else {
			state |= CONNECTED_KAD_FIREWALLED;
		}
	}
	
	Notify_ShowConnState(state);
	
	if (old_state != state) {
				
		// Get the changed value 
		
		int changed_flags = old_state ^ state;
		
		if (changed_flags & CONNECTED_ED2K) {
			// ED2K status changed
			wxString connected_server;
			CServer* ed2k_server = theApp.serverconnect->GetCurrentServer();
			if (ed2k_server) {
				connected_server = ed2k_server->GetListName();
			}
			if (state & CONNECTED_ED2K) {
				// We connected to some server
				const wxString id = theApp.serverconnect->IsLowID() ? _("with LowID") : _("with HighID");

				AddLogLine(CFormat(_("Connected to %s %s")) % connected_server % id);
			} else {
				if ( theApp.serverconnect->IsConnecting() ) {
					AddLogLine(CFormat(_("Connecting to %s")) % connected_server);
				} else {
					AddLogLine(_("Disconnected from ED2K"));
				}
			}
		}
		
		if (changed_flags & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
			if (state & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
				if (state & CONNECTED_KAD_OK) {
					AddLogLine(_("Connected to Kad (ok)"));
				} else {
					AddLogLine(_("Connected to Kad (firewalled)"));
				}
			} else {
				AddLogLine(_("Disconnected from Kad"));
			}
		}
		
		old_state = state;
	}
	
	ShowUserCount();
}


void CamuleApp::UDPSocketHandler(wxSocketEvent& event)
{
	CMuleUDPSocket* socket = (CMuleUDPSocket*)(event.GetClientData());
	wxCHECK_RET(socket, wxT("No socket owner specified."));
	
	if (!IsRunning() && !IsOnShutDown()) {
		if (event.GetSocketEvent() == wxSOCKET_INPUT) {
			// Back to the queue!
			theApp.AddPendingEvent(event);
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
			
		default:
			wxASSERT(0);
			break;
	}
}


void CamuleApp::OnUnhandledException()
{
	// Call the generic exception-handler.
	fprintf(stderr, "\taMule Version: %s\n", (const char*)unicode2char(GetFullMuleVersion()));	
	::OnUnhandledException();
}

void CamuleApp::StartKad()
{
	if (!Kademlia::CKademlia::isRunning() && thePrefs::GetNetworkKademlia()) {
		// Kad makes no sense without the Client-UDP socket.
		if (!thePrefs::IsUDPDisabled()) {
			Kademlia::CKademlia::start();
		}
	}
}

void CamuleApp::StopKad()
{
	// Stop Kad if it's running
	if (Kademlia::CKademlia::isRunning()) {
		Kademlia::CKademlia::stop();
	}
}

void CamuleApp::DisconnectED2K()
{
	// Stop Kad if it's running
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

DEFINE_LOCAL_EVENT_TYPE(wxEVT_MULE_NOTIFY_EVENT)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_AMULE_TIMER)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
