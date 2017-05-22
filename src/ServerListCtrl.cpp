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

#include "ServerListCtrl.h"	// Interface declarations

#include <common/MenuIDs.h>

#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>

#include "amule.h"		// Needed for theApp
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"	// Needed for IP2Country
	#include "amuleDlg.h"	// Needed for IP2Country
#endif
#include "ServerList.h"		// Needed for CServerList
#include "ServerConnect.h"	// Needed for CServerConnect
#include "Server.h"		// Needed for CServer and SRV_PR_*
#include "Logger.h"
#include <common/Format.h>	// Needed for CFormat
#include "Preferences.h"	// Needed for thePrefs


#define CMuleColour(x) (wxSystemSettings::GetColour(x))


BEGIN_EVENT_TABLE(CServerListCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK( -1,	CServerListCtrl::OnItemRightClicked)
	EVT_LIST_ITEM_ACTIVATED( -1,	CServerListCtrl::OnItemActivated )

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

	InsertColumn( COLUMN_SERVER_NAME,	_("Server Name"),	wxLIST_FORMAT_LEFT, 150, wxT("N") );
	InsertColumn( COLUMN_SERVER_ADDR,	_("Address"),		wxLIST_FORMAT_LEFT, 140, wxT("A") );
	InsertColumn( COLUMN_SERVER_PORT,	_("Port"),		wxLIST_FORMAT_LEFT,  25, wxT("P") );
	InsertColumn( COLUMN_SERVER_DESC,	_("Description"),	wxLIST_FORMAT_LEFT, 150, wxT("D") );
	InsertColumn( COLUMN_SERVER_PING,	_("Ping"),		wxLIST_FORMAT_LEFT,  25, wxT("p") );
	InsertColumn( COLUMN_SERVER_USERS,	_("Users"),		wxLIST_FORMAT_LEFT,  40, wxT("U") );
	InsertColumn( COLUMN_SERVER_FILES,	_("Files"),		wxLIST_FORMAT_LEFT,  45, wxT("F") );
	InsertColumn( COLUMN_SERVER_PRIO,	_("Priority"),		wxLIST_FORMAT_LEFT,  60, wxT("r") );
	InsertColumn( COLUMN_SERVER_FAILS,	_("Failed"),		wxLIST_FORMAT_LEFT,  40, wxT("f") );
	InsertColumn( COLUMN_SERVER_STATIC,	_("Static"),		wxLIST_FORMAT_LEFT,  40, wxT("S") );
	InsertColumn( COLUMN_SERVER_VERSION,	_("Version"),		wxLIST_FORMAT_LEFT,  80, wxT("V") );
	#ifdef __DEBUG__
	InsertColumn( COLUMN_SERVER_TCPFLAGS,	wxT("TCP Flags"),	wxLIST_FORMAT_LEFT,  80, wxT("t") );
	InsertColumn( COLUMN_SERVER_UDPFLAGS,	wxT("UDP Flags"),	wxLIST_FORMAT_LEFT,  80, wxT("u") );
	#endif


	LoadSettings();
}


wxString CServerListCtrl::GetOldColumnOrder() const
{
	return wxT("N,A,P,D,p,U,F,r,f,S,V,t,u");
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


void CServerListCtrl::RemoveServer(CServer* server)
{
	long result = FindItem(-1, reinterpret_cast<wxUIntPtr>(server));
	if ( result != -1 ) {
		DeleteItem(result);
		ShowServerCount();
	}
}


void CServerListCtrl::RemoveAllServers(int state)
{
	int pos = GetNextItem( -1, wxLIST_NEXT_ALL, state);
	bool connected = theApp->IsConnectedED2K() ||
	  theApp->serverconnect->IsConnecting();

	while ( pos != -1 ) {
		CServer* server = reinterpret_cast<CServer*>(GetItemData(pos));

		if (server == m_connected && connected) {
			wxMessageBox(_("You are connected to a server you are trying to delete. Please disconnect first. The server was NOT deleted."), _("Info"), wxOK, this);
			++pos;
		} else if (server->IsStaticMember()) {
			const wxString name = (!server->GetListName() ? wxString(_("(Unknown name)")) : server->GetListName());

			if (wxMessageBox(CFormat(_("Are you sure you want to delete the static server %s")) % name, _("Cancel"), wxICON_QUESTION | wxYES_NO, this) == wxYES) {
				theApp->serverlist->SetStaticServer(server, false);
				DeleteItem( pos );
				theApp->serverlist->RemoveServer( server );
			} else {
				++pos;
			}
		} else {
			DeleteItem( pos );
			theApp->serverlist->RemoveServer( server );
		}

		pos = GetNextItem(pos - 1, wxLIST_NEXT_ALL, state);
	}

	ShowServerCount();
}


void CServerListCtrl::RefreshServer( CServer* server )
{
	// Cant really refresh a NULL server
	if (!server) {
		return;
	}

	wxUIntPtr ptr = reinterpret_cast<wxUIntPtr>(server);
	long itemnr = FindItem( -1, ptr );
	if ( itemnr == -1 ) {
		// We are not at the sure that the server isn't in the list, so we can re-add
		itemnr = InsertItem( GetInsertPos( ptr ), server->GetListName() );
		SetItemPtrData( itemnr, ptr );

		wxListItem item;
		item.SetId( itemnr );
		item.SetBackgroundColour(CMuleColour(wxSYS_COLOUR_LISTBOX));
		SetItem( item );
	}

	wxString serverName;
#ifdef ENABLE_IP2COUNTRY
	// Get the country name
	if (theApp->amuledlg->m_IP2Country->IsEnabled() && thePrefs::IsGeoIPEnabled()) {
		const CountryData& countrydata = theApp->amuledlg->m_IP2Country->GetCountryData(server->GetFullIP());
		serverName << countrydata.Name;
		serverName << wxT(" - ");
	}
#endif // ENABLE_IP2COUNTRY
	serverName << server->GetListName();
	SetItem(itemnr, COLUMN_SERVER_NAME, serverName);
	SetItem(itemnr, COLUMN_SERVER_ADDR, server->GetAddress());
	if (server->GetAuxPortsList().IsEmpty()) {
		SetItem( itemnr, COLUMN_SERVER_PORT,
			CFormat(wxT("%u")) % server->GetPort());
	} else {
		SetItem( itemnr, COLUMN_SERVER_PORT,
			CFormat(wxT("%u (%s)")) % server->GetPort() % server->GetAuxPortsList());
	}
	SetItem( itemnr, COLUMN_SERVER_DESC, server->GetDescription() );

	if ( server->GetPing() ) {
		SetItem( itemnr, COLUMN_SERVER_PING,
			CastSecondsToHM(server->GetPing()/1000, server->GetPing() % 1000 ) );
	} else {
		SetItem( itemnr, COLUMN_SERVER_PING, wxEmptyString );
	}

	if ( server->GetUsers() ) {
		SetItem( itemnr, COLUMN_SERVER_USERS,
			CFormat(wxT("%u")) % server->GetUsers());
	} else {
		SetItem( itemnr, COLUMN_SERVER_USERS, wxEmptyString );
	}

	if ( server->GetFiles() ) {
		SetItem( itemnr, COLUMN_SERVER_FILES,
			CFormat(wxT("%u")) % server->GetFiles());
	} else {
		SetItem( itemnr, COLUMN_SERVER_FILES, wxEmptyString );
	}

	switch ( server->GetPreferences() ) {
		case SRV_PR_LOW:	SetItem(itemnr, COLUMN_SERVER_PRIO, _("Low"));		break;
		case SRV_PR_NORMAL:	SetItem(itemnr, COLUMN_SERVER_PRIO, _("Normal"));	break;
		case SRV_PR_HIGH:	SetItem(itemnr, COLUMN_SERVER_PRIO, _("High") );	break;
		default:		SetItem(itemnr, COLUMN_SERVER_PRIO, wxT("---"));	// this should never happen
	}

	SetItem( itemnr, COLUMN_SERVER_FAILS, CFormat(wxT("%u")) % server->GetFailedCount());
	SetItem( itemnr, COLUMN_SERVER_STATIC, ( server->IsStaticMember() ? _("Yes") : _("No") ) );
	SetItem( itemnr, COLUMN_SERVER_VERSION, server->GetVersion() );

	#if defined(__DEBUG__) && !defined(CLIENT_GUI)
	wxString flags;
	/* TCP */
	if (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION) {
		flags += wxT("c");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_NEWTAGS) {
		flags += wxT("n");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_UNICODE) {
		flags += wxT("u");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_RELATEDSEARCH) {
		flags += wxT("r");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_TYPETAGINTEGER) {
		flags += wxT("t");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_LARGEFILES) {
		flags += wxT("l");
	}
	if (server->GetTCPFlags() & SRV_TCPFLG_TCPOBFUSCATION) {
		flags += wxT("o");
	}

	SetItem( itemnr, COLUMN_SERVER_TCPFLAGS, flags );

	/* UDP */
	flags.Clear();
	if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES) {
		flags += wxT("g");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
		flags += wxT("f");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_NEWTAGS) {
		flags += wxT("n");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_UNICODE) {
		flags += wxT("u");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES2) {
		flags += wxT("G");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_LARGEFILES) {
		flags += wxT("l");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_UDPOBFUSCATION) {
		flags += wxT("o");
	}
	if (server->GetUDPFlags() & SRV_UDPFLG_TCPOBFUSCATION) {
		flags += wxT("O");
	}
	SetItem( itemnr, COLUMN_SERVER_UDPFLAGS, flags );

	#endif

	// Deletions of items causes rather large ammount of flicker, so to
	// avoid this, we resort the list to ensure correct ordering.
	if (!IsItemSorted(itemnr)) {
		SortList();
	}
}


void CServerListCtrl::HighlightServer( const CServer* server, bool highlight )
{
	// Unset the old highlighted server if we are going to set a new one
	if ( m_connected && highlight ) {
		// A recursive call to do the real work.
		HighlightServer( m_connected, false );

		m_connected = 0;
	}

	long itemnr = FindItem( -1,  reinterpret_cast<wxUIntPtr>(server) );
	if ( itemnr > -1 ) {
		wxListItem item;
		item.SetId( itemnr );

		if ( GetItem( item ) ) {
			wxFont font = GetFont();

			if ( highlight ) {
				font.SetWeight( wxBOLD );

				m_connected = server;
			}

			item.SetFont( font );

			SetItem( item );
		}
	}
}


void CServerListCtrl::ShowServerCount()
{
	wxStaticText* label = CastByName( wxT("serverListLabel"), GetParent(), wxStaticText );

	if ( label ) {
		label->SetLabel(CFormat(_("Servers (%i)")) % GetItemCount());
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


void CServerListCtrl::OnItemRightClicked(wxListEvent& event)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long index = CheckSelection(event);

	bool enable_reconnect = false;
	bool enable_static_on = false;
	bool enable_static_off = false;

	// Gather information on the selected items
	while ( index > -1 ) {
		CServer* server = reinterpret_cast<CServer*>(GetItemData(index));

		// The current server is selected, so we might display the reconnect option
		if (server == m_connected) {
			enable_reconnect = true;
		}

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

	if (GetSelectedItemCount() == 1) {
		serverMenu->Append( MP_ADDTOSTATIC, _("Mark server as static") );
		serverMenu->Append( MP_REMOVEFROMSTATIC, _("Mark server as non-static") );
	} else {
		serverMenu->Append( MP_ADDTOSTATIC, _("Mark servers as static") );
		serverMenu->Append( MP_REMOVEFROMSTATIC, _("Mark servers as non-static") );
	}

	serverMenu->AppendSeparator();

	if (GetSelectedItemCount() == 1) {
		serverMenu->Append( MP_REMOVE, _("Remove server") );
	} else {
		serverMenu->Append( MP_REMOVE, _("Remove servers") );
	}
	serverMenu->Append( MP_REMOVEALL, _("Remove all servers") );

	serverMenu->AppendSeparator();

	if (GetSelectedItemCount() == 1) {
		serverMenu->Append( MP_GETED2KLINK, _("Copy eD2k link to clipboard") );
	} else {
		serverMenu->Append( MP_GETED2KLINK, _("Copy eD2k links to clipboard") );
	}

	serverMenu->Enable( MP_REMOVEFROMSTATIC,	enable_static_off );
	serverMenu->Enable( MP_ADDTOSTATIC,			enable_static_on  );

	if ( GetSelectedItemCount() == 1 ) {
		if ( enable_reconnect )
			serverMenu->SetLabel( MP_CONNECTTO, _("Reconnect to server") );
	} else {
		serverMenu->Enable( MP_CONNECTTO, false );
	}


	PopupMenu( serverMenu, event.GetPoint() );
	delete serverMenu;
}


void CServerListCtrl::OnPriorityChange( wxCommandEvent& event )
{
	uint32 priority = 0;

	switch ( event.GetId() ) {
		case MP_PRIOLOW:		priority = SRV_PR_LOW;		break;
		case MP_PRIONORMAL:		priority = SRV_PR_NORMAL;	break;
		case MP_PRIOHIGH:		priority = SRV_PR_HIGH;		break;

		default:
			return;
	}


	ItemDataList items = GetSelectedItems();

	for ( unsigned int i = 0; i < items.size(); ++i ) {
		CServer* server = reinterpret_cast<CServer*>(items[i]);
		theApp->serverlist->SetServerPrio(server, priority);
	}
}


void CServerListCtrl::OnStaticChange( wxCommandEvent& event )
{
	bool isStatic = ( event.GetId() == MP_ADDTOSTATIC );

	ItemDataList items = GetSelectedItems();

	for ( unsigned int i = 0; i < items.size(); ++i ) {
		CServer* server = reinterpret_cast<CServer*>(items[i]);

		// Only update items that have the wrong setting
		if ( server->IsStaticMember() != isStatic ) {
			theApp->serverlist->SetStaticServer(server, isStatic);
		}
	}
}


void CServerListCtrl::OnConnectToServer( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

	if ( item > -1 ) {
		if ( theApp->IsConnectedED2K() ) {
			theApp->serverconnect->Disconnect();
		}

		theApp->serverconnect->ConnectToServer( reinterpret_cast<CServer*>(GetItemData(item)) );
	}
}


void CServerListCtrl::OnGetED2kURL( wxCommandEvent& WXUNUSED(event) )
{
	int pos = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

	wxString URL;

	while ( pos != -1 ) {
		CServer* server = reinterpret_cast<CServer*>(GetItemData(pos));

		URL += CFormat(wxT("ed2k://|server|%s|%d|/\n"))	% server->GetFullIP() % server->GetPort();

		pos = GetNextItem( pos, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	URL.RemoveLast();

	theApp->CopyTextToClipboard( URL );
}


void CServerListCtrl::OnRemoveServers( wxCommandEvent& event )
{
	if ( event.GetId() == MP_REMOVEALL ) {
		if ( GetItemCount() ) {
			wxString question = _("Are you sure that you wish to delete all servers?");

			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO, this) == wxYES ) {
				if ( theApp->serverconnect->IsConnecting() ) {
					theApp->downloadqueue->StopUDPRequests();
					theApp->serverconnect->StopConnectionTry();
					theApp->serverconnect->Disconnect();
				}

				RemoveAllServers(wxLIST_STATE_DONTCARE);
			}
		}
	} else if ( event.GetId() == MP_REMOVE ) {
		if ( GetSelectedItemCount() ) {
			wxString question;
			if (GetSelectedItemCount() == 1) {
				question = _("Are you sure that you wish to delete the selected server?");
			} else {
				question = _("Are you sure that you wish to delete the selected servers?");
			}

			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO, this) == wxYES ) {
				RemoveAllServers(wxLIST_STATE_SELECTED);
			}
		}
	}
}


void CServerListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// Check if delete was pressed
	if ((event.GetKeyCode() == WXK_DELETE) || (event.GetKeyCode() == WXK_NUMPAD_DELETE)) {
		wxCommandEvent evt;
		evt.SetId( MP_REMOVE );
		OnRemoveServers( evt );
	} else {
		event.Skip();
	}
}


int CServerListCtrl::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CServer* server1 = reinterpret_cast<CServer*>(item1);
	CServer* server2 = reinterpret_cast<CServer*>(item2);

	int mode = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;

	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
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
					uint32 a = wxUINT32_SWAP_ALWAYS(server1->GetIP());
					uint32 b = wxUINT32_SWAP_ALWAYS(server2->GetIP());
					return mode * CmpAny(a, b);
				}
			}
		// Sort by port
		case COLUMN_SERVER_PORT: return mode * CmpAny( server1->GetPort(), server2->GetPort() );
		// Sort by description
		case COLUMN_SERVER_DESC: return mode * server1->GetDescription().CmpNoCase( server2->GetDescription() );
		// Sort by Ping
		// The -1 ensures that a value of zero (no ping known) is sorted last.
		case COLUMN_SERVER_PING: return mode * CmpAny( server1->GetPing() - 1, server2->GetPing() -1 );
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
				return mode * CmpAny( server2->IsStaticMember(), server1->IsStaticMember() );
			}
		// Sort by version
		case COLUMN_SERVER_VERSION: return mode * FuzzyStrCmp(server1->GetVersion(), server2->GetVersion());

		default:
			return 0;
	}
}
// File_checked_for_headers
