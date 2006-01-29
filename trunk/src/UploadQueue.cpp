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

#include <algorithm>
#include <cstring>
#include <cmath>

#include "Types.h"

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "UploadQueue.h"	// Interface declarations
#include "ServerList.h"		// Needed for CServerList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "Server.h"		// Needed for CServer
#include "ServerConnect.h"	// Needed for CServerConnect
#include "KnownFile.h"		// Needed for CKnownFile
#include "Packet.h"		// Needed for CPacket
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "OPCodes.h"		// Needed for MAX_PURGEQUEUETIME
#include "updownclient.h"	// Needed for CUpDownClient
#include "GetTickCount.h"	// Needed for GetTickCount
#include "amule.h"		// Needed for theApp
#include "Preferences.h"
#include "ClientList.h"
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>
#include "UploadBandwidthThrottler.h"

#include <numeric>

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue()
{
	m_nLastStartUpload = 0;

	lastupslotHighID = true;
}


void CUploadQueue::AddUpNextClient(CUpDownClient* directadd)
{
	CClientPtrList::iterator toadd = m_waitinglist.end();
	CClientPtrList::iterator toaddlow = m_waitinglist.end();
	
	sint64	bestscore = -1;
	sint64	bestlowscore = -1;

	CUpDownClient* newclient;
	// select next client or use given client
	if (!directadd) {
		// Track if we purged any clients from the queue, as to only send one notify in total
		bool purged = false;
		
		CClientPtrList::iterator it = m_waitinglist.begin();
		for (; it != m_waitinglist.end(); ) {
			CClientPtrList::iterator tmp_it = it++;
			CUpDownClient* cur_client = *tmp_it;

			// clear dead clients
			if ( (::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ) {
				purged = true;
				cur_client->ClearWaitStartTime();
				RemoveFromWaitingQueue(tmp_it);
				if (!cur_client->GetSocket()) {
					if(cur_client->Disconnected(wxT("AddUpNextClient - purged"))) {
						cur_client->Safe_Delete();
						cur_client = NULL;
					}
				}
				continue;
			} 

			suspendlist::iterator it2 = std::find( suspended_uploads_list.begin(),
			                                      suspended_uploads_list.end(),
			                                      cur_client->GetUploadFileID() );
			if (cur_client->IsBanned() || it2 != suspended_uploads_list.end() ) { // Banned client or suspended upload ?
			        continue;
			}
			// finished clearing
			
			sint64 cur_score = cur_client->GetScore(true);
			if (cur_score > bestscore) {
				bestscore = cur_score;
				toadd = tmp_it;
			} else {
				cur_score = cur_client->GetScore(false);
				if ((cur_score > bestlowscore) && !cur_client->m_bAddNextConnect){
					bestlowscore = cur_score;
					toaddlow = tmp_it;
				}
			}
		}

		// Update the count on GUI if any clients were purged
		if (purged) {
			Notify_ShowQueueCount(m_waitinglist.size());
		}

		if (bestlowscore > bestscore){
			newclient = *toaddlow;
			newclient->m_bAddNextConnect = true;
		}

		if (toadd == m_waitinglist.end()) {
			return;
		}
		
		newclient = *toadd;
		lastupslotHighID = true; // VQB LowID alternate
		RemoveFromWaitingQueue(toadd);
		Notify_ShowQueueCount(m_waitinglist.size());
	} else {
		//prevent another potential access of a suspended upload

		suspendlist::iterator it = std::find( suspended_uploads_list.begin(),
		                                      suspended_uploads_list.end(),
		                                      directadd->GetUploadFileID() );
		if ( it != suspended_uploads_list.end() ) {
			return;
		} else {
			newclient = directadd;
		}
	}

	if (IsDownloading(newclient)) {
		return;
	}
	// tell the client that we are now ready to upload
	if (!newclient->IsConnected()) {
		newclient->SetUploadState(US_CONNECTING);
		if (!newclient->TryToConnect(true)) {
			return;
		}
	} else {
		CPacket* packet = new CPacket(OP_ACCEPTUPLOADREQ, 0, OP_EDONKEYPROT);
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ to ") + newclient->GetFullIP() );
		newclient->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();

	theApp.uploadBandwidthThrottler->AddToStandardList(m_uploadinglist.size(), newclient->GetSocket());
	m_uploadinglist.push_back(newclient);
	theStats::AddUploadingClient();

	// Statistic
	CKnownFile* reqfile = (CKnownFile*) newclient->GetUploadFile();
	if (reqfile) {
		reqfile->statistic.AddAccepted();
	}
	Notify_UploadCtrlAddClient(newclient);

}

void CUploadQueue::Process()
{
	if (AcceptNewClient() && not m_waitinglist.empty()) {
		m_nLastStartUpload = ::GetTickCount();
		AddUpNextClient();
	}

	// The loop that feeds the upload slots with data.
	CClientPtrList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = *it++;
		
		// It seems chatting or friend slots can get stuck at times in upload.. This needs looked into..
		if (!cur_client->GetSocket()) {
			RemoveFromUploadQueue(cur_client, true);
			if(cur_client->Disconnected(_T("CUploadQueue::Process"))){
				cur_client->Safe_Delete();
			}
		} else {
			cur_client->SendBlockData();
		}
	}

	// Save used bandwidth for speed calculations
	uint64 sentBytes = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
	(void)theApp.uploadBandwidthThrottler->GetNumberOfSentBytesOverheadSinceLastCallAndReset();

	// Update statistics
	if (sentBytes) {
		theStats::AddSentBytes(sentBytes);
	}
}


bool CUploadQueue::AcceptNewClient()
{
	// check if we can allow a new client to start downloading from us
	if (::GetTickCount() - m_nLastStartUpload < 1000 || m_uploadinglist.size() >= MAX_UP_CLIENTS_ALLOWED) {
		return false;
	}

	float kBpsUpPerClient = (float)thePrefs::GetSlotAllocation();
	float kBpsUp = theStats::GetUploadRate() / 1024.0f;
	if (thePrefs::GetMaxUpload() == UNLIMITED) {
		if (m_uploadinglist.size() < ((uint32)((kBpsUp)/kBpsUpPerClient)+2)) {
			return true;
		}
	} else {
		uint16 nMaxSlots = 0;
		if (thePrefs::GetMaxUpload() >= 10) {
			nMaxSlots = (uint16)floor((float)thePrefs::GetMaxUpload() / kBpsUpPerClient + 0.5);
				// floor(x + 0.5) is a way of doing round(x) that works with gcc < 3 ...
			if (nMaxSlots < MIN_UP_CLIENTS_ALLOWED) {
				nMaxSlots=MIN_UP_CLIENTS_ALLOWED;
			}
		} else {
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED;
		}

		if (m_uploadinglist.size() < nMaxSlots) {
			return true;
		}
	}
	return false;
}


CUploadQueue::~CUploadQueue()
{
	wxASSERT(m_waitinglist.empty());
	wxASSERT(m_uploadinglist.empty());
}


bool CUploadQueue::IsOnUploadQueue(CUpDownClient* client) const
{
	return std::find(m_waitinglist.begin(), m_waitinglist.end(), client)
		!= m_waitinglist.end();
}


bool CUploadQueue::IsDownloading(CUpDownClient* client) const
{
	return std::find(m_uploadinglist.begin(), m_uploadinglist.end(), client)
		!= m_uploadinglist.end();
}	


CUpDownClient* CUploadQueue::GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort)
{
	CClientPtrList::iterator it = m_waitinglist.begin();
	for (; it != m_waitinglist.end(); ++it) {
		CUpDownClient* cur_client = *it;
		
		if ((dwIP == cur_client->GetIP()) && (nUDPPort == cur_client->GetUDPPort())) {
			return cur_client;
		}
	}
	
	return NULL;
}


void CUploadQueue::AddClientToQueue(CUpDownClient* client)
{
	if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() && !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort()) && client->GetDownloadState() == DS_NONE && !client->IsFriend() && theStats::GetWaitingUserCount() > 50) {
		// Well, all that issues finish in the same: don't allow to add to the queue
		return;
	}

	if ( client->IsBanned() ) {
		return;
	}

	client->AddAskedCount();
	client->SetLastUpRequest();

	// Find all clients with the same user-hash
	CClientList::SourceList found = theApp.clientlist->GetClientsByHash( client->GetUserHash() );

	CClientList::SourceList::iterator it = found.begin();
	while (it != found.end()) {
		CUpDownClient* cur_client = *it++;
		
		if ( IsOnUploadQueue( cur_client ) ) {
			if ( cur_client == client ) {
				if ( client->m_bAddNextConnect && ( ( m_uploadinglist.size() < thePrefs::GetMaxUpload() ) || ( thePrefs::GetMaxUpload() == UNLIMITED ) ) ) {
					if (lastupslotHighID) {
						client->m_bAddNextConnect = false;
						RemoveFromWaitingQueue(client, true);
						AddUpNextClient(client);
						lastupslotHighID = false; // LowID alternate
						return;
					}
				}

				client->SendRankingInfo();
				Notify_QlistRefreshClient(client);
				return;

			} else {
				// Hash-clash, remove unidentified clients (possibly both)
				
				if ( !cur_client->IsIdentified() ) {
					// Cur_client isn't identifed, remove it
					theApp.clientlist->AddTrackClient( cur_client );

					RemoveFromWaitingQueue( cur_client );
					if ( !cur_client->GetSocket() ) {
						if (cur_client->Disconnected( wxT("AddClientToQueue - same userhash") ) ) {
							cur_client->Safe_Delete();
						}
					}
				}

				if ( !client->IsIdentified() ) {
					// New client isn't identified, remove it
					theApp.clientlist->AddTrackClient( client );

					if ( !client->GetSocket() ) {
						if ( client->Disconnected( wxT("AddClientToQueue - same userhash") ) ) {
							client->Safe_Delete();
						}
					}

					return;
				}
			}
		}
	}

	// Count the number of clients with the same IP-address
	found = theApp.clientlist->GetClientsByIP( client->GetIP() );

	int ipCount = 0;
	for ( it = found.begin(); it != found.end(); it++ ) {
		if ( ( *it == client ) || IsOnUploadQueue( *it ) ) {
			ipCount++;
		}
	}

	// We do not accept more than 3 clients from the same IP
	if ( ipCount > 3 ) {
		return;
	} else if ( theApp.clientlist->GetClientsFromIP(client->GetIP()) >= 3 ) {
		return;
	}

	// statistic values
	CKnownFile* reqfile = (CKnownFile*) client->GetUploadFile();
	if (reqfile) {
		reqfile->statistic.AddRequest();
	}

	// TODO find better ways to cap the list
	if (m_waitinglist.size() >= (thePrefs::GetQueueSize())) {
		return;
	}

	if (client->IsDownloading()) {
		// he's already downloading and wants probably only another file
		CPacket* packet = new CPacket(OP_ACCEPTUPLOADREQ, 0, OP_EDONKEYPROT);
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ to ") + client->GetFullIP() );
		client->SendPacket(packet,true);
		return;
	}

	if (m_waitinglist.empty() && AcceptNewClient()) {
		AddUpNextClient(client);
		m_nLastStartUpload = ::GetTickCount();
	} else {
		m_waitinglist.push_back(client);
		theStats::AddWaitingClient();
		client->ClearWaitStartTime();
		client->ClearAskedCount();
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		Notify_QlistAddClient(client);
		Notify_ShowQueueCount(m_waitinglist.size());
	}
}


bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, bool updatewindow)
{
	// Keep track of this client
	theApp.clientlist->AddTrackClient(client);
	
	CClientPtrList::iterator it = std::find(m_uploadinglist.begin(),
			m_uploadinglist.end(), client);
	
	if (it != m_uploadinglist.end()) {
		if (updatewindow) {
			Notify_UploadCtrlRemoveClient(client);
		}
		m_uploadinglist.erase(it);
		theStats::RemoveUploadingClient();
		if( client->GetTransferredUp() ) {
			theStats::AddSuccessfulUpload();
			theStats::AddUploadTime(client->GetUpStartTimeDelay() / 1000);
		} else {
			theStats::AddFailedUpload();
		}
		client->SetUploadState(US_NONE);
		client->ClearUploadBlockRequests();
		return true;
	}

	return false;
}


bool CUploadQueue::CheckForTimeOver(CUpDownClient* client)
{
	if (thePrefs::TransferFullChunks()) {
		if( client->GetUpStartTimeDelay() > 3600000 ) { // Try to keep the clients from downloading forever.
			return true;
		}
		// For some reason, some clients can continue to download after a chunk size.
		// Are they redownloading the same chunk over and over????
		if( client->GetSessionUp() > 10485760 ) {
			return true;
		}
	} else {
		CClientPtrList::iterator it = m_waitinglist.begin();
		for (; it != m_waitinglist.end(); ++it ) {
			if (client->GetScore(true,true) < (*it)->GetScore(true,false)) {
				return true;
			}
		}
	}
	
	return false;
}


uint16 CUploadQueue::GetWaitingPosition(CUpDownClient* client)
{
	if ( !IsOnUploadQueue(client) ) {
		return 0;
	}

	uint16 rank = 1;
	const uint32 myscore = client->GetScore(false);
	CClientPtrList::iterator it = m_waitinglist.begin();
	for (; it != m_waitinglist.end(); ++it) {
		if ((*it)->GetScore(false) > myscore) {
			rank++;
		}
	}
	
	return rank;
}


/*
 * This function removes a file indicated by filehash from suspended_uploads_list.
 */
void CUploadQueue::ResumeUpload( const CMD4Hash& filehash )
{
	//Find the position of the filehash in the list and remove it.
	suspendlist::iterator it = std::find( suspended_uploads_list.begin(), 
			                              suspended_uploads_list.end(),
			                              filehash );
	if ( it != suspended_uploads_list.end() )
		suspended_uploads_list.erase( it );
	
	AddLogLineM( false, CFormat( _("Resuming uploads of file: %s" ) )
				% filehash.Encode() );
}

/*
 * This function adds a file indicated by filehash to suspended_uploads_list
 */
void CUploadQueue::SuspendUpload( const CMD4Hash& filehash )
{
	AddLogLineM( false, CFormat( _("Suspending upload of file: %s" ) )
				% filehash.Encode() );

	//Append the filehash to the list.
	suspended_uploads_list.push_back(filehash);
	wxString base16hash = filehash.Encode();

	CClientPtrList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		CUpDownClient *potential = *it++;
		//check if the client is uploading the file we need to suspend
		if(potential->GetUploadFileID() == filehash) {
			//remove the unlucky client from the upload queue and add to the waiting queue
			RemoveFromUploadQueue(potential, true);

			m_waitinglist.push_back(potential);
			theStats::AddWaitingClient();
			potential->SetUploadState(US_ONUPLOADQUEUE);
			potential->SendRankingInfo();
			Notify_QlistRefreshClient(potential);
			Notify_ShowQueueCount(m_waitinglist.size());
		}
	}
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow)
{
	CClientPtrList::iterator it = std::find(m_waitinglist.begin(),
			m_waitinglist.end(), client);
	
	if (it != m_waitinglist.end()) {
		RemoveFromWaitingQueue(it);
		if (updatewindow) {
			Notify_ShowQueueCount(m_waitinglist.size());
		}
		return true;
	} else {
		return false;
	}
}


void CUploadQueue::RemoveFromWaitingQueue(CClientPtrList::iterator pos)
{
	CUpDownClient* todelete = *pos;
	m_waitinglist.erase(pos);
	theStats::RemoveWaitingClient();
	if( todelete->IsBanned() ) {
		todelete->UnBan();
	}
	Notify_QlistRemoveClient(todelete);
	todelete->SetUploadState(US_NONE);
}

