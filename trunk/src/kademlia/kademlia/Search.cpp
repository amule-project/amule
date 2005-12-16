//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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


#include "../../Tag.h"
typedef CTag ed2kCTag;

#include "Search.h"
#include "Kademlia.h"
#include "../../OPCodes.h"
#include "Defines.h"
#include "Prefs.h"
#include "Indexed.h"
#include "../io/IOException.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../kademlia/Tag.h"
#include "../../amule.h"
#include "../../SharedFileList.h"
#include "../../OtherFunctions.h"
#include "../../KnownFile.h"
#include "DownloadQueue.h"
#include "PartFile.h"
#include "SearchList.h"
#include "MemFile.h"
#include "ServerConnect.h"
#include "Server.h"
#include "ClientList.h"
#include "updownclient.h"
#include "Logger.h"
#include "../../Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CSearch::CSearch()
{
	m_created = time(NULL);
	m_searchTerms = NULL;
	m_type = (uint32)-1;
	m_answers = 0;
	m_totalRequestAnswers = 0;
	m_kadPacketSent = 0;
	m_searchID = (uint32)-1;
	m_keywordPublish = 0;
	(void)m_fileName;
	m_stoping = false;
	m_totalLoad = 0;
	m_totalLoadResponses = 0;
	bio1 = NULL;
	bio2 = NULL;
	bio3 = NULL;
	m_lastResponse = time(NULL);
}

CSearch::~CSearch()
{
	
	CPartFile* temp = theApp.downloadqueue->GetFileByKadFileSearchID(getSearchID());
	
	if(temp) {
		temp->SetKadFileSearchID(0);
	}	
	
	delete m_searchTerms;

	ContactMap::iterator it;
	for (it = m_inUse.begin(); it != m_inUse.end(); it++) {
		((CContact*)it->second)->decUse();
	}

	ContactList::const_iterator it2;
	for (it2 = m_delete.begin(); it2 != m_delete.end(); it2++) {
		delete *it2;
	}
	
	delete bio1;
	delete bio2;
	delete bio3;

	if(CKademlia::isRunning() && getNodeLoad() > 20) {
		switch(getSearchTypes()) {
			case CSearch::STOREKEYWORD:
				Kademlia::CKademlia::getIndexed()->AddLoad(getTarget(), ((uint32)(DAY2S(7)*((double)getNodeLoad()/100.0))+(uint32)time(NULL)));
				break;
			default:
				;
				// WTF? 
		}
	}
}

void CSearch::go(void)
{
	// Start with a lot of possible contacts, this is a fallback in case search stalls due to dead contacts
	if (m_possible.empty()) {
		CUInt128 distance(CKademlia::getPrefs()->getKadID());
		distance.XOR(m_target);
		CKademlia::getRoutingZone()->getClosestTo(3, m_target, distance, 50, &m_possible, true, true);
	}
	
	if (m_possible.empty()) {
		return;
	}

	ContactMap::iterator it;
	//Lets keep our contact list entries in mind to dec the inUse flag.
	for (it = m_possible.begin(); it != m_possible.end(); ++it) {
		m_inUse[it->first] = it->second;
	}
	wxASSERT(m_possible.size() == m_inUse.size());
	// Take top 3 possible
	int count = min(ALPHA_QUERY, (int)m_possible.size());
	CContact *c;
	for (int i=0; i<count; i++) {
		it = m_possible.begin();
		c = it->second;
		// Move to tried
		m_tried[it->first] = c;
		m_possible.erase(it);
		// Send request
		c->checkingType();
		sendFindValue(c->getClientID(), c->getIPAddress(), c->getUDPPort());
		if(m_type == NODE) {
			break;
		}
	}
}

//If we allow about a 15 sec delay before deleting, we won't miss a lot of delayed returning packets.
void CSearch::prepareToStop()
{
	if( m_stoping ) {
		return;
	}
	uint32 baseTime = 0;
	switch(m_type) {
		case NODE:
		case NODECOMPLETE:
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
	m_created = time(NULL) - baseTime + SEC(15);
	m_stoping = true;	
}

void CSearch::jumpStart(void)
{
	if (m_possible.empty()) {
		prepareToStop();
		return;
	}

	if ((time_t)(m_lastResponse + SEC(3)) > time(NULL)) {
		return;
	}

	while (!m_possible.empty()) {
		
		CContact *c = m_possible.begin()->second;
	
		//Have we already tried to contact this node.
		if (m_tried.count(m_possible.begin()->first) > 0) {
			//Did we get a response from this node, if so, try to store or get info.
			if(m_responded.count(m_possible.begin()->first) > 0) {
				StorePacket();
			}
			m_possible.erase(m_possible.begin());
		} else {
			// Move to tried
			m_tried[m_possible.begin()->first] = c;
			// Send request
			c->checkingType();
			sendFindValue(c->getClientID(), c->getIPAddress(), c->getUDPPort());
			break;
		}
	}
	
}

void CSearch::processResponse(uint32 fromIP, uint16 fromPort, ContactList *results)
{
	AddDebugLogLineM(false, logKadSearch, wxT("Process search response from ") + Uint32_16toStringIP_Port(fromIP, fromPort));

	if (results) {
		m_lastResponse = time(NULL);
	}

	// Remember the contacts to be deleted when finished
	ContactList::iterator response;
	for (response = results->begin(); response != results->end(); ++response) {
		m_delete.push_back(*response);
	}

	// Not interested in responses for FIND_NODE, will be added to contacts by udp listener
	if (m_type == NODE) {
		AddDebugLogLineM(false, logKadSearch, wxT("Node type search result, discarding."));
		m_answers++;
		m_possible.clear();
		delete results;
		return;
	}

	ContactMap::const_iterator tried;
	CContact *c;
	CContact *from;
	CUInt128 distance;
	CUInt128 fromDistance;

	//Find contact that is responding.
	for (tried = m_tried.begin(); tried != m_tried.end(); ++tried) {
		fromDistance = tried->first;
		from = tried->second;

		if ((from->getIPAddress() == fromIP) && (from->getUDPPort() == fromPort)) {
			// Add to list of people who responded
			m_responded[fromDistance] = from;

			// Loop through their responses
			for (response = results->begin(); response != results->end(); ++response) {
				c = *response;

				distance = c->getClientID();
				distance.XOR(m_target);

				// Ignore this contact if already know him
				if (m_possible.count(distance) > 0) {
					// AddDebugLogLineM(false, logKadSearch, wxT("Search result from already known client: ignore"));
					continue;
				}
				if (m_tried.count(distance) > 0) {
					// AddDebugLogLineM(false, logKadSearch, wxT("Search result from already tried client: ignore"));
					continue;
				}
				
				// Add to possible
				m_possible[distance] = c;
				
				if (distance < fromDistance) {

					bool top = false;
					if (m_best.size() < ALPHA_QUERY) {
						top = true;
						m_best[distance] = c;
					} else {
						ContactMap::iterator it = m_best.end();
						--it;
						if (distance < it->first) {
							// Rotate best list
							m_best.erase(it);
							m_best[distance] = c;
							top = true;
						}
					}
					
					if (top) {
						// Add to tried
						m_tried[distance] = c;
						// Send request
						c->checkingType();
						sendFindValue(c->getClientID(), c->getIPAddress(), c->getUDPPort());
					}
				}
			}

			// We don't want anything from these people, so just increment the counter.
			if( m_type == NODECOMPLETE ) {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Node complete"));
				m_answers++;
			}
			break;
		}
	}
	
	delete results;
}

void CSearch::StorePacket()
{
	wxASSERT(!m_possible.empty());
	
	CContact *from;
	CUInt128 fromDistance;
	ContactMap::const_iterator possible;

	possible = m_possible.begin();
	fromDistance = possible->first;
	from = possible->second;

	if(thePrefs::FilterLanIPs() && fromDistance.get32BitChunk(0) > SEARCHTOLERANCE) {
		AddDebugLogLineM(false, logKadSearch, wxT("Not stored: filtered lan ip"));
		return;
	}

	switch(m_type) {
		case FILE:
		case KEYWORD: {
			if (m_type == FILE) {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: File"));
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchReq (File) ") + Uint32_16toStringIP_Port(from->getIPAddress(), from->getUDPPort()));
			} else {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Keyword"));
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchReq (Keyword) ") + Uint32_16toStringIP_Port(from->getIPAddress(), from->getUDPPort()));
			}
			wxASSERT( m_searchTerms->GetLength() > 0 );
			// The data in 'm_searchTerms' is to be sent several times, so use the don't detach flag.
			CKademlia::getUDPListener()->sendPacket(m_searchTerms, KADEMLIA_SEARCH_REQ, from->getIPAddress(), from->getUDPPort());
			m_totalRequestAnswers++;
			break;
		}
		case NOTES: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Notes"));
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			bio.WriteUInt128(CKademlia::getPrefs()->getKadID());
			CKademlia::getUDPListener()->sendPacket( &bio, KADEMLIA_SRC_NOTES_REQ, from->getIPAddress(), from->getUDPPort());
			m_totalRequestAnswers++;
			break;
		}
		case STOREFILE: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreFile"));
			if( m_answers > SEARCHSTOREFILE_TOTAL ) {
				prepareToStop();
				break;
			}
			byte fileid[16];
			m_target.toByteArray(fileid);
			CKnownFile* file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
			if (file) {
				m_fileName = file->GetFileName();

				CUInt128 id(CKademlia::getPrefs()->getClientHash());
				TagList taglist;

				//We can use type for different types of sources. 
				//1 is reserved for highID sources..
				//2 cannot be used as older clients will not work.
				//3 Firewalled Kad Source.
	
				if( theApp.IsFirewalled() ) {
					if( theApp.clientlist->GetBuddy() ) {
						CUInt128 buddyID(true);
						buddyID.XOR(CKademlia::getPrefs()->getKadID());
						taglist.push_back(new CTagUInt8(TAG_SOURCETYPE, 3));
						taglist.push_back(new CTagUInt32(TAG_SERVERIP, theApp.clientlist->GetBuddy()->GetIP()));
						taglist.push_back(new CTagUInt16(TAG_SERVERPORT, theApp.clientlist->GetBuddy()->GetUDPPort()));
						byte hashBytes[16];
						buddyID.toByteArray(hashBytes);
						taglist.push_back(new CTagStr(TAG_BUDDYHASH, CMD4Hash(hashBytes).Encode()));
						taglist.push_back(new CTagUInt16(TAG_SOURCEPORT, thePrefs::GetPort()));
					} else {
						prepareToStop();
						break;
					}
				} else {
					taglist.push_back(new CTagUInt8(TAG_SOURCETYPE, 1));
					taglist.push_back(new CTagUInt16(TAG_SOURCEPORT, thePrefs::GetPort()));
				}

				CKademlia::getUDPListener()->publishPacket(from->getIPAddress(), from->getUDPPort(),m_target,id, taglist);
				m_totalRequestAnswers++;
				TagList::const_iterator it;
				for (it = taglist.begin(); it != taglist.end(); ++it) {
					delete *it;
				}
			}
			break;
		}
		case STOREKEYWORD: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreKeyword"));
			if( m_answers > SEARCHSTOREKEYWORD_TOTAL ) {
				prepareToStop();
				break;
			}
			if( bio1 ) {
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadStoreKeywReq ") + Uint32_16toStringIP_Port(from->getIPAddress(), from->getUDPPort()));								
				CKademlia::getUDPListener()->sendPacket( packet1, ((1024*50)-bio1->getAvailable()), from->getIPAddress(), from->getUDPPort() );
			}
			if( bio2 ) {
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadStoreKeywReq ") + Uint32_16toStringIP_Port(from->getIPAddress(), from->getUDPPort()));											
				CKademlia::getUDPListener()->sendPacket( packet2, ((1024*50)-bio2->getAvailable()), from->getIPAddress(), from->getUDPPort() );
			}
			if( bio3 ) {
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadStoreKeywReq ")  + Uint32_16toStringIP_Port(from->getIPAddress(), from->getUDPPort()));								
				CKademlia::getUDPListener()->sendPacket( packet3, ((1024*50)-bio3->getAvailable()), from->getIPAddress(), from->getUDPPort() );
			}
			m_totalRequestAnswers++;
			break;
		}
		case STORENOTES: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreNotes"));
			byte fileid[16];
			m_target.toByteArray(fileid);
			CKnownFile* file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
			if (file) {
				byte packet[1024*2];
				CByteIO bio(packet,sizeof(packet));
				bio.writeUInt128(m_target);
				bio.writeUInt128(CKademlia::getPrefs()->getKadID());
				uint8 tagcount = 1;
				if(file->GetFileRating() != 0) {
					++tagcount;
				}
				if(!file->GetFileComment().IsEmpty()) {
					++tagcount;
				}
				//Number of tags.
				bio.writeUInt8(tagcount);
				CTagStr fileName(TAG_FILENAME, file->GetFileName());
				bio.writeTag(&fileName);
				if(file->GetFileRating() != 0) {
					CTagUInt16 rating(TAG_FILERATING, file->GetFileRating());
					bio.writeTag(&rating);
				}
				if(!file->GetFileComment().IsEmpty()) {
					CTagStr description(TAG_DESCRIPTION, file->GetFileComment());
					bio.writeTag(&description);
				}

				CKademlia::getUDPListener()->sendPacket( packet, sizeof(packet)-bio.getAvailable(), KADEMLIA_PUB_NOTES_REQ, from->getIPAddress(), from->getUDPPort());
				m_totalRequestAnswers++;
			}
			break;
		}
		case FINDBUDDY:
		{
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: FindBuddy"));
			if( m_answers > SEARCHFINDBUDDY_TOTAL ) {
				prepareToStop();
				break;
			}
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			bio.WriteUInt128(CKademlia::getPrefs()->getClientHash());
			bio.WriteUInt16(thePrefs::GetPort());
			CKademlia::getUDPListener()->sendPacket( &bio, KADEMLIA_FINDBUDDY_REQ, from->getIPAddress(), from->getUDPPort());
			m_answers++;
			break;
		}
		case FINDSOURCE:
		{
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: FindSource"));
			if( m_answers > SEARCHFINDSOURCE_TOTAL ) {
				prepareToStop();
				break;
			}
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			if( m_fileIDs.size() != 1) {
				throw wxString(wxT("Kademlia.CSearch.processResponse: m_fileIDs.size() != 1"));
			}
			bio.WriteUInt128(m_fileIDs.front());
			bio.WriteUInt16(thePrefs::GetPort());
			CKademlia::getUDPListener()->sendPacket( &bio, KADEMLIA_CALLBACK_REQ, from->getIPAddress(), from->getUDPPort());
			m_answers++;
			break;
		}
		case NODECOMPLETE:
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: NodeComplete"));
			break;
		case NODE:
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Node"));
			break;
		default:
			AddDebugLogLineM(false, logKadSearch, wxString::Format(wxT("Search result type: Unknown (%i)"),m_type));
			break;
	}
}

void CSearch::processResult(uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
	wxString type = wxT("Unknown");
	switch(m_type) {
		case FILE:
			type = wxT("File");
			processResultFile(fromIP, fromPort, answer, info);
			break;
		case KEYWORD:
			type = wxT("Keyword");
			processResultKeyword(fromIP, fromPort, answer, info);
			break;
		case NOTES:
			type = wxT("Notes");
			processResultNotes(fromIP, fromPort, answer, info);
			break;
	}
	AddDebugLogLineM(false, logKadSearch, wxT("Got result ") + type + wxT(" from ") + Uint32_16toStringIP_Port(fromIP,fromPort));
}

void CSearch::processResultFile(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagList *info)
{
	uint8  type = 0;
	uint32 ip = 0;
	uint16 tcp = 0;
	uint16 udp = 0;
	uint32 serverip = 0;
	uint16 serverport = 0;
	uint32 clientid = 0;
	CUInt128 buddy;

	CTag *tag;
	TagList::const_iterator it;
	for (it = info->begin(); it != info->end(); ++it) {
		tag = *it;
		if (!tag->m_name.Cmp(wxT(TAG_SOURCETYPE))) {
			type = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_SOURCEIP))) {
			ip = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_SOURCEPORT))) {
			tcp = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_SOURCEUPORT))) {
			udp = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_SERVERIP))) {
			serverip = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_SERVERPORT))) {
			serverport = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_CLIENTLOWID))) {
			clientid	= tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_BUDDYHASH))) {
			CMD4Hash hash(tag->GetStr());
			buddy.setValueBE(hash.GetHash());
		}

		delete tag;
	}
	delete info;

	switch( type ) {
		case 1:
		case 3: {
			m_answers++;
			theApp.downloadqueue->KademliaSearchFile(m_searchID, &answer, &buddy, type, ip, tcp, udp, serverip, serverport, clientid);
			break;
		}
		case 2: {
			//Don't use this type, some clients will process it wrong..
			break;
		}
	}
}

void CSearch::processResultNotes(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagList *info)
{
	CEntry* entry = new CEntry();
	entry->keyID.setValue(m_target);
	entry->sourceID.setValue(answer);
	
	#warning KAD TODO: Filter Kad notes.
	
	CTag *tag;
	TagList::const_iterator it;
	for (it = info->begin(); it != info->end(); it++) {
		tag = *it;
		if (!tag->m_name.Cmp(wxT(TAG_SOURCEIP))) {
			entry->ip = tag->GetInt();
			delete tag;
		} else if (!tag->m_name.Cmp(wxT(TAG_SOURCEPORT))) {
			entry->tcpport = tag->GetInt();
			delete tag;
		} else if (!tag->m_name.Cmp(wxT(TAG_FILENAME))) {
			entry->fileName	= tag->GetStr();
			delete tag;
		} else if (!tag->m_name.Cmp(wxT(TAG_DESCRIPTION))) {
			entry->taglist.push_front(tag);
		} else if (!tag->m_name.Cmp(wxT(TAG_FILERATING))) {
			entry->taglist.push_front(tag);
		} else {
			delete tag;
		}
	}
	delete info;
	byte fileid[16];
	m_target.toByteArray(fileid);
	const CMD4Hash fileHash(fileid);
	
	//Check if this hash is in our shared files..
	CKnownFile* file = theApp.sharedfiles->GetFileByID(fileHash);
	
	if (!file) {
		// If we didn't find anything check if it's in our download queue.
		file = (CKnownFile*)theApp.downloadqueue->GetFileByID(fileHash);
	}
	
	if (file) {
		// Ok, found the file. Add the note to it.
		m_answers++;
		file->AddNote(entry);
	} else {
		AddDebugLogLineM(false, logKadSearch, wxT("Comment received for unknown file"));
	}
}

void CSearch::processResultKeyword(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagList *info)
{
	bool interested = false;
	wxString name;
	uint32 size = 0;
	wxString type;
	wxString format;
	wxString artist;
	wxString album;
	wxString title;
	uint32 length = 0;
	wxString codec;
	uint32 bitrate = 0;
	uint32 availability = 0;

	for (TagList::const_iterator it = info->begin(); it != info->end(); ++it) {
		CTag* tag = *it;

		if (!tag->m_name.Cmp(wxT(TAG_FILENAME))) {
			name	= tag->GetStr();
			interested = !name.IsEmpty();
		} else if (!tag->m_name.Cmp(wxT(TAG_FILESIZE))) {
			size = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_FILETYPE))) {
			type = tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_FILEFORMAT))) {
			format = tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_ARTIST))) {
			artist = tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_ALBUM))) {
			album = tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_TITLE))) {
			title = tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_LENGTH))) {
			length = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_BITRATE))) {
			bitrate = tag->GetInt();
		} else if (!tag->m_name.Cmp(wxT(TAG_MEDIA_CODEC))) {
			codec	= tag->GetStr();
		} else if (!tag->m_name.Cmp(wxT(TAG_SOURCES))) {
			availability = tag->GetInt();
			if( availability > 65500 ) {
				availability = 0;
			}
		}
		delete tag;
	}
	delete info;

	// Check that it matches original criteria
	// generally this is ok, but with the transition of Kad from ACP to Unicode we may receive 'wrong' results in the next months which are
	// actually not 'wrong' (don't ask for a detailed explanation)
	//WordList testWords;
	//CSearchManager::getWords(name.GetBuffer(0), &testWords);
	//
	//WordList::const_iterator mine;
	//WordList::const_iterator test;
	//CStringW keyword;
	//for (mine = m_words.begin(); mine != m_words.end(); mine++)
	//{
	//	keyword = *mine;
	//	interested = false;
	//	for (test = testWords.begin(); test != testWords.end(); test++)
	//	{
	//		if (!keyword.CompareNoCase(*test))
	//		{
	//			interested = true;
	//			break;
	//		}
	//	}
	//	if (!interested)
	//		return;
	//}

	TagPtrList taglist;
	
	if (!format.IsEmpty()) {
		taglist.push_back(new ed2kCTag(TAG_FILEFORMAT, format));
	}
	if (!artist.IsEmpty()) {
		taglist.push_back(new ed2kCTag(TAG_MEDIA_ARTIST, artist));
	}
	if (!album.IsEmpty()) {
		taglist.push_back(new ed2kCTag(TAG_MEDIA_ALBUM, album));
	}
	if (!title.IsEmpty()) {
		taglist.push_back(new ed2kCTag(TAG_MEDIA_TITLE, title));
	}
	if (length) {
		taglist.push_back(new ed2kCTag(TAG_MEDIA_LENGTH, length));
	}
	if (bitrate) {
		taglist.push_back(new ed2kCTag(TAG_MEDIA_BITRATE, bitrate));
	}
	if (availability) {
		taglist.push_back(new ed2kCTag(TAG_SOURCES, availability));
	}

	if (interested) {
		m_answers++;
		theApp.searchlist->KademliaSearchKeyword(m_searchID, &answer, name, size, type, taglist);
	} else {
		printf("Not adding search results: not interested\n");
	}
	
	// Free tags memory
	for (TagPtrList::iterator it = taglist.begin(); it != taglist.end(); ++it) {
		delete (*it);
	}	
	
}

void CSearch::sendFindValue(const CUInt128 &check, uint32 ip, uint16 port)
{
	try {
		if(m_stoping) {
			return;
		}
		CMemFile bio(33);
		switch(m_type){
			case NODE:
			case NODECOMPLETE:
				bio.WriteUInt8(KADEMLIA_FIND_NODE);
				break;
			case FILE:
			case KEYWORD:
			case FINDSOURCE:
			case NOTES:
				bio.WriteUInt8(KADEMLIA_FIND_VALUE);
				break;
			case FINDBUDDY:
			case STOREFILE:
			case STOREKEYWORD:
			case STORENOTES:
				bio.WriteUInt8(KADEMLIA_STORE);
				break;
			default:
				AddDebugLogLineM(false, logKadSearch, wxT("Invalid search type. (CSearch::sendFindValue)"));
				return;
		}
		bio.WriteUInt128(m_target);
		bio.WriteUInt128(check);
		m_kadPacketSent++;
		#ifdef __DEBUG__
		wxString Type;
		switch(m_type) {
			case NODE:
				Type = wxT("KadReqFindNode");
				break;
			case NODECOMPLETE:
				Type = wxT("KadReqFindNodeCompl");
				break;
			case FILE:
				Type = wxT("KadReqFindFile");
				break;
			case KEYWORD:
				Type = wxT("KadReqFindKeyw");
				break;
			case STOREFILE:
				Type = wxT("KadReqStoreFile");
				break;
			case STOREKEYWORD:
				Type = wxT("KadReqStoreKeyw");
				break;
			case STORENOTES:
				Type = wxT("KadReqStoreNotes");
				break;
			case NOTES:
				Type = wxT("KadReqNotes");
				break;
			default:
				Type = wxT("KadReqUnknown");
		}
		AddDebugLogLineM(false, logClientKadUDP, Type + wxT(" to ") + Uint32_16toStringIP_Port(ip,port));
		#endif

		CKademlia::getUDPListener()->sendPacket(&bio, KADEMLIA_REQ, ip, port);
	} catch (const CIOException& ioe) {
		AddDebugLogLineM( false, logKadSearch, wxString::Format(wxT("Exception in CSearch::sendFindValue (IO error(%i))"), ioe.m_cause));
	}
}

void CSearch::addFileID(const CUInt128& id)
{
	m_fileIDs.push_back(id);
}

void CSearch::PreparePacketForTags( CByteIO *bio, CKnownFile *file)
{
	try {
		if( file && bio ) {
			TagList taglist;
			
			// Name, Size
			taglist.push_back(new CTagStr(TAG_FILENAME, file->GetFileName()));
			taglist.push_back(new CTagUInt(TAG_FILESIZE, file->GetFileSize()));
			taglist.push_back(new CTagUInt(TAG_SOURCES, (uint32)file->m_nCompleteSourcesCount));
			
			// eD2K file type (Audio, Video, ...)
			// NOTE: Archives and CD-Images are published with file type "Pro"
			wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName())));
			if (!strED2KFileType.IsEmpty()) {
				taglist.push_back(new CTagStr(TAG_FILETYPE, strED2KFileType));
			}
			
			// file format (filename extension)
			int iExt = file->GetFileName().Find(wxT('.'), true);
			if (iExt != -1) {
				wxString strExt(file->GetFileName().Mid(iExt));
				if (!strExt.IsEmpty()) {
					strExt = strExt.Mid(1);
					if (!strExt.IsEmpty()) {
						taglist.push_back(new CTagStr(TAG_FILEFORMAT, strExt));
					}
				}
			}

			// additional meta data (Artist, Album, Codec, Length, ...)
			// only send verified meta data to nodes
			if (file->GetMetaDataVer() > 0) {
				static const struct{
					uint8	nName;
					uint8	nType;
				} _aMetaTags[] = 
				{
					{ FT_MEDIA_ARTIST,  2 },
					{ FT_MEDIA_ALBUM,   2 },
					{ FT_MEDIA_TITLE,   2 },
					{ FT_MEDIA_LENGTH,  3 },
					{ FT_MEDIA_BITRATE, 3 },
					{ FT_MEDIA_CODEC,   2 }
				};
				for (int i = 0; i < ARRSIZE(_aMetaTags); i++) {
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
						wxString szKadTagName = wxString::Format(wxT("%c"),pTag->GetNameID());					
						if (pTag->IsStr()) {
							taglist.push_back(new CTagStr(szKadTagName, pTag->GetStr()));
						} else {
							taglist.push_back(new CTagUInt(szKadTagName, pTag->GetInt()));
						}
					}
				}
			}
			bio->writeTagList(taglist);
			TagList::const_iterator it;
			for (it = taglist.begin(); it != taglist.end(); ++it) {
				delete *it;
			}
		} else {
			//If we get here.. Bad things happen.. Will fix this later if it is a real issue.
			wxASSERT(0);
		}
	} catch (const CIOException& ioe) {
		AddDebugLogLineM( false, logKadSearch, wxString::Format(wxT("Exception in CSearch::PreparePacketForTags (IO error(%i))"), ioe.m_cause));
	}
}

//Can't clean these up until Taglist works with CMemFiles.
void CSearch::PreparePacket(void)
{
	try {
		int count = m_fileIDs.size();
		byte fileid[16];
		CKnownFile* file = NULL;
		if( count > 150 ) {
			count = 150;
		}
		if( count > 100 ) {
			bio3 = new CByteIO(packet3,sizeof(packet3));
			bio3->writeByte(OP_KADEMLIAHEADER);
			bio3->writeByte(KADEMLIA_PUBLISH_REQ);
			bio3->writeUInt128(m_target);
			bio3->writeUInt16(count-100);
			while ( count > 100 ) {
				count--;
				bio3->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toByteArray(fileid);
				m_fileIDs.pop_front();
				file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
				if (!file) {
					printf("File not found on shared when publishing packet!\n");
				}
				PreparePacketForTags( bio3, file );
			}
		}
		if( count > 50 ) {
			bio2 = new CByteIO(packet2,sizeof(packet2));
			bio2->writeByte(OP_KADEMLIAHEADER);
			bio2->writeByte(KADEMLIA_PUBLISH_REQ);
			bio2->writeUInt128(m_target);
			bio2->writeUInt16(count-50);
			while ( count > 50 ) {
				count--;
				bio2->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toByteArray(fileid);
				m_fileIDs.pop_front();
				file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
				if (!file) {
					printf("File not found on shared when publishing packet!\n");
				}
				PreparePacketForTags( bio2, file );
			}
		}
		if( count > 0 ) {
			bio1 = new CByteIO(packet1,sizeof(packet1));
			bio1->writeByte(OP_KADEMLIAHEADER);
			bio1->writeByte(KADEMLIA_PUBLISH_REQ);
			bio1->writeUInt128(m_target);
			bio1->writeUInt16(count);
			while ( count > 0 ) {
				count--;
				bio1->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toByteArray(fileid);
				m_fileIDs.pop_front();
				file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
				if (!file) {
					printf("File not found on shared when publishing packet!\n");
				}
				PreparePacketForTags( bio1, file );
			}
		}
	} catch (const CIOException& ioe ) {
		AddDebugLogLineM( false, logKadSearch, wxString::Format(wxT("Exception in CSearch::PreparePacket (IO error(%i))"), ioe.m_cause));
	}
}

uint32 CSearch::getNodeLoad() const
{
	if( m_totalLoadResponses == 0 ) {
		return 0;
	}
	return m_totalLoad/m_totalLoadResponses;
}
