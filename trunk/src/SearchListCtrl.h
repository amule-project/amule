//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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


#ifndef SEARCHLISTCTRL_H
#define SEARCHLISTCTRL_H

#include "types.h"		// Needed for uint16 and uint32
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CSearchList;
class CSearchFile;

// CSearchListCtrl

class CSearchListCtrl : public CMuleListCtrl
{
  //DECLARE_DYNAMIC(CSearchListCtrl)

public:
	CSearchListCtrl();
	CSearchListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);

	virtual ~CSearchListCtrl();
	void	Init(CSearchList* in_searchlist);
	void	UpdateSources(CSearchFile* toupdate);
	void	AddResult(CSearchFile* toshow);
	void	RemoveResult( CSearchFile* toremove);
	void	Localize();
	void	ShowResults(uint32 nResultsID);
	//void OnRclick(wxListEvent& evt);
	void OnNMRclick(wxMouseEvent& evt);
	void OnLDclick(wxMouseEvent& evt);
	uint16	GetSearchId() { return m_nResultsID; }

	void    InvalidateSearchId() { m_nResultsID = (uint32)-1; }

protected:
	CPreferences::Table TablePrefs()	{ return CPreferences::tableSearch; }
	static int wxCALLBACK SortProc(long lParam1, long lParam2, long lParamSort);
	void OnColumnClick(wxListEvent& evt);

	bool ProcessEvent(wxEvent& evt);

	DECLARE_EVENT_TABLE()
private:
	void UpdateColor(long index,long count);

	//CTitleMenu	 m_SearchFileMenu;
	wxMenu* m_SearchFileMenu;
	CSearchList* searchlist;
	uint32		 m_nResultsID;
	bool		 asc_sort[7];	 
};

#endif // SEARCHLISTCTRL_H
