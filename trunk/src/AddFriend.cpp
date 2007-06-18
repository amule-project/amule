//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
// 
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#include "AddFriend.h"		// Interface declarations.
#include "muuli_wdr.h"		// Needed for addFriendDlg
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"	// Needed for amuleDlg
#include "ChatWnd.h"
#include "NetworkFunctions.h"
#include "OtherFunctions.h"
#include "MD4Hash.h"
#include <common/StringFunctions.h> // Needed for unicode2char 


BEGIN_EVENT_TABLE(CAddFriend, wxDialog)
	EVT_BUTTON(ID_ADDFRIEND, CAddFriend::OnAddBtn)
	EVT_BUTTON(ID_CLOSEDLG, CAddFriend::OnCloseBtn)
END_EVENT_TABLE()


CAddFriend::CAddFriend(wxWindow* parent)
: wxDialog(parent, 9995, _("Add a Friend"), wxDefaultPosition, wxDefaultSize,
wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	wxSizer* content=addFriendDlg(this, TRUE);
	content->Show(this, TRUE);
}

void CAddFriend::OnAddBtn(wxCommandEvent& WXUNUSED(evt))
{
	wxString name = CastChild(ID_USERNAME, wxTextCtrl)->GetValue().Strip(wxString::both);
	wxString hash = CastChild(ID_USERHASH, wxTextCtrl)->GetValue().Strip(wxString::both);
	wxString fullip = CastChild(ID_IPADDRESS, wxTextCtrl)->GetValue().Strip(wxString::both);
	uint16 port = StrToULong( CastChild(ID_IPORT, wxTextCtrl)->GetValue() );
	uint32 ip = StringIPtoUint32(fullip);
	
	if (!ip || !port) {
		wxMessageBox(_("You have to enter a valid IP and port!"), _("Information"), wxOK | wxICON_INFORMATION, this);
		return;		
	}
	
	CMD4Hash userhash;
	if ((not hash.IsEmpty()) and (not userhash.Decode(hash))) {
		wxMessageBox(_("The specified userhash is not valid!"), _("Information"), wxOK | wxICON_INFORMATION, this);
		return;
	};

	// Better than nothing at all...
	if ( name.IsEmpty() ) {
		name = fullip;
	}

	theApp->amuledlg->m_chatwnd->AddFriend( userhash,name, ip, port);
	
	EndModal(true); // Friend added
}


void CAddFriend::OnCloseBtn(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(false); // Friend not added
}

// File_checked_for_headers
