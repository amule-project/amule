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

	
//----------------------------------------------------------------------------
// PrefsUnifiedDlg
//----------------------------------------------------------------------------

class Rse;
class CPreferences;
class CDirectoryTreeCtrl;
class wxChoice;

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

	static void BuildItemList(CPreferences *prefs, const wxString& appdir);
	static void LoadAllItems(wxConfigBase& ini);
	static void SaveAllItems(wxConfigBase& ini);

	Rse	*Prse(int id);	// returns the Rse* corresponding to an item ID

	int GetColorIndex()  { return pchoiceColor->GetSelection(); }
	    
private:
    // WDR: member variable declarations for PrefsUnifiedDlg
	int	idMin;	// lowest dlg item ID
	int	idMax;	// highest
	Rse	**trse;	// pointer to table of Rse's from idMin to idMax
	CDirectoryTreeCtrl* pdtcShareSelector;
	wxChoice*	pchoiceColor;
	wxButton*	pbuttonColor;
	wxPanel*		CurrentPrefsPanel;
	wxPanel*		PrefsPanels[11];
private:
    // WDR: handler declarations for PrefsUnifiedDlg
	void OnOk(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnButtonBrowseWav(wxCommandEvent &event);
	void OnButtonBrowseSkin(wxCommandEvent &event);
	void OnButtonBrowseVideoplayer(wxCommandEvent &event);
	void OnButtonWizard(wxCommandEvent &event);
	void OnButtonDir(wxCommandEvent& event);
	void OnButtonSystray(wxCommandEvent& event);
	void OnButtonEditAddr(wxCommandEvent& event);
	void OnButtonColorChange(wxCommandEvent &event);
	void OnButtonIPFilterReload(wxCommandEvent &event);
	void OnSpinMaxDLR(wxSpinEvent &event);
	void OnColorCategorySelected(wxCommandEvent &event);
	void OnCheckBoxChange(wxCommandEvent &event);
	void OnFakeBrowserChange(wxCommandEvent &event);
	void OnScroll(wxScrollEvent &event);
	void OnPrefsPageChange(wxListEvent& event);

private:
    DECLARE_EVENT_TABLE()
};




#endif
