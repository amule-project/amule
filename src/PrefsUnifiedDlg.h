// This file is part of the aMule Project
// 
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Original author: Emilio Sandoz
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



#ifndef __PrefsUnifiedDlg_H__
#define __PrefsUnifiedDlg_H__

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/config.h>
#include "muuli_wdr.h"
#include <wx/choice.h>
#include <wx/button.h>

#include <map>
#include <list>


#include "color.h"
//----------------------------------------------------------------------------
// PrefsUnifiedDlg
//----------------------------------------------------------------------------

class Cfg_Base;
class CPreferences;
class CDirectoryTreeCtrl;
//class wxChoice;



const int cntStatColors = 13;


class PrefsUnifiedDlg : public wxDialog
{
public:
    // constructors and destructors
	PrefsUnifiedDlg(wxWindow* parent);
	~PrefsUnifiedDlg();
   
    bool TransferFromWindow();
	bool TransferToWindow();

	static int  GetPrefsID()		{ return s_ID; }

	static void BuildItemList( const wxString& appdir );
	static void LoadAllItems(wxConfigBase* cfg);
	static void SaveAllItems(wxConfigBase* cfg);

protected:
	static void SetPrefsID(int ID)	{ s_ID = ID; }

	//! Contains the ID of the current window or zero if no preferences window has been created.
	static int s_ID;

	//! Temporary storage for statistic-colors.
	static COLORREF	s_colors[cntStatColors];
	//! Reference for checking if the colors has changed.
	static COLORREF	s_colors_ref[cntStatColors];

	typedef std::list<Cfg_Base*>		CFGList;
	typedef std::map<int, Cfg_Base*>	CFGMap;

	static CFGMap	s_CfgList;
	static CFGList	s_MiscList;


	bool			CfgChanged(int ID);
	Cfg_Base*		GetCfg(int id);	
	
	
	CDirectoryTreeCtrl* 	m_ShareSelector;
	wxChoice*				m_choiceColor;
	wxButton*				m_buttonColor;
	wxPanel*				m_CurrentPanel;
	
	
	void OnOk(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	
	void OnButtonBrowseWav(wxCommandEvent &event);
	void OnButtonBrowseSkin(wxCommandEvent &event);
	void OnButtonBrowseVideoplayer(wxCommandEvent &event);
	void OnButtonDir(wxCommandEvent& event);
	void OnButtonSystray(wxCommandEvent& event);
	void OnButtonEditAddr(wxCommandEvent& event);
	void OnButtonColorChange(wxCommandEvent &event);
	void OnButtonIPFilterReload(wxCommandEvent &event);
	void OnColorCategorySelected(wxCommandEvent &event);
	void OnCheckBoxChange(wxCommandEvent &event);
	void OnFakeBrowserChange(wxCommandEvent &event);
	void OnPrefsPageChange(wxListEvent& event);
	void OnToolTipDelayChange(wxSpinEvent& event);

	void OnInitDialog( wxInitDialogEvent& evt );
	
    DECLARE_EVENT_TABLE()
};




#endif
