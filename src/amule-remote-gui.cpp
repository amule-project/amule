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

// Global timer. Used to cache GetTickCount() results for better performance.
MyTimer* mytimer = NULL;

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

	core_timer->Start(100);	

	amuledlg->StartGuiTimer();

	return true;

}

void CamuleRemoteGuiApp::ShowAlert(wxString msg, wxString title, int flags)
{
	CamuleGuiBase::ShowAlert(msg, title, flags);
}

int CamuleRemoteGuiApp::InitGui(bool geometry_enabled, wxString &geom_string)
{

	return CamuleGuiBase::InitGui(geometry_enabled, geom_string);
}

bool CamuleRemoteGuiApp::CopyTextToClipboard(wxString strText)
{
	return CamuleGuiBase::CopyTextToClipboard(strText);
}

uint32 CamuleRemoteGuiApp::GetPublicIP()
{
	return 0;
}

// remote gui doesn't have it's own log
void CamuleRemoteGuiApp::QueueLogLine(bool, wxString const&)
{
}

wxString CamuleRemoteGuiApp::GetLog(bool)
{
	return wxEmptyString;
}

wxString CamuleRemoteGuiApp::GetServerLog(bool)
{
	return wxEmptyString;
}

//
// Container implenentation
//
CServerConnectRem::CServerConnectRem(CRemoteConnect *conn)
{
	m_Conn = conn;
}

void CServerConnectRem::ConnectToAnyServer()
{
	CECPacket req(EC_OP_SERVER_CONNECT);
	m_Conn->Send(&req);
}

void CServerConnectRem::StopConnectionTry()
{
	// lfroen: isn't Disconnect the same ?
}

void CServerConnectRem::Disconnect()
{
	CECPacket req(EC_OP_SERVER_DISCONNECT);
	m_Conn->Send(&req);
}

void CServerConnectRem::ConnectToServer(CServer *server)
{
	CECPacket req(EC_OP_SERVER_CONNECT);
	uint32 ip = server->GetIP();
	uint16 port = server->GetPort();
	req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	m_Conn->Send(&req);
}

CServer *CServerConnectRem::GetCurrentServer()
{
	// lfroen: must find out how to do such
	return 0;
}

CServerListRem::CServerListRem(CRemoteConnect *conn) : CRemoteContainer<CServer, uint32, CEC_Server_Tag>(conn)
{
}

void CServerListRem::UpdateServerMetFromURL(wxString url)
{
	// FIXME: add command
}

void CServerListRem::SaveServermetToFile()
{
	// lfroen: stub, nothing to do
}

void CServerListRem::RemoveServer(CServer* server)
{
	CECPacket req(EC_OP_SERVER_REMOVE);
	uint32 ip = server->GetIP();
	uint16 port = server->GetPort();
	req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	m_conn->Send(&req);
}


void CIPFilterRem::Reload()
{
	CECPacket req(EC_OP_IPFILTER_RELOAD);
	m_conn->Send(&req);
}


void CSharedFilesRem::Reload(bool, bool)
{
	CECPacket req(EC_OP_SHAREDFILES_RELOAD);
	m_conn->Send(&req);
}


/*!
 * Connection to remote core
 * 
 * FIXME: implementation is waiting
 */
CRemoteConnect::CRemoteConnect()
{
}

CECPacket *CRemoteConnect::SendRecv(CECPacket *)
{
	return 0;
}

void CRemoteConnect::Send(CECPacket *)
{
}


//
// since gui is not linked with amule.cpp - define events here
//
DEFINE_EVENT_TYPE(wxEVT_NOTIFY_EVENT)
DEFINE_EVENT_TYPE(wxEVT_AMULE_TIMER)

DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_EVENT_TYPE(wxEVT_CORE_DNS_DONE)
