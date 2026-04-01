//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
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

#include "LocalSearchEngine.h"
#include "../../../../amule.h"
#include "../../../../SharedFileList.h"
#include "../../../../KnownFile.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace search {

LocalSearchEngine::LocalSearchEngine()
{
    m_statistics.startTime = std::chrono::system_clock::now();
    std::cout << "[LocalSearchEngine] Initialized" << std::endl;
}

LocalSearchEngine::~LocalSearchEngine()
{
    Shutdown();
}

SearchId LocalSearchEngine::StartSearch(const SearchParams& params)
{
    SearchId searchId = SearchId::Generate();

    std::cout << "[LocalSearchEngine] Starting search " << searchId.ToString()
              << " for query: " << params.query << std::endl;

    // Create search data
    SearchData search;
    search.params = params;
    search.state = SearchState::RUNNING;
    search.startTime = std::chrono::system_clock::now();
    search.lastUpdateTime = search.startTime;

    // Perform the search
    PerformSearch(searchId, params);

    // Store search data
    m_searches[searchId] = search;

    // Update statistics
    m_statistics.totalSearches++;
    m_statistics.activeSearches++;

    return searchId;
}

void LocalSearchEngine::StopSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        it->second.state = SearchState::CANCELLED;
        m_statistics.activeSearches--;
        std::cout << "[LocalSearchEngine] Stopped search " << searchId.ToString() << std::endl;
    }
}

void LocalSearchEngine::PauseSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end() && it->second.state == SearchState::RUNNING) {
        it->second.state = SearchState::PAUSED;
        std::cout << "[LocalSearchEngine] Paused search " << searchId.ToString() << std::endl;
    }
}

void LocalSearchEngine::ResumeSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end() && it->second.state == SearchState::PAUSED) {
        it->second.state = SearchState::RUNNING;
        std::cout << "[LocalSearchEngine] Resumed search " << searchId.ToString() << std::endl;
    }
}

void LocalSearchEngine::RequestMoreResults(SearchId searchId)
{
    // Local searches complete immediately, so no more results to request
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        std::cout << "[LocalSearchEngine] Cannot request more results for local search "
                  << searchId.ToString() << " (search already complete)" << std::endl;
    }
}

SearchState LocalSearchEngine::GetSearchState(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.state;
    }
    return SearchState::IDLE;
}

SearchParams LocalSearchEngine::GetSearchParams(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.params;
    }
    return SearchParams{};
}

std::vector<SearchResult> LocalSearchEngine::GetResults(SearchId searchId, size_t maxResults) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        if (maxResults == 0 || maxResults >= it->second.results.size()) {
            return it->second.results;
        } else {
            return std::vector<SearchResult>(
                it->second.results.begin(),
                it->second.results.begin() + maxResults
            );
        }
    }
    return {};
}

size_t LocalSearchEngine::GetResultCount(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.results.size();
    }
    return 0;
}

void LocalSearchEngine::ProcessCommand(const SearchCommand& command)
{
    // Local search engine doesn't need special command processing
    // All operations are handled through ISearchEngine interface
}

void LocalSearchEngine::ProcessMaintenance(std::chrono::system_clock::time_point currentTime)
{
    // Check for completed searches and update statistics
    for (auto& [searchId, search] : m_searches) {
        if (search.state == SearchState::RUNNING) {
            // Local searches complete immediately, mark as completed
            search.state = SearchState::COMPLETED;
            m_statistics.completedSearches++;
            m_statistics.activeSearches--;
        }
    }

    // Clean up old searches (older than 1 hour)
    auto cutoff = currentTime - std::chrono::hours(1);
    for (auto it = m_searches.begin(); it != m_searches.end(); ) {
        if (it->second.lastUpdateTime < cutoff &&
            (it->second.state == SearchState::COMPLETED ||
             it->second.state == SearchState::FAILED ||
             it->second.state == SearchState::CANCELLED)) {
            it = m_searches.erase(it);
        } else {
            ++it;
        }
    }
}

void LocalSearchEngine::Shutdown()
{
    std::cout << "[LocalSearchEngine] Shutting down" << std::endl;
    m_searches.clear();
}

LocalSearchEngine::EngineStatistics LocalSearchEngine::GetStatistics() const
{
    return m_statistics;
}

void LocalSearchEngine::PerformSearch(SearchId searchId, const SearchParams& params)
{
    std::cout << "[LocalSearchEngine] Performing local search for: " << params.query << std::endl;

    // Get shared files list
    if (!theApp->sharedfiles) {
        std::cerr << "[LocalSearchEngine] Shared files list not available" << std::endl;
        return;
    }

    std::vector<SearchResult> results;

    // Iterate through shared files
    const CKnownFileList* sharedFiles = theApp->sharedfiles;
    for (const CKnownFile* file : *sharedFiles) {
        if (!file) {
            continue;
        }

        // Create search result from file
        SearchResult result;
        result.searchId = searchId;
        result.sourceType = SearchType::LOCAL;
        result.fileName = file->GetFileName().ToStdString();
        result.fileSize = file->GetFileSize();
        result.fileHash = file->GetFileHash().Encode().ToStdString();
        result.availability = 1; // Local file always available

        // Add metadata
        if (file->GetMetaDataVer() > 0) {
            // Add file type
            std::string fileType = GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName()));
            if (!fileType.empty()) {
                result.fileType = fileType;
            }

            // Add other metadata if available
            for (const CTag* tag : file->GetTagList()) {
                if (!tag) continue;

                wxString tagName = tag->GetName();
                if (tag->IsStr()) {
                    std::string value = tag->GetStr().ToStdString();
                    result.metadata[tagName.ToStdString()] = value;
                } else if (tag->IsInt()) {
                    std::string value = std::to_string(tag->GetInt());
                    result.metadata[tagName.ToStdString()] = value;
                }
            }
        }

        // Check if file matches search criteria
        if (FileMatches(result, params)) {
            results.push_back(result);
        }

        // Stop if we've reached max results
        if (results.size() >= params.maxResults) {
            break;
        }
    }

    // Store results
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        it->second.results = results;
        it->second.lastUpdateTime = std::chrono::system_clock::now();

        // Update total results count
        m_statistics.totalResults += results.size();
    }

    std::cout << "[LocalSearchEngine] Found " << results.size() << " results" << std::endl;
}

bool LocalSearchEngine::FileMatches(const SearchResult& file, const SearchParams& params) const
{
    // Check file size constraints
    if (params.minFileSize && file.fileSize < *params.minFileSize) {
        return false;
    }

    if (params.maxFileSize && file.fileSize > *params.maxFileSize) {
        return false;
    }

    // Check file type constraints
    if (!params.fileTypes.empty()) {
        bool typeMatch = false;
        std::string fileTypeLower = ToLower(file.fileType);
        for (const auto& type : params.fileTypes) {
            if (Contains(fileTypeLower, ToLower(type))) {
                typeMatch = true;
                break;
            }
        }
        if (!typeMatch) {
            return false;
        }
    }

    // Check query match in filename
    if (!Contains(ToLower(file.fileName), ToLower(params.query))) {
        // Check if query matches metadata
        bool metadataMatch = false;
        for (const auto& [key, value] : file.metadata) {
            if (Contains(ToLower(value), ToLower(params.query))) {
                metadataMatch = true;
                break;
            }
        }
        if (!metadataMatch) {
            return false;
        }
    }

    return true;
}

std::string LocalSearchEngine::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool LocalSearchEngine::Contains(const std::string& str, const std::string& substr)
{
    return str.find(substr) != std::string::npos;
}

} // namespace search
