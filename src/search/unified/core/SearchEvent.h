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

#ifndef SEARCH_EVENT_H
#define SEARCH_EVENT_H

#include "SearchId.h"
#include "SearchTypes.h"
#include "SearchResult.h"
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace search {

// Forward declarations
struct SearchResult;

/**
 * Events sent from search thread to UI thread
 * All events are serializable for thread-safe passing via wxEvents
 */
struct SearchEvent {
    enum class Type : uint8_t {
        SEARCH_STARTED,
        SEARCH_COMPLETED,
        SEARCH_FAILED,
        SEARCH_CANCELLED,
        SEARCH_PAUSED,
        SEARCH_RESUMED,
        RESULTS_RECEIVED,
        PROGRESS_UPDATE,
        ERROR_OCCURRED
    };

    Type type;
    SearchId searchId;
    std::string errorMessage;

    // For RESULTS_RECEIVED
    std::vector<SearchResult> results;

    // For PROGRESS_UPDATE
    struct ProgressInfo {
        int percentage;
        int serversContacted;
        int nodesContacted;
        int resultsReceived;
        std::string statusMessage;

        ProgressInfo()
            : percentage(0)
            , serversContacted(0)
            , nodesContacted(0)
            , resultsReceived(0)
        {}
    };
    std::optional<ProgressInfo> progress;

    // Default constructor
    SearchEvent()
        : type(Type::ERROR_OCCURRED)
    {}

    /**
     * Serialize to byte array for thread-safe passing
     */
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data;

        // Write type
        data.push_back(static_cast<uint8_t>(type));

        // Write searchId
        uint64_t sid = searchId.value;
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&sid), reinterpret_cast<uint8_t*>(&sid) + sizeof(sid));

        // Write errorMessage
        uint32_t emLen = errorMessage.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&emLen), reinterpret_cast<uint8_t*>(&emLen) + sizeof(emLen));
        data.insert(data.end(), errorMessage.begin(), errorMessage.end());

        // Write results count
        uint32_t resultCount = results.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&resultCount), reinterpret_cast<uint8_t*>(&resultCount) + sizeof(resultCount));

        // Write results
        for (const auto& result : results) {
            auto serializedResult = result.Serialize();
            uint32_t resultLen = serializedResult.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&resultLen), reinterpret_cast<uint8_t*>(&resultLen) + sizeof(resultLen));
            data.insert(data.end(), serializedResult.begin(), serializedResult.end());
        }

        // Write progress presence flag
        uint8_t hasProgress = progress.has_value() ? 1 : 0;
        data.push_back(hasProgress);

        if (progress) {
            // Write percentage
            data.insert(data.end(), reinterpret_cast<const int8_t*>(&progress->percentage), reinterpret_cast<const int8_t*>(&progress->percentage) + sizeof(progress->percentage));

            // Write serversContacted
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&progress->serversContacted), reinterpret_cast<const uint8_t*>(&progress->serversContacted) + sizeof(progress->serversContacted));

            // Write nodesContacted
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&progress->nodesContacted), reinterpret_cast<const uint8_t*>(&progress->nodesContacted) + sizeof(progress->nodesContacted));

            // Write resultsReceived
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&progress->resultsReceived), reinterpret_cast<const uint8_t*>(&progress->resultsReceived) + sizeof(progress->resultsReceived));

            // Write statusMessage
            uint32_t smLen = progress->statusMessage.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&smLen), reinterpret_cast<uint8_t*>(&smLen) + sizeof(smLen));
            data.insert(data.end(), progress->statusMessage.begin(), progress->statusMessage.end());
        }

        return data;
    }

    /**
     * Deserialize from byte array
     */
    static SearchEvent Deserialize(const std::vector<uint8_t>& data) {
        SearchEvent event;
        size_t pos = 0;

        // Read type
        if (pos + sizeof(uint8_t) > data.size()) return event;
        event.type = static_cast<Type>(data[pos]);
        pos += sizeof(uint8_t);

        // Read searchId
        if (pos + sizeof(uint64_t) > data.size()) return event;
        event.searchId.value = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);

        // Read errorMessage
        if (pos + sizeof(uint32_t) > data.size()) return event;
        uint32_t emLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + emLen > data.size()) return event;
        event.errorMessage.assign(reinterpret_cast<const char*>(&data[pos]), emLen);
        pos += emLen;

        // Read results count
        if (pos + sizeof(uint32_t) > data.size()) return event;
        uint32_t resultCount = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read results
        for (uint32_t i = 0; i < resultCount; ++i) {
            if (pos + sizeof(uint32_t) > data.size()) return event;
            uint32_t resultLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + resultLen > data.size()) return event;
            std::vector<uint8_t> resultData(data.begin() + pos, data.begin() + pos + resultLen);
            event.results.push_back(SearchResult::Deserialize(resultData));
            pos += resultLen;
        }

        // Read progress presence flag
        if (pos + sizeof(uint8_t) > data.size()) return event;
        uint8_t hasProgress = data[pos];
        pos += sizeof(uint8_t);

        if (hasProgress) {
            ProgressInfo pi;

            // Read percentage
            if (pos + sizeof(pi.percentage) > data.size()) return event;
            pi.percentage = *reinterpret_cast<const int8_t*>(&data[pos]);
            pos += sizeof(pi.percentage);

            // Read serversContacted
            if (pos + sizeof(pi.serversContacted) > data.size()) return event;
            pi.serversContacted = *reinterpret_cast<const int*>(&data[pos]);
            pos += sizeof(pi.serversContacted);

            // Read nodesContacted
            if (pos + sizeof(pi.nodesContacted) > data.size()) return event;
            pi.nodesContacted = *reinterpret_cast<const int*>(&data[pos]);
            pos += sizeof(pi.nodesContacted);

            // Read resultsReceived
            if (pos + sizeof(pi.resultsReceived) > data.size()) return event;
            pi.resultsReceived = *reinterpret_cast<const int*>(&data[pos]);
            pos += sizeof(pi.resultsReceived);

            // Read statusMessage
            if (pos + sizeof(uint32_t) > data.size()) return event;
            uint32_t smLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + smLen > data.size()) return event;
            pi.statusMessage.assign(reinterpret_cast<const char*>(&data[pos]), smLen);
            pos += smLen;

            event.progress = pi;
        }

        return event;
    }

    /**
     * Create SEARCH_STARTED event
     */
    static SearchEvent SearchStarted(SearchId searchId) {
        SearchEvent event;
        event.type = Type::SEARCH_STARTED;
        event.searchId = searchId;
        return event;
    }

    /**
     * Create SEARCH_COMPLETED event
     */
    static SearchEvent SearchCompleted(SearchId searchId) {
        SearchEvent event;
        event.type = Type::SEARCH_COMPLETED;
        event.searchId = searchId;
        return event;
    }

    /**
     * Create SEARCH_FAILED event
     */
    static SearchEvent SearchFailed(SearchId searchId, const std::string& error) {
        SearchEvent event;
        event.type = Type::SEARCH_FAILED;
        event.searchId = searchId;
        event.errorMessage = error;
        return event;
    }

    /**
     * Create SEARCH_CANCELLED event
     */
    static SearchEvent SearchCancelled(SearchId searchId) {
        SearchEvent event;
        event.type = Type::SEARCH_CANCELLED;
        event.searchId = searchId;
        return event;
    }

    /**
     * Create RESULTS_RECEIVED event
     */
    static SearchEvent ResultsReceived(SearchId searchId, const std::vector<SearchResult>& results) {
        SearchEvent event;
        event.type = Type::RESULTS_RECEIVED;
        event.searchId = searchId;
        event.results = results;
        return event;
    }

    /**
     * Create PROGRESS_UPDATE event
     */
    static SearchEvent ProgressUpdate(SearchId searchId, const ProgressInfo& progress) {
        SearchEvent event;
        event.type = Type::PROGRESS_UPDATE;
        event.searchId = searchId;
        event.progress = progress;
        return event;
    }

    /**
     * Create ERROR_OCCURRED event
     */
    static SearchEvent ErrorOccurred(SearchId searchId, const std::string& error) {
        SearchEvent event;
        event.type = Type::ERROR_OCCURRED;
        event.searchId = searchId;
        event.errorMessage = error;
        return event;
    }
};

} // namespace search

#endif // SEARCH_EVENT_H
