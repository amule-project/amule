//
// This file is part of the aMule Project
//
// aMule Copyright (C) 2003 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "ExternalConn.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for EncodeBase16
#include "ED2KLink.h"		// Needed for CED2KLink
#include "updownclient.h"	// Needed for CUpDownClient
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "sockets.h"		// Needed for CServerConnect
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "SearchList.h"		// Needed for GetWebList
#include "IPFilter.h"		// Needed for CIPFilter
#include "ClientList.h"
#include "Preferences.h"	// Needed for CPreferences

#ifndef AMULE_DAEMON
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#endif

#include "GuiEvents.h"		// Needed for Notify_* macros
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP()
#include "ECPacket.h"		// Needed for CECPacket, CECTag
#include "ECcodes.h"		// Needed for OPcodes, TAGnames
#include "ECSpecialTags.h"	// Needed for special EC tag creator classes


using namespace otherfunctions;

enum
{	// id for sockets
	SERVER_ID = 1000,
	AUTH_ID,
	SOCKET_ID
};


#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(ExternalConn, wxEvtHandler)
	EVT_SOCKET(SERVER_ID, ExternalConn::OnServerEvent)
	EVT_SOCKET(AUTH_ID,   ExternalConn::OnSocketEvent)
	EVT_SOCKET(SOCKET_ID, ExternalConn::OnSocketEvent)
END_EVENT_TABLE()
#endif


ExternalConn::ExternalConn()
#ifdef AMULE_DAEMON
 : wxThread(wxTHREAD_JOINABLE) 
#endif
{
	m_ECServer = NULL;
	// Are we allowed to accept External Connections?
	if (thePrefs::AcceptExternalConnections() &&
	    thePrefs::ECUseTCPPort()) {
		int port = thePrefs::ECPort();
		// Create the address - listen on localhost:ECPort
		wxIPV4address addr;
		addr.Service(port);
		// Create the socket
#ifdef AMULE_DAEMON
		m_ECServer = new ECSocket(addr, 0);
#else
		m_ECServer = new ECSocket(addr, this, SERVER_ID);
#endif
		if (m_ECServer->Ok()) {
			AddLogLineM(false, wxString::Format(wxT("ECServer listening on port %d"), port));
#ifdef AMULE_DAEMON
			if ( Create() != wxTHREAD_NO_ERROR ) {
				AddLogLineM(false, _("ExternalConn: failed to Create thread"));
				delete m_ECServer;
				// This prevents the destructor to do nasty things
				m_ECServer = NULL;
			} else {
				Run();
			}
#endif
		} else {
			AddLogLineM(false, wxString::Format(wxT("Could not listen for external connections at port %d!"), port));
		}
	} else {
		AddLogLineM(false,_("External connections disabled in config file .eMule"));
	}
}

ExternalConn::~ExternalConn() {
	delete m_ECServer;
}

#ifdef AMULE_DAEMON
void *ExternalConn::Entry()
{
        while ( !TestDestroy() ) {
        	if ( m_ECServer->WaitForAccept(1, 0) ) {
				wxSocketBase *client = m_ECServer->Accept();
				if ( !client ) {
					continue;
				}
				client->Notify(false);
				ExternalConnClientThread *cli_thread = new ExternalConnClientThread(this, client);
				cli_thread->Run();
        	}
	}
	return 0;
}
#else
void ExternalConn::OnServerEvent(wxSocketEvent& WXUNUSED(event)) {
	wxSocketBase *sock;
	// Accept new connection if there is one in the pending
	// connections queue, else exit. We use Accept(FALSE) for
	// non-blocking accept (although if we got here, there
	// should ALWAYS be a pending connection).
	sock = m_ECServer->Accept(false);
	if (sock) {
		AddLogLineM(false, _("New external connection accepted"));
		sock->SetEventHandler(*this, AUTH_ID);
		sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
		sock->Notify(true);
		m_numClients++;
	} else {
		AddLogLineM(false, _("Error: couldn't accept a new external connection"));
	}
}

void ExternalConn::OnSocketEvent(wxSocketEvent& event) {
	wxSocketBase *sock = event.GetSocket();
	CECPacket * request = NULL;
	CECPacket * response = NULL;

	// Now we process the event
	switch(event.GetSocketEvent()) {
	case wxSOCKET_INPUT: {
		// We disable input events, so that the test doesn't trigger
		// wxSocketEvent again.
		sock->SetNotify(wxSOCKET_LOST_FLAG);		
		request = m_ECServer->ReadPacket(sock);		
		if (event.GetId() == AUTH_ID) {
			response = Authenticate(request);
			delete request;	request = NULL;
			m_ECServer->WritePacket(sock, response);
			if (response->GetOpCode() != EC_OP_AUTH_OK) {
				// Access denied!
				AddLogLineM(false, _("Unauthorized access attempt. Connection closed."));
				delete response; response = NULL;
				sock->Destroy();
				return;
			} else {
				// Authenticated => change socket handler
				delete response; response = NULL;
				sock->SetEventHandler(*this, SOCKET_ID);
			}
		} else {
			response = ProcessRequest2(request, m_encoders[sock]);
			delete request; request = NULL;
			m_ECServer->WritePacket(sock, response);
			delete response; response = NULL;
		}		
		// Re-Enable input events again.
		sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
		sock->Notify(true);
		break;
	}
		
	case wxSOCKET_LOST: {
		m_numClients--;
		// Destroy() should be used instead of delete wherever possible,
		// due to the fact that wxSocket uses 'delayed events' (see the
		// documentation for wxPostEvent) and we don't want an event to
		// arrive to the event handler (the frame, here) after the socket
		// has been deleted. Also, we might be doing some other thing with
		// the socket at the same time; for example, we might be in the
		// middle of a test or something. Destroy() takes care of all
		// this for us.
		AddLogLineM(false,_("External connection closed."));
		//sock->Destroy();
		sock->Close();
		
		// remove client data
		m_encoders.erase(sock);
		break;
	}
	
	default: ;
	}
}
#endif

//
// Authentication
//
CECPacket *ExternalConn::Authenticate(const CECPacket *request)
{
	CECPacket *response;

	if (request == NULL) {
		response = new CECPacket(EC_OP_AUTH_FAIL);
		return response;
	}

	if (request->GetOpCode() == EC_OP_AUTH_REQ) {
		CECTag *clientName = request->GetTagByName(EC_TAG_CLIENT_NAME);
		AddLogLineM(false, _("Connecting client: ") + ((clientName == NULL) ? wxString(_("Unknown")) : clientName->GetStringData()));
		CECTag *passwd = request->GetTagByName(EC_TAG_PASSWD_HASH);
		CECTag *protocol = request->GetTagByName(EC_TAG_PROTOCOL_VERSION);
		if (protocol != NULL) {
			uint16 proto_version = protocol->GetInt16Data();
			if (proto_version == 0x0200) {
				if (passwd == NULL) {
					if (thePrefs::ECPassword().IsEmpty()) {
						response = new CECPacket(EC_OP_AUTH_OK);
					} else {
						response = new CECPacket(EC_OP_AUTH_FAIL);
						response->AddTag(CECTag(EC_TAG_STRING, _("Authentication failed.")));
					}
				} else if (passwd->GetStringData() == thePrefs::ECPassword()) {
					response = new CECPacket(EC_OP_AUTH_OK);
				} else {
					response = new CECPacket(EC_OP_AUTH_FAIL);
					response->AddTag(CECTag(EC_TAG_STRING, _("Authentication failed.")));
				}
			} else {
				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, _("Invalid protocol version.")));
			}
		} else {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, _("Missing protocol version tag.")));
		}
	} else {
		response = new CECPacket(EC_OP_AUTH_FAIL);
		response->AddTag(CECTag(EC_TAG_STRING, _("Invalid request, you should first authenticate.")));
	}

	if (response->GetOpCode() == EC_OP_AUTH_OK) {
		AddLogLineM(false, _("Access granted."));
	} else {
		AddLogLineM(false, response->GetTagByIndex(0)->GetStringData());
	}

	return response;
}

CECPacket *Get_EC_Response_StatRequest(const CECPacket *request)
{
	wxASSERT(request->GetOpCode() == EC_OP_STAT_REQ);
	
	CECPacket *response = new CECPacket(EC_OP_STATS);

	//
	// ul/dl speeds
	response->AddTag(CECTag(EC_TAG_STATS_UL_SPEED, (uint32)(theApp.uploadqueue->GetKBps()*1024.0)));
	response->AddTag(CECTag(EC_TAG_STATS_DL_SPEED, (uint32)(theApp.downloadqueue->GetKBps()*1024.0)));
	response->AddTag(CECTag(EC_TAG_STATS_UL_SPEED_LIMIT, (uint32)(thePrefs::GetMaxUpload()*1024.0)));
	response->AddTag(CECTag(EC_TAG_STATS_DL_SPEED_LIMIT, (uint32)(thePrefs::GetMaxDownload()*1024.0)));
	
	response->AddTag(CECTag(EC_TAG_STATS_CURR_UL_COUNT,
		(uint32)theApp.uploadqueue->GetUploadQueueLength()));
	// get the source count
	uint32 stats[2];
	theApp.downloadqueue->GetDownloadStats(stats);
	response->AddTag(CECTag(EC_TAG_STATS_TOTAL_SRC_COUNT, stats[0]));
	response->AddTag(CECTag(EC_TAG_STATS_CURR_DL_COUNT, stats[1]));
	response->AddTag(CECTag(EC_TAG_STATS_TOTAL_DL_COUNT,
		(uint32)theApp.downloadqueue->GetFileCount()));
	response->AddTag(CECTag(EC_TAG_STATS_UL_QUEUE_LEN,
		(uint32)theApp.uploadqueue->GetWaitingUserCount()));

	return response;
}

CECPacket *Get_EC_Response_GetUpQueue(const CECPacket *request)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_ULOAD_QUEUE);
	
	CECPacket *response = new CECPacket(EC_OP_ULOAD_QUEUE);
	
	POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
	while (	pos ) {

		CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
		theApp.uploadqueue->GetNextFromUploadList(pos);
		if (!cur_client) {
			continue;
		}
		CECTag cli_tag(EC_TAG_UPDOWN_CLIENT, cur_client->GetUserName());
		cli_tag.AddTag(CECTag(EC_TAG_ITEM_ID, PTR_2_ID(cur_client)));
		CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
		if (file) {
			cli_tag.AddTag(CECTag(EC_TAG_PARTFILE, file->GetFileName()));
		} else {
			cli_tag.AddTag(CECTag(EC_TAG_PARTFILE, wxString(wxT("?"))));
		}
		cli_tag.AddTag(CECTag(EC_TAG_PARTFILE_SIZE_XFER, (uint32)cur_client->GetTransferedUp()));
		cli_tag.AddTag(CECTag(EC_TAG_PARTFILE_SPEED, (uint32)(cur_client->GetKBpsUp()*1024.0)));
		response->AddTag(cli_tag);
	}
	
	return response;
}	

CECPacket *Get_EC_Response_GetDownloadQueue(const CECPacket *request,
	CPartFile_Encoder_Map &encoders)
{	
	CECPacket *response = new CECPacket(EC_OP_DLOAD_QUEUE);
	
	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	std::set<uint32> queryitems;
	for (int i = 0;i < request->GetTagCount();i++) {
		CECTag *tag = request->GetTagByIndex(i);
		if ( tag->GetTagName() == EC_TAG_PARTFILE ) {
			queryitems.insert(tag->GetInt32Data());
		}
	}
	
	// check if encoder contains files that no longer in download queue
	if ( encoders.size() > theApp.downloadqueue->GetFileCount() ) {
		std::set<CPartFile *> curr_files, dead_files;
		for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); i++) {
			curr_files.insert(theApp.downloadqueue->GetFileByIndex(i));
		}
		for(CPartFile_Encoder_Map::iterator i = encoders.begin(); i != encoders.end(); i++) {
			if ( curr_files.count(i->first) == 0 ) {
				dead_files.insert(i->first);
			}
		}
		for(std::set<CPartFile *>::iterator i = dead_files.begin(); i != dead_files.end(); i++) {
			encoders.erase(*i);
		}
	} else if ( encoders.size() < theApp.downloadqueue->GetFileCount() ) {
		for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); i++) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
			if ( encoders.count(cur_file) == 0 ) {
				encoders[cur_file] = CPartFile_Encoder(cur_file);
			}
		}
	}
	for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
	
		if ( !queryitems.empty() && !queryitems.count(PTR_2_ID(cur_file)) ) {
			continue;
		}

		CEC_PartFile_Tag filetag(cur_file, detail_level);
		
		CPartFile_Encoder &enc = encoders[cur_file];
//		CECTag *etag = encoders[cur_file].Encode();
		CECTag *etag = enc.Encode();
		filetag.AddTag(etag);

		response->AddTag(filetag);
	}

	return 	response;
}


CECPacket *Get_EC_Response_PartFile_Cmd(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);

	CPartFile *pfile = 0;
	CECTag *idtag = request->GetTagByIndex(0);
	CECTag *valtag = request->GetTagByIndex(1);

	wxASSERT(idtag->GetTagName() == EC_TAG_ITEM_ID);

	uint32 id = idtag->GetInt32Data();
	for (unsigned int j = 0; j < theApp.downloadqueue->GetFileCount(); j++) {
		CPartFile *curr_file = theApp.downloadqueue->GetFileByIndex(j);
		if ( PTR_2_ID(curr_file) == id ) {
			pfile = curr_file;
			break;
		}
	}
	if ( !pfile ) {
		AddLogLineM(false,_("Remote PartFile command failed: id not found"));
		response->AddTag(CECTag(EC_TAG_STRING, _("ERROR: id not found")));
		return response;
	}
	switch (request->GetOpCode()) {
		case EC_OP_PARTFILE_REMOVE_NO_NEEDED:
			pfile->CleanUpSources(true,  false, false);
			break;
		case EC_OP_PARTFILE_REMOVE_FULL_QUEUE:
			pfile->CleanUpSources(false, true, false);
			break;
		case EC_OP_PARTFILE_REMOVE_HIGH_QUEUE:
			pfile->CleanUpSources(false, false, true);
			break;
		case EC_OP_PARTFILE_CLEANUP_SOURCES:
			pfile->CleanUpSources(true, true, true);
			break;
		case EC_OP_PARTFILE_SWAP_A4AF_THIS:
			if ((pfile->GetStatus(false) == PS_READY) ||
			    (pfile->GetStatus(false) == PS_EMPTY)) {
				theApp.downloadqueue->DisableAllA4AFAuto();
				
				CPartFile::SourceSet::iterator it = pfile->A4AFsrclist.begin();
				while ( it != pfile->A4AFsrclist.end() ) {
					CUpDownClient *cur_source = *it++;
					if ((cur_source->GetDownloadState() != DS_DOWNLOADING) &&
					    cur_source->GetRequestFile() &&
					    ( (!cur_source->GetRequestFile()->IsA4AFAuto()) ||
					      (cur_source->GetDownloadState() == DS_NONEEDEDPARTS))) {
						cur_source->SwapToAnotherFile(true, false, false, pfile);
					}
				}
			}
			break;
		case EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO:
			pfile->SetA4AFAuto(!pfile->IsA4AFAuto());
			break;
		case EC_OP_PARTFILE_SWAP_A4AF_OTHERS:
			if ((pfile->GetStatus(false) == PS_READY) ||
			    (pfile->GetStatus(false) == PS_EMPTY)) {
				theApp.downloadqueue->DisableAllA4AFAuto();
				
				CPartFile::SourceSet::iterator it = pfile->m_SrcList.begin();
				while ( it != pfile->m_SrcList.end() ) {
					CUpDownClient* cur_source = *it++;
					
					cur_source->SwapToAnotherFile(false, false, false, NULL);
				}
			}
			break;
		case EC_OP_PARTFILE_PAUSE:
			pfile->PauseFile();
			break;
		case EC_OP_PARTFILE_RESUME:
			pfile->ResumeFile();
			pfile->SavePartFile();
			break;
		case EC_OP_PARTFILE_STOP:
			pfile->StopFile();
			break;
		case EC_OP_PARTFILE_PRIO_AUTO:
			if ( !valtag ) {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: no value tag in EC_OP_PARTFILE_PRIO_AUTO")));
				return response;
			}
			if ( valtag->GetStringData() == wxT("1") ) {
				pfile->SetAutoDownPriority(true);
			} else if ( valtag->GetStringData() == wxT("0") ) {
				pfile->SetAutoDownPriority(false);
			} else {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: value for EC_OP_PARTFILE_PRIO_AUTO is bad")));
				return response;
			}
			break;
		case EC_OP_PARTFILE_PRIO_SET:
			if ( !valtag ) {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: no value tag in EC_OP_PARTFILE_PRIO_SET")));
				return response;
			}
			if ( valtag->GetStringData() == wxT("PR_LOW") ) {
				pfile->SetDownPriority(PR_LOW);
			} else if ( valtag->GetStringData() == wxT("PR_NORMAL") ) {
				pfile->SetDownPriority(PR_NORMAL);
			} else if ( valtag->GetStringData() == wxT("PR_HIGH") ) {
				pfile->SetDownPriority(PR_HIGH);
			} else {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: value for EC_OP_PARTFILE_PRIO_SET is bad")));
				return response;
			}
			break;
		case EC_OP_PARTFILE_DELETE:
			if ( thePrefs::StartNextFile() && (pfile->GetStatus() == PS_PAUSED) ) {
				theApp.downloadqueue->StartNextFile();
			}
			pfile->Delete();
			break;
		case EC_OP_PARTFILE_SET_CAT:
			if ( !valtag ) {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: no value tag in EC_OP_PARTFILE_CAT_SET")));
				return response;
			}
			break;
	}

	response->AddTag(CECTag(EC_TAG_STRING, _("OK: PartFile command dispatched")));
	return response;
}

CECPacket *Get_EC_Response_Server(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);
	CECTag *srv_tag = request->GetTagByIndex(0);
	CServer *srv = 0;
	if ( srv_tag ) {
		uint32 srv_ip = srv_tag->GetIPv4Data().IP();
		for(uint32 i = 0; i < theApp.serverlist->GetServerCount(); i++) {
			CServer *curr_srv = theApp.serverlist->GetServerAt(i);
			// lfroen: never saw 2 servers on same IP !
			if ( curr_srv->GetIP() == srv_ip) {
				srv = curr_srv;
				break;
			}
		}
		// server tag passed, but server not found
		if ( !srv ) {
			response->AddTag(CECTag(EC_TAG_STRING,
						wxString(_("ERROR: server not found: ")) + srv_tag->GetIPv4Data().StringIP()));
			return response;
		}
	}
	switch (request->GetOpCode()) {
		case EC_OP_SERVER_DISCONNECT:
			theApp.serverconnect->Disconnect();
			response->AddTag(CECTag(EC_TAG_STRING,_("OK: disconnected from server")));
			break;
		case EC_OP_SERVER_REMOVE:
			if ( srv ) {
				Notify_ServerRemove(srv);
				theApp.serverlist->RemoveServer(srv);
				response->AddTag(CECTag(EC_TAG_STRING, _("OK: server removed")));
			} else {
				response->AddTag(CECTag(EC_TAG_STRING,
							_("ERROR: need to define server to be removed")));
			}
			break;
		case EC_OP_SERVER_CONNECT:
			if ( srv ) {
				theApp.serverconnect->ConnectToServer(srv);
				response->AddTag(CECTag(EC_TAG_STRING, _("OK: trying to connect")));
			} else {
				theApp.serverconnect->ConnectToAnyServer();
				response->AddTag(CECTag(EC_TAG_STRING, _("OK: connecting to a random server")));
			}
			break;
	}	
	return response;
}

CECPacket *ProcessPreferencesRequest(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_PREFERENCES);
	uint32 selection = 0;
	CECTag *selTag = request->GetTagByName(EC_TAG_SELECT_PREFS);
	if (selTag) {
		selection = selTag->GetInt32Data();
	}

	if (selection & EC_PREFS_CATEGORIES) {
		if (theApp.glob_prefs->GetCatCount() > 1) {
			CECEmptyTag cats(EC_TAG_PREFS_CATEGORIES);
			EC_DETAIL_LEVEL dl = request->GetDetailLevel();
			for (unsigned int i = 0; i < theApp.glob_prefs->GetCatCount(); ++i) {
				Category_Struct *cat = theApp.glob_prefs->GetCategory(i);
				CECTag catTag(EC_TAG_CATEGORY, (uint32)i);
				switch (dl) {
					case EC_DETAIL_UPDATE:
					case EC_DETAIL_GUI:
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_PATH, cat->incomingpath));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_COMMENT, cat->comment));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_COLOR, (uint32)cat->color));
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_PRIO, cat->prio));
					case EC_DETAIL_WEB:
					case EC_DETAIL_CMD:
						catTag.AddTag(CECTag(EC_TAG_CATEGORY_TITLE, cat->title));
				}
				cats.AddTag(catTag);
			}
			response->AddTag(cats);
		}
	}

	if (selection & EC_PREFS_GENERAL) {
		#warning TODO
	}

	if (selection & EC_PREFS_CONNECTIONS) {
		CECEmptyTag connPrefs(EC_TAG_PREFS_CONNECTIONS);
		connPrefs.AddTag(CECTag(EC_TAG_CONN_UL_CAP, thePrefs::GetMaxGraphUploadRate()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_DL_CAP, thePrefs::GetMaxGraphDownloadRate()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_UL, thePrefs::GetMaxUpload()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_DL, thePrefs::GetMaxDownload()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_SLOT_ALLOCATION, thePrefs::GetSlotAllocation()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_TCP_PORT, thePrefs::GetPort()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_UDP_PORT, thePrefs::GetUDPPort()));
		if (thePrefs::IsUDPDisabled()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_UDP_DISABLE));
		}
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_FILE_SOURCES, thePrefs::GetMaxSourcePerFile()));
		connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_CONN, thePrefs::GetMaxConnections()));
		if (thePrefs::DoAutoConnect()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_AUTOCONNECT));
		}
		if (thePrefs::Reconnect()) {
			connPrefs.AddTag(CECEmptyTag(EC_TAG_CONN_RECONNECT));
		}
		response->AddTag(connPrefs);
	}

	if (selection & EC_PREFS_MESSAGEFILTER) {
		#warning TODO
	}

	if (selection & EC_PREFS_REMOTECONTROLS) {
		CECEmptyTag rc_prefs(EC_TAG_PREFS_REMOTECTRL);

		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_PORT, thePrefs::GetWSPort()));
		if (!thePrefs::GetWSPass().IsEmpty()) {
			rc_prefs.AddTag(CECTag(EC_TAG_PASSWD_HASH, thePrefs::GetWSPass()));
		}
		if (thePrefs::GetWSIsLowUserEnabled()) {
			CECEmptyTag lowUser(EC_TAG_WEBSERVER_GUEST);
			if (!thePrefs::GetWSLowPass().IsEmpty()) {
				lowUser.AddTag(CECTag(EC_TAG_PASSWD_HASH, thePrefs::GetWSLowPass()));
			}
			rc_prefs.AddTag(lowUser);
		}
		if (thePrefs::GetWebUseGzip()) {
			rc_prefs.AddTag(CECEmptyTag(EC_TAG_WEBSERVER_USEGZIP));
		}
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_REFRESH, thePrefs::GetWebPageRefresh()));
		response->AddTag(rc_prefs);
	}

	if (selection & EC_PREFS_ONLINESIG) {
		#warning TODO
	}

	if (selection & EC_PREFS_SERVERS) {
		#warning TODO
	}

	if (selection & EC_PREFS_FILES) {
		CECEmptyTag filePrefs(EC_TAG_PREFS_FILES);
		if (thePrefs::IsICHEnabled()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ICH_ENABLED));
		}
		if (thePrefs::IsTrustingEveryHash()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_AICH_TRUST));
		}
		if (thePrefs::AddNewFilesPaused()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_PAUSED));
		}
		if (thePrefs::GetNewAutoDown()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_AUTO_DL_PRIO));
		}
		if (thePrefs::GetPreviewPrio()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_PREVIEW_PRIO));
		}
		if (thePrefs::GetNewAutoUp()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_NEW_AUTO_UL_PRIO));
		}
		if (thePrefs::TransferFullChunks()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_UL_FULL_CHUNKS));
		}
		if (thePrefs::StartNextFile()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_START_NEXT_PAUSED));
		}
		if (thePrefs::GetSrcSeedsOn()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_SAVE_SOURCES));
		}
		if (thePrefs::GetExtractMetaData()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_EXTRACT_METADATA));
		}
		if (thePrefs::GetAllocFullChunk()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ALLOC_FULL_CHUNKS));
		}
		if (thePrefs::GetAllocFullPart()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_ALLOC_FULL_SIZE));
		}
		if (thePrefs::IsCheckDiskspaceEnabled()) {
			filePrefs.AddTag(CECEmptyTag(EC_TAG_FILES_CHECK_FREE_SPACE));
			filePrefs.AddTag(CECTag(EC_TAG_FILES_MIN_FREE_SPACE, thePrefs::GetMinFreeDiskSpace()));
		}
		response->AddTag(filePrefs);
	}

	if (selection & EC_PREFS_SRCDROP) {
		#warning TODO
	}

	if (selection & EC_PREFS_DIRECTORIES) {
		#warning TODO
	}

	if (selection & EC_PREFS_STATISTICS) {
		#warning TODO
	}

	if (selection & EC_PREFS_SECURITY) {
		CECEmptyTag secPrefs(EC_TAG_PREFS_SECURITY);
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_CAN_SEE_SHARES, thePrefs::CanSeeShares()));
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_FILE_PERMISSIONS, (uint32)thePrefs::GetFilePermissions()));
		secPrefs.AddTag(CECTag(EC_TAG_SECURITY_DIR_PERMISSIONS, (uint32)thePrefs::GetDirPermissions()));
		if (thePrefs::GetIPFilterOn()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_ENABLED));
		}
		if (thePrefs::IPFilterAutoLoad()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_AUTO_UPDATE));
		}
		secPrefs.AddTag(CECTag(EC_TAG_IPFILTER_UPDATE_URL, thePrefs::IPFilterURL()));
		secPrefs.AddTag(CECTag(EC_TAG_IPFILTER_LEVEL, thePrefs::GetIPFilterLevel()));
		if (thePrefs::FilterBadIPs()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_IPFILTER_FILTER_BAD));
		}
		if (thePrefs::IsSecureIdentEnabled()) {
			secPrefs.AddTag(CECEmptyTag(EC_TAG_SECURITY_USE_SECIDENT));
		}
		response->AddTag(secPrefs);
	}

	if (selection & EC_PREFS_CORETWEAKS) {
		CECEmptyTag cwPrefs(EC_TAG_PREFS_CORETWEAKS);
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_MAX_CONN_PER_FIVE, thePrefs::GetMaxConperFive()));
		if (thePrefs::GetSafeMaxConn()) {
			cwPrefs.AddTag(CECEmptyTag(EC_TAG_CORETW_SAFE_MAXCONN));
		}
		if (thePrefs::GetVerbose()) {
			cwPrefs.AddTag(CECEmptyTag(EC_TAG_CORETW_VERBOSE));
		}
		if (thePrefs::GetVerbosePacketError()) {
			cwPrefs.AddTag(CECEmptyTag(EC_TAG_CORETW_VERBOSE_PACKET));
		}
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_FILEBUFFER, thePrefs::GetFileBufferSize()));
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_UL_QUEUE, thePrefs::GetQueueSize()));
		cwPrefs.AddTag(CECTag(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT, thePrefs::GetServerKeepAliveTimeout()));
		response->AddTag(cwPrefs);
	}

	return response;
}

CECPacket *SetPreferencesFromRequest(const CECPacket *request)
{
	CECTag * thisTab;
	CECTag * oneTag;

	/* Maybe we'll have another way to change categories
	if ((thisTab =request->GetTagByName(EC_TAG_PREFS_CATEGORIES)) != NULL) {
	}
	*/

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_GENERAL)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_CONNECTIONS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UL_CAP)) != NULL) {
			thePrefs::SetMaxGraphUploadRate(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_DL_CAP)) != NULL) {
			thePrefs::SetMaxGraphDownloadRate(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_UL)) != NULL) {
			thePrefs::SetMaxUpload(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_DL)) != NULL) {
			thePrefs::SetMaxDownload(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_SLOT_ALLOCATION)) != NULL) {
			thePrefs::SetSlotAllocation(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_TCP_PORT)) != NULL) {
			thePrefs::SetPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UDP_PORT)) != NULL) {
			thePrefs::SetUDPPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_UDP_DISABLE)) != NULL) {
			thePrefs::SetUDPDisable(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_FILE_SOURCES)) != NULL) {
			thePrefs::SetMaxSourcesPerFile(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_MAX_CONN)) != NULL) {
			thePrefs::SetMaxConnections(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_AUTOCONNECT)) != NULL) {
			thePrefs::SetAutoConnect(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CONN_RECONNECT)) != NULL) {
			thePrefs::SetReconnect(oneTag->GetInt8Data() != 0);
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_MESSAGEFILTER)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_REMOTECTRL)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_PORT)) != NULL) {
			thePrefs::SetWSPort(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
			thePrefs::SetWSPass(oneTag->GetStringData());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_GUEST)) != NULL) {
			thePrefs::SetWSIsLowUserEnabled(oneTag->GetInt8Data() != 0);
			if ((oneTag->GetTagByName(EC_TAG_PASSWD_HASH)) != NULL) {
				thePrefs::SetWSLowPass(oneTag->GetTagByName(EC_TAG_PASSWD_HASH)->GetStringData());
			}
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_USEGZIP)) != NULL) {
			thePrefs::SetWebUseGzip(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_REFRESH)) != NULL) {
			thePrefs::SetWebPageRefresh(oneTag->GetInt32Data());
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_ONLINESIG)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_SERVERS)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_FILES)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_ICH_ENABLED)) != NULL) {
			thePrefs::SetICHEnabled(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_AICH_TRUST)) != NULL) {
			thePrefs::SetTrustingEveryHash(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_NEW_PAUSED)) != NULL) {
			thePrefs::SetAddNewFilesPaused(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_NEW_AUTO_DL_PRIO)) != NULL) {
			thePrefs::SetNewAutoDown(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_PREVIEW_PRIO)) != NULL) {
			thePrefs::SetPreviewPrio(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_NEW_AUTO_UL_PRIO)) != NULL) {
			thePrefs::SetNewAutoUp(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_UL_FULL_CHUNKS)) != NULL) {
			thePrefs::SetTransferFullChunks(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_START_NEXT_PAUSED)) != NULL) {
			thePrefs::SetStartNextFile(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_SAVE_SOURCES)) != NULL) {
			thePrefs::SetSrcSeedsOn(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_EXTRACT_METADATA)) != NULL) {
			thePrefs::SetExtractMetaData(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_ALLOC_FULL_CHUNKS)) != NULL) {
			thePrefs::SetAllocFullChunk(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_ALLOC_FULL_SIZE)) != NULL) {
			thePrefs::SetAllocFullPart(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_CHECK_FREE_SPACE)) != NULL) {
			thePrefs::SetCheckDiskspaceEnabled(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_FILES_MIN_FREE_SPACE)) != NULL) {
			thePrefs::SetMinFreeDiskSpace(oneTag->GetInt32Data());
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_SRCDROP)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_DIRECTORIES)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_STATISTICS)) != NULL) {
		#warning TODO
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_SECURITY)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_CAN_SEE_SHARES)) != NULL) {
			thePrefs::SetCanSeeShares(oneTag->GetInt8Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_FILE_PERMISSIONS)) != NULL) {
			thePrefs::SetFilePermissions(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_DIR_PERMISSIONS)) != NULL) {
			thePrefs::SetDirPermissions(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_ENABLED)) != NULL) {
			thePrefs::SetIPFilterOn(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_AUTO_UPDATE)) != NULL) {
			thePrefs::SetIPFilterAutoLoad(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_UPDATE_URL)) != NULL) {
			thePrefs::SetIPFilterURL(oneTag->GetStringData());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_LEVEL)) != NULL) {
			thePrefs::SetIPFilterLevel(oneTag->GetInt8Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_IPFILTER_FILTER_BAD)) != NULL) {
			thePrefs::SetFilterBadIPs(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SECURITY_USE_SECIDENT)) != NULL) {
			thePrefs::SetSecureIdentEnabled(oneTag->GetInt8Data() != 0);
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_CORETWEAKS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_MAX_CONN_PER_FIVE)) != NULL) {
			thePrefs::SetMaxConsPerFive(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_SAFE_MAXCONN)) != NULL) {
			thePrefs::SetSafeMaxConn(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_VERBOSE)) != NULL) {
			thePrefs::SetVerbose(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_VERBOSE_PACKET)) != NULL) {
			thePrefs::SetVerbosePacketError(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_FILEBUFFER)) != NULL) {
			thePrefs::SetFileBufferSize(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_UL_QUEUE)) != NULL) {
			thePrefs::SetQueueSize(oneTag->GetInt32Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT)) != NULL) {
			thePrefs::SetServerKeepAliveTimeout(oneTag->GetInt32Data());
		}
	}

	CECPacket *response = new CECPacket(EC_OP_NOOP);
	return response;
}

// init with some default size
uint32 *CPartFile_Encoder::m_gap_buffer = new uint32[128];
int CPartFile_Encoder::m_gap_buffer_size = 128;

// encoder side
CPartFile_Encoder::CPartFile_Encoder(CPartFile *file) :
	m_enc_data(file->GetPartCount(), file->gaplist.GetCount()*2)
{
	m_file = file;
}

// decoder side
CPartFile_Encoder::CPartFile_Encoder(int size): m_enc_data(size, 0)
{
	m_file = 0;
}

CPartFile_Encoder::~CPartFile_Encoder()
{
}
		
// stl side :)
CPartFile_Encoder::CPartFile_Encoder()
{
	m_file = 0;
}

CPartFile_Encoder::CPartFile_Encoder(const CPartFile_Encoder &obj) : m_enc_data(obj.m_enc_data)
{
	m_file = obj.m_file;
}

CPartFile_Encoder &CPartFile_Encoder::operator=(const CPartFile_Encoder &obj)
{
	m_file = obj.m_file;
	m_enc_data = obj.m_enc_data;
	return *this;
}


CECTag *CPartFile_Encoder::Encode()
{
	if ( m_gap_buffer_size < m_file->gaplist.GetCount()*2 ) {
		m_gap_buffer_size = m_file->gaplist.GetCount()*2;
		uint32 *buf = new uint32[m_gap_buffer_size];
		delete [] m_gap_buffer;
		m_gap_buffer = buf;
	} 
	
	POSITION curr_pos = m_file->gaplist.GetHeadPosition();
	uint32 *gap_buff_ptr = m_gap_buffer;
	while ( curr_pos ) {
		Gap_Struct *curr = m_file->gaplist.GetNext(curr_pos);
		*gap_buff_ptr++ = curr->start;
		*gap_buff_ptr++ = curr->end;
	}

	m_enc_data.m_gap_status.Realloc(m_gap_buffer_size);
	int gap_enc_size = 0;
	const unsigned char *gap_enc_data = m_enc_data.m_gap_status.Encode((unsigned char *)m_gap_buffer, gap_enc_size);
	
	int part_enc_size;
	const unsigned char *part_enc_data = m_enc_data.m_part_status.Encode(m_file->m_SrcpartFrequency, part_enc_size);

	//
	// Put data inside of tag in following order:
	// [size of part_enc_data] [part_enc_data] [num_of_gaps] [gap_enc_data]
	//
	unsigned char *tagdata;
	CECTag *etag = new CECTag(EC_TAG_PARTFILE_GAP_STATUS,
		2 * sizeof(uint32) + part_enc_size + gap_enc_size, (void **)&tagdata);

	// size of gap RLE data
	*((uint32 *)tagdata) = part_enc_size;
	tagdata += sizeof(uint32);
	// part data itself
	memcpy(tagdata, part_enc_data, part_enc_size);
	tagdata += part_enc_size;

	// real number of gaps - so remote size can realloc
	*((uint32 *)tagdata) = m_gap_buffer_size;
	tagdata += sizeof(uint32);
	memcpy(tagdata, gap_enc_data, gap_enc_size);

	return etag;
}

CECPacket *ExternalConn::ProcessRequest2(const CECPacket *request, CPartFile_Encoder_Map &enc_map)
{

	if ( !request ) {
		return 0;
	}

	CECPacket *response = NULL;

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();

	switch (request->GetOpCode()) {
		//
		// Misc commands
		//
		case EC_OP_COMPAT: 
			response = new CECPacket(EC_OP_COMPAT);
			response->AddTag(CECTag(EC_TAG_STRING,
				ProcessRequest(request->GetTagByIndex(0)->GetStringData())));
			break;
		case EC_OP_SHUTDOWN:
			AddLogLineM(true, _("ExternalConn: shutdown requested"));
			theApp.ExitMainLoop();
			break;
		case EC_OP_ED2K_LINK: 
			for(int i = 0; i < request->GetTagCount();i++) {
				CECTag *tag = request->GetTagByIndex(i);
				wxString link = tag->GetStringData();
				CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(unicode2char(link));
				if ( pLink->GetKind() == CED2KLink::kFile ) {
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), 0);
					response = new CECPacket(EC_OP_NOOP);
				} else if ( pLink->GetKind() == CED2KLink::kServer ) {
					CServer *server = new CServer(((CED2KServerLink *)pLink)->GetPort(), Uint32toStringIP(((CED2KServerLink *)pLink)->GetIP()));
					server->SetListName(Uint32toStringIP(((CED2KServerLink *)pLink)->GetIP()));
					theApp.serverlist->AddServer(server);
					Notify_ServerAdd(server);
					response = new CECPacket(EC_OP_NOOP);
				} else {
					AddLogLineM(true, _("ExternalConn: Unable to understand ED2k link '") + link + wxT("'."));
					response = new CECPacket(EC_OP_FAILED);
				}
			}
			break;
		//
		// Status requests
		//
		case EC_OP_STAT_REQ:
			response = Get_EC_Response_StatRequest(request);
		case EC_OP_GET_CONNSTATE:
			if (!response) {
				response = new CECPacket(EC_OP_MISC_DATA);
			}
			response->AddTag(CEC_ConnState_Tag(detail_level));
			break;
		//
		//
		//
		case EC_OP_GET_DLOAD_QUEUE:
			response = Get_EC_Response_GetDownloadQueue(request, enc_map);
			break;
		case EC_OP_GET_ULOAD_QUEUE:
			response = Get_EC_Response_GetUpQueue(request);
			break;
		case EC_OP_PARTFILE_REMOVE_NO_NEEDED:
		case EC_OP_PARTFILE_REMOVE_FULL_QUEUE:
		case EC_OP_PARTFILE_REMOVE_HIGH_QUEUE:
		case EC_OP_PARTFILE_CLEANUP_SOURCES:
		case EC_OP_PARTFILE_SWAP_A4AF_THIS:
		case EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO:
		case EC_OP_PARTFILE_SWAP_A4AF_OTHERS:
		case EC_OP_PARTFILE_PAUSE:
		case EC_OP_PARTFILE_RESUME:
		case EC_OP_PARTFILE_STOP:
		case EC_OP_PARTFILE_PRIO_AUTO:
		case EC_OP_PARTFILE_PRIO_SET:
		case EC_OP_PARTFILE_DELETE:
		case EC_OP_PARTFILE_SET_CAT:
			response = Get_EC_Response_PartFile_Cmd(request);
			break;
		case EC_OP_KNOWNFILE_SET_UP_PRIO:
			break;
		case EC_OP_KNOWNFILE_SET_UP_PRIO_AUTO:
			break;
		case EC_OP_KNOWNFILE_SET_PERM:
			break;
		case EC_OP_KNOWNFILE_SET_COMMENT:
			break;
		case EC_OP_SHAREDFILES_RELOAD:
			theApp.sharedfiles->Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;
		//
		// Server commands
		//
		case EC_OP_SERVER_DISCONNECT:
		case EC_OP_SERVER_CONNECT:
		case EC_OP_SERVER_REMOVE:
			response = Get_EC_Response_Server(request);
			break;
		case EC_OP_GET_SERVER_LIST:
			response = new CECPacket(EC_OP_SERVER_LIST);
			for(uint32 i = 0; i < theApp.serverlist->GetServerCount(); i++) {
				response->AddTag(CEC_Server_Tag(theApp.serverlist->GetServerAt(i), detail_level));
			}	
			break;
		case EC_OP_SERVER_UPDATE_FROM_URL:
			theApp.serverlist->UpdateServerMetFromURL(request->GetTagByIndex(0)->GetStringData());
			response = new CECPacket(EC_OP_NOOP);
			break;
		//
		// IPFilter
		//
		case EC_OP_IPFILTER_RELOAD:
			theApp.ipfilter->Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;
		//
		// Preferences
		//
		case EC_OP_GET_PREFERENCES:
			response = ProcessPreferencesRequest(request);
			break;
		case EC_OP_SET_PREFERENCES:
			response = SetPreferencesFromRequest(request);
			break;
		//
		// Logging
		//
		case EC_OP_ADDLOGLINE:
			AddLogLineM( (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL), request->GetTagByName(EC_TAG_STRING)->GetStringData() );
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_ADDDEBUGLOGLINE:
			AddDebugLogLineM( (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL), request->GetTagByName(EC_TAG_STRING)->GetStringData() );
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_GET_LOG:
			response = new CECPacket(EC_OP_LOG);
			response->AddTag(CECTag(EC_TAG_STRING, theApp.GetLog(false)));
			break;
		case EC_OP_GET_DEBUGLOG:
			response = new CECPacket(EC_OP_DEBUGLOG);
			response->AddTag(CECTag(EC_TAG_STRING, theApp.GetDebugLog(false)));
			break;
		case EC_OP_RESET_LOG:
			theApp.GetLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_RESET_DEBUGLOG:
			theApp.GetDebugLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_GET_LAST_LOG_ENTRY:
			{
				wxString tmp = theApp.GetLog(false);
				if (tmp.Last() == '\n') {
					tmp.RemoveLast();
				}
				response = new CECPacket(EC_OP_LOG);
				response->AddTag(CECTag(EC_TAG_STRING, tmp.AfterLast('\n')));
			}
			break;
		case EC_OP_GET_SERVERINFO:
			response = new CECPacket(EC_OP_SERVERINFO);
			response->AddTag(CECTag(EC_TAG_STRING, theApp.GetServerLog(false)));
			break;
		case EC_OP_CLEAR_SERVERINFO:
			theApp.GetServerLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		default:
			AddLogLineM(false, _("ExternalConn: invalid opcode received"));
			break;
	}	
	return response;
}


//TODO: do a function for each command
wxString ExternalConn::ProcessRequest(const wxString& item) {
	CALL_APP_DATA_LOCK;

	wxString sOp;
	wxString buffer;

	AddLogLineM(false, wxT("Remote command: ") + item);

	//---------------------------------------------------------------------
	// WEBPAGE
	//---------------------------------------------------------------------
	if (item == wxT("WEBPAGE GETGRAPH")) {
		//returns one string formatted as:
		//%d\t%d\t%d\t%d
		buffer = wxString::Format(wxT("%d\t%d\t%d\t%d"), 
			thePrefs::GetTrafficOMeterInterval(),
			thePrefs::GetMaxGraphDownloadRate(),
			thePrefs::GetMaxGraphUploadRate(),
			thePrefs::GetMaxConnections());
		return buffer;
	}
	if (item == wxT("WEBPAGE STATISTICS")) {
#ifdef AMULE_DAEMON
		return wxT("FIXME: remove code from gui");
#else
		int filecount = theApp.downloadqueue->GetFileCount();
		uint32 stats[2]; // get the source count
		theApp.downloadqueue->GetDownloadStats(stats);
		buffer = theApp.amuledlg->statisticswnd->GetHTML() +
			wxString::Format(wxT(
			"\tStatistics: \n"
			"\t\tDownloading files: %d\n"
			"\t\tFound sources: %d\n"
			"\t\tActive downloads: %d\n"
			"\t\tActive Uploads: %d\n"
			"\t\tUsers on upload queue: %d"),
			filecount, stats[0], stats[1], 
			theApp.uploadqueue->GetUploadQueueLength(), 
			theApp.uploadqueue->GetWaitingUserCount());
		return buffer;
#endif
	}
	//---------------------------------------------------------------------
	// TRANSFER
	//---------------------------------------------------------------------
	if (item == wxT("TRANSFER UL_LIST")) {
		// returns one string where each line is formatted as:
		// %s\t%s\t%s\t%ul\t%ul\t%li\n"
		wxString tempFileInfo;
		for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
			 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
			CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
			if (cur_client) {
				buffer += cur_client->GetUserName() + wxT("\t");
				tempFileInfo = cur_client->GetUploadFileInfo();
				tempFileInfo.Replace(wxT("\n"), wxT("|"));
				buffer += tempFileInfo + wxT("\t");
				CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
				if (file) {
					buffer+=file->GetFileName();
				} else {
					buffer+=wxT("?");
				}
				buffer += wxT("\t");
				buffer+=wxString::Format(wxT("%ul\t"), cur_client->GetTransferedDown());
				buffer+=wxString::Format(wxT("%ul\t"), cur_client->GetTransferedUp());
				buffer+=wxString::Format(wxT("%li\n"), (long)(cur_client->GetKBpsUp()*1024.0));
			}
		}
		return buffer;
	}
	//---------------------------------------------------------------------
	// SHAREDFILES
	//---------------------------------------------------------------------
	if (item == wxT("SHAREDFILES LIST")) {
		// returns one string where each line is formatted as:
		// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
		for (int i = 0; i < theApp.sharedfiles->GetCount(); ++i) {
			const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex(i);
			if (cur_file) {
				buffer += cur_file->GetFileName() + wxT("\t") << 
					(long)cur_file->GetFileSize() << wxT("\t") +
					( theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() ?
						theApp.CreateED2kSourceLink(cur_file) :
						theApp.CreateED2kLink(cur_file)	) + wxT("\t") +
						wxString::Format(wxT("%ld\t%ld\t%d\t%d\t%d\t%d\t"),
							(long)cur_file->statistic.GetTransfered(),
							(long)cur_file->statistic.GetAllTimeTransfered(),
							cur_file->statistic.GetRequests(),
							cur_file->statistic.GetAllTimeRequests(),
							cur_file->statistic.GetAccepts(),
							cur_file->statistic.GetAllTimeAccepts() ) +
						EncodeBase16(cur_file->GetFileHash(), 16) + wxT("\t");					
				int prio = cur_file->GetUpPriority();
				if (cur_file->IsAutoUpPriority()) {
					switch (prio) {
						case PR_LOW: 
							buffer.Append(wxT("Auto [Lo]")); break;
						case PR_NORMAL:
							buffer.Append(wxT("Auto [No]")); break;
						case PR_HIGH:
							buffer.Append(wxT("Auto [Hi]")); break;
						case PR_VERYHIGH:
							buffer.Append(wxT("Auto [Re]")); break;
						default:
							buffer.Append(wxT("-")); break;
					}
				} else {
					switch (prio) {
						case PR_VERYLOW:
							buffer.Append(wxT("Very Low")); break;
						case PR_LOW:
							buffer.Append(wxT("Low")); break;
						case PR_NORMAL:
							buffer.Append(wxT("Normal")); break;
						case PR_HIGH:
							buffer.Append(wxT("High")); break;
						case PR_VERYHIGH:
							buffer.Append(wxT("Very High")); break;
						case PR_POWERSHARE:
							buffer.Append(wxT("PowerShare[Release]")); break;
						default:
							buffer.Append(wxT("-")); break;
					}
				}
				buffer +=
					wxString::Format(wxT("\t%d\t"), prio) +
					( cur_file->IsAutoUpPriority() ? wxT("1\n") : wxT("0\n") );
			}
		}
		return buffer;
	}
/**********************************************/
/* Must still do a check from here to the end */
/**********************************************/
		//SEARCH
		if (item.Left(19) == wxT("SEARCH DOWNLOADFILE")) {
			if (item.Length() > 19) {
				uchar fileid[16];
				DecodeBase16(unicode2char(item.Mid(20)),item.Mid(20).Length(),fileid);
				theApp.searchlist->AddFileToDownloadByHash(fileid);
			}
			return wxEmptyString;
		}
		if (item.Left(18) == wxT("SEARCH DONEWSEARCH")) {
			if (item.Length() > 18) {
				//int curIndex, nextIndex;
				wxString sParams = item.Mid(19);
				int brk = sParams.First(wxT("\n"));
				
				wxString searchString = sParams.Left(brk);
				searchString.Trim(true);
				searchString.Trim(false);
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				wxString typeText = sParams.Left(brk);
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
					
				uint32 min = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				uint32 max = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
					
				uint32 avaibility = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				wxString extension = sParams.Left(brk);
				sParams=sParams.Mid(brk+1);
				
				bool globalsearch = (sParams == wxT("global")) ? true : false;
					
				theApp.searchlist->RemoveResults(0xffff);
				theApp.searchlist->NewSearch(typeText, 0xffff);				
				Packet *packet = CreateSearchPacket(searchString, typeText, extension, min, max, avaibility);
				
				// this is internal core call, but macro is useful anyway
				CoreNotify_Search_Req(packet, globalsearch);
			}
		}
		if (item.Left(14) == wxT("SEARCH WEBLIST")) {
			if (item.Length() > 14) {
				wxString sItem = item.Mid(15);
				int brk=sItem.First(wxT("\t"));
				wxString sResultLine=sItem.Left(brk);
				sItem=sItem.Mid(brk+1); brk=sItem.First(wxT("\t"));
				int sortBy = StrToLong(sItem.Left(brk));
				bool searchAsc = StrToLong(sItem.Mid(brk+1));
				return(theApp.searchlist->GetWebList(sResultLine, sortBy, searchAsc));
			}
		}
		// SHAREDFILES
		if (item.Left(11).Cmp(wxT("SHAREDFILES")) == 0) {
			if (item.Mid(12,17).Cmp(wxT("SETAUTOUPPRIORITY")) == 0) {
				if (item.Length() > 29) {
					int separator = item.Mid(30).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(30,separator);
						if (item.Mid(30+separator+1).IsNumber()) {
							bool flag = StrToLong(item.Mid(30+separator+1));
						
							uchar fileid[16];
							DecodeBase16(unicode2char(hash),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetAutoUpPriority(flag);
								return wxT("AutoUpPriority Saved");
							}
							return wxT("Bad file");
						}
					}
				}
				return wxT("Bad SETAUTOUPPRIORITY request");
			}
			if (item.Mid(12,13).Cmp(wxT("SETUPPRIORITY")) == 0) {
				if (item.Length() > 25) {
					int separator = item.Mid(26).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(26,separator);
						if (item.Mid(26+separator+1).IsNumber()) {
							int priority = StrToLong(item.Mid(26+separator+1));
						
							uchar fileid[16];
							DecodeBase16(unicode2char(hash),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetUpPriority(priority);
								return wxT("UpPriority Saved");
							}
							return wxT("Bad file");
						}
					}
				}
				return wxT("Bad SETUPPRIORITY request");
			}
			if (item.Mid(12,20).Cmp(wxT("UPDATEAUTOUPPRIORITY")) == 0) {
				if (item.Length() > 32) {
					int separator = item.Mid(33).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(33,separator);
						uchar fileid[16];
						DecodeBase16(unicode2char(hash),hash.Length(),fileid);
						CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
						if (cur_file) {
							cur_file->UpdateAutoUpPriority();
							return wxT("UpPriority Saved");
						}
						return wxT("Bad file");
					}
				}
				return wxT("Bad UPDATEAUTOUPPRIORITY request");
			}
			return wxT("Bad SHAREDFILES request");
		} //end - shared files
		
		// QUEUE
		if (item.Left(5).Cmp(wxT("QUEUE")) == 0) { //Get queue data information

			if (item.Mid(6).Cmp(wxT("UL_GETLENGTH")) == 0) {
				return wxString::Format( wxT("%i"), theApp.uploadqueue->GetUploadQueueLength() );
			}
			
			return wxT("Bad QUEUE request");
		} // end - QUEUE
		
		return wxEmptyString;
}

ExternalConnClientThread::ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock) : wxThread()
{
	m_owner = owner;
	m_sock = sock;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM(false, _("ExternalConnClientThread: failed to Create thread\n"));
	}
	sock->SetFlags(wxSOCKET_WAITALL);
}

ExternalConnClientThread::~ExternalConnClientThread()
{
	delete m_sock;
}

void *ExternalConnClientThread::Entry()
{
	CECPacket *request = m_owner->m_ECServer->ReadPacket(m_sock);
	CECPacket *response = m_owner->Authenticate(request);
	m_owner->m_ECServer->WritePacket(m_sock, response);
	if (response->GetOpCode() != EC_OP_AUTH_OK) {
		//
		// Access denied!
		//
		AddLogLineM(false, _("Unauthorized access attempt. Connection closed.\n"));
		return 0;
	}
	//
	// Authenticated
	//
	while ( !TestDestroy() ) {
		if ( m_sock->WaitForLost(0, 0) ) {
			AddLogLineM(false, _("ExternalConnClientThread: connection closed\n"));
			break;
		}
		if (m_sock->WaitForRead(1, 0)) {
			request = m_owner->m_ECServer->ReadPacket(m_sock);
			response = m_owner->ProcessRequest2(request, m_encoders);
			delete request;
			if ( response ) {
				m_owner->m_ECServer->WritePacket(m_sock, response);
				delete response;
			}
		}
	}
	return 0;
}
