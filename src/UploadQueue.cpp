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
#include "ListenSocket.h"


//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue()
{
	m_nLastStartUpload = 0;
	m_lastSort = 0;
	lastupslotHighID = true;
	m_allowKicking = true;
	m_allUploadingKnownFile = new CKnownFile;
}


CUpDownClient* CUploadQueue::SortGetBestClient(bool sortonly)
{
	CUpDownClient* newclient = NULL;
	uint32 tick = GetTickCount();
	m_lastSort = tick;
	CClientRefList::iterator it = m_waitinglist.begin();
	for (; it != m_waitinglist.end(); ) {
		CClientRefList::iterator it2 = it++;
		CUpDownClient* cur_client = it2->GetClient();

		// clear dead clients
		if (tick - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME 
			|| !theApp->sharedfiles->GetFileByID(cur_client->GetUploadFileID())) {
			cur_client->ClearWaitStartTime();
			RemoveFromWaitingQueue(it2);
			if (!cur_client->GetSocket()) {
				if (cur_client->Disconnected(wxT("AddUpNextClient - purged"))) {
					cur_client->Safe_Delete();
					cur_client = NULL;
				}
			}
			continue;
		} 

		if (cur_client->IsBanned() || IsSuspended(cur_client->GetUploadFileID())) { // Banned client or suspended upload ?
			cur_client->ClearScore();
			continue;
		}
		// finished clearing

		// Calculate score of current client
		uint32 cur_score = cur_client->CalculateScore();
		// Check if it's better than that of a previous one, and move it up then.
		CClientRefList::iterator it1 = it2;
		while (it1 != m_waitinglist.begin()) {
			it1--;
			if (cur_score > it1->GetClient()->GetScore()) {
				// swap them
				std::swap(*it2, *it1);
				it2--;
			} else {
				// no need to check further since list is already sorted
				break;
			}
		}
	}

	// Second Pass:
	// - calculate queue rank
	// - find best high id client
	// - mark all better low id clients as enabled for upload
	uint16 rank = 1;
	for (it = m_waitinglist.begin(); it != m_waitinglist.end(); ) {
		CClientRefList::iterator it2 = it++;
		CUpDownClient* cur_client = it2->GetClient();
		cur_client->SetUploadQueueWaitingPosition(rank++);
		if (newclient) {
			// There's a better high id client
			cur_client->m_bAddNextConnect = false;
		} else {
			if (cur_client->HasLowID() && !cur_client->IsConnected()) {
				// No better high id client, so start upload to this one once it connects
				cur_client->m_bAddNextConnect = true;
			} else {
				// We found a high id client (or a currently connected low id client)
				newclient = cur_client;
				cur_client->m_bAddNextConnect = false;
				if (!sortonly) {
					RemoveFromWaitingQueue(it2);
					rank--;
					lastupslotHighID = true; // VQB LowID alternate
				}
			}
		}
	}

#ifdef __DEBUG__
	AddDebugLogLineN(logLocalClient, CFormat(wxT("Current UL queue (%d):")) % (rank - 1));
	for (it = m_waitinglist.begin(); it != m_waitinglist.end(); it++) {
		CUpDownClient* c = it->GetClient();
		AddDebugLogLineN(logLocalClient, CFormat(wxT("%4d %7d  %s %5d  %s"))
			% c->GetUploadQueueWaitingPosition()
			% c->GetScore()
			% (c->HasLowID() ? (c->IsConnected() ? wxT("LoCon") : wxT("LowId")) : wxT("High "))
			% c->ECID()
			% c->GetUserName()
			);
	}
#endif	// __DEBUG__

	return newclient;
}


void CUploadQueue::AddUpNextClient(CUpDownClient* directadd)
{
	CUpDownClient* newclient = NULL;
	// select next client or use given client
	if (!directadd) {
		newclient = SortGetBestClient(false);
		if (!newclient) {
			return;
		}
	} else {
		// Check if requested file is suspended or not shared (maybe deleted recently)

		if (IsSuspended(directadd->GetUploadFileID())
			|| !theApp->sharedfiles->GetFileByID(directadd->GetUploadFileID())) {
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
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ to ") + newclient->GetFullIP() );
		newclient->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();

	theApp->uploadBandwidthThrottler->AddToStandardList(m_uploadinglist.size(), newclient->GetSocket());
	m_uploadinglist.push_back(CCLIENTREF(newclient, wxT("CUploadQueue::AddUpNextClient")));
	m_allUploadingKnownFile->AddUploadingClient(newclient);
	theStats::AddUploadingClient();

	// Statistic
	CKnownFile* reqfile = (CKnownFile*) newclient->GetUploadFile();
	if (reqfile) {
		reqfile->statistic.AddAccepted();
	}

	Notify_SharedCtrlRefreshClient(newclient->ECID(), AVAILABLE_SOURCE);
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
	// Also, if we are running out of sockets, don't add new clients, but also don't kick existing ones,
	// or uploading will cease right away.
	if (m_waitinglist.empty() || tick - m_nLastStartUpload < 1000
		|| theApp->listensocket->TooManySockets()) {
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
	CClientRefList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = it++->GetClient();
		
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

	// Periodically resort queue if it doesn't happen anyway
	if ((sint32) (tick - m_lastSort) > MIN2MS(2)) {
		SortGetBestClient(true);
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
	delete m_allUploadingKnownFile;
}


bool CUploadQueue::IsOnUploadQueue(const CUpDownClient* client) const
{
	for (CClientRefList::const_iterator it = m_waitinglist.begin(); it != m_waitinglist.end(); it++) {
		if (it->GetClient() == client) {
			return true;
		}
	}
	return false;
}


bool CUploadQueue::IsDownloading(const CUpDownClient* client) const
{
	for (CClientRefList::const_iterator it = m_uploadinglist.begin(); it != m_uploadinglist.end(); it++) {
		if (it->GetClient() == client) {
			return true;
		}
	}
	return false;
}	


CUpDownClient* CUploadQueue::GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs)
{
	CUpDownClient* pMatchingIPClient = NULL;
	
	int cMatches = 0;
	
	CClientRefList::iterator it = m_waitinglist.begin();
	for (; it != m_waitinglist.end(); ++it) {
		CUpDownClient* cur_client = it->GetClient();
		
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
		CUpDownClient* cur_client = it++->GetClient();
		
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
				Notify_SharedCtrlRefreshClient(client->ECID(), AVAILABLE_SOURCE);
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
		if ( ( it->GetClient() == client ) || IsOnUploadQueue( it->GetClient() ) ) {
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

	if (client->IsDownloading()) {
		// he's already downloading and wants probably only another file
		CPacket* packet = new CPacket(OP_ACCEPTUPLOADREQ, 0, OP_EDONKEYPROT);
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ to ") + client->GetFullIP() );
		client->SendPacket(packet,true);
		return;
	}

	// TODO find better ways to cap the list
	if (m_waitinglist.size() >= (thePrefs::GetQueueSize())) {
		return;
	}

	uint32 tick = GetTickCount();
	client->ClearWaitStartTime();
	// if possible start upload right away
	if (m_waitinglist.empty() && tick - m_nLastStartUpload >= 1000 
		&& m_uploadinglist.size() < GetMaxSlots()
		 && !theApp->listensocket->TooManySockets()) {
		AddUpNextClient(client);
		m_nLastStartUpload = tick;
	} else {
		// add to waiting queue
		m_waitinglist.push_back(CCLIENTREF(client, wxT("CUploadQueue::AddClientToQueue m_waitinglist.push_back")));
		// and sort it to update queue ranks
		SortGetBestClient(true);
		theStats::AddWaitingClient();
		client->ClearAskedCount();
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		//Notify_QlistAddClient(client);
	}
}


bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client)
{
	// Keep track of this client
	theApp->clientlist->AddTrackClient(client);
	
	CClientRefList::iterator it = std::find(m_uploadinglist.begin(),
		m_uploadinglist.end(), CCLIENTREF(client, wxEmptyString));
	
	if (it != m_uploadinglist.end()) {
		m_uploadinglist.erase(it);
		m_allUploadingKnownFile->RemoveUploadingClient(client);
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
		for (CClientRefList::iterator it = m_uploadinglist.begin(); it != m_uploadinglist.end(); ++it) {
			CUpDownClient* cur_client = it->GetClient();
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
	// "Transfer full chunks": drop client after 10 MB upload, or after an hour.
	// (so average UL speed should at least be 2.84 kB/s)
	// We don't track what he is downloading, but if it's all from one chunk he gets it.
	if (client->GetUpStartTimeDelay() > 3600000 	// time: 1h
		|| client->GetSessionUp() > 10485760) {		// data: 10MB
		m_allowKicking = false;		// kick max one client per cycle
		return true;
	}
	
	return false;
}


/*
 * This function removes a file indicated by filehash from suspended_uploads_list.
 */
void CUploadQueue::ResumeUpload( const CMD4Hash& filehash )
{
	suspendedUploadsSet.erase(filehash);
	AddLogLineN(CFormat( _("Resuming uploads of file: %s" ) )
				% filehash.Encode() );
}

/*
 * This function stops upload of a file indicated by filehash.
 *
 * a) teminate == false:
 *    File is suspended while a download completes. Then it is resumed after completion,
 *    so it makes sense to keep the client. Such files are kept in suspendedUploadsSet.
 * b) teminate == true:
 *    File is deleted. Then the client is not added to the waiting list.
 *    Waiting clients are swept out with next run of AddUpNextClient,
 *    because their file is not shared anymore.
 */
uint16 CUploadQueue::SuspendUpload(const CMD4Hash& filehash, bool terminate)
{
	AddLogLineN(CFormat( _("Suspending upload of file: %s" ) )
				% filehash.Encode() );
	uint16 removed = 0;

	//Append the filehash to the list.
	if (!terminate) {
		suspendedUploadsSet.insert(filehash);
	}

	CClientRefList::iterator it = m_uploadinglist.begin();
	while (it != m_uploadinglist.end()) {
		CUpDownClient *potential = it++->GetClient();
		//check if the client is uploading the file we need to suspend
		if (potential->GetUploadFileID() == filehash) {
			// remove the unlucky client from the upload queue
			RemoveFromUploadQueue(potential);
			// if suspend isn't permanent add it to the waiting queue
			if (terminate) {
				potential->SetUploadState(US_NONE);
			} else {
				m_waitinglist.push_back(CCLIENTREF(potential, wxT("CUploadQueue::SuspendUpload")));
				theStats::AddWaitingClient();
				potential->SetUploadState(US_ONUPLOADQUEUE);
				potential->SendRankingInfo();
				Notify_SharedCtrlRefreshClient(potential->ECID(), AVAILABLE_SOURCE);
			}
			removed++;
		}
	}
	return removed;
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client)
{
	CClientRefList::iterator it = m_waitinglist.begin();

	uint16 rank = 1;
	while (it != m_waitinglist.end()) {
		CClientRefList::iterator it1 = it++;
		if (it1->GetClient() == client) {
			RemoveFromWaitingQueue(it1);
			// update ranks of remaining queue
			while (it != m_waitinglist.end()) {
				it->GetClient()->SetUploadQueueWaitingPosition(rank++);
				it++;
			}
			return true;
		}
		rank++;
	}
	return false;
}


void CUploadQueue::RemoveFromWaitingQueue(CClientRefList::iterator pos)
{
	CUpDownClient* todelete = pos->GetClient();
	m_waitinglist.erase(pos);
	theStats::RemoveWaitingClient();
	if( todelete->IsBanned() ) {
		todelete->UnBan();
	}
	//Notify_QlistRemoveClient(todelete);
	todelete->SetUploadState(US_NONE);
	todelete->ClearScore();
	todelete->SetUploadQueueWaitingPosition(0);
}

// File_checked_for_headers
