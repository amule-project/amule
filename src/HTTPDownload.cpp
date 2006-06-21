//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Tiku
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


#include <wx/wfstream.h>
#include <wx/protocol/http.h>


#include "HTTPDownload.h"				// Interface declarations
#include <common/StringFunctions.h>		// Needed for unicode2char
#include "OtherFunctions.h"				// Needed for CastChild
#include "Logger.h"						// Needed for AddLogLine*
#include <common/Format.h>				// Needed for CFormat
#include "InternalEvents.h"				// Needed for CMuleInternalEvent
#include "Preferences.h"


#ifndef AMULE_DAEMON
#include "inetdownload.h"	// Needed for inetDownload
#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
#include "MuleGifCtrl.h"

#ifdef __WXMSW__
typedef wxGauge95 wxGaugeControl;
#else
typedef wxGauge wxGaugeControl;
#endif


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
	 	m_thread->Delete();
		delete m_thread;
	}	

	void UpdateGauge(int total, int current) {
		CFormat label(_("( %s / %s )"));
		
		label % CastItoXBytes(current);
		if (total > 0) {
			label % CastItoXBytes(total);
		} else {
			label % _("Unknown");
		}
	
		CastChild(IDC_DOWNLOADSIZE, wxStaticText)->SetLabel(label.GetString());
	
		if (total and (total != m_progressbar->GetRange())) {
			m_progressbar->SetRange(total);
		}
	
		if (current && (current <= total)) {
			m_progressbar->SetValue(current);
		}
	
		Layout();
	}

private:
	DECLARE_EVENT_TABLE();

	void OnBtnCancel(wxCommandEvent& WXUNUSED(evt)) {
		printf("HTTP download cancelled\n");
		Show(false);
	 	m_thread->Delete();
	}
	
	void OnProgress(CMuleInternalEvent& evt) {
		UpdateGauge(evt.GetExtraLong(), evt.GetInt());
	}

	void OnShutdown(CMuleInternalEvent& WXUNUSED(evt)) {
		Show(false);
		Destroy();
	}
  
	wxThread*		m_thread;
	MuleGifCtrl* 	m_ani;
	wxGaugeControl* m_progressbar;
};


BEGIN_EVENT_TABLE(CHTTPDownloadDialog, wxDialog)
	EVT_BUTTON(ID_HTTPCANCEL, CHTTPDownloadDialog::OnBtnCancel)
	EVT_MULE_INTERNAL(wxEVT_HTTP_PROGRESS, -1, CHTTPDownloadDialog::OnProgress)
	EVT_MULE_INTERNAL(wxEVT_HTTP_SHUTDOWN, -1, CHTTPDownloadDialog::OnShutdown)
END_EVENT_TABLE()

DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_PROGRESS)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_HTTP_SHUTDOWN)
	
#endif


CHTTPDownloadThread::CHTTPDownloadThread(const wxString& url, const wxString& filename, HTTP_Download_File file_id, bool showDialog)
#ifdef AMULE_DAEMON
	: wxThread(wxTHREAD_DETACHED),
#else
	: wxThread(showDialog ? wxTHREAD_JOINABLE : wxTHREAD_DETACHED),
#endif
	  m_url(url),
	  m_tempfile(filename),
	  m_result(1),
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
}


wxThread::ExitCode CHTTPDownloadThread::Entry()
{
	if (TestDestroy()) { 
		// Thread going down...
		m_result = -1;
		return NULL;
	}
	
	wxHTTP* url_handler = NULL;
	wxInputStream* url_read_stream = NULL;
	
	printf("HTTP download thread started\n");
	
	const CProxyData* proxy_data = thePrefs::GetProxyData();
	bool use_proxy = proxy_data != NULL && proxy_data->m_proxyEnable;
	
	try {	
		wxFFileOutputStream outfile(m_tempfile);
		
		if (!outfile.Ok()) {
			throw wxString(CFormat(wxT("Unable to create destination file %s for download!\n")) % m_tempfile);
		}
			
		if ( m_url.IsEmpty() ) {
			// Nowhere to download from!
			throw wxString(wxT("The URL to download can't be empty\n"));
		}

		url_handler = new wxHTTP;
		url_handler->SetProxyMode(use_proxy);
	
		url_read_stream = GetInputStream(&url_handler, m_url, use_proxy);
		
		if (!url_read_stream) {
			throw wxString(CFormat(wxT("The URL %s returned: %i - Error (%i)!")) % m_url % url_handler->GetResponse() % url_handler->GetError());
		}
		
		int download_size = url_read_stream->GetSize();
		printf("Download size: %i\n",download_size);
		
		// Here is our read buffer
		// <ken> Still, I'm sure 4092 is probably a better size.
		const unsigned MAX_HTTP_READ = 4092;
		
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
					throw wxString(wxT("Critical error while writing downloaded file"));
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
	} catch (const wxString& error) {
		if (wxFileExists(m_tempfile)) {
			wxRemoveFile(m_tempfile);
		}

		m_result = -1;		
		AddLogLineM(false, error);
	}

	delete url_read_stream;
	if (url_handler) {
		url_handler->Destroy();
	}
	
	printf("HTTP download thread ended\n");
	
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
}


//! This function's purpose is to handle redirections in a proper way.
wxInputStream* CHTTPDownloadThread::GetInputStream(wxHTTP** url_handler, const wxString& location, bool proxy)
{
	if (TestDestroy()) {
		return NULL;
	}
	
	if (!location.StartsWith(wxT("http://"))) {
		// This is not a http url
		throw wxString(wxT("Invalid URL for server.met download or http redirection (did you forget 'http://' ?)"));
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
	if (!(*url_handler)->Connect(addr, true)) {
		throw wxString(wxT("Unable to connect to http download server"));		
	}

	wxInputStream* url_read_stream = (*url_handler)->GetInputStream(url);

	printf("Host: %s:%i\n",(const char*)unicode2char(host),port);
	printf("URL: %s\n",(const char*)unicode2char(url));
	printf("Response: %i (Error: %i)\n",(*url_handler)->GetResponse(), (*url_handler)->GetError());

	if (!(*url_handler)->GetResponse()) {
		printf("WARNING: Void response on stream creation\n");
		// WTF? Why does this happen?
		// This is probably produced by an already existing connection, because
		// the input stream is created nevertheless. However, data is not the same.
		delete url_read_stream;
		throw wxString(wxT("Invalid response from http download server"));
	}

	if ((*url_handler)->GetResponse() == 301  // Moved permanently
		|| (*url_handler)->GetResponse() == 302 // Moved temporarily
		// What about 300 (multiple choices)? Do we have to handle it?
		) { 
		
		// We have to remove the current stream.
		delete url_read_stream;
			
		wxString new_location = (*url_handler)->GetHeader(wxT("Location"));
		
		(*url_handler)->Destroy();
		if (!new_location.IsEmpty()) {
			(*url_handler) = new wxHTTP;
			(*url_handler)->SetProxyMode(proxy);
			url_read_stream = GetInputStream(url_handler, new_location, proxy);
		} else {
			printf("ERROR: Redirection code received with no URL\n");
			url_handler = NULL;
			url_read_stream = NULL;
		}
	}
		
	return url_read_stream;
}

// File_checked_for_headers
