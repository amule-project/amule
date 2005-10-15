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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/menu.h>		// Needed for wxMenu
#if wxCHECK_VERSION(2,4,2)
	#include <wx/config.h>	// Needed for wxConfig in wx-2.4.2
#endif
#include <wx/fileconf.h>	// Needed for wxConfig
#include <wx/tokenzr.h>		// Needed for wxStringTokenizer
#include <wx/imaglist.h>

#include "MuleListCtrl.h"	// Interface declarations
#include "StringFunctions.h"	// Needed for StrToLong
#include "OPCodes.h"		// Needed for MP_LISTCOL_1

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
	EVT_MENU_RANGE(MP_LISTCOL_1, MP_LISTCOL_15, CMuleListCtrl::OnMenuSelected)
	EVT_MOUSEWHEEL(CMuleListCtrl::OnMouseWheel)
END_EVENT_TABLE()


CMuleListCtrl::CMuleListCtrl( wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: MuleExtern::wxGenericListCtrl( parent, winid, pos, size, style, validator, name )
{
	m_sort_func = NULL;
	m_sort_asc 	= true;
	m_sort_alt	= false;
	m_sort_column = 0;

	wxImageListType* imglist = new wxImageListType( 16, 16 );
	imglist->Add( wxBitmap(sort_dn_xpm) );
	imglist->Add( wxBitmap(sort_up_xpm) );
	imglist->Add( wxBitmap(sort_dnx2_xpm) );
	imglist->Add( wxBitmap(sort_upx2_xpm) );
	
	SetImageList( imglist, wxIMAGE_LIST_SMALL );
}


CMuleListCtrl::~CMuleListCtrl()
{
	// If the user specified a name for the list, then its options will be
	// saved upon destruction of the listctrl.
	SaveSettings();

	// Delete the arrow-image list
	delete GetImageList( wxIMAGE_LIST_SMALL );
}


void CMuleListCtrl::SaveSettings()
{
	// Dont save tables with no specified name
	wxString name = GetTableName();
	if ( name.IsEmpty() )
		return;
		

	wxConfigBase* cfg = wxConfigBase::Get();

	// Sort order
	long sort_order = 0;
	if ( GetSortAlt() ) {
		sort_order = ( GetSortAsc() ? 2 : 3 );
	} else {
		sort_order = ( GetSortAsc() ? 0 : 1 );
	}
	
	// Save the sort order
	cfg->Write( wxT("/eMule/TableSortAscending") + name, sort_order );

	// Save the sort column
	cfg->Write( wxT("/eMule/TableSortItem") + name, GetSortColumn() );
	
	// Save column widths. ATM this is also used to signify hidden columns.
	wxString buffer;
	for ( int i = 0; i < GetColumnCount(); ++i ) {
		if ( i ) buffer << wxT(",");

		buffer << GetColumnWidth(i);
	}

	cfg->Write( wxT("/eMule/TableWidths") + name, buffer );
}	


void CMuleListCtrl::LoadSettings()
{
	// Dont save tables with no specified name
	wxString name = GetTableName();
	if ( name.IsEmpty() )
		return;
		

	wxConfigBase* cfg = wxConfigBase::Get();

	// Set the sort-column (defaults to the first row)
	cfg->Read( wxT("/eMule/TableSortItem") + name, &m_sort_column, 0l );

	// Sainity check for sort-by-column value
	if (m_sort_column < 0 || m_sort_column >= GetColumnCount()) {
		wxASSERT(GetColumnCount() == 0);
		m_sort_column = 0;
	}	
	
	// Default to non-alt ascending
	long sort_order = 0;
	// Get the sort-order (defaults to ascending)
	cfg->Read( wxT("/eMule/TableSortAscending") + name, &sort_order, 0l );

	// Sainity check, defaulting to non-alt, ascending
	if ( ( sort_order < 0 ) || ( sort_order > 4 ) )
		sort_order = 0;

	// Figure out the sort order
	switch ( sort_order ) {
		// Non-alternative sort
		case 0:
		case 1:
			m_sort_asc = ( sort_order == 0 );
			m_sort_alt = false;
			break;

		// Alternative sort
		case 2:
		case 3:
			m_sort_asc = ( sort_order == 2 );
			m_sort_alt = AltSortAllowed( GetSortColumn() );
			break;
	}

	// Set the column widts
	wxString buffer;
	if ( cfg->Read( wxT("/eMule/TableWidths") + name, &buffer, wxEmptyString ) ) {
		int counter = 0;
		
		wxStringTokenizer tokenizer( buffer, wxT(",") );
		while ( tokenizer.HasMoreTokens() && ( counter < GetColumnCount() ) ) {
			SetColumnWidth( counter++, StrToLong( tokenizer.GetNextToken() ) );
		}
	}
	
	if (GetColumnCount()) {
		// Update the sort-arrow
		SetSortColumn(m_sort_column);
	
		// Resort the list after the new settings
		SortList();
	}
}


long CMuleListCtrl::GetInsertPos( long data )
{
	// Get the sort-function pointer
	wxListCtrlCompare compare = GetSortFunc();

	// Find the best place to position the item through a binary search
	int Min = 0;
	int Max = GetItemCount();

	// Only do this if there are any items and a sorter function
	if ( Max && compare ) {
		// Define our sort method
		int sortby = GetSortColumn();
		
		// Ascending or decending
		if ( !GetSortAsc() )
			sortby += SORT_OFFSET_DEC;

		if ( GetSortAlt() )
			sortby += SORT_OFFSET_ALT_ASC;

		// This search will narrow down the best place to position the new
		// item. The result will be the item after that position, which is
		// the format expected by the insertion function.
		do {
			int cur_pos = ( Max - Min ) / 2 + Min;
	
			int cmp = compare( data, GetItemData( cur_pos ), sortby );
			
			// Value is lesser than the one at the current pos
			if ( cmp < 0 )
				Max = cur_pos;
			else
				Min = cur_pos + 1;
			
		} while ( ( Min != Max ) );
	}

	return Max;
}


void CMuleListCtrl::SortList()
{
	// Stop if no sort-function has been defined
	if ( !GetSortFunc() )
		return;


	// Some sainity checking
	bool sort_alt = GetSortAlt() && AltSortAllowed( GetSortColumn() );
	
	int sortby = GetSortColumn();

	// Decending sort?
	if ( !GetSortAsc() ) 
		sortby += SORT_OFFSET_DEC;

	// Alternative sort?
	if ( sort_alt )
		sortby += SORT_OFFSET_ALT_ASC;

	
	SortItems( GetSortFunc(), sortby );
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
	wxMenu* menu = new wxMenu;
	wxListItem item;
	
	for ( int i = 0; i < GetColumnCount() && i < 15; ++i) {
		GetColumn(i, item);

		//
		menu->AppendCheckItem(i + MP_LISTCOL_1, item.GetText() );
		menu->Check( i + MP_LISTCOL_1, GetColumnWidth(i) > COL_SIZE_MIN );
	}

	PopupMenu( menu, evt.GetPoint() );

	delete menu;
}


void CMuleListCtrl::OnMenuSelected( wxCommandEvent& evt )
{
	int col = evt.GetId() - MP_LISTCOL_1;

	if ( GetColumnWidth( col ) > COL_SIZE_MIN ) {
		SetColumnWidth( col, 0 );
	} else {
		SetColumnWidth( col, wxLIST_AUTOSIZE );
	}	
}


void CMuleListCtrl::OnColumnLClick(wxListEvent& evt)
{
	// Stop if no sorter-function has been defined
	if ( !GetSortFunc() )
		return;

	// Get the currently focused item
	long pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );
	long item = -1;
	if (pos != -1) {
		item = GetItemData(pos);
	}
	
	// If the user clicked on the same column, then revert the order, otherwise sort ascending.
	if ( evt.GetColumn() == GetSortColumn() ) {
		// Is alternate sort used?
		if ( AltSortAllowed( evt.GetColumn() ) ) 
			// Ascending is just flipped, decending changes search-type
			if ( !GetSortAsc() )
				SetSortAlt( !GetSortAlt() );
			
		SetSortAsc( !GetSortAsc() );
	} else {
		SetSortAsc( true );	
		SetSortAlt( false );
		SetSortColumn( evt.GetColumn() );
	}

	// The sortlist function does the acutal work
	SortList();
	
	// Set focus on item if any was focused
	if (item != -1) {
		long it_pos = FindItem(-1,item);
		if (it_pos != -1) {
			SetItemState(it_pos,wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED & wxLIST_STATE_SELECTED);
		}
	}
}


bool CMuleListCtrl::IsOffsetDec( long offset )
{
	return ( ( offset >= SORT_OFFSET_DEC ) && ( offset < SORT_OFFSET_ALT_ASC ) ) || ( offset >= SORT_OFFSET_ALT_DEC );
}

	
void CMuleListCtrl::SetTableName( const wxString& name )
{
	m_name = name;
}


const wxString& CMuleListCtrl::GetTableName()
{
	return m_name;
}


void CMuleListCtrl::SetSortFunc(wxListCtrlCompare func)
{
	m_sort_func = func;
}


wxListCtrlCompare CMuleListCtrl::GetSortFunc()
{
	return m_sort_func;
}


bool CMuleListCtrl::GetSortAsc()
{
	return m_sort_asc;
}


void CMuleListCtrl::SetSortAsc( bool value )
{
	m_sort_asc = value;
	
	SetSortColumn( GetSortColumn() );
}


bool CMuleListCtrl::AltSortAllowed( int WXUNUSED(column) )
{
	return false;
}


bool CMuleListCtrl::GetSortAlt()
{
	return m_sort_alt;
}


void CMuleListCtrl::SetSortAlt( bool value )
{
	m_sort_alt = value;
	
	SetSortColumn( GetSortColumn() );
}


int CMuleListCtrl::GetSortColumn()
{
	return m_sort_column;
}


void CMuleListCtrl::SetSortColumn( int column )
{
	// Unset old
	SetColumnImage( m_sort_column, -1 );
	
	m_sort_column = column;
	
	// Set new
	if ( GetSortAlt() ) {
		SetColumnImage( m_sort_column, ( GetSortAsc() ? 3 : 2 ) );
	} else {
		SetColumnImage( m_sort_column, ( GetSortAsc() ? 1 : 0 ) );
	}
}

/**
 * This enables scrolling with the mouse wheel
 */
void CMuleListCtrl::OnMouseWheel(wxMouseEvent &event)
{
	event.Skip();
}


void CMuleListCtrl::SetColumnImage(int col, int image)
{
    wxListItem item;
    item.SetMask(wxLIST_MASK_IMAGE);
    item.SetImage(image);
    SetColumn(col, item);
}

long CMuleListCtrl::CheckSelection(wxMouseEvent &event)
{
	int flags = 0;
	long item_hit = HitTest( event.GetPosition(), flags );
	// Check if clicked item is selected. If not, unselect all and select it.
	if ( (item_hit != -1) && !GetItemState(item_hit, wxLIST_STATE_SELECTED) ) {
		long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		while ( index != -1 ) {
			SetItemState( index, 0, wxLIST_STATE_SELECTED );

			index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}

		SetItemState(item_hit, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
	return item_hit;
}

