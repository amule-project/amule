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

#ifndef SEARCHPACKAGEEXCEPTION_H
#define SEARCHPACKAGEEXCEPTION_H

#include <wx/datetime.h>
#include <wx/string.h>
#include <exception>
#include <stdint.h>

namespace search {

/**
 * Exception class for defective search result packages
 *
 * This exception is thrown when a search result package fails validation.
 * It automatically logs detailed information about the failure.
 */
class SearchPackageException : public std::exception {
public:
	/**
	 * Constructor
	 *
	 * @param message Error message describing what failed
	 * @param searchId The search ID this package belongs to
	 */
	SearchPackageException(const wxString& message, uint32_t searchId);

	/**
	 * Destructor
	 */
	virtual ~SearchPackageException() throw();

	/**
	 * Get the error message
	 */
	const wxString& GetMessage() const { return m_message; }

	/**
	 * Get the search ID
	 */
	uint32_t GetSearchId() const { return m_searchId; }

	/**
	 * Get the timestamp when exception was created
	 */
	const wxDateTime& GetTimestamp() const { return m_timestamp; }

	/**
	 * Log the exception details
	 */
	void LogException() const;

private:
	wxString m_message;          // Error message
	uint32_t m_searchId;          // Search ID
	wxDateTime m_timestamp;       // When exception was created
};

} // namespace search

#endif // SEARCHPACKAGEEXCEPTION_H
