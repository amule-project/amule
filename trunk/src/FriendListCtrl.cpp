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
// FriendListCtrl.cpp : implementation file
//

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/menu.h>		// Needed for wxMenu

#include "amuleDlg.h"		// Needed for CamuleDlg
#include "FriendListCtrl.h"	// Interface declarations
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "AddFriend.h"		// Needed for CAddFriend
#include "FriendList.h"		// Needed for CFriendList
#include "ClientList.h"		// Needed for CClientList
#include "ChatWnd.h"		// Needed for CChatWnd
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for MP_DETAIL
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "updownclient.h"	// Needed for CUpDownClient
#include "Friend.h"		// Needed for CFriend

// CFriendListCtrl

BEGIN_EVENT_TABLE(CFriendListCtrl,CMuleListCtrl)
	EVT_RIGHT_DOWN(CFriendListCtrl::OnNMRclick)
	EVT_LIST_ITEM_SELECTED(ID_FRIENDLIST, CFriendListCtrl::OnItemSelected) 
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CFriendListCtrl, CListCtrl)

CFriendListCtrl::CFriendListCtrl()
{
}

CFriendListCtrl::~CFriendListCtrl()
{
}

CFriendListCtrl::CFriendListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags)
{
	Init();
}

// CFriendListCtrl message handlers

void CFriendListCtrl::Init()
{
  wxRect rc=GetClientRect();
  InsertColumn(0,_("Username"),wxLIST_FORMAT_LEFT,rc.width-4);
}

void CFriendListCtrl::AddFriend(CFriend* toadd)
{
	uint32 itemnr = GetItemCount();
	itemnr=InsertItem(itemnr,toadd->m_strName.GetData());
	SetItemData(itemnr,(long)toadd);
	RefreshFriend(toadd);
}

void CFriendListCtrl::RemoveFriend(CFriend* toremove)
{
	sint32 result = FindItem(-1,(long)toremove);
	if (result != (-1) ) {
		DeleteItem(result);
	}
}

void CFriendListCtrl::RefreshFriend(CFriend* toupdate)
{
	sint32 itemnr = FindItem(-1,(long)toupdate);
	CString temp;
	temp.Format( "%s", toupdate->m_strName.GetData() );
	SetItem(itemnr,0,temp);
	if (itemnr == (-1)) {
		return;
	}
	uint8 image;
	if (!toupdate->m_LinkedClient) {
		image = 0;
	} else if (toupdate->m_LinkedClient->socket && toupdate->m_LinkedClient->socket->IsConnected()) {
		image = 2;
	} else {
		image = 1;
	}
}

void CFriendListCtrl::OnItemSelected(wxListEvent& evt)
{
	// Force the list to refresh. For some reason, Refresh() wont work on its own...
	Freeze();
	Refresh(true);
	Thaw();
}

void CFriendListCtrl::OnNMRclick(wxMouseEvent& evt)
{
	CFriend* cur_friend=NULL;
	wxMenu* menu=new wxMenu(_("Friends"));
	int cursel=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if(cursel!=(-1)) {
		cur_friend = (CFriend*)GetItemData(cursel);
		menu->Append(MP_DETAIL,_("Show &Details"));
		menu->Enable(MP_DETAIL,((cur_friend->m_LinkedClient)?TRUE:FALSE));
	}
	menu->Append(MP_ADDFRIEND,_("Add a friend"));
	if (cursel != (-1)) {
		menu->Append(MP_REMOVEFRIEND,_("Remove Friend"));
		menu->Append(MP_MESSAGE,_("Send &Message"));
		menu->Append(MP_SHOWLIST,_("View Files"));
		menu->AppendCheckItem(MP_FRIENDSLOT,_("Establish Friend Slot"));
		if (cur_friend->m_LinkedClient) {
			menu->Enable(MP_FRIENDSLOT,TRUE);
			menu->Check(MP_FRIENDSLOT,((cur_friend->m_LinkedClient->GetFriendSlot())?TRUE : FALSE));
		} else {
			menu->Enable(MP_FRIENDSLOT,FALSE);
		}
	}
	PopupMenu(menu,evt.GetPosition());
}

bool CFriendListCtrl::ProcessEvent(wxEvent& evt)
{
	CFriend* cur_friend;
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}
	wxCommandEvent& event=(wxCommandEvent&)evt;
	int cursel=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (cursel != (-1)) {
		cur_friend = (CFriend*)GetItemData(cursel);
	}
	switch (event.GetId()) {
		case MP_MESSAGE: {
			if (cur_friend->m_LinkedClient) {
				theApp.amuledlg->chatwnd->StartSession(cur_friend->m_LinkedClient);
			} else {
				CUpDownClient* chatclient = new CUpDownClient(cur_friend->m_nLastUsedPort,cur_friend->m_dwLastUsedIP,0,0,0);
				chatclient->SetUserName((char*)cur_friend->m_strName.GetData());
				theApp.clientlist->AddClient(chatclient);
				theApp.amuledlg->chatwnd->StartSession(chatclient);
			}
			return true;			
			break;
		}
		case MP_REMOVEFRIEND: {
			theApp.friendlist->RemoveFriend(cur_friend);
			return true;			
			break;
		}
		case MP_ADDFRIEND: {
			CAddFriend* dialog2=new CAddFriend(this); 
			dialog2->ShowModal();
			delete dialog2;
			return true;						
		}
		break;
		case MP_DETAIL:
		if (cur_friend->m_LinkedClient) {
			CClientDetailDialog* dialog =new CClientDetailDialog(this,cur_friend->m_LinkedClient);
			dialog->ShowModal();
			delete dialog;
			return true;
		}
		break;
		case MP_SHOWLIST: {
			if (cur_friend->m_LinkedClient) {
				cur_friend->m_LinkedClient->RequestSharedFileList();
			} else {
				CUpDownClient* newclient = new CUpDownClient(cur_friend->m_nLastUsedPort,cur_friend->m_dwLastUsedIP,0,0,0);
				newclient->SetUserName((char*)cur_friend->m_strName.GetData());
				theApp.clientlist->AddClient(newclient);
				newclient->RequestSharedFileList();
			}
			return true;			
			break;
		}
		case MP_FRIENDSLOT: {
			if (cur_friend && cur_friend->m_LinkedClient) {
				bool IsAlready;
				IsAlready = cur_friend->m_LinkedClient->GetFriendSlot();
				theApp.friendlist->RemoveAllFriendSlots();
				if( !IsAlready ) {
					cur_friend->m_LinkedClient->SetFriendSlot(true);
				}
			}
			return true;			
			break;
		}
	}

	// Kry - Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);
}
