//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://sourceforge.net/projects/amule )
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
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#if defined(__linux__)
	#include <execinfo.h>
	#include <mntent.h>
#endif
#ifdef __WXGTK__
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
#include <wx/msgdlg.h>		// Needed for wxMessageBox
#include <wx/config.h>
#include <wx/clipbrd.h>         // Needed for wxClipBoard
#include <wx/socket.h>          // Needed for wxSocket
#include <wx/splash.h>          // Needed for wxSplashScreen
#include <wx/utils.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/ipc.h>
#include <wx/intl.h>            // Needed for i18n
#include <wx/imaglist.h>        // Needed for wxImageList
#include <wx/mimetype.h>	// For launching default browser
#include <wx/textfile.h>        // Needed for wxTextFile

#include "amule.h"		// Interface declarations.
#include "GetTickCount.h"	// Needed for GetTickCount
#include "color.h"		// Interface declaration of GetColour()
#include "server.h"		// Needed for GetListName
#include "CFile.h"		// Needed for CFile
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "otherfunctions.h"	// Needed for GetTickCount
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "IPFilter.h"		// Needed for CIPFilter
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "sockets.h"		// Needed for CServerConnect
#include "ServerList.h"		// Needed for CServerList
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "FriendList.h"		// Needed for CFriendList
#include "SearchList.h"		// Needed for CSearchList
#include "ClientList.h"		// Needed for CClientList
#include "muuli_wdr.h"		// Needed for amuleDlgImages
#include "PreferencesDlg.h"	// Needed for CPreferencesDlg
#include "Preferences.h"	// Needed for CPreferences
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "ListenSocket.h"	// Needed for CListenSocket
#include "ExternalConn.h"	// Needed for ExternalConn & MuleConnection
#include "PartFile.h"           // Needed for FakeCheck
#include "KnownFile.h"          // Needed for FakeCheck

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

class MyTimer *mytimer;


// CamuleApp
IMPLEMENT_APP(CamuleApp)

#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

#ifdef __USE_SPLASH__
#include "splash.xpm"
#endif

MuleClient *client;

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
	UnlimitResource(RLIMIT_RSS);
#endif
}

CamuleApp::CamuleApp()
{
}

CamuleApp::~CamuleApp()
{
	delete listensocket;
}

int CamuleApp::OnExit()
{
	
	printf("Now, exiting main app...\n");
	/*
	 * The following destruction sequence was moved to the
	 * destructor of the dialog since some of them call
	 * functions accessing controls of the dialog.
	 */
#if 0
	/* Clear up the Online Signature file. */
	OnlineSig(true);

	theApp.listensocket->Destroy();
	theApp.clientudp->Destroy();
	delete theApp.sharedfiles;
	delete theApp.serverconnect;
	delete theApp.serverlist;
	delete theApp.knownfiles;
	delete theApp.searchlist;
	delete theApp.clientcredits;
	delete theApp.downloadqueue;
	delete theApp.uploadqueue;
	delete theApp.clientlist;
	delete theApp.friendlist;
	delete theApp.glob_prefs;
#endif

#if 0
	delete searchlist;
	delete friendlist;
	delete knownfiles;
	delete serverlist;
	delete serverconnect;
	delete sharedfiles;
	delete listensocket;
	delete clientudp;
	delete clientcredits;
	delete downloadqueue;
	delete uploadqueue;
	delete clientlist;
#endif

	if (ipfilter) {
		delete ipfilter;
	}

	//shakraw
	//ShutDownECServer();
	delete ECServerHandler;
	printf("aMule shutdown completed.\n");
	return wxApp::OnExit();
}

// CamuleApp Initialisierung

extern void InitXmlResource();

bool CamuleApp::OnInit()
{
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	mytimer = new MyTimer();

	IsReady = false;
	clientlist =	NULL;
	searchlist =	NULL;
	friendlist =	NULL;
	knownfiles =	NULL;
	serverlist =	NULL;
	serverconnect =	NULL;
	sharedfiles =	NULL;
	listensocket =	NULL;
	clientudp =	NULL;
	clientcredits =	NULL;
	downloadqueue =	NULL;
	uploadqueue =	NULL;
	ipfilter =	NULL;
	conn =		NULL;
	ipcserver =	NULL;

	// catch fatal exceptions
	wxHandleFatalExceptions(true);

	SetResourceLimits();

	// for resources
	wxFileSystem::AddHandler(new wxZipFSHandler);

	/* No more XML SHIT !!! (Creteil) */
	//wxXmlResource::Get()->InitAllHandlers();
	//InitXmlResource();

	// eagle: geometry cmd-line handling
	char *geom_string;
	geometry_is_set = 0;
	geometry_x	= 0;
	geometry_y	= 0;
	geometry_width	= 800;
	geometry_height	= 600;

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--version", 9) == 0) {
			printf("aMule %s\n", VERSION);
			#ifdef __WXMSW__ 
				_sleep(1);
			#else
				sleep(1);
			#endif
			exit(0);
		}

		if (strncmp(argv[i], "--geometry", 10) == 0) {
			geometry_is_set = 1;
			// would "--geometry --help" be acceptable? - if not s/i+1/++i
			geom_string = argv[i+1];
#ifdef __WXGTK__
			XParseGeometry(geom_string, &geometry_x, &geometry_y, &geometry_width, &geometry_height);
#else
			#warning Need to parse the geometry
#endif			
			printf("geometry:  x: %d y: %d width: %d height: %d\n", geometry_x, geometry_y, geometry_width, geometry_height);
			// there is no point continuing testing this case - Unleashed
			continue;
		}

		if (strncmp(argv[i], "--help", 6) == 0) {
			printf("This is aMule %s \n", VERSION);
			printf("\nUsage: amule [option]\n");
			printf("\n\t--help:\t\t\t\t shows this help\n");
			printf("\t--version:\t\t\t shows program version number\n");
			printf("\t--geometry geometry_string:\t sets geometry of the app\n");
			printf("\t\t\t\t\t geometry string has format [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]\n");
			printf("\t\t\t\t\t as for stardard X app definition\n");
			printf("\nReport bugs to amule.sf.net\n");
			#ifdef __WXMSW__
				_sleep(1);
			#else
				sleep(1);
			#endif
			exit(0);
		} 
	}

	printf("Initialising aMule\n");

	//if(ProcessCommandLine())
	//  return FALSE;

	//SetTopWindow(dlg);

	SetVendorName("TikuWarez");

	// Do NOT change this string to aMule nor anything else, it WILL fuck you up.
	SetAppName("eMule");

	/* Madcat - This code part is no longer neccesery, because external ED2K links are
		handled by the 'ed2k' program, and for statistics, it is easier (and faster)
		to use the online signature. Besides, disabling this code part will lower
		memory usage and speed up startup times.

		Kry - Yeah, kitty, but. I'm using my old code again to do some interesting stuff ;)
	*/

	// see if there is another instance running
	server = getenv("HOME") + wxString("/.aMule/muleconn");
	wxString hostName = "localhost";
	client = new MuleClient;
	conn = (MuleConnection *) client->MakeConnection(hostName, server, wxString::wxString("aMule IPC TESTRUN"));
	if (!conn) {
		// no connection => spawn server instead.
		localserver = new MuleServer;
		localserver->Create(server);
	} else {
		conn->Disconnect();
		delete conn;
		delete client;
		// don't allow another instance.
		printf("aMule already running: exiting\n");
		#ifdef __WXMSW__
		_sleep(1);
		#else
		sleep(1);	// this will prevent amule to hang itself
		#endif
		exit(0);	// is this clean.. perhaps not
	}

	close(0);

	// Madcat - Check if prefs can be found at ~/.lmule and rename as neccesery.
	// Kry - copy settings from xMule instead of moving dir... seems to be much less aggressive

 	int new_from_lmule = 0;
	wxString lMulePrefDir = getenv("HOME") + wxString("/.lmule");
	wxString xMulePrefDir = getenv("HOME") + wxString("/.xMule");
	wxString aMulePrefDir = getenv("HOME") + wxString("/.aMule");
	if (wxDirExists(lMulePrefDir) && !wxDirExists(aMulePrefDir)) {
		printf("Found lMule old settings, moving to new dir.\n");
		wxRenameFile(lMulePrefDir, aMulePrefDir);
		new_from_lmule = 1;
	} else new_from_lmule = 0;
	if (wxDirExists(xMulePrefDir) && (!wxDirExists(aMulePrefDir) || new_from_lmule==1)) {
		printf("Found xMule old settings, copying config & credits files.\n");
		wxMkdir(aMulePrefDir);
		if  (wxFileExists(xMulePrefDir+"/clients.met")) {
			wxCopyFile(xMulePrefDir+"/clients.met",aMulePrefDir+"/clients.met");
		}
		if  (wxFileExists(xMulePrefDir+"/emfriends.met")) {
			wxCopyFile(xMulePrefDir+"/emfriends.met",aMulePrefDir+"/emfriends.met");
		}
		if  (wxFileExists(xMulePrefDir+"/preferences.dat")) {
			wxCopyFile(xMulePrefDir+"/preferences.dat",aMulePrefDir+"/preferences.dat");
		}
		if  (wxFileExists(xMulePrefDir+"/staticservers.dat")) {
			wxCopyFile(xMulePrefDir+"/staticservers.dat",aMulePrefDir+"/staticservers.dat");
		}
		if  (wxFileExists(xMulePrefDir+"/clients.met.BAK")) {
			wxCopyFile(xMulePrefDir+"/clients.met.BAK",aMulePrefDir+"/clients.met.BAK");
		}
		if  (wxFileExists(xMulePrefDir+"/known.met")) {
			wxCopyFile(xMulePrefDir+"/known.met",aMulePrefDir+"/known.met");
		}
		if  (wxFileExists(xMulePrefDir+"/server.met")) {
			wxCopyFile(xMulePrefDir+"/server.met",aMulePrefDir+"/server.met");
		}
		if  (wxFileExists(xMulePrefDir+"/shareddir.dat")) {
			wxCopyFile(xMulePrefDir+"/shareddir.dat",aMulePrefDir+"/shareddir.dat");
		}
		if  (wxFileExists(xMulePrefDir+"/ipfilter.dat")) {
			wxCopyFile(xMulePrefDir+"/ipfilter.dat",aMulePrefDir+"/ipfilter.dat");
		}
		// wxRenameFile(xMulePrefDir, aMulePrefDir); No more
		 wxMessageBox(wxT("Copied old ~/.xMule config and credit files to ~/.aMule\nHowever, be sure NOT to remove .xMule if your Incoming / Temp folders are still there ;)"), wxT("Info"), wxOK);
	}
	// Delete old log file.
	wxRemoveFile(wxString::Format("%s/.aMule/logfile", getenv("HOME")));
	
	// Load Preferences
	glob_prefs = new CPreferences();

	
	wxTextFile version_file(theApp.glob_prefs->GetAppDir() + wxString("lastversion"));
	wxString old_version;
	wxString new_version(VERSION);
	if (version_file.Exists()) {
		version_file.Open();
		if (!version_file.Eof()) {
			old_version = version_file.GetFirstLine();
			if (old_version != new_version) {
				Trigger_New_version(old_version, new_version);		
				for (int lines_count = 0; lines_count < version_file.GetLineCount(); lines_count++) {
					version_file.RemoveLine(lines_count);
				}
				version_file.AddLine(VERSION);
				version_file.Write();
			}					
			version_file.Close();		
		} else {
		old_version = "pre_1.2.7";
		Trigger_New_version(old_version, new_version);
		version_file.Create();
		version_file.AddLine(VERSION);
		version_file.Write();
		version_file.Close();							
		}
	} else {
		old_version = "pre_1.2.7";
		Trigger_New_version(old_version, new_version);
		version_file.Create();
		version_file.AddLine(VERSION);
		version_file.Write();
		version_file.Close();				
	}	
	
	Localize_mule();
	
	CamuleDlg *dlg = new CamuleDlg(NULL, wxString::Format(wxT("aMule %s"), wxT(VERSION)));
	amuledlg = dlg;
	dlg->Show(TRUE);
#ifndef DISABLE_OLDPREFS
	amuledlg->preferenceswnd->SetPrefs(glob_prefs);
#endif
	theApp.use_chmod = 1;
	
	#ifdef __LINUX__
	wxString * incomingdir = new wxString(theApp.glob_prefs->GetIncomingDir());
	wxString * tempdir = new wxString(theApp.glob_prefs->GetTempDir());
	struct mntent* mnt_entries;
	FILE* mnt_tab;
	mnt_tab = setmntent("/etc/mtab","r");
	if (mnt_tab) {
		while ((mnt_entries = getmntent(mnt_tab))!=NULL) {
			if (strncmp(mnt_entries->mnt_type,"vfat",4)==0) {
				if (strncmp(tempdir->c_str(),mnt_entries->mnt_dir,strlen(mnt_entries->mnt_dir))==0) {
					printf("Detected temp dir %s on fat32 mount device %s, disabling chmod for that files\n",tempdir->c_str(),mnt_entries->mnt_dir);
					theApp.use_chmod = 0;
				}	
				if (strncmp(incomingdir->c_str(),mnt_entries->mnt_dir,strlen(mnt_entries->mnt_dir))==0) {
					printf("Detected Incoming %s dir on fat32 mount device %s, disabling chmod for that files\n",incomingdir->c_str(),mnt_entries->mnt_dir);
					theApp.use_chmod = 0;
				}
			}
		}
	fclose(mnt_tab);
	}
	if (incomingdir) {
		delete incomingdir;
	}
	if (tempdir) {
		delete tempdir;
	}
	
	#endif
	
	//shakraw - new EC code using wxSocket*
	//CreateECServer();
	//Activate External Connections server and see if there is another 
	//instance running
	ECServerHandler = new ExternalConn();
	
#ifndef __SYSTRAY_DISABLED__
	amuledlg->CreateSystray(wxString::Format(wxT("%s %s"), wxT(PACKAGE), wxT(VERSION)));
#endif // __SYSTRAY_DISABLED__
        
	// splashscreen
	#ifdef __USE_SPLASH__
	if (theApp.glob_prefs->UseSplashScreen() && !theApp.glob_prefs->GetStartMinimized()) {
		new wxSplashScreen(
			wxBitmap(splash_xpm),
			wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
			5000, NULL, -1, wxDefaultPosition, wxDefaultSize,
			wxSIMPLE_BORDER|wxSTAY_ON_TOP
		);
	}
	#endif

	wxIPV4address myaddr;
	myaddr.AnyAddress();
	myaddr.Service(glob_prefs->GetPort());
	printf("*** TCP socket at %d\n", glob_prefs->GetPort());

	clientlist = new CClientList();
	searchlist = new CSearchList();
	friendlist = new CFriendList();
	knownfiles = new CKnownFileList(glob_prefs->GetAppDir());
	serverlist = new CServerList(glob_prefs);
	serverconnect = new CServerConnect(serverlist, theApp.glob_prefs);
	sharedfiles = new CSharedFileList(glob_prefs, serverconnect, knownfiles);
	myaddr.Service(glob_prefs->GetUDPPort());
	clientudp = new CClientUDPSocket(myaddr);
	clientcredits = new CClientCreditsList(glob_prefs);
	downloadqueue = new CDownloadQueue(glob_prefs, sharedfiles);	// bugfix - do this before creating the uploadqueue
	uploadqueue = new CUploadQueue(glob_prefs);
	ipfilter = new CIPFilter();
	
	//webserver = new CWebServer(); //shakraw, we use amuleweb now
	//INT_PTR nResponse = dlg.DoModal();

	// init statistics stuff, better do it asap
	amuledlg->statisticswnd->Init();
	amuledlg->statisticswnd->SetUpdatePeriod();

	// must do initialisations here.. 
	amuledlg->serverwnd->serverlistctrl->Init(serverlist);
	serverlist->Init();

	// ini. downloadqueue
	theApp.downloadqueue->Init();
	amuledlg->AddLogLine(true, PACKAGE_STRING);

	// 
	theApp.sharedfiles->SetOutputCtrl((CSharedFilesCtrl *) amuledlg->sharedfileswnd->FindWindowByName("sharedFilesCt"));

	// then init firend list
	theApp.friendlist->SetWindow((CFriendListCtrl *) theApp.amuledlg->transferwnd->FindWindowById(ID_FRIENDLIST));
	theApp.friendlist->ShowFriends();

	SetTopWindow(dlg);

	// reset statistic values
	theApp.stat_sessionReceivedBytes = 0;
	theApp.stat_sessionSentBytes = 0;
	theApp.stat_reconnects = 0;
	theApp.stat_transferStarttime = 0;
	theApp.stat_serverConnectTime = 0;
	theApp.Start_time = GetTickCount64();
	theApp.sTransferDelay = 0.0;

	// Initialize and sort all lists.
	// FIX: Remove from here and put these back to the OnInitDialog()s
	// and call the OnInitDialog()s here!
	theApp.amuledlg->transferwnd->downloadlistctrl->InitSort();
	theApp.amuledlg->transferwnd->uploadlistctrl->InitSort();
	theApp.amuledlg->transferwnd->queuelistctrl->InitSort();
	theApp.amuledlg->serverwnd->serverlistctrl->InitSort();
	theApp.amuledlg->sharedfileswnd->sharedfilesctrl->InitSort();

	// call the initializers
	theApp.amuledlg->transferwnd->OnInitDialog();

	amuledlg->m_app_state = APP_STATE_RUNNING;
	
	// reload shared files
	theApp.sharedfiles->Reload(true, true);

	//shakraw, we use amuleweb now
	//if (glob_prefs->GetWSIsEnabled()) {
	//	webserver->ReloadTemplates();
	//	webserver->StartServer();
	//}

	myaddr.Service(glob_prefs->GetPort());
	listensocket = new CListenSocket(glob_prefs, myaddr);

	theApp.listensocket->StartListening();
	
	if (!listensocket->Ok()) {
		amuledlg->AddLogLine(false, CString(_("Port %d is not available. You will be LOWID")), glob_prefs->GetPort());
		CString str;
		str.Format(_("Port %d is not available !!\n\nThis will mean that you will be LOWID.\n\nUse netstat to determine when port becomes available\nand try starting amule again."), glob_prefs->GetPort());
		wxMessageBox(str, _("Error"), wxCENTRE | wxOK | wxICON_ERROR);
	}

	// Must we start minimized?
	if (theApp.glob_prefs->GetStartMinimized()) {
		// Send it to tray?
		if (theApp.glob_prefs->DoMinToTray()) {
			theApp.amuledlg->Hide_aMule();
		} else {
			theApp.amuledlg->Iconize(TRUE);
		}

	}
	
	
	// pretty much ready now. start autoconnect if 
	if (theApp.glob_prefs->DoAutoConnect()) {
		wxCommandEvent nullEvt;
		theApp.amuledlg->OnBnConnect(nullEvt);
	}

	if (theApp.glob_prefs->GetMaxGraphDownloadRate() < theApp.glob_prefs->GetMaxDownload())
		theApp.glob_prefs->SetDownloadlimit(UNLIMITED);
	if (theApp.glob_prefs->GetMaxGraphUploadRate() < theApp.glob_prefs->GetMaxUpload())
		theApp.glob_prefs->SetUploadlimit(UNLIMITED);

	if ( theApp.glob_prefs->GetMaxDownload()==0 && theApp.glob_prefs->GetMaxUpload() < 10)
		theApp.glob_prefs->SetDownloadlimit((theApp.glob_prefs->GetMaxUpload()*4)) ;
        if( theApp.glob_prefs->GetMaxUpload() != 0 && theApp.glob_prefs->GetMaxUpload() !=UNLIMITED){

	if( theApp.glob_prefs->GetMaxUpload() < 4 &&
		( theApp.glob_prefs->GetMaxUpload()*3 < theApp.glob_prefs->GetMaxDownload() ) )
		theApp.glob_prefs->SetDownloadlimit((theApp.glob_prefs->GetMaxUpload()*3));

	if( theApp.glob_prefs->GetMaxUpload() < 10 &&
		( theApp.glob_prefs->GetMaxUpload()*4 < theApp.glob_prefs->GetMaxDownload() ) )
	theApp.glob_prefs->SetDownloadlimit((theApp.glob_prefs->GetMaxUpload()*4)) ;
	}
		
	IsReady = true;
#if 0
	// set the checkpoint
	wxDebugContext::SetCheckpoint();
#endif

	// Kry - Load the sources seeds on app init
	if (theApp.glob_prefs->GetSrcSeedsOn()) {
		theApp.downloadqueue->LoadSourceSeeds();
	}	
	
	return TRUE;
}

#if 0
bool CamuleApp::InitInstance()
{
#ifdef _DUMP
	MiniDumper dumper(CURRENT_VERSION_LONG);
#endif

	pendinglink = 0;
	if (ProcessCommandline()) {
		return false;
	}
	// InitCommonControls() ist für Windows XP erforderlich, wenn ein Anwendungsmanifest
	// die Verwendung von ComCtl32.dll Version 6 oder höher zum Aktivieren
	// von visuellen Stilen angibt. Ansonsten treten beim Erstellen von Fenstern Fehler auf.
	InitCommonControls();

	CWinApp::InitInstance();

	if (!AfxSocketInit()) {
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
	AfxSocketInit();
	CamuleDlg dlg;
	amuledlg = &dlg;
	m_pMainWnd = &dlg;

	// create & initalize all the important stuff 
	glob_prefs = new CPreferences();

	//setup languag 
	clientlist = new CClientList();
	searchlist = new CSearchList();
	knownfiles = new CKnownFileList(glob_prefs->GetAppDir());
	serverlist = new CServerList(glob_prefs);
	serverconnect = new CServerConnect(serverlist, theApp.glob_prefs);
	sharedfiles = new CSharedFileList(glob_prefs, serverconnect, knownfiles);
	listensocket = new CListenSocket(glob_prefs);
	clientcredits = new CClientCreditsList(glob_prefs);
	downloadqueue = new CDownloadQueue(glob_prefs, sharedfiles);	// bugfix - do this before creating the uploadqueue
	uploadqueue = new CUploadQueue(glob_prefs);
	INT_PTR nResponse = dlg.DoModal();


	// reset statistic values
	theApp.stat_sessionReceivedBytes = 0;
	theApp.stat_sessionSentBytes = 0;
	theApp.stat_reconnects = 0;
	theApp.stat_transferStarttime = 0;
	theApp.stat_serverConnectTime = 0;

	return FALSE;
}
#endif
/* UNUSED
bool CamuleApp::ProcessCommandline()
{
#if 0
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	char buffer[50];
	sprintf(buffer, "aMule %s", CURRENT_VERSION_LONG);
	HWND maininst = FindWindow(0, buffer);
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
		CString command = cmdInfo.m_strFileName;

		//if (maininst){ moved down by Cax2 28/10/02 
		//tagCOPYDATASTRUCT sendstruct; removed by Cax2 28/10/02 
		sendstruct.cbData = command.GetLength() + 1;
		sendstruct.dwData = OP_ED2KLINK;
		sendstruct.lpData = command.GetBuffer();
		if (maininst) {	//Cax2 28/10/02
			SendMessage(maininst, WM_COPYDATA, (WPARAM) 0, (LPARAM) (PCOPYDATASTRUCT) & sendstruct);
			return true;
		} else {
			pendinglink = new CString(command);
		}
	}

	return maininst;
#endif
}
*/

void CamuleApp::UpdateReceivedBytes(int32 bytesToAdd)
{
	SetTimeOnTransfer();
	stat_sessionReceivedBytes += bytesToAdd;
}

void CamuleApp::UpdateSentBytes(int32 bytesToAdd)
{
	SetTimeOnTransfer();
	stat_sessionSentBytes += bytesToAdd;
}

void CamuleApp::SetTimeOnTransfer()
{
	if (stat_transferStarttime > 0)
		return;

	stat_transferStarttime = GetTickCount64();
	sTransferDelay = (stat_transferStarttime - Start_time)/1000.0;
}

typedef char *LPTSTR;

wxString CamuleApp::StripInvalidFilenameChars(wxString strText, bool bKeepSpaces)
{
	LPTSTR pszBuffer = (char *)strText.GetData();
	LPTSTR pszSource = pszBuffer;
	LPTSTR pszDest = pszBuffer;

	while (*pszSource != '\0') {
		if (!((*pszSource <= 31 && *pszSource >= 0) ||	// lots of invalid chars for filenames in windows :=)
			  *pszSource == '\"' || *pszSource == '*' || *pszSource == '<' || *pszSource == '>' || *pszSource == '?' || *pszSource == '|' || *pszSource == '\\' || *pszSource == '/' || *pszSource == ':')) {
			if (!bKeepSpaces && *pszSource == ' ') {
				*pszDest = '_';
			}
			else {
				*pszDest = *pszSource;
			}
			pszDest++;
		}
		pszSource++;
	}
	*pszDest = '\0';

	return strText;
}

wxString CamuleApp::CreateED2kLink(CAbstractFile* f)
{
	wxString strLink;
	strLink = strLink.Format("ed2k://|file|%s|%u|%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x|/",
	StripInvalidFilenameChars(f->GetFileName(), true).GetData(),
	f->GetFileSize(), f->GetFileHash()[0], f->GetFileHash()[1],
	f->GetFileHash()[2], f->GetFileHash()[3], f->GetFileHash()[4],
	f->GetFileHash()[5], f->GetFileHash()[6], f->GetFileHash()[7],
	f->GetFileHash()[8], f->GetFileHash()[9], f->GetFileHash()[10],
	f->GetFileHash()[11], f->GetFileHash()[12], f->GetFileHash()[13],
	f->GetFileHash()[14], f->GetFileHash()[15]);
	return strLink.GetData();
}

wxString CamuleApp::CreateED2kSourceLink(CAbstractFile* f)
{
	if (!serverconnect->IsConnected() || serverconnect->IsLowID()) {
		amuledlg->AddLogLine(true, CString(_("You need a HighID to create a valid sourcelink")));
		return CString("");
	}
	uint32 dwID = serverconnect->GetClientID();
	wxString strLink;
	strLink.Printf("ed2k://|file|%s|%u|%s|/|sources,%i.%i.%i.%i:%i|/",
	StripInvalidFilenameChars(f->GetFileName(), false).GetData(),
	f->GetFileSize(), EncodeBase16(f->GetFileHash(), 16).GetData(),
	(uint8) dwID, (uint8) (dwID >> 8), (uint8) (dwID >> 16),
	(uint8) (dwID >> 24), glob_prefs->GetPort());
	return strLink;
}

wxString CamuleApp::CreateED2kHostnameSourceLink(CAbstractFile* f)
{
	wxString strLink;
	strLink.Printf("ed2k://|file|%s|%u|%s|/|sources,%s:%i|/",
	StripInvalidFilenameChars(f->GetFileName(), false).GetData(),
	f->GetFileSize(), EncodeBase16(f->GetFileHash(),16).GetData(),
	glob_prefs->GetYourHostname(), glob_prefs->GetPort());
	return strLink;
}

wxString CamuleApp::CreateHTMLED2kLink(CAbstractFile* f)
{
	wxString strCode = "<a href=\"" + CreateED2kLink(f) + "\">" + StripInvalidFilenameChars(f->GetFileName(), true) + "</a>";
	return strCode;
}

bool CamuleApp::CopyTextToClipboard(wxString strText)
{
	if (wxTheClipboard->Open()) {
		wxTheClipboard->UsePrimarySelection(TRUE);
		wxTheClipboard->SetData(new wxTextDataObject(strText));
		wxTheClipboard->Close();
	}
	return(true);
}


/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */) 
{
	// Do not do anything if online signature is disabled in Preferences
	if (!theApp.glob_prefs->IsOnlineSignatureEnabled()) {
		return;
	}

	// Build the filenames for the two files
	char* emulesig_path = new char[strlen(glob_prefs->GetAppDir())+14];
	char* amulesig_path = new char[strlen(glob_prefs->GetAppDir())+13];
	sprintf(emulesig_path,"%sonlinesig.dat",glob_prefs->GetAppDir());
	sprintf(amulesig_path,"%samulesig.dat",glob_prefs->GetAppDir());

	// Open both files for writing
	CFile amulesig_out, emulesig_out;
	if (!emulesig_out.Open(emulesig_path, CFile::write)) {
		theApp.amuledlg->AddLogLine(true, CString(_("Failed to save"))+CString(_(" OnlineSig File")));
	}
	if (!amulesig_out.Open(amulesig_path, CFile::write)) {
		theApp.amuledlg->AddLogLine(true,CString(_("Failed to save"))+wxString(" aMule OnlineSig File"));
	}

	char buffer[256];

	if (zero) {
		emulesig_out.Write("0\n0.0|0.0|0\n", 12);
		amulesig_out.Write("0\n0\n0\n0\n0\n0.0\n0.0\n0\n0\n", 22);
	} else {
		if (serverconnect->IsConnected()) {
			// We are online
			emulesig_out.Write("1",1);
			emulesig_out.Write("|",1);
			// Name of server (Do not use GetRealName()!)
			emulesig_out.Write(serverconnect->GetCurrentServer()->GetListName(),strlen(serverconnect->GetCurrentServer()->GetListName()));
			emulesig_out.Write("|",1);
			// IP and port of the server
			emulesig_out.Write(serverconnect->GetCurrentServer()->GetFullIP(),strlen(serverconnect->GetCurrentServer()->GetFullIP()));
			emulesig_out.Write("|",1);
			sprintf(buffer,"%d",serverconnect->GetCurrentServer()->GetPort());
			emulesig_out.Write(buffer,strlen(buffer));

			// Now for amule sig
			amulesig_out.Write("1",1);
			amulesig_out.Write("\n",1);
			amulesig_out.Write(serverconnect->GetCurrentServer()->GetListName(),strlen(serverconnect->GetCurrentServer()->GetListName()));
			amulesig_out.Write("\n",1);
			amulesig_out.Write(serverconnect->GetCurrentServer()->GetFullIP(),strlen(serverconnect->GetCurrentServer()->GetFullIP()));
			amulesig_out.Write("\n",1);
			amulesig_out.Write(buffer,strlen(buffer));
			amulesig_out.Write("\n",1);

			// Low- or High-ID (only in amule sig)
			if (theApp.serverconnect->IsLowID()) {
				amulesig_out.Write("L\n",2);
			} else {
				amulesig_out.Write("H\n",2);
			}
		} else {	// Not connected to a server
			emulesig_out.Write("0",1);
			amulesig_out.Write("0\n0\n0\n0\n0\n",10);
		}
		emulesig_out.Write("\n",1);

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
		sprintf(buffer,"%d", theApp.sharedfiles->GetCount());
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	}	/* if (!zero) */

	// Nick on the network
	sprintf(buffer, "%s", theApp.glob_prefs->GetUserNick());
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total received in GB
	sprintf(buffer, "%.2f", (float)(theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded()) / 1073741824);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total sent in GB
	sprintf(buffer, "%.2f", (float)(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded()) / 1073741824);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// amule version 
	sprintf(buffer,"%s",VERSION);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Close the files
	emulesig_out.Close();
	amulesig_out.Close();
	delete[] emulesig_path;
	delete[] amulesig_path;
} //End Added By Bouc7

void CamuleApp::OnFatalException()
{
	// Close sockets first.
	if ( theApp.listensocket )
		theApp.listensocket->Destroy();
	if ( theApp.clientudp )
		theApp.clientudp->Destroy();

	// (stkn) create backtrace
#if defined(__linux__)
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
	return(true);
}

//shakraw - old kry's code - not used
void CamuleApp::CreateECServer() {
	if (ipcserver) {
		ShutDownECServer();
	}

	if (theApp.glob_prefs->ECUseTCPPort()) {
		server = wxString::Format("%i",theApp.glob_prefs->ECPort());
	} else {
		server = getenv("HOME") + wxString("/.aMule/muleconn");
	}
	ipcserver = new MuleServer;
	ipcserver->Create(server);
}

void CamuleApp::ShutDownECServer() {
	if (conn) {
			printf("An amulecmd connection exists: Disconnecting\n");
			conn->Disconnect();
			delete conn;
		}
	if (client) {
		delete client;
	}
	if (ipcserver) {
		delete ipcserver;
	}
	if (localserver) {
		delete localserver;
	}
	
}

void CamuleApp::Localize_mule() {
	
	int language;
	
	switch (theApp.glob_prefs->GetLanguageID()) {
		case 0:		
			language = wxLANGUAGE_DEFAULT;
			break;
		case 1:
			//strcpy(newlang,"ar");
			language = wxLANGUAGE_ARABIC;
			break;			
		case 2:
			//strcpy(newlang,"eu_ES");
			language = wxLANGUAGE_BASQUE;
			break;
		case 3:
			//strcpy(newlang,"bg_BG");
			language = wxLANGUAGE_BULGARIAN;
			break;
		case 4:
			//strcpy(newlang,"ca_ES");
			language = wxLANGUAGE_CATALAN;
			break;
		case 5:
			//strcpy(newlang,"zh_CN");
			language = wxLANGUAGE_CHINESE;
			break;		
		case 6:
			//strcpy(newlang,"nl_NL");
			language = wxLANGUAGE_DUTCH;
			break;
		case 7:
			//strcpy(newlang,"en_EN");
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
			//strcpy(newlang,"de_CH");
			language = wxLANGUAGE_GERMAN_SWISS;
			break;
		case 14:
			//strcpy(newlang,"it_IT");
			language = wxLANGUAGE_ITALIAN;
			break;
		case 15:
			//strcpy(newlang,"ko_KR");
			language = wxLANGUAGE_KOREAN;
			break;
		case 16:
			//strcpy(newlang,"lt_LT");
			language = wxLANGUAGE_LITHUANIAN;
			break;
		case 17:
			//strcpy(newlang,"pl_PL");
			language = wxLANGUAGE_POLISH;
			break;
		case 18:
			//strcpy(newlang,"pt_PT");
			language = wxLANGUAGE_PORTUGUESE;
			break;
		case 19:
			//strcpy(newlang,"pt_BR");
			language = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
			break;
		case 20:
			//strcpy(newlang,"ru_RU");
			language = wxLANGUAGE_RUSSIAN;
			break;
		case 21:
			//strcpy(newlang,"es_ES");
			language = wxLANGUAGE_SPANISH;
			break;
		case 22:
			//strcpy(newlang,"es_CH");
			language = wxLANGUAGE_SPANISH_CHILE;
			break;
		case 23:
			//strcpy(newlang,"es_MX");
			language = wxLANGUAGE_SPANISH_MEXICAN;
			break;
		case 24:
			//strcpy(newlang,"tr_TR");
			//Turkish makes weird things with .eMule file!!! why?
			//language = wxLANGUAGE_TURKISH;
			language = wxLANGUAGE_DEFAULT;
			break;
		default:
			language = wxLANGUAGE_DEFAULT;
			break;

	}

	if ((!m_locale.Init(language)) && (language != wxLANGUAGE_DEFAULT)) {
		wxMessageBox(wxString::wxString(_("The selected locale seems not to be installed on your box\n You must generate it to use this language.\nA good start on linux systems is the file /etc/locale.gen and the package 'locales'\nGood luck!\n(Note: I'll try to set it anyway)")));
	}
	m_locale.AddCatalogLookupPathPrefix(LOCALEDIR);
	m_locale.AddCatalog(PACKAGE);

}

/**
 * Original code author: Julian Smart from wxWindows <http://www.wxwindows.org>
 * Imported and modified by Madcat.
 * Purpose: Launches default(?) browser with the URL passed in @url variable.
 * Default browser is detected by investigating mimetypes on win32, ??? on mac,
 * and hardcoded/user-choosable on *nix.
 * Problem on *nix with mimetypes is that on most systems, they'r so screwed up
 * that they'r not usable at all. Thus we hardcode few browsers here and offer
 * user a preferences setting for choosing browser.
 * Part   I: Mac OS X:       Unimplemented. Some Mac user should do it.
 * Part  II: Windows:        Theoretically should work, untested though.
 * Part III: Linux/BSD/etc:  Should work, attempts to launch mozilla, konqueror
 *                           & opera in this order.
 */
void CamuleApp::LaunchUrl(const wxString &url) {
wxString cmd;                        /* Temporary storage for launch command. */

#ifdef __WXMAC__
	#if 0
	// Kry -Uh?
	wxString url1(url);
	if (url1.Left(5) != wxT("file:"))
		url1 = wxNativePathToURL(url1);

	OSStatus err;
	ICInstance inst;
	SInt32 startSel;
	SInt32 endSel;

	err = ICStart(&inst, 'STKA'); // put your app creator code here
	if (err == noErr) {
		#if !TARGET_CARBON
		err = ICFindConfigFile(inst, 0, nil);
		#endif
		if (err == noErr) {
		startSel = 0;
		endSel = wxStrlen(url1);
		err = ICLaunchURL(inst, "\p", url1, endSel, &startSel, &endSel);
		}
		ICStop(inst);
	}
	#endif
#elif defined(__WXMSW__)
wxFileType *ft;                            /* Temporary storage for filetype. */

	ft = wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));
	if (!ft) {
		wxLogError(
			wxT("Impossible to determine the file type for extension html."
			"Please edit your MIME types.")
		);
		return;
	}

	if (!ft->GetOpenCommand(&cmd, wxFileType::MessageParameters(url, _T("")))) {
		// TODO: some kind of configuration dialog here.
		wxMessageBox(
			_("Could not determine the command for running the browser."),
			wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION);
		delete ft;
		return;
	}
	delete ft;

	wxPuts(wxString::Format(wxT("Launch Command: %s"), cmd.c_str()));
	if (!wxExecute(cmd, FALSE)) {
		wxLogError(wxT("Error launching browser for FakeCheck."));
	}
#else
	// Try with konqeror
	cmd = wxString::Format(wxT("konqueror %s"), url.c_str());
	wxPuts(wxString::Format(wxT("Launch Command: %s"), cmd.c_str()));
	if (wxExecute(cmd, false)) {
		return; // success
	}
	cmd = wxString::Format(wxT("opera %s"), url.c_str());
	wxPuts(wxString::Format(wxT("Launch Command: %s"), cmd.c_str()));
	if (wxExecute(cmd, false)) {
		return; // success
	}
	// Try with mozilla (don't ask about the command format.
	// really. seems it has to be like this)
	cmd = wxString::Format(wxT("xterm -e sh -c 'mozilla %s'"), url.c_str());
	wxPuts(wxString::Format(wxT("Launch Command: %s"), cmd.c_str()));
	if (wxExecute(cmd, false)) {
		return; // success
	}
	// Dewd? What kind of browser do YOU use?!
	// Join the Strange Browser Users Association (SBUA) now! Only $5.50!
	wxLogError(
		_("Unable to launch browser. Please set correct browser"
		"executable path in Preferences.")
	);
#endif
}

/**
 * Madcat - Generates url for checking the fakeness of given @file by creating
 * ED2K Link from the @file and creating correct URL for sending for browser.
 * @file	File to be generated URL from
 */
wxString CamuleApp::GenFakeCheckUrl(CAbstractFile *file) {
wxString url;                    /* Temporary storage for generating the URL. */

	url = wxString::Format(
		wxT("\"http://donkeyfakes.gambri.net/fakecheck/update/fakecheck.php?ed2k=%s\""),
		CreateED2kLink(file).c_str()
	);

	// Various symbols that don't work in URL... add as neccesery.
	url.Replace(wxT(" "), wxT("."));
	url.Replace(wxT("&"), wxT("%262"));

	return url;
}


void CamuleApp::Trigger_New_version(wxString old_version, wxString new_version) {
	
	wxString version_str;
	
	version_str += wxString(wxString::Format(_(" --- This is the first time you run aMule %s ---\n\n"),new_version.c_str()));	
	
	if (new_version == wxString("CVS")) {
		version_str += wxString(_("This version is a testing version, updated daily, and \n"));
		version_str += wxString(_("we give no warranty it won't break anything, burn your house,\n"));
		version_str += wxString(_("or kill your dog. But it *should* be safe to use anyway. \n"));		
	} else if ((new_version == wxString("1.2.7")) || (old_version == wxString("pre_1.2.7"))) {
		version_str += wxString(_("This version has new SecureHash support, so your \n"));
		version_str += wxString(_("client credits will be lost on this first run. \n"));
		version_str += wxString(_("There is no way to fix that, and eMule did the same.\n"));
		version_str += wxString(_("But your hash will be safe against stealers now, and your\n"));
		version_str += wxString(_("cryptokey.dat and clients.met are eMule compatible now.\n"));
		version_str += wxString(_("Just take them from your eMule config dir and put then on ~/.aMule.\n"));
		version_str += wxString(_("If your language is changed now, please set it again on preferences."));		
		if (old_version ==  wxString("1.2.6") || (old_version == wxString("pre_1.2.7"))) {
			WORD language_id = theApp.glob_prefs->GetLanguageID();
			if (language_id<4 && language_id!=0) {
				language_id ++; 
			} else if (language_id<7) {
				language_id += 2;
			} else if (language_id<22) {
				language_id += 3; 			
			}
			theApp.glob_prefs->SetLanguageID(language_id);
		}
	}		

	version_str += wxString(_("Feel free to report any bugs to forum.amule.org"));
		
	wxMessageBox(version_str, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);	
}
