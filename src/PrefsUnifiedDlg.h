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

#ifdef __GNUG__
    #pragma interface "PrefsUnifiedDlg.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#define UNIFIED_PREF_HANDLING
//#define DISABLE_OLDPREFS

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog
#include "muuli_wdr.h"
#include "ini2.h"
#include "Preferences.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl

// WDR: class declarations

//----------------------------------------------------------------------------
// PrefsUnifiedDlg
//----------------------------------------------------------------------------

class Rse;
struct Preferences_Struct;

class PrefsUnifiedDlg: public wxDialog
{
	DECLARE_DYNAMIC_CLASS(PrefsUnifiedDlg)
	PrefsUnifiedDlg()  {}
		
public:
    // constructors and destructors
	PrefsUnifiedDlg(wxWindow* parent);
	virtual ~PrefsUnifiedDlg();
    
    // WDR: method declarations for PrefsUnifiedDlg
    virtual bool Validate();
    virtual bool TransferDataToWindow();
    virtual bool TransferDataFromWindow();

	static void ForceUlDlRateCorrelation(int id);
	static void CheckRateUnlimited(Rse* prse);

	static void BuildItemList(Preferences_Struct *prefs, char * appdir);
	static void LoadAllItems(CIni& ini);
	static void SaveAllItems(CIni& ini);

	Rse	*Prse(int id);	// returns the Rse* corresponding to an item ID

	int GetColorIndex()  { return pchoiceColor->GetSelection(); }
	void FixUDPStatus(bool enable_status) ;
    
private:
    // WDR: member variable declarations for PrefsUnifiedDlg
	int	idMin;	// lowest dlg item ID
	int	idMax;	// highest
	Rse	**trse;	// pointer to table of Rse's from idMin to idMax
	CDirectoryTreeCtrl* pdtcShareSelector;
	wxChoice*	pchoiceColor;
	wxButton*	pbuttonColor;

private:
    // WDR: handler declarations for PrefsUnifiedDlg
	void OnOk(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnButtonBrowseWav(wxEvent &event);
	void OnButtonBrowseVideoplayer(wxEvent &event);
	void OnButtonWizard(wxEvent &event);
	void OnButtonDir(wxEvent& event);
	void OnButtonSystray(wxEvent& event);
	void OnButtonEditAddr(wxEvent& event);
	void OnButtonColorChange(wxCommandEvent &event);
	void OnButtonIPFilterReload(wxCommandEvent &event);
	void OnSpinMaxDLR(wxCommandEvent &event);
	void OnColorCategorySelected(wxCommandEvent &event);
	void OnCheckBoxChange(wxEvent &event);
	void OnScroll(wxCommandEvent &event);
	
private:
    DECLARE_EVENT_TABLE()
};




#endif
