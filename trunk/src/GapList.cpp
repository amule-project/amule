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


#include "Types.h"
#include <protocol/ed2k/Constants.h>	// for PARTSIZE
#include "GapList.h"
#include "OtherFunctions.h"	// for DeleteContents

void CGapList::Init(uint64 fileSize, bool empty)
{
	m_filesize = fileSize;
	m_lastPart = fileSize / PARTSIZE;
	m_sizeLastPart = fileSize % PARTSIZE;
	// file with size of n * PARTSIZE
	if (m_sizeLastPart == 0) {
		m_sizeLastPart = PARTSIZE;
		m_lastPart--;
	}
	clear();
	if (empty) {
		AddGap(0, fileSize - 1);
	}
}


void CGapList::AddGap(uint64 start, uint64 end)
{
	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	while (it != m_gaplist.end()) {
		std::list<Gap_Struct*>::iterator it2 = it++;
		Gap_Struct* cur_gap = *it2;
	
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// this gap is inside the new gap - delete
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end + 1) {
			// head of this gap is in the new gap, or this gap is
			// directly behind the new gap - extend limit and delete
			end = cur_gap->end;
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->end <= end && cur_gap->end >= start - 1) {
			// tail of this gap is in the new gap, or this gap is
			// directly before the new gap - extend limit and delete
			start = cur_gap->start;
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (start >= cur_gap->start && end <= cur_gap->end){
			// new gap is already inside this gap - return
			return;
		// now all cases of overlap are ruled out
		} else if (cur_gap->start > start) {
			// this gap is the first behind the new gap -> insert before it
			it = it2;
			break;
		}
	}
	
	Gap_Struct* new_gap = new Gap_Struct;
	new_gap->start = start;
	new_gap->end = end;
	m_gaplist.insert(it, new_gap);
}

void CGapList::AddGap(uint16 part)
{
	uint64 start = part * PARTSIZE;
	uint64 end = start + GetPartSize(part) - 1;
	AddGap(start, end);
}

void CGapList::FillGap(uint64 start, uint64 end)
{
	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	while (it != m_gaplist.end()) {
		std::list<Gap_Struct*>::iterator it2 = it++;
		Gap_Struct* cur_gap = *it2;
	
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// our part fills this gap completly
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		} else if (start >= cur_gap->start && end <= cur_gap->end) {
			uint64 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			m_gaplist.insert(++it2, cur_gap);
			break;
		}
	}
}

void CGapList::FillGap(uint16 part)
{
	uint64 start = part * PARTSIZE;
	uint64 end = start + GetPartSize(part) - 1;
	FillGap(start, end);
}

uint64 CGapList::GetGapSize() const
{
   	uint64 allgaps = 0;

	std::list<Gap_Struct*>::const_iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); it++) {
		Gap_Struct* cur_gap = *it;
		allgaps += cur_gap->end - cur_gap->start + 1;
	}

	return allgaps;
}

uint32 CGapList::GetGapSize(uint16 part) const
{
	uint64 uRangeStart = part * PARTSIZE;
	uint64 uRangeEnd = uRangeStart + GetPartSize(part) - 1;
	uint64 uTotalGapSize = 0;

	if (uRangeEnd >= m_filesize) {
		uRangeEnd = m_filesize - 1;
	}

	std::list<Gap_Struct*>::const_iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); ++it) {
		const Gap_Struct* pGap = *it;

		if (pGap->start < uRangeStart && pGap->end > uRangeEnd) {
			uTotalGapSize += uRangeEnd - uRangeStart + 1;
			break;
		}

		if (pGap->start >= uRangeStart && pGap->start <= uRangeEnd) {
			uint64 uEnd = (pGap->end > uRangeEnd) ? uRangeEnd : pGap->end;
			uTotalGapSize += uEnd - pGap->start + 1;
		} else if (pGap->end >= uRangeStart && pGap->end <= uRangeEnd) {
			uTotalGapSize += pGap->end - uRangeStart + 1;
		}
	}

	wxASSERT( uTotalGapSize <= uRangeEnd - uRangeStart + 1 );

	return uTotalGapSize;
}

bool CGapList::IsComplete(uint64 start, uint64 end) const
{
	if (end >= m_filesize) {
		end = m_filesize-1;
	}

	std::list<Gap_Struct*>::const_iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); ++it) {
		Gap_Struct* cur_gap = *it;
		if (  (cur_gap->start >= start && cur_gap->end <= end)
			||(cur_gap->start >= start && cur_gap->start <= end)
			||(cur_gap->end <= end && cur_gap->end >= start)
			||(start >= cur_gap->start && end <= cur_gap->end)) {
			return false;	
		}
		if (cur_gap->start > end) {
			break;
		}
	}
	return true;
}

void CGapList::clear()
{
	DeleteContents(m_gaplist);
}
