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


// SharedFilesCtrl.cpp : implementation file
//


#include "muuli_wdr.h"		// Needed for ID_SHFILELIST
#include "SharedFilesCtrl.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CastItoXBytes
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CommentDialog.h"	// Needed for CCommentDialog
#include "PartFile.h"		// Needed for CPartFile
#include "MapKey.h"		// Needed for CCKey
#include "SharedFileList.h"	// Needed for CKnownFileMap
#include "opcodes.h"		// Needed for MP_PRIOVERYLOW
#include "amule.h"		// Needed for theApp
#include "color.h"		// Needed for SYSCOLOR
#include "BarShader.h"

// CSharedFilesCtrl

//IMPLEMENT_DYNAMIC(CSharedFilesCtrl, CMuleListCtrl)

BEGIN_EVENT_TABLE(CSharedFilesCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_SHFILELIST,CSharedFilesCtrl::OnNMRclick)
	EVT_LIST_COL_CLICK(ID_SHFILELIST,CSharedFilesCtrl::OnColumnClick)  
END_EVENT_TABLE()

CSharedFilesCtrl::CSharedFilesCtrl()
{
	sflist = 0;                // i_a 
	memset(&sortstat, 0, sizeof(sortstat));  // i_a 
}

CSharedFilesCtrl::~CSharedFilesCtrl()
{
}

CSharedFilesCtrl::CSharedFilesCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags|wxLC_OWNERDRAW)
{
	sflist = 0;                // i_a 
	memset(&sortstat, 0, sizeof(sortstat));  // i_a 
	m_SharedFilesMenu=NULL;
	Init();
}

void CSharedFilesCtrl::InitSort()
{
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableShared);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableShared);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:20));
}

#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

void CSharedFilesCtrl::Init(){
	InsertColumn(0,CString(_("File Name")) ,LVCFMT_LEFT,250);
	InsertColumn(1,CString(_("Size")),LVCFMT_LEFT,100);
	InsertColumn(2,CString(_("Type")),LVCFMT_LEFT,50);
	InsertColumn(3,CString(_("Priority")),LVCFMT_LEFT,70);
	InsertColumn(4,CString(_("Permission")),LVCFMT_LEFT,100);
	InsertColumn(5,CString(_("FileID")),LVCFMT_LEFT,220);
	InsertColumn(6,CString(_("Requests")),LVCFMT_LEFT,100);
	InsertColumn(7,CString(_("Accepted Requests")),LVCFMT_LEFT,100);
	InsertColumn(8,CString(_("Transferred Data")),LVCFMT_LEFT,120);
	InsertColumn(9,CString(_("Obtained Parts")),LVCFMT_LEFT,120);
}

void CSharedFilesCtrl::OnNMRclick(wxListEvent& evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long item=-1;
	if (!GetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED)) {
		for (;;) {
			item = GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if (item==-1) {
				break;
			}
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
		}
		SetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
	if(m_SharedFilesMenu==NULL) {
		wxMenu* menu=new wxMenu(CString(_("Shared Files")));
		wxMenu* prioMenu=new wxMenu();
		prioMenu->Append(MP_PRIOVERYLOW,CString(_("Very low")));
		prioMenu->Append(MP_PRIOLOW,CString(_("Low")));
		prioMenu->Append(MP_PRIONORMAL,CString(_("Normal")));
		prioMenu->Append(MP_PRIOHIGH,CString(_("High")));
		prioMenu->Append(MP_PRIOVERYHIGH,CString(_("Very High")));
		prioMenu->Append(MP_POWERSHARE,CString(_("PowerShare[Release]"))); //added for powershare (deltaHF)
		prioMenu->Append(MP_PRIOAUTO,CString(_("Auto")));

		wxMenu* permMenu=new wxMenu();
		permMenu->Append(MP_PERMALL,CString(_("Public")));
		permMenu->Append(MP_PERMFRIENDS,CString(_("Friends only")));
		permMenu->Append(MP_PERMNONE,CString(_("Locked")));

		menu->Append(438312,CString(_("Priority")),prioMenu);
		menu->Append(438313,_("Permissions"),permMenu);
		menu->AppendSeparator();
		menu->Append(MP_CMT, CString(_("Change this file's comment...")));
		menu->AppendSeparator();

		menu->Append(MP_GETED2KLINK,_("Copy ED2k &link to clipboard"));
		menu->Append(MP_GETSOURCEED2KLINK,_("Copy ED2k link to clipboard (&Source)"));
		menu->Append(MP_GETHOSTNAMESOURCEED2KLINK,_("Copy ED2k link to clipboard (Hostname)"));
		menu->Append(MP_GETHTMLED2KLINK,_("Copy ED2k link to clipboard (&HTML)"));

		m_SharedFilesMenu=menu;
	}
	PopupMenu(m_SharedFilesMenu,evt.GetPoint());
}

void CSharedFilesCtrl::Localize()
{
}

void CSharedFilesCtrl::ShowFileList(CSharedFileList* in_sflist)
{
	DeleteAllItems();
	sflist = in_sflist;
	for (CKnownFileMap::iterator pos = sflist->m_Files_map.begin();pos != sflist->m_Files_map.end();pos++) {
		ShowFile(pos->second);
	}
}

void CSharedFilesCtrl::RemoveFile(CKnownFile *toRemove)
{
	int nItem=FindItem(-1,(long)toRemove);
	if(nItem!=-1) {
		DeleteItem(nItem);
	}
	ShowFilesCount();
}

void CSharedFilesCtrl::UpdateFile(CKnownFile* file,uint32 itemnr)
{
	CString buffer;
	SetItemData(itemnr,(long)file);
	SetItem(itemnr,1,CastItoXBytes(file->GetFileSize()));
	SetItem(itemnr,2,GetFiletypeByName(file->GetFileName()));  // added by InterCeptor (show filetype) 11.11.02
	if ((file->IsAutoUpPriority())&&(theApp.glob_prefs->GetNewAutoUp())) {
		switch (file->GetUpPriority()) {
			case PR_LOW: {
				SetItem(itemnr,3,CString(_("Auto [Lo]")));
				break;					
			}
			case PR_NORMAL : {
				SetItem(itemnr,3,CString(_("Auto [No]")));
				break;
			}
			case PR_HIGH : {
				SetItem(itemnr,3,CString(_("Auto [Hi]")));
				break;
			}
			case PR_VERYHIGH : {
				SetItem(itemnr,3,CString(_("Auto [Re]")));
				break;
			}
			default:
				SetItem(itemnr,3,CString(_("Auto [UNK]")));
				break;
		}
	} else {
		switch (file->GetUpPriority()) {
			case PR_VERYLOW : {
				SetItem(itemnr,3,CString(_("Very low")));
				break;
			}
			case PR_LOW : {
				SetItem(itemnr,3,CString(_("Low")));
				break;
			}
			case PR_NORMAL : {
				SetItem(itemnr,3,CString(_("Normal")));
				break;
			}
			case PR_HIGH : {
				SetItem(itemnr,3,CString(_("High")));
				break;
			}
			case PR_VERYHIGH : {
				SetItem(itemnr,3,CString(_("Very High")));
				break;
			}
			case PR_POWERSHARE : { //added for powershare (deltaHF)
				SetItem(itemnr,3,CString(_("PowerShare[Release]")));
				break; //end
			}	
			default:
				SetItem(itemnr,3,CString(_("Unknown")));
				break;
		}
	}

	if (file->GetPermissions() == PERM_NOONE) {
		SetItem(itemnr,4,CString(_("Hidden")));
	} else if (file->GetPermissions() == PERM_FRIENDS) {
		SetItem(itemnr,4,CString(_("Friends only")));
	} else {
		SetItem(itemnr,4,CString(_("Public")));
	}

	SetItem(itemnr,5,EncodeBase16(file->GetFileHash(), 16));

	buffer.Format("%u (%u)",file->statistic.GetRequests(),file->statistic.GetAllTimeRequests());SetItem(itemnr,6,buffer);
	buffer.Format("%u (%u)",file->statistic.GetAccepts(),file->statistic.GetAllTimeAccepts());SetItem(itemnr,7,buffer);
	buffer.Format("%s (%s)",CastItoXBytes(file->statistic.GetTransfered()).GetData(), CastItoXBytes(file->statistic.GetAllTimeTransfered()).GetData());SetItem(itemnr,8,buffer);
}

void CSharedFilesCtrl::ShowFile(CKnownFile* file)
{
	ShowFile(file,GetItemCount());
}

void CSharedFilesCtrl::ShowFile(CKnownFile* file,uint32 itemnr)
{
	int newitem=InsertItem(itemnr,file->GetFileName());
	SetItemData(newitem,(long)file);
	// set background... 
	wxListItem myitem;
	myitem.m_itemId=newitem;
	myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
	SetItem(myitem);
	UpdateFile(file,itemnr);
	ShowFilesCount();
}

//bool CSharedFilesCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
bool CSharedFilesCtrl::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}

	wxCommandEvent& event=(wxCommandEvent&)evt;

	UINT selectedCount=this->GetSelectedItemCount();
	int iSel = GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (iSel != (-1)) {
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		switch (event.GetId()) {
			case MP_GETED2KLINK: {
				if(selectedCount > 1) {
					int i = iSel;
					wxString str;
					do {
						CKnownFile* file2 = (CKnownFile*)GetItemData(i);
						str += theApp.CreateED2kLink(file2) + "\n";
					} while ((i=GetNextItem(i,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1);
					theApp.CopyTextToClipboard(str);
					break;
				}
				theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
				return true;
				break;
			}
			case MP_GETSOURCEED2KLINK: {
				if(selectedCount > 1) {
					int i = iSel;
					wxString str;
					do {
						CKnownFile* file2 = (CKnownFile*)GetItemData(i);
						str += theApp.CreateED2kSourceLink(file2) + "\n";
					} while ((i=GetNextItem(i,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1);
					theApp.CopyTextToClipboard(str);
					break;
				}
				theApp.CopyTextToClipboard(theApp.CreateED2kSourceLink(file));
				return true;
				break;
			}
			case MP_GETHOSTNAMESOURCEED2KLINK: {
				if(selectedCount > 1) {
					int i = iSel;
					wxString str;
					do {
						CKnownFile* file2 = (CKnownFile*)GetItemData(i);
						str += theApp.CreateED2kHostnameSourceLink(file2) + "\n";
					} while ((i=GetNextItem(i,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1);
					theApp.CopyTextToClipboard(str);
					break;
				}
				theApp.CopyTextToClipboard(theApp.CreateED2kHostnameSourceLink(file));
				return true;
				break;
			}
			case MP_GETHTMLED2KLINK: {
				if(selectedCount > 1) {
					int i = iSel;
					wxString str;
					do {
						CKnownFile* file2 = (CKnownFile*)GetItemData(i);
						str += theApp.CreateHTMLED2kLink(file2) + "\n";
					} while ((i=GetNextItem(i,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1);
					theApp.CopyTextToClipboard(str);
					break;
				}
				theApp.CopyTextToClipboard(theApp.CreateHTMLED2kLink(file));
				return true;
				break;
			}
			case MP_PERMNONE: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					if (((CPartFile*)file)->IsPartFile()) {
						wxMessageBox(CString(_("You cannot change permissions while a file is still downloading!")));
					} else {
						file->SetPermissions(PERM_NOONE);
						SetItem(iSel,4,CString(_("Hidden")));
					}
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PERMFRIENDS: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					if (((CPartFile*)file)->IsPartFile()) {
						wxMessageBox(CString(_("You cannot change permissions while a file is still downloading!")));
					} else {
						file->SetPermissions(PERM_FRIENDS);
						SetItem(iSel,4,CString(_("Friends only")));
					}
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PERMALL: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetPermissions(PERM_ALL);
					SetItem(iSel,4,CString(_("Public")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_CMT: {
				CCommentDialog dialog(this,file);
				dialog.ShowModal();
				return true;
				break; 
			}
			case MP_PRIOVERYLOW: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_VERYLOW);
					SetItem(iSel,3,CString(_("Very low")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PRIOLOW: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_LOW);
					SetItem(iSel,3,CString(_("Low")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PRIONORMAL: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_NORMAL);
					SetItem(iSel,3,CString(_("Normal")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PRIOHIGH: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_HIGH);
					SetItem(iSel,3,CString(_("High")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PRIOVERYHIGH: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_VERYHIGH);
					SetItem(iSel,3,CString(_("Very High")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_POWERSHARE: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(false);
					file->SetUpPriority(PR_POWERSHARE);
					SetItem(iSel,3,CString(_("PowerShare[Release]")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
			case MP_PRIOAUTO: {
				long pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				while( pos != (-1) ) {
					int iSel=pos;
					file = (CKnownFile*)this->GetItemData(iSel);
					file->SetAutoUpPriority(true);
					file->UpdateAutoUpPriority();
					SetItem(iSel,3,CString(_("Auto [No]")));
					pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				}
				return true;
				break;
			}
		}
	}
	// Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);
}

//void CSharedFilesCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){
void CSharedFilesCtrl::OnColumnClick(wxListEvent& evt)
{
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableShared);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableShared);
	bool sortAscending = (sortItem != evt.GetColumn()) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = evt.GetColumn();

	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableShared, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableShared, sortAscending);

        // Ornis 4-way-sorting
	int adder=0;
	if (evt.GetColumn()>5 && evt.GetColumn()<9) {
		if (!sortAscending) {
			sortstat[evt.GetColumn()-6]=!sortstat[evt.GetColumn()-6];
		}
		adder=sortstat[evt.GetColumn()-6] ? 0:100;
	}
	// Sort table
	if (adder==0) {
		SetSortArrow(sortItem, sortAscending);
	} else {
		SetSortArrow(sortItem, sortAscending); // ? arrowDoubleUp : arrowDoubleDown);
	}
	SortItems(SortProc, sortItem + adder + (sortAscending ? 0:20));
}

int CSharedFilesCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CKnownFile* item1 = (CKnownFile*)lParam1;
	CKnownFile* item2 = (CKnownFile*)lParam2;	
	switch(lParamSort){
		case 0: //filename asc
			return strcasecmp(item1->GetFileName().c_str(),item2->GetFileName().c_str());
		case 20: //filename desc
			return strcasecmp(item2->GetFileName().c_str(),item1->GetFileName().c_str());

		case 1: //filesize asc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);

		case 21: //filesize desc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item2->GetFileSize()>item1->GetFileSize()?1:-1);


		case 2: //filetype asc
			return strcasecmp( GetFiletypeByName(item1->GetFileName()).c_str(),GetFiletypeByName( item2->GetFileName()).c_str() );
		case 22: //filetype desc
			return strcasecmp( GetFiletypeByName(item2->GetFileName()).c_str(),GetFiletypeByName( item1->GetFileName()).c_str() );

		case 3: //prio asc
			if(item1->GetUpPriority() == PR_VERYLOW )
				return 1;
			else if (item2->GetUpPriority() == PR_VERYLOW)
				return 0;
			else
				return item2->GetUpPriority()-item1->GetUpPriority();
		case 23: //prio desc
			if(item1->GetUpPriority() == PR_VERYLOW )
				return 0;
			else if (item2->GetUpPriority() == PR_VERYLOW)
				return 1;
			else
				return item1->GetUpPriority()-item2->GetUpPriority();

		case 4: //permission asc
			return item2->GetPermissions()-item1->GetPermissions();
		case 24: //permission desc
			return item1->GetPermissions()-item2->GetPermissions();

		case 5: //fileID asc
			return strcasecmp((char*)item1->GetFileHash(),(char*)item2->GetFileHash());
		case 25: //fileID desc
			return strcasecmp((char*)item2->GetFileHash(),(char*)item1->GetFileHash());

		case 6: //requests asc
			return item1->statistic.GetRequests() - item2->statistic.GetRequests();
		case 26: //requests desc
			return item2->statistic.GetRequests() - item1->statistic.GetRequests();
		case 7: //acc requests asc
			return item1->statistic.GetAccepts() - item2->statistic.GetAccepts();
		case 27: //acc requests desc
			return item2->statistic.GetAccepts() - item1->statistic.GetAccepts();
		case 8: //all transferred asc
			return item1->statistic.GetTransfered()==item2->statistic.GetTransfered()?0:(item1->statistic.GetTransfered()>item2->statistic.GetTransfered()?1:-1);
		case 28: //all transferred desc
			return item1->statistic.GetTransfered()==item2->statistic.GetTransfered()?0:(item2->statistic.GetTransfered()>item1->statistic.GetTransfered()?1:-1);

		case 10: //folder asc
			return strcasecmp((CString)item1->GetPath(),(CString)item2->GetPath());
		case 30: //folder desc
			return strcasecmp((CString)item2->GetPath(),(CString)item1->GetPath());


		case 106: //all requests asc
			return item1->statistic.GetAllTimeRequests() - item2->statistic.GetAllTimeRequests();
		case 126: //all requests desc
			return item2->statistic.GetAllTimeRequests() - item1->statistic.GetAllTimeRequests();
		case 107: //all acc requests asc
			return item1->statistic.GetAllTimeAccepts() - item2->statistic.GetAllTimeAccepts();
		case 127: //all acc requests desc
			return item2->statistic.GetAllTimeAccepts() - item1->statistic.GetAllTimeAccepts();
		case 108: //all transferred asc
			return item1->statistic.GetAllTimeTransfered()==item2->statistic.GetAllTimeTransfered()?0:(item1->statistic.GetAllTimeTransfered()>item2->statistic.GetAllTimeTransfered()?1:-1);
		case 128: //all transferred desc
			return item1->statistic.GetAllTimeTransfered()==item2->statistic.GetAllTimeTransfered()?0:(item2->statistic.GetAllTimeTransfered()>item1->statistic.GetAllTimeTransfered()?1:-1);

		default: 
			return 0;
	}
}

void CSharedFilesCtrl::UpdateItem(CKnownFile* toupdate){
  //LVFINDINFO find;
  //find.flags = LVFI_PARAM;
  //find.lParam = (LPARAM)toupdate;
  sint16 result = FindItem(-1,(long)toupdate);
  if (result != -1) {
    UpdateFile(toupdate,result);
    //Update(result) ;   // Added by Tarod to real time refresh - DonGato - 11/11/2002
    theApp.amuledlg->sharedfileswnd->Check4StatUpdate(toupdate);
  }
}

void CSharedFilesCtrl::ShowFilesCount() {
	wxString fmtstr = wxString::Format(_("Shared Files (%i)"), GetItemCount());
	wxStaticCast(FindWindowByName(wxT("sharedFilesLabel")),wxStaticText)->SetLabel(fmtstr); 
}

void CSharedFilesCtrl::OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted)
{
	CKnownFile *file = (CKnownFile*)GetItemData(item);
	assert(file != NULL);

	if ( highlighted ) {
		wxColour newcol;
		wxBrush hilBrush;

		if (GetFocus()) {
			wxColour col=wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
			newcol=wxColour(G_BLEND(col.Red(),125),
				   G_BLEND(col.Green(),125),
				   G_BLEND(col.Blue(),125));
			hilBrush = wxBrush(newcol,wxSOLID);
			dc->SetBackground(hilBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		} else {
			wxColour col=wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
  			newcol=wxColour(G_BLEND(col.Red(),125),
				  G_BLEND(col.Green(),125),
				  G_BLEND(col.Blue(),125));
 			hilBrush=wxBrush(newcol,wxSOLID);
			dc->SetBackground(hilBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		}

		newcol = wxColour(((int)newcol.Red()*65)/100,
			          ((int)newcol.Green()*65)/100,
			          ((int)newcol.Blue()*65)/100);
		dc->SetPen(wxPen(newcol,1,wxSOLID));
	} else {
		dc->SetBackground(*(wxTheBrushList->FindOrCreateBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX),wxSOLID)));
		dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		dc->SetPen(*wxTRANSPARENT_PEN);
	}
	dc->SetBrush(dc->GetBackground());
	dc->DrawRectangle(rectHL);
	dc->SetPen(*wxTRANSPARENT_PEN);

	wxRect columnRect = rect;
	#define SPARE_PIXELS_HORZ	4
	#define SPARE_PIXELS_VERT_TEXT	3
	columnRect.SetLeft(columnRect.GetLeft()+SPARE_PIXELS_HORZ);
	columnRect.SetWidth(columnRect.GetWidth()-2*SPARE_PIXELS_HORZ);
	for ( int iColumn = 0; iColumn < GetColumnCount(); iColumn++ )
	{
		wxListItem columnItem;
		GetColumn(iColumn, columnItem);
		int width = columnItem.GetWidth();
		columnRect.SetWidth(width-2*SPARE_PIXELS_HORZ);
		wxDCClipper clipper(*dc, columnRect);
		switch ( iColumn )
		{
		case 9: {
			s_ShareStatusBar.SetFileSize(file->GetFileSize()); 
			s_ShareStatusBar.SetHeight(columnRect.GetHeight()); 
			s_ShareStatusBar.SetWidth(columnRect.GetWidth()); 
			s_ShareStatusBar.Fill(RGB(255,0,0));

			if ( !file->m_AvailPartFrequency.IsEmpty()) {
				for (int i = 0;i != file->GetPartCount();i++)
					if(file->m_AvailPartFrequency[i] > 0 ){
						DWORD color = RGB(0, (210-(22*(file->m_AvailPartFrequency[i]-1)) <  0)? 0:210-(22*(file->m_AvailPartFrequency[i]-1)), 255);
						s_ShareStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
					}
				}

			s_ShareStatusBar.Draw(dc, columnRect.GetLeft(), columnRect.GetTop(), theApp.glob_prefs->UseFlatBar());
			
			break;
		}
		
		default:
			columnItem.m_col = iColumn;
			columnItem.m_itemId = item;
			GetItem(columnItem);
			dc->DrawText(columnItem.m_text, columnRect.GetLeft(), columnRect.GetTop()+SPARE_PIXELS_VERT_TEXT);
			break;

		}

		columnRect.SetLeft(columnRect.GetLeft()+width);
	}
}
