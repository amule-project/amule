// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2003, robert rostek - tecxx@rrs.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/log.h>

#include <wx/imaglist.h>
#include "DirectoryTreeCtrl.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for amuleSpecial
#include <wx/event.h>

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
	hRoot=AddRoot(wxT("/"),IMAGE_FOLDER,-1,ItemData);
	AppendItem(hRoot,wxT("Cool. Works")); // will be deleted on expanding it

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
	static int __counter=0;
	wxTreeItemId hItem = evt.GetItem(); 
	GetFirstVisibleItem(); // VQB mark initial window position
	int flags=0;
	HitTest(evt.GetPoint(),flags);
	// this event is launched _after_ checkbox value is set.. if it is set at all
	if ((hItem.IsOk()) && (flags &  wxTREE_HITTEST_ONITEMICON)) {
		bool share_it = !IsBold(hItem);
		CheckChanged(hItem, share_it);
		Toggle(hItem);
		wxTreeItemId hChild;
		long cookie=993+(++__counter);
		hChild = GetFirstChild(hItem,cookie);
		int i = 1;
		while (hChild.IsOk()) {
			MarkChildren(hChild,share_it);
			i++;
			hChild=GetNextSibling(hChild);
		}
		Toggle(hItem);
		Refresh();
	}
	HasChanged = true;
}


void CDirectoryTreeCtrl::MarkChildren(wxTreeItemId hChild,bool mark)
{
	static long int __counter=1;

	CheckChanged(hChild, mark);

	Toggle(hChild);

	wxTreeItemId hChild2;
	long int cookie=++__counter;
	hChild2 = GetFirstChild(hChild,cookie);
	int i=0;
	while(hChild2.IsOk()) {
		MarkChildren(hChild2,mark);
		i++;
		hChild2 = GetNextSibling(hChild2);
	}

	Toggle(hChild);
}

void CDirectoryTreeCtrl::AddChildItem(wxTreeItemId hBranch, wxString const& strText)
{
	wxASSERT(hBranch.IsOk()); // The place to add it is ok
	
	wxString strDir = GetFullPath(hBranch);

	wxASSERT(!strDir.IsEmpty()); // non-empty path (cannot add root dir)
	
	wxASSERT(strText.Find(wxT('/')) == -1); // Folder label has no '/' on the name
	
	strDir += strText + wxT('/');
	
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
	wxString strDir;
	wxTreeItemId hSearchItem = hItem;
	// don't traverse to the root item ... it will cause extra / to the path
	while(hSearchItem.IsOk()) {
		strDir = GetItemText(hSearchItem) + wxT("/") + strDir;
		hSearchItem = GetItemParent(hSearchItem);
	}
	// Allways has a '/' at the end
	wxASSERT(strDir.Last() == wxT('/'));
	return strDir;
}

void CDirectoryTreeCtrl::AddSubdirectories(wxTreeItemId hBranch, wxString folder)
{
	
	wxString fname;
	
	// we must collect values first because we'll call FindFirstFile() again in AddChildItem() ...
	wxArrayString ary;

	fname=wxFindFirstFile(folder + wxT('*'),wxDIR);
	
	while(!fname.IsEmpty()) {
		if(!wxDirExists(fname)) {
			fname=wxFindNextFile();
			continue;
		}
		
		if(fname.Find(wxT('/'),TRUE) != -1) {  // starts at end
			// Take just the last folder of the path
			fname=fname.Mid(fname.Find('/',TRUE)+1);  
		}
		
		wxASSERT(fname.Len() > 0);
		wxASSERT(fname.Last() != wxT('/'));

		ary.Add(fname);
		fname=wxFindNextFile();
	}
	
	// then add them
	for (unsigned int i = 0; i < ary.GetCount(); ++i) {
		AddChildItem(hBranch, ary[i]);
	}
}

bool CDirectoryTreeCtrl::HasSubdirectories(wxString folder)
{
	wxLogNull logNo; // prevent stupid log windows if we try to traverse somewhere we have no access.
	
	wxString fname=wxFindFirstFile(folder + wxT("*"),wxDIR);
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

		if (folder.Cmp(wxT("/"))) { // no removal for root dir
			while(folder.Len() > 0 && folder.Last() == wxT('/')) {
				folder.RemoveLast();	// Minus possible trailing slashes.
			}
			wxASSERT(folder.Len() > 0); // was modified by while
			folder.Append(wxT('/'));
		}

		m_lstShared.Add(list->Item(i));
	}
	
	// Has root dir a subdir shared?
	if(HasSharedSubdirectory(wxT("/"))) { // root folder
		SetItemImage(hRoot,IMAGE_FOLDER_SUB_SHARED);
	}
	
	// Is root dir shared?
	if(IsShared(wxT("/"))) {
		SetItemBold(hRoot,TRUE);
	}
	
}

bool CDirectoryTreeCtrl::HasSharedSubdirectory(wxString const& strDir)
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

bool CDirectoryTreeCtrl::IsShared(wxString const& strDir)
{
	wxString tStrDir = strDir;
		
#ifdef __WXMSW__
	return (m_lstShared.Index(tStrDir,FALSE) != wxNOT_FOUND); // case insensitive		
#else
	return (m_lstShared.Index(tStrDir) != wxNOT_FOUND); 				
#endif
}

void CDirectoryTreeCtrl::AddShare(wxString strDir)
{
		
	wxASSERT(strDir.Len() > 0);
	
	if (IsShared(strDir)) {
		return;
	}
	
	m_lstShared.Add(strDir);
}

void CDirectoryTreeCtrl::DelShare(wxString strDir)
{
	wxASSERT(strDir.Len() > 0);
	
	m_lstShared.Remove(strDir);
}

void CDirectoryTreeCtrl::UpdateParentItems(wxTreeItemId hChild, bool add)
{
	wxTreeItemId parent = hChild;
	do {
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
	} while (parent != GetRootItem());
}
