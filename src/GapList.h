//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef GAPLIST_H
#define GAPLIST_H

#include "OtherStructs.h"	// for Gap_Struct

class CGapList {
private:
	// the internal gap list
	typedef std::list<Gap_Struct*> ListType;
	ListType m_gaplist;
	// size of the part file the list belongs to
	uint64 m_filesize;
	// number of its last part
	uint16 m_lastPart;
	// size of the last part
	uint32 m_sizeLastPart;
	// get size of any part
	uint32 GetPartSize(uint16 part) const { return part == m_lastPart ? m_sizeLastPart : PARTSIZE; }
public:
	// destruct
	~CGapList() { clear(); }
	// setup (and eventually clear) list, optionally as empty (one large gap)
	void Init(uint64 fileSize, bool empty);
	// just clear list (without gaps)
	void clear();
	// add a gap for a range
	void AddGap(uint64 start, uint64 end);
	// add a gap for a part
	void AddGap(uint16 part);
	// fill a gap for a range
	void FillGap(uint64 start, uint64 end);
	// fill a gap for a part
	void FillGap(uint16 part);
	// Is this range complete ?
	bool IsComplete(uint64 start, uint64 end) const;
	// Is this part complete ?
	bool IsComplete(uint16 part) const;
	// Is the whole file complete ?
	bool IsComplete() const { return m_gaplist.empty(); }
	// number of gaps
	uint32 size() const { return m_gaplist.size(); }
	// size of all gaps
	uint64 GetGapSize() const;
	// size of gaps inside one part
	uint32 GetGapSize(uint16 part) const;

	// Iterator class to loop through the gaps read-only
	// Gaps are returned just as start/end value
	class const_iterator {
		// iterator for internal list
		ListType::const_iterator m_it;
	public:
		// constructs
		const_iterator() {};
		const_iterator(const ListType::const_iterator& it) { m_it = it; };
		// operators
		bool operator != (const const_iterator& it) const { return m_it != it.m_it; }
		const_iterator& operator ++ () { ++ m_it; return *this; }
		// get start of gap pointed to
		uint64 start() const { return (* m_it)->start; }
		// get end of gap pointed to
		uint64 end() const { return (* m_it)->end; }
	};
	// begin/end iterators for looping
	const_iterator begin() const { return const_iterator(m_gaplist.begin()); }
	const_iterator end() const { return const_iterator(m_gaplist.end()); }

};

#endif // GAPLIST_H
// File_checked_for_headers
