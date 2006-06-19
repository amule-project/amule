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

#ifndef COMMENTDIALOGLST_H
#define COMMENTDIALOGLST_H

#include <wx/sizer.h>

class CMuleListCtrl;
class wxCommandEvent;
class CPartFile;

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
	static int wxCALLBACK SortProc(long item1, long item2, long sortData);

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
	

	DECLARE_EVENT_TABLE()
};

#endif // COMMENTDIALOGLST_H
// File_checked_for_headers
