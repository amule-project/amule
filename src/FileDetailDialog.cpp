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

#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW
#include <wx/stattext.h>	// Needed for wxStaticText
#include <wx/sizer.h>
#include <wx/intl.h>
#include <wx/textctrl.h>

#include "muuli_wdr.h"		// Needed for ID_CLOSEWNDFD,...,IDC_RENAME
#include "FileDetailDialog.h"	// Interface declarations
#include "FileDetailListCtrl.h"	// Needed for CFileDetailListCtrl
#include "OtherFunctions.h"	// Needed for MakeStringEscaped
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "updownclient.h"	// Needed for CUpDownClient
#include "PartFile.h"		// Needed for CPartFile
#include "Color.h"		// Needed for SYSCOLOR
#include <common/Format.h>		// Needed for CFormat
#include "amule.h"				// Needed for theApp
#include "SharedFileList.h"		// Needed for CSharedFileList

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
: wxDialog(parent,-1,_("File Details"),wxDefaultPosition,wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
	m_file = file;
	m_timer.SetOwner(this,ID_MY_TIMER);
	m_timer.Start(5000);
	wxSizer* content=fileDetails(this,TRUE);
	UpdateData();
	content->SetSizeHints(this);
	content->Show(this,TRUE);

	// Currently renaming of completed files causes problem with kad
	FindWindow(IDC_RENAME)->Enable(file->IsPartFile());
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

	CastChild(IDC_FHASH,wxStaticText)->SetLabel(m_file->GetFileHash().Encode());
	CastChild(IDC_FSIZE,wxControl)->SetLabel(CastItoXBytes(m_file->GetFileSize()));
	CastChild(IDC_PFSTATUS,wxControl)->SetLabel(m_file->getPartfileStatus());
	bufferS = wxString::Format(wxT("%i (%i)"),m_file->GetPartCount(),m_file->GetHashCount());
	CastChild(IDC_PARTCOUNT,wxControl)->SetLabel(bufferS);
	CastChild(IDC_TRANSFERED,wxControl)->SetLabel(CastItoXBytes(m_file->GetTransfered()));
	CastChild(IDC_FD_STATS1,wxControl)->SetLabel(CastItoXBytes(m_file->GetLostDueToCorruption()));
	CastChild(IDC_FD_STATS2,wxControl)->SetLabel(CastItoXBytes(m_file->GetGainDueToCompression()));
	CastChild(IDC_FD_STATS3,wxControl)->SetLabel(CastItoIShort(m_file->TotalPacketsSavedDueToICH()));
	CastChild(IDC_COMPLSIZE,wxControl)->SetLabel(CastItoXBytes(m_file->GetCompletedSize()));
	bufferS = wxString::Format(_("%.2f%% done"),m_file->GetPercentCompleted());
	CastChild(IDC_PROCCOMPL,wxControl)->SetLabel(bufferS);
	bufferS = wxString::Format(_("%.2f kB/s"),(float)m_file->GetKBpsDown());
	CastChild(IDC_DATARATE,wxControl)->SetLabel(bufferS);
	bufferS = wxString::Format(wxT("%i"),m_file->GetSourceCount());
	CastChild(IDC_SOURCECOUNT,wxControl)->SetLabel(bufferS);
	bufferS = wxString::Format(wxT("%i"),m_file->GetTransferingSrcCount());
	CastChild(IDC_SOURCECOUNT2,wxControl)->SetLabel(bufferS);
	bufferS = wxString::Format(wxT("%i (%.1f%%)"),m_file->GetAvailablePartCount(),(float) ((m_file->GetAvailablePartCount()*100)/ m_file->GetPartCount()));
	CastChild(IDC_PARTAVAILABLE,wxControl)->SetLabel(bufferS);

	if (m_file->lastseencomplete==0) {
		bufferS = wxString(_("Unknown")).MakeLower();
	} else {
		wxDateTime last_seen(m_file->lastseencomplete);
		bufferS = last_seen.FormatISODate() + wxT(" ") + last_seen.FormatISOTime();
	}

	CastChild(IDC_LASTSEENCOMPL,wxControl)->SetLabel(bufferS);
	CastChild(IDC_RENAME,wxControl)->Enable((m_file->GetStatus() != PS_COMPLETE && m_file->GetStatus() != PS_COMPLETING)); //add by CML 
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
	const CPartFile::SourceSet& sources = m_file->GetSourceList();
	CPartFile::SourceSet::iterator it = sources.begin();
	for ( ; it != sources.end(); ++it ) {
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
			nameCountStr = wxString::Format(wxT("%li"), item->count); 
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
	CCommentDialogLst(this,m_file).ShowModal();
}


void CFileDetailDialog::OnBnClickedRename(wxCommandEvent& WXUNUSED(evt))
{
	wxString fileName = CastChild(IDC_FILENAME, wxTextCtrl)->GetValue();

	if (!fileName.IsEmpty() and (fileName != m_file->GetFileName())) {
		if (theApp.sharedfiles->RenameFile(m_file, fileName)) {
			FindWindow(IDC_FNAME)->SetLabel(MakeStringEscaped(m_file->GetFileName()));
			FindWindow(IDC_METFILE)->SetLabel(m_file->GetFullName());
	
			CastChild(IDC_FILENAME, wxTextCtrl)->SetValue(m_file->GetFileName());
	
			Layout();
		}
	}
}


bool IsDigit(const wxChar ch)
{
	switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': return true;
	}
	return false;
}

bool IsWordSeparator(const wxChar ch)
{
	switch (ch) {
		case '.':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '-':
		case '\'':
		case '"':
		case ' ': return true;
	}
	return false;
}

void ReplaceWord(wxString& str, const wxString& replaceFrom, const wxString& replaceTo, bool numbers = false)
{
	unsigned int i = 0;
	unsigned int l = replaceFrom.Length();
	while (i < str.Length()) {
		if (str.Mid(i, l) == replaceFrom) {
			if ((i == 0 || IsWordSeparator(str.GetChar(i-1))) &&
				((i == str.Length() - l || IsWordSeparator(str.GetChar(i+l))) ||
					(numbers && IsDigit(str.GetChar(i+l))))) {
				str.replace(i, l, replaceTo);
			}
			i += replaceTo.Length() - 1;
		}
		i++;
	}
}

void CFileDetailDialog::OnBnClickedButtonStrip(wxCommandEvent& WXUNUSED(evt)) {
	wxString filename;
	filename=CastChild(IDC_FILENAME,wxTextCtrl)->GetValue();
	
	int extpos = filename.Find('.', true);
	wxString ext;
	if (extpos > 0) {
		// get the extension - we do not modify it except make it lowercase
		ext = filename.Mid(extpos);
		ext.MakeLower();
		// get rid of extension and replace . with space
		filename.Truncate(extpos);
		filename.Replace(wxT("."),wxT(" "));
	}

	// Replace Space-holders with Spaces
	filename.Replace(wxT("_"),wxT(" "));
	filename.Replace(wxT("%20"),wxT(" "));

	// Some additional formatting
	filename.Replace(wxT("hYPNOTiC"), wxEmptyString);
	filename.MakeLower();
	filename.Replace(wxT("xxx"), wxT("XXX"));
//	filename.Replace(wxT("xdmnx"), wxEmptyString);
//	filename.Replace(wxT("pmp"), wxEmptyString);
//	filename.Replace(wxT("dws"), wxEmptyString);
	filename.Replace(wxT("www pornreactor com"), wxEmptyString);
	filename.Replace(wxT("sharereactor"), wxEmptyString);
	filename.Replace(wxT("found via www filedonkey com"), wxEmptyString);
	filename.Replace(wxT("deviance"), wxEmptyString);
	filename.Replace(wxT("adunanza"), wxEmptyString);
	filename.Replace(wxT("-ftv"), wxEmptyString);
	filename.Replace(wxT("flt"), wxEmptyString);
	filename.Replace(wxT("[]"), wxEmptyString);
	filename.Replace(wxT("()"), wxEmptyString);

	// Change CD, CD#, VCD{,#}, DVD{,#}, ISO, PC to uppercase
	ReplaceWord(filename, wxT("cd"), wxT("CD"), true);
	ReplaceWord(filename, wxT("vcd"), wxT("VCD"), true);
	ReplaceWord(filename, wxT("dvd"), wxT("DVD"), true);
	ReplaceWord(filename, wxT("iso"), wxT("ISO"), false);
	ReplaceWord(filename, wxT("pc"), wxT("PC"), false);

	// Make leading Caps
	// and delete 1+ spaces
	if (filename.Length()>1)
	{
		bool last_char_space = true;
		bool last_char_wordseparator = true;
		unsigned int i = 0;

		do {
			wxChar c = filename.GetChar(i);
			if (c == ' ') {
				if (last_char_space) {
					filename.Remove(i, 1);
					i--;
				} else {
					last_char_space = true;
				}
			} else if (c == '.') {
				if (last_char_space && i > 0) {
					i--;
					filename.Remove(i, 1);
				}
				last_char_space = false;
			} else {
				if (last_char_wordseparator) {
					wxString tempStr(c);
					tempStr.MakeUpper();
					filename.SetChar(i, tempStr.GetChar(0));
					last_char_space = false;
				}
			}
			last_char_wordseparator = IsWordSeparator(c);
			i++;
		} while (i < filename.Length());

		if (last_char_space && i > 0) {
			filename.Remove(i-1, 1);
		}
	}

	// should stay lowercase
	ReplaceWord(filename, wxT("By"), wxT("by"));

	// re-add extension
	filename += ext;

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
