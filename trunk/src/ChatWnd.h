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

#ifndef CHATWND_H
#define CHATWND_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ChatWnd.h"
#endif

#include <wx/panel.h>		// Needed for wxPanel
#include "Types.h"

class CDlgFriend;
class CUpDownClient;
class CChatSelector;
class CFriendListCtrl;
class CMD4Hash;

class CChatWnd : public wxPanel 
{
public:
	CChatWnd(wxWindow* pParent = NULL); 
	~CChatWnd() {};

	void StartSession(CDlgFriend* friend_client, bool setfocus = true);

	CDlgFriend* FindFriend(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);	
	void	AddFriend(CUpDownClient* toadd);
	void	AddFriend(const CMD4Hash& userhash, const wxString& name, uint32 lastUsedIP, uint32 lastUsedPort);
	void	RefreshFriend(const CMD4Hash& userhash, const wxString& name, uint32 lastUsedIP, uint32 lastUsedPort);

	void	ProcessMessage(uint64 sender, const wxString& message);
	void 	ConnectionResult(bool success, const wxString& message, uint64 id);

	void	SendMessage(const wxString& message, const wxString& client_name = wxEmptyString, uint64 to_id = 0);
		
protected:
	void	OnBnClickedCsend(wxCommandEvent& evt);
	void	OnBnClickedCclose(wxCommandEvent& evt);

	DECLARE_EVENT_TABLE()

	CFriendListCtrl* friendlistctrl;
	CChatSelector*	chatselector;
};

#endif
