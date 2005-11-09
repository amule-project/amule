//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <memory>			// Needed for auto_ptr
using std::auto_ptr;

#include <wx/defs.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>

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
#include <wx/checkbox.h>
#include <wx/fileconf.h>		// Needed for wxFileConfig

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "Server.h"			// Needed for GetListName
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerWnd.h"			// Needed for CServerWnd
#include "StatisticsDlg.h"		// Needed for CStatisticsDlg
#include "Preferences.h"		// Needed for CPreferences
#include "StringFunctions.h"
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"
#include "Logger.h"
#include "muuli_wdr.h"			// Needed for IDs
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchList.h"			// Needed for CSearchFile
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "SharedFilesCtrl.h"		// Needed for CSharedFilesCtrl
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "ClientListCtrl.h"
#include "ServerListCtrl.h"
#include "ClientCredits.h"
#include "Format.h"
#include "OtherFunctions.h"		// Needed for CastItoIShort

#include "CMD4Hash.h"
#include "ECSocket.h"
#include "ECPacket.h"
#include "ECcodes.h"
#include "ECVersion.h"

#include "MD5Sum.h"

CEConnectDlg::CEConnectDlg() :
	wxDialog(theApp.amuledlg, -1, _("Connect to remote amule"), wxDefaultPosition )
{
	CoreConnect(this, TRUE);
	
	wxString pref_host, pref_port;
	wxConfig::Get()->Read(wxT("/EC/Host" ), &pref_host, wxT("localhost"));
	wxConfig::Get()->Read(wxT("/EC/Port" ), &pref_port, wxT("4712"));
	wxConfig::Get()->Read(wxT("/EC/Password" ), &pwd_hash);
	
	CastChild(ID_REMOTE_HOST, wxTextCtrl)->SetValue(pref_host);
	CastChild(ID_REMOTE_PORT, wxTextCtrl)->SetValue(pref_port);
	CastChild(ID_EC_PASSWD, wxTextCtrl)->SetValue(pwd_hash);
	
	CentreOnParent();
}

wxString CEConnectDlg::PassHash()
{
	return pwd_hash;
}

BEGIN_EVENT_TABLE(CEConnectDlg, wxDialog)
  EVT_BUTTON(wxID_OK, CEConnectDlg::OnOK)
END_EVENT_TABLE()

void CEConnectDlg::OnOK(wxCommandEvent& evt)
{
	wxString s_port = CastChild(ID_REMOTE_PORT, wxTextCtrl)->GetValue();
	port = StrToLong( s_port );
	
	host = CastChild(ID_REMOTE_HOST, wxTextCtrl)->GetValue();
	passwd = CastChild(ID_EC_PASSWD, wxTextCtrl)->GetValue();

	if ( passwd != pwd_hash ) {
		pwd_hash = MD5Sum(passwd).GetHash();
	}
	m_save_user_pass = CastChild(ID_EC_SAVE, wxCheckBox)->IsChecked();
	evt.Skip();
}

BEGIN_EVENT_TABLE(CamuleRemoteGuiApp, wxApp)

	// Core timer
	EVT_TIMER(ID_CORETIMER, CamuleRemoteGuiApp::OnCoreTimer)

//	EVT_CUSTOM(wxEVT_MULE_NOTIFY_EVENT, -1, CamuleRemoteGuiApp::OnNotifyEvent)

END_EVENT_TABLE()


IMPLEMENT_APP(CamuleRemoteGuiApp)


int CamuleRemoteGuiApp::OnExit()
{
	if (core_timer) {
		// Stop the Core Timer
		delete core_timer;
	}
	if (amuledlg) {
		amuledlg->StopGuiTimer();
	}
	return wxApp::OnExit();
}

void CamuleRemoteGuiApp::OnCoreTimer(AMULE_TIMER_EVENT_CLASS&)
{
	if ( connect->Busy() ) {
		return;
	}
	// always query connection state and stats
	serverconnect->ReQuery();

	{
		CECPacket stats_req(EC_OP_STAT_REQ);
		auto_ptr<CECPacket> stats(connect->SendRecv(&stats_req));
		if ( !stats.get() ) {
			core_timer->Stop();
			wxMessageBox(_("Connection to remote aMule is lost. Exiting now."),
				_("Error: connection lost"), wxICON_ERROR);
			ExitMainLoop();
			return;
		}
		statistics->UpdateStats(stats.get());
	}
	
	if ( amuledlg->sharedfileswnd->IsShown() ) {
		sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);
	} else if ( amuledlg->serverwnd->IsShown() ) {
		//serverlist->FullReload(EC_OP_GET_SERVER_LIST);
		//serverlist->ReloadControl();
	} else if ( amuledlg->transferwnd->IsShown() ) {
		downloadqueue->DoRequery(EC_OP_GET_DLOAD_QUEUE, EC_TAG_PARTFILE);
		switch(amuledlg->transferwnd->clientlistctrl->GetListView()) {
			case vtUploading:
				uploadqueue->ReQueryUp();
				break;
			case vtQueued:
				uploadqueue->ReQueryWait();
				break;
			case vtClients:
				break;
			case vtNone:
				break;
		}
		amuledlg->transferwnd->ShowQueueCount(theStats::GetWaitingUserCount());
	} else if ( amuledlg->searchwnd->IsShown() ) {
		if ( searchlist->m_curr_search != -1 ) {
			searchlist->DoRequery(EC_OP_SEARCH_RESULTS, EC_TAG_SEARCHFILE);
		}
	}
	//amuledlg->ShowTransferRate();
	serverlist->UpdateUserFileStatus(serverconnect->GetCurrentServer());
}

void CamuleRemoteGuiApp::ShutDown(wxCloseEvent &WXUNUSED(evt)) {
	amuledlg->Destroy();
}

bool CamuleRemoteGuiApp::OnInit()
{
	amuledlg = NULL;
	
	if ( !wxApp::OnInit() ) {
		return false;
	}

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	// Create the Core timer
	core_timer = new wxTimer(this,ID_CORETIMER);
	if (!core_timer) {
		printf("Fatal Error: Failed to create Core Timer");
		OnExit();
	}

	connect = new CRemoteConnect;

	SetAppName(wxT("aMule"));
	
	// Load Preferences
	// This creates the CFG file we shall use
	ConfigDir = GetConfigDir();
	if ( !wxDirExists( ConfigDir ) ) {
		wxMkdir( ConfigDir, CPreferences::GetDirPermissions() );
	}
	wxConfig::Set(new wxFileConfig(wxEmptyString, wxEmptyString, ConfigDir + wxT("remote.conf")));

	glob_prefs = new CPreferencesRem(connect);	
	
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));

	bool result = ShowConnectionDialog();

	if (result) {
	#ifndef CLIENT_GUI
		Startup();
	#else
		printf("Going for main loop while we wait for events\n");	
	#endif
	}
	return result;
}

bool CamuleRemoteGuiApp::CryptoAvailable() const
{
	return clientcredits && clientcredits->CryptoAvailable();
}

bool CamuleRemoteGuiApp::ShowConnectionDialog() {
	
	dialog = new CEConnectDlg;
	do {
		
		if ( dialog->ShowModal() != wxID_OK ) {
			dialog->Destroy();
			return false;
		}
	} while ( !connect->Connect(dialog->Host(), dialog->Port(), dialog->Login(), dialog->PassHash()) );

	return true;	
}

void CamuleRemoteGuiApp::Startup() {
	
	if ( dialog->SaveUserPass() ) {
		wxConfig::Get()->Write(wxT("/EC/Host" ), dialog->Host());
		wxConfig::Get()->Write(wxT("/EC/Port" ), dialog->Port());
		wxConfig::Get()->Write(wxT("/EC/Password" ), dialog->PassHash());
	}
	dialog->Destroy();
	
	glob_prefs->LoadRemote();

	serverconnect = new CServerConnectRem(connect);
	statistics = new CStatistics(connect);
	
	clientlist = new CClientListRem(connect);
	searchlist = new CSearchListRem(connect);
	serverlist = new CServerListRem(connect);
	
	sharedfiles	= new CSharedFilesRem(connect);
	knownfiles = new CKnownFilesRem(sharedfiles);

	clientcredits = new CClientCreditsRem();
	
	// bugfix - do this before creating the uploadqueue
	downloadqueue = new CDownQueueRem(connect);
	uploadqueue = new CUpQueueRem(connect);
	ipfilter = new CIPFilterRem();

	// Parse cmdline arguments.
	wxCmdLineParser cmdline(wxApp::argc, wxApp::argv);
	cmdline.AddSwitch(wxT("v"), wxT("version"), wxT("Displays the current version number."));
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("Displays this information."));
	cmdline.AddOption(wxT("geometry"), wxEmptyString, wxT("Sets the geometry of the app.\n\t\t\t<str> uses the same format as standard X11 apps:\n\t\t\t[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"));
	cmdline.Parse();

	
	bool geometry_enabled = false;
	wxString geom_string;
	if ( cmdline.Found(wxT("geometry"), &geom_string) ) {
		geometry_enabled = true;
	}
	
	// Create main dialog
	InitGui(0, geom_string);

	serverlist->FullReload(EC_OP_GET_SERVER_LIST);
	serverlist->ReloadControl();
	sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);

	// Start the Core Timer
	core_timer->Start(1000);	

	amuledlg->StartGuiTimer();
	
}

void CamuleRemoteGuiApp::ShowAlert(wxString msg, wxString title, int flags)
{
	CamuleGuiBase::ShowAlert(msg, title, flags);
}

int CamuleRemoteGuiApp::InitGui(bool geometry_enabled, wxString &geom_string)
{
	CamuleGuiBase::InitGui(geometry_enabled, geom_string);
	SetTopWindow(amuledlg);
	return 0;
}

bool CamuleRemoteGuiApp::CopyTextToClipboard(wxString strText)
{
	return CamuleGuiBase::CopyTextToClipboard(strText);
}

uint32 CamuleRemoteGuiApp::GetPublicIP()
{
	return 0;
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
// Remote gui can't create links by itself. Pass request or retrieve from container ?
//
wxString CamuleRemoteGuiApp::CreateED2kLink(CAbstractFile const*)
{
	return wxEmptyString;
}

wxString CamuleRemoteGuiApp::CreateED2kSourceLink(CAbstractFile const *)
{
	return wxEmptyString;
}

wxString CamuleRemoteGuiApp::CreateED2kHostnameSourceLink(CAbstractFile const *)
{
	return wxEmptyString;
}

wxString CamuleRemoteGuiApp::CreateHTMLED2kLink(CAbstractFile const*f)
{
	wxString strCode = wxT("<a href=\"") + 
		CreateED2kLink(f) + wxT("\">") + 
		CleanupFilename(f->GetFileName(), true) + wxT("</a>");
	return strCode;
}

wxString CamuleRemoteGuiApp::CreateED2kAICHLink(CKnownFile const *)
{
	return wxEmptyString;
}

bool CamuleRemoteGuiApp::AddServer(CServer *, bool)
{
	#warning TODO: Add remote command
	return true;
}

void CamuleRemoteGuiApp::NotifyEvent(const GUIEvent& event)
{
	switch (event.ID) {
	        case SEARCH_ADD_TO_DLOAD:
		        downloadqueue->AddSearchToDownload((CSearchFile *)event.ptr_value, event.byte_value);
				break;

	        case PARTFILE_REMOVE_NO_NEEDED:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_REMOVE_NO_NEEDED);
				break;
	        case PARTFILE_REMOVE_FULL_QUEUE:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_REMOVE_FULL_QUEUE);
				break;
	        case PARTFILE_REMOVE_HIGH_QUEUE:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_REMOVE_HIGH_QUEUE);
				break;
	        case PARTFILE_CLEANUP_SOURCES:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_CLEANUP_SOURCES);
				break;
	        case PARTFILE_SWAP_A4AF_THIS:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_SWAP_A4AF_THIS);
				break;
        	case PARTFILE_SWAP_A4AF_OTHERS:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_SWAP_A4AF_OTHERS);
				break;
	        case PARTFILE_SWAP_A4AF_THIS_AUTO:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO);
				break;
	        case PARTFILE_PAUSE:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_PAUSE);
				break;
	        case PARTFILE_RESUME:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_RESUME);
				break;
	        case PARTFILE_STOP:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_STOP);
				break;
	        case PARTFILE_PRIO_AUTO:
	        	downloadqueue->AutoPrio((CPartFile *)event.ptr_value, event.long_value);
				break;
	        case PARTFILE_PRIO_SET:
	        	downloadqueue->Prio((CPartFile *)event.ptr_value, event.long_value);
				break;
	        case PARTFILE_SET_CAT:
	        	downloadqueue->Category((CPartFile *)event.ptr_value, event.byte_value);
				break;
	        case PARTFILE_DELETE:
	        	downloadqueue->SendFileCommand((CPartFile *)event.ptr_value, EC_OP_PARTFILE_DELETE);
				break;
			
	        case KNOWNFILE_SET_UP_PRIO:
	        	sharedfiles->SetFilePrio((CKnownFile *)event.ptr_value, event.byte_value);
				break;
	        case KNOWNFILE_SET_UP_PRIO_AUTO:
	        	sharedfiles->SetFilePrio((CKnownFile *)event.ptr_value, PR_AUTO);
				break;
	        case KNOWNFILE_SET_COMMENT:
			break;

			case SHOW_USER_COUNT:
				amuledlg->ShowUserCount(event.string_value);
				break;

			// download queue
	        case DLOAD_SET_CAT_PRIO:
			break;
	        case DLOAD_SET_CAT_STATUS:
			break;

			case ADDLOGLINE:
			case ADDDEBUGLOGLINE:
				printf("LOG: %s\n", (const char*)unicode2char(event.string_value));
				break;
			default:
				printf("ERROR: bad event %d\n", event.ID);
				wxASSERT(0);
	}
}

bool CamuleRemoteGuiApp::IsConnectedED2K() {
	return serverconnect->IsConnected();
}

void CamuleRemoteGuiApp::StartKad() {
	CECPacket req(EC_OP_KAD_START);
	connect->Send(&req);	
}

void CamuleRemoteGuiApp::StopKad() {
	CECPacket req(EC_OP_KAD_STOP);
	connect->Send(&req);	
}

uint32 CamuleRemoteGuiApp::GetED2KID() const {
	return serverconnect ? serverconnect->GetClientID() : 0;
}

/*
 * Preferences: holds both local and remote settings.
 * 
 * First, everything is loaded from local config file. Later, settings
 * that are relevant on remote side only are loaded thru EC
 */
CPreferencesRem::CPreferencesRem(CRemoteConnect *conn)
{
	m_conn = conn;

	CPreferences::BuildItemList( theApp.ConfigDir);
	CPreferences::LoadAllItems( wxConfigBase::Get() );
	
	//
	// Settings queried from remote side
	//
	m_exchange_send_selected_prefs = EC_PREFS_GENERAL | EC_PREFS_CONNECTIONS | EC_PREFS_MESSAGEFILTER |
		EC_PREFS_ONLINESIG | EC_PREFS_SERVERS | EC_PREFS_FILES | EC_PREFS_SRCDROP |
		EC_PREFS_SECURITY | EC_PREFS_CORETWEAKS;
	m_exchange_recv_selected_prefs = m_exchange_send_selected_prefs | EC_PREFS_CATEGORIES;
}

bool CPreferencesRem::LoadRemote()
{
	//
	// override local settings with remote
	CECPacket req(EC_OP_GET_PREFERENCES, EC_DETAIL_UPDATE);

	// bring categories too
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, m_exchange_recv_selected_prefs));
	auto_ptr<CECPacket> prefs(m_conn->SendRecv(&req));
	
	if ( !prefs.get() ) {
		return false;
	}
	((CEC_Prefs_Packet *)prefs.get())->Apply();

	if ( prefs->GetTagByName(EC_TAG_PREFS_CATEGORIES) != 0 ) {
		for (int i = 0; i < prefs->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagCount(); i++) {
			const CECTag *cat_tag = prefs->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagByIndex(i);
			Category_Struct *cat = new Category_Struct;
			cat->title = cat_tag->GetTagByName(EC_TAG_CATEGORY_TITLE)->GetStringData();
			cat->incomingpath = cat_tag->GetTagByName(EC_TAG_CATEGORY_PATH)->GetStringData();
			cat->comment = cat_tag->GetTagByName(EC_TAG_CATEGORY_COMMENT)->GetStringData();
			cat->color =  cat_tag->GetTagByName(EC_TAG_CATEGORY_COLOR)->GetInt32Data();
			cat->prio = cat_tag->GetTagByName(EC_TAG_CATEGORY_PRIO)->GetInt8Data();
			theApp.glob_prefs->AddCat(cat);
		}
	} else {
		Category_Struct *cat = new Category_Struct;
		cat->title = _("All");
		cat->incomingpath = wxEmptyString;
		cat->comment = wxEmptyString;
		cat->color =  0;
		cat->prio = PR_NORMAL;
		theApp.glob_prefs->AddCat(cat);
	}
	
	return true;
}

void CPreferencesRem::SendToRemote()
{
	CEC_Prefs_Packet pref_packet(m_exchange_send_selected_prefs, EC_DETAIL_UPDATE, EC_DETAIL_FULL);
	m_conn->Send(&pref_packet);
}

Category_Struct *CPreferencesRem::CreateCategory(wxString name, wxString path,
			wxString comment, uint32 color, uint8 prio)
{
	CECPacket req(EC_OP_CREATE_CATEGORY);
	CEC_Category_Tag tag(0xffffffff, name, path, comment, color, prio);
	req.AddTag(tag);
	m_conn->Send(&req);

	Category_Struct *category = new Category_Struct();
	category->incomingpath	= path;
	category->title			= name;
	category->comment		= comment;
	category->color			= color;
	category->prio			= prio;
			
	AddCat(category);
	
	return category;
}

void CPreferencesRem::UpdateCategory(uint8 cat, wxString name, wxString path,
			wxString comment, uint32 color, uint8 prio)
{
	CECPacket req(EC_OP_UPDATE_CATEGORY);
	CEC_Category_Tag tag(cat, name, path, comment, color, prio);
	req.AddTag(tag);
	m_conn->Send(&req);

	Category_Struct *category = m_CatList[cat];
	category->incomingpath	= path;
	category->title			= name;
	category->comment		= comment;
	category->color			= color;
	category->prio			= prio;
}

void CPreferencesRem::RemoveCat(uint8 cat)
{
	CECPacket req(EC_OP_DELETE_CATEGORY);
	CEC_Category_Tag tag(cat, EC_DETAIL_CMD);
	req.AddTag(tag);
	m_conn->Send(&req);
	CPreferences::RemoveCat(cat);
}

//
// Container implementation
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

bool CServerConnectRem::ReQuery()
{
	CECPacket stat_req(EC_OP_STAT_REQ);
	CECPacket *stats = m_Conn->SendRecv(&stat_req);
	if ( !stats ) {
		return false;
	}
	CEC_ConnState_Tag *tag = (CEC_ConnState_Tag *)stats->GetTagByName(EC_TAG_CONNSTATE);
    if (!tag) {
            return false;
    }

	int state = 0;
	CServer *server;
	m_ID = tag->GetEd2kId();

	if (tag->IsConnectedED2K()) {
		CECTag *srvtag = tag->GetTagByName(EC_TAG_SERVER);
		if ( !srvtag ) {
			return false;
		}
		server = theApp.serverlist->GetByID(srvtag->GetIPv4Data().IP());
		if ( m_CurrServer && (server != m_CurrServer) ) {
			theApp.amuledlg->serverwnd->serverlistctrl->HighlightServer(m_CurrServer, false);
		}
		theApp.amuledlg->serverwnd->serverlistctrl->HighlightServer(server, true);
		m_CurrServer = server;
		state |= CONNECTED_ED2K;
	} else {
	    	if ( m_CurrServer ) {
	    		theApp.amuledlg->serverwnd->serverlistctrl->HighlightServer(m_CurrServer, false);
	    		m_CurrServer = 0;
	    	}
	}

	if (tag->IsConnectedKademlia()) {
		if (tag->IsKadFirewalled()) {
			state |= CONNECTED_KAD_FIREWALLED;
		} else {
			state |= CONNECTED_KAD_OK;
		}
	}

	theApp.amuledlg->ShowConnectionState(state);
	return true;
}

/*
 * Server list: host list of ed2k servers.
 */
CServerListRem::CServerListRem(CRemoteConnect *conn) : CRemoteContainer<CServer, uint32, CEC_Server_Tag>(conn)
{
}

void CServerListRem::UpdateServerMetFromURL(wxString WXUNUSED(url))
{
	// FIXME: add command
}

void CServerListRem::SaveServerMet()
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

void CServerListRem::UpdateUserFileStatus(CServer *server)
{
	if ( server ) {
		m_TotalUser = server->GetUsers();
		m_TotalFile = server->GetFiles();
		
		wxString buffer = 
			CFormat(_("Total Users: %s | Total Files: %s")) % CastItoIShort(m_TotalUser) % CastItoIShort(m_TotalFile);
	
		Notify_ShowUserCount(buffer);
	}
}

CServer *CServerListRem::GetServerByAddress(const wxString& WXUNUSED(address), uint16 WXUNUSED(port))
{
	// It's ok to return 0 for context where this code is used in remote gui
	return 0;
}

CServer *CServerListRem::CreateItem(CEC_Server_Tag *tag)
{
	return new CServer(tag);
}

void CServerListRem::DeleteItem(CServer *in_srv)
{
	auto_ptr<CServer> srv(in_srv);
	theApp.amuledlg->serverwnd->serverlistctrl->RemoveServer(srv.get());
}

uint32 CServerListRem::GetItemID(CServer *server)
{
	return server->GetIP();
}

void CServerListRem::ProcessItemUpdate(CEC_Server_Tag *, CServer *)
{
	// server list is always realoaded from scratch
	wxASSERT(0);
}

void CServerListRem::ReloadControl()
{
	for(uint32 i = 0;i < GetCount(); i++) {
		CServer *srv = GetByIndex(i);
		theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(srv);
	}
}

void CIPFilterRem::Reload()
{
	CECPacket req(EC_OP_IPFILTER_RELOAD);
	m_conn->Send(&req);
}

void CIPFilterRem::Update(wxString /*url*/)
{
	// FIXME: add command
	wxASSERT(0);
}

/*
 * Shared files list
 */
CSharedFilesRem::CSharedFilesRem(CRemoteConnect *conn) : CRemoteContainer<CKnownFile, CMD4Hash, CEC_SharedFile_Tag>(conn, true)
{
}

void CSharedFilesRem::Reload(bool, bool)
{
	CECPacket req(EC_OP_SHAREDFILES_RELOAD);
	
	m_conn->Send(&req);
}

void CSharedFilesRem::AddFilesFromDirectory(wxString path)
{
	CECPacket req(EC_OP_SHAREDFILES_ADD_DIRECTORY);

	req.AddTag(CECTag(EC_TAG_PREFS_DIRECTORIES, path));
	
	m_conn->Send(&req);
}

CKnownFile *CSharedFilesRem::CreateItem(CEC_SharedFile_Tag *tag)
{
	CKnownFile *file = new CKnownFile(tag);

	m_enc_map[file->GetFileHash()] = RLE_Data(file->GetPartCount(), true);

	ProcessItemUpdate(tag, file);
	
	theApp.amuledlg->sharedfileswnd->sharedfilesctrl->ShowFile(file);
	
	return file;
}

void CSharedFilesRem::DeleteItem(CKnownFile *in_file)
{
	auto_ptr<CKnownFile> file(in_file);

	m_enc_map.erase(file->GetFileHash());
	
	theApp.amuledlg->sharedfileswnd->sharedfilesctrl->RemoveFile(file.get());
}

CMD4Hash CSharedFilesRem::GetItemID(CKnownFile *file)
{
	return file->GetFileHash();
}

void CSharedFilesRem::ProcessItemUpdate(CEC_SharedFile_Tag *tag, CKnownFile *file)
{
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	const unsigned char *data = m_enc_map[file->GetFileHash()].Decode((unsigned char *)parttag->GetTagData(),
		parttag->GetTagDataLen());
	for(int i = 0;i < file->GetPartCount();i++) {
		file->m_AvailPartFrequency[i] = data[i];
	}
	if ( m_inc_tags ) {
		tag->SetRequests(file->statistic.requested);
		tag->SetAllRequests(file->statistic.alltimerequested);
		tag->SetAccepts(file->statistic.accepted);
		tag->SetAllAccepts(file->statistic.alltimeaccepted);
		tag->SetXferred(file->statistic.transfered );
		tag->SetAllXferred(file->statistic.alltimetransferred);
		
		tag->SetPrio(file->m_iUpPriority);
	} else {
		file->statistic.requested = tag->GetRequests();
		file->statistic.alltimerequested = tag->GetAllRequests();
		file->statistic.accepted = tag->GetAccepts();
		file->statistic.alltimeaccepted = tag->GetAllAccepts();
		file->statistic.transfered = tag->GetXferred();
		file->statistic.alltimetransferred = tag->GetAllXferred();
	}
	if ( file->m_iUpPriority >= 10 ) {
		file->m_iUpPriority -= 10;
		file->m_bAutoUpPriority = true;
	} else {
		file->m_bAutoUpPriority = false;
	}

	theApp.knownfiles->requested += file->statistic.requested;
	theApp.knownfiles->transfered += file->statistic.transfered;
	theApp.knownfiles->accepted += file->statistic.transfered;

	theApp.amuledlg->sharedfileswnd->sharedfilesctrl->UpdateItem(file);
}

bool CSharedFilesRem::Phase1Done(CECPacket *)
{
	theApp.knownfiles->requested = 0;
	theApp.knownfiles->transfered = 0;
	theApp.knownfiles->accepted = 0;
	
	return true;
}

void CSharedFilesRem::SetFilePrio(CKnownFile *file, uint8 prio)
{
	CECPacket req(EC_OP_SHARED_SET_PRIO);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, prio));
	
	req.AddTag(hashtag);
	
	m_conn->Send(&req);
}

/*!
 * Connection to remote core
 * 
 */
CRemoteConnect::CRemoteConnect()
{
	m_ECSocket = new ECSocket;
	m_isConnected = false;
	m_busy = false;
}

bool CRemoteConnect::Connect(const wxString &host, int port,
	const wxString &WXUNUSED(login), const wxString &pass)
{
	
	ConnectionPassword = pass;
	
	// don't even try to connect without password
	if (ConnectionPassword.IsEmpty() || ConnectionPassword == wxT("d41d8cd98f00b204e9800998ecf8427e") || CMD4Hash(ConnectionPassword).IsEmpty()) {
		wxMessageBox(_("You must specify a non-empty password."), _("Error"));
		return false;
	}

	wxIPV4address addr;

	addr.Hostname(host);
	addr.Service(port);

	m_ECSocket->Connect(addr, false);
	
	#ifndef CLIENT_GUI
	m_ECSocket->WaitOnConnect(10);

	if (!m_ECSocket->IsConnected()) {
		// no connection => close gracefully
		AddLogLineM(true, _("EC Connection Failed. Unable to connect to the specified host"));
		return false;
	} else {
		return ConnectionEstablished();
	}
	#endif
	
	return true;
}

bool CRemoteConnect::ConnectionEstablished() {
	
	// Authenticate ourselves
	CECPacket packet(EC_OP_AUTH_REQ);
	packet.AddTag(CECTag(EC_TAG_CLIENT_NAME, wxString(wxT("amule-remote"))));
	packet.AddTag(CECTag(EC_TAG_CLIENT_VERSION, wxString(wxT("0x0001"))));
	packet.AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint16)EC_CURRENT_PROTOCOL_VERSION));
	packet.AddTag(CECTag(EC_TAG_PASSWD_HASH, CMD4Hash(ConnectionPassword)));

#ifdef EC_VERSION_ID
	packet.AddTag(CECTag(EC_TAG_VERSION_ID, CMD4Hash(wxT(EC_VERSION_ID))));
#endif

	if (! m_ECSocket->WritePacket(&packet) ) {
		AddLogLineM(true, _("EC Connection Failed. Unable to write data to the socket."));
		return false;
	}
    
	auto_ptr<CECPacket> reply(m_ECSocket->ReadPacket());
	
	if (!reply.get()) {
		AddLogLineM(true, _("EC Connection Failed. Empty reply."));
		return false;
	}
	
	if (reply->GetOpCode() == EC_OP_AUTH_FAIL) {
		const CECTag *reason = reply->GetTagByName(EC_TAG_STRING);
		if (reason != NULL) {
			AddLogLineM(true, CFormat(_("ExternalConn: Access denied because: %s")) % 
				wxGetTranslation(reason->GetStringData()));
		} else {
		    AddLogLineM(true, _("ExternalConn: Access denied"));
		}
		return false;
    } else if (reply->GetOpCode() != EC_OP_AUTH_OK) {
        AddLogLineM(true,_("ExternalConn: Bad reply from server. Connection closed."));
		return false;
    } else {
        m_isConnected = true;
        if (reply->GetTagByName(EC_TAG_SERVER_VERSION)) {
                AddLogLineM(true, CFormat(_("Succeeded! Connection established to aMule %s")) %
                	reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData());
        } else {
                AddLogLineM(true, _("Succeeded! Connection established."));
        }
    }
    m_isConnected = true;
    m_ECSocket->SetFlags(wxSOCKET_BLOCK);
    
	return true;	
}


CECPacket *CRemoteConnect::SendRecv(CECPacket *packet)
{
	m_busy = true;
    if (! m_ECSocket->WritePacket(packet) ) {
		m_busy = false;
    	return 0;
    }
    CECPacket *reply = m_ECSocket->ReadPacket();

	m_busy = false;
	return reply;
}

void CRemoteConnect::Send(CECPacket *packet)
{
	m_busy = true;
    if (! m_ECSocket->WritePacket(packet) ) {
		m_busy = false;
    	return;
    }
    auto_ptr<CECPacket> reply(m_ECSocket->ReadPacket());

	m_busy = false;
}

/*
 * List of uploading and waiting clients.
 */
CUpDownClientListRem::CUpDownClientListRem(CRemoteConnect *conn, int viewtype) : CRemoteContainer<CUpDownClient, uint32, CEC_UpDownClient_Tag>(conn)
{
	m_viewtype = viewtype;
}

POSITION CUpDownClientListRem::GetFirstFromList()
{
	it = m_items.begin();
	if ( it == m_items.end() ) {
		return 0;
	}
	POSITION pos;
	pos.m_ptr = (void *)&it;
	return pos;
}

CUpDownClient *CUpDownClientListRem::GetNextFromList(POSITION &pos)
{
	std::list<CUpDownClient *>::iterator *i = (std::list<CUpDownClient *>::iterator *)pos.m_ptr;
	CUpDownClient *client = *(*i);
	(*i)++;
	if ( *i == m_items.end() ) {
		pos = 0;
	}
	return client;
}

CUpDownClient::CUpDownClient(CEC_UpDownClient_Tag *tag)
{
	m_nUserIDHybrid = tag->ID();
	m_Username = tag->ClientName();
	m_clientSoft = tag->ClientSoftware();
	m_UserHash = tag->UserID();
	
	m_Friend = 0;
	
	if ( tag->HaveFile() ) {
		CMD4Hash filehash = tag->FileID();
		m_uploadingfile = theApp.sharedfiles->GetByID(filehash);
		if ( !m_uploadingfile ) {
			m_uploadingfile = theApp.downloadqueue->GetByID(filehash);
		}
	} else {
		m_uploadingfile = NULL;
	}

	m_nCurSessionUp = 0;

	credits = new CClientCredits(new CreditStruct());
}

/* Warning: do common base */

bool CUpDownClient::IsIdentified() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED);
}

bool CUpDownClient::IsBadGuy() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY);
}

bool CUpDownClient::SUIFailed() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDFAILED);
}

bool CUpDownClient::SUINeeded() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDNEEDED);
}

bool CUpDownClient::SUINotSupported() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_NOTAVAILABLE);
}

uint64 CUpDownClient::GetDownloadedTotal() const 
{
	return credits ? credits->GetDownloadedTotal() : 0;
}
	
uint64 CUpDownClient::GetUploadedTotal() const 
{
	return credits ? credits->GetUploadedTotal() : 0;
}
	
float CUpDownClient::GetScoreRatio() const {
	return credits ? credits->GetScoreRatio(GetIP()) : 0;
}
/* End Warning */

CUpDownClient::~CUpDownClient()
{
	if ( credits ) {
		delete credits;
	}
}

CUpDownClient *CUpDownClientListRem::CreateItem(CEC_UpDownClient_Tag *tag)
{
	CUpDownClient *client = new CUpDownClient(tag);
	ProcessItemUpdate(tag, client);
	
	theApp.amuledlg->transferwnd->clientlistctrl->InsertClient(client, (ViewType)m_viewtype);
	
	return client;
}

void CUpDownClientListRem::DeleteItem(CUpDownClient *client)
{
	theApp.amuledlg->transferwnd->clientlistctrl->RemoveClient(client, (ViewType)m_viewtype);
	delete client;
}

uint32 CUpDownClientListRem::GetItemID(CUpDownClient *client)
{
	return client->GetUserIDHybrid();
}

void CUpDownClientListRem::ProcessItemUpdate(CEC_UpDownClient_Tag *tag, CUpDownClient *client)
{
	uint16 state = tag->ClientState();
	
	client->m_nDownloadState = state & 0xff;
	client->m_nUploadState = (state >> 8) & 0xff;
	
	client->m_nUpDatarate = tag->SpeedUp();
	if ( client->m_nDownloadState == DS_DOWNLOADING ) {
		client->kBpsDown = tag->SpeedDown() / 1024.0;
	} else {
		client->kBpsDown = 0;
	}

	client->m_WaitTime = tag->WaitTime();
	client->m_UpStartTimeDelay = tag->XferTime();
	client->m_dwLastUpRequest = tag->LastReqTime();
	client->m_WaitStartTime = tag->QueueTime();
	
	CreditStruct *credit_struct = (CreditStruct *)client->credits->GetDataStruct();
	uint64 value = tag->XferUp();
	credit_struct->nUploadedHi = value >> 32;
	credit_struct->nUploadedLo = value & 0xffffffff;
	client->m_nTransferredUp = tag->XferUpSession();

	value = tag->XferDown();
	credit_struct->nDownloadedHi = value >> 32;
	credit_struct->nDownloadedLo = value & 0xffffffff;
}

CUpQueueRem::CUpQueueRem(CRemoteConnect *conn) : m_up_list(conn, vtUploading), m_wait_list(conn, vtQueued)
{
}

/*
 * Download queue container: hold PartFiles with progress status
 * 
 */
 
CDownQueueRem::CDownQueueRem(CRemoteConnect *conn) : CRemoteContainer<CPartFile, CMD4Hash, CEC_PartFile_Tag>(conn, true)
{
}

bool CDownQueueRem::AddED2KLink(const wxString &link, int)
{
	CECPacket req(EC_OP_ED2K_LINK);
	req.AddTag(CECTag(EC_TAG_STRING, link));
	
	m_conn->Send(&req);
	return true;
}

void CDownQueueRem::StopUDPRequests()
{
	// have no idea what is it about
}

void CDownQueueRem::ResetCatParts(int)
{
	// called when category being deleted. Command will be performed on remote side
}

bool CDownQueueRem::IsPartFile(const CKnownFile *) const
{
	// hope i understand it right
	return true;
}

CPartFile *CDownQueueRem::CreateItem(CEC_PartFile_Tag *tag)
{
	CPartFile *file = new CPartFile(tag);
	m_enc_map[file->GetFileHash()] = PartFileEncoderData(file->GetPartCount(), 10);
	ProcessItemUpdate(tag, file);
	
	theApp.amuledlg->transferwnd->downloadlistctrl->AddFile(file);
	return file;
}

void CDownQueueRem::DeleteItem(CPartFile *in_file)
{
	auto_ptr<CPartFile> file(in_file);

	theApp.amuledlg->transferwnd->downloadlistctrl->RemoveFile(file.get());
	
	m_enc_map.erase(file->GetFileHash());
}

CMD4Hash CDownQueueRem::GetItemID(CPartFile *file)
{
	return file->GetFileHash();
}

void CDownQueueRem::ProcessItemUpdate(CEC_PartFile_Tag *tag, CPartFile *file)
{
	//
	// update status
	//
	if ( m_inc_tags ) {
		uint32 tmpval = (uint32)(file->kBpsDown * 1024);
		tag->SetSpeed(tmpval);
		file->kBpsDown = tmpval / 1024.0;
		
		tag->SetSizeXfer(file->transfered);
		tag->SetSizeDone(file->completedsize);
		tag->SetSourceXferCount(file->transferingsrc);
		tag->SetSourceNotCurrCount(file->m_notCurrentSources);
		tag->SetSourceCount(file->m_source_count);
		tag->SetSourceCountA4AF(file->m_a4af_source_count);
	    tag->SetFileStatus(file->status);
	
		tag->SetLastSeenComplete(tmpval);
		file->lastseencomplete = tmpval;
		
		tag->SetFileCat(file->m_category);
		
		tag->SetPrio(file->m_iDownPriority);
	} else {
		file->kBpsDown = tag->Speed() / 1024.0;
	
		if ( file->kBpsDown > 0 ) {
			file->transfered = tag->SizeXfer();
			file->completedsize = tag->SizeDone();
		}
	
		file->transferingsrc = tag->SourceXferCount();
		file->m_notCurrentSources = tag->SourceNotCurrCount();
		file->m_source_count = tag->SourceCount();
		file->m_a4af_source_count = tag->SourceCountA4AF();
	    file->status = tag->FileStatus();
	
		file->lastseencomplete = tag->LastSeenComplete();
		
		file->m_category = tag->FileCat();
		
		file->m_iDownPriority = tag->Prio();
	}
	file->percentcompleted = (100.0*file->completedsize) / file->m_nFileSize;
	if ( file->m_iDownPriority >= 10 ) {
		file->m_iDownPriority -= 10;
		file->m_bAutoUpPriority = true;
	} else {
		file->m_bAutoUpPriority = false;
	}

	//
	// Copy part/gap status
	//
	CECTag *gaptag = tag->GetTagByName(EC_TAG_PARTFILE_GAP_STATUS);
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	CECTag *reqtag = tag->GetTagByName(EC_TAG_PARTFILE_REQ_STATUS);
	if (gaptag && parttag && reqtag) {
		POSITION curr_pos;

		wxASSERT(m_enc_map.count(file->GetFileHash()));
		
		PartFileEncoderData &encoder = m_enc_map[file->GetFileHash()];
		encoder.Decode(
			(unsigned char *)gaptag->GetTagData(), gaptag->GetTagDataLen(),
			(unsigned char *)parttag->GetTagData(), parttag->GetTagDataLen());
			
		const Gap_Struct *reqparts = (const Gap_Struct *)reqtag->GetTagData();
		int reqcount = reqtag->GetTagDataLen() / sizeof(Gap_Struct);
		
		// adjust size of gaplist to reqcount
		int gap_size = encoder.m_gap_status.Size() / (2 * sizeof(uint32));
		while ( file->gaplist.GetCount() > gap_size ) {
			file->gaplist.RemoveHead();
		}
		while ( file->gaplist.GetCount() != gap_size ) {
			file->gaplist.AddHead(new Gap_Struct);
		}
		const uint32 *gap_info = (const uint32 *)encoder.m_gap_status.Buffer();
		curr_pos = file->gaplist.GetHeadPosition();
		for (int j = 0; j < gap_size;j++) {
			Gap_Struct* gap = file->gaplist.GetNext(curr_pos);
			gap->start = ENDIAN_NTOHL(gap_info[2*j]);
			gap->end = ENDIAN_NTOHL(gap_info[2*j+1]);
		}
		
		// adjust size of requested block list
		while ( file->requestedblocks_list.GetCount() > reqcount ) {
			file->requestedblocks_list.RemoveHead();
		}
		while ( file->requestedblocks_list.GetCount() != reqcount ) {
			file->requestedblocks_list.AddHead(new Requested_Block_Struct);
		}
		curr_pos = file->requestedblocks_list.GetHeadPosition();
		for (int i = 0; i < reqcount;i++) {
			Requested_Block_Struct* block = file->requestedblocks_list.GetNext(curr_pos);
			block->StartOffset = ENDIAN_NTOHL(reqparts[i].start);
			block->EndOffset = ENDIAN_NTOHL(reqparts[i].end);
		}
		// copy parts frequency
		const unsigned char *part_info = encoder.m_part_status.Buffer();
		for(int i = 0;i < file->GetPartCount();i++) {
			file->m_SrcpartFrequency[i] = part_info[i];
		}
	} else {
		printf("ERROR: %p %p %p\n", (void*)gaptag, (void*)parttag, (void*)reqtag);
	}
	theApp.amuledlg->transferwnd->downloadlistctrl->UpdateItem(file);
}

bool CDownQueueRem::Phase1Done(CECPacket *)
{
	return true;
}

void CDownQueueRem::SendFileCommand(CPartFile *file, ec_tagname_t cmd)
{
	CECPacket req(cmd);
	req.AddTag(CECTag(EC_TAG_PARTFILE, file->GetFileHash()));
	
	m_conn->Send(&req);
}

void CDownQueueRem::Prio(CPartFile *file, uint8 prio)
{
	CECPacket req(EC_OP_PARTFILE_PRIO_SET);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, prio));
	
	m_conn->Send(&req);
}

void CDownQueueRem::AutoPrio(CPartFile *file, bool flag)
{
	CECPacket req(EC_OP_PARTFILE_PRIO_SET);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)(flag ? PR_AUTO : file->GetDownPriority())));
	
	m_conn->Send(&req);
}

void CDownQueueRem::Category(CPartFile *file, uint8 cat)
{
	CECPacket req(EC_OP_PARTFILE_SET_CAT);
	file->m_category = cat;
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	req.AddTag(hashtag);
	
	m_conn->Send(&req);
}

void CDownQueueRem::AddSearchToDownload(CSearchFile* file, uint8 category)
{
	CECPacket req(EC_OP_DOWNLOAD_SEARCH_RESULT);
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, category));
	req.AddTag(hashtag);
	
	m_conn->Send(&req);
}

CClientListRem::CClientListRem(CRemoteConnect *conn)
{
	m_conn = conn;
}

void CClientListRem::FilterQueues()
{
	// FIXME: add code
	//wxASSERT(0);
}

CSearchListRem::CSearchListRem(CRemoteConnect *conn) : CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag>(conn)
{
	m_curr_search = -1;
}

bool CSearchListRem::StartNewSearch(uint32* nSearchID, bool global_search, wxString &searchString,
	wxString& typeText, wxString &extension, uint32 min_size, uint32 max_size, uint32 availability)
{
	CECPacket search_req(EC_OP_SEARCH_START);
	search_req.AddTag(CEC_Search_Tag (searchString,
		global_search ? EC_SEARCH_GLOBAL : EC_SEARCH_LOCAL, typeText,
		extension, availability, min_size, max_size));
		
	m_conn->Send(&search_req);
	m_curr_search = *(nSearchID); // No kad remote search yet.
	
	Flush();
	
	return true;
}

void CSearchListRem::StopGlobalSearch()
{
	if (m_curr_search != -1) {
		CECPacket search_req(EC_OP_SEARCH_STOP);
		m_conn->Send(&search_req);
	}
}

CSearchFile::CSearchFile(CEC_SearchFile_Tag *tag)
{
	m_strFileName = tag->FileName();
	m_abyFileHash = tag->ID();
	m_nFileSize = tag->SizeFull();
	
	m_nSearchID = theApp.searchlist->m_curr_search;

}

// dtor is virtual - must be implemented
CSearchFile::~CSearchFile()
{	
}

CSearchFile *CSearchListRem::CreateItem(CEC_SearchFile_Tag *tag)
{
	CSearchFile *file = new CSearchFile(tag);
	ProcessItemUpdate(tag, file);
	
	theApp.amuledlg->searchwnd->AddResult(file);
	
	return file;
}

void CSearchListRem::DeleteItem(CSearchFile *file)
{
	delete file;
}

CMD4Hash CSearchListRem::GetItemID(CSearchFile *file)
{
	return file->GetFileHash();
}

void CSearchListRem::ProcessItemUpdate(CEC_SearchFile_Tag *tag, CSearchFile *file)
{
	file->m_SourceCount = tag->SourceCount();
	file->m_CompleteSourceCount = tag->CompleteSourceCount();
}

bool CSearchListRem::Phase1Done(CECPacket *reply)
{
	switch (reply->GetOpCode()) {
		case EC_OP_SEARCH_RESULTS:
			break;
		case EC_OP_SEARCH_RESULTS_DONE:
			if ( reply->GetTagCount() <= GetCount() ) {
				m_curr_search = -1;
				return false;
			}
			break;
		default:
			wxASSERT(0);
	}
	return true;
}

void CSearchListRem::RemoveResults(long nSearchID)
{
	std::map<long, std::vector<CSearchFile *> >::iterator it = m_Results.find(nSearchID);
	
	if ( it != m_Results.end() ) {
        std::vector<CSearchFile *> &list = it->second;

        for (unsigned int i = 0; i < list.size(); i++) {
			delete list[i];
		}
        m_Results.erase(it);
	}
}

bool CUpDownClient::IsBanned() const
{
	// FIXME: add code
	return false;
}

//
// Those functions have different implementation in remote gui
//
void  CUpDownClient::Ban()
{
	// FIXME: add code
	wxASSERT(0);
}

void  CUpDownClient::UnBan()
{
	// FIXME: add code
	wxASSERT(0);
}

void CUpDownClient::RequestSharedFileList()
{
	// FIXME: add code
	wxASSERT(0);
}

void CKnownFile::SetFileComment(const wxString &)
{
	// FIXME: add code
	wxASSERT(0);
}

void CKnownFile::SetFileRating(unsigned char)
{
	// FIXME: add code
	wxASSERT(0);
}

// I don't think it will be implemented - too match data transfer. But who knows ?
wxString CUpDownClient::ShowDownloadingParts() const
{
	return wxEmptyString;
}
bool CUpDownClient::SwapToAnotherFile(bool WXUNUSED(bIgnoreNoNeeded), bool WXUNUSED(ignoreSuspensions),
										bool WXUNUSED(bRemoveCompletely), CPartFile* WXUNUSED(toFile))
{
	// FIXME: add code
	wxASSERT(0);
	return false;
}
//
// Those functions are virtual. So even they don't get called they must
// be defined so linker will be happy
//
CPacket* CKnownFile::CreateSrcInfoPacket(CUpDownClient const*)
{
	wxASSERT(0);
	return 0;
}

bool CKnownFile::LoadFromFile(class CFileDataIO const*)
{
	wxASSERT(0);
	return false;
}

void CKnownFile::UpdatePartsInfo()
{
	wxASSERT(0);
}

void CKnownFile::SetFileSize(uint32)
{
	wxASSERT(0);
}

CPacket* CPartFile::CreateSrcInfoPacket(CUpDownClient const*)
{
	wxASSERT(0);
	return 0;
}

void CPartFile::UpdatePartsInfo()
{
	wxASSERT(0);
}

//
// Comments should be already here when dialog pops-up
void CPartFile::UpdateFileRatingCommentAvail()
{
}

bool CPartFile::SavePartFile(bool)
{
	wxASSERT(0);
	return false;
}

//
// since gui is not linked with amule.cpp - define events here
//
DEFINE_LOCAL_EVENT_TYPE(wxEVT_MULE_NOTIFY_EVENT)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_AMULE_TIMER)

DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
