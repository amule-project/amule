//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "PartFileConvertDlg.h"

#include <common/Format.h>
#include <common/Path.h>
#include "DataToText.h"
#include "OtherFunctions.h"
#include "PartFileConvert.h"
#include "GuiEvents.h"

#include <wx/stdpaths.h>
#include "muuli_wdr.h"

CPartFileConvertDlg*	CPartFileConvertDlg::s_convert_gui = NULL;


#ifndef __WINDOWS__
/* XPM */
static const char * convert_xpm[] = {
"16 16 9 1",
" 	c None",
".	c #B20000",
"+	c #FF0000",
"@	c #FF7F7F",
"#	c #008000",
"$	c #33B200",
"%	c #10E500",
"&	c #59FE4C",
"*	c #FFB2B2",
"        .       ",
"       .+.      ",
"      .+@+.     ",
"     .+@+.      ",
".   .+@+.#######",
".. .+@+.  #$%%&#",
".+.+@+.    #$%%#",
".@+@+.    #$%$%#",
".@@+.    #$%$#$#",
".*@@+.  #$%$# ##",
".......#$%$#   #",
"      #$%$#     ",
"     #$%$#      ",
"    #$%$#       ",
"     #$#        ",
"      #         "};
#endif /* ! __WINDOWS__ */

// Modeless Dialog Implementation
// CPartFileConvertDlg dialog

wxBEGIN_EVENT_TABLE(CPartFileConvertDlg, wxDialog)
	EVT_BUTTON(IDC_ADDITEM,		CPartFileConvertDlg::OnAddFolder)
	EVT_BUTTON(IDC_RETRY,		CPartFileConvertDlg::RetrySel)
	EVT_BUTTON(IDC_CONVREMOVE,	CPartFileConvertDlg::RemoveSel)
	EVT_BUTTON(wxID_CANCEL,		CPartFileConvertDlg::OnCloseButton)
	EVT_CLOSE(CPartFileConvertDlg::OnClose)
wxEND_EVENT_TABLE()

CPartFileConvertDlg::CPartFileConvertDlg(wxWindow* parent)
	: wxDialog(parent, -1, _("Import partfiles"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	convertDlg(this, true, true);

	m_joblist = CastChild(IDC_JOBLIST, wxListCtrl);
	m_pb_current = CastChild(IDC_CONV_PB_CURRENT, wxGauge);

	m_joblist->InsertColumn(0, _("File name"),	wxLIST_FORMAT_LEFT, 200);
	m_joblist->InsertColumn(1, _("State"),		wxLIST_FORMAT_LEFT, 100);
	m_joblist->InsertColumn(2, _("Size"),		wxLIST_FORMAT_LEFT, 100);
	m_joblist->InsertColumn(3, _("Filehash"),	wxLIST_FORMAT_LEFT, 100);

	SetIcon(wxICON(convert));

#ifdef CLIENT_GUI
	// There's no remote directory browser (yet), thus disable the
	// directory selector unless we're using a localhost connection
	if (!theApp->m_connect->IsConnectedToLocalHost()) {
		CastChild(IDC_ADDITEM, wxButton)->Enable(false);
	}
#endif
}

// Static methods

void CPartFileConvertDlg::ShowGUI(wxWindow* parent)
{
	if (s_convert_gui) {
		s_convert_gui->Show(true);
		s_convert_gui->Raise();
	} else {
		s_convert_gui = new CPartFileConvertDlg(parent);
		s_convert_gui->Show(true);
		Notify_ConvertReaddAllJobs();
	}
}

void CPartFileConvertDlg::CloseGUI()
{
	if (s_convert_gui) {
		s_convert_gui->Show(false);
		s_convert_gui->Destroy();
		s_convert_gui = NULL;
	}
}

void CPartFileConvertDlg::UpdateProgress(float percent, wxString text, wxString header)
{
	if (s_convert_gui) {
		s_convert_gui->m_pb_current->SetValue((int)percent);
		wxString buffer = CFormat("%.2f %%") % percent;
		wxStaticText* percentlabel = dynamic_cast<wxStaticText*>(s_convert_gui->FindWindow(IDC_CONV_PROZENT));
		percentlabel->SetLabel(buffer);

		if (!text.IsEmpty()) {
			dynamic_cast<wxStaticText*>(s_convert_gui->FindWindow(IDC_CONV_PB_LABEL))->SetLabel(text);
		}

		percentlabel->GetParent()->Layout();

		if (!header.IsEmpty()) {
			dynamic_cast<wxStaticBoxSizer*>(IDC_CURJOB)->GetStaticBox()->SetLabel(header);
		}
	}
}

void CPartFileConvertDlg::ClearInfo()
{
	if (s_convert_gui) {
		dynamic_cast<wxStaticBoxSizer*>(IDC_CURJOB)->GetStaticBox()->SetLabel(_("Waiting..."));
		dynamic_cast<wxStaticText*>(s_convert_gui->FindWindow(IDC_CONV_PROZENT))->SetLabel("");
		s_convert_gui->m_pb_current->SetValue(0);
		dynamic_cast<wxStaticText*>(s_convert_gui->FindWindow(IDC_CONV_PB_LABEL))->SetLabel("");
	}
}

void CPartFileConvertDlg::UpdateJobInfo(ConvertInfo& info)
{
	if (s_convert_gui) {
		// search jobitem in listctrl
		long item_nr = s_convert_gui->m_joblist->FindItem(-1, info.id);
		// if it does not exist, add it
		if (item_nr == -1) {
			item_nr = s_convert_gui->m_joblist->InsertItem(s_convert_gui->m_joblist->GetItemCount(), info.folder.GetPrintable());
			if (item_nr != -1) {
				s_convert_gui->m_joblist->SetItemData(item_nr, info.id);
			}
		}
		// update columns
		if (item_nr != -1) {
			s_convert_gui->m_joblist->SetItem(item_nr, 0, info.filename.IsOk() ? info.folder.GetPrintable() : info.filename.GetPrintable() );
			s_convert_gui->m_joblist->SetItem(item_nr, 1, GetConversionState(info.state) );
			if (info.size > 0) {
				s_convert_gui->m_joblist->SetItem(item_nr, 2, CFormat(_("%s (Disk: %s)")) % CastItoXBytes(info.size) % CastItoXBytes(info.spaceneeded));
			} else {
				s_convert_gui->m_joblist->SetItem(item_nr, 2, "");
			}
			s_convert_gui->m_joblist->SetItem(item_nr, 3, info.filehash);
		}
	}
}

void CPartFileConvertDlg::RemoveJobInfo(unsigned id)
{
	if (s_convert_gui) {
		long item_nr = s_convert_gui->m_joblist->FindItem(-1, id);
		if (item_nr != -1) {
			s_convert_gui->m_joblist->DeleteItem(item_nr);
		}
	}
}

// CPartFileConvertDlg message handlers

void CPartFileConvertDlg::OnAddFolder(wxCommandEvent& WXUNUSED(event))
{
	// TODO: use MuleRemoteDirSelector
	wxString folder = ::wxDirSelector(
		_("Please choose a folder to search for temporary downloads! (subfolders will be included)"),
		wxStandardPaths::Get().GetDocumentsDir(), wxDD_DEFAULT_STYLE,
		wxDefaultPosition, this);
	if (!folder.IsEmpty()) {
		int reply = wxMessageBox(_("Do you want the source files of successfully imported downloads be deleted?"),
					 _("Remove sources?"),
					 wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
		if (reply != wxCANCEL) {
			// TODO: use notification
			CPartFileConvert::ScanFolderToAdd(CPath(folder), (reply == wxYES));
		}
	}
}

void CPartFileConvertDlg::OnClose(wxCloseEvent& WXUNUSED(event))
{
	CloseGUI();
}

void CPartFileConvertDlg::OnCloseButton(wxCommandEvent& WXUNUSED(event))
{
	CloseGUI();
}

void CPartFileConvertDlg::RemoveSel(wxCommandEvent& WXUNUSED(event))
{
	if (m_joblist->GetSelectedItemCount() == 0) return;

	long item_nr = m_joblist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (item_nr != -1) {
		Notify_ConvertRemoveJob(m_joblist->GetItemData(item_nr));
		item_nr = m_joblist->GetNextItem(item_nr, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}

void CPartFileConvertDlg::RetrySel(wxCommandEvent& WXUNUSED(event))
{
	if (m_joblist->GetSelectedItemCount() == 0) return;

	long item_nr = m_joblist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (item_nr != -1) {
		Notify_ConvertRetryJob(m_joblist->GetItemData(item_nr));
		item_nr = m_joblist->GetNextItem(item_nr, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}
