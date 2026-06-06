//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#include "CommentDialog.h"	// Interface declarations
#include "GuiEvents.h"
#include "KnownFile.h"		// Needed for CKnownFile
#include "muuli_wdr.h"		// Needed for commentDlg

#include <set>

// CommentDialog dialog

namespace {
// Registry of open CCommentDialog instances. Iterated by
// CCommentDialog::DropReferencesTo when a CKnownFile is destroyed, so
// any dialog whose m_file matches can be dismissed before its modal
// loop tries to deref the freed pointer (issue #755 / #748 family).
//
// All access is on the GUI main thread (modal dialogs only exist on
// that thread), so no synchronisation is needed.
std::set<CCommentDialog*>& OpenInstances()
{
	static std::set<CCommentDialog*> instances;
	return instances;
}
} // namespace

//IMPLEMENT_DYNAMIC(CCommentDialog, CDialog)
CCommentDialog::CCommentDialog(wxWindow* parent,CKnownFile* file)
: wxDialog(parent,-1,_("File Comments"),
wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	m_file = file;
	wxSizer* content=commentDlg(this,TRUE);
	content->SetSizeHints(this);
	content->Show(this,TRUE);
	Center();
	ratebox = CastChild( IDC_RATELIST, wxChoice );
	OnInitDialog();
	OpenInstances().insert(this);
}

CCommentDialog::~CCommentDialog()
{
	OpenInstances().erase(this);
}

void CCommentDialog::DropReferencesTo(const CKnownFile* file)
{
	// Pointer-value comparison only — `file` may already be freed.
	for (CCommentDialog* d : OpenInstances()) {
		if (d->m_file == file) {
			d->m_file = NULL;
			// Self-dismiss: the dialog is showing data that no
			// longer exists on disk. Returning 0 matches Cancel.
			d->EndModal(0);
		}
	}
}

wxBEGIN_EVENT_TABLE(CCommentDialog,wxDialog)
	EVT_TEXT_ENTER(IDC_CMT_TEXT, CCommentDialog::OnBnClickedApply)
	EVT_BUTTON(IDCOK, CCommentDialog::OnBnClickedApply)
	EVT_BUTTON(IDC_FC_CLEAR, CCommentDialog::OnBnClickedClear)
	EVT_BUTTON(IDCCANCEL, CCommentDialog::OnBnClickedCancel)
wxEND_EVENT_TABLE()

void CCommentDialog::OnBnClickedApply(wxCommandEvent& WXUNUSED(evt))
{
	wxString comment = CastChild( IDC_CMT_TEXT, wxTextCtrl )->GetValue();
	CoreNotify_KnownFile_Comment_Set(m_file, comment, (int8)ratebox->GetSelection());
	EndModal(0);
}

void CCommentDialog::OnBnClickedClear(wxCommandEvent& WXUNUSED(evt))
{
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetValue("");
}

void CCommentDialog::OnBnClickedCancel(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}

bool CCommentDialog::OnInitDialog()
{
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetValue(m_file->GetFileComment());
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetMaxLength(MAXFILECOMMENTLEN);
	ratebox->SetSelection(m_file->GetFileRating());
	return TRUE;
}
// File_checked_for_headers
