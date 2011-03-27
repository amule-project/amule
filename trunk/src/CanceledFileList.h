//
// This file is part of the aMule Project.
//
// Copyright (c) 2010-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CANCELEDFILELIST_H
#define CANCELEDFILELIST_H

#include <set>
#include "MD4Hash.h"

//
// A list to keep track of canceled files, stored in canceled.met .
// Only the file hash is stored.
//
class CCanceledFileList
{
public:
	// Ctor, calls Init()
	CCanceledFileList();
	// Save list
	void	Save();
	// Check if hash belongs to a canceled file
	bool	IsCanceledFile(const CMD4Hash& hash) const;
	// Add a hash to the list (returns true if added, false if already in list)
	bool	Add(const CMD4Hash& hash);
	// Remove a hash from the list (returns true if removed, false if not in list)
	bool	Remove(const CMD4Hash& hash);

private:
	// The list
	typedef std::set<CMD4Hash> CanceledFileList;
	CanceledFileList m_canceledFileList;
	// The filename "canceled.met"
	wxString	m_filename;

	// Load list from file (if it exists)
	bool	Init();
};

#endif // CANCELEDFILELIST_H
// File_checked_for_headers
