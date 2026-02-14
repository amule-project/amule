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

#include "SearchStateManager.h"
#include "search/SearchLogging.h"
#include "../Logger.h"
#include <common/Format.h>
#include <cstdint>

// Maximum number of retries
const int MAX_RETRIES = 3;

SearchStateManager::SearchStateManager()
{
}

SearchStateManager::~SearchStateManager()
{
	m_observers.clear();
	m_searches.clear();
}

void SearchStateManager::RegisterObserver(ISearchStateObserver* observer)
{
	wxMutexLocker lock(m_mutex);
	if (observer) {
		m_observers.insert(observer);
	}
}

void SearchStateManager::UnregisterObserver(ISearchStateObserver* observer)
{
	wxMutexLocker lock(m_mutex);
	if (observer) {
		m_observers.erase(observer);
	}
}

void SearchStateManager::InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword)
{
	InitializeSearch(searchId, searchType, keyword, CSearchList::CSearchParams());
}

void SearchStateManager::InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword, const CSearchList::CSearchParams& params)
{
	SearchData data;
	data.searchId = searchId;
	data.searchType = searchType;
	data.keyword = keyword;
	data.state = STATE_SEARCHING;
	data.retryCount = 0;
	data.shownCount = 0;
	data.hiddenCount = 0;
	
	// Store search parameters for retry
	data.searchString = params.searchString;
	data.strKeyword = params.strKeyword;
	data.typeText = params.typeText;
	data.extension = params.extension;
	data.minSize = params.minSize;
	data.maxSize = params.maxSize;
	data.availability = params.availability;
	
	{
		wxMutexLocker lock(m_mutex);
		m_searches[searchId] = data;
	}

	// Notify observers of the new search
	NotifyObservers(searchId, STATE_SEARCHING, 0);
}

void SearchStateManager::UpdateResultCount(uint32_t searchId, size_t shown, size_t hidden)
{
	bool needNotify = false;
	SearchState oldState = STATE_IDLE;
	int retryCount = 0;
	
	{
		wxMutexLocker lock(m_mutex);
		
		SearchMap::iterator it = m_searches.find(searchId);
		if (it == m_searches.end()) {
			return;
		}

		SearchData& data = it->second;
		data.shownCount = shown;
		data.hiddenCount = hidden;

		// Update state based on result count
		if (shown > 0 || hidden > 0) {
			// Results found - reset retry count and update state
			if (data.retryCount > 0) {
				data.retryCount = 0;
			}
			// Only update state if we were in a non-result state
			if (data.state == STATE_SEARCHING ||
				data.state == STATE_RETRYING ||
				data.state == STATE_NO_RESULTS) {
				// Update state directly without calling UpdateState to avoid double lock
				if (data.state != STATE_HAS_RESULTS) {
					oldState = data.state;
					data.state = STATE_HAS_RESULTS;
					retryCount = data.retryCount;
					needNotify = true;
				}
			}
		}
	}
	
	// Notify observers outside the lock to avoid deadlocks
	if (needNotify) {
		NotifyObservers(searchId, STATE_HAS_RESULTS, retryCount);
	}
}

void SearchStateManager::EndSearch(uint32_t searchId)
{
	bool shouldUpdateState = false;
	bool hasResults = false;
	bool shouldRetry = false;
	int retryCount = 0;
	
	{
		wxMutexLocker lock(m_mutex);
		
		SearchMap::iterator it = m_searches.find(searchId);
		if (it == m_searches.end()) {
			AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u not found"))
			 % searchId);
			return;
		}

		SearchData& data = it->second;
		retryCount = data.retryCount;

		// Determine final state based on results
		if (data.shownCount > 0 || data.hiddenCount > 0) {
			// Results found - mark as completed
			shouldUpdateState = true;
			hasResults = true;
			AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u has results (shown=%zu, hidden=%zu)"))
				% searchId % data.shownCount % data.hiddenCount);
		} else {
			// No results - check if we should retry
			if (data.retryCount < MAX_RETRIES) {
				// Update to Retrying state
				shouldUpdateState = true;
				shouldRetry = true;
				AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u has no results, will retry (count=%d/%d)"))
					% searchId % (data.retryCount + 1) % MAX_RETRIES);
			} else {
				// Max retries reached, set to NO_RESULTS
				shouldUpdateState = true;
				hasResults = false;
				AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u has no results, max retries reached"))
					% searchId);
			}
		}
	}
	
	// Update state outside the lock to avoid double lock
	if (shouldUpdateState) {
		if (hasResults) {
			UpdateState(searchId, STATE_HAS_RESULTS);
			AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u -> STATE_HAS_RESULTS"))
				% searchId);
		} else if (shouldRetry) {
			UpdateState(searchId, STATE_RETRYING);
			AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u -> STATE_RETRYING"))
				% searchId);
		} else {
			UpdateState(searchId, STATE_NO_RESULTS);
			AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u -> STATE_NO_RESULTS"))
				% searchId);
		}
	} else {
		AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::EndSearch: Search %u state not updated"))
				% searchId);
	}
}

bool SearchStateManager::RequestRetry(uint32_t searchId)
{
	bool canRetry = false;
	int newRetryCount = 0;
	
	{
		wxMutexLocker lock(m_mutex);
		
		SearchMap::iterator it = m_searches.find(searchId);
		if (it == m_searches.end()) {
			return false;
		}

		SearchData& data = it->second;

		// Check if we've reached the retry limit
		if (data.retryCount >= MAX_RETRIES) {
			return false;
		}

		// Increment retry count
		data.retryCount++;
		newRetryCount = data.retryCount;
		canRetry = true;
	}
	
	// Update state to retrying outside the lock
	if (canRetry) {
		UpdateState(searchId, STATE_RETRYING);
		// Notify observers with the new retry count
		NotifyObservers(searchId, STATE_RETRYING, newRetryCount);
	}
	
	return true;
}

SearchState SearchStateManager::GetSearchState(uint32_t searchId) const
{
	wxMutexLocker lock(m_mutex);
	
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return STATE_IDLE;
	}
	return it->second.state;
}

void SearchStateManager::StoreSearchParams(uint32_t searchId, const CSearchList::CSearchParams& params)
{
	wxMutexLocker lock(m_mutex);
	
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	SearchData& data = it->second;
	// Store all search parameters for retry
	data.searchString = params.searchString;
	data.strKeyword = params.strKeyword;
	data.typeText = params.typeText;
	data.extension = params.extension;
	data.minSize = params.minSize;
	data.maxSize = params.maxSize;
	data.availability = params.availability;
}

bool SearchStateManager::GetSearchParams(uint32_t searchId, CSearchList::CSearchParams& params) const
{
	wxMutexLocker lock(m_mutex);

	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return false;
	}

	const SearchData& data = it->second;
	// Retrieve all stored search parameters
	params.searchString = data.searchString;
	params.strKeyword = data.strKeyword;
	params.typeText = data.typeText;
	params.extension = data.extension;
	params.minSize = data.minSize;
	params.maxSize = data.maxSize;
	params.availability = data.availability;

	// Convert searchType from wxString to SearchType enum
	if (data.searchType == wxT("Local")) {
		params.searchType = LocalSearch;
	} else if (data.searchType == wxT("Global")) {
		params.searchType = GlobalSearch;
	} else if (data.searchType == wxT("Kad")) {
		params.searchType = KadSearch;
	} else {
		// Default to LocalSearch if we can't determine the type
		params.searchType = LocalSearch;
	}

	return true;
}

int SearchStateManager::GetRetryCount(uint32_t searchId) const
{
	wxMutexLocker lock(m_mutex);
	
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return 0;
	}
	return it->second.retryCount;
}

wxString SearchStateManager::GetSearchType(uint32_t searchId) const
{
	wxMutexLocker lock(m_mutex);
	
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return wxEmptyString;
	}
	return it->second.searchType;
}

wxString SearchStateManager::GetKeyword(uint32_t searchId) const
{
	wxMutexLocker lock(m_mutex);
	
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return wxEmptyString;
	}
	return it->second.keyword;
}

void SearchStateManager::GetResultCount(uint32_t searchId, size_t& shown, size_t& hidden) const
{
	wxMutexLocker lock(m_mutex);
	
	shown = 0;
	hidden = 0;

	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	shown = it->second.shownCount;
	hidden = it->second.hiddenCount;
}

bool SearchStateManager::HasSearch(uint32_t searchId) const
{
	wxMutexLocker lock(m_mutex);
	return m_searches.find(searchId) != m_searches.end();
}

void SearchStateManager::RemoveSearch(uint32_t searchId)
{
	wxMutexLocker lock(m_mutex);
	m_searches.erase(searchId);
}

void SearchStateManager::UpdateState(uint32_t searchId, SearchState newState)
{
	bool needNotify = false;
	int retryCount = 0;
	
	{
		wxMutexLocker lock(m_mutex);
		
		SearchMap::iterator it = m_searches.find(searchId);
		if (it == m_searches.end()) {
			return;
		}

		SearchData& data = it->second;

		// Only update if state is changing
		if (data.state != newState) {
			data.state = newState;
			retryCount = data.retryCount;
			needNotify = true;
		}
	}
	
	// Notify observers outside the lock to avoid deadlocks
	if (needNotify) {
		NotifyObservers(searchId, newState, retryCount);
	}
}

void SearchStateManager::NotifyObservers(uint32_t searchId, SearchState state, int retryCount)
{
	for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
		(*it)->OnSearchStateChanged(searchId, state, retryCount);
	}
}
