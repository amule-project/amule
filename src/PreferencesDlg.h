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

#ifndef PREFERENCESDLG_H
#define PREFERENCESDLG_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog

class CPreferences;
class CPPgGeneral;
class CPPgConnection;
class CPPgServer;
class CPPgDirectories;
class CPPgFiles;
class CPPgStats;
class CPPgNotify;
class CPPgTweaks;
class CPPgSourcesDropping;
class CPPgGuiTweaks;

// CPreferencesDlg

class CPreferencesDlg : public wxDialog //CPropertySheet
{
  //DECLARE_DYNAMIC(CPreferencesDlg)
  DECLARE_DYNAMIC_CLASS(CPreferencesDlg)

    CPreferencesDlg() {};

public:
	CPreferencesDlg(wxWindow* parent,CPreferences* prefs);
	virtual ~CPreferencesDlg();
	
protected:
	unsigned int m_nActiveWnd;
	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()
public:
	CPPgGeneral*		m_wndGeneral;
	CPPgConnection*	m_wndConnection;
	CPPgServer*		m_wndServer;
	CPPgDirectories*	m_wndDirectories;
	CPPgFiles*		m_wndFiles;
	CPPgStats*		m_wndStats;
	CPPgNotify*		m_wndNotify;
	//CPPgIRC*			m_wndIRC;
	CPPgTweaks*		m_wndTweaks;
	CPPgSourcesDropping*    m_wndSourcesDropping;  // Sources Dropping window
	CPPgGuiTweaks*          m_wndGuiTweaks; // GUI Tweaks window
	CPreferences	*app_prefs;

	virtual int ShowModal();

	void SetPrefs(CPreferences* in_prefs);
	//afx_msg void OnDestroy();
	//virtual bool OnInitDialog();

	void OnBtnOk(wxEvent& evt);
	void OnBtnCancel(wxEvent& evt);
	void OnBtnWizard(wxEvent& evt);

        void Localize(void);
};

#endif // PREFERENCESDLG_H
