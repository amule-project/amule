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

#include <wx/defs.h>			// Needed before any other wx/*.h
#include <wx/menu.h>			// Needed for wxMenu
#include <wx/config.h>		// Needed for wxConfig in wx-2.4.2
#include <wx/fileconf.h>		// Needed for wxConfig
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/imaglist.h>

#include <common/MuleDebug.h>			// Needed for MULE_VALIDATE_
#include <common/StringFunctions.h>		// Needed for StrToLong

#include "MuleListCtrl.h"		// Interface declarations
#include "OPCodes.h"			// Needed for MP_LISTCOL_1
#include "GetTickCount.h"		// Needed for GetTickCount()
#include "OtherFunctions.h"

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
	#error Need to define COL_SIZE_MIN for your OS
#endif


BEGIN_EVENT_TABLE(CMuleListCtrl, MuleExtern::wxGenericListCtrl)
	EVT_LIST_COL_CLICK( -1, 		CMuleListCtrl::OnColumnLClick)
	EVT_LIST_COL_RIGHT_CLICK( -1,	CMuleListCtrl::OnColumnRClick)
	EVT_LIST_ITEM_SELECTED(-1,		CMuleListCtrl::OnItemSelected)
	EVT_LIST_DELETE_ITEM(-1,		CMuleListCtrl::OnItemDeleted)
	EVT_LIST_DELETE_ALL_ITEMS(-1,	CMuleListCtrl::OnAllItemsDeleted)
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
	m_tts_time = 0;
	m_tts_item = -1;

	if (imgList.GetImageCount() == 0) {
		imgList.Add(wxBitmap(sort_dn_xpm));
		imgList.Add(wxBitmap(sort_up_xpm));
		imgList.Add(wxBitmap(sort_dnx2_xpm));
		imgList.Add(wxBitmap(sort_upx2_xpm));
	}

	// Default sort-order is to sort by the first column (asc).
	m_sort_orders.push_back(CColPair(0, 0));
	
	SetImageList(&imgList, wxIMAGE_LIST_SMALL);
}


CMuleListCtrl::~CMuleListCtrl()
{
	if (not m_name.IsEmpty()) {
		SaveSettings();
	}
}


void CMuleListCtrl::SaveSettings()
{
	wxCHECK_RET(not m_name.IsEmpty(), wxT("Cannot save settings for unnamed list"));
	
	wxConfigBase* cfg = wxConfigBase::Get();

	// Save sorting, column and order
	wxString sortOrder;
	for (CSortingList::iterator it = m_sort_orders.begin(); it != m_sort_orders.end(); ++it) {
		sortOrder += wxString::Format(wxT("%u %u, "), it->first, it->second);
	}
	
	cfg->Write(wxT("/eMule/TableOrdering") + m_name, sortOrder);

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
	wxCHECK_RET(not m_name.IsEmpty(), wxT("Cannot load settings for unnamed list"));

	wxConfigBase* cfg = wxConfigBase::Get();

	// Load sort order (including sort-column)
	m_sort_orders.clear();
	wxString setting = cfg->Read(wxT("/eMule/TableOrdering") + m_name, wxEmptyString);
	
	// Prevent sorting from occuring when calling SetSorting
	wxListCtrlCompare sortFunc = m_sort_func;
	m_sort_func = NULL;
	
	wxStringTokenizer tokens(setting, wxT(","));
	while (tokens.HasMoreTokens()) {
		wxString token = tokens.GetNextToken();

		unsigned long column = 0, order = 0;

		if (token.BeforeFirst(wxT(' ')).Strip(wxString::both).ToULong(&column)) {
			if (token.AfterFirst(wxT(' ')).Strip(wxString::both).ToULong(&order)) {
				// Sanity checking, to avoid asserting if column count changes.
				if (column < (unsigned)GetColumnCount()) {
					// Sanity checking, to avoid asserting if data-format changes.
					if ((order & ~SORTING_MASK) == 0) {
						// SetSorting will take care of duplicate entries
						SetSorting(column, order);
					}
				}
			}
		}
	}
	
	// Must have at least one sort-order specified
	if (m_sort_orders.empty()) {
		m_sort_orders.push_back(CColPair(0, 0));
	}

	// Re-enable sorting and resort the contents (if any).
	m_sort_func = sortFunc;
	SortList();	

	// Set the column widths
	wxString buffer;
	if (cfg->Read( wxT("/eMule/TableWidths") + m_name, &buffer, wxEmptyString)) {
		int counter = 0;
		
		wxStringTokenizer tokenizer( buffer, wxT(",") );
		while (tokenizer.HasMoreTokens() && (counter < GetColumnCount())) {
			SetColumnWidth(counter++, StrToLong( tokenizer.GetNextToken()));
		}
	}
}


long CMuleListCtrl::GetInsertPos(long data)
{
	// Find the best place to position the item through a binary search
	int Min = 0;
	int Max = GetItemCount();

	// Only do this if there are any items and a sorter function
	if (Max && m_sort_func) {
		// This search will narrow down the best place to position the new
		// item. The result will be the item after that position, which is
		// the format expected by the insertion function.
		do {
			int cur_pos = ( Max - Min ) / 2 + Min;
			int cmp = CompareItems(data, GetItemData(cur_pos));
			
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



int CMuleListCtrl::CompareItems(long item1, long item2)
{
	CSortingList::const_iterator it = m_sort_orders.begin();
	for (; it != m_sort_orders.end(); ++it) {
		int result = m_sort_func(item1, item2, it->first | it->second);
		if (result != 0) {
			return result;
		}
	}

	// Ensure that different items are never considered equal.
	return CmpAny(item1, item2);
}


wxListCtrlCompare	g_sort_func = NULL;
CMuleListCtrl*		g_sort_list = NULL;


int CMuleListCtrl::SortProc(long item1, long item2, long)
{
	const CSortingList& orders = g_sort_list->m_sort_orders;
	
	CSortingList::const_iterator it = orders.begin();
	for (; it != orders.end(); ++it) {
		int result = g_sort_func(item1, item2, it->first | it->second);
		if (result != 0) {
			return result;
		}
	}

	// Ensure that different items are never considered equal.
	return CmpAny(item1, item2);
}


void CMuleListCtrl::SortList()
{
	wxCHECK_RET(g_sort_func == NULL, wxT("Sort-function already set"));
	wxCHECK_RET(g_sort_list == NULL, wxT("Sort-list already set"));
	
	if (m_sort_func and GetColumnCount()) {
		// Positions are likely to be invalid after sorting.
		ResetTTS();
		
		g_sort_func = m_sort_func;
		g_sort_list = this;
		
		SortItems(SortProc, 0);

		g_sort_func = NULL;
		g_sort_list = NULL;
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
	} else if (evt.GetColumn() == -1) {
		// This happens if a user clicks past the last column header.
  		return;
	}

	// Get the currently focused item
	long pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	long item = -1;
	if (pos != -1) {
		item = GetItemData(pos);
	}


	unsigned sort_order = 0;
	if (m_sort_orders.front().first == (unsigned)evt.GetColumn()) {
		// Same column as before, flip the sort-order
		sort_order = m_sort_orders.front().second;
		
		if (sort_order & SORT_DES) {
			if (AltSortAllowed(evt.GetColumn())) {
				sort_order = (~sort_order) & SORT_ALT;
			} else {
				sort_order = 0;
			}
		} else {
			sort_order = SORT_DES | (sort_order & SORT_ALT);
		}

		m_sort_orders.pop_front();
	} else {
		// Check if the column has already been set
		CSortingList::iterator it = m_sort_orders.begin();
		for (; it != m_sort_orders.end(); ++it) {
			if ((unsigned)evt.GetColumn() == it->first) {
				sort_order = it->second;
				break;
			}
		}	
	}
	
	SetSorting(evt.GetColumn(), sort_order);
	
	
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
	return m_sort_orders.front().first;
}


unsigned CMuleListCtrl::GetSortOrder() const
{
	return m_sort_orders.front().second;
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
	
	SetColumnImage(m_sort_orders.front().first, -1);
	
	CSortingList::iterator it = m_sort_orders.begin();
	for (; it != m_sort_orders.end(); ++it) {
		if (it->first == column) {
			m_sort_orders.erase(it);
			break;
		}
	}

	m_sort_orders.push_front(CColPair(column, order));	
	
	if (order & SORT_DES) {
		SetColumnImage(column, (order & SORT_ALT) ? 2 : 0);
	} else {
		SetColumnImage(column, (order & SORT_ALT) ? 3 : 1);
	}

	SortList();
}


bool CMuleListCtrl::IsItemSorted(long item)
{
	wxCHECK_MSG(m_sort_func, true, wxT("No sort function specified!"));
	wxCHECK_MSG((item >= 0) and (item < GetItemCount()), true, wxT("Invalid item"));
	
	bool sorted = true;
	long data = GetItemData(item);

	// Check that the item before the current item is smaller (or equal)
	if (item > 0) {
		sorted &= (CompareItems(GetItemData(item - 1), data) <= 0);
	}

	// Check that the item after the current item is greater (or equal)
	if (sorted and (item < GetItemCount() - 1)) {
		sorted &= (CompareItems(GetItemData(item + 1), data) >= 0);
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

	// May happen if the subclass does not forward deletion events.
	if (m_tts_item >= GetItemCount()) {
		wxASSERT(0);
		m_tts_item = -1;
	}
	
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


void CMuleListCtrl::OnItemDeleted(wxListEvent& evt)
{	
	if (evt.GetIndex() <= m_tts_item) {
		m_tts_item--;
	}

	evt.Skip();
}


void CMuleListCtrl::OnAllItemsDeleted(wxListEvent& evt)
{
	ResetTTS();
	
	evt.Skip();
}


void CMuleListCtrl::ResetTTS()
{
	m_tts_item = -1;
	m_tts_time =  0;
}
