//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "ClientTCPSocket.h"	// Interface declarations

#include <vector>

#include <wx/dynarray.h>	// Needed for wxArray
#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/tokenzr.h> 	// Needed for wxStringTokenizer

#include "Proxy.h"		// Needed for CProxyData
#include "Preferences.h"	// Needed for thePrefs
#include "OPCodes.h"		// Needed for CONNECTION_TIMEOUT
#include "Packet.h"		// Needed for CPacket
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"		// Neeed for logRemoteClient
#include "updownclient.h"	// Needed for CUpDownClient
#include <common/Format.h>		// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "ClientList.h"		// Needed for CClientList
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ClientUDPSocket.h" // Needed for CClientUDPSocket
#include "PartFile.h"		// Needed for CPartFile
#include "MemFile.h"		// Needed for CMemFile
#include "kademlia/kademlia/Kademlia.h" // Needed for CKademlia::Kademlia
#include "kademlia/kademlia/Prefs.h"	// Needed for CKademlia::CPrefs
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "IPFilter.h"		// Needed for CIPFilter
#include "ListenSocket.h"	// Needed for CListenSocket

//#define __PACKET_RECV_DUMP__

IMPLEMENT_DYNAMIC_CLASS(CClientTCPSocket,CEMSocket)

//------------------------------------------------------------------------------
// CClientTCPSocketHandler
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// CClientTCPSocketHandler
//------------------------------------------------------------------------------

class CClientTCPSocketHandler: public wxEvtHandler
{
public:
	CClientTCPSocketHandler() {};

private:
	void ClientTCPSocketHandler(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CClientTCPSocketHandler, wxEvtHandler)
	EVT_SOCKET(CLIENTTCPSOCKET_HANDLER, CClientTCPSocketHandler::ClientTCPSocketHandler)
END_EVENT_TABLE()

void CClientTCPSocketHandler::ClientTCPSocketHandler(wxSocketEvent& event)
{
	CClientTCPSocket *socket = dynamic_cast<CClientTCPSocket *>(event.GetSocket());
	wxASSERT(socket);
	if (!socket) {
		return;
	}
	
	if (socket->OnDestroy() || socket->deletethis) {
		return;
	}
	
	switch(event.GetSocketEvent()) {
		case wxSOCKET_LOST:
			socket->OnError(0xFEFF /* SOCKET_LOST is not an error */);
			break;
		case wxSOCKET_INPUT:
			socket->OnReceive(0);
			break;
		case wxSOCKET_OUTPUT:
			socket->OnSend(0);
			break;
		case wxSOCKET_CONNECTION:
			// connection stablished, nothing to do about it?
			socket->OnConnect(socket->Error() ? socket->LastError() : 0);
			break;
		default:
			// Nothing should arrive here...
			wxASSERT(0);
			break;
	}
}

//
// There can be only one. :)
//
static CClientTCPSocketHandler g_clientReqSocketHandler;


//------------------------------------------------------------------------------
// CClientTCPSocket
//------------------------------------------------------------------------------

WX_DEFINE_OBJARRAY(ArrayOfwxStrings)

CClientTCPSocket::CClientTCPSocket(CUpDownClient* in_client, const CProxyData *ProxyData)
	: CEMSocket(ProxyData)
{
	SetClient(in_client);
	ResetTimeOutTimer();
	deletethis = false;

	SetEventHandler(g_clientReqSocketHandler, CLIENTTCPSOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);

	theApp.listensocket->AddSocket(this);
	theApp.listensocket->AddConnection();
}


CClientTCPSocket::~CClientTCPSocket()
{
	// remove event handler
	SetNotify(0);
	Notify(false);

	if (m_client) {
		m_client->SetSocket( NULL );
	}
	m_client = NULL;

	if (theApp.listensocket && !theApp.listensocket->OnShutdown()) {
		theApp.listensocket->RemoveSocket(this);
	}
}


void CClientTCPSocket::ResetTimeOutTimer()
{
	timeout_timer = ::GetTickCount();
}


bool CClientTCPSocket::CheckTimeOut()
{
	// 0.42x
	uint32 uTimeout = GetTimeOut();
	if (m_client) {

		if (m_client->GetKadState() == KS_CONNECTED_BUDDY) {
			//We originally ignored the timeout here for buddies.
			//This was a stupid idea on my part. There is now a ping/pong system
			//for buddies. This ping/pong system now prevents timeouts.
			//This release will allow lowID clients with KadVersion 0 to remain connected.
			//But a soon future version needs to allow these older clients to time out to prevent dead connections from continuing.
			//JOHNTODO: Don't forget to remove backward support in a future release.
			if ( m_client->GetKadVersion() == 0 ) {
				return false;
			}
			
			uTimeout += MIN2MS(15);		
		}
		
		if (m_client->GetChatState() != MS_NONE) {
			uTimeout += CONNECTION_TIMEOUT;		
		}
	}
	
	if (::GetTickCount() - timeout_timer > uTimeout){
		timeout_timer = ::GetTickCount();
		Disconnect(wxT("Timeout"));
		return true;
	}
	
	return false;	
}


void CClientTCPSocket::SetClient(CUpDownClient* pClient)
{
	m_client = pClient;
	if (m_client) {
		m_client->SetSocket( this );
	}
}


void CClientTCPSocket::OnClose(int nErrorCode)
{
	// 0.42x
	wxASSERT(theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);
	if (nErrorCode) {
		Disconnect(wxString::Format(wxT("Closed: %u"), nErrorCode));
	} else {
		Disconnect(wxT("Close"));
	}
}


void CClientTCPSocket::Disconnect(const wxString& strReason)
{
	byConnected = ES_DISCONNECTED;
	if (m_client) {
		if (m_client->Disconnected(strReason, true)) {
			// Somehow, Safe_Delete() is beeing called by Disconnected(),
			// or any other function that sets m_client to NULL,
			// so we must check m_client first.
			if (m_client) {
				m_client->SetSocket( NULL );
				m_client->Safe_Delete();
			}
		} 
		m_client = NULL;
	}
	
	Safe_Delete();
}


void CClientTCPSocket::Safe_Delete()
{
	if ( !deletethis && !OnDestroy() ) {
		// Paranoia is back.
		SetNotify(0);
		Notify(false);
		// lfroen: first of all - stop handler
		deletethis = true;

		if (m_client) {
			m_client->SetSocket( NULL );
			m_client = NULL;
		}
		byConnected = ES_DISCONNECTED;
		Close(); // Destroy is suposed to call Close(), but.. it doesn't hurt.
		Destroy();
	}
}


bool CClientTCPSocket::ProcessPacket(const byte* buffer, uint32 size, uint8 opcode)
{
	#ifdef __PACKET_RECV_DUMP__
	printf("Rec: OPCODE %x \n",opcode);
	DumpMem(buffer, size);
	#endif
	if (!m_client && opcode != OP_HELLO) {
		throw wxString(wxT("Asks for something without saying hello"));
	} else if (m_client && opcode != OP_HELLO && opcode != OP_HELLOANSWER)
		m_client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode);
	
	switch(opcode) {
		case OP_HELLOANSWER: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HELLOANSWER from ") + m_client->GetFullIP());
			theStats::AddDownOverheadOther(size);
			m_client->ProcessHelloAnswer(buffer, size);

			// start secure identification, if
			//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
			//	- we have received eMule-OP_HELLOANSWER (new eMule)
			if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
				m_client->InfoPacketsReceived();
			}
			
			// Socket might die because of sending in InfoPacketsReceived, so check
			if (m_client) {
				m_client->ConnectionEstablished();
			}
			
			// Socket might die on ConnectionEstablished somehow. Check it.
			if (m_client) {					
				Notify_UploadCtrlRefreshClient( m_client );
			}
			
			break;
		}
		case OP_HELLO: {	// 0.43b
							
			theStats::AddDownOverheadOther(size);
			bool bNewClient = !m_client;				
			if (bNewClient) {
				// create new client to save standart informations
				m_client = new CUpDownClient(this);
			}
			
			// Do not move up!
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HELLO from ") + m_client->GetFullIP() );
			
			bool bIsMuleHello = false;
			
			try{
				bIsMuleHello = m_client->ProcessHelloPacket(buffer, size);
			} catch(...) {
				if (bNewClient && m_client) {
					// Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
					m_client->Safe_Delete();
					m_client = NULL;
				}
				throw;
			}

			// if IP is filtered, dont reply but disconnect...
			if (theApp.ipfilter->IsFiltered(m_client->GetIP())) {
				if (bNewClient) {
					m_client->Safe_Delete();
					m_client = NULL;
				}
				Disconnect(wxT("IPFilter"));
				return false;
			}
					
			wxASSERT(m_client);
			
			// now we check if we now this client already. if yes this socket will
			// be attached to the known client, the new client will be deleted
			// and the var. "client" will point to the known client.
			// if not we keep our new-constructed client ;)
			if (theApp.clientlist->AttachToAlreadyKnown(&m_client,this)) {
				// update the old client informations
				bIsMuleHello = m_client->ProcessHelloPacket(buffer, size);
			} else {
				theApp.clientlist->AddClient(m_client);
				m_client->SetCommentDirty();
			}
			Notify_UploadCtrlRefreshClient( m_client );
			// send a response packet with standart informations
			if ((m_client->GetHashType() == SO_EMULE) && !bIsMuleHello) {
				m_client->SendMuleInfoPacket(false);				
			}
			
			// Client might die from Sending in SendMuleInfoPacket, so check
			if ( m_client ) {
				m_client->SendHelloAnswer();
			}
						
			// Kry - If the other side supports it, send OS_INFO
			// Client might die from Sending in SendHelloAnswer, so check				
			if (m_client && m_client->GetOSInfoSupport()) {
				m_client->SendMuleInfoPacket(false,true); // Send the OS Info tag on the recycled Mule Info
			}				
			
			// Client might die from Sending in SendMuleInfoPacket, so check
			if ( m_client ) {
				m_client->ConnectionEstablished();
			}
			
			// start secure identification, if
			//	- we have received eMule-OP_HELLO (new eMule)				
			if (m_client && m_client->GetInfoPacketsReceived() == IP_BOTH) {
					m_client->InfoPacketsReceived();		
			}
			
			break;
		}
		case OP_REQUESTFILENAME: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTFILENAME from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			// IP banned, no answer for this request
			if (m_client->IsBanned()) {
				break;
			}
			if (size >= 16) {
				if (!m_client->GetWaitStartTime()) {
					m_client->SetWaitStartTime();
				}
				CMemFile data_in(buffer, size);
				CMD4Hash reqfilehash = data_in.ReadHash();
				CKnownFile *reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if ( reqfile == NULL ) {
					reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
					if ( !( reqfile != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
						break;
					}
				}
				// if we are downloading this file, this could be a new source
				// no passive adding of files with only one part
				if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
					if (thePrefs::GetMaxSourcePerFile() > 
						((CPartFile*)reqfile)->GetSourceCount()) {
						theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, m_client);
					}
				}

				// check to see if this is a new file they are asking for
				if (m_client->GetUploadFileID() != reqfilehash) {
						m_client->SetCommentDirty();
				}

				m_client->SetUploadFileID(reqfile);
				m_client->ProcessExtendedInfo(&data_in, reqfile);
				
				// send filename etc
				CMemFile data_out(128);
				data_out.WriteHash(reqfile->GetFileHash());
				data_out.WriteString(reqfile->GetFileName());
				CPacket* packet = new CPacket(&data_out, OP_EDONKEYPROT, OP_REQFILENAMEANSWER);
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_REQFILENAMEANSWER to ") + m_client->GetFullIP() );
				SendPacket(packet,true);

				// SendPacket might kill the socket, so check
				if (m_client)
					m_client->SendCommentInfo(reqfile);

				break;
			}
			throw wxString(wxT("Invalid OP_REQUESTFILENAME packet size"));
			break;  
		}
		case OP_SETREQFILEID: {	// 0.43b EXCEPT track of bad clients
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SETREQFILEID from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			
			if (m_client->IsBanned()) {
				break;
			}
			
			// DbT:FileRequest
			if (size == 16) {
				if (!m_client->GetWaitStartTime()) {
					m_client->SetWaitStartTime();
				}

				const CMD4Hash fileID((byte*)buffer);
				CKnownFile *reqfile = theApp.sharedfiles->GetFileByID(fileID);
				if ( reqfile == NULL ) {
					reqfile = theApp.downloadqueue->GetFileByID(fileID);
					if ( !( reqfile  != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
						CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16, OP_EDONKEYPROT);
						replypacket->Copy16ToDataBuffer(fileID.GetHash());
						theStats::AddUpOverheadFileRequest(replypacket->GetPacketSize());
						AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_FILERE to ") + m_client->GetFullIP() );
						SendPacket(replypacket, true);
						break;
					}
				}

				// check to see if this is a new file they are asking for
				if (m_client->GetUploadFileID() != fileID) {
					m_client->SetCommentDirty();
				}

				m_client->SetUploadFileID(reqfile);
				// send filestatus
				CMemFile data(16+16);
				data.WriteHash(reqfile->GetFileHash());
				if (reqfile->IsPartFile()) {
					((CPartFile*)reqfile)->WritePartStatus(&data);
				} else {
					data.WriteUInt16(0);
				}
				CPacket* packet = new CPacket(&data, OP_EDONKEYPROT, OP_FILESTATUS);
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_FILESTATUS to ") + m_client->GetFullIP() );
				SendPacket(packet, true);
				break;
			}
			throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
			break;
			// DbT:End
		}			
		
		case OP_FILEREQANSNOFIL: {	// 0.43b protocol, lacks ZZ's download manager on swap
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILEREQANSNOFIL from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			if (size == 16) {
				// if that client does not have my file maybe has another different
				CPartFile* reqfile = theApp.downloadqueue->GetFileByID(CMD4Hash((byte*)buffer));
				if ( reqfile) {
					reqfile->AddDeadSource( m_client );
				} else {
					break;
				}
					
				// we try to swap to another file ignoring no needed parts files
				switch (m_client->GetDownloadState()) {
					case DS_CONNECTED:
					case DS_ONQUEUE:
					case DS_NONEEDEDPARTS:
					if (!m_client->SwapToAnotherFile(true, true, true, NULL)) {
						theApp.downloadqueue->RemoveSource(m_client);
					}
					break;
				}
				break;
			}
			throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
			break;
		}
		
		case OP_REQFILENAMEANSWER: {	// 0.43b except check for bad clients
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQFILENAMEANSWER from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			CMemFile data(buffer, size);
			CMD4Hash hash = data.ReadHash();
			const CPartFile* file = theApp.downloadqueue->GetFileByID(hash);
			m_client->ProcessFileInfo(&data, file);
			break;
		}
		
		case OP_FILESTATUS: {		// 0.43b except check for bad clients
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILESTATUS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			CMemFile data(buffer, size);
			CMD4Hash hash = data.ReadHash();
			const CPartFile* file = theApp.downloadqueue->GetFileByID(hash);
			m_client->ProcessFileStatus(false, &data, file);
			break;
		}
		
		case OP_STARTUPLOADREQ: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_STARTUPLOADREQ from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
		
			if (!m_client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode)) {
				break;
			}
			
			m_client->CheckForAggressive();
			if ( m_client->IsBanned() ) {
				break;
			}
			
			if (size == 16) {
				const CMD4Hash fileID((byte*)buffer);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(fileID);
				if (reqfile) {
					if (m_client->GetUploadFileID() != fileID) {
						m_client->SetCommentDirty();
					}
					m_client->SetUploadFileID(reqfile);
					m_client->SendCommentInfo(reqfile);

					// Socket might die because of SendCommentInfo, so check
					if (m_client)
						theApp.uploadqueue->AddClientToQueue(m_client);
				}
			}
			break;
		}
		
		case OP_QUEUERANK: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_QUEUERANK from ") + m_client->GetFullIP() );
			 
			theStats::AddDownOverheadFileRequest(size);
			CMemFile data(buffer, size);
			uint32 rank = data.ReadUInt32();
			
			m_client->SetRemoteQueueRank(rank);
			break;
		}
		
		case OP_ACCEPTUPLOADREQ: {	// 0.42e (xcept khaos stats)
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ACCEPTUPLOADREQ from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
				if (m_client->GetDownloadState() == DS_ONQUEUE ) {
					m_client->SetDownloadState(DS_DOWNLOADING);
					m_client->SetLastPartAsked(0xffff); // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
					m_client->SendBlockRequests();
				}
			} else {
				if (!m_client->GetSentCancelTransfer()) {
					CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
					theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetFullIP() );
					m_client->SendPacket(packet,true,true);
					
					// SendPacket can cause the socket to die, so check
					if (m_client)
						m_client->SetSentCancelTransfer(1);
				}
				
				if (m_client)
					m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
			}
			break;
		}
		
		case OP_REQUESTPARTS: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTPARTS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);

			m_client->ProcessRequestPartsPacket(buffer, size, false);
			
			break;
		}
		
		case OP_CANCELTRANSFER: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CANCELTRANSFER from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			theApp.uploadqueue->RemoveFromUploadQueue(m_client);
			if ( CLogger::IsEnabled( logClient ) ) {
				AddDebugLogLineM( false, logClient, m_client->GetUserName() + wxT(": Upload session ended due canceled transfer."));
			}
			break;
		}
		
		case OP_END_OF_DOWNLOAD: { // 0.43b except check for bad clients
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_END_OF_DOWNLOAD from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			if (size>=16 && m_client->GetUploadFileID() == CMD4Hash((byte*)buffer)) {
				theApp.uploadqueue->RemoveFromUploadQueue(m_client);
				if ( CLogger::IsEnabled( logClient ) ) {
					AddDebugLogLineM( false, logClient, m_client->GetUserName() + wxT(": Upload session ended due ended transfer."));
				}
			}
			break;
		}
		
		case OP_HASHSETREQUEST: {		// 0.43b except check for bad clients
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HASHSETREQUEST from ") + m_client->GetFullIP() );
			
			
			theStats::AddDownOverheadFileRequest(size);
			if (size != 16) {
				throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
			}
			m_client->SendHashsetPacket(CMD4Hash(buffer));
			break;
		}
		
		case OP_HASHSETANSWER: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HASHSETANSWER from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			m_client->ProcessHashSet(buffer, size);
			break;
		}
		
		case OP_SENDINGPART: {		// 0.47a
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SENDINGPART from ") + m_client->GetFullIP() );
			
			if (	 m_client->GetRequestFile() && 
				!m_client->GetRequestFile()->IsStopped() && 
				(m_client->GetRequestFile()->GetStatus() == PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
				
				m_client->ProcessBlockPacket(buffer, size, false, false);
					
				if ( 	m_client && 
					( m_client->GetRequestFile()->IsStopped() || 
					  m_client->GetRequestFile()->GetStatus() == PS_PAUSED || 
					  m_client->GetRequestFile()->GetStatus() == PS_ERROR) ) {
					if (!m_client->GetSentCancelTransfer()) {
						CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
						theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
						AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetFullIP() );
						m_client->SendPacket(packet,true,true);
						
						// Socket might die because of SendPacket, so check
						if (m_client)
							m_client->SetSentCancelTransfer(1);
					}

					if (m_client)
						m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
				}
			} else {
				if (!m_client->GetSentCancelTransfer()) {
					CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
					theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetFullIP() );
					m_client->SendPacket(packet,true,true);
					
					// Socket might die because of SendPacket, so check
					m_client->SetSentCancelTransfer(1);
				}
				m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
			}
			break;
		}
		
		case OP_OUTOFPARTREQS: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_OUTOFPARTREQS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);
			if (m_client->GetDownloadState() == DS_DOWNLOADING) {
				m_client->SetDownloadState(DS_ONQUEUE);
			}
			break;
		}
		
		case OP_CHANGE_CLIENT_ID: { 	// Kad reviewed
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CHANGE_CLIENT_ID from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			CMemFile data(buffer, size);
			uint32 nNewUserID = data.ReadUInt32();
			uint32 nNewServerIP = data.ReadUInt32();
			
			if (IsLowID(nNewUserID)) { // client changed server and gots a LowID
				CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
				if (pNewServer != NULL){
					m_client->SetUserIDHybrid(nNewUserID); // update UserID only if we know the server
					m_client->SetServerIP(nNewServerIP);
					m_client->SetServerPort(pNewServer->GetPort());
				}
			} else if (nNewUserID == m_client->GetIP()) { // client changed server and gots a HighID(IP)
				m_client->SetUserIDHybrid(wxUINT32_SWAP_ALWAYS(nNewUserID));
				CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
				if (pNewServer != NULL){
					m_client->SetServerIP(nNewServerIP);
					m_client->SetServerPort(pNewServer->GetPort());
				}
			} 
			
			break;
		}					
		
		case OP_CHANGE_SLOT:{	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CHANGE_SLOT from ") + m_client->GetFullIP() );
			
			// sometimes sent by Hybrid
			theStats::AddDownOverheadOther(size);
			break;
		}			
		
		case OP_MESSAGE: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MESSAGE from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			
			CMemFile message_file(buffer, size);

			wxString message = message_file.ReadString(m_client->GetUnicodeSupport());
			if (IsMessageFiltered(message, m_client)) {
				AddLogLineM( true, CFormat(_("Message filtered from '%s' (IP:%s)")) % m_client->GetUserName() % m_client->GetFullIP());
			} else {
				AddLogLineM( true, CFormat(_("New message from '%s' (IP:%s)")) % m_client->GetUserName() % m_client->GetFullIP());
				
				Notify_ChatProcessMsg(GUI_ID(m_client->GetIP(),m_client->GetUserPort()), m_client->GetUserName() + wxT("|") + message);
			}
			break;
		}
		
		case OP_ASKSHAREDFILES:	{	// 0.43b (well, er, it does the same, but in our own way)
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILES from ") + m_client->GetFullIP() );
			
			// client wants to know what we have in share, let's see if we allow him to know that
			theStats::AddDownOverheadOther(size);
			// IP banned, no answer for this request
			if (m_client->IsBanned()) {
				break;
			}
			
			if (thePrefs::CanSeeShares() == vsfaEverybody || (thePrefs::CanSeeShares() == vsfaFriends && m_client->IsFriend())) {
				AddLogLineM( true, CFormat( _("User %s (%u) requested your sharedfiles-list -> Accepted"))
					% m_client->GetUserName() 
					% m_client->GetUserIDHybrid() );
				
				std::vector<CKnownFile*> list;
				theApp.sharedfiles->CopyFileList(list);

				CMemFile tempfile(80);
				tempfile.WriteUInt32(list.size());
				for (unsigned i = 0; i < list.size(); ++i) {
					if (!list[i]->IsLargeFile() || m_client->SupportsLargeFiles()) {
						theApp.sharedfiles->CreateOfferedFilePacket(list[i], &tempfile, NULL, m_client);
					}
				}
				
				// create a packet and send it
				CPacket* replypacket = new CPacket(&tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESANSWER);
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESANSWER to ") + m_client->GetFullIP() );
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				SendPacket(replypacket, true, true);
			} else {
				AddLogLineM( true, CFormat( _("User %s (%u) requested your sharedfiles-list -> Denied"))
					% m_client->GetUserName() 
					% m_client->GetUserIDHybrid() );
			
				CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true, true);				
			}

			break;
		}
		
		case OP_ASKSHAREDFILESANSWER: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESANSWER from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			wxString EmptyStr;
			m_client->ProcessSharedFileList(buffer, size, EmptyStr);
			break;
		}
		
		case OP_ASKSHAREDDIRS: { 		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			wxASSERT( size == 0 );
			// IP banned, no answer for this request
			if (m_client->IsBanned()) {
				break;
			}
			if ((thePrefs::CanSeeShares()==vsfaEverybody) || ((thePrefs::CanSeeShares()==vsfaFriends) && m_client->IsFriend())) {
				AddLogLineM( true, CFormat( _("User %s (%u) requested your shareddirectories-list -> Accepted") )
					% m_client->GetUserName()
					% m_client->GetUserIDHybrid() );

				// Kry - This new code from eMule will avoid duplicated folders
				ArrayOfwxStrings folders_to_send;
				
				uint32 uDirs = theApp.glob_prefs->shareddir_list.GetCount();
			
				// the shared folders
				for (uint32 iDir=0; iDir < uDirs; iDir++) {
					folders_to_send.Add(wxString(theApp.glob_prefs->shareddir_list[iDir]));
				}			
				
				bool bFoundFolder = false;
				
				wxString char_ptrDir;
				// ... the categories folders ... (category 0 -> incoming)
				for (uint32 ix=0;ix< theApp.glob_prefs->GetCatCount();ix++) {
					char_ptrDir = theApp.glob_prefs->GetCategory(ix)->incomingpath;
					bFoundFolder = false;
					for (uint32 iDir=0; iDir < (uint32)folders_to_send.GetCount(); iDir++) {	
						if (folders_to_send[iDir].CmpNoCase(char_ptrDir) == 0) {
							bFoundFolder = true;
							break;
						}
					}			
					if (!bFoundFolder) {
						folders_to_send.Add(wxString(char_ptrDir));
					}							
				}
	
				// ... and the Magic thing from the eDonkey Hybrids...
				bFoundFolder = false;
				for (uint32 iDir = 0; iDir < (uint32) folders_to_send.GetCount(); iDir++) {
					if (folders_to_send[iDir].CmpNoCase(OP_INCOMPLETE_SHARED_FILES) == 0) {
						bFoundFolder = true;
						break;
					}
				}
				if (!bFoundFolder) {
					folders_to_send.Add(wxString(OP_INCOMPLETE_SHARED_FILES));
				}
				
				// Send packet.
				CMemFile tempfile(80);

				uDirs = folders_to_send.GetCount();
				tempfile.WriteUInt32(uDirs);
				for (uint32 iDir=0; iDir < uDirs; iDir++) {
					tempfile.WriteString(folders_to_send[iDir]);
				}

				CPacket* replypacket = new CPacket(&tempfile, OP_EDONKEYPROT, OP_ASKSHAREDDIRSANS);
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDDIRSANS to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true, true);
			} else {
				AddLogLineM( true, CFormat( _("User %s (%u) requested your shareddirectories-list -> Denied") )
					% m_client->GetUserName()
					% m_client->GetUserIDHybrid() );

				CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true, true);
			}

			break;
		}
		
		case OP_ASKSHAREDFILESDIR: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIR from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			// IP banned, no answer for this request
			if (m_client->IsBanned()) {
				break;
			}
			CMemFile data(buffer, size);
										
			wxString strReqDir = data.ReadString(m_client->GetUnicodeSupport());
			if (thePrefs::CanSeeShares()==vsfaEverybody || (thePrefs::CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
				AddLogLineM( true, CFormat(_("User %s (%u) requested your sharedfiles-list for directory %s -> accepted")) % m_client->GetUserName() % m_client->GetUserIDHybrid() % strReqDir);
				wxASSERT( data.GetPosition() == data.GetLength() );
				CTypedPtrList<CPtrList, CKnownFile*> list;
				
				if (strReqDir == OP_INCOMPLETE_SHARED_FILES) {
					// get all shared files from download queue
					int iQueuedFiles = theApp.downloadqueue->GetFileCount();
					for (int i = 0; i < iQueuedFiles; i++) {
						CPartFile* pFile = theApp.downloadqueue->GetFileByIndex(i);
						if (pFile == NULL || pFile->GetStatus(true) != PS_READY) {
							continue;
						}
						list.AddTail(pFile);
					}
				} else {
					theApp.sharedfiles->GetSharedFilesByDirectory(strReqDir,list);
				}

				CMemFile tempfile(80);
				tempfile.WriteString(strReqDir);
				tempfile.WriteUInt32(list.GetCount());
				while (list.GetCount()) {
					if (!list.GetHead()->IsLargeFile() || m_client->SupportsLargeFiles()) {
						theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, NULL, m_client);
					}
					list.RemoveHead();
				}
				
				CPacket* replypacket = new CPacket(&tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESDIRANS);
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESDIRANS to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true, true);
			} else {
				AddLogLineM( true, CFormat(_("User %s (%u) requested your sharedfiles-list for directory %s -> denied")) % m_client->GetUserName() % m_client->GetUserIDHybrid() % strReqDir);
				
				CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
				theStats::AddUpOverheadOther(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true, true);
			}
			break;
		}		
		
		case OP_ASKSHAREDDIRSANS:{		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRSANS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			if (m_client->GetFileListRequested() == 1){
				CMemFile data(buffer, size);
				uint32 uDirs = data.ReadUInt32();
				for (uint32 i = 0; i < uDirs; i++){
					wxString strDir = data.ReadString(m_client->GetUnicodeSupport());
					AddLogLineM( true, CFormat( _("User %s (%u) shares directory %s") )
						% m_client->GetUserName()
						% m_client->GetUserIDHybrid()
						% strDir );
			
					CMemFile tempfile(80);
					tempfile.WriteString(strDir);
					CPacket* replypacket = new CPacket(&tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESDIR);
					theStats::AddUpOverheadOther(replypacket->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESD to ") + m_client->GetFullIP() );
					SendPacket(replypacket, true, true);
				}
				wxASSERT( data.GetPosition() == data.GetLength() );
				m_client->SetFileListRequested(uDirs);
			} else {
				AddLogLineM( true, CFormat( _("User %s (%u) sent unrequested shared dirs.") )
					% m_client->GetUserName() 
					% m_client->GetUserIDHybrid() );
			}
			break;
		}
  
		case OP_ASKSHAREDFILESDIRANS: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIRANS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			CMemFile data(buffer, size);
			wxString strDir = data.ReadString(m_client->GetUnicodeSupport());

			if (m_client->GetFileListRequested() > 0){
				AddLogLineM( true, CFormat( _("User %s (%u) sent sharedfiles-list for directory %s") )
					% m_client->GetUserName()
					% m_client->GetUserIDHybrid()
					% strDir );
				
				m_client->ProcessSharedFileList(buffer + data.GetPosition(), size - data.GetPosition(), strDir);
				if (m_client->GetFileListRequested() == 0) {
					AddLogLineM( true, CFormat( _("User %s (%u) finished sending sharedfiles-list") )
						% m_client->GetUserName()
						% m_client->GetUserIDHybrid() );
				}
			} else {
				AddLogLineM( true, CFormat( _("User %s (%u) sent unwanted sharedfiles-list") )
					% m_client->GetUserName()
					% m_client->GetUserIDHybrid() );
			}
			break;
		}
		
		case OP_ASKSHAREDDENIEDANS:
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDENIEDANS from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadOther(size);
			wxASSERT( size == 0 );
			AddLogLineM( true, CFormat( _("User %s (%u) denied access to shared directories/files list") )
				% m_client->GetUserName()
				% m_client->GetUserIDHybrid() );
					
			m_client->SetFileListRequested(0);			
			break;
		
		default:
			theStats::AddDownOverheadOther(size);
			AddDebugLogLineM( false, logRemoteClient, wxString::Format(wxT("Edonkey packet: unknown opcode: %i %x from "), opcode, opcode) + m_client->GetFullIP());
			return false;
	}
	
	return true;
}


bool CClientTCPSocket::ProcessExtPacket(const byte* buffer, uint32 size, uint8 opcode)
{
	#ifdef __PACKET_RECV_DUMP__
	printf("Rec: OPCODE %x \n",opcode);
	DumpMem(buffer,size);
	#endif
		
	// 0.42e - except the catchs on mem exception and file exception
	if (!m_client) {
		throw wxString(wxT("Unknown clients sends extended protocol packet"));
	}
	/*
	if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
		// Here comes a extended packet without finishing the hanshake.
		// IMHO, we should disconnect the client.
		throw wxString(wxT("Client send extended packet before finishing handshake"));
	}
	*/
	switch(opcode) {
		case OP_MULTIPACKET_EXT:
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET_EXT from ") + m_client->GetFullIP());
		case OP_MULTIPACKET: {	
			if (opcode == OP_MULTIPACKET) AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET from ") + m_client->GetFullIP() );

			theStats::AddDownOverheadFileRequest(size);

			if (m_client->IsBanned()) {
				break;
			}

			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_MULTIPACKET before finishing handshake"));
			}

			CMemFile data_in(buffer, size);
			CMD4Hash reqfilehash = data_in.ReadHash();
			uint64 nSize = (opcode == OP_MULTIPACKET_EXT) ? data_in.ReadUInt64() : 0;
			
			bool file_not_found = false;
			CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
			if ( reqfile == NULL ){
				reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
				if ( !( reqfile != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
					AddDebugLogLineM(false, logRemoteClient, wxT("Remote client asked for a non-shared file"));
					file_not_found = true;
				}
			}
			
			if (!file_not_found && reqfile->IsLargeFile() && !m_client->SupportsLargeFiles()) {
				AddDebugLogLineM(false, logRemoteClient, wxT("Remote client asked for a large file but doesn't support them"));
				file_not_found = true;
			}				
			
			if (!file_not_found && nSize && (reqfile->GetFileSize() != nSize)) {
				AddDebugLogLineM(false, logRemoteClient, wxT("Remote client asked for a file but specified wrong size"));
				file_not_found = true;
			}

			if (file_not_found) {
				CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16, OP_EDONKEYPROT);
				replypacket->Copy16ToDataBuffer(reqfilehash.GetHash());
				theStats::AddUpOverheadFileRequest(replypacket->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_FILEREQANSNOFIL to ") + m_client->GetFullIP() );
				SendPacket(replypacket, true);
				break;				
			}
			
			if (!m_client->GetWaitStartTime()) {
				m_client->SetWaitStartTime();
			}
			// if we are downloading this file, this could be a new source
			// no passive adding of files with only one part
			if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
				if (thePrefs::GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
					theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, m_client);
				}
			}
			// check to see if this is a new file they are asking for
			if (m_client->GetUploadFileID() != reqfilehash) {
				m_client->SetCommentDirty();
			}
			m_client->SetUploadFileID(reqfile);
			CMemFile data_out(128);
			data_out.WriteHash(reqfile->GetFileHash());
			while(data_in.GetLength()-data_in.GetPosition()) {
				if (!m_client) {
					throw wxString(wxT("Client suddenly disconnected"));
				}
				uint8 opcode_in = data_in.ReadUInt8();
				switch(opcode_in) {
					case OP_REQUESTFILENAME: {
						AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_REQUESTFILENAME") );
						m_client->ProcessExtendedInfo(&data_in, reqfile);
						data_out.WriteUInt8(OP_REQFILENAMEANSWER);
						data_out.WriteString(reqfile->GetFileName());
						break;
					}
					case OP_AICHFILEHASHREQ: {
						AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_AICHFILEHASHANS") );
						if (m_client->IsSupportingAICH() && reqfile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
							&& reqfile->GetAICHHashset()->HasValidMasterHash())
						{
							data_out.WriteUInt8(OP_AICHFILEHASHANS);
							reqfile->GetAICHHashset()->GetMasterHash().Write(&data_out);
						}
						break;
					}						
					case OP_SETREQFILEID: {
						AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_SETREQFILEID") );
						data_out.WriteUInt8(OP_FILESTATUS);
						if (reqfile->IsPartFile()) {
							((CPartFile*)reqfile)->WritePartStatus(&data_out);
						} else {
							data_out.WriteUInt16(0);
						}
						break;
					}
					//We still send the source packet seperately.. 
					//We could send it within this packet.. If agreeded, I will fix it..
					case OP_REQUESTSOURCES: {
						AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_REQUESTSOURCES") );
						//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
						if (m_client->GetSourceExchangeVersion() > 1) {
							uint32 dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
							bool bNeverAskedBefore = m_client->GetLastSrcReqTime() == 0;
							if( 
									//if not complete and file is rare
									(    reqfile->IsPartFile()
									&& (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
									&& ((CPartFile*)reqfile)->GetSourceCount() <= RARE_FILE
									) ||
									//OR if file is not rare or if file is complete
									( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
									) 
							{
								m_client->SetLastSrcReqTime();
								CPacket* tosend = reqfile->CreateSrcInfoPacket(m_client);
								if(tosend) {
									theStats::AddUpOverheadSourceExchange(tosend->GetPacketSize());
									AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ANSWERSOURCES to ") + m_client->GetFullIP() );
									SendPacket(tosend, true);
								}
							}
						}
						break;
					}
				}

			}
			if( data_out.GetLength() > 16 ) {
				CPacket* reply = new CPacket(&data_out, OP_EMULEPROT, OP_MULTIPACKETANSWER);
				theStats::AddUpOverheadFileRequest(reply->GetPacketSize());
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_MULTIPACKETANSWER to ") + m_client->GetFullIP() );
				SendPacket(reply, true);
			}
			break;
		}

		case OP_MULTIPACKETANSWER: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKETANSWER from ") + m_client->GetFullIP() );
			
			theStats::AddDownOverheadFileRequest(size);

			if (m_client->IsBanned()) {
				break;
			}

			if( m_client->GetKadPort() ) {
				Kademlia::CKademlia::bootstrap(wxUINT32_SWAP_ALWAYS(m_client->GetIP()), m_client->GetKadPort());
			}

 			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_MULTIPACKETANSWER before finishing handshake"));
			}
			
			CMemFile data_in(buffer, size);
			CMD4Hash reqfilehash = data_in.ReadHash();
			const CPartFile *reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
			//Make sure we are downloading this file.
			if ( !reqfile ) {
				throw wxString(wxT(" Wrong File ID: (OP_MULTIPACKETANSWER; reqfile==NULL)"));
			}
			if ( !m_client->GetRequestFile() ) {

				throw wxString(wxT(" Wrong File ID: OP_MULTIPACKETANSWER; client->reqfile==NULL)"));
			}
			if (reqfile != m_client->GetRequestFile()) {
				throw wxString(wxT(" Wrong File ID: OP_MULTIPACKETANSWER; reqfile!=client->reqfile)"));
			}
			while (data_in.GetLength()-data_in.GetPosition()) {
				// Some of the cases down there can actually send a packet and lose the client
				if (!m_client) {
					throw wxString(wxT("Client suddenly disconnected"));
				}
				uint8 opcode_in = data_in.ReadUInt8();
				switch(opcode_in) {
					case OP_REQFILENAMEANSWER: {
						if (!m_client) {
							throw wxString(wxT("Client suddenly disconnected"));
						} else {
							m_client->ProcessFileInfo(&data_in, reqfile);
						}
						break;
					}
					case OP_FILESTATUS: {
						if (!m_client) {
							throw wxString(wxT("Client suddenly disconnected"));
						} else {
							m_client->ProcessFileStatus(false, &data_in, reqfile);
						}
						break;
					}
					case OP_AICHFILEHASHANS: {
						if (!m_client) {
							throw wxString(wxT("Client suddenly disconnected"));
						} else {					
							m_client->ProcessAICHFileHash(&data_in, reqfile);
						}
						break;
					}
				}
			}

			break;
		}
	
		case OP_EMULEINFO: {	// 0.43b
			theStats::AddDownOverheadOther(size);

			if (!m_client->ProcessMuleInfoPacket(buffer, size)) { 
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFO from ") + m_client->GetFullIP() );
				
				// If it's not a OS Info packet, is an old client
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				if (m_client->GetInfoPacketsReceived() == IP_BOTH) {	
					m_client->InfoPacketsReceived();
				}
				m_client->SendMuleInfoPacket(true);
			} else {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFO is an OS_INFO") );
			}
			break;
		}
		case OP_EMULEINFOANSWER: {	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFOANSWER from ") + m_client->GetFullIP() );
			theStats::AddDownOverheadOther(size);
			
			m_client->ProcessMuleInfoPacket(buffer, size);
			// start secure identification, if
			//  - we have received eD2K and eMule info (old eMule)
			
			if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
				m_client->InfoPacketsReceived();				
			}
			
			break;
		}
		
		case OP_SECIDENTSTATE:{		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SECIDENTSTATE from ") + m_client->GetFullIP() );
			
			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_SECIDENTSTATE before finishing handshake"));
			}								
			m_client->ProcessSecIdentStatePacket((byte*)buffer, size);
			// ProcessSecIdentStatePacket() might cause the socket to die, so check
			if (m_client) {
				int SecureIdentState = m_client->GetSecureIdentState();
				if (SecureIdentState == IS_SIGNATURENEEDED) {
					m_client->SendSignaturePacket();
				} else if (SecureIdentState == IS_KEYANDSIGNEEDED) {
					m_client->SendPublicKeyPacket();
					// SendPublicKeyPacket() might cause the socket to die, so check
					if ( m_client ) {
						m_client->SendSignaturePacket();
					}
				}
			}
			break;
		}
		
		case OP_PUBLICKEY: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PUBLICKEY from ") + m_client->GetFullIP() );
			
			if (m_client->IsBanned() ){
				break;						
			}
			
			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_PUBLICKEY before finishing handshake"));
			}
											
			m_client->ProcessPublicKeyPacket((byte*)buffer, size);
			break;
		}
		case OP_SIGNATURE:{			// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SIGNATURE from ") + m_client->GetFullIP() );
			
			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
			}
											
			m_client->ProcessSignaturePacket((byte*)buffer, size);
			break;
		}
		case OP_SENDINGPART_I64:
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SENDINGPART_I64 from ") + m_client->GetFullIP() );
		case OP_COMPRESSEDPART_I64:
			if (opcode == OP_COMPRESSEDPART_I64) AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_COMPRESSEDPART_I64 from ") + m_client->GetFullIP() );
		case OP_COMPRESSEDPART: {	// 0.47a
			if (opcode == OP_COMPRESSEDPART) AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_COMPRESSEDPART from ") + m_client->GetFullIP() );
			
			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
			}
											
			if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
				
				m_client->ProcessBlockPacket(buffer, size, (opcode != OP_SENDINGPART_I64), (opcode == OP_COMPRESSEDPART_I64) || (opcode == OP_SENDINGPART_I64));
				
				if (m_client && (
					m_client->GetRequestFile()->IsStopped() ||
					m_client->GetRequestFile()->GetStatus() == PS_PAUSED ||
					m_client->GetRequestFile()->GetStatus() == PS_ERROR)) {
					if (!m_client->GetSentCancelTransfer()) {
						CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
						theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
						AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetFullIP() );
						m_client->SendPacket(packet,true,true);					
						
						if (m_client) {
							m_client->SetSentCancelTransfer(1);
						}
					}

					if ( m_client ) {
						m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);	
					}
				}
			} else {
				if (!m_client->GetSentCancelTransfer()) {
					CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
					theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetFullIP() );
					m_client->SendPacket(packet,true,true);
					
					if ( m_client ) {
						m_client->SetSentCancelTransfer(1);
					}
				}
			
				if ( m_client ) {
					m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
			}
			break;
		}
		case OP_REQUESTPARTS_I64: {	
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTPARTS_I64 from ") + m_client->GetFullIP()  );
			
			theStats::AddDownOverheadFileRequest(size);

			m_client->ProcessRequestPartsPacket(buffer, size, true);
			
			break;
		}		
		case OP_QUEUERANKING: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_QUEUERANKING from ") + m_client->GetFullIP()  );
			
			theStats::AddDownOverheadOther(size);
			
			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_QUEUERANKING before finishing handshake"));
			}
			
			if (size != 12) {
				throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
			}

			uint16 newrank = PeekUInt16(buffer);
			m_client->SetRemoteQueueFull(false);
			m_client->SetRemoteQueueRank(newrank);
			break;
		}
		
		case OP_REQUESTSOURCES:{	// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTSOURCES from ") + m_client->GetFullIP()  );
			
			theStats::AddDownOverheadSourceExchange(size);

			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_REQUESTSOURCES before finishing handshake"));
			}
			
			if (m_client->GetSourceExchangeVersion() >= 1) {
				if(size != 16) {
					throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
				}
				//first check shared file list, then download list
				const CMD4Hash fileID((byte*)buffer);
				CKnownFile* file = theApp.sharedfiles->GetFileByID(fileID);
				if(!file) {
					file = theApp.downloadqueue->GetFileByID(fileID);
				}
				if(file) {
					uint32 dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
					bool bNeverAskedBefore = m_client->GetLastSrcReqTime() == 0;
					if( 
					//if not complete and file is rare, allow once every 40 minutes
					( file->IsPartFile() &&
					((CPartFile*)file)->GetSourceCount() <= RARE_FILE &&
					(bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
					) ||
					//OR if file is not rare or if file is complete, allow every 90 minutes
					( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
					)
					{
						m_client->SetLastSrcReqTime();
						CPacket* tosend = file->CreateSrcInfoPacket(m_client);
						if(tosend) {
							theStats::AddUpOverheadSourceExchange(tosend->GetPacketSize());
							AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ANSWERSOURCES to ") + m_client->GetFullIP() );
							SendPacket(tosend, true, true);
						}
					}
				}
			}
			break;
		}
		
		case OP_ANSWERSOURCES: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ANSWERSOURCES from ") + m_client->GetFullIP()  );
			
			theStats::AddDownOverheadSourceExchange(size);

			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_ANSWERSOURCES before finishing handshake"));
			}
			
			CMemFile data(buffer, size);
			CMD4Hash hash = data.ReadHash();
			const CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
			if(file){
				if (file->IsPartFile()){
					//set the client's answer time
					m_client->SetLastSrcAnswerTime();
					//and set the file's last answer time
					((CPartFile*)file)->SetLastAnsweredTime();

					((CPartFile*)file)->AddClientSources(&data, m_client->GetSourceExchangeVersion(), SF_SOURCE_EXCHANGE);
				}
			}
			break;
		}
		case OP_FILEDESC: {		// 0.43b
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILEDESC from ") + m_client->GetFullIP()  );
			
			theStats::AddDownOverheadFileRequest(size);

			if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
				// Here comes a extended packet without finishing the hanshake.
				// IMHO, we should disconnect the client.
				throw wxString(wxT("Client send OP_FILEDESC before finishing handshake"));
			}
			
			m_client->ProcessMuleCommentPacket(buffer, size);
			break;
		}

		// Unsupported
		case OP_REQUESTPREVIEW: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTPREVIEW  from ") + m_client->GetFullIP()  );
			break;
		}
		// Unsupported
		case OP_PREVIEWANSWER: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PREVIEWANSWER from ") + m_client->GetFullIP()  );
			break;
		}

		case OP_PUBLICIP_ANSWER: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PUBLICIP_ANSWER from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadOther(size);
			m_client->ProcessPublicIPAnswer(buffer, size);
			break;
		}
		case OP_PUBLICIP_REQ: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PUBLICIP_REQ from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadOther(size);
			CPacket* pPacket = new CPacket(OP_PUBLICIP_ANSWER, 4, OP_EMULEPROT);
			pPacket->CopyUInt32ToDataBuffer(m_client->GetIP());
			theStats::AddUpOverheadOther(pPacket->GetPacketSize());
			AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_PUBLICIP_ANSWER to") + m_client->GetFullIP());
			SendPacket(pPacket);
			break;
		}			
		case OP_AICHANSWER: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_AICHANSWER from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadOther(size);
			m_client->ProcessAICHAnswer(buffer, size);
			break;
		}
		case OP_AICHREQUEST: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_AICHREQUEST from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadOther(size);
			m_client->ProcessAICHRequest(buffer, size);
			break;
		}
		case OP_AICHFILEHASHANS: {
			// those should not be received normally, since we should only get those in MULTIPACKET
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_AICHFILEHASHANS from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadOther(size);
			CMemFile data(buffer, size);
			m_client->ProcessAICHFileHash(&data, NULL);
			break;
		}
		case OP_AICHFILEHASHREQ: {
			// those should not be received normally, since we should only get those in MULTIPACKET
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_AICHFILEHASHREQ from ") + m_client->GetFullIP()  );
			CMemFile data(buffer, size);
			CMD4Hash hash = data.ReadHash();
			CKnownFile* pPartFile = theApp.sharedfiles->GetFileByID(hash);
			if (pPartFile == NULL){
				break;
			}
			
			if (m_client->IsSupportingAICH() && pPartFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
				&& pPartFile->GetAICHHashset()->HasValidMasterHash()) {
				CMemFile data_out;
				data_out.WriteHash(hash);
				pPartFile->GetAICHHashset()->GetMasterHash().Write(&data_out);
				CPacket* packet = new CPacket(&data_out, OP_EMULEPROT, OP_AICHFILEHASHANS);
				theStats::AddUpOverheadOther(packet->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_AICHFILEHASHANS to") + m_client->GetFullIP());
				SendPacket(packet);
			}
			break;
		}
		case OP_CALLBACK: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CALLBACK from ") + m_client->GetFullIP() );				
			theStats::AddDownOverheadFileRequest(size);
			if(!Kademlia::CKademlia::isRunning()) {
				break;
			}
			CMemFile data(buffer, size);
			CUInt128 check = data.ReadUInt128();
			check.XOR(Kademlia::CUInt128(true));
			if( check.compareTo(Kademlia::CKademlia::getPrefs()->getKadID())) {
				break;
			}
			CUInt128 fileid = data.ReadUInt128();
			byte fileid2[16];
			fileid.toByteArray(fileid2);
			const CMD4Hash fileHash(fileid2);
			if (theApp.sharedfiles->GetFileByID(fileHash) == NULL) {
				if (theApp.downloadqueue->GetFileByID(fileHash) == NULL) {
					break;
				}
			}

			uint32 ip = data.ReadUInt32();
			uint16 tcp = data.ReadUInt16();
			CUpDownClient* callback;
			callback = theApp.clientlist->FindClientByIP(wxUINT32_SWAP_ALWAYS(ip), tcp);
			if( callback == NULL ) {
				#warning Do we actually have to check friend status here?
				callback = new CUpDownClient(tcp,ip,0,0,NULL,false, false);
				theApp.clientlist->AddClient(callback);
			}
			callback->TryToConnect(true);
			break;
		}
		
		case OP_BUDDYPING: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_BUDDYPING from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadKad(size);

			CUpDownClient* buddy = theApp.clientlist->GetBuddy();
			if( buddy != m_client || m_client->GetKadVersion() == 0 || !m_client->AllowIncomeingBuddyPingPong() ) {
				//This ping was not from our buddy or wrong version or packet sent to fast. Ignore
				break;
			}
			
			m_client->SetLastBuddyPingPongTime();
			CPacket* replypacket = new CPacket(OP_BUDDYPONG, 0, OP_EMULEPROT);
			theStats::AddUpOverheadKad(replypacket->GetPacketSize());
			AddDebugLogLineM(false, logLocalClient,wxT("Local Client: OP_BUDDYPONG to ") + m_client->GetFullIP());
			SendPacket(replypacket);
			break;
		}
		case OP_BUDDYPONG: {
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_BUDDYPONG from ") + m_client->GetFullIP()  );
			theStats::AddDownOverheadKad(size);

			CUpDownClient* buddy = theApp.clientlist->GetBuddy();
			if( buddy != m_client || m_client->GetKadVersion() == 0 ) {
				//This pong was not from our buddy or wrong version. Ignore
				break;
			}
			m_client->SetLastBuddyPingPongTime();
			//All this is for is to reset our socket timeout.
			break;
		}
		case OP_REASKCALLBACKTCP: {
			theStats::AddDownOverheadFileRequest(size);
			CUpDownClient* buddy = theApp.clientlist->GetBuddy();
			if (buddy != m_client) {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REASKCALLBACKTCP from ") + m_client->GetFullIP() + wxT(" which is not our buddy!") );
				//This callback was not from our buddy.. Ignore.
				break;
			}
			AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REASKCALLBACKTCP from ") + m_client->GetFullIP()  );				
			CMemFile data_in(buffer, size);
			uint32 destip = data_in.ReadUInt32();
			uint16 destport = data_in.ReadUInt16();
			CMD4Hash hash = data_in.ReadHash();
			CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(hash);
			if (!reqfile) {
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_FILENOTFOUND to ") + m_client->GetFullIP() );
				CPacket* response = new CPacket(OP_FILENOTFOUND,0,OP_EMULEPROT);
				theStats::AddUpOverheadFileRequest(response->GetPacketSize());
				theApp.clientudp->SendPacket(response, destip, destport);
				break;
			}
			
			CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(destip, destport);
			if (sender) {
				//Make sure we are still thinking about the same file
				if (hash == sender->GetUploadFileID()) {
					sender->AddAskedCount();
					sender->SetLastUpRequest();
					//I messed up when I first added extended info to UDP
					//I should have originally used the entire ProcessExtenedInfo the first time.
					//So now I am forced to check UDPVersion to see if we are sending all the extended info.
					//For now on, we should not have to change anything here if we change
					//anything to the extended info data as this will be taken care of in ProcessExtendedInfo()
					//Update extended info. 
					if (sender->GetUDPVersion() > 3) {
						sender->ProcessExtendedInfo(&data_in, reqfile);
					} else if (sender->GetUDPVersion() > 2) {
						//Update our complete source counts.			
						uint16 nCompleteCountLast= sender->GetUpCompleteSourcesCount();
						uint16 nCompleteCountNew = data_in.ReadUInt16();
						sender->SetUpCompleteSourcesCount(nCompleteCountNew);
						if (nCompleteCountLast != nCompleteCountNew) {
							reqfile->UpdatePartsInfo();
						}
					}
					
					CMemFile data_out(128);
					if(sender->GetUDPVersion() > 3) {
						if (reqfile->IsPartFile()) {
							((CPartFile*)reqfile)->WritePartStatus(&data_out);
						} else {
							data_out.WriteUInt16(0);
						}
					}
					
					data_out.WriteUInt16(theApp.uploadqueue->GetWaitingPosition(sender));
					CPacket* response = new CPacket(&data_out, OP_EMULEPROT, OP_REASKACK);
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client UDP: OP_REASKACK to ") + m_client->GetFullIP()  );
					theApp.clientudp->SendPacket(response, destip, destport);
				} else {
					AddDebugLogLineM(false, logListenSocket, wxT("Client UDP socket; OP_REASKCALLBACKTCP; reqfile does not match"));
				}
			} else {
				if ((theStats::GetWaitingUserCount() + 50) > thePrefs::GetQueueSize()) {
					AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_QUEUEFULL to ") + m_client->GetFullIP()  );
					CPacket* response = new CPacket(OP_QUEUEFULL,0,OP_EMULEPROT);
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					theApp.clientudp->SendPacket(response, destip, destport);
				}
			}
			break;
		}
		default:
			theStats::AddDownOverheadOther(size);
			AddDebugLogLineM( false, logRemoteClient, wxString::Format(wxT("eMule packet : unknown opcode: %i %x from "),opcode,opcode) + m_client->GetFullIP());
			break;
	}
	
	return true;
}


void CClientTCPSocket::OnConnect(int nErrorCode)
{
	if (nErrorCode) {
		OnError(nErrorCode);
	} else if (!m_client) {
		// and now? Disconnect? not?			
		AddDebugLogLineM( false, logClient, wxT("Couldn't send hello packet (Client deleted!)") );
	} else if (!m_client->SendHelloPacket()) {	
		// and now? Disconnect? not?				
		AddDebugLogLineM( false, logClient, wxT("Couldn't send hello packet (Client deleted by SendHelloPacket!)") );
	} else {
		ResetTimeOutTimer();
	}
}


void CClientTCPSocket::OnSend(int nErrorCode)
{
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}


void CClientTCPSocket::OnReceive(int nErrorCode)
{
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}


void CClientTCPSocket::OnError(int nErrorCode)
{
	// 0.42e + Kry changes for handling of socket lost events
	wxString strError;
	
	if ((nErrorCode == 0) || (nErrorCode == 7) || (nErrorCode == 0xFEFF)) {	
		if (m_client) {
			if (!m_client->GetUserName().IsEmpty()) {
				strError = wxT("Client '") + m_client->GetUserName() + wxT("'");
			} else {
				strError = wxT("An unnamed client");
			}
			strError += wxT(" (IP:") + m_client->GetFullIP() + wxT(") ");
		} else {
			strError = wxT("A client ");
		}
		if (nErrorCode == 0) {
			strError += wxT("closed connection.");
		} else if (nErrorCode == 0xFEFF) {
			strError += wxT(" caused a wxSOCKET_LOST event.");
		}	else {
			strError += wxT("caused a socket blocking error.");
		}
	} else {
		if ( CLogger::IsEnabled( logClient ) && (nErrorCode != 107)) {
			// 0    -> No Error / Disconect
			// 107  -> Transport endpoint is not connected
			if (m_client) {
				if (m_client->GetUserName()) {
					strError = wxT("OnError: Client '") + m_client->GetUserName() +
						wxT("' (IP:") + m_client->GetFullIP() + 
						wxString::Format(wxT(") caused an error: %u. Disconnecting client!"), nErrorCode);
				} else {
					strError = wxT("OnError: Unknown client (IP:") + 
					m_client->GetFullIP() + 
					wxString::Format(wxT(") caused an error: %u. Disconnecting client!"), nErrorCode);
				}
			} else {
				strError = wxString::Format(wxT("OnError: A client caused an error or did something bad (error %u). Disconnecting client !"),
					nErrorCode);
			}
		} else {
			strError = wxT("Error 107 (Transport endpoint is not connected)");
		}	
	}
	
	Disconnect(strError);
}


bool CClientTCPSocket::PacketReceived(CPacket* packet)
{
	// 0.42e
	bool bResult = false;
	uint32 uRawSize = packet->GetPacketSize();

	AddDebugLogLineM( false, logRemoteClient,
		CFormat(wxT("Packet with protocol %x, opcode %x, size %u received from %s"))
			% packet->GetProtocol()
			% packet->GetOpCode()
			% packet->GetPacketSize()
			% ( m_client ? m_client->GetFullIP() : wxT("Unknown Client") )
	);

	wxString exception;
	try {
		switch (packet->GetProtocol()) {
			case OP_EDONKEYPROT:		
				bResult = ProcessPacket(packet->GetDataBuffer(),uRawSize,packet->GetOpCode());
				break;		
			case OP_PACKEDPROT:
				if (!packet->UnPackPacket()) {
					AddDebugLogLineM( false, logZLib, wxT("Failed to decompress client TCP packet."));
					bResult = false;
					break;
				} else {
					AddDebugLogLineM(false, logRemoteClient, 
						wxString::Format(wxT("Packet unpacked, new protocol %x, opcode %x, size %u"), 
							packet->GetProtocol(),
							packet->GetOpCode(),
							packet->GetPacketSize())
					);
				}
			case OP_EMULEPROT:
				bResult = ProcessExtPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
				break;
			default: {
				theStats::AddDownOverheadOther(uRawSize);
				if (m_client) {
					m_client->SetDownloadState(DS_ERROR);
				}
				Disconnect(wxT("Unknown protocol"));
				bResult = false;
			}
		}
	} catch (const CEOFException& err) {
		exception = wxT("EOF exception: ") + err.what();
	} catch (const CInvalidPacket& err) {
		exception = wxT("InvalidPacket exception: ") + err.what();
	} catch (const wxString& error) {
		exception = wxT("error: ") + (error.IsEmpty() ? wxT("Unknown error") : error);
	}

	if (!exception.IsEmpty()) {
		AddDebugLogLineM( false, logPacketErrors,
			CFormat(wxT("Caught %s\n"
						"On packet with protocol %x, opcode %x, size %u"
						"\tClientData: %s\n"))
				% exception
				% packet->GetProtocol()
				% packet->GetOpCode()
				% packet->GetPacketSize()
				% ( m_client ? m_client->GetClientFullInfo() : wxT("Unknown") )
		);
		
		if (m_client) {
			m_client->SetDownloadState(DS_ERROR);
		}
		
		AddDebugLogLineM( false, logClient, 
			CFormat( wxT("Client '%s' (IP: %s) caused an error (%s). Disconnecting client!" ) )
				% ( m_client ? m_client->GetUserName() : wxString(wxT("Unknown")) )
				% ( m_client ? m_client->GetFullIP() : wxString(wxT("Unknown")) )
				% exception
		);
		
		Disconnect(wxT("Caught exception on CClientTCPSocket::ProcessPacket\n"));
	}

	return bResult;
}


bool CClientTCPSocket::IsMessageFiltered(const wxString& Message, CUpDownClient* client) {
	
	bool filtered = false;
	// If we're chatting to the guy, we don't want to filter!
	if (client->GetChatState() != MS_CHATTING) {
		if (thePrefs::MsgOnlyFriends() && !client->IsFriend()) {
			filtered = true;
		} else if (thePrefs::MsgOnlySecure() && client->GetUserName().IsEmpty() ) {
			filtered = true;
		} else if (thePrefs::MustFilterMessages()) {
			filtered = thePrefs::IsMessageFiltered(Message);
		}
	}
	return filtered;
}

SocketSentBytes CClientTCPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
    SocketSentBytes returnStatus = CEMSocket::SendControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

    if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
        ResetTimeOutTimer();
    }

    return returnStatus;
}


SocketSentBytes CClientTCPSocket::SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
	SocketSentBytes returnStatus = CEMSocket::SendFileAndControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

    if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
        ResetTimeOutTimer();
    }

    return returnStatus;
}


void CClientTCPSocket::SendPacket(CPacket* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize)
{
	ResetTimeOutTimer();
	CEMSocket::SendPacket(packet,delpacket,controlpacket, actualPayloadSize);
}
