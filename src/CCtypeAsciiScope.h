//
// This file is part of the aMule Project.
//
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

#ifndef CCTYPEASCIISCOPE_H
#define CCTYPEASCIISCOPE_H

#include <clocale>
#include <cstdlib>
#include <cstring>


// RAII helper that pins LC_CTYPE to "C" for the duration of its scope
// and restores the previous value on destruction.
//
// Used to neutralise locale-dependent case folding around code paths
// that assume ASCII semantics. The motivating case is the Turkish
// I/i divergence in tr_TR, where libc and wxString routines fold
// ASCII 'I' to U+0131 (dotless i) instead of 'i'. That breaks:
//
//   - wxFileConfig's case-insensitive entry / group lookups (which
//     bottom out at strcasecmp / wcscasecmp on POSIX), causing the
//     sorted m_aEntries array to desync from its comparator across
//     a locale switch and Write() to append duplicate keys (#852).
//   - Lowercasing of ISO 3166-1 alpha-2 country codes used to build
//     embedded flag bitmap names (e.g. "IT" -> "ıt"), causing the
//     country flag to not render in the GUI.
//
// Only LC_CTYPE is touched, so LC_MESSAGES (translations), LC_TIME
// (date formatting), LC_NUMERIC (numbers) and LC_COLLATE (sort orders
// such as filenames in list controls) remain locale-aware.
class CCtypeAsciiScope
{
public:
	CCtypeAsciiScope() : m_saved(NULL)
	{
		const char* current = std::setlocale(LC_CTYPE, NULL);
		if (current) {
			m_saved = strdup(current);
		}
		std::setlocale(LC_CTYPE, "C");
	}

	~CCtypeAsciiScope()
	{
		if (m_saved) {
			std::setlocale(LC_CTYPE, m_saved);
			std::free(m_saved);
		}
	}

	CCtypeAsciiScope(const CCtypeAsciiScope&) = delete;
	CCtypeAsciiScope& operator=(const CCtypeAsciiScope&) = delete;

private:
	char* m_saved;
};

#endif // CCTYPEASCIISCOPE_H
