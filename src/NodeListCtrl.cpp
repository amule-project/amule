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
#pragma implementation "NodeListCtrl.h"
#endif

#include <wx/filefn.h>
#include <wx/textfile.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>

#include "NodeListCtrl.h"	// Interface declarations
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

#include "kademlia/kademlia/Kademlia.h" // Needed for CKademlia
#include "kademlia/routing/Contact.h" // Needed for CContact
#include "kademlia/routing/RoutingZone.h" // Needed for nodes routing zone

#define SYSCOLOR(x) (wxSystemSettings::GetColour(x))

using namespace Kademlia;

BEGIN_EVENT_TABLE(CNodeListCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK( -1,	CNodeListCtrl::OnItemRightClicked)
	EVT_LIST_ITEM_ACTIVATED( -1, 	CNodeListCtrl::OnItemActivated )

	EVT_MENU( MP_CONNECTTO,			CNodeListCtrl::OnBootstrapNode )
	
	EVT_MENU( MP_REMOVE,			CNodeListCtrl::OnRemoveNodes )
	EVT_MENU( MP_REMOVEALL,			CNodeListCtrl::OnRemoveNodes )
	
	EVT_CHAR( CNodeListCtrl::OnKeyPressed )
END_EVENT_TABLE()



CNodeListCtrl::CNodeListCtrl( wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
                                  long style, const wxValidator& validator, const wxString& name )
	: CMuleListCtrl( parent, winid, pos, size, style, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Node") );

	InsertColumn( COLUMN_NODE_ID, _("ID"),	wxLIST_FORMAT_LEFT, 150);
	InsertColumn( COLUMN_NODE_TYPE, _("Type"),			wxLIST_FORMAT_LEFT, 25);
	InsertColumn( COLUMN_NODE_DISTANCE, _("Distance"),			wxLIST_FORMAT_LEFT, 150);

	LoadSettings();
}


CNodeListCtrl::~CNodeListCtrl()
{
	printf("Destroying node list control\n");
}


void CNodeListCtrl::AddNode( const CContact* toadd )
{
	// RefreshNode will add the node.
	// This also means that we have simple duplicity checking. ;)
	RefreshNode( toadd );
	
	ShowNodeCount();
	
}


void CNodeListCtrl::RemoveNode( const CContact* node )
{
	
	long result = FindItem( -1, (long)node );
	if ( result != -1 ) {
		#warning TODO EC 
		#ifndef CLIENT_GUI
		CKademlia::getRoutingZone()->remove(((CContact*)GetItemData( result ))->getClientID());
		DeleteItem( result );
		#endif
	}
	ShowNodeCount();
	
}


void CNodeListCtrl::RemoveAllNodes( int state )
{
	
	int pos = GetNextItem( -1, wxLIST_NEXT_ALL, state);

	while ( pos != -1 ) {
		#warning TODO EC 
		#ifndef CLIENT_GUI
		CKademlia::getRoutingZone()->remove(((CContact*)GetItemData( pos ))->getClientID());
		DeleteItem( pos );
		pos = GetNextItem( pos-1, wxLIST_NEXT_ALL, state );
		#endif
	}

	ShowNodeCount();
	
}


void CNodeListCtrl::RefreshNode( const CContact* node )
{

	// Cant really refresh a NULL node
	if ( !node )
		return;

	// The current stats
	int itemState = 0;

	// The new or old item
	wxListItem item;
	
	long itemnr = FindItem( -1, (long)node );
	if ( itemnr != -1 ) {
		// Try to get the current item, so that we get to keep bold'ness and such
		item.SetId( itemnr );
		GetItem( item );
		
		int sortby = GetSortColumn();
		if ( !GetSortAsc() ) {
			sortby += SORT_OFFSET_DEC;
		}
		
		// Decides if we should reposition the item (through delete and insert)
		bool sorted = true;
			
		// Check if we are still "smaller" than the item before us
		if ( itemnr > 0 ) {
			sorted &= ( SortProc( (long)node, GetItemData( itemnr - 1 ), sortby ) >= 0 );
		}
		
		// Check if we are still "larger" than the item after us
		if ( itemnr < GetItemCount() -1 ) {
			sorted &= ( SortProc( (long)node, GetItemData( itemnr + 1 ), sortby ) <= 0 );
		}
		
		if ( !sorted ) {
			itemState = GetItemState( itemnr, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED );
			DeleteItem( itemnr );
			itemnr = -1;
		}
	}
	
	// New item or we deleted the old one
	if ( itemnr == -1 ) {
		// We are not sure that the node isn't in the list, so we can re-add
		itemnr = InsertItem( GetInsertPos( (long)node ), node->getClientIDString() );
		SetItemData( itemnr, (long)node );
	
		item.SetId( itemnr );
		item.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
		SetItem( item );
		
		// Resetting the state, which gets lost when we remove the old item
		SetItemState( itemnr, itemState, itemState );

		// Ensure that the item doesn't just warp out of view
		if ( itemState & wxLIST_STATE_FOCUSED ) {
			EnsureVisible( itemnr );
		}
	}
	

	SetItem( itemnr, COLUMN_NODE_ID, node->getClientIDString() );
	SetItem( itemnr, COLUMN_NODE_TYPE, wxString::Format(wxT("%i"),node->getType()) );
	SetItem( itemnr, COLUMN_NODE_DISTANCE, node->getDistanceString() );
	
}


void CNodeListCtrl::ShowNodeCount()
{
	wxStaticText* label = CastByName( wxT("nodesListLabel"), GetParent(), wxStaticText );

	if ( label ) {
		label->SetLabel( wxString::Format( _("Nodes (%i)"), GetItemCount() ) );
		label->GetParent()->Layout();
	}
}


void CNodeListCtrl::OnItemActivated( wxListEvent& event )
{
	// Unselect all items but the activated one
	long item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	while ( item > -1 ) {
		SetItemState( item, 0, wxLIST_STATE_SELECTED);
		
		item = GetNextItem( item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
		
	SetItemState( event.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

	wxCommandEvent nulEvt;
	OnBootstrapNode( nulEvt );
}


void CNodeListCtrl::OnItemRightClicked( wxListEvent& event )
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

	wxMenu* nodeMenu = new wxMenu(_("Node"));
	nodeMenu->Append( MP_CONNECTTO, _("Bootstrap node") );
	
	nodeMenu->AppendSeparator();
	
	nodeMenu->Append( MP_REMOVE, _("Remove Node(s)") );
	nodeMenu->Append( MP_REMOVEALL, _("Remove all nodes") );
	
	PopupMenu( nodeMenu, event.GetPoint() );
}

void CNodeListCtrl::OnBootstrapNode( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( item > -1 ) {
		#warning TODO EC 
		#ifndef CLIENT_GUI
		if( !Kademlia::CKademlia::isRunning() ) {
			Kademlia::CKademlia::start();
		}
	
		CContact* node = (CContact*)GetItemData( item );
		
		Kademlia::CKademlia::bootstrap(node->getIPAddress(), node->getUDPPort());
		#else
		wxMessageBox(_("You can't bootstrap a node from remote GUI yet."));
		#endif
		
	}
}
	
void CNodeListCtrl::OnRemoveNodes( wxCommandEvent& event )
{
	if ( event.GetId() == MP_REMOVEALL ) {
		if ( GetItemCount() ) {
			wxString question = _("Are you sure that you wish to delete all nodes?\nKademlia will be stopped and disconnected.");
	
			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ) {
				#warning TODO EC 
				#ifndef CLIENT_GUI
				Kademlia::CKademlia::stop();
				RemoveAllNodes( wxLIST_STATE_DONTCARE);
				#endif
			}
		}
	} else if ( event.GetId() == MP_REMOVE ) {
		if ( GetSelectedItemCount() ) {
			wxString question = _("Are you sure that you wish to delete the selected node(s)?");
	
			if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO) == wxYES ) {
				RemoveAllNodes( wxLIST_STATE_SELECTED);
			}
		}
	}
}


void CNodeListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	// Check if delete was pressed
	if ( event.GetKeyCode() == WXK_DELETE ) {
		wxCommandEvent evt;
		evt.SetId( MP_REMOVE );
		OnRemoveNodes( evt );
	} else {
		event.Skip();
	}
}


int CNodeListCtrl::SortProc( long item1, long item2, long sortData )
{
	CContact* node1 = (CContact*)item1;
	CContact* node2 = (CContact*)item2;

	int mode = 1;
	if ( sortData >= 1000 ) {
		mode = -1;
		sortData -= 1000;
	}
	
	switch ( sortData ) {
		// Sort by node-id
		case COLUMN_NODE_ID:
			return mode * node1->getClientIDString().CmpNoCase(node2->getClientIDString());
		// Sort by type
		case COLUMN_NODE_TYPE:
			return mode * CmpAny( node1->getType(), node2->getType());
		// Sort by distance
		case COLUMN_NODE_DISTANCE:
			return mode * node1->getDistanceString().CmpNoCase(node2->getDistanceString());
		default:
			return 0;
	}
}
