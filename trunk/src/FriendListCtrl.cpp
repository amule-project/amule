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
//

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/menu.h>		// Needed for wxMenu

#include "amuleDlg.h"		// Needed for CamuleDlg
#include "FriendListCtrl.h"	// Interface declarations
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "AddFriend.h"		// Needed for CAddFriend
#include "ClientList.h"		// Needed for CClientList
#include "ChatWnd.h"		// Needed for CChatWnd
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for MP_DETAIL
#include "updownclient.h"	// Needed for CUpDownClient
#include "Friend.h"		// Needed for CFriend
#include "CFile.h"		// Needed for CFile
#include "endianfix.h"
#include "otherfunctions.h"


BEGIN_EVENT_TABLE(CFriendListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CFriendListCtrl::OnNMRclick)
	EVT_LIST_ITEM_SELECTED(ID_FRIENDLIST, CFriendListCtrl::OnItemSelected)
	EVT_LIST_ITEM_ACTIVATED(ID_FRIENDLIST, CFriendListCtrl::OnItemActivated) 
	
	EVT_MENU(MP_MESSAGE, CFriendListCtrl::OnPopupMenu)
	EVT_MENU(MP_REMOVEFRIEND, CFriendListCtrl::OnPopupMenu)
	EVT_MENU(MP_ADDFRIEND, CFriendListCtrl::OnPopupMenu)
	EVT_MENU(MP_DETAIL, CFriendListCtrl::OnPopupMenu)
	EVT_MENU(MP_SHOWLIST, CFriendListCtrl::OnPopupMenu)
	EVT_MENU(MP_FRIENDSLOT, CFriendListCtrl::OnPopupMenu)
END_EVENT_TABLE()



CFriendListCtrl::CFriendListCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags)
: CMuleListCtrl(parent, id, pos, siz, flags)
{
  InsertColumn(0, _("Username"), wxLIST_FORMAT_LEFT, siz.GetWidth() - 4);
  
  // The preferences class is not availble at shutdown, so I save the path here
  m_metfile = wxString(theApp.glob_prefs->GetAppDir()) + wxString("emfriends.met"); 
  
  LoadList();
}


CFriendListCtrl::~CFriendListCtrl()
{
	SaveList();

	for ( int i = 0; i < GetItemCount(); i++ ) 
		delete (CFriend*)GetItemData(i);
}


void CFriendListCtrl::AddFriend(CFriend* toadd)
{
	uint32 itemnr = InsertItem(GetItemCount(), toadd->m_strName);
	SetItemData(itemnr, (long)toadd);
}


void CFriendListCtrl::AddFriend( uchar userhash[16], uint32 lastSeen, uint32 lastUsedIP, uint32 lastUsedPort, uint32 lastChatted, wxString name, uint32 hasHash)
{
	CFriend* NewFriend = new CFriend( userhash, lastSeen, lastUsedIP, lastUsedPort, lastChatted, name, hasHash );

	AddFriend( NewFriend );
}


void CFriendListCtrl::AddFriend(CUpDownClient* toadd)
{
	if ( toadd->IsFriend() ) {
		return;
	}
	
	CFriend* NewFriend = new CFriend( toadd );
	toadd->m_Friend = NewFriend;
	
	AddFriend( NewFriend );
}


void CFriendListCtrl::RemoveFriend(CFriend* toremove)
{
	sint32 itemnr = FindItem(-1, (long)toremove);
	
	if ( itemnr == -1 )
		return;
	
	if ( toremove->m_LinkedClient ){
		toremove->m_LinkedClient->SetFriendSlot(false);
		toremove->m_LinkedClient->m_Friend = NULL;
		toremove->m_LinkedClient = NULL;
	}
	delete toremove;
	
	DeleteItem(itemnr);
}


void CFriendListCtrl::RefreshFriend(CFriend* toupdate)
{
	sint32 itemnr = FindItem(-1, (long)toupdate);
	if (itemnr != -1) {
		SetItem(itemnr, 0, toupdate->m_strName);
		SaveList();
	}	
}


void CFriendListCtrl::OnItemSelected(wxListEvent& WXUNUSED(evt))
{
	// Force the list to refresh. For some reason, Refresh() wont work on its own...
	Freeze();
	Refresh(true);
	Thaw(); 
}


void CFriendListCtrl::OnItemActivated(wxListEvent& WXUNUSED(evt))
{
	int cursel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	
	CFriend* cur_friend = (CFriend*)GetItemData(cursel);
	
	if (cur_friend->m_LinkedClient) {
		theApp.amuledlg->chatwnd->StartSession(cur_friend->m_LinkedClient);
	} else {
		CUpDownClient* chatclient = new CUpDownClient(cur_friend->m_nLastUsedPort, cur_friend->m_dwLastUsedIP, 0, 0, 0);
		chatclient->SetUserName((char*)cur_friend->m_strName.c_str());
		theApp.clientlist->AddClient(chatclient);
		theApp.amuledlg->chatwnd->StartSession(chatclient);
	}
}


bool CFriendListCtrl::LoadList()
{
	if ( !wxFileExists(m_metfile) ) {
		return false;
	}
	
	bool result = false;
	
	CFile file;
	if ( file.Open(m_metfile) ) {
		
		uint8 header;
		if ( 1 == file.Read(&header, 1) ) {
			if ( header == MET_HEADER ) {
				uint32 nRecordsNumber;
				if ( 4 == file.Read(&nRecordsNumber, 4) ) {
		
					ENDIAN_SWAP_I_32(nRecordsNumber);
					for (uint32 i = 0; i < nRecordsNumber; i++) {
						CFriend* Record = new CFriend();
						Record->LoadFromFile(&file);
						AddFriend(Record);
					}
					
					result = true;
				}
			}
		}
		
		file.Close();
	} else if ( wxFileExists(m_metfile) ) {
		theApp.amuledlg->AddLogLine(false, _("Failed to open friendlist file 'emfriends.met' for reading!\n"));
	}
	
	return result;
}


void CFriendListCtrl::SaveList()
{
	CFile file;
	if( file.Create(m_metfile, true) ) {
		uint8 header = MET_HEADER;
		file.Write(&header, 1);
		
		uint32 nRecordsNumber = GetItemCount();
		ENDIAN_SWAP_I_32(nRecordsNumber);
		file.Write(&nRecordsNumber, 4);
		
		for ( int i = 0; i < GetItemCount(); i++ ) {
			((CFriend*)GetItemData(i))->WriteToFile(&file);
		}
		
		file.Close();	
	} else {
		theApp.amuledlg->AddLogLine(false, _("Failed to open friendlist file 'emfriends.met' for writing!\n"));
	}
}


CFriend* CFriendListCtrl::FindFriend(const uchar* abyUserHash, uint32 dwIP, uint16 nPort) const
{
	for ( int i = 0; i < GetItemCount(); i++ ) {
		CFriend* cur_friend = (CFriend*)GetItemData(i);
		
		// to avoid that unwanted clients become a friend, we have to distinguish between friends with
		// a userhash and of friends which are identified by IP+port only.
		if ( abyUserHash && cur_friend->m_dwHasHash ) {
			// check for a friend which has the same userhash as the specified one
			if (!md4cmp(cur_friend->m_abyUserhash, abyUserHash))
				return cur_friend;
		}
		else if (cur_friend->m_dwLastUsedIP == dwIP && cur_friend->m_nLastUsedPort == nPort) {
				return cur_friend;
		}
	}
	
	return NULL;
}


bool CFriendListCtrl::IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort )
{
	return FindFriend( NULL, dwLastUsedIP, nLastUsedPort );
}


void CFriendListCtrl::RemoveAllFriendSlots()
{
	for ( int i = 0; i < GetItemCount(); i++ ) {
		CFriend* cur_friend = (CFriend*)GetItemData(i);
		
		if ( cur_friend->m_LinkedClient ) {
				cur_friend->m_LinkedClient->SetFriendSlot(false);
		}
	}	
}


void CFriendListCtrl::OnNMRclick(wxMouseEvent& evt)
{
	CFriend* cur_friend = NULL;
	
	wxMenu* menu = new wxMenu(_("Friends"));
	int cursel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	
	if ( cursel != -1 ) {
		cur_friend = (CFriend*)GetItemData(cursel);
		menu->Append(MP_DETAIL, _("Show &Details"));
		menu->Enable(MP_DETAIL, cur_friend->m_LinkedClient);
	}
	
	menu->Append(MP_ADDFRIEND, _("Add a friend"));
	
	if (cursel != (-1)) {
		menu->Append(MP_REMOVEFRIEND, _("Remove Friend"));
		menu->Append(MP_MESSAGE, _("Send &Message"));
		menu->Append(MP_SHOWLIST, _("View Files"));
		menu->AppendCheckItem(MP_FRIENDSLOT, _("Establish Friend Slot"));
		
		if (cur_friend->m_LinkedClient) {
			menu->Enable(MP_FRIENDSLOT, true);
			menu->Check(MP_FRIENDSLOT, cur_friend->m_LinkedClient->GetFriendSlot());
		} else {
			menu->Enable(MP_FRIENDSLOT, false);
		}
	}
	
	PopupMenu(menu, evt.GetPosition());
}


void CFriendListCtrl::OnPopupMenu(wxCommandEvent& evt)
{
	int cursel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	
	CFriend* cur_friend = NULL;
	
	if (cursel != -1) {
		cur_friend = (CFriend*)GetItemData(cursel);
	}
	
	switch (evt.GetId()) {
		case MP_MESSAGE: {
			if (cur_friend->m_LinkedClient) {
				theApp.amuledlg->chatwnd->StartSession(cur_friend->m_LinkedClient);
			} else {
				CUpDownClient* chatclient = new CUpDownClient(cur_friend->m_nLastUsedPort, cur_friend->m_dwLastUsedIP, 0, 0, 0);
				chatclient->SetUserName((char*)cur_friend->m_strName.c_str());
				theApp.clientlist->AddClient(chatclient);
				theApp.amuledlg->chatwnd->StartSession(chatclient);
			}
			break;
		}
		
		case MP_REMOVEFRIEND: {
			RemoveFriend(cur_friend);
			SaveList();
			break;
		}
		
		case MP_ADDFRIEND: {
			CAddFriend* dialog2 = new CAddFriend(this); 
			dialog2->ShowModal();
			delete dialog2;
			SaveList();
			break;
		}
		
		case MP_DETAIL: {
			if (cur_friend->m_LinkedClient) {
				CClientDetailDialog* dialog = new CClientDetailDialog(this, cur_friend->m_LinkedClient);
				dialog->ShowModal();
				delete dialog;
			}
			break;
		}
		
		case MP_SHOWLIST: {
			if (cur_friend->m_LinkedClient) {
				cur_friend->m_LinkedClient->RequestSharedFileList();
			} else {
				CUpDownClient* newclient = new CUpDownClient(cur_friend->m_nLastUsedPort, cur_friend->m_dwLastUsedIP, 0, 0, 0);
				newclient->SetUserName((char*)cur_friend->m_strName.c_str());
				theApp.clientlist->AddClient(newclient);
				newclient->RequestSharedFileList();
			}
			break;
		}
		
		case MP_FRIENDSLOT: {
			if (cur_friend && cur_friend->m_LinkedClient) {
				bool IsAlready = cur_friend->m_LinkedClient->GetFriendSlot();
				RemoveAllFriendSlots();
				if( !IsAlready ) {
					cur_friend->m_LinkedClient->SetFriendSlot(true);
				}
			}
			break;
		}
	}
}
