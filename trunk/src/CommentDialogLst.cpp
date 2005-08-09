//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "CommentDialogLst.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/dialog.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include "CommentDialogLst.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for commentLstDlg
#include "OtherFunctions.h"	// Needed for GetRateString
#include "PartFile.h"		// Needed for CPartFile
#include "OPCodes.h"		// Needed for SOURCESSLOTS
#include "updownclient.h"	// Needed for CUpDownClient
#include "Format.h"


CCommentDialogLst::CCommentDialogLst(wxWindow*parent,CPartFile* file)
: wxDialog(parent,-1,_("File Comments"),
wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	m_file = file;
	wxSizer* content=commentLstDlg(this,TRUE);
	content->Show(this,TRUE);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME));
	pmyListCtrl = CastChild( IDC_LST, wxListCtrl );
	OnInitDialog();
}

CCommentDialogLst::~CCommentDialogLst()
{
}

BEGIN_EVENT_TABLE(CCommentDialogLst,wxDialog)
	EVT_BUTTON(IDCOK,CCommentDialogLst::OnBnClickedApply)
	EVT_BUTTON(IDCREF,CCommentDialogLst::OnBnClickedRefresh)
END_EVENT_TABLE()

void CCommentDialogLst::OnBnClickedApply(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}

void CCommentDialogLst::OnBnClickedRefresh(wxCommandEvent& WXUNUSED(evt))
{
	CompleteList();
}

#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

bool CCommentDialogLst::OnInitDialog()
{
	pmyListCtrl->InsertColumn(0, _("Username"), LVCFMT_LEFT, 130);
	pmyListCtrl->InsertColumn(1, _("File Name"), LVCFMT_LEFT, 130);
	pmyListCtrl->InsertColumn(2, _("Rating"), LVCFMT_LEFT, 80);
	pmyListCtrl->InsertColumn(3, _("Comment"), LVCFMT_LEFT, 340);
	CompleteList();
	return TRUE;
}

void CCommentDialogLst::CompleteList()
{
	int count=0;
	pmyListCtrl->DeleteAllItems();
  
	CPartFile::SourceSet::iterator it = m_file->m_SrcList.begin();
	for ( ; it != m_file->m_SrcList.end(); ++it ) {
		CUpDownClient *cur_src = *it;

		if (cur_src->GetFileComment().Length()>0 || cur_src->GetFileRating()>0) {
			pmyListCtrl->InsertItem(count, cur_src->GetUserName());
			pmyListCtrl->SetItem(count, 1, cur_src->GetClientFilename());
			pmyListCtrl->SetItem(count, 2, GetRateString(cur_src->GetFileRating()));
			pmyListCtrl->SetItem(count, 3, cur_src->GetFileComment());
			count++;
		}
	}
	wxString info;
	if (count==0) {
		info = _("No comments");
	} else {
		info = CFormat( _("%s comment(s)")) % CastItoIShort(count);
	}
	FindWindow(IDC_CMSTATUS)->SetLabel(info);
	FindWindow(IDC_CMSTATUS)->GetParent()->Layout();
	m_file->UpdateFileRatingCommentAvail();
}
