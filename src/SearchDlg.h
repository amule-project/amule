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
#include <wx/combobox.h>	// Needed for wxComboBox
#include <wx/imaglist.h>	// Needed for wxImageList
#include <wx/timer.h>		// Needed for wxTimer and wxTimerEvent
#include <wx/statbmp.h>		// Needed for wxStaticBitmap
#include <wx/listbase.h>	// Needed for wxListEvent
#include <wx/gauge.h>		// Needed for wxGauge
#include <wx/notebook.h>	// Needed for wxNotebookEvent

#include "types.h"		// Needed for uint16 and uint32
#include "resource.h"		// Needed for IDD_SEARCH
#include "CString.h"		// Needed for CString

class CSearchListCtrl;
class CMuleNotebookEvent;
class Packet;

// CSearchDlg dialog
class CSearchDlg : public wxPanel {
DECLARE_DYNAMIC_CLASS(CSearchDlg)
public:
	CSearchDlg() {};                 // dummy constructor
	CSearchDlg(wxWindow* pParent);   // standard constructor
	virtual ~CSearchDlg();           // destructor

	// public methods
	uint8		GetCatChoice();
	void		DeleteAllSearchs();
	void		ToggleLinksHandler();
	bool  		CheckTabNameExists(wxString searchString);
	void  		CreateNewTab(wxString searchString,uint32 nSearchID);
	void		DeleteSearch(uint16 nSearchID);
	void		LocalSearchEnd(uint16 count);
	void		UpdateCatChoice();
	void		AddUDPResult(uint16 count);

	enum { IDD = IDD_SEARCH };

	// public member variables
	CSearchListCtrl* searchlistctrl;

	// event handlers
	void 		OnBnClickedStarts(wxCommandEvent& evt);
	void 		OnBnClickedSdownload(wxCommandEvent& ev);
	void 		OnBnClickedCancels(wxCommandEvent& evt);
private:
	// event handlers
	void 		OnFieldsChange(wxCommandEvent& evt);
	void 		OnTimer(wxTimerEvent &evt);
	void 		OnListItemSelected(wxListEvent& ev);
	void 		OnBnClickedSearchReset(wxCommandEvent& ev);
	void 		OnBnClickedClearall(wxCommandEvent& ev);
	void 		OnRMButton(wxMouseEvent& evt);

	// private methods
	void 		StartNewSearch();
	wxString	CreateWebQuery();
	void 		OnSearchClosed(wxNotebookEvent& evt);
	void 		DirectDownload(wxCommandEvent &event);
	void		DownloadSelected();
	bool		GetGlobSearchStatus() {return globsearch;}
	virtual bool 	ProcessEvent(wxEvent& evt);

	// private member variables
	Packet*		searchpacket;
	unsigned int*	global_search_timer;
	wxGauge* 	searchprogress;
	bool		canceld;
	bool		globsearch;
	uint16		servercount;
	uint16		m_nSearchID;
	wxComboBox	typebox;
	wxComboBox	Stypebox;
	wxImageList	m_ImageList;
	wxImageList     m_StateImageList;
	wxTimer 	m_timer;
	wxStaticBitmap	m_ctrlSearchFrm;
	wxStaticBitmap	m_ctrlWebSearchFrm;
	wxStaticBitmap	m_ctrlDirectDlFrm;

	DECLARE_EVENT_TABLE()
};

// ESearchType
enum ESearchType
{
	//NOTE: The numbers are *equal* to the entries in the comboxbox -> TODO: use item data
	SearchTypeServer = 0,
	SearchTypeGlobal,
	SearchTypeJigleSOAP,
	SearchTypeJigle,
	SearchTypeFileDonkey
};

// SSearchParams
struct SSearchParams
{
	SSearchParams(void) : dwSearchID((DWORD)-1), eType(SearchTypeServer),
	                      ulMinSize(0), ulMaxSize(0), iAvailability(-1) { }
	DWORD dwSearchID;
	CString strExpression;
	ESearchType eType;
	CString strFileType;
	CString strMinSize;
	wxUint32 ulMinSize;
	CString strMaxSize;
	wxUint32 ulMaxSize;
	int iAvailability;
	CString strExtension;
	bool bMatchKeywords;
};
#endif // SEARCHDLG_H
