//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Tiku & Hetfield ( hetfield@amule.org )
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

#ifndef HTTPDOWNLOAD_H
#define HTTPDOWNLOAD_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "HTTPDownload.h"
#endif

#ifndef AMULE_DAEMON
	#include <wx/dialog.h>		// Needed for wxDialog
#endif

#include "GuiEvents.h"

class CHTTPDownloadThread;
#ifdef __WXMSW__
class wxGauge95;
#else
class wxGauge;
#endif

class MuleGifCtrl;

class wxInputStream;

class wxHTTP;
	
#ifndef AMULE_DAEMON
class CHTTPDownloadThreadDlg : public wxDialog
{
public:
	CHTTPDownloadThreadDlg(wxWindow*parent, CHTTPDownloadThread* thread);
	~CHTTPDownloadThreadDlg();

	void StopAnimation(); 
	void UpdateGauge(int dltotal,int dlnow);

private:
	DECLARE_EVENT_TABLE()

	void OnBtnCancel(wxCommandEvent& evt);
  
	CHTTPDownloadThread*	m_parent_thread;
	MuleGifCtrl* 					m_ani;
#ifdef __WXMSW__	
	wxGauge95* 						m_progressbar;
#else
	wxGauge* 						m_progressbar;
#endif
};
#endif


class CHTTPDownloadThread : public wxThread
{
private:

	wxThread::ExitCode	Entry();
	virtual void 			OnExit();

	wxString					m_url;
	wxString					m_tempfile;
	int						m_result;
	HTTP_Download_File	m_file_type;

	wxInputStream* GetInputStream(wxHTTP& url_handler, const wxString& location);

#ifndef AMULE_DAEMON 
	CHTTPDownloadThreadDlg* m_myDlg;
#endif


 public:
//	myThread::myThread(wxEvtHandler* parent,char* urlname,char* filename);
	~CHTTPDownloadThread();

	CHTTPDownloadThread(wxString urlname, wxString filename, HTTP_Download_File file_id);

};

#endif // HTTPDOWNLOAD_H