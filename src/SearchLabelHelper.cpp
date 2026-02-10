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

#include "SearchLabelHelper.h"
#include "SearchListCtrl.h"
#include "SearchDlg.h"
#include "SearchStateManager.h"
#include "MuleNotebook.h"
#include <common/Format.h>
#include "amule.h"
#include "SearchList.h"
#include "Logger.h"
#include "search/SearchLogging.h"
#include <cassert>

void UpdateSearchState(CSearchListCtrl* list, CSearchDlg* parentDlg, const wxString& state)
{
	if (!list || !parentDlg) {
		return;
	}

	// Get the result counts from the list
	size_t shown = list->GetItemCount();
	size_t hidden = list->GetHiddenItemCount();

	// Call the version that takes counts
	UpdateSearchStateWithCount(list, parentDlg, state, shown, hidden);
}

void UpdateSearchStateWithCount(CSearchListCtrl* list, CSearchDlg* parentDlg, const wxString& state, size_t shown, size_t hidden)
{
	// Check for null pointers before proceeding
	if (!list || !parentDlg) {
		return;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return;
	}

	// Get the search ID from the list
	long searchId = list->GetSearchId();
	if (searchId == 0) {
		return;
	}

	// Get the SearchStateManager
	SearchStateManager& stateManager = parentDlg->GetStateManager();
	if (!stateManager.HasSearch(searchId)) {
		return;
	}

	// Get the search type and keyword from SearchStateManager
	wxString searchType = stateManager.GetSearchType(searchId);
	wxString keyword = stateManager.GetKeyword(searchId);

	// Log the values for debugging
	SEARCH_DEBUG_LABEL(CFormat(wxT("UpdateSearchStateWithCount: searchId=%ld, state='%s', shown=%u, hidden=%u, type='%s', keyword='%s'")) % searchId % state % shown % hidden % searchType % keyword);

	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == list) {
			// Validate search type before updating
			wxString storedSearchType = list->GetSearchType();
			if (storedSearchType.IsEmpty()) {
				// Search type not set yet (old tab or bug), set it now
				list->SetSearchType(searchType);
				storedSearchType = searchType;
			} else if (storedSearchType != searchType) {
				// Search type mismatch! This should not happen.
				// Log error but continue to update to avoid leaving tab in inconsistent state
				SEARCH_DEBUG_LABEL(CFormat(wxT("UpdateSearchStateWithCount: WARNING - Search type mismatch! Stored='%s', Expected='%s', searchId=%ld")) % storedSearchType % searchType % searchId);
				// We'll still update, but this indicates a bug elsewhere
			}
			
			// Build the new tab text using data from SearchStateManager
			wxString newText;
			
			// Add search type prefix
			if (searchType == wxT("Local")) {
				newText = wxT("[Local] ");
			} else if (searchType == wxT("Global")) {
				newText = wxT("[Global] ");
			} else if (searchType == wxT("Kad")) {
				newText = wxT("[Kad] ");
			}
			
			// Add state if provided
			if (!state.IsEmpty()) {
				newText += wxT("[") + state + wxT("] ");
			}
			
			// Add the keyword
			newText += keyword;
			
			// Add count information
			// Always show count when there is a state (e.g., "No Results", "Retrying 1")
			// or when there are actual results
			if (!state.IsEmpty() || shown > 0 || hidden > 0) {
				if (hidden) {
					newText += (CFormat(wxT(" (%u/%u)")) % shown % (shown + hidden)).GetString();
				} else {
					newText += (CFormat(wxT(" (%u)")) % shown).GetString();
				}
			}

			// Log the final tab text for debugging
			SEARCH_DEBUG_LABEL(CFormat(wxT("UpdateSearchStateWithCount: Setting tab text to '%s'")) % newText);

			notebook->SetPageText(i, newText);
			break;
		}
	}
}

void UpdateHitCountWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	// Check for null pointers before proceeding
	if (!page || !parentDlg) {
		return;
	}

	// Determine the search state based on result count
	size_t shown = page->GetItemCount();
	size_t hidden = page->GetHiddenItemCount();

	// Validate counts - hidden should not exceed shown
	assert(shown >= hidden);

	wxString stateStr;
	if (shown == 0 && hidden == 0) {
		// No results yet - check if search is in progress
		long searchId = page->GetSearchId();
		if (searchId != 0) {
			// Search is active but no results yet
			stateStr = wxT("Searching");
		} else {
			// Search completed with no results
			stateStr = wxT("No Results");
		}
	} else {
		// Results are being populated or already populated
		// Clear state when results are shown - this removes [Searching], [Retrying N], etc.
		// Even if the search is still active, we clear the state when results are present
		// Note: SearchStateManager automatically resets retry count when results are found
		stateStr = wxEmptyString;
	}

	// Update the tab label with state information
	UpdateSearchState(page, parentDlg, stateStr);
}

bool RetrySearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	// Check for null pointers before proceeding
	if (!page || !parentDlg) {
		return false;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return false;
	}

	// Find the page index
	int pageIndex = -1;
	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == page) {
			pageIndex = i;
			break;
		}
	}

	// Check if page was found in notebook
	if (pageIndex == -1) {
		return false;
	}

	// Get the search ID and tab text
	long searchId = page->GetSearchId();
	assert(searchId > 0);

	wxString tabText = notebook->GetPageText(pageIndex);
	assert(!tabText.IsEmpty());

	// Determine search type from tab text
	bool isKadSearch = tabText.StartsWith(wxT("[Kad]")) || tabText.StartsWith(wxT("!"));
	bool isEd2kSearch = tabText.StartsWith(wxT("[Local] ")) || tabText.StartsWith(wxT("[Global] "));

	// Only retry ED2K searches (Local/Global), not Kad searches
	if (!isEd2kSearch) {
		return false;
	}

	// Check retry limit (max 3 retries)
	const int MAX_RETRIES = 3;
	int retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	if (retryCount >= MAX_RETRIES) {
		// Maximum retries reached - show final state
		UpdateSearchState(page, parentDlg, wxT("No Results"));
		return false;
	}

	// Start retry - this increments the retry counter
	if (!parentDlg->GetStateManager().RequestRetry(searchId)) {
		// Failed to start retry
		return false;
	}

	// Update state to show retry is in progress with count
	retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	wxString retryState = CFormat(wxT("Retrying %d")) % retryCount;
	UpdateSearchState(page, parentDlg, retryState);

	// Get the search parameters for this search
	// clang-format off
	CSearchList::CSearchParams params;
	// clang-format on
	if (!parentDlg->GetStateManager().GetSearchParams(searchId, params)) {
		// No search parameters available - cannot retry
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Start a new ED2K search with the same parameters
	// Use the same search ID to keep results in the same tab
	uint32 newSearchId = searchId;
	// Determine if this is a Local or Global search
	SearchType searchType = LocalSearch;
	if (tabText.StartsWith(wxT("[Global] "))) {
		searchType = GlobalSearch;
	}
	
	wxString error = theApp->searchlist->StartNewSearch(&newSearchId, searchType, params);

	if (!error.IsEmpty()) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}


// Update the page to show results from the new search
	page->ShowResults(newSearchId);



		


	// Retry initiated successfully - state will be updated to "Searching"
	// when the search starts
	return true;
}

bool RetryKadSearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	assert(page != nullptr);
	assert(parentDlg != nullptr);

	if (!page || !parentDlg) {
		return false;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	assert(notebook != nullptr);

	if (!notebook) {
		return false;
	}

	// Find the page index
	int pageIndex = -1;
	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == page) {
		pageIndex = i;
		break;
		}
	}

	assert(pageIndex != -1);

	if (pageIndex == -1) {
		return false;
	}

	// Get the search ID and tab text
	long searchId = page->GetSearchId();
	assert(searchId > 0);

	wxString tabText = notebook->GetPageText(pageIndex);
	assert(!tabText.IsEmpty());

	// Determine search type from tab text
	// Check for [Kad] prefix or ! prefix (which indicates Kad search in progress)
	// Also check if the tab text contains [Kad] anywhere (it might be after state info)
	bool isKadSearch = tabText.StartsWith(wxT("[Kad]")) || 
					   tabText.StartsWith(wxT("!")) ||
					   tabText.Contains(wxT("[Kad]"));

	// Only retry Kad searches
	if (!isKadSearch) {
		return false;
	}

	// Check retry limit (max 3 retries)
	const int MAX_RETRIES = 3;
	int retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	if (retryCount >= MAX_RETRIES) {
		// Maximum retries reached - show final state
		UpdateSearchState(page, parentDlg, wxT("No Results"));
		return false;
	}

	// Start retry - this increments the retry counter
	if (!parentDlg->GetStateManager().RequestRetry(searchId)) {
		// Failed to start retry
		return false;
	}

	// Update state to show retry is in progress with count
	retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	wxString retryState = CFormat(wxT("Retrying %d")) % retryCount;
	UpdateSearchState(page, parentDlg, retryState);

	// Get the search parameters for this search
	// clang-format off
	CSearchList::CSearchParams params;
	// clang-format on
	if (!parentDlg->GetStateManager().GetSearchParams(searchId, params)) {
		// No search parameters available - cannot retry
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Start a new Kad search with the same parameters
	// Use the same search ID to keep results in the same tab
	uint32 newSearchId = searchId;
	wxString error = theApp->searchlist->StartNewSearch(&newSearchId, KadSearch, params);

	if (!error.IsEmpty()) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Update the page to show results from the new search
	page->ShowResults(newSearchId);

	// Retry initiated successfully - state will be updated to "Searching"
	// when the search starts
	return true;
}
