// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "types.h"
#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */
	#include <sys/socket.h>		//
	#include <netinet/in.h>		// Needed for inet_addr, htonl
	#include <arpa/inet.h>		//
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include <wx/msgdlg.h>
#include <wx/listimpl.cpp>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/intl.h>		// Needed for _
#include <wx/filename.h>	// Needed for wxFileName

#include "ServerList.h"		// Interface declarations.
#include "ListenSocket.h"	// Needed for CListenSocket
#include "PartFile.h"		// Needed for SRV_PR_HIGH
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "sockets.h"		// Needed for CServerConnect
#include "server.h"		// Needed for CServer
#include "otherstructs.h"	// Needed for ServerMet_Struct
#include "packets.h"		// Needed for CInvalidPacket
#include "opcodes.h"		// Needed for MET_HEADER
#include "SafeFile.h"		// Needed for CSafeFile
#include "HTTPDownloadDlg.h"	// Needed for CHTTPDownloadDlg
#include "Preferences.h"	// Needed for CPreferences
#include "otherfunctions.h"	// Needed for GetTickCount
#include "amule.h"			// Needed for theApp
#include "GetTickCount.h"


//WX_DEFINE_LIST(CServerListList);

CServerList::CServerList(CPreferences* in_prefs):
list(wxKEY_NONE)
{
	servercount = 0;
	version = 0;
	serverpos = 0;
	searchserverpos = 0;
	statserverpos = 0;
	app_prefs = in_prefs;
	//udp_timer = 0;
	udp_timer.SetOwner(&theApp,TM_UDPSOCKET);
	delservercount = 0;
	m_nLastED2KServerLinkCheck = m_nLastSaved = ::GetTickCount();
}

uint8 CServerList::AutoUpdate()
{
	uint8 url_count = app_prefs->adresses_list.GetCount();
	
	if (!url_count) {
		wxMessageBox(_("No serverlist address entry in 'addresses.dat' found. Please paste a valid serverlist address into this file in order to auto-update your serverlist"),_("Unable to retrieve serverlist"),wxICON_ERROR|wxCENTRE|wxOK);
		return 0;
	}
	
	wxString strURLToDownload; 
	wxString strTempFilename;

	uint8 temp_count =0;	
	for ( uint8 i = 0; i < url_count; i++ ) {
		strURLToDownload = app_prefs->adresses_list[i]; 
		if (strURLToDownload.Find(wxT("://")) == -1) {
			AddLogLineM(true, _("Invalid URL ") + strURLToDownload);
		} else {
			strTempFilename =  theApp.ConfigDir + wxString::Format(wxT("server_auto%u.met"), temp_count);

			// lfroen - this must go to the gui side. By now, avoid to reference gui members of CamuleApp
			CHTTPDownloadDlg *dlg=new CHTTPDownloadDlg(theApp.GetTopWindow(),strURLToDownload,strTempFilename);
			int retval=dlg->ShowModal();
			if(retval==0) {
				temp_count++;		
			} else {
				AddLogLineF(true, _("Failed to download the serverlist from %s"), strURLToDownload.GetData());
			}
			delete dlg;
		}
	}

	wxASSERT(temp_count <= url_count);
	if (temp_count < url_count) {
		AddLogLineF(true, _("%u auto-update serverlist entries failed loading"), (url_count - temp_count));
	}
	
	return temp_count;
}

bool CServerList::Init()
{
	uint8 auto_count = false;
	// auto update the list by using an url
	if (app_prefs->AutoServerlist()) {
		auto_count=AutoUpdate();
	}
	// Load Metfile
	wxString strTempFilename;
	printf("*** reading servers\n");
	strTempFilename = theApp.ConfigDir + wxT("server.met");
	bool bRes = AddServermetToList(strTempFilename, false);
	if(theApp.glob_prefs->AutoServerlist()) {
		for (uint8 i=1; i<=auto_count;i++) {
			strTempFilename =  theApp.ConfigDir + wxString::Format(wxT("server_auto%u.met"), i);
			bool bRes2 = AddServermetToList(strTempFilename);
			if( !bRes && bRes2 )
				bRes = true;
		}
	}
	// insert static servers from textfile
	strTempFilename=  theApp.ConfigDir + wxT("staticservers.dat");
	AddServersFromTextFile(strTempFilename);
	return bRes;
}

bool CServerList::AddServermetToList(const wxString& strFile, bool merge)
{
	/*
	  lfroen - this checked in CamuleApp::NotifyEvent if needed
	if (!theApp.amuledlg || !theApp.amuledlg->serverwnd || !theApp.amuledlg->serverwnd->serverlistctrl) {
		return false;
	}
	*/
	
	if (!merge) {
		Notify_ServerRemoveAll();
		this->RemoveAllServers();
	}
	CSafeFile servermet;
	if(!wxFileExists(strFile)) {
		AddLogLineM(false, _("Failed to load server.met!"));
		return false;
	}

	if (!servermet.Open(strFile,CFile::read)){ //CFile::modeRead|CFile::osSequentialScan)) {
		AddLogLineM(false, _("Failed to load server.met!"));
		return false;
	}

	if ( (1 != servermet.Read(&version,1)) || (version != 0xE0 && version != MET_HEADER)) {
		servermet.Close();
		AddLogLineF(false,_("Invalid versiontag in server.met (0x%x , size %i)!"),version, sizeof(version));
		return false;
	}

	try {
		uint32 fservercount;
		if (4 != servermet.Read(&fservercount,4)) {
			throw CInvalidPacket();
		}

		ENDIAN_SWAP_I_32(fservercount);			

		ServerMet_Struct sbuffer;
		uint32 iAddCount = 0;
		Notify_ServerFreeze();

		for (uint32 j = 0;j < fservercount;j++) {
			// get server
			if (sizeof(ServerMet_Struct) != servermet.Read(&sbuffer,sizeof(ServerMet_Struct))) {
				throw CInvalidPacket();
			}
			// ENDIAN_SWAP_I_32(sbuffer.ip);	there is a ntoa somewhere
			ENDIAN_SWAP_I_16(sbuffer.port);			
			ENDIAN_SWAP_I_32(sbuffer.tagcount);
			
			CServer* newserver = new CServer(&sbuffer);

			//add tags
			for (uint32 i=0;i < sbuffer.tagcount;i++) {
				newserver->AddTagFromFile(&servermet);
			}
			// set listname for server
			if (newserver->GetListName().IsEmpty()) {
				newserver->SetListName(wxT("Server ") +newserver->GetAddress());
			}
			if (!theApp.AddServer(newserver)) {
				CServer* update = theApp.serverlist->GetServerByAddress(newserver->GetAddress(), newserver->GetPort());
				if(update) {
					update->SetListName( newserver->GetListName());
					if(!newserver->GetDescription().IsEmpty()) {
						update->SetDescription( newserver->GetDescription());
					}
					Notify_ServerRefresh(update);
				}
				delete newserver;
			} else {
				iAddCount++;
			}

			// don't yield all the time
			// Kry - What's this supposed to do?
			/*
			if(j%75==0) {
				theApp.Yield();
			}
			*/
		}
		Notify_ServerThaw();
    
		if (!merge) {
			AddLogLineF(true,_("%i servers in server.met found"),fservercount);
		} else {
			AddLogLineF(true,_("%d servers added"), iAddCount, fservercount-iAddCount);
		}
	}
	catch (CInvalidPacket) {
		AddLogLineM(true,_("Error: the file server.met is corrupted"));
		servermet.Close();
		Notify_ServerThaw();
		return false;
	}
	servermet.Close();
	return true;
}

bool CServerList::AddServer(CServer* in_server)
{
	if (theApp.glob_prefs && theApp.glob_prefs->FilterBadIPs()) {
		if ( !IsGoodServerIP( in_server )) {
			return false;
		}
	}
	CServer* test_server = GetServerByAddress(in_server->GetAddress(), in_server->GetPort());
	// lfroen - it's ok, gui status checked in Notify
	// if (test_server && theApp.amuledlg) {
	if (test_server) {
		test_server->ResetFailedCount();
		Notify_ServerRefresh( test_server );
		return false;
	}
	list.AddTail(in_server); //AddTail(in_server);
	return true;
}

void CServerList::ServerStats()
{
	uint32 temp;
	temp = (uint32)time(NULL);
	
	if(theApp.serverconnect->IsConnected() && list.GetCount() > 0) {
		CServer* ping_server = GetNextStatServer();
		CServer* test = ping_server;
		if(!ping_server) {
			return;
		}
	        while(ping_server->GetLastPinged() != 0 && (::GetTickCount() - ping_server->GetLastPinged()) < UDPSERVSTATREASKTIME) {
			ping_server = this->GetNextStatServer();
			if(ping_server == test) {
				return;
			}
		}
		if(ping_server->GetFailedCount() >= theApp.glob_prefs->GetDeadserverRetries() && theApp.glob_prefs->DeadServer()) {
			Notify_ServerRemove(ping_server);
			return;
		}
		Packet* packet = new Packet(OP_GLOBSERVSTATREQ, 4);
		srand((unsigned)time(NULL));
		uint32 time = 0x55AA0000 + (uint16)rand();
		ping_server->SetChallenge(time);
		packet->CopyToDataBuffer(0, (const char *)&time, 4);
		ping_server->SetLastPinged(::GetTickCount());
		//ping_server->SetLastPingedTime(temp);
		ping_server->AddFailedCount();
		Notify_ServerRefresh(ping_server);
		theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
		theApp.serverconnect->SendUDPPacket(packet, ping_server, true);
		
		ping_server->SetLastDescPingedCount(false);
		if(ping_server->GetLastDescPingedCount() < 2) {
			// eserver 16.45+ supports a new OP_SERVER_DESC_RES answer, if the OP_SERVER_DESC_REQ contains a uint32
			// challenge, the server returns additional info with OP_SERVER_DESC_RES. To properly distinguish the
			// old and new OP_SERVER_DESC_RES answer, the challenge has to be selected carefully. The first 2 bytes 
			// of the challenge (in network byte order) MUST NOT be a valid string-len-int16!
			uint32 randomness = 1 + (int) (((float)(0xFFFF))*rand()/(RAND_MAX+1.0));
			uint32 uDescReqChallenge = ((uint32)randomness << 16) + INV_SERV_DESC_LEN; // 0xF0FF = an 'invalid' string length.
			packet = new Packet( OP_SERVER_DESC_REQ,4);
			packet->CopyUInt32ToDataBuffer(ENDIAN_SWAP_32(uDescReqChallenge));
			theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
			theApp.serverconnect->SendUDPPacket(packet, ping_server, true);
		} else {
			ping_server->SetLastDescPingedCount(true);
		}
	}
}

//	Filter Servers with invalid IP's and Port.
//
//	Here is reserved blocks from RFC 3330 at http://www.rfc-editor.org/rfc/rfc3330.txt
//
//	Address Block             Present Use                       Reference
//	---------------------------------------------------------------------
static char const* const filtered_blocks [] = {
	"0.0.0.0/8",       // "This" Network                 [RFC1700, page 4]
	"10.0.0.0/8",      // Private-Use Networks                   [RFC1918]
	"14.0.0.0/8",      // Public-Data Networks         [RFC1700, page 181]
	"24.0.0.0/8",      // Cable Television Networks                    --
	"39.0.0.0/8",      // Reserved but subject
	                   //    to allocation                       [RFC1797]
	"127.0.0.0/8",     // Loopback                       [RFC1700, page 5]
	"128.0.0.0/16",    // Reserved but subject
	                   //    to allocation                             --
	"169.254.0.0/16",  // Link Local                                   --
	"172.16.0.0/12",   // Private-Use Networks                   [RFC1918]
	"191.255.0.0/16",  // Reserved but subject
	                   //    to allocation                             --
	"192.0.0.0/24",    // Reserved but subject
	                   //    to allocation                             --
	"192.0.2.0/24",    // Test-Net
	"192.88.99.0/24",  // 6to4 Relay Anycast                     [RFC3068]
	"192.168.0.0/16",  // Private-Use Networks                   [RFC1918]
	"198.18.0.0/15",   // Network Interconnect
	                   //    Device Benchmark Testing            [RFC2544]
	"223.255.255.0/24",// Reserved but subject
	                   //    to allocation                             --
	"224.0.0.0/4",     // Multicast                              [RFC3171]
	"240.0.0.0/4"      // Reserved for Future Use        [RFC1700, page 4]
};

struct filter_st {
	in_addr_t addr;		// Address and mask in network order.
	in_addr_t mask;
};

int const number_of_filtered_blocks = sizeof(filtered_blocks) / sizeof(char const*);
static filter_st filters[number_of_filtered_blocks];

bool CServerList::IsGoodServerIP(CServer* in_server)
{
	static bool initialized = false;
	if (!initialized) {
		for (int i = 0; i < number_of_filtered_blocks; ++i) {
			char* ipmask = strdup(filtered_blocks[i]);
			char* addr = strtok(ipmask, "/");
			char* n = strtok(NULL, "/");
			filters[i].addr = inet_addr(addr);
			filters[i].mask = htonl((1 << (32 - atoi(n))) - 1);
			free(ipmask);
		}
		initialized = true;
	}
	if (in_server->HasDynIP()) {
		return true;
	}
	in_addr_t ip = in_server->GetIP();	// Also network order.
	for (int i = 0; i < number_of_filtered_blocks; ++i) {
		if (((ip ^ filters[i].addr) & ~(filters[i].mask)) == 0) {
			return false;
		}
	}
	return true;
}

void CServerList::RemoveServer(CServer* out_server)
{
	if (out_server == theApp.serverconnect->GetCurrentServer()) {
			wxMessageBox(wxT("You are connected to the server you are trying to delete. please disconnect first."), wxT("Info"), wxOK);	
	} else {
	
		POSITION pos = list.Find( out_server );
		if ( pos != NULL ) {
			if (theApp.downloadqueue->cur_udpserver == out_server) {
				theApp.downloadqueue->cur_udpserver = 0;
			}	
			list.RemoveAt(pos);
			delservercount++;
			delete out_server;
			return;
		}
	}
}

void CServerList::RemoveAllServers()
{
	delservercount += list.GetSize();
	// no connection, safely remove all servers
	while ( !list.IsEmpty() ) {
		delete list.GetTail();
		list.RemoveTail();
	}
}

void CServerList::GetStatus(uint32 &total, uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile,float &occ)
{
	total = list.GetCount();
	failed = 0;
	user = 0;
	file = 0;
	tuser=0;
	tfile = 0;
	occ=0;
	uint32 maxusers=0;
	uint32 tuserk = 0;

	CServer* curr;
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ) {
		curr = (CServer*)list.GetNext(pos);
		if( curr->GetFailedCount() ) {
			failed++;
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
	CServer* curr;
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ) {
		curr = (CServer*)list.GetNext(pos);
		if( !curr->GetFailedCount() ) {
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
	}
}

CServerList::~CServerList()
{
	SaveServermetToFile();
	while ( !list.IsEmpty() ) {
		delete list.GetTail();
		list.RemoveTail();
	}
	udp_timer.Stop();
}

void CServerList::AddServersFromTextFile(wxString strFilename,bool isstaticserver, bool writetolog)
{
	// emanuelw(20030731) added writetolog
	wxString strLine;
	//CStdioFile f;
	//wxFFile f;
	//if (!f.Open(strFilename, CFile::modeRead | CFile::typeText))
	if(!wxFileName::FileExists(strFilename)) {
		// no file. do nothing.
		return;
	}
	wxFileInputStream stream(strFilename);
	if(!stream.Ok()) {
		return;
	}

	wxTextInputStream f(stream);

	while(strLine=f.ReadLine()) {
		if(stream.Eof()) {
			break; // stop iteration if end of file is met..
		}
		// (while won't do it for us)
		// format is host:port,Name
		if (strLine.Length() < 5) {
			continue;
		}
		if (strLine.GetChar(0) == '#' || strLine.GetChar(0) == '/') {
			continue;
		}

		// fetch host
		int pos = strLine.Find(wxT(":"));
		if (pos == -1) {
			pos = strLine.Find(wxT(","));
			if (pos == -1) {
				continue;
			}
		}
		wxString strHost = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);
		// fetch  port
		pos = strLine.Find(wxT(","));
		if (pos == -1) {
			continue;
		}
		wxString strPort = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);

		// Barry - fetch priority
		pos = strLine.Find(wxT(","));
		int priority = SRV_PR_HIGH;
		if (pos == 1) {
			wxString strPriority = strLine.Left(pos);
			try {
				priority = atoi(unicode2char(strPriority));
				if ((priority < 0) || (priority > 2)) {
					priority = SRV_PR_HIGH;
				}
			} catch (...) {
			}
			strLine = strLine.Mid(pos+1);
		}

		// fetch name
		wxString strName = strLine;
		strName.Replace(wxT("\r"),wxT( ""));
		strName.Replace(wxT("\n"),wxT( ""));

		// emanuelw(20030924) fix: if there is no name the ip is set as name
		if(wxStrlen(strName) == 0) {
			strName = strHost;
		}

		// create server object and add it to the list
		CServer* nsrv = new CServer(atoi(unicode2char(strPort)), strHost);
		nsrv->SetListName(strName);

		// emanuelw(20030924) fix: isstaticserver now is used! before it was always true
		nsrv->SetIsStaticMember(isstaticserver);

		// Barry - Was always high
		nsrv->SetPreference(priority);

		// emanuelw(20030924) added: create log entry
		if(writetolog == true) {
			AddLogLineM(true,wxString(_("Server added: "))+nsrv->GetAddress()); 
		}

		if (!theApp.AddServer(nsrv))	{
			delete nsrv;
			CServer* srvexisting = GetServerByAddress(strHost, atoi(unicode2char(strPort)));
			if (srvexisting) {
				srvexisting->SetListName(strName);
				srvexisting->SetIsStaticMember(true);
				// Barry - Was always high
				srvexisting->SetPreference(priority); 
				Notify_ServerRefresh(srvexisting);
			}
		}
	}
}

void CServerList::MoveServerDown(CServer* aServer)
{
	POSITION pos = list.Find( aServer );
	if ( pos != NULL ) {
		list.AddTail( aServer );
		list.RemoveAt(pos);
	}
}

void CServerList::Sort()
{
	POSITION pos1, pos2;
	uint16 i = 0;
	for( pos1 = list.GetHeadPosition(); (pos2 = pos1 ) != NULL;) {
		list.GetNext(pos1);
		CServer* cur_server = list.GetAt(pos2);
		if (cur_server->GetPreferences()== PR_HIGH) {
			list.AddHead(cur_server);
			list.RemoveAt(pos2);
		} else if (cur_server->GetPreferences() == PR_LOW) {
			list.AddTail(cur_server);
			list.RemoveAt(pos2);
		}
		i++;
		if (i == list.GetCount()) {
			break;
		}
	}
}

CServer* CServerList::GetNextServer()
{
	CServer* nextserver = 0;
	uint32 i = 0;
	if (serverpos>=((uint32)list.GetCount())) {
		return 0;
	}
	while (!nextserver && i != (uint32)list.GetCount()) {
		POSITION posIndex = list.FindIndex(serverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			serverpos= 0; // <<--9/27/02 zg
		}

		nextserver = list.GetAt(posIndex);
		serverpos++;
		i++;
		// TODO: Add more option to filter bad server like min ping, min users etc
		//if (nextserver->preferences = ?)
		//	nextserver = 0;
		//if (serverpos == list.GetCount()) return 0;//	serverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetNextSearchServer()
{
	CServer* nextserver = 0;
	uint32 i = 0;
	while (!nextserver && i != (uint32)list.GetCount()) {
		POSITION posIndex = list.FindIndex(searchserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			searchserverpos=0;
		}
		nextserver = list.GetAt(posIndex);
		searchserverpos++;
		i++;
		if (searchserverpos == (uint32)list.GetCount()) {
			searchserverpos = 0;
		}
	}
	return nextserver;
}

CServer* CServerList::GetNextStatServer()
{
	CServer* nextserver = 0;
	uint32 i = 0;
	while (!nextserver && i != (uint32)list.GetCount()) {
		POSITION posIndex = list.FindIndex(statserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			statserverpos=0;
		}

		nextserver = list.GetAt(posIndex);
		statserverpos++;
		i++;
		if (statserverpos == (uint32)list.GetCount()) {
			statserverpos = 0;
		}
	}
	return nextserver;
}

bool CServerList::BroadCastPacket(Packet* packet)
{
	// unused atm . but might be useful later
	if (udp_timer.IsRunning()) {
		return false;
	}
	//udp_timer = SetTimer(0,4322,UDPSEARCHSPEED,CServerList::UDPTimerProc);
	udp_timer.Start(UDPSEARCHSPEED);
	broadcastpos = list.GetHeadPosition();
	broadcastpacket = packet;
	return true;
}

void CServerList::SendNextPacket()
{
	if (theApp.listensocket->TooManySockets()) {
		//KillTimer(0,udp_timer);
		udp_timer.Stop();
		delete broadcastpacket;
		return;
	}

	if (broadcastpos != 0) {
		CServer* cur_server = list.GetAt(broadcastpos);
		if (cur_server != theApp.serverconnect->GetCurrentServer()) {
			theApp.serverconnect->SendUDPPacket(broadcastpacket,cur_server,false);
		}
		list.GetNext(broadcastpos);
	} else {
		// KillTimer(0,udp_timer);
		udp_timer.Stop();
		delete broadcastpacket;
	}
}

void CServerList::CancelUDPBroadcast()
{
	if (udp_timer.IsRunning()) {
		// KillTimer(0,udp_timer);
		// udp_timer = 0;
		udp_timer.Stop();
		delete broadcastpacket;
	}
}

#if 0
void CALLBACK CServerList::UDPTimerProc(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	theApp.serverlist->SendNextPacket();
}
#endif

CServer* CServerList::GetNextServer(CServer* lastserver)
{
	if (list.IsEmpty()) {
		return 0;
	}
	if (!lastserver) {
		return list.GetHead();
	}

	POSITION pos = list.Find(lastserver);
	if (!pos) {
		return list.GetHead();
	}
	list.GetNext(pos);
	if (!pos) {
		return NULL;
	} else {
		return list.GetAt(pos);
	}
}

CServer* CServerList::GetServerByAddress(const wxString& address, uint16 port)
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;) {
		CServer *s = list.GetNext(pos); // i_a: small speed optimization
		if (port == s->GetPort() && s->GetAddress() == address) {
			return s;
		}
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP){
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP)
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP, uint16 nPort){
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP && s->GetPort() == nPort)
			return s; 
	}
	return NULL;
}

bool CServerList::SaveServermetToFile()
{
	m_nLastSaved=::GetTickCount(); // oops.. don't save ALL the time :)

	wxString newservermet(theApp.ConfigDir + wxT("server.met.new"));
	CFile servermet;
	servermet.Open(newservermet, CFile::write);
	if (!servermet.IsOpened()) {
		AddLogLineM(false,_("Failed to save server.met!"));
		return false;
	}

	version = 0xE0;
	servermet.Write(&version, 1);
	uint32 fservercount = ENDIAN_SWAP_32(list.GetCount());
	servermet.Write(&fservercount,4);
	ServerMet_Struct sbuffer;
	CServer* nextserver;

	fservercount = list.GetCount(); // fservercount was modified above by ENDIAN_SWAP_I_32 !
	
		for (uint32 j = 0;j != fservercount;j++){
			nextserver = this->GetServerAt(j);
			// Somehow, internally we store it the wrong way.
			//sbuffer.ip = ENDIAN_SWAP_32(nextserver->GetIP());
			sbuffer.ip = nextserver->GetIP();
			sbuffer.port = ENDIAN_SWAP_16(nextserver->GetPort());
			uint16 tagcount = 12;
			if (!nextserver->GetListName().IsEmpty()) 
				tagcount++;
			if (!nextserver->GetDynIP().IsEmpty())
				tagcount++;
			if (!nextserver->GetDescription().IsEmpty())
				tagcount++;
			sbuffer.tagcount = ENDIAN_SWAP_32(tagcount);
			servermet.Write(&sbuffer, sizeof(ServerMet_Struct));	
						
			if (!nextserver->GetListName().IsEmpty()) {
				CTag servername( ST_SERVERNAME, nextserver->GetListName() );
				servername.WriteTagToFile(&servermet);
			}
			if (!nextserver->GetDynIP().IsEmpty()) {
				CTag serverdynip( ST_DYNIP, nextserver->GetDynIP() );
				serverdynip.WriteTagToFile(&servermet);
			}
			if (!nextserver->GetDescription().IsEmpty()) {
				CTag serverdesc( ST_DESCRIPTION, nextserver->GetDescription() );
				serverdesc.WriteTagToFile(&servermet);
			}
			CTag serverfail(ST_FAIL, nextserver->GetFailedCount() );
			serverfail.WriteTagToFile(&servermet);
			CTag serverpref( ST_PREFERENCE, nextserver->GetPreferences() );
			serverpref.WriteTagToFile(&servermet);
			CTag serveruser("users", nextserver->GetUsers() );
			serveruser.WriteTagToFile(&servermet);
			CTag serverfiles("files", nextserver->GetFiles() );
			serverfiles.WriteTagToFile(&servermet);
			CTag serverping(ST_PING, nextserver->GetPing() );
			serverping.WriteTagToFile(&servermet);
			CTag serverlastp(ST_LASTPING, nextserver->GetLastPinged() );
			serverlastp.WriteTagToFile(&servermet);
			CTag servermaxusers(ST_MAXUSERS, nextserver->GetMaxUsers() );
			servermaxusers.WriteTagToFile(&servermet);
			CTag softfiles(ST_SOFTFILES, nextserver->GetSoftFiles() );
			softfiles.WriteTagToFile(&servermet);
			CTag hardfiles(ST_HARDFILES, nextserver->GetHardFiles() );
			hardfiles.WriteTagToFile(&servermet);
			CTag version(ST_VERSION, nextserver->GetVersion());
			version.WriteTagToFile(&servermet);
			CTag tagUDPFlags(ST_UDPFLAGS, nextserver->GetUDPFlags() );
			tagUDPFlags.WriteTagToFile(&servermet);
			CTag tagLowIDUsers(ST_LOWIDUSERS, nextserver->GetLowIDUsers() );
			tagLowIDUsers.WriteTagToFile(&servermet);			
	}
	
	servermet.Flush();
	servermet.Close();
	wxString curservermet(theApp.ConfigDir + wxT("server.met"));
	wxString oldservermet(theApp.ConfigDir + wxT("server_met.old"));
	if ( wxFileExists(oldservermet) ) {
		wxRemoveFile(oldservermet);
	}
	if ( wxFileExists(curservermet) ) {
		wxRenameFile(curservermet,oldservermet);
	}
	wxRenameFile(newservermet,curservermet);
	return true;
}


void CServerList::Process()
{
	/* We save anyway the serverlist at exit, why saving every 2mins ???
	   and if client crash, rm ~/.aMule/server* do the trick ...
	if (::GetTickCount() - m_nLastSaved > 180000) {
		this->SaveServermetToFile();
	}
	*/
	
	// emanuelw(20030924) added:Check for new server links once per second.	
	if ((::GetTickCount() - m_nLastED2KServerLinkCheck) >= 1000) {
		wxString filename(theApp.ConfigDir + wxT("ED2KServers"));
	
		if (wxFile::Exists(filename)) {
			AddServersFromTextFile(filename, false, true);
			this->SaveServermetToFile();
			wxRemoveFile(filename);
		}
		m_nLastED2KServerLinkCheck = ::GetTickCount();
	}
}
