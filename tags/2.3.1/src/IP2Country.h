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

#ifndef IP2COUNTRY_H
#define IP2COUNTRY_H

#include "Types.h"	// Needed for uint8, uint16 and uint32

#include <map>

#include <wx/image.h>
#include <wx/string.h>


typedef struct {
	wxString Name;
	wxImage  Flag;
} CountryData;


typedef std::map<wxString, CountryData> CountryDataMap;


class CIP2Country {
public:
	CIP2Country(const wxString& configDir);
	~CIP2Country();
	const CountryData& GetCountryData(const wxString& ip);
	void Enable();
	void Disable();
	void Update();
	bool IsEnabled() { return m_geoip != NULL; }
	void DownloadFinished(uint32 result);

private:
	struct GeoIPTag *m_geoip;
	CountryDataMap m_CountryDataMap;
	wxString m_DataBaseName;
	wxString m_DataBasePath;

	void LoadFlags();
};

#endif // IP2COUNTRY_H
