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

// ChatWnd.cpp : implementation file

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/settings.h>	// Needed for wxSYS_COLOUR_WINDOW
#include <wx/html/htmlwin.h>	// Needed for wxHtmlWindow

#include "ChatWnd.h"		// Interface declarations.
#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "updownclient.h"	// Needed for CUpDownClient
#include "ChatSelector.h"	// Needed for CChatSelector
#include "muuli_wdr.h"		// Needed for messagePage
#include "color.h"		// Needed for GetColour

// CChatWnd dialog

//IMPLEMENT_DYNAMIC(CChatWnd, CDialog)
CChatWnd::CChatWnd(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent,CChatWnd::IDD)
{
	wxSizer* content=messagePage(this,TRUE);
	content->Show(this,TRUE);

	OnInitDialog();	

	wxHtmlWindow *win=(wxHtmlWindow*)FindWindowById(ID_HTMLWIN);
	int sizes[]={7,8,10,12,16,22,30};
	win->SetFonts("","",sizes);
}

CChatWnd::~CChatWnd()
{
}

BEGIN_EVENT_TABLE(CChatWnd,wxPanel)
	EVT_BUTTON(IDC_CSEND,CChatWnd::OnBnClickedCsend)
	EVT_BUTTON(IDC_CCLOSE,CChatWnd::OnBnClickedCclose)
	EVT_TEXT_ENTER(IDC_CMESSAGE,CChatWnd::OnBnClickedCsend)
END_EVENT_TABLE()

bool CChatWnd::OnInitDialog()
{
	// CResizableDialog::OnInitDialog();
	// Localize();
	// chatselector=new CChatSelector();
	chatselector=(CChatSelector*)FindWindowById(IDC_CHATSELECTOR);
	chatselector->Init();

	return true;
}

void CChatWnd::StartSession(CUpDownClient* client)
{
	if (!client->GetUserName()) {
		return;
	}

	theApp.amuledlg->SetActiveDialog(this);
	chatselector->StartSession(client,true);
}

#define GetDlgItem(a,b) wxStaticCast(FindWindowById((a)),b)

void CChatWnd::OnBnClickedCsend(wxEvent& evt)
{
	uint16 len = GetDlgItem(IDC_CMESSAGE,wxTextCtrl)->GetValue().Length()+2;
	char* messagetosend = new char[len+1];
	wxString str=GetDlgItem(IDC_CMESSAGE,wxTextCtrl)->GetValue();//>GetWindowText(messagetosend,len);
	strcpy(messagetosend,str.c_str());
	if (chatselector->SendMessage(messagetosend)) {
		GetDlgItem(IDC_CMESSAGE,wxTextCtrl)->SetValue("");
	}
	delete[] messagetosend;
	GetDlgItem(IDC_CMESSAGE,wxTextCtrl)->SetFocus();
}

void CChatWnd::OnBnClickedCclose(wxEvent& evt)
{
	chatselector->EndSession();
}

void CChatWnd::Localize()
{
}
