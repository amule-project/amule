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

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#include <cmath>		// Needed for floor

#include <wx/dcmemory.h>
#include <wx/datetime.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/intl.h>		// Needed for wxGetTranslation()

#include "DownloadListCtrl.h"	// Interface declarations
#include "OtherFunctions.h"	// Needed for CheckShowItemInGivenCat
#include "DataToText.h"		// Needed for PriorityToStr
#include "amule.h"		// Needed for theApp
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "ChatWnd.h"		// Needed for CChatWnd
#include "PartFile.h"		// Needed for CPartFile
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "FileDetailDialog.h"	// Needed for CFileDetailDialog
#include "updownclient.h"	// Needed for CUpDownClient
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "muuli_wdr.h"		// Needed for ID_DLOADLIST
#include "Color.h"		// Needed for BLEND and SYSCOLOR
#include "BarShader.h"		// Needed for CBarShader
#include "Preferences.h"
#include "Logger.h"
#include "Format.h"		// Needed for CFormat

#include <list>



class CPartFile;


int CDownloadListCtrl::s_lastOrder;
int CDownloadListCtrl::s_lastColumn;

struct CtrlItem_Struct
{
	DownloadItemType	type;
	CPartFile*	owner;
	void*		value;
	uint32		dwUpdated;
	wxBitmap*	status;

	CtrlItem_Struct()
		: type( FILE_TYPE ),
		  owner( NULL ),
		  value( NULL ),
		  dwUpdated( 0 ),
		  status( NULL )
	{ }
	
	~CtrlItem_Struct()
	{
		delete status;
	}
};



#define m_ImageList theApp.amuledlg->imagelist


BEGIN_EVENT_TABLE(CDownloadListCtrl, CMuleListCtrl)
	EVT_LIST_COL_CLICK( -1, 		CDownloadListCtrl::OnColumnLClick)
	EVT_LIST_ITEM_ACTIVATED(ID_DLOADLIST,	CDownloadListCtrl::OnItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnMouseRightClick)
	EVT_LIST_ITEM_MIDDLE_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnMouseMiddleClick)

	EVT_CHAR( CDownloadListCtrl::OnKeyPressed )

	EVT_MENU( MP_DROP_NO_NEEDED_SOURCES,	CDownloadListCtrl::OnCleanUpSources )
	EVT_MENU( MP_DROP_FULL_QUEUE_SOURCES,	CDownloadListCtrl::OnCleanUpSources )
	EVT_MENU( MP_DROP_HIGH_QUEUE_RATING_SOURCES,	CDownloadListCtrl::OnCleanUpSources )
	EVT_MENU( MP_CLEAN_UP_SOURCES,		CDownloadListCtrl::OnCleanUpSources )

	EVT_MENU( MP_CANCEL, 			CDownloadListCtrl::OnCancelFile )
	
	EVT_MENU( MP_PAUSE,			CDownloadListCtrl::OnSetStatus )
	EVT_MENU( MP_STOP,			CDownloadListCtrl::OnSetStatus )
	EVT_MENU( MP_RESUME,			CDownloadListCtrl::OnSetStatus )
	
	EVT_MENU( MP_PRIOLOW,			CDownloadListCtrl::OnSetPriority )
	EVT_MENU( MP_PRIONORMAL,		CDownloadListCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOHIGH,			CDownloadListCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOAUTO,			CDownloadListCtrl::OnSetPriority )

	EVT_MENU( MP_SWAP_A4AF_TO_THIS,		CDownloadListCtrl::OnSwapSources )
	EVT_MENU( MP_SWAP_A4AF_TO_THIS_AUTO,	CDownloadListCtrl::OnSwapSources )
	EVT_MENU( MP_SWAP_A4AF_TO_ANY_OTHER,	CDownloadListCtrl::OnSwapSources )

	EVT_MENU_RANGE( MP_ASSIGNCAT, MP_ASSIGNCAT + 99, CDownloadListCtrl::OnSetCategory )

	EVT_MENU( MP_CLEARCOMPLETED,		CDownloadListCtrl::OnClearCompleted )

	EVT_MENU( MP_GETED2KLINK,		CDownloadListCtrl::OnGetED2KLink )
	EVT_MENU( MP_GETHTMLED2KLINK,		CDownloadListCtrl::OnGetED2KLink )

	EVT_MENU( MP_METINFO,			CDownloadListCtrl::OnViewFileInfo )
	EVT_MENU( MP_VIEW,			CDownloadListCtrl::OnPreviewFile )
	EVT_MENU( MP_VIEWFILECOMMENTS,		CDownloadListCtrl::OnViewFileComments )

	EVT_MENU( MP_WS,			CDownloadListCtrl::OnGetFeedback )
	EVT_MENU( MP_RAZORSTATS, 		CDownloadListCtrl::OnGetRazorStats )

	EVT_MENU( MP_CHANGE2FILE,		CDownloadListCtrl::OnSwapSource )
	EVT_MENU( MP_SHOWLIST,			CDownloadListCtrl::OnViewFiles )
	EVT_MENU( MP_ADDFRIEND,			CDownloadListCtrl::OnAddFriend )
	EVT_MENU( MP_SENDMESSAGE,		CDownloadListCtrl::OnSendMessage )
	EVT_MENU( MP_DETAIL,			CDownloadListCtrl::OnViewClientInfo )
END_EVENT_TABLE()



//! This listtype is used when gathering the selected items.
typedef std::list<CtrlItem_Struct*>	ItemList;



CDownloadListCtrl::CDownloadListCtrl(
	wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
	long style, const wxValidator& validator, const wxString& name )
:
CMuleListCtrl( parent, winid, pos, size, style | wxLC_OWNERDRAW, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Download") );

	m_menu = NULL;

	wxColour colour = BLEND( SYSCOLOR( wxSYS_COLOUR_HIGHLIGHT ), 125 );
	m_hilightBrush  = new wxBrush( colour, wxSOLID );
	
	colour = BLEND( SYSCOLOR( wxSYS_COLOUR_BTNSHADOW), 125 );
	m_hilightUnfocusBrush = new wxBrush( colour, wxSOLID );

	InsertColumn( 0,  _("File Name"),		wxLIST_FORMAT_LEFT, 260 );
	InsertColumn( 1,  _("Size"),			wxLIST_FORMAT_LEFT,  60 );
	InsertColumn( 2,  _("Transferred"),		wxLIST_FORMAT_LEFT,  65 );
	InsertColumn( 3,  _("Completed"),		wxLIST_FORMAT_LEFT,  65 );
	InsertColumn( 4,  _("Speed"),			wxLIST_FORMAT_LEFT,  65 );
	InsertColumn( 5,  _("Progress"),		wxLIST_FORMAT_LEFT, 170 );
	InsertColumn( 6,  _("Sources"),			wxLIST_FORMAT_LEFT,  50 );
	InsertColumn( 7,  _("Priority"),		wxLIST_FORMAT_LEFT,  55 );
	InsertColumn( 8,  _("Status"),			wxLIST_FORMAT_LEFT,  70 );
	InsertColumn( 9,  _("Time Remaining"),		wxLIST_FORMAT_LEFT, 110 );
	InsertColumn( 10, _("Last Seen Complete"),	wxLIST_FORMAT_LEFT, 220 );
	InsertColumn( 11, _("Last Reception"),		wxLIST_FORMAT_LEFT, 220 );

	m_category = 0;
	m_completedFiles = 0;
	m_filecount = 0;
	LoadSettings();
	
	s_lastOrder  = (GetSortOrder() & CMuleListCtrl::SORT_DES) ? -1 : 1;
	s_lastColumn = GetSortColumn();
}


CDownloadListCtrl::~CDownloadListCtrl()
{
	while ( !m_ListItems.empty() ) {
		delete m_ListItems.begin()->second;
		m_ListItems.erase( m_ListItems.begin() );
	}
	delete m_hilightBrush;
	delete m_hilightUnfocusBrush;
}


void CDownloadListCtrl::AddFile( CPartFile* file )
{
	wxASSERT( file );
	
	// Avoid duplicate entries of files
	if ( m_ListItems.find( file ) == m_ListItems.end() ) {
		CtrlItem_Struct* newitem = new CtrlItem_Struct;
		newitem->type = FILE_TYPE;
		newitem->value = file;
	
		m_ListItems.insert( ListItemsPair( file, newitem ) );
		
		// Check if the new file is visible in the current category
		if ( file->CheckShowItemInGivenCat( m_category ) ) {
			ShowFile( file, true );
		}
	}
}


void CDownloadListCtrl::AddSource(CPartFile* owner, CUpDownClient* source, DownloadItemType type)
{
	wxASSERT( owner );
	wxASSERT( source );

	// Update the other instances of this source
	bool bFound = false;
	ListIteratorPair rangeIt = m_ListItems.equal_range(source);
	for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second; ++it ) {
		CtrlItem_Struct* cur_item = it->second;

		// Check if this source has been already added to this file => to be sure
		if ( cur_item->owner == owner ) {
			// Update this instance with its new setting
			cur_item->type = type;
			cur_item->dwUpdated = 0;
			bFound = true;
		} else if ( type == AVAILABLE_SOURCE ) {
			// The state 'Available' is exclusive
			cur_item->type = A4AF_SOURCE;
			cur_item->dwUpdated = 0;
		}
	}

	if ( bFound ) {
		return;
	}

	if ( owner->ShowSources() ) {
		CtrlItem_Struct* newitem = new CtrlItem_Struct;
		newitem->owner = owner;
		newitem->type = type;
		newitem->value = source;
		
		m_ListItems.insert( ListItemsPair(source, newitem) );

		// Find the owner-object
		ListItems::iterator it = m_ListItems.find( owner );
	
		if ( it != m_ListItems.end() ) {
			long item = FindItem( -1, (long)it->second );
			
			if ( item > -1 ) {
				item = InsertItem( item + 1, wxEmptyString );
				
				SetItemData( item, (long)newitem );

				// background.. this should be in a function
				wxListItem item;
				item.m_itemId = item;

				item.SetBackgroundColour( GetBackgroundColour() );
	
				SetItem( item );
			}
		}
	}
}


void CDownloadListCtrl::RemoveSource( const CUpDownClient* source, const CPartFile* owner )
{
	wxASSERT( source );
	
	// Retrieve all entries matching the source
	ListIteratorPair rangeIt = m_ListItems.equal_range(source);
	
	for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second; ) {
		ListItems::iterator tmp = it++;
		
		CtrlItem_Struct* item = tmp->second;
		if ( owner == NULL || owner == item->owner ) {
			// Remove it from the m_ListItems
			m_ListItems.erase( tmp );

			long index = FindItem( -1, (long)item );
			
			if ( index > -1 ) {
				DeleteItem( index );
			}
			
			delete item;
		}
	}
}


void CDownloadListCtrl::RemoveFile( CPartFile* file )
{
	wxASSERT( file );
	
	// Ensure that any assosiated sources and list-entries are removed
	ShowFile( file, false );

	// Find the assosiated list-item
	ListItems::iterator it = m_ListItems.find( file );

	if ( it != m_ListItems.end() ) {
		delete it->second;

		m_ListItems.erase( it );
	}
}


void CDownloadListCtrl::UpdateItem(const void* toupdate)
{
	// Retrieve all entries matching the source
	ListIteratorPair rangeIt = m_ListItems.equal_range( toupdate );

	// Visible lines, default to all because not all platforms
	// support the GetVisibleLines function
	long first = 0, last = GetItemCount();

#ifndef __WXMSW__
	// Get visible lines if we need them
	if ( rangeIt.first != rangeIt.second ) {
		GetVisibleLines( &first, &last );
	}
#endif
	
	for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second; ++it ) {
		CtrlItem_Struct* item = it->second;

		long index = FindItem( -1, (long)item );

		// Determine if the file should be shown in the current category
		if ( item->type == FILE_TYPE ) {
			CPartFile* file = (CPartFile*)item->value;
		
			bool show = file->CheckShowItemInGivenCat( m_category );
	
			if ( index > -1 ) {
				if ( show ) {
					item->dwUpdated = 0;

					// Only update visible lines
					if ( index >= first && index <= last) {
						RefreshItem( index );
					}
				} else {
					// Item should no longer be shown in
					// the current category
					ShowFile( file, false );
				}
			} else if ( show ) {
				// Item has been hidden but new status means
				// that it should it should be shown in the
				// current category
				ShowFile( file, true );
			}

			if (file->GetStatus() == PS_COMPLETE) {
				m_completedFiles = true;

				CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(true);
			}
		} else {
			item->dwUpdated = 0;

			// Only update visible lines
			if ( index >= first && index <= last) {
				RefreshItem( index );
			}
		}
	}
}


void CDownloadListCtrl::ShowFile( CPartFile* file, bool show )
{
	wxASSERT( file );
	
	ListItems::iterator it = m_ListItems.find( file );
	
	if ( it != m_ListItems.end() ) { 
		CtrlItem_Struct* item = it->second;
	
		if ( show ) {
			// Check if the file is already being displayed
			long index = FindItem( -1, (long)item );
			if ( index == -1 ) {	
				long newitem = InsertItem( GetItemCount(), wxEmptyString );
				
				SetItemData( newitem, (long)item );

				wxListItem myitem;
				myitem.m_itemId = newitem;
				myitem.SetBackgroundColour( GetBackgroundColour() );
				
				SetItem(myitem);	
			
				RefreshItem( newitem );

				ShowFilesCount( 1 );
			}
		} else {
			// Ensure sources are hidden
			ShowSources( file, false );

			// Try to find the file and remove it
			long index = FindItem( -1, (long)item );
			if ( index > -1 ) {
				DeleteItem( index );
				ShowFilesCount( -1 );
			}
		}
	}
}


void CDownloadListCtrl::ShowSources( CPartFile* file, bool show )
{
	// Check if the current state is the same as the new state
	if ( file->ShowSources() == show ) {
		return;
	}
	
	Freeze();
	
	file->SetShowSources( show );
	
	if ( show ) {
		const CPartFile::SourceSet& normSources = file->GetSourceList();
		const CPartFile::SourceSet& a4afSources = file->GetA4AFList();
			
		// Adding normal sources
		CPartFile::SourceSet::iterator it;
		for ( it = normSources.begin(); it != normSources.end(); ++it ) {
			switch ((*it)->GetDownloadState()) {
				case DS_DOWNLOADING:
				case DS_ONQUEUE:
					AddSource( file, *it, AVAILABLE_SOURCE );
				default:
					// Any other state
					AddSource( file, *it, UNAVAILABLE_SOURCE );
			}
			
		}

		// Adding A4AF sources
		for ( it = a4afSources.begin(); it != a4afSources.end(); ++it ) {
			AddSource( file, *it, A4AF_SOURCE );
		}
	} else {
		for ( int i = GetItemCount() - 1; i >= 0; --i ) {
			CtrlItem_Struct* item = (CtrlItem_Struct*)GetItemData(i);
		
			if ( item->type != FILE_TYPE && item->owner == file ) {
				// Remove from the grand list, this call doesn't remove the source
				// from the listctrl, because ShowSources is now false. This also
				// deletes the item.
				RemoveSource( (CUpDownClient*)item->value, file );
			}
		}
	}
	
	Thaw();
}


void CDownloadListCtrl::ChangeCategory( int newCategory )
{
	Freeze();

	// remove all displayed files with a different cat and show the correct ones
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
		const CtrlItem_Struct *cur_item = it->second;
		
		if ( cur_item->type == FILE_TYPE ) {
			CPartFile* file = (CPartFile*)cur_item->value;
	
			bool curVisibility = file->CheckShowItemInGivenCat( m_category );
			bool newVisibility = file->CheckShowItemInGivenCat( newCategory );
		
			// Check if the visibility of the file has changed. However, if the
			// current category is the default (0) category, then we can't use
			// curVisiblity to see if the visibility has changed but instead
			// have to let ShowFile() check if the file is or isn't on the list.
			if ( curVisibility != newVisibility || !newCategory ) {
				ShowFile( file, newVisibility );
			}
		}
	}
	
	Thaw();

	m_category = newCategory;
}


uint8 CDownloadListCtrl::GetCategory() const
{
	return m_category;
}


/*
 *
 */
const int itFILES = 1;
const int itSOURCES = 2;

/**
 * Helper-function: This function is used to gather selected items.
 *
 * @param list A pointer to the list to gather items from.
 * @param types The desired types OR'd together.
 * @return A list containing the selected items of the choosen types.
 */
ItemList GetSelectedItems( CDownloadListCtrl* list, int types )
{
	ItemList results;

	long index = list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while ( index > -1 ) {
		CtrlItem_Struct* item = (CtrlItem_Struct*)list->GetItemData( index );

		bool add = false;
		add |= ( item->type == FILE_TYPE ) && ( types & itFILES );
		add |= ( item->type != FILE_TYPE ) && ( types & itSOURCES );
		
		if ( add ) {
			results.push_back( item );
		}
		
		index = list->GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	return results;
}


void CDownloadListCtrl::OnCleanUpSources( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;
		
		switch ( event.GetId() ) {
			case MP_DROP_NO_NEEDED_SOURCES:
				CoreNotify_PartFile_RemoveNoNeeded( file );
				break;
				
			case MP_DROP_FULL_QUEUE_SOURCES:
				CoreNotify_PartFile_RemoveFullQueue( file );
				break;
				
			case MP_DROP_HIGH_QUEUE_RATING_SOURCES:
				CoreNotify_PartFile_RemoveHighQueue( file );
				break;
				
			case MP_CLEAN_UP_SOURCES:
				CoreNotify_PartFile_SourceCleanup( file );
				break;
		}
	}
}


void CDownloadListCtrl::OnCancelFile(wxCommandEvent& WXUNUSED(event))
{
	ItemList files = ::GetSelectedItems(this, itFILES);
	if (files.size()) {	
		wxString question = 
			_("Are you sure that you wish to delete the selected file(s)?");	
		if (wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES) {
			for (ItemList::iterator it = files.begin(); it != files.end(); ++it) {
				CPartFile* file = (CPartFile*)(*it)->value;		
				switch (file->GetStatus()) {
				case PS_WAITINGFORHASH:
				case PS_HASHING:
				case PS_COMPLETING:
				case PS_COMPLETE:
					break;
				default:
					CoreNotify_PartFile_Delete(file);
				}
			}
		}
	}
}


void CDownloadListCtrl::OnSetPriority( wxCommandEvent& event )
{
	int priority = 0;
	switch ( event.GetId() ) {
		case MP_PRIOLOW:	priority = PR_LOW;	break;
		case MP_PRIONORMAL:	priority = PR_NORMAL;	break;
		case MP_PRIOHIGH:	priority = PR_HIGH;	break;
		case MP_PRIOAUTO:	priority = PR_AUTO;	break;
		default:
			wxASSERT( false );
	}

	ItemList files = ::GetSelectedItems( this, itFILES );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;
	
		if ( priority == PR_AUTO ) {
			CoreNotify_PartFile_PrioAuto( file, true );
		} else {
			CoreNotify_PartFile_PrioAuto( file, false );

			CoreNotify_PartFile_PrioSet( file, priority, true );
		}
	}
}


void CDownloadListCtrl::OnSwapSources( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;

		switch ( event.GetId() ) {
			case MP_SWAP_A4AF_TO_THIS:
				CoreNotify_PartFile_Swap_A4AF( file );
				break;
				
			case MP_SWAP_A4AF_TO_THIS_AUTO:
				CoreNotify_PartFile_Swap_A4AF_Auto( file );
				break;
				
			case MP_SWAP_A4AF_TO_ANY_OTHER:
				CoreNotify_PartFile_Swap_A4AF_Others( file );
				break;
		}
	}
}


void CDownloadListCtrl::OnSetCategory( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;

		CoreNotify_PartFile_SetCat( file, event.GetId() - MP_ASSIGNCAT );
	}

	ChangeCategory( m_category );
}


void CDownloadListCtrl::OnSetStatus( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;

		switch ( event.GetId() ) {	
			case MP_PAUSE:
				CoreNotify_PartFile_Pause( file );
				break;
				
			case MP_RESUME:
				CoreNotify_PartFile_Resume( file );
				break;

			case MP_STOP:
				ShowSources(file, false);
				CoreNotify_PartFile_Stop( file );
				break;
		}
	}
}


void CDownloadListCtrl::OnClearCompleted( wxCommandEvent& WXUNUSED(event) )
{
	ClearCompleted();
}


void CDownloadListCtrl::OnGetED2KLink( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	wxString URIs;

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;

		switch ( event.GetId() ) {	
		case MP_GETED2KLINK:
			URIs += theApp.CreateED2kLink( file ) + wxT("\n");
			break;
			
		case MP_GETHTMLED2KLINK:
			URIs += theApp.CreateHTMLED2kLink( file ) + wxT("\n");
			break;
		}
	}

	if ( !URIs.IsEmpty() ) {
		theApp.CopyTextToClipboard( URIs.BeforeLast(wxT('\n')) );
	}
}


void CDownloadListCtrl::OnGetFeedback( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	wxString feed;

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->value;
	
		feed += CFormat(_("Feedback from: %s")) % thePrefs::GetUserNick() + wxT("\n");
		feed += CFormat(_("Client: aMule %s")) % wxT(VERSION) + wxT("\n");
		feed += CFormat(_("File Name: %s")) % file->GetFileName() + wxT("\n");
		feed += CFormat(_("File size: %s")) % CastItoXBytes(file->GetFileSize()) + wxT("\n");
		feed += CFormat(_("Download: %s")) % CastItoXBytes(file->GetCompletedSize()) + wxT("\n");
		feed += wxString::Format(_("Sources: %u"), file->GetSourceCount()) + wxT("\n");
		feed += wxString::Format(_("Complete Sources: %u"), file->m_nCompleteSourcesCount) + wxT("\n");
	}

	if ( !feed.IsEmpty() ) {
		theApp.CopyTextToClipboard( feed );
	}
}


void CDownloadListCtrl::OnGetRazorStats( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	if ( files.size() == 1 ) {
		CPartFile* file = (CPartFile*)files.front()->value;

		theApp.amuledlg->LaunchUrl(
			wxT("http://stats.razorback2.com/ed2khistory?ed2k=") +
			file->GetFileHash().Encode());
	}
}


void CDownloadListCtrl::OnViewFileInfo( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	if ( files.size() == 1 ) {
		CPartFile* file = (CPartFile*)files.front()->value;

		CFileDetailDialog dialog( this, file );
		dialog.ShowModal();
	}
}


void CDownloadListCtrl::OnViewFileComments( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	if ( files.size() == 1 ) {
		CPartFile* file = (CPartFile*)files.front()->value;

		CCommentDialogLst dialog( this, file );
		dialog.ShowModal();
	}
}


void CDownloadListCtrl::OnPreviewFile( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this, itFILES );

	if ( files.size() == 1 ) {
		PreviewFile( (CPartFile*)files.front()->value );
	}
}

void CDownloadListCtrl::OnSwapSource( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this, itSOURCES );

	for ( ItemList::iterator it = sources.begin(); it != sources.end(); ++it ) {
		CPartFile* file = (CPartFile*)(*it)->owner;
		CUpDownClient* source = (CUpDownClient*)(*it)->value;

		source->SwapToAnotherFile( true, false, false, file );
	}
}


void CDownloadListCtrl::OnViewFiles( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this, itSOURCES );

	if ( sources.size() == 1 ) {
		CUpDownClient* source = (CUpDownClient*)sources.front()->value;
		
		source->RequestSharedFileList();
	}
}


void CDownloadListCtrl::OnAddFriend( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this, itSOURCES );

	for ( ItemList::iterator it = sources.begin(); it != sources.end(); ++it ) {
		CUpDownClient* source = (CUpDownClient*)(*it)->value;

		theApp.amuledlg->chatwnd->AddFriend( source );
	}
}


void CDownloadListCtrl::OnSendMessage( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this, itSOURCES );

	if ( sources.size() == 1 ) {
		CUpDownClient* source = (CUpDownClient*)(sources.front())->value;
	
		wxString message = ::wxGetTextFromUser(
			_("Send message to user"),
			_("Message to send:"));
		if ( !message.IsEmpty() ) {
			theApp.amuledlg->chatwnd->SendMessage(message, source->GetUserName(),GUI_ID(source->GetIP(),source->GetUserPort()));
		}
	}
}


void CDownloadListCtrl::OnViewClientInfo( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this, itSOURCES );

	if ( sources.size() == 1 ) {
		CUpDownClient* source = (CUpDownClient*)(sources.front())->value;

		CClientDetailDialog dialog( this, source );
		dialog.ShowModal();
	}
}


void CDownloadListCtrl::OnColumnLClick(wxListEvent& evt)
{
	// Only change the last column if the sorted column has changed
	if (GetSortColumn() != (unsigned)evt.GetColumn()) {
		s_lastColumn = GetSortColumn();
		s_lastOrder  = (GetSortOrder() & CMuleListCtrl::SORT_DES) ? -1 : 1;
	} else {
		// Reverse the last-column order to preserve the sorting
		s_lastOrder *= -1;
	}

	// Let CMuleListCtrl handle the sorting
	evt.Skip();
}


void CDownloadListCtrl::OnItemActivated( wxListEvent& evt )
{
	CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData( evt.GetIndex() );
	
	if ( content->type == FILE_TYPE ) {
		CPartFile* file = (CPartFile*)content->value;

		ShowSources( file, !file->ShowSources() );
	}
}


void CDownloadListCtrl::OnMouseRightClick(wxListEvent & evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	if ( !GetItemState( evt.GetIndex(), wxLIST_STATE_SELECTED ) ) {
		long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

		while ( item > -1 ) {
			SetItemState( item, 0, wxLIST_STATE_SELECTED );
		
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}
		
		SetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index < 0 ) {
		return;
	}	
	
	CtrlItem_Struct* item = (CtrlItem_Struct*)GetItemData( index );
	
	if ( item->type == FILE_TYPE ) {
		if ( m_menu == NULL ) {
			m_menu = new wxMenu( _("Downloads") );

			wxMenu* priomenu = new wxMenu();
			priomenu->AppendCheckItem(MP_PRIOLOW, _("Low"));
			priomenu->AppendCheckItem(MP_PRIONORMAL, _("Normal"));
			priomenu->AppendCheckItem(MP_PRIOHIGH, _("High"));
			priomenu->AppendCheckItem(MP_PRIOAUTO, _("Auto"));

			m_menu->Append(MP_MENU_PRIO, _("Priority"), priomenu);
			m_menu->Append(MP_CANCEL, _("Cancel"));
			m_menu->Append(MP_STOP, _("&Stop"));
			m_menu->Append(MP_PAUSE, _("&Pause"));
			m_menu->Append(MP_RESUME, _("&Resume"));
			m_menu->Append(MP_CLEARCOMPLETED, _("C&lear completed"));
			//-----------------------------------------------------
			m_menu->AppendSeparator();
			//-----------------------------------------------------
			wxMenu* extendedmenu = new wxMenu();
			extendedmenu->Append(MP_SWAP_A4AF_TO_THIS,
				_("Swap every A4AF to this file now"));
			extendedmenu->AppendCheckItem(MP_SWAP_A4AF_TO_THIS_AUTO,
				_("Swap every A4AF to this file (Auto)"));
			//-----------------------------------------------------
			extendedmenu->AppendSeparator();
			//-----------------------------------------------------
			extendedmenu->Append(MP_SWAP_A4AF_TO_ANY_OTHER,
				_("Swap every A4AF to any other file now"));
			//-----------------------------------------------------
			extendedmenu->AppendSeparator();
			//-----------------------------------------------------
			extendedmenu->Append(MP_DROP_NO_NEEDED_SOURCES,
				_("Drop No Needed Sources now"));
			extendedmenu->Append(MP_DROP_FULL_QUEUE_SOURCES,
				_("Drop Full Queue Sources now"));
			extendedmenu->Append(MP_DROP_HIGH_QUEUE_RATING_SOURCES,
				_("Drop High Queue Rating Sources now"));
			extendedmenu->Append(MP_CLEAN_UP_SOURCES,
				_("Clean Up Sources now (NNS, FQS && HQRS)"));
			m_menu->Append(MP_MENU_EXTD,
				_("Extended Options"), extendedmenu);
			//-----------------------------------------------------
			m_menu->AppendSeparator();
			//-----------------------------------------------------
			m_menu->Append( MP_RAZORSTATS,
				_("Get Razorback 2's stats for this file"));
			//-----------------------------------------------------
			m_menu->AppendSeparator();
			//-----------------------------------------------------
			m_menu->Append(MP_VIEW, _("Preview"));
			m_menu->Append(MP_METINFO, _("Show file &details"));
			m_menu->Append(MP_VIEWFILECOMMENTS,
				_("Show all comments"));
			//-----------------------------------------------------
			m_menu->AppendSeparator();
			//-----------------------------------------------------
			m_menu->Append(MP_GETED2KLINK,
				_("Copy ED2k &link to clipboard"));
			m_menu->Append(MP_GETHTMLED2KLINK,
				_("Copy ED2k link to clipboard (&HTML)"));
			m_menu->Append(MP_WS,
				_("Copy feedback to clipboard"));
		
			// Add dinamic entries
			wxMenu *cats = new wxMenu(_("Category"));
			if (theApp.glob_prefs->GetCatCount() > 1) {
				for (uint32 i = 0; i < theApp.glob_prefs->GetCatCount(); i++) {
					if ( i == 0 ) {
						cats->Append( MP_ASSIGNCAT, _("unassign") );
					} else {
						cats->Append( MP_ASSIGNCAT + i,
							theApp.glob_prefs->GetCategory(i)->title );
					}
				}
			}
			m_menu->Append(MP_MENU_CATS, _("Assign to category"), cats);
			m_menu->Enable(MP_MENU_CATS, (theApp.glob_prefs->GetCatCount() > 1) );

			CPartFile* file = (CPartFile*)item->value;

			// then set state
			bool fileReady =
				(file->GetStatus() != PS_PAUSED) &&
				(file->GetStatus() != PS_ERROR);
			bool fileReady2 =
				(file->GetStatus() != PS_ERROR) &&
				(file->GetStatus() != PS_COMPLETE);
			bool fileResumable =
				(file->GetStatus() == PS_PAUSED) ||
				(file->GetStatus() == PS_INSUFFICIENT);
			
			wxMenu* menu = m_menu;
			menu->Enable( MP_CANCEL,	( file->GetStatus() != PS_COMPLETE ) );
			menu->Enable( MP_PAUSE,		fileReady && fileReady2 );
			menu->Enable( MP_STOP,		fileReady && fileReady2 );
			menu->Enable( MP_RESUME, 	fileResumable );
			menu->Enable( MP_CLEARCOMPLETED, m_completedFiles );

			wxString view;
			if (file->IsPartFile() && !(file->GetStatus() == PS_COMPLETE)) {
				view << _("Preview") << wxT(" [") <<
					file->GetPartMetFileName().BeforeLast(wxT('.')) <<
					wxT("]");
			} else if ( file->GetStatus() == PS_COMPLETE ) {
				view << _("&Open the file");
			}
			menu->SetLabel(MP_VIEW, view);
			menu->Enable(MP_VIEW, file->PreviewAvailable() );

			menu->Enable( MP_DROP_NO_NEEDED_SOURCES,	fileReady );
			menu->Enable( MP_DROP_FULL_QUEUE_SOURCES,	fileReady );
			menu->Enable( MP_DROP_HIGH_QUEUE_RATING_SOURCES,fileReady );
			menu->Enable( MP_CLEAN_UP_SOURCES,		fileReady );
			menu->Enable( MP_SWAP_A4AF_TO_THIS_AUTO,	fileReady );
			menu->Check(  MP_SWAP_A4AF_TO_THIS_AUTO, 	file->IsA4AFAuto() );
			menu->Enable( MP_SWAP_A4AF_TO_ANY_OTHER, 	fileReady );

			int priority = file->IsAutoDownPriority() ?
				PR_AUTO : file->GetDownPriority();
			
			priomenu->Check( MP_PRIOHIGH,	priority == PR_HIGH );
			priomenu->Check( MP_PRIONORMAL, priority == PR_NORMAL );
			priomenu->Check( MP_PRIOLOW,	priority == PR_LOW );
			priomenu->Check( MP_PRIOAUTO,	priority == PR_AUTO );

			menu->Enable( MP_MENU_PRIO, fileReady2 );
			menu->Enable( MP_MENU_EXTD, fileReady2 );
		
			PopupMenu(m_menu, evt.GetPoint());

			delete m_menu;

			m_menu = NULL;
		}
	} else {
		if ( m_menu == NULL ) {
			m_menu = new wxMenu(wxT("Clients"));
			m_menu->Append(MP_DETAIL, _("Show &Details"));
			m_menu->Append(MP_ADDFRIEND, _("Add to Friends"));
			m_menu->Append(MP_SHOWLIST, _("View Files"));
			m_menu->Append(MP_SENDMESSAGE, _("Send message"));
			m_menu->Append(MP_CHANGE2FILE, _("Swap to this file"));
			
			// Only enable the Swap option for A4AF sources
			m_menu->Enable(MP_CHANGE2FILE, ( item->type == A4AF_SOURCE ) );

			PopupMenu(m_menu, evt.GetPoint());

			delete m_menu;
			
			m_menu = NULL;
		}
	}
}


void CDownloadListCtrl::OnMouseMiddleClick(wxListEvent & evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	if ( !GetItemState( evt.GetIndex(), wxLIST_STATE_SELECTED ) ) {
		long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

		while ( item > -1 ) {
			SetItemState( item, 0, wxLIST_STATE_SELECTED );
		
			item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}
		
		SetItemState(evt.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index < 0 ) {
		return;
	}

	CtrlItem_Struct* item = (CtrlItem_Struct*)GetItemData( index );
	
	if ( item->type == FILE_TYPE ) {
		CFileDetailDialog(this, (CPartFile*)item->value).ShowModal();
	} else {
		CClientDetailDialog(this, (CUpDownClient*)item->value).ShowModal();
	}
}


void CDownloadListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// Check if delete was pressed
	switch (event.GetKeyCode()) {
		case WXK_BACK:
		case WXK_NUMPAD_DELETE:
		case WXK_DELETE: {
			wxCommandEvent evt;
			OnCancelFile( evt );
			break;
		}
		case WXK_F2: {
			ItemList files = ::GetSelectedItems( this, itFILES );
			if (files.size() == 1) {	
				CPartFile* file = (CPartFile*)(*(files.begin()))->value;
				wxString NewName = ::wxGetTextFromUser(
					_("Enter new name for this file:"),
					_("File rename"), file->GetFileName());
				if (!NewName.IsEmpty()) {
					file->SetFileName(NewName);
					file->SavePartFile();
				}
			}
			break;
		}
		default:
			event.Skip();
	}
}


void CDownloadListCtrl::OnDrawItem(
	int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted)
{
	// Don't do any drawing if there's nobody to see it.
	if ( !theApp.amuledlg->IsDialogVisible( CamuleDlg::TransferWnd ) ) {
		return;
	}

	CtrlItem_Struct* content = (CtrlItem_Struct *)GetItemData(item);

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
		dc->SetBackground(*(wxTheBrushList->FindOrCreateBrush(
			wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX), wxSOLID)));
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

		wxColor newcol(
			((int)old.Red() * 65) / 100,
			((int)old.Green() * 65) / 100,
			((int)old.Blue() * 65) / 100);

		dc->SetPen( wxPen(newcol, 1, wxSOLID) );
	} else {
		dc->SetPen(*wxTRANSPARENT_PEN);
	}


	dc->SetBrush( dc->GetBackground() );
	dc->DrawRectangle( rectHL.x, rectHL.y, rectHL.width, rectHL.height );

	dc->SetPen(*wxTRANSPARENT_PEN);

	if ( content->type == FILE_TYPE && ( !highlighted || !GetFocus() ) ) {
		// If we have category, override textforeground with what category tells us.
		CPartFile *file = (CPartFile *) content->value;
		if ( file->GetCategory() ) {
			dc->SetTextForeground(
				WxColourFromCr(theApp.glob_prefs->GetCatColor(file->GetCategory())) );
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
	for (int i = 0; i < GetColumnCount(); i++) {
		wxListItem listitem;
		GetColumn(i, listitem);

		if ( listitem.GetWidth() > 0 ) {
			cur_rec.width = listitem.GetWidth() - 2*iOffset;

			// Make a copy of the current rectangle so we can apply specific tweaks
			wxRect target_rec = cur_rec;
			if ( i == 5 ) {
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
				DrawFileItem(dc, i, target_rec, content);
			} else {
				DrawSourceItem(dc, i, target_rec, content);
			}

			// Increment to the next column
			cur_rec.x += listitem.GetWidth();
		}
	}

	// Draw tree last so it draws over selected and focus (looks better)
	if ( tree_show ) {
		// Gather some information
		const bool notLast = item + 1 != GetItemCount();
		const bool notFirst = item != 0;
		const bool hasNext = notLast &&
			((CtrlItem_Struct*)GetItemData(item + 1))->type != FILE_TYPE;
		const bool isOpenRoot = content->type == FILE_TYPE &&
			((CPartFile*)content->value)->ShowSources();
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

			// Draw the line to the child node if there are any children
			if (hasNext) {
				dc->DrawLine(treeCenter, middle + 3, treeCenter, cur_rec.height + 1);
			}
		}

	}
}


void CDownloadListCtrl::DrawFileItem( wxDC* dc, int nColumn, const wxRect& rect, CtrlItem_Struct* item ) const
{
	// force clipper (clip 2 px more than the rectangle from the right side)
	wxDCClipper clipper( *dc, rect.GetX(), rect.GetY(), rect.GetWidth() - 2, rect.GetHeight() );

	const CPartFile* file = (const CPartFile*)item->value;

	// Used to contain the contenst of cells that dont need any fancy drawing, just text.
	wxString text;

	switch (nColumn) {
	// Filename
	case 0: {
		if (file->HasRating()) {
			int image = Client_InvalidRating_Smiley + file->UserRating() - 1;
			wxASSERT(image >= Client_InvalidRating_Smiley);
			wxASSERT(image <= Client_ExcellentRating_Smiley);
			
			int imgWidth;
			if (file->UserRating() == 1 || file->UserRating() == 5) {
				imgWidth=16;
			} else {
				imgWidth=8;
			}
			
			// it's already centered by OnDrawItem() ...
			m_ImageList.Draw(image, *dc, rect.GetX(), rect.GetY() - 1,
				wxIMAGELIST_DRAW_TRANSPARENT);
			dc->DrawText( file->GetFileName(), rect.GetX() + imgWidth + 4, rect.GetY());
		} else {
			dc->DrawText( file->GetFileName(), rect.GetX(), rect.GetY());
		}
	}
	break;

	// Filesize
	case 1:
		text = CastItoXBytes( file->GetFileSize() );
		break;

	// Transfered
	case 2:
		text = CastItoXBytes( file->GetTransfered() );
		break;
	
	// Completed
	case 3:
		text = CastItoXBytes( file->GetCompletedSize() );
		break;
	
	// Speed
	case 4:	// speed
		if ( file->GetTransferingSrcCount() ) {
			text = wxString::Format( wxT("%.1f "), file->GetKBpsDown() ) +
				_("kB/s");
		}
		break;
	
	case 5:	// progress
	{
		if (thePrefs::ShowProgBar())
		{
			int iWidth  = rect.GetWidth() - 2;
			int iHeight = rect.GetHeight() - 2;

			// DO NOT DRAW IT ALL THE TIME
			uint32 dwTicks = GetTickCount();
			wxMemoryDC cdcStatus;
			
			if ( item->dwUpdated < dwTicks || !item->status || iWidth != item->status->GetWidth() ) {
				if ( item->status == NULL) {
					item->status = new wxBitmap(iWidth, iHeight);
				} else if ( item->status->GetWidth() != iWidth ) {
					// Only recreate if the size has changed
					item->status->Create(iWidth, iHeight);
				}
						
				cdcStatus.SelectObject( *item->status );
				
				if ( thePrefs::UseFlatBar() ) {
					DrawFileStatusBar( file, &cdcStatus,
						wxRect(0, 0, iWidth, iHeight), true);
				} else {
					DrawFileStatusBar( file, &cdcStatus,
						wxRect(1, 1, iWidth - 2, iHeight - 2), false);
		
					// Draw black border
					cdcStatus.SetPen( *wxBLACK_PEN );
					cdcStatus.SetBrush( *wxTRANSPARENT_BRUSH );
					cdcStatus.DrawRectangle( 0, 0, iWidth, iHeight );
				}
			
				item->dwUpdated = dwTicks + 5000; // Plus five seconds
			} else {
				cdcStatus.SelectObject( *item->status );
			}
			
			dc->Blit( rect.GetX(), rect.GetY() + 1, iWidth, iHeight, &cdcStatus, 0, 0);

			cdcStatus.SelectObject(wxNullBitmap);
		}
		
		if (thePrefs::ShowPercent()) {
			// Percentage of completing
			// We strip anything below the first decimal point,
			// to avoid Format doing roundings
			float percent = floor( file->GetPercentCompleted() * 10.0f ) / 10.0f;
		
			wxString buffer = wxString::Format( wxT("%.1f%%"), percent );
			int middlex = (2*rect.GetX() + rect.GetWidth()) >> 1;
			int middley = (2*rect.GetY() + rect.GetHeight()) >> 1;
			
			wxCoord textwidth, textheight;
			
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

	// Sources
	case 6:	{
		uint16 sc = file->GetSourceCount();
		uint16 ncsc = file->GetNotCurrentSourcesCount();
		if ( ncsc ) {
			text = wxString::Format( wxT("%i/%i" ), sc - ncsc, sc );
		} else {
			text = wxString::Format( wxT("%i"), sc );
		}
		
		if ( file->GetSrcA4AFCount() ) {
			text += wxString::Format( wxT("+%i"), file->GetSrcA4AFCount() );
		}

		text += wxString::Format( wxT(" (%i)"), file->GetTransferingSrcCount() );
		
		break;
	}

	// Priority
	case 7:
		text = PriorityToStr( file->GetDownPriority(), file->IsAutoDownPriority() );
		break;
			
	// File-status
	case 8:
		text = file->getPartfileStatus();
		break;
	
	// Remaining
	case 9: {
		if (	file->GetStatus() != PS_COMPLETING &&
			file->GetStatus() != PS_COMPLETE ) {
			// Size
			uint32 remains = file->GetFileSize() - file->GetCompletedSize();
			
			// time
			sint32 restTime = file->getTimeRemaining();
			text = CastSecondsToHM( restTime ) +
				wxT(" (") + CastItoXBytes( remains ) + wxT(")");
		}
		break;
	}
	
	// Last seen completed
	case 10: {
		if ( file->lastseencomplete ) {
			text = wxDateTime( file->lastseencomplete ).Format( _("%y/%m/%d %H:%M:%S") );
		} else {
			text = _("Unknown");
		}
		break;
	}
	
	// Laste received
	case 11: {
		if ( file->GetLastChangeDatetime().IsValid() ) {
			text = file->GetLastChangeDatetime().Format( _("%y/%m/%d %H:%M:%S") );
		} else {
			text = _("Unknown");
		}
	}
	} // switch

	if ( !text.IsEmpty() ) {
		dc->DrawText( text, rect.GetX(), rect.GetY() );
	}
}


void CDownloadListCtrl::DrawSourceItem(
	wxDC* dc, int nColumn, const wxRect& rect, CtrlItem_Struct* item ) const
{
	// Force clipper (clip 2 px more than the rectangle from the right side)
	wxDCClipper clipper( *dc, rect.GetX(), rect.GetY(), rect.GetWidth() - 2, rect.GetHeight() );
	wxString buffer;
	
	const CUpDownClient* client = (const CUpDownClient*)item->value;

	switch (nColumn) {
		// Client name + various icons
		case 0: {
			wxRect cur_rec = rect;
			// +3 is added by OnDrawItem()... so take it off
			// Kry - eMule says +1, so I'm trusting it
			wxPoint point( cur_rec.GetX(), cur_rec.GetY()+1 );

			if (item->type != A4AF_SOURCE) {
				uint8 image = 0;
				
				switch (client->GetDownloadState()) {
					case DS_CONNECTING:
					case DS_CONNECTED:
					case DS_WAITCALLBACK:
					case DS_TOOMANYCONNS:
						image = Client_Red_Smiley;
						break;
					case DS_ONQUEUE:
						if (client->IsRemoteQueueFull()) {
							image = Client_Grey_Smiley;
						} else {
							image = Client_Yellow_Smiley;
						}
						break;
					case DS_DOWNLOADING:
					case DS_REQHASHSET:
						image = Client_Green_Smiley;
						break;
					case DS_NONEEDEDPARTS:
					case DS_LOWTOLOWIP:
						image = Client_Grey_Smiley;
						break;
					default: // DS_NONE i.e.
						image = Client_White_Smiley;
					}

					m_ImageList.Draw(image, *dc, point.x, point.y,
						wxIMAGELIST_DRAW_TRANSPARENT);
				} else {
					m_ImageList.Draw(Client_Grey_Smiley, *dc, point.x, point.y,
						wxIMAGELIST_DRAW_TRANSPARENT);
				}

				cur_rec.x += 20;
				wxPoint point2( cur_rec.GetX(), cur_rec.GetY() + 1 );

				uint8 clientImage;
				
				if ( client->IsFriend() ) {
					clientImage = Client_Friend_Smiley;
				} else {
					switch ( client->GetClientSoft() ) {
						case SO_AMULE:
							clientImage = Client_aMule_Smiley;
							break;
						case SO_MLDONKEY:
						case SO_NEW_MLDONKEY:
						case SO_NEW2_MLDONKEY:
							clientImage = Client_mlDonkey_Smiley;
							break;
						case SO_EDONKEY:
						case SO_EDONKEYHYBRID:
							clientImage = Client_eDonkeyHybrid_Smiley;
							break;
						case SO_EMULE:
							clientImage = Client_eMule_Smiley;
							break;
						case SO_LPHANT:
							clientImage = Client_lphant_Smiley;
							break;
						case SO_SHAREAZA:
						case SO_NEW_SHAREAZA:
						case SO_NEW2_SHAREAZA:
							clientImage = Client_Shareaza_Smiley;
							break;
						case SO_LXMULE:
							clientImage = Client_xMule_Smiley;
							break;
						default:
							// cDonkey, Compatible, Unknown
							// No icon for those yet.
							// Using the eMule one + '?'
							clientImage = Client_Unknown;
							break;
					}
				}

				m_ImageList.Draw(clientImage, *dc, point2.x, point.y,
					wxIMAGELIST_DRAW_TRANSPARENT);

				if ( client->ExtProtocolAvailable() ) {
					// Ext protocol -> Draw the '+'
					m_ImageList.Draw(Client_ExtendedProtocol_Smiley, *dc, point2.x, point.y,
						wxIMAGELIST_DRAW_TRANSPARENT);
				}

				if (client->IsIdentified()) {
					// the 'v'
					m_ImageList.Draw(Client_SecIdent_Smiley, *dc, point2.x, point.y,
						wxIMAGELIST_DRAW_TRANSPARENT);					
				} else if (client->IsBadGuy()) {
					// the 'X'
					m_ImageList.Draw(Client_BadGuy_Smiley, *dc, point2.x, point.y,
						wxIMAGELIST_DRAW_TRANSPARENT);					
				}
							
				if ( client->GetUserName().IsEmpty() ) {
					dc->DrawText( wxT("?"), rect.GetX() + 40, rect.GetY() );
				} else {
					dc->DrawText( client->GetUserName(), rect.GetX() + 40,
						rect.GetY());
				}
			}
			break;

		case 3:	// completed
			if (item->type != A4AF_SOURCE && client->GetTransferedDown()) {
				buffer = CastItoXBytes(client->GetTransferedDown());
				dc->DrawText(buffer, rect.GetX(), rect.GetY());
			}
			break;

		case 4:	// speed
			if (item->type != A4AF_SOURCE && client->GetKBpsDown() > 0.001) {
				buffer = wxString::Format(wxT("%.1f "),
						client->GetKBpsDown()) + _("kB/s");
				dc->DrawText(buffer, rect.GetX(), rect.GetY());
			}
			break;

		case 5:	// file info
			if ( thePrefs::ShowProgBar() ) {
				int iWidth = rect.GetWidth() - 2;
				int iHeight = rect.GetHeight() - 2;
			
				if ( item->type != A4AF_SOURCE ) {
					uint32 dwTicks = GetTickCount();
					wxMemoryDC cdcStatus;

					if ( item->dwUpdated < dwTicks || !item->status || 
							iWidth != item->status->GetWidth() ) {
						
						if (item->status == NULL) {
							item->status = new wxBitmap(iWidth, iHeight);
						} else if ( item->status->GetWidth() != iWidth ) {
							// Only recreate if size has changed
							item->status->Create(iWidth, iHeight);
						}

						cdcStatus.SelectObject(*(item->status));

						if ( thePrefs::UseFlatBar() ) {
							DrawSourceStatusBar( client, &cdcStatus,
								wxRect(0, 0, iWidth, iHeight), true);
						} else {
							DrawSourceStatusBar( client, &cdcStatus,
								wxRect(1, 1, iWidth - 2, iHeight - 2), false);
				
							// Draw black border
							cdcStatus.SetPen( *wxBLACK_PEN );
							cdcStatus.SetBrush( *wxTRANSPARENT_BRUSH );
							cdcStatus.DrawRectangle( 0, 0, iWidth, iHeight );
						}
						
						// Plus ten seconds
						item->dwUpdated = dwTicks + 10000;
					} else {
						cdcStatus.SelectObject(*(item->status));
					}

					dc->Blit(rect.GetX(), rect.GetY() + 1, iWidth, iHeight, &cdcStatus, 0, 0);
					
					cdcStatus.SelectObject(wxNullBitmap);
				} else {
					wxString buffer = _("A4AF");
					
					int midx = (2*rect.GetX() + rect.GetWidth()) >> 1;
					int midy = (2*rect.GetY() + rect.GetHeight()) >> 1;
					
					wxCoord txtwidth, txtheight;
					
					dc->GetTextExtent(buffer, &txtwidth, &txtheight);
					
					dc->SetTextForeground(*wxBLACK);
					dc->DrawText(buffer, midx - (txtwidth >> 1), midy - (txtheight >> 1));

					// Draw black border
					dc->SetPen( *wxBLACK_PEN );
					dc->SetBrush( *wxTRANSPARENT_BRUSH );
					dc->DrawRectangle( rect.GetX(), rect.GetY() + 1, iWidth, iHeight );		
				}
			}
			break;

		case 6: {
				// Version
				dc->DrawText(client->GetClientVerString(), rect.GetX(), rect.GetY());
				break;
			}

		case 7:	// prio
			// We only show priority for sources actually queued for that file
			if (	item->type != A4AF_SOURCE &&
				client->GetDownloadState() == DS_ONQUEUE ) {
				if (client->IsRemoteQueueFull()) {
					buffer = _("Queue Full");
					dc->DrawText(buffer, rect.GetX(), rect.GetY());
				} else {
					if (client->GetRemoteQueueRank()) {
						sint16 qrDiff = client->GetRemoteQueueRank() -
							client->GetOldRemoteQueueRank();
						if(qrDiff == client->GetRemoteQueueRank() ) {
							qrDiff = 0;
						}
						wxColour savedColour = dc->GetTextForeground();
						if( qrDiff < 0 ) {
							dc->SetTextForeground(*wxBLUE);
						}
						if( qrDiff > 0 ) {
							dc->SetTextForeground(*wxRED);
						}
						//if( qrDiff == 0 ) {
						//	dc->SetTextForeground(*wxLIGHT_GREY);
						//}
						buffer = wxString::Format(_("QR: %u (%i)"),
							client->GetRemoteQueueRank(), qrDiff);
						dc->DrawText(buffer, rect.GetX(), rect.GetY());
						dc->SetTextForeground(savedColour);
					}
				}
			}
			break;

		case 8:	// status
			if (item->type != A4AF_SOURCE) {
				buffer = DownloadStateToStr( client->GetDownloadState(), 
					client->IsRemoteQueueFull() );
			} else {
				buffer = _("Asked for another file");
				if (	client->GetRequestFile() &&
					!client->GetRequestFile()->GetFileName().IsEmpty()) {
					buffer += wxT(" (") +
						client->GetRequestFile()->GetFileName() +
						wxT(")");
				}
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY());
			break;
		// Source comes from?
		case 9: {
			buffer = wxGetTranslation(OriginToText(client->GetSourceFrom()));
			dc->DrawText(buffer, rect.GetX(), rect.GetY());
			break;
		}	
	}
}


wxString CDownloadListCtrl::GetTTSText(unsigned item) const
{
	CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(item);
	
	if (content->type == FILE_TYPE) {
		CPartFile* file = (CPartFile*)content->value;

		return file->GetFileName();
	}

	return wxEmptyString;
}


int CDownloadListCtrl::SortProc(long param1, long param2, long sortData)
{
	CtrlItem_Struct* item1 = (CtrlItem_Struct*)param1;
	CtrlItem_Struct* item2 = (CtrlItem_Struct*)param2;

	int sortMod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	sortData &= CMuleListCtrl::COLUMN_MASK;
	int comp = 0;

	if ( item1->type == FILE_TYPE ) {
		if ( item2->type == FILE_TYPE ) {
			// Both are files, so we just compare them
			comp = Compare( (CPartFile*)item1->value, (CPartFile*)item2->value, sortData);
		} else {
			// A file and a source, checking if they belong to each other
			if ( item1->value == item2->owner ) {
				// A file should always be above its sources
				// Returning directly to avoid the modifier
				return -1;
			} else {
				// Source belongs to anther file, so we compare the files instead
				comp = Compare( (CPartFile*)item1->value, item2->owner, sortData);
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
				comp = Compare( item1->owner, (CPartFile*)item2->value, sortData);
			}
		} else {
			// Two sources, some different possibilites
			if ( item1->owner == item2->owner ) {
				// Avilable sources first, if we have both an
				// available and an unavailable
				comp = ( item2->type - item1->type );

				// Do we need to futher compare them? Happens if both have same type.
				if ( !comp ) {
					comp = Compare(
						(CUpDownClient*)item1->value,
						(CUpDownClient*)item2->value,
						sortData);
				}
			} else {
				// Belongs to different files, so we compare the files
				comp = Compare( item1->owner, item2->owner, sortData);
			}
		}
	}

	// We modify the result so that it matches with ascending or decending
	return sortMod * comp;
}


int CDownloadListCtrl::Compare( const CPartFile* file1, const CPartFile* file2, long lParamSort)
{
	int result = 0;

	switch (lParamSort) {
	// Sort by filename
	case 0:
		result = CmpAny(
			file1->GetFileName(),
			file2->GetFileName() );
		break;

	// Sort by size
	case 1:
		result = CmpAny(
			file1->GetFileSize(),
			file2->GetFileSize() );
		break;

	// Sort by transfered
	case 2:
		result = CmpAny(
			file1->GetTransfered(),
			file2->GetTransfered() );
		break;

	// Sort by completed
	case 3:
		result = CmpAny(
			file1->GetCompletedSize(),
			file2->GetCompletedSize() );
		break;

	// Sort by speed
	case 4:
		result = CmpAny(
			file1->GetKBpsDown() * 1024,
			file2->GetKBpsDown() * 1024 );
		break;

	// Sort by percentage completed
	case 5:
		result = CmpAny(
			file1->GetPercentCompleted(),
			file2->GetPercentCompleted() );
		break;

	// Sort by number of sources
	case 6:
		result = CmpAny(
			file1->GetSourceCount(),
			file2->GetSourceCount() );
		break;

	// Sort by priority
	case 7:
		result = CmpAny(
			file1->GetDownPriority(),
			file2->GetDownPriority() );
		break;

	// Sort by status
	case 8:
		result = CmpAny(
			file1->getPartfileStatusRang(),
			file2->getPartfileStatusRang() );
		break;

	// Sort by remaining time
	case 9:
		if (file1->getTimeRemaining() == -1) {
			if (file2->getTimeRemaining() == -1) {
				result = 0;
			} else {
				result = -1;
			}
		} else {
			if (file2->getTimeRemaining() == -1) {
				result = 1;
			} else {
				result = CmpAny(
					file1->getTimeRemaining(),
					file2->getTimeRemaining() );
			}
		}
		break;

	// Sort by last seen complete
	case 10:
		result = CmpAny(
			file1->lastseencomplete,
			file2->lastseencomplete );
		break;

	// Sort by last reception
	case 11:
		if (!file1->GetLastChangeDatetime().IsValid()) {			
			result = -1;
		} else if (!file2->GetLastChangeDatetime().IsValid()) {
			result = 1;
		} else {
			result = CmpAny(
				file1->GetLastChangeDatetime(),
				file2->GetLastChangeDatetime() );
		}
		break;
	}


	// We cannot have that two files are equal, since that will screw up
	// the placement of sources. So if they are equal, we first try to use the
	// last sort-type and then fall back on something that is bound to be unique
	// and will give a consistant result: Their hashes.
	if ( !result ) {
		// Try to sort by the last column
		if ( s_lastColumn != lParamSort ) {
			result = s_lastOrder * Compare( file1, file2, s_lastColumn );
		} else {
			// If that failed as well, then we sort by hash
			result = CmpAny(
				file1->GetFileHash(),
				file2->GetFileHash() );
		}
	}

	return result;
}


int CDownloadListCtrl::Compare(
	const CUpDownClient* client1, const CUpDownClient* client2, long lParamSort)
{
	switch (lParamSort) {
		// Sort by name
		case 0:
			return CmpAny( client1->GetUserName(), client2->GetUserName() );
	
		// Sort by status (size field)
		case 1:
			return CmpAny( client1->GetDownloadState(), client2->GetDownloadState() );
	
		// Sort by transfered in the following fields
		case 2:	
		case 3:
			return CmpAny( client1->GetTransferedDown(), client2->GetTransferedDown() );

		// Sort by speed
		case 4:
			return CmpAny( client1->GetKBpsDown(), client2->GetKBpsDown() );
		
		// Sort by parts offered (Progress field)
		case 5:
			return CmpAny(
				client1->GetAvailablePartCount(),
				client2->GetAvailablePartCount() );
		
		// Sort by client version
		case 6: {
			if ( client1->GetClientSoft() == client2->GetClientSoft() ) {
				if (client1->IsEmuleClient()) {
					return CmpAny(
						client2->GetMuleVersion(),
						client1->GetMuleVersion() );
				} else {
					return CmpAny(
						client2->GetVersion(),
						client1->GetVersion() );
				}
			} else {
				return CmpAny(
					client1->GetClientSoft(),
					client2->GetClientSoft() );
			}
		}
		
		// Sort by Queue-Rank
		case 7: {
			// This will sort by download state: Downloading, OnQueue, Connecting ...
			// However, Asked For Another will always be placed last, due to
			// sorting in SortProc
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
						return CmpAny(
							client1->GetRemoteQueueRank(),
							client2->GetRemoteQueueRank() );
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
		case 8: {
			if (client1->GetDownloadState() == client2->GetDownloadState()) {
				return CmpAny(
					client1->IsRemoteQueueFull(),
					client2->IsRemoteQueueFull() );
			} else {
				return CmpAny(
					client1->GetDownloadState(),
					client2->GetDownloadState() );
			}
		}

		// Source of source ;)
		case 9:
			return CmpAny(client1->GetSourceFrom(), client2->GetSourceFrom());
		
		default:
			return 0;
	}
}


void CDownloadListCtrl::ClearCompleted()
{
	m_completedFiles = false;
	CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(false);
	
	// Search for completed files
	for ( ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ) {
		CtrlItem_Struct* item = it->second; ++it;
		
		if ( item->type == FILE_TYPE ) {
			CPartFile* file = (CPartFile*)item->value;
			
			if ( file->IsPartFile() == false ) {
				RemoveFile(file);
			}
		}
	}
}


void CDownloadListCtrl::ShowFilesCount( int diff )
{
	m_filecount += diff;
	
	wxString str = wxString::Format( _("Downloads (%i)"), m_filecount );
	wxStaticText* label = CastByName( wxT("downloadsLabel"), GetParent(), wxStaticText );

	label->SetLabel( str );
	label->GetParent()->Layout();
}




bool CDownloadListCtrl::ShowItemInCurrentCat(
	const CPartFile* file, int newsel ) const
{
	return 
		((newsel == 0 && !thePrefs::ShowAllNotCats()) ||
		 (newsel == 0 && thePrefs::ShowAllNotCats() && file->GetCategory() == 0)) ||
		(newsel > 0 && newsel == file->GetCategory());
}



void CDownloadListCtrl::DrawFileStatusBar(
	const CPartFile* file, wxDC* dc, const wxRect& rect, bool bFlat ) const
{
	static CBarShader s_ChunkBar(16);
	
	COLORREF crHave		= ( bFlat ? RGB(   0,   0,   0 ) : RGB( 104, 104, 104 ) );
	COLORREF crPending	= ( bFlat ? RGB( 255, 255, 100 ) : RGB( 255, 208,   0 ) );
	COLORREF crProgress	= ( bFlat ? RGB(   0, 150,   0 ) : RGB(   0, 224,   0 ) );
	COLORREF crMissing	= RGB(255, 0, 0);

	s_ChunkBar.SetHeight(rect.height);
	s_ChunkBar.SetWidth(rect.width); 
	s_ChunkBar.SetFileSize( file->GetFileSize() );
	s_ChunkBar.Fill( crHave );
	s_ChunkBar.Set3dDepth( thePrefs::Get3DDepth() );


	if ( file->GetStatus() == PS_COMPLETE || file->GetStatus() == PS_COMPLETING ) {
		s_ChunkBar.Fill( crProgress );
		s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 
		return;
	}

	
	// Part availability ( of missing parts )
	const CList<Gap_Struct*>& gaplist = file->GetGapList();
	for ( POSITION pos = gaplist.GetHeadPosition(); pos; ) {
		Gap_Struct* gap = gaplist.GetNext( pos );

		// Start position
		uint32 start = ( gap->start / PARTSIZE );
		// End position
		uint32 end   = ( gap->end / PARTSIZE ) + 1;

		// Avoid going past the filesize. Dunno if this can happen, but the old code did check.
		if ( end > file->GetPartCount() )
			end = file->GetPartCount();

		// Place each gap, one PART at a time
		for ( uint32 i = start; i < end; ++i ) {
			COLORREF color;
			if ( i < file->m_SrcpartFrequency.GetCount() && file->m_SrcpartFrequency[i]) {
				int blue = 210 - ( 22 * ( file->m_SrcpartFrequency[i] - 1 ) );
				color = RGB( 0, ( blue < 0 ? 0 : blue ), 255 );
			} else {
				color = crMissing;
			}	

			if ( file->IsStopped() ) {
				color = DarkenColour( color, 2 );
			}
			
			uint32 gap_begin = ( i == start   ? gap->start : PARTSIZE * i );
			uint32 gap_end   = ( i == end - 1 ? gap->end   : PARTSIZE * ( i + 1 ) );
		
			s_ChunkBar.FillRange( gap_begin, gap_end,  color);
		}
	}
	
	
	// Pending parts
	const CList<Requested_Block_Struct*>& requestedblocks_list = file->GetRequestedBlockList();
	for ( POSITION pos = requestedblocks_list.GetHeadPosition(); pos; ) {
		COLORREF color = ( file->IsStopped() ? DarkenColour( crPending, 2 ) : crPending );
		
		Requested_Block_Struct* block = requestedblocks_list.GetNext( pos );
		s_ChunkBar.FillRange( block->StartOffset, block->EndOffset, color );
	}


	// Draw the progress-bar
	s_ChunkBar.Draw( dc, rect.x, rect.y, bFlat );

	
	// Green progressbar width
	int width = (int)(( (float)rect.width / (float)file->GetFileSize() ) *
			file->GetCompletedSize() );

	if ( bFlat ) {
		dc->SetBrush( wxBrush( crProgress, wxSOLID ) );
		
		dc->DrawRectangle( rect.x, rect.y, width, 3 );
	} else {
		// Draw the two black lines for 3d-effect
		dc->SetPen( wxPen( wxColour( 0, 0, 0 ), 1, wxSOLID ) );
		dc->DrawLine( rect.x, rect.y + 0, rect.x + width, rect.y + 0 );
		dc->DrawLine( rect.x, rect.y + 2, rect.x + width, rect.y + 2 );
		
		// Draw the green line
		dc->SetPen( wxPen( crProgress, 1, wxSOLID ) );
		dc->DrawLine( rect.x, rect.y + 1, rect.x + width, rect.y + 1 );
	}
}


void CDownloadListCtrl::DrawSourceStatusBar(
	const CUpDownClient* source, wxDC* dc, const wxRect& rect, bool bFlat) const
{
	static CBarShader s_StatusBar(16);

	uint32 crBoth		= ( bFlat ? RGB(   0, 150,   0 ) : RGB(   0, 192,   0 ) );
	uint32 crNeither		= ( bFlat ? RGB( 224, 224, 224 ) : RGB( 240, 240, 240 ) );
	uint32 crClientOnly	= ( bFlat ? RGB(   0,   0,   0 ) : RGB( 104, 104, 104 ) );
	uint32 crPending		= ( bFlat ? RGB( 255, 208,   0 ) : RGB( 255, 208,   0 ) );
	uint32 crNextPending	= ( bFlat ? RGB( 255, 255, 100 ) : RGB( 255, 255, 100 ) );

	CPartFile* reqfile = source->GetRequestFile();

	s_StatusBar.SetFileSize( reqfile->GetFileSize() );
	s_StatusBar.SetHeight(rect.height);
	s_StatusBar.SetWidth(rect.width);
	s_StatusBar.Fill(crNeither);
	s_StatusBar.Set3dDepth( thePrefs::Get3DDepth() );

	// Barry - was only showing one part from client, even when reserved bits from 2 parts
	wxString gettingParts = source->ShowDownloadingParts();

	const BitVector& partStatus = source->GetPartStatus();

	for ( uint32 i = 0; i < partStatus.size(); i++ ) {
		if ( partStatus[i]) {
			uint32 uEnd;
			if (PARTSIZE*(i+1) > reqfile->GetFileSize()) {
				uEnd = reqfile->GetFileSize();
			} else {
				uEnd = PARTSIZE*(i+1);
			}
			
			uint32 color = 0;
			if ( reqfile->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)) {
				color = crBoth;
			} else if (	source->GetDownloadState() == DS_DOWNLOADING &&
					source->GetLastBlockOffset() < uEnd &&
					source->GetLastBlockOffset() >= PARTSIZE*i) {
				color = crPending;
			} else if (gettingParts.GetChar((uint16)i) == 'Y') {
				color = crNextPending;
			} else {
				color = crClientOnly;
			}

			if ( source->GetRequestFile()->IsStopped() ) {
				color = DarkenColour( color, 2 );
			}

			s_StatusBar.FillRange( PARTSIZE*i, uEnd, color );
		}
	}

	s_StatusBar.Draw(dc, rect.x, rect.y, bFlat);
}


void CDownloadListCtrl::PreviewFile(CPartFile* file)
{
	wxString command;
	// If no player set in preferences, use mplayer.
	// And please, do a warning also :P
	if (thePrefs::GetVideoPlayer().IsEmpty()) {
		wxMessageBox(_(
			"Please set your prefered video player on preferences.\n"
			"Meanwhile, aMule will attempt to use mplayer"
			" and you will get this warning on every preview"),
			_("File preview"), wxOK);
		// Since newer versions for some reason mplayer does not automatically
		// select video output decivce and needs a parameter, go figure...
		command = wxT("xterm -T \"aMule Preview\" -iconic -e mplayer");
	} else {
		command = thePrefs::GetVideoPlayer();
	}
	// Need to use quotes in case filename contains spaces.
	command.Append(wxT(" \""));
	command += file->GetFullName();
	if (file->GetStatus() != PS_COMPLETE) {
		// Remove the .met
		command = command.BeforeLast( wxT('.') );
	}
	command += wxT("\"");

	// We can't use wxShell here, it blocks the app
	// <jacobo221> mplayer users (like me) should use xterm -T "aMule Preview" -iconic -e mplayer -idx
	if (!wxExecute(command,wxEXEC_ASYNC)) {
		AddLogLineM( true, _("ERROR: Failed to execute external media-player!") );
		AddLogLineM( false, CFormat( _("Command: %s") ) % command );
	}
}
