//this file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Tiku ( )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// HTTPDownload.cpp : implementation file
//

#ifdef __WXMSW__
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#endif

#include <wx/intl.h>
#include <cmath>
#include <curl/curl.h>

#include "amule.h"
#include "HTTPDownload.h"	// Interface declarations
#include "inetdownload.h"	// Needed for inetDownload
#include "otherfunctions.h"	// Needed for unicode2char

#ifndef AMULE_DAEMON 
	#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
	#include "MuleGifCtrl.h"
	#include <wx/sizer.h> 
	#include <wx/gauge.h>
#endif



#ifndef AMULE_DAEMON 
BEGIN_EVENT_TABLE(CHTTPThreadDlg,wxDialog)
  EVT_BUTTON(ID_CANCEL,CHTTPThreadDlg::OnBtnCancel)
END_EVENT_TABLE()

int CurlGaugeCallback(void *HTTPDlDlg, double dltotal, double dlnow, double ultotal, double ulnow);

CHTTPThreadDlg::CHTTPThreadDlg(wxWindow* parent, HTTPThread* thread)
  : wxDialog(parent,1025,_("Downloading..."),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	downloadDlg(this,TRUE)->Show(this,TRUE);
	
	m_progressbar = (wxGauge*)FindWindow(ID_HTTPDOWNLOADPROGRESS);
	wxASSERT(m_progressbar);
	m_progressbar->SetRange(100);	// Just Temp
	
	ani = (MuleGifCtrl*)FindWindowById(ID_ANIMATE);
	ani->LoadData((char*)inetDownload,sizeof(inetDownload));
	ani->Start();

	Centre();
	
	parent_thread = thread;

}

void CHTTPThreadDlg::OnBtnCancel(wxCommandEvent& WXUNUSED(evt))
{
  parent_thread->Delete();
}

void CHTTPThreadDlg::StopAnimation() { 
	ani->Stop();
};

CHTTPThreadDlg::~CHTTPThreadDlg() {
	Show(false);
	ani->Stop();
}

void CHTTPThreadDlg::UpdateGauge(int dltotal,int dlnow) {	
	
	if ((dltotal != m_progressbar->GetRange()) && (dltotal > 0)) {
		m_progressbar->SetRange(dltotal);
	}
	if ((dlnow > 0) && (dlnow <= dltotal))  {
		m_progressbar->SetValue(dlnow);
	}
}

#endif

HTTPThread::HTTPThread(wxString urlname, wxString filename,HTTP_Download_File file_id):wxThread(wxTHREAD_DETACHED) {
  	url= urlname;
  	tempfile=filename;
  	result=1;
	file_type = file_id;
	#ifndef AMULE_DAEMON 
  		myDlg= new CHTTPThreadDlg(theApp.GetTopWindow(), this);
		myDlg->Show(true);
	#endif
};

HTTPThread::~HTTPThread()
{	
	//maybe a thread deletion needed
}

wxThread::ExitCode HTTPThread::Entry()
{
	if (TestDestroy()) { 
		// Thread going down...
		return NULL;
	}
	
	FILE *outfile = fopen(unicode2char(tempfile), "w");
	
	if (outfile!=NULL) {
 		if (!url) {
			// Nowhere to download from!
			fclose(outfile);
			return NULL;
		}
		
		// Init the handle
		CURL *curl_handle = curl_easy_init();
		CURLM *curl_multi_handle =  curl_multi_init();
		
		char * tempurl = strdup((char*)unicode2char(url));
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
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, myDlg);
	#endif
		// Add the easy handle to the multi handle
		curl_multi_add_handle(curl_multi_handle, curl_handle);
		
		int running_handles = 1;

		while (running_handles) {
			if (TestDestroy()) {
				// Cancel button or going down.
				result =0;
				break;
			}
			curl_multi_perform(curl_multi_handle, &running_handles);
		}
		
		fclose(outfile);
		free(tempurl);
		curl_multi_remove_handle(curl_multi_handle, curl_handle);
		curl_easy_cleanup(curl_handle);
		curl_multi_cleanup(curl_multi_handle);		
	}
	printf("HTTP download thread end\n");
	
	return 0;
}

void HTTPThread::OnExit() {
	
#ifndef AMULE_DAEMON 
	wxMutexGuiEnter();
	if (myDlg!=NULL) {
		delete myDlg;
	}
	wxMutexGuiLeave();
#endif
	// Kry - Notice the app that the file finished download
	wxMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)file_type);
	evt.SetExtraLong((long)result);
	wxPostEvent(&theApp,evt);		
}



#ifndef AMULE_DAEMON 
int CurlGaugeCallback(void *HTTPDlDlg, double dltotal, double dlnow, double WXUNUSED(ultotal), double WXUNUSED(ulnow)) {	
//	printf("CB: %f %f - %i %i\n",dltotal, dlnow, int(dltotal),int(dlnow));
	wxMutexGuiEnter();
	((CHTTPThreadDlg*)HTTPDlDlg)->UpdateGauge(int(dltotal),int(dlnow));
	wxMutexGuiLeave();
	return 0;
}
#endif
