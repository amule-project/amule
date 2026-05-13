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

#ifndef SHAREDDIRSAPPLYTASK_H
#define SHAREDDIRSAPPLYTASK_H

#include <atomic>
#include <vector>

#include <wx/event.h>
#include <wx/thread.h>

#include <common/Path.h>

// Worker thread that flattens the user's pending share intent
// (explicit-shared paths + recursive-share roots) into a single flat
// path list ready to drop into CPreferences::shareddir_list. The
// recursive expansion is the historically-long-running step that used
// to freeze the Directories preferences panel for minutes when a user
// right-clicked a deep root like /home — moving it off the UI thread
// lets the user see a progress bar and cancel cleanly.
//
// Thread-safety contract:
//   * The constructor copies its inputs by value into thread-private
//     storage; the caller can drop its own copies immediately.
//   * The caller may invoke Cancel() (which calls wxThread::Delete())
//     from the UI thread at any time. The thread checks via
//     TestDestroy() between directory enumerations and exits with
//     m_cancelled=true.
//   * Progress events are posted to the owner via wxQueueEvent, so the
//     UI thread receives them in its normal event loop.
//   * Results are only safe to read after the wxEVT_SHARED_DIRS_APPLY_DONE
//     event arrives, at which point the worker thread is guaranteed
//     to be finished.
class CSharedDirsApplyTask : public wxThread
{
public:
	typedef std::vector<CPath> PathList;

	CSharedDirsApplyTask(const PathList & explicit_shares,
		const PathList & recursive_shares,
		wxEvtHandler * owner);

	// Request graceful termination from the UI thread. Returns once
	// the worker has acknowledged the cancel and exited.
	void Cancel();

	// Read-after-done accessors. Calling these while the thread is
	// still running is undefined.
	const PathList & GetExpandedShares() const { return m_output; }
	bool             WasCancelled() const      { return m_cancelled.load(); }
	size_t           GetTotalScanned() const   { return m_scanned.load(); }

protected:
	ExitCode Entry() override;

private:
	void ExpandRecursive(const CPath & root);
	void PostProgress();

	PathList            m_explicit;     // copy of the caller's explicit shares
	PathList            m_recursive;    // copy of the caller's recursive intents
	wxEvtHandler *      m_owner;
	PathList            m_output;       // flat path list, filled during Entry()

	std::atomic<size_t> m_scanned{0};   // directories visited so far
	std::atomic<bool>   m_cancelled{false};
	size_t              m_lastReportedScanned = 0;
};

// Events posted from the worker thread to the owner. The owner is
// expected to be a wxEvtHandler subclass that binds these via
// Bind(wxEVT_SHARED_DIRS_APPLY_PROGRESS, ...).
wxDECLARE_EVENT(wxEVT_SHARED_DIRS_APPLY_PROGRESS, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SHARED_DIRS_APPLY_DONE,     wxThreadEvent);

#endif // SHAREDDIRSAPPLYTASK_H
