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
// ListenSocket.cpp : implementation file
//


#include "ListenSocket.h"	// Interface declarations
#include "amule.h"		// Needed for theApp
#include "otherfunctions.h"	// Needed for md4cpy
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "amuleDlg.h"		// Needed for CamuleDlg
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
#include "ChatWnd.h"		// Needed for CChatWnd
#include "sockets.h"		// Needed for CServerConnect
#include "TransferWnd.h"	// Needed for transferwnd

#include <wx/listimpl.cpp>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

//WX_DEFINE_LIST(SocketListL);

BEGIN_EVENT_TABLE(CClientReqSocketHandler, wxEvtHandler)
	EVT_SOCKET(CLIENTREQSOCKET_HANDLER, CClientReqSocketHandler::ClientReqSocketHandler)
END_EVENT_TABLE()

WX_DEFINE_OBJARRAY(ArrayOfwxStrings);

IMPLEMENT_DYNAMIC_CLASS(CClientReqSocket,CEMSocket)

// CClientReqSocket
CClientReqSocket::CClientReqSocket(CPreferences* in_prefs,CUpDownClient* in_client)
{
	app_prefs = in_prefs;
	client = in_client;
	if (in_client) {
		client->socket = this;
	}
	theApp.listensocket->AddSocket(this);
	ResetTimeOutTimer();
	deletethis = false;
	
	my_handler = new CClientReqSocketHandler(this);
	SetEventHandler(*my_handler,CLIENTREQSOCKET_HANDLER);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG|wxSOCKET_LOST_FLAG);
	Notify(TRUE);
}


CClientReqSocket::~CClientReqSocket()
{
	// remove event handler
	SetNotify(0);
	Notify(FALSE);

	if (client) {
		client->socket = NULL;
	}
	client = NULL;
	if (theApp.listensocket) {
		#warning check closing method to change order and get rid of this
		theApp.listensocket->RemoveSocket(this);
	}
	
	delete my_handler;

	//DEBUG_ONLY (theApp.clientlist->Debug_SocketDeleted(this));
}

void CClientReqSocket::ResetTimeOutTimer()
{
	timeout_timer = ::GetTickCount();
}

bool CClientReqSocket::CheckTimeOut()
{
	// 0.42x
	UINT uTimeout = CONNECTION_TIMEOUT;
	if(client) {
		#ifdef __USE_KAD__
		if (client->GetKadState() == KS_CONNECTED_BUDDY) {
			return false;
		}
		#endif
		if (client->GetChatState()!=MS_NONE) {
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

	if (client) {
		if(client->Disconnected(strReason, true)){
			client->socket = NULL;
			delete client;
		} 
		client = NULL;
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
		Notify(FALSE);		
	
		if (client) {
			client->socket = NULL;
		}
		client = NULL;
		byConnected = ES_DISCONNECTED;
		deletethis = true;
		Close();
	}
}

bool CClientReqSocket::ProcessPacket(const char* packet, uint32 size, uint8 opcode)
{
	try{
		if (!client && opcode != OP_HELLO) {
			throw wxString(wxT("Asks for something without saying hello"));
		}
		switch(opcode) {
			case OP_HELLOANSWER: {
				// 0.42e
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessHelloAnswer(packet,size);

				// start secure identification, if
				//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
				//	- we have received eMule-OP_HELLOANSWER (new eMule)
				if (client->GetInfoPacketsReceived() == IP_BOTH) {
					client->InfoPacketsReceived();
				}
				
				if (client) {
					client->ConnectionEstablished();
					//theApp.amuledlg->transferwnd->clientlistctrl.RefreshClient(client);
				}
				break;
			}
			case OP_HELLO: {
				// 0.42e
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				bool bNewClient = !client;				
				if (bNewClient) {
					// create new client to save standart informations
					client = new CUpDownClient(this);
				}
				// client->ProcessHelloPacket(packet,size);
				bool bIsMuleHello = false;
				
				try{
					bIsMuleHello = client->ProcessHelloPacket(packet,size);
				}
				catch(...){
					if (bNewClient){
						// Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
						delete client;
						client = NULL;
					}
					throw;
				}

				// if IP is filtered, dont reply but disconnect...
				if (theApp.ipfilter->IsFiltered(client->GetIP())) {
					AddDebugLogLineM(true,_("Filtered IP: ") + client->GetFullIP() + wxT("(") + theApp.ipfilter->GetLastHit() + wxT(")"));					
					theApp.stat_filteredclients++;
					if (bNewClient) {
						delete client;
						client = NULL;
					}
					Disconnect(wxT("IPFilter"));
					return false;
				}
						
				// now we check if we now this client already. if yes this socket will
				// be attached to the known client, the new client will be deleted
				// and the var. "client" will point to the known client.
				// if not we keep our new-constructed client ;)
				if (theApp.clientlist->AttachToAlreadyKnown(&client,this)) {
					// update the old client informations
					bIsMuleHello = client->ProcessHelloPacket(packet,size);
				} else {
					theApp.clientlist->AddClient(client);
					client->SetCommentDirty();
				}
				//theApp.amuledlg->transferwnd->clientlistctrl.RefreshClient(client);
				// send a response packet with standart informations
				if ((client->GetHashType() == SO_EMULE) && !bIsMuleHello) {
					client->SendMuleInfoPacket(false);				
				}
				
				client->SendHelloAnswer();
				
				if (client) {
					client->ConnectionEstablished();
				}
				// start secure identification, if
				//	- we have received eMule-OP_HELLO (new eMule)				
				if (client->GetInfoPacketsReceived() == IP_BOTH) {
						client->InfoPacketsReceived();				
				}
				break;
			}
			case OP_REQUESTFILENAME: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileRequest", client, packet);				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				if (size >= 16) {
					if (!client->GetWaitStartTime()) {
						client->SetWaitStartTime();
					}
					CSafeMemFile data_in((BYTE*)packet,size);
					CMD4Hash reqfilehash;
					data_in.ReadHash16(reqfilehash);
					CKnownFile* reqfile;
					if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
						if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
							break;
						}
					}
					// if we are downloading this file, this could be a new source
					// no passive adding of files with only one part
					if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
						if (theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client);
						}
					}

					// check to see if this is a new file they are asking for
					if (client->GetUploadFileID() != reqfilehash) {
							client->SetCommentDirty();
					}

					client->SetUploadFileID(reqfile);
					client->ProcessExtendedInfo(&data_in, reqfile);
					
					// send filename etc
					CSafeMemFile data_out(128);
					data_out.WriteRaw(reqfile->GetFileHash(),16);
					data_out.Write(reqfile->GetFileName());
					Packet* packet = new Packet(&data_out);
					packet->SetOpCode(OP_REQFILENAMEANSWER);
					#ifdef __USE__DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__FileReqAnswer", client, (char*)reqfile->GetFileHash());
					}
					#endif
					theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
					SendPacket(packet,true);
					client->SendCommentInfo(reqfile);
					break;
				}
				throw wxString(wxT("Invalid OP_REQUESTFILENAME packet size"));
				break;  
			}
			case OP_SETREQFILEID: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__SetReqfileID", client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				
				if (client->IsBanned()) {
					break;
				}
				
				// DbT:FileRequest
				if (size == 16) {
					if (!client->GetWaitStartTime()) {
						client->SetWaitStartTime();
					}

					CKnownFile* reqfile;
					if ( (reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet)) == NULL ){
						if ( !((reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet)) != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
							// send file request no such file packet (0x48)
							#ifdef __USE_DEBUG__
							if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    				DebugSend("OP__FileReqAnsNoFil", client);
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
					if(md4cmp(client->GetUploadFileID(), packet) != 0) {
						client->SetCommentDirty();
					}

					client->SetUploadFileID(reqfile);
					// send filestatus
					CSafeMemFile data(16+16);
					data.WriteHash16(reqfile->GetFileHash());
					if (reqfile->IsPartFile()) {
						((CPartFile*)reqfile)->WritePartStatus(&data);
					} else {
						data.Write((uint16)0);
					}
					Packet* packet = new Packet(&data);
					packet->SetOpCode(OP_FILESTATUS);
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
		    				DebugSend("OP__FileStatus", client);
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
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileReqAnsNoFil", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size == 16)
				{
					// if that client does not have my file maybe has another different
					CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
					if (!reqfile) {
						break;
					}
					// we try to swap to another file ignoring no needed parts files
					switch (client->GetDownloadState())
					{
						case DS_CONNECTED:
						case DS_ONQUEUE:
						case DS_NONEEDEDPARTS:
						if (!client->SwapToAnotherFile(true, true, true, NULL)) {
							theApp.downloadqueue->RemoveSource(client);
						}
						break;
					}
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;
			}
			case OP_REQFILENAMEANSWER: {
				// 0.42e
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileReqAnswer", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				client->ProcessFileInfo(&data, file);
				break;
			}
			case OP_FILESTATUS: {
				// 0.42e
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileStatus", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar cfilehash[16];
				data.ReadHash16(cfilehash);
				CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
				client->ProcessFileStatus(false, &data, file);
				break;
			}
			case OP_STARTUPLOADREQ: {
				// 0.42e
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_StartUpLoadReq", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (!client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode)) {
					break;
				}
				if (size == 16) {
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if (reqfile) {
						if (md4cmp(client->GetUploadFileID(), packet) != 0) {
							client->SetCommentDirty();
						}
						client->SetUploadFileID(reqfile);
						client->SendCommentInfo(reqfile);
						theApp.uploadqueue->AddClientToQueue(client);
					}
				}
				break;
			}
			case OP_QUEUERANK: {
				// 0.42e
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_QueueRank", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uint32 rank;
				data.Read(rank);
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					Debug("  %u (prev. %d)\n", rank, client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)client->GetRemoteQueueRank());
				}
				#endif
				client->SetRemoteQueueRank(rank);
				break;
			}
			case OP_ACCEPTUPLOADREQ: {
				// 0.42e (xcept khaos stats)
				#ifdef __USE__DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugRecv("OP_AcceptUploadReq", client, size >= 16 ? packet : NULL);
					if (size > 0)
						Debug("  ***NOTE: Packet contains %u additional bytes\n", size);
					Debug("  QR=%d\n", client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)client->GetRemoteQueueRank());
				}				
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					if (client->GetDownloadState() == DS_ONQUEUE ) {
						client->SetDownloadState(DS_DOWNLOADING);
						client->m_lastPartAsked = 0xffff; // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
						client->SendBlockRequests();
					}
				} else {
					if (!client->GetSentCancelTransfer()) {
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__CancelTransfer", client);
						}
						#endif
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						client->socket->SendPacket(packet,true,true);
						client->SetSentCancelTransfer(1);
					}
				}
				break;
			}
			case OP_REQUESTPARTS: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_RequestParts", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

				CSafeMemFile data((BYTE*)packet,size);

				CMD4Hash reqfilehash;
				data.ReadHash16(reqfilehash);

				uint32 auStartOffsets[3];
				data.Read(auStartOffsets[0]);
				data.Read(auStartOffsets[1]);
				data.Read(auStartOffsets[2]);

				uint32 auEndOffsets[3];
				data.Read(auEndOffsets[0]);
				data.Read(auEndOffsets[1]);
				data.Read(auEndOffsets[2]);

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
						client->AddReqBlock(reqblock);
					} else {
						if (theApp.glob_prefs->GetVerbose()) {
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0) {
									AddLogLineM(false,wxString::Format(wxT("Client requests invalid %u. file block %u-%u (%d bytes): "), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i])  + client->GetFullIP());
								}
							}
						}
					}
					break;
			}
			case OP_CANCELTRANSFER: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_CancelTransfer", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				theApp.uploadqueue->RemoveFromUploadQueue(client);
				if (theApp.glob_prefs->GetVerbose()) {
					AddDebugLogLineM(false, client->GetUserName() + wxT(": Upload session ended due canceled transfer."));
				}
				break;
			}
			case OP_END_OF_DOWNLOAD: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_EndOfDownload", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size>=16 && !md4cmp(client->GetUploadFileID(),packet)) {
					theApp.uploadqueue->RemoveFromUploadQueue(client);
					if (theApp.glob_prefs->GetVerbose()) {
						AddDebugLogLineM(false, client->GetUserName() + wxT(": Upload session ended due ended transfer."));
					}
				}
				break;
			}
			case OP_HASHSETREQUEST: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_HashSetRequest", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
				}
				client->SendHashsetPacket((uchar*)packet);
				break;
			}
			case OP_HASHSETANSWER: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_HashSetAnswer", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				client->ProcessHashSet(packet,size);
				break;
			}
			case OP_SENDINGPART: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SendingPart", client);
				}
				#endif				
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					client->ProcessBlockPacket(packet,size);
					if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR) {
						if (!client->GetSentCancelTransfer()) {
							#ifdef __USE_DEBUG__
							if (thePrefs.GetDebugClientTCPLevel() > 0) {
							    DebugSend("OP__CancelTransfer", client);

							}
							#endif				
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
							client->socket->SendPacket(packet,true,true);
							client->SetSentCancelTransfer(1);
						}
						client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);
					}
				} else {
					if (!client->GetSentCancelTransfer()) {
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
						    DebugSend("OP__CancelTransfer", client);
						}
						#endif				
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						client->socket->SendPacket(packet,true,true);
						client->SetSentCancelTransfer(1);
					}
					client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_OUTOFPARTREQS: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__OutOfPartReqs", client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (client->GetDownloadState() == DS_DOWNLOADING) {
					client->SetDownloadState(DS_ONQUEUE);
				}
				break;
			}
			case OP_CHANGE_CLIENT_ID:{
				// 0.42e (xcept the IDHybrid)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
				    DebugSend("OP__ChangeClientID", client);
				}
				#endif				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((BYTE*)packet, size);
				uint32 nNewUserID;
				data.Read(nNewUserID);
				uint32 nNewServerIP;
				data.Read(nNewServerIP);
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					Debug("  NewUserID=%u (%08x, %s)  NewServerIP=%u (%08x, %s)\n", nNewUserID, nNewUserID, ipstr(nNewUserID), nNewServerIP, nNewServerIP, ipstr(nNewServerIP));
				}
				#endif
				if (IsLowIDED2K(nNewUserID)) { // client changed server and gots a LowID
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						#warning Here should be the IDHybrid, but we don't use it yet and I'm afraid it will break a lot ;)
						client->SetUserID(nNewUserID); // update UserID only if we know the server
						client->SetServerIP(nNewServerIP);
						client->SetServerPort(pNewServer->GetPort());
					}
				} else if (nNewUserID == client->GetIP()) { // client changed server and gots a HighID(IP)
					#warning Here should be the IDHybrid, but we don't use it yet and I'm afraid it will break a lot ;)					
					client->SetUserID(nNewUserID);
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						client->SetServerIP(nNewServerIP);
						client->SetServerPort(pNewServer->GetPort());
					}
				} else{
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						Debug("***NOTE: OP_ChangedClientID unknown contents\n");
					}
					#endif
				}
				UINT uAddData = data.GetLength() - data.GetPosition();
				if (uAddData > 0){
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						Debug("***NOTE: OP_ChangedClientID contains add. data %s\n", GetHexDump((uint8*)packet + data.GetPosition(), uAddData));
					}
					#endif
				}
				break;
			}					
			case OP_CHANGE_SLOT:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_ChangeSlot", client, size>=16 ? packet : NULL);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// sometimes sent by Hybrid
				break;
			}			
			case OP_MESSAGE: {
				// 0.42e
				// But anyway we can change all this to a simple Read on a mem file.
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_Message", client, size>=16 ? packet : NULL);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (size < 2) {
					throw wxString(wxT("Invalid message packet"));
				}
				uint16 length = ENDIAN_SWAP_16(*((uint16*)packet));
				if ((uint32)length + 2 != size) {
					throw wxString(wxT("Invalid message packet"));
				}
				AddLogLineM(true,wxString(wxT("New message from '")) + client->GetUserName() + wxT("' (IP:") + client->GetFullIP() + wxT(")"));
				#warning TODO: CHECK MESSAGE FILTERING!
				//filter me?
				if ( (theApp.glob_prefs->MsgOnlyFriends() && !client->IsFriend()) ||
					 (theApp.glob_prefs->MsgOnlySecure() && client->GetUserName()==wxEmptyString) ) {
					#if 0
					if (!client->m_bMsgFiltered) {
						AddLogLineM(true,wxString(wxT("Message filtered from '")) + client->GetUserName() + wxT("' (IP:") + client->GetFullIP() + wxT(")"));
					}
					client->m_bMsgFiltered=true;
					#endif
					break;
				}
				char* message = new char[length+1];
				memcpy(message,packet+2,length);
				message[length] = '\0';
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					Debug("  %s\n", message);
				}
				#endif				
				theApp.amuledlg->chatwnd->ProcessMessage(client, char2unicode(message));
				delete[] message;
				break;
			}
			case OP_ASKSHAREDFILES:	{
				// 0.42e (well, er, it does the same, but in our own way)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFiles", client);
				}
				#endif				
				// client wants to know what we have in share, let's see if we allow him to know that
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				CList<void*,void*> list;
				if (theApp.glob_prefs->CanSeeShares()==vsfaEverybody || (theApp.glob_prefs->CanSeeShares()==vsfaFriends && client->IsFriend())) {
					CKnownFileMap& filemap = theApp.sharedfiles->m_Files_map;
					for (CKnownFileMap::iterator pos = filemap.begin();pos != filemap.end(); pos++ ) {
						list.AddTail((void*&)pos->second);
					}
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your sharedfiles-list -> %s"),client->GetUserID(),_("Accepted")));
				} else {
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your sharedfiles-list -> %s"),client->GetUserID(),_("Denied")));
				}
				// now create the memfile for the packet
				CSafeMemFile tempfile(80);
				tempfile.Write((uint32)list.GetCount());
				while (list.GetCount()) {
					theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, false);
					list.RemoveHead();
				}
				// create a packet and send it
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugSend("OP__AskSharedFilesAnswer", client);
				}
				#endif
				Packet* replypacket = new Packet(&tempfile);
				replypacket->SetOpCode(OP_ASKSHAREDFILESANSWER);
				theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
				SendPacket(replypacket, true, true);
				break;
			}
			case OP_ASKSHAREDFILESANSWER: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesAnswer", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessSharedFileList(packet,size);
				break;
			}
			case OP_ASKSHAREDDIRS: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDirectories", client);
				}
				#endif
				
 				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				if ((theApp.glob_prefs->CanSeeShares()==vsfaEverybody) || ((theApp.glob_prefs->CanSeeShares()==vsfaFriends) && client->IsFriend())) {
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your shareddirectories-list -> "),client->GetUserID()) + _("accepted"));			

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
					tempfile.Write(uDirs);
					for (uint32 iDir=0; iDir < uDirs; iDir++) {
						tempfile.Write(folders_to_send[iDir]);
					}

					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDirsAnswer", client);
					}
					#endif				
					Packet* replypacket = new Packet(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDDIRSANS);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDirsDeniedAnswer", client);
					}
					#endif
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your shareddirectories-list -> "),client->GetUserID()) + _("denied"));			
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}

				break;
			}
			case OP_ASKSHAREDFILESDIR: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesInDirectory", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				CSafeMemFile data((uchar*)packet, size);
											
				wxString strReqDir;
				data.Read(strReqDir);
				if (theApp.glob_prefs->CanSeeShares()==vsfaEverybody || (theApp.glob_prefs->CanSeeShares()==vsfaFriends && client->IsFriend())) {
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your sharedfiles-list for directory "),client->GetUserID()) + strReqDir + wxT(" -> ") + _("accepted"));			
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
					tempfile.Write(strReqDir);
					tempfile.Write((uint32)list.GetCount());
					while (list.GetCount()) {
						theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, false);
						list.RemoveHead();
					}
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedFilesInDirectoryAnswer", client);
					}
					#endif
					Packet* replypacket = new Packet(&tempfile);
					replypacket->SetOpCode(OP_ASKSHAREDFILESDIRANS);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				} else {
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(wxT(" (%u) requested your sharedfiles-list for directory "),client->GetUserID()) + strReqDir + wxT(" -> ") + _("denied"));			
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__AskSharedDeniedAnswer", client);
					}
					#endif
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
					SendPacket(replypacket, true, true);
				}
				break;
			}		
			
			case OP_ASKSHAREDDIRSANS:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDirectoriesAnswer", client);
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (client->GetFileListRequested() == 1){
					CSafeMemFile data((uchar*)packet, size);
					uint32 uDirs;
					data.Read(uDirs);
					for (uint32 i = 0; i < uDirs; i++){
						wxString strDir;
						data.Read(strDir);
						AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) shares directory "),client->GetUserID()) + strDir);
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__AskSharedFilesInDirectory", client);
						}
						#endif
				
						CSafeMemFile tempfile(80);
						tempfile.Write(strDir);
						Packet* replypacket = new Packet(&tempfile);
						replypacket->SetOpCode(OP_ASKSHAREDFILESDIR);
						theApp.uploadqueue->AddUpDataOverheadOther(replypacket->GetPacketSize());
						SendPacket(replypacket, true, true);
					}
					wxASSERT( data.GetPosition() == data.GetLength() );
					client->SetFileListRequested(uDirs);
				} else {
						AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) sent unasked shared dirs."),client->GetUserID()));
				}
      			break;
      		}
      
			case OP_ASKSHAREDFILESDIRANS: {
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedFilesInDirectoryAnswer", client);			  
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((uchar*)packet, size, 0);
				wxString strDir;
				data.Read(strDir);

				if (client->GetFileListRequested() > 0){
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) sent sharedfiles-list for directory %s"),client->GetUserID()) + strDir);
					#warning We need a new ProcessSharedFileList that can handle dirs. ___UNICODE___
					client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), unicode2char(strDir));
					if (client->GetFileListRequested() == 0) {
						AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) finished sending sharedfiles-list"),client->GetUserID()));
					}
				} else {
					AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) sent unwanted sharedfiles-list"),client->GetUserID()));					
				}
				break;
			}
			case OP_ASKSHAREDDENIEDANS:
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AskSharedDeniedAnswer", client);
				}
				#endif
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				AddLogLineM(true,wxString(_("User ")) + client->GetUserName() + wxString::Format(_(" (%u) denied access to shareddirectories/files-list"),client->GetUserID()));												
				client->SetFileListRequested(0);			
				break;
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				AddDebugLogLineM(false,wxString::Format(wxT("Edonkey packet: unknown opcode: %i %x"),opcode,opcode));
				break;
		}
	}
	catch(CInvalidPacket ErrorPacket) {
		if (client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcesstPacket\n",unicode2char(client->GetClientFullInfo()));
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcesstPacket\n", ErrorPacket.what(),unicode2char(client->GetClientFullInfo()));
				}
			}
			client->SetDownloadState(DS_ERROR);
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
		if (client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessPacket\n",unicode2char(client->GetClientFullInfo()));
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessPacket\n", unicode2char(error),unicode2char(client->GetClientFullInfo()));
				}
			}
			client->SetDownloadState(DS_ERROR);
			// TODO write this into a debugfile
			AddDebugLogLineM(false,wxString(_("Client '")) + client->GetUserName() + wxString::Format(_(" (IP:%s) caused an error: "), unicode2char(client->GetFullIP())) + error + _(". Disconnecting client!"));
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
		if (!client) {
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
				 // 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_MultiPacket", client);
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

				if (client->IsBanned()) {
					break;
				}

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_MULTIPACKET before finishing handshake"));
				}

				CSafeMemFile data_in((BYTE*)packet,size);
				CMD4Hash reqfilehash;
				data_in.ReadRaw(reqfilehash,16);
				CKnownFile* reqfile;

				if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
					if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
						// send file request no such file packet (0x48)
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileReqAnsNoFil", client, packet);
						#endif
						Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
						replypacket->Copy16ToDataBuffer(packet);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->GetPacketSize());
						SendPacket(replypacket, true);
						break;
					}
				}

				if (!client->GetWaitStartTime()) {
					client->SetWaitStartTime();
				}
				// if we are downloading this file, this could be a new source
				// no passive adding of files with only one part
				if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
					if (theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
						theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client);
					}
				}
				// check to see if this is a new file they are asking for
				if (client->GetUploadFileID() != reqfilehash) {
					client->SetCommentDirty();
				}
				client->SetUploadFileID(reqfile);
				uint8 opcode_in;
				CSafeMemFile data_out(128);
				data_out.WriteRaw(reqfile->GetFileHash(),16);
				while(data_in.GetLength()-data_in.GetPosition()) {
					data_in.Read(opcode_in);
					switch(opcode_in) {
						case OP_REQUESTFILENAME: {
							client->ProcessExtendedInfo(&data_in, reqfile);
							data_out.Write((uint8)OP_REQFILENAMEANSWER);
							data_out.Write(reqfile->GetFileName());
							break;
						}
						case OP_SETREQFILEID: {
							data_out.Write((uint8)OP_FILESTATUS);
							if (reqfile->IsPartFile()) {
								((CPartFile*)reqfile)->WritePartStatus(&data_out);
							} else {
								data_out.Write((uint16)0);
							}
							break;
						}
						//We still send the source packet seperately.. 
						//We could send it within this packet.. If agreeded, I will fix it..
						case OP_REQUESTSOURCES: {
							//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
							if (client->GetSourceExchangeVersion() > 1) {
								//data_out.WriteUInt8(OP_ANSWERSOURCES);
								DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
								bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
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
									client->SetLastSrcReqTime();
									Packet* tosend = reqfile->CreateSrcInfoPacket(client);
									if(tosend) {
										#ifdef __USE_DEBUG__
										if (theApp.glob_prefs->GetDebugClientTCPLevel() > 0) {
											DebugSend("OP__AnswerSources", client, (char*)reqfile->GetFileHash());
										}										
										if (thePrefs.GetDebugSourceExchange()) {
											AddDebugLogLine( false, "RCV:Source Request User(%s) File(%s)", client->GetUserName(), reqfile->GetFileName() );
										}
										#endif
										theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
										SendPacket(tosend, true);
									}
								} else {
									if (theApp.glob_prefs->GetVerbose()) {
											AddLogLineM(false, wxT("RCV: Source Request to fast. (This is testing the new timers to see how much older client will not receive this)"));
									}
								}
							}
							break;
						}
					}
				}
				if( data_out.GetLength() > 16 ) {
					#ifdef __USE_DEBUG__
					if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugSend("OP__MulitPacketAns", client, (char*)reqfile->GetFileHash());
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
				// 0.42e
				#ifdef __USE_DEBUG__
				if (theApp.glob_prefs->GetDebugClientTCPLevel() > 0)
					DebugRecv("OP_MultiPacketAns", client);
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

				if (client->IsBanned()) {
					break;
				}
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_MULTIPACKETANSWER before finishing handshake"));
				}
				
				CSafeMemFile data_in((BYTE*)packet,size);
				CMD4Hash reqfilehash;
				data_in.ReadRaw(reqfilehash,16);
				CPartFile* reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
				//Make sure we are downloading this file.
				if (reqfile==NULL) {
					throw wxString(_(" Wrong File ID: (OP_MULTIPACKETANSWER; reqfile==NULL)"));
				}
				if (client->reqfile==NULL) {
					throw wxString(_(" Wrong File ID: OP_MULTIPACKETANSWER; client->reqfile==NULL)"));
				}
				if (reqfile != client->reqfile) {
					throw wxString(_(" Wrong File ID: OP_MULTIPACKETANSWER; reqfile!=client->reqfile)"));
				}
				uint8 opcode_in;
				while(data_in.GetLength()-data_in.GetPosition())
				{
					data_in.Read(opcode_in);
					switch(opcode_in)
					{
						case OP_REQFILENAMEANSWER:
						{
							client->ProcessFileInfo(&data_in, reqfile);
							break;
						}
						case OP_FILESTATUS:
						{
							client->ProcessFileStatus(false, &data_in, reqfile);
							break;
						}
					}
				}
				break;
			}
		
			case OP_EMULEINFO: {
				// 0.42e
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessMuleInfoPacket(packet,size);
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){

					DebugRecv("OP_EmuleInfo", client);
					Debug("  %s\n", client->DbgGetMuleInfo());
				}				
				#endif
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				if (client->GetInfoPacketsReceived() == IP_BOTH) {
					client->InfoPacketsReceived();
				}
				client->SendMuleInfoPacket(true);
				break;
			}
			case OP_EMULEINFOANSWER: {
				// 0.42e
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessMuleInfoPacket(packet,size);
				// start secure identification, if
				//  - we have received eD2K and eMule info (old eMule)
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugRecv("OP_EmuleInfoAnswer", client);
					Debug("  %s\n", client->DbgGetMuleInfo());
				}				
				#endif
				if (client->GetInfoPacketsReceived() == IP_BOTH) {
					client->InfoPacketsReceived();				
				}
				break;
			}
			case OP_SECIDENTSTATE:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SecIdentState", client);				
				}
				#endif
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_SECIDENTSTATE before finishing handshake"));
				}
								
				client->ProcessSecIdentStatePacket((uchar*)packet,size);
				if (client->GetSecureIdentState() == IS_SIGNATURENEEDED)
					client->SendSignaturePacket();
				else if (client->GetSecureIdentState() == IS_KEYANDSIGNEEDED){
					client->SendPublicKeyPacket();
					client->SendSignaturePacket();
				}
				break;
			}
			case OP_PUBLICKEY:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_SecIdentState", client);	
				}					
				#endif
				if (client->IsBanned() ){
					break;						
				}
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_PUBLICKEY before finishing handshake"));
				}
												
				client->ProcessPublicKeyPacket((uchar*)packet,size);
				break;
			}
 			case OP_SIGNATURE:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_Signature", client);	
				}					
				#endif
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				client->ProcessSignaturePacket((uchar*)packet,size);
				break;
			}		
			case OP_COMPRESSEDPART: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_CompressedPart", client);	
				}					
				#endif
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
				}
												
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					client->ProcessBlockPacket(packet,size,true);
					if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR) {
						if (!client->GetSentCancelTransfer()) {
							#ifdef __USE_DEBUG__
							if (thePrefs.GetDebugClientTCPLevel() > 0) {
								DebugSend("OP__CancelTransfer", client);							
							}
							#endif
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
							client->socket->SendPacket(packet,true,true);					
							client->SetSentCancelTransfer(1);
						}
						client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);						
					}
				} else {
					if (!client->GetSentCancelTransfer()) {
						#ifdef __USE_DEBUG__
						if (thePrefs.GetDebugClientTCPLevel() > 0) {
							DebugSend("OP__CancelTransfer", client);
						}
						#endif
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
						client->socket->SendPacket(packet,true,true);
						client->SetSentCancelTransfer(1);
					}
					client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_QUEUERANKING: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_QueueRanking", client);	
				}				
				#endif				
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				
				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
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
					Debug("  %u (prev. %d)\n", newrank, client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)client->GetRemoteQueueRank());
				#endif
				client->SetRemoteQueueFull(false);
				client->SetRemoteQueueRank(newrank);
				break;
			}
 			case OP_REQUESTSOURCES:{
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
						DebugRecv("OP_RequestSources", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_REQUESTSOURCES before finishing handshake"));
				}
				
				if (client->GetSourceExchangeVersion() >= 1) {
					if(size != 16) {
						throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
					}
					//first check shared file list, then download list
					CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if(!file) {
						file = theApp.downloadqueue->GetFileByID((uchar*)packet);
					}
					if(file) {
						DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
						bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
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
							client->SetLastSrcReqTime();
							Packet* tosend = file->CreateSrcInfoPacket(client);
							if(tosend) {
								theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->GetPacketSize());
								SendPacket(tosend, true, true);
								#ifdef __USE_DEBUG_
								if (thePrefs.GetDebugClientTCPLevel() > 0) {
									DebugSend("OP__AnswerSources", client, (char*)file->GetFileHash());
								}
								if (thePrefs.GetDebugSourceExchange()) {
									AddDebugLogLineF( false, "RCV:Source Request User(%s) File(%s)", client->GetUserName(), file->GetFileName().GetData());
								}
								#endif
							}
						}
					}
				}
				break;
			}
 			case OP_ANSWERSOURCES: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_AnswerSources", client, packet);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_ANSWERSOURCES before finishing handshake"));
				}
				
				CSafeMemFile data((BYTE*)packet,size);
				uchar hash[16];
				data.ReadHash16(hash);
				CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
				if(file){
					if (file->IsPartFile()){
						//set the client's answer time
						client->SetLastSrcAnswerTime();
						//and set the file's last answer time
						((CPartFile*)file)->SetLastAnsweredTime();

						((CPartFile*)file)->AddClientSources(&data, client->GetSourceExchangeVersion());
					}
				}
				break;
			}
			case OP_FILEDESC: {
				// 0.42e
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugRecv("OP_FileDesc", client);
				}
				#endif
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);

				if (!client->CheckHandshakeFinished(OP_EMULEPROT, opcode)) {
					// Here comes a extended packet without finishing the hanshake.
					// IMHO, we should disconnect the client.
					throw wxString(wxT("Client send OP_FILEDESC before finishing handshake"));
				}
				
				client->ProcessMuleCommentPacket(packet,size);
				break;
			}
			#if 0
			// Kry - If we ever import the preview capabilities, this is from 0.42e
			// Is raw c/p, unformatted, and with the emule functions.
			case OP_REQUESTPREVIEW: {
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
			}
			case OP_PREVIEWANSWER:
			{
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
			}			
			#endif
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				AddDebugLogLineM(false,wxString::Format(wxT("eMule packet : unknown opcode: %i %x"),opcode,opcode));
				break;
		}
	} catch(CInvalidPacket ErrorPacket) {
		if (client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (!strlen(ErrorPacket.what())) {
					printf("\tCaught InvalidPacket exception:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n",unicode2char(client->GetClientFullInfo()));
				} else {
					printf("\tCaught InvalidPacket exception:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n", ErrorPacket.what(),unicode2char(client->GetClientFullInfo()));
				}
			}
			client->SetDownloadState(DS_ERROR);
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
		AddDebugLogLineF(false,_("A client caused an error or did something bad: %s. Disconnecting client!"),error.GetData());
		if (client) {
			if (theApp.glob_prefs->GetVerbosePacketError()) {
				if (error.IsEmpty()) {			
					printf("\tCaught error:\n\t\tError: Unknown\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n",unicode2char(client->GetClientFullInfo()));
				} else {
					printf("\tCaught error:\n\t\tError: %s\n\t\tClientData: %s\n\ton ListenSocket::ProcessExtPacket\n", unicode2char(error),unicode2char(client->GetClientFullInfo()));
				}
			}
			client->SetDownloadState(DS_ERROR);
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

void CClientReqSocket::OnInit()
{
	// 0.42e
	// uint8 tv = 1; // commented like in eMule 0.30c (Creteil)
	// SetSockOpt(SO_DONTLINGER,&tv,sizeof(bool));
}

#warning OnConnect? ;)
#if 0
void CClientReqSocket::OnConnect(int nErrorCode)
{
	CEMSocket::OnConnect(nErrorCode);
	if (nErrorCode)
	{
	    wxString strTCPError;
		if (thePrefs.GetVerbose())
		{
		    strTCPError = GetErrorMessage(nErrorCode, 1);
		    if (nErrorCode != WSAECONNREFUSED && nErrorCode != WSAETIMEDOUT)
			    AddDebugLogLine(false, _T("Client TCP socket error (OnConnect): %s; %s"), strTCPError, DbgGetClientInfo());
		}
	}
}
#endif


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
		if (client) {
			if (client->GetUserName()) {
				strError = wxString(_("Client '")) + client->GetUserName();
				strError += wxString::Format(_("' (IP:%s) caused an error: %u. Disconnecting client!"),unicode2char(client->GetFullIP()),nErrorCode);
			} else {
				strError.Printf(_("Unknown client (IP:%s) caused an error: %u. Disconnecting client!"),unicode2char(client->GetFullIP()),nErrorCode);
			}
		} else {
			strError.Printf(_("A client caused an error or did something bad (error %u). Disconnecting client !"),nErrorCode);
		}
		AddLogLineM(false,strError);
	} else {
		strError = wxT("No error or error 107 (Transport endpoint is not connected)");
	}
	
	Disconnect(strError);
}

CClientReqSocket::CClientReqSocket()
{
}

bool CClientReqSocket::Close()
{
	return wxSocketBase::Close();
}

bool CClientReqSocket::PacketReceived(Packet* packet)
{
	// 0.42e
	bool bResult;
	UINT uRawSize = packet->GetPacketSize();	
	switch (packet->GetProtocol()) {
		case OP_EDONKEYPROT:
/**/
			bResult = ProcessPacket(packet->GetDataBuffer(),uRawSize,packet->GetOpCode());
			break;
		
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()) {
				AddDebugLogLineF(false,wxT("Failed to decompress client TCP packet; protocol=0x%02x  opcode=0x%02x  size=%u"), packet->GetProtocol(), packet->GetOpCode(), packet->GetPacketSize());				
				bResult = false;
				break;
			}
		case OP_EMULEPROT:
			bResult =  ProcessExtPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
			break;
		default: {
			theApp.downloadqueue->AddDownDataOverheadOther(uRawSize);
			if (client) {
				client->SetDownloadState(DS_ERROR);
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

bool CClientReqSocket::Create()
{
	theApp.listensocket->AddConnection();
	//bool result = CAsyncSocket::Create(0,SOCK_STREAM,FD_WRITE|FD_READ|FD_CLOSE);
	OnInit();
	return TRUE; //result;
}


void CClientReqSocketHandler::ClientReqSocketHandler(wxSocketEvent& event) {

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
			break;
		default:
			// connection requests should not arrive here..
			wxASSERT(0);
			break;
	}
	
}


IMPLEMENT_DYNAMIC_CLASS(CListenSocket,wxSocketServer)

// CListenSocket
// CListenSocket member functions
CListenSocket::CListenSocket(CPreferences* in_prefs,wxSockAddress& addr)
: wxSocketServer(addr,wxSOCKET_NOWAIT)
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
	SetEventHandler(theApp,LISTENSOCKET_HANDLER);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG);
	Notify(TRUE);
}

CListenSocket::~CListenSocket()
{
	// 0.42e + Discard() for discarding the bytes on queue
	Discard();
	Close();
	KillAllSockets();
}

bool CListenSocket::StartListening()
{
	// 0.42e
	bListening = true;
	//return (this->Create(app_prefs->GetPort(),SOCK_STREAM,FD_ACCEPT) && this->Listen());
	return TRUE;
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
		while (m_nPeningConnections) {
			m_nPeningConnections--;
			CClientReqSocket* newclient = new CClientReqSocket(app_prefs);
			// if (!Accept(*newclient)) {
			if(!AcceptWith(*newclient,FALSE)) {
				newclient->Safe_Delete();
			} else {
				//newclient->AsyncSelect(FD_WRITE|FD_READ|FD_CLOSE);
				newclient->OnInit();
			}
			AddConnection();
		}
	}
}

void CListenSocket::Process()
{
	// 042e + Kry changes for Destroy
	POSITION pos2;
	m_OpenSocketsInterval = 0;
	opensockets = 0;
	for(POSITION pos1 = socket_list.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		opensockets++;
		
		if (!cur_sock->OnDestroy()) {
			if (cur_sock->deletethis) {
				cur_sock->Destroy();
			} else {
				cur_sock->CheckTimeOut();
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
	POSITION pos1,pos2;
	for(pos1 = socket_list.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		switch (cur_sock->GetConState()) {
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
	//0.42e
	socket_list.AddTail(toadd);
}

void CListenSocket::RemoveSocket(CClientReqSocket* todel)
{
	// 0.42e
	POSITION pos2,pos1;
	for(pos1 = socket_list.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		socket_list.GetNext(pos1);
		if (socket_list.GetAt(pos2) == todel) {
			socket_list.RemoveAt(pos2);
		}
	}
}

void CListenSocket::KillAllSockets()
{
	// 0.42e reviewed - they use delete, but our safer is Destroy...
	// But I bet it would be better to call Safe_Delete on the socket.
	// Update: no... Safe_Delete MARKS for deletion. We need to delete it.
	for (POSITION pos = socket_list.GetHeadPosition();pos != 0;) {
		CClientReqSocket* cur_socket = socket_list.GetNext(pos);
		if (cur_socket->client) {
			delete cur_socket->client;
		} else {
			cur_socket->Safe_Delete();
			cur_socket->Destroy(); 
		}
	}
}

void CListenSocket::AddConnection()
{
	// 0.42e
	m_OpenSocketsInterval++;
	opensockets++;
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
	return socket_list.Find(totest);
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
