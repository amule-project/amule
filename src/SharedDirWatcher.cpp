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

#include "SharedDirWatcher.h"

#include <wx/app.h>
#include <wx/evtloop.h>
#include <wx/filename.h>

#include <common/Format.h>		// Needed for CFormat

#include <common/Path.h>

#include "amule.h"
#include "Logger.h"
#include "Preferences.h"
#include "SharedFileList.h"

namespace {

// Watcher event mask used by every Add()/AddTree() call below.
constexpr int kWatchMask = wxFSW_EVENT_CREATE | wxFSW_EVENT_DELETE |
	wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY;

#ifdef __WXOSX__
// Returns true if `inner` is a strict descendant of `outer` (i.e. `inner`
// lives inside `outer`'s subtree, not equal to it). Used on macOS to skip
// shareddir_list entries whose ancestor is also in the list — wx FSEvents
// rejects overlapping tree watches by silently returning false from a
// second AddTree() on a path already covered by an earlier stream.
bool IsStrictlyInside(const CPath & inner, const CPath & outer)
{
	if (!inner.IsOk() || !outer.IsOk() || inner.IsSameDir(outer)) {
		return false;
	}
	wxString a = outer.GetRaw();
	if (a.empty() || a.Last() != wxFILE_SEP_PATH) {
		a += wxFILE_SEP_PATH;
	}
	return inner.GetRaw().StartsWith(a);
}
#endif

} // namespace

// Coalesce a burst of FS events into one Reload. 5 s is long enough to
// merge the thousands of events a tar-extract produces, short enough for
// a "drop a file into a shared folder" interaction to feel responsive.
// macOS FSEvents debounces ~30 s on its own anyway, so on Mac the user-
// perceived latency is bounded by FSEvents, not by this timer.
static constexpr int kDebounceMs = 5000;

// Timer IDs are wx-local; use one not clashing with the prefs dialog.
static const int ID_FSWATCHER_DEBOUNCE = wxID_HIGHEST + 8231;

BEGIN_EVENT_TABLE(CSharedDirWatcher, wxEvtHandler)
	EVT_FSWATCHER(wxID_ANY, CSharedDirWatcher::OnFileSystemEvent)
	EVT_TIMER(ID_FSWATCHER_DEBOUNCE, CSharedDirWatcher::OnDebounceTimer)
END_EVENT_TABLE()


CSharedDirWatcher::CSharedDirWatcher(CSharedFileList * parent) :
	m_parent(parent),
	m_watcher(NULL),
	m_debounceTimer(this, ID_FSWATCHER_DEBOUNCE)
{
}


CSharedDirWatcher::~CSharedDirWatcher()
{
	Disable();
}


void CSharedDirWatcher::Enable()
{
	if (m_watcher) {
		return;
	}

	// wx 3.2.x's inotify backend hard-requires an active wx event loop
	// when wxFileSystemWatcher's ctor runs — Init() checks
	// wxEventLoopBase::GetActive() and silently leaves m_service null
	// when there isn't one, after which every Add() returns false. The
	// daemon's CamuleApp::OnInit() runs before the event loop starts, so
	// the first Enable() call from there must be deferred. (FSEvents on
	// macOS and ReadDirectoryChangesW on Windows don't have this gate;
	// deferring is still safe there.) Subsequent calls from the prefs
	// dialog or EC apply path already run inside an active loop, so they
	// take the immediate branch.
	if (!wxEventLoopBase::GetActive()) {
		// Queue on `this` (a wxEvtHandler) rather than the app, so that
		// if Disable()/~CSharedDirWatcher() runs before the loop drains,
		// wx purges the pending event with the handler instead of firing
		// it against a dead object.
		CallAfter(&CSharedDirWatcher::Enable);
		return;
	}

	m_watcher = new wxFileSystemWatcher();
	m_watcher->SetOwner(this);
	RegisterAllPaths();
	AddDebugLogLineN(logKnownFiles, "Shared-dir watcher enabled");
}


void CSharedDirWatcher::Disable()
{
	if (m_debounceTimer.IsRunning()) {
		m_debounceTimer.Stop();
	}
	if (!m_watcher) {
		return;
	}
	delete m_watcher;
	m_watcher = NULL;
	AddDebugLogLineN(logKnownFiles, "Shared-dir watcher disabled");
}


void CSharedDirWatcher::Refresh()
{
	if (!m_watcher) {
		return;
	}
	// wxFileSystemWatcher::RemoveAll() drops every registration; we then
	// re-Add everything from the current (possibly updated) shareddir_list.
	// This is simpler than diffing old vs new path sets and is cheap given
	// the typical list size (low hundreds at most).
	m_watcher->RemoveAll();
	RegisterAllPaths();
}


void CSharedDirWatcher::RegisterAllPaths()
{
	const thePrefs::PathList & shared = theApp->glob_prefs->shareddir_list;

#ifdef __WXOSX__
	// macOS: route through AddTree() which uses FSEvents (kernel-level
	// recursive watch). wx 3.3.2's bare Add() falls through to the kqueue
	// base and returns false on otherwise-openable directories — verified
	// against wx upstream and reproduced locally. AddTree() is also the
	// API Apple recommends for directory monitoring, so this is the
	// preferred path even once the kqueue bug is fixed.
	//
	// AddTree() is recursive, so an entry whose ancestor is also in
	// shareddir_list would create overlapping streams; wx FSEvents
	// rejects the second AddTree() on the inner path. Pre-prune
	// descendants here so we only AddTree() each top-level entry.
	for (size_t i = 0; i < shared.size(); ++i) {
		const CPath & p = shared[i];
		if (!p.IsOk() || !p.DirExists()) {
			continue;
		}
		bool covered_by_ancestor = false;
		for (size_t j = 0; j < shared.size(); ++j) {
			if (i != j && IsStrictlyInside(p, shared[j])) {
				covered_by_ancestor = true;
				break;
			}
		}
		if (covered_by_ancestor) {
			continue;
		}
		wxFileName fn = wxFileName::DirName(p.GetRaw());
		if (!m_watcher->AddTree(fn, kWatchMask)) {
			AddDebugLogLineC(logKnownFiles,
				CFormat("Shared-dir watcher: failed to AddTree %s") % p.GetRaw());
		}
	}
#else
	// Linux (inotify), BSD (kqueue), Windows (ReadDirectoryChangesW):
	// per-dir Add(). shareddir_list already enumerates every subdirectory
	// individually when the user uses the recursive-share button, so
	// inotify watch count tracks shareddir_list.size() rather than total
	// subtree depth.
	for (size_t i = 0; i < shared.size(); ++i) {
		const CPath & p = shared[i];
		if (!p.IsOk() || !p.DirExists()) {
			continue;
		}
		wxFileName fn = wxFileName::DirName(p.GetRaw());
		if (!m_watcher->Add(fn, kWatchMask)) {
			// Most likely cause on Linux is hitting
			// /proc/sys/fs/inotify/max_user_watches. Log and continue —
			// partial coverage is better than zero coverage.
			AddDebugLogLineC(logKnownFiles,
				CFormat("Shared-dir watcher: failed to add %s") % p.GetRaw());
		}
	}
#endif
}


void CSharedDirWatcher::OnFileSystemEvent(wxFileSystemWatcherEvent & event)
{
	const int changeType = event.GetChangeType();
	const wxFileName & path = event.GetPath();

	// Auto-share new subdirectories of any watched path. A user who has
	// already shared /Music gets new subdirs of /Music auto-included
	// without having to revisit the prefs dialog. Existing subdirs that
	// were originally excluded (non-recursive share) are not
	// retroactively scanned — only newly-created ones get added.
	if ((changeType & wxFSW_EVENT_CREATE) && path.IsOk() && path.DirExists()) {
		RegisterNewSubdirectory(path.GetFullPath());
	}

	// Any other event class still indicates "something interesting
	// happened" — schedule a debounced Reload so the new state is
	// reflected in the shared file list.
	if (changeType & (wxFSW_EVENT_CREATE | wxFSW_EVENT_DELETE |
			wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY)) {
		ScheduleReload();
	}
}


void CSharedDirWatcher::RegisterNewSubdirectory(const wxString & path)
{
	CPath p(path);
	if (!p.IsOk() || !p.DirExists()) {
		return;
	}

	// Skip if already on the list (defensive — duplicate inotify
	// events for the same mkdir are possible).
	thePrefs::PathList & shared = theApp->glob_prefs->shareddir_list;
	for (size_t i = 0; i < shared.size(); ++i) {
		if (shared[i].IsSameDir(p)) {
			return;
		}
	}

	shared.push_back(p);

#ifndef __WXOSX__
	// Linux/BSD/Windows: explicitly register the new subdir so events
	// for its own contents are observed. On macOS the enclosing tree's
	// FSEvents stream already covers descendants, so nothing to add.
	if (m_watcher) {
		wxFileName fn = wxFileName::DirName(path);
		if (!m_watcher->Add(fn, kWatchMask)) {
			AddDebugLogLineC(logKnownFiles,
				CFormat("Shared-dir watcher: failed to add new subdir %s") % path);
		}
	}
#endif

	AddDebugLogLineN(logKnownFiles,
		CFormat("Shared-dir watcher: auto-shared new subdir %s") % path);

	// Persist the new entry so the change survives a restart.
	theApp->glob_prefs->SaveSharedFolders();
}


void CSharedDirWatcher::ScheduleReload()
{
	// Restart the timer on every event, so a burst of N events within
	// the debounce window collapses into one Reload at the end.
	m_debounceTimer.Start(kDebounceMs, wxTIMER_ONE_SHOT);
}


void CSharedDirWatcher::OnDebounceTimer(wxTimerEvent & WXUNUSED(event))
{
	AddDebugLogLineN(logKnownFiles,
		"Shared-dir watcher: triggering Reload after debounce");
	m_parent->Reload();
}
