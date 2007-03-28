//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) created by Ornis
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/colordlg.h>

#ifdef __WXMSW__
	#include <io.h> // Do_not_auto_remove
#endif

#include "CatDialog.h"			// Interface declarations.
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchDlg.h"			// Needed for UpdateCatChoice
#include <common/StringFunctions.h>	// Needed for MakeFoldername
#include "OtherFunctions.h"		// Needed for CastChild
#include "Preferences.h"		// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "muuli_wdr.h"			// Needed for CategoriesEditWindow


BEGIN_EVENT_TABLE(CCatDialog,wxDialog)
	EVT_BUTTON(wxID_OK,		CCatDialog::OnBnClickedOk)
	EVT_BUTTON(IDC_CATCOLOR,	CCatDialog::OnBnClickColor)
	EVT_BUTTON(IDC_BROWSE,		CCatDialog::OnBnClickedBrowse)
END_EVENT_TABLE()

/*
 * This class is used in both amule and amulegui. It cannot go into
 * libmuleappgui because of preferences. When the compiler must passes
 * here on the remote client, the variable glob_prefs has two different
 * types, in one case it is (CPreferences *), on the other case it is
 * (CPreferencesRem *). A proper fix involves a class hierarchy redesign.
 */
CCatDialog::CCatDialog(wxWindow* parent, bool allowbrowse, int index)
:
wxDialog(parent, -1, _("Category"),
	wxDefaultPosition, wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	wxSizer* content = CategoriesEditWindow(this, true);
	content->Show(this, true);
	Center();
	m_category = NULL;

	// Attempt to get the specified category, this may or may not succeed,
	// we dont really care. If it fails (too high index or such), then we
	// simply get NULL and create a new category
	if (index > -1) {
		m_category = theApp->glob_prefs->GetCategory(index);
	}
	
	if (m_category) {
		// Filling values by the specified category
		CastChild(IDC_TITLE,	wxTextCtrl)->SetValue(m_category->title);
		CastChild(IDC_INCOMING,	wxTextCtrl)->SetValue(m_category->incomingpath);
		CastChild(IDC_COMMENT,	wxTextCtrl)->SetValue(m_category->comment);
		CastChild(IDC_PRIOCOMBO,wxChoice)->SetSelection(m_category->prio);
		
		m_color = m_category->color;
	} else {
		// Default values for new categories
		CastChild(IDC_TITLE,	wxTextCtrl)->SetValue(_("New Category"));
		CastChild(IDC_INCOMING,	wxTextCtrl)->SetValue(thePrefs::GetIncomingDir());
		CastChild(IDC_COMMENT,	wxTextCtrl)->SetValue(wxEmptyString);
		CastChild(IDC_PRIOCOMBO,wxChoice)->SetSelection(0);
		
		m_color = RGB(rand() % 255, rand() % 255, rand() % 255);
	}
	
	CastChild(ID_BOX_CATCOLOR, wxStaticBitmap)->
		SetBitmap(MakeBitmap(WxColourFromCr(m_color)));
	
	if (!allowbrowse) {
		CastChild(IDC_BROWSE, wxButton)->Destroy();
	}
}


CCatDialog::~CCatDialog()
{
}


wxBitmap CCatDialog::MakeBitmap(wxColour colour)
{
	wxBitmap bitmap(16, 16);

	// The DC is marked static to avoid a memory-leak which occurs
	// when GTK2 is used with wxWidgets versions before 2.6.2.
	static wxMemoryDC dc;
	dc.SelectObject(bitmap);

	dc.SetBrush(wxBrush(colour, wxSOLID));
	dc.DrawRectangle(0, 0, 16, 16);
	
	// A valid brush must be set, otherwise calls to SetObject will
	// fail once wxNullBitmap has been selected.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SelectObject(wxNullBitmap);

	return bitmap;
}


void CCatDialog::OnBnClickedBrowse(wxCommandEvent& WXUNUSED(evt))
{	
	wxString dir = CastChild(IDC_INCOMING, wxTextCtrl)->GetValue();
	
	dir = wxDirSelector( _("Choose a folder for incoming files"), dir);
	if (!dir.IsEmpty()) {
		CastChild(IDC_INCOMING, wxTextCtrl)->SetValue(dir);
	}
}


void CCatDialog::OnBnClickedOk(wxCommandEvent& WXUNUSED(evt))
{
	wxString newname = CastChild(IDC_TITLE, wxTextCtrl)->GetValue();
	
	// No empty names
	if (newname.IsEmpty()) {
		wxMessageBox(
			_("You must specify a name for the category!"),
			_("Info"), wxOK, this);
		return;
	}
	
	wxString newpath =
		MakeFoldername(CastChild(IDC_INCOMING, wxTextCtrl)->GetValue());
	
	// No empty dirs please 
	if ( newpath.IsEmpty() ) {
		wxMessageBox(
			_("You must specify a path for the category!"),
			_("Info"), wxOK, this);
		
		return;
	}

	if (!::wxDirExists(newpath)) {
		if (!wxMkdir(newpath)) {
			wxMessageBox(_(
				"Failed to create incoming dir for category. "
				"Please specify a valid path!"),
				_("Info"), wxOK, this);
			return;
		}
	}
	
	// Check if we are using an existing category, and if we are, if it has
	// been removed in the mean-while. Otherwise create new category.
	// lfroen: The only place where it could happen, is removing category
	// from remote gui, while local gui amule have dialog opened in this
	// category.
	int index = -1;
	if (m_category) {
		// Check if the original category still exists
		bool found = false;
		for (uint32 i = 0; i < theApp->glob_prefs->GetCatCount(); ++i) {
			if (m_category == theApp->glob_prefs->GetCategory(i)) {
				found = true;
				index = i;
				break;
			}
		}
		if (!found) {
			m_category = 0;
		}
	}

	if (!m_category) {
		// New category, or the old one is gone
		m_category = theApp->glob_prefs->CreateCategory(
			newname, newpath, 
			CastChild(IDC_COMMENT, wxTextCtrl)->GetValue(),
			m_color,
 			CastChild(IDC_PRIOCOMBO, wxChoice)->GetSelection());
        	
		theApp->amuledlg->m_transferwnd->AddCategory(m_category);
	} else {
		wxString oldpath = m_category->incomingpath;

		theApp->glob_prefs->UpdateCategory(index, newname, newpath, 
        	CastChild(IDC_COMMENT, wxTextCtrl)->GetValue(), m_color,
        	CastChild(IDC_PRIOCOMBO, wxChoice)->GetSelection());

		theApp->amuledlg->m_transferwnd->UpdateCategory(index);
		theApp->amuledlg->m_transferwnd->downloadlistctrl->Refresh();
		theApp->amuledlg->m_searchwnd->UpdateCatChoice();
	}
	
	EndModal(wxID_OK);
}


void CCatDialog::OnBnClickColor(wxCommandEvent& WXUNUSED(evt))
{
	wxColour newcol = wxGetColourFromUser(this, WxColourFromCr(m_color));
	if (newcol.Ok()) {
		m_color = CrFromWxColour(newcol);
		CastChild(ID_BOX_CATCOLOR, wxStaticBitmap)->
			SetBitmap(MakeBitmap(WxColourFromCr(m_color)));
	}
}
// File_checked_for_headers
