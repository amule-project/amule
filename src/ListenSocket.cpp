//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ListenSocket.h"
#endif

#include "ListenSocket.h"	// Interface declarations

#include "amule.h"		// Needed for theApp
#include "OtherFunctions.h"	// Needed for md4cpy
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "updownclient.h"	// Needed for CUpDownClient
#include "OPCodes.h"		// Needed for CONNECTION_TIMEOUT
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ClientList.h"		// Needed for CClientList
#include "IPFilter.h"		// Needed for CIPFilter
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "Preferences.h"	// Needed for CPreferences
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "Packet.h"		// Needed for CPacket
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "OtherStructs.h"	// Needed for Requested_Block_Struct
#include "ServerConnect.h"		// Needed for CServerConnect
#include "Statistics.h"
#include "Logger.h"

#include <wx/listimpl.cpp>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/tokenzr.h> 		// Needed for wxStringTokenizer

//#define __PACKET_RECV_DUMP__

//------------------------------------------------------------------------------
// CClientReqSocketHandler
//------------------------------------------------------------------------------
#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(CClientReqSocketHandler, wxEvtHandler)
	EVT_SOCKET(CLIENTREQSOCKET_HANDLER, CClientReqSocketHandler::ClientReqSocketHandler)
END_EVENT_TABLE()

CClientReqSocketHandler::CClientReqSocketHandler(CClientReqSocket* )
{
}

void CClientReqSocketHandler::ClientReqSocketHandler(wxSocketEvent& event)
{
	CClientReqSocket *socket = dynamic_cast<CClientReqSocket *>(event.GetSocket());
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
static CClientReqSocketHandler g_clientReqSocketHandler;

#else
CClientReqSocketHandler::CClientReqSocketHandler(CClientReqSocket* socket)
{
	m_socket = socket;
	socket->my_handler = this;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ERROR: CClientReqSocketHandler failed create\n");
		wxASSERT(0);
	}
}

CClientReqSocketHandler::~CClientReqSocketHandler()
{
	wxASSERT(m_socket == 0);
}

void *CClientReqSocketHandler::Entry()
{
	/*
	if (m_socket->GetClient()) {
		if (m_socket->GetClient()->HasLowID()) {
			printf("DL from lowid\n");
		} else {
			printf("DL from highid\n");
		}
	} else {
		printf("Socket with no client\n");
	}
	*/
	while ( !TestDestroy() ) {
		if ( m_socket->deletethis ) {
			//printf("CClientReqSocketHandler: socket %p in %ld being deleted\n",	m_socket, GetId());
			break;
		}
		if ( m_socket->Error()) {
			if ( m_socket->LastError() == wxSOCKET_WOULDBLOCK ) {
				if ( m_socket->WaitForWrite(0, 0) ) {
					m_socket->OnSend(0);
				}
			} else  {
				break;
			}
		}
		if ( m_socket->deletethis || m_socket->WaitForLost(0, 0) ) {
			break;
		}
		// lfroen: tradeof here - short wait time for high performance on delete
		// but long wait time for low cpu usage
		if ( m_socket->WaitForRead(0, 100) ) {
			if ( m_socket->RecievePending() ) {
				Sleep(50);
			} else {
				CALL_APP_DATA_LOCK;
				m_socket->OnReceive(0);
			}
		}
	}
	//printf("CClientReqSocketHandler: thread %ld for %p exited\n", GetId(), m_socket);
	m_socket->my_handler = 0;
	m_socket->Safe_Delete();
	m_socket = NULL;

	return 0;
}
#endif

//------------------------------------------------------------------------------
// CClientReqSocket
//------------------------------------------------------------------------------

WX_DEFINE_OBJARRAY(ArrayOfwxStrings);

IMPLEMENT_DYNAMIC_CLASS(CClientReqSocket,CEMSocket)

CClientReqSocket::CClientReqSocket(CUpDownClient* in_client, const CProxyData *ProxyData)
:
CEMSocket(ProxyData)
{
	m_client = in_client;
	if (m_client) {
		m_client->SetSocket(this);
	}
	ResetTimeOutTimer();
	deletethis = false;
	last_action = ACTION_NONE;
#ifndef AMULE_DAEMON
	my_handler = &g_clientReqSocketHandler;
	SetEventHandler(*my_handler, CLIENTREQSOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
#else
	my_handler = 0;
	Notify(false);
#endif
	theApp.listensocket->AddSocket(this);
}

void CClientReqSocket::OnInit()
{
	last_action = ACTION_CONNECT;
}

bool CClientReqSocket::Close()
{
	return CEMSocket::Close();
}

// Used in BaseClient.cpp, but not here.
bool CClientReqSocket::Create()
{
	theApp.listensocket->AddConnection();
	OnInit();
	return true;
}

CClientReqSocket::~CClientReqSocket()
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
#ifdef AMULE_DAEMON
	wxASSERT(deletethis && !my_handler);
#endif
}

void CClientReqSocket::ResetTimeOutTimer()
{
	timeout_timer = ::GetTickCount();
}

bool CClientReqSocket::CheckTimeOut()
{
// lfroen: on daemon sockets must be blocking. Which means
// that when client is downloading, i will only trust
// tcp timeout
#ifdef AMULE_DAEMON
	if (my_handler) {
		return false;
	}
#endif
	// 0.42x
	UINT uTimeout = CONNECTION_TIMEOUT;
	if(m_client) {
		#ifdef __USE_KAD__
		if (m_client->GetKadState() == KS_CONNECTED_BUDDY) {
			return false;
		}
		#endif
		if (m_client->GetChatState()!=MS_NONE) {
			timeout_timer = ::GetTickCount();
			return false;
		}
	}
	
	// Kry - Last action check added so we don't remove sources before trying to 
	// connect to them.
	 if (last_action == ACTION_NONE) {
		 timeout_timer = ::GetTickCount();
		 return false;
	 }
	
	if (::GetTickCount() - timeout_timer > uTimeout){
		timeout_timer = ::GetTickCount();
		Disconnect(wxT("Timeout"));
		return true;
	}
	return false;	
}

void CClientReqSocket::OnClose(int nErrorCode)
{
	// 0.42x
	wxASSERT (theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);
	if (nErrorCode > 0) {
		wxString strError;
		strError = wxString::Format(wxT("Closed: %u"),nErrorCode);
		Disconnect(strError);
	} else {
		Disconnect(wxT("Close"));
	}
}

void CClientReqSocket::Disconnect(const wxString& strReason)
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
};


void CClientReqSocket::Safe_Delete()
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
#ifdef AMULE_DAEMON
	if ( !my_handler )
#endif
		Close();
	}
}

bool CClientReqSocket::ProcessPacket(const char* packet, uint32 size, uint8 opcode)
{
	#ifdef __PACKET_RECV_DUMP__
	printf("Rec: OPCODE %x \n",opcode);
	DumpMem(packet,size);
	#endif
	try{
		if (!m_client && opcode != OP_HELLO) {
			throw wxString(wxT("Asks for something without saying hello"));
		} else if (m_client && opcode != OP_HELLO && opcode != OP_HELLOANSWER)
			m_client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode);
		
		switch(opcode) {
			case OP_HELLOANSWER: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HELLOANSWER") );
				theApp.statistics->AddDownDataOverheadOther(size);
				m_client->ProcessHelloAnswer(packet,size);

				// start secure identification, if
				//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
				//	- we have received eMule-OP_HELLOANSWER (new eMule)
				if (m_client->GetInfoPacketsReceived() == IP_BOTH)
					m_client->InfoPacketsReceived();
				
				// Socket might die because of sending in InfoPacketsReceived, so check
				if (m_client) {
					m_client->ConnectionEstablished();
					Notify_UploadCtrlRefreshClient( m_client );
				}
				
				break;
			}
			case OP_HELLO: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HELLO") );
								
				theApp.statistics->AddDownDataOverheadOther(size);
				bool bNewClient = !m_client;				
				if (bNewClient) {
					// create new client to save standart informations
					m_client = new CUpDownClient(this);
				}
				// client->ProcessHelloPacket(packet,size);
				bool bIsMuleHello = false;
				
				try{
					bIsMuleHello = m_client->ProcessHelloPacket(packet,size);
				}
				catch(...){
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
						
				// now we check if we now this client already. if yes this socket will
				// be attached to the known client, the new client will be deleted
				// and the var. "client" will point to the known client.
				// if not we keep our new-constructed client ;)
				if (theApp.clientlist->AttachToAlreadyKnown(&m_client,this)) {
					// update the old client informations
					bIsMuleHello = m_client->ProcessHelloPacket(packet,size);
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
				if (m_client && m_client->m_fOsInfoSupport) {
					m_client->SendMuleInfoPacket(false,true); // Send the OS Info tag on the recycled Mule Info
				}				
				
				// Client might die from Sending in SendMuleInfoPacket, so check
				if ( m_client )
					m_client->ConnectionEstablished();
				
				// start secure identification, if
				//	- we have received eMule-OP_HELLO (new eMule)				
				if (m_client && m_client->GetInfoPacketsReceived() == IP_BOTH) {
						m_client->InfoPacketsReceived();		
				}
				
				break;
			}
			case OP_REQUESTFILENAME: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTFILENAME") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				if (size >= 16) {
					if (!m_client->GetWaitStartTime()) {
						m_client->SetWaitStartTime();
					}
					CSafeMemFile data_in((BYTE*)packet,size);
					CMD4Hash reqfilehash;
					data_in.ReadHash16(reqfilehash);
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
					CSafeMemFile data_out(128);
					data_out.WriteHash16(reqfile->GetFileHash());
					data_out.WriteString(reqfile->GetFileName());
					CPacket* packet = new CPacket(&data_out);
					packet->SetOpCode(OP_REQFILENAMEANSWER);
					
					theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
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
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SETREQFILEID") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				
				if (m_client->IsBanned()) {
					break;
				}
				
				// DbT:FileRequest
				if (size == 16) {
					if (!m_client->GetWaitStartTime()) {
						m_client->SetWaitStartTime();
					}

					CKnownFile *reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if ( reqfile == NULL ) {
						reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
						if ( !( reqfile  != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
							CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16);
							replypacket->Copy16ToDataBuffer(packet);
							theApp.statistics->AddUpDataOverheadFileRequest(replypacket->GetPacketSize());
							SendPacket(replypacket, true);
							break;
						}
					}

					// check to see if this is a new file they are asking for
					if(md4cmp(m_client->GetUploadFileID(), packet) != 0) {
						m_client->SetCommentDirty();
					}

					m_client->SetUploadFileID(reqfile);
					// send filestatus
					CSafeMemFile data(16+16);
					data.WriteHash16(reqfile->GetFileHash());
					if (reqfile->IsPartFile()) {
						((CPartFile*)reqfile)->WritePartStatus(&data);
					} else {
						data.WriteUInt16(0);
					}
					CPacket* packet = new CPacket(&data);
					packet->SetOpCode(OP_FILESTATUS);
					
					theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
					SendPacket(packet, true);
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;
				// DbT:End
			}			
			
			case OP_FILEREQANSNOFIL: {	// 0.43b protocol, lacks ZZ's download manager on swap
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILEREQANSNOFIL") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (size == 16)
				{
					// if that client does not have my file maybe has another different
					CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
					if ( reqfile) {
						reqfile->AddDeadSource( m_client );
					} else {
						break;
					}
						
					// we try to swap to another file ignoring no needed parts files
					switch (m_client->GetDownloadState())
					{
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
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQFILENAMEANSWER") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				const CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				m_client->ProcessFileInfo(&data, file);
				break;
			}
			
			case OP_FILESTATUS: {		// 0.43b except check for bad clients
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILESTATUS") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				const CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				m_client->ProcessFileStatus(false, &data, file);
				break;
			}
			
			case OP_STARTUPLOADREQ: {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_STARTUPLOADREQ") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
			
				if (!m_client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode)) {
					break;
				}
				
				m_client->CheckForAggressive();
				if ( m_client->IsBanned() ) {
					break;
				}
				
				if (size == 16) {
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if (reqfile) {
						if (md4cmp(m_client->GetUploadFileID(), packet) != 0) {
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
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_QUEUERANK") );
				 
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uint32 rank = data.ReadUInt32();
				
				m_client->SetRemoteQueueRank(rank);
				break;
			}
			
			case OP_ACCEPTUPLOADREQ: {	// 0.42e (xcept khaos stats)
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ACCEPTUPLOADREQ") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					if (m_client->GetDownloadState() == DS_ONQUEUE ) {
						m_client->SetDownloadState(DS_DOWNLOADING);
						m_client->m_lastPartAsked = 0xffff; // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
						m_client->SendBlockRequests();
					}
				} else {
					if (!m_client->GetSentCancelTransfer()) {
						CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
						theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
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
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTPARTS") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);

				CSafeMemFile data((BYTE*)packet,size);

				CMD4Hash reqfilehash;
				data.ReadHash16(reqfilehash);

				uint32 auStartOffsets[3];
				auStartOffsets[0] = data.ReadUInt32();
				auStartOffsets[1] = data.ReadUInt32();
				auStartOffsets[2] = data.ReadUInt32();

				uint32 auEndOffsets[3];
				auEndOffsets[0] = data.ReadUInt32();
				auEndOffsets[1] = data.ReadUInt32();
				auEndOffsets[2] = data.ReadUInt32();


				for (int i = 0; i < ARRSIZE(auStartOffsets); i++) {
					if (auEndOffsets[i] > auStartOffsets[i]) {
						Requested_Block_Struct* reqblock = new Requested_Block_Struct;
						reqblock->StartOffset = auStartOffsets[i];
						reqblock->EndOffset = auEndOffsets[i];
						md4cpy(reqblock->FileID, reqfilehash);
						reqblock->transferred = 0;
						m_client->AddReqBlock(reqblock);
					} else {
						if ( CLogger::IsEnabled( logClient ) ) {
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0) {
									AddLogLineM( false, wxString::Format(_("Client requests invalid %u. file block %u-%u (%d bytes): "), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i])  + m_client->GetFullIP());
								}
							}
						}
					}
					break;
			}
			
			case OP_CANCELTRANSFER: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CANCELTRANSFER") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				theApp.uploadqueue->RemoveFromUploadQueue(m_client);
				if ( CLogger::IsEnabled( logClient ) ) {
					AddDebugLogLineM( false, logClient, m_client->GetUserName() + wxT(": Upload session ended due canceled transfer."));
				}
				break;
			}
			
			case OP_END_OF_DOWNLOAD: { // 0.43b except check for bad clients
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_END_OF_DOWNLOAD") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (size>=16 && !md4cmp(m_client->GetUploadFileID(),packet)) {
					theApp.uploadqueue->RemoveFromUploadQueue(m_client);
					if ( CLogger::IsEnabled( logClient ) ) {
						AddDebugLogLineM( false, logClient, m_client->GetUserName() + wxT(": Upload session ended due ended transfer."));
					}
				}
				break;
			}
			
			case OP_HASHSETREQUEST: {		// 0.43b except check for bad clients
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HASHSETREQUEST") );
				
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
				}
				m_client->SendHashsetPacket((uchar*)packet);
				break;
			}
			
			case OP_HASHSETANSWER: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_HASHSETANSWER") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				m_client->ProcessHashSet(packet,size);
				break;
			}
			
			case OP_SENDINGPART: {			// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SENDINGPART") );
				
				if (	 m_client->GetRequestFile() && 
					!m_client->GetRequestFile()->IsStopped() && 
					(m_client->GetRequestFile()->GetStatus() == PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					m_client->ProcessBlockPacket(packet,size);
					if ( 	m_client && 
						( m_client->GetRequestFile()->IsStopped() || 
						  m_client->GetRequestFile()->GetStatus() == PS_PAUSED || 
						  m_client->GetRequestFile()->GetStatus() == PS_ERROR) ) {
						if (!m_client->GetSentCancelTransfer()) {
							CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
							theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
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
						CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
						theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						m_client->SendPacket(packet,true,true);
						
						// Socket might die because of SendPacket, so check
						m_client->SetSentCancelTransfer(1);
					}
					m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			
			case OP_OUTOFPARTREQS: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_OUTOFPARTREQS") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (m_client->GetDownloadState() == DS_DOWNLOADING) {
					m_client->SetDownloadState(DS_ONQUEUE);
				}
				break;
			}
			
			case OP_CHANGE_CLIENT_ID: { 	// 0.43b (xcept the IDHybrid)
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CHANGE_CLIENT_ID") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				CSafeMemFile data((BYTE*)packet, size);
				uint32 nNewUserID = data.ReadUInt32();
				uint32 nNewServerIP = data.ReadUInt32();
				
				if (IsLowIDED2K(nNewUserID)) { // client changed server and gots a LowID
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						#warning Here should be the IDHybrid, but we don't use it yet and I'm afraid it will break a lot ;)
						m_client->SetUserID(nNewUserID); // update UserID only if we know the server
						m_client->SetServerIP(nNewServerIP);
						m_client->SetServerPort(pNewServer->GetPort());
					}
				} else if (nNewUserID == m_client->GetIP()) { // client changed server and gots a HighID(IP)
					#warning Here should be the IDHybrid, but we don't use it yet and I'm afraid it will break a lot ;)					
					m_client->SetUserID(nNewUserID);
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						m_client->SetServerIP(nNewServerIP);
						m_client->SetServerPort(pNewServer->GetPort());
					}
				} 
				
				break;
			}					
			
			case OP_CHANGE_SLOT:{	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_CHANGE_SLOT") );
				
				// sometimes sent by Hybrid
				theApp.statistics->AddDownDataOverheadOther(size);
				break;
			}			
			
			case OP_MESSAGE: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MESSAGE") );
				
				AddLogLineM( true, _("New message from '") + m_client->GetUserName() + _("' (IP:") + m_client->GetFullIP() + wxT(")"));
				theApp.statistics->AddDownDataOverheadOther(size);
				
				CSafeMemFile message_file((BYTE*)packet,size);

				//filter me?
				wxString message = message_file.ReadString(m_client->GetUnicodeSupport());
				if (IsMessageFiltered(message, m_client)) {
					if (!m_client->m_bMsgFiltered) {
						AddLogLineM( true, _("Message filtered from '") + m_client->GetUserName() + _("' (IP:") + m_client->GetFullIP() + wxT(")"));
					}
					m_client->m_bMsgFiltered=true;
				} else {
					Notify_ChatProcessMsg(GUI_ID(m_client->GetIP(),m_client->GetUserPort()), m_client->GetUserName() + wxT("|") + message);
				}
				break;
			}
			
			case OP_ASKSHAREDFILES:	{	// 0.43b (well, er, it does the same, but in our own way)
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILES") );
				
				// client wants to know what we have in share, let's see if we allow him to know that
				theApp.statistics->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				CList<void*,void*> list;
				if (thePrefs::CanSeeShares()==vsfaEverybody || (thePrefs::CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
					CKnownFileMap& filemap = theApp.sharedfiles->m_Files_map;
					for (CKnownFileMap::iterator pos = filemap.begin();pos != filemap.end(); pos++ ) {
						list.AddTail((void*&)pos->second);
					}
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list -> %s"),m_client->GetUserID(),_("Accepted")));
				} else {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list -> %s"),m_client->GetUserID(),_("Denied")));
				}
				// now create the memfile for the packet
				CSafeMemFile tempfile(80);
				tempfile.WriteUInt32(list.GetCount());
				while (list.GetCount()) {
					theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, NULL, m_client);
					list.RemoveHead();
				}
				
				// create a packet and send it
				CPacket* replypacket = new CPacket(&tempfile);
				replypacket->SetOpCode(OP_ASKSHAREDFILESANSWER);
				theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
				SendPacket(replypacket, true, true);
				break;
			}
			
			case OP_ASKSHAREDFILESANSWER: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESANSWER") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				wxString EmptyStr;
				m_client->ProcessSharedFileList(packet,size,EmptyStr);
				break;
			}
			
			case OP_ASKSHAREDDIRS: { 		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRS") );
				
 				theApp.statistics->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				if ((thePrefs::CanSeeShares()==vsfaEverybody) || ((thePrefs::CanSeeShares()==vsfaFriends) && m_client->IsFriend())) {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your shareddirectories-list -> "),m_client->GetUserID()) + _("accepted"));

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
					CSafeMemFile tempfile(80);

					uDirs = folders_to_send.GetCount();
					tempfile.WriteUInt32(uDirs);
					for (uint32 iDir=0; iDir < uDirs; iDir++) {
						tempfile.WriteString(folders_to_send[iDir]);
					}

					CPacket* replypacket = new CPacket(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDDIRSANS);
					theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your shareddirectories-list -> "),m_client->GetUserID()) + _("denied"));
					CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0);
					theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}

				break;
			}
			
			case OP_ASKSHAREDFILESDIR: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIR") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				CSafeMemFile data((uchar*)packet, size);
											
				wxString strReqDir = data.ReadString(m_client->GetUnicodeSupport());
				if (thePrefs::CanSeeShares()==vsfaEverybody || (thePrefs::CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list for directory "),m_client->GetUserID()) + strReqDir + wxT(" -> ") + _("accepted"));
					wxASSERT( data.GetPosition() == data.GetLength() );
					CTypedPtrList<CPtrList, CKnownFile*> list;
					
					if (strReqDir == OP_INCOMPLETE_SHARED_FILES) {
						// get all shared files from download queue
						int iQueuedFiles = theApp.downloadqueue->GetFileCount();
						for (int i = 0; i < iQueuedFiles; i++) {
							CPartFile* pFile = theApp.downloadqueue->GetFileByIndex(i);
							if (pFile == NULL || pFile->GetStatus(true) != PS_READY) {
								break;
							}
							list.AddTail(pFile);
						}
					} else {
						theApp.sharedfiles->GetSharedFilesByDirectory(strReqDir,list);
					}

					CSafeMemFile tempfile(80);
					tempfile.WriteString(strReqDir);
					tempfile.WriteUInt32(list.GetCount());
					while (list.GetCount()) {
						theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, NULL, m_client);
						list.RemoveHead();
					}
					
					CPacket* replypacket = new CPacket(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDFILESDIRANS);
					theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list for directory "),m_client->GetUserID()) + strReqDir + wxT(" -> ") + _("denied"));
					
					CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0);
					theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}
				break;
			}		
			
			case OP_ASKSHAREDDIRSANS:{		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRSANS") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				if (m_client->GetFileListRequested() == 1){
					CSafeMemFile data((uchar*)packet, size);
					uint32 uDirs = data.ReadUInt32();
					for (uint32 i = 0; i < uDirs; i++){
						wxString strDir = data.ReadString(m_client->GetUnicodeSupport());
						AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) shares directory "),m_client->GetUserID()) + strDir);
				
						CSafeMemFile tempfile(80);
						tempfile.WriteString(strDir);
						CPacket* replypacket = new CPacket(&tempfile);
						replypacket->SetOpCode(OP_ASKSHAREDFILESDIR);
						theApp.statistics->AddUpDataOverheadOther(replypacket->GetPacketSize());
						SendPacket(replypacket, true, true);
					}
					wxASSERT( data.GetPosition() == data.GetLength() );
					m_client->SetFileListRequested(uDirs);
				} else {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) sent unasked shared dirs."),m_client->GetUserID()));
				}
      			break;
      		}
      
			case OP_ASKSHAREDFILESDIRANS: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIRANS") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				CSafeMemFile data((uchar*)packet, size, 0);
				wxString strDir = data.ReadString(m_client->GetUnicodeSupport());

				if (m_client->GetFileListRequested() > 0){
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) sent sharedfiles-list for directory "),m_client->GetUserID()) + strDir);
					
					m_client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), strDir);
					if (m_client->GetFileListRequested() == 0) {
						AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) finished sending sharedfiles-list"),m_client->GetUserID()));
					}
				} else {
					AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) sent unwanted sharedfiles-list"),m_client->GetUserID()));
				}
				break;
			}
			
			case OP_ASKSHAREDDENIEDANS:
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDENIEDANS" ) );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				AddLogLineM( true, _("User ") + m_client->GetUserName() + wxString::Format(_(" (%u) denied access to shareddirectories/files-list"),m_client->GetUserID()));
				m_client->SetFileListRequested(0);			
				break;
			
			default:
				theApp.statistics->AddDownDataOverheadOther(size);
				AddDebugLogLineM( false, logRemoteClient, wxString::Format(wxT("Edonkey packet: unknown opcode: %i %x"), opcode, opcode) );
				break;
		}
	}
	catch (const CInvalidPacket& ErrorPacket) {
		if ( CLogger::IsEnabled( logPacketErrors ) ) {
			printf(	"\tCaught InvalidPacket exception:\n"
				"\t\tError: %s\n"
				"\t\tClientData: %s\n"
				"\ton ListenSocket::ProcesstPacket\n",
				strlen(ErrorPacket.what()) ? ErrorPacket.what() : "Unknown",
				m_client ? (const char *)unicode2char(m_client->GetClientFullInfo()) : "Unknown");
			if (m_client) {
				m_client->SetDownloadState(DS_ERROR);
			}
		}
		
		Disconnect(wxT("UnCaught invalid packet exception On ProcessPacket\n"));
		return false;
	} catch (const wxString& error) {
		if ( CLogger::IsEnabled( logPacketErrors ) ) {
			printf(	"\tCaught error:\n"
				"\t\tError: %s\n"
				"\t\tClientData: %s\n"
				"\ton ListenSocket::ProcessPacket\n",
				error.IsEmpty() ? "Unknown" : (const char *)unicode2char(error),
				m_client ? (const char *)unicode2char(m_client->GetClientFullInfo()) : "Unknown");
		}
		AddDebugLogLineM
			(
			false,
			logClient, 
			(	m_client ?
				wxT("Client '") + m_client->GetUserName() + wxT(" (IP:") +
					m_client->GetFullIP() + wxT(")") :
				wxString(wxT("An unknown client"))
			) +
			wxT(" caused an error: ") + error + wxT(". Disconnecting client!")
			);
		if (m_client) {
			m_client->SetDownloadState(DS_ERROR);
		}
		Disconnect(wxT("Client error on ListenSocket::ProcessPacket: ") + wxString(error));
		return false;
	} catch (...) {
		Disconnect(wxT("Unknown exception on ListenSocket::ProcessPacket"));
	}
	return true;
}

bool CClientReqSocket::ProcessExtPacket(const char* packet, uint32 size, uint8 opcode)
{
	#ifdef __PACKET_RECV_DUMP__
	printf("Rec: OPCODE %x \n",opcode);
	DumpMem(packet,size);
	#endif
	// 0.42e - except the catchs on mem exception and file exception
	try{
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
			case OP_MULTIPACKET: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKET") );

				theApp.statistics->AddDownDataOverheadFileRequest(size);

				if (m_client->IsBanned()) {
					break;
				}

				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_MULTIPACKET before finishing handshake"));
				}

				CSafeMemFile data_in((BYTE*)packet,size);
				CMD4Hash reqfilehash;
				data_in.ReadHash16(reqfilehash);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if ( reqfile == NULL ){
					reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
					if ( !( reqfile != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
						CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16);
						replypacket->Copy16ToDataBuffer(packet);
						theApp.statistics->AddUpDataOverheadFileRequest(replypacket->GetPacketSize());
						SendPacket(replypacket, true);
						break;
					}
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
				CSafeMemFile data_out(128);
				data_out.WriteHash16(reqfile->GetFileHash());
				while(data_in.GetLength()-data_in.GetPosition()) {
					uint8 opcode_in = data_in.ReadUInt8();
					switch(opcode_in) {
						case OP_REQUESTFILENAME: {
							m_client->ProcessExtendedInfo(&data_in, reqfile);
							data_out.WriteUInt8(OP_REQFILENAMEANSWER);
							data_out.WriteString(reqfile->GetFileName());
							break;
						}
						case OP_AICHFILEHASHREQ: {
							if (m_client->IsSupportingAICH() && reqfile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
								&& reqfile->GetAICHHashset()->HasValidMasterHash())
							{
								data_out.WriteUInt8(OP_AICHFILEHASHANS);
								reqfile->GetAICHHashset()->GetMasterHash().Write(&data_out);
							}
							break;
						}						
						case OP_SETREQFILEID: {
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
							//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
							if (m_client->GetSourceExchangeVersion() > 1) {
								//data_out.WriteUInt8(OP_ANSWERSOURCES);
								DWORD dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
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
										theApp.statistics->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
										SendPacket(tosend, true);
									}
								}
							}
							break;
						}
					}
				}
				if( data_out.GetLength() > 16 ) {
					CPacket* reply = new CPacket(&data_out, OP_EMULEPROT);
					reply->SetOpCode(OP_MULTIPACKETANSWER);
					theApp.statistics->AddUpDataOverheadFileRequest(reply->GetPacketSize());
					SendPacket(reply, true);
				}
				break;
			}

			case OP_MULTIPACKETANSWER: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_MULTIPACKETANSWER") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);

				if (m_client->IsBanned()) {
					break;
				}
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_MULTIPACKETANSWER before finishing handshake"));
				}
				
				CSafeMemFile data_in((BYTE*)packet,size);
				CMD4Hash reqfilehash;
				data_in.ReadHash16(reqfilehash);
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
				while((data_in.GetLength()-data_in.GetPosition()) && m_client)
				{
					uint8 opcode_in = data_in.ReadUInt8();
					switch(opcode_in)
					{
						case OP_REQFILENAMEANSWER:
						{
							m_client->ProcessFileInfo(&data_in, reqfile);
							break;
						}
						case OP_FILESTATUS:
						{
							m_client->ProcessFileStatus(false, &data_in, reqfile);
							break;
						}
						case OP_AICHFILEHASHANS:
						{
							m_client->ProcessAICHFileHash(&data_in, reqfile);
							break;
						}					
					}
				}
				break;
			}
		
			case OP_EMULEINFO: {	// 0.43b
				theApp.statistics->AddDownDataOverheadOther(size);

				if (!m_client->ProcessMuleInfoPacket(packet,size)) { 
					AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFO") );
					
					// If it's not a OS Info packet, is an old client
					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (m_client->GetInfoPacketsReceived() == IP_BOTH) {	
						m_client->InfoPacketsReceived();
					}
					m_client->SendMuleInfoPacket(true);
				} else {
					AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFO is a OS_INFO") );
				}
				break;
			}
			case OP_EMULEINFOANSWER: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_EMULEINFOANSWER") );
				theApp.statistics->AddDownDataOverheadOther(size);
				
				m_client->ProcessMuleInfoPacket(packet,size);
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				
				if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
					m_client->InfoPacketsReceived();				
				}
				
				break;
			}
			
			case OP_SECIDENTSTATE:{		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SECIDENTSTATE") );
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_SECIDENTSTATE before finishing handshake"));
				}								
				m_client->ProcessSecIdentStatePacket((uchar*)packet,size);
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
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PUBLICKEY") );
				
				if (m_client->IsBanned() ){
					break;						
				}
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_PUBLICKEY before finishing handshake"));
				}
												
				m_client->ProcessPublicKeyPacket((uchar*)packet,size);
				break;
			}
 			case OP_SIGNATURE:{			// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_SIGNATURE") );
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				m_client->ProcessSignaturePacket((uchar*)packet,size);
				break;
			}		
			case OP_COMPRESSEDPART: {	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_COMPRESSEDPART") );
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					m_client->ProcessBlockPacket(packet,size,true);
					if (m_client && (
						m_client->GetRequestFile()->IsStopped() ||
						m_client->GetRequestFile()->GetStatus() == PS_PAUSED ||
						m_client->GetRequestFile()->GetStatus() == PS_ERROR)) {
						if (!m_client->GetSentCancelTransfer()) {
							CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
							theApp.statistics->AddUpDataOverheadOther(packet->GetPacketSize());
							m_client->SendPacket(packet,true,true);					
							
							if (m_client)
								m_client->SetSentCancelTransfer(1);
						}

						if ( m_client )
							m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);	
					}
				} else {
					if (!m_client->GetSentCancelTransfer()) {
						CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
						theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						m_client->SendPacket(packet,true,true);
						
						if ( m_client )
							m_client->SetSentCancelTransfer(1);
					}
				
					if ( m_client )
						m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			
			case OP_QUEUERANKING: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_QUEUERANKING") );
				
				theApp.statistics->AddDownDataOverheadOther(size);
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_QUEUERANKING before finishing handshake"));
				}
				
				if (size != 12) {
					throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
				}

				uint16 newrank = PeekUInt16(packet);
				m_client->SetRemoteQueueFull(false);
				m_client->SetRemoteQueueRank(newrank);
				break;
			}
 			
			case OP_REQUESTSOURCES:{	// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTSOURCES") );
				
				theApp.statistics->AddDownDataOverheadSourceExchange(size);

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
					CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if(!file) {
						file = theApp.downloadqueue->GetFileByID((uchar*)packet);
					}
					if(file) {
						DWORD dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
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
								theApp.statistics->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
								SendPacket(tosend, true, true);
							}
						}
					}
				}
				break;
			}
 			
			case OP_ANSWERSOURCES: {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_ANSWERSOURCES") );
				
				theApp.statistics->AddDownDataOverheadSourceExchange(size);

				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_ANSWERSOURCES before finishing handshake"));
				}
				
				CSafeMemFile data((BYTE*)packet,size);
				uchar hash[16];
				data.ReadHash16(hash);
				const CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
				if(file){
					if (file->IsPartFile()){
						//set the client's answer time
						m_client->SetLastSrcAnswerTime();
						//and set the file's last answer time
						((CPartFile*)file)->SetLastAnsweredTime();

						((CPartFile*)file)->AddClientSources(&data, m_client->GetSourceExchangeVersion());
					}
				}
				break;
			}
			case OP_FILEDESC: {		// 0.43b
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_FILEDESC") );
				
				theApp.statistics->AddDownDataOverheadFileRequest(size);

				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_FILEDESC before finishing handshake"));
				}
				
				m_client->ProcessMuleCommentPacket(packet,size);
				break;
			}

			// Unsupported
			case OP_REQUESTPREVIEW: {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_REQUESTPREVIEW") );
				break;
			}
			// Unsupported
			case OP_PREVIEWANSWER: {
				AddDebugLogLineM( false, logRemoteClient, wxT("Remote Client: OP_PREVIEWANSWER") );
				break;
			}

			case OP_PUBLICIP_ANSWER: {
				theApp.statistics->AddDownDataOverheadOther(size);
				m_client->ProcessPublicIPAnswer((BYTE*)packet,size);
				break;
			}
			case OP_PUBLICIP_REQ: {
				theApp.statistics->AddDownDataOverheadOther(size);
				CPacket* pPacket = new CPacket(OP_PUBLICIP_ANSWER, 4, OP_EMULEPROT);
				pPacket->CopyUInt32ToDataBuffer(m_client->GetIP());
				theApp.statistics->AddUpDataOverheadOther(pPacket->GetPacketSize());
				SendPacket(pPacket);
				break;
			}			
			case OP_AICHANSWER: {
				theApp.statistics->AddDownDataOverheadOther(size);
				m_client->ProcessAICHAnswer(packet,size);
				break;
			}
			case OP_AICHREQUEST: {
				theApp.statistics->AddDownDataOverheadOther(size);
				m_client->ProcessAICHRequest(packet,size);
				break;
			}
			case OP_AICHFILEHASHANS: {
				// those should not be received normally, since we should only get those in MULTIPACKET
				theApp.statistics->AddDownDataOverheadOther(size);
				CSafeMemFile data((BYTE*)packet, size);
				m_client->ProcessAICHFileHash(&data, NULL);
				break;
			}
			case OP_AICHFILEHASHREQ: {
				// those should not be received normally, since we should only get those in MULTIPACKET
				CSafeMemFile data((BYTE*)packet, size);
				uchar abyHash[16];
				data.ReadHash16(abyHash);
				CKnownFile* pPartFile = theApp.sharedfiles->GetFileByID(abyHash);
				if (pPartFile == NULL){
					break;
				}
				
				if (m_client->IsSupportingAICH() && pPartFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
					&& pPartFile->GetAICHHashset()->HasValidMasterHash()) {
					CSafeMemFile data_out;
					data_out.WriteHash16(abyHash);
					pPartFile->GetAICHHashset()->GetMasterHash().Write(&data_out);
					CPacket* packet = new CPacket(&data_out, OP_EMULEPROT, OP_AICHFILEHASHANS);
					theApp.statistics->AddUpDataOverheadOther(packet->GetPacketSize());
					SendPacket(packet);
				}
				break;
			}
			default:
				theApp.statistics->AddDownDataOverheadOther(size);
				AddDebugLogLineM( false, logRemoteClient, wxString::Format(wxT("eMule packet : unknown opcode: %i %x"),opcode,opcode));
				break;
		}
	} catch (const CInvalidPacket& ErrorPacket) {
		if ( CLogger::IsEnabled( logPacketErrors ) ) {
			printf(	"\tCaught InvalidPacket exception:\n"
				"\t\tError: %s\n"
				"\t\tClientData: %s\n"
				"\ton ListenSocket::ProcessExtPacket\n",
				strlen(ErrorPacket.what()) ? ErrorPacket.what() : "Unknown",
				m_client ? (const char *)unicode2char(m_client->GetClientFullInfo()) : "Unknown");
		}
		if (m_client) {
			m_client->SetDownloadState(DS_ERROR);
		}
		Disconnect(wxT("UnCaught invalid packet exception On ProcessPacket\n"));
		return false;
	} catch (const wxString& error) {
		AddDebugLogLineM( false, logClient,
			wxT("A client caused an error or did something bad: ") +
			error + _(". Disconnecting client!"));
		if ( CLogger::IsEnabled( logPacketErrors ) ) {
			printf(	"\tCaught error:\n"
				"\t\tError: %s\n"
				"\t\tClientData: %s\n"
				"\ton ListenSocket::ProcessExtPacket\n",
				error.IsEmpty() ? "Unknown" : (const char *)unicode2char(error),
				m_client ? (const char *)unicode2char(m_client->GetClientFullInfo()) : "Unknown");
		}
		if (m_client) {
			m_client->SetDownloadState(DS_ERROR);
		}
		Disconnect(wxT("Client error on ListenSocket::ProcessExtPacket: ") + error);
		return false;
	} catch (...) {
		Disconnect(wxT("Unknown exception on ListenSocket::ProcessExtPacket"));
	}

	return true;
}


bool CClientReqSocket::Connect(amuleIPV4Address addr, bool wait) {
	last_action = ACTION_CONNECT;
	return CEMSocket::Connect(addr, wait);
}


void CClientReqSocket::OnConnect(int nErrorCode)
{
	if (nErrorCode) {
		OnError(nErrorCode);
	} else {
		if (!m_client) {
			printf("Couldn't send hello packet (client deleted!)\n");
			// and now? Disconnect? not?			
		} else {
			if (!m_client->SendHelloPacket()) {	
				printf("Couldn't send hello packet (client deleted?)\n");				
				// and now? Disconnect? not?				
			}				
		}
	}
}

void CClientReqSocket::OnSend(int nErrorCode)
{
	last_action = ACTION_SEND;
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnReceive(int nErrorCode)
{
	last_action = ACTION_RECEIVE;
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}

void CClientReqSocket::RepeatLastAction() {
	switch (last_action) {
		case ACTION_NONE:
			break;
		case ACTION_SEND:
			OnSend(0);
		case ACTION_RECEIVE:
			OnReceive(0);
		case ACTION_CONNECT: {
			byConnected = ES_DISCONNECTED;			
			amuleIPV4Address addr;
			if (m_client) {
				addr.Hostname(m_client->GetConnectIP());
				addr.Service(m_client->GetUserPort());
			} else {
				GetPeer(addr);
			}
			Connect(addr,FALSE); // non blocking.
			break;
		}
		default:
			break;
	}
}

void CClientReqSocket::OnError(int nErrorCode)
{
	// 0.42e + Kry changes for handling of socket lost events
	wxString strError;
	
	bool disconnect = true;
	
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
								wxString::Format(wxT(") caused an error: %u. Disconnecting client!"),nErrorCode);
				} else {
					strError = wxT("OnError: Unknown client (IP:") + 
					  m_client->GetFullIP() + wxString::Format(wxT(") caused an error: %u. Disconnecting client!"),nErrorCode);
				}
			} else {
				strError = wxString::Format(wxT("OnError: A client caused an error or did something bad (error %u). Disconnecting client !"),
					nErrorCode);
			}
		} else {
			strError = wxT("Error 107 (Transport endpoint is not connected)");
		}	
	}
	
	if (disconnect) {
		Disconnect(strError);
	}
	
}

bool CClientReqSocket::PacketReceived(CPacket* packet)
{
	// 0.42e
	bool bResult;
	UINT uRawSize = packet->GetPacketSize();	
	AddDebugLogLineM( false, logRemoteClient, wxString( wxT("Packet received from ") ) + ( m_client ? m_client->GetFullIP() : wxT("Unknown Client") ) );
	
	switch (packet->GetProtocol()) {
		case OP_EDONKEYPROT:		
			bResult = ProcessPacket(packet->GetDataBuffer(),uRawSize,packet->GetOpCode());
			break;		
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()) {
				AddDebugLogLineM( false, logZLib, 
					wxString::Format(wxT("Failed to decompress client TCP packet; protocol=0x%02x  opcode=0x%02x  size=%u"),
					packet->GetProtocol(),
					packet->GetOpCode(),
					packet->GetPacketSize()));
				bResult = false;
				break;
			}
		case OP_EMULEPROT:
			bResult = ProcessExtPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
			break;
		default: {
			theApp.statistics->AddDownDataOverheadOther(uRawSize);
			if (m_client) {
				m_client->SetDownloadState(DS_ERROR);
			}
			Disconnect(wxT("Unknown protocol"));
			bResult = false;
		}
	}
	return bResult;
}

bool CClientReqSocket::IsMessageFiltered(wxString Message, CUpDownClient* client) {
	
	bool filtered = false;
	// If we're chatting to the guy, we don't want to filter!
	if (client->GetChatState() != MS_CHATTING) {
		if (thePrefs::MsgOnlyFriends() && !client->IsFriend()) {
			filtered = true;
		} else if (thePrefs::MsgOnlySecure() && client->GetUserName()==wxEmptyString) {
			filtered = true;
		} else if (thePrefs::MustFilterMessages()) {
			if (thePrefs::MessageFilter().IsSameAs(wxT("*"))){  
				// Filter anything
				filtered = true;
			} else {
				wxStringTokenizer tokenizer( thePrefs::MessageFilter(), wxT(",") );
				while (tokenizer.HasMoreTokens() && !filtered) {
					if ( Message.MakeLower().Trim(false).Trim(true).Contains(
							tokenizer.GetNextToken().MakeLower().Trim(false).Trim(true))) {
						filtered = true;
					}
				}
			}
		}
	}
	return filtered;
}

#ifdef AMULE_DAEMON
void CClientReqSocket::Destroy()
{
	if ( !my_handler ) {
		CEMSocket::Destroy();
	}
}
#endif

//-----------------------------------------------------------------------------
// CListenSocket
//-----------------------------------------------------------------------------
//
// This is the socket that listens to incomming connections in aMule's TCP port
// As soon as a connection is detected, it creates a new socket of type 
// CClientReqSocket to handle (accept) the connection.
// 

CListenSocket::CListenSocket(wxIPaddress &addr, const CProxyData *ProxyData)
:
// wxSOCKET_NOWAIT    - means non-blocking i/o
// wxSOCKET_REUSEADDR - means we can reuse the socket imediately (wx-2.5.3)
#ifdef AMULE_DAEMON
CSocketServerProxy(addr, wxSOCKET_WAITALL|wxSOCKET_REUSEADDR, ProxyData),
wxThread(wxTHREAD_JOINABLE) 
#else
CSocketServerProxy(addr, wxSOCKET_NOWAIT|wxSOCKET_REUSEADDR, ProxyData)
#endif
{
	// 0.42e - vars not used by us
	bListening = false;
	shutdown = false;
	maxconnectionreached = 0;
	m_OpenSocketsInterval = 0;
	m_nPeningConnections = 0;
	peakconnections = 0;
	totalconnectionchecks = 0;
	averageconnections = 0.0;
	activeconnections = 0;
	// Set the listen socket event handler -- The handler is written in amule.cpp
	if (Ok()) {
#ifdef AMULE_DAEMON
		if ( Create() != wxTHREAD_NO_ERROR ) {
			AddLogLineM( true, wxT("CListenSocket: can not create my thread") );
		}
		Notify(false);
#else
 		SetEventHandler(theApp, LISTENSOCKET_HANDLER);
 		SetNotify(wxSOCKET_CONNECTION_FLAG);
 		Notify(true);
#endif	
		printf("ListenSocket: Ok.\n");
	} else {
		AddLogLineM( true, wxT("Error: Could not listen to TCP port.") );
		printf("ListenSocket: Could not listen to TCP port.");
	}
}

CListenSocket::~CListenSocket()
{
	shutdown = true;
	Discard();
	Close();
#ifdef AMULE_DAEMON
	AddLogLineM( true, wxT("CListenSocket: destroy") );
	global_sock_thread.Delete();
	global_sock_thread.Wait();
#endif
	KillAllSockets();
}

//
// lfroen - this used only in daemon where sockets are threaded
//
#ifdef AMULE_DAEMON
void *CListenSocket::Entry()
{
	while ( !TestDestroy() ) {
		if ( WaitForAccept(1, 0) ) {
			if ( !theApp.IsReady ) {
				wxSocketBase *s = Accept(false);
				if ( s ) {
					s->Destroy();
				}
				continue;
			}
			if ( bListening ) {
				CALL_APP_DATA_LOCK;
				OnAccept(0);
			} else {
				Sleep(10);
			}
		}
	}
	return 0;
}
#endif

bool CListenSocket::StartListening()
{
	// 0.42e
	bListening = true;
#ifdef AMULE_DAEMON
	Run();
	global_sock_thread.Run();
#endif
	return true;
}

void CListenSocket::ReStartListening()
{
	// 0.42e
	bListening = true;
	if (m_nPeningConnections) {
		m_nPeningConnections--;
		OnAccept(0);
	}
}

void CListenSocket::StopListening()
{
	// 0.42e
	bListening = false;
	maxconnectionreached++;
}

void CListenSocket::OnAccept(int nErrorCode)
{
	// 0.42e
	if (!nErrorCode) {
		m_nPeningConnections++;
		if (m_nPeningConnections < 1) {
			wxASSERT(FALSE);
			m_nPeningConnections = 1;
		}
		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting()) {
			StopListening();
			return;
		} else if (bListening == false) {
			// If the client is still at maxconnections,
			// this will allow it to go above it ...
			// But if you don't, you will get a lowID on all servers.
			ReStartListening();
		}
		// Deal with the pending connections, there might be more than one, due to
		// the StopListening() call above.
		while (m_nPeningConnections) {
			m_nPeningConnections--;
			// Create a new socket to deal with the connection
			CClientReqSocket* newclient = new CClientReqSocket();
			// Accept the connection and give it to the newly created socket
			if (AcceptWith(*newclient, false)) {
				// OnInit currently sets the last_action to ACTION_CONNECT
				newclient->OnInit();
			} else {
				newclient->Safe_Delete();
			}
			AddConnection();
		}
	}
}

void CListenSocket::AddConnection()
{
	m_OpenSocketsInterval++;
}

void CListenSocket::Process()
{
	// 042e + Kry changes for Destroy
	m_OpenSocketsInterval = 0;
	SocketSet::iterator it = socket_list.begin();
	while ( it != socket_list.end() ) {
		CClientReqSocket* cur_socket = *it++;
		if (!cur_socket->OnDestroy()) {
			if (cur_socket->deletethis) {
				cur_socket->Destroy();
			} else {
				cur_socket->CheckTimeOut();
			}
		}
	}
	
	if ((GetOpenSockets()+5 < thePrefs::GetMaxConnections() || theApp.serverconnect->IsConnecting()) && !bListening) {
		ReStartListening();
	}
}

void CListenSocket::RecalculateStats()
{
	// 0.42e
	memset(m_ConnectionStates,0,6);
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ) {
		CClientReqSocket* cur_socket = *it++;
		switch (cur_socket->GetConState()) {
			case ES_DISCONNECTED:
				m_ConnectionStates[0]++;
				break;
			case ES_NOTCONNECTED:
				m_ConnectionStates[1]++;
				break;
			case ES_CONNECTED:
				m_ConnectionStates[2]++;
				break;
		}
	}
}

void CListenSocket::AddSocket(CClientReqSocket* toadd)
{
	wxASSERT(toadd);
	socket_list.insert(toadd);
#ifdef AMULE_DAEMON
	global_sock_thread.AddSocket(toadd);
#endif
}

void CListenSocket::RemoveSocket(CClientReqSocket* todel)
{
	wxASSERT(todel);
	socket_list.erase(todel);
#ifdef AMULE_DAEMON
	global_sock_thread.RemoveSocket(todel);
#endif
}

void CListenSocket::KillAllSockets()
{
	// 0.42e reviewed - they use delete, but our safer is Destroy...
	// But I bet it would be better to call Safe_Delete on the socket.
	// Update: no... Safe_Delete MARKS for deletion. We need to delete it.
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ) {
		CClientReqSocket* cur_socket = *it++;
		if (cur_socket->GetClient()) {
			cur_socket->GetClient()->Safe_Delete();
		} else {
			cur_socket->Safe_Delete();
			cur_socket->Destroy(); 
		}
	}
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval)
{
	if (GetOpenSockets() > thePrefs::GetMaxConnections() || (m_OpenSocketsInterval > (thePrefs::GetMaxConperFive()*GetMaxConperFiveModifier()) && !bIgnoreInterval)) {
		//printf("TOO MANY SOCKETS!\n");
		return true;
	} else {
		return false;
	}
}

bool CListenSocket::IsValidSocket(CClientReqSocket* totest)
{
	// 0.42e
	return socket_list.find(totest) != socket_list.end();
}


void CListenSocket::UpdateConnectionsStatus()
{
	// 0.42e xcept for the khaos stats
	activeconnections = GetOpenSockets();
	if( peakconnections < activeconnections ) {
		peakconnections = activeconnections;
	}
	if( theApp.serverconnect->IsConnected() ) {
		totalconnectionchecks++;
		float percent;
		percent = (float)(totalconnectionchecks-1)/(float)totalconnectionchecks;
		if( percent > .99f ) {
			percent = .99f;
		}
		averageconnections = (averageconnections*percent) + (float)activeconnections*(1.0f-percent);
	}
}


float CListenSocket::GetMaxConperFiveModifier()
{
	float SpikeSize = GetOpenSockets() - averageconnections;
	if ( SpikeSize < 1 ) {
		return 1;
	}

	float SpikeTolerance = 2.5f*thePrefs::GetMaxConperFive();
	if ( SpikeSize > SpikeTolerance ) {
		return 0;
	}
	
	return 1.0f - (SpikeSize/SpikeTolerance);
}


#ifdef AMULE_DAEMON

CSocketGlobalThread::CSocketGlobalThread() : wxThread(wxTHREAD_JOINABLE)
{
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM( true, _("CSocketGlobalThread: call to Create failed") );
	}
}

void CSocketGlobalThread::AddSocket(CClientReqSocket* sock)
{
	wxASSERT(sock);
	socket_list.insert(sock);
}

void CSocketGlobalThread::RemoveSocket(CClientReqSocket* sock)
{
	wxASSERT(sock);
	socket_list.erase(sock);
}


void *CSocketGlobalThread::Entry()
{
	while ( !TestDestroy() ) {
		Sleep(10);
		std::set<CClientReqSocket *>::iterator it;
		CALL_APP_DATA_LOCK;
		it = socket_list.begin();
		while (it != socket_list.end()) {
			CClientReqSocket* cur_sock = *it++;
			if (cur_sock->deletethis) {
				socket_list.erase(cur_sock);
				continue;
			}
			if (cur_sock->Error()) {
				switch (cur_sock->LastError()) {
					case wxSOCKET_WOULDBLOCK: 
						if (cur_sock->last_action != ACTION_CONNECT) {
							// Connection state will be handled on next if
							cur_sock->RepeatLastAction();
						}
					default:
						socket_list.erase(cur_sock);				
				}
			}
			if ( !cur_sock->wxSocketBase::IsConnected()) {
					if ( cur_sock->WaitOnConnect(0, 0) ) {
						cur_sock->OnConnect(0);
					}
			} else {
				if ( cur_sock->deletethis ) { // Must we remove this socket?
					socket_list.erase(cur_sock);
					continue;
				}
				if ( cur_sock->WaitForLost(0, 0) ) { // Did the socket got closed?
					cur_sock->OnError(cur_sock->LastError());
					socket_list.erase(cur_sock);
					continue;
				}
				
				if (cur_sock->WaitForWrite(0, 0) ) { // Are we ready to write to this socket?
					cur_sock->OnSend(0);
				}				
				
				// We re-check deletethis because it could have been triggered on write
				if (!cur_sock->deletethis && cur_sock->WaitForRead(0, 0)) { // Are we ready to read from this socket?
					cur_sock->OnReceive(0);
					CUpDownClient *client = cur_sock->GetClient();
					if ( client && (client->GetDownloadState() == DS_DOWNLOADING)) {
						// If client is downloading, we create a thread for it.
						CClientReqSocketHandler *t = new CClientReqSocketHandler(cur_sock);
						//printf("Socket %p started dload\n", cur_sock);
						socket_list.erase(cur_sock);
						t->Run();
					}
				}
			}
		}
 
	}
	AddLogLineM( false, _("CSocketGlobalThread: exited") );
	return 0;
}

#endif
