// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) created by Ornis
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

// CatDialog.cpp : implementation file

#include <sys/stat.h>
#include <sys/types.h>
#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/textctrl.h>
#include <wx/statbmp.h>
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW
#include <wx/dialog.h>
#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/dcmemory.h>
#include <wx/colordlg.h>
#include <wx/dirdlg.h>

#ifdef __WXMSW__
	#include <io.h>
#endif

#include "CatDialog.h"		// Interface declarations.
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "otherfunctions.h"	// Needed for MakeFoldername
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "muuli_wdr.h"		// Needed for CategoriesEditWindow
#include "color.h"		// Needed for RGB, GetColour, GetRValue, GetGValue and GetBValue

// CCatDialog dialog

IMPLEMENT_DYNAMIC_CLASS(CCatDialog, wxDialog)

CCatDialog::CCatDialog(wxWindow* parent,int index)
	: wxDialog(parent,-1,_("Category"),
	wxDefaultPosition,wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	SetBackgroundColour(GetColour(wxSYS_COLOUR_WINDOW));
	wxSizer* content=CategoriesEditWindow(this,TRUE);
	content->Show(this,TRUE);
	Center();
	m_myCat=theApp.glob_prefs->GetCategory(index);
	if (m_myCat==NULL) {
		return;
	}
}

bool CCatDialog::OnInitDialog()
{
	m_prio=((wxChoice*)FindWindowById(IDC_PRIOCOMBO));
	wxStaticBitmap* bitmap=((wxStaticBitmap*)FindWindowById(ID_BOX_CATCOLOR));
	m_bitmap=new wxBitmap(16,16);
	mkBitmap(*m_bitmap);
	bitmapcolor=bitmap;
	UpdateData();
	return true;
}

void CCatDialog::UpdateData()
{
	((wxTextCtrl*)FindWindowById(IDC_TITLE))->SetValue(m_myCat->title);
	((wxTextCtrl*)FindWindowById(IDC_INCOMING))->SetValue(m_myCat->incomingpath);
	((wxTextCtrl*)FindWindowById(IDC_COMMENT))->SetValue(m_myCat->comment);
	newcolor=m_myCat->color;
	m_prio->SetSelection(m_myCat->prio);
	mkBitmap(*m_bitmap);
	bitmapcolor->SetBitmap((wxBitmap&)*m_bitmap);
}

CCatDialog::~CCatDialog()
{
}

BEGIN_EVENT_TABLE(CCatDialog,wxDialog)
	EVT_BUTTON(wxID_OK,CCatDialog::OnBnClickedOk)
	EVT_BUTTON(IDC_CATCOLOR,CCatDialog::OnBnClickColor)
	EVT_BUTTON(IDC_BROWSE,CCatDialog::OnBnClickedBrowse)
END_EVENT_TABLE()

// CCatDialog message handlers

void CCatDialog::OnBnClickedBrowse(wxCommandEvent& WXUNUSED(evt))
{	
	wxString dir=wxDirSelector(_("Choose a folder for incoming files"),((wxTextCtrl*)FindWindowById(IDC_INCOMING))->GetValue());
	if(!dir.IsEmpty()) {
		((wxTextCtrl*)FindWindowById(IDC_INCOMING))->SetValue(dir);
	}
}

void CCatDialog::OnBnClickedOk(wxCommandEvent& WXUNUSED(evt))
{
	wxString oldpath=m_myCat->incomingpath;
	if (((wxTextCtrl*)FindWindowById(IDC_TITLE))->GetValue().Length()>0) {
		m_myCat->title = ((wxTextCtrl*)FindWindowById(IDC_TITLE))->GetValue();
	}

	if (((wxTextCtrl*)FindWindowById(IDC_INCOMING))->GetValue().Length()>1) {
		m_myCat->incomingpath = ((wxTextCtrl*)FindWindowById(IDC_INCOMING))->GetValue();
	}

	m_myCat->comment = ((wxTextCtrl*)FindWindowById(IDC_COMMENT))->GetValue();
	m_myCat->incomingpath = MakeFoldername(m_myCat->incomingpath);
	if (!::wxDirExists(m_myCat->incomingpath)) {
		::wxMkdir(m_myCat->incomingpath); // 0777 is the default
	}

	if (m_myCat->incomingpath.CmpNoCase(oldpath)!=0) {
		theApp.sharedfiles->AddFilesFromDirectory(m_myCat->incomingpath);
		theApp.sharedfiles->Reload();
	}

	m_myCat->color=newcolor;
	m_myCat->prio=m_prio->GetSelection();
	theApp.amuledlg->transferwnd->downloadlistctrl->Refresh();
	EndModal(0);
}

void CCatDialog::mkBitmap(wxBitmap& bitmap)
{
	wxMemoryDC dc;
	dc.SelectObject(bitmap);
	wxBrush* suti=wxTheBrushList->FindOrCreateBrush(wxColor(GetBValue(newcolor),GetGValue(newcolor),GetRValue(newcolor)),wxSOLID);
	dc.SetBrush(*suti);
	dc.DrawRectangle(0,0,16,16);
	dc.SetBrush(wxNullBrush);
	dc.SelectObject(wxNullBitmap);
}

void CCatDialog::OnBnClickColor(wxCommandEvent& WXUNUSED(evt))
{
	wxColour newcol=wxGetColourFromUser(this,wxColour(GetBValue(newcolor),GetGValue(newcolor),GetRValue(newcolor)));
	if(newcol.Ok()) {
		newcolor=RGB(newcol.Blue(),newcol.Green(),newcol.Red());
		mkBitmap(*m_bitmap);
		bitmapcolor->SetBitmap(*m_bitmap);
	}
}
