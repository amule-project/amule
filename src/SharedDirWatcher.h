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

#ifndef SHAREDDIRWATCHER_H
#define SHAREDDIRWATCHER_H

#include <wx/event.h>
#include <wx/timer.h>
#include <wx/fswatcher.h>

#include <set>
#include <unordered_map>
#include <vector>

#include "Types.h"

class CSharedFileList;
class CPath;

// Watches every directory in CPreferences::shareddir_list for file/dir
// creation, deletion, rename and modification, and triggers a debounced
// CSharedFileList::Reload() so newly-added files become shared without a
// manual "Reload shared files" click.
//
// New subdirectories created under any watched path are auto-added to
// shareddir_list so the watcher keeps following them, which mirrors the
// behaviour users expect from the existing "recursive share" button —
// once a parent is shared, anything created beneath it is shared too.
// We do not persist a per-entry recursive flag because shareddir.dat
// already enumerates each subdirectory individually.
//
// Backends per platform are provided by wxFileSystemWatcher: inotify on
// Linux, FSEvents on macOS, ReadDirectoryChangesW on Windows, kqueue on
// BSD. AddTree() handles initial recursive setup; we re-Add new subdirs
// individually as we discover them so the watch graph keeps pace with
// disk changes.
class CSharedDirWatcher : public wxEvtHandler
{
public:
	explicit CSharedDirWatcher(CSharedFileList * parent);
	~CSharedDirWatcher();

	// Start watching every path in shareddir_list. Safe to call when
	// already enabled — no-op in that case.
	void Enable();

	// Stop watching and free the watcher object. Safe to call when
	// already disabled.
	void Disable();

	bool IsEnabled() const { return m_watcher != NULL; }

	// Called from CSharedFileList::Reload() after the list has been
	// rebuilt. Re-walks shareddir_list and updates the watcher's path
	// set so removed dirs stop firing events and newly-added dirs
	// start firing.
	void Refresh();

	// Event-type flags accumulated for one pending path between
	// fs-watcher delivery and debounce-flush. Stored as ints so we
	// don't have to expose wxFSW_EVENT_* in the header to includers.
	struct PendingPathEvents {
		int flags;            // bitmask of wxFSW_EVENT_CREATE/DELETE/MODIFY
		wxString renamedTo;   // populated for RENAME; empty otherwise
	};

private:
	void OnFileSystemEvent(wxFileSystemWatcherEvent & event);
	void OnDebounceTimer(wxTimerEvent & event);
#ifdef __APPLE__
	// macOS amuled (wxAppConsole) doesn't spin the main thread's
	// CFRunLoop, so FSEvents callbacks scheduled by wx never deliver.
	// A periodic non-blocking drain fixes that without spawning a
	// dedicated thread. No-op under aMule.app whose Cocoa main loop
	// already pumps the runloop.
	void OnMacRunLoopPump(wxTimerEvent & event);
#endif

	// Walk shareddir_list and Add() every path. Errors per-path are
	// logged but do not abort the rest (so a single Linux
	// max_user_watches refusal doesn't blank the whole watcher).
	void RegisterAllPaths();

	// Coalesce a burst of FS events into per-path deltas applied on
	// the debounce-timer flush. Resets the 5-second timer on every
	// new event; processing runs when the timer finally fires.
	void ScheduleProcessing();

	// Walk m_pendingEvents and apply each one to CSharedFileList via
	// NotifyPathAdded/Removed/Modified. Skipped if m_fallbackPending
	// was set by a wxFSW_EVENT_WARNING / _ERROR event since the last
	// flush -- in that case we fall back to the bulk Reload() because
	// the watcher backend has signalled it dropped events and we
	// can't trust our incremental view of the tree any more.
	void FlushPendingEvents();

	// Append a newly-created directory to shareddir_list (if not
	// already there) and start watching it. Used for Option A's
	// "auto-share new subdirs of watched parents" behaviour.
	void RegisterNewSubdirectory(const wxString & path);

	// Recursively walk each path in shareddir_list and add any subdir
	// not already listed. The "cold" twin of RegisterNewSubdirectory:
	// without this, subdirs created while aMule was offline are never
	// observed -- the watcher only fires CREATE events post-Enable(),
	// so a /Music share whose disk grew three new albums in the
	// interim would never see them until each gained a new file via
	// some other path. Batches in-memory and writes shareddir.dat
	// once at the end so a large discovery pass doesn't trigger N
	// rewrites.
	void ColdDiscoverSubdirs();

	// Recursive walker used by ColdDiscoverSubdirs. Visits every
	// subdirectory of `root`; for each that is not already in
	// `known` it inserts the path into `known` and appends to `out`.
	// Always recurses (even through already-known subdirs) so a tree
	// whose top layer is in shareddir.dat but whose deeper layers
	// are not still gets fully covered in one pass.
	void WalkForUnknownSubdirs(
		const CPath & root,
		std::set<wxString> & known,
		std::vector<CPath> & out);

	CSharedFileList *      m_parent;
	wxFileSystemWatcher *  m_watcher;
	wxTimer                m_debounceTimer;

	// Per-path accumulator. Keyed on the raw filesystem path. Events
	// coalesce per-path: a CREATE followed by N MODIFYs followed by
	// CLOSE_WRITE on the same file becomes a single dispatch on the
	// debounce flush. RENAME stores the destination in `renamedTo`.
	std::unordered_map<wxString, PendingPathEvents>  m_pendingEvents;

	// Set when the watcher backend reports overflow / drop (inotify
	// queue overflow, kqueue race, Windows ReadDirectoryChangesW
	// buffer exhaust). FlushPendingEvents() responds by calling the
	// bulk Reload() because incremental state can no longer be
	// trusted. Cleared after the fallback fires.
	bool                   m_fallbackPending;
#ifdef __APPLE__
	wxTimer                m_macPumpTimer;
#endif

	DECLARE_EVENT_TABLE()
};

#endif // SHAREDDIRWATCHER_H
