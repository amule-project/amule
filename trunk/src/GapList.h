//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <map>

class CGapList {
private:
	// The internal gap list:
	// Each gap is stored as a map entry. 
	// The first (key) is the end, the second (value) the start.
	typedef std::map<uint64,uint64> ListType;
	typedef ListType::iterator iterator;
	ListType m_gaplist;
	// size of the part file the list belongs to
	uint64 m_filesize;
	// number of parts
	uint16 m_iPartCount;
	// size of the last part
	uint32 m_sizeLastPart;
	// total gapsize
	uint64 m_totalGapSize;
	// flag if it's valid
	bool m_totalGapSizeValid;

	// cache completeness of parts
	enum ePartComplete {
		complete,
		incomplete,
		unknown
	};
	std::vector<byte> m_partsComplete;

	// get size of any part
	uint32 GetPartSize(uint16 part) const { return part == m_iPartCount - 1 ? m_sizeLastPart : PARTSIZE; }
	// check arguments, clip end, false: error
	inline bool ArgCheck(uint64 gapstart, uint64 &gapend) const;
public:
	// construct
	CGapList() { Init(0, false); } // NO MORE uninitialized variables >:(
	// setup (and eventually clear) list, optionally as empty (one large gap)
	void Init(uint64 fileSize, bool empty);
	// add a gap for a range
	void AddGap(uint64 gapstart, uint64 gapend);
	// add a gap for a part
	void AddGap(uint16 part);
	// fill a gap for a range
	void FillGap(uint64 gapstart, uint64 gapend);
	// fill a gap for a part
	void FillGap(uint16 part);
	// Is this range complete ?
	bool IsComplete(uint64 gapstart, uint64 gapend) const;
	// Is this part complete ?
	bool IsComplete(uint16 part);
	// Is the whole file complete ?
	bool IsComplete() const { return m_gaplist.empty(); }
	// number of gaps
	uint32 size() const { return m_gaplist.size(); }
	// no gaps ?
	bool empty() const { return m_gaplist.empty(); }
	// size of all gaps
	uint64 GetGapSize();
	// size of gaps inside one part
	uint32 GetGapSize(uint16 part) const;
	// print list for debugging
	void DumpList();

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
		uint64 start() const { return (* m_it).second; }
		// get end of gap pointed to
		uint64 end() const { return (* m_it).first; }
	};
	// begin/end iterators for looping
	const_iterator begin() const { return const_iterator(m_gaplist.begin()); }
	const_iterator end() const { return const_iterator(m_gaplist.end()); }

};

#endif // GAPLIST_H
// File_checked_for_headers
