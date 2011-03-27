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

#ifdef HAVE_CONFIG_H
#	include "config.h"		// Needed for ENABLE_IP2COUNTRY
#endif

#ifdef _MSC_VER
// For MSVC we need to check here if geoip is available at all. MinGW needs the include below however.

// Block unnecessary includes from GeoIP.h
#define _WINDOWS_
#define _WINSOCK2API_
#define _WS2TCPIP_H_
#include <GeoIP.h>
#endif

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

#include <GeoIP.h>
#include "IP2Country.h"

CIP2Country::CIP2Country(const wxString& configDir)
{
	m_geoip = NULL;
	m_DataBaseName = wxT("GeoIP.dat");
	m_DataBasePath = configDir + m_DataBaseName;
	if (m_CountryDataMap.empty()) {
// this must go to enable - when all usages of GetCountryData() (ClientListCtrl, DownloadListCtrl) are surrounded with an IsEnabled()
// right now, flags are loaded even when it's disabled
		LoadFlags();
	}
}

void CIP2Country::Enable()
{
	Disable();
	if (!CPath::FileExists(m_DataBasePath)) {
		Update();
		return;
	}

	m_geoip = GeoIP_open(unicode2char(m_DataBasePath), GEOIP_STANDARD);
}

void CIP2Country::Update()
{
	AddLogLineN(CFormat(_("Download new GeoIP.dat from %s")) % thePrefs::GetGeoIPUpdateUrl());
	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(thePrefs::GetGeoIPUpdateUrl(), m_DataBasePath + wxT(".download"), m_DataBasePath, HTTP_GeoIP, true, true);
	downloader->Create();
	downloader->Run();
}

void CIP2Country::Disable()
{
	if (m_geoip) {
		GeoIP_delete(m_geoip);
		m_geoip = NULL;
	}
}

void CIP2Country::DownloadFinished(uint32 result)
{
	if (result == HTTP_Success) {
		Disable();
		// download succeeded. Switch over to new database.
		wxString newDat = m_DataBasePath + wxT(".download");

		// Try to unpack the file, might be an archive
		const wxChar* geoip_files[] = {
			m_DataBaseName,
			NULL
		};

		if (UnpackArchive(CPath(newDat), geoip_files).second == EFT_Error) {
			AddLogLineC(_("Download of GeoIP.dat file failed, aborting update."));
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
		if (m_geoip) {
			AddLogLineN(CFormat(_("Successfully updated %s")) % m_DataBaseName);
		} else {
			AddLogLineC(_("Error updating GeoIP.dat"));
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
		countrydata.Name = char2unicode(flags::flagXPMCodeVector[i].code);
		countrydata.Flag = wxImage(flags::flagXPMCodeVector[i].xpm);
		
		if (countrydata.Flag.IsOk()) {
			m_CountryDataMap[countrydata.Name] = countrydata;
		} else {
			AddLogLineC(CFormat(_("Failed to load country data for '%s'.")) % countrydata.Name);
			continue;
		}
	}

	AddDebugLogLineN(logGeneral, CFormat(wxT("Loaded %d flag bitmaps.")) % m_CountryDataMap.size());  // there's never just one - no plural needed
}


CIP2Country::~CIP2Country()
{
	Disable();
}


const CountryData& CIP2Country::GetCountryData(const wxString &ip)
{
	// Should prevent the crash if the GeoIP database does not exists
	if (m_geoip == NULL) {
		CountryDataMap::iterator it = m_CountryDataMap.find(wxString(wxT("unknown")));
		it->second.Name = wxT("?");
		return it->second;	
	}
	
	const wxString CCode = wxString(char2unicode(GeoIP_country_code_by_addr(m_geoip, unicode2char(ip)))).MakeLower();
	
	CountryDataMap::iterator it = m_CountryDataMap.find(CCode);
	if (it == m_CountryDataMap.end()) { 
		// Show the code and ?? flag
		it = m_CountryDataMap.find(wxString(wxT("unknown")));
		wxASSERT(it != m_CountryDataMap.end());
		if (CCode.IsEmpty()) {
			it->second.Name = wxT("?");
		} else{
			it->second.Name = CCode;			
		}
	}
	
	return it->second;	
}

#else

#include "IP2Country.h"

CIP2Country::CIP2Country(const wxString&)
{
	m_geoip = NULL;
}

CIP2Country::~CIP2Country() {}
void CIP2Country::Enable() {}
void CIP2Country::DownloadFinished(uint32) {}

const CountryData& CIP2Country::GetCountryData(const wxString &)
{
	static CountryData dummy;
	return dummy;
}

#endif // ENABLE_IP2COUNTRY
