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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ExternalConn.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for VERSION
#endif

#include "ExternalConn.h"	// Interface declarations
#include "OtherFunctions.h"	// Needed for EncodeBase16
#include "updownclient.h"	// Needed for CUpDownClient
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "ServerConnect.h"		// Needed for CServerConnect
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "SearchList.h"		// Needed for GetSearchResults
#include "IPFilter.h"		// Needed for CIPFilter
#include "ClientList.h"
#include "Preferences.h"	// Needed for CPreferences
#include "CMD4Hash.h"		// Needed for CMD4Hash

#include "GuiEvents.h"		// Needed for Notify_* macros
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP()
#include "ECPacket.h"		// Needed for CECPacket, CECTag
#include "ECcodes.h"		// Needed for OPcodes, TAGnames
#include "ECSpecialTags.h"	// Needed for special EC tag creator classes
#include "Statistics.h"

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


ExternalConn::ExternalConn(amuleIPV4Address addr, wxString *msg)
#ifdef AMULE_DAEMON
 : wxThread(wxTHREAD_JOINABLE) 
#endif
{
	wxString msgLocal;
	m_ECServer = NULL;
	// Are we allowed to accept External Connections?
	if ( thePrefs::AcceptExternalConnections() && thePrefs::ECUseTCPPort() ) {
		// We must have a valid password, otherwise we will not allow EC connections
		if ( thePrefs::ECPassword().IsEmpty() ) {
			thePrefs::EnableExternalConnections( false );
			*msg += wxT("External connections disabled due to empty password!\n");
			AddLogLineM(true, _("External connections disabled due to empty password!"));
			return;
		}
		
		// Create the socket
#ifdef AMULE_DAEMON
		m_ECServer = new ECSocket(addr, 0);
#else
		m_ECServer = new ECSocket(addr, this, SERVER_ID);
#endif
		int port = addr.Service();
		wxString ip = addr.IPAddress();
		if (m_ECServer->Ok()) {
			msgLocal = wxT("*** TCP socket (ECServer) listening on ") + ip + 
				wxString::Format(wxT(":%d"), port);
			*msg += msgLocal + wxT("\n");
			AddLogLineM(false, msgLocal);
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
			msgLocal = wxT("Could not listen for external connections at ") + ip + 
				wxString::Format(wxT(":%d!"), port);
			*msg += msgLocal + wxT("\n");
			AddLogLineM(false, msgLocal);
		}
	} else {
		*msg += wxT("External connections disabled in config file .eMule\n");
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
		RegisterSocket(sock);
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
		if(request == NULL) {
			AddLogLineM(false, _("Invalid EC packet received"));
			break;
		}
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
		UnregisterSocket(sock);
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

	// Password must be specified if we are to allow remote connections
	if ( thePrefs::ECPassword().IsEmpty() ) {
		AddLogLineM(true, _("External connection refused due to empty password in preferences!"));	
		
		return new CECPacket(EC_OP_AUTH_FAIL);
	}
	

	if (request->GetOpCode() == EC_OP_AUTH_REQ) {
		const CECTag *clientName = request->GetTagByName(EC_TAG_CLIENT_NAME);
		const CECTag *clientVersion = request->GetTagByName(EC_TAG_CLIENT_VERSION);
		AddLogLineM(false, _("Connecting client: ") + ((clientName == NULL) ? wxString(_("Unknown")) : clientName->GetStringData()) +
			wxT(" ") + ((clientVersion == NULL) ? wxString(_("Unknown version")) : clientVersion->GetStringData()));
		const CECTag *passwd = request->GetTagByName(EC_TAG_PASSWD_HASH);
		const CECTag *protocol = request->GetTagByName(EC_TAG_PROTOCOL_VERSION);
		if (protocol != NULL) {
			uint16 proto_version = protocol->GetInt16Data();
			if (proto_version == 0x01f1) {
				if (passwd && passwd->GetStringData() == thePrefs::ECPassword()) {
					response = new CECPacket(EC_OP_AUTH_OK);
				} else {
					response = new CECPacket(EC_OP_AUTH_FAIL);
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Authentication failed.")));
				}
			} else {
				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid protocol version.")));
			}
		} else {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Missing protocol version tag.")));
		}
	} else {
		response = new CECPacket(EC_OP_AUTH_FAIL);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid request, you should first authenticate.")));
	}

	response->AddTag(CECTag(EC_TAG_SERVER_VERSION, wxT(VERSION)));

	if (response->GetOpCode() == EC_OP_AUTH_OK) {
		AddLogLineM(false, _("Access granted."));
	} else {
		AddLogLineM(false, wxGetTranslation(response->GetTagByIndex(0)->GetStringData()));
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

CECPacket *Get_EC_Response_GetSharedFiles(const CECPacket *request)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_SHARED_FILES);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);
	
	for (int i = 0; i < theApp.sharedfiles->GetCount(); ++i) {
		const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex(i);
		if ( !cur_file) {
			// lfroen: wtf - can this really happen ?!
			continue;
		}
		response->AddTag(CEC_SharedFile_Tag(cur_file, detail_level));
	}
	return response;
}

CECPacket *Get_EC_Response_GetUpQueue(const CECPacket *request)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_ULOAD_QUEUE);
	
	CECPacket *response = new CECPacket(EC_OP_ULOAD_QUEUE);
	
	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	
	POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
	while (	pos ) {

		CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
		theApp.uploadqueue->GetNextFromUploadList(pos);
		if (!cur_client) {
			continue;
		}
		CEC_UpDownClient_Tag cli_tag(cur_client, detail_level);
		
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
	std::set<CMD4Hash> queryitems;
	for (int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		if ( tag->GetTagName() == EC_TAG_PARTFILE ) {
			queryitems.insert(tag->GetMD4Data());
		}
	}
	
	
	// check if encoder contains files that no longer in download queue
	// or, we have new files without encoder yet
	std::set<CPartFile *> curr_files, dead_files;
	for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
		curr_files.insert(cur_file);
		if ( encoders.count(cur_file) == 0 ) {
			encoders[cur_file] = CPartFile_Encoder(cur_file);
		}
	}
	//
	// curr_files set is created to minimize lookup time in download queue,
	// since GetFileByID have loop inside leading to O(n), in this case
	// it will mean O(n^2)
	for(CPartFile_Encoder_Map::iterator i = encoders.begin(); i != encoders.end(); i++) {
		if ( curr_files.count(i->first) == 0 ) {
			dead_files.insert(i->first);
		}
	}
	for(std::set<CPartFile *>::iterator i = dead_files.begin(); i != dead_files.end(); i++) {
		encoders.erase(*i);
	}
	
	for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
	
		if ( !queryitems.empty() && !queryitems.count(cur_file->GetFileHash()) ) {
			continue;
		}

		CEC_PartFile_Tag filetag(cur_file, detail_level);
		
		CPartFile_Encoder &enc = encoders[cur_file];
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc.ResetEncoder();
		}
		enc.Encode(&filetag);

		response->AddTag(filetag);
	}

	return 	response;
}


CECPacket *Get_EC_Response_PartFile_Cmd(const CECPacket *request)
{
	CECPacket *response = NULL;

	// request can contain multiple files.
	for (int i = 0; i < request->GetTagCount(); ++i) {
		const CECTag *hashtag = request->GetTagByIndex(i);

		wxASSERT(hashtag->GetTagName() == EC_TAG_PARTFILE);

		CMD4Hash hash = hashtag->GetMD4Data();
		CPartFile *pfile = theApp.downloadqueue->GetFileByID( hash );
		
		if ( !pfile ) {
			AddLogLineM(false,_("Remote PartFile command failed: FileHash not found: ") + hash.Encode());
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("FileHash not found: ") + hash.Encode()));
			//return response;
			break;
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
					CPartFile::SourceSet::iterator it = pfile->A4AFsrclist.begin();
					while ( it != pfile->A4AFsrclist.end() ) {
						CUpDownClient *cur_source = *it++;
						
						cur_source->SwapToAnotherFile(true, false, false, pfile);
					}
				}
				break;
			case EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO:
				pfile->SetA4AFAuto(!pfile->IsA4AFAuto());
				break;
			case EC_OP_PARTFILE_SWAP_A4AF_OTHERS:
				if ((pfile->GetStatus(false) == PS_READY) ||
				    (pfile->GetStatus(false) == PS_EMPTY)) {
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
			case EC_OP_PARTFILE_PRIO_SET: {
					int prio = hashtag->GetTagByIndex(0)->GetInt32Data();
					if ( prio == PR_AUTO ) {
						pfile->SetAutoDownPriority(1);
					} else {
						pfile->SetAutoDownPriority(0);
						pfile->SetDownPriority(prio);
					}
				}
				break;
			case EC_OP_PARTFILE_DELETE:
				if ( thePrefs::StartNextFile() && (pfile->GetStatus() == PS_PAUSED) ) {
					theApp.downloadqueue->StartNextFile();
				}
				pfile->Delete();
				break;
/*
			case EC_OP_PARTFILE_SET_CAT:
				if ( !valtag ) {
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_STRING,
								wxTRANSLATE("no value tag in EC_OP_PARTFILE_CAT_SET")));
					return response;
				}
				break;
*/
			default:
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("OOPS! OpCode processing error!")));
				break;
		}
	}
	if (!response) {
		response = new CECPacket(EC_OP_NOOP);
	}
	return response;
}

CECPacket *Get_EC_Response_Server(const CECPacket *request)
{
	CECPacket *response = NULL;
	const CECTag *srv_tag = request->GetTagByIndex(0);
	CServer *srv = 0;
	if ( srv_tag ) {
		uint32 srv_ip = srv_tag->GetIPv4Data().IP();
		for(uint32 i = 0; i < theApp.serverlist->GetServerCount(); i++) {
			CServer *curr_srv = theApp.serverlist->GetServerByIndex(i);
			// lfroen: never saw 2 servers on same IP !
			if ( curr_srv->GetIP() == srv_ip) {
				srv = curr_srv;
				break;
			}
		}
		// server tag passed, but server not found
		if ( !srv ) {
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING,
						wxString(wxTRANSLATE("server not found: ")) + srv_tag->GetIPv4Data().StringIP()));
			return response;
		}
	}
	switch (request->GetOpCode()) {
		case EC_OP_SERVER_DISCONNECT:
			theApp.serverconnect->Disconnect();
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_SERVER_REMOVE:
			if ( srv ) {
				Notify_ServerRemove(srv);
				theApp.serverlist->RemoveServer(srv);
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING,
							wxTRANSLATE("need to define server to be removed")));
			}
			break;
		case EC_OP_SERVER_CONNECT:
			if ( srv ) {
				theApp.serverconnect->ConnectToServer(srv);
				response = new CECPacket(EC_OP_NOOP);
			} else {
				theApp.serverconnect->ConnectToAnyServer();
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
	}
	if (!response) {
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("OOPS! OpCode processing error!")));
	}
	return response;
}

CECPacket *Get_EC_Response_Search_Results(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_SEARCH_RESULTS);
	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	std::set<CMD4Hash> queryitems;
	for (int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		if ( tag->GetTagName() == EC_TAG_SEARCHFILE ) {
			queryitems.insert(tag->GetMD4Data());
		}
	}
	std::vector<CSearchFile*> list(theApp.searchlist->GetSearchResults(0xffff));
	std::vector<CSearchFile*>::const_iterator it = list.begin();
	while (it != list.end()) {
		CSearchFile* sf = *it++;
		if ( !queryitems.empty() && !queryitems.count(sf->GetFileHash()) ) {
			continue;
		}
		response->AddTag(CEC_SearchFile_Tag(sf, detail_level));
	}	
	return response;
}

CECPacket *Get_EC_Response_Search_Results_Download(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);
	for (int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		CMD4Hash hash = tag->GetMD4Data();
		theApp.searchlist->AddFileToDownloadByHash(hash);
	}
	return response;
}

CECPacket *Get_EC_Response_Search(const CECPacket *request)
{
	wxString response;
	
	CEC_Search_Tag *search_request = (CEC_Search_Tag *)request->GetTagByIndex(0);
	theApp.searchlist->RemoveResults(0xffff);

	wxString text = search_request->SearchText();
	wxString file_type = search_request->SearchFileType();
	wxString ext = search_request->SearchExt();
		
	EC_SEARCH_TYPE search_type = search_request->SearchType();
	bool global_search = false;
	switch(search_type) {
		case EC_SEARCH_GLOBAL:
			global_search = true;
		case EC_SEARCH_LOCAL:
			if (!theApp.searchlist->StartNewSearch(0xffff, global_search, text, file_type, ext, search_request->MinSize(), search_request->MaxSize(), search_request->Avail())) {
				// Not connected?
				response = _("aMule is not connected! Cannot do search.");
			} else {
				response = _("Search in progress. Refetch results in a moment!");			
			}
			break;
		case EC_SEARCH_WEB:
				response = _("WebSearch from remote interface makes no sense.");
			break;
	}
	
	CECPacket *reply = new CECPacket(EC_OP_FAILED);
	// no reply - search in progress
	reply->AddTag(CECTag(EC_TAG_STRING, response));
		
	return reply;
}

CECPacket *Get_EC_Response_Set_SharedFile_Prio(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);
	for (int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		CMD4Hash hash = tag->GetMD4Data();
		uint8 prio = tag->GetTagByIndex(0)->GetInt32Data();
		CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(hash);
		if ( !cur_file ) {
			continue;
		}
		if (prio == PR_AUTO) {
			cur_file->SetAutoUpPriority(1);
			cur_file->UpdateAutoUpPriority();
		} else {
			cur_file->SetUpPriority(prio);
		}
	}

	return response;
}

CECPacket *ProcessPreferencesRequest(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_PREFERENCES);
	const CECTag *selTag = request->GetTagByName(EC_TAG_SELECT_PREFS);
	uint32 selection = selTag ? selTag->GetInt32Data() : 0;	
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
		CECEmptyTag user_prefs(EC_TAG_PREFS_GENERAL);
		user_prefs.AddTag(CECTag(EC_TAG_USER_NICK, thePrefs::GetUserNick()));
		user_prefs.AddTag(CECTag(EC_TAG_USER_USERHASH, thePrefs::GetUserHash()));
		response->AddTag(user_prefs);
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
		CECEmptyTag msg_prefs(EC_TAG_PREFS_MESSAGEFILTER);
		if (thePrefs::MustFilterMessages()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_ENABLED));
		}
		if (thePrefs::IsFilterAllMessages()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_ALL));
		}
		if (thePrefs::MsgOnlyFriends()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_FRIENDS));
		}
		if (thePrefs::MsgOnlySecure()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_SECURE));
		}
		if (thePrefs::IsFilterByKeywords()) {
			msg_prefs.AddTag(CECEmptyTag(EC_TAG_MSGFILTER_BY_KEYWORD));
		}
		msg_prefs.AddTag(CECTag(EC_TAG_MSGFILTER_KEYWORDS, thePrefs::GetMessageFilterString()));
		response->AddTag(msg_prefs);
	}

	if (selection & EC_PREFS_REMOTECONTROLS) {
		CECEmptyTag rc_prefs(EC_TAG_PREFS_REMOTECTRL);
		rc_prefs.AddTag(CECTag(EC_TAG_WEBSERVER_PORT, thePrefs::GetWSPort()));
		if (thePrefs::GetWSIsEnabled()) {
			rc_prefs.AddTag(CECEmptyTag(EC_TAG_WEBSERVER_AUTORUN));
		}
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
		CECEmptyTag online_sig(EC_TAG_PREFS_ONLINESIG);
		if (thePrefs::IsOnlineSignatureEnabled()) {
			online_sig.AddTag(CECEmptyTag(EC_TAG_ONLINESIG_ENABLED));
		}
		response->AddTag(online_sig);
	}

	if (selection & EC_PREFS_SERVERS) {
		CECEmptyTag srv_prefs(EC_TAG_PREFS_SERVERS);
		if (thePrefs::DeadServer()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_REMOVE_DEAD));
		}
		srv_prefs.AddTag(CECTag(EC_TAG_SERVERS_DEAD_SERVER_RETRIES, (uint16)thePrefs::GetDeadserverRetries()));
		if (thePrefs::AutoServerlist()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_AUTO_UPDATE));
		}
		// Here should come the URL list...
		if (thePrefs::AddServersFromServer()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_ADD_FROM_SERVER));
		}
		if (thePrefs::AddServersFromClient()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_ADD_FROM_CLIENT));
		}
		if (thePrefs::Score()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_USE_SCORE_SYSTEM));
		}
		if (thePrefs::GetSmartIdCheck()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_SMART_ID_CHECK));
		}
		if (thePrefs::IsSafeServerConnectEnabled()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_SAFE_SERVER_CONNECT));
		}
		if (thePrefs::AutoConnectStaticOnly()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY));
		}
		if (thePrefs::IsManualHighPrio()) {
			srv_prefs.AddTag(CECEmptyTag(EC_TAG_SERVERS_MANUAL_HIGH_PRIO));
		}
		response->AddTag(srv_prefs);
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
		CECEmptyTag srcdrop(EC_TAG_PREFS_SRCDROP);
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_NONEEDED, (uint8)thePrefs::GetNoNeededSources()));
		if (thePrefs::DropFullQueueSources()) {
			srcdrop.AddTag(CECEmptyTag(EC_TAG_SRCDROP_DROP_FQS));
		}
		if (thePrefs::DropHighQueueRankingSources()) {
			srcdrop.AddTag(CECEmptyTag(EC_TAG_SRCDROP_DROP_HQRS));
		}
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_HQRS_VALUE, (uint16)thePrefs::HighQueueRanking()));
		srcdrop.AddTag(CECTag(EC_TAG_SRCDROP_AUTODROP_TIMER, (uint16)thePrefs::GetAutoDropTimer()));
		response->AddTag(srcdrop);
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
	const CECTag * thisTab;
	const CECTag * oneTag;

	/* Maybe we'll have another way to change categories
	if ((thisTab =request->GetTagByName(EC_TAG_PREFS_CATEGORIES)) != NULL) {
	}
	*/

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_GENERAL)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_USER_NICK)) != NULL) {
			thePrefs::SetUserNick(oneTag->GetStringData());
		}
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
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_ENABLED)) != NULL) {
			thePrefs::SetMustFilterMessages(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_ALL)) != NULL) {
			thePrefs::SetFilterAllMessages(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_FRIENDS)) != NULL) {
			thePrefs::SetMsgOnlyFriends(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_SECURE)) != NULL) {
			thePrefs::SetMsgOnlySecure(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_BY_KEYWORD)) != NULL) {
			thePrefs::SetFilterByKeywords(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_MSGFILTER_KEYWORDS)) != NULL) {
			thePrefs::SetMessageFilterString(oneTag->GetStringData());
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_REMOTECTRL)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_WEBSERVER_AUTORUN)) != NULL) {
			thePrefs::SetWSIsEnabled(oneTag->GetInt8Data() != 0);
		}
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
		if ((oneTag = thisTab->GetTagByName(EC_TAG_ONLINESIG_ENABLED)) != NULL) {
			thePrefs::SetOnlineSignatureEnabled(oneTag->GetInt8Data() != 0);
		}
	}

	if ((thisTab = request->GetTagByName(EC_TAG_PREFS_SERVERS)) != NULL) {
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_REMOVE_DEAD)) != NULL) {
			thePrefs::SetDeadServer(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_DEAD_SERVER_RETRIES)) != NULL) {
			thePrefs::SetDeadserverRetries(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_AUTO_UPDATE)) != NULL) {
			thePrefs::SetAutoServerlist(oneTag->GetInt8Data() != 0);
		}
		// Here should come the URL list...
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_ADD_FROM_SERVER)) != NULL) {
			thePrefs::SetAddServersFromServer(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_ADD_FROM_CLIENT)) != NULL) {
			thePrefs::SetAddServersFromClient(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_USE_SCORE_SYSTEM)) != NULL) {
			thePrefs::SetScoreSystem(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_SMART_ID_CHECK)) != NULL) {
			thePrefs::SetSmartIdCheck(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_SAFE_SERVER_CONNECT)) != NULL) {
			thePrefs::SetSafeServerConnectEnabled(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY)) != NULL) {
			thePrefs::SetAutoConnectStaticOnly(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SERVERS_MANUAL_HIGH_PRIO)) != NULL) {
			thePrefs::SetManualHighPrio(oneTag->GetInt8Data() != 0);
		}
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
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_NONEEDED)) != NULL) {
			thePrefs::SetNoNeededSources(oneTag->GetInt8Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_DROP_FQS)) != NULL) {
			thePrefs::SetDropFullQueueSources(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_DROP_HQRS)) != NULL) {
			thePrefs::SetDropHighQueueRankingSources(oneTag->GetInt8Data() != 0);
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_HQRS_VALUE)) != NULL) {
			thePrefs::SetHighQueueRanking(oneTag->GetInt16Data());
		}
		if ((oneTag = thisTab->GetTagByName(EC_TAG_SRCDROP_AUTODROP_TIMER)) != NULL) {
			thePrefs::SetAutoDropTimer(oneTag->GetInt8Data());
		}
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
CPartFile_Encoder::GapBuffer CPartFile_Encoder::m_gap_buffer(128);

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


void CPartFile_Encoder::Encode(CECTag *parent)
{
	size_t gap_list_size = m_file->gaplist.GetCount();
	
	if ( m_gap_buffer.size() < gap_list_size * 2 ) {
		m_gap_buffer.clear();
		m_gap_buffer.resize(gap_list_size * 2);
	} 
	
//	printf("GapList have %d entries\n", gap_list_size);
	
	POSITION curr_pos = m_file->gaplist.GetHeadPosition();
	GapBuffer::iterator it = m_gap_buffer.begin();
	while ( curr_pos ) {
		Gap_Struct *curr = m_file->gaplist.GetNext(curr_pos);
		*it++ = htonl(curr->start);
		*it++ = htonl(curr->end);
//		printf("GAP to buffer [%08x %08x]\n", curr->start, curr->end);
	}

//	printf("DEBUG: gap data to send %d dwords [:\n", gap_list_size);
//	otherfunctions::DumpMem_DW(&m_gap_buffer[0], gap_list_size);
//	printf("]\n");
	
	m_enc_data.m_gap_status.Realloc(gap_list_size*2*sizeof(uint32));
	int gap_enc_size = 0;
	const unsigned char *gap_enc_data = m_enc_data.m_gap_status.Encode((unsigned char *)&m_gap_buffer[0], gap_enc_size);

//	printf("DEBUG: gap data encoded %d bytes [:\n", gap_enc_size);
//	otherfunctions::DumpMem(gap_enc_data, gap_enc_size);
//	printf("]\n");
	
	int part_enc_size;
	const unsigned char *part_enc_data = m_enc_data.m_part_status.Encode(m_file->m_SrcpartFrequency, part_enc_size);


	parent->AddTag(CECTag(EC_TAG_PARTFILE_PART_STATUS, part_enc_size, part_enc_data));
	
	//
	// Put data inside of tag in following order:
	// [num_of_gaps] [gap_enc_data]
	//
	unsigned char *tagdata;
	CECTag etag(EC_TAG_PARTFILE_GAP_STATUS,
		sizeof(uint32) + gap_enc_size, (void **)&tagdata);

	// real number of gaps - so remote size can realloc
	*((uint32 *)tagdata) = htonl(gap_list_size);
	tagdata += sizeof(uint32);
	memcpy(tagdata, gap_enc_data, gap_enc_size);


	parent->AddTag(etag);

	curr_pos = m_file->requestedblocks_list.GetHeadPosition();
	wxASSERT(m_gap_buffer.size() >= (size_t)m_file->requestedblocks_list.GetCount() * 2);
	it = m_gap_buffer.begin();
	while ( curr_pos ) {
		Requested_Block_Struct* block = m_file->requestedblocks_list.GetNext(curr_pos);
		*it++ = htonl(block->StartOffset);
		*it++ = htonl(block->EndOffset);
	}
	parent->AddTag(CECTag(EC_TAG_PARTFILE_REQ_STATUS,
		m_file->requestedblocks_list.GetCount() * 2 * sizeof(uint32), (void *)&m_gap_buffer[0]));
}


CECPacket *GetStatsGraphs(const CECPacket *request)
{
	CECPacket *response = NULL;

	switch (request->GetDetailLevel()) {
		case EC_DETAIL_GUI:
			// Transfer graph db
			break;
		case EC_DETAIL_WEB: {
			double dTimestamp = 0.0;
			if (request->GetTagByName(EC_TAG_STATSGRAPH_LAST) != NULL) {
				wxString tmp = request->GetTagByName(EC_TAG_STATSGRAPH_LAST)->GetStringData();
				if (!tmp.ToDouble(&dTimestamp)) {
					dTimestamp = 0.0;
				}
			}
			uint16 nScale = request->GetTagByName(EC_TAG_STATSGRAPH_SCALE)->GetInt16Data();
			uint16 nMaxPoints = request->GetTagByName(EC_TAG_STATSGRAPH_WIDTH)->GetInt16Data();
			uint32 *graphData;
			unsigned int numPoints = theApp.statistics->GetHistoryForWeb(nMaxPoints, (double)nScale, &dTimestamp, &graphData);
			if (numPoints) {
				response = new CECPacket(EC_OP_STATSGRAPHS);
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_DATA, 3 * numPoints * sizeof(uint32), graphData));
				delete [] graphData;
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_LAST, wxString::Format(wxT("%f"), dTimestamp)));
			} else {
				response = new CECPacket(EC_OP_FAILED);
			}
		}
		case EC_DETAIL_UPDATE:
		case EC_DETAIL_CMD:
			// No graphs
			break;
	}
	if (response) {
		return response;
	} else {
		return new CECPacket(EC_OP_FAILED);
	}
}

CECPacket *ExternalConn::ProcessRequest2(const CECPacket *request, CPartFile_Encoder_Map &enc_map)
{

	if ( !request ) {
		return 0;
	}

	CECPacket *response = NULL;

	switch (request->GetOpCode()) {
		//
		// Misc commands
		//
		case EC_OP_SHUTDOWN:
			response = new CECPacket(EC_OP_NOOP);
			AddLogLineM(true, _("ExternalConn: shutdown requested"));
			theApp.ExitMainLoop();
			break;
		case EC_OP_ED2K_LINK: 
			for(int i = 0; i < request->GetTagCount();i++) {
				const CECTag *tag = request->GetTagByIndex(i);
				wxString link = tag->GetStringData();
				
				if ( theApp.downloadqueue->AddED2KLink( link ) ) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					AddLogLineM(true, _("ExternalConn: Unable to understand ed2k link '") + link + wxT("'."));
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
			response->AddTag(CEC_ConnState_Tag(request->GetDetailLevel()));
			break;
		//
		//
		//
		case EC_OP_GET_SHARED_FILES:
			response = Get_EC_Response_GetSharedFiles(request);
			break;
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
		case EC_OP_PARTFILE_PRIO_SET:
		case EC_OP_PARTFILE_DELETE:
		case EC_OP_PARTFILE_SET_CAT:
			response = Get_EC_Response_PartFile_Cmd(request);
			break;
		case EC_OP_SHAREDFILES_RELOAD:
			theApp.sharedfiles->Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;

		case EC_OP_SHARED_SET_PRIO:
			response = Get_EC_Response_Set_SharedFile_Prio(request);
			break;
			
		//
		// Server commands
		//
		case EC_OP_SERVER_DISCONNECT:
		case EC_OP_SERVER_CONNECT:
		case EC_OP_SERVER_REMOVE:
			response = Get_EC_Response_Server(request);
			break;
		case EC_OP_GET_SERVER_LIST: {
				response = new CECPacket(EC_OP_SERVER_LIST);
				EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
				for(uint32 i = 0; i < theApp.serverlist->GetServerCount(); i++) {
					response->AddTag(CEC_Server_Tag(theApp.serverlist->GetServerByIndex(i), detail_level));
				}
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
		// Search
		//
		case EC_OP_SEARCH_START:
			response = Get_EC_Response_Search(request);
			break;

		case EC_OP_SEARCH_RESULTS:
			response = Get_EC_Response_Search_Results(request);
			break;

		case EC_OP_DOWNLOAD_SEARCH_RESULT:
			response = Get_EC_Response_Search_Results_Download(request);
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
		//
		// Statistics
		//
		case EC_OP_GET_STATSGRAPHS:
			response = GetStatsGraphs(request);
//			response = new CECPacket(EC_OP_FAILED);
			break;
		case EC_OP_GET_STATSTREE:
			response = new CECPacket(EC_OP_STATSTREE);
			response->AddTag(CEC_Tree_Tag(theApp.statistics->statstree.begin().begin()));
			if (request->GetDetailLevel() == EC_DETAIL_WEB) {
				response->AddTag(CECTag(EC_TAG_SERVER_VERSION, wxT(PACKAGE_VERSION)));
				response->AddTag(CECTag(EC_TAG_USER_NICK, thePrefs::GetUserNick()));
			}
			break;		
		
		default:
			AddLogLineM(false, _("ExternalConn: invalid opcode received"));
			break;
	}	
	return response;
}


ExternalConnClientThread::ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock) : wxThread()
{
	m_owner = owner;
	m_sock = sock;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM(false, _("ExternalConnClientThread: failed to Create thread\n"));
	}
	RegisterSocket(sock);
}

ExternalConnClientThread::~ExternalConnClientThread()
{
	UnregisterSocket(m_sock);
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
