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


#ifndef SHAREDFILESWND_H
#define SHAREDFILESWND_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/gauge.h>		// Needed for wxGauge
#include <wx/statbmp.h>		// Needed for wxStaticBitmap
#include <wx/listbase.h>	// Needed for wxListEvent

#include "resource.h"		// Needed for IDD_FILES
#include "CMD4Hash.h"

class CKnownFile;
class CSharedFilesCtrl;

class CSharedFilesWnd : public wxPanel //CResizableDialog
{
  //DECLARE_DYNAMIC(CSharedFilesWnd)

public:
	CSharedFilesWnd(wxWindow* pParent = NULL);   // standard constructor
	virtual ~CSharedFilesWnd();
	void Localize();
	void Check4StatUpdate(CKnownFile* file);
// Dialog Data
	enum { IDD = IDD_FILES };
	CSharedFilesCtrl* sharedfilesctrl;

protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//virtual bool OnInitDialog();
	//virtual bool	PreTranslateMessage(MSG* pMsg);
	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()

	void OnLvnItemActivateSflist(wxListEvent& evt);
	//afx_msg void OnLvnItemActivateSflist(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnNMClickSflist(NMHDR *pNMHDR, LRESULT *pResult);
private:
	void ShowDetails(CKnownFile* cur_file);
#if 0
	CProgressCtrlX pop_bar;
	CProgressCtrlX pop_baraccept;
	CProgressCtrlX pop_bartrans;
#endif
	wxGauge* pop_bar;
	wxGauge* pop_baraccept;
	wxGauge* pop_bartrans;

	wxFont bold;
	CMD4Hash m_shownFileHash;
	wxStaticBitmap m_ctrlStatisticsFrm;
	void OnBtnReloadsharedfiles(wxCommandEvent &evt);
};

#endif // SHAREDFILESWND_H
