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

#include "DirectoryTreeCtrl.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for amuleSpecial
#include "treebasc.h"		// Needed for wxCTreeEvent

// CDirectoryTreeCtrl

#undef HTREEITEM
#define HTREEITEM wxTreeItemId

IMPLEMENT_DYNAMIC_CLASS(CDirectoryTreeCtrl, wxCTreeCtrl)
CDirectoryTreeCtrl::CDirectoryTreeCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: wxCTreeCtrl(parent,id,pos,siz,flags,wxDefaultValidator,"ShareTree"),
m_image(16,16)
{
	//m_SharedMenu.m_hMenu = NULL;
	m_bSelectSubDirs = false;
	Init();
}

CDirectoryTreeCtrl::~CDirectoryTreeCtrl()
{
}


BEGIN_EVENT_TABLE(CDirectoryTreeCtrl, wxCTreeCtrl)
END_EVENT_TABLE()

// CDirectoryTreeCtrl message handlers

void CDirectoryTreeCtrl::OnTvnItemexpanding(wxCTreeEvent& evt)
{
	HTREEITEM hItem = evt.GetItem();
	DeleteChildren(hItem);
	wxString strDir = GetFullPath(hItem);
	AddSubdirectories(hItem, strDir);
	SortChildren(hItem);
}

void CDirectoryTreeCtrl::OnLButtonDown(wxCTreeEvent& evt)
{
	// VQB adjustments to provide for sharing or unsharing of subdirectories when control key is Down
	static int __counter=0;
	HTREEITEM hItem = evt.GetItem(); //HitTest(point, &uFlags);
	GetFirstVisibleItem(); // VQB mark initial window position
	int flags=0;
	HitTest(evt.GetPoint(),flags);
	// this event is launced _after_ checkbox value is set.. if it is set at all
	if ((hItem.IsOk()) && (flags & wxTREE_HITTEST_CHECKBOX /* amule extension */)) {
		CheckChanged(hItem, IsChecked(hItem));
		if (evt.IsControlDown()) { // Is control key down ?
			bool exp;
			exp=false;
			Toggle(hItem);
			HTREEITEM hChild;
			long cookie=993+(++__counter);
			hChild = GetFirstChild(hItem,cookie);//GetChildItem(hItem);
			while (hChild.IsOk()) {
				MarkChildren(hChild,IsChecked(hItem));
				hChild=GetNextSibling(hChild);
			}
			Toggle(hItem);
			// font won't get set otherwise ...
			Toggle(GetItemParent(hItem));
			Toggle(GetItemParent(hItem));
		} 
	} 
	HasChanged = true;
}

void CDirectoryTreeCtrl::MarkChildren(HTREEITEM hChild,bool mark)
{
	static long int __counter=1;

	CheckChanged(hChild, mark);
	SetChecked(hChild,mark);
	bool exp;
	exp=false;
	Toggle(hChild);

	HTREEITEM hChild2;
	long int cookie=++__counter;
	hChild2 = GetFirstChild(hChild,cookie);//GetChildItem(hChild);
	while(hChild2.IsOk()) {
		MarkChildren(hChild2,mark);
		hChild2 = GetNextSibling(hChild2);
	}
	Toggle(hChild);
}

void CDirectoryTreeCtrl::Init(void)
{
	// init image(s)
	m_image.Add(wxBitmap(amuleSpecial(1)));
	m_image.Add(wxBitmap(amuleSpecial(2)));
	SetImageList(&m_image);

	hRoot=AddRoot("/");
	SetItemImage(hRoot, 0);  // yes, this is a folder too
	AppendItem(hRoot,"Cool. Works");

	HasChanged = false;
}

void CDirectoryTreeCtrl::AddChildItem(HTREEITEM hBranch, wxString const& strText)
{
	wxString strDir = GetFullPath(hBranch);
	size_t len = strDir.Len();
	if (hBranch.IsOk() && len > 0 && strDir.c_str()[len - 1] != '/') {
		strDir += "/";
	}
	strDir += strText;

	HTREEITEM item=AppendItem(hBranch,strText);
	SetItemImage(item,0); // it is folder!
	if(HasSharedSubdirectory(strDir)) {
		SetItemBold(item,TRUE);
	}
	if(IsShared(strDir)) {
		SetChecked(item,TRUE);
	}
	if(HasSubdirectories(strDir)) {
		AppendItem(item,"."); // trick. will show +
	}
}

wxString CDirectoryTreeCtrl::GetFullPath(HTREEITEM hItem)
{
	wxString strDir;
	HTREEITEM hSearchItem = hItem;
	// don't traverse to the root item ... it will cause extra / to the path
	while(hSearchItem.IsOk()) {
		strDir = GetItemText(hSearchItem) + "/" + strDir;
		hSearchItem = GetItemParent(hSearchItem);
	}
	return strDir;
}

void CDirectoryTreeCtrl::AddSubdirectories(HTREEITEM hBranch, wxString strDir)
{
	if (strDir.Right(1) != "/") {
		strDir += "/";
	}
	wxString fname;
	wxString dirname=strDir+"*";
	// we must collect values first because we'll call FindFirstFile() again in AddChildItem() ...
	wxArrayString ary;

	fname=wxFindFirstFile(dirname,wxDIR);
	while(!fname.IsEmpty()) {
		if(!wxDirExists(fname)) {
			fname=wxFindNextFile();
			continue;
		}
		if(fname.Find('/',TRUE)!=-1) {
			fname=fname.Mid(fname.Find('/',TRUE)+1);
		}

		ary.Add(fname);
		fname=wxFindNextFile();
	}
	// then add them
	for (unsigned int i = 0; i < ary.GetCount(); ++i) {
		AddChildItem(hBranch, ary[i]);
	}
}

bool CDirectoryTreeCtrl::HasSubdirectories(wxString strDir)
{
	wxLogNull logNo; // prevent stupid log windows if we try to traverse somewhere we have no access.
	if (strDir.Right(1) != "/") {
		strDir += "/";
	}
	wxString fname=wxFindFirstFile(strDir+"*",wxDIR);
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

	wxString tmp;
	for (unsigned int i = 0; i < list->GetCount(); ++i) {
		wxString const* str = &list->Item(i);
		size_t len = str->Len();
		if (len > 0 && str->c_str()[len - 1] != '/')
		{
			tmp = *str;
			tmp += '/';
			str = &tmp;
		}
		m_lstShared.Add(*str);
	}
	if(HasSharedSubdirectory("/")) {
		SetItemBold(hRoot,TRUE);
	}
	if(IsShared("/")) {
		SetChecked(hRoot,TRUE);
	}
}

bool CDirectoryTreeCtrl::HasSharedSubdirectory(wxString const& strDir)
{
	wxChar const* pStrDir = strDir.c_str();				// c_str() is fast (just returns a pointer).
	size_t strDirLen = strDir.Len();
	while(strDirLen > 0 && pStrDir[strDirLen - 1] == '/')
		--strDirLen;						// Minus possible trailing slashes.
	for (unsigned int i = 0; i < m_lstShared.GetCount(); ++i)
	{
		wxString const& str(m_lstShared[i]);
		size_t strLen = str.Len();
		if (strDirLen < strLen - 1)				// Minus 1 strips trailing slash of str.
		{
			wxChar const* pStr = str.c_str();
			if (
#ifdef __UNIX__
			    wxStrncmp(pStrDir, pStr, strDirLen) == 0 &&
#else
			    wxStrnicmp(pStrDir, pStr, strDirLen) == 0 &&
#endif
			    pStr[strDirLen] == '/')
				return true;
		}
	}
	return false;
}

void CDirectoryTreeCtrl::CheckChanged(HTREEITEM hItem, bool bChecked)
{
	wxString strDir = GetFullPath(hItem);
	if (bChecked) {
		AddShare(strDir);
	} else {
		DelShare(strDir);
	}
}

bool CDirectoryTreeCtrl::IsShared(wxString const& strDir)
{
	wxChar const* pStrDir = strDir.c_str();				// c_str() is fast (just returns a pointer).
	size_t strDirLen = strDir.Len();
	while(strDirLen > 1 && pStrDir[strDirLen - 1] == '/')
		--strDirLen;						// Minus possible trailing slashes.
	for (unsigned int i = 0; i < m_lstShared.GetCount(); ++i)
	{
		wxString const& str(m_lstShared[i]);
		if (strDirLen == str.Len() - 1 &&			// Minus 1 because str ends on a slash.
#ifdef __UNIX__
			    wxStrncmp(pStrDir, str.c_str(), strDirLen) == 0
#else
			    wxStrnicmp(pStrDir, str.c_str(), strDirLen) == 0
#endif
		    ) {
			return true;
			}
	}
	return false;
}

void CDirectoryTreeCtrl::AddShare(wxString strDir)
{
	if (strDir.Right(1) != '/') {
		strDir += '/';
	}
	if (IsShared(strDir)) {
		return;
	}
	m_lstShared.Add(strDir);
}

void CDirectoryTreeCtrl::DelShare(wxString strDir)
{
	if (strDir.Right(1) != '/') {
		strDir += '/';
	}
	m_lstShared.Remove(strDir.GetData());
}

void CDirectoryTreeCtrl::UpdateParentItems(HTREEITEM hChild)
{
}

bool CDirectoryTreeCtrl::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()==wxEVT_COMMAND_TREE_ITEM_EXPANDING) {
		OnTvnItemexpanding((wxCTreeEvent&)evt);
		return true;
	}
	if(evt.GetEventType()==wxEVT_COMMAND_TREE_ITEM_LEFT_CLICK /* amule extension */) {
		OnLButtonDown((wxCTreeEvent&)evt);
		return true;
	}
	return wxCTreeCtrl::ProcessEvent(evt);
}
