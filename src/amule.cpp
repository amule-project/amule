//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "amule.h"
// implementations of other header files
#pragma implementation "CTypedPtrList.h"
#pragma implementation "GetTickCount.h"
#pragma implementation "GuiEvents.h"
#pragma implementation "updownclient.h"
#endif

#include <cmath>
#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#ifdef __BSD__
		#include <sys/types.h>
	#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#ifdef __WXGTK__

	#ifdef __BSD__
     	#include <sys/param.h>
       	#include <sys/mount.h>
	#else 
		#include <execinfo.h>
		#include <mntent.h>
	#endif /* __BSD__ */

#endif

#ifdef __LINUX__
	#include <execinfo.h>
#endif

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
					//   HAVE_SYS_RESOURCE_H, LOCALEDIR, PACKAGE, 
					//   PACKAGE_STRING and VERSION
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>		// Needed for wxFileName::GetPathSeparator()
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
#include <wx/wfstream.h>

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "server.h"			// Needed for GetListName
#include "CFile.h"			// Needed for CFile
#include "otherfunctions.h"		// Needed for GetTickCount
#include "IPFilter.h"			// Needed for CIPFilter
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "sockets.h"			// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"		// Needed for CServerSocket
#include "UDPSocket.h"			// Needed for CUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "updownclient.h"		// Needed for CUpDownClient
#include "packets.h"
#include "Statistics.h"
#include "AICHSyncThread.h"

#ifndef AMULE_DAEMON
	#include <wx/splash.h>			// Needed for wxSplashScreen
	#include <wx/gauge.h>
	#include <wx/textctrl.h>
	#include <wx/clipbrd.h>			// Needed for wxClipBoard	
	
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

CamuleApp::CamuleApp()
{
	// Initialization
	
	printf("Initialising aMule\n");

	m_app_state = APP_STATE_STARTING;
	
	ConfigDir = wxGetHomeDir() + wxFileName::GetPathSeparator() + 
	wxT(".aMule") + wxFileName::GetPathSeparator();
	
	IsReady		= false;
	clientlist	= NULL;
	searchlist	= NULL;
	knownfiles	= NULL;
	serverlist	= NULL;
	serverconnect	= NULL;
	sharedfiles	= NULL;
	listensocket	= NULL;
	clientudp	= NULL;
	clientcredits	= NULL;
	downloadqueue	= NULL;
	uploadqueue	= NULL;
	ipfilter	= NULL;
	ECServerHandler = NULL;
	localserver = NULL;
	glob_prefs = NULL;
	
	m_dwPublicIP	= 0;

	webserver_pid = 0;
	
	// Apprently needed for *BSD
	SetResourceLimits();

}

CamuleApp::~CamuleApp()
{
	
	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown: Terminating core.\n");
	}
	
	// Delete associated objects
	if (serverlist) {
		delete serverlist;
		serverlist = NULL;
	}
	
	if (searchlist) {
		delete searchlist;
		searchlist = NULL;
	}
	
	if (clientcredits) {
		delete clientcredits;
		clientcredits = NULL;
	}		
	
	// Destroying CDownloadQueue calls destructor for CPartFile
	// calling CSharedFileList::SafeAddKFile occasally.
	if (sharedfiles) {
		delete sharedfiles;
		sharedfiles = NULL;
	}

	if (serverconnect) {
		delete serverconnect;
		serverconnect = NULL;
	}

	if (listensocket) {
		delete listensocket;
		listensocket = NULL;
	}

	if (knownfiles) {
		delete knownfiles;
		knownfiles = NULL;
	}

	if (clientlist) {
		delete clientlist;
		clientlist = NULL;
	}

	if (uploadqueue) {
		delete uploadqueue;
		uploadqueue = NULL;
	}

	if (downloadqueue) {
		delete downloadqueue;
		downloadqueue = NULL;
	}

	if (ipfilter) {
		delete ipfilter;
		ipfilter = NULL;
	}
	
	if (ECServerHandler) {
		delete ECServerHandler;
		ECServerHandler = NULL;
	}

	if (statistics) {
		delete statistics;		
	}		

	if (glob_prefs) {
		delete glob_prefs;
		glob_prefs = NULL;
		CPreferences::EraseItemList();
	}

	if (localserver) {
		delete localserver;
		localserver = NULL;
	}

	if (applog) {
		delete applog; // deleting a wxFFileOutputStream closes it
	}
	
	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown completed.\n");
	}
	
}

int CamuleApp::OnExit()
{
	if (m_app_state!=APP_STATE_STARTING) {
		printf("Now, exiting main app...\n");
	}
	
	// Save credits
	clientcredits->SaveList();
	
	// Save IPFilter file.
	ipfilter->SaveToFile();
	
	// Kill amuleweb if running

	if (webserver_pid) {
		printf("Killing amuleweb instance...\n");			
		wxKillError rc;
		wxKill(webserver_pid,wxSIGKILL, &rc);
		printf("Killed!\n");
	}
	
	// Return 0 for succesful program termination
	return AMULE_APP_BASE::OnExit();
}

int CamuleApp::InitGui(bool ,wxString &)
{
	return 0;
}


bool CamuleApp::OnInit()
{

	// This can't be on constructor or wx2.4.2 doesn't set it.	
	SetVendorName(wxT("TikuWarez"));
	
	// Do NOT change this string to aMule nor anything else, it WILL fuck you up.
	SetAppName(wxT("eMule"));
	
	// Parse cmdline arguments.
	wxCmdLineParser cmdline(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv);

	// Handle these arguments.
	cmdline.AddSwitch(wxT("v"), wxT("version"), wxT("Displays the current version number."));
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("Displays this information."));
	cmdline.AddSwitch(wxT("i"), wxT("enable-stdin"), wxT("Does not disable stdin."));
#ifndef AMULE_DAEMON
	cmdline.AddOption(wxT("geometry"), wxEmptyString, wxT("Sets the geometry of the app.\n\t\t\t<str> uses the same format as standard X11 apps:\n\t\t\t[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"));
#endif
	cmdline.AddSwitch(wxT("d"), wxT("disable-fatal"), wxT("Does not handle fatal exception."));
	cmdline.AddSwitch(wxT("o"), wxT("log-stdout"), wxT("Print log messages to stdout."));
	cmdline.Parse();

	if ( !cmdline.Found(wxT("disable-fatal")) ) {
#ifndef __WXMSW__
	// catch fatal exceptions
	wxHandleFatalExceptions(true);
#endif
	}

	if ( cmdline.Found(wxT("log-stdout")) ) {
		printf("Logging to stdout enabled\n");
		enable_stdout_log = true;
	} else {
		enable_stdout_log = false;
	}
	
	if ( cmdline.Found(wxT("version")) ) {
#ifndef AMULE_DAEMON		
		printf("aMule %s\n", unicode2char(GetMuleVersion()));
#else
		printf("aMule Daemon %s\n", unicode2char(GetMuleVersion()));
#endif
		return false;
	}

	if ( cmdline.Found(wxT("help")) ) {
		cmdline.Usage();
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

	// see if there is another instance running
	wxString server = ConfigDir + wxFileName::GetPathSeparator() + wxT("muleconn");
	wxString host = wxT("localhost");
	wxString IPC = wxT("aMule IPC TESTRUN");
	wxClient* client = new wxClient();
	wxConnectionBase* conn = client->MakeConnection(host, server, IPC);

	// If the connection failed, conn is NULL
	if ( conn ) {
		// An instance is already running!
		
		// This is very tricky. The most secure way to communicate is via ED2K links file
		FILE *ed2kfile;
		char filename[1024];

		/* Link seemed ok, add it to file. */
		sprintf(filename,"%s/.aMule/ED2KLinks",getenv("HOME"));
		ed2kfile = fopen(filename,"a");
		if (ed2kfile != NULL) {
			fprintf(ed2kfile,"RAISE_DIALOG");
			printf("Raised current running aMule\n");
			fclose(ed2kfile);
		} else {
			printf("Error opening file %s.Cannot raise active aMule\n", filename);
		}
		
		conn->Disconnect();
		delete conn;
		delete client;

		printf("aMule already running: exiting\n");
		return false;
	}
	delete client;

	// If there was no server, start one
	localserver = new wxServer();
	localserver->Create(server);

	// Close standard-input
	if ( !cmdline.Found(wxT("enable-stdin")) ) {
		close(0);
	}

	/* If no aMule configuration files exist, see if either lmule or xmule config
	   exists, so that we can use those. */
	wxString lMulePrefDir = wxGetHomeDir() + wxFileName::GetPathSeparator() + wxT(".lmule");
	wxString xMulePrefDir = wxGetHomeDir() + wxFileName::GetPathSeparator() + wxT(".xmule");

	if ( !wxDirExists( ConfigDir ) ) {
		if ( wxDirExists( lMulePrefDir ) ) {
			printf("Found lMule old settings, moving to new dir.\n");
			wxRenameFile(lMulePrefDir,ConfigDir);

		} else if ( wxDirExists(xMulePrefDir) ) {
			printf("Found xMule old settings, copying config & credits files.\n");
			wxMkdir( ConfigDir, CPreferences::GetDirPermissions() );

			CDirIterator xMuleDir(xMulePrefDir); 
			
			// Copy .dat files to the aMule dir			
			wxString file = xMuleDir.FindFirstFile(CDirIterator::File,wxT("*.dat"));
  			while ( !file.IsEmpty() ) {
				wxCopyFile( file, ConfigDir + wxFileName::GetPathSeparator() + file.AfterLast(wxFileName::GetPathSeparator()));
				file = xMuleDir.FindNextFile();
  			}

			// Copy .met files to the aMule dir
			file = xMuleDir.FindFirstFile(CDirIterator::File,wxT("*.met"));
			while ( !file.IsEmpty() ) {
				wxCopyFile( file, ConfigDir + wxFileName::GetPathSeparator() + file.AfterLast(wxFileName::GetPathSeparator()));
				file = xMuleDir.FindNextFile();
  			}

			ShowAlert(_("Copied old ~/.xMule config and credit files to ~/.aMule\nHowever, be sure NOT to remove .xMule if your Incoming / Temp folders are still there ;)"), _("Info"), wxOK);
		} else {
			// No settings to import, new to build.
			wxMkdir( ConfigDir, CPreferences::GetDirPermissions() );
		}
	}

#if wxCHECK_VERSION(2,5,3)
	applog = new wxFFileOutputStream(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
	if ( !applog->Ok() ) {
#else
	applog = new wxFile();
	applog->Create(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"), wxFile::read_write);
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
	
	// Build the filenames for the two OS files
	SetOSFiles(thePrefs::GetOSDir());

	// Display notification on new version or first run
	wxTextFile vfile( ConfigDir + wxFileName::GetPathSeparator() + wxT("lastversion") );
	wxString newMule(wxT(VERSION));

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

	use_chmod = true;
#ifdef __WXGTK__
	/* Test to see if the Temp or the Incoming dir is on a vfat partition. If
	   that is the case, we need to avoid chmoding to avoid lots of warnings.
	   This is done by reading through fstab entries and comparing to the
	   folders used for incomming and temp files. */
#ifndef __BSD__
	FILE* mnt_tab = setmntent("/etc/mtab","r");
	if ( mnt_tab ) {
		wxString incomingdir = thePrefs::GetIncomingDir();
		wxString tempdir = thePrefs::GetTempDir();
		struct mntent* entries;

		entries = getmntent(mnt_tab);
		while ( entries ) {
			if ( strncmp(entries->mnt_type, "vfat",4) == 0 ) {
				if ( tempdir.StartsWith( char2unicode(entries->mnt_dir )) ) {
					// Kry - We cannot addlogline because there's no GUI yet!
					AddLogLineM(false,_("Temp dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
					use_chmod = false;
				}
				if ( incomingdir.StartsWith( char2unicode(entries->mnt_dir )) ) {
					AddLogLineM(false,_("Incoming dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
					use_chmod = false;
				}
				if (!use_chmod) {
					break;
				}
			}
			entries = getmntent(mnt_tab);
		}
		fclose(mnt_tab);
	}
#else
	wxString incomingdir = thePrefs::GetIncomingDir();
	wxString tempdir = thePrefs::GetTempDir();
	long size, i;
#ifdef HAVE_SYS_STATVFS_H
	struct statvfs *mntbuf;
#else
	struct statfs *mntbuf;
#endif

	size = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < size; i++) {
		if ( !strcmp(mntbuf[i].f_fstypename,"msdos")) {
			if ( tempdir.StartsWith( char2unicode( mntbuf[i].f_mntonname )) ) {
				// Kry - We cannot addlogline because there's no GUI yet!
      			AddLogLineM(false,_("Temp dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
                    use_chmod = false;
			}
			if ( incomingdir.StartsWith( char2unicode( mntbuf[i].f_mntonname ) ) ) {
				AddLogLineM(false,_("Incoming dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
				use_chmod = false;
			}
			if (!use_chmod) {
				break;
			}
		}
	}

#endif // __BSD__
#endif

	// Load localization settings
	Localize_mule();

	statistics = new CStatistics();
	
	// Ready file-hasher
	CAddFileThread::Start();

	clientlist	= new CClientList();
	searchlist	= new CSearchList();
	knownfiles	= new CKnownFileList();
	serverlist	= new CServerList();
	serverconnect	= new CServerConnect(serverlist);
	sharedfiles	= new CSharedFileList(serverconnect, knownfiles);

	clientcredits	= new CClientCreditsList();
	
	// bugfix - do this before creating the uploadqueue
	downloadqueue	= new CDownloadQueue(sharedfiles);
	uploadqueue	= new CUploadQueue();
	ipfilter	= new CIPFilter();

	// Create main dialog
	InitGui(geometry_enabled, geom_string);
	
	// Get ready to handle connections from apps like amulecmd
	ECServerHandler = new ExternalConn();

	serverlist->Init();

	// init downloadqueue
	downloadqueue->Init();

	m_app_state = APP_STATE_RUNNING;

	// reload shared files
	sharedfiles->Reload(true, true);

	// Temp addr
	amuleIPV4Address myaddr;
	myaddr.AnyAddress();

	// Create the ListenSocket (aMule TCP socket)
	printf("*** TCP socket at %d\n", thePrefs::GetPort());
	myaddr.Service(thePrefs::GetPort());
	listensocket = new CListenSocket(myaddr);
	
	if (!thePrefs::IsUDPDisabled()) {
		// Create UDP socket
		myaddr.Service(thePrefs::GetUDPPort());
#ifdef TESTING_PROXY
		clientudp = new CClientUDPSocket(myaddr, thePrefs::GetProxyData());
//		clientudp = new CClientUDPSocket(myaddr);
#else
		clientudp = new CClientUDPSocket(myaddr);
#endif
	} else {
		printf("*** UDP socket disabled on preferences\n");
		clientudp = NULL;
	}
	

	// This command just sets a flag to control maximun number of connections.
	// Notify(true) has already been called to the ListenSocket, so events may
	// be already comming in.
	listensocket->StartListening();
	// If we wern't able to start listening, we need to warn the user
	if ( !listensocket->Ok() ) {
		AddLogLineM(true, wxString::Format(_("Port %d is not available. You will be LOWID"),
			thePrefs::GetPort()));
		ShowAlert(wxString::Format(
			_("Port %d is not available !!\n\n"
			  "This means that you will be LOWID.\n\n"
			  "Check your network to make sure the port is open for output and input."),
			thePrefs::GetPort()), _("Error"), wxOK | wxICON_ERROR);
	}

	// Ensure that the up/down ratio is used
	CPreferences::CheckUlDlRatio();

	// The user may now click on buttons
	IsReady = true;

	// Kry - Load the sources seeds on app init
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->LoadSourceSeeds();
	}
	
	// Autoconnect if that option is enabled
	if (thePrefs::DoAutoConnect()) {
		AddLogLineM(true, _("Connecting"));
		theApp.serverconnect->ConnectToAnyServer();
	}
	
	// Run webserver?
	if (thePrefs::GetWSIsEnabled()) {
		#ifndef AMULE_DAEMON
		webserver_pid = wxExecute(wxString::Format(wxT("amuleweb -f -p %d"),thePrefs::GetWSPort()));
		if (!webserver_pid) {
			AddLogLineM(false, _("You requested to run webserver from startup, but the amuleweb binnary cannot be run. Please install the package containing aMule webserver, or compile aMule using --enable-amule-webserver and run make install"));
		}
		#else
		// wxBase has no async wxExecute
		int pid = fork();
		if ( pid == -1 ) {
			printf("ERROR: fork failed with code %d\n", errno);
		} else {
			if ( pid == 0 ) {
				execlp("amuleweb", "amuleweb", "-f", "-q", 0);
				printf("execlp failed with code %d\n", errno);
				exit(0);
			} else {
				printf("aMuleweb is running on pid %d\n", pid);
				webserver_pid = pid;
			}
		}
		#endif
	}

	return true;
}

// Strips specific chars to ensure legal filenames
wxString CamuleApp::StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces)
{
	wxString result;

	for ( unsigned int i = 0; i < strText.Length(); i++ ) {
		switch ( strText.GetChar(i) ) {
			case wxT('/'):
#ifdef __WXMSW__
			case wxT('\"'):
			case wxT('*'):
			case wxT('<'):
			case wxT('>'):
			case wxT('?'):
			case wxT('|'):
			case wxT('\\'):
			case wxT(':'):
#endif
				continue;
			default:
				// Many illegal for filenames in windows below the 32th char (which is space).
				if ( (wxUChar) strText[i] > 31 ) {
						result += strText[i];
				}
		}
	}

	// Should we replace spaces?
	if ( !bKeepSpaces ) {
		result.Replace(wxT(" "), wxT("_"), TRUE);
	}

	return result;
}


// Returns a ed2k file URL
wxString CamuleApp::CreateED2kLink(const CAbstractFile *f)
{
	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	wxString strURL	= wxString(wxT("ed2k://|file|")) <<
		StripInvalidFilenameChars(f->GetFileName(), true) << wxT("|") <<
		f->GetFileSize() << wxT("|") <<
		EncodeBase16( f->GetFileHash(), 16 ) << wxT("|/");
	return strURL;
}

// Returns a ed2k source URL
wxString CamuleApp::CreateED2kSourceLink(const CAbstractFile *f)
{
	if ( !serverconnect->IsConnected() || serverconnect->IsLowID() ) {
		return wxEmptyString;
	}
	uint32 clientID = serverconnect->GetClientID();
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
		StripInvalidFilenameChars(f->GetFileName(), true) + wxT("</a>");
	return strCode;
}

// Generates an URL for checking if a file is "fake"
wxString CamuleApp::GenFakeCheckUrl(const CAbstractFile *f)
{
	wxString strURL = wxT("http://donkeyfakes.gambri.net/index.php?action=search&ed2k=");
	strURL = wxURL::ConvertToValidURI( strURL +  CreateED2kLink( f ) );
	// The following cause problems, so we escape them
	strURL.Replace(wxT("\""), wxT("%22"));
	strURL.Replace(wxT("'"),  wxT("%27"));
	strURL.Replace(wxT("`"),  wxT("%60"));
	return strURL;
}

// jugle.net fake check
wxString CamuleApp::GenFakeCheckUrl2(const CAbstractFile *f)
{
	wxString strURL = wxT("http://www.jugle.net/?fakecheck=%s");
	strURL = wxURL::ConvertToValidURI( strURL +  CreateED2kLink( f ) );
	strURL.Replace(wxT("\""), wxT("%22"));
	strURL.Replace(wxT("'"),  wxT("%27"));
	strURL.Replace(wxT("`"),  wxT("%60"));
	return strURL;
}


/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */)
{
	// Do not do anything if online signature is disabled in Preferences
	if (!thePrefs::IsOnlineSignatureEnabled() || emulesig_path.IsEmpty()) {
		// We do not need to check amulesig_path because if emulesig_path is empty,
		// that means amulesig_path is empty too.
		return;
	}

	// Open both files for writing
	CFile amulesig_out, emulesig_out;
	if (!emulesig_out.Open(emulesig_path, CFile::write)) {
		AddLogLineM(true, wxString(_("Failed to save"))+wxString(_(" OnlineSig File")));
	}
	if (!amulesig_out.Open(amulesig_path, CFile::write)) {
		AddLogLineM(true, wxString(_("Failed to save"))+wxString(_(" aMule OnlineSig File")));
	}

	char buffer[256];
	if (zero) {
		sprintf(buffer, "0\xA");
		strcat(buffer, "0.0|0.0|0");
		emulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("0\n0\n0\n0\n0\n0.0\n0.0\n0\n0\n", 22);
	} else {
		if (serverconnect->IsConnected()) {
			// We are online
			emulesig_out.Write("1",1);
			emulesig_out.Write("|",1);
			// Name of server (Do not use GetRealName()!)
			emulesig_out.Write(unicode2char(serverconnect->GetCurrentServer()->GetListName()),strlen(unicode2char(serverconnect->GetCurrentServer()->GetListName())));
			emulesig_out.Write("|",1);
			// IP and port of the server
			emulesig_out.Write(unicode2char(serverconnect->GetCurrentServer()->GetFullIP()),strlen(unicode2char(serverconnect->GetCurrentServer()->GetFullIP())));
			emulesig_out.Write("|",1);
			sprintf(buffer,"%d",serverconnect->GetCurrentServer()->GetPort());
			emulesig_out.Write(buffer,strlen(buffer));

			// Now for amule sig
			amulesig_out.Write("1",1);
			amulesig_out.Write("\n",1);
			amulesig_out.Write(unicode2char(serverconnect->GetCurrentServer()->GetListName()),strlen(unicode2char(serverconnect->GetCurrentServer()->GetListName())));
			amulesig_out.Write("\n",1);
			amulesig_out.Write(unicode2char(serverconnect->GetCurrentServer()->GetFullIP()),strlen(unicode2char(serverconnect->GetCurrentServer()->GetFullIP())));
			amulesig_out.Write("\n",1);
			amulesig_out.Write(buffer,strlen(buffer));
			amulesig_out.Write("\n",1);

			// Low- or High-ID (only in amule sig)
			if (serverconnect->IsLowID()) {
				amulesig_out.Write("L\n",2);
			} else {
				amulesig_out.Write("H\n",2);
			}
		} else if (serverconnect->IsConnecting()) {
			emulesig_out.Write("0",1);    // shouldn't be modified, to mantain eMule compatibility

                	amulesig_out.Write("2",1);
                	amulesig_out.Write("\n",1);
                	amulesig_out.Write("0",1);
                	amulesig_out.Write("\n",1);
                	amulesig_out.Write("0",1);
                	amulesig_out.Write("\n",1);
                	amulesig_out.Write("0",1);
                	amulesig_out.Write("\n",1);
                	amulesig_out.Write("0\n",2);
		} else {	// Not connected to a server
			emulesig_out.Write("0",1);
			amulesig_out.Write("0\n0\n0\n0\n0\n",10);
		}
		emulesig_out.Write("\xA",1);

		// Datarate for downloads
		sprintf(buffer,"%.1f",downloadqueue->GetKBps());
		emulesig_out.Write(buffer,strlen(buffer));
		emulesig_out.Write("|",1);
		amulesig_out.Write(buffer,strlen(buffer));
		amulesig_out.Write("\n",1);

		// Datarate for uploads
		sprintf(buffer,"%.1f",uploadqueue->GetKBps());
		emulesig_out.Write(buffer,strlen(buffer));
		emulesig_out.Write("|",1);
		amulesig_out.Write(buffer,strlen(buffer));
		amulesig_out.Write("\n",1);

		// Number of users waiting for upload
		sprintf(buffer,"%d",uploadqueue->GetWaitingUserCount());
		emulesig_out.Write(buffer,strlen(buffer));
		amulesig_out.Write(buffer,strlen(buffer));
		amulesig_out.Write("\n",1);

		// Number of shared files
		sprintf(buffer,"%d", sharedfiles->GetCount());
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	}	/* if (!zero) */

	// Nick on the network
	sprintf(buffer, "%s", unicode2char( thePrefs::GetUserNick()));
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total received in bytes
	sprintf(buffer, "%llu", (long long unsigned int)(theApp.statistics->GetSessionReceivedBytes()+ thePrefs::GetTotalDownloaded()));
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total sent in bytes
	sprintf(buffer, "%llu", (long long unsigned int)(theApp.statistics->GetSessionSentBytes()+ thePrefs::GetTotalUploaded()));
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// amule version
	sprintf(buffer,"%s",VERSION);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

        // Total received bytes in session
	if (zero) {
		amulesig_out.Write("0",1);
		amulesig_out.Write("\n",1);
	} else {
		sprintf(buffer, "%llu", (long long unsigned int)theApp.statistics->GetSessionReceivedBytes());
        	amulesig_out.Write(buffer, strlen(buffer));
        	amulesig_out.Write("\n",1);
	}

        // Total sent bytes in session
	if (zero) {
		amulesig_out.Write("0",1);
		amulesig_out.Write("\n",1);
	} else {
		sprintf(buffer, "%llu", (long long unsigned int)theApp.statistics->GetSessionSentBytes());
        	amulesig_out.Write(buffer, strlen(buffer));
        	amulesig_out.Write("\n",1);
	}

	// Uptime
	if (zero) {
		sprintf(buffer,"%u",0);
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	} else {
		//sprintf(buffer,"%s",unicode2char(CastSecondsToHM(GetUptimeSecs())));
		sprintf(buffer,"%u",statistics->GetUptimeSecs());
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	}

	// Close the files
	emulesig_out.Close();
	amulesig_out.Close();

} //End Added By Bouc7

// Gracefully handle fatal exceptions and print backtrace if possible
#include <cxxabi.h>
void CamuleApp::OnFatalException()
{
#ifdef __LINUX__

	// (stkn) create backtrace
	void *bt_array[100];	// 100 should be enough ?!?
	char **bt_strings;
	int num_entries;

	if ((num_entries = backtrace(bt_array, 100)) < 0) {
		fprintf(stderr, "* Could not generate backtrace\n");
		return;
	}

	if ((bt_strings = backtrace_symbols(bt_array, num_entries)) == NULL) {
		fprintf(stderr, "* Could not get symbol names for backtrace\n");
		return;
	}
	
	wxString *libname = new wxString[num_entries];
	wxString *funcname = new wxString[num_entries];
	wxString *address = new wxString[num_entries];
	wxString AllAddresses;
	for (int i = 0; i < num_entries; ++i) {
		wxString wxBtString = char2unicode(bt_strings[i]);
		int posLPar = wxBtString.Find(wxT('('));
		int posRPar = wxBtString.Find(wxT(')'));
		int posLBra = wxBtString.Find(wxT('['));
		int posRBra = wxBtString.Find(wxT(']'));
		bool hasFunction = true;
		if (posLPar == -1 || posRPar == -1) {
			if (posLBra == -1 || posRBra == -1) {
				/* It is important to have exactly num_entries 
				 * addresses in AllAddresses */
				AllAddresses += wxT("0x0000000 ");
				continue;
			}
			posLPar = posLBra;
			hasFunction = false;
		}
		/* Library name */
		int len = posLPar;
		libname[i] = wxBtString.Mid(0, len);
		/* Function name */
		if (hasFunction) {
			int posPlus = wxBtString.Find(wxT('+'));
			if (posPlus == -1) posPlus = posRPar;
			len = posPlus - posLPar - 1;
			funcname[i] = wxBtString.Mid(posLPar + 1, len);
			if (funcname[i].Mid(0,2) == wxT("_Z")) {
				int status;
				char *demangled = abi::__cxa_demangle(unicode2char(funcname[i]), NULL, NULL, &status);
				if (!status) {
					funcname[i] = char2unicode(demangled);
				}
				if (demangled) {
					free(demangled);
				}
			}
		}
		/* Address */
		if ( posLBra == -1 || posRBra == -1) {
			AllAddresses += wxT("0x0000000 ");
		} else {
			len = posRBra - posLBra - 1;
			address[i] = wxBtString.Mid(posLBra + 1, len);
			AllAddresses += address[i] + wxT(" ");
		}
	}
	free(bt_strings);
	
	/* Get line numbers from addresses */
	wxString command = wxString::Format(wxT("addr2line -C -f -s -e /proc/%d/exe "), getpid()) + AllAddresses;
	wxArrayString out;
	wxExecute(command, out);

	/* Print the backtrace */
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");	
	if (wxString(MOD_VERSION_LONG) == wxT("aMule CVS")) {
		fprintf(stderr, "Oops, aMule crashed!\n");
		fprintf(stderr, "Hey, stop crying! You wanted the edge, and now you fell down?!\n");
	} else {
		fprintf(stderr, "OOPS! Houston, we have a situation: seems like aMule crashed!\n");
	}
	fprintf(stderr, "Please, post these lines on the backtrace forum on http://www.amule.org/\n");
	fprintf(stderr, "aMule version is: %s\n", unicode2char(GetMuleVersion()));
	fprintf(stderr, "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n\n");
	
	for (int i = 0; i < num_entries; ++i) {
		/* If we have no function name, use the result from addr2line */
		if (funcname[i].IsEmpty()) {
			funcname[i] = out[2*i];
		}
		wxString btline =
			wxString::Format(wxT("[%d] "), i) +
			funcname[i] +
			wxT(" in ");
		/* If addr2line did not find a line number, use bt_string */
		if (out[2*i+1].Mid(0,2) == wxT("??")) {
			btline += libname[i] + wxT("[") + address[i] + wxT("]");
		} else {
			btline += out[2*i+1];
		}
		/* Print */
		fprintf(stderr, "%s\n", unicode2char(btline) );
	}
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");
	delete [] libname;
	delete [] funcname;
	delete [] address;
#else

	fprintf(stderr, "\nOOPS! - Seems like aMule crashed.\n");
	fprintf(stderr, "aMule version is: %s\n", unicode2char(GetMuleVersion()));
	fprintf(stderr, "--== no BACKTRACE for your platform ==--\n\n");
#endif // not linux
}


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
	int language;

	#define  wxLANGUAGE_CUSTOM 		wxLANGUAGE_USER_DEFINED+1
	#define  wxLANGUAGE_ITALIAN_NAPOLITAN 	wxLANGUAGE_USER_DEFINED+2

	wxLanguageInfo CustomLanguage;
	CustomLanguage.Language = wxLANGUAGE_ITALIAN_NAPOLITAN;
	CustomLanguage.CanonicalName = wxT("it_NA");
	CustomLanguage.Description = wxT("sNeo's Custom Napolitan Language");
	wxLocale::AddLanguage(CustomLanguage);

	CustomLanguage.Language = wxLANGUAGE_CUSTOM;
	CustomLanguage.CanonicalName = wxT("aMule_custom");
	CustomLanguage.Description = wxT("aMule's custom language");

	switch ( thePrefs::GetLanguageID()) {
		case 0:
			language = wxLANGUAGE_DEFAULT;
			break;
		case 1:
			//strcpy(newlang,"ar");
			language = wxLANGUAGE_ARABIC;
			break;
		case 2:
			//strcpy(newlang,"eu");
			language = wxLANGUAGE_BASQUE;
			break;
		case 3:
			//strcpy(newlang,"bg_BG");
			language = wxLANGUAGE_BULGARIAN;
			break;
		case 4:
			//strcpy(newlang,"zh_CN");
			language = wxLANGUAGE_CHINESE_SIMPLIFIED;
			break;
		case 5:
			//strcpy(newlang,"da_DK");
			language = wxLANGUAGE_DANISH;
			break;
		case 6:
			//strcpy(newlang,"nl_NL");
			language = wxLANGUAGE_DUTCH;
			break;
		case 7:
			//strcpy(newlang,"en_GB");
			language = wxLANGUAGE_ENGLISH;
			break;
		case 8:
			//strcpy(newlang,"et_EE");
			language = wxLANGUAGE_ESTONIAN;
			break;
		case 9:
			//strcpy(newlang,"fi");
			language = wxLANGUAGE_FINNISH;
			break;
		case 10:
			//strcpy(newlang,"fr_FR");
			language = wxLANGUAGE_FRENCH;
			break;
		case 11:
			//strcpy(newlang,"gl_ES");
			language = wxLANGUAGE_GALICIAN;
			break;
		case 12:
			//strcpy(newlang,"de_DE");
			language = wxLANGUAGE_GERMAN;
			break;
		case 13:
			//strcpy(newlang,"it_IT");
			language = wxLANGUAGE_ITALIAN;
			break;
		case 14:
			//strcpy(newlang,"ko_KR");
			language = wxLANGUAGE_KOREAN;
			break;
		case 15:
			//strcpy(newlang,"lt_LT");
			language = wxLANGUAGE_LITHUANIAN;
			break;
		case 16:
			//strcpy(newlang,"pl_PL");
			language = wxLANGUAGE_POLISH;
			break;
		case 17:
			//strcpy(newlang,"pt_PT");
			language = wxLANGUAGE_PORTUGUESE;
			break;
		case 18:
			//strcpy(newlang,"pt_BR");
			language = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
			break;
		case 19:
			//strcpy(newlang,"ru_RU");
			language = wxLANGUAGE_RUSSIAN;
			break;
		case 20:
			//strcpy(newlang,"es_ES");
			language = wxLANGUAGE_SPANISH;
			break;
		case 21:
			//strcpy(newlang,"es_CH");
			language = wxLANGUAGE_SPANISH_CHILE;
			break;
		case 22:
			//strcpy(newlang,"es_MX");
			language = wxLANGUAGE_SPANISH_MEXICAN;
			break;
		case 23:
			//Turkish makes weird things with .eMule file!!! why?
			//language = wxLANGUAGE_TURKISH;
			language = wxLANGUAGE_DEFAULT;
			break;
		case 24:
			//strcpy(newlang,"hu");
			language = wxLANGUAGE_HUNGARIAN;
			break;
		case 25:
			//strcpy(newlang,"ca_ES");
			language = wxLANGUAGE_CATALAN;
			break;
		case 26:
			//strcpy(newlang,"hr");
			language = wxLANGUAGE_CROATIAN;
			break;
		case 27:
			//strcpy(newlang,"it_CH");
			language = wxLANGUAGE_ITALIAN_SWISS;
			break;
		case 28:
			//strcpy(newlang,"custom");
			language = wxLANGUAGE_CUSTOM;
			break;
		case 29:
			//strcpy(newlang,"zh_TW");
			language = wxLANGUAGE_CHINESE_TRADITIONAL;
			break;
		case 30:
			language = wxLANGUAGE_SLOVENIAN;
			break;
		default:
			language = wxLANGUAGE_DEFAULT;
			break;
	}

	int language_flags = 0;
	if ((language != wxLANGUAGE_CUSTOM) && (language != wxLANGUAGE_ITALIAN_NAPOLITAN)) {
		language_flags = wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING;
	}
	
	if ((!m_locale.Init(language,language_flags)) && 
	    (language != wxLANGUAGE_DEFAULT) &&
	    (language != wxLANGUAGE_CUSTOM)) {
		AddLogLineM(false,_("The selected locale seems not to be installed on your box."
				    " (Note: I'll try to set it anyway)"));
	}
	
	if (language != wxLANGUAGE_CUSTOM) {
		m_locale.AddCatalogLookupPathPrefix(wxT(LOCALEDIR));
		m_locale.AddCatalog(wxT(PACKAGE));
	} else {
		m_locale.AddCatalogLookupPathPrefix(wxString::Format(wxT("%s/.aMule"), getenv("HOME")));
		m_locale.AddCatalog(wxT("custom"));
	}
}


// Displays information related to important changes in aMule.
// Is called when the user runs a new version of aMule
void CamuleApp::Trigger_New_version(wxString new_version)
{
	wxString info = _(" --- This is the first time you run aMule %s ---\n\n");
	info.Replace( wxT("%s"), new_version );
	if (new_version == wxT("CVS")) {
		info += _("This version is a testing version, updated daily, and \n");
		info += _("we give no warranty it won't break anything, burn your house,\n");
		info += _("or kill your dog. But it *should* be safe to use anyway. \n");
	} 
	
	// General info
	info += wxT("\n");
	info += _("More information, support and new releases can found at our homepage, \n");
	info += _("at www.aMule.org, or in our IRC channel #aMule at irc.freenode.net. \n");
	info += wxT("\n");
	info += _("Your locale has been changed to System Default due to a version change. Sorry.\n");
	info += _("Feel free to report any bugs to forum.amule.org");

	ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);

	// Set to system default... no other way AFAIK unless we change the save type.
	thePrefs::SetLanguageID(0);
}

void CamuleApp::QueueLogLine(bool addtostatusbar, const wxString& line)
{
	m_LogQueueLock.Enter();

	QueuedLogLine new_line_to_log;
	
	new_line_to_log.line = line;
	new_line_to_log.addtostatus = addtostatusbar;

	QueuedAddLogLines.push_back(new_line_to_log);

	m_LogQueueLock.Leave();
}


void CamuleApp::FlushQueuedLogLines()
{
	QueuedLogLine line_to_add;

	m_LogQueueLock.Enter();

	while (!QueuedAddLogLines.empty()) {
		line_to_add = QueuedAddLogLines.front();
		QueuedAddLogLines.pop_front();
		AddLogLineM(line_to_add.addtostatus, line_to_add.line);
	}

	m_LogQueueLock.Leave();
}

void CamuleApp::SetOSFiles(const wxString new_path) {
	if (::wxDirExists(new_path)) {
		emulesig_path = new_path + wxFileName::GetPathSeparator() + wxT("onlinesig.dat");
		amulesig_path = new_path + wxFileName::GetPathSeparator() + wxT("amulesig.dat");
	} else {
		ShowAlert(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("Error"), wxOK | wxICON_ERROR);
		emulesig_path = wxEmptyString;
		amulesig_path = wxEmptyString;
	}
}

void CamuleApp::OnDnsDone(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);
	CUDPSocket* socket=(CUDPSocket*)evt.GetClientData();	
	socket->DnsLookupDone(evt.GetExtraLong());
}

void CamuleApp::OnSourcesDnsDone(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);	
	downloadqueue->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnNotifyEvent(wxEvent& e)
{
	GUIEvent& evt = *((GUIEvent*)&e);
	NotifyEvent(evt);
}

void CamuleApp::OnUDPTimer(AMULE_TIMER_EVENT_CLASS& WXUNUSED(evt))
{
	if (IsReady) {
		serverlist->SendNextPacket();
	}
}


void CamuleApp::OnTCPTimer(AMULE_TIMER_EVENT_CLASS& WXUNUSED(evt))
{
	if(!IsReady) {
		return;
	}
	serverconnect->StopConnectionTry();
	if (serverconnect->IsConnected() ) {
		return;
	}
	serverconnect->ConnectToAnyServer();
}


void CamuleApp::OnCoreTimer(AMULE_TIMER_EVENT_CLASS& WXUNUSED(evt))
{
	// Former TimerProc section
	static uint32	msPrev1, msPrev5, msPrevSave;
	uint32 msCur = statistics->GetUptimeMsecs();
	static uint32	msPrevHist;

	// can this actually happen under wxwin ?
	if (!IsRunning() || !IsReady) {
		return;
	}

	uploadqueue->Process();
	downloadqueue->Process();
	//theApp.clientcredits->Process();
	uploadqueue->CompUpDatarateOverhead();
	downloadqueue->CompDownDatarateOverhead();

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
		serverlist->Process();
		clientlist->Process();
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
		OnlineSig(); // Added By Bouc7
		// Kry - Log lines flush
		FlushQueuedLogLines();
		// Stats tree is updated every 5 seconds. Maybe we should make it match prefs.
		statistics->UpdateStatsTree();
		downloadqueue->SortByPriority();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		wxString buffer;
		
		wxConfigBase* cfg = wxConfigBase::Get();
		buffer.Printf(wxT("%llu"),theApp.statistics->GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded());
		cfg->Write(wxT("/Statistics/TotalDownloadedBytes"), buffer);

		buffer.Printf(wxT("%llu"),theApp.statistics->GetSessionSentBytes()+thePrefs::GetTotalUploaded());
		cfg->Write(wxT("/Statistics/TotalUploadedBytes"), buffer);
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
	static int filecount = 0;
	static int bytecount = 0;

	CKnownFile* result = (CKnownFile*)evt.GetClientData();
	if (evt.GetExtraLong()) {
		CPartFile* requester = (CPartFile*)evt.GetExtraLong();
		if (downloadqueue->IsPartFile(requester)) {
			requester->PartFileHashFinished(result);
		}
	} else {
		if (knownfiles->SafeAddKFile(result)) {
			sharedfiles->SafeAddKFile(result);

			filecount++;
			bytecount += result->GetFileSize();
			// If we have added 30 files or files with a total size of ~300mb
			if ( ( filecount == 30 ) || ( bytecount >= 314572800 ) ) {
				if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
					knownfiles->Save();
					filecount = 0;
					bytecount = 0;
				}
			}
		} else {
			printf("File not added to sharedlist: %s\n", unicode2char(result->GetFileName()));
			delete result;
		}
	}

	return;
}

void CamuleApp::OnFinishedCompletion(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);
	CPartFile* completed = (CPartFile*)evt.GetClientData();
	wxASSERT(completed);
	completed->CompleteFileEnded(evt.GetInt(), (wxString*)evt.GetExtraLong());

	return;
}

void CamuleApp::ShutDown() {
	// Signal the hashing thread to terminate
	m_app_state = APP_STATE_SHUTINGDOWN;
	IsReady =  false;
	
	// Kry - Save the sources seeds on app exit
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->SaveSourceSeeds();
	}

	OnlineSig(); // Added By Bouc7

	// Close sockets to avoid new clients coming in
	if (listensocket) {
		listensocket->StopListening();
	}
	if (clientudp) {
		clientudp->Destroy();
	}
	if (serverconnect) {
		serverconnect->Disconnect();
	}

	// saving data & stuff
	if (knownfiles) {
		knownfiles->Save();
	}

	thePrefs::Add2TotalDownloaded(theApp.statistics->GetSessionReceivedBytes());
	thePrefs::Add2TotalUploaded(theApp.statistics->GetSessionSentBytes());

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
}

#if defined(__DEBUG__) 
	#if defined(__LINUX__)
	
		void CamuleApp::AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time) {
	
			socket_deletion_log_item current_socket;
			socket_deletion_log_item temp_socket;
	
			current_socket.socket_n = socket_pointer;
			current_socket.creation_time = creation_time;
			current_socket.backtrace = wxEmptyString;
	
			void *bt_array[6];	// 6 should be enough ?!?
			char **bt_strings;
			int num_entries;
	
			if ((num_entries = backtrace(bt_array, 6)) < 0) {
				current_socket.backtrace += wxT("* Could not generate backtrace\n");
			} else {
				if ((bt_strings = backtrace_symbols(bt_array, num_entries)) == NULL) {
					current_socket.backtrace += wxT("* Could not get symbol names for backtrace\n");
				}  else {
					int n;
					if (num_entries < 5) {
						n = num_entries;
					} else {
						n = 5;
					}
					for (int i = n - 1; i >= 0; i--) {
						current_socket.backtrace += wxString::Format(wxT("[%d] %s | "), i, bt_strings[i]);
					}
					current_socket.backtrace += wxT("END");
				}
			}
	
			uint32 size = SocketDeletionList.size();
			for ( uint32 i = 0; i < size; ++i ) {
				if (( SocketDeletionList[i].socket_n == socket_pointer) && ( SocketDeletionList[i].creation_time == creation_time)) {
	
					printf("\n-----------------------RSB FOUND!!!!!!!!!!!!!!!!!!!!!!!!!------------\n");
					printf("First deletion  (ptr: %u time: %u) BT:\n",temp_socket.socket_n, temp_socket.creation_time);
					printf("-> %s\n\n",unicode2char(temp_socket.backtrace));
	
					printf("Second deletion (ptr: %u time: %u) BT:\n",current_socket.socket_n,current_socket.creation_time);
					printf("-> %s\n\n",unicode2char(current_socket.backtrace));
	
					printf("--------------------------- Get Ready for RC4---------------------------\n");
	
					//wxASSERT(0);
				}
			}
	
			SocketDeletionList.push_back(current_socket);
	
		}

	#else
		void CamuleApp::AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time) {
		// No backtrace on this platform.
		}
	#endif // __LINUX__
		
#endif // __DEBUG__




bool CamuleApp::AddServer(CServer *srv)
{
	if ( serverlist->AddServer(srv) ) {
		Notify_ServerAdd(srv)
		return true;
	}
	return false;
}

//
// Kry Yay, unicoding via streams
#if wxCHECK_VERSION(2,5,3)
#include <wx/sstream.h>
void CamuleApp::AddLogLine(const wxString &msg)
{
	wxString full_line = wxDateTime::Now().FormatISODate() + wxT(" ") + 
		wxDateTime::Now().FormatISOTime() + wxT(": ") + msg + wxT("\n");
	
	wxStringInputStream stream(full_line);
	
	(*applog) << stream;
	
	if ( enable_stdout_log ) { 
		puts(unicode2charbuf(full_line));
	}

}
#else
void CamuleApp::AddLogLine(const wxString &msg)
{
        wxString curr_date = wxDateTime::Now().FormatDate() + wxT(" ") +
		wxDateTime::Now().FormatTime() + wxT(": ");
	const wxCharBuffer date_str_buf = unicode2charbuf(curr_date);
	const char *date_str = (const char *)date_str_buf;
	applog->Write(date_str, strlen(date_str));
	if ( enable_stdout_log ) {
		fputs(date_str, stdout);
	}
	
	const wxCharBuffer c_msg_buf = unicode2charbuf(msg);
	const char *c_msg = (const char *)c_msg_buf;
	if (c_msg != NULL) {
		applog->Write(c_msg, strlen(c_msg));
	}
	applog->Write("\n", 1);
	if ( enable_stdout_log ) {
		puts(c_msg);
	}	
	
	applog->Flush();
}
#endif

uint32 CamuleApp::GetPublicIP() const {
	/*
	if (m_dwPublicIP == 0 && Kademlia::CKademlia::isConnected() && !Kademlia::CKademlia::isFirewalled() )
		return ntohl(Kademlia::CKademlia::getIPAddress());
	*/
	return m_dwPublicIP;
}

void CamuleApp::SetPublicIP(const uint32 dwIP){
	if (dwIP != 0){
		wxASSERT ( !IsLowIDED2K(dwIP));
		//wxASSERT ( m_pPeerCache );
//		if ( GetPublicIP() == 0)
			//AddDebugLogLineM(false, wxString::Format(wxT("My public IP Address is: %s"),ipstr(dwIP)));
//		else if (Kademlia::CKademlia::isConnected() && !Kademlia::CKademlia::isFirewalled() && ntohl(Kademlia::CKademlia::getIPAddress()) != dwIP)
//			AddDebugLogLine(DLP_DEFAULT, false,  _T("Public IP Address reported from Kademlia (%s) differs from new found (%s)"),ipstr(ntohl(Kademlia::CKademlia::getIPAddress())),ipstr(dwIP));
//		m_pPeerCache->FoundMyPublicIPAddress(dwIP);	
	}
//	else
//		AddDebugLogLine(DLP_VERYLOW, false, _T("Deleted public IP"));
	
	m_dwPublicIP = dwIP;

}

wxString CamuleApp::GetLog(bool reset)
{
	ConfigDir = wxGetHomeDir() + wxFileName::GetPathSeparator() + 
		wxT(".aMule") + wxFileName::GetPathSeparator();
	wxFile *logfile = new wxFile();
	logfile->Open(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
	if ( !logfile->IsOpened() ) {
		return wxT("ERROR: can't open logfile");
	}
	int len = logfile->Length();
	if ( len == 0 ) {
		return wxT("WARNING: logfile is empty. Something wrong");
	}
	char *tmp_buffer = new char[len];
	logfile->Read(tmp_buffer, len);
	wxString str(char2unicode(tmp_buffer));
	delete [] tmp_buffer;
	if ( reset ) {
#if wxCHECK_VERSION(2,5,3)
		delete applog;
		applog = new wxFFileOutputStream(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
		if ( applog->Ok() ) {
#else
		applog->Close();
		applog->Create(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"), true, wxFile::read_write);
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
	        server_msg = wxT("");
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
	AddLogLine(_("ServerMessage: ") + msg);
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
	}
}

DEFINE_EVENT_TYPE(wxEVT_NOTIFY_EVENT)
DEFINE_EVENT_TYPE(wxEVT_AMULE_TIMER)

DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_EVENT_TYPE(wxEVT_CORE_DNS_DONE)
