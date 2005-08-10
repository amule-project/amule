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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ServerListCtrl.h"
#endif

#include <wx/filefn.h>
#include <wx/textfile.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>

#include "ServerListCtrl.h"	// Interface declarations
#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "OtherFunctions.h"		// Needed for CastByName
#include "ServerList.h"		// Needed for CServerList
#include "ServerWnd.h"		// Needed for CServerWnd
#include "ServerConnect.h"		// Needed for CServerConnect
#include "Server.h"			// Needed for CServer and SRV_PR_*
#include "OPCodes.h"		// Needed for MP_PRIO*
#include "Logger.h"
#include "Format.h"


#define SYSCOLOR(x) (wxSystemSettings::GetColour(x))


BEGIN_EVENT_TABLE(CServerListCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK( -1,	CServerListCtrl::OnItemRightClicked)
	EVT_LIST_ITEM_ACTIVATED( -1, 	CServerListCtrl::OnItemActivated )

	EVT_MENU( MP_PRIOLOW,			CServerListCtrl::OnPriorityChange )
	EVT_MENU( MP_PRIONORMAL,		CServerListCtrl::OnPriorityChange )
	EVT_MENU( MP_PRIOHIGH,			CServerListCtrl::OnPriorityChange )
	
	EVT_MENU( MP_ADDTOSTATIC,		CServerListCtrl::OnStaticChange )
	EVT_MENU( MP_REMOVEFROMSTATIC,	CServerListCtrl::OnStaticChange )

	EVT_MENU( MP_CONNECTTO,			CServerListCtrl::OnConnectToServer )
	
	EVT_MENU( MP_REMOVE,			CServerListCtrl::OnRemoveServers )
	EVT_MENU( MP_REMOVEALL,			CServerListCtrl::OnRemoveServers )
	
	EVT_MENU( MP_GETED2KLINK,		CServerListCtrl::OnGetED2kURL )

	EVT_CHAR( CServerListCtrl::OnKeyPressed )
END_EVENT_TABLE()



CServerListCtrl::CServerListCtrl( wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
                                  long style, const wxValidator& validator, const wxString& name )
	: CMuleListCtrl( parent, winid, pos, size, style, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Server") );

	m_connected = 0;

	InsertColumn( COLUMN_SERVER_NAME, _("Server Name"),	wxLIST_FORMAT_LEFT, 150);
	InsertColumn( COLUMN_SERVER_ADDR, _("Address"),			wxLIST_FORMAT_LEFT, 140);
	InsertColumn( COLUMN_SERVER_PORT, _("Port"),			wxLIST_FORMAT_LEFT, 25);
	InsertColumn( COLUMN_SERVER_DESC, _("Description"),	wxLIST_FORMAT_LEFT, 150);
	InsertColumn( COLUMN_SERVER_PING, _("Ping"),			wxLIST_FORMAT_LEFT, 25);
	InsertColumn( COLUMN_SERVER_USERS, _("Users"),		wxLIST_FORMAT_LEFT, 40);
	InsertColumn( COLUMN_SERVER_FILES, _("Files"),		wxLIST_FORMAT_LEFT, 45);
	InsertColumn( COLUMN_SERVER_PRIO, _("Priority"),		wxLIST_FORMAT_LEFT, 60);
	InsertColumn( COLUMN_SERVER_FAILS, _("Failed"),		wxLIST_FORMAT_LEFT, 40);
	InsertColumn( COLUMN_SERVER_STATIC, _("Static"),		wxLIST_FORMAT_LEFT, 40);
	InsertColumn( COLUMN_SERVER_VERSION, _("Version"),		wxLIST_FORMAT_LEFT, 80);

	LoadSettings();
}


CServerListCtrl::~CServerListCtrl()
{

}


void CServerListCtrl::AddServer( CServer* toadd )
{
	// RefreshServer will add the server.
	// This also means that we have simple duplicity checking. ;)
	RefreshServer( toadd );
	
	ShowServerCount();
}


void CServerListCtrl::RemoveServer( const CServer* server, bool ask_static)
{
	long result = FindItem( -1, (long)server );
	if ( result != -1 ) {
		CServer* cur_server = (CServer*) GetItemData( result );
		bool is_static = cur_server->IsStaticMember();
		if ( !ask_static || !is_static || (is_static && (wxMessageBox( CFormat(_("Are you sure you want to delete the static server %s") ) % cur_server->GetListName(), _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ))) {
			theApp.serverlist->RemoveServer( cur_server );
			DeleteItem( result );
		}
	}
	ShowServerCount();
}


void CServerListCtrl::RemoveAllServers( int state, bool ask_static )
{
	int pos = GetNextItem( -1, wxLIST_NEXT_ALL, state);
	bool connected = theApp.IsConnectedED2K() ||
	  theApp.serverconnect->IsConnecting();

	while ( pos != -1 ) {
		if ( (long)GetItemData(pos) == m_connected && connected == true) {
			wxMessageBox(_("You are connected to a server you are trying to delete. Please disconnect first. The server was NOT deleted."), _("Info"), wxOK);
			++pos;
		} else {
			CServer* cur_server = (CServer*) GetItemData( pos );
			bool is_static = cur_server->IsStaticMember();
			const wxString server_name = cur_server->GetListName().IsEmpty() ? wxString(_("(Unknown name)")) : cur_server->GetListName();
			if ( !ask_static || !is_static || (is_static && (wxMessageBox(CFormat(_("Are you sure you want to delete the static server %s")) % server_name, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ))) {
				theApp.serverlist->RemoveServer( cur_server );
				DeleteItem( pos );
			} else {
				++pos;
			}
		}
		
		pos = GetNextItem( pos-1, wxLIST_NEXT_ALL, state );
	}

	ShowServerCount();
}


void CServerListCtrl::RefreshServer( CServer* server )
{
	// Cant really refresh a NULL server 
	if ( !server )
		return;

	// The current stats
	int itemState = 0;

	// The new or old item
	wxListItem item;
	
	long itemnr = FindItem( -1, (long)server );
	if ( itemnr != -1 ) {
		// Try to get the current item, so that we get to keep bold'ness and such
		item.SetId( itemnr );
		GetItem( item );
		
		int sortby = GetSortColumn();
		if ( !GetSortAsc() )
			sortby += SORT_OFFSET_DEC;
	
		// Decides if we should reposition the item (through delete and insert)
		bool sorted = true;
			
		// Check if we are still "smaller" than the item before us
		if ( itemnr > 0 )
			sorted &= ( SortProc( (long)server, GetItemData( itemnr - 1 ), sortby ) >= 0 );

		// Check if we are still "larger" than the item after us
		if ( itemnr < GetItemCount() -1 )
			sorted &= ( SortProc( (long)server, GetItemData( itemnr + 1 ), sortby ) <= 0 );

		if ( !sorted ) {
			itemState = GetItemState( itemnr, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED );
			DeleteItem( itemnr );
			itemnr = -1;
		}
	}
	
	// New item or we deleted the old one
	if ( itemnr == -1 ) {
		// We are not at the sure that the server isn't in the list, so we can re-add
		itemnr = InsertItem( GetInsertPos( (long)server ), server->GetListName() );
		SetItemData( itemnr, (long)server );
	
		item.SetId( itemnr );
		item.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
		SetItem( item );
		
		// Resetting the state, which gets lost when we remove the old item
		SetItemState( itemnr, itemState, itemState );

		// Ensure that the item doesn't just warp out of view
		if ( itemState & wxLIST_STATE_FOCUSED )
			EnsureVisible( itemnr );
	}
	

	SetItem( itemnr, COLUMN_SERVER_NAME, server->GetListName() );
	SetItem( itemnr, COLUMN_SERVER_ADDR, server->GetAddress() );
	if (server->GetAuxPortsList().IsEmpty()) {
		SetItem( itemnr, COLUMN_SERVER_PORT, wxString::Format(wxT("%u"), server->GetPort()));
	} else {
		SetItem( itemnr, COLUMN_SERVER_PORT, wxString::Format(wxT("%u ("), server->GetPort()) + server->GetAuxPortsList() + wxT(")") );
	}
	SetItem( itemnr, COLUMN_SERVER_DESC, server->GetDescription() );
	
	if ( server->GetPing() ) {
		SetItem( itemnr, COLUMN_SERVER_PING, wxString::Format( wxT("%u"), server->GetPing() ) );
	} else {
		SetItem( itemnr, COLUMN_SERVER_PING, wxEmptyString );
	}

	if ( server->GetUsers() ) {
		SetItem( itemnr, COLUMN_SERVER_USERS, wxString::Format( wxT("%u"), server->GetUsers() ) );
	} else {
		SetItem( itemnr, COLUMN_SERVER_USERS, wxEmptyString );
	}

	if ( server->GetFiles() ) {
		SetItem( itemnr, COLUMN_SERVER_FILES, wxString::Format( wxT("%u"), server->GetFiles() ) );
	} else {
		SetItem( itemnr, COLUMN_SERVER_FILES, wxEmptyString );
	}

	switch ( server->GetPreferences() ) {
		case SRV_PR_LOW:	SetItem( itemnr, COLUMN_SERVER_PRIO, _("Low") );		break;
		case SRV_PR_NORMAL:	SetItem( itemnr, COLUMN_SERVER_PRIO, _("Normal") );	break;
		case SRV_PR_HIGH:	SetItem( itemnr, COLUMN_SERVER_PRIO, _("High") );	break;
		default:			SetItem( itemnr, COLUMN_SERVER_PRIO, wxT("---") ); // this should never happen
	}

	SetItem( itemnr, COLUMN_SERVER_FAILS, wxString::Format( wxT("%u"),server->GetFailedCount() ) );
	SetItem( itemnr, COLUMN_SERVER_STATIC, ( server->IsStaticMember() ? _("Yes") : _("No") ) );
	SetItem( itemnr, COLUMN_SERVER_VERSION, server->GetVersion() );
}


void CServerListCtrl::HighlightServer( const CServer* server, bool highlight )
{
	// Unset the old highlighted server if we are going to set a new one
	if ( m_connected && highlight ) {
		// A recursive call to do the real work. 
		HighlightServer( (CServer*)m_connected, false );

		m_connected = 0;
	}
	
	long itemnr = FindItem( -1, (long)server );
	if ( itemnr > -1 ) {
		wxListItem item;
		item.SetId( itemnr );
		
		if ( GetItem( item ) ) {
			wxFont font = GetFont();
			
			if ( highlight ) {
				font.SetWeight( wxBOLD );

				m_connected = (long)server;
			}

			item.SetFont( font );

			SetItem( item );
		}
	}
}


bool CServerListCtrl::SetStaticServer( CServer* server, bool isStatic )
{
	wxString filename = theApp.ConfigDir + wxT("staticservers.dat");
	wxTextFile file( filename );
	
	if ( !wxFileExists( filename ) )
		file.Create();

	if ( !file.Open() ) {
		AddLogLineM( false, CFormat( _("Failed to open '%s'") ) % filename );
		return false;
	}

	
	if ( isStatic ) {
		file.AddLine( server->GetAddress() + wxString::Format( wxT(":%u,%u," ), server->GetPort(), server->GetPreferences() ) + server->GetListName() );
	} else {
		wxString searchStr = server->GetAddress() + wxString::Format( wxT(":%u" ), server->GetPort() );
	
		for ( unsigned int i = 0; i < file.GetLineCount(); ) {
			wxString line = file.GetLine( i );

			// Removing name and priority
			line = line.BeforeLast(wxT(',')).BeforeLast(wxT(','));

			// Remove possible noise
			line.Strip( wxString::both );

			if ( line == searchStr ) {
				file.RemoveLine( i );
				continue;
			}
			
			++i;
		}
	}

	server->SetIsStaticMember( isStatic );
	RefreshServer( server );

	file.Write();
	file.Close();

	return true;
}


void CServerListCtrl::ShowServerCount()
{
	wxStaticText* label = CastByName( wxT("serverListLabel"), GetParent(), wxStaticText );

	if ( label ) {
		label->SetLabel( wxString::Format( _("Servers (%i)"), GetItemCount() ) );
		label->GetParent()->Layout();
	}
}


void CServerListCtrl::OnItemActivated( wxListEvent& event )
{
	// Unselect all items but the activated one
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	while ( item > -1 ) {
		SetItemState( item, 0, wxLIST_STATE_SELECTED);
		
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
		
	SetItemState( event.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

	wxCommandEvent nulEvt;
	OnConnectToServer( nulEvt );
}


void CServerListCtrl::OnItemRightClicked( wxListEvent& event )
{
	// Check if clicked item is selected. If not, unselect all and select it.
	if ( !GetItemState( event.GetIndex(), wxLIST_STATE_SELECTED ) ) {
		
		long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		
		while ( item > -1 ) {
			SetItemState( item, 0, wxLIST_STATE_SELECTED);
			
			item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		}
		
		SetItemState( event.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
	}

	bool enable_reconnect = false;
	bool enable_static_on = false;
	bool enable_static_off = false;

	// Gather information on the selected items
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	while ( index > -1 ) {
		CServer* server = (CServer*)GetItemData( index );

		// The current server is selected, so we might display the reconnect option
		if ( (long)server == m_connected ) 
			enable_reconnect = true;
		
		// We want to know which options should be enabled, either one or both
		enable_static_on	|= !server->IsStaticMember();
		enable_static_off	|=  server->IsStaticMember();
		
		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	
	wxMenu* serverMenu = new wxMenu(_("Server"));
	wxMenu* serverPrioMenu = new wxMenu();
	serverPrioMenu->Append( MP_PRIOLOW, _("Low") );
	serverPrioMenu->Append( MP_PRIONORMAL, _("Normal") );
	serverPrioMenu->Append( MP_PRIOHIGH, _("High") );
	serverMenu->Append( MP_CONNECTTO, _("Connect to server") );
	serverMenu->Append( 12345, _("Priority"), serverPrioMenu );
	
	serverMenu->AppendSeparator();

	serverMenu->Append( MP_ADDTOSTATIC, _("Mark server(s) as static") );
	serverMenu->Append( MP_REMOVEFROMSTATIC, _("Mark server(s) as non-static") );
	
	serverMenu->AppendSeparator();
	
	serverMenu->Append( MP_REMOVE, _("Remove server(s)") );
	serverMenu->Append( MP_REMOVEALL, _("Remove all servers") );
	
	serverMenu->AppendSeparator();
	
	serverMenu->Append( MP_GETED2KLINK, _("Copy ED2k link(s) to clipboard") );


	serverMenu->Enable( MP_REMOVEFROMSTATIC, 	enable_static_off );
	serverMenu->Enable( MP_ADDTOSTATIC,			enable_static_on  );

	if ( GetSelectedItemCount() == 1 ) {
		if ( enable_reconnect )
			serverMenu->SetLabel( MP_CONNECTTO, _("Reconnect to server") );
	} else {
		serverMenu->Enable( MP_CONNECTTO, false );
	}


	PopupMenu( serverMenu, event.GetPoint() );
}


void CServerListCtrl::OnPriorityChange( wxCommandEvent& event )
{
	int priority = 0;

	switch ( event.GetId() ) {
		case MP_PRIOLOW:		priority = SRV_PR_LOW;		break;
		case MP_PRIONORMAL:		priority = SRV_PR_NORMAL;	break;
		case MP_PRIOHIGH:		priority = SRV_PR_HIGH;		break;		
				
		default:
			return;
	}

	
	ItemDataList items = GetSelectedItems();
	
	for ( unsigned int i = 0; i < items.size(); ++i ) {
		CServer* server = (CServer*)items[ i ];
		
		server->SetPreference( priority );
		RefreshServer( server );	
	}
}


void CServerListCtrl::OnStaticChange( wxCommandEvent& event )
{
	bool isStatic = ( event.GetId() == MP_ADDTOSTATIC );

	ItemDataList items = GetSelectedItems();
	
	for ( unsigned int i = 0; i < items.size(); ++i ) {
		CServer* server = (CServer*)items[ i ];

		// Only update items that have the wrong setting
		if ( server->IsStaticMember() != isStatic ) {
			if ( !SetStaticServer( server, isStatic ) ) {
				wxASSERT( false );

				return;
			}
				
			server->SetIsStaticMember( isStatic );
			RefreshServer( server );
		}
	}
}


void CServerListCtrl::OnConnectToServer( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( item > -1 ) {
		if ( theApp.IsConnectedED2K() )
			theApp.serverconnect->Disconnect();

		theApp.serverconnect->ConnectToServer( (CServer*)GetItemData( item ) );
		theApp.amuledlg->ShowConnectionState( false );
	}
}
	

void CServerListCtrl::OnGetED2kURL( wxCommandEvent& WXUNUSED(event) )
{
	int pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	wxString URL;
	
	while ( pos != -1 ) {
		CServer* server = (CServer*)GetItemData(pos);
		
		URL += wxT("ed2k://|server|") + server->GetFullIP() + wxString::Format(wxT("|%d|"), server->GetPort()) + wxT("\n");		
		
		pos = GetNextItem( pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
	
	URL.RemoveLast();
	
	theApp.CopyTextToClipboard( URL );
}


void CServerListCtrl::OnRemoveServers( wxCommandEvent& event )
{
	if ( event.GetId() == MP_REMOVEALL ) {
		if ( GetItemCount() ) {
			wxString question = _("Are you sure that you wish to delete all servers?");
	
			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ) {
				if ( theApp.serverconnect->IsConnecting() ) {
					theApp.downloadqueue->StopUDPRequests();
					theApp.serverconnect->StopConnectionTry();
					theApp.serverconnect->Disconnect();
					theApp.amuledlg->ShowConnectionState( false );
				}
			
				RemoveAllServers( wxLIST_STATE_DONTCARE, true);
			}
		}
	} else if ( event.GetId() == MP_REMOVE ) {
		if ( GetSelectedItemCount() ) {
			wxString question = _("Are you sure that you wish to delete the selected server(s)?");
	
			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ) {
				RemoveAllServers( wxLIST_STATE_SELECTED, true);
			}
		}
	}
}


void CServerListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// Check if delete was pressed
	if ( event.GetKeyCode() == WXK_DELETE ) {
		wxCommandEvent evt;
		evt.SetId( MP_REMOVE );
		OnRemoveServers( evt );
	} else {
		event.Skip();
	}
}


int CServerListCtrl::SortProc( long item1, long item2, long sortData )
{
	CServer* server1 = (CServer*)item1;
	CServer* server2 = (CServer*)item2;

	int mode = 1;
	if ( sortData >= 1000 ) {
		mode = -1;
		sortData -= 1000;
	}
	
	switch ( sortData ) {
		// Sort by server-name
		case COLUMN_SERVER_NAME:
			return mode * server1->GetListName().CmpNoCase(server2->GetListName());
		
		// Sort by address
		case COLUMN_SERVER_ADDR:
			{
				if ( server1->HasDynIP() && server2->HasDynIP()) {
					return mode * server1->GetDynIP().CmpNoCase( server2->GetDynIP() );
				} else if (server1->HasDynIP()) {
					return mode * -1;
				} else if (server2->HasDynIP()) {
					return mode * 1;
				} else {
					uint32 a = server1->GetIP();
					uint32 b = server2->GetIP();
					// This might break on big endian, but who's gonna notice? ;)
					uint32 tester;
					if (!(tester = ((a & 0x000000FF) - (b & 0x000000FF)))) {
						if (!(tester = ((a & 0x0000FF00) - (b & 0x0000FF00)))) {
							if (!(tester = ((a & 0x00FF0000) - (b & 0x00FF0000)))) {
								if (!(tester = ((a & 0xFF000000) - (b & 0xFF000000)))) {
									// Same ip, different port (Shouldn't happen!)
									return 0;
								}
							}
						}
					}
					return (mode * tester);					
				}
			}
		// Sort by port
		case COLUMN_SERVER_PORT: return mode * CmpAny( server1->GetPort(), server2->GetPort() );
		// Sort by description
		case COLUMN_SERVER_DESC: return mode * server1->GetDescription().CmpNoCase( server2->GetDescription() );
		// Sort by Ping
		case COLUMN_SERVER_PING: return mode * CmpAny( server1->GetPing(), server2->GetPing() );
		// Sort by user-count
		case COLUMN_SERVER_USERS: return mode * CmpAny( server1->GetUsers(), server2->GetUsers() );
		// Sort by file-count
		case COLUMN_SERVER_FILES: return mode * CmpAny( server1->GetFiles(), server2->GetFiles() );
		// Sort by priority
		case COLUMN_SERVER_PRIO:
			{
				uint32 srv_pr1 = server1->GetPreferences();
				uint32 srv_pr2 = server2->GetPreferences();
				switch ( srv_pr1 ) {
					case SRV_PR_HIGH:	srv_pr1 = SRV_PR_MAX; break;
					case SRV_PR_NORMAL:	srv_pr1 = SRV_PR_MID; break;
					case SRV_PR_LOW:	srv_pr1 = SRV_PR_MIN; break;
					default:		return 0;
				}
				switch ( srv_pr2 ) {
					case SRV_PR_HIGH:	srv_pr2 = SRV_PR_MAX; break;
					case SRV_PR_NORMAL:	srv_pr2 = SRV_PR_MID; break;
					case SRV_PR_LOW:	srv_pr2 = SRV_PR_MIN; break;
					default:		return 0;
				}
				return mode * CmpAny( srv_pr1, srv_pr2 );
			}
		// Sort by failure-count
		case COLUMN_SERVER_FAILS: return mode * CmpAny( server1->GetFailedCount(), server2->GetFailedCount() );
		// Sort by static servers
		case COLUMN_SERVER_STATIC:
			{
				if ( server2->IsStaticMember() || server1->IsStaticMember() ) {
					if ( server1->IsStaticMember() ) {
						return mode * -1;
					} else {
						return mode *  1;
					}
				} else {
					return 0;
				}
			}
		// Sort by version
		case COLUMN_SERVER_VERSION: return mode * server1->GetVersion().CmpNoCase( server2->GetVersion() );

		default:
			return 0;
	}
}
