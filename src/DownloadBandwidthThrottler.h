//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( admin@amule.org / http://www.amule.org )
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#ifndef DOWNLOADBANDWIDTHTHROTTLER_H
#define DOWNLOADBANDWIDTHTHROTTLER_H

#include "Types.h"
#include <atomic>
#include <cstdint>


// Global token bucket that enforces thePrefs::GetMaxDownload() as a literal
// byte/sec cap across all downloading sockets.
//
// Mirrors UploadBandwidthThrottler in role, but intentionally simpler:
// download is a PULL model where the kernel tells us when bytes are ready
// (asio OnReceive fires from the I/O thread), so there's no need for a
// dedicated throttler thread + condvar like the upload side. A shared
// atomic budget that every CEMSocket consults before each Read() is
// enough.
//
// Replaces the old per-peer ratio controller (DownloadQueue::Process +
// CUpDownClient::SetDownloadLimit), which never enforced MaxDownload as a
// literal cap and instead nudged each peer's transfer rate by ~5%/tick
// against its own current speed. With a single global bucket, fast peers
// can claim unused capacity from slow peers within the same tick
// (demand-aware redistribution); the only constraint is the global cap.
class CDownloadBandwidthThrottler
{
public:
	static CDownloadBandwidthThrottler& Get();

	// Refill the bucket at the start of each DownloadQueue tick.
	//   maxDownloadKBps == 0  -> unlimited mode (Reserve returns the full
	//                            request, no decrement).
	// Leftover from the previous tick is discarded: the cap is strict, not
	// burst-friendly. A burst-friendly accumulating bucket would let a
	// quiet period bank capacity and overshoot MaxDownload after data
	// resumes -- that's exactly the surprise the PR is trying to remove.
	void RefillBudget(uint32 maxDownloadKBps, uint32 tickPeriodMs);

	// Reserve up to wantBytes from the shared budget. Returns the actual
	// number of bytes the caller may read this round (in [0, wantBytes]).
	// Returning 0 means the bucket is exhausted; the caller should
	// suspend reads (set pendingOnReceive on the socket) and wait for the
	// next refill to wake it.
	uint32 Reserve(uint32 wantBytes);

	// Refund unused bytes to the shared budget. Used when Reserve granted
	// more than Read() actually returned (TCP partial reads, EOF, etc.) so
	// the unused capacity stays available to other peers in the same tick.
	void Refund(uint32 bytes);

private:
	CDownloadBandwidthThrottler() = default;

	std::atomic<int64_t> m_bytesAvailable{ 0 };
	std::atomic<bool>    m_unlimited{ true };
};

#endif // DOWNLOADBANDWIDTHTHROTTLER_H
