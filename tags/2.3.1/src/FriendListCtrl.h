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

#ifndef FRIENDLISTCTRL_H
#define FRIENDLISTCTRL_H

#include "MuleListCtrl.h"
#include "MD4Hash.h"

class wxString;
class CFriend;

class CFriendListCtrl : public CMuleListCtrl
{
public:
	CFriendListCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags);
	~CFriendListCtrl();
	
	void 		UpdateFriend(CFriend* toupdate);
	void		RemoveFriend(CFriend* todel);

protected:
	DECLARE_EVENT_TABLE()

	void	OnRightClick(wxMouseEvent& event);
		
private:
	void	OnItemActivated(wxListEvent& event);

	// Menu Items
	void	OnShowDetails(wxCommandEvent& event);
	void	OnSendMessage(wxCommandEvent& event);
	void	OnRemoveFriend(wxCommandEvent& event);
	void	OnSetFriendslot(wxCommandEvent& event);
	void	OnAddFriend(wxCommandEvent& event);
	void	OnViewFiles(wxCommandEvent& event);
	void	OnKeyPressed(wxKeyEvent& event);
};

#endif
// File_checked_for_headers
