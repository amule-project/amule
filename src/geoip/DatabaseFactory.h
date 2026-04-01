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

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include "IGeoIPDatabase.h"
#include <wx/string.h>
#include <memory>

/**
 * @brief Database source configuration
 */
struct DatabaseSource {
    DatabaseType type;        ///< Database format type
    wxString name;           ///< Human-readable name
    wxString url;           ///< Download URL
    wxString checksumUrl;   ///< Checksum URL for verification
    int priority;           ///< Priority (lower = higher priority)
    bool enabled;           ///< Whether this source is enabled
    int timeout;            ///< Download timeout in seconds

    DatabaseSource() : type(DB_TYPE_UNKNOWN), priority(100), enabled(true), timeout(30) {}
};

/**
 * @brief Factory for creating GeoIP database instances
 */
class DatabaseFactory
{
public:
    /**
     * @brief Factory result with database instance and status
     */
    struct CreateResult {
        std::shared_ptr<IGeoIPDatabase> database;
        bool success;
        wxString errorMessage;
        
        CreateResult(std::shared_ptr<IGeoIPDatabase> db = nullptr, 
                    bool s = false, 
                    const wxString& msg = wxEmptyString)
            : database(db), success(s), errorMessage(msg) {}
    };

    /**
     * @brief Create a database instance for the given type
     * @param type Database type
     * @return Factory result with database and status
     */
    static CreateResult CreateDatabase(DatabaseType type);

    /**
     * @brief Create a database instance from file (auto-detection)
     * @param path Path to database file
     * @return Factory result with database and status
     */
    static CreateResult CreateFromFile(const wxString& path);

    /**
     * @brief Create and initialize database from file
     * @param path Path to database file
     * @param type Database type (if known)
     * @return Factory result with initialized database
     */
    static CreateResult CreateAndOpen(const wxString& path, 
                                    DatabaseType type = DB_TYPE_UNKNOWN);

    /**
     * @brief Format detection result
     */
    struct DetectResult {
        DatabaseType type;
        int confidence; // 0-100 confidence level
    };

    /**
     * @brief Detect database format from file
     * @param path Path to database file
     * @return Detected database type and confidence level
     */
    static DetectResult DetectFormat(const wxString& path);

    /**
     * @brief Get file extension for database type
     * @param type Database type
     * @return File extension with dot (e.g., ".mmdb")
     */
    static wxString GetFileExtension(DatabaseType type);

    /**
     * @brief Get supported database types
     * @return Vector of supported database types
     */
    static std::vector<DatabaseType> GetSupportedTypes();

    /**
     * @brief Check if database type is supported
     * @param type Database type
     * @return true if supported
     */
    static bool IsSupported(DatabaseType type);

    /**
     * @brief Get default database sources
     * @return Vector of available database sources
     */
    static std::vector<DatabaseSource> GetDefaultSources();

    /**
     * @brief Validation result
     */
    struct ValidateResult {
        bool valid;
        wxString error;
    };

    /**
     * @brief Validate database file
     * @param path Path to database file
     * @param type Expected database type
     * @return Validation result with error message if failed
     */
    static ValidateResult ValidateDatabase(const wxString& path, DatabaseType type);
};

#endif // DATABASEFACTORY_H