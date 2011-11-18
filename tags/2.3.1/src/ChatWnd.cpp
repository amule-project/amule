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

#include <common/MenuIDs.h>  // IDs for the chat-popup menu

#include <wx/app.h>

#include "ChatWnd.h"		// Interface declarations

#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "ClientList.h"		// Needed for CClientList
#include "ClientRef.h"		// Needed for CClientRef
#include "FriendListCtrl.h"	// Needed for CFriendListCtrl
#include "FriendList.h"		// Needed for CFriendList
#include "Friend.h"			// Needed for CFriend
#include "ChatSelector.h"	// Needed for CChatSelector
#include "muuli_wdr.h"		// Needed for messagePage
#include "OtherFunctions.h"

BEGIN_EVENT_TABLE(CChatWnd, wxPanel)
	EVT_RIGHT_DOWN(CChatWnd::OnNMRclickChatTab)

	EVT_MENU(MP_CLOSE_TAB,			CChatWnd::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS,		CChatWnd::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS,	CChatWnd::OnPopupCloseOthers)
	EVT_MENU(MP_ADDFRIEND,			CChatWnd::OnAddFriend )

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
	// We want to use our own popup menu
	chatselector->SetPopupHandler(this);
	m_menu = NULL;

	friendlistctrl = CastChild( ID_FRIENDLIST, CFriendListCtrl );
}

void CChatWnd::StartSession(CFriend* friend_client, bool setfocus)
{

	if ( !friend_client->GetName().IsEmpty() ) {
		if (setfocus) {
			theApp->amuledlg->SetActiveDialog(CamuleDlg::DT_CHAT_WND, this);
		}
		chatselector->StartSession(GUI_ID(friend_client->GetIP(), friend_client->GetPort()), friend_client->GetName(), true);
	}

	// Check to enable the window controls if needed
	CheckNewButtonsState();	
}


void CChatWnd::OnNMRclickChatTab(wxMouseEvent& evt)
{
	// Only handle events from the chat-notebook
	if (evt.GetEventObject() != (wxObject*)chatselector)
		return;
	
	if (chatselector->GetSelection() == -1) {
		return;
	}
	
	// Avoid opening another menu when it's already open
	if (m_menu == NULL) {  
		m_menu = new wxMenu(_("Chat"));
		
		m_menu->Append(MP_CLOSE_TAB, wxString(_("Close tab")));
		m_menu->Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
		m_menu->Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));
		
		m_menu->AppendSeparator();
		
		wxMenuItem * addFriend = m_menu->Append(MP_ADDFRIEND, _("Add to Friends"));

		// Disable this client if it is already a friend
		CClientRef client;
		if (chatselector->GetCurrentClient(client) && client.IsFriend()) {
			addFriend->Enable(false);
		}

		PopupMenu(m_menu, evt.GetPosition());
		
		delete m_menu;
		m_menu = NULL;
	}
}


void CChatWnd::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->DeletePage(chatselector->GetSelection());
}


void CChatWnd::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->DeleteAllPages();
}


void CChatWnd::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
	wxNotebookPage* current = chatselector->GetPage(chatselector->GetSelection());
	
	for (int i = chatselector->GetPageCount() - 1; i >= 0; i--) {
		if (current != chatselector->GetPage(i))
			chatselector->DeletePage( i );
	}
}


void CChatWnd::OnAddFriend(wxCommandEvent& WXUNUSED(evt))
{
	// Get the client that the session is open to
	CClientRef client;
	
	// Add the client as friend unless it's already a friend
	if (chatselector->GetCurrentClient(client) && !client.IsFriend()) {
		theApp->friendlist->AddFriend(client);
	}
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


void CChatWnd::UpdateFriend(CFriend* toupdate)
{
	if (toupdate->GetLinkedClient().IsLinked()) {
		chatselector->RefreshFriend(GUI_ID(toupdate->GetIP(), toupdate->GetPort()), toupdate->GetName());
	} else {
		// drop Chat session
		chatselector->EndSession(GUI_ID(toupdate->GetIP(), toupdate->GetPort()));
	}
	friendlistctrl->UpdateFriend(toupdate);
}


void CChatWnd::RemoveFriend(CFriend* todel)
{
	chatselector->EndSession(GUI_ID(todel->GetIP(), todel->GetPort()));
	friendlistctrl->RemoveFriend(todel);
}


void CChatWnd::ProcessMessage(uint64 sender, const wxString& message)
{
	if ( !theApp->amuledlg->IsDialogVisible(CamuleDlg::DT_CHAT_WND) ) {
		theApp->amuledlg->SetMessageBlink(true);
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


void CChatWnd::CheckNewButtonsState()
{
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


bool CChatWnd::IsIdValid(uint64 id)
{ 
	return chatselector->GetTabByClientID(id) >= 0;
}


void CChatWnd::ShowCaptchaResult(uint64 id, bool ok)
{
	chatselector->ShowCaptchaResult(id, ok);
}

void CChatWnd::EndSession(uint64 id)
{
	chatselector->EndSession(id);
}

// File_checked_for_headers
