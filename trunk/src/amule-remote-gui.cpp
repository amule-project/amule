//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <memory>			// Needed for auto_ptr
using std::auto_ptr;


#include <wx/ipc.h>
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>		// Needed for wxFileConfig

#include <common/Format.h>
#include <common/MD5Sum.h>


#include "amule.h"			// Interface declarations.
#include "Server.h"			// Needed for GetListName
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerWnd.h"			// Needed for CServerWnd
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"
#include "Logger.h"
#include "muuli_wdr.h"			// Needed for IDs
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "SharedFilesCtrl.h"		// Needed for CSharedFilesCtrl
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "ClientListCtrl.h"
#include "ServerListCtrl.h"
#include "ClientCredits.h"
#include "GuiEvents.h"


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

DEFINE_LOCAL_EVENT_TYPE(wxEVT_EC_INIT_DONE);

BEGIN_EVENT_TABLE(CamuleRemoteGuiApp, wxApp)

	// Core timer
	EVT_TIMER(ID_CORETIMER, CamuleRemoteGuiApp::OnPollTimer)

	EVT_CUSTOM(wxEVT_EC_CONNECTION, -1, CamuleRemoteGuiApp::OnECConnection)
	EVT_CUSTOM(wxEVT_EC_INIT_DONE, -1, CamuleRemoteGuiApp::OnECInitDone)
	
	EVT_MULE_NOTIFY(CamuleRemoteGuiApp::OnNotifyEvent)
	EVT_MULE_LOGGING(CamuleRemoteGuiApp::OnLoggingEvent)

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
	
	if ( m_connect->RequestFifoFull() ) {
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
			if ( amuledlg->m_sharedfileswnd->IsShown() ) {
				sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);
			} else if ( amuledlg->m_serverwnd->IsShown() ) {
				//serverlist->FullReload(EC_OP_GET_SERVER_LIST);
			} else if ( amuledlg->m_transferwnd->IsShown() ) {
				downloadqueue->DoRequery(EC_OP_GET_DLOAD_QUEUE, EC_TAG_PARTFILE);
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
			} else if ( amuledlg->m_searchwnd->IsShown() ) {
				if ( searchlist->m_curr_search != -1 ) {
					searchlist->DoRequery(EC_OP_SEARCH_RESULTS, EC_TAG_SEARCHFILE);
				}
			}
			// Back to the roots
			request_step = 0;
			break;
		default:
			printf("WTF?\n");
			request_step = 0;
	}
}


void CamuleRemoteGuiApp::ShutDown(wxCloseEvent &WXUNUSED(evt))
{
	// Stop the Core Timer
	delete poll_timer;
	poll_timer = NULL;
	
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
	
	if ( !wxApp::OnInit() ) {
		return false;
	}

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	// Create the polling timer
	poll_timer = new wxTimer(this,ID_CORETIMER);
	if (!poll_timer) {
		printf("Fatal Error: Failed to create Poll Timer");
		OnExit();
	}

	m_connect = new CRemoteConnect(this);

	SetAppName(wxT("aMule"));
	
	// Load Preferences
	// This creates the CFG file we shall use
	ConfigDir = GetConfigDir();
	if ( !wxDirExists( ConfigDir ) ) {
		wxMkdir( ConfigDir );
	}

	wxConfig::Set(new wxFileConfig(wxEmptyString, wxEmptyString,
		ConfigDir + wxT("remote.conf")));

	glob_prefs = new CPreferencesRem(m_connect);	
	
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));

	bool result = ShowConnectionDialog();

	printf("Going to event loop...\n");
	
	return result;
}

bool CamuleRemoteGuiApp::CryptoAvailable() const
{
	return clientcredits && clientcredits->CryptoAvailable();
}

bool CamuleRemoteGuiApp::ShowConnectionDialog() {
	
	dialog = new CEConnectDlg;

	if ( dialog->ShowModal() != wxID_OK ) {
		dialog->Destroy();
		return false;
	}
		printf("Connecting...\n");
		if ( !m_connect->ConnectToCore(dialog->Host(), dialog->Port(),
							dialog->Login(), dialog->PassHash(),
							wxT("amule-remote"), wxT("0x0001")) ) {
			wxMessageBox(_("Connection failed "),_("Error"),wxOK);
			return false;
		}

	return true;	
}

void CamuleRemoteGuiApp::OnECConnection(wxEvent& event) {
	wxECSocketEvent& evt = *((wxECSocketEvent*)&event);
	printf("Remote GUI EC event handler\n");
	AddLogLineM(true,evt.GetServerReply());
	if (evt.GetResult() == true) {
		// Connected - go to next init step
		glob_prefs->LoadRemote();
	} else {
		printf("Going down\n");
		ExitMainLoop();
	}
}

void CamuleRemoteGuiApp::OnECInitDone(wxEvent& )
{
	Startup();
}


void CamuleRemoteGuiApp::OnLoggingEvent(CLoggingEvent& evt)
{
	printf("LOG: %s\n", unicode2char(evt.Message()).data());
}


void CamuleRemoteGuiApp::OnNotifyEvent(CMuleGUIEvent& evt)
{
	evt.Notify();
}


void CamuleRemoteGuiApp::Startup() {
	
	if ( dialog->SaveUserPass() ) {
		wxConfig::Get()->Write(wxT("/EC/Host" ), dialog->Host());
		wxConfig::Get()->Write(wxT("/EC/Port" ), dialog->Port());
		wxConfig::Get()->Write(wxT("/EC/Password" ), dialog->PassHash());
	}
	dialog->Destroy();
	
	m_ConnState = 0;

	serverconnect = new CServerConnectRem(m_connect);
#warning This is broken, remote-gui will segfault in UpdateStatsTree(). We need real EC code here.
	m_statistics = new CStatistics(*m_connect);
	
	clientlist = new CClientListRem(m_connect);
	searchlist = new CSearchListRem(m_connect);
	serverlist = new CServerListRem(m_connect);
	
	sharedfiles	= new CSharedFilesRem(m_connect);
	knownfiles = new CKnownFilesRem(sharedfiles);

	clientcredits = new CClientCreditsRem();
	
	// bugfix - do this before creating the uploadqueue
	downloadqueue = new CDownQueueRem(m_connect);
	uploadqueue = new CUpQueueRem(m_connect);
	ipfilter = new CIPFilterRem(m_connect);

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

	// Forward wxLog events to CLogger
	wxLog::SetActiveTarget(new CLoggerTarget);
	
	serverlist->FullReload(EC_OP_GET_SERVER_LIST);

	sharedfiles->DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);

	// Start the Poll Timer
	poll_timer->Start(1000);	

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

wxString CamuleRemoteGuiApp::CreateED2kAICHLink(CKnownFile const *)
{
	return wxEmptyString;
}

bool CamuleRemoteGuiApp::AddServer(CServer *, bool)
{
	#warning TODO: Add remote command
	return true;
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

void CamuleRemoteGuiApp::DisconnectED2K() {
	if (IsConnectedED2K()) {
		m_connect->DisconnectED2K();
	}
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
		EC_PREFS_SECURITY | EC_PREFS_CORETWEAKS | EC_PREFS_REMOTECONTROLS;
	m_exchange_recv_selected_prefs = m_exchange_send_selected_prefs | EC_PREFS_CATEGORIES;
}

void CPreferencesRem::HandlePacket(const CECPacket *packet)
{
	((CEC_Prefs_Packet *)packet)->Apply();

	if ( packet->GetTagByName(EC_TAG_PREFS_CATEGORIES) != 0 ) {
		for (int i = 0; i < packet->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagCount(); i++) {
			const CECTag *cat_tag = packet->GetTagByName(EC_TAG_PREFS_CATEGORIES)->GetTagByIndex(i);
			Category_Struct *cat = new Category_Struct;
			cat->title = cat_tag->GetTagByName(EC_TAG_CATEGORY_TITLE)->GetStringData();
			cat->incomingpath = cat_tag->GetTagByName(EC_TAG_CATEGORY_PATH)->GetStringData();
			cat->comment = cat_tag->GetTagByName(EC_TAG_CATEGORY_COMMENT)->GetStringData();
			cat->color =  cat_tag->GetTagByName(EC_TAG_CATEGORY_COLOR)->GetInt();
			cat->prio = cat_tag->GetTagByName(EC_TAG_CATEGORY_PRIO)->GetInt();
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
	wxECInitDoneEvent event;
	theApp.AddPendingEvent(event);
	
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

Category_Struct *CPreferencesRem::CreateCategory(wxString name, wxString path,
			wxString comment, uint32 color, uint8 prio)
{
	CECPacket req(EC_OP_CREATE_CATEGORY);
	CEC_Category_Tag tag(0xffffffff, name, path, comment, color, prio);
	req.AddTag(tag);
	m_conn->SendPacket(&req);

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
	m_conn->SendPacket(&req);

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
	CECPacket stat_req(EC_OP_STAT_REQ);
	m_Conn->SendRequest(this, &stat_req);

	return true;
}
	
void CServerConnectRem::HandlePacket(const CECPacket *packet)
{
	CEC_ConnState_Tag *tag = (CEC_ConnState_Tag *)packet->GetTagByName(EC_TAG_CONNSTATE);
	if (!tag) {
		return;
	}

	theApp.m_ConnState = 0;
	CServer *server;
	m_ID = tag->GetEd2kId();

	if (tag->IsConnectedED2K()) {
		CECTag *srvtag = tag->GetTagByName(EC_TAG_SERVER);
		if ( !srvtag ) {
			return ;
		}
		server = theApp.serverlist->GetByID(srvtag->GetIPv4Data().IP());
		if ( m_CurrServer && (server != m_CurrServer) ) {
			theApp.amuledlg->m_serverwnd->serverlistctrl->HighlightServer(m_CurrServer, false);
		}
		theApp.amuledlg->m_serverwnd->serverlistctrl->HighlightServer(server, true);
		m_CurrServer = server;
		theApp.m_ConnState |= CONNECTED_ED2K;
	} else {
	    	if ( m_CurrServer ) {
	    		theApp.amuledlg->m_serverwnd->serverlistctrl->HighlightServer(m_CurrServer, false);
	    		m_CurrServer = 0;
	    	}
	}

	if (tag->IsConnectedKademlia()) {
		if (tag->IsKadFirewalled()) {
			theApp.m_ConnState |= CONNECTED_KAD_FIREWALLED;
		} else {
			theApp.m_ConnState |= CONNECTED_KAD_OK;
		}
	} else {
		if (tag->IsKadRunning()) {
			theApp.m_ConnState |= CONNECTED_KAD_NOT;
		}
	}
	
	theApp.amuledlg->ShowConnectionState();
}

/*
 * Server list: host list of ed2k servers.
 */
CServerListRem::CServerListRem(CRemoteConnect *conn)
:
CRemoteContainer<CServer, uint32, CEC_Server_Tag>(conn)
{
}

void CServerListRem::HandlePacket(const CECPacket *packet)
{
	CRemoteContainer<CServer, uint32, CEC_Server_Tag>::HandlePacket(packet);
	ReloadControl();
}

void CServerListRem::UpdateServerMetFromURL(wxString WXUNUSED(url))
{
	// FIXME: add command
}

void CServerListRem::SaveServerMet()
{
	// lfroen: stub, nothing to do
}


void CServerListRem::FilterServers()
{
	// FIXME: add code
	//wxASSERT(0);
}


void CServerListRem::RemoveServer(CServer* server)
{
	m_conn->RemoveServer(server->GetIP(),server->GetPort());
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
	theApp.amuledlg->m_serverwnd->serverlistctrl->RemoveServer(srv.get());
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
		theApp.amuledlg->m_serverwnd->serverlistctrl->RefreshServer(srv);
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

void CIPFilterRem::Update(wxString WXUNUSED(url))
{
	// FIXME: add command
	//wxASSERT(0);
}

/*
 * Shared files list
 */
CSharedFilesRem::CSharedFilesRem(CRemoteConnect *conn) : CRemoteContainer<CKnownFile, CMD4Hash, CEC_SharedFile_Tag>(conn, true)
{
	m_rename_file = NULL;
}

void CSharedFilesRem::Reload(bool, bool)
{
	CECPacket req(EC_OP_SHAREDFILES_RELOAD);
	
	m_conn->SendPacket(&req);
}


void CSharedFilesRem::AddFilesFromDirectory(wxString path)
{
	CECPacket req(EC_OP_SHAREDFILES_ADD_DIRECTORY);

	req.AddTag(CECTag(EC_TAG_PREFS_DIRECTORIES, path));
	
	m_conn->SendPacket(&req);
}


bool CSharedFilesRem::RenameFile(CKnownFile* file, const wxString& newName)
{
	CECPacket request(EC_OP_RENAME_FILE);
	request.AddTag(CECTag(EC_TAG_KNOWNFILE, file->GetFileHash()));
	request.AddTag(CECTag(EC_TAG_PARTFILE_NAME, newName));

	m_conn->SendRequest(this, &request);
	m_rename_file = file;
	m_new_name = newName;
	
	return true;
}

void CSharedFilesRem::HandlePacket(const CECPacket *packet)
{
	if ( m_rename_file && (packet->GetOpCode() == EC_OP_NOOP) ) {
		m_rename_file->SetFileName(m_new_name);
		m_rename_file = NULL;
	} else if ( packet->GetOpCode() != EC_OP_FAILED ) {
		CRemoteContainer<CKnownFile, CMD4Hash, CEC_SharedFile_Tag>::HandlePacket(packet);
	}
}

CKnownFile *CSharedFilesRem::CreateItem(CEC_SharedFile_Tag *tag)
{
	CKnownFile *file = new CKnownFile(tag);

	m_enc_map[file->GetFileHash()] = RLE_Data(file->GetPartCount(), true);

	ProcessItemUpdate(tag, file);
	
	theApp.amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFile(file);
	
	return file;
}

void CSharedFilesRem::DeleteItem(CKnownFile *in_file)
{
	auto_ptr<CKnownFile> file(in_file);

	m_enc_map.erase(file->GetFileHash());
	
	theApp.amuledlg->m_sharedfileswnd->sharedfilesctrl->RemoveFile(file.get());
}

CMD4Hash CSharedFilesRem::GetItemID(CKnownFile *file)
{
	return file->GetFileHash();
}

void CSharedFilesRem::ProcessItemUpdate(CEC_SharedFile_Tag *tag, CKnownFile *file)
{
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	const unsigned char *data =
		m_enc_map[file->GetFileHash()].Decode(
			(unsigned char *)parttag->GetTagData(),
			parttag->GetTagDataLen());
	for(int i = 0; i < file->GetPartCount(); ++i) {
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

	theApp.amuledlg->m_sharedfileswnd->sharedfilesctrl->UpdateItem(file);
}

bool CSharedFilesRem::Phase1Done(const CECPacket *)
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
	
	m_conn->SendPacket(&req);
}

/*
 * List of uploading and waiting clients.
 */
CUpDownClientListRem::CUpDownClientListRem(CRemoteConnect *conn, int viewtype) : CRemoteContainer<CUpDownClient, uint32, CEC_UpDownClient_Tag>(conn)
{
	m_viewtype = viewtype;
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
	return credits ? credits->GetScoreRatio(GetIP(), theApp.CryptoAvailable()) : 0;
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
	
	theApp.amuledlg->m_transferwnd->clientlistctrl->InsertClient(client, (ViewType)m_viewtype);
	
	return client;
}

void CUpDownClientListRem::DeleteItem(CUpDownClient *client)
{
	theApp.amuledlg->m_transferwnd->clientlistctrl->RemoveClient(client, (ViewType)m_viewtype);
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
	credit_struct->uploaded = tag->XferUp();
	client->m_nTransferredUp = tag->XferUpSession();

	credit_struct->downloaded = tag->XferDown();
}


CUpQueueRem::CUpQueueRem(CRemoteConnect *conn) : m_up_list(conn, vtUploading), m_wait_list(conn, vtQueued)
{
}

/*
 * Download queue container: hold PartFiles with progress status
 * 
 */
 
CDownQueueRem::CDownQueueRem(CRemoteConnect *conn)
:
CRemoteContainer<CPartFile, CMD4Hash, CEC_PartFile_Tag>(conn, true)
{
}

bool CDownQueueRem::AddED2KLink(const wxString &link, int)
{
	CECPacket req(EC_OP_ED2K_LINK);
	req.AddTag(CECTag(EC_TAG_STRING, link));
	
	m_conn->SendPacket(&req);
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
	
	theApp.amuledlg->m_transferwnd->downloadlistctrl->AddFile(file);
	return file;
}

void CDownQueueRem::DeleteItem(CPartFile *in_file)
{
	auto_ptr<CPartFile> file(in_file);

	theApp.amuledlg->m_transferwnd->downloadlistctrl->RemoveFile(file.get());
	
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
	
		tag->SetLastSeenComplete(file->lastseencomplete);
		
		tag->SetFileCat(file->m_category);
		
		tag->SetPrio(file->m_iDownPriority);
	} else {
		file->kBpsDown = tag->Speed() / 1024.0;
	
		if ( file->kBpsDown > 0 ) {
			file->transfered = tag->SizeXfer();
			file->SetCompletedSize(tag->SizeDone());
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
	file->percentcompleted = (100.0*file->GetCompletedSize()) / file->GetFileSize();
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
		wxASSERT(m_enc_map.count(file->GetFileHash()));
		
		PartFileEncoderData &encoder = m_enc_map[file->GetFileHash()];
		encoder.Decode(
			(unsigned char *)gaptag->GetTagData(), gaptag->GetTagDataLen(),
			(unsigned char *)parttag->GetTagData(), parttag->GetTagDataLen());
			
		const Gap_Struct *reqparts = (const Gap_Struct *)reqtag->GetTagData();
		unsigned reqcount = reqtag->GetTagDataLen() / sizeof(Gap_Struct);
		
		// adjust size of gaplist to reqcount
		unsigned gap_size = encoder.m_gap_status.Size() / (2 * sizeof(uint64));
		while ( file->m_gaplist.size() > gap_size ) {
			file->m_gaplist.pop_front();
		}
		while ( file->m_gaplist.size() != gap_size ) {
			file->m_gaplist.push_front(new Gap_Struct);
		}
		const uint64 *gap_info = (const uint64 *)encoder.m_gap_status.Buffer();
		
		
		std::list<Gap_Struct*>::iterator it = file->m_gaplist.begin();
		for (unsigned j = 0; j < gap_size;j++) {
			Gap_Struct* gap = *it++;
			gap->start = ENDIAN_NTOHLL(gap_info[2*j]);
			gap->end = ENDIAN_NTOHLL(gap_info[2*j+1]);
		}
		
		// adjust size of requested block list
		while ( file->m_requestedblocks_list.size() > reqcount ) {
			file->m_requestedblocks_list.pop_front();
		}
		while ( file->m_requestedblocks_list.size() != reqcount ) {
			file->m_requestedblocks_list.push_front(new Requested_Block_Struct);
		}

		std::list<Requested_Block_Struct*>::iterator it2 = file->m_requestedblocks_list.begin();
		for (unsigned i = 0; i < reqcount;i++) {
			Requested_Block_Struct* block = *it2++;
			block->StartOffset = ENDIAN_NTOHLL(reqparts[i].start);
			block->EndOffset = ENDIAN_NTOHLL(reqparts[i].end);
		}
		// copy parts frequency
		const unsigned char *part_info = encoder.m_part_status.Buffer();
		for(int i = 0;i < file->GetPartCount();i++) {
			file->m_SrcpartFrequency[i] = part_info[i];
		}
	} else {
		printf("ERROR: %p %p %p\n", (void*)gaptag, (void*)parttag, (void*)reqtag);
	}
	theApp.amuledlg->m_transferwnd->downloadlistctrl->UpdateItem(file);
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
	
	m_conn->SendPacket(&req);
}

void CDownQueueRem::AutoPrio(CPartFile *file, bool flag)
{
	CECPacket req(EC_OP_PARTFILE_PRIO_SET);
	
	CECTag hashtag(EC_TAG_PARTFILE, file->GetFileHash());
	
	hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)(flag ? PR_AUTO : file->GetDownPriority())));
	
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
	//wxASSERT(0);
}

CSearchListRem::CSearchListRem(CRemoteConnect *conn) : CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag>(conn)
{
	m_curr_search = -1;
}

wxString CSearchListRem::StartNewSearch(uint32* nSearchID, SearchType search_type, 
	const CSearchList::CSearchParams& params)
{
	CECPacket search_req(EC_OP_SEARCH_START);
	EC_SEARCH_TYPE ec_search_type = EC_SEARCH_LOCAL;
	switch(search_type) {
		case LocalSearch: ec_search_type = EC_SEARCH_LOCAL; break;
		case GlobalSearch: ec_search_type =  EC_SEARCH_GLOBAL; break;
		case KadSearch: ec_search_type =  EC_SEARCH_KAD; break;
	}
	search_req.AddTag(CEC_Search_Tag(params.searchString, ec_search_type,
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

void CSearchListRem::HandlePacket(const CECPacket *packet)
{
	if ( packet->GetOpCode() == EC_OP_SEARCH_PROGRESS ) {
		CoreNotify_Search_Update_Progress(packet->GetTagByIndex(0)->GetInt());
	} else {
		CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag>::HandlePacket(packet);
	}
}

CSearchFile::CSearchFile(CEC_SearchFile_Tag *tag)
	: m_parent(NULL),
	  m_showChildren(false),
	  m_sourceCount(0),
	  m_completeSourceCount(0),
	  m_kademlia(false),
	  m_clientID(0),
	  m_clientPort(0)
{
	SetFileName(tag->FileName());
	m_abyFileHash = tag->ID();
	SetFileSize(tag->SizeFull());
	
	m_searchID = theApp.searchlist->m_curr_search;

}

// dtor is virtual - must be implemented
CSearchFile::~CSearchFile()
{	
}

CSearchFile *CSearchListRem::CreateItem(CEC_SearchFile_Tag *tag)
{
	CSearchFile *file = new CSearchFile(tag);
	ProcessItemUpdate(tag, file);
	
	theApp.amuledlg->m_searchwnd->AddResult(file);
	
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
	file->m_sourceCount = tag->SourceCount();
	file->m_completeSourceCount = tag->CompleteSourceCount();
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
	
	if ( it != m_results.end() ) {
        CSearchResultList& list = it->second;

        for (unsigned int i = 0; i < list.size(); i++) {
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

void CKnownFile::SetFileSize(uint64 nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);
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
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
// File_checked_for_headers
