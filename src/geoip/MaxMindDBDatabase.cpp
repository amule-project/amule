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

#include "geoip/MaxMindDBDatabase.h"
#include <libs/common/StringFunctions.h>
#include <libs/common/Format.h>
#include <Logger.h>
#include <wx/intl.h>    // For _() macro
#include <wx/strconv.h>  // For wxConvUTF8

#include <algorithm>
#include <cstring>

#ifdef ENABLE_MAXMINDDB
#include <maxminddb.h>
#endif

MaxMindDBDatabase::MaxMindDBDatabase()
    : m_isOpen(false)
{
#ifdef ENABLE_MAXMINDDB
    memset(&m_mmdb, 0, sizeof(m_mmdb));  // Initialize struct to zero
#endif
}

std::vector<wxString> MaxMindDBDatabase::BatchGetCountryCodes(
    const std::vector<wxString>& ips)
{
    std::vector<wxString> results;
    results.reserve(ips.size());
    
#ifdef ENABLE_MAXMINDDB
    if (!m_isOpen) {
        results.assign(ips.size(), wxEmptyString);
        return results;
    }

    for (const auto& ip : ips) {
        wxString code, name;
        if (LookupCountry(ip, code, name)) {
            results.push_back(code);
        } else {
            results.push_back(wxEmptyString);
        }
    }
#else
    results.assign(ips.size(), wxT("MaxMind DB (not compiled)"));
#endif

    return results;
}

MaxMindDBDatabase::~MaxMindDBDatabase()
{
    Close();
}

bool MaxMindDBDatabase::Open(const wxString& path)
{
#ifdef ENABLE_MAXMINDDB
    Close();

    if (path.IsEmpty()) {
        AddLogLineC(_("MaxMind DB: Empty path provided"));
        return false;
    }

    // Convert wxString to char* for MaxMind DB API
    std::string path8bit = std::string(path.mb_str(wxConvUTF8));
    int status = MMDB_open(path8bit.c_str(), MMDB_MODE_MMAP, &m_mmdb);

    if (status != MMDB_SUCCESS) {
        AddLogLineC(CFormat(_("MaxMind DB: Failed to open database: %s")) % wxString::FromUTF8(MMDB_strerror(status)));
        return false;
    }

    m_isOpen = true;
    m_dbPath = path;

    AddLogLineN(CFormat(_("MaxMind DB: Opened database from %s")) % path);
    return true;
#else
    // If MaxMindDB support is disabled, always return false
    AddLogLineC(wxT("MaxMind DB support not compiled in"));
    return false;
#endif
}

void MaxMindDBDatabase::Close()
{
#ifdef ENABLE_MAXMINDDB
    if (m_isOpen) {
        MMDB_close(&m_mmdb);
        memset(&m_mmdb, 0, sizeof(m_mmdb));  // Reset struct to zero
        m_isOpen = false;
        m_dbPath.Clear();
    }
#endif
}

bool MaxMindDBDatabase::IsOpen() const
{
    return m_isOpen;
}

wxString MaxMindDBDatabase::GetCountryCode(const wxString& ip)
{
    wxString country_code;
    wxString country_name;

    if (LookupCountry(ip, country_code, country_name)) {
        return country_code;
    }

    return wxEmptyString;
}

wxString MaxMindDBDatabase::GetCountryName(const wxString& ip)
{
    wxString country_code;
    wxString country_name;

    if (LookupCountry(ip, country_code, country_name)) {
        return country_name;
    }

    return wxEmptyString;
}

bool MaxMindDBDatabase::LookupCountry(const wxString& ip, wxString& country_code, wxString& country_name) const
{
#ifdef ENABLE_MAXMINDDB
    if (!m_isOpen || ip.IsEmpty()) {
        return false;
    }

    // Convert IP to required format
    std::string ip8bit = std::string(ip.mb_str(wxConvUTF8));

    // Parse the IP address
    int gai_error = 0;
    int mmdb_error = 0;
    MMDB_lookup_result_s result = MMDB_lookup_string(&m_mmdb, ip8bit.c_str(), &gai_error, &mmdb_error);

    if (gai_error != 0) {
        // Invalid IP address format
        return false;
    }

    if (mmdb_error != MMDB_SUCCESS) {
        return false;
    }

    if (!result.found_entry) {
        return false;
    }

    // Get the country entry
    MMDB_entry_data_s entry_data;
    int status;

    // Get country ISO code
    status = MMDB_get_value(&result.entry, &entry_data, "country", "iso_code", NULL);
    if (status == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
        country_code = wxString(entry_data.utf8_string, wxConvUTF8, entry_data.data_size);
        country_code.MakeLower();
    } else {
        // If no country, try continent as fallback
        status = MMDB_get_value(&result.entry, &entry_data, "continent", "code", NULL);
        if (status == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
            country_code = wxString(entry_data.utf8_string, wxConvUTF8, entry_data.data_size);
            country_code.MakeLower();
        }
    }

    // Get country name
    status = MMDB_get_value(&result.entry, &entry_data, "country", "names", "en", NULL);
    if (status == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING) {
        country_name = wxString(entry_data.utf8_string, wxConvUTF8, entry_data.data_size);
    }

    return !country_code.IsEmpty();
#else
    // Fallback implementation if MaxMind DB is not available
    return false;
#endif
}

bool MaxMindDBDatabase::IsValid() const
{
#ifdef ENABLE_MAXMINDDB
    return m_isOpen && (m_mmdb.filename != NULL);
#else
    return false;
#endif
}

// GetType() and GetFormatName() are implemented inline in the header
// to avoid duplicate symbol errors

wxString MaxMindDBDatabase::GetVersion() const
{
#ifdef ENABLE_MAXMINDDB
    if (!m_isOpen) {
        return wxEmptyString;
    }

    wxString versionStr;
    versionStr.Printf(wxT("MaxMind DB v%d.%d"),
                      m_mmdb.metadata.binary_format_major_version,
                      m_mmdb.metadata.binary_format_minor_version);
    return versionStr;
#else
    return wxT("MaxMind DB (not compiled)");
#endif
}

wxString MaxMindDBDatabase::GetDescription() const
{
#ifdef ENABLE_MAXMINDDB
    if (!m_isOpen) {
        return wxT("Not loaded");
    }

    wxString desc;
    desc.Printf(wxT("Type: %s, Node count: %u, Record size: %d bits"),
                wxString::FromUTF8(m_mmdb.metadata.database_type),
                m_mmdb.metadata.node_count,
                m_mmdb.metadata.record_size);
    return desc;
#else
    return wxT("MaxMind DB (not compiled)");
#endif
}