//this file is part of amule
//Copyright (C)2002 Tiku ( ) & Hetfield <hetfield@email.it>
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

#ifndef HTTPDOWNLOADDLG_H
#define HTTPDOWNLOADDLG_H

#include <wx/dialog.h>		// Needed for wxDialog

class myThread;

class CHTTPDownloadDlg : public wxDialog
{
 public:
  CHTTPDownloadDlg(wxWindow*parent,wxString url,wxString tempName);
  ~CHTTPDownloadDlg();

 private:
  DECLARE_EVENT_TABLE()

  void OnBtnCancel(wxEvent& evt);
  myThread *thread;
};



class myThread : public wxThread
{
  private:
  char* url;
  char* tempfile;
  void* Entry();
  int result;
  bool delete_gui;
  CHTTPDownloadDlg* myDlg;

  public:

//  myThread::myThread(wxEvtHandler* parent,char* urlname,char* filename);
  ~myThread();

myThread::myThread(wxEvtHandler* parent,char* urlname,char* filename):wxThread(wxTHREAD_JOINABLE) {

  url=urlname;
  tempfile=filename;
  result=1;
  myDlg=(CHTTPDownloadDlg*)parent;
  delete_gui=true;

};

  bool getDeleteGUI();
  void setDeleteGUI(bool);
  
  void OnExit();

};

#endif // HTTPDOWNLOADDLG_H
