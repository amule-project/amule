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

#include "CamuleAppBase.h"	// Needed for theApp
#include "ClientList.h"		// Interface declarations.
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "amuleDlg.h"
#include "TransferWnd.h"
#include "ClientCredits.h"
#include <wx/intl.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
#include "opcodes.h"


WX_DEFINE_OBJARRAY(ArrayOfPortAndHash);

CClientList::CClientList(){
	m_dwLastBannCleanUp = 0;
	m_dwLastTrackedCleanUp = 0;
	m_bannedList.InitHashTable(331);
	m_trackedClientsList.InitHashTable(2011);
}

CClientList::~CClientList(){
	POSITION pos = m_trackedClientsList.GetStartPosition();
	uint32 nKey;
	CDeletedClient* pResult;
	while (pos != NULL){
		m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
		m_trackedClientsList.RemoveKey(nKey);
		delete pResult;
	}	
}

// xrmb : statsclientstatus
void CClientList::GetStatistics(uint32 &totalclient, int stats[], CMap<uint8, uint8, uint32, uint32> *clientStatus, CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkey, CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkeyHybrid, CMap<uint8, uint8, uint32, uint32> *clientVersionEMule){
	//if(clientStatus)		clientStatus->RemoveAll();
	totalclient = list.GetCount();
	if(clientVersionEDonkey)	clientVersionEDonkey->RemoveAll();
	if(clientVersionEMule)		clientVersionEMule->RemoveAll();
	POSITION pos1, pos2;

	for (int i=0;i<9;i++) stats[i]=0;

	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		
		switch (cur_client->GetClientSoft()) {
			case SO_UNKNOWN : stats[0]++;break;
			case SO_EDONKEY : 
				stats[1]++;
				if(clientVersionEDonkey)
					(*clientVersionEDonkey)[cur_client->GetVersion()]++;
				break;
			case SO_EDONKEYHYBRID : 
				stats[4]++;
				if(clientVersionEDonkeyHybrid)
					(*clientVersionEDonkeyHybrid)[cur_client->GetVersion()]++;
				break;
			case SO_EMULE   :
			case SO_OLDEMULE:
				stats[2]++;
				if(clientVersionEMule) {
					uint8 version = cur_client->GetMuleVersion();
					(*clientVersionEMule)[version]++;
				}
				break;
			case SO_CDONKEY : //Didn't get much time to test this "aMule Compatable" feature.
				stats[5]++;
				break;
			case SO_LXMULE:
				stats[6]++;
				break;
			case SO_AMULE:
				stats[8]++;
				break;
			case SO_MLDONKEY:
				stats[3]++;
				break;
			case SO_NEW_MLDONKEY:
				stats[7]++;
				break;
		}

		//if(clientStatus) (*clientStatus)[cur_client->GetDownloadState()]++;
	}
}


void CClientList::AddClient(CUpDownClient* toadd,bool bSkipDupTest){
	if ( !bSkipDupTest){
		if(list.Find(toadd))
			return;
	}
	#warning needs more code
	//theApp.amuledlg->transferwnd->clientlistctrl->AddClient(toadd);
	list.AddTail(toadd);
}

void CClientList::RemoveClient(CUpDownClient* toremove){
	POSITION pos = list.Find(toremove);
	if (pos){
		//just to be sure...
		theApp.uploadqueue->RemoveFromUploadQueue(toremove);
		theApp.uploadqueue->RemoveFromWaitingQueue(toremove);
		theApp.downloadqueue->RemoveSource(toremove);
		#warning needs more code
		//theApp.amuledlg->transferwnd->clientlistctrl->RemoveClient(toremove);
		list.RemoveAt(pos);
	}
}

void CClientList::DeleteAll(){
	theApp.uploadqueue->DeleteAll();
	theApp.downloadqueue->DeleteAll();
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		list.RemoveAt(pos2);
		delete cur_client; // recursiv: this will call RemoveClient
	}
}


bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender){
	POSITION pos1, pos2;
	CUpDownClient* tocheck = (*client);
	CUpDownClient* found_client = NULL;
	CUpDownClient* found_client2 = NULL;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){	//
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (tocheck->Compare(cur_client,false)){ //matching userhash
			found_client2 = cur_client;
		}
		if (tocheck->Compare(cur_client,true)){	 //matching IP
			found_client = cur_client;
			break;
		}
	}
	if (found_client == NULL)
		found_client = found_client2;

	if (found_client != NULL){
		if (tocheck == found_client){
			//we found the same client instance (client may have sent more than one OP_HELLO). do not delete that client!
			return true;
		}
		if (sender){
			if (found_client->socket){
				if (found_client->socket->IsConnected() 
					&& (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort() ) )
				{
					// if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
					if (found_client->Credits() && found_client->Credits()->GetCurrentIdentState(found_client->GetIP()) == IS_IDENTIFIED){
						theApp.amuledlg->AddDebugLogLine(false, "Clients: %s (%s), Banreason: Userhash invalid", tocheck->GetUserName(),tocheck->GetFullIP()); 
						tocheck->Ban();
						return false;
					}
	
					//IDS_CLIENTCOL Warning: Found matching client, to a currently connected client: %s (%s) and %s (%s)
					theApp.amuledlg->AddDebugLogLine(true,CString(_("WARNING! Found matching client, to a currently connected client: %s (%s) and with %s")),tocheck->GetUserName(),tocheck->GetFullIP(),found_client->GetUserName(),found_client->GetFullIP());
					return false;
				}
				found_client->socket->client = 0;
				found_client->socket->Safe_Delete();
			}
			found_client->socket = sender;
			tocheck->socket = 0;
		}
		*client = 0;
		delete tocheck;
		*client = found_client;
		return true;
	}
	return false;
}

CUpDownClient* CClientList::FindClientByIP(uint32 clientip,uint16 port){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (cur_client->GetIP() == clientip && cur_client->GetUserPort() == port)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByUserHash(uchar* clienthash){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (memcmp(cur_client->GetUserHash() ,clienthash,16)  )
				return cur_client;
	}
	return 0;
}


void CClientList::Debug_SocketDeleted(CClientReqSocket* deleted){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
	}
}

bool CClientList::Debug_IsValidClient(CUpDownClient* tocheck){
	return list.Find(tocheck);
}

// true = everything ok, hash didn't changed
// false = hash changed
bool CClientList::ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(dwIP, pResult)){
		for (unsigned int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == nPort){
				if (pResult->m_ItemsList[i].pHash != pNewHash)
					return false;
				else
					break;
			}
		}
	}
	return true;
}

void CClientList::AddTrackClient(CUpDownClient* toadd){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(toadd->GetIP(), pResult)){
		pResult->m_dwInserted = ::GetTickCount();
		for (unsigned int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == toadd->GetUserPort()){
				// already tracked, update
				pResult->m_ItemsList[i].pHash = toadd->Credits();
				return;
			}
		}
		PORTANDHASH porthash = { toadd->GetUserPort(), toadd->Credits()};
		pResult->m_ItemsList.Add(porthash);
	}
	else{
		m_trackedClientsList.SetAt(toadd->GetIP(), new CDeletedClient(toadd));
	}
}

uint16 CClientList::GetClientsFromIP(uint32 dwIP){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(dwIP, pResult)){
		return pResult->m_ItemsList.GetCount();
	}
	return 0;
}

void CClientList::Process(){
	const uint32 cur_tick = ::GetTickCount();
	if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick){
		m_dwLastBannCleanUp = cur_tick;
		
		POSITION pos = m_bannedList.GetStartPosition();
		uint32 nKey;
		uint32 dwBantime;
		while (pos != NULL){
			m_bannedList.GetNextAssoc( pos, nKey, dwBantime );
			if (dwBantime + CLIENTBANTIME < cur_tick )
				RemoveBannedClient(nKey);
		}
	}

	
	if (m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick ){
		m_dwLastTrackedCleanUp = cur_tick;
		theApp.amuledlg->AddDebugLogLine(false, "Cleaning up TrackedClientList, %i clients on List...", m_trackedClientsList.GetCount());
		POSITION pos = m_trackedClientsList.GetStartPosition();
		uint32 nKey;
		CDeletedClient* pResult;
		while (pos != NULL){
			m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
			if (pResult->m_dwInserted + KEEPTRACK_TIME < cur_tick ){
				m_trackedClientsList.RemoveKey(nKey);
				delete pResult;
			}
		}
		theApp.amuledlg->AddDebugLogLine(false, "...done, %i clients left on list", m_trackedClientsList.GetCount());
	}
}

void CClientList::AddBannedClient(uint32 dwIP){
	m_bannedList.SetAt(dwIP, ::GetTickCount());
}

bool CClientList::IsBannedClient(uint32 dwIP){
	uint32 dwBantime = 0;
	if (m_bannedList.Lookup(dwIP, dwBantime)){
		if (dwBantime + CLIENTBANTIME > ::GetTickCount() )
			return true;
		else
			RemoveBannedClient(dwIP);
	}
	return false; 
}

void CClientList::RemoveBannedClient(uint32 dwIP){
	m_bannedList.RemoveKey(dwIP);
}
