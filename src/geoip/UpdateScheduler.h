//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
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

#ifndef UPDATESCHEDULER_H
#define UPDATESCHEDULER_H

#include "DatabaseFactory.h"
#include <wx/datetime.h>
#include <wx/string.h>
#include <functional>
#include <vector>

/**
 * @brief Update check result
 */
enum class UpdateCheckResult {
    UpdateAvailable,   ///< New version available
    NoUpdate,          ///< No newer version
    Error,             ///< Error occurred
    NetworkError       ///< Network error
};

/**
 * @brief Update progress callback
 */
struct UpdateProgress {
    bool inProgress;
    int percentComplete;
    wxString statusMessage;
    wxString currentFile;
    long bytesDownloaded;
    long totalBytes;
};

/**
 * @brief Scheduler for automatic database updates
 */
class UpdateScheduler
{
public:
    /**
     * @brief Callback type for progress updates
     */
    typedef std::function<void(const UpdateProgress& progress)> ProgressCallback;

    /**
     * @brief Callback type for completion
     */
    typedef std::function<void(UpdateCheckResult result, const wxString& message)> CompletionCallback;

    UpdateScheduler();
    ~UpdateScheduler();

    /**
     * @brief Initialize the scheduler
     * @param configDir Configuration directory
     */
    void Initialize(const wxString& configDir);

    /**
     * @brief Set progress callback
     * @param callback Progress callback function
     */
    void SetProgressCallback(ProgressCallback callback);

    /**
     * @brief Set completion callback
     * @param callback Completion callback function
     */
    void SetCompletionCallback(CompletionCallback callback);

    /**
     * @brief Check for updates asynchronously
     * @param callback Called when check is complete
     */
    void CheckForUpdatesAsync(CompletionCallback callback = nullptr);

    /**
     * @brief Download and install update
     * @param source Database source to download from
     * @param callback Called when download is complete
     */
    void DownloadUpdateAsync(const DatabaseSource& source, CompletionCallback callback = nullptr);

    /**
     * @brief Check if update is in progress
     * @return true if updating
     */
    bool IsUpdating() const { return m_updateInProgress; }

    /**
     * @brief Cancel current update
     */
    void CancelUpdate();

    /**
     * @brief Get default database sources
     * @return Vector of default sources
     */
    static std::vector<DatabaseSource> GetDefaultSources();

    /**
     * @brief Get last update check time
     * @return Last check time or invalid date if never
     */
    wxDateTime GetLastCheckTime() const { return m_lastCheckTime; }

    /**
     * @brief Get last successful update time
     * @return Last update time or invalid date if never
     */
    wxDateTime GetLastUpdateTime() const { return m_lastUpdateTime; }

    /**
     * @brief Get current progress
     * @return Current progress info
     */
    UpdateProgress GetProgress() const { return m_progress; }

private:
    // Helper methods
    void ReportDownloadError(const wxString& message, const wxString& detail);
    void NotifyProgress();

    wxString m_configDir;
    ProgressCallback m_progressCallback;
    CompletionCallback m_completionCallback;
    bool m_updateInProgress;
    bool m_cancelled;
    UpdateProgress m_progress;
    wxDateTime m_lastCheckTime;
    wxDateTime m_lastUpdateTime;

    /**
     * @brief Validate downloaded file using checksum
     * @param filePath Path to downloaded file
     * @param checksum Expected checksum
     * @return true if valid
     */
    bool ValidateFile(const wxString& filePath, const wxString& checksum);

    /**
     * @brief Download file from URL
     * @param url URL to download
     * @param destPath Destination path
     * @return true if successful
     */
    bool DownloadFile(const wxString& url, const wxString& destPath);

    /**
     * @brief Decompress file if needed
     * @param filePath Path to file
     * @param outputPath Output path
     * @return true if successful
     */
    bool DecompressFile(const wxString& filePath, const wxString& outputPath);

    /**
     * @brief Get database path
     * @return Path to database file
     */
    wxString GetDatabasePath() const { return m_configDir + "GeoLite2-Country.mmdb"; }

    /**
     * @brief Get temporary file path
     * @return Path for temporary download
     */
    wxString GetTempPath() const { return m_configDir + "GeoLite2-Country.mmdb.download"; }
};

#endif // UPDATESCHEDULER_H