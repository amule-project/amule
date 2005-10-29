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

#warning =====================DEPRECATED======================

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
	InsertColumn( COLUMN_NODE_ID, _("Void"),	wxLIST_FORMAT_LEFT, 400);
	InsertItem( 1, wxT("Deprecated List") );
}


CNodeListCtrl::~CNodeListCtrl()
{
	
}


void CNodeListCtrl::AddNode( const CContact* toadd )
{
}


void CNodeListCtrl::RemoveNode( const CContact* node )
{
}


void CNodeListCtrl::RemoveAllNodes( int state )
{
}


void CNodeListCtrl::RefreshNode( const CContact* node )
{

}


void CNodeListCtrl::ShowNodeCount()
{
}


void CNodeListCtrl::OnItemActivated( wxListEvent& event )
{
}


void CNodeListCtrl::OnItemRightClicked( wxListEvent& event )
{
}

void CNodeListCtrl::OnBootstrapNode( wxCommandEvent& WXUNUSED(event) )
{
}
	
void CNodeListCtrl::OnRemoveNodes( wxCommandEvent& event )
{
}


void CNodeListCtrl::OnKeyPressed( wxKeyEvent& event )
{
	event.Skip();
}


int CNodeListCtrl::SortProc( long item1, long item2, long sortData )
{
	return 0;
}
