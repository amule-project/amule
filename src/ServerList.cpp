//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/wx.h>

#include "ServerList.h"			// Interface declarations.

#include <protocol/Protocols.h>
#include <protocol/ed2k/Constants.h>
#include <common/DataFileVersion.h>
#include <tags/ServerTags.h>

#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/url.h>			// Needed for wxURL
#include <wx/tokenzr.h>

#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ServerConnect.h"		// Needed for CServerConnect
#include "Server.h"			// Needed for CServer and SRV_PR_*
#include "OtherStructs.h"		// Needed for ServerMet_Struct
#include "CFile.h"			// Needed for CFile
#include "HTTPDownload.h"		// Needed for HTTPThread
#include "Preferences.h"		// Needed for thePrefs
#include "amule.h"			// Needed for theApp
#include "Statistics.h"			// Needed for theStats
#include "Packet.h"			// Neeed for CPacket
#include "Logger.h"
#include "ScopedPtr.h"
#include <common/Format.h>
#include "IPFilter.h"
#include <common/FileFunctions.h>	// Needed for UnpackArchive
#include <common/TextFile.h>	// Needed for CTextFile

CServerList::CServerList()
{
	m_serverpos = m_servers.end();
	m_statserverpos = m_servers.end();
	m_nLastED2KServerLinkCheck = ::GetTickCount();
	m_initialized = false;
}


bool CServerList::Init()
{
	// Load Metfile
	bool bRes = LoadServerMet(CPath(thePrefs::GetConfigDir() + wxT("server.met")));

	// insert static servers from textfile
	m_staticServersConfig = thePrefs::GetConfigDir() + wxT("staticservers.dat");
	LoadStaticServers();

	// Send the auto-update of server.met via HTTPThread requests
	current_url_index = 0;
	if ( thePrefs::AutoServerlist()) {
		AutoUpdate();
	}

	m_initialized = true;
	return bRes;
}


bool CServerList::LoadServerMet(const CPath& path)
{
	AddLogLineN(CFormat(_("Loading server.met file: %s")) % path);

	bool merge = !m_servers.empty();

	if (!path.FileExists()) {
		AddLogLineN(_("Server.met file not found!"));
		return false;
	}

	// Try to unpack the file, might be an archive
	const wxChar* mets[] = { wxT("server.met"), NULL };
	// Try to unpack the file, might be an archive
	if (UnpackArchive(path, mets).second != EFT_Met) {
		AddLogLineC(CFormat(_("Failed to load server.met file '%s', unknown format encountered.")) % path);
		return false;
	}

	CFile servermet(path, CFile::read);
	if ( !servermet.IsOpened() ){
		AddLogLineN(_("Failed to open server.met!") );
		return false;
	}


	try {
		Notify_ServerFreeze();

		byte version = servermet.ReadUInt8();

		if (version != 0xE0 && version != MET_HEADER) {
			AddLogLineC(CFormat(_("Server.met file corrupt, found invalid versiontag: 0x%x, size %i")) % version % sizeof(version));
			Notify_ServerThaw();
			return false;
		}

		uint32 fservercount = servermet.ReadUInt32();

		ServerMet_Struct sbuffer;
		uint32 iAddCount = 0;

		for ( uint32 j = 0; j < fservercount; ++j ) {
			sbuffer.ip		= servermet.ReadUInt32();
			sbuffer.port		= servermet.ReadUInt16();
			sbuffer.tagcount	= servermet.ReadUInt32();

			CServer* newserver = new CServer(&sbuffer);

			// Load tags
			for ( uint32 i = 0; i < sbuffer.tagcount; ++i ) {
				newserver->AddTagFromFile(&servermet);
			}

			// Server priorities are not in sorted order
			// High = 1, Low = 2, Normal = 0, so we have to check
			// in a less logical fashion.
			int priority = newserver->GetPreferences();
			if (priority < SRV_PR_MIN || priority > SRV_PR_MAX) {
				newserver->SetPreference(SRV_PR_NORMAL);
			}

			// set listname for server
			if ( newserver->GetListName().IsEmpty() ) {
				newserver->SetListName(wxT("Server ") +newserver->GetAddress());
			}


			if ( !theApp->AddServer(newserver) ) {
				CServer* update = GetServerByAddress(newserver->GetAddress(), newserver->GetPort());
				if(update) {
					update->SetListName( newserver->GetListName());
					if(!newserver->GetDescription().IsEmpty()) {
						update->SetDescription( newserver->GetDescription());
					}
					Notify_ServerRefresh(update);
				}
				delete newserver;
			} else {
				++iAddCount;
			}

		}

		Notify_ServerThaw();

		// cppcheck-suppress duplicateBranch
		if (!merge) {
			AddLogLineC(CFormat(wxPLURAL("%i server in server.met found", "%i servers in server.met found", fservercount)) % fservercount);
		} else {
			AddLogLineC(CFormat(wxPLURAL("%d server added", "%d servers added", iAddCount)) % iAddCount);
		}
	} catch (const CInvalidPacket& err) {
		AddLogLineC(_("Error: the file 'server.met' is corrupted: ") + err.what());
		Notify_ServerThaw();
		return false;
	} catch (const CSafeIOException& err) {
		AddLogLineC(_("IO error while reading 'server.met': ") + err.what());
		Notify_ServerThaw();
		return false;
	}

	return true;
}


bool CServerList::AddServer(CServer* in_server, bool fromUser)
{
	if ( !in_server->GetPort() ) {
		if ( fromUser ) {
			AddLogLineC(CFormat( _("Server not added: [%s:%d] does not specify a valid port.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}

		return false;
	} else if (
				!in_server->HasDynIP() &&
				(
					!IsGoodIP( in_server->GetIP(), thePrefs::FilterLanIPs() ) ||
					(	// don't test for filtered while ipfilter is still loading,
						// it will be filtered when filter loading is finished
						theApp->ipfilter->IsReady()
						&& theApp->ipfilter->IsFiltered(in_server->GetIP(), true))
				)
	          ) {
		if ( fromUser ) {
			AddLogLineC(CFormat( _("Server not added: The IP of [%s:%d] is filtered or invalid.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}

		return false;
	}

	CServer* test_server = GetServerByAddress(in_server->GetAddress(), in_server->GetPort());
	// Avoid duplicate (dynIP) servers: If the server which is to be added, is a dynIP-server
	// but we don't know yet it's DN, we need to search for an already available server with
	// that IP.
	if (test_server == NULL && in_server->GetIP() != 0) {
		test_server = GetServerByIPTCP(in_server->GetIP(), in_server->GetPort());
	}

	if (test_server) {
		if ( fromUser ) {
			AddLogLineC(CFormat( _("Server not added: Server with matching IP:Port [%s:%d] found in list.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}

		test_server->ResetFailedCount();
		Notify_ServerRefresh( test_server );

		return false;
	}

	theStats::AddServer();

	m_servers.push_back(in_server);
	NotifyObservers( EventType( EventType::INSERTED, in_server ) );

	if ( fromUser ) {
		AddLogLineC(CFormat( _("Server added: Server at [%s:%d] using the name '%s'.") )
				% in_server->GetAddress()
				% in_server->GetPort()
				% in_server->GetListName()
		);
	}


	return true;
}


void CServerList::ServerStats()
{
	uint32 tNow = ::GetTickCount();

	if (theApp->IsConnectedED2K() && !m_servers.empty()) {
		CServer* ping_server = GetNextStatServer();
		CServer* test = ping_server;
		if (!ping_server) {
			return;
		}

		while (ping_server->GetLastPingedTime() && (tNow - ping_server->GetLastPingedTime()) < UDPSERVSTATREASKTIME) {
			ping_server = GetNextStatServer();
			if (ping_server == test) {
				return;
			}
		}

		if (ping_server->GetFailedCount() >= thePrefs::GetDeadserverRetries() && thePrefs::DeadServer() && !ping_server->IsStaticMember()) {
			RemoveServer(ping_server);
			return;
		}

		srand((unsigned)time(NULL));
		ping_server->SetRealLastPingedTime(tNow); // this is not used to calcualte the next ping, but only to ensure a minimum delay for premature pings
		if (!ping_server->GetCryptPingReplyPending() && (!ping_server->GetLastPingedTime() || (tNow - ping_server->GetLastPingedTime()) >= UDPSERVSTATREASKTIME) && theApp->GetPublicIP() && thePrefs::IsServerCryptLayerUDPEnabled()) {
			// We try a obfsucation ping first and wait 20 seconds for an answer
			// if it doesn't get responsed, we don't count it as error but continue with a normal ping
			ping_server->SetCryptPingReplyPending(true);
			uint32 nPacketLen = 4 + (uint8)(rand() % 16); // max padding 16 bytes
			CScopedArray<byte> pRawPacket(nPacketLen);
			uint32 dwChallenge = (rand() << 17) | (rand() << 2) | (rand() & 0x03);
			if (dwChallenge == 0) {
				dwChallenge++;
			}

			memcpy(pRawPacket.get(), &dwChallenge, sizeof(uint32));
			for (uint32 i = 4; i < nPacketLen; i++) { // fillng up the remaining bytes with random data
				pRawPacket[i] = (uint8)rand();
			}

			ping_server->SetChallenge(dwChallenge);
			ping_server->SetLastPinged(tNow);
			ping_server->SetLastPingedTime((tNow - (uint32)UDPSERVSTATREASKTIME) + 20); // give it 20 seconds to respond

			AddDebugLogLineN(logServerUDP, CFormat(wxT(">> Sending OP__GlobServStatReq (obfuscated) to server %s:%u")) % ping_server->GetAddress() % ping_server->GetPort());

			CPacket* packet = new CPacket(pRawPacket[1], nPacketLen - 2, pRawPacket[0]);
			packet->CopyToDataBuffer(0, pRawPacket.get() + 2, nPacketLen - 2);

			theStats::AddUpOverheadServer(packet->GetPacketSize());
			theApp->serverconnect->SendUDPPacket(packet, ping_server, true, true /*raw packet*/, 12 /* Port offset is 12 for obfuscated encryption*/);
		} else if (ping_server->GetCryptPingReplyPending() || theApp->GetPublicIP() == 0 || !thePrefs::IsServerCryptLayerUDPEnabled()){
			// our obfsucation ping request was not answered, so probably the server doesn'T supports obfuscation
			// continue with a normal request
			if (ping_server->GetCryptPingReplyPending() && thePrefs::IsServerCryptLayerUDPEnabled()) {
				AddDebugLogLineN(logServerUDP, wxT("CryptPing failed for server ") + ping_server->GetListName());
			} else if (thePrefs::IsServerCryptLayerUDPEnabled()) {
				AddDebugLogLineN(logServerUDP, wxT("CryptPing skipped because our public IP is unknown for server ") + ping_server->GetListName());
			}

			ping_server->SetCryptPingReplyPending(false);

			CPacket* packet = new CPacket(OP_GLOBSERVSTATREQ, 4, OP_EDONKEYPROT);
			uint32 challenge = 0x55AA0000 + (uint16)rand();
			ping_server->SetChallenge(challenge);
			packet->CopyUInt32ToDataBuffer(challenge);
			ping_server->SetLastPinged(tNow);
			ping_server->SetLastPingedTime(tNow - (rand() % HR2S(1)));
			ping_server->AddFailedCount();
			Notify_ServerRefresh(ping_server);
			theStats::AddUpOverheadServer(packet->GetPacketSize());
			theApp->serverconnect->SendUDPPacket(packet, ping_server, true);
		} else {
			wxFAIL;
		}
	}
}


void CServerList::RemoveServer(CServer* in_server)
{
	if (in_server == theApp->serverconnect->GetCurrentServer()) {
		theApp->ShowAlert(_("You are connected to the server you are trying to delete. please disconnect first."), _("Info"), wxOK);
	} else {
		CInternalList::iterator it = std::find(m_servers.begin(), m_servers.end(), in_server);
		if ( it != m_servers.end() ) {
			if (theApp->downloadqueue->GetUDPServer() == in_server) {
				theApp->downloadqueue->SetUDPServer( 0 );
			}

			NotifyObservers( EventType( EventType::REMOVED, in_server ) );

			if (m_serverpos == it) {
				++m_serverpos;
			}
			if (m_statserverpos == it) {
				++m_statserverpos;
			}
			m_servers.erase(it);
			theStats::DeleteServer();

			Notify_ServerRemove(in_server);
			delete in_server;
		}
	}
}


void CServerList::RemoveAllServers()
{
	NotifyObservers( EventType( EventType::CLEARED ) );

	theStats::DeleteAllServers();
	// no connection, safely remove all servers
	while ( !m_servers.empty() ) {
		delete m_servers.back();
		m_servers.pop_back();
	}
	m_serverpos = m_servers.end();
	m_statserverpos = m_servers.end();
}


void CServerList::GetStatus(uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile,float &occ)
{
	failed = 0;
	user = 0;
	file = 0;
	tuser=0;
	tfile = 0;
	occ=0;
	uint32 maxusers=0;
	uint32 tuserk = 0;

	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		const CServer* const curr = *it;
		if( curr->GetFailedCount() ) {
			++failed;
		} else {
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
		tuser += curr->GetUsers();
		tfile += curr->GetFiles();

		if (curr->GetMaxUsers()) {
			tuserk += curr->GetUsers(); // total users on servers with known maximum
			maxusers+=curr->GetMaxUsers();
		}
	}
	if (maxusers>0) {
		occ=(float)(tuserk*100)/maxusers;
	}
}


void CServerList::GetUserFileStatus(uint32 &user, uint32 &file)
{
	user = 0;
	file = 0;
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		const CServer* const curr = *it;
		if( !curr->GetFailedCount() ) {
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
	}
}


CServerList::~CServerList()
{
	if (thePrefs::GetNetworkED2K() && IsInitialized()) {
		SaveServerMet();
	}
	while ( !m_servers.empty() ) {
		delete m_servers.back();
		m_servers.pop_back();
	}
}


void CServerList::LoadStaticServers()
{
	if ( !CPath::FileExists(m_staticServersConfig) ) {
		return;
	}

	wxFileInputStream stream(m_staticServersConfig);
	wxTextInputStream f(stream);

	while ( !stream.Eof() ) {
		wxString line = f.ReadLine();

		// Skip comments
		if (line.IsEmpty() || line.GetChar(0) == '#' || line.GetChar(0) == '/') {
			continue;
		}

		wxStringTokenizer tokens( line, wxT(",") );

		if ( tokens.CountTokens() != 3 ) {
			continue;
		}


		// format is host:port,priority,Name
		wxString addy = tokens.GetNextToken().Strip( wxString::both );
		wxString prio = tokens.GetNextToken().Strip( wxString::both );
		wxString name = tokens.GetNextToken().Strip( wxString::both );

		wxString host = addy.BeforeFirst( wxT(':') );
		wxString port = addy.AfterFirst( wxT(':') );


		int priority = StrToLong( prio );
		if (priority < SRV_PR_MIN || priority > SRV_PR_MAX) {
			priority = SRV_PR_NORMAL;
		}


		// We need a valid name for the list
		if ( name.IsEmpty() ) {
			name = addy;
		}


		// create server object and add it to the list
		CServer* server = new CServer( StrToLong( port ), host );

		server->SetListName( name );
		server->SetIsStaticMember( true );
		server->SetPreference( priority );


		// Try to add the server to the list
		if ( !theApp->AddServer( server ) ) {
			delete server;
			CServer* existing = GetServerByAddress( host, StrToULong( port ) );
			if ( existing) {
				existing->SetListName( name );
				existing->SetIsStaticMember( true );
				existing->SetPreference( priority );

				Notify_ServerRefresh( existing );
			}
		}
	}
}

void CServerList::SaveStaticServers()
{
	CTextFile file;
	if (!file.Open(m_staticServersConfig, CTextFile::write)) {
		AddLogLineN(CFormat( _("Failed to open '%s'") ) % m_staticServersConfig );
		return;
	}

	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		const CServer* server = *it;

		if (server->IsStaticMember()) {
			file.WriteLine(CFormat(wxT("%s:%u,%u,%s"))
				% server->GetAddress() % server->GetPort()
				% server->GetPreferences() % server->GetListName());
		}
	}

	file.Close();
}

struct ServerPriorityComparator {
	// Return true iff lhs should strictly appear earlier in the list than rhs.
	// In this case, we want higher priority servers to appear earlier.
	bool operator()(const CServer* lhs, const CServer* rhs) {
		wxASSERT
			(
			rhs->GetPreferences() == SRV_PR_LOW		||
			rhs->GetPreferences() == SRV_PR_NORMAL	||
			rhs->GetPreferences() == SRV_PR_HIGH
			);
		switch (lhs->GetPreferences()) {
			case SRV_PR_LOW:
				return false;
			case SRV_PR_NORMAL:
				return rhs->GetPreferences() == SRV_PR_LOW;
			case SRV_PR_HIGH:
				return rhs->GetPreferences() != SRV_PR_HIGH;
			default:
				wxFAIL;
				return false;
		}
	}
};

void CServerList::Sort()
{
	m_servers.sort(ServerPriorityComparator());
	// Once the list has been sorted, it doesn't really make sense to continue
	// traversing the new order from the old position.  Plus, there's a bug in
	// version of libstdc++ before gcc4 such that iterators that were equal to
	// end() were left dangling.
	m_serverpos = m_servers.begin();
	m_statserverpos = m_servers.begin();
}


CServer* CServerList::GetNextServer(bool bOnlyObfuscated)
{
	while (bOnlyObfuscated && (m_serverpos != m_servers.end()) && !((*m_serverpos)->SupportsObfuscationTCP() || (*m_serverpos)->SupportsObfuscationUDP())) {
		wxASSERT(*m_serverpos != NULL);
		++m_serverpos;
	}

	if (m_serverpos == m_servers.end()) {
		return 0;
	} else {
		if (*m_serverpos) {
			return *m_serverpos++;
		} else {
			return 0;
		}
	}
}


CServer* CServerList::GetNextStatServer()
{
	if (m_statserverpos == m_servers.end()) {
		m_statserverpos = m_servers.begin();
	}

	if (m_statserverpos == m_servers.end()) {
		return 0;
	} else {
		wxASSERT(*m_statserverpos != NULL);
		return *m_statserverpos++;
	}
}


CServer* CServerList::GetServerByAddress(const wxString& address, uint16 port) const
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		CServer* const s = *it;
		if (port == s->GetPort() && s->GetAddress() == address) {
			return s;
		}
	}
	return NULL;
}


CServer* CServerList::GetServerByIP(uint32 nIP) const
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s = *it;
		if (s->GetIP() == nIP)
			return s;
	}
	return NULL;
}


CServer* CServerList::GetServerByIPTCP(uint32 nIP, uint16 nPort) const
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s = *it;
		if (s->GetIP() == nIP && s->GetPort() == nPort)
			return s;
	}
	return NULL;
}


CServer* CServerList::GetServerByIPUDP(uint32 nIP, uint16 nUDPPort, bool bObfuscationPorts) const
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s =*it;
		if (s->GetIP() == nIP
			&& (s->GetPort() == nUDPPort-4
				|| (bObfuscationPorts && s->GetObfuscationPortUDP() == nUDPPort)
				|| s->GetPort() == nUDPPort - 12))
			return s;
	}
	return NULL;
}


CServer* CServerList::GetServerByECID(uint32 ecid) const
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s = *it;
		if (s->ECID() == ecid) {
			return s;
		}
	}
	return NULL;
}


void CServerList::SetStaticServer(CServer* server, bool isStatic)
{
	server->SetIsStaticMember(isStatic);
	Notify_ServerRefresh(server);
	SaveStaticServers();
}


void CServerList::SetServerPrio(CServer* server, uint32 prio)
{
	server->SetPreference(prio);
	Notify_ServerRefresh(server);
}


bool CServerList::SaveServerMet()
{
	CPath curservermet = CPath(thePrefs::GetConfigDir() + wxT("server.met"));

	CFile servermet(curservermet, CFile::write_safe);
	if (!servermet.IsOpened()) {
		AddLogLineN(_("Failed to save server.met!"));
		return false;
	}

	try {
		servermet.WriteUInt8(0xE0);
		servermet.WriteUInt32( m_servers.size() );

		for ( CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
			const CServer* const server = *it;

			uint16 tagcount = 12;
			if (!server->GetListName().IsEmpty()) {
				++tagcount;
			}
			if (!server->GetDynIP().IsEmpty()) {
				++tagcount;
			}
			if (!server->GetDescription().IsEmpty()) {
				++tagcount;
			}
			if (server->GetConnPort() != server->GetPort()) {
				++tagcount;
			}

			// For unicoded name, description, and dynip
			if ( !server->GetListName().IsEmpty() ) {
				++tagcount;
			}
			if ( !server->GetDynIP().IsEmpty() ) {
				++tagcount;
			}
			if ( !server->GetDescription().IsEmpty() ) {
				++tagcount;
			}
			if (!server->GetVersion().IsEmpty()) {
				++tagcount;
			}

			if (server->GetServerKeyUDP(true)) {
				++tagcount;
			}

			if (server->GetServerKeyUDPIP()) {
				++tagcount;
			}

			if (server->GetObfuscationPortTCP()) {
				++tagcount;
			}

			if (server->GetObfuscationPortUDP()) {
				++tagcount;
			}

			servermet.WriteUInt32(server->GetIP());
			servermet.WriteUInt16(server->GetPort());
			servermet.WriteUInt32(tagcount);

			if ( !server->GetListName().IsEmpty() ) {
				// This is BOM to keep eMule compatibility
				CTagString( ST_SERVERNAME, server->GetListName()).WriteTagToFile( &servermet,  utf8strOptBOM);
				CTagString( ST_SERVERNAME, server->GetListName()).WriteTagToFile( &servermet );
			}

			if ( !server->GetDynIP().IsEmpty() ) {
				// This is BOM to keep eMule compatibility
				CTagString( ST_DYNIP, server->GetDynIP()).WriteTagToFile( &servermet, utf8strOptBOM );
				CTagString( ST_DYNIP, server->GetDynIP()).WriteTagToFile( &servermet );
			}

			if ( !server->GetDescription().IsEmpty() ) {
				// This is BOM to keep eMule compatibility
				CTagString( ST_DESCRIPTION, server->GetDescription()).WriteTagToFile( &servermet, utf8strOptBOM );
				CTagString( ST_DESCRIPTION, server->GetDescription()).WriteTagToFile( &servermet );
			}

			if ( server->GetConnPort() != server->GetPort() ) {
				CTagString( ST_AUXPORTSLIST,	server->GetAuxPortsList()	).WriteTagToFile( &servermet );
			}

			CTagInt32( ST_FAIL,       server->GetFailedCount()   ).WriteTagToFile( &servermet );
			CTagInt32( ST_PREFERENCE, server->GetPreferences()   ).WriteTagToFile( &servermet );
			CTagInt32( wxT("users"),  server->GetUsers()         ).WriteTagToFile( &servermet );
			CTagInt32( wxT("files"),  server->GetFiles()         ).WriteTagToFile( &servermet );
			CTagInt32( ST_PING,       server->GetPing()          ).WriteTagToFile( &servermet );
			CTagInt32( ST_LASTPING,   server->GetLastPingedTime()).WriteTagToFile( &servermet );
			CTagInt32( ST_MAXUSERS,   server->GetMaxUsers()      ).WriteTagToFile( &servermet );
			CTagInt32( ST_SOFTFILES,  server->GetSoftFiles()     ).WriteTagToFile( &servermet );
			CTagInt32( ST_HARDFILES,  server->GetHardFiles()     ).WriteTagToFile( &servermet );
			if (!server->GetVersion().IsEmpty()){
				CTagString( ST_VERSION,	server->GetVersion() ).WriteTagToFile( &servermet, utf8strOptBOM );
				CTagString( ST_VERSION,	server->GetVersion() ).WriteTagToFile( &servermet );
			}
			CTagInt32( ST_UDPFLAGS,   server->GetUDPFlags()      ).WriteTagToFile( &servermet );
			CTagInt32( ST_LOWIDUSERS, server->GetLowIDUsers()    ).WriteTagToFile( &servermet );

			if (server->GetServerKeyUDP(true)) {
				CTagInt32(ST_UDPKEY, server->GetServerKeyUDP(true)).WriteTagToFile( &servermet );
			}

			if (server->GetServerKeyUDPIP()) {
				CTagInt32(ST_UDPKEYIP, server->GetServerKeyUDPIP()).WriteTagToFile( &servermet );
			}

			if (server->GetObfuscationPortTCP()) {
				CTagInt16(ST_TCPPORTOBFUSCATION, server->GetObfuscationPortTCP()).WriteTagToFile( &servermet );
			}

			if (server->GetObfuscationPortUDP()) {
				CTagInt16(ST_UDPPORTOBFUSCATION, server->GetObfuscationPortUDP()).WriteTagToFile( &servermet );
			}

		}
		// Now server.met.new is ready to be closed and renamed to server.met.
		// But first rename existing server.met to server.met.bak (replacing old .bak file).
		const CPath oldservermet = CPath(thePrefs::GetConfigDir() + wxT("server.met.bak"));
		if (curservermet.FileExists()) {
			CPath::RenameFile(curservermet, oldservermet, true);
		}

		servermet.Close();

	} catch (const CIOFailureException& e) {
		AddLogLineC(wxT("IO failure while writing 'server.met': ") + e.what());
		return false;
	}

	return true;
}


void CServerList::RemoveDeadServers()
{
	if ( thePrefs::DeadServer() ) {
		for ( CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ) {
			CServer* server = *it++;
			if ( server->GetFailedCount() > thePrefs::GetDeadserverRetries() && !server->IsStaticMember()) {
				RemoveServer(server);
			}
		}
	}
}

void CServerList::UpdateServerMetFromURL(const wxString& strURL)
{
	if (strURL.Find(wxT("://")) == -1) {
		AddLogLineC(_("Invalid URL"));
		return;
	}
	m_URLUpdate = strURL;
	wxString strTempFilename(thePrefs::GetConfigDir() + wxT("server.met.download"));
	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURL, strTempFilename, thePrefs::GetConfigDir() + wxT("server.met"), HTTP_ServerMet, false, false);
	downloader->Create();
	downloader->Run();
}


bool CServerList::DownloadFinished(uint32 result)
{
	bool ret = false;
	if(result == HTTP_Success) {
		const CPath tempFilename = CPath(thePrefs::GetConfigDir() + wxT("server.met.download"));

		// curl succeeded. proceed with server.met loading
		LoadServerMet(tempFilename);
		SaveServerMet();

		// So, file is loaded and merged, and also saved
		CPath::RemoveFile(tempFilename);
		AddLogLineN(CFormat(_("Finished downloading the server list from %s")) % m_URLUpdate);
		ret = true;
	// cppcheck-suppress duplicateBranch
	} else if (result == HTTP_Skipped) {
		AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % wxT("server.met"));
	} else {
		AddLogLineC(CFormat(_("Failed to download %s from %s")) % wxT("server.met") % m_URLUpdate);
	}
	return ret;
}


void CServerList::AutoUpdate()
{

	uint8 url_count = theApp->glob_prefs->adresses_list.GetCount();

	if (!url_count) {
		AddLogLineC(_("No server list address entry in 'addresses.dat' found. Please paste a valid server list address into this file in order to auto-update your server list"));
		return;
	}
	// Do current URL. Callback function will take care of the others.
	while ( current_url_index < url_count ) {
		wxString URI = theApp->glob_prefs->adresses_list[current_url_index];
		// We use wxURL to validate the URI
		if ( wxURL( URI ).GetError() == wxURL_NOERR ) {
			// Ok, got a valid URI
			m_URLUpdate = URI;
			wxString strTempFilename =
				thePrefs::GetConfigDir() + wxT("server_auto.met");
			AddLogLineC(CFormat(
				_("Start downloading server list from %s")) % URI);
			CHTTPDownloadThread *downloader = new CHTTPDownloadThread(
				URI, strTempFilename, thePrefs::GetConfigDir() + wxT("server.met"), HTTP_ServerMetAuto, false, false);
			downloader->Create();
			downloader->Run();

			return;
		} else {
			AddLogLineC(CFormat(
				_("WARNING: invalid URL specified for auto-updating of servers: %s") ) % URI);
		}
		current_url_index++;
	}
	AddLogLineC(_("No valid server.met auto-download url on addresses.dat"));
}


void CServerList::AutoDownloadFinished(uint32 result)
{
	if (result == HTTP_Success) {
		CPath tempFilename = CPath(thePrefs::GetConfigDir() + wxT("server_auto.met"));

		// curl succeeded. proceed with server.met loading
		LoadServerMet(tempFilename);
		SaveServerMet();

		// So, file is loaded and merged, and also saved
		CPath::RemoveFile(tempFilename);
	} else {
		AddLogLineC(CFormat(_("Failed to download the server list from %s") ) % m_URLUpdate);
	}

	++current_url_index;

	if (current_url_index < theApp->glob_prefs->adresses_list.GetCount()) {
		// Next!
		AutoUpdate();
	}
}


void CServerList::ObserverAdded( ObserverType* o )
{
	CObservableQueue<CServer*>::ObserverAdded( o );

	EventType::ValueList ilist;
	ilist.reserve( m_servers.size() );
	ilist.assign( m_servers.begin(), m_servers.end() );

	NotifyObservers( EventType( EventType::INITIAL, &ilist ), o );
}


uint32 CServerList::GetAvgFile() const
{
	//Since there is no real way to know how many files are in the kad network,
	//I figure to try to use the ED2K network stats to find how many files the
	//average user shares..
	uint32 totaluser = 0;
	uint32 totalfile = 0;
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
		const CServer* const curr = *it;
		//If this server has reported Users/Files and doesn't limit it's files too much
		//use this in the calculation..
		if( curr->GetUsers() && curr->GetFiles() && curr->GetSoftFiles() > 1000 ) {
			totaluser += curr->GetUsers();
			totalfile += curr->GetFiles();
		}
	}
	//If the user count is a little low, don't send back a average..
	//I added 50 to the count as many servers do not allow a large amount of files to be shared..
	//Therefore the estimate here will be lower then the actual.
	//I would love to add a way for the client to send some statistics back so we could see the real
	//values here..
	if ( totaluser > 500000 ) {
		return (totalfile/totaluser)+50;
	} else {
		return 0;
	}
}


std::vector<const CServer*> CServerList::CopySnapshot() const
{
	std::vector<const CServer*> result;
	result.reserve(m_servers.size());
	result.assign(m_servers.begin(), m_servers.end());
	return result;
}


void CServerList::FilterServers()
{
	CInternalList::iterator it = m_servers.begin();
	while (it != m_servers.end()) {
		CServer* server = *it++;

		if (server->HasDynIP()) {
			continue;
		}

		if (theApp->ipfilter->IsFiltered(server->GetIP(), true)) {
			if (server == theApp->serverconnect->GetCurrentServer()) {
				AddLogLineC(_("Local server is filtered by the IPFilters, reconnecting to a different server!"));
				theApp->serverconnect->Disconnect();
				RemoveServer(server);
				theApp->serverconnect->ConnectToAnyServer();
			} else {
				RemoveServer(server);
			}
		}
	}
}

void CServerList::CheckForExpiredUDPKeys() {

	if (!thePrefs::IsServerCryptLayerUDPEnabled()) {
		return;
	}

	uint32 cKeysTotal = 0;
	uint32 cKeysExpired = 0;
	uint32 cPingDelayed = 0;
	const uint32 dwIP = theApp->GetPublicIP();
	const uint32 tNow = ::GetTickCount();
	wxASSERT( dwIP != 0 );

	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
        CServer* pServer = *it;
		if (pServer->SupportsObfuscationUDP() && pServer->GetServerKeyUDP(true) != 0 && pServer->GetServerKeyUDPIP() != dwIP){
			cKeysTotal++;
			cKeysExpired++;
			if (tNow - pServer->GetRealLastPingedTime() < UDPSERVSTATMINREASKTIME){
				cPingDelayed++;
				// next ping: Now + (MinimumDelay - already elapsed time)
				pServer->SetLastPingedTime((tNow - (uint32)UDPSERVSTATREASKTIME) + (UDPSERVSTATMINREASKTIME - (tNow - pServer->GetRealLastPingedTime())));
			} else {
				pServer->SetLastPingedTime(0);
			}
		} else if (pServer->SupportsObfuscationUDP() && pServer->GetServerKeyUDP(false) != 0) {
			cKeysTotal++;
		}
	}
}
// File_checked_for_headers
