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

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/settings.h>

#include "CommentDialog.h"	// Interface declarations
#include "KnownFile.h"		// Needed for CKnownFile
#include "CString.h"	// Needed for CString
#include "muuli_wdr.h"		// Needed for commentDlg

// CommentDialog dialog 

//IMPLEMENT_DYNAMIC(CCommentDialog, CDialog)
CCommentDialog::CCommentDialog(wxWindow* parent,CKnownFile* file) 
: wxDialog(parent,CCommentDialog::IDD,_("File Comments"),
wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	m_file = file;
	wxSizer* content=commentDlg(this,TRUE);
	content->SetSizeHints(this);
	content->Show(this,TRUE);
	Center();
	ratebox=((wxChoice*)FindWindowById(IDC_RATELIST));
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

void CCommentDialog::OnBnClickedApply(wxCommandEvent& evt)
{
	wxString SValue;
	SValue=((wxTextCtrl*)FindWindowById(IDC_CMT_TEXT))->GetValue();
	m_file->SetFileComment(CString(SValue));
	m_file->SetFileRate((int8)ratebox->GetSelection());
	EndModal(0);
}

void CCommentDialog::OnBnClickedClear(wxCommandEvent& evt)
{
	((wxTextCtrl*)FindWindowById(IDC_CMT_TEXT))->SetValue(wxT(""));
} 

void CCommentDialog::OnBnClickedCancel(wxCommandEvent& evt)
{
	EndModal(0);
} 

bool CCommentDialog::OnInitDialog()
{
	((wxTextCtrl*)FindWindowById(IDC_CMT_TEXT))->SetValue(m_file->GetFileComment());
	((wxTextCtrl*)FindWindowById(IDC_CMT_TEXT))->SetMaxLength(50);
	ratebox->SetSelection(m_file->GetFileRate());
	return TRUE;
}
