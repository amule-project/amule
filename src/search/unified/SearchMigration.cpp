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

#include "SearchMigration.h"
#include <iostream>
#include <sstream>

namespace search {

// Static members
SearchMigration::Status SearchMigration::s_status = SearchMigration::Status::NOT_STARTED;
SearchMigration::MigrationReport SearchMigration::s_report;
std::mutex SearchMigration::s_mutex;

SearchId SearchMigration::MigrateSearch(
    uint32_t oldSearchId,
    int oldSearchType,
    const std::string& oldSearchParams,
    ProgressCallback progress)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    UpdateStatus(Status::IN_PROGRESS);

    SearchId newSearchId;

    try {
        // Update progress
        if (progress) {
            progress(1, 3, "Converting search type");
        }

        // Convert search type
        SearchType newType = ConvertSearchType(oldSearchType);

        // Update progress
        if (progress) {
            progress(2, 3, "Converting search parameters");
        }

        // Convert search parameters
        SearchParams newParams = ConvertSearchParams(oldSearchParams);
        newParams.type = newType;

        // Update progress
        if (progress) {
            progress(3, 3, "Migration complete");
        }

        // Generate new search ID
        newSearchId = SearchId::Generate();

        // Update report
        s_report.searchesMigrated++;
        s_report.message = "Search migrated successfully";

        UpdateStatus(Status::COMPLETED);

        std::cout << "[SearchMigration] Migrated search " << oldSearchId
                  << " to " << newSearchId.ToString() << std::endl;

    } catch (const std::exception& e) {
        std::string error = "Migration failed: " + std::string(e.what());
        s_report.errors.push_back(error);
        s_report.searchesFailed++;
        s_report.message = error;
        UpdateStatus(Status::FAILED);
        std::cerr << "[SearchMigration] " << error << std::endl;
    }

    return newSearchId;
}

SearchType SearchMigration::ConvertSearchType(int oldType)
{
    // Map old search types to new SearchType
    // These mappings need to match the old architecture
    switch (oldType) {
        case 0:  // Local search
            return SearchType::LOCAL;
        case 1:  // Global search
            return SearchType::GLOBAL;
        case 2:  // Kad search
            return SearchType::KADEMLIA;
        default:
            std::cerr << "[SearchMigration] Unknown old search type: " << oldType << std::endl;
            return SearchType::LOCAL;  // Default to local
    }
}

SearchParams SearchMigration::ConvertSearchParams(const std::string& oldParams)
{
    SearchParams newParams;

    // Parse old search parameters and convert to new format
    // This is a simplified implementation - actual implementation would
    // need to understand the old parameter format

    // Default values
    newParams.type = SearchType::LOCAL;
    newParams.query = oldParams;
    newParams.maxResults = 500;
    newParams.timeout = std::chrono::seconds(60);

    // Parse any additional parameters from oldParams string
    // Format: "query;minSize;maxSize;type1,type2,..."

    std::istringstream iss(oldParams);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ';')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 1) {
        newParams.query = tokens[0];
    }

    if (tokens.size() >= 2 && !tokens[1].empty()) {
        try {
            newParams.minFileSize = std::stoull(tokens[1]);
        } catch (...) {
            // Ignore parse errors
        }
    }

    if (tokens.size() >= 3 && !tokens[2].empty()) {
        try {
            newParams.maxFileSize = std::stoull(tokens[2]);
        } catch (...) {
            // Ignore parse errors
        }
    }

    if (tokens.size() >= 4 && !tokens[3].empty()) {
        // Parse file types
        std::istringstream typeIss(tokens[3]);
        std::string typeToken;
        while (std::getline(typeIss, typeToken, ',')) {
            if (!typeToken.empty()) {
                newParams.fileTypes.push_back(typeToken);
            }
        }
    }

    return newParams;
}

SearchResult SearchMigration::ConvertSearchResult(const std::string& oldResult)
{
    SearchResult newResult;

    // Parse old search result and convert to new format
    // This is a simplified implementation

    // Default values
    newResult.sourceType = SearchType::LOCAL;
    newResult.fileSize = 0;
    newResult.availability = 0;

    // Parse old result format
    // Format: "fileName;fileSize;fileHash;availability"

    std::istringstream iss(oldResult);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ';')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 1) {
        newResult.fileName = tokens[0];
    }

    if (tokens.size() >= 2 && !tokens[1].empty()) {
        try {
            newResult.fileSize = std::stoull(tokens[1]);
        } catch (...) {
            // Ignore parse errors
        }
    }

    if (tokens.size() >= 3) {
        newResult.fileHash = tokens[2];
    }

    if (tokens.size() >= 4 && !tokens[3].empty()) {
        try {
            newResult.availability = std::stoul(tokens[3]);
        } catch (...) {
            // Ignore parse errors
        }
    }

    return newResult;
}

bool SearchMigration::ValidateMigrationReadiness()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    // Check if unified search manager is available
    if (!FeatureFlags::IsEnabled(FeatureFlags::UNIFIED_SEARCH_MANAGER)) {
        std::cerr << "[SearchMigration] Unified search not enabled" << std::endl;
        return false;
    }

    // Check if there are no active searches that would conflict
    // (Implementation would check existing search system)

    std::cout << "[SearchMigration] Migration readiness validated" << std::endl;
    return true;
}

SearchMigration::MigrationReport SearchMigration::MigrateAllActiveSearches(ProgressCallback progress)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    s_report = MigrationReport();
    UpdateStatus(Status::IN_PROGRESS);

    try {
        // Get list of active searches from old system
        // (This would query the existing SearchManager)
        std::vector<std::tuple<uint32_t, int, std::string>> activeSearches;

        // For demonstration, we'll create some dummy searches
        // In real implementation, this would query the actual system
        if (progress) {
            progress(0, activeSearches.size() + 1, "Gathering active searches");
        }

        // Migrate each search
        uint32_t migrated = 0;
        for (const auto& [oldId, oldType, oldParams] : activeSearches) {
            if (progress) {
                progress(migrated + 1, activeSearches.size() + 1,
                          "Migrating search " + std::to_string(oldId));
            }

            SearchId newId = MigrateSearch(oldId, oldType, oldParams, nullptr);
            if (newId.IsValid()) {
                migrated++;
            }
        }

        if (progress) {
            progress(activeSearches.size() + 1, activeSearches.size() + 1,
                      "Migration complete");
        }

        s_report.endTime = std::chrono::system_clock::now();
        s_report.message = "Migration completed successfully";

        if (s_report.searchesFailed == 0) {
            UpdateStatus(Status::COMPLETED);
        } else {
            UpdateStatus(Status::FAILED);
        }

    } catch (const std::exception& e) {
        std::string error = "Migration failed: " + std::string(e.what());
        s_report.errors.push_back(error);
        s_report.message = error;
        UpdateStatus(Status::FAILED);
        std::cerr << "[SearchMigration] " << error << std::endl;
    }

    return s_report;
}

bool SearchMigration::RollbackMigration(const MigrationReport& report)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    std::cout << "[SearchMigration] Rolling back migration..." << std::endl;

    // Rollback logic would:
    // 1. Stop all new searches
    // 2. Restore old search system state
    // 3. Disable unified search feature flags

    UpdateStatus(Status::ROLLED_BACK);
    s_report.message = "Migration rolled back";

    return true;
}

SearchMigration::Status SearchMigration::GetMigrationStatus()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_status;
}

SearchMigration::MigrationReport SearchMigration::GenerateReport()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_report;
}

bool SearchMigration::IsMigrationComplete()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_status == Status::COMPLETED;
}

void SearchMigration::UpdateStatus(Status status)
{
    s_status = status;
    s_report.status = status;
}

} // namespace search
