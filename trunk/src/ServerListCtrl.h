// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef SERVERLISTCTRL_H
#define SERVERLISTCTRL_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/event.h>		// Needed before wx/listbase.h
#include <wx/listbase.h>	// Needed for wxListItem
#include <wx/imaglist.h>        // Needed for wxImageList

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CServer;
class CServerList;

class serverListItem : public wxListItem {
 public:
  serverListItem() {};
  ~serverListItem() {};

  CServer* data;
};

// CServerListCtrl

class CServerListCtrl : public CMuleListCtrl 
{
public:
	CServerListCtrl();
	CServerListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);

//	void	ShowServers();
	virtual ~CServerListCtrl() { }
	bool	Init(CServerList* in_list);
	void InitSort();
	bool	AddServer(CServer* toadd,bool bAddToList = true);
	void	RemoveServer(CServer* todel,bool bDelToList = true);
	void	RemoveAllServers(int state);
	bool	AddServermetToList(wxString strFile);
	void	HighlightServer(const CServer* server, bool highlight);
	void	RefreshServer(CServer* server);
	void	RemoveDeadServer();
	//void	Hide() {ShowWindow(SW_HIDE);}
	//void	Visable() {ShowWindow(SW_SHOW);}
	void	Localize();
	void	ShowFilesCount();

	void OnRclickServlist(wxListEvent& event);
	void OnLDclick(wxMouseEvent& event);
	void OnConnectTo(wxCommandEvent& event);
	void OnCopyLink(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

	void OnRemove(wxEvent& evt);
	void OnLvnColumnclickServlist(wxListEvent& evt);
	static int wxCALLBACK SortProc(long item1,long item2,long sortData);

	bool	asc_sort[9]; 
	bool ProcessEvent(wxEvent& evt);

	// Barry - New methods
	bool StaticServerFileAppend(CServer *server);
	bool StaticServerFileRemove(CServer *server);
	
	long connected;

protected:
	CPreferences::Table TablePrefs()	{ return CPreferences::tableServer; }

private:
	CServerList*	server_list;
	wxImageList		imagelist;
	wxMenu* m_ServerPrioMenu;
	wxMenu* m_ServerMenu;
};

#endif // SERVERLISTCTRL_H
