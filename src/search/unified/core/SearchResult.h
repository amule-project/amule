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

#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include "SearchId.h"
#include "SearchTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>

namespace search {

/**
 * Unified search result from any search type (local, global, Kad)
 * Designed for thread-safe serialization between search and UI threads
 */
struct SearchResult {
    SearchId searchId;
    SearchType sourceType;

    // File information
    std::string fileName;
    uint64_t fileSize;
    std::string fileType;

    // Source information
    std::string fileHash;  // MD4 hash as hex string
    uint32_t availability;

    // Metadata
    std::map<std::string, std::string> metadata;

    // Source locations
    struct SourceLocation {
        std::string ip;
        uint16_t port;
        uint32_t kadId;  // For Kad sources (NodeID)

        SourceLocation() : port(0), kadId(0) {}
        SourceLocation(const std::string& i, uint16_t p, uint32_t k = 0)
            : ip(i), port(p), kadId(k) {}
    };
    std::vector<SourceLocation> sources;

    // Timing
    std::chrono::system_clock::time_point discoveredAt;

    // Default constructor
    SearchResult()
        : fileSize(0)
        , availability(0)
        , discoveredAt(std::chrono::system_clock::now())
    {}

    /**
     * Serialize to byte array for thread-safe passing
     */
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data;

        // Write searchId
        uint64_t sid = searchId.value;
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&sid), reinterpret_cast<uint8_t*>(&sid) + sizeof(sid));

        // Write sourceType
        uint8_t st = static_cast<uint8_t>(sourceType);
        data.push_back(st);

        // Write fileName
        uint32_t fnLen = fileName.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&fnLen), reinterpret_cast<uint8_t*>(&fnLen) + sizeof(fnLen));
        data.insert(data.end(), fileName.begin(), fileName.end());

        // Write fileSize
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&fileSize), reinterpret_cast<const uint8_t*>(&fileSize) + sizeof(fileSize));

        // Write fileType
        uint32_t ftLen = fileType.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&ftLen), reinterpret_cast<uint8_t*>(&ftLen) + sizeof(ftLen));
        data.insert(data.end(), fileType.begin(), fileType.end());

        // Write fileHash
        uint32_t fhLen = fileHash.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&fhLen), reinterpret_cast<uint8_t*>(&fhLen) + sizeof(fhLen));
        data.insert(data.end(), fileHash.begin(), fileHash.end());

        // Write availability
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&availability), reinterpret_cast<const uint8_t*>(&availability) + sizeof(availability));

        // Write metadata count
        uint32_t metaCount = metadata.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&metaCount), reinterpret_cast<uint8_t*>(&metaCount) + sizeof(metaCount));

        // Write metadata entries
        for (const auto& [key, value] : metadata) {
            uint32_t keyLen = key.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&keyLen), reinterpret_cast<uint8_t*>(&keyLen) + sizeof(keyLen));
            data.insert(data.end(), key.begin(), key.end());

            uint32_t valueLen = value.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&valueLen), reinterpret_cast<uint8_t*>(&valueLen) + sizeof(valueLen));
            data.insert(data.end(), value.begin(), value.end());
        }

        // Write sources count
        uint32_t sourceCount = sources.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&sourceCount), reinterpret_cast<uint8_t*>(&sourceCount) + sizeof(sourceCount));

        // Write sources
        for (const auto& source : sources) {
            uint32_t ipLen = source.ip.size();
            data.insert(data.end(), reinterpret_cast<uint8_t*>(&ipLen), reinterpret_cast<uint8_t*>(&ipLen) + sizeof(ipLen));
            data.insert(data.end(), source.ip.begin(), source.ip.end());

            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&source.port), reinterpret_cast<const uint8_t*>(&source.port) + sizeof(source.port));

            data.insert(data.end(), reinterpret_cast<const uint8_t*>(&source.kadId), reinterpret_cast<const uint8_t*>(&source.kadId) + sizeof(source.kadId));
        }

        return data;
    }

    /**
     * Deserialize from byte array
     */
    static SearchResult Deserialize(const std::vector<uint8_t>& data) {
        SearchResult result;
        size_t pos = 0;

        // Read searchId
        if (pos + sizeof(uint64_t) > data.size()) return result;
        result.searchId.value = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);

        // Read sourceType
        if (pos + sizeof(uint8_t) > data.size()) return result;
        result.sourceType = static_cast<SearchType>(data[pos]);
        pos += sizeof(uint8_t);

        // Read fileName
        if (pos + sizeof(uint32_t) > data.size()) return result;
        uint32_t fnLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + fnLen > data.size()) return result;
        result.fileName.assign(reinterpret_cast<const char*>(&data[pos]), fnLen);
        pos += fnLen;

        // Read fileSize
        if (pos + sizeof(uint64_t) > data.size()) return result;
        result.fileSize = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += sizeof(uint64_t);

        // Read fileType
        if (pos + sizeof(uint32_t) > data.size()) return result;
        uint32_t ftLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + ftLen > data.size()) return result;
        result.fileType.assign(reinterpret_cast<const char*>(&data[pos]), ftLen);
        pos += ftLen;

        // Read fileHash
        if (pos + sizeof(uint32_t) > data.size()) return result;
        uint32_t fhLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);
        if (pos + fhLen > data.size()) return result;
        result.fileHash.assign(reinterpret_cast<const char*>(&data[pos]), fhLen);
        pos += fhLen;

        // Read availability
        if (pos + sizeof(uint32_t) > data.size()) return result;
        result.availability = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read metadata count
        if (pos + sizeof(uint32_t) > data.size()) return result;
        uint32_t metaCount = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read metadata entries
        for (uint32_t i = 0; i < metaCount; ++i) {
            if (pos + sizeof(uint32_t) > data.size()) return result;
            uint32_t keyLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + keyLen > data.size()) return result;
            std::string key(reinterpret_cast<const char*>(&data[pos]), keyLen);
            pos += keyLen;

            if (pos + sizeof(uint32_t) > data.size()) return result;
            uint32_t valueLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + valueLen > data.size()) return result;
            std::string value(reinterpret_cast<const char*>(&data[pos]), valueLen);
            pos += valueLen;

            result.metadata[key] = value;
        }

        // Read sources count
        if (pos + sizeof(uint32_t) > data.size()) return result;
        uint32_t sourceCount = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += sizeof(uint32_t);

        // Read sources
        for (uint32_t i = 0; i < sourceCount; ++i) {
            SourceLocation source;

            if (pos + sizeof(uint32_t) > data.size()) return result;
            uint32_t ipLen = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);
            if (pos + ipLen > data.size()) return result;
            source.ip.assign(reinterpret_cast<const char*>(&data[pos]), ipLen);
            pos += ipLen;

            if (pos + sizeof(uint16_t) > data.size()) return result;
            source.port = *reinterpret_cast<const uint16_t*>(&data[pos]);
            pos += sizeof(uint16_t);

            if (pos + sizeof(uint32_t) > data.size()) return result;
            source.kadId = *reinterpret_cast<const uint32_t*>(&data[pos]);
            pos += sizeof(uint32_t);

            result.sources.push_back(source);
        }

        return result;
    }
};

} // namespace search

#endif // SEARCH_RESULT_H
