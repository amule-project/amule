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

#ifndef PPGGENERAL_H
#define PPGGENERAL_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/combobox.h>	// Needed for wxComboBox

#include "resource.h"		// Needed for IDD_PPG_GENERAL

class CPreferences;

class CPPgGeneral : public wxPanel //CPropertyPage
{
  //DECLARE_DYNAMIC(CPPgGeneral)
  DECLARE_DYNAMIC_CLASS(CPPgGeneral)
    CPPgGeneral() {};

public:
	CPPgGeneral(wxWindow* parent);
	virtual ~CPPgGeneral();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_GENERAL };
protected:
	CPreferences *app_prefs;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//DECLARE_MESSAGE_MAP()
	void OnHScroll(wxScrollEvent& evt);
	DECLARE_EVENT_TABLE()
public:
	//virtual bool OnInitDialog();
 public:
	void LoadSettings(void);
public:
	void OnApply();
#if 0
	virtual bool OnApply();
	afx_msg void OnEnChangeNick()					{ SetModified(); }
	afx_msg void OnCbnSelchangeLangs()				{ SetModified(); }
	afx_msg void OnBnClickedFlat()					{ SetModified(); }
	afx_msg void OnBnClickedMintray()				{ SetModified(); }
	afx_msg void OnBnClickedBeeper()				{ SetModified(); }
	afx_msg void OnBnClickedExit()					{ SetModified(); }
	afx_msg void OnBnClickedSplashon()				{ SetModified(); }
	afx_msg void OnBnClickedDblclick()				{ SetModified(); }
	afx_msg void OnBnClickedBringtoforeground()		{ SetModified(); }
	afx_msg void OnBnClickedNotify()				{ SetModified(); }
	afx_msg void OnEnChangeTooltipdelay()			{ SetModified(); }
	afx_msg void OnBnClickedOnlinesig()				{ SetModified(); }
	afx_msg void OnBnClickedEd2kfix();
#endif
	void OnDesktopmode(wxEvent& evt);
protected:
	//wxComboBox m_language;
public:
	void Localize(void);
};

#endif // PPGGENERAL_H
