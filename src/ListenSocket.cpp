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


#include "ListenSocket.h"	// Interface declarations

#include "amule.h"		// Needed for theApp
#include "otherfunctions.h"	// Needed for md4cpy
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "updownclient.h"	// Needed for CUpDownClient
#include "opcodes.h"		// Needed for CONNECTION_TIMEOUT
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ClientList.h"		// Needed for CClientList
#include "IPFilter.h"		// Needed for CIPFilter
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "Preferences.h"	// Needed for CPreferences
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "packets.h"		// Needed for Packet
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "sockets.h"		// Needed for CServerConnect

#include <wx/listimpl.cpp>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!


//#define DEBUG_REMOTE_CLIENT_PROTOCOL


WX_DEFINE_OBJARRAY(ArrayOfwxStrings);


IMPLEMENT_DYNAMIC_CLASS(CClientReqSocket,CEMSocket)

CClientReqSocket::CClientReqSocket()
{
}

CClientReqSocket::CClientReqSocket(CPreferences* in_prefs, CUpDownClient* in_client)
{
	app_prefs = in_prefs;
	m_client = in_client;
	if (m_client) {
		m_client->SetSocket(this);
	}
	theApp.listensocket->AddSocket(this);
	ResetTimeOutTimer();
	deletethis = false;
#ifdef AMULE_DAEMON
	my_handler = 0;
	Notify(false);
#else
	my_handler = new CClientReqSocketHandler(this);
	SetEventHandler(*my_handler, CLIENTREQSOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG|
		wxSOCKET_INPUT_FLAG|
		wxSOCKET_OUTPUT_FLAG|
		wxSOCKET_LOST_FLAG);
	Notify(true);
#endif
}

void CClientReqSocket::OnInit()
{
}

bool CClientReqSocket::Close()
{
	return wxSocketBase::Close();
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

	if (theApp.listensocket) {
		#warning check closing method to change order and get rid of this
		theApp.listensocket->RemoveSocket(this);
	}
#ifdef AMULE_DAEMON
	if ( my_handler ) {
		Safe_Delete();
	}
#else
	delete my_handler;
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

void CClientReqSocket::OnClose(int nErrorCode)
{
	// 0.42x
	wxASSERT (theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);
	if (nErrorCode > 0) {
		wxString strError;
		strError.Printf(wxT("Closed: %u"),nErrorCode);
		Disconnect(strError);
	} else {
		Disconnect(wxT("Close"));
	}
}

void CClientReqSocket::Disconnect(const wxString& strReason){
	//AsyncSelect(0);

	byConnected = ES_DISCONNECTED;

	if (m_client) {
		if (m_client->Disconnected(strReason, true)) {
			m_client->SetSocket( NULL );
			m_client->Safe_Delete();
		} 
		m_client = NULL;
	}

	if (!OnDestroy()) {
		Safe_Delete();
	}
};

/* Kry - this eMule function has no use for us, because we have Destroy()

void CClientReqSocket::Delete_Timed()
{
	// it seems that MFC Sockets call socketfunctions after they are deleted,
	// even if the socket is closed and select(0) is set.
	// So we need to wait some time to make sure this doesn't happens
	if (::GetTickCount() - deltimer > 30000) {
		delete this;
	}
}
*/
void CClientReqSocket::Safe_Delete()
{
	//wxASSERT(!deletethis); 
	
	if (!deletethis) {
		//theApp.AddSocketDeleteDebug((uint32) this,created);
		// Paranoia is back.
		SetNotify(0);
		Notify(false);
		// lfroen: first of all - stop handler
		deletethis = true;
#ifdef AMULE_DAEMON
                if ( my_handler ) {
                        printf("CClientReqSocket: sock %p in Safe_Delete\n", this);
                        // lfroen: this code is executed with app mutex locked. In order
                        // to prevent deadlock in deleted thread, temporary unlock it here
                        theApp.data_mutex.Unlock();
                        my_handler->Delete();
                        theApp.data_mutex.Lock();
                }
#endif

		if (m_client) {
			m_client->SetSocket( NULL );
			m_client = NULL;
		}
		byConnected = ES_DISCONNECTED;
		Close();
	}
}

bool CClientReqSocket::ProcessPacket(const char* packet, uint32 size, uint8 opcode)
{
	try{
		if (!m_client && opcode != OP_HELLO) {
			throw wxString(wxT("Asks for something without saying hello"));
		} else if (m_client && opcode != OP_HELLO && opcode != OP_HELLOANSWER)
			m_client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode);
		
		switch(opcode) {
			case OP_HELLOANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_HELLOANSWER\n"));
				#endif
				// 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessHelloAnswer(packet,size);

				// start secure identification, if
				//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
				//	- we have received eMule-OP_HELLOANSWER (new eMule)
				if (m_client->GetInfoPacketsReceived() == IP_BOTH)
					m_client->InfoPacketsReceived();
				
				// Socket might die because of sending in InfoPacketsReceived, so check
				if (m_client) {
					m_client->ConnectionEstablished();
					//theApp.amuledlg->transferwnd->clientlistctrl.RefreshClient(client);
				}
				
				break;
			}
			case OP_HELLO: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_HELLO\n"));
				#endif
				// 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
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
					AddDebugLogLineM(true,_("Filtered IP: ") + m_client->GetFullIP() + wxT("(") + theApp.ipfilter->GetLastHit() + wxT(")"));					
					theApp.stat_filteredclients++;
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
				//theApp.amuledlg->transferwnd->clientlistctrl.RefreshClient(client);
				// send a response packet with standart informations
				if ((m_client->GetHashType() == SO_EMULE) && !bIsMuleHello) {
					m_client->SendMuleInfoPacket(false);				
				}
				
				// Client might die from Sending in SendMuleInfoPacket, so check
				if ( m_client )
					m_client->SendHelloAnswer();
				
				// Client might die from Sending in SendHelloAnswer, so check
				if ( m_client )
					m_client->ConnectionEstablished();
				
				// start secure identification, if
				//	- we have received eMule-OP_HELLO (new eMule)				
				if (m_client && m_client->GetInfoPacketsReceived() == IP_BOTH) {
						m_client->InfoPacketsReceived();				
				}
				
				break;
			}
			case OP_REQUESTFILENAME: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_REQUESTFILENAME\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileRequest", m_client, packet);				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
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
						if (theApp.glob_prefs->GetMaxSourcePerFile() > 
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
					Packet* packet = new Packet(&data_out);
					packet->SetOpCode(OP_REQFILENAMEANSWER);
					#ifdef __USE__DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__FileReqAnswer", m_client, (char*)reqfile->GetFileHash());
					}
					#endif
					theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
					SendPacket(packet,true);
	
					// SendPacket might kill the socket, so check
					if (m_client)
						m_client->SendCommentInfo(reqfile);

					break;
				}
				throw wxString(wxT("Invalid OP_REQUESTFILENAME packet size"));
				break;  
			}
			case OP_SETREQFILEID: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_SETREQFILEID\n"));
				#endif
				// 0.43b EXCEPT track of bad clients
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__SetReqfileID", m_client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				
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
							// send file request no such file packet (0x48)
							#ifdef __USE_DEBUG__
							if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    				DebugSend("OP__FileReqAnsNoFil", m_client);
							}
							#endif				
							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
							replypacket->Copy16ToDataBuffer(packet);
							theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->GetPacketSize());
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
					Packet* packet = new Packet(&data);
					packet->SetOpCode(OP_FILESTATUS);
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
		    				DebugSend("OP__FileStatus", m_client);
					}
					#endif				
					theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
					SendPacket(packet, true);
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;
				// DbT:End
			}			
			case OP_FILEREQANSNOFIL: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_FILEREQANSNOFIL\n"));
				#endif
				// 0.43b protocol, lacks ZZ's download manager on swap
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileReqAnsNoFil", m_client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size == 16)
				{
					// if that client does not have my file maybe has another different
					const CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
					if (!reqfile) {
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
			case OP_REQFILENAMEANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_REQFILENAMEANSWER\n"));
				#endif
				// 0.43b except check for bad clients
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileReqAnswer", m_client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				const CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				m_client->ProcessFileInfo(&data, file);
				break;
			}
			case OP_FILESTATUS: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_FILESTATUS\n"));
				#endif
				// 0.43b except check for bad clients
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileStatus", m_client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				const CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				m_client->ProcessFileStatus(false, &data, file);
				break;
			}
			case OP_STARTUPLOADREQ: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_STARTUPLOADREQ\n"));
				#endif
				// 0.43b except check for bad clients. Xaignar please review.
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_StartUpLoadReq", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
			
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
			case OP_QUEUERANK: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_QUEUERANK\n"));
				#endif
				// 0.43b 
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_QueueRank", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uint32 rank = data.ReadUInt32();
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					Debug("  %u (prev. %d)\n", rank, m_client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)m_client->GetRemoteQueueRank());
				}
				#endif
				m_client->SetRemoteQueueRank(rank);
				break;
			}
			case OP_ACCEPTUPLOADREQ: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ACCEPTUPLOADREQ\n"));
				#endif
				// 0.42e (xcept khaos stats)
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugRecv("OP_AcceptUploadReq", m_client, size >= 16 ? packet : NULL);
					if (size > 0)
						Debug("  ***NOTE: Packet contains %u additional bytes\n", size);
					Debug("  QR=%d\n", m_client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)m_client->GetRemoteQueueRank());
				}				
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					if (m_client->GetDownloadState() == DS_ONQUEUE ) {
						m_client->SetDownloadState(DS_DOWNLOADING);
						m_client->m_lastPartAsked = 0xffff; // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
						m_client->SendBlockRequests();
					}
				} else {
					if (!m_client->GetSentCancelTransfer()) {
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__CancelTransfer", m_client);
						}
						#endif
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
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
			case OP_REQUESTPARTS: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_REQUESTPARTS\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_RequestParts", m_client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

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


				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){
						Debug("  Start1=%u  End1=%u  Size=%u\n", auStartOffsets[0], auEndOffsets[0], auEndOffsets[0] - auStartOffsets[0]);
						Debug("  Start2=%u  End2=%u  Size=%u\n", auStartOffsets[1], auEndOffsets[1], auEndOffsets[1] - auStartOffsets[1]);
						Debug("  Start3=%u  End3=%u  Size=%u\n", auStartOffsets[2], auEndOffsets[2], auEndOffsets[2] - auStartOffsets[2]);
				}
				#endif
				
				for (int i = 0; i < ARRSIZE(auStartOffsets); i++) {
					if (auEndOffsets[i] > auStartOffsets[i]) {
						Requested_Block_Struct* reqblock = new Requested_Block_Struct;
						reqblock->StartOffset = auStartOffsets[i];
						reqblock->EndOffset = auEndOffsets[i];
						md4cpy(reqblock->FileID, reqfilehash);
						reqblock->transferred = 0;
						m_client->AddReqBlock(reqblock);
					} else {
						if (theApp.glob_prefs->GetVerbose()) {
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0) {
									AddLogLineM(false,wxString::Format(_("Client requests invalid %u. file block %u-%u (%d bytes): "), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i])  + m_client->GetFullIP());
								}
							}
						}
					}
					break;
			}
			case OP_CANCELTRANSFER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_CANCELTRANSFER\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_CancelTransfer", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				theApp.uploadqueue->RemoveFromUploadQueue(m_client);
				if (theApp.glob_prefs->GetVerbose()) {
					AddDebugLogLineM(false, m_client->GetUserName() + _(": Upload session ended due canceled transfer."));
				}
				break;
			}
			case OP_END_OF_DOWNLOAD: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_END_OF_DOWNLOAD\n"));
				#endif
				// 0.43b except check for bad clients
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_EndOfDownload", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size>=16 && !md4cmp(m_client->GetUploadFileID(),packet)) {
					theApp.uploadqueue->RemoveFromUploadQueue(m_client);
					if (theApp.glob_prefs->GetVerbose()) {
						AddDebugLogLineM(false, m_client->GetUserName() + _(": Upload session ended due ended transfer."));
					}
				} else {
					// m_client->CheckFailedFileIdReqs((uchar*)packet);
				}
				break;
			}
			case OP_HASHSETREQUEST: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_HASHSETREQUEST\n"));
				#endif
				// 0.43b except check for bad clients
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_HashSetRequest", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
				}
				m_client->SendHashsetPacket((uchar*)packet);
				break;
			}
			case OP_HASHSETANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_HASHSETANSWER\n"));
				#endif
				// 0.43b 
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_HashSetAnswer", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				m_client->ProcessHashSet(packet,size);
				break;
			}
			case OP_SENDINGPART: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_SENDINGPART\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SendingPart", m_client);
				}
				#endif				
				if (	 m_client->GetRequestFile() && 
					!m_client->GetRequestFile()->IsStopped() && 
					(m_client->GetRequestFile()->GetStatus() == PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					m_client->ProcessBlockPacket(packet,size);
					if ( 	m_client && 
						( m_client->GetRequestFile()->IsStopped() || 
						  m_client->GetRequestFile()->GetStatus() == PS_PAUSED || 
						  m_client->GetRequestFile()->GetStatus() == PS_ERROR) ) {
						if (!m_client->GetSentCancelTransfer()) {
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
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
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						m_client->SendPacket(packet,true,true);
						
						// Socket might die because of SendPacket, so check
						m_client->SetSentCancelTransfer(1);
					}
					m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_OUTOFPARTREQS: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_OUTOFPARTREQS\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__OutOfPartReqs", m_client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (m_client->GetDownloadState() == DS_DOWNLOADING) {
					m_client->SetDownloadState(DS_ONQUEUE);
				}
				break;
			}
			case OP_CHANGE_CLIENT_ID:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_CHANGE_CLIENT_ID\n"));
				#endif
				// 0.43b (xcept the IDHybrid)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__ChangeClientID", m_client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((BYTE*)packet, size);
				uint32 nNewUserID = data.ReadUInt32();
				uint32 nNewServerIP = data.ReadUInt32();
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					Debug("  NewUserID=%u (%08x, %s)  NewServerIP=%u (%08x, %s)\n", nNewUserID, nNewUserID, ipstr(nNewUserID), nNewServerIP, nNewServerIP, ipstr(nNewServerIP));
				}
				#endif
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
				#ifdef __USE_DEBUG__
				else{
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						Debug("***NOTE: OP_ChangedClientID unknown contents\n");
					}
				}
				UINT uAddData = data.GetLength() - data.GetPosition();
				if (uAddData > 0){
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						Debug("***NOTE: OP_ChangedClientID contains add. data %s\n", GetHexDump((uint8*)packet + data.GetPosition(), uAddData));
					}
				}
				#endif				
				break;
			}					
			case OP_CHANGE_SLOT:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_CHANGE_SLOT\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_ChangeSlot", m_client, size>=16 ? packet : NULL);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// sometimes sent by Hybrid
				break;
			}			
			case OP_MESSAGE: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_MESSAGE\n"));
				#endif
				// 0.43b
				// But anyway we changed all this to a simple Read on a mem file.
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_Message", m_client, size>=16 ? packet : NULL);
				}
				#endif
				AddLogLineM(true,wxString(_("New message from '")) + m_client->GetUserName() + _("' (IP:") + m_client->GetFullIP() + wxT(")"));
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				
				CSafeMemFile message_file((BYTE*)packet,size);

				#warning TODO: CHECK MESSAGE FILTERING!
				//filter me?
				if ( (theApp.glob_prefs->MsgOnlyFriends() && !m_client->IsFriend()) ||
					 (theApp.glob_prefs->MsgOnlySecure() && m_client->GetUserName()==wxEmptyString) ) {
					#if 0
					if (!m_client->m_bMsgFiltered) {
						AddLogLineM(true,wxString(_("Message filtered from '")) + m_client->GetUserName() + _("' (IP:") + m_client->GetFullIP() + wxT(")"));
					}
					m_client->m_bMsgFiltered=true;
					#endif
					break;
				}
				wxString message = message_file.ReadString();
				Notify_ChatProcessMsg(m_client, message);
				break;
			}
			case OP_ASKSHAREDFILES:	{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDFILES\n"));
				#endif
				// 0.43b (well, er, it does the same, but in our own way)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFiles", m_client);
				}
				#endif				
				// client wants to know what we have in share, let's see if we allow him to know that
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				CList<void*,void*> list;
				if (theApp.glob_prefs->CanSeeShares()==vsfaEverybody || (theApp.glob_prefs->CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
					CKnownFileMap& filemap = theApp.sharedfiles->m_Files_map;
					for (CKnownFileMap::iterator pos = filemap.begin();pos != filemap.end(); pos++ ) {
						list.AddTail((void*&)pos->second);
					}
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list -> %s"),m_client->GetUserID(),_("Accepted")));
				} else {
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list -> %s"),m_client->GetUserID(),_("Denied")));
				}
				// now create the memfile for the packet
				CSafeMemFile tempfile(80);
				tempfile.WriteUInt32(list.GetCount());
				while (list.GetCount()) {
					theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, false);
					list.RemoveHead();
				}
				// create a packet and send it
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugSend("OP__AskSharedFilesAnswer", m_client);
				}
				#endif
				Packet* replypacket = new Packet(&tempfile);
				replypacket->SetOpCode(OP_ASKSHAREDFILESANSWER);
				theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
				SendPacket(replypacket, true, true);
				break;
			}
			case OP_ASKSHAREDFILESANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDFILESANSWER\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesAnswer", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessSharedFileList(packet,size);
				break;
			}
			case OP_ASKSHAREDDIRS: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDDIRS\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDirectories", m_client);
				}
				#endif
				
 				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				if ((theApp.glob_prefs->CanSeeShares()==vsfaEverybody) || ((theApp.glob_prefs->CanSeeShares()==vsfaFriends) && m_client->IsFriend())) {
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your shareddirectories-list -> "),m_client->GetUserID()) + _("accepted"));			

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
					for (uint32 ix=0;ix<theApp.glob_prefs->GetCatCount();ix++) {
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

					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDirsAnswer", m_client);
					}
					#endif				
					Packet* replypacket = new Packet(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDDIRSANS);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDirsDeniedAnswer", m_client);
					}
					#endif
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your shareddirectories-list -> "),m_client->GetUserID()) + _("denied"));			
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}

				break;
			}
			case OP_ASKSHAREDFILESDIR: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDFILESDIR\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesInDirectory", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (m_client->IsBanned()) {
					break;
				}
				CSafeMemFile data((uchar*)packet, size);
											
				wxString strReqDir = data.ReadString();
				if (theApp.glob_prefs->CanSeeShares()==vsfaEverybody || (theApp.glob_prefs->CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list for directory "),m_client->GetUserID()) + strReqDir + wxT(" -> ") + _("accepted"));			
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
						theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, false);
						list.RemoveHead();
					}
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedFilesInDirectoryAnswer", m_client);
					}
					#endif
					Packet* replypacket = new Packet(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDFILESDIRANS);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) requested your sharedfiles-list for directory "),m_client->GetUserID()) + strReqDir + wxT(" -> ") + _("denied"));			
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDeniedAnswer", m_client);
					}
					#endif
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}
				break;
			}		
			
			case OP_ASKSHAREDDIRSANS:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDDIRSANS\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDirectoriesAnswer", m_client);
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (m_client->GetFileListRequested() == 1){
					CSafeMemFile data((uchar*)packet, size);
					uint32 uDirs = data.ReadUInt32();
					for (uint32 i = 0; i < uDirs; i++){
						wxString strDir = data.ReadString();
						AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) shares directory "),m_client->GetUserID()) + strDir);
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__AskSharedFilesInDirectory", m_client);
						}
						#endif
				
						CSafeMemFile tempfile(80);
						tempfile.WriteString(strDir);
						Packet* replypacket = new Packet(&tempfile);
						replypacket->SetOpCode(OP_ASKSHAREDFILESDIR);
						theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
						SendPacket(replypacket, true, true);
					}
					wxASSERT( data.GetPosition() == data.GetLength() );
					m_client->SetFileListRequested(uDirs);
				} else {
						AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) sent unasked shared dirs."),m_client->GetUserID()));
				}
      			break;
      		}
      
			case OP_ASKSHAREDFILESDIRANS: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDFILESDIRANS\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesInDirectoryAnswer", m_client);			  
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((uchar*)packet, size, 0);
				wxString strDir = data.ReadString();

				if (m_client->GetFileListRequested() > 0){
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) sent sharedfiles-list for directory "),m_client->GetUserID()) + strDir);
					#warning We need a new ProcessSharedFileList that can handle dirs. ___UNICODE___
					m_client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), unicode2char(strDir));
					if (m_client->GetFileListRequested() == 0) {
						AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) finished sending sharedfiles-list"),m_client->GetUserID()));
					}
				} else {
					AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) sent unwanted sharedfiles-list"),m_client->GetUserID()));					
				}
				break;
			}
			case OP_ASKSHAREDDENIEDANS:
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ASKSHAREDDENIEDANS\n"));
				#endif
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDeniedAnswer", m_client);
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				AddLogLineM(true,wxString(_("User ")) + m_client->GetUserName() + wxString::Format(_(" (%u) denied access to shareddirectories/files-list"),m_client->GetUserID()));												
				m_client->SetFileListRequested(0);			
				break;
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				AddDebugLogLineM(false,wxString::Format(_("Edonkey packet: unknown opcode: %i %x"),opcode,opcode));
				break;
		}
	}
	catch(CInvalidPacket ErrorPacket) {
		if (m_client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcesstPacket\n",unicode2char(m_client->GetClientFullInfo()));
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcesstPacket\n", ErrorPacket.what(),unicode2char(m_client->GetClientFullInfo()));
				}
			}
			m_client->SetDownloadState(DS_ERROR);
		} else {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {				
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessPacket\n");
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessPacket\n", ErrorPacket.what());
				}
			}
		}
		Disconnect(wxT("UnCaught invalid packet exception On ProcessPacket\n"));
		return false;
	}
	catch(wxString error) {
		if (m_client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessPacket\n",unicode2char(m_client->GetClientFullInfo()));
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessPacket\n", unicode2char(error),unicode2char(m_client->GetClientFullInfo()));
				}
			}
			m_client->SetDownloadState(DS_ERROR);
			// TODO write this into a debugfile
			AddDebugLogLineM(false,wxString(_("Client '")) + m_client->GetUserName() + wxString::Format(_(" (IP:%s) caused an error: "), m_client->GetFullIP().c_str()) + error + _(". Disconnecting client!"));
		} else {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessPacket\n");
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessPacket\n", unicode2char(error));
				}
			}
			AddDebugLogLineM(false,wxString(_("A unknown client caused an error or did something bad: ")) + error + _(". Disconnecting client!"));
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
			case OP_MULTIPACKET: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_MULTIPACKET\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_MultiPacket", m_client);
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

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
						// send file request no such file packet (0x48)
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileReqAnsNoFil", m_client, packet);
						#endif
						Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
						replypacket->Copy16ToDataBuffer(packet);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->GetPacketSize());
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
					if (theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
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
									Packet* tosend = reqfile->CreateSrcInfoPacket(m_client);
									if(tosend) {
										#ifdef __USE_DEBUG__
										if (theApp.glob_prefs->GetDebugClientTCPLevel() > 0) {
											DebugSend("OP__AnswerSources", m_client, (char*)reqfile->GetFileHash());
										}										
										if (thePrefs.GetDebugSourceExchange()) {
											AddDebugLogLine( false, "RCV:Source Request User(%s) File(%s)", m_client->GetUserName(), reqfile->GetFileName() );
										}
										#endif
										theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
										SendPacket(tosend, true);
									}
								} //else {
								//	if (theApp.glob_prefs->GetVerbose()) {
								//		AddLogLineM(false, _("RCV: Source Request to fast. (This is testing the new timers to see how much older client will not receive this)"));
								//	}
								//}
							}
							break;
						}
					}
				}
				if( data_out.GetLength() > 16 ) {
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__MulitPacketAns", m_client, (char*)reqfile->GetFileHash());
					}
					#endif
					Packet* reply = new Packet(&data_out, OP_EMULEPROT);
					reply->SetOpCode(OP_MULTIPACKETANSWER);
					theApp.uploadqueue->AddUpDataOverheadFileRequest(reply->GetPacketSize());
					SendPacket(reply, true);
				}
				break;
			}

			case OP_MULTIPACKETANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_MULTIPACKETANSWER\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (theApp.glob_prefs->GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_MultiPacketAns", m_client);
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

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
				while(data_in.GetLength()-data_in.GetPosition())
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
		
			case OP_EMULEINFO: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_EMULEINFO\n"));
				#endif
				// 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessMuleInfoPacket(packet,size);
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){

					DebugRecv("OP_EmuleInfo", m_client);
					Debug("  %s\n", m_client->DbgGetMuleInfo());
				}				
				#endif
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
					m_client->InfoPacketsReceived();
				}
				m_client->SendMuleInfoPacket(true);
				break;
			}
			case OP_EMULEINFOANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_EMULEINFOANSWER\n"));
				#endif
				// 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessMuleInfoPacket(packet,size);
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugRecv("OP_EmuleInfoAnswer", m_client);
					Debug("  %s\n", m_client->DbgGetMuleInfo());
				}				
				#endif
				if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
					m_client->InfoPacketsReceived();				
				}
				break;
			}
			case OP_SECIDENTSTATE:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_SECIDENTSTATE\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SecIdentState", m_client);				
				}
				#endif
				
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
						if ( m_client ) m_client->SendSignaturePacket();
					}
				}
				break;
			}
			case OP_PUBLICKEY:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_PUBLICKEY\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SecIdentState", m_client);	
				}					
				#endif
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
 			case OP_SIGNATURE:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_SIGNATURE\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_Signature", m_client);	
				}					
				#endif
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				m_client->ProcessSignaturePacket((uchar*)packet,size);
				break;
			}		
			case OP_COMPRESSEDPART: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_COMPRESSEDPART\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_CompressedPart", m_client);	
				}					
				#endif
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
					m_client->ProcessBlockPacket(packet,size,true);
					if (m_client && m_client->GetRequestFile()->IsStopped() || m_client->GetRequestFile()->GetStatus()==PS_PAUSED || m_client->GetRequestFile()->GetStatus()==PS_ERROR) {
						if (!m_client->GetSentCancelTransfer()) {
							#ifdef __USE_DEBUG__
							if (thePrefs.GetDebugClientTCPLevel() > 0) {
								DebugSend("OP__CancelTransfer", m_client);							
							}
							#endif
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
							m_client->SendPacket(packet,true,true);					
							
							if (m_client)
								m_client->SetSentCancelTransfer(1);
						}

						if ( m_client )
							m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);	
					}
				} else {
					if (!m_client->GetSentCancelTransfer()) {
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__CancelTransfer", m_client);
						}
						#endif
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						m_client->SendPacket(packet,true,true);
						
						if ( m_client )
							m_client->SetSentCancelTransfer(1);
					}
				
					if ( m_client )
						m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_QUEUERANKING: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_QUEUERANKING\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_QueueRanking", m_client);	
				}				
				#endif				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				
				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_QUEUERANKING before finishing handshake"));
				}
				
				if (size != 12) {
					throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
				}

				uint16 newrank = ENDIAN_SWAP_16(PeekUInt16(packet));
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					Debug("  %u (prev. %d)\n", newrank, m_client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)m_client->GetRemoteQueueRank());
				#endif
				m_client->SetRemoteQueueFull(false);
				m_client->SetRemoteQueueRank(newrank);
				break;
			}
 			case OP_REQUESTSOURCES:{
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_REQUESTSOURCES\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugRecv("OP_RequestSources", m_client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);

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
							Packet* tosend = file->CreateSrcInfoPacket(m_client);
							if(tosend) {
								theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
								SendPacket(tosend, true, true);
								#ifdef __USE_DEBUG_
								if (thePrefs.GetDebugClientTCPLevel() > 0) {
									DebugSend("OP__AnswerSources", m_client, (char*)file->GetFileHash());
								}
								if (thePrefs.GetDebugSourceExchange()) {
									AddDebugLogLineM(false, wxString::Format(_("RCV:Source Request User(%s) File(%s)"), m_client->GetUserName().c_str(), file->GetFileName().c_str()));
								}
								#endif
							}
						}
					}
				}
				break;
			}
 			case OP_ANSWERSOURCES: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_ANSWERSOURCES\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);

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
			case OP_FILEDESC: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_FILEDESC\n"));
				#endif
				// 0.43b
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileDesc", m_client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

				if (!m_client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_FILEDESC before finishing handshake"));
				}
				
				m_client->ProcessMuleCommentPacket(packet,size);
				break;
			}
			// Kry - If we ever import the preview capabilities, this is from 0.42e
			// Is raw c/p, unformatted, and with the emule functions.
			case OP_REQUESTPREVIEW: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_REQUESTPREVIEW\n"));
				#endif
			#if 0
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_RequestPreView", client);
				theApp.downloadqueue->AddDownDataOverheadOther(uRawSize);

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_REQUESTPREVIEW before finishing handshake"));
				}
				
				// IP banned, no answer for this request
				if (client->IsBanned())
					break;
				if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))	
				{
					client->ProcessPreviewReq(packet,size);	
					if (thePrefs.GetVerbose())
						AddDebugLogLine(true,"Client '%s' (%s) requested Preview - accepted", client->GetUserName(), ipstr(client->GetConnectIP()));
				}
				else
				{
					// we don't send any answer here, because the client should know that he was not allowed to ask
					if (thePrefs.GetVerbose())
						AddDebugLogLine(true,"Client '%s' (%s) requested Preview - denied", client->GetUserName(), ipstr(client->GetConnectIP()));
				}
				break;
			#endif
			}
			case OP_PREVIEWANSWER: {
				#ifdef DEBUG_REMOTE_CLIENT_PROTOCOL
				AddLogLineM(true,wxT("Remote Client: OP_PREVIEWANSWER\n"));
				#endif
			#if 0
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_PreviewAnswer", client);
				theApp.downloadqueue->AddDownDataOverheadOther(size);

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_PREVIEWANSWER before finishing handshake"));
				}			

				client->ProcessPreviewAnswer(packet, size);
				break;
			#endif
			}
			case OP_PUBLICIP_ANSWER: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessPublicIPAnswer((BYTE*)packet,size);
				break;
			}
			case OP_PUBLICIP_REQ: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				Packet* pPacket = new Packet(OP_PUBLICIP_ANSWER, 4, OP_EMULEPROT);
				pPacket->CopyUInt32ToDataBuffer(m_client->GetIP());
				theApp.uploadqueue->AddUpDataOverheadOther(pPacket->GetPacketSize());
				SendPacket(pPacket);
				break;
			}			
			case OP_AICHANSWER: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessAICHAnswer(packet,size);
				break;
			}
			case OP_AICHREQUEST: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				m_client->ProcessAICHRequest(packet,size);
				break;
			}
			case OP_AICHFILEHASHANS: {
				// those should not be received normally, since we should only get those in MULTIPACKET
				theApp.downloadqueue->AddDownDataOverheadOther(size);
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
				#warning Xaignar - check this for agressive
				/*
				if (pPartFile == NULL){
					m_client->CheckFailedFileIdReqs(abyHash);
					break;
				}
				*/
				if (m_client->IsSupportingAICH() && pPartFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
					&& pPartFile->GetAICHHashset()->HasValidMasterHash()) {
					CSafeMemFile data_out;
					data_out.WriteHash16(abyHash);
					pPartFile->GetAICHHashset()->GetMasterHash().Write(&data_out);
					SendPacket(new Packet(&data_out, OP_EMULEPROT, OP_AICHFILEHASHANS));
				}
				break;
			}
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				AddDebugLogLineM(false,wxString::Format(_("eMule packet : unknown opcode: %i %x"),opcode,opcode));
				break;
		}
	} catch(CInvalidPacket ErrorPacket) {
		if (m_client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n",unicode2char(m_client->GetClientFullInfo()));
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n", ErrorPacket.what(),unicode2char(m_client->GetClientFullInfo()));
				}
			}
			m_client->SetDownloadState(DS_ERROR);
		} else {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {				
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessExtPacket\n");
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: Unknown\n\ton ListenSocket::ProcessExtPacket\n", ErrorPacket.what());
				}
			}
		}
		Disconnect(wxT("UnCaught invalid packet exception On ProcessPacket\n"));
		return false;
	} catch(wxString error) {
		AddDebugLogLineM(false, wxString::Format(_("A client caused an error or did something bad: %s. Disconnecting client!"), error.c_str()));
		if (m_client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {			
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n",unicode2char(m_client->GetClientFullInfo()));
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n", unicode2char(error),unicode2char(m_client->GetClientFullInfo()));
				}
			}
			m_client->SetDownloadState(DS_ERROR);
		} else {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: Unknown\n\ton ListenSocket\n");
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: Unknown\n\ton ListenSocket\n", unicode2char(error));
				}
			}
		}
		Disconnect(wxT("Client error on ListenSocket::ProcessExtPacket: ") + error);
		return false;
	} catch (...) {
		Disconnect(wxT("Unknown exception on ListenSocket::ProcessExtPacket"));
	}

	return true;
}

void CClientReqSocket::OnConnect(int nErrorCode)
{
	//CEMSocket::OnConnect(nErrorCode);
	if (nErrorCode) {
		wxString error = wxString::Format(_("Client TCP socket error (OnConnect): %u"),nErrorCode);
		AddDebugLogLineM(false, error);
		Disconnect(error);
	}
}

void CClientReqSocket::OnSend(int nErrorCode)
{
	// 0.42e
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnError(int nErrorCode)
{
	// 0.42e
	wxString strError;
	if (theApp.glob_prefs->GetVerbose() && (nErrorCode != 0) && (nErrorCode != 107)) {
		// 0    -> No Error / Disconect
		// 107  -> Transport endpoint is not connected
		if (m_client) {
			if (m_client->GetUserName()) {
				strError = wxString(_("Client '")) + m_client->GetUserName();
				strError += wxString::Format(_("' (IP:%s) caused an error: %u. Disconnecting client!"),
					m_client->GetFullIP().c_str(), nErrorCode);
			} else {
				strError.Printf(_("Unknown client (IP:%s) caused an error: %u. Disconnecting client!"),
					m_client->GetFullIP().c_str(), nErrorCode);
			}
		} else {
			strError.Printf(_("A client caused an error or did something bad (error %u). Disconnecting client !"),
				nErrorCode);
		}
		AddLogLineM(false, strError);
	} else {
		strError = _("No error or error 107 (Transport endpoint is not connected)");
	}	
	Disconnect(strError);
}

bool CClientReqSocket::PacketReceived(Packet* packet)
{
	// 0.42e
	bool bResult;
	UINT uRawSize = packet->GetPacketSize();	
	switch (packet->GetProtocol()) {
		case OP_EDONKEYPROT:
			bResult = ProcessPacket(packet->GetDataBuffer(),uRawSize,packet->GetOpCode());
			break;		
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()) {
				AddDebugLogLineM(false, 
					wxString::Format(_("Failed to decompress client TCP packet; protocol=0x%02x  opcode=0x%02x  size=%u"),
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
			theApp.downloadqueue->AddDownDataOverheadOther(uRawSize);
			if (m_client) {
				m_client->SetDownloadState(DS_ERROR);
			}
			Disconnect(wxT("Unknown protocol"));
			bResult = false;
		}
	}
	return bResult;
}

void CClientReqSocket::OnReceive(int nErrorCode)
{
	// 0.42e
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}

//-----------------------------------------------------------------------------
// CClientReqSocketHandler
//-----------------------------------------------------------------------------
CClientReqSocketHandler::CClientReqSocketHandler(CClientReqSocket* parent)
{
	socket = parent;
	socket->my_handler = this;
#ifdef AMULE_DAEMON
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ERROR: CClientReqSocketHandler failed create\n");
		wxASSERT(0);
	}
#endif
}

#ifdef AMULE_DAEMON

void *CClientReqSocketHandler::Entry()
{
	exit_mutex.Lock();
	printf("CClientReqSocketHandler: start thread for %p\n", socket);
	while ( !TestDestroy() ) {
		if ( socket->deletethis ) {
			printf("CClientReqSocketHandler: socket %p being deleted\n", socket);
			break;
		}
		if ( socket->Error()) {
			if ( socket->LastError() == wxSOCKET_WOULDBLOCK ) {
				if ( socket->WaitForWrite(0, 0) ) {
					socket->OnSend(0);
				}
				//continue;
			} else  {
				printf("CClientReqSocketHandler: error %d on socket %p\n", socket->LastError(), socket);
				break;
			}
		}
		if ( socket->WaitForLost(0, 0) ) {
			printf("CClientReqSocketHandler: connection lost on %p\n", socket);
			break;
		}
		// lfroen: tradeof here - short wait time for high performance on delete
		// but long wait time for low cpu usage
		if ( socket->WaitForRead(0, 100) ) {
			CALL_APP_DATA_LOCK;
			socket->OnReceive(0);
		}
	}
	printf("CClientReqSocketHandler: thread for %p exited\n", socket);
	socket->my_handler = 0;
	exit_mutex.Unlock();
	return 0;
}

wxThreadError CClientReqSocketHandler::Delete()
{
	exit_mutex.Lock();
	return wxThread::Delete();
}

#else

BEGIN_EVENT_TABLE(CClientReqSocketHandler, wxEvtHandler)
	EVT_SOCKET(CLIENTREQSOCKET_HANDLER, CClientReqSocketHandler::ClientReqSocketHandler)
END_EVENT_TABLE()

void CClientReqSocketHandler::ClientReqSocketHandler(wxSocketEvent& event)
{
	if(!socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}
	if (socket->OnDestroy()) {
		return;
	}
	//printf("request at clientreqsocket\n");
	switch(event.GetSocketEvent()) {
		case wxSOCKET_LOST:
			socket->OnError(socket->LastError());
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
			// connection requests should not arrive here..
			wxASSERT(0);
			break;
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

// Do we really need that?
IMPLEMENT_DYNAMIC_CLASS(CListenSocket,wxSocketServer)

CListenSocket::CListenSocket(CPreferences* in_prefs, wxSockAddress& addr)
:
// wxSOCKET_NOWAIT    - means non-blocking i/o
// wxSOCKET_REUSEADDR - means we can reuse the socket imediately (wx-2.5.3)
#ifdef AMULE_DAEMON
wxSocketServer(addr, wxSOCKET_WAITALL|wxSOCKET_REUSEADDR), wxThread(wxTHREAD_JOINABLE) 
#else
wxSocketServer(addr, wxSOCKET_NOWAIT|wxSOCKET_REUSEADDR)
#endif
{
	// 0.42e - vars not used by us
	bListening = false;
	app_prefs = in_prefs;
	opensockets = 0;
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
			AddLogLineM(true,wxT("CListenSocket: can not create my thread\n"));
		}
		Notify(false);
#else
 		SetEventHandler(theApp, LISTENSOCKET_HANDLER);
 		SetNotify(wxSOCKET_CONNECTION_FLAG);
 		Notify(true);
#endif	
		printf("ListenSocket: Ok.\n");
	} else {
		AddLogLineM(true,wxT("Error: Could not listen to TCP port.\n"));
	}
}

CListenSocket::~CListenSocket()
{
	// 0.42e + Discard() for discarding the bytes on queue
	Discard();
	Close();
	KillAllSockets();
}

//
// lfroen - this used only in daemon where sockets are threaded
//
#ifdef AMULE_DAEMON
void *CListenSocket::Entry()
{
	while ( !TestDestroy() ) {
		if ( WaitForAccept() ) {
			if ( !theApp.IsReady ) {
				wxSocketBase *s = Accept(false);
				if ( s ) {
					s->Destroy();
				}
				continue;
			}
			CALL_APP_DATA_LOCK;
			OnAccept(0);
		}
	}
	return 0;
}
#endif

bool CListenSocket::StartListening()
{
	// 0.42e
	bListening = true;
	//return (this->Create(app_prefs->GetPort(),SOCK_STREAM,FD_ACCEPT) && this->Listen());
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
			CClientReqSocket* newclient = new CClientReqSocket(app_prefs);
			// Accept the connection and give it to the newly created socket
			if (AcceptWith(*newclient, true)) {
				// OnInit currently does nothing
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
	opensockets++;
}

void CListenSocket::Process()
{
	// 042e + Kry changes for Destroy
	m_OpenSocketsInterval = 0;
	opensockets = 0;
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ) {
		CClientReqSocket* cur_socket = *it++;
		opensockets++;
		if (!cur_socket->OnDestroy()) {
			if (cur_socket->deletethis) {
				cur_socket->Destroy();
			} else {
				cur_socket->CheckTimeOut();
			}
		}
	}
	if ((GetOpenSockets()+5 < app_prefs->GetMaxConnections() || theApp.serverconnect->IsConnecting()) && !bListening) {
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
	if (GetOpenSockets() > app_prefs->GetMaxConnections() || (m_OpenSocketsInterval > (theApp.glob_prefs->GetMaxConperFive()*GetMaxConperFiveModifier()) && !bIgnoreInterval)) {
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
	#if 0
	// -khaos--+++>
	if (peakconnections>thePrefs.GetConnPeakConnections()) {
		thePrefs.Add2ConnPeakConnections(peakconnections);
	}
	// <-----khaos-
	#endif
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

float CListenSocket::GetMaxConperFiveModifier(){
	// 0.42e + Kry
	
	// Kry - I'm making a preferences option for this.
	// It seem to slow down A LOT the sources getting on my box
	// But otoh it might help a lot on users with crappy modems 
	// or routers. Will be called 'Safe Max Connections Calculation"
	
	if (theApp.glob_prefs->GetSafeMaxConn()) {
		//This is a alpha test.. Will clean up for b version.
		float SpikeSize = GetOpenSockets() - averageconnections ;
		if ( SpikeSize < 1 ) {
			return 1;
		}
		float SpikeTolerance = 25.0f*(float)theApp.glob_prefs->GetMaxConperFive()/10.0f;
		if ( SpikeSize > SpikeTolerance ) {
			return 0;
		}
		float Modifier = (1.0f-(SpikeSize/SpikeTolerance));
		return Modifier;
	} else {
		// No modifier.
		return 1;
	}
}
#ifdef AMULE_DAEMON

CSocketGlobalThread::CSocketGlobalThread() : wxThread(wxTHREAD_JOINABLE)
{
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM(true, _("CSocketGlobalThread: call to Create failed"));
	}
}

void CSocketGlobalThread::AddSocket(CClientReqSocket* sock)
{
	socket_list.insert(sock);
}

void CSocketGlobalThread::RemoveSocket(CClientReqSocket* sock)
{
	socket_list.erase(sock);
}


void *CSocketGlobalThread::Entry()
{
	AddDebugLogLineM(true, _("Socket global thread running\n"));
	while ( !TestDestroy() ) {
		Sleep(10);
		std::set<CClientReqSocket *> erase_list;
		CALL_APP_DATA_LOCK;
		for (std::set<CClientReqSocket *>::iterator it = socket_list.begin();
			it != socket_list.end(); it++) {
			CClientReqSocket* cur_sock = *it;
			if (cur_sock->deletethis || cur_sock->Error()) {
				erase_list.insert(cur_sock);
				continue;
			}
			if ( !cur_sock->wxSocketBase::IsConnected() ) {
				if ( cur_sock->WaitOnConnect(0, 0) ) {
					cur_sock->OnConnect(0);
				}
			} else {
				if ( cur_sock->WaitForLost(0, 0) ) {
					erase_list.insert(cur_sock);
					continue;
				}
				if ( cur_sock->WaitForRead(0, 0) ) {
					cur_sock->OnReceive(0);
				}
				CUpDownClient *client = cur_sock->GetClient();
				if ( (client != 0) && (client->GetDownloadState() == DS_DOWNLOADING) ) {
					
					printf("Client %p started dload\n", client);
						
					erase_list.insert(cur_sock);
					CClientReqSocketHandler *t = new CClientReqSocketHandler(cur_sock);
					// fire & forget
					t->Run();
				}
			}
		}
		for (std::set<CClientReqSocket *>::iterator it = erase_list.begin();
			it != erase_list.end(); it++) {
			CClientReqSocket* cur_sock = *it;
			socket_list.erase(cur_sock);
		}
	}
	return 0;
}

#endif
