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

//
// Country flags are from FAMFAMFAM (http://www.famfamfam.com)
//
// Flag icons - http://www.famfamfam.com
//
// These icons are public domain, and as such are free for any use (attribution appreciated but not required).
//
// Note that these flags are named using the ISO3166-1 alpha-2 country codes where appropriate.
// A list of codes can be found at http://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
//
// If you find these icons useful, please donate via paypal to mjames@gmail.com
// (or click the donate button available at http://www.famfamfam.com/lab/icons/silk)
//
// Contact: mjames@gmail.com
//

#include "config.h"		// Needed for ENABLE_IP2COUNTRY

#ifdef ENABLE_IP2COUNTRY

#include "IP2Country.h"
#include "geoip/IP2CountryManager.h"
#include "Preferences.h"	// For thePrefs
#include "CFile.h"			// For CPath
#include "Logger.h"			// For AddLogLineM()
#include "OtherFunctions.h"	// For IsLibraryAvailable()
#include <common/Format.h>		// For CFormat()
#ifdef ENABLE_IP2COUNTRY
#include "geoip/IP2CountryManager.h"  // Include new manager header
#include "pixmaps/flags_xpm/CountryFlags.h"
#endif

#include <wx/intl.h>
#include <wx/image.h>

CIP2Country::CIP2Country(const wxString& configDir)
    : m_manager(nullptr)
    , m_CountryDataMap()
    , m_DataBaseName(wxT("GeoLite2-Country.mmdb"))
    , m_DataBasePath(configDir + m_DataBaseName)
    , m_enabled(false)
{
#ifndef _WIN32
    // Check if libmaxminddb is available at runtime
    if (!IsLibraryAvailable("maxminddb")) {
        AddLogLineC(_("ERROR: libmaxminddb not found - GeoIP/country flags will be disabled"));
        AddLogLineC(_("Please install: sudo apt install libmaxminddb-dev"));
        m_enabled = false;
        return;
    }
#endif

    // Create new IP2CountryManager instance
    m_manager = &IP2CountryManager::GetInstance();

    // Initialize the manager
    if (!m_manager->Initialize(configDir)) {
        AddLogLineN(_("IP2Country: Failed to initialize manager"));
    }
}

CIP2Country::~CIP2Country()
{
    if (m_manager) {
        m_manager->Disable();
        IP2CountryManager::DestroyInstance();
        m_manager = nullptr;
    }
}

void CIP2Country::Enable()
{
    if (!m_manager) {
        return;
    }

    m_manager->Enable();
    m_enabled = m_manager->IsEnabled();

    if (m_enabled) {
        // Load flags for legacy compatibility
        LoadFlags();
    }
}

void CIP2Country::Disable()
{
    if (!m_manager) {
        return;
    }

    m_manager->Disable();
    m_enabled = false;
}

void CIP2Country::Update()
{
    if (!m_manager) {
        return;
    }

    AddLogLineN(_("IP2Country: Starting database update..."));
    m_manager->DownloadUpdate();
}

void CIP2Country::DownloadFinished(uint32 result)
{
    // This is handled internally by IP2CountryManager now
    // Kept for API compatibility
    AddLogLineN(CFormat(_("IP2Country: Download finished with result %u")) % result);
}
const CountryDataOld& CIP2Country::GetCountryData(const wxString& ip)
{
    if (!m_enabled || !m_manager || !m_manager->IsEnabled()) {
        static CountryDataOld empty;
        return empty;
    }

    // Get data from new manager
    CountryDataNew newData = m_manager->GetCountryData(ip);

    // Use static variable to avoid returning reference to local
    static CountryDataOld result;
    result.Name = newData.Name;
    result.Flag = newData.Flag;

    // Check if we have a flag in our map
    if (result.Flag.IsOk()) {
        // Already have a valid flag
    } else if (!newData.Code.IsEmpty()) {
        // Try to get flag from legacy map
        auto it = m_CountryDataMap.find(newData.Code);
        if (it != m_CountryDataMap.end()) {
            result = it->second;
        } else {
            // Try unknown flag
            auto unknownIt = m_CountryDataMap.find(wxT("unknown"));
            if (unknownIt != m_CountryDataMap.end()) {
                result = unknownIt->second;
            }
        }
    }

    return result;
}

void CIP2Country::LoadFlags()
{
    // Load data from xpm files
    for (int i = 0; i < flags::FLAGS_XPM_SIZE; ++i) {
        CountryDataOld countrydata;
        countrydata.Name = wxString(flags::flagXPMCodeVector[i].code, wxConvISO8859_1);
        countrydata.Flag = wxImage(flags::flagXPMCodeVector[i].xpm);

        if (countrydata.Flag.IsOk()) {
            m_CountryDataMap[countrydata.Name] = countrydata;
        } else {
            AddLogLineC(CFormat(_("Failed to load country data for '%s'.")) % countrydata.Name);
        }
    }

    AddDebugLogLineN(logGeneral, CFormat(wxT("Loaded %d flag bitmaps.")) % m_CountryDataMap.size());
}

#else

#include "IP2Country.h"
CIP2Country::CIP2Country(const wxString&)
    : m_manager(nullptr)
    , m_CountryDataMap()
    , m_DataBaseName()
    , m_DataBasePath()
    , m_enabled(false)
{
    // No initialization needed when IP2Country is disabled
}

CIP2Country::~CIP2Country() {}

void CIP2Country::Enable() {}
void CIP2Country::Disable() {}
void CIP2Country::Update() {}
void CIP2Country::DownloadFinished(uint32) {}

const CountryDataOld& CIP2Country::GetCountryData(const wxString&)
{
    static CountryDataOld dummy;
    return dummy;
}

#endif // ENABLE_IP2COUNTRY