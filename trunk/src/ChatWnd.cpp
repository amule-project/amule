// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW

#include "ChatWnd.h"		// Interface declarations.
#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "FriendListCtrl.h"	// Needed for CFriendListCtrl
#include "updownclient.h"	// Needed for CUpDownClient
#include "ChatSelector.h"	// Needed for CChatSelector
#include "muuli_wdr.h"		// Needed for messagePage
#include "color.h"			// Needed for GetColour
#include "opcodes.h"


BEGIN_EVENT_TABLE(CChatWnd, wxPanel)
	EVT_BUTTON(IDC_CSEND, CChatWnd::OnBnClickedCsend)
	EVT_BUTTON(IDC_CCLOSE, CChatWnd::OnBnClickedCclose)
	EVT_TEXT_ENTER(IDC_CMESSAGE, CChatWnd::OnBnClickedCsend)
	
	EVT_RIGHT_DOWN(CChatWnd::OnRMButton)

	EVT_MENU(MP_CLOSE_TAB, CChatWnd::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS, CChatWnd::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS, CChatWnd::OnPopupCloseOthers)
END_EVENT_TABLE()


#define GetDlgItem(a, b) wxStaticCast(FindWindow((a)), b)


CChatWnd::CChatWnd(wxWindow* pParent)
: wxPanel(pParent, CChatWnd::IDD)
{
	wxSizer* content = messagePage(this, true);
	content->Show(this, true);

	chatselector = (CChatSelector*)FindWindow(IDC_CHATSELECTOR);
	friendlist = (CFriendListCtrl*)FindWindow(ID_FRIENDLIST);

	// Allow notebook to dispatch right mouse clicks to us
	chatselector->SetMouseListener(GetEventHandler());
}


void CChatWnd::StartSession(CUpDownClient* client)
{
	if ( !client->GetUserName().IsEmpty() ) {
		theApp.amuledlg->SetActiveDialog(this);
		chatselector->StartSession(client, true);
	}
}


void CChatWnd::OnBnClickedCsend(wxCommandEvent& WXUNUSED(evt))
{
	wxString message = GetDlgItem(IDC_CMESSAGE, wxTextCtrl)->GetValue();
	if (chatselector->SendMessage( message )) {
		GetDlgItem(IDC_CMESSAGE, wxTextCtrl)->Clear();
	}
	GetDlgItem(IDC_CMESSAGE, wxTextCtrl)->SetFocus();
}


void CChatWnd::OnRMButton(wxMouseEvent& evt)
{
	if( !chatselector->GetPageCount() ) {
		return;
	}
	
	// Translate the global position to a position relative to the notebook
	wxPoint newpt=((wxWindow*)evt.GetEventObject())->ClientToScreen( evt.GetPosition() );
	newpt = ScreenToClient(newpt);

	// Only show the popup-menu if we are inside the notebook widget
	if ( chatselector->GetRect().Inside( newpt ) ) {
		wxMenu* menu = new wxMenu(wxString(_("Close")));
		menu->Append(MP_CLOSE_TAB, wxString(_("Close tab")));
		menu->Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
		menu->Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));
	
		PopupMenu(menu, newpt);
	
		delete menu;
	} else {
		evt.Skip();
	}
}


void CChatWnd::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->DeletePage( chatselector->GetSelection() );
}


void CChatWnd::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->DeleteAllPages();
}


void CChatWnd::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
	wxNotebookPage* current = chatselector->GetPage( chatselector->GetSelection() );
	
	int i = 0;
	while ( i < chatselector->GetPageCount() ) {
		if ( current == chatselector->GetPage( i ) ) {
			i++;
			continue;
		}
		
		chatselector->DeletePage( i );
	}
}


void CChatWnd::OnBnClickedCclose(wxCommandEvent& WXUNUSED(evt))
{
	chatselector->EndSession();
}


CFriend* CChatWnd::FindFriend(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort)
{
	return friendlist->FindFriend(userhash, dwIP, nPort);
}


void CChatWnd::AddFriend(CUpDownClient* toadd)
{
	friendlist->AddFriend(toadd);
}


void CChatWnd::AddFriend(const CMD4Hash& userhash, uint32 lastSeen, uint32 lastUsedIP, uint32 lastUsedPort, uint32 lastChatted, wxString name, uint32 hasHash)
{
	friendlist->AddFriend( userhash, lastSeen, lastUsedIP, lastUsedPort, lastChatted, name, hasHash);
}


void CChatWnd::RefreshFriend(CFriend* toupdate)
{
	friendlist->RefreshFriend(toupdate);
	chatselector->RefreshFriend(toupdate);
}

void CChatWnd::ProcessMessage(CUpDownClient* sender, char* message)
{
	chatselector->ProcessMessage(sender, message);
}

void CChatWnd::ConnectionResult(CUpDownClient* sender, bool success)
{
	chatselector->ConnectionResult(sender, success);
}
