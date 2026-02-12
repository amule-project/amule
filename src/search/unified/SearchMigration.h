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

#ifndef SEARCH_MIGRATION_H
#define SEARCH_MIGRATION_H

#include "../core/SearchTypes.h"
#include "../core/SearchId.h"
#include "../core/SearchParams.h"
#include "../core/SearchResult.h"

#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace search {

/**
 * Migration utilities for transitioning from old search architecture
 * to new unified search architecture
 */
class SearchMigration {
public:
    /**
     * Status of migration
     */
    enum class Status {
        NOT_STARTED,
        IN_PROGRESS,
        COMPLETED,
        FAILED,
        ROLLED_BACK
    };

    /**
     * Migration report
     */
    struct MigrationReport {
        Status status;
        std::string message;
        uint32_t searchesMigrated;
        uint32_t searchesFailed;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
        std::vector<std::string> errors;

        MigrationReport()
            : status(Status::NOT_STARTED)
            , searchesMigrated(0)
            , searchesFailed(0)
            , startTime(std::chrono::system_clock::now())
            , endTime(std::chrono::system_clock::now())
        {}
    };

    /**
     * Progress callback for migration operations
     * @param current Current step number
     * @param total Total number of steps
     * @param message Status message
     */
    using ProgressCallback = std::function<void(uint32_t current, uint32_t total, const std::string& message)>;

    /**
     * Migrate old search to new architecture
     * @param oldSearchId Old search ID (from existing system)
     * @param oldSearchType Old search type
     * @param oldSearchParams Old search parameters
     * @param progress Optional progress callback
     * @return New search ID in unified architecture
     */
    static SearchId MigrateSearch(
        uint32_t oldSearchId,
        int oldSearchType,
        const std::string& oldSearchParams,
        ProgressCallback progress = nullptr
    );

    /**
     * Convert old search type to new SearchType
     * @param oldType Old search type
     * @return New SearchType
     */
    static SearchType ConvertSearchType(int oldType);

    /**
     * Convert old search parameters to new SearchParams
     * @param oldParams Old search parameters
     * @return New SearchParams
     */
    static SearchParams ConvertSearchParams(const std::string& oldParams);

    /**
     * Convert old search result to new SearchResult
     * @param oldResult Old search result
     * @return New SearchResult
     */
    static SearchResult ConvertSearchResult(const std::string& oldResult);

    /**
     * Validate migration readiness
     * @return true if system is ready for migration
     */
    static bool ValidateMigrationReadiness();

    /**
     * Perform full migration of all active searches
     * @param progress Optional progress callback
     * @return Migration report
     */
    static MigrationReport MigrateAllActiveSearches(ProgressCallback progress = nullptr);

    /**
     * Rollback migration
     * @param report Migration report to rollback
     * @return true if rollback was successful
     */
    static bool RollbackMigration(const MigrationReport& report);

    /**
     * Get migration status
     * @return Current migration status
     */
    static Status GetMigrationStatus();

    /**
     * Generate migration report
     * @return Current migration report
     */
    static MigrationReport GenerateReport();

    /**
     * Check if migration is complete
     * @return true if migration is complete
     */
    static bool IsMigrationComplete();

private:
    // Internal migration state
    static Status s_status;
    static MigrationReport s_report;
    static std::mutex s_mutex;

    /**
     * Update migration status
     * @param status New status
     */
    static void UpdateStatus(Status status);
};

} // namespace search

#endif // SEARCH_MIGRATION_H
