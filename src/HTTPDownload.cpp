//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Timo Kujala ( tiku@users.sourceforge.net )
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


#include <wx/filename.h>
#include <wx/webrequest.h>

#include "HTTPDownload.h"				// Interface declarations
#include <common/StringFunctions.h>		// Needed for unicode2char
#include "OtherFunctions.h"				// Needed for CastChild
#include "Logger.h"						// Needed for AddLogLine*
#include <common/Format.h>				// Needed for CFormat
#include "InternalEvents.h"				// Needed for CMuleInternalEvent
#include "Preferences.h"
#include "Proxy.h"


#ifndef AMULE_DAEMON
#include "inetdownload.h"	// Needed for inetDownload
#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
#include "MuleGifCtrl.h"

typedef wxGauge wxGaugeControl;

DECLARE_LOCAL_EVENT_TYPE(wxEVT_HTTP_PROGRESS, wxANY_ID)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_HTTP_SHUTDOWN, wxANY_ID)


class CHTTPDownloadDialog : public wxDialog
{
public:
	CHTTPDownloadDialog(CHTTPDownloadThread* owner)
	  : wxDialog(wxTheApp->GetTopWindow(), -1, _("Downloading..."),
			wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSYSTEM_MENU)
	{
		downloadDlg(this, true)->Show(this, true);

		m_progressbar = CastChild(ID_HTTPDOWNLOADPROGRESS, wxGaugeControl);
		m_progressbar->SetRange(100);

		m_ani = CastChild(ID_ANIMATE, MuleGifCtrl);
		m_ani->LoadData((const char*)inetDownload, sizeof(inetDownload));
		m_ani->Start();

		m_owner = owner;
	}

	~CHTTPDownloadDialog() {
		StopDownload();
	}

	void UpdateGauge(int total, int current) {
		CFormat label( "( %s / %s )" );

		const int safeCurrent = (current > 0) ? current : 0;
		const int safeTotal   = (total   > 0) ? total   : 0;

		label % CastItoXBytes(safeCurrent);
		if (safeTotal > 0) {
			label % CastItoXBytes(safeTotal);
		} else {
			label % _("Unknown");
		}

		CastChild(IDC_DOWNLOADSIZE, wxStaticText)->SetLabel(label.GetString());

		// Only touch the gauge when we know the total. Without a known total
		// we leave the gauge at its previous (valid) state — better than
		// risking m_gaugePos > m_rangeMax, which trips an assertion in
		// wxGauge::DoSetGauge on wxGTK (./src/gtk/gauge.cpp:90).
		if (safeTotal > 0) {
			if (safeTotal != m_progressbar->GetRange()) {
				m_progressbar->SetRange(safeTotal);
			}
			const int clamped = (safeCurrent <= safeTotal) ? safeCurrent : safeTotal;
			m_progressbar->SetValue(clamped);
		}

		Layout();
	}

private:
	// Unlink from the request owner and cancel it. Fire-and-forget: the
	// owner will self-destroy when wxWebRequest reports State_Cancelled
	// on the main loop. We must NOT delete the owner here — doing so
	// while a request is in flight leaves the wxWebRequest backend
	// calling into a dead sink.
	void StopDownload() {
		if (m_owner) {
			m_owner->DetachCompanion();
			m_owner->Stop();
			m_owner = NULL;
		}
	}

	void OnBtnCancel(wxCommandEvent& WXUNUSED(evt)) {
		AddLogLineN(_("HTTP download cancelled"));
		Show(false);
		StopDownload();
	}

	void OnProgress(CMuleInternalEvent& evt) {
		UpdateGauge(evt.GetExtraLong(), evt.GetInt());
	}

	void OnShutdown(CMuleInternalEvent& WXUNUSED(evt)) {
		// The thread is about to self-destroy — drop our raw pointer now
		// so our own dtor (which runs later via wxPendingDelete) does not
		// touch a freed CHTTPDownloadThread via StopDownload().
		m_owner = NULL;
		Show(false);
		Destroy();
	}

	CHTTPDownloadThread*	m_owner;	// not owned
	MuleGifCtrl*		m_ani;
	wxGaugeControl*		m_progressbar;

	wxDECLARE_EVENT_TABLE();
};


wxBEGIN_EVENT_TABLE(CHTTPDownloadDialog, wxDialog)
	EVT_BUTTON(ID_HTTPCANCEL, CHTTPDownloadDialog::OnBtnCancel)
	EVT_MULE_INTERNAL(wxEVT_HTTP_PROGRESS, -1, CHTTPDownloadDialog::OnProgress)
	EVT_MULE_INTERNAL(wxEVT_HTTP_SHUTDOWN, -1, CHTTPDownloadDialog::OnShutdown)
wxEND_EVENT_TABLE()

DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_PROGRESS)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_SHUTDOWN)

#endif


// Apply the current proxy prefs to the shared wxWebSession.
//
// wxWebProxy / wxWebSession::SetProxy are wx 3.3+ only. On wx 3.2 there is
// no programmatic way to set a proxy on wxWebRequest, so we rely on the
// backend defaults: on Linux wxWebRequest is backed by libcurl, which
// honours the standard http_proxy / https_proxy / all_proxy environment
// variables. Users of the aMule Proxy pref on wx 3.2 are no worse off than
// with the legacy wxHTTP::SetProxyMode(bool) path, which only toggled a
// boolean and never actually consumed the host / port / auth fields either.
//
// SOCKS proxies are intentionally skipped even when SetProxy is available:
// wx 3.3's wxWebProxy is HTTP-only. A libcurl backend would be required if
// SOCKS support mattered — that is out of scope for this fix.
static void ApplyProxyToDefaultSession()
{
#if wxCHECK_VERSION(3, 3, 0)
	wxWebSession& session = wxWebSession::GetDefault();
	const CProxyData* pd = thePrefs::GetProxyData();
	if (!pd || !pd->m_proxyEnable || pd->m_proxyType == PROXY_NONE) {
		session.SetProxy(wxWebProxy::FromURL(wxString()));   // clear
		return;
	}
	if (pd->m_proxyType != PROXY_HTTP) {
		AddDebugLogLineN(logHTTP, "wxWebRequest: SOCKS proxies are not supported; startup HTTP will be made direct.");
		session.SetProxy(wxWebProxy::FromURL(wxString()));
		return;
	}
	wxString url;
	if (pd->m_enablePassword && !pd->m_userName.IsEmpty()) {
		url = CFormat("http://%s:%s@%s:%u")
			% pd->m_userName % pd->m_password
			% pd->m_proxyHostName % pd->m_proxyPort;
	} else {
		url = CFormat("http://%s:%u")
			% pd->m_proxyHostName % pd->m_proxyPort;
	}
	session.SetProxy(wxWebProxy::FromURL(url));
#endif
}


CHTTPDownloadThread::CHTTPDownloadThread(const wxString& url, const wxString& filename, const wxString& oldfilename, HTTP_Download_File file_id,
										bool showDialog, bool checkDownloadNewer)
	: m_url(url),
	  m_tempfile(filename),
	  m_result(-1),
	  m_response(0),
	  m_error(0),
	  m_file_id(file_id),
	  m_companion(NULL),
	  m_finishPosted(false)
{
	if (showDialog) {
#ifndef AMULE_DAEMON
		CHTTPDownloadDialog* dialog = new CHTTPDownloadDialog(this);
		dialog->Show(true);
		m_companion = dialog;
#endif
	}

	// Conditional GET: only download if the local copy's mtime predates
	// the server version. Same contract as the old wxHTTP path.
	if (checkDownloadNewer && thePrefs::GetLastHTTPDownloadURL(file_id) == url) {
		wxFileName origFile(oldfilename);
		if (origFile.FileExists()) {
			AddDebugLogLineN(logHTTP, CFormat("URL %s matches and file %s exists, only download if newer") % url % oldfilename);
			m_lastmodified = origFile.GetModificationTime();
		}
	}

	{
		wxMutexLocker lock(s_allThreadsMutex);
		s_allThreads.insert(this);
	}

	AddDebugLogLineN(logHTTP, CFormat("HTTP download started: %s") % m_url);

	if (m_url.IsEmpty()) {
		AddLogLineC(_("The URL to download can't be empty"));
		FinishAndDestroy(HTTP_Error);
		return;
	}

	ApplyProxyToDefaultSession();

	m_request = wxWebSession::GetDefault().CreateRequest(this, m_url);
	if (!m_request.IsOk()) {
		AddLogLineC(CFormat(_("Failed to create HTTP request for %s")) % m_url);
		FinishAndDestroy(HTTP_Error);
		return;
	}

	// Storage_File: wx streams the response body to an internal temp file
	// and hands us the path on completion. We then rename it to the
	// caller-supplied m_tempfile. Redirects (incl. HTTP→HTTPS) are
	// followed by wx transparently — the whole recursive GetInputStream()
	// redirect handler of the legacy code path is gone, which is the
	// actual fix for the upstream startup crash (amule-project/amule#455).
	m_request.SetStorage(wxWebRequest::Storage_File);

	if (m_lastmodified.IsValid()) {
		AddDebugLogLineN(logHTTP, "If-Modified-Since: " + FormatDateHTTP(m_lastmodified));
		m_request.SetHeader("If-Modified-Since", FormatDateHTTP(m_lastmodified));
	}

	Bind(wxEVT_WEBREQUEST_STATE, &CHTTPDownloadThread::OnStateEvent, this);
	m_request.Start();
}


// Format the given date to a RFC-2616 compliant HTTP date.
// Example: Thu, 14 Jan 2010 15:40:23 GMT
wxString CHTTPDownloadThread::FormatDateHTTP(const wxDateTime& date)
{
	static const wxChar* s_months[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
	static const wxChar* s_dow[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };

	return CFormat("%s, %02d %s %d %02d:%02d:%02d GMT") % s_dow[date.GetWeekDay(wxDateTime::UTC)] % date.GetDay(wxDateTime::UTC) % s_months[date.GetMonth(wxDateTime::UTC)] % date.GetYear(wxDateTime::UTC) % date.GetHour(wxDateTime::UTC) % date.GetMinute(wxDateTime::UTC) % date.GetSecond(wxDateTime::UTC);
}


void CHTTPDownloadThread::OnStateEvent(wxWebRequestEvent& evt)
{
	switch (evt.GetState()) {
		case wxWebRequest::State_Active: {
			// Periodic progress notification during the download.
			if (m_companion) {
#ifndef AMULE_DAEMON
				// GetBytesExpectedToReceive() returns wxInvalidOffset (-1)
				// when the server omits Content-Length. Forwarding -1 into
				// the dialog ends up in wxGauge::SetRange(-1), which flips
				// m_rangeMax invalid and trips an assertion on the next
				// repaint (./src/gtk/gauge.cpp:90). Clamp to 0 here so the
				// dialog treats it as "unknown" (skips the range + value
				// update) until a real total shows up.
				wxFileOffset expected = m_request.GetBytesExpectedToReceive();
				CMuleInternalEvent prog(wxEVT_HTTP_PROGRESS);
				prog.SetInt((int)m_request.GetBytesReceived());
				prog.SetExtraLong((long)(expected > 0 ? expected : 0));
				wxQueueEvent(m_companion, (prog).Clone());
#endif
			}
			break;
		}

		case wxWebRequest::State_Completed: {
			wxWebResponse response = evt.GetResponse();
			m_response = response.IsOk() ? response.GetStatus() : 0;
			m_error    = 0;

			AddDebugLogLineN(logHTTP, CFormat("HTTP response %d for %s") % m_response % m_url);

			if (m_response == 304) {
				// Not Modified — nothing to write.
				AddDebugLogLineN(logHTTP, "Skipped download because requested file is not newer.");
				FinishAndDestroy(HTTP_Skipped);
			} else if (m_response >= 200 && m_response < 300) {
				// Success. wx wrote the body to its own temp file; move it
				// to the caller-supplied m_tempfile. A plain rename may
				// fail across filesystems (wx's temp dir vs the aMule
				// config dir), so fall back to copy + delete.
				const wxString wxTmp = response.GetDataFile();
				if (wxTmp.IsEmpty() || !wxFileExists(wxTmp)) {
					AddLogLineC(CFormat(_("HTTP download: empty response body for %s")) % m_url);
					FinishAndDestroy(HTTP_Error);
					break;
				}
				if (wxFileExists(m_tempfile)) {
					wxRemoveFile(m_tempfile);
				}
				bool moved = wxRenameFile(wxTmp, m_tempfile);
				if (!moved) {
					moved = wxCopyFile(wxTmp, m_tempfile) && wxRemoveFile(wxTmp);
				}
				if (!moved) {
					AddLogLineC(CFormat(_("Could not move downloaded file to %s")) % m_tempfile);
					FinishAndDestroy(HTTP_Error);
					break;
				}
				AddLogLineN(CFormat(_("Downloaded %s (%llu bytes)"))
					% m_url % (unsigned long long)m_request.GetBytesReceived());
				thePrefs::SetLastHTTPDownloadURL(m_file_id, m_url);
				FinishAndDestroy(HTTP_Success);
			} else {
				AddLogLineC(CFormat(_("The URL %s returned: %i")) % m_url % m_response);
				FinishAndDestroy(HTTP_Error);
			}
			break;
		}

		case wxWebRequest::State_Failed: {
			AddLogLineC(CFormat(_("HTTP download failed for %s: %s"))
				% m_url % evt.GetErrorDescription());
			FinishAndDestroy(HTTP_Error);
			break;
		}

		case wxWebRequest::State_Cancelled: {
			AddDebugLogLineN(logHTTP, CFormat("HTTP download cancelled: %s") % m_url);
			FinishAndDestroy(HTTP_Error);
			break;
		}

		case wxWebRequest::State_Unauthorized:
			// We have no credentials to provide interactively; treat as
			// a failure so the dispatcher reports an error to the user.
			AddLogLineC(CFormat(_("HTTP 401 Unauthorized for %s")) % m_url);
			m_request.Cancel();
			FinishAndDestroy(HTTP_Error);
			break;

		case wxWebRequest::State_Idle:
			// Initial state before Start(); nothing to do.
			break;
	}
}


void CHTTPDownloadThread::FinishAndDestroy(int result)
{
	if (m_finishPosted) {
		return;
	}
	m_finishPosted = true;
	m_result = result;

	// Clean the caller's temp file on failure so we don't leave stale
	// half-written content around. Legacy path did the same.
	if (m_result != HTTP_Success && wxFileExists(m_tempfile)) {
		wxRemoveFile(m_tempfile);
	}

#ifndef AMULE_DAEMON
	if (m_companion) {
		CMuleInternalEvent termEvent(wxEVT_HTTP_SHUTDOWN);
		wxQueueEvent(m_companion, (termEvent).Clone());
	}
#endif

	// Notify the app dispatcher (CamuleApp::OnFinishedHTTPDownload) so the
	// feature-specific handler (ipfilter, serverlist, kad, ...) runs.
	CMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)m_file_id);
	evt.SetExtraLong((long)m_result);
	wxQueueEvent(wxTheApp, (evt).Clone());

	{
		wxMutexLocker lock(s_allThreadsMutex);
		s_allThreads.erase(this);
	}

	AddDebugLogLineN(logHTTP, "HTTP download ended");

	// Schedule our own destruction after the current event returns to
	// the main loop. Must not be `delete this` — wxWebRequest may still
	// be unwinding state after handing us the terminal event.
	CallAfter([this]{ delete this; });
}


void CHTTPDownloadThread::DetachCompanion()
{
	m_companion = NULL;
}


void CHTTPDownloadThread::Stop()
{
	if (m_request.IsOk() && !m_finishPosted) {
		// Fire-and-forget: wxWebRequest will schedule a State_Cancelled
		// event on the main loop. Our OnStateEvent will run
		// FinishAndDestroy at that point.
		m_request.Cancel();
	} else if (!m_finishPosted) {
		// Request never got off the ground (e.g. invalid URL, or Stop()
		// called before Start() had a chance). Finish synchronously.
		FinishAndDestroy(HTTP_Error);
	}
}


void CHTTPDownloadThread::StopAll()
{
	ThreadSet snapshot;
	{
		wxMutexLocker lock(s_allThreadsMutex);
		snapshot = s_allThreads;
	}
	for (ThreadSet::iterator it = snapshot.begin(); it != snapshot.end(); ++it) {
		(*it)->Stop();
	}
}


CHTTPDownloadThread::ThreadSet CHTTPDownloadThread::s_allThreads;
wxMutex CHTTPDownloadThread::s_allThreadsMutex;

// File_checked_for_headers
