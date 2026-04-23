//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Timo Kujala ( tiku@users.sourceforge.net )
// Copyright (c) 2002-2011 Patrizio Bassi ( hetfield@amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
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

#ifndef HTTPDOWNLOAD_H
#define HTTPDOWNLOAD_H

#include "GuiEvents.h"		// Needed for HTTP_Download_File
#include <wx/datetime.h>	// Needed for wxDateTime
#include <wx/event.h>		// Needed for wxEvtHandler
#include <wx/webrequest.h>	// Needed for wxWebRequest
#include <set>

class wxWebRequestEvent;


enum HTTPDownloadResult {
	HTTP_Success = 0,
	HTTP_Error,
	HTTP_Skipped
};


//
// Startup HTTP downloader (version check, server.met, nodes.dat, ipfilter,
// GeoIP). Historically a wxThread running wxHTTP on a worker thread, which
// raced wxEpollDispatcher during redirect / teardown (see upstream PR #455).
//
// This implementation is event-driven on the main thread: owns a
// wxWebRequest, binds wxEVT_WEBREQUEST_STATE, and self-destroys (via
// wxPendingDelete) after Completed / Failed / Cancelled. No thread, no
// nested event loops, no modal dialogs — so no reentrancy surface.
//
// The legacy Create() / Run() / Stop() / Entry() / OnExit() names are kept
// as thin stubs so the six existing call sites (new + Create + Run) do not
// need to change. Once wx 3.3's wxWebRequestSync is a reasonable floor on
// distro defaults, this whole class could be replaced with a synchronous
// call on a worker thread with no wxSocket involvement — at that point the
// stubs can be made real and this header collapsed.
//
class CHTTPDownloadThread : public wxEvtHandler
{
public:
	/** Note: wxChar* was historically used here to dodge thread-unsafe
	 *  wxString refcounting; no longer strictly required on the main
	 *  thread, but kept for API compatibility. */
	CHTTPDownloadThread(const wxString& url, const wxString& filename, const wxString& oldfilename, HTTP_Download_File file_id,
						bool showDialog, bool checkDownloadNewer);

	static void StopAll();

	// Legacy wxThread-style entry points retained as stubs for the six
	// existing call sites. The request is already started from the ctor
	// on the main thread, so Create()/Run() have nothing to do.
	bool	Create()	{ return true; }
	void	Run()		{ /* request started in ctor */ }
	void	Stop();		// fire-and-forget cancel; terminal event follows
	void	OnExit()	{ /* no-op: we self-destroy on terminal state */ }

	// Called by the companion dialog before it tears itself down, so that
	// subsequent terminal-state cleanup does not post events to a dead
	// handler.
	void	DetachCompanion();

private:
	void	OnStateEvent(wxWebRequestEvent& evt);
	void	FinishAndDestroy(int result);

	static wxString FormatDateHTTP(const wxDateTime& date);

	wxWebRequest		m_request;
	wxString		m_url;
	wxString		m_tempfile;
	wxDateTime		m_lastmodified;	//! Date on which the file being updated was last modified.
	int			m_result;
	int			m_response;	//! HTTP response code (e.g. 200)
	int			m_error;	//! Additional error code
	HTTP_Download_File	m_file_id;
	wxEvtHandler*		m_companion;
	bool			m_finishPosted;	//! Guard against double-posting on terminal state

	typedef std::set<CHTTPDownloadThread *>	ThreadSet;
	static ThreadSet	s_allThreads;
	static wxMutex		s_allThreadsMutex;
};

#endif // HTTPDOWNLOAD_H
// File_checked_for_headers
