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

// DownloadListCtrl.cpp : implementation file



#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for DISABLE_PROGRESS
#endif

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif

#include <cmath>			// Needed for fmod
#include <algorithm>		// Needed for std::min
#include <wx/event.h>
#include <wx/font.h>
#include <wx/dcmemory.h>
#include <wx/datetime.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include "DownloadListCtrl.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CheckShowItemInGivenCat
#include "amule.h"		// Needed for theApp
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "ChatWnd.h"		// Needed for CChatWnd
#include "PartFile.h"		// Needed for CPartFile
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "FileDetailDialog.h"	// Needed for CFileDetailDialog
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "updownclient.h"	// Needed for CUpDownClient
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "muuli_wdr.h"		// Needed for ID_DLOADLIST
#include "color.h"		// Needed for G_BLEND and SYSCOLOR
#include "ClientCredits.h"		// Needed for GetCurrentIdentState
#include "Preferences.h"
#include "listbase.h"		// Needed for wxLC_OWNERDRAW

#define DLC_BARUPDATE 512

class CPartFile;


int CDownloadListCtrl::s_lastOrder;
int CDownloadListCtrl::s_lastColumn;


#define m_ImageList theApp.amuledlg->imagelist

// CDownloadListCtrl

BEGIN_EVENT_TABLE(CDownloadListCtrl, CMuleListCtrl)
	EVT_LIST_COL_CLICK( -1, 		CDownloadListCtrl::OnColumnLClick)
	EVT_LIST_ITEM_ACTIVATED(ID_DLOADLIST, CDownloadListCtrl::OnLvnItemActivate)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnNMRclick)
	EVT_KEY_UP(CDownloadListCtrl::OnKeyUp)
	EVT_KEY_DOWN(CDownloadListCtrl::OnKeyDown)
END_EVENT_TABLE()

CDownloadListCtrl::CDownloadListCtrl(wxWindow * &parent, int id, const wxPoint & pos, wxSize siz, int flags):CMuleListCtrl(parent, id, pos, siz, flags | wxLC_OWNERDRAW)
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Download") );

	m_ClientMenu = NULL;
	m_PrioMenu = NULL;
	m_FileMenu = NULL;
	wxColour col = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	wxColour newcol = wxColour(G_BLEND(col.Red(), 125), G_BLEND(col.Green(), 125), G_BLEND(col.Blue(), 125));
	m_hilightBrush = new wxBrush(newcol, wxSOLID);
	col = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
	newcol = wxColour(G_BLEND(col.Red(), 125), G_BLEND(col.Green(), 125), G_BLEND(col.Blue(), 125));
	m_hilightUnfocusBrush = new wxBrush(newcol, wxSOLID);
	isShift = false;

	s_lastOrder  = ( GetSortAsc() ? 1 : -1 );
	s_lastColumn = GetSortColumn();
	
	Init();
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


void CDownloadListCtrl::OnColumnLClick(wxListEvent& evt)
{
	// Only change the last column if the sorted column has changed
	if ( GetSortColumn() != evt.GetColumn() ) {
		s_lastColumn = GetSortColumn();
		s_lastOrder  = ( GetSortAsc() ? 1 : -1 );
	} else {
		// Reverse the last-column order to preserve the sorting
		s_lastOrder *= -1;
	}
		
	// Let CMuleListCtrl handle the sorting
	evt.Skip();
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
			if ((isShift || isCtrl || isAlt) && (cl->GetRequestFile() == toCollapse)) {
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
			selectedList->AddTail((CPartFile *) ((CtrlItem_Struct *) this->GetItemData(item))->value);
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
					CoreNotify_PartFile_PrioSet(selectedList.GetHead(), newpri, true);
					selectedList.RemoveHead();
				}
				return;
			}
			CoreNotify_PartFile_PrioSet(file, newpri, true);
		}
	}
}

void CDownloadListCtrl::OnPriLow(wxCommandEvent& WXUNUSED(evt))
{
	setPri(PR_LOW);
}

void CDownloadListCtrl::OnPriNormal(wxCommandEvent& WXUNUSED(evt))
{
	setPri(PR_NORMAL);
}

void CDownloadListCtrl::OnPriHigh(wxCommandEvent& WXUNUSED(evt))
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
				priomenu->AppendCheckItem(MP_PRIOLOW, _("Low"));
				priomenu->AppendCheckItem(MP_PRIONORMAL, _("Normal"));
				priomenu->AppendCheckItem(MP_PRIOHIGH, _("High"));
				priomenu->AppendCheckItem(MP_PRIOAUTO, _("Auto"));
				m_PrioMenu = priomenu;

				wxMenu *menu = new wxMenu(_("Downloads"));
				menu->Append(999989, _("Priority"), priomenu);
				menu->Append(MP_CANCEL, _("Cancel"));
				menu->Append(MP_STOP, _("&Stop"));
				menu->Append(MP_PAUSE, _("&Pause"));
				menu->Append(MP_RESUME, _("&Resume"));
				menu->Append(MP_CLEARCOMPLETED, _("C&lear completed"));
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
				extendedmenu->Append(MP_SWAP_A4AF_TO_THIS, _("Swap every A4AF to this file now"));
				extendedmenu->AppendCheckItem(MP_SWAP_A4AF_TO_THIS_AUTO, _("Swap every A4AF to this file (Auto)"));
				extendedmenu->AppendSeparator();
				extendedmenu->Append(MP_SWAP_A4AF_TO_ANY_OTHER, _("Swap every A4AF to any other file now"));
				extendedmenu->AppendSeparator();
				extendedmenu->Append(MP_DROP_NO_NEEDED_SOURCES, _("Drop No Needed Sources now"));
				extendedmenu->Append(MP_DROP_FULL_QUEUE_SOURCES, _("Drop Full Queue Sources now"));
				extendedmenu->Append(MP_DROP_HIGH_QUEUE_RATING_SOURCES, _("Drop High Queue Rating Sources now"));
				extendedmenu->Append(MP_CLEAN_UP_SOURCES, _("Clean Up Sources now (NNS, FQS && HQRS)"));

				menu->Append(999989, _("Extended Options"), extendedmenu);
				/* End Modif */
				menu->AppendSeparator();
				wxMenu *fakecheckmenu = new wxMenu();
				menu->Append(999989, _("FakeCheck"), fakecheckmenu);
				fakecheckmenu->Append(MP_FAKECHECK2, _("jugle.net Fake Check")); // deltahf -> fakecheck
				fakecheckmenu->Append(MP_FAKECHECK1, _("'Donkey Fakes' Fake Check"));
				menu->AppendSeparator();
				
				menu->Append(MP_OPEN, _("&Open the file"));
				menu->Append(MP_PREVIEW, _("Preview"));

				menu->Append(MP_METINFO, _("Show file &details"));
				menu->Append(MP_VIEWFILECOMMENTS, _("Show all comments"));
				menu->AppendSeparator();
				
				menu->Append(MP_GETED2KLINK, _("Copy ED2k &link to clipboard"));
				menu->Append(MP_GETHTMLED2KLINK, _("Copy ED2k link to clipboard (&HTML)"));
				menu->Append(MP_WS, _("Copy feedback to clipboard"));
				m_FileMenu = menu;

			} else {
				// Remove dynamic entries
				m_FileMenu->Remove(432843);	// Assign category
			}
			
			// Add dinamic entries 
			wxMenu *cats = new wxMenu(_("Category"));
			if (theApp.glob_prefs->GetCatCount() > 1) {
				for (uint32 i = 0; i < theApp.glob_prefs->GetCatCount(); i++) {
					cats->Append(MP_ASSIGNCAT + i, (i == 0) ? wxString(_("unassign")) : theApp.glob_prefs->GetCategory(i)->title);
				}
			}

			m_FileMenu->Append(432843, _("Assign to category"), cats);
			if (theApp.glob_prefs->GetCatCount() == 1) {
				m_FileMenu->Enable(432843, MF_GRAYED);
			} else {
				m_FileMenu->Enable(432843, MF_ENABLED);
			}
			
			// then set state
			wxMenu *menu = m_FileMenu;
			menu->Enable(MP_PAUSE, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_STOP, ((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_RESUME, ((file->GetStatus() == PS_PAUSED) ? MF_ENABLED : MF_GRAYED));
			menu->Enable(MP_OPEN, ((file->GetStatus() == PS_COMPLETE) ? MF_ENABLED : MF_GRAYED));	//<<--9/21/02
			
			wxString preview(_("Preview"));
			if (file->IsPartFile() && !(file->GetStatus() == PS_COMPLETE)) {
				preview += wxT(" [");					
		  		preview += file->GetPartMetFileName().BeforeLast(wxT('.'));
				preview += wxT("]");
			}
			menu->SetLabel(MP_PREVIEW, preview);			
			
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
				wxMenu *menu = new wxMenu(wxT("Clients"));
				menu->Append(MP_DETAIL, _("Show &Details"));
				menu->Append(MP_ADDFRIEND, _("Add to Friends"));
				menu->Append(MP_SHOWLIST, _("View Files"));
				menu->Append(MP_SENDMESSAGE, _("Send message"));
				menu->Append(MP_CHANGE2FILE, _("Swap to this file")); 
				m_ClientMenu = menu;
			}
			
			// Only enable the Swap option for A4AF sources
			m_ClientMenu->Enable(MP_CHANGE2FILE, ( content->type == 3 ) ? MF_ENABLED : MF_GRAYED );
			
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


void CDownloadListCtrl::OnDrawItem(int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted)
{
	/* Don't do any drawing if there's nobody to see it. */
    if ( !theApp.amuledlg->IsDialogVisible( CamuleDlg::TransferWnd ) ) {
		return;
	}
	
	CtrlItem_Struct *content = (CtrlItem_Struct *) GetItemData(item);
	
	// Define text-color and background
	if ((content->type == FILE_TYPE) && (highlighted)) {
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
	

	// Define the border of the drawn area
	if ( highlighted ) {
		wxColour old;
		if ( ( content->type == FILE_TYPE ) && !GetFocus() ) {
			old = m_hilightUnfocusBrush->GetColour();
		} else {
			old = m_hilightBrush->GetColour();
		}
	
		wxColor newcol( ((int)old.Red() * 65) / 100, ((int)old.Green() * 65) / 100, ((int)old.Blue() * 65) / 100);
		
		dc->SetPen( wxPen(newcol, 1, wxSOLID) );
	} else {
		dc->SetPen(*wxTRANSPARENT_PEN);
	}


	dc->SetBrush(  dc->GetBackground() );
	// Mapping the rectHL onto the bitmap
	dc->DrawRectangle( rectHL.x, rectHL.y, rectHL.width, rectHL.height );	
	
	dc->SetPen(*wxTRANSPARENT_PEN);

	if ( content->type == FILE_TYPE && ( !highlighted || !GetFocus() ) ) {
		// If we have category, override textforeground with what category tells us.
		CPartFile *file = (CPartFile *) content->value;
		if ( file->GetCategory() ) {
			dc->SetTextForeground( WxColourFromCr(theApp.glob_prefs->GetCatColor(file->GetCategory())) );
		}		
	}

	// Various constant values we use
	const int iTextOffset = ( rect.GetHeight() - dc->GetCharHeight() ) / 2;
	const int iOffset = 4;

	// The starting end ending position of the tree
	bool tree_show = false;
	int tree_start = 0;
	int tree_end = 0;
	
	wxRect cur_rec( iOffset, 0, 0, rect.height );
	for (int iCurrent = 0; iCurrent < GetColumnCount(); iCurrent++) {
		wxListItem listitem;
		GetColumn(iCurrent, listitem);
	
		cur_rec.width = listitem.GetWidth() - 2*iOffset;
		
		// Make a copy of the current rectangle so we can apply specific tweaks
		wxRect target_rec = cur_rec;
		if (iCurrent == 5) {
			tree_show = ( listitem.GetWidth() > 0 );
			
			tree_start = cur_rec.x - iOffset;
			tree_end   = cur_rec.x + iOffset;
			
			// Double the offset to make room for the cirle-marker
			target_rec.x += iOffset;
			target_rec.width -= iOffset;
		} else {
			// will ensure that text is about in the middle ;)
			target_rec.y += iTextOffset;
		}
		
		// Draw the item
		if ( content->type == FILE_TYPE ) {
			DrawFileItem(dc, iCurrent, target_rec, content);
		} else {
			DrawSourceItem(dc, iCurrent, target_rec, content);
		}
		
		// Increment to the next column
		cur_rec.x += listitem.GetWidth();
	}
	 
	// Draw tree last so it draws over selected and focus (looks better)
	if ( tree_show ) {
		// Gather some information
		const bool notLast = item + 1 != GetItemCount();
		const bool notFirst = item != 0;
		const bool hasNext = notLast && ((CtrlItem_Struct *)GetItemData(item + 1))->type != 1;
		const bool isOpenRoot = hasNext && content->type == 1;
		const bool isChild = content->type != FILE_TYPE;
		
		// Might as well calculate these now
		const int treeCenter = tree_start + 3;
		const int middle = ( cur_rec.height + 1 ) / 2;

		// Set up a new pen for drawing the tree
		dc->SetPen( *(wxThePenList->FindOrCreatePen(dc->GetTextForeground(), 1, wxSOLID)) );

		if (isChild) {
			// Draw the line to the status bar
			dc->DrawLine(tree_end, middle, tree_start + 3, middle);

			// Draw the line to the child node
			if (hasNext) {
				dc->DrawLine(treeCenter, middle, treeCenter, cur_rec.height + 1);
			}
			
			// Draw the line back up to parent node
			if (notFirst) {
				dc->DrawLine(treeCenter, middle, treeCenter, -1);
			}
		} else if ( isOpenRoot ) {
			// Draw empty circle
			dc->SetBrush(*wxTRANSPARENT_BRUSH);
			
			dc->DrawCircle( treeCenter, middle, 3 );			
			
			// Draw the line to the child node
			if (hasNext) {
				dc->DrawLine(treeCenter, middle + 3, treeCenter, cur_rec.height + 1);
			}
		}

	}
}

void CDownloadListCtrl::Init()
{
#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

	InsertColumn(0, _("File Name"), LVCFMT_LEFT, 260);
	InsertColumn(1, _("Size"), LVCFMT_LEFT, 60);
	InsertColumn(2, _("Transferred"), LVCFMT_LEFT, 65);
	InsertColumn(3, _("Completed"), LVCFMT_LEFT, 65);
	InsertColumn(4, _("Speed"), LVCFMT_LEFT, 65);
	InsertColumn(5, _("Progress"), LVCFMT_LEFT, 170);
	InsertColumn(6, _("Sources"), LVCFMT_LEFT, 50);
	InsertColumn(7, _("Priority"), LVCFMT_LEFT, 55);
	InsertColumn(8, _("Status"), LVCFMT_LEFT, 70);
	InsertColumn(9, _("Time Remaining"), LVCFMT_LEFT, 110);
	InsertColumn(10, _("Last Seen Complete"), LVCFMT_LEFT, 220);
	InsertColumn(11, _("Last Reception"), LVCFMT_LEFT, 220);

	curTab = 0;
	last_moment = 0;

	LoadSettings();
}

void CDownloadListCtrl::AddFile(CPartFile * toadd)
{
	CtrlItem_Struct *newitem = new CtrlItem_Struct;
	newitem->owner = NULL;
	newitem->type = FILE_TYPE;
	newitem->value = toadd;
	newitem->status = NULL;
	newitem->parent = NULL;
	newitem->dwUpdated = 0;
	//listcontent.Append(newitem);  
	m_ListItems.insert(ListItemsPair(toadd, newitem));

	if (toadd->CheckShowItemInGivenCat(curTab)) {
		// rip something off from DrawFileItem()
		CPartFile *pf = (CPartFile *) newitem->value;
		uint32 newid = InsertItem( GetInsertPos( (long)newitem ), pf->GetFileName() );
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
	wxASSERT(ownerIt != m_ListItems.end());
	CtrlItem_Struct *ownerItem = ownerIt->second;
	wxASSERT(ownerItem->value == owner);
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
	((source->GetDownloadState() != DS_DOWNLOADING) || (source->GetRequestFile()!=owner))) {
		return;
	}
	// insert newitem to the display too!
	// find it
	int itemnr = FindItem(-1, (long)ownerItem);
	while (GetItemCount() > itemnr + 1 && ((CtrlItem_Struct *) GetItemData(itemnr + 1))->type != FILE_TYPE) {
		itemnr++;
	}
	int newid = InsertItem(itemnr + 1, wxT("This text is not visible"));
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
void CDownloadListCtrl::RemoveFile(const CPartFile * toremove)
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

void CDownloadListCtrl::UpdateItem(void* toupdate)
{
	if ( theApp.amuledlg->IsDialogVisible( CamuleDlg::TransferWnd ) && !theApp.amuledlg->IsIconized() ) {
		// Retrieve all entries matching the source
		std::pair < ListItems::const_iterator, ListItems::const_iterator > rangeIt = m_ListItems.equal_range(toupdate);
		
		// Visible lines, default to all because not all platforms support the GetVisibleLines function
		long first = 0, last = GetItemCount();
		
#ifndef __WXMSW__
		// Get visible lines if we need them
		if ( rangeIt.first != rangeIt.second ) {
			GetVisibleLines( &first, &last );
		}
#endif
		
		for (ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++) {
			CtrlItem_Struct* updateItem = it->second;

			// Avoid searching for hidden objects
			if ( ( updateItem->type == FILE_TYPE ) || ( updateItem->owner->srcarevisible ) ) {
				sint16 result = FindItem(-1, (long)updateItem);
				if (result != -1 ) {
					updateItem->dwUpdated = 0;
					
					if ( result >= first && result <= last) {
						RefreshItem(result);
					}
				}
			}
		}
	}
}

void CDownloadListCtrl::DrawFileItem(wxDC* dc, int nColumn, const wxRect& rect, CtrlItem_Struct* lpCtrlItem)
{
	if ( rect.GetWidth() > 0 ) {
		// force clipper (clip 2 px more than the rectangle from the right side)
		wxDCClipper clipper(*dc, rect.GetX(), rect.GetY(), rect.GetWidth() - 2, rect.GetHeight() );

		CPartFile *lpPartFile = (CPartFile *) lpCtrlItem->value;
		
		switch (nColumn) {
			case 0:
				{
					// file name
					if (lpPartFile->HasComment() || lpPartFile->HasRating()) {
						int image = 6;
						if (lpPartFile->HasRating()) {
							if (lpPartFile->HasBadRating()) {
								image = 5;
							}
						}

						// it's already centered by OnDrawItem() ... 
						m_ImageList.Draw(image, *dc, rect.GetX(), rect.GetY() - 1, wxIMAGELIST_DRAW_TRANSPARENT);
						dc->DrawText(lpPartFile->GetFileName(), rect.GetX() + 15, rect.GetY());
					} else {
						dc->DrawText(lpPartFile->GetFileName(), rect.GetX(), rect.GetY());
					}
				}
				break;

			case 1:	// size
				{
					wxString buffer = CastItoXBytes(lpPartFile->GetFileSize());
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			case 2:	// transfered
				{
					wxString buffer = CastItoXBytes(lpPartFile->GetTransfered());
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			case 3:	// transfered complete
				{
					wxString buffer = CastItoXBytes(lpPartFile->GetCompletedSize());
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			case 4:	// speed
				{
					if ( lpPartFile->GetTransferingSrcCount() ) {
						wxString buffer = wxString::Format(wxT("%.1f "), lpPartFile->GetKBpsDown()) + _("kB/s");
						dc->DrawText(buffer, rect.GetX(), rect.GetY());
					}
				}
				break;

#ifndef DISABLE_PROGRESS
			case 5:	// progress
				{
					if (thePrefs::ShowProgBar()) {

						int iWidth  = rect.GetWidth();
						int iHeight = rect.GetHeight();
	
						// DO NOT DRAW IT ALL THE TIME
						DWORD dwTicks = GetTickCount();
						wxMemoryDC cdcStatus;

						if (lpCtrlItem->dwUpdated + DLC_BARUPDATE < dwTicks || !lpCtrlItem->status || iWidth != lpCtrlItem->status->GetWidth() || !lpCtrlItem->dwUpdated) {
							if (lpCtrlItem->status == NULL) {
								lpCtrlItem->status = new wxBitmap(iWidth, iHeight);
							} else {
								// Only recreate if the size has changed
								if ( ( lpCtrlItem->status->GetWidth() != iWidth ) ) 
									lpCtrlItem->status->Create(iWidth, iHeight);
							}
							cdcStatus.SelectObject(*(lpCtrlItem->status));
	
							lpPartFile->DrawStatusBar(&cdcStatus, wxRect(0, 0, iWidth, iHeight), thePrefs::UseFlatBar());
							lpCtrlItem->dwUpdated = dwTicks + 1000; // Plus one second
	
						} else {
							cdcStatus.SelectObject(*(lpCtrlItem->status));
						}

						dc->Blit( rect.GetX(), rect.GetY() + 1, iWidth, iHeight, &cdcStatus, 0, 0);
						cdcStatus.SelectObject(wxNullBitmap);
					}
					if (thePrefs::ShowPercent()) {
						// Percentage of completing
						// We strip anything below the first decimal point, to avoid Format doing roundings
						float percent = floor( lpPartFile->GetPercentCompleted() * 10.0f ) / 10.0f;
						
						wxString buffer = wxString::Format( wxT("%.1f%%"), percent );
						int middlex = (2*rect.GetX() + rect.GetWidth()) >> 1;
						int middley = (2*rect.GetY() + rect.GetHeight()) >> 1;
						dc->GetTextExtent(buffer, &textwidth, &textheight);
							wxColour AktColor = dc->GetTextForeground();
						if (thePrefs::ShowProgBar()) {
							dc->SetTextForeground(*wxWHITE);
						} else {	
							dc->SetTextForeground(*wxBLACK);
						}
						dc->DrawText(buffer, middlex - (textwidth >> 1), middley - (textheight >> 1));
						dc->SetTextForeground(AktColor);					
					}
				}
				break;
#endif

			case 6:	// sources
				{
					// Ok, after checking eMule's sources, I'm using my own implementation instead.
					uint16 sc = lpPartFile->GetSourceCount();
					uint16 ncsc = lpPartFile->GetNotCurrentSourcesCount();				

					wxString buffer;
					if ( ncsc ) {
						buffer = wxString::Format(wxT("%i/%i"), sc - ncsc, sc);
					} else {					
						buffer = wxString::Format(wxT("%i"),sc);
					}
					
					if ( lpPartFile->GetSrcA4AFCount() ) {
						buffer = buffer + wxString::Format(wxT("+%i"),lpPartFile->GetSrcA4AFCount());
					}
					
					buffer = buffer + wxString::Format(wxT(" (%i)"),lpPartFile->GetTransferingSrcCount());
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			case 7:	// Priority
				if ( lpPartFile->IsAutoDownPriority() ) {
					switch ( lpPartFile->GetDownPriority() ) {
						case PR_LOW:	dc->DrawText(_("Auto [Lo]"), rect.GetX(), rect.GetY()); break;
						case PR_NORMAL:	dc->DrawText(_("Auto [No]"), rect.GetX(), rect.GetY()); break;
						case PR_HIGH:	dc->DrawText(_("Auto [Hi]"), rect.GetX(), rect.GetY()); break;
					}
				} else {
					switch ( lpPartFile->GetDownPriority() ) {
						case PR_LOW:	dc->DrawText(_("Low"),    rect.GetX(), rect.GetY());	break;
						case PR_NORMAL:	dc->DrawText(_("Normal"), rect.GetX(), rect.GetY());	break;
						case PR_HIGH:	dc->DrawText(_("High"),   rect.GetX(), rect.GetY());	break;
					}
				}
				break;
			case 8: // Status
				dc->DrawText(lpPartFile->getPartfileStatus(), rect.GetX(), rect.GetY());
				break;

			case 9:	// remaining time & size
				{
					wxString buffer;
					if ( lpPartFile->GetStatus() != PS_COMPLETING && lpPartFile->GetStatus() != PS_COMPLETE ) {
						//size
						uint32 remains = lpPartFile->GetFileSize() - lpPartFile->GetCompletedSize();

						// time
						sint32 restTime = lpPartFile->getTimeRemaining();
						buffer = CastSecondsToHM(restTime) + wxT(" (") + CastItoXBytes(remains) + wxT(")");
					}

					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;
			case 10:	// last seen complete
				{
					wxString buffer;
					if ( lpPartFile->lastseencomplete ) {
						buffer = wxDateTime( lpPartFile->lastseencomplete ).Format( wxT("%y/%m/%d %H:%M:%S") );
					} else {
						buffer = _("Unknown");
					}
					
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;
			case 11:	// last receive
				{	
					wxString buffer;
					if ( lpPartFile->GetLastChangeDatetime() ) {
						buffer = wxDateTime( lpPartFile->GetLastChangeDatetime() ).Format( wxT("%y/%m/%d %H:%M:%S") );
					} else {
						buffer = _("Unknown");
					}

					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
		}
	}
}

#define ILD_NORMAL wxIMAGELIST_DRAW_TRANSPARENT

void CDownloadListCtrl::DrawSourceItem(wxDC * dc, int nColumn, const wxRect& rect, CtrlItem_Struct * lpCtrlItem)
{
	if ( rect.GetWidth() > 0 ) {

		// force clipper (clip 2 px more than the rectangle from the right side)
		wxDCClipper clipper(*dc, rect.GetX(), rect.GetY(), rect.GetWidth() - 2, rect.GetHeight());
		wxString buffer;
		CUpDownClient* lpUpDownClient = (CUpDownClient *) lpCtrlItem->value;
		
		switch (nColumn) {
			case 0:	// icon, name, status
				{
					wxRect cur_rec = rect;
					// +3 is added by OnDrawItem()... so take it off
					// Kry - eMule says +1, so I'm trusting it
					POINT point = { cur_rec.GetX(), cur_rec.GetY()+1 };
					
					if (lpCtrlItem->type == 2) {
						uint8 image = 0;
						switch (lpUpDownClient->GetDownloadState()) {
							case DS_CONNECTING:
							case DS_CONNECTED:
							case DS_WAITCALLBACK:
							case DS_TOOMANYCONNS:
								image = 1;
								break;
							case DS_ONQUEUE:
								if (lpUpDownClient->IsRemoteQueueFull()) {
									image = 3;
								} else {
									image = 2;
								}
								break;
							case DS_DOWNLOADING:
							case DS_REQHASHSET:
								image = 0;
								break;
							case DS_NONEEDEDPARTS:
							case DS_LOWTOLOWIP:
								image = 3;
								break;
							default:
								image = 4;
						}
								
						m_ImageList.Draw(image, *dc, point.x, point.y, ILD_NORMAL);
					} else {
						m_ImageList.Draw(3, *dc, point.x, point.y, ILD_NORMAL);
					}
					cur_rec.x += 20;
					POINT point2 = { cur_rec.GetX(), cur_rec.GetY() + 1 };
					
					uint8 clientImage;
					if (lpUpDownClient->IsFriend()) {
						clientImage = 13;
					} else {
						switch (lpUpDownClient->GetClientSoft()) {
							case SO_AMULE: 
								clientImage = 17;
								break;
							case SO_MLDONKEY:
							case SO_NEW_MLDONKEY:
							case SO_NEW2_MLDONKEY:
								clientImage = 15;
								break;
							case SO_EDONKEY:
							case SO_EDONKEYHYBRID:
								clientImage = 16;
								break;
							case SO_EMULE:
								clientImage = 14;
								break;
							case SO_LPHANT:
								clientImage = 18;
								break;
							case SO_SHAREAZA:
								clientImage = 19;
								break;
							case SO_LXMULE:
								clientImage = 20;
								break;
							default:
								// cDonkey, Compat Unk
								// No icon for those yet. Using the eMule one + '?'
								clientImage = 21;
								break;
						}	
					}
					
					m_ImageList.Draw(clientImage, *dc, point2.x, point.y, ILD_NORMAL);
					
					if (lpUpDownClient->ExtProtocolAvailable()) {
						// Ext protocol -> Draw the '+'
						m_ImageList.Draw(7, *dc, point2.x, point.y, ILD_NORMAL);		
					}
					
					if (lpUpDownClient->Credits()) {
						switch (lpUpDownClient->Credits()->GetCurrentIdentState(lpUpDownClient->GetIP())) {
							case IS_IDENTIFIED:
								// the 'v'
								m_ImageList.Draw(8, *dc, point2.x, point.y, ILD_NORMAL);
								break;		
							case IS_IDBADGUY:
								// the 'X'
								m_ImageList.Draw(9, *dc, point2.x, point.y, ILD_NORMAL);
								break;
							default:
								break;
						}
					}
					
					if (lpUpDownClient->GetUserName().IsEmpty()) {
						dc->DrawText(wxT("?"), rect.GetX() + 40, rect.GetY());
					} else {
						dc->DrawText( lpUpDownClient->GetUserName(), rect.GetX() + 40, rect.GetY());						
					}								
				}
				break;

			case 1:	// size
				break;

			case 2: // Transfered
				break;

			case 3:	// completed
				if (lpCtrlItem->type == 2 && lpUpDownClient->GetTransferedDown()) {
					buffer = CastItoXBytes(lpUpDownClient->GetTransferedDown());
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			case 4:	// speed

				if (lpCtrlItem->type == 2) {
					if (lpUpDownClient->GetKBpsDown()<0.001) {
						buffer = wxEmptyString;
					} else {
						buffer = wxString::Format(wxT("%.1f "), lpUpDownClient->GetKBpsDown()) + _("kB/s");					}
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				}
				break;

			#ifndef DISABLE_PROGRESS

			case 5:	// file info
				{
					if (thePrefs::ShowProgBar()) {
						int iWidth = rect.GetWidth();
						int iHeight = rect.GetHeight() - 2;

						DWORD dwTicks = GetTickCount();
						wxMemoryDC cdcStatus;

						if (lpCtrlItem->dwUpdated + (4 * DLC_BARUPDATE) < dwTicks || !lpCtrlItem->status || iWidth != lpCtrlItem->status->GetWidth() || !lpCtrlItem->dwUpdated) {
							if (lpCtrlItem->status == NULL) {
								lpCtrlItem->status = new wxBitmap(iWidth, iHeight);
							} else {
								// Only recreate if size has changed
								if ( lpCtrlItem->status->GetWidth() != iWidth )
									lpCtrlItem->status->Create(iWidth, iHeight);
							}

							cdcStatus.SelectObject(*(lpCtrlItem->status));

							lpUpDownClient->DrawStatusBar(&cdcStatus, wxRect(0, 0, iWidth, iHeight), (lpCtrlItem->type == 3), thePrefs::UseFlatBar());
							lpCtrlItem->dwUpdated = dwTicks + 1000; // Plus one second
						} else {
							cdcStatus.SelectObject(*(lpCtrlItem->status));
						}
						
						dc->Blit(rect.GetX(), rect.GetY() + 1, iWidth, iHeight, &cdcStatus, 0, 0);
						cdcStatus.SelectObject(wxNullBitmap);
					}
				}
				break;

			#endif

			case 6:{
				// Version				
				dc->DrawText(lpUpDownClient->GetClientVerString(), rect.GetX(), rect.GetY());
				break;
			}

			case 7:	// prio
				// We only show priority for sources actually queued for that file
				if ( ( lpCtrlItem->type == 2 ) && ( lpUpDownClient->GetDownloadState() == DS_ONQUEUE ) ) {
					if (lpUpDownClient->IsRemoteQueueFull()) {
						buffer = _("Queue Full");
						dc->DrawText(buffer, rect.GetX(), rect.GetY());
					} else {
						if (lpUpDownClient->GetRemoteQueueRank()) {
							sint16 qrDiff = lpUpDownClient->GetRemoteQueueRank() - lpUpDownClient->GetOldRemoteQueueRank();
							if(qrDiff == lpUpDownClient->GetRemoteQueueRank() )
								qrDiff = 0;
							wxColour savedColour = dc->GetTextForeground();
							if( qrDiff < 0 ) dc->SetTextForeground(*wxBLUE);
							if( qrDiff > 0 ) dc->SetTextForeground(*wxRED);
							//if( qrDiff == 0 ) dc->SetTextForeground(*wxLIGHT_GREY);
							buffer.Printf(wxT("QR: %u (%i)"), lpUpDownClient->GetRemoteQueueRank(), qrDiff);
							dc->DrawText(buffer, rect.GetX(), rect.GetY());
							dc->SetTextForeground(savedColour);
						} 
					}
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
					if (lpUpDownClient->GetRequestFile() && !lpUpDownClient->GetRequestFile()->GetFileName().IsEmpty())
						buffer += wxT(" (") + lpUpDownClient->GetRequestFile()->GetFileName() + wxT(")");
					
				}
				dc->DrawText(buffer, rect.GetX(), rect.GetY());
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
						if ((client->GetRequestFile() != partfile) || (ds != DS_DOWNLOADING)) {
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
					CoreNotify_PartFile_SetCat(selected, event.GetId() - MP_ASSIGNCAT);
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
						CoreNotify_PartFile_RemoveNoNeeded(selectedList.GetHead());
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
						CoreNotify_PartFile_RemoveFullQueue(selectedList.GetHead());
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
						CoreNotify_PartFile_RemoveHighQueue(selectedList.GetHead());
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
						CoreNotify_PartFile_SourceCleanup(selectedList.GetHead());
						// Remove this item from the selected items list
						selectedList.RemoveHead();
					}
					return true;					
					break;
					/* End modif */
				case MP_SWAP_A4AF_TO_THIS: {
					Freeze();
					if (selectedCount == 1) {
						CoreNotify_PartFile_Swap_A4AF(file);
					}
					Thaw();
					this->UpdateItem(file);						
					return true;					
					break;
				}
				case MP_SWAP_A4AF_TO_THIS_AUTO:
					CoreNotify_PartFile_Swap_A4AF_Auto(file);
					return true;				
					break;
				case MP_SWAP_A4AF_TO_ANY_OTHER: {
					Freeze();
					if (selectedCount == 1) {
						CoreNotify_PartFile_Swap_A4AF_Others(file);
					}
					Thaw();
					return true;					
					break;
				}
				case MP_CANCEL:	{
					if (selectedCount > 0) {
						Freeze();
						wxString fileList;
						bool validdelete = false;

						for (POSITION pos = selectedList.GetHeadPosition(); pos != 0; ) {
							CPartFile* cur_file = selectedList.GetNext(pos);
							
							if ( cur_file->GetStatus() != PS_COMPLETING && cur_file->GetStatus() != PS_COMPLETE) {
								validdelete = true;
								fileList += wxT("\n");
								fileList += cur_file->GetFileName();
							}
						}
						wxString quest;
						if (selectedCount==1) {
							// for single selection
							quest=_("Are you sure that you want to cancel and delete this file ?\n");
						} else {
							// for multiple selections
							quest=_("Are you sure that you want to cancel and delete these files ?\n");
						}
						if (validdelete && wxMessageBox((quest + fileList), _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES) {
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
										CoreNotify_PartFile_Delete(selectedList.GetHead());
										selectedList.RemoveHead();
										break;
									default:
										CoreNotify_PartFile_Delete(selectedList.GetHead());
										selectedList.RemoveHead();
								}
							}
						}
						Thaw();
					}
					return true;					
					break;
				}
				case MP_PRIOHIGH:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_PrioAuto(selectedList.GetHead(), false);
							CoreNotify_PartFile_PrioSet(selectedList.GetHead(), PR_HIGH, true);
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_PrioAuto(file, false);
					CoreNotify_PartFile_PrioSet(file, PR_HIGH, true);
					return true;					
					break;
				case MP_PRIOLOW:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_PrioAuto(selectedList.GetHead(), false);
							CoreNotify_PartFile_PrioSet(selectedList.GetHead(), PR_LOW, true);
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_PrioAuto(file, false);
					CoreNotify_PartFile_PrioSet(file, PR_LOW, true);
					return true;					
					break;
				case MP_PRIONORMAL:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_PrioAuto(selectedList.GetHead(), false);
							CoreNotify_PartFile_PrioSet(selectedList.GetHead(), PR_NORMAL, true);
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_PrioAuto(file, false);
					CoreNotify_PartFile_PrioSet(file, PR_NORMAL, true);
					return true;					
					break;
				case MP_PRIOAUTO:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_PrioAuto(selectedList.GetHead(), true);
							CoreNotify_PartFile_PrioSet(selectedList.GetHead(), PR_HIGH, true);
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_PrioAuto(file, true);
					CoreNotify_PartFile_PrioSet(file, PR_HIGH, true);
					return true;					
					break;
				case MP_PAUSE:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_Pause(selectedList.GetHead());
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_Pause(file);
					return true;					
					break;
				case MP_RESUME:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CoreNotify_PartFile_Resume(selectedList.GetHead());
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					CoreNotify_PartFile_Resume(file);
					return true;					
					break;
				case MP_STOP:
					if (selectedCount > 1) {
						Freeze();
						while (!selectedList.IsEmpty()) {
							CPartFile *selected = selectedList.GetHead();
							HideSources(selected);
							CoreNotify_PartFile_Stop(selected);
							selectedList.RemoveHead();
						}
						Thaw();
						return true;						
						break;
					}
					HideSources(file);
					CoreNotify_PartFile_Stop(file);
					return true;					
					break;
				case MP_FAKECHECK1:	// deltahf -> fakecheck
					    theApp.amuledlg->LaunchUrl(theApp.GenFakeCheckUrl(file));
					break;
				case MP_FAKECHECK2:
						theApp.amuledlg->LaunchUrl(theApp.GenFakeCheckUrl2(file));
					break;
				case MP_CLEARCOMPLETED:
					Freeze();
					ClearCompleted();
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
							str += theApp.CreateED2kLink(selectedList.GetHead()) + wxT("\n");
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
						wxString str;
						while (!selectedList.IsEmpty()) {
							str += theApp.CreateHTMLED2kLink(selectedList.GetHead()) + wxT("\n");
							selectedList.RemoveHead();
						}
						theApp.CopyTextToClipboard(str);
						return true;						
						break;
					}
					theApp.CopyTextToClipboard(theApp.CreateHTMLED2kLink(file));
					return true;					
					break;
				case MP_WS :{
					wxString feed = wxEmptyString;
					feed += wxString(_("Feedback from: ")) + thePrefs::GetUserNick() + wxString(wxT("\r\n"));
					feed += wxString(_("Client: aMule ")) +  wxString(wxT(VERSION)) + wxString(wxT("\r\n"));
					feed += wxString(_("File Name: ")) + file->GetFileName() + wxString(wxT("\r\n"));
					feed += wxString::Format(_("File size: %i MB"), file->GetFileSize()/1048576) + wxString(wxT("\r\n"));; 
					feed += wxString::Format(_("Download: %i MB"), file->GetCompletedSize()/1048576) + wxString(wxT("\r\n"));; 
					feed += wxString::Format(_("Sources: %i"), file->GetSourceCount()) + wxString(wxT("\r\n"));; 
					feed += wxString::Format(_("Complete Sources: %i"), file->m_nCompleteSourcesCount) + wxString(wxT("\r\n"));; 
                                        theApp.CopyTextToClipboard(feed);
					break;
				}
				case MP_OPEN:{
						if (selectedCount > 1) {
							break;
						}
						char *buffer = new char[250];
						sprintf(buffer, "%s/%s", unicode2char(thePrefs::GetIncomingDir()), unicode2char(file->GetFileName()));
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
			}
		} else {
			CUpDownClient *client = (CUpDownClient*)content->value;
			CPartFile* file = (CPartFile*)(content->parent)->value;
			switch (event.GetId()) {
				case MP_CHANGE2FILE:
					client->SwapToAnotherFile(true,false,false,file);
					
					return true;
				case MP_SHOWLIST:
					client->RequestSharedFileList();
					return true;				
					break;
				case MP_ADDFRIEND:
					theApp.amuledlg->chatwnd->AddFriend(client);
					return true;				
					break;
				case MP_SENDMESSAGE: {
					wxString message = ::wxGetTextFromUser(_("Send message to user"),_("Message to send:"));
					if (!message.IsEmpty()) {
						// false -> no focus set
						theApp.amuledlg->chatwnd->StartSession(client, false);
						theApp.amuledlg->chatwnd->SendMessage(message);
					}
					return true;				
					break;
				}
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


int CDownloadListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CtrlItem_Struct* item1 = (CtrlItem_Struct *) lParam1;
	CtrlItem_Struct* item2 = (CtrlItem_Struct *) lParam2;

	int sortMod = 1;
	if (lParamSort >= 1000) {
		sortMod = -1;
		lParamSort -= 1000;
	}

	int comp = 0;

	if ( item1->type == FILE_TYPE ) {
		if ( item2->type == FILE_TYPE ) {
			// Both are files, so we just compare them
			comp = Compare( (CPartFile*)item1->value, (CPartFile*)item2->value, lParamSort);
		} else {
			// A file and a source, checking if they belong to each other
			if ( item1->value == item2->owner ) {
				// A file should always be above its sources
				// Returning directly to avoid the modifier
				return -1;
			} else {
				// Source belongs to anther file, so we compare the files instead	
				comp = Compare( (CPartFile*)item1->value, item2->owner, lParamSort);
			}
		}
	} else {
		if ( item2->type == FILE_TYPE ) {
			// A source and a file, checking if they belong to each other
			if ( item1->owner == item2->value ) {
				// A source should always be below its file
				// Returning directly to avoid the modifier
				return 1;
			} else {
				// Source belongs to anther file, so we compare the files instead	
				comp = Compare( item1->owner, (CPartFile*)item2->value, lParamSort);
			}
		} else {
			// Two sources, some different possibilites
			if ( item1->owner == item2->owner ) {
				// Avilable sources first, if we have both an available and an unavailable			
				comp = ( item1->type - item2->type );

				// Do we need to futher compare them? Happens if both have same type.
				if ( !comp ) {
					comp = Compare( (CUpDownClient*)item1->value, (CUpDownClient*)item2->value, lParamSort);
				}
			} else {
				// Belongs to different files, so we compare the files
				comp = Compare( item1->owner, item2->owner, lParamSort);
			}
		}
	}

	// We modify the result so that it matches with ascending or decending
	return sortMod * comp;
}


int CDownloadListCtrl::Compare(CPartFile* file1, CPartFile* file2, long lParamSort)
{
	int result = 0;

	switch (lParamSort) {
		// Sort by filename		
		case 0:
			result = file1->GetFileName().CmpNoCase( file2->GetFileName() );
			break;
			
		// Sort by size
		case 1:	
			result = CmpAny( file1->GetFileSize(), file2->GetFileSize() );
			break;
			
		// Sort by transfered
		case 2:	
			result = CmpAny( file1->GetTransfered(), file2->GetTransfered() );
			break;
		
		// Sort by completed
		case 3:	
			result = CmpAny( file1->GetCompletedSize(), file2->GetCompletedSize() );
			break;
		
		// Sort by speed
		case 4:	
			result = CmpAny( file1->GetKBpsDown()*1024, file2->GetKBpsDown()*1024 );
			break;
		
		// Sort by percentage completed
		case 5:
			result = CmpAny( file1->GetPercentCompleted(), file2->GetPercentCompleted() );
			break;
		
		// Sort by number of sources		
		case 6:	
			result = CmpAny( file1->GetSourceCount(), file2->GetSourceCount() );
			break;
		
		// Sort by priority
		case 7:
			result = CmpAny( file1->GetDownPriority(), file2->GetDownPriority() );
			break;
		
		// Sort by status
		case 8:
			result = CmpAny( file1->getPartfileStatusRang(), file2->getPartfileStatusRang() );
			break;
		
		// Sort by remaining time
		case 9:
			result = CmpAny( file1->getTimeRemaining(), file2->getTimeRemaining() );
			break;
		
		// Sort by last seen complete
		case 10:
			result = CmpAny( file1->lastseencomplete, file2->lastseencomplete );
			break;
		
		// Sort by last reception
        case 11:
			result = CmpAny( file1->GetLastChangeDatetime(), file2->GetLastChangeDatetime() ); 
			break;
	}


	// We cannot have that two files are equal, since that will screw up 
	// the placement of sources. So if they are equal, we first try to use the 
	// last sort-type and then fall back on something that is bound to be unique 
	// and will give a consistant result: Their hashes.
	if ( !result ) {
		// Try to sort by the last column
		if ( s_lastColumn != lParamSort )
			result = s_lastOrder * Compare( file1, file2, s_lastColumn );
		
		// If that failed as well, then we sort by hash
		if ( !result )
			result = CmpAny( file1->GetFileHash(), file2->GetFileHash() );
	}

	return result;
}

int CDownloadListCtrl::Compare(const CUpDownClient* client1, const CUpDownClient* client2, long lParamSort)
{
	switch (lParamSort) {
		// Sort by name
		case 0:	return client1->GetUserName().CmpNoCase( client2->GetUserName() );
		// Sort by status (size field)
		case 1:	return CmpAny( client1->GetDownloadState(), client2->GetDownloadState() );
		// Sort by transfered in the following fields
		case 2:	// Completed field
		case 3:	// Transfered field
			return CmpAny( client1->GetTransferedDown(), client2->GetTransferedDown() );
		// Sort by speed
		case 4:	return CmpAny( client1->GetKBpsDown(), client2->GetKBpsDown() );
		// Sort by parts offered (Progress field)
		case 5:	return CmpAny( client1->GetAvailablePartCount(), client2->GetAvailablePartCount() );
		// Sort by client version
		case 6:
			{
				if ( client1->GetClientSoft() == client2->GetClientSoft() ) {
					if (client1->IsEmuleClient()) {
						return CmpAny( client2->GetMuleVersion(), client1->GetMuleVersion() );
					} else {
						return CmpAny( client2->GetVersion(), client1->GetVersion() );
					}
				} else {
					return CmpAny( client1->GetClientSoft(), client2->GetClientSoft() );
				}
			}
		// Sort by Queue-Rank
		case 7:
			{
				// This will sort by download state: Downloading, OnQueue, Connecting ... 
				// However, Asked For Another will always be placed last, due to sorting in SortProc
				if ( client1->GetDownloadState() != client2->GetDownloadState() ) {
					return client1->GetDownloadState() - client2->GetDownloadState();
				}
				
				// Placing items on queue before items on full queues
				if ( client1->IsRemoteQueueFull() ) {
					if ( client2->IsRemoteQueueFull() ) {
						return 0;
					} else {
						return  1;
					}
				} else if ( client2->IsRemoteQueueFull() ) {
					return -1;
				} else {
					if ( client1->GetRemoteQueueRank() ) {
						if ( client2->GetRemoteQueueRank() ) {
							return CmpAny( client1->GetRemoteQueueRank(), client2->GetRemoteQueueRank() );
						} else {
							return -1;
						}
					} else {
						if ( client2->GetRemoteQueueRank() ) {
							return  1;
						} else {
							return  0;
						}
					}
				}
			}
		// Sort by state
		case 8:
			if (client1->GetDownloadState() == client2->GetDownloadState()) {
				if (client1->IsRemoteQueueFull())
					return 1;
					
				if (client2->IsRemoteQueueFull())
					return -1;
					
				return 0;
			} else {
				return CmpAny( client1->GetDownloadState(), client2->GetDownloadState() );
			}
		default:
			return 0;
	}
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
	wxString counter;
	uint16 count = 0;	//theApp.downloadqueue->GetFileCount();

	// remove all displayed files with a different cat
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
		const CtrlItem_Struct *cur_item = it->second;
		if (cur_item->type == FILE_TYPE) {
			CPartFile *file = (CPartFile *) cur_item->value;
			if (file->GetCategory() == curTab || (!thePrefs::ShowAllNotCats() && file->GetCategory() > 0 && curTab == 0)) {
				count++;
			}
		}
	}

	wxString fmtstr = wxString::Format(_("Downloads (%i)"), GetItemCount());
	CastByName( wxT("downloadsLabel"), GetParent(), wxStaticText )->SetLabel(fmtstr);
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
			if (!file->CheckShowItemInGivenCat(newsel)) {
				HideFile(file);
			} else {
				ShowFile(file);
			}
		}
	}
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
			int newitem = InsertItem(GetItemCount(), wxT("This is not visible"));
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
	return (((newsel == 0 && !thePrefs::ShowAllNotCats()) || (newsel == 0 && thePrefs::ShowAllNotCats() && file->GetCategory() == 0)) || (newsel > 0 && newsel == file->GetCategory()));
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


void CDownloadListCtrl::SetCatStatus(int cat, int newstatus)
{
	std::list<CPartFile*> fileList;

	// It's not safe to do this manipulation directly, so we first make a list of
	// files that we are going to change.
	ListItems::iterator it = m_ListItems.begin();
	for ( ; it != m_ListItems.end(); it++ ) {
		if ( ( it->second->type == FILE_TYPE ) && it->second->value ) {
			CPartFile* cur_file = (CPartFile*)it->second->value;

			if ( cur_file->CheckShowItemInGivenCat(cat) ) {
				fileList.push_back( cur_file );
			}
		}
	}

	if ( !fileList.empty() ) {
		std::list<CPartFile*>::iterator it = fileList.begin();
		
		for ( ; it != fileList.end(); it++ ) {
			switch ( newstatus ) {
				case MP_CANCEL:
					(*it)->Delete();
					break;
				case MP_PAUSE:
					(*it)->PauseFile();
					break;
				case MP_STOP:
					(*it)->StopFile();
					break;
				case MP_RESUME:
					if ( (*it)->GetStatus() == PS_PAUSED ) {
						(*it)->ResumeFile();
					}
					break;
			}
		}
	}
}


