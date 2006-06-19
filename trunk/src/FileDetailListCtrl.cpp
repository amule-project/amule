//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "muuli_wdr.h"		// Needed for ID_CLOSEWNDFD
#include "FileDetailListCtrl.h"	// Interface declarations

#define LVCFMT_LEFT wxLIST_FORMAT_LEFT
#define wxLIST_STATE_DESELECTED 0x0000

BEGIN_EVENT_TABLE(CFileDetailListCtrl, CMuleListCtrl)
	EVT_LIST_ITEM_SELECTED(IDC_LISTCTRLFILENAMES, CFileDetailListCtrl::OnSelect) // Care for single selection
END_EVENT_TABLE()


CFileDetailListCtrl::CFileDetailListCtrl(wxWindow * &parent, int id, const wxPoint & pos, wxSize siz, int flags):CMuleListCtrl(parent, id, pos, siz, flags)
{
	// Set sorter function
	SetSortFunc(SortProc);
	
	// Initial sorting: Sources descending
	InsertColumn(0, _("File Name"), LVCFMT_LEFT, 370);
	InsertColumn(1, _("Sources"), LVCFMT_LEFT, 70);
	
	SetSorting(1, CMuleListCtrl::SORT_DES);

	SortList();
}

int CFileDetailListCtrl::SortProc(long param1, long param2, long sortData)
{ 
	// Comparison for different sortings
	SourcenameItem *item1 = (SourcenameItem*)param1;
	SourcenameItem *item2 = (SourcenameItem*)param2;

	int mod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	
	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
		case 1: return mod * (item1->count - item2->count);			// Sources descending
		case 0: return mod * item1->name.CmpNoCase(item2->name);	// Name descending
		default: return 0;
	}
} 


void CFileDetailListCtrl::OnSelect(wxListEvent& event)
{
	// Damn wxLC_SINGLE_SEL does not work! So we have to care for single selection ourselfs:
	long realpos = event.m_itemIndex;
	long pos=-1;
	for(;;) {
		// Loop through all selected items
		pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		if(pos==-1) {
			// No more selected items left
			break;
		} else if (pos != realpos) {
			// Deselect all items except the one we have just clicked
			SetItemState(pos, wxLIST_STATE_DESELECTED, wxLIST_STATE_SELECTED);
		}
	}

	event.Skip();
}
// File_checked_for_headers
