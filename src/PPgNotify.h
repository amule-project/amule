// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef PPGNOTIFY_H
#define PPGNOTIFY_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_PPG_NOTIFY

class CPreferences;

// finestra di dialogo CPPgNotify

class CPPgNotify : public wxPanel // CPropertyPage
{
  //DECLARE_DYNAMIC(CPPgNotify)
  DECLARE_DYNAMIC_CLASS(CPPgNotify)
    CPPgNotify() {};

public:	
	CPPgNotify(wxWindow* parent);
	virtual ~CPPgNotify();	
	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

	//virtual bool OnInitDialog();
	virtual bool OnApply();
	void LoadSettings(void);

// Dialog Data
	enum { IDD = IDD_PPG_NOTIFY };

protected:
	CPreferences* app_prefs;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // Supporto DDX/DDV
	wxString CPPgNotify::DialogBrowseFile(wxString Filters, wxString DefaultFileName="");

	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()
private:
public:
	void Localize(void);
#if 0
	afx_msg void OnBnClickedCbTbnUsesound()     { SetModified(); };
	afx_msg void OnBnClickedCbTbnOnlog()        { SetModified(); };	
	afx_msg void OnBnClickedCbTbnPopAlways()    { SetModified(); };
	afx_msg void OnBnClickedCbTbnOndownload()   { SetModified(); };
	afx_msg void OnBnClickedCbTbnOnchat();
	afx_msg void OnBnClickedBtnBrowseWav();
#endif
	void OnBnClickedBtnBrowseWav(wxEvent& evt);
};

#endif // PPGNOTIFY_H
