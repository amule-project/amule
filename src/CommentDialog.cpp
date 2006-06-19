//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "KnownFile.h"		// Needed for CKnownFile
#include "muuli_wdr.h"		// Needed for commentDlg
// CommentDialog dialog 

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
}

CCommentDialog::~CCommentDialog()
{
}

BEGIN_EVENT_TABLE(CCommentDialog,wxDialog)
	EVT_TEXT_ENTER(IDC_CMT_TEXT, CCommentDialog::OnBnClickedApply)
	EVT_BUTTON(IDCOK, CCommentDialog::OnBnClickedApply)
	EVT_BUTTON(IDC_FC_CLEAR, CCommentDialog::OnBnClickedClear)
	EVT_BUTTON(IDCCANCEL, CCommentDialog::OnBnClickedCancel)
END_EVENT_TABLE()

void CCommentDialog::OnBnClickedApply(wxCommandEvent& WXUNUSED(evt))
{
	wxString SValue;
	SValue = CastChild( IDC_CMT_TEXT, wxTextCtrl )->GetValue();
	m_file->SetFileComment(SValue);
	m_file->SetFileRating((int8)ratebox->GetSelection());
	EndModal(0);
}

void CCommentDialog::OnBnClickedClear(wxCommandEvent& WXUNUSED(evt))
{
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetValue(wxEmptyString);
} 

void CCommentDialog::OnBnClickedCancel(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
} 

bool CCommentDialog::OnInitDialog()
{
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetValue(m_file->GetFileComment());
	CastChild(IDC_CMT_TEXT, wxTextCtrl)->SetMaxLength(50);
	ratebox->SetSelection(m_file->GetFileRating());
	return TRUE;
}
// File_checked_for_headers
