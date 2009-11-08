//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "UploadQueue.h"	// Interface declarations

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <common/Macros.h>
#include <common/Constants.h>

#include <cmath>

#include "Types.h"		// Do_not_auto_remove (win32)

#ifdef __WXMSW__
	#include <winsock.h>	// Do_not_auto_remove
#else
	#include <sys/types.h>	// Do_not_auto_remove
	#include <netinet/in.h>	// Do_not_auto_remove
	#include <arpa/inet.h>	// Do_not_auto_remove
#endif

#include "ServerConnect.h"	// Needed for CServerConnect
#include "KnownFile.h"		// Needed for CKnownFile
#include "Packet.h"		// Needed for CPacket
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "updownclient.h"	// Needed for CUpDownClient
#include "amule.h"		// Needed for theApp
#include "Preferences.h"
#include "ClientList.h"
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>
#include "UploadBandwidthThrottler.h"
#include "GuiEvents.h"		// Needed for Notify_*


//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue()
{
	m_nLastStartUpload = 0;
	lastupslotHighID = true;
	m_allowKicking = true;
}


void CUploadQueue::AddUpNextClient(CUpDownClient* directadd)
{
	CClientPtrList::iterator toadd = m_waitinglist.end();
	CClientPtrList::iterator toaddlow = m_waitinglist.end();
	
	uint32_t bestscore = 0;
	uint32_t bestlowscore = 0;

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
			if ( (::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp->sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ) {
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
			
			uint32_t cur_score = cur_client->GetScore(true);
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

	theApp->uploadBandwidthThrottler->AddToStandardList(m_uploadinglist.size(), newclient->GetSocket());
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
	// Check if someone's waiting, if there is a slot for him, 
	// or if we should try to free a slot for him
	uint32 tick = GetTickCount();
	// Nobody waiting or upload started recently
	// (Actually instead of "empty" it should check for "no HighID clients queued",
	//  but the cost for that outweights the benefit. As it is, a slot will be freed
	//  even if it can't be taken because all of the queue is LowID. But just one,
	//  and the kicked client will instantly get it back if he has HighID.)
	if (m_waitinglist.empty() || tick - m_nLastStartUpload < 1000) {
		m_allowKicking = false;
	// Already a slot free, try to fill it
	} else if (m_uploadinglist.size() < GetMaxSlots()) {
		m_allowKicking = false;
		m_nLastStartUpload = tick;
		AddUpNextClient();
	// All slots taken, try to free one
	} else {
		m_allowKicking = true;
	}

	// The loop that feeds the upload slots with data.
	CClientPtrList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = *it++;
		
		// It seems chatting or friend slots can get stuck at times in upload.. This needs looked into..
		if (!cur_client->GetSocket()) {
			RemoveFromUploadQueue(cur_client);
			if(cur_client->Disconnected(_T("CUploadQueue::Process"))){
				cur_client->Safe_Delete();
			}
		} else {
			cur_client->SendBlockData();
		}
	}

	// Save used bandwidth for speed calculations
	uint64 sentBytes = theApp->uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
	(void)theApp->uploadBandwidthThrottler->GetNumberOfSentBytesOverheadSinceLastCallAndReset();

	// Update statistics
	if (sentBytes) {
		theStats::AddSentBytes(sentBytes);
	}
}


uint16 CUploadQueue::GetMaxSlots() const
{
	uint16 nMaxSlots = 0;
	float kBpsUpPerClient = (float)thePrefs::GetSlotAllocation();
	if (thePrefs::GetMaxUpload() == UNLIMITED) {
		float kBpsUp = theStats::GetUploadRate() / 1024.0f;
		nMaxSlots = (uint16)(kBpsUp / kBpsUpPerClient) + 2;
	} else {
		if (thePrefs::GetMaxUpload() >= 10) {
			nMaxSlots = (uint16)floor((float)thePrefs::GetMaxUpload() / kBpsUpPerClient + 0.5);
				// floor(x + 0.5) is a way of doing round(x) that works with gcc < 3 ...
			if (nMaxSlots < MIN_UP_CLIENTS_ALLOWED) {
				nMaxSlots=MIN_UP_CLIENTS_ALLOWED;
			}
		} else {
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED;
		}
	}
	if (nMaxSlots > MAX_UP_CLIENTS_ALLOWED) {
		nMaxSlots = MAX_UP_CLIENTS_ALLOWED;
	}
	return nMaxSlots;
}


CUploadQueue::~CUploadQueue()
{
	wxASSERT(m_waitinglist.empty());
	wxASSERT(m_uploadinglist.empty());
}


bool CUploadQueue::IsOnUploadQueue(const CUpDownClient* client) const
{
	return std::find(m_waitinglist.begin(), m_waitinglist.end(), client)
		!= m_waitinglist.end();
}


bool CUploadQueue::IsDownloading(CUpDownClient* client) const
{
	return std::find(m_uploadinglist.begin(), m_uploadinglist.end(), client)
		!= m_uploadinglist.end();
}	


CUpDownClient* CUploadQueue::GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs)
{
	CUpDownClient* pMatchingIPClient = NULL;
	
	int cMatches = 0;
	
	CClientPtrList::iterator it = m_waitinglist.begin();
	for (; it != m_waitinglist.end(); ++it) {
		CUpDownClient* cur_client = *it;
		
		if ((dwIP == cur_client->GetIP()) && (nUDPPort == cur_client->GetUDPPort())) {
			return cur_client;
		} else if ((dwIP == cur_client->GetIP()) && bIgnorePortOnUniqueIP) {
			pMatchingIPClient = cur_client;
			cMatches++;
		}
	}

	if (pbMultipleIPs) {
		*pbMultipleIPs = cMatches > 1;
	}
	
	if (pMatchingIPClient && cMatches == 1) {
		return pMatchingIPClient;	
	} else {
		return NULL;
	}
}


void CUploadQueue::AddClientToQueue(CUpDownClient* client)
{
	if (theApp->serverconnect->IsConnected() && theApp->serverconnect->IsLowID() && !theApp->serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort()) && client->GetDownloadState() == DS_NONE && !client->IsFriend() && theStats::GetWaitingUserCount() > 50) {
		// Well, all that issues finish in the same: don't allow to add to the queue
		return;
	}

	if ( client->IsBanned() ) {
		return;
	}

	client->AddAskedCount();
	client->SetLastUpRequest();

	// Find all clients with the same user-hash
	CClientList::SourceList found = theApp->clientlist->GetClientsByHash( client->GetUserHash() );

	CClientList::SourceList::iterator it = found.begin();
	while (it != found.end()) {
		CUpDownClient* cur_client = *it++;
		
		if ( IsOnUploadQueue( cur_client ) ) {
			if ( cur_client == client ) {
				// This is where LowID clients get their upload slot assigned.
				// They can't be contacted if they reach top of the queue, so they are just marked for uploading.
				// When they reconnect next time AddClientToQueue() is called, and they get their slot
				// through the connection they initiated.
				// Since at that time no slot is free they get assigned an extra slot,
				// so then the number of slots exceeds the configured number by one.
				// To prevent a further increase no more LowID clients get a slot, until 
				// - a HighID client has got one (which happens only after two clients 
				//   have been kicked so a slot is free again)
				// - or there is a free slot, which means there is no HighID client on queue
				if (client->m_bAddNextConnect) {
					uint16 maxSlots = GetMaxSlots();
					if (lastupslotHighID) {
						maxSlots++;
					}
					if (m_uploadinglist.size() < maxSlots) {
						client->m_bAddNextConnect = false;
						RemoveFromWaitingQueue(client);
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
					theApp->clientlist->AddTrackClient( cur_client );

					RemoveFromWaitingQueue( cur_client );
					if ( !cur_client->GetSocket() ) {
						if (cur_client->Disconnected( wxT("AddClientToQueue - same userhash") ) ) {
							cur_client->Safe_Delete();
						}
					}
				}

				if ( !client->IsIdentified() ) {
					// New client isn't identified, remove it
					theApp->clientlist->AddTrackClient( client );

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
	found = theApp->clientlist->GetClientsByIP( client->GetIP() );

	int ipCount = 0;
	for ( it = found.begin(); it != found.end(); it++ ) {
		if ( ( *it == client ) || IsOnUploadQueue( *it ) ) {
			ipCount++;
		}
	}

	// We do not accept more than 3 clients from the same IP
	if ( ipCount > 3 ) {
		return;
	} else if ( theApp->clientlist->GetClientsFromIP(client->GetIP()) >= 3 ) {
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

	uint32 tick = GetTickCount();
	client->ClearWaitStartTime();
	// if possible start upload right away
	if (m_waitinglist.empty() && tick - m_nLastStartUpload >= 1000 && m_uploadinglist.size() < GetMaxSlots()) {
		AddUpNextClient(client);
		m_nLastStartUpload = tick;
	} else {
		m_waitinglist.push_back(client);
		theStats::AddWaitingClient();
		client->ClearAskedCount();
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		Notify_QlistAddClient(client);
		Notify_ShowQueueCount(m_waitinglist.size());
	}
}


bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client)
{
	// Keep track of this client
	theApp->clientlist->AddTrackClient(client);
	
	CClientPtrList::iterator it = std::find(m_uploadinglist.begin(),
			m_uploadinglist.end(), client);
	
	if (it != m_uploadinglist.end()) {
		Notify_UploadCtrlRemoveClient(client);
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
	// Don't kick anybody if there's no need to
	if (!m_allowKicking) {
		return false;
	}
	// First, check if it is a VIP slot (friend or Release-Prio).
	if (client->GetFriendSlot()) {
		return false;	// never drop the friend
	}
	// Release-Prio and nobody on queue for it?
	if (client->GetUploadFile()->GetUpPriority() == PR_POWERSHARE) {
		// Keep it unless half of the UL slots are occupied with friends or Release uploads.
		uint16 vips = 0;
		for (CClientPtrList::iterator it = m_uploadinglist.begin(); it != m_uploadinglist.end(); ++it) {
			CUpDownClient* cur_client = *it;
			if (cur_client->GetFriendSlot() || cur_client->GetUploadFile()->GetUpPriority() == PR_POWERSHARE) {
				vips++;
			}
		}
		// allow if VIP uploads occupy at most half of the possible upload slots
		if (vips <= GetMaxSlots() / 2) {
			return false;
		}
		// Otherwise normal rules apply.
	}

	// Ordinary slots
	bool kickHim = false;

	if (thePrefs::TransferFullChunks()) {
		// "Transfer full chunks": drop client after 10 MB upload, or after an hour.
		// (so average UL speed should at least be 2.84 kB/s)
		// We don't track what he is downloading, but if it's all from one chunk he gets it.
		if (client->GetUpStartTimeDelay() > 3600000 	// time: 1h
			|| client->GetSessionUp() > 10485760) {		// data: 10MB
			kickHim = true;
		}
	} else {
		uint32 clientScore = client->GetScore(true,true);
		CClientPtrList::iterator it = m_waitinglist.begin();
		for (; it != m_waitinglist.end(); ++it ) {
			if (clientScore < (*it)->GetScore(true,false)) {
				kickHim = true;
				break;
			}
		}
	}

	if (kickHim) {
		m_allowKicking = false;		// kick max one client per cycle
	}
	
	return kickHim;
}


uint16 CUploadQueue::GetWaitingPosition(const CUpDownClient *client) const
{
	if ( !IsOnUploadQueue(client) ) {
		return 0;
	}

	uint16 rank = 1;
	const uint32 myscore = client->GetScore(false);
	CClientPtrList::const_iterator it = m_waitinglist.begin();
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
uint16 CUploadQueue::SuspendUpload( const CMD4Hash& filehash )
{
	AddLogLineM( false, CFormat( _("Suspending upload of file: %s" ) )
				% filehash.Encode() );
	uint16 removed = 0;

	//Append the filehash to the list.
	suspended_uploads_list.push_back(filehash);
	wxString base16hash = filehash.Encode();

	CClientPtrList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		CUpDownClient *potential = *it++;
		//check if the client is uploading the file we need to suspend
		if(potential->GetUploadFileID() == filehash) {
			//remove the unlucky client from the upload queue and add to the waiting queue
			RemoveFromUploadQueue(potential);

			m_waitinglist.push_back(potential);
			theStats::AddWaitingClient();
			potential->SetUploadState(US_ONUPLOADQUEUE);
			potential->SendRankingInfo();
			Notify_QlistRefreshClient(potential);
			Notify_ShowQueueCount(m_waitinglist.size());
			removed++;
		}
	}
	return removed;
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client)
{
	CClientPtrList::iterator it = std::find(m_waitinglist.begin(),
			m_waitinglist.end(), client);
	
	if (it != m_waitinglist.end()) {
		RemoveFromWaitingQueue(it);
		Notify_ShowQueueCount(m_waitinglist.size());
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

// File_checked_for_headers
