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

#include "Preferences.h"	// For thePrefs
#include "CFile.h"			// For CPath
#include "HTTPDownload.h"
#include "Logger.h"			// For AddLogLineM()
#include <common/Format.h>		// For CFormat()
#include "common/FileFunctions.h"	// For UnpackArchive
#include <common/StringFunctions.h>	// For unicode2char()
#include "pixmaps/flags_xpm/CountryFlags.h"

#include <wx/intl.h>
#include <wx/image.h>

#include "IP2Country.h"
#include "geoip/MaxMindDBDatabase.h"

CIP2Country::CIP2Country(const wxString& configDir)
	: m_db(new CMaxMindDBDatabase())
{
	m_DataBaseName = "GeoLite2-Country.mmdb";
	m_DataBasePath = configDir + m_DataBaseName;
}

bool CIP2Country::IsEnabled()
{
	return m_db && m_db->IsOpen();
}

void CIP2Country::Enable()
{
	Disable();

	if (m_CountryDataMap.empty()) {
		LoadFlags();
	}

	if (!CPath::FileExists(m_DataBasePath)) {
		Update();
		return;
	}

	m_db->Open(m_DataBasePath);
}

void CIP2Country::Update()
{
	const wxString& url = thePrefs::GetGeoIPUpdateUrl();
	if (url.IsEmpty()) {
		AddLogLineC(CFormat(
			_("No GeoLite2 update URL configured. Download GeoLite2-Country.mmdb manually (a free MaxMind account is required) and place it at %s, or set the URL in Preferences."))
			% m_DataBasePath);
		thePrefs::SetGeoIPEnabled(false);
		return;
	}
	AddLogLineN(CFormat(_("Download new %s from %s")) % m_DataBaseName % url);
	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(url, m_DataBasePath + ".download", m_DataBasePath, HTTP_GeoIP, true, true);
	downloader->Create();
	downloader->Run();
}

void CIP2Country::Disable()
{
	if (m_db) {
		m_db->Close();
	}
}

void CIP2Country::DownloadFinished(uint32 result)
{
	if (result == HTTP_Success) {
		Disable();
		// download succeeded. Switch over to new database.
		wxString newDat = m_DataBasePath + ".download";

		// Try to unpack the file, might be an archive
		wxScopedCharBuffer dataBaseName = m_DataBaseName.utf8_str();
		const char* geoip_files[] = {
			dataBaseName,
			NULL
		};

		if (UnpackArchive(CPath(newDat), geoip_files).second == EFT_Error) {
			AddLogLineC(CFormat(_("Download of %s file failed, aborting update.")) % m_DataBaseName);
			return;
		}

		if (wxFileExists(m_DataBasePath)) {
			if (!wxRemoveFile(m_DataBasePath)) {
				AddLogLineC(CFormat(_("Failed to remove %s file, aborting update.")) % m_DataBaseName);
				return;
			}
		}

		if (!wxRenameFile(newDat, m_DataBasePath)) {
			AddLogLineC(CFormat(_("Failed to rename %s file, aborting update.")) % m_DataBaseName);
			return;
		}

		Enable();
		if (IsEnabled()) {
			AddLogLineN(CFormat(_("Successfully updated %s")) % m_DataBaseName);
		} else {
			AddLogLineC(CFormat(_("Error updating %s")) % m_DataBaseName);
		}
	} else if (result == HTTP_Skipped) {
		AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % m_DataBaseName);
	} else {
		AddLogLineC(CFormat(_("Failed to download %s from %s")) % m_DataBaseName % thePrefs::GetGeoIPUpdateUrl());
		// if it failed and there is no database, turn it off
		if (!wxFileExists(m_DataBasePath)) {
			thePrefs::SetGeoIPEnabled(false);
		}
	}
}

void CIP2Country::LoadFlags()
{
	// Load data from xpm files
	for (int i = 0; i < flags::FLAGS_XPM_SIZE; ++i) {
		CountryData countrydata;
		countrydata.Name = wxString(flags::flagXPMCodeVector[i].code, wxConvISO8859_1);
		countrydata.Flag = wxImage(flags::flagXPMCodeVector[i].xpm);

		if (countrydata.Flag.IsOk()) {
			m_CountryDataMap[countrydata.Name] = countrydata;
		} else {
			AddLogLineC(CFormat(_("Failed to load country data for '%s'.")) % countrydata.Name);
			continue;
		}
	}

	AddDebugLogLineN(logGeneral, CFormat("Loaded %d flag bitmaps.") % m_CountryDataMap.size());  // there's never just one - no plural needed
}


CIP2Country::~CIP2Country()
{
	Disable();
	delete m_db;
}


const CountryData& CIP2Country::GetCountryData(const wxString &ip)
{
	// Should prevent the crash if the database does not exist
	if (!IsEnabled()) {
		CountryDataMap::iterator it = m_CountryDataMap.find(wxString("unknown"));
		it->second.Name = "?";
		return it->second;
	}

	wxString CCode = m_db->GetCountryCode(ip);
	if (CCode.IsEmpty()) {
		CCode = "unknown";
	}

	CountryDataMap::iterator it = m_CountryDataMap.find(CCode);
	if (it == m_CountryDataMap.end()) {
		// Show the code and ?? flag
		it = m_CountryDataMap.find(wxString("unknown"));
		wxASSERT(it != m_CountryDataMap.end());
		if (CCode.IsEmpty()) {
			it->second.Name = "?";
		} else {
			it->second.Name = CCode;
		}
	}

	return it->second;
}

#else

#include "IP2Country.h"

CIP2Country::CIP2Country(const wxString&)
{
	m_db = NULL;
}

CIP2Country::~CIP2Country() {}
void CIP2Country::Enable() {}
void CIP2Country::DownloadFinished(uint32) {}
bool CIP2Country::IsEnabled() { return false; }

const CountryData& CIP2Country::GetCountryData(const wxString &)
{
	static CountryData dummy;
	return dummy;
}

#endif // ENABLE_IP2COUNTRY
