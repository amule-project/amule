//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

#include <ec/cpp/ECMemSocket.h>
#include <ec/cpp/ECTag.h>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "Types.h"


/**
 * Per-file bytes cache for the FULL EC response paths.
 *
 * Backs the amulecmd `show shared` / `show DL` invocations: each one is
 * a fresh short-lived EC connection with no per-connection diff state,
 * so it always asks for `EC_DETAIL_FULL` and would otherwise force the
 * daemon to rebuild and serialize the entire shared-file / partfile
 * tag tree from scratch every time. On a 91 k-file shareset that's
 * ~10 s of CPU per invocation (issue #713).
 *
 * The cache stores one pre-serialized blob per file, keyed by ECID,
 * and freshness-stamped with the file's `m_ecGen` at the time of build.
 * On each request the daemon iterates the current snapshot of files,
 * reuses cached blobs whose gen is still current, and rebuilds only
 * the ones whose `m_ecGen` has advanced past the cached value
 * (the same per-file freshness primitive PR #727 introduced).
 *
 * A connection-side helper concatenates the per-file blobs with a
 * fresh opcode + children-count header and writes the result through
 * the real socket's `WriteBuffer` — per-connection compression (#728)
 * still applies because the cached bytes flow through the connection's
 * normal write path.
 *
 * The cached blobs use the canonical wire format chosen by
 * `CECMemSocket`: UTF-8 numbers + sentinel-extended children count, no
 * zlib. Both capability bits are advertised by every modern client, so
 * the cached form is reusable regardless of which connection ends up
 * emitting it.
 */
class CECFullResponseCache {
public:
	/// Build a self-contained CECTag for the given file. Caller owns
	/// the returned tag and is expected to throw it away after the
	/// cache serializes it.
	using TagBuilder = std::function<CECTag*(const void *file)>;

	explicit CECFullResponseCache(TagBuilder builder);

	/**
	 * Snapshot the per-file blobs for the given (file pointer, ECID,
	 * current m_ecGen) tuples. Rebuilds any stale entries inline.
	 * Returns the blobs in the same order as the inputs so the caller
	 * can write them straight to the wire.
	 *
	 * Concurrent callers each get a consistent set of bytes — rebuilds
	 * race-but-don't-corrupt (last write wins, both readers see a
	 * valid blob).
	 */
	struct FileRef {
		const void *file;	// passed to the builder
		uint32 ecid;
		uint64 gen;
	};

	std::vector<std::shared_ptr<const std::vector<unsigned char> > >
	GetBlobs(const std::vector<FileRef> &files);

	/// Drop cached entries for ECIDs not in the given set (called once
	/// per request after GetBlobs to prune files that left the
	/// shareset). Bounded memory.
	void PruneOutsideOf(const std::vector<FileRef> &alive_files);

private:
	struct Entry {
		uint64 gen;
		std::shared_ptr<const std::vector<unsigned char> > bytes;
	};

	std::mutex m_mutex;
	std::map<uint32, Entry> m_entries;
	TagBuilder m_builder;
};


#endif // EC_FULL_RESPONSE_CACHE_H
