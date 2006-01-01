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

#ifdef __WXMSW__
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#endif


#include <wx/intl.h>
#include <wx/wfstream.h>
#include <wx/protocol/http.h>
#include <cmath>
#include <memory>

#include "amule.h"
#include "HTTPDownload.h"	// Interface declarations
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "OtherFunctions.h" 	// Needed for CastChild
#include "Logger.h"		// Needed for AddLogLine*
#include <common/Format.h>		// Needed for CFormat
#include "InternalEvents.h"	// Needed for wxMuleInternalEvent
#include "Proxy.h"
#include "Preferences.h"

#ifndef AMULE_DAEMON 
	#include "inetdownload.h"	// Needed for inetDownload
	#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
	#include "MuleGifCtrl.h"
	#include <wx/sizer.h> 
	#include <wx/gauge.h>
	#include <wx/stattext.h>
#endif



#ifndef AMULE_DAEMON 
BEGIN_EVENT_TABLE(CHTTPDownloadThreadDlg,wxDialog)
  EVT_BUTTON(ID_HTTPCANCEL, CHTTPDownloadThreadDlg::OnBtnCancel)
END_EVENT_TABLE()

CHTTPDownloadThreadDlg::CHTTPDownloadThreadDlg(wxWindow* parent, CHTTPDownloadThread* thread)
  : wxDialog(parent, -1,_("Downloading..."),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	downloadDlg(this,TRUE)->Show(this,TRUE);
	
	m_progressbar = CastChild(ID_HTTPDOWNLOADPROGRESS,wxGauge);
	wxASSERT(m_progressbar);
	m_progressbar->SetRange(100);	// Just Temp
	
	m_ani = CastChild(ID_ANIMATE, MuleGifCtrl);
	m_ani->LoadData((char*)inetDownload,sizeof(inetDownload));
	m_ani->Start();
	
	m_parent_thread = thread;

}

void CHTTPDownloadThreadDlg::OnBtnCancel(wxCommandEvent& WXUNUSED(evt))
{
	printf("HTTP download cancelled\n");
 	m_parent_thread->Delete();
}

void CHTTPDownloadThreadDlg::StopAnimation() 
{ 
	if (m_ani) {
		m_ani->Stop();
	}
}

CHTTPDownloadThreadDlg::~CHTTPDownloadThreadDlg() 
{
	Show(false);
	if (m_ani) {
		m_ani->Stop();
	}
}

void CHTTPDownloadThreadDlg::UpdateGauge(int dltotal,int dlnow) 
{	
	wxString label = wxT("( ") + CastItoXBytes(dlnow) + wxT(" / ");
	if (dltotal > 0) {
		label += CastItoXBytes(dltotal);
	} else {
		label += _("Unknown");
	}
	
	label += wxT(" )");
	
	CastChild(IDC_DOWNLOADSIZE, wxStaticText)->SetLabel(label);
	
	if ((dltotal != m_progressbar->GetRange()) && (dltotal > 0)) {
		m_progressbar->SetRange(dltotal);
	}
	if ((dlnow > 0) && (dlnow <= dltotal))  {
		m_progressbar->SetValue(dlnow);
	}
	
	Layout();
}

CHTTPDownloadThreadGUI::CHTTPDownloadThreadGUI(wxString urlname, wxString filename,HTTP_Download_File file_id)
: CHTTPDownloadThreadBase(urlname,filename,file_id)
{
	#if !defined(__WXMAC__)
		m_myDlg= new CHTTPDownloadThreadDlg(theApp.GetTopWindow(), this);
		m_myDlg->Show(true);
	#endif
}

CHTTPDownloadThreadGUI::~CHTTPDownloadThreadGUI()
{	
	#if !defined(__WXMAC__)
		wxMutexGuiEnter();
		if (m_myDlg != NULL) {
			m_myDlg->StopAnimation();
			delete m_myDlg;
		}
		wxMutexGuiLeave();
	#endif	
}

void CHTTPDownloadThreadGUI::ProgressCallback(int dltotal, int dlnow) 
{
	#if !defined(__WXMAC__)
		wxMutexGuiEnter();
		m_myDlg->UpdateGauge(dltotal,dlnow);
		wxMutexGuiLeave();
	#endif	
}

#endif

CHTTPDownloadThreadBase::CHTTPDownloadThreadBase(wxString urlname, wxString filename,HTTP_Download_File file_id):wxThread(wxTHREAD_DETACHED) 
{
  	m_url = urlname;
  	m_tempfile = filename;
  	m_result = 1;
	m_file_type = file_id;
}

CHTTPDownloadThreadBase::~CHTTPDownloadThreadBase()
{	
	//maybe a thread deletion needed
}

wxThread::ExitCode CHTTPDownloadThreadBase::Entry()
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
			throw(wxString(CFormat(wxT("Unable to create destination file %s for download!\n")) % m_tempfile));
		}
			
		if ( m_url.IsEmpty() ) {
			// Nowhere to download from!
			throw(wxString(wxT("The URL to download can't be empty\n")));
		}

		url_handler = new wxHTTP;
		
		url_handler->SetProxyMode(use_proxy);
	
		url_read_stream = GetInputStream(&url_handler, m_url,use_proxy);
		
		if (!url_read_stream) {
			throw(wxString(CFormat(wxT("The URL %s returned: %i - Error (%i)!")) % m_url % url_handler->GetResponse() % url_handler->GetError()));
		}
		
		int download_size = url_read_stream->GetSize();
		printf("Download size: %i\n",download_size);
		
		// Here is our read buffer
		// <ken> Still, I'm sure 4092 is probably a better size.
		#define MAX_HTTP_READ 4092
		
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
					throw(wxString(wxT("Critical error while writing downloaded file")));
				} else {
					ProgressCallback(download_size, total_read);
				}
			}
		} while (current_read && !TestDestroy());
	} catch (const wxString& download_error) {
		if (wxFileExists(m_tempfile)) {
			wxRemoveFile(m_tempfile);
		}
		m_result = -1;		
		AddLogLineM(false,download_error);
	}

	if (url_read_stream) {
		delete url_read_stream;
	}

	if (url_handler) {
		url_handler->Destroy();
	}
	
	printf("HTTP download thread end\n");
	
	return 0;
}

void CHTTPDownloadThreadBase::OnExit() 
{
	// Kry - Notice the app that the file finished download
	wxMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)m_file_type);
	evt.SetExtraLong((long)m_result);
	wxPostEvent(&theApp,evt);
}

wxInputStream* CHTTPDownloadThreadBase::GetInputStream(wxHTTP** url_handler, const wxString& location, bool proxy) {
	// This function's purpose is to handle redirections in a proper way.
	
	if (TestDestroy()) {
		return NULL;
	}
	
	if ( !location.StartsWith(wxT("http://"))) {
		// This is not a http url
		throw(wxString(wxT("Invalid URL for server.met download or http redirection (did you forget 'http://' ?)")));
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
