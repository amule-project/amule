//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <common/MenuIDs.h>

#include "amule.h"			// Needed for theApp
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "SearchList.h"		// Needed for CSearchFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "muuli_wdr.h"		// Needed for clientImages
#include "Preferences.h"	// Needed for thePrefs
#include "GuiEvents.h"		// Needed for CoreNotify_Search_Add_Download
#include "MuleColour.h"

BEGIN_EVENT_TABLE(CSearchListCtrl, CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK(-1, CSearchListCtrl::OnRightClick)
	EVT_LIST_COL_CLICK( -1,       CSearchListCtrl::OnColumnLClick)
	EVT_LIST_COL_END_DRAG( -1,    CSearchListCtrl::OnColumnResize)

	EVT_MENU( MP_GETED2KLINK,     CSearchListCtrl::OnPopupGetUrl)
	EVT_MENU( MP_RAZORSTATS,      CSearchListCtrl::OnRazorStatsCheck)
	EVT_MENU( MP_SEARCHRELATED,   CSearchListCtrl::OnRelatedSearch)	
	EVT_MENU( MP_MARK_AS_KNOWN,   CSearchListCtrl::OnMarkAsKnown)
	EVT_MENU( MP_RESUME,          CSearchListCtrl::OnPopupDownload)
	EVT_MENU_RANGE( MP_ASSIGNCAT, MP_ASSIGNCAT + 99, CSearchListCtrl::OnPopupDownload )

	EVT_LIST_ITEM_ACTIVATED( -1,  CSearchListCtrl::OnItemActivated)
END_EVENT_TABLE()


std::list<CSearchListCtrl*> CSearchListCtrl::s_lists;


enum SearchListColumns {
	ID_SEARCH_COL_NAME = 0,
	ID_SEARCH_COL_SIZE,
	ID_SEARCH_COL_SOURCES,
	ID_SEARCH_COL_TYPE,
	ID_SEARCH_COL_FILEID,
	ID_SEARCH_COL_STATUS
};


CSearchListCtrl::CSearchListCtrl(
	wxWindow *parent,
	wxWindowID winid,
	const wxPoint &pos,
	const wxSize &size,
	long style,
	const wxValidator &validator,
	const wxString &name)
:
CMuleListCtrl(parent, winid, pos, size, style | wxLC_OWNERDRAW, validator, name),
m_filterKnown(false),
m_invert(false),
m_filterEnabled(false)
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	InsertColumn( ID_SEARCH_COL_NAME,    _("File Name"), wxLIST_FORMAT_LEFT, 500, wxT("N") );
	InsertColumn( ID_SEARCH_COL_SIZE,    _("Size"),      wxLIST_FORMAT_LEFT, 100, wxT("Z") );
	InsertColumn( ID_SEARCH_COL_SOURCES, _("Sources"),   wxLIST_FORMAT_LEFT,  50, wxT("u") );
	InsertColumn( ID_SEARCH_COL_TYPE,    _("Type"),      wxLIST_FORMAT_LEFT,  65, wxT("Y") );
	InsertColumn( ID_SEARCH_COL_FILEID,  _("FileID"),    wxLIST_FORMAT_LEFT, 280, wxT("I") );
	InsertColumn( ID_SEARCH_COL_STATUS,  _("Status"),    wxLIST_FORMAT_LEFT, 100, wxT("S") );

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
}


wxString CSearchListCtrl::GetOldColumnOrder() const
{
	return wxT("N,Z,u,Y,I,S");
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
	wxCHECK_RET(toshow->GetSearchID() == m_nResultsID, wxT("Wrong search-id for result-list"));

	const wxUIntPtr toshowdata = reinterpret_cast<wxUIntPtr>(toshow);
	CSearchFile* parent = toshow->GetParent();

	// Check if the result should be shown
	if (FindItem(-1, toshowdata) != -1) {
		return;
	} else if (parent && !parent->ShowChildren()) {
		return;
	} else if (!IsFiltered(toshow)) {
		if (toshow->HasChildren() && toshow->ShowChildren()) {
			// Only filter the parent if none of the children are shown.
			bool foundChild = false;
			const CSearchResultList& children = toshow->GetChildren();
			for (size_t i = 0; i < children.size(); ++i) {
				if (IsFiltered(children.at(i))) {
					foundChild = true;
					break;
				}
			}

			if (!foundChild) {
				// No children left, and the parent is filtered.
				m_filteredOut.push_back(toshow);
				return;
			}			
		} else {		
			m_filteredOut.push_back(toshow);
			return;
		}
	}

	// Insert the item before the item found by the search
	long insertPos;
	if (parent) {
		insertPos = FindItem(-1, (wxUIntPtr)parent);
		if (insertPos == -1) {
			wxFAIL;
			insertPos = GetItemCount();
		} else {
			insertPos++;
		}
	} else {
		insertPos = GetInsertPos(toshowdata);
	}
	long newid = InsertItem(insertPos, toshow->GetFileName().GetPrintable());

	// Sanity checks to ensure that results/children are properly positioned.
#ifdef __WXDEBUG__
	{
		if (newid > 0) {
			CSearchFile* before = (CSearchFile*)GetItemData(newid - 1);
			wxASSERT(before);
			if (parent) {
				wxASSERT((before->GetParent() == parent) || (before == parent));
			} else {
				wxASSERT(before->GetParent() != toshow);
			}
		}
		
		if ((int)newid < GetItemCount() - 1) {
			CSearchFile* after = (CSearchFile*)GetItemData(newid + 1);
			wxASSERT(after);
			if (parent) {
				wxASSERT((after->GetParent() == parent) || (!after->GetParent()));
			} else {
				wxASSERT((after->GetParent() == toshow) || (!after->GetParent()));
			}
		}
	}
#endif

	SetItemPtrData(newid, toshowdata);

	// Filesize
	SetItem(newid, ID_SEARCH_COL_SIZE, CastItoXBytes( toshow->GetFileSize() ) );

	// Source count
	wxString temp = CFormat(wxT("%d")) % toshow->GetSourceCount();
	if (toshow->GetCompleteSourceCount()) {
		temp += CFormat(wxT(" (%d)")) % toshow->GetCompleteSourceCount();
	}
	if (toshow->GetClientsCount()) {
		temp += CFormat(wxT(" [%d]")) % toshow->GetClientsCount();
	}
#if defined(__DEBUG__) && !defined(CLIENT_GUI)
	if (toshow->GetKadPublishInfo() == 0) {
		temp += wxT(" | -");
	} else {
		temp += CFormat(wxT(" | N:%u, P:%u, T:%0.2f"))
			% ((toshow->GetKadPublishInfo() & 0xFF000000) >> 24)
			% ((toshow->GetKadPublishInfo() & 0x00FF0000) >> 16)
			% ((toshow->GetKadPublishInfo() & 0x0000FFFF) / 100.0);
	}
#endif
	SetItem( newid, ID_SEARCH_COL_SOURCES, temp );

	// File-type
	SetItem( newid, ID_SEARCH_COL_TYPE, GetFiletypeByName( toshow->GetFileName() ) );

	// File-hash
	SetItem(newid, ID_SEARCH_COL_FILEID, toshow->GetFileHash().Encode() );

	// File status
	SetItem(newid, ID_SEARCH_COL_STATUS, DetermineStatusPrintable(toshow));

	// Set the color of the item
	UpdateItemColor( newid );
}


void CSearchListCtrl::RemoveResult(CSearchFile* toremove)
{
	ShowChildren(toremove, false);
	
	long index = FindItem(-1, reinterpret_cast<wxUIntPtr>(toremove));
	if (index != -1) {
		DeleteItem(index);
	} else {
		ResultList::iterator it = std::find(m_filteredOut.begin(), m_filteredOut.end(), toremove);
		if ( it != m_filteredOut.end()) {
			m_filteredOut.erase(it);
		}
	}
}


void CSearchListCtrl::UpdateResult(CSearchFile* toupdate)
{
	long index = FindItem(-1, reinterpret_cast<wxUIntPtr>(toupdate));
	if (index != -1) {
		// Update the filename, which may be changed in case of multiple variants.
		SetItem(index, ID_SEARCH_COL_NAME, toupdate->GetFileName().GetPrintable());

		wxString temp = CFormat(wxT("%d")) % toupdate->GetSourceCount();
		if (toupdate->GetCompleteSourceCount()) {
			temp += CFormat(wxT(" (%d)")) % toupdate->GetCompleteSourceCount();
		}
		if (toupdate->GetClientsCount()) {
			temp += CFormat(wxT(" [%d]")) % toupdate->GetClientsCount();
		}
#if defined(__DEBUG__) && !defined(CLIENT_GUI)
		if (toupdate->GetKadPublishInfo() == 0) {
			temp += wxT(" | -");
		} else {
			temp += CFormat(wxT(" | N:%u, P:%u, T:%0.2f"))
				% ((toupdate->GetKadPublishInfo() & 0xFF000000) >> 24)
				% ((toupdate->GetKadPublishInfo() & 0x00FF0000) >> 16)
				% ((toupdate->GetKadPublishInfo() & 0x0000FFFF) / 100.0);
		}
#endif
		SetItem(index, ID_SEARCH_COL_SOURCES, temp);

		SetItem(index, ID_SEARCH_COL_STATUS, DetermineStatusPrintable(toupdate));

		UpdateItemColor(index);

		// Deletions of items causes rather large amount of flicker, so to
		// avoid this, we resort the list to ensure correct ordering.
		if (!IsItemSorted(index)) {
			SortList();
		}
	}
}


void CSearchListCtrl::UpdateItemColor(long index)
{
	wxListItem item;
	item.SetId( index );
	item.SetColumn( ID_SEARCH_COL_SIZE );
	item.SetMask(
		wxLIST_MASK_STATE |
		wxLIST_MASK_TEXT |
		wxLIST_MASK_IMAGE |
		wxLIST_MASK_DATA |
		wxLIST_MASK_WIDTH |
		wxLIST_MASK_FORMAT);

	if (GetItem(item)) {
		CMuleColour newcol(wxSYS_COLOUR_WINDOWTEXT);

		CSearchFile* file = (CSearchFile*)GetItemData(index);

		int red		= newcol.Red();
		int green	= newcol.Green();
		int blue	= newcol.Blue();

		switch (file->GetDownloadStatus()) {
		case CSearchFile::DOWNLOADED:
			// File has already been downloaded. Mark as green.
			green = 255;
			break;
		case CSearchFile::QUEUED:
			// File is downloading.
		case CSearchFile::QUEUEDCANCELED:
			// File is downloading and has been canceled before.
			// Mark as red
			red = 255;
			break;
		case CSearchFile::CANCELED:
			// File has been canceled. Mark as magenta.
			red = 255;
			blue = 255;
			break;
		default:
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
		SetItem( newitem );	
	}
}


void CSearchListCtrl::ShowResults( long ResultsID )
{
	DeleteAllItems();
	m_nResultsID = ResultsID;
	if (ResultsID) {
		const CSearchResultList& list = theApp->searchlist->GetSearchResults(ResultsID);
		for (unsigned int i = 0; i < list.size(); ++i) {
			AddResult( list[i] );
		}
	}
}


wxUIntPtr CSearchListCtrl::GetSearchId()
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
		result = m_filter.Matches(file->GetFileName().GetPrintable());
		result = ((result && !m_invert) || (!result && m_invert));
		if (result && m_filterKnown) {
			result = file->GetDownloadStatus() == CSearchFile::NEW;
		}
	}

	return result;
}


int CSearchListCtrl::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CSearchFile* file1 = (CSearchFile*)item1;
	CSearchFile* file2 = (CSearchFile*)item2;

	// Modifies the result, 1 for ascending, -1 for decending
	int modifier = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	bool alternate = (sortData & CMuleListCtrl::SORT_ALT) != 0;

	// Decide if which should files we should sort by.
	wxUIntPtr parent1 = reinterpret_cast<wxUIntPtr>(file1->GetParent());
	wxUIntPtr parent2 = reinterpret_cast<wxUIntPtr>(file2->GetParent());
	wxUIntPtr filePtr1 = reinterpret_cast<wxUIntPtr>(file1);
	wxUIntPtr filePtr2 = reinterpret_cast<wxUIntPtr>(file2);
	if (parent1 && parent2) {
		if (parent1 != parent2) {
			return SortProc(parent1, parent2, sortData);
		}
	} else if (parent1) {
		if (parent1 == filePtr2) {
			return 1;
		} else {
			return SortProc(parent1, filePtr2, sortData);
		}
	} else if (parent2) {
		if (parent2 == filePtr1) {
			return -1;
		} else {
			return SortProc(filePtr1, parent2, sortData);
		}
	}

	int result = 0;
	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
		// Sort by filename
		case ID_SEARCH_COL_NAME:
			result = CmpAny(file1->GetFileName(), file2->GetFileName());
			break;

		// Sort file-size
		case ID_SEARCH_COL_SIZE:
			result = CmpAny( file1->GetFileSize(), file2->GetFileSize() );
			break;

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

			result = cmp;
			break;
		}
		
		// Sort by file-types
		case ID_SEARCH_COL_TYPE: {
			result = GetFiletypeByName(file1->GetFileName()).Cmp(GetFiletypeByName(file2->GetFileName()));
			if (result == 0) {
				// Same file-type, sort by extension
				result = CmpAny(file1->GetFileName().GetExt(), file2->GetFileName().GetExt());
			}

			break;
		}

		// Sort by file-hash
		case ID_SEARCH_COL_FILEID:
			result = CmpAny(file2->GetFileHash(), file1->GetFileHash());
			break;
		
		// Sort by file status
		case ID_SEARCH_COL_STATUS:
			result = CmpAny(DetermineStatusPrintable(file2), DetermineStatusPrintable(file1));
			break;
	}

	return modifier * result;
}


void CSearchListCtrl::SetSorting(unsigned column, unsigned order)
{
	Freeze();
	// First collapse all parent items
	// Backward order means our index won't be influenced by items getting collapsed.
	for (int i = GetItemCount(); i--;) {
		CSearchFile* file = ((CSearchFile*)GetItemData(i));
		if (file->ShowChildren()) {
			ShowChildren(file, false);
		}
	}

	// Then do the sorting
	CMuleListCtrl::SetSorting(column, order);
	Thaw();
}


void CSearchListCtrl::SyncLists( CSearchListCtrl* src, CSearchListCtrl* dst )
{
	wxCHECK_RET(src && dst, wxT("NULL argument in SyncLists"));

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
	}
}


void CSearchListCtrl::SyncOtherLists(CSearchListCtrl *src)
{
	std::list<CSearchListCtrl*>::iterator it;

	for (it = s_lists.begin(); it != s_lists.end(); ++it) {
		if ((*it) != src) {
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
		for (unsigned i = 1; i < theApp->glob_prefs->GetCatCount(); i++) {
			cats->Append(MP_ASSIGNCAT + i,
				theApp->glob_prefs->GetCategory(i)->title);
		}
		
		menu.Append(MP_MENU_CATS, _("Download in category"), cats);
		menu.AppendSeparator();

		const wxString & statsServer = thePrefs::GetStatsServerName();
		if (!statsServer.IsEmpty()) {
			menu.Append(MP_RAZORSTATS, CFormat(_("Get %s for this file")) % statsServer);
			menu.AppendSeparator();
		}

		menu.Append(MP_SEARCHRELATED, _("Search related files (eD2k, local server)"));
		menu.AppendSeparator();

//#warning Uncomment this here to test the MP_MARK_AS_KNOWN feature. Beware! You are on your own here, this might break "known.met"
#if 0
		menu.Append(MP_MARK_AS_KNOWN, _("Mark as known file"));
		menu.AppendSeparator();
#endif

		menu.Append(MP_GETED2KLINK, _("Copy eD2k link to clipboard"));

		// These should only be enabled for single-selections
		bool enable = (GetSelectedItemCount() == 1);
		menu.Enable(MP_GETED2KLINK, enable);
		menu.Enable(MP_MENU_CATS, (theApp->glob_prefs->GetCatCount() > 1));

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


void CSearchListCtrl::OnPopupGetUrl( wxCommandEvent& WXUNUSED(event) )
{
	wxString URIs;	
	
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while (index != -1) {
		CSearchFile* file = (CSearchFile*)GetItemData( index );

		URIs += theApp->CreateED2kLink( file ) + wxT("\n");

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	if (!URIs.IsEmpty()) {
		theApp->CopyTextToClipboard( URIs.RemoveLast() );
	}
}


void CSearchListCtrl::OnRazorStatsCheck( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if (item == -1) {
		return;
	}

	CSearchFile* file = (CSearchFile*)GetItemData( item );
	theApp->amuledlg->LaunchUrl(thePrefs::GetStatsServerURL() + file->GetFileHash().Encode());
}


void CSearchListCtrl::OnRelatedSearch( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if (item == -1) {
		return;
	}

	CSearchFile* file = (CSearchFile*)GetItemData( item );
	theApp->searchlist->StopSearch(true);
	theApp->amuledlg->m_searchwnd->ResetControls();
	CastByID( IDC_SEARCHNAME, theApp->amuledlg->m_searchwnd, wxTextCtrl )->
		SetValue(wxT("related::") + file->GetFileHash().Encode());
	theApp->amuledlg->m_searchwnd->StartNewSearch();
}


void CSearchListCtrl::OnMarkAsKnown( wxCommandEvent& WXUNUSED(event) )
{
#ifndef CLIENT_GUI
	long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (index > -1) {
		CSearchFile *searchFile = (CSearchFile *)GetItemData(index);
		CKnownFile *knownFile(new CKnownFile(*searchFile));
		theApp->knownfiles->SafeAddKFile(knownFile);
		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
#endif
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


void CSearchListCtrl::OnItemActivated(wxListEvent& event)
{
	CSearchFile* file = ((CSearchFile*)GetItemData(event.GetIndex()));
	if (file->HasChildren()) {
		ShowChildren(file, !file->ShowChildren());
	} else {
		DownloadSelected();
	}
}


bool CSearchListCtrl::AltSortAllowed(unsigned column) const
{
	switch (column) {
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
	
	// Process all selections
	long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (index > -1) {
		CSearchFile* file = (CSearchFile*)GetItemData(index);
		CoreNotify_Search_Add_Download(file, category);
		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
	// Listcontrol gets updated by notification when download is started
}


const wxBrush& GetBrush(wxSystemColour index)
{
	return CMuleColour(index).GetBrush();
}


void CSearchListCtrl::OnDrawItem(
	int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted)
{
	CSearchFile* file = (CSearchFile*)GetItemData(item);

	// Define text-color and background
	if (highlighted) {
		if (GetFocus()) {
			dc->SetBackground(GetBrush(wxSYS_COLOUR_HIGHLIGHT));
			dc->SetTextForeground(CMuleColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		} else {
			dc->SetBackground(GetBrush(wxSYS_COLOUR_BTNSHADOW));
			dc->SetTextForeground(CMuleColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		}
	} else {
		dc->SetBackground(GetBrush(wxSYS_COLOUR_LISTBOX));
		dc->SetTextForeground(CMuleColour(wxSYS_COLOUR_WINDOWTEXT));
	}

	// Define the border of the drawn area
	if (highlighted) {
		dc->SetPen(*(wxThePenList->FindOrCreatePen(CMuleColour(dc->GetBackground().GetColour()).Blend(65), 1, wxSOLID)));
	} else {
		dc->SetPen(*wxTRANSPARENT_PEN);
		dc->SetTextForeground(GetItemTextColour(item));
	}

	// Clear the background, not done automatically since the drawing is buffered.
	dc->SetBrush( dc->GetBackground() );
	dc->DrawRectangle( rectHL.x, rectHL.y, rectHL.width, rectHL.height );

	// Various constant values we use
	const int iTextOffset = ( rect.GetHeight() - dc->GetCharHeight() ) / 2;
	const int iOffset = 4;
	const int treeOffset = 11;
	const int treeCenter = 6;
	bool tree_show = false;

	wxRect cur_rec(iOffset, rect.y, 0, rect.height );
	for (int i = 0; i < GetColumnCount(); i++) {
		wxListItem listitem;
		GetColumn(i, listitem);

		if ( listitem.GetWidth() > 0 ) {
			cur_rec.width = listitem.GetWidth() - 2*iOffset;

			// Make a copy of the current rectangle so we can apply specific tweaks
			wxRect target_rec = cur_rec;
			
			// will ensure that text is about in the middle ;)
			target_rec.y += iTextOffset;
			
			if (i == 0) {
				if (file->HasChildren() || file->GetParent()) {
					tree_show = (listitem.GetWidth() > 0);
					target_rec.x += treeOffset;
					target_rec.width -= treeOffset;

					// Children are indented a bit
					if (file->GetParent()) {
						target_rec.x += 4;
						target_rec.width -= 4;
					}
				}

				// Check if the rating icon should be drawn
				if (file->HasRating()) {
					int image = Client_InvalidRating_Smiley + file->UserRating() - 1;
					
					int imgWidth = 16;
					
					theApp->amuledlg->m_imagelist.Draw(image, *dc, target_rec.GetX(),
							target_rec.GetY() - 1, wxIMAGELIST_DRAW_TRANSPARENT);

					// Move the text past the icon.
					target_rec.x += imgWidth + 4;
					target_rec.width -= imgWidth + 4;
				}
			}
		
			wxListItem cellitem;
			cellitem.SetColumn(i);
			cellitem.SetId(item);

			// Force clipper (clip 2 px more than the rectangle from the right side)
			wxDCClipper clipper(*dc, target_rec.x, target_rec.y, target_rec.width - 2, target_rec.height);
			
			if (GetItem(cellitem)) {
				dc->DrawText(cellitem.GetText(), target_rec.GetX(), target_rec.GetY());
			} else {
				dc->DrawText(wxT("GetItem failed!"), target_rec.GetX(), target_rec.GetY());
			}
			
			// Increment to the next column
			cur_rec.x += listitem.GetWidth();
		}
	}

	// Draw tree last so it draws over selected and focus (looks better)
	if (tree_show) {
		// Gather some information
		const bool notLast = (item + 1 < GetItemCount());
		const bool notFirst = (item != 0);
		const bool hasNext = notLast && ((CSearchFile*)GetItemData(item + 1))->GetParent();
		const int middle = cur_rec.y + ( cur_rec.height + 1 ) / 2;

		// Set up a new pen for drawing the tree
		dc->SetPen( *(wxThePenList->FindOrCreatePen(dc->GetTextForeground(), 1, wxSOLID)) );

		if (file->GetParent()) {
			// Draw the line to the filename
			dc->DrawLine(treeCenter, middle, treeOffset + 4, middle);

			// Draw the line to the child node
			if (hasNext) {
				dc->DrawLine(treeCenter, middle, treeCenter, cur_rec.y + cur_rec.height + 1);
			}

			// Draw the line back up to parent node
			if (notFirst) {
				dc->DrawLine(treeCenter, middle, treeCenter, cur_rec.y - 1);
			}
		} else if (file->HasChildren()) {
		   	if (file->ShowChildren()) {
				// Draw empty circle
				dc->SetBrush(*wxTRANSPARENT_BRUSH);
			} else {
				dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(GetItemTextColour(item))));
			}

			dc->DrawCircle( treeCenter, middle, 3 );

			// Draw the line to the child node if there are any children
			if (hasNext && file->ShowChildren()) {
				dc->DrawLine(treeCenter, middle + 3, treeCenter, cur_rec.y + cur_rec.height + 1);
			}
		}
	}

	// Sanity checks to ensure that results/children are properly positioned.
#ifdef __WXDEBUG__
	{
		CSearchFile* parent = file->GetParent();

		if (item > 0) {
			CSearchFile* before = (CSearchFile*)GetItemData(item - 1);
			wxASSERT(before);
			if (parent) {
				wxASSERT((before->GetParent() == parent) || (before == parent));
			} else {
				wxASSERT(before->GetParent() != file);
			}
		}
		
		if (item < GetItemCount() - 1) {
			CSearchFile* after = (CSearchFile*)GetItemData(item + 1);
			wxASSERT(after);
			if (parent) {
				wxASSERT((after->GetParent() == parent) || (!after->GetParent()));
			} else {
				wxASSERT((after->GetParent() == file) || (!after->GetParent()));
			}
		}
	}
#endif
}


void CSearchListCtrl::ShowChildren(CSearchFile* file, bool show)
{
	Freeze();
		
	file->SetShowChildren(show);

	const CSearchResultList& results = file->GetChildren();
	for (size_t i = 0; i < results.size(); ++i) {
		if (show) {
			AddResult(results[i]);
		} else {
			RemoveResult(results[i]);
		}
	}

	Thaw();
}


wxString CSearchListCtrl::GetTTSText(unsigned item) const
{
	return GetItemText(item);
}


wxString CSearchListCtrl::DetermineStatusPrintable(CSearchFile *toshow)
{
	switch (toshow->GetDownloadStatus()) {
	case CSearchFile::DOWNLOADED:
		// File has already been downloaded.
		return _("Downloaded");
	case CSearchFile::QUEUED:
		// File is downloading.
	case CSearchFile::QUEUEDCANCELED:
		// File is downloading and has been canceled before.
		return _("Queued");
	case CSearchFile::CANCELED:
		// File has been canceled.
		return _("Canceled");
	default:
		// File is new.
		return _("New");
	}
}
// File_checked_for_headers
