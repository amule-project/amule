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

#include <gtest/gtest.h>
#include "../../core/SearchParams.h"

using namespace search;

TEST(SearchParamsTest, DefaultConstructor) {
    SearchParams params;

    EXPECT_EQ(params.type, SearchType::LOCAL);
    EXPECT_TRUE(params.query.empty());
    EXPECT_EQ(params.maxResults, 500);
    EXPECT_EQ(params.timeout.count(), 60);
    EXPECT_FALSE(params.kadParams.has_value());
    EXPECT_FALSE(params.globalParams.has_value());
    EXPECT_FALSE(params.minFileSize.has_value());
    EXPECT_FALSE(params.maxFileSize.has_value());
    EXPECT_TRUE(params.fileTypes.empty());
}

TEST(SearchParamsTest, ValidateSuccess) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 100;
    params.timeout = std::chrono::seconds(30);

    EXPECT_TRUE(params.IsValid());
}

TEST(SearchParamsTest, ValidateEmptyQuery) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "";
    params.maxResults = 100;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateKadWithoutParams) {
    SearchParams params;
    params.type = SearchType::KADEMLIA;
    params.query = "test";
    params.maxResults = 100;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateKadWithParams) {
    SearchParams params;
    params.type = SearchType::KADEMLIA;
    params.query = "test";
    params.maxResults = 100;

    SearchParams::KadParams kadParams;
    kadParams.keywordHash = "ABCD1234";
    kadParams.maxNodes = 500;
    params.kadParams = kadParams;

    EXPECT_TRUE(params.IsValid());
}

TEST(SearchParamsTest, ValidateGlobalWithoutParams) {
    SearchParams params;
    params.type = SearchType::GLOBAL;
    params.query = "test";
    params.maxResults = 100;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateGlobalWithValidParams) {
    SearchParams params;
    params.type = SearchType::GLOBAL;
    params.query = "test";
    params.maxResults = 100;

    SearchParams::GlobalParams globalParams;
    globalParams.serverIp = 0x7F000001; // 127.0.0.1
    globalParams.serverPort = 4662;
    params.globalParams = globalParams;

    EXPECT_TRUE(params.IsValid());
}

TEST(SearchParamsTest, ValidateGlobalWithInvalidServerIp) {
    SearchParams params;
    params.type = SearchType::GLOBAL;
    params.query = "test";
    params.maxResults = 100;

    SearchParams::GlobalParams globalParams;
    globalParams.serverIp = 0;
    globalParams.serverPort = 4662;
    params.globalParams = globalParams;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateMaxResultsZero) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 0;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateMaxResultsTooLarge) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 20000;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateTimeoutZero) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 100;
    params.timeout = std::chrono::seconds(0);

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateTimeoutTooLarge) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 100;
    params.timeout = std::chrono::seconds(5000);

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, ValidateFileSizeRange) {
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 100;
    params.minFileSize = 1000;
    params.maxFileSize = 500;

    EXPECT_FALSE(params.IsValid());
}

TEST(SearchParamsTest, SerializeDeserialize) {
    SearchParams original;
    original.type = SearchType::KADEMLIA;
    original.query = "test query";
    original.maxResults = 200;
    original.timeout = std::chrono::seconds(120);

    SearchParams::KadParams kadParams;
    kadParams.keywordHash = "KEYWORDHASH";
    kadParams.maxNodes = 1000;
    kadParams.requestMoreResults = true;
    original.kadParams = kadParams;

    original.minFileSize = 1024;
    original.maxFileSize = 1024 * 1024 * 100;
    original.fileTypes = {"mp3", "flac", "wav"};

    auto serialized = original.Serialize();
    SearchParams deserialized = SearchParams::Deserialize(serialized);

    EXPECT_EQ(original.type, deserialized.type);
    EXPECT_EQ(original.query, deserialized.query);
    EXPECT_EQ(original.maxResults, deserialized.maxResults);
    EXPECT_EQ(original.timeout, deserialized.timeout);
    EXPECT_TRUE(deserialized.kadParams.has_value());
    EXPECT_EQ(kadParams.keywordHash, deserialized.kadParams->keywordHash);
    EXPECT_EQ(kadParams.maxNodes, deserialized.kadParams->maxNodes);
    EXPECT_EQ(kadParams.requestMoreResults, deserialized.kadParams->requestMoreResults);
    EXPECT_TRUE(deserialized.minFileSize.has_value());
    EXPECT_EQ(*original.minFileSize, *deserialized.minFileSize);
    EXPECT_TRUE(deserialized.maxFileSize.has_value());
    EXPECT_EQ(*original.maxFileSize, *deserialized.maxFileSize);
    EXPECT_EQ(original.fileTypes.size(), deserialized.fileTypes.size());
}

TEST(SearchParamsTest, SerializeDeserializeWithGlobalParams) {
    SearchParams original;
    original.type = SearchType::GLOBAL;
    original.query = "test";
    original.maxResults = 100;

    SearchParams::GlobalParams globalParams;
    globalParams.serverIp = 0xC0A80101; // 192.168.1.1
    globalParams.serverPort = 4662;
    original.globalParams = globalParams;

    auto serialized = original.Serialize();
    SearchParams deserialized = SearchParams::Deserialize(serialized);

    EXPECT_EQ(original.type, deserialized.type);
    EXPECT_TRUE(deserialized.globalParams.has_value());
    EXPECT_EQ(globalParams.serverIp, deserialized.globalParams->serverIp);
    EXPECT_EQ(globalParams.serverPort, deserialized.globalParams->serverPort);
}

TEST(SearchParamsTest, DeserializeInvalidData) {
    std::vector<uint8_t> invalidData = {0x01, 0x02, 0x03};
    SearchParams params = SearchParams::Deserialize(invalidData);

    // Should return default-constructed params
    EXPECT_EQ(params.type, SearchType::LOCAL);
    EXPECT_TRUE(params.query.empty());
}
