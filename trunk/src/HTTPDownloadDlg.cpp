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

// ClientDetailDialog.cpp : implementation file
//

#ifdef __WXMSW__
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#endif

#include "muuli_wdr.h"		// Needed for ID_CANCEL: Let it here or will fail on win32

#include <wx/intl.h>
#include <wx/sizer.h> 

#include <curl/curl.h>
#include "HTTPDownloadDlg.h"	// Interface declarations
#include "MuleGifCtrl.h"
#include "inetdownload.h"	// Needed for inetDownload
#include "otherfunctions.h"	// Needed for unicode2char


BEGIN_EVENT_TABLE(CHTTPDownloadDlg,wxDialog)
  EVT_BUTTON(ID_CANCEL,CHTTPDownloadDlg::OnBtnCancel)
END_EVENT_TABLE()


CHTTPDownloadDlg::CHTTPDownloadDlg(wxWindow* parent,wxString url,wxString tempName)
  : wxDialog(parent,1025,_("Downloading..."),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
  downloadDlg(this,TRUE)->Show(this,TRUE);

  MuleGifCtrl *ani=(MuleGifCtrl*)FindWindowById(ID_ANIMATE);
  ani->LoadData((char*)inetDownload,sizeof(inetDownload));
  ani->Start();

  Centre();

  thread=new myThread(this,url,tempName);
  thread->Create();
  thread->Run();

}

void CHTTPDownloadDlg::OnBtnCancel(wxCommandEvent& WXUNUSED(evt))
{

 thread->setDeleteGUI(false);
 EndModal(ID_CANCEL);

}


CHTTPDownloadDlg::~CHTTPDownloadDlg() {

	if (thread!=NULL) {
		thread->setDeleteGUI(false);
	}

}


myThread::~myThread()
{
//maybe a thread deletion needed
}

void myThread::setDeleteGUI(bool set)
{
  delete_gui=set;
}

bool myThread::getDeleteGUI()
{
return delete_gui;

}


wxThread::ExitCode myThread::Entry() {

 CURL *curl_handle;
 FILE *outfile;
 char* tempurl;
 
 if (TestDestroy()) return NULL;
 curl_handle = curl_easy_init();
 outfile = fopen(unicode2char(tempfile), "w");
 if (TestDestroy()) {fclose(outfile); return NULL; }
 if (outfile!=NULL) {
 if (!url) {fclose(outfile); return NULL; }
    tempurl=strdup((char*)unicode2char(url));
  
    curl_easy_setopt(curl_handle, CURLOPT_URL, tempurl);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, TRUE);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS , 10);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION , 1);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT , 15);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/4");
    curl_easy_setopt(curl_handle, CURLOPT_FILE, outfile);
    
    if (TestDestroy()) {fclose(outfile); free(tempurl); return 0; }
    if (curl_easy_perform(curl_handle)==CURLE_OK) result=0;
    fclose(outfile);
    free(tempurl);
 }
 curl_easy_cleanup(curl_handle);
 return 0;
}

void myThread::OnExit() {

	if (myDlg!=NULL && getDeleteGUI() ) {
		myDlg->EndModal(result);
	}

}
