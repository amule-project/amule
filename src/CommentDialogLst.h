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

#ifndef COMMENTDIALOGLST_H
#define COMMENTDIALOGLST_H

#include <wx/dialog.h>		// Needed for wxDialog	// Do_not_auto_remove
#include <wx/sizer.h>

class CMuleListCtrl;
class wxCommandEvent;
class CPartFile;
class CKnownFile;

/**
 * This dialog is used to display file-comments received from other clients.
 */
class CCommentDialogLst : public wxDialog
{
public:
	CCommentDialogLst(wxWindow* pParent, CPartFile* file);
	~CCommentDialogLst();

	/**
	 * Sorter function for the CMuleListCtrl used to contain the lists.
	 */
	static int wxCALLBACK SortProc(wxUIntPtr item1, wxUIntPtr item2, wxIntPtr sortData);

	/**
	 * Drop every reference to `file` from any open instance of this
	 * dialog before the CKnownFile / CPartFile is destroyed.
	 * Pointer-value comparison only — `file` may already be freed.
	 * Wired via MuleNotify::KnownFileBeingDestroyed (GuiEvents.cpp).
	 */
	static void DropReferencesTo(const CKnownFile* file);

private:
	void OnBnClickedApply(wxCommandEvent& evt);
	void OnBnClickedRefresh(wxCommandEvent& evt);

	/**
	 * Updates the contents of the comments/ratings list.
	 */
	void UpdateList();

	/**
	 * Clears the contents of the comments/ratings list.
	 */
	void ClearList();

	//! The file to display comments for.
	CPartFile* m_file;

	//! The list containing comments/ratings.
	CMuleListCtrl* m_list;


	wxDECLARE_EVENT_TABLE();
};

#endif // COMMENTDIALOGLST_H
// File_checked_for_headers
