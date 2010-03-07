//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#include <wx/ipc.h>
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>		// Needed for wxFileConfig


#include <common/Format.h>
#include <common/StringFunctions.h>
#include <common/MD5Sum.h>


#include <include/common/EventIDs.h>


#include "amule.h"			// Interface declarations.
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "ClientCredits.h"
#include "ClientListCtrl.h"
#include "DataToText.h"			// Needed for GetSoftName()
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "GuiEvents.h"
#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"		// Needed for IP2Country
#endif
#include "InternalEvents.h"	// Needed for wxEVT_CORE_FINISHED_HTTP_DOWNLOAD
#include "Logger.h"
#include "muuli_wdr.h"			// Needed for IDs
#include "PartFile.h"			// Needed for CPartFile
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "Server.h"			// Needed for GetListName
#include "ServerWnd.h"			// Needed for CServerWnd
#include "SharedFilesCtrl.h"		// Needed for CSharedFilesCtrl
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "updownclient.h"
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
#include "ScopedPtr.h"


CEConnectDlg::CEConnectDlg()
:
wxDialog(theApp->amuledlg, -1, _("Connect to remote amule"), wxDefaultPosition)
{
	CoreConnect(this, true);
	
	wxString pref_host, pref_port;
	wxConfig::Get()->Read(wxT("/EC/Host"), &pref_host, wxT("localhost"));
	wxConfig::Get()->Read(wxT("/EC/Port"), &pref_port, wxT("4712"));
	wxConfig::Get()->Read(wxT("/EC/Password"), &pwd_hash);
	
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
	port = StrToLong(s_port);
	
	host = CastChild(ID_REMOTE_HOST, wxTextCtrl)->GetValue();
	passwd = CastChild(ID_EC_PASSWD, wxTextCtrl)->GetValue();

	if (passwd != pwd_hash) {
		pwd_hash = MD5Sum(passwd).GetHash();
	}
	m_save_user_pass = CastChild(ID_EC_SAVE, wxCheckBox)->IsChecked();
	evt.Skip();
}


DEFINE_LOCAL_EVENT_TYPE(wxEVT_EC_INIT_DONE)


BEGIN_EVENT_TABLE(CamuleRemoteGuiApp, wxApp)
	// Core timer
	EVT_TIMER(ID_CORE_TIMER_EVENT, CamuleRemoteGuiApp::OnPollTimer)

	EVT_CUSTOM(wxEVT_EC_CONNECTION, -1, CamuleRemoteGuiApp::OnECConnection)
	EVT_CUSTOM(wxEVT_EC_INIT_DONE, -1, CamuleRemoteGuiApp::OnECInitDone)
	
	EVT_MULE_NOTIFY(CamuleRemoteGuiApp::OnNotifyEvent)

#ifdef ENABLE_IP2COUNTRY
	// HTTPDownload finished
	EVT_MULE_INTERNAL(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleRemoteGuiApp::OnFinishedHTTPDownload)
#endif
END_EVENT_TABLE()


IMPLEMENT_APP(CamuleRemoteGuiApp)


int CamuleRemoteGuiApp::OnExit()
{
	StopTickTimer();
	
	return wxApp::OnExit();
}


void CamuleRemoteGuiApp::OnPollTimer(wxTimerEvent&)
{
	static int request_step = 0;
	
	if (m_connect->RequestFifoFull()) {
		return;
	}
	
	switch (request_step) {
	case 0:
		serverconnect->ReQuery();
		serverlist->UpdateUserFileStatus(serverconnect->GetCurrentServer());
		request_step++;
		break;
	case 1: {
		CECPacket stats_req(EC_OP_STAT_REQ);
		m_connect->SendRequest(&m_stats_updater, &stats_req);
		amuledlg->ShowTransferRate();			
		request_step++;
		break;
	}
	case 2:
		if (amuledlg->m_sharedfileswnd->IsShown()) {
			sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);
		} else if (amuledlg->m_serverwnd->IsShown()) {
			//serverlist->FullReload(EC_OP_GET_SERVER_LIST);
		} else if (amuledlg->m_transferwnd->IsShown()) {
			downloadqueue->DoRequery(EC_OP_GET_DLOAD_QUEUE,	EC_TAG_PARTFILE);
			switch(amuledlg->m_transferwnd->clientlistctrl->GetListView()) {
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
			amuledlg->m_transferwnd->ShowQueueCount(theStats::GetWaitingUserCount());
		} else if (amuledlg->m_searchwnd->IsShown()) {
			if (searchlist->m_curr_search != -1) {
				searchlist->DoRequery(EC_OP_SEARCH_RESULTS, EC_TAG_SEARCHFILE);
			}
		}
		// Back to the roots
		request_step = 0;
		break;
	default:
		AddLogLineCS(wxT("WTF?")); // should not happen. :-)
		request_step = 0;
	}

	// Check for new links once per second.
	static uint32 lastED2KLinkCheck = 0;
	if (GetTickCount() - lastED2KLinkCheck >= 1000) {
		AddLinksFromFile();
		lastED2KLinkCheck = GetTickCount();
	}
}


void CamuleRemoteGuiApp::OnFinishedHTTPDownload(CMuleInternalEvent& event)
{
#ifdef ENABLE_IP2COUNTRY
	if (event.GetInt() == HTTP_GeoIP) {
		amuledlg->IP2CountryDownloadFinished(event.GetExtraLong());
		// If we updated, the dialog is already up. Redraw it to show the flags.
		amuledlg->Refresh();
	}
#endif	
}


void CamuleRemoteGuiApp::ShutDown(wxCloseEvent &WXUNUSED(evt))
{
	// Stop the Core Timer
	delete poll_timer;
	poll_timer = NULL;

	// Destroy the EC socket
	m_connect->Destroy();
	m_connect = NULL;

	//
	if (amuledlg) {
		amuledlg->DlgShutDown();
		amuledlg->Destroy();
		amuledlg = NULL;
	}
}


bool CamuleRemoteGuiApp::OnInit()
{
	StartTickTimer();
	amuledlg = NULL;
	
	// Get theApp
	theApp = &wxGetApp();

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	// Parse cmdline arguments.
	if (!InitCommon(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv)) {
		return false;
	}

	// Create the polling timer
	poll_timer = new wxTimer(this,ID_CORE_TIMER_EVENT);
	if (!poll_timer) {
		AddLogLineCS(_("Fatal Error: Failed to create Poll Timer"));
		OnExit();
	}

	m_connect = new CRemoteConnect(this);
	
	glob_prefs = new CPreferencesRem(m_connect);	
	
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));

	if (ShowConnectionDialog()) {
		AddLogLineNS(_("Going to event loop..."));
		return true;
	}
	
	return false;
}


bool CamuleRemoteGuiApp::CryptoAvailable() const
{
	return thePrefs::IsSecureIdentEnabled(); // good enough
}


bool CamuleRemoteGuiApp::ShowConnectionDialog()
{
	dialog = new CEConnectDlg;

	if (m_skipConnectionDialog) {
		wxCommandEvent evt;
		dialog->OnOK(evt);
	} else if (dialog->ShowModal() != wxID_OK) {
		dialog->Destroy();
		
		return false;
	}
	AddLogLineNS(_("Connecting..."));
	if (!m_connect->ConnectToCore(dialog->Host(), dialog->Port(),
		dialog->Login(), dialog->PassHash(),
		wxT("amule-remote"), wxT("0x0001"))) {
		wxMessageBox(_("Connection failed "),_("ERROR"),wxOK);
		
		return false;
	}

	return true;	
}


void CamuleRemoteGuiApp::OnECConnection(wxEvent& event) {
	wxECSocketEvent& evt = *((wxECSocketEvent*)&event);
	AddLogLineNS(_("Remote GUI EC event handler"));
	wxString reply = evt.GetServerReply();
	AddLogLineM(true, reply);
	if (evt.GetResult() == true) {
		// Connected - go to next init step
		glob_prefs->LoadRemote();
	} else {
		AddLogLineNS(_("Going down"));
		if (dialog) {	// connect failed
			wxMessageBox(
			(CFormat(_("Connection Failed. Unable to connect to %s:%d\n")) % dialog->Host() % dialog->Port()) + reply,
			_("ERROR"), wxOK);
		} else {		// server disconnected (probably terminated) later
			wxMessageBox(_("Connection closed - aMule has terminated probably."), _("ERROR"), wxOK);
		}
		ExitMainLoop();
	}
}


void CamuleRemoteGuiApp::OnECInitDone(wxEvent& )
{
	Startup();
}


void CamuleRemoteGuiApp::OnNotifyEvent(CMuleGUIEvent& evt)
{
	evt.Notify();
}


void CamuleRemoteGuiApp::Startup() {
	
	if (dialog->SaveUserPass()) {
		wxConfig::Get()->Write(wxT("/EC/Host"), dialog->Host());
		wxConfig::Get()->Write(wxT("/EC/Port"), dialog->Port());
		wxConfig::Get()->Write(wxT("/EC/Password"), dialog->PassHash());
	}
	dialog->Destroy();
	dialog = NULL;
	
	m_ConnState = 0;
	m_clientID  = 0;

	serverconnect = new CServerConnectRem(m_connect);
	m_statistics = new CStatistics(*m_connect);
	
	clientlist = new CClientListRem(m_connect);
	searchlist = new CSearchListRem(m_connect);
	serverlist = new CServerListRem(m_connect);
	
	sharedfiles	= new CSharedFilesRem(m_connect);
	knownfiles = new CKnownFilesRem(sharedfiles);

	// bugfix - do this before creating the uploadqueue
	downloadqueue = new CDownQueueRem(m_connect);
	uploadqueue = new CUpQueueRem(m_connect);
	ipfilter = new CIPFilterRem(m_connect);

	// Create main dialog
	InitGui(m_geometryEnabled, m_geometryString);

	// Forward wxLog events to CLogger
	wxLog::SetActiveTarget(new CLoggerTarget);
	serverlist->FullReload(EC_OP_GET_SERVER_LIST);
	sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);

	// Start the Poll Timer
	poll_timer->Start(1000);	
	amuledlg->StartGuiTimer();

	// Now activate GeoIP, so that the download dialog doesn't get destroyed immediately 
#ifdef ENABLE_IP2COUNTRY
	if (thePrefs::IsGeoIPEnabled()) {
		amuledlg->m_IP2Country->Enable();
	}
#endif
}


int CamuleRemoteGuiApp::ShowAlert(wxString msg, wxString title, int flags)
{
	return CamuleGuiBase::ShowAlert(msg, title, flags);
}


void CamuleRemoteGuiApp::AddRemoteLogLine(const wxString& line)
{
	amuledlg->AddLogLine(line);
}

int CamuleRemoteGuiApp::InitGui(bool geometry_enabled, wxString &geom_string)
{
	CamuleGuiBase::InitGui(geometry_enabled, geom_string);
	SetTopWindow(amuledlg);
	AddLogLineN(_("Ready")); // The first log line after the window is up triggers output of all the ones before
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


bool CamuleRemoteGuiApp::AddServer(CServer *, bool)
{
	// #warning TODO: Add remote command
	return true;
}


bool CamuleRemoteGuiApp::IsFirewalled() const
{
	if (IsConnectedED2K() && !serverconnect->IsLowID()) {
		return false;
	}

	return IsFirewalledKad();
}


bool CamuleRemoteGuiApp::IsConnectedED2K() const {
	return serverconnect && serverconnect->IsConnected();
}


void CamuleRemoteGuiApp::StartKad() {
	m_connect->StartKad();
}


void CamuleRemoteGuiApp::StopKad() {
	m_connect->StopKad();
}


void CamuleRemoteGuiApp::BootstrapKad(uint32 ip, uint16 port)
{
	CECPacket req(EC_OP_KAD_BOOTSTRAP_FROM_IP);
	req.AddTag(CECTag(EC_TAG_BOOTSTRAP_IP, ip));
	req.AddTag(CECTag(EC_TAG_BOOTSTRAP_PORT, port));
	
	m_connect->SendPacket(&req);
}


void CamuleRemoteGuiApp::UpdateNotesDat(const wxString& url)
{
	CECPacket req(EC_OP_KAD_UPDATE_FROM_URL);
	req.AddTag(CECTag(EC_TAG_KADEMLIA_UPDATE_URL, url));
	
	m_connect->SendPacket(&req);
}


void CamuleRemoteGuiApp::DisconnectED2K() {
	if (IsConnectedED2K()) {
		m_connect->DisconnectED2K();
	}
}


uint32 CamuleRemoteGuiApp::GetED2KID() const
{
	return serverconnect ? serverconnect->GetClientID() : 0;
}


uint32 CamuleRemoteGuiApp::GetID() const
{
	return m_clientID;
}


void CamuleRemoteGuiApp::ShowUserCount() {
	wxString buffer;
	
	static const wxString s_singlenetstatusformat = _("Users: %s | Files: %s");
	static const wxString s_bothnetstatusformat = _("Users: E: %s K: %s | Files: E: %s K: %s");
	
	if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_bothnetstatusformat) % CastItoIShort(theStats::GetED2KUsers()) % CastItoIShort(theStats::GetKadUsers()) % CastItoIShort(theStats::GetED2KFiles()) % CastItoIShort(theStats::GetKadFiles());
	} else if (thePrefs::GetNetworkED2K()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(theStats::GetED2KUsers()) % CastItoIShort(theStats::GetED2KFiles());
	} else if (thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(theStats::GetKadUsers()) % CastItoIShort(theStats::GetKadFiles());
	} else {
		buffer = _("No networks selected");
	}

	Notify_ShowUserCount(buffer);
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

	//
	// Settings queried from remote side
	//
	m_exchange_send_selected_prefs =
		EC_PREFS_GENERAL |
		EC_PREFS_CONNECTIONS |
		EC_PREFS_MESSAGEFILTER |
		EC_PREFS_ONLINESIG |
		EC_PREFS_SERVERS |
		EC_PREFS_FILES |
		EC_PREFS_SRCDROP |
		EC_PREFS_DIRECTORIES |
		EC_PREFS_SECURITY |
		EC_PREFS_CORETWEAKS |
		EC_PREFS_REMOTECONTROLS |
		EC_PREFS_KADEMLIA;
	m_exchange_recv_selected_prefs =
		m_exchange_send_selected_prefs |
		EC_PREFS_CATEGORIES;
}


void CPreferencesRem::HandlePacket(const CECPacket *packet)
{
	((CEC_Prefs_Packet *)packet)->Apply();

	if ( packet->GetTagByName(EC_TAG_PREFS_CATEGORIES) != 0 ) {
		for (size_t i = 0; i < packet->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagCount(); i++) {
			const CECTag *cat_tag = packet->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagByIndex(i);
			Category_Struct *cat = new Category_Struct;
			cat->title = cat_tag->GetTagByName(EC_TAG_CATEGORY_TITLE)->GetStringData();
			cat->path = CPath(cat_tag->GetTagByName(EC_TAG_CATEGORY_PATH)->GetStringData());
			cat->comment = cat_tag->GetTagByName(EC_TAG_CATEGORY_COMMENT)->GetStringData();
			cat->color =  cat_tag->GetTagByName(EC_TAG_CATEGORY_COLOR)->GetInt();
			cat->prio = cat_tag->GetTagByName(EC_TAG_CATEGORY_PRIO)->GetInt();
			theApp->glob_prefs->AddCat(cat);
		}
	} else {
		Category_Struct *cat = new Category_Struct;
		cat->title = _("All");
		cat->color =  0;
		cat->prio = PR_NORMAL;
		theApp->glob_prefs->AddCat(cat);
	}
	wxECInitDoneEvent event;
	theApp->AddPendingEvent(event);
	
}


bool CPreferencesRem::LoadRemote()
{
	//
	// override local settings with remote
	CECPacket req(EC_OP_GET_PREFERENCES, EC_DETAIL_UPDATE);

	// bring categories too
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, m_exchange_recv_selected_prefs));
	
	m_conn->SendRequest(this, &req);
	
	return true;
}


void CPreferencesRem::SendToRemote()
{
	CEC_Prefs_Packet pref_packet(m_exchange_send_selected_prefs, EC_DETAIL_UPDATE, EC_DETAIL_FULL);
	m_conn->SendPacket(&pref_packet);
}


class CCatHandler : public CECPacketHandlerBase {
	virtual void HandlePacket(const CECPacket *packet);
};


void CCatHandler::HandlePacket(const CECPacket *packet)
{
	if (packet->GetOpCode() == EC_OP_FAILED) {
		const CECTag * catTag = packet->GetTagByName(EC_TAG_CATEGORY);
		const CECTag * pathTag = packet->GetTagByName(EC_TAG_CATEGORY_PATH);
		if (catTag && pathTag && catTag->GetInt() < theApp->glob_prefs->GetCatCount()) {
			int cat = catTag->GetInt();
			Category_Struct* cs = theApp->glob_prefs->GetCategory(cat);
			wxMessageBox(CFormat(_("Can't create directory '%s' for category '%s', keeping directory '%s'."))
				% cs->path.GetPrintable() % cs->title % pathTag->GetStringData(), 
				_("ERROR"), wxOK);
			cs->path = CPath(pathTag->GetStringData());
			theApp->amuledlg->m_transferwnd->UpdateCategory(cat);
			theApp->amuledlg->m_transferwnd->downloadlistctrl->Refresh();
		}
	}
	delete this;
}


bool CPreferencesRem::CreateCategory(
	Category_Struct *& category,
	const wxString& name,
	const CPath& path,
	const wxString& comment,
	uint32 color,
	uint8 prio)
{
	CECPacket req(EC_OP_CREATE_CATEGORY);
	CEC_Category_Tag tag(0xffffffff, name, path.GetRaw(), comment, color, prio);
	req.AddTag(tag);
	m_conn->SendRequest(new CCatHandler, &req);

	category = new Category_Struct();
	category->path		= path;
	category->title		= name;
	category->comment	= comment;
	category->color		= color;
	category->prio		= prio;
			
	AddCat(category);
	
	return true;
}


bool CPreferencesRem::UpdateCategory(
	uint8 cat,
	const wxString& name,
	const CPath& path,
	const wxString& comment,
	uint32 color,
	uint8 prio)
{
	CECPacket req(EC_OP_UPDATE_CATEGORY);
	CEC_Category_Tag tag(cat, name, path.GetRaw(), comment, color, prio);
	req.AddTag(tag);
	m_conn->SendRequest(new CCatHandler, &req);

	Category_Struct *category = m_CatList[cat];
	category->path		= path;
	category->title		= name;
	category->comment	= comment;
	category->color		= color;
	category->prio		= prio;

	return true;
}


void CPreferencesRem::RemoveCat(uint8 cat)
{
	CECPacket req(EC_OP_DELETE_CATEGORY);
	CEC_Category_Tag tag(cat, EC_DETAIL_CMD);
	req.AddTag(tag);
	m_conn->SendPacket(&req);
	CPreferences::RemoveCat(cat);
}


//
// Container implementation
//
CServerConnectRem::CServerConnectRem(CRemoteConnect *conn)
{
	m_CurrServer = 0;
	m_Conn = conn;
}


void CServerConnectRem::ConnectToAnyServer()
{
	CECPacket req(EC_OP_SERVER_CONNECT);
	m_Conn->SendPacket(&req);
}


void CServerConnectRem::StopConnectionTry()
{
	// lfroen: isn't Disconnect the same ?
}


void CServerConnectRem::Disconnect()
{
	CECPacket req(EC_OP_SERVER_DISCONNECT);
	m_Conn->SendPacket(&req);
}


void CServerConnectRem::ConnectToServer(CServer *server)
{
	m_Conn->ConnectED2K(server->GetIP(), server->GetPort());
}


bool CServerConnectRem::ReQuery()
{
	CECPacket stat_req(EC_OP_GET_CONNSTATE);
	m_Conn->SendRequest(this, &stat_req);

	return true;
}
	

void CServerConnectRem::HandlePacket(const CECPacket *packet)
{
	CEC_ConnState_Tag *tag =
		(CEC_ConnState_Tag *)packet->GetTagByName(EC_TAG_CONNSTATE);
	if (!tag) {
		return;
	}
	
	theApp->m_ConnState = 0;
	CServer *server;
	m_ID = tag->GetEd2kId();
	theApp->m_clientID = tag->GetClientId();
	
	if (tag->IsConnectedED2K()) {
		CECTag *srvtag = tag->GetTagByName(EC_TAG_SERVER);
		if (!srvtag) {
			return;
		}
		server = theApp->serverlist->GetByID(srvtag->GetIPv4Data().IP());
		if (m_CurrServer && (server != m_CurrServer)) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->
				HighlightServer(m_CurrServer, false);
		}
		theApp->amuledlg->m_serverwnd->serverlistctrl->
			HighlightServer(server, true);
		m_CurrServer = server;
		theApp->m_ConnState |= CONNECTED_ED2K;
	} else {
	    	if ( m_CurrServer ) {
	    		theApp->amuledlg->m_serverwnd->serverlistctrl->
				HighlightServer(m_CurrServer, false);
	    		m_CurrServer = 0;
	    	}
	}

	if (tag->IsConnectedKademlia()) {
		if (tag->IsKadFirewalled()) {
			theApp->m_ConnState |= CONNECTED_KAD_FIREWALLED;
		} else {
			theApp->m_ConnState |= CONNECTED_KAD_OK;
		}
	} else {
		if (tag->IsKadRunning()) {
			theApp->m_ConnState |= CONNECTED_KAD_NOT;
		}
	}
	
	theApp->amuledlg->ShowConnectionState();
}


/*
 * Server list: host list of ed2k servers.
 */
CServerListRem::CServerListRem(CRemoteConnect *conn)
:
CRemoteContainer<CServer, uint32, CEC_Server_Tag>(conn, false)
{
}


void CServerListRem::HandlePacket(const CECPacket *packet)
{
	CRemoteContainer<CServer, uint32, CEC_Server_Tag>::HandlePacket(packet);
	ReloadControl();
}


void CServerListRem::UpdateServerMetFromURL(wxString url)
{
	CECPacket req(EC_OP_SERVER_UPDATE_FROM_URL);
	req.AddTag(CECTag(EC_TAG_SERVERS_UPDATE_URL, url));
	
	m_conn->SendPacket(&req);
}


void CServerListRem::SaveServerMet()
{
	// lfroen: stub, nothing to do
}


void CServerListRem::FilterServers()
{
	// FIXME: add code
	//wxFAIL;
}


void CServerListRem::RemoveServer(CServer* server)
{
	m_conn->RemoveServer(server->GetIP(),server->GetPort());
}


void CServerListRem::UpdateUserFileStatus(CServer *server)
{
	if (server) {
		m_TotalUser = server->GetUsers();
		m_TotalFile = server->GetFiles();
	}
}


CServer *CServerListRem::GetServerByAddress(const wxString& WXUNUSED(address), uint16 WXUNUSED(port)) const
{
	// It's ok to return 0 for context where this code is used in remote gui
	return 0;
}

CServer *CServerListRem::GetServerByIPTCP(uint32 WXUNUSED(nIP), uint16 WXUNUSED(nPort)) const
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
	CScopedPtr<CServer> srv(in_srv);
	theApp->amuledlg->m_serverwnd->serverlistctrl->RemoveServer(srv.get());
}


uint32 CServerListRem::GetItemID(CServer *server)
{
	return server->GetIP();
}


void CServerListRem::ProcessItemUpdate(CEC_Server_Tag *, CServer *)
{
	// server list is always realoaded from scratch
	wxFAIL;
}


void CServerListRem::ReloadControl()
{
	for(uint32 i = 0;i < GetCount(); i++) {
		CServer *srv = GetByIndex(i);
		theApp->amuledlg->m_serverwnd->serverlistctrl->RefreshServer(srv);
	}
}


CIPFilterRem::CIPFilterRem(CRemoteConnect* conn)
{
	m_conn = conn;
}


void CIPFilterRem::Reload()
{
	CECPacket req(EC_OP_IPFILTER_RELOAD);
	m_conn->SendPacket(&req);
}


void CIPFilterRem::Update(wxString url)
{
	CECPacket req(EC_OP_IPFILTER_UPDATE);
	req.AddTag(CECTag(EC_TAG_STRING, url));
	
	m_conn->SendPacket(&req);
}


/*
 * Shared files list
 */
CSharedFilesRem::CSharedFilesRem(CRemoteConnect *conn) : CRemoteContainer<CKnownFile, uint32, CEC_SharedFile_Tag>(conn, true)
{
	m_rename_file = NULL;
}


void CSharedFilesRem::Reload(bool, bool)
{
	CECPacket req(EC_OP_SHAREDFILES_RELOAD);
	
	m_conn->SendPacket(&req);
}


void CSharedFilesRem::AddFilesFromDirectory(const CPath& path)
{
	CECPacket req(EC_OP_SHAREDFILES_ADD_DIRECTORY);

	req.AddTag(CECTag(EC_TAG_PREFS_DIRECTORIES, path.GetRaw()));
	
	m_conn->SendPacket(&req);
}


bool CSharedFilesRem::RenameFile(CKnownFile* file, const CPath& newName)
{
	// We use the printable name, as the filename originated from user input,
	// and the filesystem name might not be valid on the remote host.
	const wxString strNewName = newName.GetPrintable();

	CECPacket request(EC_OP_RENAME_FILE);
	request.AddTag(CECTag(EC_TAG_KNOWNFILE, file->GetFileHash()));
	request.AddTag(CECTag(EC_TAG_PARTFILE_NAME, strNewName));

	m_conn->SendRequest(this, &request);
	m_rename_file = file;
	m_new_name = strNewName;
	
	return true;
}


void CSharedFilesRem::HandlePacket(const CECPacket *packet)
{
	if (m_rename_file && (packet->GetOpCode() == EC_OP_NOOP)) {
		m_rename_file->SetFileName(CPath(m_new_name));
		m_rename_file = NULL;
	} else if (packet->GetOpCode() != EC_OP_FAILED) {
		CRemoteContainer<CKnownFile, uint32, CEC_SharedFile_Tag>::HandlePacket(packet);
	}
}


CKnownFile *CSharedFilesRem::CreateItem(CEC_SharedFile_Tag *tag)
{
	CKnownFile *file = new CKnownFile(tag);

	m_enc_map[file->ECID()] = RLE_Data(file->GetPartCount(), true);

	ProcessItemUpdate(tag, file);
	
	theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFile(file);
	
	return file;
}


void CSharedFilesRem::DeleteItem(CKnownFile *in_file)
{
	CScopedPtr<CKnownFile> file(in_file);

	m_enc_map.erase(file->ECID());
	
	theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->RemoveFile(file.get());
}


uint32 CSharedFilesRem::GetItemID(CKnownFile *file)
{
	return file->ECID();
}


void CSharedFilesRem::ProcessItemUpdate(CEC_SharedFile_Tag *tag, CKnownFile *file)
{
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	if (parttag) {
		const uint8 *data =	m_enc_map[file->ECID()].Decode(
				(uint8 *)parttag->GetTagData(),
				parttag->GetTagDataLen());
		for(int i = 0; i < file->GetPartCount(); ++i) {
			file->m_AvailPartFrequency[i] = data[i];
		}
	}
	tag->GetRequests(&file->statistic.requested);
	tag->GetAllRequests(&file->statistic.alltimerequested);
	tag->GetAccepts(&file->statistic.accepted);
	tag->GetAllAccepts(&file->statistic.alltimeaccepted);
	tag->GetXferred(&file->statistic.transferred);
	tag->GetAllXferred(&file->statistic.alltimetransferred);
	tag->Prio(&file->m_iUpPriorityEC);
	if (file->m_iUpPriorityEC >= 10) {
		file->m_iUpPriority = file->m_iUpPriorityEC - 10;
		file->m_bAutoUpPriority = true;
	} else {
		file->m_iUpPriority = file->m_iUpPriorityEC;
		file->m_bAutoUpPriority = false;
	}
	tag->GetCompleteSourcesLow(&file->m_nCompleteSourcesCountLo);
	tag->GetCompleteSourcesHigh(&file->m_nCompleteSourcesCountHi);

	theApp->knownfiles->requested += file->statistic.requested;
	theApp->knownfiles->transferred += file->statistic.transferred;
	theApp->knownfiles->accepted += file->statistic.transferred;
	
	theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->UpdateItem(file);
}

bool CSharedFilesRem::Phase1Done(const CECPacket *)
{
	theApp->knownfiles->requested = 0;
	theApp->knownfiles->transferred = 0;
	theApp->knownfiles->accepted = 0;
	
	return true;
}

void CSharedFilesRem::SetFilePrio(CKnownFile *file, uint8 prio)
{
	CECPacket req(EC_OP_SHARED_SET_PRIO);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, prio));
	
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}

/*
 * List of uploading and waiting clients.
 */
CUpDownClientListRem::CUpDownClientListRem(CRemoteConnect *conn, int viewtype)
:
CRemoteContainer<CUpDownClient, uint32, CEC_UpDownClient_Tag>(conn, true)
{
	m_viewtype = viewtype;
}


CUpDownClient::CUpDownClient(CEC_UpDownClient_Tag *tag) : CECID(tag->ID())
{
	m_bRemoteQueueFull = false;
	m_nUserIDHybrid = tag->UserID();
	m_Username = tag->ClientName();
	m_clientSoft = tag->ClientSoftware();
	m_clientVersionString = GetSoftName(m_clientSoft);
	m_clientSoftString = m_clientVersionString;
	m_bEmuleProtocol = false;

	// The functions to retrieve m_clientVerString information are
	// currently in BaseClient.cpp, which is not linked in remote-gui app.
	// So, in the meantime, we use a tag sent from core.
	m_clientVerString = tag->SoftVerStr();
	m_strModVersion = wxEmptyString;
	wxString clientModString;
	if (!clientModString.IsEmpty()) {
		m_clientVerString += wxT(" - ") + clientModString;
	}
	m_fullClientVerString = m_clientSoftString + wxT(" ") + m_clientVerString;

	// User hash
	m_UserHash = tag->UserHash();

	// User IP:Port
	m_nConnectIP = m_dwUserIP = tag->UserIP();
	m_nUserPort = tag->UserPort();
	m_FullUserIP = m_nConnectIP;

	// Server IP:Port
	m_dwServerIP = tag->ServerIP();
	m_nServerPort = tag->ServerPort();
	m_ServerName = tag->ServerName();

	m_Friend = 0;
	if (tag->HaveFile()) {
		m_uploadingfile = theApp->sharedfiles->GetByID(tag->FileID());
	} else {
		m_uploadingfile = NULL;
	}

	m_nCurSessionUp = 0;

	credits = new CClientCredits(new CreditStruct());
}

uint16 CUpQueueRem::GetWaitingPosition(const CUpDownClient *client) const
{
	return client->GetWaitingPosition();
}

/* Warning: do common base */


bool CUpDownClient::IsIdentified() const 
{
	return m_identState == IS_IDENTIFIED;
}


bool CUpDownClient::IsBadGuy() const 
{
	return m_identState == IS_IDBADGUY;
}


bool CUpDownClient::SUIFailed() const 
{
	return m_identState == IS_IDFAILED;
}


bool CUpDownClient::SUINeeded() const 
{
	return m_identState == IS_IDNEEDED;
}


bool CUpDownClient::SUINotSupported() const 
{
	return m_identState == IS_NOTAVAILABLE;
}


uint64 CUpDownClient::GetDownloadedTotal() const 
{
	return credits->GetDownloadedTotal();
}


uint64 CUpDownClient::GetUploadedTotal() const 
{
	return credits->GetUploadedTotal();
}


double CUpDownClient::GetScoreRatio() const
{
	return credits->GetScoreRatio(GetIP(), theApp->CryptoAvailable());
}

/* End Warning */


CUpDownClient::~CUpDownClient()
{
	delete credits;
}


CUpDownClient *CUpDownClientListRem::CreateItem(CEC_UpDownClient_Tag *tag)
{
	CUpDownClient *client = new CUpDownClient(tag);
	ProcessItemUpdate(tag, client);
	
	theApp->amuledlg->m_transferwnd->clientlistctrl->InsertClient(client, (ViewType)m_viewtype);
	
	return client;
}


void CUpDownClientListRem::DeleteItem(CUpDownClient *client)
{
	theApp->amuledlg->m_transferwnd->clientlistctrl->
		RemoveClient(client, (ViewType)m_viewtype);
	delete client;
}


uint32 CUpDownClientListRem::GetItemID(CUpDownClient *client)
{
	return client->ECID();
}


void CUpDownClientListRem::ProcessItemUpdate(
	CEC_UpDownClient_Tag *tag,
	CUpDownClient *client)
{
	tag->UserID(&client->m_nUserIDHybrid);
	tag->GetCurrentIdentState(&client->m_identState);
	tag->HasObfuscatedConnection(&client->m_hasbeenobfuscatinglately);
	tag->HasExtendedProtocol(&client->m_bEmuleProtocol);

	tag->WaitingPosition(&client->m_waitingPosition);
	tag->RemoteQueueRank(&client->m_nRemoteQueueRank);
	client->m_bRemoteQueueFull = client->m_nRemoteQueueRank == 0xffff;
	tag->AskedCount(&client->m_cAsked);
	
	tag->ClientDownloadState(&client->m_nDownloadState);
	tag->ClientUploadState(&client->m_nUploadState);
	
	tag->SpeedUp(&client->m_nUpDatarate);
	if ( client->m_nDownloadState == DS_DOWNLOADING ) {
		tag->SpeedDown(&client->kBpsDown);
	} else {
		client->kBpsDown = 0;
	}

	tag->WaitTime(&client->m_WaitTime);
	tag->XferTime(&client->m_UpStartTimeDelay);
	tag->LastReqTime(&client->m_dwLastUpRequest);
	tag->QueueTime(&client->m_WaitStartTime);
	
	CreditStruct *credit_struct =
		(CreditStruct *)client->credits->GetDataStruct();
	tag->XferUp(&credit_struct->uploaded);
	tag->XferUpSession(&client->m_nTransferredUp);

	tag->XferDown(&credit_struct->downloaded);
	tag->XferDownSession(&client->m_nTransferredDown);

	tag->Score(&client->m_score);
	tag->Rating(&client->m_rating);

	theApp->amuledlg->m_transferwnd->clientlistctrl->UpdateClient(client,
		theApp->amuledlg->m_transferwnd->clientlistctrl->GetListView());	// Fixme, Listview shouldn't be required as parameter
}


CUpQueueRem::CUpQueueRem(CRemoteConnect *conn)
:
m_up_list(conn, vtUploading), m_wait_list(conn, vtQueued)
{
}


/*
 * Download queue container: hold PartFiles with progress status
 * 
 */


CDownQueueRem::CDownQueueRem(CRemoteConnect *conn)
:
CRemoteContainer<CPartFile, uint32, CEC_PartFile_Tag>(conn, true)
{
}


bool CDownQueueRem::AddLink(const wxString &link, uint8 cat)
{
	CECPacket req(EC_OP_ADD_LINK);
	CECTag link_tag(EC_TAG_STRING, link);
	link_tag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	req.AddTag(link_tag);
	
	m_conn->SendPacket(&req);
	return true;
}


void CDownQueueRem::StopUDPRequests()
{
	// have no idea what is it about
}


void CDownQueueRem::ResetCatParts(int cat)
{
	// Called when category is deleted. Command will be performed on the remote side,
	// but files still should be updated here right away, or drawing errors (colour not available)
	// will happen.
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		CPartFile* file = GetFileByIndex( i );
		
		if ( file->GetCategory() == cat ) {
			// Reset the category
			file->SetCategory( 0 );
		} else if ( file->GetCategory() > cat ) {
			// Set to the new position of the original category
			file->SetCategory( file->GetCategory() - 1 );
		}
	}
}


bool CDownQueueRem::IsPartFile(const CKnownFile *) const
{
	// hope i understand it right
	return true;
}


void CDownQueueRem::OnConnectionState(bool)
{
}


CPartFile *CDownQueueRem::CreateItem(CEC_PartFile_Tag *tag)
{
	CPartFile *file = new CPartFile(tag);
	m_enc_map[file->ECID()] = PartFileEncoderData();
	ProcessItemUpdate(tag, file);
	
	theApp->amuledlg->m_transferwnd->downloadlistctrl->AddFile(file);
	return file;
}


void CDownQueueRem::DeleteItem(CPartFile *in_file)
{
	CScopedPtr<CPartFile> file(in_file);

	theApp->amuledlg->m_transferwnd->downloadlistctrl->RemoveFile(file.get());
	
	m_enc_map.erase(file->ECID());
}


uint32 CDownQueueRem::GetItemID(CPartFile *file)
{
	return file->ECID();
}


void CDownQueueRem::ProcessItemUpdate(CEC_PartFile_Tag *tag, CPartFile *file)
{
	//
	// update status
	//
	tag->Speed(&file->m_kbpsDown);
	file->kBpsDown = file->m_kbpsDown / 1024.0;
	
	tag->SizeXfer(&file->transferred);
	tag->SizeDone(&file->completedsize);
	tag->SourceXferCount(&file->transferingsrc);
	tag->SourceNotCurrCount(&file->m_notCurrentSources);
	tag->SourceCount(&file->m_source_count);
	tag->SourceCountA4AF(&file->m_a4af_source_count);
	tag->FileStatus(&file->status);
	tag->Stopped(&file->m_stopped);

	tag->LastSeenComplete(&file->lastseencomplete);
	tag->LastDateChanged(&file->m_lastDateChanged);
	tag->DownloadActiveTime(&file->m_nDlActiveTime);
	tag->AvailablePartCount(&file->m_availablePartsCount);

	tag->GetLostDueToCorruption(&file->m_iLostDueToCorruption);
	tag->GetGainDueToCompression(&file->m_iGainDueToCompression);
	tag->TotalPacketsSavedDueToICH(&file->m_iTotalPacketsSavedDueToICH);

	tag->FileCat(&file->m_category);
	
	tag->Prio(&file->m_iDownPriorityEC);
	if ( file->m_iDownPriorityEC >= 10 ) {
		file->m_iDownPriority = file->m_iDownPriorityEC - 10;
		file->m_bAutoDownPriority = true;
	} else {
		file->m_iDownPriority = file->m_iDownPriorityEC;
		file->m_bAutoDownPriority = false;
	}

	file->percentcompleted = (100.0*file->GetCompletedSize()) / file->GetFileSize();

	//
	// Copy part/gap status
	//
	CECTag *gaptag = tag->GetTagByName(EC_TAG_PARTFILE_GAP_STATUS);
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	CECTag *reqtag = tag->GetTagByName(EC_TAG_PARTFILE_REQ_STATUS);
	if (gaptag || parttag || reqtag) {
		wxASSERT(m_enc_map.count(file->ECID()));
		
		PartFileEncoderData &encoder = m_enc_map[file->ECID()];

		if (gaptag) {
			ArrayOfUInts64 gaps;
			encoder.DecodeGaps(gaptag, gaps);
			int gap_size = gaps.size() / 2;
			// clear gaplist
			file->m_gaplist.Init(file->GetFileSize(), false);

			// and refill it
			for (int j = 0; j < gap_size; j++) {
				file->m_gaplist.AddGap(gaps[2*j], gaps[2*j+1]);
			}
		}
		if (parttag) {
			encoder.DecodeParts(parttag, file->m_SrcpartFrequency);
			// sanity check
			wxASSERT (file->m_SrcpartFrequency.size() == file->GetPartCount());
		}
		if (reqtag) {
			ArrayOfUInts64 reqs;
			encoder.DecodeReqs(reqtag, reqs);
			int req_size = reqs.size() / 2;
			// clear reqlist
			DeleteContents(file->m_requestedblocks_list);

			// and refill it
			for (int j = 0; j < req_size; j++) {
				Requested_Block_Struct* block = new Requested_Block_Struct;
				block->StartOffset = reqs[2*j];
				block->EndOffset   = reqs[2*j+1];
				file->m_requestedblocks_list.push_back(block);
			}
		}
	}
	
	// Get source names and counts
	CECTag *srcnametag = tag->GetTagByName(EC_TAG_PARTFILE_SOURCE_NAMES);
	if (srcnametag) {
		SourcenameItemMap &map = file->GetSourcenameItemMap();
		size_t max = srcnametag->GetTagCount();
		for (size_t i = 0; i < max; i++) {
			CECTag *ntag = srcnametag->GetTagByIndex(i);
			uint32 key = ntag->GetInt();
			int count = ntag->GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS)->GetInt();
			if (count == 0) {
				map.erase(key);
			} else {
				SourcenameItem &item = map[key];
				item.count = count;
				CECTag *nametag = ntag->GetTagByName(EC_TAG_PARTFILE_SOURCE_NAMES);
				if (nametag) {
					item.name = nametag->GetStringData();
				}
			}
		}
	}
	
	// Get comments
	CECTag *commenttag = tag->GetTagByName(EC_TAG_PARTFILE_COMMENTS);
	if (commenttag) {
		file->ClearFileRatingList();
		int max = commenttag->GetTagCount();
		for (int i = 0; i < max - 3; ) {
			wxString u = commenttag->GetTagByIndex(i++)->GetStringData();
			wxString f = commenttag->GetTagByIndex(i++)->GetStringData();
			int r = commenttag->GetTagByIndex(i++)->GetInt();
			wxString c = commenttag->GetTagByIndex(i++)->GetStringData();
			file->AddFileRatingList(u, f, r, c);
		}
		file->UpdateFileRatingCommentAvail();
	}
		
	theApp->amuledlg->m_transferwnd->downloadlistctrl->UpdateItem(file);
}


bool CDownQueueRem::Phase1Done(const CECPacket *)
{
	return true;
}


void CDownQueueRem::SendFileCommand(CPartFile *file, ec_tagname_t cmd)
{
	CECPacket req(cmd);
	req.AddTag(CECTag(EC_TAG_PARTFILE, file->GetFileHash()));
	
	m_conn->SendPacket(&req);
}


void CDownQueueRem::Prio(CPartFile *file, uint8 prio)
{
	CECPacket req(EC_OP_PARTFILE_PRIO_SET);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, prio));
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}


void CDownQueueRem::AutoPrio(CPartFile *file, bool flag)
{
	CECPacket req(EC_OP_PARTFILE_PRIO_SET);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
		(uint8)(flag ? PR_AUTO : file->GetDownPriority())));
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}


void CDownQueueRem::Category(CPartFile *file, uint8 cat)
{
	CECPacket req(EC_OP_PARTFILE_SET_CAT);
	file->m_category = cat;
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}


void CDownQueueRem::AddSearchToDownload(CSearchFile* file, uint8 category)
{
	CECPacket req(EC_OP_DOWNLOAD_SEARCH_RESULT);
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, category));
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}


CClientListRem::CClientListRem(CRemoteConnect *conn)
{
	m_conn = conn;
}


void CClientListRem::FilterQueues()
{
	// FIXME: add code
	//wxFAIL;
}


CSearchListRem::CSearchListRem(CRemoteConnect *conn) : CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag>(conn, true)
{
	m_curr_search = -1;
}


wxString CSearchListRem::StartNewSearch(
	uint32* nSearchID, SearchType search_type, 
	const CSearchList::CSearchParams& params)
{
	CECPacket search_req(EC_OP_SEARCH_START);
	EC_SEARCH_TYPE ec_search_type = EC_SEARCH_LOCAL;
	switch(search_type) {
		case LocalSearch: ec_search_type = EC_SEARCH_LOCAL; break;
		case GlobalSearch: ec_search_type =  EC_SEARCH_GLOBAL; break;
		case KadSearch: ec_search_type =  EC_SEARCH_KAD; break;
	}
	search_req.AddTag(
		CEC_Search_Tag(params.searchString, ec_search_type,
		params.typeText, params.extension, params.availability,
		params.minSize, params.maxSize));
		
	m_conn->SendPacket(&search_req);
	m_curr_search = *(nSearchID); // No kad remote search yet.
	
	Flush();
	
	return wxEmptyString; // EC reply will have the error mesg is needed.
}


void CSearchListRem::StopGlobalSearch()
{
	if (m_curr_search != -1) {
		CECPacket search_req(EC_OP_SEARCH_STOP);
		m_conn->SendPacket(&search_req);
	}
}


void CSearchListRem::StopKadSearch()
{
// FIXME implementation needed
}


void CSearchListRem::HandlePacket(const CECPacket *packet)
{
	if ( packet->GetOpCode() == EC_OP_SEARCH_PROGRESS ) {
		CoreNotify_Search_Update_Progress(packet->GetTagByIndex(0)->GetInt());
	} else {
		CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag>::HandlePacket(packet);
	}
}


CSearchFile::CSearchFile(CEC_SearchFile_Tag *tag)
:
m_parent(NULL),
m_showChildren(false),
m_sourceCount(0),
m_completeSourceCount(0),
m_kademlia(false),
m_downloadStatus(NEW),
m_clientID(0),
m_clientPort(0)
{
	SetFileName(CPath(tag->FileName()));
	m_abyFileHash = tag->ID();
	SetFileSize(tag->SizeFull());
	
	m_searchID = theApp->searchlist->m_curr_search;

}


// dtor is virtual - must be implemented
CSearchFile::~CSearchFile()
{	
}


CSearchFile *CSearchListRem::CreateItem(CEC_SearchFile_Tag *tag)
{
	CSearchFile *file = new CSearchFile(tag);
	ProcessItemUpdate(tag, file);
	
	theApp->amuledlg->m_searchwnd->AddResult(file);
	
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
	uint32 sourceCount = file->m_sourceCount;
	uint32 completeSourceCount = file->m_completeSourceCount;
	CSearchFile::DownloadStatus status = file->m_downloadStatus;
	tag->SourceCount(&file->m_sourceCount);
	tag->CompleteSourceCount(&file->m_completeSourceCount);
	tag->DownloadStatus((uint32 *) &file->m_downloadStatus);

	if (file->m_sourceCount != sourceCount
			|| file->m_completeSourceCount != completeSourceCount
			|| file->m_downloadStatus != status) {
		if (theApp->amuledlg && theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->UpdateResult(file);
		}
	}
}


bool CSearchListRem::Phase1Done(const CECPacket *WXUNUSED(reply))
{
	CECPacket progress_req(EC_OP_SEARCH_PROGRESS);
	m_conn->SendRequest(this, &progress_req);

	return true;
}


void CSearchListRem::RemoveResults(long nSearchID)
{
	ResultMap::iterator it = m_results.find(nSearchID);
	if (it != m_results.end()) {
		CSearchResultList& list = it->second;
		for (unsigned int i = 0; i < list.size(); ++i) {
			delete list[i];
		}
		m_results.erase(it);
	}
}


const CSearchResultList& CSearchListRem::GetSearchResults(long nSearchID)
{
	ResultMap::const_iterator it = m_results.find(nSearchID);
	if (it != m_results.end()) {
		return it->second;
	}

	// TODO: Should we assert in this case?
	static CSearchResultList list;
	return list;
}


void CStatsUpdaterRem::HandlePacket(const CECPacket *packet)
{
	theStats::UpdateStats(packet);
  theApp->ShowUserCount(); // maybe there should be a check if a usercount changed ?
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
	wxFAIL;
}


void  CUpDownClient::UnBan()
{
	// FIXME: add code
	wxFAIL;
}


void CUpDownClient::RequestSharedFileList()
{
	// FIXME: add code
	wxFAIL;
}


void CKnownFile::SetFileComment(const wxString &)
{
	// FIXME: add code
	wxMessageBox(_("Comments and ratings are not supported on remote gui yet"), _("Information"), wxOK | wxICON_INFORMATION);
}


void CKnownFile::SetFileRating(unsigned char)
{
	// FIXME: add code
}


// I don't think it will be implemented - too match data transfer. But who knows ?
wxString CUpDownClient::ShowDownloadingParts() const
{
	return wxEmptyString;
}


bool CUpDownClient::SwapToAnotherFile(
	bool WXUNUSED(bIgnoreNoNeeded),
	bool WXUNUSED(ignoreSuspensions),
	bool WXUNUSED(bRemoveCompletely),
	CPartFile* WXUNUSED(toFile))
{
	// FIXME: add code
	wxFAIL;
	return false;
}


//
// Those functions are virtual. So even they don't get called they must
// be defined so linker will be happy
//
CPacket* CKnownFile::CreateSrcInfoPacket(const CUpDownClient *, uint8 /*byRequestedVersion*/, uint16 /*nRequestedOptions*/)
{
	wxFAIL;
	return 0;
}


bool CKnownFile::LoadFromFile(const class CFileDataIO*)
{
	wxFAIL;
	return false;
}


void CKnownFile::UpdatePartsInfo()
{
	wxFAIL;
}


CPacket* CPartFile::CreateSrcInfoPacket(CUpDownClient const *, uint8 /*byRequestedVersion*/, uint16 /*nRequestedOptions*/)
{
	wxFAIL;
	return 0;
}


void CPartFile::UpdatePartsInfo()
{
	wxFAIL;
}


void CPartFile::UpdateFileRatingCommentAvail()
{
	bool prevComment = m_hasComment;
	int prevRating = m_iUserRating;

	m_hasComment = false;
	m_iUserRating = 0;
	int ratingCount = 0;

	FileRatingList::iterator it = m_FileRatingList.begin();
	for (; it != m_FileRatingList.end(); ++it) {
		SFileRating& cur_rat = *it;
		
		if (!cur_rat.Comment.IsEmpty()) {
			m_hasComment = true;
		}

		uint8 rating = cur_rat.Rating;
		if (rating) {
			wxASSERT(rating <= 5);
			
			ratingCount++;
			m_iUserRating += rating;
		}
	}
	
	if (ratingCount) {
		m_iUserRating /= ratingCount;
		wxASSERT(m_iUserRating > 0 && m_iUserRating <= 5);
	}
	
	if ((prevComment != m_hasComment) || (prevRating != m_iUserRating)) {
		UpdateDisplayedInfo();
	}
}

bool CPartFile::SavePartFile(bool)
{
	wxFAIL;
	return false;
}

CamuleRemoteGuiApp *theApp;

//
// since gui is not linked with amule.cpp - define events here
//
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
// File_checked_for_headers
