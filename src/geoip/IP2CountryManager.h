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

#ifndef IP2COUNTRYMANAGER_H
#define IP2COUNTRYMANAGER_H

#include "DatabaseFactory.h"
#include "IGeoIPDatabase.h"
#include "UpdateScheduler.h"
#include <map>
#include <memory>
#include <wx/image.h> // Include wxImage
#include <wx/string.h>

// Use the same structure as the legacy code for compatibility
typedef struct {
  wxString Name; ///< Country name (e.g., "United States")
  wxString Code; ///< ISO 3166-1 alpha-2 country code (e.g., "us")
  wxImage Flag;  ///< Flag image
} CountryDataNew;

/**
 * @brief Map of country code to country data
 */
typedef std::map<wxString, CountryDataNew> CountryDataMap;

/**
 * @brief Database status enumeration
 */
enum class DatabaseStatus {
  NotInitialized, ///< Database not initialized
  Loading,        ///< Database is loading
  Ready,          ///< Database is ready for use
  Downloading,    ///< Database is being downloaded
  DownloadFailed, ///< Database download failed
  Outdated,       ///< Database is outdated (update available)
  Error           ///< Database error occurred
};

/**
 * @brief Database error enumeration
 */
enum class DatabaseError {
  None,                ///< No error
  NetworkError,        ///< Network connection failed
  FilePermissionError, ///< File permission error
  DiskSpaceError,      ///< Not enough disk space
  CorruptDatabase,     ///< Database is corrupted
  ServerError,         ///< Server error
  Timeout,             ///< Download timeout
  UnknownError         ///< Unknown error
};

/**
 * @brief Main manager for IP to country lookups
 */
class IP2CountryManager {
public:
  /**
   * @brief Get singleton instance
   * @return Reference to singleton instance
   */
  static IP2CountryManager &GetInstance();

  /**
   * @brief Destroy singleton instance
   */
  static void DestroyInstance();

  /**
   * @brief Initialize the manager
   * @param configDir Configuration directory
   * @return true if successful
   */
  bool Initialize(const wxString &configDir);

  /**
   * @brief Auto-migrate GeoIP URL from configuration
   * Automatically detects and updates obsolete URLs
   */
  void AutoMigrateGeoIPUrl();

  /**
   * @brief Enable IP2Country functionality
   */
  void Enable();

  /**
   * @brief Disable IP2Country functionality
   */
  void Disable();

  /**
   * @brief Check if IP2Country is enabled
   * @return true if enabled
   */
  bool IsEnabled() const {
    return m_enabled && m_database && m_database->IsValid();
  }

  /**
   * @brief Get country data for an IP address
   * @param ip IP address string
   * @return Country data
   */
  CountryDataNew GetCountryData(const wxString &ip);

  /**
   * @brief Get country code for an IP address
   * @param ip IP address string
   * @return Country code (lowercase) or empty string
   */
  wxString GetCountryCode(const wxString &ip);

  /**
   * @brief Get country name for an IP address
   * @param ip IP address string
   * @return Country name or empty string
   */
  wxString GetCountryName(const wxString &ip);

  /**
   * @brief Check for database updates
   */
  void CheckForUpdates();
  /**
   * @brief Download and install update
   */
  void DownloadUpdate();

  /**
   * @brief Download database automatically
   * @return true if download and installation successful
   */
  bool DownloadDatabase();

  /**
   * @brief Reload the database
   * @return true if successful
   */
  bool Reload();

  /**
   * @brief Get current database type
   * @return Database type
   */
  DatabaseType GetDatabaseType() const {
    return m_database ? m_database->GetType() : DB_TYPE_UNKNOWN;
  }

  /**
   * @brief Get current database version
   * @return Version string
   */
  wxString GetDatabaseVersion() const {
    return m_database ? m_database->GetVersion() : wxString(wxEmptyString);
  }

  /**
   * @brief Get last update check time
   * @return DateTime or invalid if never
   */
  wxDateTime GetLastUpdateCheck() const {
    return m_scheduler ? m_scheduler->GetLastCheckTime() : wxDateTime();
  }

  /**
   * @brief Set custom database path
   * @param path Path to database file
   */
  void SetDatabasePath(const wxString &path);

  /**
   * @brief Get current database path
   * @return Path to database file
   */
  wxString GetDatabasePath() const { return m_databasePath; }

  /**
   * @brief Set update check interval
   * @param days Number of days between checks
   */
  void SetUpdateCheckInterval(int days);

  /**
   * @brief Set database download URL
   * @param url Download URL for GeoIP database
   */
  void SetDatabaseDownloadUrl(const wxString &url);

  /**
   * @brief Get database download URL
   * @return Download URL for GeoIP database
   */
  wxString GetDatabaseDownloadUrl() const { return m_downloadUrl; }

  /**
   * @brief Get update check interval
   * @return Days between checks
   */
  int GetUpdateCheckInterval() const { return m_updateCheckDays; }

  /**
   * @brief Enable or disable automatic updates
   * @param enabled true to enable
   */
  void SetAutoUpdateEnabled(bool enabled);

  /**
   * @brief Check if automatic updates are enabled
   * @return true if enabled
   */
  bool IsAutoUpdateEnabled() const { return m_autoUpdateEnabled; }

  /**
   * @brief Get current database status
   * @return Database status
   */
  DatabaseStatus GetDatabaseStatus() const { return m_status; }

  /**
   * @brief Get status message
   * @return Status message string
   */
  wxString GetStatusMessage() const { return m_statusMessage; }

  /**
   * @brief Get last database error
   * @return Last error type
   */
  DatabaseError GetLastError() const { return m_lastError; }

  /**
   * @brief Get error message for an error type
   * @param error Error type
   * @return Error message string
   */
  static wxString GetErrorMessage(DatabaseError error);

  /**
   * @brief Test database with a sample IP address
   * @param testIP IP address to test
   * @param resultCountry Output parameter for country name
   * @param errorMessage Output parameter for error message
   * @return true if test successful
   */
  bool TestDatabase(const wxString &testIP, wxString &resultCountry,
                    wxString &errorMessage);

public:
  IP2CountryManager();
  ~IP2CountryManager();

private:
  // Disable copying
  IP2CountryManager(const IP2CountryManager &) = delete;
  IP2CountryManager &operator=(const IP2CountryManager &) = delete;

  /**
   * @brief Load country flags from resources
   */
  void LoadFlags();

  /**
   * @brief Load database from current path
   * @return true if successful
   */
  bool LoadDatabase();

  static std::unique_ptr<IP2CountryManager> m_instance; ///< Singleton instance
  wxString m_configDir;                         ///< Configuration directory
  wxString m_databasePath;                      ///< Current database path
  wxString m_downloadUrl;                       ///< Database download URL
  std::shared_ptr<IGeoIPDatabase> m_database;   ///< Database instance
  std::unique_ptr<UpdateScheduler> m_scheduler; ///< Update scheduler
  CountryDataMap m_CountryDataMap;              ///< Country flag data
  bool m_enabled;            ///< Whether functionality is enabled
  bool m_autoUpdateEnabled;  ///< Auto-update enabled
  int m_updateCheckDays;     ///< Days between update checks
  DatabaseStatus m_status;   ///< Current database status
  wxString m_statusMessage;  ///< Current status message
  DatabaseError m_lastError; ///< Last error that occurred

  // Update callback handlers
  void OnUpdateProgress(const UpdateProgress &progress);
  void OnUpdateComplete(UpdateCheckResult result, const wxString &message);
};

// Inline access function for convenience
inline IP2CountryManager &IP2Country() {
  return IP2CountryManager::GetInstance();
}

#endif // IP2COUNTRYMANAGER_H