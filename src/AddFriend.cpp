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
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "AddFriend.h"		// Interface declarations.
#include "muuli_wdr.h"		// Needed for addFriendDlg
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"	// Needed for amuleDlg
#include "ChatWnd.h"
#include "otherfunctions.h"
#include "CMD4Hash.h"
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

CAddFriend::CAddFriend(wxWindow* parent)
: wxDialog(parent, 9995, _("Add a Friend"), wxDefaultPosition, wxDefaultSize,
wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	wxSizer* content=addFriendDlg(this, TRUE);
	content->Show(this, TRUE);
}

BEGIN_EVENT_TABLE(CAddFriend, wxDialog)
	EVT_BUTTON(ID_ADDFRIEND, CAddFriend::OnAddBtn)
	EVT_BUTTON(ID_CLOSEDLG, CAddFriend::OnCloseBtn)
END_EVENT_TABLE()


void CAddFriend::OnAddBtn(wxCommandEvent& WXUNUSED(evt))
{
	int a, b, c, d, scancount;
	wxString name, fullip, hash;
	uint32 ip = 0;
	uint16 port = 0;
	
	name = CastChild(ID_USERNAME, wxTextCtrl)->GetValue();
	hash = CastChild(ID_USERHASH, wxTextCtrl)->GetValue();
	fullip = CastChild(ID_IPADDRESS, wxTextCtrl)->GetValue();
	port = StrToULong( CastChild(ID_IPORT, wxTextCtrl)->GetValue() );

	scancount = sscanf(unicode2char(fullip), "%d.%d.%d.%d", &a, &b, &c, &d);
	if ( scancount != 4 || port <= 0 ) {
		wxMessageBox(_("You have to enter a valid IP and port!"));
		return;
	};
	
	if ( hash.Length() != 0 && hash.Length() != 32 ) {
		wxMessageBox(_("The specified userhash is not valid!"));
		return;
	};

	ip = a | (b << 8) | (c << 16) | (d << 24);

	// Better than nothing at all...
	if ( name.IsEmpty() )
		name = fullip;

	CMD4Hash userhash;
	if ( !hash.IsEmpty() )
		userhash.Decode( hash );
		
	theApp.amuledlg->chatwnd->AddFriend( userhash, 0, ip, port, 0, name, ( hash.IsEmpty() ? 0 : 1 ) );
	
	EndModal(1);
}


void CAddFriend::OnCloseBtn(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}
