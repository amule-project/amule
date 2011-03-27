//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Robert Rostek ( tecxx@rrs.at )
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
#include <vector>
#include <map>

#include <common/Path.h>


class CDirectoryTreeCtrl : public wxTreeCtrl
{
public:
	typedef std::vector<CPath> PathList;
	
	CDirectoryTreeCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags);
	virtual ~CDirectoryTreeCtrl();

	// get all shared directories
	void GetSharedDirectories(PathList* list);
	// set list of shared directories
	void SetSharedDirectories(PathList* list);
	
	// User made any changes to list?
	bool HasChanged;

	// initialize control
	void Init();

private:
	// add a new item
	void AddChildItem(wxTreeItemId hBranch, const CPath& item);
	// add subdirectory items
	void AddSubdirectories(wxTreeItemId hBranch, const CPath& path);
	// returns true if folder has at least one subdirectory
	bool HasSubdirectories(const CPath& path);
	// return the full path of an item (like C:\abc\somewhere\inheaven\)
	CPath GetFullPath(wxTreeItemId hItem);
	// check status of an item has changed
	void CheckChanged(wxTreeItemId hItem, bool bChecked, bool recursed);
	// returns true if a subdirectory of strDir is shared
	bool HasSharedSubdirectory(const CPath& path);
	// set shared directories according to list
	void UpdateSharedDirectories();
	// when sharing a directory, make all parent directories red
	void UpdateParentItems(wxTreeItemId hChild, bool add);
	// set color red if there's a shared subdirectory
	void SetHasSharedSubdirectory(wxTreeItemId hItem, bool add);

	// share list access
	bool IsShared(const CPath& path);
	void AddShare(const CPath& path);
	void DelShare(const CPath& path);
	void MarkChildren(wxTreeItemId hChild, bool mark, bool recursed);
	
	void OnItemExpanding(wxTreeEvent& evt);
	void OnRButtonDown(wxTreeEvent& evt);
	void OnItemActivated(wxTreeEvent& evt);

	typedef std::pair<wxString, CPath> SharedMapItem;
	typedef std::map<wxString, CPath> SharedMap;
	SharedMap m_lstShared;
	// get map key from path (normalized path)
	wxString GetKey(const CPath& path);

	bool m_IsInit;
	// Are we running the remote GUI, and from a remote location?
	bool m_IsRemote;
	
	wxTreeItemId m_root;
	
	DECLARE_EVENT_TABLE()
};

#undef wxTreeItemId

#endif // DIRECTORYTREECTRL_H
// File_checked_for_headers
