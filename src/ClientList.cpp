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

#include "ClientList.h"		// Interface declarations.

#include <protocol/Protocols.h>
#include <protocol/ed2k/Constants.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad2/Client2Client/TCP.h>

#include "amule.h"		// Needed for theApp
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFIlter
#include "updownclient.h"	// Needed for CUpDownClient
#include "Preferences.h"	// Needed for thePrefs
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include "GuiEvents.h"		// Needed for Notify_*
#include "Packet.h"

#include <common/Format.h>

#include "kademlia/kademlia/Search.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "kademlia/routing/Contact.h"


/**
 * CDeletedClient Class
 *
 * This class / list is a bit overkill, but currently needed to avoid any
 * exploit possibility. It will keep track of certain clients attributes
 * for 2 hours, while the CUpDownClient object might be deleted already.
 * Currently saves: IP, Port, UserHash.
 */
class CDeletedClient
{
public:
	CDeletedClient(CUpDownClient* pClient)
	{
		m_dwInserted = ::GetTickCount();
		PortAndHash porthash = { pClient->GetUserPort(), pClient->GetCreditsHash()};
		m_ItemsList.push_back(porthash);
	}
	
	struct PortAndHash
	{
		uint16 nPort;
		void* pHash;
	};

	typedef std::list<PortAndHash> PaHList;
	PaHList	m_ItemsList;
	uint32	m_dwInserted;
};



CClientList::CClientList()
	: m_deadSources( true )
{
	m_dwLastBannCleanUp = 0;
	m_dwLastTrackedCleanUp = 0;
	m_dwLastClientCleanUp = 0;
	m_pBuddy = NULL;
	m_nBuddyStatus = Disconnected;
#ifdef __WXDEBUG__
	m_delete_queue_closed = false;
#endif
}


CClientList::~CClientList()
{
	DeleteContents(m_trackedClientsList);

	wxASSERT(m_clientList.empty());
	wxASSERT(m_delete_queue.empty());
}


void CClientList::AddClient( CUpDownClient* toadd )
{
	// Ensure that only new clients can be added to the list
	if ( toadd->GetClientState() == CS_NEW ) {
		// Update the client-state
		toadd->m_clientState = CS_LISTED;
	
 		Notify_ClientCtrlAddClient( toadd );
	
		// We always add the ID/ptr pair, regardles of the actual ID value
		m_clientList.insert( IDMapPair( toadd->GetUserIDHybrid(), toadd ) );

		// We only add the IP if it is valid
		if ( toadd->GetIP() ) {
			m_ipList.insert( IDMapPair( toadd->GetIP(), toadd ) );
		}

		// We only add the hash if it is valid
		if ( toadd->HasValidHash() ) {
			m_hashList.insert( HashMapPair( toadd->GetUserHash(), toadd ) );
		}

		toadd->UpdateStats();
	}
}


void CClientList::AddToDeleteQueue(CUpDownClient* client)
{
	RemoveFromKadList( client );
	RemoveDirectCallback( client );

	// We have to remove the client from the list immediatly, to avoit it getting
	// found by functions such as AttachToAlreadyKnown and GetClientsFromIP, 
	// however, if the client isn't on the clientlist, then it is safe to delete 
	// it right now. Otherwise, push it onto the queue.
	if ( RemoveIDFromList( client ) ) {
		// Also remove the ip and hash entries
		RemoveIPFromList( client );
		RemoveHashFromList( client );
		
		wxASSERT(!m_delete_queue_closed);
		m_delete_queue.push_back( client );
	} else {
		delete client;
	}
}


void CClientList::UpdateClientID( CUpDownClient* client, uint32 newID )
{
	// Sanity check
	if ( ( client->GetClientState() != CS_LISTED ) || ( client->GetUserIDHybrid() == newID ) )
		return;

	// First remove the ID entry
	RemoveIDFromList( client );

	// Add the new entry
	m_clientList.insert( IDMapPair( newID, client ) );
}


void CClientList::UpdateClientIP( CUpDownClient* client, uint32 newIP )
{
	// Sanity check
	if ( ( client->GetClientState() != CS_LISTED ) || ( client->GetIP() == newIP ) )
		return;

	// Remove the old IP entry
	RemoveIPFromList( client );

	if ( newIP ) {
		m_ipList.insert( IDMapPair( newIP, client ) );
	}
}

	
void CClientList::UpdateClientHash( CUpDownClient* client, const CMD4Hash& newHash )
{
	// Sanity check
	if ( ( client->GetClientState() != CS_LISTED ) || ( client->GetUserHash() == newHash ) )
		return;


	// Remove the old entry
	RemoveHashFromList( client );

	// And add the new one if valid
	if ( !newHash.IsEmpty() ) {
		m_hashList.insert( HashMapPair( newHash, client ) );
	}
}


bool CClientList::RemoveIDFromList( CUpDownClient* client )
{
	bool result = false;

	// First remove the ID entry
	std::pair<IDMap::iterator, IDMap::iterator> range = m_clientList.equal_range( client->GetUserIDHybrid() );

	for ( ; range.first != range.second; ++range.first ) {
		if ( client == range.first->second ) {
			/* erase() will invalidate the iterator, but we're not using it anymore 
			    anyway (notice the break;) */
			m_clientList.erase( range.first );
			result = true;

			break;
		}
	}
	
	return result;
}


void CClientList::RemoveIPFromList( CUpDownClient* client )
{
	// Check if we need to look for the IP entry
	if ( !client->GetIP() ) {
		return;
	}
		
	// Remove the IP entry
	std::pair<IDMap::iterator, IDMap::iterator> range = m_ipList.equal_range( client->GetIP() );

	for ( ; range.first != range.second; ++range.first ) {
		if ( client == range.first->second ) {
			/* erase() will invalidate the iterator, but we're not using it anymore 
			    anyway (notice the break;) */
			m_ipList.erase( range.first );
			break;
		}
	}
}

void CClientList::RemoveHashFromList( CUpDownClient* client )
{
	// Nothing to remove
	if ( !client->HasValidHash() ) {
		return;
	}

	// Find all items with the specified hash
	std::pair<HashMap::iterator, HashMap::iterator> range = m_hashList.equal_range( client->GetUserHash() );

	for ( ; range.first != range.second; ++range.first ) {
		if ( client == range.first->second ) {
			/* erase() will invalidate the iterator, but we're not using it anymore 
			    anyway (notice the break;) */
			m_hashList.erase( range.first );
			break;
		}
	}
}


CUpDownClient* CClientList::FindMatchingClient( CUpDownClient* client )
{
	typedef std::pair<IDMap::const_iterator, IDMap::const_iterator> IDMapIteratorPair;
	wxCHECK(client, NULL);
	
	const uint32 userIP = client->GetIP();
	const uint32 userID = client->GetUserIDHybrid();
	const uint16 userPort = client->GetUserPort();
	const uint16 userKadPort = client->GetKadPort();

	
	// LowID clients need a different set of checks
	if (client->HasLowID()) {
		// User is firewalled ... Must do two checks.
		if (userIP && (userPort || userKadPort)) {
			IDMapIteratorPair range = m_ipList.equal_range(userIP);

			for ( ; range.first != range.second; ++range.first ) {
				CUpDownClient* other = range.first->second;
				wxASSERT(userIP == other->GetIP());
						
				if (userPort && (userPort == other->GetUserPort())) {
					return other;
				  } else if (userKadPort && (userKadPort == other->GetKadPort())) {
					return other;
				}
			}
		}

		const uint32 serverIP = client->GetServerIP();
		const uint32 serverPort = client->GetServerPort();		
		if (userID && serverIP && serverPort) {		
			IDMapIteratorPair range = m_clientList.equal_range(userID);

			for (; range.first != range.second; ++range.first) {
				CUpDownClient* other = range.first->second;
				wxASSERT(userID == other->GetUserIDHybrid());

				// For lowid, we also have to check the server
				if (serverIP == other->GetServerIP()) {
					if (serverPort == other->GetServerPort()) {
						return other;
					}
				}
			}
		}
	} else if (userPort || userKadPort) {
		// Check by IP first, then by ID
		struct { const IDMap& map; uint32 value; } toCheck[] = {
			{ m_ipList, userIP }, { m_clientList, userID }
		};
		
		for (size_t i = 0; i < itemsof(toCheck); ++i) {
			if (toCheck[i].value == 0) {
				// We may not have both (or any) of these values.
				continue;
			}
			
			IDMapIteratorPair range = toCheck[i].map.equal_range(toCheck[i].value);
			
			if (userPort) {
				IDMap::const_iterator it = range.first;
				for (; it != range.second; ++it) {
					if (userPort == it->second->GetUserPort()) {
						return it->second;
					}
				}
			}
			
			if (userKadPort) {
				IDMap::const_iterator it = range.first;
				for (; it != range.second; ++it) {
					if (userKadPort == it->second->GetKadPort()) {
						return it->second;
					}
				}
			}
		}
	}
		
		
	// If anything else fails, then we look at hashes
	if ( client->HasValidHash() ) {
		// Find all items with the specified hash
		std::pair<HashMap::iterator, HashMap::iterator> range = m_hashList.equal_range( client->GetUserHash() );

		// Just return the first item if any
		if ( range.first != range.second ) {
			return range.first->second;
		}
	}

	// Nothing found, must be a new client
	return NULL;
}


uint32 CClientList::GetClientCount() const
{
	return m_clientList.size(); 
}


void CClientList::DeleteAll()
{
	m_ipList.clear();
	m_hashList.clear();
	
	while ( !m_clientList.empty() ) {
		IDMap::iterator it = m_clientList.begin();
		
		// Will call the removal of the item on this same class 
		it->second->Disconnected(wxT("Removed while deleting all from ClientList."));
		it->second->Safe_Delete();
	}

	// Clean up the clients now queued for deletion
#ifdef __WXDEBUG__
	m_delete_queue_closed = true;
#endif
	ProcessDeleteQueue();
}


bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientTCPSocket* sender)
{
	CUpDownClient* tocheck = (*client);
	
	CUpDownClient* found_client = FindMatchingClient( tocheck );

	if ( tocheck == found_client ) {
		// We found the same client instance (client may have sent more than one OP_HELLO). do not delete that client!
		return true;
	}
		
	if (found_client != NULL){
		if (sender) {
			if (found_client->GetSocket()) {
				if (found_client->IsConnected() 
					&& (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort() ) )
				{
					// if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
					if (found_client->IsIdentified()){
						AddDebugLogLineM( false, logClient, wxT("Client: ") + tocheck->GetUserName() + wxT("(") + tocheck->GetFullIP() +  wxT("), Banreason: Userhash invalid")); 
						tocheck->Ban();
						return false;
					}
	
					AddDebugLogLineM( false, logClient, wxT("WARNING! Found matching client, to a currently connected client: ") 
															+ tocheck->GetUserName() + wxT("(") +  tocheck->GetFullIP() 
															+ wxT(") and ") + found_client->GetUserName() + wxT("(") +  found_client->GetFullIP() + wxT(")"));
					return false;
				}
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


CUpDownClient* CClientList::FindClientByIP( uint32 clientip, uint16 port )
{
	// Find all items with the specified ip
	std::pair<IDMap::iterator, IDMap::iterator> range = m_ipList.equal_range( clientip );

	for ( ; range.first != range.second; ++range.first ) {
		CUpDownClient* cur_client = range.first->second;
		// Check if it's actually the client we want
		if ( cur_client->GetUserPort() == port ) {
			return cur_client;
		}
	}

	return NULL;
}


CUpDownClient* CClientList::FindClientByIP( uint32 clientip )
{
	// Find all items with the specified ip
	std::pair<IDMap::iterator, IDMap::iterator> range = m_ipList.equal_range( clientip );

	return (range.first != range.second) ? range.first->second : NULL;
}


bool CClientList::IsIPAlreadyKnown(uint32_t ip)
{
	// Find all items with the specified ip
	std::pair<IDMap::iterator, IDMap::iterator> range = m_ipList.equal_range(ip);
	return range.first != range.second;
}


bool CClientList::ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash)
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.find( dwIP );
	
	if ( it != m_trackedClientsList.end() ) {
		CDeletedClient* pResult = it->second;
	
		CDeletedClient::PaHList::iterator it2 = pResult->m_ItemsList.begin();
		for ( ; it2 != pResult->m_ItemsList.end(); ++it2 ) {
			if ( it2->nPort == nPort ) {
				if ( it2->pHash != pNewHash) {
					return false;
				} else {
					break;
				}
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

		CDeletedClient::PaHList::iterator it2 = pResult->m_ItemsList.begin();
		for ( ; it2 != pResult->m_ItemsList.end(); ++it2 ) {
			if ( it2->nPort == toadd->GetUserPort() ) {
				// already tracked, update
				it2->pHash = toadd->GetCreditsHash();
				return;
			}
		}
	
		// New client for that IP, add an entry
		CDeletedClient::PortAndHash porthash = { toadd->GetUserPort(), toadd->GetCreditsHash()};
		pResult->m_ItemsList.push_back(porthash);
	} else {
		m_trackedClientsList[ toadd->GetIP() ] = new CDeletedClient(toadd);
	}
}


uint16 CClientList::GetClientsFromIP(uint32 dwIP)
{
	std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.find( dwIP );
	
	if ( it != m_trackedClientsList.end() ) {
		return it->second->m_ItemsList.size();
	} else {
		return 0;
	}
}


void CClientList::ProcessDeleteQueue()
{
	// Delete pending clients
	while ( !m_delete_queue.empty() ) {
		CUpDownClient* toremove = m_delete_queue.front();
		m_delete_queue.pop_front();
		
		// Doing what RemoveClient used to do. Just to be sure...
		theApp->uploadqueue->RemoveFromUploadQueue( toremove );
		theApp->uploadqueue->RemoveFromWaitingQueue( toremove );
		theApp->downloadqueue->RemoveSource( toremove );
	
		Notify_ClientCtrlRemoveClient( toremove );

		delete toremove;
	}
}


void CClientList::Process()
{
	const uint32 cur_tick = ::GetTickCount();

	ProcessDeleteQueue();

	if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick) {
		m_dwLastBannCleanUp = cur_tick;
		
		ClientMap::iterator it = m_bannedList.begin();
		while ( it != m_bannedList.end() ) {
			if ( it->second + CLIENTBANTIME < cur_tick ) {
				ClientMap::iterator tmp = it++;		

				m_bannedList.erase( tmp );
				theStats::RemoveBannedClient();
			} else {
				++it;
			}
		}
	}

	
	if ( m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick ) {
		m_dwLastTrackedCleanUp = cur_tick;
		
		std::map<uint32, CDeletedClient*>::iterator it = m_trackedClientsList.begin();
		while ( it != m_trackedClientsList.end() ) {
			std::map<uint32, CDeletedClient*>::iterator cur_src = it++;
			
			if ( cur_src->second->m_dwInserted + KEEPTRACK_TIME < cur_tick ) {
				delete cur_src->second;
				m_trackedClientsList.erase( cur_src );
			}
		}
	}
	
	//We need to try to connect to the clients in m_KadList
	//If connected, remove them from the list and send a message back to Kad so we can send a ACK.
	//If we don't connect, we need to remove the client..
	//The sockets timeout should delete this object.

	// buddy is just a flag that is used to make sure we are still connected or connecting to a buddy.
	buddyState buddy = Disconnected;
	
	std::set<CUpDownClient*>::iterator current_it = m_KadSources.begin();
	while (current_it != m_KadSources.end()) {
		CUpDownClient* cur_client = *current_it;
		++current_it; // Won't be used anymore till while loop 
		if( !Kademlia::CKademlia::IsRunning() ) {
			//Clear out this list if we stop running Kad.
			//Setting the Kad state to KS_NONE causes it to be removed in the switch below.
			cur_client->SetKadState(KS_NONE);
		}
		switch (cur_client->GetKadState()) {
			case KS_QUEUED_FWCHECK:
			case KS_QUEUED_FWCHECK_UDP:
				//Another client asked us to try to connect to them to check their firewalled status.
				cur_client->TryToConnect(true);
				break;

			case KS_CONNECTING_FWCHECK:
				//Ignore this state as we are just waiting for results.
				break;

			case KS_FWCHECK_UDP:
			case KS_CONNECTING_FWCHECK_UDP:
				// We want a UDP firewallcheck from this client and are just waiting to get connected to send the request
				break;

			case KS_CONNECTED_FWCHECK:
				//We successfully connected to the client.
				//We now send a ack to let them know.
				if (cur_client->GetKadVersion() >= 7) {
					// The result is now sent per TCP instead of UDP, because this will fail if our intern port is unreachable.
					// But we want the TCP testresult regardless if UDP is firewalled, the new UDP state and test takes care of the rest
					wxASSERT(cur_client->IsConnected());
					//AddDebugLogLineM(false, logClient, wxT("Sent OP_KAD_FWTCPCHECK_ACK"));
					CPacket *packet = new CPacket(OP_KAD_FWTCPCHECK_ACK, 0, OP_EMULEPROT);
					cur_client->SafeSendPacket(packet);
				} else {
					DebugSend(KadFirewalledAckRes, wxUINT32_SWAP_ALWAYS(cur_client->GetIP()), cur_client->GetKadPort());
					Kademlia::CKademlia::GetUDPListener()->SendNullPacket(KADEMLIA_FIREWALLED_ACK_RES, wxUINT32_SWAP_ALWAYS(cur_client->GetIP()), cur_client->GetKadPort(), 0, NULL);
				}
				//We are done with this client. Set Kad status to KS_NONE and it will be removed in the next cycle.
				cur_client->SetKadState(KS_NONE);
				break;

			case KS_INCOMING_BUDDY:
				//A firewalled client wants us to be his buddy.
				//If we already have a buddy, we set Kad state to KS_NONE and it's removed in the next cycle.
				//If not, this client will change to KS_CONNECTED_BUDDY when it connects.
				if( m_nBuddyStatus == Connected ) {
					cur_client->SetKadState(KS_NONE);
				}
				break;

			case KS_QUEUED_BUDDY:
				//We are firewalled and want to request this client to be a buddy.
				//But first we check to make sure we are not already trying another client.
				//If we are not already trying. We try to connect to this client.
				//If we are already connected to a buddy, we set this client to KS_NONE and it's removed next cycle.
				//If we are trying to connect to a buddy, we just ignore as the one we are trying may fail and we can then try this one.
				if( m_nBuddyStatus == Disconnected ) {
					buddy = Connecting;
					m_nBuddyStatus = Connecting;
					cur_client->SetKadState(KS_CONNECTING_BUDDY);
					cur_client->TryToConnect(true);
					Notify_ServerUpdateED2KInfo();
				} else {
					if( m_nBuddyStatus == Connected ) {
						cur_client->SetKadState(KS_NONE);
					}
				}
				break;

			case KS_CONNECTING_BUDDY:
				//We are trying to connect to this client.
				//Although it should NOT happen, we make sure we are not already connected to a buddy.
				//If we are we set to KS_NONE and it's removed next cycle.
				//But if we are not already connected, make sure we set the flag to connecting so we know 
				//things are working correctly.
				if( m_nBuddyStatus == Connected ) {
					cur_client->SetKadState(KS_NONE);
				} else {
					wxASSERT( m_nBuddyStatus == Connecting );
					buddy = Connecting;
				}
				break;

			case KS_CONNECTED_BUDDY:
				//A potential connected buddy client wanting to me in the Kad network
				//We set our flag to connected to make sure things are still working correctly.
				buddy = Connected;

				//If m_nBuddyStatus is not connected already, we set this client as our buddy!
				if( m_nBuddyStatus != Connected ) {
					m_pBuddy = cur_client;
					m_nBuddyStatus = Connected;
					Notify_ServerUpdateED2KInfo();
				}
				if( m_pBuddy == cur_client && theApp->IsFirewalled() && cur_client->SendBuddyPingPong() ) {
					cur_client->SendBuddyPing();
				}
				break;

			default:
				RemoveFromKadList(cur_client);
		}
	}

	//We either never had a buddy, or lost our buddy..
	if( buddy == Disconnected ) {
		if( m_nBuddyStatus != Disconnected || m_pBuddy ) {
			if( Kademlia::CKademlia::IsRunning() && theApp->IsFirewalled() && Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) ) {
				//We are a lowID client and we just lost our buddy.
				//Go ahead and instantly try to find a new buddy.
				Kademlia::CKademlia::GetPrefs()->SetFindBuddy();
			}
			m_pBuddy = NULL;
			m_nBuddyStatus = Disconnected;
			Notify_ServerUpdateED2KInfo();
		}
	}

	if ( Kademlia::CKademlia::IsConnected() ) {
		// we only need a buddy if direct callback is not available
		if(Kademlia::CKademlia::IsFirewalled() && Kademlia::CUDPFirewallTester::IsFirewalledUDP(true)) {
			// TODO: Kad buddies won't work with RequireCrypt, so it is disabled for now, but should (and will)
			// be fixed in later version
			// Update: buddy connections themselves support obfuscation properly since eMule 0.49a and aMule SVN 2008-05-09
			// (this makes it work fine if our buddy uses require crypt), however callback requests don't support it yet so we
			// wouldn't be able to answer callback requests with RequireCrypt, protocolchange intended for eMule 0.49b
			if(m_nBuddyStatus == Disconnected && Kademlia::CKademlia::GetPrefs()->GetFindBuddy() && !thePrefs::IsClientCryptLayerRequired()) {
				AddDebugLogLineM(false, logKadMain, wxT("Starting BuddySearch"));
				//We are a firewalled client with no buddy. We have also waited a set time
				//to try to avoid a false firewalled status.. So lets look for a buddy..
				if (!Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::FINDBUDDY, true, Kademlia::CUInt128(true).XOR(Kademlia::CKademlia::GetPrefs()->GetKadID()))) {
					//This search ID was already going. Most likely reason is that
					//we found and lost our buddy very quickly and the last search hadn't
					//had time to be removed yet. Go ahead and set this to happen again
					//next time around.
					Kademlia::CKademlia::GetPrefs()->SetFindBuddy();
				}
			}
		} else {
			if( m_pBuddy ) {
				//Lets make sure that if we have a buddy, they are firewalled!
				//If they are also not firewalled, then someone must have fixed their firewall or stopped saturating their line.. 
				//We just set the state of this buddy to KS_NONE and things will be cleared up with the next cycle.
				if( !m_pBuddy->HasLowID() ) {
					m_pBuddy->SetKadState(KS_NONE);
				}
			}
		}
	} else {
		if( m_pBuddy ) {
			//We are not connected anymore. Just set this buddy to KS_NONE and things will be cleared out on next cycle.
			m_pBuddy->SetKadState(KS_NONE);
		}
	}
	
	CleanUpClientList();
	ProcessDirectCallbackList();
}


void CClientList::AddBannedClient(uint32 dwIP)
{
	m_bannedList[dwIP] = ::GetTickCount();
	theStats::AddBannedClient();
}


bool CClientList::IsBannedClient(uint32 dwIP)
{
	ClientMap::iterator it = m_bannedList.find( dwIP );
		
	if ( it != m_bannedList.end() ) {
		if ( it->second + CLIENTBANTIME > ::GetTickCount() ) {
			return true;
		} else {
			RemoveBannedClient(dwIP);
		}
	}
	return false; 
}


void CClientList::RemoveBannedClient(uint32 dwIP)
{
	m_bannedList.erase(dwIP);
	theStats::RemoveBannedClient();
}


void CClientList::FilterQueues()
{
	// Filter client list
	for ( IDMap::iterator it = m_ipList.begin(); it != m_ipList.end(); ) {
		IDMap::iterator tmp = it++; // Don't change this to a ++it! 
		
		if ( theApp->ipfilter->IsFiltered(tmp->second->GetConnectIP())) {
			tmp->second->Disconnected(wxT("Filtered by IPFilter"));
			tmp->second->Safe_Delete();
		}
	}
}


CClientList::SourceList	CClientList::GetClientsByHash( const CMD4Hash& hash )
{
	SourceList results;

	// Find all items with the specified hash
	std::pair<HashMap::iterator, HashMap::iterator> range = m_hashList.equal_range( hash );

	for ( ; range.first != range.second; ++range.first)  {
		results.push_back( range.first->second );
	}
	
	return results;
}


CClientList::SourceList	CClientList::GetClientsByIP( unsigned long ip )
{
	SourceList results;

	// Find all items with the specified hash
	std::pair<IDMap::iterator, IDMap::iterator> range = m_ipList.equal_range( ip );

	for ( ; range.first != range.second; range.first++ )  {
		results.push_back( range.first->second );
	}
	
	return results;
}


const CClientList::IDMap& CClientList::GetClientList()
{
	return m_clientList;
}


void CClientList::AddDeadSource(const CUpDownClient* client)
{
	m_deadSources.AddDeadSource( client );
}


bool CClientList::IsDeadSource(const CUpDownClient* client)
{
	return m_deadSources.IsDeadSource( client );
}

bool CClientList::SendChatMessage(uint64 client_id, const wxString& message)
{
	CUpDownClient* client = FindClientByIP(IP_FROM_GUI_ID(client_id), PORT_FROM_GUI_ID(client_id));
	AddDebugLogLineM( false, logClient, wxT("Trying to Send Message.") );
	if (client) {
		AddDebugLogLineM( false, logClient, wxT("Sending.") );
	} else {
		AddDebugLogLineM( true, logClient, 
			CFormat( wxT("No client (GUI_ID %lli [%s:%llu]) found in CClientList::SendChatMessage(). Creating") ) 
				% client_id 
				% Uint32toStringIP(IP_FROM_GUI_ID(client_id))
				% PORT_FROM_GUI_ID(client_id) );
		client = new CUpDownClient(PORT_FROM_GUI_ID(client_id),IP_FROM_GUI_ID(client_id),0,0,NULL, true, true);
		AddClient(client);
	}
	return client->SendChatMessage(message);
}

void CClientList::SetChatState(uint64 client_id, uint8 state) {
	CUpDownClient* client = FindClientByIP(IP_FROM_GUI_ID(client_id), PORT_FROM_GUI_ID(client_id));
	if (client) {
		client->SetChatState(state);
	}
}

/* Kad stuff */

bool CClientList::RequestTCP(Kademlia::CContact* contact, uint8_t connectOptions)
{
	uint32_t nContactIP = wxUINT32_SWAP_ALWAYS(contact->GetIPAddress());
	// don't connect ourself
	if (theApp->GetPublicIP() == nContactIP && thePrefs::GetPort() == contact->GetTCPPort()) {
		return false;
	}

	CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->GetTCPPort());

	if (!pNewClient) {
		//#warning Do we actually have to check friendstate here?
		pNewClient = new CUpDownClient(contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, NULL, false, true);
	} else if (pNewClient->GetKadState() != KS_NONE) {
		return false; // already busy with this client in some way (probably buddy stuff), don't mess with it
	}

	//Add client to the lists to be processed.
	pNewClient->SetKadPort(contact->GetUDPPort());
	pNewClient->SetKadState(KS_QUEUED_FWCHECK);
	if (contact->GetClientID() != 0) {
		uint8_t ID[16];
		contact->GetClientID().ToByteArray(ID);
		pNewClient->SetUserHash(CMD4Hash(ID));
		pNewClient->SetConnectOptions(connectOptions, true, false);
	}
	AddToKadList(pNewClient); // This was a direct adding, but I like to check duplicates
	//This method checks if this is a dup already.
	AddClient(pNewClient);
	return true;
}

void CClientList::RequestBuddy(Kademlia::CContact* contact, uint8_t connectOptions)
{
	uint32_t nContactIP = wxUINT32_SWAP_ALWAYS(contact->GetIPAddress());
	// Don't connect to ourself
	if (theApp->GetPublicIP() == nContactIP && thePrefs::GetPort() == contact->GetTCPPort()) {
		return;
	}

	CUpDownClient* pNewClient = FindClientByIP(nContactIP, contact->GetTCPPort());
	if (!pNewClient) {
		pNewClient = new CUpDownClient(contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, NULL, false, true );
	} else if (pNewClient->GetKadState() != KS_NONE) {
		return; // already busy with this client in some way (probably fw stuff), don't mess with it
	} else if (IsKadFirewallCheckIP(nContactIP)) { // doing a kad firewall check with this IP, abort
		AddDebugLogLineM(false, logKadMain, wxT("Kad TCP firewallcheck / Buddy request collision for IP ") + Uint32toStringIP(nContactIP));
		return;
	}

	//Add client to the lists to be processed.
	pNewClient->SetKadPort(contact->GetUDPPort());
	pNewClient->SetKadState(KS_QUEUED_BUDDY);
	uint8_t ID[16];
	contact->GetClientID().ToByteArray(ID);
	pNewClient->SetUserHash(CMD4Hash(ID));
	pNewClient->SetConnectOptions(connectOptions, true, false);
	AddToKadList(pNewClient);
	//This method checks if this is a dup already.
	AddClient(pNewClient);
}

bool CClientList::IncomingBuddy(Kademlia::CContact* contact, Kademlia::CUInt128* buddyID)
{
	uint32_t nContactIP = wxUINT32_SWAP_ALWAYS(contact->GetIPAddress());
	//If aMule already knows this client, abort this.. It could cause conflicts.
	//Although the odds of this happening is very small, it could still happen.
	if (FindClientByIP(nContactIP, contact->GetTCPPort())) {
		return false;
	} else if (IsKadFirewallCheckIP(nContactIP)) { // doing a kad firewall check with this IP, abort
		AddDebugLogLineM(false, logKadMain, wxT("Kad TCP firewallcheck / Buddy request collision for IP ") + Uint32toStringIP(nContactIP));
		return false;
	}

	if (theApp->GetPublicIP() == nContactIP && thePrefs::GetPort() == contact->GetTCPPort()) {
		return false; // don't connect ourself
	}

	//Add client to the lists to be processed.
	CUpDownClient* pNewClient = new CUpDownClient(contact->GetTCPPort(), contact->GetIPAddress(), 0, 0, NULL, false, true );
	pNewClient->SetKadPort(contact->GetUDPPort());
	pNewClient->SetKadState(KS_INCOMING_BUDDY);
	byte ID[16];
	contact->GetClientID().ToByteArray(ID);
	pNewClient->SetUserHash(CMD4Hash(ID));
	buddyID->ToByteArray(ID);
	pNewClient->SetBuddyID(ID);
	AddToKadList(pNewClient);
	AddClient(pNewClient);
	return true;
}

void CClientList::RemoveFromKadList(CUpDownClient* torem)
{
	wxCHECK_RET(torem, wxT("NULL pointer in RemoveFromKadList"));

	if (m_KadSources.erase(torem)) {
		if(torem == m_pBuddy) {
			m_pBuddy = NULL;
			m_nBuddyStatus = Disconnected;
			Notify_ServerUpdateED2KInfo();
		}
	}
}

void CClientList::AddToKadList(CUpDownClient* toadd)
{
	wxCHECK_RET(toadd, wxT("NULL pointer in AddToKadList"));

	m_KadSources.insert(toadd); // This will take care of duplicates.
}

bool CClientList::DoRequestFirewallCheckUDP(const Kademlia::CContact& contact)
{
	// first make sure we don't know this IP already from somewhere
	if (IsIPAlreadyKnown(wxUINT32_SWAP_ALWAYS(contact.GetIPAddress()))) {
		return false;
	}
	// fine, just create the client object, set the state and wait
	// TODO: We don't know the client's userhash, this means we cannot build an obfuscated connection, which
	// again mean that the whole check won't work on "Require Obfuscation" setting, which is not a huge problem,
	// but certainly not nice. Only somewhat acceptable way to solve this is to use the KadID instead.
	CUpDownClient* pNewClient = new CUpDownClient(contact.GetTCPPort(), contact.GetIPAddress(), 0, 0, NULL, false, true);
	pNewClient->SetKadState(KS_QUEUED_FWCHECK_UDP);
	AddDebugLogLineM(false, logClient, wxT("Selected client for UDP Firewallcheck: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(contact.GetIPAddress())));
	AddToKadList(pNewClient);
	AddClient(pNewClient);
	wxASSERT(!pNewClient->SupportsDirectUDPCallback());
	return true;
}

void CClientList::CleanUpClientList()
{
	// We remove clients which are not needed any more by time
	// this check is also done on CUpDownClient::Disconnected, however it will not catch all
	// cases (if a client changes the state without beeing connected
	//
	// Adding this check directly to every point where any state changes would be more effective,
	// is however not compatible with the current code, because there are points where a client has
	// no state for some code lines and the code is also not prepared that a client object gets
	// invalid while working with it (aka setting a new state)
	// so this way is just the easy and safe one to go (as long as amule is basically single threaded)
	const uint32 cur_tick = ::GetTickCount();
	if (m_dwLastClientCleanUp + CLIENTLIST_CLEANUP_TIME < cur_tick ){
		m_dwLastClientCleanUp = cur_tick;
		uint32 cDeleted = 0;
		IDMap::iterator current_it = m_clientList.begin();
		while (current_it != m_clientList.end()) {
			CUpDownClient* pCurClient = current_it->second;
			++current_it; // Won't be used till while loop again
			// Don't delete sources coming from source seeds for 10 mins,
			// to give them a chance to connect and become a useful source.
			if (pCurClient->GetSourceFrom() == SF_SOURCE_SEEDS && cur_tick - (uint32)theStats::GetStartTime() < MIN2MS(10)) continue;
			if ((pCurClient->GetUploadState() == US_NONE || (pCurClient->GetUploadState() == US_BANNED && !pCurClient->IsBanned()))
				&& pCurClient->GetDownloadState() == DS_NONE
				&& pCurClient->GetChatState() == MS_NONE
				&& pCurClient->GetKadState() == KS_NONE
				&& pCurClient->GetSocket() == NULL)
			{
				cDeleted++;
				pCurClient->Disconnected(wxT("Removed during ClientList cleanup."));
				pCurClient->Safe_Delete(); 
			} else {
				if (!(pCurClient->GetUploadState() == US_NONE || (pCurClient->GetUploadState() == US_BANNED && !pCurClient->IsBanned()))) {
					AddDebugLogLineM(false, logProxy,
						CFormat(wxT("Debug: Not deleted client %x with up state: %i "))
							% (long int)pCurClient % pCurClient->GetUploadState());
				}
				if (!(pCurClient->GetDownloadState() == DS_NONE)) {
					AddDebugLogLineM(false, logProxy, 
						CFormat(wxT("Debug: Not deleted client %x with down state: %i "))
							% (long int)pCurClient % pCurClient->GetDownloadState());
				}
				if (!(pCurClient->GetChatState() == MS_NONE)) {	
					AddDebugLogLineM(false, logProxy, 
						CFormat(wxT("Debug: Not deleted client %x with chat state: %i "))
							% (long int)pCurClient % pCurClient->GetChatState());
				}
				if (!(pCurClient->GetKadState() == KS_NONE)) {
					AddDebugLogLineM(false, logProxy, 
						CFormat(wxT("Debug: Not deleted client %x with kad state: %i ip: %s"))
							% (long int)pCurClient % pCurClient->GetKadState() % pCurClient->GetFullIP());
				}
				if (!(pCurClient->GetSocket() == NULL)) {
					AddDebugLogLineM(false, logProxy, 
						CFormat(wxT("Debug: Not deleted client %x: has socket")) % (long int)pCurClient);
				}	
				AddDebugLogLineM(false, logProxy, 
					CFormat(wxT("Debug: Not deleted client %x with kad version: %i"))
						% (long int)pCurClient % pCurClient->GetKadVersion());				
			}
		}
		AddDebugLogLineM(false, logClient, wxString::Format(wxT("Cleaned ClientList, removed %i not used known clients"), cDeleted));
	}
}

void CClientList::AddKadFirewallRequest(uint32 ip)
{
	uint32 ticks = ::GetTickCount();
	IpAndTicks add = { ip, ticks };
	m_firewallCheckRequests.push_front(add);
	while (!m_firewallCheckRequests.empty()) {
		if (ticks - m_firewallCheckRequests.back().inserted > SEC2MS(180)) {
			m_firewallCheckRequests.pop_back();
		} else {
			break;
		}
	}
}

bool CClientList::IsKadFirewallCheckIP(uint32 ip) const
{
	uint32 ticks = ::GetTickCount();
	for (IpAndTicksList::const_iterator it = m_firewallCheckRequests.begin(); it != m_firewallCheckRequests.end(); ++it) {
		if (it->ip == ip && ticks - it->inserted < SEC2MS(180)) {
			return true;
		}
	}
	return false;
}

void CClientList::AddDirectCallbackClient(CUpDownClient* toAdd)
{
	wxASSERT(toAdd->GetDirectCallbackTimeout() != 0);
	if (toAdd->HasBeenDeleted()) {
		return;
	}
	for (DirectCallbackList::const_iterator it = m_currentDirectCallbacks.begin(); it != m_currentDirectCallbacks.end(); ++it) {
		if (*it == toAdd) {
			wxFAIL; // might happen very rarely on multiple connection tries, could be fixed in the client class, till then it's not much of a problem though
			return;
		}
	}
	m_currentDirectCallbacks.push_back(toAdd);
}

void CClientList::ProcessDirectCallbackList()
{
	// we do check if any direct callbacks have timed out by now
	const uint32_t cur_tick = ::GetTickCount();
	for (DirectCallbackList::iterator it = m_currentDirectCallbacks.begin(); it != m_currentDirectCallbacks.end();) {
		DirectCallbackList::iterator it2 = it++;
		CUpDownClient* curClient = *it2;
		if (curClient->GetDirectCallbackTimeout() < cur_tick) {
			wxASSERT(curClient->GetDirectCallbackTimeout() != 0);
			// TODO LOGREMOVE
			//DebugLog(_T("DirectCallback timed out (%s)"), pCurClient->DbgGetClientInfo());
			m_currentDirectCallbacks.erase(it2);
			if (curClient->Disconnected(wxT("Direct Callback Timeout"))) {
				curClient->Safe_Delete();
			}
		}
	}
}

void CClientList::AddTrackCallbackRequests(uint32_t ip)
{
	uint32_t now = ::GetTickCount();
	IpAndTicks add = { ip, now };
	m_directCallbackRequests.push_front(add);
	while (!m_directCallbackRequests.empty()) {
		if (now - m_directCallbackRequests.back().inserted > MIN2MS(3)) {
			m_directCallbackRequests.pop_back();
		} else {
			break;
		}
	}
}

bool CClientList::AllowCallbackRequest(uint32_t ip) const
{
	uint32_t now = ::GetTickCount();
	for (IpAndTicksList::const_iterator it = m_directCallbackRequests.begin(); it != m_directCallbackRequests.end(); ++it) {
		if (it->ip == ip && now - it->inserted < MIN2MS(3)) {
			return false;
		}
	}
	return true;
}
// File_checked_for_headers
