//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ServerList.h"
#endif

#include "Types.h"

#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif

#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/filename.h>	// Needed for wxFileName
#include <wx/url.h>			// Needed for wxURL
#include <wx/tokenzr.h>

#include "ServerList.h"			// Interface declarations.
#include "ListenSocket.h"		// Needed for CListenSocket
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ServerConnect.h"		// Needed for CServerConnect
#include "Server.h"				// Needed for CServer and SRV_PR_*
#include "OtherStructs.h"		// Needed for ServerMet_Struct
#include "Packet.h"				// Needed for CInvalidPacket
#include "OPCodes.h"			// Needed for MET_HEADER
#include "CFile.h"				// Needed for CSafeFile
#include "HTTPDownload.h"		// Needed for HTTPThread
#include "Preferences.h"		// Needed for CPreferences
#include "amule.h"				// Needed for theApp
#include "GetTickCount.h"		// Neeed for GetTickCount
#include "NetworkFunctions.h"	// Needed for StringIPtoUint32
#include "Statistics.h"			// Needed for CStatistics
#include "StringFunctions.h" // Needed for unicode2char 
#include "Logger.h"
#include "Format.h"
#include "IPFilter.h"

CServerList::CServerList()
{
	m_serverpos = m_servers.end();
	m_statserverpos = m_servers.end();
	delservercount = 0;
	m_nLastED2KServerLinkCheck = ::GetTickCount();
}


bool CServerList::Init()
{
	// Load Metfile
	wxString strTempFilename;
	strTempFilename = theApp.ConfigDir + wxT("server.met");
	bool bRes = LoadServerMet(strTempFilename);

	// insert static servers from textfile
	strTempFilename=  theApp.ConfigDir + wxT("staticservers.dat");
	LoadStaticServers( strTempFilename );
	
	// Send the auto-update of server.met via HTTPThread requests
	current_url_index = 0;
	if ( thePrefs::AutoServerlist()) {
		AutoUpdate();
	}	
	
	return bRes;
}


bool CServerList::LoadServerMet(const wxString& strFile)
{
	AddLogLineM( false, CFormat( _("Loading server.met file: %s") ) % strFile );
	
	bool merge = !m_servers.empty();
	
	if ( !wxFileExists(strFile) ) {
		AddLogLineM( false, _("Server.met file not found!") );
		return false;
	}

	CSafeFile servermet( strFile,CFile::read );
	if ( !servermet.IsOpened() ){ 
		AddLogLineM( false, _("Failed to open server.met!") );
		return false;
	}

	
	try {
		Notify_ServerFreeze();
		
		byte version = servermet.ReadUInt8();
		
		if (version != 0xE0 && version != MET_HEADER) {
			AddLogLineM(false, wxString::Format(_("Invalid versiontag in server.met (0x%x , size %i)!"),version, sizeof(version)));
			throw CInvalidPacket(wxT("Corrupted server.met"));
		}

		uint32 fservercount = servermet.ReadUInt32();

		ServerMet_Struct sbuffer;
		uint32 iAddCount = 0;

		for ( uint32 j = 0; j < fservercount; ++j ) {
			sbuffer.ip			= servermet.ReadUInt32();
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
			
			
			if ( !theApp.AddServer(newserver) ) {
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
    
		if (!merge) {
			AddLogLineM(true, wxString::Format(_("%i servers in server.met found"),fservercount));
		} else {
			AddLogLineM(true, wxString::Format(_("%d servers added"), iAddCount));
		}
	} catch (const CInvalidPacket& e) {
		AddLogLineM(true,_("Error: the file server.met is corrupted"));
		Notify_ServerThaw();
		return false;
	}
	
	return true;
}


bool CServerList::AddServer(CServer* in_server, bool fromUser)
{
	if ( !in_server->GetPort() ) {
		if ( fromUser ) {
			AddLogLineM( true,
				CFormat( _("Server not added: [%s:%d] does not specify a valid port.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}

		return false;
	} else if (
				!in_server->HasDynIP() &&
				(
					!IsGoodIP( in_server->GetIP(), thePrefs::FilterLanIPs() ) ||
					theApp.ipfilter->IsFiltered( in_server->GetIP() )
				)
	          ) {
		if ( fromUser ) {
			AddLogLineM( true,
				CFormat( _("Server not added: The IP of [%s:%d] is filtered or invalid.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}
	
		return false;
	}
	
	CServer* test_server = GetServerByAddress(in_server->GetAddress(), in_server->GetPort());

	if (test_server) {
		if ( fromUser ) {
			AddLogLineM( true,
				CFormat( _("Server not added: Server with matching IP:Port [%s:%d] found in list.") )
					% in_server->GetAddress()
					% in_server->GetPort()
			);
		}
		
		test_server->ResetFailedCount();
		Notify_ServerRefresh( test_server );
		
		return false;
	}
	
	m_servers.push_back(in_server);
	NotifyObservers( EventType( EventType::INSERTED, in_server ) );

	if ( fromUser ) {
		AddLogLineM( true,
			CFormat( _("Server added: Server at [%s:%d] using the name '%s'.") )
				% in_server->GetAddress()
				% in_server->GetPort()
				% in_server->GetListName()
		);
	}

	
	return true;
}


void CServerList::ServerStats()
{
	uint32 temp;
	temp = (uint32)time(NULL);
	
	if(theApp.IsConnectedED2K() && m_servers.size() > 0) {
		CServer* ping_server = GetNextStatServer();
		CServer* test = ping_server;
		if(!ping_server) {
			return;
		}
		while(ping_server->GetLastPinged() != 0 && (::GetTickCount() - ping_server->GetLastPinged()) < UDPSERVSTATREASKTIME) {
			ping_server = GetNextStatServer();
			if(ping_server == test) {
				return;
			}
		}
		if(ping_server->GetFailedCount() >= thePrefs::GetDeadserverRetries() && thePrefs::DeadServer() && !ping_server->IsStaticMember()) {
			Notify_ServerRemove(ping_server);
			return;
		}
		CPacket* packet = new CPacket(OP_GLOBSERVSTATREQ, 4);
		srand((unsigned)time(NULL));
		uint32 time = 0x55AA0000 + (uint16)rand();
		ping_server->SetChallenge(time);
		packet->CopyUInt32ToDataBuffer(time);
		ping_server->SetLastPinged(::GetTickCount());
		//ping_server->SetLastPingedTime(temp);
		ping_server->AddFailedCount();
		Notify_ServerRefresh(ping_server);
		theApp.statistics->AddUpDataOverheadServer(packet->GetPacketSize());
		theApp.serverconnect->SendUDPPacket(packet, ping_server, true);
		
		ping_server->SetLastDescPingedCount(false);
		if(ping_server->GetLastDescPingedCount() < 2) {
			// eserver 16.45+ supports a new OP_SERVER_DESC_RES answer, if the OP_SERVER_DESC_REQ contains a uint32
			// challenge, the server returns additional info with OP_SERVER_DESC_RES. To properly distinguish the
			// old and new OP_SERVER_DESC_RES answer, the challenge has to be selected carefully. The first 2 bytes 
			// of the challenge (in network byte order) MUST NOT be a valid string-len-int16!
			uint32 randomness = 1 + (int) (((float)(0xFFFF))*rand()/(RAND_MAX+1.0));
			uint32 uDescReqChallenge = ((uint32)randomness << 16) + INV_SERV_DESC_LEN; // 0xF0FF = an 'invalid' string length.
			packet = new CPacket( OP_SERVER_DESC_REQ,4);
			packet->CopyUInt32ToDataBuffer(uDescReqChallenge);
			theApp.statistics->AddUpDataOverheadServer(packet->GetPacketSize());
			theApp.serverconnect->SendUDPPacket(packet, ping_server, true);
		} else {
			ping_server->SetLastDescPingedCount(true);
		}
	}
}


void CServerList::RemoveServer(CServer* in_server)
{
	if (in_server == theApp.serverconnect->GetCurrentServer()) {
		theApp.ShowAlert(_("You are connected to the server you are trying to delete. please disconnect first."), _("Info"), wxOK);	
	} else {
		CInternalList::iterator it = std::find(m_servers.begin(), m_servers.end(), in_server);
		if ( it != m_servers.end() ) {
			if (theApp.downloadqueue->GetUDPServer() == in_server) {
				theApp.downloadqueue->SetUDPServer( 0 );
			}	
			
			NotifyObservers( EventType( EventType::REMOVED, in_server ) );
		
			if (m_serverpos == it) {
				++m_serverpos;
			}
			if (m_statserverpos == it) {
				++m_statserverpos;
			}
			m_servers.erase(it);
			++delservercount;
			delete in_server;
		}
	}
}


void CServerList::RemoveAllServers()
{
	NotifyObservers( EventType( EventType::CLEARED ) );
	
	delservercount += m_servers.size();
	// no connection, safely remove all servers
	while ( !m_servers.empty() ) {
		delete m_servers.back();
		m_servers.pop_back();
	}
	m_serverpos = m_servers.end();
	m_statserverpos = m_servers.end();
}


void CServerList::GetStatus(uint32 &total, uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile,float &occ)
{
	total = m_servers.size();
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
	SaveServerMet();
	while ( !m_servers.empty() ) {
		delete m_servers.back();
		m_servers.pop_back();
	}
}


void CServerList::LoadStaticServers( const wxString& filename )
{
	if ( !wxFileName::FileExists( filename ) ) {
		return;
	}
	
	wxFileInputStream stream( filename );
	wxTextInputStream f(stream);

	while ( !stream.Eof() ) {
		wxString line = f.ReadLine();
		
		// Skip comments
		if ( line.GetChar(0) == '#' || line.GetChar(0) == '/') {
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
		if ( !theApp.AddServer( server ) ) {
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
				wxASSERT(0);
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


CServer* CServerList::GetNextServer()
{
	if (m_serverpos == m_servers.end()) {
		return 0;
	} else {
		wxASSERT(*m_serverpos != NULL);
		return *m_serverpos++;
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


CServer* CServerList::GetServerByAddress(const wxString& address, uint16 port)
{
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		CServer* const s = *it;
		if (port == s->GetPort() && s->GetAddress() == address) {
			return s;
		}
	}
	return NULL;
}


CServer* CServerList::GetServerByIP(uint32 nIP){
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s = *it;
		if (s->GetIP() == nIP)
			return s; 
	}
	return NULL;
}


CServer* CServerList::GetServerByIP(uint32 nIP, uint16 nPort){
	for (CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it){
        CServer* const s = *it;
		if (s->GetIP() == nIP && s->GetPort() == nPort)
			return s; 
	}
	return NULL;
}


bool CServerList::SaveServerMet()
{
	wxString newservermet = theApp.ConfigDir + wxT("server.met.new");
	
	CSafeFile servermet( newservermet, CFile::write );
	if (!servermet.IsOpened()) {
		AddLogLineM(false,_("Failed to save server.met!"));
		return false;
	}


	servermet.WriteUInt8(0xE0);
	servermet.WriteUInt32( m_servers.size() );

	
	for ( CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		const CServer* const server = *it;

		uint16 tagcount = 12;
		if ( !server->GetListName().IsEmpty() ) 			++tagcount;
		if ( !server->GetDynIP().IsEmpty() )				++tagcount;
		if ( !server->GetDescription().IsEmpty() )			++tagcount;
		if ( server->GetConnPort() != server->GetPort() )	++tagcount;		
		#if wxUSE_UNICODE
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
		if (!server->GetVersion().IsEmpty()){
			++tagcount;
		}
		#endif
		
		
		servermet.WriteUInt32(server->GetIP());
		servermet.WriteUInt16(server->GetPort());
		servermet.WriteUInt32(tagcount);
					
		if ( !server->GetListName().IsEmpty() ) {
			#if wxUSE_UNICODE
			// This is BOM to keep eMule compatibility
			CTag( ST_SERVERNAME,	server->GetListName()		).WriteTagToFile( &servermet,  utf8strOptBOM);
			#endif
			CTag( ST_SERVERNAME,	server->GetListName()		).WriteTagToFile( &servermet );
		}
		
		if ( !server->GetDynIP().IsEmpty() ) {
			#if wxUSE_UNICODE
			// This is BOM to keep eMule compatibility
			CTag( ST_DYNIP,			server->GetDynIP()			).WriteTagToFile( &servermet, utf8strOptBOM );
			#endif			
			CTag( ST_DYNIP,			server->GetDynIP()			).WriteTagToFile( &servermet );
		}
		
		if ( !server->GetDescription().IsEmpty() ) {
			#if wxUSE_UNICODE
			// This is BOM to keep eMule compatibility
			CTag( ST_DESCRIPTION,	server->GetDescription()	).WriteTagToFile( &servermet, utf8strOptBOM );
			#endif			
			CTag( ST_DESCRIPTION,	server->GetDescription()	).WriteTagToFile( &servermet );
		}
		
		if ( server->GetConnPort() != server->GetPort() ) {
			CTag( ST_AUXPORTSLIST,	server->GetAuxPortsList()	).WriteTagToFile( &servermet );
		}
		
		CTag( ST_FAIL,			server->GetFailedCount()	).WriteTagToFile( &servermet );
		CTag( ST_PREFERENCE,	server->GetPreferences()	).WriteTagToFile( &servermet );
		CTag( "users",			server->GetUsers()			).WriteTagToFile( &servermet );
		CTag( "files",			server->GetFiles()			).WriteTagToFile( &servermet );
		CTag( ST_PING,			server->GetPing()			).WriteTagToFile( &servermet );
		CTag( ST_LASTPING,		server->GetLastPinged()		).WriteTagToFile( &servermet );
		CTag( ST_MAXUSERS,		server->GetMaxUsers()		).WriteTagToFile( &servermet );
		CTag( ST_SOFTFILES,		server->GetSoftFiles()		).WriteTagToFile( &servermet );
		CTag( ST_HARDFILES,		server->GetHardFiles()		).WriteTagToFile( &servermet );
		if (!server->GetVersion().IsEmpty()){
			#if wxUSE_UNICODE			
			CTag( ST_VERSION,		server->GetVersion()		).WriteTagToFile( &servermet, utf8strOptBOM );
			#endif
			CTag( ST_VERSION,		server->GetVersion()		).WriteTagToFile( &servermet );
		}
		CTag( ST_UDPFLAGS,		server->GetUDPFlags()		).WriteTagToFile( &servermet );
		CTag( ST_LOWIDUSERS,	server->GetLowIDUsers()		).WriteTagToFile( &servermet );
	}
	
	servermet.Close();
	wxString curservermet = theApp.ConfigDir + wxT("server.met");
	wxString oldservermet = theApp.ConfigDir + wxT("server_met.old");
	
	if ( wxFileExists(oldservermet) ) {
		wxRemoveFile(oldservermet);
	}
	
	if ( wxFileExists(curservermet) ) {
		wxRenameFile(curservermet, oldservermet);
	}
	
	wxRenameFile(newservermet, curservermet);
	
	return true;
}


void CServerList::RemoveDeadServers()
{
	if ( thePrefs::DeadServer() ) {
		for ( CInternalList::const_iterator it = m_servers.begin(); it != m_servers.end(); ) {
			CServer* const cur_server = *it;
			++it;
			if ( cur_server->GetFailedCount() > thePrefs::GetDeadserverRetries() && !cur_server->IsStaticMember()) {
				RemoveServer(cur_server);
			}
		}
	}
}

void CServerList::UpdateServerMetFromURL(const wxString& strURL)
{
	if (strURL.Find(wxT("://")) == -1) {
		AddLogLineM(true, _("Invalid URL"));
		return;
	}
	URLUpdate = strURL;
	wxString strTempFilename(theApp.ConfigDir + wxT("server.met.download"));
	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURL,strTempFilename, HTTP_ServerMet);
	downloader->Create();
	downloader->Run();
}

void CServerList::DownloadFinished(uint32 result) 
{
	if(result==1) {
		wxString strTempFilename(theApp.ConfigDir + wxT("server.met.download"));
		// curl succeeded. proceed with server.met loading
		LoadServerMet(strTempFilename);
		SaveServerMet();
		// So, file is loaded and merged, and also saved
		wxRemoveFile(strTempFilename);
	} else {
		AddLogLineM(true, CFormat( _("Failed to download the server list from %s") ) % URLUpdate);
	}
}


void CServerList::AutoUpdate() 
{
	
	uint8 url_count = theApp.glob_prefs->adresses_list.GetCount();
	
	if (!url_count) {
		AddLogLineM(true, _("No serverlist address entry in 'addresses.dat' found. Please paste a valid serverlist address into this file in order to auto-update your serverlist"));
		return;
	}
	
	wxString strURLToDownload; 
	wxString strTempFilename;

	// Do current URL. Callback function will take care of the others.
	while ( current_url_index < url_count ) {
		wxString URI = theApp.glob_prefs->adresses_list[current_url_index];

		// We use wxURL to validate the URI
		if ( wxURL( URI ).GetError() == wxURL_NOERR ) {
			// Ok, got a valid URI
			URLAutoUpdate = strURLToDownload;
			strTempFilename =  theApp.ConfigDir + wxT("server_auto.met");
		
			CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURLToDownload,strTempFilename, HTTP_ServerMetAuto);
			downloader->Create();
			downloader->Run();
		
			return;
		} else {
			AddLogLineM(true, CFormat( _("Warning, invalid URL specified for auto-updating of servers: %s") ) % URI);
		}
		
		current_url_index++;
	}

	AddLogLineM(true, _("No valid server.met auto-download url on addresses.dat"));
}


void CServerList::AutoDownloadFinished(uint32 result) 
{
	
	if(result==1) {
		wxString strTempFilename(theApp.ConfigDir + wxT("server_auto.met"));
		// curl succeeded. proceed with server.met loading
		LoadServerMet(strTempFilename);
		SaveServerMet();
		// So, file is loaded and merged, and also saved
		wxRemoveFile(strTempFilename);
	} else {
		AddLogLineM(true, CFormat(_("Failed to download the server list from %s") ) % URLUpdate);
	}
	
	++current_url_index;
	

	if (current_url_index < theApp.glob_prefs->adresses_list.GetCount()) {		
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
	//Therefore the extimate here will be lower then the actual.
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
