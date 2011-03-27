//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Froenchenko Leonid (lfroen@gmail.com)
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
#include "ClientList.h"
#include "Preferences.h"	// Needed for CPreferences
#include "Logger.h"
#include "GuiEvents.h"		// Needed for Notify_* macros
#include "Statistics.h"		// Needed for theStats
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "Friend.h"
#include "FriendList.h"
#include "RandomFunctions.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"


//-------------------- File_Encoder --------------------


/*
 * Encode 'obtained parts' info to be sent to remote gui
 */
class CKnownFile_Encoder {
	// number of sources for each part for progress bar colouring
	RLE_Data m_enc_data;
protected:
	const CKnownFile *m_file;
public:
	CKnownFile_Encoder(const CKnownFile *file = 0) { m_file = file; }

	virtual ~CKnownFile_Encoder() {}

	virtual void Encode(CECTag *parent_tag);

	virtual void ResetEncoder()
	{
		m_enc_data.ResetEncoder();
	}

	virtual void SetShared() { }
	virtual bool IsShared() { return true; }
	virtual bool IsPartFile_Encoder() { return false; }
	const CKnownFile * GetFile() { return m_file; }
};

/*!
 * PartStatus strings and gap lists are quite long - RLE encoding will help.
 * 
 * Instead of sending each time full part-status string, send
 * RLE encoded difference from previous one.
 *
 * PartFileEncoderData class is used for decode only,
 * while CPartFile_Encoder is used for encode only.
 */
class CPartFile_Encoder : public CKnownFile_Encoder {
	// blocks requested for download
	RLE_Data m_req_status;
	// gap list
	RLE_Data m_gap_status;
	// source names
	SourcenameItemMap m_sourcenameItemMap;
	// counter for unique source name ids
	int m_sourcenameID;
	// not all part files are shared (only when at least one part is complete)
	bool m_shared;

	// cast inherited member to CPartFile
	CPartFile * m_PartFile() { wxASSERT(m_file->IsCPartFile()); return (CPartFile *)m_file; }
public:
	// encoder side
	CPartFile_Encoder(const CPartFile *file = 0) : CKnownFile_Encoder(file)
	{ 
		m_sourcenameID = 0;
		m_shared = false;
	}

	virtual ~CPartFile_Encoder() {}
	
	// encode - take data from m_file
	virtual void Encode(CECTag *parent_tag);

	// Encoder may reset history if full info requested
	virtual void ResetEncoder();

	virtual void SetShared() { m_shared = true; }
	virtual bool IsShared() { return m_shared; }
	virtual bool IsPartFile_Encoder() { return true; }
};

class CFileEncoderMap : public std::map<uint32, CKnownFile_Encoder*> {
	typedef std::set<uint32> IDSet;
public:
	~CFileEncoderMap();
	void UpdateEncoders();
};

CFileEncoderMap::~CFileEncoderMap()
{
	// DeleteContents() causes infinite recursion here!
	for (iterator it = begin(); it != end(); it++) {
		delete it->second;
	}
}

// Check if encoder contains files that are no longer used
// or if we have new files without encoder yet.
void CFileEncoderMap::UpdateEncoders()
{
	IDSet curr_files, dead_files;
	// Downloads
	std::vector<CPartFile*> downloads;
	theApp->downloadqueue->CopyFileList(downloads, true);
	for (uint32 i = downloads.size(); i--;) {
		uint32 id = downloads[i]->ECID();
		curr_files.insert(id);
		if (!count(id)) {
			(*this)[id] = new CPartFile_Encoder(downloads[i]);
		}
	}
	// Shares
	std::vector<CKnownFile*> shares;
	theApp->sharedfiles->CopyFileList(shares);
	for (uint32 i = shares.size(); i--;) {
		uint32 id = shares[i]->ECID();
		// Check if it is already there.
		// The curr_files.count(id) is enough, the IsCPartFile() is just a speedup.
		if (shares[i]->IsCPartFile() && curr_files.count(id)) {
			(*this)[id]->SetShared();
			continue;
		}
		curr_files.insert(id);
		if (!count(id)) {
			(*this)[id] = new CKnownFile_Encoder(shares[i]);
		}
	}
	// Check for removed files, and store them in a set for deletion.
	// (std::map documentation is unclear if a construct like
	//		iterator to_del = it++; erase(to_del) ;
	// works or invalidates it too.)
	for (iterator it = begin(); it != end(); it++) {
		if (!curr_files.count(it->first)) {
			dead_files.insert(it->first);
		}
	}
	// then delete them
	for (IDSet::iterator it = dead_files.begin(); it != dead_files.end(); it++) {
		iterator it2 = find(*it);
		delete it2->second;
		erase(it2);
	}
}


//-------------------- CECServerSocket --------------------

class CECServerSocket : public CECMuleSocket
{
public:
	CECServerSocket(ECNotifier *notifier);
	virtual ~CECServerSocket();

	virtual const CECPacket *OnPacketReceived(const CECPacket *packet, uint32 trueSize);
	virtual void OnLost();

	virtual void WriteDoneAndQueueEmpty();

	void	ResetLog() { m_LoggerAccess.Reset(); }
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
	CFileEncoderMap	m_FileEncoder;
	CObjTagMap		m_obj_tagmap;
	CECPacket *ProcessRequest2(const CECPacket *request);

	virtual bool IsAuthorized() { return m_conn_state == CONN_ESTABLISHED; }
};


CECServerSocket::CECServerSocket(ECNotifier *notifier)
:
CECMuleSocket(true),
m_conn_state(CONN_INIT),
m_passwd_salt(GetRandomUint64())
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


const CECPacket *CECServerSocket::OnPacketReceived(const CECPacket *packet, uint32 trueSize)
{
	packet->DebugPrint(true, trueSize);

	const CECPacket *reply = NULL;

	if (m_conn_state == CONN_FAILED) {
		// Client didn't close the socket when authentication failed.
		AddLogLineN(_("Client sent packet after authentication failed."));
		CloseSocket();
	}

	if (m_conn_state != CONN_ESTABLISHED) {
		// This is called twice:
		// 1) send salt
		// 2) verify password
		reply = Authenticate(packet);
	} else {
		reply = ProcessRequest2(packet);
	}
	return reply;
}


void CECServerSocket::OnLost()
{
	AddLogLineN(_("External connection closed."));
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
			AddLogLineC(_("External connections disabled due to empty password!"));
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
			msgLocal = CFormat(wxT("*** TCP socket (ECServer) listening on %s:%d")) % ip % port;
			*msg += msgLocal + wxT("\n");
			AddLogLineN(msgLocal);
		} else {
			msgLocal = CFormat(wxT("Could not listen for external connections at %s:%d!")) % ip % port;
			*msg += msgLocal + wxT("\n");
			AddLogLineN(msgLocal);
		}
	} else {
		*msg += wxT("External connections disabled in config file\n");
		AddLogLineN(_("External connections disabled in config file"));
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
	AddDebugLogLineN(logGeneral,
		CFormat(wxT("ExternalConn::KillAllSockets(): %d sockets to destroy.")) %
			socket_list.size());
	SocketSet::iterator it = socket_list.begin();
	while (it != socket_list.end()) {
		CECServerSocket *s = *(it++);
		s->Close();
		s->Destroy();
	}
}


void ExternalConn::ResetAllLogs()
{
	SocketSet::iterator it = socket_list.begin();
	while (it != socket_list.end()) {
		CECServerSocket *s = *(it++);
		s->ResetLog();
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
		AddLogLineN(_("New external connection accepted"));
	} else {
		delete sock;
		AddLogLineN(_("ERROR: couldn't accept a new external connection"));
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
		AddLogLineC(_("External connection refused due to empty password in preferences!"));	
		
		return new CECPacket(EC_OP_AUTH_FAIL);
	}
		
	if ((m_conn_state == CONN_INIT) && (request->GetOpCode() == EC_OP_AUTH_REQ) ) {
		const CECTag *clientName = request->GetTagByName(EC_TAG_CLIENT_NAME);
		const CECTag *clientVersion = request->GetTagByName(EC_TAG_CLIENT_VERSION);
		
		AddLogLineN(CFormat( _("Connecting client: %s %s") )
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
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("You cannot connect to a release version from an arbitrary development snapshot! *sigh* possible crash prevented")));
#endif
		} else if (protocol != NULL) {
			uint16 proto_version = protocol->GetInt();
			if (proto_version == EC_CURRENT_PROTOCOL_VERSION) {
				response = new CECPacket(EC_OP_AUTH_SALT);
				response->AddTag(CECTag(EC_TAG_PASSWD_SALT, m_passwd_salt));
				m_conn_state = CONN_SALT_SENT;
				//
				// So far ok, check capabilities of client
				//
				bool canZLIB = false, canUTF8numbers = false;
				if (request->GetTagByName(EC_TAG_CAN_ZLIB)) {
					canZLIB = true;
					m_my_flags |= EC_FLAG_ZLIB;
				}
				if (request->GetTagByName(EC_TAG_CAN_UTF8_NUMBERS)) {
					canUTF8numbers = true;
					m_my_flags |= EC_FLAG_UTF8_NUMBERS;
				}
				m_haveNotificationSupport = request->GetTagByName(EC_TAG_CAN_NOTIFY) != NULL;
				AddDebugLogLineN(logEC, CFormat(wxT("Client capabilities: ZLIB: %s  UTF8 numbers: %s  Push notification: %s") )
					% (canZLIB ? wxT("yes") : wxT("no"))
					% (canUTF8numbers ? wxT("yes") : wxT("no"))
					% (m_haveNotificationSupport ? wxT("yes") : wxT("no")));
			} else {
				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid protocol version.")
					+ CFormat(wxT("( %#.4x != %#.4x )")) % proto_version % (uint16_t)EC_CURRENT_PROTOCOL_VERSION));
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
			AddLogLineN(wxString(wxGetTranslation(err)) 
						+ wxT(" ") + thePrefs::ECPassword());
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
				AddLogLineN(wxGetTranslation(err));
			}
		}
	} else {
		response = new CECPacket(EC_OP_AUTH_FAIL);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid request, please authenticate first.")));
	}
	
	if (response->GetOpCode() == EC_OP_AUTH_OK) {
		m_conn_state = CONN_ESTABLISHED;
		AddLogLineN(_("Access granted."));
		// Establish notification handler if client supports it
		if (HaveNotificationSupport()) {
			theApp->ECServerHandler->m_ec_notifier->Add_EC_Client(this);
		}
	} else if (response->GetOpCode() == EC_OP_AUTH_FAIL) {
		// Log message sent to client
		if (response->GetFirstTagSafe()->IsString()) {
			AddLogLineN(CFormat(_("Sent error message \"%s\" to client.")) % wxGetTranslation(response->GetFirstTagSafe()->GetStringData()));
		}
		// Access denied!
		amuleIPV4Address address;
		GetPeer(address);
		AddLogLineN(CFormat(_("Unauthorized access attempt from %s. Connection closed.")) % address.IPAddress() );
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
				response->AddTag(CECTag(EC_TAG_STATS_KAD_IN_LAN_MODE, Kademlia::CKademlia::IsRunningInLANMode()));
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

static CECPacket *Get_EC_Response_GetSharedFiles(const CECPacket *request, CFileEncoderMap &encoders)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_SHARED_FILES);

	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_KNOWNFILE> queryitems(request);

	encoders.UpdateEncoders();
	
	for (uint32 i = 0; i < theApp->sharedfiles->GetFileCount(); ++i) {
		CKnownFile *cur_file = (CKnownFile *)theApp->sharedfiles->GetFileByIndex(i);

		if ( !cur_file || (!queryitems.empty() && !queryitems.count(cur_file->ECID())) ) {
			continue;
		}

		CEC_SharedFile_Tag filetag(cur_file, detail_level);
		CKnownFile_Encoder *enc = encoders[cur_file->ECID()];
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc->ResetEncoder();
		}
		enc->Encode(&filetag);
		response->AddTag(filetag);
	}
	return response;
}

static CECPacket *Get_EC_Response_GetUpdate(CFileEncoderMap &encoders, CObjTagMap &tagmap)
{
	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	encoders.UpdateEncoders();
	for (CFileEncoderMap::iterator it = encoders.begin(); it != encoders.end(); ++it) {
		const CKnownFile *cur_file = it->second->GetFile();
		CValueMap &valuemap = tagmap.GetValueMap(cur_file->ECID());
		// Completed cleared Partfiles are still stored as CPartfile,
		// but encoded as KnownFile, so we have to check the encoder type
		// instead of the file type.
		if (it->second->IsPartFile_Encoder()) {
			CEC_PartFile_Tag filetag((const CPartFile*) cur_file, EC_DETAIL_INC_UPDATE, &valuemap);
			// Add information if partfile is shared
			filetag.AddTag(EC_TAG_PARTFILE_SHARED, it->second->IsShared(), &valuemap);

			CPartFile_Encoder * enc = (CPartFile_Encoder *) encoders[cur_file->ECID()];
			enc->Encode(&filetag);
			response->AddTag(filetag);
		} else {
			CEC_SharedFile_Tag filetag(cur_file, EC_DETAIL_INC_UPDATE, &valuemap);
			CKnownFile_Encoder * enc = encoders[cur_file->ECID()];
			enc->Encode(&filetag);
			response->AddTag(filetag);
		}
	}

	// Add clients
	CECEmptyTag clients(EC_TAG_CLIENT);
	const CClientList::IDMap& clientList = theApp->clientlist->GetClientList();
	bool onlyTransmittingClients = thePrefs::IsTransmitOnlyUploadingClients();
	for (CClientList::IDMap::const_iterator it = clientList.begin(); it != clientList.end(); it++) {
		const CUpDownClient* cur_client = it->second.GetClient();
		if (onlyTransmittingClients && !cur_client->IsDownloading()) {
			// For poor CPU cores only transmit uploading clients. This will save a lot of CPU.
			// Set ExternalConnect/TransmitOnlyUploadingClients to 1 for it.
			continue;
		}
		CValueMap &valuemap = tagmap.GetValueMap(cur_client->ECID());
		clients.AddTag(CEC_UpDownClient_Tag(cur_client, EC_DETAIL_INC_UPDATE, &valuemap));
	}
	response->AddTag(clients);

	// Add servers
	CECEmptyTag servers(EC_TAG_SERVER);
	std::vector<const CServer*> serverlist = theApp->serverlist->CopySnapshot();
	uint32 nrServers = serverlist.size();
	for (uint32 i = 0; i < nrServers; i++) {
		const CServer* cur_server = serverlist[i];
		CValueMap &valuemap = tagmap.GetValueMap(cur_server->ECID());
		servers.AddTag(CEC_Server_Tag(cur_server, &valuemap));
	}
	response->AddTag(servers);

	// Add friends
	CECEmptyTag friends(EC_TAG_FRIEND);
	for (CFriendList::const_iterator it = theApp->friendlist->begin(); it != theApp->friendlist->end(); it++) {
		const CFriend* cur_friend = *it;
		CValueMap &valuemap = tagmap.GetValueMap(cur_friend->ECID());
		friends.AddTag(CEC_Friend_Tag(cur_friend, &valuemap));
	}
	response->AddTag(friends);

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

	const CClientRefList& clients = theApp->uploadqueue->GetUploadingList();
	CClientRefList::const_iterator it = clients.begin();
	for (; it != clients.end(); ++it) {
		CUpDownClient* cur_client = it->GetClient();

		if (!cur_client) {	// shouldn't happen
			continue;
		}
		if (!queryitems.empty() && !queryitems.count(cur_client->ECID())) {
			continue;
		}
		CValueMap *valuemap = NULL;
		if (detail_level == EC_DETAIL_INC_UPDATE) {
			valuemap = &tagmap.GetValueMap(cur_client->ECID());
		}
		CEC_UpDownClient_Tag cli_tag(cur_client, detail_level, valuemap);
		
		response->AddTag(cli_tag);
	}

	return response;
}


static CECPacket *Get_EC_Response_GetDownloadQueue(const CECPacket *request, CFileEncoderMap &encoders)
{	
	CECPacket *response = new CECPacket(EC_OP_DLOAD_QUEUE);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_PARTFILE> queryitems(request);
	
	encoders.UpdateEncoders();

	for (unsigned int i = 0; i < theApp->downloadqueue->GetFileCount(); i++) {
		CPartFile *cur_file = theApp->downloadqueue->GetFileByIndex(i);
	
		if ( !queryitems.empty() && !queryitems.count(cur_file->ECID()) ) {
			continue;
		}

		CEC_PartFile_Tag filetag(cur_file, detail_level);
		
		CPartFile_Encoder * enc = (CPartFile_Encoder *) encoders[cur_file->ECID()];
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc->ResetEncoder();
		}
		enc->Encode(&filetag);

		response->AddTag(filetag);
	}
	return response;
}


static CECPacket *Get_EC_Response_PartFile_Cmd(const CECPacket *request)
{
	CECPacket *response = NULL;

	// request can contain multiple files.
	for (CECPacket::const_iterator it1 = request->begin(); it1 != request->end(); it1++) {
		const CECTag &hashtag = *it1;

		wxASSERT(hashtag.GetTagName() == EC_TAG_PARTFILE);

		CMD4Hash hash = hashtag.GetMD4Data();
		CPartFile *pfile = theApp->downloadqueue->GetFileByID( hash );
		
		if ( !pfile ) {
			AddLogLineN(CFormat(_("Remote PartFile command failed: FileHash not found: %s")) % hash.Encode());
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING, CFormat(wxString(wxTRANSLATE("FileHash not found: %s"))) % hash.Encode()));
			//return response;
			break;
		}
		switch (request->GetOpCode()) {
			case EC_OP_PARTFILE_SWAP_A4AF_THIS:
				CoreNotify_PartFile_Swap_A4AF(pfile);
				break;
			case EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO:
				CoreNotify_PartFile_Swap_A4AF_Auto(pfile);
				break;
			case EC_OP_PARTFILE_SWAP_A4AF_OTHERS:
				CoreNotify_PartFile_Swap_A4AF_Others(pfile);
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
					uint8 prio = hashtag.GetFirstTagSafe()->GetInt();
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
				pfile->SetCategory(hashtag.GetFirstTagSafe()->GetInt());
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

	wxString full_addr = request->GetTagByNameSafe(EC_TAG_SERVER_ADDRESS)->GetStringData();
	wxString name = request->GetTagByNameSafe(EC_TAG_SERVER_NAME)->GetStringData();
	
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
	const CECTag *srv_tag = request->GetTagByName(EC_TAG_SERVER);
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


static CECPacket *Get_EC_Response_Friend(const CECPacket *request)
{
	CECPacket *response = NULL;
	const CECTag *tag = request->GetTagByName(EC_TAG_FRIEND_ADD);
	if (tag) {
		const CECTag *subtag = tag->GetTagByName(EC_TAG_CLIENT);
		if (subtag) {
			CUpDownClient * client = theApp->clientlist->FindClientByECID(subtag->GetInt());
			if (client) {
				theApp->friendlist->AddFriend(CCLIENTREF(client, wxT("Get_EC_Response_Friend theApp->friendlist->AddFriend")));
				response = new CECPacket(EC_OP_NOOP);
			}
		} else {
			const CECTag *hashtag	= tag->GetTagByName(EC_TAG_FRIEND_HASH);
			const CECTag *iptag		= tag->GetTagByName(EC_TAG_FRIEND_IP);
			const CECTag *porttag	= tag->GetTagByName(EC_TAG_FRIEND_PORT);
			const CECTag *nametag	= tag->GetTagByName(EC_TAG_FRIEND_NAME);
			if (hashtag && iptag && porttag && nametag) {
				theApp->friendlist->AddFriend(hashtag->GetMD4Data(), iptag->GetInt(), porttag->GetInt(), nametag->GetStringData());
				response = new CECPacket(EC_OP_NOOP);
			}
		}
	} else if ((tag = request->GetTagByName(EC_TAG_FRIEND_REMOVE))) {
		const CECTag *subtag = tag->GetTagByName(EC_TAG_FRIEND);
		if (subtag) {
			CFriend * Friend = theApp->friendlist->FindFriend(subtag->GetInt());
			if (Friend) {
				theApp->friendlist->RemoveFriend(Friend);
				response = new CECPacket(EC_OP_NOOP);
			}
		}
	} else if ((tag = request->GetTagByName(EC_TAG_FRIEND_FRIENDSLOT))) {
		const CECTag *subtag = tag->GetTagByName(EC_TAG_FRIEND);
		if (subtag) {
			CFriend * Friend = theApp->friendlist->FindFriend(subtag->GetInt());
			if (Friend) {
				theApp->friendlist->SetFriendSlot(Friend, tag->GetInt() != 0);
				response = new CECPacket(EC_OP_NOOP);
			}
		}
	} else if ((tag = request->GetTagByName(EC_TAG_FRIEND_SHARED))) {
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, wxT("Request shared files list not implemented yet.")));
#if 0
		// This works fine - but there is no way atm to transfer the results to amulegui, so disable it for now.

		const CECTag *subtag = tag->GetTagByName(EC_TAG_FRIEND);
		if (subtag) {
			CFriend * Friend = theApp->friendlist->FindFriend(subtag->GetInt());
			if (Friend) {
				theApp->friendlist->RequestSharedFileList(Friend);
				response = new CECPacket(EC_OP_NOOP);
			}
		} else if ((subtag = tag->GetTagByName(EC_TAG_CLIENT))) {
			CUpDownClient * client = theApp->clientlist->FindClientByECID(subtag->GetInt());
			if (client) {
				client->RequestSharedFileList();
				response = new CECPacket(EC_OP_NOOP);
			}
		}
#endif
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
	CTagSet<uint32, EC_TAG_SEARCHFILE> queryitems(request);

	const CSearchResultList& list = theApp->searchlist->GetSearchResults(0xffffffff);
	CSearchResultList::const_iterator it = list.begin();
	while (it != list.end()) {
		CSearchFile* sf = *it++;
		if ( !queryitems.empty() && !queryitems.count(sf->ECID()) ) {
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
		CValueMap &valuemap = tagmap.GetValueMap(sf->ECID());
		response->AddTag(CEC_SearchFile_Tag(sf, EC_DETAIL_INC_UPDATE, &valuemap));
		// Add the children
		if (sf->HasChildren()) {
			const CSearchResultList& children = sf->GetChildren();
			for (size_t i = 0; i < children.size(); ++i) {
				CSearchFile* sfc = children.at(i);
				CValueMap &valuemap1 = tagmap.GetValueMap(sfc->ECID());
				response->AddTag(CEC_SearchFile_Tag(sfc, EC_DETAIL_INC_UPDATE, &valuemap1));
			}
		}
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Results_Download(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_STRINGS);
	for (CECPacket::const_iterator it = request->begin(); it != request->end(); it++) {
		const CECTag &tag = *it;
		CMD4Hash hash = tag.GetMD4Data();
		uint8 category = tag.GetFirstTagSafe()->GetInt();
		theApp->searchlist->AddFileToDownloadByHash(hash, category);
	}
	return response;
}

static CECPacket *Get_EC_Response_Search_Stop(const CECPacket *WXUNUSED(request))
{
	CECPacket *reply = new CECPacket(EC_OP_MISC_DATA);
	theApp->searchlist->StopSearch();
	return reply;
}

static CECPacket *Get_EC_Response_Search(const CECPacket *request)
{
	wxString response;
	
	CEC_Search_Tag *search_request = (CEC_Search_Tag *)request->GetFirstTagSafe();
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
	uint32 op = EC_OP_FAILED;
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
				op = EC_OP_STRINGS;
			}
			break;
		}
		case EC_SEARCH_WEB:
				response = wxTRANSLATE("WebSearch from remote interface makes no sense.");
			break;
	}
	
	CECPacket *reply = new CECPacket(op);
	// error or search in progress
	reply->AddTag(CECTag(EC_TAG_STRING, response));
		
	return reply;
}

static CECPacket *Get_EC_Response_Set_SharedFile_Prio(const CECPacket *request)
{
	CECPacket *response = new CECPacket(EC_OP_NOOP);
	for (CECPacket::const_iterator it = request->begin(); it != request->end(); it++) {
		const CECTag &tag = *it;
		CMD4Hash hash = tag.GetMD4Data();
		uint8 prio = tag.GetFirstTagSafe()->GetInt();
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

void CPartFile_Encoder::Encode(CECTag *parent)
{
	//
	// Source part frequencies
	//
	CKnownFile_Encoder::Encode(parent);

	//
	// Gaps
	//
	const CGapList& gaplist = m_PartFile()->GetGapList();
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
	const CPartFile::CReqBlockPtrList& requestedblocks = m_PartFile()->GetRequestedBlockList();
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
	const CPartFile::SourceSet &sources = m_PartFile()->GetSourceList();
	for (CPartFile::SourceSet::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		const CClientRef &cur_src = *it; 
		if (cur_src.GetRequestFile() != m_file || cur_src.GetClientFilename().Length() == 0) {
			continue;
		}
		const wxString &name = cur_src.GetClientFilename();
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
	CKnownFile_Encoder::ResetEncoder();
	m_gap_status.ResetEncoder();
	m_req_status.ResetEncoder();
}

void CKnownFile_Encoder::Encode(CECTag *parent)
{
	//
	// Source part frequencies
	//
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

CECPacket *CECServerSocket::ProcessRequest2(const CECPacket *request)
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
				AddLogLineC(_("External Connection: shutdown requested"));
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
			for (CECPacket::const_iterator it = request->begin(); it != request->end(); it++) {
				const CECTag &tag = *it;
				wxString link = tag.GetStringData();
				int category = 0;
				const CECTag *cattag = tag.GetTagByName(EC_TAG_PARTFILE_CAT);
				if (cattag) {
					category = cattag->GetInt();
				}
				AddLogLineC(CFormat(_("ExternalConn: adding link '%s'.")) % link);
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
			if ( request->GetDetailLevel() != EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetSharedFiles(request, m_FileEncoder);
			}
			break;
		case EC_OP_GET_DLOAD_QUEUE:
			if ( request->GetDetailLevel() != EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetDownloadQueue(request, m_FileEncoder);
			}
			break;
		//
		// This will evolve into an update-all for inc tags
		//
		case EC_OP_GET_UPDATE:
			if ( request->GetDetailLevel() == EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetUpdate(m_FileEncoder, m_obj_tagmap);
			}
			break;
		case EC_OP_GET_ULOAD_QUEUE:
			response = Get_EC_Response_GetClientQueue(request, m_obj_tagmap, EC_OP_ULOAD_QUEUE);
			break;
		case EC_OP_PARTFILE_REMOVE_NO_NEEDED:
		case EC_OP_PARTFILE_REMOVE_FULL_QUEUE:
		case EC_OP_PARTFILE_REMOVE_HIGH_QUEUE:
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
			wxString newName = request->GetTagByNameSafe(EC_TAG_PARTFILE_NAME)->GetStringData();
			// search first in downloadqueue - it might be in known files as well
			CKnownFile* file = theApp->downloadqueue->GetFileByID(fileHash);
			if (!file) {
				file = theApp->knownfiles->FindKnownFileByID(fileHash);
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
		case EC_OP_CLEAR_COMPLETED: {
			ListOfUInts32 toClear;
			for (CECTag::const_iterator it = request->begin(); it != request->end(); it++) {
				if (it->GetTagName() == EC_TAG_ECID) {
					toClear.push_back(it->GetInt());
				}
			}
			theApp->downloadqueue->ClearCompleted(toClear);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		case EC_OP_CLIENT_SWAP_TO_ANOTHER_FILE: {
			theApp->sharedfiles->Reload();
			uint32 idClient = request->GetTagByNameSafe(EC_TAG_CLIENT)->GetInt();
			CUpDownClient * client = theApp->clientlist->FindClientByECID(idClient);
			CMD4Hash idFile = request->GetTagByNameSafe(EC_TAG_PARTFILE)->GetMD4Data();
			CPartFile * file = theApp->downloadqueue->GetFileByID(idFile);
			if (client && file) {
				client->SwapToAnotherFile( true, false, false, file);
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		case EC_OP_SHARED_FILE_SET_COMMENT: {
			CMD4Hash hash = request->GetTagByNameSafe(EC_TAG_KNOWNFILE)->GetMD4Data();
			CKnownFile * file = theApp->sharedfiles->GetFileByID(hash);
			if (file) {
				wxString newComment = request->GetTagByNameSafe(EC_TAG_KNOWNFILE_COMMENT)->GetStringData();
				uint8 newRating = request->GetTagByNameSafe(EC_TAG_KNOWNFILE_RATING)->GetInt();
				CoreNotify_KnownFile_Comment_Set(file, newComment, newRating);
			}
			response = new CECPacket(EC_OP_NOOP);
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
			wxString url = request->GetFirstTagSafe()->GetStringData();

			// Save the new url, and update the UI (if not amuled).
			Notify_ServersURLChanged(url);
			thePrefs::SetEd2kServersUrl(url);

			theApp->serverlist->UpdateServerMetFromURL(url);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		case EC_OP_SERVER_SET_STATIC_PRIO: {
			uint32 ecid = request->GetTagByNameSafe(EC_TAG_SERVER)->GetInt();
			CServer * server = theApp->serverlist->GetServerByECID(ecid);
			if (server) {
				const CECTag * staticTag = request->GetTagByName(EC_TAG_SERVER_STATIC);
				if (staticTag) {
					theApp->serverlist->SetStaticServer(server, staticTag->GetInt() > 0);
				}
				const CECTag * prioTag = request->GetTagByName(EC_TAG_SERVER_PRIO);
				if (prioTag) {
					theApp->serverlist->SetServerPrio(server, prioTag->GetInt());
				}
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		//
		// Friends
		//
		case EC_OP_FRIEND:
			response = Get_EC_Response_Friend(request);
			break;

		//
		// IPFilter
		//
		case EC_OP_IPFILTER_RELOAD:
			NotifyAlways_IPFilter_Reload();
			response = new CECPacket(EC_OP_NOOP);
			break;

		case EC_OP_IPFILTER_UPDATE: {
			wxString url = request->GetFirstTagSafe()->GetStringData();
			if (url.IsEmpty()) {
				url = thePrefs::IPFilterURL();
			}
			NotifyAlways_IPFilter_Update(url);
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
				response = Get_EC_Response_Search_Results(m_obj_tagmap);
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
				CEC_Category_Tag *tag = (CEC_Category_Tag *)request->GetFirstTagSafe();
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
				CEC_Category_Tag *tag = (CEC_Category_Tag *)request->GetFirstTagSafe();
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
				uint32 cat = request->GetFirstTagSafe()->GetInt();
				// this noes not only update the gui, but actually deletes the cat
				Notify_CategoryDelete(cat);
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		
		//
		// Logging
		//
		case EC_OP_ADDLOGLINE:
			if (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL) {
				AddLogLineC(request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			} else {
				AddLogLineN(request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_ADDDEBUGLOGLINE:
			if (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL) {
				AddDebugLogLineC(logGeneral, request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			} else {
				AddDebugLogLineN(logGeneral, request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			}
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
			if (thePrefs::GetNetworkKademlia()) {
				response = new CECPacket(EC_OP_NOOP);
				if ( !Kademlia::CKademlia::IsRunning() ) {
					Kademlia::CKademlia::Start();
					theApp->ShowConnectionState();
				}
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Kad is disabled in preferences.")));
			}
			break;
		case EC_OP_KAD_STOP:
			theApp->StopKad();
			theApp->ShowConnectionState();
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_KAD_UPDATE_FROM_URL: {
			wxString url = request->GetFirstTagSafe()->GetStringData();

			// Save the new url, and update the UI (if not amuled).
			Notify_NodesURLChanged(url);
			thePrefs::SetKadNodesUrl(url);

			theApp->UpdateNotesDat(url);
			response = new CECPacket(EC_OP_NOOP);
			break;
		}
		case EC_OP_KAD_BOOTSTRAP_FROM_IP:
			if (thePrefs::GetNetworkKademlia()) {
				theApp->BootstrapKad(request->GetTagByNameSafe(EC_TAG_BOOTSTRAP_IP)->GetInt(),
									 request->GetTagByNameSafe(EC_TAG_BOOTSTRAP_PORT)->GetInt());
				theApp->ShowConnectionState();
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Kad is disabled in preferences.")));
			}
			break;

		//
		// Networks
		// These requests are currently used only in the text client
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
			if (response) {
				theApp->ShowConnectionState();
			} else {
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
				theApp->ShowConnectionState();
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
	}
	if (!response) {
		AddLogLineN(CFormat(_("External Connection: invalid opcode received: %#x")) % request->GetOpCode());
		wxFAIL;
		response = new CECPacket(EC_OP_FAILED);
		response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid opcode (wrong protocol version?)")));
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
