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

// AddFriend.cpp : implementation file
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "AddFriend.h"		// Interface declarations.
#include "muuli_wdr.h"		// Needed for addFriendDlg
#include "CString.h"	// Needed for CString
#include "amule.h"		// Needed for theApp
#include "FriendList.h"		// Needed for CFriendList

// CAddFriend dialog

//IMPLEMENT_DYNAMIC(CAddFriend, CDialog)
IMPLEMENT_DYNAMIC_CLASS(CAddFriend,wxDialog)

CAddFriend::CAddFriend(wxWindow* parent)
: wxDialog(parent,9995,_("Add a Friend"),wxDefaultPosition,wxDefaultSize,
wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	wxSizer* content=addFriendDlg(this,TRUE);
	content->Show(this,TRUE);
	OnInitDialog();
}

CAddFriend::~CAddFriend()
{
}

bool CAddFriend::OnInitDialog()
{
	return true;
}

BEGIN_EVENT_TABLE(CAddFriend,wxDialog)
	EVT_BUTTON(ID_ADDFRIEND,CAddFriend::OnAddBtn)
	EVT_BUTTON(ID_CLOSEDLG,CAddFriend::OnCloseBtn)
END_EVENT_TABLE()

// CAddFriend message handlers

#define GetDlgItem(a,b) wxStaticCast(FindWindowById((a)),b)

void CAddFriend::OnAddBtn(wxEvent& evt)
{
	CString name,userhash, fullip;
	uint32 ip;
	uint16 port;
	ip=port=0;
	if( GetDlgItem(ID_USERNAME,wxTextCtrl)->GetValue().Length() ) {
		name.Format("%s",GetDlgItem(ID_USERNAME,wxTextCtrl)->GetValue().GetData());
	}
	if(GetDlgItem(ID_IPADDRESS,wxTextCtrl)->GetValue().Length()) {
		fullip.Format("%s", GetDlgItem(ID_IPADDRESS,wxTextCtrl)->GetValue().GetData());
	} else {
		wxMessageBox(_("You have to enter a valid IP and port!"));
		return;
	}
	if(GetDlgItem(ID_IPORT,wxTextCtrl)->GetValue().Length()) {
		wxString buff=GetDlgItem(ID_IPORT,wxTextCtrl)->GetValue();
		port = (atoi(buff.GetData())) ? atoi(buff.GetData()) : 0;
	} else {
		wxMessageBox(_("You have to enter a valid IP and port!"));
		return;
	}
	int a,b,c,d;
	a=b=c=d=0;
	sscanf(fullip.GetData(),"%d.%d.%d.%d",&a,&b,&c,&d);
	ip=a|(b<<8)|(c<<16)|(d<<24);
	theApp.friendlist->AddFriend(NULL, 0, ip, port, 0, name, 0 );
	EndModal(1);
}

void CAddFriend::OnCloseBtn(wxEvent& evt)
{
	EndModal(0);
}
