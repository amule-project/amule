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

#ifndef SEARCHCONTROLLER_H
#define SEARCHCONTROLLER_H

#include <functional>
#include <wx/string.h>
#include "SearchModel.h"

// Forward declarations
class CSearchFile;

namespace search {

class SearchController {
public:
    using SearchStartedCallback = std::function<void(uint32_t)>;
    using SearchCompletedCallback = std::function<void(uint32_t)>;
    using ResultsReceivedCallback = std::function<void(uint32_t, const std::vector<CSearchFile*>&)>;
    using ErrorCallback = std::function<void(uint32_t, const wxString&)>;
    using ProgressCallback = std::function<void(uint32_t, int)>;

    // Detailed progress information
    struct ProgressInfo {
	int percentage = 0;
	int serversContacted = 0;
	int resultsReceived = 0;
	wxString currentStatus;
    };
    using DetailedProgressCallback = std::function<void(uint32_t, const ProgressInfo&)>;

    virtual ~SearchController() = default;

    // Core search operations
    virtual void startSearch(const SearchParams& params) = 0;
    virtual void stopSearch() = 0;
    virtual void requestMoreResults() = 0;

    // Async callback for more results completion
    using MoreResultsCallback = std::function<void(uint32_t, bool, const wxString&)>;

    // State information
    virtual SearchState getState() const = 0;
    virtual SearchParams getSearchParams() const = 0;
    virtual long getSearchId() const = 0;

    // Result access
    virtual std::vector<CSearchFile*> getResults() const = 0;
    virtual size_t getResultCount() const = 0;

    // Callback setters with move semantics
    void setOnSearchStarted(SearchStartedCallback callback) { m_onSearchStarted = std::move(callback); }
    void setOnSearchCompleted(SearchCompletedCallback callback) { m_onSearchCompleted = std::move(callback); }
    void setOnResultsReceived(ResultsReceivedCallback callback) { m_onResultsReceived = std::move(callback); }
    void setOnError(ErrorCallback callback) { m_onError = std::move(callback); }
    void setOnProgress(ProgressCallback callback) { m_onProgress = std::move(callback); }
    void setOnDetailedProgress(DetailedProgressCallback callback) { m_onDetailedProgress = std::move(callback); }
    void setOnMoreResults(MoreResultsCallback callback) { m_onMoreResults = std::move(callback); }

    // Clear all callbacks
    void clearCallbacks() {
        m_onSearchStarted = nullptr;
        m_onSearchCompleted = nullptr;
        m_onResultsReceived = nullptr;
        m_onError = nullptr;
        m_onProgress = nullptr;
        m_onDetailedProgress = nullptr;
        m_onMoreResults = nullptr;
    }

protected:
    // Protected callbacks for derived classes to trigger
    void notifySearchStarted(uint32_t searchId) { if (m_onSearchStarted) m_onSearchStarted(searchId); }
    void notifySearchCompleted(uint32_t searchId) { if (m_onSearchCompleted) m_onSearchCompleted(searchId); }
    void notifyResultsReceived(uint32_t searchId, const std::vector<CSearchFile*>& results) {
	if (m_onResultsReceived) m_onResultsReceived(searchId, results);
    }
    void notifyError(uint32_t searchId, const wxString& error) { if (m_onError) m_onError(searchId, error); }
    void notifyProgress(uint32_t searchId, int progress) { if (m_onProgress) m_onProgress(searchId, progress); }
    void notifyDetailedProgress(uint32_t searchId, const ProgressInfo& info) { if (m_onDetailedProgress) m_onDetailedProgress(searchId, info); }
    void notifyMoreResults(uint32_t searchId, bool success, const wxString& message) {
	if (m_onMoreResults) m_onMoreResults(searchId, success, message);
    }

private:
    SearchStartedCallback m_onSearchStarted = nullptr;
    SearchCompletedCallback m_onSearchCompleted = nullptr;
    ResultsReceivedCallback m_onResultsReceived = nullptr;
    ErrorCallback m_onError = nullptr;
    ProgressCallback m_onProgress = nullptr;
    DetailedProgressCallback m_onDetailedProgress = nullptr;
    MoreResultsCallback m_onMoreResults = nullptr;
};

} // namespace search

#endif // SEARCHCONTROLLER_H
