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
	#include <wx/msw/registry.h>	// Needed for wxRegKey
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#ifdef __WXGTK__
	#include <execinfo.h>
	#include <mntent.h>
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

#include <wx/msgdlg.h>			// Needed for wxMessageBox
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

#include "amule.h"				// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "server.h"				// Needed for GetListName
#include "CFile.h"				// Needed for CFile
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"		// Needed for CQueueListCtrl
#include "UploadListCtrl.h"		// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "otherfunctions.h"		// Needed for GetTickCount
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
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
#include "FriendList.h"			// Needed for CFriendList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "PreferencesDlg.h"		// Needed for CPreferencesDlg
#include "Preferences.h"		// Needed for CPreferences
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

#ifdef __USE_SPLASH__
#include "splash.xpm"
#endif


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
	UnlimitResource(RLIMIT_RSS);
#endif
}


int CamuleApp::OnExit()
{
	printf("Now, exiting main app...\n");

	if (ipfilter) 
		delete ipfilter;

	delete ECServerHandler;

	delete listensocket;
	
	printf("aMule shutdown completed.\n");
	
	// Return 0 for succesful program termination
	return 0;
}


bool CamuleApp::OnInit()
{
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	mytimer = new MyTimer();

	// Initialization
	IsReady			= false;
	clientlist		= NULL;
	searchlist		= NULL;
	friendlist		= NULL;
	knownfiles		= NULL;
	serverlist		= NULL;
	serverconnect	= NULL;
	sharedfiles		= NULL;
	listensocket	= NULL;
	clientudp		= NULL;
	clientcredits	= NULL;
	downloadqueue	= NULL;
	uploadqueue 	= NULL;
	ipfilter		= NULL;
	
	// reset statistic values
	stat_sessionReceivedBytes = 0;
	stat_sessionSentBytes = 0;
	stat_reconnects = 0;
	stat_transferStarttime = 0;
	stat_serverConnectTime = 0;
	Start_time = GetTickCount64();
	sTransferDelay = 0.0;


	// Default geometry of the GUI. Can be changed with a cmdline argument...
	bool geometry_enabled = false;
	// Standard size is 800x600 at position (0,0)
	int geometry_x = 0;
	int geometry_y = 0;
	unsigned int geometry_width = 800;
	unsigned int geometry_height = 600;


	// catch fatal exceptions
	wxHandleFatalExceptions(true);

	// Apprently needed for *BSD
	SetResourceLimits();

	
	// Parse cmdline arguments. 
	wxCmdLineParser cmdline(wxApp::argc, wxApp::argv);

	// Handle these arguments.
	cmdline.AddSwitch("v", "version", "Displays the current version number.");
	cmdline.AddSwitch("h", "help", "Displays this information.");
	cmdline.AddOption("geometry", "", "Sets the geometry of the app.\n\t\t\t<str> uses the same format as standard X11 apps:\n\t\t\t[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]");
	cmdline.Parse();

	if ( cmdline.Found("version") ) {
		printf("aMule %s\n", VERSION);
			
		return false;
	}

	if ( cmdline.Found("help") ) {
		cmdline.Usage();
		
		return false;
	}

	wxString geom_string;
	if ( cmdline.Found("geometry", &geom_string) ) {
	
		// I plan on moving this to a seperate function, as it just clutters up OnInit()
		
#ifdef __WXGTK__
	
		XParseGeometry(geom_string.c_str(), &geometry_x, &geometry_y, &geometry_width, &geometry_height);

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

	SetVendorName("TikuWarez");

	// Do NOT change this string to aMule nor anything else, it WILL fuck you up.
	SetAppName("eMule");


	// see if there is another instance running
	wxString server = getenv("HOME") + wxString("/.aMule/muleconn");
	wxClient* client = new wxClient();
	wxConnectionBase* conn = client->MakeConnection("localhost", server, wxT("aMule IPC TESTRUN"));
	
	// If the connection failed, conn is NULL	
	if ( conn ) {
		// An instance is already running!
		conn->Disconnect();
		delete conn;
		delete client;
		
		printf("aMule already running: exiting\n");
		return false;
	}

	// If there was no server, start one 
	localserver = new wxServer();
	localserver->Create(server);


	// Close standard-input
	close(0);

	/* If no aMule configuration files exist, see if either lmule or xmule config
	   exists, so that we can use those. */
	wxString lMulePrefDir = getenv("HOME") + wxString("/.lmule");
	wxString xMulePrefDir = getenv("HOME") + wxString("/.xMule");
	wxString aMulePrefDir = getenv("HOME") + wxString("/.aMule");
	
	if ( !wxDirExists( aMulePrefDir ) ) {
		if ( wxDirExists( lMulePrefDir ) ) {
			printf("Found lMule old settings, moving to new dir.\n");
			wxRenameFile(lMulePrefDir, aMulePrefDir);
		
		} else if ( wxDirExists(xMulePrefDir) ) {
			printf("Found xMule old settings, copying config & credits files.\n");
			wxMkdir(aMulePrefDir);

			// Copy .dat files to the aMule dir
			wxString file = wxFindFirstFile(xMulePrefDir + "/*.dat", wxFILE);
  			while ( !file.IsEmpty() ) {
				wxCopyFile( file, aMulePrefDir + "/" + file.AfterLast('/'));
				
				file = wxFindNextFile();
  			}
		
			// Copy .met files to the aMule dir
			file = wxFindFirstFile(xMulePrefDir + "/*.met", wxFILE);
  			while ( !file.IsEmpty() ) {
				wxCopyFile( file, aMulePrefDir + "/" + file.AfterLast('/'));
			
				file = wxFindNextFile();
  			}
		
			wxMessageBox(wxT("Copied old ~/.xMule config and credit files to ~/.aMule\nHowever, be sure NOT to remove .xMule if your Incoming / Temp folders are still there ;)"), wxT("Info"), wxOK);
		}
	}


	// Delete old log file.
	wxRemoveFile(wxString::Format("%s/.aMule/logfile", getenv("HOME")));
	
	// Load Preferences
	glob_prefs = new CPreferences();


	// Display notification on new version or first run
	wxTextFile vfile( aMulePrefDir + wxString("/lastversion") );
	wxString newMule(VERSION);
	if ( vfile.Open() && !vfile.Eof() ) {
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
			
		vfile.AddLine(VERSION);
		vfile.Write();
		vfile.Close();
	}	
	

	use_chmod = true;
#ifdef __WXGTK__
	/* Test to see if the Temp or the Incoming dir is on a vfat partition. If
	   that is the case, we need to avoid chmoding to avoid lots of warnings.
	   This is done by reading through fstab entries and comparing to the 
	   folders used for incomming and temp files. */
	
	FILE* mnt_tab = setmntent("/etc/mtab","r");
	if ( mnt_tab ) {
		wxString incomingdir = glob_prefs->GetIncomingDir();
		wxString tempdir = glob_prefs->GetTempDir();
		struct mntent* entries;

		entries = getmntent(mnt_tab);
		while ( entries ) {
			if ( strncmp(entries->mnt_type, "vfat",4) == 0 ) {
				if ( tempdir.StartsWith( entries->mnt_dir ) ) {
					amuledlg->AddLogLine(false, "Temp dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings.");
					use_chmod = false;
				} else if ( incomingdir.StartsWith( entries->mnt_dir ) ) {
					amuledlg->AddLogLine(false, "Incoming dir is placed on a FAT32 partition. Disabling chmod to avoid useless warnings.");
					use_chmod = false;
				}
			}

			entries = getmntent(mnt_tab);
		}
	
		fclose(mnt_tab);
	}
#endif


	// Load localization settings
	Localize_mule();
	

	// Create main dialog
	amuledlg = new CamuleDlg(NULL, wxString::Format(wxT("aMule %s"), wxT(VERSION)));

	
	// Should default/last-used position be overridden?
	if ( geometry_enabled ) {
		amuledlg->Move( geometry_x, geometry_y );
		amuledlg->SetClientSize( geometry_width, geometry_height - 58 );
	}
	
	amuledlg->Show(TRUE);


#ifndef DISABLE_OLDPREFS
	amuledlg->preferenceswnd->SetPrefs(glob_prefs);
#endif


	// Get ready to handle connections from apps like amulecmd
	ECServerHandler = new ExternalConn();


#ifndef __SYSTRAY_DISABLED__
	amuledlg->CreateSystray(wxString::Format(wxT("%s %s"), wxT(PACKAGE), wxT(VERSION)));
#endif 
        

	// splashscreen
	#ifdef __USE_SPLASH__
	if (glob_prefs->UseSplashScreen() && !glob_prefs->GetStartMinimized()) {
		new wxSplashScreen( wxBitmap(splash_xpm),
		                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
		                    5000, NULL, -1, wxDefaultPosition, wxDefaultSize,
		                    wxSIMPLE_BORDER|wxSTAY_ON_TOP
		);
	}
	#endif


	clientlist		= new CClientList();
	searchlist		= new CSearchList();
	friendlist		= new CFriendList();
	knownfiles		= new CKnownFileList(glob_prefs->GetAppDir());
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
	

	// Init statistics stuff, better do it asap
	amuledlg->statisticswnd->Init();
	amuledlg->statisticswnd->SetUpdatePeriod();

	// must do initialisations here.. 
	amuledlg->serverwnd->serverlistctrl->Init(serverlist);
	serverlist->Init();

	// init downloadqueue
	downloadqueue->Init();
	amuledlg->AddLogLine(true, PACKAGE_STRING);

	sharedfiles->SetOutputCtrl((CSharedFilesCtrl *) amuledlg->sharedfileswnd->FindWindowByName("sharedFilesCt"));

	// then init firend list
	friendlist->SetWindow((CFriendListCtrl *) amuledlg->transferwnd->FindWindowById(ID_FRIENDLIST));
	friendlist->ShowFriends();


	SetTopWindow(amuledlg);


	// Initialize and sort all lists.
	// FIX: Remove from here and put these back to the OnInitDialog()s
	// and call the OnInitDialog()s here!
	amuledlg->transferwnd->downloadlistctrl->InitSort();
	amuledlg->transferwnd->uploadlistctrl->InitSort();
	amuledlg->transferwnd->queuelistctrl->InitSort();
	amuledlg->serverwnd->serverlistctrl->InitSort();
	amuledlg->sharedfileswnd->sharedfilesctrl->InitSort();

	// call the initializers
	amuledlg->transferwnd->OnInitDialog();

	amuledlg->m_app_state = APP_STATE_RUNNING;
	
	// reload shared files
	sharedfiles->Reload(true, true);


	// Create a socket and start listening
	myaddr.Service(glob_prefs->GetPort());
	listensocket = new CListenSocket(glob_prefs, myaddr);
	listensocket->StartListening();

	// If we wern't able to start listening, we need to warn the user
	if ( !listensocket->Ok() ) {
		amuledlg->AddLogLine(false, wxT(_("Port %d is not available. You will be LOWID")), glob_prefs->GetPort());
		wxString str;
		str.Format(_("Port %d is not available !!\n\nThis will mean that you will be LOWID.\n\nUse netstat to determine when port becomes available\nand try starting amule again."), glob_prefs->GetPort());
		
		wxMessageBox(str, _("Error"), wxCENTRE | wxOK | wxICON_ERROR);
	}


	// Must we start minimized?
	if (glob_prefs->GetStartMinimized()) {
		// Send it to tray?
		if (glob_prefs->DoMinToTray()) {
			amuledlg->Hide_aMule();
		} else {
			amuledlg->Iconize(TRUE);
		}
	}
	
	
	// Autoconnect if that option is enabled 
	if (glob_prefs->DoAutoConnect()) {
		wxCommandEvent nullEvt;
		amuledlg->OnBnConnect(nullEvt);
	}


	if (glob_prefs->GetMaxGraphDownloadRate() < glob_prefs->GetMaxDownload())
		glob_prefs->SetDownloadlimit(UNLIMITED);
		
	if (glob_prefs->GetMaxGraphUploadRate() < glob_prefs->GetMaxUpload())
		glob_prefs->SetUploadlimit(UNLIMITED);

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
	if (theApp.glob_prefs->GetSrcSeedsOn()) {
		theApp.downloadqueue->LoadSourceSeeds();
	}

	return TRUE;
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
			case '\"':
			case '*':
			case '<':
			case '>':
			case '?':
			case '|':
			case '\\':
			case '/':
			case ':':
				continue;
			default:
				// Many illegal for filenames in windows
				if ( strText[i] > 31 ) {
						result += strText[i];
				}
		}
	}

	// Should we replace spaces?
	if ( !bKeepSpaces ) {
		result.Replace(" ", "_", TRUE);
	}

	return result;
}


// Returns a ed2k file URL
wxString CamuleApp::CreateED2kLink(CAbstractFile* f)
{
	wxString strURL;

	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	strURL << "ed2k://|file|"
	       << StripInvalidFilenameChars(f->GetFileName(), true)
		   << "|"
	       << f->GetFileSize()
		   << "|"
		   << EncodeBase16( f->GetFileHash(), 16 )
		   << "|/";
	
	return strURL;
}


// Returns a ed2k source URL
wxString CamuleApp::CreateED2kSourceLink(CAbstractFile* f)
{
	if ( !serverconnect->IsConnected() || serverconnect->IsLowID() ) {
		wxMessageBox(_("You need a HighID to create a valid sourcelink"));
		return "";
	}
	
	uint32 clientID = serverconnect->GetClientID();
	
	// Create the first part of the URL
	wxString strURL = CreateED2kLink( f );
	
	// And append the source information: "|sources,<ip>:<port>|/"
	strURL << "|sources,"
	       << (uint8) clientID << "."
		   << (uint8) (clientID >> 8) << "."
		   << (uint8) (clientID >> 16) << "."
		   << (uint8) (clientID >> 24) << ":"
		   << glob_prefs->GetPort() << "|/";

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
	strURL << "|sources,"
	       << glob_prefs->GetYourHostname() << ":"
		   << glob_prefs->GetPort() << "|/";

	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,<host>:<port>|/"
	return strURL;	
}


// Creates a ED2k hyperlink 
wxString CamuleApp::CreateHTMLED2kLink(CAbstractFile* f)
{
	wxString strCode = "<a href=\"" + CreateED2kLink(f) + "\">" + StripInvalidFilenameChars(f->GetFileName(), true) + "</a>";
	return strCode;
}


// Generates an URL for checking if a file is "fake"
wxString CamuleApp::GenFakeCheckUrl(CAbstractFile *f)
{
	wxString strURL = "http://donkeyfakes.gambri.net/fakecheck/update/fakecheck.php?ed2k=";
	
	wxString strED2kURL = CreateED2kLink( f );

	// Various symbols that don't work in URLs... add as neccesery.
	strED2kURL.Replace( " ", "." );
	strED2kURL.Replace( "&", "%262" );

	strURL += strED2kURL;

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
	if (!glob_prefs->IsOnlineSignatureEnabled()) {
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
		amuledlg->AddLogLine(true, wxString(_("Failed to save"))+wxString(_(" OnlineSig File")));
	}
	if (!amulesig_out.Open(amulesig_path, CFile::write)) {
		amuledlg->AddLogLine(true, wxString(_("Failed to save"))+wxString(" aMule OnlineSig File"));
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
			if (serverconnect->IsLowID()) {
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
		sprintf(buffer,"%d", sharedfiles->GetCount());
		amulesig_out.Write(buffer, strlen(buffer));
		amulesig_out.Write("\n",1);
	}	/* if (!zero) */

	// Nick on the network
	sprintf(buffer, "%s", glob_prefs->GetUserNick());
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total received in GB
	sprintf(buffer, "%.2f", (float)(stat_sessionReceivedBytes+glob_prefs->GetTotalDownloaded()) / 1073741824);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// Total sent in GB
	sprintf(buffer, "%.2f", (float)(stat_sessionSentBytes+glob_prefs->GetTotalUploaded()) / 1073741824);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

	// amule version 
	sprintf(buffer,"%s",VERSION);
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);

        // Total received in MB in session
        sprintf(buffer, "%.2f", (float)(stat_sessionReceivedBytes) / 1048576);
        amulesig_out.Write(buffer, strlen(buffer));
        amulesig_out.Write("\n",1);
				
        // Total sent in GB in session
        sprintf(buffer, "%.2f", (float)(stat_sessionSentBytes) / 1048576);
        amulesig_out.Write(buffer, strlen(buffer));
        amulesig_out.Write("\n",1);

	// Uptime
	sprintf(buffer,"%s",CastSecondsToHM(theApp.GetUptimeSecs()).GetData());
	amulesig_out.Write(buffer, strlen(buffer));
	amulesig_out.Write("\n",1);
					
	// Close the files
	emulesig_out.Close();
	amulesig_out.Close();
	delete[] emulesig_path;
	delete[] amulesig_path;
} //End Added By Bouc7


// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
	// Close sockets first.
	if ( listensocket )
		listensocket->Destroy();
	if ( clientudp )
		clientudp->Destroy();

	// (stkn) create backtrace
#ifdef __WXGTK__
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
	return true;
}


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
	int language;
	
	switch (glob_prefs->GetLanguageID()) {
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
			//strcpy(newlang,"ca_ES")
			language = wxLANGUAGE_CATALAN;
			break;
		case 5:
			//strcpy(newlang,"zh_CN");
			language = wxLANGUAGE_CHINESE;
			break;		
		case 6:
			//strcpy(newlang,"da_DK");
			language = wxLANGUAGE_DANISH;
			break;
		case 7:
			//strcpy(newlang,"nl_NL");
			language = wxLANGUAGE_DUTCH;
			break;
		case 8:
			//strcpy(newlang,"en_GB");
			language = wxLANGUAGE_ENGLISH;
			break;
		case 9:
			//strcpy(newlang,"et_EE");
			language = wxLANGUAGE_ESTONIAN;
			break;
		case 10:
			//strcpy(newlang,"fi");
			language = wxLANGUAGE_FINNISH;
			break;
		case 11:
			//strcpy(newlang,"fr_FR");
			language = wxLANGUAGE_FRENCH;
			break;
		case 12:
			//strcpy(newlang,"gl_ES");
			language = wxLANGUAGE_GALICIAN;
			break;
		case 13:
			//strcpy(newlang,"de_DE");
			language = wxLANGUAGE_GERMAN;
			break;
		case 14:
			//strcpy(newlang,"de_CH");
			language = wxLANGUAGE_GERMAN_SWISS;
			break;
		case 15:
			//strcpy(newlang,"it_IT");
			language = wxLANGUAGE_ITALIAN;
			break;
		case 16:
			//strcpy(newlang,"ko_KR");
			language = wxLANGUAGE_KOREAN;
			break;
		case 17:
			//strcpy(newlang,"lt_LT");
			language = wxLANGUAGE_LITHUANIAN;
			break;
		case 18:
			//strcpy(newlang,"pl_PL");
			language = wxLANGUAGE_POLISH;
			break;
		case 19:
			//strcpy(newlang,"pt_PT");
			language = wxLANGUAGE_PORTUGUESE;
			break;
		case 20:
			//strcpy(newlang,"pt_BR");
			language = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
			break;
		case 21:
			//strcpy(newlang,"ru_RU");
			language = wxLANGUAGE_RUSSIAN;
			break;
		case 22:
			//strcpy(newlang,"es_ES");
			language = wxLANGUAGE_SPANISH;
			break;
		case 23:
			//strcpy(newlang,"es_CH");
			language = wxLANGUAGE_SPANISH_CHILE;
			break;
		case 24:
			//strcpy(newlang,"es_MX");
			language = wxLANGUAGE_SPANISH_MEXICAN;
			break;
		case 25:
			//Turkish makes weird things with .eMule file!!! why?
			//language = wxLANGUAGE_TURKISH;
			language = wxLANGUAGE_DEFAULT;
			break;
		case 26:
			//strcpy(newlang,"hu");
			language = wxLANGUAGE_HUNGARIAN;
			break;
		case 27:
			//strcpy(newlang,"ca_ES");
			language = wxLANGUAGE_CATALAN;
			break;
		default:
			language = wxLANGUAGE_DEFAULT;
			break;

	}

	if ((!m_locale.Init(language)) && (language != wxLANGUAGE_DEFAULT)) {
		
		wxMessageBox(wxT(_("The selected locale seems not to be installed on your box\n You must generate it to use this language.\nA good start on linux systems is the file /etc/locale.gen and the package 'locales'\nGood luck!\n(Note: I'll try to set it anyway)")));
	}
	
	m_locale.AddCatalogLookupPathPrefix(LOCALEDIR);
	m_locale.AddCatalog(PACKAGE);
}


/*
	Try to launch the specified url:
	 - Windows: The default browser will be used.
	 - Mac: Currently not implemented
	 - Anything else: Try a number of hardcoded browsers. Should be made configuable...
*/	
void CamuleApp::LaunchUrl( const wxString& url )
{
	wxString cmd;
	
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


#elif defined (__WXMSW__)

	// This is where the default browser is stored
	wxRegKey key( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\http\\shell\\open\\command" );

	wxString value = key.QueryDefaultValue();

	if ( !value.IsEmpty() ) {
		cmd = value + " " + url;

		if ( !wxExecute( cmd, false ) ) {
			wxLogError(wxT("Error launching browser for FakeCheck."));
		}
	} else {
		wxMessageBox( _("Could not determine the command for running the browser."), wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION);
	}

#else

	wxArrayString list;

	// Try with these browers
	list.Add( "firefox '%s'" );
	list.Add( "konqueror '%s'" );
	list.Add( "galeon '%s'" );
	list.Add( "opera '%s'" );
	
	// ( Don't ask about the command format. Really. Seems it has to be like this )
	list.Add( "xterm -e sh -c 'mozilla %s'" ); 

	for ( unsigned int i = 0; i < list.GetCount(); i++ ) {
		cmd = list[i];
		cmd.Replace( "%s", url );
	
		// Pipes cause problems, so escape them
		cmd.Replace( "|", "%7C" );
		
		if ( wxExecute( cmd, false ) ) {
			printf( "Launch Command: %s", cmd.c_str() );
			return;
		}
	}

	// Unable to execute browser. But this error message doesn't make sense, 
	// cosidering that you _can't_ set the browser executable path... =/
	wxLogError( _("Unable to launch browser. Please set correct browser executable path in Preferences.") );
	
#endif

}


// Displays information related to important changes in aMule.
// Is called when the user runs a new version of aMule
void CamuleApp::Trigger_New_version(wxString old_version, wxString new_version)
{
	wxString info;
	
	info = _(" --- This is the first time you run aMule %s ---\n\n");
	info.Replace( "%s", new_version );	
	
	if (new_version == wxT("CVS")) {
		info += wxT(_("This version is a testing version, updated daily, and \n"));
		info += wxT(_("we give no warranty it won't break anything, burn your house,\n"));
		info += wxT(_("or kill your dog. But it *should* be safe to use anyway. \n"));		
	} else if ((new_version == wxT("2.0.0rc1")) || (old_version == wxT("1.2.6"))) {
		info += wxT(_("This version has new SecureIdent support, so your \n"));
		info += wxT(_("client credits will be lost on this first run. \n"));
		info += wxT(_("There is no way to fix that, and eMule did the same.\n"));
		info += wxT(_("But your hash will be safe against stealers now, and your\n"));
		info += wxT(_("cryptokey.dat and clients.met are eMule compatible now.\n"));
		info += wxT(_("Just take them from your eMule config dir and put then on ~/.aMule.\n"));
		
	}		

	info += wxT(_("Feel free to report any bugs to forum.amule.org"));
		
	wxMessageBox(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);	

	// Set to system default... no other way AFAIK unless we change the save type.			
		
	glob_prefs->SetLanguageID(0);
	
}
