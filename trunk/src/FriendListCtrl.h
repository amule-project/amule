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

#ifndef FRIENDLISTCTRL_H
#define FRIENDLISTCTRL_H

#include <wx/imaglist.h>
#include "MuleListCtrl.h"		// Needed for CMuleListCtrl

class CFriend;

// CFriendListCtrl

class CFriendListCtrl : public CMuleListCtrl
{
  //DECLARE_DYNAMIC(CFriendListCtrl)
  friend class CFriendList;

public:
	CFriendListCtrl();
	CFriendListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);
	virtual ~CFriendListCtrl();

	void	Init();
	void	Localize();

protected:
	DECLARE_EVENT_TABLE()
#if 0
	DECLARE_MESSAGE_MAP()
#endif

	CPreferences::Table TablePrefs()	{ return CPreferences::tableNone; }
	void	AddFriend(CFriend* toadd);
	void	RemoveFriend(CFriend* toremove);
	void	RefreshFriend(CFriend* toupdate);
#if 0
	bool	OnCommand(WPARAM wParam,LPARAM lParam );
	void	OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
#endif
	virtual bool ProcessEvent(wxEvent& evt);
	void OnNMRclick(wxMouseEvent& evt);
private:
	//CTitleMenu m_ClientMenu;
	wxImageList imagelist;
};

#endif // FRIENDLISTCTRL_H
