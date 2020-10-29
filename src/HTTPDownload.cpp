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


#ifdef HAVE_CONFIG_H
#	include "config.h"				// Needed for HAVE_LIBCURL
#endif

#ifdef HAVE_LIBCURL
#	include <curl/curl.h>
#	include <common/ClientVersion.h>		// Needed for the VERSION_ defines
#	include <common/MacrosProgramSpecific.h>	// Needed for GUI_ONLY()
#else
#	include <wx/protocol/http.h>
#endif

#include <wx/wfstream.h>				// Needed for wxFFileOutputStream
#include "HTTPDownload.h"				// Interface declarations
#include <common/StringFunctions.h>		// Needed for unicode2char
#include "OtherFunctions.h"				// Needed for CastChild
#include "Logger.h"						// Needed for AddLogLine*
#include <common/Format.h>				// Needed for CFormat
#include "InternalEvents.h"				// Needed for CMuleInternalEvent
#include "Preferences.h"
#include "ScopedPtr.h"
#include <wx/filename.h>				// Needed for wxFileName


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
	CHTTPDownloadDialog(CHTTPDownloadThread* thread)
	  : wxDialog(wxTheApp->GetTopWindow(), -1, _("Downloading..."),
			wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSYSTEM_MENU)
	{
		downloadDlg(this, true)->Show(this, true);

		m_progressbar = CastChild(ID_HTTPDOWNLOADPROGRESS, wxGaugeControl);
		m_progressbar->SetRange(100);

		m_ani = CastChild(ID_ANIMATE, MuleGifCtrl);
		m_ani->LoadData((const char*)inetDownload, sizeof(inetDownload));
		m_ani->Start();

		m_thread = thread;
	}

	~CHTTPDownloadDialog() {
		StopThread();
	}

	void UpdateGauge(int total, int current) {
		CFormat label( wxT("( %s / %s )") );

		label % CastItoXBytes(current);
		if (total > 0) {
			label % CastItoXBytes(total);
		} else {
			label % _("Unknown");
		}

		CastChild(IDC_DOWNLOADSIZE, wxStaticText)->SetLabel(label.GetString());

		if (total && (total != m_progressbar->GetRange())) {
			m_progressbar->SetRange(total);
		}

		if (current && (current <= total)) {
			m_progressbar->SetValue(current);
		}

		Layout();
	}

private:
	void StopThread() {
		if (m_thread) {
			m_thread->Stop();
			delete m_thread;
			m_thread = NULL;
		}
	}

	void OnBtnCancel(wxCommandEvent& WXUNUSED(evt)) {
		AddLogLineN(_("HTTP download cancelled"));
		Show(false);
		StopThread();
	}

	void OnProgress(CMuleInternalEvent& evt) {
		UpdateGauge(evt.GetExtraLong(), evt.GetInt());
	}

	void OnShutdown(CMuleInternalEvent& WXUNUSED(evt)) {
		Show(false);
		Destroy();
	}

	CMuleThread*	m_thread;
	MuleGifCtrl*	m_ani;
	wxGaugeControl* m_progressbar;

	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(CHTTPDownloadDialog, wxDialog)
	EVT_BUTTON(ID_HTTPCANCEL, CHTTPDownloadDialog::OnBtnCancel)
	EVT_MULE_INTERNAL(wxEVT_HTTP_PROGRESS, -1, CHTTPDownloadDialog::OnProgress)
	EVT_MULE_INTERNAL(wxEVT_HTTP_SHUTDOWN, -1, CHTTPDownloadDialog::OnShutdown)
END_EVENT_TABLE()

DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_PROGRESS)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_SHUTDOWN)

#endif


CHTTPDownloadThread::CHTTPDownloadThread(const wxString& url, const wxString& filename, const wxString& oldfilename, HTTP_Download_File file_id,
										bool showDialog, bool checkDownloadNewer)
#ifdef AMULE_DAEMON
	: CMuleThread(wxTHREAD_DETACHED),
#else
	: CMuleThread(showDialog ? wxTHREAD_JOINABLE : wxTHREAD_DETACHED),
#endif
	  m_url(url),
	  m_tempfile(filename),
	  m_result(-1),
	  m_file_id(file_id),
	  m_companion(NULL)
{
	if (showDialog) {
#ifndef AMULE_DAEMON
		CHTTPDownloadDialog* dialog = new CHTTPDownloadDialog(this);
		dialog->Show(true);
		m_companion = dialog;
#endif
	}
	// Get the date on which the original file was last modified
	// Only if it's the same URL we used for the last download and if the file exists.
	if (checkDownloadNewer && thePrefs::GetLastHTTPDownloadURL(file_id) == url) {
		wxFileName origFile = wxFileName(oldfilename);
		if (origFile.FileExists()) {
			AddDebugLogLineN(logHTTP, CFormat(wxT("URL %s matches and file %s exists, only download if newer")) % url % oldfilename);
			m_lastmodified = origFile.GetModificationTime();
		}
	}
	wxMutexLocker lock(s_allThreadsMutex);
	s_allThreads.insert(this);
}


// Format the given date to a RFC-2616 compliant HTTP date
// Example: Thu, 14 Jan 2010 15:40:23 GMT
wxString CHTTPDownloadThread::FormatDateHTTP(const wxDateTime& date)
{
	static const wxChar* s_months[] = { wxT("Jan"), wxT("Feb"), wxT("Mar"), wxT("Apr"), wxT("May"), wxT("Jun"), wxT("Jul"), wxT("Aug"), wxT("Sep"), wxT("Oct"), wxT("Nov"), wxT("Dec") };
	static const wxChar* s_dow[] = { wxT("Sun"), wxT("Mon"), wxT("Tue"), wxT("Wed"), wxT("Thu"), wxT("Fri"), wxT("Sat") };

	return CFormat(wxT("%s, %02d %s %d %02d:%02d:%02d GMT")) % s_dow[date.GetWeekDay(wxDateTime::UTC)] % date.GetDay(wxDateTime::UTC) % s_months[date.GetMonth(wxDateTime::UTC)] % date.GetYear(wxDateTime::UTC) % date.GetHour(wxDateTime::UTC) % date.GetMinute(wxDateTime::UTC) % date.GetSecond(wxDateTime::UTC);
}


#ifdef HAVE_LIBCURL

size_t mule_curl_write_callback(char *ptr, size_t WXUNUSED(size), size_t nmemb, void *userdata)
{
	wxFFileOutputStream *outstream = static_cast<wxFFileOutputStream *>(userdata);

	// According to the documentation size will always be 1.
	outstream->Write(ptr, nmemb);

	return outstream->LastWrite();
}

#ifdef __DEBUG__
int mule_curl_debug_callback(CURL* WXUNUSED(handle), curl_infotype type, char *data, size_t WXUNUSED(size), void* WXUNUSED(userptr))
{
	if (type == CURLINFO_TEXT) {
		AddDebugLogLineN(logHTTP, CFormat(wxT("curl: %s")) % wxString(data));
	}

	return 0;
}
#endif

int mule_curl_xferinfo_callback(void *clientp, curl_off_t GUI_ONLY(dltotal), curl_off_t GUI_ONLY(dlnow), curl_off_t WXUNUSED(ultotal), curl_off_t WXUNUSED(ulnow))
{
	CHTTPDownloadThread *thread = static_cast<CHTTPDownloadThread *>(clientp);

	if (thread->TestDestroy()) {
		// returning nonzero will abort the current transfer
		return 1;
	}

#ifndef AMULE_DAEMON
	if (thread->GetProgressDialog()) {
		CMuleInternalEvent evt(wxEVT_HTTP_PROGRESS);
		evt.SetInt(dlnow);
		evt.SetExtraLong(dltotal);
		wxPostEvent(thread->GetProgressDialog(), evt);
	}
#endif

	return 0;
}

int mule_curl_progress_callback(void *clientp, double dltotal, double dlnow, double WXUNUSED(ultotal), double WXUNUSED(ulnow))
{
	return mule_curl_xferinfo_callback(clientp, (curl_off_t)dltotal, (curl_off_t)dlnow, 0, 0);
}

#endif /* HAVE_LIBCURL */


CMuleThread::ExitCode CHTTPDownloadThread::Entry()
{
	if (TestDestroy()) {
		return NULL;
	}

#ifdef HAVE_LIBCURL
	CURL *curl;
	CURLcode res;
	static unsigned int curl_version = 0;
#else
	wxHTTP* url_handler = NULL;
#endif

	AddDebugLogLineN(logHTTP, wxT("HTTP download thread started"));

	const CProxyData* proxy_data = thePrefs::GetProxyData();
	bool use_proxy = proxy_data != NULL && proxy_data->m_proxyEnable;

	try {
		wxFFileOutputStream outfile(m_tempfile);

		if (!outfile.Ok()) {
			throw wxString(CFormat(_("Unable to create destination file %s for download!")) % m_tempfile);
		}

		if (m_url.IsEmpty()) {
			// Nowhere to download from!
			throw wxString(_("The URL to download can't be empty"));
		}

#ifdef HAVE_LIBCURL

		if (curl_version == 0) {
			curl_version = curl_version_info(CURLVERSION_NOW)->version_num;
		}

		curl = curl_easy_init();
		if (curl) {
			struct curl_slist *list = NULL;

			curl_easy_setopt(curl, CURLOPT_URL, (const char *)unicode2char(m_url));

			// follow redirects
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

			// set write callback
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mule_curl_write_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outfile);

#ifdef __DEBUG__
			// send libcurl verbose messages to aMule debug log
			curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, mule_curl_debug_callback);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

			// show download progress
			// This callback is used to cancel an ongoing transfer.
#if LIBCURL_VERSION_NUM >= 0x072000
			// CURLOPT_XFERINFOFUNCTION was introduced in 7.32.0.
			if (curl_version >= 0x072000) {
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, mule_curl_xferinfo_callback);
				curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
			} else
#endif
			{
				// CURLOPT_PROGRESSFUNCTION is deprecated in favour of CURLOPT_XFERINFOFUNCTION
				curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, mule_curl_progress_callback);
				curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
			}
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

			// Build a conditional get request if the last modified date of the file being updated is known
			if (m_lastmodified.IsValid()) {
				// Set a flag in the HTTP header that we only download if the file is newer.
				// see: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.25
				AddDebugLogLineN(logHTTP, wxT("If-Modified-Since: ") + FormatDateHTTP(m_lastmodified));
				list = curl_slist_append(list, (const char *)unicode2char(wxT("If-Modified-Since: ") + FormatDateHTTP(m_lastmodified)));
			}

			// set custom header(s)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

			// some servers don't like requests without a user-agent, so set one
			// always use a numerical version value, that's why we don't use VERSION
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "aMule/" wxSTRINGIZE(VERSION_MJR) "." wxSTRINGIZE(VERSION_MIN) "." wxSTRINGIZE(VERSION_UPDATE));

			// set the outgoing address
			if (!thePrefs::GetAddress().empty()) {
				curl_easy_setopt(curl, CURLOPT_INTERFACE, (const char *)unicode2char(thePrefs::GetAddress()));
			}

			// proxy
			if (use_proxy) {
				switch (proxy_data->m_proxyType) {
					case PROXY_SOCKS5:
						curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
#if LIBCURL_VERSION_NUM >= 0x073700
						// CURLOPT_SOCKS5_AUTH was added in 7.55.0
						if (proxy_data->m_enablePassword) {
							curl_easy_setopt(curl, CURLOPT_SOCKS5_AUTH, CURLAUTH_BASIC);
						} else {
							curl_easy_setopt(curl, CURLOPT_SOCKS5_AUTH, CURLAUTH_NONE);
						}
#endif
						break;
					case PROXY_SOCKS4:
						curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
						break;
					case PROXY_HTTP:
						curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
						// Using only "Basic" authentication as the rest of the code
						// (in Proxy.cpp) supports only that
						curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
						// Not tunneling through the proxy on purpose, let it cache
						// HTTP traffic if possible (this is the default behaviour).
						//curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 0L);
						break;
					case PROXY_SOCKS4a:
						curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4A);
						break;
					default:
						goto noProxy;
				}
				curl_easy_setopt(curl, CURLOPT_PROXY, (const char *)unicode2char(proxy_data->m_proxyHostName));
				curl_easy_setopt(curl, CURLOPT_PROXYPORT, proxy_data->m_proxyPort);
				if (proxy_data->m_enablePassword) {
					curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, (const char *)unicode2char(proxy_data->m_userName));
					curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, (const char *)unicode2char(proxy_data->m_password));
				}
			}
noProxy:

			// perform the action
			res = curl_easy_perform(curl);

			// clean up
			curl_slist_free_all(list);

			// check the result
			if (res != CURLE_OK) {
				m_result = HTTP_Error;
				curl_easy_cleanup(curl);
				throw wxString(CFormat(_("HTTP download failed: %s")) % wxString(curl_easy_strerror(res)));
			} else {
				long response_code;
				res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (res != CURLE_OK) {
					// getinfo() failed, but perform() succeeded. Let's assume the transfer was O.K.
					AddDebugLogLineN(logHTTP, wxT("curl_easy_getinfo() failed: ") + wxString(curl_easy_strerror(res)));
					m_result = HTTP_Success;
				} else {
					AddDebugLogLineN(logHTTP, CFormat(wxT("Response code: %d")) % response_code);
					if (response_code == 304) {		// "Not Modified"
						m_result = HTTP_Skipped;
					} else if (response_code == 200) {	// "OK"
						m_result = HTTP_Success;
						/* TRANSLATORS: parameters are 'size transferred', 'URL' and 'download time' */
						CFormat message(_("HTTP: Downloaded %s from '%s' in %s"));

						// get downloaded size
#if LIBCURL_VERSION_NUM >= 0x073700
						/* CURLINFO_SIZE_DOWNLOAD_T was introduced in 7.55.0 */
						/* check the runtime version, too */
						if (curl_version >= 0x073700) {
							curl_off_t dl;
							curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dl);
							message % CastItoXBytes(dl);
						} else
#endif
						{
							double dl;
							curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &dl);
							message % CastItoXBytes((uint64)dl);
						}

						// get effective URL
						{
							char *url = NULL;
							curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
							message % wxString(url);
						}

						// get download time
#if LIBCURL_VERSION_NUM >= 0x073d00
						/* In libcurl 7.61.0, support was added for extracting the time in plain
						   microseconds. Older libcurl versions are stuck in using 'double' for this
						   information. */
						if (curl_version >= 0x073d00) {
							curl_off_t tm;
							curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &tm);
							// CastSecondsToHM() uses milliseconds while we have microseconds now
							message % CastSecondsToHM(tm / 1000000, (tm / 1000) % 1000);
						} else
#endif
						{
							double tm;
							curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &tm);
							message % CastSecondsToHM((uint32)tm, (uint16)(tm * 1000));
						}

						// Summarize transfer details
						AddLogLineN(message);
					} else {
						m_result = HTTP_Error;
					}
				}
			}

			// clean up
			curl_easy_cleanup(curl);
		}

#else /* == not HAVE_LIBCURL */

		url_handler = new wxHTTP;
		url_handler->SetProxyMode(use_proxy);

		// Build a conditional get request if the last modified date of the file being updated is known
		if (m_lastmodified.IsValid()) {
			// Set a flag in the HTTP header that we only download if the file is newer.
			// see: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.25
			AddDebugLogLineN(logHTTP, wxT("If-Modified-Since: ") + FormatDateHTTP(m_lastmodified));
			url_handler->SetHeader(wxT("If-Modified-Since"), FormatDateHTTP(m_lastmodified));
		}

		CScopedPtr<wxInputStream> url_read_stream(GetInputStream(url_handler, m_url, use_proxy));

		if (!url_read_stream.get()) {
			if (m_response == 304) {
				m_result = HTTP_Skipped;
				AddDebugLogLineN(logHTTP, wxT("Skipped download because requested file is not newer."));
				throw wxString(wxEmptyString);
			} else {
				m_result = HTTP_Error;
				throw wxString(CFormat(_("The URL %s returned: %i - Error (%i)!")) % m_url % m_response % m_error);
			}
		}

		int download_size = url_read_stream->GetSize();
#ifdef __DEBUG__
		if (download_size == -1) {
			AddDebugLogLineN(logHTTP, wxT("Download size not received, downloading until connection is closed"));
		} else {
			AddDebugLogLineN(logHTTP, CFormat(wxT("Download size: %i")) % download_size);
		}
#endif

		// Here is our read buffer
		// <ken> Still, I'm sure 4092 is probably a better size.
		// MP: Most people can download at least at 32kb/s from http...
		const unsigned MAX_HTTP_READ = 32768;

		char buffer[MAX_HTTP_READ];
		int current_read = 0;
		int total_read = 0;
		do {
			url_read_stream->Read(buffer, MAX_HTTP_READ);
			current_read = url_read_stream->LastRead();
			if (current_read) {
				total_read += current_read;
				outfile.Write(buffer,current_read);
				int current_write = outfile.LastWrite();
				if (current_read != current_write) {
					throw wxString(_("Critical error while writing downloaded file"));
				} else if (m_companion) {
#ifndef AMULE_DAEMON
					CMuleInternalEvent evt(wxEVT_HTTP_PROGRESS);
					evt.SetInt(total_read);
					evt.SetExtraLong(download_size);
					wxPostEvent(m_companion, evt);
#endif
				}
			}
		} while (current_read && !TestDestroy());

		if (current_read == 0) {
			if (download_size == -1) {
				// Download was probably succesful.
				AddLogLineN(CFormat(_("Downloaded %d bytes")) % total_read);
				m_result = HTTP_Success;
			} else if (total_read != download_size) {
				m_result = HTTP_Error;
				throw wxString(CFormat(_("Expected %d bytes, but downloaded %d bytes")) % download_size % total_read);
			} else {
				// Download was succesful.
				m_result = HTTP_Success;
			}
		}

#endif /* ifelse HAVE_LIBCURL */

	} catch (const wxString& error) {
		if (wxFileExists(m_tempfile)) {
			wxRemoveFile(m_tempfile);
		}
		if (!error.IsEmpty()) {
			AddLogLineC(error);
		}
	}

	if (m_result == HTTP_Success) {
		thePrefs::SetLastHTTPDownloadURL(m_file_id, m_url);
	}

#ifndef HAVE_LIBCURL
	if (url_handler) {
		url_handler->Destroy();
	}
#endif

	AddDebugLogLineN(logHTTP, wxT("HTTP download thread ended"));

	return 0;
}


void CHTTPDownloadThread::OnExit()
{
#ifndef AMULE_DAEMON
	if (m_companion) {
		CMuleInternalEvent termEvent(wxEVT_HTTP_SHUTDOWN);
		wxPostEvent(m_companion, termEvent);
	}
#endif

	// Notice the app that the file finished download
	CMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)m_file_id);
	evt.SetExtraLong((long)m_result);
	wxPostEvent(wxTheApp, evt);
	wxMutexLocker lock(s_allThreadsMutex);
	s_allThreads.erase(this);
}


#ifndef HAVE_LIBCURL
//! This function's purpose is to handle redirections in a proper way.
wxInputStream* CHTTPDownloadThread::GetInputStream(wxHTTP * & url_handler, const wxString& location, bool proxy)
{
	// Extract the protocol name
	wxString protocol(location.BeforeFirst(wxT(':')));

	if (TestDestroy()) {
		return NULL;
	}

	if (protocol != wxT("http")) {
		// This is not a http url
		throw wxString(CFormat(_("Protocol not supported for HTTP download: %s")) % protocol);
	}

	// Get the host

	// Remove the "http://"
	wxString host = location.Right(location.Len() - 7); // strlen("http://") -> 7

	// I belive this is a bug...
	// Sometimes "Location" header looks like this:
	// "http://www.whatever.com:8080http://www.whatever.com/downloads/something.zip"
	// So let's clean it...

	int bad_url_pos = host.Find(wxT("http://"));
	wxString location_url;
	if (bad_url_pos != -1) {
		// Malformed Location field on header (bug above?)
		location_url = host.Mid(bad_url_pos);
		host = host.Left(bad_url_pos);
		// After the first '/' non-http-related, it's the location part of the URL
		location_url = location_url.Right(location_url.Len() - 7).AfterFirst(wxT('/'));
	} else {
		// Regular Location field
		// After the first '/', it's the location part of the URL
		location_url = host.AfterFirst(wxT('/'));
		// The host is everything till the first "/"
		host = host.BeforeFirst(wxT('/'));
	}

	// Build the cleaned url now
	wxString url = wxT("http://") + host + wxT("/") + location_url;

	int port = 80;
	if (host.Find(wxT(':')) != -1) {
		// This http url has a port
		port = wxAtoi(host.AfterFirst(wxT(':')));
		host = host.BeforeFirst(wxT(':'));
	}

	wxIPV4address addr;
	addr.Hostname(host);
	addr.Service(port);
	if (!url_handler->Connect(addr, true)) {
		throw wxString(_("Unable to connect to HTTP download server"));
	}

	wxInputStream* url_read_stream = url_handler->GetInputStream(url);

	/* store the HTTP response code */
	m_response = url_handler->GetResponse();

	/* store the HTTP error code */
	m_error = url_handler->GetError();

	AddDebugLogLineN(logHTTP, CFormat(wxT("Host: %s:%i\n")) % host % port);
	AddDebugLogLineN(logHTTP, CFormat(wxT("URL: %s\n")) % url);
	AddDebugLogLineN(logHTTP, CFormat(wxT("Response: %i (Error: %i)")) % m_response % m_error);

	if (!m_response) {
		AddDebugLogLineC(logHTTP, wxT("WARNING: Void response on stream creation"));
		// WTF? Why does this happen?
		// This is probably produced by an already existing connection, because
		// the input stream is created nevertheless. However, data is not the same.
		delete url_read_stream;
		throw wxString(_("Invalid response from HTTP download server"));
	}

	if (m_response == 301  // Moved permanently
		|| m_response == 302 // Moved temporarily
		// What about 300 (multiple choices)? Do we have to handle it?
		) {

		// We have to remove the current stream.
		delete url_read_stream;

		wxString new_location = url_handler->GetHeader(wxT("Location"));
		AddDebugLogLineN(logHTTP, CFormat(wxT("Redirecting to: %s")) % new_location);

		url_handler->Destroy();
		if (!new_location.IsEmpty()) {
			url_handler = new wxHTTP;
			url_handler->SetProxyMode(proxy);
			if (m_lastmodified.IsValid()) {
				// Set a flag in the HTTP header that we only download if the file is newer.
				// see: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.25
				url_handler->SetHeader(wxT("If-Modified-Since"), FormatDateHTTP(m_lastmodified));
			}
			url_read_stream = GetInputStream(url_handler, new_location, proxy);
		} else {
			AddDebugLogLineC(logHTTP, wxT("ERROR: Redirection code received with no URL"));
			url_handler = NULL;
			url_read_stream = NULL;
		}
	} else if (m_response == 304) {		// "Not Modified"
		delete url_read_stream;
		url_handler->Destroy();
		url_read_stream = NULL;
		url_handler = NULL;
	}

	return url_read_stream;
}
#endif /* !HAVE_LIBCURL */

void CHTTPDownloadThread::StopAll()
{
	ThreadSet allThreads;
	{
		wxMutexLocker lock(s_allThreadsMutex);
		std::swap(allThreads, s_allThreads);
	}
	for (ThreadSet::iterator it = allThreads.begin(); it != allThreads.end(); ++it) {
		(*it)->Stop();
	}
}

CHTTPDownloadThread::ThreadSet CHTTPDownloadThread::s_allThreads;
wxMutex CHTTPDownloadThread::s_allThreadsMutex;

// File_checked_for_headers
