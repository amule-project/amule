//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <wx/defs.h>			// Needed before any other wx/*.h
#include <wx/menu.h>			// Needed for wxMenu
#include <wx/config.h>		// Needed for wxConfig in wx-2.4.2
#include <wx/fileconf.h>		// Needed for wxConfig
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/imaglist.h>

#include <common/MuleDebug.h>			// Needed for MULE_VALIDATE_
#include <common/StringFunctions.h>	// Needed for StrToLong

#include "MuleListCtrl.h"		// Interface declarations
#include "OPCodes.h"			// Needed for MP_LISTCOL_1
#include "GetTickCount.h"		// Needed for GetTickCount()

#include <cctype>				// Needed for isprint() and tolower

// For arrow-pixmaps
#include "pixmaps/sort_dn.xpm"
#include "pixmaps/sort_up.xpm"
#include "pixmaps/sort_dnx2.xpm"
#include "pixmaps/sort_upx2.xpm"


// Global constants
#ifdef __WXGTK__
	const int COL_SIZE_MIN = 10;
#elif defined(__WXMSW__) || defined(__WXMAC__) || defined(__WXCOCOA__)
	const int COL_SIZE_MIN = 0;
#else
	#error Need to set col_minsize for your OS
#endif


BEGIN_EVENT_TABLE(CMuleListCtrl, MuleExtern::wxGenericListCtrl)
	EVT_LIST_COL_CLICK( -1, 		CMuleListCtrl::OnColumnLClick)
	EVT_LIST_COL_RIGHT_CLICK( -1,	CMuleListCtrl::OnColumnRClick)
	EVT_LIST_ITEM_SELECTED(-1,		CMuleListCtrl::OnItemSelected)
	EVT_CHAR(						CMuleListCtrl::OnChar)
	EVT_MENU_RANGE(MP_LISTCOL_1, MP_LISTCOL_15, CMuleListCtrl::OnMenuSelected)
	EVT_MOUSEWHEEL(CMuleListCtrl::OnMouseWheel)
END_EVENT_TABLE()


//! Shared list of arrow-pixmaps
static wxImageListType imgList(16, 16, true, 0);
	

CMuleListCtrl::CMuleListCtrl(wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: MuleExtern::wxGenericListCtrl(parent, winid, pos, size, style, validator, name)
{
	m_sort_func = NULL;
	m_sort_order = 0;
	m_sort_column = 0;
	m_tts_time = 0;
	m_tts_item = -1;

	if (imgList.GetImageCount() == 0) {
		imgList.Add(wxBitmap(sort_dn_xpm));
		imgList.Add(wxBitmap(sort_up_xpm));
		imgList.Add(wxBitmap(sort_dnx2_xpm));
		imgList.Add(wxBitmap(sort_upx2_xpm));
	}
	
	SetImageList(&imgList, wxIMAGE_LIST_SMALL);
}


CMuleListCtrl::~CMuleListCtrl()
{
	// If the user specified a name for the list, then its options will be
	// saved upon destruction of the listctrl.
	SaveSettings();
}


void CMuleListCtrl::SaveSettings()
{
	// Dont save tables with no specified name
	if ( m_name.IsEmpty() ) {
		return;
	}
	
	wxConfigBase* cfg = wxConfigBase::Get();

	// Save sorting, column and order
	cfg->Write(wxT("/eMule/TableSorting") + m_name, (long)(m_sort_column | m_sort_order));

	// Save column widths. ATM this is also used to signify hidden columns.
	wxString buffer;
	for ( int i = 0; i < GetColumnCount(); ++i ) {
		if ( i ) buffer << wxT(",");

		buffer << GetColumnWidth(i);
	}

	cfg->Write( wxT("/eMule/TableWidths") +m_name, buffer );
}	


void CMuleListCtrl::LoadSettings()
{
	// Dont save tables with no specified name
	if (m_name.IsEmpty()) {
		return;
	}

	wxConfigBase* cfg = wxConfigBase::Get();

	// Load sort order (including sort-column)
	long setting = cfg->Read(wxT("/eMule/TableSorting") + m_name, 0l);
	
	unsigned column = setting & COLUMN_MASK;
	unsigned order  = setting & SORTING_MASK;
	
	// Sainity checking, to avoid asserting due to wrong saved settings
	if (column >= (unsigned)GetColumnCount()) {
		column = order = 0;
	}

	// Set the column widts
	wxString buffer;
	if (cfg->Read( wxT("/eMule/TableWidths") + m_name, &buffer, wxEmptyString)) {
		int counter = 0;
		
		wxStringTokenizer tokenizer( buffer, wxT(",") );
		while (tokenizer.HasMoreTokens() && (counter < GetColumnCount())) {
			SetColumnWidth(counter++, StrToLong( tokenizer.GetNextToken()));
		}
	}
	
	if (GetColumnCount()) {
		// Update the sorting
		SetSorting(column, order);
		SortList();
	}
}


long CMuleListCtrl::GetInsertPos(long data)
{
	// Get the sort-function pointer
	wxListCtrlCompare compare = m_sort_func;

	// Find the best place to position the item through a binary search
	int Min = 0;
	int Max = GetItemCount();

	// Only do this if there are any items and a sorter function
	if (Max && compare) {
		// This search will narrow down the best place to position the new
		// item. The result will be the item after that position, which is
		// the format expected by the insertion function.
		do {
			int cur_pos = ( Max - Min ) / 2 + Min;
			int cmp = compare(data, GetItemData(cur_pos), m_sort_column | m_sort_order);
			
			// Value is lesser than the one at the current pos
			if ( cmp < 0 ) {
				Max = cur_pos;
			} else {
				Min = cur_pos + 1;
			}			
		} while ((Min != Max));
	}

	return Max;
}


void CMuleListCtrl::SortList()
{
	if (m_sort_func) {
		// Positions are likely to be invalid after sorting.
		ResetTTS();
		
		SortItems(m_sort_func, m_sort_column | m_sort_order);
	}
}


CMuleListCtrl::ItemDataList CMuleListCtrl::GetSelectedItems() const
{
	// Create the initial vector with as many items as are selected
	ItemDataList list( GetSelectedItemCount() );
	
	// Current item being located
	unsigned int current = 0;
	
	long pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	while ( pos != -1 ) {
		wxASSERT( current < list.size() );
	
		list[ current++ ] = GetItemData( pos );

		pos = GetNextItem( pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
	
	return list;
}


void CMuleListCtrl::OnColumnRClick(wxListEvent& evt)
{
	wxMenu menu;
	wxListItem item;
	
	for ( int i = 0; i < GetColumnCount() && i < 15; ++i) {
		GetColumn(i, item);

		menu.AppendCheckItem(i + MP_LISTCOL_1, item.GetText() );
		menu.Check( i + MP_LISTCOL_1, GetColumnWidth(i) > COL_SIZE_MIN );
	}

	PopupMenu(&menu, evt.GetPoint());
}


void CMuleListCtrl::OnMenuSelected( wxCommandEvent& evt )
{
	int col = evt.GetId() - MP_LISTCOL_1;

	if (GetColumnWidth(col) > COL_SIZE_MIN) {
		SetColumnWidth(col, 0);
	} else {
		SetColumnWidth(col, wxLIST_AUTOSIZE);
	}	
}


void CMuleListCtrl::OnColumnLClick(wxListEvent& evt)
{
	// Stop if no sorter-function has been defined
	if (!m_sort_func) {
		return;
	}

	// Get the currently focused item
	long pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	long item = -1;
	if (pos != -1) {
		item = GetItemData(pos);
	}
	
	// If the user clicked on the same column, then revert the order, 
	// otherwise sort ascending.
	if ((unsigned)evt.GetColumn() == m_sort_column) {
		if (m_sort_order & SORT_DES) {
			if (AltSortAllowed(m_sort_column)) {
				m_sort_order = (~m_sort_order) & SORT_ALT;
			} else {
				m_sort_order = 0;
			}
		} else {
			m_sort_order = SORT_DES | (m_sort_order & SORT_ALT);
		}
	} else {
		m_sort_order = 0;
	}

	SetSorting(evt.GetColumn(), m_sort_order);
	

	// The sortlist function does the acutal work
	SortList();
	
	// Set focus on item if any was focused
	if (item != -1) {
		long it_pos = FindItem(-1,item);
		if (it_pos != -1) {
			SetItemState(it_pos,wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
		}
	}
}


void CMuleListCtrl::ClearSelection() 
{
	if (GetSelectedItemCount()) {
		long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		while (index != -1) {
			SetItemState(index, 0, wxLIST_STATE_SELECTED);
			index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}
	}
}


void CMuleListCtrl::SetTableName(const wxString& name)
{
	m_name = name;
}


unsigned CMuleListCtrl::GetSortColumn() const
{
	return m_sort_column;
}


unsigned CMuleListCtrl::GetSortOrder() const
{
	return m_sort_order;
}


void CMuleListCtrl::SetSortFunc(wxListCtrlCompare func)
{
	m_sort_func = func;
}


bool CMuleListCtrl::AltSortAllowed(unsigned WXUNUSED(column)) const
{
	return false;
}


void CMuleListCtrl::SetSorting(unsigned column, unsigned order)
{
	MULE_VALIDATE_PARAMS(column < (unsigned)GetColumnCount(), wxT("Invalid column to sort by."));
	MULE_VALIDATE_PARAMS(!(order & ~SORTING_MASK), wxT("Sorting order contains invalid data."));
	
	// Unset old column
	SetColumnImage(m_sort_column, -1);
	
	m_sort_column = column;
	m_sort_order  = order;

	if (order & SORT_DES) {
		SetColumnImage(column, (order & SORT_ALT) ? 2 : 0);
	} else {
		SetColumnImage(column, (order & SORT_ALT) ? 3 : 1);
	}
}


bool CMuleListCtrl::IsItemSorted(long item)
{
	wxCHECK_MSG(m_sort_func, true, wxT("No sort function specified!"));
	
	int sortby = m_sort_column | m_sort_order;
	bool sorted = true;
	long data = GetItemData(item);

	// Check that the item before the current item is smaller (or equal)
	if (item > 0) {
		sorted &= (m_sort_func(GetItemData(item - 1), data, sortby) <= 0);
	}

	// Check that the item after the current item is greater (or equal)
	if (sorted and (item < GetItemCount() - 1)) {
		sorted &= (m_sort_func(GetItemData(item + 1), data, sortby) >= 0);
	}

	return sorted;
}


void CMuleListCtrl::OnMouseWheel(wxMouseEvent &event)
{
 	// This enables scrolling with the mouse wheel
	event.Skip();
}


void CMuleListCtrl::SetColumnImage(unsigned col, int image)
{
    wxListItem item;
    item.SetMask(wxLIST_MASK_IMAGE);
	item.SetImage(image);
    SetColumn(col, item);	
}


long CMuleListCtrl::CheckSelection(wxMouseEvent& event)
{
	int flags = 0;
	wxListEvent evt;
	evt.m_itemIndex = HitTest(event.GetPosition(), flags);

	return CheckSelection(evt);
}


long CMuleListCtrl::CheckSelection(wxListEvent& event)
{
	long item = event.GetIndex();
	
	// Check if clicked item is selected. If not, unselect all and select it.
	if ((item != -1) && !GetItemState(item, wxLIST_STATE_SELECTED)) {
		ClearSelection();
		
		SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
	
	return item;
}


wxString CMuleListCtrl::GetTTSText(unsigned item) const
{
	MULE_VALIDATE_PARAMS(item < (unsigned)GetItemCount(), wxT("Invalid row."));
	MULE_VALIDATE_STATE((GetWindowStyle() & wxLC_OWNERDRAW) == 0,
		wxT("GetTTSText must be overwritten for owner-drawn lists."));

	return GetItemText(item);
}


void CMuleListCtrl::OnChar(wxKeyEvent& evt)
{
	if (evt.AltDown() or evt.ControlDown() or evt.MetaDown()) {
		if (evt.CmdDown() and (evt.GetKeyCode() == wxT('a'))) {
			// Ctrl+a (Command+a on Mac) was pressed, select all items
			for (int i = 0; i < GetItemCount(); ++i) {
				SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			}
		}
		
		evt.Skip();
		return;
	} else if (m_tts_time + 1500u < GetTickCount()) {
		m_tts_text.Clear();
	}
	
	m_tts_time = GetTickCount();
	m_tts_text.Append(tolower(evt.GetKeyCode()));
	unsigned next = (m_tts_item == -1) ? 0 : m_tts_item;
	
	for (unsigned i = 0, count = GetItemCount(); i < count; ++i) {
		wxString text = GetTTSText((next + i) % count).MakeLower();

		if (text.StartsWith(m_tts_text)) {
			ClearSelection();
			
			m_tts_item = (next + i) % count;
			SetItemState(m_tts_item, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED, 
					wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
			EnsureVisible(m_tts_item);

			return;
		}
	}
	
	if (m_tts_item != -1) {
		// Crop the string so that it matches the old item (avoid typos).
		wxString text = GetTTSText(m_tts_item).MakeLower();
		
		// If the last key didn't result in a hit, then we skip the event.
		if (!text.StartsWith(m_tts_text)) {
			if ((m_tts_text.Length() == 2) && (m_tts_text[0] == m_tts_text[1])) {
				// Special case, single-char, repeated. This allows toggeling
				// between items starting with a specific letter.
				m_tts_text.Clear();
				// Increment, so the next will be selected (or wrap around).
				m_tts_item++;
				OnChar(evt);
			} else {
				m_tts_text.RemoveLast();
				evt.Skip(true);
			}
		}
	} else {
		evt.Skip(true);
	}
}


void CMuleListCtrl::OnItemSelected(wxListEvent& evt)
{
	// We reset the current TTS session if the user manually changes the selection
	if (m_tts_item != evt.GetIndex()) {
		ResetTTS();

		// The item is changed so that the next TTS starts from the selected item.
		m_tts_item = evt.GetIndex();
	}

	evt.Skip();
}


void CMuleListCtrl::ResetTTS()
{
	m_tts_item = -1;
	m_tts_time =  0;
}
