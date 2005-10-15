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

#include <wx/defs.h>		// Needed before any other wx/*.h

#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/accel.h>
#include "ChatWnd.h"		// Interface declarations.
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "FriendListCtrl.h"	// Needed for CFriendListCtrl
#include "updownclient.h"	// Needed for CUpDownClient
#include "ChatSelector.h"	// Needed for CChatSelector
#include "muuli_wdr.h"		// Needed for messagePage
#include "Color.h"		// Needed for GetColour
#include "MuleNotebook.h"	// Needed for MULENOTEBOOK events
#include "OPCodes.h"
#include "OtherFunctions.h"

BEGIN_EVENT_TABLE(CChatWnd, wxPanel)
	EVT_TEXT_ENTER(IDC_CMESSAGE, CChatWnd::OnBnClickedCsend)
	EVT_BUTTON(IDC_CSEND, CChatWnd::OnBnClickedCsend)
	EVT_BUTTON(IDC_CCLOSE, CChatWnd::OnBnClickedCclose)
	EVT_MULENOTEBOOK_ALL_PAGES_CLOSED(IDC_CHATSELECTOR, CChatWnd::OnAllPagesClosed)
END_EVENT_TABLE()


CChatWnd::CChatWnd(wxWindow* pParent)
: wxPanel(pParent, -1)
{
	wxSizer* content = messagePage(this, true);
	content->Show(this, true);

	chatselector = CastChild( IDC_CHATSELECTOR, CChatSelector );
	friendlistctrl = CastChild( ID_FRIENDLIST, CFriendListCtrl );
}

void CChatWnd::StartSession(CDlgFriend* friend_client, bool setfocus)
{

	if ( !friend_client->m_name.IsEmpty() ) {
		if (setfocus) {
			theApp.amuledlg->SetActiveDialog(CamuleDlg::ChatWnd, this);
		}
		chatselector->StartSession(GUI_ID(friend_client->m_ip, friend_client->m_port), friend_client->m_name, true);
	}

	// Check to enable the window controls if needed
	CheckNewButtonsState();	
}


void CChatWnd::OnBnClickedCsend(wxCommandEvent& WXUNUSED(evt))
{
	wxString message = CastChild(IDC_CMESSAGE, wxTextCtrl)->GetValue();
	
	SendMessage(message);
}


void CChatWnd::OnBnClickedCclose(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->EndSession();
}


void CChatWnd::OnAllPagesClosed(wxNotebookEvent& WXUNUSED(evt))
{
	CastChild(IDC_CMESSAGE, wxTextCtrl)->Clear();
	// Check to disable the window controls
	CheckNewButtonsState();
}


CDlgFriend* CChatWnd::FindFriend(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort)
{
	return friendlistctrl->FindFriend(userhash, dwIP, nPort);
}


void CChatWnd::AddFriend(CUpDownClient* toadd)
{
	friendlistctrl->AddFriend(toadd);
}


void CChatWnd::AddFriend(const CMD4Hash& userhash, const wxString& name, uint32 lastUsedIP, uint32 lastUsedPort)
{
	friendlistctrl->AddFriend( userhash, name, lastUsedIP, lastUsedPort);
}


void CChatWnd::RefreshFriend(const CMD4Hash& userhash, const wxString& name, uint32 lastUsedIP, uint32 lastUsedPort)
{
	CDlgFriend* toupdate = friendlistctrl->FindFriend(userhash, lastUsedIP, lastUsedPort);
	if (toupdate) {
		if (!name.IsEmpty()) {
			toupdate->m_name = name;	
		} 
		// If name is empty, this is a disconnection/deletion event
		toupdate->islinked = !name.IsEmpty();
		friendlistctrl->RefreshFriend(toupdate);
		chatselector->RefreshFriend(GUI_ID(toupdate->m_ip, toupdate->m_port),name);
	}
}

void CChatWnd::ProcessMessage(uint64 sender, const wxString& message)
{
	if ( !theApp.amuledlg->IsDialogVisible( CamuleDlg::ChatWnd ) ) {
		theApp.amuledlg->SetMessageBlink(true);
	}
	if (chatselector->ProcessMessage(sender, message)) {
		// Check to enable the window controls if needed
		CheckNewButtonsState();
	}
}

void CChatWnd::ConnectionResult(bool success, const wxString& message, uint64 id)
{
	chatselector->ConnectionResult(success, message, id);
}

void CChatWnd::SendMessage(const wxString& message, const wxString& client_name, uint64 to_id)
{
	
	if (chatselector->SendMessage( message, client_name, to_id )) {
		CastChild(IDC_CMESSAGE, wxTextCtrl)->Clear();
	}

	// Check to enable the window controls if needed
	CheckNewButtonsState();
	CastChild(IDC_CMESSAGE, wxTextCtrl)->SetFocus();
}


void CChatWnd::CheckNewButtonsState() {
	switch (chatselector->GetPageCount()) {
			case 0:
				GetParent()->FindWindow(IDC_CSEND)->Enable(false);
				GetParent()->FindWindow(IDC_CCLOSE)->Enable(false);
				GetParent()->FindWindow(IDC_CMESSAGE)->Enable(false);
				break;
			case 1:
				GetParent()->FindWindow(IDC_CSEND)->Enable(true);
				GetParent()->FindWindow(IDC_CCLOSE)->Enable(true);
				GetParent()->FindWindow(IDC_CMESSAGE)->Enable(true);
				break;
			default:
				// Nothing to be done here. Keep current state, which should be enabled.
				wxASSERT(GetParent()->FindWindow(IDC_CSEND)->IsEnabled());
				wxASSERT(GetParent()->FindWindow(IDC_CCLOSE)->IsEnabled());
				wxASSERT(GetParent()->FindWindow(IDC_CMESSAGE)->IsEnabled());			
				break;
	}
}
