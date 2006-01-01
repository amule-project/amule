//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Robert Rostek ( tecxx@rrs.at )
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

#ifndef DIRECTORYTREECTRL_H
#define DIRECTORYTREECTRL_H

#include <wx/treectrl.h>

#define USRMSG_ITEMSTATECHANGED		(47101) + 16
#define MP_SHAREDFOLDERS_FIRST	46901

class CDirectoryTreeCtrl : public wxTreeCtrl
{

  DECLARE_DYNAMIC_CLASS(CDirectoryTreeCtrl) 

    CDirectoryTreeCtrl() {};
public:
	CDirectoryTreeCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);
	virtual ~CDirectoryTreeCtrl();

	// initialize control
	void Init(void);
	// get all shared directories
	void GetSharedDirectories(wxArrayString* list);
	// set shared directories
	void SetSharedDirectories(wxArrayString* list);
	// User made any changes to list?
	bool HasChanged;

private:
	wxImageList m_image; 
	wxTreeItemId hRoot;
	// add a new item
	void AddChildItem(wxTreeItemId hBranch, const wxString& strText);
	// add subdirectory items
	void AddSubdirectories(wxTreeItemId hBranch, const wxString& folder);
	// returns true if folder has at least one subdirectory
	bool HasSubdirectories(const wxString& folder);
	// return the full path of an item (like C:\abc\somewhere\inheaven\)
	wxString GetFullPath(wxTreeItemId hItem);
	// check status of an item has changed
	void CheckChanged(wxTreeItemId hItem, bool bChecked);
	// returns true if a subdirectory of strDir is shared
	bool HasSharedSubdirectory(const wxString& strDir);
	// when sharing a directory, make all parent directories red
	void UpdateParentItems(wxTreeItemId hChild, bool add);

	// share list access
	bool IsShared(const wxString& strDir);
	void AddShare(const wxString& strDir);
	void DelShare(const wxString& strDir);
	void MarkChildren(wxTreeItemId hChild,bool mark);
	
protected:
	void OnTvnItemexpanding(wxTreeEvent& evt);
	void OnRButtonDown(wxTreeEvent& evt);
	void OnItemActivated(wxTreeEvent& evt);

	wxArrayString m_lstShared;
	wxString m_strLastRightClicked;
	bool m_bSelectSubDirs;
	
	
	DECLARE_EVENT_TABLE()
};

#undef wxTreeItemId

#endif // DIRECTORYTREECTRL_H
