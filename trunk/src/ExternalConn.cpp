//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008 Froenchenko Leonid (lfroen@gmail.com)
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

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#include <ec/cpp/ECMuleSocket.h>		// Needed for CECSocket

#include <common/Format.h>		// Needed for CFormat

#include <common/ClientVersion.h>
#include <common/MD5Sum.h>

#include "ExternalConn.h"	// Interface declarations
#include "updownclient.h"	// Needed for CUpDownClient
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "PartFile.h"		// Needed for CPartFile
#include "ServerConnect.h"	// Needed for CServerConnect
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "amule.h"		// Needed for theApp
#include "SearchList.h"		// Needed for GetSearchResults
#include "IPFilter.h"		// Needed for CIPFilter
#include "ClientList.h"
#include "Preferences.h"	// Needed for CPreferences
#include "Logger.h"
#include "GuiEvents.h"		// Needed for Notify_* macros
#include "Statistics.h"		// Needed for theStats
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "RandomFunctions.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"


//-------------------- CECServerSocket --------------------

class CECServerSocket : public CECMuleSocket
{
public:
	CECServerSocket(ECNotifier *notifier);
	virtual ~CECServerSocket();

	virtual const CECPacket *OnPacketReceived(const CECPacket *packet);
	virtual void OnLost();

	virtual void WriteDoneAndQueueEmpty();
private:
	ECNotifier *m_ec_notifier;

	const CECPacket *Authenticate(const CECPacket *);

	enum {
		CONN_INIT,
		CONN_SALT_SENT,
		CONN_ESTABLISHED,
		CONN_FAILED
	} m_conn_state;

	uint64_t m_passwd_salt;
	CLoggerAccess m_LoggerAccess;
	CPartFile_Encoder_Map	m_part_encoder;
	CKnownFile_Encoder_Map	m_shared_encoder;
	CObjTagMap		m_obj_tagmap;
	CECPacket *ProcessRequest2(
		const CECPacket *request,
		CPartFile_Encoder_Map &,
		CKnownFile_Encoder_Map &,
		CObjTagMap &);
	
};


CECServerSocket::CECServerSocket(ECNotifier *notifier)
:
CECMuleSocket(true),
m_conn_state(CONN_INIT),
m_passwd_salt(GetRandomUint64()),
m_part_encoder(),
m_shared_encoder(),
m_obj_tagmap()
{
	wxASSERT(theApp->ECServerHandler);
	theApp->ECServerHandler->AddSocket(this);
	m_ec_notifier = notifier;
}


CECServerSocket::~CECServerSocket()
{
	wxASSERT(theApp->ECServerHandler);
	theApp->ECServerHandler->RemoveSocket(this);
}


const CECPacket *CECServerSocket::OnPacketReceived(const CECPacket *packet)
{
	packet->DebugPrint(true);

	const CECPacket *reply = NULL;

	if (m_conn_state == CONN_FAILED) {
		// Client didn't close the socket when authentication failed.
		AddLogLineM(false, _("Client sent packet after authentication failed."));
		CloseSocket();
	}

	if (m_conn_state != CONN_ESTABLISHED) {
		// This is called twice:
		// 1) send salt
		// 2) verify password
		reply = Authenticate(packet);
	} else {
		reply = ProcessRequest2(
			packet, m_part_encoder, m_shared_encoder, m_obj_tagmap);
	}
	return reply;
}


void CECServerSocket::OnLost()
{
	AddLogLineM(false,_("External connection closed."));
	theApp->ECServerHandler->m_ec_notifier->Remove_EC_Client(this);
	DestroySocket();
}

void CECServerSocket::WriteDoneAndQueueEmpty()
{
	if ( HaveNotificationSupport() && (m_conn_state == CONN_ESTABLISHED) ) {
		CECPacket *packet = m_ec_notifier->GetNextPacket(this);
		if ( packet ) {
			SendPacket(packet);
		}
	} else {
		//printf("[EC] %p: WriteDoneAndQueueEmpty but notification disabled\n", this);
	}
}

//-------------------- ExternalConn --------------------

enum
{	// id for sockets
	SERVER_ID = 1000
};


BEGIN_EVENT_TABLE(ExternalConn, wxEvtHandler)
	EVT_SOCKET(SERVER_ID, ExternalConn::OnServerEvent)
END_EVENT_TABLE()


ExternalConn::ExternalConn(amuleIPV4Address addr, wxString *msg)
{
	wxString msgLocal;
	m_ECServer = NULL;
	// Are we allowed to accept External Connections?
	if ( thePrefs::AcceptExternalConnections() ) {
		// We must have a valid password, otherwise we will not allow EC connections
		if (thePrefs::ECPassword().IsEmpty()) {
			*msg += wxT("External connections disabled due to empty password!\n");
			AddLogLineM(true, _("External connections disabled due to empty password!"));
			return;
		}
		
		// Create the socket
		m_ECServer = new wxSocketServer(addr, wxSOCKET_REUSEADDR);
		m_ECServer->SetEventHandler(*this, SERVER_ID);
		m_ECServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
		m_ECServer->Notify(true);

		int port = addr.Service();
		wxString ip = addr.IPAddress();
		if (m_ECServer->Ok()) {
			msgLocal = wxT("*** TCP socket (ECServer) listening on ") + ip + 
				wxString::Format(wxT(":%d"), port);
			*msg += msgLocal + wxT("\n");
			AddLogLineM(false, msgLocal);
		} else {
			msgLocal = wxT("Could not listen for external connections at ") + ip + 
				wxString::Format(wxT(":%d!"), port);
			*msg += msgLocal + wxT("\n");
			AddLogLineM(false, msgLocal);
		}
	} else {
		*msg += wxT("External connections disabled in config file\n");
		AddLogLineM(false,_("External connections disabled in config file"));
	}
	m_ec_notifier = new ECNotifier();
}


ExternalConn::~ExternalConn()
{
	KillAllSockets();
	delete m_ECServer;
	delete m_ec_notifier;
}


void ExternalConn::AddSocket(CECServerSocket *s)
{
	wxASSERT(s);
	socket_list.insert(s);
}


void ExternalConn::RemoveSocket(CECServerSocket *s)
{
	wxASSERT(s);
	socket_list.erase(s);
}


void ExternalConn::KillAllSockets()
{
	AddDebugLogLineM(false, logGeneral,
		CFormat(wxT("ExternalConn::KillAllSockets(): %d sockets to destroy.")) %
			socket_list.size());
	SocketSet::iterator it = socket_list.begin();
	while (it != socket_list.end()) {
		CECServerSocket *s = *(it++);
		s->Close();
		s->Destroy();
	}
}


void ExternalConn::OnServerEvent(wxSocketEvent& WXUNUSED(event))
{
	CECServerSocket *sock = new CECServerSocket(m_ec_notifier);
	// Accept new connection if there is one in the pending
	// connections queue, else exit. We use Accept(FALSE) for
	// non-blocking accept (although if we got here, there
	// should ALWAYS be a pending connection).
	if ( m_ECServer->AcceptWith(*sock, false) ) {
		AddLogLineM(false, _("New external connection accepted"));
	} else {
		delete sock;
		AddLogLineM(false, _("ERROR: couldn't accept a new external connection"));
	}
	
}

//
// Authentication
//
const CECPacket *CECServerSocket::Authenticate(const CECPacket *request)
{
	CECPacket *response;
	
	if (request == NULL) {
		return new CECPacket(EC_OP_AUTH_FAIL);
	}

	// Password must be specified if we are to allow remote connections
	if ( thePrefs::ECPassword().IsEmpty() ) {
		AddLogLineM(true, _("External connection refused due to empty password in preferences!"));	
		
		return new CECPacket(EC_OP_AUTH_FAIL);
	}
		
	if ((m_conn_state == CONN_INIT) && (request->GetOpCode() == EC_OP_AUTH_REQ) ) {
		const CECTag *clientName = request->GetTagByName(EC_TAG_CLIENT_NAME);
		const CECTag *clientVersion = request->GetTagByName(EC_TAG_CLIENT_VERSION);
		
		AddLogLineM(false, CFormat( _("Connecting client: %s %s") )
			% ( clientName ? clientName->GetStringData() : wxString(_("Unknown")) )
			% ( clientVersion ? clientVersion->GetStringData() : wxString(_("Unknown version")) ) );
		const CECTag *protocol = request->GetTagByName(EC_TAG_PROTOCOL_VERSION);
#ifdef EC_VERSION_ID
		// For SVN versions, both client and server must use SVNDATE, and they must be the same
		CMD4Hash vhash;
		if (!vhash.Decode(wxT(EC_VERSION_ID))) {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxT("Fatal error, version hash is not a valid MD4-hash.")));
		} else if (!request->GetTagByName(EC_TAG_VERSION_ID) || request->GetTagByNameSafe(EC_TAG_VERSION_ID)->GetMD4Data() != vhash) {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Incorrect EC version ID, there might be binary incompatibility. Use core and remote from same snapshot.")));
#else
		// For release versions, we don't want to allow connections from any arbitrary SVN client.
		if (request->GetTagByName(EC_TAG_VERSION_ID)) { 
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("You cannot connect to a release version from an arbitrary SVN version! *sigh* possible crash prevented")));
#endif
		} else if (protocol != NULL) {
			uint16 proto_version = protocol->GetInt();
			if (proto_version == EC_CURRENT_PROTOCOL_VERSION) {
				response = new CECPacket(EC_OP_AUTH_SALT);
				response->AddTag(CECTag(EC_TAG_PASSWD_SALT, m_passwd_salt));
				m_conn_state = CONN_SALT_SENT;
			} else {
				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid protocol version.") + wxString::Format(wxT("( %i != %i )"),proto_version,EC_CURRENT_PROTOCOL_VERSION)));
			}
		} else {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Missing protocol version tag.")));
		}
	} else if ((m_conn_state == CONN_SALT_SENT) && (request->GetOpCode() == EC_OP_AUTH_PASSWD)) {
		const CECTag *passwd = request->GetTagByName(EC_TAG_PASSWD_HASH);
		CMD4Hash passh;

		if (!passh.Decode(thePrefs::ECPassword())) {
			wxString err = wxTRANSLATE("Authentication failed: invalid hash specified as EC password.");
			AddLogLineM(false, wxString(wxGetTranslation(err)) + wxT(" ") + thePrefs::ECPassword());
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, err));				
		} else {
			wxString saltHash = MD5Sum(CFormat(wxT("%lX")) % m_passwd_salt).GetHash();
			wxString saltStr = CFormat(wxT("%lX")) % m_passwd_salt;
			
			passh.Decode(MD5Sum(thePrefs::ECPassword().Lower() + saltHash).GetHash());
			
			if (passwd && passwd->GetMD4Data() == passh) {
				response = new CECPacket(EC_OP_AUTH_OK);
				response->AddTag(CECTag(EC_TAG_SERVER_VERSION, wxT(VERSION)));
			} else {
				wxString err;
				if (passwd) {
					err = wxTRANSLATE("Authentication failed: wrong password.");
				} else {
					err = wxTRANSLATE("Authentication failed: missing password.");
				}

				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, err));
				AddLogLineM(false, wxGetTranslation(err));
			}
		}
	} else {
		response = new CECPacket(EC_OP_AUTH_FAIL);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid request, please authenticate first.")));
	}
	
	if (response->GetOpCode() == EC_OP_AUTH_OK) {
		m_conn_state = CONN_ESTABLISHED;
		AddLogLineM(false, _("Access granted."));
		// Establish notification handler if client supports it
		if (HaveNotificationSupport()) {
			theApp->ECServerHandler->m_ec_notifier->Add_EC_Client(this);
		}
	} else if (response->GetOpCode() == EC_OP_AUTH_FAIL) {
		// Log message sent to client
		if (response->GetTagByIndex(0)->IsString()) {
			AddLogLineM(false, CFormat(_("Sent error message \"%s\" to client.")) % wxGetTranslation(response->GetTagByIndex(0)->GetStringData()));
		}
		// Access denied!
		AddLogLineM(false, _("Unauthorized access attempt. Connection closed."));
		m_conn_state = CONN_FAILED;
	}

	return response;
}

// Make a Logger tag (if there are any logging messages) and add it to the response
static void AddLoggerTag(CECPacket *response, CLoggerAccess &LoggerAccess)
{
	if (LoggerAccess.HasString()) {
		CECEmptyTag tag(EC_TAG_STATS_LOGGER_MESSAGE);
		// Tag structure is fix: tag carries nothing, inside are the strings
		// maximum of 200 log lines per message
		int entries = 0;
		wxString line;
		while (entries < 200 && LoggerAccess.GetString(line)) {
			tag.AddTag(CECTag(EC_TAG_STRING, line));
			entries++;
		}
		response->AddTag(tag);
		//printf("send Log tag %d %d\n", FirstEntry, entries);
	}
}

static CECPacket *Get_EC_Response_StatRequest(const CECPacket *request, CLoggerAccess &LoggerAccess)
{
	CECPacket *response = new CECPacket(EC_OP_STATS);

	switch (request->GetDetailLevel()) {
		case EC_DETAIL_FULL:
			response->AddTag(CECTag(EC_TAG_STATS_UP_OVERHEAD, (uint32)theStats::GetUpOverheadRate()));
			response->AddTag(CECTag(EC_TAG_STATS_DOWN_OVERHEAD, (uint32)theStats::GetDownOverheadRate()));
			response->AddTag(CECTag(EC_TAG_STATS_BANNED_COUNT, /*(uint32)*/theStats::GetBannedCount()));
			AddLoggerTag(response, LoggerAccess);
		case EC_DETAIL_WEB:
		case EC_DETAIL_CMD:
			response->AddTag(CECTag(EC_TAG_STATS_UL_SPEED, (uint32)theStats::GetUploadRate()));
			response->AddTag(CECTag(EC_TAG_STATS_DL_SPEED, (uint32)(theStats::GetDownloadRate())));
			response->AddTag(CECTag(EC_TAG_STATS_UL_SPEED_LIMIT, (uint32)(thePrefs::GetMaxUpload()*1024.0)));
			response->AddTag(CECTag(EC_TAG_STATS_DL_SPEED_LIMIT, (uint32)(thePrefs::GetMaxDownload()*1024.0)));
			response->AddTag(CECTag(EC_TAG_STATS_UL_QUEUE_LEN, /*(uint32)*/theStats::GetWaitingUserCount()));
			response->AddTag(CECTag(EC_TAG_STATS_TOTAL_SRC_COUNT, /*(uint32)*/theStats::GetFoundSources()));
			// User/Filecounts
			{
				uint32 totaluser = 0, totalfile = 0;
				theApp->serverlist->GetUserFileStatus( totaluser, totalfile );
				response->AddTag(CECTag(EC_TAG_STATS_ED2K_USERS, totaluser));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_USERS, Kademlia::CKademlia::GetKademliaUsers()));
				response->AddTag(CECTag(EC_TAG_STATS_ED2K_FILES, totalfile));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_FILES, Kademlia::CKademlia::GetKademliaFiles()));
			}
			// Kad stats
			if (Kademlia::CKademlia::IsConnected()) {
				response->AddTag(CECTag(EC_TAG_STATS_KAD_FIREWALLED_UDP, Kademlia::CUDPFirewallTester::IsFirewalledUDP(true)));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_SOURCES, Kademlia::CKademlia::GetIndexed()->m_totalIndexSource));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_KEYWORDS, Kademlia::CKademlia::GetIndexed()->m_totalIndexKeyword));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_NOTES, Kademlia::CKademlia::GetIndexed()->m_totalIndexNotes));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_LOAD, Kademlia::CKademlia::GetIndexed()->m_totalIndexLoad));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_IP_ADRESS, wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetPrefs()->GetIPAddress())));
				response->AddTag(CECTag(EC_TAG_STATS_BUDDY_STATUS, theApp->clientlist->GetBuddyStatus()));
				uint32 BuddyIP = 0;
				uint16 BuddyPort = 0;
				CUpDownClient * Buddy = theApp->clientlist->GetBuddy();
				if (Buddy) {
					BuddyIP = Buddy->GetIP();
					BuddyPort = Buddy->GetUDPPort();
				}
				response->AddTag(CECTag(EC_TAG_STATS_BUDDY_IP, BuddyIP));
				response->AddTag(CECTag(EC_TAG_STATS_BUDDY_PORT, BuddyPort));
			}
		case EC_DETAIL_UPDATE:
		case EC_DETAIL_INC_UPDATE:
			break;
	};

	return response;
}

static CECPacket *Get_EC_Response_GetSharedFiles(const CECPacket *request, CKnownFile_Encoder_Map &encoders)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_SHARED_FILES);

	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_KNOWNFILE> queryitems(request);

	encoders.UpdateEncoders(theApp->sharedfiles);
	
	for (uint32 i = 0; i < theApp->sharedfiles->GetFileCount(); ++i) {
		CKnownFile *cur_file = (CKnownFile *)theApp->sharedfiles->GetFileByIndex(i);

		if ( !cur_file || (!queryitems.empty() && !queryitems.count(cur_file->ECID())) ) {
			continue;
		}

		CEC_SharedFile_Tag filetag(cur_file, detail_level);
		CKnownFile_Encoder &enc = encoders[cur_file];
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc.ResetEncoder();
		}
		enc.Encode(&filetag);
		response->AddTag(filetag);
	}
	return response;
}

static CECPacket *Get_EC_Response_GetSharedFiles(CKnownFile_Encoder_Map &encoders, CObjTagMap &tagmap)
{
	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	encoders.UpdateEncoders(theApp->sharedfiles);
	for (uint32 i = 0; i < theApp->sharedfiles->GetFileCount(); ++i) {
		CKnownFile *cur_file = (CKnownFile *)theApp->sharedfiles->GetFileByIndex(i);
		
		//
		// Hashes of tags are maintained on "per-object" basis. So, in this mode only
		// same kind of objects can go into particular query type.
		// But files from download queue (aka partfiles) need to be listed both as downloads
		// and as shares (with different tag content).
		// So simply increment the object pointer - it's only a map key and never used for referencing.
		//
		void * mapKey = cur_file;
		if (!cur_file) continue;
		if (cur_file->IsPartFile()) {
			mapKey = (void *) ((char *)mapKey + 1);
		}

		CValueMap &valuemap = tagmap.GetValueMap(mapKey);
		CEC_SharedFile_Tag filetag(cur_file, EC_DETAIL_INC_UPDATE, &valuemap);
		CKnownFile_Encoder &enc = encoders[cur_file];
		enc.Encode(&filetag);
		
		response->AddTag(filetag);
	}
	return response;
}

static CECPacket *Get_EC_Response_GetClientQueue(const CECPacket *request, CObjTagMap &tagmap, int op)
{
	CECPacket *response = new CECPacket(op);
	
	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();

	//
	// request can contain list of queried items
	// (not for incremental update of course)
	CTagSet<uint32, EC_TAG_CLIENT> queryitems(request);

	const CClientPtrList& clients = (op == EC_OP_WAIT_QUEUE)	? theApp->uploadqueue->GetWaitingList()
																: theApp->uploadqueue->GetUploadingList();
	CClientPtrList::const_iterator it = clients.begin();
	for (; it != clients.end(); ++it) {
		CUpDownClient* cur_client = *it;

		if (!cur_client) {	// shouldn't happen
			continue;
		}
		if (!queryitems.empty() && !queryitems.count(cur_client->ECID())) {
			continue;
		}
		CValueMap *valuemap = NULL;
		if (detail_level == EC_DETAIL_INC_UPDATE) {
			valuemap = &tagmap.GetValueMap(cur_client);
		}
		CEC_UpDownClient_Tag cli_tag(cur_client, detail_level, valuemap);
		
		response->AddTag(cli_tag);
	}

	return response;
}


static CECPacket *Get_EC_Response_GetDownloadQueue(CPartFile_Encoder_Map &encoders, CObjTagMap &tagmap)
{	
	CECPacket *response = new CECPacket(EC_OP_DLOAD_QUEUE);

	encoders.UpdateEncoders(theApp->downloadqueue);
	for (unsigned int i = 0; i < theApp->downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp->downloadqueue->GetFileByIndex(i);

		CValueMap &valuemap = tagmap.GetValueMap(cur_file);
		CEC_PartFile_Tag filetag(cur_file, EC_DETAIL_INC_UPDATE, &valuemap);
		CPartFile_Encoder &enc = encoders[cur_file];
		enc.Encode(&filetag);
		
		response->AddTag(filetag);
	}	
	return 	response;
}

static CECPacket *Get_EC_Response_GetDownloadQueue(const CECPacket *request, CPartFile_Encoder_Map &encoders)
{	
	CECPacket *response = new CECPacket(EC_OP_DLOAD_QUEUE);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_PARTFILE> queryitems(request);
	
	encoders.UpdateEncoders(theApp->downloadqueue);

	for (unsigned int i = 0; i < theApp->downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp->downloadqueue->GetFileByIndex(i);
	
		if ( !queryitems.empty() && !queryitems.count(cur_file->ECID()) ) {
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


static CECPacket *Get_EC_Response_PartFile_Cmd(const CECPacket *request)
{
	CECPacket *response = NULL;

	// request can contain multiple files.
	for (unsigned int i = 0; i < request->GetTagCount(); ++i) {
		const CECTag *hashtag = request->GetTagByIndex(i);

		wxASSERT(hashtag->GetTagName() == EC_TAG_PARTFILE);

		CMD4Hash hash = hashtag->GetMD4Data();
		CPartFile *pfile = theApp->downloadqueue->GetFileByID( hash );
		
		if ( !pfile ) {
			AddLogLineM(false,CFormat(_("Remote PartFile command failed: FileHash not found: %s")) % hash.Encode());
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING, CFormat(wxString(wxTRANSLATE("FileHash not found: %s"))) % hash.Encode()));
			//return response;
			break;
		}
		switch (request->GetOpCode()) {
			case EC_OP_PARTFILE_SWAP_A4AF_THIS:
				if ((pfile->GetStatus(false) == PS_READY) ||
					(pfile->GetStatus(false) == PS_EMPTY)) {
					CPartFile::SourceSet::const_iterator it = pfile->GetA4AFList().begin();
					while ( it != pfile->GetA4AFList().end() ) {
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
					CPartFile::SourceSet::const_iterator it = pfile->GetSourceList().begin();
					while ( it != pfile->GetSourceList().end() ) {
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
					uint8 prio = hashtag->GetTagByIndexSafe(0)->GetInt();
					if ( prio == PR_AUTO ) {
						pfile->SetAutoDownPriority(1);
					} else {
						pfile->SetAutoDownPriority(0);
						pfile->SetDownPriority(prio);
					}
				}
				break;
			case EC_OP_PARTFILE_DELETE:
				if ( thePrefs::StartNextFile() && (pfile->GetStatus() != PS_PAUSED) ) {
					theApp->downloadqueue->StartNextFile(pfile);
				}
				pfile->Delete();
				break;

			case EC_OP_PARTFILE_SET_CAT:
				pfile->SetCategory(hashtag->GetTagByIndexSafe(0)->GetInt());
				break;

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

static CECPacket *Get_EC_Response_Server_Add(const CECPacket *request)
{
	CECPacket *response = NULL;

	const CECTag *srv_tag = request->GetTagByIndex(0);

	wxString full_addr = srv_tag->GetTagByName(EC_TAG_SERVER_ADDRESS)->GetStringData();
	wxString name = srv_tag->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData();
	
	wxString s_ip = full_addr.Left(full_addr.Find(':'));
	wxString s_port = full_addr.Mid(full_addr.Find(':') + 1);
	
	long port = StrToULong(s_port);
	CServer* toadd = new CServer(port, s_ip);
	toadd->SetListName(name.IsEmpty() ? full_addr : name);
	
	if ( theApp->AddServer(toadd, true) ) {
		response = new CECPacket(EC_OP_NOOP);
	} else {
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, _("Server not added")));
		delete toadd;
	}
	
	return response;
}

static CECPacket *Get_EC_Response_Server(const CECPacket *request)
{
	CECPacket *response = NULL;
	const CECTag *srv_tag = request->GetTagByIndex(0);
	CServer *srv = 0;
	if ( srv_tag ) {
		srv = theApp->serverlist->GetServerByIPTCP(srv_tag->GetIPv4Data().IP(), srv_tag->GetIPv4Data().m_port);
		// server tag passed, but server not found
		if ( !srv ) {
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING,
						CFormat(wxString(wxTRANSLATE("server not found: %s"))) % srv_tag->GetIPv4Data().StringIP()));
			return response;
		}
	}
	switch (request->GetOpCode()) {
		case EC_OP_SERVER_DISCONNECT:
			theApp->serverconnect->Disconnect();
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_SERVER_REMOVE:
			if ( srv ) {
				theApp->serverlist->RemoveServer(srv);
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING,
							wxTRANSLATE("need to define server to be removed")));
			}
			break;
		case EC_OP_SERVER_CONNECT:
			if (thePrefs::GetNetworkED2K()) {
				if ( srv ) {
					theApp->serverconnect->ConnectToServer(srv);
					response = new CECPacket(EC_OP_NOOP);
				} else {
					theApp->serverconnect->ConnectToAnyServer();
					response = new CECPacket(EC_OP_NOOP);
				}
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("eD2k is disabled in preferences.")));
			}
			break;
	}
	if (!response) {
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("OOPS! OpCode processing error!")));
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Results(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_SEARCH_RESULTS);
		
	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<CMD4Hash, EC_TAG_SEARCHFILE> queryitems(request);

	const CSearchResultList& list = theApp->searchlist->GetSearchResults(0xffffffff);
	CSearchResultList::const_iterator it = list.begin();
	while (it != list.end()) {
		CSearchFile* sf = *it++;
		if ( !queryitems.empty() && !queryitems.count(sf->GetFileHash()) ) {
			continue;
		}
		response->AddTag(CEC_SearchFile_Tag(sf, detail_level));
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Results(CObjTagMap &tagmap)
{
	CECPacket *response = new CECPacket(EC_OP_SEARCH_RESULTS);

	const CSearchResultList& list = theApp->searchlist->GetSearchResults(0xffffffff);
	CSearchResultList::const_iterator it = list.begin();
	while (it != list.end()) {
		CSearchFile* sf = *it++;
		CValueMap &valuemap = tagmap.GetValueMap(sf);
		response->AddTag(CEC_SearchFile_Tag(sf, EC_DETAIL_INC_UPDATE, &valuemap));
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Results_Download(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);
	for (unsigned int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		CMD4Hash hash = tag->GetMD4Data();
		uint8 category = tag->GetTagByIndexSafe(0)->GetInt();
		theApp->searchlist->AddFileToDownloadByHash(hash, category);
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Stop(const CECPacket *WXUNUSED(request))
{
	CECPacket *reply = new CECPacket(EC_OP_MISC_DATA);
	theApp->searchlist->StopGlobalSearch();
	return reply;
}

static CECPacket *Get_EC_Response_Search(const CECPacket *request)
{
	wxString response;
	
	CEC_Search_Tag *search_request = (CEC_Search_Tag *)request->GetTagByIndex(0);
	theApp->searchlist->RemoveResults(0xffffffff);

	CSearchList::CSearchParams params;
	params.searchString	= search_request->SearchText();
	params.typeText		= search_request->SearchFileType();
	params.extension	= search_request->SearchExt();
	params.minSize		= search_request->MinSize();
	params.maxSize		= search_request->MaxSize();
	params.availability	= search_request->Avail();
	
	
	EC_SEARCH_TYPE search_type = search_request->SearchType();
	SearchType core_search_type = LocalSearch;
	switch (search_type) {
		case EC_SEARCH_GLOBAL:
			core_search_type = GlobalSearch;
		case EC_SEARCH_KAD:
			if (core_search_type != GlobalSearch) { // Not a global search obviously
				core_search_type = KadSearch;
			}
		case EC_SEARCH_LOCAL: {
			uint32 search_id = 0xffffffff;
			wxString error = theApp->searchlist->StartNewSearch(&search_id, core_search_type, params);
			if (!error.IsEmpty()) {
				response = error;
			} else {
				response = wxTRANSLATE("Search in progress. Refetch results in a moment!");			
			}
			break;
		}
		case EC_SEARCH_WEB:
				response = wxTRANSLATE("WebSearch from remote interface makes no sense.");
			break;
	}
	
	CECPacket *reply = new CECPacket(EC_OP_FAILED);
	// error or search in progress
	reply->AddTag(CECTag(EC_TAG_STRING, response));
		
	return reply;
}

static CECPacket *Get_EC_Response_Set_SharedFile_Prio(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_NOOP);
	for (unsigned int i = 0;i < request->GetTagCount();i++) {
		const CECTag *tag = request->GetTagByIndex(i);
		CMD4Hash hash = tag->GetMD4Data();
		uint8 prio = tag->GetTagByIndexSafe(0)->GetInt();
		CKnownFile* cur_file = theApp->sharedfiles->GetFileByID(hash);
		if ( !cur_file ) {
			continue;
		}
		if (prio == PR_AUTO) {
			cur_file->SetAutoUpPriority(1);
			cur_file->UpdateAutoUpPriority();
		} else {
			cur_file->SetAutoUpPriority(0);
			cur_file->SetUpPriority(prio);
		}
		Notify_SharedFilesUpdateItem(cur_file);
	}

	return response;
}

static CECPacket *Get_EC_Response_Kad_Connect(const CECPacket *request)
{
	CECPacket *response;
	if (thePrefs::GetNetworkKademlia()) {
		response = new CECPacket(EC_OP_NOOP);
		if ( !Kademlia::CKademlia::IsRunning() ) {
			Kademlia::CKademlia::Start();
			theApp->ShowConnectionState();
		}
		const CECTag *addrtag = request->GetTagByIndex(0);
		if ( addrtag ) {
			uint32 ip = addrtag->GetIPv4Data().IP();
			uint16 port = addrtag->GetIPv4Data().m_port;
			Kademlia::CKademlia::Bootstrap(ip, port, true);
		}
	} else {
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Kad is disabled in preferences.")));
	}

	return response;
}

void CPartFile_Encoder::Encode(CECTag *parent)
{
	//
	// Source part frequencies
	//
	// These are not always populated, don't send a tag in this case
	//
	if (!m_file->m_SrcpartFrequency.empty()) {
		int part_enc_size;
		bool changed;
		const uint8 *part_enc_data = m_part_status.Encode(m_file->m_SrcpartFrequency, part_enc_size, changed);
		if (changed) {
			parent->AddTag(CECTag(EC_TAG_PARTFILE_PART_STATUS, part_enc_size, part_enc_data));
		}
		delete[] part_enc_data;
	}

	//
	// Gaps
	//
	const CGapList& gaplist = m_file->GetNewGapList();
	const size_t gap_list_size = gaplist.size();
	ArrayOfUInts64 gaps;
	gaps.reserve(gap_list_size * 2);
	
	for (CGapList::const_iterator curr_pos = gaplist.begin(); 
			curr_pos != gaplist.end(); ++curr_pos) {
		gaps.push_back(curr_pos.start());
		gaps.push_back(curr_pos.end());
	}

	int gap_enc_size = 0;
	bool changed;
	const uint8 *gap_enc_data = m_gap_status.Encode(gaps, gap_enc_size, changed);
	if (changed) {
		parent->AddTag(CECTag(EC_TAG_PARTFILE_GAP_STATUS, gap_enc_size, (void *)gap_enc_data));
	}
	delete[] gap_enc_data;
	
	//
	// Requested blocks
	//
	ArrayOfUInts64 req_buffer;
	const CPartFile::CReqBlockPtrList& requestedblocks = m_file->GetRequestedBlockList();
	CPartFile::CReqBlockPtrList::const_iterator curr_pos2 = requestedblocks.begin();

	for ( ; curr_pos2 != requestedblocks.end(); ++curr_pos2 ) {
		Requested_Block_Struct* block = *curr_pos2;
		req_buffer.push_back(block->StartOffset);
		req_buffer.push_back(block->EndOffset);
	}
	int req_enc_size = 0;
	const uint8 *req_enc_data = m_req_status.Encode(req_buffer, req_enc_size, changed);
	if (changed) {
		parent->AddTag(CECTag(EC_TAG_PARTFILE_REQ_STATUS, req_enc_size, (void *)req_enc_data));
	}
	delete[] req_enc_data;

	//
	// Source names
	//
	// First count occurrence of all source names
	//
	CECEmptyTag sourceNames(EC_TAG_PARTFILE_SOURCE_NAMES);
	typedef std::map<wxString, int> strIntMap;
	strIntMap nameMap;
	const CPartFile::SourceSet &sources = m_file->GetSourceList();
	for (CPartFile::SourceSet::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		CUpDownClient *cur_src = *it; 
		if (cur_src->GetRequestFile() != m_file || cur_src->GetClientFilename().Length() == 0) {
			continue;
		}
		const wxString &name = cur_src->GetClientFilename();
		strIntMap::iterator itm = nameMap.find(name);
		if (itm == nameMap.end()) {
			nameMap[name] = 1;
		} else {
			itm->second++;
		}
	}
	//
	// Go through our last list
	//
	for (SourcenameItemMap::iterator it1 = m_sourcenameItemMap.begin(); it1 != m_sourcenameItemMap.end();) {
		SourcenameItemMap::iterator it2 = it1++;
		strIntMap::iterator itm = nameMap.find(it2->second.name);
		if (itm == nameMap.end()) {
			// name doesn't exist anymore, tell client to forget it
			CECTag tag(EC_TAG_PARTFILE_SOURCE_NAMES, it2->first);
			tag.AddTag(CECIntTag(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS, 0));
			sourceNames.AddTag(tag);
			// and forget it
			m_sourcenameItemMap.erase(it2);
		} else {
			// update count if it changed
			if (it2->second.count != itm->second) {
				CECTag tag(EC_TAG_PARTFILE_SOURCE_NAMES, it2->first);
				tag.AddTag(CECIntTag(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS, itm->second));
				sourceNames.AddTag(tag);
				it2->second.count = itm->second;
			}
			// remove it from nameMap so that only new names are left there
			nameMap.erase(itm);
		}
	}
	//
	// Add new names
	//
	for (strIntMap::iterator it3 = nameMap.begin(); it3 != nameMap.end(); it3++) {
		int id = ++m_sourcenameID;
		CECIntTag tag(EC_TAG_PARTFILE_SOURCE_NAMES, id);
		tag.AddTag(CECTag(EC_TAG_PARTFILE_SOURCE_NAMES, it3->first));
		tag.AddTag(CECIntTag(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS, it3->second));
		sourceNames.AddTag(tag);
		// remember it
		m_sourcenameItemMap[id] = SourcenameItem(it3->first, it3->second);
	}
	if (sourceNames.HasChildTags()) {
		parent->AddTag(sourceNames);
	}

}

void CPartFile_Encoder::ResetEncoder()
{
	m_part_status.ResetEncoder();
	m_gap_status.ResetEncoder();
	m_req_status.ResetEncoder();
}

void CKnownFile_Encoder::Encode(CECTag *parent)
{
	// Reference to the availability list
	const ArrayOfUInts16& list = m_file->IsPartFile() ?
		((CPartFile*)m_file)->m_SrcpartFrequency :
		m_file->m_AvailPartFrequency;
	// Don't add tag if available parts aren't populated yet.
	if (!list.empty()) {
		int part_enc_size;
		bool changed;
		const uint8 *part_enc_data = m_enc_data.Encode(list, part_enc_size, changed);
		if (changed) {
			parent->AddTag(CECTag(EC_TAG_PARTFILE_PART_STATUS, part_enc_size, part_enc_data));
		}
		delete[] part_enc_data;
	}
}

static CECPacket *GetStatsGraphs(const CECPacket *request)
{
	CECPacket *response = NULL;

	switch (request->GetDetailLevel()) {
		case EC_DETAIL_WEB:
		case EC_DETAIL_FULL: {
			double dTimestamp = 0.0;
			if (request->GetTagByName(EC_TAG_STATSGRAPH_LAST) != NULL) {
				dTimestamp = request->GetTagByName(EC_TAG_STATSGRAPH_LAST)->GetDoubleData();
			}
			uint16 nScale = request->GetTagByNameSafe(EC_TAG_STATSGRAPH_SCALE)->GetInt();
			uint16 nMaxPoints = request->GetTagByNameSafe(EC_TAG_STATSGRAPH_WIDTH)->GetInt();
			uint32 *graphData;
			unsigned int numPoints = theApp->m_statistics->GetHistoryForWeb(nMaxPoints, (double)nScale, &dTimestamp, &graphData);
			if (numPoints) {
				response = new CECPacket(EC_OP_STATSGRAPHS);
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_DATA, 4 * numPoints * sizeof(uint32), graphData));
				delete [] graphData;
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_LAST, dTimestamp));
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("No points for graph.")));
			}
			break;
		}
		case EC_DETAIL_INC_UPDATE:
		case EC_DETAIL_UPDATE:
		case EC_DETAIL_CMD:
			// No graphs
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Your client is not configured for this detail level.")));
			break;
	}
	if (!response) {
		response = new CECPacket(EC_OP_FAILED);
		// Unknown reason
	}

	return response;
}

CECPacket *CECServerSocket::ProcessRequest2(const CECPacket *request,
	CPartFile_Encoder_Map &enc_part_map, CKnownFile_Encoder_Map &enc_shared_map, CObjTagMap &objmap)
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
			if (!theApp->IsOnShutDown()) {
				response = new CECPacket(EC_OP_NOOP);
				AddLogLineM(true, _("External Connection: shutdown requested"));
#ifndef AMULE_DAEMON
				{
					wxCloseEvent evt;
					evt.SetCanVeto(false);
					theApp->ShutDown(evt);
				}
#else
				theApp->ExitMainLoop();
#endif
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Already shutting down.")));
			}
			break;
		case EC_OP_ADD_LINK: 
			for(unsigned int i = 0; i < request->GetTagCount();i++) {
				const CECTag *tag = request->GetTagByIndex(i);
				wxString link = tag->GetStringData();
				int category = 0;
				const CECTag *cattag = tag->GetTagByName(EC_TAG_PARTFILE_CAT);
				if (cattag) {
					category = cattag->GetInt();
				}
				AddLogLineM(true, CFormat(_("ExternalConn: adding link '%s'.")) % link);
				if ( theApp->downloadqueue->AddLink(link, category) ) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					// Error messages are printed by the add function.
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid link or already on list.")));
				}
			}
			break;
		//
		// Status requests
		//
		case EC_OP_STAT_REQ:
			response = Get_EC_Response_StatRequest(request, m_LoggerAccess);
			response->AddTag(CEC_ConnState_Tag(request->GetDetailLevel()));
			break;
		case EC_OP_GET_CONNSTATE:
			response = new CECPacket(EC_OP_MISC_DATA);
			response->AddTag(CEC_ConnState_Tag(request->GetDetailLevel()));
			break;
		//
		//
		//
		case EC_OP_GET_SHARED_FILES:
			if ( request->GetDetailLevel() == EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetSharedFiles(enc_shared_map, objmap);
			} else {
				response = Get_EC_Response_GetSharedFiles(request, enc_shared_map);
			}
			break;
		case EC_OP_GET_DLOAD_QUEUE:
			if ( request->GetDetailLevel() == EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetDownloadQueue(enc_part_map, objmap);
			} else {
				response = Get_EC_Response_GetDownloadQueue(request, enc_part_map);
			}
			break;
		case EC_OP_GET_ULOAD_QUEUE:
			response = Get_EC_Response_GetClientQueue(request, objmap, EC_OP_ULOAD_QUEUE);
			break;
		case EC_OP_GET_WAIT_QUEUE:
			response = Get_EC_Response_GetClientQueue(request, objmap, EC_OP_WAIT_QUEUE);
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
			theApp->sharedfiles->Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_SHARED_SET_PRIO:
			response = Get_EC_Response_Set_SharedFile_Prio(request);
			break;
		case EC_OP_RENAME_FILE: {
			CMD4Hash fileHash = request->GetTagByNameSafe(EC_TAG_KNOWNFILE)->GetMD4Data();
			CKnownFile* file = theApp->knownfiles->FindKnownFileByID(fileHash);
			wxString newName = request->GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData();
			if (!file) {
				file = theApp->downloadqueue->GetFileByID(fileHash);
			}
			if (!file) {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("File not found.")));
				break;
			}
			if (newName.IsEmpty()) {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid file name.")));
				break;
			}
			
			if (theApp->sharedfiles->RenameFile(file, CPath(newName))) {
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Unable to rename file.")));
			}

			break;
		}


		//
		// Server commands
		//
		case EC_OP_SERVER_ADD:
			response = Get_EC_Response_Server_Add(request);
			break;
		case EC_OP_SERVER_DISCONNECT:
		case EC_OP_SERVER_CONNECT:
		case EC_OP_SERVER_REMOVE:
			response = Get_EC_Response_Server(request);
			break;
		case EC_OP_GET_SERVER_LIST: {
				response = new CECPacket(EC_OP_SERVER_LIST);
				if (!thePrefs::GetNetworkED2K()) {
					// Kad only: just send an empty list
					break;
				}
				EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
				std::vector<const CServer*> servers = theApp->serverlist->CopySnapshot();
				for (
					std::vector<const CServer*>::const_iterator it = servers.begin();
					it != servers.end();
					++it
					) {
					response->AddTag(CEC_Server_Tag(*it, detail_level));
				}
			}
			break;
		case EC_OP_SERVER_UPDATE_FROM_URL: {
			wxString url = request->GetTagByIndexSafe(0)->GetStringData();

			// Save the new url, and update the UI (if not amuled).
			Notify_ServersURLChanged(url);
			thePrefs::SetEd2kServersUrl(url);

			theApp->serverlist->UpdateServerMetFromURL(url);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		//
		// IPFilter
		//
		case EC_OP_IPFILTER_RELOAD:
			theApp->ipfilter->Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;

		case EC_OP_IPFILTER_UPDATE: {
			wxString url = request->GetTagByIndexSafe(0)->GetStringData();
			if (url == wxEmptyString) {
				url = thePrefs::IPFilterURL();
			}
			theApp->ipfilter->Update(url);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		//
		// Search
		//
		case EC_OP_SEARCH_START:
			response = Get_EC_Response_Search(request);
			break;

		case EC_OP_SEARCH_STOP:
			response = Get_EC_Response_Search_Stop(request);
			break;

		case EC_OP_SEARCH_RESULTS:
			if ( request->GetDetailLevel() == EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_Search_Results(objmap);
			} else {
				response = Get_EC_Response_Search_Results(request);
			}
			break;
			
		case EC_OP_SEARCH_PROGRESS:
			response = new CECPacket(EC_OP_SEARCH_PROGRESS);
			response->AddTag(CECTag(EC_TAG_SEARCH_STATUS,
				theApp->searchlist->GetSearchProgress()));
			break;
			
		case EC_OP_DOWNLOAD_SEARCH_RESULT:
			response = Get_EC_Response_Search_Results_Download(request);
			break;
		//
		// Preferences
		//
		case EC_OP_GET_PREFERENCES:
			response = new CEC_Prefs_Packet(request->GetTagByNameSafe(EC_TAG_SELECT_PREFS)->GetInt(), request->GetDetailLevel());
			break;
		case EC_OP_SET_PREFERENCES:
			((CEC_Prefs_Packet *)request)->Apply();
			theApp->glob_prefs->Save();
			if (thePrefs::IsFilteringClients()) {
				theApp->clientlist->FilterQueues();
			}
			if (thePrefs::IsFilteringServers()) {
				theApp->serverlist->FilterServers();
			}
			if (!thePrefs::GetNetworkED2K() && theApp->IsConnectedED2K()) {
				theApp->DisconnectED2K();
			}
			if (!thePrefs::GetNetworkKademlia() && theApp->IsConnectedKad()) {
				theApp->StopKad();
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
			
		case EC_OP_CREATE_CATEGORY:
			if ( request->GetTagCount() == 1 ) {
				CEC_Category_Tag *tag = (CEC_Category_Tag *)request->GetTagByIndex(0);
				if (tag->Create()) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_CATEGORY, theApp->glob_prefs->GetCatCount() - 1));
					response->AddTag(CECTag(EC_TAG_CATEGORY_PATH, tag->Path()));
				}
				Notify_CategoryAdded();
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
		case EC_OP_UPDATE_CATEGORY:
			if ( request->GetTagCount() == 1 ) {
				CEC_Category_Tag *tag = (CEC_Category_Tag *)request->GetTagByIndex(0);
				if (tag->Apply()) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_CATEGORY, tag->GetInt()));
					response->AddTag(CECTag(EC_TAG_CATEGORY_PATH, tag->Path()));
				}
				Notify_CategoryUpdate(tag->GetInt());
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
		case EC_OP_DELETE_CATEGORY:
			if ( request->GetTagCount() == 1 ) {
				uint32 cat = request->GetTagByIndex(0)->GetInt();
				// this noes not only update the gui, but actually deletes the cat
				Notify_CategoryDelete(cat);
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		
		//
		// Logging
		//
		case EC_OP_ADDLOGLINE:
			AddLogLineM( (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL), request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData() );
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_ADDDEBUGLOGLINE:
			AddDebugLogLineM( (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL), logGeneral, request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData() );
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_GET_LOG:
			response = new CECPacket(EC_OP_LOG);
			response->AddTag(CECTag(EC_TAG_STRING, theApp->GetLog(false)));
			break;
		case EC_OP_GET_DEBUGLOG:
			response = new CECPacket(EC_OP_DEBUGLOG);
			response->AddTag(CECTag(EC_TAG_STRING, theApp->GetDebugLog(false)));
			break;
		case EC_OP_RESET_LOG:
			theApp->GetLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_RESET_DEBUGLOG:
			theApp->GetDebugLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_GET_LAST_LOG_ENTRY:
			{
				wxString tmp = theApp->GetLog(false);
				if (tmp.Last() == '\n') {
					tmp.RemoveLast();
				}
				response = new CECPacket(EC_OP_LOG);
				response->AddTag(CECTag(EC_TAG_STRING, tmp.AfterLast('\n')));
			}
			break;
		case EC_OP_GET_SERVERINFO:
			response = new CECPacket(EC_OP_SERVERINFO);
			response->AddTag(CECTag(EC_TAG_STRING, theApp->GetServerLog(false)));
			break;
		case EC_OP_CLEAR_SERVERINFO:
			theApp->GetServerLog(true);
			response = new CECPacket(EC_OP_NOOP);
			break;
		//
		// Statistics
		//
		case EC_OP_GET_STATSGRAPHS:
			response = GetStatsGraphs(request);
			break;
		case EC_OP_GET_STATSTREE: {
			theApp->m_statistics->UpdateStatsTree();
			response = new CECPacket(EC_OP_STATSTREE);
			CECTag* tree = theStats::GetECStatTree(request->GetTagByNameSafe(EC_TAG_STATTREE_CAPPING)->GetInt());
			if (tree) {
				response->AddTag(*tree);
				delete tree;
			}
			if (request->GetDetailLevel() == EC_DETAIL_WEB) {
				response->AddTag(CECTag(EC_TAG_SERVER_VERSION, wxT(VERSION)));
				response->AddTag(CECTag(EC_TAG_USER_NICK, thePrefs::GetUserNick()));
			}
			break;
		}
		
		//
		// Kad
		//
		case EC_OP_KAD_START:
			response = Get_EC_Response_Kad_Connect(request);
			break;
		case EC_OP_KAD_STOP:
			theApp->StopKad();
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_KAD_UPDATE_FROM_URL: {
			wxString url = request->GetTagByIndexSafe(0)->GetStringData();

			// Save the new url, and update the UI (if not amuled).
			Notify_NodesURLChanged(url);
			thePrefs::SetKadNodesUrl(url);

			theApp->UpdateNotesDat(url);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		case EC_OP_KAD_BOOTSTRAP_FROM_IP:
			theApp->BootstrapKad(request->GetTagByIndexSafe(0)->GetInt(),
			                     request->GetTagByIndexSafe(1)->GetInt());
			response = new CECPacket(EC_OP_NOOP);
			break;

		//
		// Networks
		//
		case EC_OP_CONNECT:
			if (thePrefs::GetNetworkED2K()) {
				response = new CECPacket(EC_OP_STRINGS);
				if (theApp->IsConnectedED2K()) {
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Already connected to eD2k.")));
				} else {
					theApp->serverconnect->ConnectToAnyServer();
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Connecting to eD2k...")));
				}
			}
			if (thePrefs::GetNetworkKademlia()) {
				if (!response) {
					response = new CECPacket(EC_OP_STRINGS);
				}
				if (theApp->IsConnectedKad()) {
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Already connected to Kad.")));
				} else {
					theApp->StartKad();
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Connecting to Kad...")));
				}
			}
			if (!response) {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("All networks are disabled.")));
			}
			break;
		case EC_OP_DISCONNECT:
			if (theApp->IsConnected()) {
				response = new CECPacket(EC_OP_STRINGS);
				if (theApp->IsConnectedED2K()) {
					theApp->serverconnect->Disconnect();
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Disconnected from eD2k.")));
				}
				if (theApp->IsConnectedKad()) {
					theApp->StopKad();
					response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Disconnected from Kad.")));
				}
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;

		default:
			AddLogLineM(false, wxString::Format(_("External Connection: invalid opcode received: %#x"), request->GetOpCode()));
			wxFAIL;
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid opcode (wrong protocol version?)")));
			break;
	}
	return response;
}

/*
 * Here notification-based EC. Notification will be sorted by priority for possible throttling. 
 */
 
/*
 * Core general status
 */
ECStatusMsgSource::ECStatusMsgSource()
{
	m_last_ed2k_status_sent = 0xffffffff;
	m_last_kad_status_sent = 0xffffffff;
	m_server = (void *)0xffffffff;
}

uint32 ECStatusMsgSource::GetEd2kStatus()
{
	if ( theApp->IsConnectedED2K() ) {
		return theApp->GetED2KID();
	} else if ( theApp->serverconnect->IsConnecting() ) {
		return 1;
	} else {
		return 0;
	}
}

uint32 ECStatusMsgSource::GetKadStatus()
{
	if ( theApp->IsConnectedKad() ) {
		return 1;
	} else if ( Kademlia::CKademlia::IsFirewalled() ) {
		return 2;
	} else if ( Kademlia::CKademlia::IsRunning() ) {
		return 3;
	}
	return 0;
}

CECPacket *ECStatusMsgSource::GetNextPacket()
{
	if ( (m_last_ed2k_status_sent != GetEd2kStatus()) ||
		(m_last_kad_status_sent != GetKadStatus()) ||
		(m_server != theApp->serverconnect->GetCurrentServer()) ) {

		m_last_ed2k_status_sent = GetEd2kStatus();
		m_last_kad_status_sent = GetKadStatus();
		m_server = theApp->serverconnect->GetCurrentServer();
		
		CECPacket *response = new CECPacket(EC_OP_STATS);
		response->AddTag(CEC_ConnState_Tag(EC_DETAIL_UPDATE));
		return response;
	}
	return 0;
}

/*
 * Downloading files
*/
ECPartFileMsgSource::ECPartFileMsgSource()
{
	for (unsigned int i = 0; i < theApp->downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp->downloadqueue->GetFileByIndex(i);
		PARTFILE_STATUS status = { true, false, false, false, true, cur_file };
		m_dirty_status[cur_file->GetFileHash()] = status;
	}
}

void ECPartFileMsgSource::SetDirty(CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	if ( m_dirty_status.find(filehash) != m_dirty_status.end() ) {
		m_dirty_status[filehash].m_dirty = true;;
	}
}

void ECPartFileMsgSource::SetNew(CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) == m_dirty_status.end() );
	PARTFILE_STATUS status = { true, false, false, false, true, file };
	m_dirty_status[filehash] = status;
}

void ECPartFileMsgSource::SetCompleted(CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) != m_dirty_status.end() );

	m_dirty_status[filehash].m_finished = true;
}

void ECPartFileMsgSource::SetRemoved(CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) != m_dirty_status.end() );

	m_dirty_status[filehash].m_removed = true;
}

CECPacket *ECPartFileMsgSource::GetNextPacket()
{
	for(std::map<CMD4Hash, PARTFILE_STATUS>::iterator it = m_dirty_status.begin();
		it != m_dirty_status.end(); it++) {
		if ( it->second.m_new || it->second.m_dirty || it->second.m_removed) {
			CMD4Hash filehash = it->first;
			
			CPartFile *partfile = it->second.m_file;

			CECPacket *packet = new CECPacket(EC_OP_DLOAD_QUEUE);
			if ( it->second.m_removed ) {
				CECTag tag(EC_TAG_PARTFILE, filehash);
				packet->AddTag(tag);
				m_dirty_status.erase(it);
			} else {
				CEC_PartFile_Tag tag(partfile, it->second.m_new ? EC_DETAIL_FULL : EC_DETAIL_UPDATE);
				packet->AddTag(tag);
			}
			m_dirty_status[filehash].m_new = false;
			m_dirty_status[filehash].m_dirty = false;

			return packet;
		}
	}
	return 0;
}

/*
 * Shared files - similar to downloading
 */
ECKnownFileMsgSource::ECKnownFileMsgSource()
{
	for (unsigned int i = 0; i < theApp->sharedfiles->GetFileCount(); i++) {
		CKnownFile *cur_file = (CKnownFile *)theApp->sharedfiles->GetFileByIndex(i);
		KNOWNFILE_STATUS status = { true, false, false, true, cur_file };
		m_dirty_status[cur_file->GetFileHash()] = status;
	}
}

void ECKnownFileMsgSource::SetDirty(CKnownFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	if ( m_dirty_status.find(filehash) != m_dirty_status.end() ) {
		m_dirty_status[filehash].m_dirty = true;;
	}
}

void ECKnownFileMsgSource::SetNew(CKnownFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) == m_dirty_status.end() );
	KNOWNFILE_STATUS status = { true, false, false, true, file };
	m_dirty_status[filehash] = status;
}
	
void ECKnownFileMsgSource::SetRemoved(CKnownFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) != m_dirty_status.end() );
	
	m_dirty_status[filehash].m_removed = true;
}

CECPacket *ECKnownFileMsgSource::GetNextPacket()
{
	for(std::map<CMD4Hash, KNOWNFILE_STATUS>::iterator it = m_dirty_status.begin();
		it != m_dirty_status.end(); it++) {
		if ( it->second.m_new || it->second.m_dirty || it->second.m_removed) {
			CMD4Hash filehash = it->first;
			
			CKnownFile *partfile = it->second.m_file;
			
			CECPacket *packet = new CECPacket(EC_OP_SHARED_FILES);
			if ( it->second.m_removed ) {
				CECTag tag(EC_TAG_PARTFILE, filehash);
				packet->AddTag(tag);
				m_dirty_status.erase(it);
			} else {
				CEC_SharedFile_Tag tag(partfile, it->second.m_new ? EC_DETAIL_FULL : EC_DETAIL_UPDATE);
				packet->AddTag(tag);
			}
			m_dirty_status[filehash].m_new = false;
			m_dirty_status[filehash].m_dirty = false;
			
			return packet;
		}
	}
	return 0;
}
	
/*
 * Notification about search status
*/
ECSearchMsgSource::ECSearchMsgSource()
{
}

CECPacket *ECSearchMsgSource::GetNextPacket()
{
	if ( m_dirty_status.empty() ) {
		return 0;
	}
	
	CECPacket *response = new CECPacket(EC_OP_SEARCH_RESULTS);
	for(std::map<CMD4Hash, SEARCHFILE_STATUS>::iterator it = m_dirty_status.begin();
		it != m_dirty_status.end(); it++) {
		
		if ( it->second.m_new ) {
			response->AddTag(CEC_SearchFile_Tag(it->second.m_file, EC_DETAIL_FULL));
			it->second.m_new = false;
		} else if ( it->second.m_dirty ) {
			response->AddTag(CEC_SearchFile_Tag(it->second.m_file, EC_DETAIL_UPDATE));
		}
		
	}
	
	return response;
}

void ECSearchMsgSource::FlushStatus()
{
	m_dirty_status.clear();
}

void ECSearchMsgSource::SetDirty(CSearchFile *file)
{
	if ( m_dirty_status.count(file->GetFileHash()) ) {
		m_dirty_status[file->GetFileHash()].m_dirty = true;
	} else {
		m_dirty_status[file->GetFileHash()].m_new = true;
		m_dirty_status[file->GetFileHash()].m_dirty = true;
		m_dirty_status[file->GetFileHash()].m_child_dirty = true;
		m_dirty_status[file->GetFileHash()].m_file = file;		
	}
}

void ECSearchMsgSource::SetChildDirty(CSearchFile *file)
{
	m_dirty_status[file->GetFileHash()].m_child_dirty = true;
}
	
/*
 * Notification about uploading clients
 */
CECPacket *ECClientMsgSource::GetNextPacket()
{
	return 0;
}
	
//
// Notification iface per-client
//
ECNotifier::ECNotifier()
{
}

ECNotifier::~ECNotifier()
{
	while (m_msg_source.begin() != m_msg_source.end())
		Remove_EC_Client(m_msg_source.begin()->first);
}

CECPacket *ECNotifier::GetNextPacket(ECUpdateMsgSource *msg_source_array[])
{
	CECPacket *packet = 0;
	//
	// priority 0 is highest
	//
	for(int i = 0; i < EC_STATUS_LAST_PRIO; i++) {
		if ( (packet = msg_source_array[i]->GetNextPacket()) != 0 ) {
			break;
		}
	}
	return packet;
}

CECPacket *ECNotifier::GetNextPacket(CECServerSocket *sock)
{
	//
	// OnOutput is called for a first time before
	// socket is registered
	// 
	if ( m_msg_source.count(sock) ) {
		ECUpdateMsgSource **notifier_array = m_msg_source[sock];
		if ( !notifier_array ) {
			return 0;
		}
		CECPacket *packet = GetNextPacket(notifier_array);
		printf("[EC] next update packet; opcode=%x\n",packet ? packet->GetOpCode() : 0xff);
		return packet;
	} else {
		return 0;
	}
}

//
// Interface to notification macros
//
void ECNotifier::DownloadFile_SetDirty(CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		CECServerSocket *sock = i->first;
		if ( sock->HaveNotificationSupport() ) {
			ECUpdateMsgSource **notifier_array = i->second;
			((ECPartFileMsgSource *)notifier_array[EC_PARTFILE])->SetDirty(file);
		}
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_RemoveFile(CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		ECUpdateMsgSource **notifier_array = i->second;
		((ECPartFileMsgSource *)notifier_array[EC_PARTFILE])->SetRemoved(file);
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_RemoveSource(CPartFile *)
{
	// per-partfile source list is not supported (yet), and IMHO quite useless
}

void ECNotifier::DownloadFile_AddFile(CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		ECUpdateMsgSource **notifier_array = i->second;
		((ECPartFileMsgSource *)notifier_array[EC_PARTFILE])->SetNew(file);
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_AddSource(CPartFile *)
{
	// per-partfile source list is not supported (yet), and IMHO quite useless
}

void ECNotifier::SharedFile_AddFile(CKnownFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		ECUpdateMsgSource **notifier_array = i->second;
		((ECKnownFileMsgSource *)notifier_array[EC_KNOWN])->SetNew(file);
	}
	NextPacketToSocket();
}

void ECNotifier::SharedFile_RemoveFile(CKnownFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		ECUpdateMsgSource **notifier_array = i->second;
		((ECKnownFileMsgSource *)notifier_array[EC_KNOWN])->SetRemoved(file);
	}
	NextPacketToSocket();
}

void ECNotifier::SharedFile_RemoveAllFiles()
{
	// need to figure out what to do here
}
	
void ECNotifier::Add_EC_Client(CECServerSocket *sock)
{
	ECUpdateMsgSource **notifier_array = new ECUpdateMsgSource *[EC_STATUS_LAST_PRIO];
	notifier_array[EC_STATUS] = new ECStatusMsgSource();
	notifier_array[EC_SEARCH] = new ECSearchMsgSource();
	notifier_array[EC_PARTFILE] = new ECPartFileMsgSource();
	notifier_array[EC_CLIENT] = new ECClientMsgSource();
	notifier_array[EC_KNOWN] = new ECKnownFileMsgSource();

	m_msg_source[sock] = notifier_array;
}

void ECNotifier::Remove_EC_Client(CECServerSocket *sock)
{
	if (m_msg_source.count(sock)) {
		ECUpdateMsgSource **notifier_array = m_msg_source[sock];

		m_msg_source.erase(sock);
		
		for(int i = 0; i < EC_STATUS_LAST_PRIO; i++) {
			delete notifier_array[i];
		}
		delete [] notifier_array;
	}
}

void ECNotifier::NextPacketToSocket()
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); i++) {
		CECServerSocket *sock = i->first;
		if ( sock->HaveNotificationSupport() && !sock->DataPending() ) {
			ECUpdateMsgSource **notifier_array = i->second;
			CECPacket *packet = GetNextPacket(notifier_array);
			if ( packet ) {
				printf("[EC] sending update packet; opcode=%x\n",packet->GetOpCode());
				sock->SendPacket(packet);
			}
		}
	}
}

// File_checked_for_headers
