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


#include "FriendListCtrl.h"	// Interface declarations

#include <common/MenuIDs.h>
#include <common/MacrosProgramSpecific.h>

#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "AddFriend.h"		// Needed for CAddFriend
#include "ChatWnd.h"		// Needed for CChatWnd
#include "Friend.h"		// Needed for CFriend
#include "muuli_wdr.h"
#include "SafeFile.h"
#include "FriendList.h"		// Needed for the friends list

BEGIN_EVENT_TABLE(CFriendListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CFriendListCtrl::OnRightClick)
	EVT_LIST_ITEM_ACTIVATED(ID_FRIENDLIST, CFriendListCtrl::OnItemActivated) 
	
	EVT_MENU(MP_MESSAGE, CFriendListCtrl::OnSendMessage)
	EVT_MENU(MP_REMOVEFRIEND, CFriendListCtrl::OnRemoveFriend)
	EVT_MENU(MP_ADDFRIEND, CFriendListCtrl::OnAddFriend)
	EVT_MENU(MP_DETAIL, CFriendListCtrl::OnShowDetails)
	EVT_MENU(MP_SHOWLIST, CFriendListCtrl::OnViewFiles)
	EVT_MENU(MP_FRIENDSLOT, CFriendListCtrl::OnSetFriendslot)

	EVT_CHAR(CFriendListCtrl::OnKeyPressed)
END_EVENT_TABLE()


CFriendListCtrl::CFriendListCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags)
: CMuleListCtrl(parent, id, pos, siz, flags)
{
  InsertColumn(0, _("Username"), wxLIST_FORMAT_LEFT, siz.GetWidth() - 4);
}

CFriendListCtrl::~CFriendListCtrl()
{
}


void CFriendListCtrl::RemoveFriend(CFriend* toremove)
{
	if (!toremove) {
		return;
	}
	
	sint32 itemnr = FindItem(-1, reinterpret_cast<wxUIntPtr>(toremove));
	
	if ( itemnr == -1 )
		return;
	
	DeleteItem(itemnr);
}


void CFriendListCtrl::UpdateFriend(CFriend* toupdate)
{
	if (!toupdate) {
		return;
	}

	sint32 itemnr = FindItem(-1, reinterpret_cast<wxUIntPtr>(toupdate));
	if (itemnr == -1) {
		itemnr = InsertItem(GetItemCount(), wxEmptyString);
		SetItemPtrData(itemnr, reinterpret_cast<wxUIntPtr>(toupdate));
	}

	SetItem(itemnr, 0, toupdate->GetName());
	SetItemTextColour(itemnr, toupdate->GetLinkedClient().IsLinked() ? *wxBLUE : *wxBLACK);
}


void CFriendListCtrl::OnItemActivated(wxListEvent& WXUNUSED(event))
{
	int cursel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	
	CFriend* cur_friend = (CFriend*)GetItemData(cursel);
	
	/* ignore this one, it is not activated anymore :) */
	if (cur_friend == NULL) {
		return;
	}

	theApp->amuledlg->m_chatwnd->StartSession(cur_friend);
	
}


void CFriendListCtrl::OnRightClick(wxMouseEvent& event)
{
	int cursel = CheckSelection(event);
	
	CFriend* cur_friend = NULL;
	
	wxMenu* menu = new wxMenu(_("Friends"));
	
	if ( cursel != -1 ) {
		cur_friend = (CFriend*)GetItemData(cursel);
		menu->Append(MP_DETAIL, _("Show &Details"));
		menu->Enable(MP_DETAIL, cur_friend->GetLinkedClient().IsLinked());
	}
	
	menu->Append(MP_ADDFRIEND, _("Add a friend"));
	
	if (cursel != (-1)) {
		menu->Append(MP_REMOVEFRIEND, _("Remove Friend"));
		menu->Append(MP_MESSAGE, _("Send &Message"));
		menu->Append(MP_SHOWLIST, _("View Files"));
		menu->AppendCheckItem(MP_FRIENDSLOT, _("Establish Friend Slot"));
		if (cur_friend->GetLinkedClient().IsLinked()) {
			menu->Enable(MP_FRIENDSLOT, true);
			menu->Check(MP_FRIENDSLOT, cur_friend->HasFriendSlot());
		} else {
			menu->Enable(MP_FRIENDSLOT, false);
		}
	}
	
	PopupMenu(menu, event.GetPosition());
	delete menu;
}

void CFriendListCtrl::OnSendMessage(wxCommandEvent& WXUNUSED(event)) {
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CFriend* cur_friend = (CFriend*)GetItemData(index);
		theApp->amuledlg->m_chatwnd->StartSession(cur_friend);			
		//#warning CORE/GUI!			
		#ifndef CLIENT_GUI
		theApp->friendlist->StartChatSession(cur_friend);
		#endif		

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}	
}


void CFriendListCtrl::OnRemoveFriend(wxCommandEvent& WXUNUSED(event))
{
	wxString question;
	if (GetSelectedItemCount() == 1) {
		question = _("Are you sure that you wish to delete the selected friend?");
	} else {
		question = _("Are you sure that you wish to delete the selected friends?");
	}

	if ( wxMessageBox( question, _("Cancel"), wxICON_QUESTION | wxYES_NO, this) == wxYES ) {
		long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
		while( index != -1 ) {
			CFriend* cur_friend = (CFriend*)GetItemData(index);
			theApp->friendlist->RemoveFriend(cur_friend);
			// -1 because we changed the list and removed that item.
			index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		}
	}
}


void CFriendListCtrl::OnAddFriend(wxCommandEvent& WXUNUSED(event))
{
	CAddFriend(this).ShowModal();			
}


void CFriendListCtrl::OnShowDetails(wxCommandEvent& WXUNUSED(event))
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CFriend* cur_friend = (CFriend*)GetItemData(index);
		if (cur_friend->GetLinkedClient().IsLinked()) {
			CClientDetailDialog(this, cur_friend->GetLinkedClient()).ShowModal();
		}		
		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}	
	
}


void CFriendListCtrl::OnViewFiles(wxCommandEvent& WXUNUSED(event))
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CFriend* cur_friend = (CFriend*)GetItemData(index);
		theApp->friendlist->RequestSharedFileList(cur_friend);
		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}	
	
}


void CFriendListCtrl::OnSetFriendslot(wxCommandEvent& event)
{
	// Get item
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	CFriend* cur_friend = (CFriend*)GetItemData(index);	
	theApp->friendlist->SetFriendSlot(cur_friend, event.IsChecked());
	index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if (index != -1) {
		wxMessageBox(_("You are not allowed to set more than one friendslot.\n Only one slot was assigned."), _("Multiple selection"), wxOK | wxICON_ERROR, this);
	}
}


void CFriendListCtrl::OnKeyPressed(wxKeyEvent& event)
{
	// Check if delete was pressed
	if ((event.GetKeyCode() == WXK_DELETE) || (event.GetKeyCode() == WXK_NUMPAD_DELETE)) {
		if (GetItemCount()) {
			wxCommandEvent evt;
			evt.SetId( MP_REMOVEFRIEND );
			OnRemoveFriend( evt );
		}
	} else {
		event.Skip();
	}
}
// File_checked_for_headers
