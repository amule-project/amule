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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "SearchListCtrl.h"	// Interface declarations
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "OtherFunctions.h"	// Needed for CastItoXBytes
#include "PartFile.h"		// Needed for CPartFile and CKnownFile
#include "SearchList.h"		// Needed for CSearchFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "OPCodes.h"		// Needed for MP_ASSIGNCAT
#include "amule.h"			// Needed for theApp
#include "Color.h"			// Needed for BLEND and SYSCOLOR
#include "muuli_wdr.h"		// Needed for clientImages
#include "Preferences.h"	// Needed for thePrefs

#include <wx/menu.h>
#include <wx/settings.h>
#include <algorithm>


// For arrow-pixmaps
#include "pixmaps/sort_dn.xpm"
#include "pixmaps/sort_up.xpm"
#include "pixmaps/sort_dnx2.xpm"
#include "pixmaps/sort_upx2.xpm"



BEGIN_EVENT_TABLE(CSearchListCtrl, CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK(-1, CSearchListCtrl::OnRightClick)
	EVT_LIST_COL_CLICK( -1,       CSearchListCtrl::OnColumnLClick)
	EVT_LIST_COL_END_DRAG( -1,    CSearchListCtrl::OnColumnResize)

	EVT_MENU( MP_GETED2KLINK,     CSearchListCtrl::OnPopupGetUrl)
	EVT_MENU( MP_RAZORSTATS,      CSearchListCtrl::OnRazorStatsCheck)
	EVT_MENU( MP_RESUME,          CSearchListCtrl::OnPopupDownload)
	EVT_MENU_RANGE( MP_ASSIGNCAT, MP_ASSIGNCAT + 99, CSearchListCtrl::OnPopupDownload )

	EVT_LIST_ITEM_ACTIVATED( -1,  CSearchListCtrl::OnItemActivated)
END_EVENT_TABLE()

//! Shared list of arrow-pixmaps + 
static wxImageListType s_imgList(16, 16, true, 0);
//! The index of the first rating icon
static int s_ratingOffset = 0;


std::list<CSearchListCtrl*> CSearchListCtrl::s_lists;


enum SearchListColumns {
	ID_SEARCH_COL_NAME = 0,
	ID_SEARCH_COL_SIZE,
	ID_SEARCH_COL_SOURCES,
	ID_SEARCH_COL_TYPE,
	ID_SEARCH_COL_FILEID
};


CSearchListCtrl::CSearchListCtrl( wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name )
	: CMuleListCtrl( parent, winid, pos, size, style, validator, name),
	  m_filterKnown(false),
	  m_invert(false),
	  m_filterEnabled(false)
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	InsertColumn( ID_SEARCH_COL_NAME,    _("File Name"), wxLIST_FORMAT_LEFT, 500);
	InsertColumn( ID_SEARCH_COL_SIZE,    _("Size"),      wxLIST_FORMAT_LEFT, 100);
	InsertColumn( ID_SEARCH_COL_SOURCES, _("Sources"),   wxLIST_FORMAT_LEFT, 50);
	InsertColumn( ID_SEARCH_COL_TYPE,    _("Type"),      wxLIST_FORMAT_LEFT, 65);
	InsertColumn( ID_SEARCH_COL_FILEID,  _("FileID"),    wxLIST_FORMAT_LEFT, 280);

	m_nResultsID = 0;

	// Only load settings for first list, otherwise sync with current lists
	if ( s_lists.empty() ) {
		// Set the name to enable loading of settings
		SetTableName( wxT("Search") );
	
		LoadSettings();

		// Unset the name to avoid the settings getting saved every time a list is closed
		SetTableName( wxEmptyString );
	} else {
		// Sync this list with one of the others
		SyncLists( s_lists.front(), this );
	}

	// Add the list so that it will be synced with the other lists
	s_lists.push_back( this );

	
	if (s_imgList.GetImageCount() == 0) {
		s_imgList.Add(wxBitmap(sort_dn_xpm));
		s_imgList.Add(wxBitmap(sort_up_xpm));
		s_imgList.Add(wxBitmap(sort_dnx2_xpm));
		s_imgList.Add(wxBitmap(sort_upx2_xpm));
		
		// Add rating icons
		s_ratingOffset = s_imgList.Add(wxBitmap(clientImages(Client_InvalidRating_Smiley)));
		s_imgList.Add(wxBitmap(clientImages(Client_PoorRating_Smiley)));
		s_imgList.Add(wxBitmap(clientImages(Client_GoodRating_Smiley)));
		s_imgList.Add(wxBitmap(clientImages(Client_FairRating_Smiley)));
		s_imgList.Add(wxBitmap(clientImages(Client_ExcellentRating_Smiley)));		
	}

	SetImageList(&s_imgList, wxIMAGE_LIST_SMALL);
}


CSearchListCtrl::~CSearchListCtrl()
{
	std::list<CSearchListCtrl*>::iterator it = std::find( s_lists.begin(), s_lists.end(), this );

	if ( it != s_lists.end() )
		s_lists.erase( it );

	// We only save the settings if the last list was closed
	if ( s_lists.empty() ) {
		// In order to get the settings saved, we need to set the name
		SetTableName( wxT("Search") );
	}
}


void CSearchListCtrl::AddResult(CSearchFile* toshow)
{
	// Ensure that only the right results gets added
	if ( toshow->GetSearchID() != m_nResultsID )
		return;

	// Check if the result should be shown
	if (!IsFiltered(toshow)) {
		m_filteredOut.push_back(toshow);
		
		return;
	}

	// Rating
	int image = -1;
	if (toshow->HasRating()) {
		image = s_ratingOffset + toshow->UserRating() - 1;
	}
	
	// Insert the item before the item found by the search
	uint32 newid = InsertItem( GetInsertPos( (long)toshow ), toshow->GetFileName(), image );

	SetItemData( newid, (long)toshow );

	// Filesize
	SetItem(newid, ID_SEARCH_COL_SIZE, CastItoXBytes( toshow->GetFileSize() ) );

	// Source count
	wxString temp = wxString::Format( wxT("%d (%d)"), toshow->GetSourceCount(), toshow->GetCompleteSourceCount() );
	SetItem( newid, ID_SEARCH_COL_SOURCES, temp );

	// File-type
	SetItem( newid, ID_SEARCH_COL_TYPE, GetFiletypeByName( toshow->GetFileName() ) );

	// File-hash
	SetItem(newid, ID_SEARCH_COL_FILEID, toshow->GetFileHash().Encode() );

	// Set the color of the item
	UpdateItemColor( newid );
}


void CSearchListCtrl::UpdateResult(CSearchFile* toupdate)
{
	long index = FindItem(-1, (long)toupdate);
	if (index != -1) {
		wxString temp = wxString::Format( wxT("%d (%d)"), toupdate->GetSourceCount(), toupdate->GetCompleteSourceCount());
		SetItem(index, ID_SEARCH_COL_SOURCES, temp);
		
		UpdateItemColor(index);
		
		// Deletions of items causes rather large ammount of flicker, so to
		// avoid this, we resort the list to ensure correct ordering.
		if (!IsItemSorted(index)) {
			SortList();
		}
	}
}


void CSearchListCtrl::UpdateItemColor( long index )
{
	wxListItem item;
	item.SetId( index );
	item.SetColumn( ID_SEARCH_COL_SIZE );
	item.SetMask( wxLIST_MASK_STATE|wxLIST_MASK_TEXT|wxLIST_MASK_IMAGE|wxLIST_MASK_DATA|wxLIST_MASK_WIDTH|wxLIST_MASK_FORMAT );

	if ( GetItem(item) ) {
		wxColour newcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

		CSearchFile* file = (CSearchFile*)GetItemData(index);

		CKnownFile* sameFile = theApp.downloadqueue->GetFileByID(file->GetFileHash());
		if ( !sameFile ) {
			sameFile = theApp.knownfiles->FindKnownFileByID(file->GetFileHash());
		}

		int red		= newcol.Red();
		int green	= newcol.Green();
		int blue	= newcol.Blue();

		if ( sameFile ) {
			if ( sameFile->IsPartFile() ) {
				// File is already being downloaded. Marks as red.
				red = 255;
			} else {
				// File has already been downloaded. Mark as green.
				green = 200;
			}
		} else {
			// File is new, colour after number of files
			blue += file->GetSourceCount() * 5;
			if ( blue > 255 ) {
				blue = 255;
			}
		}

		// don't forget to set the item data back...
		wxListItem newitem;
		newitem.SetId( index );
		newitem.SetTextColour( wxColour( red, green, blue ) );
		newitem.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
		SetItem( newitem );	
	}
}


void CSearchListCtrl::ShowResults( long ResultsID )
{
	DeleteAllItems();
	m_nResultsID = ResultsID;

	if ( ResultsID ) {
		CSearchList::ResultMap::iterator it = theApp.searchlist->m_Results.find( m_nResultsID );

		if ( it != theApp.searchlist->m_Results.end() ) {
			CSearchList::SearchList& list = it->second;

			for ( unsigned int i = 0; i < list.size(); i++ ) {
				AddResult( list[i] );
			}
		}
	}
}


long CSearchListCtrl::GetSearchId()
{
	return m_nResultsID;
}


void CSearchListCtrl::SetFilter(const wxString& regExp, bool invert, bool filterKnown)
{
	if (regExp.IsEmpty()) {
		// Show everything
		m_filterText = wxT(".*");
	} else {
		m_filterText = regExp;
	}
	
	m_filter.Compile(m_filterText, wxRE_DEFAULT | wxRE_ICASE);
	m_filterKnown = filterKnown;
	m_invert = invert;
	
	if (m_filterEnabled) {
		// Swap the list of filtered results so we can freely add new items to the list
		ResultList curFiltered;
		std::swap(curFiltered, m_filteredOut);
	
		// Filter items already on the list
		for (int i = 0; i < GetItemCount();) {
			CSearchFile* file = (CSearchFile*)GetItemData(i);
		
			if (IsFiltered(file)) {
				++i;
			} else {
				m_filteredOut.push_back(file);
				DeleteItem(i);
			}
		}

		// Check the previously filtered items.
		ResultList::iterator it = curFiltered.begin();
		for (; it != curFiltered.end(); ++it) {
			if (IsFiltered(*it)) {
				AddResult(*it);
			} else {
				m_filteredOut.push_back(*it);
			}
		}
	}
}


void CSearchListCtrl::EnableFiltering(bool enabled)
{
	if (enabled != m_filterEnabled) {
		m_filterEnabled = enabled;
		
		if (enabled) {
			SetFilter(m_filterText, m_invert, m_filterKnown);
		} else {
			ResultList::iterator it = m_filteredOut.begin();
			for (; it != m_filteredOut.end(); ++it) {
				AddResult(*it);
			}

			m_filteredOut.clear();
		}
	}
}


size_t CSearchListCtrl::GetHiddenItemCount() const
{
	return m_filteredOut.size();
}


bool CSearchListCtrl::IsFiltered(const CSearchFile* file)
{
	// By default, everything is displayed
	bool result = true;
	
	if (m_filterEnabled && m_filter.IsValid()) {
		result = m_filter.Matches(file->GetFileName());
		result = ((result && !m_invert) || (!result && m_invert));
	
		if (result && m_filterKnown) {
			result = !theApp.downloadqueue->GetFileByID(file->GetFileHash());

			if (result) {
				result = !theApp.knownfiles->FindKnownFileByID(file->GetFileHash());
			}
		}
	}

	return result;
}


int CSearchListCtrl::SortProc(long item1, long item2, long sortData)
{
	CSearchFile* file1 = (CSearchFile*)item1;
	CSearchFile* file2 = (CSearchFile*)item2;

	// Modifies the result, 1 for ascending, -1 for decending
	int modifier = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	bool alternate = (sortData & CMuleListCtrl::SORT_ALT);

	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
		// Sort by filename
		case ID_SEARCH_COL_NAME:
			return modifier * file1->GetFileName().CmpNoCase( file2->GetFileName() );

		// Sort file-size
		case ID_SEARCH_COL_SIZE:
			return modifier * CmpAny( file1->GetFileSize(), file2->GetFileSize() );

		// Sort by sources
		case ID_SEARCH_COL_SOURCES: {
			
			int cmp = CmpAny( file1->GetSourceCount(), file2->GetSourceCount() );

			int cmp2 = CmpAny( file1->GetCompleteSourceCount(), file2->GetCompleteSourceCount() );

			if ( alternate ) {
				// Swap criterias
				int temp = cmp2;
				cmp2 = cmp;
				cmp = temp;
			}

			if ( cmp == 0 ) {
				cmp = cmp2;
			}

			return modifier * cmp;
		}
		
		// Sort by file-types
		case ID_SEARCH_COL_TYPE: {
			wxString fileName1 = wxT(".") + file1->GetFileName().AfterLast('.').MakeLower();
			wxString fileName2 = wxT(".") + file2->GetFileName().AfterLast('.').MakeLower();
			
			int result = GetFiletypeByName(fileName1).Cmp(GetFiletypeByName(fileName2));
			if (result == 0) {
				// Same file-type, sort by extension
				result = fileName1.Cmp(fileName2);
			}

			return modifier * result;
		}

		// Sort by file-hash
		case ID_SEARCH_COL_FILEID:
			return modifier * file2->GetFileHash().Encode().Cmp( file1->GetFileHash().Encode() );

		default:
			return 0;
	}
}


void CSearchListCtrl::SyncLists( CSearchListCtrl* src, CSearchListCtrl* dst )
{
	wxASSERT( src );
	wxASSERT( dst );

	// Column widths
	for ( int i = 0; i < src->GetColumnCount(); i++ ) {
		// We do this check since just setting the width causes a redraw
		if ( dst->GetColumnWidth( i ) != src->GetColumnWidth( i ) ) {
			dst->SetColumnWidth( i, src->GetColumnWidth( i ) );
		}
	}

	// Sync sorting
	unsigned column = src->GetSortColumn();
	unsigned order  = src->GetSortOrder();
	if (column != dst->GetSortColumn() || order != dst->GetSortOrder()) {
		dst->SetSorting(column, order);
		dst->SortList();
	}
}


void CSearchListCtrl::SyncOtherLists( CSearchListCtrl* src )
{
	std::list<CSearchListCtrl*>::iterator it;

	for ( it = s_lists.begin(); it != s_lists.end(); ++it ) {
		if ( (*it) != src ) {
			SyncLists( src, *it );
		}
	}
}


void CSearchListCtrl::OnRightClick(wxListEvent& event)
{
	CheckSelection(event);
	
	if (GetSelectedItemCount()) {
		// Create the popup-menu
		wxMenu menu(_("File"));
		menu.Append(MP_RESUME, _("Download"));
		
		wxMenu* cats = new wxMenu(_("Category"));
		cats->Append(MP_ASSIGNCAT, _("Main"));
		for (unsigned i = 1; i < theApp.glob_prefs->GetCatCount(); i++) {
			cats->Append(MP_ASSIGNCAT + i,
				theApp.glob_prefs->GetCategory(i)->title);
		}
		
		menu.Append(MP_MENU_CATS, _("Download in category"), cats);
		menu.AppendSeparator();
		menu.Append(MP_RAZORSTATS, _("Get Razorback 2's stats for this file"));
		menu.AppendSeparator();
		menu.Append(MP_GETED2KLINK, _("Copy ED2k link to clipboard"));

		// These should only be enabled for single-selections
		bool enable = GetSelectedItemCount();
		menu.Enable(MP_GETED2KLINK, enable);
		menu.Enable(MP_MENU_CATS, (theApp.glob_prefs->GetCatCount() > 1));

		PopupMenu(&menu, event.GetPoint());
	} else {
		event.Skip();
	}
}


void CSearchListCtrl::OnColumnLClick( wxListEvent& event )
{
	// Let the real event handler do its work first
	CMuleListCtrl::OnColumnLClick( event );
	
	SyncOtherLists( this );
}


void CSearchListCtrl::OnColumnResize( wxListEvent& WXUNUSED(event) )
{
	SyncOtherLists( this );
}


void CSearchListCtrl::OnPopupGetUrl( wxCommandEvent& event )
{
	wxString URIs;	
	
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CSearchFile* file = (CSearchFile*)GetItemData( index );

		URIs += theApp.CreateED2kLink( file ) + wxT("\n");

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	if ( !URIs.IsEmpty() ) {
		theApp.CopyTextToClipboard( URIs.RemoveLast() );
	}
}


void CSearchListCtrl::OnRazorStatsCheck( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item == -1 ) {
		return;
	}

	CSearchFile* file = (CSearchFile*)GetItemData( item );
	theApp.amuledlg->LaunchUrl(wxT("http://stats.razorback2.com/ed2khistory?ed2k=") + file->GetFileHash().Encode());
}


void CSearchListCtrl::OnPopupDownload(wxCommandEvent& event)
{
	if (event.GetId() == MP_RESUME) {
		// Via the "Download" menu-item, use category specified in drop-down menu
		DownloadSelected();
	} else {
		// Via an "Download in category" item
		DownloadSelected(event.GetId() - MP_ASSIGNCAT);
	}
}


void CSearchListCtrl::OnItemActivated( wxListEvent& WXUNUSED(event) )
{
	DownloadSelected();
}


bool CSearchListCtrl::AltSortAllowed(unsigned column) const
{
	switch ( column ) {
		case ID_SEARCH_COL_SOURCES:
			return true;
		
		default:
			return false;
	}
}


void CSearchListCtrl::DownloadSelected(int category)
{
	FindWindowById(IDC_SDOWNLOAD)->Enable(FALSE);

	// Either the "Download" menu-item, the download-button, double-click or enter
	if (category == -1) {
		// Defaults to main category
		category = 0;
		
		if (CastByID(IDC_EXTENDEDSEARCHCHECK, NULL, wxCheckBox)->GetValue()) {
			category = CastByID(ID_AUTOCATASSIGN, NULL, wxChoice)->GetSelection();
		}		
	}
	
	long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (index > -1) {
		CoreNotify_Search_Add_Download((CSearchFile*)GetItemData(index), category);
		
		UpdateItemColor(index);

		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}

}

