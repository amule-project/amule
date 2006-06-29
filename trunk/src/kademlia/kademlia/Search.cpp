//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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



#include "Search.h"

#include <include/protocol/Protocols.h>
#include <include/protocol/kad/Client2Client/UDP.h>
#include <include/protocol/kad/Constants.h>
#include <include/protocol/kad2/Client2Client/UDP.h>
#include <include/tags/FileTags.h>

#include "Defines.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
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
	m_uAnswers = 0;
	m_totalRequestAnswers = 0;
	m_kadPacketSent = 0;
	m_searchID = (uint32)-1;
	m_keywordPublish = 0;
	(void)m_fileName;
	m_stoping = false;
	m_totalLoad = 0;
	m_totalLoadResponses = 0;
	m_lastResponse = time(NULL);
}

CSearch::~CSearch()
{
	
	CPartFile* temp = theApp.downloadqueue->GetFileByKadFileSearchID(GetSearchID());
	
	if(temp) {
		temp->SetKadFileSearchID(0);
	}	
	
	delete m_searchTerms;

	ContactMap::iterator it;
	for (it = m_inUse.begin(); it != m_inUse.end(); it++) {
		((CContact*)it->second)->DecUse();
	}

	ContactList::const_iterator it2;
	for (it2 = m_delete.begin(); it2 != m_delete.end(); it2++) {
		delete *it2;
	}
	
	if(CKademlia::IsRunning() && GetNodeLoad() > 20) {
		switch(GetSearchTypes()) {
			case CSearch::STOREKEYWORD:
				Kademlia::CKademlia::GetIndexed()->AddLoad(GetTarget(), ((uint32)(DAY2S(7)*((double)GetNodeLoad()/100.0))+(uint32)time(NULL)));
				break;
			default:
				;
				// WTF? 
		}
	}
	
	switch (m_type) {
		case KEYWORD:
			Notify_KadSearchEnd(m_searchID);
			break;
		default:
			;
			// WTF?
	}
}

void CSearch::Go(void)
{
	// Start with a lot of possible contacts, this is a fallback in case search stalls due to dead contacts
	if (m_possible.empty()) {
		CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
		distance.XOR(m_target);
		CKademlia::GetRoutingZone()->GetClosestTo(3, m_target, distance, 50, &m_possible, true, true);
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
		c->CheckingType();
		SendFindValue(c->GetClientID(), c->GetIPAddress(), c->GetUDPPort());
		if(m_type == NODE) {
			break;
		}
	}
}

//If we allow about a 15 sec delay before deleting, we won't miss a lot of delayed returning packets.
void CSearch::PrepareToStop()
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

void CSearch::JumpStart(void)
{
	if (m_possible.empty()) {
		PrepareToStop();
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
			c->CheckingType();
			SendFindValue(c->GetClientID(), c->GetIPAddress(), c->GetUDPPort());
			break;
		}
	}
	
}

void CSearch::ProcessResponse(uint32 fromIP, uint16 fromPort, ContactList *results)
{
	AddDebugLogLineM(false, logKadSearch, wxT("Process search response from ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(fromIP), fromPort));

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
		m_uAnswers++;
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

		if ((from->GetIPAddress() == fromIP) && (from->GetUDPPort() == fromPort)) {
			// Add to list of people who responded
			m_responded[fromDistance] = from;

			// Loop through their responses
			for (response = results->begin(); response != results->end(); ++response) {
				c = *response;

				distance = c->GetClientID();
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
						c->CheckingType();
						SendFindValue(c->GetClientID(), c->GetIPAddress(), c->GetUDPPort());
					}
				}
			}

			// We don't want anything from these people, so just increment the counter.
			if( m_type == NODECOMPLETE ) {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Node complete"));
				m_uAnswers++;
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

	if(thePrefs::FilterLanIPs() && fromDistance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		AddDebugLogLineM(false, logKadSearch, wxT("Not stored: filtered lan ip"));
		return;
	}

	switch(m_type) {
		case FILE:
		case KEYWORD: {
			if (m_type == FILE) {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: File"));
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchReq (File) ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(from->GetIPAddress()), from->GetUDPPort()));
			} else {
				AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Keyword"));
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchReq (Keyword) ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(from->GetIPAddress()), from->GetUDPPort()));
			}
			wxASSERT( m_searchTerms->GetLength() > 0 );
			// The data in 'm_searchTerms' is to be sent several times, so use the don't detach flag.
			CKademlia::GetUDPListener()->SendPacket(m_searchTerms, KADEMLIA_SEARCH_REQ, from->GetIPAddress(), from->GetUDPPort());
			m_totalRequestAnswers++;
			break;
		}
		case NOTES: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: Notes"));
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			bio.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
			CKademlia::GetUDPListener()->SendPacket( &bio, KADEMLIA_SRC_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort());
			m_totalRequestAnswers++;
			break;
		}
		case STOREFILE: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreFile"));
			if( m_uAnswers > SEARCHSTOREFILE_TOTAL ) {
				PrepareToStop();
				break;
			}
			byte fileid[16];
			m_target.ToByteArray(fileid);
			CKnownFile* file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
			if (file) {
				m_fileName = file->GetFileName();

				CUInt128 id(CKademlia::GetPrefs()->GetClientHash());
				TagPtrList taglist;

				//We can use type for different types of sources. 
				//1 is reserved for highID sources..
				//2 cannot be used as older clients will not work.
				//3 Firewalled Kad Source.
				//4 >4GB file HighID Source.
				//5 >4GB file Firewalled Kad source.
				
				if( theApp.IsFirewalled() ) {
					if( theApp.clientlist->GetBuddy() ) {
						CUInt128 buddyID(true);
						buddyID.XOR(CKademlia::GetPrefs()->GetKadID());
						taglist.push_back(new CTagInt8(TAG_SOURCETYPE, file->IsLargeFile() ? 5 : 3));
						taglist.push_back(new CTagVarInt(TAG_SERVERIP, theApp.clientlist->GetBuddy()->GetIP()));
						taglist.push_back(new CTagVarInt(TAG_SERVERPORT, theApp.clientlist->GetBuddy()->GetUDPPort()));
						byte hashBytes[16];
						buddyID.ToByteArray(hashBytes);
						taglist.push_back(new CTagString(TAG_BUDDYHASH, CMD4Hash(hashBytes).Encode()));
						taglist.push_back(new CTagVarInt(TAG_SOURCEPORT, thePrefs::GetPort()));
					} else {
						PrepareToStop();
						break;
					}
				} else {
					taglist.push_back(new CTagInt8(TAG_SOURCETYPE, file->IsLargeFile() ? 4 : 1));
					taglist.push_back(new CTagVarInt(TAG_SOURCEPORT, thePrefs::GetPort()));
				}

				CKademlia::GetUDPListener()->PublishPacket(from->GetIPAddress(), from->GetUDPPort(),m_target,id, taglist);
				m_totalRequestAnswers++;
				TagPtrList::const_iterator it;
				for (it = taglist.begin(); it != taglist.end(); ++it) {
					delete *it;
				}
			}
			break;
		}
		case STOREKEYWORD: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreKeyword"));

			uint16 iCount = (m_fileIDs.size() > 150) ? 150 : m_fileIDs.size();

			if( (m_uAnswers > SEARCHSTOREKEYWORD_TOTAL) || (iCount == 0) ) {
				PrepareToStop();
				break;
			}

			UIntList::const_iterator itListFileID = m_fileIDs.begin();

			while (iCount) {
				uint16 iPacketCount = (iCount > 50) ? 50 : iCount;
				CMemFile packetdata(1024*iPacketCount); // Allocate a good amount of space.
				packetdata.WriteUInt128(m_target);
				packetdata.WriteUInt16(iPacketCount);
				while (iPacketCount) {
					iCount--;
					iPacketCount--;
					CKnownFile* pFile = theApp.sharedfiles->GetFileByID(CMD4Hash(*itListFileID));
					if (pFile) {
						packetdata.WriteUInt128(*itListFileID);
						PreparePacketForTags( &packetdata, pFile );
					}
					++itListFileID;
				}
				// Send packet
				if (from->GetVersion() > 1) {
					AddDebugLogLineM(false, logClientKadUDP, wxT("Kad2StoreKeywReq ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(from->GetIPAddress()), from->GetUDPPort()));
					CKademlia::GetUDPListener()->SendPacket( &packetdata, KADEMLIA2_PUBLISH_KEY_REQ, from->GetIPAddress(), from->GetUDPPort());
				} else {
					AddDebugLogLineM(false, logClientKadUDP, wxT("KadStoreKeywReq ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(from->GetIPAddress()), from->GetUDPPort()));
					CKademlia::GetUDPListener()->SendPacket( &packetdata, KADEMLIA_PUBLISH_REQ, from->GetIPAddress(), from->GetUDPPort());
				}
			}
			
			m_totalRequestAnswers++;
			break;
		}
		case STORENOTES: {
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: StoreNotes"));
			byte fileid[16];
			m_target.ToByteArray(fileid);
			CKnownFile* file = theApp.sharedfiles->GetFileByID(CMD4Hash(fileid));
			if (file) {
				byte packet[1024*2];
				CMemFile bio(packet,sizeof(packet));
				bio.WriteUInt128(m_target);
				bio.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
				uint8 tagcount = 1;
				if(file->GetFileRating() != 0) {
					++tagcount;
				}
				if(!file->GetFileComment().IsEmpty()) {
					++tagcount;
				}
				//Number of tags.
				bio.WriteUInt8(tagcount);
				CTagString fileName(TAG_FILENAME, file->GetFileName());
				bio.WriteTag(fileName);
				if(file->GetFileRating() != 0) {
					CTagVarInt rating(TAG_FILERATING, file->GetFileRating());
					bio.WriteTag(rating);
				}
				if(!file->GetFileComment().IsEmpty()) {
					CTagString description(TAG_DESCRIPTION, file->GetFileComment());
					bio.WriteTag(description);
				}

				CKademlia::GetUDPListener()->SendPacket( packet, sizeof(packet)-bio.GetAvailable(), KADEMLIA_PUB_NOTES_REQ, from->GetIPAddress(), from->GetUDPPort());
				m_totalRequestAnswers++;
			}
			break;
		}
		case FINDBUDDY:
		{
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: FindBuddy"));
			if( m_uAnswers > SEARCHFINDBUDDY_TOTAL ) {
				PrepareToStop();
				break;
			}
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			bio.WriteUInt128(CKademlia::GetPrefs()->GetClientHash());
			bio.WriteUInt16(thePrefs::GetPort());
			CKademlia::GetUDPListener()->SendPacket( &bio, KADEMLIA_FINDBUDDY_REQ, from->GetIPAddress(), from->GetUDPPort());
			m_uAnswers++;
			break;
		}
		case FINDSOURCE:
		{
			AddDebugLogLineM(false, logKadSearch, wxT("Search result type: FindSource"));
			if( m_uAnswers > SEARCHFINDSOURCE_TOTAL ) {
				PrepareToStop();
				break;
			}
			CMemFile bio(34);
			bio.WriteUInt128(m_target);
			if( m_fileIDs.size() != 1) {
				throw wxString(wxT("Kademlia.CSearch.processResponse: m_fileIDs.size() != 1"));
			}
			bio.WriteUInt128(m_fileIDs.front());
			bio.WriteUInt16(thePrefs::GetPort());
			CKademlia::GetUDPListener()->SendPacket( &bio, KADEMLIA_CALLBACK_REQ, from->GetIPAddress(), from->GetUDPPort());
			m_uAnswers++;
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

void CSearch::ProcessResult(uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagPtrList *info)
{
	wxString type = wxT("Unknown");
	switch(m_type) {
		case FILE:
			type = wxT("File");
			ProcessResultFile(fromIP, fromPort, answer, info);
			break;
		case KEYWORD:
			type = wxT("Keyword");
			ProcessResultKeyword(fromIP, fromPort, answer, info);
			break;
		case NOTES:
			type = wxT("Notes");
			ProcessResultNotes(fromIP, fromPort, answer, info);
			break;
	}
	AddDebugLogLineM(false, logKadSearch, wxT("Got result ") + type + wxT(" from ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(fromIP),fromPort));
}

void CSearch::ProcessResultFile(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagPtrList *info)
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
	TagPtrList::const_iterator it;
	for (it = info->begin(); it != info->end(); ++it) {
		tag = *it;
		if (!tag->GetName().Cmp(TAG_SOURCETYPE)) {
			type = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEIP)) {
			ip = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
			tcp = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SOURCEUPORT)) {
			udp = tag->GetInt();
		} else if (!tag->GetName().Cmp((TAG_SERVERIP))) {
			serverip = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_SERVERPORT)) {
			serverport = tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_CLIENTLOWID)) {
			clientid	= tag->GetInt();
		} else if (!tag->GetName().Cmp(TAG_BUDDYHASH)) {
			CMD4Hash hash;
			// TODO: Error handling
			if (not hash.Decode(tag->GetStr())) {
#ifdef __DEBUG__
				printf("Invalid buddy-hash: '%s'\n", (const char*)tag->GetStr().fn_str());
#endif
			}
			
			buddy.SetValueBE(hash.GetHash());
		}

		delete tag;
	}
	delete info;

	switch( type ) {
		case 1:
		case 3: {
			m_uAnswers++;
			theApp.downloadqueue->KademliaSearchFile(m_searchID, &answer, &buddy, type, ip, tcp, udp, serverip, serverport, clientid);
			break;
		}
		case 2: {
			//Don't use this type, some clients will process it wrong..
			break;
		}
	}
}

void CSearch::ProcessResultNotes(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagPtrList *info)
{
	CEntry* entry = new CEntry();
	entry->m_iKeyID.SetValue(m_target);
	entry->m_iSourceID.SetValue(answer);
	
	bool bFilterComment = false;
	
	CTag *tag;
	TagPtrList::const_iterator it;
	for (it = info->begin(); it != info->end(); it++) {
		tag = *it;
		if (!tag->GetName().Cmp(TAG_SOURCEIP)) {
			entry->m_iIP = tag->GetInt();
			delete tag;
		} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
			entry->m_iTCPport = tag->GetInt();
			delete tag;
		} else if (!tag->GetName().Cmp(TAG_FILENAME)) {
			entry->m_sFileName = tag->GetStr();
			delete tag;
		} else if (!tag->GetName().Cmp(TAG_DESCRIPTION)) {
			wxString strComment(tag->GetStr());
			bFilterComment = thePrefs::IsMessageFiltered(strComment);
			entry->m_lTagList.push_front(tag);
		} else if (!tag->GetName().Cmp(TAG_FILERATING)) {
			entry->m_lTagList.push_front(tag);
		} else {
			delete tag;
		}
	}
	delete info;

	if (bFilterComment) {
		delete entry;
		return;
	}
	
	byte fileid[16];
	m_target.ToByteArray(fileid);
	const CMD4Hash fileHash(fileid);
	
	//Check if this hash is in our shared files..
	CKnownFile* file = theApp.sharedfiles->GetFileByID(fileHash);
	
	if (!file) {
		// If we didn't find anything check if it's in our download queue.
		file = (CKnownFile*)theApp.downloadqueue->GetFileByID(fileHash);
	}
	
	if (file) {
		// Ok, found the file. Add the note to it.
		m_uAnswers++;
		file->AddNote(entry);
	} else {
		AddDebugLogLineM(false, logKadSearch, wxT("Comment received for unknown file"));
	}
}

void CSearch::ProcessResultKeyword(uint32 WXUNUSED(fromIP), uint16 WXUNUSED(fromPort), const CUInt128 &answer, TagPtrList *info)
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

	for (TagPtrList::const_iterator it = info->begin(); it != info->end(); ++it) {
		CTag* tag = *it;
		if (tag->GetName() == TAG_FILENAME) {
			name	= tag->GetStr();
			interested = !name.IsEmpty();
		} else if (tag->GetName() == TAG_FILESIZE) {
			if (tag->IsBsob() && (tag->GetBsobSize() == 8)) {
				// Kad1.0 uint64 type using a BSOB.
				size = PeekUInt64(tag->GetBsob());
			} else {
				wxASSERT(tag->IsInt());
				size = tag->GetInt();	
			}
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
			codec	= tag->GetStr();
		} else if (tag->GetName() == TAG_SOURCES) {
			availability = tag->GetInt();
			if( availability > 65500 ) {
				availability = 0;
			}
		}
		delete tag;
	}
	delete info;
	
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

	if (interested) {
		m_uAnswers++;
		theApp.searchlist->KademliaSearchKeyword(m_searchID, &answer, name, size, type, taglist);
	} else {
		printf("Not adding search results: not interested\n");
	}
	
	// Free tags memory
	for (TagPtrList::iterator it = taglist.begin(); it != taglist.end(); ++it) {
		delete (*it);
	}	
	
}

void CSearch::SendFindValue(const CUInt128 &check, uint32 ip, uint16 port)
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
		AddDebugLogLineM(false, logClientKadUDP, Type + wxT(" to ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
		#endif

		CKademlia::GetUDPListener()->SendPacket(&bio, KADEMLIA_REQ, ip, port);
	} catch (const CEOFException& err) {
		AddDebugLogLineM(true, logKadIndex, wxT("CEOFException in CIndexed::sendFindValue: ") + err.what());
	} catch (const CInvalidPacket& err) {
		AddDebugLogLineM(true, logKadIndex, wxT("CInvalidPacket Exception in CIndexed::sendFindValue: ") + err.what());		
	} catch (const wxString& e) {
		AddDebugLogLineM(true, logKadIndex, wxT("Exception in CIndexed::sendFindValue: ") + e);
	}
}

void CSearch::AddFileID(const CUInt128& id)
{
	m_fileIDs.push_back(id);
}

void CSearch::PreparePacketForTags( CMemFile *bio, CKnownFile *file)
{
	TagPtrList taglist;
	
	try {
		if( file && bio ) {		
			// Name, Size
			taglist.push_back(new CTagString(TAG_FILENAME, file->GetFileName()));
			if (file->IsLargeFile()) {
				byte size64[sizeof(uint64)];
				PokeUInt64(size64,file->GetFileSize());
				taglist.push_back(new CTagBsob(TAG_FILESIZE, size64, sizeof(uint64)));	
			} else {
				taglist.push_back(new CTagVarInt(TAG_FILESIZE, file->GetFileSize()));
			}
			taglist.push_back(new CTagVarInt(TAG_SOURCES, (uint32)file->m_nCompleteSourcesCount));
			
			// eD2K file type (Audio, Video, ...)
			// NOTE: Archives and CD-Images are published with file type "Pro"
			wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName())));
			if (!strED2KFileType.IsEmpty()) {
				taglist.push_back(new CTagString(TAG_FILETYPE, strED2KFileType));
			}
			
			// file format (filename extension)
			int iExt = file->GetFileName().Find(wxT('.'), true);
			if (iExt != -1) {
				wxString strExt(file->GetFileName().Mid(iExt));
				if (!strExt.IsEmpty()) {
					strExt = strExt.Mid(1);
					if (!strExt.IsEmpty()) {
						taglist.push_back(new CTagString(TAG_FILEFORMAT, strExt));
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
						wxString szKadTagName = wxString::Format(wxT("%c"),pTag->GetNameID());					
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
			wxASSERT(0);
		}
	} catch (const CEOFException& err) {
		AddDebugLogLineM(true, logKadIndex, wxT("CEOFException in CIndexed::PreparePacketForTags: ") + err.what());
	} catch (const CInvalidPacket& err) {
		AddDebugLogLineM(true, logKadIndex, wxT("CInvalidPacket Exception in CIndexed::PreparePacketForTags: ") + err.what());		
	} catch (const wxString& e) {
		AddDebugLogLineM(true, logKadIndex, wxT("Exception in CIndexed::PreparePacketForTags: ") + e);
	} 
	
	for (TagPtrList::const_iterator it = taglist.begin(); it != taglist.end(); ++it) {
		delete *it;
	}
}

uint32 CSearch::GetNodeLoad() const
{
	if( m_totalLoadResponses == 0 ) {
		return 0;
	}
	return m_totalLoad/m_totalLoadResponses;
}

uint32 CSearch::GetAnswers() const {
	// Each 50 files make for a response packet, so we have to convert this from packet responses to full store responses.
	return (m_fileIDs.size() ? m_uAnswers/((m_fileIDs.size()+49)/50) : m_uAnswers);
}

// File_checked_for_headers
