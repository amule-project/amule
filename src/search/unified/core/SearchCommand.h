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

#ifndef SEARCH_COMMAND_H
#define SEARCH_COMMAND_H

#include "SearchId.h"
#include "SearchParams.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace search {

// Forward declaration
struct SearchParams;

/**
 * Commands sent from UI thread to search thread
 * All commands are serializable for thread-safe queue passing
 */
struct SearchCommand {
    enum class Type : uint8_t {
        START_SEARCH,
        STOP_SEARCH,
        PAUSE_SEARCH,
        RESUME_SEARCH,
        REQUEST_MORE_RESULTS,
        GET_SEARCH_STATE,
        GET_SEARCH_PARAMS,
        GET_RESULTS,
        GET_RESULT_COUNT,
        CANCEL_ALL_SEARCHES
    };

    Type type;
    SearchId searchId;
    SearchParams params;
    size_t maxResults;

    // Response callback (executed in search thread, result sent back to UI)
    using ResponseCallback = std::function<void(const std::vector<uint8_t>& response)>;
    ResponseCallback responseCallback;

    // Default constructor
    SearchCommand()
        : type(Type::GET_RESULTS)
        , maxResults(0)
    {}

    /**
     * Serialize to byte array for thread-safe passing
     * Note: responseCallback is not serialized
     */
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data;

        // Write type
        data.push_back(static_cast<uint8_t>(type));

        // Write searchId
        uint64_t sid = searchId.value;
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&sid), reinterpret_cast<uint8_t*>(&sid) + sizeof(sid));

        // Write params
        auto serializedParams = params.Serialize();
        uint32_t paramsLen = serializedParams.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&paramsLen), reinterpret_cast<uint8_t*>(&paramsLen) + sizeof(paramsLen));
        data.insert(data.end(), serializedParams.begin(), serializedParams.end());

        // Write maxResults
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&maxResults), reinterpret_cast<const uint8_t*>(&maxResults) + sizeof(maxResults));

        return data;
    }

    /**
     * Deserialize from byte array
     * Note: responseCallback is not deserialized
     */
    static SearchCommand Deserialize(const std::vector<uint8_t>& data) {
        SearchCommand command;
        size_t pos = 0;

        // Read type
        if (pos + sizeof(uint8_t) > data.size()) return command;
        command.type = static_cast<Type>(data[pos]);
        pos += sizeof(uint8_t);

        // Read searchId
        if (pos + sizeof(uint64_t) > data.size()) return command;
        command.searchId.value = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);

        // Read params
        if (pos + sizeof(uint32_t) > data.size()) return command;
        uint32_t paramsLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + paramsLen > data.size()) return command;
        std::vector<uint8_t> paramsData(data.begin() + pos, data.begin() + pos + paramsLen);
        command.params = SearchParams::Deserialize(paramsData);
        pos += paramsLen;

        // Read maxResults
        if (pos + sizeof(size_t) > data.size()) return command;
        command.maxResults = *reinterpret_cast<const size_t*>(&data[pos]);
        pos += sizeof(size_t);

        return command;
    }

    /**
     * Create START_SEARCH command
     */
    static SearchCommand StartSearch(const SearchParams& params) {
        SearchCommand cmd;
        cmd.type = Type::START_SEARCH;
        cmd.params = params;
        cmd.searchId = SearchId::Generate();
        return cmd;
    }

    /**
     * Create STOP_SEARCH command
     */
    static SearchCommand StopSearch(SearchId searchId) {
        SearchCommand cmd;
        cmd.type = Type::STOP_SEARCH;
        cmd.searchId = searchId;
        return cmd;
    }

    /**
     * Create PAUSE_SEARCH command
     */
    static SearchCommand PauseSearch(SearchId searchId) {
        SearchCommand cmd;
        cmd.type = Type::PAUSE_SEARCH;
        cmd.searchId = searchId;
        return cmd;
    }

    /**
     * Create RESUME_SEARCH command
     */
    static SearchCommand ResumeSearch(SearchId searchId) {
        SearchCommand cmd;
        cmd.type = Type::RESUME_SEARCH;
        cmd.searchId = searchId;
        return cmd;
    }

    /**
     * Create REQUEST_MORE_RESULTS command
     */
    static SearchCommand RequestMoreResults(SearchId searchId) {
        SearchCommand cmd;
        cmd.type = Type::REQUEST_MORE_RESULTS;
        cmd.searchId = searchId;
        return cmd;
    }

    /**
     * Create GET_RESULTS command
     */
    static SearchCommand GetResults(SearchId searchId, size_t maxResults = 0) {
        SearchCommand cmd;
        cmd.type = Type::GET_RESULTS;
        cmd.searchId = searchId;
        cmd.maxResults = maxResults;
        return cmd;
    }

    /**
     * Create GET_RESULT_COUNT command
     */
    static SearchCommand GetResultCount(SearchId searchId) {
        SearchCommand cmd;
        cmd.type = Type::GET_RESULT_COUNT;
        cmd.searchId = searchId;
        return cmd;
    }

    /**
     * Create CANCEL_ALL_SEARCHES command
     */
    static SearchCommand CancelAllSearches() {
        SearchCommand cmd;
        cmd.type = Type::CANCEL_ALL_SEARCHES;
        return cmd;
    }
};

} // namespace search

#endif // SEARCH_COMMAND_H
