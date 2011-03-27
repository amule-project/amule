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


#include "Types.h"
#include <protocol/ed2k/Constants.h>	// for PARTSIZE
#include "GapList.h"

#include "Logger.h"
#include <common/Format.h>

void CGapList::Init(uint64 fileSize, bool isEmpty)
{
	m_filesize = fileSize;
	m_iPartCount = fileSize / PARTSIZE + 1;
	m_sizeLastPart = fileSize % PARTSIZE;
	// file with size of n * PARTSIZE
	if (m_sizeLastPart == 0 
		&& fileSize) {  // that's only for pre-init in ctor
		m_sizeLastPart = PARTSIZE;
		m_iPartCount--;
	}
	m_gaplist.clear();
	m_partsComplete.clear();
	if (isEmpty) {
		m_partsComplete.resize(m_iPartCount, incomplete);
		AddGap(0, fileSize - 1);
	} else {
		m_partsComplete.resize(m_iPartCount, complete);
	}
	m_totalGapSizeValid = false;
}


void CGapList::AddGap(uint64 gapstart, uint64 gapend)
{
	if (!ArgCheck(gapstart, gapend)) {
		return;
	}

//	AddDebugLogLineN(logPartFile, CFormat(wxT("  AddGap: %5d - %5d")) % gapstart % gapend);

	// mark involved part(s) as incomplete
	uint16 partlast = gapend / PARTSIZE;
	for (uint16 part = gapstart / PARTSIZE; part <= partlast; part++) {
		m_partsComplete[part] = incomplete;
	}
	// total gap size has to be recalculated
	m_totalGapSizeValid = false;

	// find a place to start:
	// first gap which ends >= our gap start - 1
	// (-1 so we can join adjacent gaps)
	iterator it = m_gaplist.lower_bound(gapstart > 0 ? gapstart - 1 : 0);
	while (it != m_gaplist.end()) {
		iterator it2 = it++;
		uint64 curGapStart = it2->second;
		uint64 curGapEnd   = it2->first;

		if (curGapStart >= gapstart && curGapEnd <= gapend) {
			// this gap is inside the new gap - delete
			m_gaplist.erase(it2);
		} else if (curGapStart >= gapstart && curGapStart <= gapend + 1) {
			// head of this gap is in the new gap, or this gap is
			// directly behind the new gap - extend limit and delete
			gapend = curGapEnd;
			m_gaplist.erase(it2);
		} else if (curGapEnd <= gapend && curGapEnd >= gapstart - 1) {
			// tail of this gap is in the new gap, or this gap is
			// directly before the new gap - extend limit and delete
			gapstart = curGapStart;
			m_gaplist.erase(it2);
		} else if (curGapStart <= gapstart && curGapEnd >= gapend) {
			// new gap is already inside this gap - return
			return;
		// now all cases of overlap are ruled out
		} else if (curGapStart > gapstart) {
			// this gap is the first behind the new gap -> insert before it
			it = it2;
			break;
		}
	}
	// for fastest insertion point to the element AFTER which we want to insert	
	if (it != m_gaplist.begin()) {
		--it;
	}
	m_gaplist.insert(it, std::pair<uint64,uint64>(gapend, gapstart));
}

void CGapList::AddGap(uint16 part)
{
	if (part >= m_iPartCount) {
		wxFAIL;
		return;
	}
	uint64 gapstart = part * PARTSIZE;
	uint64 gapend = gapstart + GetPartSize(part) - 1;
	AddGap(gapstart, gapend);
	m_partsComplete[part] = incomplete;
}

void CGapList::FillGap(uint64 partstart, uint64 partend)
{
	if (!ArgCheck(partstart, partend)) {
		return;
	}

//	AddDebugLogLineN(logPartFile, CFormat(wxT("  FillGap: %5d - %5d")) % partstart % partend);

	// mark involved part(s) to be reexamined for completeness
	uint16 partlast = partend / PARTSIZE;
	for (uint16 part = partstart / PARTSIZE; part <= partlast; part++) {
		m_partsComplete[part] = unknown;
	}
	// also total gap size
	m_totalGapSizeValid = false;

	// find a place to start:
	// first gap which ends >= our part start
	iterator it = m_gaplist.lower_bound(partstart);
	while (it != m_gaplist.end()) {
		iterator it2 = it++;
		uint64 curGapStart = it2->second;
		uint64 curGapEnd   = it2->first;

		if (curGapStart >= partstart) {
			if (curGapEnd <= partend) {
				// our part fills this gap completly
				m_gaplist.erase(it2);
			} else if (curGapStart <= partend) {
				// lower part of this gap is in the part - shrink gap:
				//   (this is the most common case: curGapStart == partstart && curGapEnd > partend)
				it2->second = partend + 1;
				// end of our part was in the gap: we're done
				break;
			} else {
				// gap starts behind our part end: we're done
				break;
			}
		} else {
			// curGapStart < partstart
			if (curGapEnd > partend) {
				// our part is completely enclosed by the gap
				// cut it in two, leaving our part out:
				// shrink the gap so it becomes the second gap
				it2->second = partend + 1;
				// insert new first gap
				iterator it3(it2);
				if (it3 != m_gaplist.begin()) {
					--it3;
				}
				m_gaplist.insert(it3, std::pair<uint64,uint64>(partstart - 1, curGapStart));
				// we're done
				break;
			} else if (curGapEnd >= partstart) {
				// upper part of this gap is in the part - shrink gap:
				// insert shorter gap
				iterator it3(it2);
				if (it3 != m_gaplist.begin()) {
					--it3;
				}
				m_gaplist.insert(it3, std::pair<uint64,uint64>(partstart - 1, curGapStart));
				// and delete the old one
				m_gaplist.erase(it2);
			}
			// else: gap is before our part start (should not happen)
		}
	}
}

void CGapList::FillGap(uint16 part)
{
	if (part >= m_iPartCount) {
		wxFAIL;
		return;
	}
	uint64 gapstart = part * PARTSIZE;
	uint64 gapend = gapstart + GetPartSize(part) - 1;
	FillGap(gapstart, gapend);
	m_partsComplete[part] = complete;
}

uint64 CGapList::GetGapSize()
{
	if (!m_totalGapSizeValid) {
		m_totalGapSizeValid = true;
   		m_totalGapSize = 0;

		ListType::const_iterator it = m_gaplist.begin();
		for (; it != m_gaplist.end(); it++) {
			m_totalGapSize += it->first - it->second + 1;
		}
	}
	return m_totalGapSize;
}

uint32 CGapList::GetGapSize(uint16 part) const
{
	uint64 uRangeStart = part * PARTSIZE;
	uint64 uRangeEnd = uRangeStart + GetPartSize(part) - 1;
	uint64 uTotalGapSize = 0;

	// find a place to start:
	// first gap which ends >= our gap start
	ListType::const_iterator it = m_gaplist.lower_bound(uRangeStart);
	for (; it != m_gaplist.end(); ++it) {
		uint64 curGapStart = it->second;
		uint64 curGapEnd   = it->first;

		if (curGapStart <= uRangeStart && curGapEnd >= uRangeEnd) {
			// total range is in this gap
			uTotalGapSize += uRangeEnd - uRangeStart + 1;
			break;
		} else if (curGapStart >= uRangeStart) {
			if (curGapStart <= uRangeEnd) {
				// start of this gap is in our range
				uTotalGapSize += std::min(curGapEnd, uRangeEnd) - curGapStart + 1;
			} else {
				// this gap starts behind our range
				break;
			}
		} else if (curGapEnd >= uRangeStart && curGapEnd <= uRangeEnd) {
			// end of this gap is in our range
			uTotalGapSize += curGapEnd - uRangeStart + 1;
		}
	}

	wxASSERT( uTotalGapSize <= uRangeEnd - uRangeStart + 1 );
	return uTotalGapSize;
}

bool CGapList::IsComplete(uint64 gapstart, uint64 gapend) const
{
	if (!ArgCheck(gapstart, gapend)) {
		return false;
	}

	// find a place to start:
	// first gap which ends >= our gap start
	ListType::const_iterator it = m_gaplist.lower_bound(gapstart);
	for (; it != m_gaplist.end(); ++it) {
		uint64 curGapStart = it->second;
		uint64 curGapEnd   = it->first;

		if (  (curGapStart >= gapstart    && curGapEnd   <= gapend)
			||(curGapStart >= gapstart    && curGapStart <= gapend)
			||(curGapEnd   <= gapend      && curGapEnd   >= gapstart)
			||(gapstart    >= curGapStart && gapend      <= curGapEnd)) {
			return false;	
		}
		if (curGapStart > gapend) {
			break;
		}
	}
	return true;
}

bool CGapList::IsComplete(uint16 part)
{
// There is a bug in the ED2K protocol:
// For files of size n * PARTSIZE one part too much is transmitted in the availability bitfield.
// Allow completion detection of this dummy part, and always report it as complete
// (so it doesn't get asked for).
	if (part == m_iPartCount && m_sizeLastPart == PARTSIZE) {
		return true;
	}
// Remaining error check
	if (part >= m_iPartCount) {
		wxFAIL;
		return false;
	}
	ePartComplete status = (ePartComplete) m_partsComplete[part];
	if (status == unknown) {
		uint64 partstart = part * PARTSIZE;
		uint64 partend = partstart + GetPartSize(part) - 1;
		status = IsComplete(partstart, partend) ? complete : incomplete;
		m_partsComplete[part] = status;
	}
	return status == complete;
}

void CGapList::DumpList()
{
	int i = 0;
	for (const_iterator it = begin(); it != end(); ++it) {
		AddDebugLogLineN(logPartFile, CFormat(wxT("  %3d: %5d - %5d")) % i++ % it.start() % it.end());
	}
}

inline bool CGapList::ArgCheck(uint64 gapstart, uint64 &gapend) const
{
	// end < start: serious error
	if (gapend < gapstart) {
		wxFAIL;
		return false;
	}

	// gaps shouldn't go past file anymore either
	if (gapend >= m_filesize) {
		wxFAIL;
		gapend = m_filesize - 1;	// fix it
	}
	return true;
}
