// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#include <unistd.h>           // Needed for close(2) and sleep(3)
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

	#include <X11/Xlib.h>		// Needed for XParseGeometry
	#include <gdk/gdk.h>
	#include <gtk/gtk.h>
	
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT, HAVE_SYS_RESOURCE_H,
#endif				//   LOCALEDIR, PACKAGE, PACKAGE_STRING and VERSION

#ifdef __WXMAC__
	#warning Kry? Get rid of this!
	#include <wx/wx.h>
#endif
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>                // Needed for wxFileName::GetPathSeparator()
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/clipbrd.h>			// Needed for wxClipBoard
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/splash.h>			// Needed for wxSplashScreen
#include <wx/utils.h>
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/textfile.h>		// Needed for wxTextFile
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/msgdlg.h>			// Needed for wxMessageBox
#include <wx/url.h>

#include "amule.h"				// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "server.h"				// Needed for GetListName
#include "CFile.h"				// Needed for CFile
#include "otherfunctions.h"		// Needed for GetTickCount
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerWnd.h"			// Needed for CServerWnd
#include "StatisticsDlg.h"		// Needed for CStatisticsDlg
#include "IPFilter.h"			// Needed for CIPFilter
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "sockets.h"			// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"	// Needed for CServerSocket
#include "UDPSocket.h"		// Needed for CUDPSocket
#include "PartFile.h"		// Needed for CPartFile
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "packets.h"

#warning This ones must be removed ASAP - exception: amuledlg, will be the LAST one.
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"		// Needed for CQueueListCtrl
#include "UploadListCtrl.h"		// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "ChatWnd.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

BEGIN_EVENT_TABLE(CamuleApp, wxApp)

	// Socket handlers
		// Listen Socket
		EVT_SOCKET(LISTENSOCKET_HANDLER, CamuleApp::ListenSocketHandler)
		// Clients sockets
		//EVT_SOCKET(CLIENTREQSOCKET_HANDLER, CamuleApp::ClientReqSocketHandler)
		// UDP Socket (servers)
		EVT_SOCKET(UDPSOCKET_HANDLER, CamuleApp::UDPSocketHandler)
		// UDP Socket (clients)
		EVT_SOCKET(CLIENTUDPSOCKET_HANDLER, CamuleApp::ClientUDPSocketHandler)
		// Server Socket
		//EVT_SOCKET(SERVERSOCKET_HANDLER, CamuleApp::ServerSocketHandler)

	// Socket timers (TCP + UDO)
		EVT_TIMER(TM_UDPSOCKET, CamuleApp::OnUDPTimer)
		EVT_TIMER(TM_TCPSOCKET, CamuleApp::OnTCPTimer)

	// Core timer
		EVT_TIMER(ID_CORETIMER, CamuleApp::OnCoreTimer)

	// Async dns handling
		EVT_MENU(TM_DNSDONE, CamuleApp::OnDnsDone)
		EVT_MENU(TM_SOURCESDNSDONE, CamuleApp::OnSourcesDnsDone)

	// Hash ended notifier
		EVT_MENU(TM_FINISHEDHASHING, CamuleApp::OnFinishedHashing)

	// Hashing thread finished and dead
		EVT_MENU(TM_HASHTHREADFINISHED, CamuleApp::OnHashingShutdown)

	// File completion ended notifier
		EVT_MENU(TM_FILECOMPLETIONFINISHED, CamuleApp::OnFinishedCompletion)

END_EVENT_TABLE()

IMPLEMENT_APP(CamuleApp)


// Global timer. Used to cache GetTickCount() results for better performance.
class MyTimer *mytimer;


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


int CamuleApp::OnExit()
{

	if (core_timer) {
		// Stop the Core Timer
		delete core_timer;
	}

	if (m_app_state!=APP_STATE_STARTING) {
		printf("Now, exiting main app...\n");
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

	if (glob_prefs) {
		delete glob_prefs;
		glob_prefs = NULL;
	}

	if (localserver) {
		delete localserver;
		localserver = NULL;
	}
	
	if (mytimer) {
		delete mytimer;
	}

	if (m_app_state!=APP_STATE_STARTING) {
		printf("aMule shutdown completed.\n");
	}

	// Return 0 for succesful program termination
	return wxApp::OnExit();
}


bool CamuleApp::OnInit()
{
	m_app_state = APP_STATE_STARTING;
	ConfigDir = wxGetHomeDir() + wxFileName::GetPathSeparator() + wxT(".aMule") + wxFileName::GetPathSeparator();

	// Initialization
	IsReady			= false;
	amuledlg 		= NULL;
	clientlist			= NULL;
	searchlist		= NULL;
	knownfiles		= NULL;
	serverlist		= NULL;
	serverconnect	= NULL;
	sharedfiles		= NULL;
	listensocket		= NULL;
	clientudp			= NULL;
	clientcredits		= NULL;
	downloadqueue	= NULL;
	uploadqueue 	= NULL;
	ipfilter			= NULL;

	// Default geometry of the GUI. Can be changed with a cmdline argument...
	bool geometry_enabled = false;
	// Standard size is 800x600 at position (0,0)
	int geometry_x = 0;
	int geometry_y = 0;
	unsigned int geometry_width = 800;
	unsigned int geometry_height = 600;

// reset statistic values
	stat_sessionReceivedBytes = 0;
	stat_sessionSentBytes = 0;
	stat_reconnects = 0;
	stat_transferStarttime = 0;
	stat_serverConnectTime = 0;
	sTransferDelay = 0.0;


	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	mytimer = new MyTimer();

	Start_time = GetTickCount64();

#ifndef __WXMSW__
	// catch fatal exceptions
	wxHandleFatalExceptions(true);
#endif

	// Apprently needed for *BSD
	SetResourceLimits();


	// Parse cmdline arguments.
	wxCmdLineParser cmdline(wxApp::argc, wxApp::argv);

	// Handle these arguments.
	cmdline.AddSwitch(wxT("v"), wxT("version"), wxT("Displays the current version number."));
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("Displays this information."));
	cmdline.AddSwitch(wxT("i"), wxT("enable-stdin"), wxT("Does not disable stdin."));
	cmdline.AddOption(wxT("geometry"), wxT(""), wxT("Sets the geometry of the app.\n\t\t\t<str> uses the same format as standard X11 apps:\n\t\t\t[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"));
	cmdline.Parse();

	if ( cmdline.Found(wxT("version")) ) {
		printf("aMule %s\n", VERSION);

		return false;
	}

	if ( cmdline.Found(wxT("help")) ) {
		cmdline.Usage();

		return false;
	}

	
	wxString geom_string;
	if ( cmdline.Found(wxT("geometry"), &geom_string) ) {

		// I plan on moving this to a seperate function, as it just clutters up OnInit()

#ifdef __WXGTK__

		XParseGeometry(unicode2char(geom_string), &geometry_x, &geometry_y, &geometry_width, &geometry_height);

		geometry_enabled = true;

#elif defined (__WXMSW__)
		/*
			This implementation might work with mac, provided that the
			SetSize() function works as expected.
		*/

		// Remove possible prefix
		if ( geom_string.GetChar(0) == '=' )
			geom_string.Remove( 0, 1 );

		// Stupid ToLong functions forces me to use longs =(
		long width = geometry_width;
		long height = geometry_height;

		// Get the avilable display area
		wxRect display = wxGetClientDisplayRect();

		// We want to place aMule inside the client area by default
		long x = display.x;
		long y = display.y;

		// Tokenize the string
		wxStringTokenizer tokens(geom_string, "xX+-");

		// First part: Program width
		if ( tokens.GetNextToken().ToLong( &width ) ) {

			wxString prefix = geom_string[ tokens.GetPosition() - 1 ];
			if ( prefix == "x" || prefix == "X" ) {

				// Second part: Program height
				if ( tokens.GetNextToken().ToLong( &height ) ) {

					prefix = geom_string[ tokens.GetPosition() - 1 ];
					if ( prefix == "+" || prefix == "-" ) {

						// Third part: X-Offset
						if ( tokens.GetNextToken().ToLong( &x ) ) {
							if ( prefix == "-" )
								x = display.GetRight() - ( width + x );

							prefix = geom_string[ tokens.GetPosition() - 1 ];
							if ( prefix == "+" || prefix == "-" ) {

								// Fourth part: Y-Offset
								if ( tokens.GetNextToken().ToLong( &y ) ) {
									if ( prefix == "-" )
										y = display.GetBottom() - ( height + y );
								}
							}
						}
					}

					// We need at least height and width to override default geomtry
					geometry_enabled = true;

					geometry_x = x;
					geometry_y = y;
					geometry_width = width;
					geometry_height = height;
				}
			}
		}
#else
		#warning Need to parse the geometry for non-GTK/WIN platforms
#endif

		printf("geometry:  x: %d y: %d width: %d height: %d\n", geometry_x, geometry_y, geometry_width, geometry_height);
	}


	printf("Initialising aMule\n");

	SetVendorName(wxT("TikuWarez"));

	// Do NOT change this string to aMule nor anything else, it WILL fuck you up.
	SetAppName(wxT("eMule"));


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
			wxMkdir(ConfigDir);

			// Copy .dat files to the aMule dir
			wxString file = wxFindFirstFile(xMulePrefDir + wxFileName::GetPathSeparator() + wxT("*.dat"), wxFILE);
  			while ( !file.IsEmpty() ) {
				wxCopyFile( file, ConfigDir + wxFileName::GetPathSeparator() + file.AfterLast(wxFileName::GetPathSeparator()));

				file = wxFindNextFile();
  			}

			// Copy .met files to the aMule dir
			file = wxFindFirstFile(xMulePrefDir + wxFileName::GetPathSeparator() + wxT("*.met"), wxFILE);
  			while ( !file.IsEmpty() ) {
				wxCopyFile( file, ConfigDir + wxFileName::GetPathSeparator() + file.AfterLast(wxFileName::GetPathSeparator()));

				file = wxFindNextFile();
  			}

			wxMessageBox(_("Copied old ~/.xMule config and credit files to ~/.aMule\nHowever, be sure NOT to remove .xMule if your Incoming / Temp folders are still there ;)"), _("Info"), wxOK);
		} else {
			// No settings to import, new to build.
			wxMkdir(ConfigDir);
		}
	}


	// Delete old log file.
	wxString logname(ConfigDir + wxT("logfile"));
	wxRemoveFile(logname);
	wxTextFile file(logname);
	if (!file.Create()) {
		printf("Error creating log file!\n");
	}
	file.Close();
	// Load Preferences
	glob_prefs = new CPreferences();

	// Build the filenames for the two OS files
	SetOSFiles(glob_prefs->GetOSDir());

	// Create the Core timer
	core_timer=new wxTimer(this,ID_CORETIMER);
	if (!core_timer) {
		printf("Fatal Error: Failed to create Core Timer");
		OnExit();
	}

	// Display notification on new version or first run
	wxTextFile vfile( ConfigDir + wxFileName::GetPathSeparator() + wxT("lastversion") );
	wxString newMule(wxT(VERSION));
	if ( wxFileExists( vfile.GetName() ) && vfile.Open() && !vfile.Eof() ) {
		if ( vfile.GetFirstLine() != newMule ) {
			Trigger_New_version( vfile.GetFirstLine(), newMule );

			// Remove prior version
			while ( vfile.GetLineCount() ) {
				vfile.RemoveLine(0);
			}

			vfile.AddLine(newMule);
			vfile.Write();
		}

		vfile.Close();
	} else {
		Trigger_New_version( wxT("pre_2.0.0rc1"), newMule );

		// If we failed to open the file, create it
		if ( !vfile.IsOpened() )
			vfile.Create();

		vfile.AddLine(wxT(VERSION));
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
		wxString incomingdir = glob_prefs->GetIncomingDir();
		wxString tempdir = glob_prefs->GetTempDir();
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
	wxString incomingdir = glob_prefs->GetIncomingDir();
	wxString tempdir = glob_prefs->GetTempDir();
	long size, i;
	struct statfs *mntbuf;

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

	// Create Hashing thread
	CAddFileThread::Setup();


	clientlist		= new CClientList();
	searchlist		= new CSearchList();
	knownfiles		= new CKnownFileList();
	serverlist		= new CServerList(glob_prefs);
	serverconnect	= new CServerConnect(serverlist, glob_prefs);
	sharedfiles		= new CSharedFileList(glob_prefs, serverconnect, knownfiles);

	wxIPV4address myaddr;
	myaddr.AnyAddress();
	myaddr.Service(glob_prefs->GetUDPPort());
	printf("*** TCP socket at %d\n", glob_prefs->GetPort());
	clientudp		= new CClientUDPSocket(myaddr);

	clientcredits	= new CClientCreditsList(glob_prefs);
	downloadqueue	= new CDownloadQueue(glob_prefs, sharedfiles);	// bugfix - do this before creating the uploadqueue
	uploadqueue		= new CUploadQueue(glob_prefs);
	ipfilter		= new CIPFilter();

	// Create listen socket 
	myaddr.Service(glob_prefs->GetPort());
	listensocket = new CListenSocket(glob_prefs, myaddr);

	// Create main dialog
	// Should default/last-used position be overridden?
	if (geometry_enabled ) {
		amuledlg = new CamuleDlg(NULL, wxString::Format(wxT("aMule %s"), wxT(VERSION)), wxPoint(geometry_x,geometry_y), wxSize( geometry_width, geometry_height - 58 ));
	} else {
		amuledlg = new CamuleDlg(NULL, wxString::Format(wxT("aMule %s"), wxT(VERSION)));
	}

	// Get ready to handle connections from apps like amulecmd
	ECServerHandler = new ExternalConn();

	serverlist->Init();

	// init downloadqueue
	downloadqueue->Init();

	sharedfiles->SetOutputCtrl((CSharedFilesCtrl *) amuledlg->sharedfileswnd->FindWindow(wxT("sharedFilesCt")));


	SetTopWindow(amuledlg);

	m_app_state = APP_STATE_RUNNING;

	// reload shared files
	sharedfiles->Reload(true, true);

	listensocket->StartListening();

	// If we wern't able to start listening, we need to warn the user

	if ( !listensocket->Ok() ) {
		GUIEvent event(ADDLOGLINE);
		event.string_value = wxString::Format(_("Port %d is not available. You will be LOWID"), glob_prefs->GetPort());
		event.byte_value = true;
		NotifyEvent(event);
		#warning we need to move this lowid warning to the GUI itself.
		wxMessageBox(wxString::Format(_("Port %d is not available !!\n\nThis mean that you will be LOWID.\n\nCheck your network to make sure the port is open for output and input."), glob_prefs->GetPort()), _("Error"), wxOK | wxICON_ERROR);
	}


	// Autoconnect if that option is enabled
	if (glob_prefs->DoAutoConnect()) {
		wxCommandEvent nullEvt;
		amuledlg->OnBnConnect(nullEvt);
	}


	// Calculate maximum download-rate
	if ( glob_prefs->GetMaxDownload() == 0 && glob_prefs->GetMaxUpload() < 10)
		glob_prefs->SetDownloadlimit((glob_prefs->GetMaxUpload()*4)) ;
        if( glob_prefs->GetMaxUpload() != 0 && glob_prefs->GetMaxUpload() !=UNLIMITED){

	if( glob_prefs->GetMaxUpload() < 4 &&
		( glob_prefs->GetMaxUpload()*3 < glob_prefs->GetMaxDownload() ) )
		glob_prefs->SetDownloadlimit((glob_prefs->GetMaxUpload()*3));

	if( glob_prefs->GetMaxUpload() < 10 &&
		( glob_prefs->GetMaxUpload()*4 < glob_prefs->GetMaxDownload() ) )
	glob_prefs->SetDownloadlimit((glob_prefs->GetMaxUpload()*4)) ;
	}

	// The user may now click on buttons
	IsReady = true;

	// Kry - Load the sources seeds on app init
	if (glob_prefs->GetSrcSeedsOn()) {
		downloadqueue->LoadSourceSeeds();
	}

	// Everything should be loaded now, so sort the list widgets
	amuledlg->InitSort();
	
	// Start the Core Timer

	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	core_timer->Start(100);	

	// Start the Gui Timer
	
	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	amuledlg->StartGuiTimer();
	
	return true;
}


// Updates the number of received bytes and marks when transfers first began
void CamuleApp::UpdateReceivedBytes(int32 bytesToAdd)
{
	SetTimeOnTransfer();
	stat_sessionReceivedBytes += bytesToAdd;
}


// Updates the number of received bytes and marks when transfers first began
void CamuleApp::UpdateSentBytes(int32 bytesToAdd)
{
	SetTimeOnTransfer();
	stat_sessionSentBytes += bytesToAdd;
}


// Saves the time where transfers were started and calucated the time before
void CamuleApp::SetTimeOnTransfer()
{
	if (stat_transferStarttime)
		return;

	stat_transferStarttime = GetTickCount64();
	sTransferDelay = (stat_transferStarttime - Start_time)/1000.0;
}


// Returns the uptime in millie-seconds
uint64 CamuleApp::GetUptimeMsecs()
{
	return GetTickCount64() - Start_time;
}


// Returns the uptime in seconds
uint32 CamuleApp::GetUptimeSecs()
{
	return GetUptimeMsecs() / 1000;
}


// Returns the amount of time where transfers have been going on
uint32 CamuleApp::GetTransferSecs()
{
	return ( GetTickCount64() - stat_transferStarttime ) / 1000;
}


// Returns the amount of time where we've been connected to a server
uint32 CamuleApp::GetServerSecs()
{
	return ( GetTickCount64() - stat_serverConnectTime) / 1000;
}


// Strips specific chars to ensure legal filenames
wxString CamuleApp::StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces)
{
	wxString result;

	for ( unsigned int i = 0; i < strText.Length(); i++ ) {
		switch ( strText.GetChar(i) ) {
			case wxT('\"'):
			case wxT('*'):
			case wxT('<'):
			case wxT('>'):
			case wxT('?'):
			case wxT('|'):
			case wxT('\\'):
			case wxT('/'):
			case wxT(':'):
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
wxString CamuleApp::CreateED2kLink(CAbstractFile* f)
{
	wxString strURL;

	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	strURL << wxT("ed2k://|file|")
	       << StripInvalidFilenameChars(f->GetFileName(), true)
		   << wxT("|")
	       << f->GetFileSize()
		   << wxT("|")
		   << EncodeBase16( f->GetFileHash(), 16 )
		   << wxT("|/");

	return strURL;
}


// Returns a ed2k source URL
wxString CamuleApp::CreateED2kSourceLink(CAbstractFile* f)
{
	if ( !serverconnect->IsConnected() || serverconnect->IsLowID() ) {
		wxMessageBox(_("You need a HighID to create a valid sourcelink"));
		return wxT("");
	}

	uint32 clientID = serverconnect->GetClientID();

	// Create the first part of the URL
	wxString strURL = CreateED2kLink( f );

	// And append the source information: "|sources,<ip>:<port>|/"
	strURL << wxT("|sources,")
	       << (uint8) clientID << wxT(".")
		   << (uint8) (clientID >> 8) << wxT(".")
		   << (uint8) (clientID >> 16) << wxT(".")
		   << (uint8) (clientID >> 24) << wxT(":")
		   << glob_prefs->GetPort() << wxT("|/");

	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,<ip>:<port>|/"
	return strURL;
}


// Returns a ed2k source URL using a hostname rather than IP. Currently, the
// hostname doesn't appear to be set, thus this function wont work as intended.
wxString CamuleApp::CreateED2kHostnameSourceLink(CAbstractFile* f)
{
	wxString strURL;

	// Create the first part of the URL
	strURL = CreateED2kLink( f );

	// Append the source information: "|sources,<host>:port|/"
	strURL << wxT("|sources,")
	       << glob_prefs->GetYourHostname() << wxT(":")
		   << glob_prefs->GetPort() << wxT("|/");

	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,<host>:<port>|/"
	return strURL;
}


// Creates a ED2k hyperlink
wxString CamuleApp::CreateHTMLED2kLink(CAbstractFile* f)
{
	wxString strCode = wxT("<a href=\"") + CreateED2kLink(f) + wxT("\">") + StripInvalidFilenameChars(f->GetFileName(), true) + wxT("</a>");
	return strCode;
}


// Generates an URL for checking if a file is "fake"
wxString CamuleApp::GenFakeCheckUrl(CAbstractFile *f)
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
wxString CamuleApp::GenFakeCheckUrl2(CAbstractFile *f)
{
	
    wxString strURL = wxT("http://www.jugle.net/?fakecheck=%s");
	
	strURL = wxURL::ConvertToValidURI( strURL +  CreateED2kLink( f ) );

	strURL.Replace(wxT("\""), wxT("%22"));
	strURL.Replace(wxT("'"),  wxT("%27"));
	strURL.Replace(wxT("`"),  wxT("%60"));

	return strURL;
}

// Sets the contents of the clipboard. Prior content  erased.
bool CamuleApp::CopyTextToClipboard(wxString strText)
{
	if (wxTheClipboard->Open()) {
		wxTheClipboard->UsePrimarySelection(TRUE);
		wxTheClipboard->SetData(new wxTextDataObject(strText));
		wxTheClipboard->Close();

		return true;
	} else {
		return false;
	}
}


/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */)
{
	// Do not do anything if online signature is disabled in Preferences
	if (!glob_prefs->IsOnlineSignatureEnabled() || emulesig_path.IsEmpty()) {
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
	sprintf(buffer, "%s", unicode2char(glob_prefs->GetUserNick()));
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total received in bytes
	sprintf(buffer, "%llu", (stat_sessionReceivedBytes+glob_prefs->GetTotalDownloaded()));
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total sent in bytes
	sprintf(buffer, "%llu", (stat_sessionSentBytes+glob_prefs->GetTotalUploaded()));
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
		sprintf(buffer, "%llu", stat_sessionReceivedBytes);
        	amulesig_out.Write(buffer, strlen(buffer));
        	amulesig_out.Write("\n",1);
	}

        // Total sent bytes in session
	if (zero) {
		amulesig_out.Write("0",1);
		amulesig_out.Write("\n",1);
	} else {
		sprintf(buffer, "%llu", stat_sessionSentBytes);
        	amulesig_out.Write(buffer, strlen(buffer));
        	amulesig_out.Write("\n",1);
	}

	// Uptime
	if (zero) {
		sprintf(buffer,"%s",unicode2char(CastSecondsToHM(0).GetData()));
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	} else {
		sprintf(buffer,"%s",unicode2char(CastSecondsToHM(GetUptimeSecs())));
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	}

	// Close the files
	emulesig_out.Close();
	amulesig_out.Close();

} //End Added By Bouc7

// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
#ifndef __WXMSW__
	// Close sockets first.
	if ( listensocket )
		listensocket->Destroy();
	if ( clientudp )
		clientudp->Destroy();

	// (stkn) create backtrace
#ifdef __WXGTK__
#ifndef __BSD__
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

	fprintf(stderr, "\nOOPS! - Seems like aMule crashed\n--== BACKTRACE FOLLOWS: ==--\n\n");
	for (int i = 0; i < num_entries; i++) {
		fprintf(stderr, "[%d] %s\n", i, bt_strings[i]);
	}
	free(bt_strings);
#endif

#ifdef __BSD__
	fprintf(stderr, "\nOOPS! - Seems like aMule crashed\n--== no BACKTRACE yet \n\n");
#endif // __BSD__
	
#endif
#endif
}

#define wxGTK_WINDOW 1
#define SHIFT (8 * (sizeof(short int) - sizeof(char)))

static bool GetColourWidget(int &red, int &green, int &blue, int type)
{
#ifdef __WXGTK__
	GtkWidget *widget;
        GtkStyle *def;

        if (type == wxGTK_WINDOW) {
                widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                (def = gtk_rc_get_style(widget)) ? : (def =gtk_widget_get_default_style());
        }
        else return FALSE;

        GdkColor *col;
        col = def->bg;
        red = col[GTK_STATE_NORMAL].red;
        green = col[GTK_STATE_NORMAL].green;
        blue = col[GTK_STATE_NORMAL].blue;
        gtk_widget_destroy(widget);
        return TRUE;
#else
	  return FALSE;
#endif
}


// external helper function
wxColour GetColour(wxSystemColour what)
{
	static wxColour *_systemWindowColour = NULL;

	switch (what) {
		case wxSYS_COLOUR_WINDOW:
			if (!_systemWindowColour) {
				int red, green, blue;
				if (!GetColourWidget(red, green, blue, wxGTK_WINDOW)) {
					red = green = blue = 0x9c40;
				}
				_systemWindowColour = new wxColour(red >> SHIFT, green >> SHIFT, blue >> SHIFT);
			}
			return *_systemWindowColour;
			break;
		default:
			break;
	}
	return true;
}


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
	int language;

	#define  wxLANGUAGE_CUSTOM 				wxLANGUAGE_USER_DEFINED+1
	#define  wxLANGUAGE_ITALIAN_NAPOLITAN 	wxLANGUAGE_USER_DEFINED+2

	wxLanguageInfo CustomLanguage;
	CustomLanguage.Language = wxLANGUAGE_ITALIAN_NAPOLITAN;
	CustomLanguage.CanonicalName = wxT("it_NA");
	CustomLanguage.Description = wxT("sNeo's Custom Napolitan Language");
	wxLocale::AddLanguage(CustomLanguage);

	CustomLanguage.Language = wxLANGUAGE_CUSTOM;
	CustomLanguage.CanonicalName = wxT("aMule_custom");
	CustomLanguage.Description = wxT("aMule's custom language");

	switch (glob_prefs->GetLanguageID()) {
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
			language = wxLANGUAGE_CHINESE;
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
			//strcpy(newlang,"it_NA");
			language = wxLANGUAGE_ITALIAN_NAPOLITAN;
			break;
		case 28:
			//strcpy(newlang,"custom");
			language = wxLANGUAGE_CUSTOM;
			break;
		default:
			language = wxLANGUAGE_DEFAULT;
			break;

	}

	int language_flags = 0;
	
	if ((language != wxLANGUAGE_CUSTOM) && (language != wxLANGUAGE_ITALIAN_NAPOLITAN)) {
		language_flags = wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING;
	}
	
	if ((!m_locale.Init(language,language_flags)) && (language != wxLANGUAGE_DEFAULT) && (language != wxLANGUAGE_CUSTOM)) {
		AddLogLineM(false,_("The selected locale seems not to be installed on your box. (Note: I'll try to set it anyway)"));
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
void CamuleApp::Trigger_New_version(wxString old_version, wxString new_version)
{
	wxString info;

	info = _(" --- This is the first time you run aMule %s ---\n\n");
	info.Replace( wxT("%s"), new_version );

	if (new_version == wxT("CVS")) {
		info += _("This version is a testing version, updated daily, and \n");
		info += _("we give no warranty it won't break anything, burn your house,\n");
		info += _("or kill your dog. But it *should* be safe to use anyway. \n");
	} else if (old_version == wxT("1.2.6")) {
		info += _("This version has new SecureIdent support, so your \n");
		info += _("client credits will be lost on this first run. \n");
		info += _("There is no way to fix that, and eMule did the same.\n");
		info += _("But your hash will be safe against stealers now, and your\n");
		info += _("cryptkey.dat, clients.met and preferences.dat are eMule compatible now.\n");
		info += _("Just take them from your eMule config dir and put then on ~/.aMule.\n");
	} else if (old_version == wxT("2.0.0-rc1")) {
		info += _("This rc2 version fixes most of the rc1 version bugs and adds new features.\n");
		info += _("A full changelog can be found in the Changelog file or at www.amule.org.\n");
	}

	info += _("Your locale has been changed to System Default due to a version change. Sorry.\n");
	info += _("Feel free to report any bugs to forum.amule.org");


	wxMessageBox(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);

	// Set to system default... no other way AFAIK unless we change the save type.

	glob_prefs->SetLanguageID(0);

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


void CamuleApp::FlushQueuedLogLines() {

	QueuedLogLine line_to_add;

	m_LogQueueLock.Enter();

	wxASSERT(amuledlg);

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
		::wxMessageBox(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("Error"), wxOK | wxICON_ERROR);
		emulesig_path = wxEmptyString;
		amulesig_path = wxEmptyString;
	}
}


void CamuleApp::ListenSocketHandler(wxSocketEvent& event) {

	wxASSERT(event.GetSocket()->IsKindOf(CLASSINFO(CListenSocket)));
	CListenSocket * socket = (CListenSocket*) event.GetSocket();

	if(!IsReady || !socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
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


void CamuleApp::ClientReqSocketHandler(wxSocketEvent& event) {

	wxASSERT(event.GetSocket()->IsKindOf(CLASSINFO(CClientReqSocket)));
	CClientReqSocket * socket = (CClientReqSocket *) event.GetSocket();

	if(!IsReady || !socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}

	if (socket->OnDestroy()) {
		return;
	}

	//printf("request at clientreqsocket\n");
	switch(event.GetSocketEvent()) {
		case wxSOCKET_LOST:
			socket->OnError(socket->LastError());
			break;
		case wxSOCKET_INPUT:
			socket->OnReceive(0);
			break;
		case wxSOCKET_OUTPUT:
			socket->OnSend(0);
			break;
		case wxSOCKET_CONNECTION:
			// connection stablished, nothing to do about it?
			break;
		default:
			// connection requests should not arrive here..
			wxASSERT(0);
			break;
	}

}


void CamuleApp::UDPSocketHandler(wxSocketEvent& event) {

	wxASSERT(event.GetSocket()->IsKindOf(CLASSINFO(CUDPSocket)));
	CUDPSocket * socket = (CUDPSocket*) event.GetSocket();

	if(!IsReady || !socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}

	switch(event.GetSocketEvent()) {
		case wxSOCKET_INPUT:
			socket->OnReceive(0);
			break;
		default:
			wxASSERT(0);
			break;
	}

}


void CamuleApp::ServerSocketHandler(wxSocketEvent& event) {
	//printf("Got a server event\n");
	//wxMessageBox(wxString::Format("Got Server Event %u",event.GetSocketEvent()));

	wxASSERT(event.GetSocket()->IsKindOf(CLASSINFO(CServerSocket)));
	CServerSocket * socket = (CServerSocket*) event.GetSocket();

	if(!IsReady || !socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}

	if (socket->OnDestroy()) {
		return;
	}

	switch(event.GetSocketEvent()) {
		case wxSOCKET_CONNECTION:
			socket->OnConnect(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_LOST:
			socket->OnError(socket->LastError());
			break;
		case wxSOCKET_INPUT:
			socket->OnReceive(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_OUTPUT:
			socket->OnSend(wxSOCKET_NOERROR);
			break;
		default:
			wxASSERT(0);
			break;
	}


}

void CamuleApp::ClientUDPSocketHandler(wxSocketEvent& event) {

	wxASSERT(event.GetSocket()->IsKindOf(CLASSINFO(CClientUDPSocket)));
	CClientUDPSocket * socket = (CClientUDPSocket*) event.GetSocket();

	if(!IsReady || !socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}

	switch(event.GetSocketEvent()) {
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

void CamuleApp::OnDnsDone(wxCommandEvent& evt)
{
	CUDPSocket* socket=(CUDPSocket*)evt.GetClientData();
	struct sockaddr_in *si=(struct sockaddr_in*)evt.GetExtraLong();
	socket->DnsLookupDone(si);
}


void CamuleApp::OnSourcesDnsDone(wxCommandEvent& evt)
{
	struct sockaddr_in *si=(struct sockaddr_in*)evt.GetExtraLong();
	downloadqueue->OnHostnameResolved(si);
}

void CamuleApp::OnUDPTimer(wxTimerEvent& WXUNUSED(evt))
{
	if (IsReady) {
		serverlist->SendNextPacket();
	}
}


void CamuleApp::OnTCPTimer(wxTimerEvent& WXUNUSED(evt))
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


void CamuleApp::OnCoreTimer(wxTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section

	static uint32	msPrev1, msPrev5, msPrevSave;

	uint32 			msCur = GetUptimeMsecs();

	// can this actually happen under wxwin ?
	if (!IsRunning() || !IsReady) {
		return;
	}

	uploadqueue->Process();
	downloadqueue->Process();
	//theApp.clientcredits->Process();
	uploadqueue->CompUpDatarateOverhead();
	downloadqueue->CompDownDatarateOverhead();

	if (msCur-msPrev1 > 950) {  // approximately every second
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
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		listensocket->Process();
		OnlineSig(); // Added By Bouc7
		// Kry - Log lines flush
		FlushQueuedLogLines();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		wxString buffer;
		
		wxConfigBase* cfg = wxConfig::Get();
		buffer.Printf(wxT("%llu"),stat_sessionReceivedBytes+glob_prefs->GetTotalDownloaded());
		cfg->Write(wxT("/Statistics/TotalDownloadedBytes"), buffer);

		buffer.Printf(wxT("%llu"),stat_sessionSentBytes+glob_prefs->GetTotalUploaded());
		cfg->Write(wxT("/Statistics/TotalUploadedBytes"), buffer);
	}

	// Recomended by lugdunummaster himself - from emule 0.30c
	serverconnect->KeepConnectionAlive();

}

void CamuleApp::OnHashingShutdown(wxCommandEvent& WXUNUSED(evt))
{
	if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
		printf("Hashing thread ended\n");
		// Save the known.met file
		knownfiles->Save();
	} else {
		printf("Hashing thread terminated, ready to shutdown\n");
	}
}


void CamuleApp::OnFinishedHashing(wxCommandEvent& evt)
{
	static int filecount = 0;
	static int bytecount = 0;

	CKnownFile* result = (CKnownFile*)evt.GetClientData();
	printf("Finished Hashing %s\n",unicode2char(result->GetFileName()));
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
			printf("Not added\n");
			delete result;
		}
	}

	return;
}

void CamuleApp::OnFinishedCompletion(wxCommandEvent& evt)
{
	CPartFile* completed = (CPartFile*)evt.GetClientData();

	wxASSERT(completed);

	completed->CompleteFileEnded(evt.GetInt(), (wxString*)evt.GetExtraLong());

	return;
}


void CamuleApp::ShutDown() {
	// Signal the hashing thread to terminate
	m_app_state = APP_STATE_SHUTINGDOWN;
	IsReady =  false;
	amuledlg->Destroy();
	if (CAddFileThread::IsRunning()) {
		CAddFileThread::Shutdown();
	}
}


#if defined(__DEBUG__) 
	#if defined(__LINUX__)
	
		void CamuleApp::AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time) {
	
			socket_deletion_log_item current_socket;
			socket_deletion_log_item temp_socket;
	
			current_socket.socket_n = socket_pointer;
			current_socket.creation_time = creation_time;
			current_socket.backtrace = wxT("");
	
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
	

void CamuleApp::NotifyEvent(GUIEvent event) {

	if (!amuledlg && (event.ID!=ADDLOGLINE)) {
		return;
	}

   
	switch (event.ID) {
		// GUI->CORE events
		// no need to check pointers: if event is here, gui must be running
		
		// search
	        case SEARCH_LOCAL_REQ:
			uploadqueue->AddUpDataOverheadServer(((Packet *)event.ptr_value)->GetPacketSize());
			serverconnect->SendPacket( (Packet *)event.ptr_value, 0 );
			break;
	        case SEARCH_GLOBAL_REQ:
			serverconnect->SendUDPPacket((Packet *)event.ptr_value, (CServer *)event.ptr_aux_value, false);
			break;
	        case SEARCH_ADD_TO_DLOAD:
			downloadqueue->AddSearchToDownload((CSearchFile *)event.ptr_value, event.byte_value);
			break;
	        case SEARCH_ADD_RESULT:
			break;
	        case SEARCH_UPDATE_SOURCES:
			
			break;

		// PartFile
	        case PARTFILE_REMOVE_NO_NEEDED:
			((CPartFile *)event.ptr_value)->RemoveNoNeededSources();
			break;
	        case PARTFILE_REMOVE_FULL_QUEUE:
			((CPartFile *)event.ptr_value)->RemoveFullQueueSources();
			break;
	        case PARTFILE_REMOVE_HIGH_QUEUE:
			((CPartFile *)event.ptr_value)->RemoveHighQueueRatingSources();
			break;
	        case PARTFILE_CLEANUP_SOURCES:
			((CPartFile *)event.ptr_value)->CleanUpSources();
			break;
	        case PARTFILE_SWAP_A4AF_THIS: {
			CPartFile *file = (CPartFile *)event.ptr_value;
			if ((file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)) {
				downloadqueue->DisableAllA4AFAuto();

				CPartFile::SourceSet::iterator it;
				for ( it = file->A4AFsrclist.begin(); it != file->A4AFsrclist.end(); ) {
					CUpDownClient *cur_source = *it++;
					if ((cur_source->GetDownloadState() != DS_DOWNLOADING) && cur_source->GetRequestFile() &&
					    ( (!cur_source->GetRequestFile()->IsA4AFAuto()) ||
					      (cur_source->GetDownloadState() == DS_NONEEDEDPARTS))) {
						cur_source->SwapToAnotherFile(true, false, false, file);
					}
				}
		        }
		}
			break;
        	case PARTFILE_SWAP_A4AF_OTHERS: {
			CPartFile *file = (CPartFile *)event.ptr_value;
			if ((file->GetStatus(false) == PS_READY) || (file->GetStatus(false) == PS_EMPTY)) {
				downloadqueue->DisableAllA4AFAuto();

				CPartFile::SourceSet::iterator it = file->m_SrcList.begin();
				for( ; it != file->m_SrcList.end(); ++it ) {
					(*it)->SwapToAnotherFile(false, false, false, NULL);
				}
			}
			break;
		}
	        case PARTFILE_SWAP_A4AF_THIS_AUTO:
			((CPartFile *)event.ptr_value)->SetA4AFAuto(!((CPartFile *)event.ptr_value)->IsA4AFAuto());
			break;
	        case PARTFILE_PAUSE:
			((CPartFile *)event.ptr_value)->PauseFile(event.byte_value);
			break;
	        case PARTFILE_RESUME:
			((CPartFile *)event.ptr_value)->ResumeFile();
			((CPartFile *)event.ptr_value)->SavePartFile();
			break;
	        case PARTFILE_STOP:
			((CPartFile *)event.ptr_value)->StopFile(event.byte_value);
			break;
	        case PARTFILE_PRIO_AUTO:
			((CPartFile *)event.ptr_value)->SetAutoDownPriority(event.long_value);
			break;
	        case PARTFILE_PRIO_SET:
			((CPartFile *)event.ptr_value)->SetDownPriority(event.long_value,
									event.longlong_value, event.longlong_value);
			break;
	        case PARTFILE_SET_CAT:
			((CPartFile *)event.ptr_value)->SetCategory(event.byte_value);
			break;
	        case PARTFILE_DELETE:
			if ( theApp.glob_prefs->StartNextFile() &&
			     (((CPartFile *)event.ptr_value)->GetStatus() == PS_PAUSED) ) {
				downloadqueue->StartNextFile();
			}
			((CPartFile *)event.ptr_value)->Delete();
			break;
	        case KNOWNFILE_SET_UP_PRIO:
			((CKnownFile *)event.ptr_value)->SetAutoUpPriority(false);
			((CKnownFile *)event.ptr_value)->SetUpPriority(event.byte_value);
			break;
	        case KNOWNFILE_SET_UP_PRIO_AUTO:
			((CKnownFile *)event.ptr_value)->SetAutoUpPriority(true);
			((CKnownFile *)event.ptr_value)->UpdateAutoUpPriority();
			break;
	        case KNOWNFILE_SET_PERM:
			((CKnownFile *)event.ptr_value)->SetPermissions(event.byte_value);
			break;
	        case KNOWNFILE_SET_COMMENT:
			((CKnownFile *)event.ptr_value)->SetFileComment(event.string_value);
			break;
			// download queue
	        case DLOAD_SET_CAT_PRIO:
			downloadqueue->SetCatPrio(event.long_value, event.short_value);
			break;
	        case DLOAD_SET_CAT_STATUS:
			downloadqueue->SetCatStatus(event.long_value, event.short_value);
			break;
		// CORE->GUI
		// queue list
		case QLIST_CTRL_ADD_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->queuelistctrl ) {
				amuledlg->transferwnd->queuelistctrl->AddClient((CUpDownClient*)event.ptr_value);
			}
			break;
		case QLIST_CTRL_RM_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->queuelistctrl ) {
				amuledlg->transferwnd->queuelistctrl->RemoveClient((CUpDownClient*)event.ptr_value);
			}
			break;
		case QLIST_CTRL_REFRESH_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->queuelistctrl ) {
				amuledlg->transferwnd->queuelistctrl->RefreshClient((CUpDownClient*)event.ptr_value);
			}
			break;
		// shared files
		case SHAREDFILES_UPDATEITEM:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->UpdateItem((CKnownFile*)event.ptr_value);
			}
			break;
		// download ctrl
		case DOWNLOAD_CTRL_UPDATEITEM:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->UpdateItem((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_ADD_FILE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->AddFile((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_ADD_SOURCE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->AddSource((CPartFile*)event.ptr_value,
										   (CUpDownClient*)event.ptr_aux_value,
										   event.byte_value);
			}
			break;
		case DOWNLOAD_CTRL_RM_FILE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->RemoveFile((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_RM_SOURCE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->RemoveSource((CUpDownClient*)event.ptr_value,
										      (CPartFile*)event.ptr_aux_value);
			}
			break;

		case DOWNLOAD_CTRL_SHOW_HIDE_FILE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				// for remote gui just send message
				amuledlg->transferwnd->downloadlistctrl->Freeze();
				if (!CheckShowItemInGivenCat((CPartFile*)event.ptr_value,
							     amuledlg->transferwnd->downloadlistctrl->curTab)) {
					amuledlg->transferwnd->downloadlistctrl->HideFile((CPartFile*)event.ptr_value);
				} else {
					amuledlg->transferwnd->downloadlistctrl->ShowFile((CPartFile*)event.ptr_value);
				}
				amuledlg->transferwnd->downloadlistctrl->Thaw();
			}
			break;

		case DOWNLOAD_CTRL_HIDE_SOURCE:
		if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
					amuledlg->transferwnd->downloadlistctrl->HideSources((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_INIT_SORT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->InitSort();
			}
			break;
		case DOWNLOAD_CTRL_SHOW_FILES_COUNT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->ShowFilesCount();
			}
			break;
		// upload ctrl
			case UPLOAD_CTRL_ADD_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->uploadlistctrl ) {
				amuledlg->transferwnd->uploadlistctrl->AddClient((CUpDownClient*)event.ptr_value);
			}
			break;
		case UPLOAD_CTRL_REFRESH_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->uploadlistctrl ) {
				amuledlg->transferwnd->uploadlistctrl->RefreshClient((CUpDownClient*)event.ptr_value);
			}
			break;
		case UPLOAD_CTRL_RM_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->uploadlistctrl ) {
				amuledlg->transferwnd->uploadlistctrl->RemoveClient((CUpDownClient*)event.ptr_value);
			}
			break;
		// server
		case SERVER_ADD:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->AddServer((CServer*)event.ptr_value, true);
			}
			break;
		case SERVER_RM:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->RemoveServer((CServer*)event.ptr_value);
			}
			break;
		case SERVER_RM_DEAD:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->RemoveDeadServer();
			}
			break;
		case SERVER_RM_ALL:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->DeleteAllItems();
			}
			break;
		case SERVER_HIGHLIGHT:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->HighlightServer((CServer*)event.ptr_value,
										     event.byte_value);
			}
			break;
		case SERVER_REFRESH:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->RefreshServer((CServer*)event.ptr_value);
			}
			break;
		case SERVER_FREEZE:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->Freeze();
			}
			break;
		case SERVER_THAW:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->Thaw();
			}
			break;

		// notification
		case SHOW_NOTIFIER:
			amuledlg->ShowNotifier(event.string_value,event.long_value,event.byte_value);
			break;
		case SHOW_CONN_STATE:
			amuledlg->ShowConnectionState(event.byte_value, event.string_value,
						      event.long_value);
			break;
		case SHOW_QUEUE_COUNT:
			if ( amuledlg->transferwnd ) {
				amuledlg->transferwnd->ShowQueueCount(event.long_value);
			}
			break;
		case SHOW_UPDATE_CAT_TABS:
			if ( amuledlg->transferwnd ) {
				amuledlg->transferwnd->UpdateCatTabTitles();
			}
			break;
			
		case SHOW_GUI:
			amuledlg->Show_aMule(true);
			break;
			
		// search window
		case SEARCH_CANCEL:
			if ( amuledlg->searchwnd ) {
				amuledlg->searchwnd->OnBnClickedCancels(*(wxCommandEvent *)event.ptr_value);
			}
			break;

		// chat window
		case CHAT_REFRESH_FRIEND:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->RefreshFriend((CFriend *)event.ptr_value);
			}
			break;
		case CHAT_FIND_FRIEND:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->FindFriend(*(CMD4Hash *)event.ptr_value, event.long_value, event.short_value);
			}
			break;
		case CHAT_CONN_RESULT:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->ConnectionResult((CUpDownClient *)event.ptr_value, event.byte_value);
			}
			break;

		// logging
		case ADDLOGLINE:
			if (amuledlg) {
				amuledlg->AddLogLine(event.byte_value,event.string_value);
			} else {
				QueueLogLine(event.byte_value,event.string_value);
			}
			break;
		case ADDDEBUGLOGLINE:
			if (amuledlg) {
				amuledlg->AddDebugLogLine(event.byte_value,event.string_value);
			} else {
				wxASSERT(0);
				//QueueLogLine(event.byte_value,event.string_value);
			}
			break;
		default:
			printf("Unknown event notified to wxApp\n");
			wxASSERT(0);
	}

};


bool CamuleApp::AddServer(CServer *srv)
{
	if ( serverlist->AddServer(srv) ) {
		if ( amuledlg && amuledlg->serverwnd ) {
			amuledlg->serverwnd->serverlistctrl->AddServer(srv, true);
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}

}

CFriend *CamuleApp::FindFriend(CMD4Hash *hash, uint32 ip, uint16 port)
{
	if ( amuledlg && amuledlg->chatwnd ) {
		return 	amuledlg->chatwnd->FindFriend(*hash, ip, port);
	}
	return NULL;
}

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
