// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef PPGSERVER_H
#define PPGSERVER_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_PPG_SERVER

class CPreferences;

// CPPgServer dialog

class CPPgServer : public wxPanel //CPropertyPage
{
  //DECLARE_DYNAMIC(CPPgServer)
  DECLARE_DYNAMIC_CLASS(CPPgServer)
    CPPgServer() {};

public:
	CPPgServer(wxWindow* parent);
	virtual ~CPPgServer();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_SERVER };
protected:
	CPreferences *app_prefs;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_EVENT_TABLE()
	//DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedCheck1();
	//virtual bool OnInitDialog();
 public:
	void LoadSettings(void);
public:
	void OnApply();
#if 0
	virtual bool OnApply();

	afx_msg void OnEnChangeServerretries()			{ SetModified(); }
	afx_msg void OnBnClickedRemovedead()			{ SetModified(); }
	afx_msg void OnBnClickedAutoserver()			{ SetModified(); }
	afx_msg void OnBnClickedUpdateserverconnect()	{ SetModified(); }
	afx_msg void OnBnClickedUpdateserverclient()	{ SetModified(); }
	afx_msg void OnBnClickedFilter()				{ SetModified(); }
	afx_msg void OnBnClickedScore()					{ SetModified(); }
#endif
	void Localize(void);
	void OnBnClickedEditadr(wxEvent& evt);
};

#endif // PPGSERVER_H
