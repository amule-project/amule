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

#include <wx/utils.h>

#include "CamuleAppBase.h"	// Needed for theApp
#include "FriendList.h"		// Interface declarations.
#include "FriendListCtrl.h"	// Needed for CFriendListCtrl
#include "updownclient.h"	// Needed for CUpDownClient
#include "packets.h"		// Needed for CInvalidPacket
#include "Preferences.h"	// Needed for CPreferences
#include "CFile.h"		// Needed for CFile
#include "otherfunctions.h"	// Needed for GetTickCount
#include "Friend.h"		// Needed for CFriend

CFriendList::CFriendList(void)
{
	LoadList();
	m_nLastSaved = ::GetTickCount();
}

CFriendList::~CFriendList(void)
{
	SaveList();
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos))
		delete m_listFriends.GetAt(pos);
	m_listFriends.RemoveAll();	
}

bool CFriendList::LoadList()
{
	CFriend* Record = 0;
	CFile file;
	wxString strFileName =wxString(theApp.glob_prefs->GetAppDir()) +wxString("emfriends.met");
	if (!wxFileExists(strFileName)) {
		return false;
	}
	uint8 header;
	file.Open(strFileName);
	try {
		if ( 1 != file.Read(&header,1) ) {
			throw CInvalidPacket();
		}
		if (header != MET_HEADER) {
			throw CInvalidPacket();
		}
		uint32 nRecordsNumber;
		if ( 4 != file.Read(&nRecordsNumber,4) ) {
			throw CInvalidPacket();
		}
		ENDIAN_SWAP_I_32(nRecordsNumber);
		for (uint32 i = 0; i < nRecordsNumber; i++) {
			Record =  new CFriend();
			Record->LoadFromFile(&file);
			m_listFriends.Append(Record);//AddTail(Record);
		}
	}
	catch ( CInvalidPacket ) {
		file.Close();
		return false;
	}
	file.Close();
	return true;
}

void CFriendList::SaveList()
{
	CFile file;
	wxString strFileName = wxString(theApp.glob_prefs->GetAppDir()) + wxString("emfriends.met");
	if(!file.Create(strFileName,TRUE)) {
		return;
	}
	uint8 header = MET_HEADER;
	file.Write(&header,1);
	uint32 nRecordsNumber = m_listFriends.GetCount();
	ENDIAN_SWAP_I_32(nRecordsNumber);
	file.Write(&nRecordsNumber,4);
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		m_listFriends.GetAt(pos)->WriteToFile(&file);
	}	
	file.Close();
}

CFriend* CFriendList::SearchFriend(const uchar* abyUserHash, uint32 dwIP, uint16 nPort) const {
	POSITION pos = m_listFriends.GetHeadPosition();
	while (pos){
		CFriend* cur_friend = m_listFriends.GetNext(pos);
		// to avoid that unwanted clients become a friend, we have to distinguish between friends with
		// a userhash and of friends which are identified by IP+port only.
		if (cur_friend->m_dwHasHash){
			// check for a friend which has the same userhash as the specified one
			if (!md4cmp(cur_friend->m_abyUserhash, abyUserHash))
				return cur_friend;
		}
		else{
			if (cur_friend->m_dwLastUsedIP == dwIP && cur_friend->m_nLastUsedPort == nPort)
				return cur_friend;
		}
	}
	return NULL;
}


void CFriendList::RefreshFriend(CFriend* torefresh)
{
	if (m_wndOutput) {
		m_wndOutput->RefreshFriend(torefresh);
	}
}

void CFriendList::ShowFriends()
{
	if (!m_wndOutput){
		wxASSERT ( false );
		return;
	}
	m_wndOutput->DeleteAllItems();
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		m_wndOutput->AddFriend(cur_friend);	
	}
}

//Added this to work with the IRC.. Probably a better way to do it.. But wanted this in the release..

void CFriendList::AddFriend( uchar t_m_abyUserhash[16], uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, wxString tm_strName, uint32 tm_dwHasHash)
{
	CFriend* Record = 0;
	Record = new CFriend( t_m_abyUserhash, tm_dwLastSeen, tm_dwLastUsedIP, tm_nLastUsedPort, tm_dwLastChatted, tm_strName, tm_dwHasHash );
	m_listFriends.Append(Record);//AddTail(Record);
	ShowFriends();
}

// Added for the friends function in the IRC..

bool CFriendList::IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort )
{
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		if ( cur_friend->m_dwLastUsedIP == dwLastUsedIP && cur_friend->m_nLastUsedPort == nLastUsedPort ){
			return true;
		}
	}
	return false;
}

void CFriendList::AddFriend(CUpDownClient* toadd)
{
	if (toadd->IsFriend()) {
		return;
	}
	CFriend* NewFriend = new CFriend(toadd);
	toadd->m_Friend = NewFriend;
	m_listFriends.Append(NewFriend);//AddTail(NewFriend);
	if (m_wndOutput) {
		m_wndOutput->AddFriend(NewFriend);
	}
	this->SaveList();
}

void CFriendList::RemoveFriend(CFriend* todel){
	POSITION pos = m_listFriends.Find(todel);
	if (!pos){
		wxASSERT ( false );
		return;
	}
	if (todel->m_LinkedClient){
		todel->m_LinkedClient->SetFriendSlot(false);
		todel->m_LinkedClient->m_Friend = NULL;
		todel->m_LinkedClient = NULL;
	}
	if (m_wndOutput)
		m_wndOutput->RemoveFriend(todel);
	m_listFriends.RemoveAt(pos);
	delete todel;
	SaveList();
}


void CFriendList::RemoveAllFriendSlots()
{
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		if (cur_friend->m_LinkedClient){
			cur_friend->m_LinkedClient->SetFriendSlot(false);
		}
	}	
}

void CFriendList::Process()
{
/* Madcat - No need to save friends every second, as
 * they are saved during exiting anyway.
 */
}
