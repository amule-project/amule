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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "CommentDialogLst.h"	// Interface declarations
#include "muuli_wdr.h"			// Needed for commentLstDlg
#include "OtherFunctions.h"		// Needed for GetRateString
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"		// Needed for CUpDownClient
#include <common/Format.h>				// Needed for CFormat
#include "MuleListCtrl.h"		// Needed for CMuleListCtrl
#include "amule.h"				// Needed for theApp
#include "DownloadQueue.h"		// Needed for CDownloadQueue

#include <wx/sizer.h>			// Needed for wxSizer


BEGIN_EVENT_TABLE(CCommentDialogLst,wxDialog)
	EVT_BUTTON(IDCOK,CCommentDialogLst::OnBnClickedApply)
	EVT_BUTTON(IDCREF,CCommentDialogLst::OnBnClickedRefresh)
END_EVENT_TABLE()


struct SFileRating
{
	wxString UserName;
	wxString FileName;
	sint16   Rating;
	wxString Comment;
};


CCommentDialogLst::CCommentDialogLst(wxWindow*parent, CPartFile* file)
	: wxDialog(parent, -1, wxString(_("File Comments")), 
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_file = file;
	
	wxSizer* content = commentLstDlg(this, true);
	content->Show(this, true);

	m_list = CastChild(IDC_LST, CMuleListCtrl);
	m_list->InsertColumn(0, _("Username"), wxLIST_FORMAT_LEFT, 130);
	m_list->InsertColumn(1, _("File Name"), wxLIST_FORMAT_LEFT, 130);
	m_list->InsertColumn(2, _("Rating"), wxLIST_FORMAT_LEFT, 80);
	m_list->InsertColumn(3, _("Comment"), wxLIST_FORMAT_LEFT, 340);
	m_list->SetSortFunc(SortProc);
	
	UpdateList();
}


CCommentDialogLst::~CCommentDialogLst()
{
	ClearList();
}


void CCommentDialogLst::OnBnClickedApply(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}


void CCommentDialogLst::OnBnClickedRefresh(wxCommandEvent& WXUNUSED(evt))
{
	UpdateList();
}


void CCommentDialogLst::UpdateList()
{
	int count = 0;
	ClearList();
 
	CPartFile::SourceSet::iterator it = m_file->m_SrcList.begin();
	for ( ; it != m_file->m_SrcList.end(); ++it ) {
		CUpDownClient *cur_src = *it;

		if (cur_src->GetFileComment().Length()>0 || cur_src->GetFileRating()>0) {
			SFileRating* entry = new SFileRating();
			entry->UserName = cur_src->GetUserName();
			entry->FileName = cur_src->GetClientFilename();
			entry->Rating   = cur_src->GetFileRating();
			entry->Comment  = cur_src->GetFileComment();
			
			m_list->InsertItem(count, cur_src->GetUserName());
			m_list->SetItem(count, 1, cur_src->GetClientFilename());
			m_list->SetItem(count, 2, GetRateString(cur_src->GetFileRating()));
			m_list->SetItem(count, 3, cur_src->GetFileComment());
			m_list->SetItemData(count, (long)entry);
			count++;
		}
	}

	wxString info;
	if (count == 0) {
		info = _("No comments");
	} else {
		info = CFormat( _("%s comment(s)")) % CastItoIShort(count);
	}
	
	FindWindow(IDC_CMSTATUS)->SetLabel(info);
	FindWindow(IDC_CMSTATUS)->GetParent()->Layout();
	
	m_file->UpdateFileRatingCommentAvail();
}


void CCommentDialogLst::ClearList()
{
	size_t count = m_list->GetItemCount();
	for (size_t i = 0; i < count; ++i) {
		delete (SFileRating*)m_list->GetItemData(i);
	}

	m_list->DeleteAllItems();
}


int CCommentDialogLst::SortProc(long item1, long item2, long sortData)
{
	SFileRating* file1 = (SFileRating*)item1;
	SFileRating* file2 = (SFileRating*)item2;
	
	int mod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	
	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
		case 0:		return mod * file1->UserName.Cmp(file2->UserName);
		case 1:		return mod * file1->FileName.Cmp(file2->FileName);
		case 2:		return mod * (file1->Rating - file2->Rating);
		case 3:		return mod * file1->Comment.Cmp(file2->Comment);
		default:
			return 0;
	}
}

