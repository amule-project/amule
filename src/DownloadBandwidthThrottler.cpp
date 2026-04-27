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

#include "DownloadBandwidthThrottler.h"

#include <climits>


CDownloadBandwidthThrottler& CDownloadBandwidthThrottler::Get()
{
	static CDownloadBandwidthThrottler s_instance;
	return s_instance;
}


void CDownloadBandwidthThrottler::RefillBudget(uint32 maxDownloadKBps, uint32 tickPeriodMs)
{
	if (maxDownloadKBps == 0) {
		// MaxDownload=0 means literal unlimited. Saturate the bucket so
		// even if a Reserve() raced past the m_unlimited check it would
		// still return the full request.
		m_unlimited.store(true, std::memory_order_release);
		m_bytesAvailable.store(INT64_MAX, std::memory_order_release);
		return;
	}

	// uint64 intermediate so a MaxDownload up to the UI cap (1 000 000 KB/s)
	// doesn't overflow uint32: 1 000 000 * 1024 * 300 = 3 * 10^11 > 2^32.
	const int64_t budget =
		(int64_t)maxDownloadKBps * 1024 * tickPeriodMs / 1000;

	m_unlimited.store(false, std::memory_order_release);
	// Add this tick's budget to whatever leftover was unconsumed last
	// tick, but cap the bucket at 2x budget so a long quiet period
	// can't bank capacity that bursts well past the average cap.
	// Strict overwrite (no carry-over) starves TCP: the receiver pauses
	// reads when the bucket empties mid-tick, the seeder's TCP flow
	// control reads that as "consumer overloaded" and slows down, and
	// by the time the bucket refills the seeder isn't sending fast
	// enough to consume the new budget. A small carry-over keeps
	// reads flowing across tick boundaries.
	int64_t current = m_bytesAvailable.load(std::memory_order_acquire);
	if (current < 0) {
		current = 0;
	}
	int64_t newBudget = current + budget;
	const int64_t cap = budget * 2;
	if (newBudget > cap) {
		newBudget = cap;
	}
	m_bytesAvailable.store(newBudget, std::memory_order_release);
}


uint32 CDownloadBandwidthThrottler::Reserve(uint32 wantBytes)
{
	if (m_unlimited.load(std::memory_order_acquire)) {
		return wantBytes;
	}

	int64_t current = m_bytesAvailable.load(std::memory_order_acquire);
	while (current > 0) {
		const uint32 granted = (current < (int64_t)wantBytes)
			? (uint32)current
			: wantBytes;
		if (m_bytesAvailable.compare_exchange_weak(
				current,
				current - granted,
				std::memory_order_acq_rel,
				std::memory_order_acquire)) {
			return granted;
		}
		// CAS failed; `current` was reloaded with the latest value.
	}
	return 0;
}


void CDownloadBandwidthThrottler::Refund(uint32 bytes)
{
	if (bytes == 0 || m_unlimited.load(std::memory_order_acquire)) {
		return;
	}
	m_bytesAvailable.fetch_add((int64_t)bytes, std::memory_order_acq_rel);
}
