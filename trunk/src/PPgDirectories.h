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

#ifndef PPGDIRECTORIES_H
#define PPGDIRECTORIES_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_PPG_DIRECTORIES

class CPreferences;
class CDirectoryTreeCtrl;

// CPPgDirectories dialog

class CPPgDirectories : public wxPanel //CPropertyPage
{
  //DECLARE_DYNAMIC(CPPgDirectories)
  DECLARE_DYNAMIC_CLASS(CPPgDirectories)
    CPPgDirectories() {};

public:
	CPPgDirectories(wxWindow* parent);									// standard constructor
	virtual ~CPPgDirectories();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_DIRECTORIES };

protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()

public:
	//virtual bool OnInitDialog();
public:
	CDirectoryTreeCtrl* m_ShareSelector;
	CPreferences* app_prefs;
private:
	bool SelectDir(char* outdir, char* titletext);
public:
	void OnBnClickedSelincdir(wxEvent& e);
	void OnBnClickedSeltempdir(wxEvent& e);
	void BrowseVideoplayer(wxEvent& e);

	virtual bool OnApply();
#if 0

	afx_msg void OnBnClickedSelincdir();
	afx_msg void OnBnClickedSeltempdir();
	
	afx_msg void OnEnChangeIncfiles()	{ SetModified(); }
	afx_msg void OnEnChangeTempfiles()	{ SetModified(); }
	afx_msg void OnBnClickedCheck1()	{ SetModified(); }
protected:
	virtual bool OnCommand(WPARAM wParam, LPARAM lParam);
#endif
public:
	void Localize(void);
	void LoadSettings(void);
};

#endif // PPGDIRECTORIES_H
