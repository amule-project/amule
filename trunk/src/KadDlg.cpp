// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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


// KadDlg.cpp : implementation file
//

#include <wx/textctrl.h>
#include "KadDlg.h"
#include "muuli_wdr.h"

#include "CMD4Hash.h"
#include "otherfunctions.h"
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
	EVT_LIST_ITEM_SELECTED(ID_KADSEARCHLIST, CKadDlg::OnKadSearchListItemSelected)
	
	EVT_RIGHT_DOWN(CKadDlg::OnRMButton)

END_EVENT_TABLE()



CKadDlg::CKadDlg(wxWindow* pParent) : wxPanel(pParent, -1) {

	wxSizer* content=KadDlg(this, true);
	content->Show(this, true);

	NodesList = (wxListCtrl*)FindWindowById(ID_NODELIST);
	wxASSERT( NodesList );

	CurrentKadSearches = (wxListCtrl*)FindWindowById(ID_KADSEARCHLIST);
	wxASSERT( CurrentKadSearches );
	
}

// Enables or disables the node connect button depending on the conents of the text fields
void	CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields 
	int textfields[] = { ID_NODE_IP1, ID_NODE_IP2, ID_NODE_IP3, ID_NODE_IP4, ID_NODE_PORT};

	bool enable = false;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable &= ((wxTextCtrl*)FindWindowById( textfields[i] ))->GetLineLength(0);
	}
	
	// Enable the node connect button if any fields contain text
	FindWindowById(ID_NODECONNECT)->Enable( enable );
}

void	CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& evt) {
	if (FindWindowById(ID_NODECONNECT)->IsEnabled()) {
		// Connect to node
	} else {
		wxMessageBox(_("Please fill all fields required"));
	}
}

void	CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& evt) {
	
}

void	CKadDlg::OnNodeListItemSelected(wxListEvent& evt) {
	
}

void	CKadDlg::OnKadSearchListItemSelected(wxListEvent& evt) {
	
}

void	CKadDlg::OnRMButton(wxMouseEvent& evt) {

}
