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

#ifndef UPLOADLISTCTRL_H
#define UPLOADLISTCTRL_H

#include <wx/imaglist.h>
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CUpDownClient;

// CUploadListCtrl

#ifndef SW_HIDE
	#define SW_HIDE 1
#endif
#ifndef SW_SHOW
	#define SW_SHOW 2
#endif

class CUploadListCtrl : public CMuleListCtrl {
public:
	CUploadListCtrl();
	CUploadListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);

	virtual		~CUploadListCtrl();
	void		Init();
	void		InitSort();
	void		AddClient(CUpDownClient* client);
	void		RemoveClient(CUpDownClient* client);
	void		RefreshClient(CUpDownClient* client);
	void		Hide() {/*ShowWindow(SW_HIDE);*/}
	void		Visable() {/*ShowWindow(SW_SHOW);*/}
	void		Localize();
	virtual void	OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted);
	void		ShowSelectedUserDetails();
private:
	CPreferences::Table TablePrefs() { return CPreferences::tableUpload; }
	static int wxCALLBACK SortProc(long lp1,long lp2,long lpSort);

	void		OnColumnClick(wxListEvent& evt);
	void		OnNMRclick(wxMouseEvent& evt);


	bool		ProcessEvent(wxEvent& evt);

	bool		asc_sort[7];	 
	wxImageList	imagelist;
	wxMenu*		m_ClientMenu;
	wxBrush*	m_hilightBrush,*m_hilightUnfocusBrush;

	DECLARE_EVENT_TABLE()
};

#endif // UPLOADLISTCTRL_H
