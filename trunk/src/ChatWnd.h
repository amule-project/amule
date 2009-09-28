//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef CHATWND_H
#define CHATWND_H

#include <wx/panel.h>		// Needed for wxPanel
#include <wx/notebook.h>		// Needed for wxNotebookEvent
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
	void	RemoveFriend(const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort);
	void	RefreshFriend(class CFriend* Friend, bool connected);

	void	ProcessMessage(uint64 sender, const wxString& message);
	void 	ConnectionResult(bool success, const wxString& message, uint64 id);

	void	SendMessage(const wxString& message, const wxString& client_name = wxEmptyString, uint64 to_id = 0);

	bool	IsIdValid(uint64 id);
	void	ShowCaptchaResult(uint64 id, bool ok);
	void	EndSession(uint64 id);
		
protected:
	/**
	 * Event-handler for displaying the chat-popup menu.
	 */
	void	OnNMRclickChatTab(wxMouseEvent& evt);
	/**
	 * Event-handler fo the Close item on the popup-menu.
	 */
	void	OnPopupClose(wxCommandEvent& evt);
	
	/**
	 * Event-handler fo the CloseAll item on the popup-menu.
	 */
	void	OnPopupCloseAll(wxCommandEvent& evt);

	/**
	 * Event-handler fo the CloseOthers item on the popup-menu.
	 */
	void	OnPopupCloseOthers(wxCommandEvent& evt);

	/**
	 * Event-handler fo the AddFriend item on the popup-menu.
	 */
	void	OnAddFriend(wxCommandEvent& evt);

	void	OnBnClickedCsend(wxCommandEvent& evt);
	void	OnBnClickedCclose(wxCommandEvent& evt);
	void	OnAllPagesClosed(wxNotebookEvent& evt);
	void	CheckNewButtonsState();

	DECLARE_EVENT_TABLE()

	//! Variable used to ensure that the popup menu doesn't get displayed twice.
	wxMenu* m_menu;
	//! Pointer to the control serving as the friend list
	CFriendListCtrl* friendlistctrl;
	//! Pointer to the chat tabs.
	CChatSelector*	chatselector;
};

#endif
// File_checked_for_headers
