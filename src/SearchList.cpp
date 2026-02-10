//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "SearchList.h"		// Interface declarations.
#include "SearchTimeoutManager.h"	// Search timeout manager
#include "search/SearchAutoRetry.h"	// Auto-retry manager
#include "search/SearchPackageValidator.h"	// Package validator
#include "search/SearchPackageException.h"	// Package exception
#include "search/SearchResultHandler.h"	// Result handler interface
#include "search/SearchResultRouter.h"	// Result router
#include "search/PerSearchState.h"	// Per-search state management
#include "search/SearchIdGenerator.h"	// Search ID generation

#include "include/common/MacrosProgramSpecific.h"	// Needed for NOT_ON_REMOTEGUI

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <tags/ClientTags.h>
#include <tags/FileTags.h>

#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "amule.h"			// Needed for theApp
#include "ServerConnect.h"	// Needed for theApp->serverconnect
#include "Server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for theApp->serverlist
#include "Statistics.h"		// Needed for theStats
#include "ObservableQueue.h"// Needed for CQueueObserver
#include <common/Format.h>
#include "Logger.h"			// Needed for AddLogLineM/...
#include "Packet.h"			// Needed for CPacket
#include "GuiEvents.h"		// Needed for Notify_*


#ifndef AMULE_DAEMON
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "SearchDlg.h"		// Needed for CSearchDlg
#endif

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"

#include "SearchExpr.h"

#include "Scanner.h"
void LexInit(const wxString& pszInput);
void LexFree();

#include "Parser.hpp"
int yyerror(wxString errstr);


static wxString s_strCurKadKeyword;

static CSearchExpr _SearchExpr;

wxArrayString _astrParserErrors;

// Mutex for thread-safe access to the global parser state
static wxMutex s_parserMutex;


// Helper function for lexer.
void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	// Lock the parser mutex for thread safety
	wxMutexLocker lock(s_parserMutex);

	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;

	for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
		const wxString& str = pexpr->m_aExpr[i];
		if (str == SEARCHOPTOK_AND) {
			iOpAnd++;
		} else if (str == SEARCHOPTOK_OR) {
			iOpOr++;
		} else if (str == SEARCHOPTOK_NOT) {
			iOpNot++;
		}
	}

	// this limit (+ the additional operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
	//	+1 Type (Audio, Video)
	//	+1 MinSize
	//	+1 MaxSize
	//	+1 Avail
	//	+1 Extension
	//	+1 Complete sources
	//	+1 Codec
	//	+1 Bitrate
	//	+1 Length
	//	+1 Title
	//	+1 Album
	//	+1 Artist
	// ---------------
	//  12
	if (iOpAnd + iOpOr + iOpNot > 10) {
		yyerror(wxT("Search expression is too complex"));
	}

	_SearchExpr.m_aExpr.Empty();

	// optimize search expression, if no OR nor NOT specified
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0) {
		// figure out if we can use a better keyword than the one the user selected
		// for example most user will search like this "The oxymoronaccelerator 2", which would ask the node which indexes "the"
		// This causes higher traffic for such nodes and makes them a viable target to attackers, while the kad result should be
		// the same or even better if we ask the node which indexes the rare keyword "oxymoronaccelerator", so we try to rearrange
		// keywords and generally assume that the longer keywords are rarer
		if (/*thePrefs::GetRearrangeKadSearchKeywords() &&*/ !s_strCurKadKeyword.IsEmpty()) {
			for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
				if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
					if (pexpr->m_aExpr[i] != s_strCurKadKeyword
						&& pexpr->m_aExpr[i].find_first_of(Kademlia::CSearchManager::GetInvalidKeywordChars()) == wxString::npos
						&& pexpr->m_aExpr[i].Find('"') != 0 // no quoted expressions as keyword
						&& pexpr->m_aExpr[i].length() >= 3
						&& s_strCurKadKeyword.length() < pexpr->m_aExpr[i].length())
					{
						s_strCurKadKeyword = pexpr->m_aExpr[i];
					}
				}
			}
		}
		wxString strAndTerms;
		for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
			if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
				// Minor optimization: Because we added the Kad keyword to the boolean search expression,
				// we remove it here (and only here) again because we know that the entire search expression
				// does only contain (implicit) ANDed strings.
				if (pexpr->m_aExpr[i] != s_strCurKadKeyword) {
					if (!strAndTerms.IsEmpty()) {
						strAndTerms += ' ';
					}
					strAndTerms += pexpr->m_aExpr[i];
				}
			}
		}
		wxASSERT( _SearchExpr.m_aExpr.GetCount() == 0);
		_SearchExpr.m_aExpr.Add(strAndTerms);
	} else {
		if (pexpr->m_aExpr.GetCount() != 1 || pexpr->m_aExpr[0] != s_strCurKadKeyword)
			_SearchExpr.Add(pexpr);
	}
}


//! Helper class for packet creation
class CSearchExprTarget
{
public:
	CSearchExprTarget(CMemFile* pData, EUtf8Str eStrEncode, bool supports64bit, bool& using64bit)
		: m_data(pData),
		  m_eStrEncode(eStrEncode),
		  m_supports64bit(supports64bit),
		  m_using64bit(using64bit)
	{
		m_using64bit = false;
	}

	void WriteBooleanAND()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x00);			// "AND"
	}

	void WriteBooleanOR()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x01);			// "OR"
	}

	void WriteBooleanNOT()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x02);			// "NOT"
	}

	void WriteMetaDataSearchParam(const wxString& rstrValue)
	{
		m_data->WriteUInt8(1);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
	}

	void WriteMetaDataSearchParam(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParamASCII(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, utf8strNone); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

	void WriteMetaDataSearchParam(uint8_t uMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

protected:
	CMemFile* m_data;
	EUtf8Str m_eStrEncode;
	bool m_supports64bit;
	bool& m_using64bit;
};




///////////////////////////////////////////////////////////
// CSearchList

BEGIN_EVENT_TABLE(CSearchList, wxEvtHandler)
	EVT_MULE_TIMER(wxID_ANY, CSearchList::OnSearchTimer)
END_EVENT_TABLE()


CSearchList::CSearchList()
{
	// Initialize search timeout manager
	m_timeoutManager = std::make_unique<SearchTimeoutManager>();

	// Set timeout callback to handle search timeouts
	m_timeoutManager->setTimeoutCallback(
		[this](uint32_t searchId, SearchTimeoutManager::SearchType type, const wxString& reason) {
			// Convert SearchTimeoutManager::SearchType to SearchTimeoutType
			SearchTimeoutType timeoutType;
			switch (type) {
				case SearchTimeoutManager::LocalSearch:
					timeoutType = TimeoutLocalSearch;
					break;
				case SearchTimeoutManager::GlobalSearch:
					timeoutType = TimeoutGlobalSearch;
					break;
				case SearchTimeoutManager::KadSearch:
					timeoutType = TimeoutKadSearch;
					break;
				default:
					timeoutType = TimeoutLocalSearch;
					break;
			}
			OnSearchTimeout(searchId, timeoutType, reason);
		}
	);

	// Configure timeouts for different search types
	// Local searches: 30 seconds (quick response expected)
	m_timeoutManager->setLocalSearchTimeout(30000);
	// Global searches: 2 minutes (can take longer to query multiple servers)
	m_timeoutManager->setGlobalSearchTimeout(120000);
	// Kad searches: 3 minutes (Kad network can be slower)
	m_timeoutManager->setKadSearchTimeout(180000);
	// Heartbeat interval: 10 seconds (check for stalled searches)
	m_timeoutManager->setHeartbeatInterval(10000);

	AddDebugLogLineC(logSearch, wxT("CSearchList: SearchTimeoutManager initialized with callbacks"));
}


CSearchList::~CSearchList()
{
	StopSearch();


	while (!m_results.empty()) {
		RemoveResults(m_results.begin()->first);
	}
}


void CSearchList::RemoveResults(long searchID)
{
	// A non-existent search id will just be ignored
	Kademlia::CSearchManager::StopSearch(searchID, true);

	ResultMap::iterator it = m_results.find(searchID);
	if ( it != m_results.end() ) {
		CSearchResultList& list = it->second;

		for (size_t i = 0; i < list.size(); ++i) {
			delete list.at(i);
		}

		m_results.erase( it );
	}
}


uint32 CSearchList::GetNextSearchID()
{
	// Use the new thread-safe search ID generator
	return search::SearchIdGenerator::Instance().generateId();
}

// Per-search state management methods implementation

::PerSearchState* CSearchList::getOrCreateSearchState(long searchId, SearchType searchType, const wxString& searchString)
{
	wxMutexLocker lock(m_searchMutex);
	
	auto it = m_searchStates.find(searchId);
	if (it != m_searchStates.end()) {
		// Search state already exists, return it
		return it->second.get();
	}
	
	// Create new search state
	auto state = std::make_unique<::PerSearchState>(searchId, static_cast<uint8_t>(searchType), searchString);
	auto* statePtr = state.get();

	// Set the owner reference for callbacks
	statePtr->setSearchList(this);

	m_searchStates[searchId] = std::move(state);

	return statePtr;
}

::PerSearchState* CSearchList::getSearchState(long searchId)
{
	wxMutexLocker lock(m_searchMutex);
	auto it = m_searchStates.find(searchId);
	return (it != m_searchStates.end()) ? it->second.get() : nullptr;
}

const ::PerSearchState* CSearchList::getSearchState(long searchId) const
{
	wxMutexLocker lock(m_searchMutex);
	auto it = m_searchStates.find(searchId);
	return (it != m_searchStates.end()) ? it->second.get() : nullptr;
}

void CSearchList::removeSearchState(long searchId, bool releaseId)
{
	wxMutexLocker lock(m_searchMutex);

	// Get the search type to determine if we need to remove Kad ID mapping
	auto* searchState = getSearchState(searchId);
	if (searchState && searchState->getSearchType() == static_cast<uint8_t>(KadSearch)) {
		// Remove the Kad search ID mapping
		uint32_t kadSearchId = 0xffffff00 | (searchId & 0xff);
		m_kadSearchIdMap.erase(kadSearchId);
	}

	// Remove from search states map
	m_searchStates.erase(searchId);

	// Also remove from legacy active searches map for compatibility
	// Remove search parameters
	m_searchParams.erase(searchId);

	// Release the search ID for reuse (if requested)
	if (releaseId) {
		search::SearchIdGenerator::Instance().releaseId(searchId);
	}
}

bool CSearchList::hasSearchState(long searchId) const
{
	wxMutexLocker lock(m_searchMutex);
	return m_searchStates.find(searchId) != m_searchStates.end();
}

std::vector<long> CSearchList::getActiveSearchIds() const
{
	wxMutexLocker lock(m_searchMutex);
	std::vector<long> ids;
	ids.reserve(m_searchStates.size());

	for (const auto& pair : m_searchStates) {
		ids.push_back(pair.first);
	}

	return ids;
}

void CSearchList::mapKadSearchId(uint32_t kadSearchId, long originalSearchId)
{
	wxMutexLocker lock(m_searchMutex);
	m_kadSearchIdMap[kadSearchId] = originalSearchId;

	AddDebugLogLineC(logSearch, CFormat(wxT("Mapped Kad search ID %u to original search ID %ld"))
		% kadSearchId % originalSearchId);
}

long CSearchList::getOriginalSearchId(uint32_t kadSearchId) const
{
	wxMutexLocker lock(m_searchMutex);
	KadSearchIdMap::const_iterator it = m_kadSearchIdMap.find(kadSearchId);
	if (it != m_kadSearchIdMap.end()) {
		return it->second;
	}
	return 0;
}

void CSearchList::removeKadSearchIdMapping(uint32_t kadSearchId)
{
	wxMutexLocker lock(m_searchMutex);
	m_kadSearchIdMap.erase(kadSearchId);

	AddDebugLogLineC(logSearch, CFormat(wxT("Removed Kad search ID mapping for %u"))
		% kadSearchId);
}

wxString CSearchList::StartNewSearch(uint32* searchID, SearchType type, CSearchParams& params)
{
	// Check that we can actually perform the specified desired search.
	if ((type == KadSearch) && !Kademlia::CKademlia::IsRunning()) {
		return _("Kad search can't be done if Kad is not running");
	} else if ((type == LocalSearch || type == GlobalSearch) && !theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// CRITICAL FIX: Removed duplicate detection for per-search tab architecture
	// In a per-search tab system, each search should get its own unique ID and tab
	// Duplicate detection prevents users from running multiple searches with the same parameters
	// which is a valid use case (e.g., searching the same term on different servers over time)
	// The SearchIdGenerator now generates unique, non-reusable IDs, so duplicate IDs are impossible

	if (type == KadSearch) {
		Kademlia::WordList words;
		Kademlia::CSearchManager::GetWords(params.searchString, &words);
		if (!words.empty()) {
			params.strKeyword = words.front();
		} else {
			return _("No keyword for Kad search - aborting");
		}
	}

	bool supports64bit = type == KadSearch ? true : theApp->serverconnect->GetCurrentServer() != NULL && (theApp->serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
	bool packetUsing64bit;

	// This MemFile is automatically free'd
	// Pass Kad keyword from params for Kad searches
	CMemFilePtr data = CreateSearchData(params, type, supports64bit, packetUsing64bit, (type == KadSearch) ? params.strKeyword : wxString(wxEmptyString));

	if (data.get() == NULL) {
		wxASSERT(_astrParserErrors.GetCount());
		wxString error;

		for (unsigned int i = 0; i < _astrParserErrors.GetCount(); ++i) {
			error += _astrParserErrors[i] + wxT("\n");
		}

		return error;
	}

	if (type == KadSearch) {
		try {
			// Generate search ID through SearchIdGenerator for consistency
			if (*searchID == 0) {
				*searchID = GetNextSearchID();
			} else {
				// If searchID was provided, reserve it in the generator to ensure uniqueness
				// First check if it's already active (e.g., from duplicate detection)
				if (search::SearchIdGenerator::Instance().isValidId(*searchID)) {
					// ID is already active - this happens for duplicate active searches
					// Don't try to reserve it (it's already reserved)
					AddDebugLogLineC(logSearch, CFormat(wxT("Reusing active search ID %u for Kad search"))
						% *searchID);
				} else if (!search::SearchIdGenerator::Instance().reserveId(*searchID)) {
					// Add debugging info
					AddDebugLogLineC(logSearch, CFormat(wxT("Failed to reserve search ID %u for Kad search: already in use or invalid"))
						% *searchID);
					return _("Search ID is already in use");
				}
			}

			// Convert to Kademlia's special ID format (0xffffff??)
			// This ensures Kademlia uses our ID instead of generating its own
			uint32_t kadSearchId = 0xffffff00 | (*searchID & 0xff);

			// Stop any existing search with this ID (for safety)
			Kademlia::CSearchManager::StopSearch(kadSearchId, false);

			// searchstring will get tokenized there
			// Pass our generated ID to Kademlia
			Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(params.strKeyword, data->GetLength(), data->GetRawBuffer(), kadSearchId);

			// Verify Kademlia used our ID
			if (search->GetSearchID() != kadSearchId) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Kademlia changed search ID: expected %u, got %u"))
					% kadSearchId % search->GetSearchID());
				// Release our reserved ID
				search::SearchIdGenerator::Instance().releaseId(*searchID);
				delete search;
				return _("Kademlia search ID mismatch");
			}

			// Map Kad search ID to original search ID for result routing
			mapKadSearchId(kadSearchId, *searchID);

			// Create per-search state for Kad search
			auto* searchState = getOrCreateSearchState(*searchID, type, params.searchString);
			if (!searchState) {
				// Release the reserved ID on failure
				search::SearchIdGenerator::Instance().releaseId(*searchID);
				removeKadSearchIdMapping(kadSearchId);
				delete search;
				return _("Failed to create per-search state for Kad search");
			}

			// Initialize Kad search state
			searchState->setKadSearchFinished(false);
			searchState->setKadSearchRetryCount(0);
			searchState->setKadKeyword(params.strKeyword);  // Store Kad keyword per-search

			// Store search parameters for this search ID
			m_searchParams[*searchID] = params;
		} catch (const wxString& what) {
			AddLogLineC(what);
			return _("Unexpected error while attempting Kad search: ") + what;
		}
	} else if (type == LocalSearch || type == GlobalSearch) {
		// This is an ed2k search, local or global
		// Always generate search ID through SearchIdGenerator for consistency
		if (*searchID == 0) {
			*searchID = GetNextSearchID();
		} else {
			// If searchID was provided, reserve it in the generator to ensure uniqueness
			// First check if it's already active (e.g., from duplicate detection)
			if (search::SearchIdGenerator::Instance().isValidId(*searchID)) {
				// ID is already active - this happens for duplicate active searches
				// Don't try to reserve it (it's already reserved)
				AddDebugLogLineC(logSearch, CFormat(wxT("Reusing active search ID %u for ED2K search"))
					% *searchID);
			} else if (!search::SearchIdGenerator::Instance().reserveId(*searchID)) {
				// Add debugging info
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to reserve search ID %u for ED2K search: already in use or invalid"))
					% *searchID);
				return _("Search ID is already in use");
			}
		}
		
		// Create per-search state for ED2K search
		auto* searchState = getOrCreateSearchState(*searchID, type, params.searchString);
		if (!searchState) {
			// Release the reserved ID on failure
			search::SearchIdGenerator::Instance().releaseId(*searchID);
			return _("Failed to create per-search state for ED2K search");
		}

		// Store search parameters for this search ID
		m_searchParams[*searchID] = params;

		CPacket* searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_SEARCHREQUEST);

		NOT_ON_REMOTEGUI(
			theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
		)
		theApp->serverconnect->SendPacket(searchPacket, (type == LocalSearch));

		if (type == GlobalSearch) {
			// Store search packet in per-search state
			searchState->setSearchPacket(std::unique_ptr<CPacket>(searchPacket), packetUsing64bit);

			// CRITICAL FIX: Initialize global search timer and server queue immediately
			// For global searches, we need to start querying other servers via UDP
			// The timer triggers OnGlobalSearchTimer which sends UDP requests to other servers
			// This initialization must happen here, not in LocalSearchEnd(), because:
			// 1. LocalSearchEnd() is only called when TCP results arrive from the local server
			// 2. Global searches may not receive TCP results (they primarily use UDP)
			// 3. Without this initialization, the global search timer never starts and the search gets stuck

			// Create and set up the server queue
			auto serverQueue = std::make_unique<CQueueObserver<CServer*>>();
			searchState->setServerQueue(std::move(serverQueue));

			// Create and set up the timer
			auto timer = std::make_unique<CTimer>(this, *searchID);
			searchState->setTimer(std::move(timer));

			// Start the timer immediately to begin querying other servers
			// The timer fires every 750ms and sends UDP requests to the next server in the queue
			if (!searchState->startTimer(750, false)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to start global search timer for ID=%u"))
					% *searchID);
			} else {
				AddDebugLogLineC(logSearch, CFormat(wxT("Global search timer started for ID=%u"))
					% *searchID);
			}
		} else if (type == LocalSearch) {
			// CRITICAL FIX: Add timeout mechanism for local searches
			// Local searches can get stuck if the server doesn't respond or returns no results
			// We need a timeout to ensure the search is marked as complete even if no results arrive
			// The timeout is set to 30 seconds, which is reasonable for a local search
			static const int LOCAL_SEARCH_TIMEOUT_MS = 30000;

			// Create and set up a timeout timer
			auto timeoutTimer = std::make_unique<CTimer>(this, *searchID);
			searchState->setTimer(std::move(timeoutTimer));

			// Start the timeout timer (one-shot)
			// This will trigger OnGlobalSearchTimer after timeout, which will check if the search is still active
			// If no results have arrived, the timer handler will mark the search as complete
			if (!searchState->startTimer(LOCAL_SEARCH_TIMEOUT_MS, true)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to start local search timeout timer for ID=%u"))
					% *searchID);
			} else {
				AddDebugLogLineC(logSearch, CFormat(wxT("Local search timeout timer started for ID=%u (timeout=%dms)"))
					% *searchID % LOCAL_SEARCH_TIMEOUT_MS);
			}
		}
		// Note: For local searches, SendPacket with delpacket=true takes ownership of the packet
		// For global searches, delpacket=false so we retain ownership and store it in searchState
	}

	// Log search start
	AddDebugLogLineC(logSearch, CFormat(wxT("Search started: ID=%u, Type=%d, String='%s'"))
		% *searchID % (int)type % params.searchString);

	// Log Kad-specific info
	if (type == KadSearch) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Kad search prepared: ID=%u, Keyword='%s'"))
			% *searchID % params.strKeyword);
	}

	// RELIABLE RETRY FIX: Register search with timeout manager
	// This ensures searches don't get stuck in [Searching] state
	// The timeout manager will detect stalled searches and trigger recovery
	if (m_timeoutManager) {
		SearchTimeoutManager::SearchType timeoutType;
		switch (type) {
			case LocalSearch:
				timeoutType = SearchTimeoutManager::LocalSearch;
				break;
			case GlobalSearch:
				timeoutType = SearchTimeoutManager::GlobalSearch;
				break;
			case KadSearch:
				timeoutType = SearchTimeoutManager::KadSearch;
				break;
			default:
				timeoutType = SearchTimeoutManager::LocalSearch;
				break;
		}

		if (!m_timeoutManager->registerSearch(*searchID, timeoutType)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to register search %u with timeout manager"))
				% *searchID);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Search %u registered with timeout manager (type=%d)"))
				% *searchID % (int)timeoutType);
		}
	}

	return wxEmptyString;
}


CSearchList::CSearchParams CSearchList::GetSearchParams(long searchID)
{
	ParamMap::iterator it = m_searchParams.find(searchID);
	if (it != m_searchParams.end()) {
		return it->second;
	}
	return CSearchParams(); // Return empty params if not found
}


wxString CSearchList::RequestMoreResults(long searchID)
{
	// Check if we're connected to eD2k
	if (!theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Get the original search parameters
	CSearchParams params = GetSearchParams(searchID);
	if (params.searchString.IsEmpty()) {
		return _("No search parameters available for this search");
	}

	// Stop any current search to prevent race conditions
	StopSearch(true);

	// Use the original search ID to append results to the same search
	// Don't create a new search ID - we want to append results to the existing search
	uint32 originalSearchID = searchID;

	// Create a new global search with the same parameters using the original search ID
	return StartNewSearch(&originalSearchID, GlobalSearch, params);
}


wxString CSearchList::RequestMoreResultsFromServer(const CServer* server, long searchId)
{
	// Check if we're connected to eD2k
	if (!theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Check if server is valid
	if (!server) {
		return _("Invalid server");
	}

	// Get the original search parameters
	CSearchParams params = GetSearchParams(searchId);
	if (params.searchString.IsEmpty()) {
		return _("No search parameters available for this search");
	}

	// Create search data packet
	bool packetUsing64bit = false;
	CMemFilePtr data = CreateSearchData(params, GlobalSearch, server->SupportsLargeFilesUDP(), packetUsing64bit);
	if (!data) {
		return _("Failed to create search data");
	}

	// Determine which search request type to use based on server capabilities
	CPacket* searchPacket = NULL;
	if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
		// Use OP_GLOBSEARCHREQ3 for servers that support large files and extended getfiles
		CMemFile extData(50);
		uint32_t tagCount = 1;
		extData.WriteUInt32(tagCount);
		CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
		flags.WriteNewEd2kTag(&extData);
		searchPacket = new CPacket(OP_GLOBSEARCHREQ3, data->GetLength() + (uint32_t)extData.GetLength(), OP_EDONKEYPROT);
		searchPacket->CopyToDataBuffer(0, extData.GetRawBuffer(), extData.GetLength());
		searchPacket->CopyToDataBuffer(extData.GetLength(), data->GetRawBuffer(), data->GetLength());
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ3: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
		// Use OP_GLOBSEARCHREQ2 for servers that support extended getfiles
		searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ2);
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ2: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	} else {
		// Use OP_GLOBSEARCHREQ for basic servers
		searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ);
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	}

	// Send the search request to the server
	NOT_ON_REMOTEGUI(
		theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
	)
	// Cast away const because SendUDPPacket doesn't take const pointer
	theApp->serverconnect->SendUDPPacket(searchPacket, const_cast<CServer*>(server), true);

	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	// Find the active ED2K search (Local or Global)
	// This function is called when TCP search results are received from the local server
	wxMutexLocker lock(m_searchMutex);
	long searchId = -1;
	::PerSearchState* state = nullptr;

	// Find the most recent active ED2K search (Local or Global)
	// Note: We need to check both LocalSearch and GlobalSearch because:
	// 1. For LocalSearch: TCP results mark the end of the search
	// 2. For GlobalSearch: TCP results from the local server arrive first, then UDP results from other servers
	for (auto it = m_searchStates.rbegin(); it != m_searchStates.rend(); ++it) {
		if (it->second && it->second->isSearchActive()) {
			uint8_t type = it->second->getSearchType();
			if (type == LocalSearch || type == GlobalSearch) {
				searchId = it->first;
				state = it->second.get();
				break;
			}
		}
	}

	if (!state) {
		// No active ED2K search
		return;
	}

	// Get search type from per-search state
	uint8_t searchType = state->getSearchType();

	if (searchType == GlobalSearch) {
		// For global search, the timer should already be running (started in StartNewSearch)
		// The TCP results from the local server have been received, but the global search
		// continues via UDP to other servers (handled by OnGlobalSearchTimer)
		// Nothing to do here - the timer is already running and will continue querying servers
		AddDebugLogLineC(logSearch, CFormat(wxT("Global search TCP results received for ID=%u, continuing with UDP queries"))
			% searchId);
	} else if (searchType == LocalSearch) {
		// For local search, TCP results mark the end of the search
		// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
		// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
		ResultMap::iterator it = m_results.find(searchId);
		bool hasResults = (it != m_results.end()) && !it->second.empty();

		// Only mark the search as finished if we have results
		if (hasResults) {
			OnSearchComplete(searchId, static_cast<SearchType>(searchType), hasResults);
		} else {
			// No results - let the UI handle retry through SearchStateManager
			// Just mark the search as finished internally
			// Release the search ID since search is complete (no results)
			if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (no results)"))
					% searchId);
			} else {
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (no results) - already released?"))
					% searchId);
			}
			// Mark search as inactive
			state->setSearchActive(false);
		}
	}
}


uint32 CSearchList::GetSearchProgress(long searchId) const
{
	if (searchId == -1) {
		// No active search
		return 0;
	}

	// Get the per-search state
	const ::PerSearchState* state = getSearchState(searchId);
	if (!state) {
		// No state found for this search ID
		return 0;
	}

	// Get search type from per-search state
	uint8_t searchType = state->getSearchType();

	if (searchType == KadSearch) {
		// We cannot measure the progress of Kad searches.
		// But we can tell when they are over.
		return state->isKadSearchFinished() ? 0xfffe : 0;
	}

	// Check if search is active
	if (!state->isSearchActive()) {
		// Search is not active
		return 0;
	}

	switch (searchType) {
		case LocalSearch:
			return 0xffff;

		case GlobalSearch:
			// Calculate progress based on per-search server queue
			return state->getProgress();

		default:
			wxFAIL;
	}
	return 0;
}


void CSearchList::OnSearchTimer(CTimerEvent& ev)
{
	// Find the active search (could be global or local for timeout)
	wxMutexLocker lock(m_searchMutex);
	long searchId = -1;
	::PerSearchState* state = nullptr;

	// Find the most recent active search (Global or Local)
	// Note: We need to check both types because local searches now use a timeout timer
	for (auto it = m_searchStates.rbegin(); it != m_searchStates.rend(); ++it) {
		if (it->second && it->second->isSearchActive()) {
			uint8_t searchType = it->second->getSearchType();
			if (searchType == GlobalSearch || searchType == LocalSearch) {
				searchId = it->first;
				state = it->second.get();
				break;
			}
		}
	}

	if (!state) {
		// No active search
		return;
	}

	// Get search type from per-search state
	uint8_t searchType = state->getSearchType();

	// Handle local search timeout
	if (searchType == LocalSearch) {
		// Local search timeout - check if we have results
		ResultMap::iterator it = m_results.find(searchId);
		bool hasResults = (it != m_results.end()) && !it->second.empty();

		// Stop the timer (it's a one-shot timer)
		state->stopTimer();

		// Mark the search as complete
		if (hasResults) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Local search timeout with results for ID=%u"))
				% searchId);
			OnSearchComplete(searchId, LocalSearch, hasResults);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Local search timeout with no results for ID=%u"))
				% searchId);
			// Release the search ID
			if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (local search timeout, no results)"))
					% searchId);
			}
			// Mark search as inactive
			state->setSearchActive(false);
		}
		return;
	}

	// Handle global search (existing logic)
	// Get search packet from per-search state
	CPacket* searchPacket = state->getSearchPacket();
	if (!searchPacket) {
		// This was a pending event, handled after 'Stop' was pressed.
		return;
	}

	CQueueObserver<CServer*>* serverQueue = state->getServerQueue();
	if (!serverQueue) {
		// No server queue set
		return;
	}

	if (!serverQueue->IsActive()) {
		theApp->serverlist->AddObserver(serverQueue);
	}

	// UDP requests must not be sent to this server.
	const CServer* localServer = theApp->serverconnect->GetCurrentServer();
	if (localServer) {
		uint32 localIP = localServer->GetIP();
		uint16 localPort = localServer->GetPort();
		while (serverQueue->GetRemaining()) {
			CServer* server = serverQueue->GetNext();

			// Compare against the currently connected server.
			if ((server->GetPort() == localPort) && (server->GetIP() == localIP)) {
				// We've already requested from the local server.
				continue;
			} else {
				if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
					CMemFile data(50);
					uint32_t tagCount = 1;
					data.WriteUInt32(tagCount);
					CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
					flags.WriteNewEd2kTag(&data);
					CPacket *extSearchPacket = new CPacket(OP_GLOBSEARCHREQ3, searchPacket->GetPacketSize() + (uint32_t)data.GetLength(), OP_EDONKEYPROT);
					extSearchPacket->CopyToDataBuffer(0, data.GetRawBuffer(), data.GetLength());
					extSearchPacket->CopyToDataBuffer(data.GetLength(), searchPacket->GetDataBuffer(), searchPacket->GetPacketSize());
					NOT_ON_REMOTEGUI(
											theStats::AddUpOverheadServer(extSearchPacket->GetPacketSize());
					)
					theApp->serverconnect->SendUDPPacket(extSearchPacket, server, true);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ3 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
				} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
					if (!state->is64bitPacket() || server->SupportsLargeFilesUDP()) {
						searchPacket->SetOpCode(OP_GLOBSEARCHREQ2);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ2 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						NOT_ON_REMOTEGUI(
													theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
						)
						theApp->serverconnect->SendUDPPacket(searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				} else {
					if (!state->is64bitPacket() || server->SupportsLargeFilesUDP()) {
						searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						NOT_ON_REMOTEGUI(
													theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
						)
						theApp->serverconnect->SendUDPPacket(searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				}
				CoreNotify_Search_Update_Progress(GetSearchProgress(searchId));
				return;
			}
		}
	}
	// No more servers left to ask.

	// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
	// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
	ResultMap::iterator it = m_results.find(searchId);
	bool hasResults = (it != m_results.end()) && !it->second.empty();

	// Only mark the search as finished if we have results
	if (hasResults) {
		OnSearchComplete(searchId, GlobalSearch, hasResults);
		// Only stop if not retrying
		if (state->isSearchActive()) {
			StopSearch(searchId, true);
		}
	} else {
		// No results - let the UI handle retry through SearchStateManager
		// Notify the UI that global search has ended
		// Release the search ID since search is complete (no results)
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (global search, no results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (global search, no results) - already released?"))
				% searchId);
		}

		// Mark search as inactive
		state->setSearchActive(false);
		Notify_GlobalSearchEnd();
	}
}


void CSearchList::ProcessSharedFileList(const uint8_t* in_packet, uint32 size,
	CUpDownClient* sender, bool *moreResultsAvailable, const wxString& directory)
{
	wxCHECK_RET(sender, wxT("No sender in search-results from client."));

	long searchID = reinterpret_cast<wxUIntPtr>(sender);

#ifndef AMULE_DAEMON
	if (!theApp->amuledlg->m_searchwnd->CheckTabNameExists(LocalSearch, sender->GetUserName())) {
		theApp->amuledlg->m_searchwnd->CreateNewTab(sender->GetUserName() + wxT(" (0)"), searchID);
	}
#endif

	const CMemFile packet(in_packet, size);
	uint32 results = packet.ReadUInt32();
	bool unicoded = (sender->GetUnicodeSupport() != utf8strNone);
	for (unsigned int i = 0; i != results; ++i){
		CSearchFile* toadd = new CSearchFile(packet, unicoded, searchID, 0, 0, directory);
		toadd->SetClientID(sender->GetUserIDHybrid());
		toadd->SetClientPort(sender->GetUserPort());
		AddToList(toadd, true);
	}

	if (moreResultsAvailable)
		*moreResultsAvailable = false;

	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1) {
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (moreResultsAvailable) {
				*moreResultsAvailable = (ucMore == 1);
			}
		}
	}
}


void CSearchList::ProcessSearchAnswer(const uint8_t* in_packet, uint32_t size, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	CMemFile packet(in_packet, size);

	uint32_t results = packet.ReadUInt32();

	// Get the search ID from the active searches map in a thread-safe manner
	// This ensures results are associated with the correct search
	long searchId = -1;
	SearchType searchType = LocalSearch; // Default to local search

	{
		wxMutexLocker lock(m_searchMutex);
		if (!m_searchStates.empty()) {
			// Check if this is from the local server (TCP) or a remote server (UDP)
			// TCP responses are for local searches, UDP responses are for global searches
			bool isFromLocalServer = false;
			if (theApp && theApp->serverconnect) {
				const CServer* currentServer = theApp->serverconnect->GetCurrentServer();
				if (currentServer && currentServer->GetIP() == serverIP && currentServer->GetPort() == serverPort) {
					isFromLocalServer = true;
				}
			}

			// Find the most recent search matching the response type
			for (auto it = m_searchStates.rbegin(); it != m_searchStates.rend(); ++it) {
				if (it->second && it->second->isSearchActive()) {
					uint8_t type = it->second->getSearchType();
					if (isFromLocalServer && type == LocalSearch) {
						searchId = it->first;
						searchType = LocalSearch;
						break;
					} else if (!isFromLocalServer && type == GlobalSearch) {
						searchId = it->first;
						searchType = GlobalSearch;
						break;
					}
				}
			}
		}
	}

	// If no valid search ID found, drop the results
	if (searchId == -1) {
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Received search results from %s:%u but no matching active search found, dropping results"),
			(uint32_t)serverIP, serverPort));
		return;
	}

	// Collect all results first
	std::vector<CSearchFile*> resultVector;
	for (; results > 0; --results) {
		resultVector.push_back(new CSearchFile(packet, optUTF8, searchId, serverIP, serverPort));
	}

	// Process results through validator (this adds them to SearchList)
	NOT_ON_REMOTEGUI(
		if (!resultVector.empty()) {
			search::SearchResultRouter::Instance().RouteResults(searchId, resultVector);
		}
	)
}


void CSearchList::ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	// Get the search ID from the active searches map in a thread-safe manner
	// This ensures results are associated with the correct search
	long searchId = -1;
	{
		wxMutexLocker lock(m_searchMutex);
		if (!m_searchStates.empty()) {
			// Find the most recent global search (UDP is only used for global searches)
			// We need to ensure we don't accidentally route to a local search
			for (auto it = m_searchStates.rbegin(); it != m_searchStates.rend(); ++it) {
				if (it->second && it->second->isSearchActive() && it->second->getSearchType() == GlobalSearch) {
					searchId = it->first;
					break;
				}
			}
		}
	}

	// If no valid search ID found, drop the result
	// UDP results should only go to global searches, not local searches
	if (searchId == -1) {
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Received UDP search result from %s:%u but no active global search found, dropping result"),
			(uint32_t)serverIP, serverPort));
		return;
	}

	// Create result
	CSearchFile* result = new CSearchFile(packet, optUTF8, searchId, serverIP, serverPort);

	// Process result through validator (this adds it to SearchList)
	NOT_ON_REMOTEGUI(
		search::SearchResultRouter::Instance().RouteResult(searchId, result);
	)
}


bool CSearchList::AddToList(CSearchFile* toadd, bool clientResponse)
{
	const uint64 fileSize = toadd->GetFileSize();
	// If filesize is 0, or file is too large for the network, drop it
	if ((fileSize == 0) || (fileSize > MAX_FILE_SIZE)) {
		AddDebugLogLineN(logSearch,
				CFormat(wxT("Dropped result with filesize %u: %s"))
					% fileSize
					% toadd->GetFileName().GetPrintable());

		delete toadd;
		return false;
	}

	// Get the result type for this specific search (thread-safe)
	wxString resultTypeForSearch;
	{
		wxMutexLocker lock(m_searchMutex);
		ParamMap::iterator it = m_searchParams.find(toadd->GetSearchID());
		if (it != m_searchParams.end()) {
			resultTypeForSearch = it->second.typeText;
		}
	}

	// If the result was not the type the user wanted, drop it.
	if ((clientResponse == false) && !resultTypeForSearch.IsEmpty() && resultTypeForSearch != ED2KFTSTR_PROGRAM) {
		if (resultTypeForSearch.CmpNoCase(wxT("Any")) != 0) {
			if (GetFileTypeByName(toadd->GetFileName()) != resultTypeForSearch) {
				AddDebugLogLineN(logSearch,
					CFormat( wxT("Dropped result type %s != %s, file %s") )
						% GetFileTypeByName(toadd->GetFileName())
						% resultTypeForSearch
						% toadd->GetFileName().GetPrintable());

				delete toadd;
				return false;
			}
		}
	}

	// Get, or implicitly create, the map of results for this search
	CSearchResultList& results = m_results[toadd->GetSearchID()];

	for (size_t i = 0; i < results.size(); ++i) {
		CSearchFile* item = results.at(i);

		if ((toadd->GetFileHash() == item->GetFileHash()) && (toadd->GetFileSize() == item->GetFileSize())) {
			AddDebugLogLineN(logSearch, CFormat(wxT("Received duplicate results for '%s' : %s")) % item->GetFileName().GetPrintable() % item->GetFileHash().Encode());
			// Add the child, possibly updating the parents filename.
			item->AddChild(toadd);
			Notify_Search_Update_Sources(item);
			return true;
		}
	}

	AddDebugLogLineN(logSearch,
		CFormat(wxT("Added new result '%s' : %s"))
			% toadd->GetFileName().GetPrintable() % toadd->GetFileHash().Encode());

	// New unique result, simply add and display.
	results.push_back(toadd);
	Notify_Search_Add_Result(toadd);

	// RELIABLE RETRY FIX: Update heartbeat when results are received
	// This indicates the search is making progress and should not be timed out
	if (m_timeoutManager) {
		m_timeoutManager->updateHeartbeat(toadd->GetSearchID());
		AddDebugLogLineC(logSearch, CFormat(wxT("Updated heartbeat for search %u (result received)"))
			% toadd->GetSearchID());
	}

	return true;
}


const CSearchResultList& CSearchList::GetSearchResults(long searchID) const
{
	ResultMap::const_iterator it = m_results.find(searchID);
	if (it != m_results.end()) {
		return it->second;
	}

	// TODO: Should we assert in this case?
	static CSearchResultList list;
	return list;
}


void CSearchList::AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat)
{
	ResultMap::iterator it = m_results.begin();
	for ( ; it != m_results.end(); ++it ) {
		CSearchResultList& list = it->second;

		for ( unsigned int i = 0; i < list.size(); ++i ) {
			if ( list[i]->GetFileHash() == hash ) {
				CoreNotify_Search_Add_Download( list[i], cat );

				return;
			}
		}
	}
}


void CSearchList::StopSearch(bool globalOnly)
{
	// This legacy function stops all searches
	// For backward compatibility, we stop all active searches
	auto activeIds = getActiveSearchIds();
	for (long searchId : activeIds) {
		StopSearch(searchId, globalOnly);
	}
}

void CSearchList::StopSearch(long searchID, bool globalOnly)
{
	// RELIABLE RETRY FIX: Unregister search from timeout manager
	if (m_timeoutManager && m_timeoutManager->isSearchRegistered(searchID)) {
		m_timeoutManager->unregisterSearch(searchID);
		AddDebugLogLineC(logSearch, CFormat(wxT("Unregistered search %u from timeout manager"))
			% searchID);
	}

	// Get the search state for this ID
	auto* searchState = getSearchState(searchID);
	if (!searchState) {
		// Search not found, nothing to stop
		return;
	}

	// Get search type from state
	uint8_t searchType = searchState->getSearchType();

	if (searchType == GlobalSearch) {
		// Stop the timer for this search
		searchState->stopTimer();

		// Clear search packet for this search
		searchState->clearSearchPacket();

		CoreNotify_Search_Update_Progress(0xffff);
	} else if (searchType == KadSearch && !globalOnly) {
		// Convert original search ID to Kad search ID format
		uint32_t kadSearchId = 0xffffff00 | (searchID & 0xff);

		// Remove the Kad search ID mapping
		removeKadSearchIdMapping(kadSearchId);

		// Stop Kad search using the Kad search ID
		Kademlia::CSearchManager::StopSearch(kadSearchId, false);
		searchState->setKadSearchFinished(true);
	}

	// Remove the search state
	removeSearchState(searchID);
}


CSearchList::CMemFilePtr CSearchList::CreateSearchData(CSearchParams& params, SearchType type, bool supports64bit, bool& packetUsing64bit, const wxString& kadKeyword)
{
	// Count the number of used parameters
	unsigned int parametercount = 0;
	if ( !params.typeText.IsEmpty() )	++parametercount;
	if ( params.minSize > 0 )			++parametercount;
	if ( params.maxSize > 0 )			++parametercount;
	if ( params.availability > 0 )		++parametercount;
	if ( !params.extension.IsEmpty() )	++parametercount;

	wxString typeText = params.typeText;
	if (typeText == ED2KFTSTR_ARCHIVE){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		typeText = ED2KFTSTR_PROGRAM;
	} else if (typeText == ED2KFTSTR_CDIMAGE){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		typeText = ED2KFTSTR_PROGRAM;
	}

	// Must write parametercount - 1 parameter headers
	CMemFilePtr data(new CMemFile(100));

	// Lock the parser mutex for thread-safe access to global parser state
	wxMutexLocker parserLock(s_parserMutex);

	_astrParserErrors.Empty();
	_SearchExpr.m_aExpr.Empty();

	// Use the provided Kad keyword instead of global state
	if (type == KadSearch && !kadKeyword.IsEmpty()) {
		wxASSERT( !kadKeyword.IsEmpty() );
		// Store it in the global variable for backward compatibility with ParsedSearchExpression
		// TODO: Refactor ParsedSearchExpression to accept kadKeyword as parameter
		s_strCurKadKeyword = kadKeyword;
	} else if (type == KadSearch) {
		// Fallback to params.strKeyword if kadKeyword not provided
		wxASSERT( !params.strKeyword.IsEmpty() );
		s_strCurKadKeyword = params.strKeyword;
	} else {
		s_strCurKadKeyword.Clear();
	}

	LexInit(params.searchString);
	int iParseResult = yyparse();
	LexFree();

	if (_astrParserErrors.GetCount() > 0) {
		for (unsigned int i=0; i < _astrParserErrors.GetCount(); ++i) {
			AddLogLineNS(CFormat(wxT("Error %u: %s\n")) % i % _astrParserErrors[i]);
		}

		return CMemFilePtr(nullptr);
	}

	if (iParseResult != 0) {
		_astrParserErrors.Add(CFormat(wxT("Undefined error %i on search expression")) % iParseResult);

		return CMemFilePtr(nullptr);
	}

	if (type == KadSearch && s_strCurKadKeyword != params.strKeyword) {
		AddDebugLogLineN(logSearch, CFormat(wxT("Keyword was rearranged, using '%s' instead of '%s'")) % s_strCurKadKeyword % params.strKeyword);
		params.strKeyword = s_strCurKadKeyword;
	}

	parametercount += _SearchExpr.m_aExpr.GetCount();

	// For Kad searches with empty expression, the keyword is written directly
	// and should be counted as a parameter
	if (_SearchExpr.m_aExpr.GetCount() == 0 && type == KadSearch && !params.strKeyword.IsEmpty()) {
		++parametercount;
	}

	/* Leave the unicode comment there, please... */
	CSearchExprTarget target(data.get(), true /*I assume everyone is unicoded */ ? utf8strRaw : utf8strNone, supports64bit, packetUsing64bit);

	unsigned int iParameterCount = 0;
	if (_SearchExpr.m_aExpr.GetCount() <= 1) {
		// lugdunummaster requested that searches without OR or NOT operators,
		// and hence with no more expressions than the string itself, be sent
		// using a series of ANDed terms, intersecting the ANDs on the terms
		// (but prepending them) instead of putting the boolean tree at the start
		// like other searches. This type of search is supposed to take less load
		// on servers. Go figure.
		//
		// input:      "a" AND min=1 AND max=2
		// instead of: AND AND "a" min=1 max=2
		// we use:     AND "a" AND min=1 max=2

		// CRITICAL FIX: Handle Kad searches with empty expression
		// For Kad searches with a single keyword, the parser removes the keyword
		// from _SearchExpr.m_aExpr because it's used as the Kad keyword.
		// This results in an empty packet if we don't handle it specially.
		if (_SearchExpr.m_aExpr.GetCount() > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[0]);
		} else if (type == KadSearch && !params.strKeyword.IsEmpty()) {
			// For Kad searches with empty expression, use the keyword directly
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(params.strKeyword);
			AddDebugLogLineC(logSearch, CFormat(wxT("Using Kad keyword directly for packet: '%s'"))
				% params.strKeyword);
		}

		if (!typeText.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			// Type is always ascii string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, typeText);
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - I keep this here, ready if we ever allow such searches...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, complete);
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}
		#endif // 0

		// If this assert fails... we're seriously fucked up

		wxASSERT( iParameterCount == parametercount );

	} else {
		if (!params.extension.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.availability > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!typeText.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		//#warning TODO - same as above...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minLength > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!album.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!artist.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}
		#endif // 0

		// As above, if this fails, we're seriously fucked up.
		wxASSERT( iParameterCount + _SearchExpr.m_aExpr.GetCount() == parametercount );

		for (unsigned int j = 0; j < _SearchExpr.m_aExpr.GetCount(); ++j) {
			if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_AND) {
				target.WriteBooleanAND();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_OR) {
				target.WriteBooleanOR();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_NOT) {
				target.WriteBooleanNOT();
			} else {
				target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[j]);
			}
		}

		if (!params.typeText.IsEmpty()) {
			// Type is always ASCII string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, params.typeText);
		}

		if (params.minSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0) {
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()) {
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - third and last warning of the same series.
		#if 0
		if (complete > 0) {
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, pParams->uComplete);
		}

		if (minBitrate > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}

		#endif // 0
	}

	// Packet ready to go.
	return data;
}


void CSearchList::KademliaSearchKeyword(uint32_t searchID, const Kademlia::CUInt128 *fileID,
	const wxString& name, uint64_t size, const wxString& type, uint32_t kadPublishInfo, const TagPtrList& taglist)
{
	// Convert Kad search ID to original search ID for routing
	long originalSearchId = getOriginalSearchId(searchID);
	if (originalSearchId == 0) {
		AddDebugLogLineC(logSearch, CFormat(wxT("KademliaSearchKeyword: No mapping found for Kad search ID %u, result will be lost"))
			% searchID);
		return;
	}

	AddDebugLogLineC(logSearch, CFormat(wxT("KademliaSearchKeyword: Routing result from Kad ID %u to original ID %ld"))
		% searchID % originalSearchId);

	EUtf8Str eStrEncode = utf8strRaw;

	CMemFile temp(250);
	uint8_t fileid[16];
	fileID->ToByteArray(fileid);
	temp.WriteHash(CMD4Hash(fileid));

	temp.WriteUInt32(0);	// client IP
	temp.WriteUInt16(0);	// client port

	// write tag list
	unsigned int uFilePosTagCount = temp.GetPosition();
	uint32 tagcount = 0;
	temp.WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTagString tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	CTagInt64 tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	if (!type.IsEmpty()) {
		CTagString tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(&temp, eStrEncode);
		tagcount++;
	}

	// Misc tags (bitrate, etc)
	for (TagPtrList::const_iterator it = taglist.begin(); it != taglist.end(); ++it) {
		(*it)->WriteTagToFile(&temp,eStrEncode);
		tagcount++;
	}

	temp.Seek(uFilePosTagCount, wxFromStart);
	temp.WriteUInt32(tagcount);

	temp.Seek(0, wxFromStart);

	CSearchFile *tempFile = new CSearchFile(temp, (eStrEncode == utf8strRaw), originalSearchId, 0, 0, wxEmptyString, true);
	tempFile->SetKadPublishInfo(kadPublishInfo);


	// Process result through validator (this adds it to SearchList)
	NOT_ON_REMOTEGUI(
		search::SearchResultRouter::Instance().RouteResult(originalSearchId, tempFile);
	)
}

void CSearchList::UpdateSearchFileByHash(const CMD4Hash& hash)
{
	for (ResultMap::iterator it = m_results.begin(); it != m_results.end(); ++it) {
		CSearchResultList& results = it->second;
		for (size_t i = 0; i < results.size(); ++i) {
			CSearchFile* item = results.at(i);

			if (hash == item->GetFileHash()) {
				// This covers only parent items,
				// child items have to be updated separately.
				Notify_Search_Update_Sources(item);
			}
		}
	}
}


void CSearchList::SetKadSearchFinished()
{
	// Find the active Kad search
	wxMutexLocker lock(m_searchMutex);
	long searchId = -1;
	::PerSearchState* state = nullptr;

	// Find the most recent active Kad search
	for (auto it = m_searchStates.rbegin(); it != m_searchStates.rend(); ++it) {
		if (it->second && it->second->getSearchType() == KadSearch && it->second->isSearchActive()) {
			searchId = it->first;
			state = it->second.get();
			break;
		}
	}

	if (!state) {
		// No active Kad search
		return;
	}

	// Check if we have any results for the current search
	ResultMap::iterator it = m_results.find(searchId);
	bool hasResults = (it != m_results.end()) && !it->second.empty();

	// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
	// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
	// Only mark the search as finished if we have results
	if (hasResults) {
		OnSearchComplete(searchId, KadSearch, hasResults);
	} else {
		// No results - let the UI handle retry through SearchStateManager
		// Just mark the Kad search as finished internally in per-search state
		state->setKadSearchFinished(true);
		// Release the search ID since search is complete (no results)
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released Kad search ID %u (no results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release Kad search ID %u (no results) - already released?"))
				% searchId);
		}
		// Mark search as inactive
		state->setSearchActive(false);
	}
}


void CSearchList::OnSearchComplete(long searchId, SearchType type, bool hasResults)
{
	// RELIABLE RETRY FIX: Unregister search from timeout manager
	if (m_timeoutManager && m_timeoutManager->isSearchRegistered(searchId)) {
		m_timeoutManager->unregisterSearch(searchId);
		AddDebugLogLineC(logSearch, CFormat(wxT("Unregistered search %u from timeout manager (on complete)"))
			% searchId);
	}

	// Update result count
	ResultMap::iterator it = m_results.find(searchId);
	int resultCount = (it != m_results.end()) ? it->second.size() : 0;

	// Log marking search as finished
	AddDebugLogLineC(logSearch, CFormat(wxT("Marking search finished: ID=%ld, Type=%d"))
		% searchId % (int)type);

	// Get the per-search state
	::PerSearchState* state = getSearchState(searchId);
	if (state) {
		// Mark search as inactive
		state->setSearchActive(false);

		// Mark Kad search as finished
		if (type == KadSearch) {
			state->setKadSearchFinished(true);
		}
	}

	// Mark search as finished
	if (type == KadSearch) {
		// Release Kad search ID
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released Kad search ID %u (search completed with results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release Kad search ID %u (search completed) - already released?"))
				% searchId);
		}
	} else {
		Notify_SearchLocalEnd();

		// Release the search ID for non-Kad searches
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (search completed with results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (search completed) - already released?"))
				% searchId);
		}
	}

	// CRITICAL FIX: Remove search state and parameters from maps when search completes
	// This prevents duplicate detection from finding inactive searches and reusing their IDs
	// Note: ID was already released above, so pass false to avoid double-release
	AddDebugLogLineC(logSearch, CFormat(wxT("Removing search state and parameters for completed search ID=%ld"))
		% searchId);
	removeSearchState(searchId, false);
}


void CSearchList::OnSearchRetry(long searchId, SearchType type, int retryNum)
{
	// Log retry attempt
	AddDebugLogLineC(logSearch, CFormat(wxT("OnSearchRetry: SearchID=%ld, Type=%d, RetryNum=%d"))
		% searchId % (int)type % retryNum);

	// Get original parameters
	CSearchParams params = GetSearchParams(searchId);
	if (params.searchString.IsEmpty()) {
		AddDebugLogLineC(logSearch,
			CFormat(wxT("Retry %d for search %ld failed: no parameters"))
				% retryNum % searchId);
		return;
	}

	// Clean up old search state before retrying
	// Get the per-search state and mark it as inactive
	::PerSearchState* state = getSearchState(searchId);
	if (state) {
		state->setSearchActive(false);
	}
	// Remove search parameters (they will be recreated with new ID)
	m_searchParams.erase(searchId);
	// Remove per-search state
	removeSearchState(searchId);

	// Release the old search ID before retrying
	if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u before retry"))
			% searchId);
	}
	// Note: if release fails, the ID might already be released (e.g., search completed)

	// Start new search with same parameters
	uint32 newSearchId = 0;
	wxString error = StartNewSearch(&newSearchId, type, params);

	if (!error.IsEmpty()) {
		AddDebugLogLineC(logSearch,
			wxString::Format(wxT("Retry %d for search %ld failed: %s"),
				retryNum, searchId, error.c_str()));
		return;
	}


	// Move results from old search ID to new search ID
	ResultMap::iterator resultsIt = m_results.find(searchId);
	if (resultsIt != m_results.end()) {
		// Update the search ID for all results
		CSearchResultList& results = resultsIt->second;
		for (size_t i = 0; i < results.size(); ++i) {
			results[i]->SetSearchID(newSearchId);
		}
		// Move the results to the new search ID
		m_results[newSearchId] = results;
		m_results.erase(searchId);
		AddDebugLogLineC(logSearch, wxString::Format(wxT("Moved %zu results from search %ld to %ld"), results.size(), searchId, newSearchId));
	}


	// Log success
	AddDebugLogLineC(logSearch,
		wxString::Format(wxT("Retry %d started for search %ld (new ID: %u)"),
			retryNum, searchId, newSearchId));
}


// RELIABLE RETRY FIX: Handle search timeout from SearchTimeoutManager
void CSearchList::OnSearchTimeout(uint32_t searchId, SearchTimeoutType type, const wxString& reason)
{
	AddDebugLogLineC(logSearch, CFormat(wxT("OnSearchTimeout: SearchID=%u, Type=%d, Reason='%s'"))
		% searchId % (int)type % reason);

	// Get the per-search state
	::PerSearchState* state = getSearchState(searchId);
	if (!state) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: state not found, already cleaned up"))
			% searchId);
		return;
	}

	// Check if search is still active
	if (!state->isSearchActive()) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: search already inactive, ignoring"))
			% searchId);
		return;
	}

	// Check if we have results
	ResultMap::iterator it = m_results.find(searchId);
	bool hasResults = (it != m_results.end()) && !it->second.empty();

	if (hasResults) {
		// Search has results but didn't complete properly
		// Mark it as complete with results
		AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: has %zu results, marking as complete"))
			% searchId % it->second.size());

		// Convert SearchTimeoutType to SearchType
		SearchType searchType;
		switch (type) {
			case TimeoutLocalSearch:
				searchType = LocalSearch;
				break;
			case TimeoutGlobalSearch:
				searchType = GlobalSearch;
				break;
			case TimeoutKadSearch:
				searchType = KadSearch;
				break;
			default:
				searchType = LocalSearch;
				break;
		}

		OnSearchComplete(searchId, searchType, true);
	} else {
		// Search has no results and timed out
		// This is the critical case where searches get stuck in [Searching] state
		AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: no results, triggering recovery"))
			% searchId);

		// Convert SearchTimeoutType to SearchType
		SearchType searchType;
		switch (type) {
			case TimeoutLocalSearch:
				searchType = LocalSearch;
				break;
			case TimeoutGlobalSearch:
				searchType = GlobalSearch;
				break;
			case TimeoutKadSearch:
				searchType = KadSearch;
				break;
			default:
				searchType = LocalSearch;
				break;
		}

		// Check if we can retry
		int maxRetries = 3;
		int currentRetry = 0;

		if (state) {
			currentRetry = state->getKadSearchRetryCount();
		}

		if (currentRetry < maxRetries) {
			// Trigger retry
			AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: triggering retry (%d/%d)"))
				% searchId % (currentRetry + 1) % maxRetries);

			OnSearchRetry(searchId, searchType, currentRetry + 1);
		} else {
			// Max retries reached, mark as complete with no results
			AddDebugLogLineC(logSearch, CFormat(wxT("Search timeout for %u: max retries reached, marking as complete with no results"))
				% searchId);

			// Mark search as inactive
			if (state) {
				state->setSearchActive(false);
			}

			// Release the search ID
			if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (timeout, max retries reached)"))
					% searchId);
			}

			// Remove search state
			removeSearchState(searchId, false);

			// Notify that search ended with no results
			// This will trigger the UI to show [No Results] instead of [Searching]
			if (searchType == GlobalSearch) {
				Notify_SearchLocalEnd();
			}
		}
	}
}


// RELIABLE RETRY FIX: Validate and recover stuck searches
void CSearchList::ValidateAndRecoverSearches()
{
	// This method can be called periodically to validate search states
	// and recover any searches that might be stuck

	wxMutexLocker lock(m_searchMutex);

	for (auto it = m_searchStates.begin(); it != m_searchStates.end(); ) {
		long searchId = it->first;
		::PerSearchState* state = it->second.get();

		if (!state || !state->isSearchActive()) {
			// Skip inactive searches
			++it;
			continue;
		}

		// Check if search is registered with timeout manager
		if (m_timeoutManager && !m_timeoutManager->isSearchRegistered(searchId)) {
			// Search is active but not registered with timeout manager
			// This is a bug - register it now
			AddDebugLogLineC(logSearch, CFormat(wxT("Recovery: Active search %u not registered with timeout manager, registering now"))
				% searchId);

			SearchTimeoutManager::SearchType timeoutType;
			switch (state->getSearchType()) {
				case LocalSearch:
					timeoutType = SearchTimeoutManager::LocalSearch;
					break;
				case GlobalSearch:
					timeoutType = SearchTimeoutManager::GlobalSearch;
					break;
				case KadSearch:
					timeoutType = SearchTimeoutManager::KadSearch;
					break;
				default:
					timeoutType = SearchTimeoutManager::LocalSearch;
					break;
			}

			m_timeoutManager->registerSearch(searchId, timeoutType);
		}

		// Check if timer is running for ED2K searches
		uint8_t searchType = state->getSearchType();
		if (searchType == LocalSearch || searchType == GlobalSearch) {
			if (!state->isTimerRunning()) {
				// Search is active but timer is not running
				// This is a bug - restart the timer
				AddDebugLogLineC(logSearch, CFormat(wxT("Recovery: Active search %u (type=%d) timer not running, restarting"))
					% searchId % (int)searchType);

				if (searchType == LocalSearch) {
					// Restart timeout timer
					state->startTimer(30000, true);
				} else if (searchType == GlobalSearch) {
					// Restart global search timer
					state->startTimer(750, false);
				}
			}
		}

		++it;
	}
}


// File_checked_for_headers

wxString CSearchList::RequestMoreResultsForSearch(long searchId)
{
	// Request more results for the given search
	// This is a wrapper around RequestMoreResults that handles the search ID
	return RequestMoreResults(searchId);
}
