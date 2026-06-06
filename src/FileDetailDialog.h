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

#ifndef FILEDETAILDIALOG_H
#define FILEDETAILDIALOG_H

#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/listctrl.h>
#include <wx/timer.h>

#include <vector>

class CPartFile;
class CKnownFile;

// CFileDetailDialog dialog

class CFileDetailDialog : public wxDialog
{
public:
	CFileDetailDialog(wxWindow *parent, std::vector<CPartFile *> & files, int index);
	virtual ~CFileDetailDialog();

	/**
	 * Drop every reference to `file` from any open instance of this
	 * dialog before the underlying CPartFile is destroyed. Stops the
	 * update-timer's deref of `m_file`, walks `m_files` to scrub
	 * matching entries, and dismisses the dialog if its currently
	 * active file is the destroyed one. Pointer-value comparison
	 * only — `file` may already be freed. Wired via
	 * MuleNotify::KnownFileBeingDestroyed (GuiEvents.cpp).
	 */
	static void DropReferencesTo(const CKnownFile* file);

protected:
	void OnTimer(wxTimerEvent& evt);
	wxDECLARE_EVENT_TABLE();

private:
	void UpdateData(bool resetFilename);
	std::vector<CPartFile *> & m_files;
	CPartFile* m_file;
	int m_index;
	wxTimer m_timer;
	bool m_filenameChanged;

	void OnClosewnd(wxCommandEvent& evt);
	void FillSourcenameList();
	void setEnableForApplyButton();
	void setValueForFilenameTextEdit(const wxString &s);
	void resetValueForFilenameTextEdit();

	void OnBnClickedButtonStrip(wxCommandEvent& evt);
	void OnBnClickedShowComment(wxCommandEvent& evt);
	void OnBnClickedTakeOver(wxCommandEvent& evt);
	void OnListClickedTakeOver(wxListEvent& evt);
	void OnTextFileNameChange(wxCommandEvent& evt);
	void OnBnClickedOk(wxCommandEvent& evt);
	void OnBnClickedApply(wxCommandEvent& evt);
	void OnBnClickedPrevFile(wxCommandEvent& evt);
	void OnBnClickedNextFile(wxCommandEvent& evt);
};

#endif // FILEDETAILDIALOG_H
// File_checked_for_headers
