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


wxBEGIN_EVENT_TABLE(CDirectoryTreeCtrl, wxTreeCtrl)
	EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY,	CDirectoryTreeCtrl::OnRButtonDown)
	EVT_TREE_ITEM_ACTIVATED(wxID_ANY,	CDirectoryTreeCtrl::OnItemActivated)
	EVT_TREE_ITEM_EXPANDED(wxID_ANY,	CDirectoryTreeCtrl::OnItemExpanding)
wxEND_EVENT_TABLE()


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
	: wxTreeCtrl(parent,id,pos,siz,flags,wxDefaultValidator,"ShareTree")
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
	m_root = AddRoot("", IMAGE_FOLDER, -1,
					new CItemData(CPath()));

	if (!m_IsRemote) {
	#ifndef __WINDOWS__
		AddChildItem(m_root, CPath("/"));
	#else
		// this might take awhile, so change the cursor
		::wxSetCursor(*wxHOURGLASS_CURSOR);
		// retrieve bitmask of all drives available
		uint32 drives = GetLogicalDrives();
		drives >>= 1;
		for (char drive = 'C'; drive <= 'Z'; drive++) {
			drives >>= 1;
			if (! (drives & 1)) { // skip non existent drives
				continue;
			}
			wxString driveStr = CFormat("%c:") % drive;
			uint32 type = GetDriveType((driveStr + "\\").wc_str());

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
		return;
	}

	// Right-click is the "recursive share" gesture. Historically the
	// handler eagerly walked the entire subtree (expanding every
	// directory it had never opened, then marking each one as
	// shared) which on large roots like /home produced multi-minute
	// UI freezes with no progress indicator and no way to cancel —
	// see issue #592.
	//
	// We now record the intent on the right-clicked item only.
	// PrefsUnifiedDlg::OnOk flattens the recursive-share roots into
	// concrete subdirectory paths on a background thread with a
	// progress dialog and a cancel button. Already-expanded
	// descendants are still toggled visually here so the tree
	// reflects what the user sees, but no new directory enumeration
	// is performed; collapsed subtrees retain their current visual
	// state and will be correctly bolded the next time they are
	// expanded (AddChildItem checks IsInsideRecursiveShare).
	const wxTreeItemId hItem = evt.GetItem();
	const bool wasBold = IsBold(hItem);
	const CPath fullPath = GetFullPath(hItem);

	if (wasBold) {
		// Unshare. Clean both the recursive intent and any
		// m_lstShared entries that live underneath this path —
		// the latter is what removes the flat descendants that a
		// previous Prefs session may have committed from a
		// recursive share. Doing it as an in-memory map sweep
		// catches subdirs that aren't currently rendered in the
		// tree without having to expand them all from disk.
		DelRecursiveShare(fullPath);
		DelSharesUnder(fullPath);
	} else {
		AddRecursiveShare(fullPath);
	}

	// Walk only the *already-loaded* descendants so the in-tree
	// visual stays consistent without forcing any disk I/O.
	MarkChildren(hItem, !wasBold, false);
	HasChanged = true;
}


void CDirectoryTreeCtrl::MarkChildren(wxTreeItemId hChild, bool mark, bool recursed)
{
	// Touch only the children that are *already* loaded into the
	// tree control. We no longer enumerate collapsed subtrees here —
	// that would re-introduce the unbounded directory walk OnRButton-
	// Down used to do. Collapsed subtrees stay as-is and are
	// re-evaluated on demand by AddChildItem (which consults the
	// recursive-share intent) the next time the user expands them.
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild2 = GetFirstChild(hChild, cookie);
	if (hChild2.IsOk()) {
		SetHasSharedSubdirectory(hChild, mark);
	}
	while (hChild2.IsOk()) {
		if (IsExpanded(hChild) || ItemHasChildren(hChild2)) {
			MarkChildren(hChild2, mark, true);
		} else {
			CheckChanged(hChild2, mark, true);
		}

		hChild2 = GetNextSibling(hChild2);
	}

	CheckChanged(hChild, mark, recursed);
}


void CDirectoryTreeCtrl::AddChildItem(wxTreeItemId hBranch, const CPath& item)
{
	wxCHECK_RET(hBranch.IsOk(), "Attempted to add children to invalid item");

	CPath fullPath = GetFullPath(hBranch).JoinPaths(item);
	wxTreeItemId treeItem = AppendItem(hBranch, item.GetPrintable(),
						IMAGE_FOLDER, -1,
						new CItemData(item));

	// BUG: wxGenericTreeControl won't set text calculated sizes when the item is created in AppendItem.
	// This causes asserts on Mac and possibly other systems, so we have to repeat setting the string here.
	SetItemText(treeItem, item.GetPrintable());

	// Bold means "this directory is part of the pending share set",
	// covering both the explicit-share case (m_lstShared) and the
	// implicit case where the item is a descendant of a recursive-
	// share root (m_lstSharedRecursive). The latter check is what
	// keeps the tree visually consistent after a right-click whose
	// recursive expansion is now deferred to commit time — the
	// subtree gets bolded lazily as the user opens it.
	if (IsShared(fullPath) || IsRecursiveShare(fullPath)
		|| IsInsideRecursiveShare(fullPath))
	{
		SetItemBold(treeItem, true);
	}

	if (HasSharedSubdirectory(fullPath)) {
		SetHasSharedSubdirectory(treeItem, true);
	}

	if (HasSubdirectories(fullPath)) {
		// Trick. will show + if it has subdirs
		AppendItem(treeItem, ".");
	}
}


CPath CDirectoryTreeCtrl::GetFullPath(wxTreeItemId hItem)
{
	{ wxCHECK_MSG(hItem.IsOk(), CPath(), "Invalid item in GetFullPath"); }

	CPath result;
	for (; hItem.IsOk(); hItem = GetItemParent(hItem)) {
		CItemData* data = dynamic_cast<CItemData*>(GetItemData(hItem));
		wxCHECK_MSG(data, CPath(), "Missing data-item in GetFullPath");

		result = data->GetPathComponent().JoinPaths(result);
	}

	return result;
}


void CDirectoryTreeCtrl::AddSubdirectories(wxTreeItemId hBranch, const CPath& path)
{
	wxCHECK_RET(path.IsOk(), "Invalid path in AddSubdirectories");

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
	wxCHECK_RET(list, "Invalid list in GetSharedDirectories");

	for (SharedMap::iterator it = m_lstShared.begin(); it != m_lstShared.end(); ++it) {
		list->push_back(it->second);
	}
}


void CDirectoryTreeCtrl::SetSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, "Invalid list in SetSharedDirectories");

	m_lstShared.clear();
	for (PathList::iterator it = list->begin(); it != list->end(); ++it) {
		m_lstShared.insert(SharedMapItem(GetKey(*it), *it));
	}

	if (m_IsInit) {
		UpdateSharedDirectories();
	}
}


void CDirectoryTreeCtrl::GetRecursiveSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, "Invalid list in GetRecursiveSharedDirectories");

	for (SharedMap::iterator it = m_lstSharedRecursive.begin();
		it != m_lstSharedRecursive.end(); ++it)
	{
		list->push_back(it->second);
	}
}


void CDirectoryTreeCtrl::SetRecursiveSharedDirectories(PathList* list)
{
	wxCHECK_RET(list, "Invalid list in SetRecursiveSharedDirectories");

	m_lstSharedRecursive.clear();
	for (PathList::iterator it = list->begin(); it != list->end(); ++it) {
		m_lstSharedRecursive.insert(SharedMapItem(GetKey(*it), *it));
	}

	// Mirror SetSharedDirectories: refresh the tree so a recursive
	// root reloaded from shareddir-recursive.dat at prefs-open is
	// rendered bold straight away rather than only after the user
	// happens to expand its branch. PrefsUnifiedDlg calls this and
	// SetSharedDirectories back-to-back; without this refresh, only
	// the explicit set would have been visualised on first paint.
	if (m_IsInit) {
		UpdateSharedDirectories();
	}
}


wxString CDirectoryTreeCtrl::GetKey(const CPath& path)
{
	if (m_IsRemote) {
		return path.GetRaw();
	}

	// Sanity check, see IsSameAs() in Path.cpp.  Skip wxGetCwd() when
	// the path is already absolute — Normalize ignores cwd in that
	// case, and wxGetCwd() emits a wxLogSysError for every call when
	// the process's recorded CWD has been removed.
	wxString cwd;
	wxFileName fn(path.GetRaw());
	if (!fn.IsAbsolute()) {
		cwd = wxGetCwd();
	}
	// wxPATH_NORM_ALL is deprecated in wx3 — use explicit flags instead (excluding wxPATH_NORM_ENV_VARS)
	const int flags = wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_CASE | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_LONG | wxPATH_NORM_SHORTCUT;
	fn.Normalize(flags, cwd);
	return fn.GetFullPath();
}


void CDirectoryTreeCtrl::UpdateSharedDirectories()
{
	// ugly hack to at least show shared dirs in remote gui
	if (m_IsRemote) {
		DeleteChildren(m_root);
		for (SharedMap::iterator it = m_lstShared.begin(); it != m_lstShared.end(); ++it) {
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

		const CPath fullPath = GetFullPath(hItem);
		if (bChecked) {
			AddShare(fullPath);
		} else {
			DelShare(fullPath);
			// Double-clicking a recursive-share root must also drop
			// the recursive intent, otherwise the expansion task at
			// commit time would re-flatten the subtree and the user
			// would see the files reappear in the shared list. Calls
			// from MarkChildren on descendants of a recursive root
			// are harmless no-ops here (only the root is keyed in
			// m_lstSharedRecursive).
			DelRecursiveShare(fullPath);
		}

		if (!recursed) {
			UpdateParentItems(hItem, bChecked);
		}
	}
}


bool CDirectoryTreeCtrl::IsShared(const CPath& path)
{
	wxCHECK_MSG(path.IsOk(), false, "Invalid path in IsShared");

	return m_lstShared.find(GetKey(path)) != m_lstShared.end();
}


void CDirectoryTreeCtrl::AddShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), "Invalid path in AddShare");

	if (IsShared(path)) {
		return;
	}

	m_lstShared.insert(SharedMapItem(GetKey(path), path));
}


void CDirectoryTreeCtrl::DelShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), "Invalid path in DelShare");

	m_lstShared.erase(GetKey(path));
}


bool CDirectoryTreeCtrl::IsRecursiveShare(const CPath& path)
{
	return m_lstSharedRecursive.find(GetKey(path)) != m_lstSharedRecursive.end();
}


bool CDirectoryTreeCtrl::IsInsideRecursiveShare(const CPath& path)
{
	// True iff `path` is a strict descendant of any recursive-share
	// root. Used by AddChildItem to bold subtree items when the tree
	// is expanded long after the right-click that set the intent.
	if (m_lstSharedRecursive.empty() || !path.IsOk()) {
		return false;
	}
	const wxString key = GetKey(path);
	for (SharedMap::const_iterator it = m_lstSharedRecursive.begin();
		it != m_lstSharedRecursive.end(); ++it)
	{
		const wxString rootKey = it->first;
		if (key.length() > rootKey.length()
			&& key.StartsWith(rootKey)
			&& (rootKey.empty()
				|| rootKey.Last() == wxFileName::GetPathSeparator()
				|| key[rootKey.length()] == wxFileName::GetPathSeparator()))
		{
			return true;
		}
	}
	return false;
}


void CDirectoryTreeCtrl::AddRecursiveShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), "Invalid path in AddRecursiveShare");

	const wxString key = GetKey(path);
	m_lstSharedRecursive.insert(SharedMapItem(key, path));
}


void CDirectoryTreeCtrl::DelRecursiveShare(const CPath& path)
{
	wxCHECK_RET(path.IsOk(), "Invalid path in DelRecursiveShare");

	m_lstSharedRecursive.erase(GetKey(path));
}


void CDirectoryTreeCtrl::DelSharesUnder(const CPath& root)
{
	if (!root.IsOk() || m_lstShared.empty()) {
		return;
	}

	// Compare on the normalized form, with a trailing separator so
	// /home doesn't also match /home2. If the root happens to end in
	// a separator already (e.g. the Windows drive root "C:\"), we
	// use it as-is.
	wxString prefix = GetKey(root);
	if (prefix.empty()) {
		return;
	}
	if (prefix.Last() != wxFileName::GetPathSeparator()) {
		prefix += wxFileName::GetPathSeparator();
	}

	for (SharedMap::iterator it = m_lstShared.begin();
		it != m_lstShared.end(); )
	{
		if (it->first.StartsWith(prefix)) {
			it = m_lstShared.erase(it);
		} else {
			++it;
		}
	}
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
