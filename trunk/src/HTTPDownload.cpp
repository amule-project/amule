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

#include <curl/curl.h>

#include "amule.h"
#include "HTTPDownload.h"	// Interface declarations
#include "inetdownload.h"	// Needed for inetDownload
#include "otherfunctions.h"	// Needed for unicode2char

#ifndef AMULE_DAEMON 
	#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32
	#include "MuleGifCtrl.h"
	#include <wx/sizer.h> 
#endif



#ifndef AMULE_DAEMON 
BEGIN_EVENT_TABLE(CHTTPThreadDlg,wxDialog)
  EVT_BUTTON(ID_CANCEL,CHTTPThreadDlg::OnBtnCancel)
END_EVENT_TABLE()


CHTTPThreadDlg::CHTTPThreadDlg(wxWindow* parent, HTTPThread* thread)
  : wxDialog(parent,1025,_("Downloading..."),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
  downloadDlg(this,TRUE)->Show(this,TRUE);

  ani = (MuleGifCtrl*)FindWindowById(ID_ANIMATE);
  ani->LoadData((char*)inetDownload,sizeof(inetDownload));
  ani->Start();

  Centre();
	
  parent_thread = thread;

}

void CHTTPThreadDlg::OnBtnCancel(wxCommandEvent& WXUNUSED(evt))
{
  // TODO
}

CHTTPThreadDlg::~CHTTPThreadDlg() {
	ani->Stop();
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
	CURL *curl_handle;
	FILE *outfile;
	char* tempurl;

	if (TestDestroy()) return NULL;
	curl_handle = curl_easy_init();
	outfile = fopen(unicode2char(tempfile), "w");
	if (TestDestroy()) {fclose(outfile); return NULL; }
	if (outfile!=NULL) {
 		if (!url) {
			fclose(outfile);
			return NULL;
		}
		tempurl = strdup((char*)unicode2char(url));

		curl_easy_setopt(curl_handle, CURLOPT_URL, tempurl);
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, TRUE);
		curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS , 10);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION , 1);
//		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT , 15);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/4");
		curl_easy_setopt(curl_handle, CURLOPT_FILE, outfile);

		if (TestDestroy()) {
			fclose(outfile);
			free(tempurl);
			return 0;
		}
		if (curl_easy_perform(curl_handle)==CURLE_OK) result = 0;
		fclose(outfile);
		free(tempurl);
	}
	curl_easy_cleanup(curl_handle);
	printf("HTTP download thread end\n");
	
	return 0;
}

void HTTPThread::OnExit() {
#ifndef AMULE_DAEMON 
	if (myDlg!=NULL) {
		myDlg->Show(false);
		delete myDlg;
	}
#endif
	// Kry - Notice the app that the file finished download
	wxMuleInternalEvent evt(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD);
	evt.SetInt((int)file_type);
	evt.SetExtraLong((long)result);
	wxPostEvent(&theApp,evt);		
}

