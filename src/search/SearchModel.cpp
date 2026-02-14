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

#include "SearchModel.h"
#include "../SearchFile.h" // For CSearchFile
#include "../MD4Hash.h" // For CMD4Hash
#include "../Logger.h"
#include <common/Format.h>

namespace search {

SearchModel::SearchModel()
    : m_state(SearchState::Idle)
    , m_searchId(-1)
    , m_filterInvert(false)
    , m_filterKnownOnly(false)
{
}

SearchModel::~SearchModel()
{
    clearResults();
}

void SearchModel::setSearchParams(const SearchParams& params)
{
    wxMutexLocker lock(m_mutex);
    m_params = params;
}

SearchParams SearchModel::getSearchParams() const
{
    wxMutexLocker lock(m_mutex);
    return m_params;
}

SearchParams SearchModel::getSearchParamsThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_params;
}

void SearchModel::addResult(CSearchFile* result)
{
    wxMutexLocker lock(m_mutex);
    // Check for duplicates before adding
    if (!isDuplicate(result)) {
        m_results.push_back(std::unique_ptr<CSearchFile>(result));
    } else {
        // Duplicate found, delete the result
        delete result;
    }
}

void SearchModel::addResults(const std::vector<CSearchFile*>& results)
{
    wxMutexLocker lock(m_mutex);
    for (CSearchFile* result : results) {
        // Check for duplicates before adding
        if (!isDuplicate(result)) {
            m_results.push_back(std::unique_ptr<CSearchFile>(result));
        } else {
            // Duplicate found, delete the result
            delete result;
        }
    }
}

bool SearchModel::isDuplicate(const CSearchFile* result) const
{
    wxMutexLocker lock(m_mutex);
    for (const auto& existing : m_results) {
        // Check by hash and size (primary duplicate detection)
        if (result->GetFileHash() == existing->GetFileHash() &&
            result->GetFileSize() == existing->GetFileSize()) {
            return true;
        }
    }
    return false;
}

void SearchModel::clearResults()
{
    wxMutexLocker lock(m_mutex);
    m_results.clear();
}

size_t SearchModel::getResultCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_results.size();
}

std::vector<CSearchFile*> SearchModel::getResults() const
{
    wxMutexLocker lock(m_mutex);
    // Convert unique_ptr to raw pointers for compatibility
    std::vector<CSearchFile*> results;
    results.reserve(m_results.size());
    for (const auto& result : m_results) {
	results.push_back(result.get());
    }
    return results;
}

bool SearchModel::hasResults() const
{
    wxMutexLocker lock(m_mutex);
    return !m_results.empty();
}

void SearchModel::setSearchState(SearchState state)
{
    wxMutexLocker lock(m_mutex);
    m_state = state;
}

SearchState SearchModel::getSearchState() const
{
    wxMutexLocker lock(m_mutex);
    return m_state;
}

SearchState SearchModel::getSearchStateThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_state;
}

void SearchModel::setSearchId(long searchId)
{
    wxMutexLocker lock(m_mutex);
    m_searchId = searchId;
}

long SearchModel::getSearchId() const
{
    wxMutexLocker lock(m_mutex);
    return m_searchId;
}

long SearchModel::getSearchIdThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_searchId;
}

// New methods for enhanced result management

CSearchFile* SearchModel::getResultByIndex(size_t index) const
{
    wxMutexLocker lock(m_mutex);
    if (index >= m_results.size()) {
        return nullptr;
    }
    return m_results[index].get();
}

CSearchFile* SearchModel::getResultByHash(const CMD4Hash& hash) const
{
    wxMutexLocker lock(m_mutex);
    for (const auto& result : m_results) {
        if (result->GetFileHash() == hash) {
            return result.get();
        }
    }
    return nullptr;
}

std::vector<CSearchFile*> SearchModel::findResultsByString(const wxString& searchString) const
{
    wxMutexLocker lock(m_mutex);
    std::vector<CSearchFile*> results;

    wxString lowerSearch = searchString.Lower();
    for (const auto& result : m_results) {
        wxString fileName = result->GetFileName().GetPrintable().Lower();
        if (fileName.Contains(lowerSearch)) {
            results.push_back(result.get());
        }
    }

    return results;
}

size_t SearchModel::getShownResultCount() const
{
    wxMutexLocker lock(m_mutex);
    if (m_filterString.IsEmpty()) {
        return m_results.size();
    }

    size_t count = 0;
    for (const auto& result : m_results) {
        if (matchesFilter(result.get())) {
            count++;
        }
    }
    return count;
}

size_t SearchModel::getHiddenResultCount() const
{
    wxMutexLocker lock(m_mutex);
    if (m_filterString.IsEmpty()) {
        return 0;
    }

    size_t count = 0;
    for (const auto& result : m_results) {
        if (!matchesFilter(result.get())) {
            count++;
        }
    }
    return count;
}

void SearchModel::filterResults(const wxString& filter, bool invert, bool knownOnly)
{
    wxMutexLocker lock(m_mutex);
    m_filterString = filter;
    m_filterInvert = invert;
    m_filterKnownOnly = knownOnly;
}

void SearchModel::clearFilters()
{
    wxMutexLocker lock(m_mutex);
    m_filterString.Clear();
    m_filterInvert = false;
    m_filterKnownOnly = false;
}

bool SearchModel::matchesFilter(const CSearchFile* result) const
{
    // If no filter is set, all results match
    if (m_filterString.IsEmpty()) {
        return true;
    }

    // Check known-only filter
    if (m_filterKnownOnly) {
        // Check if file is known (shared or downloading)
        // This would need to check against known files list
        // For now, we'll implement a basic check
        // TODO: Implement proper known files check
    }

    // Check string filter
    bool matches = false;
    wxString fileName = result->GetFileName().GetPrintable().Lower();

    try {
        wxRegEx regex(m_filterString, wxRE_DEFAULT | wxRE_ICASE);
        if (regex.IsValid()) {
            matches = regex.Matches(fileName);
        } else {
            // If regex is invalid, treat as simple substring match
            matches = fileName.Contains(m_filterString.Lower());
        }
    } catch (...) {
        // If regex fails, treat as simple substring match
        matches = fileName.Contains(m_filterString.Lower());
    }

    // Apply invert if needed
    if (m_filterInvert) {
        matches = !matches;
    }

    return matches;
}

} // namespace search
