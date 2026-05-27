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

#include <set>


CECFullResponseCache::CECFullResponseCache(TagBuilder builder)
	: m_builder(std::move(builder))
{}


std::vector<std::shared_ptr<const std::vector<unsigned char> > >
CECFullResponseCache::GetBlobs(const std::vector<FileRef> &files)
{
	std::vector<std::shared_ptr<const std::vector<unsigned char> > > result;
	result.reserve(files.size());

	// First pass under the lock: collect cached entries that are still
	// fresh; mark stale / missing entries for build.
	std::vector<size_t> to_build;
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		for (size_t i = 0; i < files.size(); ++i) {
			std::map<uint32, Entry>::iterator it = m_entries.find(files[i].ecid);
			if (it != m_entries.end() && it->second.gen >= files[i].gen) {
				result.push_back(it->second.bytes);
			} else {
				// Placeholder; filled in below
				result.push_back(std::shared_ptr<const std::vector<unsigned char> >());
				to_build.push_back(i);
			}
		}
	}

	// Build stale / missing entries outside the lock. Tag construction
	// + serialization is the slow part — holding the cache mutex
	// through it would serialise every concurrent reader on the first
	// thread's rebuild.
	for (size_t i = 0; i < to_build.size(); ++i) {
		const size_t idx = to_build[i];
		const FileRef &ref = files[idx];

		std::unique_ptr<CECTag> tag(m_builder(ref.file));
		std::shared_ptr<const std::vector<unsigned char> > bytes(
			new std::vector<unsigned char>(CECMemSocket::SerializeTag(*tag)));

		// Re-lock briefly to install. Two concurrent rebuilds for the
		// same file are tolerated: each produces a logically
		// equivalent blob, the last writer wins, both callers' result
		// vectors point at a valid blob via the shared_ptr they
		// already took out.
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			Entry &slot = m_entries[ref.ecid];
			slot.gen = ref.gen;
			slot.bytes = bytes;
		}
		result[idx] = bytes;
	}

	return result;
}


void CECFullResponseCache::PruneOutsideOf(const std::vector<FileRef> &alive_files)
{
	std::set<uint32> alive;
	for (size_t i = 0; i < alive_files.size(); ++i) {
		alive.insert(alive_files[i].ecid);
	}

	std::lock_guard<std::mutex> lk(m_mutex);
	for (std::map<uint32, Entry>::iterator it = m_entries.begin();
		it != m_entries.end(); ) {
		if (!alive.count(it->first)) {
			m_entries.erase(it++);
		} else {
			++it;
		}
	}
}
