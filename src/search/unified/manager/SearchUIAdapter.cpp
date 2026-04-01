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

#include "SearchUIAdapter.h"
#include "UnifiedSearchManager.h"
#include "SearchEvents.h"
#include <iostream>

namespace search {

// Custom event type
wxDEFINE_EVENT(wxEVT_SEARCH_EVENT, wxCommandEvent);

SearchUIAdapter::SearchUIAdapter(wxWindow* parent)
    : wxEvtHandler()
    , m_manager(nullptr)
    , m_initialized(false)
{
    if (parent) {
        parent->PushEventHandler(this);
    }
}

SearchUIAdapter::~SearchUIAdapter()
{
    Shutdown();
}

bool SearchUIAdapter::Initialize()
{
    if (m_initialized) {
        return true;
    }

    // Check feature flag
    if (!IsUnifiedSearchEnabled()) {
        std::cout << "[SearchUIAdapter] Unified search disabled by feature flag" << std::endl;
        return false;
    }

    // Create search manager
    m_manager = new UnifiedSearchManager();

    // Set event callback
    m_manager->SetEventCallback([this](const SearchEvent& event) {
        // Post event to UI thread via wxWidgets
        auto serialized = event.Serialize();
        wxCommandEvent* evt = new wxCommandEvent(wxEVT_SEARCH_EVENT);
        evt->SetInt(static_cast<int>(event.type));
        evt->SetString(wxString::FromUTF8(event.errorMessage.c_str()));
        evt->SetClientObject(new wxClientDataContainer(serialized));
        wxQueueEvent(this, evt);
    });

    // Start search manager
    m_manager->Start();

    // Bind event handler
    Bind(wxEVT_SEARCH_EVENT, &SearchUIAdapter::OnSearchEvent, this);

    m_initialized = true;
    std::cout << "[SearchUIAdapter] Initialized successfully" << std::endl;

    return true;
}

void SearchUIAdapter::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    // Unbind event handler
    Unbind(wxEVT_SEARCH_EVENT, &SearchUIAdapter::OnSearchEvent, this);

    // Shutdown search manager
    if (m_manager) {
        m_manager->Shutdown();
        delete m_manager;
        m_manager = nullptr;
    }

    m_initialized = false;
    std::cout << "[SearchUIAdapter] Shutdown complete" << std::endl;
}

bool SearchUIAdapter::IsUnifiedSearchEnabled() const
{
    return FeatureFlags::IsEnabled(FeatureFlags::UNIFIED_SEARCH_MANAGER);
}

SearchId SearchUIAdapter::StartSearch(const SearchParams& params)
{
    if (!m_initialized || !m_manager) {
        return SearchId::Invalid();
    }

    SearchCommand cmd = SearchCommand::StartSearch(params);
    if (m_manager->SendCommand(cmd)) {
        return cmd.searchId;
    }

    return SearchId::Invalid();
}

void SearchUIAdapter::StopSearch(SearchId searchId)
{
    if (!m_initialized || !m_manager) {
        return;
    }

    SearchCommand cmd = SearchCommand::StopSearch(searchId);
    m_manager->SendCommand(cmd);
}

void SearchUIAdapter::PauseSearch(SearchId searchId)
{
    if (!m_initialized || !m_manager) {
        return;
    }

    SearchCommand cmd = SearchCommand::PauseSearch(searchId);
    m_manager->SendCommand(cmd);
}

void SearchUIAdapter::ResumeSearch(SearchId searchId)
{
    if (!m_initialized || !m_manager) {
        return;
    }

    SearchCommand cmd = SearchCommand::ResumeSearch(searchId);
    m_manager->SendCommand(cmd);
}

void SearchUIAdapter::RequestMoreResults(SearchId searchId)
{
    if (!m_initialized || !m_manager) {
        return;
    }

    SearchCommand cmd = SearchCommand::RequestMoreResults(searchId);
    m_manager->SendCommand(cmd);
}

std::vector<SearchResult> SearchUIAdapter::GetResults(SearchId searchId, size_t maxResults)
{
    if (!m_initialized || !m_manager) {
        return {};
    }

    std::vector<SearchResult> results;
    bool gotResponse = false;

    SearchCommand cmd = SearchCommand::GetResults(searchId, maxResults);
    cmd.responseCallback = [&](const std::vector<uint8_t>& response) {
        // Deserialize results
        size_t pos = 0;
        while (pos < response.size()) {
            if (pos + sizeof(uint32_t) > response.size()) break;
            uint32_t resultLen = *reinterpret_cast<const uint32_t*>(&response[pos]);
            pos += sizeof(uint32_t);
            if (pos + resultLen > response.size()) break;
            std::vector<uint8_t> resultData(response.begin() + pos, response.begin() + pos + resultLen);
            results.push_back(SearchResult::Deserialize(resultData));
            pos += resultLen;
        }
        gotResponse = true;
    };

    m_manager->SendCommand(cmd);

    // Wait for response (simple implementation)
    auto start = std::chrono::steady_clock::now();
    while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return results;
}

size_t SearchUIAdapter::GetResultCount(SearchId searchId)
{
    if (!m_initialized || !m_manager) {
        return 0;
    }

    size_t count = 0;
    bool gotResponse = false;

    SearchCommand cmd = SearchCommand::GetResultCount(searchId);
    cmd.responseCallback = [&](const std::vector<uint8_t>& response) {
        if (response.size() >= sizeof(size_t)) {
            count = *reinterpret_cast<const size_t*>(&response[0]);
        }
        gotResponse = true;
    };

    m_manager->SendCommand(cmd);

    // Wait for response
    auto start = std::chrono::steady_clock::now();
    while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return count;
}

void SearchUIAdapter::CancelAllSearches()
{
    if (!m_initialized || !m_manager) {
        return;
    }

    SearchCommand cmd = SearchCommand::CancelAllSearches();
    m_manager->SendCommand(cmd);
}

void SearchUIAdapter::OnSearchEvent(wxCommandEvent& event)
{
    // Deserialize event
    wxClientDataContainer* container = dynamic_cast<wxClientDataContainer*>(event.GetClientObject());
    if (!container) {
        return;
    }

    const auto& data = container->GetData();
    SearchEvent searchEvent = SearchEvent::Deserialize(data);

    // Process event
    ProcessEvent(searchEvent);
}

void SearchUIAdapter::ProcessEvent(const SearchEvent& event)
{
    switch (event.type) {
        case SearchEvent::Type::SEARCH_STARTED:
            if (m_onSearchStarted) {
                m_onSearchStarted(event.searchId);
            }
            break;

        case SearchEvent::Type::SEARCH_COMPLETED:
            if (m_onSearchCompleted) {
                m_onSearchCompleted(event.searchId);
            }
            break;

        case SearchEvent::Type::SEARCH_FAILED:
            if (m_onSearchFailed) {
                m_onSearchFailed(event.searchId, wxString::FromUTF8(event.errorMessage.c_str()));
            }
            break;

        case SearchEvent::Type::SEARCH_CANCELLED:
            if (m_onSearchCancelled) {
                m_onSearchCancelled(event.searchId);
            }
            break;

        case SearchEvent::Type::SEARCH_PAUSED:
            // No specific callback for pause
            break;

        case SearchEvent::Type::SEARCH_RESUMED:
            // No specific callback for resume
            break;

        case SearchEvent::Type::RESULTS_RECEIVED:
            if (m_onResultsReceived) {
                m_onResultsReceived(event.searchId, event.results);
            }
            break;

        case SearchEvent::Type::PROGRESS_UPDATE:
            if (m_onProgress && event.progress) {
                m_onProgress(event.searchId, event.progress->percentage,
                          wxString::FromUTF8(event.progress->statusMessage.c_str()));
            }
            break;

        case SearchEvent::Type::ERROR_OCCURRED:
            if (m_onError) {
                m_onError(wxString::FromUTF8(event.errorMessage.c_str()));
            }
            break;

        default:
            std::cerr << "[SearchUIAdapter] Unknown event type: "
                      << static_cast<int>(event.type) << std::endl;
            break;
    }
}

} // namespace search
