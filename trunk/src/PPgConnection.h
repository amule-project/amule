// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef PPGCONNECTION_H
#define PPGCONNECTION_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_PPG_CONNECTION

class CPreferences;

// CPPgConnection dialog

class CPPgConnection : public wxPanel //CPropertyPage
{
  //DECLARE_DYNAMIC(CPPgConnection)
  DECLARE_DYNAMIC_CLASS(CPPgConnection)
    CPPgConnection() {};

public:
	CPPgConnection(wxWindow* parent);
	virtual ~CPPgConnection();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_CONNECTION };
protected:
	CPreferences *app_prefs;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()
public:
	//virtual bool OnInitDialog();
public:
	void LoadSettings(void);
	void Localize(void);
public:
	void OnApply();
#if 0
	virtual bool OnApply();
	afx_msg void OnEnChangeDownloadCap()			{ SetModified(); }
	afx_msg void OnEnChangeUploadCap()				{ SetModified(); }
	afx_msg void OnEnChangeMaxdown()				{ SetModified(); }
	afx_msg void OnEnChangeMaxup()					{ SetModified(); }
	afx_msg void OnEnChangePort()					{ SetModified(); }
	afx_msg void OnEnChangeMaxcon()					{ SetModified(); }
	afx_msg void OnEnChangeMaxsourceperfile()		{ SetModified(); }
	afx_msg void OnEnChangeMaxsourceperfilesoft()	{ SetModified(); }
	afx_msg void OnEnChangeMaxsourceperfileudp()	{ SetModified(); }
	afx_msg void OnBnClickedAutoconnect()			{ SetModified(); }
	afx_msg void OnBnClickedReconn()				{ SetModified(); }
#endif
	void OnUDPDISABLEChecked(wxEvent& evt);
};

#endif // PPGCONNECTION_H
