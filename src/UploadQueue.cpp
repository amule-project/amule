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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "UploadQueue.h"
#endif

#include <cstring>
#include <cmath>			// Needed for std::exp
#include "types.h"
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "UploadQueue.h"	// Interface declarations
#include "ServerList.h"		// Needed for CServerList
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "server.h"			// Needed for CServer
#include "sockets.h"		// Needed for CServerConnect
#include "KnownFile.h"		// Needed for CKnownFile
#include "packets.h"		// Needed for Packet
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "opcodes.h"		// Needed for MAX_PURGEQUEUETIME
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h"	// Needed for GetTickCount
#include "amule.h"		// Needed for theApp
#include "Preferences.h"
#include "ClientList.h"

#ifndef AMULE_DAEMON
	#include "OScopeCtrl.h"		// Needed for DelayPoints
	#include "TransferWnd.h"	// Needed for CTransferWnd	
#endif

#include <numeric>

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue()
{
	msPrevProcess = ::GetTickCount();
	kBpsEst = 2.0;
	kBpsUp = 0.0;
	successfullupcount = 0;
	failedupcount = 0;
	totaluploadtime = 0;
	m_nUpDatarateTotal = 0;
	m_nUpDataRateMSOverhead = 0;
	m_nUpDatarateOverhead = 0;
	m_nUpDataOverheadSourceExchange = 0;
	m_nUpDataOverheadFileRequest = 0;
	m_nUpDataOverheadOther = 0;
	m_nUpDataOverheadServer = 0;
	m_nUpDataOverheadSourceExchangePackets = 0;
	m_nUpDataOverheadFileRequestPackets = 0;
	m_nUpDataOverheadOtherPackets = 0;
	m_nUpDataOverheadServerPackets = 0;
	m_nLastStartUpload = 0;

	lastupslotHighID = true;
}

void CUploadQueue::AddUpNextClient(CUpDownClient* directadd){
	POSITION toadd = 0;
	POSITION toaddlow = 0;
	uint32	bestscore = 0;
	uint32	bestlowscore = 0;
	
	CUpDownClient* newclient;
	// select next client or use given client
	if (!directadd) {
		POSITION pos1, pos2;
		for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
			waitinglist.GetNext(pos1);
			CUpDownClient* cur_client =	waitinglist.GetAt(pos2);
			// clear dead clients
			if ( (::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ) {
				cur_client->ClearWaitStartTime();
				RemoveFromWaitingQueue(pos2);	
				if (!cur_client->GetSocket()) {
					if(cur_client->Disconnected(wxT("AddUpNextClient - purged"))) {
						cur_client->Safe_Delete();
						cur_client = NULL;
					}
				}
				continue;
			} 
			
			suspendlist::iterator it = std::find( suspended_uploads_list.begin(), 
			                                      suspended_uploads_list.end(),
			                                      cur_client->GetUploadFileID() );
			if (cur_client->IsBanned() || it != suspended_uploads_list.end() ) { // Banned client or suspended upload ?
			        continue;
			} 
			// finished clearing
			uint32 cur_score = cur_client->GetScore(true);
			if ( cur_score > bestscore){
				bestscore = cur_score;
				toadd = pos2;
			} else {
				cur_score = cur_client->GetScore(false);
				if ((cur_score > bestlowscore) && !cur_client->m_bAddNextConnect){
					bestlowscore = cur_score;
					toaddlow = pos2;
				}
			}			
		}

		if (bestlowscore > bestscore){
			newclient = waitinglist.GetAt(toaddlow);
			newclient->m_bAddNextConnect = true;
		}		

		if (!toadd) {
			return;
		}
		newclient = waitinglist.GetAt(toadd);
		lastupslotHighID = true; // VQB LowID alternate		
		RemoveFromWaitingQueue(toadd);
		Notify_ShowQueueCount(waitinglist.GetCount());
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
		// newclient->TryToConnect(true);
		if (!newclient->TryToConnect(true)) {
			return;
		}
	} else {
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		newclient->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();
	uploadinglist.AddTail(newclient);
	
	// statistic
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(newclient->GetUploadFileID());
	if (reqfile) {
		reqfile->statistic.AddAccepted();
	}
	Notify_UploadCtrlAddClient(newclient);

}

void CUploadQueue::Process()
{
	if (AcceptNewClient() && waitinglist.GetCount()) {
		m_nLastStartUpload = ::GetTickCount();
		AddUpNextClient();
	}
	if (!uploadinglist.GetCount()) {
		return;
	}
	int16 clientsrdy = 0;
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0; ) {
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);
		if ( (cur_client->GetSocket()) && (!cur_client->GetSocket()->IsBusy()) && cur_client->HasBlocks()) {
			clientsrdy++;
		}
	}
	if (!clientsrdy) {
		if ((kBpsEst -= 1.0) < 1.0)
			kBpsEst = 1.0;
		clientsrdy++;
	} else {
		kBpsEst += 1.0;
		
		if ( thePrefs::GetMaxUpload() != UNLIMITED ) {
			if ( kBpsEst > (float)(thePrefs::GetMaxUpload()))
				kBpsEst = (float)(thePrefs::GetMaxUpload());
		}
	}
	float	kBpsSendPerClient = kBpsEst/clientsrdy;
	uint32	bytesSent = 0;
	for (POSITION pos = uploadinglist.GetHeadPosition(); pos != NULL; ) {
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);
		bytesSent += cur_client->SendBlockData(kBpsSendPerClient);
	}
	
	// smooth current UL rate with a first-order filter
	static uint32	bytesNotCounted;
	uint32	msCur = ::GetTickCount();
	if (msCur==msPrevProcess) {  		// sometimes we get two pulse quickly in a row
		bytesNotCounted += bytesSent;	// avoid divide-by-zero in rate computation then
	} else {
		float	msfDeltaT = (float)(msCur-msPrevProcess);
		float	lambda = std::exp(-msfDeltaT/4000.0);
		kBpsUp = kBpsUp*lambda + (((bytesSent+bytesNotCounted)/1.024)/msfDeltaT)*(1.0-lambda);
		bytesNotCounted = 0;
		msPrevProcess = msCur;
	}
}

bool CUploadQueue::AcceptNewClient()
{
	// check if we can allow a new client to start downloading from us
	if (::GetTickCount() - m_nLastStartUpload < 1000 || uploadinglist.GetCount() >= MAX_UP_CLIENTS_ALLOWED) {
		return false;
	}

	float kBpsUpPerClient = (float)thePrefs::GetSlotAllocation();
	if (thePrefs::GetMaxUpload() == UNLIMITED) {
		if ((uint32)uploadinglist.GetCount() < ((uint32)(kBpsUp/kBpsUpPerClient)+2)) {
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

		if ((uint32)uploadinglist.GetCount() < nMaxSlots) {
			return true;
		}
	}
	return false;
}

CUploadQueue::~CUploadQueue()
{

}

POSITION CUploadQueue::GetWaitingClient(CUpDownClient* client)
{
	return waitinglist.Find(client); 
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP(uint32 dwIP)
{
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		if (dwIP == waitinglist.GetAt(pos)->GetIP()) {
			return waitinglist.GetAt(pos);
		}
	}
	return 0;
}

POSITION CUploadQueue::GetDownloadingClient(CUpDownClient* client)
{
	return uploadinglist.Find( client );
}

void CUploadQueue::AddClientToQueue(CUpDownClient* client)
{
	if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() && !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort()) && client->GetDownloadState() == DS_NONE && !client->IsFriend() && GetWaitingUserCount() > 50) {
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
	for ( ; it != found.end(); it++ ) {
		if ( IsOnUploadQueue( *it ) ) {
			CUpDownClient* cur_client = *it;
			
			if ( cur_client == client ) {
				if ( client->m_bAddNextConnect && ( ( uploadinglist.GetCount() < thePrefs::GetMaxUpload() ) || ( thePrefs::GetMaxUpload() == UNLIMITED ) ) ) {
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
				bool oldID = (cur_client->credits && cur_client->credits->GetCurrentIdentState(cur_client->GetIP()) == IS_IDENTIFIED);
				bool newID = (client->credits && client->credits->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED);
			
				if ( !oldID ) {
					// Cur_client isn't identifed, remove it
					theApp.clientlist->AddTrackClient( cur_client );

					RemoveFromWaitingQueue( cur_client );
					if ( !cur_client->GetSocket() ) {
						if (cur_client->Disconnected( wxT("AddClientToQueue - same userhash") ) ) {
							cur_client->Safe_Delete();
						}
					}
				}

				if ( !newID ) {
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
	if ( client->GetUploadFile() ) {
		client->GetUploadFile()->statistic.AddRequest();
	}
	
	// TODO find better ways to cap the list
	if ( (uint32)waitinglist.GetCount() > (thePrefs::GetQueueSize())) {
		return;
	}
	
	if (client->IsDownloading()) {
		// he's already downloading and wants probably only another file
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		client->SendPacket(packet,true);
		return;
	}

	if (waitinglist.IsEmpty() && AcceptNewClient()) {
		AddUpNextClient(client);
		m_nLastStartUpload = ::GetTickCount();
	} else {
		waitinglist.AddTail(client);
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		Notify_QlistAddClient(client);
		Notify_ShowQueueCount(waitinglist.GetCount());
	}
}

bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, bool updatewindow)
{
	theApp.clientlist->AddTrackClient(client); // Keep track of this client
	POSITION pos = uploadinglist.Find( client );
	if ( pos != NULL ) {
		if (updatewindow) {
			Notify_UploadCtrlRemoveClient(client);
		}
		uploadinglist.RemoveAt(pos);
		if( client->GetTransferedUp() ) {
			successfullupcount++;
			totaluploadtime += client->GetUpStartTimeDelay()/1000;
		} else {
			failedupcount++;
		}
		client->SetUploadState(US_NONE);
		client->ClearUploadBlockRequests();
		return true;
	}
	return false;
}

uint32 CUploadQueue::GetAverageUpTime()
{
	if( successfullupcount ) {
		return totaluploadtime/successfullupcount;
	}
	return 0;
}

bool CUploadQueue::CheckForTimeOver(CUpDownClient* client)
{
	if (thePrefs::TransferFullChunks()) {
		if( client->GetUpStartTimeDelay() > 3600000 ) { // Try to keep the clients from downloading for ever.
			return true;
		}
		// For some reason, some clients can continue to download after a chunk size.
		// Are they redownloading the same chunk over and over????
		if( client->GetSessionUp() > 10485760 ) {
			return true;
		}
	} else {
		for (POSITION pos = waitinglist.GetHeadPosition();pos != 0; ) {
			if (client->GetScore(true,true) < waitinglist.GetNext(pos)->GetScore(true,false)) {
				return true;
			}
		}
	}
	return false;
}

void CUploadQueue::DeleteAll()
{
	waitinglist.RemoveAll();
	uploadinglist.RemoveAll();
}

uint16 CUploadQueue::GetWaitingPosition(CUpDownClient* client)
{
	if ( !IsOnUploadQueue(client) ) {
		return 0;
	}

	uint16 rank = 1;
	uint32 myscore = client->GetScore(false);
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0; ) {
		if (waitinglist.GetNext(pos)->GetScore(false) > myscore) {
			rank++;
		}
	}
	return rank;
}

void CUploadQueue::CompUpDatarateOverhead()
{
	// Adding the new overhead
	m_nUpDatarateTotal += m_nUpDataRateMSOverhead * 10;
	m_AverageUDRO_list.push_back( m_nUpDataRateMSOverhead * 10 );
	
	// Reset the overhead count
	m_nUpDataRateMSOverhead = 0;
	
	// We want at least 11 elements before we will start doing averages
	if ( m_AverageUDRO_list.size() > 10 ) {
		
		// We want 150 elements at most
		if ( m_AverageUDRO_list.size() > 150 ) {
			m_nUpDatarateTotal -= m_AverageUDRO_list.front();
		
			m_AverageUDRO_list.pop_front();
		
			m_nUpDatarateOverhead = m_nUpDatarateTotal / 150.0f;
		} else {
			m_nUpDatarateOverhead = m_nUpDatarateTotal / (double)m_AverageUDRO_list.size();
		}
	} else if ( m_AverageUDRO_list.size() == 10 ) {
		// Create the starting average once we have 10 items
		m_nUpDatarateTotal = std::accumulate( m_AverageUDRO_list.begin(),
		                                      m_AverageUDRO_list.end(), 0 );
	
		m_nUpDatarateOverhead = m_nUpDatarateTotal / 10.0;
		
	} else {
		m_nUpDatarateOverhead = 0;
	}
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

	
	printf("%s: Resuming uploads of file.\n", unicode2char(EncodeBase16(filehash, 16)));
}

/*
 * This function adds a file indicated by filehash to suspended_uploads_list
 */
void CUploadQueue::SuspendUpload( const CMD4Hash& filehash )
{
	//Append the filehash to the list.
	suspended_uploads_list.push_back(filehash);
	wxString base16hash = EncodeBase16(filehash, 16);
	
	POSITION pos = uploadinglist.GetHeadPosition();
	while(pos) { //while we have a valid position
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		//check if the client is uploading the file we need to suspend
		if(potential->GetUploadFileID() == filehash) {
			//remove the unlucky client from the upload queue and add to the waiting queue
			RemoveFromUploadQueue(potential, 1);
			
			waitinglist.AddTail(potential);
			potential->SetUploadState(US_ONUPLOADQUEUE);
			potential->SendRankingInfo();
			Notify_QlistRefreshClient(potential);
			Notify_ShowQueueCount(waitinglist.GetCount());
		}
	}
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow)
{
	POSITION pos = waitinglist.Find(client);
	if (pos) {
		RemoveFromWaitingQueue(pos);
		if (updatewindow) {
			Notify_ShowQueueCount(waitinglist.GetCount());
		}
		return true;
	} else {
		return false;
	}
}

void CUploadQueue::RemoveFromWaitingQueue(POSITION pos)
{
	CUpDownClient* todelete = waitinglist.GetAt(pos);
	waitinglist.RemoveAt(pos);
	if( todelete->IsBanned() ) {
		todelete->UnBan();
	}
	Notify_QlistRemoveClient(todelete);
	todelete->SetUploadState(US_NONE);
}

CUpDownClient* CUploadQueue::GetNextClient(CUpDownClient* lastclient)
{
	if (waitinglist.IsEmpty()) {
		return 0;
	}
	if (!lastclient) {
		return waitinglist.GetHead();
	}
	POSITION pos = waitinglist.Find(lastclient);
	if (!pos) {
		return waitinglist.GetHead();
	}
	waitinglist.GetNext(pos);
	if (!pos) {
		return NULL;
	} else {
		return waitinglist.GetAt(pos);
	}
}
