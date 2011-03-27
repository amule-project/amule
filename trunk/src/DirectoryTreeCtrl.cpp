//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Robert Rostek ( tecxx@rrs.at )
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

#include "DirectoryTreeCtrl.h"	// Interface declarations

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/imaglist.h>

#include <common/StringFunctions.h>
#include <common/FileFunctions.h>
#include "amule.h"			// Needed for theApp
#include "muuli_wdr.h"		// Needed for amuleSpecial


BEGIN_EVENT_TABLE(CDirectoryTreeCtrl, wxTreeCtrl)
	EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY,	CDirectoryTreeCtrl::OnRButtonDown)
	EVT_TREE_ITEM_ACTIVATED(wxID_ANY,	CDirectoryTreeCtrl::OnItemActivated)
	EVT_TREE_ITEM_EXPANDED(wxID_ANY,	CDirectoryTreeCtrl::OnItemExpanding)
END_EVENT_TABLE()


class CItemData : public wxTreeItemData
{
	public:
		CItemData(const CPath& pathComponent)
			: m_path(pathComponent)
		{
		}

		~CItemData() {}

		const CPath& GetPathComponent() const { return m_path; }
	private:
		CPath	m_path;
};


CDirectoryTreeCtrl::CDirectoryTreeCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags)
	: wxTreeCtrl(parent,id,pos,siz,flags,wxDefaultValidator,wxT("ShareTree"))
{
	m_IsInit = false;
	HasChanged = false;
#ifdef CLIENT_GUI
	m_IsRemote = !theApp->m_connect->IsConnectedToLocalHost();
#else
	m_IsRemote = false;
#endif
}


CDirectoryTreeCtrl::~CDirectoryTreeCtrl()
{
}

enum {
	IMAGE_FOLDER = 0,
	IMAGE_FOLDER_SUB_SHARED
};


void CDirectoryTreeCtrl::Init()
{
	// already done ?
	if (m_IsInit) {
		return;
	}
	m_IsInit = true;

	// init image(s)
	wxImageList* images = new wxImageList(16, 16);
	images->Add(wxBitmap(amuleSpecial(1)));
	images->Add(wxBitmap(amuleSpecial(2)));
	// Gives wxTreeCtrl ownership of the list
	AssignImageList(images);

	
	// Create an empty root item, which we can
	// safely append when creating a full path.
	m_root = AddRoot(wxEmptyString, IMAGE_FOLDER, -1,
					new CItemData(CPath()));
	
	if (!m_IsRemote) {
	#ifndef __WXMSW__
		AddChildItem(m_root, CPath(wxT("/")));
	#else
		// this might take awhile, so change the cursor
		::wxSetCursor(*wxHOURGLASS_CURSOR);
		// retrieve bitmask of all drives available
		uint32 drives = GetLogicalDrives();
		drives >>= 1;
		for (char drive = 'C'; drive <= 'Z'; drive++) {
			drives >>= 1;
			if (! (drives & 1)) { // skip non existant drives
				continue;
			}
			wxString driveStr = CFormat(wxT("%c:")) % drive;
			uint32 type = GetDriveType(driveStr + wxT("\\"));

			// skip removable/undefined drives, share only fixed or remote drives
			if ((type == 3 || type == 4)   // fixed drive / remote drive
				&& CPath::DirExists(driveStr)) {
				AddChildItem(m_root, CPath(driveStr));
			}
		}
		::wxSetCursor(*wxSTANDARD_CURSOR);
	#endif
	}

	HasChanged = false;

	UpdateSharedDirectories();
}


void CDirectoryTreeCtrl::OnItemExpanding(wxTreeEvent& evt)
{
	wxTreeItemId hItem = evt.GetItem();

	// Force reloading of the path
	DeleteChildren(hItem);
	AddSubdirectories(hItem, GetFullPath(hItem));

	SortChildren(hItem);
}


void CDirectoryTreeCtrl::OnItemActivated(wxTreeEvent& evt)
{
	if (!m_IsRemote) {
		CheckChanged(evt.GetItem(), !IsBold(evt.GetItem()), false);
		HasChanged = true;
	}
}


void CDirectoryTreeCtrl::OnRButtonDown(wxTreeEvent& evt)
{
	if (m_IsRemote) {
		SelectItem(evt.GetItem()); // looks weird otherwise
	} else {
		// this might take awhile, so change the cursor
		::wxSetCursor(*wxHOURGLASS_CURSOR);
		MarkChildren(evt.GetItem(), !IsBold(evt.GetItem()), false);
		::wxSetCursor(*wxSTANDARD_CURSOR);
		HasChanged = true;
	}
}


void CDirectoryTreeCtrl::MarkChildren(wxTreeItemId hChild, bool mark, bool recursed)
{
	// Ensure that children are added, otherwise we might only get a "." entry.
	if (!IsExpanded(hChild) && ItemHasChildren(hChild)) {
		DeleteChildren(hChild);
		AddSubdirectories(hChild, GetFullPath(hChild));
		SortChildren(hChild);
	}
	
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild2 = GetFirstChild(hChild, cookie);
	if (hChild2.IsOk()) {
		SetHasSharedSubdirectory(hChild, mark);
	}
	while (hChild2.IsOk()) {
		MarkChildren(hChild2, mark, true);

		hChild2 = GetNextSibling(hChild2);
	}

	CheckChanged(hChild, mark, recursed);
}


void CDirectoryTreeCtrl::AddChildItem(wxTreeItemId hBranch, const CPath& item)
{
	wxCHECK_RET(hBranch.IsOk(), wxT("Attempted to add children to invalid item"));
	
	CPath fullPath = GetFullPath(hBranch).JoinPaths(item);
	wxTreeItemId treeItem = AppendItem(hBranch, item.GetPrintable(),
						IMAGE_FOLDER, -1,
						new CItemData(item));
	
	if (IsShared(fullPath)) {
		SetItemBold(treeItem, true);
	}
	
	if (HasSharedSubdirectory(fullPath)) {
		SetHasSharedSubdirectory(treeItem, true);
	}
	
	if (HasSubdirectories(fullPath)) {
		// Trick. will show + if it has subdirs
		AppendItem(treeItem, wxT("."));
	}
}


CPath CDirectoryTreeCtrl::GetFullPath(wxTreeItemId hItem)
{
	wxCHECK_MSG(hItem.IsOk(), CPath(), wxT("Invalid item in GetFullPath"));
	
	CPath result;
	for (; hItem.IsOk(); hItem = GetItemParent(hItem)) {
		CItemData* data = dynamic_cast<CItemData*>(GetItemData(hItem));
		wxCHECK_MSG(data, CPath(), wxT("Missing data-item in GetFullPath"));

		result = data->GetPathComponent().JoinPaths(result);
	}

	return result;
}


void CDirectoryTreeCtrl::AddSubdirectories(wxTreeItemId hBranch, const CPath& path)
{
	wxCHECK_RET(path.IsOk(), wxT("Invalid path in AddSubdirectories"));

	CDirIterator sharedDir(path);
	
	CPath dirName = sharedDir.GetFirstFile(CDirIterator::Dir); 
	while (dirName.IsOk()) {
		AddChildItem(hBranch, dirName);
		
		dirName = sharedDir.GetNextFile();
	}
}


bool CDirectoryTreeCtrl::HasSubdirectories(const CPath& folder)
{
	// Prevent error-messages if we try to traverse somewhere we have no access.
	wxLogNull logNo;

	return CDirIterator(folder).HasSubDirs();
}


void CDirectoryTreeCtrl::GetSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, wxT("Invalid list in GetSharedDirectories"));
	
	for (SharedMap::iterator it = m_lstShared.begin(); it != m_lstShared.end(); it++) {
		list->push_back(it->second);
	}
}


void CDirectoryTreeCtrl::SetSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, wxT("Invalid list in SetSharedDirectories"));
	
	m_lstShared.clear();
	for (PathList::iterator it = list->begin(); it != list->end(); it++) {
		m_lstShared.insert(SharedMapItem(GetKey(*it), *it));
	}

	if (m_IsInit) {
		UpdateSharedDirectories();
	}
}


wxString CDirectoryTreeCtrl::GetKey(const CPath& path)
{
	if (m_IsRemote) {
		return path.GetRaw();
	}

	// Sanity check, see IsSameAs() in Path.cpp
	const wxString cwd = wxGetCwd();
	const int flags = (wxPATH_NORM_ALL | wxPATH_NORM_CASE) & ~wxPATH_NORM_ENV_VARS;
	wxFileName fn(path.GetRaw());
	fn.Normalize(flags, cwd);
	return fn.GetFullPath();
}


void CDirectoryTreeCtrl::UpdateSharedDirectories()
{
	// ugly hack to at least show shared dirs in remote gui
	if (m_IsRemote) {
		DeleteChildren(m_root);
		for (SharedMap::iterator it = m_lstShared.begin(); it != m_lstShared.end(); it++) {
			AppendItem(m_root, it->second.GetPrintable(), IMAGE_FOLDER, -1, new CItemData(it->second));
		}
		return;
	}

	// Mark all shared root items (on windows this can be multiple
	// drives, on unix there is only the root dir).
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild = GetFirstChild(GetRootItem(), cookie);
	
	while (hChild.IsOk()) {
		// Does this drive have shared subfolders?
		if (HasSharedSubdirectory(GetFullPath(hChild))) { 
			SetHasSharedSubdirectory(hChild, true);
		}
		
		// Is this drive shared?
		if (IsShared(GetFullPath(hChild))) {
			SetItemBold(hChild, true);
		}

		hChild = GetNextSibling(hChild);
	}
}


bool CDirectoryTreeCtrl::HasSharedSubdirectory(const CPath& path)
{
	SharedMap::iterator it = m_lstShared.upper_bound(GetKey(path) + wxFileName::GetPathSeparator());
	if (it == m_lstShared.end()) {
		return false;
	}
	// upper_bound() doesn't find the directory itself, so no need to check for that.
	return it->second.StartsWith(path);
}


void CDirectoryTreeCtrl::SetHasSharedSubdirectory(wxTreeItemId hItem, bool add)
{
	SetItemImage(hItem, add ? IMAGE_FOLDER_SUB_SHARED : IMAGE_FOLDER);
}


void CDirectoryTreeCtrl::CheckChanged(wxTreeItemId hItem, bool bChecked, bool recursed)
{
	if (IsBold(hItem) != bChecked) {
		SetItemBold(hItem, bChecked);

		if (bChecked) {
			AddShare(GetFullPath(hItem)); 
		} else {
			DelShare(GetFullPath(hItem));
		}

		if (!recursed) {
			UpdateParentItems(hItem, bChecked);
		}
	}
}


bool CDirectoryTreeCtrl::IsShared(const CPath& path)
{
	wxCHECK_MSG(path.IsOk(), false, wxT("Invalid path in IsShared"));

	return m_lstShared.find(GetKey(path)) != m_lstShared.end();
}


void CDirectoryTreeCtrl::AddShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), wxT("Invalid path in AddShare"));
	
	if (IsShared(path)) {
		return;
	}
	
	m_lstShared.insert(SharedMapItem(GetKey(path), path));
}


void CDirectoryTreeCtrl::DelShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), wxT("Invalid path in DelShare"));
	
	m_lstShared.erase(GetKey(path));
}


void CDirectoryTreeCtrl::UpdateParentItems(wxTreeItemId hChild, bool add)
{
	wxTreeItemId parent = hChild;
	while (parent != GetRootItem()) {
		parent = GetItemParent(parent);
		if (add) {
			if (GetItemImage(parent) == IMAGE_FOLDER_SUB_SHARED) {
				// parent already marked -> so are all its parents, finished
				break;
			} else {
				SetHasSharedSubdirectory(parent, true);
			}
		} else {
			if (GetItemImage(parent) == IMAGE_FOLDER_SUB_SHARED) {
				// check if now there are still other shared dirs
				if (HasSharedSubdirectory(GetFullPath(parent))) {
					// yes, then further parents can stay red
					break;
				} else {
					// no, further parents have to be checked too
					SetHasSharedSubdirectory(parent, false);
				}
			} else {  // should not happen (unmark child of which the parent is already unmarked
				break;
			}
		}
	}
}
// File_checked_for_headers
