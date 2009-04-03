//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Robert Rostek ( tecxx@rrs.at )
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

#include "DirectoryTreeCtrl.h"	// Interface declarations

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/imaglist.h>

#include <common/StringFunctions.h>
#include <common/FileFunctions.h>
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
			: m_data(0)
			, m_path(pathComponent)
		{
		}

		~CItemData() {}

		void AddCount() { m_data++; }
		void SubCount() { m_data--; }
		int GetCount() const { return m_data; }
		const CPath& GetPathComponent() const { return m_path; }
	private:
		int	m_data;
		CPath	m_path;
};


CDirectoryTreeCtrl::CDirectoryTreeCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags)
	: wxTreeCtrl(parent,id,pos,siz,flags,wxDefaultValidator,wxT("ShareTree"))
{
	m_IsInit = false;
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
	wxTreeItemId root = AddRoot(wxEmptyString, IMAGE_FOLDER, -1,
					new CItemData(CPath()));
	

#ifndef __WXMSW__
	AddChildItem(root, CPath(wxT("/")));
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
		wxString driveStr = wxString::Format(wxT("%c:"), drive);
		uint32 type = GetDriveType(driveStr + wxT("\\"));

		// skip removable/undefined drives, share only fixed or remote drives
		if ((type == 3 || type == 4)   // fixed drive / remote drive
			&& CPath::DirExists(driveStr)) {
			AddChildItem(root, CPath(driveStr));
		}
	}
	::wxSetCursor(*wxSTANDARD_CURSOR);
#endif

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
	CheckChanged(evt.GetItem(), !IsBold(evt.GetItem()));
	HasChanged = true;
}


void CDirectoryTreeCtrl::OnRButtonDown(wxTreeEvent& evt)
{
	MarkChildren(evt.GetItem(), !IsBold(evt.GetItem()));
	HasChanged = true;
}


void CDirectoryTreeCtrl::MarkChildren(wxTreeItemId hChild, bool mark)
{
	// Ensure that children are added, otherwise we might only get a "." entry.
	if (!IsExpanded(hChild) && ItemHasChildren(hChild)) {
		DeleteChildren(hChild);
		AddSubdirectories(hChild, GetFullPath(hChild));
		SortChildren(hChild);
	}
	
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild2 = GetFirstChild(hChild, cookie);
	while (hChild2.IsOk()) {
		MarkChildren(hChild2, mark);

		hChild2 = GetNextSibling(hChild2);
	}

	CheckChanged(hChild, mark);
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
		UpdateParentItems(treeItem, true);
	}
	
	if (HasSharedSubdirectory(fullPath)) {
		SetItemImage(treeItem, IMAGE_FOLDER_SUB_SHARED);
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
	
	list->insert(list->end(), m_lstShared.begin(), m_lstShared.end());
}


void CDirectoryTreeCtrl::SetSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, wxT("Invalid list in SetSharedDirectories"));
	
	m_lstShared.clear();
	m_lstShared.insert(m_lstShared.end(), list->begin(), list->end());

	if (m_IsInit) {
		UpdateSharedDirectories();
	}
}


void CDirectoryTreeCtrl::UpdateSharedDirectories()
{
	// Mark all shared root items (on windows this can be multiple
	// drives, on unix there is only the root dir).
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild = GetFirstChild(GetRootItem(), cookie);
	
	while (hChild.IsOk()) {
		// Does this drive have shared subfolders?
		if (HasSharedSubdirectory(GetFullPath(hChild))) { 
			SetItemImage(hChild, IMAGE_FOLDER_SUB_SHARED);
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
	PathList::iterator it = m_lstShared.begin();
	for (; it != m_lstShared.end(); ++it) {
		// IsSameDir to avoid the case where 'path' itself is shared.
		if (it->StartsWith(path) && (!it->IsSameDir(path))) {
			return true;
		}
	}

	return false;
}


void CDirectoryTreeCtrl::CheckChanged(wxTreeItemId hItem, bool bChecked)
{
	if (IsBold(hItem) != bChecked) {
		SetItemBold(hItem, bChecked);

		if (bChecked) {
			AddShare(GetFullPath(hItem)); 
		} else {
			DelShare(GetFullPath(hItem));
		}

		UpdateParentItems(hItem, bChecked);
	}
}


bool CDirectoryTreeCtrl::IsShared(const CPath& path)
{
	wxCHECK_MSG(path.IsOk(), false, wxT("Invalid path in IsShared"));

	PathList::iterator it = m_lstShared.begin();
	for (; it != m_lstShared.end(); ++it) {
		if (it->IsSameDir(path)) {
			return true;
		}
	}

	return false;
}


void CDirectoryTreeCtrl::AddShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), wxT("Invalid path in AddShare"));
	
	if (IsShared(path)) {
		return;
	}
	
	m_lstShared.push_back(path);
}


void CDirectoryTreeCtrl::DelShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), wxT("Invalid path in DelShare"));
	
	PathList::iterator it = m_lstShared.begin();
	for (; it != m_lstShared.end(); ++it ) {
		if (it->IsSameDir(path)) {
			m_lstShared.erase(it);
			return;
		}
	}
}


void CDirectoryTreeCtrl::UpdateParentItems(wxTreeItemId hChild, bool add)
{
	wxTreeItemId parent = hChild;
	while (parent != GetRootItem()) {
		parent = GetItemParent(parent);
		CItemData* parent_data = dynamic_cast<CItemData*>(GetItemData(parent));
		if (add) {
			parent_data->AddCount();
			if (parent_data->GetCount()==1) {
				SetItemImage(parent, IMAGE_FOLDER_SUB_SHARED);
			}
		} else {
			switch (parent_data->GetCount()) {
				case 0:
					break;
				case 1:
					SetItemImage(parent, IMAGE_FOLDER);
				default:
					parent_data->SubCount();
					break;
			}
		}
	};
}
// File_checked_for_headers
