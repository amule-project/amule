// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

// FileDetailDialog.cpp : implementation file
//

#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW
#include <wx/stattext.h>	// Needed for wxStaticText
#include <wx/sizer.h>
#include <wx/intl.h>
#include <wx/textctrl.h>

#include "muuli_wdr.h"		// Needed for ID_CLOSEWNDFD,...,IDC_RENAME
#include "FileDetailDialog.h"	// Interface declarations
#include "FileDetailListCtrl.h"	// Needed for CFileDetailListCtrl
#include "otherfunctions.h"	// Needed for MakeStringEscaped
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "updownclient.h"	// Needed for CUpDownClient
#include "PartFile.h"		// Needed for CPartFile
#include "color.h"		// Needed for SYSCOLOR

#define ID_MY_TIMER 1652

//IMPLEMENT_DYNAMIC(CFileDetailDialog, CDialog)
BEGIN_EVENT_TABLE(CFileDetailDialog,wxDialog)
	EVT_BUTTON(ID_CLOSEWNDFD, CFileDetailDialog::OnClosewnd)
	EVT_BUTTON(IDC_BUTTONSTRIP, CFileDetailDialog::OnBnClickedButtonStrip)
	EVT_BUTTON(IDC_TAKEOVER, CFileDetailDialog::OnBnClickedTakeOver)
	EVT_LIST_ITEM_ACTIVATED(IDC_LISTCTRLFILENAMES, CFileDetailDialog::OnListClickedTakeOver)
	EVT_BUTTON(IDC_CMTBT, CFileDetailDialog::OnBnClickedShowComment) //for comment
	EVT_BUTTON(IDC_RENAME, CFileDetailDialog::OnBnClickedRename) // Added by Tarod [Juanjo]
	EVT_TIMER(ID_MY_TIMER,CFileDetailDialog::OnTimer)
END_EVENT_TABLE()

CFileDetailDialog::CFileDetailDialog(wxWindow* parent,CPartFile* file)
: wxDialog(parent,9998,(_("File Details")),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE)
{
	m_file = file;
	m_timer.SetOwner(this,ID_MY_TIMER);
	m_timer.Start(5000);
	wxSizer* content=fileDetails(this,TRUE);
	UpdateData();
	content->SetSizeHints(this);
	content->Show(this,TRUE);
	Centre();
}

CFileDetailDialog::~CFileDetailDialog()
{
	m_timer.Stop();
}

void CFileDetailDialog::OnTimer(wxTimerEvent& WXUNUSED(evt)) {
	UpdateData();
}

void CFileDetailDialog::OnClosewnd(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}

void CFileDetailDialog::UpdateData()
{
	wxString bufferS;
	CastChild(IDC_FNAME,wxStaticText)->SetLabel(MakeStringEscaped(TruncateFilename(m_file->GetFileName(),60)));
	CastChild(IDC_METFILE,wxStaticText)->SetLabel(MakeStringEscaped(TruncateFilename(m_file->GetFullName(),60,true)));
	wxString tmp=CastChild(IDC_FILENAME,wxTextCtrl)->GetValue();

	if (tmp.Length()<3) {
		CastChild(IDC_FILENAME,wxTextCtrl)->SetValue(m_file->GetFileName());
	}

	CastChild(IDC_FHASH,wxStaticText)->SetLabel(EncodeBase16(m_file->GetFileHash(), 16));
	CastChild(IDC_FSIZE,wxControl)->SetLabel(CastItoXBytes(m_file->GetFileSize()));
	CastChild(IDC_PFSTATUS,wxControl)->SetLabel(m_file->getPartfileStatus());
	bufferS.Printf(wxT("%i(%i)"),m_file->GetPartCount(),m_file->GetHashCount());
	CastChild(IDC_PARTCOUNT,wxControl)->SetLabel(bufferS);
	CastChild(IDC_TRANSFERED,wxControl)->SetLabel(CastItoXBytes(m_file->GetTransfered()));
	CastChild(IDC_FD_STATS1,wxControl)->SetLabel(CastItoXBytes(m_file->GetLostDueToCorruption()).GetData());
	CastChild(IDC_FD_STATS2,wxControl)->SetLabel(CastItoXBytes(m_file->GetGainDueToCompression()).GetData());
	CastChild(IDC_FD_STATS3,wxControl)->SetLabel(CastItoIShort(m_file->TotalPacketsSavedDueToICH()));
	CastChild(IDC_COMPLSIZE,wxControl)->SetLabel(CastItoXBytes(m_file->GetCompletedSize()));
	bufferS.Printf(wxT("%.2f "),m_file->GetPercentCompleted());
	CastChild(IDC_PROCCOMPL,wxControl)->SetLabel(bufferS+wxT("% ")+wxString(_("done")));
	bufferS.Printf(wxT("%.2f %s"),(float)m_file->GetKBpsDown(),_("kB/s"));
	CastChild(IDC_DATARATE,wxControl)->SetLabel(bufferS);
	bufferS.Printf(wxT("%i"),m_file->GetSourceCount());
	CastChild(IDC_SOURCECOUNT,wxControl)->SetLabel(bufferS);
	bufferS.Printf(wxT("%i"),m_file->GetTransferingSrcCount());
	CastChild(IDC_SOURCECOUNT2,wxControl)->SetLabel(bufferS);
	bufferS.Printf(wxT("%i (%.1f%%)"),m_file->GetAvailablePartCount(),(float) ((m_file->GetAvailablePartCount()*100)/ m_file->GetPartCount()));
	CastChild(IDC_PARTAVAILABLE,wxControl)->SetLabel(bufferS);

	if (m_file->lastseencomplete==0) {
		bufferS = wxString(_("Unknown")).MakeLower();
	} else {
		char tmps[80];
		static char const* lastseencomplete_fmt = "%A, %x, %X";	// Suppress compiler warning.
		strftime(tmps,sizeof(tmps),lastseencomplete_fmt,localtime(&m_file->lastseencomplete));
		bufferS=char2unicode(tmps);//.Format( "%s",m_file->lastseencomplete.Format( "%A, %x, %X"));
	}

	CastChild(IDC_LASTSEENCOMPL,wxControl)->SetLabel(bufferS);
	CastChild(IDC_RENAME,wxControl)->Enable((m_file->GetStatus() == PS_COMPLETE || m_file->GetStatus() == PS_COMPLETING) ? false:true);//add by CML 
	FillSourcenameList();
	Layout();
}

// CFileDetailDialog message handlers

#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

// by Juanjo
void CFileDetailDialog::FillSourcenameList()
{
	CUpDownClient* cur_src; 
	CFileDetailListCtrl* pmyListCtrl; 
	int itempos; 
	wxString nameCountStr; 
	pmyListCtrl = CastChild(IDC_LISTCTRLFILENAMES, CFileDetailListCtrl ); 

// PA: imported new version from emule 0.30e

	// reset
	for (int i=0;i<pmyListCtrl->GetItemCount();i++){
		pmyListCtrl->SetItem(i, 1, wxT("0"));	
		SourcenameItem *item = (SourcenameItem *) pmyListCtrl->GetItemData(i);
		item->count = 0;
	}

	// update
	for ( CPartFile::SourceSet::iterator it = m_file->m_SrcList.begin(); it != m_file->m_SrcList.end(); ++it ) { 
		cur_src = *it; 
		if (cur_src->GetRequestFile()!=m_file || cur_src->GetClientFilename().Length()==0)
			continue;

		if ((itempos=pmyListCtrl->FindItem(-1,cur_src->GetClientFilename())) == -1) { 
			int itemid = pmyListCtrl->InsertItem(0, cur_src->GetClientFilename()); 
			pmyListCtrl->SetItem(0, 1, wxT("1")); 
			SourcenameItem *item = new SourcenameItem;
			item->name = cur_src->GetClientFilename();
			item->count = 1;
			pmyListCtrl->SetItemData(0, (long)item); 
			// background.. argh -- PA: was in old version - do we still need this?
			wxListItem tmpitem;
			tmpitem.m_itemId=itemid;
			tmpitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
			pmyListCtrl->SetItem(tmpitem);
		} else { 
			SourcenameItem *item = (SourcenameItem *) pmyListCtrl->GetItemData(itempos); 
			item->count++;
			nameCountStr.Printf(wxT("%li"), item->count); 
			pmyListCtrl->SetItem(itempos, 1, nameCountStr); 
		} 
	}

	pmyListCtrl->SortList();

	// remove 0'er
	for (int i=0;i<pmyListCtrl->GetItemCount();i++)
	{
		SourcenameItem *item = (SourcenameItem *) pmyListCtrl->GetItemData(i); 
		if (item->count==0)
		{
			delete item;
			pmyListCtrl->DeleteItem(i);
			i--;  // PA: one step back is enough, no need to go back to 0
		}
	}

// PA: end

	Layout();
}
// End by Juanjo


void CFileDetailDialog::OnBnClickedShowComment(wxCommandEvent& WXUNUSED(evt))
{
	CCommentDialogLst* dialog=new CCommentDialogLst(this,m_file);
	dialog->ShowModal();
	delete dialog;
}

void CFileDetailDialog::OnBnClickedRename(wxCommandEvent& WXUNUSED(evt))
{
	wxString NewFileName=CastChild(IDC_FILENAME,wxTextCtrl)->GetValue();
	m_file->SetFileName(NewFileName);
	m_file->SavePartFile(); 
	FindWindow(IDC_FNAME)->SetLabel(MakeStringEscaped(m_file->GetFileName()));
	FindWindow(IDC_METFILE)->SetLabel(m_file->GetFullName());
	
	CastChild( IDC_FILENAME, wxTextCtrl )->SetValue(m_file->GetFileName());
} 

void CFileDetailDialog::OnBnClickedButtonStrip(wxCommandEvent& WXUNUSED(evt)) {
	wxString filename,tempStr;
	char c;
	filename=CastChild(IDC_FILENAME,wxTextCtrl)->GetValue();
	
	// Replace . with - except the last one (extention-dot)
	int extpos=filename.Find('.',TRUE);
	if (extpos>=0) {
		filename.Replace(wxT("."),wxT("-"));
		filename.SetChar(extpos,'.');
	}

	// Replace Space-holders with Spaces
	filename.Replace(wxT("_"),wxT("-"));
	filename.Replace(wxT("%20"),wxT("-"));

	// Barry - Some additional formatting
	filename.MakeLower();
	filename.Replace(wxT("sharereactor"), wxEmptyString);
	filename.Replace(wxT("deviance"), wxEmptyString);
	filename.Replace(wxT("-ftv"), wxEmptyString);
	filename.Replace(wxT("flt"), wxEmptyString);
	filename.Replace(wxT("[]"), wxEmptyString);
	filename.Replace(wxT("()"), wxEmptyString);
	filename.Replace(wxT("  "), wxT("-"));

	// Make leading Caps 
	if (filename.Length()>1)
	{
		tempStr=filename.GetChar(0);
		tempStr.MakeUpper();
		c = tempStr.GetChar(0);
		filename.SetChar(0, c);
		
		for (unsigned int ix = 0; ix < filename.Length() - 1; ++ix)
		{
			if (filename.GetChar(ix) == ' ') 
			{
				tempStr=filename.GetChar(ix+1);
				tempStr.MakeUpper();
				c=tempStr.GetChar(0);
				filename.SetChar(ix+1,c);
			}
		}
	}

	CastChild(IDC_FILENAME,wxTextCtrl)->SetValue(filename);
}

void CFileDetailDialog::OnBnClickedTakeOver(wxCommandEvent& WXUNUSED(evt))
{
	CFileDetailListCtrl* pmyListCtrl;
	pmyListCtrl = CastChild( IDC_LISTCTRLFILENAMES, CFileDetailListCtrl );
	if (pmyListCtrl->GetSelectedItemCount() > 0) {
		long pos=-1;
		for(;;) {
			pos=pmyListCtrl->GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if(pos==-1) {
				break;
			}
			CastChild(IDC_FILENAME,wxTextCtrl)->SetValue(pmyListCtrl->GetItemText(pos));
		}
	}
}

void CFileDetailDialog::OnListClickedTakeOver(wxListEvent& WXUNUSED(evt))
{
	CFileDetailListCtrl* pmyListCtrl;
	pmyListCtrl = CastChild( IDC_LISTCTRLFILENAMES, CFileDetailListCtrl );
	if (pmyListCtrl->GetSelectedItemCount() > 0) {
		long pos=-1;
		for(;;) {
			pos=pmyListCtrl->GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if(pos==-1) {
				break;
			}
			CastChild(IDC_FILENAME,wxTextCtrl)->SetValue(pmyListCtrl->GetItemText(pos));
		}
	}
}
