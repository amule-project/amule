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

#ifndef FILEDETAILDIALOG_H
#define FILEDETAILDIALOG_H

#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/timer.h>		// Needed for wxTimer

class CPartFile;

// CFileDetailDialog dialog

class CFileDetailDialog : public wxDialog //CDialog
{
  //DECLARE_DYNAMIC(CFileDetailDialog)

public:
	CFileDetailDialog(wxWindow* parent, CPartFile* file);   // standard constructor
	virtual ~CFileDetailDialog();
	//virtual bool OnInitDialog();
	void Localize();

protected:
	void OnTimer(wxTimerEvent& evt);
#if 0
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnTimer(unsigned int nIDEvent);
	void OnDestroy();
	DECLARE_MESSAGE_MAP()

	static int CALLBACK CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);	// Tarod [Juanjo]
#endif
	DECLARE_EVENT_TABLE()

private:
	void UpdateData();
	CPartFile* m_file;
	struct SourcenameItem {
		wxString	name;
		long		count;
	};
	//uint32 m_timer;
	wxTimer m_timer;
	void OnClosewnd(wxCommandEvent& evt);
	//afx_msg void OnBnClickedButtonrename();	// Tarod [Juanjo]
	//afx_msg void OnBnClickedButtonStrip();
	//afx_msg void TakeOver();
	void FillSourcenameList();

	void OnBnClickedButtonrename(wxCommandEvent& evt);
	void OnBnClickedButtonStrip(wxCommandEvent& evt);
	void OnBnClickedFileinfo(wxCommandEvent& evt);
	void OnBnClickedShowComment(wxCommandEvent& evt);//for Comment//
	void OnBnClickedTakeOver(wxCommandEvent& evt);
	void OnListClickedTakeOver(wxListEvent& evt);
	void OnBnClickedRename(wxCommandEvent& evt);

};

#endif // FILEDETAILDIALOG_H
