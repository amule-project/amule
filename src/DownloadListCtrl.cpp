//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

// DownloadListCtrl.cpp : implementation file



#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for DISABLE_PROGRESS
#endif

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif

#include <algorithm>		// Needed for std::min
#include <wx/font.h>
#include <wx/dcmemory.h>

#include "DownloadListCtrl.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CheckShowItemInGivenCat
#include "amule.h"		// Needed for theApp
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "FriendList.h"		// Needed for CFriendList
#include "PartFile.h"		// Needed for CPartFile
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "FileDetailDialog.h"	// Needed for CFileDetailDialog
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "updownclient.h"	// Needed for CUpDownClient
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "muuli_wdr.h"		// Needed for ID_DLOADLIST
#include "color.h"		// Needed for G_BLEND and SYSCOLOR

#define DLC_BARUPDATE 512
#define strcmpi strcasecmp

class CPartFile;

// CDownloadListCtrl

// hmm some parts here might be overkill ;) ...
// oh well at least I can do/draw/paint everything I want in this window now ;)
IMPLEMENT_DYNAMIC_CLASS(CDownloadListCtrl, CMuleListCtrl)

BEGIN_EVENT_TABLE(CDownloadListCtrl, CMuleListCtrl)
	EVT_LIST_COL_END_DRAG(ID_DLOADLIST, CDownloadListCtrl::OnColResize)
	EVT_LIST_COL_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnColumnClick)
	EVT_LIST_ITEM_ACTIVATED(ID_DLOADLIST, CDownloadListCtrl::OnLvnItemActivate)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnNMRclick)
	EVT_KEY_UP(CDownloadListCtrl::OnKeyUp)
	EVT_KEY_DOWN(CDownloadListCtrl::OnKeyDown)
END_EVENT_TABLE()

void preloadImages(wxImageList * imgs)
{

	imgs->Add(wxBitmap(clientImages(0)));
	imgs->Add(wxBitmap(clientImages(1)));
	imgs->Add(wxBitmap(clientImages(2)));
	imgs->Add(wxBitmap(clientImages(3)));
	imgs->Add(wxBitmap(clientImages(4)));
	imgs->Add(wxBitmap(clientImages(5)));
	imgs->Add(wxBitmap(clientImages(6)));
	imgs->Add(wxBitmap(clientImages(7)));
	imgs->Add(wxBitmap(clientImages(8)));
	imgs->Add(wxBitmap(clientImages(9)));
	imgs->Add(wxBitmap(clientImages(10)));
	imgs->Add(wxBitmap(clientImages(11)));
	imgs->Add(wxBitmap(clientImages(17)));
	imgs->Add(wxBitmap(clientImages(18)));
}

//IMPLEMENT_DYNAMIC(CDownloadListCtrl, CListBox)
CDownloadListCtrl::CDownloadListCtrl():m_ImageList(170, 16)
{
	SetImageList(&m_ImageList, wxIMAGE_LIST_SMALL);
	preloadImages(&m_ImageList);
}

CDownloadListCtrl::CDownloadListCtrl(wxWindow * &parent, int id, const wxPoint & pos, wxSize siz, int flags):CMuleListCtrl(parent, id, pos, siz, flags | wxLC_OWNERDRAW), m_ImageList(170, 16)
{
	m_ClientMenu = NULL;
	m_PrioMenu = NULL;
	m_FileMenu = NULL;
	wxColour col = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	wxColour newcol = wxColour(G_BLEND(col.Red(), 125), G_BLEND(col.Green(), 125), G_BLEND(col.Blue(), 125));
	m_hilightBrush = new wxBrush(newcol, wxSOLID);
	col = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
	newcol = wxColour(G_BLEND(col.Red(), 125), G_BLEND(col.Green(), 125), G_BLEND(col.Blue(), 125));
	m_hilightUnfocusBrush = new wxBrush(newcol, wxSOLID);
	SetImageList(&m_ImageList, wxIMAGE_LIST_SMALL);
	preloadImages(&m_ImageList);
	Init();
}

void CDownloadListCtrl::InitSort()
{
	/* Only load the settings when doing startup time sorting. All other sortings
	   must manage on their own.
	*/
	if (!theApp.amuledlg->IsRunning()) {
		LoadSettings();
	}
	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableDownload);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableDownload);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0 : 100));
	isShift = false;
}

CDownloadListCtrl::~CDownloadListCtrl()
{
	while (m_ListItems.empty() == false) {
		delete m_ListItems.begin()->second;	// second = CtrlItem_Struct*
		m_ListItems.erase(m_ListItems.begin());
	}
	delete m_hilightBrush;
	delete m_hilightUnfocusBrush;
}

void CDownloadListCtrl::HideSources(CPartFile * toCollapse, bool isShift, bool isCtrl, bool isAlt)
{
	Freeze();
	int pre, post;
	pre = post = 0;
	for (int i = 0; i < GetItemCount(); i++) {
		CtrlItem_Struct *item = (CtrlItem_Struct *) this->GetItemData(i);
		if (item->owner == toCollapse) {
			pre++;
			CUpDownClient *cl = (CUpDownClient*)item->value;
			if ((isShift || isCtrl || isAlt) && (cl->GetDownloadFile() == toCollapse)) {
				uint8 ds = cl->GetDownloadState();
				if ((isShift && ds == DS_DOWNLOADING) || (isCtrl && cl->GetRemoteQueueRank() > 0) || (isAlt && ds != DS_NONEEDEDPARTS)) {
					continue;
				}
			}
			item->dwUpdated = 0;
			if (item->status) {
				delete item->status;
			}
			item->status = NULL;	// clear it!!!!
			DeleteItem(i--);
			post++;
		}
	}
	toCollapse->m_bShowOnlyDownloading = isShift;
	if (pre - post == 0) {
		toCollapse->srcarevisible = false;
	}
	Thaw();
	theApp.amuledlg->transfers_frozen = false;
}

void CDownloadListCtrl::collectSelections(CTypedPtrList < CPtrList, CPartFile * >*selectedList)
{
	long item = -1;
	for (;;) {
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1) {
			break;
		}
		if (((CtrlItem_Struct *) this->GetItemData(item))->type == 1) {
			selectedList->Append((CPartFile *) ((CtrlItem_Struct *) this->GetItemData(item))->value);
		}
	}
}

void CDownloadListCtrl::setPri(int newpri)
{
	CTypedPtrList < CPtrList, CPartFile * >selectedList;
	collectSelections(&selectedList);
	long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1) {
		CtrlItem_Struct *content = (CtrlItem_Struct *) this->GetItemData(item);
		UINT selectedCount = this->GetSelectedItemCount();
		if (content->type == 1) {
			CPartFile *file = (CPartFile *) content->value;
			if (selectedCount > 1) {
				while (!selectedList.IsEmpty()) {
					//selectedList.GetHead()->SetPriority(newpri);
					selectedList.GetHead()->SetDownPriority(newpri);
					selectedList.RemoveHead();
				}
				return;
			}
			//file->SetPriority(newpri);
			file->SetDownPriority(newpri);
		}
	}
}

void CDownloadListCtrl::OnPriLow(wxCommandEvent & evt)
{
	setPri(PR_LOW);
}

void CDownloadListCtrl::OnPriNormal(wxCommandEvent & evt)
{
	setPri(PR_NORMAL);
}

void CDownloadListCtrl::OnPriHigh(wxCommandEvent & evt)
{
	setPri(PR_HIGH);
}

// laziness strikes
// defined in winuser.h on MSW - only define here if not defined already
#ifndef MF_CHECKED
	#define MF_CHECKED TRUE
#endif
#ifndef MF_UNCHECKED
	#define MF_UNCHECKED FALSE
#endif
#ifndef MF_ENABLED
	#define MF_ENABLED TRUE
#endif
#ifndef MF_GRAYED
	#define MF_GRAYED FALSE
#endif

void CDownloadListCtrl::OnNMRclick(wxListEvent & evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long item = -1;
	if (!GetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED)) {
		for (;;) {
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == -1) {
				break;
			}
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
		}
		SetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	item = -1;
	for (;;) {
		item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (item == -1) {
			break;
		}
		CtrlItem_Struct *content = (CtrlItem_Struct *) this->GetItemData(item);
		if (content->type == 1) {
			CPartFile *file = (CPartFile *) content->value;
			if (m_PrioMenu == NULL) {
				wxMenu *priomenu = new wxMenu();
				priomenu->AppendCheckItem(MP_PRIOLOW, CString(_("Low")));
				priomenu->AppendCheckItem(MP_PRIONORMAL, CString(_("Normal")));
				priomenu->AppendCheckItem(MP_PRIOHIGH, CString(_("High")));
				priomenu->AppendCheckItem(MP_PRIOAUTO, CString(_("Auto")));
				m_PrioMenu = priomenu;

				wxMenu *menu = new wxMenu(CString(_("Downloads")));
				menu->Append(999989, CString(_("Priority")), priomenu);
				menu->Append(MP_CANCEL, CString(_("Cancel")));
				menu->Append(MP_STOP, CString(_("&Stop")));
				menu->Append(MP_PAUSE, CString(_("&Pause")));
				menu->Append(MP_RESUME, CString(_("&Resume")));
				menu->AppendSeparator();
				/* Razor 1a - Modif by MikaelB
				   Menu items for :
				   - Drop No Needed Sources now
				   - Drop Full Queue Sources now
				   - Drop High Queue Rating Sources now
				   - Clean Up Sources now ( drop NNS, FQS and HQRS )
				   - Swap every A4AF to this file now
				   - Swap every A4AF to this file ( AUTO )
				   - Swap every A4AF to any other file now   */
				wxMenu *extendedmenu = new wxMenu();
				extendedmenu->Append(MP_SWAP_A4AF_TO_THIS, CString(_("Swap every A4AF to this file now")));
				extendedmenu->AppendCheckItem(MP_SWAP_A4AF_TO_THIS_AUTO, CString(_("Swap every A4AF to this file (Auto)")));
				extendedmenu->Append(MP_SWAP_A4AF_TO_ANY_OTHER, CString(_("Swap every A4AF to any other file now")));
				extendedmenu->AppendSeparator();
				extendedmenu->Append(MP_DROP_NO_NEEDED_SOURCES, CString(_("Drop No Needed Sources now")));
				extendedmenu->Append(MP_DROP_FULL_QUEUE_SOURCES, CString(_("Drop Full Queue Sources now")));
				extendedmenu->Append(MP_DROP_HIGH_QUEUE_RATING_SOURCES, CString(_("Drop High Queue Rating Sources now")));
				extendedmenu->Append(MP_CLEAN_UP_SOURCES, CString(_("Clean Up Sources now (NNS, FQS && HQRS)")));

				menu->Append(999989, CString(_("Extended Options")), extendedmenu);
				/* End Modif */
				menu->AppendSeparator();
				menu->Append(MP_OPEN, CString(_("&Open the file")));
				menu->Append(MP_PREVIEW, CString(_("Preview")));

				menu->Append(MP_METINFO, CString(_("Show file &details")));
				menu->Append(MP_VIEWFILECOMMENTS, CString(_("Show all comments")));
				menu->Append(MP_FAKECHECK1, CString(_("Check Fake")));// deltahf -> fakecheck
				menu->AppendSeparator();
				menu->Append(MP_CLEARCOMPLETED, CString(_("C&lear completed")));
			
				menu->Append(MP_GETED2KLINK, CString(_("Copy ED2k &link to clipboard")));
				menu->Append(MP_GETHTMLED2KLINK, CString(_("Copy ED2k link to clipboard (&HTML)")));
				m_FileMenu = menu;

			} else {
				// Remove dynamic entries
				m_FileMenu->Remove(432843);	// Assign category
				m_FileMenu->Remove(MP_TOOGLELIST);
			}
			
			// Add dinamic entries 
			wxMenu *cats = new wxMenu(CString(_("Category")));
			if (theApp.glob_prefs->GetCatCount() > 1) {
				for (int i = 0; i < theApp.glob_prefs->GetCatCount(); i++) {
					cats->Append(MP_ASSIGNCAT + i, (i == 0) ? CString(_("unassign")) : CString(theApp.glob_prefs->GetCategory(i)->title));
				}
			}

			m_FileMenu->Append(432843, CString(_("Assign to category")), cats);
			if (theApp.glob_prefs->GetCatCount() == 1) {
				m_FileMenu->Enable(432843, MF_GRAYED);
			} else {
				m_FileMenu->Enable(432843, MF_ENABLED);
			}

			if (theApp.amuledlg->list_no_refresh) {
				m_FileMenu->Append(MP_TOOGLELIST,CString(_("Show Lists")));
			} else {
				m_FileMenu->Append(MP_TOOGLELIST,CString(_("Hide Lists")));
			}				
					
			// then set state
			wxMenu *menu = m_FileMenu;
			menu->Enable(MP_PAUSE, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_STOP, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_RESUME, ((file->GetStatus() == PS_PAUSED) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_OPEN, ((file->GetStatus() == PS_COMPLETE) ? MF_ENABLED : MF_GRAYED));	//<<--9/21/02
			if (file->IsPartFile()) {
			  wxString preview(_("Preview ["));
			  char* buffer = nstrdup(file->GetPartMetFileName());
			  int n = strlen(buffer);
			  if (n >= 4)
				  buffer[n-4] = 0;
			  menu->SetLabel(MP_PREVIEW, preview + buffer + "]");
			  delete[] buffer;
			}
			menu->Enable(MP_PREVIEW, ((file->PreviewAvailable())? MF_ENABLED : MF_GRAYED));

			/* Razor 1a - Modif by MikaelB
			   Set menu items' state for :
			   - Drop No Needed Sources now
			   - Drop Full Queue Sources now
			   - Drop High Queue Rating Sources now
			   - Clean Up Sources now ( drop NNS, FQS and HQRS )
			   - Swap every A4AF to this file now
			   - Swap every A4AF to this file ( AUTO )
			   - Swap every A4AF to any other file now
			   */

			menu->Enable(MP_DROP_NO_NEEDED_SOURCES, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_DROP_FULL_QUEUE_SOURCES, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_DROP_HIGH_QUEUE_RATING_SOURCES, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_CLEAN_UP_SOURCES, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_SWAP_A4AF_TO_THIS_AUTO, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Check(MP_SWAP_A4AF_TO_THIS_AUTO, file->IsA4AFAuto()? MF_CHECKED : MF_UNCHECKED);
			menu->Enable(MP_SWAP_A4AF_TO_ANY_OTHER, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			/* End modif */

			menu->Enable(MP_CANCEL, MF_ENABLED);
			menu->Enable(MP_TOOGLELIST, MF_ENABLED);

			wxMenu *priomenu = m_PrioMenu;
			//priomenu->Check(MP_PRIOHIGH, (file->GetPriority() == PR_HIGH) ? MF_CHECKED : MF_UNCHECKED);
			//priomenu->Check(MP_PRIONORMAL, (file->GetPriority() == PR_NORMAL) ? MF_CHECKED : MF_UNCHECKED);
			//priomenu->Check(MP_PRIOLOW, (file->GetPriority() == PR_LOW) ? MF_CHECKED : MF_UNCHECKED);
			priomenu->Check(MP_PRIOHIGH, (!file->IsAutoDownPriority() && (file->GetDownPriority() == PR_HIGH)) ? MF_CHECKED : MF_UNCHECKED);
			priomenu->Check(MP_PRIONORMAL, (!file->IsAutoDownPriority() && (file->GetDownPriority() == PR_NORMAL)) ? MF_CHECKED : MF_UNCHECKED);
			priomenu->Check(MP_PRIOLOW, (!file->IsAutoDownPriority() && (file->GetDownPriority() == PR_LOW)) ? MF_CHECKED : MF_UNCHECKED);
			priomenu->Check(MP_PRIOAUTO, file->IsAutoDownPriority() ? MF_CHECKED : MF_UNCHECKED);


			PopupMenu(m_FileMenu, evt.GetPoint());
		} else {
			if (m_ClientMenu == NULL) {
				wxMenu *menu = new wxMenu("Clients");
				menu->Append(MP_DETAIL, CString(_("Show &Details")));
				menu->Append(MP_ADDFRIEND, CString(_("Add to Friends")));
				menu->Append(MP_SHOWLIST, CString(_("View Files")));
				m_ClientMenu = menu;
			}
			PopupMenu(m_ClientMenu, evt.GetPoint());
		}
		// make sure that we terminate
		break;
	}
	if (item == -1) {
		// no selection.. actually this event won't get fired in this case so 
		// do nothing..
	}
}

void CDownloadListCtrl::OnColResize(wxListEvent & evt)
{
	return;
}

void CDownloadListCtrl::OnDrawItem(int item, wxDC * dc, const wxRect & rect, const wxRect & rectHL, bool highlighted)
{
	/* Don't do any drawing if there's nobody to see it. */

	if (!theApp.amuledlg->IsRunning() || (theApp.amuledlg->GetActiveDialog() != 2)) {
		return;
	}
	
	CtrlItem_Struct *content = (CtrlItem_Struct *) GetItemData(item);

	if ((content->type == 1) && (highlighted)) {
		if (GetFocus()) {
			dc->SetBackground(*m_hilightBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		} else {
			dc->SetBackground(*m_hilightUnfocusBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		}
	} else {
		dc->SetBackground(*(wxTheBrushList->FindOrCreateBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX), wxSOLID)));
		dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	}
	/* If we have category, override textforeground with what category tells us. */

	CPartFile *file = (CPartFile *) content->value;
	if (file->GetCategory() > 0) {
		dc->SetTextForeground(theApp.glob_prefs->GetCatColor(file->GetCategory()));
	}
	/* we must fill the background */
	wxPen mypen;
	if (content->type == FILE_TYPE && highlighted) {
		/* set pen so that we'll get nice border */
		wxColour old = GetFocus()? m_hilightBrush->GetColour() : m_hilightUnfocusBrush->GetColour();
		wxColour newcol = wxColour(((int)old.Red() * 65) / 100, ((int)old.Green() * 65) / 100, ((int)old.Blue() * 65) / 100);
		mypen = wxPen(newcol, 1, wxSOLID);
		dc->SetPen(mypen);
	} else {
		if (content->type != FILE_TYPE && highlighted) {
			wxColour old = m_hilightBrush->GetColour();
			wxColour newcol = wxColour(((int)old.Red() * 65) / 100, ((int)old.Green() * 65) / 100, ((int)old.Blue() * 65) / 100);
			mypen = wxPen(newcol, 1, wxSOLID);
			dc->SetPen(mypen);
		} else {
			dc->SetPen(*wxTRANSPARENT_PEN);
		}
	}
	dc->SetBrush(dc->GetBackground());
	/* lagloose: removes flicker
	   Madcat: Breaking rectangle lines :( Figure out a way to keep rectangles, and become my 
	   personal hero :) */
	dc->DrawRectangle(rectHL);
	/* end lagloose */
	dc->SetPen(*wxTRANSPARENT_PEN);
	RECT cur_rec;
	int tree_start = 0, tree_end = 0;
	bool notLast = item + 1 != GetItemCount();
	bool notFirst = item != 0;
	cur_rec.left = rect.x;
	cur_rec.top = rect.y;
	cur_rec.right = rect.x + rect.width;
	cur_rec.bottom = rect.y + rect.height;
	int iOffset = 4;	//dc->GetTextExtent(_T(" "), 1 ).cx*2;
	int iCount = GetColumnCount();	//pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left;
	cur_rec.right -= iOffset;
	cur_rec.left += iOffset;

	if (content->type == 1) {
		for (int iCurrent = 0; iCurrent < iCount; iCurrent++) {
			int iColumn = iCurrent;	//pHeaderCtrl->OrderToIndex(iCurrent);
			wxListItem listitem;
			GetColumn(iColumn, listitem);
			int cx = listitem.GetWidth();
			if (iColumn == 5) {
				int iNextLeft = cur_rec.left + cx;
				//set up tree vars
				cur_rec.left = cur_rec.right + iOffset;
				cur_rec.right = cur_rec.left + std::min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iOffset;
				DrawFileItem(dc, 5, &cur_rec, content);
				cur_rec.left = iNextLeft;
			} else {
				cur_rec.right += cx;
				cur_rec.top += 3;	// will ensure that text is about in the middle ;)
				DrawFileItem(dc, iColumn, &cur_rec, content);
				cur_rec.top -= 3;
				cur_rec.left += cx;
			}
		}
	} else if (content->type == 3 || content->type == 2) {

		for (int iCurrent = 0; iCurrent < iCount; iCurrent++) {

			int iColumn = iCurrent;	//pHeaderCtrl->OrderToIndex(iCurrent);
			wxListItem listitem;
			GetColumn(iColumn, listitem);
			int cx = listitem.GetWidth();

			if (iColumn == 5) {
				int iNextLeft = cur_rec.left + cx;
				//set up tree vars
				cur_rec.left = cur_rec.right + iOffset;
				cur_rec.right = cur_rec.left + std::min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iOffset;
				DrawSourceItem(dc, 5, &cur_rec, content);
				cur_rec.left = iNextLeft;
			} else {
				while (iCurrent < iCount) {
					int iNext = iCurrent + 1;	//pHeaderCtrl->OrderToIndex(iCurrent + 1);
					if (iNext == 1 /*|| iNext == 5 || iNext == 7 || iNext == 8 */ ) {
						wxListItem newlistitem;
						GetColumn(iNext, newlistitem);
						cx += newlistitem.GetWidth();	//GetColumnWidth(iNext);
					} else {
						break;
					}
					iCurrent++;
				}
				cur_rec.right += cx;
				cur_rec.top += 3;	// will ensure that text is about in the middle ;)
				DrawSourceItem(dc, iColumn, &cur_rec, content);
				cur_rec.top -= 3;
				cur_rec.left += cx;
			}
		}
	}

	//draw tree last so it draws over selected and focus (looks better)
	if (tree_start < tree_end) {
		//set new bounds
		RECT tree_rect;
		tree_rect.top = rect.y;	//lpDrawItemStruct->rcItem.top;
		tree_rect.bottom = rect.y + rect.height;	//lpDrawItemStruct->rcItem.bottom;
		tree_rect.left = tree_start;
		tree_rect.right = tree_end;
		// TODO:varmaanki clipper?
		//dc->SetBoundsRect(&tree_rect, DCB_DISABLE);

		//gather some information
		bool hasNext = notLast && ((CtrlItem_Struct *) this->GetItemData(item + 1))->type != 1;
		bool isOpenRoot = hasNext && content->type == 1;
		bool isChild = content->type != 1;
		//might as well calculate these now
		int treeCenter = tree_start + 3;
		int middle = (cur_rec.top + cur_rec.bottom + 1) / 2;

		//set up a new pen for drawing the tree
		wxPen pn;
		//pn.CreatePen(PS_SOLID, 1, dc->GetTextColor());
		pn = *(wxThePenList->FindOrCreatePen(dc->GetTextForeground(), 1, wxSOLID));
		//oldpn = dc->SelectObject(&pn);
		dc->SetPen(pn);

		if (isChild) {
			//draw the line to the status bar
			//dc->MoveTo(tree_end, middle);
			//dc->LineTo(tree_start + 3, middle);
			dc->DrawLine(tree_end, middle, tree_start + 3, middle);

			//draw the line to the child node
			if (hasNext) {
				//dc->MoveTo(treeCenter, middle);
				//dc->LineTo(treeCenter, cur_rec.bottom + 1);
				dc->DrawLine(treeCenter, middle, treeCenter, cur_rec.bottom + 1);
			}
		} else if (isOpenRoot) {
			//draw circle
			RECT circle_rec;
			//COLORREF crBk = dc->GetBkColor();
			wxColour crBk = dc->GetBackground().GetColour();
			circle_rec.top = middle - 2;
			circle_rec.bottom = middle + 3;
			circle_rec.left = treeCenter - 2;
			circle_rec.right = treeCenter + 3;
			dc->DrawLine(circle_rec.left, circle_rec.top, circle_rec.right, circle_rec.top);
			dc->DrawLine(circle_rec.right, circle_rec.top, circle_rec.right, circle_rec.bottom);
			dc->DrawLine(circle_rec.right, circle_rec.bottom, circle_rec.left, circle_rec.bottom);
			dc->DrawLine(circle_rec.left, circle_rec.bottom, circle_rec.left, circle_rec.top);
			//dc->FrameRect(&circle_rec, &CBrush(dc->GetTextColor()));
			//dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(dc->GetTextForeground(),wxSOLID)));

			wxPen oldpen = dc->GetPen();
			dc->SetPen(*(wxThePenList->FindOrCreatePen(crBk, 1, wxSOLID)));
			dc->DrawPoint(circle_rec.left, circle_rec.top);
			dc->DrawPoint(circle_rec.right, circle_rec.top);
			dc->DrawPoint(circle_rec.left, circle_rec.bottom);
			dc->DrawPoint(circle_rec.right, circle_rec.bottom);
			dc->SetPen(oldpen);

			//draw the line to the child node
			if (hasNext) {
				//dc->MoveTo(treeCenter, middle + 3);
				//dc->LineTo(treeCenter, cur_rec.bottom + 1);
				dc->DrawLine(treeCenter, middle + 3, treeCenter, cur_rec.bottom + 1);

			}
		}
		//draw the line back up to parent node
		if (notFirst && isChild) {
			dc->DrawLine(treeCenter, middle, treeCenter, cur_rec.top - 1);
		}
	}
	
}

void CDownloadListCtrl::Init()
{
#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

	InsertColumn(0, CString(_("File Name")), LVCFMT_LEFT, 260);
	InsertColumn(1, CString(_("Size")), LVCFMT_LEFT, 60);
	InsertColumn(2, CString(_("Transferred")), LVCFMT_LEFT, 65);
	InsertColumn(3, CString(_("Completed")), LVCFMT_LEFT, 65);
	InsertColumn(4, CString(_("Speed")), LVCFMT_LEFT, 65);
	InsertColumn(5, CString(_("Progress")), LVCFMT_LEFT, 170);
	InsertColumn(6, CString(_("Sources")), LVCFMT_LEFT, 50);
	InsertColumn(7, CString(_("Priority")), LVCFMT_LEFT, 55);
	InsertColumn(8, CString(_("Status")), LVCFMT_LEFT, 70);
	InsertColumn(9, CString(_("Time Remaining")), LVCFMT_LEFT, 110);
	InsertColumn(10, CString(_("Last Seen Complete")), LVCFMT_LEFT, 220);
	InsertColumn(11, CString(_("Last Reception")), LVCFMT_LEFT, 220);

	curTab = 0;
	last_moment = 0;
}

void CDownloadListCtrl::AddFile(CPartFile * toadd)
{
	CtrlItem_Struct *newitem = new CtrlItem_Struct;
	uint16 itemnr = GetItemCount();
	newitem->owner = NULL;
	newitem->type = FILE_TYPE;
	newitem->value = toadd;
	newitem->status = NULL;
	newitem->parent = NULL;
	newitem->dwUpdated = 0;
	//listcontent.Append(newitem);  
	m_ListItems.insert(ListItemsPair(toadd, newitem));
	//InsertItem(LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)newitem);

	if (CheckShowItemInGivenCat(toadd, curTab)) {
		// rip something off from DrawFileItem()
		CPartFile *pf = (CPartFile *) newitem->value;
		uint32 newid = InsertItem(itemnr, pf->GetFileName());
		SetItemData(newid, (long)newitem);

		wxListItem myitem;
		myitem.m_itemId = newid;

		myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
		SetItem(myitem);
	}
	ShowFilesCount();
}

void CDownloadListCtrl::AddSource(CPartFile * owner, CUpDownClient * source, bool notavailable)
{
	CtrlItem_Struct *newitem = new CtrlItem_Struct;
	newitem->owner = owner;
	newitem->type = (notavailable) ? UNAVAILABLE_SOURCE : AVAILABLE_SOURCE;
	newitem->value = source;
	newitem->status = NULL;
	newitem->dwUpdated = 0;

	//listcontent.Append(newitem);
	// Update cross link to the owner
	ListItems::const_iterator ownerIt = m_ListItems.find(owner);
	//ASSERT(ownerIt != m_ListItems.end());
	CtrlItem_Struct *ownerItem = ownerIt->second;
	//ASSERT(ownerItem->value == owner);
	newitem->parent = ownerItem;

	// The same source could be added a few time but only one time per file 

	// Update the other instances of this source
	bool bFound = false;
	std::pair < ListItems::const_iterator, ListItems::const_iterator > rangeIt = m_ListItems.equal_range(source);
	for (ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++) {
		CtrlItem_Struct *cur_item = it->second;

		// Check if this source has been already added to this file => to be sure
		if (cur_item->owner == owner) {
			// Update this instance with its new setting
			cur_item->type = newitem->type;
			cur_item->dwUpdated = 0;
			bFound = true;
		} else if (notavailable == false) {
			// The state 'Available' is exclusive
			cur_item->type = UNAVAILABLE_SOURCE;
			cur_item->dwUpdated = 0;
		}
	}
	if (bFound == true) {
		delete newitem;
		return;
	}

	m_ListItems.insert(ListItemsPair(source, newitem));

	if (!owner->srcarevisible) {
		return;
	}
	if (owner->m_bShowOnlyDownloading &&
	((source->GetDownloadState() != DS_DOWNLOADING) || (source->GetDownloadFile()!=owner))) {
		return;
	}
	// insert newitem to the display too!
	// find it
	int itemnr = FindItem(-1, (long)ownerItem);
	while (GetItemCount() > itemnr + 1 && ((CtrlItem_Struct *) GetItemData(itemnr + 1))->type != FILE_TYPE) {
		itemnr++;
	}
	int newid = InsertItem(itemnr + 1, "This text is not visible");
	SetItemData(newid, (long)newitem);

	// background.. this should be in a function
	wxListItem newitemL;
	newitemL.m_itemId = newid;

	newitemL.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
	SetItem(newitemL);

}


void CDownloadListCtrl::RemoveSource(CUpDownClient * source, CPartFile * owner)
{
	// Retrieve all entries matching the source
	std::pair < ListItems::iterator, ListItems::iterator > rangeIt = m_ListItems.equal_range(source);
	for (ListItems::iterator it = rangeIt.first; it != rangeIt.second;) {
		CtrlItem_Struct *delItem = it->second;
		if (owner == NULL || owner == delItem->owner) {
			// Remove it from the m_ListItems           
			ListItems::iterator tmp = it;
			it++;
			/*it = */ m_ListItems.erase(tmp);

			//LVFINDINFO find;
			//find.flags = LVFI_PARAM;
			//find.lParam = (LPARAM)delitem;
			//sint16 result = FindItem(&find);
			sint16 result = FindItem(-1, (long)delItem);
			if (result != (-1)) {
				DeleteItem(result);
			}
			delete delItem;
		} else {
			it++;
		}
	}
}

// argh. wxWin lists. remove these!!!
void CDownloadListCtrl::RemoveFile(CPartFile * toremove)
{
	// Retrieve all entries matching the File or linked to the file
	// Remark: The 'asked another files' clients must be removed from here
	//ASSERT(toremove != NULL);
	for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end();) {
		CtrlItem_Struct *delItem = it->second;
		if (delItem->owner == toremove || delItem->value == toremove) {
			// Remove it from the m_ListItems
			ListItems::iterator tmp = it;
			it++;
			/*it = */ m_ListItems.erase(tmp);

			// Remove it from the CListCtrl
			//LVFINDINFO find;
			//find.flags = LVFI_PARAM;
			//find.lParam = (LPARAM)delItem;
			sint16 result = FindItem(-1, (long)delItem);

			if (result != (-1)) {
				DeleteItem(result);
			}
			// finally it could be delete
			delete delItem;
		} else {
			it++;
		}
	}
	ShowFilesCount();
}

void CDownloadListCtrl::UpdateItem(void *toupdate)
{
	if (!(theApp.amuledlg->IsIconized())) {
		// Retrieve all entries matching the source
		std::pair < ListItems::const_iterator, ListItems::const_iterator > rangeIt = m_ListItems.equal_range(toupdate);
		for (ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++) {
			CtrlItem_Struct *updateItem = it->second;

			// Find entry in CListCtrl and update object
			//LVFINDINFO find;
			//find.flags = LVFI_PARAM;
			//find.lParam = (LPARAM)updateItem;
			sint16 result = FindItem(-1, (long)updateItem);
			if (result != (-1)) {
				updateItem->dwUpdated = 0;
			#ifdef __WXMSW__ // Lets hope MSW listctrl can handle it.
				RefreshItem(result);
			#else
				long first = 0, last = 0;
				GetVisibleLines(&first, &last);
				if (result >= first && result <= last) {
					RefreshItem(result);
				}
			#endif
			}
		}
	}
}

void CDownloadListCtrl::DrawFileItem(wxDC * dc, int nColumn, LPRECT lpRect, CtrlItem_Struct * lpCtrlItem)
{
	if (lpRect->left < lpRect->right) {

		// force clipper (clip 2 px more than the rectangle from the right side)
		wxDCClipper clipper(*dc, lpRect->left, lpRect->top, lpRect->right - lpRect->left - 2, lpRect->bottom - lpRect->top);

		CString buffer;
		CPartFile *lpPartFile = (CPartFile *) lpCtrlItem->value;
		switch (nColumn) {

			case 0:{
					// file name
					if (lpPartFile->HasComment() || lpPartFile->HasRating()) {
						int image = 9;
						if (lpPartFile->HasRating()) {
							if (lpPartFile->HasBadRating()) {
								image = 10;
							}
						}
						// it's already centered by OnDrawItem() ... 
						POINT point = { lpRect->left - 4, lpRect->top /*+3 */  };
						//m_ImageList.Draw(dc, image, point, ILD_NORMAL);
						m_ImageList.Draw(image, *dc, point.x, point.y - 1, wxIMAGELIST_DRAW_TRANSPARENT);
						lpRect->left += 9;
						dc->DrawText(lpPartFile->GetFileName(), lpRect->left, lpRect->top);
						lpRect->left -= 9;
					} else {
						dc->DrawText(lpPartFile->GetFileName(), lpRect->left, lpRect->top);
					}
				}
				break;

			case 1:	// size
				buffer.Format("%s", CastItoXBytes(lpPartFile->GetFileSize()).GetData());
				//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			case 2:	// transfered
				buffer.Format("%s", CastItoXBytes(lpPartFile->GetTransfered()).GetData());
				//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);   
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			case 3:	// transfered complete
				buffer.Format("%s", CastItoXBytes(lpPartFile->GetCompletedSize()).GetData());
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			case 4:	// speed
				if (lpPartFile->GetTransferingSrcCount() == 0) {
					buffer = "";
				} else {
#ifdef DOWNLOADRATE_FILTERED
					buffer.Format("%.1f %s", lpPartFile->GetKBpsDown(), "kB/s");
#else
					buffer.Format("%.1f %s", lpPartFile->GetDatarate() / 1024.0f, "kB/s");
#endif
				}
				//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			#ifndef DISABLE_PROGRESS

			case 5:	// progress
				{
					lpRect->bottom--;
					lpRect->top++;
					int iWidth = lpRect->right - lpRect->left;
					int iHeight = lpRect->bottom - lpRect->top;

					// DO NOT DRAW IT ALL THE TIME
					DWORD dwTicks = GetTickCount();
					wxMemoryDC cdcStatus;

					if (lpCtrlItem->dwUpdated + DLC_BARUPDATE < dwTicks || !lpCtrlItem->status || iWidth != lpCtrlItem->status->GetWidth() || !lpCtrlItem->dwUpdated) {
						if (lpCtrlItem->status == NULL) {
							lpCtrlItem->status = new wxBitmap(iWidth, iHeight);
						} else {
							//delete lpCtrlItem->status;
							//lpCtrlItem->status=NULL;
							//lpCtrlItem->status=new wxBitmap(iWidth,iHeight);
							lpCtrlItem->status->Create(iWidth, iHeight);	//SetWidth(iWidth);
						}
						//lpCtrlItem->status->Create(iWidth,iHeight);
						cdcStatus.SelectObject(*(lpCtrlItem->status));

						lpPartFile->DrawStatusBar(&cdcStatus, wxRect(0, 0, iWidth, iHeight), theApp.glob_prefs->UseFlatBar());
						lpCtrlItem->dwUpdated = dwTicks + (rand() % 128);

					} else {
						cdcStatus.SelectObject(*(lpCtrlItem->status));
					}

					dc->Blit(lpRect->left, lpRect->top, iWidth, iHeight, &cdcStatus, 0, 0);
					cdcStatus.SelectObject(wxNullBitmap);

					// ts: Percentage of completing
					float percentage = 100.0*(float)lpPartFile->GetCompletedSize()/(float)lpPartFile->GetFileSize();
					buffer.Format("%.1f %s", percentage, "%");
					int middlex = (lpRect->left + lpRect->right) / 2;
					int middley = (lpRect->bottom + lpRect->top) / 2;
					wxCoord *textwidth, *textheight;
					textwidth = new wxCoord();
					textheight = new wxCoord();
					dc->GetTextExtent(buffer, textwidth, textheight);
					wxColour AktColor = dc->GetTextForeground();
					dc->SetTextForeground(*wxWHITE);
					dc->DrawText(buffer, middlex - (*textwidth / 2), middley - (*textheight / 2));
					dc->SetTextForeground(AktColor);

					lpRect->bottom++;
					lpRect->top--;
				}
				break;

				#endif

			case 6:	// sources
				{
					// Ok, after checking eMule's sources, I'm using my own implementation instead.
					uint16 sc = lpPartFile->GetSourceCount();
					uint16 ncsc = lpPartFile->GetNotCurrentSourcesCount();				

					if(ncsc>0) {
						buffer = buffer + wxString::Format("%i/",sc-ncsc);
					}
					
					buffer = buffer + wxString::Format("%i",sc);
					
					if (lpPartFile->GetSrcA4AFCount()>0 ) {
						buffer = buffer + (wxString::Format("+%i",lpPartFile->GetSrcA4AFCount()));						
					}
					
					buffer = buffer + (wxString::Format(" (%i)",lpPartFile->GetTransferingSrcCount()));
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;
				
			case 7:	// prio
				switch(lpPartFile->GetDownPriority()) {
				case PR_LOW:
					if ( lpPartFile->IsAutoDownPriority() ) {
						dc->DrawText(_("Auto [Lo]"), lpRect->left, lpRect->top);
					} else {
						dc->DrawText(_("Low"), lpRect->left, lpRect->top);
					}
					break;
				case PR_NORMAL:
					if ( lpPartFile->IsAutoDownPriority() ) {
						dc->DrawText(_("Auto [No]"), lpRect->left, lpRect->top);
					} else {
						dc->DrawText(_("Normal"), lpRect->left, lpRect->top);
					}
					break;
				case PR_HIGH:
					if ( lpPartFile->IsAutoDownPriority() ) {
						dc->DrawText(_("Auto [Hi]"), lpRect->left, lpRect->top);
					} else {
						dc->DrawText(_("High"), lpRect->left, lpRect->top);
					}
					break;
				}
				break;

			case 8:	// <<--9/21/02
				buffer.Format("%s", lpPartFile->getPartfileStatus().GetData());
				//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			case 9:	// remaining time & size
				{
					//char bufferSize[50];
					//char bufferTime[50];

					//size 
					uint32 remains;
					remains = lpPartFile->GetFileSize() - lpPartFile->GetCompletedSize();	//<<-- 09/27/2002, CML

					if (remains < 0) {
						remains = 0;
					}
					// time 
					sint32 restTime = lpPartFile->getTimeRemaining();
					buffer.Format("%s (%s)", CastSecondsToHM(restTime).GetData(), CastItoXBytes(remains).GetData());
					if (lpPartFile->GetStatus() == PS_COMPLETING || lpPartFile->GetStatus() == PS_COMPLETE) {
						buffer = "";
					}
					//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;
			case 10:	// last seen complete
				{
					if (lpPartFile->lastseencomplete == 0) {
						buffer = (_("Unknown"));
					} else {
						char tmpstr[512];
						static char const* lastseencomplete_fmt = "%y/%m/%d %H:%M:%S"; // Suppress compiler warning.changed by deltaHF (Georg Ludwig fix)
						strftime(tmpstr, sizeof(tmpstr), lastseencomplete_fmt,
						         localtime(&lpPartFile->lastseencomplete));
						buffer = tmpstr;	//lpPartFile->lastseencomplete.Format("%A, %x, %X");
					}
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;
			case 11:	// last receive
				if (!IsColumnHidden(11)) {
					if (lpPartFile->GetLastChangeDatetime() != 0) {
						char tmpstr[512];
						time_t kello = lpPartFile->GetLastChangeDatetime();
						static char const* lastchangedate_fmt = "%y/%m/%d %H:%M:%S"; // Suppress compiler warning.changed by deltaHF (Georg Ludwig fix)
						strftime(tmpstr, sizeof(tmpstr), lastchangedate_fmt, localtime(&kello));
						buffer = tmpstr;	//lpPartFile->GetLastChangeDatetime().Format( theApp.glob_prefs->GetDateTimeFormat());
					} else
						buffer = "";
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
		}
	}
}

#define ILD_NORMAL wxIMAGELIST_DRAW_TRANSPARENT

void CDownloadListCtrl::DrawSourceItem(wxDC * dc, int nColumn, LPRECT lpRect, CtrlItem_Struct * lpCtrlItem)
{
	if (lpRect->left < lpRect->right) {

		// force clipper (clip 2 px more than the rectangle from the right side)
		wxDCClipper clipper(*dc, lpRect->left, lpRect->top, lpRect->right - lpRect->left - 2, lpRect->bottom - lpRect->top);
		CString buffer;
		CUpDownClient *lpUpDownClient = (CUpDownClient *) lpCtrlItem->value;
		switch (nColumn) {

			case 0:	// icon, name, status
				{
					RECT cur_rec;
					memcpy(&cur_rec, lpRect, sizeof(RECT));
					// +3 is added by OnDrawItem()... so take it off 
					// Kry - eMule says +1, so I'm trusting it
					POINT point = { cur_rec.left, cur_rec.top+1 };
					if (lpCtrlItem->type == 2) {
						switch (lpUpDownClient->GetDownloadState()) {
							case DS_CONNECTING:
								m_ImageList.Draw(1, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_CONNECTED:
								m_ImageList.Draw(1, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_WAITCALLBACK:
								m_ImageList.Draw(1, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_ONQUEUE:
								if (lpUpDownClient->IsRemoteQueueFull()) {
									m_ImageList.Draw(3, *dc, point.x, point.y, ILD_NORMAL);
								} else {
									m_ImageList.Draw(2, *dc, point.x, point.y, ILD_NORMAL);
								}
								break;
							case DS_DOWNLOADING:
								m_ImageList.Draw(0, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_REQHASHSET:
								m_ImageList.Draw(0, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_NONEEDEDPARTS:
								m_ImageList.Draw(3, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_LOWTOLOWIP:
								m_ImageList.Draw(3, *dc, point.x, point.y, ILD_NORMAL);
								break;
							case DS_TOOMANYCONNS:
								m_ImageList.Draw(1, *dc, point.x, point.y, ILD_NORMAL);
								break;
							default:
								m_ImageList.Draw(4, *dc, point.x, point.y, ILD_NORMAL);
						}
					} else {

						m_ImageList.Draw(3, *dc, point.x, point.y, ILD_NORMAL);
					}
					cur_rec.left += 20;
					if (lpUpDownClient->IsFriend()) {
						POINT point2 = { cur_rec.left, cur_rec.top + 1 };
						m_ImageList.Draw(6, *dc, point2.x, point.y, ILD_NORMAL);
						cur_rec.left += 20;
					} else if (lpUpDownClient->ExtProtocolAvailable() && lpUpDownClient->GetClientSoft() == SO_AMULE) {
						POINT point2 = { cur_rec.left, cur_rec.top + 1 };
						m_ImageList.Draw(13, *dc, point2.x, point.y, ILD_NORMAL);
						cur_rec.left += 20;
					} else if (lpUpDownClient->ExtProtocolAvailable()) {
						POINT point2 = { cur_rec.left, cur_rec.top + 1 };
						m_ImageList.Draw(5, *dc, point2.x, point.y, ILD_NORMAL);
						cur_rec.left += 20;
					} else {
						POINT point2 = { cur_rec.left, cur_rec.top + 1 };
						if (lpUpDownClient->GetClientSoft() == SO_MLDONKEY || lpUpDownClient->GetClientSoft() == SO_NEW_MLDONKEY) {
							m_ImageList.Draw(8, *dc, point2.x, point.y, ILD_NORMAL);
						} else if (lpUpDownClient->GetClientSoft() == SO_AMULE) {
							m_ImageList.Draw(12, *dc, point2.x, point.y, ILD_NORMAL);
						} else if (lpUpDownClient->GetClientSoft() == SO_EDONKEYHYBRID) {
							m_ImageList.Draw(11, *dc, point2.x, point.y, ILD_NORMAL);
						} else {
							m_ImageList.Draw(7, *dc, point2.x, point.y, ILD_NORMAL);
						}
						cur_rec.left += 20;
					}

					if (!lpUpDownClient->GetUserName()) {
						buffer = "?";
					} else {
						buffer.Format("%s", lpUpDownClient->GetUserName());
					}
					//dc->DrawText(buffer, cur_rec.left, cur_rec.top);
					lpRect->left += 40;
					dc->DrawText(buffer, lpRect->left, lpRect->top);
					lpRect->left -= 40;
				}
				break;

			case 1:	// size
				break;

			case 2:
				if (!IsColumnHidden(3)) {
					dc->DrawText("", lpRect->left, lpRect->top);
					break;
				}

			case 3:	// completed
				if (lpCtrlItem->type == 2 && lpUpDownClient->GetTransferedDown()) {
					buffer.Format("%s", CastItoXBytes(lpUpDownClient->GetTransferedDown()).GetData());
					//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT); 
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;

			case 4:	// speed

				if (lpCtrlItem->type == 2) {
					if (lpUpDownClient->GetKBpsDown()<0.001) {
						buffer = "";
					} else {
						buffer.Format("%.1f %s", lpUpDownClient->GetKBpsDown(), "kB/s");
					}
					//dc->DrawText(buffer,(int)strlen(buffer),lpRect, DLC_DT_TEXT);
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;

			#ifndef DISABLE_PROGRESS

			case 5:	// file info
				{
					lpRect->bottom--;
					lpRect->top++;

					int iWidth = lpRect->right - lpRect->left;
					int iHeight = lpRect->bottom - lpRect->top;

					DWORD dwTicks = GetTickCount();
					wxMemoryDC cdcStatus;

					if (lpCtrlItem->dwUpdated + (4 * DLC_BARUPDATE) < dwTicks || !lpCtrlItem->status || iWidth != lpCtrlItem->status->GetWidth() || !lpCtrlItem->dwUpdated) {
						if (lpCtrlItem->status == NULL) {
							lpCtrlItem->status = new wxBitmap(iWidth, iHeight);
						} else {
							//delete lpCtrlItem->status;
							//lpCtrlItem->status=NULL;
							//lpCtrlItem->status=new wxBitmap(iWidth,iHeight);
							lpCtrlItem->status->Create(iWidth, iHeight);
						}

						cdcStatus.SelectObject(*(lpCtrlItem->status));

						lpUpDownClient->DrawStatusBar(&cdcStatus, wxRect(0, 0, iWidth, iHeight), (lpCtrlItem->type == 3), theApp.glob_prefs->UseFlatBar());
						lpCtrlItem->dwUpdated = dwTicks + (rand() % 128);
					} else {
						cdcStatus.SelectObject(*(lpCtrlItem->status));
					}
					dc->Blit(lpRect->left, lpRect->top, iWidth, iHeight, &cdcStatus, 0, 0);
					cdcStatus.SelectObject(wxNullBitmap);

					lpRect->top--;
					lpRect->bottom++;
				}
				break;

			#endif

			case 6:{
				// sources
				switch (lpUpDownClient->GetClientSoft()) {					
				case SO_EDONKEY:
					buffer.Format(_("eDonkey v%i"),lpUpDownClient->GetVersion());
					break;
				case SO_EDONKEYHYBRID:
					buffer.Format(_("eDonkeyHybrid v%i"),lpUpDownClient->GetVersion());
					break;
				case SO_EMULE:
				case SO_OLDEMULE:
					buffer.Format(_("eMule v%02X"),lpUpDownClient->GetMuleVersion());
					break;
				case SO_CDONKEY:
					buffer.Format("cDonkey v%02X",lpUpDownClient->GetMuleVersion());
					break;
				case SO_AMULE:
					if (lpUpDownClient->GetClientModString().IsEmpty() == false) {
						buffer.Format(_("aMule [ %s ]"),lpUpDownClient->GetClientModString().c_str());
					} else {
						buffer.Format(_("aMule v0.%02X"),lpUpDownClient->GetMuleVersion());
					}
					break;
				case SO_LXMULE:
					buffer.Format(_("lMule/xMule v0.%02X"),lpUpDownClient->GetMuleVersion());
					break;
				case SO_MLDONKEY:
					buffer.Format(_("Old MLdonkey"));
					break;
				case SO_NEW_MLDONKEY:
					buffer.Format(_("New MLdonkey"));
					break;
				/*
				case SO_SHAREAZA:
					buffer.Format("Shareaza v%.2f",(float)lpUpDownClient->GetVersion()/1000.0f);
					break;
				*/
				default:
					buffer.Format(_("Unknown"));
				}
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;
			}

			case 7:	// prio
				if (lpUpDownClient->GetDownloadState() == DS_ONQUEUE) {
					if (lpUpDownClient->IsRemoteQueueFull()) {
						buffer.Format("%s", (_("Queue Full")));
						dc->DrawText(buffer, lpRect->left, lpRect->top);
					} else {
						if (lpUpDownClient->GetRemoteQueueRank()) {
							buffer.Format("QR: %u", lpUpDownClient->GetRemoteQueueRank());
							dc->DrawText(buffer, lpRect->left, lpRect->top);
						} else {
							buffer = "";
							dc->DrawText(buffer, lpRect->left, lpRect->top);
						}
					}
				} else {
					buffer = "";
					dc->DrawText(buffer, lpRect->left, lpRect->top);
				}
				break;

			case 8:	// status
				if (lpCtrlItem->type == 2) {
					switch (lpUpDownClient->GetDownloadState()) {
						case DS_CONNECTING:
							buffer = _("Connecting");
							break;
						case DS_CONNECTED:
							buffer = _("Asking");
							break;
						case DS_WAITCALLBACK:
							buffer = _("Connecting via server");
							break;
						case DS_ONQUEUE:
							if (lpUpDownClient->IsRemoteQueueFull()) {
								buffer = _("Queue Full");
							} else {
								buffer = _("On Queue");
							}
							break;
						case DS_DOWNLOADING:
							buffer = _("Transferring");
							break;
						case DS_REQHASHSET:
							buffer = _("Receiving hashset");
							break;
						case DS_NONEEDEDPARTS:
							buffer = _("No needed parts");
							break;
						case DS_LOWTOLOWIP:
							buffer = _("Cannot connect LowID to LowID");
							break;
						case DS_TOOMANYCONNS:
							buffer = _("Too many connections");
							break;
						default:
							buffer = _("Unknown");
					}
				} else {
					buffer = _("Asked for another file");
				}
				dc->DrawText(buffer, lpRect->left, lpRect->top);
				break;

			case 9:	// remaining time & size
				break;
		}
	}
}


void CDownloadListCtrl::OnLvnItemActivate(wxListEvent & evt)
{
	CtrlItem_Struct *content = (CtrlItem_Struct *) this->GetItemData(evt.GetIndex());	//pNMIA->iItem);

	bool added = false;

	if (content->type == FILE_TYPE) {
		CPartFile *partfile = (CPartFile *) content->value;

		if (!partfile->srcarevisible) {
			Freeze();
			for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
				CtrlItem_Struct *cur_item = it->second;
				if (cur_item->owner == partfile) {
					CUpDownClient *client = (CUpDownClient *) cur_item->value;
					wxString textData;
					wxString status;

					// lagloose
					partfile->m_bShowOnlyDownloading = isShift;
					if (isShift) {
						uint8 ds = client->GetDownloadState();
						if ((client->GetDownloadFile() != partfile) || (ds != DS_DOWNLOADING)) {
							continue;
						}
					}
					// end lagloose

					int newid = InsertItem(evt.GetIndex() + 1, textData);
					added = true;
					wxListItem newitem;
					newitem.m_itemId = newid;
					newitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
					SetItem(newitem);
					SetItemData(newid, (long)cur_item);
				}
				partfile->srcarevisible = added;
			}
			theApp.amuledlg->transfers_frozen = false;
			Thaw();
		} else {
			HideSources(partfile, false, false, false);
		}
	}
	// lagloose
	isShift = false;
	// end lagloose
	//*pResult = 0; <- ???
}

bool CDownloadListCtrl::ProcessEvent(wxEvent & evt)
{
	if (evt.GetEventType() != wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}
	
	wxCommandEvent & event = (wxCommandEvent &) evt;
	long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != (-1)) {
		CtrlItem_Struct *content = (CtrlItem_Struct *) this->GetItemData(item);
		UINT selectedCount = this->GetSelectedItemCount();
		CTypedPtrList < CPtrList, CPartFile * >selectedList;
		item = -1;
		for (;;) {
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			if (item == (-1)) {
				break;
			}
			if (((CtrlItem_Struct *) this->GetItemData(item))->type == 1) {
				selectedList.AddTail((CPartFile *) ((CtrlItem_Struct *) this->GetItemData(item))->value);
			}
		}
		if (content->type == 1) {
			CPartFile *file = (CPartFile *) content->value;

			if (event.GetId() >= MP_ASSIGNCAT && event.GetId() <= MP_ASSIGNCAT + 99) {
				while (!selectedList.IsEmpty()) {
					CPartFile *selected = selectedList.GetHead();
					selected->SetCategory(event.GetId() - MP_ASSIGNCAT);
					selectedList.RemoveHead();
				}
				ChangeCategory(curTab);
			}

			switch (event.GetId()) {
					/* Razor 1a - Modif by MikaelB
					   Event for Drop No Needed Sources */
				case MP_DROP_NO_NEEDED_SOURCES:
					// While selected items aren't empty
					while (!selectedList.IsEmpty()) {
						// Remove No Needed sources from the current selected item
						selectedList.GetHead()->RemoveNoNeededSources();
						// Remove this item from the selected items list
						selectedList.RemoveHead();
					}
					return true;					
					break;
					/* End modif */

					/* Razor 1a - Modif by MikaelB
					   Event for Drop Full Queue Sources */
				case MP_DROP_FULL_QUEUE_SOURCES:
					// While selected items aren't empty
					while (!selectedList.IsEmpty()) {
						// Remove Full Queue sources from the current selected item
						selectedList.GetHead()->RemoveFullQueueSources();
						// Remove this item from the selected items list
						selectedList.RemoveHead();
					}
					return true;
					break;
					/* End modif */

					/* Razor 1a - Modif by MikaelB
					   Event for Drop High Queue Rating Sources */
				case MP_DROP_HIGH_QUEUE_RATING_SOURCES:
					// While selected items aren't empty
					while (!selectedList.IsEmpty()) {
						// Remove High Queue Rating sources from the current selected item
						selectedList.GetHead()->RemoveHighQueueRatingSources();
						// Remove this item from the selected items list
						selectedList.RemoveHead();
					}
					return true;					
					break;
					/* End modif */

					/* Razor 1a - Modif by MikaelB
					   Event for Clean Up Sources */
				case MP_CLEAN_UP_SOURCES:
					// While selected items aren't empty
					while (!selectedList.IsEmpty()) {
						// Clean up sources from the current selected item
						selectedList.GetHead()->CleanUpSources();
						// Remove this item from the selected items list
						selectedList.RemoveHead();
					}
					return true;					
					break;
					/* End modif */
				case MP_SWAP_A4AF_TO_THIS: {
					Freeze();
					if (selectedCount == 1 
					&& (file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY))
					{
						theApp.downloadqueue->DisableAllA4AFAuto();

						POSITION pos1, pos2;
						for (pos1 = file->A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;) {
							file->A4AFsrclist.GetNext(pos1);
							CUpDownClient *cur_source = file->A4AFsrclist.GetAt(pos2);
							if (cur_source->GetDownloadState() != DS_DOWNLOADING
							&& cur_source->reqfile
							&& ((!cur_source->reqfile->IsA4AFAuto()) || cur_source->GetDownloadState() == DS_NONEEDEDPARTS)
							&& !cur_source->IsSwapSuspended(file))
							{
								CPartFile* oldfile = cur_source->reqfile;
								if (cur_source->SwapToAnotherFile(true, false, false, file)) {
									cur_source->DontSwapTo(oldfile);
								}
							}
						}

					}
					theApp.amuledlg->transfers_frozen = false;					
					Thaw();
					this->UpdateItem(file);						
					return true;					
					break;
				}
				case MP_SWAP_A4AF_TO_THIS_AUTO:
					file->SetA4AFAuto(!file->IsA4AFAuto());
					return true;				
					break;
				case MP_SWAP_A4AF_TO_ANY_OTHER: {
					Freeze();
					if (selectedCount == 1 
					&& (file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)) {
						theApp.downloadqueue->DisableAllA4AFAuto();
						for (int sl=0;sl<SOURCESSLOTS;sl++) {
							if (!file->srclists[sl].IsEmpty()) {
								POSITION pos1, pos2;
								for(pos1 = file->srclists[sl].GetHeadPosition(); (pos2 = pos1) != NULL;) {
									file->srclists[sl].GetNext(pos1);
									file->srclists[sl].GetAt(pos2)->SwapToAnotherFile(false, false, false, NULL);
								}
							}
						}
					}
					theApp.amuledlg->transfers_frozen = false;
					Thaw();
					return true;					
					break;
				}
				case MP_CANCEL:	{
					if (selectedCount > 0) {
						Freeze();
						wxString fileList;
						bool validdelete = false;

						for (POSITION pos = selectedList.GetHeadPosition(); pos != 0; selectedList.GetNext(pos)) {
							if(selectedList.GetAt(pos)->GetStatus() != PS_COMPLETING && selectedList.GetAt(pos)->GetStatus() != PS_COMPLETE) {
								validdelete = true;
								fileList += "\n";
								fileList += selectedList.GetAt(pos)->GetFileName();
							}
						}
						wxString quest;
						if (selectedCount==1) {
							// for single selection
							quest=CString(_("Are you sure that you want to cancel and delete this file ?\n"));
						} else {
							// for multiple selections
							quest=CString(_("Are you sure that you want to cancel and delete these files ?\n"));
						}
						if (validdelete && wxMessageBox((quest + fileList), "Cancel", wxICON_QUESTION | wxYES_NO) == wxYES) {
							while (!selectedList.IsEmpty()) {
								HideSources(selectedList.GetHead());
								switch (selectedList.GetHead()->GetStatus()) {
									case PS_WAITINGFORHASH:
									case PS_HASHING:
									case PS_COMPLETING:
									case PS_COMPLETE:
										selectedList.RemoveHead();
										break;
									case PS_PAUSED:
										selectedList.GetHead()->Delete();
										selectedList.RemoveHead();
										break;
									default:
										if (theApp.glob_prefs->StartNextFile()) {
											theApp.downloadqueue->StartNextFile();
										}
										selectedList.GetHead()->Delete();
										selectedList.RemoveHead();
								}
							}
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
					}
					return true;					
					break;
				}
				case MP_PRIOHIGH:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(false);	//[Tarod]
							selectedList.GetHead()->SetDownPriority(PR_HIGH);
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_HIGH);
					return true;					
					break;
				case MP_PRIOLOW:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(false);	//[Tarod]
							selectedList.GetHead()->SetDownPriority(PR_LOW);
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_LOW);
					return true;					
					break;
				case MP_PRIONORMAL:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(false);	//[Tarod]
							selectedList.GetHead()->SetDownPriority(PR_NORMAL);
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_NORMAL);
					return true;					
					break;
				case MP_PRIOAUTO:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(true);
							selectedList.GetHead()->SetDownPriority(PR_HIGH);
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->SetAutoDownPriority(true);
					file->SetDownPriority(PR_HIGH);
					return true;					
					break;
				case MP_PAUSE:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->PauseFile();
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->PauseFile();
					return true;					
					break;
				case MP_RESUME:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							selectedList.GetHead()->ResumeFile();
							selectedList.GetHead()->SavePartFile();
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					file->ResumeFile();
					file->SavePartFile();
					return true;					
					break;
				case MP_STOP:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CPartFile *selected = selectedList.GetHead();
							HideSources(selected);
							selected->StopFile();
							selectedList.RemoveHead();
						}
						theApp.amuledlg->transfers_frozen = false;
						Thaw();
						return true;						
						break;
					}
					HideSources(file);
					file->StopFile();
					return true;					
					break;
					case MP_FAKECHECK1:	// deltahf -> fakecheck
					    theApp.LaunchUrl(theApp.GenFakeCheckUrl(file));
					break;
				case MP_CLEARCOMPLETED:
					Freeze();
					ClearCompleted();
					theApp.amuledlg->transfers_frozen = false;
					Thaw();
					return true;				
					break;
				case MP_METINFO: {
					CFileDetailDialog *dialog = new CFileDetailDialog(this, file);
					dialog->ShowModal();
					delete dialog;
					return true;
					break;
				}
				case MP_GETED2KLINK:
					if (selectedCount > 1) {
						wxString str;
						while (!selectedList.IsEmpty()) {
							str += theApp.CreateED2kLink(selectedList.GetHead()) + "\n";
							selectedList.RemoveHead();
						}
						theApp.CopyTextToClipboard(str);
						return true;
						break;
					}
					theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
					return true;					
					break;
				case MP_GETHTMLED2KLINK:
					if (selectedCount > 1) {
						CString str;
						while (!selectedList.IsEmpty()) {
							str += theApp.CreateHTMLED2kLink(selectedList.GetHead()) + "\n";
							selectedList.RemoveHead();
						}
						theApp.CopyTextToClipboard(str);
						return true;						
						break;
					}
					theApp.CopyTextToClipboard(theApp.CreateHTMLED2kLink(file));
					return true;					
					break;
				case MP_OPEN:{
						if (selectedCount > 1) {
							break;
						}
						char *buffer = new char[250];
						sprintf(buffer, "%s/%s", theApp.glob_prefs->GetIncomingDir(), file->GetFileName().GetData());
						//ShellOpenFile(buffer);
						printf("===> open %s\n", buffer);
						delete[] buffer;
						return true;						
						break;
					}
				case MP_PREVIEW:{
						if (selectedCount > 1) {
							return true;							
							break;
						}
						file->PreviewFile();
						return true;						
						break;
					}
				case MP_VIEWFILECOMMENTS:{
						CCommentDialogLst dialog(this, file);
						dialog.ShowModal();
						return true;					
						break;
					}
				case MP_TOOGLELIST: {
					if (theApp.amuledlg->list_no_refresh) {
						theApp.amuledlg->list_no_refresh = false;
  						theApp.amuledlg->Thaw_AllTransfering();												
					} else {
						theApp.amuledlg->list_no_refresh = true;
						theApp.amuledlg->Freeze_AllTransfering();													
					}
					return true;
				}
			}
		} else {
			CUpDownClient *client = (CUpDownClient *) content->value;
			switch (event.GetId()) {
				case MP_SHOWLIST:
					client->RequestSharedFileList();
					return true;				
					break;
				case MP_ADDFRIEND:
					theApp.friendlist->AddFriend(client);
					return true;				
					break;
				case MP_DETAIL:
					CClientDetailDialog * dialog = new CClientDetailDialog(this, client);
					dialog->ShowModal();
					delete dialog;
					//dialog.DoModal();
					return true;				
					break;
			}
		}
		// cleanup multiselection
		selectedList.RemoveAll();
	} else {
		// nothing selected
		switch (event.GetId()) {
			case MP_CLEARCOMPLETED:
				ClearCompleted();
				return true;			
				break;
		}
	}

	// should we call this? (no!)
	evt.Skip();
	// Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);
}


void CDownloadListCtrl::OnColumnClick(wxListEvent & evt)
{	//NMHDR* pNMHDR, LRESULT* pResult){
	//NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableDownload);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableDownload);
	bool sortAscending = (sortItem != evt.GetColumn())? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = evt.GetColumn();
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableDownload, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableDownload, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0 : 100));
}

int CDownloadListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CtrlItem_Struct *item1 = (CtrlItem_Struct *) lParam1;
	CtrlItem_Struct *item2 = (CtrlItem_Struct *) lParam2;

	int sortMod = 1;
	if (lParamSort >= 100) {
		sortMod = -1;
		lParamSort -= 100;
	}

	int comp;

	if (item1->type == 1 && item2->type != 1) {
		if (item1->value == item2->parent->value) {
			return -1;
		}

		comp = Compare((CPartFile *) item1->value, (CPartFile *) (item2->parent->value), lParamSort);

	} else if (item2->type == 1 && item1->type != 1) {
		if (item1->parent->value == item2->value) {
			return 1;
		}

		comp = Compare((CPartFile *) (item1->parent->value), (CPartFile *) item2->value, lParamSort);

	} else if (item1->type == 1) {
		CPartFile *file1 = (CPartFile *) item1->value;
		CPartFile *file2 = (CPartFile *) item2->value;

		comp = Compare(file1, file2, lParamSort);

	} else {
		CUpDownClient *client1 = (CUpDownClient *) item1->value;
		CUpDownClient *client2 = (CUpDownClient *) item2->value;
		comp = Compare((CPartFile *) (item1->parent->value), (CPartFile *) (item2->parent->value), lParamSort);
		if (comp != 0) {
			return sortMod * comp;
		}
		if (item1->type != item2->type) {
			return item1->type - item2->type;
		}

		comp = Compare(client1, client2, lParamSort, sortMod);
	}

	return sortMod * comp;
}

int CDownloadListCtrl::Compare(CPartFile * file1, CPartFile * file2, long lParamSort)
{
	switch (lParamSort) {
		case 0:	//filename asc
			return strcmpi(file1->GetFileName(), file2->GetFileName());
		case 1:	//size asc
			return file1->GetFileSize() - file2->GetFileSize();
		case 2:	//transfered asc
			return file1->GetTransfered() - file2->GetTransfered();
		case 3:	//completed asc
			return file1->GetCompletedSize() - file2->GetCompletedSize();
		case 4:	//speed asc
#ifdef DOWNLOADRATE_FILTERED
			return (int)(file1->GetKBpsDown()-file2->GetKBpsDown())*1024;
#else
			return file1->GetDatarate() - file2->GetDatarate();
#endif
		case 5:	//progress asc
			{
				float comp = file1->GetPercentCompleted() - file2->GetPercentCompleted();
				if (comp > 0) {
					return 1;
				} else {
					if (comp < 0) {
						return -1;
					} else {
						return 0;
					}
				}
			}
		case 6:	//sources asc
			return file1->GetSourceCount() - file2->GetSourceCount();
		case 7:	//priority asc
			//return file1->GetPriority() - file2->GetPriority();
			return file1->GetDownPriority() - file2->GetDownPriority();
		case 8:	//Status asc 
			return file1->getPartfileStatusRang() - file2->getPartfileStatusRang();
		case 9:	//Remaining Time asc 
			return file1->getTimeRemaining() - file2->getTimeRemaining();
		case 10: // Last seen complete - changed by deltaHF *start* (Georg Ludwig fix)
                                                 {
            time_t t1 = file1->lastseencomplete;
            time_t t2 = file2->lastseencomplete;

                return (t1 > t2) ? 1 : (t1 < t2) ? -1 : 0;
            }
        case 11: // Last reception
                                                  {
            time_t t1 = file1->GetLastChangeDatetime();
            time_t t2 = file2->GetLastChangeDatetime();

                return (t1 > t2) ? 1 : (t1 < t2) ? -1 : 0; // *end*
            }
		default:
			return 0;
	}
}

int CDownloadListCtrl::Compare(CUpDownClient * client1, CUpDownClient * client2, long lParamSort, int sortMod)
{
	switch (lParamSort) {
		case 0:	//name asc
			if (client1->GetUserName() == client2->GetUserName()) {
				return 0;
			} else if (!client1->GetUserName()) {
				return 1;
			} else if (!client2->GetUserName()) {
				return -1;
			}
			return strcmpi(client1->GetUserName(), client2->GetUserName());
		case 1:	//size but we use status asc
			return client1->GetDownloadState() - client2->GetDownloadState();
		case 2:	//transfered asc
		case 3:	//completed asc
			return client1->GetTransferedDown() - client2->GetTransferedDown();
		case 4:	//speed asc
			return (int)((client1->GetKBpsDown() - client2->GetKBpsDown())*1024.0);
		case 5:	//progress asc
			return client1->GetAvailablePartCount() - client2->GetAvailablePartCount();
		case 6:
			if (client1->GetClientSoft() == client2->GetClientSoft()) {
				if (client1->IsEmuleClient()) {
					return client2->GetMuleVersion() - client1->GetMuleVersion();
				} else {
					return client2->GetVersion() - client1->GetVersion();
				}
			}
			return client1->GetClientSoft() - client2->GetClientSoft();
		case 7:	//qr asc
			if (client1->GetRemoteQueueRank() == 0 && client1->GetDownloadState() == DS_ONQUEUE && client1->IsRemoteQueueFull() == true) {
				return 1;
			}
			if (client2->GetRemoteQueueRank() == 0 && client2->GetDownloadState() == DS_ONQUEUE && client2->IsRemoteQueueFull() == true) {
				return -1;
			}
			if (client1->GetRemoteQueueRank() == 0) {
				return 1;
			}
			if (client2->GetRemoteQueueRank() == 0) {
				return -1;
			}
			return client1->GetRemoteQueueRank() - client2->GetRemoteQueueRank();
		case 8:
			if (client1->GetDownloadState() == client2->GetDownloadState()) {
				if (client1->IsRemoteQueueFull()) {
					return 1;
				}
				if (client2->IsRemoteQueueFull()) {
					return -1;
				} else {
					return 0;
				}
			}
			return client1->GetDownloadState() - client2->GetDownloadState();
		default:
			return 0;
	}
}


void CDownloadListCtrl::CreateMenues()
{
}

wxString CDownloadListCtrl::getTextList()
{
	wxString out = "";
	int i=0;
	// Search for file(s)
	for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {	// const is better
		CtrlItem_Struct *cur_item = it->second;
		if (cur_item->type == FILE_TYPE) {
			CPartFile *file = reinterpret_cast < CPartFile * >(cur_item->value);

/*			if (file->IsPartFile() == false && CheckShowItemInGivenCat(file, curTab)) {
				RemoveFile(file);
			}
*/			//theApp.amuledlg->AddLogLine(false, CString(wxT("%s")), file->GetFileName());

			char buffer[255 + 1];
			strncpy(buffer, file->GetFileName(), 255);
			buffer[255] = '\0';

			wxString temp;
			i++;
			temp.Printf("%i: %s\t [%.1f%%] %i/%i - %s", i,buffer, file->GetPercentCompleted(), file->GetTransferingSrcCount(), file->GetSourceCount(), file->getPartfileStatus().GetData());
#ifdef DOWNLOADRATE_FILTERED	
			if (file->GetKBpsDown()>0.001) {
				temp += wxString::Format(" %.1f kB/s",(float)file->GetKBpsDown());
#else
			if (file->GetDatarate()>0) {
				temp += wxString::Format(" %.1f kB/s",(float)file->GetDatarate()/1024);
#endif
			}
			out += temp;
			out += "\n";
		}
	}

	theApp.amuledlg->AddLogLine(false, CString(wxT("%s")), out.c_str());
	return out;
}

void CDownloadListCtrl::ClearCompleted()
{
	// Search for completed file(s)
	for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end();) {
		CtrlItem_Struct *cur_item = it->second;
		it++;	// Already point to the next iterator. 
		if (cur_item->type == FILE_TYPE) {
			CPartFile *file = reinterpret_cast < CPartFile * >(cur_item->value);
			if (file->IsPartFile() == false) {
				RemoveFile(file);
			}
		}
	}
}

void CDownloadListCtrl::ShowFilesCount()
{
	CString counter;
	uint16 count = 0;	//theApp.downloadqueue->GetFileCount();

	// remove all displayed files with a different cat
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
		const CtrlItem_Struct *cur_item = it->second;
		if (cur_item->type == FILE_TYPE) {
			CPartFile *file = (CPartFile *) cur_item->value;
			if (file->GetCategory() == curTab || (!theApp.glob_prefs->ShowAllNotCats() && file->GetCategory() > 0 && curTab == 0)) {
				count++;
			}
		}
	}

	wxString fmtstr = wxString::Format(_("Downloads (%i)"), GetItemCount());
	wxStaticCast(FindWindowByName(wxT("downloadsLabel")), wxStaticText)->SetLabel(fmtstr);
}

void CDownloadListCtrl::ShowSelectedFileDetails()
{
	if (GetSelectedItemCount() == 0) {
		return;
	}
	int cursel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	CtrlItem_Struct *content = (CtrlItem_Struct *) this->GetItemData(cursel);

	if (content->type == FILE_TYPE) {
		CPartFile *file = (CPartFile *) content->value;
		//CFileDetailDialog dialog(file);
		//dialog.DoModal();

		if ((file->HasComment() || file->HasRating()) /*&& p.x<13 */ ) {
			//CCommentDialogLst dialog(file);
			//dialog.DoModal();
		} else {
			//CFileDetailDialog dialog(file);
			//dialog.DoModal();
		}

	} else {
		//CClientDetailDialog dialog(client);
		//dialog.DoModal();
		printf("Show details me too\n");
	}
}

void CDownloadListCtrl::ChangeCategory(int newsel)
{

	Freeze();

	// remove all displayed files with a different cat and show the correct ones
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
		const CtrlItem_Struct *cur_item = it->second;
		if (cur_item->type == FILE_TYPE) {
			CPartFile *file = reinterpret_cast < CPartFile * >(cur_item->value);
			if (!CheckShowItemInGivenCat(file, newsel)) {
				HideFile(file);
			} else {
				ShowFile(file);
			}
		}
	}
	theApp.amuledlg->transfers_frozen = false;
	Thaw();
	curTab = newsel;
	ShowFilesCount();
}

void CDownloadListCtrl::HideFile(CPartFile * tohide)
{
	HideSources(tohide);

	// Retrieve all entries matching the source
	std::pair < ListItems::const_iterator, ListItems::const_iterator > rangeIt = m_ListItems.equal_range(tohide);
	for (ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++) {
		CtrlItem_Struct *updateItem = it->second;

		// Find entry in CListCtrl and update object
		//LVFINDINFO find;
		//find.flags = LVFI_PARAM;
		//find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(-1, (long)updateItem);
		if (result != (-1)) {
			DeleteItem(result);
			return;
		}
	}
}

void CDownloadListCtrl::ShowFile(CPartFile * toshow)
{
	// Retrieve all entries matching the source
	std::pair < ListItems::const_iterator, ListItems::const_iterator > rangeIt = m_ListItems.equal_range(toshow);
	for (ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++) {
		CtrlItem_Struct *updateItem = it->second;

		// Check if entry is already in the List
		//LVFINDINFO find;
		//find.flags = LVFI_PARAM;
		//find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(-1, (long)updateItem);
		if (result == (-1)) {
			//InsertItem(LVIF_PARAM,GetItemCount(),0,0,0,0,(LPARAM)updateItem);
			int newitem = InsertItem(GetItemCount(), "This is not visible");
			SetItemData(newitem, (long)updateItem);

			wxListItem myitem;
			myitem.m_itemId = newitem;
			myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
			SetItem(myitem);
		}
		return;
	}
}

bool CDownloadListCtrl::ShowItemInCurrentCat(CPartFile * file, int newsel)
{
	return (((newsel == 0 && !theApp.glob_prefs->ShowAllNotCats()) || (newsel == 0 && theApp.glob_prefs->ShowAllNotCats() && file->GetCategory() == 0)) || (newsel > 0 && newsel == file->GetCategory()));
}

//lagloose
void CDownloadListCtrl::OnKeyUp(wxKeyEvent & event)
{
	isShift = false;
	event.Skip();
}


void CDownloadListCtrl::OnKeyDown(wxKeyEvent & event)
{
	if (event.GetKeyCode() == WXK_SHIFT) {
		isShift = true;
	} else {
		isShift = false;
	}

	event.Skip();
}
// end lagloose
bool CDownloadListCtrl::this_is_the_moment() {

	uint32 i = GetTickCount();
	if((i - last_moment) > 10 ) { // 1 sec
		last_moment = i;
		return true;	
	} else {
		return false;	
	}
}
