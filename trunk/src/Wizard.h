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

#ifndef WIZARD_H
#define WIZARD_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/colour.h>		// Needed before wx/listctrl.h
#include <wx/event.h>		// Needed before wx/listctrl.h
#include <wx/listctrl.h>	// Needed for wxListEvent and wxListCtrl

#include <wx/dialog.h>		// Needed for wxDialog

#include "resource.h"		// Needed for IDD_WIZARD

class CPreferences;

// Wizard dialog

class Wizard : public wxDialog
{
  //DECLARE_DYNAMIC(Wizard)

public:
	Wizard(wxWindow* pParent = NULL);   // standard constructor
	virtual ~Wizard();
	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }
	void Localize();
	virtual bool OnInitDialog();

	int m_iOS;
	int m_iTotalDownload;

// Dialog Data
	enum { IDD = IDD_WIZARD };
protected:
	CPreferences* app_prefs;
protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()
public:
	void OnBnClickedApply(wxEvent& evt);
	void OnBnClickedCancel(wxEvent& evt);
	void OnBnClickedWizRadioOsNtxp(wxEvent& evt);
	void OnBnClickedWizRadioUs98me(wxEvent& evt);
	void OnBnClickedWizLowdownloadRadio(wxEvent& evt);
	void OnBnClickedWizMediumdownloadRadio(wxEvent& evt);
	void OnBnClickedWizHighdownloadRadio(wxEvent& evt);
	void OnBnClickedWizResetButton(wxEvent& evt);
protected:
	//CListCtrl m_provider;
	wxListCtrl* m_provider;
private:
	void SetCustomItemsActivation();
public:
	//afx_msg void OnNMClickProviders(NMHDR *pNMHDR, LRESULT *pResult);
	void OnNMClickProviders(wxListEvent& evt);

};

#endif // WIZARD_H
