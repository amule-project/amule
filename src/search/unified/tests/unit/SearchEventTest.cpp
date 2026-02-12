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
#include "../../core/SearchEvent.h"
#include "../../core/SearchId.h"
#include "../../core/SearchResult.h"

using namespace search;

TEST(SearchEventTest, DefaultConstructor) {
    SearchEvent event;

    EXPECT_EQ(event.type, SearchEvent::Type::ERROR_OCCURRED);
    EXPECT_TRUE(event.errorMessage.empty());
    EXPECT_TRUE(event.results.empty());
    EXPECT_FALSE(event.progress.has_value());
}

TEST(SearchEventTest, SearchStarted) {
    SearchId searchId = SearchId::Generate();
    SearchEvent event = SearchEvent::SearchStarted(searchId);

    EXPECT_EQ(event.type, SearchEvent::Type::SEARCH_STARTED);
    EXPECT_EQ(event.searchId, searchId);
}

TEST(SearchEventTest, SearchCompleted) {
    SearchId searchId = SearchId::Generate();
    SearchEvent event = SearchEvent::SearchCompleted(searchId);

    EXPECT_EQ(event.type, SearchEvent::Type::SEARCH_COMPLETED);
    EXPECT_EQ(event.searchId, searchId);
}

TEST(SearchEventTest, SearchFailed) {
    SearchId searchId = SearchId::Generate();
    std::string error = "Test error message";
    SearchEvent event = SearchEvent::SearchFailed(searchId, error);

    EXPECT_EQ(event.type, SearchEvent::Type::SEARCH_FAILED);
    EXPECT_EQ(event.searchId, searchId);
    EXPECT_EQ(event.errorMessage, error);
}

TEST(SearchEventTest, SearchCancelled) {
    SearchId searchId = SearchId::Generate();
    SearchEvent event = SearchEvent::SearchCancelled(searchId);

    EXPECT_EQ(event.type, SearchEvent::Type::SEARCH_CANCELLED);
    EXPECT_EQ(event.searchId, searchId);
}

TEST(SearchEventTest, ResultsReceived) {
    SearchId searchId = SearchId::Generate();
    std::vector<SearchResult> results;

    SearchResult result1;
    result1.searchId = searchId;
    result1.fileName = "file1.mp3";
    result1.fileSize = 1024 * 1024;

    SearchResult result2;
    result2.searchId = searchId;
    result2.fileName = "file2.mp3";
    result2.fileSize = 2 * 1024 * 1024;

    results.push_back(result1);
    results.push_back(result2);

    SearchEvent event = SearchEvent::ResultsReceived(searchId, results);

    EXPECT_EQ(event.type, SearchEvent::Type::RESULTS_RECEIVED);
    EXPECT_EQ(event.searchId, searchId);
    EXPECT_EQ(event.results.size(), 2);
    EXPECT_EQ(event.results[0].fileName, "file1.mp3");
    EXPECT_EQ(event.results[1].fileName, "file2.mp3");
}

TEST(SearchEventTest, ProgressUpdate) {
    SearchId searchId = SearchId::Generate();
    SearchEvent::ProgressInfo progress;
    progress.percentage = 50;
    progress.serversContacted = 5;
    progress.nodesContacted = 10;
    progress.resultsReceived = 100;
    progress.statusMessage = "Searching...";

    SearchEvent event = SearchEvent::ProgressUpdate(searchId, progress);

    EXPECT_EQ(event.type, SearchEvent::Type::PROGRESS_UPDATE);
    EXPECT_EQ(event.searchId, searchId);
    EXPECT_TRUE(event.progress.has_value());
    EXPECT_EQ(event.progress->percentage, 50);
    EXPECT_EQ(event.progress->serversContacted, 5);
    EXPECT_EQ(event.progress->nodesContacted, 10);
    EXPECT_EQ(event.progress->resultsReceived, 100);
    EXPECT_EQ(event.progress->statusMessage, "Searching...");
}

TEST(SearchEventTest, ErrorOccurred) {
    SearchId searchId = SearchId::Generate();
    std::string error = "Network error";
    SearchEvent event = SearchEvent::ErrorOccurred(searchId, error);

    EXPECT_EQ(event.type, SearchEvent::Type::ERROR_OCCURRED);
    EXPECT_EQ(event.searchId, searchId);
    EXPECT_EQ(event.errorMessage, error);
}

TEST(SearchEventTest, SerializeDeserialize) {
    SearchEvent original;
    original.type = SearchEvent::Type::SEARCH_COMPLETED;
    original.searchId = SearchId::Generate();
    original.errorMessage = "Test message";

    SearchResult result;
    result.searchId = original.searchId;
    result.fileName = "test.mp3";
    result.fileSize = 1024 * 1024;
    original.results.push_back(result);

    SearchEvent::ProgressInfo progress;
    progress.percentage = 75;
    progress.statusMessage = "Almost done";
    original.progress = progress;

    auto serialized = original.Serialize();
    SearchEvent deserialized = SearchEvent::Deserialize(serialized);

    EXPECT_EQ(original.type, deserialized.type);
    EXPECT_EQ(original.searchId, deserialized.searchId);
    EXPECT_EQ(original.errorMessage, deserialized.errorMessage);
    EXPECT_EQ(original.results.size(), deserialized.results.size());
    EXPECT_EQ(original.results[0].fileName, deserialized.results[0].fileName);
    EXPECT_TRUE(deserialized.progress.has_value());
    EXPECT_EQ(progress.percentage, deserialized.progress->percentage);
    EXPECT_EQ(progress.statusMessage, deserialized.progress->statusMessage);
}

TEST(SearchEventTest, SerializeDeserializeWithMultipleResults) {
    SearchEvent original;
    original.type = SearchEvent::Type::RESULTS_RECEIVED;
    original.searchId = SearchId::Generate();

    for (int i = 0; i < 10; ++i) {
        SearchResult result;
        result.searchId = original.searchId;
        result.fileName = "file" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        original.results.push_back(result);
    }

    auto serialized = original.Serialize();
    SearchEvent deserialized = SearchEvent::Deserialize(serialized);

    EXPECT_EQ(original.results.size(), deserialized.results.size());
    for (size_t i = 0; i < original.results.size(); ++i) {
        EXPECT_EQ(original.results[i].fileName, deserialized.results[i].fileName);
        EXPECT_EQ(original.results[i].fileSize, deserialized.results[i].fileSize);
    }
}

TEST(SearchEventTest, DeserializeInvalidData) {
    std::vector<uint8_t> invalidData = {0x01, 0x02, 0x03};
    SearchEvent event = SearchEvent::Deserialize(invalidData);

    // Should return default-constructed event
    EXPECT_EQ(event.type, SearchEvent::Type::ERROR_OCCURRED);
    EXPECT_EQ(event.searchId.value, 0);
}
