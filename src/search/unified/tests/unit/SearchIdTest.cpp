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
#include "../../core/SearchId.h"

using namespace search;

TEST(SearchIdTest, GenerateUniqueId) {
    SearchId id1 = SearchId::Generate();
    SearchId id2 = SearchId::Generate();

    EXPECT_NE(id1.value, id2.value);
    EXPECT_TRUE(id1.IsValid());
    EXPECT_TRUE(id2.IsValid());
}

TEST(SearchIdTest, DefaultConstructor) {
    SearchId id;

    EXPECT_EQ(id.value, 0);
    EXPECT_FALSE(id.IsValid());
}

TEST(SearchIdTest, ValueConstructor) {
    SearchId id(42);

    EXPECT_EQ(id.value, 42);
    EXPECT_TRUE(id.IsValid());
}

TEST(SearchIdTest, ComparisonOperators) {
    SearchId id1(1);
    SearchId id2(2);
    SearchId id3(1);

    EXPECT_EQ(id1, id3);
    EXPECT_NE(id1, id2);
    EXPECT_LT(id1, id2);
    EXPECT_LE(id1, id2);
    EXPECT_LE(id1, id3);
    EXPECT_GT(id2, id1);
    EXPECT_GE(id2, id1);
    EXPECT_GE(id3, id1);
}

TEST(SearchIdTest, ToString) {
    SearchId id(12345);
    std::string str = id.ToString();

    EXPECT_EQ(str, "12345");
}

TEST(SearchIdTest, Invalid) {
    SearchId id = SearchId::Invalid();

    EXPECT_EQ(id.value, 0);
    EXPECT_FALSE(id.IsValid());
}

TEST(SearchIdTest, ThreadSafety) {
    const int numThreads = 100;
    const int idsPerThread = 100;
    std::vector<SearchId> allIds;

    auto generateIds = [&]() {
        for (int i = 0; i < idsPerThread; ++i) {
            allIds.push_back(SearchId::Generate());
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(generateIds);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(allIds.size(), numThreads * idsPerThread);

    // Check all IDs are unique
    std::set<uint64_t> uniqueIds;
    for (const auto& id : allIds) {
        EXPECT_TRUE(uniqueIds.insert(id.value).second);
    }
}
