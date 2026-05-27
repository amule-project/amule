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

#include "ECMemSocket.h"

#include "ECCodes.h"		// Needed for EC_FLAG_*


CECMemSocket::CECMemSocket()
:
	CECSocket(false)
{
	// Lock the canonical wire format the cache assumes its consumers can
	// decode: UTF-8 numbers + sentinel-extended children count, no zlib.
	// Both capability flags are advertised by every modern client; the
	// cached bytes are reusable regardless of which connection ends up
	// writing them out (per-connection compression is layered on by the
	// real socket's WriteBuffer at send time, separately from this).
	m_my_flags |= EC_FLAG_UTF8_NUMBERS | EC_FLAG_LARGE_TAG_COUNT;
	SetTxFlags(m_my_flags);
}


uint32 CECMemSocket::InternalWrite(const void *ptr, uint32 len)
{
	const unsigned char *p = static_cast<const unsigned char *>(ptr);
	m_bytes.insert(m_bytes.end(), p, p + len);
	return len;
}


std::vector<unsigned char> CECMemSocket::SerializeTag(const CECTag &tag)
{
	CECMemSocket mem;
	// Walk the tag into the per-instance buffer chain. Serialize emits
	// name + type + length + body + nested children — a self-contained
	// blob that can be re-inserted as a child of any later response by
	// just writing the bytes through a real connection's WriteBuffer.
	tag.Serialize(mem);
	// Tag serialization does not zlib-encode (we asked for raw bytes by
	// keeping EC_FLAG_ZLIB out of m_tx_flags), so FlushBuffers has no
	// trailing deflate state to flush — but it does cycle the in-flight
	// CQueuedData into m_output_queue, which is what we drain next.
	mem.FlushBuffers();
	mem.OnOutput();		// drains m_output_queue → InternalWrite → m_bytes
	return mem.TakeBytes();
}
