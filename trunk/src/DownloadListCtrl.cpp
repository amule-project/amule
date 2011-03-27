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

#include "DownloadListCtrl.h"	// Interface declarations

#include <protocol/ed2k/ClientSoftware.h>
#include <common/MenuIDs.h>

#include <common/Format.h>	// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "BarShader.h"		// Needed for CBarShader
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "DataToText.h"		// Needed for PriorityToStr
#include "DownloadQueue.h"
#include "FileDetailDialog.h"	// Needed for CFileDetailDialog
#include "GuiEvents.h"		// Needed for CoreNotify_*
#include "Logger.h"
#include "muuli_wdr.h"		// Needed for ID_DLOADLIST
#include "PartFile.h"		// Needed for CPartFile
#include "Preferences.h"
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "TerminationProcess.h"	// Needed for CTerminationProcess
#include "TransferWnd.h"
#include "SourceListCtrl.h"

class CPartFile;


struct FileCtrlItem_Struct
{
	FileCtrlItem_Struct()
		: dwUpdated(0),
		  status(NULL),
		  m_fileValue(NULL)
	{ }
	
	~FileCtrlItem_Struct() {
		delete status;
	}

	CPartFile* GetFile() const {
		return m_fileValue;
	}

	void SetContents(CPartFile* file) {
		m_fileValue = file;
	}
	
	uint32		dwUpdated;
	wxBitmap*	status;

private:
	CPartFile*			m_fileValue;
};

#define m_ImageList theApp->amuledlg->m_imagelist

enum ColumnEnum {
	ColumnPart = 0,
	ColumnFileName,
	ColumnSize,
	ColumnTransferred,
	ColumnCompleted,
	ColumnSpeed,
	ColumnProgress,
	ColumnSources,
	ColumnPriority,
	ColumnStatus,
	ColumnTimeRemaining,
	ColumnLastSeenComplete,
	ColumnLastReception,
	ColumnNumberOfColumns
};
	
BEGIN_EVENT_TABLE(CDownloadListCtrl, CMuleListCtrl)
	EVT_LIST_ITEM_ACTIVATED(ID_DLOADLIST,	CDownloadListCtrl::OnItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnMouseRightClick)
	EVT_LIST_ITEM_MIDDLE_CLICK(ID_DLOADLIST, CDownloadListCtrl::OnMouseMiddleClick)
	EVT_LIST_ITEM_SELECTED(ID_DLOADLIST, CDownloadListCtrl::OnItemSelectionChanged)
	EVT_LIST_ITEM_DESELECTED(ID_DLOADLIST, CDownloadListCtrl::OnItemSelectionChanged)

	EVT_CHAR( CDownloadListCtrl::OnKeyPressed )

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

	EVT_MENU( MP_GETMAGNETLINK,		CDownloadListCtrl::OnGetLink )
	EVT_MENU( MP_GETED2KLINK,		CDownloadListCtrl::OnGetLink )

	EVT_MENU( MP_METINFO,			CDownloadListCtrl::OnViewFileInfo )
	EVT_MENU( MP_VIEW,			CDownloadListCtrl::OnPreviewFile )
	EVT_MENU( MP_VIEWFILECOMMENTS,		CDownloadListCtrl::OnViewFileComments )

	EVT_MENU( MP_WS,			CDownloadListCtrl::OnGetFeedback )

END_EVENT_TABLE()

//! This listtype is used when gathering the selected items.
typedef std::list<FileCtrlItem_Struct*>	ItemList;

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

	m_hilightBrush  = CMuleColour(wxSYS_COLOUR_HIGHLIGHT).Blend(125).GetBrush();

	m_hilightUnfocusBrush = CMuleColour(wxSYS_COLOUR_BTNSHADOW).Blend(125).GetBrush();

	InsertColumn( ColumnPart,			_("Part"),					wxLIST_FORMAT_LEFT,  30, wxT("a") );
	InsertColumn( ColumnFileName,		_("File Name"),				wxLIST_FORMAT_LEFT, 260, wxT("N") );
	InsertColumn( ColumnSize,			_("Size"),					wxLIST_FORMAT_LEFT,  60, wxT("Z") );
	InsertColumn( ColumnTransferred,	_("Transferred"),			wxLIST_FORMAT_LEFT,  65, wxT("T") );
	InsertColumn( ColumnCompleted,		_("Completed"),				wxLIST_FORMAT_LEFT,  65, wxT("C") );
	InsertColumn( ColumnSpeed,			_("Speed"),					wxLIST_FORMAT_LEFT,  65, wxT("S") );
	InsertColumn( ColumnProgress,		_("Progress"),				wxLIST_FORMAT_LEFT, 170, wxT("P") );
	InsertColumn( ColumnSources,		_("Sources"),				wxLIST_FORMAT_LEFT,  50, wxT("u") );
	InsertColumn( ColumnPriority,		_("Priority"),				wxLIST_FORMAT_LEFT,  55, wxT("p") );
	InsertColumn( ColumnStatus,			_("Status"),				wxLIST_FORMAT_LEFT,  70, wxT("s") );
	InsertColumn( ColumnTimeRemaining,  _("Time Remaining"),		wxLIST_FORMAT_LEFT, 110, wxT("r") );
	InsertColumn( ColumnLastSeenComplete, _("Last Seen Complete"),	wxLIST_FORMAT_LEFT, 220, wxT("c") );
	InsertColumn( ColumnLastReception,	_("Last Reception"),		wxLIST_FORMAT_LEFT, 220, wxT("R") );

	m_category = 0;
	m_filecount = 0;
	LoadSettings();
	
	//m_ready = true;
}

// This is the order the columns had before extendable list-control settings save/load code was introduced.
// Don't touch when inserting new columns!
wxString CDownloadListCtrl::GetOldColumnOrder() const
{
	return wxT("N,Z,T,C,S,P,u,p,s,r,c,R");
}

CDownloadListCtrl::~CDownloadListCtrl()
{
	while ( !m_ListItems.empty() ) {
		delete m_ListItems.begin()->second;
		m_ListItems.erase( m_ListItems.begin() );
	}
}

void CDownloadListCtrl::AddFile( CPartFile* file )
{
	wxASSERT( file );
	
	// Avoid duplicate entries of files
	if ( m_ListItems.find( file ) == m_ListItems.end() ) {
		FileCtrlItem_Struct* newitem = new FileCtrlItem_Struct;
		newitem->SetContents(file);
	
		m_ListItems.insert( ListItemsPair( file, newitem ) );
		
		// Check if the new file is visible in the current category
		if ( file->CheckShowItemInGivenCat( m_category ) ) {
			ShowFile( file, true );
			if (file->IsCompleted()) {
				CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(true);
			}
			SortList();
		}
	}
}

void CDownloadListCtrl::RemoveFile( CPartFile* file )
{
	wxASSERT( file );
	
	// Ensure that any list-entries are removed
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
		FileCtrlItem_Struct* item = it->second;

		long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(item) );

		// Determine if the file should be shown in the current category

		CPartFile* file = item->GetFile();
	
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

		if (file->IsCompleted() && show) {
			CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(true);
		}
	}
}


void CDownloadListCtrl::ShowFile( CPartFile* file, bool show )
{
	wxASSERT( file );
	
	ListItems::iterator it = m_ListItems.find( file );
	
	if ( it != m_ListItems.end() ) { 
		FileCtrlItem_Struct* item = it->second;
	
		if ( show ) {
			// Check if the file is already being displayed
			long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(item) );
			if ( index == -1 ) {	
				long newitem = InsertItem( GetItemCount(), wxEmptyString );
				
				SetItemPtrData( newitem, reinterpret_cast<wxUIntPtr>(item) );

				wxListItem myitem;
				myitem.m_itemId = newitem;
				myitem.SetBackgroundColour( GetBackgroundColour() );
				
				SetItem(myitem);	
			
				RefreshItem( newitem );

				ShowFilesCount( 1 );
			}
		} else {
			// Try to find the file and remove it
			long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(item) );
			if ( index > -1 ) {
				DeleteItem( index );
				ShowFilesCount( -1 );
			}
		}
	}
}

void CDownloadListCtrl::ChangeCategory( int newCategory )
{
	Freeze();

	bool hasCompletedDownloads = false;

	// remove all displayed files with a different cat and show the correct ones
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++) {
		
		CPartFile* file =  it->second->GetFile();

		bool curVisibility = file->CheckShowItemInGivenCat( m_category );
		bool newVisibility = file->CheckShowItemInGivenCat( newCategory );

		if (newVisibility && file->IsCompleted()) {
			hasCompletedDownloads = true;
		}
	
		// Check if the visibility of the file has changed. However, if the
		// current category is the default (0) category, then we can't use
		// curVisiblity to see if the visibility has changed but instead
		// have to let ShowFile() check if the file is or isn't on the list.
		if ( curVisibility != newVisibility || !newCategory ) {
			ShowFile( file, newVisibility );
		}
	}
	
	CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(hasCompletedDownloads);

	Thaw();

	m_category = newCategory;
}


uint8 CDownloadListCtrl::GetCategory() const
{
	return m_category;
}


/**
 * Helper-function: This function is used to gather selected items.
 *
 * @param list A pointer to the list to gather items from.
 * @return A list containing the selected items.
 */
ItemList GetSelectedItems( CDownloadListCtrl* list)
{
	ItemList results;

	long index = list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while ( index > -1 ) {
		FileCtrlItem_Struct* item = (FileCtrlItem_Struct*)list->GetItemData( index );
		results.push_back( item );
		
		index = list->GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	return results;
}


void CDownloadListCtrl::OnCancelFile(wxCommandEvent& WXUNUSED(event))
{
	ItemList files = ::GetSelectedItems(this);
	for (ItemList::iterator it = files.begin(); it != files.end(); ) {
		ItemList::iterator it1 = it++;
		CPartFile* file = (*it1)->GetFile();
		if (file) {
			switch (file->GetStatus()) {
				case PS_WAITINGFORHASH:
				case PS_HASHING:
				case PS_COMPLETING:
				case PS_COMPLETE:
					files.erase(it1);
					break;
			}
		}
	}
	if (files.size()) {	
		wxString question;
		if (files.size() == 1) {
			question = _("Are you sure that you wish to delete the selected file?");
		} else {
			question = _("Are you sure that you wish to delete the selected files?");
		}
		if (wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO, this) == wxYES) {
			for (ItemList::iterator it = files.begin(); it != files.end(); ++it) {
				CPartFile* file = (*it)->GetFile();
				if (file) {
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
			wxFAIL;
	}

	ItemList files = ::GetSelectedItems( this );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (*it)->GetFile();
	
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
	ItemList files = ::GetSelectedItems( this );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (*it)->GetFile();

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
	ItemList files = ::GetSelectedItems( this );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CoreNotify_PartFile_SetCat( (*it)->GetFile(), event.GetId() - MP_ASSIGNCAT );
		ShowFile((*it)->GetFile(), false);
	}
	wxListEvent ev;
	OnItemSelectionChanged(ev);	// clear clients that may have been shown

	ChangeCategory( m_category );	// This only updates the visibility of the clear completed button
	theApp->amuledlg->m_transferwnd->UpdateCatTabTitles();
}


void CDownloadListCtrl::OnSetStatus( wxCommandEvent& event )
{
	ItemList files = ::GetSelectedItems( this );

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (*it)->GetFile();

		switch ( event.GetId() ) {	
			case MP_PAUSE:
				CoreNotify_PartFile_Pause( file );
				break;
				
			case MP_RESUME:
				CoreNotify_PartFile_Resume( file );
				break;

			case MP_STOP:
				CoreNotify_PartFile_Stop( file );
				break;
		}
	}
}


void CDownloadListCtrl::OnClearCompleted( wxCommandEvent& WXUNUSED(event) )
{
	ClearCompleted();
}


void CDownloadListCtrl::OnGetLink(wxCommandEvent& event)
{
	ItemList files = ::GetSelectedItems( this );

	wxString URIs;

	for ( ItemList::iterator it = files.begin(); it != files.end(); ++it ) {
		CPartFile* file = (*it)->GetFile();

		if ( event.GetId() == MP_GETED2KLINK ) {
			URIs += theApp->CreateED2kLink( file ) + wxT("\n");
		} else {
			URIs += theApp->CreateMagnetLink( file ) + wxT("\n");
		}
	}

	if ( !URIs.IsEmpty() ) {
		theApp->CopyTextToClipboard( URIs.BeforeLast(wxT('\n')) );
	}
}


void CDownloadListCtrl::OnGetFeedback(wxCommandEvent& WXUNUSED(event))
{
	wxString feed;
	ItemList files = ::GetSelectedItems( this );

	for (ItemList::iterator it = files.begin(); it != files.end(); ++it) {
		if (feed.IsEmpty()) {
			feed = CFormat(_("Feedback from: %s (%s)\n\n")) % thePrefs::GetUserNick() % theApp->GetFullMuleVersion();
		} else {
			feed += wxT("\n");
		}
		feed += (*it)->GetFile()->GetFeedback();
	}

	if (!feed.IsEmpty()) {
		theApp->CopyTextToClipboard(feed);
	}
}

void CDownloadListCtrl::OnViewFileInfo( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this );

	if ( files.size() == 1 ) {
		CFileDetailDialog dialog( this, files.front()->GetFile() );
		dialog.ShowModal();
	}
}


void CDownloadListCtrl::OnViewFileComments( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this );

	if ( files.size() == 1 ) {
		CCommentDialogLst dialog( this, files.front()->GetFile() );
		dialog.ShowModal();
	}
}

void CDownloadListCtrl::OnPreviewFile( wxCommandEvent& WXUNUSED(event) )
{
	ItemList files = ::GetSelectedItems( this );

	if ( files.size() == 1 ) {
		PreviewFile(files.front()->GetFile());
	}
}

void CDownloadListCtrl::OnItemActivated( wxListEvent& evt )
{	
	CPartFile* file = ((FileCtrlItem_Struct*)GetItemData( evt.GetIndex()))->GetFile();

	if ((!file->IsPartFile() || file->IsCompleted()) && file->PreviewAvailable()) {
		PreviewFile( file );
	}	
}

void CDownloadListCtrl::OnItemSelectionChanged( wxListEvent& )
{	
	if (!IsSorting()) {
		CKnownFileVector filesVector;
		filesVector.reserve(GetSelectedItemCount());

		long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		
		while ( index > -1 ) {
			CPartFile* file = ((FileCtrlItem_Struct*)GetItemData( index ))->GetFile();
			if (file->IsPartFile()) {
				filesVector.push_back(file);
			}
			index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		}
		
		std::sort(filesVector.begin(), filesVector.end());
		theApp->amuledlg->m_transferwnd->clientlistctrl->ShowSources(filesVector);
	}
}

void CDownloadListCtrl::OnMouseRightClick(wxListEvent& evt)
{
	long index = CheckSelection(evt);
	if (index < 0) {
		return;
	}	
	
	delete m_menu;
	m_menu = NULL;

	FileCtrlItem_Struct* item = (FileCtrlItem_Struct*)GetItemData( index );
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
	m_menu->Append(MP_MENU_EXTD,
		_("Extended Options"), extendedmenu);
	//-----------------------------------------------------
	m_menu->AppendSeparator();
	//-----------------------------------------------------

	m_menu->Append(MP_VIEW, _("Preview"));
	m_menu->Append(MP_METINFO, _("Show file &details"));
	m_menu->Append(MP_VIEWFILECOMMENTS, _("Show all comments"));
	//-----------------------------------------------------
	m_menu->AppendSeparator();
	//-----------------------------------------------------
	m_menu->Append(MP_GETMAGNETLINK,
		_("Copy magnet URI to clipboard"));
	m_menu->Append(MP_GETED2KLINK,
		_("Copy eD2k &link to clipboard"));
	m_menu->Append(MP_WS,
		_("Copy feedback to clipboard"));
	//-----------------------------------------------------
	m_menu->AppendSeparator();
	//-----------------------------------------------------	
	// Add dynamic entries
	wxMenu *cats = new wxMenu(_("Category"));
	if (theApp->glob_prefs->GetCatCount() > 1) {
		for (uint32 i = 0; i < theApp->glob_prefs->GetCatCount(); i++) {
			if ( i == 0 ) {
				cats->Append( MP_ASSIGNCAT, _("unassign") );
			} else {
				cats->Append( MP_ASSIGNCAT + i,
					theApp->glob_prefs->GetCategory(i)->title );
			}
		}
	}
	m_menu->Append(MP_MENU_CATS, _("Assign to category"), cats);
	m_menu->Enable(MP_MENU_CATS, (theApp->glob_prefs->GetCatCount() > 1) );

	CPartFile* file = item->GetFile();
	// then set state
	bool canStop;
	bool canPause;
	bool canCancel;
	bool fileResumable;
	if (file->GetStatus(true) != PS_ALLOCATING) {
		const uint8_t fileStatus = file->GetStatus();
		canStop =
			(fileStatus != PS_ERROR) &&
			(fileStatus != PS_COMPLETE) &&
			(file->IsStopped() != true);
		canPause = (file->GetStatus() != PS_PAUSED) && canStop;
		fileResumable =
			(fileStatus == PS_PAUSED) ||
			(fileStatus == PS_ERROR) ||
			(fileStatus == PS_INSUFFICIENT);
		canCancel = fileStatus != PS_COMPLETE;
	} else {
		canStop = canPause = canCancel = fileResumable = false;
	}

	m_menu->Enable( MP_CANCEL,	canCancel );
	m_menu->Enable( MP_PAUSE,	canPause );
	m_menu->Enable( MP_STOP,	canStop );
	m_menu->Enable( MP_RESUME, 	fileResumable );
	m_menu->Enable( MP_CLEARCOMPLETED, CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->IsEnabled() );

	wxString view;
	if (file->IsPartFile() && !file->IsCompleted()) {
		view = CFormat(wxT("%s [%s]")) % _("Preview")
				% file->GetPartMetFileName().RemoveExt();
	} else if ( file->IsCompleted() ) {
		view = _("&Open the file");
	}
	m_menu->SetLabel(MP_VIEW, view);
	m_menu->Enable(MP_VIEW, file->PreviewAvailable());

	FileRatingList ratingList;
	item->GetFile()->GetRatingAndComments(ratingList);
	m_menu->Enable(MP_VIEWFILECOMMENTS, !ratingList.empty());

	m_menu->Check(  MP_SWAP_A4AF_TO_THIS_AUTO, 	file->IsA4AFAuto() );

	int priority = file->IsAutoDownPriority() ? PR_AUTO : file->GetDownPriority();
	
	priomenu->Check( MP_PRIOHIGH,	priority == PR_HIGH );
	priomenu->Check( MP_PRIONORMAL, priority == PR_NORMAL );
	priomenu->Check( MP_PRIOLOW,	priority == PR_LOW );
	priomenu->Check( MP_PRIOAUTO,	priority == PR_AUTO );

	m_menu->Enable( MP_MENU_EXTD, canPause );

	bool autosort = thePrefs::AutoSortDownload(false);
	PopupMenu(m_menu, evt.GetPoint());
	thePrefs::AutoSortDownload(autosort);

	delete m_menu;
	m_menu = NULL;
}


void CDownloadListCtrl::OnMouseMiddleClick(wxListEvent& evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long index = CheckSelection(evt);
	if ( index < 0 ) {
		return;
	}

	bool autosort = thePrefs::AutoSortDownload(false);
	CFileDetailDialog(this, ((FileCtrlItem_Struct*)GetItemData( index ))->GetFile()).ShowModal();
	thePrefs::AutoSortDownload(autosort);
}


void CDownloadListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// Check if delete was pressed
	switch (event.GetKeyCode()) {
		case WXK_NUMPAD_DELETE:
		case WXK_DELETE: {
			wxCommandEvent evt;
			OnCancelFile( evt );
			break;
		}
		case WXK_F2: {
			ItemList files = ::GetSelectedItems( this );
			if (files.size() == 1) {	
				CPartFile* file = files.front()->GetFile();
				
				// Currently renaming of completed files causes problem with kad
				if (file->IsPartFile()) {
					wxString strNewName = ::wxGetTextFromUser(
						_("Enter new name for this file:"),
						_("File rename"), file->GetFileName().GetPrintable());
				
					CPath newName = CPath(strNewName);
					if (newName.IsOk() && (newName != file->GetFileName())) {
						theApp->sharedfiles->RenameFile(file, newName);
					}
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
	if ( !theApp->amuledlg->IsDialogVisible( CamuleDlg::DT_TRANSFER_WND ) ) {
		return;
	}

	FileCtrlItem_Struct* content = (FileCtrlItem_Struct *)GetItemData(item);

	// Define text-color and background
	// and the border of the drawn area
	if (highlighted) {
		CMuleColour colour;
		if (GetFocus()) {
			dc->SetBackground(m_hilightBrush);
			colour = m_hilightBrush.GetColour();
		} else {
			dc->SetBackground(m_hilightUnfocusBrush);
			colour = m_hilightUnfocusBrush.GetColour();
		}
		dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		dc->SetPen( colour.Blend(65).GetPen() );
	} else {
		dc->SetBackground(*(wxTheBrushList->FindOrCreateBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX), wxSOLID)));
		dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		dc->SetPen(*wxTRANSPARENT_PEN);
	}
	dc->SetBrush( dc->GetBackground() );
	
	dc->DrawRectangle( rectHL.x, rectHL.y, rectHL.width, rectHL.height );

	dc->SetPen(*wxTRANSPARENT_PEN);

	if (!highlighted || !GetFocus() ) {
		// If we have category, override textforeground with what category tells us.
		CPartFile *file = content->GetFile();
		if ( file->GetCategory() ) {
			dc->SetTextForeground(CMuleColour(theApp->glob_prefs->GetCatColor(file->GetCategory())) );
		}
	}

	// Various constant values we use
	const int iTextOffset = ( rect.GetHeight() - dc->GetCharHeight() ) / 2;
	const int iOffset = 4;

	wxRect cur_rec( iOffset, rect.y, 0, rect.height );
	for (int i = 0; i < GetColumnCount(); i++) {
		wxListItem listitem;
		GetColumn(i, listitem);

		if (listitem.GetWidth() > 2*iOffset) {
			cur_rec.width = listitem.GetWidth() - 2*iOffset;

			// Make a copy of the current rectangle so we can apply specific tweaks
			wxRect target_rec = cur_rec;
			if ( i == ColumnProgress ) {
				// Double the offset to make room for the cirle-marker
				target_rec.x += iOffset;
				target_rec.width -= iOffset;
			} else {
				// will ensure that text is about in the middle ;)
				target_rec.y += iTextOffset;
			}

			// Draw the item
			DrawFileItem(dc, i, target_rec, content);
		}
		
		// Increment to the next column
		cur_rec.x += listitem.GetWidth();
	}
}


void CDownloadListCtrl::DrawFileItem( wxDC* dc, int nColumn, const wxRect& rect, FileCtrlItem_Struct* item ) const
{
	wxDCClipper clipper( *dc, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight() );

	const CPartFile* file = item->GetFile();

	// Used to contain the contenst of cells that dont need any fancy drawing, just text.
	wxString text;

	switch (nColumn) {
		// Part Number
		case ColumnPart: {
			if (file->IsPartFile() && !file->IsCompleted()) {
			  text = CFormat(wxT("%03d")) % file->GetPartMetNumber();
			}
			break;
		}
		// Filename
		case ColumnFileName: {
			wxString filename = file->GetFileName().GetPrintable();
	
			if (file->HasRating() || file->HasComment()) {
				int image = Client_CommentOnly_Smiley;
				if (file->HasRating()) {
					image = Client_InvalidRating_Smiley + file->UserRating() - 1;
				}	
					
				wxASSERT(image >= Client_InvalidRating_Smiley);
				wxASSERT(image <= Client_CommentOnly_Smiley);
				
				int imgWidth = 16;
				
				// it's already centered by OnDrawItem() ...
				m_ImageList.Draw(image, *dc, rect.GetX(), rect.GetY() - 1,
					wxIMAGELIST_DRAW_TRANSPARENT);
				dc->DrawText(filename, rect.GetX() + imgWidth + 4, rect.GetY());
			} else {
				dc->DrawText(filename, rect.GetX(), rect.GetY());
			}
			break;
		}
		
		// Filesize
		case ColumnSize:
			text = CastItoXBytes( file->GetFileSize() );
			break;
	
		// Transferred
		case ColumnTransferred:
			text = CastItoXBytes( file->GetTransferred() );
			break;
		
		// Completed
		case ColumnCompleted:
			text = CastItoXBytes( file->GetCompletedSize() );
			break;
		
		// Speed
		case ColumnSpeed:
			if ( file->GetTransferingSrcCount() ) {
				text = CFormat(_("%.1f kB/s")) % file->GetKBpsDown();
			}
			break;
	
		// Progress	
		case ColumnProgress:{
			if (thePrefs::ShowProgBar()) {
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
				
				if (thePrefs::ShowPercent()) {
					// Percentage of completing
					// We strip anything below the first decimal point,
					// to avoid Format doing roundings
					float percent = floor( file->GetPercentCompleted() * 10.0f ) / 10.0f;
				
					wxString buffer = CFormat(wxT("%.1f%%")) % percent;
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
		}

		// Sources
		case ColumnSources:	{
			uint16 sc = file->GetSourceCount();
			uint16 ncsc = file->GetNotCurrentSourcesCount();
			if ( ncsc ) {
				text = CFormat(wxT("%i/%i")) % (sc - ncsc) % sc;
			} else {
				text = CFormat(wxT("%i")) % sc;
			}
	
			if ( file->GetSrcA4AFCount() ) {
				text += CFormat(wxT("+%i")) % file->GetSrcA4AFCount();
			}
	
			if ( file->GetTransferingSrcCount() ) {
				text += CFormat(wxT(" (%i)")) % file->GetTransferingSrcCount();
			}
	
			break;
		}
	
		// Priority
		case ColumnPriority:
			text = PriorityToStr( file->GetDownPriority(), file->IsAutoDownPriority() );
			break;
				
		// File-status
		case ColumnStatus:
			text = file->getPartfileStatus();
			break;
		
		// Remaining
		case ColumnTimeRemaining: {
			if ((file->GetStatus() != PS_COMPLETING) && file->IsPartFile()) {
				uint64 remainSize = file->GetFileSize() - file->GetCompletedSize();
				sint32 remainTime = file->getTimeRemaining();
				
				if (remainTime >= 0) {
					text = CastSecondsToHM(remainTime);
				} else {
					text = _("Unknown");
				}
	
				text += wxT(" (") + CastItoXBytes(remainSize) + wxT(")");
			}
			break;
		}
		
		// Last seen completed
		case ColumnLastSeenComplete: {
			if ( file->lastseencomplete ) {
				text = wxDateTime( file->lastseencomplete ).Format( _("%y/%m/%d %H:%M:%S") );
			} else {
				text = _("Unknown");
			}
			break;
		}
		
		// Last received
		case ColumnLastReception: {
			const time_t lastReceived = file->GetLastChangeDatetime();
			if (lastReceived) {
				text = wxDateTime(lastReceived).Format( _("%y/%m/%d %H:%M:%S") );
			} else {
				text = _("Unknown");
			}
		}
	} 

	if ( !text.IsEmpty() ) {
		dc->DrawText( text, rect.GetX(), rect.GetY() );
	}
}

wxString CDownloadListCtrl::GetTTSText(unsigned item) const
{
	return ((FileCtrlItem_Struct*)GetItemData(item))->GetFile()->GetFileName().GetPrintable();
}


int CDownloadListCtrl::SortProc(wxUIntPtr param1, wxUIntPtr param2, long sortData)
{
	FileCtrlItem_Struct* item1 = (FileCtrlItem_Struct*)param1;
	FileCtrlItem_Struct* item2 = (FileCtrlItem_Struct*)param2;

	int sortMod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	sortData &= CMuleListCtrl::COLUMN_MASK;

	// We modify the result so that it matches with ascending or decending
	return sortMod * Compare( item1->GetFile(), item2->GetFile(), sortData);
}


int CDownloadListCtrl::Compare( const CPartFile* file1, const CPartFile* file2, long lParamSort)
{
	int result = 0;

	switch (lParamSort) {
	// Sort by part number
	case ColumnPart:
		result = CmpAny(
			file1->GetPartMetNumber(),
			file2->GetPartMetNumber() );
		break;

	// Sort by filename
	case ColumnFileName:
		result = CmpAny(
			file1->GetFileName(),
			file2->GetFileName() );
		break;

	// Sort by size
	case ColumnSize:
		result = CmpAny(
			file1->GetFileSize(),
			file2->GetFileSize() );
		break;

	// Sort by transferred
	case ColumnTransferred:
		result = CmpAny(
			file1->GetTransferred(),
			file2->GetTransferred() );
		break;

	// Sort by completed
	case ColumnCompleted:
		result = CmpAny(
			file1->GetCompletedSize(),
			file2->GetCompletedSize() );
		break;

	// Sort by speed
	case ColumnSpeed:
		result = CmpAny(
			file1->GetKBpsDown() * 1024,
			file2->GetKBpsDown() * 1024 );
		break;

	// Sort by percentage completed
	case ColumnProgress:
		result = CmpAny(
			file1->GetPercentCompleted(),
			file2->GetPercentCompleted() );
		break;

	// Sort by number of sources
	case ColumnSources:
		result = CmpAny(
			file1->GetSourceCount(),
			file2->GetSourceCount() );
		break;

	// Sort by priority
	case ColumnPriority:
		result = CmpAny(
			file1->GetDownPriority(),
			file2->GetDownPriority() );
		break;

	// Sort by status
	case ColumnStatus:
		result = CmpAny(
			file1->getPartfileStatusRang(),
			file2->getPartfileStatusRang() );
		break;

	// Sort by remaining time
	case ColumnTimeRemaining:
		if (file1->getTimeRemaining() == -1) {
			if (file2->getTimeRemaining() == -1) {
				result = 0;
			} else {
				result = 1;
			}
		} else {
			if (file2->getTimeRemaining() == -1) {
				result = -1;
			} else {
				result = CmpAny(
					file1->getTimeRemaining(),
					file2->getTimeRemaining() );
			}
		}
		break;

	// Sort by last seen complete
	case ColumnLastSeenComplete:
		result = CmpAny(
			file1->lastseencomplete,
			file2->lastseencomplete );
		break;

	// Sort by last reception
	case ColumnLastReception:
		result = CmpAny(
			file1->GetLastChangeDatetime(),
			file2->GetLastChangeDatetime() );
		break;
	}

	return result;
}

void CDownloadListCtrl::ClearCompleted()
{
	CastByID(ID_BTNCLRCOMPL, GetParent(), wxButton)->Enable(false);
	
	// Search for completed files
	ListOfUInts32 toClear;
	for ( ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ) {
		FileCtrlItem_Struct* item = it->second; ++it;
		
		CPartFile* file = item->GetFile();
		
		if (file->IsCompleted() && file->CheckShowItemInGivenCat(m_category)) {
			toClear.push_back(file->ECID());
		}
	}
	if (!toClear.empty()) {
		theApp->downloadqueue->ClearCompleted(toClear);
	}
}


void CDownloadListCtrl::ShowFilesCount( int diff )
{
	m_filecount += diff;
	
	wxStaticText* label = CastByName( wxT("downloadsLabel"), GetParent(), wxStaticText );

	label->SetLabel(CFormat(_("Downloads (%i)")) % m_filecount);
	label->GetParent()->Layout();
}


static const CMuleColour crHave(104, 104, 104);
static const CMuleColour crFlatHave(0, 0, 0);

static const CMuleColour crPending(255, 208, 0);
static const CMuleColour crFlatPending(255, 255, 100);

static const CMuleColour crProgress(0, 224, 0);
static const CMuleColour crFlatProgress(0, 150, 0);

static const CMuleColour crMissing(255, 0, 0);

void CDownloadListCtrl::DrawFileStatusBar(
	const CPartFile* file, wxDC* dc, const wxRect& rect, bool bFlat ) const
{
	static CBarShader s_ChunkBar(16);
	
	s_ChunkBar.SetHeight(rect.height);
	s_ChunkBar.SetWidth(rect.width); 
	s_ChunkBar.SetFileSize( file->GetFileSize() );
	s_ChunkBar.Set3dDepth( thePrefs::Get3DDepth() );

	if ( file->IsCompleted() || file->GetStatus() == PS_COMPLETING ) {
		s_ChunkBar.Fill( bFlat ? crFlatProgress : crProgress );
		s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 
		return;
	}
	
	// Part availability ( of missing parts )
	const CGapList& gaplist = file->GetGapList();
	CGapList::const_iterator it = gaplist.begin();
	uint64 lastGapEnd = 0;
	CMuleColour colour;
	
	for (; it != gaplist.end(); ++it) {
		
		// Start position
		uint32 start = ( it.start() / PARTSIZE );
		// fill the Have-Part (between this gap and the last)
		if (it.start()) {
		  s_ChunkBar.FillRange(lastGapEnd + 1, it.start() - 1,  bFlat ? crFlatHave : crHave);
		}
		lastGapEnd = it.end();
		// End position
		uint32 end   = ( it.end() / PARTSIZE ) + 1;

		// Avoid going past the filesize. Dunno if this can happen, but the old code did check.
		if ( end > file->GetPartCount() ) {
			end = file->GetPartCount();
		}

		// Place each gap, one PART at a time
		for ( uint64 i = start; i < end; ++i ) {
			if ( i < file->m_SrcpartFrequency.size() && file->m_SrcpartFrequency[i]) {
				int blue = 210 - ( 22 * ( file->m_SrcpartFrequency[i] - 1 ) );
				colour.Set(0, ( blue < 0 ? 0 : blue ), 255 );
			} else {
				colour = crMissing;
			}

			if ( file->IsStopped() ) {
				colour.Blend(50);
			}
			
			uint64 gap_begin = ( i == start   ? it.start() : PARTSIZE * i );
			uint64 gap_end   = ( i == end - 1 ? it.end()   : PARTSIZE * ( i + 1 ) - 1 );
		
			s_ChunkBar.FillRange( gap_begin, gap_end,  colour);
		}
	}
	
	// fill the last Have-Part (between this gap and the last)
	s_ChunkBar.FillRange(lastGapEnd + 1, file->GetFileSize() - 1,  bFlat ? crFlatHave : crHave);
	
	// Pending parts
	const CPartFile::CReqBlockPtrList& requestedblocks_list = file->GetRequestedBlockList();
	CPartFile::CReqBlockPtrList::const_iterator it2 = requestedblocks_list.begin();
	// adjacing pending parts must be joined to avoid bright lines between them
	uint64 lastStartOffset = 0;
	uint64 lastEndOffset = 0;
	
	colour = bFlat ? crFlatPending : crPending;
	
	if ( file->IsStopped() ) {
		colour.Blend(50);
	}

	for (; it2 != requestedblocks_list.end(); ++it2) {
		
		if ((*it2)->StartOffset > lastEndOffset + 1) { 
			// not adjacing, draw last block
			s_ChunkBar.FillRange(lastStartOffset, lastEndOffset, colour);
			lastStartOffset = (*it2)->StartOffset;
			lastEndOffset   = (*it2)->EndOffset;
		} else {
			// adjacing, grow block
			lastEndOffset   = (*it2)->EndOffset;
		}
	}
	
	s_ChunkBar.FillRange(lastStartOffset, lastEndOffset, colour);


	// Draw the progress-bar
	s_ChunkBar.Draw( dc, rect.x, rect.y, bFlat );

	
	// Green progressbar width
	int width = (int)(( (float)rect.width / (float)file->GetFileSize() ) *
			file->GetCompletedSize() );

	if ( bFlat ) {
		dc->SetBrush( crFlatProgress.GetBrush() );
		
		dc->DrawRectangle( rect.x, rect.y, width, 3 );
	} else {
		// Draw the two black lines for 3d-effect
		dc->SetPen( *wxBLACK_PEN );
		dc->DrawLine( rect.x, rect.y + 0, rect.x + width, rect.y + 0 );
		dc->DrawLine( rect.x, rect.y + 2, rect.x + width, rect.y + 2 );
		
		// Draw the green line
		dc->SetPen( *(wxThePenList->FindOrCreatePen( crProgress , 1, wxSOLID ) ));
		dc->DrawLine( rect.x, rect.y + 1, rect.x + width, rect.y + 1 );
	}
}

#ifdef __WXMSW__
#	define QUOTE	wxT("\"")
#else
#	define QUOTE	wxT("\'")
#endif

void CDownloadListCtrl::PreviewFile(CPartFile* file)
{
	wxString command;
	// If no player set in preferences, use mplayer.
	// And please, do a warning also :P
	if (thePrefs::GetVideoPlayer().IsEmpty()) {
		wxMessageBox(_(
			"To prevent this warning to show up in every preview,\nset your preferred video player in preferences (default is mplayer)."),
			_("File preview"), wxOK, this);
		// Since newer versions for some reason mplayer does not automatically
		// select video output device and needs a parameter, go figure...
		command = wxT("xterm -T \"aMule Preview\" -iconic -e mplayer ") QUOTE wxT("$file") QUOTE;
	} else {
		command = thePrefs::GetVideoPlayer();
	}

	wxString partFile;	// File name with full path
	wxString partName;	// File name only, without path

	// Check if we are (pre)viewing a completed file or not
	if (!file->IsCompleted()) {
		// Remove the .met and see if out video player specifiation uses the magic string
		partName = file->GetPartMetFileName().RemoveExt().GetRaw();
		partFile = thePrefs::GetTempDir().JoinPaths(file->GetPartMetFileName().RemoveExt()).GetRaw();
	} else {
		// This is a complete file
		// FIXME: This is probably not going to work if the filenames are mangled ...
		partName = file->GetFileName().GetRaw();
		partFile = file->GetFullName().GetRaw();
	}

	// Compatibility with old behaviour
	if (!command.Replace(wxT("$file"), wxT("%PARTFILE"))) {
		if ((command.Find(wxT("%PARTFILE")) == wxNOT_FOUND) && (command.Find(wxT("%PARTNAME")) == wxNOT_FOUND)) {
			// No magic string, so we just append the filename to the player command
			// Need to use quotes in case filename contains spaces
			command << wxT(" ") << QUOTE << wxT("%PARTFILE") << QUOTE;
		}
	}

#ifndef __WXMSW__
	// We have to escape quote characters in the file name, otherwise arbitrary
	// options could be passed to the player.
	partFile.Replace(QUOTE, wxT("\\") QUOTE);
	partName.Replace(QUOTE, wxT("\\") QUOTE);
#endif

	command.Replace(wxT("%PARTFILE"), partFile);
	command.Replace(wxT("%PARTNAME"), partName);

	// We can't use wxShell here, it blocks the app
	CTerminationProcess *p = new CTerminationProcess(command);
	int ret = wxExecute(command, wxEXEC_ASYNC, p);
	bool ok = ret > 0;
	if (!ok) {
		delete p;
		AddLogLineC(CFormat( _("ERROR: Failed to execute external media-player! Command: `%s'") ) %
			command );
	}
}
// File_checked_for_headers
