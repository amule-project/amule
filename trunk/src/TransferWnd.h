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

#ifndef TRANSFERWND_H
#define TRANSFERWND_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "types.h"		// Needed for uint32

class CUploadListCtrl;
class CDownloadListCtrl;
class CQueueListCtrl;
class CMuleNotebook;
class wxListCtrl;
class wxSplitterEvent;
class wxNotebookEvent;
class wxCommandEvent;
class wxMouseEvent;
class wxEvent;

// CTransferWnd dialog

class CTransferWnd : public wxPanel //CResizableDialog
{
  //DECLARE_DYNAMIC(CTransferWnd)
  DECLARE_DYNAMIC_CLASS(CTransferWnd)

public:
	CTransferWnd(wxWindow* pParent = NULL);   // standard constructor
	virtual ~CTransferWnd();
	void	ShowQueueCount(uint32 number);
	void	SwitchUploadList(wxCommandEvent& evt);
	void	Localize();
	void	UpdateCatTabTitles();
	bool	OnInitDialog();
	CUploadListCtrl*		uploadlistctrl;
	CDownloadListCtrl*	downloadlistctrl;
	CQueueListCtrl*		queuelistctrl;
	
	// TODO: client list on transfer window
	//CClientListCtrl			clientlistctrl;
	
	//wxToolTip		m_ttip;
	bool			windowtransferstate;
protected:
	void DoResize(int delta);
	void UpdateSplitterRange();
	void SetInitLayout();
	virtual bool ProcessEvent(wxEvent& evt);

	void DoSplitResize(int delta);
	void UpdateToolTips(void);
	int GetItemUnderMouse(wxListCtrl* ctrl);
	void OnSelchangeDltab(wxNotebookEvent& evt);
	void OnNMRclickDLtab(wxMouseEvent& evt);
    void OnSashPositionChanged(wxSplitterEvent& evt);

	int AddCategorie(wxString newtitle,wxString newincoming,wxString newcomment,bool addTab);
	void EditCatTabLabel(int index,wxString newlabel);

	CMuleNotebook* m_dlTab;

	DECLARE_EVENT_TABLE()

	//DECLARE_MESSAGE_MAP()

private:
	wxString m_strToolTip;
	int m_iOldToolTipItemDown;
	int m_iOldToolTipItemUp;
	int m_iOldToolTipItemQueue;	
	int rightclickindex;
	bool CatMenu;
	void OnBtnClearDownloads(wxCommandEvent &evt);
	//void OnBtnSwitchUpload(wxCommandEvent &evt);
};

#endif // TRANSFERWND_H
