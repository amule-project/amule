//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include "GenericClientListCtrl.h"

#include <protocol/ed2k/ClientSoftware.h>
#include <common/MenuIDs.h>

#include <common/Format.h>	// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "BarShader.h"		// Needed for CBarShader
#include "BitVector.h"
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "ChatWnd.h"		// Needed for CChatWnd
#include "CommentDialogLst.h"	// Needed for CCommentDialogLst
#include "DataToText.h"		// Needed for PriorityToStr
#include "FileDetailDialog.h"	// Needed for CFileDetailDialog
#include "GetTickCount.h"	// Needed for GetTickCount
#include "GuiEvents.h"		// Needed for CoreNotify_*
#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"	// Needed for IP2Country
#endif
#include "Logger.h"
#include "muuli_wdr.h"		// Needed for ID_DLOADLIST
#include "PartFile.h"		// Needed for CPartFile
#include "Preferences.h"
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "TerminationProcess.h"	// Needed for CTerminationProcess
#include "ClientRef.h"		// Needed for CClientRef
#include "FriendList.h"

struct ClientCtrlItem_Struct
{
	ClientCtrlItem_Struct()
		: dwUpdated(0),
		  status(NULL),
		  m_owner(NULL),
		  m_type(UNAVAILABLE_SOURCE)
	{ }
	
	~ClientCtrlItem_Struct() {
		delete status;
	}

	SourceItemType GetType() const {
		return m_type;
	}

	CKnownFile* GetOwner() const {
		return m_owner;
	}


	CClientRef& GetSource() {
		return m_sourceValue;
	}

	void SetContents(CKnownFile* owner, const CClientRef& source, SourceItemType type) {
		m_owner = owner;
		m_sourceValue = source;
		m_type = type;
	}
	
	void SetType(SourceItemType type) { m_type = type; }
	
	uint32		dwUpdated;
	wxBitmap*	status;

private:
	CKnownFile*		m_owner;
	CClientRef		m_sourceValue;
	SourceItemType	m_type;
};

#define m_ImageList theApp->amuledlg->m_imagelist

BEGIN_EVENT_TABLE(CGenericClientListCtrl, CMuleListCtrl)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY,	CGenericClientListCtrl::OnItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, CGenericClientListCtrl::OnMouseRightClick)
	EVT_LIST_ITEM_MIDDLE_CLICK(wxID_ANY, CGenericClientListCtrl::OnMouseMiddleClick)

	EVT_CHAR( CGenericClientListCtrl::OnKeyPressed )

	EVT_MENU( MP_CHANGE2FILE,		CGenericClientListCtrl::OnSwapSource )
	EVT_MENU( MP_SHOWLIST,			CGenericClientListCtrl::OnViewFiles )
	EVT_MENU( MP_ADDFRIEND,			CGenericClientListCtrl::OnAddFriend )
	EVT_MENU( MP_FRIENDSLOT,		CGenericClientListCtrl::OnSetFriendslot )
	EVT_MENU( MP_SENDMESSAGE,		CGenericClientListCtrl::OnSendMessage )
	EVT_MENU( MP_DETAIL,			CGenericClientListCtrl::OnViewClientInfo )
END_EVENT_TABLE()

//! This listtype is used when gathering the selected items.
typedef std::list<ClientCtrlItem_Struct*>	ItemList;

CGenericClientListCtrl::CGenericClientListCtrl(
    const wxString& tablename,
	wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
	long style, const wxValidator& validator, const wxString& name )
:
CMuleListCtrl( parent, winid, pos, size, style | wxLC_OWNERDRAW | wxLC_VRULES | wxLC_HRULES, validator, name ),
m_columndata(0, NULL)
{
	// Setting the sorter function must be done in the derived class, to use the translation function correctly.
	// SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( tablename );

	m_menu = NULL;
	m_showing = false;
	
	m_hilightBrush  = CMuleColour(wxSYS_COLOUR_HIGHLIGHT).Blend(125).GetBrush();

	m_hilightUnfocusBrush = CMuleColour(wxSYS_COLOUR_BTNSHADOW).Blend(125).GetBrush();
	
	m_clientcount = 0;
}

wxString CGenericClientListCtrl::TranslateCIDToName(GenericColumnEnum cid)
{
	wxString name;

	switch (cid) {
		case ColumnUserName:
			name = wxT("N");
			break;
		case ColumnUserDownloaded:
			name = wxT("D");
			break;
		case ColumnUserUploaded:
			name = wxT("U");
			break;
		case ColumnUserSpeedDown:
			name = wxT("S");
			break;
		case ColumnUserSpeedUp:
			name = wxT("s");
			break;
		case ColumnUserProgress:
			name = wxT("P");
			break;
		case ColumnUserAvailable:
			name = wxT("A");
			break;
		case ColumnUserVersion:
			name = wxT("V");
			break;
		case ColumnUserQueueRankLocal:
			name = wxT("Q");
			break;
		case ColumnUserQueueRankRemote:
			name = wxT("q");
			break;
		case ColumnUserOrigin:
			name = wxT("O");
			break;
		case ColumnUserFileNameDownload:
			name = wxT("F");
			break;
		case ColumnUserFileNameUpload:
			name = wxT("f");
			break;
		case ColumnUserFileNameDownloadRemote:
			name = wxT("R");
			break;
		case ColumnInvalid:
		default:
			wxFAIL;
			break;
	}

	return name;
}

void CGenericClientListCtrl::InitColumnData()
{
	if (!m_columndata.n_columns) {
		throw wxString(wxT("CRITICAL: Initialization of the column data lacks subclass information"));
	}
	
	for (int i = 0; i < m_columndata.n_columns; ++i) { 
		InsertColumn( i, wxGetTranslation(m_columndata.columns[i].title), wxLIST_FORMAT_LEFT, m_columndata.columns[i].width, TranslateCIDToName(m_columndata.columns[i].cid));
	}

	LoadSettings();	
}

CGenericClientListCtrl::~CGenericClientListCtrl()
{
	while ( !m_ListItems.empty() ) {
		delete m_ListItems.begin()->second;
		m_ListItems.erase( m_ListItems.begin() );
	}
}
void CGenericClientListCtrl::RawAddSource(CKnownFile* owner, CClientRef source, SourceItemType type)
{
	ClientCtrlItem_Struct* newitem = new ClientCtrlItem_Struct;
	newitem->SetContents(owner, source, type);
	
	m_ListItems.insert( ListItemsPair(source.ECID(), newitem) );
	
	long item = InsertItem( GetItemCount(), wxEmptyString );
	SetItemPtrData( item, reinterpret_cast<wxUIntPtr>(newitem) );
	SetItemBackgroundColour( item, GetBackgroundColour() );	
}

void CGenericClientListCtrl::AddSource(CKnownFile* owner, const CClientRef& source, SourceItemType type)
{
	wxCHECK_RET(owner, wxT("NULL owner in CGenericClientListCtrl::AddSource"));
	wxCHECK_RET(source.IsLinked(), wxT("Unlinked source in CGenericClientListCtrl::AddSource"));
	
	// Update the other instances of this source
	bool bFound = false;
	ListIteratorPair rangeIt = m_ListItems.equal_range(source.ECID());
	for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second; ++it ) {
		ClientCtrlItem_Struct* cur_item = it->second;

		// Check if this source has been already added to this file => to be sure
		if ( cur_item->GetOwner() == owner ) {
			// Update this instance with its new setting
			if ((type == A4AF_SOURCE) && 
				cur_item->GetSource().GetRequestFile() 
				&& std::binary_search(m_knownfiles.begin(), m_knownfiles.end(), cur_item->GetSource().GetRequestFile())) {				
				cur_item->SetContents(owner, source, AVAILABLE_SOURCE);
			} else {
				cur_item->SetContents(owner, source, type);
			}			
			cur_item->dwUpdated = 0;
			bFound = true;
		} else if ( type == AVAILABLE_SOURCE ) {
			// The state 'Available' is exclusive
			cur_item->SetContents(cur_item->GetOwner(), source, A4AF_SOURCE);
			cur_item->dwUpdated = 0;
		}
	}

	if ( bFound ) {
		return;
	}

	if ( std::binary_search(m_knownfiles.begin(), m_knownfiles.end(), owner) ) {
		RawAddSource(owner, source, type);
		
		ShowSourcesCount( 1 );
	}
}

void CGenericClientListCtrl::RawRemoveSource( ListItems::iterator& it)
{
	ClientCtrlItem_Struct* item = it->second;
	
	long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(item) );
	
	if ( index > -1 ) {
		DeleteItem( index );
	}

	delete item;

	// Remove it from the m_ListItems
	m_ListItems.erase( it );
}

void CGenericClientListCtrl::RemoveSource(uint32 source, const CKnownFile* owner)
{
	// A NULL owner means remove it no matter what.
	wxCHECK_RET(source, wxT("NULL source in CGenericClientListCtrl::RemoveSource"));
	
	// Retrieve all entries matching the source
	ListIteratorPair rangeIt = m_ListItems.equal_range(source);
	
	int removedItems = 0;
	
	for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second;  /* no ++, it happens later */) {
		ListItems::iterator tmp = it++;
		
		if ( owner == NULL || owner == tmp->second->GetOwner() ) {

			RawRemoveSource(tmp);
			
			++removedItems;
		}
	}
	
	ShowSourcesCount((-1) * removedItems);
}

void CGenericClientListCtrl::UpdateItem(uint32 toupdate, SourceItemType type)
{
	// Retrieve all entries matching the source
	ListIteratorPair rangeIt = m_ListItems.equal_range( toupdate );
	
	if ( rangeIt.first != rangeIt.second ) {
		// Visible lines, default to all because not all platforms
		// support the GetVisibleLines function
		long first = 0, last = GetItemCount();

	#ifndef __WXMSW__
		// Get visible lines if we need them
		GetVisibleLines( &first, &last );
	#endif
		
		for ( ListItems::iterator it = rangeIt.first; it != rangeIt.second; ++it ) {
			ClientCtrlItem_Struct* item = it->second;
			
			long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(item) );

			if ((type == A4AF_SOURCE) && 
					item->GetSource().GetRequestFile() 
					&& std::binary_search(m_knownfiles.begin(), m_knownfiles.end(), item->GetSource().GetRequestFile())) {
				
				item->SetType(AVAILABLE_SOURCE);	
			} else {
				item->SetType(type);
			}
			
			item->dwUpdated = 0;
			
			// Only update visible lines
			if ( index >= first && index <= last) {
				RefreshItem( index );
			}
		}
	}
}

void CGenericClientListCtrl::ShowSources( const CKnownFileVector& files )
{	
	Freeze();
	
	// The stored vector is sorted, as is the received one, so we can use binary_search

	for (unsigned i = 0; i < m_knownfiles.size(); ++i) {
		// Files that are not in the new list must have show set to false.
		if (!std::binary_search(files.begin(), files.end(), m_knownfiles[i])) {
			SetShowSources(m_knownfiles[i], false);
		}
	}
	
	// This will call again SetShowSources in files that were in both vectors. Right now
	// that function is just a inline setter, so any way to prevent it would be wasteful,
	// but this must be reviewed if that fact changes.

	for (unsigned i = 0; i < files.size(); ++i) {
		SetShowSources(files[i], true);
	}
	
	// We must cleanup sources that are not in the received files. 
	
	int itemDiff = 0;
	
	for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); /* no ++, it happens later */) {
		ListItems::iterator tmp = it++;
		ClientCtrlItem_Struct* item = tmp->second;
		if (!std::binary_search(files.begin(), files.end(), item->GetOwner())) {
			// Remove it from the m_ListItems
			RawRemoveSource(tmp);
			--itemDiff;
		}
	}
	
	for (unsigned i = 0; i < files.size(); ++i) {
		
		// Only those that weren't showing already
		if (!std::binary_search(m_knownfiles.begin(), m_knownfiles.end(), files[i])) {
			
			CKnownFile* file = files[i];

			wxASSERT_MSG(file, wxT("NULL file in CGenericClientListCtrl::ShowSources"));
			
			if (file) {
				
				CKnownFile::SourceSet::const_iterator it;
			
				if (IsShowingDownloadSources()) {
					const CKnownFile::SourceSet& normSources = (dynamic_cast<CPartFile*>( file ))->GetSourceList();
					const CKnownFile::SourceSet& a4afSources = (dynamic_cast<CPartFile*>( file ))->GetA4AFList();
						
					// Adding normal sources
					for ( it = normSources.begin(); it != normSources.end(); ++it ) {
						switch (it->GetDownloadState()) {
							case DS_DOWNLOADING:
							case DS_ONQUEUE:
								RawAddSource( file, *it, AVAILABLE_SOURCE );
								++itemDiff;
								break;
							default:
								// Any other state
								RawAddSource( file, *it, UNAVAILABLE_SOURCE );
								++itemDiff;
						}
					}
			
					// Adding A4AF sources
					for ( it = a4afSources.begin(); it != a4afSources.end(); ++it ) {
						// Only add if the A4AF file is not in the shown list.
						if (!std::binary_search(files.begin(), files.end(), it->GetRequestFile())) {
							RawAddSource( file, *it, A4AF_SOURCE );
							++itemDiff;
						}
					}
				} else {
					// Known file
					const CKnownFile::SourceSet& sources = file->m_ClientUploadList;
					for ( it = sources.begin(); it != sources.end(); ++it ) {
						switch (it->GetUploadState()) {
							case US_UPLOADING:
							case US_ONUPLOADQUEUE:
								RawAddSource( file, *it, AVAILABLE_SOURCE );
								++itemDiff;
								break;
							default:
								// Any other state
								RawAddSource( file, *it, UNAVAILABLE_SOURCE );
								++itemDiff;
						}
					}			
				}
			}
		}
	}

	m_knownfiles = files;	
	
	ShowSourcesCount( itemDiff );
		 
	SortList();
	
	Thaw();
}

/**
 * Helper-function: This function is used to gather selected items.
 *
 * @param list A pointer to the list to gather items from.
 * @return A list containing the selected items of the choosen types.
 */
ItemList GetSelectedItems( CGenericClientListCtrl* list )
{
	ItemList results;

	long index = list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while ( index > -1 ) {
		ClientCtrlItem_Struct* item = (ClientCtrlItem_Struct*)list->GetItemData( index );

		results.push_back( item );
		
		index = list->GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	return results;
}

void CGenericClientListCtrl::OnSwapSource( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this );
	
	for ( ItemList::iterator it = sources.begin(); it != sources.end(); ++it ) {
		CKnownFile * kf = (*it)->GetOwner();
		if (!kf->IsPartFile()) {
			wxFAIL_MSG(wxT("File is not a partfile when swapping sources"));
			continue;
		}
		(*it)->GetSource().SwapToAnotherFile( true, false, false,  dynamic_cast<CPartFile*>(kf));
	}
}


void CGenericClientListCtrl::OnViewFiles( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this );

	if ( sources.size() == 1 ) {		
		sources.front()->GetSource().RequestSharedFileList();
	}
}


void CGenericClientListCtrl::OnAddFriend( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this );

	for ( ItemList::iterator it = sources.begin(); it != sources.end(); ++it ) {
		CClientRef &client = (*it)->GetSource();
		if (client.IsFriend()) {
			theApp->friendlist->RemoveFriend(client.GetFriend());
		} else {
			theApp->friendlist->AddFriend(client);
		}
	}
}


void CGenericClientListCtrl::OnSetFriendslot(wxCommandEvent& evt)
{
	ItemList sources = ::GetSelectedItems( this );

	ItemList::iterator it = sources.begin(); 
	if (it != sources.end()) {
		CClientRef &client = (*it)->GetSource();
		theApp->friendlist->SetFriendSlot(client.GetFriend(), evt.IsChecked());
		it++;
	}
	if (it != sources.end()) {
		wxMessageBox(_("You are not allowed to set more than one friendslot.\n Only one slot was assigned."), _("Multiple selection"), wxOK | wxICON_ERROR, this);
	}
}


void CGenericClientListCtrl::OnSendMessage( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this );

	if ( sources.size() == 1 ) {
		CClientRef & source = (sources.front())->GetSource();

		// These values are cached, since calling wxGetTextFromUser will
		// start an event-loop, in which the client may be deleted.
		wxString userName = source.GetUserName();
		uint64 userID = GUI_ID(source.GetIP(), source.GetUserPort());
		
		wxString message = ::wxGetTextFromUser(_("Send message to user"),
			_("Message to send:"));
		if ( !message.IsEmpty() ) {
			theApp->amuledlg->m_chatwnd->SendMessage(message, userName, userID);
		}
	}
}


void CGenericClientListCtrl::OnViewClientInfo( wxCommandEvent& WXUNUSED(event) )
{
	ItemList sources = ::GetSelectedItems( this );

	if ( sources.size() == 1 ) {
		CClientDetailDialog( this, sources.front()->GetSource() ).ShowModal();
	}
}


void CGenericClientListCtrl::OnItemActivated( wxListEvent& evt )
{
	CClientDetailDialog( this, ((ClientCtrlItem_Struct*)GetItemData( evt.GetIndex()))->GetSource() ).ShowModal();
}


void CGenericClientListCtrl::OnMouseRightClick(wxListEvent& evt)
{
	long index = CheckSelection(evt);
	if (index < 0) {
		return;
	}	
	
	delete m_menu;
	m_menu = NULL;
	
	ClientCtrlItem_Struct* item = (ClientCtrlItem_Struct*)GetItemData( index );
	CClientRef& client = item->GetSource();
	
	m_menu = new wxMenu(wxT("Clients"));
	m_menu->Append(MP_DETAIL, _("Show &Details"));
	m_menu->Append(MP_ADDFRIEND, client.IsFriend() ? _("Remove from friends") : _("Add to Friends"));

	m_menu->AppendCheckItem(MP_FRIENDSLOT, _("Establish Friend Slot"));
	if (client.IsFriend()) {
		m_menu->Enable(MP_FRIENDSLOT, true);
		m_menu->Check(MP_FRIENDSLOT, client.GetFriendSlot());
	} else {
		m_menu->Enable(MP_FRIENDSLOT, false);
	}

	m_menu->Append(MP_SHOWLIST, _("View Files"));
	m_menu->Append(MP_SENDMESSAGE, _("Send message"));
	
	m_menu->Append(MP_CHANGE2FILE, _("Swap to this file"));
	
	// Only enable the Swap option for A4AF sources
	m_menu->Enable(MP_CHANGE2FILE, (item->GetType() == A4AF_SOURCE));
	// We need a valid IP if we are to message the client
	m_menu->Enable(MP_SENDMESSAGE, (client.GetIP() != 0));
	
	m_menu->Enable(MP_SHOWLIST, !client.HasDisabledSharedFiles());
	
	PopupMenu(m_menu, evt.GetPoint());
	
	delete m_menu;
	m_menu = NULL;
	
}


void CGenericClientListCtrl::OnMouseMiddleClick(wxListEvent& evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long index = CheckSelection(evt);
	if ( index < 0 ) {
		return;
	}
	
	CClientDetailDialog(this, ((ClientCtrlItem_Struct*)GetItemData( index ))->GetSource()).ShowModal();
}


void CGenericClientListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// No actions right now.
	//switch (event.GetKeyCode()) {
	//	default:
			event.Skip();
	//}
}


void CGenericClientListCtrl::OnDrawItem(
	int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted)
{
	// Don't do any drawing if there's nobody to see it.
	if ( !theApp->amuledlg->IsDialogVisible( GetParentDialog() ) ) {
		return;
	}

	ClientCtrlItem_Struct* content = (ClientCtrlItem_Struct *)GetItemData(item);

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

	// Various constant values we use
	const int iTextOffset = (( rect.GetHeight() - dc->GetCharHeight() ) / 2) + 1 /* Fixes rounding in the centering math, much easier than floor() */;
	const int iOffset = 2;
	wxASSERT(m_ImageList.GetImageCount() > 0);
	int imageListBitmapYOffset = 0;
	int imageListBitmapXSize = 0;
	if (m_ImageList.GetSize(0, imageListBitmapXSize, imageListBitmapYOffset)) {
		imageListBitmapXSize += 2; // Padding.
		imageListBitmapYOffset = ((rect.GetHeight() - imageListBitmapYOffset) / 2) + 1 /* Fixes rounding like above */;
	} else {
		wxFAIL;
	}

	wxRect cur_rec( iOffset, rect.y, 0, rect.height );
	
	for (int i = 0; i < GetColumnCount(); ++i) {
		
		int columnwidth = GetColumnWidth(i);

		if (columnwidth > 2*iOffset) {
			// Make a copy of the current rectangle so we can apply specific tweaks
			wxRect target_rec = cur_rec;
			target_rec.width = columnwidth - 2*iOffset;

			GenericColumnEnum cid = m_columndata.columns[i].cid;
			
			// Draw the item
			DrawClientItem(dc, cid, target_rec, content, iTextOffset, imageListBitmapYOffset, imageListBitmapXSize);
		}
		
		// Increment to the next column
		cur_rec.x += columnwidth;
	}
}

void CGenericClientListCtrl::DrawClientItem(wxDC* dc, int nColumn, const wxRect& rect, ClientCtrlItem_Struct* item, int iTextOffset, int iBitmapOffset, int iBitmapXSize ) const
{
	wxDCClipper clipper( *dc, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight() );
	wxString buffer;
	
	const CClientRef& client = item->GetSource();

	switch (nColumn) {
		// Client name + various icons
		case ColumnUserName: {
			// Point will get shifted per drawing.

			wxPoint point( rect.GetX(), rect.GetY() );

			uint8 image = Client_Grey_Smiley;

			if (item->GetType() != A4AF_SOURCE) {
				
				switch (client.GetDownloadState()) {
					case DS_CONNECTING:
					case DS_CONNECTED:
					case DS_WAITCALLBACK:
					case DS_TOOMANYCONNS:
						image = Client_Red_Smiley;
						break;
					case DS_ONQUEUE:
						if (client.IsRemoteQueueFull()) {
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
						image = Client_Grey_Smiley; // Redundant
						break;
					default: // DS_NONE i.e.
						image = Client_White_Smiley;
					}

				} else {
					// Default (Client_Grey_Smiley)
				}

				m_ImageList.Draw(image, *dc, point.x, point.y + iBitmapOffset, wxIMAGELIST_DRAW_TRANSPARENT);

				// Next

				point.x += iBitmapXSize; 

				uint8 clientImage = Client_Unknown;
				
				if ( client.IsFriend() ) {
					clientImage = Client_Friend_Smiley;
				} else {
					switch ( client.GetClientSoft() ) {
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
							// Which is a faillback to the default (Client_Unknown)
							break;
					}
				}

				int realY = point.y + iBitmapOffset;
				m_ImageList.Draw(clientImage, *dc, point.x, realY, wxIMAGELIST_DRAW_TRANSPARENT);

				if (client.GetScoreRatio() > 1) {
					// Has credits, draw the gold star
					m_ImageList.Draw(Client_CreditsYellow_Smiley, *dc, point.x, realY, 
						wxIMAGELIST_DRAW_TRANSPARENT );
				}	else if ( !client.ExtProtocolAvailable() ) {
					// No Ext protocol -> Draw the '-'
					m_ImageList.Draw(Client_ExtendedProtocol_Smiley, *dc, point.x, realY,
						wxIMAGELIST_DRAW_TRANSPARENT);
				}

				if (client.IsIdentified()) {
					// the 'v'
					m_ImageList.Draw(Client_SecIdent_Smiley, *dc, point.x, realY,
						wxIMAGELIST_DRAW_TRANSPARENT);					
				} else if (client.IsBadGuy()) {
					// the 'X'
					m_ImageList.Draw(Client_BadGuy_Smiley, *dc, point.x, realY,
						wxIMAGELIST_DRAW_TRANSPARENT);					
				}
							
				if (client.GetObfuscationStatus() == OBST_ENABLED) {
					// the "¿" except it's a key
					m_ImageList.Draw(Client_Encryption_Smiley, *dc, point.x, realY,
						wxIMAGELIST_DRAW_TRANSPARENT);					
				}				
				
				// Next

				point.x += iBitmapXSize; 

				wxString userName;
#ifdef ENABLE_IP2COUNTRY
				if (theApp->amuledlg->m_IP2Country->IsEnabled() && thePrefs::IsGeoIPEnabled()) {
					// Draw the flag. Size can't be precached.
					const CountryData& countrydata = theApp->amuledlg->m_IP2Country->GetCountryData(client.GetFullIP());

					realY = point.y + (rect.GetHeight() - countrydata.Flag.GetHeight())/2 + 1 /* floor() */;

					dc->DrawBitmap(countrydata.Flag,
						point.x, realY,
						true);
				
					userName << countrydata.Name;
				
					userName << wxT(" - ");

					point.x += countrydata.Flag.GetWidth() + 2 /*Padding*/;
				}
#endif // ENABLE_IP2COUNTRY
				if (client.GetUserName().IsEmpty()) {
					userName << wxT("?");
				} else {
					userName << client.GetUserName();
				}

				dc->DrawText(userName, point.x, rect.GetY() + iTextOffset);
			}
			break;

		case ColumnUserDownloaded:
			if (item->GetType() != A4AF_SOURCE && client.GetTransferredDown()) {
				buffer = CastItoXBytes(client.GetTransferredDown());
				dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			}
			break;
		case ColumnUserUploaded:
			if (item->GetType() != A4AF_SOURCE && client.GetTransferredUp()) {
				buffer = CastItoXBytes(client.GetTransferredUp());
				dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			}
			break;
		case ColumnUserSpeedDown:
			if (item->GetType() != A4AF_SOURCE && client.GetKBpsDown() > 0.001) {
				buffer = CFormat(_("%.1f kB/s")) % client.GetKBpsDown();
				dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			}
			break;
		case ColumnUserSpeedUp:
			// Datarate is in bytes.
			if (item->GetType() != A4AF_SOURCE && client.GetUploadDatarate() > 1024) {
				buffer = CFormat(_("%.1f kB/s")) % (client.GetUploadDatarate() / 1024.0);
				dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			}
			break;
		case ColumnUserProgress:
			if ( thePrefs::ShowProgBar() ) {
				int iWidth = rect.GetWidth() - 2;
				int iHeight = rect.GetHeight() - 2;

				// don't draw Text beyond the bar
				dc->SetClippingRegion(rect.GetX(), rect.GetY() + 1, iWidth, iHeight);
			
				if ( item->GetType() != A4AF_SOURCE ) {
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
				} else {
					wxString a4af;
					CPartFile* p = client.GetRequestFile();
					if (p) {
						a4af = p->GetFileName().GetPrintable();
					} else {
						a4af = wxT("?");
					}
					buffer = CFormat(wxT("%s: %s")) % _("A4AF") % a4af;
					
					int midx = (2*rect.GetX() + rect.GetWidth()) >> 1;
					int midy = (2*rect.GetY() + rect.GetHeight()) >> 1;
					
					wxCoord txtwidth, txtheight;
					
					dc->GetTextExtent(buffer, &txtwidth, &txtheight);
					
					dc->SetTextForeground(*wxBLACK);
					dc->DrawText(buffer, wxMax(rect.GetX() + 2, midx - (txtwidth >> 1)), midy - (txtheight >> 1));

					// Draw black border
					dc->SetPen( *wxBLACK_PEN );
					dc->SetBrush( *wxTRANSPARENT_BRUSH );
					dc->DrawRectangle( rect.GetX(), rect.GetY() + 1, iWidth, iHeight );		
				}
			}
			break;

		case ColumnUserAvailable: {
				if ( client.GetUpPartCount() ) {
					DrawStatusBar( client, dc, rect );
				}
				break;
			}

		case ColumnUserVersion: {
				dc->DrawText(client.GetClientVerString(), rect.GetX(), rect.GetY() + iTextOffset);
				break;
			}

		case ColumnUserQueueRankRemote: {
			sint16 qrDiff = 0;
			wxColour savedColour = dc->GetTextForeground();	
			// We only show the queue rank for sources actually queued for that file
			if (	item->GetType() != A4AF_SOURCE && client.GetDownloadState() == DS_ONQUEUE ) {
				if (client.IsRemoteQueueFull()) {
					buffer = _("Queue Full");
				} else {
					uint16 rank = client.GetRemoteQueueRank();
					if (rank) {
						qrDiff = rank - client.GetOldRemoteQueueRank();
						if (qrDiff == rank) {
							qrDiff = 0;
						}
						if ( qrDiff < 0 ) {
							dc->SetTextForeground(*wxBLUE);
						}
						if ( qrDiff > 0 ) {
							dc->SetTextForeground(*wxRED);
						}
						buffer = CFormat(_("On Queue: %u (%i)")) % rank % qrDiff;
					} else {
						buffer = _("On Queue");
					}
				}
			} else {
				if (item->GetType() != A4AF_SOURCE) {
					buffer = DownloadStateToStr( client.GetDownloadState(), 
						client.IsRemoteQueueFull() );
				} else {
					buffer = _("Asked for another file");
					if (	client.GetRequestFile() &&
						client.GetRequestFile()->GetFileName().IsOk()) {
						buffer += CFormat(wxT(" (%s)")) 
							% client.GetRequestFile()->GetFileName();
					}
				}
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			if (qrDiff) {
				dc->SetTextForeground(savedColour);
			}
			break;
		}
		case ColumnUserQueueRankLocal:	
			if (item->GetType() != A4AF_SOURCE) {
				if (client.GetUploadState() == US_ONUPLOADQUEUE ) {
					uint16 nRank = client.GetUploadQueueWaitingPosition();
					if (nRank == 0) {
						buffer = _("Waiting for upload slot");
					} else {
						buffer = CFormat(_("On Queue: %u")) % nRank;
					}
				} else if (client.GetUploadState() == US_UPLOADING) {
					buffer = _("Uploading");
				} else {
					buffer = _("None");
				}
			} else {
				buffer = _("Asked for another file");
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			break;
		// Source comes from?
		case ColumnUserOrigin: {
			buffer = wxGetTranslation(OriginToText(client.GetSourceFrom()));
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			break;
		}
		// Local file name to identify on multi select
		case ColumnUserFileNameDownload: {
			const CPartFile * pf = client.GetRequestFile();
			if (pf) {
				buffer = pf->GetFileName().GetPrintable();
			} else {
				buffer = _("Unknown");
				buffer = wxT("[") + buffer + wxT("]");
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			break;
		}
		case ColumnUserFileNameUpload: {
			const CKnownFile * kf = client.GetUploadFile();
			if (kf) {
				buffer = kf->GetFileName().GetPrintable();
			} else {
				buffer = _("Unknown");
				buffer = wxT("[") + buffer + wxT("]");
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			break;
		}
		case ColumnUserFileNameDownloadRemote: {
			bool nameMissmatch = false;
			wxColour savedColour = dc->GetTextForeground();
			if (client.GetClientFilename().IsEmpty()) {
				buffer = _("Unknown");
				buffer = wxT("[") + buffer + wxT("]");
			} else {
				buffer = client.GetClientFilename();
				const CPartFile * pf = client.GetRequestFile();
				if (pf && (pf->GetFileName().GetPrintable().CmpNoCase(buffer) != 0)) {
					nameMissmatch = true;
					dc->SetTextForeground(*wxRED);
				}
			}
			dc->DrawText(buffer, rect.GetX(), rect.GetY() + iTextOffset);
			if (nameMissmatch) {
				dc->SetTextForeground(savedColour);
			}
			break;
		}
	}
}

int CGenericClientListCtrl::SortProc(wxUIntPtr param1, wxUIntPtr param2, long sortData)
{
	ClientCtrlItem_Struct* item1 = (ClientCtrlItem_Struct*)param1;
	ClientCtrlItem_Struct* item2 = (ClientCtrlItem_Struct*)param2;

	int sortMod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	sortData &= CMuleListCtrl::COLUMN_MASK;
	int comp = 0;

	// Two sources, some different possibilites
	// Avilable sources first, if we have both an
	// available and an unavailable
	comp = ( item2->GetType() - item1->GetType() );

	if (comp) {
			// unavailable and available. The order is fixed regardless of sort-order.
		return comp;
	} else {
		comp = Compare(item1->GetSource(), item2->GetSource(), sortData);
	}

	// We modify the result so that it matches with ascending or decending
	return sortMod * comp;
}

int CGenericClientListCtrl::Compare(
	const CClientRef& client1, const CClientRef& client2, long lParamSort)
{
	switch (lParamSort) {
		// Sort by name
		case ColumnUserName:
			return CmpAny( client1.GetUserName(), client2.GetUserName() );
	
		// Sort by transferred in the following fields
		case ColumnUserDownloaded:	
			return CmpAny( client1.GetTransferredDown(), client2.GetTransferredDown() );

		// Sort by transferred in the following fields
		case ColumnUserUploaded:
			return CmpAny( client1.GetTransferredUp(), client2.GetTransferredUp() );

		// Sort by speed
		case ColumnUserSpeedDown:
			return CmpAny( client1.GetKBpsDown(), client2.GetKBpsDown() );
		
		// Sort by speed
		case ColumnUserSpeedUp:
			return CmpAny( client1.GetUploadDatarate(), client2.GetUploadDatarate() );

		// Sort by parts offered
		case ColumnUserProgress:
			return CmpAny(
				client1.GetAvailablePartCount(),
				client2.GetAvailablePartCount() );
		
		// Sort by client version
		case ColumnUserVersion: {
			int cmp = client1.GetSoftStr().Cmp(client2.GetSoftStr());

			if (cmp == 0) {
				cmp = CmpAny(client1.GetVersion(), client2.GetVersion());
			}
			if (cmp == 0) {
				cmp = client1.GetClientModString().Cmp(client2.GetClientModString());
			}
			return cmp;
		}
		
		// Sort by Queue-Rank
		case ColumnUserQueueRankRemote: {
			// This will sort by download state: Downloading, OnQueue, Connecting ...
			// However, Asked For Another will always be placed last, due to
			// sorting in SortProc
			if ( client1.GetDownloadState() != client2.GetDownloadState() ) {
				return client1.GetDownloadState() - client2.GetDownloadState();
			}

			// Placing items on queue before items on full queues
			if ( client1.IsRemoteQueueFull() ) {
				if ( client2.IsRemoteQueueFull() ) {
					return 0;
				} else {
					return  1;
				}
			} else if ( client2.IsRemoteQueueFull() ) {
				return -1;
			} else {
				if ( client1.GetRemoteQueueRank() ) {
					if ( client2.GetRemoteQueueRank() ) {
						return CmpAny(
							client1.GetRemoteQueueRank(),
							client2.GetRemoteQueueRank() );
					} else {
						return -1;
					}
				} else {
					if ( client2.GetRemoteQueueRank() ) {
						return  1;
					} else {
						return  0;
					}
				}
			}
		}
		
		// Sort by Queue-Rank
		case ColumnUserQueueRankLocal: {
			// This will sort by download state: Downloading, OnQueue, Connecting ...
			// However, Asked For Another will always be placed last, due to
			// sorting in SortProc
			if ( client1.GetUploadState() != client2.GetUploadState() ) {
				return client1.GetUploadState() - client2.GetUploadState();
			}

			uint16 rank1 = client1.GetUploadQueueWaitingPosition();
			uint16 rank2 = client2.GetUploadQueueWaitingPosition();
			// Placing items on queue before items on full queues
			if ( !rank1 ) {
				if ( !rank2 ) {
					return 0;
				} else {
					return 1;
				}
			} else if ( !rank2 ) {
				return -1;
			} else {
				if ( rank1 ) {
					if ( rank2 ) {
						return CmpAny(
							rank1,
							rank2 );
					} else {
						return -1;
					}
				} else {
					if ( rank2 ) {
						return  1;
					} else {
						return  0;
					}
				}
			}
		}

		// Source of source ;)
		case ColumnUserOrigin:
			return CmpAny(client1.GetSourceFrom(), client2.GetSourceFrom());
		
		// Sort by local filename (download)
		case ColumnUserFileNameDownload: {
			wxString buffer1, buffer2;
			const CPartFile * pf1 = client1.GetRequestFile();
			if (pf1) {
				buffer1 = pf1->GetFileName().GetPrintable();
			}
			const CPartFile * pf2 = client2.GetRequestFile();
			if (pf2) {
				buffer2 = pf2->GetFileName().GetPrintable();
			}
			return CmpAny(buffer1, buffer2);
		}

		// Sort by local filename (upload)
		case ColumnUserFileNameUpload: {
			wxString buffer1, buffer2;
			const CKnownFile * kf1 = client1.GetUploadFile();
			if (kf1) {
				buffer1 = kf1->GetFileName().GetPrintable();
			}
			const CKnownFile * kf2 = client2.GetUploadFile();
			if (kf2) {
				buffer2 = kf2->GetFileName().GetPrintable();
			}
			return CmpAny(buffer1, buffer2);
		}

		case ColumnUserFileNameDownloadRemote: {
			return CmpAny(client1.GetClientFilename(), client2.GetClientFilename());
		}

		default:
			return 0;
	}
}


void CGenericClientListCtrl::ShowSourcesCount( int diff )
{
	m_clientcount += diff;
	wxStaticText* label = CastByID( ID_CLIENTCOUNT, GetParent(), wxStaticText );

	if (label) {	
		label->SetLabel(CFormat(wxT("%i")) % m_clientcount);
		label->GetParent()->Layout();
	}
}

static const CMuleColour crBoth(0, 192, 0);
static const CMuleColour crFlatBoth(0, 150, 0);

static const CMuleColour crNeither(240, 240, 240);
static const CMuleColour crFlatNeither(224, 224, 224);

static const CMuleColour crClientOnly(104, 104, 104);
static const CMuleColour crFlatClientOnly(0, 0, 0);

static const CMuleColour crPending(255, 208, 0);
static const CMuleColour crNextPending(255, 255, 100);

void CGenericClientListCtrl::DrawSourceStatusBar(
	const CClientRef& source, wxDC* dc, const wxRect& rect, bool bFlat) const
{
	static CBarShader s_StatusBar(16);

	CPartFile* reqfile = source.GetRequestFile();

	s_StatusBar.SetHeight(rect.height);
	s_StatusBar.SetWidth(rect.width);
	s_StatusBar.Set3dDepth( thePrefs::Get3DDepth() );
	const BitVector& partStatus = source.GetPartStatus();

	if (reqfile && reqfile->GetPartCount() == partStatus.size()) {
		s_StatusBar.SetFileSize(reqfile->GetFileSize());
		uint16 lastDownloadingPart = source.GetDownloadState() == DS_DOWNLOADING 
									? source.GetLastDownloadingPart() : 0xffff;
		uint16 nextRequestedPart = source.GetNextRequestedPart();

		for ( uint16 i = 0; i < partStatus.size(); i++ ) {
			uint64 uStart = PARTSIZE * i;
			uint64 uEnd = uStart + reqfile->GetPartSize(i) - 1;

			CMuleColour colour;
			if (!partStatus.get(i)) {
				// client does not have this part
				// light grey
				colour = bFlat ? crFlatNeither : crNeither;
			} else if ( reqfile->IsComplete(i)) {
				// completed part
				// green
				colour = bFlat ? crFlatBoth : crBoth;
			} else if (lastDownloadingPart == i) {
				// downloading part
				// yellow
				colour = crPending;
			} else if (nextRequestedPart == i) {
				// requested part
				// light yellow
				colour = crNextPending;
			} else {
				// client has this part, we need it
				// black
				colour = bFlat ? crFlatClientOnly : crClientOnly;
			}

			if ( source.GetRequestFile()->IsStopped() ) {
				colour.Blend(50);
			}

			s_StatusBar.FillRange(uStart, uEnd, colour);
		}
	} else {
		s_StatusBar.SetFileSize(1);
		s_StatusBar.FillRange(0, 1, bFlat ? crFlatNeither : crNeither);
	}

	s_StatusBar.Draw(dc, rect.x, rect.y, bFlat);
}

static const CMuleColour crUnavailable(240, 240, 240);
static const CMuleColour crFlatUnavailable(224, 224, 224);

static const CMuleColour crAvailable(104, 104, 104);
static const CMuleColour crFlatAvailable(0, 0, 0);

void CGenericClientListCtrl::DrawStatusBar( const CClientRef& client, wxDC* dc, const wxRect& rect1 ) const
{
	wxRect rect = rect1;
	rect.y		+= 1;
	rect.height	-= 2;

	wxPen   old_pen   = dc->GetPen();
	wxBrush old_brush = dc->GetBrush();
	bool bFlat = thePrefs::UseFlatBar();

	wxRect barRect = rect;
	if (!bFlat) { // round bar has a black border, the bar itself is 1 pixel less on each border
		barRect.x ++;
		barRect.y ++;
		barRect.height -= 2;
		barRect.width -= 2;
	}
	static CBarShader s_StatusBar(16);

	uint32 partCount = client.GetUpPartCount();

	// Seems the partfile in the client object is not necessarily valid when bar is drawn for the first time.
	// Keep it simple and make all parts same size.
	s_StatusBar.SetFileSize(partCount * PARTSIZE);
	s_StatusBar.SetHeight(barRect.height);
	s_StatusBar.SetWidth(barRect.width);
	s_StatusBar.Set3dDepth( thePrefs::Get3DDepth() );

	uint64 uEnd = 0;
	for ( uint64 i = 0; i < partCount; i++ ) {
		uint64 uStart = PARTSIZE * i;
		uEnd = uStart + PARTSIZE - 1;

		s_StatusBar.FillRange(uStart, uEnd, client.IsUpPartAvailable(i) ? (bFlat ? crFlatAvailable : crAvailable) : (bFlat ? crFlatUnavailable : crUnavailable));
	}
	// fill the rest (if partStatus is empty)
	s_StatusBar.FillRange(uEnd + 1, partCount * PARTSIZE - 1, bFlat ? crFlatUnavailable : crUnavailable);
	s_StatusBar.Draw(dc, barRect.x, barRect.y, bFlat);

	if (!bFlat) {
		// Draw black border
		dc->SetPen( *wxBLACK_PEN );
		dc->SetBrush( *wxTRANSPARENT_BRUSH );
		dc->DrawRectangle(rect);
	}

	dc->SetPen( old_pen );
	dc->SetBrush( old_brush );
}

// File_checked_for_headers
