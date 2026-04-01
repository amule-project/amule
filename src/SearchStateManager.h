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

#ifndef SEARCHSTATEMANAGER_H
#define SEARCHSTATEMANAGER_H

#include <wx/string.h>
#include <wx/thread.h>
#include <map>
#include <set>
#include <cstdint>

// Forward declarations
class CSearchList;

// Include SearchList.h to access CSearchParams
#include "SearchList.h"
class CSearchListCtrl;
class CSearchDlg;

/**
 * Search state enumeration
 */
enum SearchState {
	STATE_IDLE,         // Search not started
	STATE_SEARCHING,    // Search in progress
	STATE_POPULATING,   // Results are being populated
	STATE_RETRYING,     // Search is being retried
	STATE_NO_RESULTS,   // Search completed with no results
	STATE_HAS_RESULTS   // Search has results
};

/**
 * Search state observer interface
 *
 * Classes that want to be notified of search state changes
 * should implement this interface.
 */
class ISearchStateObserver {
public:
	virtual ~ISearchStateObserver() {}

	/**
	 * Called when the search state changes
	 *
	 * @param searchId The search ID
	 * @param state The new search state
	 * @param retryCount The current retry count (0 if not retrying)
	 */
	virtual void OnSearchStateChanged(uint32_t searchId, SearchState state, int retryCount) = 0;

	/**
	 * Called when a retry is requested for a search
	 *
	 * @param searchId The search ID to retry
	 * @return true if the retry was initiated, false otherwise
	 */
	virtual bool OnRetryRequested(uint32_t searchId) = 0;
};

/**
 * Search state manager
 *
 * This class manages the state of search tabs atomically.
 * It provides a single interface for updating state and
 * notifies observers of state changes.
 */
class SearchStateManager {
public:
	/**
	 * Constructor
	 */
	SearchStateManager();

	/**
	 * Destructor
	 */
	~SearchStateManager();

	/**
	 * Register an observer for search state changes
	 *
	 * @param observer The observer to register
	 */
	void RegisterObserver(ISearchStateObserver* observer);

	/**
	 * Unregister an observer
	 *
	 * @param observer The observer to unregister
	 */
	void UnregisterObserver(ISearchStateObserver* observer);

	/**
	 * Initialize a new search
	 *
	 * @param searchId The search ID
	 * @param searchType The search type (e.g., "Local", "Global", "Kad")
	 * @param keyword The search keyword
	 */
	void InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword);

	/**
	 * Initialize a new search with parameters
	 *
	 * @param searchId The search ID
	 * @param searchType The search type (e.g., "Local", "Global", "Kad")
	 * @param keyword The search keyword
	 * @param params The search parameters to store
	 */
	void InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword, const CSearchList::CSearchParams& params);

	/**
	 * Update the result count for a search
	 *
	 * @param searchId The search ID
	 * @param shown The number of shown results
	 * @param hidden The number of hidden results
	 */
	void UpdateResultCount(uint32_t searchId, size_t shown, size_t hidden);

	/**
	 * End a search
	 *
	 * @param searchId The search ID
	 */
	void EndSearch(uint32_t searchId);

	/**
	 * Request a retry for a search
	 *
	 * @param searchId The search ID to retry
	 * @return true if the retry was initiated, false otherwise
	 */
	bool RequestRetry(uint32_t searchId);

	/**
	 * Get the search state
	 *
	 * @param searchId The search ID
	 * @return The search state
	 */
	SearchState GetSearchState(uint32_t searchId) const;

	/**
	 * Get the retry count for a search
	 *
	 * @param searchId The search ID
	 * @return The retry count
	 */
	int GetRetryCount(uint32_t searchId) const;

	/**
	 * Get the search type for a search
	 *
	 * @param searchId The search ID
	 * @return The search type
	 */
	wxString GetSearchType(uint32_t searchId) const;

	/**
	 * Get the keyword for a search
	 *
	 * @param searchId The search ID
	 * @return The keyword
	 */
	wxString GetKeyword(uint32_t searchId) const;

	/**
	 * Get the result count for a search
	 *
	 * @param searchId The search ID
	 * @param shown Output parameter for shown results
	 * @param hidden Output parameter for hidden results
	 */
	void GetResultCount(uint32_t searchId, size_t& shown, size_t& hidden) const;

	/**
	 * Check if a search exists
	 *
	 * @param searchId The search ID
	 * @return true if the search exists, false otherwise
	 */
	bool HasSearch(uint32_t searchId) const;

	/**
	 * Remove a search
	 *
	 * @param searchId The search ID
	 */
	void RemoveSearch(uint32_t searchId);

	/**
	 * Update the state of a search
	 *
	 * @param searchId The search ID
	 * @param newState The new state
	 */
	void UpdateState(uint32_t searchId, SearchState newState);

	/**
	 * Store search parameters for retry
	 *
	 * @param searchId The search ID
	 * @param params The search parameters to store
	 */
	void StoreSearchParams(uint32_t searchId, const CSearchList::CSearchParams& params);

	/**
	 * Get stored search parameters for retry
	 *
	 * @param searchId The search ID
	 * @param params Output parameter for search parameters
	 * @return true if parameters were found, false otherwise
	 */
	bool GetSearchParams(uint32_t searchId, CSearchList::CSearchParams& params) const;

private:
	/**
	 * Search data structure
	 */
	struct SearchData {
		uint32_t searchId;
		wxString searchType;
		wxString keyword;
		SearchState state;
		int retryCount;
		size_t shownCount;
		size_t hiddenCount;
		
		// Store search parameters for retry
		wxString searchString;
		wxString strKeyword;
		wxString typeText;
		wxString extension;
		uint64_t minSize;
		uint64_t maxSize;
		uint32_t availability;

		SearchData()
			: searchId(0)
			, state(STATE_IDLE)
			, retryCount(0)
			, shownCount(0)
			, hiddenCount(0)
			, minSize(0)
			, maxSize(0)
			, availability(0)
		{}
	};

	/**
	 * Notify observers of a state change
	 *
	 * @param searchId The search ID
	 * @param state The new state
	 * @param retryCount The retry count
	 */
	void NotifyObservers(uint32_t searchId, SearchState state, int retryCount);

	// Map of search ID to search data
	typedef std::map<uint32_t, SearchData> SearchMap;
	SearchMap m_searches;

	// Set of observers
	typedef std::set<ISearchStateObserver*> ObserverSet;
	ObserverSet m_observers;

	// Mutex for thread-safe access
	mutable wxMutex m_mutex;
};

#endif // SEARCHSTATEMANAGER_H
