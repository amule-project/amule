//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

//
// KadDlg.cpp : implementation file
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "KadDlg.h"
#endif

#include <wx/textctrl.h>
#include "KadDlg.h"
#include "muuli_wdr.h"

#include "CMD4Hash.h"
#include "OtherFunctions.h"
#include <wx/sizer.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(CKadDlg, wxPanel)
	
	EVT_TEXT(ID_NODE_IP1, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_IP2, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_IP3, CKadDlg::OnFieldsChange)	
	EVT_TEXT(ID_NODE_IP4, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_PORT, CKadDlg::OnFieldsChange)

	EVT_BUTTON(ID_NODECONNECT, CKadDlg::OnBnClickedBootstrapClient)
	EVT_BUTTON(ID_KNOWNNODECONNECT, CKadDlg::OnBnClickedBootstrapKnown)
	
	EVT_LIST_ITEM_SELECTED(ID_NODELIST, CKadDlg::OnNodeListItemSelected)
	
	EVT_RIGHT_DOWN(CKadDlg::OnRMButton)

END_EVENT_TABLE()



CKadDlg::CKadDlg(wxWindow* pParent) : wxPanel(pParent, -1) {

	wxSizer* content=KadDlg(this, true);
	content->Show(this, true);

	NodesList = (wxListCtrl*)FindWindowById(ID_NODELIST);
	wxASSERT( NodesList );

	NodesList->InsertColumn(0,_("Id"),wxLIST_FORMAT_LEFT,140);
	NodesList->InsertColumn(1,_("Type"),wxLIST_FORMAT_LEFT,50);
	NodesList->InsertColumn(2,_("Distance"),wxLIST_FORMAT_LEFT,220);
	
}

// Enables or disables the node connect button depending on the conents of the text fields
void	CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields 
	int textfields[] = { ID_NODE_IP1, ID_NODE_IP2, ID_NODE_IP3, ID_NODE_IP4, ID_NODE_PORT};

	bool enable = false;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable &= !((wxTextCtrl*)FindWindowById( textfields[i] ))->GetValue().IsEmpty();
	}
	
	// Enable the node connect button if all fields contain text
	FindWindowById(ID_NODECONNECT)->Enable( enable );
}

void	CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& WXUNUSED(evt)) {
	if (FindWindowById(ID_NODECONNECT)->IsEnabled()) {
		// Connect to node
	} else {
		wxMessageBox(_("Please fill all fields required"));
	}
}

void	CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& WXUNUSED(evt)) {
	
}

void	CKadDlg::OnNodeListItemSelected(wxListEvent& WXUNUSED(evt)) {
	
}

void	CKadDlg::OnRMButton(wxMouseEvent& WXUNUSED(evt)) {

}
