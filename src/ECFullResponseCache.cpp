//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2016 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ECFullResponseCache.h"

#include "KnownFile.h"		// For CKnownFile::GetGlobalECGen()


CECFullResponseCache::CECFullResponseCache(Builder builder)
	: m_cachedGen(0)
	, m_builder(std::move(builder))
{}


std::shared_ptr<const CECPacket> CECFullResponseCache::Get()
{
	const uint64 now_gen = CKnownFile::GetGlobalECGen();
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_cached && m_cachedGen == now_gen) {
			return m_cached;
		}
	}

	// Rebuild outside the lock: this is the slow part (iterating the
	// shareset / download queue and constructing per-file tag trees),
	// and we don't want to serialize every reader on it. Two concurrent
	// rebuilds for the same generation are harmless — they each produce
	// a logically equivalent tree; the last writer to take the lock
	// wins, the earlier one's tree is dropped by the shared_ptr
	// refcount as soon as its caller releases it. Total cost is still
	// bounded: rebuilds happen at most once per advance of
	// s_globalEcGen.
	std::shared_ptr<const CECPacket> fresh(m_builder());

	{
		std::lock_guard<std::mutex> lk(m_mutex);
		// Skip the gen re-check on purpose. If s_globalEcGen advanced
		// while we were building, our snapshot is stale relative to
		// the *latest* state but still a coherent snapshot of *some*
		// recent state; the next caller will see the newer gen and
		// trigger another rebuild. Stamping with our own snapshot
		// keeps the cache monotonic (we never go backwards).
		m_cached = fresh;
		m_cachedGen = now_gen;
	}
	return fresh;
}
