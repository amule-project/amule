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


#include "muuli_wdr.h"		// Needed for ID_ADDTOLIST
#include "ServerWnd.h"		// Interface declarations.
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "Preferences.h"	// Needed for CPreferences
#include "ServerConnect.h"
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp
#include "Logger.h"

#include "ClientList.h"

BEGIN_EVENT_TABLE(CServerWnd,wxPanel)
	EVT_BUTTON(ID_ADDTOLIST,CServerWnd::OnBnClickedAddserver)
	EVT_BUTTON(IDC_ED2KDISCONNECT,CServerWnd::OnBnClickedED2KDisconnect)
	EVT_BUTTON(ID_UPDATELIST,CServerWnd::OnBnClickedUpdateservermetfromurl)
	EVT_TEXT_ENTER(IDC_SERVERLISTURL,CServerWnd::OnBnClickedUpdateservermetfromurl)
	EVT_BUTTON(ID_BTN_RESET, CServerWnd::OnBnClickedResetLog)
	EVT_BUTTON(ID_BTN_RESET_SERVER, CServerWnd::OnBnClickedResetServerLog)
	EVT_SPLITTER_SASH_POS_CHANGED(ID_SRV_SPLITTER,CServerWnd::OnSashPositionChanged)
END_EVENT_TABLE()

	
CServerWnd::CServerWnd(wxWindow* pParent /*=NULL*/, int splitter_pos)
: wxPanel(pParent, -1)
{
	wxSizer* sizer = serverListDlg(this,TRUE);

	// init serverlist
	// no use now. too early.

	serverlistctrl = CastChild( ID_SERVERLIST, CServerListCtrl );

	CastChild( ID_SRV_SPLITTER, wxSplitterWindow )->SetSashPosition(splitter_pos, true);
	CastChild( ID_SRV_SPLITTER, wxSplitterWindow )->SetSashGravity(0.5f);
	CastChild( IDC_NODESLISTURL, wxTextCtrl )->SetValue(thePrefs::GetKadNodesUrl());
	CastChild( IDC_SERVERLISTURL, wxTextCtrl )->SetValue(thePrefs::GetEd2kServersUrl());

	// Insert two columns, currently without a header
	wxListCtrl* ED2KInfoList = CastChild( ID_ED2KINFO, wxListCtrl );
	wxASSERT(ED2KInfoList);
	ED2KInfoList->InsertColumn(0, wxEmptyString);
	ED2KInfoList->InsertColumn(1, wxEmptyString);

	wxListCtrl* KadInfoList = CastChild( ID_KADINFO, wxListCtrl );
	wxASSERT(KadInfoList);
	KadInfoList->InsertColumn(0, wxEmptyString);
	KadInfoList->InsertColumn(1, wxEmptyString);
	
	sizer->Show(this,TRUE);
}


CServerWnd::~CServerWnd()
{
	thePrefs::SetEd2kServersUrl(CastChild( IDC_SERVERLISTURL, wxTextCtrl )->GetValue());
	thePrefs::SetKadNodesUrl(CastChild( IDC_NODESLISTURL, wxTextCtrl )->GetValue());	
}


void CServerWnd::UpdateServerMetFromURL(const wxString& strURL)
{
	thePrefs::SetEd2kServersUrl(strURL);
	theApp->serverlist->UpdateServerMetFromURL(strURL);
}


void CServerWnd::OnBnClickedAddserver(wxCommandEvent& WXUNUSED(evt))
{
	wxString servername = CastChild( IDC_SERVERNAME, wxTextCtrl )->GetValue();
	wxString serveraddr = CastChild( IDC_IPADDRESS, wxTextCtrl )->GetValue();
	long port = StrToULong( CastChild( IDC_SPORT, wxTextCtrl )->GetValue() );

	if ( serveraddr.IsEmpty() ) {
		AddLogLineC(_("Server not added: No IP or hostname specified."));
		return;
	}
	
	if ( port <= 0 || port > 65535 ) {
		AddLogLineC(_("Server not added: Invalid server-port specified."));
		return;
	}
  
	CServer* toadd = new CServer( port, serveraddr );
	toadd->SetListName( servername.IsEmpty() ? serveraddr : servername );
	
	if ( theApp->AddServer( toadd, true ) ) {
		CastChild( IDC_SERVERNAME, wxTextCtrl )->Clear();
		CastChild( IDC_IPADDRESS, wxTextCtrl )->Clear();
		CastChild( IDC_SPORT, wxTextCtrl )->Clear();
	} else {
		CServer* update = theApp->serverlist->GetServerByAddress(toadd->GetAddress(), toadd->GetPort());
		// See note on CServerList::AddServer
		if (update == NULL && toadd->GetIP() != 0) {
			update = theApp->serverlist->GetServerByIPTCP(toadd->GetIP(), toadd->GetPort());
		}		
		
		if ( update ) {
			update->SetListName(toadd->GetListName());
			serverlistctrl->RefreshServer(update);
		}
		delete toadd;
	}
	
	theApp->serverlist->SaveServerMet();
}


void CServerWnd::OnBnClickedUpdateservermetfromurl(wxCommandEvent& WXUNUSED(evt))
{
	wxString strURL = CastChild( IDC_SERVERLISTURL, wxTextCtrl )->GetValue();
	UpdateServerMetFromURL(strURL);
}


void CServerWnd::OnBnClickedResetLog(wxCommandEvent& WXUNUSED(evt))
{
	theApp->GetLog(true); // Reset it.
}


void CServerWnd::OnBnClickedResetServerLog(wxCommandEvent& WXUNUSED(evt))
{
	theApp->GetServerLog(true); // Reset it
}


void CServerWnd::UpdateED2KInfo()
{
	wxListCtrl* ED2KInfoList = CastChild( ID_ED2KINFO, wxListCtrl );
	
	ED2KInfoList->DeleteAllItems();
	ED2KInfoList->InsertItem(0, _("eD2k Status:"));

	if (theApp->IsConnectedED2K()) {
		ED2KInfoList->SetItem(0, 1, _("Connected"));

		// Connection data		
		
		ED2KInfoList->InsertItem(1, _("IP:Port"));
		ED2KInfoList->SetItem(1, 1, theApp->serverconnect->IsLowID() ? 
			 wxString(_("LowID")) : Uint32_16toStringIP_Port( theApp->GetED2KID(), thePrefs::GetPort()));

		ED2KInfoList->InsertItem(2, _("ID"));
		// No need to test the server connect, it's already true
		ED2KInfoList->SetItem(2, 1, CFormat(wxT("%u")) % theApp->GetED2KID());
		
		ED2KInfoList->InsertItem(3, wxEmptyString);		

		if (theApp->serverconnect->IsLowID()) {
			ED2KInfoList->SetItem(1, 1, _("Server")); // LowID, unknown ip
			ED2KInfoList->SetItem(3, 1, _("LowID"));
		} else {
			ED2KInfoList->SetItem(1, 1, Uint32_16toStringIP_Port(theApp->GetED2KID(), thePrefs::GetPort()));
			ED2KInfoList->SetItem(3, 1, _("HighID"));
		}
		
	} else {
		// No data
		ED2KInfoList->SetItem(0, 1, _("Not Connected"));
	}

	// Fit the width of the columns
	ED2KInfoList->SetColumnWidth(0, -1);
	ED2KInfoList->SetColumnWidth(1, -1);
}

void CServerWnd::UpdateKadInfo()
{
	wxListCtrl* KadInfoList = CastChild( ID_KADINFO, wxListCtrl );

	int next_row = 0;

	KadInfoList->DeleteAllItems();

	KadInfoList->InsertItem(next_row, _("Kademlia Status:"));

	if (theApp->IsKadRunning()) {
		KadInfoList->SetItem(next_row++, 1, (theApp->IsKadRunningInLanMode() ? _("Running in LAN mode") : _("Running")));

		// Connection data
		KadInfoList->InsertItem(next_row, _("Status:"));
		KadInfoList->SetItem(next_row++, 1, theApp->IsConnectedKad() ? _("Connected"): _("Disconnected"));
		if (theApp->IsConnectedKad()) {
			KadInfoList->InsertItem(next_row, _("Connection State:"));
			KadInfoList->SetItem(next_row++, 1, theApp->IsFirewalledKad() ?
				wxString(CFormat(_("Firewalled - open TCP port %d in your router or firewall")) % thePrefs::GetPort())
				: wxString(_("OK")));
			KadInfoList->InsertItem(next_row, _("UDP Connection State:"));
			bool UDPFirewalled = theApp->IsFirewalledKadUDP();
			KadInfoList->SetItem(next_row++, 1, UDPFirewalled ?
				wxString(CFormat(_("Firewalled - open UDP port %d in your router or firewall")) % thePrefs::GetUDPPort())
				: wxString(_("OK")));

			if (theApp->IsFirewalledKad() || UDPFirewalled) {
				KadInfoList->InsertItem(next_row, _("Firewalled state: "));
				wxString BuddyState;
				switch ( theApp->GetBuddyStatus() )
				{
					case Disconnected:
						if (!theApp->IsFirewalledKad()) {
							BuddyState = _("No buddy required - TCP port open");
						} else if (!UDPFirewalled) {
							BuddyState = _("No buddy required - UDP port open");
						} else {
							BuddyState = _("No buddy");
						}
						break;
					case Connecting:
						BuddyState = _("Connecting to buddy");
						break;
					case Connected:
						BuddyState = CFormat(_("Connected to buddy at %s")) % Uint32_16toStringIP_Port(theApp->GetBuddyIP(), theApp->GetBuddyPort());
						break;
				}
				KadInfoList->SetItem(next_row++, 1, BuddyState);
			}

			KadInfoList->InsertItem(next_row, _("IP address:"));
			KadInfoList->SetItem(next_row++, 1, Uint32toStringIP(theApp->GetKadIPAdress()));

			// Index info
			KadInfoList->InsertItem(next_row, _("Indexed sources:"));
			KadInfoList->SetItem(next_row++, 1, CFormat(wxT("%d")) % theApp->GetKadIndexedSources());
			KadInfoList->InsertItem(next_row, _("Indexed keywords:"));
			KadInfoList->SetItem(next_row++, 1, CFormat(wxT("%d")) % theApp->GetKadIndexedKeywords());
			KadInfoList->InsertItem(next_row, _("Indexed notes:"));
			KadInfoList->SetItem(next_row++, 1, CFormat(wxT("%d")) % theApp->GetKadIndexedNotes());
			KadInfoList->InsertItem(next_row, _("Indexed load:"));
			KadInfoList->SetItem(next_row++, 1, CFormat(wxT("%d")) % theApp->GetKadIndexedLoad());

			KadInfoList->InsertItem(next_row, _("Average Users:"));
			KadInfoList->SetItem(next_row, 1, CastItoIShort(theApp->GetKadUsers()));
			++next_row;
			KadInfoList->InsertItem(next_row, _("Average Files:"));
			KadInfoList->SetItem(next_row, 1, CastItoIShort(theApp->GetKadFiles()));
		}
	} else {
		// No data
		KadInfoList->SetItem(next_row, 1, _("Not running"));
	}

	// Fit the width of the columns
	KadInfoList->SetColumnWidth(0, -1);
	KadInfoList->SetColumnWidth(1, -1);
}

void CServerWnd::OnSashPositionChanged(wxSplitterEvent& WXUNUSED(evt))
{
	if (theApp->amuledlg) {
		theApp->amuledlg->m_srv_split_pos = CastChild( wxT("SrvSplitterWnd"), wxSplitterWindow )->GetSashPosition();
	}
}

void CServerWnd::OnBnClickedED2KDisconnect(wxCommandEvent& WXUNUSED(evt))
{
	if (theApp->serverconnect->IsConnecting()) {
		theApp->serverconnect->StopConnectionTry();
	} else {
		theApp->serverconnect->Disconnect();
	}	
}
// File_checked_for_headers
