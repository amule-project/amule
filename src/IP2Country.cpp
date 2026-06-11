//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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
#include "icons/icon_data.h"		// For amule_get_all_icons()

#include <wx/artprov.h>			// For wxArtProvider::GetBitmap
#include <wx/intl.h>
#include <wx/image.h>

#include <cstring>			// For strncmp

#include "IP2Country.h"
#include "geoip/MaxMindDBDatabase.h"
#include "PrefsUnifiedDlg.h"		// For NotifyIP2CountryUpdateFailedIfOpen

CIP2Country::CIP2Country(const wxString& configDir)
	: m_db(new CMaxMindDBDatabase()),
	  m_TriedPreviousMonth(false),
	  m_ManualUpdate(false)
{
	m_DataBaseName = "geoip.mmdb";
	m_DataBasePath = configDir + m_DataBaseName;

	// One-shot migration: the v2.x file lived at GeoLite2-Country.mmdb.
	// If that legacy file exists and the new canonical geoip.mmdb does
	// not, move it across so an upgrading user doesn't lose flag display
	// silently. If both exist (e.g. they followed the new docs while
	// keeping the old file around) leave each alone.
	const wxString legacyPath = configDir + "GeoLite2-Country.mmdb";
	if (CPath::FileExists(legacyPath) && !CPath::FileExists(m_DataBasePath)) {
		if (wxRenameFile(legacyPath, m_DataBasePath)) {
			AddLogLineN(CFormat(_("Migrated existing GeoLite2-Country.mmdb to %s"))
				% m_DataBasePath);
		}
	}
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

	// One-shot backfill: files written by builds older than the
	// source-aware prefs have no LoadedSource recorded, which would
	// leave the prefs status line attribution-less. Best-effort guess:
	// attribute the existing file to the currently configured source
	// so the status line shows *something* meaningful. The user can
	// always click "Update now" to overwrite with the real source.
	if (m_db->IsOpen() && thePrefs::GetGeoIPLoadedSource().IsEmpty()) {
		thePrefs::SetGeoIPLoadedSource(thePrefs::GetGeoIPSource());
	}
}

void CIP2Country::Update(bool manualUpdate)
{
	m_TriedPreviousMonth = false;
	m_ManualUpdate = manualUpdate;
	StartDownload(0);
}

void CIP2Country::StartDownload(int monthOffset)
{
	const wxString url = thePrefs::GetGeoIPResolvedDownloadUrl(monthOffset);
	if (url.IsEmpty()) {
		wxString msg;
		switch (thePrefs::GetGeoIPSource()) {
		case CPreferences::GeoIPSourceMaxMind:
			msg = _(
				"IP2Country: MaxMind selected as the GeoIP source but no License Key "
				"configured. Open Preferences → IP2Country, paste your free MaxMind "
				"License Key and click 'Update now'.");
			break;
		case CPreferences::GeoIPSourceCustom:
			msg = _(
				"IP2Country: Custom URL selected as the GeoIP source but no URL "
				"configured. Open Preferences → IP2Country and supply a URL that "
				"points to an .mmdb (or .gz / .tar.gz containing one).");
			break;
		default:
			msg = _(
				"IP2Country: failed to resolve a GeoIP download URL.");
			break;
		}
		AddLogLineC(msg);
		if (m_ManualUpdate) {
			PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
		}
		m_ManualUpdate = false;
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
	// Snapshot + clear the manual flag up front so any early return
	// below doesn't leave it set for the next StartDownload (e.g. a
	// subsequent auto-update would inherit the popup behaviour).
	const bool manual = m_ManualUpdate;
	m_ManualUpdate = false;

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
			const wxString msg = CFormat(_("Download of %s file failed, aborting update.")) % m_DataBaseName;
			AddLogLineC(msg);
			if (manual) {
				PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
			}
			return;
		}

		if (wxFileExists(m_DataBasePath)) {
			if (!wxRemoveFile(m_DataBasePath)) {
				const wxString msg = CFormat(_("Failed to remove %s file, aborting update.")) % m_DataBaseName;
				AddLogLineC(msg);
				if (manual) {
					PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
				}
				return;
			}
		}

		if (!wxRenameFile(newDat, m_DataBasePath)) {
			const wxString msg = CFormat(_("Failed to rename %s file, aborting update.")) % m_DataBaseName;
			AddLogLineC(msg);
			if (manual) {
				PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
			}
			return;
		}

		Enable();
		if (IsEnabled()) {
			AddLogLineN(CFormat(_("Successfully updated %s")) % m_DataBaseName);
			// Record which source actually wrote the file so the prefs
			// status line can attribute it correctly even after the
			// user flips the source dropdown to a different provider
			// they haven't downloaded from yet.
			thePrefs::SetGeoIPLoadedSource(thePrefs::GetGeoIPSource());
		} else {
			const wxString msg = CFormat(_("Error updating %s")) % m_DataBaseName;
			AddLogLineC(msg);
			if (manual) {
				PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
			}
		}
	} else if (result == HTTP_Skipped) {
		AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % m_DataBaseName);
	} else {
		// DB-IP early-month fallback: the new month's file frequently
		// 404s for the first few days while DB-IP publishes it. Retry
		// once with monthOffset=-1 so the previous (definitely-published)
		// month carries the user through the gap. MaxMind / Custom URLs
		// aren't month-templated, so the fallback is gated on source.
		// Re-arm the manual flag so the retry's eventual outcome still
		// surfaces a popup; we only cleared it as a one-shot guard.
		if (thePrefs::GetGeoIPSource() == CPreferences::GeoIPSourceDBIP
			&& !m_TriedPreviousMonth) {
			m_TriedPreviousMonth = true;
			m_ManualUpdate = manual;
			AddLogLineN(_(
				"DB-IP download failed for the current month - retrying with "
				"the previous month's URL."));
			StartDownload(-1);
			return;
		}
		const wxString msg = CFormat(_("Failed to download %s from %s")) % m_DataBaseName
			% thePrefs::GetGeoIPResolvedDownloadUrl(m_TriedPreviousMonth ? -1 : 0);
		AddLogLineC(msg);
		if (manual) {
			PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(msg);
		}
		// if it failed and there is no database, turn it off
		if (!wxFileExists(m_DataBasePath)) {
			thePrefs::SetGeoIPEnabled(false);
		}
	}
}

void CIP2Country::LoadFlags()
{
	// Walk the embedded icon table and pick out anything named
	// "flag_<code>". The table is built by src/icons/embed_icons.py
	// from src/icons/flags/<code>.png at compile time; CamuleArtProvider
	// (registered in CamuleGuiApp::OnInit) hands us back a decoded
	// wxBitmap for each.
	int icon_count = 0;
	const struct AMuleIconEntry *icons = amule_get_all_icons(&icon_count);
	const char flag_prefix[] = "flag_";
	const size_t flag_prefix_len = sizeof(flag_prefix) - 1;

	for (int i = 0; i < icon_count; ++i) {
		const char *name = icons[i].name;
		if (strncmp(name, flag_prefix, flag_prefix_len) != 0) {
			continue;
		}
		const char *code = name + flag_prefix_len;

		CountryData countrydata;
		countrydata.Name = wxString(code, wxConvISO8859_1);

		const wxString art_id = wxString::Format("amule:%s", name);
		countrydata.Flag = wxArtProvider::GetBitmap(art_id).ConvertToImage();

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
