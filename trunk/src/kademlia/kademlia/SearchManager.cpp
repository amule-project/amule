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

//#include "stdafx.h"
//#include "resource.h"
#include "SearchManager.h"
#include "Search.h"
#include "Kademlia.h"
#include "Indexed.h"
#include "../../OPCodes.h"
#include "Defines.h"
#include "Tag.h"
#include "../routing/Contact.h"
#include "../utils/UInt128.h"
#include "../io/ByteIO.h"
#include "../io/IOException.h"
#include "../kademlia/Prefs.h"
#include "MemFile.h"
#include "OtherFunctions.h"
#include "Logger.h"

#include <wx/tokenzr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


wxChar* InvKadKeywordChars = wxT(" ()[]{}<>,._-!?:;\\/");

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

uint32 CSearchManager::m_nextID = 0;
SearchMap CSearchManager::m_searches;

void CSearchManager::stopSearch(uint32 searchID, bool delayDelete)
{
	if(searchID == (uint32)-1) {
		return;
	}
	
	SearchMap::iterator it = m_searches.begin();
	while ( it != m_searches.end()) {
		if (it->second->m_searchID == searchID) {
			if(delayDelete) {
				it->second->prepareToStop();
				++it;
			} else {
				delete it->second;
				m_searches.erase(it++);
			}
		} else {
			++it;
		}
	}
}

bool CSearchManager::isNodeSearch(const CUInt128 &target)
{
	SearchMap::iterator it = m_searches.begin(); 
	while (it != m_searches.end()) {
		if (it->second->m_target == target) {
			return 	it->second->m_type == CSearch::NODE ||
				it->second->m_type == CSearch::NODECOMPLETE;
		} else {
			++it;
		}
	}
	
	return false;
}

void CSearchManager::stopAllSearches(void)
{
	SearchMap::iterator it;
	for (it = m_searches.begin(); it != m_searches.end(); ++it) {
		delete it->second;
	}
	
	m_searches.clear();
}

bool CSearchManager::startSearch(CSearch* pSearch)
{
	if (alreadySearchingFor(pSearch->m_target)) {
		delete pSearch;
		return false;
	}
	m_searches[pSearch->m_target] = pSearch;
	pSearch->go();
	return true;
}

void CSearchManager::deleteSearch(CSearch* pSearch)
{
	delete pSearch;
}

CSearch* CSearchManager::prepareFindKeywords(const wxString& keyword, CMemFile* ed2k_packet, uint32 searchid)
{
	CSearch *s = new CSearch;
	try {
		s->m_type = CSearch::KEYWORD;

		// This will actually get the first word.
		getWords(keyword, &s->m_words);
		if (s->m_words.size() == 0) {
			throw wxString(_("Kademlia: search keyword too short"));
		}

		wxString wstrKeyword = s->m_words.front();
		
		printf("Keyword for search: %s\n",(const char*)unicode2char(wstrKeyword));
		
		// Kry - I just decided to assume everyone is unicoded
		KadGetKeywordHash(wstrKeyword, &s->m_target);
				
		if (alreadySearchingFor(s->m_target)) {
			throw wxT("Kademlia: Search keyword is already on search list: ") + wstrKeyword;
		}

		// Write complete packet
		s->m_searchTerms = ed2k_packet;
		s->m_searchTerms->Seek(0,wxFromStart);
		s->m_searchTerms->WriteUInt128(s->m_target);
		if (s->m_searchTerms->GetLength() > (16 /*uint128*/ + 1 /*uint8*/)) { 
			// There is actually ed2k search data
			s->m_searchTerms->WriteUInt8(1);
		} // 0 is default, no need for else branch
	
		// if called from external client - use predefined search id
		s->m_searchID = ((searchid & 0xffffff00) == 0xffffff00) ? searchid : ++m_nextID;
		
		m_searches[s->m_target] = s;
		s->go();
	} catch (const CIOException& ioe) {
		wxString strError = wxString::Format(wxT("IO-Exception in %s: Error %u") , __FUNCTION__, ioe.m_cause);
		delete s;
		throw strError;
	} catch (...) {
		delete s;
		throw;
	}
	return s;
}

CSearch* CSearchManager::prepareLookup(uint32 type, bool start, const CUInt128 &id)
{
	
	if(alreadySearchingFor(id)) {
		return NULL;
	}

	CSearch *s = new CSearch;
	try {
		switch(type) {
			case CSearch::STOREKEYWORD:
				if(!Kademlia::CKademlia::getIndexed()->SendStoreRequest(id)) {
					delete s;
					return NULL;
				}
				break;
		}

		s->m_type = type;
		s->m_target = id;

		// Write complete packet
		s->m_searchTerms = new CMemFile();
		s->m_searchTerms->WriteUInt128(s->m_target);
		s->m_searchTerms->WriteUInt8(1);

		s->m_searchID = ++m_nextID;
		if( start ) {
			m_searches[s->m_target] = s;
			s->go();
		}
	} catch (const CIOException& ioe) {
		AddDebugLogLineM( false, logKadSearch,
			wxString::Format(wxT("Exception in CSearchManager::prepareLookup (IO error(%i))"),
				ioe.m_cause));
		delete s;
		return NULL;
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch,
			wxT("Exception in CSearchManager::prepareLookup"));
		delete s;
		throw;
	}

	return s;
}

void CSearchManager::findNode(const CUInt128 &id)
{
	if (alreadySearchingFor(id)) {
		return;
	}

	CSearch *s = new CSearch;
	s->m_type = CSearch::NODE;
	s->m_target = id;
	s->m_searchTerms = NULL;
	m_searches[s->m_target] = s;
	s->go();
}

void CSearchManager::findNodeComplete(const CUInt128 &id)
{
	if (alreadySearchingFor(id)) {
		return;
	}

	CSearch *s = new CSearch;
	s->m_type = CSearch::NODECOMPLETE;
	s->m_target = id;
	s->m_searchTerms = NULL;
	m_searches[s->m_target] = s;
	s->go();
}

bool CSearchManager::alreadySearchingFor(const CUInt128 &target)
{
	return m_searches.count(target);
}

void CSearchManager::getWords(const wxString& str, WordList *words)
{
	int len = 0;
	wxString current_word;
	wxStringTokenizer tkz(str, InvKadKeywordChars);
	while (tkz.HasMoreTokens()) {
		current_word = tkz.GetNextToken();
		
		if ((len = current_word.Length()) > 2) {
			KadTagStrMakeLower(current_word);
			words->remove(current_word);
			words->push_back(current_word);
		}
	}
	// If the last word is 3 bytes long, chances are it's a file extension.
	if(words->size() > 1 && len == 3) {
		words->pop_back();
	}
}

void CSearchManager::jumpStart(void)
{
	time_t now = time(NULL);
	SearchMap::iterator next_it = m_searches.begin();
	while (next_it != m_searches.end()) {
		SearchMap::iterator current_it = next_it++; /* don't change this to a ++next_it! */
		switch(current_it->second->getSearchTypes()){
			case CSearch::FILE: {
				if (current_it->second->m_created + SEARCHFILE_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHFILE_TOTAL ||
						current_it->second->m_created + SEARCHFILE_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}					
				break;
			}
			case CSearch::KEYWORD: {
				if (current_it->second->m_created + SEARCHKEYWORD_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHKEYWORD_TOTAL ||
						current_it->second->m_created + SEARCHKEYWORD_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::NOTES: {
				if (current_it->second->m_created + SEARCHNOTES_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHNOTES_TOTAL ||
						current_it->second->m_created + SEARCHNOTES_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::FINDBUDDY: {
				if (current_it->second->m_created + SEARCHFINDBUDDY_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHFINDBUDDY_TOTAL ||
						current_it->second->m_created + SEARCHFINDBUDDY_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::FINDSOURCE: {
				if (current_it->second->m_created + SEARCHFINDSOURCE_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHFINDSOURCE_TOTAL ||
						current_it->second->m_created + SEARCHFINDSOURCE_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::NODE: {
				if (current_it->second->m_created + SEARCHNODE_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::NODECOMPLETE: {
				if (current_it->second->m_created + SEARCHNODE_LIFETIME < now) {
					CKademlia::getPrefs()->setPublish(true);
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	(current_it->second->m_created + SEARCHNODECOMP_LIFETIME < now) &&
						(current_it->second->getAnswers() > SEARCHNODECOMP_TOTAL)) {
					CKademlia::getPrefs()->setPublish(true);
					delete current_it->second;
					m_searches.erase(current_it);
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::STOREFILE: {
				if (current_it->second->m_created + SEARCHSTOREFILE_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHSTOREFILE_TOTAL ||
						current_it->second->m_created + SEARCHSTOREFILE_LIFETIME - SEC(20) < now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::STOREKEYWORD: {
				if (current_it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHSTOREKEYWORD_TOTAL ||
						current_it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME - SEC(20)< now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			case CSearch::STORENOTES: {
				if (current_it->second->m_created + SEARCHSTORENOTES_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else if (	current_it->second->getAnswers() > SEARCHSTORENOTES_TOTAL ||
						current_it->second->m_created + SEARCHSTORENOTES_LIFETIME - SEC(20)< now) {
					current_it->second->prepareToStop();
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
			default: {
				if (current_it->second->m_created + SEARCH_LIFETIME < now) {
					delete current_it->second;
					m_searches.erase(current_it);
				} else {
					current_it->second->jumpStart();
				}
				break;
			}
		}
	}
}

void CSearchManager::updateStats(void)
{
	uint8 m_totalFile = 0;
	uint8 m_totalStoreSrc = 0;
	uint8 m_totalStoreKey = 0;
	uint8 m_totalSource = 0;
	uint8 m_totalNotes = 0;
	uint8 m_totalStoreNotes = 0;
		
	for (SearchMap::iterator it = m_searches.begin(); it != m_searches.end(); ++it) {
		switch(it->second->getSearchTypes()){
			case CSearch::FILE: {
				m_totalFile++;
				break;
			}
			case CSearch::STOREFILE: {
				m_totalStoreSrc++;
				break;
			}
			case CSearch::STOREKEYWORD:	{
				m_totalStoreKey++;
				break;
			}
			case CSearch::FINDSOURCE: {
				m_totalSource++;
				break;
			}
			case CSearch::STORENOTES: {
				m_totalStoreNotes++;
				break;
			}
			case CSearch::NOTES: {
				m_totalNotes++;
				break;
			}
			default:
				break;
		}
	}
	
	CPrefs *prefs = CKademlia::getPrefs();
	prefs->setTotalFile(m_totalFile);
	prefs->setTotalStoreSrc(m_totalStoreSrc);
	prefs->setTotalStoreKey(m_totalStoreKey);
	prefs->setTotalSource(m_totalSource);
	prefs->setTotalNotes(m_totalNotes);
	prefs->setTotalStoreNotes(m_totalStoreNotes);
}

void CSearchManager::processPublishResult(const CUInt128 &target, const uint8 load, const bool loadResponse)
{
	CSearch *s = NULL;
	SearchMap::const_iterator it = m_searches.find(target);
	if (it != m_searches.end()) {
		s = it->second;
	}

	if (s == NULL) {
//		AddDebugLogLineM(false, logKadSearch,
//			wxT("Search either never existed or receiving late results (CSearchManager::processPublishResults)"));
		return;
	}
	
	s->m_answers++;
	if( loadResponse ) {
		s->updateNodeLoad( load );
	}
}


void CSearchManager::processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results)
{
	CSearch *s = NULL;
	SearchMap::const_iterator it = m_searches.find(target);
	if (it != m_searches.end()) {
		s = it->second;
	}

	if (s == NULL) {
		AddDebugLogLineM(false, logKadSearch,
			wxT("Search either never existed or receiving late results (CSearchManager::processResponse)"));
		ContactList::const_iterator it2;
		for (it2 = results->begin(); it2 != results->end(); ++it2) {
			delete (*it2);
		}
		delete results;
		return;
	} else {
		s->processResponse(fromIP, fromPort, results);
	}
}

void CSearchManager::processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
	CSearch *s = NULL;
	SearchMap::const_iterator it = m_searches.find(target);
	if (it != m_searches.end()) {
		s = it->second;
	}

	if (s == NULL) {
		AddDebugLogLineM (false, logKadSearch,
			wxT("Search either never existed or receiving late results (CSearchManager::processResult)"));
		for (TagList::const_iterator tagIt = info->begin(); tagIt != info->end(); tagIt++) {
			delete *tagIt;
		}
		delete info;
	} else {
		s->processResult(fromIP, fromPort, answer, info);
	}
}
