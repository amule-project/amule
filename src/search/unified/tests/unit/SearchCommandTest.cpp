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
#include "../../core/SearchCommand.h"
#include "../../core/SearchParams.h"

using namespace search;

TEST(SearchCommandTest, DefaultConstructor) {
    SearchCommand cmd;

    EXPECT_EQ(cmd.type, SearchCommand::Type::GET_RESULTS);
    EXPECT_EQ(cmd.maxResults, 0);
}

TEST(SearchCommandTest, StartSearch) {
    SearchParams params;
    params.type = SearchType::KADEMLIA;
    params.query = "test";

    SearchCommand cmd = SearchCommand::StartSearch(params);

    EXPECT_EQ(cmd.type, SearchCommand::Type::START_SEARCH);
    EXPECT_TRUE(cmd.searchId.IsValid());
    EXPECT_EQ(cmd.params.type, params.type);
    EXPECT_EQ(cmd.params.query, params.query);
}

TEST(SearchCommandTest, StopSearch) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::StopSearch(searchId);

    EXPECT_EQ(cmd.type, SearchCommand::Type::STOP_SEARCH);
    EXPECT_EQ(cmd.searchId, searchId);
}

TEST(SearchCommandTest, PauseSearch) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::PauseSearch(searchId);

    EXPECT_EQ(cmd.type, SearchCommand::Type::PAUSE_SEARCH);
    EXPECT_EQ(cmd.searchId, searchId);
}

TEST(SearchCommandTest, ResumeSearch) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::ResumeSearch(searchId);

    EXPECT_EQ(cmd.type, SearchCommand::Type::RESUME_SEARCH);
    EXPECT_EQ(cmd.searchId, searchId);
}

TEST(SearchCommandTest, RequestMoreResults) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::RequestMoreResults(searchId);

    EXPECT_EQ(cmd.type, SearchCommand::Type::REQUEST_MORE_RESULTS);
    EXPECT_EQ(cmd.searchId, searchId);
}

TEST(SearchCommandTest, GetResults) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::GetResults(searchId, 100);

    EXPECT_EQ(cmd.type, SearchCommand::Type::GET_RESULTS);
    EXPECT_EQ(cmd.searchId, searchId);
    EXPECT_EQ(cmd.maxResults, 100);
}

TEST(SearchCommandTest, GetResultCount) {
    SearchId searchId = SearchId::Generate();
    SearchCommand cmd = SearchCommand::GetResultCount(searchId);

    EXPECT_EQ(cmd.type, SearchCommand::Type::GET_RESULT_COUNT);
    EXPECT_EQ(cmd.searchId, searchId);
}

TEST(SearchCommandTest, CancelAllSearches) {
    SearchCommand cmd = SearchCommand::CancelAllSearches();

    EXPECT_EQ(cmd.type, SearchCommand::Type::CANCEL_ALL_SEARCHES);
}

TEST(SearchCommandTest, SerializeDeserialize) {
    SearchCommand original;
    original.type = SearchCommand::Type::START_SEARCH;
    original.searchId = SearchId::Generate();
    original.params.type = SearchType::KADEMLIA;
    original.params.query = "test query";
    original.maxResults = 100;

    auto serialized = original.Serialize();
    SearchCommand deserialized = SearchCommand::Deserialize(serialized);

    EXPECT_EQ(original.type, deserialized.type);
    EXPECT_EQ(original.searchId, deserialized.searchId);
    EXPECT_EQ(original.params.type, deserialized.params.type);
    EXPECT_EQ(original.params.query, deserialized.params.query);
    EXPECT_EQ(original.maxResults, deserialized.maxResults);
}

TEST(SearchCommandTest, SerializeDeserializeWithKadParams) {
    SearchCommand original = SearchCommand::StartSearch(SearchParams{});

    original.params.type = SearchType::KADEMLIA;
    original.params.query = "test";
    SearchParams::KadParams kadParams;
    kadParams.keywordHash = "HASH";
    kadParams.maxNodes = 500;
    original.params.kadParams = kadParams;

    auto serialized = original.Serialize();
    SearchCommand deserialized = SearchCommand::Deserialize(serialized);

    EXPECT_EQ(original.type, deserialized.type);
    EXPECT_TRUE(deserialized.params.kadParams.has_value());
    EXPECT_EQ(kadParams.keywordHash, deserialized.params.kadParams->keywordHash);
    EXPECT_EQ(kadParams.maxNodes, deserialized.params.kadParams->maxNodes);
}

TEST(SearchCommandTest, DeserializeInvalidData) {
    std::vector<uint8_t> invalidData = {0x01, 0x02, 0x03};
    SearchCommand cmd = SearchCommand::Deserialize(invalidData);

    // Should return default-constructed command
    EXPECT_EQ(cmd.type, SearchCommand::Type::GET_RESULTS);
    EXPECT_EQ(cmd.maxResults, 0);
}
