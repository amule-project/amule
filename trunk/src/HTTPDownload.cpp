//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

//
// HTTPDownload.cpp : implementation file
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "HTTPDownload.h"
#endif

#ifdef __WXMSW__
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#endif

#include <wx/intl.h>
#include <cmath>
#include <curl/curl.h>

#include "amule.h"
#include "HTTPDownload.h"	// Interface declarations
#ifndef AMULE_DAEMON
#include "inetdownload.h"	// Needed for inetDownload
#endif
#include "StringFunctions.h"	// Needed for unicode2char
#include "OtherFunctions.h" 	// Needed for CastChild

#ifndef AMULE_DAEMON 
	#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
	#include "MuleGifCtrl.h"
	#include <wx/sizer.h> 
	#include <wx/gauge.h>
#endif



#ifndef AMULE_DAEMON 
BEGIN_EVENT_TABLE(CHTTPDownloadThreadDlg,wxDialog)
  EVT_BUTTON(ID_CANCEL,CHTTPDownloadThreadDlg::OnBtnCancel)
END_EVENT_TABLE()

int CurlGaugeCallback(void *HTTPDlDlg, double dltotal, double dlnow, double ultotal, double ulnow);

CHTTPDownloadThreadDlg::CHTTPDownloadThreadDlg(wxWindow* parent, CHTTPDownloadThread* thread)
  : wxDialog(parent,1025,_("Downloading..."),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	downloadDlg(this,TRUE)->Show(this,TRUE);
	
	m_progressbar = CastChild(ID_HTTPDOWNLOADPROGRESS,wxGauge);
	wxASSERT(m_progressbar);
	m_progressbar->SetRange(100);	// Just Temp
	
	m_ani = CastChild(ID_ANIMATE, MuleGifCtrl);
	m_ani->LoadData((char*)inetDownload,sizeof(inetDownload));
	m_ani->Start();

	Centre();
	
	m_parent_thread = thread;

}

void CHTTPDownloadThreadDlg::OnBtnCancel(wxCommandEvent& WXUNUSED(evt))
{
  m_parent_thread->Delete();
}

void CHTTPDownloadThreadDlg::StopAnimation() 
{ 
	m_ani->Stop();
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
	
	if ((dltotal != m_progressbar->GetRange()) && (dltotal > 0)) {
		m_progressbar->SetRange(dltotal);
	}
	if ((dlnow > 0) && (dlnow <= dltotal))  {
		m_progressbar->SetValue(dlnow);
	}
}

#endif

CHTTPDownloadThread::CHTTPDownloadThread(wxString urlname, wxString filename,HTTP_Download_File file_id):wxThread(wxTHREAD_DETACHED) 
{
  	m_url = urlname;
  	m_tempfile = filename;
  	m_result = 1;
	m_file_type = file_id;
	#ifndef AMULE_DAEMON 
  		m_myDlg= new CHTTPDownloadThreadDlg(theApp.GetTopWindow(), this);
		m_myDlg->Show(true);
	#endif
}

CHTTPDownloadThread::~CHTTPDownloadThread()
{	
	//maybe a thread deletion needed
}

wxThread::ExitCode CHTTPDownloadThread::Entry()
{
	if (TestDestroy()) { 
		// Thread going down...
		return NULL;
	}
	
	FILE *outfile = fopen(unicode2char(m_tempfile), "w");
	
	if (outfile!=NULL) {
		if ( m_url.IsEmpty() ) {
			// Nowhere to download from!
			fclose(outfile);
			return NULL;
		}
		
		// Init the handle
		CURL *curl_handle = curl_easy_init();
		#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
		CURLM *curl_multi_handle =  curl_multi_init();
		#endif
		
		char * tempurl = strdup((const char *)unicode2char(m_url));
		// Options for the easy handler
		curl_easy_setopt(curl_handle, CURLOPT_URL, tempurl);
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS , 10);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION , 1);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/4");
		curl_easy_setopt(curl_handle, CURLOPT_FILE, outfile);
	#ifndef AMULE_DAEMON
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, FALSE);	
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, &CurlGaugeCallback);
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, m_myDlg);
	#endif
		
		#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
		// Add the easy handle to the multi handle
		curl_multi_add_handle(curl_multi_handle, curl_handle);
		
		int running_handles = 1;

		while (running_handles) {
			if (TestDestroy()) {
				// Cancel button or going down.
				m_result =0;
				break;
			}
			curl_multi_perform(curl_multi_handle, &running_handles);
		}
		#else
		if (TestDestroy() || (curl_easy_perform(curl_handle)!=CURLE_OK)) {
			m_result =0;
		}	
		#endif
		
		fclose(outfile);
		free(tempurl);
		#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
		curl_multi_remove_handle(curl_multi_handle, curl_handle);
		curl_easy_cleanup(curl_handle);
		curl_multi_cleanup(curl_multi_handle);				
		#else
		curl_easy_cleanup(curl_handle);
		#endif
	}
	
#ifndef AMULE_DAEMON 
	m_myDlg->StopAnimation();
#endif	
	
	printf("HTTP download thread end\n");
	
	return 0;
}

void CHTTPDownloadThread::OnExit() 
{
	
#ifndef AMULE_DAEMON 
	wxMutexGuiEnter();
	if (m_myDlg!=NULL) {
		delete m_myDlg;
	}
	wxMutexGuiLeave();
#endif
	// Kry - Notice the app that the file finished download
	wxMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)m_file_type);
	evt.SetExtraLong((long)m_result);
	wxPostEvent(&theApp,evt);		
}

#ifndef AMULE_DAEMON 
int CurlGaugeCallback(void *HTTPDlDlg, double dltotal, double dlnow, double WXUNUSED(ultotal), double WXUNUSED(ulnow)) 
{	
	wxMutexGuiEnter();
//	printf("CB: %f %f - %i %i\n",dltotal, dlnow, int(dltotal),int(dlnow));
	((CHTTPDownloadThreadDlg*)HTTPDlDlg)->UpdateGauge(int(dltotal),int(dlnow));
	wxMutexGuiLeave();
	return 0;
}
#endif
