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

#include "DatabaseFactory.h"
#include "geoip/MaxMindDBDatabase.h"
#include <Logger.h>
#include <libs/common/StringFunctions.h>
#include <libs/common/Format.h>  // Added for CFormat
#include <wx/filename.h>
DatabaseFactory::CreateResult DatabaseFactory::CreateDatabase(DatabaseType type)
{
    switch (type) {
        case DB_TYPE_MAXMIND_DB:
            return CreateResult(std::make_shared<MaxMindDBDatabase>(), true);

        // Legacy GeoIP not supported per user requirements
        case DB_TYPE_LEGACY_GEOIP:
            return CreateResult(nullptr, false, _("Legacy GeoIP format is no longer supported"));

        case DB_TYPE_CSV:
            // TODO: Implement CSV support
            return CreateResult(nullptr, false, _("CSV format support not yet implemented"));

        case DB_TYPE_SQLITE:
            // TODO: Implement SQLite support
            return CreateResult(nullptr, false, _("SQLite format support not yet implemented"));

        case DB_TYPE_UNKNOWN:
        default:
            return CreateResult(nullptr, false, _("Unknown database type"));
    }
}

DatabaseFactory::CreateResult DatabaseFactory::CreateFromFile(const wxString& path)
{
    DetectResult detection = DetectFormat(path);

    if (detection.confidence < 50) {
        return CreateResult(nullptr, false, 
                          CFormat(_("Low confidence format detection: %d%%")) % detection.confidence);
    }

    CreateResult result = CreateDatabase(detection.type);
    if (!result.success) {
        return result;
    }

    if (result.database && result.database->Open(path)) {
        return CreateResult(result.database, true);
    }

    return CreateResult(nullptr, false, _("Failed to open database file"));
}

DatabaseFactory::CreateResult DatabaseFactory::CreateAndOpen(const wxString& path, 
                                                           DatabaseType type)
{
    if (type == DB_TYPE_UNKNOWN) {
        DetectResult detection = DetectFormat(path);
        type = detection.type;
    }

    CreateResult result = CreateDatabase(type);
    if (!result.success) {
        return result;
    }

    if (result.database && result.database->Open(path)) {
        return CreateResult(result.database, true);
    }

    return CreateResult(nullptr, false, _("Failed to open database file"));
}

DatabaseFactory::DetectResult DatabaseFactory::DetectFormat(const wxString& path)
{
    DetectResult result{DB_TYPE_UNKNOWN, 0};

    if (path.IsEmpty()) {
        return result;
    }

    wxFileName fn(path);
    wxString ext = fn.GetExt().Lower();

    if (ext == "mmdb") {
        result.type = DB_TYPE_MAXMIND_DB;
        result.confidence = 90;
        return result;
    }

    if (ext == "dat") {
        result.type = DB_TYPE_LEGACY_GEOIP;
        result.confidence = 80;
        return result;
    }

    if (ext == "csv") {
        result.type = DB_TYPE_CSV;
        result.confidence = 85;
        return result;
    }

    if (ext == "db" || ext == "sqlite") {
        result.type = DB_TYPE_SQLITE;
        result.confidence = 85;
        return result;
    }

    // Try to detect by reading file header
    FILE* fp = wxFopen(path, "rb");
    if (!fp) {
        return result;
    }

    unsigned char header[16];
    size_t read = fread(header, 1, sizeof(header), fp);
    fclose(fp);

    if (read < 4) {
        return result;
    }

    // MaxMind DB magic bytes: 0xDB 0xEE 0x47 0x0F
    if (header[0] == 0xDB && header[1] == 0xEE && header[2] == 0x47 && header[3] == 0x0F) {
        result.type = DB_TYPE_MAXMIND_DB;
        result.confidence = 95;
        return result;
    }

    // Legacy GeoIP: might start with GeoIP Country V6
    if (memcmp(header, "GeoIP Country", 13) == 0) {
        result.type = DB_TYPE_LEGACY_GEOIP;
        result.confidence = 85;
        return result;
    }

    return result;
}

wxString DatabaseFactory::GetFileExtension(DatabaseType type)
{
    switch (type) {
        case DB_TYPE_MAXMIND_DB:
            return ".mmdb";
        case DB_TYPE_LEGACY_GEOIP:
            return ".dat";
        case DB_TYPE_CSV:
            return ".csv";
        case DB_TYPE_SQLITE:
            return ".db";
        default:
            return wxEmptyString;
    }
}

std::vector<DatabaseType> DatabaseFactory::GetSupportedTypes()
{
    return {
        DB_TYPE_MAXMIND_DB,
        DB_TYPE_CSV
    };
}

bool DatabaseFactory::IsSupported(DatabaseType type)
{
    return type == DB_TYPE_MAXMIND_DB || type == DB_TYPE_CSV;
}

std::vector<DatabaseSource> DatabaseFactory::GetDefaultSources()
{
    std::vector<DatabaseSource> sources;
    
    // Primary source: GitHub mirror
    DatabaseSource github;
    github.type = DB_TYPE_MAXMIND_DB;
    github.name = _("GitHub Mirror (8bitsaver)");
    github.url = "https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb";
    github.checksumUrl = "https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb.sha256";
    github.priority = 0;
    github.enabled = true;
    sources.push_back(github);
    
    // Secondary source: jsDelivr CDN
    DatabaseSource jsdelivr;
    jsdelivr.type = DB_TYPE_MAXMIND_DB;
    jsdelivr.name = _("jsDelivr CDN");
    jsdelivr.url = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb";
    jsdelivr.checksumUrl = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb.sha256";
    jsdelivr.priority = 1;
    jsdelivr.enabled = true;
    sources.push_back(jsdelivr);
    
    // Tertiary source: npm package (gzipped)
    DatabaseSource npm;
    npm.type = DB_TYPE_MAXMIND_DB;
    npm.name = _("NPM Package (WP Statistics)");
    npm.url = "https://cdn.jsdelivr.net/npm/geolite2-country/GeoLite2-Country.mmdb.gz";
    npm.checksumUrl = "";
    npm.priority = 2;
    npm.enabled = true;
    sources.push_back(npm);
    
    return sources;
}

DatabaseFactory::ValidateResult DatabaseFactory::ValidateDatabase(const wxString& path, DatabaseType type)
{
    ValidateResult result{false, wxEmptyString};
    
    // Check if file exists
    if (!wxFileExists(path)) {
        result.error = _("Database file does not exist");
        return result;
    }
    
    // Check file size (minimum 1KB)
    wxULongLong fileSize = wxFileName::GetSize(path);
    if (fileSize < 1024) {
        result.error = _("Database file is too small (minimum 1KB required)");
        return result;
    }
    
    // For MaxMind DB, check magic bytes
    if (type == DB_TYPE_MAXMIND_DB || type == DB_TYPE_UNKNOWN) {
        FILE* fp = wxFopen(path, "rb");
        if (fp) {
            unsigned char header[4];
            size_t read = fread(header, 1, 4, fp);
            fclose(fp);
            
            if (read == 4 && header[0] == 0xDB && header[1] == 0xEE && header[2] == 0x47 && header[3] == 0x0F) {
                result.valid = true;
                return result;
            }
        }
        
        if (type == DB_TYPE_MAXMIND_DB) {
            result.error = _("Not a valid MaxMind DB file (missing magic bytes)");
            return result;
        }
    }
    
    // If type is unknown but we got here, it's probably valid
    if (type == DB_TYPE_UNKNOWN) {
        result.valid = true;
        return result;
    }
    
    result.error = _("Unsupported database type validation");
    return result;
}