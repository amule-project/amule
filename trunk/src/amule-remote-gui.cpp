//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include "SourceListCtrl.h"
#include "ChatWnd.h"
#include "DataToText.h"			// Needed for GetSoftName()
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "Friend.h"
#include "GetTickCount.h"	// Needed for GetTickCount
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
#include "UpDownClientEC.h"		// Needed for CUpDownClient
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
#include "ScopedPtr.h"
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg


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
	static uint32 msPrevStats = 0;
	
	if (m_connect->RequestFifoFull()) {
		return;
	}
	
	switch (request_step) {
	case 0:
		serverconnect->ReQuery();
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
		if (amuledlg->m_sharedfileswnd->IsShown()
			|| amuledlg->m_chatwnd->IsShown()
			|| amuledlg->m_serverwnd->IsShown()) {
			// update downloads, shared files and servers
			knownfiles->DoRequery(EC_OP_GET_UPDATE, EC_TAG_KNOWNFILE);
		} else if (amuledlg->m_transferwnd->IsShown()) {
			// update both downloads and shared files
			knownfiles->DoRequery(EC_OP_GET_UPDATE, EC_TAG_KNOWNFILE);
		} else if (amuledlg->m_searchwnd->IsShown()) {
			if (searchlist->m_curr_search != -1) {
				searchlist->DoRequery(EC_OP_SEARCH_RESULTS, EC_TAG_SEARCHFILE);
			}
		} else if (amuledlg->m_statisticswnd->IsShown()) {
			int sStatsUpdate = thePrefs::GetStatsInterval();
			uint32 msCur = theStats::GetUptimeMillis();
			if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
				msPrevStats = msCur;
				stattree->DoRequery();
			}
		}
		// Back to the roots
		request_step = 0;
		break;
	default:
		wxFAIL;
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
	if (event.GetInt() == HTTP_GeoIP) {
		amuledlg->IP2CountryDownloadFinished(event.GetExtraLong());
		// If we updated, the dialog is already up. Redraw it to show the flags.
		amuledlg->Refresh();
	}
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
	delete m_allUploadingKnownFile;
	delete stattree;
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
	long enableZLIB;
	wxConfig::Get()->Read(wxT("/EC/ZLIB"), &enableZLIB, 1);
	m_connect->SetCapabilities(enableZLIB != 0, true, false);	// ZLIB, UTF8 numbers, notification
	
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
	AddLogLineC(reply);
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
	stattree = new CStatTreeRem(m_connect);
	
	clientlist = new CUpDownClientListRem(m_connect);
	searchlist = new CSearchListRem(m_connect);
	serverlist = new CServerListRem(m_connect);
	friendlist = new CFriendListRem(m_connect);

	
	sharedfiles	= new CSharedFilesRem(m_connect);
	knownfiles = new CKnownFilesRem(m_connect);

	downloadqueue = new CDownQueueRem(m_connect);
	ipfilter = new CIPFilterRem(m_connect);

	m_allUploadingKnownFile = new CKnownFile;

	// Create main dialog
	InitGui(m_geometryEnabled, m_geometryString);

	// Forward wxLog events to CLogger
	wxLog::SetActiveTarget(new CLoggerTarget);
	knownfiles->DoRequery(EC_OP_GET_UPDATE, EC_TAG_KNOWNFILE);

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


wxString CamuleRemoteGuiApp::GetLog(bool reset)
{
	if (reset) {
		amuledlg->ResetLog(ID_LOGVIEW);
		CECPacket req(EC_OP_RESET_LOG);
		m_connect->SendPacket(&req);
	}
	return wxEmptyString;
}


wxString CamuleRemoteGuiApp::GetServerLog(bool)
{
	return wxEmptyString;
}


bool CamuleRemoteGuiApp::AddServer(CServer * server, bool)
{
	CECPacket req(EC_OP_SERVER_ADD);
	req.AddTag(CECTag(EC_TAG_SERVER_ADDRESS, CFormat(wxT("%s:%d")) % server->GetAddress() % server->GetPort()));
	req.AddTag(CECTag(EC_TAG_SERVER_NAME, server->GetListName()));
	m_connect->SendPacket(&req);

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

	const CECTag *cat_tags = packet->GetTagByName(EC_TAG_PREFS_CATEGORIES);
	if (cat_tags) {
		for (CECTag::const_iterator it = cat_tags->begin(); it != cat_tags->end(); it++) {
			const CECTag &cat_tag = *it;
			Category_Struct *cat = new Category_Struct;
			cat->title = cat_tag.GetTagByName(EC_TAG_CATEGORY_TITLE)->GetStringData();
			cat->path = CPath(cat_tag.GetTagByName(EC_TAG_CATEGORY_PATH)->GetStringData());
			cat->comment = cat_tag.GetTagByName(EC_TAG_CATEGORY_COMMENT)->GetStringData();
			cat->color =  cat_tag.GetTagByName(EC_TAG_CATEGORY_COLOR)->GetInt();
			cat->prio = cat_tag.GetTagByName(EC_TAG_CATEGORY_PRIO)->GetInt();
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
		if (srvtag) {
			server = theApp->serverlist->GetByID(srvtag->GetInt());
			if (server != m_CurrServer) {
				theApp->amuledlg->m_serverwnd->serverlistctrl->HighlightServer(server, true);
				m_CurrServer = server;
			}
		}
		theApp->m_ConnState |= CONNECTED_ED2K;
	} else if ( m_CurrServer ) {
	    theApp->amuledlg->m_serverwnd->serverlistctrl->HighlightServer(m_CurrServer, false);
	    m_CurrServer = 0;
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
CRemoteContainer<CServer, uint32, CEC_Server_Tag>(conn, true)
{
}


void CServerListRem::HandlePacket(const CECPacket *)
{
	// There is no packet for the server list, it is part of the general update packet
	wxFAIL;
	// CRemoteContainer<CServer, uint32, CEC_Server_Tag>::HandlePacket(packet);
}


void CServerListRem::UpdateServerMetFromURL(wxString url)
{
	CECPacket req(EC_OP_SERVER_UPDATE_FROM_URL);
	req.AddTag(CECTag(EC_TAG_SERVERS_UPDATE_URL, url));
	
	m_conn->SendPacket(&req);
}


void CServerListRem::SetStaticServer(CServer* server, bool isStatic)
{
	// update display right away
	server->SetIsStaticMember(isStatic);
	Notify_ServerRefresh(server);

	CECPacket req(EC_OP_SERVER_SET_STATIC_PRIO);
	req.AddTag(CECTag(EC_TAG_SERVER, server->ECID()));
	req.AddTag(CECTag(EC_TAG_SERVER_STATIC, isStatic));
	
	m_conn->SendPacket(&req);
}


void CServerListRem::SetServerPrio(CServer* server, uint32 prio)
{
	// update display right away
	server->SetPreference(prio);
	Notify_ServerRefresh(server);

	CECPacket req(EC_OP_SERVER_SET_STATIC_PRIO);
	req.AddTag(CECTag(EC_TAG_SERVER, server->ECID()));
	req.AddTag(CECTag(EC_TAG_SERVER_PRIO, prio));
	
	m_conn->SendPacket(&req);
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
	CServer * server = new CServer(tag);
	ProcessItemUpdate(tag, server);
	return server;
}


void CServerListRem::DeleteItem(CServer *in_srv)
{
	CScopedPtr<CServer> srv(in_srv);
	theApp->amuledlg->m_serverwnd->serverlistctrl->RemoveServer(srv.get());
}


uint32 CServerListRem::GetItemID(CServer *server)
{
	return server->ECID();
}


void CServerListRem::ProcessItemUpdate(CEC_Server_Tag * tag, CServer * server)
{
	if (!tag->HasChildTags()) {
		return;
	}
	tag->ServerName(& server->listname);
	tag->ServerDesc(& server->description);
	tag->ServerVersion(& server->m_strVersion);
	tag->GetMaxUsers(& server->maxusers);
	
	tag->GetFiles(& server->files);
	tag->GetUsers(& server->users);
   
	tag->GetPrio(& server->preferences); // SRV_PR_NORMAL = 0, so it's ok
    tag->GetStatic(& server->staticservermember);

	tag->GetPing(& server->ping);
	tag->GetFailed(& server->failedcount);	

	theApp->amuledlg->m_serverwnd->serverlistctrl->RefreshServer(server);
}


CServer::CServer(CEC_Server_Tag *tag) : CECID(tag->GetInt())
{
	ip = tag->GetTagByNameSafe(EC_TAG_SERVER_IP)->GetInt();
	port = tag->GetTagByNameSafe(EC_TAG_SERVER_PORT)->GetInt();
	
	Init();
}


/*
 * IP filter
 */
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
CSharedFilesRem::CSharedFilesRem(CRemoteConnect *conn)
{
	m_conn = conn;
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

	m_conn->SendPacket(&request);
	
	return true;
}


void CSharedFilesRem::SetFileCommentRating(CKnownFile* file, const wxString& newComment, int8 newRating)
{
	CECPacket request(EC_OP_SHARED_FILE_SET_COMMENT);
	request.AddTag(CECTag(EC_TAG_KNOWNFILE, file->GetFileHash()));
	request.AddTag(CECTag(EC_TAG_KNOWNFILE_COMMENT, newComment));
	request.AddTag(CECTag(EC_TAG_KNOWNFILE_RATING, newRating));

	m_conn->SendPacket(&request);
}


void CKnownFilesRem::DeleteItem(CKnownFile * file)
{
	uint32 id = file->ECID();
	if (theApp->sharedfiles->count(id)) {
		theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->RemoveFile(file);
		theApp->sharedfiles->erase(id);
	}
	if (theApp->downloadqueue->count(id)) {
		theApp->amuledlg->m_transferwnd->downloadlistctrl->RemoveFile((CPartFile *) file);
		theApp->downloadqueue->erase(id);
	}
	delete file;
}


uint32 CKnownFilesRem::GetItemID(CKnownFile *file)
{
	return file->ECID();
}


void CKnownFilesRem::ProcessItemUpdate(CEC_SharedFile_Tag *tag, CKnownFile *file)
{
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	if (parttag) {
		const uint8 *data =	file->m_partStatus.Decode(
				(uint8 *)parttag->GetTagData(),
				parttag->GetTagDataLen());
		for(int i = 0; i < file->GetPartCount(); ++i) {
			file->m_AvailPartFrequency[i] = data[i];
		}
	}
	wxString fileName;
	if (tag->FileName(fileName)) {
		file->SetFileName(CPath(fileName));
	}
	if (tag->FilePath(fileName)) {
		file->m_filePath = CPath(fileName);
	}
	tag->UpPrio(&file->m_iUpPriorityEC);
	tag->GetAICHHash(file->m_AICHMasterHash);
	tag->GetRequests(&file->statistic.requested);
	tag->GetAllRequests(&file->statistic.alltimerequested);
	tag->GetAccepts(&file->statistic.accepted);
	tag->GetAllAccepts(&file->statistic.alltimeaccepted);
	tag->GetXferred(&file->statistic.transferred);
	tag->GetAllXferred(&file->statistic.alltimetransferred);
	tag->UpPrio(&file->m_iUpPriorityEC);
	if (file->m_iUpPriorityEC >= 10) {
		file->m_iUpPriority = file->m_iUpPriorityEC - 10;
		file->m_bAutoUpPriority = true;
	} else {
		file->m_iUpPriority = file->m_iUpPriorityEC;
		file->m_bAutoUpPriority = false;
	}
	tag->GetCompleteSourcesLow(&file->m_nCompleteSourcesCountLo);
	tag->GetCompleteSourcesHigh(&file->m_nCompleteSourcesCountHi);
	tag->GetCompleteSources(&file->m_nCompleteSourcesCount);

	tag->GetOnQueue(&file->m_queuedCount);

	tag->GetComment(file->m_strComment);
	tag->GetRating(file->m_iRating);

	requested += file->statistic.requested;
	transferred += file->statistic.transferred;
	accepted += file->statistic.transferred;
	
	theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->UpdateItem(file);

	if (file->IsPartFile()) {
		ProcessItemUpdatePartfile((CEC_PartFile_Tag *) tag, (CPartFile *) file);
	}
}

void CSharedFilesRem::SetFilePrio(CKnownFile *file, uint8 prio)
{
	CECPacket req(EC_OP_SHARED_SET_PRIO);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, prio));
	
	req.AddTag(hashtag);
	
	m_conn->SendPacket(&req);
}

void CKnownFilesRem::ProcessUpdate(const CECTag *reply, CECPacket *, int)
{
	requested = 0;
	transferred = 0;
	accepted = 0;

	std::set<uint32> core_files;
	for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); it++) {
		const CECTag * curTag = &*it;
		ec_tagname_t tagname = curTag->GetTagName();
		if (tagname == EC_TAG_CLIENT) {
			theApp->clientlist->ProcessUpdate(curTag, NULL, EC_TAG_CLIENT);
		} else if (tagname == EC_TAG_SERVER) {
			theApp->serverlist->ProcessUpdate(curTag, NULL, EC_TAG_SERVER);
		} else if (tagname == EC_TAG_FRIEND) {
			theApp->friendlist->ProcessUpdate(curTag, NULL, EC_TAG_FRIEND);
		} else if (tagname == EC_TAG_KNOWNFILE || tagname == EC_TAG_PARTFILE) {
			CEC_SharedFile_Tag *tag = (CEC_SharedFile_Tag *) curTag;
			uint32 id = tag->ID();
			core_files.insert(id);
			if ( m_items_hash.count(id) ) {
				// Item already known: update it
				ProcessItemUpdate(tag, m_items_hash[id]);
			} else {
				CKnownFile * newFile;
				if (tag->GetTagName() == EC_TAG_PARTFILE) {
					CPartFile *file = new CPartFile((CEC_PartFile_Tag *) tag);
					ProcessItemUpdate(tag, file);
					(*theApp->downloadqueue)[id] = file;
					theApp->amuledlg->m_transferwnd->downloadlistctrl->AddFile(file);
					newFile = file;
				} else {
					newFile = new CKnownFile(tag);
					ProcessItemUpdate(tag, newFile);
					(*theApp->sharedfiles)[id] = newFile;
					theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFile(newFile);
				}
				AddItem(newFile);
			}
		}
	}
	// remove items no longer present
	for(iterator it = begin(); it != end();) {
		iterator it2 = it++;
		if (!core_files.count(GetItemID(*it2))) {
			RemoveItem(it2);	// This calls DeleteItem, where it is removed from lists and views.
		}
	}
}

CKnownFilesRem::CKnownFilesRem(CRemoteConnect * conn) : CRemoteContainer<CKnownFile, uint32, CEC_SharedFile_Tag>(conn, true)
{
	requested = 0;
	transferred = 0;
	accepted = 0;
}


/*
 * List of uploading and waiting clients.
 */
CUpDownClientListRem::CUpDownClientListRem(CRemoteConnect *conn)
:
CRemoteContainer<CClientRef, uint32, CEC_UpDownClient_Tag>(conn, true)
{
}


CClientRef::CClientRef(CEC_UpDownClient_Tag *tag)
{
	m_client = new CUpDownClient(tag);
#ifdef DEBUG_ZOMBIE_CLIENTS
	m_client->Link(wxT("TAG"));
#else
	m_client->Link();
#endif
}


CUpDownClient::CUpDownClient(CEC_UpDownClient_Tag *tag) : CECID(tag->ID())
{
	m_linked = 0;
#ifdef DEBUG_ZOMBIE_CLIENTS
	m_linkedDebug = false;
#endif
	// Clients start up empty, then get asked for their data.
	// So all data here is processed in ProcessItemUpdate and thus updatable.
	m_bEmuleProtocol		= false;
	m_AvailPartCount		= 0;
	m_clientSoft			= 0;
	m_nDownloadState		= 0;
	m_Friend				= NULL;
	m_bFriendSlot			= false;
	m_nKadPort				= 0;
	m_kBpsDown				= 0;
	m_dwUserIP				= 0;
	m_lastDownloadingPart	= 0xffff;
	m_nextRequestedPart		= 0xffff;
	m_obfuscationStatus		= 0;
	m_nOldRemoteQueueRank	= 0;
	m_nRemoteQueueRank		= 0;
	m_reqfile				= NULL;
	m_score					= 0;
	m_dwServerIP			= 0;
	m_nServerPort			= 0;
	m_nSourceFrom			= SF_NONE;
	m_nTransferredDown		= 0;
	m_nTransferredUp		= 0;
	m_nUpDatarate			= 0;
	m_uploadingfile			= NULL;
	m_waitingPosition		= 0;
	m_nUploadState			= 0;
	m_nUserIDHybrid			= 0;
	m_nUserPort				= 0;
	m_nClientVersion		= 0;
	m_fNoViewSharedFiles	= false;
	m_identState			= IS_NOTAVAILABLE;
	m_bRemoteQueueFull		= false;

	credits = new CClientCredits(new CreditStruct());
}

#ifdef DEBUG_ZOMBIE_CLIENTS
void CUpDownClient::Unlink(const wxString& from)
{
	std::multiset<wxString>::iterator it = m_linkedFrom.find(from);
	if (it != m_linkedFrom.end()) {
		m_linkedFrom.erase(it);
	}
	m_linked--;
	if (!m_linked) {
		if (m_linkedDebug) {
			AddLogLineN(CFormat(wxT("Last reference to client %d %p unlinked, delete it.")) % ECID() % this);
		}
		delete this;
	}
}

#else

void CUpDownClient::Unlink()
{
	m_linked--;
	if (!m_linked) {
		delete this;
	}
}
#endif


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


CClientRef *CUpDownClientListRem::CreateItem(CEC_UpDownClient_Tag *tag)
{
	CClientRef *client = new CClientRef(tag);
	ProcessItemUpdate(tag, client);
	
	return client;
}


void CUpDownClientListRem::DeleteItem(CClientRef *clientref)
{
	CUpDownClient* client = clientref->GetClient();
	if (client->m_reqfile) {
		client->m_reqfile->DelSource(client);
		client->m_reqfile = NULL;
	}
	Notify_SourceCtrlRemoveSource(client->ECID(), (CPartFile*) NULL);

	if (client->m_uploadingfile) {
		client->m_uploadingfile->RemoveUploadingClient(client);	// this notifies
		client->m_uploadingfile = NULL;
	}
	theApp->m_allUploadingKnownFile->RemoveUploadingClient(client);	// in case it vanished directly while uploading
	Notify_SharedCtrlRemoveClient(client->ECID(), (CKnownFile*) NULL);

	if (client->m_Friend) {
		client->m_Friend->UnLinkClient();	// this notifies
		client->m_Friend = NULL;
	}

#ifdef DEBUG_ZOMBIE_CLIENTS
	if (client->m_linked > 1) {
		AddLogLineC(CFormat(wxT("Client %d still linked in %d places: %s")) % client->ECID() % (client->m_linked - 1) % client->GetLinkedFrom());
		client->m_linkedDebug = true;
	}
#endif

	delete clientref;
}


uint32 CUpDownClientListRem::GetItemID(CClientRef *client)
{
	return client->ECID();
}


void CUpDownClientListRem::ProcessItemUpdate(
	CEC_UpDownClient_Tag *tag,
	CClientRef *clientref)
{
	if (!tag->HasChildTags()) {
		return;		// speed exit for clients without any change
	}
	CUpDownClient *client = clientref->GetClient();

	tag->UserID(&client->m_nUserIDHybrid);
	tag->ClientName(&client->m_Username);
	// Client Software
	bool sw_updated = false;
	if (tag->ClientSoftware(client->m_clientSoft)) {
		client->m_clientSoftString = GetSoftName(client->m_clientSoft);
		sw_updated = true;
	}
	if (tag->SoftVerStr(client->m_clientVerString) || sw_updated) {
		if (client->m_clientSoftString == _("Unknown")) {
			client->m_fullClientVerString = client->m_clientSoftString;
		} else {
			client->m_fullClientVerString = client->m_clientSoftString + wxT(" ") + client->m_clientVerString;
		}
	}
	// User hash
	tag->UserHash(&client->m_UserHash);

	// User IP:Port
	tag->UserIP(client->m_dwUserIP);
	tag->UserPort(&client->m_nUserPort);

	// Server IP:Port
	tag->ServerIP(&client->m_dwServerIP);
	tag->ServerPort(&client->m_nServerPort);
	tag->ServerName(&client->m_ServerName);

	tag->KadPort(client->m_nKadPort);
	tag->FriendSlot(client->m_bFriendSlot);

	tag->GetCurrentIdentState(&client->m_identState);
	tag->ObfuscationStatus(client->m_obfuscationStatus);
	tag->HasExtendedProtocol(&client->m_bEmuleProtocol);

	tag->WaitingPosition(&client->m_waitingPosition);
	tag->RemoteQueueRank(&client->m_nRemoteQueueRank);
	client->m_bRemoteQueueFull = client->m_nRemoteQueueRank == 0xffff;
	tag->OldRemoteQueueRank(&client->m_nOldRemoteQueueRank);
	
	tag->ClientDownloadState(client->m_nDownloadState);
	if (tag->ClientUploadState(client->m_nUploadState)) {
		if (client->m_nUploadState == US_UPLOADING) {
			theApp->m_allUploadingKnownFile->AddUploadingClient(client);
		} else {
			theApp->m_allUploadingKnownFile->RemoveUploadingClient(client);
		}
	}
	
	tag->SpeedUp(&client->m_nUpDatarate);
	if ( client->m_nDownloadState == DS_DOWNLOADING ) {
		tag->SpeedDown(&client->m_kBpsDown);
	} else {
		client->m_kBpsDown = 0;
	}

	//tag->WaitTime(&client->m_WaitTime);
	//tag->XferTime(&client->m_UpStartTimeDelay);
	//tag->LastReqTime(&client->m_dwLastUpRequest);
	//tag->QueueTime(&client->m_WaitStartTime);
	
	CreditStruct *credit_struct =
		(CreditStruct *)client->credits->GetDataStruct();
	tag->XferUp(&credit_struct->uploaded);
	tag->XferUpSession(&client->m_nTransferredUp);

	tag->XferDown(&credit_struct->downloaded);
	tag->XferDownSession(&client->m_nTransferredDown);

	tag->Score(&client->m_score);

	tag->NextRequestedPart(client->m_nextRequestedPart);
	tag->LastDownloadingPart(client->m_lastDownloadingPart);

	uint8 sourceFrom = 0;
	if (tag->GetSourceFrom(sourceFrom)) {
		client->m_nSourceFrom = (ESourceFrom)sourceFrom;
	}

	tag->RemoteFilename(client->m_clientFilename);
	tag->DisableViewShared(client->m_fNoViewSharedFiles);
	tag->Version(client->m_nClientVersion);
	tag->ModVersion(client->m_strModVersion);
	tag->OSInfo(client->m_sClientOSInfo);
	tag->AvailableParts(client->m_AvailPartCount);

	// Download client
	uint32 fileID;
	bool notified = false;
	if (tag->RequestFile(fileID)) {
		if (client->m_reqfile) {
			Notify_SourceCtrlRemoveSource(client->ECID(), client->m_reqfile);
			client->m_reqfile->DelSource(client);
			client->m_reqfile = NULL;
			client->m_downPartStatus.clear();
		}
		CKnownFile * kf = theApp->knownfiles->GetByID(fileID);
		if (kf && kf->IsCPartFile()) {
			client->m_reqfile = (CPartFile *) kf;
			client->m_reqfile->AddSource(client);
			client->m_downPartStatus.setsize(kf->GetPartCount(), 0);
			Notify_SourceCtrlAddSource(client->m_reqfile, CCLIENTREF(client, wxT("AddSource")), A4AF_SOURCE);
			notified = true;
		}
	}

	// Part status
	CECTag * partStatusTag = tag->GetTagByName(EC_TAG_CLIENT_PART_STATUS);
	if (partStatusTag) {
		if (partStatusTag->GetTagDataLen() == 0) {
			// empty tag means full source
			client->m_downPartStatus.SetAllTrue();
		} else if (partStatusTag->GetTagDataLen() == client->m_downPartStatus.SizeBuffer()) {
			client->m_downPartStatus.SetBuffer(partStatusTag->GetTagData());
		}
		notified = false;
	}

	if (!notified && client->m_reqfile && client->m_reqfile->ShowSources()) {
		SourceItemType type;
		switch (client->GetDownloadState()) {
			case DS_DOWNLOADING:
			case DS_ONQUEUE:
				// We will send A4AF, which will be checked.
				type = A4AF_SOURCE;
				break;
			default:
				type = UNAVAILABLE_SOURCE;
				break;
		}
			
		Notify_SourceCtrlUpdateSource(client->ECID(), type);
	}

	// Upload client
	notified = false;
	if (tag->UploadFile(fileID)) {
		if (client->m_uploadingfile) {
			client->m_uploadingfile->RemoveUploadingClient(client);	// this notifies
			notified = true;
			client->m_uploadingfile = NULL;
		}
		CKnownFile * kf = theApp->knownfiles->GetByID(fileID);
		if (kf) {
			client->m_uploadingfile = kf;
			client->m_upPartStatus.setsize(kf->GetPartCount(), 0);
			client->m_uploadingfile->AddUploadingClient(client);	// this notifies
			notified = true;
		}
	}

	// Part status
	partStatusTag = tag->GetTagByName(EC_TAG_CLIENT_UPLOAD_PART_STATUS);
	if (partStatusTag) {
		if (partStatusTag->GetTagDataLen() == client->m_upPartStatus.SizeBuffer()) {
			client->m_upPartStatus.SetBuffer(partStatusTag->GetTagData());
		}
		notified = false;
	}

	if (!notified && client->m_uploadingfile 
		&& (client->m_uploadingfile->ShowPeers() || (client->m_nUploadState == US_UPLOADING))) {
			// notify if KnowFile is selected, or if it's uploading (in case clients are in show uploading mode)
		SourceItemType type;
		switch (client->GetUploadState()) {
			case US_UPLOADING:
			case US_ONUPLOADQUEUE:
				type = AVAILABLE_SOURCE;
				break;
			default:
				type = UNAVAILABLE_SOURCE;
				break;
		}
		Notify_SharedCtrlRefreshClient(client->ECID(), type);
	}
}


/*
 * Download queue container: hold PartFiles with progress status
 * 
 */


bool CDownQueueRem::AddLink(const wxString &link, uint8 cat)
{
	CECPacket req(EC_OP_ADD_LINK);
	CECTag link_tag(EC_TAG_STRING, link);
	link_tag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	req.AddTag(link_tag);
	
	m_conn->SendPacket(&req);
	return true;
}


void CDownQueueRem::ResetCatParts(int cat)
{
	// Called when category is deleted. Command will be performed on the remote side,
	// but files still should be updated here right away, or drawing errors (colour not available)
	// will happen.
	for (iterator it = begin(); it != end(); it++) {
		CPartFile* file = it->second;
		file->RemoveCategory(cat);
	}
}



void CKnownFilesRem::ProcessItemUpdatePartfile(CEC_PartFile_Tag *tag, CPartFile *file)
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
	tag->Shared(&file->m_isShared);
	tag->A4AFAuto(file->m_is_A4AF_auto);

	tag->GetLostDueToCorruption(&file->m_iLostDueToCorruption);
	tag->GetGainDueToCompression(&file->m_iGainDueToCompression);
	tag->TotalPacketsSavedDueToICH(&file->m_iTotalPacketsSavedDueToICH);

	tag->FileCat(&file->m_category);
	
	tag->DownPrio(&file->m_iDownPriorityEC);
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
		PartFileEncoderData &encoder = file->m_PartFileEncoderData;

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
		for (CECTag::const_iterator it = srcnametag->begin(); it != srcnametag->end(); it++) {
			uint32 key = it->GetInt();
			int count = it->GetTagByNameSafe(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS)->GetInt();
			if (count == 0) {
				map.erase(key);
			} else {
				SourcenameItem &item = map[key];
				item.count = count;
				const CECTag *nametag = it->GetTagByName(EC_TAG_PARTFILE_SOURCE_NAMES);
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
		for (CECTag::const_iterator it = commenttag->begin(); it != commenttag->end(); ) {
			wxString u = (it++)->GetStringData();
			wxString f = (it++)->GetStringData();
			int r = (it++)->GetInt();
			wxString c = (it++)->GetStringData();
			file->AddFileRatingList(u, f, r, c);
		}
		file->UpdateFileRatingCommentAvail();
	}
		
	// Update A4AF sources
	ListOfUInts32 & clientIDs = file->GetA4AFClientIDs();
	CECTag *a4aftag = tag->GetTagByName(EC_TAG_PARTFILE_A4AF_SOURCES);
	if (a4aftag) {
		file->ClearA4AFList();
		clientIDs.clear();
		for (CECTag::const_iterator it = a4aftag->begin(); it != a4aftag->end(); it++) {
			if (it->GetTagName() != EC_TAG_ECID) {	// should always be this
				continue;
			}
			uint32 id = it->GetInt();
			CClientRef * src = theApp->clientlist->GetByID(id);
			if (src) {
				file->AddA4AFSource(src->GetClient());
			} else {
				// client wasn't transmitted yet, try it later
				clientIDs.push_back(id);
			}
		}
	} else if (!clientIDs.empty()) {
		// Process clients from the last pass whose ids were still unknown then
		for (ListOfUInts32::iterator it = clientIDs.begin(); it != clientIDs.end(); ) {
			ListOfUInts32::iterator it1 = it++;
			uint32 id = *it1;
			CClientRef * src = theApp->clientlist->GetByID(id);
			if (src) {
				file->AddA4AFSource(src->GetClient());
				clientIDs.erase(it1);
			}
		}
	}
		
	theApp->amuledlg->m_transferwnd->downloadlistctrl->UpdateItem(file);

	// If file is shared check if it is already listed in shared files.
	// If not, add it and show it.
	if (file->IsShared() && !theApp->sharedfiles->count(file->ECID())) {
		(*theApp->sharedfiles)[file->ECID()] = file;
		theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFile(file);
	}
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
	file->SetCategory(cat);
	
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


void CDownQueueRem::ClearCompleted(const ListOfUInts32 & ecids)
{
	CECPacket req(EC_OP_CLEAR_COMPLETED);
	for (ListOfUInts32::const_iterator it = ecids.begin(); it != ecids.end(); it++) {
		req.AddTag(CECTag(EC_TAG_ECID, *it));
	}
	
	m_conn->SendPacket(&req);
}


/*
 * List of friends.
 */
CFriendListRem::CFriendListRem(CRemoteConnect *conn)
:
CRemoteContainer<CFriend, uint32, CEC_Friend_Tag>(conn, true)
{
}


void CFriendListRem::HandlePacket(const CECPacket *)
{
	wxFAIL;		// not needed
}


CFriend * CFriendListRem::CreateItem(CEC_Friend_Tag * tag)
{
	CFriend * Friend = new CFriend(tag->ID());
	ProcessItemUpdate(tag, Friend);
	return Friend;
}


void CFriendListRem::DeleteItem(CFriend * Friend)
{
	Friend->UnLinkClient(false);
	Notify_ChatRemoveFriend(Friend);
}


uint32 CFriendListRem::GetItemID(CFriend * Friend)
{
	return Friend->ECID();
}


void CFriendListRem::ProcessItemUpdate(CEC_Friend_Tag * tag, CFriend * Friend)
{
	if (!tag->HasChildTags()) {
		return;
	}
	tag->Name(Friend->m_strName);
	tag->UserHash(Friend->m_UserHash);
	tag->IP(Friend->m_dwLastUsedIP);
	tag->Port(Friend->m_nLastUsedPort);
	uint32 clientID;
	bool notified = false;
	if (tag->Client(clientID)) {
		if (clientID) {
			CClientRef * client = theApp->clientlist->GetByID(clientID);
			if (client) {
				Friend->LinkClient(*client);	// this notifies
				notified = true;
			}
		} else {
			// Unlink
			Friend->UnLinkClient(false);
		}
	} 
	if (!notified) {
		Notify_ChatUpdateFriend(Friend);
	}
}


void CFriendListRem::AddFriend(const CClientRef& toadd)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECEmptyTag addtag(EC_TAG_FRIEND_ADD);
	addtag.AddTag(CECTag(EC_TAG_CLIENT, toadd.ECID()));
	req.AddTag(addtag);
	
	m_conn->SendPacket(&req);
}


void CFriendListRem::AddFriend(const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort, const wxString& name)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECEmptyTag addtag(EC_TAG_FRIEND_ADD);
	addtag.AddTag(CECTag(EC_TAG_FRIEND_HASH, userhash));
	addtag.AddTag(CECTag(EC_TAG_FRIEND_IP, lastUsedIP));
	addtag.AddTag(CECTag(EC_TAG_FRIEND_PORT, lastUsedPort));
	addtag.AddTag(CECTag(EC_TAG_FRIEND_NAME, name));
	req.AddTag(addtag);
	
	m_conn->SendPacket(&req);
}


void CFriendListRem::RemoveFriend(CFriend* toremove)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECEmptyTag removetag(EC_TAG_FRIEND_REMOVE);
	removetag.AddTag(CECTag(EC_TAG_FRIEND, toremove->ECID()));
	req.AddTag(removetag);
	
	m_conn->SendPacket(&req);
}


void CFriendListRem::SetFriendSlot(CFriend* Friend, bool new_state)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECTag slottag(EC_TAG_FRIEND_FRIENDSLOT, new_state);
	slottag.AddTag(CECTag(EC_TAG_FRIEND, Friend->ECID()));
	req.AddTag(slottag);
	
	m_conn->SendPacket(&req);
}


void CFriendListRem::RequestSharedFileList(CFriend* Friend)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECEmptyTag sharedtag(EC_TAG_FRIEND_SHARED);
	sharedtag.AddTag(CECTag(EC_TAG_FRIEND, Friend->ECID()));
	req.AddTag(sharedtag);
	
	m_conn->SendPacket(&req);
}


void CFriendListRem::RequestSharedFileList(CClientRef& client)
{
	CECPacket req(EC_OP_FRIEND);
	
	CECEmptyTag sharedtag(EC_TAG_FRIEND_SHARED);
	sharedtag.AddTag(CECTag(EC_TAG_CLIENT, client.ECID()));
	req.AddTag(sharedtag);
	
	m_conn->SendPacket(&req);
}



/*
 * Search results
 */
CSearchListRem::CSearchListRem(CRemoteConnect *conn) : CRemoteContainer<CSearchFile, uint32, CEC_SearchFile_Tag>(conn, true)
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


void CSearchListRem::StopSearch(bool)
{
	if (m_curr_search != -1) {
		CECPacket search_req(EC_OP_SEARCH_STOP);
		m_conn->SendPacket(&search_req);
	}
}


void CSearchListRem::HandlePacket(const CECPacket *packet)
{
	if ( packet->GetOpCode() == EC_OP_SEARCH_PROGRESS ) {
		CoreNotify_Search_Update_Progress(packet->GetFirstTagSafe()->GetInt());
	} else {
		CRemoteContainer<CSearchFile, uint32, CEC_SearchFile_Tag>::HandlePacket(packet);
	}
}


CSearchFile::CSearchFile(CEC_SearchFile_Tag *tag)
:
CECID(tag->ID()),
m_parent(NULL),
m_showChildren(false),
m_sourceCount(0),
m_completeSourceCount(0),
m_kademlia(false),
m_downloadStatus(NEW),
m_clientID(0),
m_clientPort(0),
m_kadPublishInfo(0)
{
	SetFileName(CPath(tag->FileName()));
	m_abyFileHash = tag->FileHash();
	SetFileSize(tag->SizeFull());
	
	m_searchID = theApp->searchlist->m_curr_search;
	uint32 parentID = tag->ParentID();
	if (parentID) {
		CSearchFile * parent = theApp->searchlist->GetByID(parentID);
		if (parent) {
			parent->AddChild(this);
		}
	}
}


void CSearchFile::AddChild(CSearchFile* file)
{
	m_children.push_back(file);
	file->m_parent = this;
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


uint32 CSearchListRem::GetItemID(CSearchFile *file)
{
	return file->ECID();
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


void CUpDownClient::RequestSharedFileList()
{
	CClientRef ref = CCLIENTREF(this, wxEmptyString);
	theApp->friendlist->RequestSharedFileList(ref);
}


bool CUpDownClient::SwapToAnotherFile(
	bool WXUNUSED(bIgnoreNoNeeded),
	bool WXUNUSED(ignoreSuspensions),
	bool WXUNUSED(bRemoveCompletely),
	CPartFile* toFile)
{
	CECPacket req(EC_OP_CLIENT_SWAP_TO_ANOTHER_FILE);
	req.AddTag(CECTag(EC_TAG_CLIENT, ECID()));
	req.AddTag(CECTag(EC_TAG_PARTFILE, toFile->GetFileHash()));
	theApp->m_connect->SendPacket(&req);
	
	return true;
}


wxString CAICHHash::GetString() const
{
	return EncodeBase32(m_abyBuffer, HASHSIZE);
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


void CStatTreeRem::DoRequery()
{
	CECPacket request(EC_OP_GET_STATSTREE);
	if (thePrefs::GetMaxClientVersions() != 0) {
		request.AddTag(CECTag(EC_TAG_STATTREE_CAPPING, (uint8)thePrefs::GetMaxClientVersions()));
	}
	m_conn->SendRequest(this, &request);
}

void CStatTreeRem::HandlePacket(const CECPacket * p)
{
	const CECTag* treeRoot = p->GetTagByName(EC_TAG_STATTREE_NODE);
	if (treeRoot) {
		theApp->amuledlg->m_statisticswnd->RebuildStatTreeRemote(treeRoot);
		theApp->amuledlg->m_statisticswnd->ShowStatistics();
	}
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
