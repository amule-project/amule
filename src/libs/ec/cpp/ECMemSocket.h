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

#ifndef EC_MEM_SOCKET_H
#define EC_MEM_SOCKET_H

#include "ECSocket.h"
#include "ECTag.h"

#include <vector>


/**
 * In-memory CECSocket sink. All Internal* I/O is stubbed; bytes that
 * the upper layers would have written to a real socket are captured
 * into an in-memory vector instead.
 *
 * Intended use is daemon-internal pre-serialization: build a CECTag
 * tree, call `tag.Serialize(mem)` against an instance of this class,
 * then take the captured bytes for caching. The cached bytes are
 * later written back through a real connection's CECSocket via
 * `WriteBuffer` (which applies per-connection deflate when required),
 * so the cached format must match what a remote receiver expects.
 *
 * Wire format used by the captured bytes: UTF-8 numbers +
 * LARGE_TAG_COUNT, no zlib. Every modern EC client advertises both
 * capability flags, so cached blobs are reusable across any client
 * the daemon talks to today.
 */
class CECMemSocket : public CECSocket {
public:
	CECMemSocket();

	/**
	 * Serialize a single tag (name + type + length + body + nested
	 * children) into a stand-alone byte vector. The bytes can later
	 * be re-emitted on a real CECSocket via `WriteBuffer` as a single
	 * child slot inside a larger response.
	 */
	static std::vector<unsigned char> SerializeTag(const CECTag &tag);

	/// Hand ownership of the captured bytes to the caller. Subsequent
	/// reads return an empty vector.
	std::vector<unsigned char> TakeBytes() { return std::move(m_bytes); }

private:
	std::vector<unsigned char> m_bytes;

	// CECSocket virtuals — every actual I/O hook is a no-op except
	// InternalWrite, which appends to m_bytes.
	void WriteDoneAndQueueEmpty() override {}
	bool InternalConnect(uint32_t, uint16_t, bool) override { return true; }
	bool InternalWaitOnConnect(long, long) override { return true; }
	bool InternalWaitForWrite(long, long) override { return true; }
	bool InternalWaitForRead(long, long) override { return true; }
	int  InternalGetLastError() override { return 0; }
	void InternalClose() override {}
	bool InternalError() override { return false; }
	uint32 InternalRead(void *, uint32) override { return 0; }
	uint32 InternalWrite(const void *ptr, uint32 len) override;
	bool InternalIsConnected() override { return true; }
	void InternalDestroy() override {}
};


#endif // EC_MEM_SOCKET_H
