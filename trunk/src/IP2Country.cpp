//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


#define IP2COUNTRY_C


#include "IP2Country.h"


#include <common/Format.h>		// For CFormat()
#include <common/StringFunctions.h>	// For unicode2char()
#include "Logger.h"			// For AddLogLineM()


#include <wx/image.h>


#include "pixmaps/flags_xpm/CountryFlags.h"


GeoIP *CIP2Country::s_geoip(NULL);
std::auto_ptr<wxBitmap> CIP2Country::s_flagUnknown(NULL);


CIP2Country::CIP2Country()
:
m_mutexGeoip(),
m_flagMap()
{
	wxMutexLocker l(m_mutexGeoip);
	if (!s_geoip) {
		s_geoip = GeoIP_new(GEOIP_STANDARD);
		// Creata a black bitmap
		wxImage imgFlagUnknown(16, 11, true);
		s_flagUnknown.reset(new wxBitmap(imgFlagUnknown));
		
		// Load data from xpm files
		for (int i = 0; i < FLAGS_XPM_SIZE; ++i) {
			std::string CCode = flagXPMCodeVector[i].code;
			wxImage img(flagXPMCodeVector[i].xpm);
			if (img.IsOk()) {
				wxBitmap bitmap(img);
				m_flagMap[CCode] = bitmap;
			} else {
				AddLogLineM(true, CFormat(_(
					"CIP2Country::CIP2Country() "
					"Failed to load country data from '%s'")) %
					char2unicode(CCode.c_str()));
			}
		}
		AddLogLineM(false, CFormat(_("Loaded %d flag bitmaps.")) %
			m_flagMap.size());
	}
}


CIP2Country::~CIP2Country()
{
}


wxString CIP2Country::CountryCode(const wxString &ip) const
{
	wxMutexLocker l(m_mutexGeoip);
	wxString ret(char2unicode(
		GeoIP_country_code_by_addr(
			s_geoip, unicode2char(ip))));

	return ret.MakeLower();
}


wxString CIP2Country::CountryName(const wxString &ip) const
{
	wxMutexLocker l(m_mutexGeoip);
	wxString ret(char2unicode(
		GeoIP_country_name_by_addr(
			s_geoip, unicode2char(ip))));

	return ret;
}


const wxBitmap &CIP2Country::CountryFlag(const wxString &ip) const
{
	std::string CCode(unicode2char(CountryCode(ip)));
	std::map<std::string, wxBitmap>::const_iterator it = m_flagMap.find(CCode);
	if (it != m_flagMap.end()) {
		return it->second;
	} else {
		return *s_flagUnknown;
	}
}

