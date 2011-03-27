//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/menu.h>			// Needed for wxMenu
#include <wx/fileconf.h>		// Needed for wxConfig
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/imaglist.h>		// Needed for wxImageList

#include <common/MuleDebug.h>			// Needed for MULE_VALIDATE_
#include <common/StringFunctions.h>		// Needed for StrToLong

#include <common/MenuIDs.h>

#include "MuleListCtrl.h"		// Interface declarations
#include "GetTickCount.h"		// Needed for GetTickCount()
#include "OtherFunctions.h"


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
	EVT_LIST_ITEM_DESELECTED(-1,	CMuleListCtrl::OnItemSelected)
	EVT_LIST_DELETE_ITEM(-1,		CMuleListCtrl::OnItemDeleted)
	EVT_LIST_DELETE_ALL_ITEMS(-1,	CMuleListCtrl::OnAllItemsDeleted)
	EVT_CHAR(						CMuleListCtrl::OnChar)
	EVT_MENU_RANGE(MP_LISTCOL_1, MP_LISTCOL_15, CMuleListCtrl::OnMenuSelected)
	EVT_MOUSEWHEEL(CMuleListCtrl::OnMouseWheel)
END_EVENT_TABLE()


//! Shared list of arrow-pixmaps
static wxImageList imgList(16, 16, true, 0);


CMuleListCtrl::CMuleListCtrl(wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: MuleExtern::wxGenericListCtrl(parent, winid, pos, size, style, validator, name)
{
	m_sort_func = NULL;
	m_tts_time = 0;
	m_tts_item = -1;
	m_isSorting = false;

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
	if (!m_name.IsEmpty()) {
		SaveSettings();
	}
}

long CMuleListCtrl::InsertColumn(long col, const wxString& heading, int format, int width, const wxString& name)
{
	if (!name.IsEmpty()) {
#ifdef __DEBUG__
		// Check for valid names
		wxASSERT_MSG(name.Find(wxT(':')) == wxNOT_FOUND, wxT("Column name \"") + name + wxT("\" contains invalid characters!"));
		wxASSERT_MSG(name.Find(wxT(',')) == wxNOT_FOUND, wxT("Column name \"") + name + wxT("\" contains invalid characters!"));

		// Check for uniqueness of names.
		for (ColNameList::const_iterator it = m_column_names.begin(); it != m_column_names.end(); ++it) {
			if (name == it->name) {
				wxFAIL_MSG(wxT("Column name \"") + name + wxT("\" is not unique!"));
			}
		}
#endif
		// Insert name at position col.
		ColNameList::iterator it = m_column_names.begin();
		while (it != m_column_names.end() && it->index < col) {
			++it;
		}
		m_column_names.insert(it, ColNameEntry(col, width, name));
		while (it != m_column_names.end()) {
			++it;
			++(it->index);
		}
	}

	return MuleExtern::wxGenericListCtrl::InsertColumn(col, heading, format, width);
}

void CMuleListCtrl::SaveSettings()
{
	wxCHECK_RET(!m_name.IsEmpty(), wxT("Cannot save settings for unnamed list"));
	
	wxConfigBase* cfg = wxConfigBase::Get();

	// Save sorting, column and order
	wxString sortOrder;
	for (CSortingList::iterator it = m_sort_orders.begin(); it != m_sort_orders.end();) {
		wxString columnName = GetColumnName(it->first);
		if (!columnName.IsEmpty()) {
			sortOrder += columnName;
			sortOrder += wxT(":");
			sortOrder += it->second & SORT_DES ? wxT("1") : wxT("0");
			sortOrder += wxT(":");
			sortOrder += it->second & SORT_ALT ? wxT("1") : wxT("0");
			if (++it != m_sort_orders.end()) {
				sortOrder += wxT(",");
			}
		} else {
			++it;
		}
	}
	
	cfg->Write(wxT("/eMule/TableOrdering") + m_name, sortOrder);

	// Save column widths. ATM this is also used to signify hidden columns.
	wxString buffer;
	for (int i = 0; i < GetColumnCount(); ++i) {
		wxString columnName = GetColumnName(i);
		if (!columnName.IsEmpty()) {
			if (!buffer.IsEmpty()) {
				buffer << wxT(",");
			}
			int currentwidth = GetColumnWidth(i);
			int savedsize = (m_column_sizes.size() && (i < (int) m_column_sizes.size())) ? m_column_sizes[i] : 0;
			buffer << columnName << wxT(":") << ((currentwidth > 0) ? currentwidth : (-1 * savedsize));
		}
	}

	cfg->Write(wxT("/eMule/TableWidths") + m_name, buffer);
}	

void CMuleListCtrl::ParseOldConfigEntries(const wxString& sortOrders, const wxString& columnWidths)
{
	// Set sort order (including sort column)
	wxStringTokenizer tokens(sortOrders, wxT(","));
	while (tokens.HasMoreTokens()) {
		wxString token = tokens.GetNextToken();

		long column = 0;
		unsigned long order = 0;

		if (token.BeforeFirst(wxT(' ')).Strip(wxString::both).ToLong(&column)) {
			if (token.AfterFirst(wxT(' ')).Strip(wxString::both).ToULong(&order)) {
				column = GetNewColumnIndex(column);
				// Sanity checking, to avoid asserting if column count changes.
				if (column >= 0 && column < GetColumnCount()) {
					// Sanity checking, to avoid asserting if data-format changes.
					if ((order & ~SORTING_MASK) == 0) {
						// SetSorting will take care of duplicate entries
						SetSorting(column, order);
					}
				}
			}
		}
	}

	// Set column widths
	int counter = 0;
	wxStringTokenizer tokenizer(columnWidths, wxT(","));
	while (tokenizer.HasMoreTokens()) {
		long idx = GetNewColumnIndex(counter++);
		long width = StrToLong(tokenizer.GetNextToken());
		if (idx >= 0) {
			SetColumnWidth(idx, width);
		}
	}
}

void CMuleListCtrl::LoadSettings()
{
	wxCHECK_RET(!m_name.IsEmpty(), wxT("Cannot load settings for unnamed list"));

	wxConfigBase* cfg = wxConfigBase::Get();

	// Load sort order (including sort-column)
	m_sort_orders.clear();
	wxString sortOrders = cfg->Read(wxT("/eMule/TableOrdering") + m_name, wxEmptyString);
	wxString columnWidths = cfg->Read(wxT("/eMule/TableWidths") + m_name, wxEmptyString);

	// Prevent sorting from occuring when calling SetSorting
	MuleListCtrlCompare sortFunc = m_sort_func;
	m_sort_func = NULL;

	if (columnWidths.Find(wxT(':')) == wxNOT_FOUND) {
		// Old-style config entries...
		ParseOldConfigEntries(sortOrders, columnWidths);
	} else {
		// Sort orders
		wxStringTokenizer tokens(sortOrders, wxT(","));
		// Sort orders are stored in order primary, secondary, ...
		// We want to apply them with SetSorting(), so we have to apply them in reverse order,
		// so that the primary order is applied last and wins.
		// Read them with tokenizer and store them in a list in reverse order.
		CStringList tokenList;
		while (tokens.HasMoreTokens()) {
			tokenList.push_front(tokens.GetNextToken());
		}
		for (CStringList::iterator it = tokenList.begin(); it != tokenList.end(); it++) {
			wxString token = *it;
			wxString name = token.BeforeFirst(wxT(':'));
			long order = StrToLong(token.AfterFirst(wxT(':')).BeforeLast(wxT(':')));
			long alt = StrToLong(token.AfterLast(wxT(':')));
			int col = GetColumnIndex(name);
			if (col >= 0) {
				SetSorting(col, (order ? SORT_DES : 0) | (alt ? SORT_ALT : 0));
			}
		}

		// Column widths
		wxStringTokenizer tkz(columnWidths, wxT(","));
		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			wxString name = token.BeforeFirst(wxT(':'));
			long width = StrToLong(token.AfterFirst(wxT(':')));
			int col = GetColumnIndex(name);
			if (col >= 0) {
				if (col >= (int) m_column_sizes.size()) {
					m_column_sizes.resize(col + 1, 0);
				}
				m_column_sizes[col] = abs(width);
				SetColumnWidth(col, (width > 0) ? width : 0);
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
}


const wxString& CMuleListCtrl::GetColumnName(int index) const
{
	for (ColNameList::const_iterator it = m_column_names.begin(); it != m_column_names.end(); ++it) {
		if (it->index == index) {
			return it->name;
		}
	}
	return EmptyString;
}

int CMuleListCtrl::GetColumnDefaultWidth(int index) const
{
	for (ColNameList::const_iterator it = m_column_names.begin(); it != m_column_names.end(); ++it) {
		if (it->index == index) {
			return it->defaultWidth;
		}
	}
	return wxLIST_AUTOSIZE;
}

int CMuleListCtrl::GetColumnIndex(const wxString& name) const
{
	for (ColNameList::const_iterator it = m_column_names.begin(); it != m_column_names.end(); ++it) {
		if (it->name == name) {
			return it->index;
		}
	}
	return -1;
}

int CMuleListCtrl::GetNewColumnIndex(int oldindex) const
{
	wxStringTokenizer oldcolumns(GetOldColumnOrder(), wxT(","), wxTOKEN_RET_EMPTY_ALL);

	while (oldcolumns.HasMoreTokens()) {
		wxString name = oldcolumns.GetNextToken();
		if (oldindex == 0) {
			return GetColumnIndex(name);
		}
		--oldindex;
	}
	return -1;
}

long CMuleListCtrl::GetInsertPos(wxUIntPtr data)
{
	// Find the best place to position the item through a binary search
	int Min = 0;
	int Max = GetItemCount();

	// Only do this if there are any items and a sorter function.
	// Otherwise insert at end.
	if (Max && m_sort_func) {
		// This search will find the place to position the new item
		// so it matches current sorting.
		// The result will be the position the new item will have
		// after insertion, which is the format expected by the InsertItem function.
		// If the item equals another item it will be inserted after it.
		do {
			int cur_pos = ( Max - Min ) / 2 + Min;
			int cmp = CompareItems(data, GetItemData(cur_pos));
			
			// Value is less than the one at the current pos
			if ( cmp < 0 ) {
				Max = cur_pos;
			} else {
				Min = cur_pos + 1;
			}			
		} while ((Min != Max));
	}

	return Max;
}


int CMuleListCtrl::CompareItems(wxUIntPtr item1, wxUIntPtr item2)
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

int CMuleListCtrl::SortProc(wxUIntPtr item1, wxUIntPtr item2, long data)
{
	MuleSortData* sortdata = (MuleSortData*) data;
	const CSortingList& orders = sortdata->m_sort_orders;
	
	CSortingList::const_iterator it = orders.begin();
	for (; it != orders.end(); ++it) {
		int result = sortdata->m_sort_func(item1, item2, it->first | it->second);
		if (result != 0) {
			return result;
		}
	}

	// Ensure that different items are never considered equal.
	return CmpAny(item1, item2);
}

void CMuleListCtrl::SortList()
{
	if (!IsSorting() && (m_sort_func && GetColumnCount())) {

		m_isSorting = true;
	
		MuleSortData sortdata(m_sort_orders, m_sort_func);

		// In many cases control already has correct order, and sorting causes nasty flickering.
		// Make one pass through it to check if sorting is necessary at all.
		int nrItems = GetItemCount();
		bool clean = true;
		long lastItemdata = 0;
		if (nrItems > 1) {
			lastItemdata = GetItemData(0);
		}
		for (int i = 1; i < nrItems; i++) {
			long nextItemdata = GetItemData(i);
			if (SortProc(lastItemdata, nextItemdata, (long int)&sortdata) > 0) {
				// ok - we need to sort
				clean = false;
				break;
			}
			lastItemdata = nextItemdata;
		}
		if (clean) {
			// no need to sort
			m_isSorting = false;
			return;
		}

		// Positions are likely to be invalid after sorting.
		ResetTTS();
		
		// Store the current selected items
		ItemDataList selectedItems = GetSelectedItems();
		// Store the focused item
		long pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
		wxUIntPtr focused = (pos == -1) ? 0 : GetItemData(pos);
		
		SortItems(SortProc, (long int)&sortdata);
		
		// Re-select the selected items.
		for (unsigned i = 0; i < selectedItems.size(); ++i) {
			long it_pos = FindItem(-1, selectedItems[i]);
			if (it_pos != -1) {
				SetItemState(it_pos, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			}
		}
		
		// Set focus on item if any was focused
		if (focused) {
			long it_pos = FindItem(-1, focused);
			if (it_pos != -1) {
				SetItemState(it_pos, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
			}
		}

		m_isSorting = false;
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
	unsigned int col = evt.GetId() - MP_LISTCOL_1;

	if (col >= m_column_sizes.size()) {
		m_column_sizes.resize(col + 1, 0);
	}

	if (GetColumnWidth(col) > COL_SIZE_MIN) {
		m_column_sizes[col] = GetColumnWidth(col);
		SetColumnWidth(col, 0);
	} else {
		int oldsize = m_column_sizes[col];
		SetColumnWidth(col, (oldsize > 0) ? oldsize : GetColumnDefaultWidth(col));
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


bool CMuleListCtrl::AltSortAllowed(unsigned WXUNUSED(column)) const
{
	return false;
}


void CMuleListCtrl::SetSorting(unsigned column, unsigned order)
{
	MULE_VALIDATE_PARAMS(column < (unsigned)GetColumnCount(), wxT("Invalid column to sort by."));
	MULE_VALIDATE_PARAMS(!(order & ~SORTING_MASK), wxT("Sorting order contains invalid data."));
	
	if (!m_sort_orders.empty()) {
		SetColumnImage(m_sort_orders.front().first, -1);
	
		CSortingList::iterator it = m_sort_orders.begin();
		for (; it != m_sort_orders.end(); ++it) {
			if (it->first == column) {
				m_sort_orders.erase(it);
				break;
			}
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
	wxCHECK_MSG((item >= 0) && (item < GetItemCount()), true, wxT("Invalid item"));
	
	bool sorted = true;
	wxUIntPtr data = GetItemData(item);

	// Check that the item before the current item is smaller (or equal)
	if (item > 0) {
		sorted &= (CompareItems(GetItemData(item - 1), data) <= 0);
	}

	// Check that the item after the current item is greater (or equal)
	if (sorted && (item < GetItemCount() - 1)) {
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
	int key = evt.GetKeyCode();
	if (key == 0) {
		// We prefer GetKeyCode() to GetUnicodeKey(), in order to work
		// around a bug in the GetUnicodeKey(), that causes values to
		// be returned untranslated. This means for instance, that if
		// shift and '1' is pressed, the result is '1' rather than '!'
		// (as it should be on my keyboard). This has been reported:
		// http://sourceforge.net/tracker/index.php?func=detail&aid=1864810&group_id=9863&atid=109863
		key = evt.GetUnicodeKey();
	} else if (key >= WXK_START) {
		// wxKeycodes are ignored, as they signify events such as the 'home'
		// button. Unicoded chars are not checked as there is an overlap valid
		// chars and the wx keycodes.
		evt.Skip();
		return;
	}
	
	// We wish to avoid handling shortcuts, with the exception of 'select-all'.
	if (evt.AltDown() || evt.ControlDown() || evt.MetaDown()) {
		if (evt.CmdDown() && (evt.GetKeyCode() == 0x01)) {
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
	m_tts_text.Append(wxTolower(key));

	// May happen if the subclass does not forward deletion events.
	// Or rather when single-char-repeated (see below) wraps around, so don't assert.
	if (m_tts_item >= GetItemCount()) {
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
	if (IsSorting()) {
		// Selection/Deselection that happened while sorting.
		evt.Veto();
	} else {
			// We reset the current TTS session if the user manually changes the selection
		if (m_tts_item != evt.GetIndex()) {
			ResetTTS();

			// The item is changed so that the next TTS starts from the selected item.
			m_tts_item = evt.GetIndex();
		}
		evt.Skip();
	}
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

wxString CMuleListCtrl::GetOldColumnOrder() const
{
	return wxEmptyString;
}
// File_checked_for_headers
