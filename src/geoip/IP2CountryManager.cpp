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

#include "IP2CountryManager.h"
#include "src/pixmaps/flags_xpm/CountryFlags.h"
#include <Logger.h>
#include <Preferences.h>
#ifdef __WINDOWS__
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif
#include <CFile.h>
#include <Preferences.h>
#include <common/FileFunctions.h>
#include <common/Format.h>
#include <wx/datetime.h> // For wxDateTime
#include <wx/dir.h>      // For wxDir
#include <wx/filefn.h>   // For wxFileExists, wxRenameFile
#include <wx/utils.h>    // For wxExecute
#include <wx/wfstream.h> // For wxFFileOutputStream

// Static member initialization
std::unique_ptr<IP2CountryManager> IP2CountryManager::m_instance = nullptr;

// Constructor
IP2CountryManager::IP2CountryManager()
    : m_configDir(), m_databasePath(),
      m_downloadUrl("https://cdn.jsdelivr.net/gh/8bitsaver/"
                    "maxmind-geoip@release/GeoLite2-Country.mmdb"),
      m_database(nullptr), m_scheduler(nullptr), m_enabled(false),
      m_autoUpdateEnabled(true), m_updateCheckDays(7),
      m_status(DatabaseStatus::NotInitialized),
      m_statusMessage(_("Not initialized")), m_lastError(DatabaseError::None) {}

// Destructor
IP2CountryManager::~IP2CountryManager() {
  // Save current URL to preferences before exiting
  if (!m_downloadUrl.IsEmpty()) {
    thePrefs::SetGeoIPUpdateUrl(m_downloadUrl);
  }

  Disable();
}

IP2CountryManager &IP2CountryManager::GetInstance() {
  if (!m_instance) {
    m_instance = std::make_unique<IP2CountryManager>();
  }
  return *m_instance;
}

void IP2CountryManager::DestroyInstance() { m_instance.reset(); }

bool IP2CountryManager::Initialize(const wxString &configDir) {
  m_configDir = configDir;

  // Ensure trailing slash
  if (!m_configDir.IsEmpty() && !m_configDir.EndsWith("/") &&
      !m_configDir.EndsWith("\\")) {
    m_configDir += "/";
  }

  // Auto-migrate old GeoIP URL from configuration
  AutoMigrateGeoIPUrl();

  // Initialize update scheduler
  m_scheduler = std::make_unique<UpdateScheduler>();
  m_scheduler->Initialize(m_configDir);

  // Set up callbacks
  m_scheduler->SetProgressCallback(
      [this](const UpdateProgress &progress) { OnUpdateProgress(progress); });

  m_scheduler->SetCompletionCallback(
      [this](UpdateCheckResult result, const wxString &message) {
        OnUpdateComplete(result, message);
      });

  // Set default database path
  m_databasePath = m_configDir + "GeoLite2-Country.mmdb";

  AddLogLineN(CFormat(_("IP2Country Manager initialized")));

  // Try to load existing database
  return LoadDatabase();
}

void IP2CountryManager::AutoMigrateGeoIPUrl() {
  // Get the URL from preferences (will be empty if not set)
  wxString configUrl = thePrefs::GetGeoIPUpdateUrl();

  if (configUrl.IsEmpty()) {
    // Use default URL
    AddLogLineN(CFormat(_("IP2Country: Using default download URL: %s")) %
                m_downloadUrl);
    // Set default URL in preferences for future use
    thePrefs::SetGeoIPUpdateUrl(m_downloadUrl);
    return;
  }

  // Use SetDatabaseDownloadUrl to handle automatic migration of obsolete URLs
  wxString oldUrl = configUrl;
  SetDatabaseDownloadUrl(configUrl);

  // If URL was updated, save the new URL to preferences
  if (m_downloadUrl != oldUrl) {
    thePrefs::SetGeoIPUpdateUrl(m_downloadUrl);
    AddLogLineN(
        CFormat(_("IP2Country: Configuration URL migrated and saved: %s")) %
        m_downloadUrl);
  } else {
    AddLogLineN(CFormat(_("IP2Country: Using configured download URL: %s")) %
                m_downloadUrl);
  }
}

void IP2CountryManager::Enable() {
  if (m_enabled) {
    return;
  }

  if (!m_database) {
    // Try to load database
    if (!LoadDatabase()) {
      m_status = DatabaseStatus::Error;
      m_statusMessage = _("Failed to load database");
      AddLogLineC(_("IP2Country: Failed to load database, leaving disabled"));
      return;
    }
  }

  // Load country flags
  if (m_CountryDataMap.empty()) {
    LoadFlags();
  }

  m_enabled = true;
  AddLogLineN(_("IP2Country: Enabled"));
}

void IP2CountryManager::Disable() {
  if (!m_enabled) {
    return;
  }

  m_enabled = false;
  m_database.reset();
  m_status = DatabaseStatus::NotInitialized;
  m_statusMessage = _("Disabled");

  AddLogLineN(_("IP2Country: Disabled"));
}

CountryDataNew IP2CountryManager::GetCountryData(const wxString &ip) {
  CountryDataNew result;

  if (!m_enabled || !m_database || !m_database->IsValid()) {
    // Return unknown country
    result.Code = "unknown";
    result.Name = "?";
    return result;
  }

  if (ip.IsEmpty()) {
    result.Code = "unknown";
    result.Name = "?";
    return result;
  }

  // Get country code from database
  wxString countryCode = m_database->GetCountryCode(ip);

  if (countryCode.IsEmpty()) {
    countryCode = "unknown";
  }

  // Look up in our flag map
  auto it = m_CountryDataMap.find(countryCode.Lower());
  if (it != m_CountryDataMap.end()) {
    result = it->second;
  } else {
    // Unknown country code
    result.Code = countryCode.Lower();
    result.Name = countryCode.Upper();

    // Try to find unknown flag
    auto unknownIt = m_CountryDataMap.find("unknown");
    if (unknownIt != m_CountryDataMap.end()) {
      result.Flag = unknownIt->second.Flag;
    }
  }

  return result;
}

wxString IP2CountryManager::GetCountryCode(const wxString &ip) {
  if (!m_enabled || !m_database) {
    return wxEmptyString;
  }

  return m_database->GetCountryCode(ip);
}

wxString IP2CountryManager::GetCountryName(const wxString &ip) {
  if (!m_enabled || !m_database) {
    return wxEmptyString;
  }

  return m_database->GetCountryName(ip);
}

void IP2CountryManager::CheckForUpdates() {
  if (!m_scheduler) {
    AddLogLineC(_("Update scheduler not initialized"));
    return;
  }

  m_scheduler->CheckForUpdatesAsync();
}

void IP2CountryManager::DownloadUpdate() {
  if (!m_scheduler) {
    AddLogLineC(_("Update scheduler not initialized"));
    return;
  }

  auto sources = UpdateScheduler::GetDefaultSources();
  for (const auto &source : sources) {
    if (source.enabled) {
      m_scheduler->DownloadUpdateAsync(source);
      return;
    }
  }

  AddLogLineC(_("No enabled update sources"));
}

bool IP2CountryManager::Reload() { return LoadDatabase(); }

void IP2CountryManager::SetDatabasePath(const wxString &path) {
  if (m_databasePath == path) {
    return;
  }

  m_databasePath = path;

  // Reload if enabled
  if (m_enabled) {
    LoadDatabase();
  }
}

void IP2CountryManager::SetUpdateCheckInterval(int days) {
  m_updateCheckDays = std::max(1, std::min(days, 30));
}

void IP2CountryManager::SetAutoUpdateEnabled(bool enabled) {
  m_autoUpdateEnabled = enabled;

  if (enabled) {
    AddLogLineN(_("IP2Country: Auto-update enabled"));
  } else {
    AddLogLineN(_("IP2Country: Auto-update disabled"));
  }
}

void IP2CountryManager::LoadFlags() {
  m_CountryDataMap.clear();

  // Load data from xpm files
  for (int i = 0; i < flags::FLAGS_XPM_SIZE; ++i) {
    CountryDataNew countrydata;
    countrydata.Code =
        wxString(flags::flagXPMCodeVector[i].code, wxConvISO8859_1);
    countrydata.Flag = wxImage(flags::flagXPMCodeVector[i].xpm);
    countrydata.Name = countrydata.Code;

    if (countrydata.Flag.IsOk()) {
      m_CountryDataMap[countrydata.Code] = countrydata;
    } else {
      AddLogLineC(CFormat(_("Failed to load flag for country code: %s")) %
                  countrydata.Code);
    }
  }

  AddDebugLogLineN(logGeneral,
                   CFormat(wxT("IP2Country: Loaded %d country flags")) %
                       m_CountryDataMap.size());
}

wxString IP2CountryManager::GetErrorMessage(DatabaseError error) {
  switch (error) {
  case DatabaseError::None:
    return _("No error");
  case DatabaseError::NetworkError:
    return _(
        "Network connection failed. Please check your internet connection.");
  case DatabaseError::FilePermissionError:
    return _("File permission error. Please check write permissions for the "
             "configuration directory.");
  case DatabaseError::DiskSpaceError:
    return _("Not enough disk space to download the database.");
  case DatabaseError::CorruptDatabase:
    return _("Downloaded database is corrupted. Please try downloading again.");
  case DatabaseError::ServerError:
    return _(
        "Server error. The database server may be temporarily unavailable.");
  case DatabaseError::Timeout:
    return _("Download timed out. Please check your network connection and try "
             "again.");
  case DatabaseError::UnknownError:
  default:
    return _("Unknown error occurred");
  }
}

bool IP2CountryManager::TestDatabase(const wxString &testIP,
                                     wxString &resultCountry,
                                     wxString &errorMessage) {
  if (!m_database || !m_database->IsValid()) {
    errorMessage = _("Database not loaded or invalid");
    return false;
  }

  if (testIP.IsEmpty()) {
    errorMessage = _("Empty IP address provided");
    return false;
  }

  CountryDataNew data = GetCountryData(testIP);
  if (data.Code == "unknown") {
    errorMessage = CFormat(_("Unknown country for IP: %s")) % testIP;
    return false;
  }

  resultCountry = data.Name;
  errorMessage.Clear();
  return true;
}

bool IP2CountryManager::LoadDatabase() {
  m_status = DatabaseStatus::Loading;
  m_statusMessage = _("Loading database...");
  m_lastError = DatabaseError::None;

  // Try to open the database
  auto result = DatabaseFactory::CreateAndOpen(m_databasePath);
  m_database = result.database;

  if (!m_database) {
    // Database doesn't exist - try to find one in config dir
    wxDir dir(m_configDir);
    if (dir.IsOpened()) {
      wxString filename;
      bool cont = dir.GetFirst(&filename, "*.mmdb", wxDIR_FILES);
      while (cont) {
        wxString fullPath = m_configDir + filename;
        auto searchResult = DatabaseFactory::CreateAndOpen(fullPath);
        if (searchResult.database) {
          m_database = searchResult.database;
          m_databasePath = fullPath;
          AddLogLineN(CFormat(_("Found database at: %s")) % fullPath);
          break;
        }
        cont = dir.GetNext(&filename);
      }
    }
  }

  if (!m_database) {
    AddLogLineN(CFormat(_("No GeoIP database found at: %s")) % m_databasePath);
    m_status = DatabaseStatus::Downloading;
    m_statusMessage = _("Downloading database...");

    // Attempt automatic download
    if (!DownloadDatabase()) {
      m_status = DatabaseStatus::DownloadFailed;
      m_statusMessage = _("Failed to download database");
      AddLogLineN(_("Failed to download GeoIP database automatically."));
      AddLogLineN(_("You can download the GeoLite2-Country database from:"));
      AddLogLineN(_("  - https://github.com/8bitsaver/maxmind-geoip"));
      return false;
    }
  }

  AddLogLineN(CFormat(_("IP2Country: Loaded database from %s")) %
              m_databasePath);
  AddLogLineN(CFormat(_("IP2Country: Database version: %s")) %
              m_database->GetVersion());
  AddLogLineN(CFormat(_("IP2Country: Database format: %s")) %
              m_database->GetFormatName());

  m_status = DatabaseStatus::Ready;
  m_statusMessage =
      CFormat(_("Database ready (version: %s)")) % m_database->GetVersion();

  return true;
}

void IP2CountryManager::OnUpdateProgress(const UpdateProgress &progress) {
  if (progress.inProgress) {
    // Only show progress if we have meaningful byte counts
    if (progress.totalBytes > 0) {
      wxString status = CFormat(_("Downloading GeoIP: %d%% (%s / %s)")) %
                        progress.percentComplete %
                        CastItoXBytes(progress.bytesDownloaded) %
                        CastItoXBytes(progress.totalBytes);
      AddLogLineN(status);
    } else if (progress.bytesDownloaded > 0) {
      // Show only downloaded bytes if total is unknown
      wxString status = CFormat(_("Downloading GeoIP: %s received")) %
                        CastItoXBytes(progress.bytesDownloaded);
      AddLogLineN(status);
    } else {
      // Minimal status for unknown progress
      wxString status =
          CFormat(_("Downloading GeoIP: %d%%")) % progress.percentComplete;
      AddLogLineN(status);
    }
  } else {
    AddLogLineN(progress.statusMessage);
  }
}

void IP2CountryManager::OnUpdateComplete(UpdateCheckResult result,
                                         const wxString &message) {
  switch (result) {
  case UpdateCheckResult::UpdateAvailable:
    AddLogLineN(CFormat(_("Update available: %s")) % message);
    break;

  case UpdateCheckResult::NoUpdate:
    AddLogLineN(CFormat(_("No update available: %s")) % message);
    break;

  case UpdateCheckResult::NetworkError:
    AddLogLineC(CFormat(_("Network error during update: %s")) % message);
    break;

  case UpdateCheckResult::Error:
  default:
    AddLogLineC(CFormat(_("Update error: %s")) % message);
    break;
  }

  // If update was successful, reload the database
  if (result == UpdateCheckResult::UpdateAvailable) {
    if (Reload()) {
      AddLogLineN(_("IP2Country: Database reloaded after update"));
    }
  }
}

void IP2CountryManager::SetDatabaseDownloadUrl(const wxString &url) {
  if (!url.IsEmpty() && url != m_downloadUrl) {
    // Check and update obsolete URLs - IMPORTANT for user migration
    wxString newUrl = url;

    // Update obsolete MaxMind URLs
    if (newUrl.Contains("geolite.maxmind.com") ||
        newUrl.Contains("GeoIP.dat.gz")) {

      AddLogLineN(_("IP2Country: Detected obsolete GeoIP URL - auto-updating "
                    "to new format"));

      // Replace with new URL format
      newUrl = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/"
               "GeoLite2-Country.mmdb";
    }

    m_downloadUrl = newUrl;
  }
}

bool IP2CountryManager::DownloadDatabase() {
  AddLogLineN(
      _("Attempting to download GeoLite2-Country database automatically..."));

  // Try to download the database - log start only
  AddLogLineN(_("Starting download..."));

  // Download the database synchronously
  wxString tempPath = m_configDir + "GeoLite2-Country.mmdb.temp";
  bool downloadSuccess = false;

  AddLogLineN(CFormat(_("Downloading from: %s")) % m_downloadUrl);

  // Remove any existing temp file
  if (wxFileExists(tempPath)) {
    if (!wxRemoveFile(tempPath)) {
      m_lastError = DatabaseError::FilePermissionError;
      AddLogLineC(_("Failed to remove existing temporary file"));
      return false;
    }
  }

  // Set timeout (5 minutes max)
  wxDateTime startTime = wxDateTime::Now();
  const int MAX_DOWNLOAD_MINUTES = 5;

  // Platform-specific download implementations
#ifdef __WINDOWS__
  // Windows implementation using WinINet with timeout
  HINTERNET hInternet =
      InternetOpen(wxT("aMule"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

  if (hInternet) {
    HINTERNET hConnect =
        InternetOpenUrl(hInternet, m_downloadUrl.wc_str(), NULL, 0,
                        INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, 0);

    if (hConnect) {
      // Set timeout for the connection
      DWORD timeout = 30000; // 30 seconds
      InternetSetOption(hConnect, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
                        sizeof(timeout));
      InternetSetOption(hConnect, INTERNET_OPTION_SEND_TIMEOUT, &timeout,
                        sizeof(timeout));
      InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout,
                        sizeof(timeout));

      DWORD bytesRead;
      char buffer[4096];
      HANDLE hFile = CreateFile(tempPath.wc_str(), GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

      if (hFile != INVALID_HANDLE_VALUE) {
        bool downloading = true;
        while (downloading) {
          // Check for overall timeout
          if (wxDateTime::Now().Subtract(startTime).GetMinutes() >=
              MAX_DOWNLOAD_MINUTES) {
            m_lastError = DatabaseError::Timeout;
            AddLogLineC(_("Download timed out after 5 minutes"));
            CloseHandle(hFile);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
          }

          // Read with timeout
          if (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead)) {
            if (bytesRead > 0) {
              DWORD written;
              WriteFile(hFile, buffer, bytesRead, &written, NULL);

              // Update progress every 64KB
              static size_t totalRead = 0;
              totalRead += bytesRead;
              if (totalRead % (64 * 1024) == 0) {
                AddLogLineN(CFormat(_("Downloaded %s so far...")) %
                            CastItoXBytes(totalRead));
              }
            } else {
              // No more data
              downloading = false;
            }
          } else {
            // Read failed, check if it's a timeout
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING || error == WAIT_TIMEOUT) {
              // Continue waiting
              Sleep(100);
            } else {
              // Other error
              AddLogLineC(CFormat(_("Download error: %d")) % error);
              downloading = false;
              downloadSuccess = false;
            }
          }

          // Small delay to prevent CPU hogging
          Sleep(10);
        }
        CloseHandle(hFile);
        downloadSuccess = true;
      }
      InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hInternet);
  }
#else
  // Unix implementation using curl/wget with robust timeout handling
  wxString curlCommand = wxString::Format(
      "curl -L --connect-timeout 30 --max-time 180 -o \"%s\" \"%s\"", tempPath,
      m_downloadUrl);

  // Check for timeout during execution
  wxStopWatch timer;
  int result = wxExecute(curlCommand, wxEXEC_SYNC);

  // Check for execution timeout (4 minutes max)
  if (timer.Time() > 240000) { // 4 minutes in milliseconds
    m_lastError = DatabaseError::Timeout;
    AddLogLineC(_("Download timed out after 4 minutes"));
    downloadSuccess = false;
  } else if (result != 0 || !wxFileExists(tempPath)) {
    // Fallback to wget if curl fails - use timeout options
    wxString wgetCommand =
        wxString::Format("wget --timeout=30 --tries=3 -O \"%s\" \"%s\"",
                         tempPath, m_downloadUrl);
    result = wxExecute(wgetCommand, wxEXEC_SYNC);
    if (result == 0 && wxFileExists(tempPath)) {
      downloadSuccess = true;
    } else {
      downloadSuccess = false;
      m_lastError = DatabaseError::NetworkError;
      AddLogLineC(CFormat(_("Download failed with error code: %d")) % result);

      // Check for wget-specific error messages
      if (result == 4) {
        AddLogLineC(_("wget: Network failure (DNS resolution, connection "
                      "refused, etc.)"));
      } else if (result == 8) {
        m_lastError = DatabaseError::ServerError;
        AddLogLineC(
            _("wget: Server returned error response (404 Not Found, etc.)"));
      }
    }
  } else {
    downloadSuccess = true;
  }
#endif

  // Process downloaded file
  if (downloadSuccess && wxFileExists(tempPath)) {
    // Check if file is gzipped by extension or magic number
    bool isGzipped = m_downloadUrl.EndsWith(".gz");
    if (!isGzipped) {
      // Check magic number for gzip (0x1f 0x8b)
      wxFile file(tempPath);
      if (file.IsOpened()) {
        unsigned char magic[2];
        if (file.Read(magic, 2) == 2) {
          isGzipped = (magic[0] == 0x1F && magic[1] == 0x8B);
        }
        file.Close();
      }
    }

    wxString finalTempPath = tempPath;

    // Extract gzip if needed
    if (isGzipped) {
      wxString gzipPath = tempPath;
      if (!gzipPath.EndsWith(".gz")) {
        gzipPath += ".gz";
      }

      // Rename to .gz if needed
      if (wxFileExists(tempPath) && !wxFileExists(gzipPath)) {
        wxRenameFile(tempPath, gzipPath);
      }

#ifdef __WINDOWS__
      // Windows: Skip decompression and use the file as-is
      AddLogLineN(
          _("Windows: Skipping gzip decompression - will use file directly"));
      finalTempPath = gzipPath;
#else
      // Unix-like systems: use gunzip
      wxString extractCommand = wxString::Format("gunzip -f \"%s\"", gzipPath);
      int result = wxExecute(extractCommand, wxEXEC_SYNC);
      if (result != 0) {
        m_lastError = DatabaseError::CorruptDatabase;
        AddLogLineC(_("Failed to extract gzip file"));
        wxRemoveFile(gzipPath);
        return false;
      }
      finalTempPath = gzipPath.BeforeLast('.');
#endif
    }

    // Move to final location
    if (wxFileExists(finalTempPath)) {
      if (wxRenameFile(finalTempPath, m_databasePath)) {
        AddLogLineN(_("Database downloaded successfully!"));

        // Try to load the downloaded database
        auto loadResult = DatabaseFactory::CreateAndOpen(m_databasePath);
        m_database = loadResult.database;
        downloadSuccess = (m_database != nullptr);
        if (!downloadSuccess) {
          m_lastError = DatabaseError::CorruptDatabase;
          AddLogLineC(_("Downloaded database is corrupted or invalid"));
        }
      } else {
        m_lastError = DatabaseError::FilePermissionError;
        AddLogLineC(_("Failed to move database to final location"));
        downloadSuccess = false;
      }
    } else {
      AddLogLineC(_("Downloaded file not found after processing"));
      downloadSuccess = false;
    }
  }

  // Log completion status directly
  AddLogLineN(_("Download completed"));

  return downloadSuccess && m_database != nullptr;
}
