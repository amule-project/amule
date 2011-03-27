//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#include "muuli_wdr.h"		// Needed for ID_CLOSEWNDFD,...,IDC_APPLY
#include "FileDetailDialog.h"	// Interface declarations
#include "FileDetailListCtrl.h"	// Needed for CFileDetailListCtrl
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "PartFile.h"		// Needed for CPartFile
#include "amule.h"		// Needed for theApp
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "OtherFunctions.h"
#include "MuleColour.h"

#define ID_MY_TIMER 1652

//IMPLEMENT_DYNAMIC(CFileDetailDialog, CDialog)
BEGIN_EVENT_TABLE(CFileDetailDialog,wxDialog)
	EVT_BUTTON(ID_CLOSEWNDFD, CFileDetailDialog::OnClosewnd)
	EVT_BUTTON(IDC_BUTTONSTRIP, CFileDetailDialog::OnBnClickedButtonStrip)
	EVT_BUTTON(IDC_TAKEOVER, CFileDetailDialog::OnBnClickedTakeOver)
	EVT_LIST_ITEM_ACTIVATED(IDC_LISTCTRLFILENAMES, CFileDetailDialog::OnListClickedTakeOver)
	EVT_BUTTON(IDC_CMTBT, CFileDetailDialog::OnBnClickedShowComment)
	EVT_TEXT(IDC_FILENAME, CFileDetailDialog::OnTextFileNameChange)
	EVT_BUTTON(IDC_APPLY_AND_CLOSE, CFileDetailDialog::OnBnClickedOk)
	EVT_BUTTON(IDC_APPLY, CFileDetailDialog::OnBnClickedApply)
	EVT_TIMER(ID_MY_TIMER,CFileDetailDialog::OnTimer)
END_EVENT_TABLE()

CFileDetailDialog::CFileDetailDialog(wxWindow *parent, CPartFile *file)
:
wxDialog(parent, -1, _("File Details"), wxDefaultPosition, wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX),
m_file(file),
m_filenameChanged(false)
{
	theApp->m_FileDetailDialogActive++;
	m_timer.SetOwner(this, ID_MY_TIMER);
	m_timer.Start(5000);
	wxSizer *content = fileDetails(this, true);
	UpdateData();
	content->SetSizeHints(this);
	content->Show(this, true);
}

CFileDetailDialog::~CFileDetailDialog()
{
	theApp->m_FileDetailDialogActive = 0;
	m_timer.Stop();
}

void CFileDetailDialog::OnTimer(wxTimerEvent& WXUNUSED(evt))
{
	UpdateData();
}

void CFileDetailDialog::OnClosewnd(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}

void CFileDetailDialog::UpdateData()
{
	wxString bufferS;
	CastChild(IDC_FNAME,   wxStaticText)->SetLabel(MakeStringEscaped(m_file->GetFileName().TruncatePath(60)));
	CastChild(IDC_METFILE, wxStaticText)->SetLabel(MakeStringEscaped(m_file->GetFullName().TruncatePath(60, true)));

	wxString tmp = CastChild(IDC_FILENAME, wxTextCtrl)->GetValue();
	if (tmp.Length() < 3) {
		resetValueForFilenameTextEdit();
	}

	CastChild(IDC_FHASH,wxStaticText)->SetLabel(m_file->GetFileHash().Encode());
	bufferS = CFormat(wxT("%u bytes (%s)")) % m_file->GetFileSize() % CastItoXBytes(m_file->GetFileSize());
	CastChild(IDC_FSIZE,wxControl)->SetLabel(bufferS);
	CastChild(IDC_PFSTATUS,wxControl)->SetLabel(m_file->getPartfileStatus());
	bufferS = CFormat(wxT("%i (%i)")) % m_file->GetPartCount() % m_file->GetHashCount();
	CastChild(IDC_PARTCOUNT,wxControl)->SetLabel(bufferS);
	CastChild(IDC_TRANSFERRED,wxControl)->SetLabel(CastItoXBytes(m_file->GetTransferred()));
	CastChild(IDC_FD_STATS1,wxControl)->SetLabel(CastItoXBytes(m_file->GetLostDueToCorruption()));
	CastChild(IDC_FD_STATS2,wxControl)->SetLabel(CastItoXBytes(m_file->GetGainDueToCompression()));
	CastChild(IDC_FD_STATS3,wxControl)->SetLabel(CastItoIShort(m_file->TotalPacketsSavedDueToICH()));
	CastChild(IDC_COMPLSIZE,wxControl)->SetLabel(CastItoXBytes(m_file->GetCompletedSize()));
	bufferS = CFormat(_("%.2f%% done")) % m_file->GetPercentCompleted();
	CastChild(IDC_PROCCOMPL,wxControl)->SetLabel(bufferS);
	bufferS = CFormat(_("%.2f kB/s")) % m_file->GetKBpsDown();
	CastChild(IDC_DATARATE,wxControl)->SetLabel(bufferS);
	bufferS = CFormat(wxT("%i")) % m_file->GetSourceCount();
	CastChild(IDC_SOURCECOUNT,wxControl)->SetLabel(bufferS);
	bufferS = CFormat(wxT("%i")) % m_file->GetTransferingSrcCount();
	CastChild(IDC_SOURCECOUNT2,wxControl)->SetLabel(bufferS);
	bufferS = CFormat(wxT("%i (%.1f%%)"))
		% m_file->GetAvailablePartCount()
		% ((m_file->GetAvailablePartCount() * 100.0)/ m_file->GetPartCount());
	CastChild(IDC_PARTAVAILABLE,wxControl)->SetLabel(bufferS);
	bufferS = CastSecondsToHM(m_file->GetDlActiveTime());
	CastChild(IDC_DLACTIVETIME, wxControl)->SetLabel(bufferS);
	
	if (m_file->lastseencomplete==0) {
		bufferS = wxString(_("Unknown")).MakeLower();
	} else {
		wxDateTime last_seen(m_file->lastseencomplete);
		bufferS = last_seen.FormatISODate() + wxT(" ") + last_seen.FormatISOTime();
	}

	CastChild(IDC_LASTSEENCOMPL,wxControl)->SetLabel(bufferS);
	setEnableForApplyButton();
	// disable "Show all comments" button if there are no comments
	FileRatingList list;
	m_file->GetRatingAndComments(list);
	CastChild(IDC_CMTBT, wxControl)->Enable(!list.empty());
	FillSourcenameList();
	Layout();
}

// CFileDetailDialog message handlers

void CFileDetailDialog::FillSourcenameList()
{
	CFileDetailListCtrl* pmyListCtrl; 
	int itempos;
	int inserted = 0; 
	pmyListCtrl = CastChild(IDC_LISTCTRLFILENAMES, CFileDetailListCtrl ); 

	// reset
	for (int i=0;i<pmyListCtrl->GetItemCount();i++){
		SourcenameItem *item = reinterpret_cast<SourcenameItem *>(pmyListCtrl->GetItemData(i));
		item->count = 0;
	}

	// update
#ifdef CLIENT_GUI
	const SourcenameItemMap &sources = m_file->GetSourcenameItemMap();
	for (SourcenameItemMap::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		const SourcenameItem &cur_src = it->second;
		itempos = pmyListCtrl->FindItem(-1,cur_src.name);
		if (itempos == -1) {
			int itemid = pmyListCtrl->InsertItem(0, cur_src.name);
			SourcenameItem *item = new SourcenameItem(cur_src.name, cur_src.count);
			pmyListCtrl->SetItemPtrData(0, reinterpret_cast<wxUIntPtr>(item));
			// background.. argh -- PA: was in old version - do we still need this?
			wxListItem tmpitem;
			tmpitem.m_itemId = itemid;
			tmpitem.SetBackgroundColour(CMuleColour(wxSYS_COLOUR_LISTBOX));
			pmyListCtrl->SetItem(tmpitem);
			inserted++;
		} else { 
			SourcenameItem *item = reinterpret_cast<SourcenameItem *>(pmyListCtrl->GetItemData(itempos));
			item->count = cur_src.count;
		} 
	}
#else // CLIENT_GUI
	const CKnownFile::SourceSet& sources = m_file->GetSourceList();
	CKnownFile::SourceSet::const_iterator it = sources.begin();
	for ( ; it != sources.end(); ++it ) {
		const CClientRef &cur_src = *it; 
		if (cur_src.GetRequestFile() != m_file ||
		    cur_src.GetClientFilename().Length() == 0) {
			continue;
		}

		itempos = pmyListCtrl->FindItem(-1,cur_src.GetClientFilename());
		if (itempos == -1) { 
			int itemid = pmyListCtrl->InsertItem(0, cur_src.GetClientFilename()); 
			SourcenameItem *item = new SourcenameItem(cur_src.GetClientFilename(), 1);
			pmyListCtrl->SetItemPtrData(0, reinterpret_cast<wxUIntPtr>(item));
			// background.. argh -- PA: was in old version - do we still need this?
			wxListItem tmpitem;
			tmpitem.m_itemId=itemid;
			tmpitem.SetBackgroundColour(CMuleColour(wxSYS_COLOUR_LISTBOX));
			pmyListCtrl->SetItem(tmpitem);
			inserted++;
		} else { 
			SourcenameItem *item = reinterpret_cast<SourcenameItem *>(pmyListCtrl->GetItemData(itempos));
			item->count++;
		} 
	}
#endif // CLIENT_GUI

	// remove 0'er and update counts
	for (int i = 0; i < pmyListCtrl->GetItemCount(); ++i) {
		SourcenameItem *item = reinterpret_cast<SourcenameItem *>(pmyListCtrl->GetItemData(i));
		if (item->count == 0) {
			delete item;
			pmyListCtrl->DeleteItem(i);
			i--;  // PA: one step back is enough, no need to go back to 0
		} else {
			pmyListCtrl->SetItem(i, 1, CFormat(wxT("%i")) % item->count);
		}
	}

	if (inserted) {
		pmyListCtrl->SortList();
	}
	// no need to call Layout() here, it's called in UpdateData()
}


void CFileDetailDialog::OnBnClickedShowComment(wxCommandEvent& WXUNUSED(evt))
{
	CCommentDialogLst(this,m_file).ShowModal();
}


void CFileDetailDialog::resetValueForFilenameTextEdit()
{
	CastChild(IDC_FILENAME, wxTextCtrl)->SetValue(m_file->GetFileName().GetPrintable());
	m_filenameChanged = false;
	setEnableForApplyButton();
}


void CFileDetailDialog::setValueForFilenameTextEdit(const wxString &s)
{
	CastChild(IDC_FILENAME, wxTextCtrl)->SetValue(s);
	m_filenameChanged = true;
	setEnableForApplyButton();
}


void CFileDetailDialog::setEnableForApplyButton()
{
	bool enabled = 
		m_file->GetStatus() != PS_COMPLETE &&
		m_file->GetStatus() != PS_COMPLETING &&
		m_filenameChanged;
	CastChild(IDC_APPLY, wxControl)->Enable(enabled);
	// Make OK button default so Text can be applied by hitting return
	CastChild(enabled ? IDC_APPLY_AND_CLOSE : ID_CLOSEWNDFD, wxButton)->SetDefault();
}


void CFileDetailDialog::OnTextFileNameChange(wxCommandEvent& WXUNUSED(evt))
{
	m_filenameChanged = true;
	setEnableForApplyButton();
}


void CFileDetailDialog::OnBnClickedOk(wxCommandEvent& evt)
{
	OnBnClickedApply(evt);
	OnClosewnd(evt);
}


void CFileDetailDialog::OnBnClickedApply(wxCommandEvent& WXUNUSED(evt))
{
	CPath fileName = CPath(CastChild(IDC_FILENAME, wxTextCtrl)->GetValue());

	if (fileName.IsOk() && (fileName != m_file->GetFileName())) {
		if (theApp->sharedfiles->RenameFile(m_file, fileName)) {
			FindWindow(IDC_FNAME)->SetLabel(MakeStringEscaped(m_file->GetFileName().GetPrintable()));
			FindWindow(IDC_METFILE)->SetLabel(m_file->GetFullName().GetPrintable());
			
			resetValueForFilenameTextEdit();
	
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
		case ',':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '-':
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

void CFileDetailDialog::OnBnClickedButtonStrip(wxCommandEvent& WXUNUSED(evt))
{
	wxString filename;
	filename = CastChild(IDC_FILENAME, wxTextCtrl)->GetValue();
	
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

	setValueForFilenameTextEdit(filename);
}

void CFileDetailDialog::OnBnClickedTakeOver(wxCommandEvent& WXUNUSED(evt))
{
	CFileDetailListCtrl* pmyListCtrl;
	pmyListCtrl = CastChild( IDC_LISTCTRLFILENAMES, CFileDetailListCtrl );
	if (pmyListCtrl->GetSelectedItemCount() > 0) {
		// get first selected item (there is only one)
		long pos = pmyListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (pos != -1) {	// shouldn't happen, we checked if something is selected
			setValueForFilenameTextEdit(pmyListCtrl->GetItemText(pos));
		}
	}
}

void CFileDetailDialog::OnListClickedTakeOver(wxListEvent& WXUNUSED(evt))
{
	wxCommandEvent ev;
	OnBnClickedTakeOver(ev);
}
// File_checked_for_headers
