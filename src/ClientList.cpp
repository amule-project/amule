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

#include "amule.h"			// Needed for theApp
#include "ClientList.h"		// Interface declarations.
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFIlter
#include "amuleDlg.h"
#include "TransferWnd.h"
#include "ClientCredits.h"
#include <wx/intl.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
#include "opcodes.h"

WX_DEFINE_OBJARRAY(ArrayOfPortAndHash);

CClientList::CClientList()
{
	m_dwLastBannCleanUp = 0;
	m_dwLastTrackedCleanUp = 0;
}

CClientList::~CClientList()
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.begin();
	
	for ( ; it != m_trackedClientsList.end(); ++it ){
		delete it->second;
	}

	m_trackedClientsList.clear();
}

// xrmb : statsclientstatus
void CClientList::GetStatistics(uint32 &totalclient, uint32 stats[], clientmap16* WXUNUSED(clientStatus), clientmap32 *clientVersionEDonkey, clientmap32 *clientVersionEDonkeyHybrid, clientmap32 *clientVersionEMule, clientmap32 *clientVersionAMule){
	//if(clientStatus)		clientStatus->RemoveAll();
	totalclient = list.size();
	if(clientVersionEDonkey)	clientVersionEDonkey->clear();
	if(clientVersionEMule)		clientVersionEMule->clear();
	if(clientVersionEDonkeyHybrid)	clientVersionEDonkeyHybrid->clear();
	if(clientVersionAMule)		clientVersionAMule->clear();

	for (int i=0;i<18;i++) stats[i]=0;

	for ( SourceSet::iterator it = list.begin(); it != list.end(); ++it ) { 
		CUpDownClient* cur_client =	(*it);
		
		if (cur_client->HasLowID()) {
			stats[11]++;		
		}
		
		switch (cur_client->GetClientSoft()) {
			case SO_UNKNOWN : 
				stats[0]++;
				break;
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
					if (version == 0xFF || version == 0x66 || version==0x69 || version==0x90 || version==0x33 || version==0x60) {
						continue;					
					}
					(*clientVersionEMule)[cur_client->GetVersion()]++;
				}
				break;
			case SO_CDONKEY : 
				stats[5]++;
				break;
			case SO_LXMULE:
				stats[6]++;
				break;
			case SO_AMULE:
				stats[8]++;
				if(clientVersionAMule) {
					uint8 version = cur_client->GetMuleVersion();
					if (version == 0xFF || version == 0x66 || version==0x69 || version==0x90 || version==0x33 || version==0x60) {
						continue;					
					}
					(*clientVersionAMule)[cur_client->GetVersion()]++;
				}
				break;
			case SO_MLDONKEY:
				stats[3]++;
				break;
			case SO_NEW_MLDONKEY:
			case SO_NEW2_MLDONKEY:
				stats[7]++;
				break;
			case SO_COMPAT_UNK:
				stats[9]++;
				break;
			case SO_LPHANT:
				stats[10]++;
				break;
			case SO_SHAREAZA:
				stats[16]++;
				break;
		}
		
		if (cur_client->Credits() != NULL){
			switch(cur_client->Credits()->GetCurrentIdentState(cur_client->GetIP())){
				case IS_IDENTIFIED:
					stats[12]++;
					break;
				case IS_IDFAILED:
				case IS_IDNEEDED:
				case IS_IDBADGUY:
					stats[13]++;
				default:
					break;
			}
		}
		
		if (cur_client->GetSocket()) {
			stats[17]++;
		}
		
		//if(clientStatus) (*clientStatus)[cur_client->GetDownloadState()]++;
	}
}


void CClientList::AddClient(CUpDownClient* toadd, bool WXUNUSED(bSkipDupTest) )
{
	#warning needs more code
	//theApp.amuledlg->transferwnd->clientlistctrl->AddClient(toadd);
	
	// No need to manually test, as a std::set does not allow duplicates
	list.insert(toadd);
}


void CClientList::AddToDeleteQueue(CUpDownClient* client)
{
	// We have to remove the client from the list immediatly, to avoit it getting
	// found by functions such as AttachToAlreadyKnown and GetClientsFromIP, 
	// however, if the client isn't on the clientlist, then it is safe to delete 
	// it right now. Otherwise, push it onto the queue.
	if ( list.erase( client ) ) {	
		delete_queue.push_back( client );
	} else {
		delete client;
	}
}


void CClientList::DeleteAll()
{
	theApp.uploadqueue->DeleteAll();
	theApp.downloadqueue->DeleteAll();
	while ( !list.empty() ) {
		CUpDownClient* cur_src = *list.begin();
		list.erase( list.begin() );
		delete cur_src;
	}
	
	while ( !delete_queue.empty() ) {
		delete delete_queue.front();
		delete_queue.pop_front();
	}
}


bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender)
{
	CUpDownClient* tocheck = (*client);
	CUpDownClient* found_client = NULL;
	CUpDownClient* found_client2 = NULL;
	for ( SourceSet::iterator it = list.begin(); it != list.end(); ++it ) {
		CUpDownClient* cur_client =	(*it);
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
			if (found_client->GetSocket()){
				if (found_client->IsConnected() 
					&& (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort() ) )
				{
					// if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
					if (found_client->Credits() && found_client->Credits()->GetCurrentIdentState(found_client->GetIP()) == IS_IDENTIFIED){
						AddDebugLogLineM(false, wxString::Format(_("Clients: %s (%s), Banreason: Userhash invalid"), tocheck->GetUserName().c_str(), tocheck->GetFullIP().c_str())); 
						tocheck->Ban();
						return false;
					}
	
					//IDS_CLIENTCOL Warning: Found matching client, to a currently connected client: %s (%s) and %s (%s)
					AddDebugLogLineM(true, wxString::Format(_("WARNING! Found matching client, to a currently connected client: %s (%s) and with %s"), tocheck->GetUserName().c_str(), tocheck->GetFullIP().c_str(), found_client->GetUserName().c_str(), found_client->GetFullIP().c_str()));
					return false;
				}
				found_client->GetSocket()->client = NULL;
				found_client->GetSocket()->Safe_Delete();
			}
			found_client->SetSocket( sender );
			tocheck->SetSocket( NULL );
		}
		*client = 0;
		tocheck->Safe_Delete();
		*client = found_client;
		return true;
	}
	return false;
}


CUpDownClient* CClientList::FindClientByIP(uint32 clientip,uint16 port)
{
	for ( SourceSet::iterator it = list.begin(); it != list.end(); ++it ) {
		CUpDownClient* cur_client =	(*it);
		if (cur_client->GetIP() == clientip && cur_client->GetUserPort() == port)
			return cur_client;
	}
	return 0;
}


// true = everything ok, hash didn't changed
// false = hash changed
bool CClientList::ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash)
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.find( dwIP );
	
	if ( it != m_trackedClientsList.end() ) {
		CDeletedClient* pResult = it->second;
		
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


void CClientList::AddTrackClient(CUpDownClient* toadd)
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.find( toadd->GetIP() );
	
	if ( it != m_trackedClientsList.end() ) {
		CDeletedClient* pResult = it->second;
	
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
	} else {
		m_trackedClientsList[ toadd->GetIP() ] = new CDeletedClient(toadd);
	}
}


uint16 CClientList::GetClientsFromIP(uint32 dwIP)
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.find( dwIP );
	
	if ( it != m_trackedClientsList.end() ) {
		return it->second->m_ItemsList.GetCount();
	} else {
		return 0;
	}
}

void CClientList::Process()
{
	const uint32 cur_tick = ::GetTickCount();

//	if ( !delete_queue.empty() )
//		printf("Deleting %d clients on delete_queue.\n", delete_queue.size());
	
	while ( !delete_queue.empty() ) {
		CUpDownClient* toremove = delete_queue.front();
		delete_queue.pop_front();
		
		// Doing what RemoveClient used to do. Just to be sure...
		theApp.uploadqueue->RemoveFromUploadQueue( toremove );
		theApp.uploadqueue->RemoveFromWaitingQueue( toremove );
		theApp.downloadqueue->RemoveSource( toremove );
	
		#warning needs more code
		//theApp.amuledlg->transferwnd->clientlistctrl->RemoveClient(toremove);
				
		delete toremove;
	}
	
	if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick) {
		m_dwLastBannCleanUp = cur_tick;
		
		std::map<uint32, uint32>::iterator it = m_bannedList.begin();
		while ( it != m_bannedList.end() ) {
			uint32 nKey = it->first;
			uint32 dwBantime = it->second;
		
			++it;
			
			if (dwBantime + CLIENTBANTIME < cur_tick )
				RemoveBannedClient(nKey);
		}
	}

	
	if (m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick ){
		m_dwLastTrackedCleanUp = cur_tick;
		AddDebugLogLineM(false, wxString::Format(_("Cleaning up TrackedClientList, %i clients on List..."), m_trackedClientsList.size()));
		
		std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.begin();
		while ( it != m_trackedClientsList.end() ) {
			std::map<uint32, CDeletedClient*>::iterator cur_src = it++;
			
			if ( cur_src->second->m_dwInserted + KEEPTRACK_TIME < cur_tick ) {
				delete cur_src->second;
				m_trackedClientsList.erase( cur_src );
			}
		}
		AddDebugLogLineM(false, wxString::Format(_("...done, %i clients left on list"), m_trackedClientsList.size()));
	}
}

void CClientList::AddBannedClient(uint32 dwIP){
	m_bannedList[dwIP] = ::GetTickCount();
}

bool CClientList::IsBannedClient(uint32 dwIP)
{
	std::map<uint32, uint32>::iterator it = m_bannedList.find( dwIP );
		
	if ( it != m_bannedList.end() ){
		if ( it->second + CLIENTBANTIME > ::GetTickCount() )
			return true;
		else
			RemoveBannedClient(dwIP);
	}
	return false; 
}

void CClientList::RemoveBannedClient(uint32 dwIP)
{
	m_bannedList.erase(dwIP);
}

void CClientList::FilterQueues() {
	// Filter client list
	SourceSet::iterator it = list.begin();
	while ( it != list.end() ) {
		CUpDownClient* client = *it++;
		if (theApp.ipfilter->IsFiltered(client->GetIP())) {
			client->Safe_Delete();
		}
	}
}

