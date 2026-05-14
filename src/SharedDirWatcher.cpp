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

#ifdef __APPLE__
#include <CoreFoundation/CFRunLoop.h>
#endif

#include <common/Format.h>		// Needed for CFormat

#include <common/Path.h>
#include <common/FileFunctions.h>		// CDirIterator for cold-discover walk

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

// Timer IDs are wx-local; use ones not clashing with the prefs dialog.
static const int ID_FSWATCHER_DEBOUNCE   = wxID_HIGHEST + 8231;
#ifdef __APPLE__
static const int ID_FSWATCHER_MAC_PUMP   = wxID_HIGHEST + 8232;

// CFRunLoop pump cadence on macOS amuled. 200 ms keeps user-perceived
// latency below the 5 s debounce window's resolution while costing only
// ~5 non-blocking wakeups/second on the main thread.
static constexpr int kMacPumpMs = 200;
#endif

BEGIN_EVENT_TABLE(CSharedDirWatcher, wxEvtHandler)
	EVT_FSWATCHER(wxID_ANY, CSharedDirWatcher::OnFileSystemEvent)
	EVT_TIMER(ID_FSWATCHER_DEBOUNCE, CSharedDirWatcher::OnDebounceTimer)
#ifdef __APPLE__
	EVT_TIMER(ID_FSWATCHER_MAC_PUMP, CSharedDirWatcher::OnMacRunLoopPump)
#endif
END_EVENT_TABLE()


CSharedDirWatcher::CSharedDirWatcher(CSharedFileList * parent) :
	m_parent(parent),
	m_watcher(NULL),
	m_debounceTimer(this, ID_FSWATCHER_DEBOUNCE)
#ifdef __APPLE__
	, m_macPumpTimer(this, ID_FSWATCHER_MAC_PUMP)
#endif
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
	// First-enable also needs the cold-discovery pass; otherwise
	// subdirs that grew while the daemon was offline are never seen
	// until Reload() fires Refresh() (which happens only when
	// shareddir_list itself changes -- not at startup).
	ColdDiscoverSubdirs();

#ifdef __APPLE__
	// wx's FSEvents wrapper schedules its stream on the thread's
	// CFRunLoop at AddTree() time. aMule.app has a real Cocoa event
	// loop that pumps that runloop, so callbacks deliver normally.
	// amuled (wxAppConsole) does not — its event loop never spins
	// CFRunLoop, so events would queue forever. Start a periodic
	// non-blocking pump only in that case.
	if (wxTheApp && !wxTheApp->IsGUI()) {
		m_macPumpTimer.Start(kMacPumpMs, wxTIMER_CONTINUOUS);
	}
#endif

	AddDebugLogLineN(logKnownFiles, "Shared-dir watcher enabled");
}


void CSharedDirWatcher::Disable()
{
	if (m_debounceTimer.IsRunning()) {
		m_debounceTimer.Stop();
	}
#ifdef __APPLE__
	if (m_macPumpTimer.IsRunning()) {
		m_macPumpTimer.Stop();
	}
#endif
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
	// Cold-discovery is one-shot at Enable() only. Re-running it from
	// Refresh() risks adding duplicate entries when a runtime
	// RegisterNewSubdirectory wrote a path in one canonical form
	// (e.g. macOS-resolved "/private/tmp/...") while the disk walk
	// later produces the unresolved form ("/tmp/...") -- both go in
	// as distinct strings. Startup is the only moment we genuinely
	// have to walk to catch up; runtime new-dirs are covered by
	// RegisterNewSubdirectory from OnFileSystemEvent.
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

	// Only auto-add when an ancestor is in the user's recursive set
	// (shareddir-recursive.dat). Non-recursive shares are now strict
	// -- a new subdir under /Music does NOT get auto-shared unless
	// /Music (or some ancestor of it) was marked recursive via the
	// UI. This protects desktop users with sensitive nested folders
	// from silent recursion.
	if (!theApp->glob_prefs->IsRecursiveAncestor(p)) {
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


void CSharedDirWatcher::ColdDiscoverSubdirs()
{
	if (!m_watcher) {
		return;
	}

	thePrefs::PathList & shared = theApp->glob_prefs->shareddir_list;
	if (shared.empty()) {
		return;
	}

	// Walk only the user's recursive roots, not every shared dir.
	// Non-recursive (explicit) shares are strict: their pre-existing
	// subdirs do NOT get auto-included. Same policy as the HOT path
	// (RegisterNewSubdirectory) -- both auto-add behaviours gate on
	// IsRecursiveAncestor.
	const thePrefs::PathList & roots =
		theApp->glob_prefs->shareddir_recursive_list;
	if (roots.empty()) {
		return;
	}

	// Membership index over the current list: avoids O(N*M) string
	// comparisons when M (newly-discovered subdirs) is large. Keys are
	// raw path strings -- matches the comparison RegisterNewSubdirectory
	// uses for its own dedup check, just lifted into a set.
	// std::set rather than unordered_set: wxString has operator< but
	// no stdlib std::hash specialization, and N here is at most a
	// few thousand even on heavy users -- log-N membership is fine.
	std::set<wxString> known;
	for (size_t i = 0; i < shared.size(); ++i) {
		known.insert(shared[i].GetRaw());
	}

	// Discovered new subdirs go here; we add them in bulk after the
	// walk so a single SaveSharedFolders() flushes the whole batch
	// rather than rewriting shareddir.dat once per discovery.
	std::vector<CPath> discovered;

	// Walk each recursive root's subtree. CSharedFileList's Reload
	// already handled the root itself; this surfaces previously-
	// uncovered descendants only.
	for (size_t i = 0; i < roots.size(); ++i) {
		const CPath & root = roots[i];
		if (!root.IsOk() || !root.DirExists()) {
			continue;
		}
		WalkForUnknownSubdirs(root, known, discovered);
	}

	if (discovered.empty()) {
		return;
	}

	for (size_t i = 0; i < discovered.size(); ++i) {
		const CPath & sub = discovered[i];
		shared.push_back(sub);
#ifndef __WXOSX__
		// On Linux/BSD/Windows the inotify/kqueue/RDCW backends need
		// each subdir registered explicitly. macOS's FSEvents stream
		// from the enclosing AddTree() already covers descendants.
		wxFileName fn = wxFileName::DirName(sub.GetRaw());
		if (!m_watcher->Add(fn, kWatchMask)) {
			AddDebugLogLineC(logKnownFiles,
				CFormat("Shared-dir watcher: failed to add cold-discovered %s")
					% sub.GetRaw());
		}
#endif
		AddDebugLogLineN(logKnownFiles,
			CFormat("Shared-dir watcher: cold-discovered subdir %s")
				% sub.GetRaw());
	}

	AddDebugLogLineN(logKnownFiles,
		CFormat("Shared-dir watcher: cold discovery added %u subdir(s)")
			% (unsigned)discovered.size());

	// Single rewrite of shareddir.dat for the whole batch.
	theApp->glob_prefs->SaveSharedFolders();

	// The initial share-scan already ran (amule.cpp invokes Reload
	// before EnableDirectoryWatcher), so the in-memory shared file
	// list doesn't yet reflect the newly-discovered subdirs. Match
	// the HOT path's behaviour and schedule a debounced Reload to
	// pick up the files under each new subdir without blocking
	// Enable().
	ScheduleReload();
}


void CSharedDirWatcher::WalkForUnknownSubdirs(
	const CPath & root,
	std::set<wxString> & known,
	std::vector<CPath> & out)
{
	CDirIterator dir(root);
	for (CPath sub = dir.GetFirstFile(CDirIterator::Dir);
		sub.IsOk();
		sub = dir.GetNextFile())
	{
		CPath full = root.JoinPaths(sub);
		const wxString key = full.GetRaw();
		if (known.find(key) == known.end()) {
			known.insert(key);
			out.push_back(full);
		}
		// Always recurse: known subdirs may themselves contain
		// unknown grandchildren, and we want a single Refresh() to
		// pick up the whole offline-created tree, not just the top
		// layer of new dirs.
		WalkForUnknownSubdirs(full, known, out);
	}
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


#ifdef __APPLE__
void CSharedDirWatcher::OnMacRunLoopPump(wxTimerEvent & WXUNUSED(event))
{
	// Non-blocking drain (returnAfterSourceHandled=true, timeout=0):
	// dispatches any FSEvents callbacks queued on this thread's
	// CFRunLoop since the last tick, then returns immediately. wx's
	// FSEvents wrapper translates those callbacks into wx events and
	// posts them to our OnFileSystemEvent via the normal event queue.
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
}
#endif
