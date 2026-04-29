//
// This file is part of the aMule Project.
//
// Copyright (c) 2026 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "MaxMindDBDatabase.h"

#include "Logger.h"
#include <common/Format.h>
#include <common/StringFunctions.h>		// unicode2char
#include <wx/intl.h>				// _() gettext macro


CMaxMindDBDatabase::CMaxMindDBDatabase()
	: m_isOpen(false)
{
}


CMaxMindDBDatabase::~CMaxMindDBDatabase()
{
	Close();
}


bool CMaxMindDBDatabase::Open(const wxString& path)
{
	Close();

	int status = MMDB_open(unicode2char(path), MMDB_MODE_MMAP, &m_mmdb);
	if (status != MMDB_SUCCESS) {
		AddLogLineC(CFormat(_("Failed to open MaxMindDB database '%s': %s"))
			% path % wxString::FromUTF8(MMDB_strerror(status)));
		return false;
	}

	m_isOpen = true;
	return true;
}


void CMaxMindDBDatabase::Close()
{
	if (m_isOpen) {
		MMDB_close(&m_mmdb);
		m_isOpen = false;
	}
}


wxString CMaxMindDBDatabase::GetCountryCode(const wxString& ip) const
{
	if (!m_isOpen) {
		return wxEmptyString;
	}

	int gai_error = 0;
	int mmdb_error = 0;
	MMDB_lookup_result_s result = MMDB_lookup_string(
		const_cast<MMDB_s*>(&m_mmdb), unicode2char(ip),
		&gai_error, &mmdb_error);

	if (gai_error || mmdb_error != MMDB_SUCCESS || !result.found_entry) {
		return wxEmptyString;
	}

	// GeoLite2-Country and equivalents nest the ISO code under
	// country -> iso_code (UTF-8 string).
	MMDB_entry_data_s entry_data;
	int status = MMDB_get_value(&result.entry, &entry_data,
		"country", "iso_code", NULL);
	if (status != MMDB_SUCCESS || !entry_data.has_data
		|| entry_data.type != MMDB_DATA_TYPE_UTF8_STRING
		|| entry_data.data_size == 0) {
		return wxEmptyString;
	}

	// utf8_string is NOT NUL-terminated; data_size is the byte length.
	wxString code = wxString::FromUTF8(entry_data.utf8_string, entry_data.data_size);
	return code.Lower();
}
