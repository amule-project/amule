//
// This file is part of the aMule Project.
//
// Copyright (c) 2026 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "SharedDirsApplyTask.h"

#include <set>

#include <common/FileFunctions.h>		// CDirIterator

wxDEFINE_EVENT(wxEVT_SHARED_DIRS_APPLY_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SHARED_DIRS_APPLY_DONE,     wxThreadEvent);

// Coalesce progress events so the UI thread isn't flooded — every
// 256 directories is enough for a responsive bar without dominating
// the wxQueueEvent path.
static constexpr size_t kProgressBatch = 256;


CSharedDirsApplyTask::CSharedDirsApplyTask(const PathList & explicit_shares,
		const PathList & recursive_shares,
		wxEvtHandler * owner) :
	wxThread(wxTHREAD_JOINABLE),
	m_explicit(explicit_shares),
	m_recursive(recursive_shares),
	m_owner(owner)
{
}


void CSharedDirsApplyTask::Cancel()
{
	if (IsRunning()) {
		// wxThread::Delete on a joinable thread requests termination
		// and waits for the worker to exit. The worker checks via
		// TestDestroy() between directory steps and bails promptly.
		Delete();
	}
}


wxThread::ExitCode CSharedDirsApplyTask::Entry()
{
	// Start from the explicit set so subdir-overlap dedup happens in
	// one place. Use a set keyed on the raw path for O(log N) dedup
	// against later inserts from the recursive walk.
	std::set<wxString> seen;
	for (const CPath & p : m_explicit) {
		if (p.IsOk() && p.DirExists()) {
			if (seen.insert(p.GetRaw()).second) {
				m_output.push_back(p);
			}
		}
	}

	for (const CPath & root : m_recursive) {
		if (TestDestroy()) {
			m_cancelled.store(true);
			break;
		}
		if (!root.IsOk() || !root.DirExists()) {
			continue;
		}
		if (seen.insert(root.GetRaw()).second) {
			m_output.push_back(root);
		}
		ExpandRecursive(root);
	}

	// Force a final progress flush so the UI sees the true total
	// before the DONE event lands.
	if (!m_cancelled.load() && m_scanned.load() != m_lastReportedScanned) {
		PostProgress();
	}

	if (m_owner) {
		auto * done = new wxThreadEvent(wxEVT_SHARED_DIRS_APPLY_DONE);
		done->SetInt(m_cancelled.load() ? 1 : 0);
		wxQueueEvent(m_owner, done);
	}
	return (ExitCode)0;
}


void CSharedDirsApplyTask::ExpandRecursive(const CPath & root)
{
	// Iterative walk so a deep tree doesn't blow the worker's stack
	// and so the cancel check can happen between *every* directory
	// rather than only on the way back up.
	std::vector<CPath> queue;
	queue.push_back(root);

	while (!queue.empty()) {
		if (TestDestroy()) {
			m_cancelled.store(true);
			return;
		}

		CPath dir = queue.back();
		queue.pop_back();

		// Enumerate immediate subdirectories. CDirIterator's first
		// GetFirstFile returns an invalid CPath if the dir can't be
		// opened (permission denied, vanished, etc.), so a simple
		// IsOk() check on the first result is enough to skip
		// unreadable trees without aborting the whole task.
		CDirIterator finder(dir);
		for (CPath sub = finder.GetFirstFile(CDirIterator::DirNoHidden);
			sub.IsOk();
			sub = finder.GetNextFile())
		{
			CPath fullSub = dir.JoinPaths(sub);
			if (m_output.empty()
				|| m_output.back().GetRaw() != fullSub.GetRaw())
			{
				// Cheap last-write dedup; full dedup happens via
				// CPreferences::ReloadSharedFolders later.
				m_output.push_back(fullSub);
			}
			queue.push_back(fullSub);
		}

		const size_t now = ++m_scanned;
		if (now - m_lastReportedScanned >= kProgressBatch) {
			PostProgress();
		}
	}
}


void CSharedDirsApplyTask::PostProgress()
{
	const size_t now = m_scanned.load();
	m_lastReportedScanned = now;
	if (!m_owner) {
		return;
	}
	auto * ev = new wxThreadEvent(wxEVT_SHARED_DIRS_APPLY_PROGRESS);
	ev->SetInt(static_cast<int>(now));
	wxQueueEvent(m_owner, ev);
}
