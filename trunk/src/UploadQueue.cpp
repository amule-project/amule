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

#include <cstring>
#include <cmath>			// Needed for std::exp
#include "types.h"
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
#ifdef __OPENBSD__
       #include <sys/types.h>
#endif /* __OPENBSD__ */
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "UploadQueue.h"	// Interface declarations
#include "ServerList.h"		// Needed for CServerList
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "OScopeCtrl.h"		// Needed for DelayPoints
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "server.h"			// Needed for CServer
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "sockets.h"		// Needed for CServerConnect
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "KnownFile.h"		// Needed for CKnownFile
#include "packets.h"		// Needed for Packet
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "opcodes.h"		// Needed for MAX_PURGEQUEUETIME
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h"	// Needed for GetTickCount
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue(CPreferences* in_prefs){
	app_prefs = in_prefs;

	msPrevProcess = ::GetTickCount();
	kBpsEst = 2.0;
	kBpsUp = 0.0;
	bannedcount = 0;
	successfullupcount = 0;
	failedupcount = 0;
	totaluploadtime = 0;
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

	lastupslotHighID = true; // Uninitialized on eMule
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
			if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ) {
				RemoveFromWaitingQueue(pos2,true);	
				if (!cur_client->socket) {
					if(cur_client->Disconnected("AddUpNextClient - purged")) {
						delete cur_client;
						cur_client = NULL;
					}
				}
				continue;
			} 
			
			suspendlist::iterator it = std::find( suspended_uploads_list.begin(), 
			                                      suspended_uploads_list.end(),
			                                      CMD4Hash(cur_client->GetUploadFileID()) );
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
		RemoveFromWaitingQueue(toadd, true);
		theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
	} else {
		//prevent another potential access of a suspended upload
	
		suspendlist::iterator it = std::find( suspended_uploads_list.begin(), 
		                                      suspended_uploads_list.end(),
		                                      CMD4Hash(directadd->GetUploadFileID()) );		
		if ( it != suspended_uploads_list.end() ) {
			return;
		} else {
			newclient = directadd;
		}
	}
	
	// Thief clients handling [BlackRat]
	if (newclient->thief) {
		// what kind of thief is it ?
		wxString type;
		switch (newclient->leechertype){
		/* Add log line according to leecher type */
			case 1 : {
				type = _("invalid eMule client");
				break;
			}
			case 2 : {
				type = _("suspicious mod string change");
				break;
			}
			case 3 : {
				type = _("known leecher");
				break;
			}
			case 4 : {
				type = _("suspicious hash change");
				break;
			}
			case 5 : {
				type = _("use your own hash");
				break;
			}
			default : {
				type = _("suspicious name change");
			}
		}
		AddLogLineM(false, newclient->GetUserName() + wxT(" [") + newclient->FullUserIP + wxString::Format(_(":%i] using "),newclient->m_nUserPort) + newclient->m_clientVerString + _(" removed : ") + type);		
		// remove client !
		theApp.uploadqueue->RemoveFromUploadQueue(newclient,true);
		return;
	}

	// Anti-leecher mods and irregular clients [BlackRat - LSD]
	if (
	(newclient->GetUserName().Cmp(wxT("celinesexy")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("Chief")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("darkmule")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("dodgethis")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("edevil")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("energyfaker")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("eVortex")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("|eVorte|X|")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("$GAM3R$")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("G@m3r")) == 0) ||		 
	(newclient->GetUserName().Cmp(wxT("Leecha")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("Mison")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("phArAo")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("RAMMSTEIN")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("Reverse")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("[toXic]")) == 0) ||
	(newclient->GetUserName().Cmp(wxT("$WAREZ$")) == 0) ||
	(newclient->m_strModVersion.Cmp(wxT("aldo")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("booster")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("darkmule")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("d-unit")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("DM-")) == 0 ) ||       
	(newclient->m_strModVersion.Cmp(wxT("dodgethis")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("Dragon")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("egomule")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("eVortex")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("father")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("Freeza")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("gt mod")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("imperator")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("LegoLas")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("Max")) == 0 )  ||
	(newclient->m_strModVersion.Cmp(wxT("Mison")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("SpeedLoad")) == 0 ) ||
	(newclient->m_strModVersion.Cmp(wxT("|X|")) == 0 ) ||
	((newclient->m_strModVersion.IsEmpty() == false) && (newclient->GetClientSoft() != SO_EMULE) && (newclient->GetClientSoft() != SO_LXMULE) && (newclient->GetClientSoft() != SO_AMULE) ) ||
 	((!newclient->GetMuleVersion() && (newclient->GetClientSoft()==SO_EMULE || newclient->GetClientSoft()==SO_OLDEMULE)) && (newclient->GetVersion()==60 || !newclient->GetVersion())) ||
 	(!newclient->ExtProtocolAvailable() && newclient->GetClientSoft()==SO_EMULE && (newclient->GetVersion()==60 || !newclient->GetVersion())) ||
 	((newclient->GetVersion()>589) && (newclient->GetSourceExchangeVersion()>0) && (newclient->GetClientSoft()== SO_EDONKEY)))
	{		
		// thief !
		newclient->thief=true;
		theApp.uploadqueue->RemoveFromUploadQueue(newclient,true);
		AddLogLineM(false, newclient->GetUserName() + wxString::Format(_(" [%s:%i] using "), unicode2char(newclient->GetFullIP()),newclient->GetUserPort()) + newclient->GetClientVerString() +  _("removed : leecher, invalid eMule or irregular Donkey"));
		return;
	}	

	if (IsDownloading(newclient)) {
		return;
	}
	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected()) {
		newclient->SetUploadState(US_CONNECTING);
		// newclient->TryToConnect(true);
		if (!newclient->TryToConnect(true)) {
			return;
		}
	} else {
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		newclient->socket->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();
	uploadinglist.AddTail(newclient);
	
	// statistic
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
	if (reqfile) {
		reqfile->statistic.AddAccepted();
	}
	theApp.amuledlg->transferwnd->uploadlistctrl->AddClient(newclient);

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
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		CUpDownClient* cur_client = uploadinglist.GetAt(pos);
		if ( (cur_client->socket) && (!cur_client->socket->IsBusy()) && cur_client->HasBlocks()) {
			clientsrdy++;
		}
	}
	if (!clientsrdy) {
		if ((kBpsEst -= 2.0) < 1.0)
			kBpsEst = 1.0;
		clientsrdy++;
	} else {
		if ((kBpsEst += 2.0) > (float)(app_prefs->GetMaxUpload()))
			kBpsEst = (float)(app_prefs->GetMaxUpload());
	}
	float	kBpsSendPerClient = kBpsEst/clientsrdy;
	uint32	bytesSent = 0;
	POSITION pos1,pos2;
	for (pos1 = uploadinglist.GetHeadPosition();( pos2 = pos1 ) != NULL; ) {
		uploadinglist.GetNext(pos1);
		CUpDownClient* cur_client = uploadinglist.GetAt(pos2);
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

	float kBpsUpPerClient = (float)theApp.glob_prefs->GetSlotAllocation();
	if (theApp.glob_prefs->GetMaxUpload() == UNLIMITED) {
		if ((uint32)uploadinglist.GetCount() < ((uint32)(kBpsUp/kBpsUpPerClient)+2)) {
			return true;
		}
	} else {
		uint16 nMaxSlots = 0;
		if (theApp.glob_prefs->GetMaxUpload() >= 10) {
			nMaxSlots = (uint16)floor((float)theApp.glob_prefs->GetMaxUpload() / kBpsUpPerClient + 0.5);
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
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		if (client == uploadinglist.GetAt(pos)) {
			return pos;
		}
	}
	return 0;
}

void CUploadQueue::UpdateBanCount()
{
	int count=0;
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		CUpDownClient* cur_client= waitinglist.GetAt(pos);
		if(cur_client->IsBanned()) {
			count++;
		}
	}
	SetBanCount(count);
}

void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit)
{

	if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() && !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort()) && client->GetDownloadState() == DS_NONE && !client->IsFriend() && GetWaitingUserCount() > 50) {
		// Well, all that issues finish in the same: don't allow to add to the queue
		return;
	}

	
	if (client->IsBanned()) {
		if (::GetTickCount() - client->GetBanTime() > 18000000) {
			client->UnBan();
		} else {
			return;
		}
	}

	client->AddAskedCount();
	client->SetLastUpRequest();

	if (!bIgnoreTimelimit) {
		client->AddRequestCount(client->GetUploadFileID());
	}

	// check for double
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		CUpDownClient* cur_client= waitinglist.GetAt(pos);
		if (cur_client == client) { // already on queue
			if (client->m_bAddNextConnect && (uploadinglist.GetCount() < theApp.glob_prefs->GetMaxUpload())){
				if (lastupslotHighID) {
					client->m_bAddNextConnect = false;
					RemoveFromWaitingQueue(client, true);
					AddUpNextClient(client);
					lastupslotHighID = false; // LowID alternate
					return;
				}
			}
			// VQB end
			client->SendRankingInfo();
			theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(client);
			return;			
		} else if ( client->Compare(cur_client) ) {
			// another client with same ip or hash
			AddDebugLogLineM(false,wxString::Format(_("Client '%s' and '%s' have the same userhash or IP - removed '%s'"),unicode2char(client->GetUserName()),unicode2char(cur_client->GetUserName()),unicode2char(cur_client->GetUserName())));
			RemoveFromWaitingQueue(pos,true);	
			if (!cur_client->socket) {
				if(cur_client->Disconnected("AddClientToQueue - same userhash 1")) {
					delete cur_client;
				}				
			}
			return;
		}
	}
	// done

	// Add clients server to list.
	if (theApp.glob_prefs->AddServersFromClient()) {
		in_addr host;
		host.s_addr = client->GetServerIP();
		CServer* srv = new CServer(client->GetServerPort(), inet_ntoa(host));
		srv->SetListName(srv->GetAddress());
		
		if (!theApp.amuledlg->serverwnd->serverlistctrl->AddServer(srv, true)) {
			delete srv;
		}
		/*
		} else {
			theApp.amuledlg->AddLogLine(false, _("Added new server: %s:%d"), srv->GetFullIP(), srv->GetPort());
		}
		*/
	}

	// statistic values
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	if (reqfile) {
		reqfile->statistic.AddRequest();
	}
	// TODO find better ways to cap the list
	if ((uint32)waitinglist.GetCount() > (theApp.glob_prefs->GetQueueSize()+bannedcount)) {
		return;
	}
	if (client->IsDownloading()) {
		// he's already downloading and wants probably only another file
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		client->socket->SendPacket(packet,true);
		return;
	}
	if (waitinglist.IsEmpty() && AcceptNewClient()) {
		AddUpNextClient(client);
		m_nLastStartUpload = ::GetTickCount();
	} else {
		waitinglist.AddTail(client);
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		theApp.amuledlg->transferwnd->queuelistctrl->AddClient(client);
		theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
	}
}

bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, bool updatewindow)
{
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		if (client == uploadinglist.GetAt(pos)) {
			if (updatewindow) {
				theApp.amuledlg->transferwnd->uploadlistctrl->RemoveClient(uploadinglist.GetAt(pos));
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
	if (theApp.glob_prefs->TransferFullChunks()) {
		if( client->GetUpStartTimeDelay() > 3600000 ) { // Try to keep the clients from downloading for ever.
			return true;
		}
		// For some reason, some clients can continue to download after a chunk size.
		// Are they redownloading the same chunk over and over????
		if( client->GetSessionUp() > 10485760 ) {
			return true;
		}
	} else {
		for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
			if (client->GetScore(true,true) < waitinglist.GetAt(pos)->GetScore(true,false)) {
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
	if (!IsOnUploadQueue(client)) {
		return 0;
	}
	uint16 rank = 1;
	uint32 myscore = client->GetScore(false);
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		if (waitinglist.GetAt(pos)->GetScore(false) > myscore) {
			rank++;
		}
	}
	return rank;
}

void CUploadQueue::CompUpDatarateOverhead()
{
	m_AvarageUDRO_list.push_back(m_nUpDataRateMSOverhead);
	m_nUpDatarateOverhead = 0;
	m_nUpDataRateMSOverhead = 0;	
	
	if(m_AvarageUDRO_list.size() > 10) {
		if (m_AvarageUDRO_list.size() > 150) {
			m_AvarageUDRO_list.pop_front();
		}

		for ( int i = 0,  size = m_AvarageUDRO_list.size(); i < size; i++ ) {
			m_nUpDatarateOverhead += m_AvarageUDRO_list.at(i);
		}
		
		m_nUpDatarateOverhead = 10*m_nUpDatarateOverhead/m_AvarageUDRO_list.size();
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
	
	printf("%s: Suspending uploads of file.\n", unicode2char(base16hash));
	
	POSITION pos = uploadinglist.GetHeadPosition();
	while(pos) { //while we have a valid position
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		//check if the client is uploading the file we need to suspend
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0) {
			//remove the unlucky client from the upload queue and add to the waiting queue
			RemoveFromUploadQueue(potential, 1);
			printf("%s: Removed user '%s'\n", unicode2char(base16hash) , unicode2char(potential->GetUserName()));
			//this code to add to the waitinglist was copied from the end of AddClientToQueue()
			//the function itself is not used as it could prevent the requeuing of the client
			waitinglist.AddTail(potential);
			potential->SetUploadState(US_ONUPLOADQUEUE);
			potential->SendRankingInfo();
			theApp.amuledlg->transferwnd->queuelistctrl->AddClient(potential);
			theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
			printf("%s: ReQueued user '%s'\n", unicode2char(base16hash), unicode2char(potential->GetUserName()));
		}
	}
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow)
{
	POSITION pos = waitinglist.Find(client);
	if (pos) {
		RemoveFromWaitingQueue(pos,updatewindow);
		if (updatewindow) {
			theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
		}
		return true;
	} else {
		return false;
	}
}

void CUploadQueue::RemoveFromWaitingQueue(POSITION pos, bool updatewindow)
{
	CUpDownClient* todelete = waitinglist.GetAt(pos);
	waitinglist.RemoveAt(pos);
	if( todelete->IsBanned() ) {
		todelete->UnBan();
	}
	//if (updatewindow)
	theApp.amuledlg->transferwnd->queuelistctrl->RemoveClient(todelete);
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

void CUploadQueue::FindSourcesForFileById(CTypedPtrList<CPtrList, CUpDownClient*>* srclist, uchar* filehash)
{
	POSITION pos;
	
	pos = uploadinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0) {
			srclist->AddTail(potential);
		}
	}

	pos = waitinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = waitinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0) {
			srclist->AddTail(potential);
		}
	}
}

// TimerProc is on amule.cpp now
