//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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


#ifndef SERVERWND_H
#define SERVERWND_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/html/htmlwin.h>	// Needed for wxHtmlWindow
#include <wx/textctrl.h>	// Needed for wxTextCtrl
#include <wx/statbmp.h>		// Needed for wxStaticBitmap

#include "resource.h"		// Needed for IDD_SERVER

class CServerListCtrl;

class CServerWnd : public wxPanel //CResizableDialog
{
  //DECLARE_DYNAMIC(CServerWnd)
  DECLARE_DYNAMIC_CLASS(CServerWnd)
    
    CServerWnd() {};

public:
	CServerWnd(wxWindow* pParent);   // standard constructor
	virtual ~CServerWnd();
	void Localize();
	void UpdateServerMetFromURL(wxString strURL);
	void UpdateMyInfo();

	//wxSizer* Create(wxWindow* parent);
// Dialog Data
	enum { IDD = IDD_SERVER };
	CServerListCtrl* serverlistctrl;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//virtual bool OnInitDialog();
	//DECLARE_MESSAGE_MAP()
	void OnSashPositionChanged();
public:
	//CHyperTextCtrl servermsgbox;
	wxHtmlWindow servermsgbox;
	wxTextCtrl logbox;
	void OnBnClickedAddserver(wxEvent& evt);
	void OnBnClickedUpdateservermetfromurl(wxEvent& evt);
	//afx_msg void OnBnClickedAddserver();
	//afx_msg void OnBnClickedUpdateservermetfromurl();
	void OnBnClickedResetLog(wxEvent& evt);
private:
	wxStaticBitmap m_ctrlNewServerFrm;
	wxStaticBitmap m_ctrlUpdateServerFrm;

	DECLARE_EVENT_TABLE()
};

#endif // SERVERWND_H
