//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "SearchPackageException.h"
#include "SearchLogging.h"
#include "../Logger.h"
#include <common/Format.h>

namespace search {

SearchPackageException::SearchPackageException(const wxString& message, uint32_t searchId)
	: m_message(message)
	, m_searchId(searchId)
	, m_timestamp(wxDateTime::Now())
{
	// Log immediately on creation
	LogException();
}

SearchPackageException::~SearchPackageException() throw()
{
}

void SearchPackageException::LogException() const
{
	wxString logMsg = CFormat(wxT("SearchPackageException: SearchID=%u, Message='%s', Timestamp='%s'"))
		% m_searchId % m_message % m_timestamp.FormatISOCombined();

	SEARCH_DEBUG(logMsg);

	// Also write to error log
	AddLogLineC(logMsg);
}

} // namespace search
