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
#include "amule.h"			// Needed for theApp
#include "color.h"			// Needed for SYSCOLOR
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "Preferences.h"	// Needed for CPreferences

#include <wx/menu.h>

// CSearchListCtrl

// CARE: This will lock the notebook when it does find a control.
//       Call the function UngetSearchListControl() to unlock the notebook. But
//       only after you are done with the Control returned by this function.
//
CSearchListCtrl* GetSearchListControl(long nSearchID)
{
	CMuleNotebook* nb=(CMuleNotebook*)wxWindow::FindWindowById(ID_NOTEBOOK);
	if ( !nb ) return NULL;

	nb->m_LockTabs.Lock();

	for (unsigned int tabCounter=0; tabCounter < nb->GetPageCount(); tabCounter++) {
		if(nb->GetUserData(tabCounter)==nSearchID) {
			return (CSearchListCtrl*)(nb->GetPage(tabCounter));
		}
	}

	nb->m_LockTabs.Unlock();
	return NULL;
}

void UngetSearchListControl(CSearchListCtrl* ctrl)
{
	if ( !ctrl ) return;			// NB was already unlocked

	CMuleNotebook* nb=(CMuleNotebook*)wxWindow::FindWindowById(ID_NOTEBOOK);
	nb->m_LockTabs.Unlock();
}

//IMPLEMENT_DYNAMIC_CLASS(CSearchListCtrl,CMuleListCtrl)

BEGIN_EVENT_TABLE(CSearchListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CSearchListCtrl::OnNMRclick)
	EVT_LEFT_DCLICK(CSearchListCtrl::OnLDclick)
END_EVENT_TABLE()

CSearchListCtrl::CSearchListCtrl(wxWindow* parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags)
{
	wxASSERT( parent );

	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Search") );

	m_SearchFileMenu=NULL;
}

void CSearchListCtrl::Init(CSearchList* in_searchlist)
{
	//SetExtendedStyle(LVS_EX_FULLROWSELECT);
	//ModifyStyle(LVS_SINGLESEL,0);
	searchlist = in_searchlist;
  
	#define LVCFMT_LEFT wxLIST_FORMAT_LEFT
	InsertColumn(0,_("File Name"),LVCFMT_LEFT,500);
	InsertColumn(1,_("Size"),LVCFMT_LEFT,100);
	InsertColumn(2,_("Sources"),LVCFMT_LEFT,50);
	InsertColumn(3,_("Type"),LVCFMT_LEFT,65);
	InsertColumn(4,_("FileID"),LVCFMT_LEFT,280);
	
	LoadSettings();
}

CSearchListCtrl::~CSearchListCtrl()
{
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
		m->AppendSeparator();
		m->Append(MP_FAKECHECK2,_("jugle.net Fake Check")); // deltahf -> fakecheck
		m->Append(MP_FAKECHECK1,_("'Donkey Fakes' Fake Check"));
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
	uint32 newid=InsertItem( GetInsertPos((long)toshow),toshow->GetFileName());
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
}

void CSearchListCtrl::UpdateColor(long index,long WXUNUSED(count))
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
	SortList();
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

void CSearchListCtrl::ShowResults(long nResultsID)
{
	DeleteAllItems();
	m_nResultsID = nResultsID;

	for (POSITION pos = searchlist->list.GetHeadPosition(); pos != 0; searchlist->list.GetNext(pos)) {
		if ( ((CSearchFile*)searchlist->list.GetAt(pos))->GetSearchID() == m_nResultsID) {
			AddResult(searchlist->list.GetAt(pos));
		}
	}
}

int CSearchListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CSearchFile* item1 = (CSearchFile*)lParam1;
	CSearchFile* item2 = (CSearchFile*)lParam2;	
	switch(lParamSort){
		case 0: //filename asc
		    return item1->GetFileName().CmpNoCase(item2->GetFileName());
		case 1000: //filename desc
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
		case 1001: //size desc
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
		case 1002: {  //sources desc
			int cmpS = item2->GetIntTagValue(FT_SOURCES) - item1->GetIntTagValue(FT_SOURCES);
			if (!cmpS) {			
				return(item2->GetIntTagValue(FT_COMPLETE_SOURCES) - item2->GetIntTagValue(FT_COMPLETE_SOURCES));
			} else {
				return (cmpS);
			}	

		}		
		case 3: //type asc
			return GetFiletypeByName(item1->GetFileName()).Cmp(GetFiletypeByName(item2->GetFileName()));

		case 1003: //type  desc
			return GetFiletypeByName(item2->GetFileName()).Cmp(GetFiletypeByName(item1->GetFileName()));

		case 4: //filahash asc
			return memcmp(item1->GetFileHash(),item2->GetFileHash(),16);
		case 1004: //filehash desc
			return memcmp(item2->GetFileHash(),item1->GetFileHash(),16);
		default:
			return 0;
	}
}

bool CSearchListCtrl::ProcessEvent(wxEvent& evt)
{
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
				theApp.amuledlg->LaunchUrl(theApp.GenFakeCheckUrl(file));
				break;
			}
			case MP_FAKECHECK2: {
				theApp.amuledlg->LaunchUrl(theApp.GenFakeCheckUrl2(file));
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


