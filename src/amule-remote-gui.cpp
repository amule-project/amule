#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <wx/defs.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>

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

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "server.h"			// Needed for GetListName
#include "otherfunctions.h"		// Needed for GetTickCount
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerWnd.h"			// Needed for CServerWnd
#include "StatisticsDlg.h"		// Needed for CStatisticsDlg
#include "Preferences.h"		// Needed for CPreferences
#include "StringFunctions.h"
#include "PartFile.h"			// Needed for CPartFile

#include "muuli_wdr.h"			// Needed for IDs
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"		// Needed for CSharedFilesCtrl
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "ClientListCtrl.h"
#include "ChatWnd.h"

BEGIN_EVENT_TABLE(CamuleRemoteGuiApp, wxApp)

	// Core timer
//		EVT_TIMER(ID_CORETIMER, CamuleRemoteGuiApp::OnCoreTimer)
		
//		EVT_CUSTOM(wxEVT_NOTIFY_EVENT, -1, CamuleRemoteGuiApp::OnNotifyEvent)
		
END_EVENT_TABLE()


IMPLEMENT_APP(CamuleRemoteGuiApp)

// Initialization of the static MyTimer member variables.
uint32 MyTimer::tic32 = 0;
uint64 MyTimer::tic64 = 0;
	
// Global timer. Used to cache GetTickCount() results for better performance.
class MyTimer* mytimer = NULL;

int CamuleRemoteGuiApp::OnExit()
{
	if (core_timer) {
		// Stop the Core Timer
		delete core_timer;
	}
	return wxApp::OnExit();
}

void CamuleRemoteGuiApp::ShutDown() {
	amuledlg->Destroy();
	if (mytimer) {
		delete mytimer;
		mytimer = NULL;
	}
}

bool CamuleRemoteGuiApp::OnInit()
{
	amuledlg = NULL;
	
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	mytimer = new MyTimer();
	
	if ( !wxApp::OnInit() ) {
		return false;
	}
	
	// Create the Core timer
	core_timer=new wxTimer(this,ID_CORETIMER);
	if (!core_timer) {
		printf("Fatal Error: Failed to create Core Timer");
		OnExit();
	}

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

void CamuleRemoteGuiApp::ShowAlert(wxString msg, wxString title, int flags)
{
	wxMessageBox(msg, title, flags);
}

int CamuleRemoteGuiApp::InitGui(bool geometry_enabled, wxString &geom_string)
{
	// Standard size is 800x600 at position (0,0)
	int geometry_x = 0;
	int geometry_y = 0;
	unsigned int geometry_width = 800;
	unsigned int geometry_height = 600;

	if ( geometry_enabled ) {	
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
	// Should default/last-used position be overridden?
	m_FrameTitle = wxString::Format(wxT("aMule %s"), wxT(VERSION));
	if ( geometry_enabled ) {
		amuledlg = new CamuleDlg(NULL, m_FrameTitle,
			wxPoint(geometry_x,geometry_y),
			wxSize( geometry_width, geometry_height - 58 ));
	} else {
		amuledlg = new CamuleDlg(NULL, m_FrameTitle);
	}
	SetTopWindow(amuledlg);

	return 0;
}
