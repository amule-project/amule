// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


// SearchListCtrl.cpp : implementation file
//


#include "muuli_wdr.h"		// Needed for ID_SERVERLIST
#include "SearchListCtrl.h"	// Interface declarations
#include "PartFile.h"		// Needed for CPartFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "otherfunctions.h"	// Needed for CastItoXBytes
#include "SearchList.h"		// Needed for CSearchFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "opcodes.h"		// Needed for MP_RESUME
#include "amule.h"		// Needed for theApp
#include "color.h"		// Needed for SYSCOLOR
#include "MuleNotebook.h"	// Needed for CMuleNotebook

// CSearchListCtrl

//IMPLEMENT_DYNAMIC_CLASS(CSearchListCtrl,CMuleListCtrl)

BEGIN_EVENT_TABLE(CSearchListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CSearchListCtrl::OnNMRclick)
	EVT_LEFT_DCLICK(CSearchListCtrl::OnLDclick)
	EVT_LIST_COL_CLICK(ID_SERVERLIST,CSearchListCtrl::OnColumnClick)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CSearchListCtrl, CMuleListCtrl)

CSearchListCtrl::CSearchListCtrl()
{
	memset(&asc_sort,0,6);
}

CSearchListCtrl::CSearchListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags)
{
	memset(&asc_sort,0,6);
	m_SearchFileMenu=NULL;
}

void CSearchListCtrl::Init(CSearchList* in_searchlist)
{
	//SetExtendedStyle(LVS_EX_FULLROWSELECT);
	//ModifyStyle(LVS_SINGLESEL,0);
	searchlist = in_searchlist;
  
	#define LVCFMT_LEFT wxLIST_FORMAT_LEFT
	InsertColumn(0,CString(_("File Name")),LVCFMT_LEFT,500);
	InsertColumn(1,CString(_("Size")),LVCFMT_LEFT,100);
	InsertColumn(2,CString(_("Sources")),LVCFMT_LEFT,50);
	InsertColumn(3,CString(_("Type")),LVCFMT_LEFT,65);
	InsertColumn(4,CString(_("FileID")),LVCFMT_LEFT,280);
	
	// contrary to other lists, here we can set everything in constructor
	// as this control is created only in run-time
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableSearch);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableSearch);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:10));
}

CSearchListCtrl::~CSearchListCtrl()
{
}

void CSearchListCtrl::OnColumnClick(wxListEvent& evt)
{
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableSearch);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableSearch);
	bool sortAscending = (sortItem != evt.GetColumn()) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = evt.GetColumn();

	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableSearch, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableSearch, sortAscending);

	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:10));
}

void CSearchListCtrl::OnNMRclick(wxMouseEvent& evt)
{

	// Check if clicked item is selected. If not, unselect all and select it.
	int lips = 0;
	long item=-1;
	int index=HitTest(evt.GetPosition(), lips);
	if (!GetItemState(index, wxLIST_STATE_SELECTED)) {
		for (;;) {
			item = GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if (item==-1) {
				break;
			}
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
		}
		SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
	// create popup-menu
	if(m_SearchFileMenu==NULL) {
		wxMenu* m=new wxMenu(_("File"));
		m->Append(MP_RESUME,_("Download"));
		m->Append(MP_GETED2KLINK,_("Copy ED2k link to clipboard"));
		m->Append(MP_GETHTMLED2KLINK,_("Copy ED2k link to clipboard (HTML)"));
		m->Append(MP_FAKECHECK1,_("Check Fake")); // deltahf -> fakecheck
		m->AppendSeparator();
		//  Removing this entry cause nobody knows why its here :)
		//  Besides, it crashes amule.
		//  m->Append(MP_REMOVESELECTED,_("Remove Selected"));
		m->Append(MP_REMOVE,_("Close this search result"));
		m->Append(MP_REMOVEALL,_("Clear All"));
		m_SearchFileMenu=m;
	}
	PopupMenu(m_SearchFileMenu,evt.GetPosition());
}

void CSearchListCtrl::OnLDclick(wxMouseEvent& event)
{
	int lips=0;
	int index=HitTest(event.GetPosition(),lips);
	if(index>=0) {
		SetItemState(index,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
	}
	wxCommandEvent nulEvt;
	theApp.amuledlg->searchwnd->OnBnClickedSdownload(nulEvt);
}

void CSearchListCtrl::Localize()
{
}

// CSearchListCtrl message handlers

void CSearchListCtrl::AddResult(CSearchFile* toshow)
{
	if (toshow->GetSearchID() != m_nResultsID) {
		return;
	}
	uint32 itemnr=GetItemCount();
	uint32 newid=InsertItem(itemnr,toshow->GetFileName());
	SetItemData(newid,(long)toshow);
	char buffer[50];
	uint32 filesize=toshow->GetIntTagValue(FT_FILESIZE);
	SetItem(newid,1,CastItoXBytes(filesize));
	sprintf(buffer,"%d (%d)",toshow->GetIntTagValue(FT_SOURCES), toshow->GetIntTagValue(FT_COMPLETE_SOURCES));
	SetItem(newid,2,char2unicode(buffer));
	wxString pim=toshow->GetFileName();
	SetItem(newid,3,GetFiletypeByName(pim));
	buffer[0]=0;
	for(uint16 i=0;i!=16;i++) {
		sprintf(buffer,"%s%02X",buffer,toshow->GetFileHash()[i]);
	}
	SetItem(newid,4,char2unicode(buffer));
	// set color
	UpdateColor(newid,toshow->GetIntTagValue(FT_SOURCES));
	// Re-sort the items after each added item to keep the sort order.
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableSearch);
	bool SortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableSearch);
	SortItems(SortProc, sortItem + (SortAscending ? 0:10));
}

void CSearchListCtrl::UpdateColor(long index,long count)
{
	wxListItem item;
	item.m_itemId=index;
	item.m_col=1;
	item.m_mask=wxLIST_MASK_STATE|wxLIST_MASK_TEXT|wxLIST_MASK_IMAGE|wxLIST_MASK_DATA|wxLIST_MASK_WIDTH|wxLIST_MASK_FORMAT;
	if(GetItem(item)) {
		wxColour newcol;
		newcol=wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		CSearchFile* file=(CSearchFile*)GetItemData(index); //item.GetData();
		if(!file) {
			return;
		}
		CKnownFile* sameFile=theApp.sharedfiles->GetFileByID(file->GetFileHash());
		if(!sameFile) {
			sameFile=theApp.downloadqueue->GetFileByID(file->GetFileHash());
		}
		int red,green,blue;
		red=newcol.Red();
		green=newcol.Green();
		blue=newcol.Blue();
		if(sameFile) {
			if(sameFile->IsPartFile()) {
				// already downloading
				red=(file->GetSourceCount()+4)*20;
				if(red>255) {
					red=255;
				}
			} else {
				// already downloaded
				green=128;
			}
		} else {
			blue=(file->GetSourceCount()-1)*20;
			if(blue>255) {
				blue=255;
			}
		}
		newcol.Set(red,green,blue);
		item.SetTextColour(newcol);
		// don't forget to set the item data back...
		wxListItem newitem;
		newitem.m_itemId=index;
		//wxColour* jes=new wxColour(red,green,blue);
		newitem.SetTextColour(newcol); //*jes);
		newitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
		SetItem(newitem);
	}
}

void CSearchListCtrl::UpdateSources(CSearchFile* toupdate)
{
	long index=FindItem(-1,(long)toupdate);
	if(index!=(-1)) {
		char buffer[50];
		sprintf(buffer,"%d (%d)",toupdate->GetSourceCount(),toupdate->GetCompleteSourceCount());
		SetItem(index,2,char2unicode(buffer));
		UpdateColor(index,toupdate->GetSourceCount());
	}
  
	// Re-sort the items after each added source to keep the sort order.
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableSearch);
	bool SortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableSearch);
	SortItems(SortProc, sortItem + (SortAscending ? 0:10));
}

void CSearchListCtrl::RemoveResult(CSearchFile* toremove)
{
	//LVFINDINFO find;
	//find.flags = LVFI_PARAM;
	//find.lParam = (LPARAM)toremove;
	sint32 result = FindItem(-1,(long)toremove);
	if(result != (-1)) {
		this->DeleteItem(result);
	}
}

void CSearchListCtrl::ShowResults(uint32 nResultsID)
{
	DeleteAllItems();
	m_nResultsID = nResultsID;
	searchlist->ShowResults(m_nResultsID);
}

int CSearchListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CSearchFile* item1 = (CSearchFile*)lParam1;
	CSearchFile* item2 = (CSearchFile*)lParam2;	
	switch(lParamSort){
		case 0: //filename asc
		    return item1->GetFileName().CmpNoCase(item2->GetFileName());
		case 10: //filename desc
			return item2->GetFileName().CmpNoCase(item1->GetFileName());
		case 1: //size asc
			// Need to avoid nasty bug on files >2.4Gb
			// return item1->GetIntTagValue(FT_FILESIZE) - item2->GetIntTagValue(FT_FILESIZE);
			if (item1->GetIntTagValue(FT_FILESIZE) > item2->GetIntTagValue(FT_FILESIZE)) {
				// item1 > item2
				return 1;
			} else if (item1->GetIntTagValue(FT_FILESIZE) < item2->GetIntTagValue(FT_FILESIZE)) {
				// item1 < item2
				return -1;
			} else {
				// item1 == item2
				return 0;
			}		
		case 11: //size desc
			// Need to avoid nasty bug on files >2.4Gb
			//	return item2->GetIntTagValue(FT_FILESIZE) - item1->GetIntTagValue(FT_FILESIZE);
			if (item2->GetIntTagValue(FT_FILESIZE) > item1->GetIntTagValue(FT_FILESIZE)) {
				// item1 < item2
				return 1;
			} else if (item2->GetIntTagValue(FT_FILESIZE) < item1->GetIntTagValue(FT_FILESIZE)) {
				// item1 > item2
				return -1;
			} else {
				// item1 == item2
				return 0;
			}
		case 2: {  //sources asc
			int cmpS = item1->GetIntTagValue(FT_SOURCES) - item2->GetIntTagValue(FT_SOURCES);
			if (!cmpS) {			
				return(item1->GetIntTagValue(FT_COMPLETE_SOURCES) - item1->GetIntTagValue(FT_COMPLETE_SOURCES));
			} else {
				return (cmpS);
			}	

		}
		case 12: {  //sources desc
			int cmpS = item2->GetIntTagValue(FT_SOURCES) - item1->GetIntTagValue(FT_SOURCES);
			if (!cmpS) {			
				return(item2->GetIntTagValue(FT_COMPLETE_SOURCES) - item2->GetIntTagValue(FT_COMPLETE_SOURCES));
			} else {
				return (cmpS);
			}	

		}		
		case 3: //type asc
			return GetFiletypeByName(item1->GetFileName()).Cmp(GetFiletypeByName(item2->GetFileName()));

		case 13: //type  desc
			return GetFiletypeByName(item2->GetFileName()).Cmp(GetFiletypeByName(item1->GetFileName()));

		case 4: //filahash asc
			return memcmp(item1->GetFileHash(),item2->GetFileHash(),16);
		case 14: //filehash desc
			return memcmp(item2->GetFileHash(),item1->GetFileHash(),16);
		default:
			return 0;
	}
}

bool CSearchListCtrl::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()==wxEVT_COMMAND_LIST_COL_CLICK) {
		// ok, this won't get dispatched through event table
		// so do it manually here
		OnColumnClick((wxListEvent&)evt);
		return CMuleListCtrl::ProcessEvent(evt);
	}
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}

	// Kry - Cleaner to call the overloaded function - Column hiding gets processed there.	
	
	if ((evt.GetId() >= MP_LISTCOL_1) && (evt.GetId() <= MP_LISTCOL_15)) {
		return CMuleListCtrl::ProcessEvent(evt);
	}			
	
	wxCommandEvent& event=(wxCommandEvent&)evt;
	CSearchFile* file ;
	int item;
	item=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (item != (-1)) {
		file = (CSearchFile*)GetItemData(item);
		switch (event.GetId()) {
			case MP_GETED2KLINK: {
				theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
				break;
			}
			case MP_GETHTMLED2KLINK: {
				theApp.CopyTextToClipboard(theApp.CreateHTMLED2kLink(file));
				break;
			}
			case MP_FAKECHECK1: {	// deltahf -> fakecheck
				theApp.LaunchUrl(theApp.GenFakeCheckUrl(file));
				break;
			}
			case MP_RESUME: {
				wxCommandEvent nulEvt;
				theApp.amuledlg->searchwnd->OnBnClickedSdownload(nulEvt);
				break;
			}
			case MP_REMOVEALL: {
				theApp.amuledlg->searchwnd->DeleteAllSearchs();
				break;
			}
			// Nobody knows why this is here, so disabling it until someone comes up with a reason.
			// Besides, it crashes amule.
			/*
			case MP_REMOVESELECTED: {
				//SetRedraw(false);
				Freeze();
				while (item!=(-1)) {
					//pos=GetFirstSelectedItemPosition();
					//item = this->GetNextSelectedItem(pos);
					theApp.searchlist->RemoveResults((CSearchFile*)this->GetItemData(item));
					item=GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				//SetRedraw(true);
				Thaw();
				break;
			} */
		}
	}
	switch (event.GetId()) {
		case MP_REMOVE:
			theApp.amuledlg->searchwnd->DeleteSearch(m_nResultsID);
			break;
		case MP_REMOVEALL:
			theApp.amuledlg->searchwnd->DeleteAllSearchs();
			break;
		default:
			break;
	}
	// calling old is probably a bad idea after all..
	// return CMuleListCtrl::ProcessEvent(evt);
	return true;
}
