//this file is part of aMule
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
#include "otherfunctions.h"	// Needed for md4cpy
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "updownclient.h"	// Needed for CUpDownClient
#include "amule.h"		// Needed for theApp
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
#include "ChatSelector.h"	// Needed for CChatSelector
#include "sockets.h"		// Needed for CServerConnect

#ifndef ID_SOKETTI
#define ID_SOKETTI 7772
#endif

//WX_DEFINE_LIST(SocketListL);

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
	
	deleted = false;

	SetEventHandler(*theApp.amuledlg,ID_SOKETTI);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG|wxSOCKET_LOST_FLAG);
	Notify(TRUE);
}


CClientReqSocket::~CClientReqSocket()
{
	// remove event handler
	SetNotify(0);
	Notify(FALSE);

	if (client) {
		client->socket = 0;
	}
	client = 0;
	
	if (!deleted) {
		theApp.listensocket->RemoveSocket(this);
	}
	
	//DEBUG_ONLY (theApp.clientlist->Debug_SocketDeleted(this));
}

void CClientReqSocket::ResetTimeOutTimer()
{
	timeout_timer = ::GetTickCount();
}

bool CClientReqSocket::CheckTimeOut()
{
	// eMule 0.30c (Creteil)
	if (::GetTickCount() - timeout_timer > CONNECTION_TIMEOUT) {
		timeout_timer = ::GetTickCount();
		Disconnect();
		return true;
	}
	return false;
}

void CClientReqSocket::OnClose(int nErrorCode)
{
	CEMSocket::OnClose(nErrorCode);
	Disconnect();
}

void CClientReqSocket::Disconnect()
{
	byConnected = ES_DISCONNECTED;
	if (!client) {
		// Kry - No point on calling Safe_Delete - it only sets the deletethis flag		

		theApp.listensocket->RemoveSocket(this);
		deleted = true;
		Destroy();

	} else {
		client->Disconnected();
	}
}

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
	//deltimer = ::GetTickCount();
	// if (m_hSocket != INVALID_SOCKET)
	//  ShutDown(2);
	client = NULL;
	byConnected = ES_DISCONNECTED;
	theApp.listensocket->RemoveSocket(this);
	deleted = true;
	Destroy();
}

bool CClientReqSocket::ProcessPacket(char* packet, uint32 size, uint8 opcode)
{
	try{
		if (!client && opcode != OP_HELLO) {
			throw wxString(wxT("Asks for something without saying hello"));
		}
		switch(opcode) {
			case OP_HELLOANSWER: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessHelloAnswer(packet,size);
				if (client) {
					client->ConnectionEstablished();
				}
				break;
			}
			case OP_HELLO: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				bool bNewClient = !client;				
				if (bNewClient) {
					// create new client to save standart informations
					client = new CUpDownClient(this);
				}
				// client->ProcessHelloPacket(packet,size);

				try{
					client->ProcessHelloPacket(packet,size);
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
					theApp.amuledlg->AddDebugLogLine(true,CString(_("Filtered IP: %s (%s)")).GetData(),client->GetFullIP(),theApp.ipfilter->GetLastHit().GetData());					
					theApp.stat_filteredclients++;
					if (bNewClient){
						delete client;
						client = NULL;
						Disconnect();
					}
					else
						client->Disconnected();
					break;
				}
						
				// now we check if we now this client already. if yes this socket will
				// be attached to the known client, the new client will be deleted
				// and the var. "client" will point to the known client.
				// if not we keep our new-constructed client ;)
				if (theApp.clientlist->AttachToAlreadyKnown(&client,this)) {
					// update the old client informations
					client->ProcessHelloPacket(packet,size);
				} else {
					theApp.clientlist->AddClient(client);
					client->SetCommentDirty();
				}

				// send a response packet with standart informations
				if (client->GetClientSoft() == SO_EMULE) {
					client->SendMuleInfoPacket(false);
				}
				client->SendHelloAnswer();
				if (client) {
					client->ConnectionEstablished();
				}
				break;
			}
			case OP_FILEREQUEST: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				if (size == 16 || (size > 16 && client->GetExtendedRequestsVersion() > 0)) {
					if (!client->GetWaitStartTime()) {
						client->SetWaitStartTime();
					}
					uchar reqfileid[16];
					md4cpy(reqfileid,packet);
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfileid);
					if (!reqfile){
						// if we've just started a download we may want to use that client as a source
						CKnownFile* partfile = theApp.downloadqueue->GetFileByID(reqfileid);
						if (partfile && partfile->IsPartFile()) {
							if (theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)partfile)->GetSourceCount()) {
								if (!((CPartFile*)partfile)->IsStopped()) {
									theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)partfile,client);
								}
							}
						}
					}
					if (!reqfile) {
						// 26-Jul-2003: removed sending of OP_FILEREQANSNOFIL when receiving OP_FILEREQUEST
						//	for better compatibility with ed2k protocol (eDonkeyHybrid) and to save some traffic
						//
						//	*) The OP_FILEREQANSNOFIL _has_ to be sent on receiving OP_SETREQFILEID.
						//	*) The OP_FILEREQANSNOFIL _may_ be sent on receiving OP_FILEREQUEST but in almost all cases 
						//	   it's not needed because the remote client will also send a OP_SETREQFILEID.

						// DbT:FileRequest
						// send file request no such file packet (0x48)
						// Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
						// md4cpy(replypacket->pBuffer, packet);
						// theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->size);
						// SendPacket(replypacket, true);
						// DbT:End
						break;
					}
					// if wer are downloading this file, this could be a new source
					if (reqfile->IsPartFile()) {
						if (theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
							if (!((CPartFile*)reqfile)->IsStopped()) {
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
							}
						}
					}
					// check to see if this is a new file they are asking for
					if(md4cmp(client->GetUploadFileID(), packet) != 0) {
						client->SetCommentDirty();
					}
					// CKnownFile* clientreqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
					// if( clientreqfile )
					// clientreqfile->SubQueuedCount();
					// reqfile->AddQueuedCount();
					// send filename etc
					client->SetUploadFileID((uchar*)packet);
					// md4cpy(client->reqfileid,packet);
					CSafeMemFile* data = new CSafeMemFile (128);
					data->WriteRaw(reqfile->GetFileHash(),16);
					// Kry - Sending the length twice is not a good idea!
					// File name length is writeen on writing a wxString
					//data->Write((uint16)reqfile->GetFileName().Length());
					data->Write(reqfile->GetFileName());
					// TODO: Don't let 'ProcessUpFileStatus' re-process the entire packet and search the fileid
					// again in 'sharedfiles' -> waste of time.
					client->ProcessUpFileStatus(packet,size);
					Packet* packet = new Packet(data);
					packet->opcode = OP_FILEREQANSWER;
					theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
					SendPacket(packet,true);
					client->SendCommentInfo(reqfile);
					delete data;
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;  
			}
			case OP_FILEREQANSNOFIL: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				// DbT:FileRequest
				if (size >= 16) {
					// if that client do not have my file maybe has another different
					CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
					if (!reqfile) {
						break;
					}
					// we try to swap to another file ignoring no needed parts files
					if (client) switch (client->GetDownloadState()) {
						case DS_ONQUEUE:
						case DS_NONEEDEDPARTS:
							if (!client->SwapToAnotherFile(true, true, true, NULL)) {
								theApp.downloadqueue->RemoveSource(client, true);
							}
							break;
					}
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;
				// DbT:End
			}
			case OP_FILEREQANSWER: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				client->ProcessFileInfo(packet,size);
				break;
			}
			case OP_FILESTATUS: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				client->ProcessFileStatus(packet,size);
				break;
			}
			case OP_STARTUPLOADREQ: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if(size == 16) {
					uchar reqfileid[16];
					md4cpy(reqfileid,packet);
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfileid);
					if (reqfile) {
						if (reqfile->IsPartFile()) {
							if(theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
							}
						}
						if(md4cmp(client->GetUploadFileID(), packet) != 0) {
							client->SetCommentDirty();
						}
						client->SetUploadFileID((uchar*)packet);
						client->SendCommentInfo(reqfile);
					}
				}
				theApp.uploadqueue->AddClientToQueue(client);
				break;
			}
			case OP_QUEUERANK: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uint32 rank;
				data.Read(rank);
				client->SetRemoteQueueRank(rank);
				break;
			}
			case OP_ACCEPTUPLOADREQ: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					if (client->GetDownloadState() == DS_ONQUEUE ) {
						client->SetDownloadState(DS_DOWNLOADING);
						client->m_lastPartAsked = 0xffff; // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
						client->SendBlockRequests();
					}
				} else {
					Packet* packet = new Packet(OP_CANCELTRANSFER,0);
					theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
					client->socket->SendPacket(packet,true,true);
					client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_REQUESTPARTS: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar reqfilehash[16];
				data.ReadRaw(reqfilehash,16);
				Requested_Block_Struct* reqblock1 = new Requested_Block_Struct;
				Requested_Block_Struct* reqblock2 = new Requested_Block_Struct;
				Requested_Block_Struct* reqblock3 = new Requested_Block_Struct;
				data.Read(reqblock1->StartOffset);
				data.Read(reqblock2->StartOffset);
				data.Read(reqblock3->StartOffset);
				data.Read(reqblock1->EndOffset);
				data.Read(reqblock2->EndOffset);
				data.Read(reqblock3->EndOffset);
				md4cpy(&reqblock1->FileID,reqfilehash);
				md4cpy(&reqblock2->FileID,reqfilehash);
				md4cpy(&reqblock3->FileID,reqfilehash);

				if (reqblock1->EndOffset-reqblock1->StartOffset == 0) {
					delete reqblock1;
				} else {
					client->AddReqBlock(reqblock1);
				}
				if (reqblock2->EndOffset-reqblock2->StartOffset == 0) {
					delete reqblock2;
				} else {
					client->AddReqBlock(reqblock2);
				}
				if (reqblock3->EndOffset-reqblock3->StartOffset == 0) {
					delete reqblock3;
				} else {
					client->AddReqBlock(reqblock3);
				}
				break;
			}
			case OP_CANCELTRANSFER: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				theApp.uploadqueue->RemoveFromUploadQueue(client);
				theApp.amuledlg->AddDebugLogLine(false, "%s: Upload session ended due canceled transfer.", client->GetUserName());
				client->SetUploadFileID(NULL);
				break;
			}
			case OP_END_OF_DOWNLOAD: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size>=16 && !md4cmp(client->GetUploadFileID(),packet)) {
					theApp.uploadqueue->RemoveFromUploadQueue(client);
					theApp.amuledlg->AddDebugLogLine(false, "%s: Upload session ended due ended transfer.", client->GetUserName());
					client->SetUploadFileID(NULL);
				}
				break;
			}
			case OP_HASHSETREQUEST: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
				}
				client->SendHashsetPacket(packet);
				break;
			}
			case OP_HASHSETANSWER: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				// eMule doesn't throw here for some reason... 
				/*
				if (client->GetDownloadState() != DS_REQHASHSET) {
					throw wxString(wxT("Unwanted hashset"));
				}
				*/
				client->ProcessHashSet(packet,size);
				break;
			}
			case OP_SENDINGPART: {
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					client->ProcessBlockPacket(packet,size);
					if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR) {
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						client->socket->SendPacket(packet,true,true);
						client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);
					}
				} else {
					Packet* packet = new Packet(OP_CANCELTRANSFER,0);
					theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
					client->socket->SendPacket(packet,true,true);
					client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_OUTOFPARTREQS: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (client->GetDownloadState() == DS_DOWNLOADING) {
					client->SetDownloadState(DS_ONQUEUE);
				}
				break;
			}
			case OP_SETREQFILEID: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (client->IsBanned()) {
					break;
				}
				// DbT:FileRequest
				if (size >= 16) {
					if (!client->GetWaitStartTime()) {
						client->SetWaitStartTime();
					}
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
					if (!reqfile) {
						// send file request no such file packet (0x48)
						Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
						memcpy(replypacket->pBuffer, packet, 16);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->size);
						SendPacket(replypacket, true);
						break;
					}
					// if we are downloading this file, this could be a new source
					if (reqfile->IsPartFile()) {
						if( theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount() ) {
							theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
						}
					}
					// check to see if this is a new file they are asking for
					if(md4cmp(client->GetUploadFileID(), packet) != 0) {
						client->SetCommentDirty();
					}
					
					//send filestatus
					client->SetUploadFileID((uchar*)packet);
					CSafeMemFile data(16+16);
					data.WriteRaw(reqfile->GetFileHash(),16);
					if (reqfile->IsPartFile()) {
						((CPartFile*)reqfile)->WritePartStatus(&data);
					} else {
						data.Write((uint16)0); // Shouldn't this be a single (uint16)0 ?
						data.Write((uint8)0);
					}
					Packet* packet = new Packet(&data);
					packet->opcode = OP_FILESTATUS;
					theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
					SendPacket(packet,true);
					break;
				}
				throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
				break;
				// DbT:End
			}
			case OP_CHANGE_CLIENT_ID:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((BYTE*)packet, size);
				uint32 nNewUserID;
				data.Read(nNewUserID);
				uint32 nNewServerIP;
				data.Read(nNewServerIP);
        if (nNewUserID < 16777216) { // client changed server and gots a LowID
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						client->SetUserID(nNewUserID); // update UserID only if we know the server
						client->SetServerIP(nNewServerIP);
						client->SetServerPort(pNewServer->GetPort());
					}
				} else if (nNewUserID == client->GetIP()) { // client changed server and gots a HighID(IP)
					client->SetUserID(nNewUserID);
					CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
					if (pNewServer != NULL){
						client->SetServerIP(nNewServerIP);
						client->SetServerPort(pNewServer->GetPort());
					}
				}
				break;
			}					
			case OP_CHANGE_SLOT:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// sometimes sent by Hybrid
				break;
			}			
			case OP_MESSAGE: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (size < 2) {
					throw wxString(wxT("Invalid message packet"));
				}
				uint16 length;
				memcpy(&length,packet,2);
				ENDIAN_SWAP_I_16(length);
				if ((uint32)length + 2 != size) {
					throw wxString(wxT("Invalid message packet"));
				}
				theApp.amuledlg->AddDebugLogLine(true,"New Message from '%s' (IP:%s)",client->GetUserName(), client->GetFullIP());
				#warning TODO: CHECK MESSAGE FILTERING!
				//filter me?
				if ( (theApp.glob_prefs->MsgOnlyFriends() && !client->IsFriend()) ||
					 (theApp.glob_prefs->MsgOnlySecure() && client->GetUserName()==NULL) ) {
					#if 0
					if (!client->m_bMsgFiltered)
						AddDebugLogLine(false,"Filtered Message from '%s' (IP:%s)",client->GetUserName(), client->GetFullIP());
					client->m_bMsgFiltered=true;
					#endif
					break;
				}
				char* message = new char[length+1];
				memcpy(message,packet+2,length);
				message[length] = '\0';
				theApp.amuledlg->chatwnd->chatselector->ProcessMessage(client,message);
				delete[] message;
				break;
			}
			case OP_ASKSHAREDFILES:	{
				// client wants to know what we have in share, let's see if we allow him to know that
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				CList<void*,void*> list;
				if (theApp.glob_prefs->CanSeeShares() == 0 || // everybody
				(theApp.glob_prefs->CanSeeShares() == 1 && client->IsFriend())) // friend
				{
					CKnownFileMap& filemap = theApp.sharedfiles->m_Files_map;
					for (CKnownFileMap::iterator pos = filemap.begin();pos != filemap.end(); pos++ ) {
						list.AddTail((void*&)pos->second);
					}
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your sharedfiles-list -> %s")).GetData(),client->GetUserName(),client->GetUserID(),CString(_("accepted")).GetData() );
				} else {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your sharedfiles-list -> %s")),client->GetUserName(),client->GetUserID(),CString(_("denied")).GetData());
				}
				// now create the memfile for the packet
				CSafeMemFile tempfile(80);
				tempfile.Write((uint32)list.GetCount());
				while (list.GetCount()) {
					theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile);
					list.RemoveHead();
				}
				// create a packet and send it
				Packet* replypacket = new Packet(&tempfile);
				replypacket->opcode = OP_ASKSHAREDFILESANSWER;
				theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
				SendPacket(replypacket, true, true);
				break;
			}
			case OP_ASKSHAREDFILESANSWER: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessSharedFileList(packet,size);
				break;
			}
			case OP_ASKSHAREDDIRS: {
 				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				
				if (theApp.glob_prefs->CanSeeShares()==0 || (theApp.glob_prefs->CanSeeShares()==1 && client->IsFriend())) {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your shareddirectories-list -> %s")),client->GetUserName(),client->GetUserID(),CString(_("accepted")).GetBuffer());			

					// eMule does other way: it builds the CArray with the folders
					// and then, write it to packet. But we don't need it, anyway.
					// This should work.
					
					// Our packet.
					CSafeMemFile tempfile(80);
					
					// Shared folders...
					uint32 uDirs = theApp.glob_prefs->shareddir_list.GetCount();
					tempfile.Write(uDirs);
					for (uint32 iDir=0; iDir < uDirs; iDir++) {
						wxString strDir(theApp.glob_prefs->shareddir_list[iDir]);
						tempfile.Write(strDir);
					}
					
					// and the incoming folders
					for (uint32 ix=0;ix<theApp.glob_prefs->GetCatCount();ix++) {
						wxString strDir;
						strDir=CString(theApp.glob_prefs->GetCategory(ix)->incomingpath);
						tempfile.Write(strDir);
					}
					
					// Send packet.
					
					Packet* replypacket = new Packet(&tempfile);
					replypacket->opcode = OP_ASKSHAREDDIRSANS;
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
				} else {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your shareddirectories-list -> %s")),client->GetUserName(),client->GetUserID(),CString(_("denied")).GetData());
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
				}

				break;
			}
			case OP_ASKSHAREDFILESDIR: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// IP banned, no answer for this request
				if (client->IsBanned()) {
					break;
				}
				CSafeMemFile data((uchar*)packet, size);
											
				wxString strReqDir;
				data.Read(strReqDir);
				
				if (theApp.glob_prefs->CanSeeShares()==0 || (theApp.glob_prefs->CanSeeShares()==1 && client->IsFriend())) {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your sharedfiles-list for directory %s -> %s")),client->GetUserName(),client->GetUserID(),strReqDir.GetData(),CString(_("accepted")).GetData());
					wxASSERT( data.GetPosition() == data.GetLength() );
					CTypedPtrList<CPtrList, CKnownFile*> list;
					theApp.sharedfiles->GetSharedFilesByDirectory(strReqDir,list);

					CSafeMemFile tempfile(80);
					tempfile.Write(strReqDir);
					tempfile.Write((uint32)list.GetCount());
					while (list.GetCount()) {
						theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile);
						list.RemoveHead();
					}

					Packet* replypacket = new Packet(&tempfile);
					replypacket->opcode = OP_ASKSHAREDFILESDIRANS;
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
				} else {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) requested your sharedfiles-list for directory %s -> %s")),client->GetUserName(),client->GetUserID(),strReqDir.GetData(),CString(_("denied")).GetData());
					Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
				}
				break;
			}		
			
			case OP_ASKSHAREDDIRSANS:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (client->GetFileListRequested() == 1){
					CSafeMemFile data((uchar*)packet, size);
					uint32 uDirs;
					data.Read(uDirs);
					for (UINT i = 0; i < uDirs; i++){
						wxString strDir;
						data.Read(strDir);
						theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) shares directory %s")),client->GetUserName(),client->GetUserID(),strDir.GetData());

						CSafeMemFile tempfile(80);
						tempfile.Write(strDir);
						Packet* replypacket = new Packet(&tempfile);
						replypacket->opcode = OP_ASKSHAREDFILESDIR;
						theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
						SendPacket(replypacket, true, true);
					}
					wxASSERT( data.GetPosition() == data.GetLength() );
					client->SetFileListRequested(uDirs);
				} /*else {
						AddLogLine(true,GetResString(IDS_SHAREDANSW2),client->GetUserName(),client->GetUserID());
				}*/
      	break;
      }
      
		  case OP_ASKSHAREDFILESDIRANS: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data((uchar*)packet, size, 0);
				wxString strDir;
				data.Read(strDir);

				if (client->GetFileListRequested() > 0){
			    		theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) sent sharedfiles-list for directory %s")),client->GetUserName(),client->GetUserID(),strDir.GetData());
					// We need a new ProcessSharedFileList that can handle dirs.
					client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), (char*)strDir.GetData());
					//client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition());
					if (client->GetFileListRequested() == 0) {
						theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) finished sending sharedfiles-list")),client->GetUserName(),client->GetUserID());
					}
				} else {
					theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) sent unwanted sharedfiles-list")),client->GetUserName(),client->GetUserID());
				}
				break;
			}
			case OP_ASKSHAREDDENIEDANS:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				wxASSERT( size == 0 );
				theApp.amuledlg->AddLogLine(true,CString(_("User %s (%u) denied access to shareddirectories/files-list")),client->GetUserName(),client->GetUserID());
				client->SetFileListRequested(0);			
				break;
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				theApp.amuledlg->AddDebugLogLine(false,"Edonkey packet: unknown opcode: %i %x",opcode,opcode);
				break;
		}
		/* catch(CFileException* error) {
			OUTPUT_DEBUG_TRACE();
			error->Delete(); // mf
			throw wxString(wxT("Invalid or corrupted packet received"));
		}*/
	}
	catch(CInvalidPacket) {
		#if 0
		printf("Uncatched invalid packet exception\n");
		#endif
		client->SetDownloadState(DS_ERROR);
		Disconnect();
		return false;
	}
	catch(wxString error) {
		if (client) {
			client->SetDownloadState(DS_ERROR);
			// TODO write this into a debugfile
			theApp.amuledlg->AddDebugLogLine(false,CString(_("Client '%s' (IP:%s) caused an error: %s. Disconnecting client!")),client->GetUserName(),client->GetFullIP(),error.GetData());
		} else {
			theApp.amuledlg->AddDebugLogLine(false,CString(_("A client caused an error or did something bad: %s. Disconnecting client!")),error.GetData());
		}
		Disconnect();
		return false;
	}
	return true;
}

bool CClientReqSocket::ProcessExtPacket(char* packet, uint32 size, uint8 opcode)
{
	try{
		if (!client) {
			throw wxString(wxT("Unknown clients sends extended protocol packet"));
		}
		switch(opcode) {
			case OP_EMULEINFO: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessMuleInfoPacket(packet,size);
				client->SendMuleInfoPacket(true);
				break;
			}
			case OP_EMULEINFOANSWER: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessMuleInfoPacket(packet,size);
				break;
			}
			case OP_SECIDENTSTATE:{
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
				if (client->IsBanned() ){
					break;						
				}
				client->ProcessPublicKeyPacket((uchar*)packet,size);
				break;
			}
 			case OP_SIGNATURE:{
				client->ProcessSignaturePacket((uchar*)packet,size);
				break;
			}		
			case OP_COMPRESSEDPART: {
				if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)) {
					client->ProcessBlockPacket(packet,size,true);
					if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR) {
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						client->socket->SendPacket(packet,true,true);
						client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);
					}
				} else {
					Packet* packet = new Packet(OP_CANCELTRANSFER,0);
					theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
					client->socket->SendPacket(packet,true,true);
					client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
				}
				break;
			}
			case OP_QUEUERANKING: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if (size != 12) {
					throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
				}
				uint16 newrank;
				memcpy(&newrank,packet+0,2);
				ENDIAN_SWAP_I_16(newrank);
				client->SetRemoteQueueFull(false);
				client->SetRemoteQueueRank(newrank);
				break;
			}
 			case OP_REQUESTSOURCES:{
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);
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
						//if not complete and file is rare, allow once every 10 minutes
						( file->IsPartFile() &&
						((CPartFile*)file)->GetSourceCount() <= RARE_FILE * 2 &&
						(bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASK)
						) ||
						//OR if file is not rare or if file is complete, allow every 90 minutes
						( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASK * MINCOMMONPENALTY) )
						)
						{
							client->SetLastSrcReqTime();
							Packet* tosend = file->CreateSrcInfoPacket(client);
							if(tosend) {
								theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->size);
								SendPacket(tosend, true, true);
								theApp.amuledlg->AddDebugLogLine( false, "RCV:Source Request User(%s) File(%s)", client->GetUserName(), file->GetFileName().GetData());
							}
						}
					}
				}
				break;
			}
 			case OP_ANSWERSOURCES: {
				theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);
				CSafeMemFile data((BYTE*)packet,size);
				uchar hash[16];
				data.ReadRaw(hash,16);
				CKnownFile* file = theApp.downloadqueue->GetFileByID((uchar*)packet);
				if(file){
					if (file->IsPartFile()){
						//set the client's answer time
						client->SetLastSrcAnswerTime();
						//and set the file's last answer time
						((CPartFile*)file)->SetLastAnsweredTime();

						if  (!(((CPartFile*)file)->IsStopped())) {
							((CPartFile*)file)->AddClientSources(&data, client->GetSourceExchangeVersion());
						}
					}
				}
				break;
			}
			case OP_FILEDESC: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				client->ProcessMuleCommentPacket(packet,size);
				break;
			}
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				theApp.amuledlg->AddDebugLogLine(false,"eMule packet : unknown opcode: %i %x",opcode,opcode);
				break;
		}
	}
	catch(wxString error) {
		theApp.amuledlg->AddDebugLogLine(false,CString(_("A client caused an error or did something bad: %s. Disconnecting client!")),error.GetData());
		if (client) {
			client->SetDownloadState(DS_ERROR);
		}
		Disconnect();
		return false;
	}
	return true;
}

void CClientReqSocket::OnInit()
{
	// uint8 tv = 1; // commented like in eMule 0.30c (Creteil)
	// SetSockOpt(SO_DONTLINGER,&tv,sizeof(bool));
}

void CClientReqSocket::OnSend(int nErrorCode)
{
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnError(int nErrorCode)
{
	if (theApp.glob_prefs->GetVerbose() && (nErrorCode != 0) && (nErrorCode != 107)) {
		// 0    -> No Error / Disconect
		// 107  -> Transport endpoint is not connected
		if (client) {
			if (client->GetUserName()) {
				theApp.amuledlg->AddLogLine(false,CString(_("Client '%s' (IP:%s) caused an error: %u. Disconnecting client!")),client->GetUserName(),client->GetFullIP(),nErrorCode);
			} else {
				theApp.amuledlg->AddLogLine(false,CString(_("Unknown client (IP:%s) caused an error: %u. Disconnecting client!")),client->GetFullIP(),nErrorCode);
			}
		} else {
			theApp.amuledlg->AddLogLine(false, CString(_("A client caused an error or did something bad (error %u). Disconnecting client !")),nErrorCode);
		}
	}
	Disconnect();
}

CClientReqSocket::CClientReqSocket()
{
}

bool CClientReqSocket::Close()
{
	return wxSocketBase::Close();
}

void CClientReqSocket::PacketReceived(Packet* packet)
{
	switch (packet->prot) {
		case OP_EDONKEYPROT:
			ProcessPacket(packet->pBuffer,packet->size,packet->opcode);
			break;
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()) {
				#if 0
				SOCKADDR_IN sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				int nSockAddrLen = sizeof(sockAddr);
				GetPeerName((SOCKADDR*)&sockAddr,&nSockAddrLen);
				AddDebugLogLine(false,wxT("Failed to decompress client TCP packet; IP=%s  protocol=0x%02x  opcode=0x%02x  size=%u"), inet_ntoa(sockAddr.sin_addr), packet->prot, packet->opcode, packet->size);				
				#endif
				break;
			}
		case OP_EMULEPROT:
			ProcessExtPacket(packet->pBuffer,packet->size,packet->opcode);
			break;
		default: {
			if (client) {
				client->SetDownloadState(DS_ERROR);
			}
			Disconnect();
		}
	}
}

void CClientReqSocket::OnReceive(int nErrorCode)
{
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

IMPLEMENT_DYNAMIC_CLASS(CListenSocket,wxSocketServer)

// CListenSocket
// CListenSocket member functions
CListenSocket::CListenSocket(CPreferences* in_prefs,wxSockAddress& addr)
: wxSocketServer(addr,wxSOCKET_NOWAIT)
{
	app_prefs = in_prefs;
	opensockets = 0;
	maxconnectionreached = 0;
	m_OpenSocketsInterval = 0;
	m_nPeningConnections = 0;
	// bListening = false; // Creteil 0.30c

	SetEventHandler(*theApp.amuledlg,ID_SOKETTI);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG);
	Notify(TRUE);
}

CListenSocket::~CListenSocket()
{
	Discard();
	Close();
	KillAllSockets();
}

bool CListenSocket::StartListening()
{
	bListening = true;
	//return (this->Create(app_prefs->GetPort(),SOCK_STREAM,FD_ACCEPT) && this->Listen());
	return TRUE;
}

void CListenSocket::ReStartListening()
{
	bListening = true;
	if (m_nPeningConnections) {
		m_nPeningConnections--;
		OnAccept(0);
	}
}

void CListenSocket::StopListening()
{
	bListening = false;
	maxconnectionreached++;
}

void CListenSocket::OnAccept(int nErrorCode)
{
	if (!nErrorCode) {
		m_nPeningConnections++;
		if (m_nPeningConnections < 1) {
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
	POSITION pos2;
	m_OpenSocketsInterval = 0;
	opensockets = 0;
	for(POSITION pos1 = socket_list.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		opensockets++;
		cur_sock->CheckTimeOut();
	}
	if (!bListening && (GetOpenSockets()+5 < app_prefs->GetMaxConnections() || theApp.serverconnect->IsConnecting())) {
		ReStartListening();
	}
}

void CListenSocket::RecalculateStats()
{
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
	socket_list.AddTail(toadd);
}

void CListenSocket::RemoveSocket(CClientReqSocket* todel)
{
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
	for (POSITION pos = socket_list.GetHeadPosition();pos != 0;) {
		CClientReqSocket* cur_socket = socket_list.GetNext(pos);
		if (cur_socket->client) {
			cur_socket->client->Destroy();
		} else {
			cur_socket->Destroy();
		}
	}
}

void CListenSocket::AddConnection()
{
	m_OpenSocketsInterval++;
	opensockets++;
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval)
{
	if (GetOpenSockets() > app_prefs->GetMaxConnections() || (m_OpenSocketsInterval > theApp.glob_prefs->GetMaxConperFive() && !bIgnoreInterval)) {
		return true;
	} else {
		return false;
	}
}

bool CListenSocket::IsValidSocket(CClientReqSocket* totest)
{
	return socket_list.Find(totest);
}

void CListenSocket::Debug_ClientDeleted(CUpDownClient* deleted)
{
	POSITION pos1, pos2;
	for (pos1 = socket_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		socket_list.GetNext(pos1);
		#if 0
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		if (!AfxIsValidAddress(cur_sock, sizeof(CClientReqSocket))) {
			AfxDebugBreak();
		}
		if (cur_sock->client == deleted) {
			AfxDebugBreak();
		}
		#endif
	}
}
