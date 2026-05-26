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

#ifndef EC_FULL_RESPONSE_CACHE_H
#define EC_FULL_RESPONSE_CACHE_H

#include <ec/cpp/ECPacket.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include "Types.h"


/**
 * Daemon-wide cache for an EC FULL response packet.
 *
 * Wraps a builder function that constructs a CECPacket from current
 * daemon state. The packet is rebuilt only when the global EC
 * generation counter (CKnownFile::GetGlobalECGen, bumped by every
 * MarkECChanged() hook) has advanced since the last build. Between
 * changes, every consumer gets the same shared_ptr — the same tag
 * tree is serialized to each connection by the existing
 * CECSocket::SendPacket / WritePacket path, but the per-file
 * CEC_SharedFile_Tag / CEC_PartFile_Tag construction (the bulk of the
 * cost for big libraries) runs once per change, not once per request.
 *
 * Designed for the unfiltered FULL paths only:
 *   * EC_OP_GET_SHARED_FILES  with EC_DETAIL_FULL and no queryitems
 *     (amulecmd `show shared`).
 *   * EC_OP_GET_DLOAD_QUEUE   with EC_DETAIL_FULL and no queryitems
 *     (amulecmd `show DL`).
 *
 * INC_UPDATE / EC_DETAIL_UPDATE paths build per-connection diffs and
 * cannot share a tag tree across connections — they bypass the cache
 * and stay on the existing per-connection code path. Requests with
 * queryitems populated (amuleweb's phase-3 follow-up for newly
 * discovered file IDs) ask for a small filtered subset and also bypass
 * the cache.
 */
class CECFullResponseCache {
public:
	/**
	 * Builder returns a freshly-allocated CECPacket. Ownership
	 * transfers to the cache (held via shared_ptr).
	 */
	using Builder = std::function<CECPacket*()>;

	explicit CECFullResponseCache(Builder builder);

	/**
	 * Returns a shared pointer to a CECPacket whose tag tree matches
	 * the daemon state at the most-recent global EC generation. If
	 * the cached snapshot is stale (or absent), runs the builder
	 * before returning.
	 *
	 * Thread-safe. Multiple concurrent callers may all see a stale
	 * cache and trigger overlapping rebuilds — that's intentional
	 * (cheaper than serializing every request on the rebuild lock);
	 * the last completed build wins, all callers see a consistent
	 * snapshot.
	 */
	std::shared_ptr<const CECPacket> Get();

private:
	std::mutex m_mutex;
	std::shared_ptr<const CECPacket> m_cached;
	uint64 m_cachedGen;
	Builder m_builder;
};


#endif // EC_FULL_RESPONSE_CACHE_H
