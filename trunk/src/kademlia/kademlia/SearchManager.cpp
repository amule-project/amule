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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
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
#include "SafeFile.h"
#include "OtherFunctions.h"
#include "Logger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


wxChar* InvKadKeywordChars = wxT(" ()[]{}<>,._-!?:\\/");

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

uint32 CSearchManager::m_nextID = 0;
SearchMap CSearchManager::m_searches;

void CSearchManager::stopSearch(uint32 searchID, bool delayDelete)
{
	try {
		if(searchID == (uint32)-1) {
			return;
		}
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end()) {
			if (it->second->m_searchID == searchID) {
				if(delayDelete) {
					it->second->prepareToStop();
					++it;
				} else {
					delete it->second;
					it = m_searches.erase(it);
				}
			} else {
				++it;
			}
		}
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::stopSearch"));
	}
}

bool CSearchManager::isNodeSearch(const CUInt128 &target)
{
	try {
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end()) {
			if (it->second->m_target == target) {
				if( it->second->m_type == CSearch::NODE || it->second->m_type == CSearch::NODECOMPLETE ) {
					return true;
				} else {
					return false;
				}
			} else {
				++it;
			}
		}
	} catch (...)  {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::isNodeSearch"));
	}
	return false;
}

void CSearchManager::stopAllSearches(void)
{
	try {
		SearchMap::iterator it;
		for (it = m_searches.begin(); it != m_searches.end(); ++it) {
			delete it->second;
		}
		m_searches.clear();
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::stopAllSearches"));
	}
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

CSearch* CSearchManager::prepareFindKeywords(bool bUnicode, const wxString& keyword1, uint32 uSearchTermsSize, byte* pucSearchTermsData)
{
	CSearch *s = new CSearch;
	try {
		s->m_type = CSearch::KEYWORD;

		getWords(keyword1, &s->m_words);
		if (s->m_words.size() == 0) {
			delete s;
			throw wxString(_("Kademlia: search keyword too short"));
		}

		wxString wstrKeyword = s->m_words.front();
		if (bUnicode) {
			KadGetKeywordHash(wstrKeyword, &s->m_target);
		} 
		#if 0
		// Kry - I removed it anyway. It's been a while since the first unicoded eMule version.
		else {
			// backward compatibility: use local ACP encoding
			// TODO: to be removed in some months (when majority of nodes are Unicode compatible)
			CStringA strA(wstrKeyword);
			KadGetKeywordHash(strA, &s->m_target);
		}
		#endif

		if (alreadySearchingFor(s->m_target)) {
			delete s;
			throw wxT("Kademlia: Search keyword is already on search list: ") + wstrKeyword;
		}

		// Write complete packet
		s->m_searchTerms = new CSafeMemFile();
		s->m_searchTerms->WriteUInt128(s->m_target);
		if (uSearchTermsSize == 0) {
			s->m_searchTerms->WriteUInt8(0);
		} else {
			s->m_searchTerms->WriteUInt8(1);
			s->m_searchTerms->Write(pucSearchTermsData, uSearchTermsSize);
		}

		s->m_searchID = ++m_nextID;
		m_searches[s->m_target] = s;
		s->go();
	} catch (CIOException* ioe) {
		wxString strError = wxString::Format(wxT("IO-Exception in %s: Error %u") , __FUNCTION__, ioe->m_cause);
		ioe->Delete();
		delete s;
		throw strError;
	} catch (wxString strException) {
		throw strException;
	} catch (...) {
		e->Delete();
		delete s;
		throw wxString::Format(_T("Exception in %s: Unhandled (search packet I/O?)"), __FUNCTION__, );
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
		s->m_searchTerms = new CSafeMemFile();
		s->m_searchTerms->WriteUInt128(s->m_target);
		s->m_searchTerms->WriteUInt8(1);

		s->m_searchID = ++m_nextID;
		if( start ) {
			m_searches[s->m_target] = s;
			s->go();
		}
	} catch ( CIOException *ioe ) {
		AddDebugLogLineM( false, logKadSearch, wxString::Format(wxT("Exception in CSearchManager::prepareLookup (IO error(%i))"), ioe->m_cause));
		ioe->Delete();
		delete s;
		return NULL;
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::prepareLookup"));
		delete s;
		return NULL;
	}
	return s;
}

void CSearchManager::findNode(const CUInt128 &id)
{
	if (alreadySearchingFor(id)) {
		return;
	}

	try {
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		m_searches[s->m_target] = s;
		s->go();
	} catch (...) 	{
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::findNode"));
	}
	return;
}

void CSearchManager::findNodeComplete(const CUInt128 &id)
{
	if (alreadySearchingFor(id)) {
		return;
	}

	try {
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODECOMPLETE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		m_searches[s->m_target] = s;
		s->go();
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::findNodeComplete"));
	}
	return;
}

bool CSearchManager::alreadySearchingFor(const CUInt128 &target)
{
	bool retVal = false;
	try {
		retVal = (m_searches.count(target) > 0);
	} catch (...) 	{
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::alreadySearchingFor"));
	}
	return retVal;
}

void CSearchManager::getWords(const wxString& str, WordList *words)
{
	int len = 0;
	wxString current_word;
	uint32 i;
	wxStringTokenizer tkz(str, InvKadKeywordChars);
	while (tkz.HasMoreTokens()) {
		current_word = tkz.GetNextToken();
		
		if (len = current_word.Length() > 2) {
			KadTagStrMakeLower(current_word);
			#warninig terribly unoptimized... must be fixed.
			for( i = 0; i < words->size(); i++) {
				wordtemp = words->front();
				words->pop_front();
				if(  !wordtemp.IsSameAs(current_word)) {
					// Back to the list
					words->push_back(wordtemp);
				}
			}
			words->push_back(current_word);
		}
	}
	// Kry - Why this?
	if(words->size() > 1 && len == 3) {
		words->pop_back();
	}
}

void CSearchManager::jumpStart(void)
{
	time_t now = time(NULL);
	try {
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end()) {
			switch(it->second->getSearchTypes()){
				case CSearch::FILE: {
					if (it->second->m_created + SEARCHFILE_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHFILE_TOTAL || it->second->m_created + SEARCHFILE_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::KEYWORD: {
					if (it->second->m_created + SEARCHKEYWORD_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHKEYWORD_TOTAL || it->second->m_created + SEARCHKEYWORD_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::NOTES: {
					if (it->second->m_created + SEARCHNOTES_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHNOTES_TOTAL || it->second->m_created + SEARCHNOTES_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::FINDBUDDY: {
					if (it->second->m_created + SEARCHFINDBUDDY_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHFINDBUDDY_TOTAL || it->second->m_created + SEARCHFINDBUDDY_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::FINDSOURCE: {
					if (it->second->m_created + SEARCHFINDSOURCE_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHFINDSOURCE_TOTAL || it->second->m_created + SEARCHFINDSOURCE_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::NODE: {
					if (it->second->m_created + SEARCHNODE_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::NODECOMPLETE: {
					if (it->second->m_created + SEARCHNODE_LIFETIME < now) {
						CKademlia::getPrefs()->setPublish(true);
						delete it->second;
						it = m_searches.erase(it);
					} else if ((it->second->m_created + SEARCHNODECOMP_LIFETIME < now) && (it->second->getCount() > SEARCHNODECOMP_TOTAL)) {
						CKademlia::getPrefs()->setPublish(true);
						delete it->second;
						it = m_searches.erase(it);
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::STOREFILE: {
					if (it->second->m_created + SEARCHSTOREFILE_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHSTOREFILE_TOTAL || it->second->m_created + SEARCHSTOREFILE_LIFETIME - SEC(20) < now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::STOREKEYWORD: {
					if (it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHSTOREKEYWORD_TOTAL || it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME - SEC(20)< now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				case CSearch::STORENOTES: {
					if (it->second->m_created + SEARCHSTORENOTES_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else if (it->second->getCount() > SEARCHSTORENOTES_TOTAL || it->second->m_created + SEARCHSTORENOTES_LIFETIME - SEC(20)< now) {
						it->second->prepareToStop();
						++it;
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
				default: {
					if (it->second->m_created + SEARCH_LIFETIME < now) {
						delete it->second;
						it = m_searches.erase(it);
					} else {
						it->second->jumpStart();
						++it;
					}
					break;
				}
			}
		}
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::jumpStart"));
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
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end()) {
			switch(it->second->getSearchTypes()){
				case CSearch::FILE: {
					m_totalFile++;
					++it;
					break;
				}
				case CSearch::STOREFILE: {
					m_totalStoreSrc++;
					++it;
					break;
				}
				case CSearch::STOREKEYWORD:	{
					m_totalStoreKey++;
					++it;
					break;
				}
				case CSearch::FINDSOURCE: {
					m_totalSource++;
					++it;
					break;
				}
				case CSearch::STORENOTES: {
					m_totalStoreNotes++;
					++it;
					break;
				}
				case CSearch::NOTES: {
					m_totalNotes++;
					++it;
					break;
				}
				default:
					++it;
					break;
			}
		}
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::updateStats"));
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
	try {
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end()) {
			s = it->second;
		}
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::processPublishResults"));
	}

	if (s == NULL) {
//		AddDebugLogLineM(false, logKadSearch, wxT("Search either never existed or receiving late results (CSearchManager::processPublishResults)"));
		return;
	}
	
	s->m_count++;
	if( loadResponse ) {
		s->updateNodeLoad( load );
	}
}


void CSearchManager::processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results)
{
	CSearch *s = NULL;
	try {
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end())
			s = it->second;
	} catch (...) {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::processResponse"));
	}

	if (s == NULL) {
//		AddDebugLogLineM(false, logKadSearch, wxT("Search either never existed or receiving late results (CSearchManager::processResponse)"));
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
	try {
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end()) {
			s = it->second;
		}
	} catch (...)  {
		AddDebugLogLineM(false, logKadSearch, wxT("Exception in CSearchManager::processResult"));
	}

	if (s == NULL) {
//		AddDebugLogLineM (false, logKadSearch, wxT("Search either never existed or receiving late results (CSearchManager::processResult)"));
		TagList::const_iterator it;
		for (it = info->begin(); it != info->end(); it++) {
			delete *it;
		}
		delete info;
	} else {
		s->processResult(fromIP, fromPort, answer, info);
	}
}
