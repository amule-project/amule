//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
// Copyright (c) 2004-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "Search.h"

#include <protocol/Protocols.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <tags/FileTags.h>

#include "Defines.h"
#include "UDPFirewallTester.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/KadClientSearcher.h"
#include "../../amule.h"
#include "../../SharedFileList.h"
#include "../../DownloadQueue.h"
#include "../../PartFile.h"
#include "../../SearchList.h"
#include "../../MemFile.h"
#include "../../ClientList.h"
#include "../../updownclient.h"
#include "../../Logger.h"
#include "../../Preferences.h"
#include "../../GuiEvents.h"

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CSearch::CSearch()
{
	m_created = time(NULL);
	m_type = (uint32_t)-1;
	m_answers = 0;
	m_totalRequestAnswers = 0;
	m_searchID = (uint32_t)-1;
	m_stopping = false;
	m_totalLoad = 0;
	m_totalLoadResponses = 0;
	m_lastResponse = m_created;
	m_searchTermsData = NULL;
	m_searchTermsDataSize = 0;
	m_nodeSpecialSearchRequester = NULL;
	m_closestDistantFound = 0;
	m_requestedMoreNodesContact = NULL;
}

CSearch::~CSearch()
{
	// remember the closest node we found and tried to contact (if any) during this search
	// for statistical caluclations, but only if its a certain type
	switch (m_type) {
		case NODECOMPLETE:
		case FILE:
		case KEYWORD:
		case NOTES:
		case STOREFILE:
		case STOREKEYWORD:
		case STORENOTES:
		case FINDSOURCE: // maybe also exclude
			if (m_closestDistantFound != 0) {
				CKademlia::StatsAddClosestDistance(m_closestDistantFound);
			}
			break;
		default: // NODE, NODESPECIAL, NODEFWCHECKUDP, FINDBUDDY
			break;
	}

	if (m_nodeSpecialSearchRequester != NULL) {
		// inform requester that our search failed
		m_nodeSpecialSearchRequester->KadSearchIPByNodeIDResult(KCSR_NOTFOUND, 0, 0);
	}

	// Check if a source search is currently being done.
	CPartFile* temp = theApp->downloadqueue->GetFileByKadFileSearchID(GetSearchID());

	// Reset the searchID if a source search is currently being done.
	if (temp) {
		temp->SetKadFileSearchID(0);
	}

	// Decrease the use count for any contacts that are in our contact list.
	for (ContactMap::iterator it = m_inUse.begin(); it != m_inUse.end(); ++it) {
		it->second->DecUse();
	}

	// Delete any temp contacts...
	for (ContactList::const_iterator it = m_delete.begin(); it != m_delete.end(); ++it) {
		if (!(*it)->InUse()) {
			delete *it;
		}
	}

	// Check if this search was containing an overload node and adjust time of next time we use that node.
	if (CKademlia::IsRunning() && GetNodeLoad() > 20) {
		switch(GetSearchTypes()) {
			case CSearch::STOREKEYWORD:
				Kademlia::CKademlia::GetIndexed()->AddLoad(GetTarget(), ((uint32_t)(DAY2S(7)*((double)GetNodeLoad()/100.0))+(uint32_t)time(NULL)));
				break;
		}
	}

	if (m_searchTermsData) {
		delete [] m_searchTermsData;
	}

	switch (m_type) {
		case KEYWORD:
			Notify_KadSearchEnd(m_searchID);
			break;
	}
}

void CSearch::Go()
{
	// Start with a lot of possible contacts, this is a fallback in case search stalls due to dead contacts
	if (m_possible.empty()) {
		CUInt128 distance(CKademlia::GetPrefs()->GetKadID() ^ m_target);
		CKademlia::GetRoutingZone()->GetClosestTo(3, m_target, distance, 50, &m_possible, true, true);
	}

	if (!m_possible.empty()) {
		//Lets keep our contact list entries in mind to dec the inUse flag.
		for (ContactMap::iterator it = m_possible.begin(); it != m_possible.end(); ++it) {
			m_inUse[it->first] = it->second;
		}

		wxASSERT(m_possible.size() == m_inUse.size());

		// Take top ALPHA_QUERY to start search with.
		int count = m_type == NODE ? 1 : min(ALPHA_QUERY, (int)m_possible.size());

		// Send initial packets to start the search.
		ContactMap::iterator it = m_possible.begin();
		for (int i = 0; i < count; i++) {
			CContact *c = it->second;
			// Move to tried
			m_tried[it->first] = c;
			// Send the KadID so other side can check if I think it has the right KadID.
			// Send request
			SendFindValue(c);
			++it;
		}
	}
}

//If we allow about a 15 sec delay before deleting, we won't miss a lot of delayed returning packets.
void CSearch::PrepareToStop() throw()
{
	// Check if already stopping.
	if (m_stopping) {
		return;
	}

	// Set basetime by search type.
	uint32_t baseTime = 0;
	switch (m_type) {
		case NODE:
		case NODECOMPLETE:
		case NODESPECIAL:
		case NODEFWCHECKUDP:
			baseTime = SEARCHNODE_LIFETIME;
			break;
		case FILE:
			baseTime = SEARCHFILE_LIFETIME;
			break;
		case KEYWORD:
			baseTime = SEARCHKEYWORD_LIFETIME;
			break;
		case NOTES:
			baseTime = SEARCHNOTES_LIFETIME;
			break;
		case STOREFILE:
			baseTime = SEARCHSTOREFILE_LIFETIME;
			break;
		case STOREKEYWORD:
			baseTime = SEARCHSTOREKEYWORD_LIFETIME;
			break;
		case STORENOTES:
			baseTime = SEARCHSTORENOTES_LIFETIME;
			break;
		case FINDBUDDY:
			baseTime = SEARCHFINDBUDDY_LIFETIME;
			break;
		case FINDSOURCE:
			baseTime = SEARCHFINDSOURCE_LIFETIME;
			break;
		default:
			baseTime = SEARCH_LIFETIME;
	}

	// Adjust created time so that search will delete within 15 seconds.
	// This gives late results time to be processed.
	m_created = time(NULL) - baseTime + SEC(15);
	m_stopping = true;	
}

void CSearch::JumpStart()
{
	// If we had a response within the last 3 seconds, no need to jumpstart the search.
	if ((time_t)(m_lastResponse + SEC(3)) > time(NULL)) {
		return;
	}

	// If we ran out of contacts, stop search.
	if (m_possible.empty()) {
		PrepareToStop();
		return;
	}

	// Is this a find lookup and are the best two (=KADEMLIA_FIND_VALUE) nodes dead/unreachable?
	// In this case try to discover more close nodes before using our other results
	// The reason for this is that we may not have found the closest node alive due to results being limited to 2 contacts,
	// which could very well have been the duplicates of our dead closest nodes
	bool lookupCloserNodes = false;
	if (m_requestedMoreNodesContact == NULL && GetRequestContactCount() == KADEMLIA_FIND_VALUE && m_tried.size() >= 3 * KADEMLIA_FIND_VALUE) {
		ContactMap::const_iterator it = m_tried.begin();
		lookupCloserNodes = true;
		for (unsigned i = 0; i < KADEMLIA_FIND_VALUE; i++) {
			if (m_responded.count(it->first) > 0) {
				lookupCloserNodes = false;
				break;
			}
			++it;
		}
		if (lookupCloserNodes) {
			while (it != m_tried.end()) {
				if (m_responded.count(it->first) > 0) {
					AddDebugLogLineN(logKadSearch, CFormat(wxT("Best %d nodes for lookup (id=%x) were unreachable or dead, reasking closest for more")) % KADEMLIA_FIND_VALUE % GetSearchID());
					SendFindValue(it->second, true);
					return;
				}
				++it;
			}
		}
	}

	// Search for contacts that can be used to jumpstart a stalled search.
	while (!m_possible.empty()) {
		// Get a contact closest to our target.
		CContact *c = m_possible.begin()->second;
	
		// Have we already tried to contact this node.
		if (m_tried.count(m_possible.begin()->first) > 0) {
			// Did we get a response from this node, if so, try to store or get info.
			if (m_responded.count(m_possible.begin()->first) > 0) {
				StorePacket();
			}
			// Remove from possible list.
			m_possible.erase(m_possible.begin());
		} else {
			// Add to tried list.
			m_tried[m_possible.begin()->first] = c;
			// Send the KadID so other side can check if I think it has the right KadID.
			// Send request
			SendFindValue(c);
			break;
		}
	}
	
}

void CSearch::ProcessResponse(uint32_t fromIP, uint16_t fromPort, ContactList *results)
{
	AddDebugLogLineN(logKadSearch, wxT("Processing search response from ") + KadIPPortToString(fromIP, fromPort));

	ContactList::iterator response;
	// Remember the contacts to be deleted when finished
	for (response = results->begin(); response != results->end(); ++response) {
		m_delete.push_back(*response);
	}

	m_lastResponse = time(NULL);

	// Find contact that is responding.
	CUInt128 fromDistance(0u);
	CContact *fromContact = NULL;
	for (ContactMap::const_iterator it = m_tried.begin(); it != m_tried.end(); ++it) {
		CContact *tmpContact = it->second;
		if ((tmpContact->GetIPAddress() == fromIP) && (tmpContact->GetUDPPort() == fromPort)) {
			fromDistance = it->first;
			fromContact = tmpContact;
			break;
		}
	}

	// Make sure the node is not sending more results than we requested, which is not only a protocol violation
	// but most likely a malicious answer
	if (results->size() > GetRequestContactCount() && !(m_requestedMoreNodesContact == fromContact && results->size() <= KADEMLIA_FIND_VALUE_MORE)) {
		AddDebugLogLineN(logKadSearch, wxT("Node ") + KadIPToString(fromIP) + wxT(" sent more contacts than requested on a routing query, ignoring response"));
		return;
	}

	if (m_type == NODEFWCHECKUDP) {
		m_answers++;
		return;
	}

	// Not interested in responses for FIND_NODE, will be added to contacts by udp listener
	if (m_type == NODE) {
		AddDebugLogLineN(logKadSearch, wxT("Node type search result, discarding."));
		// Note that we got an answer.
		m_answers++;
		// We clear the possible list to force the search to stop.
		m_possible.clear();
		return;
	}

	if (fromContact != NULL) {
		bool providedCloserContacts = false;
		std::map<uint32_t, unsigned> receivedIPs;
		std::map<uint32_t, unsigned> receivedSubnets;
		// A node is not allowed to answer with contacts to itself
		receivedIPs[fromIP] = 1;
		receivedSubnets[fromIP & 0xFFFFFF00] = 1;
		// Loop through their responses
		for (ContactList::iterator it = results->begin(); it != results->end(); ++it) {
			// Get next result
			CContact *c = *it;
			// calc distance this result is to the target
			CUInt128 distance(c->GetClientID() ^ m_target);

			if (distance < fromDistance) {
				providedCloserContacts = true;
			}

			// Ignore this contact if already known or tried it.
			if (m_possible.count(distance) > 0) {
				AddDebugLogLineN(logKadSearch, wxT("Search result from already known client: ignore"));
				continue;
			}
			if (m_tried.count(distance) > 0) {
				AddDebugLogLineN(logKadSearch, wxT("Search result from already tried client: ignore"));
				continue;
			}

			// We only accept unique IPs in the answer, having multiple IDs pointing to one IP in the routing tables
			// is no longer allowed since eMule0.49a, aMule-2.2.1 anyway
			if (receivedIPs.count(c->GetIPAddress()) > 0) {
				AddDebugLogLineN(logKadSearch, wxT("Multiple KadIDs pointing to same IP (") + KadIPToString(c->GetIPAddress()) + wxT(") in Kad2Res answer - ignored, sent by ") + KadIPToString(fromContact->GetIPAddress()));
				continue;
			} else {
				receivedIPs[c->GetIPAddress()] = 1;
			}
				// and no more than 2 IPs from the same /24 subnet
			if (receivedSubnets.count(c->GetIPAddress() & 0xFFFFFF00) > 0 && !::IsLanIP(wxUINT32_SWAP_ALWAYS(c->GetIPAddress()))) {
				wxASSERT(receivedSubnets.find(c->GetIPAddress() & 0xFFFFFF00) != receivedSubnets.end());
				int subnetCount = receivedSubnets.find(c->GetIPAddress() & 0xFFFFFF00)->second;
				if (subnetCount >= 2) {
					AddDebugLogLineN(logKadSearch, wxT("More than 2 KadIDs pointing to same subnet (") + KadIPToString(c->GetIPAddress() & 0xFFFFFF00) + wxT("/24) in Kad2Res answer - ignored, sent by ") + KadIPToString(fromContact->GetIPAddress()));
					continue;
				} else {
					receivedSubnets[c->GetIPAddress() & 0xFFFFFF00] = subnetCount + 1;
				}
			} else {
				receivedSubnets[c->GetIPAddress() & 0xFFFFFF00] = 1;
			}

			// Add to possible
			m_possible[distance] = c;

			// Verify if the result is closer to the target than the one we just checked.
			if (distance < fromDistance) {
				// The top ALPHA_QUERY of results are used to determine if we send a request.
				bool top = false;
				if (m_best.size() < ALPHA_QUERY) {
					top = true;
					m_best[distance] = c;
				} else {
					ContactMap::iterator worst = m_best.end();
					--worst;
					if (distance < worst->first) {
						// Prevent having more than ALPHA_QUERY within the Best list.
						m_best.erase(worst);
						m_best[distance] = c;
						top = true;
					}
				}

				if (top) {
					// We determined this contact is a candidate for a request.
					// Add to tried
					m_tried[distance] = c;
					// Send the KadID so other side can check if I think it has the right KadID.
					// Send request
					SendFindValue(c);
				}
			}
		}

		// Add to list of people who responded.
		m_responded[fromDistance] = providedCloserContacts;

		// Complete node search, just increment the counter.
		if (m_type == NODECOMPLETE || m_type == NODESPECIAL) {
			AddDebugLogLineN(logKadSearch, wxString(wxT("Search result type: Node")) + (m_type == NODECOMPLETE ? wxT("Complete") : wxT("Special")));
			m_answers++;
		}
	}
}

void CSearch::StorePacket()
{
	wxASSERT(!m_possible.empty());

	// This method is currently only called by jumpstart so only use best possible.
	ContactMap::const_iterator possible = m_possible.begin();
	CUInt128 fromDistance(possible->first);
	CContact *from = possible->second;

	if (fromDistance < m_closestDistantFound || m_closestDistantFound == 0) {
		m_closestDistantFound = fromDistance;
	}

	// Make sure this is a valid node to store.
	if (fromDistance.Get32BitChunk(0) > SEARCHTOLERANCE && !::IsLanIP(wxUINT32_SWAP_ALWAYS(from->GetIPAddress()))) {
		return;
	}

	// What kind of search are we doing?
	switch (m_type) {
		case FILE: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: File"));
			CMemFile searchTerms;
			searchTerms.WriteUInt128(m_target);
			if (from->GetVersion() >= 3) {
				// Find file we are storing info about.
				uint8_t fileid[16];
				m_target.ToByteArray(fileid);
				CKnownFile *file = theApp->downloadqueue->GetFileByID(CMD4Hash(fileid));
				if (file) {
					// Start position range (0x0 to 0x7FFF)
					searchTerms.WriteUInt16(0);
					searchTerms.WriteUInt64(file->GetFileSize());
					DebugSend(Kad2SearchSourceReq, from->GetIPAddress(), from->GetUDPPort());
					if (from->GetVersion() >= 6) {
						CUInt128 clientID = from->GetClientID();
						CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_SOURCE_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
					} else {
						CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_SOURCE_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
						wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
					}
				} else {
					PrepareToStop();
					break;
				}
			} else {
				searchTerms.WriteUInt8(1);
				DebugSendF(wxT("KadSearchReq(File)"), from->GetIPAddress(), from->GetUDPPort());
				CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
			}
			m_totalRequestAnswers++;
			break;
		}
		case KEYWORD: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: Keyword"));
			CMemFile searchTerms;
			searchTerms.WriteUInt128(m_target);
			if (from->GetVersion() >= 3) {
				if (m_searchTermsDataSize == 0) {
					// Start position range (0x0 to 0x7FFF)
					searchTerms.WriteUInt16(0);
				} else {
					// Start position range (0x8000 to 0xFFFF)
					searchTerms.WriteUInt16(0x8000);
					searchTerms.Write(m_searchTermsData, m_searchTermsDataSize);
				}
				DebugSend(Kad2SearchKeyReq, from->GetIPAddress(), from->GetUDPPort());
			} else {
				if (m_searchTermsDataSize == 0) {
					searchTerms.WriteUInt8(0);
					// We send this extra byte to flag we handle large files.
					searchTerms.WriteUInt8(0);
				} else {
					// Set to 2 to flag we handle large files.
					searchTerms.WriteUInt8(2);
					searchTerms.Write(m_searchTermsData, m_searchTermsDataSize);
				}
				DebugSendF(wxT("KadSearchReq(Keyword)"), from->GetIPAddress(), from->GetUDPPort());
			}
			if (from->GetVersion() >= 6) {
				CUInt128 clientID = from->GetClientID();
				CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_KEY_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
			} else if (from->GetVersion() >= 3) {
				CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_KEY_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
				wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
			} else {
				CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
			}
			m_totalRequestAnswers++;
			break;
		}
		case NOTES: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: Notes"));
			// Write complete packet.
			CMemFile searchTerms;
			searchTerms.WriteUInt128(m_target);
			if (from->GetVersion() >= 3) {
				// Find file we are storing info about.
				uint8_t fileid[16];
				m_target.ToByteArray(fileid);
				CKnownFile *file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
				if (file) {
					// Start position range (0x0 to 0x7FFF)
					searchTerms.WriteUInt64(file->GetFileSize());
					DebugSend(Kad2SearchNotesReq, from->GetIPAddress(), from->GetUDPPort());
					if (from->GetVersion() >= 6) {
						CUInt128 clientID = from->GetClientID();
						CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
					} else {
						CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
						wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
					}
				} else {
					PrepareToStop();
					break;
				}
			} else {
				searchTerms.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
				DebugSend(KadSearchNotesReq, from->GetIPAddress(), from->GetUDPPort());
				CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
			}
			m_totalRequestAnswers++;
			break;
		}
		case STOREFILE: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreFile"));
			// Try to store ourselves as a source to a Node.
			// As a safeguard, check to see if we already stored to the max nodes.
			if (m_answers > SEARCHSTOREFILE_TOTAL) {
				PrepareToStop();
				break;
			}

			// Find the file we are trying to store as a source to.
			uint8_t fileid[16];
			m_target.ToByteArray(fileid);
			CKnownFile* file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
			if (file) {
				// We store this mostly for GUI reasons.
				m_fileName = file->GetFileName().GetPrintable();

				// Get our clientID for the packet.
				CUInt128 id(CKademlia::GetPrefs()->GetClientHash());
				TagPtrList taglist;

				//We can use type for different types of sources. 
				//1 HighID sources..
				//2 cannot be used as older clients will not work.
				//3 Firewalled Kad Source.
				//4 >4GB file HighID Source.
				//5 >4GB file Firewalled Kad source.
				//6 Firewalled source with Direct Callback (supports >4GB)

				bool directCallback = false;
				if (theApp->IsFirewalled()) {
					directCallback = (Kademlia::CKademlia::IsRunning() && !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified());
					if (directCallback) {
						// firewalled, but direct udp callback is possible so no need for buddies
						// We are not firewalled..
						taglist.push_back(new CTagVarInt(TAG_SOURCETYPE, 6));
						taglist.push_back(new CTagVarInt(TAG_SOURCEPORT, thePrefs::GetPort()));
						if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
							taglist.push_back(new CTagInt16(TAG_SOURCEUPORT, CKademlia::GetPrefs()->GetInternKadPort()));
						}
						if (from->GetVersion() >= 2) {
							taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
						}
					} else if (theApp->clientlist->GetBuddy()) {	// We are firewalled, make sure we have a buddy.
						// We send the ID to our buddy so they can do a callback.
						CUInt128 buddyID(true);
						buddyID ^= CKademlia::GetPrefs()->GetKadID();
						taglist.push_back(new CTagInt8(TAG_SOURCETYPE, file->IsLargeFile() ? 5 : 3));
						taglist.push_back(new CTagVarInt(TAG_SERVERIP, theApp->clientlist->GetBuddy()->GetIP()));
						taglist.push_back(new CTagVarInt(TAG_SERVERPORT, theApp->clientlist->GetBuddy()->GetUDPPort()));
						uint8_t hashBytes[16];
						buddyID.ToByteArray(hashBytes);
						taglist.push_back(new CTagString(TAG_BUDDYHASH, CMD4Hash(hashBytes).Encode()));
						taglist.push_back(new CTagVarInt(TAG_SOURCEPORT, thePrefs::GetPort()));
						if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
							taglist.push_back(new CTagInt16(TAG_SOURCEUPORT, CKademlia::GetPrefs()->GetInternKadPort()));
						}
						if (from->GetVersion() >= 2) {
							taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
						}
					} else {
						// We are firewalled, but lost our buddy.. Stop everything.
						PrepareToStop();
						break;
					}
				} else {
					// We're not firewalled..
					taglist.push_back(new CTagInt8(TAG_SOURCETYPE, file->IsLargeFile() ? 4 : 1));
					taglist.push_back(new CTagVarInt(TAG_SOURCEPORT, thePrefs::GetPort()));
					if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
						taglist.push_back(new CTagInt16(TAG_SOURCEUPORT, CKademlia::GetPrefs()->GetInternKadPort()));
					}
					if (from->GetVersion() >= 2) {
						taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
					}
				}

				taglist.push_back(new CTagInt8(TAG_ENCRYPTION, CPrefs::GetMyConnectOptions(true, true)));

				// Send packet
				CKademlia::GetUDPListener()->SendPublishSourcePacket(*from, m_target, id, taglist);
				m_totalRequestAnswers++;
				// Delete all tags.
				deleteTagPtrListEntries(&taglist);
			} else {
				PrepareToStop();
			}
			break;
		}
		case STOREKEYWORD: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreKeyword"));
			// Try to store keywords to a Node.
			// As a safeguard, check to see if we already stored to the max nodes.
			if (m_answers > SEARCHSTOREKEYWORD_TOTAL) {
				PrepareToStop();
				break;
			}

			uint16_t count = m_fileIDs.size();
			if (count == 0) {
				PrepareToStop();
				break;
			} else if (count > 150) {
				count = 150;
			}

			UIntList::const_iterator itListFileID = m_fileIDs.begin();
			uint8_t fileid[16];

			while (count && (itListFileID != m_fileIDs.end())) {
				uint16_t packetCount = 0;
				CMemFile packetdata(1024*50); // Allocate a good amount of space.			
				packetdata.WriteUInt128(m_target);
				packetdata.WriteUInt16(0); // Will be updated before sending.
				while ((packetCount < 50) && (itListFileID != m_fileIDs.end())) {
					CUInt128 id(*itListFileID);
					id.ToByteArray(fileid);
					CKnownFile *pFile = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
					if (pFile) {
						count--;
						packetCount++;
						packetdata.WriteUInt128(id);
						PreparePacketForTags(&packetdata, pFile);
					}
					++itListFileID;
				}

				// Correct file count.
				uint64_t current_pos = packetdata.GetPosition();
				packetdata.Seek(16);
				packetdata.WriteUInt16(packetCount);
				packetdata.Seek(current_pos);

				// Send packet
				if (from->GetVersion() >= 6) {
					DebugSend(Kad2PublishKeyReq, from->GetIPAddress(), from->GetUDPPort());
					CUInt128 clientID = from->GetClientID();
					CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_KEY_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
				} else if (from->GetVersion() >= 2) {
					DebugSend(Kad2PublishKeyReq, from->GetIPAddress(), from->GetUDPPort());
					CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_KEY_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
					wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
				} else {
					wxFAIL;
				}
			}
			m_totalRequestAnswers++;
			break;
		}
		case STORENOTES: {
			AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreNotes"));
			// Find file we are storing info about.
			uint8_t fileid[16];
			m_target.ToByteArray(fileid);
			CKnownFile* file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));

			if (file) {
				CMemFile packetdata(1024*2);
				// Send the hash of the file we're storing info about.
				packetdata.WriteUInt128(m_target);
				// Send our ID with the info.
				packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());

				// Create our taglist.
				TagPtrList taglist;
				taglist.push_back(new CTagString(TAG_FILENAME, file->GetFileName().GetPrintable()));
				if (file->GetFileRating() != 0) {
					taglist.push_back(new CTagVarInt(TAG_FILERATING, file->GetFileRating()));
				}
				if (!file->GetFileComment().IsEmpty()) {
					taglist.push_back(new CTagString(TAG_DESCRIPTION, file->GetFileComment()));
				}
				if (from->GetVersion() >= 2) {
					taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
				}
				packetdata.WriteTagPtrList(taglist);

				// Send packet
				if (from->GetVersion() >= 6) {
					DebugSend(Kad2PublishNotesReq, from->GetIPAddress(), from->GetUDPPort());
					CUInt128 clientID = from->GetClientID();
					CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
				} else if (from->GetVersion() >= 2) {
					DebugSend(Kad2PublishNotesReq, from->GetIPAddress(), from->GetUDPPort());
					CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
					wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
				} else {
					wxFAIL;
				}
				m_totalRequestAnswers++;
				// Delete all tags.
				deleteTagPtrListEntries(&taglist);
			} else {
				PrepareToStop();
			}
			break;
		}
		case FINDBUDDY:
		{
			AddDebugLogLineN(logKadSearch, wxT("Search request type: FindBuddy"));
			// Send a buddy request as we are firewalled.
			// As a safeguard, check to see if we already requested the max nodes.
			if (m_answers > SEARCHFINDBUDDY_TOTAL) {
				PrepareToStop();
				break;
			}

			CMemFile packetdata;
			// Send the ID we used to find our buddy. Used for checks later and allows users to callback someone if they change buddies.
			packetdata.WriteUInt128(m_target);
			// Send client hash so they can do a callback.
			packetdata.WriteUInt128(CKademlia::GetPrefs()->GetClientHash());
			// Send client port so they can do a callback.
			packetdata.WriteUInt16(thePrefs::GetPort());

			DebugSend(KadFindBuddyReq, from->GetIPAddress(), from->GetUDPPort());
			if (from->GetVersion() >= 6) {
				CUInt128 clientID = from->GetClientID();
				CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_FINDBUDDY_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
			} else {
				CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_FINDBUDDY_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
				wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
			}
			m_answers++;
			break;
		}
		case FINDSOURCE:
		{
			AddDebugLogLineN(logKadSearch, wxT("Search request type: FindSource"));
			// Try to find if this is a buddy to someone we want to contact.
			// As a safeguard, check to see if we already requested the max nodes.
			if (m_answers > SEARCHFINDSOURCE_TOTAL) {
				PrepareToStop();
				break;
			}

			CMemFile packetdata(34);
			// This is the ID that the person we want to contact used to find a buddy.
			packetdata.WriteUInt128(m_target);
			if (m_fileIDs.size() != 1) {
				throw wxString(wxT("Kademlia.CSearch.processResponse: m_fileIDs.size() != 1"));
			}
			// Currently, we limit the type of callbacks for sources. We must know a file this person has for it to work.
			packetdata.WriteUInt128(m_fileIDs.front());
			// Send our port so the callback works.
			packetdata.WriteUInt16(thePrefs::GetPort());
			// Send packet
			DebugSend(KadCallbackReq, from->GetIPAddress(), from->GetUDPPort());
			if (from->GetVersion() >= 6) {
				CUInt128 clientID = from->GetClientID();
				CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_CALLBACK_REQ, from->GetIPAddress(), from->GetUDPPort(), from->GetUDPKey(), &clientID);
			} else {
				CKademlia::GetUDPListener()->SendPacket( packetdata, KADEMLIA_CALLBACK_REQ, from->GetIPAddress(), from->GetUDPPort(), 0, NULL);
				wxASSERT(from->GetUDPKey() == CKadUDPKey(0));
			}
			m_answers++;
			break;
		}
		case NODESPECIAL: {
			// we are looking for the IP of a given NodeID, so we just check if we 0 distance and if so, report the
			// tip to the requester
			if (fromDistance == 0) {
				m_nodeSpecialSearchRequester->KadSearchIPByNodeIDResult(KCSR_SUCCEEDED, wxUINT32_SWAP_ALWAYS(from->GetIPAddress()), from->GetTCPPort());
				m_nodeSpecialSearchRequester = NULL;
				PrepareToStop();
			}
			break;
		 }
		case NODECOMPLETE:
			AddDebugLogLineN(logKadSearch, wxT("Search request type: NodeComplete"));
			break;
		case NODE:
			AddDebugLogLineN(logKadSearch, wxT("Search request type: Node"));
			break;
		default:
			AddDebugLogLineN(logKadSearch, CFormat(wxT("Search result type: Unknown (%i)")) % m_type);
			break;
	}
}

void CSearch::ProcessResult(const CUInt128& answer, TagPtrList *info)
{
	wxString type = wxT("Unknown");
	switch (m_type) {
		case FILE:
			type = wxT("File");
			ProcessResultFile(answer, info);
			break;
		case KEYWORD:
			type = wxT("Keyword");
			ProcessResultKeyword(answer, info);
			break;
		case NOTES:
			type = wxT("Notes");
			ProcessResultNotes(answer, info);
			break;
	}
	AddDebugLogLineN(logKadSearch, wxT("Got result (") + type + wxT(")"));
}

void CSearch::ProcessResultFile(const CUInt128& answer, TagPtrList *info)
{
	// Process a possible source to a file.
	// Set of data we could receive from the result.
	uint8_t type = 0;
	uint32_t ip = 0;
	uint16_t tcp = 0;
	uint16_t udp = 0;
	uint32_t buddyip = 0;
	uint16_t buddyport = 0;
	uint8_t byCryptOptions = 0; // 0 = not supported.
	CUInt128 buddy;

	for (TagPtrList::const_iterator it = info->begin(); it != info->end(); ++it) {
		CTag *tag = *it;
		if (!tag->GetName().Cmp(TAG_SOURCETYPE)) {
			type = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEIP)) {
			ip = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
			tcp = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEUPORT)) {
			udp = tag->GetInt();
		} else if (!tag->GetName().Cmp((TAG_SERVERIP))) {
			buddyip = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SERVERPORT)) {
			buddyport = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_BUDDYHASH)) {
			CMD4Hash hash;
			// TODO: Error handling
			if (!hash.Decode(tag->GetStr())) {
#ifdef __DEBUG__
				printf("Invalid buddy-hash: '%s'\n", (const char*)tag->GetStr().fn_str());
#endif
			}
			buddy.SetValueBE(hash.GetHash());
		} else if (!tag->GetName().Cmp(TAG_ENCRYPTION)) {
			byCryptOptions = (uint8)tag->GetInt();
		}
	}

	// Process source based on its type. Currently only one method is needed to process all types.
	switch( type ) {
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
			AddDebugLogLineN(logKadSearch, CFormat(wxT("Trying to add a source type %i, ip %s")) % type % KadIPPortToString(ip, udp));
			m_answers++;
			theApp->downloadqueue->KademliaSearchFile(m_searchID, &answer, &buddy, type, ip, tcp, udp, buddyip, buddyport, byCryptOptions);
			break;
		case 2: 
			//Don't use this type, some clients will process it wrong.
		default:
			break;
	}
}

void CSearch::ProcessResultNotes(const CUInt128& answer, TagPtrList *info)
{
	// Process a received Note to a file.
	// Create a Note and set the IDs.
	CEntry* entry = new CEntry();
	entry->m_uKeyID.SetValue(m_target);
	entry->m_uSourceID.SetValue(answer);

	bool bFilterComment = false;

	// Loop through tags and pull wanted into. Currently we only keep Filename, Rating, Comment.
	for (TagPtrList::iterator it = info->begin(); it != info->end(); ++it) {
		CTag *tag = *it;
		if (!tag->GetName().Cmp(TAG_SOURCEIP)) {
			entry->m_uIP = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
			entry->m_uTCPport = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_FILENAME)) {
			entry->SetFileName(tag->GetStr());
		} else if (!tag->GetName().Cmp(TAG_DESCRIPTION)) {
			wxString strComment(tag->GetStr());
			bFilterComment = thePrefs::IsMessageFiltered(strComment);
			entry->AddTag(tag);
			*it = NULL;	// Prevent actual data being freed
		} else if (!tag->GetName().Cmp(TAG_FILERATING)) {
			entry->AddTag(tag);
			*it = NULL;	// Prevent actual data being freed
		}
	}

	if (bFilterComment) {
		delete entry;
		return;
	}

	uint8_t fileid[16];
	m_target.ToByteArray(fileid);
	const CMD4Hash fileHash(fileid);

	//Check if this hash is in our shared files..
	CKnownFile* file = theApp->sharedfiles->GetFileByID(fileHash);

	if (!file) {
		// If we didn't find anything check if it's in our download queue.
		file = (CKnownFile*)theApp->downloadqueue->GetFileByID(fileHash);
	}

	// If we found a file try to add the note to the file.
	if (file) {
		file->AddNote(entry);
		m_answers++;
	} else {
		AddDebugLogLineN(logKadSearch, wxT("Comment received for unknown file"));
		delete entry;
	}
}

void CSearch::ProcessResultKeyword(const CUInt128& answer, TagPtrList *info)
{
	// Process a keyword that we received.
	// Set of data we can use for a keyword result.
	wxString name;
	uint64_t size = 0;
	wxString type;
	wxString format;
	wxString artist;
	wxString album;
	wxString title;
	uint32_t length = 0;
	wxString codec;
	uint32_t bitrate = 0;
	uint32_t availability = 0;
	uint32_t publishInfo = 0;
	// Flag that is set if we want this keyword
	bool bFileName = false;
	bool bFileSize = false;

	for (TagPtrList::const_iterator it = info->begin(); it != info->end(); ++it) {
		CTag* tag = *it;
		if (tag->GetName() == TAG_FILENAME) {
			name = tag->GetStr();
			bFileName = !name.IsEmpty();
		} else if (tag->GetName() == TAG_FILESIZE) {
			if (tag->IsBsob() && (tag->GetBsobSize() == 8)) {
				// Kad1.0 uint64 type using a BSOB.
				size = PeekUInt64(tag->GetBsob());
			} else {
				wxASSERT(tag->IsInt());
				size = tag->GetInt();
			}
			bFileSize = true;
		} else if (tag->GetName() == TAG_FILETYPE) {
			type = tag->GetStr();
		} else if (tag->GetName() == TAG_FILEFORMAT) {
			format = tag->GetStr();
		} else if (tag->GetName() == TAG_MEDIA_ARTIST) {
			artist = tag->GetStr();
		} else if (tag->GetName() == TAG_MEDIA_ALBUM) {
			album = tag->GetStr();
		} else if (tag->GetName() == TAG_MEDIA_TITLE) {
			title = tag->GetStr();
		} else if (tag->GetName() == TAG_MEDIA_LENGTH) {
			length = tag->GetInt();
		} else if (tag->GetName() == TAG_MEDIA_BITRATE) {
			bitrate = tag->GetInt();
		} else if (tag->GetName() == TAG_MEDIA_CODEC) {
			codec = tag->GetStr();
		} else if (tag->GetName() == TAG_SOURCES) {
			availability = tag->GetInt();
			// Some rouge client was setting a invalid availability, just set it to 0.
			if( availability > 65500 ) {
				availability = 0;
			}
		} else if (tag->GetName() == TAG_PUBLISHINFO) {
			// we don't keep this as tag, but as a member property of the searchfile, as we only need its informations
			// in the search list and don't want to carry the tag over when downloading the file (and maybe even wrongly publishing it)
			publishInfo = (uint32_t)tag->GetInt();
#ifdef __DEBUG__
			uint32_t differentNames = (publishInfo & 0xFF000000) >> 24;
			uint32_t publishersKnown = (publishInfo & 0x00FF0000) >> 16;
			uint32_t trustValue = publishInfo & 0x0000FFFF;
			AddDebugLogLineN(logKadSearch, CFormat(wxT("Received PublishInfo Tag: %u different names, %u publishers, %.2f trustvalue")) % differentNames % publishersKnown % ((double)trustValue/ 100.0));
#endif
		}
	}

	// If we don't have a valid filename and filesize, drop this keyword.
	if (!bFileName || !bFileSize) {
		AddDebugLogLineN(logKadSearch, wxString(wxT("No ")) + (!bFileName ? wxT("filename") : wxT("filesize")) + wxT(" on search result, ignoring"));
		return;
	}

	// the file name of the current search response is stored in "name"
	// the list of words the user entered is stored in "m_words"
	// so the file name now gets parsed for all the words entered by the user (even repetitive ones):

	// Step 1: Get the words of the response file name
	WordList listFileNameWords;
	CSearchManager::GetWords(name, &listFileNameWords, true);

	// Step 2: Look for each entered search word in those present in the filename
	bool bFileNameMatchesSearch = true;  // this will be set to "false", if not all search words are found in the file name

	for (WordList::const_iterator itSearchWords = m_words.begin(); itSearchWords != m_words.end(); ++itSearchWords) {
		bool bSearchWordPresent = false;
		for (WordList::iterator itFileNameWords = listFileNameWords.begin(); itFileNameWords != listFileNameWords.end(); ++itFileNameWords) {
			if (!itFileNameWords->CmpNoCase(*itSearchWords)) {
				listFileNameWords.erase(itFileNameWords);  // remove not to find same word twice
				bSearchWordPresent = true;
				break;  // found word, go on using the next searched word
			}
		}
		if (!bSearchWordPresent) {
			bFileNameMatchesSearch = false;  // not all search words were found in the file name
			break;
		}
	}

	// Step 3: Accept result only if all(!) words are found
	if (bFileNameMatchesSearch) {
		TagPtrList taglist;

		if (!format.IsEmpty()) {
			taglist.push_back(new CTagString(TAG_FILEFORMAT, format));
		}
		if (!artist.IsEmpty()) {
			taglist.push_back(new CTagString(TAG_MEDIA_ARTIST, artist));
		}
		if (!album.IsEmpty()) {
			taglist.push_back(new CTagString(TAG_MEDIA_ALBUM, album));
		}
		if (!title.IsEmpty()) {
			taglist.push_back(new CTagString(TAG_MEDIA_TITLE, title));
		}
		if (length) {
			taglist.push_back(new CTagVarInt(TAG_MEDIA_LENGTH, length));
		}
		if (bitrate) {
			taglist.push_back(new CTagVarInt(TAG_MEDIA_BITRATE, bitrate));
		}
		if (availability) {
			taglist.push_back(new CTagVarInt(TAG_SOURCES, availability));
		}

		m_answers++;
		theApp->searchlist->KademliaSearchKeyword(m_searchID, &answer, name, size, type, publishInfo, taglist);

		// Free tags memory
		deleteTagPtrListEntries(&taglist);
	}
}

void CSearch::SendFindValue(CContact *contact, bool reaskMore)
{
	// Found a node that we think has contacts closer to our target.
	try {
		if (m_stopping) {
			return;
		}

		CMemFile packetdata(33);
		// The number of returned contacts is based on the type of search.
		uint8_t contactCount = GetRequestContactCount();

		if (reaskMore) {
			if (m_requestedMoreNodesContact == NULL) {
				m_requestedMoreNodesContact = contact;
				wxASSERT(contactCount == KADEMLIA_FIND_VALUE);
				contactCount = KADEMLIA_FIND_VALUE_MORE;
			} else {
				wxFAIL;
			}
		}

		if (contactCount > 0) {
			packetdata.WriteUInt8(contactCount);
		} else {
			return;
		}

		// Put the target we want into the packet.
		packetdata.WriteUInt128(m_target);
		// Add the ID of the contact we're contacting for sanity checks on the other end.
		packetdata.WriteUInt128(contact->GetClientID());
		if (contact->GetVersion() >= 2) {
			if (contact->GetVersion() >= 6) {
				CUInt128 clientID = contact->GetClientID();
				CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_REQ, contact->GetIPAddress(), contact->GetUDPPort(), contact->GetUDPKey(), &clientID);
			} else {
				CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_REQ, contact->GetIPAddress(), contact->GetUDPPort(), 0, NULL);
				wxASSERT(contact->GetUDPKey() == CKadUDPKey(0));
			}
#ifdef __DEBUG__
			switch (m_type) {
				case NODE:
					DebugSendF(wxT("Kad2Req(Node)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case NODECOMPLETE:
					DebugSendF(wxT("Kad2Req(NodeComplete)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case NODESPECIAL:
					DebugSendF(wxT("Kad2Req(NodeSpecial)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case NODEFWCHECKUDP:
					DebugSendF(wxT("Kad2Req(NodeFWCheckUDP)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case FILE:
					DebugSendF(wxT("Kad2Req(File)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case KEYWORD:
					DebugSendF(wxT("Kad2Req(Keyword)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case STOREFILE:
					DebugSendF(wxT("Kad2Req(StoreFile)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case STOREKEYWORD:
					DebugSendF(wxT("Kad2Req(StoreKeyword)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case STORENOTES:
					DebugSendF(wxT("Kad2Req(StoreNotes)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				case NOTES:
					DebugSendF(wxT("Kad2Req(Notes)"), contact->GetIPAddress(), contact->GetUDPPort());
					break;
				default:
					DebugSend(Kad2Req, contact->GetIPAddress(), contact->GetUDPPort());
					break;
			}
#endif
		} else {
			wxFAIL;
		}
	} catch (const CEOFException& err) {
		AddDebugLogLineC(logKadSearch, wxT("CEOFException in CSearch::SendFindValue: ") + err.what());
	} catch (const CInvalidPacket& err) {
		AddDebugLogLineC(logKadSearch, wxT("CInvalidPacket Exception in CSearch::SendFindValue: ") + err.what());
	} catch (const wxString& e) {
		AddDebugLogLineC(logKadSearch, wxT("Exception in CSearch::SendFindValue: ") + e);
	}
}

// TODO: Redundant metadata checks
void CSearch::PreparePacketForTags(CMemFile *bio, CKnownFile *file)
{
	// We're going to publish a keyword, set up the tag list
	TagPtrList taglist;
	
	try {
		if (file && bio) {
			// Name, Size
			taglist.push_back(new CTagString(TAG_FILENAME, file->GetFileName().GetPrintable()));
			taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
			taglist.push_back(new CTagVarInt(TAG_SOURCES, file->m_nCompleteSourcesCount));

			// eD2K file type (Audio, Video, ...)
			// NOTE: Archives and CD-Images are published with file type "Pro"
			wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName())));
			if (!strED2KFileType.IsEmpty()) {
				taglist.push_back(new CTagString(TAG_FILETYPE, strED2KFileType));
			}
			
			// additional meta data (Artist, Album, Codec, Length, ...)
			// only send verified meta data to nodes
			if (file->GetMetaDataVer() > 0) {
				static const struct{
					uint8_t	nName;
					uint8_t	nType;
				} _aMetaTags[] = 
				{
					{ FT_MEDIA_ARTIST,  2 },
					{ FT_MEDIA_ALBUM,   2 },
					{ FT_MEDIA_TITLE,   2 },
					{ FT_MEDIA_LENGTH,  3 },
					{ FT_MEDIA_BITRATE, 3 },
					{ FT_MEDIA_CODEC,   2 }
				};
				for (unsigned int i = 0; i < itemsof(_aMetaTags); i++) {
					const ::CTag* pTag = file->GetTag(_aMetaTags[i].nName, _aMetaTags[i].nType);
					if (pTag) {
						// skip string tags with empty string values
						if (pTag->IsStr() && pTag->GetStr().IsEmpty()) {
							continue;
						}
						// skip integer tags with '0' values
						if (pTag->IsInt() && pTag->GetInt() == 0) {
							continue;
						}
						wxString szKadTagName = CFormat(wxT("%c")) % pTag->GetNameID();
						if (pTag->IsStr()) {
							taglist.push_back(new CTagString(szKadTagName, pTag->GetStr()));
						} else {
							taglist.push_back(new CTagVarInt(szKadTagName, pTag->GetInt()));
						}
					}
				}
			}
			bio->WriteTagPtrList(taglist);
		} else {
			//If we get here.. Bad things happen.. Will fix this later if it is a real issue.
			wxFAIL;
		}
	} catch (const CEOFException& err) {
		AddDebugLogLineC(logKadSearch, wxT("CEOFException in CSearch::PreparePacketForTags: ") + err.what());
	} catch (const CInvalidPacket& err) {
		AddDebugLogLineC(logKadSearch, wxT("CInvalidPacket Exception in CSearch::PreparePacketForTags: ") + err.what());
	} catch (const wxString& e) {
		AddDebugLogLineC(logKadSearch, wxT("Exception in CSearch::PreparePacketForTags: ") + e);
	} 

	deleteTagPtrListEntries(&taglist);
}

void CSearch::SetSearchTermData(uint32_t searchTermsDataSize, const uint8_t *searchTermsData)
{
	m_searchTermsDataSize = searchTermsDataSize;
	m_searchTermsData = new uint8_t [searchTermsDataSize];
	memcpy(m_searchTermsData, searchTermsData, searchTermsDataSize);
}

uint8_t CSearch::GetRequestContactCount() const
{
	// Returns the amount of contacts we request on routing queries based on the search type
	switch (m_type) {
		case NODE:
		case NODECOMPLETE:
		case NODESPECIAL:
		case NODEFWCHECKUDP:
			return KADEMLIA_FIND_NODE;
		case FILE:
		case KEYWORD:
		case FINDSOURCE:
		case NOTES:
			return KADEMLIA_FIND_VALUE;
		case FINDBUDDY:
		case STOREFILE:
		case STOREKEYWORD:
		case STORENOTES:
			return KADEMLIA_STORE;
		default:
			AddDebugLogLineN(logKadSearch, wxT("Invalid search type. (CSearch::GetRequestContactCount())"));
			wxFAIL;
			return 0;
	}
}
