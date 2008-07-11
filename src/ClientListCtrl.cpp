//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ClientListCtrl.h"

#include <protocol/ed2k/ClientSoftware.h>
#include <common/MenuIDs.h>

#include "amule.h"
#include "amuleDlg.h"
#include "BarShader.h"		// Needed for CBarShader
#include "ChatWnd.h"
#include "ClientDetailDialog.h"
#include "ClientList.h"
#include "DataToText.h"
#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"	// Needed for IP2Country
#endif
#include "KnownFile.h"
#include "Preferences.h"
#include "updownclient.h"
#include "UploadQueue.h"

#include <wx/menu.h>
#include <wx/textdlg.h>
#include <wx/dc.h>
#include <wx/settings.h>

////////////////////////////////////////////////////////////
// Sorter functions.

int CompareVersions(const CUpDownClient* client1, const CUpDownClient* client2)
{
	if (client1->GetClientSoft() != client2->GetClientSoft()) {
		return client1->GetSoftStr().Cmp(client2->GetSoftStr());
	}

	if (client1->GetVersion() != client2->GetVersion()) {
		return CmpAny(client1->GetVersion(), client2->GetVersion());
	}

	return client1->GetClientModString().Cmp(client2->GetClientModString());
}


////////////////////////////////////////////////////////////
// CClientListCtrl


BEGIN_EVENT_TABLE( CClientListCtrl, CMuleListCtrl )
	EVT_RIGHT_DOWN(CClientListCtrl::OnRightClick)
	EVT_LIST_ITEM_MIDDLE_CLICK(-1, CClientListCtrl::OnMiddleClick)

	EVT_MENU( MP_DETAIL,             CClientListCtrl::OnShowDetails	)
	EVT_MENU( MP_ADDFRIEND,          CClientListCtrl::OnAddFriend	)
	EVT_MENU( MP_SHOWLIST,           CClientListCtrl::OnViewFiles	)
	EVT_MENU( MP_SENDMESSAGE,        CClientListCtrl::OnSendMessage	)
	EVT_MENU( MP_UNBAN,              CClientListCtrl::OnUnbanClient	)
	EVT_MENU_RANGE( MP_SWITCHCTRL_0, MP_SWITCHCTRL_9, CClientListCtrl::OnChangeView	)
END_EVENT_TABLE()


#define m_imagelist theApp->amuledlg->m_imagelist


/**
 * This struct is used to keep track of the different view-types.
 *
 * Each view has a number of attributes, namely a title and serveral functions 
 * which are used by the CClientListCtrl class. This struct is used to store these
 * for easier access.
 *
 * Please note that none of the values are required, however for a fully functional
 * view, it is nescesarry to specificy all of them.
 */
struct ClientListView
{
	//! The name of the view, this is used to load and save column-preferences.
	wxString	m_title;
	
	//! Pointer to the initialization function.
	void		(*m_init)(CClientListCtrl*);

	//! Pointer to the drawing function 
	void		(*m_draw)(CUpDownClient*, int, wxDC*, const wxRect&);
	
	//! Pointer to the sorting function.
	MuleListCtrlCompare	m_sort;

	//! Pointer to the function which returns the old column order.
	wxString	(*m_getOldColumnOrder)();
};


//! This is the list of currently usable views, in the same order as the ViewType enum.
ClientListView g_listViews[] = 
{
	//! None: This view does nothing at all.
	{
		wxEmptyString,
		NULL,
		NULL,
		NULL,
		NULL,
	},

	//! Uploading: The clients currently being uploaded to.
	{
		wxT("Uploads"),
		CUploadingView::Initialize,
		CUploadingView::DrawCell,
		CUploadingView::SortProc,
		CUploadingView::GetOldColumnOrder,
	},

	//! Queued: The clients currently queued for uploading.
	{
		wxT("Queue"),
		CQueuedView::Initialize,
		CQueuedView::DrawCell,
		CQueuedView::SortProc,
		CQueuedView::GetOldColumnOrder,
	},

	//! Clients The complete list of all known clients.
	{
		wxT("Clients"),
		CClientsView::Initialize,
		CClientsView::DrawCell,
		CClientsView::SortProc,
		CClientsView::GetOldColumnOrder,
	}
};



CClientListCtrl::CClientListCtrl( wxWindow *parent, wxWindowID winid, const wxPoint &pos, const wxSize &size, long style, const wxValidator& validator, const wxString& name )
:
CMuleListCtrl( parent, winid, pos, size, style | wxLC_OWNERDRAW, validator, name )
{
	m_viewType = vtNone;
	
	m_menu = NULL;

	m_hilightBrush  = CMuleColour(wxSYS_COLOUR_HIGHLIGHT).Blend(125).GetBrush();

	m_hilightUnfocusBrush = CMuleColour(wxSYS_COLOUR_BTNSHADOW).Blend(125).GetBrush();

	// We show the uploading-list initially
	SetListView( vtUploading );
}


CClientListCtrl::~CClientListCtrl()
{

}


ViewType CClientListCtrl::GetListView()
{
	return m_viewType;
}


void CClientListCtrl::SetListView( ViewType newView )
{
	if ( m_viewType != newView ) {
		if (m_viewType != vtNone) {
			SaveSettings();
			ClearAll();
		}
		
		m_viewType = newView;

		const ClientListView& view = g_listViews[ (int)newView ];
		
		// Initialize the selected view
		if ( view.m_init ) {
			view.m_init( this );
		}
	
		SetTableName( view.m_title );	
		SetSortFunc( view.m_sort );

		if (newView != vtNone) {
			LoadSettings();
		}
	}
}


void CClientListCtrl::ToggleView()
{
	// Disallow toggling if the list is disabled
	if ( m_viewType == vtNone ) {
		return;
	}
	
	unsigned int view = (int)m_viewType + 1;
	
	if ( view < itemsof(g_listViews) ) {
		SetListView( (ViewType)(view) );
	} else {
		SetListView( (ViewType)(1) );
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
void CClientListCtrl::OnRightClick(wxMouseEvent& event)
{
	long index = CheckSelection(event);
	
	if ( m_menu == NULL ) {
		
		bool banned = false;
		bool validIP = false;
		bool isfriend = false;
		bool hasdisabledsharedfiles = false;

		// Check if the client is banned
		if ( index > -1 ) {
			CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));

			banned = client->IsBanned();
			validIP = (client->GetIP() != 0);
			isfriend = client->IsFriend();
			hasdisabledsharedfiles = client->HasDisabledSharedFiles();
		}
				
		m_menu = new wxMenu(_("Clients"));
		m_menu->Append( MP_DETAIL,      _("Show &Details") );
		m_menu->Append( MP_ADDFRIEND,   isfriend ? _("Remove from friends") : _("Add to Friends") );
		m_menu->Append( MP_SHOWLIST,    _("View Files") );
		m_menu->Append( MP_SENDMESSAGE, _("Send message") );
		m_menu->Append( MP_UNBAN,       _("Unban") );
		
		m_menu->AppendSeparator();
	
		wxMenu* view = new wxMenu();
		view->Append( MP_SWITCHCTRL_0 + 1, _("Show Uploads") );
		view->Append( MP_SWITCHCTRL_0 + 2, _("Show Queue") );
		view->Append( MP_SWITCHCTRL_0 + 3, _("Show Clients") );
	
		view->Enable( MP_SWITCHCTRL_0 + (int)m_viewType, false );
		
		m_menu->Append( 0, _("Select View"), view );
		
		m_menu->Enable( MP_DETAIL,      index > -1 );
		m_menu->Enable( MP_SHOWLIST,    index > -1 );
				
		m_menu->Enable( MP_UNBAN,       banned );		
		m_menu->Enable( MP_SHOWLIST,    !hasdisabledsharedfiles );		
		m_menu->Enable( MP_ADDFRIEND,   validIP );
		m_menu->Enable( MP_SENDMESSAGE, validIP );

		PopupMenu( m_menu, event.GetPosition() );
		
		delete m_menu;
		
		m_menu = NULL;
	}
}


void CClientListCtrl::OnMiddleClick(wxListEvent& event)
{
	long index = CheckSelection(event);

	if (index > -1) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));

		CClientDetailDialog dialog(this, client);
		
		dialog.ShowModal();
	}
}


void CClientListCtrl::OnChangeView( wxCommandEvent& event )
{
	int view = event.GetId() - MP_SWITCHCTRL_0;

	SetListView( (ViewType)view );
}

	
void CClientListCtrl::OnAddFriend( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while ( index != -1 ) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));
		if (client->IsFriend()) {
			theApp->amuledlg->m_chatwnd->RemoveFriend(client->GetUserHash(), client->GetIP(), client->GetUserPort());
		} else {
			theApp->amuledlg->m_chatwnd->AddFriend( client );
		}
		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
}


void CClientListCtrl::OnShowDetails( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index > -1 ) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));
		CClientDetailDialog dialog(this, client);
		dialog.ShowModal();
	}
}


void CClientListCtrl::OnViewFiles( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index > -1 ) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));
		client->RequestSharedFileList();
	}
}


void CClientListCtrl::OnSendMessage( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index > -1 ) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));

		// These values are cached, since calling wxGetTextFromUser will
		// start an event-loop, in which the client may be deleted.
		wxString userName = client->GetUserName();
		uint64 userID = GUI_ID(client->GetIP(),client->GetUserPort());

		wxString message = ::wxGetTextFromUser( _("Send message to user"), _("Message to send:") );
		
		if (!message.IsEmpty()) {
			theApp->amuledlg->m_chatwnd->SendMessage(message, userName, userID);
		}
	}
}

		
void CClientListCtrl::OnUnbanClient( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index > -1 ) {
		CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( index ));
		if ( client->IsBanned() ) {
			client->UnBan();
		}		
	}
}


void CClientListCtrl::InsertClient( CUpDownClient* client, ViewType view )
{
	wxCHECK_RET(client, wxT("Attempted to add NULL client pointer."));
	
	if ( ( m_viewType != view ) || ( view == vtNone ) ) {
		return;
	}
	
	long index = InsertItem( GetItemCount(), wxEmptyString );
	SetItemPtrData( index, reinterpret_cast<wxUIntPtr>(client) );

	wxListItem myitem;
	myitem.SetId( index );
	myitem.SetBackgroundColour( CMuleColour(wxSYS_COLOUR_LISTBOX) );
	
	SetItem(myitem);

	RefreshItem( index );
}


void CClientListCtrl::RemoveClient( CUpDownClient* client, ViewType view )
{
	wxCHECK_RET(client, wxT("Attempted to remove NULL client pointer."));

	if ( ( m_viewType != view ) || ( view == vtNone ) ) {
		return;
	}
	
	long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(client) );
	
	if ( index > -1 ) {
		DeleteItem( index );
	}
}


void CClientListCtrl::UpdateClient( CUpDownClient* client, ViewType view )
{
	wxCHECK_RET(client, wxT("Attempted to update NULL client pointer."));

	if ( ( m_viewType != view ) || ( view == vtNone ) ) {
		return;
	}
	
	if ( theApp->amuledlg->IsDialogVisible( CamuleDlg::DT_TRANSFER_WND ) ) {
		// Visible lines, default to all because not all platforms support the GetVisibleLines function
		long first = 0, last = GetItemCount();
		long result = FindItem( -1, reinterpret_cast<wxUIntPtr>(client) );
		if ( result > -1 ) {
#ifndef __WXMSW__
			GetVisibleLines( &first, &last );
#endif
			if ( result >= first && result <= last) {
				RefreshItem(result);
			}
		}
	}
}


void CClientListCtrl::OnDrawItem( int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted )
{
	// Don't do any drawing if we not being watched.
	if ( !theApp->amuledlg || !theApp->amuledlg->IsDialogVisible( CamuleDlg::DT_TRANSFER_WND ) ) {
		return;
	}
	
	if ( highlighted ) {
		if ( GetFocus() ) {
			dc->SetBackground(m_hilightBrush);
			dc->SetTextForeground( CMuleColour(wxSYS_COLOUR_HIGHLIGHTTEXT) );
		} else {
			dc->SetBackground(m_hilightUnfocusBrush);
			dc->SetTextForeground( CMuleColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		}
	
		CMuleColour colour = GetFocus() ? m_hilightBrush.GetColour() : m_hilightUnfocusBrush.GetColour();
		dc->SetPen( *(wxThePenList->FindOrCreatePen( colour.Blend(65), 1, wxSOLID) ));
	} else {
		dc->SetBackground( CMuleColour(wxSYS_COLOUR_LISTBOX).GetBrush());
		dc->SetTextForeground( CMuleColour(wxSYS_COLOUR_WINDOWTEXT) );
		dc->SetPen(*wxTRANSPARENT_PEN);
	}
	
	dc->SetBrush(dc->GetBackground());
	dc->DrawRectangle(rectHL);
	dc->SetPen(*wxTRANSPARENT_PEN);
	
	CUpDownClient* client = reinterpret_cast<CUpDownClient *>(GetItemData(item));
	wxRect cur_rect = rect;
	cur_rect.x += 4;

	const ClientListView& view = g_listViews[ (int)m_viewType ];

	if ( view.m_draw ) {
		for ( int i = 0; i < GetColumnCount(); i++ ) {
			int width = GetColumnWidth( i );
			if ( width ) {
				cur_rect.width = width - 8;
		
				wxDCClipper clipper( *dc, cur_rect );
		
				view.m_draw( client, i, dc, cur_rect );
			}
			cur_rect.x += width;
		}
	}
}


wxString CClientListCtrl::GetTTSText(unsigned item) const
{
	CUpDownClient *client = reinterpret_cast<CUpDownClient *>(GetItemData( item ));

	return client->GetUserName();
}


wxString CClientListCtrl::GetOldColumnOrder() const
{
	const ClientListView& view = g_listViews[ (int)m_viewType ];
	if (view.m_getOldColumnOrder) {
		return view.m_getOldColumnOrder();
	}
	return wxEmptyString;
}


/////////////////////////////////////////////////////////////////////////////////////////////
void CUploadingView::Initialize( CClientListCtrl* list )
{
	list->InsertColumn( 0,	_("Username"),        wxLIST_FORMAT_LEFT, 150, wxT("U") );
	list->InsertColumn( 1,	_("File"),            wxLIST_FORMAT_LEFT, 275, wxT("F") );
	list->InsertColumn( 2,	_("Client Software"), wxLIST_FORMAT_LEFT, 100, wxT("C") );
	list->InsertColumn( 3,	_("Speed"),           wxLIST_FORMAT_LEFT,  60, wxT("S") );
	list->InsertColumn( 4,	_("Transferred"),     wxLIST_FORMAT_LEFT,  65, wxT("T") );
	list->InsertColumn( 5,	_("Waited"),          wxLIST_FORMAT_LEFT,  60, wxT("W") );
	list->InsertColumn( 6,	_("Upload Time"),     wxLIST_FORMAT_LEFT,  60, wxT("u") );
	list->InsertColumn( 7,	_("Status"),          wxLIST_FORMAT_LEFT, 110, wxT("s") );
	list->InsertColumn( 8,	_("Obtained Parts"),  wxLIST_FORMAT_LEFT, 100, wxT("P") );
	list->InsertColumn( 9,	_("Upload/Download"), wxLIST_FORMAT_LEFT, 100, wxT("R") );
	list->InsertColumn( 10,	_("Remote Status"),   wxLIST_FORMAT_LEFT, 100, wxT("r") );			

	// Insert any existing items on the list
	const CClientPtrList& uploading = theApp->uploadqueue->GetUploadingList();
	CClientPtrList::const_iterator it = uploading.begin();
	for (; it != uploading.end(); ++it) {
		list->InsertClient( *it, list->GetListView() );		
	}
}


wxString CUploadingView::GetOldColumnOrder()
{
	return wxT("U,F,C,S,T,W,u,s,P,R,r");
}


void CUploadingView::DrawCell( CUpDownClient* client, int column, wxDC* dc, const wxRect& rect )
{
	wxString buffer;	
	
	switch ( column ) {
	case 0: {
		uint8 clientImage;

		if ( client->IsFriend() ) {
			clientImage = Client_Friend_Smiley;
		} else {
			switch (client->GetClientSoft()) {
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
				// Maybe we would like to make different icons?
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
				// cDonkey, Compat Unk
				// No icon for those yet. Using the eMule one + '?'
				clientImage = Client_Unknown;
				break;
			}
		}

		m_imagelist.Draw(clientImage, *dc, rect.x, rect.y + 1,
			wxIMAGELIST_DRAW_TRANSPARENT);

		if (client->GetScoreRatio() > 1) {
			// Has credits, draw the gold star
			m_imagelist.Draw(Client_CreditsYellow_Smiley, *dc, rect.x, rect.y + 1,
				wxIMAGELIST_DRAW_TRANSPARENT );
		} else if (client->ExtProtocolAvailable()) {
			// Ext protocol -> Draw the '+'
			m_imagelist.Draw(Client_ExtendedProtocol_Smiley, *dc, rect.x, rect.y + 1,
				wxIMAGELIST_DRAW_TRANSPARENT );
		}

		if (client->IsIdentified()) {
			// the 'v'
			m_imagelist.Draw(Client_SecIdent_Smiley, *dc, rect.x, rect.y + 1,
				wxIMAGELIST_DRAW_TRANSPARENT);					
		} else if (client->IsBadGuy()) {
			// the 'X'
			m_imagelist.Draw(Client_BadGuy_Smiley, *dc, rect.x, rect.y + 1,
				wxIMAGELIST_DRAW_TRANSPARENT);					
		}
		
		if (client->HasObfuscatedConnectionBeenEstablished()) {
			// the "¿" except it's a key
			m_imagelist.Draw(Client_Encryption_Smiley, *dc, rect.x, rect.y + 1,
				wxIMAGELIST_DRAW_TRANSPARENT);					
		}

		wxString userName;
#ifdef ENABLE_IP2COUNTRY
		// Draw the flag
		const CountryData& countrydata =
			theApp->amuledlg->m_IP2Country->GetCountryData(client->GetFullIP());
		dc->DrawBitmap(countrydata.Flag, rect.x + 20, rect.y + 5,
			wxIMAGELIST_DRAW_TRANSPARENT);
			
		userName << countrydata.Name;
		userName << wxT(" - ");
#endif // ENABLE_IP2COUNTRY
		userName << client->GetUserName();
		dc->DrawText(userName, rect.x + 40, rect.y + 3);
		
		return;
	}

	case 1:
		if ( client->GetUploadFile() ) {
			buffer = client->GetUploadFile()->GetFileName().GetPrintable();
		} else {
			buffer = _("N/A");
		}
		break;

	case 2:
		buffer = client->GetClientVerString();
		break;
	
	case 3:
		buffer = wxString::Format( wxT("%.1f"), client->GetUploadDatarate() / 1024.0f );
	
		buffer += wxT(" ");
		buffer += _("kB/s");
		break;
		
	case 4:
		buffer = CastItoXBytes(client->GetSessionUp());
		break;
	
	case 5:
		buffer = CastSecondsToHM((client->GetWaitTime())/1000);
		break;
	
	case 6:
		buffer = CastSecondsToHM((client->GetUpStartTimeDelay())/1000);
		break;
	
	case 7:
		switch ( client->GetUploadState() ) {
		case US_CONNECTING:
			buffer = _("Connecting");
			break;
			
		case US_WAITCALLBACK:
			buffer = _("Connecting via server");
			break;
			
		case US_UPLOADING:
			buffer = wxT("<-- ");
			buffer.Append(_("Transferring"));
			
			if (client->GetDownloadState() == DS_DOWNLOADING) {
				buffer.Append(wxT(" -->"));
			}
			break;
			
		case US_ONUPLOADQUEUE:
			buffer = _("On Queue");
			break;
			
		default:
			buffer = _("Unknown");
		}
		break;
		
	case 8:
		if ( client->GetUpPartCount() ) {
			CUploadingView::DrawStatusBar( client, dc, rect );
		}
		return;
	
	case 9:
		buffer = CastItoXBytes( client->GetUploadedTotal() ) +
			wxT(" / ") + CastItoXBytes(client->GetDownloadedTotal());
		break;
 
	case 10: 
		if ( client->GetDownloadState() == DS_ONQUEUE ) { 
			if ( client->IsRemoteQueueFull() ) { 
				buffer = _("Queue Full"); 
			} else { 
				if (client->GetRemoteQueueRank()) { 
					buffer = wxString::Format(_("QR: %u"), client->GetRemoteQueueRank()); 
				} else { 
					buffer = _("Unknown"); 
				} 
			} 
		} else if ( client->GetDownloadState() == DS_DOWNLOADING ) {
			buffer += wxString::Format( wxT("%.1f"), client->GetKBpsDown() ); 
			buffer += wxT(" ");
			buffer += _("kB/s");
		} else { 
			buffer = _("Unknown"); 
		} 
		break;
	}

	dc->DrawText( buffer, rect.x, rect.y + 3 );
}


int CUploadingView::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CUpDownClient* client1 = (CUpDownClient*)item1;
	CUpDownClient* client2 = (CUpDownClient*)item2;

	// Sorting ascending or decending
	int mode = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	
	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
	// Sort by username
	case 0:	return mode * client1->GetUserName().CmpNoCase( client2->GetUserName() );

	
	// Sort by requested file
	case 1: {
		const CKnownFile* file1 = client1->GetUploadFile();
		const CKnownFile* file2 = client2->GetUploadFile();

		if ( file1 && file2  ) {
			return mode * CmpAny(file1->GetFileName(), file2->GetFileName());
		} 
		
		return mode * CmpAny( file1, file2 );
	}
	
	// Sort by client software
	case 2: return mode * CompareVersions(client1, client2);
	
	// Sort by speed
	case 3: return mode * CmpAny( client1->GetUploadDatarate(), client2->GetUploadDatarate() );
	
	// Sort by transferred
	case 4: return mode * CmpAny( client1->GetSessionUp(), client2->GetSessionUp() );
	
	// Sort by wait-time
	case 5: return mode * CmpAny( client1->GetWaitTime(), client2->GetWaitTime() );
	
	// Sort by upload time
	case 6: return mode * CmpAny( client1->GetUpStartTimeDelay(), client2->GetUpStartTimeDelay() );
	
	// Sort by state
	case 7: return mode * CmpAny( client1->GetUploadState(), client2->GetUploadState() );
	
	// Sort by partcount
	case 8: return mode * CmpAny( client1->GetUpPartCount(), client2->GetUpPartCount() );
	
	// Sort by U/D ratio
	case 9: return mode * CmpAny( client2->GetDownloadedTotal(), client1->GetDownloadedTotal() );
	
	// Sort by remote rank
	case 10: return mode * CmpAny( client2->GetRemoteQueueRank(), client1->GetRemoteQueueRank() );

	default:
		return 0;
	}
}

static const CMuleColour crUnavailable(240, 240, 240);
static const CMuleColour crFlatUnavailable(224, 224, 224);

static const CMuleColour crAvailable(104, 104, 104);
static const CMuleColour crFlatAvailable(0, 0, 0);

void CUploadingView::DrawStatusBar( CUpDownClient* client, wxDC* dc, const wxRect& rect1 )
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

	uint32 partCount = client->GetUpPartCount();

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

		s_StatusBar.FillRange(uStart, uEnd, client->IsUpPartAvailable(i) ? (bFlat ? crFlatAvailable : crAvailable) : (bFlat ? crFlatUnavailable : crUnavailable));
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


/////////////////////////////////////////////////////////////////////////////////////////////
void CQueuedView::Initialize( CClientListCtrl* list )
{
	list->InsertColumn( 0,	_("Username"),        wxLIST_FORMAT_LEFT, 150, wxT("U") );
	list->InsertColumn( 1,	_("File"),            wxLIST_FORMAT_LEFT, 275, wxT("F") );
	list->InsertColumn( 2,	_("Client Software"), wxLIST_FORMAT_LEFT, 100, wxT("C") );
	list->InsertColumn( 3,	_("File Priority"),   wxLIST_FORMAT_LEFT, 110, wxT("p") );
	list->InsertColumn( 4,	_("Rating"),          wxLIST_FORMAT_LEFT,  60, wxT("R") );
	list->InsertColumn( 5,	_("Score"),           wxLIST_FORMAT_LEFT,  60, wxT("c") );
	list->InsertColumn( 6,	_("Asked"),           wxLIST_FORMAT_LEFT,  60, wxT("A") );
	list->InsertColumn( 7,	_("Last Seen"),       wxLIST_FORMAT_LEFT, 110, wxT("L") );
	list->InsertColumn( 8,	_("Entered Queue"),   wxLIST_FORMAT_LEFT, 110, wxT("Q") );
	list->InsertColumn( 9,	_("Banned"),          wxLIST_FORMAT_LEFT,  60, wxT("B") );
	list->InsertColumn( 10,	_("Obtained Parts"),  wxLIST_FORMAT_LEFT, 100, wxT("P") );

	// Insert any existing items on the list
	const CClientPtrList& uploading = theApp->uploadqueue->GetWaitingList();
	CClientPtrList::const_iterator it = uploading.begin();
	for (; it != uploading.end(); ++it) {
		list->InsertClient( *it, list->GetListView() );		
	}
}


wxString CQueuedView::GetOldColumnOrder()
{
	return wxT("U,F,C,p,R,c,A,L,Q,B,P");
}


void CQueuedView::DrawCell( CUpDownClient* client, int column, wxDC* dc, const wxRect& rect )
{
	wxString buffer;

	switch ( column ) {
	// These 3 are the same for both lists
	case 0:
	case 1:
	case 2:
		CUploadingView::DrawCell( client, column, dc, rect );
		return;

	case 3:
		if ( client->GetUploadFile() ) {
			buffer = PriorityToStr( client->GetUploadFile()->GetUpPriority(), false );
		} else {
			buffer = _("Unknown");
		}

		break;
		
	case 4:
		buffer = wxString::Format( wxT("%.1f"), (float)client->GetScore(false,false,true) );
		break;
	
	case 5:
		if ( client->HasLowID() ) {
			buffer = wxString::Format( wxT("%i %s"), client->GetScore(false), _("LowID") );
		} else {
			buffer = wxString::Format(wxT("%i"),client->GetScore(false));
		}
		break;
		
	case 6:
		buffer = wxString::Format( wxT("%i"), client->GetAskedCount() );
		break;
		
#ifndef CLIENT_GUI
	case 7:
		buffer = CastSecondsToHM((::GetTickCount() - client->GetLastUpRequest())/1000);
		break;
	
	case 8:
		buffer = CastSecondsToHM((::GetTickCount() - client->GetWaitStartTime())/1000);
		break;
#else
	case 7:
		buffer = CastSecondsToHM(client->GetLastUpRequest()/1000);
		break;
	
	case 8:
		buffer = CastSecondsToHM(client->GetWaitStartTime()/1000);
		break;
#endif
	case 9:
		if ( client->IsBanned() ) {
			buffer = _("Yes");
		} else {
			buffer = _("No");
		}
		
		break;
		
	case 10:
		if ( client->GetUpPartCount() ) {
			CUploadingView::DrawStatusBar( client, dc, rect );
		}
		
		return;
	}
			
	dc->DrawText( buffer, rect.x, rect.y + 3 );
}


int CQueuedView::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CUpDownClient* client1 = (CUpDownClient*)item1;
	CUpDownClient* client2 = (CUpDownClient*)item2;

	// Ascending or decending?
	int mode = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;

	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
	// Sort by username
	case 0: return mode * client1->GetUserName().CmpNoCase( client2->GetUserName() );
	
	// Sort by filename
	case 1: {
		const CKnownFile* file1 = client1->GetUploadFile();
		const CKnownFile* file2 = client2->GetUploadFile();

		if ( file1 && file2 ) {
			return mode * CmpAny(file1->GetFileName(), file2->GetFileName());
		}

		// Place files with filenames on top
		return -mode * CmpAny( file1, file2 );
	}
	
	// Sort by client software
	case 2:	return mode * CompareVersions(client1, client2);
	
	// Sort by file upload-priority
	case 3: {
		const CKnownFile* file1 = client1->GetUploadFile();
		const CKnownFile* file2 = client2->GetUploadFile();

		if ( file1 && file2  ) {
			int8 prioA = file1->GetUpPriority();
			int8 prioB = file2->GetUpPriority();

			// Work-around for PR_VERYLOW which has value 4. See KnownFile.h for that stupidity ...
			return mode * CmpAny( ( prioA != PR_VERYLOW ? prioA : -1 ), ( prioB != PR_VERYLOW ? prioB : -1 ) );
		} 
		
		// Place files with priorities on top
		return -mode * CmpAny( file1, file2 );
	}
	
	// Sort by rating
	case 4: return mode * CmpAny( client1->GetScore(false,false,true), client2->GetScore(false,false,true) );

	// Sort by score
	case 5: return mode * CmpAny( client1->GetScore(false), client2->GetScore(false) );
	
	// Sort by Asked count
	case 6:	return mode * CmpAny( client1->GetAskedCount(), client2->GetAskedCount() );
	
	// Sort by Last seen
	case 7: return mode * CmpAny( client1->GetLastUpRequest(), client2->GetLastUpRequest() );
	
	// Sort by entered time
	case 8: return mode * CmpAny( client1->GetWaitStartTime(), client2->GetWaitStartTime() );

	// Sort by banned
	case 9: return mode * CmpAny( client1->IsBanned(), client2->IsBanned() );

	default: return 0;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
void CClientsView::Initialize( CClientListCtrl* list )
{
	list->InsertColumn( 0, _("Username"),          wxLIST_FORMAT_LEFT, 150, wxT("U") );
	list->InsertColumn( 1, _("Upload Status"),     wxLIST_FORMAT_LEFT, 150, wxT("S") );
	list->InsertColumn( 2, _("Transferred Up"),    wxLIST_FORMAT_LEFT, 150, wxT("T") );
	list->InsertColumn( 3, _("Download Status"),   wxLIST_FORMAT_LEFT, 150, wxT("s") );
	list->InsertColumn( 4, _("Transferred Down"),  wxLIST_FORMAT_LEFT, 150, wxT("t") );
	list->InsertColumn( 5, _("Client Software"),   wxLIST_FORMAT_LEFT, 150, wxT("C") );
	list->InsertColumn( 6, _("Connected"),         wxLIST_FORMAT_LEFT, 150, wxT("c") );
	list->InsertColumn( 7, _("Userhash"),          wxLIST_FORMAT_LEFT, 150, wxT("H") );
	list->InsertColumn( 8, _("Encrypted"),         wxLIST_FORMAT_LEFT, 100, wxT("E") );			
	list->InsertColumn( 9, _("Hide shared files"), wxLIST_FORMAT_LEFT, 100, wxT("h") );			

	const CClientList::IDMap& clist = theApp->clientlist->GetClientList();
	CClientList::IDMap::const_iterator it = clist.begin();
	
	for ( ; it != clist.end(); ++it ) {
		list->InsertClient( it->second, list->GetListView() );
	}
}


wxString CClientsView::GetOldColumnOrder()
{
	return wxT("U,S,T,s,t,C,c,H,E,h");
}


void CClientsView::DrawCell( CUpDownClient* client, int column, wxDC* dc, const wxRect& rect )
{
	wxString buffer;
	
	switch ( column ) {
		case 0:
		CUploadingView::DrawCell( client, column, dc, rect );
		return;
	
	case 1:
		CUploadingView::DrawCell( client, 7, dc, rect );
		return;
	
	case 2:
		buffer = CastItoXBytes( client->GetUploadedTotal() );
		
		break;
		
	case 3:
		buffer = DownloadStateToStr( client->GetDownloadState(),
					     client->IsRemoteQueueFull() );
		break;

	case 4:
		buffer = CastItoXBytes( client->GetDownloadedTotal() );
		break;
		
	case 5:
		buffer = client->GetClientVerString();
		break;
		
	case 6:
		if ( client->IsConnected() ) {
			buffer = _("Yes");
		} else {
			buffer = _("No");
		}
		
		break;
		
	case 7:
		buffer = client->GetUserHash().Encode();
		break;

	case 8:
		buffer = client->HasObfuscatedConnectionBeenEstablished() ?
			_("Yes") : _("No");
		break;		

	case 9:
		buffer = client->GetUserName().IsEmpty() ?
			wxT("?") :
			(client->HasDisabledSharedFiles() ? _("Yes") : _("No"));
		break;		
	}

	dc->DrawText( buffer, rect.x, rect.y + 3 );
}


int CClientsView::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CUpDownClient* client1 = (CUpDownClient*)item1;
	CUpDownClient* client2 = (CUpDownClient*)item2;

	// Ascending or decending?
	int mode = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;

	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
	// Sort by Username
	case 0: return mode * client1->GetUserName().CmpNoCase( client2->GetUserName() );
	
	// Sort by Uploading-state
	case 1: return mode * CmpAny( client1->GetUploadState(), client2->GetUploadState() );
	
	// Sort by data-uploaded
	case 2:			
		return mode * CmpAny( client1->GetUploadedTotal(), client2->GetUploadedTotal() );
	
	// Sort by Downloading-state
	case 3:
		if( client1->GetDownloadState() == client2->GetDownloadState() ){
			if( client1->IsRemoteQueueFull() && client2->IsRemoteQueueFull() ) {
				return mode *  0;
			} else if( client1->IsRemoteQueueFull() ) {
				return mode *  1;
			} else if( client2->IsRemoteQueueFull() ) {
				return mode * -1;
			} else {
				return mode *  0;
			}
		}
		return mode * CmpAny( client1->GetDownloadState(), client2->GetDownloadState() );
	
	// Sort by data downloaded
	case 4:
		return mode * CmpAny( client1->GetDownloadedTotal(), client2->GetDownloadedTotal() );
	
	// Sort by client-software
	case 5: return mode * CompareVersions(client1, client2);
	
	// Sort by connection
	case 6: return mode * CmpAny( client1->IsConnected(), client2->IsConnected() );

	// Sort by user-hash
	case 7: return mode * CmpAny( client1->GetUserHash(), client2->GetUserHash() );
	
	// Sort by Obfuscation state
	case 8: return mode * CmpAny( client2->HasObfuscatedConnectionBeenEstablished(), client1->HasObfuscatedConnectionBeenEstablished() );
		
	// Sort by Shared Files DISabled
	case 9: return mode * CmpAny( client2->HasDisabledSharedFiles(), client1->HasDisabledSharedFiles() );

	default:
		return 0;
	}

}
// File_checked_for_headers
