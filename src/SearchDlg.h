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


#ifndef SEARCHDLG_H
#define SEARCHDLG_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/timer.h>		// Needed for wxTimer and wxTimerEvent

#include "types.h"		// Needed for uint16 and uint32

#include <set>

class CMuleNotebook;
class CSearchListCtrl;
class CMuleNotebookEvent;
class Packet;
class CServer;

class wxListEvent;
class wxNotebookEvent;
class wxSpinEvent;
class wxGauge;

class CSearchDlg : public wxPanel {
public:
	CSearchDlg(wxWindow* pParent);   
	~CSearchDlg() {};

	uint8		GetCatChoice();
	void		DeleteAllSearchs();
	void		ToggleLinksHandler();
	bool		CheckTabNameExists(wxString searchString);
	void		CreateNewTab(wxString searchString, long nSearchID);
	void		DeleteSearch(uint16 nSearchID);
	void		LocalSearchEnd(uint16 count);
	void		UpdateCatChoice();

	void		UpdateHitCount(CSearchListCtrl* page);

	void		FieldsChanged();

	// Event handlers
	void		OnBnClickedStarts(wxCommandEvent& evt);
	void		OnBnClickedSdownload(wxCommandEvent& ev);
	void		OnBnClickedCancels(wxCommandEvent& evt);

	void		OnPopupClose(wxCommandEvent& evt);
	void		OnPopupCloseAll(wxCommandEvent& evt);
	void		OnPopupCloseOthers(wxCommandEvent& evt);

	CMuleNotebook*	notebook;
	wxGauge*	progressbar;
private:
	// Event handlers
	void		OnEditFieldsChange(wxCommandEvent& evt);
	void		OnSpinFieldsChange(wxSpinEvent& evt);
	void		OnTimer(wxTimerEvent &evt);
	void		OnListItemSelected(wxListEvent& ev);
	void		OnBnClickedSearchReset(wxCommandEvent& ev);
	void		OnBnClickedClearall(wxCommandEvent& ev);
	void		OnExtendedSearchChange(wxCommandEvent& ev);
	void		OnRMButton(wxMouseEvent& evt);
	void        OnBtnWebSearch(wxCommandEvent &evt);

	void		StartNewSearch();
	void		OnSearchClosed(wxNotebookEvent& evt);
	void		OnSearchPageChanged(wxNotebookEvent& evt);
	void		DirectDownload(wxCommandEvent &event);

	Packet*		searchpacket;

	bool		canceld;
	bool		globalsearch;

	uint32	last_search_time;

	// Used to keep track of the servers we have sent UDP packet to
	std::set<CServer*> askedlist;
	
	DECLARE_EVENT_TABLE()
};

#endif // SEARCHDLG_H
