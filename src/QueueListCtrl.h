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

#ifndef QUEUELISTCTRL_H
#define QUEUELISTCTRL_H

#include "MuleListCtrl.h"		// Needed for CMuleListCtrl
#include <wx/imaglist.h>

class CUpDownClient;

#ifndef SW_HIDE
	#define SW_HIDE 1
#endif
#ifndef SW_SHOW
	#define SW_SHOW 2
#endif

class CQueueListCtrl : public CMuleListCtrl
{
  //DECLARE_DYNAMIC(CQueueListCtrl)

public:
	CQueueListCtrl();
	CQueueListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);

	virtual ~CQueueListCtrl();
	void	Init();
	void InitSort();
	void	AddClient(CUpDownClient* client);
	void	RemoveClient(CUpDownClient* client);
	void	RefreshClient(CUpDownClient* client);
	void	Hide() {/*ShowWindow(SW_HIDE);*/}
	void	Visable() {/*ShowWindow(SW_SHOW);*/}
	void	Localize();

protected:
	virtual int TablePrefs();
	static int wxCALLBACK SortProc(long lp1,long lp2,long lpsort);
	void OnNMRclick(wxMouseEvent& evt);
	void OnColumnClick(wxListEvent& evt);
	bool ProcessEvent(wxEvent& evt);
	virtual void OnDrawItem(int item,wxDC* dc,const wxRect& rc,const wxRect& rectHL,bool hl);

	DECLARE_EVENT_TABLE()
private:
	bool		 asc_sort[8];	 

	wxMenu*	   m_ClientMenu;
	// Barry - Refresh the queue every 10 seconds
	wxTimer  m_hTimer;
 public:
	static void  QueueUpdateTimer(); //HWND hwnd, unsigned int uiMsg, unsigned int idEvent, DWORD dwTime);
	void OnTimer(wxTimerEvent& evt);

	void	ShowSelectedUserDetails();
	wxBrush* m_hilightBrush,*m_hilightUnfocusBrush;
};

#endif // QUEUELISTCTRL_H
