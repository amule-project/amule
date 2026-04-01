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

#ifndef SEARCH_PARAMS_H
#define SEARCH_PARAMS_H

#include "SearchTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace search {

/**
 * Parameters for starting a search
 * Supports parameters specific to each search type
 */
struct SearchParams {
    SearchType type;
    std::string query;

    // Type-specific parameters
    struct KadParams {
        std::string keywordHash;
        uint32_t maxNodes;
        bool requestMoreResults;

        KadParams()
            : maxNodes(500)
            , requestMoreResults(false)
        {}
    };

    struct GlobalParams {
        uint32_t serverIp;
        uint16_t serverPort;

        GlobalParams()
            : serverIp(0)
            , serverPort(0)
        {}
    };

    std::optional<KadParams> kadParams;
    std::optional<GlobalParams> globalParams;

    // Common parameters
    uint32_t maxResults;
    std::chrono::seconds timeout;

    // Filtering
    std::optional<uint64_t> minFileSize;
    std::optional<uint64_t> maxFileSize;
    std::vector<std::string> fileTypes;

    // Default constructor
    SearchParams()
        : type(SearchType::LOCAL)
        , maxResults(500)
        , timeout(std::chrono::seconds(60))
    {}

    /**
     * Validate search parameters
     * @return true if parameters are valid
     */
    bool IsValid() const {
        if (query.empty()) {
            return false;
        }

        switch (type) {
            case SearchType::KADEMLIA:
                if (!kadParams) {
                    return false;
                }
                break;
            case SearchType::GLOBAL:
                if (!globalParams) {
                    return false;
                }
                if (globalParams->serverIp == 0 || globalParams->serverPort == 0) {
                    return false;
                }
                break;
            case SearchType::LOCAL:
                // Local searches don't need type-specific params
                break;
        }

        if (maxResults == 0 || maxResults > 10000) {
            return false;
        }

        if (timeout.count() == 0 || timeout.count() > 3600) {
            return false;
        }

        if (minFileSize && maxFileSize && *minFileSize > *maxFileSize) {
            return false;
        }

        return true;
    }

    /**
     * Serialize to byte array for thread-safe passing
     */
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data;

        // Write type
        uint8_t t = static_cast<uint8_t>(type);
        data.push_back(t);

        // Write query
        uint32_t qLen = query.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&qLen), reinterpret_cast<uint8_t*>(&qLen) + sizeof(qLen));
        data.insert(data.end(), query.begin(), query.end());

        // Write kadParams presence flag
        uint8_t hasKadParams = kadParams.has_value() ? 1 : 0;
        data.push_back(hasKadParams);

        if (kadParams) {
            // Write keywordHash
            uint32_t khLen = kadParams->keywordHash.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&khLen), reinterpret_cast<uint8_t*>(&khLen) + sizeof(khLen));
            data.insert(data.end(), kadParams->keywordHash.begin(), kadParams->keywordHash.end());

            // Write maxNodes
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&kadParams->maxNodes), reinterpret_cast<const uint8_t*>(&kadParams->maxNodes) + sizeof(kadParams->maxNodes));

            // Write requestMoreResults
            uint8_t rmr = kadParams->requestMoreResults ? 1 : 0;
            data.push_back(rmr);
        }

        // Write globalParams presence flag
        uint8_t hasGlobalParams = globalParams.has_value() ? 1 : 0;
        data.push_back(hasGlobalParams);

        if (globalParams) {
            // Write serverIp
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&globalParams->serverIp), reinterpret_cast<const uint8_t*>(&globalParams->serverIp) + sizeof(globalParams->serverIp));

            // Write serverPort
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&globalParams->serverPort), reinterpret_cast<const uint8_t*>(&globalParams->serverPort) + sizeof(globalParams->serverPort));
        }

        // Write maxResults
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&maxResults), reinterpret_cast<const uint8_t*>(&maxResults) + sizeof(maxResults));

        // Write timeout
        uint64_t timeoutSecs = timeout.count();
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&timeoutSecs), reinterpret_cast<const uint8_t*>(&timeoutSecs) + sizeof(timeoutSecs));

        // Write minFileSize presence flag
        uint8_t hasMinSize = minFileSize.has_value() ? 1 : 0;
        data.push_back(hasMinSize);

        if (minFileSize) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&*minFileSize), reinterpret_cast<const uint8_t*>(&*minFileSize) + sizeof(*minFileSize));
        }

        // Write maxFileSize presence flag
        uint8_t hasMaxSize = maxFileSize.has_value() ? 1 : 0;
        data.push_back(hasMaxSize);

        if (maxFileSize) {
            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&*maxFileSize), reinterpret_cast<const uint8_t*>(&*maxFileSize) + sizeof(*maxFileSize));
        }

        // Write fileTypes count
        uint32_t ftCount = fileTypes.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&ftCount), reinterpret_cast<uint8_t*>(&ftCount) + sizeof(ftCount));

        // Write fileTypes
        for (const auto& ft : fileTypes) {
            uint32_t ftLen = ft.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&ftLen), reinterpret_cast<uint8_t*>(&ftLen) + sizeof(ftLen));
            data.insert(data.end(), ft.begin(), ft.end());
        }

        return data;
    }

    /**
     * Deserialize from byte array
     */
    static SearchParams Deserialize(const std::vector<uint8_t>& data) {
        SearchParams params;
        size_t pos = 0;

        // Read type
        if (pos + sizeof(uint8_t) > data.size()) return params;
        params.type = static_cast<SearchType>(data[pos]);
        pos += sizeof(uint8_t);

        // Read query
        if (pos + sizeof(uint32_t) > data.size()) return params;
        uint32_t qLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + qLen > data.size()) return params;
        params.query.assign(reinterpret_cast<const char*>(&data[pos]), qLen);
        pos += qLen;

        // Read kadParams presence flag
        if (pos + sizeof(uint8_t) > data.size()) return params;
        uint8_t hasKadParams = data[pos];
        pos += sizeof(uint8_t);

        if (hasKadParams) {
            KadParams kp;

            // Read keywordHash
            if (pos + sizeof(uint32_t) > data.size()) return params;
            uint32_t khLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + khLen > data.size()) return params;
            kp.keywordHash.assign(reinterpret_cast<const char*>(&data[pos]), khLen);
            pos += khLen;

            // Read maxNodes
            if (pos + sizeof(uint32_t) > data.size()) return params;
            kp.maxNodes = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);

            // Read requestMoreResults
            if (pos + sizeof(uint8_t) > data.size()) return params;
            kp.requestMoreResults = data[pos] != 0;
            pos += sizeof(uint8_t);

            params.kadParams = kp;
        }

        // Read globalParams presence flag
        if (pos + sizeof(uint8_t) > data.size()) return params;
        uint8_t hasGlobalParams = data[pos];
        pos += sizeof(uint8_t);

        if (hasGlobalParams) {
            GlobalParams gp;

            // Read serverIp
            if (pos + sizeof(uint32_t) > data.size()) return params;
            gp.serverIp = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);

            // Read serverPort
            if (pos + sizeof(uint16_t) > data.size()) return params;
            gp.serverPort = *reinterpret_cast<const uint16_t*>(&data[pos]);
            pos += sizeof(uint16_t);

            params.globalParams = gp;
        }

        // Read maxResults
        if (pos + sizeof(uint32_t) > data.size()) return params;
        params.maxResults = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read timeout
        if (pos + sizeof(uint64_t) > data.size()) return params;
        uint64_t timeoutSecs = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);
        params.timeout = std::chrono::seconds(timeoutSecs);

        // Read minFileSize presence flag
        if (pos + sizeof(uint8_t) > data.size()) return params;
        uint8_t hasMinSize = data[pos];
        pos += sizeof(uint8_t);

        if (hasMinSize) {
            if (pos + sizeof(uint64_t) > data.size()) return params;
            uint64_t minSize = *reinterpret_cast<const uint64_t*>(&data[pos]);
            pos += sizeof(uint64_t);
            params.minFileSize = minSize;
        }

        // Read maxFileSize presence flag
        if (pos + sizeof(uint8_t) > data.size()) return params;
        uint8_t hasMaxSize = data[pos];
        pos += sizeof(uint8_t);

        if (hasMaxSize) {
            if (pos + sizeof(uint64_t) > data.size()) return params;
            uint64_t maxSize = *reinterpret_cast<const uint64_t*>(&data[pos]);
            pos += sizeof(uint64_t);
            params.maxFileSize = maxSize;
        }

        // Read fileTypes count
        if (pos + sizeof(uint32_t) > data.size()) return params;
        uint32_t ftCount = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read fileTypes
        for (uint32_t i = 0; i < ftCount; ++i) {
            if (pos + sizeof(uint32_t) > data.size()) return params;
            uint32_t ftLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + ftLen > data.size()) return params;
            params.fileTypes.emplace_back(reinterpret_cast<const char*>(&data[pos]), ftLen);
            pos += ftLen;
        }

        return params;
    }
};

} // namespace search

#endif // SEARCH_PARAMS_H
