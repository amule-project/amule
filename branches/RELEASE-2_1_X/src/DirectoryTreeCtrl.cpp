//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/log.h>

#include <wx/imaglist.h>
#include "DirectoryTreeCtrl.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for amuleSpecial
#include <common/StringFunctions.h> // Needed for unicode2char
#include <wx/event.h>

#include "FileFunctions.h"

#ifdef __WXMSW__
	#define ROOT_CHAR	wxT('\\')
	#define ROOT_STRING	wxT("\\")
#else
	#define ROOT_CHAR	wxT('/')
	#define ROOT_STRING	wxT("/")
#endif

// CDirectoryTreeCtrl

IMPLEMENT_DYNAMIC_CLASS(CDirectoryTreeCtrl, wxTreeCtrl)

BEGIN_EVENT_TABLE(CDirectoryTreeCtrl, wxTreeCtrl)
	EVT_TREE_ITEM_ACTIVATED(IDC_SHARESELECTOR,CDirectoryTreeCtrl::OnItemActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(IDC_SHARESELECTOR,CDirectoryTreeCtrl::OnRButtonDown)
	EVT_TREE_ITEM_EXPANDED(IDC_SHARESELECTOR,CDirectoryTreeCtrl::OnTvnItemexpanding)
END_EVENT_TABLE()


class CDirectoryTreeItemData : public wxTreeItemData
{
	public:
		CDirectoryTreeItemData() { Data = 0;};
		~CDirectoryTreeItemData() {};

		void AddCount() { Data++; }
		void SubCount() { Data--; }
		int GetCount() { return Data; }
	private:
		int Data;
};


CDirectoryTreeCtrl::CDirectoryTreeCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: wxTreeCtrl(parent,id,pos,siz,flags,wxDefaultValidator,wxT("ShareTree")),
m_image(16,16)
{
	m_bSelectSubDirs = false;
	Init();
}

CDirectoryTreeCtrl::~CDirectoryTreeCtrl()
{
}

enum {
	IMAGE_FOLDER = 0,
	IMAGE_FOLDER_SUB_SHARED
};

void CDirectoryTreeCtrl::Init(void)
{
	// init image(s)
	m_image.Add(wxBitmap(amuleSpecial(1)));
	m_image.Add(wxBitmap(amuleSpecial(2)));
	SetImageList(&m_image);

	CDirectoryTreeItemData* ItemData = new CDirectoryTreeItemData();
	hRoot=AddRoot(ROOT_STRING,IMAGE_FOLDER,-1,ItemData);
	#ifndef __WXMSW__
		AppendItem(hRoot,wxT("Cool. Works")); // will be deleted on expanding it
	#else
		// Root doesn't show on Windows because I set that flag on muuli. So add Drives here.
		char drive;
		for (drive = 'C'; drive <= 'Z'; drive++) {
			wxString DriveStr = wxString::Format(wxT("%c:"),drive);
			if (CheckDirExists(DriveStr+ROOT_CHAR)) {
				AddChildItem(hRoot,DriveStr);
			}
		}
	#endif

	HasChanged = false;
}

// CDirectoryTreeCtrl message handlers

void CDirectoryTreeCtrl::OnTvnItemexpanding(wxTreeEvent& evt)
{
	wxTreeItemId hItem = evt.GetItem();
	DeleteChildren(hItem);
	AddSubdirectories(hItem, GetFullPath(hItem));
	SortChildren(hItem);
}

void CDirectoryTreeCtrl::OnItemActivated(wxTreeEvent& evt)
{
	// VQB adjustments to provide for sharing or unsharing of subdirectories when control key is Down
	// Kry - No more when is down... right click on image will be the way.
	GetFirstVisibleItem(); // VQB mark initial window position
	int flags=0;
	wxTreeItemId hItem = HitTest(evt.GetPoint(),flags);
	// this event is launced _after_ checkbox value is set.. if it is set at all
	if (hItem.IsOk()) {
		CheckChanged(hItem, !IsBold(hItem));
		Refresh();
	}
	HasChanged = true;
}

void CDirectoryTreeCtrl::OnRButtonDown(wxTreeEvent& evt)
{
	// VQB adjustments to provide for sharing or unsharing of subdirectories when control key is Down
	// Kry - No more when is down... right click on image will be the way.
	wxTreeItemId hItem = evt.GetItem(); 
	GetFirstVisibleItem(); // VQB mark initial window position
	int flags=0;
	HitTest(evt.GetPoint(),flags);
	// this event is launched _after_ checkbox value is set.. if it is set at all
	if (flags & (wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL)) {
		if (hItem.IsOk()) {
			MarkChildren(hItem, !IsBold(hItem));
			Refresh();
		}
	}
	
	HasChanged = true;
}


void CDirectoryTreeCtrl::MarkChildren(wxTreeItemId hChild, bool mark)
{
	// Ensure that children are added, otherwise we might only get a "." entry.
	if (not IsExpanded(hChild) and ItemHasChildren(hChild)) {
		DeleteChildren(hChild);
		AddSubdirectories(hChild, GetFullPath(hChild));
		SortChildren(hChild);
	}
	
	wxTreeItemIdValue cookie;
	wxTreeItemId hChild2 = GetFirstChild(hChild,cookie);
	while (hChild2.IsOk()) {
		MarkChildren(hChild2, mark);
		hChild2 = GetNextSibling(hChild2);
	}

	CheckChanged(hChild, mark);
}


void CDirectoryTreeCtrl::AddChildItem(wxTreeItemId hBranch, const wxString& strText)
{
	wxASSERT(hBranch.IsOk()); // The place to add it is ok
	
	wxString strDir = GetFullPath(hBranch);

	wxASSERT(strText.Find(ROOT_CHAR) == -1); // Folder label has no '/' on the name
	
	strDir += strText + ROOT_CHAR;
	
	CDirectoryTreeItemData* ItemData = new CDirectoryTreeItemData();
	wxTreeItemId item=AppendItem(hBranch,strText,IMAGE_FOLDER,-1,ItemData);
	
	if(IsShared(strDir)) {
		SetItemBold(item,TRUE);
		UpdateParentItems(item,true);
	}
	
	if (HasSharedSubdirectory(strDir)) {
		SetItemImage(item,IMAGE_FOLDER_SUB_SHARED);
	}
	
	if(HasSubdirectories(strDir)) {
		AppendItem(item,wxT(".")); // Trick. will show + if it has subdirs
	}
}

wxString CDirectoryTreeCtrl::GetFullPath(wxTreeItemId hItem)
{
	wxASSERT(hItem.IsOk());
	// don't traverse to the root item ... it will cause extra / to the path
	if (hItem == hRoot) {
		#ifndef __WXMSW__
			return ROOT_STRING;
		#else
			return wxEmptyString;
		#endif
	} else {
		wxString strDir = ROOT_STRING;
		while(hItem.IsOk() && (hItem != hRoot))  {	
			#ifndef __WXMSW__
				strDir = ROOT_STRING + GetItemText(hItem)  + strDir;
			#else
				if (GetItemParent(hItem) != hRoot) {
					strDir = ROOT_STRING+ GetItemText(hItem)  + strDir;
				} else {
					strDir = GetItemText(hItem)  + strDir;
				}			 
			#endif
			hItem = GetItemParent(hItem);		
		}
		// Allways has a '/' at the end
		wxASSERT(strDir.Last() == ROOT_CHAR);
		return strDir;
	}
}

void CDirectoryTreeCtrl::AddSubdirectories(wxTreeItemId hBranch, const wxString& folder)
{
	// we must collect values first because we'll call GetFirstFile() again in AddChildItem() ...
	wxArrayString ary;

	CDirIterator SharedDir(folder); 
	wxString fname = SharedDir.GetFirstFile(CDirIterator::Dir); // We just want dirs
	
	while(!fname.IsEmpty()) {
		if(fname.Find(ROOT_CHAR,TRUE) != -1) {  // starts at end
			// Take just the last folder of the path
			fname=fname.Mid(fname.Find(ROOT_CHAR,TRUE)+1);  
		}
		
		wxASSERT(fname.Len() > 0);
		wxASSERT(fname.Last() != ROOT_CHAR);

		ary.Add(fname);
		fname= SharedDir.GetNextFile();
	}
	
	// then add them
	for (unsigned int i = 0; i < ary.GetCount(); ++i) {
		AddChildItem(hBranch, ary[i]);
	}
}

bool CDirectoryTreeCtrl::HasSubdirectories(const wxString& folder)
{
	wxLogNull logNo; // prevent stupid log windows if we try to traverse somewhere we have no access.

	CDirIterator SharedDir(folder); 
	wxString fname = SharedDir.GetFirstFile(CDirIterator::Dir); // We just want dirs
	
	if(!fname.IsEmpty()) {
		return TRUE; // at least one directory ...
	}
	return FALSE; // no match
}

void CDirectoryTreeCtrl::GetSharedDirectories(wxArrayString* list)
{
	for(unsigned int i = 0; i < m_lstShared.GetCount(); ++i) {
		list->Add(m_lstShared[i]);
	}
}

void CDirectoryTreeCtrl::SetSharedDirectories(wxArrayString* list)
{
	m_lstShared.Clear();

	for (unsigned int i = 0; i < list->GetCount(); ++i) {
		// The references returned by Item, Last or operator[] are not const
		wxString& folder = list->Item(i);
		
		wxASSERT(folder.Len() > 0);

		if (folder != ROOT_STRING) { // no removal for root dir
			while(folder.Len() > 0 && folder.Last() == ROOT_CHAR) {
				folder.RemoveLast();	// Minus possible trailing slashes.
			}
			wxASSERT(folder.Len() > 0); // was modified by while
			folder.Append(ROOT_CHAR);
		}
		m_lstShared.Add(list->Item(i));
	}
	
	
	#ifndef __WXMSW__
		// Has root dir a subdir shared?
		if(HasSharedSubdirectory(ROOT_STRING)) { // root folder
			SetItemImage(hRoot,IMAGE_FOLDER_SUB_SHARED);
		}
		// Is root dir shared?
		if(IsShared(ROOT_STRING)) {
			SetItemBold(hRoot,TRUE);
		}
	#else 
		// On Windows, we have to check every drive
		wxTreeItemId hChild;
		wxTreeItemIdValue cookie;
		hChild = GetFirstChild(hRoot,cookie);
		while (hChild.IsOk()) {
			// Does this drive have shared subfolders?
			if (HasSharedSubdirectory(GetFullPath(hChild))) { 
				SetItemImage(hChild,IMAGE_FOLDER_SUB_SHARED);
			}
			// Is this drive shared?
			if (IsShared(GetFullPath(hChild))) {
				SetItemBold(hChild,TRUE);
			}
			hChild = GetNextSibling(hChild);
		}
	#endif
	
}

bool CDirectoryTreeCtrl::HasSharedSubdirectory(const wxString& strDir)
{
	wxString tStrDir = strDir;

#ifdef __WXMSW__	
	tStrDir.MakeLower();	// Speed reasons
#endif
	
	size_t tStrDirLen = tStrDir.Len(); // Speed reasons
	for (unsigned int i = 0; i < m_lstShared.GetCount(); ++i)
	{
		wxString const& str(m_lstShared[i]);
		if (tStrDirLen < str.Len())	 { // Optimizing speed
#ifdef __WXMSW__
			if (wxString(str).MakeLower().StartsWith(tStrDir)) {
#else
               if (str.StartsWith(tStrDir)) {
#endif
				return true;
			}
		}
	}
	return false;
}

void CDirectoryTreeCtrl::CheckChanged(wxTreeItemId hItem, bool bChecked)
{
	if (bChecked) {
		if (!IsBold(hItem)) {
			SetItemBold(hItem,TRUE);
			AddShare(GetFullPath(hItem)); 
			UpdateParentItems(hItem,true);
		}
	} else {
		if (IsBold(hItem)) {
			SetItemBold(hItem,FALSE);
			DelShare(GetFullPath(hItem)); 
			UpdateParentItems(hItem,false);
		}
	}
}

bool CDirectoryTreeCtrl::IsShared(const wxString& strDir)
{	
#ifdef __WXMSW__
	return (m_lstShared.Index(strDir,FALSE) != wxNOT_FOUND); // case insensitive		
#else
	return (m_lstShared.Index(strDir) != wxNOT_FOUND); 				
#endif
}

void CDirectoryTreeCtrl::AddShare(const wxString& strDir)
{
		
	wxASSERT(strDir.Len() > 0);
	
	if (IsShared(strDir)) {
		return;
	}
	
	m_lstShared.Add(strDir);
}

void CDirectoryTreeCtrl::DelShare(const wxString& strDir)
{
	wxASSERT(strDir.Len() > 0);
	
	m_lstShared.Remove(strDir);
}

void CDirectoryTreeCtrl::UpdateParentItems(wxTreeItemId hChild, bool add)
{
	wxTreeItemId parent = hChild;
	while (parent != GetRootItem()) {
		parent = GetItemParent(parent);
		CDirectoryTreeItemData* parent_data = ((CDirectoryTreeItemData*) GetItemData(parent));
		if (add) {
			parent_data->AddCount();
			if (parent_data->GetCount()==1) {
				SetItemImage(parent,IMAGE_FOLDER_SUB_SHARED);
			}
		} else {
			switch (parent_data->GetCount()) {
				case 0:
					break;
				case 1:
					SetItemImage(parent,IMAGE_FOLDER);
				default:
					parent_data->SubCount();
					break;
			}
		}
	};
}
