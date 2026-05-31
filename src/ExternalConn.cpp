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

#include "config.h"				// Needed for VERSION

#include <set>					// Needed for std::set (m_lastSentFileIds)

#include <ec/cpp/ECMuleSocket.h>		// Needed for CECSocket

#include <common/Format.h>			// Needed for CFormat

#include <common/ClientVersion.h>
#include <common/MD5Sum.h>

#include "ExternalConn.h"			// Interface declarations
#include "ECFullResponseCache.h"		// Needed for s_ec*FullCache
#include "updownclient.h"			// Needed for CUpDownClient
#include "Server.h"				// Needed for CServer
#include "ServerList.h"				// Needed for CServerList
#include "PartFile.h"				// Needed for CPartFile
#include "ServerConnect.h"			// Needed for CServerConnect
#include "UploadQueue.h"			// Needed for CUploadQueue
#include "amule.h"				// Needed for theApp
#include "SearchList.h"				// Needed for GetSearchResults
#include "ClientList.h"
#include "Preferences.h"			// Needed for CPreferences
#include "Logger.h"
#include "GuiEvents.h"				// Needed for Notify_* macros
#include "Statistics.h"				// Needed for theStats
#include "KnownFileList.h"			// Needed for CKnownFileList
#include "Friend.h"
#include "FriendList.h"
#include "RandomFunctions.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "Statistics.h"


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
	const CPartFile * m_PartFile() { wxCHECK(m_file->IsCPartFile(), NULL); return static_cast<const CPartFile *>(m_file); }
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
	for (iterator it = begin(); it != end(); ++it) {
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
	for (iterator it = begin(); it != end(); ++it) {
		if (!curr_files.count(it->first)) {
			dead_files.insert(it->first);
		}
	}
	// then delete them
	for (IDSet::iterator it = dead_files.begin(); it != dead_files.end(); ++it) {
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

	// Bound on the WriteDoneAndQueueEmpty -> SendPacket -> OnOutput ->
	// WriteDoneAndQueueEmpty recursion chain that drains the notifier
	// queue. See WriteDoneAndQueueEmpty for the full reasoning.
	int		m_notification_dispatch_depth;

	// EC INC_UPDATE skip-unchanged state. `m_lastEcGenSeen*` values are
	// the highest `CKnownFile::s_globalEcGen` already reflected in the
	// client's view *for that particular request path* — files with a
	// smaller `m_ecGen` did not change since the last response of that
	// path and can be skipped this cycle. Three independent counters
	// because the request paths interleave on different schedules:
	//   * `m_lastEcGenSeen`        — `EC_OP_GET_UPDATE`        (amulegui)
	//   * `m_lastEcGenSeenShared`  — `EC_OP_GET_SHARED_FILES`  (amuleweb)
	//   * `m_lastEcGenSeenPart`    — `EC_OP_GET_DLOAD_QUEUE`   (amuleweb)
	uint64		m_lastEcGenSeen;
	uint64		m_lastEcGenSeenShared;
	uint64		m_lastEcGenSeenPart;

	// Client opted in to partial-update protocol at auth time (advertised
	// `EC_TAG_CAN_PARTIAL_UPDATE`). When set, `Get_EC_Response_GetUpdate`
	// skips unchanged files entirely and emits explicit `EC_TAG_FILE_REMOVED`
	// markers for files that disappeared since the previous cycle; the
	// client mirrors this by skipping its bulk deletion loop. When *not*
	// set, the server falls back to emitting empty "alive marker" tags for
	// unchanged files so old clients (which infer deletion from absence)
	// keep working unchanged — see `Get_EC_Response_GetUpdate`.
	bool		m_partialUpdateActive;
	// Set of file ECIDs sent in the previous response for each EC
	// request path. Diffed against the current snapshot to compute the
	// removal list emitted to partial-update-capable clients. Tracked
	// per-path because amulegui uses `EC_OP_GET_UPDATE` (mixed shared +
	// partfile, served by `Get_EC_Response_GetUpdate`) while amuleweb
	// drives two separate INC_UPDATE streams via `EC_OP_GET_SHARED_FILES`
	// and `EC_OP_GET_DLOAD_QUEUE` (each served by its own handler).
	std::set<uint32> m_lastSentFileIds;
	std::set<uint32> m_lastSentSharedFileIds;
	std::set<uint32> m_lastSentPartFileIds;
};


CECServerSocket::CECServerSocket(ECNotifier *notifier)
:
CECMuleSocket(true),
m_conn_state(CONN_INIT),
m_passwd_salt(GetRandomUint64()),
m_notification_dispatch_depth(0),
m_lastEcGenSeen(0),
m_lastEcGenSeenShared(0),
m_lastEcGenSeenPart(0),
m_partialUpdateActive(false)
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
	if (!HaveNotificationSupport() || m_conn_state != CONN_ESTABLISHED) {
		//printf("[EC] %p: WriteDoneAndQueueEmpty but notification disabled\n", this);
		return;
	}

	// CECSocket::OnOutput drains the per-socket output queue, then calls
	// WriteDoneAndQueueEmpty to pull the next notification packet. The
	// chain runs synchronously on the main thread:
	//
	//   WriteDoneAndQueueEmpty -> SendPacket -> WritePacket + OnOutput
	//     -> OnOutput drains queue -> WriteDoneAndQueueEmpty -> ...
	//
	// On a busy amuled (many peers / files generating notifications) the
	// ECNotifier always has the next packet ready, so the recursion never
	// bottoms out. The main thread stays inside this chain processing the
	// notifier feed and never yields back to the wx event loop. That
	// starves every other event -- including LibSocketLost from a
	// half-closed EC peer, which is what CECServerSocket::OnLost needs
	// to fire so it can tear the dead socket down.
	//
	// In the wedged-amuleweb scenario reported in #666, the peer is in
	// kernel CLOSE-WAIT, writes silently succeed against the dead
	// kernel buffer, and amuled spins indefinitely flushing the
	// notifier to nowhere -- amulegui can't connect because the main
	// thread is permanently occupied.
	//
	// Cap the dispatch depth so the chain returns to the event loop
	// every MAX_DEPTH packets. The pending asio LibSocketSend
	// completions (or LibSocketLost, if the peer has gone away) get
	// processed in between; on a healthy peer the loop simply resumes
	// when OnSend re-enters via the next completion.
	static const int MAX_DEPTH = 8;
	if (m_notification_dispatch_depth >= MAX_DEPTH) {
		return;
	}

	// ECNotifier::GetNextPacket returns a fresh new CECPacket(...) and
	// the caller owns it; SendPacket(const CECPacket*) only serialises
	// it into the per-socket output queue and never deletes.  Hand the
	// raw pointer to a smart pointer so the CECPacket (and its CECTag
	// tree) get freed at scope exit instead of leaking on every
	// notification dispatch.  Pre-fix, the EC notification path was
	// the dominant retained-bytes leak on long-running amuled with
	// connected amulegui / amuleweb peers (#765).
	CSmartPtr<CECPacket> packet(m_ec_notifier->GetNextPacket(this));
	if (!packet) {
		return;
	}

	m_notification_dispatch_depth++;
	try {
		SendPacket(packet.get());
	} catch (...) {
		m_notification_dispatch_depth--;
		throw;
	}
	m_notification_dispatch_depth--;
}

//-------------------- ExternalConn --------------------

ExternalConn::ExternalConn(amuleIPV4Address addr, wxString *msg)
{
	wxString msgLocal;
	m_ECServer = NULL;
	// Are we allowed to accept External Connections?
	if ( thePrefs::AcceptExternalConnections() ) {
		// We must have a valid password, otherwise we will not allow EC connections
		if (thePrefs::ECPassword().IsEmpty()) {
			*msg += "External connections disabled due to empty password!\n";
			AddLogLineC(_("External connections disabled due to empty password!"));
			return;
		}

		// Create the socket
		m_ECServer = new CExternalConnListener(addr, MULE_SOCKET_REUSEADDR, this);
		m_ECServer->Notify(true);

		int port = addr.Service();
		wxString ip = addr.IPAddress();
		if (m_ECServer->IsOk()) {
			msgLocal = CFormat("*** TCP socket (ECServer) listening on %s:%d") % ip % port;
			*msg += msgLocal + "\n";
			AddLogLineN(msgLocal);
		} else {
			msgLocal = CFormat("Could not listen for external connections at %s:%d!") % ip % port;
			*msg += msgLocal + "\n";
			AddLogLineN(msgLocal);
		}
	} else {
		*msg += "External connections disabled in config file\n";
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
		CFormat("ExternalConn::KillAllSockets(): %d sockets to destroy.") %
			socket_list.size());
	SocketSet::iterator it = socket_list.begin();
	while (it != socket_list.end()) {
		CECServerSocket *s = *(it++);
		s->Close();
		s->Destroy();
	}
	socket_list.clear();
}


void ExternalConn::ResetAllLogs()
{
	SocketSet::iterator it = socket_list.begin();
	while (it != socket_list.end()) {
		CECServerSocket *s = *(it++);
		s->ResetLog();
	}
}


void CExternalConnListener::OnAccept()
{
	CECServerSocket *sock = new CECServerSocket(m_conn->m_ec_notifier);
	// Accept new connection if there is one in the pending
	// connections queue, else exit. We use Accept(FALSE) for
	// non-blocking accept (although if we got here, there
	// should ALWAYS be a pending connection).
	if (AcceptWith(*sock, false)) {
		// Apply EC keepalive on the freshly-accepted server-side
		// socket so amuled detects a half-open EC client (gui
		// process killed, network blip, FIN lost) symmetrically
		// with what the client just enabled on its end. Without
		// this, the kernel sits on the dead connection for the
		// default ~2h TCP retransmit timeout, holding the
		// CECServerSocket and its m_ec_notifier reference.
		sock->ApplyEcKeepalive();
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
		// cppcheck-suppress unreadVariable
		const CECTag *clientName = request->GetTagByName(EC_TAG_CLIENT_NAME);
		// cppcheck-suppress unreadVariable
		const CECTag *clientVersion = request->GetTagByName(EC_TAG_CLIENT_VERSION);

		AddLogLineN(CFormat( _("Connecting client: %s %s") )
			% ( clientName ? clientName->GetStringData() : wxString(_("Unknown")) )
			% ( clientVersion ? clientVersion->GetStringData() : wxString(_("Unknown version")) ) );
		const CECTag *protocol = request->GetTagByName(EC_TAG_PROTOCOL_VERSION);
#ifdef EC_VERSION_ID
		// For snapshot builds, both client and server must use GITDATE, and they must be the same
		CMD4Hash vhash;
		if (!vhash.Decode(EC_VERSION_ID)) {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, "Fatal error, version hash is not a valid MD4-hash."));
		} else if (!request->GetTagByName(EC_TAG_VERSION_ID) || request->GetTagByNameSafe(EC_TAG_VERSION_ID)->GetMD4Data() != vhash) {
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Incorrect EC version ID, there might be binary incompatibility. Use core and remote from same snapshot.")));
#else
		// For release versions, we don't want to allow connections from any arbitrary snapshot client.
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
				if (request->GetTagByName(EC_TAG_CAN_ZLIB)) {
					m_my_flags |= EC_FLAG_ZLIB;
				}
				// Mark the connection as local (loopback / RFC1918 LAN /
				// RFC3927 link-local) so CECSocket::WritePacket can skip
				// ZLIB per-packet for small/medium responses. deflate +
				// inflate is pure overhead on the local machine and on a
				// gigabit LAN (~125 MB/s line rate, well past the
				// ~37 MB/s break-even point for streaming zlib). The
				// per-packet gate still falls back to ZLIB for packets
				// above kLocalPeerZlibBypassMax so very large responses
				// (e.g. show shared on a 90 k+ shared-file library)
				// stay inside the receiver's 256 MB ReadHeader gate.
				{
					const uint32 peer_ip = GetPeerInt();
					const bool is_local = peer_ip == 0
						|| IsLoopbackIP(peer_ip)
						|| IsLanIP(peer_ip)
						|| IsLinkLocalIP(peer_ip);
					SetLocalPeer(is_local);
					if (is_local) {
						AddDebugLogLineN(logEC,
							CFormat("EC peer %s is local (loopback/LAN/link-local) — bypassing ZLIB for small/medium packets")
								% GetPeer());
					}
				}
				if (request->GetTagByName(EC_TAG_CAN_UTF8_NUMBERS)) {
					m_my_flags |= EC_FLAG_UTF8_NUMBERS;
				}
				if (request->GetTagByName(EC_TAG_CAN_LARGE_TAG_COUNT)) {
					// Client can decode the sentinel-extended children-
					// count format in CECTag::WriteChildren (#199). Only
					// new clients send this tag; old clients omit it
					// and we keep the wire format capped at uint16.
					m_my_flags |= EC_FLAG_LARGE_TAG_COUNT;
				}
				if (request->GetTagByName(EC_TAG_CAN_PARTIAL_UPDATE)) {
					// Client understands the partial-update protocol:
					// `Get_EC_Response_GetUpdate` may omit unchanged
					// files and emit explicit `EC_TAG_FILE_REMOVED`
					// markers instead of relying on absence-implies-
					// deletion. Old clients omit this tag and we keep
					// the backward-compat alive-marker path active for
					// them — see `Get_EC_Response_GetUpdate`.
					m_partialUpdateActive = true;
				}
				m_haveNotificationSupport = request->GetTagByName(EC_TAG_CAN_NOTIFY) != NULL;
				AddDebugLogLineN(logEC, CFormat("Client capabilities: ZLIB: %s  UTF8 numbers: %s  Push notification: %s  Large tag count: %s  Partial update: %s" )
					% ((m_my_flags & EC_FLAG_ZLIB) ? "yes" : "no")
					% ((m_my_flags & EC_FLAG_UTF8_NUMBERS) ? "yes" : "no")
					% (m_haveNotificationSupport ? "yes" : "no")
					% ((m_my_flags & EC_FLAG_LARGE_TAG_COUNT) ? "yes" : "no")
					% (m_partialUpdateActive ? "yes" : "no"));
			} else {
				response = new CECPacket(EC_OP_AUTH_FAIL);
				response->AddTag(CECTag(EC_TAG_STRING, wxString(wxTRANSLATE("Invalid protocol version."))
					+ wxString(CFormat("( %#.4x != %#.4x )") % proto_version % (uint16_t)EC_CURRENT_PROTOCOL_VERSION)));
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
						+ " " + thePrefs::ECPassword());
			response = new CECPacket(EC_OP_AUTH_FAIL);
			response->AddTag(CECTag(EC_TAG_STRING, err));
		} else {
			wxString saltHash = MD5Sum(CFormat("%lX") % m_passwd_salt).GetHash();
			wxString saltStr = CFormat("%lX") % m_passwd_salt;

			passh.Decode(MD5Sum(thePrefs::ECPassword().Lower() + saltHash).GetHash());

			if (passwd && passwd->GetMD4Data() == passh) {
				response = new CECPacket(EC_OP_AUTH_OK);
				response->AddTag(CECTag(EC_TAG_SERVER_VERSION, VERSION));
				// Echo the negotiated large-tag-count capability so
				// the client mirrors EC_FLAG_LARGE_TAG_COUNT into its
				// own m_my_flags. Without this echo, client wouldn't
				// know the server supports the extended wire format
				// and would never set the flag in its outgoing
				// per-packet headers (#199).
				if (m_my_flags & EC_FLAG_LARGE_TAG_COUNT) {
					response->AddTag(CECEmptyTag(EC_TAG_CAN_LARGE_TAG_COUNT));
				}
				if (m_partialUpdateActive) {
					// Confirm partial-update mode so the client switches
					// off its bulk "missing == deleted" fallback and
					// expects explicit `EC_TAG_FILE_REMOVED` markers.
					response->AddTag(CECEmptyTag(EC_TAG_CAN_PARTIAL_UPDATE));
				}
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
		AddLogLineN(CFormat(_("Unauthorized access attempt from %s. Connection closed.")) % GetPeer() );
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
		// This is not an actual INC_UPDATE.
		// amulegui only sets the detail level of the stats package to EC_DETAIL_INC_UPDATE
		// so that the included conn state tag is created the way it is needed here.
		case EC_DETAIL_INC_UPDATE:
			response->AddTag(CECTag(EC_TAG_STATS_UP_OVERHEAD, (uint32)theStats::GetUpOverheadRate()));
			response->AddTag(CECTag(EC_TAG_STATS_DOWN_OVERHEAD, (uint32)theStats::GetDownOverheadRate()));
			response->AddTag(CECTag(EC_TAG_STATS_BANNED_COUNT, /*(uint32)*/theStats::GetBannedCount()));
			AddLoggerTag(response, LoggerAccess);
			// Needed only for the remote tray icon context menu
			response->AddTag(CECTag(EC_TAG_STATS_TOTAL_SENT_BYTES, theStats::GetTotalSentBytes()));
			response->AddTag(CECTag(EC_TAG_STATS_TOTAL_RECEIVED_BYTES, theStats::GetTotalReceivedBytes()));
			response->AddTag(CECTag(EC_TAG_STATS_SHARED_FILE_COUNT, theStats::GetSharedFileCount()));
		/* fall through */
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
				response->AddTag(CECTag(EC_TAG_STATS_KAD_NODES, CStatistics::GetKadNodes()));
			}
			// Kad stats
			if (Kademlia::CKademlia::IsConnected()) {
				response->AddTag(CECTag(EC_TAG_STATS_KAD_FIREWALLED_UDP, Kademlia::CUDPFirewallTester::IsFirewalledUDP(true)));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_SOURCES, Kademlia::CKademlia::GetIndexed()->m_totalIndexSource));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_KEYWORDS, Kademlia::CKademlia::GetIndexed()->m_totalIndexKeyword));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_NOTES, Kademlia::CKademlia::GetIndexed()->m_totalIndexNotes));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_INDEXED_LOAD, Kademlia::CKademlia::GetIndexed()->m_totalIndexLoad));
				response->AddTag(CECTag(EC_TAG_STATS_KAD_IP_ADDRESS, wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetPrefs()->GetIPAddress())));
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
			break;
	};

	return response;
}

static CECPacket *Get_EC_Response_GetSharedFiles(const CECPacket *request, CFileEncoderMap &encoders,
	uint64 &io_lastEcGenSeen,
	bool partial_update_active, std::set<uint32> &io_lastSentFileIds)
{
	wxASSERT(request->GetOpCode() == EC_OP_GET_SHARED_FILES);

	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_KNOWNFILE> queryitems(request);

	encoders.UpdateEncoders();

	// Skip-unchanged + EC_TAG_FILE_REMOVED is wired only for the
	// `EC_DETAIL_UPDATE` polling path that amuleweb uses (`EC_OP_GET_
	// SHARED_FILES` re-issued each cycle with an encoder-retained diff
	// state) and only when the client opted into the partial-update
	// protocol at auth. `EC_DETAIL_FULL` callers (amulecmd `show shared`,
	// any one-shot query) still get every alive file as a full tag.
	const bool skip_unchanged_path =
		partial_update_active && detail_level == EC_DETAIL_UPDATE;
	const uint64 ec_snapshot = skip_unchanged_path
		? CKnownFile::GetGlobalECGen() : 0;
	const uint64 ec_threshold = io_lastEcGenSeen;

	// Snapshot the shared-file list once. GetFileByIndex() does an O(N)
	// std::advance over the underlying std::map and re-acquires list_mut
	// on every call -- looping it N times is O(N^2) and pegs the main
	// thread for tens of minutes on users with tens of thousands of
	// shared files (issue #666).
	std::vector<CKnownFile*> snapshot;
	theApp->sharedfiles->CopyFileList(snapshot);

	// Snapshot the alive set for the partial-update removal diff below.
	std::set<uint32> current_file_ids;

	for (std::vector<CKnownFile*>::const_iterator it = snapshot.begin();
		it != snapshot.end(); ++it) {
		const CKnownFile *cur_file = *it;

		if ( !cur_file || (!queryitems.empty() && !queryitems.count(cur_file->ECID())) ) {
			continue;
		}
		const uint32 ecid = cur_file->ECID();
		if (skip_unchanged_path) {
			current_file_ids.insert(ecid);
			if (cur_file->GetECGen() <= ec_threshold) {
				// Client already has the latest exported view of
				// this file; absence here is "no change", not
				// "deleted" — see `EC_TAG_FILE_REMOVED` emission
				// below.
				continue;
			}
		}

		CEC_SharedFile_Tag filetag(cur_file, detail_level);
		CKnownFile_Encoder *enc = encoders[ecid];
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc->ResetEncoder();
		}
		enc->Encode(&filetag);
		response->AddTag(filetag);
	}

	if (skip_unchanged_path) {
		// One EC_TAG_FILE_REMOVED per file that was in the previous
		// response but is no longer alive on the server.
		for (std::set<uint32>::const_iterator it = io_lastSentFileIds.begin();
			it != io_lastSentFileIds.end(); ++it) {
			if (!current_file_ids.count(*it)) {
				response->AddTag(CECTag(EC_TAG_FILE_REMOVED, *it));
			}
		}
		io_lastSentFileIds.swap(current_file_ids);
		io_lastEcGenSeen = ec_snapshot;
	}
	return response;
}

static CECPacket *Get_EC_Response_GetUpdate(CFileEncoderMap &encoders, CObjTagMap &tagmap,
	uint64 &io_lastEcGenSeen,
	bool partial_update_active, std::set<uint32> &io_lastSentFileIds)
{
	CECPacket *response = new CECPacket(EC_OP_SHARED_FILES);

	// Snapshot the global EC generation now. Any file whose `m_ecGen`
	// exceeds the caller's `m_lastEcGenSeen` has been touched by a
	// `MarkECChanged()` hook since the last response for this client and
	// is sent through the encoder; anything older is unchanged from the
	// client's point of view and skipped. Reading the snapshot before the
	// iteration means files that change mid-loop are picked up on the
	// next request (their `m_ecGen` will exceed our snapshot).
	const uint64 ec_snapshot = CKnownFile::GetGlobalECGen();
	const uint64 ec_threshold = io_lastEcGenSeen;

	encoders.UpdateEncoders();

	// Snapshot the IDs of all files currently alive on the server. Used
	// by the partial-update path below to diff against the previous cycle
	// and synthesize `EC_TAG_FILE_REMOVED` markers.
	std::set<uint32> current_file_ids;

	for (CFileEncoderMap::iterator it = encoders.begin(); it != encoders.end(); ++it) {
		const CKnownFile *cur_file = it->second->GetFile();
		const uint32 ecid = cur_file->ECID();
		current_file_ids.insert(ecid);

		if (cur_file->GetECGen() <= ec_threshold) {
			// Nothing exported has changed since the client's last
			// view of this file. Two paths depending on whether the
			// client negotiated partial-update at auth time:
			if (partial_update_active) {
				// New protocol: skip the file entirely. The client
				// only deletes when it sees an explicit
				// `EC_TAG_FILE_REMOVED` (emitted below), so absence
				// here is correctly interpreted as "no change".
				continue;
			}
			// Legacy clients (amulegui / amuleweb on master) treat
			// any file missing from the response as deleted, then
			// re-add it on the next full-sweep cycle — wedging the
			// GUI on big libraries (#713). Emit a 5-byte alive
			// marker (`EC_TAG_KNOWNFILE` / `EC_TAG_PARTFILE` with
			// the ECID and no children); the client's
			// `if (tag->HasChildTags()) ProcessItemUpdate(...)`
			// already treats childless tags as a no-op update but
			// still records the file as present.
			const ec_tagname_t tagname = it->second->IsPartFile_Encoder()
				? EC_TAG_PARTFILE : EC_TAG_KNOWNFILE;
			response->AddTag(CECTag(tagname, ecid));
			continue;
		}
		CValueMap &valuemap = tagmap.GetValueMap(ecid);
		// Completed cleared Partfiles are still stored as CPartfile,
		// but encoded as KnownFile, so we have to check the encoder type
		// instead of the file type.
		if (it->second->IsPartFile_Encoder()) {
			CEC_PartFile_Tag filetag(static_cast<const CPartFile*>(cur_file), EC_DETAIL_INC_UPDATE, &valuemap);
			// Add information if partfile is shared
			filetag.AddTag(EC_TAG_PARTFILE_SHARED, it->second->IsShared(), &valuemap);

			CPartFile_Encoder * enc = static_cast<CPartFile_Encoder *>(encoders[ecid]);
			enc->Encode(&filetag);
			response->AddTag(filetag);
		} else {
			CEC_SharedFile_Tag filetag(cur_file, EC_DETAIL_INC_UPDATE, &valuemap);
			CKnownFile_Encoder * enc = encoders[ecid];
			enc->Encode(&filetag);
			response->AddTag(filetag);
		}
	}

	if (partial_update_active) {
		// Partial-update protocol: emit one `EC_TAG_FILE_REMOVED` per
		// file that was in the previous response but is no longer
		// alive on the server. Replaces the legacy client's bulk
		// "anything missing == deleted" inference.
		for (std::set<uint32>::const_iterator it = io_lastSentFileIds.begin();
			it != io_lastSentFileIds.end(); ++it) {
			if (!current_file_ids.count(*it)) {
				response->AddTag(CECTag(EC_TAG_FILE_REMOVED, *it));
			}
		}
		io_lastSentFileIds.swap(current_file_ids);
	}

	io_lastEcGenSeen = ec_snapshot;

	// Add clients
	CECEmptyTag clients(EC_TAG_CLIENT);
	const CClientList::IDMap& clientList = theApp->clientlist->GetClientList();
	bool onlyTransmittingClients = thePrefs::IsTransmitOnlyUploadingClients();
	for (CClientList::IDMap::const_iterator it = clientList.begin(); it != clientList.end(); ++it) {
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
	for (CFriendList::const_iterator it = theApp->friendlist->begin(); it != theApp->friendlist->end(); ++it) {
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


static CECPacket *Get_EC_Response_GetDownloadQueue(const CECPacket *request, CFileEncoderMap &encoders,
	uint64 &io_lastEcGenSeen,
	bool partial_update_active, std::set<uint32> &io_lastSentFileIds)
{
	CECPacket *response = new CECPacket(EC_OP_DLOAD_QUEUE);

	EC_DETAIL_LEVEL detail_level = request->GetDetailLevel();
	//
	// request can contain list of queried items
	CTagSet<uint32, EC_TAG_PARTFILE> queryitems(request);

	encoders.UpdateEncoders();

	// Skip-unchanged + EC_TAG_FILE_REMOVED is wired only for the
	// `EC_DETAIL_UPDATE` polling path that amuleweb uses, and only when
	// the client opted into the partial-update protocol at auth. Other
	// callers still get every alive file as a full tag.
	const bool skip_unchanged_path =
		partial_update_active && detail_level == EC_DETAIL_UPDATE;
	const uint64 ec_snapshot = skip_unchanged_path
		? CKnownFile::GetGlobalECGen() : 0;
	const uint64 ec_threshold = io_lastEcGenSeen;

	// Snapshot once to avoid re-locking downloadqueue's mutex on every
	// iteration (see Get_EC_Response_GetSharedFiles for the matching
	// shared-files fix in issue #666).
	std::vector<CPartFile*> snapshot;
	theApp->downloadqueue->CopyFileList(snapshot);

	std::set<uint32> current_file_ids;

	for (std::vector<CPartFile*>::const_iterator it = snapshot.begin();
		it != snapshot.end(); ++it) {
		CPartFile *cur_file = *it;

		if ( !queryitems.empty() && !queryitems.count(cur_file->ECID()) ) {
			continue;
		}
		const uint32 ecid = cur_file->ECID();
		if (skip_unchanged_path) {
			current_file_ids.insert(ecid);
			if (cur_file->GetECGen() <= ec_threshold) {
				// Client already has the latest exported view of
				// this partfile; absence here is "no change",
				// not "deleted" — see `EC_TAG_FILE_REMOVED`
				// emission below.
				continue;
			}
		}

		CEC_PartFile_Tag filetag(cur_file, detail_level);

		CPartFile_Encoder * enc = static_cast<CPartFile_Encoder *>(encoders[ecid]);
		if ( detail_level != EC_DETAIL_UPDATE ) {
			enc->ResetEncoder();
		}
		enc->Encode(&filetag);

		response->AddTag(filetag);
	}

	if (skip_unchanged_path) {
		for (std::set<uint32>::const_iterator it = io_lastSentFileIds.begin();
			it != io_lastSentFileIds.end(); ++it) {
			if (!current_file_ids.count(*it)) {
				response->AddTag(CECTag(EC_TAG_FILE_REMOVED, *it));
			}
		}
		io_lastSentFileIds.swap(current_file_ids);
		io_lastEcGenSeen = ec_snapshot;
	}
	return response;
}


// Build a CEC_SharedFile_Tag for the per-file cache. The output is
// self-contained — the encoder is reset before each Encode call in
// FULL mode (see Get_EC_Response_GetSharedFiles), so a local encoder
// suffices. Caller owns the returned tag.
static CECTag *BuildSharedFileCacheTag(const void *file_v)
{
	const CKnownFile *cur_file = static_cast<const CKnownFile *>(file_v);
	CEC_SharedFile_Tag *filetag = new CEC_SharedFile_Tag(cur_file, EC_DETAIL_FULL);
	CKnownFile_Encoder enc(cur_file);
	enc.ResetEncoder();
	enc.Encode(filetag);
	return filetag;
}


// Same shape for partfiles.
static CECTag *BuildPartFileCacheTag(const void *file_v)
{
	const CPartFile *cur_file = static_cast<const CPartFile *>(file_v);
	CEC_PartFile_Tag *filetag = new CEC_PartFile_Tag(cur_file, EC_DETAIL_FULL);
	CPartFile_Encoder enc(cur_file);
	enc.ResetEncoder();
	enc.Encode(filetag);
	return filetag;
}


// Two daemon-wide caches. Each holds one pre-serialized blob per file
// in its domain, freshness-stamped with the file's m_ecGen at build
// time. A request rebuilds only entries whose file gen has advanced
// past the cached gen — the same per-file freshness primitive the
// INC_UPDATE path uses.
static CECFullResponseCache s_sharedFilesFullCache(BuildSharedFileCacheTag);
static CECFullResponseCache s_downloadQueueFullCache(BuildPartFileCacheTag);


static CECPacket *Get_EC_Response_PartFile_Cmd(const CECPacket *request)
{
	CECPacket *response = NULL;

	// request can contain multiple files.
	for (CECPacket::const_iterator it1 = request->begin(); it1 != request->end(); ++it1) {
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
				theApp->friendlist->AddFriend(CCLIENTREF(client, "Get_EC_Response_Friend theApp->friendlist->AddFriend"));
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
			}
			// Idempotent: the desired end state of REMOVE is "friend
			// not in the list", which is already true if FindFriend
			// returned null (transient sync skew between amulegui's
			// local view and the daemon's m_FriendList). Returning
			// EC_OP_FAILED here forces the GUI into a resend / hang
			// loop on the stale ECID.
			response = new CECPacket(EC_OP_NOOP);
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
		const CECTag *subtag = tag->GetTagByName(EC_TAG_FRIEND);
		if (subtag) {
			CFriend * Friend = theApp->friendlist->FindFriend(subtag->GetInt());
			if (Friend) {
				theApp->friendlist->RequestSharedFileList(Friend);
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Friend not found.")));
			}
		} else if ((subtag = tag->GetTagByName(EC_TAG_CLIENT))) {
			CUpDownClient * client = theApp->clientlist->FindClientByECID(subtag->GetInt());
			if (client) {
				client->RequestSharedFileList();
				response = new CECPacket(EC_OP_NOOP);
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Client not found.")));
			}
		} else {
			response = new CECPacket(EC_OP_FAILED);
			response->AddTag(CECTag(EC_TAG_STRING,
				wxTRANSLATE("EC_TAG_FRIEND_SHARED requires EC_TAG_FRIEND or EC_TAG_CLIENT.")));
		}
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
	for (CECPacket::const_iterator it = request->begin(); it != request->end(); ++it) {
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

	const CEC_Search_Tag *search_request = static_cast<const CEC_Search_Tag *>(request->GetFirstTagSafe());
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
		/* fall through */
		case EC_SEARCH_KAD:
			if (core_search_type != GlobalSearch) { // Not a global search obviously
				core_search_type = KadSearch;
			}
		/* fall through */
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
	for (CECPacket::const_iterator it = request->begin(); it != request->end(); ++it) {
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
	for (strIntMap::iterator it3 = nameMap.begin(); it3 != nameMap.end(); ++it3) {
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
		static_cast<const CPartFile*>(m_file)->m_SrcpartFrequency :
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
			uint32 *graphData = NULL;
			uint32 *connData = NULL;
			uint64 sessionDl = 0, sessionUl = 0, sessionKad = 0;
			double sessionTimespan = 0.0;
			unsigned int numPoints = theApp->m_statistics->GetHistoryForGui(
				nMaxPoints, (double)nScale, &dTimestamp, &graphData, &connData,
				sessionDl, sessionUl, sessionKad, sessionTimespan);
			if (numPoints) {
				response = new CECPacket(EC_OP_STATSGRAPHS);
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_DATA, 4 * numPoints * sizeof(uint32), graphData));
				// Per-point active uploads / active downloads. Older
				// amulegui builds simply ignore the unknown tag.
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_DATA_CONN, 2 * numPoints * sizeof(uint32), connData));
				delete [] graphData;
				delete [] connData;
				// Latest session totals — let amulegui compute the same
				// kBytesReceived / sTimestamp session average monolithic
				// shows, instead of falling back to a GUI-local integral.
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_SESSION_DL, sessionDl));
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_SESSION_UL, sessionUl));
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_SESSION_KAD, sessionKad));
				response->AddTag(CECTag(EC_TAG_STATSGRAPH_SESSION_TIMESPAN, sessionTimespan));
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
		case EC_OP_ADD_LINK: {
			// Aggregate the per-link results into a single response: until
			// #206 was filed, every iteration overwrote the previous response,
			// so a batch of N-1 successes followed by one failure looked like
			// a total failure to the caller (and vice versa).
			int successCount = 0;
			int failCount = 0;
			for (CECPacket::const_iterator it = request->begin(); it != request->end(); ++it) {
				const CECTag &tag = *it;
				wxString link = tag.GetStringData();
				int category = 0;
				const CECTag *cattag = tag.GetTagByName(EC_TAG_PARTFILE_CAT);
				if (cattag) {
					category = cattag->GetInt();
				}
				AddLogLineC(CFormat(_("ExternalConn: adding link '%s'.")) % link);
				if ( theApp->downloadqueue->AddLink(link, category) ) {
					++successCount;
				} else {
					// Per-link error reasons are already printed by AddLink().
					++failCount;
				}
			}
			if (failCount == 0) {
				response = new CECPacket(EC_OP_NOOP);
			} else if (successCount == 0) {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING, wxTRANSLATE("Invalid link or already on list.")));
			} else {
				response = new CECPacket(EC_OP_FAILED);
				response->AddTag(CECTag(EC_TAG_STRING,
					CFormat(wxString(wxTRANSLATE("%d of %d links failed (invalid or already on list).")))
						% failCount % (failCount + successCount)));
			}
			break;
		}
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
			if ( request->GetDetailLevel() == EC_DETAIL_FULL
				&& CTagSet<uint32, EC_TAG_KNOWNFILE>(request).empty()
				&& (m_my_flags & EC_FLAG_UTF8_NUMBERS)
				&& (m_my_flags & EC_FLAG_LARGE_TAG_COUNT) ) {
				// Per-file bytes cache. Daemon-wide map<ECID, bytes>
				// keyed off CKnownFile::s_globalEcGen; rebuilds only
				// per-file entries whose gen advanced since last use.
				// Concatenated and written through the connection's
				// socket with the same per-connection compression
				// machinery WritePacket uses.
				std::vector<CKnownFile*> snapshot;
				theApp->sharedfiles->CopyFileList(snapshot);
				std::vector<CECFullResponseCache::FileRef> refs;
				refs.reserve(snapshot.size());
				for (size_t i = 0; i < snapshot.size(); ++i) {
					if (!snapshot[i]) continue;
					CECFullResponseCache::FileRef r;
					r.file = snapshot[i];
					r.ecid = snapshot[i]->ECID();
					r.gen = snapshot[i]->GetECGen();
					refs.push_back(r);
				}
				std::vector<std::shared_ptr<const std::vector<unsigned char> > > blobs
					= s_sharedFilesFullCache.GetBlobs(refs);
				s_sharedFilesFullCache.PruneOutsideOf(refs);
				SendCachedBodyResponse(EC_OP_SHARED_FILES, blobs);
				return NULL;
			}
			if ( request->GetDetailLevel() != EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetSharedFiles(request, m_FileEncoder,
					m_lastEcGenSeenShared,
					m_partialUpdateActive, m_lastSentSharedFileIds);
			}
			break;
		case EC_OP_GET_DLOAD_QUEUE:
			if ( request->GetDetailLevel() == EC_DETAIL_FULL
				&& CTagSet<uint32, EC_TAG_PARTFILE>(request).empty()
				&& (m_my_flags & EC_FLAG_UTF8_NUMBERS)
				&& (m_my_flags & EC_FLAG_LARGE_TAG_COUNT) ) {
				std::vector<CPartFile*> snapshot;
				theApp->downloadqueue->CopyFileList(snapshot);
				std::vector<CECFullResponseCache::FileRef> refs;
				refs.reserve(snapshot.size());
				for (size_t i = 0; i < snapshot.size(); ++i) {
					if (!snapshot[i]) continue;
					CECFullResponseCache::FileRef r;
					r.file = snapshot[i];
					r.ecid = snapshot[i]->ECID();
					r.gen = snapshot[i]->GetECGen();
					refs.push_back(r);
				}
				std::vector<std::shared_ptr<const std::vector<unsigned char> > > blobs
					= s_downloadQueueFullCache.GetBlobs(refs);
				s_downloadQueueFullCache.PruneOutsideOf(refs);
				SendCachedBodyResponse(EC_OP_DLOAD_QUEUE, blobs);
				return NULL;
			}
			if ( request->GetDetailLevel() != EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetDownloadQueue(request, m_FileEncoder,
					m_lastEcGenSeenPart,
					m_partialUpdateActive, m_lastSentPartFileIds);
			}
			break;
		//
		// This will evolve into an update-all for inc tags
		//
		case EC_OP_GET_UPDATE:
			if ( request->GetDetailLevel() == EC_DETAIL_INC_UPDATE ) {
				response = Get_EC_Response_GetUpdate(m_FileEncoder, m_obj_tagmap,
					m_lastEcGenSeen,
					m_partialUpdateActive, m_lastSentFileIds);
			}
			break;
		case EC_OP_GET_ULOAD_QUEUE:
			response = Get_EC_Response_GetClientQueue(request, m_obj_tagmap, EC_OP_ULOAD_QUEUE);
			break;
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
			for (CECTag::const_iterator it = request->begin(); it != request->end(); ++it) {
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
			static_cast<const CEC_Prefs_Packet *>(request)->Apply();
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
				CEC_Category_Tag tag(*static_cast<const CEC_Category_Tag*>(request->GetFirstTagSafe()));
				if (tag.Create()) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_CATEGORY, theApp->glob_prefs->GetCatCount() - 1));
					response->AddTag(CECTag(EC_TAG_CATEGORY_PATH, tag.Path()));
				}
				Notify_CategoryAdded();
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
		case EC_OP_UPDATE_CATEGORY:
			if ( request->GetTagCount() == 1 ) {
				CEC_Category_Tag tag(*static_cast<const CEC_Category_Tag*>(request->GetFirstTagSafe()));
				if (tag.Apply()) {
					response = new CECPacket(EC_OP_NOOP);
				} else {
					response = new CECPacket(EC_OP_FAILED);
					response->AddTag(CECTag(EC_TAG_CATEGORY, tag.GetInt()));
					response->AddTag(CECTag(EC_TAG_CATEGORY_PATH, tag.Path()));
				}
				Notify_CategoryUpdate(tag.GetInt());
			} else {
				response = new CECPacket(EC_OP_NOOP);
			}
			break;
		case EC_OP_DELETE_CATEGORY:
			if ( request->GetTagCount() == 1 ) {
				uint32 cat = request->GetFirstTagSafe()->GetInt();
				// this does not only update the gui, but actually deletes the cat
				Notify_CategoryDelete(cat);
			}
			response = new CECPacket(EC_OP_NOOP);
			break;

		//
		// Logging
		//
		case EC_OP_ADDLOGLINE:
			// cppcheck-suppress duplicateBranch
			if (request->GetTagByName(EC_TAG_LOG_TO_STATUS) != NULL) {
				AddLogLineC(request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			} else {
				AddLogLineN(request->GetTagByNameSafe(EC_TAG_STRING)->GetStringData());
			}
			response = new CECPacket(EC_OP_NOOP);
			break;
		case EC_OP_ADDDEBUGLOGLINE:
			// cppcheck-suppress duplicateBranch
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
				response->AddTag(CECTag(EC_TAG_SERVER_VERSION, VERSION));
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
	std::vector<CPartFile*> snapshot;
	theApp->downloadqueue->CopyFileList(snapshot);
	for (std::vector<CPartFile*>::const_iterator it = snapshot.begin();
		it != snapshot.end(); ++it) {
		CPartFile *cur_file = *it;
		PARTFILE_STATUS status = { true, false, false, false, true, cur_file };
		m_dirty_status[cur_file->GetFileHash()] = status;
	}
}

void ECPartFileMsgSource::SetDirty(const CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	if ( m_dirty_status.find(filehash) != m_dirty_status.end() ) {
		m_dirty_status[filehash].m_dirty = true;;
	}
}

void ECPartFileMsgSource::SetNew(const CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) == m_dirty_status.end() );
	PARTFILE_STATUS status = { true, false, false, false, true, file };
	m_dirty_status[filehash] = status;
}

void ECPartFileMsgSource::SetCompleted(const CPartFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) != m_dirty_status.end() );

	m_dirty_status[filehash].m_finished = true;
}

void ECPartFileMsgSource::SetRemoved(const CPartFile *file)
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

			const CPartFile *partfile = it->second.m_file;

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
	std::vector<CKnownFile*> snapshot;
	theApp->sharedfiles->CopyFileList(snapshot);
	for (std::vector<CKnownFile*>::const_iterator it = snapshot.begin();
		it != snapshot.end(); ++it) {
		const CKnownFile *cur_file = *it;
		KNOWNFILE_STATUS status = { true, false, false, true, cur_file };
		m_dirty_status[cur_file->GetFileHash()] = status;
	}
}

void ECKnownFileMsgSource::SetDirty(const CKnownFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	if ( m_dirty_status.find(filehash) != m_dirty_status.end() ) {
		m_dirty_status[filehash].m_dirty = true;;
	}
}

void ECKnownFileMsgSource::SetNew(const CKnownFile *file)
{
	CMD4Hash filehash = file->GetFileHash();
	wxASSERT ( m_dirty_status.find(filehash) == m_dirty_status.end() );
	KNOWNFILE_STATUS status = { true, false, false, true, file };
	m_dirty_status[filehash] = status;
}

void ECKnownFileMsgSource::SetRemoved(const CKnownFile *file)
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

			const CKnownFile *partfile = it->second.m_file;

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

void ECSearchMsgSource::SetDirty(const CSearchFile *file)
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

void ECSearchMsgSource::SetChildDirty(const CSearchFile *file)
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
		//printf("[EC] next update packet; opcode=%x\n",packet ? packet->GetOpCode() : 0xff);
		return packet;
	} else {
		return 0;
	}
}

//
// Interface to notification macros
//
void ECNotifier::DownloadFile_SetDirty(const CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); ++i) {
		CECServerSocket *sock = i->first;
		if ( sock->HaveNotificationSupport() ) {
			ECUpdateMsgSource **notifier_array = i->second;
			static_cast<ECPartFileMsgSource *>(notifier_array[EC_PARTFILE])->SetDirty(file);
		}
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_RemoveFile(const CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); ++i) {
		ECUpdateMsgSource **notifier_array = i->second;
		static_cast<ECPartFileMsgSource *>(notifier_array[EC_PARTFILE])->SetRemoved(file);
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_RemoveSource(const CPartFile *)
{
	// per-partfile source list is not supported (yet), and IMHO quite useless
}

void ECNotifier::DownloadFile_AddFile(const CPartFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); ++i) {
		ECUpdateMsgSource **notifier_array = i->second;
		static_cast<ECPartFileMsgSource *>(notifier_array[EC_PARTFILE])->SetNew(file);
	}
	NextPacketToSocket();
}

void ECNotifier::DownloadFile_AddSource(const CPartFile *)
{
	// per-partfile source list is not supported (yet), and IMHO quite useless
}

void ECNotifier::SharedFile_AddFile(const CKnownFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); ++i) {
		ECUpdateMsgSource **notifier_array = i->second;
		static_cast<ECKnownFileMsgSource *>(notifier_array[EC_KNOWN])->SetNew(file);
	}
	NextPacketToSocket();
}

void ECNotifier::SharedFile_RemoveFile(const CKnownFile *file)
{
	for(std::map<CECServerSocket *, ECUpdateMsgSource **>::iterator i = m_msg_source.begin();
		i != m_msg_source.end(); ++i) {
		ECUpdateMsgSource **notifier_array = i->second;
		static_cast<ECKnownFileMsgSource *>(notifier_array[EC_KNOWN])->SetRemoved(file);
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
		i != m_msg_source.end(); ++i) {
		CECServerSocket *sock = i->first;
		if ( sock->HaveNotificationSupport() && !sock->DataPending() ) {
			ECUpdateMsgSource **notifier_array = i->second;
			// Same ownership contract as WriteDoneAndQueueEmpty: the
			// CECPacket from GetNextPacket is caller-owned and
			// SendPacket only serialises it.  Wrap so it's freed at
			// scope exit (#765).
			CSmartPtr<CECPacket> packet(GetNextPacket(notifier_array));
			if (packet) {
				//printf("[EC] sending update packet; opcode=%x\n",packet->GetOpCode());
				sock->SendPacket(packet.get());
			}
		}
	}
}

// File_checked_for_headers
