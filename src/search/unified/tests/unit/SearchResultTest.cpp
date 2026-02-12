//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of GNU General Public License as published by
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
// Foundation, Inc., 02110-1301, USA
//

#include <gtest/gtest.h>
#include "../../core/SearchResult.h"
#include "../../core/SearchId.h"

using namespace search;

TEST(SearchResultTest, DefaultConstructor) {
    SearchResult result;

    EXPECT_EQ(result.fileSize, 0);
    EXPECT_EQ(result.availability, 0);
    EXPECT_TRUE(result.fileName.empty());
    EXPECT_TRUE(result.fileHash.empty());
    EXPECT_TRUE(result.sources.empty());
    EXPECT_TRUE(result.metadata.empty());
}

TEST(SearchResultTest, SerializeDeserialize) {
    SearchResult original;
    original.searchId = SearchId::Generate();
    original.sourceType = SearchType::KADEMLIA;
    original.fileName = "test_file.mp3";
    original.fileSize = 1024 * 1024;
    original.fileType = "Audio";
    original.fileHash = "A1B2C3D4E5F67890ABCDEF1234567890";
    original.availability = 5;

    SearchResult::SourceLocation source1("192.168.1.1", 4662, 12345);
    SearchResult::SourceLocation source2("192.168.1.2", 4662, 12346);
    original.sources.push_back(source1);
    original.sources.push_back(source2);

    original.metadata["artist"] = "Test Artist";
    original.metadata["album"] = "Test Album";

    auto serialized = original.Serialize();
    SearchResult deserialized = SearchResult::Deserialize(serialized);

    EXPECT_EQ(original.searchId, deserialized.searchId);
    EXPECT_EQ(original.sourceType, deserialized.sourceType);
    EXPECT_EQ(original.fileName, deserialized.fileName);
    EXPECT_EQ(original.fileSize, deserialized.fileSize);
    EXPECT_EQ(original.fileType, deserialized.fileType);
    EXPECT_EQ(original.fileHash, deserialized.fileHash);
    EXPECT_EQ(original.availability, deserialized.availability);
    EXPECT_EQ(original.sources.size(), deserialized.sources.size());
    EXPECT_EQ(original.metadata.size(), deserialized.metadata.size());
}

TEST(SearchResultTest, SerializeDeserializeEmpty) {
    SearchResult original;
    original.searchId = SearchId::Generate();
    original.sourceType = SearchType::LOCAL;

    auto serialized = original.Serialize();
    SearchResult deserialized = SearchResult::Deserialize(serialized);

    EXPECT_EQ(original.searchId, deserialized.searchId);
    EXPECT_EQ(original.sourceType, deserialized.sourceType);
    EXPECT_TRUE(deserialized.fileName.empty());
    EXPECT_EQ(deserialized.fileSize, 0);
}

TEST(SearchResultTest, SerializeDeserializeWithMultipleSources) {
    SearchResult original;
    original.searchId = SearchId::Generate();
    original.sourceType = SearchType::GLOBAL;

    for (int i = 0; i < 10; ++i) {
        SearchResult::SourceLocation source(
            "192.168.1." + std::to_string(i),
            4662 + i,
            10000 + i
        );
        original.sources.push_back(source);
    }

    auto serialized = original.Serialize();
    SearchResult deserialized = SearchResult::Deserialize(serialized);

    EXPECT_EQ(original.sources.size(), deserialized.sources.size());
    for (size_t i = 0; i < original.sources.size(); ++i) {
        EXPECT_EQ(original.sources[i].ip, deserialized.sources[i].ip);
        EXPECT_EQ(original.sources[i].port, deserialized.sources[i].port);
        EXPECT_EQ(original.sources[i].kadId, deserialized.sources[i].kadId);
    }
}

TEST(SearchResultTest, SerializeDeserializeWithComplexMetadata) {
    SearchResult original;
    original.searchId = SearchId::Generate();
    original.sourceType = SearchType::KADEMLIA;

    original.metadata["artist"] = "Test Artist";
    original.metadata["album"] = "Test Album";
    original.metadata["year"] = "2024";
    original.metadata["genre"] = "Rock";
    original.metadata["bitrate"] = "320";

    auto serialized = original.Serialize();
    SearchResult deserialized = SearchResult::Deserialize(serialized);

    EXPECT_EQ(original.metadata.size(), deserialized.metadata.size());
    for (const auto& [key, value] : original.metadata) {
        EXPECT_EQ(deserialized.metadata[key], value);
    }
}

TEST(SearchResultTest, DeserializeInvalidData) {
    std::vector<uint8_t> invalidData = {0x01, 0x02, 0x03};
    SearchResult result = SearchResult::Deserialize(invalidData);

    // Should return a default-constructed result
    EXPECT_EQ(result.searchId.value, 0);
    EXPECT_EQ(result.sourceType, SearchType::LOCAL);
}
