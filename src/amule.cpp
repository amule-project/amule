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

#ifdef __WXGTK__

	#ifdef __BSD__
     	#include <sys/param.h>
       	#include <sys/mount.h>
	#else 
		#ifdef __SOLARIS__
			#include <sys/mnttab.h>
		#else
			#include <mntent.h>
		#endif
	#endif /* __BSD__ */

#endif

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
					//   HAVE_SYS_RESOURCE_H, LOCALEDIR, PACKAGE, 
					//   PACKAGE_STRING and VERSION
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/url.h>
#include <wx/wfstream.h>

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "Server.h"			// Needed for GetListName
#include "CFile.h"			// Needed for CFile
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "IPFilter.h"			// Needed for CIPFilter
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket
#include "ServerSocket.h"		// Needed for CServerSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "ServerConnect.h"			// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerUDPSocket.h"			// Needed for CServerUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "updownclient.h"		// Needed for CUpDownClient
#include "Packet.h"
#include "Statistics.h"
#include "AICHSyncThread.h"
#include "Logger.h"

#ifndef AMULE_DAEMON
	#include <wx/splash.h>			// Needed for wxSplashScreen
	#include <wx/gauge.h>
	#include <wx/textctrl.h>
	#include <wx/clipbrd.h>			// Needed for wxClipBoard	
	#include <wx/msgdlg.h>			// Needed for wxMessageBox

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

		<< wxT("Current version is: aMule ") << GetMuleVersion() << "\n";

#ifdef AMULE_DAEMON
	printf("FATAL ERROR! %s\n", (const char*)unicode2char(msg));
#else
	wxMessageBox( msg, "FATAL ERROR!", wxICON_ERROR );
#endif	

	exit(1);
#endif

	m_app_state = APP_STATE_STARTING;
	
	ConfigDir = GetConfigDir();
	
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
	
	// Save IPFilter file.
	ipfilter->SaveToFile();
	
	// Kill amuleweb if running
	if (webserver_pid) {
		printf("Killing amuleweb instance...\n");
		wxKillError rc;
		wxKill(webserver_pid,wxSIGKILL, &rc);
		printf("Killed!\n");
	}


	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown: Terminating core.\n");
	}
	
	
	//printf("ServerList... ");
	if (serverlist) {
		delete serverlist;
		serverlist = NULL;
	}
	
	//printf("SearchList... ");
	if (searchlist) {
		delete searchlist;
		searchlist = NULL;
	}
	
	//printf("ClientCredits... ");
	if (clientcredits) {
		delete clientcredits;
		clientcredits = NULL;
	}		
	
	// Destroying CDownloadQueue calls destructor for CPartFile
	// calling CSharedFileList::SafeAddKFile occasally.
	//printf("SharedFiles... ");
	if (sharedfiles) {
		delete sharedfiles;
		sharedfiles = NULL;
	}
	
	//printf("SeverConnect... ");
	if (serverconnect) {
		delete serverconnect;
		serverconnect = NULL;
	}
	
	//printf("ListenSocket... ");
	if (listensocket) {
		delete listensocket;
		listensocket = NULL;
	}
	
	//printf("KnownFiles... ");
	if (knownfiles) {
		delete knownfiles;
		knownfiles = NULL;
	}
	
	//printf("ClientList... ");
	if (clientlist) {
		delete clientlist;
		clientlist = NULL;
	}
	
	//printf("UploadQueue... ");
	if (uploadqueue) {
		delete uploadqueue;
		uploadqueue = NULL;
	}
	
	//printf("DownloadQueue... ");
	if (downloadqueue) {
		delete downloadqueue;
		downloadqueue = NULL;
	}
	
	//printf("IPFilter... ");
	if (ipfilter) {
		delete ipfilter;
		ipfilter = NULL;
	}
	
	//printf("ECServerHandler... ");
	if (ECServerHandler) {
		delete ECServerHandler;
		ECServerHandler = NULL;
	}

	//printf("Statistics... ");
	if (statistics) {
		delete statistics;		
	}		

	//printf("Preferences... ");
	if (glob_prefs) {
		delete glob_prefs;
		glob_prefs = NULL;
		CPreferences::EraseItemList();
	}

	//printf("LocalServer... ");
	if (localserver) {
		delete localserver;
		localserver = NULL;
	}
	
	//printf("AppLog... ");
	if (applog) {
		delete applog; // deleting a wxFFileOutputStream closes it
		applog = NULL;
	}
	
	//printf("Done! ");
	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown completed.\n");
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

	sent = 0;
	
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
		printf("aMule %s\n", (const char *)unicode2char(GetMuleVersion()));
#else
		printf("aMule Daemon %s\n", (const char *)unicode2char(GetMuleVersion()));
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
	#ifdef __SOLARIS__
		FILE* mnt_tab = fopen("/etc/mtab","r");		
	#else
		FILE* mnt_tab = setmntent("/etc/mtab","r");
	#endif
	if ( mnt_tab ) {
		wxString incomingdir = thePrefs::GetIncomingDir();
		wxString tempdir = thePrefs::GetTempDir();
		#ifdef __SOLARIS__
			struct mnttab entries;
			while ( getmntent(mnt_tab,&entries )!=-1) {
				if ( strncmp(entries.mnt_fstype, "vfat",4) == 0 ) {
					#if wxUSE_UNICODE
					if ( tempdir.StartsWith( UTF82unicode(entries.mnt_mountp )) ) {
					#else 
					if ( tempdir.StartsWith( char2unicode(entries.mnt_mountp )) ) {
					#endif
		#else
			struct mntent* entries;
			entries = getmntent(mnt_tab);
			while ( entries ) {
				if ( strncmp(entries->mnt_type, "vfat",4) == 0 ) {
					#if wxUSE_UNICODE
					if ( tempdir.StartsWith( UTF82unicode(entries->mnt_dir )) ) {
					#else 
					if ( tempdir.StartsWith( char2unicode(entries->mnt_dir )) ) {
					#endif
		#endif	
					// Kry - We cannot addlogline because there's no GUI yet!
					AddLogLineM(false,_("Temp dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
					use_chmod = false;
				}
				#ifdef __SOLARIS__
					#if wxUSE_UNICODE
					if ( incomingdir.StartsWith( UTF82unicode(entries.mnt_mountp )) ) {
					#else
					if ( incomingdir.StartsWith( char2unicode(entries.mnt_mountp)) ) {
					#endif
				#else
					#if wxUSE_UNICODE
					if ( incomingdir.StartsWith( UTF82unicode(entries->mnt_dir )) ) {
					#else
					if ( incomingdir.StartsWith( char2unicode(entries->mnt_dir )) ) {
					#endif
				#endif
					AddLogLineM(false,_("Incoming dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
					use_chmod = false;
				}
				if (!use_chmod) {
					break;
				}
			}
			#ifndef __SOLARIS__
				entries = getmntent(mnt_tab);
			#endif
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
			#if wxUSE_UNICODE
			if ( tempdir.StartsWith( UTF82unicode( mntbuf[i].f_mntonname )) ) {
			#else
			if ( tempdir.StartsWith( char2unicode( mntbuf[i].f_mntonname )) ) {
			#endif
				// Kry - We cannot addlogline because there's no GUI yet!
      			AddLogLineM(false,_("Temp dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings."));
                    use_chmod = false;
			}
			#if wxUSE_UNICODE
			if ( incomingdir.StartsWith( UTF82unicode( mntbuf[i].f_mntonname ) ) ) {
			#else
			if ( incomingdir.StartsWith( char2unicode( mntbuf[i].f_mntonname ) ) ) {
			#endif
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
	
	sharedfiles	= new CSharedFileList(knownfiles);
	clientcredits	= new CClientCreditsList();
	
	// bugfix - do this before creating the uploadqueue
	downloadqueue	= new CDownloadQueue();
	uploadqueue	= new CUploadQueue();
	ipfilter	= new CIPFilter();

	// Create main dialog
	InitGui(geometry_enabled, geom_string);
	
	serverlist->Init();

	// init downloadqueue
	downloadqueue->LoadMetFiles( thePrefs::GetTempDir() );

	// Creates all needed listening sockets
	wxString msg;
	bool ok;
	ok = ReinitializeNetwork(&msg);
	if (!msg.IsEmpty()) {
		printf("%s", (const char *)unicode2char(msg));
	}

	m_app_state = APP_STATE_RUNNING;

	// reload shared files
	sharedfiles->Reload(true);
	
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
		webserver_pid = wxExecute(wxString(wxT("amuleweb -f -p ")) << thePrefs::GetWSPort());
		if (!webserver_pid) {
			AddLogLineM(false, _(
				"You requested to run webserver from startup, "
				"but the amuleweb binary cannot be run. "
				"Please install the package containing aMule webserver, "
				"or compile aMule using --enable-webserver and run make install"));
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

bool CamuleApp::ReinitializeNetwork(wxString *msg)
{
	bool ok = true;
	static bool firstTime = true;
	
	if (!firstTime) {
		// TODO: Destroy previously created sockets
	}
	firstTime = false;
	
	// Some sanity checks first
	if (thePrefs::ECPort() == thePrefs::GetPort()) {
		*msg << wxT("Network configuration failed! You can't configure aMule TCP port and External Connections port at the same port number. Please, change one of them.");
		return false;
	}
	if (thePrefs::GetUDPPort() == (thePrefs::GetPort()+3)) {
		*msg << wxT("Network configuration failed! You can't configure UDP(TCP+3) port and UDP (extended eMule) port at the same port number. Please, change one of them.");
		return false;
	}
	
	// Create the address where we are going to listen
	// TODO: read this from configuration file
	amuleIPV4Address myaddr;
	myaddr.AnyAddress();
	wxString ip = myaddr.IPAddress();
	
	// Get ready to handle connections from apps like amulecmd
	myaddr.Service(thePrefs::ECPort());
	ECServerHandler = new ExternalConn(myaddr, msg);

	// Create the UDP socket.
	// Used for extended eMule protocol, Queue Rating, File Reask Ping.
	// Default is port 4672.
	if (!thePrefs::IsUDPDisabled()) {
		myaddr.Service(thePrefs::GetUDPPort());
//#ifdef TESTING_PROXY
		clientudp = new CClientUDPSocket(myaddr, thePrefs::GetProxyData());
		*msg << wxT("*** Client UDP socket (extended eMule) at ") <<
			ip << wxT(":") << thePrefs::GetUDPPort() << wxT("\n");
	} else {
		*msg << wxT("*** Client UDP socket (extended eMule) disabled on preferences\n");
		clientudp = NULL;
	}
	
	// Creates the UDP socket TCP+3.
	// Used for source asking on servers.
	myaddr.Service(thePrefs::GetPort()+3);
	serverconnect = new CServerConnect(serverlist, myaddr);
	
	*msg << wxT("*** Server UDP socket (TCP+3) at ") << 
		ip << wxT(":") << thePrefs::GetPort() + 3 << wxT("\n");
	
	// Create the ListenSocket (aMule TCP socket).
	// Used for Client Port / Connections from other clients,
	// Client to Client Source Exchange.
	// Default is 4662.
	*msg << wxT("*** TCP socket (TCP) listening on ") <<
		ip << wxT(":") << thePrefs::GetPort() << wxT("\n");
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
		err <<	_("Port ") << thePrefs::GetPort() <<
			_(" is not available. You will be LOWID");
		*msg << err;
		AddLogLineM(true, err);
		err.Clear();
		err << _("Port ") << thePrefs::GetPort() << _(
			" is not available!\n\n"
			"This means that you will be LOWID.\n\n"
			"Check your network to make sure the port is open for output and input.");
		ShowAlert(err, _("Error"), wxOK | wxICON_ERROR);
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
		CleanupFilename(f->GetFileName(), true) + wxT("</a>");
	return strCode;
}

wxString validateURI(const wxString url)
{
	wxString strURI;
#if wxCHECK_VERSION_FULL(2,5,3,2)
	wxURI* uri = new wxURI(url);
	strURI=uri->BuildURI();
#else
	strURI=wxURL::ConvertToValidURI(url);
	// The following cause problems, so we escape them
	strURI.Replace(wxT("\""), wxT("%22")); 
	strURI.Replace(wxT("'"),  wxT("%27")); 
	strURI.Replace(wxT("`"),  wxT("%60")); 
#endif
	return strURI;
}
// Generates an URL for checking if a file is "fake"
wxString CamuleApp::GenFakeCheckUrl(const CAbstractFile *f)
{
	wxString strURL = wxT("http://donkeyfakes.gambri.net/index.php?action=search&ed2k=");
	strURL = validateURI( strURL +  CreateED2kLink( f ) );
	return strURL;
}

// jugle.net fake check
wxString CamuleApp::GenFakeCheckUrl2(const CAbstractFile *f)
{
	wxString strURL = wxT("http://www.jugle.net/?fakecheck=%s");
	strURL = validateURI( strURL +  CreateED2kLink( f ) );
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

	// Open both files if needed
	if (!emulesig_out.IsOpened()) {
		if ( !wxFileExists( emulesig_path ) ) {
			if ( !emulesig_out.Create(emulesig_path) ) {
				AddLogLineM(true, wxString(_("Failed to create"))+_(" OnlineSig File"));
				// Will never try again.
				amulesig_path.Clear();
				emulesig_path.Clear();
				return;
			}
		} else if  ( !emulesig_out.Open(emulesig_path) ) {
			AddLogLineM(true, wxString(_("Failed to open"))+_(" OnlineSig File"));
			// Will never try again.
			amulesig_path.Clear();
			emulesig_path.Clear();
			return;
		}
	}
	if (!amulesig_out.IsOpened()) {
		if ( !wxFileExists( amulesig_path ) ) {
			if ( !amulesig_out.Create(amulesig_path) ) {
				AddLogLineM(true, wxString(_("Failed to create"))+_(" aMule OnlineSig File"));
				// Will never try again.
				amulesig_path.Clear();
				emulesig_path.Clear();
				return;
			}
		} else if  ( !amulesig_out.Open(amulesig_path) ) {
			AddLogLineM(true, wxString(_("Failed to open"))+_(" aMule OnlineSig File"));
			// Will never try again.
			amulesig_path.Clear();
			emulesig_path.Clear();
			return;
		}
	}

#if wxCHECK_VERSION(2,5,3)
	emulesig_out.Clear();
	amulesig_out.Clear();
#else
	unsigned int i;
	for( i = 0; i < emulesig_out.GetLineCount(); ++i) {
		emulesig_out.RemoveLine(1);
	}
	for( i = 0; i < amulesig_out.GetLineCount(); ++i) {
		amulesig_out.RemoveLine(1);
	}
#endif
	
	wxString emulesig_string;
	
	if (zero) {
		
		emulesig_string = wxT("0\xA0.0|0.0|0");
		amulesig_out.AddLine(wxT("0\n0\n0\n0\n0\n0.0\n0.0\n0\n0"));
		
	} else {
		if (serverconnect->IsConnected()) {

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
		/*
		emulesig_out.Write("\xA",1);
		*/

		wxString temp;
		
		// Datarate for downloads
		temp = wxString::Format(wxT("%.1f"),downloadqueue->GetKBps());
		
		emulesig_string += temp + wxT("|");
		amulesig_out.AddLine(temp);

		// Datarate for uploads
		temp = wxString::Format(wxT("%.1f"),uploadqueue->GetKBps());
		
		emulesig_string += temp + wxT("|");		
		amulesig_out.AddLine(temp);

		// Number of users waiting for upload
		temp = wxString::Format(wxT("%d"),uploadqueue->GetWaitingUserCount());
		
		emulesig_string += temp; 
		amulesig_out.AddLine(temp);
		
		// Number of shared files (not on eMule)
		amulesig_out.AddLine(wxString::Format(wxT("%d"), sharedfiles->GetCount()));
		
	}	/* if (!zero) */
	
	// eMule signature finished here. Write the line to the wxTextFile.
	emulesig_out.AddLine(emulesig_string);

	// Now for aMule signature extras
	
	// Nick on the network
	amulesig_out.AddLine(thePrefs::GetUserNick());

	// Total received in bytes
	amulesig_out.AddLine(wxString::Format(wxT("%llu"),
		(long long unsigned int)(theApp.statistics->GetSessionReceivedBytes() +
		thePrefs::GetTotalDownloaded())));

	// Total sent in bytes
	amulesig_out.AddLine(wxString::Format(wxT("%llu"),
		(long long unsigned int)(theApp.statistics->GetSessionSentBytes() +
		thePrefs::GetTotalUploaded())));

	// amule version
#ifdef CVSDATE
	amulesig_out.AddLine(wxT(VERSION " " CVSDATE));
#else
	amulesig_out.AddLine(wxT(VERSION));
#endif

        // Total received bytes in session
	if (zero) {
		amulesig_out.AddLine(wxT("0"));
	} else {
		amulesig_out.AddLine(wxString::Format(wxT("%llu"),
			(long long unsigned int)theApp.statistics->GetSessionReceivedBytes()));
	}

        // Total sent bytes in session
	if (zero) {
		amulesig_out.AddLine(wxT("0"));
	} else {
		amulesig_out.AddLine(wxString::Format(wxT("%llu"),
			(long long unsigned int)theApp.statistics->GetSessionSentBytes()));
	}

	// Uptime
	if (zero) {
		amulesig_out.AddLine(wxT("0"));
	} else {
		amulesig_out.AddLine(wxString::Format(wxT("%u"),statistics->GetUptimeSecs()));
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
	if (wxString(MOD_VERSION_LONG) == wxT("aMule CVS")) {
		fprintf(stderr, "Oops, aMule crashed!\n");
		fprintf(stderr, "Hey, stop crying! You wanted the edge, and now you fell down?!\n");
	} else {
		fprintf(stderr, "OOPS! Houston, we have a situation: seems like aMule crashed!\n");
	}

	fprintf(stderr, "Please, post the following lines, on the aMule Crashes forum on:\n");
	fprintf(stderr, "    http://forum.amule.org/board.php?boardid=67&sid=/\n");
	fprintf(stderr, "You should also try to generate a real backtrace of this error, please read:\n");
	fprintf(stderr, "    http://www.amule.org/wiki/index.php/Backtraces\n");
	fprintf(stderr, "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n\n");
	fprintf(stderr, "aMule version is: %s\n\n", (const char *)unicode2char(GetMuleVersion()));
	
	otherfunctions::print_backtrace(1); // 1 == skip this function.
	
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");	
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

void CamuleApp::OnUDPDnsDone(wxEvent& e)
{
	wxMuleInternalEvent& evt = *((wxMuleInternalEvent*)&e);
	CServerUDPSocket* socket=(CServerUDPSocket*)evt.GetClientData();	
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
	statistics->CompUpDatarateOverhead();
	statistics->CompDownDatarateOverhead();

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
		theApp.sharedfiles->Process();
		
		if( serverconnect->IsConnecting() && !serverconnect->IsSingleConnect() ) {
			serverconnect->TryAnotherConnectionrequest();
		}
		if (serverconnect->IsConnecting()) {
			serverconnect->CheckForTimeout();
		}
		listensocket->UpdateConnectionsStatus();
		
		//printf("Bytes sent this second: %i\n",sent);
		sent = 0;
	}

	
	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		listensocket->Process();
		OnlineSig(); // Added By Bouc7
		// Stats tree is updated every 5 seconds. Maybe we should make it match prefs.
		statistics->UpdateStatsTree();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		wxString buffer;
		
		wxConfigBase* cfg = wxConfigBase::Get();
		buffer = wxString::Format(wxT("%llu"),(long long unsigned int)(theApp.statistics->GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded()));
		cfg->Write(wxT("/Statistics/TotalDownloadedBytes"), buffer);

		buffer = wxString::Format(wxT("%llu"),(long long unsigned int)(theApp.statistics->GetSessionSentBytes()+thePrefs::GetTotalUploaded()));
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
			printf("File not added to sharedlist: %s\n", (const char *)unicode2char(result->GetFileName()));
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

bool CamuleApp::AddServer(CServer *srv)
{
	if ( serverlist->AddServer(srv) ) {
		Notify_ServerAdd(srv);
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
	applog->Sync();

	if (enable_stdout_log) { 
		puts(unicode2char(full_line));
	}

}
#else
void CamuleApp::AddLogLine(const wxString &msg)
{
	wxString curr_date = wxDateTime::Now().FormatISODate() + wxT(" ") +
		wxDateTime::Now().FormatISOTime() + wxT(": ");
	applog->Write(curr_date + msg + wxT("\n"));
	applog->Flush();

	if (enable_stdout_log) {
		Unicode2CharBuf date_str_buf(unicode2char(curr_date));
		// conversion may fail, so must check date_str
		if (date_str_buf) {
			fputs(date_str_buf, stdout);
		}
		Unicode2CharBuf c_msg_buf(unicode2char(msg));
		// conversion may fail, so must check c_msg
		if (c_msg_buf) {
			fputs(c_msg_buf, stdout);
		}
	}
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
	ConfigDir = GetConfigDir();
	wxFile *logfile = new wxFile();
	logfile->Open(ConfigDir + wxFileName::GetPathSeparator() + wxT("logfile"));
	if ( !logfile->IsOpened() ) {
		return wxTRANSLATE("ERROR: can't open logfile");
	}
	int len = logfile->Length();
	if ( len == 0 ) {
		return wxTRANSLATE("WARNING: logfile is empty. Something is wrong.");
	}
	char *tmp_buffer = new char[len + sizeof(wxChar)];
	logfile->Read(tmp_buffer, len);
	memset(tmp_buffer + len, 0, sizeof(wxChar));
#if wxCHECK_VERSION(2,5,3)
	#if wxUSE_UNICODE
	wxString str((wxWCharBuffer&)tmp_buffer);
	#else
	wxString str(tmp_buffer);
	#endif
#else
	#if wxUSE_UNICODE
	wxString str(UTF82unicode(tmp_buffer));
	#else
	wxString str(tmp_buffer);
	#endif
#endif
	delete [] tmp_buffer;
	delete logfile;
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
DEFINE_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
