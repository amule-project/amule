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

#include "SearchControllerBase.h"
#include "SearchLogging.h"
#include "../SearchFile.h"
#include "../Logger.h"
#include "../amule.h"
#include "../SearchList.h"

namespace search {

SearchControllerBase::SearchControllerBase()
    : m_model(std::make_unique<SearchModel>())
    , m_retryCount(DEFAULT_RETRY_COUNT)
    , m_currentRetry(0)
{
}

SearchControllerBase::~SearchControllerBase()
{
    // Don't clear results here - the legacy system (CSearchList) manages the lifetime
    // of CSearchFile objects. Clearing them here would cause a double-free.
}

void SearchControllerBase::handleSearchError(uint32_t searchId, const wxString& error)
{
    m_model->setSearchState(SearchState::Error);
    notifyError(searchId, error);
}

void SearchControllerBase::resetSearchState()
{
    // Reset retry counter - derived classes can override to reset additional state
    m_currentRetry = 0;
}

void SearchControllerBase::stopSearchBase()
{
    // Update state
    m_model->setSearchState(SearchState::Idle);
    resetSearchState();

    // Notify completion
    uint32_t searchId = m_model->getSearchId();
    notifySearchCompleted(searchId);
}

bool SearchControllerBase::validatePrerequisites()
{
    // Always return true - no external dependencies
    return true;
}

bool SearchControllerBase::validateSearchParams(const SearchParams& params)
{
    // Combine validation checks for efficiency
    if (!params.isValid() || params.searchString.IsEmpty()) {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, params.searchString.IsEmpty() 
	    ? _("Search string cannot be empty")
	    : _("Invalid search parameters"));
	return false;
    }

    return true;
}

bool SearchControllerBase::validateRetryLimit(wxString& error) const
{
    if (m_currentRetry >= m_retryCount) {
	error = _("Maximum retry limit reached");
	return false;
    }
    return true;
}

void SearchControllerBase::updateSearchState(const SearchParams& params, uint32_t searchId, SearchState state)
{
    m_model->setSearchParams(params);
    m_model->setSearchId(searchId);
    m_model->setSearchState(state);
}

SearchState SearchControllerBase::getState() const
{
    return m_model->getSearchState();
}

SearchParams SearchControllerBase::getSearchParams() const
{
    return m_model->getSearchParams();
}

long SearchControllerBase::getSearchId() const
{
    return m_model->getSearchId();
}

std::vector<CSearchFile*> SearchControllerBase::getResults() const
{
    return m_model->getResults();
}

size_t SearchControllerBase::getResultCount() const
{
    return m_model->getResultCount();
}

void SearchControllerBase::handleResult(uint32_t searchId, CSearchFile* result)
{
    // Only handle results for our search
    if (searchId != static_cast<uint32_t>(m_model->getSearchId())) {
	return;
    }

    // Add result to model (handles duplicates internally)
    m_model->addResult(result);

    // Also add result to legacy SearchList for UI display
    // The SearchListCtrl::ShowResults() method retrieves results from SearchList
    CSearchFile* resultForUI = result;
    if (theApp && theApp->searchlist) {
	// Create a copy for SearchList (SearchModel owns the original)
	CSearchFile* resultCopy = new CSearchFile(*result);
	resultCopy->SetSearchID(searchId);
	theApp->searchlist->AddToList(resultCopy, false);
	// Use the copy for UI notification to ensure consistency
	resultForUI = resultCopy;
    }

    // Notify about new result
    std::vector<CSearchFile*> results;
    results.push_back(resultForUI);
    notifyResultsReceived(searchId, results);
}

void SearchControllerBase::handleResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    // Only handle results for our search
    if (searchId != static_cast<uint32_t>(m_model->getSearchId())) {
	return;
    }

    // Add all results to model (handles duplicates internally)
    m_model->addResults(results);

    // Also add results to legacy SearchList for UI display
    // The SearchListCtrl::ShowResults() method retrieves results from SearchList
    std::vector<CSearchFile*> resultsForUI;
    if (theApp && theApp->searchlist) {
	for (CSearchFile* result : results) {
	    // Create a copy for SearchList (SearchModel owns the original)
	    CSearchFile* resultCopy = new CSearchFile(*result);
	    resultCopy->SetSearchID(searchId);
	    theApp->searchlist->AddToList(resultCopy, false);
	    // Use the copy for UI notification to ensure consistency
	    resultsForUI.push_back(resultCopy);
	}
    } else {
	// If SearchList is not available, use the original results
	resultsForUI = results;
    }

    // Notify about new results
    notifyResultsReceived(searchId, resultsForUI);
}

bool SearchControllerBase::handlesSearch(uint32_t searchId) const
{
    return searchId == static_cast<uint32_t>(m_model->getSearchId());
}

void SearchControllerBase::updateSearchId(uint32_t newSearchId)
{
    m_model->setSearchId(newSearchId);
    SEARCH_DEBUG(wxString::Format(wxT("Updated controller search ID to %u"), newSearchId));
}

bool SearchControllerBase::validateConfiguration() const
{
    if (m_retryCount < 0) {
	return false;
    }
    return true;
}

} // namespace search
