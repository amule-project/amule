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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "amule.h"
// implementations of other header files
#pragma implementation "CTypedPtrList.h"
#pragma implementation "GetTickCount.h"
#pragma implementation "GuiEvents.h"
#pragma implementation "updownclient.h"
#endif

#include <cmath>
#include <csignal>
#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <wx/defs.h>
#include <wx/process.h>

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
					//   HAVE_SYS_RESOURCE_H, HAVE_SYS_STATVFS_H and VERSION
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
#include "ServerConnect.h"		// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "FriendList.h"			// Needed for CFriendList
#include "updownclient.h"		// Needed for CUpDownClient
#include "Packet.h"
#include "Statistics.h"
#include "AICHSyncThread.h"
#include "Logger.h"
#include "Format.h"

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


void OnShutdownSignal( int /* sig */ ) 
{
	static bool terminating = false;

	if ( !terminating ) {
		terminating = true;
		printf("Shutdown requested, terminating.\n");
#ifndef AMULE_DAEMON
		if ( theApp.amuledlg ) {
			((wxWindow*)theApp.amuledlg)->Close( true );
		} else {
			printf("Dialog not found, forcing termination.\n");
			exit( 1 );
		}
#else
		theApp.ExitMainLoop();
#endif
	} else {
		printf("Forced termination!\n");
		exit( 1 );
	}
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
	
	clientlist	= NULL;
	searchlist	= NULL;
	knownfiles	= NULL;
	serverlist	= NULL;
	serverconnect	= NULL;
	sharedfiles	= NULL;
	listensocket	= NULL;
	clientudp	= NULL;
	clientcredits	= NULL;
	friendlist = NULL;
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

	if (friendlist) {
		delete friendlist;
		friendlist = NULL;
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
		applog = NULL;
	}
	
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
#ifndef __WXMSW__
	// get rid of sigpipe
	signal(SIGPIPE, SIG_IGN);
	// Handle sigint and sigterm
	signal(SIGINT, OnShutdownSignal);
	signal(SIGTERM, OnShutdownSignal);
#endif
	
	sent = 0;
	
	// This can't be on constructor or wx2.4.2 doesn't set it.	
	SetVendorName(wxT("TikuWarez"));
	
	SetAppName(wxT("aMule"));

	OSType = wxGetOsDescription().BeforeFirst( wxT(' ') );
	if ( OSType.IsEmpty() ) {
		OSType = wxT("Unknown");
	}	
	
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
		printf("aMule %s (OS: %s)\n", (const char *)unicode2char(GetMuleVersion()), (const char*)unicode2char(OSType));
#else
		printf("aMule Daemon %s (OS: %s)\n", (const char *)unicode2char(GetMuleVersion()), (const char*)unicode2char(OSType));
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

	printf("Checking if there is an instance already running...\n");
	// see if there is another instance running
	wxString server = ConfigDir + wxT("muleconn");
	wxString host = wxT("localhost");
	wxString IPC = wxT("aMule IPC TESTRUN");
	wxClient* client = new wxClient();
	wxConnectionBase* conn = client->MakeConnection(host, server, IPC);
	// If the connection failed, conn is NULL
	if ( conn ) {
		// An instance is already running!
		printf("There is an instance of aMule already running\n");
		// This is very tricky. The most secure way to communicate is via ED2K links file
		FILE *ed2kfile;
		char filename[1024];

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

#ifndef __WINDOWS__
	// Backwards compatibility, check for old config file if our amule.conf file doesn't exist
	wxString homeDir = wxGetHomeDir() + wxFileName::GetPathSeparator();
	if ( !wxFileExists( ConfigDir + wxT("amule.conf") ) ) {
		// Check if an old config file exists
		#ifdef __APPLE__
		// Mac is a special case
		wxString OldConfig = wxT("Library/Preferences/eMule Preferences");
		#else
		// *BSD, linux, unix, solaris, whatever
		wxString OldConfig = wxT(".eMule");
		#endif
		if ( wxFileExists( homeDir + OldConfig ) ) {
			wxCopyFile( homeDir + OldConfig, ConfigDir + wxT("amule.conf") );
 		}
 	}
#endif

	// This creates the CFG file we shall use
	wxConfigBase* cfg = new wxConfig( wxEmptyString, wxEmptyString, ConfigDir + wxT("amule.conf") );
	
	// Set the config object as the global cfg file
	wxConfig::Set( cfg );


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

	// Some sanity check
	if (!thePrefs::UseTrayIcon()) {
		thePrefs::SetMinToTray(false);
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
	wxChar ch = thePrefs::GetLanguageID().GetChar(0);
	if (ch >= '0' && ch <= '9') {
		wxString info(_("Your locale has been changed to System Default due to a configuration change. Sorry.\n"));
		thePrefs::SetLanguageID(wxLang2Str(wxLANGUAGE_DEFAULT));
		ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
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
				if ( (!strncmp(entries.mnt_fstype, "vfat",4)) 
					|| (!strncmp(entries.mnt_fstype, "fat",3)) 
					|| (!strncmp(entries.mnt_fstype, "msdos",5)) 
					|| (!strncmp(entries.mnt_fstype, "smbfs",5)) 
				) {
					#if wxUSE_UNICODE
					if ( tempdir.StartsWith( UTF82unicode(entries.mnt_mountp )) ) {
					#else 
					if ( tempdir.StartsWith( char2unicode(entries.mnt_mountp )) ) {
					#endif
		#else
			struct mntent* entries;
			entries = getmntent(mnt_tab);
			while ( entries ) {
				if ( (!strncmp(entries->mnt_type, "vfat",4)) 
					|| (!strncmp(entries->mnt_type, "fat",3)) 
					|| (!strncmp(entries->mnt_type, "msdos",5)) 
					|| (!strncmp(entries->mnt_type, "smbfs",5)) 
				) {
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
#if defined(HAVE_SYS_STATVFS_H) && !defined(__FREEBSD__)	
	struct statvfs *mntbuf;
#else
	struct statfs *mntbuf;
#endif

	size = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < size; i++) {
		if ( (!strncmp(mntbuf[i].f_fstypename,"vfat",4))
			|| (!strncmp(mntbuf[i].f_fstypename,"fat",3))
			|| (!strncmp(mntbuf[i].f_fstypename,"msdos",5))
			|| (!strncmp(mntbuf[i].f_fstypename,"smbfs",5))
		) {
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

	statistics = new CStatistics();
	
	// Ready file-hasher
	CAddFileThread::Start();

	clientlist	= new CClientList();
	friendlist = new CFriendList();
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

	// reload shared files
	sharedfiles->Reload(true);
	
	// Ensure that the up/down ratio is used
	CPreferences::CheckUlDlRatio();

	// The user can start pressing buttons like mad if he feels like it.
	m_app_state = APP_STATE_RUNNING;
	
	// Kry - Load the sources seeds on app init
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->LoadSourceSeeds();
	}
	
	// Autoconnect if that option is enabled
	if (thePrefs::DoAutoConnect()) {
		AddLogLineM(true, _("Connecting"));
		theApp.serverconnect->ConnectToAnyServer();
	}

	// No webserver on Win at all (yet)
#ifndef __WXMSW__
	// Run webserver?
	if (thePrefs::GetWSIsEnabled()) {
		wxString aMuleConfigFile(ConfigDir + wxT("amule.conf"));
		#ifndef AMULE_DAEMON
		webserver_pid = wxExecute(wxString(wxT("amuleweb --amule-config-file=")) + aMuleConfigFile);
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
				execlp("amuleweb", "amuleweb", (const char *)unicode2char(wxT("--amule-config-file=") + aMuleConfigFile), 0);
				printf("execlp failed with code %d\n", errno);
				exit(0);
			} else {
				// wait few seconds to give amuleweb chance to start or forked child to exit
				sleep(3);
				if (wxProcess::Exists(pid)) {
					printf("aMuleweb is running on pid %d\n", pid);
					webserver_pid = pid;
				} else {
					printf("ERROR: aMuleweb not started\n");
				}
			}
		}
		#endif
	}
#endif /* ! __WXMSW__ */

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
			_(" is not available. You will be LOWID") << wxT("\n");
		*msg << err;
		AddLogLineM(true, err);
		err.Clear();
		err << _("Port ") << thePrefs::GetPort() << _(
			" is not available!\n\n"
			"This means that you will be LOWID.\n\n"
			"Check your network to make sure the port is open for output and input.");
		ShowAlert(err, _("Error"), wxOK | wxICON_ERROR);
	}

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
		AddLogLineM(true, wxString(_("Failed to create"))+_(" OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}
	
	if ( !amulesig_out.Create(m_amulesig_path) ) {
		AddLogLineM(true, wxString(_("Failed to create"))+_(" aMule OnlineSig File"));
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
	}
	
	// eMule signature finished here. Write the line to the wxTextFile.
	emulesig_out.AddLine(emulesig_string);

	// Now for aMule signature extras
	
	// Nick on the network
	amulesig_out.AddLine(thePrefs::GetUserNick());

	// Total received in bytes
	amulesig_out.AddLine( CFormat( wxT("%llu") ) %
		(uint64)(theApp.statistics->GetSessionReceivedBytes() +
		thePrefs::GetTotalDownloaded()) );

	// Total sent in bytes
	amulesig_out.AddLine( CFormat( wxT("%llu") ) %
		(uint64)(theApp.statistics->GetSessionSentBytes() +
		thePrefs::GetTotalUploaded()) );

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
			(uint64)theApp.statistics->GetSessionReceivedBytes() );

        // Total sent bytes in session
		amulesig_out.AddLine( CFormat( wxT("%llu") ) %
			(uint64)theApp.statistics->GetSessionSentBytes() );
		
		// Uptime
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
	fprintf(stderr, "aMule version is: %s\n", (const char *)unicode2char(GetMuleVersion()));
	fprintf(stderr, "Running on: %s\n\n", (const char*)unicode2char(wxGetOsDescription()));
	
	otherfunctions::print_backtrace(1); // 1 == skip this function.
	
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
	info += _("Feel free to report any bugs to forum.amule.org");

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
void CamuleApp::OnAssert(const wxChar *file, int line, 
						 const wxChar *cond, const wxChar *msg)
{
	printf("\nAssertion failed. Backtrace follows:\n");

	// Skip the function-calls directly related to the assert call.
	print_backtrace( 5 );

	printf("\n");
		
	if ( wxThread::IsMain() ) {
		AMULE_APP_BASE::OnAssert( file, line, cond, msg );
	} else {
		wxString errmsg = CFormat( wxT("%s:%d: Assertion '%s' failed. %s") )
			% file % line % cond % ( msg ? msg : wxT("") );

		printf("%s\n", (const char*)unicode2char( errmsg ));
		
		// Abort, allows gdb to catch the assertion
		raise( SIGABRT );
	}
}
#endif


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
	if(!IsRunning()) {
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
	static uint32	msPrev1, msPrev5, msPrevSave, msPrevHist, msPrevOS;
	uint32 msCur = statistics->GetUptimeMsecs();

	if (!IsRunning()) {
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
		
		sent = 0;
	}

	
	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		listensocket->Process();
		// Stats tree is updated every 5 seconds. Maybe we should make it match prefs.
		statistics->UpdateStatsTree();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		wxString buffer;
		
		wxConfigBase* cfg = wxConfigBase::Get();
		buffer = wxString::Format( wxT("%llu"), theApp.statistics->GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded() );
		cfg->Write(wxT("/Statistics/TotalDownloadedBytes"), buffer);

		buffer = wxString::Format( wxT("%llu"), theApp.statistics->GetSessionSentBytes()+thePrefs::GetTotalUploaded() );
		cfg->Write(wxT("/Statistics/TotalUploadedBytes"), buffer);
	}

	// Special
	if (msCur-msPrevOS >= thePrefs::GetOSUpdate()) {
		OnlineSig(); // Added By Bouc7		
		msPrevOS = msCur;
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
			AddDebugLogLineM(false, logKnownFiles, wxT("Safe adding file to sharedlist: ") + result->GetFileName());			
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
			AddDebugLogLineM(false, logKnownFiles, wxT("File not added to sharedlist: ") + result->GetFileName());
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
	
	// Kry - Save the sources seeds on app exit
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->SaveSourceSeeds();
	}

	OnlineSig(true); // Added By Bouc7

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
