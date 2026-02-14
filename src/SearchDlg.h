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

#ifndef SEARCHDLG_H
#define SEARCHDLG_H

#include <wx/panel.h>		// Needed for wxPanel
#include <wx/notebook.h>	// needed for wxBookCtrlEvent in wx 2.8
#include <wx/timer.h>		// Needed for wxTimer
#include <map>			// Needed for std::map
#include <memory>		// Needed for std::unique_ptr

#include "Types.h"		// Needed for uint16 and uint32
#include "SearchList.h"		// Needed for SearchType
#include "SearchStateManager.h"	// Needed for SearchStateManager and ISearchStateObserver
#include "search/UnifiedSearchManager.h"	// Needed for UnifiedSearchManager
#include "SimpleSearchCache.h"	// Needed for SimpleSearchCache (duplicate detection)


class CMuleNotebook;
class CSearchListCtrl;
class CMuleNotebookEvent;
class wxListEvent;
class wxSpinEvent;
class wxGauge;
class CSearchFile;


/**
 * This class represents the Search Dialog, which takes care of
 * enabling the user to search and to display results in a readable
 * manner.
 */
class CSearchDlg : public wxPanel, public ISearchStateObserver
{
public:
	/**
	 * Constructor.
	 *
	 * @param pParent The parent widget passed to the wxPanel constructor.
	 */
	CSearchDlg(wxWindow* pParent);

	/**
	 * Destructor.
	 */
	~CSearchDlg();


	/**
	 * Adds the provided result to the right result-list.
	 *
	 * Please note that there is no duplicates checking, so the files should
	 * indeed be a new result.
	 */
	void AddResult(CSearchFile* toadd);

	/**
	 * Updates a changed result.
	 *
	 * @param A pointer to the updated CSearchFile.
	 *
	 * This function will update the source-count and color of the result, and
	 * if needed, it will also move the result so that the current sorting
	 * is maintained.
	 */
	void UpdateResult(CSearchFile* toupdate);

	/**
	 * Checks if a result-page with the specified heading exists.
	 *
	 * @param searchString The heading to look for.
	 */
	bool		CheckTabNameExists(SearchType searchType, const wxString& searchString);

	/**
	 * Creates a new tab and displays the specified results.
	 *
	 * @param searchString This will be the heading of the new page.
	 * @param nSearchID The results with this searchId will be displayed.
	 */
	void		CreateNewTab(const wxString& searchString, wxUIntPtr nSearchID);


	/**
	 * Call this function to signify that the global search is over.
	 */
	void		GlobalSearchEnd();

	/**
	 * Call this function to signify that the local search is over.
	 */
	void		LocalSearchEnd();

	/**
	 * Call this function to signify that the kad search is over.
	 */
	void		KadSearchEnd(uint32 id);

	/**
	 * Updates the hit count display for the given search list control.
	 */
	void		UpdateHitCount(CSearchListCtrl* list);

	/**
	 * Updates the tab label with state information for the given search list control.
	 */
	void		UpdateTabLabelWithState(CSearchListCtrl* list, const wxString& state);

	/**
	 * Implementation of ISearchStateObserver interface.
	 * Called when the search state changes.
	 *
	 * @param searchId The search ID
	 * @param state The new search state
	 * @param retryCount The current retry count
	 */
	void		OnSearchStateChanged(uint32_t searchId, SearchState state, int retryCount);

	/**
	 * Implementation of ISearchStateObserver interface.
	 * Called when a retry is requested for a search.
	 *
	 * @param searchId The search ID to retry
	 * @return true if the retry was initiated, false otherwise
	 */
	bool		OnRetryRequested(uint32_t searchId);

	/**
	 * Gets the notebook widget containing search result tabs.
	 */
	CMuleNotebook* GetNotebook() const { return m_notebook; }

	/**
	 * Gets the search state manager.
	 */
	SearchStateManager& GetStateManager() { return m_stateManager; }

	/**
	 * Gets the unified search manager.
	 */
	search::UnifiedSearchManager& GetUnifiedSearchManager() { return m_unifiedSearchManager; }

	/**
	 * Updates the enabled state of the start button based on connection status.
	 */
	void		UpdateStartButtonState();

	/**
	 * This function updates the category list according to existing categories.
	 */
	void		UpdateCatChoice();

	/**
	 * Helper function which resets the controls.
	 */
	void		ResetControls();

	/**
	 * Helper function to get the search list control for a given ID.
	 */
	CSearchListCtrl* GetSearchList(wxUIntPtr id);

	// Event handler and helper function
	void		OnBnClickedDownload(wxCommandEvent& event);
	void		OnBnClickedReset(wxCommandEvent& event);
	void		OnBnClickedClear(wxCommandEvent& event);
	void		OnBnClickedMore(wxCommandEvent& event);

	/**
	 * Updates the progress bar.
	 */
	void	UpdateProgress(uint32 new_value);

	void	StartNewSearch();

	void FixSearchTypes();

private:
	// Event handlers
	void		OnFieldChanged(wxEvent& evt);

	void		OnListItemSelected(wxListEvent& ev);
	void		OnExtendedSearchChange(wxCommandEvent& ev);
	void		OnFilterCheckChange(wxCommandEvent& ev);
	void		OnFilteringChange(wxCommandEvent& ev);

	void		OnSearchClosing(wxBookCtrlEvent& evt);

	void		OnBnClickedStart(wxCommandEvent& evt);
	void		OnBnClickedStop(wxCommandEvent& evt);

	/**
	 * Called when the search type selection changes.
	 */
	void		OnSearchTypeChanged(wxCommandEvent& evt);

	/**
	 * Event-handler for page-changes which takes care of enabling/disabling the download button.
	 */
	void		OnSearchPageChanged(wxBookCtrlEvent& evt);

	uint32		m_last_search_time;

	wxGauge*	m_progressbar;

	CMuleNotebook*	m_notebook;

	wxArrayString m_searchchoices;

	// Timer for checking "More" button timeouts
	wxTimer		m_timeoutCheckTimer;

	// Track active "More" button searches by search ID
	std::map<uint32, wxDateTime> m_moreButtonSearches;

	// Store original tab texts for "More" button searches by search ID
	std::map<uint32, wxString> m_originalTabTexts;

	// Handle timeout checks
	void		OnTimeoutCheck(wxTimerEvent& event);

	// Handle "More" button timeout
	void		HandleMoreButtonTimeout(uint32 searchId);

	// Search state manager
	SearchStateManager			m_stateManager;

	// Unified search manager - common abstraction layer for all search types
	search::UnifiedSearchManager	m_unifiedSearchManager;

	// Simple search cache - handles duplicate search detection
	SimpleSearchCache			m_searchCache;

	// Mutex to protect UI updates from concurrent network callbacks
	mutable wxMutex m_uiUpdateMutex;

	// Mutex to protect search creation and prevent race conditions
	mutable wxMutex m_searchCreationMutex;

	DECLARE_EVENT_TABLE()
};

#endif
// File_checked_for_headers
