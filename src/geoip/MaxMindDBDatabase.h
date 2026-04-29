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

#ifndef MAXMINDDBDATABASE_H
#define MAXMINDDBDATABASE_H

#include <maxminddb.h>
#include <wx/string.h>


// Thin RAII wrapper around libmaxminddb for IP-to-country lookups against a
// MaxMind .mmdb database (GeoLite2-Country or equivalent). Replaces the legacy
// libGeoIP v1 / .dat backend that MaxMind discontinued in 2019.
class CMaxMindDBDatabase
{
public:
	CMaxMindDBDatabase();
	~CMaxMindDBDatabase();

	// Open the .mmdb at `path` in MMAP mode. Returns false (and logs) on failure.
	// Calling Open() on an already-open instance closes the previous handle first.
	bool	Open(const wxString& path);

	void	Close();

	bool	IsOpen() const { return m_isOpen; }

	// Look up `ip` (IPv4 or IPv6 textual) and return the lowercased ISO 3166-1
	// alpha-2 country code (e.g. "us"). Empty string on miss / invalid input.
	wxString	GetCountryCode(const wxString& ip) const;

private:
	MMDB_s	m_mmdb;
	bool	m_isOpen;

	CMaxMindDBDatabase(const CMaxMindDBDatabase&) = delete;
	CMaxMindDBDatabase& operator=(const CMaxMindDBDatabase&) = delete;
};

#endif // MAXMINDDBDATABASE_H
